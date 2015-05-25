/* NetHack 3.6	amisnd.c	$NHDT-Date: 1432512796 2015/05/25 00:13:16 $  $NHDT-Branch: master $:$NHDT-Revision: 1.7 $ */
/* 	Copyright (c) 1992, 1993, 1995 by Gregg Wonderly */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file contains music playing code.
 *
 * If we were REALLY determined, we would make the sound play
 * asynchronously, but I'll save that one for a rainy day...
 */

#include "hack.h"
#include "dlb.h"

#undef red
#undef blue
#undef green
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <devices/audio.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <graphics/gfxbase.h>

#include <proto/exec.h>
#include <clib/alib_protos.h>
#include <proto/dos.h>
#include <proto/graphics.h>

#include <stdio.h>
#include <stdlib.h>

#define AMII_AVERAGE_VOLUME 60

int amii_volume = AMII_AVERAGE_VOLUME;

typedef struct VHDR {
    char name[4];
    long len;
    unsigned long oneshot, repeat, samples;
    UWORD freq;
    UBYTE n_octaves, compress;
    LONG volume;
} VHDR;

typedef struct IFFHEAD {
    char FORM[4];
    long flen;
    char _8SVX[4];
    VHDR vhdr;
    char NAME[4];
    long namelen;
} IFFHEAD;

extern struct GfxBase *GfxBase;

UBYTE whichannel[] = { 1, 2, 4, 8 };
void makesound(char *, char *, int vol);
void amii_speaker(struct obj *instr, char *melody, int vol);

/* A major scale in indexs to freqtab... */
int notetab[] = { 0, 2, 4, 5, 7, 9, 11, 12 };

/* Frequencies for a scale starting at one octave below 'middle C' */
long freqtab[] = {
    220, /*A */
    233, /*Bb*/
    246, /*B */
    261, /*C */
    277, /*Db*/
    293, /*D */
    311, /*Eb*/
    329, /*E */
    349, /*F */
    370, /*Gb*/
    392, /*G */
    415, /*Ab*/
    440, /*A */
};

#ifdef TESTING
main(argc, argv) int argc;
char **argv;
{
    makesound("wooden_flute", "AwBwCwDwEwFwGwawbwcwdwewfwgw", 60);
    makesound("wooden_flute", "AhBhChDhEhFhGhahbhchdhehfhgh", 60);
    makesound("wooden_flute", "AqBqCqDqEqFqGqaqbqcqdqeqfqgq", 60);
    makesound("wooden_flute", "AeBeCeDeEeFeGeaebecedeeefege", 60);
    makesound("wooden_flute", "AxBxCxDxExFxGxaxbxcxdxexfxgx", 60);
    makesound("wooden_flute", "AtBtCtDtEtFtGtatbtctdtetftgt", 60);
    makesound("wooden_flute", "AtBtCtDtEtFtGtatbtctdtetftgt", 60);
    makesound("wooden_flute", "AtBtCtDtEtFtGtatbtctdtetftgt", 60);
    makesound("wooden_flute", "AtBtCtDtEtFtGtatbtctdtetftgt", 60);
    makesound("wooden_flute", "AtBtCtDtEtFtGtatbtctdtetftgt", 60);
    makesound("wooden_flute", "AtBtCtDtEtFtGtatbtctdtetftgt", 60);
    makesound("wooden_flute", "AtBtCtDtEtFtGtatbtctdtetftgt", 60);
    makesound("wooden_flute", "AtBtCtDtEtFtGtatbtctdtetftgt", 60);
    makesound("wooden_flute", "AtBtCtDtEtFtGtatbtctdtetftgt", 60);
}
#else
void
amii_speaker(struct obj *instr, char *melody, int vol)
{
    int typ = instr->otyp;
    char *actualn = (char *) OBJ_NAME(objects[typ]);

    /* Make volume be relative to users volume level, with 60 being the
     * average level that will be passed to us.
     */
    vol = vol * amii_volume / AMII_AVERAGE_VOLUME;

    makesound(actualn, melody, vol);
}
#endif

