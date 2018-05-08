/* NetHack 3.6	priest.c	$NHDT-Date: 1501725407 2017/08/03 01:56:47 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.44 $ */
/* Copyright (c) Izchak Miller, Steve Linhart, 1989.              */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "mfndpos.h"

/* these match the categorizations shown by enlightenment */
#define ALGN_SINNED (-4) /* worse than strayed (-1..-3) */
#define ALGN_PIOUS 14    /* better than fervent (9..13) */

STATIC_DCL boolean FDECL(histemple_at, (struct monst *, XCHAR_P, XCHAR_P));
STATIC_DCL boolean FDECL(has_shrine, (struct monst *));

void
newepri(mtmp)
struct monst *mtmp;
{
    if (!mtmp->mextra)
        mtmp->mextra = newmextra();
    if (!EPRI(mtmp)) {
        EPRI(mtmp) = (struct epri *) alloc(sizeof(struct epri));
        (void) memset((genericptr_t) EPRI(mtmp), 0, sizeof(struct epri));
    }
}

void
free_epri(mtmp)
struct monst *mtmp;
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
move_special(mtmp, in_his_shop, appr, uondoor, avoid, omx, omy, gx, gy)
register struct monst *mtmp;
boolean in_his_shop;
schar appr;
boolean uondoor, avoid;
register xchar omx, omy, gx, gy;
{
    register xchar nx, ny, nix, niy;
    register schar i;
    schar chcnt, cnt;
    coord poss[9];
    long info[9];
    long allowflags;
    struct obj *ib = (struct obj *) 0;

    if (omx == gx && omy == gy)
        return 0;
    if (mtmp->mconf) {
        avoid = FALSE;
        appr = 0;
    }

    nix = omx;
    niy = omy;
    if (mtmp->isshk)
        allowflags = ALLOW_SSM;
    else
        allowflags = ALLOW_SSM | ALLOW_SANCT;
    if (passes_walls(mtmp->data))
        allowflags |= (ALLOW_ROCK | ALLOW_WALL);
    if (throws_rocks(mtmp->data))
        allowflags |= ALLOW_ROCK;
    if (tunnels(mtmp->data))
        allowflags |= ALLOW_DIG;
    if (!nohands(mtmp->data) && !verysmall(mtmp->data)) {
        allowflags |= OPENDOOR;
        if (monhaskey(mtmp, TRUE))
            allowflags |= UNLOCKDOOR;
    }
    if (is_giant(mtmp->data))
        allowflags |= BUSTDOOR;
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
            if (avoid && (info[i] & NOTONL))
                continue;
            if ((!appr && !rn2(++chcnt))
                || (appr && GDIST(nx, ny) < GDIST(nix, niy))) {
                nix = nx;
                niy = ny;
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
        remove_monster(omx, omy);
        place_monster(mtmp, nix, niy);
        newsym(nix, niy);
        if (mtmp->isshk && !in_his_shop && inhishop(mtmp))
            check_special_room(FALSE);
        if (ib) {
            if (cansee(mtmp->mx, mtmp->my))
                pline("%s picks up %s.", Monnam(mtmp),
                      distant_name(ib, doname));
            obj_extract_self(ib);
            (void) mpickobj(mtmp, ib);
        }
        return 1;
    }
    return 0;
}

char
temple_occupied(array)
register char *array;
{
    register char *ptr;

    for (ptr = array; *ptr; ptr++)
        if (rooms[*ptr - ROOMOFFSET].rtype == TEMPLE)
            return *ptr;
    return '\0';
}

STATIC_OVL boolean
histemple_at(priest, x, y)
register struct monst *priest;
register xchar x, y;
{
    return (boolean) (priest && priest->ispriest
                      && (EPRI(priest)->shroom == *in_rooms(x, y, TEMPLE))
                      && on_level(&(EPRI(priest)->shrlevel), &u.uz));
}

