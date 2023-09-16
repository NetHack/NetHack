/* NetHack 3.7	dog.c	$NHDT-Date: 1693427872 2023/08/30 20:37:52 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.146 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

static int pet_type(void);
static void set_mon_lastmove(struct monst *);
static int mon_leave(struct monst *);
static boolean keep_mon_accessible(struct monst *);

enum arrival {
    Before_you =  0, /* monsters kept on migrating_mons for accessibility;
                      * they haven't actually left their level */
    With_you   =  1, /* pets and level followers */
    After_you  =  2, /* regular migrating monsters */
    Wiz_arrive = -1  /* resurrect(wizard.c) */
};

void
newedog(struct monst *mtmp)
{
    if (!mtmp->mextra)
        mtmp->mextra = newmextra();
    if (!EDOG(mtmp)) {
        EDOG(mtmp) = (struct edog *) alloc(sizeof(struct edog));
        (void) memset((genericptr_t) EDOG(mtmp), 0, sizeof(struct edog));
    }
}

void
free_edog(struct monst *mtmp)
{
    if (mtmp->mextra && EDOG(mtmp)) {
        free((genericptr_t) EDOG(mtmp));
        EDOG(mtmp) = (struct edog *) 0;
    }
    mtmp->mtame = 0;
}

void
initedog(struct monst *mtmp)
{
    mtmp->mtame = is_domestic(mtmp->data) ? 10 : 5;
    mtmp->mpeaceful = 1;
    mtmp->mavenge = 0;
    set_malign(mtmp); /* recalc alignment now that it's tamed */
    mtmp->mleashed = 0;
    mtmp->meating = 0;
    EDOG(mtmp)->droptime = 0;
    EDOG(mtmp)->dropdist = 10000;
    EDOG(mtmp)->apport = ACURR(A_CHA);
    EDOG(mtmp)->whistletime = 0;
    EDOG(mtmp)->hungrytime = 1000 + gm.moves;
    EDOG(mtmp)->ogoal.x = -1; /* force error if used before set */
    EDOG(mtmp)->ogoal.y = -1;
    EDOG(mtmp)->abuse = 0;
    EDOG(mtmp)->revivals = 0;
    EDOG(mtmp)->mhpmax_penalty = 0;
    EDOG(mtmp)->killed_by_u = 0;
    u.uconduct.pets++;
}

static int
pet_type(void)
{
    if (gu.urole.petnum != NON_PM)
        return  gu.urole.petnum;
    else if (gp.preferred_pet == 'c')
        return  PM_KITTEN;
    else if (gp.preferred_pet == 'd')
        return  PM_LITTLE_DOG;
    else
        return  rn2(2) ? PM_KITTEN : PM_LITTLE_DOG;
}

struct monst *
make_familiar(struct obj *otmp, coordxy x, coordxy y, boolean quietly)
{
    struct permonst *pm;
    struct monst *mtmp = 0;
    int chance, trycnt = 100;

    do {
        mmflags_nht mmflags;
        int cgend, mndx;

        if (otmp) { /* figurine; otherwise spell */
            mndx = otmp->corpsenm;
            pm = &mons[mndx];
            /* activating a figurine provides one way to exceed the
               maximum number of the target critter created--unless
               it has a special limit (erinys, Nazgul) */
            if ((gm.mvitals[mndx].mvflags & G_EXTINCT)
                && mbirth_limit(mndx) != MAXMONNO) {
                if (!quietly)
                    /* have just been given "You <do something with>
                       the figurine and it transforms." message */
                    pline("... into a pile of dust.");
                break; /* mtmp is null */
            }
        } else if (!rn2(3)) {
            pm = &mons[pet_type()];
        } else {
            pm = rndmonst();
            if (!pm) {
                if (!quietly)
                    There("seems to be nothing available for a familiar.");
                break;
            }
        }

        mmflags = MM_EDOG | MM_IGNOREWATER | NO_MINVENT | MM_NOMSG;
        cgend = otmp ? (otmp->spe & CORPSTAT_GENDER) : 0;
        mmflags |= ((cgend == CORPSTAT_FEMALE) ? MM_FEMALE
                    : (cgend == CORPSTAT_MALE) ? MM_MALE : 0L);

        mtmp = makemon(pm, x, y, mmflags);
        if (otmp) { /* figurine */
            if (!mtmp) {
                /* monster has been genocided or target spot is occupied */
                if (!quietly)
                    pline_The(
                           "figurine writhes and then shatters into pieces!");
                break;
            } else if (mtmp->isminion) {
                /* Fixup for figurine of an Angel:  makemon() is willing to
                   create a random Angel as either an ordinary monster or as
                   a minion of random allegiance.  We don't want the latter
                   here in case it successfully becomes a pet. */
                mtmp->isminion = 0;
                free_emin(mtmp);
                /* [This could and possibly should be redone as a new
                   MM_flag passed to makemon() to suppress making a minion
                   so that no post-creation fixup would be needed.] */
            }
        }
    } while (!mtmp && --trycnt > 0);

    if (!mtmp)
        return (struct monst *) 0;

    if (is_pool(mtmp->mx, mtmp->my) && minliquid(mtmp))
        return (struct monst *) 0;

    initedog(mtmp);
    mtmp->msleeping = 0;
    if (otmp) { /* figurine; resulting monster might not become a pet */
        chance = rn2(10); /* 0==tame, 1==peaceful, 2==hostile */
        if (chance > 2)
            chance = otmp->blessed ? 0 : !otmp->cursed ? 1 : 2;
        /* 0,1,2:  b=80%,10,10; nc=10%,80,10; c=10%,10,80 */
        if (chance > 0) {
            mtmp->mtame = 0;   /* not tame after all */
            u.uconduct.pets--; /* doesn't count as creating a pet */
            if (chance == 2) { /* hostile (cursed figurine) */
                if (!quietly)
                    You("get a bad feeling about this.");
                mtmp->mpeaceful = 0;
                set_malign(mtmp);
            }
        }
        /* if figurine has been named, give same name to the monster */
        if (has_oname(otmp))
            mtmp = christen_monst(mtmp, ONAME(otmp));
    }
    set_malign(mtmp); /* more alignment changes */
    newsym(mtmp->mx, mtmp->my);

    /* must wield weapon immediately since pets will otherwise drop it */
    if (mtmp->mtame && attacktype(mtmp->data, AT_WEAP)) {
        mtmp->weapon_check = NEED_HTH_WEAPON;
        (void) mon_wield_item(mtmp);
    }
    return mtmp;
}

