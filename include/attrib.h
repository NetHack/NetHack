/* NetHack 3.6	attrib.h	$NHDT-Date: 1432512779 2015/05/25 00:12:59 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/* Copyright 1988, Mike Stephenson				  */
/* NetHack may be freely redistributed.  See license for details. */

/*	attrib.h - Header file for character class processing. */

#ifndef ATTRIB_H
#define ATTRIB_H

enum attrib_types {
    A_STR = 0,
    A_INT,
    A_WIS,
    A_DEX,
    A_CON,
    A_CHA,

    A_MAX /* used in rn2() selection of attrib */
};

#define ABASE(x) (u.acurr.a[x])
#define ABON(x) (u.abon.a[x])
#define AEXE(x) (u.aexe.a[x])
#define ACURR(x) (acurr(x))
#define ACURRSTR (acurrstr())
/* should be: */
/* #define ACURR(x) (ABON(x) + ATEMP(x) + (Upolyd  ? MBASE(x) : ABASE(x)) */
#define MCURR(x) (u.macurr.a[x])
#define AMAX(x) (u.amax.a[x])
#define MMAX(x) (u.mamax.a[x])

#define ATEMP(x) (u.atemp.a[x])
#define ATIME(x) (u.atime.a[x])

/* KMH -- Conveniences when dealing with strength constants */
#define STR18(x) (18 + (x))  /* 18/xx */
#define STR19(x) (100 + (x)) /* For 19 and above */

struct attribs {
    schar a[A_MAX];
};

#define ATTRMAX(x)                                        \
    ((x == A_STR && Upolyd && strongmonst(g.youmonst.data)) \
         ? STR18(100)                                     \
         : g.urace.attrmax[x])
#define ATTRMIN(x) (g.urace.attrmin[x])

#endif /* ATTRIB_H */
