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

#include "fmod.h"
#include "hack.h"

#ifdef USER_SOUNDS

FMOD_SYSTEM *systemvar;
FMOD_SOUND *soundvar;
FMOD_CHANNEL *channel = 0; 


void
play_usersound(const char *filename, int volume UNUSED)
{
    FMOD_System_Create(&systemvar, FMOD_VERSION);
    FMOD_System_Init(systemvar, 32, FMOD_INIT_NORMAL, NULL);
    FMOD_System_CreateSound(systemvar, filename, FMOD_CREATESAMPLE, 0, &soundvar);
    FMOD_System_PlaySound(systemvar, soundvar, 0, 0, NULL);
}

#endif /*USER_SOUNDS*/
/* ntsound.c */
