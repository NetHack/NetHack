/* NetHack 3.7	engrave.c	$NHDT-Date: 1612055954 2021/01/31 01:19:14 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.102 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

static int stylus_ok(struct obj *);
static boolean u_can_engrave(void);
static int engrave(void);
static const char *blengr(void);

char *
random_engraving(char *outbuf)
{
    const char *rumor;

    /* a random engraving may come from the "rumors" file,
       or from the "engrave" file (formerly in an array here) */
    if (!rn2(4) || !(rumor = getrumor(0, outbuf, TRUE)) || !*rumor)
        (void) get_rnd_text(ENGRAVEFILE, outbuf, rn2, MD_PAD_RUMORS);

    wipeout_text(outbuf, (int) (strlen(outbuf) / 4), 0);
    return outbuf;
}

/* Partial rubouts for engraving characters. -3. */
static const struct {
    char wipefrom;
    const char *wipeto;
} rubouts[] = { { 'A', "^" },
                { 'B', "Pb[" },
                { 'C', "(" },
                { 'D', "|)[" },
                { 'E', "|FL[_" },
                { 'F', "|-" },
                { 'G', "C(" },
                { 'H', "|-" },
                { 'I', "|" },
                { 'K', "|<" },
                { 'L', "|_" },
                { 'M', "|" },
                { 'N', "|\\" },
                { 'O', "C(" },
                { 'P', "F" },
                { 'Q', "C(" },
                { 'R', "PF" },
                { 'T', "|" },
                { 'U', "J" },
                { 'V', "/\\" },
                { 'W', "V/\\" },
                { 'Z', "/" },
                { 'b', "|" },
                { 'd', "c|" },
                { 'e', "c" },
                { 'g', "c" },
                { 'h', "n" },
                { 'j', "i" },
                { 'k', "|" },
                { 'l', "|" },
                { 'm', "nr" },
                { 'n', "r" },
                { 'o', "c" },
                { 'q', "c" },
                { 'w', "v" },
                { 'y', "v" },
                { ':', "." },
                { ';', ",:" },
                { ',', "." },
                { '=', "-" },
                { '+', "-|" },
                { '*', "+" },
                { '@', "0" },
                { '0', "C(" },
                { '1', "|" },
                { '6', "o" },
                { '7', "/" },
                { '8', "3o" } };

/* degrade some of the characters in a string */
void
wipeout_text(
    char *engr,    /* engraving text */
    int cnt,       /* number of chars to degrade */
    unsigned seed) /* for semi-controlled randomization */
{
    char *s;
    int i, j, nxt, use_rubout;
    unsigned lth = (unsigned) strlen(engr);

    if (lth && cnt > 0) {
        while (cnt--) {
            /* pick next character */
            if (!seed) {
                /* random */
                nxt = rn2((int) lth);
                use_rubout = rn2(4);
            } else {
                /* predictable; caller can reproduce the same sequence by
                   supplying the same arguments later, or a pseudo-random
                   sequence by varying any of them */
                nxt = seed % lth;
                seed *= 31, seed %= (BUFSZ - 1);
                use_rubout = seed & 3;
            }
            s = &engr[nxt];
            if (*s == ' ')
                continue;

            /* rub out unreadable & small punctuation marks */
            if (index("?.,'`-|_", *s)) {
                *s = ' ';
                continue;
            }

            if (!use_rubout) {
                i = SIZE(rubouts);
            } else {
                for (i = 0; i < SIZE(rubouts); i++)
                    if (*s == rubouts[i].wipefrom) {
                        unsigned ln = (unsigned) strlen(rubouts[i].wipeto);
                        /*
                         * Pick one of the substitutes at random.
                         */
                        if (!seed) {
                            j = rn2((int) ln);
                        } else {
                            seed *= 31, seed %= (BUFSZ - 1);
                            j = seed % ln;
                        }
                        *s = rubouts[i].wipeto[j];
                        break;
                    }
            }

            /* didn't pick rubout; use '?' for unreadable character */
            if (i == SIZE(rubouts))
                *s = '?';
        }
    }

    /* trim trailing spaces */
    while (lth && engr[lth - 1] == ' ')
        engr[--lth] = '\0';
}

/* check whether hero can reach something at ground level */
boolean
can_reach_floor(boolean check_pit)
{
    struct trap *t;

    if (u.uswallow
        || (u.ustuck && !sticks(g.youmonst.data)
            /* assume that arms are pinned rather than that the hero
               has been lifted up above the floor [doesn't explain
               how hero can attack the creature holding him or her;
               that's life in nethack...] */
            && attacktype(u.ustuck->data, AT_HUGS))
        || (Levitation && !(Is_airlevel(&u.uz) || Is_waterlevel(&u.uz))))
        return FALSE;
    /* Restricted/unskilled riders can't reach the floor */
    if (u.usteed && P_SKILL(P_RIDING) < P_BASIC)
        return FALSE;
    if (u.uundetected && ceiling_hider(g.youmonst.data))
        return FALSE;

    if (Flying || g.youmonst.data->msize >= MZ_HUGE)
        return TRUE;

    if (check_pit && (t = t_at(u.ux, u.uy)) != 0
        && (uteetering_at_seen_pit(t) || uescaped_shaft(t)))
        return FALSE;

    return TRUE;
}

/* give a message after caller has determined that hero can't reach */
void
cant_reach_floor(coordxy x, coordxy y, boolean up, boolean check_pit)
{
    You("can't reach the %s.",
        up ? ceiling(x, y)
           : (check_pit && can_reach_floor(FALSE))
               ? "bottom of the pit"
               : surface(x, y));
}

const char *
surface(coordxy x, coordxy y)
{
    struct rm *lev = &levl[x][y];

    if (u_at(x, y) && u.uswallow && is_animal(u.ustuck->data))
        return "maw";
    else if (IS_AIR(lev->typ) && Is_airlevel(&u.uz))
        return "air";
    else if (is_pool(x, y))
        return (Underwater && !Is_waterlevel(&u.uz))
            ? "bottom" : hliquid("water");
    else if (is_ice(x, y))
        return "ice";
    else if (is_lava(x, y))
        return hliquid("lava");
    else if (lev->typ == DRAWBRIDGE_DOWN)
        return "bridge";
    else if (IS_ALTAR(levl[x][y].typ))
        return "altar";
    else if (IS_GRAVE(levl[x][y].typ))
        return "headstone";
    else if (IS_FOUNTAIN(levl[x][y].typ))
        return "fountain";
    else if ((IS_ROOM(lev->typ) && !Is_earthlevel(&u.uz))
             || IS_WALL(lev->typ) || IS_DOOR(lev->typ) || lev->typ == SDOOR)
        return "floor";
    else
        return "ground";
}

