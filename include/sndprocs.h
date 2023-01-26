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
 *                                             const char *str, int32_t volume);
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
    unsigned long sndcap; /* capabilities in the port */
    void (*sound_init_nhsound)(void);
    void (*sound_exit_nhsound)(const char *);
    void (*sound_achievement)(schar, schar, int32_t);
    void (*sound_soundeffect)(char *desc, int32_t, int32_t volume);
    void (*sound_hero_playnotes)(int32_t instrument, const char *str, int32_t volume);
    void (*sound_play_usersound)(char *filename, int32_t volume, int32_t idx);
};

extern struct sound_procs sndprocs;

#define SOUNDID(soundname) #soundname, ((enum soundlib_ids) soundlib_##soundname)

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

enum sound_effect_entries {
    se_zero_invalid                   = 0,
    se_faint_splashing                = 1,
    se_crackling_of_hellfire          = 2,
    se_heart_beat                     = 3,
    se_typing_noise                   = 4,
    se_hollow_sound                   = 5,
    se_rustling_paper                 = 6,
    se_crushing_sound                 = 7,
    se_splash                         = 8,
    se_chains_rattling_gears_turning  = 9,
    se_smashing_and_crushing          = 10,
    se_gears_turning_chains_rattling  = 11,
    se_loud_splash                    = 12,
    se_lound_crash                    = 13,
    se_crashing_rock                  = 14,
    se_sizzling                       = 15,
    se_crashing_boulder               = 16,
    se_boulder_drop                   = 17,
    se_item_tumble_downwards          = 18,
    se_drain_noises                   = 19,
    se_ring_in_drain                  = 20,
    se_groans_and_moans               = 21,
    se_scratching                     = 22,
    se_glass_shattering               = 23,
    se_egg_cracking                   = 24,
    se_gushing_sound                  = 25,
    se_glass_crashing                 = 26,
    se_egg_splatting                  = 27,
    se_sinister_laughter              = 28,
    se_blast                          = 29,
    se_stone_breaking                 = 30,
    se_stone_crumbling                = 31,
    se_snakes_hissing                 = 32,
    se_loud_pop                       = 33,
    se_clanking_pipe                  = 34,
    se_sewer_song                     = 35,
    se_monster_behind_boulder         = 36,
    se_wailing_of_the_banshee         = 37,
    se_swoosh                         = 38,
    se_explosion                      = 39,
    se_crashing_sound                 = 40,
    se_someone_summoning              = 41,
    se_rushing_wind_noise             = 42,
    se_splat_from_engulf              = 43,
    se_faint_sloshing                 = 44,
    se_crunching_sound                = 45,
    se_slurping_sound                 = 46,
    se_masticating_sound              = 47,
    se_distant_thunder                = 48,
    se_applause                       = 49,
    se_shrill_whistle                 = 50,
    se_someone_yells                  = 51,
    se_door_unlock_and_open           = 52,
    se_door_open                      = 53,
    se_door_crash_open                = 54,
    se_dry_throat_rattle              = 55,
    se_cough                          = 56,
    se_angry_snakes                   = 57,
    se_zap_then_explosion             = 58,
    se_zap                            = 59,
    se_horn_being_played              = 60,
    se_mon_chugging_potion            = 61,
    se_bugle_playing_reveille         = 62,
    se_crash_through_floor            = 63,
    se_thump                          = 64,
    se_scream                         = 65,
    se_tumbler_click                  = 66,
    se_gear_turn                      = 67,
    se_divine_music                   = 68,
    se_thunderclap                    = 69,
    se_sad_wailing                    = 70,
    se_maniacal_laughter              = 71,
    se_rumbling_of_earth              = 72,
    se_clanging_sound                 = 73,
    se_mutter_imprecations            = 74,
    se_mutter_incantation             = 75,
    se_angry_voice                    = 76,
    se_sceptor_pounding               = 77,
    se_courtly_conversation           = 78,
    se_low_buzzing                    = 79,
    se_angry_drone                    = 80,
    se_bees                           = 81,
    se_someone_searching              = 82,
    se_guards_footsteps               = 83,
    se_faint_chime                    = 84,
    se_loud_click                     = 85,
    se_soft_click                     = 86,
    se_squeak                         = 87,
    se_squeak_C                       = 88,
    se_squeak_D_flat                  = 89,
    se_squeak_D                       = 90,
    se_squeak_E_flat                  = 91,
    se_squeak_E                       = 92,
    se_squeak_F                       = 93,
    se_squeak_F_sharp                 = 94,
    se_squeak_G                       = 95,
    se_squeak_G_sharp                 = 96,
    se_squeak_A                       = 97,
    se_squeak_B_flat                  = 98,
    se_squeak_B                       = 99,
    se_someone_bowling                = 100,
    se_rumbling                       = 101,
    se_loud_crash                     = 102,
    se_deafening_roar_atmospheric     = 103,
    se_low_hum                        = 104,
    se_laughter                       = 105,
    se_cockatrice_hiss                = 106,
    se_chant                          = 107,
    se_cracking_sound                 = 108,
    se_ripping_sound                  = 109,
    se_thud                           = 110,
    se_clank                          = 111,
    se_crumbling_sound                = 112,
    se_soft_crackling                 = 113,
    se_crackling                      = 114,
    se_sharp_crack                    = 115,
    se_wall_of_force                  = 116,
    se_alarm                          = 117,
    se_kick_door_it_shatters          = 118,
    se_kick_door_it_crashes_open      = 119,
    se_bubble_rising                  = 120,
    se_bolt_of_lightning              = 121,
    se_board_squeak                   = 122,
    se_board_squeaks_loudly           = 123,
    se_boing                          = 124,
    se_crashed_ceiling                = 125,
    se_clash                          = 126,
    se_crash_door                     = 127,
    se_crash                          = 128,
    se_crash_throne_destroyed         = 129,
    se_crash_something_broke          = 130,
    se_kadoom_boulder_falls_in        = 131,
    se_klunk_pipe                     = 132,
    se_kerplunk_boulder_gone          = 133,
    se_klunk                          = 134,
    se_klick                          = 135,
    se_kaboom_door_explodes           = 136,
    se_kaboom_boom_boom               = 137,
    se_kaablamm_of_mine               = 138,
    se_kaboom                         = 139,
    se_splat_egg                      = 140,
    se_destroy_web                    = 141,
    se_iron_ball_dragging_you         = 142,
    se_iron_ball_hits_you             = 143,
    se_lid_slams_open_falls_shut      = 144,
    se_chain_shatters                 = 145,
    se_furious_bubbling               = 146,
    se_air_crackles                   = 147,
    se_potion_crash_and_break         = 148,
    se_hiss                           = 149,
    se_growl                          = 150,
    se_canine_bark                    = 151,
    se_canine_growl                   = 152,
    se_canine_whine                   = 153,
    se_canine_yip                     = 154,
    se_canine_howl                    = 155,
    se_canine_yowl                    = 156,
    se_canine_yelp                    = 157,
    se_feline_meow                    = 158,
    se_feline_purr                    = 159,
    se_feline_yip                     = 160,
    se_feline_mew                     = 161,
    se_feline_yowl                    = 162,
    se_feline_yelp                    = 163,
    se_roar                           = 164,
    se_snarl                          = 165,
    se_buzz                           = 166,
    se_squeek                         = 167,
    se_squawk                         = 168,
    se_squeal                         = 169,
    se_screech                        = 170,
    se_equine_neigh                   = 171,
    se_equine_whinny                  = 172,
    se_equine_whicker                 = 173,
    se_bovine_moo                     = 174,
    se_bovine_bellow                  = 175,
    se_wail                           = 176,
    se_groan                          = 177,
    se_grunt                          = 178,
    se_gurgle                         = 179,
    se_elephant_trumpet               = 180,
    se_snake_rattle                   = 181,
    se_yelp                           = 182,
    se_jabberwock_burble              = 183,
    se_shriek                         = 184,
    se_bone_rattle                    = 185,
    se_orc_grunt                      = 186,
    se_avian_screak                   = 187,
    number_of_se_entries
};