struct monst *
makedog(void)
{
    register struct monst *mtmp;
    register struct obj *otmp;
    const char *petname;
    int pettype;

    if (gp.preferred_pet == 'n')
        return ((struct monst *) 0);

    pettype = pet_type();
    if (pettype == PM_LITTLE_DOG)
        petname = gd.dogname;
    else if (pettype == PM_PONY)
        petname = gh.horsename;
    else
        petname = gc.catname;

    /* default pet names */
    if (!*petname && pettype == PM_LITTLE_DOG) {
        /* All of these names were for dogs. */
        if (Role_if(PM_CAVE_DWELLER))
            petname = "Slasher"; /* The Warrior */
        if (Role_if(PM_SAMURAI))
            petname = "Hachi"; /* Shibuya Station */
        if (Role_if(PM_BARBARIAN))
            petname = "Idefix"; /* Obelix */
        if (Role_if(PM_RANGER))
            petname = "Sirius"; /* Orion's dog */
    }

    mtmp = makemon(&mons[pettype], u.ux, u.uy, MM_EDOG);

    if (!mtmp)
        return ((struct monst *) 0); /* pets were genocided */

    gc.context.startingpet_mid = mtmp->m_id;
    /* Horses already wear a saddle */
    if (pettype == PM_PONY && !!(otmp = mksobj(SADDLE, TRUE, FALSE))) {
        otmp->dknown = otmp->bknown = otmp->rknown = 1;
        put_saddle_on_mon(otmp, mtmp);
    }

    if (!gp.petname_used++ && *petname)
        mtmp = christen_monst(mtmp, petname);

    initedog(mtmp);
    return  mtmp;
}

static void
set_mon_lastmove(struct monst *mtmp)
{
    mtmp->mlstmv = gm.moves;
}

/* record `last move time' for all monsters prior to level save so that
   mon_arrive() can catch up for lost time when they're restored later */
void
update_mlstmv(void)
{
    iter_mons(set_mon_lastmove);
}

/* note: always reset when used so doesn't need to be part of struct 'g' */
static struct monst *failed_arrivals = 0;

void
losedogs(void)
{
    register struct monst *mtmp, **mprev;
    int dismissKops = 0, xyloc;

    failed_arrivals = 0;
    /*
     * First, scan gm.migrating_mons for shopkeepers who want to dismiss Kops,
     * and scan gm.mydogs for shopkeepers who want to retain kops.
     * Second, dismiss kops if warranted, making more room for arrival.
     * Third, replace monsters who went onto migrating_mons in order to
     * be accessible from other levels but didn't actually leave the level.
     * Fourth, place monsters accompanying the hero.
     * Last, place migrating monsters coming to this level.
     *
     * Hero might eventually be displaced (due to the third step, but
     * occurring later), which is the main reason to do the second step
     * sooner (in turn necessitating the first step, rather than combining
     * the list scans with monster placement).
     */

    /* check for returning shk(s) */
    for (mtmp = gm.migrating_mons; mtmp; mtmp = mtmp->nmon) {
        if (mtmp->mux != u.uz.dnum || mtmp->muy != u.uz.dlevel)
            continue;
        if (mtmp->isshk) {
            if (ESHK(mtmp)->dismiss_kops) {
                if (dismissKops == 0)
                    dismissKops = 1;
                ESHK(mtmp)->dismiss_kops = FALSE; /* reset */
            } else if (!mtmp->mpeaceful) {
                /* an unpacified shk is returning; don't dismiss kops
                   even if another pacified one is willing to do so */
                dismissKops = -1;
                /* [keep looping; later monsters might need ESHK reset] */
            }
        }
    }
    /* make the same check for gm.mydogs */
    for (mtmp = gm.mydogs; mtmp && dismissKops >= 0; mtmp = mtmp->nmon) {
        if (mtmp->isshk) {
            /* hostile shk might accompany hero [ESHK(mtmp)->dismiss_kops
               can't be set here; it's only used for gm.migrating_mons] */
            if (!mtmp->mpeaceful)
                dismissKops = -1;
        }
    }

    /* when a hostile shopkeeper chases hero to another level
       and then gets paid off there, get rid of summoned kops
       here now that he has returned to his shop level */
    if (dismissKops > 0)
        make_happy_shoppers(TRUE);

    /* put monsters who went onto migrating_mons in order to be accessible
       when other levels are active back to their positions on this level;
       they're handled before mydogs so that monsters accompanying the
       hero can't steal the spot that belongs to them; these migraters
       should always be able to arrive because they were present on the
       level at the time the hero left [if they can't arrive for some
       reason, mon_arrive() will put them on the 'failed_arrivals' list] */
    for (mprev = &gm.migrating_mons; (mtmp = *mprev) != 0; ) {
        xyloc = mtmp->mtrack[0].x; /* (for legibility) */
        if (mtmp->mux == u.uz.dnum && mtmp->muy == u.uz.dlevel
            && xyloc == MIGR_EXACT_XY) {
            /* remove mtmp from migrating_mons */
            *mprev = mtmp->nmon;
            mon_arrive(mtmp, Before_you);
        } else {
            mprev = &mtmp->nmon;
        }
    }

    /* place pets and/or any other monsters who accompany hero;
       any that fail to arrive (level may be full) will be moved
       first to failed_arrivals, then to migrating_mons scheduled
       to arrive back on this level if hero leaves and returns */
    while ((mtmp = gm.mydogs) != 0) {
        gm.mydogs = mtmp->nmon;
        mon_arrive(mtmp, With_you);
    }

    /* time for migrating monsters to arrive; monsters who belong on
       this level but fail to arrive get put on the failed_arrivals list
       temporarily [by mon_arrive()], then back onto the migrating_mons
       list below */
    for (mprev = &gm.migrating_mons; (mtmp = *mprev) != 0; ) {
        xyloc = mtmp->mtrack[0].x;
        if (mtmp->mux == u.uz.dnum && mtmp->muy == u.uz.dlevel
            && xyloc != MIGR_EXACT_XY) {
            /* remove mtmp from migrating_mons */
            *mprev = mtmp->nmon;
            /* note: if there's no room, it ends up on failed_arrivals list */
            mon_arrive(mtmp, After_you);
        } else {
            mprev = &mtmp->nmon;
        }
    }

    /* put any monsters who couldn't arrive back on migrating_mons,
       clearing out the temporary 'failed_arrivals' list in the process */
    while ((mtmp = failed_arrivals) != 0) {
        failed_arrivals = mtmp->nmon;
        /* mon_arrive() put mtmp onto fmon, but if there wasn't room to
           arrive, relmon() was used to take it off again; put it back now
           because m_into_limbo() expects it to be there */
        mtmp->nmon = fmon;
        fmon = mtmp;
        /* set this monster to migrate back to this level if hero leaves
           and then returns */
        m_into_limbo(mtmp);
    }
}

