/* NetHack 3.6	restore.c	$NHDT-Date: 1575245087 2019/12/02 00:04:47 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.136 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2009. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "lev.h"
#include "tcap.h" /* for TERMLIB and ASCIIGRAPH */

#if defined(MICRO)
extern int dotcnt; /* shared with save */
extern int dotrow; /* shared with save */
#endif

#ifdef USE_TILES
extern void FDECL(substitute_tiles, (d_level *)); /* from tile.c */
#endif

#ifdef ZEROCOMP
STATIC_DCL void NDECL(zerocomp_minit);
STATIC_DCL void FDECL(zerocomp_mread, (int, genericptr_t, unsigned int));
STATIC_DCL int NDECL(zerocomp_mgetc);
#endif

STATIC_DCL void NDECL(def_minit);
STATIC_DCL void FDECL(def_mread, (int, genericptr_t, unsigned int));

STATIC_DCL void NDECL(find_lev_obj);
STATIC_DCL void FDECL(restlevchn, (int));
STATIC_DCL void FDECL(restdamage, (int, BOOLEAN_P));
STATIC_DCL void FDECL(restobj, (int, struct obj *));
STATIC_DCL struct obj *FDECL(restobjchn, (int, BOOLEAN_P, BOOLEAN_P));
STATIC_OVL void FDECL(restmon, (int, struct monst *));
STATIC_DCL struct monst *FDECL(restmonchn, (int, BOOLEAN_P));
STATIC_DCL struct fruit *FDECL(loadfruitchn, (int));
STATIC_DCL void FDECL(freefruitchn, (struct fruit *));
STATIC_DCL void FDECL(ghostfruit, (struct obj *));
STATIC_DCL boolean FDECL(restgamestate, (int, unsigned int *, unsigned int *));
STATIC_DCL void FDECL(restlevelstate, (unsigned int, unsigned int));
STATIC_DCL int FDECL(restlevelfile, (int, XCHAR_P));
STATIC_OVL void FDECL(restore_msghistory, (int));
STATIC_DCL void FDECL(reset_oattached_mids, (BOOLEAN_P));
STATIC_DCL void FDECL(rest_levl, (int, BOOLEAN_P));

static struct restore_procs {
    const char *name;
    int mread_flags;
    void NDECL((*restore_minit));
    void FDECL((*restore_mread), (int, genericptr_t, unsigned int));
    void FDECL((*restore_bclose), (int));
} restoreprocs = {
#if !defined(ZEROCOMP) || (defined(COMPRESS) || defined(ZLIB_COMP))
    "externalcomp", 0, def_minit, def_mread, def_bclose,
#else
    "zerocomp", 0, zerocomp_minit, zerocomp_mread, zerocomp_bclose,
#endif
};

/*
 * Save a mapping of IDs from ghost levels to the current level.  This
 * map is used by the timer routines when restoring ghost levels.
 */
#define N_PER_BUCKET 64
struct bucket {
    struct bucket *next;
    struct {
        unsigned gid; /* ghost ID */
        unsigned nid; /* new ID */
    } map[N_PER_BUCKET];
};

STATIC_DCL void NDECL(clear_id_mapping);
STATIC_DCL void FDECL(add_id_mapping, (unsigned, unsigned));

static int n_ids_mapped = 0;
static struct bucket *id_map = 0;

#ifdef AMII_GRAPHICS
void FDECL(amii_setpens, (int)); /* use colors from save file */
extern int amii_numcolors;
#endif

#include "display.h"

boolean restoring = FALSE;
static NEARDATA struct fruit *oldfruit;
static NEARDATA long omoves;

#define Is_IceBox(o) ((o)->otyp == ICE_BOX ? TRUE : FALSE)

/* Recalculate level.objects[x][y], since this info was not saved. */
STATIC_OVL void
find_lev_obj()
{
    register struct obj *fobjtmp = (struct obj *) 0;
    register struct obj *otmp;
    int x, y;

    for (x = 0; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++)
            level.objects[x][y] = (struct obj *) 0;

    /*
     * Reverse the entire fobj chain, which is necessary so that we can
     * place the objects in the proper order.  Make all obj in chain
     * OBJ_FREE so place_object will work correctly.
     */
    while ((otmp = fobj) != 0) {
        fobj = otmp->nobj;
        otmp->nobj = fobjtmp;
        otmp->where = OBJ_FREE;
        fobjtmp = otmp;
    }
    /* fobj should now be empty */

    /* Set level.objects (as well as reversing the chain back again) */
    while ((otmp = fobjtmp) != 0) {
        fobjtmp = otmp->nobj;
        place_object(otmp, otmp->ox, otmp->oy);
    }
}

/* Things that were marked "in_use" when the game was saved (ex. via the
 * infamous "HUP" cheat) get used up here.
 */
void
inven_inuse(quietly)
boolean quietly;
{
    register struct obj *otmp, *otmp2;

    for (otmp = invent; otmp; otmp = otmp2) {
        otmp2 = otmp->nobj;
        if (otmp->in_use) {
            if (!quietly)
                pline("Finishing off %s...", xname(otmp));
            useup(otmp);
        }
    }
}

STATIC_OVL void
restlevchn(fd)
register int fd;
{
    int cnt;
    s_level *tmplev, *x;

    sp_levchn = (s_level *) 0;
    mread(fd, (genericptr_t) &cnt, sizeof(int));
    for (; cnt > 0; cnt--) {
        tmplev = (s_level *) alloc(sizeof(s_level));
        mread(fd, (genericptr_t) tmplev, sizeof(s_level));
        if (!sp_levchn)
            sp_levchn = tmplev;
        else {
            for (x = sp_levchn; x->next; x = x->next)
                ;
            x->next = tmplev;
        }
        tmplev->next = (s_level *) 0;
    }
}

STATIC_OVL void
restdamage(fd, ghostly)
int fd;
boolean ghostly;
{
    int counter;
    struct damage *tmp_dam;

    mread(fd, (genericptr_t) &counter, sizeof(counter));
    if (!counter)
        return;
    tmp_dam = (struct damage *) alloc(sizeof(struct damage));
    while (--counter >= 0) {
        char damaged_shops[5], *shp = (char *) 0;

        mread(fd, (genericptr_t) tmp_dam, sizeof(*tmp_dam));
        if (ghostly)
            tmp_dam->when += (monstermoves - omoves);
        Strcpy(damaged_shops,
               in_rooms(tmp_dam->place.x, tmp_dam->place.y, SHOPBASE));
        if (u.uz.dlevel) {
            /* when restoring, there are two passes over the current
             * level.  the first time, u.uz isn't set, so neither is
             * shop_keeper().  just wait and process the damage on
             * the second pass.
             */
            for (shp = damaged_shops; *shp; shp++) {
                struct monst *shkp = shop_keeper(*shp);

                if (shkp && inhishop(shkp)
                    && repair_damage(shkp, tmp_dam, (int *) 0, TRUE))
                    break;
            }
        }
        if (!shp || !*shp) {
            tmp_dam->next = level.damagelist;
            level.damagelist = tmp_dam;
            tmp_dam = (struct damage *) alloc(sizeof(*tmp_dam));
        }
    }
    free((genericptr_t) tmp_dam);
}

