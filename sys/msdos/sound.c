/* NetHack 3.6	sound.c	$NHDT-Date: 1432512792 2015/05/25 00:13:12 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/*   Copyright (c) NetHack PC Development Team 1993,1995            */
/*   NetHack may be freely redistributed.  See license for details. */
/*                                                                  */
/*
 * sound.c - Hardware sound support
 *
 *Edit History:
 *     Initial Creation                              93/10/01
 *     Added PC Speaker Support for BC compilers     95/06/14
 *     Completed various fixes			     96/02/19
 *
 */

#include "hack.h"
#include <stdio.h>
#include "portio.h"

#include <dos.h>
#include <ctype.h>

#ifndef TESTING

#define printf pline

int
assign_soundcard(sopt)
char *sopt;
{
    iflags.hassound = 0;
#ifdef PCMUSIC
    iflags.usepcspeaker = 0;
#endif

    if (strncmpi(sopt, "def", 3) == 0) { /* default */
                                         /* do nothing - default */
    }
#ifdef PCMUSIC
    else if (strncmpi(sopt, "speaker", 7) == 0) { /* pc speaker */
        iflags.usepcspeaker = 1;
    }
#endif
    else if (strncmpi(sopt, "auto", 4) == 0) { /* autodetect */
                                               /*
                                                * Auto-detect Priorities (arbitrary for now):
                                                *	Just pcspeaker
                                                */
        if (0)
            ;
#ifdef PCMUSIC
        else
            iflags.usepcspeaker = 1;
#endif
    } else {
        return 0;
    }
    return 1;
}
#endif

#ifdef PCMUSIC

/* 8254/3 Control Word Defines */

#define CTR0SEL (0 << 6)
#define CTR1SEL (1 << 6)
#define CTR2SEL (2 << 6)
#define RDBACK (3 << 6)

#define LATCH (0 << 4)
#define RW_LSB (1 << 4)
#define RW_MSB (2 << 4) /* If both LSB and MSB are read, LSB is done first \
                           */

#define MODE0 (0 << 1) /* Interrupt on terminal count */
#define MODE1 (1 << 1) /* Hardware One-Shot */
#define MODE2 (2 << 1) /* Pulse Generator */
#define MODE3 (3 << 1) /* Square Wave Generator */
#define MODE4 (4 << 1) /* Software Triggered Strobe */
#define MODE5 (5 << 1) /* Hardware Triggered Strobe */

#define BINARY (0 << 0) /* Binary counter (16 bits) */
#define BCD (1 << 0)    /* Binary Coded Decimal (BCD) Counter (4 Decades) */

/* Misc 8254/3 Defines */

#define TIMRFRQ (1193180UL) /* Input frequency to the clock (Hz) */

/* Speaker Defines */

#define TIMER (1 << 0)   /* Timer 2 Output connected to Speaker */
#define SPKR_ON (1 << 1) /* Turn on/off Speaker */

/* Port Definitions */

/* 8254/3 Ports */

#define CTR0 0x40
#define CTR1 0x41
#define CTR2 0x42
#define CTRL 0x43

/* Speaker Port */

#define SPEAKER 0x61

void
startsound(unsigned freq)
{
    /* To start a sound on the PC:
     *
     * First, set the second counter to have the correct frequency:
     */

    unsigned count;

    if (freq == 0)
        freq = 523;

    count = TIMRFRQ / freq; /* Divide frequencies to get count. */

#ifdef TESTING
    printf("freq = %u, count = %u\n", freq, count);
#endif

    outportb(CTRL, CTR2SEL | RW_LSB | RW_MSB | MODE3 | BINARY);
    outportb(CTR2, count & 0x0FF);
    outportb(CTR2, count / 0x100);

    /* Next, turn on the speaker */

    outportb(SPEAKER, inportb(SPEAKER) | TIMER | SPKR_ON);
}

void
stopsound(void)
{
    outportb(SPEAKER, inportb(SPEAKER) & ~(TIMER | SPKR_ON));
}

static unsigned tempo, length, octave, mtype;

/* The important numbers here are 287700UL for the factors and 4050816000UL
 * which gives the 440 Hz for the A below middle C. "middle C" is assumed to
 * be the C at octave 3. The rest are computed by multiplication/division of
 * 2^(1/12) which came out to 1.05946 on my calculator.  It is assumed that
 * no one will request an octave beyond 6 or below 0.  (At octave 7, some
 * notes still come out ok, but by the end of the octave, the "notes" that
 * are produced are just ticks.

 * These numbers were chosen by a process based on the C64 tables (which
 * weren't standardized) and then were 'standardized' by giving them the
 * closest value.  That's why they don't seem to be based on any sensible
 * number.
 */

