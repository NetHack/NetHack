/* NetHack 3.7	pager.c	$NHDT-Date: 1655120486 2022/06/13 11:41:26 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.225 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2018. */
/* NetHack may be freely redistributed.  See license for details. */

/* This file contains the command routines dowhatis() and dohelp() and */
/* a few other help related facilities */

#include "hack.h"
#include "dlb.h"

static boolean is_swallow_sym(int);
static int append_str(char *, const char *);
static void trap_description(char *, int, coordxy, coordxy);
static void look_at_object(char *, coordxy, coordxy, int);
static void look_at_monster(char *, char *, struct monst *, coordxy, coordxy);
static struct permonst *lookat(coordxy, coordxy, char *, char *);
static void checkfile(char *, struct permonst *, boolean, boolean, char *);
static int add_cmap_descr(int, int, int, int, coord,
                          const char *, const char *,
                          boolean *, const char **, char *);
static void look_region_nearby(coordxy *, coordxy *, coordxy *, coordxy *,
                               boolean);
static void look_all(boolean, boolean);
static void look_traps(boolean);
static void do_supplemental_info(char *, struct permonst *, boolean);
static void whatdoes_help(void);
static void docontact(void);
static void dispfile_help(void);
static void dispfile_shelp(void);
static void dispfile_optionfile(void);
static void dispfile_optmenu(void);
static void dispfile_license(void);
static void dispfile_debughelp(void);
static void dispfile_usagehelp(void);
static void hmenu_doextversion(void);
static void hmenu_dohistory(void);
static void hmenu_dowhatis(void);
static void hmenu_dowhatdoes(void);
static void hmenu_doextlist(void);
static void domenucontrols(void);
#ifdef PORT_HELP
extern void port_help(void);
#endif
static char *setopt_cmd(char *);

static const char invisexplain[] = "remembered, unseen, creature",
           altinvisexplain[] = "unseen creature"; /* for clairvoyance */

/* Returns "true" for characters that could represent a monster's stomach. */
static boolean
is_swallow_sym(int c)
{
    int i;

    for (i = S_sw_tl; i <= S_sw_br; i++)
        if ((int) gs.showsyms[i] == c)
            return TRUE;
    return FALSE;
}

/* Append " or "+new_str to the end of buf if new_str doesn't already exist
   as a substring of buf.  Return 1 if the string was appended, 0 otherwise.
   It is expected that buf is of size BUFSZ. */
static int
append_str(char *buf, const char *new_str)
{
    static const char sep[] = " or ";
    size_t oldlen, space_left;

    if (strstri(buf, new_str))
        return 0; /* already present */

    oldlen = strlen(buf);
    if (oldlen >= BUFSZ - 1) {
        if (oldlen > BUFSZ - 1)
            impossible("append_str: 'buf' contains %lu characters.",
                       (unsigned long) oldlen);
        return 0; /* no space available */
    }

    /* some space available, but not necessarily enough for full append */
    space_left = BUFSZ - 1 - oldlen;  /* space remaining in buf */
    (void) strncat(buf, sep, space_left);
    if (space_left > sizeof sep - 1)
        (void) strncat(buf, new_str, space_left - (sizeof sep - 1));
    return 1; /* something was appended, possibly just part of " or " */
}

/* shared by monster probing (via query_objlist!) as well as lookat() */
char *
self_lookat(char *outbuf)
{
    char race[QBUFSZ], trapbuf[QBUFSZ];

    /* include race with role unless polymorphed */
    race[0] = '\0';
    if (!Upolyd)
        Sprintf(race, "%s ", gu.urace.adj);
    Sprintf(outbuf, "%s%s%s called %s",
            /* being blinded may hide invisibility from self */
            (Invis && (senseself() || !Blind)) ? "invisible " : "", race,
            pmname(&mons[u.umonnum], Ugender), gp.plname);
    if (u.usteed)
        Sprintf(eos(outbuf), ", mounted on %s", y_monnam(u.usteed));
    if (u.uundetected || (Upolyd && U_AP_TYPE))
        mhidden_description(&gy.youmonst, FALSE, eos(outbuf));
    if (Punished)
        Sprintf(eos(outbuf), ", chained to %s",
                uball ? ansimpleoname(uball) : "nothing?");
    if (u.utrap) /* bear trap, pit, web, in-floor, in-lava, tethered */
        Sprintf(eos(outbuf), ", %s", trap_predicament(trapbuf, 0, FALSE));
    return outbuf;
}

/* format a description of 'mon's health for look_at_monster(), done_in_by();
   result isn't Healer-specific (not trained for arbitrary creatures) */
char *
monhealthdescr(struct monst *mon, boolean addspace, char *outbuf)
{
#if 0   /* [disable this for the time being] */
    int mhp_max = max(mon->mhpmax, 1), /* bullet proofing */
        pct = (mon->mhp * 100) / mhp_max;

    if (mon->mhp >= mhp_max)
        Strcpy(outbuf, "uninjured");
    else if (mon->mhp <= 1 || pct < 5)
        Sprintf(outbuf, "%s%s", (mon->mhp > 0) ? "nearly " : "",
                !nonliving(mon->data) ? "deceased" : "defunct");
    else
        Sprintf(outbuf, "%swounded",
                (pct >= 95) ? "barely "
                : (pct >= 80) ? "slightly "
                  : (pct < 20) ? "heavily "
                    : "");
    if (addspace)
        (void) strkitten(outbuf, ' ');
#else
    nhUse(mon);
    nhUse(addspace);
    *outbuf = '\0';
#endif
    return outbuf;
}

/* copy a trap's description into outbuf[] */
static void
trap_description(char *outbuf, int tnum, coordxy x, coordxy y)
{
    /*
     * Trap detection used to display a bear trap at locations having
     * a trapped door or trapped container or both.  They're semi-real
     * traps now (defined trap types but not part of ftrap chain).
     */
    if (trapped_chest_at(tnum, x, y))
        Strcpy(outbuf, "trapped chest"); /* might actually be a large box */
    else if (trapped_door_at(tnum, x, y))
        Strcpy(outbuf, "trapped door"); /* not "trap door"... */
    else
        Strcpy(outbuf, trapname(tnum, FALSE));
    return;
}

/* describe a hidden monster; used for look_at during extended monster
   detection and for probing; also when looking at self */
void
mhidden_description(
    struct monst *mon,
    boolean altmon, /* for probing: if mimicking a monster, say so */
    char *outbuf)
{
    struct obj *otmp;
    boolean fakeobj, isyou = (mon == &gy.youmonst);
    coordxy x = isyou ? u.ux : mon->mx, y = isyou ? u.uy : mon->my;
    int glyph = (gl.level.flags.hero_memory && !isyou) ? levl[x][y].glyph
                                                      : glyph_at(x, y);

    *outbuf = '\0';
    if (M_AP_TYPE(mon) == M_AP_FURNITURE
        || M_AP_TYPE(mon) == M_AP_OBJECT) {
        Strcpy(outbuf, ", mimicking ");
        if (M_AP_TYPE(mon) == M_AP_FURNITURE) {
            Strcat(outbuf, an(defsyms[mon->mappearance].explanation));
        } else if (M_AP_TYPE(mon) == M_AP_OBJECT
                   /* remembered glyph, not glyph_at() which is 'mon' */
                   && glyph_is_object(glyph)) {
 objfrommap:
            otmp = (struct obj *) 0;
            fakeobj = object_from_map(glyph, x, y, &otmp);
            Strcat(outbuf, (otmp && otmp->otyp != STRANGE_OBJECT)
                              ? ansimpleoname(otmp)
                              : an(obj_descr[STRANGE_OBJECT].oc_name));
            if (fakeobj) {
                otmp->where = OBJ_FREE; /* object_from_map set to OBJ_FLOOR */
                dealloc_obj(otmp);
            }
        } else {
            Strcat(outbuf, something);
        }
    } else if (M_AP_TYPE(mon) == M_AP_MONSTER) {
        if (altmon)
            Sprintf(outbuf, ", masquerading as %s",
                    an(pmname(&mons[mon->mappearance], Mgender(mon))));
    } else if (isyou ? u.uundetected : mon->mundetected) {
        Strcpy(outbuf, ", hiding");
        if (hides_under(mon->data)) {
            Strcat(outbuf, " under ");
            /* remembered glyph, not glyph_at() which is 'mon' */
            if (glyph_is_object(glyph))
                goto objfrommap;
            Strcat(outbuf, something);
        } else if (is_hider(mon->data)) {
            Sprintf(eos(outbuf), " on the %s",
                    ceiling_hider(mon->data) ? "ceiling"
                       : surface(x, y)); /* trapper */
        } else {
            if (mon->data->mlet == S_EEL && is_pool(x, y))
                Strcat(outbuf, " in murky water");
        }
    }
}

/* extracted from lookat(); also used by namefloorobj() */
boolean
object_from_map(int glyph, coordxy x, coordxy y, struct obj **obj_p)
{
    boolean fakeobj = FALSE, mimic_obj = FALSE;
    struct monst *mtmp;
    struct obj *otmp;
    int glyphotyp = glyph_to_obj(glyph);

    *obj_p = (struct obj *) 0;
    /* TODO: check inside containers in case glyph came from detection */
    if ((otmp = sobj_at(glyphotyp, x, y)) == 0)
        for (otmp = gl.level.buriedobjlist; otmp; otmp = otmp->nobj)
            if (otmp->ox == x && otmp->oy == y && otmp->otyp == glyphotyp)
                break;

    /* there might be a mimic here posing as an object */
    mtmp = m_at(x, y);
    if (mtmp && is_obj_mappear(mtmp, (unsigned) glyphotyp)) {
        otmp = 0;
        mimic_obj = TRUE;
    } else
        mtmp = 0;

    if (!otmp || otmp->otyp != glyphotyp) {
        /* this used to exclude STRANGE_OBJECT; now caller deals with it */
        otmp = mksobj(glyphotyp, FALSE, FALSE);
        if (!otmp)
            return FALSE;
        fakeobj = TRUE;
        if (otmp->oclass == COIN_CLASS)
            otmp->quan = 2L; /* to force pluralization */
        else if (otmp->otyp == SLIME_MOLD)
            otmp->spe = gc.context.current_fruit; /* give it a type */
        if (mtmp && has_mcorpsenm(mtmp)) { /* mimic as corpse/statue */
            if (otmp->otyp == SLIME_MOLD)
                /* override gc.context.current_fruit to avoid
                     look, use 'O' to make new named fruit, look again
                   giving different results when current_fruit changes */
                otmp->spe = MCORPSENM(mtmp);
            else
                otmp->corpsenm = MCORPSENM(mtmp);
        } else if (otmp->otyp == CORPSE && glyph_is_body(glyph)) {
            otmp->corpsenm = glyph_to_body_corpsenm(glyph);
        } else if (otmp->otyp == STATUE && glyph_is_statue(glyph)) {
            otmp->corpsenm = glyph_to_statue_corpsenm(glyph);
        }
        if (otmp->otyp == LEASH)
            otmp->leashmon = 0;
        /* extra fields needed for shop price with doname() formatting */
        otmp->where = OBJ_FLOOR;
        otmp->ox = x, otmp->oy = y;
        otmp->no_charge = (otmp->otyp == STRANGE_OBJECT && costly_spot(x, y));
    }
    /* if located at adjacent spot, mark it as having been seen up close
       (corpse type will be known even if dknown is 0, so we don't need a
       touch check for cockatrice corpse--we're looking without touching) */
    if (otmp && next2u(x, y) && !Blind && !Hallucination
        /* redundant: we only look for an object which matches current
           glyph among floor and buried objects; when !Blind, any buried
           object's glyph will have been replaced by whatever is present
           on the surface as soon as we moved next to its spot */
        && (fakeobj || otmp->where == OBJ_FLOOR) /* not buried */
        /* terrain mode views what's already known, doesn't learn new stuff */
        && !iflags.terrainmode) /* so don't set dknown when in terrain mode */
        otmp->dknown = 1; /* if a pile, clearly see the top item only */
    if (fakeobj && mtmp && mimic_obj
        && (otmp->dknown || (M_AP_FLAG(mtmp) & M_AP_F_DKNOWN))) {
        mtmp->m_ap_type |= M_AP_F_DKNOWN;
        otmp->dknown = 1;
    }
    *obj_p = otmp;
    return fakeobj; /* when True, caller needs to dealloc *obj_p */
}

