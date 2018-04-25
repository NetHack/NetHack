/* NetHack 3.6	engrave.h	$NHDT-Date: 1432512777 2015/05/25 00:12:57 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Kenneth Lorber, Kensington, Maryland, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef ENGRAVE_H
#define ENGRAVE_H

struct engr {
    struct engr *nxt_engr;
    char *engr_txt;
    xchar engr_x, engr_y;
    unsigned engr_lth; /* for save & restore; not length of text */
    long engr_time;    /* moment engraving was (will be) finished */
    xchar engr_type;
#define DUST 1
#define ENGRAVE 2
#define BURN 3
#define MARK 4
#define ENGR_BLOOD 5
#define HEADSTONE 6
#define N_ENGRAVE 6
};

#define newengr(lth) \
    (struct engr *) alloc((unsigned)(lth) + sizeof(struct engr))
#define dealloc_engr(engr) free((genericptr_t)(engr))

#endif /* ENGRAVE_H */