unsigned long notefactors[12] = { 483852, 456695, 431063, 406869,
                                  384033, 362479, 342135, 322932,
                                  304808, 287700, 271553, 256312 };

void
note(long notenum)
{
    startsound((unsigned) (4050816000UL / notefactors[notenum % 12]
                           >> (7 - notenum / 12)));
}

int notes[7] = { 9, 11, 0, 2, 4, 5, 7 };

char *
startnote(char *c)
{
    long n;

    n = notes[toupper(*c++) - 'A'] + octave * 12;
    if (*c == '#' || *c == '+') {
        n++;
        c++;
    } else if (*c == '-') {
        if (n)
            n--;
        c++;
    }

    note(n);

    return --c;
}

void
delaytime(unsigned time)
{
    /* time and twait are in units of milliseconds */

    unsigned twait;

    switch (toupper(mtype)) {
    case 'S':
        twait = time / 4;
        break;
    case 'L':
        twait = 0;
        break;
    default:
        twait = time / 8;
        break;
    }

    msleep(time - twait);
    stopsound();
    msleep(twait);
}

char *
delaynote(char *c)
{
    unsigned time = 0;

    while (isdigit(*c))
        time = time * 10 + (*c++ - '0');

    if (!time)
        time = length;

    time = (unsigned) (240000 / time / tempo);

    while (*c == '.') {
        time = time * 3 / 2;
        c++;
    }

    delaytime(time);

    return c;
}

void
initspeaker(void)
{
    tempo = 120, length = 4, octave = 3, mtype = 'N';
}

void
play(char *tune)
{
    char *c, *n;
    unsigned num;

    for (c = tune; *c;) {
        sscanf(c + 1, "%u", &num);
        for (n = c + 1; isdigit(*n); n++) /* do nothing */
            ;
        if (isspace(*c))
            c++;
        else
            switch (toupper(*c)) {
            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
            case 'G':
                c = startnote(c);
            case 'P':
                c = delaynote(++c);
                break;
#if 0
		case 'M': c++; mtype = *c; c++; break;
		case 'T':
		    if (num) tempo = num;
		    else printf ("Zero Tempo (%s)!\n", c);
		    c = n;
		    break;
		case 'L':
		    if (num) length = num;
		    else printf ("Zero Length (%s)!\n", c);
		    c = n;
		    break;
		case 'O':
		    if (num <= 7)
			octave = num;
		    c = n;
		    break;
		case 'N':
		    note (num);
		    delaytime ((240000/length/tempo));
		    c = n;
		    break;
		case '>': if (octave < 7) octave++; c++; break;
		case '<': if (octave) octave--; c++; break;
#endif
            case ' ':
                c++;
                break;
            default:
                printf("Unrecognized play value (%s)!\n", c);
                return;
            }
    }
}

#ifndef TESTING
void
pc_speaker(struct obj *instr, char *tune)
{
    if (!iflags.usepcspeaker)
        return;
    initspeaker();
    switch (instr->otyp) {
    case WOODEN_FLUTE:
    case MAGIC_FLUTE:
        octave = 5; /* up one octave */
        break;
    case TOOLED_HORN:
    case FROST_HORN:
    case FIRE_HORN:
        octave = 2; /* drop two octaves */
        break;
    case BUGLE:
        break;
    case WOODEN_HARP:
    case MAGIC_HARP:
        length = 8;
        mtype = 'L'; /* fast, legato */
        break;
    }
    play(tune);
}

#else

main()
{
    char s[80];
    int tool;

    initspeaker();
    printf("1) flute\n2) horn\n3) harp\n4) other\n");
    fgets(s, 80, stdin);
    sscanf(s, "%d", &tool);
    switch (tool) {
    case 1:
        octave = 5;
        break;
    case 2:
        octave = 2;
        break;
    case 3:
        length = 8;
        mtype = 'L';
        break;
    default:
        break;
    }
    printf("Enter tune:");
    fgets(s, 80, stdin);
    play(s);
}
#endif

#endif /* PCMUSIC */

/* sound.c */
