/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
/* NetHack may be freely redistributed.  See license for details. */

#define NEED_VARARGS
#include "hack.h"
#include <fcntl.h>
// #include "wceconf.h"

static union {
	time_t t_val;
	struct time_pack {
		unsigned int ss:6;
		unsigned int mm:6;
		unsigned int dd:5;
		unsigned int hh:6;
		unsigned int mo:4;
		unsigned int yr:10;
		unsigned int wd:3;
	} tm_val;
} _t_cnv;

#define IS_LEAP(yr) (((yr)%4==0 || (yr)%100==0) && !(yr)%400==0)
static char _day_mo_leap[12] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static char _day_mo[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

struct tm * __cdecl localtime ( const time_t *ptime )
{
	static struct tm ptm;
	int i;
	if( !ptime ) return NULL;

	_t_cnv.t_val = *ptime;

    ptm.tm_sec		= _t_cnv.tm_val.ss ;     /* seconds after the minute - [0,59] */
    ptm.tm_min		= _t_cnv.tm_val.mm;     /* minutes after the hour - [0,59] */
    ptm.tm_hour	= _t_cnv.tm_val.hh;    /* hours since midnight - [0,23] */
    ptm.tm_mday	= _t_cnv.tm_val.dd;    /* day of the month - [1,31] */
    ptm.tm_mon		= _t_cnv.tm_val.mo-1;     /* months since January - [0,11] */
    ptm.tm_year	= _t_cnv.tm_val.yr;    /* years since 1900 */
    ptm.tm_wday	= _t_cnv.tm_val.wd;    /* days since Sunday - [0,6] */

	ptm.tm_yday = _t_cnv.tm_val.dd;    /* days since January 1 - [0,365] */
	for( i=0; i<ptm.tm_mon; i++ )
		ptm.tm_yday += IS_LEAP(_t_cnv.tm_val.yr+1900)?_day_mo_leap[i] : _day_mo[i] ; 

	ptm.tm_isdst = 0;   /* daylight savings time flag  - NOT IMPLEMENTED */
	return &ptm;
}

time_t __cdecl time(time_t * timeptr)
{
	SYSTEMTIME stm;
	GetLocalTime(&stm);

	_t_cnv.tm_val.yr = stm.wYear-1900; 
	_t_cnv.tm_val.mo = stm.wMonth; 
	_t_cnv.tm_val.dd = stm.wDay; 
	_t_cnv.tm_val.hh = stm.wHour; 
	_t_cnv.tm_val.mm = stm.wMinute; 
	_t_cnv.tm_val.ss = stm.wSecond; 
	_t_cnv.tm_val.wd = stm.wDayOfWeek; 

	if( timeptr) 
		*timeptr = _t_cnv.t_val;
	return _t_cnv.t_val;
}

/*------------------------------------------------------------------------------*/
/* __io.h__ */
/* Hack io.h function with stdio.h functions */
/* ASSUMPTION : int can hold FILE* */
static char _nh_cwd[255];

int __cdecl close(int f) {
	FILE* p = (FILE*)f;
	return fclose(p);
}

int __cdecl creat(const char *fname , int mode)
{
	FILE* f;
	f = fopen(fname, "w+");
	if( !f ) return -1;
	else     return (int)f;
}

int __cdecl eof(int f)
{
	FILE* p = (FILE*)f;
	return feof(p);
}

long __cdecl lseek( int handle, long offset, int origin )
{
	FILE* p = (FILE*)handle;
	if( !fseek(p, offset, origin) ) return ftell(p);
	else return -1;
}

int __cdecl open( const char *filename, int oflag, ... )
{
    va_list ap;
    int pmode;
	char mode[16];
	char fname[255];
	FILE* f;

    va_start(ap, oflag);
    pmode = va_arg(ap, int);
    va_end(ap);

	ZeroMemory(mode, sizeof(mode));
	if( (oflag & (O_WRONLY|O_APPEND)) ==  (O_WRONLY|O_APPEND) )	strcat(mode, "a");
	else if( (oflag & (O_RDWR | O_APPEND)) ==  (O_RDWR | O_APPEND) ) strcat(mode, "a+");
	else if( (oflag & (O_RDWR | O_TRUNC)) == (O_RDWR | O_TRUNC) ) strcat(mode, "w+");
	else if( (oflag & (O_RDWR | O_CREAT)) == (O_RDWR | O_CREAT) ) strcat(mode, "a+");
	else if( (oflag & O_RDWR) == O_RDWR ) strcat(mode, "r+");
	else if( (oflag & O_WRONLY) == O_WRONLY ) strcat(mode, "w");
	else if( (oflag & O_RDONLY) == O_RDONLY ) strcat(mode, "r");

	if( oflag & O_BINARY ) strcat(mode, "b");
	if( oflag & O_TEXT ) strcat(mode, "t");

	fname[0]='\x0';
	if( *filename!='\\' && *filename!='/' ) {
		strncat(fname, _nh_cwd, sizeof(fname));
		strncat(fname, "\\", sizeof(fname) - strlen(fname));
	}
	strncat(fname, filename, sizeof(fname) - strlen(fname));

	f = fopen(fname, mode);
	if( !f ) return -1;
	else {
		if( !(oflag & O_APPEND) ) fseek(f, SEEK_SET, 0);
		return (int)f;
	}
}

int __cdecl read( int handle, void *buffer, unsigned int count )
{
	FILE* p = (FILE*)handle;
	return fread(buffer, 1, count, p);
}

int __cdecl unlink(const char * filename)
{
	TCHAR wbuf[NHSTR_BUFSIZE];
	char fname[255];

	fname[0]='\x0';
	if( *filename!='\\' && *filename!='/' ) {
		strncat(fname, _nh_cwd, sizeof(fname));
		strncat(fname, "\\", sizeof(fname) - strlen(fname));
	}
	strncat(fname, filename, sizeof(fname) - strlen(fname));

	return !DeleteFileW(NH_A2W(fname, wbuf, NHSTR_BUFSIZE));
}

int __cdecl write( int handle, const void *buffer, unsigned int count )
{
	FILE* p = (FILE*)handle;
	return fwrite(buffer, 1, count, p);
}

int __cdecl rename( const char *oldname, const char *newname )
{
	WCHAR f1[255];
	WCHAR f2[255];
	MultiByteToWideChar(CP_ACP, 0, oldname, -1, f1, 255);
	MultiByteToWideChar(CP_ACP, 0, newname, -1, f2, 255);
	return !MoveFile(f1, f2);
}

int chdir( const char *dirname )
{
	ZeroMemory(_nh_cwd, sizeof(_nh_cwd));
	strncpy(_nh_cwd, dirname, sizeof(_nh_cwd)-1);
	return 0;
}

char *getcwd( char *buffer, int maxlen )
{
	if( maxlen<(int)strlen(_nh_cwd) ) return NULL;
	else return strcpy(buffer, _nh_cwd);
}

/*------------------------------------------------------------------------------*/
/* __errno.h__ */
int errno;

/*------------------------------------------------------------------------------*/
/*
 * Chdrive() changes the default drive.
 */
void
chdrive(char *str)
{
	return;
}

/*
 * This is used in nhlan.c to implement some of the LAN_FEATURES.
 */
char *get_username(lan_username_size)
int *lan_username_size;
{
	static char username_buffer[BUFSZ];
	strcpy(username_buffer, "nhsave");
	return username_buffer;
}

void Delay(int ms)
{
	(void)Sleep(ms);
}

void more()
{

}

int isatty(int f)
{
	return 0;
}


#ifdef WIN_CE_2xx
int __cdecl isupper(int c)
{
	char str[2];
	WCHAR wstr[2];
	str[0] = c;
	str[1] = 0;

	NH_A2W(str, wstr, 1);
	return iswupper(wstr[0]);
}

int __cdecl isdigit(int c)
{
	return  ('0' <= c && c <= '9');
}

int __cdecl isspace(int c)
{
	char str[2];
	WCHAR wstr[2];
	str[0] = c;
	str[1] = 0;

	NH_A2W(str, wstr, 1);
	return iswspace(wstr[0]);
}

int __cdecl isprint(int c)
{
	char str[2];
	WCHAR wstr[2];
	str[0] = c;
	str[1] = 0;

	NH_A2W(str, wstr, 1);
	return iswprint(wstr[0]);
}

char* __cdecl _strdup(const char* s)
{
	char* p;
	p = malloc(strlen(s)+1);
	return strcpy(p, s);
}

char* __cdecl strrchr( const char *s, int c )
{
	WCHAR wstr[1024];
	WCHAR *w;
	w = wcsrchr(NH_A2W(s, wstr, 1024), c);
	if(w) return (char*)(s + (w - wstr));
	else return NULL;
}

int   __cdecl _stricmp(const char* a, const char* b)
{
	 return strncmpi(a, b, 65535u);
}

#endif