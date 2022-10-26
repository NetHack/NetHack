/* NetHack 3.7	oldcrtl.c	$NHDT-Date: 1596498304 2020/08/03 23:45:04 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.9 $ */
/*       Pat Rankin  May'90                                       */
/* VMS NetHack support, not needed for vms 4.6,4.7,5.x,or later   */

#ifdef VERYOLD_VMS
/*
 * The following routines are used by NetHack but were not available
 * from the C Run-Time Library (VAXCRTL) prior to VMS V4.6.
 *
 *        atexit, memcmp, memcpy, qsort, rename, vprintf, vsprintf
 *
 * Most of them are implemented here, but others will have to be worked
 * around in another fashion [such as '#define USE_OLDARGS' (even though
 * <varargs.h> is available) to avoid the need for vprintf & vsprintf].
 *
 */
#define REG register
#define const

#ifndef SUPPRESS_MEM_FUNCS
/* note: hand optimized for VAX (hardware pre-decrement & post-increment) */

/* void *memset(void *, int, size_t) -- fill chunk of memory.
*/
char *
memset(dst, fil, cnt)
REG char *dst;
REG char fil;
REG int cnt;
{
    char *dst_p = dst;
    while (--cnt >= 0)
        *dst++ = fil;
    return dst_p;
}

/* void *memcpy(void *, const void *, size_t) -- copy chunk of memory.
*/
char *
memcpy(dst, src, cnt)
REG char *dst;
REG const char *src;
REG int cnt;
{
    char *dst_p = dst;
    while (--cnt >= 0)
        *dst++ = *src++;
    return dst_p;
}

/* void *memmove(void *, const void *, size_t) -- copy possibly overlapping
 * mem.
*/
char *
memmove(dst, src, cnt)
REG char *dst;
REG const char *src;
REG int cnt;
{
    char *dst_p = dst;
    if (src == dst || cnt <= 0) {
        ; /* do nothing */
    } else if (dst < src || dst >= src + cnt) {
        while (--cnt >= 0)
            *dst++ = *src++;
    } else { /* work backwards */
        dst += cnt, src += cnt;
        while (--cnt >= 0)
            *--dst = *--src;
    }
    return dst_p;
}

/* void *memchr(const void *, int, size_t) -- search for a byte.
*/
char *
memchr(buf, byt, len)
REG const char *buf;
REG char byt;
REG int len;
{
    while (--len >= 0)
        if (*buf++ == byt) /* found */
            return (char *) --buf;
    return (char *) 0; /* not found */
}

/* int memcmp(const void *, const void *, size_t) -- compare two chunks.
*/
int
memcmp(buf1, buf2, len)
REG const char *buf1;
REG const char *buf2;
REG int len;
{
    while (--len >= 0)
        if (*buf1++ != *buf2++)
            return (*--buf1 - *--buf2);
    return 0; /* buffers matched */
}
#endif /*!SUPPRESS_MEM_FUNCS*/

#ifndef SUPPRESS_ATEXIT
/* int atexit(void (*)(void)) -- register an exit handler.
*/
#define MAX_EXIT_FUNCS 32 /* arbitrary (32 matches VAX C v3.x docs) */
struct ex_hndlr {
    long reserved, (*routine)(), arg_count, *arg1_addr;
};
static int ex_cnt = 0; /* number of handlers registered so far */
static struct {
    long dummy_arg;
    struct ex_hndlr handler; /*(black box)*/
} ex_data[MAX_EXIT_FUNCS];   /* static handler data */
extern unsigned long sys$dclexh();

int
atexit(function)
void (*function)(); /* note: actually gets called with 1 arg */
{
    if (ex_cnt < MAX_EXIT_FUNCS) {
        ex_data[ex_cnt].dummy_arg = 0; /* ultimately receives exit reason */
        ex_data[ex_cnt].handler.reserved = 0;
        ex_data[ex_cnt].handler.routine = (long (*) ()) function;
        ex_data[ex_cnt].handler.arg_count = 1; /*(required)*/
        ex_data[ex_cnt].handler.arg1_addr = &ex_data[ex_cnt].dummy_arg;
        (void) sys$dclexh(
            &ex_data[ex_cnt].handler); /* declare exit handler */
        return ++ex_cnt;               /*(non-zero)*/
    } else
        return 0;
}
#endif /*!SUPPRESS_ATEXIT*/

#ifndef SUPPRESS_RENAME
/* int rename(const char *, const char *) -- rename a file (on same device).
*/
#ifndef EVMSERR
#include <errno.h>
#define C$$TRANSLATE(status) (errno = EVMSERR, vaxc$errno = (status))
#endif
extern unsigned long lib$rename_file();

int
rename(old_name, new_name)
const char *old_name;
const char *new_name;
{
    struct dsc {
        unsigned short len, mbz;
        const char *adr;
    } old_dsc, new_dsc;
    unsigned long status;

    /* put strings into descriptors and call run-time library routine */
    new_dsc.mbz = old_dsc.mbz = 0; /* type and class unspecified */
    old_dsc.len = strlen(old_dsc.adr = old_name);
    new_dsc.len = strlen(new_dsc.adr = new_name);
    status = lib$rename_file(&old_dsc, &new_dsc); /* omit optional args */
    if (!(status & 1)) {                          /* even => failure */
        C$$TRANSLATE(status);
        return -1;
    } else /*  odd => success */
        return 0;
}
#endif /*!SUPPRESS_RENAME*/

#ifndef SUPPRESS_QSORT
/* void qsort(void *, size_t, size_t, int (*)()) -- sort arbitrary collection.
*/
extern char *malloc(); /* assume no alloca() available */
extern void free();

void
qsort(base, count, size, compare)
char *base;
int count;
REG int size;
int (*compare)();
{
    REG int i, cmp;
    REG char *next, *prev, *tmp = 0;
    char wrk_buf[512];

    /* just use a shuffle sort (tradeoff between efficiency & simplicity) */
    /*  [Optimal if already sorted; worst case when initially reversed.]  */
    for (next = base, i = 1; i < count; i++) {
        prev = next, next += size; /* increment front pointer */
        if ((cmp = (*compare)(next, prev)) < 0) {
            /* found element out of order; move other(s) up then re-insert it
             */
            if (!tmp)
                tmp = size > (int) (sizeof wrk_buf) ? malloc(size) : wrk_buf;
            memcpy(tmp, next, size); /* save smaller element */
            while (cmp < 0) {
                memcpy(prev + size, prev, size); /* move larger elem. up */
                prev -= size;                    /* decrement back pointer */
                cmp = (prev >= base ? (*compare)(tmp, prev) : 0);
            }
            memcpy(prev + size, tmp, size); /* restore small element */
        }
    }
    if (tmp != 0 && tmp != wrk_buf)
        free(tmp);
    return;
}
#endif /*!SUPPRESS_QSORT*/

#endif /*VERYOLD_VMS*/
