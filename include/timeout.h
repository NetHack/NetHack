/* NetHack 3.7	timeout.h	$NHDT-Date: 1596498564 2020/08/03 23:49:24 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.13 $ */
/* Copyright 1994, Dean Luick                                     */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef TIMEOUT_H
#define TIMEOUT_H

/* generic timeout function */
typedef void (*timeout_proc)(ANY_P *, long);

/* kind of timer */
enum timer_type {
    TIMER_LEVEL = 0,   /* event specific to level [melting ice] */
    TIMER_GLOBAL = 1,  /* event follows current play [not used] */
    TIMER_OBJECT = 2,  /* event follows an object [various] */
    TIMER_MONSTER = 3, /* event follows a monster [not used] */
    NUM_TIMER_KINDS    /* 4 */
};

/* save/restore timer ranges */
#define RANGE_LEVEL 0  /* save/restore timers staying on level */
#define RANGE_GLOBAL 1 /* save/restore timers following global play */

/*
 * Timeout functions.  Add a define here, then put it in the table
 * in timeout.c.  "One more level of indirection will fix everything."
 */
enum timeout_types {
    ROT_ORGANIC = 0, /* for buried organics */
    ROT_CORPSE,
    REVIVE_MON,
    ZOMBIFY_MON,
    BURN_OBJECT,
    HATCH_EGG,
    FIG_TRANSFORM,
    MELT_ICE_AWAY,
    SHRINK_GLOB,

    NUM_TIME_FUNCS
};

#define timer_is_pos(ttype) ((ttype) == MELT_ICE_AWAY)
#define timer_is_obj(ttype) ((ttype) == ROT_ORGANIC      \
                             || (ttype) == ROT_CORPSE    \
                             || (ttype) == REVIVE_MON    \
                             || (ttype) == ZOMBIFY_MON   \
                             || (ttype) == BURN_OBJECT   \
                             || (ttype) == HATCH_EGG     \
                             || (ttype) == FIG_TRANSFORM \
                             || (ttype) == SHRINK_GLOB)

/* used in timeout.c */
typedef struct fe {
    struct fe *next;          /* next item in chain */
    long timeout;             /* when we time out */
    unsigned long tid;        /* timer ID */
    short kind;               /* kind of use */
    short func_index;         /* what to call when we time out */
    anything arg;             /* pointer to timeout argument */
    Bitfield(needs_fixup, 1); /* does arg need to be patched? */
} timer_element;

#endif /* TIMEOUT_H */