const char *
ceiling(coordxy x, coordxy y)
{
    struct rm *lev = &levl[x][y];
    const char *what;

    /* other room types will no longer exist when we're interested --
     * see check_special_room()
     */
    if (*in_rooms(x, y, VAULT))
        what = "vault's ceiling";
    else if (*in_rooms(x, y, TEMPLE))
        what = "temple's ceiling";
    else if (*in_rooms(x, y, SHOPBASE))
        what = "shop's ceiling";
    else if (Is_waterlevel(&u.uz))
        /* water plane has no surface; its air bubbles aren't below sky */
        what = "water above";
    else if (IS_AIR(lev->typ))
        what = "sky";
    else if (Underwater)
        what = "water's surface";
    else if ((IS_ROOM(lev->typ) && !Is_earthlevel(&u.uz))
             || IS_WALL(lev->typ) || IS_DOOR(lev->typ) || lev->typ == SDOOR)
        what = "ceiling";
    else
        what = "rock cavern";

    return what;
}

struct engr *
engr_at(coordxy x, coordxy y)
{
    register struct engr *ep = head_engr;

    while (ep) {
        if (x == ep->engr_x && y == ep->engr_y)
            return ep;
        ep = ep->nxt_engr;
    }
    return (struct engr *) 0;
}

/* Decide whether a particular string is engraved at a specified
 * location; a case-insensitive substring match is used.
 * Ignore headstones, in case the player names herself "Elbereth".
 *
 * If strict checking is requested, the word is only considered to be
 * present if it is intact and is the entire content of the engraving.
 */
boolean
sengr_at(const char *s, coordxy x, coordxy y, boolean strict)
{
    register struct engr *ep = engr_at(x, y);

    if (ep && ep->engr_type != HEADSTONE && ep->engr_time <= g.moves) {
        return (strict ? !strcmpi(ep->engr_txt, s)
                       : (strstri(ep->engr_txt, s) != 0));
    }
    return FALSE;
}

void
u_wipe_engr(int cnt)
{
    if (can_reach_floor(TRUE))
        wipe_engr_at(u.ux, u.uy, cnt, FALSE);
}

void
wipe_engr_at(coordxy x, coordxy y, xint16 cnt, boolean magical)
{
    register struct engr *ep = engr_at(x, y);

    /* Headstones are indelible */
    if (ep && ep->engr_type != HEADSTONE) {
        debugpline1("asked to erode %d characters", cnt);
        if (ep->engr_type != BURN || is_ice(x, y) || (magical && !rn2(2))) {
            if (ep->engr_type != DUST && ep->engr_type != ENGR_BLOOD) {
                cnt = rn2(1 + 50 / (cnt + 1)) ? 0 : 1;
                debugpline1("actually eroding %d characters", cnt);
            }
            wipeout_text(ep->engr_txt, (int) cnt, 0);
            while (ep->engr_txt[0] == ' ')
                ep->engr_txt++;
            if (!ep->engr_txt[0])
                del_engr(ep);
        }
    }
}

void
read_engr_at(coordxy x, coordxy y)
{
    struct engr *ep = engr_at(x, y);
    int sensed = 0;

    /* Sensing an engraving does not require sight,
     * nor does it necessarily imply comprehension (literacy).
     */
    if (ep && ep->engr_txt[0]) {
        switch (ep->engr_type) {
        case DUST:
            if (!Blind) {
                sensed = 1;
                pline("%s is written here in the %s.", Something,
                      is_ice(x, y) ? "frost" : "dust");
            }
            break;
        case ENGRAVE:
        case HEADSTONE:
            if (!Blind || can_reach_floor(TRUE)) {
                sensed = 1;
                pline("%s is engraved here on the %s.", Something,
                      surface(x, y));
            }
            break;
        case BURN:
            if (!Blind || can_reach_floor(TRUE)) {
                sensed = 1;
                pline("Some text has been %s into the %s here.",
                      is_ice(x, y) ? "melted" : "burned", surface(x, y));
            }
            break;
        case MARK:
            if (!Blind) {
                sensed = 1;
                pline("There's some graffiti on the %s here.", surface(x, y));
            }
            break;
        case ENGR_BLOOD:
            /* "It's a message!  Scrawled in blood!"
             * "What's it say?"
             * "It says... `See you next Wednesday.'" -- Thriller
             */
            if (!Blind) {
                sensed = 1;
                You_see("a message scrawled in blood here.");
            }
            break;
        default:
            impossible("%s is written in a very strange way.", Something);
            sensed = 1;
        }

        if (sensed) {
            char *et, buf[BUFSZ];
            int maxelen = (int) (sizeof buf
                                 /* sizeof "literal" counts terminating \0 */
                                 - sizeof "You feel the words: \"\".");

            if ((int) strlen(ep->engr_txt) > maxelen) {
                (void) strncpy(buf, ep->engr_txt, maxelen);
                buf[maxelen] = '\0';
                et = buf;
            } else {
                et = ep->engr_txt;
            }
            You("%s: \"%s\".", (Blind) ? "feel the words" : "read", et);
            if (g.context.run > 0)
                nomul(0);
        }
    }
}

void
make_engr_at(coordxy x, coordxy y, const char *s, long e_time, xint16 e_type)
{
    struct engr *ep;
    unsigned smem = Strlen(s) + 1;

    if ((ep = engr_at(x, y)) != 0)
        del_engr(ep);
    ep = newengr(smem);
    (void) memset((genericptr_t)ep, 0, smem + sizeof(struct engr));
    ep->nxt_engr = head_engr;
    head_engr = ep;
    ep->engr_x = x;
    ep->engr_y = y;
    ep->engr_txt = (char *) (ep + 1);
    Strcpy(ep->engr_txt, s);
    /* engraving Elbereth shows wisdom */
    if (!g.in_mklev && !strcmp(s, "Elbereth"))
        exercise(A_WIS, TRUE);
    ep->engr_time = e_time;
    ep->engr_type = e_type > 0 ? e_type : rnd(N_ENGRAVE - 1);
    ep->engr_lth = smem;
}

