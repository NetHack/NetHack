/*	SCCS Id: @(#)dbridge.c	3.4	2000/02/05	*/
/*	Copyright (c) 1989 by Jean-Christophe Collet		  */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file contains the drawbridge manipulation (create, open, close,
 * destroy).
 *
 * Added comprehensive monster-handling, and the "entity" structure to
 * deal with players as well. - 11/89
 */

#include "hack.h"

#ifdef OVLB
STATIC_DCL void FDECL(get_wall_for_db, (int *, int *));
STATIC_DCL struct entity *FDECL(e_at, (int, int));
STATIC_DCL void FDECL(m_to_e, (struct monst *, int, int, struct entity *));
STATIC_DCL void FDECL(u_to_e, (struct entity *));
STATIC_DCL void FDECL(set_entity, (int, int, struct entity *));
STATIC_DCL const char *FDECL(e_nam, (struct entity *));
#ifdef D_DEBUG
static const char *FDECL(Enam, (struct entity *)); /* unused */
#endif
STATIC_DCL const char *FDECL(E_phrase, (struct entity *, const char *));
STATIC_DCL boolean FDECL(e_survives_at, (struct entity *, int, int));
STATIC_DCL void FDECL(e_died, (struct entity *, int, int));
STATIC_DCL boolean FDECL(automiss, (struct entity *));
STATIC_DCL boolean FDECL(e_missed, (struct entity *, BOOLEAN_P));
STATIC_DCL boolean FDECL(e_jumps, (struct entity *));
STATIC_DCL void FDECL(do_entity, (struct entity *));
#endif /* OVLB */

#ifdef OVL0

boolean
is_pool(x,y)
int x,y;
{
    schar ltyp;

    if (!isok(x,y)) return FALSE;
    ltyp = levl[x][y].typ;
    if (ltyp == POOL || ltyp == MOAT || ltyp == WATER) return TRUE;
    if (ltyp == DRAWBRIDGE_UP &&
	(levl[x][y].drawbridgemask & DB_UNDER) == DB_MOAT) return TRUE;
    return FALSE;
}

boolean
is_lava(x,y)
int x,y;
{
    schar ltyp;

    if (!isok(x,y)) return FALSE;
    ltyp = levl[x][y].typ;
    if (ltyp == LAVAPOOL
	|| (ltyp == DRAWBRIDGE_UP
	    && (levl[x][y].drawbridgemask & DB_UNDER) == DB_LAVA)) return TRUE;
    return FALSE;
}

boolean
is_ice(x,y)
int x,y;
{
    schar ltyp;

    if (!isok(x,y)) return FALSE;
    ltyp = levl[x][y].typ;
    if (ltyp == ICE
	|| (ltyp == DRAWBRIDGE_UP
	    && (levl[x][y].drawbridgemask & DB_UNDER) == DB_ICE)) return TRUE;
    return FALSE;
}

#endif /* OVL0 */

#ifdef OVL1

/*
 * We want to know whether a wall (or a door) is the portcullis (passageway)
 * of an eventual drawbridge.
 *
 * Return value:  the direction of the drawbridge.
 */

int
is_drawbridge_wall(x,y)
int x,y;
{
	struct rm *lev;

	lev = &levl[x][y];
	if (lev->typ != DOOR && lev->typ != DBWALL)
		return (-1);

	if (IS_DRAWBRIDGE(levl[x+1][y].typ) &&
	    (levl[x+1][y].drawbridgemask & DB_DIR) == DB_WEST)
		return (DB_WEST);
	if (IS_DRAWBRIDGE(levl[x-1][y].typ) &&
	    (levl[x-1][y].drawbridgemask & DB_DIR) == DB_EAST)
		return (DB_EAST);
	if (IS_DRAWBRIDGE(levl[x][y-1].typ) &&
	    (levl[x][y-1].drawbridgemask & DB_DIR) == DB_SOUTH)
		return (DB_SOUTH);
	if (IS_DRAWBRIDGE(levl[x][y+1].typ) &&
	    (levl[x][y+1].drawbridgemask & DB_DIR) == DB_NORTH)
		return (DB_NORTH);

	return (-1);
}