/* restore one object */
STATIC_OVL void
restobj(fd, otmp)
int fd;
struct obj *otmp;
{
    int buflen;

    mread(fd, (genericptr_t) otmp, sizeof(struct obj));

    /* next object pointers are invalid; otmp->cobj needs to be left
       as is--being non-null is key to restoring container contents */
    otmp->nobj = otmp->nexthere = (struct obj *) 0;
    /* non-null oextra needs to be reconstructed */
    if (otmp->oextra) {
        otmp->oextra = newoextra();

        /* oname - object's name */
        mread(fd, (genericptr_t) &buflen, sizeof(buflen));
        if (buflen > 0) { /* includes terminating '\0' */
            new_oname(otmp, buflen);
            mread(fd, (genericptr_t) ONAME(otmp), buflen);
        }
        /* omonst - corpse or statue might retain full monster details */
        mread(fd, (genericptr_t) &buflen, sizeof(buflen));
        if (buflen > 0) {
            newomonst(otmp);
            /* this is actually a monst struct, so we
               can just defer to restmon() here */
            restmon(fd, OMONST(otmp));
        }
        /* omid - monster id number, connecting corpse to ghost */
        mread(fd, (genericptr_t) &buflen, sizeof(buflen));
        if (buflen > 0) {
            newomid(otmp);
            mread(fd, (genericptr_t) OMID(otmp), buflen);
        }
        /* olong - temporary gold */
        mread(fd, (genericptr_t) &buflen, sizeof(buflen));
        if (buflen > 0) {
            newolong(otmp);
            mread(fd, (genericptr_t) OLONG(otmp), buflen);
        }
        /* omailcmd - feedback mechanism for scroll of mail */
        mread(fd, (genericptr_t) &buflen, sizeof(buflen));
        if (buflen > 0) {
            char *omailcmd = (char *) alloc(buflen);

            mread(fd, (genericptr_t) omailcmd, buflen);
            new_omailcmd(otmp, omailcmd);
            free((genericptr_t) omailcmd);
        }
    }
}

STATIC_OVL struct obj *
restobjchn(fd, ghostly, frozen)
register int fd;
boolean ghostly, frozen;
{
    register struct obj *otmp, *otmp2 = 0;
    register struct obj *first = (struct obj *) 0;
    int buflen;

    while (1) {
        mread(fd, (genericptr_t) &buflen, sizeof buflen);
        if (buflen == -1)
            break;

        otmp = newobj();
        restobj(fd, otmp);
        if (!first)
            first = otmp;
        else
            otmp2->nobj = otmp;

        if (ghostly) {
            unsigned nid = context.ident++;
            add_id_mapping(otmp->o_id, nid);
            otmp->o_id = nid;
        }
        if (ghostly && otmp->otyp == SLIME_MOLD)
            ghostfruit(otmp);
        /* Ghost levels get object age shifted from old player's clock
         * to new player's clock.  Assumption: new player arrived
         * immediately after old player died.
         */
        if (ghostly && !frozen && !age_is_relative(otmp))
            otmp->age = monstermoves - omoves + otmp->age;

        /* get contents of a container or statue */
        if (Has_contents(otmp)) {
            struct obj *otmp3;

            otmp->cobj = restobjchn(fd, ghostly, Is_IceBox(otmp));
            /* restore container back pointers */
            for (otmp3 = otmp->cobj; otmp3; otmp3 = otmp3->nobj)
                otmp3->ocontainer = otmp;
        } else if (SchroedingersBox(otmp)) {
            struct obj *catcorpse;

            /*
             * TODO:  Remove this after 3.6.x save compatibility is dropped.
             *
             * As of 3.6.2, SchroedingersBox() always has a cat corpse in it.
             * For 3.6.[01], it was empty and its weight was falsified
             * to have the value it would have had if there was one inside.
             * Put a non-rotting cat corpse in this box to convert to 3.6.2.
             *
             * [Note: after this fix up, future save/restore of this object
             * will take the Has_contents() code path above.]
             */
            if ((catcorpse = mksobj(CORPSE, TRUE, FALSE)) != 0) {
                otmp->spe = 1; /* flag for special SchroedingersBox */
                set_corpsenm(catcorpse, PM_HOUSECAT);
                (void) stop_timer(ROT_CORPSE, obj_to_any(catcorpse));
                add_to_container(otmp, catcorpse);
                otmp->owt = weight(otmp);
            }
        }
        if (otmp->bypass)
            otmp->bypass = 0;
        if (!ghostly) {
            /* fix the pointers */
            if (otmp->o_id == context.victual.o_id)
                context.victual.piece = otmp;
            if (otmp->o_id == context.tin.o_id)
                context.tin.tin = otmp;
            if (otmp->o_id == context.spbook.o_id)
                context.spbook.book = otmp;
        }
        otmp2 = otmp;
    }
    if (first && otmp2->nobj) {
        impossible("Restobjchn: error reading objchn.");
        otmp2->nobj = 0;
    }

    return first;
}

/* restore one monster */
STATIC_OVL void
restmon(fd, mtmp)
int fd;
struct monst *mtmp;
{
    int buflen;

    mread(fd, (genericptr_t) mtmp, sizeof(struct monst));

    /* next monster pointer is invalid */
    mtmp->nmon = (struct monst *) 0;
    /* non-null mextra needs to be reconstructed */
    if (mtmp->mextra) {
        mtmp->mextra = newmextra();

        /* mname - monster's name */
        mread(fd, (genericptr_t) &buflen, sizeof(buflen));
        if (buflen > 0) { /* includes terminating '\0' */
            new_mname(mtmp, buflen);
            mread(fd, (genericptr_t) MNAME(mtmp), buflen);
        }
        /* egd - vault guard */
        mread(fd, (genericptr_t) &buflen, sizeof(buflen));
        if (buflen > 0) {
            newegd(mtmp);
            mread(fd, (genericptr_t) EGD(mtmp), sizeof(struct egd));
        }
        /* epri - temple priest */
        mread(fd, (genericptr_t) &buflen, sizeof(buflen));
        if (buflen > 0) {
            newepri(mtmp);
            mread(fd, (genericptr_t) EPRI(mtmp), sizeof(struct epri));
        }
        /* eshk - shopkeeper */
        mread(fd, (genericptr_t) &buflen, sizeof(buflen));
        if (buflen > 0) {
            neweshk(mtmp);
            mread(fd, (genericptr_t) ESHK(mtmp), sizeof(struct eshk));
        }
        /* emin - minion */
        mread(fd, (genericptr_t) &buflen, sizeof(buflen));
        if (buflen > 0) {
            newemin(mtmp);
            mread(fd, (genericptr_t) EMIN(mtmp), sizeof(struct emin));
        }
        /* edog - pet */
        mread(fd, (genericptr_t) &buflen, sizeof(buflen));
        if (buflen > 0) {
            newedog(mtmp);
            mread(fd, (genericptr_t) EDOG(mtmp), sizeof(struct edog));
        }
        /* mcorpsenm - obj->corpsenm for mimic posing as corpse or
           statue (inline int rather than pointer to something) */
        mread(fd, (genericptr_t) &MCORPSENM(mtmp), sizeof MCORPSENM(mtmp));
    } /* mextra */
}

