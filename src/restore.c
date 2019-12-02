/* NetHack 3.7	restore.c	$NHDT-Date: 1575245087 2019/12/02 00:04:47 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.136 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2009. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "lev.h"
#include "tcap.h" /* for TERMLIB and ASCIIGRAPH */
#include "sfproto.h"



#if defined(MICRO)
extern int dotcnt; /* shared with save */
extern int dotrow; /* shared with save */
#endif

#ifdef USE_TILES
extern void FDECL(substitute_tiles, (d_level *)); /* from tile.c */
#endif

#ifdef ZEROCOMP
static void NDECL(zerocomp_minit);
static void FDECL(zerocomp_mread, (int, genericptr_t, unsigned int));
static int NDECL(zerocomp_mgetc);
#endif

static void NDECL(find_lev_obj);
static void FDECL(restlevchn, (NHFILE *));
static void FDECL(restdamage, (NHFILE *, BOOLEAN_P));
static void FDECL(restobj, (NHFILE *, struct obj *));
static struct obj *FDECL(restobjchn, (NHFILE *, BOOLEAN_P, BOOLEAN_P));
static void FDECL(restmon, (NHFILE *, struct monst *));
static struct monst *FDECL(restmonchn, (NHFILE *, BOOLEAN_P));
static struct fruit *FDECL(loadfruitchn, (NHFILE *));
static void FDECL(freefruitchn, (struct fruit *));
static void FDECL(ghostfruit, (struct obj *));
static boolean FDECL(restgamestate, (NHFILE *, unsigned int *, unsigned int *));
static void FDECL(restlevelstate, (unsigned int, unsigned int));
static int FDECL(restlevelfile, (NHFILE *, XCHAR_P));
static void FDECL(restore_msghistory, (NHFILE *));
static void FDECL(reset_oattached_mids, (BOOLEAN_P));
static void FDECL(rest_levl, (NHFILE *, BOOLEAN_P));

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

static void NDECL(clear_id_mapping);
static void FDECL(add_id_mapping, (unsigned, unsigned));

#ifdef AMII_GRAPHICS
void FDECL(amii_setpens, (int)); /* use colors from save file */
extern int amii_numcolors;
#endif

#include "display.h"

#define Is_IceBox(o) ((o)->otyp == ICE_BOX ? TRUE : FALSE)

/* Recalculate g.level.objects[x][y], since this info was not saved. */
static void
find_lev_obj()
{
    register struct obj *fobjtmp = (struct obj *) 0;
    register struct obj *otmp;
    int x, y;

    for (x = 0; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++)
            g.level.objects[x][y] = (struct obj *) 0;

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

    /* Set g.level.objects (as well as reversing the chain back again) */
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

    for (otmp = g.invent; otmp; otmp = otmp2) {
        otmp2 = otmp->nobj;
        if (otmp->in_use) {
            if (!quietly)
                pline("Finishing off %s...", xname(otmp));
            useup(otmp);
        }
    }
}

static void
restlevchn(nhfp)
NHFILE *nhfp;
{
    int cnt;
    s_level *tmplev, *x;

    g.sp_levchn = (s_level *) 0;
    if (nhfp->structlevel)
        mread(nhfp->fd, (genericptr_t) &cnt, sizeof(int));
    if (nhfp->fieldlevel)
        sfi_int(nhfp, &cnt, "levchn", "lev_count", 1);

    for (; cnt > 0; cnt--) {
        tmplev = (s_level *) alloc(sizeof(s_level));
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) tmplev, sizeof(s_level));
        if (nhfp->fieldlevel)
            sfi_s_level(nhfp, tmplev, "levchn", "s_level", 1);

        if (!g.sp_levchn)
            g.sp_levchn = tmplev;
        else {
            for (x = g.sp_levchn; x->next; x = x->next)
                ;
            x->next = tmplev;
        }
        tmplev->next = (s_level *) 0;
    }
}

static void
restdamage(nhfp, ghostly)
NHFILE *nhfp;
boolean ghostly;
{
    unsigned int dmgcount;
    int counter;
    struct damage *tmp_dam;

    if (nhfp->structlevel)
        mread(nhfp->fd, (genericptr_t) &dmgcount, sizeof(dmgcount));
    if (nhfp->fieldlevel)
        sfi_unsigned(nhfp, &dmgcount, "damage", "damage_count", 1);
    counter = (int) dmgcount;

    if (!counter)
        return;
    tmp_dam = (struct damage *) alloc(sizeof(struct damage));
    while (--counter >= 0) {
        char damaged_shops[5], *shp = (char *) 0;

        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) tmp_dam, sizeof(*tmp_dam));
        if (nhfp->fieldlevel)
            sfi_damage(nhfp, tmp_dam, "damage", "damage", 1);

        if (ghostly)
            tmp_dam->when += (g.monstermoves - g.omoves);
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
            tmp_dam->next = g.level.damagelist;
            g.level.damagelist = tmp_dam;
            tmp_dam = (struct damage *) alloc(sizeof(*tmp_dam));
        }
    }
    free((genericptr_t) tmp_dam);
}

/* restore one object */
static void
restobj(nhfp, otmp)
NHFILE *nhfp;
struct obj *otmp;
{
    int buflen;

