/*	SCCS Id: @(#)mextra.h	3.5	2006/01/03	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MEXTRA_H
#define MEXTRA_H

#ifndef ALIGN_H
#include "align.h"
#endif

#define FCSIZ	(ROWNO+COLNO)

struct fakecorridor {
	xchar fx,fy,ftyp;
};

struct egd {
	int fcbeg, fcend;	/* fcend: first unused pos */
	int vroom;		/* room number of the vault */
	xchar gdx, gdy;		/* goal of guard's walk */
	xchar ogx, ogy;		/* guard's last position */
	d_level gdlevel;	/* level (& dungeon) guard was created in */
	xchar warncnt;		/* number of warnings to follow */
	Bitfield(gddone,1);	/* true iff guard has released player */
	Bitfield(unused,7);
	struct fakecorridor fakecorr[FCSIZ];
};

struct epri {
	aligntyp shralign;	/* alignment of priest's shrine */
				/* leave as first field to match emin */
	schar shroom;		/* index in rooms */
	coord shrpos;		/* position of shrine */
	d_level shrlevel;	/* level (& dungeon) of shrine */
};

/* note: roaming priests (no shrine) switch from ispriest to isminion
   (and emin extension) */

#define REPAIR_DELAY	5	/* minimum delay between shop damage & repair */
#define BILLSZ	200

struct bill_x {
	unsigned bo_id;
	boolean useup;
	long price;		/* price per unit */
	long bquan;		/* amount used up */
};

struct eshk {
	long robbed;		/* amount stolen by most recent customer */
	long credit;		/* amount credited to customer */
	long debit;		/* amount of debt for using unpaid items */
	long loan;		/* shop-gold picked (part of debit) */
	int shoptype;		/* the value of rooms[shoproom].rtype */
	schar shoproom;		/* index in rooms; set by inshop() */
	schar unused;		/* to force alignment for stupid compilers */
	boolean following;	/* following customer since he owes us sth */
	boolean surcharge;	/* angry shk inflates prices */
	coord shk;		/* usual position shopkeeper */
	coord shd;		/* position shop door */
	d_level shoplevel;	/* level (& dungeon) of his shop */
	int billct;		/* no. of entries of bill[] in use */
	struct bill_x bill[BILLSZ];
	struct bill_x *bill_p;
	int visitct;		/* nr of visits by most recent customer */
	char customer[PL_NSIZ]; /* most recent customer */
	char shknam[PL_NSIZ];
};

#define NOTANGRY(mon)	((mon)->mpeaceful)
#define ANGRY(mon)	(!NOTANGRY(mon))

struct emin {
	aligntyp min_align;	/* alignment of minion */
	boolean  renegade;	/* hostile co-aligned priest or Angel */
};

/*	various types of pet food, the lower, the better liked.	*/

#define DOGFOOD 0
#define CADAVER 1
#define ACCFOOD 2
#define MANFOOD 3
#define APPORT	4
#define POISON	5
#define UNDEF	6
#define TABU	7

struct edog {
	long droptime;			/* moment dog dropped object */
	unsigned dropdist;		/* dist of drpped obj from @ */
	int apport;			/* amount of training */
	long whistletime;		/* last time he whistled */
	long hungrytime;		/* will get hungry at this time */
	coord ogoal;			/* previous goal location */
	int abuse;			/* track abuses to this pet */
	int revivals;			/* count pet deaths */
	int mhpmax_penalty;		/* while starving, points reduced */
	Bitfield(killed_by_u, 1);	/* you attempted to kill him */
};

struct mextra {
	char *mname;
	struct egd *egd;
	struct epri *epri;
	struct eshk *eshk;
	struct emin *emin;
	struct edog *edog;
};

#define MNAME(mon)	((mon)->mextra->mname)
#define EGD(mon)	((mon)->mextra->egd)
#define EPRI(mon)	((mon)->mextra->epri)
#define ESHK(mon)	((mon)->mextra->eshk)
#define EMIN(mon)	((mon)->mextra->emin)
#define EDOG(mon)	((mon)->mextra->edog)
#define has_name(mon)	((mon)->mextra && MNAME(mon))

#endif /* MEXTRA_H */