STATIC_OVL struct monst *
restmonchn(fd, ghostly)
register int fd;
boolean ghostly;
{
    register struct monst *mtmp, *mtmp2 = 0;
    register struct monst *first = (struct monst *) 0;
    int offset, buflen;

    while (1) {
        mread(fd, (genericptr_t) &buflen, sizeof(buflen));
        if (buflen == -1)
            break;

        mtmp = newmonst();
        restmon(fd, mtmp);
        if (!first)
            first = mtmp;
        else
            mtmp2->nmon = mtmp;

        if (ghostly) {
            unsigned nid = context.ident++;
            add_id_mapping(mtmp->m_id, nid);
            mtmp->m_id = nid;
        }
        offset = mtmp->mnum;
        mtmp->data = &mons[offset];
        if (ghostly) {
            int mndx = monsndx(mtmp->data);
            if (propagate(mndx, TRUE, ghostly) == 0) {
                /* cookie to trigger purge in getbones() */
                mtmp->mhpmax = DEFUNCT_MONSTER;
            }
        }
        if (mtmp->minvent) {
            struct obj *obj;
            mtmp->minvent = restobjchn(fd, ghostly, FALSE);
            /* restore monster back pointer */
            for (obj = mtmp->minvent; obj; obj = obj->nobj)
                obj->ocarry = mtmp;
        }
        if (mtmp->mw) {
            struct obj *obj;

            for (obj = mtmp->minvent; obj; obj = obj->nobj)
                if (obj->owornmask & W_WEP)
                    break;
            if (obj)
                mtmp->mw = obj;
            else {
                MON_NOWEP(mtmp);
                impossible("bad monster weapon restore");
            }
        }

        if (mtmp->isshk)
            restshk(mtmp, ghostly);
        if (mtmp->ispriest)
            restpriest(mtmp, ghostly);

        if (!ghostly) {
            if (mtmp->m_id == context.polearm.m_id)
                context.polearm.hitmon = mtmp;
        }
        mtmp2 = mtmp;
    }
    if (first && mtmp2->nmon) {
        impossible("Restmonchn: error reading monchn.");
        mtmp2->nmon = 0;
    }
    return first;
}

STATIC_OVL struct fruit *
loadfruitchn(fd)
int fd;
{
    register struct fruit *flist, *fnext;

    flist = 0;
    while (fnext = newfruit(), mread(fd, (genericptr_t) fnext, sizeof *fnext),
           fnext->fid != 0) {
        fnext->nextf = flist;
        flist = fnext;
    }
    dealloc_fruit(fnext);
    return flist;
}

STATIC_OVL void
freefruitchn(flist)
register struct fruit *flist;
{
    register struct fruit *fnext;

    while (flist) {
        fnext = flist->nextf;
        dealloc_fruit(flist);
        flist = fnext;
    }
}

STATIC_OVL void
ghostfruit(otmp)
register struct obj *otmp;
{
    register struct fruit *oldf;

    for (oldf = oldfruit; oldf; oldf = oldf->nextf)
        if (oldf->fid == otmp->spe)
            break;

    if (!oldf)
        impossible("no old fruit?");
    else
        otmp->spe = fruitadd(oldf->fname, (struct fruit *) 0);
}

#ifdef SYSCF
#define SYSOPT_CHECK_SAVE_UID sysopt.check_save_uid
#else
#define SYSOPT_CHECK_SAVE_UID TRUE
#endif