    if (nhfp->structlevel)
        mread(nhfp->fd, (genericptr_t) otmp, sizeof(struct obj));
    if (nhfp->fieldlevel)
        sfi_obj(nhfp, otmp, "obj", "obj", 1);

    /* next object pointers are invalid; otmp->cobj needs to be left
       as is--being non-null is key to restoring container contents */
    otmp->nobj = otmp->nexthere = (struct obj *) 0;
    /* non-null oextra needs to be reconstructed */
    if (otmp->oextra) {
        otmp->oextra = newoextra();

        /* oname - object's name */
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) &buflen, sizeof(buflen));
        if (nhfp->fieldlevel)
            sfi_int(nhfp, &buflen, "obj", "oname_length", 1);

        if (buflen > 0) { /* includes terminating '\0' */
            new_oname(otmp, buflen);
            if (nhfp->structlevel)
                mread(nhfp->fd, (genericptr_t) ONAME(otmp), buflen);
            if (nhfp->fieldlevel)
                sfi_str(nhfp, ONAME(otmp), "obj", "oname", buflen);
        }

        /* omonst - corpse or statue might retain full monster details */
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) &buflen, sizeof(buflen));
        if (nhfp->fieldlevel)
            sfi_int(nhfp, &buflen, "obj", "omonst_length", 1);
        if (buflen > 0) {
            newomonst(otmp);
            /* this is actually a monst struct, so we
               can just defer to restmon() here */
            restmon(nhfp, OMONST(otmp));
        }

        /* omid - monster id number, connecting corpse to ghost */
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) &buflen, sizeof(buflen));
        if (nhfp->fieldlevel)
            sfi_int(nhfp, &buflen, "obj", "omid_length", 1);

        if (buflen > 0) {
            newomid(otmp);
            if (nhfp->structlevel)
                mread(nhfp->fd, (genericptr_t) OMID(otmp), buflen);
            if (nhfp->fieldlevel)
                sfi_unsigned(nhfp, OMID(otmp), "obj", "omid", 1);
        }

        /* olong - temporary gold */
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) &buflen, sizeof(buflen));
        if (nhfp->fieldlevel)
            sfi_int(nhfp, &buflen, "obj",  "olong_length", 1);
        if (buflen > 0) {
            newolong(otmp);
            if (nhfp->structlevel)
                mread(nhfp->fd, (genericptr_t) OLONG(otmp), buflen);
            if (nhfp->fieldlevel)
                sfi_long(nhfp, OLONG(otmp), "obj", "olong", 1);
        }

        /* omailcmd - feedback mechanism for scroll of mail */
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) &buflen, sizeof(buflen));
        if (nhfp->fieldlevel)
            sfi_int(nhfp, &buflen, "obj", "omailcmd_length", 1);
        if (buflen > 0) {
            char *omailcmd = (char *) alloc(buflen);

            if (nhfp->structlevel)
                mread(nhfp->fd, (genericptr_t) omailcmd, buflen);
            if (nhfp->fieldlevel)
                sfi_str(nhfp, omailcmd, "obj", "omailcmd", buflen);
            new_omailcmd(otmp, omailcmd);
            free((genericptr_t) omailcmd);
        }
    }
}

