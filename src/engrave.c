/* NetHack 3.6	engrave.c	$NHDT-Date: 1456304550 2016/02/24 09:02:30 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.61 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "lev.h"

STATIC_VAR NEARDATA struct engr *head_engr;

char *
random_engraving(outbuf)
char *outbuf;
{
    const char *rumor;

    /* a random engraving may come from the "rumors" file,
       or from the "engrave" file (formerly in an array here) */
    if (!rn2(4) || !(rumor = getrumor(0, outbuf, TRUE)) || !*rumor)
        (void) get_rnd_text(ENGRAVEFILE, outbuf);

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
wipeout_text(engr, cnt, seed)
char *engr;
int cnt;
unsigned seed; /* for semi-controlled randomization */
{
    char *s;
    int i, j, nxt, use_rubout, lth = (int) strlen(engr);

    if (lth && cnt > 0) {
        while (cnt--) {
            /* pick next character */
            if (!seed) {
                /* random */
                nxt = rn2(lth);
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

            if (!use_rubout)
                i = SIZE(rubouts);
            else
                for (i = 0; i < SIZE(rubouts); i++)
                    if (*s == rubouts[i].wipefrom) {
                        /*
                         * Pick one of the substitutes at random.
                         */
                        if (!seed)
                            j = rn2(strlen(rubouts[i].wipeto));
                        else {
                            seed *= 31, seed %= (BUFSZ - 1);
                            j = seed % (strlen(rubouts[i].wipeto));
                        }
                        *s = rubouts[i].wipeto[j];
                        break;
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
can_reach_floor(check_pit)
boolean check_pit;
{
    struct trap *t;

    if (u.uswallow)
        return FALSE;
    /* Restricted/unskilled riders can't reach the floor */
    if (u.usteed && P_SKILL(P_RIDING) < P_BASIC)
        return FALSE;
    if (check_pit && !Flying
        && (t = t_at(u.ux, u.uy)) != 0 && uteetering_at_seen_pit(t))
        return FALSE;

    return (boolean) ((!Levitation || Is_airlevel(&u.uz)
                       || Is_waterlevel(&u.uz))
                      && (!u.uundetected || !is_hider(youmonst.data)
                          || u.umonnum == PM_TRAPPER));
}

/* give a message after caller has determined that hero can't reach */
void
cant_reach_floor(x, y, up, check_pit)
int x, y;
boolean up, check_pit;
{
    You("can't reach the %s.",
        up ? ceiling(x, y)
           : (check_pit && can_reach_floor(FALSE))
               ? "bottom of the pit"
               : surface(x, y));
}

const char *
surface(x, y)
register int x, y;
{
    register struct rm *lev = &levl[x][y];

    if (x == u.ux && y == u.uy && u.uswallow && is_animal(u.ustuck->data))
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
ceiling(x, y)
register int x, y;
{
    register struct rm *lev = &levl[x][y];
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
engr_at(x, y)
xchar x, y;
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
int
sengr_at(s, x, y, strict)
const char *s;
xchar x, y;
boolean strict;
{
    register struct engr *ep = engr_at(x, y);

    if (ep && ep->engr_type != HEADSTONE && ep->engr_time <= moves) {
        return strict ? (fuzzymatch(ep->engr_txt, s, "", TRUE))
                      : (strstri(ep->engr_txt, s) != 0);
    }

    return FALSE;
}

void
u_wipe_engr(cnt)
int cnt;
{
    if (can_reach_floor(TRUE))
        wipe_engr_at(u.ux, u.uy, cnt, FALSE);
}

void
wipe_engr_at(x, y, cnt, magical)
xchar x, y, cnt, magical;
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
read_engr_at(x, y)
int x, y;
{
    register struct engr *ep = engr_at(x, y);
    int sensed = 0;
    char buf[BUFSZ];

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
            char *et;
            unsigned maxelen = BUFSZ - sizeof("You feel the words: \"\". ");
            if (strlen(ep->engr_txt) > maxelen) {
                (void) strncpy(buf, ep->engr_txt, (int) maxelen);
                buf[maxelen] = '\0';
                et = buf;
            } else
                et = ep->engr_txt;
            You("%s: \"%s\".", (Blind) ? "feel the words" : "read", et);
            if (context.run > 1)
                nomul(0);
        }
    }
}

void
make_engr_at(x, y, s, e_time, e_type)
int x, y;
const char *s;
long e_time;
xchar e_type;
{
    struct engr *ep;
    unsigned smem = strlen(s) + 1;

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
    if (!in_mklev && !strcmp(s, "Elbereth"))
        exercise(A_WIS, TRUE);
    ep->engr_time = e_time;
    ep->engr_type = e_type > 0 ? e_type : rnd(N_ENGRAVE - 1);
    ep->engr_lth = smem;
}

/* delete any engraving at location <x,y> */
void
del_engr_at(x, y)
int x, y;
{
    register struct engr *ep = engr_at(x, y);

    if (ep)
        del_engr(ep);
}

/*
 * freehand - returns true if player has a free hand
 */
int
freehand()
{
    return (!uwep || !welded(uwep)
            || (!bimanual(uwep) && (!uarms || !uarms->cursed)));
}

static NEARDATA const char styluses[] = { ALL_CLASSES, ALLOW_NONE,
                                          TOOL_CLASS,  WEAPON_CLASS,
                                          WAND_CLASS,  GEM_CLASS,
                                          RING_CLASS,  0 };

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

/* return 1 if action took 1 (or more) moves, 0 if error or aborted */
int
doengrave()
{
    boolean dengr = FALSE;    /* TRUE if we wipe out the current engraving */
    boolean doblind = FALSE;  /* TRUE if engraving blinds the player */
    boolean doknown = FALSE;  /* TRUE if we identify the stylus */
    boolean eow = FALSE;      /* TRUE if we are overwriting oep */
    boolean jello = FALSE;    /* TRUE if we are engraving in slime */
    boolean ptext = TRUE;     /* TRUE if we must prompt for engrave text */
    boolean teleengr = FALSE; /* TRUE if we move the old engraving */
    boolean zapwand = FALSE;  /* TRUE if we remove a wand charge */
    xchar type = DUST;        /* Type of engraving made */
    char buf[BUFSZ];          /* Buffer for final/poly engraving text */
    char ebuf[BUFSZ];         /* Buffer for initial engraving text */
    char fbuf[BUFSZ];         /* Buffer for "your fingers" */
    char qbuf[QBUFSZ];        /* Buffer for query text */
    char post_engr_text[BUFSZ]; /* Text displayed after engraving prompt */
    const char *everb;          /* Present tense of engraving type */
    const char *eloc; /* Where the engraving is (ie dust/floor/...) */
    char *sp;         /* Place holder for space count of engr text */
    int len;          /* # of nonspace chars of new engraving text */
    int maxelen;      /* Max allowable length of engraving text */
    struct engr *oep = engr_at(u.ux, u.uy);
    /* The current engraving */
    struct obj *otmp; /* Object selected with which to engrave */
    char *writer;

    multi = 0;              /* moves consumed */
    nomovemsg = (char *) 0; /* occupation end message */

    buf[0] = (char) 0;
    ebuf[0] = (char) 0;
    post_engr_text[0] = (char) 0;
    maxelen = BUFSZ - 1;
    if (is_demon(youmonst.data) || youmonst.data->mlet == S_VAMPIRE)
        type = ENGR_BLOOD;

    /* Can the adventurer engrave at all? */

    if (u.uswallow) {
        if (is_animal(u.ustuck->data)) {
            pline("What would you write?  \"Jonah was here\"?");
            return 0;
        } else if (is_whirly(u.ustuck->data)) {
            cant_reach_floor(u.ux, u.uy, FALSE, FALSE);
            return 0;
        } else
            jello = TRUE;
    } else if (is_lava(u.ux, u.uy)) {
        You_cant("write on the %s!", surface(u.ux, u.uy));
        return 0;
    } else if (is_pool(u.ux, u.uy) || IS_FOUNTAIN(levl[u.ux][u.uy].typ)) {
        You_cant("write on the %s!", surface(u.ux, u.uy));
        return 0;
    }
    if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz) /* in bubble */) {
        You_cant("write in thin air!");
        return 0;
    } else if (!accessible(u.ux, u.uy)) {
        /* stone, tree, wall, secret corridor, pool, lava, bars */
        You_cant("write here.");
        return 0;
    }
    if (cantwield(youmonst.data)) {
        You_cant("even hold anything!");
        return 0;
    }
    if (check_capacity((char *) 0))
        return 0;

    /* One may write with finger, or weapon, or wand, or..., or...
     * Edited by GAN 10/20/86 so as not to change weapon wielded.
     */

    otmp = getobj(styluses, "write with");
    if (!otmp) /* otmp == zeroobj if fingers */
        return 0;

    if (otmp == &zeroobj) {
        Strcat(strcpy(fbuf, "your "), body_part(FINGERTIP));
        writer = fbuf;
    } else
        writer = yname(otmp);

    /* There's no reason you should be able to write with a wand
     * while both your hands are tied up.
     */
    if (!freehand() && otmp != uwep && !otmp->owornmask) {
        You("have no free %s to write with!", body_part(HAND));
        return 0;
    }

    if (jello) {
        You("tickle %s with %s.", mon_nam(u.ustuck), writer);
        Your("message dissolves...");
        return 0;
    }
    if (otmp->oclass != WAND_CLASS && !can_reach_floor(TRUE)) {
        cant_reach_floor(u.ux, u.uy, FALSE, TRUE);
        return 0;
    }
    if (IS_ALTAR(levl[u.ux][u.uy].typ)) {
        You("make a motion towards the altar with %s.", writer);
        altar_wrath(u.ux, u.uy);
        return 0;
    }
    if (IS_GRAVE(levl[u.ux][u.uy].typ)) {
        if (otmp == &zeroobj) { /* using only finger */
            You("would only make a small smudge on the %s.",
                surface(u.ux, u.uy));
            return 0;
        } else if (!levl[u.ux][u.uy].disturbed) {
            You("disturb the undead!");
            levl[u.ux][u.uy].disturbed = 1;
            (void) makemon(&mons[PM_GHOUL], u.ux, u.uy, NO_MM_FLAGS);
            exercise(A_WIS, FALSE);
            return 1;
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
                return 1;
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
                        type = (xchar) 0; /* random */
                        (void) random_engraving(buf);
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
                    if (flags.verbose)
                        pline("This %s is a wand of digging!", xname(otmp));
                    doknown = TRUE;
                }
                Strcpy(post_engr_text,
                       (Blind && !Deaf)
                          ? "You hear drilling!"
                          : Blind
                             ? "You feel tremors."
                             : IS_GRAVE(levl[u.ux][u.uy].typ)
                                 ? "Chips fly out from the headstone."
                                 : is_ice(u.ux, u.uy)
                                    ? "Ice chips fly up from the ice surface!"
                                    : (level.locations[u.ux][u.uy].typ
                                       == DRAWBRIDGE_DOWN)
                                       ? "Splinters fly up from the bridge."
                                       : "Gravel flies up from the floor.");
                break;
            /* type = BURN wands */
            case WAN_FIRE:
                ptext = TRUE;
                type = BURN;
                if (!objects[otmp->otyp].oc_name_known) {
                    if (flags.verbose)
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
                    if (flags.verbose)
                        pline("This %s is a wand of lightning!", xname(otmp));
                    doknown = TRUE;
                }
                if (!Blind) {
                    Strcpy(post_engr_text, "Lightning arcs from the wand.");
                    doblind = TRUE;
                } else
                    Strcpy(post_engr_text, !Deaf
                                ? "You hear crackling!"
                                : "Your hair stands up!");
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
        if (is_blade(otmp)) {
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
            return 0;
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
            if (oep)
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
                } else
                    pline("%s can't wipe out this engraving.", Yname2(otmp));
            else
                pline("%s %s.", Yobjnam2(otmp, "get"),
                      is_ice(u.ux, u.uy) ? "frosty" : "dusty");
            break;
        default:
            break;
        }
        break;

    case VENOM_CLASS:
        if (wizard) {
            pline("Writing a poison pen letter??");
            break;
        }
        /*FALLTHRU*/
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
        make_engr_at(u.ux, u.uy, buf, moves, type);
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
        return 1;
    }
    /*
     * Special effects should have deleted the current engraving (if
     * possible) by now.
     */
    if (oep) {
        register char c = 'n';

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
                return 0;
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
                } else
                    /* Don't delete engr until after we *know* we're engraving
                     */
                    eow = TRUE;
            } else if (type == DUST || type == MARK || type == ENGR_BLOOD) {
                You("cannot wipe out the message that is %s the %s here.",
                    oep->engr_type == BURN
                        ? (is_ice(u.ux, u.uy) ? "melted into" : "burned into")
                        : "engraved in",
                    surface(u.ux, u.uy));
                return 1;
            } else if (type != oep->engr_type || c == 'n') {
                if (!Blind || can_reach_floor(TRUE))
                    You("will overwrite the current message.");
                eow = TRUE;
            }
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
    if (otmp != &zeroobj)
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
            return 1;
        } else {
            pline1(Never_mind);
            return 0;
        }
    }

    /* A single `x' is the traditional signature of an illiterate person */
    if (len != 1 || (!index(ebuf, 'x') && !index(ebuf, 'X')))
        u.uconduct.literate++;

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

    /* Figure out how long it took to engrave, and if player has
     * engraved too much.
     */
    switch (type) {
    default:
        multi = -(len / 10);
        if (multi)
            nomovemsg = "You finish your weird engraving.";
        break;
    case DUST:
        multi = -(len / 10);
        if (multi)
            nomovemsg = "You finish writing in the dust.";
        break;
    case HEADSTONE:
    case ENGRAVE:
        multi = -(len / 10);
        if (otmp->oclass == WEAPON_CLASS
            && (otmp->otyp != ATHAME || otmp->cursed)) {
            multi = -len;
            maxelen = ((otmp->spe + 3) * 2) + 1;
            /* -2 => 3, -1 => 5, 0 => 7, +1 => 9, +2 => 11
             * Note: this does not allow a +0 anything (except an athame)
             * to engrave "Elbereth" all at once.
             * However, you can engrave "Elb", then "ere", then "th".
             */
            pline("%s dull.", Yobjnam2(otmp, "get"));
            costly_alteration(otmp, COST_DEGRD);
            if (len > maxelen) {
                multi = -maxelen;
                otmp->spe = -3;
            } else if (len > 1)
                otmp->spe -= len >> 1;
            else
                otmp->spe -= 1; /* Prevent infinite engraving */
        } else if (otmp->oclass == RING_CLASS || otmp->oclass == GEM_CLASS) {
            multi = -len;
        }
        if (multi)
            nomovemsg = "You finish engraving.";
        break;
    case BURN:
        multi = -(len / 10);
        if (multi)
            nomovemsg = is_ice(u.ux, u.uy)
                          ? "You finish melting your message into the ice."
                          : "You finish burning your message into the floor.";
        break;
    case MARK:
        multi = -(len / 10);
        if (otmp->otyp == MAGIC_MARKER) {
            maxelen = otmp->spe * 2; /* one charge / 2 letters */
            if (len > maxelen) {
                Your("marker dries out.");
                otmp->spe = 0;
                multi = -(maxelen / 10);
            } else if (len > 1)
                otmp->spe -= len >> 1;
            else
                otmp->spe -= 1; /* Prevent infinite graffiti */
        }
        if (multi)
            nomovemsg = "You finish defacing the dungeon.";
        break;
    case ENGR_BLOOD:
        multi = -(len / 10);
        if (multi)
            nomovemsg = "You finish scrawling.";
        break;
    }

    /* Chop engraving down to size if necessary */
    if (len > maxelen) {
        for (sp = ebuf; maxelen && *sp; sp++)
            if (*sp == ' ')
                maxelen--;
        if (!maxelen && *sp) {
            *sp = '\0';
            if (multi)
                nomovemsg = "You cannot write any more.";
            You("are only able to write \"%s\".", ebuf);
        }
    }

    if (oep) /* add to existing engraving */
        Strcpy(buf, oep->engr_txt);
    (void) strncat(buf, ebuf, BUFSZ - (int) strlen(buf) - 1);
    /* Put the engraving onto the map */
    make_engr_at(u.ux, u.uy, buf, moves - multi, type);

    if (post_engr_text[0])
        pline("%s", post_engr_text);
    if (doblind && !resists_blnd(&youmonst)) {
        You("are blinded by the flash!");
        make_blinded((long) rnd(50), FALSE);
        if (!Blind)
            Your1(vision_clears);
    }
    return 1;
}