STATIC_OVL
boolean
restgamestate(fd, stuckid, steedid)
register int fd;
unsigned int *stuckid, *steedid;
{
    struct flag newgameflags;
#ifdef SYSFLAGS
    struct sysflag newgamesysflags;
#endif
    struct context_info newgamecontext; /* all 0, but has some pointers */
    struct obj *otmp;
    struct obj *bc_obj;
    char timebuf[15];
    unsigned long uid;
    boolean defer_perm_invent;

    mread(fd, (genericptr_t) &uid, sizeof uid);
    if (SYSOPT_CHECK_SAVE_UID
        && uid != (unsigned long) getuid()) { /* strange ... */
        /* for wizard mode, issue a reminder; for others, treat it
           as an attempt to cheat and refuse to restore this file */
        pline("Saved game was not yours.");
        if (!wizard)
            return FALSE;
    }

    newgamecontext = context; /* copy statically init'd context */
    mread(fd, (genericptr_t) &context, sizeof (struct context_info));
    context.warntype.species = (context.warntype.speciesidx >= LOW_PM)
                                  ? &mons[context.warntype.speciesidx]
                                  : (struct permonst *) 0;
    /* context.victual.piece, .tin.tin, .spellbook.book, and .polearm.hitmon
       are pointers which get set to Null during save and will be recovered
       via corresponding o_id or m_id while objs or mons are being restored */

    /* we want to be able to revert to command line/environment/config
       file option values instead of keeping old save file option values
       if partial restore fails and we resort to starting a new game */
    newgameflags = flags;
    mread(fd, (genericptr_t) &flags, sizeof (struct flag));
    /* avoid keeping permanent inventory window up to date during restore
       (setworn() calls update_inventory); attempting to include the cost
       of unpaid items before shopkeeper's bill is available is a no-no;
       named fruit names aren't accessible yet either
       [3.6.2: moved perm_invent from flags to iflags to keep it out of
       save files; retaining the override here is simpler than trying to
       to figure out where it really belongs now] */
    defer_perm_invent = iflags.perm_invent;
    iflags.perm_invent = FALSE;
    /* wizard and discover are actually flags.debug and flags.explore;
       player might be overriding the save file values for them;
       in the discover case, we don't want to set that for a normal
       game until after the save file has been removed */
    iflags.deferred_X = (newgameflags.explore && !discover);
    if (newgameflags.debug) {
        /* authorized by startup code; wizard mode exists and is allowed */
        wizard = TRUE, discover = iflags.deferred_X = FALSE;
    } else if (wizard) {
        /* specified by save file; check authorization now */
        set_playmode();
    }
#ifdef SYSFLAGS
    newgamesysflags = sysflags;
    mread(fd, (genericptr_t) &sysflags, sizeof(struct sysflag));
#endif

    role_init(); /* Reset the initial role, race, gender, and alignment */
#ifdef AMII_GRAPHICS
    amii_setpens(amii_numcolors); /* use colors from save file */
#endif
    mread(fd, (genericptr_t) &u, sizeof(struct you));

#define ReadTimebuf(foo)                   \
    mread(fd, (genericptr_t) timebuf, 14); \
    timebuf[14] = '\0';                    \
    foo = time_from_yyyymmddhhmmss(timebuf);

    ReadTimebuf(ubirthday);
    mread(fd, &urealtime.realtime, sizeof urealtime.realtime);
    ReadTimebuf(urealtime.start_timing); /** [not used] **/
    /* current time is the time to use for next urealtime.realtime update */
    urealtime.start_timing = getnow();

    set_uasmon();
#ifdef CLIPPING
    cliparound(u.ux, u.uy);
#endif
    if (u.uhp <= 0 && (!Upolyd || u.mh <= 0)) {
        u.ux = u.uy = 0; /* affects pline() [hence You()] */
        You("were not healthy enough to survive restoration.");
        /* wiz1_level.dlevel is used by mklev.c to see if lots of stuff is
         * uninitialized, so we only have to set it and not the other stuff.
         */
        wiz1_level.dlevel = 0;
        u.uz.dnum = 0;
        u.uz.dlevel = 1;
        /* revert to pre-restore option settings */
        iflags.deferred_X = FALSE;
        iflags.perm_invent = defer_perm_invent;
        flags = newgameflags;
#ifdef SYSFLAGS
        sysflags = newgamesysflags;
#endif
        context = newgamecontext;
        return FALSE;
    }
    /* in case hangup save occurred in midst of level change */
    assign_level(&u.uz0, &u.uz);

    /* this stuff comes after potential aborted restore attempts */
    restore_killers(fd);
    restore_timers(fd, RANGE_GLOBAL, FALSE, 0L);
    restore_light_sources(fd);
    invent = restobjchn(fd, FALSE, FALSE);

    /* restore dangling (not on floor or in inventory) ball and/or chain */
    bc_obj = restobjchn(fd, FALSE, FALSE);
    while (bc_obj) {
        struct obj *nobj = bc_obj->nobj;

        if (bc_obj->owornmask)
            setworn(bc_obj, bc_obj->owornmask);
        bc_obj->nobj = (struct obj *) 0;
        bc_obj = nobj;
    }

    migrating_objs = restobjchn(fd, FALSE, FALSE);
    migrating_mons = restmonchn(fd, FALSE);
    mread(fd, (genericptr_t) mvitals, sizeof mvitals);

    /*
     * There are some things after this that can have unintended display
     * side-effects too early in the game.
     * Disable see_monsters() here, re-enable it at the top of moveloop()
     */
    defer_see_monsters = TRUE;

    /* this comes after inventory has been loaded */
    for (otmp = invent; otmp; otmp = otmp->nobj)
        if (otmp->owornmask)
            setworn(otmp, otmp->owornmask);

    /* reset weapon so that player will get a reminder about "bashing"
       during next fight when bare-handed or wielding an unconventional
       item; for pick-axe, we aren't able to distinguish between having
       applied or wielded it, so be conservative and assume the former */
    otmp = uwep;   /* `uwep' usually init'd by setworn() in loop above */
    uwep = 0;      /* clear it and have setuwep() reinit */
    setuwep(otmp); /* (don't need any null check here) */
    if (!uwep || uwep->otyp == PICK_AXE || uwep->otyp == GRAPPLING_HOOK)
        unweapon = TRUE;

    restore_dungeon(fd);
    restlevchn(fd);
    mread(fd, (genericptr_t) &moves, sizeof moves);
    mread(fd, (genericptr_t) &monstermoves, sizeof monstermoves);
    mread(fd, (genericptr_t) &quest_status, sizeof (struct q_score));
    mread(fd, (genericptr_t) spl_book, (MAXSPELL + 1) * sizeof (struct spell));
    restore_artifacts(fd);
    restore_oracles(fd);
    if (u.ustuck)
        mread(fd, (genericptr_t) stuckid, sizeof *stuckid);
    if (u.usteed)
        mread(fd, (genericptr_t) steedid, sizeof *steedid);
    mread(fd, (genericptr_t) pl_character, sizeof pl_character);

    mread(fd, (genericptr_t) pl_fruit, sizeof pl_fruit);
    freefruitchn(ffruit); /* clean up fruit(s) made by initoptions() */
    ffruit = loadfruitchn(fd);

    restnames(fd);
    restore_waterlevel(fd);
    restore_msghistory(fd);
    /* must come after all mons & objs are restored */
    relink_timers(FALSE);
    relink_light_sources(FALSE);
    /* inventory display is now viable */
    iflags.perm_invent = defer_perm_invent;
    return TRUE;
}

/* update game state pointers to those valid for the current level (so we
 * don't dereference a wild u.ustuck when saving the game state, for instance)
 */
STATIC_OVL void
restlevelstate(stuckid, steedid)
unsigned int stuckid, steedid;
{
    register struct monst *mtmp;

    if (stuckid) {
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
            if (mtmp->m_id == stuckid)
                break;
        if (!mtmp)
            panic("Cannot find the monster ustuck.");
        u.ustuck = mtmp;
    }
    if (steedid) {
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
            if (mtmp->m_id == steedid)
                break;
        if (!mtmp)
            panic("Cannot find the monster usteed.");
        u.usteed = mtmp;
        remove_monster(mtmp->mx, mtmp->my);
    }
}

/*ARGSUSED*/
STATIC_OVL int
restlevelfile(fd, ltmp)
int fd; /* fd used in MFLOPPY only */
xchar ltmp;
{
    int nfd;
    char whynot[BUFSZ];

#ifndef MFLOPPY
    nhUse(fd);
#endif
    nfd = create_levelfile(ltmp, whynot);
    if (nfd < 0) {
        /* BUG: should suppress any attempt to write a panic
           save file if file creation is now failing... */
        panic("restlevelfile: %s", whynot);
    }
#ifdef MFLOPPY
    if (!savelev(nfd, ltmp, COUNT_SAVE)) {
        /* The savelev can't proceed because the size required
         * is greater than the available disk space.
         */
        pline("Not enough space on `%s' to restore your game.", levels);

        /* Remove levels and bones that may have been created.
         */
        (void) nhclose(nfd);
#ifdef AMIGA
        clearlocks();
#else /* !AMIGA */
        eraseall(levels, alllevels);
        eraseall(levels, allbones);

        /* Perhaps the person would like to play without a
         * RAMdisk.
         */
        if (ramdisk) {
            /* PlaywoRAMdisk may not return, but if it does
             * it is certain that ramdisk will be 0.
             */
            playwoRAMdisk();
            /* Rewind save file and try again */
            (void) lseek(fd, (off_t) 0, 0);
            (void) validate(fd, (char *) 0); /* skip version etc */
            return dorecover(fd);            /* 0 or 1 */
        }
#endif /* ?AMIGA */
        pline("Be seeing you...");
        nh_terminate(EXIT_SUCCESS);
    }
#endif /* MFLOPPY */
    bufon(nfd);
    savelev(nfd, ltmp, WRITE_SAVE | FREE_SAVE);
    bclose(nfd);
    return 2;
}