/*
 * Use is_db_wall where you want to verify that a
 * drawbridge "wall" is UP in the location x, y
 * (instead of UP or DOWN, as with is_drawbridge_wall).
 */
boolean
is_db_wall(x,y)
int x,y;
{
	return((boolean)( levl[x][y].typ == DBWALL ));
}


/*
 * Return true with x,y pointing to the drawbridge if x,y initially indicate
 * a drawbridge or drawbridge wall.
 */
boolean
find_drawbridge(x,y)
int *x,*y;
{
	int dir;

	if (IS_DRAWBRIDGE(levl[*x][*y].typ))
		return TRUE;
	dir = is_drawbridge_wall(*x,*y);
	if (dir >= 0) {
		switch(dir) {
			case DB_NORTH: (*y)++; break;
			case DB_SOUTH: (*y)--; break;
			case DB_EAST:  (*x)--; break;
			case DB_WEST:  (*x)++; break;
		}
		return TRUE;
	}
	return FALSE;
}

#endif /* OVL1 */
#ifdef OVLB

/*
 * Find the drawbridge wall associated with a drawbridge.
 */
STATIC_OVL void
get_wall_for_db(x,y)
int *x,*y;
{
	switch (levl[*x][*y].drawbridgemask & DB_DIR) {
		case DB_NORTH: (*y)--; break;
		case DB_SOUTH: (*y)++; break;
		case DB_EAST:  (*x)++; break;
		case DB_WEST:  (*x)--; break;
	}
}

/*
 * Creation of a drawbridge at pos x,y.
 *     dir is the direction.
 *     flag must be put to TRUE if we want the drawbridge to be opened.
 */

boolean
create_drawbridge(x,y,dir,flag)
int x,y,dir;
boolean flag;
{
	int x2,y2;
	boolean horiz;
	boolean lava = levl[x][y].typ == LAVAPOOL; /* assume initialized map */

	x2 = x; y2 = y;
	switch(dir) {
		case DB_NORTH:
			horiz = TRUE;
			y2--;
			break;
		case DB_SOUTH:
			horiz = TRUE;
			y2++;
			break;
		case DB_EAST:
			horiz = FALSE;
			x2++;
			break;
		default:
			impossible("bad direction in create_drawbridge");
			/* fall through */
		case DB_WEST:
			horiz = FALSE;
			x2--;
			break;
	}
	if (!IS_WALL(levl[x2][y2].typ))
		return(FALSE);
	if (flag) {             /* We want the bridge open */
		levl[x][y].typ = DRAWBRIDGE_DOWN;
		levl[x2][y2].typ = DOOR;
		levl[x2][y2].doormask = D_NODOOR;
	} else {
		levl[x][y].typ = DRAWBRIDGE_UP;
		levl[x2][y2].typ = DBWALL;
		/* Drawbridges are non-diggable. */
		levl[x2][y2].wall_info = W_NONDIGGABLE;
	}
	levl[x][y].horizontal = !horiz;
	levl[x2][y2].horizontal = horiz;
	levl[x][y].drawbridgemask = dir;
	if(lava) levl[x][y].drawbridgemask |= DB_LAVA;
	return(TRUE);
}

struct entity {
	struct monst *emon;	  /* youmonst for the player */
	struct permonst *edata;   /* must be non-zero for record to be valid */
	int ex, ey;
};

#define ENTITIES 2

static NEARDATA struct entity occupants[ENTITIES];