/* called from resurrect() in addition to losedogs() */
void
mon_arrive(struct monst *mtmp, int when)
{
    struct trap *t;
    coordxy xlocale, ylocale, xyloc, xyflags;
    xint16 wander;
    int num_segs;
    boolean failed_to_place = FALSE;
    stairway *stway;
    d_level fromdlev;

    mtmp->nmon = fmon;
    fmon = mtmp;
    if (mtmp->isshk)
        set_residency(mtmp, FALSE);

    num_segs = mtmp->wormno;
    /* baby long worms have no tail so don't use is_longworm() */
    if (mtmp->data == &mons[PM_LONG_WORM]) {
        mtmp->wormno = get_wormno();
        if (mtmp->wormno)
            initworm(mtmp, num_segs);
    } else
        mtmp->wormno = 0;

    /* some monsters might need to do something special upon arrival
       _after_ the current level has been fully set up; see dochug() */
    mtmp->mstrategy |= STRAT_ARRIVE;
    mtmp->mstate &= ~(MON_MIGRATING | MON_LIMBO);

    /* make sure mnexto(rloc_to(set_apparxy())) doesn't use stale data */
    mtmp->mux = u.ux, mtmp->muy = u.uy;
    xyloc = mtmp->mtrack[0].x;
    xyflags = mtmp->mtrack[0].y;
    xlocale = mtmp->mtrack[1].x;
    ylocale = mtmp->mtrack[1].y;
    fromdlev.dnum = mtmp->mtrack[2].x;
    fromdlev.dlevel = mtmp->mtrack[2].y;
    mon_track_clear(mtmp);

    if (mtmp == u.usteed)
        return; /* don't place steed on the map */
    if (when == With_you) {
        /* When a monster accompanies you, sometimes it will arrive
           at your intended destination and you'll end up next to
           that spot.  This code doesn't control the final outcome;
           goto_level(do.c) decides who ends up at your target spot
           when there is a monster there too. */
        if (!MON_AT(u.ux, u.uy)
            && !rn2(mtmp->mtame ? 10 : mtmp->mpeaceful ? 5 : 2))
            rloc_to(mtmp, u.ux, u.uy);
        else
            mnexto(mtmp, RLOC_NOMSG);
        return;
    } else if (when == Wiz_arrive) {
        /* resurrect() is bringing existing wizard to harass the hero */
        xyloc = MIGR_WITH_HERO;
    }
    /*
     * The monster arrived on this level independently of the player.
     * Its coordinate fields were overloaded for use as flags that
     * specify its final destination.
     */

    if (mtmp->mlstmv < gm.moves - 1L) {
        /* heal monster for time spent in limbo */
        long nmv = gm.moves - 1L - mtmp->mlstmv;

        mon_catchup_elapsed_time(mtmp, nmv);

        /* let monster move a bit on new level (see placement code below) */
        wander = (xint16) min(nmv, 8L);
    } else
        wander = 0;

    switch (xyloc) {
    case MIGR_APPROX_XY: /* {x,y}locale set above */
        break;
    case MIGR_EXACT_XY:
        wander = 0;
        break;
    case MIGR_WITH_HERO:
        xlocale = u.ux, ylocale = u.uy;
        break;
    case MIGR_STAIRS_UP:
        if ((stway = stairway_find_from(&fromdlev, FALSE)) != 0) {
            xlocale = stway->sx;
            ylocale = stway->sy;
        }
        break;
    case MIGR_STAIRS_DOWN:
        if ((stway = stairway_find_from(&fromdlev, FALSE)) != 0) {
            xlocale = stway->sx;
            ylocale = stway->sy;
        }
        break;
    case MIGR_LADDER_UP:
        if ((stway = stairway_find_from(&fromdlev, TRUE)) != 0) {
            xlocale = stway->sx;
            ylocale = stway->sy;
        }
        break;
    case MIGR_LADDER_DOWN:
        if ((stway = stairway_find_from(&fromdlev, TRUE)) != 0) {
            xlocale = stway->sx;
            ylocale = stway->sy;
        }
        break;
    case MIGR_SSTAIRS:
        if ((stway = stairway_find(&fromdlev)) != 0) {
            xlocale = stway->sx;
            ylocale = stway->sy;
        }
        break;
    case MIGR_PORTAL:
        if (In_endgame(&u.uz)) {
            /* there is no arrival portal for endgame levels */
            /* BUG[?]: for simplicity, this code relies on the fact
               that we know that the current endgame levels always
               build upwards and never have any exclusion subregion
               inside their TELEPORT_REGION settings. */
            xlocale = rn1(gu.updest.hx - gu.updest.lx + 1, gu.updest.lx);
            ylocale = rn1(gu.updest.hy - gu.updest.ly + 1, gu.updest.ly);
            break;
        }
        /* find the arrival portal */
        for (t = gf.ftrap; t; t = t->ntrap)
            if (t->ttyp == MAGIC_PORTAL)
                break;
        if (t) {
            xlocale = t->tx, ylocale = t->ty;
            break;
        } else if (iflags.debug_fuzzer
                   && (stway = stairway_find_dir(!builds_up(&u.uz))) != 0) {
            /* debugfuzzer returns from or enters another branch */
            xlocale = stway->sx, ylocale = stway->sy;
        } else if (!(u.uevent.qexpelled
                     && (Is_qstart(&u.uz0) || Is_qstart(&u.uz)))) {
            impossible("mon_arrive: no corresponding portal?");
        } /*FALLTHRU*/
    default:
    case MIGR_RANDOM:
        xlocale = ylocale = 0;
        break;
    }

    if ((mtmp->migflags & MIGR_LEFTOVERS) != 0L) {
        /* Pick up the rest of the MIGR_TO_SPECIES objects */
        if (gm.migrating_objs)
            deliver_obj_to_mon(mtmp, 0, DF_ALL);
    }

    if (xlocale && wander) {
        /* monster moved a bit; pick a nearby location */
        /* mnearto() deals w/stone, et al */
        char *r = in_rooms(xlocale, ylocale, 0);

        if (r && *r) {
            coord c;

            /* somexy() handles irregular rooms */
            if (somexy(&gr.rooms[*r - ROOMOFFSET], &c))
                xlocale = c.x, ylocale = c.y;
            else
                xlocale = ylocale = 0;
        } else { /* not in a room */
            int i, j;

            i = max(1, xlocale - wander);
            j = min(COLNO - 1, xlocale + wander);
            xlocale = rn1(j - i, i);
            i = max(0, ylocale - wander);
            j = min(ROWNO - 1, ylocale + wander);
            ylocale = rn1(j - i, i);
        }
    } /* moved a bit */

    mtmp->mx = 0; /*(already is 0)*/
    mtmp->my = xyflags;
    if (xlocale)
        failed_to_place = !mnearto(mtmp, xlocale, ylocale, FALSE, RLOC_NOMSG);
    else
        failed_to_place = !rloc(mtmp, RLOC_NOMSG);

    if (failed_to_place) {
        if (when != Wiz_arrive)
            /* losedogs() will deal with this */
            relmon(mtmp, &failed_arrivals);
        else /* when==Wiz_arrive => not being called by losedogs() */
            m_into_limbo(mtmp);
    }
}