static void
look_at_object(char *buf, /* output buffer */
               coordxy x, coordxy y, int glyph)
{
    struct obj *otmp = 0;
    boolean fakeobj = object_from_map(glyph, x, y, &otmp);

    if (otmp) {
        Strcpy(buf, (otmp->otyp != STRANGE_OBJECT)
                     ? distant_name(otmp, otmp->dknown ? doname_with_price
                                                       : doname_vague_quan)
                     : obj_descr[STRANGE_OBJECT].oc_name);
        if (fakeobj) {
            otmp->where = OBJ_FREE; /* object_from_map set it to OBJ_FLOOR */
            dealloc_obj(otmp), otmp = 0;
        }
    } else
        Strcpy(buf, something); /* sanity precaution */

    if (otmp && otmp->where == OBJ_BURIED)
        Strcat(buf, " (buried)");
    else if (levl[x][y].typ == STONE || levl[x][y].typ == SCORR)
        Strcat(buf, " embedded in stone");
    else if (IS_WALL(levl[x][y].typ) || levl[x][y].typ == SDOOR)
        Strcat(buf, " embedded in a wall");
    else if (closed_door(x, y))
        Strcat(buf, " embedded in a door");
    else if (is_pool(x, y))
        Strcat(buf, " in water");
    else if (is_lava(x, y))
        Strcat(buf, " in molten lava"); /* [can this ever happen?] */
    return;
}

static void
look_at_monster(char *buf,
                char *monbuf, /* buf: output, monbuf: optional output */
                struct monst *mtmp,
                coordxy x, coordxy y)
{
    char *name, monnambuf[BUFSZ], healthbuf[BUFSZ];
    boolean accurate = !Hallucination;

    name = (mtmp->data == &mons[PM_COYOTE] && accurate)
              ? coyotename(mtmp, monnambuf)
              : distant_monnam(mtmp, ARTICLE_NONE, monnambuf);
    Sprintf(buf, "%s%s%s%s",
            (mtmp->mx != x || mtmp->my != y)
                ? ((mtmp->isshk && accurate) ? "tail of " : "tail of a ")
                : "",
            accurate ? monhealthdescr(mtmp, TRUE, healthbuf) : "",
            (mtmp->mtame && accurate)
                ? "tame "
                : (mtmp->mpeaceful && accurate)
                    ? "peaceful "
                    : "",
            name);
    if (u.ustuck == mtmp) {
        if (u.uswallow || iflags.save_uswallow) /* monster detection */
            Strcat(buf, digests(mtmp->data) ? ", swallowing you"
                                            : ", engulfing you");
        else
            Strcat(buf, (Upolyd && sticks(gy.youmonst.data))
                          ? ", being held" : ", holding you");
    }
    /* if mtmp isn't able to move (other than because it is a type of
       monster that never moves), say so [excerpt from mstatusline() for
       stethoscope or wand of probing] */
    if (mtmp->mfrozen)
        /* unfortunately mfrozen covers temporary sleep and being busy
           (donning armor, for instance) as well as paralysis */
        Strcat(buf, ", can't move (paralyzed or sleeping or busy)");
    else if (mtmp->msleeping)
        /* sleeping for an indeterminate duration */
        Strcat(buf, ", asleep");
    else if ((mtmp->mstrategy & STRAT_WAITMASK) != 0)
        /* arbitrary reason why it isn't moving */
        Strcat(buf, ", meditating");

    if (mtmp->mleashed)
        Strcat(buf, ", leashed to you");
    if (mtmp->mtrapped && cansee(mtmp->mx, mtmp->my)) {
        struct trap *t = t_at(mtmp->mx, mtmp->my);
        int tt = t ? t->ttyp : NO_TRAP;

        /* newsym lets you know of the trap, so mention it here */
        if (tt == BEAR_TRAP || is_pit(tt) || tt == WEB) {
            Sprintf(eos(buf), ", trapped in %s", an(trapname(tt, FALSE)));
            t->tseen = 1;
        }
    }

    /* we know the hero sees a monster at this location, but if it's shown
       due to persistant monster detection he might remember something else */
    if (mtmp->mundetected || M_AP_TYPE(mtmp))
        mhidden_description(mtmp, FALSE, eos(buf));

    if (monbuf) {
        unsigned how_seen = howmonseen(mtmp);

        monbuf[0] = '\0';
        if (how_seen != 0 && how_seen != MONSEEN_NORMAL) {
            if (how_seen & MONSEEN_NORMAL) {
                Strcat(monbuf, "normal vision");
                how_seen &= ~MONSEEN_NORMAL;
                /* how_seen can't be 0 yet... */
                if (how_seen)
                    Strcat(monbuf, ", ");
            }
            if (how_seen & MONSEEN_SEEINVIS) {
                Strcat(monbuf, "see invisible");
                how_seen &= ~MONSEEN_SEEINVIS;
                if (how_seen)
                    Strcat(monbuf, ", ");
            }
            if (how_seen & MONSEEN_INFRAVIS) {
                Strcat(monbuf, "infravision");
                how_seen &= ~MONSEEN_INFRAVIS;
                if (how_seen)
                    Strcat(monbuf, ", ");
            }
            if (how_seen & MONSEEN_TELEPAT) {
                Strcat(monbuf, "telepathy");
                how_seen &= ~MONSEEN_TELEPAT;
                if (how_seen)
                    Strcat(monbuf, ", ");
            }
            if (how_seen & MONSEEN_XRAYVIS) {
                /* Eyes of the Overworld */
                Strcat(monbuf, "astral vision");
                how_seen &= ~MONSEEN_XRAYVIS;
                if (how_seen)
                    Strcat(monbuf, ", ");
            }
            if (how_seen & MONSEEN_DETECT) {
                Strcat(monbuf, "monster detection");
                how_seen &= ~MONSEEN_DETECT;
                if (how_seen)
                    Strcat(monbuf, ", ");
            }
            if (how_seen & MONSEEN_WARNMON) {
                if (Hallucination) {
                    Strcat(monbuf, "paranoid delusion");
                } else {
                    unsigned long mW = (gc.context.warntype.obj
                                        | gc.context.warntype.polyd),
                                  m2 = mtmp->data->mflags2;
                    const char *whom = ((mW & M2_HUMAN & m2) ? "human"
                                        : (mW & M2_ELF & m2) ? "elf"
                                          : (mW & M2_ORC & m2) ? "orc"
                                            : (mW & M2_DEMON & m2) ? "demon"
                                              : pmname(mtmp->data,
                                                       Mgender(mtmp)));

                    Sprintf(eos(monbuf), "warned of %s", makeplural(whom));
                }
                how_seen &= ~MONSEEN_WARNMON;
                if (how_seen)
                    Strcat(monbuf, ", ");
            }
            /* should have used up all the how_seen bits by now */
            if (how_seen) {
                impossible("lookat: unknown method of seeing monster");
                Sprintf(eos(monbuf), "(%u)", how_seen);
            }
        } /* seen by something other than normal vision */
    } /* monbuf is non-null */
}

/* describe a pool location's contents; might return a static buffer so
   caller should use it or copy it before calling waterbody_name() again
   [3.7: moved here from mkmaze.c] */
const char *
waterbody_name(coordxy x, coordxy y)
{
    static char pooltype[40];
    struct rm *lev;
    schar ltyp;
    boolean hallucinate = Hallucination && !gp.program_state.gameover;

    if (!isok(x, y))
        return "drink"; /* should never happen */
    lev = &levl[x][y];
    ltyp = lev->typ;
    if (ltyp == DRAWBRIDGE_UP)
        ltyp = db_under_typ(lev->drawbridgemask);

    if (ltyp == LAVAPOOL) {
        Snprintf(pooltype, sizeof pooltype, "molten %s", hliquid("lava"));
        return pooltype;
    } else if (ltyp == ICE) {
        if (!hallucinate)
            return "ice";
        Snprintf(pooltype, sizeof pooltype, "frozen %s", hliquid("water"));
        return pooltype;
    } else if (ltyp == POOL) {
        Snprintf(pooltype, sizeof pooltype, "pool of %s", hliquid("water"));
        return pooltype;
    } else if (ltyp == MOAT) {
        /* a bit of extra flavor over general moat */
        if (hallucinate) {
            Snprintf(pooltype, sizeof pooltype, "deep %s", hliquid("water"));
            return pooltype;
        } else if (Is_medusa_level(&u.uz)) {
            /* somewhat iffy since ordinary stairs can take you beneath,
               but previous generic "water" was rather anti-climactic */
            return "shallow sea";
        } else if (Is_juiblex_level(&u.uz)) {
            return "swamp";
        } else if (Role_if(PM_SAMURAI) && Is_qstart(&u.uz)) {
            /* samurai quest home level has two isolated moat spots;
               they sound silly if farlook describes them as such */
            return "pond";
        } else {
            return "moat";
        }
    } else if (IS_WATERWALL(ltyp)) {
        if (Is_waterlevel(&u.uz))
            return "limitless water"; /* even if hallucinating */
        Snprintf(pooltype, sizeof pooltype, "wall of %s", hliquid("water"));
        return pooltype;
    }
    /* default; should be unreachable */
    return "water"; /* don't hallucinate this as some other liquid */
}

/*
 * Return the name of the glyph found at (x,y).
 * If not hallucinating and the glyph is a monster, also monster data.
 */
