/*	SCCS Id: @(#)bones.c	3.4	2003/09/06	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985,1993. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "lev.h"

extern char bones[];	/* from files.c */
#ifdef MFLOPPY
extern long bytes_counted;
#endif

STATIC_DCL boolean FDECL(no_bones_level, (d_level *));
STATIC_DCL void FDECL(goodfruit, (int));
STATIC_DCL void FDECL(resetobjs,(struct obj *,BOOLEAN_P));
STATIC_DCL void FDECL(drop_upon_death, (struct monst *, struct obj *));

STATIC_OVL boolean
no_bones_level(lev)
d_level *lev;
{
	extern d_level save_dlevel;		/* in do.c */
	s_level *sptr;

	if (ledger_no(&save_dlevel)) assign_level(lev, &save_dlevel);

	return (boolean)(((sptr = Is_special(lev)) != 0 && !sptr->boneid)
		|| !dungeons[lev->dnum].boneid
		   /* no bones on the last or multiway branch levels */
		   /* in any dungeon (level 1 isn't multiway).       */
		|| Is_botlevel(lev) || (Is_branchlev(lev) && lev->dlevel > 1)
		   /* no bones in the invocation level               */
		|| (In_hell(lev) && lev->dlevel == dunlevs_in_dungeon(lev) - 1)
		);
}

/* Call this function for each fruit object saved in the bones level: it marks
 * that particular type of fruit as existing (the marker is that that type's
 * ID is positive instead of negative).  This way, when we later save the
 * chain of fruit types, we know to only save the types that exist.
 */
STATIC_OVL void
goodfruit(id)
int id;
{
	register struct fruit *f;

	for(f=ffruit; f; f=f->nextf) {
		if(f->fid == -id) {
			f->fid = id;
			return;
		}
	}
}

STATIC_OVL void
resetobjs(ochain,restore)
struct obj *ochain;
boolean restore;
{
	struct obj *otmp;

	for (otmp = ochain; otmp; otmp = otmp->nobj) {
		if (otmp->cobj)
		    resetobjs(otmp->cobj,restore);

		if (((otmp->otyp != CORPSE || otmp->corpsenm < SPECIAL_PM)
			&& otmp->otyp != STATUE)
			&& (!otmp->oartifact ||
			   (restore && (exist_artifact(otmp->otyp, ONAME(otmp))
					|| is_quest_artifact(otmp))))) {
			otmp->oartifact = 0;
			otmp->onamelth = 0;
			*ONAME(otmp) = '\0';
		} else if (otmp->oartifact && restore)
			artifact_exists(otmp,ONAME(otmp),TRUE);
		if (!restore) {
			/* do not zero out o_ids for ghost levels anymore */

			if(objects[otmp->otyp].oc_uses_known) otmp->known = 0;
			otmp->dknown = otmp->bknown = 0;
			otmp->rknown = 0;
			otmp->invlet = 0;
			otmp->no_charge = 0;

			if (otmp->otyp == SLIME_MOLD) goodfruit(otmp->spe);
#ifdef MAIL
			else if (otmp->otyp == SCR_MAIL) otmp->spe = 1;
#endif
			else if (otmp->otyp == EGG) otmp->spe = 0;
			else if (otmp->otyp == TIN) {
			    /* make tins of unique monster's meat be empty */
			    if (otmp->corpsenm >= LOW_PM &&
				    (mons[otmp->corpsenm].geno & G_UNIQ))
				otmp->corpsenm = NON_PM;
			} else if (otmp->otyp == AMULET_OF_YENDOR) {
			    /* no longer the real Amulet */
			    otmp->otyp = FAKE_AMULET_OF_YENDOR;
			    curse(otmp);
			} else if (otmp->otyp == CANDELABRUM_OF_INVOCATION) {
			    if (otmp->lamplit)
				end_burn(otmp, TRUE);
			    otmp->otyp = WAX_CANDLE;
			    otmp->age = 50L;  /* assume used */
			    if (otmp->spe > 0)
				otmp->quan = (long)otmp->spe;
			    otmp->spe = 0;
			    otmp->owt = weight(otmp);
			    curse(otmp);
			} else if (otmp->otyp == BELL_OF_OPENING) {
			    otmp->otyp = BELL;
			    curse(otmp);
			} else if (otmp->otyp == SPE_BOOK_OF_THE_DEAD) {
			    otmp->otyp = SPE_BLANK_PAPER;
			    curse(otmp);
			}
		}
	}
}