/* heal monster for time spent elsewhere */
void
mon_catchup_elapsed_time(
    struct monst *mtmp,
    long nmv) /* number of moves */
{
    int imv = 0; /* avoid zillions of casts and lint warnings */

#if defined(DEBUG) || (NH_DEVEL_STATUS != NH_STATUS_RELEASED)

    if (nmv < 0L) { /* crash likely... */
        panic("catchup from future time?");
        /*NOTREACHED*/
        return;
    } else if (nmv == 0L) { /* safe, but shouldn't happen */
        impossible("catchup from now?");
    } else
#endif
        if (nmv >= LARGEST_INT) /* paranoia */
        imv = LARGEST_INT - 1;
    else
        imv = (int) nmv;

    /* might stop being afraid, blind or frozen */
    /* set to 1 and allow final decrement in movemon() */
    if (mtmp->mblinded) {
        if (imv >= (int) mtmp->mblinded)
            mtmp->mblinded = 1;
        else
            mtmp->mblinded -= imv;
    }
    if (mtmp->mfrozen) {
        if (imv >= (int) mtmp->mfrozen)
            mtmp->mfrozen = 1;
        else
            mtmp->mfrozen -= imv;
    }
    if (mtmp->mfleetim) {
        if (imv >= (int) mtmp->mfleetim)
            mtmp->mfleetim = 1;
        else
            mtmp->mfleetim -= imv;
    }

    /* might recover from temporary trouble */
    if (mtmp->mtrapped && rn2(imv + 1) > 40 / 2)
        mtmp->mtrapped = 0;
    if (mtmp->mconf && rn2(imv + 1) > 50 / 2)
        mtmp->mconf = 0;
    if (mtmp->mstun && rn2(imv + 1) > 10 / 2)
        mtmp->mstun = 0;

    /* might finish eating or be able to use special ability again */
    if (imv > mtmp->meating)
        finish_meating(mtmp);
    else
        mtmp->meating -= imv;
    if (imv > mtmp->mspec_used)
        mtmp->mspec_used = 0;
    else
        mtmp->mspec_used -= imv;

    /* reduce tameness for every 150 moves you are separated */
    if (mtmp->mtame) {
        int wilder = (imv + 75) / 150;
        if (mtmp->mtame > wilder)
            mtmp->mtame -= wilder; /* less tame */
        else if (mtmp->mtame > rn2(wilder))
            mtmp->mtame = 0; /* untame */
        else
            mtmp->mtame = mtmp->mpeaceful = 0; /* hostile! */
    }
    /* check to see if it would have died as a pet; if so, go wild instead
     * of dying the next time we call dog_move()
     */
    if (mtmp->mtame && !mtmp->isminion
        && (carnivorous(mtmp->data) || herbivorous(mtmp->data))) {
        struct edog *edog = EDOG(mtmp);

        if ((gm.moves > edog->hungrytime + 500 && mtmp->mhp < 3)
            || (gm.moves > edog->hungrytime + 750))
            mtmp->mtame = mtmp->mpeaceful = 0;
    }

    if (!mtmp->mtame && mtmp->mleashed) {
        /* leashed monsters should always be with hero, consequently
           never losing any time to be accounted for later */
        impossible("catching up for leashed monster?");
        m_unleash(mtmp, FALSE);
    }

    /* recover lost hit points */
    if (!regenerates(mtmp->data))
        imv /= 20;
    if (mtmp->mhp + imv >= mtmp->mhpmax)
        mtmp->mhp = mtmp->mhpmax;
    else
        mtmp->mhp += imv;

    set_mon_lastmove(mtmp);
}

/* bookkeeping when mtmp is about to leave the current level;
   common to keepdogs() and migrate_to_level() */