static struct permonst *
lookat(coordxy x, coordxy y, char *buf, char *monbuf)
{
    struct monst *mtmp = (struct monst *) 0;
    struct permonst *pm = (struct permonst *) 0;
    int glyph;

    buf[0] = monbuf[0] = '\0';
    glyph = glyph_at(x, y);
    if (u_at(x, y) && canspotself()
        && !(iflags.save_uswallow
             && glyph == mon_to_glyph(u.ustuck, rn2_on_display_rng))
        && (!iflags.terrainmode || (iflags.terrainmode & TER_MON) != 0)) {
        /* fill in buf[] */
        (void) self_lookat(buf);

        /* file lookup can't distinguish between "gnomish wizard" monster
           and correspondingly named player character, always picking the
           former; force it to find the general "wizard" entry instead */
        if (Role_if(PM_WIZARD) && Race_if(PM_GNOME) && !Upolyd)
            pm = &mons[PM_WIZARD];

        /* When you see yourself normally, no explanation is appended
           (even if you could also see yourself via other means).
           Sensing self while blind or swallowed is treated as if it
           were by normal vision (cf canseeself()). */
        if ((Invisible || u.uundetected) && !Blind
            && !(u.uswallow || iflags.save_uswallow)) {
            unsigned how = 0;

            if (Infravision)
                how |= 1;
            if (Unblind_telepat)
                how |= 2;
            if (Detect_monsters)
                how |= 4;

            if (how)
                Sprintf(eos(buf), " [seen: %s%s%s%s%s]",
                        (how & 1) ? "infravision" : "",
                        /* add comma if telep and infrav */
                        ((how & 3) > 2) ? ", " : "",
                        (how & 2) ? "telepathy" : "",
                        /* add comma if detect and (infrav or telep or both) */
                        ((how & 7) > 4) ? ", " : "",
                        (how & 4) ? "monster detection" : "");
        }
    } else if (u.uswallow) {
        /* when swallowed, we're only called for spots adjacent to hero,
           and blindness doesn't prevent hero from feeling what holds him */
        Sprintf(buf, "interior of %s", a_monnam(u.ustuck));
        pm = u.ustuck->data;
    } else if (glyph_is_monster(glyph)) {
        gb.bhitpos.x = x;
        gb.bhitpos.y = y;
        if ((mtmp = m_at(x, y)) != 0) {
            look_at_monster(buf, monbuf, mtmp, x, y);
            pm = mtmp->data;
        } else if (Hallucination) {
            /* 'monster' must actually be a statue */
            Strcpy(buf, rndmonnam((char *) 0));
        }
    } else if (glyph_is_object(glyph)) {
        look_at_object(buf, x, y, glyph); /* fill in buf[] */
    } else if (glyph_is_trap(glyph)) {
        int tnum = glyph_to_trap(glyph);

        trap_description(buf, tnum, x, y);
    } else if (glyph_is_warning(glyph)) {
        int warnindx = glyph_to_warning(glyph);

        Strcpy(buf, def_warnsyms[warnindx].explanation);
    } else if (glyph_is_nothing(glyph)) {
        Strcpy(buf, "dark part of a room");
    } else if (glyph_is_unexplored(glyph)) {
        if (Underwater && !Is_waterlevel(&u.uz)) {
            /* "unknown" == previously mapped but not visible when
               submerged; better terminology appreciated... */
            Strcpy(buf, (next2u(x, y)) ? "land" : "unknown");
        } else {
            Strcpy(buf, "unexplored area");
        }
    } else if (glyph_is_invisible(glyph)) {
        /* already handled */
    } else if (!glyph_is_cmap(glyph)) {
        Strcpy(buf, "unexplored area");
    } else {
        int amsk;
        aligntyp algn;
        short symidx = glyph_to_cmap(glyph);

        switch (symidx) {
        case S_altar:
            amsk = altarmask_at(x, y);
            algn = Amask2align(amsk & AM_MASK);
            Sprintf(buf, "%s %saltar",
                    /* like endgame high priests, endgame high altars
                       are only recognizable when immediately adjacent */
                    (Is_astralevel(&u.uz) && !next2u(x, y)
                     && (amsk & AM_SANCTUM))
                        ? "aligned"
                        : align_str(algn),
                    (amsk & AM_SANCTUM) ? "high " : "");
            break;
        case S_ndoor:
            if (is_drawbridge_wall(x, y) >= 0)
                Strcpy(buf, "open drawbridge portcullis");
            else if ((levl[x][y].doormask & ~D_TRAPPED) == D_BROKEN)
                Strcpy(buf, "broken door");
            else
                Strcpy(buf, "doorway");
            break;
        case S_cloud:
            Strcpy(buf,
                   Is_airlevel(&u.uz) ? "cloudy area" : "fog/vapor cloud");
            break;
        case S_pool:
        case S_water:
        case S_lava:
        case S_ice: /* for hallucination; otherwise defsyms[] would be fine */
            Strcpy(buf, waterbody_name(x, y));
            break;
        case S_stone:
            if (!levl[x][y].seenv) {
                Strcpy(buf, "unexplored");
                break;
            } else if (Underwater && !Is_waterlevel(&u.uz)) {
                /* "unknown" == previously mapped but not visible when
                   submerged; better terminology appreciated... */
                Strcpy(buf, (next2u(x, y)) ? "land" : "unknown");
                break;
            } else if (levl[x][y].typ == STONE || levl[x][y].typ == SCORR) {
                Strcpy(buf, "stone");
                break;
            }
            /*FALLTHRU*/
        default:
            Strcpy(buf, defsyms[symidx].explanation);
            break;
        }
    }
    return (pm && !Hallucination) ? pm : (struct permonst *) 0;
}

/*
 * Look in the "data" file for more info.  Called if the user typed in the
 * whole name (user_typed_name == TRUE), or we've found a possible match
 * with a character/glyph and flags.help is TRUE.
 *
 * NOTE: when (user_typed_name == FALSE), inp is considered read-only and
 *       must not be changed directly, e.g. via lcase(). We want to force
 *       lcase() for data.base lookup so that we can have a clean key.
 *       Therefore, we create a copy of inp _just_ for data.base lookup.
 */
static void
checkfile(char *inp, struct permonst *pm, boolean user_typed_name,
          boolean without_asking, char *supplemental_name)
{
    dlb *fp;
    char buf[BUFSZ], newstr[BUFSZ], givenname[BUFSZ];
    char *ep, *dbase_str;
    unsigned long txt_offset = 0L;
    winid datawin = WIN_ERR;

    fp = dlb_fopen(DATAFILE, "r");
    if (!fp) {
        pline("Cannot open 'data' file!");
        return;
    }
    /* If someone passed us garbage, prevent fault. */
    if (!inp || strlen(inp) > (BUFSZ - 1)) {
        impossible("bad do_look buffer passed (%s)!",
                   !inp ? "null" : "too long");
        goto checkfile_done;
    }

    /* To prevent the need for entries in data.base like *ngel to account
     * for Angel and angel, make the lookup string the same for both
     * user_typed_name and picked name.
     */
    if (pm != (struct permonst *) 0 && !user_typed_name)
        dbase_str = strcpy(newstr, pm->pmnames[NEUTRAL]);
    else
        dbase_str = strcpy(newstr, inp);
    (void) lcase(dbase_str);

    /*
     * TODO:
     * The switch from xname() to doname_vague_quan() in look_at_obj()
     * had the unintendded side-effect of making names picked from
     * pointing at map objects become harder to simplify for lookup.
     * We should split the prefix and suffix handling used by wish
     * parsing and also wizmode monster generation out into separate
     * routines and use those routines here.  This currently lacks
     * erosion handling and probably lots of other bits and pieces
     * that wishing already understands and most of this duplicates
     * stuff already done for wish handling or monster generation.
     */
    if (!strncmp(dbase_str, "interior of ", 12))
        dbase_str += 12;
    if (!strncmp(dbase_str, "a ", 2))
        dbase_str += 2;
    else if (!strncmp(dbase_str, "an ", 3))
        dbase_str += 3;
    else if (!strncmp(dbase_str, "the ", 4))
        dbase_str += 4;
    else if (!strncmp(dbase_str, "some ", 5))
        dbase_str += 5;
    else if (digit(*dbase_str)) {
        /* remove count prefix ("2 ya") which can come from looking at map */
        while (digit(*dbase_str))
            ++dbase_str;
        if (*dbase_str == ' ')
            ++dbase_str;
    }
    if (!strncmp(dbase_str, "pair of ", 8))
        dbase_str += 8;
    if (!strncmp(dbase_str, "tame ", 5))
        dbase_str += 5;
    else if (!strncmp(dbase_str, "peaceful ", 9))
        dbase_str += 9;
    if (!strncmp(dbase_str, "invisible ", 10))
        dbase_str += 10;
    if (!strncmp(dbase_str, "saddled ", 8))
        dbase_str += 8;
    if (!strncmp(dbase_str, "blessed ", 8))
        dbase_str += 8;
    else if (!strncmp(dbase_str, "uncursed ", 9))
        dbase_str += 9;
    else if (!strncmp(dbase_str, "cursed ", 7))
        dbase_str += 7;
    if (!strncmp(dbase_str, "empty ", 6))
        dbase_str += 6;
    if (!strncmp(dbase_str, "partly used ", 12))
        dbase_str += 12;
    else if (!strncmp(dbase_str, "partly eaten ", 13))
        dbase_str += 13;
    if (!strncmp(dbase_str, "statue of ", 10))
        dbase_str[6] = '\0';
    else if (!strncmp(dbase_str, "figurine of ", 12))
        dbase_str[8] = '\0';
    /* remove enchantment ("+0 aklys"); [for 3.6.0 and earlier, this wasn't
       needed because looking at items on the map used xname() rather than
       doname() hence known enchantment was implicitly suppressed] */
    if (*dbase_str && strchr("+-", dbase_str[0]) && digit(dbase_str[1])) {
        ++dbase_str; /* skip sign */
        while (digit(*dbase_str))
            ++dbase_str;
        if (*dbase_str == ' ')
            ++dbase_str;
    }
    /* "towel", "wet towel", and "moist towel" share one data.base entry;
       for "wet towel", we keep prefix so that the prompt will ask about
       "wet towel"; for "moist towel", we also want to ask about "wet towel".
       (note: strncpy() only terminates output string if the specified
       count is bigger than the length of the substring being copied) */
    if (!strncmp(dbase_str, "moist towel", 11))
        memcpy(dbase_str += 2, "wet", 3); /* skip "mo" replace "ist" */

    /* Make sure the name is non-empty. */
    if (*dbase_str) {
        long pass1offset = -1L;
        int chk_skip, pass = 1;
        boolean yes_to_moreinfo, found_in_file, pass1found_in_file,
                skipping_entry;
        char *sp, *ap, *alt = 0; /* alternate description */

        /* adjust the input to remove "named " and "called " */
        if ((ep = strstri(dbase_str, " named ")) != 0) {
            alt = ep + 7;
            if ((ap = strstri(dbase_str, " called ")) != 0 && ap < ep)
                ep = ap; /* "named" is alt but truncate at "called" */
        } else if ((ep = strstri(dbase_str, " called ")) != 0) {
            copynchars(givenname, ep + 8, BUFSZ - 1);
            alt = givenname;
            if (supplemental_name && (sp = strstri(inp, " called ")) != 0)
                copynchars(supplemental_name, sp + 8, BUFSZ - 1);
        } else
            ep = strstri(dbase_str, ", ");
        if (ep && ep > dbase_str)
            *ep = '\0';
        /* remove article from 'alt' name ("a pair of lenses named
           The Eyes of the Overworld" simplified above to "lenses named
           The Eyes of the Overworld", now reduced to "The Eyes of the
           Overworld", skip "The" as with base name processing) */
        if (alt && (!strncmpi(alt, "a ", 2)
                    || !strncmpi(alt, "an ", 3)
                    || !strncmpi(alt, "the ", 4)))
            alt = strchr(alt, ' ') + 1;
        /* remove charges or "(lit)" or wizmode "(N aum)" */
        if ((ep = strstri(dbase_str, " (")) != 0 && ep > dbase_str)
            *ep = '\0';
        if (alt && (ap = strstri(alt, " (")) != 0 && ap > alt)
            *ap = '\0';

        /*
         * If the object is named, then the name is the alternate description;
         * otherwise, the result of makesingular() applied to the name is.
         * This isn't strictly optimal, but named objects of interest to the
         * user will usually be found under their name, rather than under
         * their object type, so looking for a singular form is pointless.
         */
        if (!alt)
            alt = makesingular(dbase_str);

        pass1found_in_file = FALSE;
        for (pass = !strcmp(alt, dbase_str) ? 0 : 1; pass >= 0; --pass) {
            found_in_file = skipping_entry = FALSE;
            txt_offset = 0L;
            if (dlb_fseek(fp, txt_offset, SEEK_SET) < 0 ) {
                impossible("can't get to start of 'data' file");
                goto checkfile_done;
            }
            /* skip first record; read second */
            if (!dlb_fgets(buf, BUFSZ, fp) || !dlb_fgets(buf, BUFSZ, fp)) {
                impossible("can't read 'data' file");
                goto checkfile_done;
            } else if (sscanf(buf, "%8lx\n", &txt_offset) < 1
                       || txt_offset == 0L)
                goto bad_data_file;

            /* look for the appropriate entry */
            while (dlb_fgets(buf, BUFSZ, fp)) {
                if (*buf == '.')
                    break; /* we passed last entry without success */

                if (digit(*buf)) {
                    /* a number indicates the end of current entry */
                    skipping_entry = FALSE;
                } else if (!skipping_entry) {
                    if (!(ep = strchr(buf, '\n')))
                        goto bad_data_file;
                    (void) strip_newline((ep > buf) ? ep - 1 : ep);
                    /* if we match a key that begins with "~", skip
                       this entry */
                    chk_skip = (*buf == '~') ? 1 : 0;
                    if ((pass == 0 && pmatch(&buf[chk_skip], dbase_str))
                        || (pass == 1 && alt && pmatch(&buf[chk_skip], alt))) {
                        if (chk_skip) {
                            skipping_entry = TRUE;
                            continue;
                        } else {
                            found_in_file = TRUE;
                            if (pass == 1)
                                pass1found_in_file = TRUE;
                            break;
                        }
                    }
                }
            }
            if (found_in_file) {
                long entry_offset, fseekoffset;
                int entry_count;
                int i;

                /* skip over other possible matches for the info */
                do {
                    if (!dlb_fgets(buf, BUFSZ, fp))
                        goto bad_data_file;
                } while (!digit(*buf));
                if (sscanf(buf, "%ld,%d\n", &entry_offset, &entry_count) < 2)
                    goto bad_data_file;
                fseekoffset = (long) txt_offset + entry_offset;
                if (pass == 1)
                    pass1offset = fseekoffset;
                else if (fseekoffset == pass1offset)
                    goto checkfile_done;

                yes_to_moreinfo = FALSE;
                if (!user_typed_name && !without_asking) {
                    char *entrytext = pass ? alt : dbase_str;
                    char question[QBUFSZ];

                    Strcpy(question, "More info about \"");
                    /* +2 => length of "\"?" */
                    copynchars(eos(question), entrytext,
                               (int) (sizeof question - 1
                                      - (strlen(question) + 2)));
                    Strcat(question, "\"?");
                    if (yn(question) == 'y')
                        yes_to_moreinfo = TRUE;
                }

                if (user_typed_name || without_asking || yes_to_moreinfo) {
                    if (dlb_fseek(fp, fseekoffset, SEEK_SET) < 0) {
                        pline("? Seek error on 'data' file!");
                        goto checkfile_done;
                    }
                    datawin = create_nhwindow(NHW_MENU);
                    for (i = 0; i < entry_count; i++) {
                        /* room for 1-tab or 8-space prefix + BUFSZ-1 + \0 */
                        char tabbuf[BUFSZ + 8], *tp;

                        if (!dlb_fgets(tabbuf, BUFSZ, fp))
                            goto bad_data_file;
                        tp = tabbuf;
                        if (!strchr(tp, '\n'))
                            goto bad_data_file;
                        (void) strip_newline(tp);
                        /* text in this file is indented with one tab but
                           someone modifying it might use spaces instead */
                        if (*tp == '\t') {
                            ++tp;
                        } else if (*tp == ' ') {
                            /* remove up to 8 spaces (we expect 8-column
                               tab stops but user might have them set at
                               something else so we don't require it) */
                            do {
                                ++tp;
                            } while (tp < &tabbuf[8] && *tp == ' ');
                        } else if (*tp) { /* empty lines are ok */
                            goto bad_data_file;
                        }
                        /* if a tab after the leading one is found,
                           convert tabs into spaces; the attributions
                           at the end of quotes typically have them */
                        if (strchr(tp, '\t') != 0)
                            (void) tabexpand(tp);
                        putstr(datawin, 0, tp);
                    }
                    display_nhwindow(datawin, FALSE);
                    destroy_nhwindow(datawin), datawin = WIN_ERR;
                }
            } else if (user_typed_name && pass == 0 && !pass1found_in_file)
                pline("I don't have any information on those things.");
        }
    }
    goto checkfile_done; /* skip error feedback */

 bad_data_file:
    impossible("'data' file in wrong format or corrupted");
 checkfile_done:
    if (datawin != WIN_ERR)
        destroy_nhwindow(datawin);
    (void) dlb_fclose(fp);
    return;
}