int
dorecover(fd)
register int fd;
{
    unsigned int stuckid = 0, steedid = 0; /* not a register */
    xchar ltmp;
    int rtmp;
    struct obj *otmp;

    restoring = TRUE;
    get_plname_from_file(fd, plname);
    getlev(fd, 0, (xchar) 0, FALSE);
    if (!restgamestate(fd, &stuckid, &steedid)) {
        display_nhwindow(WIN_MESSAGE, TRUE);
        savelev(-1, 0, FREE_SAVE); /* discard current level */
        (void) nhclose(fd);
        (void) delete_savefile();
        restoring = FALSE;
        return 0;
    }
    restlevelstate(stuckid, steedid);
#ifdef INSURANCE
    savestateinlock();
#endif
    rtmp = restlevelfile(fd, ledger_no(&u.uz));
    if (rtmp < 2)
        return rtmp; /* dorecover called recursively */

    /* these pointers won't be valid while we're processing the
     * other levels, but they'll be reset again by restlevelstate()
     * afterwards, and in the meantime at least u.usteed may mislead
     * place_monster() on other levels
     */
    u.ustuck = (struct monst *) 0;
    u.usteed = (struct monst *) 0;

#ifdef MICRO
#ifdef AMII_GRAPHICS
    {
        extern struct window_procs amii_procs;
        if (WINDOWPORT("amii") {
            extern winid WIN_BASE;
            clear_nhwindow(WIN_BASE); /* hack until there's a hook for this */
        }
    }
#else
        clear_nhwindow(WIN_MAP);
#endif
    clear_nhwindow(WIN_MESSAGE);
    You("return to level %d in %s%s.", depth(&u.uz),
        dungeons[u.uz.dnum].dname,
        flags.debug ? " while in debug mode"
                    : flags.explore ? " while in explore mode" : "");
    curs(WIN_MAP, 1, 1);
    dotcnt = 0;
    dotrow = 2;
    if (!WINDOWPORT("X11"))
        putstr(WIN_MAP, 0, "Restoring:");
#endif
    restoreprocs.mread_flags = 1; /* return despite error */
    while (1) {
        mread(fd, (genericptr_t) &ltmp, sizeof ltmp);
        if (restoreprocs.mread_flags == -1)
            break;
        getlev(fd, 0, ltmp, FALSE);
#ifdef MICRO
        curs(WIN_MAP, 1 + dotcnt++, dotrow);
        if (dotcnt >= (COLNO - 1)) {
            dotrow++;
            dotcnt = 0;
        }
        if (!WINDOWPORT("X11")) {
            putstr(WIN_MAP, 0, ".");
        }
        mark_synch();
#endif
        rtmp = restlevelfile(fd, ltmp);
        if (rtmp < 2)
            return rtmp; /* dorecover called recursively */
    }
    restoreprocs.mread_flags = 0;

#ifdef BSD
    (void) lseek(fd, 0L, 0);
#else
    (void) lseek(fd, (off_t) 0, 0);
#endif
    (void) validate(fd, (char *) 0); /* skip version and savefile info */
    get_plname_from_file(fd, plname);

    getlev(fd, 0, (xchar) 0, FALSE);
    (void) nhclose(fd);

    /* Now set the restore settings to match the
     * settings used by the save file output routines
     */
    reset_restpref();

    restlevelstate(stuckid, steedid);
    program_state.something_worth_saving = 1; /* useful data now exists */

    if (!wizard && !discover)
        (void) delete_savefile();
    if (Is_rogue_level(&u.uz))
        assign_graphics(ROGUESET);
#ifdef USE_TILES
    substitute_tiles(&u.uz);
#endif
#ifdef MFLOPPY
    gameDiskPrompt();
#endif
    max_rank_sz(); /* to recompute mrank_sz (botl.c) */
    /* take care of iron ball & chain */
    for (otmp = fobj; otmp; otmp = otmp->nobj)
        if (otmp->owornmask)
            setworn(otmp, otmp->owornmask);

    if ((uball && !uchain) || (uchain && !uball)) {
        impossible("restgamestate: lost ball & chain");
        /* poor man's unpunish() */
        setworn((struct obj *) 0, W_CHAIN);
        setworn((struct obj *) 0, W_BALL);
    }

    /* in_use processing must be after:
     *    + The inventory has been read so that freeinv() works.
     *    + The current level has been restored so billing information
     *      is available.
     */
    inven_inuse(FALSE);

    load_qtlist(); /* re-load the quest text info */
    /* Set up the vision internals, after levl[] data is loaded
       but before docrt(). */
    reglyph_darkroom();
    vision_reset();
    vision_full_recalc = 1; /* recompute vision (not saved) */

    run_timers(); /* expire all timers that have gone off while away */
    docrt();
    restoring = FALSE;
    clear_nhwindow(WIN_MESSAGE);

    /* Success! */
    welcome(FALSE);
    check_special_room(FALSE);
    return 1;
}

void
restcemetery(fd, cemeteryaddr)
int fd;
struct cemetery **cemeteryaddr;
{
    struct cemetery *bonesinfo, **bonesaddr;
    int flag;

    mread(fd, (genericptr_t) &flag, sizeof flag);
    if (flag == 0) {
        bonesaddr = cemeteryaddr;
        do {
            bonesinfo = (struct cemetery *) alloc(sizeof *bonesinfo);
            mread(fd, (genericptr_t) bonesinfo, sizeof *bonesinfo);
            *bonesaddr = bonesinfo;
            bonesaddr = &(*bonesaddr)->next;
        } while (*bonesaddr);
    } else {
        *cemeteryaddr = 0;
    }
}

/*ARGSUSED*/
STATIC_OVL void
rest_levl(fd, rlecomp)
int fd;
boolean rlecomp;
{
#ifdef RLECOMP
    short i, j;
    uchar len;
    struct rm r;

    if (rlecomp) {
        (void) memset((genericptr_t) &r, 0, sizeof(r));
        i = 0;
        j = 0;
        len = 0;
        while (i < ROWNO) {
            while (j < COLNO) {
                if (len > 0) {
                    levl[j][i] = r;
                    len -= 1;
                    j += 1;
                } else {
                    mread(fd, (genericptr_t) &len, sizeof(uchar));
                    mread(fd, (genericptr_t) &r, sizeof(struct rm));
                }
            }
            j = 0;
            i += 1;
        }
        return;
    }
#else /* !RLECOMP */
    nhUse(rlecomp);
#endif /* ?RLECOMP */
    mread(fd, (genericptr_t) levl, sizeof levl);
}

void
trickery(reason)
char *reason;
{
    pline("Strange, this map is not as I remember it.");
    pline("Somebody is trying some trickery here...");
    pline("This game is void.");
    Strcpy(killer.name, reason ? reason : "");
    done(TRICKED);
}

void
getlev(fd, pid, lev, ghostly)
int fd, pid;
xchar lev;
boolean ghostly;
{
    register struct trap *trap;
    register struct monst *mtmp;
    long elapsed;
    branch *br;
    int hpid;
    xchar dlvl;
    int x, y;
#ifdef TOS
    short tlev;
#endif

    if (ghostly)
        clear_id_mapping();

#if defined(MSDOS) || defined(OS2)
    setmode(fd, O_BINARY);
#endif
    /* Load the old fruit info.  We have to do it first, so the
     * information is available when restoring the objects.
     */
    if (ghostly)
        oldfruit = loadfruitchn(fd);

    /* First some sanity checks */
    mread(fd, (genericptr_t) &hpid, sizeof(hpid));
/* CHECK:  This may prevent restoration */
#ifdef TOS
    mread(fd, (genericptr_t) &tlev, sizeof(tlev));
    dlvl = tlev & 0x00ff;
#else
    mread(fd, (genericptr_t) &dlvl, sizeof(dlvl));
#endif
    if ((pid && pid != hpid) || (lev && dlvl != lev)) {
        char trickbuf[BUFSZ];

        if (pid && pid != hpid)
            Sprintf(trickbuf, "PID (%d) doesn't match saved PID (%d)!", hpid,
                    pid);
        else
            Sprintf(trickbuf, "This is level %d, not %d!", dlvl, lev);
        if (wizard)
            pline1(trickbuf);
        trickery(trickbuf);
    }
    restcemetery(fd, &level.bonesinfo);
    rest_levl(fd,
              (boolean) ((sfrestinfo.sfi1 & SFI1_RLECOMP) == SFI1_RLECOMP));
    mread(fd, (genericptr_t) lastseentyp, sizeof(lastseentyp));
    mread(fd, (genericptr_t) &omoves, sizeof(omoves));
    elapsed = monstermoves - omoves;
    mread(fd, (genericptr_t) &upstair, sizeof(stairway));
    mread(fd, (genericptr_t) &dnstair, sizeof(stairway));
    mread(fd, (genericptr_t) &upladder, sizeof(stairway));
    mread(fd, (genericptr_t) &dnladder, sizeof(stairway));
    mread(fd, (genericptr_t) &sstairs, sizeof(stairway));
    mread(fd, (genericptr_t) &updest, sizeof(dest_area));
    mread(fd, (genericptr_t) &dndest, sizeof(dest_area));
    mread(fd, (genericptr_t) &level.flags, sizeof(level.flags));
    mread(fd, (genericptr_t) doors, sizeof(doors));
    rest_rooms(fd); /* No joke :-) */
    if (nroom)
        doorindex = rooms[nroom - 1].fdoor + rooms[nroom - 1].doorct;
    else
        doorindex = 0;

    restore_timers(fd, RANGE_LEVEL, ghostly, elapsed);
    restore_light_sources(fd);
    fmon = restmonchn(fd, ghostly);

    rest_worm(fd); /* restore worm information */
    ftrap = 0;
    while (trap = newtrap(),
           mread(fd, (genericptr_t) trap, sizeof(struct trap)),
           trap->tx != 0) { /* need "!= 0" to work around DICE 3.0 bug */
        trap->ntrap = ftrap;
        ftrap = trap;
    }
    dealloc_trap(trap);
    fobj = restobjchn(fd, ghostly, FALSE);
    find_lev_obj();
    /* restobjchn()'s `frozen' argument probably ought to be a callback
       routine so that we can check for objects being buried under ice */
    level.buriedobjlist = restobjchn(fd, ghostly, FALSE);
    billobjs = restobjchn(fd, ghostly, FALSE);
    rest_engravings(fd);

    /* reset level.monsters for new level */
    for (x = 0; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++)
            level.monsters[x][y] = (struct monst *) 0;
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (mtmp->isshk)
            set_residency(mtmp, FALSE);
        place_monster(mtmp, mtmp->mx, mtmp->my);
        if (mtmp->wormno)
            place_wsegs(mtmp, NULL);

        /* regenerate monsters while on another level */
        if (!u.uz.dlevel)
            continue;
        if (ghostly) {
            /* reset peaceful/malign relative to new character;
               shopkeepers will reset based on name */
            if (!mtmp->isshk)
                mtmp->mpeaceful =
                    (is_unicorn(mtmp->data)
                     && sgn(u.ualign.type) == sgn(mtmp->data->maligntyp))
                        ? TRUE
                        : peace_minded(mtmp->data);
            set_malign(mtmp);
        } else if (elapsed > 0L) {
            mon_catchup_elapsed_time(mtmp, elapsed);
        }
        /* update shape-changers in case protection against
           them is different now than when the level was saved */
        restore_cham(mtmp);
        /* give hiders a chance to hide before their next move */
        if (ghostly || (elapsed > 00 && elapsed > (long) rnd(10)))
            hide_monst(mtmp);
    }

    restdamage(fd, ghostly);
    rest_regions(fd, ghostly);
    if (ghostly) {
        /* Now get rid of all the temp fruits... */
        freefruitchn(oldfruit), oldfruit = 0;

        if (lev > ledger_no(&medusa_level)
            && lev < ledger_no(&stronghold_level) && xdnstair == 0) {
            coord cc;

            mazexy(&cc);
            xdnstair = cc.x;
            ydnstair = cc.y;
            levl[cc.x][cc.y].typ = STAIRS;
        }

        br = Is_branchlev(&u.uz);
        if (br && u.uz.dlevel == 1) {
            d_level ltmp;

            if (on_level(&u.uz, &br->end1))
                assign_level(&ltmp, &br->end2);
            else
                assign_level(&ltmp, &br->end1);

            switch (br->type) {
            case BR_STAIR:
            case BR_NO_END1:
            case BR_NO_END2: /* OK to assign to sstairs if it's not used */
                assign_level(&sstairs.tolev, &ltmp);
                break;
            case BR_PORTAL: /* max of 1 portal per level */
                for (trap = ftrap; trap; trap = trap->ntrap)
                    if (trap->ttyp == MAGIC_PORTAL)
                        break;
                if (!trap)
                    panic("getlev: need portal but none found");
                assign_level(&trap->dst, &ltmp);
                break;
            }
        } else if (!br) {
            struct trap *ttmp = 0;

            /* Remove any dangling portals. */
            for (trap = ftrap; trap; trap = ttmp) {
                ttmp = trap->ntrap;
                if (trap->ttyp == MAGIC_PORTAL)
                    deltrap(trap);
            }
        }
    }

    /* must come after all mons & objs are restored */
    relink_timers(ghostly);
    relink_light_sources(ghostly);
    reset_oattached_mids(ghostly);

    if (ghostly)
        clear_id_mapping();
}