/* delete any engraving at location <x,y> */
void
del_engr_at(coordxy x, coordxy y)
{
    struct engr *ep = engr_at(x, y);

    if (ep)
        del_engr(ep);
}

/*
 * freehand - returns true if player has a free hand
 */
int
freehand(void)
{
    return (!uwep || !welded(uwep)
            || (!bimanual(uwep) && (!uarms || !uarms->cursed)));
}

/* getobj callback for an object to engrave with */
static int
stylus_ok(struct obj *obj)
{
    if (!obj)
        return GETOBJ_SUGGEST;

    /* Potential extension: exclude weapons that don't make any sense (such as
     * bullwhips) and downplay rings and gems that wouldn't be good to write
     * with (such as glass and non-gem rings) */
    if (obj->oclass == WEAPON_CLASS || obj->oclass == WAND_CLASS
        || obj->oclass == GEM_CLASS || obj->oclass == RING_CLASS)
        return GETOBJ_SUGGEST;

    /* Only markers and towels are recommended tools. */
    if (obj->oclass == TOOL_CLASS
        && (obj->otyp == TOWEL || obj->otyp == MAGIC_MARKER))
        return GETOBJ_SUGGEST;

    return GETOBJ_DOWNPLAY;
}

/* can hero engrave at all (at their location)? */
static boolean
u_can_engrave(void)
{
    if (u.uswallow) {
        if (is_animal(u.ustuck->data)) {
            pline("What would you write?  \"Jonah was here\"?");
            return FALSE;
        } else if (is_whirly(u.ustuck->data)) {
            cant_reach_floor(u.ux, u.uy, FALSE, FALSE);
            return FALSE;
        }
    } else if (is_lava(u.ux, u.uy)) {
        You_cant("write on the %s!", surface(u.ux, u.uy));
        return FALSE;
    } else if (is_pool(u.ux, u.uy) || IS_FOUNTAIN(levl[u.ux][u.uy].typ)) {
        You_cant("write on the %s!", surface(u.ux, u.uy));
        return FALSE;
    }
    if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz) /* in bubble */) {
        You_cant("write in thin air!");
        return FALSE;
    } else if (!accessible(u.ux, u.uy)) {
        /* stone, tree, wall, secret corridor, pool, lava, bars */
        You_cant("write here.");
        return FALSE;
    }
    if (cantwield(g.youmonst.data)) {
        You_cant("even hold anything!");
        return FALSE;
    }
    if (check_capacity((char *) 0))
        return FALSE;
    return TRUE;
}

/* Mohs' Hardness Scale:
 *  1 - Talc             6 - Orthoclase
 *  2 - Gypsum           7 - Quartz
 *  3 - Calcite          8 - Topaz
 *  4 - Fluorite         9 - Corundum
 *  5 - Apatite         10 - Diamond
 *
 * Since granite is an igneous rock hardness ~ 7, anything >= 8 should
 * probably be able to scratch the rock.
 * Devaluation of less hard gems is not easily possible because obj struct
 * does not contain individual oc_cost currently. 7/91
 *
 * steel      - 5-8.5   (usu. weapon)
 * diamond    - 10                      * jade       -  5-6      (nephrite)
 * ruby       -  9      (corundum)      * turquoise  -  5-6
 * sapphire   -  9      (corundum)      * opal       -  5-6
 * topaz      -  8                      * glass      - ~5.5
 * emerald    -  7.5-8  (beryl)         * dilithium  -  4-5??
 * aquamarine -  7.5-8  (beryl)         * iron       -  4-5
 * garnet     -  7.25   (var. 6.5-8)    * fluorite   -  4
 * agate      -  7      (quartz)        * brass      -  3-4
 * amethyst   -  7      (quartz)        * gold       -  2.5-3
 * jasper     -  7      (quartz)        * silver     -  2.5-3
 * onyx       -  7      (quartz)        * copper     -  2.5-3
 * moonstone  -  6      (orthoclase)    * amber      -  2-2.5
 */