/* extracted from do_screen_description() */
static int
add_cmap_descr(
    int found,          /* number of matching descriptions so far */
    int idx,            /* cmap index into defsyms[] */
    int glyph,          /* map glyph of screen symbol being described;
                         * anything other than NO_GLYPH implies 'looked' */
    int article,        /* 0: (none), 1: a/an, 2: the */
    coord cc,           /* map location */
    const char *x_str,  /* description of defsyms[idx] */
    const char *prefix, /* text to insert in front of first match */
    boolean *hit_trap,  /* input/output: True if a trap has been described */
    const char **firstmatch, /* output: pointer to 1st matching description */
    char *out_str)      /* input/output: current description gets appended */
{
    char *mbuf = NULL;
    int absidx = abs(idx);

    if (glyph == NO_GLYPH) {
        /* use x_str [almost] as-is */
        if (!strcmp(x_str, "water")) {
            /* duplicate some transformations performed by waterbody_name() */
            if (idx == S_pool)
                x_str = "pool of water";
            else if (idx == S_water)
                x_str = !Is_waterlevel(&u.uz) ? "wall of water"
                                              : "limitless water";
        }
        if (absidx == S_pool)
            idx = S_pool;
    } else if (absidx == S_pool || idx == S_water
               || idx == S_lava || idx == S_ice) {
        /* replace some descriptions (x_str) with waterbody_name() */
        schar save_ltyp = levl[cc.x][cc.y].typ;
        long save_prop = EHalluc_resistance;

        /* grab a scratch buffer we can safely return (via *firstmatch
           when applicable) */
        mbuf = mon_nam(&gy.youmonst);

        if (absidx == S_pool) {
            levl[cc.x][cc.y].typ = (idx == S_pool) ? POOL : MOAT;
            idx = S_pool; /* force fake negative moat value to be positive */
        } else {
            /* we might be examining a pool location but trying to match
               water or lava; override the terrain with what we're matching
               because that's what waterbody_name() bases its result on;
               it's not pool so must be one of water/lava/ice to get here */
            levl[cc.x][cc.y].typ = (idx == S_water) ? WATER
                                   : (idx == S_lava) ? LAVAPOOL
                                     : ICE;
        }
        EHalluc_resistance = 1;
        Strcpy(mbuf, waterbody_name(cc.x, cc.y));
        EHalluc_resistance = save_prop;
        levl[cc.x][cc.y].typ = save_ltyp;

        /* shorten the feedback for farlook/quicklook: "a pool or ..." */
        if (!strcmp(mbuf, "pool of water"))
            mbuf[4] = '\0';
        else if (!strcmp(mbuf, "molten lava"))
            Strcpy(mbuf, "lava");
        x_str = mbuf;
        article = !(!strncmp(x_str, "water", 5)
                    || !strncmp(x_str, "lava", 4)
                    || !strncmp(x_str, "swamp", 5)
                    || !strncmp(x_str, "molten", 6)
                    || !strncmp(x_str, "shallow", 7)
                    || !strncmp(x_str, "limitless", 9));
    }

    if (!found) {
        /* this is the first match */
        if (is_cmap_trap(idx) && idx != S_vibrating_square) {
            Sprintf(out_str, "%sa trap", prefix);
            *hit_trap = TRUE;
        } else {
            Sprintf(out_str, "%s%s", prefix,
                    article == 2 ? the(x_str)
                    : article == 1 ? an(x_str) : x_str);
        }
        *firstmatch = x_str;
        found = 1;
    } else if (!(*hit_trap && is_cmap_trap(idx))
               && !(found >= 3 && is_cmap_drawbridge(idx))
               /* don't mention vibrating square outside of Gehennom
                  unless this happens to be one (hallucination?) */
               && (idx != S_vibrating_square || Inhell
                   || (glyph_is_trap(glyph)
                       && glyph_to_trap(glyph) == VIBRATING_SQUARE))) {
        /* append unless out_str already contains the string to append */
        found += append_str(out_str, (article == 2) ? the(x_str)
                                     : (article == 1) ? an(x_str)
                                       : x_str);
        if (is_cmap_trap(idx) && idx != S_vibrating_square)
            *hit_trap = TRUE;
    }
    return found;
}

int
do_screen_description(coord cc, boolean looked, int sym, char *out_str,
                      const char **firstmatch,
                      struct permonst **for_supplement)
{
    static const char mon_interior[] = "the interior of a monster",
                      unreconnoitered[] = "unreconnoitered";
    static char look_buf[BUFSZ];
    char prefix[BUFSZ];
    int i, j, alt_i, glyph = NO_GLYPH,
        skipped_venom = 0, found = 0; /* count of matching syms found */
    boolean hit_trap, need_to_look = FALSE,
            submerged = (Underwater && !Is_waterlevel(&u.uz)),
            hallucinate = (Hallucination && !gp.program_state.gameover);
    const char *x_str;
    nhsym tmpsym;
    glyph_info glyphinfo = nul_glyphinfo;

    if (looked) {
        glyph = glyph_at(cc.x, cc.y);
        /* Convert glyph at selected position to a symbol for use below. */
        map_glyphinfo(cc.x, cc.y, glyph, 0, &glyphinfo);
        sym = glyphinfo.ttychar;
        Sprintf(prefix, "%s        ", encglyph(glyphinfo.glyph));
    } else
        Sprintf(prefix, "%c        ", sym);

    /*
     * Check all the possibilities, saving all explanations in a buffer.
     * When all have been checked then the string is printed.
     */

    /*
     * Handle restricted vision range (limited to adjacent spots when
     * swallowed or underwater) cases first.
     *
     * 3.6.0 listed anywhere on map, other than self, as "interior
     * of a monster" when swallowed, and non-adjacent water or
     * non-water anywhere as "dark part of a room" when underwater.
     * "unreconnoitered" is an attempt to convey "even if you knew
     * what was there earlier, you don't know what is there in the
     * current circumstance".
     *
     * (Note: 'self' will always be visible when swallowed so we don't
     * need special swallow handling for <ux,uy>.
     * Another note: for '#terrain' without monsters, u.uswallow and
     * submerged will always both be False and skip this code.)
     */
    x_str = 0;
    if (!looked) {
        ; /* skip special handling */
    } else if (((u.uswallow || submerged) && !next2u(cc.x, cc.y))
               /* detection showing some category, so mostly background */
               || ((iflags.terrainmode & (TER_DETECT | TER_MAP)) == TER_DETECT
                   && glyph == cmap_to_glyph(S_stone))) {
        x_str = unreconnoitered;
        need_to_look = FALSE;
    } else if (is_swallow_sym(sym)) {
        x_str = mon_interior;
        need_to_look = TRUE; /* for specific monster type */
    }
    if (x_str) {
        /* we know 'found' is zero here, but guard against some other
           special case being inserted ahead of us someday */
        if (!found) {
            Sprintf(out_str, "%s%s", prefix, x_str);
            *firstmatch = x_str;
            found++;
        } else {
            found += append_str(out_str, x_str); /* not 'an(x_str)' */
        }
        /* for is_swallow_sym(), we want to list the current symbol's
           other possibilities (wand for '/', throne for '\\', &c) so
           don't jump to the end for the x_str==mon_interior case */
        if (x_str == unreconnoitered)
            goto didlook;
    }
 check_monsters:
    /* Check for monsters */
    if (!iflags.terrainmode || (iflags.terrainmode & TER_MON) != 0) {
        for (i = 1; i < MAXMCLASSES; i++) {
            if (i == S_invisible)  /* avoid matching on this */
                continue;
            if (sym == (looked ? gs.showsyms[i + SYM_OFF_M]
                               : def_monsyms[i].sym)
                && def_monsyms[i].explain && *def_monsyms[i].explain) {
                need_to_look = TRUE;
                if (!found) {
                    Sprintf(out_str, "%s%s",
                            prefix, an(def_monsyms[i].explain));
                    *firstmatch = def_monsyms[i].explain;
                    found++;
                } else {
                    found += append_str(out_str, an(def_monsyms[i].explain));
                }
            }
        }
        /* handle '@' as a special case if it refers to you and you're
           playing a character which isn't normally displayed by that
           symbol; firstmatch is assumed to already be set for '@' */
        if ((looked ? (sym == gs.showsyms[S_HUMAN + SYM_OFF_M]
                       && u_at(cc.x, cc.y))
                    : (sym == def_monsyms[S_HUMAN].sym && !flags.showrace))
            && !(Race_if(PM_HUMAN) || Race_if(PM_ELF)) && !Upolyd)
            found += append_str(out_str, "you"); /* tack on "or you" */
    }

    /* Now check for objects */
    if (!iflags.terrainmode || (iflags.terrainmode & TER_OBJ) != 0) {
        for (i = 1; i < MAXOCLASSES; i++) {
            if (sym == (looked ? gs.showsyms[i + SYM_OFF_O]
                               : def_oc_syms[i].sym)
                || (looked && i == ROCK_CLASS && glyph_is_statue(glyph))) {
                need_to_look = TRUE;
                if (looked && i == VENOM_CLASS) {
                    skipped_venom++;
                    continue;
                }
                if (!found) {
                    Sprintf(out_str, "%s%s",
                            prefix, an(def_oc_syms[i].explain));
                    *firstmatch = def_oc_syms[i].explain;
                    found++;
                } else {
                    found += append_str(out_str, an(def_oc_syms[i].explain));
                }
            }
        }
    }

