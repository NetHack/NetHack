/* NetHack 3.6	rip.c	$NHDT-Date: 1488788514 2017/03/06 08:21:54 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.23 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2017. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#if defined(TTY_GRAPHICS) || defined(X11_GRAPHICS) || defined(GEM_GRAPHICS) \
    || defined(MSWIN_GRAPHICS) || defined(DUMPLOG) || defined(CURSES_GRAPHICS)
#define TEXT_TOMBSTONE
#endif
#if defined(mac) || defined(__BEOS__) || defined(WIN32_GRAPHICS)
#ifndef TEXT_TOMBSTONE
#define TEXT_TOMBSTONE
#endif
#endif

#ifdef TEXT_TOMBSTONE
STATIC_DCL void FDECL(center, (int, char *));

#ifndef NH320_DEDICATION
/* A normal tombstone for end of game display. */
static const char *rip_txt[] = {
    "                       ----------",
    "                      /          \\",
    "                     /    REST    \\",
    "                    /      IN      \\",
    "                   /     PEACE      \\",
    "                  /                  \\",
    "                  |                  |", /* Name of player */
    "                  |                  |", /* Amount of $ */
    "                  |                  |", /* Type of death */
    "                  |                  |", /* . */
    "                  |                  |", /* . */
    "                  |                  |", /* . */
    "                  |       1001       |", /* Real year of death */
    "                 *|     *  *  *      | *",
    "        _________)/\\\\_//(\\/(/\\)/\\//\\/|_)_______", 0
};
#define STONE_LINE_CENT 28 /* char[] element of center of stone face */
#else                      /* NH320_DEDICATION */
/* NetHack 3.2.x displayed a dual tombstone as a tribute to Izchak. */
static const char *rip_txt[] = {
    "              ----------                      ----------",
    "             /          \\                    /          \\",
    "            /    REST    \\                  /    This    \\",
    "           /      IN      \\                /  release of  \\",
    "          /     PEACE      \\              /   NetHack is   \\",
    "         /                  \\            /   dedicated to   \\",
    "         |                  |            |  the memory of   |",
    "         |                  |            |                  |",
    "         |                  |            |  Izchak Miller   |",
    "         |                  |            |   1935 - 1994    |",
    "         |                  |            |                  |",
    "         |                  |            |     Ascended     |",
    "         |       1001       |            |                  |",
    "      *  |     *  *  *      | *        * |      *  *  *     | *",
    " _____)/\\|\\__//(\\/(/\\)/\\//\\/|_)________)/|\\\\_/_/(\\/(/\\)/\\/\\/|_)____",
    0
};
#define STONE_LINE_CENT 19 /* char[] element of center of stone face */
#endif                     /* NH320_DEDICATION */
#define STONE_LINE_LEN                               \
    16               /* # chars that fit on one line \
                      * (note 1 ' ' border)          \
                      */
#define NAME_LINE 6  /* *char[] line # for player name */
#define GOLD_LINE 7  /* *char[] line # for amount of gold */
#define DEATH_LINE 8 /* *char[] line # for death description */
#define YEAR_LINE 12 /* *char[] line # for year */

static char **rip;

STATIC_OVL void
center(line, text)
int line;
char *text;
{
    register char *ip, *op;
    ip = text;
    op = &rip[line][STONE_LINE_CENT - ((strlen(text) + 1) >> 1)];
    while (*ip)
        *op++ = *ip++;
}

void
genl_outrip(tmpwin, how, when)
winid tmpwin;
int how;
time_t when;
{
    register char **dp;
    register char *dpx;
    char buf[BUFSZ];
    long year;
    register int x;
    int line;

    rip = dp = (char **) alloc(sizeof(rip_txt));
    for (x = 0; rip_txt[x]; ++x)
        dp[x] = dupstr(rip_txt[x]);
    dp[x] = (char *) 0;

    /* Put name on stone */
    Sprintf(buf, "%s", plname);
    buf[STONE_LINE_LEN] = 0;
    center(NAME_LINE, buf);

    /* Put $ on stone */
    Sprintf(buf, "%ld Au", done_money);
    buf[STONE_LINE_LEN] = 0; /* It could be a *lot* of gold :-) */
    center(GOLD_LINE, buf);

    /* Put together death description */
    formatkiller(buf, sizeof buf, how, FALSE);

    /* Put death type on stone */
    for (line = DEATH_LINE, dpx = buf; line < YEAR_LINE; line++) {
        register int i, i0;
        char tmpchar;

        if ((i0 = strlen(dpx)) > STONE_LINE_LEN) {
            for (i = STONE_LINE_LEN; ((i0 > STONE_LINE_LEN) && i); i--)
                if (dpx[i] == ' ')
                    i0 = i;
            if (!i)
                i0 = STONE_LINE_LEN;
        }
        tmpchar = dpx[i0];
        dpx[i0] = 0;
        center(line, dpx);
        if (tmpchar != ' ') {
            dpx[i0] = tmpchar;
            dpx = &dpx[i0];
        } else
            dpx = &dpx[i0 + 1];
    }

    /* Put year on stone */
    year = yyyymmdd(when) / 10000L;
    Sprintf(buf, "%4ld", year);
    center(YEAR_LINE, buf);

#ifdef DUMPLOG
    if (tmpwin == 0)
        dump_forward_putstr(0, 0, "Game over:", TRUE);
    else
#endif
        putstr(tmpwin, 0, "");

    for (; *dp; dp++)
        putstr(tmpwin, 0, *dp);

    putstr(tmpwin, 0, "");
#ifdef DUMPLOG
    if (tmpwin != 0)
#endif
        putstr(tmpwin, 0, "");

    for (x = 0; rip_txt[x]; x++) {
        free((genericptr_t) rip[x]);
    }
    free((genericptr_t) rip);
    rip = 0;
}

#endif /* TEXT_TOMBSTONE */

/*rip.c*/