#if defined(SND_LIB_QTSOUND) || defined(SND_LIB_PORTAUDIO) \
        || defined(SND_LIB_OPENAL) || defined(SND_LIB_SDL_MIXER) \
        || defined(SND_LIB_MINIAUDIO) || defined(SND_LIB_FMOD) \
        || defined(SND_LIB_SOUND_ESCCODES) || defined(SND_LIB_VISSOUND) \
        || defined(SND_LIB_WINDSOUND) || defined(SND_LIB_MACSOUND)

#define SND_LIB_INTEGRATED   /* shortcut for conditional code in other files */

#define Play_usersound(filename, vol, idx) \
    do {                                                                  \
        if (!Deaf && soundprocs.sound_play_usersound                      \
            && ((soundprocs.sndcap & SNDCAP_USERSOUNDS) != 0))            \
            (*soundprocs.sound_play_usersound)((filename), (vol), (idx)); \
    } while(0)

#define Soundeffect(seid, vol) \
    do {                                                              \
        if (!Deaf && soundprocs.sound_soundeffect                     \
            && ((soundprocs.sndcap & SNDCAP_SOUNDEFFECTS) != 0))      \
            (*soundprocs.sound_soundeffect)(emptystr, (seid), (vol)); \
    } while(0)

#define Hero_playnotes(instrument, str, vol) \
    do {                                                                    \
        if (!Deaf && soundprocs.sound_hero_playnotes                        \
            && ((soundprocs.sndcap & SNDCAP_HEROMUSIC) != 0))               \
            (*soundprocs.sound_hero_playnotes)((instrument), (str), (vol)); \
    } while(0)
#else
#ifdef SND_LIB_INTEGRATED
#undef  SND_LIB_INTEGRATED
#endif
#define Play_usersound(filename, vol, idx)
#define Soundeffect(seid, vol)
#define Hero_playnotes(instrument, str, vol)
#endif

#endif /* SNDPROCS_H */
