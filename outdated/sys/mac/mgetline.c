/* NetHack 3.6	mgetline.c	$NHDT-Date: 1432512797 2015/05/25 00:13:17 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Kenneth Lorber, Kensington, Maryland, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "mactty.h"
#include "macwin.h"
#include "macpopup.h"
#include "func_tab.h"

extern int NDECL(extcmd_via_menu); /* cmd.c */

typedef Boolean FDECL((*key_func), (unsigned char));

int
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

/*
 * Read a line closed with '\n' into the array char bufp[BUFSZ].
 * (The '\n' is not stored. The string is closed with a '\0'.)
 * Reading can be interrupted by an escape ('\033') - now the
 * resulting string is "\033".
 */
void
mac_getlin(const char *query, char *bufp)
{
    topl_getlin(query, bufp, false);
}

/* Read in an extended command - doing command line completion for
 * when enough characters have been entered to make a unique command.
 * This is just a modified getlin() followed by a lookup.   -jsb
 */
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
