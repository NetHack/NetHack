/*	SCCS Id: @(#)lev.h	3.4	1994/03/18	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*	Common include file for save and restore routines */

#ifndef LEV_H
#define LEV_H

#define COUNT_SAVE	0x1
#define WRITE_SAVE	0x2
#define FREE_SAVE	0x4

/* operations of the various saveXXXchn & co. routines */
#define perform_bwrite(mode)	((mode) & (COUNT_SAVE|WRITE_SAVE))
#define release_data(mode)	((mode) & FREE_SAVE)

/* The following are used in mkmaze.c */
struct container {
	struct container *next;
	xchar x, y;
	short what;
	genericptr_t list;
};

#define CONS_OBJ   0
#define CONS_MON   1
#define CONS_HERO  2
#define CONS_TRAP  3

struct bubble {
	xchar x, y;	/* coordinates of the upper left corner */
	schar dx, dy;	/* the general direction of the bubble's movement */
	uchar *bm;	/* pointer to the bubble bit mask */
	struct bubble *prev, *next; /* need to traverse the list up and down */
	struct container *cons;
};

/* used in light.c */
typedef struct ls_t {
    struct ls_t *next;
    xchar x, y;		/* source's position */
    short range;	/* source's current range */
    short flags;
    short type;		/* type of light source */
    genericptr_t id;	/* source's identifier */
} light_source;

#endif /* LEV_H */
