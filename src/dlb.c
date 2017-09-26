/* NetHack 3.6	dlb.c	$NHDT-Date: 1446975464 2015/11/08 09:37:44 $  $NHDT-Branch: master $:$NHDT-Revision: 1.15 $ */
/* Copyright (c) Kenneth Lorber, Bethesda, Maryland, 1993. */
/* NetHack may be freely redistributed.  See license for details. */

#include "config.h"
#include "dlb.h"

#ifdef __DJGPP__
#include <string.h>
#endif

#define DATAPREFIX 4

#if defined(OVERLAY)
#define STATIC_DCL extern
#define STATIC_OVL
#else /* !OVERLAY */
#define STATIC_DCL static
#define STATIC_OVL static
#endif /* OVERLAY */

#ifdef DLB
/*
 * Data librarian.  Present a STDIO-like interface to NetHack while
 * multiplexing on one or more "data libraries".  If a file is not found
 * in a given library, look for it outside the libraries.
 */

typedef struct dlb_procs {
    boolean NDECL((*dlb_init_proc));
    void NDECL((*dlb_cleanup_proc));
    boolean FDECL((*dlb_fopen_proc), (DLB_P, const char *, const char *));
    int FDECL((*dlb_fclose_proc), (DLB_P));
    int FDECL((*dlb_fread_proc), (char *, int, int, DLB_P));
    int FDECL((*dlb_fseek_proc), (DLB_P, long, int));
    char *FDECL((*dlb_fgets_proc), (char *, int, DLB_P));
    int FDECL((*dlb_fgetc_proc), (DLB_P));
    long FDECL((*dlb_ftell_proc), (DLB_P));
} dlb_procs_t;

/* without extern.h via hack.h, these haven't been declared for us */
extern FILE *FDECL(fopen_datafile, (const char *, const char *, int));

#ifdef DLBLIB
/*
 * Library Implementation:
 *
 * When initialized, we open all library files and read in their tables
 * of contents.  The library files stay open all the time.  When
 * a open is requested, the libraries' directories are searched.  If
 * successful, we return a descriptor that contains the library, file
 * size, and current file mark.  This descriptor is used for all
 * successive calls.
 *
 * The ability to open more than one library is supported but used
 * only in the Amiga port (the second library holds the sound files).
 * For Unix, the idea would be to split the NetHack library
 * into text and binary parts, where the text version could be shared.
 */

#define MAX_LIBS 4
static library dlb_libs[MAX_LIBS];

STATIC_DCL boolean FDECL(readlibdir, (library * lp));
STATIC_DCL boolean FDECL(find_file, (const char *name, library **lib,
                                     long *startp, long *sizep));
STATIC_DCL boolean NDECL(lib_dlb_init);
STATIC_DCL void NDECL(lib_dlb_cleanup);
STATIC_DCL boolean FDECL(lib_dlb_fopen, (dlb *, const char *, const char *));
STATIC_DCL int FDECL(lib_dlb_fclose, (dlb *));
STATIC_DCL int FDECL(lib_dlb_fread, (char *, int, int, dlb *));
STATIC_DCL int FDECL(lib_dlb_fseek, (dlb *, long, int));
STATIC_DCL char *FDECL(lib_dlb_fgets, (char *, int, dlb *));
STATIC_DCL int FDECL(lib_dlb_fgetc, (dlb *));
STATIC_DCL long FDECL(lib_dlb_ftell, (dlb *));

/* not static because shared with dlb_main.c */
boolean FDECL(open_library, (const char *lib_name, library *lp));
void FDECL(close_library, (library * lp));

/* without extern.h via hack.h, these haven't been declared for us */
extern char *FDECL(eos, (char *));

/*
 * Read the directory out of the library.  Return 1 if successful,
 * 0 if it failed.
 *
 * NOTE: An improvement of the file structure should be the file
 * size as part of the directory entry or perhaps in place of the
 * offset -- the offset can be calculated by a running tally of
 * the sizes.
 *
 * Library file structure:
 *
 * HEADER:
 * %3ld library FORMAT revision (currently rev 1)
 * %1c  space
 * %8ld # of files in archive (includes 1 for directory)
 * %1c  space
 * %8ld size of allocation for string space for directory names
 * %1c  space
 * %8ld library offset - sanity check - lseek target for start of first file
 * %1c  space
 * %8ld size - sanity check - byte size of complete archive file
 *
 * followed by one DIRECTORY entry for each file in the archive, including
 *  the directory itself:
 * %1c  handling information (compression, etc.)  Always ' ' in rev 1.
 * %s   file name
 * %1c  space
 * %8ld offset in archive file of start of this file
 * %c   newline
 *
 * followed by the contents of the files
 */
