/* NetHack 3.6	pager.c	$NHDT-Date: 1448482543 2015/11/25 20:15:43 $  $NHDT-Branch: master $:$NHDT-Revision: 1.86 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* This file contains the command routines dowhatis() and dohelp() and */
/* a few other help related facilities */

#include "hack.h"
#include "dlb.h"

STATIC_DCL boolean FDECL(is_swallow_sym, (int));
STATIC_DCL int FDECL(append_str, (char *, const char *));
STATIC_DCL void FDECL(look_at_object, (char *, int, int, int));
STATIC_DCL void FDECL(look_at_monster, (char *, char *,
                                        struct monst *, int, int));
STATIC_DCL struct permonst *FDECL(lookat, (int, int, char *, char *));
STATIC_DCL void FDECL(checkfile, (char *, struct permonst *,
                                  BOOLEAN_P, BOOLEAN_P));
STATIC_DCL void FDECL(look_all, (BOOLEAN_P,BOOLEAN_P));
STATIC_DCL boolean FDECL(help_menu, (int *));
STATIC_DCL void NDECL(docontact);
#ifdef PORT_HELP
extern void NDECL(port_help);
#endif

/* Returns "true" for characters that could represent a monster's stomach. */
STATIC_OVL boolean
is_swallow_sym(c)
int c;
{
    int i;

    for (i = S_sw_tl; i <= S_sw_br; i++)
        if ((int) showsyms[i] == c)
            return TRUE;
    return FALSE;
}

/*
 * Append new_str to the end of buf if new_str doesn't already exist as
 * a substring of buf.  Return 1 if the string was appended, 0 otherwise.
 * It is expected that buf is of size BUFSZ.
 */
STATIC_OVL int
append_str(buf, new_str)
char *buf;
const char *new_str;
{
    int space_left; /* space remaining in buf */

    if (strstri(buf, new_str))
        return 0;

    space_left = BUFSZ - strlen(buf) - 1;
    (void) strncat(buf, " or ", space_left);
    (void) strncat(buf, new_str, space_left - 4);
    return 1;
}

/* shared by monster probing (via query_objlist!) as well as lookat() */
char *
self_lookat(outbuf)
char *outbuf;
{
    char race[QBUFSZ];

    /* include race with role unless polymorphed */
    race[0] = '\0';
    if (!Upolyd)
        Sprintf(race, "%s ", urace.adj);
    Sprintf(outbuf, "%s%s%s called %s",
            /* being blinded may hide invisibility from self */
            (Invis && (senseself() || !Blind)) ? "invisible " : "", race,
            mons[u.umonnum].mname, plname);
    if (u.usteed)
        Sprintf(eos(outbuf), ", mounted on %s", y_monnam(u.usteed));
    return outbuf;
}

/* extracted from lookat(); also used by namefloorobj() */
boolean
object_from_map(glyph, x, y, obj_p)
int glyph, x, y;
struct obj **obj_p;
{
    boolean fakeobj = FALSE;
    struct monst *mtmp;
    struct obj *otmp = vobj_at(x, y);
    int glyphotyp = glyph_to_obj(glyph);

    *obj_p = (struct obj *) 0;
    /* there might be a mimic here posing as an object */
    mtmp = m_at(x, y);
    if (mtmp && is_obj_mappear(mtmp, (unsigned) glyphotyp))
        otmp = 0;
    else
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
            otmp->spe = context.current_fruit; /* give it a type */
        if (mtmp && has_mcorpsenm(mtmp)) /* mimic as corpse/statue */
            otmp->corpsenm = MCORPSENM(mtmp);
    }
    /* if located at adjacent spot, mark it as having been seen up close */
    if (otmp && distu(x, y) <= 2 && !Blind && !Hallucination)
        otmp->dknown = 1;

    *obj_p = otmp;
    return fakeobj; /* when True, caller needs to dealloc *obj_p */
}

