/* NetHack 3.7	getpos.c	$NHDT-Date: 1708126536 2024/02/16 23:35:36 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.307 $ */
/*-Copyright (c) Pasi Kallinen, 2023. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

extern const char what_is_an_unknown_object[]; /* from pager.c */

staticfn void getpos_toggle_hilite_state(void);
staticfn void getpos_getvalids_selection(struct selectionvar *,
                                       boolean (*)(coordxy, coordxy));
staticfn void getpos_help_keyxhelp(winid, const char *, const char *, int);
staticfn void getpos_help(boolean, const char *);
staticfn int QSORTCALLBACK cmp_coord_distu(const void *, const void *);
staticfn int gloc_filter_classify_glyph(int);
staticfn int gloc_filter_floodfill_matcharea(coordxy, coordxy);
staticfn void gloc_filter_floodfill(coordxy, coordxy);
staticfn void gloc_filter_init(void);
staticfn void gloc_filter_done(void);
staticfn void gather_locs(coord **, int *, int);
staticfn void truncate_to_map(coordxy *, coordxy *, schar, schar);
staticfn void getpos_refresh(void);
/* Callback function for getpos() to highlight desired map locations.
 * Parameter TRUE: initialize and highlight, FALSE: done (remove highlights).
 */
staticfn void (*getpos_hilitefunc)(boolean) = (void (*)(boolean)) 0;
staticfn boolean (*getpos_getvalid)(coordxy, coordxy)
                                          = (boolean (*)(coordxy, coordxy)) 0;
enum getposHiliteState {
    HiliteNormalMap = 0,
    HiliteGoodposSymbol = 1,
    HiliteBackground = 2,
};

static enum getposHiliteState
    getpos_hilite_state = HiliteNormalMap,
    defaultHiliteState = HiliteNormalMap;

void
getpos_sethilite(
    void (*gp_hilitef)(boolean),
    boolean (*gp_getvalidf)(coordxy, coordxy))
{
    boolean (*old_getvalid)(coordxy, coordxy) = getpos_getvalid;
    uint32 old_map_frame_color = gw.wsettings.map_frame_color;
    struct selectionvar *sel = selection_new();

    defaultHiliteState = iflags.bgcolors ? HiliteBackground : HiliteNormalMap;
    if (gp_getvalidf != old_getvalid)
        getpos_hilite_state = defaultHiliteState;

    getpos_getvalids_selection(sel, getpos_getvalid);
    getpos_hilitefunc = gp_hilitef;
    getpos_getvalid = gp_getvalidf;
    getpos_getvalids_selection(sel, getpos_getvalid);
    gw.wsettings.map_frame_color = (getpos_hilite_state == HiliteBackground)
                                   ? HI_ZAP : NO_COLOR;

    if (getpos_getvalid != old_getvalid
        || gw.wsettings.map_frame_color != old_map_frame_color)
        selection_force_newsyms(sel);
    selection_free(sel, TRUE);
}

/* cycle 'getpos_hilite_state' to its next state;
   when 'bgcolors' is Off, it will alternate between not showing valid
   positions and showing them via temporary S_goodpos symbol;
   when 'bgcolors' is On, there are three states and showing them via
   setting background color becomes the default */
staticfn void
getpos_toggle_hilite_state(void)
{
    /* getpos_hilitefunc isn't Null */
    if (getpos_hilite_state == HiliteGoodposSymbol) {
        /* currently on, finish */
        (*getpos_hilitefunc)(FALSE); /* tmp_at(DISP_END) */
    }

    getpos_hilite_state = (getpos_hilite_state + 1)
                          % (iflags.bgcolors ? 3 : 2);
    /* resetting the callback functions to their current values will draw
       valid-spots with background color if that is the new state and turn
       off that color if it was the previous state */
    getpos_sethilite(getpos_hilitefunc, getpos_getvalid);

    if (getpos_hilite_state == HiliteGoodposSymbol) {
        /* now on, begin */
        (*getpos_hilitefunc)(TRUE);
    }
}

boolean
mapxy_valid(coordxy x, coordxy y)
{
    if (getpos_getvalid)
        return (*getpos_getvalid)(x, y);
    return FALSE;
}

staticfn void
getpos_getvalids_selection(
    struct selectionvar *sel,
    boolean (*validf)(coordxy, coordxy))
{
    coordxy x, y;

    if (!sel || !validf)
        return;

    for (x = 1; x < sel->wid; x++)
        for (y = 0; y < sel->hei; y++)
            if ((*validf)(x, y))
                selection_setpoint(x, y, sel, 1);
}

static const char *const gloc_descr[NUM_GLOCS][4] = {
    { "any monsters", "monster", "next/previous monster", "monsters" },
    { "any items", "item", "next/previous object", "objects" },
    { "any doors", "door", "next/previous door or doorway",
      "doors or doorways" },
    { "any unexplored areas", "unexplored area", "unexplored location",
      "locations next to unexplored locations" },
    { "anything interesting", "interesting thing", "anything interesting",
      "anything interesting" },
    { "any valid locations", "valid location", "valid location",
      "valid locations" }
};

static const char *const gloc_filtertxt[NUM_GFILTER] = {
    "",
    " in view",
    " in this area"
};