void
get_plname_from_file(fd, plbuf)
int fd;
char *plbuf;
{
    int pltmpsiz = 0;
    (void) read(fd, (genericptr_t) &pltmpsiz, sizeof(pltmpsiz));
    (void) read(fd, (genericptr_t) plbuf, pltmpsiz);
    return;
}

STATIC_OVL void
restore_msghistory(fd)
register int fd;
{
    int msgsize, msgcount = 0;
    char msg[BUFSZ];

    while (1) {
        mread(fd, (genericptr_t) &msgsize, sizeof(msgsize));
        if (msgsize == -1)
            break;
        if (msgsize > (BUFSZ - 1))
            panic("restore_msghistory: msg too big (%d)", msgsize);
        mread(fd, (genericptr_t) msg, msgsize);
        msg[msgsize] = '\0';
        putmsghistory(msg, TRUE);
        ++msgcount;
    }
    if (msgcount)
        putmsghistory((char *) 0, TRUE);
    debugpline1("Read %d messages from savefile.", msgcount);
}

/* Clear all structures for object and monster ID mapping. */
STATIC_OVL void
clear_id_mapping()
{
    struct bucket *curr;

    while ((curr = id_map) != 0) {
        id_map = curr->next;
        free((genericptr_t) curr);
    }
    n_ids_mapped = 0;
}

