/* NetHack 5.0	mac68ksound.c	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* Copyright (c) NetHack Mac 68k port, 2026. */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Classic Mac OS Sound Manager soundlib for the NetHack 5 sound
 * interface (sndprocs.h, doc/sound.txt).  Used by the 68k/PPC classic
 * builds (sys/mac68k); the Cocoa sound/macsound is the modern macOS
 * equivalent.
 *
 * Sounds are named 'snd ' resources looked up in any open resource
 * file; the packaging step converts the uuencoded wavs in sound/wav
 * with sys/mac68k/tools/wav2snd.py and merges them into the
 * application's resource fork, named after the wav basename
 * ("sound_Bugle_A", "se_squeak_D_flat", "sa2_xplevelup", ...).
 * Missing resources are silently skipped, so sound coverage grows
 * with whatever sound/wav ships.
 */

#include "hack.h"
#include <Sound.h>
#include <Resources.h>

#ifdef SND_LIB_MAC68KSOUND

/* macwin.h pulls in the world; declare what we need (sys/mac68k/macwin.c) */
extern void C2P(const char *c, unsigned char *p);

static void mac68ksound_init_nhsound(void);
static void mac68ksound_exit_nhsound(const char *);
static void mac68ksound_achievement(schar, schar, int32_t);
static void mac68ksound_soundeffect(char *, int32_t, int32_t);
static void mac68ksound_hero_playnotes(int32_t, const char *, int32_t);
static void mac68ksound_play_usersound(char *, int32_t, int32_t);
static void mac68ksound_ambience(int32_t, int32_t, int32_t);
static void mac68ksound_verbal(char *, int32_t, int32_t, int32_t, int32_t);

static void mac68ksound_stop(void);
static void mac68ksound_play_named(const char *cname);
static void mac68ksound_play_named_sync(const char *cname);

struct sound_procs mac68ksound_procs = {
    SOUNDID(mac68ksound),
    SOUND_TRIGGER_SOUNDEFFECTS | SOUND_TRIGGER_HEROMUSIC
        | SOUND_TRIGGER_ACHIEVEMENTS,
    mac68ksound_init_nhsound,
    mac68ksound_exit_nhsound,
    mac68ksound_achievement,
    mac68ksound_soundeffect,
    mac68ksound_hero_playnotes,
    mac68ksound_play_usersound,
    mac68ksound_ambience,
    mac68ksound_verbal,
};

/* one channel; the previous sound (and its locked resource) stays
   alive until the next play request or exit */
static SndChannelPtr snd_chan = (SndChannelPtr) 0;
static Handle snd_res = (Handle) 0;

static void
mac68ksound_stop(void)
{
    if (snd_chan) {
        (void) SndDisposeChannel(snd_chan, true); /* quietNow */
        snd_chan = (SndChannelPtr) 0;
    }
    if (snd_res) {
        HUnlock(snd_res);
        ReleaseResource(snd_res);
        snd_res = (Handle) 0;
    }
}

static void
mac68ksound_play_named(const char *cname)
{
    Str255 pname;
    Handle h;

    mac68ksound_stop();

    C2P(cname, pname);
    h = GetNamedResource('snd ', pname);
    if (!h)
        return; /* no such sound installed; stay silent */

    HLock(h);
    if (SndNewChannel(&snd_chan, 0, 0L, (SndCallBackUPP) 0) == noErr
        && SndPlay(snd_chan, (SndListHandle) h, true) == noErr) {
        snd_res = h; /* released by the next mac68ksound_stop() */
    } else {
        mac68ksound_stop();
        HUnlock(h);
        ReleaseResource(h);
    }
}

/* blocking play, for all but the last note of a melody; SndPlay with
   a NULL channel allocates its own and plays synchronously */
static void
mac68ksound_play_named_sync(const char *cname)
{
    Str255 pname;
    Handle h;

    mac68ksound_stop();

    C2P(cname, pname);
    h = GetNamedResource('snd ', pname);
    if (!h)
        return;

    HLock(h);
    (void) SndPlay((SndChannelPtr) 0, (SndListHandle) h, false);
    HUnlock(h);
    ReleaseResource(h);
}

static void
mac68ksound_init_nhsound(void)
{
    /* channel is created lazily per play */
}

static void
mac68ksound_exit_nhsound(const char *reason UNUSED)
{
    mac68ksound_stop();
}

