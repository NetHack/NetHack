/* fmod.c */
/* NetHack may be freely redistributed.  See license for details. */

#ifdef SND_LIB_FMOD

#include "hack.h"
#include "fmod.h"

FMOD_SYSTEM *systemvar;
FMOD_CHANNEL *channel1;
FMOD_CHANNEL *channelMus;

/*
 * fmod sound interface for NetHack
 *
 * Replace 'fmod' with your soundlib name in this template file.
 * Should be placed in ../sound/fmod/.
 */

#define soundlib_fmod soundlib_nosound + 1

static void fmod_init_nhsound(void);
static void fmod_exit_nhsound(const char *);
static void fmod_achievement(schar, schar, int32_t);
static void fmod_soundeffect(char *, int32_t, int32_t);
static void fmod_hero_playnotes(int32_t, char *, int32_t);
static void fmod_play_usersound(char *, int32_t, int32_t);

struct sound_procs fmod_procs = {
    SOUNDID(fmod),
    0L,
    SNDCAP_USERSOUNDS | SNDCAP_HEROMUSIC
        | SNDCAP_ACHIEVEMENTS |SNDCAP_SOUNDEFFECTS,
    fmod_init_nhsound,
    fmod_exit_nhsound,
    fmod_achievement,
    fmod_soundeffect,
    fmod_hero_playnotes,
    fmod_play_usersound,
};

/*
 *
 *  Types of potential sound supports (all are optionally implemented):
 *
 *      SNDCAP_USERSOUNDS     User-specified sounds that play based on config
 *                            file entries that identify a regular expression
 *                            to match against message window text, and identify
 *                            an external sound file to load in response.
 *                            The sound interface function pointer used to invoke
 *                            it:
 *
 *                             void (*sound_play_usersound)(char *filename,
 *                                             int32_t volume, int32_t idx);
 *
 *      SNDCAP_HEROMUSIC      Invoked by the core when the in-game hero is
 *                            playing a tune on an instrument. The sound
 *                            interface function pointer used to invoke it:
 *
 *                             void (*sound_hero_playnotes)(int32_t instrument,
 *                                                 char *str, int32_t volume);
 *
 *      SNDCAP_ACHIEVEMENTS   Invoked by the core when an in-game achievement
 *                            is reached. The soundlib routines could play
 *                            appropriate theme or mood music in response.
 *                            There would need to be a way to map the
 *                            achievements to external user-specified sounds.
 *                            The sound interface function pointer used to
 *                            invoke it:
 *
 *                                void (*sound_achievement)(schar, schar,
 *                                                          int32_t);
 *
 *      SNDCAP_SOUNDEFFECTS   Invoked by the core when something
 *                            sound-producing happens in the game. The soundlib
 *                            routines could play an appropriate sound effect
 *                            in response. They can be public-domain or
 *                            suitably-licensed stock sounds included with the
 *                            game source and made available during the build
 *                            process, or (not-yet-implemented) a way to
 *                            tie particular sound effects to a user-specified
 *                            sound fmods in a config file. The sound
 *                            interface function pointer used to invoke it:
 *
 *                               void (*sound_soundeffect)(char *desc, int32_t,
 *                                                              int32_t volume);
 *
 * The routines below would call into your sound library.
 * to fulfill the functionality.
 */

static void
fmod_init_nhsound(void)
{
    /* Initialize external sound library */
    FMOD_System_Create(&systemvar, FMOD_VERSION);
    FMOD_System_Init(systemvar, 32, FMOD_INIT_NORMAL, 0);
}

static void
fmod_exit_nhsound(const char *reason)
{
    /* Close / Terminate external sound library */
    FMOD_System_Close(&systemvar);
    FMOD_System_Release(&systemvar);
}

/* fulfill SNDCAP_ACHIEVEMENTS */
static void
fmod_achievement(schar ach1, schar ach2, int32_t repeat)
{


}

/* fulfill SNDCAP_SOUNDEFFECTS */
static void
fmod_soundeffect(char *desc, int32_t seid, int volume)
{

}

/* fulfill SNDCAP_HEROMUSIC */
static void fmod_hero_playnotes(int32_t instrument, char *str, int32_t volume)
{

}

/* fulfill  SNDCAP_USERSOUNDS */
static void
fmod_play_usersound(char *filename, int volume, int idx)
{
    FMOD_System_CreateSound(systemvar, filename, FMOD_CREATESAMPLE, 0,
                            &soundvar);
    if (strstr(filename, "music_") != 0) {
        FMOD_Channel_Stop(channelMus);
        FMOD_System_PlaySound(systemvar, soundvar, 0, 0, &channelMus);
        FMOD_Channel_SetMode(channelMus, FMOD_LOOP_NORMAL);
        FMOD_Channel_SetVolume(channelMus, .5);
    } else {
        FMOD_Channel_Stop(channel1);
        FMOD_System_PlaySound(systemvar, soundvar, 0, 0, &channel1);
    }
}

#endif /* SND_LIB_FMOD  */
       /* end of fmod.c */