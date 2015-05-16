# 1 "lev_main.c"
# 1 "<built-in>"
# 1 "<command line>"
# 1 "lev_main.c"
# 14 "lev_main.c"
# 1 "../include/hack.h" 1
# 11 "../include/hack.h"
# 1 "../include/config.h" 1
# 39 "../include/config.h"
# 1 "../include/config1.h" 1
# 40 "../include/config.h" 2
# 345 "../include/config.h"
# 1 "../include/tradstdc.h" 1
# 88 "../include/tradstdc.h"
# 1 "/usr/lib/gcc/i686-apple-darwin9/4.0.1/include/stdarg.h" 1 3 4
# 43 "/usr/lib/gcc/i686-apple-darwin9/4.0.1/include/stdarg.h" 3 4
typedef __builtin_va_list __gnuc_va_list;
# 105 "/usr/lib/gcc/i686-apple-darwin9/4.0.1/include/stdarg.h" 3 4
typedef __gnuc_va_list va_list;
# 89 "../include/tradstdc.h" 2
# 222 "../include/tradstdc.h"
typedef void * genericptr_t;
# 346 "../include/config.h" 2
# 359 "../include/config.h"
typedef signed char schar;
# 373 "../include/config.h"
typedef unsigned char uchar;
# 443 "../include/config.h"
# 1 "../include/global.h" 1
# 9 "../include/global.h"
# 1 "/usr/include/stdio.h" 1 3 4
# 64 "/usr/include/stdio.h" 3 4
# 1 "/usr/include/_types.h" 1 3 4
# 27 "/usr/include/_types.h" 3 4
# 1 "/usr/include/sys/_types.h" 1 3 4
# 32 "/usr/include/sys/_types.h" 3 4
# 1 "/usr/include/sys/cdefs.h" 1 3 4
# 33 "/usr/include/sys/_types.h" 2 3 4
# 1 "/usr/include/machine/_types.h" 1 3 4
# 34 "/usr/include/machine/_types.h" 3 4
# 1 "/usr/include/i386/_types.h" 1 3 4
# 37 "/usr/include/i386/_types.h" 3 4
typedef signed char __int8_t;



typedef unsigned char __uint8_t;
typedef short __int16_t;
typedef unsigned short __uint16_t;
typedef int __int32_t;
typedef unsigned int __uint32_t;
typedef long long __int64_t;
typedef unsigned long long __uint64_t;

typedef long __darwin_intptr_t;
typedef unsigned int __darwin_natural_t;
# 70 "/usr/include/i386/_types.h" 3 4
typedef int __darwin_ct_rune_t;





typedef union {
 char __mbstate8[128];
 long long _mbstateL;
} __mbstate_t;

typedef __mbstate_t __darwin_mbstate_t;


typedef int __darwin_ptrdiff_t;





typedef long unsigned int __darwin_size_t;





typedef __builtin_va_list __darwin_va_list;





typedef int __darwin_wchar_t;




typedef __darwin_wchar_t __darwin_rune_t;


typedef int __darwin_wint_t;




typedef unsigned long __darwin_clock_t;
typedef __uint32_t __darwin_socklen_t;
typedef long __darwin_ssize_t;
typedef long __darwin_time_t;
# 35 "/usr/include/machine/_types.h" 2 3 4
# 34 "/usr/include/sys/_types.h" 2 3 4
# 58 "/usr/include/sys/_types.h" 3 4
struct __darwin_pthread_handler_rec
{
 void (*__routine)(void *);
 void *__arg;
 struct __darwin_pthread_handler_rec *__next;
};
struct _opaque_pthread_attr_t { long __sig; char __opaque[36]; };
struct _opaque_pthread_cond_t { long __sig; char __opaque[24]; };
struct _opaque_pthread_condattr_t { long __sig; char __opaque[4]; };
struct _opaque_pthread_mutex_t { long __sig; char __opaque[40]; };
struct _opaque_pthread_mutexattr_t { long __sig; char __opaque[8]; };
struct _opaque_pthread_once_t { long __sig; char __opaque[4]; };
struct _opaque_pthread_rwlock_t { long __sig; char __opaque[124]; };
struct _opaque_pthread_rwlockattr_t { long __sig; char __opaque[12]; };
struct _opaque_pthread_t { long __sig; struct __darwin_pthread_handler_rec *__cleanup_stack; char __opaque[596]; };
# 94 "/usr/include/sys/_types.h" 3 4
typedef __int64_t __darwin_blkcnt_t;
typedef __int32_t __darwin_blksize_t;
typedef __int32_t __darwin_dev_t;
typedef unsigned int __darwin_fsblkcnt_t;
typedef unsigned int __darwin_fsfilcnt_t;
typedef __uint32_t __darwin_gid_t;
typedef __uint32_t __darwin_id_t;
typedef __uint64_t __darwin_ino64_t;



typedef __uint32_t __darwin_ino_t;

typedef __darwin_natural_t __darwin_mach_port_name_t;
typedef __darwin_mach_port_name_t __darwin_mach_port_t;
typedef __uint16_t __darwin_mode_t;
typedef __int64_t __darwin_off_t;
typedef __int32_t __darwin_pid_t;
typedef struct _opaque_pthread_attr_t
   __darwin_pthread_attr_t;
typedef struct _opaque_pthread_cond_t
   __darwin_pthread_cond_t;
typedef struct _opaque_pthread_condattr_t
   __darwin_pthread_condattr_t;
typedef unsigned long __darwin_pthread_key_t;
typedef struct _opaque_pthread_mutex_t
   __darwin_pthread_mutex_t;
typedef struct _opaque_pthread_mutexattr_t
   __darwin_pthread_mutexattr_t;
typedef struct _opaque_pthread_once_t
   __darwin_pthread_once_t;
typedef struct _opaque_pthread_rwlock_t
   __darwin_pthread_rwlock_t;
typedef struct _opaque_pthread_rwlockattr_t
   __darwin_pthread_rwlockattr_t;
typedef struct _opaque_pthread_t
   *__darwin_pthread_t;
typedef __uint32_t __darwin_sigset_t;
typedef __int32_t __darwin_suseconds_t;
typedef __uint32_t __darwin_uid_t;
typedef __uint32_t __darwin_useconds_t;
typedef unsigned char __darwin_uuid_t[16];
# 28 "/usr/include/_types.h" 2 3 4

typedef int __darwin_nl_item;
typedef int __darwin_wctrans_t;



typedef unsigned long __darwin_wctype_t;
# 65 "/usr/include/stdio.h" 2 3 4
# 75 "/usr/include/stdio.h" 3 4
typedef __darwin_off_t off_t;




typedef __darwin_size_t size_t;






typedef __darwin_off_t fpos_t;
# 98 "/usr/include/stdio.h" 3 4
struct __sbuf {
 unsigned char *_base;
 int _size;
};


struct __sFILEX;
# 132 "/usr/include/stdio.h" 3 4
typedef struct __sFILE {
 unsigned char *_p;
 int _r;
 int _w;
 short _flags;
 short _file;
 struct __sbuf _bf;
 int _lbfsize;


 void *_cookie;
 int (*_close)(void *);
 int (*_read) (void *, char *, int);
 fpos_t (*_seek) (void *, fpos_t, int);
 int (*_write)(void *, const char *, int);


 struct __sbuf _ub;
 struct __sFILEX *_extra;
 int _ur;


 unsigned char _ubuf[3];
 unsigned char _nbuf[1];


 struct __sbuf _lb;


 int _blksize;
 fpos_t _offset;
} FILE;



extern FILE *__stdinp;
extern FILE *__stdoutp;
extern FILE *__stderrp;




# 248 "/usr/include/stdio.h" 3 4

void clearerr(FILE *);
int fclose(FILE *);
int feof(FILE *);
int ferror(FILE *);
int fflush(FILE *);
int fgetc(FILE *);
int fgetpos(FILE * , fpos_t *);
char *fgets(char * , int, FILE *);
FILE *fopen(const char * , const char * );
int fprintf(FILE * , const char * , ...) ;
int fputc(int, FILE *);
int fputs(const char * , FILE * ) __asm("_" "fputs" "$UNIX2003");
size_t fread(void * , size_t, size_t, FILE * );
FILE *freopen(const char * , const char * ,
     FILE * ) __asm("_" "freopen" "$UNIX2003");
int fscanf(FILE * , const char * , ...) ;
int fseek(FILE *, long, int);
int fsetpos(FILE *, const fpos_t *);
long ftell(FILE *);
size_t fwrite(const void * , size_t, size_t, FILE * ) __asm("_" "fwrite" "$UNIX2003");
int getc(FILE *);
int getchar(void);
char *gets(char *);

extern const int sys_nerr;
extern const char *const sys_errlist[];

void perror(const char *);
int printf(const char * , ...) ;
int putc(int, FILE *);
int putchar(int);
int puts(const char *);
int remove(const char *);
int rename (const char *, const char *);
void rewind(FILE *);
int scanf(const char * , ...) ;
void setbuf(FILE * , char * );
int setvbuf(FILE * , char * , int, size_t);
int sprintf(char * , const char * , ...) ;
int sscanf(const char * , const char * , ...) ;
FILE *tmpfile(void);
char *tmpnam(char *);
int ungetc(int, FILE *);
int vfprintf(FILE * , const char * , va_list) ;
int vprintf(const char * , va_list) ;
int vsprintf(char * , const char * , va_list) ;

int asprintf(char **, const char *, ...) ;
int vasprintf(char **, const char *, va_list) ;










char *ctermid(char *);

char *ctermid_r(char *);

FILE *fdopen(int, const char *);

char *fgetln(FILE *, size_t *);

int fileno(FILE *);
void flockfile(FILE *);

const char
 *fmtcheck(const char *, const char *);
int fpurge(FILE *);

int fseeko(FILE *, off_t, int);
off_t ftello(FILE *);
int ftrylockfile(FILE *);
void funlockfile(FILE *);
int getc_unlocked(FILE *);
int getchar_unlocked(void);

int getw(FILE *);

int pclose(FILE *);
FILE *popen(const char *, const char *);
int putc_unlocked(int, FILE *);
int putchar_unlocked(int);

int putw(int, FILE *);
void setbuffer(FILE *, char *, int);
int setlinebuf(FILE *);

int snprintf(char * , size_t, const char * , ...) ;
char *tempnam(const char *, const char *) __asm("_" "tempnam" "$UNIX2003");
int vfscanf(FILE * , const char * , va_list) ;
int vscanf(const char * , va_list) ;
int vsnprintf(char * , size_t, const char * , va_list) ;
int vsscanf(const char * , const char * , va_list) ;

FILE *zopen(const char *, const char *, int);








FILE *funopen(const void *,
  int (*)(void *, char *, int),
  int (*)(void *, const char *, int),
  fpos_t (*)(void *, fpos_t, int),
  int (*)(void *));

# 371 "/usr/include/stdio.h" 3 4

int __srget(FILE *);
int __svfscanf(FILE *, const char *, va_list) ;
int __swbuf(int, FILE *);








static __inline int __sputc(int _c, FILE *_p) {
 if (--_p->_w >= 0 || (_p->_w >= _p->_lbfsize && (char)_c != '\n'))
  return (*_p->_p++ = _c);
 else
  return (__swbuf(_c, _p));
}
# 10 "../include/global.h" 2
# 61 "../include/global.h"
typedef schar xchar;

typedef xchar boolean;
# 74 "../include/global.h"
typedef uchar nhsym;
# 110 "../include/global.h"
# 1 "../include/coord.h" 1
# 9 "../include/coord.h"
typedef struct nhcoord {
 xchar x,y;
} coord;
# 111 "../include/global.h" 2
# 121 "../include/global.h"
# 1 "../include/unixconf.h" 1
# 285 "../include/unixconf.h"
# 1 "/usr/include/time.h" 1 3 4
# 69 "/usr/include/time.h" 3 4
# 1 "/usr/include/_structs.h" 1 3 4
# 24 "/usr/include/_structs.h" 3 4
# 1 "/usr/include/sys/_structs.h" 1 3 4
# 88 "/usr/include/sys/_structs.h" 3 4
struct timespec
{
 __darwin_time_t tv_sec;
 long tv_nsec;
};
# 25 "/usr/include/_structs.h" 2 3 4
# 70 "/usr/include/time.h" 2 3 4







typedef __darwin_clock_t clock_t;
# 87 "/usr/include/time.h" 3 4
typedef __darwin_time_t time_t;


struct tm {
 int tm_sec;
 int tm_min;
 int tm_hour;
 int tm_mday;
 int tm_mon;
 int tm_year;
 int tm_wday;
 int tm_yday;
 int tm_isdst;
 long tm_gmtoff;
 char *tm_zone;
};
# 113 "/usr/include/time.h" 3 4
extern char *tzname[];


extern int getdate_err;

extern long timezone __asm("_" "timezone" "$UNIX2003");

extern int daylight;


char *asctime(const struct tm *);
clock_t clock(void) __asm("_" "clock" "$UNIX2003");
char *ctime(const time_t *);
double difftime(time_t, time_t);
struct tm *getdate(const char *);
struct tm *gmtime(const time_t *);
struct tm *localtime(const time_t *);
time_t mktime(struct tm *) __asm("_" "mktime" "$UNIX2003");
size_t strftime(char * , size_t, const char * , const struct tm * ) __asm("_" "strftime" "$UNIX2003");
char *strptime(const char * , const char * , struct tm * ) __asm("_" "strptime" "$UNIX2003");
time_t time(time_t *);


void tzset(void);



char *asctime_r(const struct tm * , char * );
char *ctime_r(const time_t *, char *);
struct tm *gmtime_r(const time_t * , struct tm * );
struct tm *localtime_r(const time_t * , struct tm * );


time_t posix2time(time_t);



void tzsetwall(void);
time_t time2posix(time_t);
time_t timelocal(struct tm * const);
time_t timegm(struct tm * const);



int nanosleep(const struct timespec *, struct timespec *) __asm("_" "nanosleep" "$UNIX2003");


# 286 "../include/unixconf.h" 2
# 296 "../include/unixconf.h"
# 1 "../include/system.h" 1
# 21 "../include/system.h"
# 1 "/usr/include/sys/types.h" 1 3 4
# 72 "/usr/include/sys/types.h" 3 4
# 1 "/usr/include/sys/appleapiopts.h" 1 3 4
# 73 "/usr/include/sys/types.h" 2 3 4





# 1 "/usr/include/machine/types.h" 1 3 4
# 37 "/usr/include/machine/types.h" 3 4
# 1 "/usr/include/i386/types.h" 1 3 4
# 78 "/usr/include/i386/types.h" 3 4
typedef signed char int8_t;

typedef unsigned char u_int8_t;


typedef short int16_t;

typedef unsigned short u_int16_t;


typedef int int32_t;

typedef unsigned int u_int32_t;


typedef long long int64_t;

typedef unsigned long long u_int64_t;




typedef int32_t register_t;




typedef __darwin_intptr_t intptr_t;



typedef unsigned long int uintptr_t;




typedef u_int64_t user_addr_t;
typedef u_int64_t user_size_t;
typedef int64_t user_ssize_t;
typedef int64_t user_long_t;
typedef u_int64_t user_ulong_t;
typedef int64_t user_time_t;





typedef u_int64_t syscall_arg_t;
# 38 "/usr/include/machine/types.h" 2 3 4
# 79 "/usr/include/sys/types.h" 2 3 4


# 1 "/usr/include/machine/endian.h" 1 3 4
# 37 "/usr/include/machine/endian.h" 3 4
# 1 "/usr/include/i386/endian.h" 1 3 4
# 99 "/usr/include/i386/endian.h" 3 4
# 1 "/usr/include/sys/_endian.h" 1 3 4
# 124 "/usr/include/sys/_endian.h" 3 4
# 1 "/usr/include/libkern/_OSByteOrder.h" 1 3 4
# 66 "/usr/include/libkern/_OSByteOrder.h" 3 4
# 1 "/usr/include/libkern/i386/_OSByteOrder.h" 1 3 4
# 44 "/usr/include/libkern/i386/_OSByteOrder.h" 3 4
static __inline__
__uint16_t
_OSSwapInt16(
    __uint16_t _data
)
{
    return ((_data << 8) | (_data >> 8));
}

static __inline__
__uint32_t
_OSSwapInt32(
    __uint32_t _data
)
{
    __asm__ ("bswap   %0" : "+r" (_data));
    return _data;
}


static __inline__
__uint64_t
_OSSwapInt64(
    __uint64_t _data
)
{
    __asm__ ("bswap   %%eax\n\t"
             "bswap   %%edx\n\t"
             "xchgl   %%eax, %%edx"
             : "+A" (_data));
    return _data;
}
# 67 "/usr/include/libkern/_OSByteOrder.h" 2 3 4
# 125 "/usr/include/sys/_endian.h" 2 3 4
# 100 "/usr/include/i386/endian.h" 2 3 4
# 38 "/usr/include/machine/endian.h" 2 3 4
# 82 "/usr/include/sys/types.h" 2 3 4


typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;

typedef unsigned long u_long;


typedef unsigned short ushort;
typedef unsigned int uint;


typedef u_int64_t u_quad_t;
typedef int64_t quad_t;
typedef quad_t * qaddr_t;

typedef char * caddr_t;
typedef int32_t daddr_t;


typedef __darwin_dev_t dev_t;



typedef u_int32_t fixpt_t;


typedef __darwin_blkcnt_t blkcnt_t;




typedef __darwin_blksize_t blksize_t;




typedef __darwin_gid_t gid_t;





typedef __uint32_t in_addr_t;




typedef __uint16_t in_port_t;



typedef __darwin_ino_t ino_t;





typedef __darwin_ino64_t ino64_t;






typedef __int32_t key_t;



typedef __darwin_mode_t mode_t;




typedef __uint16_t nlink_t;





typedef __darwin_id_t id_t;



typedef __darwin_pid_t pid_t;
# 176 "/usr/include/sys/types.h" 3 4
typedef int32_t segsz_t;
typedef int32_t swblk_t;


typedef __darwin_uid_t uid_t;
# 235 "/usr/include/sys/types.h" 3 4
typedef __darwin_ssize_t ssize_t;
# 245 "/usr/include/sys/types.h" 3 4
typedef __darwin_useconds_t useconds_t;




typedef __darwin_suseconds_t suseconds_t;
# 260 "/usr/include/sys/types.h" 3 4
# 1 "/usr/include/sys/_structs.h" 1 3 4
# 183 "/usr/include/sys/_structs.h" 3 4

typedef struct fd_set {
 __int32_t fds_bits[(((1024) + (((sizeof(__int32_t) * 8)) - 1)) / ((sizeof(__int32_t) * 8)))];
} fd_set;



static __inline int
__darwin_fd_isset(int _n, struct fd_set *_p)
{
 return (_p->fds_bits[_n/(sizeof(__int32_t) * 8)] & (1<<(_n % (sizeof(__int32_t) * 8))));
}
# 261 "/usr/include/sys/types.h" 2 3 4




typedef __int32_t fd_mask;
# 318 "/usr/include/sys/types.h" 3 4
typedef __darwin_pthread_attr_t pthread_attr_t;



typedef __darwin_pthread_cond_t pthread_cond_t;



typedef __darwin_pthread_condattr_t pthread_condattr_t;



typedef __darwin_pthread_mutex_t pthread_mutex_t;



typedef __darwin_pthread_mutexattr_t pthread_mutexattr_t;



typedef __darwin_pthread_once_t pthread_once_t;



typedef __darwin_pthread_rwlock_t pthread_rwlock_t;



typedef __darwin_pthread_rwlockattr_t pthread_rwlockattr_t;



typedef __darwin_pthread_t pthread_t;






typedef __darwin_pthread_key_t pthread_key_t;





typedef __darwin_fsblkcnt_t fsblkcnt_t;




typedef __darwin_fsfilcnt_t fsfilcnt_t;
# 22 "../include/system.h" 2
# 92 "../include/system.h"
extern long lrand48();
extern void srand48();





extern void exit (int);
# 118 "../include/system.h"
extern void free (genericptr_t);
# 127 "../include/system.h"
extern void perror (const char *);






extern void qsort (genericptr_t,size_t,size_t, int(*)(const void *,const void *));
# 301 "../include/system.h"
# 1 "/usr/include/string.h" 1 3 4
# 80 "/usr/include/string.h" 3 4

void *memchr(const void *, int, size_t);
int memcmp(const void *, const void *, size_t);
void *memcpy(void *, const void *, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);

char *stpcpy(char *, const char *);
char *strcasestr(const char *, const char *);

char *strcat(char *, const char *);
char *strchr(const char *, int);
int strcmp(const char *, const char *);
int strcoll(const char *, const char *);
char *strcpy(char *, const char *);
size_t strcspn(const char *, const char *);
char *strerror(int) __asm("_" "strerror" "$UNIX2003");
int strerror_r(int, char *, size_t);
size_t strlen(const char *);
char *strncat(char *, const char *, size_t);
int strncmp(const char *, const char *, size_t);
char *strncpy(char *, const char *, size_t);

char *strnstr(const char *, const char *, size_t);

char *strpbrk(const char *, const char *);
char *strrchr(const char *, int);
size_t strspn(const char *, const char *);
char *strstr(const char *, const char *);
char *strtok(char *, const char *);
size_t strxfrm(char *, const char *, size_t);



void *memccpy(void *, const void *, int, size_t);
char *strtok_r(char *, const char *, char **);
char *strdup(const char *);

int bcmp(const void *, const void *, size_t);
void bcopy(const void *, void *, size_t);
void bzero(void *, size_t);
int ffs(int);
int ffsl(long);
int fls(int);
int flsl(long);
char *index(const char *, int);
void memset_pattern4(void *, const void *, size_t);
void memset_pattern8(void *, const void *, size_t);
void memset_pattern16(void *, const void *, size_t);
char *rindex(const char *, int);
int strcasecmp(const char *, const char *);
size_t strlcat(char *, const char *, size_t);
size_t strlcpy(char *, const char *, size_t);
void strmode(int, char *);
int strncasecmp(const char *, const char *, size_t);
char *strsep(char **, const char *);
char *strsignal(int sig);
void swab(const void * , void * , ssize_t);



# 302 "../include/system.h" 2
# 353 "../include/system.h"
extern unsigned sleep();
# 362 "../include/system.h"
extern char *getenv (const char *);
extern char *getlogin();






extern pid_t getpid(void);
extern uid_t getuid(void);
extern gid_t getgid(void);
# 507 "../include/system.h"
extern int tgetent (char *,const char *);
extern void tputs (const char *,int,int (*)());

extern int tgetnum (const char *);
extern int tgetflag (const char *);
extern char *tgetstr (const char *,char **);
extern char *tgoto (const char *,int,int);
# 524 "../include/system.h"
extern struct tm *localtime (const time_t *);




extern time_t time (time_t *);
# 297 "../include/unixconf.h" 2


# 1 "/usr/include/stdlib.h" 1 3 4
# 61 "/usr/include/stdlib.h" 3 4
# 1 "/usr/include/available.h" 1 3 4
# 62 "/usr/include/stdlib.h" 2 3 4



# 1 "/usr/include/sys/wait.h" 1 3 4
# 79 "/usr/include/sys/wait.h" 3 4
typedef enum {
 P_ALL,
 P_PID,
 P_PGID
} idtype_t;
# 116 "/usr/include/sys/wait.h" 3 4
# 1 "/usr/include/sys/signal.h" 1 3 4
# 81 "/usr/include/sys/signal.h" 3 4
# 1 "/usr/include/machine/signal.h" 1 3 4
# 34 "/usr/include/machine/signal.h" 3 4
# 1 "/usr/include/i386/signal.h" 1 3 4
# 39 "/usr/include/i386/signal.h" 3 4
typedef int sig_atomic_t;
# 55 "/usr/include/i386/signal.h" 3 4
# 1 "/usr/include/i386/_structs.h" 1 3 4
# 56 "/usr/include/i386/signal.h" 2 3 4
# 35 "/usr/include/machine/signal.h" 2 3 4
# 82 "/usr/include/sys/signal.h" 2 3 4
# 154 "/usr/include/sys/signal.h" 3 4
# 1 "/usr/include/sys/_structs.h" 1 3 4
# 57 "/usr/include/sys/_structs.h" 3 4
# 1 "/usr/include/machine/_structs.h" 1 3 4
# 31 "/usr/include/machine/_structs.h" 3 4
# 1 "/usr/include/i386/_structs.h" 1 3 4
# 38 "/usr/include/i386/_structs.h" 3 4
# 1 "/usr/include/mach/i386/_structs.h" 1 3 4
# 43 "/usr/include/mach/i386/_structs.h" 3 4
struct __darwin_i386_thread_state
{
    unsigned int __eax;
    unsigned int __ebx;
    unsigned int __ecx;
    unsigned int __edx;
    unsigned int __edi;
    unsigned int __esi;
    unsigned int __ebp;
    unsigned int __esp;
    unsigned int __ss;
    unsigned int __eflags;
    unsigned int __eip;
    unsigned int __cs;
    unsigned int __ds;
    unsigned int __es;
    unsigned int __fs;
    unsigned int __gs;
};
# 89 "/usr/include/mach/i386/_structs.h" 3 4
struct __darwin_fp_control
{
    unsigned short __invalid :1,
        __denorm :1,
    __zdiv :1,
    __ovrfl :1,
    __undfl :1,
    __precis :1,
      :2,
    __pc :2,





    __rc :2,