static struct obj *
restobjchn(nhfp, ghostly, frozen)
NHFILE *nhfp;
boolean ghostly, frozen;
{
    register struct obj *otmp, *otmp2 = 0;
    register struct obj *first = (struct obj *) 0;
    int buflen;

    while (1) {
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) &buflen, sizeof buflen);
        if (nhfp->fieldlevel)
            sfi_int(nhfp, &buflen, "obj",  "obj_length", 1);

        if (buflen == -1)
            break;

        otmp = newobj();
        restobj(nhfp, otmp);
        if (!first)
            first = otmp;
        else
            otmp2->nobj = otmp;

        if (ghostly) {
            unsigned nid = g.context.ident++;
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
            otmp->age = g.monstermoves - g.omoves + otmp->age;

        /* get contents of a container or statue */
        if (Has_contents(otmp)) {
            struct obj *otmp3;

            otmp->cobj = restobjchn(nhfp, ghostly, Is_IceBox(otmp));
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
            if (otmp->o_id == g.context.victual.o_id)
                g.context.victual.piece = otmp;
            if (otmp->o_id == g.context.tin.o_id)
                g.context.tin.tin = otmp;
            if (otmp->o_id == g.context.spbook.o_id)
                g.context.spbook.book = otmp;
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
static void
restmon(nhfp, mtmp)
NHFILE *nhfp;
struct monst *mtmp;
{
    int buflen;

    if (nhfp->structlevel)
        mread(nhfp->fd, (genericptr_t) mtmp, sizeof(struct monst));
    if (nhfp->fieldlevel)
        sfi_monst(nhfp, mtmp, "mon", "monst_length", 1);

    /* next monster pointer is invalid */
    mtmp->nmon = (struct monst *) 0;
    /* non-null mextra needs to be reconstructed */
    if (mtmp->mextra) {
        mtmp->mextra = newmextra();

        /* mname - monster's name */
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) &buflen, sizeof(buflen));
        if (nhfp->fieldlevel)
            sfi_int(nhfp, &buflen, "mon", "mname_length", 1);
        if (buflen > 0) { /* includes terminating '\0' */
            new_mname(mtmp, buflen);
            if (nhfp->structlevel)
                mread(nhfp->fd, (genericptr_t) MNAME(mtmp), buflen);
            if (nhfp->fieldlevel)
                sfi_str(nhfp, MNAME(mtmp), "mon", "mname", buflen);
        }
        /* egd - vault guard */
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) &buflen, sizeof(buflen));
        if (nhfp->fieldlevel)
            sfi_int(nhfp, &buflen, "mon", "egd_length", 1);

        if (buflen > 0) {
            newegd(mtmp);
            if (nhfp->structlevel)
                mread(nhfp->fd, (genericptr_t) EGD(mtmp), sizeof(struct egd));
            if (nhfp->fieldlevel)
                sfi_egd(nhfp, EGD(mtmp), "mon", "egd", 1);
        }
        /* epri - temple priest */
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) &buflen, sizeof(buflen));
        if (nhfp->fieldlevel)
            sfi_int(nhfp, &buflen, "mon", "epri_length", 1);
        if (buflen > 0) {
            newepri(mtmp);
            if (nhfp->structlevel)
                mread(nhfp->fd, (genericptr_t) EPRI(mtmp), sizeof(struct epri));
            if (nhfp->fieldlevel)
                sfi_epri(nhfp, EPRI(mtmp), "mon", "epri", 1);
        }
        /* eshk - shopkeeper */
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) &buflen, sizeof(buflen));
        if (nhfp->fieldlevel)
            sfi_int(nhfp, &buflen, "mon", "eshk_length", 1);
        if (buflen > 0) {
            neweshk(mtmp);
            if (nhfp->structlevel)
                mread(nhfp->fd, (genericptr_t) ESHK(mtmp), sizeof(struct eshk));
            if (nhfp->fieldlevel)
                sfi_eshk(nhfp, ESHK(mtmp), "mon", "eshk", 1);
        }
        /* emin - minion */
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) &buflen, sizeof(buflen));
        if (nhfp->fieldlevel)
            sfi_int(nhfp, &buflen, "mon", "emin_length", 1);
        if (buflen > 0) {
            newemin(mtmp);
            if (nhfp->structlevel)
                mread(nhfp->fd, (genericptr_t) EMIN(mtmp), sizeof(struct emin));
            if (nhfp->fieldlevel)
                sfi_emin(nhfp, EMIN(mtmp), "mon", "emin", 1);
        }
        /* edog - pet */
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) &buflen, sizeof(buflen));
        if (nhfp->fieldlevel)
            sfi_int(nhfp, &buflen, "mon", "edog_length", 1);
        if (buflen > 0) {
            newedog(mtmp);
            if (nhfp->structlevel)
                mread(nhfp->fd, (genericptr_t) EDOG(mtmp), sizeof(struct edog));
            if (nhfp->fieldlevel)
                sfi_edog(nhfp, EDOG(mtmp), "mon", "edog", 1);
        }
        /* mcorpsenm - obj->corpsenm for mimic posing as corpse or
           statue (inline int rather than pointer to something) */
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) &MCORPSENM(mtmp), sizeof MCORPSENM(mtmp));
        if (nhfp->fieldlevel)
            sfi_int(nhfp, &MCORPSENM(mtmp), "mon", "mcorpsenm", 1);
    } /* mextra */
}

static struct monst *
restmonchn(nhfp, ghostly)
NHFILE *nhfp;
boolean ghostly;
{
    register struct monst *mtmp, *mtmp2 = 0;
    register struct monst *first = (struct monst *) 0;
    int offset, buflen;

    while (1) {
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) &buflen, sizeof(buflen));
        if (nhfp->fieldlevel)
            sfi_int(nhfp, &buflen, "mon", "monst_length", 1);
        if (buflen == -1)
            break;

        mtmp = newmonst();
        restmon(nhfp, mtmp);
        if (!first)
            first = mtmp;
        else
            mtmp2->nmon = mtmp;

        if (ghostly) {
            unsigned nid = g.context.ident++;
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
            mtmp->minvent = restobjchn(nhfp, ghostly, FALSE);
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
            if (mtmp->m_id == g.context.polearm.m_id)
                g.context.polearm.hitmon = mtmp;
        }
        mtmp2 = mtmp;
    }
    if (first && mtmp2->nmon) {
        impossible("Restmonchn: error reading monchn.");
        mtmp2->nmon = 0;
    }
    return first;
}

static struct fruit *
loadfruitchn(nhfp)
NHFILE *nhfp;
{
    register struct fruit *flist, *fnext;

    flist = 0;
    for (;;) {
        fnext = newfruit();
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t)fnext, sizeof *fnext);
        if (nhfp->fieldlevel)
            sfi_fruit(nhfp, fnext, "fruit", "fruit", 1);
        if (fnext->fid != 0) {
            fnext->nextf = flist;
            flist = fnext;
        } else
            break;
    }
    dealloc_fruit(fnext);
    return flist;
}

static void
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

