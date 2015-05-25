/* NetHack 3.6	beconf.h	$NHDT-Date: 1432512783 2015/05/25 00:13:03 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/* Copyright (c) Dean Luick 1996.	*/
/* NetHack may be freely redistributed.  See license for details. */

/* Configuration for Be Inc.'s BeOS */

#ifndef BECONF_H
#define BECONF_H

/*
 * We must use UNWIDENED_PROTOTYPES because we mix C++ and C.
 */

#define index strchr
#define rindex strrchr
#define Rand rand /* Be should have a better rand function! */
#define tgetch getchar
#define FCMASK 0666
#define PORT_ID "BeOS"
#define TEXTCOLOR
#define POSIX_TYPES
#define SIG_RET_TYPE __signal_func_ptr

#include <time.h>   /* for time_t */
#include <unistd.h> /* for lseek() */

/* could go in extern.h, under bemain.c (or something..) */
void regularize(char *);

/* instead of including system.h... */
#include <string.h>
#include <stdlib.h>
#include <termcap.h>

#endif /* BECONF_H */
