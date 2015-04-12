/* NetHack 3.5	context.h	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.5	context.h	$Date: 2012/01/10 17:47:16 $  $Revision: 1.15 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */


/* If you change the context structure make sure you increment EDITLEVEL in   */
/* patchlevel.h if needed.                                                    */

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

struct dig_info {		/* apply.c, hack.c */
	int	effort;
	d_level level;
	coord	pos;
	long lastdigtime;
	boolean down, chew, warned, quiet;
};

struct tin_info {
	struct	obj *tin;
	unsigned o_id;		/* o_id of tin in save file */
	int	usedtime, reqtime;
};

struct book_info {
	struct obj *book;	/* last/current book being xscribed */
	unsigned o_id;		/* o_id of book in save file */
	schar delay;		/* moves left for this spell */
};

struct takeoff_info {
	long mask;
	long what;
	int delay;
	boolean cancelled_don;
	char disrobing[CONTEXTVERBSZ+1];
};

struct victual_info {
	struct	obj *piece;	/* the thing being eaten, or last thing that
				 * was partially eaten, unless that thing was
				 * a tin, which uses the tin structure above,
				 * in which case this should be 0 */
	unsigned o_id;		/* o_id of food object in save file */
	/* doeat() initializes these when piece is valid */
	int	usedtime,	/* turns spent eating */
		reqtime;	/* turns required to eat */
	int	nmod;		/* coded nutrition per turn */
	Bitfield(canchoke,1);	/* was satiated at beginning */

	/* start_eating() initializes these */
	Bitfield(fullwarn,1);	/* have warned about being full */
	Bitfield(eating,1);	/* victual currently being eaten */
	Bitfield(doreset,1);	/* stop eating at end of turn */
};

struct warntype_info {
	unsigned long obj;	/* object warn_of_mon monster type M2 */
	unsigned long polyd;	/* warn_of_mon monster type M2 due to poly */
	struct permonst *species;	/* particular species due to poly */
	short speciesidx;	/* index of above in mons[] (for save/restore) */
};

struct polearm_info {
	struct monst *hitmon;	/* the monster we tried to hit last */
	unsigned m_id;		/* monster id of hitmon, in save file */
};

struct tribute_info {
	size_t tributesz;	/* make it possible to skip this in future */
	boolean  enabled;	/* Do we have tributes turned on? */
	Bitfield(bookstock,1);	/* Have we stocked the book? */
				/* Markers for other tributes can go here */
	/* 31 free bits */
};
	
struct context_info {
	unsigned ident;		/* social security number for each monster */
	unsigned no_of_wizards; /* 0, 1 or 2 (wizard and his shadow) */
	unsigned run;		/* 0: h (etc), 1: H (etc), 2: fh (etc) */
				/* 3: FH, 4: ff+, 5: ff-, 6: FF+, 7: FF- */
				/* 8: travel */
	unsigned startingpet_mid;
	int	 current_fruit;	/* fruit->fid corresponding to pl_fruit[] */
	int	 warnlevel;
	int	 rndencode;	/* randomized escape sequence introducer */
	long next_attrib_check;	/* next attribute check */
	long stethoscope_move;
	short stethoscope_movement;
	boolean  travel;	/* find way automatically to u.tx,u.ty */
	boolean  travel1;	/* first travel step */
	boolean  forcefight;
	boolean  nopick;	/* do not pickup objects (as when running) */
	boolean  made_amulet;
	boolean  mon_moving;	/* monsters' turn to move */
	boolean  move;
	boolean  mv;
	boolean  bypasses;	/* bypass flag is set on at least one fobj */
	boolean  botl;		/* partially redo status line */
	boolean  botlx;		/* print an entirely new bottom line */
	struct dig_info digging;
	struct victual_info victual;
	struct tin_info tin;
	struct book_info spbook;
	struct takeoff_info takeoff;
	struct warntype_info warntype;
	struct polearm_info polearm;
	struct tribute_info tribute;
};

extern NEARDATA struct context_info context;

#endif /* CONTEXT_H */
