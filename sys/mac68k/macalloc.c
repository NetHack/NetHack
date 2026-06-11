/* NetHack 5.0	macalloc.c	*/
/* Copyright (c) Ingo Paschke, 2026. */
/* NetHack may be freely redistributed.  See license for details. */
/* macalloc.c -- pooling allocator for the classic-Mac (System 7) 68k port.
 *
 * newlib's malloc/realloc/free are correct but do real per-call work (size-bin
 * search, block split/coalesce).  Level generation issues ~12,000 tiny
 * alloc/realloc/free calls -- almost all from Lua, which creates objects via
 * realloc(NULL,n) -- and on a 16 MHz 68030 that per-call cost dominates startup
 * (measured ~6x slower without this).  This is a classic small-object /
 * segregated free-list pool ("simple segregated storage"): requests are rounded
 * up to a fixed size class served from a per-class free-list, with whole slabs
 * grabbed in bulk from newlib.  alloc/free become O(1) list ops and a same-class
 * realloc is a no-op; blocks larger than the biggest class pass through.
 *
 * It interposes via the linker (-Wl,--wrap=malloc,--wrap=free,--wrap=realloc in
 * the mac68k hint), so it fronts core alloc(), Lua's nhl_alloc, and libc alike.
 * Every block we return carries an 8-byte header (which also keeps the user
 * pointer 8-byte aligned) holding a 24-bit magic + size class.  free()/realloc()
 * validate the magic, so a pointer that bypassed the wrap (e.g. calloc, which
 * newlib routes through _malloc_r, not the malloc symbol) is recognized and
 * forwarded to the real allocator rather than corrupting the pool.
 *
 * Define NHMAC_ALLOC_STATS to compile in the call/slab counters and have
 * macalloc_stats() log per-phase deltas to dprintf.log; off by default.
 */
#include <stddef.h>
#include <string.h>     /* memcpy */

extern void *__real_malloc(size_t);
extern void  __real_free(void *);
extern void *__real_realloc(void *, size_t);

/* size classes -- all multiples of 8 so blocks stay 8-byte aligned */
#define NCLASSES 12
static const unsigned short g_class[NCLASSES] = {
    16, 24, 32, 48, 64, 96, 128, 160, 192, 256, 384, 512
};
#define SLAB_BYTES 16384            /* bulk chunk carved into one class */

#define HDR_MAGIC 0xC0FFEE00UL      /* top 24 bits; low 8 = class idx or HDR_BIG */
#define HDR_MASK  0xFFFFFF00UL
#define HDR_BIG   0xFFUL

typedef struct {
    unsigned long tag;              /* HDR_MAGIC | class (or HDR_BIG) */
    unsigned long size;             /* requested user size (for realloc copy) */
} hdr_t;                            /* sizeof == 8 */

#define HDRSZ    ((size_t) sizeof(hdr_t))
#define USERP(h) ((void *) ((char *) (h) + HDRSZ))
#define HDRP(p)  ((hdr_t *) ((char *) (p) - HDRSZ))

static void *g_free[NCLASSES];      /* per-class free-list (of user pointers) */

#ifdef NHMAC_ALLOC_STATS
#include <Events.h>                 /* TickCount */
extern void mac_dprintf(char *format, ...);
static unsigned long c_malloc, c_free, c_realloc, c_bytes, c_slabs, c_big;
static unsigned long p_malloc, p_free, p_realloc, p_bytes, p_slabs, p_big;
static long p_ticks;
#define STAT(s) s
#else
#define STAT(s) ((void) 0)
#endif

static int
class_for(size_t n)
{
    int i;
    for (i = 0; i < NCLASSES; i++)
        if ((size_t) g_class[i] >= n)
            return i;
    return -1;                      /* too big -> passthrough */
}

static int
refill(int c)
{
    size_t stride = HDRSZ + g_class[c];
    int n = (int) (SLAB_BYTES / stride);
    char *slab;
    int i;

    if (n < 1)
        n = 1;
    slab = (char *) __real_malloc((size_t) n * stride);
    if (!slab)
        return 0;
    STAT(c_slabs++);
    for (i = 0; i < n; i++) {
        hdr_t *h = (hdr_t *) (slab + (size_t) i * stride);
        void *u = USERP(h);
        h->tag = HDR_MAGIC | (unsigned long) c;
        h->size = 0;
        *(void **) u = g_free[c];   /* push onto free-list */
        g_free[c] = u;
    }
    return 1;
}

