/* NetHack 3.5	rnd.c	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.5	rnd.c	$Date: 2009/05/06 10:47:41 $  $Revision: 1.7 $ */
/*	SCCS Id: @(#)rnd.c	3.5	2004/08/27	*/
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* "Rand()"s definition is determined by [OS]conf.h */
#if defined(LINT) && defined(UNIX)	/* rand() is long... */
extern int NDECL(rand);
#define RND(x)	(rand() % x)
#else /* LINT */
# if defined(UNIX) || defined(RANDOM)
#define RND(x)	(int)(Rand() % (long)(x))
# else
/* Good luck: the bottom order bits are cyclic. */
#define RND(x)	(int)((Rand()>>3) % (x))
# endif
#endif /* LINT */

int
rn2(x)		/* 0 <= rn2(x) < x */
register int x;
{
#ifdef BETA
	if (x <= 0) {
		debugpline("rn2(%d) attempted", x);
		return(0);
	}
	x = RND(x);
	return(x);
#else
	return(RND(x));
#endif
}

int
rnl(x)		/* 0 <= rnl(x) < x; sometimes subtracting Luck */
register int x;	/* good luck approaches 0, bad luck approaches (x-1) */
{
	register int i, adjustment;

#ifdef BETA
	if (x <= 0) {
		debugpline("rnl(%d) attempted", x);
		return(0);
	}
#endif

	adjustment = Luck;
	if (x <= 15) {
	    /* for small ranges, use Luck/3 (rounded away from 0);
	       also guard against architecture-specific differences
	       of integer division involving negative values */
	    adjustment = (abs(adjustment) + 1) / 3 * sgn(adjustment);
	    /*
	     *	 11..13 ->  4
	     *	  8..10 ->  3
	     *	  5.. 7 ->  2
	     *	  2.. 4 ->  1
	     *	 -1,0,1 ->  0 (no adjustment)
	     *	 -4..-2 -> -1
	     *	 -7..-5 -> -2
	     *	-10..-8 -> -3
	     *	-13..-11-> -4
	     */
	}

	i = RND(x);
	if (adjustment && rn2(37 + abs(adjustment))) {
	    i -= adjustment;
	    if (i < 0) i = 0;
	    else if (i >= x) i = x-1;
	}
	return i;
}

int
rnd(x)		/* 1 <= rnd(x) <= x */
register int x;
{
#ifdef BETA
	if (x <= 0) {
		debugpline("rnd(%d) attempted", x);
		return(1);
	}
	x = RND(x)+1;
	return(x);
#else
	return(RND(x)+1);
#endif
}

int
d(n,x)		/* n <= d(n,x) <= (n*x) */
register int n, x;
{
	register int tmp = n;

#ifdef BETA
	if (x < 0 || n < 0 || (x == 0 && n != 0)) {
		debugpline("d(%d,%d) attempted", n, x);
		return(1);
	}
#endif
	while(n--) tmp += RND(x);
	return(tmp); /* Alea iacta est. -- J.C. */
}

int
rne(x)
register int x;
{
	register int tmp, utmp;

	utmp = (u.ulevel < 15) ? 5 : u.ulevel/3;
	tmp = 1;
	while (tmp < utmp && !rn2(x))
		tmp++;
	return tmp;

	/* was:
	 *	tmp = 1;
	 *	while(!rn2(x)) tmp++;
	 *	return(min(tmp,(u.ulevel < 15) ? 5 : u.ulevel/3));
	 * which is clearer but less efficient and stands a vanishingly
	 * small chance of overflowing tmp
	 */
}

int
rnz(i)
int i;
{
#ifdef LINT
	int x = i;
	int tmp = 1000;
#else
	register long x = i;
	register long tmp = 1000;
#endif
	tmp += rn2(1000);
	tmp *= rne(4);
	if (rn2(2)) { x *= tmp; x /= 1000; }
	else { x *= 1000; x /= tmp; }
	return((int)x);
}

/*rnd.c*/