/* Add a mapping to the ID map. */
STATIC_OVL void
add_id_mapping(gid, nid)
unsigned gid, nid;
{
    int idx;

    idx = n_ids_mapped % N_PER_BUCKET;
    /* idx is zero on first time through, as well as when a new bucket is */
    /* needed */
    if (idx == 0) {
        struct bucket *gnu = (struct bucket *) alloc(sizeof(struct bucket));
        gnu->next = id_map;
        id_map = gnu;
    }

    id_map->map[idx].gid = gid;
    id_map->map[idx].nid = nid;
    n_ids_mapped++;
}

/*
 * Global routine to look up a mapping.  If found, return TRUE and fill
 * in the new ID value.  Otherwise, return false and return -1 in the new
 * ID.
 */
boolean
lookup_id_mapping(gid, nidp)
unsigned gid, *nidp;
{
    int i;
    struct bucket *curr;

    if (n_ids_mapped)
        for (curr = id_map; curr; curr = curr->next) {
            /* first bucket might not be totally full */
            if (curr == id_map) {
                i = n_ids_mapped % N_PER_BUCKET;
                if (i == 0)
                    i = N_PER_BUCKET;
            } else
                i = N_PER_BUCKET;

            while (--i >= 0)
                if (gid == curr->map[i].gid) {
                    *nidp = curr->map[i].nid;
                    return TRUE;
                }
        }

    return FALSE;
}

STATIC_OVL void
reset_oattached_mids(ghostly)
boolean ghostly;
{
    struct obj *otmp;
    unsigned oldid, nid;
    for (otmp = fobj; otmp; otmp = otmp->nobj) {
        if (ghostly && has_omonst(otmp)) {
            struct monst *mtmp = OMONST(otmp);

            mtmp->m_id = 0;
            mtmp->mpeaceful = mtmp->mtame = 0; /* pet's owner died! */
        }
        if (ghostly && has_omid(otmp)) {
            (void) memcpy((genericptr_t) &oldid, (genericptr_t) OMID(otmp),
                          sizeof(oldid));
            if (lookup_id_mapping(oldid, &nid))
                (void) memcpy((genericptr_t) OMID(otmp), (genericptr_t) &nid,
                              sizeof(nid));
            else
                free_omid(otmp);
        }
    }
}

#ifdef SELECTSAVED
/* put up a menu listing each character from this player's saved games;
   returns 1: use plname[], 0: new game, -1: quit */
int
restore_menu(bannerwin)
winid bannerwin; /* if not WIN_ERR, clear window and show copyright in menu */
{
    winid tmpwin;
    anything any;
    char **saved;
    menu_item *chosen_game = (menu_item *) 0;
    int k, clet, ch = 0; /* ch: 0 => new game */

    *plname = '\0';
    saved = get_saved_games(); /* array of character names */
    if (saved && *saved) {
        tmpwin = create_nhwindow(NHW_MENU);
        start_menu(tmpwin);
        any = zeroany; /* no selection */
        if (bannerwin != WIN_ERR) {
            /* for tty; erase copyright notice and redo it in the menu */
            clear_nhwindow(bannerwin);
            /* COPYRIGHT_BANNER_[ABCD] */
            for (k = 1; k <= 4; ++k)
                add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
                         copyright_banner_line(k), MENU_UNSELECTED);
            add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, "",
                     MENU_UNSELECTED);
        }
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
                 "Select one of your saved games", MENU_UNSELECTED);
        for (k = 0; saved[k]; ++k) {
            any.a_int = k + 1;
            add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, saved[k],
                     MENU_UNSELECTED);
        }
        clet = (k <= 'n' - 'a') ? 'n' : 0; /* new game */
        any.a_int = -1;                    /* not >= 0 */
        add_menu(tmpwin, NO_GLYPH, &any, clet, 0, ATR_NONE,
                 "Start a new character", MENU_UNSELECTED);
        clet = (k + 1 <= 'q' - 'a') ? 'q' : 0; /* quit */
        any.a_int = -2;
        add_menu(tmpwin, NO_GLYPH, &any, clet, 0, ATR_NONE,
                 "Never mind (quit)", MENU_SELECTED);
        /* no prompt on end_menu, as we've done our own at the top */
        end_menu(tmpwin, (char *) 0);
        if (select_menu(tmpwin, PICK_ONE, &chosen_game) > 0) {
            ch = chosen_game->item.a_int;
            if (ch > 0)
                Strcpy(plname, saved[ch - 1]);
            else if (ch < 0)
                ++ch; /* -1 -> 0 (new game), -2 -> -1 (quit) */
            free((genericptr_t) chosen_game);
        } else {
            ch = -1; /* quit menu without making a selection => quit */
        }
        destroy_nhwindow(tmpwin);
        if (bannerwin != WIN_ERR) {
            /* for tty; clear the menu away and put subset of copyright back
             */
            clear_nhwindow(bannerwin);
            /* COPYRIGHT_BANNER_A, preceding "Who are you?" prompt */
            if (ch == 0)
                putstr(bannerwin, 0, copyright_banner_line(1));
        }
    }
    free_saved_games(saved);
    return (ch > 0) ? 1 : ch;
}
#endif /* SELECTSAVED */

void
minit()
{
    (*restoreprocs.restore_minit)();
    return;
}

void
mread(fd, buf, len)
register int fd;
register genericptr_t buf;
register unsigned int len;
{
    (*restoreprocs.restore_mread)(fd, buf, len);
    return;
}

/* examine the version info and the savefile_info data
   that immediately follows it.
   Return 0 if it passed the checks.
   Return 1 if it failed the version check.
   Return 2 if it failed the savefile feature check.
   Return -1 if it failed for some unknown reason.
 */