/* the #engrave command */
int
doengrave(void)
{
    boolean dengr = FALSE;    /* TRUE if we wipe out the current engraving */
    boolean doblind = FALSE;  /* TRUE if engraving blinds the player */
    boolean doknown = FALSE;  /* TRUE if we identify the stylus */
    boolean eow = FALSE;      /* TRUE if we are overwriting oep */
    boolean jello = FALSE;    /* TRUE if we are engraving in slime */
    boolean ptext = TRUE;     /* TRUE if we must prompt for engrave text */
    boolean teleengr = FALSE; /* TRUE if we move the old engraving */
    boolean zapwand = FALSE;  /* TRUE if we remove a wand charge */
    xint16 type = DUST;       /* Type of engraving made */
    xint16 oetype = 0;        /* will be set to type of current engraving */
    char buf[BUFSZ];          /* Buffer for final/poly engraving text */
    char ebuf[BUFSZ];         /* Buffer for initial engraving text */
    char fbuf[BUFSZ];         /* Buffer for "your fingers" */
    char qbuf[QBUFSZ];        /* Buffer for query text */
    char post_engr_text[BUFSZ]; /* Text displayed after engraving prompt */
    const char *everb;          /* Present tense of engraving type */
    const char *eloc; /* Where the engraving is (ie dust/floor/...) */
    char *sp;         /* Place holder for space count of engr text */
    size_t len;          /* # of nonspace chars of new engraving text */
    struct engr *oep = engr_at(u.ux, u.uy);
    /* The current engraving */
    struct obj *otmp; /* Object selected with which to engrave */
    char *writer;

    g.multi = 0;              /* moves consumed */
    g.nomovemsg = (char *) 0; /* occupation end message */

    buf[0] = (char) 0;
    ebuf[0] = (char) 0;
    post_engr_text[0] = (char) 0;
    if (oep)
        oetype = oep->engr_type;
    if (is_demon(g.youmonst.data) || is_vampire(g.youmonst.data))
        type = ENGR_BLOOD;

    /* Can the adventurer engrave at all? */
    if (!u_can_engrave())
        return ECMD_OK;

    jello = (u.uswallow && !(is_animal(u.ustuck->data)
                             || is_whirly(u.ustuck->data)));

    /* One may write with finger, or weapon, or wand, or..., or...
     * Edited by GAN 10/20/86 so as not to change weapon wielded.
     */

    otmp = getobj("write with", stylus_ok, GETOBJ_PROMPT);
    if (!otmp) /* otmp == cg.zeroobj if fingers */
        return ECMD_CANCEL;

    if (otmp == &cg.zeroobj) {
        Strcat(strcpy(fbuf, "your "), body_part(FINGERTIP));
        writer = fbuf;
    } else {
        writer = yname(otmp);
    }

    /* There's no reason you should be able to write with a wand
     * while both your hands are tied up.
     */
    if (!freehand() && otmp != uwep && !otmp->owornmask) {
        You("have no free %s to write with!", body_part(HAND));
        return ECMD_OK;
    }

    if (jello) {
        You("tickle %s with %s.", mon_nam(u.ustuck), writer);
        Your("message dissolves...");
        return ECMD_OK;
    }
    if (otmp->oclass != WAND_CLASS && !can_reach_floor(TRUE)) {
        cant_reach_floor(u.ux, u.uy, FALSE, TRUE);
        return ECMD_OK;
    }
    if (IS_ALTAR(levl[u.ux][u.uy].typ)) {
        You("make a motion towards the altar with %s.", writer);
        altar_wrath(u.ux, u.uy);
        return ECMD_OK;
    }
    if (IS_GRAVE(levl[u.ux][u.uy].typ)) {
        if (otmp == &cg.zeroobj) { /* using only finger */
            You("would only make a small smudge on the %s.",
                surface(u.ux, u.uy));
            return ECMD_OK;
        } else if (!levl[u.ux][u.uy].disturbed) {
            You("disturb the undead!");
            levl[u.ux][u.uy].disturbed = 1;
            (void) makemon(&mons[PM_GHOUL], u.ux, u.uy, NO_MM_FLAGS);
            exercise(A_WIS, FALSE);
            return ECMD_TIME;
        }
    }

    /* SPFX for items */

    switch (otmp->oclass) {
    default:
    case AMULET_CLASS:
    case CHAIN_CLASS:
    case POTION_CLASS:
    case COIN_CLASS:
        break;
    case RING_CLASS:
        /* "diamond" rings and others should work */
    case GEM_CLASS:
        /* diamonds & other hard gems should work */
        if (objects[otmp->otyp].oc_tough) {
            type = ENGRAVE;
            break;
        }
        break;
    case ARMOR_CLASS:
        if (is_boots(otmp)) {
            type = DUST;
            break;
        }
        /*FALLTHRU*/
    /* Objects too large to engrave with */
    case BALL_CLASS:
    case ROCK_CLASS:
        You_cant("engrave with such a large object!");
        ptext = FALSE;
        break;
    /* Objects too silly to engrave with */
    case FOOD_CLASS:
    case SCROLL_CLASS:
    case SPBOOK_CLASS:
        pline("%s would get %s.", Yname2(otmp),
              is_ice(u.ux, u.uy) ? "all frosty" : "too dirty");
        ptext = FALSE;
        break;
    case RANDOM_CLASS: /* This should mean fingers */
        break;

    /* The charge is removed from the wand before prompting for
     * the engraving text, because all kinds of setup decisions
     * and pre-engraving messages are based upon knowing what type
     * of engraving the wand is going to do.  Also, the player
     * will have potentially seen "You wrest .." message, and
     * therefore will know they are using a charge.
     */
    case WAND_CLASS:
        if (zappable(otmp)) {
            check_unpaid(otmp);
            if (otmp->cursed && !rn2(WAND_BACKFIRE_CHANCE)) {
                wand_explode(otmp, 0);
                return ECMD_TIME;
            }
            zapwand = TRUE;
            if (!can_reach_floor(TRUE))
                ptext = FALSE;

            switch (otmp->otyp) {
            /* DUST wands */
            default:
                break;
            /* NODIR wands */
            case WAN_LIGHT:
            case WAN_SECRET_DOOR_DETECTION:
            case WAN_CREATE_MONSTER:
            case WAN_WISHING:
            case WAN_ENLIGHTENMENT:
                zapnodir(otmp);
                break;
            /* IMMEDIATE wands */
            /* If wand is "IMMEDIATE", remember to affect the
             * previous engraving even if turning to dust.
             */
            case WAN_STRIKING:
                Strcpy(post_engr_text,
                    "The wand unsuccessfully fights your attempt to write!");
                break;
            case WAN_SLOW_MONSTER:
                if (!Blind) {
                    Sprintf(post_engr_text, "The bugs on the %s slow down!",
                            surface(u.ux, u.uy));
                }
                break;
            case WAN_SPEED_MONSTER:
                if (!Blind) {
                    Sprintf(post_engr_text, "The bugs on the %s speed up!",
                            surface(u.ux, u.uy));
                }
                break;
            case WAN_POLYMORPH:
                if (oep) {
                    if (!Blind) {
                        type = (xint16) 0; /* random */
                        (void) random_engraving(buf);
                    } else {
                        /* keep the same type so that feels don't
                           change and only the text is altered,
                           but you won't know anyway because
                           you're a _blind writer_ */
                        if (oetype)
                            type = oetype;
                        xcrypt(blengr(), buf);
                    }
                    dengr = TRUE;
                }
                break;
            case WAN_NOTHING:
            case WAN_UNDEAD_TURNING:
            case WAN_OPENING:
            case WAN_LOCKING:
            case WAN_PROBING:
                break;
            /* RAY wands */
            case WAN_MAGIC_MISSILE:
                ptext = TRUE;
                if (!Blind) {
                    Sprintf(post_engr_text,
                            "The %s is riddled by bullet holes!",
                            surface(u.ux, u.uy));
                }
                break;
            /* can't tell sleep from death - Eric Backus */
            case WAN_SLEEP:
            case WAN_DEATH:
                if (!Blind) {
                    Sprintf(post_engr_text, "The bugs on the %s stop moving!",
                            surface(u.ux, u.uy));
                }
                break;
            case WAN_COLD:
                if (!Blind)
                    Strcpy(post_engr_text,
                           "A few ice cubes drop from the wand.");
                if (!oep || (oep->engr_type != BURN))
                    break;
                /*FALLTHRU*/
            case WAN_CANCELLATION:
            case WAN_MAKE_INVISIBLE:
                if (oep && oep->engr_type != HEADSTONE) {
                    if (!Blind)
                        pline_The("engraving on the %s vanishes!",
                                  surface(u.ux, u.uy));
                    dengr = TRUE;
                }
                break;
            case WAN_TELEPORTATION:
                if (oep && oep->engr_type != HEADSTONE) {
                    if (!Blind)
                        pline_The("engraving on the %s vanishes!",
                                  surface(u.ux, u.uy));
                    teleengr = TRUE;
                }
                break;
            /* type = ENGRAVE wands */
            case WAN_DIGGING:
                ptext = TRUE;
                type = ENGRAVE;
                if (!objects[otmp->otyp].oc_name_known) {
                    if (Verbose(1, doengrave1))
                        pline("This %s is a wand of digging!", xname(otmp));
                    doknown = TRUE;
                }
                Strcpy(post_engr_text,
                       (Blind && !Deaf)
                          ? "You hear drilling!"    /* Deaf-aware */
                          : Blind
                             ? "You feel tremors."
                             : IS_GRAVE(levl[u.ux][u.uy].typ)
                                 ? "Chips fly out from the headstone."
                                 : is_ice(u.ux, u.uy)
                                    ? "Ice chips fly up from the ice surface!"
                                    : (g.level.locations[u.ux][u.uy].typ
                                       == DRAWBRIDGE_DOWN)
                                       ? "Splinters fly up from the bridge."
                                       : "Gravel flies up from the floor.");
                break;
            /* type = BURN wands */
            case WAN_FIRE:
                ptext = TRUE;
                type = BURN;
                if (!objects[otmp->otyp].oc_name_known) {
                    if (Verbose(1, doengrave2))
                        pline("This %s is a wand of fire!", xname(otmp));
                    doknown = TRUE;
                }
                Strcpy(post_engr_text, Blind ? "You feel the wand heat up."
                                             : "Flames fly from the wand.");
                break;
            case WAN_LIGHTNING:
                ptext = TRUE;
                type = BURN;
                if (!objects[otmp->otyp].oc_name_known) {
                    if (Verbose(1, doengrave3))
                        pline("This %s is a wand of lightning!", xname(otmp));
                    doknown = TRUE;
                }
                if (!Blind) {
                    Strcpy(post_engr_text, "Lightning arcs from the wand.");
                    doblind = TRUE;
                } else {
                    Strcpy(post_engr_text, !Deaf
                                ? "You hear crackling!"     /* Deaf-aware */
                                : "Your hair stands up!");
                }
                break;

            /* type = MARK wands */
            /* type = ENGR_BLOOD wands */
            }
        } else { /* end if zappable */
            /* failing to wrest one last charge takes time */
            ptext = FALSE; /* use "early exit" below, return 1 */
            /* give feedback here if we won't be getting the
               "can't reach floor" message below */
            if (can_reach_floor(TRUE)) {
                /* cancelled wand turns to dust */
                if (otmp->spe < 0)
                    zapwand = TRUE;
                /* empty wand just doesn't write */
                else
                    pline_The("wand is too worn out to engrave.");
            }
        }
        break;

    case WEAPON_CLASS:
        if (otmp->oartifact == ART_FIRE_BRAND)
            type = BURN;
        else if (is_blade(otmp)) {
            if ((int) otmp->spe > -3)
                type = ENGRAVE;
            else
                pline("%s too dull for engraving.", Yobjnam2(otmp, "are"));
        }
        break;

    case TOOL_CLASS:
        if (otmp == ublindf) {
            pline(
                "That is a bit difficult to engrave with, don't you think?");
            return ECMD_OK;
        }
        switch (otmp->otyp) {
        case MAGIC_MARKER:
            if (otmp->spe <= 0)
                Your("marker has dried out.");
            else
                type = MARK;
            break;
        case TOWEL:
            /* Can't really engrave with a towel */
            ptext = FALSE;
            if (oep) {
                if (oep->engr_type == DUST
                    || oep->engr_type == ENGR_BLOOD
                    || oep->engr_type == MARK) {
                    if (is_wet_towel(otmp))
                        dry_a_towel(otmp, -1, TRUE);
                    if (!Blind)
                        You("wipe out the message here.");
                    else
                        pline("%s %s.", Yobjnam2(otmp, "get"),
                              is_ice(u.ux, u.uy) ? "frosty" : "dusty");
                    dengr = TRUE;
                } else {
                    pline("%s can't wipe out this engraving.", Yname2(otmp));
                }
            } else {
                pline("%s %s.", Yobjnam2(otmp, "get"),
                      is_ice(u.ux, u.uy) ? "frosty" : "dusty");
            }
            break;
        default:
            break;
        }
        break;

    case VENOM_CLASS:
        /* this used to be ``if (wizard)'' and fall through to ILLOBJ_CLASS
           for normal play, but splash of venom isn't "illegal" because it
           could occur in normal play via wizard mode bones */
        pline("Writing a poison pen letter?");
        break;

    case ILLOBJ_CLASS:
        impossible("You're engraving with an illegal object!");
        break;
    }

    if (IS_GRAVE(levl[u.ux][u.uy].typ)) {
        if (type == ENGRAVE || type == 0) {
            type = HEADSTONE;
        } else {
            /* ensures the "cannot wipe out" case */
            type = DUST;
            dengr = FALSE;
            teleengr = FALSE;
            buf[0] = '\0';
        }
    }

    /*
     * End of implement setup
     */

    /* Identify stylus */
    if (doknown) {
        learnwand(otmp);
        if (objects[otmp->otyp].oc_name_known)
            more_experienced(0, 10);
    }
    if (teleengr) {
        rloc_engr(oep);
        oep = (struct engr *) 0;
    }
    if (dengr) {
        del_engr(oep);
        oep = (struct engr *) 0;
    }
    /* Something has changed the engraving here */
    if (*buf) {
        make_engr_at(u.ux, u.uy, buf, g.moves, type);
        if (!Blind)
            pline_The("engraving now reads: \"%s\".", buf);
        ptext = FALSE;
    }
    if (zapwand && (otmp->spe < 0)) {
        pline("%s %sturns to dust.", The(xname(otmp)),
              Blind ? "" : "glows violently, then ");
        if (!IS_GRAVE(levl[u.ux][u.uy].typ))
            You(
    "are not going to get anywhere trying to write in the %s with your dust.",
                is_ice(u.ux, u.uy) ? "frost" : "dust");
        useup(otmp);
        otmp = 0; /* wand is now gone */
        ptext = FALSE;
    }
    /* Early exit for some implements. */
    if (!ptext) {
        if (otmp && otmp->oclass == WAND_CLASS && !can_reach_floor(TRUE))
            cant_reach_floor(u.ux, u.uy, FALSE, TRUE);
        return ECMD_TIME;
    }
    /*
     * Special effects should have deleted the current engraving (if
     * possible) by now.
     */
    if (oep) {
        char c = 'n';

        /* Give player the choice to add to engraving. */
        if (type == HEADSTONE) {
            /* no choice, only append */
            c = 'y';
        } else if (type == oep->engr_type
                   && (!Blind || oep->engr_type == BURN
                       || oep->engr_type == ENGRAVE)) {
            c = yn_function("Do you want to add to the current engraving?",
                            ynqchars, 'y');
            if (c == 'q') {
                pline1(Never_mind);
                return ECMD_OK;
            }
        }

        if (c == 'n' || Blind) {
            if (oep->engr_type == DUST
                || oep->engr_type == ENGR_BLOOD
                || oep->engr_type == MARK) {
                if (!Blind) {
                    You("wipe out the message that was %s here.",
                        (oep->engr_type == DUST)
                            ? "written in the dust"
                            : (oep->engr_type == ENGR_BLOOD)
                                ? "scrawled in blood"
                                : "written");
                    del_engr(oep);
                    oep = (struct engr *) 0;
                } else {
                    /* defer deletion until after we *know* we're engraving */
                    eow = TRUE;
                }
            } else if (type == DUST || type == MARK || type == ENGR_BLOOD) {
                You("cannot wipe out the message that is %s the %s here.",
                    oep->engr_type == BURN
                        ? (is_ice(u.ux, u.uy) ? "melted into" : "burned into")
                        : "engraved in",
                    surface(u.ux, u.uy));
                return ECMD_TIME;
            } else if (type != oep->engr_type || c == 'n') {
                if (!Blind || can_reach_floor(TRUE))
                    You("will overwrite the current message.");
                eow = TRUE;
            }
        } else if (oep && (int) strlen(oep->engr_txt) >= BUFSZ - 1) {
            There("is no room to add anything else here.");
            return ECMD_TIME;
        }
    }

    eloc = surface(u.ux, u.uy);
    switch (type) {
    default:
        everb = (oep && !eow ? "add to the weird writing on"
                             : "write strangely on");
        break;
    case DUST:
        everb = (oep && !eow ? "add to the writing in" : "write in");
        eloc = is_ice(u.ux, u.uy) ? "frost" : "dust";
        break;
    case HEADSTONE:
        everb = (oep && !eow ? "add to the epitaph on" : "engrave on");
        break;
    case ENGRAVE:
        everb = (oep && !eow ? "add to the engraving in" : "engrave in");
        break;
    case BURN:
        everb = (oep && !eow
                     ? (is_ice(u.ux, u.uy) ? "add to the text melted into"
                                           : "add to the text burned into")
                     : (is_ice(u.ux, u.uy) ? "melt into" : "burn into"));
        break;
    case MARK:
        everb = (oep && !eow ? "add to the graffiti on" : "scribble on");
        break;
    case ENGR_BLOOD:
        everb = (oep && !eow ? "add to the scrawl on" : "scrawl on");
        break;
    }

    /* Tell adventurer what is going on */
    if (otmp != &cg.zeroobj)
        You("%s the %s with %s.", everb, eloc, doname(otmp));
    else
        You("%s the %s with your %s.", everb, eloc, body_part(FINGERTIP));

    /* Prompt for engraving! */
    Sprintf(qbuf, "What do you want to %s the %s here?", everb, eloc);
    getlin(qbuf, ebuf);
    /* convert tabs to spaces and condense consecutive spaces to one */
    mungspaces(ebuf);

    /* Count the actual # of chars engraved not including spaces */
    len = strlen(ebuf);
    for (sp = ebuf; *sp; sp++)
        if (*sp == ' ')
            len -= 1;

    if (len == 0 || index(ebuf, '\033')) {
        if (zapwand) {
            if (!Blind)
                pline("%s, then %s.", Tobjnam(otmp, "glow"),
                      otense(otmp, "fade"));
            return ECMD_TIME;
        } else {
            pline1(Never_mind);
            return ECMD_OK;
        }
    }

    /* A single `x' is the traditional signature of an illiterate person */
    if (len != 1 || (!index(ebuf, 'x') && !index(ebuf, 'X')))
        if (!u.uconduct.literate++)
            livelog_printf(LL_CONDUCT, "became literate by engraving \"%s\"",
                           ebuf);

    /* Mix up engraving if surface or state of mind is unsound.
       Note: this won't add or remove any spaces. */
    for (sp = ebuf; *sp; sp++) {
        if (*sp == ' ')
            continue;
        if (((type == DUST || type == ENGR_BLOOD) && !rn2(25))
            || (Blind && !rn2(11)) || (Confusion && !rn2(7))
            || (Stunned && !rn2(4)) || (Hallucination && !rn2(2)))
            *sp = ' ' + rnd(96 - 2); /* ASCII '!' thru '~'
                                        (excludes ' ' and DEL) */
    }

    /* Previous engraving is overwritten */
    if (eow) {
        del_engr(oep);
        oep = (struct engr *) 0;
    }

    Strcpy(g.context.engraving.text, ebuf);
    g.context.engraving.nextc = g.context.engraving.text;
    g.context.engraving.stylus = otmp;
    g.context.engraving.type = type;
    g.context.engraving.pos.x = u.ux;
    g.context.engraving.pos.y = u.uy;
    g.context.engraving.actionct = 0;
    set_occupation(engrave, "engraving", 0);

    if (post_engr_text[0])
        pline("%s", post_engr_text);
    if (doblind && !resists_blnd(&g.youmonst)) {
        You("are blinded by the flash!");
        make_blinded((long) rnd(50), FALSE);
        if (!Blind)
            Your1(vision_clears);
    }

    /* Engraving will always take at least one action via being run as an
     * occupation, so do not count this setup as taking time. */
    return ECMD_OK;
}