#define DLB_MIN_VERS 1 /* min library version readable by this code */
#define DLB_MAX_VERS 1 /* max library version readable by this code */

/*
 * Read the directory from the library file.   This will allocate and
 * fill in our globals.  The file pointer is reset back to position
 * zero.  If any part fails, leave nothing that needs to be deallocated.
 *
 * Return TRUE on success, FALSE on failure.
 */
STATIC_OVL boolean
readlibdir(lp)
library *lp; /* library pointer to fill in */
{
    int i;
    char *sp;
    long liboffset, totalsize;

    if (fscanf(lp->fdata, "%ld %ld %ld %ld %ld\n", &lp->rev, &lp->nentries,
               &lp->strsize, &liboffset, &totalsize) != 5)
        return FALSE;
    if (lp->rev > DLB_MAX_VERS || lp->rev < DLB_MIN_VERS)
        return FALSE;

    lp->dir = (libdir *) alloc(lp->nentries * sizeof(libdir));
    lp->sspace = (char *) alloc(lp->strsize);

    /* read in each directory entry */
    for (i = 0, sp = lp->sspace; i < lp->nentries; i++) {
        lp->dir[i].fname = sp;
        if (fscanf(lp->fdata, "%c%s %ld\n", &lp->dir[i].handling, sp,
                   &lp->dir[i].foffset) != 3) {
            free((genericptr_t) lp->dir);
            free((genericptr_t) lp->sspace);
            lp->dir = (libdir *) 0;
            lp->sspace = (char *) 0;
            return FALSE;
        }
        sp = eos(sp) + 1;
    }

    /* calculate file sizes using offset information */
    for (i = 0; i < lp->nentries; i++) {
        if (i == lp->nentries - 1)
            lp->dir[i].fsize = totalsize - lp->dir[i].foffset;
        else
            lp->dir[i].fsize = lp->dir[i + 1].foffset - lp->dir[i].foffset;
    }

    (void) fseek(lp->fdata, 0L, SEEK_SET); /* reset back to zero */
    lp->fmark = 0;

    return TRUE;
}

/*
 * Look for the file in our directory structure.  Return 1 if successful,
 * 0 if not found.  Fill in the size and starting position.
 */
STATIC_OVL boolean
find_file(name, lib, startp, sizep)
const char *name;
library **lib;
long *startp, *sizep;
{
    int i, j;
    library *lp;

    for (i = 0; i < MAX_LIBS && dlb_libs[i].fdata; i++) {
        lp = &dlb_libs[i];
        for (j = 0; j < lp->nentries; j++) {
            if (FILENAME_CMP(name, lp->dir[j].fname) == 0) {
                *lib = lp;
                *startp = lp->dir[j].foffset;
                *sizep = lp->dir[j].fsize;
                return TRUE;
            }
        }
    }
    *lib = (library *) 0;
    *startp = *sizep = 0;
    return FALSE;
}

/*
 * Open the library of the given name and fill in the given library
 * structure.  Return TRUE if successful, FALSE otherwise.
 */
boolean
open_library(lib_name, lp)
const char *lib_name;
library *lp;
{
    boolean status = FALSE;

    lp->fdata = fopen_datafile(lib_name, RDBMODE, DATAPREFIX);
    if (lp->fdata) {
        if (readlibdir(lp)) {
            status = TRUE;
        } else {
            (void) fclose(lp->fdata);
            lp->fdata = (FILE *) 0;
        }
    }
    return status;
}

void
close_library(lp)
library *lp;
{
    (void) fclose(lp->fdata);
    free((genericptr_t) lp->dir);
    free((genericptr_t) lp->sspace);

    (void) memset((char *) lp, 0, sizeof(library));
}

/*
 * Open the library file once using stdio.  Keep it open, but
 * keep track of the file position.
 */