staticfn void
getpos_help_keyxhelp(
    winid tmpwin,
    const char *k1, const char *k2,
    int gloc)
{
    char sbuf[BUFSZ], fbuf[QBUFSZ];
    const char *move_cursor_to = "move the cursor to ",
               *filtertxt = gloc_filtertxt[iflags.getloc_filter];

    if (gloc == GLOC_EXPLORE) {
        /* default of "move to unexplored location" is inaccurate
           because the position will be one spot short of that */
        move_cursor_to = "move the cursor next to an ";
        if (iflags.getloc_usemenu)
            /* default is too wide for basic 80-column tty so shorten it
               to avoid wrapping */
            filtertxt = strsubst(strcpy(fbuf, filtertxt),
                                 "this area", "area");
    }
    Sprintf(sbuf, "Use '%s'/'%s' to %s%s%s.",
            k1, k2,
            iflags.getloc_usemenu ? "get a menu of " : move_cursor_to,
            gloc_descr[gloc][2 + iflags.getloc_usemenu], filtertxt);
    putstr(tmpwin, 0, sbuf);
}

DISABLE_WARNING_FORMAT_NONLITERAL

/* the response for '?' help request in getpos() */
staticfn void
getpos_help(boolean force, const char *goal)
{
    static const char *const fastmovemode[2] = { "8 units at a time",
                                                 "skipping same glyphs" };
    char sbuf[BUFSZ];
    boolean doing_what_is;
    winid tmpwin = create_nhwindow(NHW_MENU);

    Sprintf(sbuf,
            "Use '%s', '%s', '%s', '%s' to move the cursor to %s.", /* hjkl */
            visctrl(cmd_from_func(do_move_west)),
            visctrl(cmd_from_func(do_move_south)),
            visctrl(cmd_from_func(do_move_north)),
            visctrl(cmd_from_func(do_move_east)), goal);
    putstr(tmpwin, 0, sbuf);
    Sprintf(sbuf,
            "Use '%s', '%s', '%s', '%s' to fast-move the cursor, %s.",
            visctrl(cmd_from_func(do_run_west)),
            visctrl(cmd_from_func(do_run_south)),
            visctrl(cmd_from_func(do_run_north)),
            visctrl(cmd_from_func(do_run_east)),
            fastmovemode[iflags.getloc_moveskip]);
    putstr(tmpwin, 0, sbuf);
    Sprintf(sbuf, "(or prefix normal move with '%s' or '%s' to fast-move)",
            visctrl(cmd_from_func(do_run)),
            visctrl(cmd_from_func(do_rush)));
    putstr(tmpwin, 0, sbuf);
    putstr(tmpwin, 0, "Or enter a background symbol (ex. '<').");
    Sprintf(sbuf, "Use '%s' to move the cursor on yourself.",
           visctrl(gc.Cmd.spkeys[NHKF_GETPOS_SELF]));
    putstr(tmpwin, 0, sbuf);
    if (!iflags.terrainmode || (iflags.terrainmode & TER_MON) != 0) {
        getpos_help_keyxhelp(tmpwin,
                             visctrl(gc.Cmd.spkeys[NHKF_GETPOS_MON_NEXT]),
                             visctrl(gc.Cmd.spkeys[NHKF_GETPOS_MON_PREV]),
                             GLOC_MONS);
    }
    if (goal && !strcmp(goal, "a monster"))
        goto skip_non_mons;
    if (!iflags.terrainmode || (iflags.terrainmode & TER_OBJ) != 0) {
        getpos_help_keyxhelp(tmpwin,
                             visctrl(gc.Cmd.spkeys[NHKF_GETPOS_OBJ_NEXT]),
                             visctrl(gc.Cmd.spkeys[NHKF_GETPOS_OBJ_PREV]),
                             GLOC_OBJS);
    }
    if (!iflags.terrainmode || (iflags.terrainmode & TER_MAP) != 0) {
        /* these are primarily useful when choosing a travel
           destination for the '_' command */
        getpos_help_keyxhelp(tmpwin,
                             visctrl(gc.Cmd.spkeys[NHKF_GETPOS_DOOR_NEXT]),
                             visctrl(gc.Cmd.spkeys[NHKF_GETPOS_DOOR_PREV]),
                             GLOC_DOOR);
        getpos_help_keyxhelp(tmpwin,
                             visctrl(gc.Cmd.spkeys[NHKF_GETPOS_UNEX_NEXT]),
                             visctrl(gc.Cmd.spkeys[NHKF_GETPOS_UNEX_PREV]),
                             GLOC_EXPLORE);
        getpos_help_keyxhelp(tmpwin,
                          visctrl(gc.Cmd.spkeys[NHKF_GETPOS_INTERESTING_NEXT]),
                          visctrl(gc.Cmd.spkeys[NHKF_GETPOS_INTERESTING_PREV]),
                             GLOC_INTERESTING);
    }
    Sprintf(sbuf, "Use '%s' to change fast-move mode to %s.",
            visctrl(gc.Cmd.spkeys[NHKF_GETPOS_MOVESKIP]),
            fastmovemode[!iflags.getloc_moveskip]);
    putstr(tmpwin, 0, sbuf);
    if (!iflags.terrainmode || (iflags.terrainmode & TER_DETECT) == 0) {
        Sprintf(sbuf, "Use '%s' to toggle menu listing for possible targets.",
                visctrl(gc.Cmd.spkeys[NHKF_GETPOS_MENU]));
        putstr(tmpwin, 0, sbuf);
        Sprintf(sbuf,
                "Use '%s' to change the mode of limiting possible targets.",
                visctrl(gc.Cmd.spkeys[NHKF_GETPOS_LIMITVIEW]));
        putstr(tmpwin, 0, sbuf);
    }
    if (!iflags.terrainmode) {
        char kbuf[BUFSZ];

        if (getpos_getvalid) {
            Sprintf(sbuf, "Use '%s' or '%s' to move to valid locations.",
                    visctrl(gc.Cmd.spkeys[NHKF_GETPOS_VALID_NEXT]),
                    visctrl(gc.Cmd.spkeys[NHKF_GETPOS_VALID_PREV]));
            putstr(tmpwin, 0, sbuf);
        }
        if (getpos_hilitefunc) {
            Sprintf(sbuf, "Use '%s' to toggle marking of valid locations.",
                    visctrl(gc.Cmd.spkeys[NHKF_GETPOS_SHOWVALID]));
            putstr(tmpwin, 0, sbuf);
        }
        Sprintf(sbuf, "Use '%s' to toggle automatic description.",
                visctrl(gc.Cmd.spkeys[NHKF_GETPOS_AUTODESC]));
        putstr(tmpwin, 0, sbuf);
        if (iflags.cmdassist) { /* assisting the '/' command, I suppose... */
            Sprintf(sbuf,
                    (iflags.getpos_coords == GPCOORDS_NONE)
        ? "(Set 'whatis_coord' option to include coordinates with '%s' text.)"
        : "(Reset 'whatis_coord' option to omit coordinates from '%s' text.)",
                    visctrl(gc.Cmd.spkeys[NHKF_GETPOS_AUTODESC]));
        }
 skip_non_mons:
        /* disgusting hack; the alternate selection characters work for any
           getpos call, but only matter for dowhatis (and doquickwhatis,
           also for dotherecmdmenu's simulated mouse) */
        doing_what_is = (goal == what_is_an_unknown_object);
        if (doing_what_is) {
            Sprintf(kbuf, "'%s' or '%s' or '%s' or '%s'",
                    visctrl(gc.Cmd.spkeys[NHKF_GETPOS_PICK]),
                    visctrl(gc.Cmd.spkeys[NHKF_GETPOS_PICK_Q]),
                    visctrl(gc.Cmd.spkeys[NHKF_GETPOS_PICK_O]),
                    visctrl(gc.Cmd.spkeys[NHKF_GETPOS_PICK_V]));
        } else {
            Sprintf(kbuf, "'%s'", visctrl(gc.Cmd.spkeys[NHKF_GETPOS_PICK]));
        }
        Snprintf(sbuf, sizeof(sbuf),
                 "Type a %s when you are at the right place.", kbuf);
        putstr(tmpwin, 0, sbuf);
        if (doing_what_is) {
            Sprintf(sbuf,
      "  '%s' describe current spot, show 'more info', move to another spot.",
                    visctrl(gc.Cmd.spkeys[NHKF_GETPOS_PICK_V]));
            putstr(tmpwin, 0, sbuf);
            Sprintf(sbuf,
                    "  '%s' describe current spot,%s move to another spot;",
                    visctrl(gc.Cmd.spkeys[NHKF_GETPOS_PICK]),
                    flags.help && !force ? " prompt if 'more info'," : "");
            putstr(tmpwin, 0, sbuf);
            Sprintf(sbuf,
                    "  '%s' describe current spot, move to another spot;",
                    visctrl(gc.Cmd.spkeys[NHKF_GETPOS_PICK_Q]));
            putstr(tmpwin, 0, sbuf);
            Sprintf(sbuf,
                    "  '%s' describe current spot, stop looking at things;",
                    visctrl(gc.Cmd.spkeys[NHKF_GETPOS_PICK_O]));
            putstr(tmpwin, 0, sbuf);
        }
    }
    if (!force)
        putstr(tmpwin, 0, "Type Space or Escape when you're done.");
    putstr(tmpwin, 0, "");
    display_nhwindow(tmpwin, TRUE);
    destroy_nhwindow(tmpwin);
}

