/*	SCCS Id: @(#)heaputil.c	3.3	94/07/17	*/
/* Copyright (c) Michael Allison, Toronto, 1994			  */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * We want to know the following information:
 *
 *	1. allocations that are made but never freed, and where they occur.
 *	2. attempts to free unallocated blocks.
 *	3. the peak amount of heap space allocated during the program.
 *
 */

#include "config.h"
#include <ctype.h>

#ifdef MICRO
#include <stdlib.h>
#endif

#ifdef MICRO		/* see src/alloc.c */
# define MONITOR_HEAP_FMT
#endif

#ifdef MONITOR_HEAP_FMT
# define PTR_FMT "%p"
# define PTR_TYP genericptr_t	/* (void *) */
#else
# define PTR_FMT "%lx"
# define PTR_TYP unsigned long
#endif

extern genericptr_t FDECL(malloc,(size_t));
#ifdef free
#undef free
#endif
extern void FDECL(free,(genericptr_t));

#define quit() exit(EXIT_FAILURE)

static void NDECL(out_of_memory);
static void FDECL(doline,(char *));
static void FDECL(chain,(char *));
static void FDECL(unchain,(char *));
static void NDECL(walkblocks);
static struct memblock *FDECL(findprev,(PTR_TYP));
static void FDECL(btempl,(char *));

#ifdef VMS
static FILE *vms_fopen(name, mode) const char *name, *mode;
{
	return fopen(name, mode, "mbc=64", "shr=nil");
}
# define fopen(f,m) vms_fopen(f,m)
#endif


#define DEFAULTNAME "heapuse.log"
#define MAXERR 4
#ifndef _MAX_PATH
#define _MAX_PATH  120
#endif

struct memblock {
	struct memblock *next;
	long sequence, bsize;
	PTR_TYP address;
	char fileinfo[5 + _MAX_PATH + 1];
};

/* a cheap way to try to split addresses into two categories */
#define CHAIN_SELECT(X) (((unsigned long)X >> 10) & 1)

#define OPERATION 0		/* first character; + for alloc, - for free */
#define BLKSIZE	  1		/* block size; 5 digits starting at 2nd char */
#define ADDRESS	  7		/* sizeof "+12345 " - sizeof "" */
#define FILEINFO  fileinfo_offset

#define terminate_fields(a)	\
	a[ADDRESS - 1] = a[FILEINFO - 1] = '\0'
#define zero_fields(a)		\
	a[OPERATION] = a[BLKSIZE] = a[ADDRESS] = a[FILEINFO] = '\0'

static int fileinfo_offset;
static struct memblock *firstblock[2] = {0,0}, *blkcache = 0;
static long peakmem;
static long curmem;
static long totaldynmem;
static long lineno;
static FILE *infile;
static char line[255];
static const char *infilenm;
static int errcount;
static int have_template;

int main(argc, argv)
int argc;
char *argv[];
{
	infilenm = (argc < 2) ? getenv("NH_HEAPLOG") : argv[1];
	if (!infilenm || !*infilenm) infilenm = DEFAULTNAME;

	infile = fopen(infilenm,"r");
	if (!infile) {
		printf("%s not found or unavailable\n",infilenm);
		quit();
	}

	while (fgets(line, sizeof line, infile)) {
		++lineno;
		doline(line);
		zero_fields(line);
	}

	fclose(infile);
	walkblocks();
	printf("Peak heap usage was %ld bytes.\n",peakmem);
	printf("Total heap memory allocations was %ld bytes.\n", totaldynmem);
	exit(EXIT_SUCCESS);
	/*NOTREACHED*/
	return 0;
}

static void out_of_memory()
{
	printf("heaputil: out of memory at line %ld of %s\n",
		lineno, infilenm);
	quit();
}

/*
 * Sample:
+   43 6852:0A0A  568 ..\win\tty\wintty.c
-      6852:0A0A 1291 ..\win\tty\wintty.c
 * Description:
^ size  address  line file	^: + => alloc, - => free
 *
 * Note: In environments other than MSDOS 16 bit segmented ones,
 *	 the address field will just be a single string of digits.
 *
 * Parsing:
 *	The operation flag and allocation size are in fixed columns;
 *	the address starts at a fixed offset and its width is expected
 *	to be the same for every entry, but that width may vary from
 *	platform to platform; the line number is a right-justified
 *	4-digit number separated from the address by one space and
 *	followed after a space by the filename; line+file+newline
 *	are treated as a single unit throughout.
 *
 *	Only the first line obtained from the file will be scanned
 *	to determine the offsets of the various fields:  operation,
 *	size, address, and fileinfo.  Subsequent lines are processed
 *	by depositing NUL at the appropriate offsets in the string.
 *	This method seems to be quite fast and avoids complicated
 *	parsing.
 */