static void
ghostfruit(otmp)
register struct obj *otmp;
{
    register struct fruit *oldf;

    for (oldf = g.oldfruit; oldf; oldf = oldf->nextf)
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

static
boolean
restgamestate(nhfp, stuckid, steedid)
NHFILE *nhfp;
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

    if (nhfp->fieldlevel && nhfp->addinfo)
        sfi_addinfo(nhfp, "gamestate", "start", "", 0);

    if (nhfp->structlevel)
        mread(nhfp->fd, (genericptr_t) &uid, sizeof uid);
    if (nhfp->fieldlevel)
        sfi_ulong(nhfp, &uid, "gamestate", "uid", 1);
      
    if (SYSOPT_CHECK_SAVE_UID
        && uid != (unsigned long) getuid()) { /* strange ... */
        /* for wizard mode, issue a reminder; for others, treat it
           as an attempt to cheat and refuse to restore this file */
        pline("Saved game was not yours.");
        if (!wizard)
            return FALSE;
    }

    newgamecontext = g.context; /* copy statically init'd context */
    if (nhfp->structlevel)
        mread(nhfp->fd, (genericptr_t) &g.context, sizeof (struct context_info));
    if (nhfp->fieldlevel)
        sfi_context_info(nhfp, &g.context, "gamestate", "g.context", 1);
    g.context.warntype.species = (g.context.warntype.speciesidx >= LOW_PM)
                                  ? &mons[g.context.warntype.speciesidx]
                                  : (struct permonst *) 0;
    /* g.context.victual.piece, .tin.tin, .spellbook.book, and .polearm.hitmon
       are pointers which get set to Null during save and will be recovered
       via corresponding o_id or m_id while objs or mons are being restored */

    /* we want to be able to revert to command line/environment/config
       file option values instead of keeping old save file option values
       if partial restore fails and we resort to starting a new game */
    newgameflags = flags;
    if (nhfp->structlevel)
        mread(nhfp->fd, (genericptr_t) &flags, sizeof (struct flag));
    if (nhfp->fieldlevel)
        sfi_flag(nhfp, &flags, "gamestate", "flags", 1);

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
    if (nhfp->structlevel)
        mread(nhfp->fd, (genericptr_t) &sysflags, sizeof(struct sysflag));
    if (nhfp->fieldlevel)
        sfi_sysflag(nhfp, &sysflags, "gamestate", "sysflags", 1);
    mread(fd, (genericptr_t) &sysflags, sizeof(struct sysflag));
#endif

    role_init(); /* Reset the initial role, race, gender, and alignment */
#ifdef AMII_GRAPHICS
    amii_setpens(amii_numcolors); /* use colors from save file */
#endif
    if (nhfp->structlevel)
        mread(nhfp->fd, (genericptr_t) &u, sizeof(struct you));
    if (nhfp->fieldlevel)
        sfi_you(nhfp, &u, "gamestate", "you", 1);
    g.youmonst.cham = u.mcham;
        
    if (nhfp->structlevel)
        mread(nhfp->fd, (genericptr_t) timebuf, 14);
    if (nhfp->fieldlevel)
        sfi_str(nhfp, timebuf, "gamestate", "ubirthday", 14);
    timebuf[14] = '\0';
    ubirthday = time_from_yyyymmddhhmmss(timebuf);
    if (nhfp->structlevel)
        mread(nhfp->fd, &urealtime.realtime, sizeof urealtime.realtime);
    if (nhfp->fieldlevel)
        sfi_long(nhfp, &urealtime.realtime, "gamestate", "realtime", 1);

    if (nhfp->structlevel)
        mread(nhfp->fd, (genericptr_t) timebuf, 14);
    if (nhfp->fieldlevel)
        sfi_str(nhfp, timebuf, "gamestate", "start_timing", 14);
    timebuf[14] = '\0';
    urealtime.start_timing = time_from_yyyymmddhhmmss(timebuf);

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
        g.context = newgamecontext;
        g.youmonst = cg.zeromonst;
        return FALSE;
    }
    /* in case hangup save occurred in midst of level change */
    assign_level(&u.uz0, &u.uz);

    /* this stuff comes after potential aborted restore attempts */
    restore_killers(nhfp);
    restore_timers(nhfp, RANGE_GLOBAL, FALSE, 0L);
    restore_light_sources(nhfp);

    if (nhfp->fieldlevel && nhfp->addinfo)
        sfi_addinfo(nhfp, "objchain", "start", "invent", 0);
    g.invent = restobjchn(nhfp, FALSE, FALSE);

    /* restore dangling (not on floor or in inventory) ball and/or chain */
    bc_obj = restobjchn(nhfp, FALSE, FALSE);
    while (bc_obj) {
        struct obj *nobj = bc_obj->nobj;

        if (bc_obj->owornmask)
            setworn(bc_obj, bc_obj->owornmask);
        bc_obj->nobj = (struct obj *) 0;
        bc_obj = nobj;
    }
    g.migrating_objs = restobjchn(nhfp, FALSE, FALSE);
    g.migrating_mons = restmonchn(nhfp, FALSE);

    if (nhfp->structlevel) {
        mread(nhfp->fd, (genericptr_t) g.mvitals, sizeof g.mvitals);
    }
    if (nhfp->fieldlevel) {
        int i;

        for (i = 0; i < NUMMONS; ++i)
            sfi_mvitals(nhfp, &g.mvitals[i], "gamestate", "g.mvitals", 1);
    }

    /*
     * There are some things after this that can have unintended display
     * side-effects too early in the game.
     * Disable see_monsters() here, re-enable it at the top of moveloop()
     */
    g.defer_see_monsters = TRUE;

    /* this comes after inventory has been loaded */
    for (otmp = g.invent; otmp; otmp = otmp->nobj)
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
        g.unweapon = TRUE;

    restore_dungeon(nhfp);
    restlevchn(nhfp);
    if (nhfp->structlevel) {
        mread(nhfp->fd, (genericptr_t) &g.moves, sizeof g.moves);
        mread(nhfp->fd, (genericptr_t) &g.monstermoves, sizeof g.monstermoves);
        mread(nhfp->fd, (genericptr_t) &g.quest_status, sizeof (struct q_score));
        mread(nhfp->fd, (genericptr_t) g.spl_book, (MAXSPELL + 1) * sizeof (struct spell));
    }
    if (nhfp->fieldlevel) {
        int i;
        struct spell *sptmp;

        sfi_long(nhfp, &g.moves, "gamestate", "g.moves", 1);
        sfi_long(nhfp, &g.monstermoves, "gamestate", "g.monstermoves", 1);
        sfi_q_score(nhfp, &g.quest_status, "gamestate", "g.quest_status", 1);
        sptmp = g.spl_book;
        for (i = 0; i < (MAXSPELL + 1); ++i)
            sfi_spell(nhfp, sptmp++, "gamestate", "g.spl_book", 1);
    }        
    restore_artifacts(nhfp);
    restore_oracles(nhfp);
    if (u.ustuck) {
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) stuckid, sizeof *stuckid);
        if (nhfp->fieldlevel)
            sfi_unsigned(nhfp, stuckid, "gamestate", "ustuck_id", 1);
    }
    if (u.usteed) {
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) steedid, sizeof *steedid);
        if (nhfp->fieldlevel)
            sfi_unsigned(nhfp, steedid, "gamestate", "usteed_id", 1);
    }
    if (nhfp->structlevel) {
        mread(nhfp->fd, (genericptr_t) g.pl_character, sizeof g.pl_character);
        mread(nhfp->fd, (genericptr_t) g.pl_fruit, sizeof g.pl_fruit);
    }
    if (nhfp->fieldlevel) {
        sfi_char(nhfp, g.pl_character, "gamestate", "g.pl_character",
                 sizeof g.pl_character);
        sfi_char(nhfp, g.pl_fruit, "gamestate", "g.pl_fruit",
                 sizeof g.pl_fruit);
    }
    freefruitchn(g.ffruit); /* clean up fruit(s) made by initoptions() */
    g.ffruit = loadfruitchn(nhfp);
  
    restnames(nhfp);
    restore_waterlevel(nhfp);
    restore_msghistory(nhfp);
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
static void
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
static int
restlevelfile(nhfp, ltmp)
NHFILE *nhfp; /* used in MFLOPPY only */
xchar ltmp;
{
    char whynot[BUFSZ];
#ifdef MFLOPPY
    int savemode;
#endif
    NHFILE *nnhfp = (NHFILE *) 0;

#ifndef MFLOPPY
    nhUse(nhfp);
#endif
    nnhfp = create_levelfile(ltmp, whynot);
    if (!nnhfp) {
        /* BUG: should suppress any attempt to write a panic
           save file if file creation is now failing... */
        panic("restlevelfile: %s", whynot);
    }
#ifdef MFLOPPY
    savemode = nnhfp->mode;
    nnhfp->mode = COUNTING;
    if (!savelev(nnhfp, ltmp)) {
        /* The savelev can't proceed because the size required
         * is greater than the available disk space.
         */
        pline("Not enough space on `%s' to restore your game.", levels);

        /* Remove levels and bones that may have been created.
         */
        close_nhfile(nnhfp);
#ifdef AMIGA
        clearlocks();
#else /* !AMIGA */
        eraseall(levels, g.alllevels);
        eraseall(levels, g.allbones);

        /* Perhaps the person would like to play without a
         * RAMdisk.
         */
        if (g.ramdisk) {
            /* PlaywoRAMdisk may not return, but if it does
             * it is certain that g.ramdisk will be 0.
             */
            playwoRAMdisk();
            /* Rewind save file and try again */
            rewind_nhfile(nhfp);
            (void) validate(nhfp, (char *) 0); /* skip version etc */
            return dorecover(nhfp);            /* 0 or 1 */
        }
#endif /* ?AMIGA */
        pline("Be seeing you...");
        nh_terminate(EXIT_SUCCESS);
    }
    nnhfp->mode = savemode;
#endif /* MFLOPPY */
    bufon(nnhfp->fd);
    nnhfp->mode = WRITING | FREEING;
    savelev(nnhfp, ltmp);
    close_nhfile(nnhfp);
    return 2;
}