RESTORE_WARNING_FORMAT_NONLITERAL

staticfn int QSORTCALLBACK
cmp_coord_distu(const void *a, const void *b)
{
    const coord *c1 = a;
    const coord *c2 = b;
    int dx, dy, dist_1, dist_2;

    dx = u.ux - c1->x;
    dy = u.uy - c1->y;
    dist_1 = max(abs(dx), abs(dy));
    dx = u.ux - c2->x;
    dy = u.uy - c2->y;
    dist_2 = max(abs(dx), abs(dy));

    if (dist_1 == dist_2)
        return (c1->y != c2->y) ? (c1->y - c2->y) : (c1->x - c2->x);

    return dist_1 - dist_2;
}

#define IS_UNEXPLORED_LOC(x,y) \
    (isok((x), (y))                                             \
     && glyph_is_unexplored(levl[(x)][(y)].glyph)               \
     && !levl[(x)][(y)].seenv)

#define GLOC_SAME_AREA(x,y) \
    (isok((x), (y))                                             \
     && (selection_getpoint((x),(y), gg.gloc_filter_map)))

staticfn int
gloc_filter_classify_glyph(int glyph)
{
    int c;

    if (!glyph_is_cmap(glyph))
        return 0;

    c = glyph_to_cmap(glyph);

    if (is_cmap_room(c) || is_cmap_furniture(c))
        return 1;
    else if (is_cmap_wall(c) || c == S_tree)
        return 2;
    else if (is_cmap_corr(c))
        return 3;
    else if (is_cmap_water(c))
        return 4;
    else if (is_cmap_lava(c))
        return 5;
    return 0;
}

staticfn int
gloc_filter_floodfill_matcharea(coordxy x, coordxy y)
{
    int glyph = back_to_glyph(x, y);

    if (!levl[x][y].seenv)
        return FALSE;

    if (glyph == gg.gloc_filter_floodfill_match_glyph)
        return TRUE;

    if (gloc_filter_classify_glyph(glyph)
        == gloc_filter_classify_glyph(gg.gloc_filter_floodfill_match_glyph))
        return TRUE;

    return FALSE;
}