STATIC_OVL void
look_at_object(buf, x, y, glyph)
char *buf; /* output buffer */
int x, y, glyph;
{
    struct obj *otmp = 0;
    boolean fakeobj = object_from_map(glyph, x, y, &otmp);

    if (otmp) {
        Strcpy(buf, (otmp->otyp != STRANGE_OBJECT)
                     ? distant_name(otmp, xname)
                     : obj_descr[STRANGE_OBJECT].oc_name);
        if (fakeobj)
            dealloc_obj(otmp), otmp = 0;
    } else
        Strcpy(buf, something); /* sanity precaution */

    if (levl[x][y].typ == STONE || levl[x][y].typ == SCORR)
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

STATIC_OVL void
look_at_monster(buf, monbuf, mtmp, x, y)
char *buf, *monbuf; /* buf: output, monbuf: optional output */
struct monst *mtmp;
int x, y;
{
    char *name, monnambuf[BUFSZ];
    boolean accurate = !Hallucination;

    if (mtmp->data == &mons[PM_COYOTE] && accurate)
        name = coyotename(mtmp, monnambuf);
    else
        name = distant_monnam(mtmp, ARTICLE_NONE, monnambuf);

    Sprintf(buf, "%s%s%s",
            (mtmp->mx != x || mtmp->my != y)
                ? ((mtmp->isshk && accurate) ? "tail of " : "tail of a ")
                : "",
            (mtmp->mtame && accurate)
                ? "tame "
                : (mtmp->mpeaceful && accurate)
                    ? "peaceful "
                    : "",
            name);
    if (u.ustuck == mtmp)
        Strcat(buf, (Upolyd && sticks(youmonst.data))
                     ? ", being held" : ", holding you");
    if (mtmp->mleashed)
        Strcat(buf, ", leashed to you");

    if (mtmp->mtrapped && cansee(mtmp->mx, mtmp->my)) {
        struct trap *t = t_at(mtmp->mx, mtmp->my);
        int tt = t ? t->ttyp : NO_TRAP;

        /* newsym lets you know of the trap, so mention it here */
        if (tt == BEAR_TRAP || tt == PIT || tt == SPIKED_PIT || tt == WEB)
            Sprintf(eos(buf), ", trapped in %s",
                    an(defsyms[trap_to_defsym(tt)].explanation));
    }

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
                if (Hallucination)
                    Strcat(monbuf, "paranoid delusion");
                else
                    Sprintf(eos(monbuf), "warned of %s",
                            makeplural(mtmp->data->mname));
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

/*
 * Return the name of the glyph found at (x,y).
 * If not hallucinating and the glyph is a monster, also monster data.
 */
STATIC_OVL struct permonst *
lookat(x, y, buf, monbuf)
int x, y;
char *buf, *monbuf;
{
    struct monst *mtmp = (struct monst *) 0;
    struct permonst *pm = (struct permonst *) 0;
    int glyph;

    buf[0] = monbuf[0] = '\0';
    glyph = glyph_at(x, y);
    if (u.ux == x && u.uy == y && canspotself()) {
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
        if ((Invisible || u.uundetected) && !Blind && !u.uswallow) {
            unsigned how = 0;

            if (Infravision)
                how |= 1;
            if (Unblind_telepat)
                how |= 2;
            if (Detect_monsters)
                how |= 4;

            if (how)
                Sprintf(
                    eos(buf), " [seen: %s%s%s%s%s]",
                    (how & 1) ? "infravision" : "",
                    /* add comma if telep and infrav */
                    ((how & 3) > 2) ? ", " : "", (how & 2) ? "telepathy" : "",
                    /* add comma if detect and (infrav or telep or both) */
                    ((how & 7) > 4) ? ", " : "",
                    (how & 4) ? "monster detection" : "");
        }
    } else if (u.uswallow) {
        /* all locations when swallowed other than the hero are the monster */
        Sprintf(buf, "interior of %s",
                Blind ? "a monster" : a_monnam(u.ustuck));
        pm = u.ustuck->data;
    } else if (glyph_is_monster(glyph)) {
        bhitpos.x = x;
        bhitpos.y = y;
        if ((mtmp = m_at(x, y)) != 0) {
            look_at_monster(buf, monbuf, mtmp, x, y);
            pm = mtmp->data;
        }
    } else if (glyph_is_object(glyph)) {
        look_at_object(buf, x, y, glyph); /* fill in buf[] */
    } else if (glyph_is_trap(glyph)) {
        int tnum = what_trap(glyph_to_trap(glyph));

        Strcpy(buf, defsyms[trap_to_defsym(tnum)].explanation);
    } else if (!glyph_is_cmap(glyph)) {
        Strcpy(buf, "unexplored area");
    } else
        switch (glyph_to_cmap(glyph)) {
        case S_altar:
            Sprintf(buf, "%s %saltar",
                    /* like endgame high priests, endgame high altars
                       are only recognizable when immediately adjacent */
                    (Is_astralevel(&u.uz) && distu(x, y) > 2)
                        ? "aligned"
                        : align_str(
                              Amask2align(levl[x][y].altarmask & ~AM_SHRINE)),
                    ((levl[x][y].altarmask & AM_SHRINE)
                     && (Is_astralevel(&u.uz) || Is_sanctum(&u.uz)))
                        ? "high "
                        : "");
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
        case S_stone:
            if (!levl[x][y].seenv) {
                Strcpy(buf, "unexplored");
                break;
            } else if (levl[x][y].typ == STONE || levl[x][y].typ == SCORR) {
                Strcpy(buf, "stone");
                break;
            }
            /*else FALLTHRU*/
        default:
            Strcpy(buf, defsyms[glyph_to_cmap(glyph)].explanation);
            break;
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
STATIC_OVL void
checkfile(inp, pm, user_typed_name, without_asking)
char *inp;
struct permonst *pm;
boolean user_typed_name, without_asking;
{
    dlb *fp;
    char buf[BUFSZ], newstr[BUFSZ];
    char *ep, *dbase_str;
    unsigned long txt_offset = 0L;
    int chk_skip;
    boolean found_in_file = FALSE, skipping_entry = FALSE;
    winid datawin = WIN_ERR;

    fp = dlb_fopen(DATAFILE, "r");
    if (!fp) {
        pline("Cannot open data file!");
        return;
    }

    /*
     * If someone passed us garbage, prevent fault.
     */
    if (!inp || (inp && strlen(inp) > (BUFSZ - 1))) {
        pline("bad do_look buffer passed!");
        return;
    }

    /* To prevent the need for entries in data.base like *ngel to account
     * for Angel and angel, make the lookup string the same for both
     * user_typed_name and picked name.
     */
    if (pm != (struct permonst *) 0 && !user_typed_name)
        dbase_str = strcpy(newstr, pm->mname);
    else
        dbase_str = strcpy(newstr, inp);
    (void) lcase(dbase_str);

    if (!strncmp(dbase_str, "interior of ", 12))
        dbase_str += 12;
    if (!strncmp(dbase_str, "a ", 2))
        dbase_str += 2;
    else if (!strncmp(dbase_str, "an ", 3))
        dbase_str += 3;
    else if (!strncmp(dbase_str, "the ", 4))
        dbase_str += 4;
    if (!strncmp(dbase_str, "tame ", 5))
        dbase_str += 5;
    else if (!strncmp(dbase_str, "peaceful ", 9))
        dbase_str += 9;
    if (!strncmp(dbase_str, "invisible ", 10))
        dbase_str += 10;
    if (!strncmp(dbase_str, "saddled ", 8))
        dbase_str += 8;
    if (!strncmp(dbase_str, "statue of ", 10))
        dbase_str[6] = '\0';
    else if (!strncmp(dbase_str, "figurine of ", 12))
        dbase_str[8] = '\0';

    /* Make sure the name is non-empty. */
    if (*dbase_str) {
        /* adjust the input to remove "named " and convert to lower case */
        char *alt = 0; /* alternate description */

        if ((ep = strstri(dbase_str, " named ")) != 0)
            alt = ep + 7;
        else
            ep = strstri(dbase_str, " called ");
        if (!ep)
            ep = strstri(dbase_str, ", ");
        if (ep && ep > dbase_str)
            *ep = '\0';

        /*
         * If the object is named, then the name is the alternate description;
         * otherwise, the result of makesingular() applied to the name is.
         * This
         * isn't strictly optimal, but named objects of interest to the user
         * will usually be found under their name, rather than under their
         * object type, so looking for a singular form is pointless.
         */
        if (!alt)
            alt = makesingular(dbase_str);

        /* skip first record; read second */
        txt_offset = 0L;
        if (!dlb_fgets(buf, BUFSZ, fp) || !dlb_fgets(buf, BUFSZ, fp)) {
            impossible("can't read 'data' file");
            (void) dlb_fclose(fp);
            return;
        } else if (sscanf(buf, "%8lx\n", &txt_offset) < 1 || txt_offset == 0L)
            goto bad_data_file;

        /* look for the appropriate entry */
        while (dlb_fgets(buf, BUFSZ, fp)) {
            if (*buf == '.')
                break; /* we passed last entry without success */

            if (digit(*buf)) {
                /* a number indicates the end of current entry */
                skipping_entry = FALSE;
            } else if (!skipping_entry) {
                if (!(ep = index(buf, '\n')))
                    goto bad_data_file;
                *ep = 0;
                /* if we match a key that begins with "~", skip this entry */
                chk_skip = (*buf == '~') ? 1 : 0;
                if (pmatch(&buf[chk_skip], dbase_str)
                    || (alt && pmatch(&buf[chk_skip], alt))) {
                    if (chk_skip) {
                        skipping_entry = TRUE;
                        continue;
                    } else {
                        found_in_file = TRUE;
                        break;
                    }
                }
            }
        }
    }

    if (found_in_file) {
        long entry_offset;
        int entry_count;
        int i;

        /* skip over other possible matches for the info */
        do {
            if (!dlb_fgets(buf, BUFSZ, fp))
                goto bad_data_file;
        } while (!digit(*buf));
        if (sscanf(buf, "%ld,%d\n", &entry_offset, &entry_count) < 2) {
        bad_data_file:
            impossible("'data' file in wrong format or corrupted");
            /* window will exist if we came here from below via 'goto' */
            if (datawin != WIN_ERR)
                destroy_nhwindow(datawin);
            (void) dlb_fclose(fp);
            return;
        }

        if (user_typed_name || without_asking || yn("More info?") == 'y') {
            if (dlb_fseek(fp, (long) txt_offset + entry_offset, SEEK_SET)
                < 0) {
                pline("? Seek error on 'data' file!");
                (void) dlb_fclose(fp);
                return;
            }
            datawin = create_nhwindow(NHW_MENU);
            for (i = 0; i < entry_count; i++) {
                if (!dlb_fgets(buf, BUFSZ, fp))
                    goto bad_data_file;
                if ((ep = index(buf, '\n')) != 0)
                    *ep = 0;
                if (index(buf + 1, '\t') != 0)
                    (void) tabexpand(buf + 1);
                putstr(datawin, 0, buf + 1);
            }
            display_nhwindow(datawin, FALSE);
            destroy_nhwindow(datawin);
        }
    } else if (user_typed_name)
        pline("I don't have any information on those things.");

    (void) dlb_fclose(fp);
}

int
do_screen_description(cc, looked, sym, out_str, firstmatch)
coord cc;
boolean looked;
int sym;
char *out_str;
const char **firstmatch;
{
    boolean need_to_look = FALSE;
    int glyph = NO_GLYPH;
    static char look_buf[BUFSZ];
    char prefix[BUFSZ];
    int found = 0; /* count of matching syms found */
    int i, alt_i;
    int skipped_venom = 0;
    boolean hit_trap;
    const char *x_str;
    static const char *mon_interior = "the interior of a monster";

    if (looked) {
        int oc;
        unsigned os;

        glyph = glyph_at(cc.x, cc.y);
        /* Convert glyph at selected position to a symbol for use below. */
        (void) mapglyph(glyph, &sym, &oc, &os, cc.x, cc.y);

        Sprintf(prefix, "%s        ", encglyph(glyph));
    } else
        Sprintf(prefix, "%c        ", sym);

    /*
     * Check all the possibilities, saving all explanations in a buffer.
     * When all have been checked then the string is printed.
     */

    /*
     * Special case: if identifying from the screen, and we're swallowed,
     * and looking at something other than our own symbol, then just say
     * "the interior of a monster".
     */
    if (u.uswallow && looked
        && (is_swallow_sym(sym) || (int) showsyms[S_stone] == sym)) {
        if (!found) {
            Sprintf(out_str, "%s%s", prefix, mon_interior);
            *firstmatch = mon_interior;
        } else {
            found += append_str(out_str, mon_interior);
        }
        need_to_look = TRUE;
        goto didlook;
    }

    /* Check for monsters */
    for (i = 0; i < MAXMCLASSES; i++) {
        if (sym == (looked ? showsyms[i + SYM_OFF_M] : def_monsyms[i].sym)
            && def_monsyms[i].explain) {
            need_to_look = TRUE;
            if (!found) {
                Sprintf(out_str, "%s%s", prefix, an(def_monsyms[i].explain));
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
    if ((looked ? (sym == showsyms[S_HUMAN + SYM_OFF_M]
                   && cc.x == u.ux && cc.y == u.uy)
                : (sym == def_monsyms[S_HUMAN].sym && !flags.showrace))
        && !(Race_if(PM_HUMAN) || Race_if(PM_ELF)) && !Upolyd)
        found += append_str(out_str, "you"); /* tack on "or you" */

    /* Now check for objects */
    for (i = 1; i < MAXOCLASSES; i++) {
        if (sym == (looked ? showsyms[i + SYM_OFF_O] : def_oc_syms[i].sym)) {
            need_to_look = TRUE;
            if (looked && i == VENOM_CLASS) {
                skipped_venom++;
                continue;
            }
            if (!found) {
                Sprintf(out_str, "%s%s", prefix, an(def_oc_syms[i].explain));
                *firstmatch = def_oc_syms[i].explain;
                found++;
            } else {
                found += append_str(out_str, an(def_oc_syms[i].explain));
            }
        }
    }

    if (sym == DEF_INVISIBLE) {
        if (!found) {
            Sprintf(out_str, "%s%s", prefix, an(invisexplain));
            *firstmatch = invisexplain;
            found++;
        } else {
            found += append_str(out_str, an(invisexplain));
        }
    }

#define is_cmap_trap(i) ((i) >= S_arrow_trap && (i) <= S_polymorph_trap)
#define is_cmap_drawbridge(i) ((i) >= S_vodbridge && (i) <= S_hcdbridge)

    /* Now check for graphics symbols */
    alt_i = (sym == (looked ? showsyms[0] : defsyms[0].sym)) ? 0 : (2 + 1);
    for (hit_trap = FALSE, i = 0; i < MAXPCHARS; i++) {
        /* when sym is the default background character, we process
           i == 0 three times: unexplored, stone, dark part of a room */
        if (alt_i < 2) {
            x_str = !alt_i++ ? "unexplored" : "stone";
            i = 0; /* for second iteration, undo loop increment */
            /* alt_i is now 1 or 2 */
        } else {
            if (alt_i++ == 2)
                i = 0; /* undo loop increment */
            x_str = defsyms[i].explanation;
            /* alt_i is now 3 or more and no longer of interest */
        }
        if (sym == (looked ? showsyms[i] : defsyms[i].sym) && *x_str) {
            /* avoid "an unexplored", "an stone", "an air", "a water",
               "a floor of a room", "a dark part of a room";
               article==2 => "the", 1 => "an", 0 => (none) */
            int article = strstri(x_str, " of a room") ? 2
                          : !(alt_i <= 2
                              || strcmp(x_str, "air") == 0
                              || strcmp(x_str, "water") == 0);

            if (!found) {
                if (is_cmap_trap(i)) {
                    Sprintf(out_str, "%sa trap", prefix);
                    hit_trap = TRUE;
                } else {
                    Sprintf(out_str, "%s%s", prefix,
                            article == 2 ? the(x_str)
                            : article == 1 ? an(x_str) : x_str);
                }
                *firstmatch = x_str;
                found++;
            } else if (!u.uswallow && !(hit_trap && is_cmap_trap(i))
                       && !(found >= 3 && is_cmap_drawbridge(i))
                       /* don't mention vibrating square outside of Gehennom
                          unless this happens to be one (hallucination?) */
                       && (i != S_vibrating_square || Inhell
                           || (looked && glyph_is_trap(glyph)
                               && glyph_to_trap(glyph) == VIBRATING_SQUARE))) {
                found += append_str(out_str, (article == 2) ? the(x_str)
                                             : (article == 1) ? an(x_str)
                                               : x_str);
                if (is_cmap_trap(i))
                    hit_trap = TRUE;
            }

            if (i == S_altar || is_cmap_trap(i))
                need_to_look = TRUE;
        }
    }

    /* Now check for warning symbols */
    for (i = 1; i < WARNCOUNT; i++) {
        x_str = def_warnsyms[i].explanation;
        if (sym == (looked ? warnsyms[i] : def_warnsyms[i].sym)) {
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

    /* handle optional boulder symbol as a special case */
    if (iflags.bouldersym && sym == iflags.bouldersym) {
        if (!found) {
            *firstmatch = "boulder";
            Sprintf(out_str, "%s%s", prefix, an(*firstmatch));
            found++;
        } else {
            found += append_str(out_str, "boulder");
        }
    }

    /*
     * If we are looking at the screen, follow multiple possibilities or
     * an ambiguous explanation by something more detailed.
     */
 didlook:
    if (looked) {
        if (found > 1 || need_to_look) {
            char monbuf[BUFSZ];
            char temp_buf[BUFSZ];

            (void) lookat(cc.x, cc.y, look_buf, monbuf);
            *firstmatch = look_buf;
            if (*(*firstmatch)) {
                Sprintf(temp_buf, " (%s)", *firstmatch);
                (void) strncat(out_str, temp_buf,
                               BUFSZ - strlen(out_str) - 1);
                found = 1; /* we have something to look up */
            }
            if (monbuf[0]) {
                Sprintf(temp_buf, " [seen: %s]", monbuf);
                (void) strncat(out_str, temp_buf,
                               BUFSZ - strlen(out_str) - 1);
            }
        }
    }

    return found;
}

/* getpos() return values */
#define LOOK_TRADITIONAL 0 /* '.' -- ask about "more info?" */
#define LOOK_QUICK 1       /* ',' -- skip "more info?" */
#define LOOK_ONCE 2        /* ';' -- skip and stop looping */
#define LOOK_VERBOSE 3     /* ':' -- show more info w/o asking */

/* also used by getpos hack in do_name.c */
const char what_is_an_unknown_object[] = "an unknown object";

int
do_look(mode, click_cc)
int mode;
coord *click_cc;
{
    boolean quick = (mode == 1); /* use cursor; don't search for "more info" */
    boolean clicklook = (mode == 2); /* right mouse-click method */
    char out_str[BUFSZ];
    const char *firstmatch = 0;
    struct permonst *pm = 0;
    int i = '\0', ans = 0;
    int sym;              /* typed symbol or converted glyph */
    int found;            /* count of matching syms found */
    coord cc;             /* screen pos of unknown glyph */
    boolean save_verbose; /* saved value of flags.verbose */
    boolean from_screen;  /* question from the screen */

    if (!clicklook) {
        if (quick) {
            from_screen = TRUE; /* yes, we want to use the cursor */
            i = 'y';
        }

        if (i != 'y') {
            menu_item *pick_list = (menu_item *) 0;
            winid win;
            anything any;

            any = zeroany;
            win = create_nhwindow(NHW_MENU);
            start_menu(win);
            any.a_char = '/';
            /* 'y' and 'n' to keep backwards compatibility with previous
               versions: "Specify unknown object by cursor?" */
            add_menu(win, NO_GLYPH, &any,
                     flags.lootabc ? 0 : any.a_char, 'y', ATR_NONE,
                     "something on the map", MENU_UNSELECTED);
            any.a_char = 'i';
            add_menu(win, NO_GLYPH, &any,
                     flags.lootabc ? 0 : any.a_char, 0, ATR_NONE,
                     "something you're carrying", MENU_UNSELECTED);
            any.a_char = '?';
            add_menu(win, NO_GLYPH, &any,
                     flags.lootabc ? 0 : any.a_char, 'n', ATR_NONE,
                     "something else (by symbol or name)", MENU_UNSELECTED);
            if (!u.uswallow && !Hallucination) {
                any = zeroany;
                add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE,
                         "", MENU_UNSELECTED);
                /* these options work sensibly for the swallowed case,
                   but there's no reason for the player to use them then;
                   objects work fine when hallucinating, but screen
                   symbol/monster class letter doesn't match up with
                   bogus monster type, so suppress when hallucinating */
                any.a_char = 'm';
                add_menu(win, NO_GLYPH, &any,
                         flags.lootabc ? 0 : any.a_char, 0, ATR_NONE,
                         "nearby monsters", MENU_UNSELECTED);
                any.a_char = 'M';
                add_menu(win, NO_GLYPH, &any,
                         flags.lootabc ? 0 : any.a_char, 0, ATR_NONE,
                         "all monsters shown on map", MENU_UNSELECTED);
                any.a_char = 'o';
                add_menu(win, NO_GLYPH, &any,
                         flags.lootabc ? 0 : any.a_char, 0, ATR_NONE,
                         "nearby objects", MENU_UNSELECTED);
                any.a_char = 'O';
                add_menu(win, NO_GLYPH, &any,
                         flags.lootabc ? 0 : any.a_char, 0, ATR_NONE,
                         "all objects shown on map", MENU_UNSELECTED);
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
            return 0;
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
                return 0;
            *out_str = '\0';
            for (invobj = invent; invobj; invobj = invobj->nobj)
                if (invobj->invlet == invlet) {
                    strcpy(out_str, singular(invobj, xname));
                    break;
                }
            if (*out_str)
                checkfile(out_str, pm, TRUE, TRUE);
            return 0;
          }
        case '?':
            from_screen = FALSE;
            getlin("Specify what? (type the word)", out_str);
            if (out_str[0] == '\0' || out_str[0] == '\033')
                return 0;

            if (out_str[1]) { /* user typed in a complete string */
                checkfile(out_str, pm, TRUE, TRUE);
                return 0;
            }
            sym = out_str[0];
            break;
        case 'm':
            look_all(TRUE, TRUE); /* list nearby monsters */
            return 0;
        case 'M':
            look_all(FALSE, TRUE); /* list all monsters */
            return 0;
        case 'o':
            look_all(TRUE, FALSE); /* list nearby objects */
            return 0;
        case 'O':
            look_all(FALSE, FALSE); /* list all objects */
            return 0;
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
                if (flags.verbose)
                    pline("Please move the cursor to %s.",
                          what_is_an_unknown_object);
                else
                    pline("Pick an object.");

                ans = getpos(&cc, quick, what_is_an_unknown_object);
                if (ans < 0 || cc.x < 0) {
                    flags.verbose = save_verbose;
                    return 0; /* done */
                }
                flags.verbose = FALSE; /* only print long question once */
            }
        }

        found = do_screen_description(cc, (from_screen || clicklook), sym,
                                      out_str, &firstmatch);

        /* Finally, print out our explanation. */
        if (found) {
            /* Used putmixed() because there may be an encoded glyph present
             */
            putmixed(WIN_MESSAGE, 0, out_str);

            /* check the data file for information about this thing */
            if (found == 1 && ans != LOOK_QUICK && ans != LOOK_ONCE
                && (ans == LOOK_VERBOSE || (flags.help && !quick))
                && !clicklook) {
                char temp_buf[BUFSZ];

                Strcpy(temp_buf, firstmatch);
                checkfile(temp_buf, pm, FALSE,
                          (boolean) (ans == LOOK_VERBOSE));
            }
        } else {
            pline("I've never heard of such things.");
        }

    } while (from_screen && !quick && ans != LOOK_ONCE && !clicklook);

    flags.verbose = save_verbose;
    return 0;
}

STATIC_OVL void
look_all(nearby, do_mons)
boolean nearby; /* True => within BOLTLIM, False => entire map */
boolean do_mons; /* True => monsters, False => objects */
{
    winid win;
    int x, y, lo_x, lo_y, hi_x, hi_y, glyph, count = 0;
    char buf[BUFSZ], outbuf[BUFSZ], coordbuf[12], fmt[12]; /* "%02d,%02d" */

    /* row,column orientation rather than cartesian x,y */
    Sprintf(fmt, "%%%sd,%%%sd",
            (ROWNO <= 100) ? "02" : (ROWNO <= 1000) ? "03" : "",
            (COLNO <= 100) ? "02" : (COLNO <= 1000) ? "03" : "");

    win = create_nhwindow(NHW_TEXT);
    lo_y = nearby ? max(u.uy - BOLT_LIM, 0) : 0;
    lo_x = nearby ? max(u.ux - BOLT_LIM, 1) : 1;
    hi_y = nearby ? min(u.uy + BOLT_LIM, ROWNO - 1) : ROWNO - 1;
    hi_x = nearby ? min(u.ux + BOLT_LIM, COLNO - 1) : COLNO - 1;
    for (y = lo_y; y <= hi_y; y++) {
        for (x = lo_x; x <= hi_x; x++) {
            buf[0] = '\0';
            glyph = glyph_at(x, y);
            if (do_mons) {
                if (glyph_is_monster(glyph)) {
                    struct monst *mtmp;

                    bhitpos.x = x; /* [is this actually necessary?] */
                    bhitpos.y = y;
                    if (x == u.ux && y == u.uy && canspotself()) {
                        (void) self_lookat(buf);
                        ++count;
                    } else if ((mtmp = m_at(x, y)) != 0) {
                        look_at_monster(buf, (char *) 0, mtmp, x, y);
                        ++count;
                    }
                } else if (glyph_is_invisible(glyph)) {
                    /* remembered, unseen, creature */
                    Strcpy(buf, invisexplain);
                    ++count;
                } else if (glyph_is_warning(glyph)) {
                    int warnindx = glyph_to_warning(glyph);

                    Strcpy(buf, def_warnsyms[warnindx].explanation);
                    ++count;
                }
            } else { /* !do_mons */
                if (glyph_is_object(glyph)) {
                    look_at_object(buf, x, y, glyph);
                    ++count;
                }
            }
            if (*buf) {
                if (count == 1) {
                    char which[12];

                    Strcpy(which, do_mons ? "monsters" : "objects");
                    if (nearby) {
                        Sprintf(coordbuf, fmt, u.uy, u.ux);
                        Sprintf(outbuf, "%s currently shown near %s:",
                                upstart(which), coordbuf);
                    } else
                        Sprintf(outbuf, "All %s currently shown on the map:",
                                which);
                    putstr(win, 0, outbuf);
                    putstr(win, 0, "");
                }
                Sprintf(coordbuf, fmt, y, x);
                /* prefix: "C  row,column  " */
                Sprintf(outbuf, "%s  %s  ", encglyph(glyph), coordbuf);
                /* guard against potential overflow */
                buf[sizeof buf - 1 - strlen(outbuf)] = '\0';
                Strcat(outbuf, buf);
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

/* the '/' command */
int
dowhatis()
{
    return do_look(0, (coord *) 0);
}

/* the ';' command */
int
doquickwhatis()
{
    return do_look(1, (coord *) 0);
}

/* the '^' command */
int
doidtrap()
{
    register struct trap *trap;
    int x, y, tt;

    if (!getdir("^"))
        return 0;
    x = u.ux + u.dx;
    y = u.uy + u.dy;
    for (trap = ftrap; trap; trap = trap->ntrap)
        if (trap->tx == x && trap->ty == y) {
            if (!trap->tseen)
                break;
            tt = trap->ttyp;
            if (u.dz) {
                if (u.dz < 0 ? (tt == TRAPDOOR || tt == HOLE)
                             : tt == ROCKTRAP)
                    break;
            }
            tt = what_trap(tt);
            pline("That is %s%s%s.",
                  an(defsyms[trap_to_defsym(tt)].explanation),
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
            return 0;
        }
    pline("I can't see a trap there.");
    return 0;
}

char *
dowhatdoes_core(q, cbuf)
char q;
char *cbuf;
{
    dlb *fp;
    char bufr[BUFSZ];
    register char *buf = &bufr[6], *ep, ctrl, meta;

    fp = dlb_fopen(CMDHELPFILE, "r");
    if (!fp) {
        pline("Cannot open data file!");
        return 0;
    }

    ctrl = ((q <= '\033') ? (q - 1 + 'A') : 0);
    meta = ((0x80 & q) ? (0x7f & q) : 0);
    while (dlb_fgets(buf, BUFSZ - 6, fp)) {
        if ((ctrl && *buf == '^' && *(buf + 1) == ctrl)
            || (meta && *buf == 'M' && *(buf + 1) == '-'
                && *(buf + 2) == meta) || *buf == q) {
            ep = index(buf, '\n');
            if (ep)
                *ep = 0;
            if (ctrl && buf[2] == '\t') {
                buf = bufr + 1;
                (void) strncpy(buf, "^?      ", 8);
                buf[1] = ctrl;
            } else if (meta && buf[3] == '\t') {
                buf = bufr + 2;
                (void) strncpy(buf, "M-?     ", 8);
                buf[2] = meta;
            } else if (buf[1] == '\t') {
                buf = bufr;
                buf[0] = q;
                (void) strncpy(buf + 1, "       ", 7);
            }
            (void) dlb_fclose(fp);
            Strcpy(cbuf, buf);
            return cbuf;
        }
    }
    (void) dlb_fclose(fp);
    return (char *) 0;
}

int
dowhatdoes()
{
    char bufr[BUFSZ];
    char q, *reslt;

#if defined(UNIX) || defined(VMS)
    introff();
#endif
    q = yn_function("What command?", (char *) 0, '\0');
#if defined(UNIX) || defined(VMS)
    intron();
#endif
    reslt = dowhatdoes_core(q, bufr);
    if (reslt)
        pline1(reslt);
    else
        pline("I've never heard of such commands.");
    return 0;
}

void
docontact()
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

/* data for help_menu() */
static const char *help_menu_items[] = {
    /*  0*/ "About NetHack (version information).",
    /*  1*/ "Long description of the game and commands.",
    /*  2*/ "List of game commands.",
    /*  3*/ "Concise history of NetHack.",
    /*  4*/ "Info on a character in the game display.",
    /*  5*/ "Info on what a given key does.",
    /*  6*/ "List of game options.",
    /*  7*/ "Longer explanation of game options.",
    /*  8*/ "List of extended commands.",
    /*  9*/ "The NetHack license.",
    /* 10*/ "Support information.",
#ifdef PORT_HELP
    "%s-specific help and commands.",
#define PORT_HELP_ID 100
#define WIZHLP_SLOT 12
#else
#define WIZHLP_SLOT 11
#endif
    "List of wizard-mode commands.", "", (char *) 0
};

STATIC_OVL boolean
help_menu(sel)
int *sel;
{
    winid tmpwin = create_nhwindow(NHW_MENU);
#ifdef PORT_HELP
    char helpbuf[QBUFSZ];
#endif
    int i, n;
    menu_item *selected;
    anything any;

    any = zeroany; /* zero all bits */
    start_menu(tmpwin);
    if (!wizard)
        help_menu_items[WIZHLP_SLOT] = "",
        help_menu_items[WIZHLP_SLOT + 1] = (char *) 0;
    for (i = 0; help_menu_items[i]; i++)
#ifdef PORT_HELP
        /* port-specific line has a %s in it for the PORT_ID */
        if (help_menu_items[i][0] == '%') {
            Sprintf(helpbuf, help_menu_items[i], PORT_ID);
            any.a_int = PORT_HELP_ID + 1;
            add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, helpbuf,
                     MENU_UNSELECTED);
        } else
#endif
        {
            any.a_int = (*help_menu_items[i]) ? i + 1 : 0;
            add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
                     help_menu_items[i], MENU_UNSELECTED);
        }
    end_menu(tmpwin, "Select one item:");
    n = select_menu(tmpwin, PICK_ONE, &selected);
    destroy_nhwindow(tmpwin);
    if (n > 0) {
        *sel = selected[0].item.a_int - 1;
        free((genericptr_t) selected);
        return TRUE;
    }
    return FALSE;
}

/* the '?' command */
int
dohelp()
{
    int sel = 0;

    if (help_menu(&sel)) {
        switch (sel) {
        case 0:
            (void) doextversion();
            break;
        case 1:
            display_file(HELP, TRUE);
            break;
        case 2:
            display_file(SHELP, TRUE);
            break;
        case 3:
            (void) dohistory();
            break;
        case 4:
            (void) dowhatis();
            break;
        case 5:
            (void) dowhatdoes();
            break;
        case 6:
            option_help();
            break;
        case 7:
            display_file(OPTIONFILE, TRUE);
            break;
        case 8:
            (void) doextlist();
            break;
        case 9:
            display_file(LICENSE, TRUE);
            break;
        case 10:
            (void) docontact();
            break;
#ifdef PORT_HELP
        case PORT_HELP_ID:
            port_help();
            break;
#endif
        default:
            /* handle slot 11 or 12 */
            display_file(DEBUGHELP, TRUE);
            break;
        }
    }
    return 0;
}

/* the 'V' command; also a choice for '?' */
int
dohistory()
{
    display_file(HISTORY, TRUE);
    return 0;
}

/*pager.c*/
