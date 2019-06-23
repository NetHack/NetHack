/* NetHack 3.7	sfstruct.c	$NHDT-Date: 1559994625 2019/06/08 11:50:25 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.121 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2009. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifdef TRACE_BUFFERING
#ifdef bufon
#undef bufon
#endif
#ifdef bufoff
#undef bufoff
#endif
#ifdef bflush
#undef bflush
#endif
#ifdef bwrite
#undef bwrite
#endif
#ifdef bclose
#undef bclose
#endif
#ifdef mread
#undef mread
#endif
#ifdef minit
#undef minit
#endif
void FDECL(newread, (NHFILE *, int, int, genericptr_t, unsigned int));
void FDECL(bufon, (int));
void FDECL(bufoff, (int));
void FDECL(bflush, (int));
void FDECL(bwrite, (int, genericptr_t, unsigned int));
void FDECL(mread, (int, genericptr_t, unsigned int));
void NDECL(minit);
void FDECL(bclose, (int));
#endif /* TRACE_BUFFERING */

#if defined(UNIX) || defined(WIN32)
#define USE_BUFFERING
#endif

/*
 * historical structlevel savefile writing and reading routines follow.
 * These were moved here from save.c and restore.c between 3.6.3 and 3.7.0.
 */

struct restore_info restoreinfo = {
	"externalcomp", 0,
};


static int bw_fd = -1;
static FILE *bw_FILE = 0;
static boolean buffering = FALSE;

void
bufon(fd)
    int fd;
{
#ifdef USE_BUFFERING
    if(bw_fd != fd) {
        if(bw_fd >= 0)
            panic("double buffering unexpected");
        bw_fd = fd;
        if((bw_FILE = fdopen(fd, "w")) == 0)
            panic("buffering of file %d failed", fd);
    }
#endif
    buffering = TRUE;
}

void
bufoff(fd)
int fd;
{
    bflush(fd);
    buffering = FALSE;
}

void
bwrite(fd, loc, num)
register int fd;
register genericptr_t loc;
register unsigned num;
{
    boolean failed;

#ifdef MFLOPPY
    bytes_counted += num;
    if (count_only)
        return;
#endif

#ifdef USE_BUFFERING
    if (buffering) {
        if (fd != bw_fd)
            panic("unbuffered write to fd %d (!= %d)", fd, bw_fd);

        failed = (fwrite(loc, (int) num, 1, bw_FILE) != 1);
    } else
#endif /* USE_BUFFERING */
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
}

void
bflush(fd)
int fd;
{
#ifdef USE_BUFFERING
    if (fd == bw_fd) {
        if (fflush(bw_FILE) == EOF)
            panic("flush of savefile failed!");
    }
#endif
    return;
}

void
bclose(fd)
int fd;
{
    bufoff(fd);
#ifdef USE_BUFFERING
    if (fd == bw_fd) {
        (void) fclose(bw_FILE);
        bw_fd = -1;
        bw_FILE = 0;
    } else
#endif
        (void) nhclose(fd);
    return;
}

void
minit()
{
    return;
}

void
mread(fd, buf, len)
register int fd;
register genericptr_t buf;
register unsigned int len;
{
    register int rlen;
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
            if (g.restoring) {
                (void) nhclose(fd);
                (void) delete_savefile();
                error("Error restoring old game.");
            }
            panic("Error reading level file.");
        }
    }
}

#ifdef TRACE_BUFFERING

static FILE *tracefile;
#define TFILE "trace-buffering.log"

#define TRACE(xx) \
    tracefile = fopen(TFILE, "a"); \
    (void) fprintf(tracefile, "%s from %s:%d (%d)\n", __FUNCTION__, fncname, linenum, xx); \
    fclose(tracefile);

void
Bufon(fd, fncname, linenum)
int fd;
const char *fncname;
int linenum;
{
    TRACE(fd);    
    bufon(fd);
}

void
Bufoff(fd, fncname, linenum)
int fd;
const char *fncname;
int linenum;
{
    TRACE(fd);    
    bufoff(fd);
}

void
Bflush(fd, fncname, linenum)
int fd;
const char *fncname;
int linenum;
{
    TRACE(fd);    
    bflush(fd);
}

void
Bwrite(fd, loc, num, fncname, linenum)
register int fd;
register genericptr_t loc;
register unsigned num;
const char *fncname;
int linenum;
{
    TRACE(fd);    
    bwrite(fd, loc, num);
}

void
Bclose(fd, fncname, linenum)
int fd;
const char *fncname;
int linenum;
{
    TRACE(fd);    
    bclose(fd);
}

void
Minit(fncname, linenum)
const char *fncname;
int linenum;
{
    TRACE(-1);
    minit();
}

void
Mread(fd, buf, len, fncname, linenum)
register int fd;
register genericptr_t buf;
register unsigned int len;
const char *fncname;
int linenum;
{
    TRACE(fd);    
    mread(fd, buf, len);
}
#endif