STATIC_OVL
struct entity *
e_at(x, y)
int x, y;
{
	int entitycnt;

	for (entitycnt = 0; entitycnt < ENTITIES; entitycnt++)
		if ((occupants[entitycnt].edata) &&
		    (occupants[entitycnt].ex == x) &&
		    (occupants[entitycnt].ey == y))
			break;
#ifdef D_DEBUG
	pline("entitycnt = %d", entitycnt);
	wait_synch();
#endif
	return((entitycnt == ENTITIES)?
	       (struct entity *)0 : &(occupants[entitycnt]));
}

STATIC_OVL void
m_to_e(mtmp, x, y, etmp)
struct monst *mtmp;
int x, y;
struct entity *etmp;
{
	etmp->emon = mtmp;
	if (mtmp) {
		etmp->ex = x;
		etmp->ey = y;
		if (mtmp->wormno && (x != mtmp->mx || y != mtmp->my))
			etmp->edata = &mons[PM_LONG_WORM_TAIL];
		else
			etmp->edata = mtmp->data;
	} else
		etmp->edata = (struct permonst *)0;
}

STATIC_OVL void
u_to_e(etmp)
struct entity *etmp;
{
	etmp->emon = &youmonst;
	etmp->ex = u.ux;
	etmp->ey = u.uy;
	etmp->edata = youmonst.data;
}

STATIC_OVL void
set_entity(x, y, etmp)
int x, y;
struct entity *etmp;
{
	if ((x == u.ux) && (y == u.uy))
		u_to_e(etmp);
	else if (MON_AT(x, y))
		m_to_e(m_at(x, y), x, y, etmp);
	else
		etmp->edata = (struct permonst *)0;
}

#define is_u(etmp) (etmp->emon == &youmonst)
#define e_canseemon(etmp) (is_u(etmp) ? (boolean)TRUE : canseemon(etmp->emon))

/*
 * e_strg is a utility routine which is not actually in use anywhere, since
 * the specialized routines below suffice for all current purposes.
 */

/* #define e_strg(etmp, func) (is_u(etmp)? (char *)0 : func(etmp->emon)) */

STATIC_OVL const char *
e_nam(etmp)
struct entity *etmp;
{
	return(is_u(etmp)? "you" : mon_nam(etmp->emon));
}

#ifdef D_DEBUG
/*
 * Enam is another unused utility routine:  E_phrase is preferable.
 */

static const char *
Enam(etmp)
struct entity *etmp;
{
	return(is_u(etmp)? "You" : Monnam(etmp->emon));
}
#endif /* D_DEBUG */

/*
 * Generates capitalized entity name, makes 2nd -> 3rd person conversion on
 * verb, where necessary.
 */

STATIC_OVL const char *
E_phrase(etmp, verb)
struct entity *etmp;
const char *verb;
{
	static char wholebuf[80];

	Strcpy(wholebuf, is_u(etmp) ? "You" : Monnam(etmp->emon));
	if (!*verb) return(wholebuf);
	Strcat(wholebuf, " ");
	if (is_u(etmp))
	    Strcat(wholebuf, verb);
	else
	    Strcat(wholebuf, vtense((char *)0, verb));
	return(wholebuf);
}

/*
 * Simple-minded "can it be here?" routine
 */

STATIC_OVL boolean
e_survives_at(etmp, x, y)
struct entity *etmp;
int x, y;
{
	if (noncorporeal(etmp->edata))
		return(TRUE);
	if (is_pool(x, y))
		return (boolean)((is_u(etmp) &&
				(Wwalking || Amphibious || Swimming ||
				Flying || Levitation)) ||
			is_swimmer(etmp->edata) || is_flyer(etmp->edata) ||
			is_floater(etmp->edata));
	/* must force call to lava_effects in e_died if is_u */
	if (is_lava(x, y))
		return (boolean)((is_u(etmp) && (Levitation || Flying)) ||
			    likes_lava(etmp->edata) || is_flyer(etmp->edata));
	if (is_db_wall(x, y))
		return((boolean)(is_u(etmp) ? Passes_walls :
			passes_walls(etmp->edata)));
	return(TRUE);
}

