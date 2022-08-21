/* NetHack 3.7	priest.c	$NHDT-Date: 1624322670 2021/06/22 00:44:30 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.70 $ */
/* Copyright (c) Izchak Miller, Steve Linhart, 1989.              */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "mfndpos.h"

/* these match the categorizations shown by enlightenment */
#define ALGN_SINNED (-4) /* worse than strayed (-1..-3) */
#define ALGN_PIOUS 14    /* better than fervent (9..13) */

static boolean histemple_at(struct monst *, coordxy, coordxy);
static boolean has_shrine(struct monst *);

void
newepri(struct monst *mtmp)
{
    if (!mtmp->mextra)
        mtmp->mextra = newmextra();
    if (!EPRI(mtmp)) {
        EPRI(mtmp) = (struct epri *) alloc(sizeof(struct epri));
        (void) memset((genericptr_t) EPRI(mtmp), 0, sizeof(struct epri));
    }
}

void
free_epri(struct monst *mtmp)
{
    if (mtmp->mextra && EPRI(mtmp)) {
        free((genericptr_t) EPRI(mtmp));
        EPRI(mtmp) = (struct epri *) 0;
    }
    mtmp->ispriest = 0;
}

/*
 * Move for priests and shopkeepers.  Called from shk_move() and pri_move().
 * Valid returns are  1: moved  0: didn't  -1: let m_move do it  -2: died.
 */
int
move_special(struct monst *mtmp, boolean in_his_shop, schar appr,
             boolean uondoor, boolean avoid,
             coordxy omx, coordxy omy, coordxy gx, coordxy gy)
{
    register coordxy nx, ny, nix, niy;
    register schar i;
    schar chcnt, cnt;
    coord poss[9];
    long info[9];
    long ninfo = 0;
    long allowflags;
#if 0 /* dead code; see below */
    struct obj *ib = (struct obj *) 0;
#endif

    if (omx == gx && omy == gy)
        return 0;
    if (mtmp->mconf) {
        avoid = FALSE;
        appr = 0;
    }

    nix = omx;
    niy = omy;
    allowflags = mon_allowflags(mtmp);
    cnt = mfndpos(mtmp, poss, info, allowflags);

    if (mtmp->isshk && avoid && uondoor) { /* perhaps we cannot avoid him */
        for (i = 0; i < cnt; i++)
            if (!(info[i] & NOTONL))
                goto pick_move;
        avoid = FALSE;
    }

#define GDIST(x, y) (dist2(x, y, gx, gy))
 pick_move:
    chcnt = 0;
    for (i = 0; i < cnt; i++) {
        nx = poss[i].x;
        ny = poss[i].y;
        if (IS_ROOM(levl[nx][ny].typ)
            || (mtmp->isshk && (!in_his_shop || ESHK(mtmp)->following))) {
            if (avoid && (info[i] & NOTONL) && !(info[i] & ALLOW_M))
                continue;
            if ((!appr && !rn2(++chcnt))
                || (appr && GDIST(nx, ny) < GDIST(nix, niy))
                || (info[i] & ALLOW_M)) {
                nix = nx;
                niy = ny;
                ninfo = info[i];
            }
        }
    }
    if (mtmp->ispriest && avoid && nix == omx && niy == omy
        && onlineu(omx, omy)) {
        /* might as well move closer as long it's going to stay
         * lined up */
        avoid = FALSE;
        goto pick_move;
    }

    if (nix != omx || niy != omy) {

        if (ninfo & ALLOW_M) {
            /* mtmp is deciding it would like to attack this turn.
             * Returns from m_move_aggress don't correspond to the same things
             * as this function should return, so we need to translate. */
            switch (m_move_aggress(mtmp, nix, niy)) {
            case 2:
                return -2; /* died making the attack */
            case 3:
                return 1; /* attacked and spent this move */
            }
        }

        if (MON_AT(nix, niy))
            return 0;
        remove_monster(omx, omy);
        place_monster(mtmp, nix, niy);
        newsym(nix, niy);
        if (mtmp->isshk && !in_his_shop && inhishop(mtmp))
            check_special_room(FALSE);
#if 0 /* dead code; maybe someday someone will track down why... */
        if (ib) {
            if (cansee(mtmp->mx, mtmp->my))
                pline("%s picks up %s.", Monnam(mtmp),
                      distant_name(ib, doname));
            obj_extract_self(ib);
            (void) mpickobj(mtmp, ib);
        }
#endif
        return 1;
    }
    return 0;
}

