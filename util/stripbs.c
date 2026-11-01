/* NetHack 5.0  stripbs.c                                         */
/* Copyright (c) Michael Allison, 2025.                                */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * a simple filter to strip character-backspace-character
 * from stdin and write the final character to stdout.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if defined(__GNUC__)
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

int main(int argc UNUSED, char *argv[] UNUSED)
{
    int stop = 0, trouble = 0;
    char buf[2];
    char *cp = &buf[0], *prev = &buf[1];

    *prev = 0;
    while (!stop) {
        if ((fread(buf, 1, 1, stdin)) > 0) {
            if (*cp == 8) {
                *prev = 0;
            } else {
                if (*prev)
                    fputc(*prev, stdout);
                *prev = *cp;
            }
        } else {
            if (errno != EOF)
                trouble = 1;
            if (*prev)
                fputc(*prev, stdout);
            stop = 1;
        }
    }
    fflush(stdout);
    fclose(stdout);
    return trouble ? EXIT_FAILURE : EXIT_SUCCESS;
}