STATIC_OVL void
e_died(etmp, dest, how)
struct entity *etmp;
int dest, how;
{
	if (is_u(etmp)) {
		if (how == DROWNING) {
			killer = 0;	/* drown() sets its own killer */
			(void) drown();
		} else if (how == BURNING) {
			killer = 0;	/* lava_effects() sets its own killer */
			(void) lava_effects();
		} else {
			coord xy;

			/* use more specific killer if specified */
			if (!killer) {
			    killer_format = KILLED_BY_AN;
			    killer = "falling drawbridge";
			}
			done(how);
			/* So, you didn't die */
			if (!e_survives_at(etmp, etmp->ex, etmp->ey)) {
			    if (enexto(&xy, etmp->ex, etmp->ey, etmp->edata)) {
				pline("A %s force teleports you away...",
				      Hallucination ? "normal" : "strange");
				teleds(xy.x, xy.y);
			    }
			    /* otherwise on top of the drawbridge is the
			     * only viable spot in the dungeon, so stay there
			     */
			}
		}
		/* we might have crawled out of the moat to survive */
		etmp->ex = u.ux,  etmp->ey = u.uy;
	} else {
		killer = 0;
		/* fake "digested to death" damage-type suppresses corpse */
#define mk_message(dest) ((dest & 1) ? "" : (char *)0)
#define mk_corpse(dest)  ((dest & 2) ? AD_DGST : AD_PHYS)
		/* if monsters are moving, one of them caused the destruction */
		if (flags.mon_moving)
		    monkilled(etmp->emon, mk_message(dest), mk_corpse(dest));
		else		/* you caused it */
		    xkilled(etmp->emon, dest);
		etmp->edata = (struct permonst *)0;
#undef mk_message
#undef mk_corpse
	}
}


/*
 * These are never directly affected by a bridge or portcullis.
 */

STATIC_OVL boolean
automiss(etmp)
struct entity *etmp;
{
	return (boolean)((is_u(etmp) ? Passes_walls :
			passes_walls(etmp->edata)) || noncorporeal(etmp->edata));
}

/*
 * Does falling drawbridge or portcullis miss etmp?
 */

STATIC_OVL boolean
e_missed(etmp, chunks)
struct entity *etmp;
boolean chunks;
{
	int misses;

#ifdef D_DEBUG
	if (chunks)
		pline("Do chunks miss?");
#endif
	if (automiss(etmp))
		return(TRUE);

	if (is_flyer(etmp->edata) &&
	    (is_u(etmp)? !Sleeping :
	     (etmp->emon->mcanmove && !etmp->emon->msleeping)))
						 /* flying requires mobility */
		misses = 5;	/* out of 8 */
	else if (is_floater(etmp->edata) ||
		    (is_u(etmp) && Levitation))	 /* doesn't require mobility */
		misses = 3;
	else if (chunks && is_pool(etmp->ex, etmp->ey))
		misses = 2;				    /* sitting ducks */
	else
		misses = 0;

	if (is_db_wall(etmp->ex, etmp->ey))
		misses -= 3;				    /* less airspace */

#ifdef D_DEBUG
	pline("Miss chance = %d (out of 8)", misses);
#endif

	return((boolean)((misses >= rnd(8))? TRUE : FALSE));
}

/*
 * Can etmp jump from death?
 */

STATIC_OVL boolean
e_jumps(etmp)
struct entity *etmp;
{
	int tmp = 4;		/* out of 10 */

	if (is_u(etmp)? (Sleeping || Fumbling) :
		        (!etmp->emon->mcanmove || etmp->emon->msleeping ||
			 !etmp->edata->mmove   || etmp->emon->wormno))
		return(FALSE);

	if (is_u(etmp)? Confusion : etmp->emon->mconf)
		tmp -= 2;

	if (is_u(etmp)? Stunned : etmp->emon->mstun)
		tmp -= 3;

