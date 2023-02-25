/* NetHack 3.7	sndprocs.h	$NHDT-Date: $  $NHDT-Branch: $:$NHDT-Revision: $ */
/* Copyright (c) Michael Allison, 2022                                */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef SNDPROCS_H
#define SNDPROCS_H

enum soundlib_ids {
    soundlib_nosound,
#ifdef SND_LIB_PORTAUDIO
    soundlib_portaudio,
#endif
#ifdef SND_LIB_OPENAL
    soundlib_openal,
#endif
#ifdef SND_LIB_SDL_MIXER
    soundlib_sdl_mixer,
#endif
#ifdef SND_LIB_MINIAUDIO
    soundlib_miniaudio,
#endif
#ifdef SND_LIB_FMOD
    soundlib_fmod,
#endif
#ifdef SND_LIB_SOUND_ESCCODES
    soundlib_sound_esccodes,
#endif
#ifdef SND_LIB_VISSOUND
    soundlib_vissound,
#endif
#ifdef SND_LIB_WINDSOUND
    soundlib_windsound,
#endif
#ifdef SND_LIB_MACSOUND
    soundlib_macsound,
#endif
#ifdef SND_LIB_QTSOUND
    soundlib_qtsound,
#endif
    soundlib_notused
};

struct sound_procs {
    const char *soundname;
    enum soundlib_ids soundlib_id;
    unsigned long sound_triggers; /* capabilities in the port */
    void (*sound_init_nhsound)(void);
    void (*sound_exit_nhsound)(const char *);
    void (*sound_achievement)(schar, schar, int32_t);
    void (*sound_soundeffect)(char *desc, int32_t, int32_t volume);
    void (*sound_hero_playnotes)(int32_t instrument, const char *str,
                                 int32_t volume);
    void (*sound_play_usersound)(char *filename, int32_t volume, int32_t idx);
    void (*sound_ambience)(int32_t ambience_action, int32_t ambienceid,
                           int32_t proximity);
    void (*sound_verbal)(char *text, int32_t gender, int32_t tone,
                         int32_t vol, int32_t moreinfo);
};

struct sound_voice {
    int32_t serialno;
    int32_t gender;
    int32_t tone;
    int32_t volume;
    int32_t moreinfo;
    struct monst *mon;
    const char *nameid;
};

extern struct sound_procs sndprocs;

#define SOUNDID(soundname) #soundname, ((enum soundlib_ids) soundlib_##soundname)

/*
 * Types of triggers
 */
#define SOUND_TRIGGER_USERSOUNDS   0x0001L
#define SOUND_TRIGGER_HEROMUSIC    0x0002L
#define SOUND_TRIGGER_ACHIEVEMENTS 0x0004L
#define SOUND_TRIGGER_SOUNDEFFECTS 0x0008L
#define SOUND_TRIGGER_AMBIENCE     0x0010L
#define SOUND_TRIGGER_VERBAL       0x0020L
                            /* 26 free bits */

extern struct sound_procs soundprocs;

/* subset for NetHack */
enum instruments {
    ins_cello = 43, ins_orchestral_harp = 47, ins_choir_aahs = 53,
    ins_trumpet = 57, ins_trombone = 58, ins_french_horn = 61,
    ins_baritone_sax = 68, ins_english_horn = 70, ins_piccolo = 73,
    ins_flute = 74, ins_pan_flute = 76, ins_blown_bottle = 77,
    ins_whistle = 79, ins_tinkle_bell = 113, ins_woodblock = 116,
    ins_taiko_drum = 117, ins_melodic_tom = 118, ins_seashore = 123,
    ins_no_instrument
};