int
dorecover(nhfp)
NHFILE *nhfp;
{
    unsigned int stuckid = 0, steedid = 0; /* not a register */
    xchar ltmp;
    int rtmp;
    struct obj *otmp;

    g.restoring = TRUE;
    get_plname_from_file(nhfp, g.plname);
    getlev(nhfp, 0, (xchar) 0, FALSE);
    if (!restgamestate(nhfp, &stuckid, &steedid)) {
        NHFILE tnhfp;

        display_nhwindow(WIN_MESSAGE, TRUE);
        zero_nhfile(&tnhfp);
        tnhfp.mode = FREEING;
        tnhfp.fd = -1;
        savelev(&tnhfp, 0); /* discard current level */
        /* no need tfor close_nhfile(&tnhfp), which
           is not really affiliated with an open file */
        close_nhfile(nhfp);
        (void) delete_savefile();
        g.restoring = FALSE;
        return 0;
    }
    restlevelstate(stuckid, steedid);
#ifdef INSURANCE
    savestateinlock();
#endif
    rtmp = restlevelfile(nhfp, ledger_no(&u.uz));
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
        g.dungeons[u.uz.dnum].dname,
        flags.debug ? " while in debug mode"
                    : flags.explore ? " while in explore mode" : "");
    curs(WIN_MAP, 1, 1);
    dotcnt = 0;
    dotrow = 2;
    if (!WINDOWPORT("X11"))
        putstr(WIN_MAP, 0, "Restoring:");
