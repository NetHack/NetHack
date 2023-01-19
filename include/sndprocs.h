/* NetHack 3.7	sndprocs.h	$NHDT-Date: $  $NHDT-Branch: $:$NHDT-Revision: $ */
/* Copyright (c) Michael Allison, 2022                                */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef SNDPROCS_H
#define SNDPROCS_H

/*
 *
 *  Types of potential sound supports (all are optional):
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
 *                            sound samples in a config file. The sound
 *                            interface function pointer used to invoke it:
 *
 *                               void (*sound_soundeffect)(char *desc, int32_t,
 *                                                              int32_t volume);
 *
 *  Development notes:
 *       - gc.chosen_soundlib holds the soundlib_id that will be initialized
 *         at the appropriate time (startup or after an option change). It
 *         is initialized to soundlib_nosound, so that is what will be used if
 *         the initial value isn't replaced via an assign_soundlib() call
 *         prior to the call to the activate_chosen_soundlib() in
 *         moveloop_preamble() at the start of the game.
 *       - ga.active_soundlib holds the soundlib_id of the active soundlib.
 *         It is initialized to soundlib_unassigned. It will get changed to
 *         reflect the activated soundlib_id once activate_chosen_soundlib()
 *         has been called.
 *
 */

enum soundlib_ids {
    soundlib_unassigned = 0,
#ifdef SND_LIB_QTSOUND
    soundlib_qtsound,
#endif
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
    soundlib_nosound
};

struct sound_procs {
    const char *soundname;
    enum soundlib_ids soundlib_id;
    unsigned long sndcap; /* capabilities in the port */
    void (*sound_init_nhsound)(void);
    void (*sound_exit_nhsound)(const char *);
    void (*sound_achievement)(schar, schar, int32_t);
    void (*sound_soundeffect)(char *desc, int32_t, int32_t volume);
    void (*sound_hero_playnotes)(int32_t instrument, char *str, int32_t volume);
    void (*sound_play_usersound)(char *filename, int32_t volume, int32_t idx);
};

extern struct sound_procs sndprocs;

#define SOUNDID(soundname) #soundname, soundlib_##soundname

/*
 * SOUNDCAP
 */
#define SNDCAP_USERSOUNDS   0x0001L
#define SNDCAP_HEROMUSIC    0x0002L
#define SNDCAP_ACHIEVEMENTS 0x0004L
#define SNDCAP_SOUNDEFFECTS 0x0008L
                            /* 28 free bits */

extern struct sound_procs soundprocs;

/* subset for NetHack */
enum instruments {
    ins_choir_aahs = 53, ins_trumpet = 57, ins_trombone = 58,
    ins_french_horn = 61, ins_english_horn = 70, ins_piccolo = 73,
    ins_flute = 74, ins_pan_flute = 76, ins_blown_bottle = 77,
    ins_whistle = 79, ins_tinkle_bell = 113, ins_woodblock = 116,
    ins_taiko_drum = 117, ins_melodic_tom = 118, ins_seashore = 123,
    ins_fencepost
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
    ins_fencepost
};
#endif
#endif /* SNDPROCS_H */
