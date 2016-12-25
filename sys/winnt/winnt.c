/* NetHack 3.6	winnt.c	$NHDT-Date: 1431737068 2015/05/16 00:44:28 $  $NHDT-Branch: master $:$NHDT-Revision: 1.26 $ */
/* Copyright (c) NetHack PC Development Team 1993, 1994 */
/* NetHack may be freely redistributed.  See license for details. */

/*
 *  WIN32 system functions.
 *
 *  Initial Creation: Michael Allison - January 31/93
 *
 */

#define NEED_VARARGS
#include "hack.h"
#include <dos.h>
#ifndef __BORLANDC__
#include <direct.h>
#endif
#include <ctype.h>
#include "win32api.h"
#include "wintty.h"
#ifdef WIN32

/*
 * The following WIN32 API routines are used in this file.
 *
 * GetDiskFreeSpace
 * GetVolumeInformation
 * GetUserName
 * FindFirstFile
 * FindNextFile
 * FindClose
 *
 */

/* globals required within here */
HANDLE ffhandle = (HANDLE) 0;
WIN32_FIND_DATA ffd;

/* The function pointer nt_kbhit contains a kbhit() equivalent
 * which varies depending on which window port is active.
 * For the tty port it is tty_kbhit() [from nttty.c]
 * For the win32 port it is win32_kbhit() [from winmain.c]
 * It is initialized to point to def_kbhit [in here] for safety.
 */

int def_kbhit(void);
int (*nt_kbhit)() = def_kbhit;

char
switchar()
{
    /* Could not locate a WIN32 API call for this- MJA */
    return '-';
}

long
freediskspace(path)
char *path;
{
    char tmppath[4];
    DWORD SectorsPerCluster = 0;
    DWORD BytesPerSector = 0;
    DWORD FreeClusters = 0;
    DWORD TotalClusters = 0;

    tmppath[0] = *path;
    tmppath[1] = ':';
    tmppath[2] = '\\';
    tmppath[3] = '\0';
    GetDiskFreeSpace(tmppath, &SectorsPerCluster, &BytesPerSector,
                     &FreeClusters, &TotalClusters);
    return (long) (SectorsPerCluster * BytesPerSector * FreeClusters);
}

/*
 * Functions to get filenames using wildcards
 */
int
findfirst(path)
char *path;
{
    if (ffhandle) {
        FindClose(ffhandle);
        ffhandle = (HANDLE) 0;
    }
    ffhandle = FindFirstFile(path, &ffd);
    return (ffhandle == INVALID_HANDLE_VALUE) ? 0 : 1;
}

int
findnext()
{
    return FindNextFile(ffhandle, &ffd) ? 1 : 0;
}

char *
foundfile_buffer()
{
    return &ffd.cFileName[0];
}

long
filesize(file)
char *file;
{
    if (findfirst(file)) {
        return ((long) ffd.nFileSizeLow);
    } else
        return -1L;
}

/*
 * Chdrive() changes the default drive.
 */
void
chdrive(str)
char *str;
{
    char *ptr;
    char drive;
    if ((ptr = index(str, ':')) != (char *) 0) {
        drive = toupper((uchar) *(ptr - 1));
        _chdrive((drive - 'A') + 1);
    }
}

static int
max_filename()
{
    DWORD maxflen;
    int status = 0;

    status = GetVolumeInformation((LPTSTR) 0, (LPTSTR) 0, 0, (LPDWORD) 0,
                                  &maxflen, (LPDWORD) 0, (LPTSTR) 0, 0);
    if (status)
        return maxflen;
    else
        return 0;
}

int
def_kbhit()
{
    return 0;
}

/*
 * Strip out troublesome file system characters.
 */

void nt_regularize(s) /* normalize file name */
register char *s;
{
    register unsigned char *lp;

    for (lp = s; *lp; lp++)
        if (*lp == '?' || *lp == '"' || *lp == '\\' || *lp == '/'
            || *lp == '>' || *lp == '<' || *lp == '*' || *lp == '|'
            || *lp == ':' || (*lp > 127))
            *lp = '_';
}

/*
 * This is used in nhlan.c to implement some of the LAN_FEATURES.
 */
char *
get_username(lan_username_size)
int *lan_username_size;
{
    static TCHAR username_buffer[BUFSZ];
    unsigned int status;
    DWORD i = BUFSZ - 1;

    /* i gets updated with actual size */
    status = GetUserName(username_buffer, &i);
    if (status)
        username_buffer[i] = '\0';
    else
        Strcpy(username_buffer, "NetHack");
    if (lan_username_size)
        *lan_username_size = strlen(username_buffer);
    return username_buffer;
}

