/* NetHack 3.7	align.h	$NHDT-Date: 1596498525 2020/08/03 23:48:45 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.11 $ */
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
/* Some altars are considered as shrines, so we need a flag. */
#define AM_SHRINE 8

#define AM_SPLEV_CO 3
#define AM_SPLEV_NONCO 7
#define AM_SPLEV_RANDOM 8

#define Amask2align(x)                                          \
    ((aligntyp)((!(x)) ? A_NONE : ((x) == AM_LAWFUL) ? A_LAWFUL \
                                                     : ((int) x) - 2))
#define Align2amask(x) \
    (((x) == A_NONE) ? AM_NONE : ((x) == A_LAWFUL) ? AM_LAWFUL : (x) + 2)

/* Because clearly Nethack needs more ways to specify alignment.
   The Amask2msa AM_LAWFUL check needs to mask with AM_MASK to
   strip off possible AM_SHRINE bit */
#define Amask2msa(x) (((x) & AM_MASK) == 4 ? 3 : (x) & AM_MASK)
#define Msa2amask(x) ((x) == 3 ? 4 : (x))
#define MSA_NONE    0  /* unaligned or multiple alignments */
#define MSA_LAWFUL  1
#define MSA_NEUTRAL 2
#define MSA_CHAOTIC 3

#endif /* ALIGN_H */