    if (sym == DEF_INVISIBLE) {
        /* for active clairvoyance, use alternate "unseen creature" */
        boolean usealt = (EDetect_monsters & I_SPECIAL) != 0L;
        const char *unseen_explain = usealt ? altinvisexplain
                                    : Blind ? altinvisexplain : invisexplain;

        if (!found) {
            Sprintf(out_str, "%s%s", prefix, an(unseen_explain));
            *firstmatch = unseen_explain;
            found++;
        } else {
            found += append_str(out_str, an(unseen_explain));
        }
    }
    if ((glyph && glyph_is_nothing(glyph))
        || (looked && sym == gs.showsyms[SYM_NOTHING + SYM_OFF_X])) {
        x_str = "the dark part of a room";
        if (!found) {
            Sprintf(out_str, "%s%s", prefix, x_str);
            *firstmatch = x_str;
            found++;
        } else {
            found += append_str(out_str, x_str);
        }
    }
    if ((glyph && glyph_is_unexplored(glyph))
        || (looked && sym == gs.showsyms[SYM_UNEXPLORED + SYM_OFF_X])) {
        x_str = "unexplored";
        if (submerged)
            x_str = "land"; /* replace "unexplored" */
        if (!found) {
            Sprintf(out_str, "%s%s", prefix, x_str);
            *firstmatch = x_str;
            found++;
        } else {
            found += append_str(out_str, x_str);
        }
    }
    /* Now check for graphics symbols */
    for (hit_trap = FALSE, i = 0; i < MAXPCHARS; i++) {
        /*
         * Index hackery:  we want
         *   "a pool or a moat or a wall of water or lava"
         * rather than
         *   "a pool or a moat or lava or a wall of water"
         * but S_lava comes before S_water so 'i' reaches it sooner.
         * Use 'alt_i' for the rest of the loop to behave as if their
         * places were swapped.
         */
        alt_i = ((i != S_water && i != S_lava) ? i /* as-is */
                 : (S_water + S_lava) - i); /* swap water and lava */
        x_str = defsyms[alt_i].explanation;
        if (!*x_str)  /* cmap includes beams, shield effects, swallow  +*/
            continue; /*+ boundaries, and explosions; skip all of those */
        if (sym == (looked ? gs.showsyms[alt_i] : defsyms[alt_i].sym)) {
            int article; /* article==2 => "the", 1 => "an", 0 => (none) */

            /* check if dark part of a room was already included above */
            if (alt_i == S_darkroom && glyph && glyph_is_nothing(glyph))
                continue;

            /* avoid "an unexplored", "an stone", "an air",
               "a floor of a room", "a dark part of a room" */
            article = strstri(x_str, " of a room") ? 2
                      : !(alt_i == S_stone
                          || strcmp(x_str, "air") == 0
                          || strcmp(x_str, "land") == 0);
            found = add_cmap_descr(found, alt_i, glyph, article,
                                   cc, x_str, prefix,
                                   &hit_trap, firstmatch, out_str);
            if (alt_i == S_pool) {
                /* "pool of water" and "moat" use the same symbol and glyph
                   but have different descriptions; when handling pool, add
                   it a second time for moat but pass an alternate symbol;
                   skip incrementing 'found' to avoid "can be many things" */
                (void) add_cmap_descr(found, -S_pool, glyph, 1,
                                      cc, "moat", prefix,
                                      &hit_trap, firstmatch, out_str);
                need_to_look = TRUE;
            }

            if (alt_i == S_altar || is_cmap_trap(alt_i)
                || (hallucinate && (alt_i == S_water /* S_pool already done */
                                    || alt_i == S_lava || alt_i == S_ice)))
                need_to_look = TRUE;
        }
    }

    /* Now check for warning symbols */
    for (i = 1; i < WARNCOUNT; i++) {
        x_str = def_warnsyms[i].explanation;
        if (sym == (looked ? gw.warnsyms[i] : def_warnsyms[i].sym)) {
            if (!found) {
                Sprintf(out_str, "%s%s", prefix, def_warnsyms[i].explanation);
                *firstmatch = def_warnsyms[i].explanation;
                found++;
            } else {
                found += append_str(out_str, def_warnsyms[i].explanation);
            }
            /* Kludge: warning trumps boulders on the display.
               Reveal the boulder too or player can get confused */
            if (looked && sobj_at(BOULDER, cc.x, cc.y))
                Strcat(out_str, " co-located with a boulder");
            break; /* out of for loop*/
        }
    }

    /* if we ignored venom and list turned out to be short, put it back */
    if (skipped_venom && found < 2) {
        x_str = def_oc_syms[VENOM_CLASS].explain;
        if (!found) {
            Sprintf(out_str, "%s%s", prefix, an(x_str));
            *firstmatch = x_str;
            found++;
        } else {
            found += append_str(out_str, an(x_str));
        }
    }

    /* Finally, handle some optional overriding symbols */
    for (j = SYM_OFF_X; j < SYM_MAX; ++j) {
        if (j == (SYM_INVISIBLE + SYM_OFF_X))
            continue;       /* already handled above */
        tmpsym = Is_rogue_level(&u.uz) ? go.ov_rogue_syms[j]
                                       : go.ov_primary_syms[j];
        if (tmpsym && sym == tmpsym) {
            switch (j) {
            case SYM_BOULDER + SYM_OFF_X: {
                static const char boulder[] = "boulder";

                if (!found) {
                    *firstmatch = boulder;
                    Sprintf(out_str, "%s%s", prefix, an(*firstmatch));
                    found++;
                } else {
                    found += append_str(out_str, boulder);
                }
                break;
            }
            case SYM_PET_OVERRIDE + SYM_OFF_X:
                if (looked) {
                    /* convert to symbol without override in effect */
                    map_glyphinfo(cc.x, cc.y, glyph, MG_FLAG_NOOVERRIDE,
                                  &glyphinfo);
                    sym = glyphinfo.ttychar;
                    goto check_monsters;
                }
                break;
            case SYM_HERO_OVERRIDE + SYM_OFF_X:
                sym = gs.showsyms[S_HUMAN + SYM_OFF_M];
                goto check_monsters;
            }
        }
    }
#if 0
    /* handle optional boulder symbol as a special case */
    if (o_syms[SYM_BOULDER + SYM_OFF_X]
        && sym == o_syms[SYM_BOULDER + SYM_OFF_X]) {
        if (!found) {
            *firstmatch = "boulder";
            Sprintf(out_str, "%s%s", prefix, an(*firstmatch));
            found++;
        } else {
            found += append_str(out_str, "boulder");
        }
    }
#endif

    /*
     * If we are looking at the screen, follow multiple possibilities or
     * an ambiguous explanation by something more detailed.
     */

    if (found > 4)
        /* 3.6.3: this used to be "That can be many things" (without prefix)
           which turned it into a sentence that lacked its terminating period;
           we could add one below but reinstating the prefix here is better */
        Sprintf(out_str, "%scan be many things", prefix);

 didlook:
    if (looked) {
        struct permonst *pm = (struct permonst *)0;

        if (found > 1 || need_to_look) {
            char monbuf[BUFSZ];
            char temp_buf[BUFSZ];

            pm = lookat(cc.x, cc.y, look_buf, monbuf);
            if (pm && for_supplement)
                *for_supplement = pm;

            if (look_buf[0] != '\0')
                *firstmatch = look_buf;
            if (*(*firstmatch)) {
                Snprintf(temp_buf, sizeof temp_buf, " (%s)", *firstmatch);
                (void) strncat(out_str, temp_buf,
                               BUFSZ - strlen(out_str) - 1);
                found = 1; /* we have something to look up */
            }
            if (monbuf[0]) {
                Snprintf(temp_buf, sizeof temp_buf, " [seen: %s]", monbuf);
                (void) strncat(out_str, temp_buf,
                               BUFSZ - strlen(out_str) - 1);
            }
        }
    }

    return found;
}

/* also used by getpos hack in do_name.c */
const char what_is_an_unknown_object[] = "an unknown object";