             :1,
      :3;
};
typedef struct __darwin_fp_control __darwin_fp_control_t;
# 147 "/usr/include/mach/i386/_structs.h" 3 4
struct __darwin_fp_status
{
    unsigned short __invalid :1,
        __denorm :1,
    __zdiv :1,
    __ovrfl :1,
    __undfl :1,
    __precis :1,
    __stkflt :1,
    __errsumm :1,
    __c0 :1,
    __c1 :1,
    __c2 :1,
    __tos :3,
    __c3 :1,
    __busy :1;
};
typedef struct __darwin_fp_status __darwin_fp_status_t;
# 191 "/usr/include/mach/i386/_structs.h" 3 4
struct __darwin_mmst_reg
{
 char __mmst_reg[10];
 char __mmst_rsrv[6];
};
# 210 "/usr/include/mach/i386/_structs.h" 3 4
struct __darwin_xmm_reg
{
 char __xmm_reg[16];
};
# 232 "/usr/include/mach/i386/_structs.h" 3 4
struct __darwin_i386_float_state
{
 int __fpu_reserved[2];
 struct __darwin_fp_control __fpu_fcw;
 struct __darwin_fp_status __fpu_fsw;
 __uint8_t __fpu_ftw;
 __uint8_t __fpu_rsrv1;
 __uint16_t __fpu_fop;
 __uint32_t __fpu_ip;
 __uint16_t __fpu_cs;
 __uint16_t __fpu_rsrv2;
 __uint32_t __fpu_dp;
 __uint16_t __fpu_ds;
 __uint16_t __fpu_rsrv3;
 __uint32_t __fpu_mxcsr;
 __uint32_t __fpu_mxcsrmask;
 struct __darwin_mmst_reg __fpu_stmm0;
 struct __darwin_mmst_reg __fpu_stmm1;
 struct __darwin_mmst_reg __fpu_stmm2;
 struct __darwin_mmst_reg __fpu_stmm3;
 struct __darwin_mmst_reg __fpu_stmm4;
 struct __darwin_mmst_reg __fpu_stmm5;
 struct __darwin_mmst_reg __fpu_stmm6;
 struct __darwin_mmst_reg __fpu_stmm7;
 struct __darwin_xmm_reg __fpu_xmm0;
 struct __darwin_xmm_reg __fpu_xmm1;
 struct __darwin_xmm_reg __fpu_xmm2;
 struct __darwin_xmm_reg __fpu_xmm3;
 struct __darwin_xmm_reg __fpu_xmm4;
 struct __darwin_xmm_reg __fpu_xmm5;
 struct __darwin_xmm_reg __fpu_xmm6;
 struct __darwin_xmm_reg __fpu_xmm7;
 char __fpu_rsrv4[14*16];
 int __fpu_reserved1;
};
# 308 "/usr/include/mach/i386/_structs.h" 3 4
struct __darwin_i386_exception_state
{
    unsigned int __trapno;
    unsigned int __err;
    unsigned int __faultvaddr;
};
# 326 "/usr/include/mach/i386/_structs.h" 3 4
struct __darwin_x86_debug_state32
{
 unsigned int __dr0;
 unsigned int __dr1;
 unsigned int __dr2;
 unsigned int __dr3;
 unsigned int __dr4;
 unsigned int __dr5;
 unsigned int __dr6;
 unsigned int __dr7;
};
# 358 "/usr/include/mach/i386/_structs.h" 3 4
struct __darwin_x86_thread_state64
{
 __uint64_t __rax;
 __uint64_t __rbx;
 __uint64_t __rcx;
 __uint64_t __rdx;
 __uint64_t __rdi;
 __uint64_t __rsi;
 __uint64_t __rbp;
 __uint64_t __rsp;
 __uint64_t __r8;
 __uint64_t __r9;
 __uint64_t __r10;
 __uint64_t __r11;
 __uint64_t __r12;
 __uint64_t __r13;
 __uint64_t __r14;
 __uint64_t __r15;
 __uint64_t __rip;
 __uint64_t __rflags;
 __uint64_t __cs;
 __uint64_t __fs;
 __uint64_t __gs;
};
# 413 "/usr/include/mach/i386/_structs.h" 3 4
struct __darwin_x86_float_state64
{
 int __fpu_reserved[2];
 struct __darwin_fp_control __fpu_fcw;
 struct __darwin_fp_status __fpu_fsw;
 __uint8_t __fpu_ftw;
 __uint8_t __fpu_rsrv1;
 __uint16_t __fpu_fop;


 __uint32_t __fpu_ip;
 __uint16_t __fpu_cs;

 __uint16_t __fpu_rsrv2;


 __uint32_t __fpu_dp;
 __uint16_t __fpu_ds;

 __uint16_t __fpu_rsrv3;
 __uint32_t __fpu_mxcsr;
 __uint32_t __fpu_mxcsrmask;
 struct __darwin_mmst_reg __fpu_stmm0;
 struct __darwin_mmst_reg __fpu_stmm1;
 struct __darwin_mmst_reg __fpu_stmm2;
 struct __darwin_mmst_reg __fpu_stmm3;
 struct __darwin_mmst_reg __fpu_stmm4;
 struct __darwin_mmst_reg __fpu_stmm5;
 struct __darwin_mmst_reg __fpu_stmm6;
 struct __darwin_mmst_reg __fpu_stmm7;
 struct __darwin_xmm_reg __fpu_xmm0;
 struct __darwin_xmm_reg __fpu_xmm1;
 struct __darwin_xmm_reg __fpu_xmm2;
 struct __darwin_xmm_reg __fpu_xmm3;
 struct __darwin_xmm_reg __fpu_xmm4;
 struct __darwin_xmm_reg __fpu_xmm5;
 struct __darwin_xmm_reg __fpu_xmm6;
 struct __darwin_xmm_reg __fpu_xmm7;
 struct __darwin_xmm_reg __fpu_xmm8;
 struct __darwin_xmm_reg __fpu_xmm9;
 struct __darwin_xmm_reg __fpu_xmm10;
 struct __darwin_xmm_reg __fpu_xmm11;
 struct __darwin_xmm_reg __fpu_xmm12;
 struct __darwin_xmm_reg __fpu_xmm13;
 struct __darwin_xmm_reg __fpu_xmm14;
 struct __darwin_xmm_reg __fpu_xmm15;
 char __fpu_rsrv4[6*16];
 int __fpu_reserved1;
};
# 517 "/usr/include/mach/i386/_structs.h" 3 4
struct __darwin_x86_exception_state64
{
    unsigned int __trapno;
    unsigned int __err;
    __uint64_t __faultvaddr;
};
# 535 "/usr/include/mach/i386/_structs.h" 3 4
struct __darwin_x86_debug_state64
{
 __uint64_t __dr0;
 __uint64_t __dr1;
 __uint64_t __dr2;
 __uint64_t __dr3;
 __uint64_t __dr4;
 __uint64_t __dr5;
 __uint64_t __dr6;
 __uint64_t __dr7;
};
# 39 "/usr/include/i386/_structs.h" 2 3 4
# 48 "/usr/include/i386/_structs.h" 3 4
struct __darwin_mcontext32
{
 struct __darwin_i386_exception_state __es;
 struct __darwin_i386_thread_state __ss;
 struct __darwin_i386_float_state __fs;
};
# 68 "/usr/include/i386/_structs.h" 3 4
struct __darwin_mcontext64
{
 struct __darwin_x86_exception_state64 __es;
 struct __darwin_x86_thread_state64 __ss;
 struct __darwin_x86_float_state64 __fs;
};
# 94 "/usr/include/i386/_structs.h" 3 4
typedef struct __darwin_mcontext32 *mcontext_t;
# 32 "/usr/include/machine/_structs.h" 2 3 4
# 58 "/usr/include/sys/_structs.h" 2 3 4
# 75 "/usr/include/sys/_structs.h" 3 4
struct __darwin_sigaltstack
{
 void *ss_sp;
 __darwin_size_t ss_size;
 int ss_flags;
};
# 128 "/usr/include/sys/_structs.h" 3 4
struct __darwin_ucontext
{
 int uc_onstack;
 __darwin_sigset_t uc_sigmask;
 struct __darwin_sigaltstack uc_stack;
 struct __darwin_ucontext *uc_link;
 __darwin_size_t uc_mcsize;
 struct __darwin_mcontext32 *uc_mcontext;



};
# 218 "/usr/include/sys/_structs.h" 3 4
typedef struct __darwin_sigaltstack stack_t;
# 227 "/usr/include/sys/_structs.h" 3 4
typedef struct __darwin_ucontext ucontext_t;
# 155 "/usr/include/sys/signal.h" 2 3 4
# 168 "/usr/include/sys/signal.h" 3 4
typedef __darwin_sigset_t sigset_t;
# 181 "/usr/include/sys/signal.h" 3 4
union sigval {

 int sival_int;
 void *sival_ptr;
};





struct sigevent {
 int sigev_notify;
 int sigev_signo;
 union sigval sigev_value;
 void (*sigev_notify_function)(union sigval);
 pthread_attr_t *sigev_notify_attributes;
};


typedef struct __siginfo {
 int si_signo;
 int si_errno;
 int si_code;
 pid_t si_pid;
 uid_t si_uid;
 int si_status;
 void *si_addr;
 union sigval si_value;
 long si_band;
 unsigned long __pad[7];
} siginfo_t;
# 292 "/usr/include/sys/signal.h" 3 4
union __sigaction_u {
 void (*__sa_handler)(int);
 void (*__sa_sigaction)(int, struct __siginfo *,
         void *);
};


struct __sigaction {
 union __sigaction_u __sigaction_u;
 void (*sa_tramp)(void *, int, int, siginfo_t *, void *);
 sigset_t sa_mask;
 int sa_flags;
};




struct sigaction {
 union __sigaction_u __sigaction_u;
 sigset_t sa_mask;
 int sa_flags;
};
# 354 "/usr/include/sys/signal.h" 3 4
typedef void (*sig_t)(int);
# 371 "/usr/include/sys/signal.h" 3 4
struct sigvec {
 void (*sv_handler)(int);
 int sv_mask;
 int sv_flags;
};
# 390 "/usr/include/sys/signal.h" 3 4
struct sigstack {
 char *ss_sp;
 int ss_onstack;
};
# 412 "/usr/include/sys/signal.h" 3 4

void (*signal(int, void (*)(int)))(int);

# 117 "/usr/include/sys/wait.h" 2 3 4
# 1 "/usr/include/sys/resource.h" 1 3 4
# 76 "/usr/include/sys/resource.h" 3 4
# 1 "/usr/include/sys/_structs.h" 1 3 4
# 100 "/usr/include/sys/_structs.h" 3 4
struct timeval
{
 __darwin_time_t tv_sec;
 __darwin_suseconds_t tv_usec;
};
# 77 "/usr/include/sys/resource.h" 2 3 4
# 88 "/usr/include/sys/resource.h" 3 4
typedef __uint64_t rlim_t;
# 142 "/usr/include/sys/resource.h" 3 4
struct rusage {
 struct timeval ru_utime;
 struct timeval ru_stime;
# 153 "/usr/include/sys/resource.h" 3 4
 long ru_maxrss;

 long ru_ixrss;
 long ru_idrss;
 long ru_isrss;
 long ru_minflt;
 long ru_majflt;
 long ru_nswap;
 long ru_inblock;
 long ru_oublock;
 long ru_msgsnd;
 long ru_msgrcv;
 long ru_nsignals;
 long ru_nvcsw;
 long ru_nivcsw;


};
# 213 "/usr/include/sys/resource.h" 3 4
struct rlimit {
 rlim_t rlim_cur;
 rlim_t rlim_max;
};
# 235 "/usr/include/sys/resource.h" 3 4

int getpriority(int, id_t);

int getiopolicy_np(int, int);

int getrlimit(int, struct rlimit *) __asm("_" "getrlimit" "$UNIX2003");
int getrusage(int, struct rusage *);
int setpriority(int, id_t, int);

int setiopolicy_np(int, int, int);

int setrlimit(int, const struct rlimit *) __asm("_" "setrlimit" "$UNIX2003");

# 118 "/usr/include/sys/wait.h" 2 3 4
# 201 "/usr/include/sys/wait.h" 3 4
union wait {
 int w_status;



 struct {

  unsigned int w_Termsig:7,
    w_Coredump:1,
    w_Retcode:8,
    w_Filler:16;







 } w_T;





 struct {

  unsigned int w_Stopval:8,
    w_Stopsig:8,
    w_Filler:16;






 } w_S;
};
# 254 "/usr/include/sys/wait.h" 3 4

pid_t wait(int *) __asm("_" "wait" "$UNIX2003");
pid_t waitpid(pid_t, int *, int) __asm("_" "waitpid" "$UNIX2003");

int waitid(idtype_t, id_t, siginfo_t *, int) __asm("_" "waitid" "$UNIX2003");


pid_t wait3(int *, int, struct rusage *);
pid_t wait4(pid_t, int *, int, struct rusage *);


# 66 "/usr/include/stdlib.h" 2 3 4

# 1 "/usr/include/alloca.h" 1 3 4
# 35 "/usr/include/alloca.h" 3 4

void *alloca(size_t);

# 68 "/usr/include/stdlib.h" 2 3 4
# 81 "/usr/include/stdlib.h" 3 4
typedef __darwin_ct_rune_t ct_rune_t;




typedef __darwin_rune_t rune_t;






typedef __darwin_wchar_t wchar_t;



typedef struct {
 int quot;
 int rem;
} div_t;

typedef struct {
 long quot;
 long rem;
} ldiv_t;


typedef struct {
 long long quot;
 long long rem;
} lldiv_t;
# 134 "/usr/include/stdlib.h" 3 4
extern int __mb_cur_max;
# 144 "/usr/include/stdlib.h" 3 4

void abort(void) __attribute__((__noreturn__));
int abs(int) __attribute__((__const__));
int atexit(void (*)(void));
double atof(const char *);
int atoi(const char *);
long atol(const char *);

long long
  atoll(const char *);

void *bsearch(const void *, const void *, size_t,
     size_t, int (*)(const void *, const void *));
void *calloc(size_t, size_t);
div_t div(int, int) __attribute__((__const__));
void exit(int) __attribute__((__noreturn__));
void free(void *);
char *getenv(const char *);
long labs(long) __attribute__((__const__));
ldiv_t ldiv(long, long) __attribute__((__const__));

long long
  llabs(long long);
lldiv_t lldiv(long long, long long);

void *malloc(size_t);
int mblen(const char *, size_t);
size_t mbstowcs(wchar_t * , const char * , size_t);
int mbtowc(wchar_t * , const char * , size_t);
void qsort(void *, size_t, size_t,
     int (*)(const void *, const void *));
int rand(void);
void *realloc(void *, size_t);
void srand(unsigned);
double strtod(const char *, char **) __asm("_" "strtod" "$UNIX2003");
float strtof(const char *, char **) __asm("_" "strtof" "$UNIX2003");
long strtol(const char *, char **, int);
long double
  strtold(const char *, char **) ;

long long
  strtoll(const char *, char **, int);

unsigned long
  strtoul(const char *, char **, int);

unsigned long long
  strtoull(const char *, char **, int);

int system(const char *) __asm("_" "system" "$UNIX2003");
size_t wcstombs(char * , const wchar_t * , size_t);
int wctomb(char *, wchar_t);


void _Exit(int) __attribute__((__noreturn__));
long a64l(const char *);
double drand48(void);
char *ecvt(double, int, int *, int *);
double erand48(unsigned short[3]);
char *fcvt(double, int, int *, int *);
char *gcvt(double, int, char *);
int getsubopt(char **, char * const *, char **);
int grantpt(int);

char *initstate(unsigned, char *, size_t);



long jrand48(unsigned short[3]);
char *l64a(long);
void lcong48(unsigned short[7]);
long lrand48(void);
char *mktemp(char *);
int mkstemp(char *);
long mrand48(void);
long nrand48(unsigned short[3]);
int posix_openpt(int);
char *ptsname(int);
int putenv(char *) __asm("_" "putenv" "$UNIX2003");
long random(void);
int rand_r(unsigned *);

char *realpath(const char * , char * ) __asm("_" "realpath" "$DARWIN_EXTSN");



unsigned short
 *seed48(unsigned short[3]);
int setenv(const char *, const char *, int) __asm("_" "setenv" "$UNIX2003");

void setkey(const char *) __asm("_" "setkey" "$UNIX2003");



char *setstate(const char *);
void srand48(long);

void srandom(unsigned);



int unlockpt(int);

int unsetenv(const char *) __asm("_" "unsetenv" "$UNIX2003");
# 266 "/usr/include/stdlib.h" 3 4
u_int32_t
  arc4random(void);
void arc4random_addrandom(unsigned char *dat, int datlen);
void arc4random_stir(void);


char *cgetcap(char *, const char *, int);
int cgetclose(void);
int cgetent(char **, char **, const char *);
int cgetfirst(char **, char **);
int cgetmatch(const char *, const char *);
int cgetnext(char **, char **);
int cgetnum(char *, const char *, long *);
int cgetset(const char *);
int cgetstr(char *, const char *, char **);
int cgetustr(char *, const char *, char **);

int daemon(int, int) __asm("_" "daemon" "$1050") __attribute__((deprecated));
char *devname(dev_t, mode_t);
char *devname_r(dev_t, mode_t, char *buf, int len);
char *getbsize(int *, long *);
int getloadavg(double [], int);
const char
 *getprogname(void);

int heapsort(void *, size_t, size_t,
     int (*)(const void *, const void *));
int mergesort(void *, size_t, size_t,
     int (*)(const void *, const void *));
void qsort_r(void *, size_t, size_t, void *,
     int (*)(void *, const void *, const void *));
int radixsort(const unsigned char **, int, const unsigned char *,
     unsigned);
void setprogname(const char *);
int sradixsort(const unsigned char **, int, const unsigned char *,
     unsigned);
void sranddev(void);
void srandomdev(void);
void *reallocf(void *, size_t);

long long
  strtoq(const char *, char **, int);
unsigned long long
  strtouq(const char *, char **, int);

extern char *suboptarg;
void *valloc(size_t);







# 300 "../include/unixconf.h" 2
# 1 "/usr/include/unistd.h" 1 3 4
# 72 "/usr/include/unistd.h" 3 4
# 1 "/usr/include/sys/unistd.h" 1 3 4
# 138 "/usr/include/sys/unistd.h" 3 4
struct accessx_descriptor {
 unsigned int ad_name_offset;
 int ad_flags;
 int ad_pad[2];
};
# 73 "/usr/include/unistd.h" 2 3 4
# 133 "/usr/include/unistd.h" 3 4
typedef __darwin_uuid_t uuid_t;
# 414 "/usr/include/unistd.h" 3 4


void _exit(int) __attribute__((__noreturn__));
int access(const char *, int);
unsigned int
  alarm(unsigned int);
int chdir(const char *);
int chown(const char *, uid_t, gid_t);
int close(int) __asm("_" "close" "$UNIX2003");
size_t confstr(int, char *, size_t) __asm("_" "confstr" "$UNIX2003");
char *crypt(const char *, const char *);
char *ctermid(char *);
int dup(int);
int dup2(int, int);

void encrypt(char *, int) __asm("_" "encrypt" "$UNIX2003");



int execl(const char *, const char *, ...);
int execle(const char *, const char *, ...);
int execlp(const char *, const char *, ...);
int execv(const char *, char * const *);
int execve(const char *, char * const *, char * const *);
int execvp(const char *, char * const *);
int fchown(int, uid_t, gid_t);
int fchdir(int);
pid_t fork(void);
long fpathconf(int, int);
int fsync(int) __asm("_" "fsync" "$UNIX2003");
int ftruncate(int, off_t);
char *getcwd(char *, size_t);
gid_t getegid(void);
uid_t geteuid(void);
gid_t getgid(void);
int getgroups(int, gid_t []);
long gethostid(void);
int gethostname(char *, size_t);
char *getlogin(void);
int getlogin_r(char *, size_t);
int getopt(int, char * const [], const char *) __asm("_" "getopt" "$UNIX2003");
pid_t getpgid(pid_t);
pid_t getpgrp(void);
pid_t getpid(void);
pid_t getppid(void);
pid_t getsid(pid_t);
uid_t getuid(void);
char *getwd(char *);
int isatty(int);
int lchown(const char *, uid_t, gid_t) __asm("_" "lchown" "$UNIX2003");
int link(const char *, const char *);
int lockf(int, int, off_t) __asm("_" "lockf" "$UNIX2003");
off_t lseek(int, off_t, int);
int nice(int) __asm("_" "nice" "$UNIX2003");
long pathconf(const char *, int);
int pause(void) __asm("_" "pause" "$UNIX2003");
int pipe(int [2]);
ssize_t pread(int, void *, size_t, off_t) __asm("_" "pread" "$UNIX2003");
ssize_t pwrite(int, const void *, size_t, off_t) __asm("_" "pwrite" "$UNIX2003");
ssize_t read(int, void *, size_t) __asm("_" "read" "$UNIX2003");
ssize_t readlink(const char * , char * , size_t);
int rmdir(const char *);
int setegid(gid_t);
int seteuid(uid_t);
int setgid(gid_t);
int setpgid(pid_t, pid_t);

pid_t setpgrp(void) __asm("_" "setpgrp" "$UNIX2003");



int setregid(gid_t, gid_t) __asm("_" "setregid" "$UNIX2003");
int setreuid(uid_t, uid_t) __asm("_" "setreuid" "$UNIX2003");
pid_t setsid(void);
int setuid(uid_t);
unsigned int
  sleep(unsigned int) __asm("_" "sleep" "$UNIX2003");
void swab(const void * , void * , ssize_t);
int symlink(const char *, const char *);
void sync(void);
long sysconf(int);
pid_t tcgetpgrp(int);
int tcsetpgrp(int, pid_t);
int truncate(const char *, off_t);
char *ttyname(int);

int ttyname_r(int, char *, size_t) __asm("_" "ttyname_r" "$UNIX2003");



useconds_t
  ualarm(useconds_t, useconds_t);
int unlink(const char *);
int usleep(useconds_t) __asm("_" "usleep" "$UNIX2003");
pid_t vfork(void);
ssize_t write(int, const void *, size_t) __asm("_" "write" "$UNIX2003");

extern char *optarg;
extern int optind, opterr, optopt;


# 1 "/usr/include/sys/select.h" 1 3 4
# 78 "/usr/include/sys/select.h" 3 4
# 1 "/usr/include/sys/_structs.h" 1 3 4
# 79 "/usr/include/sys/select.h" 2 3 4
# 134 "/usr/include/sys/select.h" 3 4



int pselect(int, fd_set * , fd_set * ,
  fd_set * , const struct timespec * ,
  const sigset_t * )






  __asm("_" "pselect" "$UNIX2003")


  ;


# 1 "/usr/include/sys/_select.h" 1 3 4
# 39 "/usr/include/sys/_select.h" 3 4
int select(int, fd_set * , fd_set * ,
  fd_set * , struct timeval * )






  __asm("_" "select" "$UNIX2003")


  ;
# 153 "/usr/include/sys/select.h" 2 3 4


# 516 "/usr/include/unistd.h" 2 3 4

void _Exit(int) __attribute__((__noreturn__));
int accessx_np(const struct accessx_descriptor *, size_t, int *, uid_t);
int acct(const char *);
int add_profil(char *, size_t, unsigned long, unsigned int);
void *brk(const void *);
int chroot(const char *);
void endusershell(void);
int execvP(const char *, const char *, char * const *);
char *fflagstostr(unsigned long);
int getdtablesize(void);
int getdomainname(char *, int);
int getgrouplist(const char *, int, int *, int *);
mode_t getmode(const void *, mode_t);
int getpagesize(void) __attribute__((__const__));
char *getpass(const char *);
int getpeereid(int, uid_t *, gid_t *);
int getpgid(pid_t _pid);
int getsgroups_np(int *, uuid_t);
int getsid(pid_t _pid);
char *getusershell(void);
int getwgroups_np(int *, uuid_t);
int initgroups(const char *, int);
int iruserok(unsigned long, int, const char *, const char *);
int iruserok_sa(const void *, int, int, const char *, const char *);
int issetugid(void);
char *mkdtemp(char *);
int mknod(const char *, mode_t, dev_t);
int mkstemp(char *);
int mkstemps(char *, int);
char *mktemp(char *);
int nfssvc(int, void *);
int profil(char *, size_t, unsigned long, unsigned int);
int pthread_setugid_np(uid_t, gid_t);
int pthread_getugid_np( uid_t *, gid_t *);
int rcmd(char **, int, const char *, const char *, const char *, int *);
int rcmd_af(char **, int, const char *, const char *, const char *, int *,
  int);
int reboot(int);
int revoke(const char *);
int rresvport(int *);
int rresvport_af(int *, int);
int ruserok(const char *, int, const char *, const char *);
void *sbrk(int);
int setdomainname(const char *, int);
int setgroups(int, const gid_t *);
void sethostid(long);
int sethostname(const char *, int);

void setkey(const char *) __asm("_" "setkey" "$UNIX2003");



int setlogin(const char *);
void *setmode(const char *);
int setrgid(gid_t);
int setruid(uid_t);
int setsgroups_np(int, const uuid_t);
void setusershell(void);
int setwgroups_np(int, const uuid_t);
int strtofflags(char **, unsigned long *, unsigned long *);
int swapon(const char *);
int syscall(int, ...);
int ttyslot(void);
int undelete(const char *);
int unwhiteout(const char *);
void *valloc(size_t);

extern char *suboptarg;
int getsubopt(char **, char * const *, char **);
# 597 "/usr/include/unistd.h" 3 4
int getattrlist(const char*,void*,void*,size_t,unsigned long) __asm("_" "getattrlist" "$UNIX2003");
int setattrlist(const char*,void*,void*,size_t,unsigned long) __asm("_" "setattrlist" "$UNIX2003");
int exchangedata(const char*,const char*,unsigned long);
int getdirentriesattr(int,void*,void*,size_t,unsigned long*,unsigned long*,unsigned long*,unsigned long);
int searchfs(const char*,void*,void*,unsigned long,unsigned long,void*);

int fsctl(const char *,unsigned long,void*,unsigned long);


extern int optreset;



# 301 "../include/unixconf.h" 2
# 122 "../include/global.h" 2
# 274 "../include/global.h"
extern long *alloc (unsigned int);
extern char *dupstr (const char *);




struct version_info {
 unsigned long incarnation;
 unsigned long feature_set;
 unsigned long entity_count;
 unsigned long struct_sizes1;
 unsigned long struct_sizes2;
};

struct savefile_info {
 unsigned long sfi1;
 unsigned long sfi2;
 unsigned long sfi3;
};
# 444 "../include/config.h" 2
# 12 "../include/hack.h" 2

# 1 "../include/lint.h" 1
# 14 "../include/hack.h" 2
# 120 "../include/hack.h"
# 1 "../include/align.h" 1
# 10 "../include/align.h"
typedef schar aligntyp;

typedef struct align {
 aligntyp type;
 int record;
} align;
# 121 "../include/hack.h" 2
# 1 "../include/dungeon.h" 1
# 9 "../include/dungeon.h"
typedef struct d_flags {
 unsigned town:1;
 unsigned hellish:1;
 unsigned maze_like:1;
 unsigned rogue_like:1;
 unsigned align:3;
 unsigned unused:1;
} d_flags;

typedef struct d_level {
 xchar dnum;
 xchar dlevel;
} d_level;

typedef struct s_level {
 struct s_level *next;
 d_level dlevel;
 char proto[15];
 char boneid;
 uchar rndlevs;
 d_flags flags;
} s_level;

typedef struct stairway {
 xchar sx, sy;
 d_level tolev;
 char up;
} stairway;
# 47 "../include/dungeon.h"
typedef struct dest_area {
 xchar lx, ly;
 xchar hx, hy;
 xchar nlx, nly;
 xchar nhx, nhy;
} dest_area;

typedef struct dungeon {
 char dname[24];
 char proto[15];
 char boneid;
 d_flags flags;
 xchar entry_lev;
 xchar num_dunlevs;
 xchar dunlev_ureached;
 int ledger_start,
  depth_start;
} dungeon;







typedef struct branch {
    struct branch *next;
    int id;
    int type;
    d_level end1;
    d_level end2;
    boolean end1_up;
} branch;
# 153 "../include/dungeon.h"
struct linfo {
 unsigned char flags;
# 172 "../include/dungeon.h"
};
# 197 "../include/dungeon.h"
typedef struct mapseen {
 struct mapseen *next;
 branch *br;
 d_level lev;
 struct mapseen_feat {

     unsigned nfount:2;
     unsigned nsink:2;
     unsigned naltar:2;
     unsigned nthrone:2;

     unsigned ngrave:2;
     unsigned ntree:2;
     unsigned water:2;
     unsigned lava:2;

     unsigned ice:2;

     unsigned nshop:2;
     unsigned ntemple:2;


     unsigned msalign:2;

     unsigned shoptype:5;
 } feat;
 struct mapseen_flags {
     unsigned unreachable:1;
     unsigned forgot:1;
     unsigned knownbones:1;
     unsigned oracle:1;
     unsigned sokosolved:1;
     unsigned bigroom:1;
     unsigned castle:1;
     unsigned castletune:1;

     unsigned valley:1;
     unsigned msanctum:1;
     unsigned ludios:1;
     unsigned roguelevel:1;
 } flags;

 char *custom;
 unsigned custom_lth;
 struct mapseen_rooms {
     unsigned seen:1;
     unsigned untended:1;
 } msrooms[(40 +1)*2];

 struct cemetery *final_resting_place;
} mapseen;
# 122 "../include/hack.h" 2
# 1 "../include/monsym.h" 1
# 123 "../include/hack.h" 2
# 1 "../include/mkroom.h" 1
# 11 "../include/mkroom.h"
struct mkroom {
 schar lx,hx,ly,hy;
 schar rtype;
 schar orig_rtype;
 schar rlit;
 schar needfill;
 schar needjoining;
 schar doorct;
 schar fdoor;
 schar nsubrooms;
 boolean irregular;
 struct mkroom *sbrooms[24];
 struct monst *resident;
};

