/* NetHack 3.6	celib.c	$NHDT-Date: 1432512803 2015/05/25 00:13:23 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#define NEED_VARARGS
#include "hack.h"
#include <fcntl.h>
// #include "wceconf.h"

static union {
    time_t t_val;
    struct time_pack {
        unsigned int ss : 6;
        unsigned int mm : 6;
        unsigned int dd : 5;
        unsigned int hh : 6;
        unsigned int mo : 4;
        unsigned int yr : 10;
        unsigned int wd : 3;
    } tm_val;
} _t_cnv;

#define IS_LEAP(yr) (((yr) % 4 == 0 || (yr) % 100 == 0) && !(yr) % 400 == 0)
static char _day_mo_leap[12] = { 31, 29, 31, 30, 31, 30,
                                 31, 31, 30, 31, 30, 31 };
static char _day_mo[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

struct tm *__cdecl localtime(const time_t *ptime)
{
    static struct tm ptm;
    int i;
    if (!ptime)
        return NULL;

    _t_cnv.t_val = *ptime;

    ptm.tm_sec = _t_cnv.tm_val.ss;     /* seconds after the minute - [0,59] */
    ptm.tm_min = _t_cnv.tm_val.mm;     /* minutes after the hour - [0,59] */
    ptm.tm_hour = _t_cnv.tm_val.hh;    /* hours since midnight - [0,23] */
    ptm.tm_mday = _t_cnv.tm_val.dd;    /* day of the month - [1,31] */
    ptm.tm_mon = _t_cnv.tm_val.mo - 1; /* months since January - [0,11] */
    ptm.tm_year = _t_cnv.tm_val.yr;    /* years since 1900 */
    ptm.tm_wday = _t_cnv.tm_val.wd;    /* days since Sunday - [0,6] */

    ptm.tm_yday = _t_cnv.tm_val.dd; /* days since January 1 - [0,365] */
    for (i = 0; i < ptm.tm_mon; i++)
        ptm.tm_yday +=
            IS_LEAP(_t_cnv.tm_val.yr + 1900) ? _day_mo_leap[i] : _day_mo[i];

    ptm.tm_isdst = 0; /* daylight savings time flag  - NOT IMPLEMENTED */
    return &ptm;
}

time_t __cdecl time(time_t *timeptr)
{
    SYSTEMTIME stm;
    GetLocalTime(&stm);

    _t_cnv.tm_val.yr = stm.wYear - 1900;
    _t_cnv.tm_val.mo = stm.wMonth;
    _t_cnv.tm_val.dd = stm.wDay;
    _t_cnv.tm_val.hh = stm.wHour;
    _t_cnv.tm_val.mm = stm.wMinute;
    _t_cnv.tm_val.ss = stm.wSecond;
    _t_cnv.tm_val.wd = stm.wDayOfWeek;

    if (timeptr)
        *timeptr = _t_cnv.t_val;
    return _t_cnv.t_val;
}

time_t __cdecl mktime(struct tm *tb)
{
    if (!tb)
        return (time_t) -1;

    _t_cnv.tm_val.yr = tb->tm_year;
    _t_cnv.tm_val.mo = tb->tm_mon;
    _t_cnv.tm_val.dd = tb->tm_mday;
    _t_cnv.tm_val.hh = tb->tm_hour;
    _t_cnv.tm_val.mm = tb->tm_min;
    _t_cnv.tm_val.ss = tb->tm_sec;
    _t_cnv.tm_val.wd = tb->tm_wday;

    return _t_cnv.t_val;
}

/*------------------------------------------------------------------------------*/
/* __io.h__ */
/* Hack io.h function with stdio.h functions */
/* ASSUMPTION : int can hold FILE* */
static TCHAR _nh_cwd[MAX_PATH];
const int MAGIC_OFFSET = 5;
#define FILE_TABLE_SIZE 256
static HANDLE _nh_file_table[FILE_TABLE_SIZE];
static int file_pointer = -1;

