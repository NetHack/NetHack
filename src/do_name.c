/* NetHack 3.6	do_name.c	$NHDT-Date: 1446808440 2015/11/06 11:14:00 $  $NHDT-Branch: master $:$NHDT-Revision: 1.77 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

STATIC_DCL char *NDECL(nextmbuf);
STATIC_DCL void FDECL(getpos_help, (BOOLEAN_P, const char *));
STATIC_DCL void NDECL(do_mname);
STATIC_DCL void FDECL(do_oname, (struct obj *));
STATIC_DCL void NDECL(namefloorobj);
STATIC_DCL char *FDECL(bogusmon, (char *,char *));

extern const char what_is_an_unknown_object[]; /* from pager.c */

#define NUMMBUF 5

/* manage a pool of BUFSZ buffers, so callers don't have to */
STATIC_OVL char *
nextmbuf()
{
    static char NEARDATA bufs[NUMMBUF][BUFSZ];
    static int bufidx = 0;

    bufidx = (bufidx + 1) % NUMMBUF;
    return bufs[bufidx];
}

/* function for getpos() to highlight desired map locations.
 * parameter value 0 = initialize, 1 = highlight, 2 = done
 */
void FDECL((*getpos_hilitefunc), (int)) = (void FDECL((*), (int))) 0;

void
getpos_sethilite(f)
void FDECL((*f), (int));
{
    getpos_hilitefunc = f;
}

/* the response for '?' help request in getpos() */
STATIC_OVL void
getpos_help(force, goal)
boolean force;
const char *goal;
{
    char sbuf[BUFSZ];
    boolean doing_what_is;
    winid tmpwin = create_nhwindow(NHW_MENU);

    Sprintf(sbuf, "Use [%c%c%c%c] to move the cursor to %s.", /* hjkl */
            Cmd.move_W, Cmd.move_S, Cmd.move_N, Cmd.move_E, goal);
    putstr(tmpwin, 0, sbuf);
    putstr(tmpwin, 0, "Use [HJKL] to move the cursor 8 units at a time.");
    putstr(tmpwin, 0, "Or enter a background symbol (ex. <).");
    putstr(tmpwin, 0, "Use @ to move the cursor on yourself.");
    if (getpos_hilitefunc != NULL)
        putstr(tmpwin, 0, "Use $ to display valid locations.");
    putstr(tmpwin, 0, "Use # to toggle automatic description.");
    /* disgusting hack; the alternate selection characters work for any
       getpos call, but they only matter for dowhatis (and doquickwhatis) */
    doing_what_is = (goal == what_is_an_unknown_object);
    Sprintf(sbuf, "Type a .%s when you are at the right place.",
            doing_what_is ? " or , or ; or :" : "");
    putstr(tmpwin, 0, sbuf);
    if (!force)
        putstr(tmpwin, 0, "Type Space or Escape when you're done.");
    putstr(tmpwin, 0, "");
    display_nhwindow(tmpwin, TRUE);
    destroy_nhwindow(tmpwin);
}