static void
mac68ksound_achievement(schar ach1, schar ach2, int32_t repeat UNUSED)
{
    const char *resourcename = (const char *) 0;

    if (ach1 != 0) /* actual achievements: no sounds provided */
        return;

    switch ((int) ach2) {
    case sa2_splashscreen:
        resourcename = "sa2_splashscreen";
        break;
    case sa2_newgame_nosplash:
        resourcename = "sa2_newgame_nosplash";
        break;
    case sa2_restoregame:
        resourcename = "sa2_restoregame";
        break;
    case sa2_xplevelup:
        resourcename = "sa2_xplevelup";
        break;
    case sa2_xpleveldown:
        resourcename = "sa2_xpleveldown";
        break;
    }
    if (resourcename)
        mac68ksound_play_named(resourcename);
}

/* Core's SND_SOUNDEFFECTS_AUTOMAP name table requires USER_SOUNDS, so
   build the same seffects.h-generated table locally instead. */
struct se_automapping {
    enum sound_effect_entries seid;
    const char *basename;
};

#define SEFFECTS_AUTOMAP
static const struct se_automapping se_mappings[number_of_se_entries] = {
    { se_zero_invalid, "" },
#include "seffects.h"
};
#undef SEFFECTS_AUTOMAP

static void
mac68ksound_soundeffect(char *desc UNUSED, int32_t seid, int32_t volume UNUSED)
{
    char buf[80]; /* "se_" + longest basename fits comfortably */

    if (seid <= se_zero_invalid || seid >= number_of_se_entries
        || se_mappings[seid].seid != seid) /* table out of sequence */
        return;
    Snprintf(buf, sizeof buf, "se_%s", se_mappings[seid].basename);
    mac68ksound_play_named(buf);
}

/* sound/wav ships per-note samples (sound_<Instrument>_<A..G>) for the
   melodic instruments and a single sound_<Instrument> clip for the
   rest; same names and ins_ mapping as sound/macsound.  Melodies play
   each note synchronously except the last, which goes async. */
static void
mac68ksound_hero_playnotes(int32_t instrument, const char *str,
                           int32_t volume UNUSED)
{
    const char *base = (const char *) 0;
    boolean has_notes = FALSE;
    char resname[64];
    int i, notecount;

    switch (instrument) {
    case ins_flute: /* WOODEN_FLUTE */
        base = "sound_Wooden_Flute", has_notes = TRUE;
        break;
    case ins_pan_flute: /* MAGIC_FLUTE */
        base = "sound_Magic_Flute", has_notes = TRUE;
        break;
    case ins_english_horn: /* TOOLED_HORN */
        base = "sound_Tooled_Horn", has_notes = TRUE;
        break;
    case ins_french_horn: /* FROST_HORN */
        base = "sound_Frost_Horn";
        break;
    case ins_baritone_sax: /* FIRE_HORN */
        base = "sound_Fire_Horn";
        break;
    case ins_trumpet: /* BUGLE */
        base = "sound_Bugle", has_notes = TRUE;
        break;
    case ins_orchestral_harp: /* WOODEN_HARP */
        base = "sound_Wooden_Harp", has_notes = TRUE;
        break;
    case ins_cello: /* MAGIC_HARP */
        base = "sound_Magic_Harp", has_notes = TRUE;
        break;
    case ins_tinkle_bell: /* BELL, BELL_OF_OPENING */
        base = "sound_Bell";
        break;
    case ins_taiko_drum: /* DRUM_OF_EARTHQUAKE */
        base = "sound_Drum_Of_Earthquake";
        break;
    case ins_melodic_tom: /* LEATHER_DRUM */
        base = "sound_Leather_Drum";
        break;
    }
    if (!base)
        return;
    if (!has_notes || !str || !*str) {
        mac68ksound_play_named(base);
        return;
    }
    notecount = (int) strlen(str);
    for (i = 0; i < notecount; ++i) {
        if (str[i] < 'A' || str[i] > 'G')
            continue;
        Snprintf(resname, sizeof resname, "%s_%c", base, str[i]);
        if (i < notecount - 1)
            mac68ksound_play_named_sync(resname);
        else
            mac68ksound_play_named(resname); /* final note: async */
    }
}

static void
mac68ksound_play_usersound(char *filename UNUSED, int32_t volume UNUSED,
                           int32_t idx UNUSED)
{
}

static void
mac68ksound_ambience(int32_t ambience_action UNUSED, int32_t ambienceid UNUSED,
                     int32_t proximity UNUSED)
{
}

static void
mac68ksound_verbal(char *text UNUSED, int32_t gender UNUSED, int32_t tone UNUSED,
                   int32_t vol UNUSED, int32_t moreinfo UNUSED)
{
}

#endif /* SND_LIB_MAC68KSOUND */

/*mac68ksound.c*/