int
do_look(int mode, coord *click_cc)
{
    boolean quick = (mode == 1); /* use cursor; don't search for "more info" */
    boolean clicklook = (mode == 2); /* right mouse-click method */
    char out_str[BUFSZ] = DUMMY;
    const char *firstmatch = 0;
    struct permonst *pm = 0, *supplemental_pm = 0;
    int i = '\0', ans = 0;
    int sym;              /* typed symbol or converted glyph */
    int found;            /* count of matching syms found */
    coord cc;             /* screen pos of unknown glyph */
    boolean save_verbose; /* saved value of flags.verbose */
    boolean from_screen;  /* question from the screen */
    int clr = 0;

    cc.x = 0;
    cc.y = 0;

    if (!clicklook) {
        if (quick) {
            from_screen = TRUE; /* yes, we want to use the cursor */
            i = 'y';
        } else {
            menu_item *pick_list = (menu_item *) 0;
            winid win;
            anything any;

            any = cg.zeroany;
            win = create_nhwindow(NHW_MENU);
            start_menu(win, MENU_BEHAVE_STANDARD);
            any.a_char = '/';
            /* 'y' and 'n' to keep backwards compatibility with previous
               versions: "Specify unknown object by cursor?" */
            add_menu(win, &nul_glyphinfo, &any,
                     flags.lootabc ? 0 : any.a_char, 'y', ATR_NONE,
                     clr, "something on the map", MENU_ITEMFLAGS_NONE);
            any.a_char = 'i';
            add_menu(win, &nul_glyphinfo, &any,
                     flags.lootabc ? 0 : any.a_char, 0, ATR_NONE,
                     clr, "something you're carrying", MENU_ITEMFLAGS_NONE);
            any.a_char = '?';
            add_menu(win, &nul_glyphinfo, &any,
                     flags.lootabc ? 0 : any.a_char, 'n', ATR_NONE,
                     clr, "something else (by symbol or name)",
                     MENU_ITEMFLAGS_NONE);
            if (!u.uswallow && !Hallucination) {
                any = cg.zeroany;
                add_menu(win, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                         clr, "", MENU_ITEMFLAGS_NONE);
                /* these options work sensibly for the swallowed case,
                   but there's no reason for the player to use them then;
                   objects work fine when hallucinating, but screen
                   symbol/monster class letter doesn't match up with
                   bogus monster type, so suppress when hallucinating */
                any.a_char = 'm';
                add_menu(win, &nul_glyphinfo, &any,
                         flags.lootabc ? 0 : any.a_char, 0, ATR_NONE,
                         clr, "nearby monsters", MENU_ITEMFLAGS_NONE);
                any.a_char = 'M';
                add_menu(win, &nul_glyphinfo, &any,
                         flags.lootabc ? 0 : any.a_char, 0, ATR_NONE,
                         clr, "all monsters shown on map", MENU_ITEMFLAGS_NONE);
                any.a_char = 'o';
                add_menu(win, &nul_glyphinfo, &any,
                         flags.lootabc ? 0 : any.a_char, 0, ATR_NONE,
                         clr, "nearby objects", MENU_ITEMFLAGS_NONE);
                any.a_char = 'O';
                add_menu(win, &nul_glyphinfo, &any,
                         flags.lootabc ? 0 : any.a_char, 0, ATR_NONE,
                         clr, "all objects shown on map", MENU_ITEMFLAGS_NONE);
                any.a_char = '^';
                add_menu(win, &nul_glyphinfo, &any,
                         flags.lootabc ? 0 : any.a_char, 0, ATR_NONE,
                         clr, "nearby traps", MENU_ITEMFLAGS_NONE);
                any.a_char = '\"';
                add_menu(win, &nul_glyphinfo, &any,
                         flags.lootabc ? 0 : any.a_char, 0, ATR_NONE,
                         clr, "all seen or remembered traps",
                         MENU_ITEMFLAGS_NONE);
            }
            end_menu(win, "What do you want to look at:");
            if (select_menu(win, PICK_ONE, &pick_list) > 0) {
                i = pick_list->item.a_char;
                free((genericptr_t) pick_list);
            }
            destroy_nhwindow(win);
        }

        switch (i) {
        default:
        case 'q':
            return ECMD_OK;
        case 'y':
        case '/':
            from_screen = TRUE;
            sym = 0;
            cc.x = u.ux;
            cc.y = u.uy;
            break;
        case 'i':
          {
            char invlet;
            struct obj *invobj;

            invlet = display_inventory((const char *) 0, TRUE);
            if (!invlet || invlet == '\033')
                return ECMD_OK;
            *out_str = '\0';
            for (invobj = gi.invent; invobj; invobj = invobj->nobj)
                if (invobj->invlet == invlet) {
                    strcpy(out_str, singular(invobj, xname));
                    break;
                }
            if (*out_str)
                checkfile(out_str, pm, TRUE, TRUE, (char *) 0);
            return ECMD_OK;
          }
        case '?':
            from_screen = FALSE;
            getlin("Specify what? (type the word)", out_str);
            if (strcmp(out_str, " ")) /* keep single space as-is */
                /* remove leading and trailing whitespace and
                   condense consecutive internal whitespace */
                mungspaces(out_str);
            if (out_str[0] == '\0' || out_str[0] == '\033')
                return ECMD_OK;

            if (out_str[1]) { /* user typed in a complete string */
                checkfile(out_str, pm, TRUE, TRUE, (char *) 0);
                return ECMD_OK;
            }
            sym = out_str[0];
            break;
        case 'm':
            look_all(TRUE, TRUE); /* list nearby monsters */
            return ECMD_OK;
        case 'M':
            look_all(FALSE, TRUE); /* list all monsters */
            return ECMD_OK;
        case 'o':
            look_all(TRUE, FALSE); /* list nearby objects */
            return ECMD_OK;
        case 'O':
            look_all(FALSE, FALSE); /* list all objects */
            return ECMD_OK;
        case '^':
            look_traps(TRUE); /* list nearby traps */
            return ECMD_OK;
        case '\"':
            look_traps(FALSE); /* list all traps (visible or remembered) */
            return ECMD_OK;
        }
    } else { /* clicklook */
        cc.x = click_cc->x;
        cc.y = click_cc->y;
        sym = 0;
        from_screen = FALSE;
    }

    /* Save the verbose flag, we change it later. */
    save_verbose = flags.verbose;
    flags.verbose = flags.verbose && !quick;
    /*
     * The user typed one letter, or we're identifying from the screen.
     */
    do {
        /* Reset some variables. */
        pm = (struct permonst *) 0;
        found = 0;
        out_str[0] = '\0';

        if (from_screen || clicklook) {
            if (from_screen) {
                if (Verbose(2, dolook))
                    pline("Please move the cursor to %s.",
                          what_is_an_unknown_object);
                else
                    pline("Pick an object.");

                ans = getpos(&cc, quick, what_is_an_unknown_object);
                if (ans < 0 || cc.x < 0)
                    break; /* done */
                flags.verbose = FALSE; /* only print long question once */
            }
        }

        found = do_screen_description(cc, (from_screen || clicklook), sym,
                                      out_str, &firstmatch, &supplemental_pm);

        /* Finally, print out our explanation. */
        if (found) {
            /* use putmixed() because there may be an encoded glyph present */
            putmixed(WIN_MESSAGE, 0, out_str);
#ifdef DUMPLOG
            {
                char dmpbuf[BUFSZ];

                /* putmixed() bypasses pline() so doesn't write to DUMPLOG;
                   tty puts it into ^P recall, so it ought to be there;
                   DUMPLOG is plain text, so override graphics character;
                   at present, force space, but we ought to use defsyms[]
                   value for the glyph the graphics character came from */
                (void) decode_mixed(dmpbuf, out_str);
                if (dmpbuf[0] < ' ' || dmpbuf[0] >= 127) /* ASCII isprint() */
                    dmpbuf[0] = ' ';
                dumplogmsg(dmpbuf);
            }
#endif

            /* check the data file for information about this thing */
            if (found == 1 && ans != LOOK_QUICK && ans != LOOK_ONCE
                && (ans == LOOK_VERBOSE || (flags.help && !quick))
                && !clicklook) {
                char temp_buf[BUFSZ], supplemental_name[BUFSZ];

                supplemental_name[0] = '\0';
                Strcpy(temp_buf, firstmatch);
                checkfile(temp_buf, pm, FALSE,
                          (boolean) (ans == LOOK_VERBOSE), supplemental_name);
                if (supplemental_pm)
                    do_supplemental_info(supplemental_name, supplemental_pm,
                                         (boolean) (ans == LOOK_VERBOSE));
            }
        } else {
            pline("I've never heard of such things.");
        }
    } while (from_screen && !quick && ans != LOOK_ONCE && !clicklook);

    flags.verbose = save_verbose;
    return ECMD_OK;
}

static void
look_region_nearby(
    coordxy *lo_x, coordxy *lo_y,
    coordxy *hi_x, coordxy *hi_y, boolean nearby)
{
    *lo_y = nearby ? max(u.uy - BOLT_LIM, 0) : 0;
    *lo_x = nearby ? max(u.ux - BOLT_LIM, 1) : 1;
    *hi_y = nearby ? min(u.uy + BOLT_LIM, ROWNO - 1) : ROWNO - 1;
    *hi_x = nearby ? min(u.ux + BOLT_LIM, COLNO - 1) : COLNO - 1;
}

DISABLE_WARNING_FORMAT_NONLITERAL /* RESTORE is after do_supplemental_info() */

static void
look_all(
    boolean nearby,  /* True => within BOLTLIM, False => entire map */
    boolean do_mons) /* True => monsters, False => objects */
{
    winid win;
    int glyph, count = 0;
    coordxy x, y, lo_x, lo_y, hi_x, hi_y;
    char lookbuf[BUFSZ], outbuf[BUFSZ];

    win = create_nhwindow(NHW_TEXT);
    look_region_nearby(&lo_x, &lo_y, &hi_x, &hi_y, nearby);
    for (y = lo_y; y <= hi_y; y++) {
        for (x = lo_x; x <= hi_x; x++) {
            lookbuf[0] = '\0';
            glyph = glyph_at(x, y);
            if (do_mons) {
                if (glyph_is_monster(glyph)) {
                    struct monst *mtmp;

                    gb.bhitpos.x = x; /* [is this actually necessary?] */
                    gb.bhitpos.y = y;
                    if (u_at(x, y) && canspotself()) {
                        (void) self_lookat(lookbuf);
                        ++count;
                    } else if ((mtmp = m_at(x, y)) != 0) {
                        look_at_monster(lookbuf, (char *) 0, mtmp, x, y);
                        ++count;
                    }
                } else if (glyph_is_invisible(glyph)) {
                    /* remembered, unseen, creature */
                    Strcpy(lookbuf, invisexplain);
                    ++count;
                } else if (glyph_is_warning(glyph)) {
                    int warnindx = glyph_to_warning(glyph);

                    Strcpy(lookbuf, def_warnsyms[warnindx].explanation);
                    ++count;
                }
            } else { /* !do_mons */
                if (glyph_is_object(glyph)) {
                    look_at_object(lookbuf, x, y, glyph);
                    ++count;
                }
            }
            if (*lookbuf) {
                char coordbuf[20], which[12], cmode;

                cmode = (iflags.getpos_coords != GPCOORDS_NONE)
                           ? iflags.getpos_coords : GPCOORDS_MAP;
                if (count == 1) {
                    Strcpy(which, do_mons ? "monsters" : "objects");
                    if (nearby)
                        Sprintf(outbuf, "%s currently shown near %s:",
                                upstart(which),
                                (cmode != GPCOORDS_COMPASS)
                                  ? coord_desc(u.ux, u.uy, coordbuf, cmode)
                                  : !canspotself() ? "your position" : "you");
                    else
                        Sprintf(outbuf, "All %s currently shown on the map:",
                                which);
                    putstr(win, 0, outbuf);
                    /* hack alert! Qt watches a text window for any line
                       with 4 consecutive spaces and renders the window
                       in a fixed-width font it if finds at least one */
                    putstr(win, 0, "    "); /* separator */
                }
                (void) coord_desc(x, y, coordbuf, cmode);
                /* this format wrinkle makes the commas of <x,y> line up;
                   it isn't needed when all the y values have same number
                   of digits but looks better when there is a mixture of 1
                   and 2 digit values; done unconditionally because we
                   would need two passes over the map to determine whether
                   y width is uniform or a mixture; x width is not a factor
                   because the result gets right-justified by %8s; adding
                   a trailing space effectively pushes non-space text left */
                if (cmode == GPCOORDS_MAP && y < 10)
                    (void) strkitten(coordbuf, ' ');
                /* prefix: "coords  C  " where 'C' is mon or obj symbol */
                Sprintf(outbuf, (cmode == GPCOORDS_SCREEN) ? "%s  "
                                  : (cmode == GPCOORDS_MAP) ? "%8s  "
                                      : "%12s  ",
                        coordbuf);
                Sprintf(eos(outbuf), "%s  ", encglyph(glyph));
                /* guard against potential overflow */
                lookbuf[sizeof lookbuf - 1 - strlen(outbuf)] = '\0';
                Strcat(outbuf, lookbuf);
                putmixed(win, 0, outbuf);
            }
        }
    }
    if (count)
        display_nhwindow(win, TRUE);
    else
        pline("No %s are currently shown %s.",
              do_mons ? "monsters" : "objects",
              nearby ? "nearby" : "on the map");
    destroy_nhwindow(win);
}

/* give a /M style display of discovered traps, even when they're covered */
static void
look_traps(boolean nearby)
{
    winid win;
    struct trap *t;
    int glyph, tnum, count = 0;
    coordxy x, y, lo_x, lo_y, hi_x, hi_y;
    char lookbuf[BUFSZ], outbuf[BUFSZ];

    win = create_nhwindow(NHW_TEXT);
    look_region_nearby(&lo_x, &lo_y, &hi_x, &hi_y, nearby);
    for (y = lo_y; y <= hi_y; y++) {
        for (x = lo_x; x <= hi_x; x++) {
            lookbuf[0] = '\0';
            glyph = glyph_at(x, y);
            if (glyph_is_trap(glyph)) {
                tnum = glyph_to_trap(glyph);
                trap_description(lookbuf, tnum, x, y);
                ++count;
            } else if ((t = t_at(x, y)) != 0 && t->tseen
                       /* can't use /" to track traps moved by bubbles or
                          clouds except when hero has direct line of sight */
                       && ((!Is_waterlevel(&u.uz) && !Is_airlevel(&u.uz))
                           || couldsee(x, y))) {
                Strcpy(lookbuf, trapname(t->ttyp, FALSE));
                Sprintf(eos(lookbuf), ", obscured by %s", encglyph(glyph));
                glyph = trap_to_glyph(t);
                ++count;
            }
            if (*lookbuf) {
                char coordbuf[20], cmode;

                cmode = (iflags.getpos_coords != GPCOORDS_NONE)
                           ? iflags.getpos_coords : GPCOORDS_MAP;
                if (count == 1) {
                    Sprintf(outbuf, "%sseen or remembered traps%s:",
                            nearby ? "nearby " : "",
                            nearby ? "" : " on this level");
                    putstr(win, 0, upstart(outbuf));
                    /* hack alert! Qt watches a text window for any line
                       with 4 consecutive spaces and renders the window
                       in a fixed-width font it if finds at least one */
                    putstr(win, 0, "    "); /* separator */
                }
                /* prefix: "coords  C  " where 'C' is trap symbol */
                Sprintf(outbuf, (cmode == GPCOORDS_SCREEN) ? "%s  "
                                  : (cmode == GPCOORDS_MAP) ? "%8s  "
                                      : "%12s  ",
                        coord_desc(x, y, coordbuf, cmode));
                Sprintf(eos(outbuf), "%s  ", encglyph(glyph));
                /* guard against potential overflow */
                lookbuf[sizeof lookbuf - 1 - strlen(outbuf)] = '\0';
                Strcat(outbuf, lookbuf);
                putmixed(win, 0, outbuf);
            }
        }
    }
    if (count)
        display_nhwindow(win, TRUE);
    else
        pline("No traps seen or remembered%s.", nearby ? " nearby" : "");
    destroy_nhwindow(win);
}