int
getpos(ccp, force, goal)
coord *ccp;
boolean force;
const char *goal;
{
    int result = 0;
    int cx, cy, i, c;
    int sidx, tx, ty;
    boolean msg_given = TRUE; /* clear message window by default */
    boolean auto_msg = FALSE;
    boolean show_goal_msg = FALSE;
    static const char pick_chars[] = ".,;:";
    const char *cp;
    boolean hilite_state = FALSE;

    if (!goal)
        goal = "desired location";
    if (flags.verbose) {
        pline("(For instructions type a ?)");
        msg_given = TRUE;
    }
    cx = ccp->x;
    cy = ccp->y;
#ifdef CLIPPING
    cliparound(cx, cy);
#endif
    curs(WIN_MAP, cx, cy);
    flush_screen(0);
#ifdef MAC
    lock_mouse_cursor(TRUE);
#endif
    for (;;) {
        if (show_goal_msg) {
            pline("Move cursor to %s:", goal);
            curs(WIN_MAP, cx, cy);
            flush_screen(0);
            show_goal_msg = FALSE;
        } else if (auto_msg && !msg_given && !hilite_state) {
            coord cc;
            int sym = 0;
            char tmpbuf[BUFSZ];
            const char *firstmatch = NULL;

            cc.x = cx;
            cc.y = cy;
            if (do_screen_description(cc, TRUE, sym, tmpbuf, &firstmatch)) {
                pline1(firstmatch);
                curs(WIN_MAP, cx, cy);
                flush_screen(0);
            }
        }

        c = nh_poskey(&tx, &ty, &sidx);

        if (hilite_state) {
            (*getpos_hilitefunc)(2);
            hilite_state = FALSE;
            curs(WIN_MAP, cx, cy);
            flush_screen(0);
        }

        if (auto_msg)
            msg_given = FALSE;

        if (c == '\033') {
            cx = cy = -10;
            msg_given = TRUE; /* force clear */
            result = -1;
            break;
        }
        if (c == 0) {
            if (!isok(tx, ty))
                continue;
            /* a mouse click event, just assign and return */
            cx = tx;
            cy = ty;
            break;
        }
        if ((cp = index(pick_chars, c)) != 0) {
            /* '.' => 0, ',' => 1, ';' => 2, ':' => 3 */
            result = (int) (cp - pick_chars);
            break;
        }
        for (i = 0; i < 8; i++) {
            int dx, dy;

            if (Cmd.dirchars[i] == c) {
                /* a normal movement letter or digit */
                dx = xdir[i];
                dy = ydir[i];
            } else if (Cmd.alphadirchars[i] == lowc((char) c)
                       || (Cmd.num_pad && Cmd.dirchars[i] == (c & 0177))) {
                /* a shifted movement letter or Meta-digit */
                dx = 8 * xdir[i];
                dy = 8 * ydir[i];
            } else
                continue;

            /* truncate at map edge; diagonal moves complicate this... */
            if (cx + dx < 1) {
                dy -= sgn(dy) * (1 - (cx + dx));
                dx = 1 - cx; /* so that (cx+dx == 1) */
            } else if (cx + dx > COLNO - 1) {
                dy += sgn(dy) * ((COLNO - 1) - (cx + dx));
                dx = (COLNO - 1) - cx;
            }
            if (cy + dy < 0) {
                dx -= sgn(dx) * (0 - (cy + dy));
                dy = 0 - cy; /* so that (cy+dy == 0) */
            } else if (cy + dy > ROWNO - 1) {
                dx += sgn(dx) * ((ROWNO - 1) - (cy + dy));
                dy = (ROWNO - 1) - cy;
            }
            cx += dx;
            cy += dy;
            goto nxtc;
        }

        if (c == '?' || redraw_cmd(c)) {
            if (c == '?')
                getpos_help(force, goal);
            else         /* ^R */
                docrt(); /* redraw */
            /* update message window to reflect that we're still targetting */
            show_goal_msg = TRUE;
            msg_given = TRUE;
        } else if ((c == '$') && (getpos_hilitefunc != NULL)) {
            if (!hilite_state) {
                (*getpos_hilitefunc)(0);
                (*getpos_hilitefunc)(1);
                hilite_state = TRUE;
            }
            goto nxtc;
        } else if (c == '#') {
            auto_msg = !auto_msg;
            pline("Automatic description %sis %s.",
                  flags.verbose ? "of features under cursor " : "",
                  auto_msg ? "on" : "off");
            if (!auto_msg)
                show_goal_msg = TRUE;
            msg_given = TRUE;
            goto nxtc;
        } else if (c == '@') {
            cx = u.ux;
            cy = u.uy;
            goto nxtc;
        } else {
            if (!index(quitchars, c)) {
                char matching[MAXPCHARS];
                int pass, lo_x, lo_y, hi_x, hi_y, k = 0;
                (void) memset((genericptr_t) matching, 0, sizeof matching);
                for (sidx = 1; sidx < MAXPCHARS; sidx++)
                    if (c == defsyms[sidx].sym || c == (int) showsyms[sidx])
                        matching[sidx] = (char) ++k;
                if (k) {
                    for (pass = 0; pass <= 1; pass++) {
                        /* pass 0: just past current pos to lower right;
                           pass 1: upper left corner to current pos */
                        lo_y = (pass == 0) ? cy : 0;
                        hi_y = (pass == 0) ? ROWNO - 1 : cy;
                        for (ty = lo_y; ty <= hi_y; ty++) {
                            lo_x = (pass == 0 && ty == lo_y) ? cx + 1 : 1;
                            hi_x = (pass == 1 && ty == hi_y) ? cx : COLNO - 1;
                            for (tx = lo_x; tx <= hi_x; tx++) {
                                /* look at dungeon feature, not at
                                 * user-visible glyph */
                                k = back_to_glyph(tx, ty);
                                /* uninteresting background glyph */
                                if (glyph_is_cmap(k)
                                    && (IS_DOOR(levl[tx][ty].typ)
                                        || glyph_to_cmap(k) == S_room
                                        || glyph_to_cmap(k) == S_darkroom
                                        || glyph_to_cmap(k) == S_corr
                                        || glyph_to_cmap(k) == S_litcorr)) {
                                    /* what hero remembers to be at tx,ty */
                                    k = glyph_at(tx, ty);
                                }
                                if (glyph_is_cmap(k)
                                    && matching[glyph_to_cmap(k)]
                                    && levl[tx][ty].seenv
                                    && (!IS_WALL(levl[tx][ty].typ))
                                    && (levl[tx][ty].typ != SDOOR)
                                    && glyph_to_cmap(k) != S_room
                                    && glyph_to_cmap(k) != S_corr
                                    && glyph_to_cmap(k) != S_litcorr) {
                                    cx = tx, cy = ty;
                                    if (msg_given) {
                                        clear_nhwindow(WIN_MESSAGE);
                                        msg_given = FALSE;
                                    }
                                    goto nxtc;
                                }
                            } /* column */
                        }     /* row */
                    }         /* pass */
                    pline("Can't find dungeon feature '%c'.", c);
                    msg_given = TRUE;
                    goto nxtc;
                } else {
                    char note[QBUFSZ];

                    if (!force)
                        Strcpy(note, "aborted");
                    else
                        Sprintf(note, "use %c%c%c%c or .", /* hjkl */
                                Cmd.move_W, Cmd.move_S, Cmd.move_N,
                                Cmd.move_E);
                    pline("Unknown direction: '%s' (%s).", visctrl((char) c),
                          note);
                    msg_given = TRUE;
                } /* k => matching */
            }     /* !quitchars */
            if (force)
                goto nxtc;
            pline("Done.");
            msg_given = FALSE; /* suppress clear */
            cx = -1;
            cy = 0;
            result = 0; /* not -1 */
            break;
        }
    nxtc:
        ;
#ifdef CLIPPING
        cliparound(cx, cy);
#endif
        curs(WIN_MAP, cx, cy);
        flush_screen(0);
    }
#ifdef MAC
    lock_mouse_cursor(FALSE);
#endif
    if (msg_given)
        clear_nhwindow(WIN_MESSAGE);
    ccp->x = cx;
    ccp->y = cy;
    getpos_hilitefunc = NULL;
    return result;
}

/* allocate space for a monster's name; removes old name if there is one */
void
new_mname(mon, lth)
struct monst *mon;
int lth; /* desired length (caller handles adding 1 for terminator) */
{
    if (lth) {
        /* allocate mextra if necessary; otherwise get rid of old name */
        if (!mon->mextra)
            mon->mextra = newmextra();
        else
            free_mname(mon); /* already has mextra, might also have name */
        MNAME(mon) = (char *) alloc((unsigned) lth);
    } else {
        /* zero length: the new name is empty; get rid of the old name */
        if (has_mname(mon))
            free_mname(mon);
    }
}