/* occupation callback for engraving some text */
static int
engrave(void)
{
    struct engr *oep;
    char buf[BUFSZ]; /* holds the post-this-action engr text, including
                      * anything already there */
    const char *finishverb; /* "You finish [foo]." */
    struct obj * stylus; /* shorthand for g.context.engraving.stylus */
    boolean firsttime = (g.context.engraving.actionct == 0);
    int rate = 10; /* # characters that can be engraved in this action */
    boolean truncate = FALSE;

    boolean carving = (g.context.engraving.type == ENGRAVE
                       || g.context.engraving.type == HEADSTONE);
    boolean dulling_wep, marker;
    char *endc; /* points at character 1 beyond the last character to engrave
                   this action */
    int i, space_left;

    if (g.context.engraving.pos.x != u.ux
        || g.context.engraving.pos.y != u.uy) { /* teleported? */
        pline("You are unable to continue engraving.");
        return 0;
    }
    /* Stylus might have been taken out of inventory and destroyed somehow.
     * Not safe to dereference stylus until after this. */
    if (g.context.engraving.stylus == &cg.zeroobj) { /* bare finger */
        stylus = (struct obj *) 0;
    } else {
        for (stylus = g.invent; stylus; stylus = stylus->nobj) {
            if (stylus == g.context.engraving.stylus) {
                break;
            }
        }
        if (!stylus) {
            pline("You are unable to continue engraving.");
            return 0;
        }
    }

    dulling_wep = (carving && stylus && stylus->oclass == WEAPON_CLASS
                   && (stylus->otyp != ATHAME || stylus->cursed));
    marker = (stylus && stylus->otyp == MAGIC_MARKER
              && g.context.engraving.type == MARK);

    g.context.engraving.actionct++;

    /* sanity checks */
    if (dulling_wep && !is_blade(stylus)) {
        impossible("carving with non-bladed weapon");
    } else if (g.context.engraving.type == MARK && !marker) {
        impossible("making graffiti with non-marker stylus");
    }

    /* Step 1: Compute rate. */
    if (carving && stylus
        && (dulling_wep || stylus->oclass == RING_CLASS
            || stylus->oclass == GEM_CLASS)) {
        /* slow engraving methods */
        rate = 1;
    } else if (marker) {
        /* one charge / 2 letters */
        rate = min(rate, stylus->spe * 2);
    }

    /* Step 2: Compute last character that can be engraved this action. */
    i = rate;
    for (endc = g.context.engraving.nextc; *endc && i > 0; endc++) {
        if (*endc != ' ') {
            i--;
        }
    }

    /* Step 3: affect stylus from engraving - it might wear out. */
    if (dulling_wep) {
        /* Dull the weapon at a rate of -1 enchantment per 2 characters,
         * rounding down.
         * The number of characters obtainable given starting enchantment:
         * -2 => 3, -1 => 5, 0 => 7, +1 => 9, +2 => 11
         * Note: this does not allow a +0 anything (except an athame) to
         * engrave "Elbereth" all at once.
         * However, you can engrave "Elb", then "ere", then "th", by taking
         * advantage of the rounding down. */
        if (firsttime) {
            pline("%s dull.", Yobjnam2(stylus, "get"));
        }
        if (g.context.engraving.actionct % 2 == 1) { /* 1st, 3rd, ... action */
            /* deduct a point on 1st, 3rd, 5th, ... turns, unless this is the
             * last character being engraved (a rather convoluted way to round
             * down), but always deduct a point on the 1st turn to prevent
             * zero-cost engravings.
             * Check for truncation *before* deducting a point - otherwise,
             * attempting to e.g. engrave 3 characters with a -2 weapon will
             * stop at the 1st. */
            if (stylus->spe <= -3) {
                if (firsttime) {
                    impossible("<= -3 weapon valid for engraving");
                }
                truncate = TRUE;
            } else if (*endc || g.context.engraving.actionct == 1) {
                stylus->spe -= 1;
                update_inventory();
            }
        }
    } else if (marker) {
        int ink_cost = max(rate / 2, 1); /* Prevent infinite graffiti */

        if (stylus->spe < ink_cost) {
            impossible("overly dry marker valid for graffiti?");
            ink_cost = stylus->spe;
            truncate = TRUE;
        }
        stylus->spe -= ink_cost;
        update_inventory();
        if (stylus->spe == 0) {
            /* can't engrave any further; truncate the string */
            Your("marker dries out.");
            truncate = TRUE;
        }
    }

    switch (g.context.engraving.type) {
    default:
        finishverb = "your weird engraving";
        break;
    case DUST:
        finishverb = "writing in the dust";
        break;
    case HEADSTONE:
    case ENGRAVE:
        finishverb = "engraving";
        break;
    case BURN:
        finishverb = is_ice(u.ux, u.uy) ? "melting your message into the ice"
                     : "burning your message into the floor";
        break;
    case MARK:
        finishverb = "defacing the dungeon";
        break;
    case ENGR_BLOOD:
        finishverb = "scrawling";
    }

    /* actions that happen at the end of every engraving action go here */

    buf[0] = '\0';
    oep = engr_at(u.ux, u.uy);
    if (oep) /* add to existing engraving */
        Strcpy(buf, oep->engr_txt);

    space_left = (int) (sizeof buf - strlen(buf) - 1U);
    if (endc - g.context.engraving.nextc > space_left) {
        You("run out of room to write.");
        endc = g.context.engraving.nextc + space_left;
        truncate = TRUE;
    }

    /* If the stylus did wear out mid-engraving, truncate the input so that we
     * can't go any further. */
    if (truncate && *endc != '\0') {
        *endc = '\0';
        You("are only able to write \"%s\".", g.context.engraving.text);
    } else {
        /* input was not truncated; stylus may still have worn out on the last
         * character, though */
        truncate = FALSE;
    }

    (void) strncat(buf, g.context.engraving.nextc,
                   min(space_left, endc - g.context.engraving.nextc));
    make_engr_at(u.ux, u.uy, buf, g.moves - g.multi, g.context.engraving.type);

    if (*endc) {
        g.context.engraving.nextc = endc;
        return 1; /* not yet finished this turn */
    } else { /* finished engraving */
        /* actions that happen after the engraving is finished go here */

        if (truncate) {
            /* Now that "You are only able to write 'foo'" also prints at the
             * end of engraving, this might be redundant. */
            You("cannot write any more.");
        } else if (!firsttime) {
            /* only print this if engraving took multiple actions */
            You("finish %s.", finishverb);
        }
        g.context.engraving.text[0] = '\0';
        g.context.engraving.nextc = (char *) 0;
        g.context.engraving.stylus = (struct obj *) 0;
    }
    return 0;
}

