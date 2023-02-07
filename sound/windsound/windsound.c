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

/* sound interface routines */
static void windsound_init_nhsound(void);
static void windsound_exit_nhsound(const char *);
static void windsound_achievement(schar, schar, int32_t);
static void windsound_soundeffect(char *, int32_t, int32_t);
static void windsound_hero_playnotes(int32_t instrument, const char *str, int32_t volume);
static void windsound_play_usersound(char *, int32_t, int32_t);
static void windsound_ambience(int32_t ambienceid, int32_t ambience_action,
                               int32_t hero_proximity);
static void windsound_verbal(char *text, int32_t gender, int32_t tone,
                             int32_t vol, int32_t moreinfo);


/* supporting routines */
static void adjust_soundargs_for_compiler(int32_t *, DWORD *, char **);
static void maybe_preinsert_directory(int32_t, char *, char *, size_t);

struct sound_procs windsound_procs = {
    SOUNDID(windsound),
    SOUND_TRIGGER_USERSOUNDS | SOUND_TRIGGER_SOUNDEFFECTS
        | SOUND_TRIGGER_HEROMUSIC | SOUND_TRIGGER_AMBIENCE
        | SOUND_TRIGGER_ACHIEVEMENTS | SOUND_TRIGGER_VERBAL,
    windsound_init_nhsound,
    windsound_exit_nhsound,
    windsound_achievement,
    windsound_soundeffect,
    windsound_hero_playnotes,
    windsound_play_usersound,
    windsound_ambience,
    windsound_verbal,
};

static void
windsound_init_nhsound(void)
{
    /* No steps required */
}

static void
windsound_exit_nhsound(const char *reason)
{
}

static void
windsound_achievement(schar ach1, schar ach2, int32_t repeat)
{
    int reslt = 0;
    const char *filename;
    char resourcename[120], buf[PATHLEN];
    int findsound_approach = sff_base_only;
    DWORD fdwsound = SND_NODEFAULT;
    char *exedir = (char *) 0;

    adjust_soundargs_for_compiler(&findsound_approach, &fdwsound, &exedir);
    maybe_preinsert_directory(findsound_approach, exedir, buf, sizeof buf);
    fdwsound |= SND_ASYNC;

    if (ach1 == 0 && ach2 == 0)
        return;

    resourcename[0] = '\0';
    if (ach1 == 0) {
        int sa2 = (int) ach2;

        if (sa2 > sa2_zero_invalid && sa2 < number_of_sa2_entries) {
            switch(sa2) {
                case sa2_splashscreen:
                    Strcpy(resourcename, "sa2_splashscreen");
                    break;
                case sa2_newgame_nosplash:
                    Strcpy(resourcename, "sa2_newgame_nosplash");
                    break;
                case sa2_restoregame:
                    Strcpy(resourcename, "sa2_restoregame");
                    break;
                case sa2_xplevelup:
                    Strcpy(resourcename, "sa2_xplevelup");
                    break;
                case sa2_xpleveldown:
                    Strcpy(resourcename, "sa2_xpleveldown");
                    break;
            }
        }
    }
    if (resourcename[0] == '\0')
        return;
    adjust_soundargs_for_compiler(&findsound_approach, &fdwsound, &exedir);
    /* the final, or only, one is played ASYNC */
    maybe_preinsert_directory(findsound_approach, exedir, buf, sizeof buf);
    filename = base_soundname_to_filename(resourcename,
                                          buf, sizeof buf, findsound_approach);
    if (filename) {
        reslt = PlaySound(filename, NULL, fdwsound);
    }
}

static void
windsound_ambience(int32_t ambienceid, int32_t ambience_action,
                   int32_t hero_proximity)
{
}

static void
windsound_verbal(char *text, int32_t gender, int32_t tone,
                 int32_t vol, int32_t moreinfo)
{
}

static void
windsound_soundeffect(char *desc, int32_t seid, int32_t volume)
{
#ifdef SND_SOUNDEFFECTS_AUTOMAP
    int reslt = 0;
    int32_t findsound_approach = sff_base_only;
    char buf[PATHLEN];
    const char *filename;
    DWORD fdwsound = SND_NODEFAULT;
    char *exedir = 0;

    adjust_soundargs_for_compiler(&findsound_approach, &fdwsound, &exedir);
    maybe_preinsert_directory(findsound_approach, exedir, buf, sizeof buf);
    fdwsound |= SND_ASYNC;
    if (seid >= se_squeak_C || seid <= se_squeak_B) {
        filename = get_sound_effect_filename(seid, buf, sizeof buf, findsound_approach);
    } else {
        filename = get_sound_effect_filename(seid, buf, sizeof buf, findsound_approach);
        fdwsound &= ~(SND_RESOURCE);
        fdwsound |= SND_FILENAME;
    }

    if (filename) {
        reslt = PlaySound(filename, NULL, fdwsound);
    }
#endif
}

