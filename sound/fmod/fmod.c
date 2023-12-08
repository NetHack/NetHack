/* fmod.c */
/* Copyright (c) Taylor Daley, 2023. */
/* NetHack may be freely redistributed.  See license for details. */

#ifdef SND_LIB_FMOD

#include "hack.h"
#include "fmod.h"

static FMOD_SYSTEM *systemvar;
static FMOD_SOUND *soundvar;
static FMOD_CHANNEL *channel1;
static FMOD_CHANNEL *channelMus;

static void fmod_init_nhsound(void);
static void fmod_exit_nhsound(const char *);
static void fmod_achievement(schar, schar, int32_t);
static void fmod_soundeffect(char *, int32_t, int32_t);
static void fmod_hero_playnotes(int32_t instrument, const char *str, int32_t volume);
static void fmod_play_usersound(char *, int32_t, int32_t);
static void fmod_ambience(int32_t, int32_t, int32_t);
static void fmod_verbal(char *, int32_t, int32_t, int32_t, int32_t);

struct sound_procs fmod_procs = {
    SOUNDID(fmod),
    SOUND_TRIGGER_USERSOUNDS | 0,
    fmod_init_nhsound,
    fmod_exit_nhsound,
    fmod_achievement,
    fmod_soundeffect,
    fmod_hero_playnotes,
    fmod_play_usersound,
    fmod_ambience,
    fmod_verbal
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
    FMOD_System_Close(systemvar);
    FMOD_System_Release(systemvar);
}

/* fulfill SNDCAP_ACHIEVEMENTS */
static void
fmod_achievement(schar ach1, schar ach2, int32_t repeat)
{
    //  to be added in future
}

/* fulfill SNDCAP_SOUNDEFFECTS */
static void
fmod_soundeffect(char *desc, int32_t seid, int32_t volume)
{
    //  to be added in future
}

/* fulfill SNDCAP_HEROMUSIC */
static void fmod_hero_playnotes(int32_t instrument, const char *str, int32_t volume)
{
    //  to be added in future
}

/* fulfill  SOUND_TRIGGER_USERSOUNDS */
static void
fmod_play_usersound(char *filename, int32_t volume UNUSED, int32_t idx UNUSED)
{
    FMOD_System_CreateSound(systemvar, filename, FMOD_CREATESAMPLE, 0,
                            &soundvar);
    if (strstr(filename, "music_") != NULL) {
        FMOD_Channel_Stop(channelMus);
        FMOD_System_PlaySound(systemvar, soundvar, 0, 0, &channelMus);
        FMOD_Channel_SetMode(channelMus, FMOD_LOOP_NORMAL);
    } else {
        FMOD_Channel_Stop(channel1);
        FMOD_System_PlaySound(systemvar, soundvar, 0, 0, &channel1);
    }
}

static void
fmod_ambience(int32_t ambienceid, int32_t ambience_action,
              int32_t hero_proximity)
{
    //  to be added in future
}

static void
fmod_verbal(char *text, int32_t gender, int32_t tone,
            int32_t vol, int32_t moreinfo)
{
    //  to be added in future
}

#endif /* SND_LIB_FMOD  */
       /* end of fmod.c */