static HANDLE
get_file_handle(int i)
{
    i -= MAGIC_OFFSET;
    if (i >= 0 && i < FILE_TABLE_SIZE)
        return _nh_file_table[i];
    else
        return INVALID_HANDLE_VALUE;
}

static int
alloc_file_handle(HANDLE h)
{
    int i;
    if (file_pointer == -1) {
        file_pointer = 0;
        for (i = 0; i < FILE_TABLE_SIZE; i++)
            _nh_file_table[i] = INVALID_HANDLE_VALUE;
    }

    i = (file_pointer + 1) % FILE_TABLE_SIZE;
    while (_nh_file_table[i] != INVALID_HANDLE_VALUE) {
        if (i == file_pointer) {
            MessageBox(NULL, _T("Ran out of file handles."),
                       _T("Fatal Error"), MB_OK);
            abort();
        }
        i = (i + 1) % FILE_TABLE_SIZE;
    }

    file_pointer = i;
    _nh_file_table[file_pointer] = h;
    return file_pointer + MAGIC_OFFSET;
}

int __cdecl close(int f)
{
    int retval;
    f -= MAGIC_OFFSET;
    if (f < 0 || f >= FILE_TABLE_SIZE)
        return -1;
    retval = (CloseHandle(_nh_file_table[f]) ? 0 : -1);
    _nh_file_table[f] = INVALID_HANDLE_VALUE;
    return retval;
}

int __cdecl creat(const char *fname, int mode)
{
    HANDLE f;
    TCHAR wbuf[MAX_PATH + 1];
    ZeroMemory(wbuf, sizeof(wbuf));
    NH_A2W(fname, wbuf, MAX_PATH);

    f = CreateFile(wbuf, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                   FILE_ATTRIBUTE_NORMAL, NULL);

    if (f == INVALID_HANDLE_VALUE)
        return -1;
    else
        return alloc_file_handle(f);
}

int __cdecl eof(int f)
{
    DWORD fpos, fsize;
    HANDLE p = get_file_handle(f);

    if (f == -1)
        return -1;

    fpos = SetFilePointer(p, 0, NULL, FILE_CURRENT);
    fsize = SetFilePointer(p, 0, NULL, FILE_END);
    if (fpos == 0xFFFFFFFF || fsize == 0xFFFFFFFF)
        return -1;
    if (fpos == fsize)
        return 1;
    else {
        SetFilePointer(p, fpos, NULL, FILE_BEGIN);
        return 0;
    }
}

long __cdecl lseek(int f, long offset, int origin)
{
    HANDLE p = get_file_handle(f);
    DWORD fpos;
    switch (origin) {
    case SEEK_SET:
        fpos = SetFilePointer(p, offset, NULL, FILE_BEGIN);
        break;
    case SEEK_CUR:
        fpos = SetFilePointer(p, offset, NULL, FILE_CURRENT);
        break;
    case SEEK_END:
        fpos = SetFilePointer(p, offset, NULL, FILE_END);
        break;
    default:
        fpos = 0xFFFFFFFF;
        break;
    }
    if (fpos == 0xFFFFFFFF)
        return -1;
    else
        return (long) fpos;
}