STATIC_OVL void
drop_upon_death(mtmp, cont)
struct monst *mtmp;
struct obj *cont;
{
	struct obj *otmp;

	uswapwep = 0; /* ensure curse() won't cause swapwep to drop twice */
	while ((otmp = invent) != 0) {
		obj_extract_self(otmp);
		obj_no_longer_held(otmp);

		otmp->owornmask = 0;
		/* lamps don't go out when dropped */
		if ((cont || artifact_light(otmp)) && obj_is_burning(otmp))
		    end_burn(otmp, TRUE);	/* smother in statue */

		if(otmp->otyp == SLIME_MOLD) goodfruit(otmp->spe);

		if(rn2(5)) curse(otmp);
		if (mtmp)
			(void) add_to_minv(mtmp, otmp);
		else if (cont)
			(void) add_to_container(cont, otmp);
		else
			place_object(otmp, u.ux, u.uy);
	}
#ifndef GOLDOBJ
	if(u.ugold) {
		long ugold = u.ugold;
		if (mtmp) mtmp->mgold = ugold;
		else if (cont) (void) add_to_container(cont, mkgoldobj(ugold));
		else (void)mkgold(ugold, u.ux, u.uy);
		u.ugold = ugold;	/* undo mkgoldobj()'s removal */
	}
#endif
	if (cont) cont->owt = weight(cont);
}

/* check whether bones are feasible */
boolean
can_make_bones()
{
	register struct trap *ttmp;

	if (ledger_no(&u.uz) <= 0 || ledger_no(&u.uz) > maxledgerno())
	    return FALSE;
	if (no_bones_level(&u.uz))
	    return FALSE;		/* no bones for specific levels */
	if (u.uswallow) {
	    return FALSE;		/* no bones when swallowed */
	}
	if (!Is_branchlev(&u.uz)) {
	    /* no bones on non-branches with portals */
	    for(ttmp = ftrap; ttmp; ttmp = ttmp->ntrap)
		if (ttmp->ttyp == MAGIC_PORTAL) return FALSE;
	}

	if(depth(&u.uz) <= 0 ||		/* bulletproofing for endgame */
	   (!rn2(1 + (depth(&u.uz)>>2))	/* fewer ghosts on low levels */
#ifdef WIZARD
		&& !wizard
#endif
		)) return FALSE;
	/* don't let multiple restarts generate multiple copies of objects
	 * in bones files */
	if (discover) return FALSE;
	return TRUE;
}

/* save bones and possessions of a deceased adventurer */
void
savebones(corpse)
struct obj *corpse;
{
	int fd, x, y;
	struct trap *ttmp;
	struct monst *mtmp;
	struct permonst *mptr;
	struct fruit *f;
	char c, *bonesid;
	char whynot[BUFSZ];

	/* caller has already checked `can_make_bones()' */

	clear_bypasses();
	fd = open_bonesfile(&u.uz, &bonesid);
	if (fd >= 0) {
		(void) close(fd);
		compress_bonesfile();
#ifdef WIZARD
		if (wizard) {
		    if (yn("Bones file already exists.  Replace it?") == 'y') {
			if (delete_bonesfile(&u.uz)) goto make_bones;
			else pline("Cannot unlink old bones.");
		    }
		}
#endif
		return;
	}

#ifdef WIZARD
 make_bones:
#endif
	unleash_all();
	/* in case these characters are not in their home bases */
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	    if (DEADMONSTER(mtmp)) continue;
	    mptr = mtmp->data;
	    if (mtmp->iswiz || mptr == &mons[PM_MEDUSA] ||
		    mptr->msound == MS_NEMESIS || mptr->msound == MS_LEADER ||
		    mptr == &mons[PM_VLAD_THE_IMPALER])
		mongone(mtmp);
	}
#ifdef STEED
	if (u.usteed) dismount_steed(DISMOUNT_BONES);