#if 0
enum instruments_broad {
    ins_acoustic_grand_piano = 1, ins_bright_acoustic_piano = 2,
    ins_electric_grand_piano = 3, ins_honkytonk_piano = 4,
    ins_electric_piano_1 = 5, ins_electric_piano_2 = 6,
    ins_harpsichord = 7, ins_clavinet = 8, ins_celesta = 9,
    ins_glockenspiel = 10, ins_music_box = 11, ins_vibraphone = 12,
    ins_marimba = 13, ins_xylophone = 14, ins_tubular_bells = 15,
    ins_dulcimer = 16, ins_drawbar_organ = 17,
    ins_percussive_organ = 18, ins_rock_organ = 19, ins_church_organ = 20,
    ins_reed_organ = 21, ins_french_accordion = 22, ins_harmonica = 23,
    ins_tango_accordion = 24, ins_acoustic_guitar__nylon = 25,
    ins_acoustic_guitar_steel = 26, ins_electric_guitar_jazz = 27,
    ins_electric_guitar_clean = 28, ins_electric_guitar_muted = 29,
    ins_overdriven_guitar = 30, ins_distortion_guitar = 31,
    ins_guitar_harmonics = 32, ins_acoustic_bass = 33,
    ins_electric_bass__fingered = 34, ins_electric_bass_picked = 35,
    ins_fretless_bass = 36, ins_slap_bass_1 = 37, ins_slap_bass_2 = 38,
    ins_synth_bass_1 = 39, ins_synth_bass_2 = 40, ins_violin = 41,
    ins_viola = 42, ins_cello = 43, ins_contrabass = 44,
    ins_tremolo_strings = 45, ins_pizzicato_strings = 46,
    ins_orchestral_harp = 47, ins_timpani = 48,
    ins_string_ensemble_1 = 49, ins_string_ensemble_2 = 50,
    ins_synthstrings_1 = 51, ins_synthstrings_2 = 52, ins_choir_aahs = 53,
    ins_voice_oohs = 54, ins_synth_voice = 55, ins_orchestra_hit = 56,
    ins_trumpet = 57, ins_trombone = 58, ins_tuba = 59, ins_muted_trumpet = 60,
    ins_french_horn = 61, ins_brass_section = 62, ins_synthbrass_1 = 63,
    ins_synthbrass_2 = 64, ins_soprano_sax = 65, ins_alto_sax = 66,
    ins_tenor_sax = 67, ins_baritone_sax = 68, ins_oboe = 69,
    ins_english_horn = 70, ins_bassoon = 71, ins_clarinet = 72,
    ins_piccolo = 73, ins_flute = 74, ins_recorder = 75,
    ins_pan_flute = 76, ins_blown_bottle = 77, ins_shakuhachi = 78,
    ins_whistle = 79, ins_ocarina = 80, ins_sitar = 105, ins_banjo = 106,
    ins_shamisen = 107, ins_koto = 108, ins_kalimba = 109, ins_bag_pipe = 110,
    ins_fiddle = 111, ins_shanai = 112, ins_tinkle_bell = 113, ins_agogo = 114,
    ins_steel_drums = 115, ins_woodblock = 116, ins_taiko_drum = 117,
    ins_melodic_tom = 118, ins_synth_drum = 119, ins_reverse_cymbal = 120,
    ins_guitar_fret_noise = 121, ins_breath_noise = 122, ins_seashore = 123,
    ins_bird_tweet = 124, ins_telephone_ring = 125, ins_helicopter = 126,
    ins_applause = 127, ins_gunshot = 128,
    ins_no_instrument
};
#endif

#define SEFFECTS_ENUM
enum sound_effect_entries {
    se_zero_invalid = 0,
#include "seffects.h"
    number_of_se_entries
};
#undef SEFFECTS_ENUM

enum ambience_actions {
    ambience_nothing, ambience_begin, ambience_end, ambience_update
};

enum ambiences {
    amb_noambience,
};

enum voice_moreinfo {
    voice_nothing_special,
    voice_talking_artifact = 0x0001,
    voice_deity            = 0x0002,
    voice_oracle           = 0x0004,
    voice_throne           = 0x0008,
    voice_death            = 0x0010
};

enum achievements_arg2 {
    sa2_zero_invalid, sa2_splashscreen, sa2_newgame_nosplash, sa2_restoregame,
    sa2_xplevelup, sa2_xpleveldown, number_of_sa2_entries
};

/*
Arguments for sound_achievement(schar arg1, schar arg2, int32_t aflags)

Arguments for actual achievements, those in you.h,
        arg1 = the achievement value.
        arg2 = 0 (irrelevant).
      aflags = 0 for first time, 1 for repeat.

These next ones make use of arg2, and aflags may be
filled with additional int values dependent on arg2.
arg1 must always be 0 for these.

SoundAchievement(0, sa2_splashscreen, 0);
SoundAchievement(0, sa2_newgame_nosplash, 0);
SoundAchievement(0, sa2_restoregame, 0);
SoundAchievement(0, sa2_levelup, level);
SoundAchievement(0, sa2_xpleveldown, level);
*/

#if defined(SND_LIB_QTSOUND) || defined(SND_LIB_PORTAUDIO) \
        || defined(SND_LIB_OPENAL) || defined(SND_LIB_SDL_MIXER) \
        || defined(SND_LIB_MINIAUDIO) || defined(SND_LIB_FMOD) \
        || defined(SND_LIB_SOUND_ESCCODES) || defined(SND_LIB_VISSOUND) \
        || defined(SND_LIB_WINDSOUND) || defined(SND_LIB_MACSOUND)

