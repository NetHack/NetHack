/*   SCCS Id: @(#)viewtib.c   3.3     94/03/20                      */
/*   Copyright (c) NetHack PC Development Team 1993, 1994           */
/*   NetHack may be freely redistributed.  See license for details. */
/*
 *   View a NetHack binary tile file (.tib file)
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef __GO32__
#include <unistd.h>
#define _far
#endif
#include <dos.h>
#if defined(_MSC_VER) || defined(__BORLANDC__)
#include <conio.h>
#endif

#pragma warning(disable:4309)	/* initializing */
#pragma warning(disable:4018)	/* signed/unsigned mismatch */
#pragma warning(disable:4131)	/* old style declarator */
#pragma warning(disable:4127)	/* conditional express. is constant */


#define VIDEO_BIOS  0x10
#define DOSCALL	    0x21

#define USHORT		unsigned short
#define MODE640x480	0x0012  /* Switch to VGA 640 x 480 Graphics mode */
#define MODETEXT	0x0003  /* Switch to Text mode 3 */

#define BACKGROUND_VGA_COLOR 0
#define SCREENWIDTH	640
#define SCREENHEIGHT	480
#define SCREENBYTES	80
#define VIDEOSEG	0xa000
#define VFPTRSEG	0x0000
#define VFPTROFF	0x010C
#define SCREENPLANES	4
#define COLORDEPTH	16
#define egawriteplane(n)	{ outportb(0x3c4,2); outportb(0x3c5,n); }
#define col2x8(c)	((c) * 8) 
#define col2x16(c)	((c) * 16)
#define col2x(c)	((c) * 2)
#define row2y(c)	((c) * 16)
	
#define ROWS_PER_TILE	16
#define COLS_PER_TILE   16
#define EMPTY_TILE	-1
#define TIBHEADER_SIZE 1024	/* Use this for size, allows expansion */
#define PLANAR_STYLE	0
#define PIXEL_STYLE	1
#define DJGPP_COMP	0
#define MSC_COMP	1
#define BC_COMP		2
#define OTHER_COMP	10

struct tibhdr_struct {
	char  ident[80];	/* Identifying string           */
	char  timestamp[26];	/* Ascii timestamp              */
	char  tilestyle;	/* 0 = planar, 1 = pixel        */
	char  compiler;		/* 0 = DJGPP, 1 = MSC, >1 other */
	short tilecount;	/* number of tiles in file      */
	short numcolors;	/* number of colors in palette  */
	char  palette[256 * 3];	/* palette                      */
};

struct tileplane {
	char image[ROWS_PER_TILE][2];
};

struct planar_tile_struct {
	struct tileplane plane[SCREENPLANES];
};


#ifndef MK_PTR
/*
 * Depending on environment, this is a macro to construct either:
 *
 *     -  a djgpp long 32 bit pointer from segment & offset values
 *     -  a far pointer from segment and offset values
 *
 */
# ifdef __GO32__
#  define MK_PTR(seg, offset) (void *)(0xE0000000+((((unsigned)seg << 4) \
     + (unsigned)offset)))
# else
#  define MK_PTR(seg, offset) (void __far *)(((unsigned long)seg << 16) \
     + (unsigned long)(unsigned)offset)
# endif /* __GO32__ */
#endif /* MK_PTR */

# ifdef __GO32__
#define __far
# endif
# ifdef _MSC_VER
#define outportb _outp
#define outportw _outpw
# endif

#ifdef __BORLANDC__
#define MEMCPY(dest,src,n)      _fmemcpy(dest,src,n)
#else
#define MEMCPY(dest,src,n)      memcpy(dest,src,n)
#endif

void vga_SwitchMode(unsigned int);
void vga_SetPalette(char *);
/* void vga_NoBorder(int); */
int  vga_vgaCheck(void);
char __far  *vga_FontPtrs(void);
void vga_WriteStr(char *,int, int, int, int);
void vga_WriteChar(int, int, int, int);
void vga_ClearScreen(int);
void vga_DisplayTile(struct planar_tile_struct *, int, int);
int  OpenTileFile(char *);
void CloseTileFile(void);
int  ReadTileFile(int, struct planar_tile_struct *);
int  ReadTileFileHeader(struct tibhdr_struct *);