char
temple_occupied(char *array)
{
    register char *ptr;

    for (ptr = array; *ptr; ptr++)
        if (g.rooms[*ptr - ROOMOFFSET].rtype == TEMPLE)
            return *ptr;
    return '\0';
}

static boolean
histemple_at(struct monst *priest, coordxy x, coordxy y)
{
    return (boolean) (priest && priest->ispriest
                      && (EPRI(priest)->shroom == *in_rooms(x, y, TEMPLE))
                      && on_level(&(EPRI(priest)->shrlevel), &u.uz));
}

boolean
inhistemple(struct monst *priest)
{
    /* make sure we have a priest */
    if (!priest || !priest->ispriest)
        return FALSE;
    /* priest must be on right level and in right room */
    if (!histemple_at(priest, priest->mx, priest->my))
        return FALSE;
    /* temple room must still contain properly aligned altar */
    return has_shrine(priest);
}

/*
 * pri_move: return 1: moved  0: didn't  -1: let m_move do it  -2: died
 */
int
pri_move(struct monst *priest)
{
    register coordxy gx, gy, omx, omy;
    schar temple;
    boolean avoid = TRUE;

    omx = priest->mx;
    omy = priest->my;

    if (!histemple_at(priest, omx, omy))
        return -1;

    temple = EPRI(priest)->shroom;

    gx = EPRI(priest)->shrpos.x;
    gy = EPRI(priest)->shrpos.y;

    gx += rn1(3, -1); /* mill around the altar */
    gy += rn1(3, -1);

    if (!priest->mpeaceful
        || (Conflict && !resist_conflict(priest))) {
        if (monnear(priest, u.ux, u.uy)) {
            if (Displaced)
                Your("displaced image doesn't fool %s!", mon_nam(priest));
            (void) mattacku(priest);
            return 0;
        } else if (index(u.urooms, temple)) {
            /* chase player if inside temple & can see him */
            if (priest->mcansee && m_canseeu(priest)) {
                gx = u.ux;
                gy = u.uy;
            }
            avoid = FALSE;
        }
    } else if (Invis)
        avoid = FALSE;

    return move_special(priest, FALSE, TRUE, FALSE, avoid, omx, omy, gx, gy);
}

/* exclusively for mktemple() */
void
priestini(
    d_level *lvl,
    struct mkroom *sroom,
    int sx, int sy,
    boolean sanctum) /* is it the seat of the high priest? */
{
    struct monst *priest;
    struct obj *otmp;
    int cnt;
    int px = 0, py = 0, i, si = rn2(N_DIRS);
    struct permonst *prim = &mons[sanctum ? PM_HIGH_CLERIC
                                          : PM_ALIGNED_CLERIC];

    for (i = 0; i < N_DIRS; i++) {
        px = sx + xdir[DIR_CLAMP(i+si)];
        py = sy + ydir[DIR_CLAMP(i+si)];
        if (pm_good_location(px, py, prim))
            break;
    }
    if (i == N_DIRS)
        px = sx, py = sy;

    if (MON_AT(px, py))
        (void) rloc(m_at(px, py), RLOC_NOMSG); /* insurance */

    priest = makemon(prim, px, py, MM_EPRI);
    if (priest) {
        EPRI(priest)->shroom = (schar) ((sroom - g.rooms) + ROOMOFFSET);
        EPRI(priest)->shralign = Amask2align(levl[sx][sy].altarmask);
        EPRI(priest)->shrpos.x = sx;
        EPRI(priest)->shrpos.y = sy;
        assign_level(&(EPRI(priest)->shrlevel), lvl);
        mon_learns_traps(priest, ALL_TRAPS); /* traps are known */
        priest->mpeaceful = 1;
        priest->ispriest = 1;
        priest->isminion = 0;
        priest->msleeping = 0;
        set_malign(priest); /* mpeaceful may have changed */

        /* now his/her goodies... */
        if (sanctum && EPRI(priest)->shralign == A_NONE
            && on_level(&sanctum_level, &u.uz)) {
            (void) mongets(priest, AMULET_OF_YENDOR);
        }
        /* 2 to 4 spellbooks */
        for (cnt = rn1(3, 2); cnt > 0; --cnt) {
            (void) mpickobj(priest, mkobj(SPBOOK_no_NOVEL, FALSE));
        }
        /* robe [via makemon()] */
        if (rn2(2) && (otmp = which_armor(priest, W_ARMC)) != 0) {
            if (p_coaligned(priest))
                uncurse(otmp);
            else
                curse(otmp);
        }
    }
}