#endif
	dmonsfree();		/* discard dead or gone monsters */

	/* mark all fruits as nonexistent; when we come to them we'll mark
	 * them as existing (using goodfruit())
	 */
	for(f=ffruit; f; f=f->nextf) f->fid = -f->fid;

	/* check iron balls separately--maybe they're not carrying it */
	if (uball) uball->owornmask = uchain->owornmask = 0;

	/* dispose of your possessions, usually cursed */
	if (u.ugrave_arise == (NON_PM - 1)) {
		struct obj *otmp;

		/* embed your possessions in your statue */
		otmp = mk_named_object(STATUE, &mons[u.umonnum],
				       u.ux, u.uy, plname);

		drop_upon_death((struct monst *)0, otmp);
		if (!otmp) return;	/* couldn't make statue */
		mtmp = (struct monst *)0;
	} else if (u.ugrave_arise < LOW_PM) {
		/* drop everything */
		drop_upon_death((struct monst *)0, (struct obj *)0);
		/* trick makemon() into allowing monster creation
		 * on your location
		 */
		in_mklev = TRUE;
		mtmp = makemon(&mons[PM_GHOST], u.ux, u.uy, MM_NONAME);
		in_mklev = FALSE;
		if (!mtmp) return;
		mtmp = christen_monst(mtmp, plname);
		if (corpse)
			(void) obj_attach_mid(corpse, mtmp->m_id); 
	} else {
		/* give your possessions to the monster you become */
		in_mklev = TRUE;
		mtmp = makemon(&mons[u.ugrave_arise], u.ux, u.uy, NO_MM_FLAGS);
		in_mklev = FALSE;
		if (!mtmp) {
			drop_upon_death((struct monst *)0, (struct obj *)0);
			return;
		}
		mtmp = christen_monst(mtmp, plname);
		newsym(u.ux, u.uy);
		Your("body rises from the dead as %s...",
			an(mons[u.ugrave_arise].mname));
		display_nhwindow(WIN_MESSAGE, FALSE);
		drop_upon_death(mtmp, (struct obj *)0);
		m_dowear(mtmp, TRUE);
	}
	if (mtmp) {
		mtmp->m_lev = (u.ulevel ? u.ulevel : 1);
		mtmp->mhp = mtmp->mhpmax = u.uhpmax;
		mtmp->female = flags.female;
		mtmp->msleeping = 1;
	}
	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		resetobjs(mtmp->minvent,FALSE);
		/* do not zero out m_ids for bones levels any more */
		mtmp->mlstmv = 0L;
		if(mtmp->mtame) mtmp->mtame = mtmp->mpeaceful = 0;
	}
	for(ttmp = ftrap; ttmp; ttmp = ttmp->ntrap) {
		ttmp->madeby_u = 0;
		ttmp->tseen = (ttmp->ttyp == HOLE);
	}
	resetobjs(fobj,FALSE);
	resetobjs(level.buriedobjlist, FALSE);

	/* Hero is no longer on the map. */
	u.ux = u.uy = 0;

	/* Clear all memory from the level. */
	for(x=0; x<COLNO; x++) for(y=0; y<ROWNO; y++) {
	    levl[x][y].seenv = 0;
	    levl[x][y].waslit = 0;
	    levl[x][y].glyph = cmap_to_glyph(S_stone);
	}

	fd = create_bonesfile(&u.uz, &bonesid, whynot);
	if(fd < 0) {
#ifdef WIZARD
		if(wizard)
			pline("%s", whynot);
#endif
		/* bones file creation problems are silent to the player.
		 * Keep it that way, but place a clue into the paniclog.
		 */
		paniclog("savebones", whynot);
		return;
	}
	c = (char) (strlen(bonesid) + 1);

#ifdef MFLOPPY  /* check whether there is room */
	if (iflags.checkspace) {
	    savelev(fd, ledger_no(&u.uz), COUNT_SAVE);
	    /* savelev() initializes bytes_counted to 0, so it must come
	     * first here even though it does not in the real save.  the
	     * resulting extra bflush() at the end of savelev() may increase
	     * bytes_counted by a couple over what the real usage will be.
	     *
	     * note it is safe to call store_version() here only because
	     * bufon() is null for ZEROCOMP, which MFLOPPY uses -- otherwise
	     * this code would have to know the size of the version
	     * information itself.
	     */
	    store_version(fd);
	    bwrite(fd, (genericptr_t) &c, sizeof c);
	    bwrite(fd, (genericptr_t) bonesid, (unsigned) c);	/* DD.nnn */
	    savefruitchn(fd, COUNT_SAVE);
	    bflush(fd);
	    if (bytes_counted > freediskspace(bones)) { /* not enough room */
# ifdef WIZARD
		if (wizard)
			pline("Insufficient space to create bones file.");
# endif
		(void) close(fd);
		cancel_bonesfile();
		return;
	    }
	    co_false();	/* make sure stuff before savelev() gets written */
	}