staticfn void
gloc_filter_floodfill(coordxy x, coordxy y)
{
    gg.gloc_filter_floodfill_match_glyph = back_to_glyph(x, y);

    set_selection_floodfillchk(gloc_filter_floodfill_matcharea);
    selection_floodfill(gg.gloc_filter_map, x, y, FALSE);
}

staticfn void
gloc_filter_init(void)
{
    if (iflags.getloc_filter == GFILTER_AREA) {
        if (!gg.gloc_filter_map) {
            gg.gloc_filter_map = selection_new();
        }
        /* special case: if we're in a doorway, try to figure out which
           direction we're moving, and use that side of the doorway */
        if (IS_DOOR(levl[u.ux][u.uy].typ)) {
            if ((u.dx || u.dy) && isok(u.ux + u.dx, u.uy + u.dy)) {
                gloc_filter_floodfill(u.ux + u.dx, u.uy + u.dy);
            } else {
                /* TODO: maybe add both sides of the doorway? */
            }
        } else {
            gloc_filter_floodfill(u.ux, u.uy);
        }
    }
}

staticfn void
gloc_filter_done(void)
{
    if (gg.gloc_filter_map) {
        selection_free(gg.gloc_filter_map, TRUE);
        gg.gloc_filter_map = (struct selectionvar *) 0;

    }
}

DISABLE_WARNING_UNREACHABLE_CODE

boolean
gather_locs_interesting(coordxy x, coordxy y, int gloc)
{
    int glyph, sym;

    if (iflags.getloc_filter == GFILTER_VIEW && !cansee(x, y))
        return FALSE;
    if (iflags.getloc_filter == GFILTER_AREA && !GLOC_SAME_AREA(x, y)
        && !GLOC_SAME_AREA(x - 1, y) && !GLOC_SAME_AREA(x, y - 1)
        && !GLOC_SAME_AREA(x + 1, y) && !GLOC_SAME_AREA(x, y + 1))
        return FALSE;

    glyph = glyph_at(x, y);
    sym = glyph_is_cmap(glyph) ? glyph_to_cmap(glyph) : -1;
    switch (gloc) {
    default:
    case GLOC_MONS:
        /* unlike '/M', this skips monsters revealed by
           warning glyphs and remembered unseen ones */
        return (glyph_is_monster(glyph)
                && glyph != monnum_to_glyph(PM_LONG_WORM_TAIL,MALE)
                && glyph != monnum_to_glyph(PM_LONG_WORM_TAIL, FEMALE));
    case GLOC_OBJS:
        return (glyph_is_object(glyph)
                && glyph != objnum_to_glyph(BOULDER)
                && glyph != objnum_to_glyph(ROCK));
    case GLOC_DOOR:
        return (glyph_is_cmap(glyph)
                && (is_cmap_door(sym)
                    || is_cmap_drawbridge(sym)
                    || sym == S_ndoor));
    case GLOC_EXPLORE:
        return (glyph_is_cmap(glyph)
                && !glyph_is_nothing(glyph_to_cmap(glyph))
                && (is_cmap_door(sym)
                    || is_cmap_drawbridge(sym)
                    || sym == S_ndoor
                    || is_cmap_room(sym)
                    || is_cmap_corr(sym))
                && (IS_UNEXPLORED_LOC(x + 1, y)
                    || IS_UNEXPLORED_LOC(x - 1, y)
                    || IS_UNEXPLORED_LOC(x, y + 1)
                    || IS_UNEXPLORED_LOC(x, y - 1)));
    case GLOC_VALID:
        if (getpos_getvalid)
            return (*getpos_getvalid)(x, y);
        /*FALLTHRU*/
    case GLOC_INTERESTING:
        return (gather_locs_interesting(x, y, GLOC_DOOR)
                || !((glyph_is_cmap(glyph)
                      && (is_cmap_wall(sym)
                          || sym == S_tree
                          || sym == S_bars
                          || sym == S_ice
                          || sym == S_air
                          || sym == S_cloud
                          || is_cmap_lava(sym)
                          || is_cmap_water(sym)
                          || sym == S_ndoor
                          || is_cmap_room(sym)
                          || is_cmap_corr(sym)))
                     || glyph_is_nothing(glyph)
                     || glyph_is_unexplored(glyph)));
    }
    /*NOTREACHED*/
    return FALSE;
}

RESTORE_WARNINGS

/* gather locations for monsters or objects shown on the map */
staticfn void
gather_locs(coord **arr_p, int *cnt_p, int gloc)
{
    int pass, idx;
    coordxy x, y;

    /*
     * We always include the hero's location even if there is no monster
     * (invisible hero without see invisible) or object (usual case)
     * displayed there.  That way, the count will always be at least 1,
     * and player has a visual indicator (cursor returns to hero's spot)
     * highlighting when successive 'm's or 'o's have cycled all the way
     * through all monsters or objects.
     *
     * Hero's spot will always sort to array[0] because it will always
     * be the shortest distance (namely, 0 units) away from <u.ux,u.uy>.
     */

    gloc_filter_init();

    *cnt_p = idx = 0;
    for (pass = 0; pass < 2; pass++) {
        for (x = 1; x < COLNO; x++)
            for (y = 0; y < ROWNO; y++) {
                if (u_at(x, y) || gather_locs_interesting(x, y, gloc)) {
                    if (!pass) {
                        ++*cnt_p;
                    } else {
                        (*arr_p)[idx].x = x;
                        (*arr_p)[idx].y = y;
                        ++idx;
                    }
                }
            }

        if (!pass) /* end of first pass */
            *arr_p = (coord *) alloc(*cnt_p * sizeof (coord));
        else /* end of second pass */
            qsort(*arr_p, *cnt_p, sizeof (coord), cmp_coord_distu);
    } /* pass */

    gloc_filter_done();
}