char   __far *screentable[SCREENHEIGHT];
char   tmp[SCREENWIDTH];
int    vp[4];
char   masktable[8]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
int    curcol,currow;
char   *paletteptr;
struct planar_tile_struct ptile;
FILE   *tilefile;
struct tibhdr_struct tibheader;
char   __far *font;

main(argc,argv)
	int argc;
	char *argv[];
{
	unsigned int i;
	int row,col;
	int gcount;
	char buf[80];
	
	puts("VIEW NetHack Tile File version 1.1\n\n");

	if(argc <= 1) {
	    printf("Please provide a path to a .TIB file\n");
	    exit(1);
	}
	
	/* make the argument upper case */
	strupr(argv[1]);

	for (i=0; i < SCREENHEIGHT; ++i) {
		screentable[i]=MK_PTR(VIDEOSEG, (i * SCREENBYTES));
	}
	vga_SwitchMode(MODE640x480);
	font = vga_FontPtrs();
/*	vga_NoBorder(BACKGROUND_VGA_COLOR); */
	OpenTileFile(argv[1]);
	if (ReadTileFileHeader(&tibheader)) {
		printf("Error reading %s header",argv[1]);
		exit(1);
	}
	paletteptr = tibheader.palette;
	vga_SetPalette(paletteptr);
	vga_ClearScreen(BACKGROUND_VGA_COLOR);
	vp[0] = 8;
	vp[1] = 4;
	vp[2] = 2;
	vp[3] = 1;
	gcount = 0;
	for (row = 0; row < 30; ++row) {
	   	for (col = 0; col < 40; ++col) {
			ReadTileFile(gcount++,&ptile);
	   		vga_DisplayTile(&ptile,col,row);
	   		if (gcount >= tibheader.tilecount) break;
	   	}
	   	if (gcount >= tibheader.tilecount) break;
	}
	if (row < 26) {
		char *compiler;
		++row;
		col = 0;
		vga_WriteStr(tibheader.ident,strlen(tibheader.ident)
			,col,row,9);
			
		sprintf(buf,"Created %s",tibheader.timestamp);
		++row;
		vga_WriteStr(buf,strlen(buf),col,row,9);

		sprintf(buf,"%s style, %d colors",
			tibheader.tilestyle ? "Pixel" : "Planar",
			tibheader.numcolors);
		++row;
		vga_WriteStr(buf,strlen(buf),col,row,9);

		if (tibheader.compiler == MSC_COMP)
			compiler = "Microsoft C";
		else if (tibheader.compiler == BC_COMP)
			compiler = "Borland C";
		else if (tibheader.compiler == DJGPP_COMP)
			compiler = "djgpp";
		else
			compiler = "unknown";
		
		sprintf(buf,"Written by %s compiler",compiler);
		++row;
		vga_WriteStr(buf,strlen(buf),col,row,9);
	}
	CloseTileFile();
	getch();
	vga_SwitchMode(MODETEXT);
	puts("Ok");
	return 0;
}

void vga_SwitchMode(unsigned int mode)
{
	union REGS regs;
	
	regs.x.ax = mode;
	(void) int86(VIDEO_BIOS, &regs, &regs);
}

#if 0
void vga_NoBorder(int bc)
{
	union REGS regs;

	regs.h.ah = (char)0x10;
	regs.h.al = (char)0x01;
	regs.h.bh = (char)bc;
	regs.h.bl = 0;
	(void) int86(VIDEO_BIOS, &regs, &regs);	
}
#endif

void
vga_DisplayTile(gp,x,y)
struct planar_tile_struct *gp;
int x,y;
{
	int i,pixx,pixy;
	char __far *tmp;
	int vplane;
	
	pixy = row2y(y);		/* convert to pixels */
	pixx = col2x(x);

	for(vplane=0; vplane < 4; ++vplane) {
		egawriteplane(vp[vplane]);
		for(i=0;i < ROWS_PER_TILE; ++i) {
			tmp = screentable[i+pixy];
 			tmp += pixx;
                      MEMCPY(tmp,gp->plane[vplane].image[i],2);
		}
	}
	egawriteplane(15);
}