/* release a monster's name; retains mextra even if all fields are now null */
void
free_mname(mon)
struct monst *mon;
{
    if (has_mname(mon)) {
        free((genericptr_t) MNAME(mon));
        MNAME(mon) = (char *) 0;
    }
}

/* allocate space for an object's name; removes old name if there is one */
void
new_oname(obj, lth)
struct obj *obj;
int lth; /* desired length (caller handles adding 1 for terminator) */
{
    if (lth) {
        /* allocate oextra if necessary; otherwise get rid of old name */
        if (!obj->oextra)
            obj->oextra = newoextra();
        else
            free_oname(obj); /* already has oextra, might also have name */
        ONAME(obj) = (char *) alloc((unsigned) lth);
    } else {
        /* zero length: the new name is empty; get rid of the old name */
        if (has_oname(obj))
            free_oname(obj);
    }
}

/* release an object's name; retains oextra even if all fields are now null */
void
free_oname(obj)
struct obj *obj;
{
    if (has_oname(obj)) {
        free((genericptr_t) ONAME(obj));
        ONAME(obj) = (char *) 0;
    }
}

/*  safe_oname() always returns a valid pointer to
 *  a string, either the pointer to an object's name
 *  if it has one, or a pointer to an empty string
 *  if it doesn't.
 */
const char *
safe_oname(obj)
struct obj *obj;
{
    if (has_oname(obj))
        return ONAME(obj);
    return "";
}

/* historical note: this returns a monster pointer because it used to
   allocate a new bigger block of memory to hold the monster and its name */
struct monst *
christen_monst(mtmp, name)
struct monst *mtmp;
const char *name;
{
    int lth;
    char buf[PL_PSIZ];

    /* dogname & catname are PL_PSIZ arrays; object names have same limit */
    lth = (name && *name) ? ((int) strlen(name) + 1) : 0;
    if (lth > PL_PSIZ) {
        lth = PL_PSIZ;
        name = strncpy(buf, name, PL_PSIZ - 1);
        buf[PL_PSIZ - 1] = '\0';
    }
    new_mname(mtmp, lth); /* removes old name if one is present */
    if (lth)
        Strcpy(MNAME(mtmp), name);
    return mtmp;
}

/* allow player to assign a name to some chosen monster */
STATIC_OVL void
do_mname()
{
    char buf[BUFSZ], monnambuf[BUFSZ];
    coord cc;
    register int cx, cy;
    register struct monst *mtmp;
    char qbuf[QBUFSZ];

    if (Hallucination) {
        You("would never recognize it anyway.");
        return;
    }
    cc.x = u.ux;
    cc.y = u.uy;
    if (getpos(&cc, FALSE, "the monster you want to name") < 0
        || (cx = cc.x) < 0)
        return;
    cy = cc.y;

    if (cx == u.ux && cy == u.uy) {
        if (u.usteed && canspotmon(u.usteed))
            mtmp = u.usteed;
        else {
            pline("This %s creature is called %s and cannot be renamed.",
                  beautiful(), plname);
            return;
        }
    } else
        mtmp = m_at(cx, cy);

    if (!mtmp
        || (!sensemon(mtmp)
            && (!(cansee(cx, cy) || see_with_infrared(mtmp))
                || mtmp->mundetected || mtmp->m_ap_type == M_AP_FURNITURE
                || mtmp->m_ap_type == M_AP_OBJECT
                || (mtmp->minvis && !See_invisible)))) {
        pline("I see no monster there.");
        return;
    }
    /* special case similar to the one in lookat() */
    Sprintf(qbuf, "What do you want to call %s?",
            distant_monnam(mtmp, ARTICLE_THE, monnambuf));
    getlin(qbuf, buf);
    if (!*buf || *buf == '\033')
        return;
    /* strip leading and trailing spaces; unnames monster if all spaces */
    (void) mungspaces(buf);

    /* unique monsters have their own specific names or titles;
       shopkeepers, temple priests and other minions use alternate
       name formatting routines which ignore any user-supplied name */
    if (mtmp->data->geno & G_UNIQ)
        pline("%s doesn't like being called names!", upstart(monnambuf));
    else if (mtmp->isshk
             && !(Deaf || mtmp->msleeping || !mtmp->mcanmove
                  || mtmp->data->msound <= MS_ANIMAL))
        verbalize("I'm %s, not %s.", shkname(mtmp), buf);
    else if (mtmp->ispriest || mtmp->isminion || mtmp->isshk)
        pline("%s will not accept the name %s.", upstart(monnambuf), buf);
    else
        (void) christen_monst(mtmp, buf);
}

/*
 * This routine changes the address of obj. Be careful not to call it
 * when there might be pointers around in unknown places. For now: only
 * when obj is in the inventory.
 */
STATIC_OVL
void
do_oname(obj)
register struct obj *obj;
{
    char *bufp, buf[BUFSZ], bufcpy[BUFSZ], qbuf[QBUFSZ];
    const char *aname;
    short objtyp;

    /* Do this now because there's no point in even asking for a name */
    if (obj->otyp == SPE_NOVEL) {
        pline("%s already has a published name.", Ysimple_name2(obj));
        return;
    }

    Sprintf(qbuf, "What do you want to name %s ",
            is_plural(obj) ? "these" : "this");
    (void) safe_qbuf(qbuf, qbuf, "?", obj, xname, simpleonames, "item");
    getlin(qbuf, buf);
    if (!*buf || *buf == '\033')
        return;
    /* strip leading and trailing spaces; unnames item if all spaces */
    (void) mungspaces(buf);

    /* relax restrictions over proper capitalization for artifacts */
    if ((aname = artifact_name(buf, &objtyp)) != 0 && objtyp == obj->otyp)
        Strcpy(buf, aname);

    if (obj->oartifact) {
        pline_The("artifact seems to resist the attempt.");
        return;
    } else if (restrict_name(obj, buf) || exist_artifact(obj->otyp, buf)) {
        /* this used to change one letter, substituting a value
           of 'a' through 'y' (due to an off by one error, 'z'
           would never be selected) and then force that to
           upper case if such was the case of the input;
           now, the hand slip scuffs one or two letters as if
           the text had been trodden upon, sometimes picking
           punctuation instead of an arbitrary letter;
           unfortunately, we have to cover the possibility of
           it targetting spaces so failing to make any change
           (we know that it must eventually target a nonspace
           because buf[] matches a valid artifact name) */
        Strcpy(bufcpy, buf);
        /* for "the Foo of Bar", only scuff "Foo of Bar" part */
        bufp = !strncmpi(bufcpy, "the ", 4) ? (buf + 4) : buf;
        do {
            wipeout_text(bufp, rnd(2), (unsigned) 0);
        } while (!strcmp(buf, bufcpy));
        pline("While engraving, your %s slips.", body_part(HAND));
        display_nhwindow(WIN_MESSAGE, FALSE);
        You("engrave: \"%s\".", buf);
    }
    obj = oname(obj, buf);
}