static int
mon_leave(struct monst *mtmp)
{
    struct obj *obj;
    int num_segs = 0; /* return value */

    /* set minvent's obj->no_charge to 0 */
    for (obj = mtmp->minvent; obj; obj = obj->nobj) {
        if (Has_contents(obj))
            picked_container(obj); /* does the right thing */
        obj->no_charge = 0;
    }

    /* if this is a shopkeeper, clear the 'resident' field of her shop;
       if/when she returns, it will be set back by mon_arrive()  */
    if (mtmp->isshk)
        set_residency(mtmp, TRUE);

    /* if this is a long worm, handle its tail segments before mtmp itself;
       we pass possibly truncated segment count to caller via return value  */
    if (mtmp->wormno) {
        int cnt = count_wsegs(mtmp), mx = mtmp->mx, my = mtmp->my;

        /* since monst->wormno is overloaded to hold the number of
           tail segments during migration, a very long worm with
           more segments than can fit in that field gets truncated */
        num_segs = min(cnt, MAX_NUM_WORMS - 1);
        wormgone(mtmp);
        /* put the head back; note: mtmp might not be on the map if this
           is happening during a failed attempt to migrate to this level */
        if (mx)
            place_monster(mtmp, mx, my);
    }

    return num_segs;
}

/* when hero leaves a level, some monsters should be placed on the
   migrating_mons list instead of being stashed inside the level's file */
static boolean
keep_mon_accessible(struct monst *mon)
{
    /* the Wizard is kept accessible so that his harassment can fetch
       him instead of creating a new instance but also so that he can
       be put back at his current location if hero returns to his level */
    if (mon->iswiz)
        return TRUE;
    /* monsters with special attachment to a particular level only need
       to be kept accessible when on some other level */
    if (mon->mextra
        && ((mon->isshk && !on_level(&u.uz, &ESHK(mon)->shoplevel))
            || (mon->ispriest && !on_level(&u.uz, &EPRI(mon)->shrlevel))
            || (mon->isgd && !on_level(&u.uz, &EGD(mon)->gdlevel))))
        return TRUE;
    /* normal monsters go into the level save file instead of being held
       on the migrating_mons list for off-level accessibility */
    return FALSE;
}

/* called when you move to another level */
void
keepdogs(
    boolean pets_only) /* true for ascension or final escape */
{
    register struct monst *mtmp, *mtmp2;

    for (mtmp = fmon; mtmp; mtmp = mtmp2) {
        mtmp2 = mtmp->nmon;
        if (DEADMONSTER(mtmp))
            continue;
        if (pets_only) {
            if (!mtmp->mtame)
                continue; /* reject non-pets */
            /* don't block pets from accompanying hero's dungeon
               escape or ascension simply due to mundane trifles;
               unlike level change for steed, don't bother trying
               to achieve a normal trap escape first */
            mtmp->mtrapped = 0;
            mtmp->meating = 0;
            mtmp->msleeping = 0;
            mtmp->mfrozen = 0;
            mtmp->mcanmove = 1;
        }
        if (((monnear(mtmp, u.ux, u.uy) && levl_follower(mtmp))
             /* the wiz will level t-port from anywhere to chase
                the amulet; if you don't have it, will chase you
                only if in range. -3. */
             || (u.uhave.amulet && mtmp->iswiz))
            && (!helpless(mtmp)
                /* eg if level teleport or new trap, steed has no control
                   to avoid following */
                || (mtmp == u.usteed))
            /* monster won't follow if it hasn't noticed you yet */
            && !(mtmp->mstrategy & STRAT_WAITFORU)) {
            int num_segs;
            boolean stay_behind = FALSE;

            if (mtmp->mtrapped)
                (void) mintrap(mtmp, NO_TRAP_FLAGS); /* try to escape */
            if (mtmp == u.usteed) {
                /* make sure steed is eligible to accompany hero */
                mtmp->mtrapped = 0;       /* escape trap */
                mtmp->meating = 0;        /* terminate eating */
                mdrop_special_objs(mtmp); /* drop Amulet */
            } else if (mtmp->meating || mtmp->mtrapped) {
                if (canseemon(mtmp))
                    pline("%s is still %s.", Monnam(mtmp),
                          mtmp->meating ? "eating" : "trapped");
                stay_behind = TRUE;
            } else if (mon_has_amulet(mtmp)) {
                if (canseemon(mtmp))
                    pline("%s seems very disoriented for a moment.",
                          Monnam(mtmp));
                stay_behind = TRUE;
            }
            if (stay_behind) {
                if (mtmp->mleashed) {
                    pline("%s leash suddenly comes loose.",
                          humanoid(mtmp->data)
                              ? (mtmp->female ? "Her" : "His")
                              : "Its");
                    m_unleash(mtmp, FALSE);
                }
                if (mtmp == u.usteed) {
                    /* can't happen unless someone makes a change
                       which scrambles the stay_behind logic above */
                    impossible("steed left behind?");
                    dismount_steed(DISMOUNT_GENERIC);
                }
                continue;
            }

            /* prepare to take mtmp off the map */
            num_segs = mon_leave(mtmp);
            /* take off map and move mtmp from fmon list to mydogs */
            relmon(mtmp, &gm.mydogs); /* mtmp->mx,my retain current value */
            mtmp->mx = mtmp->my = 0; /* mx==0 implies migrating */
            mtmp->wormno = num_segs;
            mtmp->mlstmv = gm.moves;
        } else if (keep_mon_accessible(mtmp)) {
            /* we want to be able to find the Wizard when his next
               resurrection chance comes up, but have him resume his
               present location if player returns to this level before
               that time; also needed for monsters (shopkeeper, temple
               priest, vault guard) who have level data in mon->mextra
               in case #wizmakemap is used to replace their home level
               while they're away from it */
            migrate_to_level(mtmp, ledger_no(&u.uz), MIGR_EXACT_XY,
                             (coord *) 0);
        } else if (mtmp->mleashed) {
            /* this can happen if your quest leader ejects you from the
               "home" level while a leashed pet isn't next to you */
            pline("%s leash goes slack.", s_suffix(Monnam(mtmp)));
            m_unleash(mtmp, FALSE);
        }
    }
}