	if (is_db_wall(etmp->ex, etmp->ey))
		tmp -= 2;			    /* less room to maneuver */

#ifdef D_DEBUG
	pline("%s to jump (%d chances in 10)", E_phrase(etmp, "try"), tmp);
#endif
	return((boolean)((tmp >= rnd(10))? TRUE : FALSE));
}

STATIC_OVL void
do_entity(etmp)
struct entity *etmp;
{
	int newx, newy, at_portcullis, oldx, oldy;
	boolean must_jump = FALSE, relocates = FALSE, e_inview;
	struct rm *crm;

	if (!etmp->edata)
		return;

	e_inview = e_canseemon(etmp);
	oldx = etmp->ex;
	oldy = etmp->ey;
	at_portcullis = is_db_wall(oldx, oldy);
	crm = &levl[oldx][oldy];

	if (automiss(etmp) && e_survives_at(etmp, oldx, oldy)) {
		if (e_inview && (at_portcullis || IS_DRAWBRIDGE(crm->typ)))
			pline_The("%s passes through %s!",
			      at_portcullis ? "portcullis" : "drawbridge",
			      e_nam(etmp));
		return;
	}
	if (e_missed(etmp, FALSE)) {
		if (at_portcullis)
			pline_The("portcullis misses %s!",
			      e_nam(etmp));
#ifdef D_DEBUG
		else
			pline_The("drawbridge misses %s!",
			      e_nam(etmp));
#endif
		if (e_survives_at(etmp, oldx, oldy))
			return;
		else {
#ifdef D_DEBUG
			pline("Mon can't survive here");
#endif
			if (at_portcullis)
				must_jump = TRUE;
			else
				relocates = TRUE; /* just ride drawbridge in */
		}
	} else {
		if (crm->typ == DRAWBRIDGE_DOWN) {
			pline("%s crushed underneath the drawbridge.",
			      E_phrase(etmp, "are"));		  /* no jump */
			e_died(etmp, e_inview? 3 : 2, CRUSHING);/* no corpse */
			return;   /* Note: Beyond this point, we know we're  */
		}		  /* not at an opened drawbridge, since all  */
		must_jump = TRUE; /* *missable* creatures survive on the     */
	}			  /* square, and all the unmissed ones die.  */
	if (must_jump) {
	    if (at_portcullis) {
		if (e_jumps(etmp)) {
		    relocates = TRUE;
#ifdef D_DEBUG
		    pline("Jump succeeds!");
#endif
		} else {
		    if (e_inview)
			pline("%s crushed by the falling portcullis!",
			      E_phrase(etmp, "are"));
		    else if (flags.soundok)
			You_hear("a crushing sound.");
		    e_died(etmp, e_inview? 3 : 2, CRUSHING);
		    /* no corpse */
		    return;
		}
	    } else { /* tries to jump off bridge to original square */
		relocates = !e_jumps(etmp);
#ifdef D_DEBUG
		pline("Jump %s!", (relocates)? "fails" : "succeeds");
#endif
	    }
	}

/*
 * Here's where we try to do relocation.  Assumes that etmp is not arriving
 * at the portcullis square while the drawbridge is falling, since this square
 * would be inaccessible (i.e. etmp started on drawbridge square) or
 * unnecessary (i.e. etmp started here) in such a situation.
 */
#ifdef D_DEBUG
	pline("Doing relocation.");
#endif
	newx = oldx;
	newy = oldy;
	(void)find_drawbridge(&newx, &newy);
	if ((newx == oldx) && (newy == oldy))
		get_wall_for_db(&newx, &newy);
#ifdef D_DEBUG
	pline("Checking new square for occupancy.");
#endif
	if (relocates && (e_at(newx, newy))) {

/*
 * Standoff problem:  one or both entities must die, and/or both switch
 * places.  Avoid infinite recursion by checking first whether the other
 * entity is staying put.  Clean up if we happen to move/die in recursion.
 */
		struct entity *other;

		other = e_at(newx, newy);
#ifdef D_DEBUG
		pline("New square is occupied by %s", e_nam(other));
#endif
		if (e_survives_at(other, newx, newy) && automiss(other)) {
			relocates = FALSE;	      /* "other" won't budge */
#ifdef D_DEBUG
			pline("%s suicide.", E_phrase(etmp, "commit"));
#endif
		} else {

#ifdef D_DEBUG
			pline("Handling %s", e_nam(other));
#endif
			while ((e_at(newx, newy) != 0) &&
			       (e_at(newx, newy) != etmp))
				do_entity(other);
#ifdef D_DEBUG
			pline("Checking existence of %s", e_nam(etmp));
			wait_synch();
#endif
			if (e_at(oldx, oldy) != etmp) {
#ifdef D_DEBUG
			    pline("%s moved or died in recursion somewhere",
				  E_phrase(etmp, "have"));
			    wait_synch();
#endif
			    return;
			}
		}
	}
	if (relocates && !e_at(newx, newy)) {/* if e_at() entity = worm tail */
#ifdef D_DEBUG
		pline("Moving %s", e_nam(etmp));
#endif
		if (!is_u(etmp)) {
			remove_monster(etmp->ex, etmp->ey);
			place_monster(etmp->emon, newx, newy);
			update_monster_region(etmp->emon);
		} else {
			u.ux = newx;
			u.uy = newy;
		}
		etmp->ex = newx;
		etmp->ey = newy;
		e_inview = e_canseemon(etmp);
	}
#ifdef D_DEBUG
	pline("Final disposition of %s", e_nam(etmp));
	wait_synch();
#endif
	if (is_db_wall(etmp->ex, etmp->ey)) {
#ifdef D_DEBUG
		pline("%s in portcullis chamber", E_phrase(etmp, "are"));
		wait_synch();
#endif
		if (e_inview) {
			if (is_u(etmp)) {
				You("tumble towards the closed portcullis!");
				if (automiss(etmp))
					You("pass through it!");
				else
					pline_The("drawbridge closes in...");
			} else
				pline("%s behind the drawbridge.",
				      E_phrase(etmp, "disappear"));
		}
		if (!e_survives_at(etmp, etmp->ex, etmp->ey)) {
			killer_format = KILLED_BY_AN;
			killer = "closing drawbridge";
			e_died(etmp, 0, CRUSHING);	       /* no message */
			return;
		}
#ifdef D_DEBUG
		pline("%s in here", E_phrase(etmp, "survive"));
#endif
	} else {
#ifdef D_DEBUG
		pline("%s on drawbridge square", E_phrase(etmp, "are"));
#endif
		if (is_pool(etmp->ex, etmp->ey) && !e_inview)
			if (flags.soundok)
				You_hear("a splash.");
		if (e_survives_at(etmp, etmp->ex, etmp->ey)) {
			if (e_inview && !is_flyer(etmp->edata) &&
			    !is_floater(etmp->edata))
				pline("%s from the bridge.",
				      E_phrase(etmp, "fall"));
			return;
		}
#ifdef D_DEBUG
		pline("%s cannot survive on the drawbridge square",Enam(etmp));
#endif
		if (is_pool(etmp->ex, etmp->ey) || is_lava(etmp->ex, etmp->ey))
		    if (e_inview && !is_u(etmp)) {
			/* drown() will supply msgs if nec. */
			boolean lava = is_lava(etmp->ex, etmp->ey);

			if (Hallucination)
			    pline("%s the %s and disappears.",
				  E_phrase(etmp, "drink"),
				  lava ? "lava" : "moat");
			else
			    pline("%s into the %s.",
				  E_phrase(etmp, "fall"),
				  lava ? "lava" : "moat");
		    }
		killer_format = NO_KILLER_PREFIX;
		killer = "fell from a drawbridge";
		e_died(etmp, e_inview ? 3 : 2,      /* CRUSHING is arbitrary */
		       (is_pool(etmp->ex, etmp->ey)) ? DROWNING :
		       (is_lava(etmp->ex, etmp->ey)) ? BURNING :
						       CRUSHING); /*no corpse*/
		return;
	}
}