struct obj *
oname(obj, name)
struct obj *obj;
const char *name;
{
    int lth;
    char buf[PL_PSIZ];

    lth = *name ? (int) (strlen(name) + 1) : 0;
    if (lth > PL_PSIZ) {
        lth = PL_PSIZ;
        name = strncpy(buf, name, PL_PSIZ - 1);
        buf[PL_PSIZ - 1] = '\0';
    }
    /* If named artifact exists in the game, do not create another.
     * Also trying to create an artifact shouldn't de-artifact
     * it (e.g. Excalibur from prayer). In this case the object
     * will retain its current name. */
    if (obj->oartifact || (lth && exist_artifact(obj->otyp, name)))
        return obj;

    new_oname(obj, lth); /* removes old name if one is present */
    if (lth)
        Strcpy(ONAME(obj), name);

    if (lth)
        artifact_exists(obj, name, TRUE);
    if (obj->oartifact) {
        /* can't dual-wield with artifact as secondary weapon */
        if (obj == uswapwep)
            untwoweapon();
        /* activate warning if you've just named your weapon "Sting" */
        if (obj == uwep)
            set_artifact_intrinsic(obj, TRUE, W_WEP);
        /* if obj is owned by a shop, increase your bill */
        if (obj->unpaid)
            alter_cost(obj, 0L);
    }
    if (carried(obj))
        update_inventory();
    return obj;
}

static NEARDATA const char callable[] = {
    SCROLL_CLASS, POTION_CLASS, WAND_CLASS,  RING_CLASS, AMULET_CLASS,
    GEM_CLASS,    SPBOOK_CLASS, ARMOR_CLASS, TOOL_CLASS, 0
};

boolean
objtyp_is_callable(i)
int i;
{
    return (boolean) (objects[i].oc_uname
                      || (OBJ_DESCR(objects[i])
                          && index(callable, objects[i].oc_class)));
}

/* C and #name commands - player can name monster or object or type of obj */
int
docallcmd()
{
    struct obj *obj;
    winid win;
    anything any;
    menu_item *pick_list = 0;
    char ch, allowall[2];
    /* if player wants a,b,c instead of i,o when looting, do that here too */
    boolean abc = flags.lootabc;

    win = create_nhwindow(NHW_MENU);
    start_menu(win);
    any = zeroany;
    any.a_char = 'm'; /* group accelerator 'C' */
    add_menu(win, NO_GLYPH, &any, abc ? 0 : any.a_char, 'C', ATR_NONE,
             "a monster", MENU_UNSELECTED);
    if (invent) {
        /* we use y and n as accelerators so that we can accept user's
           response keyed to old "name an individual object?" prompt */
        any.a_char = 'i'; /* group accelerator 'y' */
        add_menu(win, NO_GLYPH, &any, abc ? 0 : any.a_char, 'y', ATR_NONE,
                 "a particular object in inventory", MENU_UNSELECTED);
        any.a_char = 'o'; /* group accelerator 'n' */
        add_menu(win, NO_GLYPH, &any, abc ? 0 : any.a_char, 'n', ATR_NONE,
                 "the type of an object in inventory", MENU_UNSELECTED);
    }
    any.a_char = 'f'; /* group accelerator ',' (or ':' instead?) */
    add_menu(win, NO_GLYPH, &any, abc ? 0 : any.a_char, ',', ATR_NONE,
             "the type of an object upon the floor", MENU_UNSELECTED);
    any.a_char = 'd'; /* group accelerator '\' */
    add_menu(win, NO_GLYPH, &any, abc ? 0 : any.a_char, '\\', ATR_NONE,
             "the type of an object on discoveries list", MENU_UNSELECTED);
    any.a_char = 'a'; /* group accelerator 'l' */
    add_menu(win, NO_GLYPH, &any, abc ? 0 : any.a_char, 'l', ATR_NONE,
             "record an annotation for the current level", MENU_UNSELECTED);
    end_menu(win, "What do you want to name?");
    if (select_menu(win, PICK_ONE, &pick_list) > 0) {
        ch = pick_list[0].item.a_char;
        free((genericptr_t) pick_list);
    } else
        ch = 'q';
    destroy_nhwindow(win);

    switch (ch) {
    default:
    case 'q':
        break;
    case 'm': /* name a visible monster */
        do_mname();
        break;
    case 'i': /* name an individual object in inventory */
        allowall[0] = ALL_CLASSES;
        allowall[1] = '\0';
        obj = getobj(allowall, "name");
        if (obj)
            do_oname(obj);
        break;
    case 'o': /* name a type of object in inventory */
        obj = getobj(callable, "call");
        if (obj) {
            /* behave as if examining it in inventory;
               this might set dknown if it was picked up
               while blind and the hero can now see */
            (void) xname(obj);

            if (!obj->dknown) {
                You("would never recognize another one.");
#if 0
            } else if (!objtyp_is_callable(obj->otyp)) {
                You("know those as well as you ever will.");
#endif
            } else {
                docall(obj);
            }
        }
        break;
    case 'f': /* name a type of object visible on the floor */
        namefloorobj();
        break;
    case 'd': /* name a type of object on the discoveries list */
        rename_disco();
        break;
    case 'a': /* annotate level */
        donamelevel();
        break;
    }
    return 0;
}

