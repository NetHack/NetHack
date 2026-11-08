/*
 * $NHDT-Header: NetHack 5.0 tiletxt.c 2022/03/13_15:06:45 NetHack-5.0:1.0 $
 * Copyright (c) Michael Allison 2022.
 * NetHack may be freely redistributed.  See license for details.
 */

/* alternate compilation for tilemap.c to create tiletxt.o
   that does not rely on "cc -c -o tiletxt.o tilemap.c"
   since many pre-POSIX compilers did not support that */
#define TILETEXT
#include "tilemap.c"
/*tiletxt.c*/