char *
dxdy_to_dist_descr(coordxy dx, coordxy dy, boolean fulldir)
{
    static char buf[30];
    int dst;

    if (!dx && !dy) {
        Sprintf(buf, "here");
    } else if ((dst = xytod(dx, dy)) != -1) {
        /* explicit direction; 'one step' is implicit */
        Sprintf(buf, "%s", directionname(dst));
    } else {
        static const char *const dirnames[4][2] = {
            { "n", "north" },
            { "s", "south" },
            { "w", "west" },
            { "e", "east" } };
        buf[0] = '\0';
        /* 9999: protect buf[] against overflow caused by invalid values */
        if (dy) {
            if (abs(dy) > 9999)
                dy = sgn(dy) * 9999;
            Sprintf(eos(buf), "%d%s%s", abs(dy), dirnames[(dy > 0)][fulldir],
                    dx ? "," : "");
        }
        if (dx) {
            if (abs(dx) > 9999)
                dx = sgn(dx) * 9999;
            Sprintf(eos(buf), "%d%s", abs(dx),
                    dirnames[2 + (dx > 0)][fulldir]);
        }
    }
    return buf;
}

DISABLE_WARNING_FORMAT_NONLITERAL

/* coordinate formatting for 'whatis_coord' option */
char *
coord_desc(coordxy x, coordxy y, char *outbuf, char cmode)
{
    static char screen_fmt[16]; /* [12] suffices: "[%02d,%02d]" */
    int dx, dy;

    outbuf[0] = '\0';
    switch (cmode) {
    default:
        break;
    case GPCOORDS_COMFULL:
    case GPCOORDS_COMPASS:
        /* "east", "3s", "2n,4w" */
        dx = x - u.ux;
        dy = y - u.uy;
        Sprintf(outbuf, "(%s)",
                dxdy_to_dist_descr(dx, dy, cmode == GPCOORDS_COMFULL));
        break;
    case GPCOORDS_MAP: /* x,y */
        /* upper left corner of map is <1,0>;
           with default COLNO,ROWNO lower right corner is <79,20> */
        Sprintf(outbuf, "<%d,%d>", x, y);
        break;
    case GPCOORDS_SCREEN: /* y+2,x */
        /* for normal map sizes, force a fixed-width formatting so that
           /m, /M, /o, and /O output lines up cleanly; map sizes bigger
           than Nx999 or 999xM will still work, but not line up like normal
           when displayed in a column setting.

           The (100) is placed in brackets below to mark the [: "03"] as
           explicit compile-time dead code for clang */
        if (!*screen_fmt)
            Sprintf(screen_fmt, "[%%%sd,%%%sd]",
                    (ROWNO - 1 + 2 < (100)) ? "02" :  "03",
                    (COLNO - 1 < (100)) ? "02" : "03");
        /* map line 0 is screen row 2;
           map column 0 isn't used, map column 1 is screen column 1 */
        Sprintf(outbuf, screen_fmt, y + 2, x);
        break;
    }
    return outbuf;
}

RESTORE_WARNING_FORMAT_NONLITERAL

void
auto_describe(coordxy cx, coordxy cy)
{
    coord cc;
    int sym = 0;
    char tmpbuf[BUFSZ];
    const char *firstmatch = "unknown";

    cc.x = cx;
    cc.y = cy;
    if (do_screen_description(cc, TRUE, sym, tmpbuf, &firstmatch,
                              (struct permonst **) 0)) {
        (void) coord_desc(cx, cy, tmpbuf, iflags.getpos_coords);
        custompline((SUPPRESS_HISTORY | OVERRIDE_MSGTYPE | NO_CURS_ON_U),
                    "%s%s%s%s%s", firstmatch, *tmpbuf ? " " : "", tmpbuf,
                    (iflags.autodescribe
                     && getpos_getvalid && !(*getpos_getvalid)(cx, cy))
                      ? " (invalid target)" : "",
                    (iflags.getloc_travelmode && !is_valid_travelpt(cx, cy))
                      ? " (no travel path)" : "");
        curs(WIN_MAP, cx, cy);
        flush_screen(0);
    }
}

boolean
getpos_menu(coord *ccp, int gloc)
{
    coord *garr = DUMMY;
    int gcount = 0;
    winid tmpwin;
    anything any;
    int i, pick_cnt;
    menu_item *picks = (menu_item *) 0;
    char tmpbuf[BUFSZ];
    int clr = NO_COLOR;

    gather_locs(&garr, &gcount, gloc);

    if (gcount < 2) { /* gcount always includes the hero */
        free((genericptr_t) garr);
        You("cannot %s %s.",
            (iflags.getloc_filter == GFILTER_VIEW) ? "see" : "detect",
            gloc_descr[gloc][0]);
        return FALSE;
    }

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);
    any = cg.zeroany;

    /* gather_locs returns array[0] == you. skip it. */
    for (i = 1; i < gcount; i++) {
        char fullbuf[BUFSZ];
        coord tmpcc;
        const char *firstmatch = "unknown";
        int sym = 0;

        any.a_int = i + 1;
        tmpcc.x = garr[i].x;
        tmpcc.y = garr[i].y;
        if (do_screen_description(tmpcc, TRUE, sym, tmpbuf,
                              &firstmatch, (struct permonst **)0)) {
            (void) coord_desc(garr[i].x, garr[i].y, tmpbuf,
                              iflags.getpos_coords);
            Snprintf(fullbuf, sizeof fullbuf, "%s%s%s", firstmatch,
                    (*tmpbuf ? " " : ""), tmpbuf);
            add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0,
                     ATR_NONE, clr, fullbuf, MENU_ITEMFLAGS_NONE);
        }
    }

    Sprintf(tmpbuf, "Pick %s%s%s",
            an(gloc_descr[gloc][1]),
            gloc_filtertxt[iflags.getloc_filter],
            iflags.getloc_travelmode ? " for travel destination" : "");
    end_menu(tmpwin, tmpbuf);
    pick_cnt = select_menu(tmpwin, PICK_ONE, &picks);
    destroy_nhwindow(tmpwin);
    if (pick_cnt > 0) {
        ccp->x = garr[picks->item.a_int - 1].x;
        ccp->y = garr[picks->item.a_int - 1].y;
        free((genericptr_t) picks);
    }
    free((genericptr_t) garr);
    return (pick_cnt > 0);
}

