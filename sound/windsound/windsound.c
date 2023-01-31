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
static void windsound_hero_playnotes(int32_t instrument, const char *str, int32_t volume);
static void windsound_play_usersound(char *, int32_t, int32_t);

struct sound_procs windsound_procs = {
    SOUNDID(windsound),
    SOUND_TRIGGER_USERSOUNDS | SOUND_TRIGGER_SOUNDEFFECTS
        | SOUND_TRIGGER_HEROMUSIC,
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
#ifdef SND_SOUNDEFFECTS_AUTOMAP
    int reslt = 0;
    int32_t sefnflag = 0;
    char buf[PATHLEN];
    const char *filename;
    DWORD fdwsound;

    if (seid >= se_squeak_C || seid <= se_squeak_B) {
#if defined(__MINGW32__)
	/* The mingw32 resources don't seem to be able to be retrieved by the
	 * API PlaySound function with the SND_RESOURCE flag. Use files from
	 * the file system instead. */
        extern char *sounddir;   /* in sounds.c, set in files.c */
        char *mingw_exedir;

	if (!sounddir) {
	    exedir = mingw_exepath();
            if (exedir)
                if (strlen(exedir) < sizeof buf - 30) {
                    Strcpy(buf, exedir);
                    sefnflag = 2; /* 2 = use the directory name already in buf */
                }
        }
        filename = get_sound_effect_filename(seid, buf, sizeof buf, sefnflag);
        fdwsound = SND_ASYNC | SND_NODEFAULT;
#else
        sefnflag = 1;
	/* sefnflag = 1 means just obtain the soundeffect base name with
	 * no directory name and no file extension. That's because we're
	 * going to use the base soundeffect name as the name of a resource
	 * that's embedded into the .exe file, passing SND_RESOURCE flag to
	 * Windows API PlaySound().
         */
	filename = get_sound_effect_filename(seid, buf, sizeof buf, sefnflag);
	fdwsound = SND_ASYNC | SND_RESOURCE;
#endif
    } else {
        filename = get_sound_effect_filename(seid, buf, sizeof buf, sefnflag);
        fdwsound = SND_ASYNC | SND_NODEFAULT;
    }

    if (filename) {
        reslt = PlaySound(filename, NULL, fdwsound);
//        (void) sndPlaySound(filename, fdwsound);
    }
#endif
}

#define WAVEMUSIC_SOUNDS

void
windsound_hero_playnotes(int32_t instrument, const char *str, int32_t volume)
{
#ifdef WAVEMUSIC_SOUNDS
    int reslt = 0;
    boolean has_note_variations = FALSE;
    char resourcename[120], *end_of_res = 0;
    const char *c = 0;

    if (!str)
        return;
    resourcename[0] = '\0';
    switch(instrument) {
        case ins_flute:             /* WOODEN_FLUTE */
            Strcpy(resourcename, "sound_Wooden_Flute");
            has_note_variations = TRUE;
            break;
        case ins_pan_flute:         /* MAGIC_FLUTE */
            Strcpy(resourcename, "sound_Magic_Flute");
            has_note_variations = TRUE;
            break;
        case ins_english_horn:      /* TOOLED_HORN */
            Strcpy(resourcename, "sound_Tooled_Horn");
            has_note_variations = TRUE;
            break;
        case ins_french_horn:       /* FROST_HORN */
            Strcpy(resourcename, "sound_Frost_Horn");  
            break;
        case ins_baritone_sax:      /* FIRE_HORN */
            Strcpy(resourcename, "sound_Fire_Horn");  
            break;
        case ins_trumpet:           /* BUGLE */
            Strcpy(resourcename, "sound_Bugle");  
            has_note_variations = TRUE;
            break;
        case ins_orchestral_harp:   /* WOODEN_HARP */
            Strcpy(resourcename, "sound_Wooden_Harp");  
            has_note_variations = TRUE;
            break;
        case ins_cello:             /* MAGIC_HARP */
            Strcpy(resourcename, "sound_Magic_Harp");
            has_note_variations = TRUE;
            break;
        case ins_tinkle_bell:
            Strcpy(resourcename, "sound_Bell");
            break;
        case ins_taiko_drum:        /* DRUM_OF_EARTHQUAKE */
            Strcpy(resourcename, "sound_Drum_Of_Earthquake");
            break;
        case ins_melodic_tom:       /* LEATHER_DRUM */
            Strcpy(resourcename, "sound_Leather_Drum");
            break;
    }
    if (has_note_variations) {
        int i, idx = 0, notecount = strlen(str);
        static const char *const note_suffixes[]
                                = { "_A", "_B", "_C", "_D", "_E", "_F", "_G" };

        end_of_res = eos(resourcename);
        c = str;
        for (i = 0; i < notecount; ++i) {
            if (*c >= 'A' && *c <= 'G') {
                idx = (*c) - 'A';
                Strcpy(end_of_res, note_suffixes[idx]);
                if (i == (notecount - 1))
                    break;  /* drop out of for-loop and play it async below */
                reslt = PlaySound(resourcename, NULL,
                                  SND_SYNC | SND_RESOURCE);
            }
            c++;
        }
    }
    /* the final, or only, one is played ASYNC */
    reslt = PlaySound(resourcename, NULL, SND_ASYNC | SND_RESOURCE);
#endif
}

void
windsound_play_usersound(char *filename, int32_t volume UNUSED, int32_t idx UNUSED)
{
    /*    pline("play_usersound: %s (%d).", filename, volume); */
    (void) sndPlaySound(filename, SND_ASYNC | SND_NODEFAULT);
}

#endif /* SND_LIB_WINDSOUND */
