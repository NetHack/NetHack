/* NetHack 3.7	bones.c	$NHDT-Date: 1596498151 2020/08/03 23:42:31 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.103 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985,1993. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

static boolean no_bones_level(d_level *);
static void goodfruit(int);
static void resetobjs(struct obj *, boolean);
static void give_to_nearby_mon(struct obj *, int, int);
static boolean fixuporacle(struct monst *);

static boolean
no_bones_level(d_level *lev)
{
    s_level *sptr;

    if (ledger_no(&g.save_dlevel))
        assign_level(lev, &g.save_dlevel);

    return (boolean) (((sptr = Is_special(lev)) != 0 && !sptr->boneid)
                      || !g.dungeons[lev->dnum].boneid
                      /* no bones on the last or multiway branch levels
                         in any dungeon (level 1 isn't multiway) */
                      || Is_botlevel(lev)
                      || (Is_branchlev(lev) && lev->dlevel > 1)
                      /* no bones in the invocation level */
                      || (In_hell(lev)
                          && lev->dlevel == dunlevs_in_dungeon(lev) - 1));
}

/* Call this function for each fruit object saved in the bones level: it marks
 * that particular type of fruit as existing (the marker is that that type's
 * ID is positive instead of negative).  This way, when we later save the
 * chain of fruit types, we know to only save the types that exist.
 */
static void
goodfruit(int id)
{
    struct fruit *f = fruit_from_indx(-id);

    if (f)
        f->fid = id;
}