void
migrate_to_level(
    struct monst *mtmp,
    xint16 tolev, /* destination level */
    xint16 xyloc, /* MIGR_xxx destination xy location: */
    coord *cc)   /* optional destination coordinates */
{
    d_level new_lev;
    coordxy xyflags;
    coordxy mx = mtmp->mx, my = mtmp->my; /* <mx,my> needed below */
    int num_segs; /* count of worm segments */

    if (mtmp->mleashed) {
        mtmp->mtame--;
        m_unleash(mtmp, TRUE);
    }

    /* prepare to take mtmp off the map */
    num_segs = mon_leave(mtmp);
    /* take off map and move mtmp from fmon list to migrating_mons */
    relmon(mtmp, &gm.migrating_mons); /* mtmp->mx,my retain their value */
    mtmp->mstate |= MON_MIGRATING;

    new_lev.dnum = ledger_to_dnum((xint16) tolev);
    new_lev.dlevel = ledger_to_dlev((xint16) tolev);
    /* overload mtmp->[mx,my], mtmp->[mux,muy], and mtmp->mtrack[] as
       destination codes */
    xyflags = (depth(&new_lev) < depth(&u.uz)); /* 1 => up */
    if (In_W_tower(mx, my, &u.uz))
        xyflags |= 2;
    mtmp->wormno = num_segs;
    mtmp->mlstmv = gm.moves;
    mtmp->mtrack[2].x = u.uz.dnum; /* migrating from this dungeon */
    mtmp->mtrack[2].y = u.uz.dlevel; /* migrating from this dungeon level */
    mtmp->mtrack[1].x = cc ? cc->x : mx;
    mtmp->mtrack[1].y = cc ? cc->y : my;
    mtmp->mtrack[0].x = xyloc;
    mtmp->mtrack[0].y = xyflags;
    mtmp->mux = new_lev.dnum;
    mtmp->muy = new_lev.dlevel;
    mtmp->mx = mtmp->my = 0; /* mx==0 implies migrating */

    /* don't extinguish a mobile light; it still exists but has changed
       from local (monst->mx > 0) to global (mx==0, not on this level) */
    if (emits_light(mtmp->data))
        vision_recalc(0);
}

/* when entering the endgame, levels from the dungeon and its branches are
   discarded because they can't be reached again; do the same for monsters
   and objects scheduled to migrate to those levels */
void
discard_migrations(void)
{
    struct monst *mtmp, **mprev;
    struct obj *otmp, **oprev;
    d_level dest;

    for (mprev = &gm.migrating_mons; (mtmp = *mprev) != 0; ) {
        dest.dnum = mtmp->mux;
        dest.dlevel = mtmp->muy;
        /* the Wizard is kept regardless of location so that he is
           ready to be brought back; nothing should be scheduled to
           migrate to the endgame but if we find such, we'll keep it */
        if (mtmp->iswiz || In_endgame(&dest)) {
            mprev = &mtmp->nmon; /* keep mtmp on migrating_mons */
        } else {
            *mprev = mtmp->nmon; /* remove mtmp from migrating_mons */
            mtmp->nmon = 0;
            discard_minvent(mtmp, FALSE);
            /* bypass mongone() and its call to m_detach() plus dmonsfree() */
            dealloc_monst(mtmp);
        }
    }

    /* objects get similar treatment */
    for (oprev = &gm.migrating_objs; (otmp = *oprev) != 0; ) {
        dest.dnum = otmp->ox;
        dest.dlevel = otmp->oy;
        /* there is no special case like the Wizard (certainly not the
           Amulet; the hero has to be carrying it to enter the endgame
           which triggers the call to this routine); again we don't
           expect any objects to be migrating to the endgame but will
           keep any we find so that they could be delivered */
        if (In_endgame(&dest)) {
            oprev = &otmp->nobj; /* keep otmp on migrating_objs */
        } else {
            /* bypass obj_extract_self() */
            *oprev = otmp->nobj; /* remove otmp from migrating_objs */
            otmp->nobj = 0;
            otmp->where = OBJ_FREE;
            otmp->owornmask = 0L; /* overloaded for destination usage;
                                   * obfree() will complain if nonzero */
            obfree(otmp, (struct obj *) 0); /* releases any contents too */
        }
    }
}

/* returns the quality of an item of food; the lower the better;
   fungi and ghouls will eat even tainted food */