/*
 * Close the drawbridge located at x,y
 */

void
close_drawbridge(x,y)
int x,y;
{
	register struct rm *lev1, *lev2;
	struct trap *t;
	int x2, y2;

	lev1 = &levl[x][y];
	if (lev1->typ != DRAWBRIDGE_DOWN) return;
	x2 = x; y2 = y;
	get_wall_for_db(&x2,&y2);
	if (cansee(x,y) || cansee(x2,y2))
		You("see a drawbridge %s up!",
		    (((u.ux == x || u.uy == y) && !Underwater) ||
		     distu(x2,y2) < distu(x,y)) ? "coming" : "going");
	lev1->typ = DRAWBRIDGE_UP;
	lev2 = &levl[x2][y2];
	lev2->typ = DBWALL;
	switch (lev1->drawbridgemask & DB_DIR) {
		case DB_NORTH:
		case DB_SOUTH:
			lev2->horizontal = TRUE;
			break;
		case DB_WEST:
		case DB_EAST:
			lev2->horizontal = FALSE;
			break;
	}
	lev2->wall_info = W_NONDIGGABLE;
	set_entity(x, y, &(occupants[0]));
	set_entity(x2, y2, &(occupants[1]));
	do_entity(&(occupants[0]));		/* Do set_entity after first */
	set_entity(x2, y2, &(occupants[1]));	/* do_entity for worm tail */
	do_entity(&(occupants[1]));
	if(OBJ_AT(x,y) && flags.soundok)
	    You_hear("smashing and crushing.");
	(void) revive_nasty(x,y,(char *)0);
	(void) revive_nasty(x2,y2,(char *)0);
	delallobj(x, y);
	delallobj(x2, y2);
	if ((t = t_at(x, y)) != 0) deltrap(t);
	if ((t = t_at(x2, y2)) != 0) deltrap(t);
	newsym(x, y);
	newsym(x2, y2);
	block_point(x2,y2);	/* vision */
}