/* while loading bones, clean up text which might accidentally
   or maliciously disrupt player's terminal when displayed */
void
sanitize_engravings(void)
{
    struct engr *ep;

    for (ep = head_engr; ep; ep = ep->nxt_engr) {
        sanitize_name(ep->engr_txt);
    }
}

void
save_engravings(NHFILE *nhfp)
{
    struct engr *ep, *ep2;
    unsigned no_more_engr = 0;

    for (ep = head_engr; ep; ep = ep2) {
        ep2 = ep->nxt_engr;
        if (ep->engr_lth && ep->engr_txt[0] && perform_bwrite(nhfp)) {
            if (nhfp->structlevel) {
                bwrite(nhfp->fd, (genericptr_t)&(ep->engr_lth),
                       sizeof ep->engr_lth);
                bwrite(nhfp->fd, (genericptr_t)ep,
                       sizeof (struct engr) + ep->engr_lth);
            }
        }
        if (release_data(nhfp))
            dealloc_engr(ep);
    }
    if (perform_bwrite(nhfp)) {
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t)&no_more_engr, sizeof no_more_engr);
    }
    if (release_data(nhfp))
        head_engr = 0;
}

void
rest_engravings(NHFILE *nhfp)
{
    struct engr *ep;
    unsigned lth = 0;

    head_engr = 0;
    while (1) {
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) &lth, sizeof(unsigned));

        if (lth == 0)
            return;
        ep = newengr(lth);
        if (nhfp->structlevel) {
            mread(nhfp->fd, (genericptr_t) ep, sizeof(struct engr) + lth);
        }
        ep->nxt_engr = head_engr;
        head_engr = ep;
        ep->engr_txt = (char *) (ep + 1);	/* Andreas Bormann */
        /* mark as finished for bones levels -- no problem for
         * normal levels as the player must have finished engraving
         * to be able to move again */
        ep->engr_time = g.moves;
    }
}