/* while loading bones, clean up text which might accidentally
   or maliciously disrupt player's terminal when displayed */
void
sanitize_engravings()
{
    struct engr *ep;

    for (ep = head_engr; ep; ep = ep->nxt_engr) {
        sanitize_name(ep->engr_txt);
    }
}

void
save_engravings(fd, mode)
int fd, mode;
{
    struct engr *ep, *ep2;
    unsigned no_more_engr = 0;

    for (ep = head_engr; ep; ep = ep2) {
        ep2 = ep->nxt_engr;
        if (ep->engr_lth && ep->engr_txt[0] && perform_bwrite(mode)) {
            bwrite(fd, (genericptr_t) &ep->engr_lth, sizeof ep->engr_lth);
            bwrite(fd, (genericptr_t) ep, sizeof (struct engr) + ep->engr_lth);
        }
        if (release_data(mode))
            dealloc_engr(ep);
    }
    if (perform_bwrite(mode))
        bwrite(fd, (genericptr_t) &no_more_engr, sizeof no_more_engr);
    if (release_data(mode))
        head_engr = 0;
}

void
rest_engravings(fd)
int fd;
{
    struct engr *ep;
    unsigned lth;

    head_engr = 0;
    while (1) {
        mread(fd, (genericptr_t) &lth, sizeof lth);
        if (lth == 0)
            return;
        ep = newengr(lth);
        mread(fd, (genericptr_t) ep, sizeof (struct engr) + lth);
        ep->nxt_engr = head_engr;
        head_engr = ep;
        ep->engr_txt = (char *) (ep + 1); /* Andreas Bormann */
        /* Mark as finished for bones levels -- no problem for
         * normal levels as the player must have finished engraving
         * to be able to move again.
         */
        ep->engr_time = moves;
    }
}

