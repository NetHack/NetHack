/* NetHack 3.6	macsnd.c	$NHDT-Date: 1432512798 2015/05/25 00:13:18 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $ */
/* 	Copyright (c) 1992 by Jon Watte */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file contains music playing code.
 *
 * If we were REALLY determinated, we would make the sound play
 * asynchronously, but I'll save that one for a rainy day...
 *
 * This may break A/UX, since it defines MAC but we need to
 * check that the toolbox is booted. I'll defer that one too.
 *
 * - h+ 921128
 */

#include "hack.h"
#include "mactty.h"
#include "macwin.h"
#if 1 /*!TARGET_API_MAC_CARBON*/
#include <Sound.h>
#include <Resources.h>
#endif

#ifndef freqDurationCmd
#define freqDurationCmd 40
#endif

#define SND_BUFFER(s) (&(*s)[20])
#define SND_LEN(s) (GetHandleSize(s) - 42)

void
mac_speaker(struct obj *instr, char *melody)
{
    SndChannelPtr theChannel = (SndChannelPtr) 0;
    SndCommand theCmd;
    Handle theSound;
    unsigned char theName[32];
    char *n = (char *) &theName[1];
    int typ = instr->otyp;
    const char *actualn = OBJ_NAME(objects[typ]);

    /*
     * First: are we in the library ?
     */
    if (flags.silent) {
        return;
    }

    /*
     * Is this a known instrument ?
     */
    strcpy(n, actualn);
    theName[0] = strlen(n);
    theSound = GetNamedResource('snd ', theName);
    if (!theSound) {
        return;
    }
    HLock(theSound);

    /*
     * Set up the synth
     */
    if (SndNewChannel(&theChannel, sampledSynth, initMono + initNoInterp,
                      (void *) 0) == noErr) {
        char midi_note[] = { 57, 59, 60, 62, 64, 65, 67 };

        short err;
        short snd_len = SND_LEN(theSound) / 18;

        theCmd.cmd = soundCmd;
        theCmd.param1 = 0;
        theCmd.param2 = (long) SND_BUFFER(theSound);
        err = SndDoCommand(theChannel, &theCmd, false);

        /*
         * We rack 'em up all in a row
         * The mac will play them correctly and then end, since
         * we do a sync close below.
         *
         */
        while (*melody && !err) {
            while (*melody > 'G') {
                *melody -= 8;
            }
            while (*melody < 'A') {
                *melody += 8;
            }
            theCmd.cmd = freqDurationCmd;
            theCmd.param1 = snd_len;
            theCmd.param2 = midi_note[*melody - 'A'];
            err = SndDoCommand(theChannel, &theCmd, false);
            melody++;
        }
        SndDisposeChannel(theChannel, false); /* Sync wait for completion */
        ReleaseResource(theSound);
    }
}

void
tty_nhbell(void)
{
    Handle h = GetNamedResource('snd ', "\pNetHack Bell");

    if (h) {
        HLock(h);
        SndPlay((SndChannelPtr) 0, (SndListHandle) h, 0);
        ReleaseResource(h);
    } else
        SysBeep(30);
}