struct shclass {
 const char *name;
 char symb;
 int prob;
 schar shdist;



 struct itp {
     int iprob;
     int itype;
 } iprobs[6];
 const char * const *shknms;
};

extern struct mkroom rooms[(40 +1)*2];
extern struct mkroom* subrooms;







extern struct mkroom *dnstairs_room, *upstairs_room, *sstairs_room;

extern coord doors[120];
# 124 "../include/hack.h" 2
# 1 "../include/objclass.h" 1
# 12 "../include/objclass.h"
struct objclass {
 short oc_name_idx;
 short oc_descr_idx;
 char * oc_uname;
 unsigned oc_name_known:1;
 unsigned oc_merge:1;
 unsigned oc_uses_known:1;



 unsigned oc_pre_discovered:1;

 unsigned oc_magic:1;
 unsigned oc_charged:1;
 unsigned oc_unique:1;
 unsigned oc_nowish:1;

 unsigned oc_big:1;


 unsigned oc_tough:1;

 unsigned oc_dir:2;
# 45 "../include/objclass.h"
 unsigned oc_material:5;
# 82 "../include/objclass.h"
 schar oc_subtyp;
# 93 "../include/objclass.h"
 uchar oc_oprop;
 char oc_class;
 schar oc_delay;
 uchar oc_color;

 short oc_prob;
 unsigned short oc_weight;
 short oc_cost;


 schar oc_wsdam, oc_wldam;
 schar oc_oc1, oc_oc2;






 unsigned short oc_nutrition;
};

struct class_sym {
 char sym;
 const char *name;
 const char *explain;
};

struct objdescr {
 const char *oc_name;
 const char *oc_descr;
};

extern struct objclass objects[];
extern struct objdescr obj_descr[];
# 184 "../include/objclass.h"
struct fruit {
 char fname[32];
 int fid;
 struct fruit *nextf;
};
# 125 "../include/hack.h" 2
# 1 "../include/youprop.h" 1
# 9 "../include/youprop.h"
# 1 "../include/prop.h" 1
# 89 "../include/prop.h"
struct prop {

 long extrinsic;
# 118 "../include/prop.h"
 long blocked;


 long intrinsic;
# 132 "../include/prop.h"
};
# 10 "../include/youprop.h" 2
# 1 "../include/permonst.h" 1
# 22 "../include/permonst.h"
struct attack {
 uchar aatyp;
 uchar adtyp, damn, damd;
};
# 40 "../include/permonst.h"
# 1 "../include/monattk.h" 1
# 41 "../include/permonst.h" 2
# 1 "../include/monflag.h" 1
# 42 "../include/permonst.h" 2

struct permonst {
 const char *mname;
 char mlet;
 schar mlevel,
   mmove,
   ac,
   mr;
 aligntyp maligntyp;
 unsigned short geno;
 struct attack mattk[6];
 unsigned short cwt,
   cnutrit;
 uchar msound;
 uchar msize;
 uchar mresists;
 uchar mconveys;
 unsigned long mflags1,
   mflags2;
 unsigned short mflags3;

 uchar mcolor;

};

extern struct permonst
  mons[];
# 11 "../include/youprop.h" 2
# 1 "../include/mondata.h" 1
# 12 "../include/youprop.h" 2
# 1 "../include/pm.h" 1
# 13 "../include/youprop.h" 2
# 126 "../include/hack.h" 2
# 1 "../include/wintype.h" 1
# 10 "../include/wintype.h"
typedef int winid;


typedef union any {
    genericptr_t a_void;
    struct obj *a_obj;
    struct monst *a_monst;
    int a_int;
    char a_char;
    schar a_schar;
    unsigned int a_uint;
    long a_long;
    unsigned long a_ulong;
    int *a_iptr;
    long *a_lptr;
    unsigned long *a_ulptr;
    unsigned *a_uptr;

} anything;
# 51 "../include/wintype.h"
typedef struct mi {
    anything item;
    long count;
} menu_item;
# 127 "../include/hack.h" 2
# 1 "../include/context.h" 1
# 23 "../include/context.h"
struct dig_info {
 int effort;
 d_level level;
 coord pos;
 long lastdigtime;
 boolean down, chew, warned, quiet;
};

struct tin_info {
 struct obj *tin;
 unsigned o_id;
 int usedtime, reqtime;
};

struct book_info {
 struct obj *book;
 unsigned o_id;
 schar delay;
};

struct takeoff_info {
 long mask;
 long what;
 int delay;
 boolean cancelled_don;
 char disrobing[30 +1];
};

struct victual_info {
 struct obj *piece;



 unsigned o_id;

 int usedtime,
  reqtime;
 int nmod;
 unsigned canchoke:1;


 unsigned fullwarn:1;
 unsigned eating:1;
 unsigned doreset:1;
};

struct warntype_info {
 unsigned long obj;
 unsigned long polyd;
 struct permonst *species;
 short speciesidx;
};

struct polearm_info {
 struct monst *hitmon;
 unsigned m_id;
};

struct tribute_info {
 size_t tributesz;
 boolean enabled;
 unsigned bookstock:1;


};

struct context_info {
 unsigned ident;
 unsigned no_of_wizards;
 unsigned run;


 unsigned startingpet_mid;
 int current_fruit;
 int warnlevel;
 int rndencode;
 long next_attrib_check;
 long stethoscope_move;
 short stethoscope_movement;
 boolean travel;
 boolean travel1;
 boolean forcefight;
 boolean nopick;
 boolean made_amulet;
 boolean mon_moving;
 boolean move;
 boolean mv;
 boolean bypasses;
 boolean botl;
 boolean botlx;
 boolean door_opened;
 struct dig_info digging;
 struct victual_info victual;
 struct tin_info tin;
 struct book_info spbook;
 struct takeoff_info takeoff;
 struct warntype_info warntype;
 struct polearm_info polearm;
 struct tribute_info tribute;
};

extern struct context_info context;
# 128 "../include/hack.h" 2
# 1 "../include/decl.h" 1
# 11 "../include/decl.h"
extern int (*occupation)(void);
extern int (*afternmv)(void);

extern const char *hname;
extern int hackpid;

extern int locknum;





extern char SAVEF[];




extern int bases[18];

extern int multi;
extern const char *multi_reason;
extern int nroom;
extern int nsubroom;
extern int occtime;


extern nhsym warnsyms[6];
extern int warn_obj_cnt;

extern int x_maze_max, y_maze_max;
extern int otg_temp;

extern int in_doagain;

extern struct dgn_topology {
    d_level d_oracle_level;
    d_level d_bigroom_level;
    d_level d_rogue_level;
    d_level d_medusa_level;
    d_level d_stronghold_level;
    d_level d_valley_level;
    d_level d_wiz1_level;
    d_level d_wiz2_level;
    d_level d_wiz3_level;
    d_level d_juiblex_level;
    d_level d_orcus_level;
    d_level d_baalzebub_level;
    d_level d_asmodeus_level;
    d_level d_portal_level;
    d_level d_sanctum_level;
    d_level d_earth_level;
    d_level d_water_level;
    d_level d_fire_level;
    d_level d_air_level;
    d_level d_astral_level;
    xchar d_tower_dnum;
    xchar d_sokoban_dnum;
    xchar d_mines_dnum, d_quest_dnum;
    d_level d_qstart_level, d_qlocate_level, d_nemesis_level;
    d_level d_knox_level;
    d_level d_mineend_level;
    d_level d_sokoend_level;
} dungeon_topology;
# 106 "../include/decl.h"
extern stairway dnstair, upstair;





extern stairway dnladder, upladder;





extern stairway sstairs;

extern dest_area updest, dndest;

extern coord inv_pos;
extern dungeon dungeons[];
extern s_level *sp_levchn;


# 1 "../include/quest.h" 1
# 10 "../include/quest.h"
struct q_score {
 unsigned first_start:1;
 unsigned met_leader:1;
 unsigned not_ready:3;
 unsigned pissed_off:1;
 unsigned got_quest:1;

 unsigned first_locate:1;
 unsigned met_intermed:1;
 unsigned got_final:1;

 unsigned made_goal:3;
 unsigned met_nemesis:1;
 unsigned killed_nemesis:1;
 unsigned in_battle:1;

 unsigned cheater:1;
 unsigned touched_artifact:1;
 unsigned offered_artifact:1;
 unsigned got_thanks:1;




 unsigned ldrgend:2;
 unsigned nemgend:2;
 unsigned godgend:2;



 unsigned leader_is_dead:1;
 unsigned leader_m_id;
};
# 128 "../include/decl.h" 2
extern struct q_score quest_status;

extern char pl_character[32];
extern char pl_race;

extern char pl_fruit[32];
extern struct fruit *ffruit;

extern char tune[6];


extern struct linfo level_info[(16 * 32)];

extern struct sinfo {
 int gameover;
 int stopprint;

 volatile int done_hup;

 int preserve_locks;

 int something_worth_saving;
 int panicking;
 int exiting;
 int in_moveloop;
 int in_impossible;

 int in_paniclog;

 int wizkit_wishing;
} program_state;

extern boolean restoring;

extern const char quitchars[];
extern const char vowels[];
extern const char ynchars[];
extern const char ynqchars[];
extern const char ynaqchars[];
extern const char ynNaqchars[];
extern long yn_number;

extern const char disclosure_options[];

extern int smeq[];
extern int doorindex;
extern char *save_cm;

extern struct kinfo {
    struct kinfo *next;
    int id;
    int format;



    char name[256];
} killer;

extern long done_money;
extern const char *configfile;
extern char lastconfigfile[256];
extern char plname[32];
extern char dogname[];
extern char catname[];
extern char horsename[];
extern char preferred_pet;
extern const char *occtxt;
extern const char *nomovemsg;
extern char lock[];

extern const schar xdir[], ydir[], zdir[];

extern schar tbx, tby;

extern struct multishot { int n, i; short o; boolean s; } m_shot;

extern long moves, monstermoves;
extern long wailmsg;

extern boolean in_mklev;
extern boolean stoned;
extern boolean unweapon;
extern boolean mrg_to_wielded;
extern boolean defer_see_monsters;

extern boolean in_steed_dismounting;

extern const int shield_static[];

# 1 "../include/spell.h" 1
# 15 "../include/spell.h"
struct spell {
    short sp_id;
    xchar sp_lev;
    int sp_know;
};
# 218 "../include/decl.h" 2
extern struct spell spl_book[];

# 1 "../include/color.h" 1
# 54 "../include/color.h"
struct menucoloring {
    struct nhregex *match;
    char *origstr;
    int color, attr;
    struct menucoloring *next;
};
# 221 "../include/decl.h" 2

extern const int zapcolors[];


extern const struct class_sym def_oc_syms[18];
extern uchar oc_syms[18];
extern const struct class_sym def_monsyms[61];
extern uchar monsyms[61];

# 1 "../include/obj.h" 1
# 12 "../include/obj.h"
union vptrs {
     struct obj *v_nexthere;
     struct obj *v_ocontainer;
     struct monst *v_ocarry;
};





struct oextra {
 char *oname;
 struct monst *omonst;
 unsigned *omid;
 long *olong;
 char *omailcmd;
};

struct obj {
 struct obj *nobj;
 union vptrs v;




 struct obj *cobj;
 unsigned o_id;
 xchar ox,oy;
 short otyp;
 unsigned owt;
 long quan;

 schar spe;
# 54 "../include/obj.h"
 char oclass;
 char invlet;
 char oartifact;

 xchar where;
# 68 "../include/obj.h"
 xchar timed;

 unsigned cursed:1;
 unsigned blessed:1;
 unsigned unpaid:1;
 unsigned no_charge:1;
 unsigned known:1;
 unsigned dknown:1;
 unsigned bknown:1;
 unsigned rknown:1;

 unsigned oeroded:2;
 unsigned oeroded2:2;





 unsigned oerodeproof:1;
 unsigned olocked:1;
 unsigned obroken:1;

 unsigned otrapped:1;



 unsigned recharged:3;

 unsigned lamplit:1;
    unsigned globby:1;
 unsigned greased:1;
 unsigned nomerge:1;
 unsigned was_thrown:1;

 unsigned in_use:1;
 unsigned bypass:1;
 unsigned cknown:1;
 unsigned lknown:1;


 int corpsenm;




 int usecount;

 unsigned oeaten;
 long age;
 long owornmask;
 struct oextra *oextra;
};
# 231 "../include/decl.h" 2
extern struct obj *invent,
 *uarm, *uarmc, *uarmh, *uarms, *uarmg, *uarmf,
 *uarmu,
 *uskin, *uamul, *uleft, *uright, *ublindf,
 *uwep, *uswapwep, *uquiver;

extern struct obj *uchain;
extern struct obj *uball;
extern struct obj *migrating_objs;
extern struct obj *billobjs;
extern struct obj *current_wand, *thrownobj, *kickedobj;

extern struct obj zeroobj;
extern anything zeroany;

# 1 "../include/you.h" 1
# 9 "../include/you.h"
# 1 "../include/attrib.h" 1
# 39 "../include/attrib.h"
struct attribs {
 schar a[6];
};
# 10 "../include/you.h" 2
# 1 "../include/monst.h" 1
# 39 "../include/monst.h"
# 1 "../include/mextra.h" 1
# 66 "../include/mextra.h"
struct fakecorridor {
 xchar fx, fy, ftyp;
};

struct egd {
 int fcbeg, fcend;
 int vroom;
 xchar gdx, gdy;
 xchar ogx, ogy;
 d_level gdlevel;
 xchar warncnt;
 unsigned gddone:1;
 unsigned witness:2;
 unsigned unused:5;
 struct fakecorridor fakecorr[(21 +80)];
};




struct epri {
 aligntyp shralign;
 schar shroom;
 coord shrpos;
 d_level shrlevel;
 long intone_time,
      enter_time,
      hostile_time,
      peaceful_time;
};
# 105 "../include/mextra.h"
struct bill_x {
 unsigned bo_id;
 boolean useup;
 long price;
 long bquan;
};

struct eshk {
 long robbed;
 long credit;
 long debit;
 long loan;
 int shoptype;
 schar shoproom;
 schar unused;
 boolean following;
 boolean surcharge;
 boolean dismiss_kops;
 coord shk;
 coord shd;
 d_level shoplevel;
 int billct;
 struct bill_x bill[200];
 struct bill_x *bill_p;
 int visitct;
 char customer[32];
 char shknam[32];
};




struct emin {
 aligntyp min_align;
 boolean renegade;
};
# 155 "../include/mextra.h"
struct edog {
 long droptime;
 unsigned dropdist;
 int apport;
 long whistletime;
 long hungrytime;
 coord ogoal;
 int abuse;
 int revivals;
 int mhpmax_penalty;
 unsigned killed_by_u:1;
};




struct mextra {
 char *mname;
 struct egd *egd;
 struct epri *epri;
 struct eshk *eshk;
 struct emin *emin;
 struct edog *edog;
 int mcorpsenm;
};
# 40 "../include/monst.h" 2


struct monst {
 struct monst *nmon;
 struct permonst *data;
 unsigned m_id;
 short mnum;
 short cham;
 short movement;
 uchar m_lev;
 aligntyp malign;

 xchar mx, my;
 xchar mux, muy;

 coord mtrack[4];
 int mhp, mhpmax;
 unsigned mappearance;
 uchar m_ap_type;






 schar mtame;
 unsigned short mintrinsics;
 int mspec_used;

 unsigned female:1;
 unsigned minvis:1;
 unsigned invis_blkd:1;
 unsigned perminvis:1;
 unsigned mcan:1;
 unsigned mburied:1;
 unsigned mundetected:1;




 unsigned mcansee:1;

 unsigned mspeed:2;
 unsigned permspeed:2;
 unsigned mrevived:1;
 unsigned mcloned:1;
 unsigned mavenge:1;
 unsigned mflee:1;

 unsigned mfleetim:7;
 unsigned msleeping:1;

 unsigned mblinded:7;
 unsigned mstun:1;

 unsigned mfrozen:7;
 unsigned mcanmove:1;

 unsigned mconf:1;
 unsigned mpeaceful:1;
 unsigned mtrapped:1;
 unsigned mleashed:1;
 unsigned isshk:1;
 unsigned isminion:1;
 unsigned isgd:1;
 unsigned ispriest:1;

 unsigned iswiz:1;
 unsigned wormno:5;




 unsigned long mstrategy;
# 135 "../include/monst.h"
 long mtrapseen;
 long mlstmv;
 long mspare1;
 struct obj *minvent;

 struct obj *mw;
 long misc_worn_check;
 xchar weapon_check;

 int meating;
 struct mextra *mextra;
};
# 11 "../include/you.h" 2



# 1 "../include/skills.h" 1
# 99 "../include/skills.h"
struct skills {
 xchar skill;
 xchar max_skill;
 unsigned short advance;
};
# 113 "../include/skills.h"
struct def_skill {
 xchar skill;
 xchar skmax;
};
# 15 "../include/you.h" 2



struct RoleName {
 const char *m;
 const char *f;
};

struct RoleAdvance {

 xchar infix, inrnd;
 xchar lofix, lornd;
 xchar hifix, hirnd;
};

struct u_have {
 unsigned amulet:1;
 unsigned bell:1;
 unsigned book:1;
 unsigned menorah:1;
 unsigned questart:1;
 unsigned unused:3;
};

struct u_event {
 unsigned minor_oracle:1;
 unsigned major_oracle:1;
 unsigned read_tribute:1;
 unsigned qcalled:1;
 unsigned qexpelled:1;
 unsigned qcompleted:1;
 unsigned uheard_tune:2;

 unsigned uopened_dbridge:1;
 unsigned invoked:1;
 unsigned gehennom_entered:1;
 unsigned uhand_of_elbereth:2;
 unsigned udemigod:1;
 unsigned uvibrated:1;
 unsigned ascended:1;
};

struct u_achieve {
 unsigned amulet:1;
 unsigned bell:1;
 unsigned book:1;
 unsigned menorah:1;
 unsigned enter_gehennom:1;
 unsigned ascended:1;
 unsigned mines_luckstone:1;
 unsigned finish_sokoban:1;

 unsigned killed_medusa:1;
};

struct u_realtime {
 long realtime;
 time_t restored;
 time_t endtime;
};






struct u_conduct {
 long unvegetarian;
 long unvegan;
 long food;
 long gnostic;
 long weaphit;
 long killer;
 long literate;
 long polypiles;
 long polyselfs;
 long wishes;
 long wisharti;

};

struct u_roleplay {
    boolean blind;
    boolean nudist;
    long numbones;
};


struct Role {

 struct RoleName name;
 struct RoleName rank[9];
 const char *lgod, *ngod, *cgod;
 const char *filecode;
 const char *homebase;
 const char *intermed;


 short malenum,
       femalenum,
       petnum,
       ldrnum,
       guardnum,
       neminum,
       enemy1num,
       enemy2num;
 char enemy1sym,
       enemy2sym;
 short questarti;


 short allow;
# 138 "../include/you.h"
 xchar attrbase[6];
 xchar attrdist[6];
 struct RoleAdvance hpadv;
 struct RoleAdvance enadv;
 xchar xlev;
 xchar initrecord;


 int spelbase;
 int spelheal;
 int spelshld;
 int spelarmr;
 int spelstat;
 int spelspec;
 int spelsbon;
# 165 "../include/you.h"
};

extern const struct Role roles[];
extern struct Role urole;
# 179 "../include/you.h"
struct Race {

 const char *noun;
 const char *adj;
 const char *coll;
 const char *filecode;
 struct RoleName individual;


 short malenum,
       femalenum,
       mummynum,
       zombienum;


 short allow;
 short selfmask,
       lovemask,
       hatemask;


 xchar attrmin[6];
 xchar attrmax[6];
 struct RoleAdvance hpadv;
 struct RoleAdvance enadv;
# 217 "../include/you.h"
};

extern const struct Race races[];
extern struct Race urace;




struct Gender {
 const char *adj;
 const char *he;
 const char *him;
 const char *his;
 const char *filecode;
 short allow;
};



extern const struct Gender genders[];
# 246 "../include/you.h"
struct Align {
 const char *noun;
 const char *adj;
 const char *filecode;
 short allow;
 aligntyp value;
};


extern const struct Align aligns[];



struct you {
 xchar ux, uy;
 schar dx, dy, dz;
 schar di;
 xchar tx, ty;
 xchar ux0, uy0;
 d_level uz, uz0;
 d_level utolev;
 uchar utotype;
 boolean umoved;
 int last_str_turn;

 int ulevel;
 int ulevelmax;
 unsigned utrap;
 unsigned utraptype;






 char urooms[5];
 char urooms0[5];
 char uentered[5];
 char ushops[5];
 char ushops0[5];
 char ushops_entered[5];
 char ushops_left[5];

 int uhunger;
 unsigned uhs;

 struct prop uprops[(67)+1];

 unsigned umconf;
 unsigned usick_type:2;





 int nv_range;
 int xray_range;






 int bglyph;
 int cglyph;
 int bc_order;
 int bc_felt;

 int umonster;
 int umonnum;

 int mh, mhmax, mtimedone;
 struct attribs macurr,
   mamax;
 int ulycn;

 unsigned ucreamed;
 unsigned uswldtim;

 unsigned uswallow:1;
 unsigned uinwater:1;

 unsigned uundetected:1;
 unsigned mfemale:1;
 unsigned uinvulnerable:1;
 unsigned uburied:1;
 unsigned uedibility:1;


 unsigned udg_cnt;
 struct u_achieve uachieve;
 struct u_event uevent;
 struct u_have uhave;
 struct u_conduct uconduct;
 struct u_roleplay uroleplay;
 struct attribs acurr,
   aexe,
   abon,
   amax,
   atemp,
   atime;
 align ualign;



 aligntyp ualignbase[2];
 schar uluck, moreluck;




 schar uhitinc;
 schar udaminc;
 schar uac;
 uchar uspellprot;
 uchar usptime;
 uchar uspmtime;
 int uhp,uhpmax;
 int uen, uenmax;
 xchar uhpinc[30], ueninc[30];
 int ugangr;
 int ugifts;
 int ublessed, ublesscnt;
 long umoney0;
 long uspare1;
 long uexp, urexp;
 long ucleansed;
 long usleep;
 int uinvault;
 struct monst *ustuck;
 struct monst *usteed;
 long ugallop;
 int urideturns;
 int umortality;
 int ugrave_arise;
 int weapon_slots;
 int skills_advanced;
 xchar skill_record[60];
 struct skills weapon_skills[(38 +1)];
 boolean twoweap;

};
# 247 "../include/decl.h" 2
extern struct you u;
extern time_t ubirthday;
extern struct u_realtime urealtime;

# 1 "../include/onames.h" 1
# 252 "../include/decl.h" 2




extern struct monst youmonst;
extern struct monst *mydogs, *migrating_mons;

extern struct mvitals {
 uchar born;
 uchar died;
 uchar mvflags;
} mvitals[381];

extern struct c_color_names {
    const char *const c_black, *const c_amber, *const c_golden,
  *const c_light_blue,*const c_red, *const c_green,
  *const c_silver, *const c_blue, *const c_purple,
  *const c_white, *const c_orange;
} c_color_names;
# 284 "../include/decl.h"
extern const char *c_obj_colors[];

extern struct c_common_strings {
    const char *const c_nothing_happens, *const c_thats_enough_tries,
  *const c_silly_thing_to, *const c_shudder_for_moment,
  *const c_something, *const c_Something,
  *const c_You_can_move_again,
  *const c_Never_mind, *c_vision_clears,
  *const c_the_your[2];
} c_common_strings;
# 306 "../include/decl.h"
extern const char *materialnm[];
# 322 "../include/decl.h"
extern boolean vision_full_recalc;
extern char **viz_array;


extern winid WIN_MESSAGE;



extern winid WIN_MAP, WIN_INVEN;
# 341 "../include/decl.h"
extern char toplines[];

extern struct tc_gbl_data {
    char *tc_AS, *tc_AE;
    int tc_LI, tc_CO;
} tc_gbl_data;







extern const char * const monexplain[], invisexplain[], * const oclass_names[];
# 384 "../include/decl.h"
extern char *fqn_prefix[10];




extern struct savefile_info sfcap, sfrestinfo, sfsaveinfo;

struct autopickup_exception {
 char *pattern;
 boolean grab;
 struct autopickup_exception *next;
};


extern char *ARGV0;
# 129 "../include/hack.h" 2
# 1 "../include/timeout.h" 1
# 11 "../include/timeout.h"
typedef void (*timeout_proc) (union any *, long);
# 37 "../include/timeout.h"
typedef struct fe {
    struct fe *next;
    long timeout;
    unsigned long tid;
    short kind;
    short func_index;
    anything arg;
    unsigned needs_fixup:1;
} timer_element;
# 130 "../include/hack.h" 2

 extern coord bhitpos;
# 155 "../include/hack.h"
# 1 "../include/trap.h" 1
# 12 "../include/trap.h"
union vlaunchinfo {
 short v_launch_otyp;
 coord v_launch2;
 uchar v_conjoined;
 short v_tnote;
};

struct trap {
 struct trap *ntrap;
 xchar tx,ty;
 d_level dst;
 coord launch;
 unsigned ttyp:5;
 unsigned tseen:1;
 unsigned once:1;
 unsigned madeby_u:1;






 union vlaunchinfo vl;




};

extern struct trap *ftrap;
# 156 "../include/hack.h" 2
# 1 "../include/flag.h" 1
# 18 "../include/flag.h"
struct flag {
 boolean acoustics;
 boolean autodig;
 boolean autoquiver;
 boolean autoopen;
 boolean beginner;
 boolean biff;
 boolean bones;
 boolean confirm;
 boolean dark_room;
 boolean debug;

 boolean end_own;
 boolean explore;

 boolean female;
  boolean friday13;
 boolean help;
 boolean ignintr;
 boolean ins_chkpt;
 boolean invlet_constant;
 boolean legacy;
 boolean lit_corridor;
 boolean nap;
 boolean null;
 boolean perm_invent;
 boolean pickup;
 boolean pickup_thrown;
 boolean pushweapon;
 boolean rest_on_space;
 boolean safe_dog;
 boolean showexp;
 boolean showscore;
 boolean silent;
 boolean sortloot;
 boolean sortpack;
 boolean sparkle;
 boolean standout;
 boolean time;
 boolean tombstone;
 boolean verbose;
 int end_top, end_around;
 unsigned moonphase;
 unsigned long suppress_alert;


 int paranoia_bits;







 int pickup_burden;
 int pile_limit;
 char inv_order[18];
 char pickup_types[18];





 char end_disclose[6 + 1];

 char menu_style;
 boolean made_fruit;
# 112 "../include/flag.h"
 int initrole;
 int initrace;
 int initgend;
 int initalign;
 int randomall;
 int pantheon;