/* get a monster's alignment type without caller needing EPRI & EMIN */
aligntyp
mon_aligntyp(struct monst *mon)
{
    aligntyp algn = mon->ispriest ? EPRI(mon)->shralign
                                  : mon->isminion ? EMIN(mon)->min_align
                                                  : mon->data->maligntyp;

    if (algn == A_NONE)
        return A_NONE; /* negative but differs from chaotic */
    return (algn > 0) ? A_LAWFUL : (algn < 0) ? A_CHAOTIC : A_NEUTRAL;
}

/*
 * Specially aligned monsters are named specially.
 *      - aligned priests with ispriest and high priests have shrines
 *              they retain ispriest and epri when polymorphed
 *      - aligned priests without ispriest are roamers
 *              they have isminion set and use emin rather than epri
 *      - minions do not have ispriest but have isminion and emin
 *      - caller needs to inhibit Hallucination if it wants to force
 *              the true name even when under that influence
 */
char *
priestname(
    struct monst *mon,
    int article,
    char *pname) /* caller-supplied output buffer */
{
    boolean do_hallu = Hallucination,
            aligned_priest = mon->data == &mons[PM_ALIGNED_CLERIC],
            high_priest = mon->data == &mons[PM_HIGH_CLERIC];
    char whatcode = '\0';
    const char *what = do_hallu ? rndmonnam(&whatcode) : mon_pmname(mon);

    if (!mon->ispriest && !mon->isminion) /* should never happen...  */
        return strcpy(pname, what);       /* caller must be confused */

    *pname = '\0';
    if (article != ARTICLE_NONE && (!do_hallu || !bogon_is_pname(whatcode))) {
        if (article == ARTICLE_YOUR || (article == ARTICLE_A && high_priest))
            article = ARTICLE_THE;
        if (article == ARTICLE_THE) {
            Strcat(pname, "the ");
        } else {
            char buf2[BUFSZ] = DUMMY;

            /* don't let "Angel of <foo>" fool an() into using "the " */
            Strcpy(buf2, pname);
            *buf2 = lowc(*buf2);
            (void) just_an(pname, buf2);
        }
    }
    /* pname[] contains "" or {"a ","an ","the "} */
    if (mon->minvis) {
        /* avoid "a invisible priest" */
        if (!strcmp(pname, "a "))
            Strcpy(pname, "an ");
        Strcat(pname, "invisible ");
    }
    if (mon->isminion && EMIN(mon)->renegade) {
        /* avoid "an renegade Angel" */
        if (!strcmp(pname, "an ")) /* will fail for "an invisible " */
            Strcpy(pname, "a ");
        Strcat(pname, "renegade ");
    }

    if (mon->ispriest || aligned_priest) { /* high_priest implies ispriest */
        if (!aligned_priest && !high_priest) {
            ; /* polymorphed priest; use ``what'' as is */
        } else {
            if (high_priest)
                Strcat(pname, "high ");
            if (Hallucination)
                what = "poohbah";
            else if (mon->female)
                what = "priestess";
            else
                what = "priest";
        }
    } else {
        if (mon->mtame && !strcmpi(what, "Angel"))
            Strcat(pname, "guardian ");
    }

    Strcat(pname, what);
    /* same as distant_monnam(), more or less... */
    if (do_hallu || !high_priest || !Is_astralevel(&u.uz)
        || next2u(mon->mx, mon->my) || g.program_state.gameover) {
        Strcat(pname, " of ");
        Strcat(pname, halu_gname(mon_aligntyp(mon)));
    }
    return pname;
}

boolean
p_coaligned(struct monst *priest)
{
    return (boolean) (u.ualign.type == mon_aligntyp(priest));
}

