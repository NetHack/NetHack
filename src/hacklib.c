/*	SCCS Id: @(#)hacklib.c	3.5	2007/03/05	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* Copyright (c) Robert Patrick Rankin, 1991		  */
/* NetHack may be freely redistributed.  See license for details. */

/* We could include only config.h, except for the overlay definitions... */
#include "hack.h"
/*=
    Assorted 'small' utility routines.	They're virtually independent of
NetHack, except that rounddiv may call panic().

      return type     routine name    argument type(s)
	boolean		digit		(char)
	boolean		letter		(char)
	char		highc		(char)
	char		lowc		(char)
	char *		lcase		(char *)
	char *		upstart		(char *)
	char *		mungspaces	(char *)
	char *		eos		(char *)
	char *		strkitten	(char *,char)
	void		copynchars	(char *,const char *,int)
	char *		s_suffix	(const char *)
	char *		xcrypt		(const char *, char *)
	boolean		onlyspace	(const char *)
	char *		tabexpand	(char *)
	char *		visctrl		(char)
	char *		strsubst	(char *, const char *, const char *)
	const char *	ordin		(int)
	char *		sitoa		(int)
	int		sgn		(int)
	int		rounddiv	(long, int)
	int		distmin		(int, int, int, int)
	int		dist2		(int, int, int, int)
	boolean		online2		(int, int)
	boolean		pmatch		(const char *, const char *)
	int		strncmpi	(const char *, const char *, int)
	char *		strstri		(const char *, const char *)
	boolean		fuzzymatch	(const char *,const char *,const char *,boolean)
	void		setrandom	(void)
	time_t		getnow		(void)
	int		getyear		(void)
	char *		yymmdd		(time_t)
	long		yyyymmdd	(time_t)
	long		hhmmss		(time_t)
	int		phase_of_the_moon	(void)
	boolean		friday_13th	(void)
	int		night		(void)
	int		midnight	(void)
=*/
#ifdef LINT
# define Static		/* pacify lint */
#else
# define Static static
#endif

boolean
digit(c)		/* is 'c' a digit? */
    char c;
{
    return((boolean)('0' <= c && c <= '9'));
}

boolean
letter(c)		/* is 'c' a letter?  note: '@' classed as letter */
    char c;
{
    return((boolean)(('@' <= c && c <= 'Z') || ('a' <= c && c <= 'z')));
}

char
highc(c)			/* force 'c' into uppercase */
    char c;
{
    return((char)(('a' <= c && c <= 'z') ? (c & ~040) : c));
}

char
lowc(c)			/* force 'c' into lowercase */
    char c;
{
    return((char)(('A' <= c && c <= 'Z') ? (c | 040) : c));
}

char *
lcase(s)		/* convert a string into all lowercase */
    char *s;
{
    register char *p;

    for (p = s; *p; p++)
	if ('A' <= *p && *p <= 'Z') *p |= 040;
    return s;
}

char *
upstart(s)		/* convert first character of a string to uppercase */
    char *s;
{
    if (s) *s = highc(*s);
    return s;
}

/* remove excess whitespace from a string buffer (in place) */
char *
mungspaces(bp)
char *bp;
{
    register char c, *p, *p2;
    boolean was_space = TRUE;

    for (p = p2 = bp; (c = *p) != '\0'; p++) {
	if (c == '\t') c = ' ';
	if (c != ' ' || !was_space) *p2++ = c;
	was_space = (c == ' ');
    }
    if (was_space && p2 > bp) p2--;
    *p2 = '\0';
    return bp;
}

char *
eos(s)			/* return the end of a string (pointing at '\0') */
    register char *s;
{
    while (*s) s++;	/* s += strlen(s); */
    return s;
}

/* strcat(s, {c,'\0'}); */
char *
strkitten(s, c)		/* append a character to a string (in place) */
    char *s;
    char c;
{
    char *p = eos(s);

    *p++ = c;
    *p = '\0';
    return s;
}

void
copynchars(dst, src, n)		/* truncating string copy */
    char *dst;
    const char *src;
    int n;
{
    /* copies at most n characters, stopping sooner if terminator reached;
       treats newline as input terminator; unlike strncpy, always supplies
       '\0' terminator so dst must be able to hold at least n+1 characters */
    while (n > 0 && *src != '\0' && *src != '\n') {
	*dst++ = *src++;
	--n;
    }
    *dst = '\0';
}

char *
s_suffix(s)		/* return a name converted to possessive */
    const char *s;
{
    Static char buf[BUFSZ];

    Strcpy(buf, s);
    if(!strcmpi(buf, "it"))
	Strcat(buf, "s");
    else if(*(eos(buf)-1) == 's')
	Strcat(buf, "'");
    else
	Strcat(buf, "'s");
    return buf;
}