/* shortcut for conditional code in other files */
#define SND_LIB_INTEGRATED

#define Play_usersound(filename, vol, idx) \
    do {                                                                      \
        if (iflags.sounds && !Deaf && soundprocs.sound_play_usersound         \
            && ((soundprocs.sound_triggers & SOUND_TRIGGER_USERSOUNDS) != 0)) \
            (*soundprocs.sound_play_usersound)((filename), (vol), (idx));     \
    } while(0)

#define Soundeffect(seid, vol) \
    do {                                                                      \
        if (iflags.sounds && !Deaf && soundprocs.sound_soundeffect            \
          && ((soundprocs.sound_triggers & SOUND_TRIGGER_SOUNDEFFECTS) != 0)) \
            (*soundprocs.sound_soundeffect)(emptystr, (seid), (vol));         \
    } while(0)

/* Player's perspective, not the hero's; no Deaf suppression */
#define SoundeffectEvenIfDeaf(seid, vol) \
    do {                                                                      \
        if (iflags.sounds && !soundprocs.sound_soundeffect                    \
          && ((soundprocs.sound_triggers & SOUND_TRIGGER_SOUNDEFFECTS) != 0)) \
            (*soundprocs.sound_soundeffect)(emptystr, (seid), (vol));         \
    } while(0)

#define Hero_playnotes(instrument, str, vol) \
    do {                                                                     \
        if (iflags.sounds && !Deaf && soundprocs.sound_hero_playnotes        \
            && ((soundprocs.sound_triggers & SOUND_TRIGGER_HEROMUSIC) != 0)) \
            (*soundprocs.sound_hero_playnotes)((instrument), (str), (vol));  \
    } while(0)

/* Player's perspective, not the hero's; no Deaf suppression */
#define SoundAchievement(arg1, arg2, avals) \
    do {                                                                      \
        if (iflags.sounds && soundprocs.sound_achievement                     \
          && ((soundprocs.sound_triggers & SOUND_TRIGGER_ACHIEVEMENTS) != 0)) \
            (*soundprocs.sound_achievement)((arg1), (arg2), (avals));         \
    } while(0)

/* sound_speak is in sound.c */
#define SoundSpeak(text) \
    do {                                                                     \
        if ((gp.pline_flags & (PLINE_VERBALIZE | PLINE_SPEECH)) != 0         \
            && soundprocs.sound_verbal && iflags.voices                      \
            && ((soundprocs.sound_triggers & SOUND_TRIGGER_VERBAL) != 0))    \
            sound_speak(text);                                               \
    } while(0)

/* set_voice is in sound.c */
#define SetVoice(mon, tone, vol, moreinfo) \
    do {                                                                     \
        set_voice(mon, tone, vol, moreinfo);                                 \
    } while(0)

/*  void (*sound_achievement)(schar, schar, int32_t); */

#ifdef SOUNDLIBONLY
#undef SOUNDLIBONLY
#endif
#define SOUNDLIBONLY
#ifdef SND_SPEECH
#define VOICEONLY
#else
#define VOICEONLY UNUSED
#endif

#else  /*  NO SOUNDLIB IS INTEGRATED AFTER THIS */

#ifdef SND_LIB_INTEGRATED
#undef SND_LIB_INTEGRATED
#endif
#define Play_usersound(filename, vol, idx)
#define Soundeffect(seid, vol)
#define Hero_playnotes(instrument, str, vol)
#define SoundAchievement(arg1, arg2, avals)
#define SoundSpeak(text)
#define SetVoice(mon, tone, vol, moreinfo)
#ifdef SOUNDLIBONLY
#undef SOUNDLIBONLY
#endif
#define SOUNDLIBONLY UNUSED
#ifdef SND_SPEECH
#undef SND_SPEECH
#endif
#ifdef VOICEONLY
#undef VOICEONLY
#endif
#define VOICEONLY UNUSED

#endif  /* No SOUNDLIB */

enum findsound_approaches {
    findsound_embedded,
    findsound_soundfile
};

enum sound_file_flags {
    sff_default,            /* add dir prefix + '/' + sound + suffix */
    sff_base_only,          /* base sound name only, no dir, no suffix */
    sff_havedir_append_rest, /* dir provided, append base sound name + suffix */
    sff_baseknown_add_rest /* base is already known, add dir and suffix */
};

#endif /* SNDPROCS_H */
