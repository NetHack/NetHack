/*	SCCS Id: @(#)winnt.c	 3.4	 1997/04/12		  */
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
HANDLE ffhandle = (HANDLE)0;
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
	GetDiskFreeSpace(tmppath, &SectorsPerCluster,
			&BytesPerSector,
			&FreeClusters,
			&TotalClusters);
	return (long)(SectorsPerCluster * BytesPerSector *
			FreeClusters);
}

/*
 * Functions to get filenames using wildcards
 */
int
findfirst(path)
char *path;
{
	if (ffhandle){
		 FindClose(ffhandle);
		 ffhandle = (HANDLE)0;
	}
	ffhandle = FindFirstFile(path,&ffd);
	return 
	  (ffhandle == INVALID_HANDLE_VALUE) ? 0 : 1;
}

int
findnext() 
{
	return FindNextFile(ffhandle,&ffd) ? 1 : 0;
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
		return ((long)ffd.nFileSizeLow);
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
	if ((ptr = index(str, ':')) != (char *)0) 
	{
		drive = toupper(*(ptr - 1));
		_chdrive((drive - 'A') + 1);
	}
}

static int
max_filename()
{
	DWORD maxflen;
	int status=0;
	
	status = GetVolumeInformation((LPTSTR)0,(LPTSTR)0, 0
			,(LPDWORD)0,&maxflen,(LPDWORD)0,(LPTSTR)0,0);
	if (status) return maxflen;
	else return 0;
}

int
def_kbhit()
{
	return 0;
}

/* 
 * Windows NT version >= 3.5x and above supports long file names,
 * even on FAT volumes (VFAT), so no need for nt_regularize.
 * Windows NT 3.1 could not do long file names except on NTFS,
 * so nt_regularize was required.
 */

void
nt_regularize(s)	/* normalize file name */
register char *s;
{
	register unsigned char *lp;

	for (lp = s; *lp; lp++)
	    if ( *lp == '?' || *lp == '"' || *lp == '\\' ||
		 *lp == '/' || *lp == '>' || *lp == '<'  ||
		 *lp == '*' || *lp == '|' || *lp == ':'  || (*lp > 127))
			*lp = '_';
}

/*
 * This is used in nhlan.c to implement some of the LAN_FEATURES.
 */
char *get_username(lan_username_size)
int *lan_username_size;
{
	static TCHAR username_buffer[BUFSZ];
	unsigned int status;
	DWORD i = BUFSZ - 1;

	/* i gets updated with actual size */
	status = GetUserName(username_buffer, &i);		
	if (status) username_buffer[i] = '\0';
	else Strcpy(username_buffer, "NetHack");
	if (lan_username_size) *lan_username_size = strlen(username_buffer);
	return username_buffer;
}

# if 0
char *getxxx()
{
char     szFullPath[MAX_PATH] = "";
HMODULE  hInst = NULL;  	/* NULL gets the filename of this module */

GetModuleFileName(hInst, szFullPath, sizeof(szFullPath));
return &szFullPath[0];
}
# endif


/* fatal error */
/*VARARGS1*/

void
error VA_DECL(const char *,s)
	VA_START(s);
	VA_INIT(s, const char *);
	/* error() may get called before tty is initialized */
	if (iflags.window_inited) end_screen();
	putchar('\n');
	Vprintf(s,VA_ARGS);
	putchar('\n');
	VA_END();
	exit(EXIT_FAILURE);
}

void Delay(int ms)
{
	(void)Sleep(ms);
}
#endif /* WIN32 */

/*winnt.c*/