static void *
pool_alloc(size_t n)
{
    int c;
    void *u;

    if (n == 0)
        n = 1;
    c = class_for(n);
    if (c < 0) {                    /* big block: passthrough with our header */
        hdr_t *h = (hdr_t *) __real_malloc(HDRSZ + n);
        if (!h)
            return NULL;
        h->tag = HDR_MAGIC | HDR_BIG;
        h->size = n;
        STAT(c_big++);
        return USERP(h);
    }
    if (!g_free[c] && !refill(c))
        return NULL;
    u = g_free[c];
    g_free[c] = *(void **) u;        /* pop */
    HDRP(u)->size = n;
    return u;
}

static void
pool_free(void *p)
{
    hdr_t *h;
    int c;

    if (!p)
        return;
    h = HDRP(p);
    if ((h->tag & HDR_MASK) != HDR_MAGIC) { /* not ours (bypassed the wrap) */
        __real_free(p);
        return;
    }
    if ((h->tag & 0xFFUL) == HDR_BIG) {
        __real_free(h);
        return;
    }
    c = (int) (h->tag & 0xFFUL);
    if (c >= NCLASSES) {             /* magic collision on a foreign block:
                                        never index g_free[] out of range */
        __real_free(p);
        return;
    }
    *(void **) p = g_free[c];        /* push back */
    g_free[c] = p;
}

void *
__wrap_malloc(size_t n)
{
    STAT(c_malloc++);
    STAT(c_bytes += n);
    return pool_alloc(n);
}

void
__wrap_free(void *p)
{
    STAT(if (p) c_free++);
    pool_free(p);
}

void *
__wrap_realloc(void *p, size_t n)
{
    hdr_t *h;
    void *np;
    size_t oldsz;
    int oc, nc;

    STAT(c_realloc++);
    STAT(c_bytes += n);
    if (!p)
        return pool_alloc(n);
    if (n == 0) {
        pool_free(p);
        return NULL;
    }
    h = HDRP(p);
    if ((h->tag & HDR_MASK) != HDR_MAGIC)   /* not ours */
        return __real_realloc(p, n);

    oc = (int) (h->tag & 0xFFUL);
    if (oc != (int) HDR_BIG && oc >= NCLASSES) /* magic collision: foreign
                                                  block, h->size is garbage */
        return __real_realloc(p, n);
    nc = class_for(n);
    if (oc != (int) HDR_BIG && nc == oc) {  /* same class: grow/shrink in place */
        h->size = n;
        return p;
    }
    if (oc == (int) HDR_BIG && nc < 0) {     /* big -> big: real realloc */
        hdr_t *nh = (hdr_t *) __real_realloc(h, HDRSZ + n);
        if (!nh)
            return NULL;
        nh->tag = HDR_MAGIC | HDR_BIG;
        nh->size = n;
        return USERP(nh);
    }
    /* class change (incl. pool<->big): alloc new, copy, free old */
    oldsz = h->size;
    np = pool_alloc(n);
    if (!np)
        return NULL;
    memcpy(np, p, oldsz < n ? oldsz : n);
    pool_free(p);
    return np;
}

/* Profiling hook: with NHMAC_ALLOC_STATS, log call/slab counts + elapsed ticks
   since the last call to dprintf.log; otherwise a no-op.  Called from macmain. */
void
macalloc_stats(const char *tag)
{
#ifdef NHMAC_ALLOC_STATS
    long now = TickCount();

    mac_dprintf("ALLOC %-12s dt=%ld tk  malloc=%lu free=%lu realloc=%lu bytes=%lu"
                " slabs=%lu big=%lu",
                tag, now - p_ticks,
                c_malloc - p_malloc, c_free - p_free, c_realloc - p_realloc,
                c_bytes - p_bytes, c_slabs - p_slabs, c_big - p_big);
    p_ticks = now;
    p_malloc = c_malloc;
    p_free = c_free;
    p_realloc = c_realloc;
    p_bytes = c_bytes;
    p_slabs = c_slabs;
    p_big = c_big;
#else
    (void) tag;
#endif
}
