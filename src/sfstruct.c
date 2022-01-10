/* NetHack 3.7	sfstruct.c	$NHDT-Date: 1606765215 2020/11/30 19:40:15 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.4 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2009. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/*
 * historical structlevel savefile writing and reading routines follow.
 * These were moved here from save.c and restore.c between 3.6.3 and 3.7.0.
 */

static int getidx(int, int);

#if defined(UNIX) || defined(WIN32)
#define USE_BUFFERING
#endif

struct restore_info restoreinfo = {
    "externalcomp", 0,
};

#define MAXFD 5
enum {NOFLG = 0, NOSLOT = 1};
static int bw_sticky[MAXFD] = {-1,-1,-1,-1,-1};
static int bw_buffered[MAXFD] = {0,0,0,0,0};
#ifdef USE_BUFFERING
static FILE *bw_FILE[MAXFD] = {0,0,0,0,0};
#endif

/*
 * Presumably, the fdopen() to allow use of stdio fwrite()
 * over write() was done for performance or functionality
 * reasons to help some particular platform long ago.
 *
 * There have been some issues being encountered with the
 * implementation due to having an individual set of
 * tracking variables, even though there were nested
 * sets of open fd (like INSURANCE).
 *
 * This uses an individual tracking entry for each fd
 * being used.
 *
 * Some notes:
 *
 * Once buffered IO (stdio) has been enabled on the file
 * associated with a descriptor via fdopen():
 *
 *    1. If you use bufoff and bufon to try and toggle the
 *       use of write vs fwrite; the code just tracks which
 *       routine is to be called through the tracking
 *       variables and acts accordingly.
 *             bw_sticky[]    -  used to find the index number for
 *                               the fd that is stored in it, or -1
 *                               if it is a free slot.
 *             bw_buffered[]  -  indicator that buffered IO routines
 *                               are available for use.
 *             bw_FILE[]      -  the non-zero FILE * for use in calling
 *                               fwrite() when bw_buffered[] is also
 *                               non-zero.
 *
 *    2. It is illegal to call close(fd) after fdopen(), you
 *       must always use fclose() on the FILE * from
 *       that point on, so care must be taken to never call
 *       close(fd) on the underlying fd or bad things will
 *       happen.
 */

static int
getidx(int fd, int flg)
{
    int i, retval = -1;

    for (i = 0; i < MAXFD; ++i)
        if (bw_sticky[i] == fd)
            return i;
    if (flg == NOSLOT)
        return retval;
    for (i = 0; i < MAXFD; ++i)
        if (bw_sticky[i] < 0) {
            bw_sticky[i] = fd;
            retval = i;
            break;
        }
    return retval;
}

/* Let caller know that bclose() should handle it (TRUE) */
boolean
close_check(int fd)
{
    int idx = getidx(fd, NOSLOT);
    boolean retval = FALSE;

    if (idx >= 0)
        retval = TRUE;
    return retval;
}

void
bufon(int fd)
{
    int idx = getidx(fd, NOFLG);

    if (idx >= 0) {
        bw_sticky[idx] = fd;
#ifdef USE_BUFFERING
        if (bw_buffered[idx])
            panic("buffering already enabled");
        if (!bw_FILE[idx]) {
            if ((bw_FILE[idx] = fdopen(fd, "w")) == 0)
                panic("buffering of file %d failed", fd);
        }
        bw_buffered[idx] = (bw_FILE[idx] != 0);
#else
        bw_buffered[idx] = 1;
#endif
    }
}

void
bufoff(int fd)
{
    int idx = getidx(fd, NOFLG);

    if (idx >= 0) {
        bflush(fd);
        bw_buffered[idx] = 0;     /* just a flag that says "use write(fd)" */
    }
}

void
bclose(int fd)
{
    int idx = getidx(fd, NOSLOT);

    bufoff(fd);     /* sets bw_buffered[idx] = 0 */
    if (idx >= 0) {
#ifdef USE_BUFFERING
        if (bw_FILE[idx]) {
            (void) fclose(bw_FILE[idx]);
            bw_FILE[idx] = 0;
        } else
#endif
            close(fd);
        /* return the idx to the pool */
        bw_sticky[idx] = -1;
    }
    return;
}

void
bflush(int fd)
{
    int idx = getidx(fd, NOFLG);

    if (idx >= 0) {
#ifdef USE_BUFFERING
        if (bw_FILE[idx]) {
           if (fflush(bw_FILE[idx]) == EOF)
               panic("flush of savefile failed!");
        }
#endif
    }
    return;
}

void
bwrite(int fd, const genericptr_t loc, unsigned num)
{
    boolean failed;
    int idx = getidx(fd, NOFLG);

    if (idx >= 0) {
#ifdef USE_BUFFERING
        if (bw_buffered[idx] && bw_FILE[idx]) {
            failed = (fwrite(loc, (int) num, 1, bw_FILE[idx]) != 1);
        } else
#endif /* UNIX */
        {
            /* lint wants 3rd arg of write to be an int; lint -p an unsigned */
#if defined(BSD) || defined(ULTRIX) || defined(WIN32) || defined(_MSC_VER)
            failed = ((long) write(fd, loc, (int) num) != (long) num);
#else /* e.g. SYSV, __TURBOC__ */
            failed = ((long) write(fd, loc, num) != (long) num);
#endif
        }
        if (failed) {
#if defined(UNIX) || defined(VMS) || defined(__EMX__)
            if (g.program_state.done_hup)
                nh_terminate(EXIT_FAILURE);
            else
#endif
                panic("cannot write %u bytes to file #%d", num, fd);
        }
    } else
        impossible("fd not in list (%d)?", fd);
}

/*  ===================================================== */

void
minit(void)
{
    return;
}

void
mread(int fd, genericptr_t buf, unsigned len)
{
    int rlen;
#if defined(BSD) || defined(ULTRIX)
#define readLenType int
#else /* e.g. SYSV, __TURBOC__ */
#define readLenType unsigned
#endif

    rlen = read(fd, buf, (readLenType) len);
    if ((readLenType) rlen != (readLenType) len) {
        if (restoreinfo.mread_flags == 1) { /* means "return anyway" */
            restoreinfo.mread_flags = -1;
            return;
        } else {
            pline("Read %d instead of %u bytes.", rlen, len);
            if (g.program_state.restoring) {
                (void) nhclose(fd);
                (void) delete_savefile();
                error("Error restoring old game.");
            }
            panic("Error reading level file.");
        }
    }
}