void
makesound(char *actualn, char *melody, int vol)
{
    char *t;
    int c, cycles, dot, dlay;
    dlb *stream = 0;
    IFFHEAD iffhead;
    struct IOAudio *AudioIO = 0;
    struct MsgPort *AudioMP = 0;
    struct Message *AudioMSG = 0;
    ULONG device = -1;
    BYTE *waveptr = 0;
    LONG frequency = 440, duration = 1, clock, samp, samples, samcyc = 1;
    unsigned char name[100];

    if (flags.silent)
        return;

    if (GfxBase->DisplayFlags & PAL)
        clock = 3546895;
    else
        clock = 3579545;

    /*
     * Convert type to file name - if there's nothing to play we
     * shouldn't be here in the first place.
     */
    strncpy(name, actualn, sizeof(name));
    for (t = strchr(name, ' '); t; t = strchr(name, ' '))
        *t = '_';
    if ((stream = dlb_fopen(name, "r")) == NULL) {
        perror(name);
        return;
    }

    AudioIO = (struct IOAudio *) AllocMem(sizeof(struct IOAudio),
                                          MEMF_PUBLIC | MEMF_CLEAR);
    if (AudioIO == 0)
        goto killaudio;

    AudioMP = CreateMsgPort();
    if (AudioMP == 0)
        goto killaudio;

    AudioIO->ioa_Request.io_Message.mn_ReplyPort = AudioMP;
    AudioIO->ioa_Request.io_Message.mn_Node.ln_Pri = 0;
    AudioIO->ioa_Request.io_Command = ADCMD_ALLOCATE;
    AudioIO->ioa_Request.io_Flags = ADIOF_NOWAIT;
    AudioIO->ioa_AllocKey = 0;
    AudioIO->ioa_Data = whichannel;
    AudioIO->ioa_Length = sizeof(whichannel);

    device = OpenDevice(AUDIONAME, 0L, (struct IORequest *) AudioIO, 0L);
    if (device != 0)
        goto killaudio;

    if (dlb_fread((genericptr_t) &iffhead, sizeof(iffhead), 1, stream) != 1)
        goto killaudio;

    /* This is an even number of bytes long */
    if (dlb_fread(name, (iffhead.namelen + 1) & ~1, 1, stream) != 1)
        goto killaudio;

    if (dlb_fread((genericptr_t) &samples, 4, 1, stream) != 1)
        goto killaudio;

    if (dlb_fread((genericptr_t) &samples, 4, 1, stream) != 1)
        goto killaudio;

    waveptr = AllocMem(samples, MEMF_CHIP | MEMF_PUBLIC);
    if (!waveptr)
        goto killaudio;

    if (dlb_fread(waveptr, samples, 1, stream) != 1)
        goto killaudio;

    while (melody[0] && melody[1]) {
        c = *melody++;
        duration = *melody++;
        dot = 0;
        if (*melody == '.') {
            dot = 1;
            ++melody;
        }
        switch (duration) {
        case 'w':
            dlay = 3;
            duration = 1;
            cycles = 1;
            break;
        case 'h':
            dlay = 3;
            duration = 2;
            cycles = 1;
            break;
        case 'q':
            dlay = 2;
            duration = 4;
            cycles = 1;
            break;
        case 'e':
            dlay = 1;
            duration = 8;
            cycles = 1;
            break;
        case 'x':
            dlay = 0;
            duration = 16;
            cycles = 1;
            break;
        case 't':
            dlay = 0;
            duration = 32;
            cycles = 1;
            break;
        default:
            goto killaudio; /* unrecognized duration */
        }

        /* Lower case characters are one octave above upper case */
        switch (c) {
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
            c -= 'a' - 7;
            break;

        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
            c -= 'A';
            break;

        default:
            continue;
        }

        samcyc = samples;

        /* lowercase start at middle 'C', upper case are one octave below */
        frequency = c > 7 ? freqtab[notetab[c % 7]] * 2 : freqtab[notetab[c]];

        /* We can't actually do a dotted whole note unless we add code for a
         * real
         * 8SVX sample which includes sustain sample information to tell us
         * how
         * to hold the note steady...  So when duration == 1, ignore 'dot'...
         */
        if (dot && duration > 1)
            samp = ((samples / duration) * 3) / 2;
        else
            samp = samples / duration;

        /* Only use some of the samples based on frequency */
        samp = frequency * samp / 880;

        /* The 22khz samples are middle C samples, so adjust the play
         * back frequency accordingly
         */
        frequency = (frequency * (iffhead.vhdr.freq * 2) / 3) / 440L;

        AudioIO->ioa_Request.io_Message.mn_ReplyPort = AudioMP;
        AudioIO->ioa_Request.io_Command = CMD_WRITE;
        AudioIO->ioa_Request.io_Flags = ADIOF_PERVOL;
        AudioIO->ioa_Data = (BYTE *) waveptr;
        AudioIO->ioa_Length = samp;

        /* The clock rate represents the unity rate, so dividing by
         * the frequency gives us a period ratio...
         */
        /*printf( "clock: %ld, freq: %ld, div: %ld\n", clock, frequency,
         * clock/frequency );*/
        AudioIO->ioa_Period = clock / frequency;
        AudioIO->ioa_Volume = vol;
        AudioIO->ioa_Cycles = cycles;

        BeginIO((struct IORequest *) AudioIO);
        WaitPort(AudioMP);
        AudioMSG = GetMsg(AudioMP);
        if (dlay)
            Delay(dlay);
    }

killaudio:
    if (stream)
        dlb_fclose(stream);
    if (waveptr)
        FreeMem(waveptr, samples);
    if (device == 0)
        CloseDevice((struct IORequest *) AudioIO);
    if (AudioMP)
        DeleteMsgPort(AudioMP);
    if (AudioIO)
        FreeMem(AudioIO, sizeof(*AudioIO));
}