static void doline(line)
char *line;
{
	if (!have_template) btempl(line);
	if (line[OPERATION] == '+') chain(line);
	else if (line[OPERATION] == '-') unchain(line);
	else {
		printf("%6ld: invalid operation, badly formatted line.\n",
			lineno);
		printf(" -> %s",line);
		if (++errcount > MAXERR) {
		   printf("Giving up on this file, its not right.\n");
		   quit();
		}
	}
}

static void chain(line)
char *line;
{
	struct memblock *block;
	int i;

	terminate_fields(line);
	if (blkcache) {
		block = blkcache;
		blkcache = block->next;
	} else {
		block = (struct memblock *)malloc(sizeof(struct memblock));
		if (!block) out_of_memory();
	}

	block->sequence = lineno;
	block->bsize = atol(&line[BLKSIZE]);
	(void)sscanf(&line[ADDRESS], PTR_FMT, &block->address);
	Strcpy(block->fileinfo, &line[FILEINFO]);
	if (block->address == 0) {
		printf("%6ld: allocation of %ld bytes returned null pointer, %s",
			block->sequence, block->bsize, block->fileinfo);
		return;
	}

	i = CHAIN_SELECT(block->address);
	block->next = firstblock[i];
	firstblock[i] = block;

	curmem += block->bsize;
	if (curmem > peakmem) peakmem = curmem;
	totaldynmem += block->bsize;
}

static void unchain(line)
char *line;
{
	struct memblock *block;
	struct memblock *rmblock = (struct memblock *)0;
	PTR_TYP address;
	int i;

	terminate_fields(line);
	(void)sscanf(&line[ADDRESS], PTR_FMT, &address);
	i = CHAIN_SELECT(address);

	if (address == 0) {
		printf("%6ld: attempt to free null pointer, %s",
			lineno, &line[FILEINFO]);
		return;
	} else if (firstblock[i]) {
		/* special case, first one so no prev available */
		if (address == firstblock[i]->address) {
			rmblock = firstblock[i];
			firstblock[i] = rmblock->next;
		} else {
			block = findprev(address);
			if (block) {
				rmblock = block->next;
				block->next = rmblock->next;
			}
		}
	}

	if (!rmblock) {
		printf("%6ld: attempt to free unallocated block, %s",
			lineno, &line[FILEINFO]);
	} else {
		curmem -= rmblock->bsize;
		if (curmem < 0) {
			printf("%6ld: impossible, current memory use negative, %s",
				lineno, &line[FILEINFO]);
		}
		rmblock->next = blkcache;
		blkcache = rmblock;
	}
}

static struct memblock *
findprev(maddress)
PTR_TYP maddress;
{
	struct memblock *tmpblk, *nextblk;

	tmpblk = firstblock[CHAIN_SELECT(maddress)];
	while (tmpblk) {
		nextblk = tmpblk->next;
		if (nextblk) {
			if (maddress == nextblk->address) return tmpblk;
		}
		tmpblk = nextblk;
	}
	return (struct memblock *)0;
}

static void walkblocks()
{
	struct memblock *tmpblk, *nextblk;
	int i, k;
	long unfreedmem = 0L;

	/* make a sorted list from all the chains; expected to be short */
	tmpblk = 0;
	do {
		k = 0;
		for (i = 1; i < SIZE(firstblock); i++)
			if (!firstblock[k] || (firstblock[i] &&
			    firstblock[i]->sequence > firstblock[k]->sequence))
				k = i;
		nextblk = firstblock[k];
		if (nextblk) {
			firstblock[k] = nextblk->next;
			nextblk->next = tmpblk;
			tmpblk = nextblk;
		}
	} while (nextblk);

	while (tmpblk) {
		printf("%6ld: not freed: %5ld bytes, %s",
			tmpblk->sequence, tmpblk->bsize, tmpblk->fileinfo);
		unfreedmem += tmpblk->bsize;
		nextblk = tmpblk->next;
		free((genericptr_t)tmpblk);
		tmpblk = nextblk;
	}

	printf("Total of %ld bytes not freed before exit.\n", unfreedmem);

	/* free memblock cache */
	while (blkcache) {
		tmpblk = blkcache;
		blkcache = tmpblk->next;
		free((genericptr_t)tmpblk);
	}
}

static void btempl(line)
char *line;
{
	char *p;

	p = &line[ADDRESS - 1];
	while (isspace(*p)) ++p;
	while (*p && *p != ' ' && *p != '\n') ++p;
	if (*p == ' ') ++p;		/* skip first space */
	fileinfo_offset = p - line;

	++have_template;
}

/*heaputil.c*/