void
docall(obj)
register struct obj *obj;
{
    char buf[BUFSZ], qbuf[QBUFSZ];
    struct obj otemp;
    register char **str1;

    if (!obj->dknown)
        return; /* probably blind */
    otemp = *obj;
    otemp.quan = 1L;
    otemp.oextra = (struct oextra *) 0;

    if (objects[otemp.otyp].oc_class == POTION_CLASS && otemp.fromsink)
        /* kludge, meaning it's sink water */
        Sprintf(qbuf, "Call a stream of %s fluid:",
                OBJ_DESCR(objects[otemp.otyp]));
    else
        Sprintf(qbuf, "Call %s:", an(xname(&otemp)));
    getlin(qbuf, buf);
    if (!*buf || *buf == '\033')
        return;

    /* clear old name */
    str1 = &(objects[obj->otyp].oc_uname);
    if (*str1)
        free((genericptr_t) *str1);

    /* strip leading and trailing spaces; uncalls item if all spaces */
    (void) mungspaces(buf);
    if (!*buf) {
        if (*str1) { /* had name, so possibly remove from disco[] */
            /* strip name first, for the update_inventory() call
               from undiscover_object() */
            *str1 = (char *) 0;
            undiscover_object(obj->otyp);
        }
    } else {
        *str1 = dupstr(buf);
        discover_object(obj->otyp, FALSE, TRUE); /* possibly add to disco[] */
    }
}

STATIC_OVL void
namefloorobj()
{
    coord cc;
    int glyph;
    char buf[BUFSZ];
    struct obj *obj = 0;
    boolean fakeobj = FALSE, use_plural;

    cc.x = u.ux, cc.y = u.uy;
    /* "dot for under/over you" only makes sense when the cursor hasn't
       been moved off the hero's '@' yet, but there's no way to adjust
       the help text once getpos() has started */
    Sprintf(buf, "object on map (or '.' for one %s you)",
            (u.uundetected && hides_under(youmonst.data)) ? "over" : "under");
    if (getpos(&cc, FALSE, buf) < 0 || cc.x <= 0)
        return;
    if (cc.x == u.ux && cc.y == u.uy) {
        obj = vobj_at(u.ux, u.uy);
    } else {
        glyph = glyph_at(cc.x, cc.y);
        if (glyph_is_object(glyph))
            fakeobj = object_from_map(glyph, cc.x, cc.y, &obj);
        /* else 'obj' stays null */
    }
    if (!obj) {
        /* "under you" is safe here since there's no object to hide under */
        pline("There doesn't seem to be any object %s.",
              (cc.x == u.ux && cc.y == u.uy) ? "under you" : "there");
        return;
    }
    /* note well: 'obj' might be as instance of STRANGE_OBJECT if target
       is a mimic; passing that to xname (directly or via simpleonames)
       would yield "glorkum" so we need to handle it explicitly; it will
       always fail the Hallucination test and pass the !callable test,
       resulting in the "can't be assigned a type name" message */
    Strcpy(buf, (obj->otyp != STRANGE_OBJECT)
                 ? simpleonames(obj)
                 : obj_descr[STRANGE_OBJECT].oc_name);
    use_plural = (obj->quan > 1L);
    if (Hallucination) {
        const char *unames[6];
        char tmpbuf[BUFSZ];

        /* straight role name */
        unames[0] = ((Upolyd ? u.mfemale : flags.female) && urole.name.f)
                     ? urole.name.f
                     : urole.name.m;
        /* random rank title for hero's role */
        unames[1] = rank_of(rnd(30), Role_switch, flags.female);
        /* random fake monster */
        unames[2] = bogusmon(tmpbuf, (char *) 0);
        /* increased chance for fake monster */
        unames[3] = unames[2];
        /* traditional */
        unames[4] = roguename();
        /* silly */
        unames[5] = "Wibbly Wobbly";
        pline("%s %s to call you \"%s.\"",
              The(buf), use_plural ? "decide" : "decides",
              unames[rn2(SIZE(unames))]);
    } else if (!objtyp_is_callable(obj->otyp)) {
        pline("%s %s can't be assigned a type name.",
              use_plural ? "Those" : "That", buf);
    } else if (!obj->dknown) {
        You("don't know %s %s well enough to name %s.",
            use_plural ? "those" : "that", buf, use_plural ? "them" : "it");
    } else {
        docall(obj);
    }
    if (fakeobj)
        dealloc_obj(obj);
}

static const char *const ghostnames[] = {
    /* these names should have length < PL_NSIZ */
    /* Capitalize the names for aesthetics -dgk */
    "Adri",    "Andries",       "Andreas",     "Bert",    "David",  "Dirk",
    "Emile",   "Frans",         "Fred",        "Greg",    "Hether", "Jay",
    "John",    "Jon",           "Karnov",      "Kay",     "Kenny",  "Kevin",
    "Maud",    "Michiel",       "Mike",        "Peter",   "Robert", "Ron",
    "Tom",     "Wilmar",        "Nick Danger", "Phoenix", "Jiro",   "Mizue",
    "Stephan", "Lance Braccus", "Shadowhawk"
};

/* ghost names formerly set by x_monnam(), now by makemon() instead */
const char *
rndghostname()
{
    return rn2(7) ? ghostnames[rn2(SIZE(ghostnames))] : (const char *) plname;
}