DISABLE_WARNING_FORMAT_NONLITERAL

/* to support '#stats' wizard-mode command */
void
engr_stats(const char *hdrfmt, char *hdrbuf, long *count, long *size)
{
    struct engr *ep;

    Sprintf(hdrbuf, hdrfmt, (long) sizeof (struct engr));
    *count = *size = 0L;
    for (ep = head_engr; ep; ep = ep->nxt_engr) {
        ++*count;
        *size += (long) sizeof *ep + (long) ep->engr_lth;
    }
}

RESTORE_WARNING_FORMAT_NONLITERAL

void
del_engr(struct engr *ep)
{
    if (ep == head_engr) {
        head_engr = ep->nxt_engr;
    } else {
        struct engr *ept;

        for (ept = head_engr; ept; ept = ept->nxt_engr)
            if (ept->nxt_engr == ep) {
                ept->nxt_engr = ep->nxt_engr;
                break;
            }
        if (!ept) {
            impossible("Error in del_engr?");
            return;
        }
    }
    dealloc_engr(ep);
}

/* randomly relocate an engraving */
void
rloc_engr(struct engr *ep)
{
    int tx, ty, tryct = 200;

    do {
        if (--tryct < 0)
            return;
        tx = rn1(COLNO - 3, 2);
        ty = rn2(ROWNO);
    } while (engr_at(tx, ty) || !goodpos(tx, ty, (struct monst *) 0, 0));

    ep->engr_x = tx;
    ep->engr_y = ty;
}