char *
xcrypt(str, buf)	/* trivial text encryption routine (see makedefs) */
const char *str;
char *buf;
{
    register const char *p;
    register char *q;
    register int bitmask;

    for (bitmask = 1, p = str, q = buf; *p; q++) {
	*q = *p++;
	if (*q & (32|64)) *q ^= bitmask;
	if ((bitmask <<= 1) >= 32) bitmask = 1;
    }
    *q = '\0';
    return buf;
}

boolean
onlyspace(s)		/* is a string entirely whitespace? */
    const char *s;
{
    for (; *s; s++)
	if (*s != ' ' && *s != '\t') return FALSE;
    return TRUE;
}

char *
tabexpand(sbuf)		/* expand tabs into proper number of spaces */
    char *sbuf;
{
    char buf[BUFSZ];
    register char *bp, *s = sbuf;
    register int idx;

    if (!*s) return sbuf;

    /* warning: no bounds checking performed */
    for (bp = buf, idx = 0; *s; s++)
	if (*s == '\t') {
	    do *bp++ = ' '; while (++idx % 8);
	} else {
	    *bp++ = *s;
	    idx++;
	}
    *bp = 0;
    return strcpy(sbuf, buf);
}

char *
visctrl(c)		/* make a displayable string from a character */
    char c;
{
    Static char ccc[3];

    c &= 0177;

    ccc[2] = '\0';
    if (c < 040) {
	ccc[0] = '^';
	ccc[1] = c | 0100;	/* letter */
    } else if (c == 0177) {
	ccc[0] = '^';
	ccc[1] = c & ~0100;	/* '?' */
    } else {
	ccc[0] = c;		/* printable character */
	ccc[1] = '\0';
    }
    return ccc;
}

/* substitute a word or phrase in a string (in place) */
/* caller is responsible for ensuring that bp points to big enough buffer */
char *
strsubst(bp, orig, replacement)
    char *bp;
    const char *orig, *replacement;
{
    char *found, buf[BUFSZ];

    if (bp) {
	found = strstr(bp, orig);
	if (found) {
		Strcpy(buf, found + strlen(orig));
		Strcpy(found, replacement);
		Strcat(bp, buf);
	}
    }
    return bp;
}


const char *
ordin(n)		/* return the ordinal suffix of a number */
    int n;			/* note: should be non-negative */
{
    register int dd = n % 10;

    return (dd == 0 || dd > 3 || (n % 100) / 10 == 1) ? "th" :
	    (dd == 1) ? "st" : (dd == 2) ? "nd" : "rd";
}

char *
sitoa(n)		/* make a signed digit string from a number */
    int n;
{
    Static char buf[13];

    Sprintf(buf, (n < 0) ? "%d" : "+%d", n);
    return buf;
}

int
sgn(n)			/* return the sign of a number: -1, 0, or 1 */
    int n;
{
    return (n < 0) ? -1 : (n != 0);
}

int
rounddiv(x, y)		/* calculate x/y, rounding as appropriate */
    long x;
    int  y;
{
    int r, m;
    int divsgn = 1;

    if (y == 0)
	panic("division by zero in rounddiv");
    else if (y < 0) {
	divsgn = -divsgn;  y = -y;
    }
    if (x < 0) {
	divsgn = -divsgn;  x = -x;
    }
    r = x / y;
    m = x % y;
    if (2*m >= y) r++;

    return divsgn * r;
}

int
distmin(x0, y0, x1, y1) /* distance between two points, in moves */
    int x0, y0, x1, y1;
{
    register int dx = x0 - x1, dy = y0 - y1;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
  /*  The minimum number of moves to get from (x0,y0) to (x1,y1) is the
   :  larger of the [absolute value of the] two deltas.
   */
    return (dx < dy) ? dy : dx;
}

int
dist2(x0, y0, x1, y1)	/* square of euclidean distance between pair of pts */
    int x0, y0, x1, y1;
{
    register int dx = x0 - x1, dy = y0 - y1;
    return dx * dx + dy * dy;
}

boolean
online2(x0, y0, x1, y1) /* are two points lined up (on a straight line)? */
    int x0, y0, x1, y1;
{
    int dx = x0 - x1, dy = y0 - y1;
    /*  If either delta is zero then they're on an orthogonal line,
     *  else if the deltas are equal (signs ignored) they're on a diagonal.
     */
    return((boolean)(!dy || !dx || (dy == dx) || (dy + dx == 0)));	/* (dy == -dx) */
}