 boolean lootabc;
 boolean showrace;
 boolean travelcmd;
 int runmode;
};
# 167 "../include/flag.h"
struct instance_flags {





 int in_lava_effects;
 int last_msg;
 int purge_monsters;
 int override_ID;
 int suppress_price;
 coord travelcc;
 boolean window_inited;
 boolean vision_inited;
 boolean sanity_check;
 boolean mon_polycontrol;

 unsigned msg_history;
 int menu_headings;
 int *opt_booldup;
 int *opt_compdup;

 boolean altmeta;

 boolean cbreak;
 boolean deferred_X;
 boolean num_pad;
 boolean news;
 boolean mention_walls;
 boolean menu_tab_sep;
 boolean menu_head_objsym;
 boolean menu_requested;

 boolean renameallowed;
 boolean renameinprogress;
 boolean toptenwin;
 boolean zerocomp;
 boolean rlecomp;
 uchar num_pad_mode;
 boolean echo;
 boolean use_menu_color;
# 216 "../include/flag.h"
 uchar bouldersym;

 char prevmsg_window;
 boolean extmenu;
# 257 "../include/flag.h"
 boolean wc_color;
 boolean wc_hilite_pet;
 boolean wc_ascii_map;
 boolean wc_tiled_map;
 boolean wc_preload_tiles;
 int wc_tile_width;
 int wc_tile_height;
 char *wc_tile_file;
 boolean wc_inverse;
 int wc_align_status;
 int wc_align_message;
 int wc_vary_msgcount;
 char *wc_foregrnd_menu;
 char *wc_backgrnd_menu;
 char *wc_foregrnd_message;
 char *wc_backgrnd_message;
 char *wc_foregrnd_status;
 char *wc_backgrnd_status;
 char *wc_foregrnd_text;
 char *wc_backgrnd_text;
 char *wc_font_map;
 char *wc_font_message;
 char *wc_font_status;
 char *wc_font_menu;
 char *wc_font_text;
 int wc_fontsiz_map;
 int wc_fontsiz_message;
 int wc_fontsiz_status;
 int wc_fontsiz_menu;
 int wc_fontsiz_text;
 int wc_scroll_amount;
 int wc_scroll_margin;

 int wc_map_mode;

 int wc_player_selection;
 boolean wc_splash_screen;
 boolean wc_popup_dialog;

 boolean wc_eight_bit_input;
 boolean wc_mouse_support;
 boolean wc2_fullscreen;
 boolean wc2_softkeyboard;
 boolean wc2_wraptext;
 boolean wc2_selectsaved;
 boolean wc2_darkgray;
 boolean cmdassist;
 boolean clicklook;
 boolean obsolete;
 struct autopickup_exception *autopickup_exceptions[2];
# 316 "../include/flag.h"
 unsigned save_uinwater:1;
 unsigned save_uburied:1;
};
# 339 "../include/flag.h"
extern struct flag flags;



extern struct instance_flags iflags;
# 381 "../include/flag.h"
struct func_tab;
# 391 "../include/flag.h"
struct cmd {
    unsigned serialno;
    boolean num_pad;
    boolean pcHack_compat;
    boolean phone_layout;
    boolean swap_yz;
    char move_W, move_NW, move_N, move_NE,
  move_E, move_SE, move_S, move_SW;
    const char *dirchars;
    const char *alphadirchars;
    const struct func_tab *commands[256];
};

extern struct cmd Cmd;
# 157 "../include/hack.h" 2
# 1 "../include/rm.h" 1
# 227 "../include/rm.h"
struct symdef {
    uchar sym;
    const char *explanation;

    uchar color;

};

struct symparse {
 unsigned range;





 int idx;
 const char *name;
};







struct symsetentry {
 struct symsetentry *next;
 char *name;
 char *desc;
 int idx;
 int handling;
 unsigned nocolor:1;
 unsigned primary:1;
 unsigned rogue:1;

};
# 281 "../include/rm.h"
extern const struct symdef defsyms[95];
extern const struct symdef def_warnsyms[6];
extern int currentgraphics;
extern nhsym showsyms[];

extern struct symsetentry symset[2];
# 388 "../include/rm.h"
struct rm {
 int glyph;
 schar typ;
 uchar seenv;
 unsigned flags:5;
 unsigned horizontal:1;
 unsigned lit:1;
 unsigned waslit:1;

 unsigned roomno:6;
 unsigned edge:1;
 unsigned candig:1;
};
# 507 "../include/rm.h"
struct damage {
 struct damage *next;
 long when, cost;
 coord place;
 schar typ;
};



struct cemetery {
 struct cemetery *next;

 char who[32 + 4*(1+3) + 1];

 char how[100 + 1];

 char when[4+2+2 + 2+2+2 + 1];

 schar frpx, frpy;
 boolean bonesknown;
};

struct levelflags {
 uchar nfountains;
 uchar nsinks;

 unsigned has_shop:1;
 unsigned has_vault:1;
 unsigned has_zoo:1;
 unsigned has_court:1;
 unsigned has_morgue:1;
 unsigned has_beehive:1;
 unsigned has_barracks:1;
 unsigned has_temple:1;

 unsigned has_swamp:1;
 unsigned noteleport:1;
 unsigned hardfloor:1;
 unsigned nommap:1;
 unsigned hero_memory:1;
 unsigned shortsighted:1;
 unsigned graveyard:1;
 unsigned sokoban_rules:1;

 unsigned is_maze_lev:1;
 unsigned is_cavernous_lev:1;
 unsigned arboreal:1;
 unsigned wizard_bones:1;


        unsigned corrmaze:1;

};

typedef struct
{
    struct rm locations[80][21];

    struct obj *objects[80][21];
    struct monst *monsters[80][21];






    struct obj *objlist;
    struct obj *buriedobjlist;
    struct monst *monlist;
    struct damage *damagelist;
    struct cemetery *bonesinfo;
    struct levelflags flags;
}
dlevel_t;

extern schar lastseentyp[80][21];

extern dlevel_t level;
# 158 "../include/hack.h" 2
# 1 "../include/vision.h" 1
# 159 "../include/hack.h" 2
# 1 "../include/display.h" 1
# 160 "../include/hack.h" 2
# 1 "../include/engrave.h" 1
# 10 "../include/engrave.h"
struct engr {
 struct engr *nxt_engr;
 char *engr_txt;
 xchar engr_x, engr_y;
 unsigned engr_lth;
 long engr_time;
 xchar engr_type;







};
# 161 "../include/hack.h" 2
# 1 "../include/rect.h" 1
# 10 "../include/rect.h"
typedef struct nhrect {
 xchar lx, ly;
 xchar hx, hy;
} NhRect;
# 162 "../include/hack.h" 2
# 1 "../include/region.h" 1
# 12 "../include/region.h"
typedef boolean (*callback_proc) (genericptr_t, genericptr_t);
# 34 "../include/region.h"
typedef struct {
  NhRect bounding_box;
  NhRect *rects;
  short nrects;
  boolean attach_2_u;
  unsigned int attach_2_m;

  const char* enter_msg;
  const char* leave_msg;
  long ttl;
  short expire_f;
  short can_enter_f;

  short enter_f;
  short can_leave_f;

  short leave_f;
  short inside_f;

  unsigned int player_flags;
  unsigned int* monsters;
  short n_monst;
  short max_monst;





  boolean visible;
  int glyph;
  anything arg;

} NhRegion;
# 163 "../include/hack.h" 2
# 177 "../include/hack.h"
# 1 "../include/extern.h" 1
# 15 "../include/extern.h"
extern char *fmt_ptr (const void *);







extern void moveloop (int);
extern void stop_occupation(void);
extern void display_gamewindows(void);
extern void newgame(void);
extern void welcome (int);
extern time_t get_realtime(void);



extern int doapply(void);
extern int dorub(void);
extern int dojump(void);
extern int jump (int);
extern int number_leashed(void);
extern void o_unleash (struct obj *);
extern void m_unleash (struct monst *,int);
extern void unleash_all(void);
extern boolean next_to_u(void);
extern struct obj *get_mleash (struct monst *);
extern const char *beautiful(void);
extern void check_leash (int,int);
extern boolean um_dist (int,int,int);
extern boolean snuff_candle (struct obj *);
extern boolean snuff_lit (struct obj *);
extern boolean catch_lit (struct obj *);
extern void use_unicorn_horn (struct obj *);
extern boolean tinnable (struct obj *);
extern void reset_trapset(void);
extern void fig_transform (union any *, long);
extern int unfixable_trouble_count (int);



extern void init_artifacts(void);
extern void save_artifacts (int);
extern void restore_artifacts (int);
extern const char *artiname (int);
extern struct obj *mk_artifact (struct obj *,int);
extern const char *artifact_name (const char *,short *);
extern boolean exist_artifact (int,const char *);
extern void artifact_exists (struct obj *,const char *,int);
extern int nartifact_exist(void);
extern boolean arti_immune (struct obj *,int);
extern boolean spec_ability (struct obj *,unsigned long);
extern boolean confers_luck (struct obj *);
extern boolean arti_reflects (struct obj *);
extern boolean shade_glare (struct obj *);
extern boolean restrict_name (struct obj *,const char *);
extern boolean defends (int,struct obj *);
extern boolean defends_when_carried (int,struct obj *);
extern boolean protects (struct obj *,int);
extern void set_artifact_intrinsic (struct obj *,int,long);
extern int touch_artifact (struct obj *,struct monst *);
extern int spec_abon (struct obj *,struct monst *);
extern int spec_dbon (struct obj *,struct monst *,int);
extern void discover_artifact (int);
extern boolean undiscovered_artifact (int);
extern int disp_artifact_discoveries (winid);
extern boolean artifact_hit (struct monst *,struct monst *, struct obj *,int *,int);

extern int doinvoke(void);
extern boolean finesse_ahriman (struct obj *);
extern void arti_speak (struct obj *);
extern boolean artifact_light (struct obj *);
extern long spec_m2 (struct obj *);
extern boolean artifact_has_invprop (struct obj *,int);
extern long arti_cost (struct obj *);
extern struct obj *what_gives (long *);
extern void Sting_effects (int);
extern int retouch_object (struct obj **,int);
extern void retouch_equipment (int);



extern boolean adjattrib (int,int,int);
extern void gainstr (struct obj *,int,int);
extern void losestr (int);
extern void poisontell (int,int);
extern void poisoned (const char *,int,const char *,int,int);
extern void change_luck (int);
extern int stone_luck (int);
extern void set_moreluck(void);
extern void restore_attrib(void);
extern void exercise (int,int);
extern void exerchk(void);
extern void init_attr (int);
extern void redist_attr(void);
extern void adjabil (int,int);
extern int newhp(void);
extern schar acurr (int);
extern schar acurrstr(void);
extern boolean extremeattr (int);
extern void adjalign (int);
extern int is_innate (int);
extern char *from_what (int);
extern void uchangealign (int,int);



extern void ballrelease (int);
extern void ballfall(void);
extern void placebc(void);
extern void unplacebc(void);
extern void set_bc (int);
extern void move_bc (int,int,int,int,int,int);
extern boolean drag_ball (int,int, int *,xchar *,xchar *,xchar *,xchar *, boolean *,int);

extern void drop_ball (int,int);
extern void drag_down(void);



extern void sanitize_name (char *);
extern void drop_upon_death (struct monst *,struct obj *,int,int);
extern boolean can_make_bones(void);
extern void savebones (int,time_t,struct obj *);
extern int getbones(void);



extern int xlev_to_rank (int);
extern int title_to_mon (const char *,int *,int *);
extern void max_rank_sz(void);



extern int describe_level (char *);
extern const char *rank_of (int,int,int);
extern void bot(void);

extern void status_initialize (int);
extern void status_finish(void);
extern void genl_status_init(void);
extern void genl_status_finish(void);
extern void genl_status_update (int, genericptr_t, int, int);
extern void genl_status_enablefield (int, const char *, const char *,int);

extern void genl_status_threshold (int,int,anything,int,int,int);
extern boolean set_status_hilites (char *op);
extern void clear_status_hilites(void);
extern char *get_status_hilites (char *, int);
extern boolean status_hilite_menu(void);





extern boolean redraw_cmd (int);
# 185 "../include/extern.h"
extern void reset_occupations(void);
extern void set_occupation (int (*)(void),const char *,int);
extern char pgetchar(void);
extern void pushch (int);
extern void savech (int);
extern void add_debug_extended_commands(void);
extern void reset_commands (int);
extern void rhack (char *);
extern int doextlist(void);
extern int extcmd_via_menu(void);
extern int enter_explore_mode(void);
extern void enlightenment (int,int);
extern void youhiding (int,int);
extern void show_conduct (int);
extern int xytod (int,int);
extern void dtoxy (coord *,int);
extern int movecmd (int);
extern int dxdy_moveok(void);
extern int getdir (const char *);
extern void confdir(void);
extern const char *directionname (int);
extern int isok (int,int);
extern int get_adjacent_loc (const char *, const char *, int, int, coord *);
extern const char *click_to_cmd (int,int,int);

extern void hangup (int);
extern void end_of_input(void);

extern char readchar(void);
extern void sanity_check(void);
extern char yn_function (const char *, const char *, int);
extern boolean paranoid_query (int,const char *);



extern boolean is_pool (int,int);
extern boolean is_lava (int,int);
extern boolean is_pool_or_lava (int,int);
extern boolean is_ice (int,int);
extern boolean is_moat (int,int);
extern schar db_under_typ (int);
extern int is_drawbridge_wall (int,int);
extern boolean is_db_wall (int,int);
extern boolean find_drawbridge (int *,int*);
extern boolean create_drawbridge (int,int,int,int);
extern void open_drawbridge (int,int);
extern void close_drawbridge (int,int);
extern void destroy_drawbridge (int,int);



extern void decl_init(void);



extern struct obj *o_in (struct obj*,int);
extern struct obj *o_material (struct obj*,unsigned);
extern int gold_detect (struct obj *);
extern int food_detect (struct obj *);
extern int object_detect (struct obj *,int);
extern int monster_detect (struct obj *,int);
extern int trap_detect (struct obj *);
extern const char *level_distance (d_level *);
extern void use_crystal_ball (struct obj *);
extern void do_mapping(void);
extern void do_vicinity_map(void);
extern void cvt_sdoor_to_door (struct rm *);




extern int findit(void);
extern int openit(void);
extern boolean detecting (void (*)(int,int,void *));
extern void find_trap (struct trap *);
extern int dosearch0 (int);
extern int dosearch(void);
extern void sokoban_detect(void);
extern void reveal_terrain (int);



extern int dig_typ (struct obj *,int,int);
extern boolean is_digging(void);



extern int holetime(void);
extern boolean dig_check (struct monst *, int, int, int);
extern void digactualhole (int,int,struct monst *,int);
extern boolean dighole (int,int,coord *);
extern int use_pick_axe (struct obj *);
extern int use_pick_axe2 (struct obj *);
extern boolean mdig_tunnel (struct monst *);
extern void watch_dig (struct monst *,int,int,int);
extern void zap_dig(void);
extern struct obj *bury_an_obj (struct obj *, boolean *);
extern void bury_objs (int,int);
extern void unearth_objs (int,int);
extern void rot_organic (union any *, long);
extern void rot_corpse (union any *, long);
extern struct obj *buried_ball (coord *);
extern void buried_ball_to_punishment(void);
extern void buried_ball_to_freedom(void);
extern schar fillholetyp (int,int,int);
extern void liquid_flow (int,int,int,struct trap *, const char *);
extern boolean conjoined_pits (struct trap *,struct trap *,int);
# 302 "../include/extern.h"
extern void magic_map_background (int,int,int);
extern void map_background (int,int,int);
extern void map_trap (struct trap *,int);
extern void map_object (struct obj *,int);
extern void map_invisible (int,int);
extern void unmap_object (int,int);
extern void map_location (int,int,int);
extern void feel_location (int,int);
extern void newsym (int,int);
extern void shieldeff (int,int);
extern void tmp_at (int,int);
extern void swallowed (int);
extern void under_ground (int);
extern void under_water (int);
extern void see_monsters(void);
extern void set_mimic_blocking(void);
extern void see_objects(void);
extern void see_traps(void);
extern void curs_on_u(void);
extern int doredraw(void);
extern void docrt(void);
extern void show_glyph (int,int,int);
extern void clear_glyph_buffer(void);
extern void row_refresh (int,int,int);
extern void cls(void);
extern void flush_screen (int);
extern int back_to_glyph (int,int);
extern int zapdir_to_glyph (int,int,int);
extern int glyph_at (int,int);
extern void set_wall_state(void);
extern void unset_seenv (struct rm *,int,int,int,int);







extern int dodrop(void);
extern boolean boulder_hits_pool (struct obj *,int,int,int);
extern boolean flooreffects (struct obj *,int,int,const char *);
extern void doaltarobj (struct obj *);
extern boolean canletgo (struct obj *,const char *);
extern void dropx (struct obj *);
extern void dropy (struct obj *);
extern void dropz (struct obj *,int);
extern void obj_no_longer_held (struct obj *);
extern int doddrop(void);
extern int dodown(void);
extern int doup(void);

extern void save_currentstate(void);

extern void goto_level (d_level *,int,int,int);
extern void schedule_goto (d_level *,int,int,int, const char *,const char *);

extern void deferred_goto(void);
extern boolean revive_corpse (struct obj *);
extern void revive_mon (union any *, long);
extern int donull(void);
extern int dowipe(void);
extern void set_wounded_legs (long,int);
extern void heal_legs(void);



extern int getpos (coord *,int,const char *);
extern void getpos_sethilite (void (*f)(int) );
extern void new_mname (struct monst *,int);
extern void free_mname (struct monst *);
extern void new_oname (struct obj *,int);
extern void free_oname (struct obj *);
extern const char *safe_oname (struct obj *);
extern struct monst *christen_monst (struct monst *,const char *);
extern struct obj *oname (struct obj *,const char *);
extern boolean objtyp_is_callable (int);
extern int docallcmd(void);
extern void docall (struct obj *);
extern const char *rndghostname(void);
extern char *x_monnam (struct monst *,int,const char *,int,int);
extern char *l_monnam (struct monst *);
extern char *mon_nam (struct monst *);
extern char *noit_mon_nam (struct monst *);
extern char *Monnam (struct monst *);
extern char *noit_Monnam (struct monst *);
extern char *m_monnam (struct monst *);
extern char *y_monnam (struct monst *);
extern char *Adjmonnam (struct monst *,const char *);
extern char *Amonnam (struct monst *);
extern char *a_monnam (struct monst *);
extern char *distant_monnam (struct monst *,int,char *);
extern char *rndmonnam (char *);
extern const char *hcolor (const char *);
extern const char *rndcolor(void);
extern const char *roguename(void);
extern struct obj *realloc_obj (struct obj *, int, genericptr_t, int, const char *);

extern char *coyotename (struct monst *,char *);
extern const char *noveltitle (int *);
extern const char *lookup_novel (const char *, int *);
# 413 "../include/extern.h"
extern void off_msg (struct obj *);
extern void set_wear (struct obj *);
extern boolean donning (struct obj *);
extern boolean doffing (struct obj *);
extern void cancel_don(void);
extern int stop_donning (struct obj *);
extern int Armor_off(void);
extern int Armor_gone(void);
extern int Helmet_off(void);
extern int Gloves_off(void);
extern int Boots_off(void);
extern int Cloak_off(void);
extern int Shield_off(void);
extern int Shirt_off(void);
extern void Amulet_off(void);
extern void Ring_on (struct obj *);
extern void Ring_off (struct obj *);
extern void Ring_gone (struct obj *);
extern void Blindf_on (struct obj *);
extern void Blindf_off (struct obj *);
extern int dotakeoff(void);
extern int doremring(void);
extern int cursed (struct obj *);
extern int armoroff (struct obj *);
extern int canwearobj (struct obj *, long *, int);
extern int dowear(void);
extern int doputon(void);
extern void find_ac(void);
extern void glibr(void);
extern struct obj *some_armor (struct monst *);
extern struct obj *stuck_ring (struct obj *,int);
extern struct obj *unchanger(void);
extern void reset_remarm(void);
extern int doddoremarm(void);
extern int destroy_arm (struct obj *);
extern void adj_abon (struct obj *,int);
extern boolean inaccessible_equipment (struct obj *,const char *,int);



extern void newedog (struct monst *);
extern void free_edog (struct monst *);
extern void initedog (struct monst *);
extern struct monst *make_familiar (struct obj *,int,int,int);
extern struct monst *makedog(void);
extern void update_mlstmv(void);
extern void losedogs(void);
extern void mon_arrive (struct monst *,int);
extern void mon_catchup_elapsed_time (struct monst *,long);
extern void keepdogs (int);
extern void migrate_to_level (struct monst *,int,int,coord *);
extern int dogfood (struct monst *,struct obj *);
extern boolean tamedog (struct monst *,struct obj *);
extern void abuse_dog (struct monst *);
extern void wary_dog (struct monst *, int);



extern struct obj *droppables (struct monst *);
extern int dog_nutrition (struct monst *,struct obj *);
extern int dog_eat (struct monst *,struct obj *,int,int,int);
extern int dog_move (struct monst *,int);



extern void finish_meating (struct monst *);



extern boolean ghitm (struct monst *,struct obj *);
extern void container_impact_dmg (struct obj *,int,int);
extern int dokick(void);
extern boolean ship_object (struct obj *,int,int,int);
extern void obj_delivery (int);
extern schar down_gate (int,int);
extern void impact_drop (struct obj *,int,int,int);



extern int dothrow(void);
extern int dofire(void);
extern void endmultishot (int);
extern void hitfloor (struct obj *);
extern void hurtle (int,int,int,int);
extern void mhurtle (struct monst *,int,int,int);
extern void throwit (struct obj *,long,int);
extern int omon_adj (struct monst *,struct obj *,int);
extern int thitmonst (struct monst *,struct obj *);
extern int hero_breaks (struct obj *,int,int,int);
extern int breaks (struct obj *,int,int);
extern void release_camera_demon (struct obj *, int,int);
extern void breakobj (struct obj *,int,int,int,int);
extern boolean breaktest (struct obj *);
extern boolean walk_path (coord *, coord *, boolean (*)(genericptr_t,int,int), genericptr_t);
extern boolean hurtle_step (genericptr_t, int, int);



extern int def_char_to_objclass (int);
extern int def_char_to_monclass (int);

extern void switch_symbols (int);
extern void assign_graphics (int);
extern void init_r_symbols(void);
extern void init_symbols(void);
extern void update_bouldersym(void);
extern void init_showsyms(void);
extern void init_l_symbols(void);
extern void clear_symsetentry (int,int);
extern void update_l_symset (struct symparse *,int);
extern void update_r_symset (struct symparse *,int);
extern boolean cursed_object_at (int, int);



extern void save_dungeon (int,int,int);
extern void restore_dungeon (int);
extern void insert_branch (branch *,int);
extern void init_dungeons(void);
extern s_level *find_level (const char *);
extern s_level *Is_special (d_level *);
extern branch *Is_branchlev (d_level *);
extern xchar ledger_no (d_level *);
extern xchar maxledgerno(void);
extern schar depth (d_level *);
extern xchar dunlev (d_level *);
extern xchar dunlevs_in_dungeon (d_level *);
extern xchar ledger_to_dnum (int);
extern xchar ledger_to_dlev (int);
extern xchar deepest_lev_reached (int);
extern boolean on_level (d_level *,d_level *);
extern void next_level (int);
extern void prev_level (int);
extern void u_on_newpos (int,int);
extern void u_on_rndspot (int);
extern void u_on_sstairs (int);
extern void u_on_upstairs(void);
extern void u_on_dnstairs(void);
extern boolean On_stairs (int,int);
extern void get_level (d_level *,int);
extern boolean Is_botlevel (d_level *);
extern boolean Can_fall_thru (d_level *);
extern boolean Can_dig_down (d_level *);
extern boolean Can_rise_up (int,int,d_level *);
extern boolean has_ceiling (d_level *);
extern boolean In_quest (d_level *);
extern boolean In_mines (d_level *);
extern branch *dungeon_branch (const char *);
extern boolean at_dgn_entrance (const char *);
extern boolean In_hell (d_level *);
extern boolean In_V_tower (d_level *);
extern boolean On_W_tower_level (d_level *);
extern boolean In_W_tower (int,int,d_level *);
extern void find_hell (d_level *);
extern void goto_hell (int,int);
extern void assign_level (d_level *,d_level *);
extern void assign_rnd_level (d_level *,d_level *,int);
extern int induced_align (int);
extern boolean Invocation_lev (d_level *);
extern xchar level_difficulty(void);
extern schar lev_by_name (const char *);
extern schar print_dungeon (int,schar *,xchar *);
extern char *get_annotation (d_level *);
extern int donamelevel(void);
extern int dooverview(void);
extern void show_overview (int,int);
extern void forget_mapseen (int);
extern void init_mapseen (d_level *);
extern void recalc_mapseen(void);
extern void mapseen_temple (struct monst *);
extern void room_discovered (int);
extern void recbranch_mapseen (d_level *, d_level *);
extern void remdun_mapseen (int);
# 595 "../include/extern.h"
extern void eatmupdate(void);
extern boolean is_edible (struct obj *);
extern void init_uhunger(void);
extern int Hear_again(void);
extern void reset_eat(void);
extern int doeat(void);
extern void gethungry(void);
extern void morehungry (int);
extern void lesshungry (int);
extern boolean is_fainted(void);
extern void reset_faint(void);
extern void violated_vegetarian(void);
extern void newuhs (int);
extern struct obj *floorfood (const char *,int);
extern void vomit(void);
extern int eaten_stat (int,struct obj *);
extern void food_disappears (struct obj *);
extern void food_substitution (struct obj *,struct obj *);
extern void eating_conducts (struct permonst *);
extern int eat_brains (struct monst *,struct monst *,int,int *);
extern void fix_petrification(void);
extern void consume_oeaten (struct obj *,int);
extern boolean maybe_finished_meal (int);
extern void set_tin_variety (struct obj *,int);
extern int tin_variety_txt (char *,int *);
extern void tin_details (struct obj *,int,char *);
extern boolean Popeye (int);



extern void done1 (int);
extern int done2(void);



extern void done_in_by (struct monst *,int);

extern void panic (const char *,...) __attribute__ ((format (printf, 1, 2)));

extern void done (int);
extern void container_contents (struct obj *,int,int,int);
extern void terminate (int);
extern int dovanquished(void);
extern int num_genocides(void);
extern void delayed_killer (int, int, const char*);
extern struct kinfo *find_delayed_killer (int);
extern void dealloc_killer (struct kinfo*);
extern void save_killers (int,int);
extern void restore_killers (int);
extern char *build_english_list (char *);

extern void panictrace_setsignals (int);




extern char *random_engraving (char *);
extern void wipeout_text (char *,int,unsigned);
extern boolean can_reach_floor (int);
extern void cant_reach_floor (int,int,int,int);
extern const char *surface (int,int);
extern const char *ceiling (int,int);
extern struct engr *engr_at (int,int);
extern int sengr_at (const char *,int,int,int);
extern void u_wipe_engr (int);
extern void wipe_engr_at (int,int,int,int);
extern void read_engr_at (int,int);
extern void make_engr_at (int,int,const char *,long,int);
extern void del_engr_at (int,int);
extern int freehand(void);
extern int doengrave(void);
extern void sanitize_engravings(void);
extern void save_engravings (int,int);
extern void rest_engravings (int);
extern void del_engr (struct engr *);
extern void rloc_engr (struct engr *);
extern void make_grave (int,int,const char *);