/* add dx,dy to cx,cy, truncating at map edges */
staticfn void
truncate_to_map(coordxy *cx, coordxy *cy, schar dx, schar dy)
{
    /* diagonal moves complicate this... */
    if (*cx + dx < 1) {
        dy -= sgn(dy) * (1 - (*cx + dx));
        dx = 1 - *cx; /* so that (cx+dx == 1) */
    } else if (*cx + dx > COLNO - 1) {
        dy += sgn(dy) * ((COLNO - 1) - (*cx + dx));
        dx = (COLNO - 1) - *cx;
    }
    if (*cy + dy < 0) {
        dx -= sgn(dx) * (0 - (*cy + dy));
        dy = 0 - *cy; /* so that (cy+dy == 0) */
    } else if (*cy + dy > ROWNO - 1) {
        dx += sgn(dx) * ((ROWNO - 1) - (*cy + dy));
        dy = (ROWNO - 1) - *cy;
    }
    *cx += dx;
    *cy += dy;
}

/* called when ^R typed; if '$' is being shown for valid spots, remove that;
   if alternate background color is being shown for that, redraw it */
staticfn void
getpos_refresh(void)
{
    if (getpos_hilitefunc && getpos_hilite_state == HiliteGoodposSymbol) {
        (*getpos_hilitefunc)(FALSE); /* tmp_at(DISP_END) */
        getpos_hilite_state = defaultHiliteState;
    }

    docrt_flags(docrtRefresh);

    if (getpos_hilitefunc && getpos_hilite_state == HiliteBackground) {
        /* resetting to current values will draw valid-spots highlighting */
        getpos_sethilite(getpos_hilitefunc, getpos_getvalid);
    }
}

/* have the player use movement keystrokes to position the cursor at a
   particular map location, then use one of [.,:;] to pick the spot */