#endif
    restoreinfo.mread_flags = 1; /* return despite error */
    while (1) {
        if (nhfp->structlevel) {
            mread(nhfp->fd, (genericptr_t) &ltmp, sizeof ltmp);
            if (restoreinfo.mread_flags == -1)
                break;
        }
        if (nhfp->fieldlevel) {
            sfi_xchar(nhfp, &ltmp, "gamestate", "level_number", 1);
            if (nhfp->eof)
                break;
	}
        getlev(nhfp, 0, ltmp, FALSE);
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
        rtmp = restlevelfile(nhfp, ltmp);
        if (rtmp < 2)
            return rtmp; /* dorecover called recursively */
    }
    restoreinfo.mread_flags = 0;
    if (nhfp->fieldlevel && nhfp->addinfo)
        sfi_addinfo(nhfp, "NetHack", "end", "savefile", 0);

    rewind_nhfile(nhfp);        /* return to beginning of file */
    (void) validate(nhfp, (char *) 0);
    get_plname_from_file(nhfp, g.plname);

    getlev(nhfp, 0, (xchar) 0, FALSE);
    close_nhfile(nhfp);
    restlevelstate(stuckid, steedid);
    g.program_state.something_worth_saving = 1; /* useful data now exists */

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
    max_rank_sz(); /* to recompute g.mrank_sz (botl.c) */
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

    /* Set up the vision internals, after levl[] data is loaded
       but before docrt(). */
    reglyph_darkroom();
    vision_reset();
    g.vision_full_recalc = 1; /* recompute vision (not saved) */

    run_timers(); /* expire all timers that have gone off while away */
    docrt();
    g.restoring = FALSE;
    clear_nhwindow(WIN_MESSAGE);

    /* Success! */
    welcome(FALSE);
    check_special_room(FALSE);
    return 1;
}

void
restcemetery(nhfp, cemeteryaddr)
NHFILE *nhfp;
struct cemetery **cemeteryaddr;
{
    struct cemetery *bonesinfo, **bonesaddr;
    int cflag;

    if (nhfp->structlevel)
        mread(nhfp->fd, (genericptr_t) &cflag, sizeof cflag);
    if (nhfp->fieldlevel)
        sfi_int(nhfp, &cflag, "cemetery", "cemetery_flag", 1);
    if (cflag == 0) {
        bonesaddr = cemeteryaddr;
        do {
            bonesinfo = (struct cemetery *) alloc(sizeof *bonesinfo);
            if (nhfp->structlevel)
                mread(nhfp->fd, (genericptr_t) bonesinfo, sizeof *bonesinfo);
            if (nhfp->fieldlevel)
                sfi_cemetery(nhfp, bonesinfo, "bones", "bonesinfo", 1);
            *bonesaddr = bonesinfo;
            bonesaddr = &(*bonesaddr)->next;
        } while (*bonesaddr);
    } else {
        *cemeteryaddr = 0;
    }
}

/*ARGSUSED*/
static void
rest_levl(nhfp, rlecomp)
NHFILE *nhfp;
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
                    if (nhfp->structlevel) {
                        mread(nhfp->fd, (genericptr_t) &len, sizeof(uchar));
                        mread(nhfp->fd, (genericptr_t) &r, sizeof(struct rm));
                    }
                    if (nhfp->fieldlevel) {
                        sfi_uchar(nhfp, &len, "room", "levl", 1);
                        sfi_rm(nhfp, &r, "room", "rm", 1);
                    }
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
    if (nhfp->structlevel) {
        mread(nhfp->fd, (genericptr_t) levl, sizeof levl);
    }
    if (nhfp->fieldlevel) {
        int c, r;

        for (c = 0; c < COLNO; ++c)
            for (r = 0; r < ROWNO; ++r)    
                sfi_rm(nhfp, &levl[c][r], "room", "levl", 1);
    }
}

void
trickery(reason)
char *reason;
{
    pline("Strange, this map is not as I remember it.");
    pline("Somebody is trying some trickery here...");
    pline("This game is void.");
    Strcpy(g.killer.name, reason ? reason : "");
    done(TRICKED);
}

void
getlev(nhfp, pid, lev, ghostly)
NHFILE *nhfp;
int pid;
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
    if (nhfp->structlevel)
        setmode(nhfp->fd, O_BINARY);