int
dogfood(struct monst *mon, struct obj *obj)
{
    struct permonst *mptr = mon->data, *fptr;
    boolean carni = carnivorous(mptr), herbi = herbivorous(mptr),
            starving, mblind;
    int fx;

    if (is_quest_artifact(obj) || obj_resists(obj, 0, 95))
        return obj->cursed ? TABU : APPORT;

    switch (obj->oclass) {
    case FOOD_CLASS:
        fx = (obj->otyp == CORPSE || obj->otyp == TIN || obj->otyp == EGG)
                ? obj->corpsenm
                : NUMMONS; /* valid mons[mndx] to pacify static analyzer */
        fptr = &mons[fx];

        if (obj->otyp == CORPSE && is_rider(fptr))
            return TABU;
        if ((obj->otyp == CORPSE || obj->otyp == EGG)
            /* Medusa's corpse doesn't pass the touch_petrifies() test
               but does cause petrification if eaten */
            && (touch_petrifies(fptr) || obj->corpsenm == PM_MEDUSA)
            && !resists_ston(mon))
            return POISON;
        if (obj->otyp == LUMP_OF_ROYAL_JELLY
            && mon->data == &mons[PM_KILLER_BEE]) {
            struct monst *mtmp = find_pmmonst(PM_QUEEN_BEE);

            /* if there's a queen bee on the level, don't eat royal jelly;
               if there isn't, do eat it and grow into a queen */
            return !mtmp ? DOGFOOD : TABU;
        }
        if (!carni && !herbi)
            return obj->cursed ? UNDEF : APPORT;

        /* a starving pet will eat almost anything */
        starving = (mon->mtame && !mon->isminion
                    && EDOG(mon)->mhpmax_penalty);
        /* even carnivores will eat carrots if they're temporarily blind */
        mblind = (!mon->mcansee && haseyes(mon->data));

        /* ghouls prefer old corpses and unhatchable eggs, yum!
           they'll eat fresh non-veggy corpses and hatchable eggs
           when starving; they never eat stone-to-flesh'd meat */
        if (mptr == &mons[PM_GHOUL]) {
            if (obj->otyp == CORPSE)
                return (peek_at_iced_corpse_age(obj) + 50L <= gm.moves
                        && !(fx == PM_LIZARD || fx == PM_LICHEN)) ? DOGFOOD
                       : (starving && !vegan(fptr)) ? ACCFOOD
                         : POISON;
            if (obj->otyp == EGG)
                return stale_egg(obj) ? CADAVER : starving ? ACCFOOD : POISON;
            return TABU;
        }

        switch (obj->otyp) {
        case TRIPE_RATION:
        case MEATBALL:
        case MEAT_RING:
        case MEAT_STICK:
        case ENORMOUS_MEATBALL:
            return carni ? DOGFOOD : MANFOOD;
        case EGG:
            return carni ? CADAVER : MANFOOD;
        case CORPSE:
            if ((peek_at_iced_corpse_age(obj) + 50L <= gm.moves
                 && !(fx == PM_LIZARD || fx == PM_LICHEN)
                 && mptr->mlet != S_FUNGUS)
                || (acidic(fptr) && !resists_acid(mon))
                || (poisonous(fptr) && !resists_poison(mon)))
                return POISON;
            /* avoid polymorph unless starving or abused (in which case the
               pet will consider it for a chance to become more powerful) */
            else if (is_shapeshifter(fptr) && mon->mtame > 1 && !starving)
                return MANFOOD;
            else if (vegan(fptr))
                return herbi ? CADAVER : MANFOOD;
            /* most humanoids will avoid cannibalism unless starving;
               arbitrary: elves won't eat other elves even then */
            else if (humanoid(mptr) && same_race(mptr, fptr)
                     && (!is_undead(mptr) && fptr->mlet != S_KOBOLD
                         && fptr->mlet != S_ORC && fptr->mlet != S_OGRE))
                return (starving && carni && !is_elf(mptr)) ? ACCFOOD : TABU;
            else
                return carni ? CADAVER : MANFOOD;
        case GLOB_OF_GREEN_SLIME: /* other globs use the default case */
            /* turning into slime is preferable to starvation */
            return (starving || slimeproof(mon->data)) ? ACCFOOD : POISON;
        case CLOVE_OF_GARLIC:
            return (is_undead(mptr) || is_vampshifter(mon)) ? TABU
                   : (herbi || starving) ? ACCFOOD
                     : MANFOOD;
        case TIN:
            return metallivorous(mptr) ? ACCFOOD : MANFOOD;
        case APPLE:
            return herbi ? DOGFOOD : starving ? ACCFOOD : MANFOOD;
        case CARROT:
            return (herbi || mblind) ? DOGFOOD : starving ? ACCFOOD : MANFOOD;
        case BANANA:
            /* monkeys and apes (tamable) plus sasquatch prefer these,
               yetis will only will only eat them if starving */
            return (mptr->mlet == S_YETI && herbi) ? DOGFOOD
                   : (herbi || starving) ? ACCFOOD
                     : MANFOOD;
        default:
            if (starving)
                return ACCFOOD;
            return (obj->otyp > SLIME_MOLD) ? (carni ? ACCFOOD : MANFOOD)
                                            : (herbi ? ACCFOOD : MANFOOD);
        }
    default:
        if (obj->otyp == AMULET_OF_STRANGULATION
            || obj->otyp == RIN_SLOW_DIGESTION)
            return TABU;
        if (mon_hates_silver(mon) && objects[obj->otyp].oc_material == SILVER)
            return TABU;
        if (mptr == &mons[PM_GELATINOUS_CUBE] && is_organic(obj))
            return ACCFOOD;
        if (metallivorous(mptr) && is_metallic(obj)
            && (is_rustprone(obj) || mptr != &mons[PM_RUST_MONSTER])) {
            /* Non-rustproofed ferrous-based metals are preferred. */
            return (is_rustprone(obj) && !obj->oerodeproof) ? DOGFOOD
                                                            : ACCFOOD;
        }
        if (!obj->cursed
            && obj->oclass != BALL_CLASS
            && obj->oclass != CHAIN_CLASS)
            return APPORT;
        /*FALLTHRU*/
    case ROCK_CLASS:
        return UNDEF;
    }
}

/*
 * tamedog() used to return the monster, which might have changed address
 * if a new one was created in order to allocate the edog extension.
 * With the separate mextra structure added in 3.6.x it always operates
 * on the original mtmp.  It now returns TRUE if the taming succeeded.
 */
