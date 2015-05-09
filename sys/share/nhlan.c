/* NetHack 3.6	nhlan.c	$NHDT-Date: 1431192778 2015/05/09 17:32:58 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/* NetHack 3.6	nhlan.c	$Date: 2009/05/06 10:50:26 $  $Revision: 1.5 $ */
/*	SCCS Id: @(#)nhlan.c	3.5	1999/11/21	*/
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
init_lan_features()
{
    lan_username();
}
/*
 * The get_lan_username() call is a required call, since some of
 * the other LAN features depend on a unique username being available.
 *
 */
char lusername[MAX_LAN_USERNAME];
int lusername_size = MAX_LAN_USERNAME;

char *
lan_username()
{
    char *lu;
    lu = get_username(&lusername_size);
    if (lu) {
        Strcpy(lusername, lu);
        return lusername;
    } else
        return (char *) 0;
}
#endif /*LAN_FEATURES*/
/*nhlan.c*/