boolean
pmatch(patrn, strng)	/* match a string against a pattern */
    const char *patrn, *strng;
{
    char s, p;
  /*
   :  Simple pattern matcher:  '*' matches 0 or more characters, '?' matches
   :  any single character.  Returns TRUE if 'strng' matches 'patrn'.
   */
pmatch_top:
    s = *strng++;  p = *patrn++;	/* get next chars and pre-advance */
    if (!p)			/* end of pattern */
	return((boolean)(s == '\0'));		/* matches iff end of string too */
    else if (p == '*')		/* wildcard reached */
	return((boolean)((!*patrn || pmatch(patrn, strng-1)) ? TRUE :
		s ? pmatch(patrn-1, strng) : FALSE));
    else if (p != s && (p != '?' || !s))  /* check single character */
	return FALSE;		/* doesn't match */
    else				/* return pmatch(patrn, strng); */
	goto pmatch_top;	/* optimize tail recursion */
}

#ifndef STRNCMPI
int
strncmpi(s1, s2, n)	/* case insensitive counted string comparison */
    register const char *s1, *s2;
    register int n; /*(should probably be size_t, which is usually unsigned)*/
{					/*{ aka strncasecmp }*/
    register char t1, t2;

    while (n--) {
	if (!*s2) return (*s1 != 0);	/* s1 >= s2 */
	else if (!*s1) return -1;	/* s1  < s2 */
	t1 = lowc(*s1++);
	t2 = lowc(*s2++);
	if (t1 != t2) return (t1 > t2) ? 1 : -1;
    }
    return 0;				/* s1 == s2 */
}
#endif	/* STRNCMPI */

#ifndef STRSTRI

char *
strstri(str, sub)	/* case insensitive substring search */
    const char *str;
    const char *sub;
{
    register const char *s1, *s2;
    register int i, k;
# define TABSIZ 0x20	/* 0x40 would be case-sensitive */
    char tstr[TABSIZ], tsub[TABSIZ];	/* nibble count tables */
# if 0
    assert( (TABSIZ & ~(TABSIZ-1)) == TABSIZ ); /* must be exact power of 2 */
    assert( &lowc != 0 );			/* can't be unsafe macro */
# endif

    /* special case: empty substring */
    if (!*sub)	return (char *) str;

    /* do some useful work while determining relative lengths */
    for (i = 0; i < TABSIZ; i++)  tstr[i] = tsub[i] = 0;	/* init */
    for (k = 0, s1 = str; *s1; k++)  tstr[*s1++ & (TABSIZ-1)]++;
    for (	s2 = sub; *s2; --k)  tsub[*s2++ & (TABSIZ-1)]++;

    /* evaluate the info we've collected */
    if (k < 0)	return (char *) 0;  /* sub longer than str, so can't match */
    for (i = 0; i < TABSIZ; i++)	/* does sub have more 'x's than str? */
	if (tsub[i] > tstr[i])	return (char *) 0;  /* match not possible */

    /* now actually compare the substring repeatedly to parts of the string */
    for (i = 0; i <= k; i++) {
	s1 = &str[i];
	s2 = sub;
	while (lowc(*s1++) == lowc(*s2++))
	    if (!*s2)  return (char *) &str[i];		/* full match */
    }
    return (char *) 0;	/* not found */
}
#endif	/* STRSTRI */

/* compare two strings for equality, ignoring the presence of specified
   characters (typically whitespace) and possibly ignoring case */
boolean
fuzzymatch(s1, s2, ignore_chars, caseblind)
    const char *s1, *s2;
    const char *ignore_chars;
    boolean caseblind;
{
    register char c1, c2;

    do {
	while ((c1 = *s1++) != '\0' && index(ignore_chars, c1) != 0) continue;
	while ((c2 = *s2++) != '\0' && index(ignore_chars, c2) != 0) continue;
	if (!c1 || !c2) break;	/* stop when end of either string is reached */

	if (caseblind) {
	    c1 = lowc(c1);
	    c2 = lowc(c2);
	}
    } while (c1 == c2);

    /* match occurs only when the end of both strings has been reached */
    return (boolean)(!c1 && !c2);
}

/*
 * Time routines
 *
 * The time is used for:
 *	- seed for rand()
 *	- year on tombstone and yyyymmdd in record file
 *	- phase of the moon (various monsters react to NEW_MOON or FULL_MOON)
 *	- night and midnight (the undead are dangerous at midnight)
 *	- determination of what files are "very old"
 */