boolean
tamedog(struct monst *mtmp, struct obj *obj)
{
    /* reduce timed sleep or paralysis, leaving mtmp->mcanmove as-is
       (note: if mtmp is donning armor, this will reduce its busy time) */
    if (mtmp->mfrozen)
        mtmp->mfrozen = (mtmp->mfrozen + 1) / 2;
    /* end indefinite sleep; using distance==1 limits the waking to mtmp */
    if (mtmp->msleeping)
        wake_nearto(mtmp->mx, mtmp->my, 1); /* [different from wakeup()] */

    /* The Wiz, Medusa and the quest nemeses aren't even made peaceful. */
    if (mtmp->iswiz || mtmp->data == &mons[PM_MEDUSA]
        || (mtmp->data->mflags3 & M3_WANTSARTI))
        return FALSE;

    /* worst case, at least it'll be peaceful. */
    mtmp->mpeaceful = 1;
    set_malign(mtmp);
    if (flags.moonphase == FULL_MOON && night() && rn2(6) && obj
        && mtmp->data->mlet == S_DOG)
        return FALSE;

    /* If we cannot tame it, at least it's no longer afraid. */
    mtmp->mflee = 0;
    mtmp->mfleetim = 0;

    /* make grabber let go now, whether it becomes tame or not */
    if (mtmp == u.ustuck) {
        if (u.uswallow)
            expels(mtmp, mtmp->data, TRUE);
        else if (!(Upolyd && sticks(gy.youmonst.data)))
            unstuck(mtmp);
    }

    /* feeding it treats makes it tamer */
    if (mtmp->mtame && obj) {
        int tasty;

        if (mtmp->mcanmove && !mtmp->mconf && !mtmp->meating
            && ((tasty = dogfood(mtmp, obj)) == DOGFOOD
                || (tasty <= ACCFOOD
                    && EDOG(mtmp)->hungrytime <= gm.moves))) {
            /* pet will "catch" and eat this thrown food */
            if (canseemon(mtmp)) {
                boolean big_corpse =
                    (obj->otyp == CORPSE && obj->corpsenm >= LOW_PM
                     && mons[obj->corpsenm].msize > mtmp->data->msize);
                pline("%s catches %s%s", Monnam(mtmp), the(xname(obj)),
                      !big_corpse ? "." : ", or vice versa!");
            } else if (cansee(mtmp->mx, mtmp->my))
                pline("%s.", Tobjnam(obj, "stop"));
            /* dog_eat expects a floor object */
            place_object(obj, mtmp->mx, mtmp->my);
            (void) dog_eat(mtmp, obj, mtmp->mx, mtmp->my, FALSE);
            /* eating might have killed it, but that doesn't matter here;
               a non-null result suppresses "miss" message for thrown
               food and also implies that the object has been deleted */
            return TRUE;
        } else
            return FALSE;
    }

    /* if already tame, taming magic might make it become tamer */
    if (mtmp->mtame) {
        /* maximum tameness is 20, only reachable via eating */
        if (rnd(10) > mtmp->mtame)
            mtmp->mtame++;
        return FALSE; /* didn't just get tamed */
    }
    /* pacify angry shopkeeper but don't tame him/her/it/them */
    if (mtmp->isshk) {
        make_happy_shk(mtmp, FALSE);
        return FALSE;
    }

    if (!mtmp->mcanmove
        /* monsters with conflicting structures cannot be tamed
           [note: the various mextra structures don't actually conflict
           with each other anymore] */
        || mtmp->isshk || mtmp->isgd || mtmp->ispriest || mtmp->isminion
        || is_covetous(mtmp->data) || is_human(mtmp->data)
        || (is_demon(mtmp->data) && !is_demon(gy.youmonst.data))
        || (obj && dogfood(mtmp, obj) >= MANFOOD))
        return FALSE;

    if (mtmp->m_id == gq.quest_status.leader_m_id)
        return FALSE;

    /* add the pet extension */
    newedog(mtmp);
    initedog(mtmp);

    if (obj) { /* thrown food */
        /* defer eating until the edog extension has been set up */
        place_object(obj, mtmp->mx, mtmp->my); /* put on floor */
        /* devour the food (might grow into larger, genocided monster) */
        if (dog_eat(mtmp, obj, mtmp->mx, mtmp->my, TRUE) == 2)
            return TRUE; /* oops, it died... */
        /* `obj' is now obsolete */
    }

    newsym(mtmp->mx, mtmp->my);
    if (attacktype(mtmp->data, AT_WEAP)) {
        mtmp->weapon_check = NEED_HTH_WEAPON;
        (void) mon_wield_item(mtmp);
    }
    return TRUE;
}

/*
 * Called during pet revival or pet life-saving.
 * If you killed the pet, it revives wild.
 * If you abused the pet a lot while alive, it revives wild.
 * If you abused the pet at all while alive, it revives untame.
 * If the pet wasn't abused and was very tame, it might revive tame.
 */
void
wary_dog(struct monst *mtmp, boolean was_dead)
{
    struct edog *edog;
    boolean quietly = was_dead;

    finish_meating(mtmp);

    if (!mtmp->mtame)
        return;
    edog = !mtmp->isminion ? EDOG(mtmp) : 0;

    /* if monster was starving when it died, undo that now */
    if (edog && edog->mhpmax_penalty) {
        mtmp->mhpmax += edog->mhpmax_penalty;
        mtmp->mhp += edog->mhpmax_penalty; /* heal it */
        edog->mhpmax_penalty = 0;
    }

    if (edog && (edog->killed_by_u == 1 || edog->abuse > 2)) {
        mtmp->mpeaceful = mtmp->mtame = 0;
        if (edog->abuse >= 0 && edog->abuse < 10)
            if (!rn2(edog->abuse + 1))
                mtmp->mpeaceful = 1;
        if (!quietly && cansee(mtmp->mx, mtmp->my)) {
            if (haseyes(gy.youmonst.data)) {
                if (haseyes(mtmp->data))
                    pline("%s %s to look you in the %s.", Monnam(mtmp),
                          mtmp->mpeaceful ? "seems unable" : "refuses",
                          body_part(EYE));
                else
                    pline("%s avoids your gaze.", Monnam(mtmp));
            }
        }
    } else {
        /* chance it goes wild anyway - Pet Sematary */
        mtmp->mtame = rn2(mtmp->mtame + 1);
        if (!mtmp->mtame)
            mtmp->mpeaceful = rn2(2);
    }

    if (!mtmp->mtame) {
        if (!quietly && canspotmon(mtmp))
            pline("%s %s.", Monnam(mtmp),
                  mtmp->mpeaceful ? "is no longer tame" : "has become feral");
        newsym(mtmp->mx, mtmp->my);
        /* a life-saved monster might be leashed;
           don't leave it that way if it's no longer tame */
        if (mtmp->mleashed)
            m_unleash(mtmp, TRUE);
        if (mtmp == u.usteed)
            dismount_steed(DISMOUNT_THROWN);
    } else if (edog) {
        /* it's still a pet; start a clean pet-slate now */
        edog->revivals++;
        edog->killed_by_u = 0;
        edog->abuse = 0;
        edog->ogoal.x = edog->ogoal.y = -1;
        if (was_dead || edog->hungrytime < gm.moves + 500L)
            edog->hungrytime = gm.moves + 500L;
        if (was_dead) {
            edog->droptime = 0L;
            edog->dropdist = 10000;
            edog->whistletime = 0L;
            edog->apport = 5;
        } /* else lifesaved, so retain current values */
    }
}

void
abuse_dog(struct monst *mtmp)
{
    if (!mtmp->mtame)
        return;

    if (Aggravate_monster || Conflict)
        mtmp->mtame /= 2;
    else
        mtmp->mtame--;

    if (mtmp->mtame && !mtmp->isminion)
        EDOG(mtmp)->abuse++;

    if (!mtmp->mtame && mtmp->mleashed)
        m_unleash(mtmp, TRUE);

    /* don't make a sound if pet is in the middle of leaving the level */
    /* newsym isn't necessary in this case either */
    if (mtmp->mx != 0) {
        if (mtmp->mtame && rn2(mtmp->mtame))
            yelp(mtmp);
        else
            growl(mtmp); /* give them a moment's worry */

        if (!mtmp->mtame)
            newsym(mtmp->mx, mtmp->my);
    }
}

/*dog.c*/
