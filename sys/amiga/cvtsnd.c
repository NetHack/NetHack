/* NetHack 3.6	cvtsnd.c	$NHDT-Date: 1432512794 2015/05/25 00:13:14 $  $NHDT-Branch: master $:$NHDT-Revision: 1.7 $ */
/* 	Copyright (c) 1995, Andrew Church, Olney, Maryland        */
/* NetHack may be freely redistributed.  See license for details. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    short namelen;
    char name[62];
    char misc[64]; /* rest of MacBinary header */
    long FORM;
    long flen;
    long AIFF;
    long SSND;
    long sndlen;
} AIFF;

typedef struct {
    char FORM[4];
    long flen;
    char _8SVX[4];
    char VHDR[4];
    long vhlen;
    long oneshot, repeat;
    long samples; /* 'samplesPerHiCycle' in the docs - usually 32, so
                   *    we'll use that */
    short freq;
    char octaves, compress;
    long volume;
    char NAME[4];
    long nlen;     /* should be 64; see name[] comment */
    char name[64]; /* for simplicity, i.e. just fwrite() entiree header */
    char BODY[4];
    long blen;
} IFF;

main(int ac, char **av)
{
    FILE *in, *out;
    AIFF aiff;
    IFF iff;
    static char buf[16384];
    long n, len;

    if (ac != 3) {
        fprintf(stderr, "Usage: %s input-file output-file\n", av[0]);
        exit(20);
    }
    if (!(in = fopen(av[1], "r"))) {
        fprintf(stderr, "Can't open input file\n");
        exit(20);
    }
    if (!(out = fopen(av[2], "w"))) {
        fprintf(stderr, "Can't open output file\n");
        exit(20);
    }

    fread(&aiff, sizeof(aiff), 1, in);
    memcpy(iff.FORM, "FORM", 4);
    iff.flen = sizeof(iff) + aiff.sndlen - 8;
    memcpy(iff._8SVX, "8SVX", 4);
    memcpy(iff.VHDR, "VHDR", 4);
    iff.vhlen = 20;
    iff.oneshot = aiff.sndlen;
    iff.repeat = 0;
    iff.samples = 32;
    iff.freq = 22000;
    iff.octaves = 1;
    iff.compress = 0;
    iff.volume = 0x10000;
    memcpy(iff.NAME, "NAME", 4);
    iff.nlen = 64;
    strncpy(iff.name, aiff.name, 62);
    iff.name[aiff.namelen] = 0;
    memcpy(iff.BODY, "BODY", 4);
    iff.blen = aiff.sndlen;
    fwrite(&iff, sizeof(iff), 1, out);
    len = aiff.sndlen;
    do {
        if (len >= sizeof(buf))
            n = fread(buf, 1, sizeof(buf), in);
        else
            n = fread(buf, 1, len, in);
        if (n) {
            fwrite(buf, 1, n, out);
            len -= n;
        }
    } while (len && n);

    if (len)
        fprintf(stderr, "Warning: %ld bytes of sample missing\n", len);
    fclose(in);
    fclose(out);
    exit(0);
}
