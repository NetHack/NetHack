/* NetHack 5.0	mgetline.c	$NHDT-Date: 1432512797 2015/05/25 00:13:17 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Kenneth Lorber, Kensington, Maryland, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "mactty.h"
#include "macwin.h"
#include "func_tab.h"

extern int extcmd_via_menu(void); /* cmd.c */

static int
get_line_from_key_queue(char *bufp)
{
    *bufp = 0;
    if (try_key_queue(bufp)) {
        while (*bufp) {
            if (*bufp == 10 || *bufp == 13) {
                *bufp = 0;
            }
            bufp++;
        }
        return true;
    }
    return false;
}

static void
topl_getlin(const char *query, char *bufp, Boolean ext)
{
    if (get_line_from_key_queue(bufp))
        return;

    enter_topl_mode((char *) query);
    while (topl_key(nhgetch(), ext))
        ;
    leave_topl_mode(bufp);
}

/* Read a line into bufp[BUFSZ]; escape interrupts, yielding "\033". */
void
mac_getlin(const char *query, char *bufp)
{
    topl_getlin(query, bufp, false);
}

/* Read an extended command: getlin followed by lookup in extcmdlist. */
int
mac_get_ext_cmd()
{
    char bufp[BUFSZ];
    int i;

    if (iflags.extmenu)
        return extcmd_via_menu();
    topl_getlin("# ", bufp, true);
    for (i = 0; extcmdlist[i].ef_txt != (char *) 0; i++)
        if (!strcmp(bufp, extcmdlist[i].ef_txt))
            break;
    if (extcmdlist[i].ef_txt == (char *) 0)
        i = -1; /* not found */

    return i;
}

/* macgetline.c */
