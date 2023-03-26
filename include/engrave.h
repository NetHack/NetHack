/* NetHack 3.7	engrave.h	$NHDT-Date: 1596498535 2020/08/03 23:48:55 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.10 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Kenneth Lorber, Kensington, Maryland, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef ENGRAVE_H
#define ENGRAVE_H

enum engraving_texts { actual_text, remembered_text, pristine_text, text_states };

struct engr {
    struct engr *nxt_engr;
    char *engr_txt[text_states];
    coordxy engr_x, engr_y;
    unsigned engr_szeach;  /* length of text including trailing NUL */
    unsigned engr_alloc; /* for save & restore; not length of text */
    long engr_time;    /* moment engraving was (will be) finished */
    xint8 engr_type;
#define DUST 1
#define ENGRAVE 2
#define BURN 3
#define MARK 4
#define ENGR_BLOOD 5
#define HEADSTONE 6
#define N_ENGRAVE 6
    Bitfield(guardobjects, 1); /* if engr_txt is "Elbereth", it is effective
                                * against monsters when an object is present
                                * even when hero isn't (so behaves similarly
                                * to how Elbereth did in 3.4.3) */
    Bitfield(nowipeout, 1);    /* this engraving will not degrade */
    Bitfield(eread, 1);        /* the engraving text has been read or felt */
    /* 5 free bits */
};

#define newengr(lth) \
    (struct engr *) alloc((unsigned) (lth) + (unsigned) sizeof (struct engr))
#define dealloc_engr(engr) free((genericptr_t) (engr))

#define engraving_to_defsym(ep) \
    (levl[(ep)->engr_x][(ep)->engr_y].typ == CORR ? S_engrcorr : S_engroom)

#define spot_shows_engravings(x,y)      \
    (levl[(x)][(y)].typ == CORR         \
     || levl[(x)][(y)].typ == SCORR     \
     || levl[(x)][(y)].typ == ICE       \
     || levl[(x)][(y)].typ == ROOM )

#endif /* ENGRAVE_H */