/*
 * Open the drawbridge located at x,y
 */

void
open_drawbridge(x,y)
int x,y;
{
	register struct rm *lev1, *lev2;
	struct trap *t;
	int x2, y2;

	lev1 = &levl[x][y];
	if (lev1->typ != DRAWBRIDGE_UP) return;
	x2 = x; y2 = y;
	get_wall_for_db(&x2,&y2);
	if (cansee(x,y) || cansee(x2,y2))
		You("see a drawbridge %s down!",
		    (distu(x2,y2) < distu(x,y)) ? "going" : "coming");
	lev1->typ = DRAWBRIDGE_DOWN;
	lev2 = &levl[x2][y2];
	lev2->typ = DOOR;
	lev2->doormask = D_NODOOR;
	set_entity(x, y, &(occupants[0]));
	set_entity(x2, y2, &(occupants[1]));
	do_entity(&(occupants[0]));		/* do set_entity after first */
	set_entity(x2, y2, &(occupants[1]));	/* do_entity for worm tails */
	do_entity(&(occupants[1]));
	(void) revive_nasty(x,y,(char *)0);
	delallobj(x, y);
	if ((t = t_at(x, y)) != 0) deltrap(t);
	if ((t = t_at(x2, y2)) != 0) deltrap(t);
	newsym(x, y);
	newsym(x2, y2);
	unblock_point(x2,y2);	/* vision */
	if (Is_stronghold(&u.uz)) u.uevent.uopened_dbridge = TRUE;
}

/*
 * Let's destroy the drawbridge located at x,y
 */