void
vga_SetPalette(p)
	char *p;
{
	union REGS regs;
	int i;

	outportb(0x3c6,0xff);
	for(i=0;i < COLORDEPTH; ++i) {
		outportb(0x3c8,i);
		outportb(0x3c9,(*p++) >> 2);
		outportb(0x3c9,(*p++) >> 2);
		outportb(0x3c9,(*p++) >> 2);
	}
	regs.x.bx = 0x0000;
	for(i=0;i < COLORDEPTH; ++i) {
		regs.x.ax = 0x1000;
		(void) int86(VIDEO_BIOS,&regs,&regs);
		regs.x.bx += 0x0101;
	}
}

int ReadTileFile(tilenum,gp)
int tilenum;
struct planar_tile_struct *gp;
{
	long fpos;
	
	fpos = ((long)(tilenum) * (long)sizeof(struct planar_tile_struct)) +
		(long)TIBHEADER_SIZE;
	if (fseek(tilefile,fpos,SEEK_SET)) {
		return 1;
	} else {
	  	fread(gp, sizeof(struct planar_tile_struct), 1, tilefile);
	}
	return 0;
}

int ReadTileFileHeader(tibhdr)
struct tibhdr_struct *tibhdr;
{
	if (fseek(tilefile,0L,SEEK_SET)) {
		return 1;
	} else {
	  	fread(tibhdr, sizeof(struct tibhdr_struct), 1, tilefile);
	}
	return 0;
} 

int
OpenTileFile(fname)
char *fname;
{
	tilefile = fopen(fname,"rb");
	if (tilefile == (FILE *)0) {
		printf("Unable to open tile file %s\n",
				fname);
		return 1;
	}
	return 0;
}

void
CloseTileFile()
{
	fclose(tilefile);   
}

char __far  *vga_FontPtrs(void)
{
	USHORT  __far *tmp;
	char __far *retval;
	USHORT fseg, foff;
	tmp  = (USHORT __far *)MK_PTR(((USHORT)VFPTRSEG),((USHORT)VFPTROFF));
	foff = *tmp++;
	fseg = *tmp;
	retval = (char __far *)MK_PTR(fseg,foff);
	return retval;
}

void 
vga_WriteChar(ch,x,y,colour)
int ch,x,y,colour;
{
	char __far *cp;

	int  i,mx,my;
	int pixx,pixy;
	int chr,floc;
	char volatile tc;
	
	mx = x;
	my = y;
	chr = ch;
	if (chr < 32) chr = ' ';

	outportb(0x3ce,5);
	outportb(0x3cf,2);
	pixy = row2y(my);
	pixx = col2x8(mx);
			
	for (i=0; i < 16; ++i) {
		cp = screentable[pixy+i];
		cp += (pixx >> 3);
		floc = (chr<<4)+i;			
		outportb(0x3ce,8);
		outportb(0x3cf,font[floc]);
		tc = *cp;	/* wrt mode 2, must read, then write */
		*cp = (char)colour;
		outportb(0x3ce,8);
		outportb(0x3cf,~font[floc]);
		tc = *cp;	/* wrt mode 2, must read, then write */
		*cp = (char)BACKGROUND_VGA_COLOR;
	}
	if (mx < (80 - 1 )) ++mx;
	outportb(0x3ce,5);
	outportb(0x3cf,0);
	outportb(0x3ce,8);
	outportb(0x3cf,255);
}

void 
vga_WriteStr(s,len,x,y,colour)
char *s;
int len,x,y,colour;
{
	int i,mx,my;
	char *cp;

	cp = s;
	i  = 0;
	mx = x;
	my = y;
	while( (*cp != 0) && (i < len) && (mx < (79))) {
		vga_WriteChar(*cp,mx,my,colour);
		++cp;
		++i;
		++mx;
	}
}

void vga_ClearScreen(colour)
int colour;
{
	int y,j;
	char a;
	char _far *pch;
	
	outportb(0x3ce,5);
	outportb(0x3cf,2);

	for (y=0; y < SCREENHEIGHT; ++y) {
		pch = screentable[y];
		for (j=0; j < SCREENBYTES; ++j) {
			outportb(0x3ce,8);
			outportb(0x3cf,255);
			a = pch[j];     
			pch[j] = (char)colour;
		}
	}
	outportb(0x3ce,5);
	outportb(0x3cf,0);
}
