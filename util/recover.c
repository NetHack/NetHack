/* NetHack 3.7	recover.c	$NHDT-Date: 1658093138 2022/07/17 21:25:38 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.31 $ */
/*	Copyright (c) Janet Walz, 1992.				  */
/* NetHack may be freely redistributed.  See license for details. */

/*
 *  Utility for reconstructing NetHack save file from a set of individual
 *  level files.  Requires that the `checkpoint' option be enabled at the
 *  time NetHack creates those level files.
 */
#ifdef WIN32
#include <errno.h>
#include "win32api.h"
#endif

#include "config.h"
#if !defined(O_WRONLY) && !defined(LSC) && !defined(AZTEC_C)
#include <fcntl.h>
#endif

#ifdef VMS
extern int vms_creat(const char *, unsigned);
extern int vms_open(const char *, int, unsigned);
#endif /* VMS */

#ifndef nhUse
#define nhUse(arg) (void)(arg)
#endif

int restore_savefile(char *);
void set_levelfile_name(int);
int open_levelfile(int);
int create_savefile(void);
void copy_bytes(int, int);
static void store_formatindicator(int);

#ifndef WIN_CE
#define Fprintf (void) fprintf
#else
#define Fprintf (void) nhce_message
static void nhce_message(FILE *, const char *, ...);
#endif

#define Close (void) close

#if defined(EXEPATH)
char *exepath(char *);
#endif

#if defined(__BORLANDC__) && !defined(_WIN32)
extern unsigned _stklen = STKSIZ;
#endif

/* SAVESIZE is defined in "fnamesiz.h" */
char savename[SAVESIZE]; /* holds relative path of save file from playground */

DISABLE_WARNING_UNREACHABLE_CODE

