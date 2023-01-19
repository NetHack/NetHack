/* NetHack 3.7	windsound.c	$NHDT-Date: $  $NHDT-Branch: $NHDT-Revision: $ */
/* Copyright (c) Michael Allison, 2022                                */
/* NetHack may be freely redistributed.  See license for details. */

#include "win32api.h"
#include "hack.h"
#include <mmsystem.h>

/*
 * The win32api sound interface
 *
 */

#include "win32api.h"
#include "hack.h"

#ifdef SND_LIB_WINDSOUND

static void windsound_init_nhsound(void);
static void windsound_exit_nhsound(const char *);
static void windsound_achievement(schar, schar, int32_t);
static void windsound_soundeffect(char *, int32_t, int32_t);
static void windsound_hero_playnotes(int32_t instrument, char *str, int32_t volume);
static void windsound_play_usersound(char *, int32_t, int32_t);

struct sound_procs windsound_procs = {
    SOUNDID(windsound),
    0L,
    windsound_init_nhsound,
    windsound_exit_nhsound,
    windsound_achievement,
    windsound_soundeffect,
    windsound_hero_playnotes,
    windsound_play_usersound,
};

void
windsound_init_nhsound(void)
{
    /* No steps required */
}

void
windsound_exit_nhsound(const char *reason)
{
}

void
windsound_achievement(schar ach1, schar ach2, int32_t repeat)
{
}

void
windsound_soundeffect(char *desc, int32_t seid, int32_t volume)
{
}

void
windsound_hero_playnotes(int32_t instrument, char *str, int32_t volume)
{
}

void
windsound_play_usersound(char *filename, int32_t volume UNUSED, int32_t idx UNUSED)
{
    /*    pline("play_usersound: %s (%d).", filename, volume); */
    (void) sndPlaySound(filename, SND_ASYNC | SND_NODEFAULT);
}

#endif /* SND_LIB_WINDSOUND */