/* to support '#stats' wizard-mode command */
void
engr_stats(hdrfmt, hdrbuf, count, size)
const char *hdrfmt;
char *hdrbuf;
long *count, *size;
{
    struct engr *ep;

    Sprintf(hdrbuf, hdrfmt, (long) sizeof (struct engr));
    *count = *size = 0L;
    for (ep = head_engr; ep; ep = ep->nxt_engr) {
        ++*count;
        *size += (long) sizeof *ep + (long) ep->engr_lth;
    }
}

void
del_engr(ep)
register struct engr *ep;
{
    if (ep == head_engr) {
        head_engr = ep->nxt_engr;
    } else {
        register struct engr *ept;

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
rloc_engr(ep)
struct engr *ep;
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
make_grave(x, y, str)
int x, y;
const char *str;
{
    char buf[BUFSZ];

    /* Can we put a grave here? */
    if ((levl[x][y].typ != ROOM && levl[x][y].typ != GRAVE) || t_at(x, y))
        return;
    /* Make the grave */
    levl[x][y].typ = GRAVE;
    /* Engrave the headstone */
    del_engr_at(x, y);
    if (!str)
        str = get_rnd_text(EPITAPHFILE, buf);
    make_engr_at(x, y, str, 0L, HEADSTONE);
    return;
}

/*engrave.c*/