#define WAVEMUSIC_SOUNDS

static void
windsound_hero_playnotes(int32_t instrument, const char *str, int32_t volume)
{
#ifdef WAVEMUSIC_SOUNDS
    int reslt = 0;
    boolean has_note_variations = FALSE;
    const char *filename;
    char resourcename[120], buf[PATHLEN], *end_of_res = 0;
    const char *c = 0;
    int findsound_approach = sff_base_only;
    DWORD fdwsound = SND_NODEFAULT;
    char *exedir = (char *) 0;

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
    adjust_soundargs_for_compiler(&findsound_approach, &fdwsound, &exedir);
    if (has_note_variations) {
        int i, idx = 0, notecount = strlen(str);
        static const char *const note_suffixes[]
                                = { "_A", "_B", "_C", "_D", "_E", "_F", "_G" };

        end_of_res = eos(resourcename);
        c = str;
        fdwsound |= SND_SYNC;
        for (i = 0; i < notecount; ++i) {
            if (*c >= 'A' && *c <= 'G') {
                idx = (*c) - 'A';
                Strcpy(end_of_res, note_suffixes[idx]);
                maybe_preinsert_directory(findsound_approach, exedir, buf, sizeof buf);
                filename = base_soundname_to_filename(resourcename,
                                               buf, sizeof buf, findsound_approach);
                if (i == (notecount - 1))
                    break;  /* drop out of for-loop and play it async below */
                reslt = PlaySound(buf, NULL, fdwsound);
            }
            c++;
        }
        fdwsound &= ~(SND_SYNC);
    }
    fdwsound |= SND_ASYNC;
    /* the final, or only, one is played ASYNC */
    maybe_preinsert_directory(findsound_approach, exedir, buf, sizeof buf);
    reslt = PlaySound(buf, NULL, fdwsound);
#endif
}

static void
windsound_play_usersound(char *filename, int32_t volume UNUSED, int32_t idx UNUSED)
{
    /*    pline("play_usersound: %s (%d).", filename, volume); */
    (void) sndPlaySound(filename, SND_ASYNC | SND_NODEFAULT | SND_FILENAME);
}

static void
adjust_soundargs_for_compiler(
    int32_t *sefilename_flags,
    DWORD *fdwsound,
    char **dirbuf)
{
    /* The mingw32 resources don't seem to be able to be retrieved by the
     * API PlaySound function with the SND_RESOURCE flag. Use files from
     * the file system instead. */

    enum findsound_approaches approach =
#if !defined(__MINGW32__)
        findsound_embedded;
#else
        findsound_soundfile;
#endif

    if (approach == findsound_soundfile) {
        char *exe_dir;

        if (*sefilename_flags == sff_base_only
            || *sefilename_flags == sff_baseknown_add_rest) {
            exe_dir = windows_exepath();
            if (exe_dir) {
                *dirbuf = exe_dir;
                /* switch the sff_base_only to a sff_havedir_append_rest */
                *sefilename_flags = sff_havedir_append_rest;
                *fdwsound |= SND_FILENAME;
            }
        }
    } else {
        if (*sefilename_flags == sff_base_only
            || *sefilename_flags == sff_baseknown_add_rest) {
            /* *sefilename_flags = sff_base_only means just obtain the
             * soundeffect base name with no directory name and no file
             * extension. That's because we're going to use the base
             * soundeffect name as the name of a resource that's embedded
             * into the .exe file, passing SND_RESOURCE flag to
             * Windows API PlaySound().
             */
            *sefilename_flags = sff_base_only;
            *fdwsound |= SND_RESOURCE;
        }
    }
}

static void
maybe_preinsert_directory(int32_t findsound_approach, char *exedir, char *buf, size_t sz)
{
    int largest_basename = 35;

    /* findsound_approach = sff_havdir_append_rest means a directory name will be
     * inserted into the begining of buf and the remaining parts of the
     * resource/file name will be appended by
     * get_sound_effect_filename(seid, buf, sizeof buf, findsound_approach)
     * when it sees the sff_havedir_append_rest indicator.
     */

    if (findsound_approach == sff_havedir_append_rest) {
        if (exedir) {
            if (strlen(exedir) < (sz - largest_basename))
                Strcpy(buf, exedir);
            else
                *buf = '\0';
        }
    }
}
#endif /* SND_LIB_WINDSOUND */