int __cdecl open(const char *filename, int oflag, ...)
{
    TCHAR fname[MAX_PATH + 1];
    TCHAR path[MAX_PATH + 1];
    HANDLE f;
    DWORD fileaccess;
    DWORD filecreate;

    /* O_TEXT is not supported */

    /*
     * decode the access flags
     */
    switch (oflag & (_O_RDONLY | _O_WRONLY | _O_RDWR)) {
    case _O_RDONLY: /* read access */
        fileaccess = GENERIC_READ;
        break;
    case _O_WRONLY: /* write access */
        fileaccess = GENERIC_READ | GENERIC_WRITE;
        break;
    case _O_RDWR: /* read and write access */
        fileaccess = GENERIC_READ | GENERIC_WRITE;
        break;
    default: /* error, bad oflag */
        return -1;
    }

    /*
     * decode open/create method flags
     */
    switch (oflag & (_O_CREAT | _O_EXCL | _O_TRUNC)) {
    case 0:
    case _O_EXCL: // ignore EXCL w/o CREAT
        filecreate = OPEN_EXISTING;
        break;

    case _O_CREAT:
        filecreate = OPEN_ALWAYS;
        break;

    case _O_CREAT | _O_EXCL:
    case _O_CREAT | _O_TRUNC | _O_EXCL:
        filecreate = CREATE_NEW;
        break;

    case _O_TRUNC:
    case _O_TRUNC | _O_EXCL: // ignore EXCL w/o CREAT
        filecreate = TRUNCATE_EXISTING;
        break;

    case _O_CREAT | _O_TRUNC:
        filecreate = CREATE_ALWAYS;
        break;

    default:
        return -1;
    }

    /* assemple the file name */
    ZeroMemory(fname, sizeof(fname));
    ZeroMemory(path, sizeof(path));
    NH_A2W(filename, fname, MAX_PATH);
    if (*filename != '\\' && *filename != '/') {
        _tcscpy(path, _nh_cwd);
        _tcsncat(path, _T("\\"), MAX_PATH - _tcslen(path));
    }
    _tcsncat(path, fname, MAX_PATH - _tcslen(path));

    /*
     * try to open/create the file
     */
    if ((f = CreateFile(path, fileaccess, 0, NULL, filecreate,
                        FILE_ATTRIBUTE_NORMAL, NULL))
        == INVALID_HANDLE_VALUE) {
        return -1;
    }

    if (!(oflag & O_APPEND))
        SetFilePointer(f, 0, NULL, FILE_BEGIN);
    return alloc_file_handle(f);
}

int __cdecl read(int f, void *buffer, unsigned int count)
{
    HANDLE p = get_file_handle(f);
    DWORD bytes_read;
    if (!ReadFile(p, buffer, count, &bytes_read, NULL))
        return -1;
    else
        return (int) bytes_read;
}

int __cdecl unlink(const char *filename)
{
    TCHAR wbuf[MAX_PATH + 1];
    TCHAR fname[MAX_PATH + 1];

    ZeroMemory(wbuf, sizeof(wbuf));
    ZeroMemory(fname, sizeof(fname));
    NH_A2W(filename, wbuf, MAX_PATH);
    if (*filename != '\\' && *filename != '/') {
        _tcscpy(fname, _nh_cwd);
        _tcsncat(fname, _T("\\"), MAX_PATH - _tcslen(fname));
    }
    _tcsncat(fname, wbuf, MAX_PATH - _tcslen(fname));

    return !DeleteFileW(fname);
}

int __cdecl write(int f, const void *buffer, unsigned int count)
{
    HANDLE p = get_file_handle(f);
    DWORD bytes_written;
    if (!WriteFile(p, buffer, count, &bytes_written, NULL))
        return -1;
    else
        return (int) bytes_written;
}

int __cdecl rename(const char *oldname, const char *newname)
{
    WCHAR f1[MAX_PATH + 1];
    WCHAR f2[MAX_PATH + 1];
    ZeroMemory(f1, sizeof(f1));
    ZeroMemory(f2, sizeof(f2));
    MultiByteToWideChar(CP_ACP, 0, oldname, -1, f1, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, newname, -1, f2, MAX_PATH);
    return !MoveFile(f1, f2);
}

int __cdecl access(const char *path, int mode)
{
    DWORD attr;
    WCHAR f[MAX_PATH + 1];
    ZeroMemory(f, sizeof(f));
    MultiByteToWideChar(CP_ACP, 0, path, -1, f, MAX_PATH);

    attr = GetFileAttributes(f);
    if (attr == (DWORD) -1)
        return -1;

    if ((attr & FILE_ATTRIBUTE_READONLY) && (mode & 2))
        return -1;
    else
        return 0;
}

int
chdir(const char *dirname)
{
    ZeroMemory(_nh_cwd, sizeof(_nh_cwd));
    NH_A2W(dirname, _nh_cwd, MAX_PATH);
    return 0;
}

