/* NetHack 3.6	lev.h	$NHDT-Date: 1432512781 2015/05/25 00:13:01 $  $NHDT-Branch: master $:$NHDT-Revision: 1.12 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2006. */
/* NetHack may be freely redistributed.  See license for details. */

/*	Common include file for save and restore routines */

#ifndef LEV_H
#define LEV_H

#define COUNT_SAVE 0x1
#define WRITE_SAVE 0x2
#define FREE_SAVE 0x4

#define MAX_BMASK 4

/* operations of the various saveXXXchn & co. routines */
#define perform_bwrite(mode) ((mode) & (COUNT_SAVE | WRITE_SAVE))
#define release_data(mode) ((mode) &FREE_SAVE)

/* The following are used in mkmaze.c */
struct container {
    struct container *next;
    xchar x, y;
    short what;
    genericptr_t list;
};

enum bubble_contains_types {
    CONS_OBJ = 0,
    CONS_MON,
    CONS_HERO,
    CONS_TRAP
};

struct bubble {
    xchar x, y;   /* coordinates of the upper left corner */
    schar dx, dy; /* the general direction of the bubble's movement */
    uchar bm[MAX_BMASK + 2];    /* bubble bit mask */
    struct bubble *prev, *next; /* need to traverse the list up and down */
    struct container *cons;
};

/* used in light.c */
typedef struct ls_t {
    struct ls_t *next;
    xchar x, y;  /* source's position */
    short range; /* source's current range */
    short flags;
    short type;  /* type of light source */
    anything id; /* source's identifier */
} light_source;

#endif /* LEV_H */