static void
resetobjs(struct obj *ochain, boolean restore)
{
    struct obj *otmp, *nobj;

    for (otmp = ochain; otmp; otmp = nobj) {
        nobj = otmp->nobj;
        if (otmp->cobj)
            resetobjs(otmp->cobj, restore);
        if (otmp->in_use) {
            obj_extract_self(otmp);
            dealloc_obj(otmp);
            continue;
        }

        if (restore) {
            /* artifact bookkeeping needs to be done during
               restore; other fixups are done while saving */
            if (otmp->oartifact) {
                if (exist_artifact(otmp->otyp, safe_oname(otmp))
                    || is_quest_artifact(otmp)) {
                    /* prevent duplicate--revert to ordinary obj */
                    otmp->oartifact = 0;
                    if (has_oname(otmp))
                        free_oname(otmp);
                } else {
                    artifact_exists(otmp, safe_oname(otmp), TRUE,
                                    ONAME_BONES);
                }
            } else if (has_oname(otmp)) {
                sanitize_name(ONAME(otmp));
            }
            /* 3.6.3: set no_charge for partly eaten food in shop;
               all other items become goods for sale if in a shop */
            if (otmp->oclass == FOOD_CLASS && otmp->oeaten) {
                struct obj *top;
                char *p;
                xchar ox, oy;

                for (top = otmp; top->where == OBJ_CONTAINED;
                     top = top->ocontainer)
                    continue;
                otmp->no_charge = (top->where == OBJ_FLOOR
                                   && get_obj_location(top, &ox, &oy, 0)
                                   /* can't use costly_spot() since its
                                      result depends upon hero's location */
                                   && inside_shop(ox, oy)
                                   && *(p = in_rooms(ox, oy, SHOPBASE))
                                   && tended_shop(&g.rooms[*p - ROOMOFFSET]));
            }
        } else { /* saving */
            /* do not zero out o_ids for ghost levels anymore */

            if (objects[otmp->otyp].oc_uses_known)
                otmp->known = 0;
            otmp->dknown = otmp->bknown = 0;
            otmp->rknown = 0;
            otmp->lknown = 0;
            otmp->cknown = 0;
            otmp->invlet = 0;
            otmp->no_charge = 0;
            otmp->was_thrown = 0;

            /* strip user-supplied names */
            /* Statue and some corpse names are left intact,
               presumably in case they came from score file.
               [TODO: this ought to be done differently--names
               which came from such a source or came from any
               stoned or killed monster should be flagged in
               some manner; then we could just check the flag
               here and keep "real" names (dead pets, &c) while
               discarding player notes attached to statues.] */
            if (has_oname(otmp)
                && !(otmp->oartifact || otmp->otyp == STATUE
                     || otmp->otyp == SPE_NOVEL
                     || (otmp->otyp == CORPSE
                         && otmp->corpsenm >= SPECIAL_PM))) {
                free_oname(otmp);
            }

            if (otmp->otyp == SLIME_MOLD) {
                goodfruit(otmp->spe);
#ifdef MAIL_STRUCTURES
            } else if (otmp->otyp == SCR_MAIL) {
                /* 0: delivered in-game via external event;
                   1: from bones or wishing; 2: written with marker */
                if (otmp->spe == 0)
                    otmp->spe = 1;
#endif
            } else if (otmp->otyp == EGG) {
                otmp->spe = 0; /* not "laid by you" in next game */
            } else if (otmp->otyp == TIN) {
                /* make tins of unique monster's meat be empty */
                if (otmp->corpsenm >= LOW_PM
                    && unique_corpstat(&mons[otmp->corpsenm]))
                    otmp->corpsenm = NON_PM;
            } else if (otmp->otyp == CORPSE || otmp->otyp == STATUE) {
                int mnum = otmp->corpsenm;

                /* Discard incarnation details of unique monsters
                   (by passing null instead of otmp for object),
                   shopkeepers (by passing false for revival flag),
                   temple priests, and vault guards in order to
                   prevent corpse revival or statue reanimation. */
                if (has_omonst(otmp)
                    && cant_revive(&mnum, FALSE, (struct obj *) 0)) {
                    free_omonst(otmp);
                    /* mnum is now either human_zombie or doppelganger;
                       for corpses of uniques, we need to force the
                       transformation now rather than wait until a
                       revival attempt, otherwise eating this corpse
                       would behave as if it remains unique */
                    if (mnum == PM_DOPPELGANGER && otmp->otyp == CORPSE)
                        set_corpsenm(otmp, mnum);
                }
            } else if (is_mines_prize(otmp) || is_soko_prize(otmp)) {
                /* achievement tracking; in case prize was moved off its
                   original level (which is always a no-bones level) */
                otmp->nomerge = 0;
            } else if (otmp->otyp == AMULET_OF_YENDOR) {
                /* no longer the real Amulet */
                otmp->otyp = FAKE_AMULET_OF_YENDOR;
                curse(otmp);
            } else if (otmp->otyp == CANDELABRUM_OF_INVOCATION) {
                if (otmp->lamplit)
                    end_burn(otmp, TRUE);
                otmp->otyp = WAX_CANDLE;
                otmp->age = 50L; /* assume used */
                if (otmp->spe > 0)
                    otmp->quan = (long) otmp->spe;
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

/* while loading bones, strip out text possibly supplied by old player
   that might accidentally or maliciously disrupt new player's display */
void
sanitize_name(char *namebuf)
{
    int c;
    boolean strip_8th_bit = (WINDOWPORT("tty")
                             && !iflags.wc_eight_bit_input);

    /* it's tempting to skip this for single-user platforms, since
       only the current player could have left these bones--except
       things like "hearse" and other bones exchange schemes make
       that assumption false */
    while (*namebuf) {
        c = *namebuf & 0177;
        if (c < ' ' || c == '\177') {
            /* non-printable or undesirable */
            *namebuf = '.';
        } else if (c != *namebuf) {
            /* expected to be printable if user wants such things */
            if (strip_8th_bit)
                *namebuf = '_';
        }
        ++namebuf;
    }
}

/* Give object to a random object-liking monster on or adjacent to x,y
   but skipping hero's location.
   If no such monster, place object on floor at x,y. */
static void
give_to_nearby_mon(struct obj *otmp, int x, int y)
{
    struct monst *mtmp;
    struct monst *selected = (struct monst *) 0;
    int nmon = 0, xx, yy;

    for (xx = x - 1; xx <= x + 1; ++xx) {
        for (yy = y - 1; yy <= y + 1; ++yy) {
            if (!isok(xx, yy))
                continue;
            if (u_at(xx, yy))
                continue;
            if (!(mtmp = m_at(xx, yy)))
                continue;
            /* This doesn't do any checks on otmp to see that it matches the
             * likes_* property, intentionally. Assume that the monster is
             * rifling through and taking things that look interesting. */
            if (!(likes_gold(mtmp->data) || likes_gems(mtmp->data)
                  || likes_objs(mtmp->data) || likes_magic(mtmp->data)))
                continue;
            nmon++;
            if (!rn2(nmon))
                selected = mtmp;
        }
    }
    if (selected && can_carry(selected, otmp))
        add_to_minv(selected, otmp);
    else
        place_object(otmp, x, y);
}

/* called by savebones(); also by finish_paybill(shk.c) */
void
drop_upon_death(struct monst *mtmp, /* monster if hero turned into one (other than ghost) */
                struct obj *cont,   /* container if hero is turned into a statue */
                int x, int y)
{
    struct obj *otmp;

    /* when dual-wielding, the second weapon gets dropped rather than welded if
       it becomes cursed; ensure that that won't happen here by ending dual-wield */
    u.twoweap = FALSE; /* bypass set_twoweap() */

    /* all inventory is dropped (for the normal case), even non-droppable things
       like worn armor and accessories, welded weapon, or cursed loadstones */
    while ((otmp = g.invent) != 0) {
        obj_extract_self(otmp);
        /* when turning into green slime, all gear remains held;
           other types "arise from the dead" do aren't holding
           equipment during their brief interval as a corpse */
        if (!mtmp || is_undead(mtmp->data))
            obj_no_longer_held(otmp);

        /* lamps don't go out when dropped */
        if ((cont || artifact_light(otmp)) && obj_is_burning(otmp))
            end_burn(otmp, TRUE); /* smother in statue */
        otmp->owornmask = 0L;

        if (otmp->otyp == SLIME_MOLD)
            goodfruit(otmp->spe);

        if (rn2(5))
            curse(otmp);
        if (mtmp)
            (void) add_to_minv(mtmp, otmp);
        else if (cont)
            (void) add_to_container(cont, otmp);
        else if (!rn2(8))
            give_to_nearby_mon(otmp, x, y);
        else
            place_object(otmp, x, y);
    }
    if (cont)
        cont->owt = weight(cont);
}

/* possibly restore oracle's room and/or put her back inside it; returns
   False if she's on the wrong level and should be removed, True otherwise */
static boolean
fixuporacle(struct monst *oracle)
{
    coord cc;
    int ridx, o_ridx;

    /* oracle doesn't move, but knight's joust or monk's staggering blow
       could push her onto a hole in the floor; at present, traps don't
       activate in such situation hence she won't fall to another level;
       however, that could change so be prepared to cope with such things */
    if (!Is_oracle_level(&u.uz))
        return FALSE;

    oracle->mpeaceful = 1;
    o_ridx = levl[oracle->mx][oracle->my].roomno - ROOMOFFSET;
    if (o_ridx >= 0 && g.rooms[o_ridx].rtype == DELPHI)
        return TRUE; /* no fixup needed */

    /*
     * The Oracle isn't in DELPHI room.  Either hero entered her chamber
     * and got the one-time welcome message, converting it into an
     * ordinary room, or she got teleported out, or both.  Try to put
     * her back inside her room, if necessary, and restore its type.
     */

    /* find original delphi chamber; should always succeed */
    for (ridx = 0; ridx < SIZE(g.rooms); ++ridx)
        if (g.rooms[ridx].orig_rtype == DELPHI)
            break;

    if (o_ridx != ridx && ridx < SIZE(g.rooms)) {
        /* room found and she's not not in it, so try to move her there */
        cc.x = (g.rooms[ridx].lx + g.rooms[ridx].hx) / 2;
        cc.y = (g.rooms[ridx].ly + g.rooms[ridx].hy) / 2;
        if (enexto(&cc, cc.x, cc.y, oracle->data)) {
            rloc_to(oracle, cc.x, cc.y);
            o_ridx = levl[oracle->mx][oracle->my].roomno - ROOMOFFSET;
        }
        /* [if her room is already full, she might end up outside;
           that's ok, next hero just won't get any welcome message,
           same as used to happen before this fixup was introduced] */
    }
    if (ridx == o_ridx) /* if she's in her room, mark it as such */
        g.rooms[ridx].rtype = DELPHI;
    return TRUE; /* keep oracle in new bones file */
}

/* check whether bones are feasible */
boolean
can_make_bones(void)
{
    register struct trap *ttmp;

    if (!flags.bones)
        return FALSE;
    if (ledger_no(&u.uz) <= 0 || ledger_no(&u.uz) > maxledgerno())
        return FALSE;
    if (no_bones_level(&u.uz))
        return FALSE; /* no bones for specific levels */
    if (u.uswallow) {
        return FALSE; /* no bones when swallowed */
    }
    if (!Is_branchlev(&u.uz)) {
        /* no bones on non-branches with portals */
        for (ttmp = g.ftrap; ttmp; ttmp = ttmp->ntrap)
            if (ttmp->ttyp == MAGIC_PORTAL)
                return FALSE;
    }

    if (depth(&u.uz) <= 0                 /* bulletproofing for endgame */
        || (!rn2(1 + (depth(&u.uz) >> 2)) /* fewer ghosts on low levels */
            && !wizard))
        return FALSE;
    /* don't let multiple restarts generate multiple copies of objects
       in bones files */
    if (discover)
        return FALSE;
    return TRUE;
}

/* save bones and possessions of a deceased adventurer */
void
savebones(int how, time_t when, struct obj *corpse)
{
    int x, y;
    struct trap *ttmp;
    struct monst *mtmp;
    struct permonst *mptr;
    struct fruit *f;
    struct cemetery *newbones;
    char c, *bonesid;
    char whynot[BUFSZ];
    NHFILE *nhfp;

    /* caller has already checked `can_make_bones()' */

    clear_bypasses();
    nhfp = open_bonesfile(&u.uz, &bonesid);
    if (nhfp) {
        close_nhfile(nhfp);
        if (wizard) {
            if (yn("Bones file already exists.  Replace it?") == 'y') {
                if (delete_bonesfile(&u.uz))
                    goto make_bones;
                else
                    pline("Cannot unlink old bones.");
            }
        }
        /* compression can change the file's name, so must
           wait until after any attempt to delete this file */
        compress_bonesfile();
        return;
    }

 make_bones:
    unleash_all();
    /* in case these characters are not in their home bases */
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        mptr = mtmp->data;
        if (mtmp->iswiz || mptr == &mons[PM_MEDUSA]
            || mptr->msound == MS_NEMESIS || mptr->msound == MS_LEADER
            || mptr == &mons[PM_VLAD_THE_IMPALER]
            || (mptr == &mons[PM_ORACLE] && !fixuporacle(mtmp)))
            mongone(mtmp);
    }
    if (u.usteed)
        dismount_steed(DISMOUNT_BONES);
    dmonsfree(); /* discard dead or gone monsters */

    /* mark all fruits as nonexistent; when we come to them we'll mark
     * them as existing (using goodfruit())
     */
    for (f = g.ffruit; f; f = f->nextf)
        f->fid = -f->fid;

    /* check iron balls separately--maybe they're not carrying it */
    if (uball)
        uball->owornmask = uchain->owornmask = 0L;

    /* dispose of your possessions, usually cursed */
    if (u.ugrave_arise == (NON_PM - 1)) {
        struct obj *otmp;

        /* embed your possessions in your statue */
        otmp = mk_named_object(STATUE, &mons[u.umonnum], u.ux, u.uy, g.plname);

        drop_upon_death((struct monst *) 0, otmp, u.ux, u.uy);
        if (!otmp)
            return; /* couldn't make statue */
        mtmp = (struct monst *) 0;
    } else if (u.ugrave_arise < LOW_PM) {
        /* drop everything */
        drop_upon_death((struct monst *) 0, (struct obj *) 0, u.ux, u.uy);
        /* trick makemon() into allowing monster creation
         * on your location
         */
        g.in_mklev = TRUE;
        mtmp = makemon(&mons[PM_GHOST], u.ux, u.uy, MM_NONAME);
        g.in_mklev = FALSE;
        if (!mtmp)
            return;
        mtmp = christen_monst(mtmp, g.plname);
        if (corpse)
            (void) obj_attach_mid(corpse, mtmp->m_id);
    } else {
        /* give your possessions to the monster you become */
        g.in_mklev = TRUE; /* use <u.ux,u.uy> as-is */
        mtmp = makemon(&mons[u.ugrave_arise], u.ux, u.uy, NO_MINVENT);
        g.in_mklev = FALSE;
        if (!mtmp) { /* arise-type might have been genocided */
            drop_upon_death((struct monst *) 0, (struct obj *) 0, u.ux, u.uy);
            u.ugrave_arise = NON_PM; /* in case caller cares */
            return;
        }
        mtmp = christen_monst(mtmp, g.plname);
        newsym(u.ux, u.uy);
        /* ["Your body rises from the dead as an <mname>..." used
           to be given here, but it has been moved to done() so that
           it gets delivered even when savebones() isn't called] */
        drop_upon_death(mtmp, (struct obj *) 0, u.ux, u.uy);
        /* 'mtmp' now has hero's inventory; if 'mtmp' is a mummy, give it
           a wrapping unless already carrying one */
        if (mtmp->data->mlet == S_MUMMY && !m_carrying(mtmp, MUMMY_WRAPPING))
            (void) mongets(mtmp, MUMMY_WRAPPING);
        m_dowear(mtmp, TRUE);
    }
    if (mtmp) {
        mtmp->m_lev = (u.ulevel ? u.ulevel : 1);
        mtmp->mhp = mtmp->mhpmax = u.uhpmax;
        mtmp->female = flags.female;
        mtmp->msleeping = 1;
    }
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        resetobjs(mtmp->minvent, FALSE);
        /* do not zero out m_ids for bones levels any more */
        mtmp->mlstmv = 0L;
        if (mtmp->mtame)
            mtmp->mtame = mtmp->mpeaceful = 0;
    }
    for (ttmp = g.ftrap; ttmp; ttmp = ttmp->ntrap) {
        ttmp->madeby_u = 0;
        ttmp->tseen = (ttmp->ttyp == HOLE);
    }
    resetobjs(fobj, FALSE);
    resetobjs(g.level.buriedobjlist, FALSE);

    /* Hero is no longer on the map. */
    u.ux0 = u.ux, u.uy0 = u.uy;
    u.ux = u.uy = 0;

    /* Clear all memory from the level. */
    for (x = 1; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++) {
            levl[x][y].seenv = 0;
            levl[x][y].waslit = 0;
            levl[x][y].glyph = GLYPH_UNEXPLORED;
            g.lastseentyp[x][y] = 0;
        }

    /* Attach bones info to the current level before saving. */
    newbones = (struct cemetery *) alloc(sizeof *newbones);
    /* entries are '\0' terminated but have fixed length allocations,
       so pre-fill with spaces to initialize any excess room */
    (void) memset((genericptr_t) newbones, ' ', sizeof *newbones);
    /* format name+role,&c, death reason, and date+time;
       gender and alignment reflect final values rather than what the
       character started out as, same as topten and logfile entries */
    Sprintf(newbones->who, "%s-%.3s-%.3s-%.3s-%.3s", g.plname, g.urole.filecode,
            g.urace.filecode, genders[flags.female].filecode,
            aligns[1 - u.ualign.type].filecode);
    formatkiller(newbones->how, sizeof newbones->how, how, TRUE);
    Strcpy(newbones->when, yyyymmddhhmmss(when));
    /* final resting place, used to decide when bones are discovered */
    newbones->frpx = u.ux0, newbones->frpy = u.uy0;
    newbones->bonesknown = FALSE;
    /* if current character died on a bones level, the cemetery list
       will have multiple entries, most recent (this dead hero) first */
    newbones->next = g.level.bonesinfo;
    g.level.bonesinfo = newbones;
    /* flag these bones if they are being created in wizard mode;
       they might already be flagged as such, even when we're playing
       in normal mode, if this level came from a previous bones file */
    if (wizard)
        g.level.flags.wizard_bones = 1;

    nhfp = create_bonesfile(&u.uz, &bonesid, whynot);
    if (!nhfp) {
        if (wizard)
            pline1(whynot);
        /* bones file creation problems are silent to the player.
         * Keep it that way, but place a clue into the paniclog.
         */
        paniclog("savebones", whynot);
        return;
    }
    c = (char) (strlen(bonesid) + 1);

    nhfp->mode = WRITING | FREEING;
    store_version(nhfp);
    store_savefileinfo(nhfp);
    if (nhfp->structlevel) {
        /* if a bones pool digit is in use, it precedes the bonesid
           string and isn't recorded in the file */
        bwrite(nhfp->fd, (genericptr_t) &c, sizeof c);
        bwrite(nhfp->fd, (genericptr_t) bonesid, (unsigned) c); /* DD.nn */
        savefruitchn(nhfp);
    }
    update_mlstmv(); /* update monsters for eventual restoration */
    savelev(nhfp, ledger_no(&u.uz));
    close_nhfile(nhfp);
    commit_bonesfile(&u.uz);
    compress_bonesfile();
}

int
getbones(void)
{
    int ok;
    NHFILE *nhfp = (NHFILE *) 0;
    char c = 0, *bonesid, oldbonesid[40]; /* was [10]; more should be safer */

    if (discover) /* save bones files for real games */
        return 0;

    if (!flags.bones)
        return 0;
    /* wizard check added by GAN 02/05/87 */
    if (rn2(3) /* only once in three times do we find bones */
        && !wizard)
        return 0;
    if (no_bones_level(&u.uz))
        return 0;

    nhfp = open_bonesfile(&u.uz, &bonesid);
    if (!nhfp)
        return 0;
    if (nhfp && nhfp->structlevel && nhfp->fd < 0)
        return 0;
    if (nhfp && nhfp->fieldlevel) {
        if (nhfp->style.deflt && !nhfp->fpdef)
        return 0;
    }

    if (validate(nhfp, g.bones) != 0) {
        if (!wizard)
            pline("Discarding unusable bones; no need to panic...");
        ok = FALSE;
    } else {
        ok = TRUE;
        if (wizard) {
            if (yn("Get bones?") == 'n') {
                close_nhfile(nhfp);
                compress_bonesfile();
                return 0;
            }
        }
        if (nhfp->structlevel) {
            /* if a bones pool digit is in use, it precedes the bonesid
               string and wasn't recorded in the file */
            mread(nhfp->fd, (genericptr_t) &c,
                  sizeof c); /* length including terminating '\0' */
            mread(nhfp->fd, (genericptr_t) oldbonesid,
                  (unsigned) c); /* DD.nn or Qrrr.n for role rrr */
        }
        if (strcmp(bonesid, oldbonesid) != 0) {
            char errbuf[BUFSZ];

            Sprintf(errbuf, "This is bones level '%s', not '%s'!",
                    oldbonesid, bonesid);
            if (wizard) {
                pline1(errbuf);
                ok = FALSE; /* won't die of trickery */
            }
            trickery(errbuf);
        } else {
            register struct monst *mtmp;

            getlev(nhfp, 0, 0);

            /* Note that getlev() now keeps tabs on unique
             * monsters such as demon lords, and tracks the
             * birth counts of all species just as makemon()
             * does.  If a bones monster is extinct or has been
             * subject to genocide, their mhpmax will be
             * set to the magic DEFUNCT_MONSTER cookie value.
             */
            for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
                if (has_mgivenname(mtmp))
                    sanitize_name(MGIVENNAME(mtmp));
                if (mtmp->mhpmax == DEFUNCT_MONSTER) {
                    if (wizard) {
                        debugpline1("Removing defunct monster %s from bones.",
                                    mtmp->data->pmnames[NEUTRAL]);
                    }
                    mongone(mtmp);
                } else
                    /* to correctly reset named artifacts on the level */
                    resetobjs(mtmp->minvent, TRUE);
            }
            resetobjs(fobj, TRUE);
            resetobjs(g.level.buriedobjlist, TRUE);
            fix_shop_damage();
        }
    }
    close_nhfile(nhfp);
    sanitize_engravings();
    u.uroleplay.numbones++;

    if (wizard) {
        if (yn("Unlink bones?") == 'n') {
            compress_bonesfile();
            return ok;
        }
    }
    if (!delete_bonesfile(&u.uz)) {
        /* When N games try to simultaneously restore the same
         * bones file, N-1 of them will fail to delete it
         * (the first N-1 under AmigaDOS, the last N-1 under UNIX).
         * So no point in a mysterious message for a normal event
         * -- just generate a new level for those N-1 games.
         */
        /* pline("Cannot unlink bones."); */
        return 0;
    }
    return ok;
}

/* check whether current level contains bones from a particular player */
boolean
bones_include_name(const char *name)
{
    struct cemetery *bp;
    size_t len;
    char buf[BUFSZ];

    /* prepare buffer by appending terminal hyphen to name, to avoid partial
     * matches producing false positives */
    Strcpy(buf, name);
    Strcat(buf, "-");
    len = strlen(buf);

    for (bp = g.level.bonesinfo; bp; bp = bp->next) {
        if (!strncmp(bp->who, buf, len))
            return TRUE;
    }

    return FALSE;
}

/*bones.c*/