int
getpos(coord *ccp, boolean force, const char *goal)
{
    static struct {
        int nhkf, ret;
    } const pick_chars_def[] = {
        { NHKF_GETPOS_PICK, LOOK_TRADITIONAL },
        { NHKF_GETPOS_PICK_Q, LOOK_QUICK },
        { NHKF_GETPOS_PICK_O, LOOK_ONCE },
        { NHKF_GETPOS_PICK_V, LOOK_VERBOSE }
    };
    static const int mMoOdDxX_def[] = {
        NHKF_GETPOS_MON_NEXT,
        NHKF_GETPOS_MON_PREV,
        NHKF_GETPOS_OBJ_NEXT,
        NHKF_GETPOS_OBJ_PREV,
        NHKF_GETPOS_DOOR_NEXT,
        NHKF_GETPOS_DOOR_PREV,
        NHKF_GETPOS_UNEX_NEXT,
        NHKF_GETPOS_UNEX_PREV,
        NHKF_GETPOS_INTERESTING_NEXT,
        NHKF_GETPOS_INTERESTING_PREV,
        NHKF_GETPOS_VALID_NEXT,
        NHKF_GETPOS_VALID_PREV
    };
    struct _cmd_queue cq, *cmdq;
    const char *cp;
    char pick_chars[6];
    char mMoOdDxX[13];
    int result = 0;
    int i, c;
    int sidx;
    coordxy cx, cy;
    coordxy tx = u.ux, ty = u.uy;
    boolean msg_given = TRUE; /* clear message window by default */
    boolean show_goal_msg = FALSE;
    coord *garr[NUM_GLOCS] = DUMMY;
    int gcount[NUM_GLOCS] = DUMMY;
    int gidx[NUM_GLOCS] = DUMMY;
    schar udx = u.dx, udy = u.dy, udz = u.dz;
    int dx, dy;
    boolean rushrun = FALSE;

    /* temporary? if we have a queued direction, return the adjacent spot
       in that direction */
    if (!gi.in_doagain) {
        if ((cmdq = cmdq_pop()) != 0) {
            cq = *cmdq;
            free((genericptr_t) cmdq);
            if (cq.typ == CMDQ_DIR && !cq.dirz) {
                ccp->x = u.ux + cq.dirx;
                ccp->y = u.uy + cq.diry;
            } else {
                cmdq_clear(CQ_CANNED);
                result = -1;
            }
            return result;
        }
    }

    for (i = 0; i < SIZE(pick_chars_def); i++)
        pick_chars[i] = gc.Cmd.spkeys[pick_chars_def[i].nhkf];
    pick_chars[SIZE(pick_chars_def)] = '\0';

    for (i = 0; i < SIZE(mMoOdDxX_def); i++)
        mMoOdDxX[i] = gc.Cmd.spkeys[mMoOdDxX_def[i]];
    mMoOdDxX[SIZE(mMoOdDxX_def)] = '\0';

    handle_tip(TIP_GETPOS);

    if (!goal)
        goal = "desired location";
    if (flags.verbose) {
        pline("(For instructions type a '%s')",
              visctrl(gc.Cmd.spkeys[NHKF_GETPOS_HELP]));
        msg_given = TRUE;
    }
    cx = gg.getposx = ccp->x;
    cy = gg.getposy = ccp->y;
#ifdef CLIPPING
    cliparound(cx, cy);
#endif
    curs(WIN_MAP, cx, cy);
    flush_screen(0);
#ifdef MAC
    lock_mouse_cursor(TRUE);
#endif
    lock_mouse_buttons(TRUE);
    for (;;) {
        if (show_goal_msg) {
            pline("Move cursor to %s:", goal);
            curs(WIN_MAP, cx, cy);
            flush_screen(0);
            show_goal_msg = FALSE;
        } else if (iflags.autodescribe && !msg_given) {
            auto_describe(cx, cy);
        }

        rushrun = FALSE;

        if ((cmdq = cmdq_pop()) != 0) {
            if (cmdq->typ == CMDQ_KEY) {
                c = cmdq->key;
            } else {
                cmdq_clear(CQ_CANNED);
                result = -1;
                goto exitgetpos;
            }
            free(cmdq);
        } else {
            c = readchar_poskey(&tx, &ty, &sidx);
            /* remember_getpos is normally False because reusing the
               cursor positioning during ^A is almost never the right
               thing to do, but caller could set it if that was needed */
            if (iflags.remember_getpos && !gi.in_doagain)
                cmdq_add_key(CQ_REPEAT, c);
        }

        if (iflags.autodescribe)
            msg_given = FALSE;

        if (c == gc.Cmd.spkeys[NHKF_ESC]) {
            cx = cy = -10;
            msg_given = TRUE; /* force clear */
            result = -1;
            break;
        }
        if (c == cmd_from_func(do_run) || c == cmd_from_func(do_rush)) {
            c = readchar_poskey(&tx, &ty, &sidx);
            rushrun = TRUE;
        }
        if (c == 0) {
            if (!isok(tx, ty))
                continue;
            /* a mouse click event, just assign and return */
            cx = tx;
            cy = ty;
            break;
        }
        if ((cp = strchr(pick_chars, c)) != 0) {
            /* '.' => 0, ',' => 1, ';' => 2, ':' => 3 */
            result = pick_chars_def[(int) (cp - pick_chars)].ret;
            break;
        } else if (movecmd(c, MV_WALK)) {
            if (rushrun)
                goto do_rushrun;
            dx = u.dx;
            dy = u.dy;
            truncate_to_map(&cx, &cy, dx, dy);
            goto nxtc;
        } else if (movecmd(c, MV_RUSH) || movecmd(c, MV_RUN)) {
 do_rushrun:
            if (iflags.getloc_moveskip) {
                /* skip same glyphs */
                int glyph = glyph_at(cx, cy);

                dx = u.dx;
                dy = u.dy;
                while (isok(cx + dx, cy + dy)
                       && glyph == glyph_at(cx + dx, cy + dy)
                       && isok(cx + dx + u.dx, cy + dy + u.dy)
                       && glyph == glyph_at(cx + dx + u.dx, cy + dy + u.dy)) {
                    dx += u.dx;
                    dy += u.dy;

                }
            } else {
                dx = 8 * u.dx;
                dy = 8 * u.dy;
            }
            truncate_to_map(&cx, &cy, dx, dy);
            goto nxtc;
        }

        if (c == gc.Cmd.spkeys[NHKF_GETPOS_HELP] || redraw_cmd(c)) {
            /* '?' will redraw twice, first when removing popup text window
               after showing the help text, then to reset highlighting */
            if (c == gc.Cmd.spkeys[NHKF_GETPOS_HELP])
                getpos_help(force, goal);
            /* ^R: docrt(), hilite_state = default */
            getpos_refresh();
            curs(WIN_MAP, cx, cy);
            /* update message window to reflect that we're still targeting */
            show_goal_msg = TRUE;
        } else if (c == gc.Cmd.spkeys[NHKF_GETPOS_SHOWVALID]) {
            if (getpos_hilitefunc) {
                getpos_toggle_hilite_state();
                curs(WIN_MAP, cx, cy);
            }
            show_goal_msg = TRUE; /* we're still targeting */
            goto nxtc;
        } else if (c == gc.Cmd.spkeys[NHKF_GETPOS_AUTODESC]) {
            iflags.autodescribe = !iflags.autodescribe;
            pline("Automatic description %sis %s.",
                  flags.verbose ? "of features under cursor " : "",
                  iflags.autodescribe ? "on" : "off");
            if (!iflags.autodescribe)
                show_goal_msg = TRUE;
            msg_given = TRUE;
            goto nxtc;
        } else if (c == gc.Cmd.spkeys[NHKF_GETPOS_LIMITVIEW]) {
            static const char *const view_filters[NUM_GFILTER] = {
                "Not limiting targets",
                "Limiting targets to those in sight",
                "Limiting targets to those in same area"
            };

            iflags.getloc_filter = (iflags.getloc_filter + 1) % NUM_GFILTER;
            for (i = 0; i < NUM_GLOCS; i++) {
                if (garr[i]) {
                    free((genericptr_t) garr[i]);
                    garr[i] = NULL;
                }
                gidx[i] = gcount[i] = 0;
            }
            pline("%s.", view_filters[iflags.getloc_filter]);
            msg_given = TRUE;
            goto nxtc;
        } else if (c == gc.Cmd.spkeys[NHKF_GETPOS_MENU]) {
            iflags.getloc_usemenu = !iflags.getloc_usemenu;
            pline("%s a menu to show possible targets%s.",
                  iflags.getloc_usemenu ? "Using" : "Not using",
                  iflags.getloc_usemenu
                      ? " for 'm|M', 'o|O', 'd|D', and 'x|X'" : "");
            msg_given = TRUE;
            goto nxtc;
        } else if (c == gc.Cmd.spkeys[NHKF_GETPOS_SELF]) {
            /* reset 'm&M', 'o&O', &c; otherwise, there's no way for player
               to achieve that except by manually cycling through all spots */
            for (i = 0; i < NUM_GLOCS; i++)
                gidx[i] = 0;
            cx = u.ux;
            cy = u.uy;
            goto nxtc;
        } else if (c == gc.Cmd.spkeys[NHKF_GETPOS_MOVESKIP]) {
            iflags.getloc_moveskip = !iflags.getloc_moveskip;
            pline("%skipping over similar terrain when fastmoving the cursor.",
                  iflags.getloc_moveskip ? "S" : "Not s");
            msg_given = TRUE;
            goto nxtc;
        } else if ((cp = strchr(mMoOdDxX, c)) != 0) { /* 'm|M', 'o|O', &c */
            /* nearest or farthest monster or object or door or unexplored */
            int gtmp = (int) (cp - mMoOdDxX), /* 0..7 */
                gloc = gtmp >> 1;             /* 0..3 */

            if (iflags.getloc_usemenu) {
                coord tmpcrd;

                if (getpos_menu(&tmpcrd, gloc)) {
                    cx = tmpcrd.x;
                    cy = tmpcrd.y;
                }
                goto nxtc;
            }

            if (!garr[gloc]) {
                gather_locs(&garr[gloc], &gcount[gloc], gloc);
                gidx[gloc] = 0; /* garr[][0] is hero's spot */
            }
            if (!(gtmp & 1)) {  /* c=='m' || c=='o' || c=='d' || c=='x') */
                gidx[gloc] = (gidx[gloc] + 1) % gcount[gloc];
            } else {            /* c=='M' || c=='O' || c=='D' || c=='X') */
                if (--gidx[gloc] < 0)
                    gidx[gloc] = gcount[gloc] - 1;
            }
            cx = garr[gloc][gidx[gloc]].x;
            cy = garr[gloc][gidx[gloc]].y;
            goto nxtc;
        } else {
            if (!strchr(quitchars, c)) {
                char matching[MAXPCHARS];
                int pass, k = 0;
                coordxy lo_x, lo_y, hi_x, hi_y;

                (void) memset((genericptr_t) matching, 0, sizeof matching);
                for (sidx = 0; sidx < MAXPCHARS; sidx++) {
                    /* don't even try to match some terrain: walls, room... */
                    if (is_cmap_wall(sidx) || is_cmap_room(sidx)
                        || is_cmap_corr(sidx) || is_cmap_door(sidx)
                        || sidx == S_ndoor)
                        continue;
                    if (c == defsyms[sidx].sym
                        || c == (int) gs.showsyms[sidx]
                        /* have '^' match webs and vibrating square or any
                           other trap that uses something other than '^' */
                        || (c == '^' && is_cmap_trap(sidx)))
                        matching[sidx] = (char) ++k;
                }
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
                                /* first, look at what is currently visible
                                   (might be monster) */
                                k = glyph_at(tx, ty);
                                if (glyph_is_cmap(k)
                                    && matching[glyph_to_cmap(k)])
                                    goto foundc;
                                /* next, try glyph that's remembered here
                                   (might be trap or object) */
                                if (gl.level.flags.hero_memory
                                    /* !terrainmode: don't move to remembered
                                       trap or object if not currently shown */
                                    && !iflags.terrainmode) {
                                    k = levl[tx][ty].glyph;
                                    if (glyph_is_cmap(k)
                                        && matching[glyph_to_cmap(k)])
                                        goto foundc;
                                }
                                /* last, try actual terrain here (shouldn't
                                   we be using gl.lastseentyp[][] instead?) */
                                if (levl[tx][ty].seenv) {
                                    k = back_to_glyph(tx, ty);
                                    if (glyph_is_cmap(k)
                                        && matching[glyph_to_cmap(k)])
                                        goto foundc;
                                }
                                continue;
 foundc:
                                cx = tx, cy = ty;
                                if (msg_given) {
                                    clear_nhwindow(WIN_MESSAGE);
                                    msg_given = FALSE;
                                }
                                goto nxtc;
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
                    else /* hjkl */
                        Sprintf(note, "use '%s', '%s', '%s', '%s' or '%s'",
                                visctrl(cmd_from_func(do_move_west)),
                                visctrl(cmd_from_func(do_move_south)),
                                visctrl(cmd_from_func(do_move_north)),
                                visctrl(cmd_from_func(do_move_east)),
                                visctrl(gc.Cmd.spkeys[NHKF_GETPOS_PICK]));
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
        gg.getposx = cx, gg.getposy = cy;
#ifdef CLIPPING
        cliparound(cx, cy);
#endif
        curs(WIN_MAP, cx, cy);
        flush_screen(0);
    }
 exitgetpos:
#ifdef MAC
    lock_mouse_cursor(FALSE);
#endif
    lock_mouse_buttons(FALSE);
    if (msg_given)
        clear_nhwindow(WIN_MESSAGE);
    ccp->x = cx;
    ccp->y = cy;
    gg.getposx = gg.getposy = 0;
    for (i = 0; i < NUM_GLOCS; i++)
        if (garr[i])
            free((genericptr_t) garr[i]);
    getpos_sethilite(NULL, NULL);
    u.dx = udx, u.dy = udy, u.dz = udz;
    return result;
}

/*getpos.c*/