extern int newpw(void);
extern int experience (struct monst *,int);
extern void more_experienced (int,int);
extern void losexp (const char *);
extern void newexplevel(void);
extern void pluslvl (int);
extern long rndexp (int);



extern void explode (int,int,int,int,int,int);
extern long scatter (int, int, int, unsigned int, struct obj *);
extern void splatter_burning_oil (int, int);
extern void explode_oil (struct obj *,int,int);



extern void makeroguerooms(void);
extern void corr (int,int);
extern void makerogueghost(void);



extern char *fname_encode (const char *, int, char *, char *, int);
extern char *fname_decode (int, char *, char *, int);
extern const char *fqname (const char *, int, int);
extern FILE *fopen_datafile (const char *,const char *,int);



extern void set_levelfile_name (char *,int);
extern int create_levelfile (int,char *);
extern int open_levelfile (int,char *);
extern void delete_levelfile (int);
extern void clearlocks(void);
extern int create_bonesfile (d_level*,char **, char *);



extern void commit_bonesfile (d_level *);
extern int open_bonesfile (d_level*,char **);
extern int delete_bonesfile (d_level*);
extern void compress_bonesfile(void);
extern void set_savefile_name (int);

extern void save_savefile_name (int);


extern void set_error_savefile(void);

extern int create_savefile(void);
extern int open_savefile(void);
extern int delete_savefile(void);
extern int restore_saved_game(void);
extern void nh_compress (const char *);
extern void nh_uncompress (const char *);
extern boolean lock_file (const char *,int,int);
extern void unlock_file (const char *);



extern boolean read_config_file (const char *, int);
extern void check_recordfile (const char *);
extern void read_wizkit(void);
extern int read_sym_file (int);
extern int parse_sym_line (char *,int);
extern void paniclog (const char *, const char *);
extern int validate_prefix_locations (char *);



extern char** get_saved_games(void);
extern void free_saved_games (char**);




extern void assure_syscf_file(void);

extern int nhclose (int);




extern boolean debugcore (const char *, int);

extern boolean read_tribute (const char *,const char *,int);



extern void floating_above (const char *);
extern void dogushforth (int);



extern void dryup (int,int, int);
extern void drinkfountain(void);
extern void dipfountain (struct obj *);
extern void breaksink (int,int);
extern void drinksink(void);



extern anything *uint_to_any (unsigned);
extern anything *long_to_any (long);
extern anything *monst_to_any (struct monst *);
extern anything *obj_to_any (struct obj *);
extern boolean revive_nasty (int,int,const char*);
extern void movobj (struct obj *,int,int);
extern boolean may_dig (int,int);
extern boolean may_passwall (int,int);
extern boolean bad_rock (struct permonst *,int,int);
extern int cant_squeeze_thru (struct monst *);
extern boolean invocation_pos (int,int);
extern boolean test_move (int, int, int, int, int);
extern boolean u_rooted(void);
extern void domove(void);
extern boolean overexertion(void);
extern void invocation_message(void);
extern boolean pooleffects (int);
extern void spoteffects (int);
extern char *in_rooms (int,int,int);
extern boolean in_town (int,int);
extern void check_special_room (int);
extern int dopickup(void);
extern void lookaround(void);
extern boolean crawl_destination (int,int);
extern int monster_nearby(void);
extern void nomul (int);
extern void unmul (const char *);
extern void losehp (int,const char *,int);
extern int weight_cap(void);
extern int inv_weight(void);
extern int near_capacity(void);
extern int calc_capacity (int);
extern int max_capacity(void);
extern boolean check_capacity (const char *);
extern int inv_cnt (int);
extern long money_cnt (struct obj *);



extern boolean digit (int);
extern boolean letter (int);
extern char highc (int);
extern char lowc (int);
extern char *lcase (char *);
extern char *ucase (char *);
extern char *upstart (char *);
extern char *mungspaces (char *);
extern char *eos (char *);
extern char *strkitten (char *,int);
extern void copynchars (char *,const char *,int);
extern char chrcasecpy (int,int);
extern char *strcasecpy (char *,const char *);
extern char *s_suffix (const char *);
extern char *ing_suffix (const char *);
extern char *xcrypt (const char *,char *);
extern boolean onlyspace (const char *);
extern char *tabexpand (char *);
extern char *visctrl (int);
extern char *strsubst (char *,const char *,const char *);
extern const char *ordin (int);
extern char *sitoa (int);
extern int sgn (int);
extern int rounddiv (long,int);
extern int dist2 (int,int,int,int);
extern int isqrt (int);
extern int distmin (int,int,int,int);
extern boolean online2 (int,int,int,int);
extern boolean pmatch (const char *,const char *);
extern boolean pmatchi (const char *,const char *);
extern boolean pmatchz (const char *,const char *);

extern int strncmpi (const char *,const char *,int);


extern char *strstri (const char *,const char *);

extern boolean fuzzymatch (const char *,const char *,const char *,int);
extern void setrandom(void);
extern time_t getnow(void);
extern int getyear(void);



extern long yyyymmdd (time_t);
extern long hhmmss (time_t);
extern char *yyyymmddhhmmss (time_t);
extern time_t time_from_yyyymmddhhmmss (char *);
extern int phase_of_the_moon(void);
extern boolean friday_13th(void);
extern int night(void);
extern int midnight(void);



extern struct obj **objarr_init (int);
extern void objarr_set (struct obj *, int, struct obj **, int);
extern void assigninvlet (struct obj *);
extern struct obj *merge_choice (struct obj *,struct obj *);
extern int merged (struct obj **,struct obj **);



extern void addinv_core1 (struct obj *);
extern void addinv_core2 (struct obj *);
extern struct obj *addinv (struct obj *);
extern struct obj *hold_another_object (struct obj *,const char *,const char *,const char *);

extern void useupall (struct obj *);
extern void useup (struct obj *);
extern void consume_obj_charge (struct obj *,int);
extern void freeinv_core (struct obj *);
extern void freeinv (struct obj *);
extern void delallobj (int,int);
extern void delobj (struct obj *);
extern struct obj *sobj_at (int,int,int);
extern struct obj *nxtobj (struct obj *,int,int);
extern struct obj *carrying (int);
extern boolean have_lizard(void);
extern struct obj *o_on (unsigned int,struct obj *);
extern boolean obj_here (struct obj *,int,int);
extern boolean wearing_armor(void);
extern boolean is_worn (struct obj *);
extern struct obj *g_at (int,int);
extern struct obj *getobj (const char *,const char *);
extern int ggetobj (const char *,int (*)(struct obj *),int,int,unsigned *);
extern int askchain (struct obj **,const char *,int,int (*)(struct obj *), int (*)(struct obj *),int,const char *);

extern void fully_identify_obj (struct obj *);
extern int identify (struct obj *);
extern void identify_pack (int,int);
extern void learn_unseen_invent(void);
extern void prinv (const char *,struct obj *,long);
extern char *xprname (struct obj *,const char *,int,int,long,long);
extern int ddoinv(void);
extern char display_inventory (const char *,int);
extern int display_binventory (int,int,int);
extern struct obj *display_cinventory (struct obj *);
extern struct obj *display_minventory (struct monst *,int,char *);
extern int dotypeinv(void);
extern const char *dfeature_at (int,int,char *);
extern int look_here (int,int);
extern int dolook(void);
extern boolean will_feel_cockatrice (struct obj *,int);
extern void feel_cockatrice (struct obj *,int);
extern void stackobj (struct obj *);
extern int doprgold(void);
extern int doprwep(void);
extern int doprarm(void);
extern int doprring(void);
extern int dopramulet(void);
extern int doprtool(void);
extern int doprinuse(void);
extern void useupf (struct obj *,long);
extern char *let_to_name (int,int,int);
extern void free_invbuf(void);
extern void reassign(void);
extern int doorganize(void);
extern int count_unpaid (struct obj *);
extern int count_buc (struct obj *,int);
extern long count_contents (struct obj *,int,int,int);
extern void carry_obj_effects (struct obj *);
extern const char *currency (long);
extern void silly_thing (const char *,struct obj *);




extern void getwindowsz(void);
extern void getioctls(void);
extern void setioctls(void);

extern int dosuspend(void);





extern void new_light_source (int, int, int, int, union any *);
extern void del_light_source (int, union any *);
extern void do_light_sources (char **);
extern struct monst *find_mid (unsigned, unsigned);
extern void save_light_sources (int, int, int);
extern void restore_light_sources (int);
extern void relink_light_sources (int);
extern void obj_move_light_source (struct obj *, struct obj *);
extern boolean any_light_source(void);
extern void snuff_light_source (int, int);
extern boolean obj_sheds_light (struct obj *);
extern boolean obj_is_burning (struct obj *);
extern void obj_split_light_source (struct obj *, struct obj *);
extern void obj_merge_light_sources (struct obj *,struct obj *);
extern void obj_adjust_light_radius (struct obj *,int);
extern int candle_light_range (struct obj *);
extern int arti_light_radius (struct obj *);
extern const char *arti_light_description (struct obj *);
extern int wiz_light_sources(void);







extern boolean picking_lock (int *,int *);
extern boolean picking_at (int,int);
extern void breakchestlock (struct obj *,int);
extern void reset_pick(void);
extern int pick_lock (struct obj *);
extern int doforce(void);
extern boolean boxlock (struct obj *,struct obj *);
extern boolean doorlock (struct obj *,int,int);
extern int doopen(void);
extern boolean stumble_on_door_mimic (int,int);
extern int doopen_indir (int,int);
extern int doclose(void);
# 1042 "../include/extern.h"
extern void getmailstatus(void);

extern void ckmailstatus(void);
extern void readmail (struct obj *);




extern void dealloc_monst (struct monst *);
extern boolean is_home_elemental (struct permonst *);
extern struct monst *clone_mon (struct monst *,int,int);
extern int monhp_per_lvl (struct monst *);
extern void newmonhp (struct monst *,int);
extern struct mextra *newmextra(void);
extern void copy_mextra (struct monst *,struct monst *);
extern struct monst *makemon (struct permonst *,int,int,int);
extern boolean create_critters (int,struct permonst *,int);
extern struct permonst *rndmonst(void);
extern void reset_rndmonst (int);
extern struct permonst *mkclass (int,int);
extern int mkclass_poly (int);
extern int adj_lev (struct permonst *);
extern struct permonst *grow_up (struct monst *,struct monst *);
extern int mongets (struct monst *,int);
extern int golemhp (int);
extern boolean peace_minded (struct permonst *);
extern void set_malign (struct monst *);
extern void newmcorpsenm (struct monst *);
extern void freemcorpsenm (struct monst *);
extern void set_mimic_sym (struct monst *);
extern int mbirth_limit (int);
extern void mimic_hit_msg (struct monst *, int);
extern void mkmonmoney (struct monst *, long);
extern int bagotricks (struct obj *,int,int *);
extern boolean propagate (int, int,int);
extern boolean usmellmon (struct permonst *);



extern int mapglyph (int, int *, int *, unsigned *, int, int);
extern char *encglyph (int);
extern void genl_putmixed (winid, int, const char *);



extern int castmu (struct monst *,struct attack *,int,int);
extern int buzzmu (struct monst *,struct attack *);



extern int fightm (struct monst *);
extern int mattackm (struct monst *,struct monst *);
extern boolean engulf_target (struct monst *,struct monst *);
extern int mdisplacem (struct monst *,struct monst *,int);
extern void paralyze_monst (struct monst *,int);
extern int sleep_monst (struct monst *,int,int);
extern void slept_monst (struct monst *);
extern void xdrainenergym (struct monst *,int);
extern long attk_protection (int);
extern void rustm (struct monst *,struct obj *);



extern const char *mpoisons_subj (struct monst *,struct attack *);
extern void u_slow_down(void);
extern struct monst *cloneu(void);
extern void expels (struct monst *,struct permonst *,int);
extern struct attack *getmattk (struct permonst *,int,int *,struct attack *);
extern int mattacku (struct monst *);
extern int magic_negation (struct monst *);
extern boolean gulp_blnd_check(void);
extern int gazemu (struct monst *,struct attack *);
extern void mdamageu (struct monst *,int);
extern int could_seduce (struct monst *,struct monst *,struct attack *);
extern int doseduce (struct monst *);



extern void newemin (struct monst *);
extern void free_emin (struct monst *);
extern int monster_census (int);
extern int msummon (struct monst *);
extern void summon_minion (int,int);
extern int demon_talk (struct monst *);
extern long bribe (struct monst *);
extern int dprince (int);
extern int dlord (int);
extern int llord(void);
extern int ndemon (int);
extern int lminion(void);
extern void lose_guardian_angel (struct monst *);
extern void gain_guardian_angel(void);






extern void sort_rooms(void);
extern void add_room (int,int,int,int,int,int,int);
extern void add_subroom (struct mkroom *,int,int,int,int, int,int,int);

extern void makecorridors(void);
extern void add_door (int,int,struct mkroom *);
extern void mklev(void);



extern void topologize (struct mkroom *);

extern void place_branch (branch *,int,int);
extern boolean occupied (int,int);
extern int okdoor (int,int);
extern void dodoor (int,int,struct mkroom *);
extern void mktrap (int,int,struct mkroom *,coord*);
extern void mkstairs (int,int,int,struct mkroom *);
extern void mkinvokearea(void);
extern void mineralize (int, int, int, int, int);



void flood_fill_rm (int,int,int,int,int);
void remove_rooms (int,int,int,int);



extern void wallification (int,int,int,int);
extern void walkfrom (int,int,int);
extern void makemaz (const char *);
extern void mazexy (coord *);
extern void bound_digging(void);
extern void mkportal (int,int,int,int);
extern boolean bad_location (int,int,int,int,int,int);
extern void place_lregion (int,int,int,int, int,int,int,int, int,d_level *);


extern void fumaroles(void);
extern void movebubbles(void);
extern void water_friction(void);
extern void save_waterlevel (int,int);
extern void restore_waterlevel (int);
extern const char *waterbody_name (int,int);



extern struct oextra *newoextra(void);
extern void copy_oextra (struct obj *,struct obj *);
extern void dealloc_oextra (struct oextra *);
extern void newomonst (struct obj *);
extern void free_omonst (struct obj *);
extern void newomid (struct obj *);
extern void free_omid (struct obj *);
extern void newolong (struct obj *);
extern void free_olong (struct obj *);
extern void new_omailcmd (struct obj *,const char *);
extern void free_omailcmd (struct obj *);
extern struct obj *mkobj_at (int,int,int,int);
extern struct obj *mksobj_at (int,int,int,int,int);
extern struct obj *mkobj (int,int);
extern int rndmonnum(void);
extern boolean bogon_is_pname (int);
extern struct obj *splitobj (struct obj *,long);
extern void replace_object (struct obj *,struct obj *);
extern void bill_dummy_object (struct obj *);
extern void costly_alteration (struct obj *,int);
extern struct obj *mksobj (int,int,int);
extern int bcsign (struct obj *);
extern int weight (struct obj *);
extern struct obj *mkgold (long,int,int);
extern struct obj *mkcorpstat (int,struct monst *,struct permonst *,int,int,unsigned);

extern int corpse_revive_type (struct obj *);
extern struct obj *obj_attach_mid (struct obj *, unsigned);
extern struct monst *get_mtraits (struct obj *, int);
extern struct obj *mk_tt_object (int,int,int);
extern struct obj *mk_named_object (int,struct permonst *,int,int,const char *);

extern struct obj *rnd_treefruit_at (int, int);
extern void set_corpsenm (struct obj *, int);
extern void start_corpse_timeout (struct obj *);
extern void bless (struct obj *);
extern void unbless (struct obj *);
extern void curse (struct obj *);
extern void uncurse (struct obj *);
extern void blessorcurse (struct obj *,int);
extern boolean is_flammable (struct obj *);
extern boolean is_rottable (struct obj *);
extern void place_object (struct obj *,int,int);
extern void remove_object (struct obj *);
extern void discard_minvent (struct monst *);
extern void obj_extract_self (struct obj *);
extern void extract_nobj (struct obj *, struct obj **);
extern void extract_nexthere (struct obj *, struct obj **);
extern int add_to_minv (struct monst *, struct obj *);
extern struct obj *add_to_container (struct obj *, struct obj *);
extern void add_to_migration (struct obj *);
extern void add_to_buried (struct obj *);
extern void dealloc_obj (struct obj *);
extern void obj_ice_effects (int, int, int);
extern long peek_at_iced_corpse_age (struct obj *);
extern int hornoplenty (struct obj *,int);
extern void obj_sanity_check(void);
extern struct obj* obj_nexto (struct obj*);
extern struct obj* obj_nexto_xy (int, int, int, unsigned);
extern struct obj* obj_absorb (struct obj**, struct obj**);
extern struct obj* obj_meld (struct obj**, struct obj**);



extern void mkroom (int);
extern void fill_zoo (struct mkroom *);
extern struct permonst *antholemon(void);
extern boolean nexttodoor (int,int);
extern boolean has_dnstairs (struct mkroom *);
extern boolean has_upstairs (struct mkroom *);
extern int somex (struct mkroom *);
extern int somey (struct mkroom *);
extern boolean inside_room (struct mkroom *,int,int);
extern boolean somexy (struct mkroom *,coord *);
extern void mkundead (coord *,int,int);
extern struct permonst *courtmon(void);
extern void save_rooms (int);
extern void rest_rooms (int);
extern struct mkroom *search_special (int);
extern int cmap_to_type (int);



extern int undead_to_corpse (int);
extern int genus (int,int);
extern int pm_to_cham (int);
extern int minliquid (struct monst *);
extern int movemon(void);
extern int meatmetal (struct monst *);
extern int meatobj (struct monst *);
extern void mpickgold (struct monst *);
extern boolean mpickstuff (struct monst *,const char *);
extern int curr_mon_load (struct monst *);
extern int max_mon_load (struct monst *);
extern boolean can_carry (struct monst *,struct obj *);
extern int mfndpos (struct monst *,coord *,long *,long);
extern boolean monnear (struct monst *,int,int);
extern void dmonsfree(void);
extern int mcalcmove (struct monst*);
extern void mcalcdistress(void);
extern void replmon (struct monst *,struct monst *);
extern void relmon (struct monst *,struct monst **);
extern struct obj *mlifesaver (struct monst *);
extern boolean corpse_chance (struct monst *,struct monst *,int);
extern void mondead (struct monst *);
extern void mondied (struct monst *);
extern void mongone (struct monst *);
extern void monstone (struct monst *);
extern void monkilled (struct monst *,const char *,int);
extern void unstuck (struct monst *);
extern void killed (struct monst *);
extern void xkilled (struct monst *,int);
extern void mon_to_stone (struct monst*);
extern void mnexto (struct monst *);
extern void maybe_mnexto (struct monst *);
extern boolean mnearto (struct monst *,int,int,int);
extern void m_respond (struct monst *);
extern void setmangry (struct monst *);
extern void wakeup (struct monst *);
extern void wake_nearby(void);
extern void wake_nearto (int,int,int);
extern void seemimic (struct monst *);
extern void rescham(void);
extern void restartcham(void);
extern void restore_cham (struct monst *);
extern boolean hideunder (struct monst*);
extern void hide_monst (struct monst *);
extern void mon_animal_list (int);
extern int select_newcham_form (struct monst *);
extern void mgender_from_permonst (struct monst *, struct permonst *);
extern int newcham (struct monst *,struct permonst *,int,int);
extern int can_be_hatched (int);
extern int egg_type_from_parent (int,int);
extern boolean dead_species (int,int);
extern void kill_genocided_monsters(void);
extern void golemeffects (struct monst *,int,int);
extern boolean angry_guards (int);
extern void pacify_guards(void);
extern void decide_to_shapeshift (struct monst *,int);



extern void set_mon_data (struct monst *,struct permonst *,int);
extern struct attack *attacktype_fordmg (struct permonst *,int,int);
extern boolean attacktype (struct permonst *,int);
extern boolean noattacks (struct permonst *);
extern boolean poly_when_stoned (struct permonst *);
extern boolean resists_drli (struct monst *);
extern boolean resists_magm (struct monst *);
extern boolean resists_blnd (struct monst *);
extern boolean can_blnd (struct monst *,struct monst *,int,struct obj *);
extern boolean ranged_attk (struct permonst *);
extern boolean hates_silver (struct permonst *);
extern boolean mon_hates_silver (struct monst *);
extern boolean passes_bars (struct permonst *);
extern boolean can_blow (struct monst *);
extern boolean can_be_strangled (struct monst *);
extern boolean can_track (struct permonst *);
extern boolean breakarm (struct permonst *);
extern boolean sliparm (struct permonst *);
extern boolean sticks (struct permonst *);
extern boolean cantvomit (struct permonst *);
extern int num_horns (struct permonst *);

extern struct attack *dmgtype_fromattack (struct permonst *,int,int);
extern boolean dmgtype (struct permonst *,int);
extern int max_passive_dmg (struct monst *,struct monst *);
extern boolean same_race (struct permonst *,struct permonst *);
extern int monsndx (struct permonst *);
extern int name_to_mon (const char *);
extern int name_to_monclass (const char *,int *);
extern int gender (struct monst *);
extern int pronoun_gender (struct monst *);
extern boolean levl_follower (struct monst *);
extern int little_to_big (int);
extern int big_to_little (int);
extern const char *locomotion (const struct permonst *,const char *);
extern const char *stagger (const struct permonst *,const char *);
extern const char *on_fire (struct permonst *,struct attack *);
extern const struct permonst *raceptr (struct monst *);
extern boolean olfaction (struct permonst *);



extern boolean itsstuck (struct monst *);
extern boolean mb_trapped (struct monst *);
extern boolean monhaskey (struct monst *,int);
extern void mon_regen (struct monst *,int);
extern int dochugw (struct monst *);
extern boolean onscary (int,int,struct monst *);
extern void monflee (struct monst *, int, int, int);
extern void mon_yells (struct monst *, const char *);
extern int dochug (struct monst *);
extern int m_move (struct monst *,int);
extern void dissolve_bars (int,int);
extern boolean closed_door (int,int);
extern boolean accessible (int,int);
extern void set_apparxy (struct monst *);
extern boolean can_ooze (struct monst *);
extern boolean can_fog (struct monst *);
extern boolean should_displace (struct monst *,coord *,long *,int, int,int);

extern boolean undesirable_disp (struct monst *,int,int);



extern void monst_init(void);



extern void monstr_init(void);



extern struct monst *mk_mplayer (struct permonst *,int, int,int);

extern void create_mplayers (int,int);
extern void mplayer_talk (struct monst *);
# 1459 "../include/extern.h"
extern int thitu (int,int,struct obj *,const char *);
extern int ohitmon (struct monst *,struct obj *,int,int);
extern void thrwmu (struct monst *);
extern int spitmu (struct monst *,struct attack *);
extern int breamu (struct monst *,struct attack *);
extern boolean linedup (int,int,int,int,int);
extern boolean lined_up (struct monst *);
extern struct obj *m_carrying (struct monst *,int);
extern void m_useupall (struct monst *,struct obj *);
extern void m_useup (struct monst *,struct obj *);
extern void m_throw (struct monst *,int,int,int,int,int,struct obj *);
extern boolean hits_bars (struct obj **,int,int,int,int);



extern boolean find_defensive (struct monst *);
extern int use_defensive (struct monst *);
extern int rnd_defensive_item (struct monst *);
extern boolean find_offensive (struct monst *);



extern int use_offensive (struct monst *);
extern int rnd_offensive_item (struct monst *);
extern boolean find_misc (struct monst *);
extern int use_misc (struct monst *);
extern int rnd_misc_item (struct monst *);
extern boolean searches_for_item (struct monst *,struct obj *);
extern boolean mon_reflects (struct monst *,const char *);
extern boolean ureflects (const char *,const char *);
extern boolean munstone (struct monst *,int);
extern boolean munslime (struct monst *,int);



extern void awaken_soldiers (struct monst *);
extern int do_play_instrument (struct obj *);
# 1504 "../include/extern.h"
extern struct nhregex * regex_init(void);
extern boolean regex_compile (const char *, struct nhregex *);
extern const char *regex_error_desc (struct nhregex *);
extern boolean regex_match (const char *, struct nhregex*);
extern void regex_free (struct nhregex *);
# 1525 "../include/extern.h"
extern void init_objects(void);
extern void obj_shuffle_range (int,int *,int *);
extern int find_skates(void);
extern void oinit(void);
extern void savenames (int,int);
extern void restnames (int);
extern void discover_object (int,int,int);
extern void undiscover_object (int);
extern int dodiscovered(void);
extern int doclassdisco(void);
extern void rename_disco(void);



extern void objects_init(void);



extern char *obj_typename (int);
extern char *simple_typename (int);
extern boolean obj_is_pname (struct obj *);
extern char *distant_name (struct obj *,char *(*)(struct obj *));
extern char *fruitname (int);
extern char *xname (struct obj *);
extern char *mshot_xname (struct obj *);
extern boolean the_unique_obj (struct obj *);
extern boolean the_unique_pm (struct permonst *);
extern char *doname (struct obj *);
extern boolean not_fully_identified (struct obj *);
extern char *corpse_xname (struct obj *,const char *,unsigned);
extern char *cxname (struct obj *);
extern char *cxname_singular (struct obj *);
extern char *killer_xname (struct obj *);
extern char *short_oname (struct obj *,char *(*)(struct obj *),char *(*)(struct obj *), unsigned);

extern const char *singular (struct obj *,char *(*)(struct obj *));
extern char *an (const char *);
extern char *An (const char *);
extern char *The (const char *);
extern char *the (const char *);
extern char *aobjnam (struct obj *,const char *);
extern char *yobjnam (struct obj *,const char *);
extern char *Yobjnam2 (struct obj *,const char *);
extern char *Tobjnam (struct obj *,const char *);
extern char *otense (struct obj *,const char *);
extern char *vtense (const char *,const char *);
extern char *Doname2 (struct obj *);
extern char *yname (struct obj *);
extern char *Yname2 (struct obj *);
extern char *ysimple_name (struct obj *);
extern char *Ysimple_name2 (struct obj *);
extern char *simpleonames (struct obj *);
extern char *ansimpleoname (struct obj *);
extern char *thesimpleoname (struct obj *);
extern char *bare_artifactname (struct obj *);
extern char *makeplural (const char *);
extern char *makesingular (const char *);
extern struct obj *readobjnam (char *,struct obj *);
extern int rnd_class (int,int);
extern const char *suit_simple_name (struct obj *);
extern const char *cloak_simple_name (struct obj *);
extern const char *helm_simple_name (struct obj *);
extern const char *mimic_obj_name (struct monst *);
extern char *safe_qbuf (char *,const char *,const char *,struct obj *, char *(*)(struct obj *),char *(*)(struct obj *),const char *);




