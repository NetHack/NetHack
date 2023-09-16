/* macsound.m */
/* Copyright Michael Allison, 2023 */
/* NetHack may be freely redistributed.  See license for details. */


/*
 * macsound
 */

#include "hack.h"

#ifdef DEBUG
#undef DEBUG
#endif

/* #import <Foundation/Foundation.h> */
#import <AppKit/AppKit.h>

#define SPEECH_SOUNDS
#ifdef SPEECH_SOUNDS
#import <AVFoundation/AVFoundation.h>
#endif

static void macsound_init_nhsound(void);
static void macsound_exit_nhsound(const char *);
static void macsound_achievement(schar, schar, int32_t);
static void macsound_soundeffect(char *, int32_t, int32_t);
static void macsound_hero_playnotes(int32_t, const char *, int32_t);
static void macsound_play_usersound(char *, int32_t, int32_t);
static void macsound_ambience(int32_t, int32_t, int32_t);
static void macsound_verbal(char *, int32_t, int32_t, int32_t, int32_t);
static int affiliate(int32_t seid, const char *soundname);



/*
 * Sound capabilities that can be enabled:
 *    SOUND_TRIGGER_USERSOUNDS | SOUND_TRIGGER_HEROMUSIC
 *        | SOUND_TRIGGER_ACHIEVEMENTS | SOUND_TRIGGER_SOUNDEFFECTS
 *        | SOUND_TRIGGER_AMBIENCE,
 */

struct sound_procs macsound_procs = {
    SOUNDID(macsound),
    SOUND_TRIGGER_HEROMUSIC | SOUND_TRIGGER_SOUNDEFFECTS
        | SOUND_TRIGGER_ACHIEVEMENTS
#ifdef SND_SPEECH
        | SOUND_TRIGGER_VERBAL
#endif
    |0,
    macsound_init_nhsound,
    macsound_exit_nhsound,
    macsound_achievement,
    macsound_soundeffect,
    macsound_hero_playnotes,
    macsound_play_usersound,
    macsound_ambience,
    macsound_verbal,
};

static void
macsound_init_nhsound(void)
{
    /* Initialize external sound library */
}

static void
macsound_exit_nhsound(const char *reason UNUSED)
{
    /* Close / Terminate external sound library */

}

static void
macsound_ambience(int32_t ambienceid UNUSED, int32_t ambience_action UNUSED,
                int32_t hero_proximity UNUSED)
{
}

/* magic number 47 is the current number of sound_ files to include */
#define EXTRA_SOUNDS 47

static int32_t affiliation[number_of_se_entries + number_of_sa2_entries + EXTRA_SOUNDS] = { 0 };
static NSString *soundstring[number_of_se_entries + number_of_sa2_entries + EXTRA_SOUNDS];
static NSSound *seSound[number_of_se_entries + number_of_sa2_entries + EXTRA_SOUNDS];

#ifdef SND_SOUNDEFFECTS_AUTOMAP
#define AUTOMAPONLY
#else
#define AUTOMAPONLY UNUSED
#endif

/* fulfill SOUND_TRIGGER_SOUNDEFFECTS */
static void
macsound_soundeffect(char *desc UNUSED, int32_t seid AUTOMAPONLY, int vol AUTOMAPONLY)
{
#ifdef SND_SOUNDEFFECTS_AUTOMAP

  /* Supposedly, the following locations are searched in this order:
   *  1. the application's main bundle
   *  2. ~/Library/Sounds
   *  3. /Library/Sounds
   *  4. /Network/Library/Sounds
   *  5. /System/Library/Sounds
   */

    char buf[1024];
    const char *soundname;
    float fvolume = (float) vol / 100.00;

    if (fvolume < 0.1 || fvolume > 1.0)
        fvolume = 1.0;

    if (seid <= se_zero_invalid || seid >= number_of_se_entries)
        return;
    if (!affiliation[seid]) {
        soundname = get_sound_effect_filename(seid, buf, sizeof buf,
                                              sff_base_only);
        if (soundname) {
            affiliate(seid, soundname);
        }
    }
    if (affiliation[seid]) {
        if ([seSound[seid] isPlaying])
            [seSound[seid] stop];
        if ([seSound[seid] volume] != fvolume)
            [seSound[seid] setVolume:fvolume];
        [seSound[seid] play];
    }
#endif
}

