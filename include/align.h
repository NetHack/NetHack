/* NetHack 3.7	align.h	$NHDT-Date: 1604105154 2020/10/31 00:45:54 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.14 $ */
/* Copyright (c) Mike Stephenson, Izchak Miller  1991.		  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef ALIGN_H
#define ALIGN_H

typedef schar aligntyp; /* basic alignment type */

typedef struct align { /* alignment & record */
    aligntyp type;
    int record;
} align;

/* bounds for "record" -- respect initial alignments of 10 */
#define ALIGNLIM (10L + (g.moves / 200L))

#define A_NONE (-128) /* the value range of type */

#define A_CHAOTIC (-1)
#define A_NEUTRAL 0
#define A_LAWFUL 1

#define A_COALIGNED 1
#define A_OPALIGNED (-1)

#define AM_NONE 0
#define AM_CHAOTIC 1
#define AM_NEUTRAL 2
#define AM_LAWFUL 4

#define AM_MASK 7
/* Some altars are considered as shrines, so we need a flag for that
   for the altarmask field of struct rm. */
#define AM_SHRINE 8

/* special level flags, gone by the time the level has been loaded */
#define AM_SPLEV_CO     3 /* co-aligned: force alignment to match hero's  */
#define AM_SPLEV_NONCO  7 /* non-co-aligned: force alignment to not match */
#define AM_SPLEV_RANDOM 8

#define Amask2align(x) \
    ((aligntyp) ((((x) & AM_MASK) == 0) ? A_NONE                \
                 : (((x) & AM_MASK) == AM_LAWFUL) ? A_LAWFUL    \
                   : ((int) (x) - 2))) /* 2 => 0, 1 => -1 */
#define Align2amask(x) \
    ((unsigned) (((x) == A_NONE) ? AM_NONE                      \
                 : ((x) == A_LAWFUL) ? AM_LAWFUL                \
                   : ((x) + 2))) /* -1 => 1, 0 => 2 */

/* Because clearly Nethack needs more ways to specify alignment...
   Amask2msa(): 1, 2, 4 converted to 1, 2, 3 to fit within a width 2 bitfield;
   Msa2amask(): 1, 2, 3 converted back to 1, 2, 4;
   For Amask2msa(), 'x' might have the shrine bit set so strip that off. */
#define Amask2msa(x) ((((x) & AM_MASK) == 4) ? 3 : (x) & AM_MASK)
#define Msa2amask(x) (((x) == 3) ? 4 : (x))
#define MSA_NONE    0  /* unaligned or multiple alignments */

#endif /* ALIGN_H */
