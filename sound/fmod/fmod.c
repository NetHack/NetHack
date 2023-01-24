/* fmod.c */
/* NetHack may be freely redistributed.  See license for details. */

#ifdef SND_LIB_FMOD

#include "hack.h"
#include "fmod.h"

FMOD_SYSTEM *systemvar;
FMOD_SOUND *soundvar;
FMOD_CHANNEL *channel1;
FMOD_CHANNEL *channelMus;

static void fmod_init_nhsound(void);
static void fmod_exit_nhsound(const char *);
static void fmod_achievement(schar, schar, int32_t);
static void fmod_soundeffect(char *, int32_t, int32_t);
static void fmod_hero_playnotes(int32_t, char *, int32_t);
static void fmod_play_usersound(char *, int32_t, int32_t);

struct sound_procs fmod_procs = {
    SOUNDID(fmod),
    SNDCAP_USERSOUNDS,
    fmod_init_nhsound,
    fmod_exit_nhsound,
    fmod_achievement,
    fmod_soundeffect,
    fmod_hero_playnotes,
    fmod_play_usersound,
};

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
fmod_play_usersound(const char *filename, int32_t volume UNUSED, int32_t idx UNUSED)
{
    FMOD_System_CreateSound(systemvar, filename, FMOD_CREATESAMPLE, 0,
                            &soundvar);
    if (strstr(filename, "music_") != 0) {
        FMOD_Channel_Stop(channelMus);
        FMOD_System_PlaySound(systemvar, soundvar, 0, 0, &channelMus);
        FMOD_Channel_SetMode(channelMus, FMOD_LOOP_NORMAL);
    } else {
        FMOD_Channel_Stop(channel1);
        FMOD_System_PlaySound(systemvar, soundvar, 0, 0, &channel1);
    }
}

#endif /* SND_LIB_FMOD  */
       /* end of fmod.c */