extern boolean match_optname (const char *,const char *,int,int);
extern void initoptions(void);
extern void initoptions_init(void);
extern void initoptions_finish(void);
extern void parseoptions (char *,int,int);
extern int doset(void);
extern int dotogglepickup(void);
extern void option_help(void);
extern void next_opt (winid,const char *);
extern int fruitadd (char *,struct fruit *);
extern int choose_classes_menu (const char *,int,int,char *,char *);
extern void add_menu_cmd_alias (int, int);
extern char map_menu_cmd (int);
extern void assign_warnings (uchar *);
extern char *nh_getenv (const char *);
extern void set_duplicate_opt_detection (int);
extern void set_wc_option_mod_status (unsigned long, int);
extern void set_wc2_option_mod_status (unsigned long, int);
extern void set_option_mod_status (const char *,int);
extern int add_autopickup_exception (const char *);
extern void free_autopickup_exceptions(void);
extern int load_symset (const char *,int);
extern void parsesymbols (char *);
extern struct symparse *match_sym (char *);
extern void set_playmode(void);
extern int sym_val (char *);
extern boolean add_menu_coloring (char *);
extern boolean get_menu_coloring (char *, int *, int *);
extern void free_menu_coloring(void);



extern char *self_lookat (char *);
extern int dowhatis(void);
extern int doquickwhatis(void);
extern int doidtrap(void);
extern int dowhatdoes(void);
extern char *dowhatdoes_core (int, char *);
extern int dohelp(void);
extern int dohistory(void);
extern int do_screen_description (coord, int, int, char *, const char **);
extern int do_look (int, coord *);
# 1688 "../include/extern.h"
extern int collect_obj_classes (char *,struct obj *,int,boolean (*) (struct obj *), int *);

extern boolean rider_corpse_revival (struct obj *,int);
extern void add_valid_menu_class (int);
extern boolean allow_all (struct obj *);
extern boolean allow_category (struct obj *);
extern boolean is_worn_by_type (struct obj *);
extern int ck_bag (struct obj *);




extern int pickup (int);
extern int pickup_object (struct obj *, long, int);
extern int query_category (const char *, struct obj *, int, menu_item **, int);

extern int query_objlist (const char *, struct obj *, int, menu_item **, int, boolean (*)(struct obj *));

extern struct obj *pick_obj (struct obj *);
extern int encumber_msg(void);
extern int doloot(void);
extern boolean container_gone (int (*)(struct obj *));
extern boolean u_handsy(void);
extern int use_container (struct obj **,int);
extern int loot_mon (struct monst *,int *,boolean *);
extern int dotip(void);
extern boolean is_autopickup_exception (struct obj *, int);



extern void pline (const char *,...) __attribute__ ((format (printf, 1, 2)));
extern void Norep (const char *,...) __attribute__ ((format (printf, 1, 2)));
extern void free_youbuf(void);
extern void You (const char *,...) __attribute__ ((format (printf, 1, 2)));
extern void Your (const char *,...) __attribute__ ((format (printf, 1, 2)));
extern void You_feel (const char *,...) __attribute__ ((format (printf, 1, 2)));
extern void You_cant (const char *,...) __attribute__ ((format (printf, 1, 2)));
extern void You_hear (const char *,...) __attribute__ ((format (printf, 1, 2)));
extern void You_see (const char *,...) __attribute__ ((format (printf, 1, 2)));
extern void pline_The (const char *,...) __attribute__ ((format (printf, 1, 2)));
extern void There (const char *,...) __attribute__ ((format (printf, 1, 2)));
extern void verbalize (const char *,...) __attribute__ ((format (printf, 1, 2)));
extern void raw_printf (const char *,...) __attribute__ ((format (printf, 1, 2)));
extern void impossible (const char *,...) __attribute__ ((format (printf, 1, 2)));
extern const char *align_str (int);
extern void mstatusline (struct monst *);
extern void ustatusline(void);
extern void self_invis_message(void);
extern void pudding_merge_message (struct obj*, struct obj*);



extern void set_uasmon(void);
extern void float_vs_flight(void);
extern void change_sex(void);
extern void polyself (int);
extern int polymon (int);
extern void rehumanize(void);
extern int dobreathe(void);
extern int dospit(void);
extern int doremove(void);
extern int dospinweb(void);
extern int dosummon(void);
extern int dogaze(void);
extern int dohide(void);
extern int dopoly(void);
extern int domindblast(void);
extern void skinback (int);
extern const char *mbodypart (struct monst *,int);
extern const char *body_part (int);
extern int poly_gender(void);
extern void ugolemeffects (int,int);



extern void set_itimeout (long *,long);
extern void incr_itimeout (long *,int);
extern void make_confused (long,int);
extern void make_stunned (long,int);
extern void make_blinded (long,int);
extern void make_sick (long, const char *, int,int);
extern void make_slimed (long,const char *);
extern void make_stoned (long,const char *,int,const char *);
extern void make_vomiting (long,int);
extern boolean make_hallucinated (long,int,long);
extern int dodrink(void);
extern int dopotion (struct obj *);
extern int peffects (struct obj *);
extern void healup (int,int,int,int);
extern void strange_feeling (struct obj *,const char *);
extern void potionhit (struct monst *,struct obj *,int);
extern void potionbreathe (struct obj *);
extern int dodip(void);
extern void mongrantswish (struct monst **);
extern void djinni_from_bottle (struct obj *);
extern struct monst *split_mon (struct monst *,struct monst *);
extern const char *bottlename(void);



extern boolean critically_low_hp (int);



extern int dosacrifice(void);
extern boolean can_pray (int);
extern int dopray(void);
extern const char *u_gname(void);
extern int doturn(void);
extern const char *a_gname(void);
extern const char *a_gname_at (int x,int y);
extern const char *align_gname (int);
extern const char *halu_gname (int);
extern const char *align_gtitle (int);
extern void altar_wrath (int,int);




extern int move_special (struct monst *,int,int,int,int, int,int,int,int);

extern char temple_occupied (char *);
extern boolean inhistemple (struct monst *);
extern int pri_move (struct monst *);
extern void priestini (d_level *,struct mkroom *,int,int,int);
extern aligntyp mon_aligntyp (struct monst *);
extern char *priestname (struct monst *,char *);
extern boolean p_coaligned (struct monst *);
extern struct monst *findpriest (int);
extern void intemple (int);
extern void forget_temple_entry (struct monst *);
extern void priest_talk (struct monst *);
extern struct monst *mk_roamer (struct permonst *,int, int,int,int);

extern void reset_hostility (struct monst *);
extern boolean in_your_sanctuary (struct monst *,int,int);
extern void ghod_hitsu (struct monst *);
extern void angry_priest(void);
extern void clearpriests(void);
extern void restpriest (struct monst *,int);
extern void newepri (struct monst *);
extern void free_epri (struct monst *);



extern void onquest(void);
extern void nemdead(void);
extern void artitouch (struct obj *);
extern boolean ok_to_quest(void);
extern void leader_speaks (struct monst *);
extern void nemesis_speaks(void);
extern void quest_chat (struct monst *);
extern void quest_talk (struct monst *);
extern void quest_stat_check (struct monst *);
extern void finish_quest (struct obj *);



extern void load_qtlist(void);
extern void unload_qtlist(void);
extern short quest_info (int);
extern const char *ldrname(void);
extern boolean is_quest_artifact (struct obj*);
extern void com_pager (int);
extern void qt_pager (int);
extern struct permonst *qt_montype(void);
extern void deliver_splev_message(void);
# 1867 "../include/extern.h"
extern void learnscroll (struct obj *);
extern char *tshirt_text (struct obj *, char *);
extern int doread(void);
extern boolean is_chargeable (struct obj *);
extern void recharge (struct obj *,int);
extern void forget_objects (int);
extern void forget_levels (int);
extern void forget_traps(void);
extern void forget_map (int);
extern int seffects (struct obj *);
extern void drop_boulder_on_player (int, int, int, int);
extern boolean drop_boulder_on_monster (int, int, int, int);
extern void wand_explode (struct obj *,int);



extern void litroom (int,struct obj *);
extern void do_genocide (int);
extern void punish (struct obj *);
extern void unpunish(void);
extern boolean cant_revive (int *,int,struct obj *);
extern boolean create_particular(void);



extern void init_rect(void);
extern NhRect *get_rect (NhRect *);
extern NhRect *rnd_rect(void);
extern void remove_rect (NhRect *);
extern void add_rect (NhRect *);
extern void split_rects (NhRect *,NhRect *);


extern void clear_regions(void);
extern void run_regions(void);
extern boolean in_out_region (int,int);
extern boolean m_in_out_region (struct monst *,int,int);
extern void update_player_regions(void);
extern void update_monster_region (struct monst *);
extern NhRegion *visible_region_at (int,int);
extern void show_region (NhRegion*, int, int);
extern void save_regions (int,int);
extern void rest_regions (int,int);
extern NhRegion* create_gas_cloud (int, int, int, int);
extern boolean region_danger(void);
extern void region_safety(void);



extern void inven_inuse (int);
extern int dorecover (int);
extern void restcemetery (int,struct cemetery **);
extern void trickery (char *);
extern void getlev (int,int,int,int);
extern void get_plname_from_file (int, char *);



extern void minit(void);
extern boolean lookup_id_mapping (unsigned, unsigned *);
extern void mread (int,genericptr_t,unsigned int);
extern int validate (int,const char *);
extern void reset_restpref(void);
extern void set_restpref (const char *);
extern void set_savepref (const char *);



extern void genl_outrip (winid,int,time_t);



extern int rn2 (int);
extern int rnl (int);
extern int rnd (int);
extern int d (int,int);
extern int rne (int);
extern int rnz (int);



extern boolean validrole (int);
extern boolean validrace (int, int);
extern boolean validgend (int, int, int);
extern boolean validalign (int, int, int);
extern int randrole(void);
extern int randrace (int);
extern int randgend (int, int);
extern int randalign (int, int);
extern int str2role (const char *);
extern int str2race (const char *);
extern int str2gend (char *);
extern int str2align (char *);
extern boolean ok_role (int, int, int, int);
extern int pick_role (int, int, int, int);
extern boolean ok_race (int, int, int, int);
extern int pick_race (int, int, int, int);
extern boolean ok_gend (int, int, int, int);
extern int pick_gend (int, int, int, int);
extern boolean ok_align (int, int, int, int);
extern int pick_align (int, int, int, int);
extern void rigid_role_checks(void);
extern boolean setrolefilter (char *);
extern char *build_plselection_prompt (char *,int,int,int,int,int);
extern char *root_plselection_prompt (char *,int,int,int,int,int);
extern void plnamesuffix(void);
extern void role_selection_prolog (int,winid);
extern void role_menu_extra (int,winid);
extern void role_init(void);
extern const char *Hello (struct monst *);
extern const char *Goodbye(void);



extern char *getrumor (int,char *, int);
extern char *get_rnd_text (const char *, char *);
extern void outrumor (int,int);
extern void outoracle (int, int);
extern void save_oracles (int,int);
extern void restore_oracles (int);
extern int doconsult (struct monst *);
extern void rumor_check(void);



extern int dosave(void);
extern int dosave0(void);
extern boolean tricked_fileremoved (int, char *);

extern void savestateinlock(void);






extern void savelev (int,int,int);

extern genericptr_t mon_to_buffer (struct monst *, int *);
extern void bufon (int);
extern void bufoff (int);
extern void bflush (int);
extern void bwrite (int,genericptr_t,unsigned int);
extern void bclose (int);
extern void def_bclose (int);



extern void savecemetery (int,int,struct cemetery **);
extern void savefruitchn (int,int);
extern void store_plname_in_file (int);
extern void free_dungeons(void);
extern void freedynamicdata(void);
extern void store_savefileinfo (int);



extern long money2mon (struct monst *, long);
extern void money2u (struct monst *, long);
extern void shkgone (struct monst *);
extern void set_residency (struct monst *,int);
extern void replshk (struct monst *,struct monst *);
extern void restshk (struct monst *,int);
extern char inside_shop (int,int);
extern void u_left_shop (char *,int);
extern void remote_burglary (int,int);
extern void u_entered_shop (char *);
extern void pick_pick (struct obj *);
extern boolean same_price (struct obj *,struct obj *);
extern void shopper_financial_report(void);
extern int inhishop (struct monst *);
extern struct monst *shop_keeper (int);
extern boolean tended_shop (struct mkroom *);
extern boolean is_unpaid (struct obj *);
extern void delete_contents (struct obj *);
extern void obfree (struct obj *,struct obj *);
extern void home_shk (struct monst *,int);
extern void make_happy_shk (struct monst *,int);
extern void make_happy_shoppers (int);
extern void hot_pursuit (struct monst *);
extern void make_angry_shk (struct monst *,int,int);
extern int dopay(void);
extern boolean paybill (int);
extern void finish_paybill(void);
extern struct obj *find_oid (unsigned);
extern long contained_cost (struct obj *,struct monst *,long,int, int);
extern long contained_gold (struct obj *);
extern void picked_container (struct obj *);
extern void alter_cost (struct obj *,long);
extern long unpaid_cost (struct obj *,int);
extern boolean billable (struct monst **,struct obj *,int,int);
extern void addtobill (struct obj *,int,int,int);
extern void splitbill (struct obj *,struct obj *);
extern void subfrombill (struct obj *,struct monst *);
extern long stolen_value (struct obj *,int,int,int,int);
extern void sellobj_state (int);
extern void sellobj (struct obj *,int,int);
extern int doinvbill (int);
extern struct monst *shkcatch (struct obj *,int,int);
extern void add_damage (int,int,long);
extern int repair_damage (struct monst *,struct damage *,int);
extern int shk_move (struct monst *);
extern void after_shk_move (struct monst *);
extern boolean is_fshk (struct monst *);
extern void shopdig (int);
extern void pay_for_damage (const char *,int);
extern boolean costly_spot (int,int);
extern struct obj *shop_object (int,int);
extern void price_quote (struct obj *);
extern void shk_chat (struct monst *);
extern void check_unpaid_usage (struct obj *,int);
extern void check_unpaid (struct obj *);
extern void costly_gold (int,int,long);
extern boolean block_door (int,int);
extern boolean block_entry (int,int);
extern char *shk_your (char *,struct obj *);
extern char *Shk_Your (char *,struct obj *);



extern void neweshk (struct monst *);
extern void free_eshk (struct monst *);
extern void stock_room (int,struct mkroom *);
extern boolean saleable (struct monst *,struct obj *);
extern int get_shop_item (int);
extern const char *shkname (struct monst *);
extern boolean shkname_is_pname (struct monst *);
extern boolean is_izchak (struct monst *,int);



extern void take_gold(void);
extern int dosit(void);
extern void rndcurse(void);
extern void attrcurse(void);



extern void dosounds(void);
extern const char *growl_sound (struct monst *);
extern void growl (struct monst *);
extern void yelp (struct monst *);
extern void whimper (struct monst *);
extern void beg (struct monst *);
extern int dotalk(void);







extern void sys_early_init(void);
extern void sysopt_release(void);
extern void sysopt_seduce_set (int);
# 2131 "../include/extern.h"
extern boolean check_room (xchar *,xchar *,xchar *,xchar *,int);
extern boolean create_room (int,int,int,int, int,int,int,int);

extern void create_secret_door (struct mkroom *,int);
extern boolean dig_corridor (coord *,coord *,int,int,int);
extern void fill_room (struct mkroom *,int);
extern boolean load_special (const char *);






extern int study_book (struct obj *);
extern void book_disappears (struct obj *);
extern void book_substitution (struct obj *,struct obj *);
extern void age_spells(void);
extern int docast(void);
extern int spell_skilltype (int);
extern int spelleffects (int,int);
extern void losespells(void);
extern int dovspell(void);
extern void initialspell (struct obj *);






extern long somegold (long);
extern void stealgold (struct monst *);
extern void remove_worn_item (struct obj *,int);
extern int steal (struct monst *, char *);
extern int mpickobj (struct monst *,struct obj *);
extern void stealamulet (struct monst *);
extern void maybe_absorb_item (struct monst *,struct obj *,int,int);
extern void mdrop_obj (struct monst *,struct obj *,int);
extern void mdrop_special_objs (struct monst *);
extern void relobj (struct monst *,int,int);
extern struct obj *findgold (struct obj *);



extern void rider_cant_reach(void);
extern boolean can_saddle (struct monst *);
extern int use_saddle (struct obj *);
extern boolean can_ride (struct monst *);
extern int doride(void);
extern boolean mount_steed (struct monst *, int);
extern void exercise_steed(void);
extern void kick_steed(void);
extern void dismount_steed (int);
extern void place_monster (struct monst *,int,int);
extern boolean stucksteed (int);



extern boolean goodpos (int,int,struct monst *,unsigned);
extern boolean enexto (coord *,int,int,struct permonst *);
extern boolean enexto_core (coord *,int,int,struct permonst *,unsigned);
extern void teleds (int,int,int);
extern boolean safe_teleds (int);
extern boolean teleport_pet (struct monst *,int);
extern void tele(void);
extern boolean scrolltele (struct obj *);
extern int dotele(void);
extern void level_tele(void);
extern void domagicportal (struct trap *);
extern void tele_trap (struct trap *);
extern void level_tele_trap (struct trap *);
extern void rloc_to (struct monst *,int,int);
extern boolean rloc (struct monst *, int);
extern boolean tele_restrict (struct monst *);
extern void mtele_trap (struct monst *, struct trap *,int);
extern int mlevel_tele_trap (struct monst *, struct trap *,int,int);
extern boolean rloco (struct obj *);
extern int random_teleport_level(void);
extern boolean u_teleport_mon (struct monst *,int);
# 2217 "../include/extern.h"
extern void burn_away_slime(void);
extern void nh_timeout(void);
extern void fall_asleep (int, int);
extern void attach_egg_hatch_timeout (struct obj *, long);
extern void attach_fig_transform_timeout (struct obj *);
extern void kill_egg (struct obj *);
extern void hatch_egg (union any *, long);
extern void learn_egg_type (int);
extern void burn_object (union any *, long);
extern void begin_burn (struct obj *, int);
extern void end_burn (struct obj *, int);
extern void do_storms(void);
extern boolean start_timer (long, int, int, union any *);
extern long stop_timer (int, union any *);
extern long peek_timer (int,union any *);
extern void run_timers(void);
extern void obj_move_timers (struct obj *, struct obj *);
extern void obj_split_timers (struct obj *, struct obj *);
extern void obj_stop_timers (struct obj *);
extern boolean obj_has_timer (struct obj *,int);
extern void spot_stop_timers (int,int,int);
extern long spot_time_expires (int,int,int);
extern long spot_time_left (int,int,int);
extern boolean obj_is_local (struct obj *);
extern void save_timers (int,int,int);
extern void restore_timers (int,int,int,long);
extern void relink_timers (int);
extern int wiz_timeout_queue(void);
extern void timer_sanity_check(void);



extern void formatkiller (char *,unsigned,int);
extern void topten (int,time_t);
extern void prscore (int,char **);
extern struct obj *tt_oname (struct obj *);



extern void initrack(void);
extern void settrack(void);
extern coord *gettrack (int,int);



extern boolean burnarmor (struct monst *);
extern int erode_obj (struct obj *,const char *,int,int);
extern boolean grease_protect (struct obj *,const char *,struct monst *);
extern struct trap *maketrap (int,int,int);
extern void fall_through (int);
extern struct monst *animate_statue (struct obj *,int,int,int,int *);
extern struct monst *activate_statue_trap (struct trap *,int,int,int);

extern void dotrap (struct trap *, unsigned);
extern void seetrap (struct trap *);
extern void feeltrap (struct trap *);
extern int mintrap (struct monst *);
extern void instapetrify (const char *);
extern void minstapetrify (struct monst *,int);
extern void selftouch (const char *);
extern void mselftouch (struct monst *,const char *,int);
extern void float_up(void);
extern void fill_pit (int,int);
extern int float_down (long, long);
extern void climb_pit(void);
extern boolean fire_damage (struct obj *,int,int,int);
extern int fire_damage_chain (struct obj *,int,int,int,int);
extern void acid_damage(struct obj *);
extern int water_damage (struct obj *,const char*,int);
extern void water_damage_chain (struct obj *,int);
extern boolean drown(void);
extern void drain_en (int);
extern int dountrap(void);
extern void cnv_trap_obj (int,int,struct trap *,int);
extern int untrap (int);
extern boolean openholdingtrap (struct monst *,boolean *);
extern boolean closeholdingtrap (struct monst *,boolean *);
extern boolean openfallingtrap (struct monst *,int,boolean *);
extern boolean chest_trap (struct obj *,int,int);
extern void deltrap (struct trap *);
extern boolean delfloortrap (struct trap *);
extern struct trap *t_at (int,int);
extern void b_trapped (const char *,int);
extern boolean unconscious(void);
extern void blow_up_landmine (struct trap *);
extern int launch_obj (int,int,int,int,int,int);
extern boolean launch_in_progress(void);
extern void force_launch_placement(void);
extern boolean uteetering_at_seen_pit (struct trap *);
extern boolean lava_effects(void);
extern void sink_into_lava(void);
extern void sokoban_guilt(void);



extern void u_init(void);



extern void erode_armor (struct monst *,int);
extern boolean attack_checks (struct monst *,struct obj *);
extern void check_caitiff (struct monst *);
extern int find_roll_to_hit (struct monst *,int,struct obj *,int *,int *);
extern boolean attack (struct monst *);
extern boolean hmon (struct monst *,struct obj *,int);
extern int damageum (struct monst *,struct attack *);
extern void missum (struct monst *,struct attack *,int);
extern int passive (struct monst *,int,int,int,int);
extern void passive_obj (struct monst *,struct obj *,struct attack *);
extern void stumble_onto_mimic (struct monst *);
extern int flash_hits_mon (struct monst *,struct obj *);
extern void light_hits_gremlin (struct monst *,int);







extern void sethanguphandler (void (*)(int));
extern boolean authorize_wizard_mode(void);
extern boolean check_user_string (char *);





extern void gettty(void);
extern void settty (const char *);
extern void setftty(void);
extern void intron(void);
extern void introff(void);
extern void error (const char *,...) __attribute__ ((format (printf, 1, 2)));





extern void getlock(void);
extern void regularize (char *);




extern int dosh(void);


extern int child (int);


extern boolean file_exists (const char *);
# 2381 "../include/extern.h"
extern void newegd (struct monst *);
extern void free_egd (struct monst *);
extern boolean grddead (struct monst *);
extern char vault_occupied (char *);
extern void invault(void);
extern int gd_move (struct monst *);
extern void paygd(void);
extern long hidden_gold(void);
extern boolean gd_sound(void);
extern void vault_gd_watching (unsigned int);



extern char *version_string (char *);
extern char *getversionstring (char *);
extern int doversion(void);
extern int doextversion(void);



extern boolean check_version (struct version_info *, const char *,int);

extern boolean uptodate (int,const char *);
extern void store_version (int);
extern unsigned long get_feature_notice_ver (char *);
extern unsigned long get_current_feature_ver(void);
extern const char *copyright_banner_line (int);
# 2436 "../include/extern.h"
extern void vision_init(void);
extern int does_block (int,int,struct rm*);
extern void vision_reset(void);
extern void vision_recalc (int);
extern void block_point (int,int);
extern void unblock_point (int,int);
extern boolean clear_path (int,int,int,int);
extern void do_clear_area (int,int,int, void (*)(int,int,genericptr_t),genericptr_t);

extern unsigned howmonseen (struct monst *);
# 2529 "../include/extern.h"
extern const char *weapon_descr (struct obj *);
extern int hitval (struct obj *,struct monst *);
extern int dmgval (struct obj *,struct monst *);
extern struct obj *select_rwep (struct monst *);
extern struct obj *select_hwep (struct monst *);
extern void possibly_unwield (struct monst *,int);
extern void mwepgone (struct monst *);
extern int mon_wield_item (struct monst *);
extern int abon(void);
extern int dbon(void);
extern int enhance_weapon_skill(void);
extern void unrestrict_weapon_skill (int);
extern void use_skill (int,int);
extern void add_weapon_skill (int);
extern void lose_weapon_skill (int);
extern int weapon_type (struct obj *);
extern int uwep_skill_type(void);
extern int weapon_hit_bonus (struct obj *);
extern int weapon_dam_bonus (struct obj *);
extern void skill_init (const struct def_skill *);



extern void were_change (struct monst *);
extern int counter_were (int);
extern int were_beastie (int);
extern void new_were (struct monst *);
extern int were_summon (struct permonst *,int,int *,char *);
extern void you_were(void);
extern void you_unwere (int);



extern void setuwep (struct obj *);
extern void setuqwep (struct obj *);
extern void setuswapwep (struct obj *);
extern int dowield(void);
extern int doswapweapon(void);
extern int dowieldquiver(void);
extern boolean wield_tool (struct obj *,const char *);
extern int can_twoweapon(void);
extern void drop_uswapwep(void);
extern int dotwoweapon(void);
extern void uwepgone(void);
extern void uswapwepgone(void);
extern void uqwepgone(void);
extern void untwoweapon(void);
extern int chwepon (struct obj *,int);
extern int welded (struct obj *);
extern void weldmsg (struct obj *);
extern void setmnotwielded (struct monst *,struct obj *);
extern boolean mwelded (struct obj *);



extern void choose_windows (const char *);




extern boolean genl_can_suspend_no(void);
extern boolean genl_can_suspend_yes(void);
extern char genl_message_menu (int,int,const char *);
extern void genl_preference_update (const char *);
extern char *genl_getmsghistory (int);
extern void genl_putmsghistory (const char *,int);

extern void nhwindows_hangup(void);




extern void amulet(void);
extern int mon_has_amulet (struct monst *);
extern int mon_has_special (struct monst *);
extern int tactics (struct monst *);
extern void aggravate(void);
extern void clonewiz(void);
extern int pick_nasty(void);
extern int nasty (struct monst*);
extern void resurrect(void);
extern void intervene(void);
extern void wizdead(void);
extern void cuss (struct monst *);



extern int get_wormno(void);
extern void initworm (struct monst *,int);
extern void worm_move (struct monst *);
extern void worm_nomove (struct monst *);
extern void wormgone (struct monst *);
extern void wormhitu (struct monst *);
extern void cutworm (struct monst *,int,int,struct obj *);
extern void see_wsegs (struct monst *);
extern void detect_wsegs (struct monst *,int);
extern void save_worm (int,int);
extern void rest_worm (int);
extern void place_wsegs (struct monst *);
extern void remove_worm (struct monst *);
extern void place_worm_tail_randomly (struct monst *,int,int);
extern int count_wsegs (struct monst *);
extern boolean worm_known (struct monst *);
extern boolean worm_cross (int,int,int,int);



extern void setworn (struct obj *,long);
extern void setnotworn (struct obj *);
extern long wearslot (struct obj *);
extern void mon_set_minvis (struct monst *);
extern void mon_adjust_speed (struct monst *,int,struct obj *);
extern void update_mon_intrinsics (struct monst *,struct obj *,int,int);

extern int find_mac (struct monst *);
extern void m_dowear (struct monst *,int);
extern struct obj *which_armor (struct monst *,long);
extern void mon_break_armor (struct monst *,int);
extern void bypass_obj (struct obj *);
extern void clear_bypasses(void);
extern void bypass_objlist (struct obj *,int);
extern struct obj *nxt_unbypassed_obj (struct obj *);
extern int racial_exception (struct monst *, struct obj *);



extern int dowrite (struct obj *);