/* Create a headstone at the given location.
 * The caller is responsible for newsym(x, y).
 */
void
make_grave(coordxy x, coordxy y, const char *str)
{
    char buf[BUFSZ];

    /* Can we put a grave here? */
    if ((levl[x][y].typ != ROOM && levl[x][y].typ != GRAVE) || t_at(x, y))
        return;
    /* Make the grave */
    if (!set_levltyp(x, y, GRAVE))
        return;
    /* Engrave the headstone */
    del_engr_at(x, y);
    if (!str)
        str = get_rnd_text(EPITAPHFILE, buf, rn2, MD_PAD_RUMORS);
    make_engr_at(x, y, str, 0L, HEADSTONE);
    return;
}

static const char blind_writing[][21] = {
    {0x44, 0x66, 0x6d, 0x69, 0x62, 0x65, 0x22, 0x45, 0x7b, 0x71,
     0x65, 0x6d, 0x72, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    {0x51, 0x67, 0x60, 0x7a, 0x7f, 0x21, 0x40, 0x71, 0x6b, 0x71,
     0x6f, 0x67, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x49, 0x6d, 0x73, 0x69, 0x62, 0x65, 0x22, 0x4c, 0x61, 0x7c,
     0x6d, 0x67, 0x24, 0x42, 0x7f, 0x69, 0x6c, 0x77, 0x67, 0x7e, 0x00},
    {0x4b, 0x6d, 0x6c, 0x66, 0x30, 0x4c, 0x6b, 0x68, 0x7c, 0x7f,
     0x6f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x51, 0x67, 0x70, 0x7a, 0x7f, 0x6f, 0x67, 0x68, 0x64, 0x71,
     0x21, 0x4f, 0x6b, 0x6d, 0x7e, 0x72, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x4c, 0x63, 0x76, 0x61, 0x71, 0x21, 0x48, 0x6b, 0x7b, 0x75,
     0x67, 0x63, 0x24, 0x45, 0x65, 0x6b, 0x6b, 0x65, 0x00, 0x00, 0x00},
    {0x4c, 0x67, 0x68, 0x6b, 0x78, 0x68, 0x6d, 0x76, 0x7a, 0x75,
     0x21, 0x4f, 0x71, 0x7a, 0x75, 0x6f, 0x77, 0x00, 0x00, 0x00, 0x00},
    {0x44, 0x66, 0x6d, 0x7c, 0x78, 0x21, 0x50, 0x65, 0x66, 0x65,
     0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x44, 0x66, 0x73, 0x69, 0x62, 0x65, 0x22, 0x56, 0x7d, 0x63,
     0x69, 0x76, 0x6b, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};

static const char *
blengr(void)
{
    return blind_writing[rn2(SIZE(blind_writing))];
}

/*engrave.c*/