#endif
    /* Load the old fruit info.  We have to do it first, so the
     * information is available when restoring the objects.
     */
    if (ghostly)
        g.oldfruit = loadfruitchn(nhfp);

    /* First some sanity checks */
    if (nhfp->structlevel)
        mread(nhfp->fd, (genericptr_t) &hpid, sizeof(hpid));
    if (nhfp->fieldlevel)
        sfi_int(nhfp, &hpid, "gamestate", "g.hackpid", 1);

/* CHECK:  This may prevent restoration */
#ifdef TOS
    if (nhfp->structlevel)
        mread(nhfp->fd, (genericptr_t) &tlev, sizeof(tlev));
    if (nhfp->fieldlevel)
        sfi_short(nhfp, &tlev, "gamestate", "tlev", 1);
    dlvl = tlev & 0x00ff;
#else
    if (nhfp->structlevel)
        mread(nhfp->fd, (genericptr_t) &dlvl, sizeof(dlvl));
    if (nhfp->fieldlevel)
        sfi_xchar(nhfp, &dlvl, "gamestate", "dlvl", 1);
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
    restcemetery(nhfp, &g.level.bonesinfo);
    rest_levl(nhfp,
              (boolean) ((sfrestinfo.sfi1 & SFI1_RLECOMP) == SFI1_RLECOMP));
    if (nhfp->structlevel) {
        mread(nhfp->fd, (genericptr_t) g.lastseentyp, sizeof(g.lastseentyp));
        mread(nhfp->fd, (genericptr_t) &g.omoves, sizeof(g.omoves));
    }
    if (nhfp->fieldlevel) {
        int c, r;

        for (c = 0; c < COLNO; ++c)
            for (r = 0; r < ROWNO; ++r)
                sfi_schar(nhfp, &g.lastseentyp[c][r],
                          "lev", "g.lastseentyp", 1);

        sfi_long(nhfp, &g.omoves, "lev", "timestmp", 1);
    }
    elapsed = g.monstermoves - g.omoves;

    if (nhfp->structlevel) {
        mread(nhfp->fd, (genericptr_t)&g.upstair, sizeof(stairway));
        mread(nhfp->fd, (genericptr_t)&g.dnstair, sizeof(stairway));
        mread(nhfp->fd, (genericptr_t)&g.upladder, sizeof(stairway));
        mread(nhfp->fd, (genericptr_t)&g.dnladder, sizeof(stairway));
        mread(nhfp->fd, (genericptr_t)&g.sstairs, sizeof(stairway));
        mread(nhfp->fd, (genericptr_t)&g.updest, sizeof(dest_area));
        mread(nhfp->fd, (genericptr_t)&g.dndest, sizeof(dest_area));
        mread(nhfp->fd, (genericptr_t)&g.level.flags, sizeof(g.level.flags));
        mread(nhfp->fd, (genericptr_t)g.doors, sizeof(g.doors));
    }
    if (nhfp->fieldlevel) {
        int i;

        sfi_stairway(nhfp, &g.upstair, "lev", "g.upstair", 1);
        sfi_stairway(nhfp, &g.dnstair, "lev", "g.dnstair", 1);
        sfi_stairway(nhfp, &g.upladder, "lev", "g.upladder", 1);
        sfi_stairway(nhfp, &g.dnladder, "lev", "g.dnladder", 1);
        sfi_stairway(nhfp, &g.sstairs, "lev", "g.sstairs", 1);
        sfi_dest_area(nhfp, &g.updest, "lev", "g.updest", 1);
        sfi_dest_area(nhfp, &g.dndest, "lev", "g.dndest", 1);
        sfi_levelflags(nhfp, &g.level.flags, "lev", "g.level.flags", 1);
        for (i = 0; i < DOORMAX; ++i)
            sfi_nhcoord(nhfp, &g.doors[i], "lev", "g.doors", 1);
    }
    rest_rooms(nhfp); /* No joke :-) */
    if (g.nroom)
        g.doorindex = g.rooms[g.nroom - 1].fdoor + g.rooms[g.nroom - 1].doorct;
    else
        g.doorindex = 0;
  
    restore_timers(nhfp, RANGE_LEVEL, ghostly, elapsed);
    restore_light_sources(nhfp);
    fmon = restmonchn(nhfp, ghostly);

    /* rest_worm(fd); */    /* restore worm information */
    rest_worm(nhfp);    /* restore worm information */

    g.ftrap = 0;
    for (;;) {
        trap = newtrap();
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t)trap, sizeof(struct trap));
        if (nhfp->fieldlevel)
            sfi_trap(nhfp, trap, "trap", "trap", 1);
        if (trap->tx != 0) {
            trap->ntrap = g.ftrap;
            g.ftrap = trap;
        } else
            break;
    }
    dealloc_trap(trap);

    fobj = restobjchn(nhfp, ghostly, FALSE);
    find_lev_obj();
    /* restobjchn()'s `frozen' argument probably ought to be a callback
       routine so that we can check for objects being buried under ice */
    g.level.buriedobjlist = restobjchn(nhfp, ghostly, FALSE);
    g.billobjs = restobjchn(nhfp, ghostly, FALSE);
    rest_engravings(nhfp);

    /* reset level.monsters for new level */
    for (x = 0; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++)
            g.level.monsters[x][y] = (struct monst *) 0;
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
    restdamage(nhfp, ghostly);
    rest_regions(nhfp, ghostly);

    if (ghostly) {
        /* Now get rid of all the temp fruits... */
        freefruitchn(g.oldfruit), g.oldfruit = 0;

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
            case BR_NO_END2: /* OK to assign to g.sstairs if it's not used */
                assign_level(&g.sstairs.tolev, &ltmp);
                break;
            case BR_PORTAL: /* max of 1 portal per level */
                for (trap = g.ftrap; trap; trap = trap->ntrap)
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
            for (trap = g.ftrap; trap; trap = ttmp) {
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
get_plname_from_file(nhfp, plbuf)
NHFILE *nhfp;
char *plbuf;
{
    int pltmpsiz = 0;

    if (nhfp->structlevel) {
        (void) read(nhfp->fd, (genericptr_t) &pltmpsiz, sizeof(pltmpsiz));
        (void) read(nhfp->fd, (genericptr_t) plbuf, pltmpsiz);
    }
    if (nhfp->fieldlevel) {
            sfi_int(nhfp, &pltmpsiz, "plname", "plname_size", 1);
            sfi_str(nhfp, plbuf, "plname", "g.plname", pltmpsiz);
    }
    return;
}

static void
restore_msghistory(nhfp)
NHFILE *nhfp;
{
    int msgsize, msgcount = 0;
    char msg[BUFSZ];

    while (1) {
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) &msgsize, sizeof(msgsize));
        if (nhfp->fieldlevel)
            sfi_int(nhfp, &msgsize, "msghistory", "msghistory_length", 1);
        if (msgsize == -1)
            break;
        if (msgsize > (BUFSZ - 1))
            panic("restore_msghistory: msg too big (%d)", msgsize);
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) msg, msgsize);
        if (nhfp->fieldlevel)
            sfi_str(nhfp, msg, "msghistory", "msg", msgsize);
        msg[msgsize] = '\0';
        putmsghistory(msg, TRUE);
        ++msgcount;
    }
    if (msgcount)
        putmsghistory((char *) 0, TRUE);
    debugpline1("Read %d messages from savefile.", msgcount);
}