#endif /* MFLOPPY */

	store_version(fd);
	bwrite(fd, (genericptr_t) &c, sizeof c);
	bwrite(fd, (genericptr_t) bonesid, (unsigned) c);	/* DD.nnn */
	savefruitchn(fd, WRITE_SAVE | FREE_SAVE);
	update_mlstmv();	/* update monsters for eventual restoration */
	savelev(fd, ledger_no(&u.uz), WRITE_SAVE | FREE_SAVE);
	bclose(fd);
	commit_bonesfile(&u.uz);
	compress_bonesfile();
}

int
getbones()
{
	register int fd;
	register int ok;
	char c, *bonesid, oldbonesid[10];

	if(discover)		/* save bones files for real games */
		return(0);

	/* wizard check added by GAN 02/05/87 */
	if(rn2(3)	/* only once in three times do we find bones */
#ifdef WIZARD
		&& !wizard
#endif
		) return(0);
	if(no_bones_level(&u.uz)) return(0);
	fd = open_bonesfile(&u.uz, &bonesid);
	if (fd < 0) return(0);

	if ((ok = uptodate(fd, bones)) == 0) {
#ifdef WIZARD
	    if (!wizard)
#endif
		pline("Discarding unuseable bones; no need to panic...");
	} else {
#ifdef WIZARD
		if(wizard)  {
			if(yn("Get bones?") == 'n') {
				(void) close(fd);
				compress_bonesfile();
				return(0);
			}
		}
#endif
		mread(fd, (genericptr_t) &c, sizeof c);	/* length incl. '\0' */
		mread(fd, (genericptr_t) oldbonesid, (unsigned) c); /* DD.nnn */
		if (strcmp(bonesid, oldbonesid) != 0) {
			char errbuf[BUFSZ];

			Sprintf(errbuf, "This is bones level '%s', not '%s'!",
				oldbonesid, bonesid);
#ifdef WIZARD
			if (wizard) {
				pline("%s", errbuf);
				ok = FALSE;	/* won't die of trickery */
			}
#endif
			trickery(errbuf);
		} else {
			register struct monst *mtmp;

			getlev(fd, 0, 0, TRUE);

			/* Note that getlev() now keeps tabs on unique
			 * monsters such as demon lords, and tracks the
			 * birth counts of all species just as makemon()
			 * does.  If a bones monster is extinct or has been
			 * subject to genocide, their mhpmax will be
			 * set to the magic DEFUNCT_MONSTER cookie value.
			 */
			for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
			    if (mtmp->mhpmax == DEFUNCT_MONSTER) {
#if defined(DEBUG) && defined(WIZARD)
				if (wizard)
				    pline("Removing defunct monster %s from bones.",
					mtmp->data->mname);
#endif
				mongone(mtmp);
			    } else
				/* to correctly reset named artifacts on the level */
				resetobjs(mtmp->minvent,TRUE);
			}
			resetobjs(fobj,TRUE);
			resetobjs(level.buriedobjlist,TRUE);
		}
	}
	(void) close(fd);

#ifdef WIZARD
	if(wizard) {
		if(yn("Unlink bones?") == 'n') {
			compress_bonesfile();
			return(ok);
		}
	}
#endif
	if (!delete_bonesfile(&u.uz)) {
		/* When N games try to simultaneously restore the same
		 * bones file, N-1 of them will fail to delete it
		 * (the first N-1 under AmigaDOS, the last N-1 under UNIX).
		 * So no point in a mysterious message for a normal event
		 * -- just generate a new level for those N-1 games.
		 */
		/* pline("Cannot unlink bones."); */
		return(0);
	}
	return(ok);
}

/*bones.c*/