char *
getcwd(char *buffer, int maxlen)
{
    if (maxlen < (int) _tcslen(_nh_cwd))
        return NULL;
    else
        return NH_W2A(_nh_cwd, buffer, maxlen);
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
char *
get_username(lan_username_size)
int *lan_username_size;
{
    static char username_buffer[BUFSZ];
    strcpy(username_buffer, "nhsave");
    return username_buffer;
}

void
Delay(int ms)
{
    (void) Sleep(ms);
}

void
more()
{
}

int
isatty(int f)
{
    return 0;
}

#if defined(WIN_CE_PS2xx) || defined(WIN32_PLATFORM_HPCPRO)
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
    return ('0' <= c && c <= '9');
}

int __cdecl isxdigit(int c)
{
    return (('0' <= c && c <= '9') || ('a' <= c && c <= 'f')
            || ('A' <= c && c <= 'F'));
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

char *__cdecl _strdup(const char *s)
{
    char *p;
    p = malloc(strlen(s) + 1);
    return strcpy(p, s);
}

char *__cdecl strrchr(const char *s, int c)
{
    WCHAR wstr[1024];
    WCHAR *w;
    w = wcsrchr(NH_A2W(s, wstr, 1024), c);
    if (w)
        return (char *) (s + (w - wstr));
    else
        return NULL;
}

int __cdecl _stricmp(const char *a, const char *b)
{
    return strncmpi(a, b, 65535u);
}

#endif

#if defined(WIN_CE_PS2xx)
/* stdio.h functions are missing from PAlm Size PC SDK 1.2 (SH3 and MIPS) */

#pragma warning(disable : 4273)

FILE *__cdecl fopen(const char *filename, const char *mode)
{
    int modeflag;
    int whileflag;
    int filedes;

    /* First mode character must be 'r', 'w', or 'a'. */
    switch (*mode) {
    case 'r':
        modeflag = _O_RDONLY;
        break;
    case 'w':
        modeflag = _O_WRONLY | _O_CREAT | _O_TRUNC;
        break;
    case 'a':
        modeflag = _O_WRONLY | _O_CREAT | _O_APPEND;
        break;
    default:
        return NULL;
    }

    whileflag = 1;
    while (*++mode && whileflag)
        switch (*mode) {
        case '+':
            if (modeflag & _O_RDWR)
                whileflag = 0;
            else {
                modeflag |= _O_RDWR;
                modeflag &= ~(_O_RDONLY | _O_WRONLY);
            }
            break;

        case 'b':
            if (modeflag & (_O_TEXT | _O_BINARY))
                whileflag = 0;
            else
                modeflag |= _O_BINARY;
            break;

        case 't': /* not supported */
            whileflag = 0;
            break;

        default:
            whileflag = 0;
            break;
        }

    if ((filedes = open(filename, modeflag)) == -1)
        return NULL;

    return (FILE *) filedes;
}

int __cdecl fscanf(FILE *f, const char *format, ...)
{
    /* Format spec:  %[*] [width] [l] type ] */
    int ch;
    int sch;
    int matched = 0;
    int width = 65535;
    int modifier = -1;
    int skip_flag = 0;
    int n_read = 0;
    char buf[BUFSZ];
    TCHAR wbuf[BUFSZ];
    char *p;
    va_list args;

#define RETURN_SCANF(i) \
    {                   \
        va_end(args);   \
        return i;       \
    }
#define NEXT_CHAR(f) (n_read++, fgetc(f))

    va_start(args, format);

    ch = *format++;
    sch = NEXT_CHAR(f);
    while (ch && sch != EOF) {
        if (isspace(ch)) {
            while (ch && isspace(ch))
                ch = *format++;
            while (sch != EOF && isspace(sch))
                sch = NEXT_CHAR(f);
            format--;
            goto next_spec;
        }

        /* read % */
        if (ch != '%') {
            if (sch != ch)
                RETURN_SCANF(matched);
            sch = NEXT_CHAR(f);
            goto next_spec;
        } else {
            /* process '%%' */
            ch = *format++;
            if (ch == '%') {
                if (sch != '%')
                    RETURN_SCANF(matched);
                sch = NEXT_CHAR(f);
                goto next_spec;
            }

            if (ch == '*') {
                /* read skip flag - '*' */
                skip_flag = 1;
                ch = *format++;
            }

            /* get width */
            if (isdigit(ch)) {
                width = 0;
                while (ch && isdigit(ch)) {
                    width = width * 10 + (ch - '0');
                    ch = *format++;
                }
            }

            /* get modifier */
            if (ch == 'l') {
                modifier = 'l';
                ch = *format++;
            }

            /* get type */
            switch (ch) {
            case 'c':
                if (!skip_flag) {
                    *(va_arg(args, char *) ) = sch;
                    matched++;
                }
                sch = NEXT_CHAR(f);
                goto next_spec;
            case 'd':
                p = buf;
                /* skip space */
                while (sch != EOF && isspace(sch))
                    sch = NEXT_CHAR(f);
                while (sch != EOF && isdigit(sch) && --width >= 0) {
                    *p++ = sch;
                    sch = NEXT_CHAR(f);
                }
                *p = '\x0';
                if (!skip_flag) {
                    matched++;
                    if (modifier == 'l') {
                        *(va_arg(args, long *) ) =
                            wcstol(NH_A2W(buf, wbuf, BUFSZ), NULL, 10);
                    } else {
                        *(va_arg(args, int *) ) =
                            wcstol(NH_A2W(buf, wbuf, BUFSZ), NULL, 10);
                    }
                }
                goto next_spec;
            case 'x':
                p = buf;
                while (sch != EOF && isspace(sch))
                    sch = NEXT_CHAR(f);
                while (sch != EOF && isxdigit(sch) && --width >= 0) {
                    *p++ = sch;
                    sch = NEXT_CHAR(f);
                }
                *p = '\x0';
                if (!skip_flag) {
                    matched++;
                    if (modifier == 'l') {
                        *(va_arg(args, long *) ) =
                            wcstol(NH_A2W(buf, wbuf, BUFSZ), NULL, 16);
                    } else {
                        *(va_arg(args, int *) ) =
                            wcstol(NH_A2W(buf, wbuf, BUFSZ), NULL, 16);
                    }
                }
                goto next_spec;
            case 'n':
                *(va_arg(args, int *) ) = n_read;
                matched++;
                goto next_spec;
            case 's':
                if (skip_flag) {
                    while (sch != EOF && !isspace(sch) && --width >= 0) {
                        sch = NEXT_CHAR(f);
                    }
                } else {
                    p = va_arg(args, char *);
                    while (sch != EOF && !isspace(sch) && --width >= 0) {
                        *p++ = sch;
                        sch = NEXT_CHAR(f);
                    }
                    *p = '\x0';
                    matched++;
                }
                goto next_spec;
            case '[': {
                char pattern[256];
                int start, end;
                int negate;

                ZeroMemory(pattern, sizeof(pattern));
                p = pattern;

                /* try to parse '^' modifier */
                ch = *format++;
                if (ch == '^') {
                    negate = 1;
                    ch = *format++;
                } else {
                    negate = 0;
                }
                if (ch == 0)
                    RETURN_SCANF(EOF);

                for (; ch && ch != ']'; ch = *format++) {
                    /* try to parse range: a-z */
                    if (format[0] == '-' && format[1] && format[1] != ']') {
                        start = ch;
                        format++;
                        end = *format++;
                        while (start <= end) {
                            if (!strchr(pattern, (char) start))
                                *p++ = (char) start;
                            start++;
                        }
                    } else {
                        if (!strchr(pattern, (char) ch))
                            *p++ = (char) ch;
                    }
                }

                if (skip_flag) {
                    while (sch != EOF && strchr(pattern, sch)
                           && --width >= 0) {
                        sch = NEXT_CHAR(f);
                    }
                } else {
                    p = va_arg(args, char *);
                    if (negate)
                        while (sch != EOF && !strchr(pattern, sch)
                               && --width >= 0) {
                            *p++ = sch;
                            sch = NEXT_CHAR(f);
                        }
                    else
                        while (sch != EOF && strchr(pattern, sch)
                               && --width >= 0) {
                            *p++ = sch;
                            sch = NEXT_CHAR(f);
                        }
                    *p = '\x0';
                    matched++;
                }
            }
                goto next_spec;
            default:
                RETURN_SCANF(EOF);
            }
        }

    next_spec:
        width = 65535;
        modifier = -1;
        skip_flag = 0;
        ch = *format++;
    }
    fseek(f, -1, SEEK_CUR);
    RETURN_SCANF(matched);

#undef RETURN_SCANF
#undef NEXT_CHAR
}

int __cdecl fprintf(FILE *f, const char *format, ...)
{
    int retval;
    va_list args;

    if (!f || !format)
        return 0;

    va_start(args, format);
    retval = vfprintf(f, format, args);
    va_end(args);

    return retval;
}

int __cdecl vfprintf(FILE *f, const char *format, va_list args)
{
    char buf[4096];
    int retval;

    if (!f || !format)
        return 0;

    retval = vsprintf(buf, format, args);

    write((int) f, buf, strlen(buf));

    return retval;
}

int __cdecl fgetc(FILE *f)
{
    char c;
    int fh = (int) f;

    if (!f)
        return EOF;
    if (read(fh, &c, 1) == 1)
        return c;
    else
        return EOF;
}

char *__cdecl fgets(char *s, int size, FILE *f)
{
    /* not the best performance but it will do for now...*/
    char c;
    if (!f || !s || size == 0)
        return NULL;
    while (--size > 0) {
        if ((c = fgetc(f)) == EOF)
            return NULL;

        *s++ = c;
        if (c == '\n')
            break;
    }
    *s = '\x0';
    return s;
}

int __cdecl printf(const char *format, ...)
{
    int retval;
    va_list args;

    if (!format)
        return 0;

    va_start(args, format);
    retval = vprintf(format, args);
    va_end(args);

    return retval;
}

int __cdecl vprintf(const char *format, va_list args)
{
    char buf[4096];
    int retval;

    retval = vsprintf(buf, format, args);
    puts(buf);
    return retval;
}

// int    __cdecl putchar(int);
int __cdecl puts(const char *s)
{
    TCHAR wbuf[4096];
    NH_A2W(s, wbuf, 4096);
    MessageBox(NULL, wbuf, _T("stdout"), MB_OK);
    return 0;
}

FILE *__cdecl _getstdfilex(int desc)
{
    return NULL;
}

int __cdecl fclose(FILE *f)
{
    if (!f)
        return EOF;
    return close((int) f) == -1 ? EOF : 0;
}

size_t __cdecl fread(void *p, size_t size, size_t count, FILE *f)
{
    int read_bytes;
    if (!f || !p || size == 0 || count == 0)
        return 0;
    read_bytes = read((int) f, p, size * count);
    return read_bytes > 0 ? (read_bytes / size) : 0;
}

size_t __cdecl fwrite(const void *p, size_t size, size_t count, FILE *f)
{
    int write_bytes;
    if (!f || !p || size == 0 || count == 0)
        return 0;
    write_bytes = write((int) f, p, size * count);
    return write_bytes > 0 ? write_bytes / size : 0;
}

int __cdecl fflush(FILE *f)
{
    return 0;
}

int __cdecl feof(FILE *f)
{
    return (f && eof((int) f) == 0) ? 0 : 1;
}

int __cdecl fseek(FILE *f, long offset, int from)
{
    return (f && lseek((int) f, offset, from) >= 0) ? 0 : 1;
}

long __cdecl ftell(FILE *f)
{
    return f ? lseek((int) f, 0, SEEK_CUR) : -1;
}

#endif