/*
 * Monster naming functions:
 * x_monnam is the generic monster-naming function.
 *                seen        unseen       detected               named
 * mon_nam:     the newt        it      the invisible orc       Fido
 * noit_mon_nam:the newt (as if detected) the invisible orc     Fido
 * l_monnam:    newt            it      invisible orc           dog called Fido
 * Monnam:      The newt        It      The invisible orc       Fido
 * noit_Monnam: The newt (as if detected) The invisible orc     Fido
 * Adjmonnam:   The poor newt   It      The poor invisible orc  The poor Fido
 * Amonnam:     A newt          It      An invisible orc        Fido
 * a_monnam:    a newt          it      an invisible orc        Fido
 * m_monnam:    newt            xan     orc                     Fido
 * y_monnam:    your newt     your xan  your invisible orc      Fido
 */

/* Bug: if the monster is a priest or shopkeeper, not every one of these
 * options works, since those are special cases.
 */
char *
x_monnam(mtmp, article, adjective, suppress, called)
register struct monst *mtmp;
int article;
/* ARTICLE_NONE, ARTICLE_THE, ARTICLE_A: obvious
 * ARTICLE_YOUR: "your" on pets, "the" on everything else
 *
 * If the monster would be referred to as "it" or if the monster has a name
 * _and_ there is no adjective, "invisible", "saddled", etc., override this
 * and always use no article.
 */
const char *adjective;
int suppress;
/* SUPPRESS_IT, SUPPRESS_INVISIBLE, SUPPRESS_HALLUCINATION, SUPPRESS_SADDLE.
 * EXACT_NAME: combination of all the above
 */
boolean called;
{
    char *buf = nextmbuf();
    struct permonst *mdat = mtmp->data;
    const char *pm_name = mdat->mname;
    boolean do_hallu, do_invis, do_it, do_saddle;
    boolean name_at_start, has_adjectives;
    char *bp;

    if (program_state.gameover)
        suppress |= SUPPRESS_HALLUCINATION;
    if (article == ARTICLE_YOUR && !mtmp->mtame)
        article = ARTICLE_THE;

    do_hallu = Hallucination && !(suppress & SUPPRESS_HALLUCINATION);
    do_invis = mtmp->minvis && !(suppress & SUPPRESS_INVISIBLE);
    do_it = !canspotmon(mtmp) && article != ARTICLE_YOUR
            && !program_state.gameover && mtmp != u.usteed
            && !(u.uswallow && mtmp == u.ustuck) && !(suppress & SUPPRESS_IT);
    do_saddle = !(suppress & SUPPRESS_SADDLE);

    buf[0] = '\0';

    /* unseen monsters, etc.  Use "it" */
    if (do_it) {
        Strcpy(buf, "it");
        return buf;
    }

    /* priests and minions: don't even use this function */
    if (mtmp->ispriest || mtmp->isminion) {
        char priestnambuf[BUFSZ];
        char *name;
        long save_prop = EHalluc_resistance;
        unsigned save_invis = mtmp->minvis;

        /* when true name is wanted, explicitly block Hallucination */
        if (!do_hallu)
            EHalluc_resistance = 1L;
        if (!do_invis)
            mtmp->minvis = 0;
        name = priestname(mtmp, priestnambuf);
        EHalluc_resistance = save_prop;
        mtmp->minvis = save_invis;
        if (article == ARTICLE_NONE && !strncmp(name, "the ", 4))
            name += 4;
        return strcpy(buf, name);
    }
    /* an "aligned priest" not flagged as a priest or minion should be
       "priest" or "priestess" (normally handled by priestname()) */
    if (mdat == &mons[PM_ALIGNED_PRIEST])
        pm_name = mtmp->female ? "priestess" : "priest";
    else if (mdat == &mons[PM_HIGH_PRIEST] && mtmp->female)
        pm_name = "high priestess";

    /* Shopkeepers: use shopkeeper name.  For normal shopkeepers, just
     * "Asidonhopo"; for unusual ones, "Asidonhopo the invisible
     * shopkeeper" or "Asidonhopo the blue dragon".  If hallucinating,
     * none of this applies.
     */
    if (mtmp->isshk && !do_hallu) {
        if (adjective && article == ARTICLE_THE) {
            /* pathological case: "the angry Asidonhopo the blue dragon"
               sounds silly */
            Strcpy(buf, "the ");
            Strcat(strcat(buf, adjective), " ");
            Strcat(buf, shkname(mtmp));
            return buf;
        }
        Strcat(buf, shkname(mtmp));
        if (mdat == &mons[PM_SHOPKEEPER] && !do_invis)
            return buf;
        Strcat(buf, " the ");
        if (do_invis)
            Strcat(buf, "invisible ");
        Strcat(buf, pm_name);
        return buf;
    }

    /* Put the adjectives in the buffer */
    if (adjective)
        Strcat(strcat(buf, adjective), " ");
    if (do_invis)
        Strcat(buf, "invisible ");
    if (do_saddle && (mtmp->misc_worn_check & W_SADDLE) && !Blind
        && !Hallucination)
        Strcat(buf, "saddled ");
    if (buf[0] != 0)
        has_adjectives = TRUE;
    else
        has_adjectives = FALSE;

    /* Put the actual monster name or type into the buffer now */
    /* Be sure to remember whether the buffer starts with a name */
    if (do_hallu) {
        char rnamecode;
        char *rname = rndmonnam(&rnamecode);

        Strcat(buf, rname);
        name_at_start = bogon_is_pname(rnamecode);
    } else if (has_mname(mtmp)) {
        char *name = MNAME(mtmp);

        if (mdat == &mons[PM_GHOST]) {
            Sprintf(eos(buf), "%s ghost", s_suffix(name));
            name_at_start = TRUE;
        } else if (called) {
            Sprintf(eos(buf), "%s called %s", pm_name, name);
            name_at_start = (boolean) type_is_pname(mdat);
        } else if (is_mplayer(mdat) && (bp = strstri(name, " the ")) != 0) {
            /* <name> the <adjective> <invisible> <saddled> <rank> */
            char pbuf[BUFSZ];

            Strcpy(pbuf, name);
            pbuf[bp - name + 5] = '\0'; /* adjectives right after " the " */
            if (has_adjectives)
                Strcat(pbuf, buf);
            Strcat(pbuf, bp + 5); /* append the rest of the name */
            Strcpy(buf, pbuf);
            article = ARTICLE_NONE;
            name_at_start = TRUE;
        } else {
            Strcat(buf, name);
            name_at_start = TRUE;
        }
    } else if (is_mplayer(mdat) && !In_endgame(&u.uz)) {
        char pbuf[BUFSZ];

        Strcpy(pbuf, rank_of((int) mtmp->m_lev, monsndx(mdat),
                             (boolean) mtmp->female));
        Strcat(buf, lcase(pbuf));
        name_at_start = FALSE;
    } else {
        Strcat(buf, pm_name);
        name_at_start = (boolean) type_is_pname(mdat);
    }

    if (name_at_start && (article == ARTICLE_YOUR || !has_adjectives)) {
        if (mdat == &mons[PM_WIZARD_OF_YENDOR])
            article = ARTICLE_THE;
        else
            article = ARTICLE_NONE;
    } else if ((mdat->geno & G_UNIQ) && article == ARTICLE_A) {
        article = ARTICLE_THE;
    }

    {
        char buf2[BUFSZ];

        switch (article) {
        case ARTICLE_YOUR:
            Strcpy(buf2, "your ");
            Strcat(buf2, buf);
            Strcpy(buf, buf2);
            return buf;
        case ARTICLE_THE:
            Strcpy(buf2, "the ");
            Strcat(buf2, buf);
            Strcpy(buf, buf2);
            return buf;
        case ARTICLE_A:
            return an(buf);
        case ARTICLE_NONE:
        default:
            return buf;
        }
    }
}