void
destroy_drawbridge(x,y)
int x,y;
{
	register struct rm *lev1, *lev2;
	struct trap *t;
	int x2, y2;
	boolean e_inview;
	struct entity *etmp1 = &(occupants[0]), *etmp2 = &(occupants[1]);

	lev1 = &levl[x][y];
	if (!IS_DRAWBRIDGE(lev1->typ))
		return;
	x2 = x; y2 = y;
	get_wall_for_db(&x2,&y2);
	lev2 = &levl[x2][y2];
	if ((lev1->drawbridgemask & DB_UNDER) == DB_MOAT ||
	    (lev1->drawbridgemask & DB_UNDER) == DB_LAVA) {
		struct obj *otmp;
		boolean lava = (lev1->drawbridgemask & DB_UNDER) == DB_LAVA;
		if (lev1->typ == DRAWBRIDGE_UP) {
			if (cansee(x2,y2))
			    pline_The("portcullis of the drawbridge falls into the %s!",
				  lava ? "lava" : "moat");
			else if (flags.soundok)
				You_hear("a loud *SPLASH*!");
		} else {
			if (cansee(x,y))
			    pline_The("drawbridge collapses into the %s!",
				  lava ? "lava" : "moat");
			else if (flags.soundok)
				You_hear("a loud *SPLASH*!");
		}
		lev1->typ = lava ? LAVAPOOL : MOAT;
		lev1->drawbridgemask = 0;
		if ((otmp = sobj_at(BOULDER,x,y)) != 0) {
		    obj_extract_self(otmp);
		    (void) flooreffects(otmp,x,y,"fall");
		}
	} else {
		if (cansee(x,y))
			pline_The("drawbridge disintegrates!");
		else
			You_hear("a loud *CRASH*!");
		lev1->typ =
			((lev1->drawbridgemask & DB_ICE) ? ICE : ROOM);
		lev1->icedpool =
			((lev1->drawbridgemask & DB_ICE) ? ICED_MOAT : 0);
	}
	wake_nearto(x, y, 500);
	lev2->typ = DOOR;
	lev2->doormask = D_NODOOR;
	if ((t = t_at(x, y)) != 0) deltrap(t);
	if ((t = t_at(x2, y2)) != 0) deltrap(t);
	newsym(x,y);
	newsym(x2,y2);
	if (!does_block(x2,y2,lev2)) unblock_point(x2,y2);	/* vision */
	if (Is_stronghold(&u.uz)) u.uevent.uopened_dbridge = TRUE;

	set_entity(x2, y2, etmp2); /* currently only automissers can be here */
	if (etmp2->edata) {
		e_inview = e_canseemon(etmp2);
		if (!automiss(etmp2)) {
			if (e_inview)
				pline("%s blown apart by flying debris.",
				      E_phrase(etmp2, "are"));
			killer_format = KILLED_BY_AN;
			killer = "exploding drawbridge";
			e_died(etmp2, e_inview? 3 : 2, CRUSHING); /*no corpse*/
		}	     /* nothing which is vulnerable can survive this */
	}
	set_entity(x, y, etmp1);
	if (etmp1->edata) {
		e_inview = e_canseemon(etmp1);
		if (e_missed(etmp1, TRUE)) {
#ifdef D_DEBUG
			pline("%s spared!", E_phrase(etmp1, "are"));
#endif
		} else {
			if (e_inview) {
			    if (!is_u(etmp1) && Hallucination)
				pline("%s into some heavy metal!",
				      E_phrase(etmp1, "get"));
			    else
				pline("%s hit by a huge chunk of metal!",
				      E_phrase(etmp1, "are"));
			} else {
			    if (flags.soundok && !is_u(etmp1) && !is_pool(x,y))
				You_hear("a crushing sound.");
#ifdef D_DEBUG
			    else
				pline("%s from shrapnel",
				      E_phrase(etmp1, "die"));
#endif
			}
			killer_format = KILLED_BY_AN;
			killer = "collapsing drawbridge";
			e_died(etmp1, e_inview? 3 : 2, CRUSHING); /*no corpse*/
			if(lev1->typ == MOAT) do_entity(etmp1);
		}
	}
}

#endif /* OVLB */

/*dbridge.c*/