#if 0
char *getxxx()
{
char     szFullPath[MAX_PATH] = "";
HMODULE  hInst = NULL;  	/* NULL gets the filename of this module */

GetModuleFileName(hInst, szFullPath, sizeof(szFullPath));
return &szFullPath[0];
}
#endif

/* fatal error */
/*VARARGS1*/
void error
VA_DECL(const char *, s)
{
    char buf[BUFSZ];
    VA_START(s);
    VA_INIT(s, const char *);
    /* error() may get called before tty is initialized */
    if (iflags.window_inited)
        end_screen();
    if (!strncmpi(windowprocs.name, "tty", 3)) {
        buf[0] = '\n';
        (void) vsprintf(&buf[1], s, VA_ARGS);
        Strcat(buf, "\n");
        msmsg(buf);
    } else {
        (void) vsprintf(buf, s, VA_ARGS);
        Strcat(buf, "\n");
        raw_printf(buf);
    }
    VA_END();
    exit(EXIT_FAILURE);
}

void
Delay(int ms)
{
    (void) Sleep(ms);
}

extern void NDECL(backsp);

void
win32_abort()
{
    if (wizard) {
        int c, ci, ct;

        if (!iflags.window_inited)
            c = 'n';
        ct = 0;
        msmsg("Execute debug breakpoint wizard?");
        while ((ci = nhgetch()) != '\n') {
            if (ct > 0) {
                backsp(); /* \b is visible on NT */
                (void) putchar(' ');
                backsp();
                ct = 0;
                c = 'n';
            }
            if (ci == 'y' || ci == 'n' || ci == 'Y' || ci == 'N') {
                ct = 1;
                c = ci;
                msmsg("%c", c);
            }
        }
        if (c == 'y')
            DebugBreak();
    }
    abort();
}

static char interjection_buf[INTERJECTION_TYPES][1024];
static int interjection[INTERJECTION_TYPES];

void
interject_assistance(num, interjection_type, ptr1, ptr2)
int num;
int interjection_type;
genericptr_t ptr1;
genericptr_t ptr2;
{
    switch (num) {
    case 1: {
        char *panicmsg = (char *) ptr1;
        char *datadir = (char *) ptr2;
        char *tempdir = nh_getenv("TEMP");
        interjection_type = INTERJECT_PANIC;
        interjection[INTERJECT_PANIC] = 1;
        /*
         * ptr1 = the panic message about to be delivered.
         * ptr2 = the directory prefix of the dungeon file
         *        that failed to open.
         * Check to see if datadir matches tempdir or a
         * common windows temp location. If it does, inform
         * the user that they are probably trying to run the
         * game from within their unzip utility, so the required
         * files really don't exist at the location. Instruct
         * them to unpack them first.
         */
        if (panicmsg && datadir) {
            if (!strncmpi(datadir, "C:\\WINDOWS\\TEMP", 15)
                || strstri(datadir, "TEMP")
                || (tempdir && strstri(datadir, tempdir))) {
                (void) strncpy(
                    interjection_buf[INTERJECT_PANIC],
                    "\nOne common cause of this error is attempting to "
                    "execute\n"
                    "the game by double-clicking on it while it is "
                    "displayed\n"
                    "inside an unzip utility.\n\n"
                    "You have to unzip the contents of the zip file into a\n"
                    "folder on your system, and then run \"NetHack.exe\" or "
                    "\n"
                    "\"NetHackW.exe\" from there.\n\n"
                    "If that is not the situation, you are encouraged to\n"
                    "report the error as shown above.\n\n",
                    1023);
            }
        }
    } break;
    }
}

void
interject(interjection_type)
int interjection_type;
{
    if (interjection_type >= 0 && interjection_type < INTERJECTION_TYPES)
        msmsg(interjection_buf[interjection_type]);
}

#ifdef RUNTIME_PORT_ID
/*
 * _M_IX86 is Defined for x86 processors. This is not defined for x64
 * processors.
 * _M_X64  is Defined for x64 processors.
 * _M_IA64 is Defined for Itanium Processor Family 64-bit processors.
 * _WIN64  is Defined for applications for Win64.
 */
#ifndef _M_IX86
#ifdef _M_X64
#define TARGET_PORT "(x64) "
#endif
#ifdef _M_IA64
#define TARGET_PORT "(IA64) "
#endif
#endif

#ifndef TARGET_PORT
#define TARGET_PORT "(x86) "
#endif

void
append_port_id(buf)
char *buf;
{
    char *portstr = TARGET_PORT;
    Sprintf(eos(buf), " %s", portstr);
}
#endif /* RUNTIME_PORT_ID */

#endif /* WIN32 */

/*winnt.c*/