char *
l_monnam(mtmp)
struct monst *mtmp;
{
    return x_monnam(mtmp, ARTICLE_NONE, (char *) 0,
                    (has_mname(mtmp)) ? SUPPRESS_SADDLE : 0, TRUE);
}

char *
mon_nam(mtmp)
struct monst *mtmp;
{
    return x_monnam(mtmp, ARTICLE_THE, (char *) 0,
                    (has_mname(mtmp)) ? SUPPRESS_SADDLE : 0, FALSE);
}

/* print the name as if mon_nam() was called, but assume that the player
 * can always see the monster--used for probing and for monsters aggravating
 * the player with a cursed potion of invisibility
 */
char *
noit_mon_nam(mtmp)
struct monst *mtmp;
{
    return x_monnam(mtmp, ARTICLE_THE, (char *) 0,
                    (has_mname(mtmp)) ? (SUPPRESS_SADDLE | SUPPRESS_IT)
                                       : SUPPRESS_IT,
                    FALSE);
}

char *
Monnam(mtmp)
struct monst *mtmp;
{
    register char *bp = mon_nam(mtmp);

    *bp = highc(*bp);
    return  bp;
}

char *
noit_Monnam(mtmp)
struct monst *mtmp;
{
    register char *bp = noit_mon_nam(mtmp);

    *bp = highc(*bp);
    return  bp;
}

/* monster's own name */
char *
m_monnam(mtmp)
struct monst *mtmp;
{
    return x_monnam(mtmp, ARTICLE_NONE, (char *) 0, EXACT_NAME, FALSE);
}

/* pet name: "your little dog" */
char *
y_monnam(mtmp)
struct monst *mtmp;
{
    int prefix, suppression_flag;

    prefix = mtmp->mtame ? ARTICLE_YOUR : ARTICLE_THE;
    suppression_flag = (has_mname(mtmp)
                        /* "saddled" is redundant when mounted */
                        || mtmp == u.usteed)
                           ? SUPPRESS_SADDLE
                           : 0;

    return x_monnam(mtmp, prefix, (char *) 0, suppression_flag, FALSE);
}

char *
Adjmonnam(mtmp, adj)
struct monst *mtmp;
const char *adj;
{
    char *bp = x_monnam(mtmp, ARTICLE_THE, adj,
                        has_mname(mtmp) ? SUPPRESS_SADDLE : 0, FALSE);

    *bp = highc(*bp);
    return  bp;
}

char *
a_monnam(mtmp)
struct monst *mtmp;
{
    return x_monnam(mtmp, ARTICLE_A, (char *) 0,
                    has_mname(mtmp) ? SUPPRESS_SADDLE : 0, FALSE);
}

char *
Amonnam(mtmp)
struct monst *mtmp;
{
    char *bp = a_monnam(mtmp);

    *bp = highc(*bp);
    return  bp;
}

/* used for monster ID by the '/', ';', and 'C' commands to block remote
   identification of the endgame altars via their attending priests */
char *
distant_monnam(mon, article, outbuf)
struct monst *mon;
int article; /* only ARTICLE_NONE and ARTICLE_THE are handled here */
char *outbuf;
{
    /* high priest(ess)'s identity is concealed on the Astral Plane,
       unless you're adjacent (overridden for hallucination which does
       its own obfuscation) */
    if (mon->data == &mons[PM_HIGH_PRIEST] && !Hallucination
        && Is_astralevel(&u.uz) && distu(mon->mx, mon->my) > 2) {
        Strcpy(outbuf, article == ARTICLE_THE ? "the " : "");
        Strcat(outbuf, mon->female ? "high priestess" : "high priest");
    } else {
        Strcpy(outbuf, x_monnam(mon, article, (char *) 0, 0, TRUE));
    }
    return outbuf;
}

/* fake monsters used to be in a hard-coded array, now in a data file */
STATIC_OVL char *
bogusmon(buf, code)
char *buf, *code;
{
    char *mname = buf;

    get_rnd_text(BOGUSMONFILE, buf);
    /* strip prefix if present */
    if (!letter(*mname)) {
        if (code)
            *code = *mname;
        ++mname;
    } else {
        if (code)
            *code = '\0';
    }
    return mname;
}