/* TIME_type: type of the argument to time(); we actually use &(time_t) */
#if defined(BSD) && !defined(POSIX_TYPES)
# define TIME_type long *
#else
# define TIME_type time_t *
#endif
/* LOCALTIME_type: type of the argument to localtime() */
#if (defined(ULTRIX) && !(defined(ULTRIX_PROTO) || defined(NHSTDC))) || (defined(BSD) && !defined(POSIX_TYPES))
# define LOCALTIME_type long *
#else
# define LOCALTIME_type time_t *
#endif

#if defined(AMIGA) && !defined(AZTEC_C) && !defined(__SASC_60) && !defined(_DCC) && !defined(__GNUC__)
extern struct tm *FDECL(localtime,(time_t *));
#endif
STATIC_DCL struct tm *NDECL(getlt);

void
setrandom()
{
	time_t now = getnow();		/* time((TIME_type) 0) */

	/* the types are different enough here that sweeping the different
	 * routine names into one via #defines is even more confusing
	 */
#ifdef RANDOM	/* srandom() from sys/share/random.c */
	srandom((unsigned int) now);
#else
# if defined(__APPLE__) || defined(BSD) || defined(LINUX) || defined(ULTRIX) || defined(CYGWIN32) /* system srandom() */
#  if defined(BSD) && !defined(POSIX_TYPES) && defined(SUNOS4)
	(void)
#  endif
		srandom((int) now);
# else
#  ifdef UNIX	/* system srand48() */
	srand48((long) now);
#  else		/* poor quality system routine */
	srand((int) now);
#  endif
# endif
#endif
}

time_t
getnow()
{
	time_t datetime = 0;

	(void) time((TIME_type) &datetime);
	return datetime;
}

STATIC_OVL struct tm *
getlt()
{
	time_t date = getnow();

	return localtime((LOCALTIME_type) &date);
}

int
getyear()
{
	return(1900 + getlt()->tm_year);
}

#if 0
/* This routine is no longer used since in 20YY it yields "1YYmmdd". */
char *
yymmdd(date)
time_t date;
{
	Static char datestr[10];
	struct tm *lt;

	if (date == 0)
		lt = getlt();
	else
		lt = localtime((LOCALTIME_type) &date);

	Sprintf(datestr, "%02d%02d%02d",
		lt->tm_year, lt->tm_mon + 1, lt->tm_mday);
	return(datestr);
}
#endif

long
yyyymmdd(date)
time_t date;
{
	long datenum;
	struct tm *lt;

	if (date == 0)
		lt = getlt();
	else
		lt = localtime((LOCALTIME_type) &date);

	/* just in case somebody's localtime supplies (year % 100)
	   rather than the expected (year - 1900) */
	if (lt->tm_year < 70)
	    datenum = (long)lt->tm_year + 2000L;
	else
	    datenum = (long)lt->tm_year + 1900L;
	/* yyyy --> yyyymm */
	datenum = datenum * 100L + (long)(lt->tm_mon + 1);
	/* yyyymm --> yyyymmdd */
	datenum = datenum * 100L + (long)lt->tm_mday;
	return datenum;
}

long
hhmmss(date)
time_t date;
{
	long timenum;
	struct tm *lt;

	if (date == 0)
		lt = getlt();
	else
		lt = localtime((LOCALTIME_type) &date);

	timenum = lt->tm_hour * 10000L + lt->tm_min * 100L + lt->tm_sec;
	return timenum;
}

/*
 * moon period = 29.53058 days ~= 30, year = 365.2422 days
 * days moon phase advances on first day of year compared to preceding year
 *	= 365.2422 - 12*29.53058 ~= 11
 * years in Metonic cycle (time until same phases fall on the same days of
 *	the month) = 18.6 ~= 19
 * moon phase on first day of year (epact) ~= (11*(year%19) + 29) % 30
 *	(29 as initial condition)
 * current phase in days = first day phase + days elapsed in year
 * 6 moons ~= 177 days
 * 177 ~= 8 reported phases * 22
 * + 11/22 for rounding
 */
int
phase_of_the_moon()		/* 0-7, with 0: new, 4: full */
{
	register struct tm *lt = getlt();
	register int epact, diy, goldn;

	diy = lt->tm_yday;
	goldn = (lt->tm_year % 19) + 1;
	epact = (11 * goldn + 18) % 30;
	if ((epact == 25 && goldn > 11) || epact == 24)
		epact++;

	return( (((((diy + epact) * 6) + 11) % 177) / 22) & 7 );
}

boolean
friday_13th()
{
	register struct tm *lt = getlt();

	return((boolean)(lt->tm_wday == 5 /* friday */ && lt->tm_mday == 13));
}

