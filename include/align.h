/* NetHack 3.7	align.h	$NHDT-Date: 1604269810 2020/11/01 22:30:10 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.15 $ */
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

/* align masks */
#define AM_NONE         0x00
#define AM_CHAOTIC      0x01
#define AM_NEUTRAL      0x02
#define AM_LAWFUL       0x04
#define AM_MASK         0x07 /* mask for "normal" alignment values */

/* Some altars are considered shrines, add a flag for that
   for the altarmask field of struct rm. */
#define AM_SHRINE       0x08
/* High altar on Astral plane or Moloch's sanctum */
#define AM_SANCTUM      0x10

/* special level flags, gone by the time the level has been loaded */
#define AM_SPLEV_CO     0x20 /* co-aligned: force alignment to match hero's  */
#define AM_SPLEV_NONCO  0x40 /* non-co-aligned: force alignment to not match */
#define AM_SPLEV_RANDOM 0x80

#define Amask2align(x) \
    ((aligntyp) ((((x) & AM_MASK) == 0) ? A_NONE                \
                 : (((x) & AM_MASK) == AM_LAWFUL) ? A_LAWFUL    \
                   : ((int) ((x) & AM_MASK)) - 2)) /* 2 => 0, 1 => -1 */
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
