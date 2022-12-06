/* NetHack 3.7	ntsound.c	$NHDT-Date: 1596498316 2020/08/03 23:45:16 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.13 $ */
/*   Copyright (c) NetHack PC Development Team 1993                 */
/*   NetHack may be freely redistributed.  See license for details. */
/*                                                                  */
/*
 * ntsound.c - Windows NT NetHack sound support
 *
 *Edit History:
 *     Initial Creation                              93/12/11
 *
 */

#include "win32api.h"
#include "hack.h"
#include <mmsystem.h>

#ifdef USER_SOUNDS

void
play_usersound(const char* filename, int volume UNUSED)
{
    /*    pline("play_usersound: %s (%d).", filename, volume); */
    (void) sndPlaySound(filename, SND_ASYNC | SND_NODEFAULT);
}

#endif /*USER_SOUNDS*/
/* ntsound.c */