int
night()
{
	register int hour = getlt()->tm_hour;

	return(hour < 6 || hour > 21);
}

int
midnight()
{
	return(getlt()->tm_hour == 0);
}

#ifdef UNICODE_WIDEWINPORT
nhwchar *
nhwstrncpy(dest, strSource, cnt)
nhwchar *dest;
const char *strSource;
size_t cnt;
{
	nhwchar *d = dest;
	const char *s = strSource;
	size_t dcnt = 0;

	while(*s && dcnt < cnt) {
	    *d++ = (nhwchar)*s++;
	    dcnt++;
	}
	if (dcnt < cnt) *d = 0;
	return dest;
}

nhwchar *
nhwncpy(dest, src, cnt)
nhwchar *dest;
const nhwchar *src;
size_t cnt;
{
	nhwchar *d = dest;
	const nhwchar *s = src;
	size_t dcnt = 0;

	while(*s && dcnt < cnt) {
	    *d++ = *s++;
	    dcnt++;
	}
	if (dcnt < cnt) *d = 0;
	return dest;
}

nhwchar *
nhwcpy(dest, src)
nhwchar *dest;
const nhwchar *src;
{
	nhwchar *d = dest;
	const nhwchar *s = src;

	while(*s) {
	    *d++ = *s++;
	}
	*d = 0;
	return dest;
}

nhwchar *
nhwstrcpy(dest, strSource)
nhwchar *dest;
const char *strSource;
{
	nhwchar *d = dest;
	const char *s = strSource;

	while(*s) {
	    *d++ = *s++;
	}
	*d = 0;
	return dest;
}

char *
strnhwcpy(strDest, src)
char *strDest;
const nhwchar *src;
{
	char *d = strDest;
	const nhwchar *s = src;

	while(*s) {
	    *d++ = (char)*s++;
	}
	*d = 0;
	return strDest;
}

nhwchar *
nhwstrcat(dest, strSource)
nhwchar *dest;
const char *strSource;
{
	nhwchar *d = dest;
	const char *s = strSource;

	while(*d) d++;
	while(*s) {
	    *d++ = *s++;
	}
	*d = 0;
	return dest;
}

nhwchar *
nhwcat(dest, src)
nhwchar *dest;
const nhwchar *src;
{
	nhwchar *d = dest;
	const nhwchar *s = src;

	while(*d) d++;
	while(*s) {
	    *d++ = *s++;
	}
	*d = 0;
	return dest;
}

nhwchar *
nhwindex(ss, c)
const nhwchar *ss;
int c;
{
	const nhwchar *s = ss;

	while (*s) {
	    if (*s == c) return (nhwchar *)s;
	    s++;
	}
	if (*s == c) return (nhwchar *)s;
	return (nhwchar *)0;
}

size_t nhwlen(src)
const nhwchar *src;
{
	register size_t dl = 0;

	while(*src++) dl++;
	return dl;
}

int
nhwcmp(s1, s2)	/* case sensitive comparison */
register const nhwchar *s1, *s2;
{
    register nhwchar t1, t2;

    for (;;) {
	if (!*s2) return (*s1 != 0);	/* s1 >= s2 */
	else if (!*s1) return -1;	/* s1  < s2 */
	t1 = *s1++;
	t2 = *s2++;
	if (t1 != t2) return (t1 > t2) ? 1 : -1;
    }
    return 0;				/* s1 == s2 */
}

int
nhwncmp(s1, s2, n)	/* case sensitive counted nhwchar (wide string) comparison */
    register const nhwchar *s1, *s2;
    register int n; /*(should probably be size_t, which is usually unsigned)*/
{
    register nhwchar t1, t2;

    while (n--) {
	if (!*s2) return (*s1 != 0);	/* s1 >= s2 */
	else if (!*s1) return -1;	/* s1  < s2 */
	t1 = *s1++;
	t2 = *s2++;
	if (t1 != t2) return (t1 > t2) ? 1 : -1;
    }
    return 0;				/* s1 == s2 */
}

int
nhwstrcmp(s1, s2)
register const nhwchar *s1;
const char *s2;
{
    register nhwchar t1, t2;

    for (;;) {
	if (!*s2) return (*s1 != 0);	/* s1 >= s2 */
	else if (!*s1) return -1;	/* s1  < s2 */
	t1 = *s1++;
	t2 = (nhwchar)*s2++;
	if (t1 != t2) return (t1 > t2) ? 1 : -1;
    }
    return 0;				/* s1 == s2 */
}
#endif
/*hacklib.c*/