int
validate(fd, name)
int fd;
const char *name;
{
    int rlen;
    struct savefile_info sfi;
    unsigned long compatible;
    boolean verbose = name ? TRUE : FALSE, reslt = FALSE;

    if (!(reslt = uptodate(fd, name)))
        return 1;

    rlen = read(fd, (genericptr_t) &sfi, sizeof sfi);
    minit(); /* ZEROCOMP */
    if (rlen == 0) {
        if (verbose) {
            pline("File \"%s\" is empty during save file feature check?",
                  name);
            wait_synch();
        }
        return -1;
    }

    compatible = (sfi.sfi1 & sfcap.sfi1);

    if ((sfi.sfi1 & SFI1_ZEROCOMP) == SFI1_ZEROCOMP) {
        if ((compatible & SFI1_ZEROCOMP) != SFI1_ZEROCOMP) {
            if (verbose) {
                pline("File \"%s\" has incompatible ZEROCOMP compression.",
                      name);
                wait_synch();
            }
            return 2;
        } else if ((sfrestinfo.sfi1 & SFI1_ZEROCOMP) != SFI1_ZEROCOMP) {
            set_restpref("zerocomp");
        }
    }

    if ((sfi.sfi1 & SFI1_EXTERNALCOMP) == SFI1_EXTERNALCOMP) {
        if ((compatible & SFI1_EXTERNALCOMP) != SFI1_EXTERNALCOMP) {
            if (verbose) {
                pline("File \"%s\" lacks required internal compression.",
                      name);
                wait_synch();
            }
            return 2;
        } else if ((sfrestinfo.sfi1 & SFI1_EXTERNALCOMP)
                   != SFI1_EXTERNALCOMP) {
            set_restpref("externalcomp");
        }
    }

    /* RLECOMP check must be last, after ZEROCOMP or INTERNALCOMP adjustments
     */
    if ((sfi.sfi1 & SFI1_RLECOMP) == SFI1_RLECOMP) {
        if ((compatible & SFI1_RLECOMP) != SFI1_RLECOMP) {
            if (verbose) {
                pline("File \"%s\" has incompatible run-length compression.",
                      name);
                wait_synch();
            }
            return 2;
        } else if ((sfrestinfo.sfi1 & SFI1_RLECOMP) != SFI1_RLECOMP) {
            set_restpref("rlecomp");
        }
    }
    /* savefile does not have RLECOMP level location compression, so adjust */
    else
        set_restpref("!rlecomp");

    return 0;
}

void
reset_restpref()
{
#ifdef ZEROCOMP
    if (iflags.zerocomp)
        set_restpref("zerocomp");
    else
#endif
        set_restpref("externalcomp");
#ifdef RLECOMP
    if (iflags.rlecomp)
        set_restpref("rlecomp");
    else
#endif
        set_restpref("!rlecomp");
}

void
set_restpref(suitename)
const char *suitename;
{
    if (!strcmpi(suitename, "externalcomp")) {
        restoreprocs.name = "externalcomp";
        restoreprocs.restore_mread = def_mread;
        restoreprocs.restore_minit = def_minit;
        sfrestinfo.sfi1 |= SFI1_EXTERNALCOMP;
        sfrestinfo.sfi1 &= ~SFI1_ZEROCOMP;
        def_minit();
    }
    if (!strcmpi(suitename, "!rlecomp")) {
        sfrestinfo.sfi1 &= ~SFI1_RLECOMP;
    }
#ifdef ZEROCOMP
    if (!strcmpi(suitename, "zerocomp")) {
        restoreprocs.name = "zerocomp";
        restoreprocs.restore_mread = zerocomp_mread;
        restoreprocs.restore_minit = zerocomp_minit;
        sfrestinfo.sfi1 |= SFI1_ZEROCOMP;
        sfrestinfo.sfi1 &= ~SFI1_EXTERNALCOMP;
        zerocomp_minit();
    }
#endif
#ifdef RLECOMP
    if (!strcmpi(suitename, "rlecomp")) {
        sfrestinfo.sfi1 |= SFI1_RLECOMP;
    }
#endif
}

#ifdef ZEROCOMP
#define RLESC '\0' /* Leading character for run of RLESC's */

#ifndef ZEROCOMP_BUFSIZ
#define ZEROCOMP_BUFSIZ BUFSZ
#endif
static NEARDATA unsigned char inbuf[ZEROCOMP_BUFSIZ];
static NEARDATA unsigned short inbufp = 0;
static NEARDATA unsigned short inbufsz = 0;
static NEARDATA short inrunlength = -1;
static NEARDATA int mreadfd;

STATIC_OVL int
zerocomp_mgetc()
{
    if (inbufp >= inbufsz) {
        inbufsz = read(mreadfd, (genericptr_t) inbuf, sizeof inbuf);
        if (!inbufsz) {
            if (inbufp > sizeof inbuf)
                error("EOF on file #%d.\n", mreadfd);
            inbufp = 1 + sizeof inbuf; /* exactly one warning :-) */
            return -1;
        }
        inbufp = 0;
    }
    return inbuf[inbufp++];
}

STATIC_OVL void
zerocomp_minit()
{
    inbufsz = 0;
    inbufp = 0;
    inrunlength = -1;
}

STATIC_OVL void
zerocomp_mread(fd, buf, len)
int fd;
genericptr_t buf;
register unsigned len;
{
    /*register int readlen = 0;*/
    if (fd < 0)
        error("Restore error; mread attempting to read file %d.", fd);
    mreadfd = fd;
    while (len--) {
        if (inrunlength > 0) {
            inrunlength--;
            *(*((char **) &buf))++ = '\0';
        } else {
            register short ch = zerocomp_mgetc();
            if (ch < 0) {
                restoreprocs.mread_flags = -1;
                return;
            }
            if ((*(*(char **) &buf)++ = (char) ch) == RLESC) {
                inrunlength = zerocomp_mgetc();
            }
        }
        /*readlen++;*/
    }
}
#endif /* ZEROCOMP */

STATIC_OVL void
def_minit()
{
    return;
}

STATIC_OVL void
def_mread(fd, buf, len)
register int fd;
register genericptr_t buf;
register unsigned int len;
{
    register int rlen;
#if defined(BSD) || defined(ULTRIX)
#define readLenType int
#else /* e.g. SYSV, __TURBOC__ */
#define readLenType unsigned
#endif

    rlen = read(fd, buf, (readLenType) len);
    if ((readLenType) rlen != (readLenType) len) {
        if (restoreprocs.mread_flags == 1) { /* means "return anyway" */
            restoreprocs.mread_flags = -1;
            return;
        } else {
            pline("Read %d instead of %u bytes.", rlen, len);
            if (restoring) {
                (void) nhclose(fd);
                (void) delete_savefile();
                error("Error restoring old game.");
            }
            panic("Error reading level file.");
        }
    }
}

/*restore.c*/