boolean
inhistemple(priest)
struct monst *priest;
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
pri_move(priest)
register struct monst *priest;
{
    register xchar gx, gy, omx, omy;
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
        || (Conflict && !resist(priest, RING_CLASS, 0, 0))) {
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
priestini(lvl, sroom, sx, sy, sanctum)
d_level *lvl;
struct mkroom *sroom;
int sx, sy;
boolean sanctum; /* is it the seat of the high priest? */
{
    struct monst *priest;
    struct obj *otmp;
    int cnt;

    if (MON_AT(sx + 1, sy))
        (void) rloc(m_at(sx + 1, sy), FALSE); /* insurance */

    priest = makemon(&mons[sanctum ? PM_HIGH_PRIEST : PM_ALIGNED_PRIEST],
                     sx + 1, sy, MM_EPRI);
    if (priest) {
        EPRI(priest)->shroom = (schar) ((sroom - rooms) + ROOMOFFSET);
        EPRI(priest)->shralign = Amask2align(levl[sx][sy].altarmask);
        EPRI(priest)->shrpos.x = sx;
        EPRI(priest)->shrpos.y = sy;
        assign_level(&(EPRI(priest)->shrlevel), lvl);
        priest->mtrapseen = ~0; /* traps are known */
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
            (void) mpickobj(priest, mkobj(SPBOOK_CLASS, FALSE));
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
mon_aligntyp(mon)
struct monst *mon;
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
priestname(mon, pname)
register struct monst *mon;
char *pname; /* caller-supplied output buffer */
{
    boolean do_hallu = Hallucination,
            aligned_priest = mon->data == &mons[PM_ALIGNED_PRIEST],
            high_priest = mon->data == &mons[PM_HIGH_PRIEST];
    char whatcode = '\0';
    const char *what = do_hallu ? rndmonnam(&whatcode) : mon->data->mname;

    if (!mon->ispriest && !mon->isminion) /* should never happen...  */
        return strcpy(pname, what);       /* caller must be confused */

    *pname = '\0';
    if (!do_hallu || !bogon_is_pname(whatcode))
        Strcat(pname, "the ");
    if (mon->minvis)
        Strcat(pname, "invisible ");
    if (mon->isminion && EMIN(mon)->renegade)
        Strcat(pname, "renegade ");

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
        || distu(mon->mx, mon->my) <= 2 || program_state.gameover) {
        Strcat(pname, " of ");
        Strcat(pname, halu_gname(mon_aligntyp(mon)));
    }
    return pname;
}

boolean
p_coaligned(priest)
struct monst *priest;
{
    return (boolean) (u.ualign.type == mon_aligntyp(priest));
}

STATIC_OVL boolean
has_shrine(pri)
struct monst *pri;
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
findpriest(roomno)
char roomno;
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

/* called from check_special_room() when the player enters the temple room */
void
intemple(roomno)
int roomno;
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

        epri_p = EPRI(priest);
        shrined = has_shrine(priest);
        sanctum = (priest->data == &mons[PM_HIGH_PRIEST]
                   && (Is_sanctum(&u.uz) || In_endgame(&u.uz)));
        can_speak = (priest->mcanmove && !priest->msleeping);
        if (can_speak && !Deaf && moves >= epri_p->intone_time) {
            unsigned save_priest = priest->ispriest;

            /* don't reveal the altar's owner upon temple entry in
               the endgame; for the Sanctum, the next message names
               Moloch so suppress the "of Moloch" for him here too */
            if (sanctum && !Hallucination)
                priest->ispriest = 0;
            pline("%s intones:",
                  canseemon(priest) ? Monnam(priest) : "A nearby voice");
            priest->ispriest = save_priest;
            epri_p->intone_time = moves + (long) d(10, 500); /* ~2505 */
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
        } else if (moves >= epri_p->enter_time) {
            Sprintf(buf, "Pilgrim, you enter a %s place!",
                    !shrined ? "desecrated" : "sacred");
            msg1 = buf;
        }
        if (msg1 && can_speak && !Deaf) {
            verbalize1(msg1);
            if (msg2)
                verbalize1(msg2);
            epri_p->enter_time = moves + (long) d(10, 100); /* ~505 */
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
            if (moves >= *this_time || *other_time >= *this_time) {
                You(msg1, msg2);
                *this_time = moves + (long) d(10, 20); /* ~55 */
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
            && (mtmp = makemon(&mons[PM_GHOST], u.ux, u.uy, NO_MM_FLAGS))
                   != 0) {
            int ngen = mvitals[PM_GHOST].born;
            if (canspotmon(mtmp))
                pline("A%s ghost appears next to you%c",
                      ngen < 5 ? "n enormous" : "",
                      ngen < 10 ? '!' : '.');
            else
                You("sense a presence close by!");
            mtmp->mpeaceful = 0;
            set_malign(mtmp);
            if (flags.verbose)
                You("are frightened to death, and unable to move.");
            nomul(-3);
            multi_reason = "being terrified of a ghost";
            nomovemsg = "You regain your composure.";
        }
    }
}

/* reset the move counters used to limit temple entry feedback;
   leaving the level and then returning yields a fresh start */
void
forget_temple_entry(priest)
struct monst *priest;
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
priest_talk(priest)
register struct monst *priest;
{
    boolean coaligned = p_coaligned(priest);
    boolean strayed = (u.ualign.record < 0);

    /* KMH, conduct */
    if(!u.uconduct.gnostic++)
        livelog_printf(LL_CONDUCT,
               "rejected atheism by consulting with %s",
               mon_nam(priest));

    if (priest->mflee || (!priest->ispriest && coaligned && strayed)) {
        pline("%s doesn't want anything to do with you!", Monnam(priest));
        priest->mpeaceful = 0;
        return;
    }

    /* priests don't chat unless peaceful and in their own temple */
    if (!inhistemple(priest) || !priest->mpeaceful
        || !priest->mcanmove || priest->msleeping) {
        static const char *cranky_msg[3] = {
            "Thou wouldst have words, eh?  I'll give thee a word or two!",
            "Talk?  Here is what I have to say!",
            "Pilgrim, I would speak no longer with thee."
        };

        if (!priest->mcanmove || priest->msleeping) {
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
    if (!money_cnt(invent)) {
        if (coaligned && !strayed) {
            long pmoney = money_cnt(priest->minvent);
            if (pmoney > 0L) {
                /* Note: two bits is actually 25 cents.  Hmm. */
                pline("%s gives you %s for an ale.", Monnam(priest),
                      (pmoney == 1L) ? "one bit" : "two bits");
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
            if (money_cnt(invent) > (offer * 2L)) {
                verbalize("Cheapskate.");
            } else {
                verbalize("I thank thee for thy contribution.");
                /* give player some token */
                exercise(A_WIS, TRUE);
            }
        } else if (offer < (u.ulevel * 400)) {
            verbalize("Thou art indeed a pious individual.");
            if (money_cnt(invent) < (offer * 2L)) {
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
            verbalize("Thy devotion has been rewarded.");
            if (!(HProtection & INTRINSIC)) {
                HProtection |= FROMOUTSIDE;
                if (!u.ublessed)
                    u.ublessed = rn1(3, 2);
            } else
                u.ublessed++;
        } else {
            verbalize("Thy selfless generosity is deeply appreciated.");
            if (money_cnt(invent) < (offer * 2L) && coaligned) {
                if (strayed && (moves - u.ucleansed) > 5000L) {
                    u.ualign.record = 0; /* cleanse thee */
                    u.ucleansed = moves;
                } else {
                    adjalign(2);
                }
            }
        }
    }
}

struct monst *
mk_roamer(ptr, alignment, x, y, peaceful)
register struct permonst *ptr;
aligntyp alignment;
xchar x, y;
boolean peaceful;
{
    register struct monst *roamer;
    register boolean coaligned = (u.ualign.type == alignment);

#if 0 /* this was due to permonst's pxlth field which is now gone */
    if (ptr != &mons[PM_ALIGNED_PRIEST] && ptr != &mons[PM_ANGEL])
        return (struct monst *) 0;
#endif

    if (MON_AT(x, y))
        (void) rloc(m_at(x, y), FALSE); /* insurance */

    if (!(roamer = makemon(ptr, x, y, MM_ADJACENTOK | MM_EMIN)))
        return (struct monst *) 0;

    EMIN(roamer)->min_align = alignment;
    EMIN(roamer)->renegade = (coaligned && !peaceful);
    roamer->ispriest = 0;
    roamer->isminion = 1;
    roamer->mtrapseen = ~0; /* traps are known */
    roamer->mpeaceful = peaceful;
    roamer->msleeping = 0;
    set_malign(roamer); /* peaceful may have changed */

    /* MORE TO COME */
    return roamer;
}

void
reset_hostility(roamer)
register struct monst *roamer;
{
    if (!roamer->isminion)
        return;
    if (roamer->data != &mons[PM_ALIGNED_PRIEST]
        && roamer->data != &mons[PM_ANGEL])
        return;

    if (EMIN(roamer)->min_align != u.ualign.type) {
        roamer->mpeaceful = roamer->mtame = 0;
        set_malign(roamer);
    }
    newsym(roamer->mx, roamer->my);
}

boolean
in_your_sanctuary(mon, x, y)
struct monst *mon; /* if non-null, <mx,my> overrides <x,y> */
xchar x, y;
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
ghod_hitsu(priest)
struct monst *priest;
{
    int x, y, ax, ay, roomno = (int) temple_occupied(u.urooms);
    struct mkroom *troom;

    if (!roomno || !has_shrine(priest))
        return;

    ax = x = EPRI(priest)->shrpos.x;
    ay = y = EPRI(priest)->shrpos.y;
    troom = &rooms[roomno - ROOMOFFSET];

    if ((u.ux == x && u.uy == y) || !linedup(u.ux, u.uy, x, y, 1)) {
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

    buzz(-10 - (AD_ELEC - 1), 6, x, y, sgn(tbx),
         sgn(tby)); /* bolt of lightning */
    exercise(A_WIS, FALSE);
}

void
angry_priest()
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
clearpriests()
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
restpriest(mtmp, ghostly)
register struct monst *mtmp;
boolean ghostly;
{
    if (u.uz.dlevel) {
        if (ghostly)
            assign_level(&(EPRI(mtmp)->shrlevel), &u.uz);
    }
}

/*
 * align_str(), piousness(), mstatusline() and ustatusline() used to be
 * in pline.c, presumeably because the latter two generate one line of
 * output.  The USE_OLDARGS config gets warnings from 2016ish-vintage
 * gcc (for -Wint-to-pointer-cast, activated by -Wall or -W) when they
 * follow pline() itself.  Fixing up the variadic calls like is done for
 * lev_comp would be needlessly messy there.
 *
 * They don't belong here.  If/when enlightenment ever gets split off
 * from cmd.c (which definitely doesn't belong there), they should go
 * with it.
 */

const char *
align_str(alignment)
aligntyp alignment;
{
    switch ((int) alignment) {
    case A_CHAOTIC:
        return "chaotic";
    case A_NEUTRAL:
        return "neutral";
    case A_LAWFUL:
        return "lawful";
    case A_NONE:
        return "unaligned";
    }
    return "unknown";
}

/* used for self-probing */
char *
piousness(showneg, suffix)
boolean showneg;
const char *suffix;
{
    static char buf[32]; /* bigger than "insufficiently neutral" */
    const char *pio;

    /* note: piousness 20 matches MIN_QUEST_ALIGN (quest.h) */
    if (u.ualign.record >= 20)
        pio = "piously";
    else if (u.ualign.record > 13)
        pio = "devoutly";
    else if (u.ualign.record > 8)
        pio = "fervently";
    else if (u.ualign.record > 3)
        pio = "stridently";
    else if (u.ualign.record == 3)
        pio = "";
    else if (u.ualign.record > 0)
        pio = "haltingly";
    else if (u.ualign.record == 0)
        pio = "nominally";
    else if (!showneg)
        pio = "insufficiently";
    else if (u.ualign.record >= -3)
        pio = "strayed";
    else if (u.ualign.record >= -8)
        pio = "sinned";
    else
        pio = "transgressed";

    Sprintf(buf, "%s", pio);
    if (suffix && (!showneg || u.ualign.record >= 0)) {
        if (u.ualign.record != 3)
            Strcat(buf, " ");
        Strcat(buf, suffix);
    }
    return buf;
}

/* stethoscope or probing applied to monster -- one-line feedback */
void
mstatusline(mtmp)
struct monst *mtmp;
{
    aligntyp alignment = mon_aligntyp(mtmp);
    char info[BUFSZ], monnambuf[BUFSZ];

    info[0] = 0;
    if (mtmp->mtame) {
        Strcat(info, ", tame");
        if (wizard) {
            Sprintf(eos(info), " (%d", mtmp->mtame);
            if (!mtmp->isminion)
                Sprintf(eos(info), "; hungry %ld; apport %d",
                        EDOG(mtmp)->hungrytime, EDOG(mtmp)->apport);
            Strcat(info, ")");
        }
    } else if (mtmp->mpeaceful)
        Strcat(info, ", peaceful");

    if (mtmp->data == &mons[PM_LONG_WORM]) {
        int segndx, nsegs = count_wsegs(mtmp);

        /* the worm code internals don't consider the head of be one of
           the worm's segments, but we count it as such when presenting
           worm feedback to the player */
        if (!nsegs) {
            Strcat(info, ", single segment");
        } else {
            ++nsegs; /* include head in the segment count */
            segndx = wseg_at(mtmp, bhitpos.x, bhitpos.y);
            Sprintf(eos(info), ", %d%s of %d segments",
                    segndx, ordin(segndx), nsegs);
        }
    }
    if (mtmp->cham >= LOW_PM && mtmp->data != &mons[mtmp->cham])
        /* don't reveal the innate form (chameleon, vampire, &c),
           just expose the fact that this current form isn't it */
        Strcat(info, ", shapechanger");
    /* pets eating mimic corpses mimic while eating, so this comes first */
    if (mtmp->meating)
        Strcat(info, ", eating");
    /* a stethoscope exposes mimic before getting here so this
       won't be relevant for it, but wand of probing doesn't */
    if (mtmp->mundetected || mtmp->m_ap_type)
        mhidden_description(mtmp, TRUE, eos(info));
    if (mtmp->mcan)
        Strcat(info, ", cancelled");
    if (mtmp->mconf)
        Strcat(info, ", confused");
    if (mtmp->mblinded || !mtmp->mcansee)
        Strcat(info, ", blind");
    if (mtmp->mstun)
        Strcat(info, ", stunned");
    if (mtmp->msleeping)
        Strcat(info, ", asleep");
#if 0 /* unfortunately mfrozen covers temporary sleep and being busy \
         (donning armor, for instance) as well as paralysis */
    else if (mtmp->mfrozen)
        Strcat(info, ", paralyzed");
#else
    else if (mtmp->mfrozen || !mtmp->mcanmove)
        Strcat(info, ", can't move");
#endif
    /* [arbitrary reason why it isn't moving] */
    else if (mtmp->mstrategy & STRAT_WAITMASK)
        Strcat(info, ", meditating");
    if (mtmp->mflee)
        Strcat(info, ", scared");
    if (mtmp->mtrapped)
        Strcat(info, ", trapped");
    if (mtmp->mspeed)
        Strcat(info, (mtmp->mspeed == MFAST) ? ", fast"
                      : (mtmp->mspeed == MSLOW) ? ", slow"
                         : ", [? speed]");
    if (mtmp->minvis)
        Strcat(info, ", invisible");
    if (mtmp == u.ustuck)
        Strcat(info, sticks(youmonst.data) ? ", held by you"
                      : !u.uswallow ? ", holding you"
                         : attacktype_fordmg(u.ustuck->data, AT_ENGL, AD_DGST)
                            ? ", digesting you"
                            : is_animal(u.ustuck->data) ? ", swallowing you"
                               : ", engulfing you");
    if (mtmp == u.usteed)
        Strcat(info, ", carrying you");

    /* avoid "Status of the invisible newt ..., invisible" */
    /* and unlike a normal mon_nam, use "saddled" even if it has a name */
    Strcpy(monnambuf, x_monnam(mtmp, ARTICLE_THE, (char *) 0,
                               (SUPPRESS_IT | SUPPRESS_INVISIBLE), FALSE));

    pline("Status of %s (%s):  Level %d  HP %d(%d)  AC %d%s.", monnambuf,
          align_str(alignment), mtmp->m_lev, mtmp->mhp, mtmp->mhpmax,
          find_mac(mtmp), info);
}

/* stethoscope or probing applied to hero -- one-line feedback */
void
ustatusline()
{
    char info[BUFSZ];

    info[0] = '\0';
    if (Sick) {
        Strcat(info, ", dying from");
        if (u.usick_type & SICK_VOMITABLE)
            Strcat(info, " food poisoning");
        if (u.usick_type & SICK_NONVOMITABLE) {
            if (u.usick_type & SICK_VOMITABLE)
                Strcat(info, " and");
            Strcat(info, " illness");
        }
    }
    if (Stoned)
        Strcat(info, ", solidifying");
    if (Slimed)
        Strcat(info, ", becoming slimy");
    if (Strangled)
        Strcat(info, ", being strangled");
    if (Vomiting)
        Strcat(info, ", nauseated"); /* !"nauseous" */
    if (Confusion)
        Strcat(info, ", confused");
    if (Blind) {
        Strcat(info, ", blind");
        if (u.ucreamed) {
            if ((long) u.ucreamed < Blinded || Blindfolded
                || !haseyes(youmonst.data))
                Strcat(info, ", cover");
            Strcat(info, "ed by sticky goop");
        } /* note: "goop" == "glop"; variation is intentional */
    }
    if (Stunned)
        Strcat(info, ", stunned");
    if (!u.usteed && Wounded_legs) {
        const char *what = body_part(LEG);
        if ((Wounded_legs & BOTH_SIDES) == BOTH_SIDES)
            what = makeplural(what);
        Sprintf(eos(info), ", injured %s", what);
    }
    if (Glib)
        Sprintf(eos(info), ", slippery %s", makeplural(body_part(HAND)));
    if (u.utrap)
        Strcat(info, ", trapped");
    if (Fast)
        Strcat(info, Very_fast ? ", very fast" : ", fast");
    if (u.uundetected)
        Strcat(info, ", concealed");
    if (Invis)
        Strcat(info, ", invisible");
    if (u.ustuck) {
        if (sticks(youmonst.data))
            Strcat(info, ", holding ");
        else
            Strcat(info, ", held by ");
        Strcat(info, mon_nam(u.ustuck));
    }

    pline("Status of %s (%s):  Level %d  HP %d(%d)  AC %d%s.", plname,
          piousness(FALSE, align_str(u.ualign.type)),
          Upolyd ? mons[u.umonnum].mlevel : u.ulevel, Upolyd ? u.mh : u.uhp,
          Upolyd ? u.mhmax : u.uhpmax, u.uac, info);
}

/*priest.c*/