/* return a random monster name, for hallucination */
char *
rndmonnam(code)
char *code;
{
    static char buf[BUFSZ];
    char *mname;
    int name;
#define BOGUSMONSIZE 100 /* arbitrary */

    if (code)
        *code = '\0';

    do {
        name = rn1(SPECIAL_PM + BOGUSMONSIZE - LOW_PM, LOW_PM);
    } while (name < SPECIAL_PM
             && (type_is_pname(&mons[name]) || (mons[name].geno & G_NOGEN)));

    if (name >= SPECIAL_PM) {
        mname = bogusmon(buf, code);
    } else {
        mname = strcpy(buf, mons[name].mname);
    }
    return mname;
#undef BOGUSMONSIZE
}

/* check bogusmon prefix to decide whether it's a personal name */
boolean
bogon_is_pname(code)
char code;
{
    if (!code)
        return FALSE;
    return index("-+=", code) ? TRUE : FALSE;
}

/* name of a Rogue player */
const char *
roguename()
{
    char *i, *opts;

    if ((opts = nh_getenv("ROGUEOPTS")) != 0) {
        for (i = opts; *i; i++)
            if (!strncmp("name=", i, 5)) {
                char *j;
                if ((j = index(i + 5, ',')) != 0)
                    *j = (char) 0;
                return i + 5;
            }
    }
    return rn2(3) ? (rn2(2) ? "Michael Toy" : "Kenneth Arnold")
                  : "Glenn Wichman";
}

static NEARDATA const char *const hcolors[] = {
    "ultraviolet", "infrared", "bluish-orange", "reddish-green", "dark white",
    "light black", "sky blue-pink", "salty", "sweet", "sour", "bitter",
    "striped", "spiral", "swirly", "plaid", "checkered", "argyle", "paisley",
    "blotchy", "guernsey-spotted", "polka-dotted", "square", "round",
    "triangular", "cabernet", "sangria", "fuchsia", "wisteria", "lemon-lime",
    "strawberry-banana", "peppermint", "romantic", "incandescent",
    "octarine", /* Discworld: the Colour of Magic */
};

const char *
hcolor(colorpref)
const char *colorpref;
{
    return (Hallucination || !colorpref) ? hcolors[rn2(SIZE(hcolors))]
                                         : colorpref;
}

/* return a random real color unless hallucinating */
const char *
rndcolor()
{
    int k = rn2(CLR_MAX);

    return Hallucination ? hcolor((char *) 0)
                         : (k == NO_COLOR) ? "colorless"
                                           : c_obj_colors[k];
}

/* Aliases for road-runner nemesis
 */
static const char *const coynames[] = {
    "Carnivorous Vulgaris", "Road-Runnerus Digestus", "Eatibus Anythingus",
    "Famishus-Famishus", "Eatibus Almost Anythingus", "Eatius Birdius",
    "Famishius Fantasticus", "Eternalii Famishiis", "Famishus Vulgarus",
    "Famishius Vulgaris Ingeniusi", "Eatius-Slobbius", "Hardheadipus Oedipus",
    "Carnivorous Slobbius", "Hard-Headipus Ravenus", "Evereadii Eatibus",
    "Apetitius Giganticus", "Hungrii Flea-Bagius", "Overconfidentii Vulgaris",
    "Caninus Nervous Rex", "Grotesques Appetitus", "Nemesis Ridiculii",
    "Canis latrans"
};

char *
coyotename(mtmp, buf)
struct monst *mtmp;
char *buf;
{
    if (mtmp && buf) {
        Sprintf(buf, "%s - %s",
                x_monnam(mtmp, ARTICLE_NONE, (char *) 0, 0, TRUE),
                mtmp->mcan ? coynames[SIZE(coynames) - 1]
                           : coynames[rn2(SIZE(coynames) - 1)]);
    }
    return buf;
}

/* make sure "The Colour of Magic" remains the first entry in here */
static const char *const sir_Terry_novels[] = {
    "The Colour of Magic", "The Light Fantastic", "Equal Rites", "Mort",
    "Sourcery", "Wyrd Sisters", "Pyramids", "Guards! Guards!", "Eric",
    "Moving Pictures", "Reaper Man", "Witches Abroad", "Small Gods",
    "Lords and Ladies", "Men at Arms", "Soul Music", "Interesting Times",
    "Maskerade", "Feet of Clay", "Hogfather", "Jingo", "The Last Continent",
    "Carpe Jugulum", "The Fifth Elephant", "The Truth", "Thief of Time",
    "The Last Hero", "The Amazing Maurice and his Educated Rodents",
    "Night Watch", "The Wee Free Men", "Monstrous Regiment",
    "A Hat Full of Sky", "Going Postal", "Thud!", "Wintersmith",
    "Making Money", "Unseen Academicals", "I Shall Wear Midnight", "Snuff",
    "Raising Steam", "The Shepherd's Crown"
};

const char *
noveltitle(novidx)
int *novidx;
{
    int j, k = SIZE(sir_Terry_novels);

    j = rn2(k);
    if (novidx) {
        if (*novidx == -1)
            *novidx = j;
        else if (*novidx >= 0 && *novidx < k)
            j = *novidx;
    }
    return sir_Terry_novels[j];
}

const char *
lookup_novel(lookname, idx)
const char *lookname;
int *idx;
{
    int k;

    /* Take American or U.K. spelling of this one */
    if (!strcmpi(The(lookname), "The Color of Magic"))
        lookname = sir_Terry_novels[0];

    for (k = 0; k < SIZE(sir_Terry_novels); ++k) {
        if (!strcmpi(lookname, sir_Terry_novels[k])
            || !strcmpi(The(lookname), sir_Terry_novels[k])) {
            if (idx)
                *idx = k;
            return sir_Terry_novels[k];
        }
    }
    /* name not found; if novelidx is already set, override the name */
    if (idx && *idx >= 0 && *idx < SIZE(sir_Terry_novels))
        return sir_Terry_novels[*idx];

    return (const char *) 0;
}

/*do_name.c*/
