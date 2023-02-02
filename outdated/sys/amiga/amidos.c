/* NetHack 3.6	amidos.c	$NHDT-Date: 1432512796 2015/05/25 00:13:16 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $ */
/* Copyright (c) Olaf Seibert, Nijmegen, The Netherlands, 1988,1990.    */
/* Copyright (c) Kenneth Lorber, Bethesda, Maryland, 1991,1992,1993,1996.  */
/* NetHack may be freely redistributed.  See license for details.	*/

/*
 * An assortment of imitations of cheap plastic MSDOS and Unix functions.
 */

#include "hack.h"
#include "winami.h"

/* Defined in config.h, let's undefine it here (static function below) */
#undef strcmpi

#include <libraries/dos.h>
#include <exec/execbase.h>
#include <intuition/intuition.h>

#undef COUNT
#if defined(__SASC_60) || defined(__GNUC__)
#include <proto/exec.h>
#include <proto/dos.h>
#endif

#ifdef AZTEC_50
#include <functions.h>
#undef strcmpi
#endif

/* Prototypes */
#ifndef CROSS_TO_AMIGA
#include "NH:sys/amiga/amiwind.p"
#include "NH:sys/amiga/winami.p"
#include "NH:sys/amiga/amidos.p"
#else
#include "winami.p"
#include "winami.p"
#include "amidos.p"
#endif

extern char Initialized;
extern struct window_procs amii_procs;
struct ami_sysflags sysflags = {0};
FILE *fopenp(const char *, const char *);

#ifndef __SASC_60
int Enable_Abort = 0; /* for stdio package */
#endif

/* Initial path, so we can find NetHack.cnf */
char PATH[PATHLEN] = "NetHack:";

static boolean record_exists(void);

void
flushout()
{
    (void) fflush(stdout);
}

#ifndef getuid
getuid()
{
    return 1;
}
#endif

#ifndef getlogin
char *
getlogin()
{
    return ((char *) NULL);
}
#endif

#ifndef AZTEC_50
int
abs(x)
int x;
{
    return x < 0 ? -x : x;
}
#endif

#ifdef SHELL
int
dosh()
{
    int i;
    char buf[BUFSZ];
    extern struct ExecBase *SysBase;

    /* Only under 2.0 and later ROMs do we have System() */
    if (SysBase->LibNode.lib_Version >= 37 && !amibbs) {
        getlin("Enter CLI Command...", buf);
        if (buf[0] != '\033')
            i = System(buf, NULL);
    } else {
        i = 0;
        pline("No mysterious force prevented you from using multitasking.");
    }
    return i;
}
#endif /* SHELL */

#ifdef MFLOPPY
#include <ctype.h>

#define Sprintf (void) sprintf

#define EXTENSION 72

/*
 *  This routine uses an approximation of the free bytes on a disk.
 *  How large a file you can actually write depends on the number of
 *  extension blocks you need for it.
 *  In each extenstion block there are maximum 72 pointers to blocks,
 *  so every 73 disk blocks have only 72 available for data.
 *  The (necessary) file header is also good for 72 data block pointers.
 */
/* TODO: update this for FFS */
long
freediskspace(path)
char *path;
{
#ifdef UNTESTED
    /* these changes from Patric Mueller <bhaak@gmx.net> for AROS to
     * handle larger disks.  Also needs limits.h and aros/oldprograms.h
     * for AROS.  (keni)
     */
    unsigned long long freeBytes = 0;
#else
    register long freeBytes = 0;
#endif
    register struct InfoData *infoData; /* Remember... longword aligned */
    char fileName[32];

    /*
     *  Find a valid path on the device of which we want the free space.
     *  If there is a colon in the name, it is an absolute path
     *  and all up to the colon is everything we need.
     *  Remember slashes in a volume name are allowed!
     *  If there is no colon, it is relative to the current directory,
     *  so must be on the current device, so "" is enough...
     */
    {
        register char *colon;

        strncpy(fileName, path, sizeof(fileName) - 1);
        fileName[31] = 0;
        if (colon = strchr(fileName, ':'))
            colon[1] = '\0';
        else
            fileName[0] = '\0';
    }
    {
        BPTR fileLock;
        infoData = (struct InfoData *) alloc(sizeof(struct InfoData));
        if (fileLock = Lock(fileName, SHARED_LOCK)) {
            if (Info(fileLock, infoData)) {
                /* We got a kind of DOS volume, since we can Lock it. */
                /* Calculate number of blocks available for new file */
                /* Kludge for the ever-full VOID: (oops RAM:) device */
                if (infoData->id_UnitNumber == -1
                    && infoData->id_NumBlocks == infoData->id_NumBlocksUsed) {
                    freeBytes = AvailMem(0L) - 64 * 1024L;
                    /* Just a stupid guess at the */
                    /* Ram-Handler overhead per block: */
                    freeBytes -= freeBytes / 16;
                } else {
                    /* Normal kind of DOS file system device/volume */
                    freeBytes =
                        infoData->id_NumBlocks - infoData->id_NumBlocksUsed;
                    freeBytes -= (freeBytes + EXTENSION) / (EXTENSION + 1);
                    freeBytes *= infoData->id_BytesPerBlock;
#ifdef UNTESTED
                    if (freeBytes > LONG_MAX) {
                        freeBytes = LONG_MAX;
                    }
#endif
                }
                if (freeBytes < 0)
                    freeBytes = 0;
            }
            UnLock(fileLock);
        }
        free(infoData);
        return freeBytes;
    }
}