/* Clear all structures for object and monster ID mapping. */
static void
clear_id_mapping()
{
    struct bucket *curr;

    while ((curr = g.id_map) != 0) {
        g.id_map = curr->next;
        free((genericptr_t) curr);
    }
    g.n_ids_mapped = 0;
}

/* Add a mapping to the ID map. */
static void
add_id_mapping(gid, nid)
unsigned gid, nid;
{
    int idx;

    idx = g.n_ids_mapped % N_PER_BUCKET;
    /* idx is zero on first time through, as well as when a new bucket is */
    /* needed */
    if (idx == 0) {
        struct bucket *gnu = (struct bucket *) alloc(sizeof(struct bucket));
        gnu->next = g.id_map;
        g.id_map = gnu;
    }

    g.id_map->map[idx].gid = gid;
    g.id_map->map[idx].nid = nid;
    g.n_ids_mapped++;
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

    if (g.n_ids_mapped)
        for (curr = g.id_map; curr; curr = curr->next) {
            /* first bucket might not be totally full */
            if (curr == g.id_map) {
                i = g.n_ids_mapped % N_PER_BUCKET;
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

static void
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
   returns 1: use g.plname[], 0: new game, -1: quit */
int
restore_menu(bannerwin)
winid bannerwin; /* if not WIN_ERR, clear window and show copyright in menu */
{
    winid tmpwin;
    anything any;
    char **saved;
    menu_item *chosen_game = (menu_item *) 0;
    int k, clet, ch = 0; /* ch: 0 => new game */

    *g.plname = '\0';
    saved = get_saved_games(); /* array of character names */
    if (saved && *saved) {
        tmpwin = create_nhwindow(NHW_MENU);
        start_menu(tmpwin);
        any = cg.zeroany; /* no selection */
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
                Strcpy(g.plname, saved[ch - 1]);
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

int
validate(nhfp, name)
NHFILE *nhfp;
const char *name;
{
    int rlen = 0;
    struct savefile_info sfi;
    unsigned long utdflags = 0L;
    boolean verbose = name ? TRUE : FALSE, reslt = FALSE;

    if (nhfp->structlevel)
        utdflags |= UTD_CHECKSIZES;
    if (nhfp->fieldlevel)
        utdflags |= UTD_CHECKFIELDCOUNTS |
                    UTD_SKIP_SANITY1 | UTD_SKIP_SAVEFILEINFO;
    if (!(reslt = uptodate(nhfp, name, utdflags))) return 1;

    if ((nhfp->mode & WRITING) == 0) {
	if (nhfp->structlevel)
            rlen = read(nhfp->fd, (genericptr_t) &sfi, sizeof sfi);
        if (nhfp->fieldlevel)
	    sfi_savefile_info(nhfp, &sfi, "savefileinfo", "savefile_info", 1);
    } else {
        if (nhfp->structlevel)
            rlen = read(nhfp->fd, (genericptr_t) &sfi, sizeof sfi);
        if (nhfp->fieldlevel)
            sfi_savefile_info(nhfp, &sfi, "savefileinfo", "savefile_info", 1);
        minit();		/* ZEROCOMP */
        if (rlen == 0) {
	    if (verbose) {
	        pline("File \"%s\" is empty during save file feature check?", name);
	        wait_synch();
	    }
	    return -1;
        }
    }
    return 0;
}

/*restore.c*/
