/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* NetHack 3.7 cursnearby.c */
/* Copyright (c) Kestrel Gregorich-Trevor, 2021. Based on cursinvt.c, by Fredrik Ljungdahl. */
/* NetHack may be freely redistributed.  See license for details. */

#include "curses.h"
#include "hack.h"
#include "wincurs.h"
#include "cursnearby.h"

static void curs_nearby_updated(WINDOW *);
static void curs_show_nearby(WINDOW *);

struct pn_data {
    boolean monsters;       /* Display nearby monsters or items */
};

static struct pn_data pn = { FALSE };

/* Runs when the game indicates that the nearby characters window has been updated */
void
curs_update_nearby(int arg)
{
    WINDOW *win = curses_get_nhwin(NEARBY_WIN);

    /* Check if the nearby window is enabled in first place */
    if (!win) {
        /* It's not. Re-initialize the main windows if the
           option was enabled. */
        if (iflags.perm_nearby) {
            /* [core_]status_initialize, curses_create_main_windows,
               curses_last_messages, [core_]doredraw; doredraw will
               call (*update_inventory) [curses_update_inventory] which
               will call us but 'win' should be defined that time */
            curs_reset_windows(TRUE, FALSE);
            /* TODO: guard against window creation failure [if that's
               possible] which would lead to uncontrolled recursion */
        }
        return;
    }
    if (arg) {
        pn.monsters = !pn.monsters;
    }

    /* clear anything displayed from previous update */
    werase(win);
    curs_nearby_updated(win);
    return;
}

/* persistent nearby characters has been updated or scrolled/panned; re-display it */
static void
curs_nearby_updated(WINDOW *win)
{
    /* display collected nearby characters, probably clipped */
    curs_show_nearby(win);

    if (curses_window_has_border(NEARBY_WIN))
        box(win, 0, 0);

    wnoutrefresh(win);
}

/* display the inventory menu-like data collected in pi.array[] */
static void
curs_show_nearby(WINDOW *win)
{
    int attr, color, x, y, width, height, available_width,
        cx, cy, lo_x, lo_y, hi_x, hi_y, glyph, 
        border = curses_window_has_border(INV_WIN) ? 1 : 0;
    int count = 0;
    char lookbuf[BUFSZ], outbuf[BUFSZ], glyphbuf[40];
    boolean do_mons = pn.monsters;
    boolean peaceful = FALSE;
    glyph_info glyphinfo;

    x = border, y = border; /* same for every line; 1 if border, 0 otherwise */
    curses_get_window_size(NEARBY_WIN, &height, &width);
    wmove(win, y, x);
    lo_y = TRUE ? max(u.uy - BOLT_LIM, 0) : 0;
    lo_x = TRUE ? max(u.ux - BOLT_LIM, 1) : 1;
    hi_y = TRUE ? min(u.uy + BOLT_LIM, ROWNO - 1) : ROWNO - 1;
    hi_x = TRUE ? min(u.ux + BOLT_LIM, COLNO - 1) : COLNO - 1;
    for (cy = lo_y; cy <= hi_y; cy++) {
        for (cx = lo_x; cx <= hi_x; cx++) {
            lookbuf[0] = '\0';
            glyph = glyph_at(cx, cy);
            map_glyphinfo(0, 0, glyph, 0, &glyphinfo);
            color = glyphinfo.color;
            peaceful = FALSE;
            attr = 0;
            if (do_mons) {
                if (glyph_is_monster(glyph)) {
                    struct monst *mtmp;

                    if ((mtmp = m_at(cx, cy)) != 0) {
                        look_at_monster(lookbuf, (char *) 0, mtmp, cx, cy);
                        ++count;
                        if (mtmp->mtame) attr = iflags.wc2_petattr;
                        if (mtmp->mpeaceful) peaceful = TRUE;
                    }
                } else if (glyph_is_invisible(glyph)) {
                    Strcpy(lookbuf, "remembered, unseen, creature");
                    ++count;
                } else if (glyph_is_warning(glyph)) {
                    int warnindx = glyph_to_warning(glyph);

                    Strcpy(lookbuf, def_warnsyms[warnindx].explanation);
                    ++count;
                }
            } else { /* !do_mons */
                if (glyph_is_object(glyph)) {
                    look_at_object(lookbuf, cx, cy, glyph);
                    ++count;
                }
            }
            if (*lookbuf) {
                char coordbuf[20], which[12], cmode;
                if (count == 1) {
                    Strcpy(which, do_mons ? "monsters" : "objects");
                    if (TRUE)
                        Sprintf(outbuf, "Nearby %s (Toggle: ])",
                                upstart(which));
                    else
                        Sprintf(outbuf, "Shown %s",
                                which);
                    curses_menu_color_attr(win, NO_COLOR, A_REVERSE, ON);
                    mvwaddstr(win, 1, 1, outbuf);
                    curses_menu_color_attr(win, NO_COLOR, A_REVERSE, OFF);
                    #if 0
                    putstr(win, 0, "    "); /* separator */
                    #endif
                }
                Sprintf(outbuf, " %s %s ", 
                    decode_mixed(glyphbuf, encglyph(glyph)),
                    peaceful ? "+" : "-");
                /* guard against potential overflow */
                lookbuf[sizeof lookbuf - 1 - strlen(outbuf)] = '\0';
                Strcat(outbuf, lookbuf);
                curses_menu_color_attr(win, color, attr, ON);
                mvwaddstr(win, count + 1, 1, outbuf);
                curses_menu_color_attr(win, color, attr, OFF);
            }
        }
    }
    return;
}