long
filesize(file)
char *file;
{
    register BPTR fileLock;
    register struct FileInfoBlock *fileInfoBlock;
    register long size = 0;

    fileInfoBlock =
        (struct FileInfoBlock *) alloc(sizeof(struct FileInfoBlock));
    if (fileLock = Lock(file, SHARED_LOCK)) {
        if (Examine(fileLock, fileInfoBlock)) {
            size = fileInfoBlock->fib_Size;
        }
        UnLock(fileLock);
    }
    free(fileInfoBlock);
    return size;
}

#if 0
void
eraseall(path, files)
const char *path, *files;
{
    BPTR dirLock, dirLock2;
    struct FileInfoBlock *fibp;
    int chklen;
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED)
    if(files != g.alllevels)panic("eraseall");
#endif
    chklen=(int)index(files,'*')-(int)files;

    if (dirLock = Lock( (char *)path ,SHARED_LOCK)) {
	dirLock2=DupLock(dirLock);
	dirLock2= CurrentDir(dirLock2);
	fibp=AllocMem(sizeof(struct FileInfoBlock),0);
	if(fibp){
	    if(Examine(dirLock,fibp)){
		while(ExNext(dirLock,fibp)){
		    if(!strncmp(fibp->fib_FileName,files,chklen)){
			DeleteFile(fibp->fib_FileName);
		    }
		}
	    }
	    FreeMem(fibp,sizeof(struct FileInfoBlock));
	}
	UnLock(dirLock);
	UnLock(CurrentDir(dirLock2));
    }
}
#endif

/* This size makes that most files can be copied with two Read()/Write()s */

#if 0 /* Unused */
#define COPYSIZE 4096

char *CopyFile(from, to)
const char *from, *to;
{
    register BPTR fromFile, toFile;
    register char *buffer;
    register long size;
    char *error = NULL;

    buffer = (char *) alloc(COPYSIZE);
    if (fromFile = Open( (char *)from, MODE_OLDFILE)) {
	if (toFile = Open( (char *)to, MODE_NEWFILE)) {
	    while (size = Read(fromFile, buffer, (long)COPYSIZE)) {
		if (size == -1){
		    error = "Read error";
		    break;
		}
		if (size != Write(toFile, buffer, size)) {
		    error = "Write error";
		    break;
		}
	    }
	    Close(toFile);
	} else
	    error = "Cannot open destination";
	Close(fromFile);
    } else
	error = "Cannot open source (this should not occur)";
    free(buffer);
    return error;
}
#endif

#ifdef MFLOPPY
/* this should be replaced */
saveDiskPrompt(start)
{
    char buf[BUFSIZ], *bp;
    BPTR fileLock;
    if (sysflags.asksavedisk) {
        /* Don't prompt if you can find the save file */
        if (fileLock = Lock(gs.SAVEF, SHARED_LOCK)) {
            UnLock(fileLock);
#if defined(TTY_GRAPHICS)
            if (windowprocs.win_init_nhwindows
                != amii_procs.win_init_nhwindows)
                clear_nhwindow(WIN_MAP);
#endif
#if defined(AMII_GRAPHICS)
            if (windowprocs.win_init_nhwindows
                == amii_procs.win_init_nhwindows)
                clear_nhwindow(WIN_BASE);
#endif
            return 1;
        }
        pline("If save file is on a SAVE disk, put that disk in now.");
        if (strlen(gs.SAVEF) > QBUFSZ - 25 - 22)
            panic("not enough buffer space for prompt");
/* THIS IS A HACK */
#if defined(TTY_GRAPHICS)
        if (windowprocs.win_init_nhwindows != amii_procs.win_init_nhwindows) {
            getlin("File name ?", buf);
            clear_nhwindow(WIN_MAP);
        }
#endif
#if defined(AMII_GRAPHICS)
        if (windowprocs.win_init_nhwindows == amii_procs.win_init_nhwindows) {
            getlind("File name ?", buf, gs.SAVEF);
            clear_nhwindow(WIN_BASE);
        }
#endif
        clear_nhwindow(WIN_MESSAGE);
        if (!start && *buf == '\033')
            return 0;

        /* Strip any whitespace. Also, if nothing was entered except
         * whitespace, do not change the value of gs.SAVEF.
         */
        for (bp = buf; *bp; bp++) {
            if (!isspace(*bp)) {
                strncpy(gs.SAVEF, bp, PATHLEN);
                break;
            }
        }
    }
    return 1;
}
#endif /* MFLOPPY */