static boolean
has_shrine(struct monst *pri)
{
    struct rm *lev;
    struct epri *epri_p;

    if (!pri || !pri->ispriest)
        return FALSE;
    epri_p = EPRI(pri);
    lev = &levl[epri_p->shrpos.x][epri_p->shrpos.y];
    if (!IS_ALTAR(lev->typ) || !(lev->altarmask & AM_SHRINE))
        return FALSE;
    return (boolean) (epri_p->shralign
                      == (Amask2align(lev->altarmask & ~AM_SHRINE)));
}

struct monst *
findpriest(char roomno)
{
    register struct monst *mtmp;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        if (mtmp->ispriest && (EPRI(mtmp)->shroom == roomno)
            && histemple_at(mtmp, mtmp->mx, mtmp->my))
            return mtmp;
    }
    return (struct monst *) 0;
}

DISABLE_WARNING_FORMAT_NONLITERAL

/* called from check_special_room() when the player enters the temple room */
void
intemple(int roomno)
{
    struct monst *priest, *mtmp;
    struct epri *epri_p;
    boolean shrined, sanctum, can_speak;
    long *this_time, *other_time;
    const char *msg1, *msg2;
    char buf[BUFSZ];

    /* don't do anything if hero is already in the room */
    if (temple_occupied(u.urooms0))
        return;

    if ((priest = findpriest((char) roomno)) != 0) {
        /* tended */
        record_achievement(ACH_TMPL);

        epri_p = EPRI(priest);
        shrined = has_shrine(priest);
        sanctum = (priest->data == &mons[PM_HIGH_CLERIC]
                   && (Is_sanctum(&u.uz) || In_endgame(&u.uz)));
        can_speak = !helpless(priest);
        if (can_speak && !Deaf && g.moves >= epri_p->intone_time) {
            unsigned save_priest = priest->ispriest;

            /* don't reveal the altar's owner upon temple entry in
               the endgame; for the Sanctum, the next message names
               Moloch so suppress the "of Moloch" for him here too */
            if (sanctum && !Hallucination)
                priest->ispriest = 0;
            pline("%s intones:",
                  canseemon(priest) ? Monnam(priest) : "A nearby voice");
            priest->ispriest = save_priest;
            epri_p->intone_time = g.moves + (long) d(10, 500); /* ~2505 */
            /* make sure that we don't suppress entry message when
               we've just given its "priest intones" introduction */
            epri_p->enter_time = 0L;
        }
        msg1 = msg2 = 0;
        if (sanctum && Is_sanctum(&u.uz)) {
            if (priest->mpeaceful) {
                /* first time inside */
                msg1 = "Infidel, you have entered Moloch's Sanctum!";
                msg2 = "Be gone!";
                priest->mpeaceful = 0;
                /* became angry voluntarily; no penalty for attacking him */
                set_malign(priest);
            } else {
                /* repeat visit, or attacked priest before entering */
                msg1 = "You desecrate this place by your presence!";
            }
        } else if (g.moves >= epri_p->enter_time) {
            Sprintf(buf, "Pilgrim, you enter a %s place!",
                    !shrined ? "desecrated" : "sacred");
            msg1 = buf;
        }
        if (msg1 && can_speak && !Deaf) {
            verbalize1(msg1);
            if (msg2)
                verbalize1(msg2);
            epri_p->enter_time = g.moves + (long) d(10, 100); /* ~505 */
        }
        if (!sanctum) {
            if (!shrined || !p_coaligned(priest)
                || u.ualign.record <= ALGN_SINNED) {
                msg1 = "have a%s forbidding feeling...";
                msg2 = (!shrined || !p_coaligned(priest)) ? "" : " strange";
                this_time = &epri_p->hostile_time;
                other_time = &epri_p->peaceful_time;
            } else {
                msg1 = "experience %s sense of peace.";
                msg2 = (u.ualign.record >= ALGN_PIOUS) ? "a" : "an unusual";
                this_time = &epri_p->peaceful_time;
                other_time = &epri_p->hostile_time;
            }
            /* give message if we haven't seen it recently or
               if alignment update has caused it to switch from
               forbidding to sense-of-peace or vice versa */
            if (g.moves >= *this_time || *other_time >= *this_time) {
                You(msg1, msg2);
                *this_time = g.moves + (long) d(10, 20); /* ~55 */
                /* avoid being tricked by the RNG:  switch might have just
                   happened and previous random threshold could be larger */
                if (*this_time <= *other_time)
                    *other_time = *this_time - 1L;
            }
        }
        /* recognize the Valley of the Dead and Moloch's Sanctum
           once hero has encountered the temple priest on those levels */
        mapseen_temple(priest);
    } else {
        /* untended */

        switch (rn2(4)) {
        case 0:
            You("have an eerie feeling...");
            break;
        case 1:
            You_feel("like you are being watched.");
            break;
        case 2:
            pline("A shiver runs down your %s.", body_part(SPINE));
            break;
        default:
            break; /* no message; unfortunately there's no
                      EPRI(priest)->eerie_time available to
                      make sure we give one the first time */
        }
        if (!rn2(5)
            && (mtmp = makemon(&mons[PM_GHOST], u.ux, u.uy, MM_NOMSG))
                   != 0) {
            int ngen = g.mvitals[PM_GHOST].born;
            if (canspotmon(mtmp))
                pline("A%s ghost appears next to you%c",
                      ngen < 5 ? "n enormous" : "",
                      ngen < 10 ? '!' : '.');
            else
                You("sense a presence close by!");
            mtmp->mpeaceful = 0;
            set_malign(mtmp);
            if (Verbose(3, intemple))
                You("are frightened to death, and unable to move.");
            nomul(-3);
            g.multi_reason = "being terrified of a ghost";
            g.nomovemsg = "You regain your composure.";
        }
    }
}