#define WAVEMUSIC_SOUNDS

#ifdef WAVEMUSIC_SOUNDS
#define WAVEMUSICONLY
#else
#define WAVEMUSICONLY UNUSED
#endif

/* This is the number of sound_ files that support WAVEMUSIC_SOUNDS */
static const int wavemusic_sound_count = EXTRA_SOUNDS;

/* fulfill SOUND_TRIGGER_HEROMUSIC */
static void macsound_hero_playnotes(int32_t instrument WAVEMUSICONLY,
                  const char *str WAVEMUSICONLY, int32_t vol WAVEMUSICONLY)
{
#ifdef WAVEMUSIC_SOUNDS
    uint32_t pseudo_seid, pseudo_seid_base = 0;
    BOOL has_note_variations = false;
    char resourcename[120], *end_of_res = 0;
    const char *c = 0;
    float fvolume = (float) vol / 100.00;
    int time_remaining = 0;
    NSTimeInterval dur;
    NSTimeInterval elapsed;

    if (fvolume < 0.1 || fvolume > 1.0)
        fvolume = 1.0;

    if (!str)
        return;
    resourcename[0] = '\0';
    switch(instrument) {
        case ins_tinkle_bell:
            Strcpy(resourcename, "sound_Bell");
            pseudo_seid_base = 0;
            break;
        case ins_taiko_drum:        /* DRUM_OF_EARTHQUAKE */
            Strcpy(resourcename, "sound_Drum_Of_Earthquake");
            pseudo_seid_base = 1;
            break;
        case ins_baritone_sax:      /* FIRE_HORN */
            Strcpy(resourcename, "sound_Fire_Horn");
            pseudo_seid_base = 2;
            break;
        case ins_french_horn:       /* FROST_HORN */
            Strcpy(resourcename, "sound_Frost_Horn");
            pseudo_seid_base = 3;
            break;
        case ins_melodic_tom:       /* LEATHER_DRUM */
            Strcpy(resourcename, "sound_Leather_Drum");
            pseudo_seid_base = 4;
            break;
        case ins_trumpet:           /* BUGLE */
            Strcpy(resourcename, "sound_Bugle");
            has_note_variations = true;
            pseudo_seid_base = 5;
            break;
        case ins_cello:             /* MAGIC_HARP */
            Strcpy(resourcename, "sound_Magic_Harp");
            has_note_variations = true;
            pseudo_seid_base = 12;
            break;
        case ins_english_horn:      /* TOOLED_HORN */
            Strcpy(resourcename, "sound_Tooled_Horn");
            has_note_variations = true;
            pseudo_seid_base = 19;
            break;
        case ins_flute:             /* WOODEN_FLUTE */
            Strcpy(resourcename, "sound_Wooden_Flute");
            has_note_variations = true;
            pseudo_seid_base = 26;
            break;
        case ins_orchestral_harp:   /* WOODEN_HARP */
            Strcpy(resourcename, "sound_Wooden_Harp");
            has_note_variations = true;
            pseudo_seid_base = 33;
            break;
        case ins_pan_flute:         /* MAGIC_FLUTE */
            Strcpy(resourcename, "sound_Magic_Flute");
            has_note_variations = true;
            pseudo_seid_base = 40;
            break;
    }
    pseudo_seid_base += number_of_se_entries; /* get past se_ entries */

    int i, idx = 0, notecount = strlen(str);
    static const char *const note_suffixes[]
                            = { "_A", "_B", "_C", "_D", "_E", "_F", "_G" };

    end_of_res = eos(resourcename);
    c = str;
    for (i = 0; i < notecount; ++i) {
        if (*c >= 'A' && *c <= 'G') {
            pseudo_seid = pseudo_seid_base;
            if (has_note_variations) {
                idx = (*c) - 'A';
                pseudo_seid += idx;
                if (pseudo_seid >= SIZE(affiliation))
                    break;
                Strcpy(end_of_res, note_suffixes[idx]);
            }
            if (!affiliation[pseudo_seid]) {
                affiliate(pseudo_seid, resourcename);
            }
            if (affiliation[pseudo_seid]) {
                if ([seSound[pseudo_seid] isPlaying]) {
                    dur = [seSound[pseudo_seid] duration];
                    elapsed = [seSound[pseudo_seid] currentTime];
                    time_remaining = (int) ((dur - elapsed) * 1000);
                    msleep(time_remaining);
                }
                if ([seSound[pseudo_seid] isPlaying])
                    [seSound[pseudo_seid] stop];
                if ([seSound[pseudo_seid] volume] != fvolume)
                    [seSound[pseudo_seid] setVolume:fvolume];
                [seSound[pseudo_seid] play];
#if 0
                if (i < notecount - 1) {
                    /* more notes to follow */
                    msleep(150);
                    [seSound[pseudo_seid] stop];
                }
#endif
            }
        }
        c++;
    }
#endif
}

