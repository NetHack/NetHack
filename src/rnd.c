/* NetHack 3.7	rnd.c	$NHDT-Date: 1596498205 2020/08/03 23:43:25 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.30 $ */
/*      Copyright (c) 2004 by Robert Patrick Rankin               */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifdef USE_ISAAC64
#include "isaac64.h"

static int whichrng(int (*fn)(int));

#if 0
static isaac64_ctx rng_state;
#endif

struct rnglist_t {
    int (*fn)(int);
    boolean init;
    isaac64_ctx rng_state;
};

enum { CORE = 0, DISP = 1 };

static struct rnglist_t rnglist[] = {
    { rn2, FALSE, { 0 } },                      /* CORE */
    { rn2_on_display_rng, FALSE, { 0 } },       /* DISP */
};

static int
whichrng(int (*fn)(int))
{
    int i;

    for (i = 0; i < SIZE(rnglist); ++i)
        if (rnglist[i].fn == fn)
            return i;
    return -1;
}

void
init_isaac64(unsigned long seed, int (*fn)(int))
{
    unsigned char new_rng_state[sizeof seed];
    unsigned i;
    int rngindx = whichrng(fn);

    if (rngindx < 0)
        panic("Bad rng function passed to init_isaac64().");

    for (i = 0; i < sizeof seed; i++) {
        new_rng_state[i] = (unsigned char) (seed & 0xFF);
        seed >>= 8;
    }
    isaac64_init(&rnglist[rngindx].rng_state, new_rng_state,
                 (int) sizeof seed);
}

static int
RND(int x)
{
    return (isaac64_next_uint64(&rnglist[CORE].rng_state) % x);
}

/* 0 <= rn2(x) < x, but on a different sequence from the "main" rn2;
   used in cases where the answer doesn't affect gameplay and we don't
   want to give users easy control over the main RNG sequence. */
int
rn2_on_display_rng(register int x)
{
    return (isaac64_next_uint64(&rnglist[DISP].rng_state) % x);
}

#else   /* USE_ISAAC64 */

/* "Rand()"s definition is determined by [OS]conf.h */
#if defined(LINT) && defined(UNIX) /* rand() is long... */
extern int rand(void);
#define RND(x) (rand() % x)
#else /* LINT */
#if defined(UNIX) || defined(RANDOM)
#define RND(x) ((int) (Rand() % (long) (x)))
#else
/* Good luck: the bottom order bits are cyclic. */
#define RND(x) ((int) ((Rand() >> 3) % (x)))
#endif
#endif /* LINT */
int
rn2_on_display_rng(register int x)
{
    static unsigned seed = 1;
    seed *= 2739110765;
    return (int)((seed >> 16) % (unsigned)x);
}
#endif  /* USE_ISAAC64 */

/* 0 <= rn2(x) < x */
int
rn2(register int x)
{
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED)
    if (x <= 0) {
        impossible("rn2(%d) attempted", x);
        return 0;
    }
    x = RND(x);
    return x;
#else
    return RND(x);
#endif
}

/* 0 <= rnl(x) < x; sometimes subtracting Luck;
   good luck approaches 0, bad luck approaches (x-1) */
int
rnl(register int x)
{
    register int i, adjustment;

#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED)
    if (x <= 0) {
        impossible("rnl(%d) attempted", x);
        return 0;
    }
#endif

    adjustment = Luck;
    if (x <= 15) {
        /* for small ranges, use Luck/3 (rounded away from 0);
           also guard against architecture-specific differences
           of integer division involving negative values */
        adjustment = (abs(adjustment) + 1) / 3 * sgn(adjustment);
        /*
         *       11..13 ->  4
         *        8..10 ->  3
         *        5.. 7 ->  2
         *        2.. 4 ->  1
         *       -1,0,1 ->  0 (no adjustment)
         *       -4..-2 -> -1
         *       -7..-5 -> -2
         *      -10..-8 -> -3
         *      -13..-11-> -4
         */
    }

    i = RND(x);
    if (adjustment && rn2(37 + abs(adjustment))) {
        i -= adjustment;
        if (i < 0)
            i = 0;
        else if (i >= x)
            i = x - 1;
    }
    return i;
}

/* 1 <= rnd(x) <= x */
int
rnd(register int x)
{
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED)
    if (x <= 0) {
        impossible("rnd(%d) attempted", x);
        return 1;
    }
#endif
    x = RND(x) + 1;
    return x;
}

int
rnd_on_display_rng(register int x)
{
    return rn2_on_display_rng(x) + 1;
}

/* d(N,X) == NdX == dX+dX+...+dX N times; n <= d(n,x) <= (n*x) */
int
d(register int n, register int x)
{
    register int tmp = n;

#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED)
    if (x < 0 || n < 0 || (x == 0 && n != 0)) {
        impossible("d(%d,%d) attempted", n, x);
        return 1;
    }
#endif
    while (n--)
        tmp += RND(x);
    return tmp; /* Alea iacta est. -- J.C. */
}

/* 1 <= rne(x) <= max(u.ulevel/3,5) */
int
rne(register int x)
{
    register int tmp, utmp;

    utmp = (u.ulevel < 15) ? 5 : u.ulevel / 3;
    tmp = 1;
    while (tmp < utmp && !rn2(x))
        tmp++;
    return tmp;

    /* was:
     *  tmp = 1;
     *  while (!rn2(x))
     *    tmp++;
     *  return min(tmp, (u.ulevel < 15) ? 5 : u.ulevel / 3);
     * which is clearer but less efficient and stands a vanishingly
     * small chance of overflowing tmp
     */
}

/* rnz: everyone's favorite! */
int
rnz(int i)
{
#ifdef LINT
    int x = i;
    int tmp = 1000;
#else
    register long x = (long) i;
    register long tmp = 1000L;
#endif

    tmp += rn2(1000);
    tmp *= rne(4);
    if (rn2(2)) {
        x *= tmp;
        x /= 1000;
    } else {
        x *= 1000;
        x /= tmp;
    }
    return (int) x;
}

/*rnd.c*/
