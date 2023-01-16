/*   NetHack may be freely redistributed.  See license for details. 
 *                                                                  
 *
 * ntsound.c - Windows NT NetHack sound support
 *
 *Edit History:
 *     Initial Creation                              93/12/11
 *
 */

#include "fmod.h"
#include "fmod_helper.h"
#include "hack.h"

#ifdef USER_SOUNDS

FMOD_SOUND *soundvar;
FMOD_CHANNEL *channel1;
FMOD_CHANNEL *channelMus;

void
play_usersound(const char *filename, int volume UNUSED)
{
    FMOD_System_CreateSound(systemvar, filename, FMOD_CREATESAMPLE, 0,
                            &soundvar);
    if (strstr(filename, "music_") != 0) {
        FMOD_Channel_Stop(channelMus);
        FMOD_System_PlaySound(systemvar, soundvar, 0, 0, &channelMus);
        FMOD_Channel_SetMode(channelMus, FMOD_LOOP_NORMAL);
        FMOD_Channel_SetVolume(channelMus, .5);
    } 
    else {
        FMOD_Channel_Stop(channel1);
        FMOD_System_PlaySound(systemvar, soundvar, 0, 0, &channel1);
    }
}

#endif /*USER_SOUNDS*/
/* ntsound.c */