#define ACHIEVEMENT_SOUNDS

#ifdef ACHIEVEMENT_SOUNDS
#define ACHIEVEONLY
#else
#define ACHIEVEONLY UNUSED
#endif

/* fulfill SOUND_TRIGGER_ACHIEVEMENTS */
static void
macsound_achievement(schar ach1 ACHIEVEONLY,
schar ach2 ACHIEVEONLY,
int32_t moreinfo UNUSED)
{
#ifdef ACHIEVEMENT_SOUNDS
    char resourcename[120];
    uint32_t pseudo_seid, pseudo_seid_base;
    int time_remaining = 0;
    NSTimeInterval dur;
    NSTimeInterval elapsed;

    if (ach1 == 0 && ach2 == 0)
        return;

    resourcename[0] = '\0';
    pseudo_seid_base = 0;
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
            if (resourcename[0] == '\0')
                return;

            /* get past se_ entries and the wavemusic entries */
            pseudo_seid_base += (number_of_se_entries + wavemusic_sound_count);

            pseudo_seid = pseudo_seid_base + sa2;
            if (!affiliation[pseudo_seid]) {
                affiliate(pseudo_seid, resourcename);
            }
            if (affiliation[pseudo_seid]) {
                if ([seSound[pseudo_seid] isPlaying]) {
                    dur = [seSound[pseudo_seid] duration];
                    elapsed = [seSound[pseudo_seid] currentTime];
                    time_remaining = (int) ((dur - elapsed) * 1000);
                    msleep(time_remaining);
                }
                if ([seSound[pseudo_seid] isPlaying]) {
                    [seSound[pseudo_seid] stop];
                }
                [seSound[pseudo_seid] play];
            }
        }
    }
#endif
}

/* fulfill  SOUND_TRIGGER_USERSOUNDS */
static void
macsound_play_usersound(char *filename UNUSED, int volume UNUSED, int idx UNUSED)
{

}

#ifdef SND_SPEECH
#define SPEECHONLY
#else
#define SPEECHONLY UNUSED
#endif

static void
macsound_verbal(char *text SPEECHONLY, int32_t gndr SPEECHONLY, int32_t tone_of_voice UNUSED, int32_t vol UNUSED, int32_t moreinfo UNUSED)
{
#ifdef SND_SPEECH
    NSMutableString *speechstring;
    AVSpeechUtterance *text2speechutt;
    AVSpeechSynthesizer *synthesizer;
 
    synthesizer = [[AVSpeechSynthesizer alloc]init];
    speechstring = [NSMutableString stringWithUTF8String:text];
    text2speechutt = [AVSpeechUtterance speechUtteranceWithString:speechstring];
    text2speechutt.rate = 0.50f;
    text2speechutt.pitchMultiplier = 0.8f;
    text2speechutt.postUtteranceDelay = 0.2f;
    text2speechutt.volume = (float) vol / 100;
    if (gndr > 0)
        text2speechutt.voice = [AVSpeechSynthesisVoice voiceWithIdentifier:@"com.apple.ttsbundle.siri_female_en-GB_compact"];
    else
        text2speechutt.voice = [AVSpeechSynthesisVoice voiceWithIdentifier:@"com.apple.ttsbundle.siri_male_en-GB_compact"];

    [synthesizer speakUtterance:text2speechutt];
#endif  /* SND_SPEECH */
}

static int
affiliate(int32_t id, const char *soundname)
{
    if (!soundname || id <= se_zero_invalid || id >= SIZE(affiliation))
        return 0;

    if (!affiliation[id]) {
        affiliation[id] = id;
        soundstring[id] = [NSString stringWithUTF8String:soundname];
        seSound[id] = [NSSound soundNamed:soundstring[id]];
    }
    return 1;
}
/* end of macsound.m */