extern void learnwand (struct obj *);
extern int bhitm (struct monst *,struct obj *);
extern void probe_monster (struct monst *);
extern boolean get_obj_location (struct obj *,xchar *,xchar *,int);
extern boolean get_mon_location (struct monst *,xchar *,xchar *,int);
extern struct monst *get_container_location (struct obj *obj, int *, int *);
extern struct monst *montraits (struct obj *,coord *);
extern struct monst *revive (struct obj *,int);
extern int unturn_dead (struct monst *);
extern void cancel_item (struct obj *);
extern boolean drain_item (struct obj *);
extern struct obj *poly_obj (struct obj *, int);
extern boolean obj_resists (struct obj *,int,int);
extern boolean obj_shudders (struct obj *);
extern void do_osshock (struct obj *);
extern int bhito (struct obj *,struct obj *);
extern int bhitpile (struct obj *,int (*)(struct obj *,struct obj *),int,int,int);
extern int zappable (struct obj *);
extern void zapnodir (struct obj *);
extern int dozap(void);
extern int zapyourself (struct obj *,int);
extern void ubreatheu (struct attack *);
extern int lightdamage (struct obj *,int,int);
extern boolean flashburn (long);
extern boolean cancel_monst (struct monst *,struct obj *, int,int,int);

extern void zapsetup(void);
extern void zapwrapup(void);
extern void weffects (struct obj *);
extern int spell_damage_bonus (int);
extern const char *exclam (int force);
extern void hit (const char *,struct monst *,const char *);
extern void miss (const char *,struct monst *);
extern struct monst *bhit (int,int,int,int,int (*)(struct monst *,struct obj *), int (*)(struct obj *,struct obj *),struct obj **);

extern struct monst *boomhit (struct obj *,int,int);
extern int zhitm (struct monst *,int,int,struct obj **);
extern int burn_floor_objects (int,int,int,int);
extern void buzz (int,int,int,int,int,int);
extern void melt_ice (int,int,const char *);
extern void start_melt_ice_timeout (int,int,long);
extern void melt_ice_away (union any *, long);
extern int zap_over_floor (int,int,int,boolean *,int);
extern void fracture_rock (struct obj *);
extern boolean break_statue (struct obj *);
extern void destroy_item (int,int);
extern int destroy_mitem (struct monst *,int,int);
extern int resist (struct monst *,int,int,int);
extern void makewish(void);
# 178 "../include/hack.h" 2
# 1 "../include/winprocs.h" 1
# 9 "../include/winprocs.h"
# 1 "../include/botl.h" 1
# 62 "../include/botl.h"
extern const char *status_fieldnames[];
# 10 "../include/winprocs.h" 2


struct window_procs {
    const char *name;


    unsigned long wincap;
    unsigned long wincap2;
    void (*win_init_nhwindows) (int *, char **);
    void (*win_player_selection)(void);
    void (*win_askname)(void);
    void (*win_get_nh_event)(void) ;
    void (*win_exit_nhwindows) (const char *);
    void (*win_suspend_nhwindows) (const char *);
    void (*win_resume_nhwindows)(void);
    winid (*win_create_nhwindow) (int);
    void (*win_clear_nhwindow) (winid);
    void (*win_display_nhwindow) (winid, int);
    void (*win_destroy_nhwindow) (winid);
    void (*win_curs) (winid,int,int);
    void (*win_putstr) (winid, int, const char *);
    void (*win_putmixed) (winid, int, const char *);
    void (*win_display_file) (const char *, int);
    void (*win_start_menu) (winid);
    void (*win_add_menu) (winid,int,const union any *, int,int,int,const char *, int);

    void (*win_end_menu) (winid, const char *);
    int (*win_select_menu) (winid, int, struct mi **);
    char (*win_message_menu) (int,int,const char *);
    void (*win_update_inventory)(void);
    void (*win_mark_synch)(void);
    void (*win_wait_synch)(void);

    void (*win_cliparound) (int, int);




    void (*win_print_glyph) (winid,int,int,int);
    void (*win_raw_print) (const char *);
    void (*win_raw_print_bold) (const char *);
    int (*win_nhgetch)(void);
    int (*win_nh_poskey) (int *, int *, int *);
    void (*win_nhbell)(void);
    int (*win_doprev_message)(void);
    char (*win_yn_function) (const char *, const char *, int);
    void (*win_getlin) (const char *,char *);
    int (*win_get_ext_cmd)(void);
    void (*win_number_pad) (int);
    void (*win_delay_output)(void);
# 70 "../include/winprocs.h"
    void (*win_start_screen)(void);
    void (*win_end_screen)(void);

    void (*win_outrip) (winid,int,time_t);
    void (*win_preference_update) (const char *);
    char * (*win_getmsghistory) (int);
    void (*win_putmsghistory) (const char *,int);

    void (*win_status_init)(void);
    void (*win_status_finish)(void);
    void (*win_status_enablefield) (int,const char *,const char *,int);
    void (*win_status_update) (int,genericptr_t,int,int);

    void (*win_status_threshold) (int,int,anything,int,int,int);


    boolean (*win_can_suspend)(void);
};

extern

volatile

 struct window_procs windowprocs;
# 258 "../include/winprocs.h"
struct wc_Opt {
 const char *wc_name;
 unsigned long wc_bit;
};
# 179 "../include/hack.h" 2
# 1 "../include/sys.h" 1
# 9 "../include/sys.h"
struct sysopt {
 char *support;
 char *recover;
 char *wizards;
 char *explorers;
 char *shellers;
 char *debugfiles;
 int env_dbgfl;




 int maxplayers;

 int persmax;
 int pers_is_uid;
 int entrymax;
 int pointsmin;
 int tt_oname_maxrank;


 char *gdbpath;
 char *greppath;
 int panictrace_gdb;

 int panictrace_libc;


 int seduce;
 int check_save_uid;
};

extern struct sysopt sysopt;
# 180 "../include/hack.h" 2
# 15 "lev_main.c" 2
# 1 "../include/date.h" 1
# 16 "lev_main.c" 2
# 1 "../include/sp_lev.h" 1
# 57 "../include/sp_lev.h"
enum opcode_defs {
    SPO_NULL = 0,
    SPO_MESSAGE,
    SPO_MONSTER,
    SPO_OBJECT,
    SPO_ENGRAVING,
    SPO_ROOM,
    SPO_SUBROOM,
    SPO_DOOR,
    SPO_STAIR,
    SPO_LADDER,
    SPO_ALTAR,
    SPO_FOUNTAIN,
    SPO_SINK,
    SPO_POOL,
    SPO_TRAP,
    SPO_GOLD,
    SPO_CORRIDOR,
    SPO_LEVREGION,
    SPO_DRAWBRIDGE,
    SPO_MAZEWALK,
    SPO_NON_DIGGABLE,
    SPO_NON_PASSWALL,
    SPO_WALLIFY,
    SPO_MAP,
    SPO_ROOM_DOOR,
    SPO_REGION,
    SPO_MINERALIZE,
    SPO_CMP,
    SPO_JMP,
    SPO_JL,
    SPO_JLE,
    SPO_JG,
    SPO_JGE,
    SPO_JE,
    SPO_JNE,
    SPO_TERRAIN,
    SPO_REPLACETERRAIN,
    SPO_EXIT,
    SPO_ENDROOM,
    SPO_POP_CONTAINER,
    SPO_PUSH,
    SPO_POP,
    SPO_RN2,
    SPO_DEC,
    SPO_INC,
    SPO_MATH_ADD,
    SPO_MATH_SUB,
    SPO_MATH_MUL,
    SPO_MATH_DIV,
    SPO_MATH_MOD,
    SPO_MATH_SIGN,
    SPO_COPY,
    SPO_END_MONINVENT,
    SPO_GRAVE,
    SPO_FRAME_PUSH,
    SPO_FRAME_POP,
    SPO_CALL,
    SPO_RETURN,
    SPO_INITLEVEL,
    SPO_LEVEL_FLAGS,
    SPO_VAR_INIT,
    SPO_SHUFFLE_ARRAY,
    SPO_DICE,

    SPO_SEL_ADD,
    SPO_SEL_POINT,
    SPO_SEL_RECT,
    SPO_SEL_FILLRECT,
    SPO_SEL_LINE,
    SPO_SEL_RNDLINE,
    SPO_SEL_GROW,
    SPO_SEL_FLOOD,
    SPO_SEL_RNDCOORD,
    SPO_SEL_ELLIPSE,
    SPO_SEL_FILTER,
    SPO_SEL_GRADIENT,
    SPO_SEL_COMPLEMENT,

    MAX_SP_OPCODES
};
# 246 "../include/sp_lev.h"
struct opvar {
    xchar spovartyp;
    union {
 char *str;
 long l;
    } vardata;
};

struct splev_var {
    struct splev_var *next;
    char *name;
    xchar svtyp;
    union {
 struct opvar *value;
 struct opvar **arrayvalues;
    } data;
    long array_len;
};

struct splevstack {
    long depth;
    long depth_alloc;
    struct opvar **stackdata;
};


struct sp_frame {
    struct sp_frame *next;
    struct splevstack *stack;
    struct splev_var *variables;
    long n_opcode;
};


struct sp_coder {
    struct splevstack *stack;
    struct sp_frame *frame;
    int premapped;
    boolean solidify;
    struct mkroom *croom;
    struct mkroom *tmproomlist[5 +1];
    boolean failed_room[5 +1];
    int n_subroom;
    boolean exit_script;
    int lvl_is_joined;

    int opcode;
    struct opvar *opdat;
};
# 307 "../include/sp_lev.h"
typedef struct {
    xchar is_random;
    long getloc_flags;
    int x, y;
} unpacked_coord;

typedef struct {
 int cmp_what;
 int cmp_val;
} opcmp;

typedef struct {
 long jmp_target;
} opjmp;


typedef union str_or_len {
 char *str;
 int len;
} Str_or_Len;

typedef struct {
 xchar init_style;
 long flags;
 schar filling;
 boolean init_present, padding;
 char fg, bg;
 boolean smoothed, joined;
 xchar lit, walled;
 boolean icedpools;
} lev_init;

typedef struct {
 xchar wall, pos, secret, mask;
} room_door;

typedef struct {
    long coord;
    xchar x, y, type;
} trap;

typedef struct {
 Str_or_Len name, appear_as;
 short id;
 aligntyp align;
    long coord;
 xchar x, y, class, appear;
 schar peaceful, asleep;
        short female, invis, cancelled, revived, avenge, fleeing, blinded, paralyzed, stunned, confused;
        long seentraps;
 short has_invent;
} monster;

typedef struct {
 Str_or_Len name;
 int corpsenm;
 short id, spe;
    long coord;
 xchar x, y, class, containment;
 schar curse_state;
 int quan;
 short buried;
 short lit;
        short eroded, locked, trapped, recharged, invis, greased, broken;
} object;

typedef struct {
    long coord;
 xchar x, y;
 aligntyp align;
 xchar shrine;
} altar;

typedef struct {
 xchar x1, y1, x2, y2;
 xchar rtype, rlit, rirreg;
} region;

typedef struct {
    xchar ter, tlit;
} terrain;

typedef struct {
    xchar chance;
    xchar x1,y1,x2,y2;
    xchar fromter, toter, tolit;
} replaceterrain;


typedef struct {
 struct { xchar x1, y1, x2, y2; } inarea;
 struct { xchar x1, y1, x2, y2; } delarea;
 boolean in_islev, del_islev;
 xchar rtype, padding;
 Str_or_Len rname;
} lev_region;

typedef struct {
 struct {
  xchar room;
  xchar wall;
  xchar door;
 } src, dest;
} corridor;

typedef struct _room {
 Str_or_Len name;
 Str_or_Len parent;
 xchar x, y, w, h;
 xchar xalign, yalign;
 xchar rtype, chance, rlit, filled, joined;
} room;

typedef struct {
 schar zaligntyp;
 schar keep_region;
 schar halign, valign;
 char xsize, ysize;
 char **map;
} mazepart;

typedef struct {
    int opcode;
    struct opvar *opdat;
} _opcode;

typedef struct {
    _opcode *opcodes;
    long n_opcodes;
} sp_lev;

typedef struct {
 xchar x, y, direction, count, lit;
 char typ;
} spill;



struct lc_funcdefs_parm {
    char *name;
    char parmtype;
    struct lc_funcdefs_parm *next;
};

struct lc_funcdefs {
    struct lc_funcdefs *next;
    char *name;
    long addr;
    sp_lev code;
    long n_called;
    struct lc_funcdefs_parm *params;
    long n_params;
};

struct lc_vardefs {
    struct lc_vardefs *next;
    char *name;
    long var_type;
    long n_used;
};

struct lc_breakdef {
    struct lc_breakdef *next;
    struct opvar *breakpoint;
    int break_depth;
};
# 17 "lev_main.c" 2



# 1 "/usr/include/ctype.h" 1 3 4
# 69 "/usr/include/ctype.h" 3 4
# 1 "/usr/include/runetype.h" 1 3 4
# 70 "/usr/include/runetype.h" 3 4
typedef __darwin_wint_t wint_t;
# 81 "/usr/include/runetype.h" 3 4
typedef struct {
 __darwin_rune_t __min;
 __darwin_rune_t __max;
 __darwin_rune_t __map;
 __uint32_t *__types;
} _RuneEntry;

typedef struct {
 int __nranges;
 _RuneEntry *__ranges;
} _RuneRange;

typedef struct {
 char __name[14];
 __uint32_t __mask;
} _RuneCharClass;

typedef struct {
 char __magic[8];
 char __encoding[32];

 __darwin_rune_t (*__sgetrune)(const char *, __darwin_size_t, char const **);
 int (*__sputrune)(__darwin_rune_t, char *, __darwin_size_t, char **);
 __darwin_rune_t __invalid_rune;

 __uint32_t __runetype[(1 <<8 )];
 __darwin_rune_t __maplower[(1 <<8 )];
 __darwin_rune_t __mapupper[(1 <<8 )];






 _RuneRange __runetype_ext;
 _RuneRange __maplower_ext;
 _RuneRange __mapupper_ext;

 void *__variable;
 int __variable_len;




 int __ncharclasses;
 _RuneCharClass *__charclasses;
} _RuneLocale;




extern _RuneLocale _DefaultRuneLocale;
extern _RuneLocale *_CurrentRuneLocale;

# 70 "/usr/include/ctype.h" 2 3 4
# 145 "/usr/include/ctype.h" 3 4

unsigned long ___runetype(__darwin_ct_rune_t);
__darwin_ct_rune_t ___tolower(__darwin_ct_rune_t);
__darwin_ct_rune_t ___toupper(__darwin_ct_rune_t);


static __inline int
isascii(int _c)
{
 return ((_c & ~0x7F) == 0);
}
# 164 "/usr/include/ctype.h" 3 4

int __maskrune(__darwin_ct_rune_t, unsigned long);



static __inline int
__istype(__darwin_ct_rune_t _c, unsigned long _f)
{



 return (isascii(_c) ? !!(_DefaultRuneLocale.__runetype[_c] & _f)
  : !!__maskrune(_c, _f));

}

static __inline __darwin_ct_rune_t
__isctype(__darwin_ct_rune_t _c, unsigned long _f)
{



 return (_c < 0 || _c >= (1 <<8 )) ? 0 :
  !!(_DefaultRuneLocale.__runetype[_c] & _f);

}
# 204 "/usr/include/ctype.h" 3 4

__darwin_ct_rune_t __toupper(__darwin_ct_rune_t);
__darwin_ct_rune_t __tolower(__darwin_ct_rune_t);



static __inline int
__wcwidth(__darwin_ct_rune_t _c)
{
 unsigned int _x;

 if (_c == 0)
  return (0);
 _x = (unsigned int)__maskrune(_c, 0xe0000000L|0x00040000L);
 if ((_x & 0xe0000000L) != 0)
  return ((_x & 0xe0000000L) >> 30);
 return ((_x & 0x00040000L) != 0 ? 1 : -1);
}






static __inline int
isalnum(int _c)
{
 return (__istype(_c, 0x00000100L|0x00000400L));
}

static __inline int
isalpha(int _c)
{
 return (__istype(_c, 0x00000100L));
}

static __inline int
isblank(int _c)
{
 return (__istype(_c, 0x00020000L));
}

static __inline int
iscntrl(int _c)
{
 return (__istype(_c, 0x00000200L));
}


static __inline int
isdigit(int _c)
{
 return (__isctype(_c, 0x00000400L));
}

static __inline int
isgraph(int _c)
{
 return (__istype(_c, 0x00000800L));
}

static __inline int
islower(int _c)
{
 return (__istype(_c, 0x00001000L));
}

static __inline int
isprint(int _c)
{
 return (__istype(_c, 0x00040000L));
}

static __inline int
ispunct(int _c)
{
 return (__istype(_c, 0x00002000L));
}

static __inline int
isspace(int _c)
{
 return (__istype(_c, 0x00004000L));
}

static __inline int
isupper(int _c)
{
 return (__istype(_c, 0x00008000L));
}


static __inline int
isxdigit(int _c)
{
 return (__isctype(_c, 0x00010000L));
}

static __inline int
toascii(int _c)
{
 return (_c & 0x7F);
}

static __inline int
tolower(int _c)
{
        return (__tolower(_c));
}

static __inline int
toupper(int _c)
{
        return (__toupper(_c));
}


static __inline int
digittoint(int _c)
{
 return (__maskrune(_c, 0x0F));
}

static __inline int
ishexnumber(int _c)
{
 return (__istype(_c, 0x00010000L));
}

static __inline int
isideogram(int _c)
{
 return (__istype(_c, 0x00080000L));
}

static __inline int
isnumber(int _c)
{
 return (__istype(_c, 0x00000400L));
}

static __inline int
isphonogram(int _c)
{
 return (__istype(_c, 0x00200000L));
}

static __inline int
isrune(int _c)
{
 return (__istype(_c, 0xFFFFFFF0L));
}

static __inline int
isspecial(int _c)
{
 return (__istype(_c, 0x00100000L));
}
# 21 "lev_main.c" 2
# 47 "lev_main.c"
# 1 "/usr/include/fcntl.h" 1 3 4
# 23 "/usr/include/fcntl.h" 3 4
# 1 "/usr/include/sys/fcntl.h" 1 3 4
# 305 "/usr/include/sys/fcntl.h" 3 4
struct flock {
 off_t l_start;
 off_t l_len;
 pid_t l_pid;
 short l_type;
 short l_whence;
};







struct radvisory {
       off_t ra_offset;
       int ra_count;
};






typedef struct fsignatures {
 off_t fs_file_start;
 void *fs_blob_start;
 size_t fs_blob_size;
} fsignatures_t;
# 343 "/usr/include/sys/fcntl.h" 3 4
typedef struct fstore {
 unsigned int fst_flags;
 int fst_posmode;
 off_t fst_offset;
 off_t fst_length;
 off_t fst_bytesalloc;
} fstore_t;



typedef struct fbootstraptransfer {
  off_t fbt_offset;
  size_t fbt_length;
  void *fbt_buffer;
} fbootstraptransfer_t;
# 377 "/usr/include/sys/fcntl.h" 3 4
#pragma pack(4)

struct log2phys {
 unsigned int l2p_flags;
 off_t l2p_contigbytes;
 off_t l2p_devoffset;
};

#pragma pack()
# 396 "/usr/include/sys/fcntl.h" 3 4
struct _filesec;
typedef struct _filesec *filesec_t;


typedef enum {
 FILESEC_OWNER = 1,
 FILESEC_GROUP = 2,
 FILESEC_UUID = 3,
 FILESEC_MODE = 4,
 FILESEC_ACL = 5,
 FILESEC_GRPUUID = 6,


 FILESEC_ACL_RAW = 100,
 FILESEC_ACL_ALLOCSIZE = 101
} filesec_property_t;






int open(const char *, int, ...) __asm("_" "open" "$UNIX2003");
int creat(const char *, mode_t) __asm("_" "creat" "$UNIX2003");
int fcntl(int, int, ...) __asm("_" "fcntl" "$UNIX2003");

int openx_np(const char *, int, filesec_t);
int flock(int, int);
filesec_t filesec_init(void);
filesec_t filesec_dup(filesec_t);
void filesec_free(filesec_t);
int filesec_get_property(filesec_t, filesec_property_t, void *);
int filesec_set_property(filesec_t, filesec_property_t, const void *);
int filesec_query_property(filesec_t, filesec_property_t, int *);




# 23 "/usr/include/fcntl.h" 2 3 4
# 48 "lev_main.c" 2
# 82 "lev_main.c"
extern int yyparse(void);
extern void init_yyin (FILE *);
extern void init_yyout (FILE *);

int main (int, char **);
void yyerror (const char *);
void yywarning (const char *);
int yywrap(void);
int get_floor_type (int);
int get_room_type (char *);
int get_trap_type (char *);
int get_monster_id (char *, int);
int get_object_id (char *, int);
boolean check_monster_char (int);
boolean check_object_char (int);
char what_map_char (int);
void scan_map (char *, sp_lev *);
boolean check_subrooms(void);
boolean write_level_file (char *, sp_lev *);

struct lc_funcdefs *funcdef_new (long, char *);
void funcdef_free_all (struct lc_funcdefs *);
struct lc_funcdefs *funcdef_defined (struct lc_funcdefs *, char *, int);


struct lc_vardefs *vardef_new (long, char *);
void vardef_free_all (struct lc_vardefs *);
struct lc_vardefs *vardef_defined (struct lc_vardefs *, char *, int);

void splev_add_from (sp_lev *, sp_lev *);

extern void monst_init(void);
extern void objects_init(void);
extern void decl_init(void);

void add_opcode (sp_lev *, int, genericptr_t);

static boolean write_common_data (int);
static boolean write_maze (int, sp_lev *);
static void init_obj_classes(void);
static int case_insensitive_comp (const char *, const char *);

void lc_pline (const char *, ...);
void lc_error (const char *, ...);
void lc_warning (const char *, ...);
char *decode_parm_chr (int);
char *decode_parm_str (char *);
struct opvar *set_opvar_int (struct opvar *, long);
struct opvar *set_opvar_coord (struct opvar *, long);
struct opvar *set_opvar_region (struct opvar *, long);
struct opvar *set_opvar_mapchar (struct opvar *, long);
struct opvar *set_opvar_monst (struct opvar *, long);
struct opvar *set_opvar_obj (struct opvar *, long);
struct opvar *set_opvar_str (struct opvar *, const char *);
struct opvar *set_opvar_var (struct opvar *, const char *);
void add_opvars (sp_lev *, const char *, ...);
void break_stmt_start(void);
void break_stmt_end (sp_lev *);
void break_stmt_new (sp_lev *, long);
char *funcdef_paramtypes (struct lc_funcdefs *);
const char *spovar2str (long);
void vardef_used (struct lc_vardefs *, char *);
void check_vardef_type (struct lc_vardefs *, char *, long);
struct lc_vardefs *add_vardef_type (struct lc_vardefs *, char *, long);

int reverse_jmp_opcode (int);
struct opvar *opvar_clone (struct opvar *);
void start_level_def (sp_lev **, char *);

static struct {
    const char *name;
    int type;
} trap_types[] = { { "arrow", 1 },
                   { "dart", 2 },
                   { "falling rock", 3 },
                   { "board", 4 },
                   { "bear", 5 },
                   { "land mine", 6 },
                   { "rolling boulder", 7 },
                   { "sleep gas", 8 },
                   { "rust", 9 },
                   { "fire", 10 },
                   { "pit", 11 },
                   { "spiked pit", 12 },
                   { "hole", 13 },
                   { "trap door", 14 },
                   { "teleport", 15 },
                   { "level teleport", 16 },
                   { "magic portal", 17 },
                   { "web", 18 },
                   { "statue", 19 },
                   { "magic", 20 },
                   { "anti magic", 21 },
                   { "polymorph", 22 },
                   { 0, 0 } };

static struct {
    const char *name;
    int type;
} room_types[] = {


    { "ordinary", 0 },
    { "throne", 2 },
    { "swamp", 3 },
    { "vault", 4 },
    { "beehive", 5 },
    { "morgue", 6 },
    { "barracks", 7 },
    { "zoo", 8 },
    { "delphi", 9 },
    { "temple", 10 },
    { "anthole", 13 },
    { "cocknest", 12 },
    { "leprehall", 11 },
    { "shop", 14 },
    { "armor shop", 15 },
    { "scroll shop", 16 },
    { "potion shop", 17 },
    { "weapon shop", 18 },
    { "food shop", 19 },
    { "ring shop", 20 },
    { "wand shop", 21 },
    { "tool shop", 22 },
    { "book shop", 23 },
    { "health food shop", 24 },
    { "candle shop", 25 },
    { 0, 0 }
};

const char *fname = "(stdin)";
int fatal_error = 0;
int got_errors = 0;
int be_verbose = 0;
int fname_counter = 1;






extern unsigned int max_x_map, max_y_map;

extern int nh_line_number;

extern int token_start_pos;
extern char curr_token[512];

struct lc_vardefs *variable_definitions = ((void *)0);
struct lc_funcdefs *function_definitions = ((void *)0);

extern int allow_break_statements;
extern struct lc_breakdef *break_list;

int
main(argc, argv)
int argc;
char **argv;
{
    FILE *fin;
    int i;
    boolean errors_encountered = ((boolean)0);
# 263 "lev_main.c"
    monst_init();
    objects_init();
    decl_init();

    init_obj_classes();

    init_yyout(__stdoutp);
    if (argc == 1) {
        init_yyin(__stdinp);
        (void) yyparse();
        if (fatal_error > 0) {
            errors_encountered = ((boolean)1);
        }
    } else {
        for (i = 1; i < argc; i++) {
            fname = argv[i];
            if (!strcmp(fname, "-v")) {
                be_verbose++;
                continue;
            }
            fin = freopen(fname, "r", __stdinp);
            if (!fin) {
                lc_pline("Can't open \"%s\" for input.\n", fname);
                perror(fname);
                errors_encountered = ((boolean)1);
            } else {
                fname_counter = 1;
                init_yyin(fin);
                (void) yyparse();
                nh_line_number = 1;
                if (fatal_error > 0 || got_errors > 0) {
                    errors_encountered = ((boolean)1);
                    fatal_error = 0;
                }
            }
        }
    }
    exit(errors_encountered ? 1 : 0);

    return 0;
}
# 313 "lev_main.c"
void
yyerror(s)
const char *s;
{
    char *e = ((char *) s + strlen(s) - 1);

    (void) fprintf(__stderrp, "%s: line %d, pos %d: %s", fname, nh_line_number,
                   token_start_pos - (int) strlen(curr_token), s);
    if (*e != '.' && *e != '!')
        (void) fprintf(__stderrp, " at \"%s\"", curr_token);
    (void) fprintf(__stderrp, "\n");

    if (++fatal_error > 25) {
        (void) fprintf(__stderrp, "Too many errors, good bye!\n");
        exit(1);
    }
}




void
yywarning(s)
const char *s;
{
    (void) fprintf(__stderrp, "%s: line %d : WARNING : %s\n", fname,
                   nh_line_number, s);
}




int
yywrap()
{
    return 1;
}
# 361 "lev_main.c"
static int lc_pline_mode = 0;


static void lc_vpline (const char *, va_list);

void lc_pline
(const char * line, ...) { va_list the_args;
{
    __builtin_va_start(the_args,line);
    ;
    lc_vpline(line, the_args);
    __builtin_va_end(the_args); };
}