/* Return 1 if the record file was found */
static boolean
record_exists()
{
    FILE *file;

    if (file = fopenp(RECORD, "r")) {
        fclose(file);
        return TRUE;
    }
    return FALSE;
}

#ifdef MFLOPPY
/*
 * Under MSDOS: Prompt for game disk, then check for record file.
 * For Amiga: do nothing, but called from restore.c
 */
void
gameDiskPrompt()
{
}
#endif

/*
 * Add a slash to any name not ending in / or :.  There must
 * be room for the /.
 */
void
append_slash(name)
char *name;
{
    char *ptr;

    if (!*name)
        return;

    ptr = eos(name) - 1;
    if (*ptr != '/' && *ptr != ':') {
        *++ptr = '/';
        *++ptr = '\0';
    }
}

void
getreturn(str)
const char *str;
{
    int ch;

    raw_printf("Hit <RETURN> %s.", str);
    while ((ch = nhgetch()) != '\n' && ch != '\r')
        continue;
}

/* Follow the PATH, trying to fopen the file.
 */
#define PATHSEP ';'

FILE *
fopenp(name, mode)
register const char *name, *mode;
{
    register char *bp, *pp, lastch;
    register FILE *fp;
    register BPTR theLock;
    char buf[BUFSIZ];

    /* Try the default directory first.  Then look along PATH.
     */
    if (strlen(name) >= BUFSIZ)
        return (NULL);
    strcpy(buf, name);
    if (theLock = Lock(buf, SHARED_LOCK)) {
        UnLock(theLock);
        if (fp = fopen(buf, mode))
            return fp;
    }
    pp = PATH;
    while (pp && *pp) {
        bp = buf;
        while (*pp && *pp != PATHSEP) {
            if (bp > buf + BUFSIZ - 1)
                return (NULL);
            lastch = *bp++ = *pp++;
        }
        if (lastch != ':' && lastch != '/' && bp != buf)
            *bp++ = '/';
        if (bp + strlen(name) > buf + BUFSIZ - 1)
            return (NULL);
        strcpy(bp, name);
        if (theLock = Lock(buf, SHARED_LOCK)) {
            UnLock(theLock);
            if (fp = fopen(buf, mode))
                return fp;
        }
        if (*pp)
            pp++;
    }
    return NULL;
}
#endif /* MFLOPPY */

#ifdef CHDIR

/*
 *  A not general-purpose directory changing routine.
 *  Assumes you want to return to the original directory eventually,
 *  by chdir()ing to orgdir (which is defined in pcmain.c).
 *  Assumes -1 is not a valid lock, since 0 is valid.
 */

#define NO_LOCK ((BPTR) -1)

static BPTR OrgDirLock = NO_LOCK;

chdir(dir) char *dir;
{
    extern char orgdir[];

    if (dir == orgdir) {
        /* We want to go back to where we came from. */
        if (OrgDirLock != NO_LOCK) {
            UnLock(CurrentDir(OrgDirLock));
            OrgDirLock = NO_LOCK;
        }
    } else {
        /*
         * Go to some new place. If still at the original
         * directory, save the FileLock.
         */
        BPTR newDir;

        if (newDir = Lock((char *) dir, SHARED_LOCK)) {
            if (OrgDirLock == NO_LOCK) {
                OrgDirLock = CurrentDir(newDir);
            } else {
                UnLock(CurrentDir(newDir));
            }
        } else {
            return -1; /* Failed */
        }
    }
    /* CurrentDir always succeeds if you have a lock */
    return 0;
}

#endif /* CHDIR */

/* Chdir back to original directory
 */
#undef exit
void
nethack_exit(code)
{
#ifdef CHDIR
    extern char orgdir[];
#endif

#ifdef CHDIR
    chdir(orgdir); /* chdir, not chdirx */
#endif

#ifdef AMII_GRAPHICS
    if (windowprocs.win_init_nhwindows == amii_procs.win_init_nhwindows)
        CleanUp();
#endif
    exit(code);
}

void regularize(s) /* normalize file name - we don't like :'s or /'s */
register char *s;
{
    register char *lp;

    while ((lp = strchr(s, ':')) || (lp = strchr(s, '/')))
        *lp = '_';
}
