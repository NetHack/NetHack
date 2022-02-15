/* NetHack 3.7	nhlan.c	$NHDT-Date: 1596498282 2020/08/03 23:44:42 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.11 $ */
/* Copyright (c) Michael Allison, 1997                  */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Currently shared by the following ports:
 *	WIN32
 *
 * The code in here is used to take advantage of added features
 * that might be available in a Local Area Network environment.
 *
 * 	Network Username of player
 */

#include "hack.h"
#include <ctype.h>

#ifdef LAN_FEATURES

void
init_lan_features(void)
{
    lan_username();
}
/*
 * The get_lan_username() call is a required call, since some of
 * the other LAN features depend on a unique username being available.
 *
 */

char *
lan_username(void)
{
    char *lu;
    lu = get_username(&g.lusername_size);
    if (lu) {
        Strcpy(g.lusername, lu);
        return g.lusername;
    } else
        return (char *) 0;
}
#endif /*LAN_FEATURES*/
/*nhlan.c*/
