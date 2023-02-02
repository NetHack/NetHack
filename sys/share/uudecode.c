/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
 * Modified 12 April 1990 by Mark Adler for use on MSDOS systems with
 * Microsoft C and Turbo C.
 *
 * Modifed 13 February 1991 by Greg Roelofs for use on VMS systems.  As
 * with the MS-DOS version, the setting of the file mode has been disabled.
 * Compile and link normally (but note that the shared-image link option
 * produces a binary only 6 blocks long, as opposed to the 137-block one
 * produced by an ordinary link).  To set up the VMS symbol to run the
 * program ("run uudecode filename" won't work), do:
 *              uudecode :== "$disk:[directory]uudecode.exe"
 * and don't forget the leading "$" or it still won't work.  The binaries
 * produced by this program are in VMS "stream-LF" format; this makes no
 * difference to VMS when running decoded executables, nor to VMS unzip,
 * but other programs such as zoo or arc may or may not require the file
 * to be "BILFed" (or "unBILFed" or whatever).  Also, unlike the other
 * flavors, VMS files don't get overwritten (a higher version is created).
 *
 * Modified 13 April 1991 by Gary Mussar to be forgiving of systems that
 * appear to be stripping trailing blanks.
 *
 * Modified 28 February 2002 for use on WIN32 systems with Microsoft C.
 *
 * Modified 08 July 2006 to cast strlen() result to int to suppress a
 * warning on platforms where size_t > sizeof(int).
 *
 * $NHDT-Date: 1432512787 2015/05/25 00:13:07 $  $NHDT-Branch: master $:$NHDT-Revision: 1.7 $
 */

#ifndef lint
/* static char sccsid[] = "@(#)uudecode.c 5.5 (Berkeley) 7/6/88"; */
#endif /* not lint */

#ifdef __MSDOS__ /* For Turbo C */
#define MSDOS 1
#endif

#ifdef _WIN32
#undef MSDOS
#undef __MSDOS__
#ifndef WIN32
#define WIN32
#endif
#endif

#if __APPLE__
#include "config.h"
#endif

/*
 * uudecode [input]
 *
 * create the specified file, decoding as you go.
 * used with uuencode.
 */
#include <stdio.h>

#ifdef VMS
#include <types.h>
#include <stat.h>
#else
#if !defined(MSDOS) && !defined(WIN32)
#include <pwd.h>
#endif
#include <sys/types.h> /* MSDOS, WIN32, or UNIX */
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#endif

#include "warnings.h"

static void decode(FILE *, FILE *);
static void outdec(char *, FILE *, int);

/* single-character decode */
#define DEC(c) (((c) - ' ') & 077)

DISABLE_WARNING_UNREACHABLE_CODE

int
main(int argc, char **argv)
{
    FILE *in, *out;
    int mode;
    char dest[128];
    char buf[80];

    /* optional input arg */
    if (argc > 1) {
        if ((in = fopen(argv[1], "r")) == NULL) {
            perror(argv[1]);
            exit(1);
        }
        argv++;
        argc--;
    } else
        in = stdin;

    if (argc != 1) {
        printf("Usage: uudecode [infile]\n");
        exit(2);
    }

    /* search for header line */
    for (;;) {
        if (fgets(buf, sizeof buf, in) == NULL) {
            fprintf(stderr, "No begin line\n");
            exit(3);
        }
        if (strncmp(buf, "begin ", 6) == 0)
            break;
    }
    (void) sscanf(buf, "begin %o %s", &mode, dest);

#if !defined(MSDOS) && !defined(VMS) && !defined(WIN32) && !defined(MACOS)
    /* handle ~user/file format */
    if (dest[0] == '~') {
        char *sl;
        struct passwd *getpwnam();
        struct passwd *user;
        char dnbuf[100], *strchr(), *strcat(), *strcpy();

        sl = strchr(dest, '/');
        if (sl == NULL) {
            fprintf(stderr, "Illegal ~user\n");
            exit(3);
        }
        *sl++ = 0;
        user = getpwnam(dest + 1);
        if (user == NULL) {
            fprintf(stderr, "No such user as %s\n", dest);
            exit(4);
        }
        strcpy(dnbuf, user->pw_dir);
        strcat(dnbuf, "/");
        strcat(dnbuf, sl);
        strcpy(dest, dnbuf);
    }
#endif /* !defined(MSDOS) && !defined(VMS) */

/* create output file */
#if defined(MSDOS) || defined(WIN32)
    out = fopen(dest, "wb"); /* Binary file */
#else
    out = fopen(dest, "w");
#endif
    if (out == NULL) {
        perror(dest);
        exit(4);
    }
#if !defined(MSDOS) && !defined(VMS) && !defined(WIN32) /* i.e., UNIX */
    chmod(dest, mode);
#endif

    decode(in, out);

    if (fgets(buf, sizeof buf, in) == NULL || strcmp(buf, "end\n")) {
        fprintf(stderr, "No end line\n");
        exit(5);
    }
    exit(0);
    /*NOTREACHED*/
    return 0;
}

RESTORE_WARNINGS

/*
 * copy from in to out, decoding as you go along.
 */
void
decode(FILE *in, FILE *out)
{
    char buf[80];
    char *bp;
    int n, i, expected;

    for (;;) {
        /* for each input line */
        if (fgets(buf, sizeof buf, in) == NULL) {
            printf("Short file\n");
            exit(10);
        }
        n = DEC(buf[0]);
        if ((n <= 0) || (buf[0] == '\n'))
            break;

        /* Calculate expected # of chars and pad if necessary */
        expected = ((n + 2) / 3) << 2;
        for (i = (int) strlen(buf) - 1; i <= expected; i++)
            buf[i] = ' ';

        bp = &buf[1];
        while (n > 0) {
            outdec(bp, out, n);
            bp += 4;
            n -= 3;
        }
    }
}

/*
 * output a group of 3 bytes (4 input characters).
 * the input chars are pointed to by p, they are to
 * be output to file f.  n is used to tell us not to
 * output all of them at the end of the file.
 */
void
outdec(char *p, FILE *f, int n)
{
    int c1, c2, c3;

    c1 = DEC(*p) << 2 | DEC(p[1]) >> 4;
    c2 = DEC(p[1]) << 4 | DEC(p[2]) >> 2;
    c3 = DEC(p[2]) << 6 | DEC(p[3]);
    if (n >= 1)
        putc(c1, f);
    if (n >= 2)
        putc(c2, f);
    if (n >= 3)
        putc(c3, f);
}

#if !defined(MSDOS) && !defined(VMS) && !defined(WIN32) && !defined(MACOS)
/*
 * Return the ptr in sp at which the character c appears;
 * NULL if not found
 */

#ifndef NULL
#define NULL 0
#endif

char *
index(sp, c)
register char *sp, c;
{
    do {
        if (*sp == c)
            return (sp);
    } while (*sp++);
    return (NULL);
}
#endif