STATIC_OVL boolean
lib_dlb_init(VOID_ARGS)
{
    /* zero out array */
    (void) memset((char *) &dlb_libs[0], 0, sizeof(dlb_libs));

    /* To open more than one library, add open library calls here. */
    if (!open_library(DLBFILE, &dlb_libs[0]))
        return FALSE;
#ifdef DLBFILE2
    if (!open_library(DLBFILE2, &dlb_libs[1])) {
        close_library(&dlb_libs[0]);
        return FALSE;
    }
#endif
    return TRUE;
}

STATIC_OVL void
lib_dlb_cleanup(VOID_ARGS)
{
    int i;

    /* close the data file(s) */
    for (i = 0; i < MAX_LIBS && dlb_libs[i].fdata; i++)
        close_library(&dlb_libs[i]);
}

/*ARGSUSED*/
STATIC_OVL boolean
lib_dlb_fopen(dp, name, mode)
dlb *dp;
const char *name;
const char *mode UNUSED;
{
    long start, size;
    library *lp;

    /* look up file in directory */
    if (find_file(name, &lp, &start, &size)) {
        dp->lib = lp;
        dp->start = start;
        dp->size = size;
        dp->mark = 0;
        return TRUE;
    }

    return FALSE; /* failed */
}

/*ARGUSED*/
STATIC_OVL int
lib_dlb_fclose(dp)
dlb *dp UNUSED;
{
    /* nothing needs to be done */
    return 0;
}

STATIC_OVL int
lib_dlb_fread(buf, size, quan, dp)
char *buf;
int size, quan;
dlb *dp;
{
    long pos, nread, nbytes;

    /* make sure we don't read into the next file */
    if ((dp->size - dp->mark) < (size * quan))
        quan = (dp->size - dp->mark) / size;
    if (quan == 0)
        return 0;

    pos = dp->start + dp->mark;
    if (dp->lib->fmark != pos) {
        fseek(dp->lib->fdata, pos, SEEK_SET); /* check for error??? */
        dp->lib->fmark = pos;
    }

    nread = fread(buf, size, quan, dp->lib->fdata);
    nbytes = nread * size;
    dp->mark += nbytes;
    dp->lib->fmark += nbytes;

    return nread;
}

STATIC_OVL int
lib_dlb_fseek(dp, pos, whence)
dlb *dp;
long pos;
int whence;
{
    long curpos;

    switch (whence) {
    case SEEK_CUR:
        curpos = dp->mark + pos;
        break;
    case SEEK_END:
        curpos = dp->size - pos;
        break;
    default: /* set */
        curpos = pos;
        break;
    }
    if (curpos < 0)
        curpos = 0;
    if (curpos > dp->size)
        curpos = dp->size;

    dp->mark = curpos;
    return 0;
}

STATIC_OVL char *
lib_dlb_fgets(buf, len, dp)
char *buf;
int len;
dlb *dp;
{
    int i;
    char *bp, c = 0;

    if (len <= 0)
        return buf; /* sanity check */

    /* return NULL on EOF */
    if (dp->mark >= dp->size)
        return (char *) 0;

    len--; /* save room for null */
    for (i = 0, bp = buf; i < len && dp->mark < dp->size && c != '\n';
         i++, bp++) {
        if (dlb_fread(bp, 1, 1, dp) <= 0)
            break; /* EOF or error */
        c = *bp;
    }
    *bp = '\0';

#if defined(MSDOS) || defined(WIN32)
    if ((bp = index(buf, '\r')) != 0) {
        *bp++ = '\n';
        *bp = '\0';
    }
#endif

    return buf;
}

STATIC_OVL int
lib_dlb_fgetc(dp)
dlb *dp;
{
    char c;

    if (lib_dlb_fread(&c, 1, 1, dp) != 1)
        return EOF;
    return (int) c;
}

STATIC_OVL long
lib_dlb_ftell(dp)
dlb *dp;
{
    return dp->mark;
}

const dlb_procs_t lib_dlb_procs = { lib_dlb_init,  lib_dlb_cleanup,
                                    lib_dlb_fopen, lib_dlb_fclose,
                                    lib_dlb_fread, lib_dlb_fseek,
                                    lib_dlb_fgets, lib_dlb_fgetc,
                                    lib_dlb_ftell };