static const char *suptext1[] = {
    "%s is a member of a marauding horde of orcs",
    "rumored to have brutally attacked and plundered",
    "the ordinarily sheltered town that is located ",
    "deep within The Gnomish Mines.",
    "",
    "The members of that vicious horde proudly and ",
    "defiantly acclaim their allegiance to their",
    "leader %s in their names.",
    (char *) 0,
};

static const char *suptext2[] = {
    "\"%s\" is the common dungeon name of",
    "a nefarious orc who is known to acquire property",
    "from thieves and sell it off for profit.",
    "",
    "The perpetrator was last seen hanging around the",
    "stairs leading to the Gnomish Mines.",
    (char *) 0,
};

static void
do_supplemental_info(char *name, struct permonst *pm, boolean without_asking)
{
    const char **textp;
    winid datawin = WIN_ERR;
    char *entrytext = name, *bp = (char *) 0, *bp2 = (char *) 0;
    char question[QBUFSZ];
    boolean yes_to_moreinfo = FALSE;
    boolean is_marauder = (name && pm && is_orc(pm));

    /*
     * Provide some info on some specific things
     * meant to support in-game mythology, and not
     * available from data.base or other sources.
     */
    if (is_marauder && (strlen(name) < (BUFSZ - 1))) {
        char fullname[BUFSZ];

        bp = strstri(name, " of ");
        bp2 = strstri(name, " the Fence");

        if (bp || bp2) {
            Strcpy(fullname, name);
            if (!without_asking) {
                Strcpy(question, "More info about \"");
                /* +2 => length of "\"?" */
                copynchars(eos(question), entrytext,
                    (int) (sizeof question - 1 - (strlen(question) + 2)));
                Strcat(question, "\"?");
                if (yn(question) == 'y')
                yes_to_moreinfo = TRUE;
            }
            if (yes_to_moreinfo) {
                int i, subs = 0;
                const char *gang = (char *) 0;

                if (bp) {
                    textp = suptext1;
                    gang = bp + 4;
                    *bp = '\0';
                } else {
                    textp = suptext2;
                    gang = "";
                }
                datawin = create_nhwindow(NHW_MENU);
                for (i = 0; textp[i]; i++) {
                    char buf[BUFSZ];
                    const char *txt;

                    if (strstri(textp[i], "%s") != 0) {
                        Sprintf(buf, textp[i], subs++ ? gang : fullname);
                        txt = buf;
                    } else
                        txt = textp[i];
                    putstr(datawin, 0, txt);
                }
                display_nhwindow(datawin, FALSE);
                destroy_nhwindow(datawin), datawin = WIN_ERR;
            }
        }
    }
}

RESTORE_WARNING_FORMAT_NONLITERAL

/* the #whatis command */
int
dowhatis(void)
{
    return do_look(0, (coord *) 0);
}

/* the #glance command */
int
doquickwhatis(void)
{
    return do_look(1, (coord *) 0);
}

/* the #showtrap command */
int
doidtrap(void)
{
    register struct trap *trap;
    int tt, glyph;
    coordxy x, y;

    if (!getdir("^"))
        return ECMD_CANCEL;
    x = u.ux + u.dx;
    y = u.uy + u.dy;

    /* trapped doors and chests used to be shown as fake bear traps;
       they have their own trap types now but aren't part of the ftrap
       chain; usually they revert to normal door or chest when the hero
       sees them but player might be using '^' while the hero is blind */
    glyph = glyph_at(x, y);
    if (glyph_is_trap(glyph)
        && ((tt = glyph_to_trap(glyph)) == BEAR_TRAP
            || tt == TRAPPED_DOOR || tt == TRAPPED_CHEST)) {
        boolean chesttrap = trapped_chest_at(tt, x, y);

        if (chesttrap || trapped_door_at(tt, x, y)) {
            pline("That is a trapped %s.", chesttrap ? "chest" : "door");
            return ECMD_OK; /* trap ID'd, but no time elapses */
        }
    }

    for (trap = gf.ftrap; trap; trap = trap->ntrap)
        if (trap->tx == x && trap->ty == y) {
            if (!trap->tseen)
                break;
            tt = trap->ttyp;
            if (u.dz) {
                if (u.dz < 0 ? is_hole(tt) : tt == ROCKTRAP)
                    break;
            }
            pline("That is %s%s%s.",
                  an(trapname(tt, FALSE)),
                  !trap->madeby_u
                     ? ""
                     : (tt == WEB)
                        ? " woven"
                        /* trap doors & spiked pits can't be made by
                           player, and should be considered at least
                           as much "set" as "dug" anyway */
                        : (tt == HOLE || tt == PIT)
                           ? " dug"
                           : " set",
                  !trap->madeby_u ? "" : " by you");
            return ECMD_OK;
        }
    pline("I can't see a trap there.");
    return ECMD_OK;
}

/*
    Implements a rudimentary if/elif/else/endif interpretor and use
    conditionals in dat/cmdhelp to describe what command each keystroke
    currently invokes, so that there isn't a lot of "(debug mode only)"
    and "(if number_pad is off)" cluttering the feedback that the user
    sees.  (The conditionals add quite a bit of clutter to the raw data
    but users don't see that.  number_pad produces a lot of conditional
    commands:  basic letters vs digits, 'g' vs 'G' for '5', phone
    keypad vs normal layout of digits, and QWERTZ keyboard swap between
    y/Y/^Y/M-y/M-Y/M-^Y and z/Z/^Z/M-z/M-Z/M-^Z.)

    The interpretor understands
     '&#' for comment,
     '&? option' for 'if' (also '&? !option'
                           or '&? option=value[,value2,...]'
                           or '&? !option=value[,value2,...]'),
     '&: option' for 'elif' (with argument variations same as 'if';
                             any number of instances for each 'if'),
     '&:' for 'else' (also '&: #comment';
                      0 or 1 instance for a given 'if'), and
     '&.' for 'endif' (also '&. #comment'; required for each 'if').

    The option handling is a bit of a mess, with no generality for
    which options to deal with and only a comma separated list of
    integer values for the '=value' part.  number_pad is the only
    supported option that has a value; the few others (wizard/debug,
    rest_on_space, #if SHELL, #if SUSPEND) are booleans.
*/

static void
whatdoes_help(void)
{
    dlb *fp;
    char *p, buf[BUFSZ];
    winid tmpwin;

    fp = dlb_fopen(KEYHELP, "r");
    if (!fp) {
        pline("Cannot open \"%s\" data file!", KEYHELP);
        display_nhwindow(WIN_MESSAGE, TRUE);
        return;
    }
    tmpwin = create_nhwindow(NHW_TEXT);
    while (dlb_fgets(buf, (int) sizeof buf, fp)) {
        if (*buf == '#')
            continue;
        for (p = buf; *p; p++)
            if (*p != ' ' && *p != '\t')
                break;
        putstr(tmpwin, 0, p);
    }
    (void) dlb_fclose(fp);
    display_nhwindow(tmpwin, TRUE);
    destroy_nhwindow(tmpwin);
}

#if 0
#define WD_STACKLIMIT 5
struct wd_stack_frame {
    Bitfield(active, 1);
    Bitfield(been_true, 1);
    Bitfield(else_seen, 1);
};

static boolean whatdoes_cond(char *, struct wd_stack_frame *, int *, int);

static boolean
whatdoes_cond(char *buf, struct wd_stack_frame *stack, int *depth, int lnum)
{
    const char badstackfmt[] = "cmdhlp: too many &%c directives at line %d.";
    boolean newcond, neg, gotopt;
    char *p, *q, act = buf[1];
    int np = 0;

    newcond = (act == '?' || !stack[*depth].been_true);
    buf += 2;
    mungspaces(buf);
    if (act == '#' || *buf == '#' || !*buf || !newcond) {
        gotopt = (*buf && *buf != '#');
        *buf = '\0';
        neg = FALSE; /* lint suppression */
        p = q = (char *) 0;
    } else {
        gotopt = TRUE;
        if ((neg = (*buf == '!')) != 0)
            if (*++buf == ' ')
                ++buf;
        p = strchr(buf, '='), q = strchr(buf, ':');
        if (!p || (q && q < p))
            p = q;
        if (p) { /* we have a value specified */
            /* handle a space before or after (or both) '=' (or ':') */
            if (p > buf && p[-1] == ' ')
                p[-1] = '\0'; /* end of keyword in buf[] */
            *p++ = '\0'; /* terminate keyword, advance to start of value */
            if (*p == ' ')
                p++;
        }
    }
    if (*buf && (act == '?' || act == ':')) {
        if (!strcmpi(buf, "number_pad")) {
            if (!p) {
                newcond = iflags.num_pad;
            } else {
                /* convert internal encoding (separate yes/no and 0..3)
                   back to user-visible one (-1..4) */
                np = iflags.num_pad ? (1 + iflags.num_pad_mode) /* 1..4 */
                                    : (-1 * iflags.num_pad_mode); /* -1..0 */
                newcond = FALSE;
                for (; p; p = q) {
                    q = strchr(p, ',');
                    if (q)
                        *q++ = '\0';
                    if (atoi(p) == np) {
                        newcond = TRUE;
                        break;
                    }
                }
            }
        } else if (!strcmpi(buf, "rest_on_space")) {
            newcond = flags.rest_on_space;
        } else if (!strcmpi(buf, "debug") || !strcmpi(buf, "wizard")) {
            newcond = flags.debug; /* == wizard */
        } else if (!strcmpi(buf, "shell")) {
#ifdef SHELL
            /* should we also check sysopt.shellers? */
            newcond = TRUE;
#else
            newcond = FALSE;
#endif
        } else if (!strcmpi(buf, "suspend")) {
#ifdef SUSPEND
            /* sysopt.shellers is also used for dosuspend()... */
            newcond = TRUE;
#else
            newcond = FALSE;
#endif
        } else {
            impossible(
                "cmdhelp: unrecognized &%c conditional at line %d: \"%.20s\"",
                       act, lnum, buf);
            neg = FALSE;
        }
        /* this works for number_pad too: &? !number_pad:-1,0
           would be true for 1..4 after negation */
        if (neg)
            newcond = !newcond;
    }
    switch (act) {
    default:
    case '#': /* comment */
        break;
    case '.': /* endif */
        if (--*depth < 0) {
            impossible(badstackfmt, '.', lnum);
            *depth = 0;
        }
        break;
    case ':': /* else or elif */
        if (*depth == 0 || stack[*depth].else_seen) {
            impossible(badstackfmt, ':', lnum);
            *depth = 1; /* so that stack[*depth - 1] is a valid access */
        }
        if (stack[*depth].active || stack[*depth].been_true
            || !stack[*depth - 1].active)
            stack[*depth].active = 0;
        else if (newcond)
            stack[*depth].active = stack[*depth].been_true = 1;
        if (!gotopt)
            stack[*depth].else_seen = 1;
        break;
    case '?': /* if */
        if (++*depth >= WD_STACKLIMIT) {
            impossible(badstackfmt, '?', lnum);
            *depth = WD_STACKLIMIT - 1;
        }
        stack[*depth].active = (newcond && stack[*depth - 1].active) ? 1 : 0;
        stack[*depth].been_true = stack[*depth].active;
        stack[*depth].else_seen = 0;
        break;
    }
    return stack[*depth].active ? TRUE : FALSE;
}
#endif /* 0 */