int
main(int argc, char *argv[])
{
    int argno;
    const char *dir = (char *) 0;
#ifdef AMIGA
    char *startdir = (char *) 0;
#endif

    if (!dir)
        dir = getenv("NETHACKDIR");
    if (!dir)
        dir = getenv("HACKDIR");
#if defined(EXEPATH)
    if (!dir)
        dir = exepath(argv[0]);
#endif
    if (argc == 1 || (argc == 2 && !strcmp(argv[1], "-"))) {
        Fprintf(stderr, "Usage: %s [ -d directory ] base1 [ base2 ... ]\n",
                argv[0]);
#if defined(WIN32) || defined(MSDOS)
        if (dir) {
            Fprintf(
                stderr,
                "\t(Unless you override it with -d, recover will look \n");
            Fprintf(stderr, "\t in the %s directory on your system)\n", dir);
        }
#endif
        exit(EXIT_FAILURE);
    }

    argno = 1;
    if (!strncmp(argv[argno], "-d", 2)) {
        dir = argv[argno] + 2;
        if (*dir == '=' || *dir == ':')
            dir++;
        if (!*dir && argc > argno) {
            argno++;
            dir = argv[argno];
        }
        if (!*dir) {
            Fprintf(stderr,
                    "%s: flag -d must be followed by a directory name.\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
        argno++;
    }
#if defined(SECURE) && !defined(VMS)
    if (dir
#ifdef HACKDIR
        && strcmp(dir, HACKDIR)
#endif
            ) {
        (void) setgid(getgid());
        (void) setuid(getuid());
    }
#endif /* SECURE && !VMS */

#ifdef HACKDIR
    if (!dir)
        dir = HACKDIR;
#endif

#ifdef AMIGA
    startdir = getcwd(0, 255);
#endif
    if (dir && chdir((char *) dir) < 0) {
        Fprintf(stderr, "%s: cannot chdir to %s.\n", argv[0], dir);
        exit(EXIT_FAILURE);
    }

    while (argc > argno) {
        if (restore_savefile(argv[argno]) == 0)
            Fprintf(stderr, "recovered \"%s\" to %s\n", argv[argno],
                    savename);
        argno++;
    }
#ifdef AMIGA
    if (startdir)
        (void) chdir(startdir);
#endif
    exit(EXIT_SUCCESS);
    /*NOTREACHED*/
    return 0;
}

RESTORE_WARNINGS

static char lock[256];

void
set_levelfile_name(int lev)
{
    char *tf;

    tf = strrchr(lock, '.');
    if (!tf)
        tf = lock + strlen(lock);
    (void) sprintf(tf, ".%d", lev);
#ifdef VMS
    (void) strcat(tf, ";1");
#endif
}

int
open_levelfile(int lev)
{
    int fd;

    set_levelfile_name(lev);
#if defined(MICRO) || defined(WIN32) || defined(MSDOS)
    fd = open(lock, O_RDONLY | O_BINARY);
#else
    fd = open(lock, O_RDONLY, 0);
#endif
    return fd;
}

int
create_savefile(void)
{
    int fd;

#if defined(MICRO) || defined(WIN32) || defined(MSDOS)
    fd = open(savename, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, FCMASK);
#else
    fd = creat(savename, FCMASK);
#endif
    return fd;
}

void
copy_bytes(int ifd, int ofd)
{
    char buf[BUFSIZ];
    int nfrom, nto;

    do {
        nfrom = read(ifd, buf, BUFSIZ);
        nto = write(ofd, buf, nfrom);
        if (nto != nfrom) {
            Fprintf(stderr, "file copy failed!\n");
            exit(EXIT_FAILURE);
        }
    } while (nfrom == BUFSIZ);
}

int
restore_savefile(char *basename)
{
    int gfd, lfd, sfd;
    int res = 0, lev, savelev, hpid, pltmpsiz, filecmc;
    xint8 levc;
    struct version_info version_data;
    struct savefile_info sfi;
    char plbuf[PL_NSIZ], indicator;

    /* level 0 file contains:
     *  pid of creating process (ignored here)
     *  level number for current level of save file
     *  name of save file nethack would have created
     *  savefile info
     *  player name
     *  and game state
     */
    (void) strcpy(lock, basename);
    gfd = open_levelfile(0);
    if (gfd < 0) {
#if defined(WIN32) && !defined(WIN_CE)
        if (errno == EACCES) {
            Fprintf(
                stderr,
                "\nThere are files from a game in progress under your name.");
            Fprintf(stderr, "\nThe files are locked or inaccessible.");
            Fprintf(stderr, "\nPerhaps the other game is still running?\n");
        } else
            Fprintf(stderr, "\nTrouble accessing level 0 (errno = %d).\n",
                    errno);
#endif
        Fprintf(stderr, "Cannot open level 0 for %s.\n", basename);
        return -1;
    }
    if (read(gfd, (genericptr_t) &hpid, sizeof hpid) != sizeof hpid) {
        Fprintf(
            stderr, "%s\n%s%s%s\n",
            "Checkpoint data incompletely written or subsequently clobbered;",
            "recovery for \"", basename, "\" impossible.");
        Close(gfd);
        return -1;
    }
    if (read(gfd, (genericptr_t) &savelev, sizeof(savelev))
        != sizeof(savelev)) {
        Fprintf(stderr, "Checkpointing was not in effect for %s -- recovery "
                        "impossible.\n",
                basename);
        Close(gfd);
        return -1;
    }
    if ((read(gfd, (genericptr_t) savename, sizeof savename)
         != sizeof savename)
        || (read(gfd, (genericptr_t) &indicator, sizeof indicator)
            != sizeof indicator)
        || (read(gfd, (genericptr_t) &filecmc, sizeof filecmc)
            != sizeof filecmc)
        || (read(gfd, (genericptr_t) &version_data, sizeof version_data)
            != sizeof version_data)
        || (read(gfd, (genericptr_t) &sfi, sizeof sfi) != sizeof sfi)
        || (read(gfd, (genericptr_t) &pltmpsiz, sizeof pltmpsiz)
            != sizeof pltmpsiz) || (pltmpsiz > PL_NSIZ)
        || (read(gfd, (genericptr_t) plbuf, pltmpsiz) != pltmpsiz)) {
        Fprintf(stderr, "Error reading %s -- can't recover.\n", lock);
        Close(gfd);
        return -1;
    }

    /* save file should contain:
     *  format indicator and cmc
     *  version info
     *  savefile info
     *  player name
     *  current level (including pets)
     *  (non-level-based) game state
     *  other levels
     */
    sfd = create_savefile();
    if (sfd < 0) {
        Fprintf(stderr, "Cannot create savefile %s.\n", savename);
        Close(gfd);
        return -1;
    }

    lfd = open_levelfile(savelev);
    if (lfd < 0) {
        Fprintf(stderr, "Cannot open level of save for %s.\n", basename);
        Close(gfd);
        Close(sfd);
        return -1;
    }

    store_formatindicator(sfd);
    if (write(sfd, (genericptr_t) &version_data, sizeof version_data)
        != sizeof version_data) {
        Fprintf(stderr, "Error writing %s; recovery failed.\n", savename);
        Close(gfd);
        Close(sfd);
        Close(lfd);
        return -1;
    }

    if (write(sfd, (genericptr_t) &sfi, sizeof sfi) != sizeof sfi) {
        Fprintf(stderr,
                "Error writing %s; recovery failed (savefile_info).\n",
                savename);
        Close(gfd);
        Close(sfd);
        Close(lfd);
        return -1;
    }

    if (write(sfd, (genericptr_t) &pltmpsiz, sizeof pltmpsiz)
        != sizeof pltmpsiz) {
        Fprintf(stderr,
                "Error writing %s; recovery failed (player name size).\n",
                savename);
        Close(gfd);
        Close(sfd);
        Close(lfd);
        return -1;
    }

    if (write(sfd, (genericptr_t) plbuf, pltmpsiz) != pltmpsiz) {
        Fprintf(stderr, "Error writing %s; recovery failed (player name).\n",
                savename);
        Close(gfd);
        Close(sfd);
        Close(lfd);
        return -1;
    }

    copy_bytes(lfd, sfd);
    Close(lfd);
    (void) unlink(lock);

    copy_bytes(gfd, sfd);
    Close(gfd);
    set_levelfile_name(0);
    (void) unlink(lock);

    for (lev = 1; lev < 256 && res == 0; lev++) {
        /* level numbers are kept in 'xint8's in save.c, so the
         * maximum level number (for the endlevel) must be < 256
         */
        if (lev != savelev) {
            lfd = open_levelfile(lev);
            if (lfd >= 0) {
                /* any or all of these may not exist */
                levc = (xint8) lev;
                if (write(sfd, (genericptr_t) &levc, sizeof levc)
                    != sizeof levc)
                    res = -1;
                else
                    copy_bytes(lfd, sfd);
                Close(lfd);
                (void) unlink(lock);
            }
        }
    }

    Close(sfd);

#if 0 /* OBSOLETE, HackWB is no longer in use */
#ifdef AMIGA
    if (res == 0) {
        /* we need to create an icon for the saved game
         * or HackWB won't notice the file.
         */
        char iconfile[FILENAME];
        int in, out;

        (void) sprintf(iconfile, "%s.info", savename);
        in = open("NetHack:default.icon", O_RDONLY);
        out = open(iconfile, O_WRONLY | O_TRUNC | O_CREAT);
        if (in > -1 && out > -1) {
            copy_bytes(in, out);
        }
        if (in > -1)
            close(in);
        if (out > -1)
            close(out);
    }
#endif /*AMIGA*/
#endif
    return res;
}

static void
store_formatindicator(int fd)
{
    char indicate = 'h';      /* historical */
    int cmc = 0;

    write(fd, (genericptr_t) &indicate, sizeof indicate);
    write(fd, (genericptr_t) &cmc, sizeof cmc);
}



#ifdef EXEPATH
#ifdef __DJGPP__
#define PATH_SEPARATOR '/'
#else
#define PATH_SEPARATOR '\\'
#endif

#define EXEPATHBUFSZ 256
char exepathbuf[EXEPATHBUFSZ];

char *
exepath(char *str)
{
    char *tmp, *tmp2;
    int bsize;

    if (!str)
        return (char *) 0;
    nhUse(bsize);
    bsize = EXEPATHBUFSZ;
    tmp = exepathbuf;
#if !defined(WIN32)
    strcpy(tmp, str);
#else
#if defined(WIN_CE)
    {
        TCHAR wbuf[EXEPATHBUFSZ];
        GetModuleFileName((HANDLE) 0, wbuf, EXEPATHBUFSZ);
        NH_W2A(wbuf, tmp, bsize);
    }
#else
    *(tmp + GetModuleFileName((HANDLE) 0, tmp, bsize)) = '\0';
#endif
#endif
    tmp2 = strrchr(tmp, PATH_SEPARATOR);
    if (tmp2)
        *tmp2 = '\0';
    return tmp;
}
#endif /* EXEPATH */

#ifdef WIN_CE
void
nhce_message(FILE *f, const char *str, ...)
{
    va_list ap;
    TCHAR wbuf[NHSTR_BUFSIZE];
    char buf[NHSTR_BUFSIZE];

    va_start(ap, str);
    vsprintf(buf, str, ap);
    va_end(ap);

    MessageBox(NULL, NH_A2W(buf, wbuf, NHSTR_BUFSIZE), TEXT("Recover"),
               MB_OK);
}
#endif

/*recover.c*/