#endif /* DLBLIB */

#ifdef DLBRSRC
const dlb_procs_t rsrc_dlb_procs = { rsrc_dlb_init,  rsrc_dlb_cleanup,
                                     rsrc_dlb_fopen, rsrc_dlb_fclose,
                                     rsrc_dlb_fread, rsrc_dlb_fseek,
                                     rsrc_dlb_fgets, rsrc_dlb_fgetc,
                                     rsrc_dlb_ftell };
#endif

/* Global wrapper functions ------------------------------------------------
 */

#define do_dlb_init (*dlb_procs->dlb_init_proc)
#define do_dlb_cleanup (*dlb_procs->dlb_cleanup_proc)
#define do_dlb_fopen (*dlb_procs->dlb_fopen_proc)
#define do_dlb_fclose (*dlb_procs->dlb_fclose_proc)
#define do_dlb_fread (*dlb_procs->dlb_fread_proc)
#define do_dlb_fseek (*dlb_procs->dlb_fseek_proc)
#define do_dlb_fgets (*dlb_procs->dlb_fgets_proc)
#define do_dlb_fgetc (*dlb_procs->dlb_fgetc_proc)
#define do_dlb_ftell (*dlb_procs->dlb_ftell_proc)

static const dlb_procs_t *dlb_procs;
static boolean dlb_initialized = FALSE;

boolean
dlb_init()
{
    if (!dlb_initialized) {
#ifdef DLBLIB
        dlb_procs = &lib_dlb_procs;
#endif
#ifdef DLBRSRC
        dlb_procs = &rsrc_dlb_procs;
#endif

        if (dlb_procs)
            dlb_initialized = do_dlb_init();
    }

    return dlb_initialized;
}

void
dlb_cleanup()
{
    if (dlb_initialized) {
        do_dlb_cleanup();
        dlb_initialized = FALSE;
    }
}

dlb *
dlb_fopen(name, mode)
const char *name, *mode;
{
    FILE *fp;
    dlb *dp;

    if (!dlb_initialized)
        return (dlb *) 0;

    /* only support reading; ignore possible binary flag */
    if (!mode || mode[0] != 'r')
        return (dlb *) 0;

    dp = (dlb *) alloc(sizeof(dlb));
    if (do_dlb_fopen(dp, name, mode))
        dp->fp = (FILE *) 0;
    else if ((fp = fopen_datafile(name, mode, DATAPREFIX)) != 0)
        dp->fp = fp;
    else {
        /* can't find anything */
        free((genericptr_t) dp);
        dp = (dlb *) 0;
    }

    return dp;
}

int
dlb_fclose(dp)
dlb *dp;
{
    int ret = 0;

    if (dlb_initialized) {
        if (dp->fp)
            ret = fclose(dp->fp);
        else
            ret = do_dlb_fclose(dp);

        free((genericptr_t) dp);
    }
    return ret;
}

int
dlb_fread(buf, size, quan, dp)
char *buf;
int size, quan;
dlb *dp;
{
    if (!dlb_initialized || size <= 0 || quan <= 0)
        return 0;
    if (dp->fp)
        return (int) fread(buf, size, quan, dp->fp);
    return do_dlb_fread(buf, size, quan, dp);
}

int
dlb_fseek(dp, pos, whence)
dlb *dp;
long pos;
int whence;
{
    if (!dlb_initialized)
        return EOF;
    if (dp->fp)
        return fseek(dp->fp, pos, whence);
    return do_dlb_fseek(dp, pos, whence);
}

char *
dlb_fgets(buf, len, dp)
char *buf;
int len;
dlb *dp;
{
    if (!dlb_initialized)
        return (char *) 0;
    if (dp->fp)
        return fgets(buf, len, dp->fp);
    return do_dlb_fgets(buf, len, dp);
}

int
dlb_fgetc(dp)
dlb *dp;
{
    if (!dlb_initialized)
        return EOF;
    if (dp->fp)
        return fgetc(dp->fp);
    return do_dlb_fgetc(dp);
}

long
dlb_ftell(dp)
dlb *dp;
{
    if (!dlb_initialized)
        return 0;
    if (dp->fp)
        return ftell(dp->fp);
    return do_dlb_ftell(dp);
}

#endif /* DLB */

/*dlb.c*/