RESTORE_WARNING_FORMAT_NONLITERAL

/* reset the move counters used to limit temple entry feedback;
   leaving the level and then returning yields a fresh start */
void
forget_temple_entry(struct monst *priest)
{
    struct epri *epri_p = priest->ispriest ? EPRI(priest) : 0;

    if (!epri_p) {
        impossible("attempting to manipulate shrine data for non-priest?");
        return;
    }
    epri_p->intone_time = epri_p->enter_time = epri_p->peaceful_time =
        epri_p->hostile_time = 0L;
}

void
priest_talk(struct monst *priest)
{
    boolean coaligned = p_coaligned(priest);
    boolean strayed = (u.ualign.record < 0);

    /* KMH, conduct */
    if (!u.uconduct.gnostic++)
        livelog_printf(LL_CONDUCT,
                       "rejected atheism by consulting with %s",
                       mon_nam(priest));

    if (priest->mflee || (!priest->ispriest && coaligned && strayed)) {
        pline("%s doesn't want anything to do with you!", Monnam(priest));
        priest->mpeaceful = 0;
        return;
    }

    /* priests don't chat unless peaceful and in their own temple */
    if (!inhistemple(priest) || !priest->mpeaceful || helpless(priest)) {
        static const char *const cranky_msg[3] = {
            "Thou wouldst have words, eh?  I'll give thee a word or two!",
            "Talk?  Here is what I have to say!",
            "Pilgrim, I would speak no longer with thee."
        };

        if (helpless(priest)) {
            pline("%s breaks out of %s reverie!", Monnam(priest),
                  mhis(priest));
            priest->mfrozen = priest->msleeping = 0;
            priest->mcanmove = 1;
        }
        priest->mpeaceful = 0;
        verbalize1(cranky_msg[rn2(3)]);
        return;
    }

    /* you desecrated the temple and now you want to chat? */
    if (priest->mpeaceful && *in_rooms(priest->mx, priest->my, TEMPLE)
        && !has_shrine(priest)) {
        verbalize(
              "Begone!  Thou desecratest this holy place with thy presence.");
        priest->mpeaceful = 0;
        return;
    }
    if (!money_cnt(g.invent)) {
        if (coaligned && !strayed) {
            long pmoney = money_cnt(priest->minvent);
            if (pmoney > 0L) {
                const char *bits;
                bits = (Hallucination) ? currency(pmoney)
                                       : (pmoney == 1L) ? "bit" : "bits";
                /* Note: two bits is actually 25 cents.  Hmm. */
                pline("%s gives you %s%s for an ale.", Monnam(priest),
                      (pmoney == 1L) ? "one " : "two ", bits);
                money2u(priest, pmoney > 1L ? 2 : 1);
            } else
                pline("%s preaches the virtues of poverty.", Monnam(priest));
            exercise(A_WIS, TRUE);
        } else
            pline("%s is not interested.", Monnam(priest));
        return;
    } else {
        long offer;

        pline("%s asks you for a contribution for the temple.",
              Monnam(priest));
        if ((offer = bribe(priest)) == 0) {
            verbalize("Thou shalt regret thine action!");
            if (coaligned)
                adjalign(-1);
        } else if (offer < (u.ulevel * 200)) {
            if (money_cnt(g.invent) > (offer * 2L)) {
                verbalize("Cheapskate.");
            } else {
                verbalize("I thank thee for thy contribution.");
                /* give player some token */
                exercise(A_WIS, TRUE);
            }
        } else if (offer < (u.ulevel * 400)) {
            verbalize("Thou art indeed a pious individual.");
            if (money_cnt(g.invent) < (offer * 2L)) {
                if (coaligned && u.ualign.record <= ALGN_SINNED)
                    adjalign(1);
                verbalize("I bestow upon thee a blessing.");
                incr_itimeout(&HClairvoyant, rn1(500, 500));
            }
        } else if (offer < (u.ulevel * 600)
                   /* u.ublessed is only active when Protection is
                      enabled via something other than worn gear
                      (theft by gremlin clears the intrinsic but not
                      its former magnitude, making it recoverable) */
                   && (!(HProtection & INTRINSIC)
                       || (u.ublessed < 20
                           && (u.ublessed < 9 || !rn2(u.ublessed))))) {
            verbalize("Thou hast been rewarded for thy devotion.");
            if (!(HProtection & INTRINSIC)) {
                HProtection |= FROMOUTSIDE;
                if (!u.ublessed)
                    u.ublessed = rn1(3, 2);
            } else
                u.ublessed++;
        } else {
            verbalize("Thy selfless generosity is deeply appreciated.");
            if (money_cnt(g.invent) < (offer * 2L) && coaligned) {
                if (strayed && (g.moves - u.ucleansed) > 5000L) {
                    u.ualign.record = 0; /* cleanse thee */
                    u.ucleansed = g.moves;
                } else {
                    adjalign(2);
                }
            }
        }
    }
}