static void
lc_vpline(const char *line, va_list the_args)
# 392 "lev_main.c"
{

    char pbuf[3 * 256];
    static char nomsg[] = "(no message)";


    if (!line || !*line)
        line = nomsg;
    if (strchr(line, '%')) {
        (void) vsprintf(pbuf, line, the_args);
        pbuf[256 - 1] = '\0';
        line = pbuf;
    }
    switch (lc_pline_mode) {
    case 2:
        yyerror(line);
        break;
    case 1:
        yywarning(line);
        break;
    default:
        (void) fprintf(__stderrp, "%s\n", line);
        break;
    }
    lc_pline_mode = 0;
    return;



}


void lc_error
(const char * line, ...) { va_list the_args;
{
    __builtin_va_start(the_args,line);
    ;
    lc_pline_mode = 2;
    lc_vpline(line, the_args);
    __builtin_va_end(the_args); };
    return;
}


void lc_warning
(const char * line, ...) { va_list the_args;
{
    __builtin_va_start(the_args,line);
    ;
    lc_pline_mode = 1;
    lc_vpline(line, the_args);
    __builtin_va_end(the_args); };
    return;
}

char *
decode_parm_chr(chr)
char chr;
{
    static char buf[32];

    switch (chr) {
    default:
        (void) strcpy(buf, "unknown");
        break;
    case 'i':
        (void) strcpy(buf, "int");
        break;
    case 'r':
        (void) strcpy(buf, "region");
        break;
    case 's':
        (void) strcpy(buf, "str");
        break;
    case 'O':
        (void) strcpy(buf, "obj");
        break;
    case 'c':
        (void) strcpy(buf, "coord");
        break;
    case ' ':
        (void) strcpy(buf, "nothing");
        break;
    case 'm':
        (void) strcpy(buf, "mapchar");
        break;
    case 'M':
        (void) strcpy(buf, "monster");
        break;
    }
    return buf;
}

char *
decode_parm_str(str)
char *str;
{
    static char tmpbuf[1024];
    char *p = str;
    tmpbuf[0] = '\0';
    if (str) {
        for (; *p; p++) {
            (void) strcat(tmpbuf, decode_parm_chr(*p));
            if (*(p + 1))
                (void) strcat(tmpbuf, ", ");
        }
    }
    return tmpbuf;
}

struct opvar *
set_opvar_int(ov, val)
struct opvar *ov;
long val;
{
    if (ov) {
        ov->spovartyp = 0x01;
        ov->vardata.l = val;
    }
    return ov;
}

struct opvar *
set_opvar_coord(ov, val)
struct opvar *ov;
long val;
{
    if (ov) {
        ov->spovartyp = 0x04;
        ov->vardata.l = val;
    }
    return ov;
}

struct opvar *
set_opvar_region(ov, val)
struct opvar *ov;
long val;
{
    if (ov) {
        ov->spovartyp = 0x05;
        ov->vardata.l = val;
    }
    return ov;
}

struct opvar *
set_opvar_mapchar(ov, val)
struct opvar *ov;
long val;
{
    if (ov) {
        ov->spovartyp = 0x06;
        ov->vardata.l = val;
    }
    return ov;
}

struct opvar *
set_opvar_monst(ov, val)
struct opvar *ov;
long val;
{
    if (ov) {
        ov->spovartyp = 0x07;
        ov->vardata.l = val;
    }
    return ov;
}

struct opvar *
set_opvar_obj(ov, val)
struct opvar *ov;
long val;
{
    if (ov) {
        ov->spovartyp = 0x08;
        ov->vardata.l = val;
    }
    return ov;
}

struct opvar *
set_opvar_str(ov, val)
struct opvar *ov;
const char *val;
{
    if (ov) {
        ov->spovartyp = 0x02;
        ov->vardata.str = (val) ? strdup(val) : ((void *)0);
    }
    return ov;
}

struct opvar *
set_opvar_var(ov, val)
struct opvar *ov;
const char *val;
{
    if (ov) {
        ov->spovartyp = 0x03;
        ov->vardata.str = (val) ? strdup(val) : ((void *)0);
    }
    return ov;
}





static void vadd_opvars (sp_lev *, const char *, va_list);

void add_opvars
(sp_lev * sp, const char * fmt, ...) { va_list the_args;
    __builtin_va_start(the_args,fmt);
    ;
    vadd_opvars(sp, fmt, the_args);
    __builtin_va_end(the_args); };
}


static void
vadd_opvars(sp_lev *sp, const char *fmt, va_list the_args)
{
# 633 "lev_main.c"
    const char *p, *lp;
    long la;


    for (p = fmt; *p != '\0'; p++) {
        switch (*p) {
        case ' ':
            break;
        case 'i':
        {
            struct opvar *ov = (struct opvar *) memset((genericptr_t) alloc(sizeof(struct opvar)), 0, sizeof(struct opvar));
            set_opvar_int(ov, la = __builtin_va_arg(the_args,long) );
            add_opcode(sp, SPO_PUSH, ov);
            break;
        }
        case 'c':
        {
            struct opvar *ov = (struct opvar *) memset((genericptr_t) alloc(sizeof(struct opvar)), 0, sizeof(struct opvar));
            set_opvar_coord(ov, la = __builtin_va_arg(the_args,long) );
            add_opcode(sp, SPO_PUSH, ov);
            break;
        }
        case 'r':
        {
            struct opvar *ov = (struct opvar *) memset((genericptr_t) alloc(sizeof(struct opvar)), 0, sizeof(struct opvar));
            set_opvar_region(ov, la = __builtin_va_arg(the_args,long) );
            add_opcode(sp, SPO_PUSH, ov);
            break;
        }
        case 'm':
        {
            struct opvar *ov = (struct opvar *) memset((genericptr_t) alloc(sizeof(struct opvar)), 0, sizeof(struct opvar));
            set_opvar_mapchar(ov, la = __builtin_va_arg(the_args,long) );
            add_opcode(sp, SPO_PUSH, ov);
            break;
        }
        case 'M':
        {
            struct opvar *ov = (struct opvar *) memset((genericptr_t) alloc(sizeof(struct opvar)), 0, sizeof(struct opvar));
            set_opvar_monst(ov, la = __builtin_va_arg(the_args,long) );
            add_opcode(sp, SPO_PUSH, ov);
            break;
        }
        case 'O':
        {
            struct opvar *ov = (struct opvar *) memset((genericptr_t) alloc(sizeof(struct opvar)), 0, sizeof(struct opvar));
            set_opvar_obj(ov, la = __builtin_va_arg(the_args,long) );
            add_opcode(sp, SPO_PUSH, ov);
            break;
        }
        case 's':
        {
            struct opvar *ov = (struct opvar *) memset((genericptr_t) alloc(sizeof(struct opvar)), 0, sizeof(struct opvar));
            set_opvar_str(ov, lp = __builtin_va_arg(the_args,const char *) );
            add_opcode(sp, SPO_PUSH, ov);
            break;
        }
        case 'v':
        {
            struct opvar *ov = (struct opvar *) memset((genericptr_t) alloc(sizeof(struct opvar)), 0, sizeof(struct opvar));
            set_opvar_var(ov, lp = __builtin_va_arg(the_args,const char *) );
            add_opcode(sp, SPO_PUSH, ov);
            break;
        }
        case 'o':
        {
            long i = la = __builtin_va_arg(the_args,int);
            if (i < 0 || i >= MAX_SP_OPCODES)
                lc_pline("add_opvars: unknown opcode '%ld'.", i);
            add_opcode(sp, i, ((void *)0));
            break;
        }
        default:
            lc_pline("add_opvars: illegal format character '%c'.", *p);
            break;
        }
    }
    return;
}

void
break_stmt_start()
{
    allow_break_statements++;
}

void
break_stmt_end(splev)
sp_lev *splev;
{
    struct lc_breakdef *tmp = break_list;
    struct lc_breakdef *prv = ((void *)0);
    while (tmp) {
        if (tmp->break_depth == allow_break_statements) {
            struct lc_breakdef *nxt = tmp->next;
            set_opvar_int(tmp->breakpoint,
                          splev->n_opcodes - tmp->breakpoint->vardata.l - 1);
            tmp->next = ((void *)0);
            if (tmp) free((genericptr_t)(tmp));
            if (!prv)
                break_list = ((void *)0);
            else
                prv->next = nxt;
            tmp = nxt;
        } else {
            prv = tmp;
            tmp = tmp->next;
        }
    }
    allow_break_statements--;
}

void
break_stmt_new(splev, i)
sp_lev *splev;
long i;
{
    struct lc_breakdef *tmp = (struct lc_breakdef *) memset((genericptr_t) alloc(sizeof(struct lc_breakdef)), 0, sizeof(struct lc_breakdef));
    tmp->breakpoint = (struct opvar *) memset((genericptr_t) alloc(sizeof(struct opvar)), 0, sizeof(struct opvar));
    tmp->break_depth = allow_break_statements;
    tmp->next = break_list;
    break_list = tmp;
    set_opvar_int(tmp->breakpoint, i);
    add_opcode(splev, SPO_PUSH, tmp->breakpoint);
    add_opcode(splev, SPO_JMP, ((void *)0));
}

struct lc_funcdefs *
funcdef_new(addr, name)
long addr;
char *name;
{
    struct lc_funcdefs *f = (struct lc_funcdefs *) memset((genericptr_t) alloc(sizeof(struct lc_funcdefs)), 0, sizeof(struct lc_funcdefs));
    if (!f) {
        lc_error("Could not alloc function definition for '%s'.", name);
        return ((void *)0);
    }
    f->next = ((void *)0);
    f->addr = addr;
    f->name = strdup(name);
    f->n_called = 0;
    f->n_params = 0;
    f->params = ((void *)0);
    f->code.opcodes = ((void *)0);
    f->code.n_opcodes = 0;
    return f;
}

void
funcdef_free_all(fchain)
struct lc_funcdefs *fchain;
{
    struct lc_funcdefs *tmp = fchain;
    struct lc_funcdefs *nxt;
    struct lc_funcdefs_parm *tmpparam;
    while (tmp) {
        nxt = tmp->next;
        if (tmp->name) free((genericptr_t)(tmp->name));
        while (tmp->params) {
            tmpparam = tmp->params->next;
            if (tmp->params->name) free((genericptr_t)(tmp->params->name));
            tmp->params = tmpparam;
        }

        if (tmp) free((genericptr_t)(tmp));
        tmp = nxt;
    }
}

char *
funcdef_paramtypes(f)
struct lc_funcdefs *f;
{
    int i = 0;
    struct lc_funcdefs_parm *fp = f->params;
    char *tmp = (char *) alloc((f->n_params) + 1);
    if (!tmp)
        return ((void *)0);
    while (fp) {
        tmp[i++] = fp->parmtype;
        fp = fp->next;
    }
    tmp[i] = '\0';
    return tmp;
}

struct lc_funcdefs *
funcdef_defined(f, name, casesense)
struct lc_funcdefs *f;
char *name;
int casesense;
{
    while (f) {
        if (casesense) {
            if (!strcmp(name, f->name))
                return f;
        } else {
            if (!case_insensitive_comp(name, f->name))
                return f;
        }
        f = f->next;
    }
    return ((void *)0);
}

struct lc_vardefs *
vardef_new(typ, name)
long typ;
char *name;
{
    struct lc_vardefs *f = (struct lc_vardefs *) memset((genericptr_t) alloc(sizeof(struct lc_vardefs)), 0, sizeof(struct lc_vardefs));
    if (!f) {
        lc_error("Could not alloc variable definition for '%s'.", name);
        return ((void *)0);
    }
    f->next = ((void *)0);
    f->var_type = typ;
    f->name = strdup(name);
    f->n_used = 0;
    return f;
}

void
vardef_free_all(fchain)
struct lc_vardefs *fchain;
{
    struct lc_vardefs *tmp = fchain;
    struct lc_vardefs *nxt;
    while (tmp) {
        if (be_verbose && (tmp->n_used == 0))
            lc_warning("Unused variable '%s'", tmp->name);
        nxt = tmp->next;
        if (tmp->name) free((genericptr_t)(tmp->name));
        if (tmp) free((genericptr_t)(tmp));
        tmp = nxt;
    }
}

struct lc_vardefs *
vardef_defined(f, name, casesense)
struct lc_vardefs *f;
char *name;
int casesense;
{
    while (f) {
        if (casesense) {
            if (!strcmp(name, f->name))
                return f;
        } else {
            if (!case_insensitive_comp(name, f->name))
                return f;
        }
        f = f->next;
    }
    return ((void *)0);
}

const char *
spovar2str(spovar)
long spovar;
{
    static int togl = 0;
    static char buf[2][128];
    const char *n = ((void *)0);
    int is_array = (spovar & 0x40);
    spovar &= ~0x40;

    switch (spovar) {
    default:
        lc_error("spovar2str(%ld)", spovar);
        break;
    case 0x01:
        n = "integer";
        break;
    case 0x02:
        n = "string";
        break;
    case 0x03:
        n = "variable";
        break;
    case 0x04:
        n = "coordinate";
        break;
    case 0x05:
        n = "region";
        break;
    case 0x06:
        n = "mapchar";
        break;
    case 0x07:
        n = "monster";
        break;
    case 0x08:
        n = "object";
        break;
    }

    togl = ((togl + 1) % 2);

    snprintf(buf[togl], 127, "%s%s", n, (is_array ? " array" : ""));
    return buf[togl];
}

void
vardef_used(vd, varname)
struct lc_vardefs *vd;
char *varname;
{
    struct lc_vardefs *tmp;
    if ((tmp = vardef_defined(vd, varname, 1)))
        tmp->n_used++;
}

void
check_vardef_type(vd, varname, vartype)
struct lc_vardefs *vd;
char *varname;
long vartype;
{
    struct lc_vardefs *tmp;
    if ((tmp = vardef_defined(vd, varname, 1))) {
        if (tmp->var_type != vartype)
            lc_error("Trying to use variable '%s' as %s, when it is %s.",
                     varname, spovar2str(vartype), spovar2str(tmp->var_type));
    } else
        lc_error("Variable '%s' not defined.", varname);
}

struct lc_vardefs *
add_vardef_type(vd, varname, vartype)
struct lc_vardefs *vd;
char *varname;
long vartype;
{
    struct lc_vardefs *tmp;
    if ((tmp = vardef_defined(vd, varname, 1))) {
        if (tmp->var_type != vartype)
            lc_error("Trying to redefine variable '%s' as %s, when it is %s.",
                     varname, spovar2str(vartype), spovar2str(tmp->var_type));
    } else {
        tmp = vardef_new(vartype, varname);
        tmp->next = vd;
        return tmp;
    }
    return vd;
}

int
reverse_jmp_opcode(opcode)
int opcode;
{
    switch (opcode) {
    case SPO_JE:
        return SPO_JNE;
    case SPO_JNE:
        return SPO_JE;
    case SPO_JL:
        return SPO_JGE;
    case SPO_JG:
        return SPO_JLE;
    case SPO_JLE:
        return SPO_JG;
    case SPO_JGE:
        return SPO_JL;
    default:
        lc_error("Cannot reverse comparison jmp opcode %d.", opcode);
        return SPO_NULL;
    }
}


struct opvar *
opvar_clone(ov)
struct opvar *ov;
{
    if (ov) {
        struct opvar *tmpov = (struct opvar *) alloc(sizeof(struct opvar));
        if (!tmpov)
            panic("could not alloc opvar struct");
        switch (ov->spovartyp) {
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
        case 0x08:
        case 0x01: {
            tmpov->spovartyp = ov->spovartyp;
            tmpov->vardata.l = ov->vardata.l;
        } break;
        case 0x03:
        case 0x02: {
            int len = strlen(ov->vardata.str);
            tmpov->spovartyp = ov->spovartyp;
            tmpov->vardata.str = (char *) alloc(len + 1);
            (void) memcpy((genericptr_t) tmpov->vardata.str,
                          (genericptr_t) ov->vardata.str, len);
            tmpov->vardata.str[len] = '\0';
        } break;
        default: {
            lc_error("Unknown opvar_clone value type (%d)!", ov->spovartyp);
        }
        }
        return tmpov;
    }
    return ((void *)0);
}

void
splev_add_from(splev, from_splev)
sp_lev *splev;
sp_lev *from_splev;
{
    int i;
    if (splev && from_splev)
        for (i = 0; i < from_splev->n_opcodes; i++)
            add_opcode(splev, from_splev->opcodes[i].opcode,
                       opvar_clone(from_splev->opcodes[i].opdat));
}

void
start_level_def(splev, ldfname)
sp_lev **splev;
char *ldfname;
{
    struct lc_funcdefs *f;
    if (strchr(ldfname, '.'))
        lc_error("Invalid dot ('.') in level name '%s'.", ldfname);
    if ((int) strlen(ldfname) > 14)
        lc_error("Level names limited to 14 characters ('%s').", ldfname);
    f = function_definitions;
    while (f) {
        f->n_called = 0;
        f = f->next;
    }
    *splev = (sp_lev *) alloc(sizeof(sp_lev));
    (*splev)->n_opcodes = 0;
    (*splev)->opcodes = ((void *)0);

    vardef_free_all(variable_definitions);
    variable_definitions = ((void *)0);
}




int
get_floor_type(c)
char c;
{
    int val;

    ;
    val = what_map_char(c);
    if (val == 127) {
        val = (-1);
        yywarning("Invalid fill character in MAZE declaration");
    }
    return val;
}




int
get_room_type(s)
char *s;
{
    register int i;

    ;
    for (i = 0; room_types[i].name; i++)
        if (!strcmp(s, room_types[i].name))
            return ((int) room_types[i].type);
    return (-1);
}




int
get_trap_type(s)
char *s;
{
    register int i;

    ;
    for (i = 0; trap_types[i].name; i++)
        if (!strcmp(s, trap_types[i].name))
            return trap_types[i].type;
    return (-1);
}




int
get_monster_id(s, c)
char *s;
char c;
{
    register int i, class;

    ;
    class = c ? def_char_to_monclass(c) : 0;
    if (class == 61)
        return (-1);

    for (i = ((-1)+1); i < 381; i++)
        if (!class || class == mons[i].mlet)
            if (!strcmp(s, mons[i].mname))
                return i;

    for (i = ((-1)+1); i < 381; i++)
        if (!class || class == mons[i].mlet)
            if (!case_insensitive_comp(s, mons[i].mname)) {
                if (be_verbose)
                    lc_warning("Monster type \"%s\" matches \"%s\".", s,
                               mons[i].mname);
                return i;
            }
    return (-1);
}




int
get_object_id(s, c)
char *s;
char c;
{
    int i, class;
    const char *objname;

    ;
    class = (c > 0) ? def_char_to_objclass(c) : 0;
    if (class == 18)
        return (-1);

    for (i = class ? bases[class] : 0; i < 437; i++) {
        if (class && objects[i].oc_class != class)
            break;
        objname = obj_descr[i].oc_name;
        if (objname && !strcmp(s, objname))
            return i;
    }
    for (i = class ? bases[class] : 0; i < 437; i++) {
        if (class && objects[i].oc_class != class)
            break;
        objname = obj_descr[i].oc_name;
        if (objname && !case_insensitive_comp(s, objname)) {
            if (be_verbose)
                lc_warning("Object type \"%s\" matches \"%s\".", s, objname);
            return i;
        }
    }
    return (-1);
}

static void
init_obj_classes()
{
    int i, class, prev_class;

    prev_class = -1;
    for (i = 0; i < 437; i++) {
        class = objects[i].oc_class;
        if (class != prev_class) {
            bases[class] = i;
            prev_class = class;
        }
    }
}




boolean
check_monster_char(c)
char c;
{
    return (def_char_to_monclass(c) != 61);
}




boolean
check_object_char(c)
char c;
{
    return (def_char_to_objclass(c) != 18);
}




char
what_map_char(c)
char c;
{
    ;
    switch (c) {
    case ' ':
        return (0);
    case '#':
        return (23);
    case '.':
        return (24);
    case '-':
        return (2);
    case '|':
        return (1);
    case '+':
        return (22);
    case 'A':
        return (34);
    case 'B':
        return (7);
    case 'C':
        return (35);
    case 'S':
        return (14);
    case 'H':
        return (15);
    case '{':
        return (27);
    case '\\':
        return (28);
    case 'K':
        return (29);
    case '}':
        return (17);
    case 'P':
        return (16);
    case 'L':
        return (20);
    case 'I':
        return (32);
    case 'W':
        return (18);
    case 'T':
        return (13);
    case 'F':
        return (21);
    case 'x':
        return (36);
    }
    return (127);
}

void
add_opcode(sp, opc, dat)
sp_lev *sp;
int opc;
genericptr_t dat;
{
    long nop = sp->n_opcodes;
    _opcode *tmp;

    if ((opc < 0) || (opc >= MAX_SP_OPCODES))
        lc_error("Unknown opcode '%d'", opc);

    tmp = (_opcode *) alloc(sizeof(_opcode) * (nop + 1));
    if (sp->opcodes && nop) {
        (void) memcpy(tmp, sp->opcodes, sizeof(_opcode) * nop);
        free(sp->opcodes);
    } else if (!tmp)
        lc_error("Could not alloc opcode space");

    sp->opcodes = tmp;

    sp->opcodes[nop].opcode = opc;
    sp->opcodes[nop].opdat = dat;

    sp->n_opcodes++;
}





void
scan_map(map, sp)
char *map;
sp_lev *sp;
{
    register int i, len;
    register char *s1, *s2;
    int max_len = 0;
    int max_hig = 0;
    char *tmpmap[21];
    int dx, dy;
    char *mbuf;


    for (s1 = s2 = map; *s1; s1++)
        if (*s1 < '0' || *s1 > '9')
            *s2++ = *s1;
    *s2 = '\0';


    s1 = map;
    while (s1 && *s1) {
        s2 = strchr(s1, '\n');
        if (s2) {
            len = (int) (s2 - s1);
            s1 = s2 + 1;
        } else {
            len = (int) strlen(s1);
            s1 = (char *) 0;
        }
        if (len > max_len)
            max_len = len;
    }


    while (map && *map) {
        tmpmap[max_hig] = (char *) alloc(max_len);
        s1 = strchr(map, '\n');
        if (s1) {
            len = (int) (s1 - map);
            s1++;
        } else {
            len = (int) strlen(map);
            s1 = map + len;
        }
        for (i = 0; i < len; i++)
            if ((tmpmap[max_hig][i] = what_map_char(map[i]))
                == 127) {
                lc_warning("Invalid character '%c' @ (%d, %d) - replacing "
                           "with stone",
                           map[i], max_hig, i);
                tmpmap[max_hig][i] = 0;
            }
        while (i < max_len)
            tmpmap[max_hig][i++] = 0;
        map = s1;
        max_hig++;
    }



    max_x_map = max_len - 1;
    max_y_map = max_hig - 1;

    if (max_len > 76 || max_hig > 21) {
        lc_error("Map too large at (%d x %d), max is (%d x %d)", max_len,
                 max_hig, 76, 21);
    }

    mbuf = (char *) alloc(((max_hig - 1) * max_len) + (max_len - 1) + 2);
    for (dy = 0; dy < max_hig; dy++)
        for (dx = 0; dx < max_len; dx++)
            mbuf[(dy * max_len) + dx] = (tmpmap[dy][dx] + 1);

    mbuf[((max_hig - 1) * max_len) + (max_len - 1) + 1] = '\0';

    add_opvars(sp, "siio", mbuf,max_hig,max_len,SPO_MAP);

    for (dy = 0; dy < max_hig; dy++)
        if (tmpmap[dy]) free((genericptr_t)(tmpmap[dy]));
    if (mbuf) free((genericptr_t)(mbuf));
}




static boolean
write_common_data(fd)
int fd;
{
    static struct version_info version_data = {
        0x03060000UL, 0x00060000UL, 0x211b517dUL, 0xc48195c4UL,
        0x00025000UL
    };

    if ((long) write(fd, (genericptr_t)(&version_data), sizeof version_data) != (long) (sizeof version_data)) return ((boolean)0);;
    return ((boolean)1);
}





static boolean
write_maze(fd, maze)
int fd;
sp_lev *maze;
{
    int i;

    if (!write_common_data(fd))
        return ((boolean)0);

    if ((long) write(fd, (genericptr_t)(&(maze->n_opcodes)), sizeof(maze->n_opcodes)) != (long) (sizeof(maze->n_opcodes))) return ((boolean)0);;

    for (i = 0; i < maze->n_opcodes; i++) {
        _opcode tmpo = maze->opcodes[i];

        if ((long) write(fd, (genericptr_t)(&(tmpo.opcode)), sizeof(tmpo.opcode)) != (long) (sizeof(tmpo.opcode))) return ((boolean)0);;

        if (tmpo.opcode < SPO_NULL || tmpo.opcode >= MAX_SP_OPCODES)
            panic("write_maze: unknown opcode (%d).", tmpo.opcode);

        if (tmpo.opcode == SPO_PUSH) {
            genericptr_t opdat = tmpo.opdat;
            if (opdat) {
                struct opvar *ov = (struct opvar *) opdat;
                int size;
                if ((long) write(fd, (genericptr_t)(&(ov->spovartyp)), sizeof(ov->spovartyp)) != (long) (sizeof(ov->spovartyp))) return ((boolean)0);;
                switch (ov->spovartyp) {
                case 0x00:
                    break;
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x01:
                    if ((long) write(fd, (genericptr_t)(&(ov->vardata.l)), sizeof(ov->vardata.l)) != (long) (sizeof(ov->vardata.l))) return ((boolean)0);;
                    break;
                case 0x03:
                case 0x02:
                    if (ov->vardata.str)
                        size = strlen(ov->vardata.str);
                    else
                        size = 0;
                    if ((long) write(fd, (genericptr_t)(&size), sizeof(size)) != (long) (sizeof(size))) return ((boolean)0);;
                    if (size) {
                        if ((long) write(fd, (genericptr_t)(ov->vardata.str), size) != (long) (size)) return ((boolean)0);;
                        if (ov->vardata.str) free((genericptr_t)(ov->vardata.str));
                    }
                    break;
                default:
                    panic("write_maze: unknown data type (%d).",
                          ov->spovartyp);
                }
            } else
                panic("write_maze: PUSH with no data.");
        } else {

            genericptr_t opdat = tmpo.opdat;
            if (opdat)
                panic("write_maze: opcode (%d) has data.", tmpo.opcode);
        }

        if (tmpo.opdat) free((genericptr_t)(tmpo.opdat));
    }

    if (maze->opcodes) free((genericptr_t)(maze->opcodes));
    maze->opcodes = ((void *)0);



    return ((boolean)1);
}





boolean
write_level_file(filename, lvl)
char *filename;
sp_lev *lvl;
{
    int fout;
    char lbuf[60];

    lbuf[0] = '\0';



    (void) strcat(lbuf, filename);
    (void) strcat(lbuf, ".lev");




    fout = open(lbuf, 0x0001 | 0x0200 | 0, 0644);

    if (fout < 0)
        return ((boolean)0);

    if (!lvl)
        panic("write_level_file");

    if (be_verbose)
        fprintf(__stdoutp, "File: '%s', opcodes: %ld\n", lbuf, lvl->n_opcodes);

    if (!write_maze(fout, lvl))
        return ((boolean)0);

    (void) close(fout);

    return ((boolean)1);
}

static int
case_insensitive_comp(s1, s2)
const char *s1;
const char *s2;
{
    unsigned char u1, u2;

    for (;; s1++, s2++) {
        u1 = tolower((unsigned char) *s1);
        u2 = tolower((unsigned char) *s2);
        if ((u1 == '\0') || (u1 != u2)) {
            break;
        }
    }
    return u1 - u2;
}
