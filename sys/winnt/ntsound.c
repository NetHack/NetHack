/*   SCCS Id: @(#)ntsound.c   3.4     $Date$                        */
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

#include "hack.h"
#include "win32api.h"
#include <mmsystem.h>

#ifdef USER_SOUNDS

void play_usersound(filename, volume)
const char* filename;
int volume;
{
/*    pline("play_usersound: %s (%d).", filename, volume); */
	(void)sndPlaySound(filename, SND_ASYNC | SND_NODEFAULT);
}

#endif /*USER_SOUNDS*/
/* ntsound.c */