struct monst *
mk_roamer(struct permonst *ptr, aligntyp alignment, coordxy x, coordxy y,
          boolean peaceful)
{
    register struct monst *roamer;
    register boolean coaligned = (u.ualign.type == alignment);

#if 0 /* this was due to permonst's pxlth field which is now gone */
    if (ptr != &mons[PM_ALIGNED_CLERIC] && ptr != &mons[PM_ANGEL])
        return (struct monst *) 0;
#endif

    if (MON_AT(x, y))
        (void) rloc(m_at(x, y), RLOC_NOMSG); /* insurance */

    if (!(roamer = makemon(ptr, x, y, MM_ADJACENTOK | MM_EMIN | MM_NOMSG)))
        return (struct monst *) 0;

    EMIN(roamer)->min_align = alignment;
    EMIN(roamer)->renegade = (coaligned && !peaceful);
    roamer->ispriest = 0;
    roamer->isminion = 1;
    mon_learns_traps(roamer, ALL_TRAPS); /* traps are known */
    roamer->mpeaceful = peaceful;
    roamer->msleeping = 0;
    set_malign(roamer); /* peaceful may have changed */

    /* MORE TO COME */
    return roamer;
}

void
reset_hostility(struct monst *roamer)
{
    if (!roamer->isminion)
        return;
    if (roamer->data != &mons[PM_ALIGNED_CLERIC]
        && roamer->data != &mons[PM_ANGEL])
        return;

    if (EMIN(roamer)->min_align != u.ualign.type) {
        roamer->mpeaceful = roamer->mtame = 0;
        set_malign(roamer);
    }
    newsym(roamer->mx, roamer->my);
}

boolean
in_your_sanctuary(
    struct monst *mon, /* if non-null, <mx,my> overrides <x,y> */
    coordxy x, coordxy y)
{
    register char roomno;
    register struct monst *priest;

    if (mon) {
        if (is_minion(mon->data) || is_rider(mon->data))
            return FALSE;
        x = mon->mx, y = mon->my;
    }
    if (u.ualign.record <= ALGN_SINNED) /* sinned or worse */
        return FALSE;
    if ((roomno = temple_occupied(u.urooms)) == 0
        || roomno != *in_rooms(x, y, TEMPLE))
        return FALSE;
    if ((priest = findpriest(roomno)) == 0)
        return FALSE;
    return (boolean) (has_shrine(priest) && p_coaligned(priest)
                      && priest->mpeaceful);
}

