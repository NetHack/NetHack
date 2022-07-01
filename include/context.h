/* NetHack 3.7	context.h	$NHDT-Date: 1646428003 2022/03/04 21:06:43 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.45 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2006. */
/* NetHack may be freely redistributed.  See license for details. */

/* If you change the context structure make sure you increment EDITLEVEL in */
/* patchlevel.h if needed. */

#ifndef CONTEXT_H
#define CONTEXT_H

#define CONTEXTVERBSZ 30

/*
 * The context structure houses things that the game tracks
 * or adjusts during the game, to preserve game state or context.
 *
 * The entire structure is saved with the game.
 *
 */

struct dig_info { /* apply.c, hack.c */
    int effort;
    d_level level;
    coord pos;
    long lastdigtime;
    boolean down, chew, warned, quiet;
};

struct tin_info {
    struct obj *tin;
    unsigned o_id; /* o_id of tin in save file */
    int usedtime, reqtime;
};

struct book_info {
    struct obj *book; /* last/current book being xscribed */
    unsigned o_id;    /* o_id of book in save file */
    schar delay;      /* moves left for this spell */
};

struct takeoff_info {
    long mask;
    long what;
    int delay;
    boolean cancelled_don;
    char disrobing[CONTEXTVERBSZ + 1];
};

struct victual_info {
    struct obj *piece; /* the thing being eaten, or last thing that
                        * was partially eaten, unless that thing was
                        * a tin, which uses the tin structure above,
                        * in which case this should be 0 */
    unsigned o_id;     /* o_id of food object in save file */
    /* doeat() initializes these when piece is valid */
    int usedtime,          /* turns spent eating */
        reqtime;           /* turns required to eat */
    int nmod;              /* coded nutrition per turn */
    Bitfield(canchoke, 1); /* was satiated at beginning */

    /* start_eating() initializes these */
    Bitfield(fullwarn, 1); /* have warned about being full */
    Bitfield(eating, 1);   /* victual currently being eaten */
    Bitfield(doreset, 1);  /* stop eating at end of turn */
};

struct engrave_info {
    char text[BUFSZ];   /* actual text being engraved - doengrave() handles all
                           the possible mutations of this */
    char *nextc;        /* next character(s) in text[] to engrave */
    struct obj *stylus; /* object doing the writing */
    xint8 type;         /* type of engraving (DUST, MARK, etc) */
    coord pos;          /* location the engraving is being placed on */
    int actionct;       /* nth turn spent engraving */
};

struct warntype_info {
    unsigned long obj;        /* object warn_of_mon monster type M2 */
    unsigned long polyd;      /* warn_of_mon monster type M2 due to poly */
    struct permonst *species; /* particular species due to poly */
    short speciesidx; /* index of above in mons[] (for save/restore) */
};

struct polearm_info {
    struct monst *hitmon; /* the monster we tried to hit last */
    unsigned m_id;        /* monster id of hitmon, in save file */
};

struct obj_split {
    unsigned parent_oid, /* set: splitobj(),         */
             child_oid;  /* reset: clear_splitobjs() */
};

struct tribute_info {
    size_t tributesz;       /* make it possible to skip this in future */
    boolean enabled;        /* Do we have tributes turned on? */
    Bitfield(bookstock, 1); /* Have we stocked the book? */
    Bitfield(Deathnotice,1);    /* Did Death notice the book? */
    /* Markers for other tributes can go here */
    /* 30 free bits */
};

struct novel_tracking { /* for choosing random passage when reading novel */
    unsigned id;        /* novel oid from previous passage selection */
    int count;          /* number of passage indices available in pasg[] */
    xint8 pasg[30];     /* pasg[0..count-1] are passage indices */
    /* tribute file is allowed to have more than 30 passages for a novel;
       if it does, reading will first choose a random subset of 30 of them;
       reading all 30 or switching to a different novel and then back again
       will pick a new subset, independent of previous ones so might not
       contain all--or even any--of the ones left out of the prior subset;
       chatting with Death works the same as reading a novel except that
       sometimes he delivers one of a few hardcoded messages rather than a
       passage from the Death Quotes section of dat/tribute */
};

struct achievement_tracking {
    unsigned mines_prize_oid,   /* luckstone->o_id */
             soko_prize_oid,    /* {bag or amulet}->o_id */
             castle_prize_old;  /* wand->o_id; not yet implemented */
    /* record_achievement() wants the item type for livelog() event */
    short    mines_prize_otyp,  /* luckstone */
             soko_prize_otyp,   /* bag of holding or amulet of reflection */
             castle_prize_otyp; /* strange object (someday wand of wishing) */
    boolean minetn_reached;     /* avoid redundant checking for town entry */
};

struct context_info {
    unsigned ident;         /* social security number for each monster */
    unsigned no_of_wizards; /* 0, 1 or 2 (wizard and his shadow) */
    unsigned run;           /* 0: h (etc), 1: H (etc), 2: fh (etc) */
                            /* 3: FH, 4: ff+, 5: ff-, 6: FF+, 7: FF- */
                            /* 8: travel */
    unsigned startingpet_mid; /* monster id number for initial pet */
    int current_fruit;      /* fruit->fid corresponding to g.pl_fruit[] */
    int mysteryforce;       /* adjusts how often "mysterious force" kicks in */
    int rndencode;          /* randomized escape sequence introducer */
    int warnlevel;          /* threshold (digit) to warn about unseen mons */
    long next_attrib_check; /* next attribute check */
    long seer_turn;         /* when random clairvoyance will next kick in */
    long stethoscope_seq;   /* when a stethoscope was last used; first use
                             * during a move takes no time, second uses move */
    boolean travel;  /* find way automatically to u.tx,u.ty */
    boolean travel1; /* first travel step */
    boolean forcefight;
    boolean nopick; /* do not pickup objects (as when running) */
    boolean made_amulet;
    boolean mon_moving; /* monsters' turn to move */
    boolean move;
    boolean mv;
    boolean bypasses;    /* bypass flag is set on at least one fobj */
    boolean botl;        /* partially redo status line */
    boolean botlx;       /* print an entirely new bottom line */
    boolean door_opened; /* set to true if door was opened during test_move */
    boolean enhance_tip; /* player is informed about #enhance */
    boolean swim_tip;    /* player was informed about walking into water */
    struct dig_info digging;
    struct victual_info victual;
    struct engrave_info engraving;
    struct tin_info tin;
    struct book_info spbook;
    struct takeoff_info takeoff;
    struct warntype_info warntype;
    struct polearm_info polearm;
    struct obj_split objsplit; /* track most recently split object stack */
    struct tribute_info tribute;
    struct novel_tracking novel;
    struct achievement_tracking achieveo;
};

#endif /* CONTEXT_H */