char *
dowhatdoes_core(char q, char *cbuf)
{
    char buf[BUFSZ];
#if 0
    dlb *fp;
    struct wd_stack_frame stack[WD_STACKLIMIT];
    boolean cond;
    int ctrl, meta, depth = 0, lnum = 0;
#endif /* 0 */
    const char *ec_desc;

    if ((ec_desc = key2extcmddesc(q)) != NULL) {
        char keybuf[QBUFSZ];

        /* note: if "%-8s" gets changed, the "%8.8s" in dowhatdoes() will
           need a comparable change */
        Sprintf(buf, "%-8s%s.", key2txt(q, keybuf), ec_desc);
        Strcpy(cbuf, buf);
        return cbuf;
    }
    return 0;
#if 0
    fp = dlb_fopen(CMDHELPFILE, "r");
    if (!fp) {
        pline("Cannot open \"%s\" data file!", CMDHELPFILE);
        return 0;
    }

    meta = (0x80 & (uchar) q) != 0;
    if (meta)
        q &= 0x7f;
    ctrl = (0x1f & (uchar) q) == (uchar) q;
    if (ctrl)
        q |= 0x40; /* NUL -> '@', ^A -> 'A', ... ^Z -> 'Z', ^[ -> '[', ... */
    else if (q == 0x7f)
        ctrl = 1, q = '?';

    (void) memset((genericptr_t) stack, 0, sizeof stack);
    cond = stack[0].active = 1;
    while (dlb_fgets(buf, sizeof buf, fp)) {
        ++lnum;
        if (buf[0] == '&' && buf[1] && strchr("?:.#", buf[1])) {
            cond = whatdoes_cond(buf, stack, &depth, lnum);
            continue;
        }
        if (!cond)
            continue;
        if (meta ? (buf[0] == 'M' && buf[1] == '-'
                    && (ctrl ? buf[2] == '^' && highc(buf[3]) == q
                             : buf[2] == q))
                 : (ctrl ? buf[0] == '^' && highc(buf[1]) == q
                         : buf[0] == q)) {
            (void) strip_newline(buf);
            if (strchr(buf, '\t'))
                (void) tabexpand(buf);
            if (meta && ctrl && buf[4] == ' ') {
                (void) strncpy(buf, "M-^?    ", 8);
                buf[3] = q;
            } else if (meta && buf[3] == ' ') {
                (void) strncpy(buf, "M-?     ", 8);
                buf[2] = q;
            } else if (ctrl && buf[2] == ' ') {
                (void) strncpy(buf, "^?      ", 8);
                buf[1] = q;
            } else if (buf[1] == ' ') {
                (void) strncpy(buf, "?       ", 8);
                buf[0] = q;
            }
            (void) dlb_fclose(fp);
            Strcpy(cbuf, buf);
            return cbuf;
        }
    }
    (void) dlb_fclose(fp);
    if (depth != 0)
        impossible("cmdhelp: mismatched &? &: &. conditionals.");
    return (char *) 0;
#endif /* 0 */
}

/* the whatdoes command */
int
dowhatdoes(void)
{
    static boolean once = FALSE;
    char bufr[BUFSZ];
    char q, *reslt;

    if (!once) {
        pline("Ask about '&' or '?' to get more info.%s",
#ifdef ALTMETA
              iflags.altmeta ? "  (For ESC, type it twice.)" :
#endif
              "");
        once = TRUE;
    }
#if defined(UNIX) || defined(VMS)
    introff(); /* disables ^C but not ^\ */
#endif
    q = yn_function("What command?", (char *) 0, '\0', TRUE);
#ifdef ALTMETA
    if (q == '\033' && iflags.altmeta) {
        /* in an ideal world, we would know whether another keystroke
           was already pending, but this is not an ideal world...
           if user typed ESC, we'll essentially hang until another
           character is typed */
        q = yn_function("]", (char *) 0, '\0', TRUE);
        if (q != '\033')
            q = (char) ((uchar) q | 0200);
    }
#endif /*ALTMETA*/
#if defined(UNIX) || defined(VMS)
    intron(); /* reenables ^C */
#endif
    reslt = dowhatdoes_core(q, bufr);
    if (reslt) {
        char *p = strchr(reslt, '\n'); /* 'm' prefix has two lines of output */

        if (q == '&' || q == '?')
            whatdoes_help();
        if (!p) {
            /* normal usage; 'reslt' starts with key, some indentation, and
               then explanation followed by '.' for sentence punctuation */
            pline("%s", reslt);
        } else {
            /* for 'm' prefix, where 'reslt' has an embedded newline to
               indicate and separate two lines of output; we add a comma to
               first line so that the combination is a complete sentence */
            *p = '\0'; /* replace embedded newline with end of first line */
            pline("%s,", reslt);
            /* cheat by knowing how dowhatdoes_core() handles key portion */
            pline("%8.8s%s", reslt, p + 1);
        }
    } else {
        pline("No such command '%s', char code %d (0%03o or 0x%02x).",
              visctrl(q), (uchar) q, (uchar) q, (uchar) q);
    }
    return ECMD_OK;
}

static void
docontact(void)
{
    winid cwin = create_nhwindow(NHW_TEXT);
    char buf[BUFSZ];

    if (sysopt.support) {
        /*XXX overflow possibilities*/
        Sprintf(buf, "To contact local support, %s", sysopt.support);
        putstr(cwin, 0, buf);
        putstr(cwin, 0, "");
    } else if (sysopt.fmtd_wizard_list) { /* formatted SYSCF WIZARDS */
        Sprintf(buf, "To contact local support, contact %s.",
                sysopt.fmtd_wizard_list);
        putstr(cwin, 0, buf);
        putstr(cwin, 0, "");
    }
    putstr(cwin, 0, "To contact the NetHack development team directly,");
    /*XXX overflow possibilities*/
    Sprintf(buf, "see the 'Contact' form on our website or email <%s>.",
            DEVTEAM_EMAIL);
    putstr(cwin, 0, buf);
    putstr(cwin, 0, "");
    putstr(cwin, 0, "For more information on NetHack, or to report a bug,");
    Sprintf(buf, "visit our website \"%s\".", DEVTEAM_URL);
    putstr(cwin, 0, buf);
    display_nhwindow(cwin, FALSE);
    destroy_nhwindow(cwin);
}

static void
dispfile_help(void)
{
    display_file(HELP, TRUE);
}

static void
dispfile_shelp(void)
{
    display_file(SHELP, TRUE);
}

static void
dispfile_optionfile(void)
{
    display_file(OPTIONFILE, TRUE);
}

static void
dispfile_optmenu(void)
{
    display_file(OPTMENUHELP, TRUE);
}

static void
dispfile_license(void)
{
    display_file(LICENSE, TRUE);
}

static void
dispfile_debughelp(void)
{
    display_file(DEBUGHELP, TRUE);
}

static void
dispfile_usagehelp(void)
{
    display_file(USAGEHELP, TRUE);
}

static void
hmenu_doextversion(void)
{
    (void) doextversion();
}

static void
hmenu_dohistory(void)
{
    (void) dohistory();
}

static void
hmenu_dowhatis(void)
{
    (void) dowhatis();
}

static void
hmenu_dowhatdoes(void)
{
    (void) dowhatdoes();
}

static void
hmenu_doextlist(void)
{
    (void) doextlist();
}

static void
domenucontrols(void)
{
    winid cwin = create_nhwindow(NHW_TEXT);

    show_menu_controls(cwin, FALSE);
    display_nhwindow(cwin, FALSE);
    destroy_nhwindow(cwin);
}

/* data for dohelp() */
static const struct {
    void (*f)(void);
    const char *text;
} help_menu_items[] = {
    { hmenu_doextversion, "About NetHack (version information)." },
    { dispfile_help, "Long description of the game and commands." },
    { dispfile_shelp, "List of game commands." },
    { hmenu_dohistory, "Concise history of NetHack." },
    { hmenu_dowhatis, "Info on a character in the game display." },
    { hmenu_dowhatdoes, "Info on what a given key does." },
    { option_help, "List of game options." },
    { dispfile_optionfile, "Longer explanation of game options." },
    { dispfile_optmenu, "Using the %s command to set options." },
    { dokeylist, "Full list of keyboard commands." },
    { hmenu_doextlist, "List of extended commands." },
    { domenucontrols, "List menu control keys." },
    { dispfile_usagehelp, "Description of NetHack's command line." },
    { dispfile_license, "The NetHack license." },
    { docontact, "Support information." },
#ifdef PORT_HELP
    { port_help, "%s-specific help and commands." },
#endif
    { dispfile_debughelp, "List of wizard-mode commands." },
    { (void (*)(void)) 0, (char *) 0 }
};

DISABLE_WARNING_FORMAT_NONLITERAL

/* the #help command */
int
dohelp(void)
{
    winid tmpwin = create_nhwindow(NHW_MENU);
    char helpbuf[QBUFSZ], tmpbuf[QBUFSZ];
    int i, n;
    menu_item *selected;
    anything any;
    int sel;
    int clr = 0;

    any = cg.zeroany; /* zero all bits */
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);

    for (i = 0; help_menu_items[i].text; i++) {
        if (!wizard && help_menu_items[i].f == dispfile_debughelp)
            continue;
        if (sysopt.hideusage && help_menu_items[i].f == dispfile_usagehelp)
            continue;

        if (help_menu_items[i].text[0] == '%') {
            Sprintf(helpbuf, help_menu_items[i].text, PORT_ID);
        } else if (help_menu_items[i].f == dispfile_optmenu) {
            Sprintf(helpbuf, help_menu_items[i].text, setopt_cmd(tmpbuf));
        } else {
            Strcpy(helpbuf, help_menu_items[i].text);
        }
        any.a_int = i + 1;
        add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE, clr,
                 helpbuf, MENU_ITEMFLAGS_NONE);
    }
    end_menu(tmpwin, "Select one item:");
    n = select_menu(tmpwin, PICK_ONE, &selected);
    destroy_nhwindow(tmpwin);
    if (n > 0) {
        sel = selected[0].item.a_int - 1;
        free((genericptr_t) selected);
        (void) (*help_menu_items[sel].f)();
    }
    return ECMD_OK;
}

RESTORE_WARNING_FORMAT_NONLITERAL

/* format the key or extended command name of command used to set options;
   normally 'O' but could be bound to something else, or not bound at all;
   with the implementation of a simple options subset, now need 'mO' to get
   the full options command; format it as 'm O' */
static char *
setopt_cmd(char *outbuf)
{
    char cmdbuf[QBUFSZ];
    const char *cmdnm;
    char key;

    Strcpy(outbuf, "\'");
    /* #optionsfull */
    key = cmd_from_func(doset);
    if (key) {
        Strcat(outbuf, visctrl(key));
    } else {
        /* extended command name, with leading "#" */
        cmdnm = cmdname_from_func(doset, cmdbuf, TRUE);
        if (!cmdnm) /* paranoia */
            cmdnm = "optionsfull";
        Sprintf(eos(outbuf), "%s%.31s", (*cmdnm != '#') ? "#" : "", cmdnm);

        /* since there's no key bound to #optionsfull, include 'm O' */
        Strcat(outbuf, "\' or \'");
        /* m prefix plus #options */
        key = cmd_from_func(do_reqmenu);
        if (key) {
            /* key for 'm' prefix */
            Strcat(outbuf, visctrl(key));
        } else {
            /* extended command name for 'm' prefix */
            cmdnm = cmdname_from_func(do_reqmenu, cmdbuf, TRUE);
            if (!cmdnm)
                cmdnm = "reqmenu";
            Sprintf(eos(outbuf), "%s%.31s", (*cmdnm != '#') ? "#" : "", cmdnm);
        }
        /* this is slightly iffy because the user shouldn't type <space> to
           get the command we're describing, but it improves readability */
        Strcat(outbuf, " ");
        /* now #options, normally 'O' */
        key = cmd_from_func(doset_simple);
        if (key) {
            Strcat(outbuf, visctrl(key));
        } else {
            /* extended command name */
            cmdnm = cmdname_from_func(doset_simple, cmdbuf, TRUE);
            if (!cmdnm) /* paranoia */
                cmdnm = "options";
            Sprintf(eos(outbuf), "%s%.31s", (*cmdnm != '#') ? "#" : "", cmdnm);
        }
    }
    Strcat(outbuf, "\'");
    return outbuf;
}

/* the 'V' command; also a choice for '?' */
int
dohistory(void)
{
    display_file(HISTORY, TRUE);
    return ECMD_OK;
}

/*pager.c*/