/* when attacking "priest" in his temple */
void
ghod_hitsu(struct monst *priest)
{
    coordxy x, y, ax, ay;
    int roomno = (int) temple_occupied(u.urooms);
    struct mkroom *troom;

    if (!roomno || !has_shrine(priest))
        return;

    ax = x = EPRI(priest)->shrpos.x;
    ay = y = EPRI(priest)->shrpos.y;
    troom = &g.rooms[roomno - ROOMOFFSET];

    if (u_at(x, y) || !linedup(u.ux, u.uy, x, y, 1)) {
        if (IS_DOOR(levl[u.ux][u.uy].typ)) {
            if (u.ux == troom->lx - 1) {
                x = troom->hx;
                y = u.uy;
            } else if (u.ux == troom->hx + 1) {
                x = troom->lx;
                y = u.uy;
            } else if (u.uy == troom->ly - 1) {
                x = u.ux;
                y = troom->hy;
            } else if (u.uy == troom->hy + 1) {
                x = u.ux;
                y = troom->ly;
            }
        } else {
            switch (rn2(4)) {
            case 0:
                x = u.ux;
                y = troom->ly;
                break;
            case 1:
                x = u.ux;
                y = troom->hy;
                break;
            case 2:
                x = troom->lx;
                y = u.uy;
                break;
            default:
                x = troom->hx;
                y = u.uy;
                break;
            }
        }
        if (!linedup(u.ux, u.uy, x, y, 1))
            return;
    }

    switch (rn2(3)) {
    case 0:
        pline("%s roars in anger:  \"Thou shalt suffer!\"",
              a_gname_at(ax, ay));
        break;
    case 1:
        pline("%s voice booms:  \"How darest thou harm my servant!\"",
              s_suffix(a_gname_at(ax, ay)));
        break;
    default:
        pline("%s roars:  \"Thou dost profane my shrine!\"",
              a_gname_at(ax, ay));
        break;
    }

    buzz(BZ_M_SPELL(BZ_OFS_AD(AD_ELEC)), 6, x, y, sgn(g.tbx),
         sgn(g.tby)); /* bolt of lightning */
    exercise(A_WIS, FALSE);
}

void
angry_priest(void)
{
    register struct monst *priest;
    struct rm *lev;

    if ((priest = findpriest(temple_occupied(u.urooms))) != 0) {
        struct epri *eprip = EPRI(priest);

        wakeup(priest, FALSE);
        setmangry(priest, FALSE);
        /*
         * If the altar has been destroyed or converted, let the
         * priest run loose.
         * (When it's just a conversion and there happens to be
         * a fresh corpse nearby, the priest ought to have an
         * opportunity to try converting it back; maybe someday...)
         */
        lev = &levl[eprip->shrpos.x][eprip->shrpos.y];
        if (!IS_ALTAR(lev->typ)
            || ((aligntyp) Amask2align(lev->altarmask & AM_MASK)
                != eprip->shralign)) {
            if (!EMIN(priest))
                newemin(priest);
            priest->ispriest = 0; /* now a roaming minion */
            priest->isminion = 1;
            EMIN(priest)->min_align = eprip->shralign;
            EMIN(priest)->renegade = FALSE;
            /* discard priest's memory of his former shrine;
               if we ever implement the re-conversion mentioned
               above, this will need to be removed */
            free_epri(priest);
        }
    }
}

/*
 * When saving bones, find priests that aren't on their shrine level,
 * and remove them.  This avoids big problems when restoring bones.
 * [Perhaps we should convert them into roamers instead?]
 */
void
clearpriests(void)
{
    struct monst *mtmp;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        if (mtmp->ispriest && !on_level(&(EPRI(mtmp)->shrlevel), &u.uz))
            mongone(mtmp);
    }
}

/* munge priest-specific structure when restoring -dlc */
void
restpriest(struct monst *mtmp, boolean ghostly)
{
    if (u.uz.dlevel) {
        if (ghostly)
            assign_level(&(EPRI(mtmp)->shrlevel), &u.uz);
    }
}

/*priest.c*/
