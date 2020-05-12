/* NetHack 3.6	cesound.c	$NHDT-Date: 1432512799 2015/05/25 00:13:19 $  $NHDT-Branch: master $:$NHDT-Revision: 1.13 $ */
/*   Copyright (c) NetHack PC Development Team 1993                 */
/*   NetHack may be freely redistributed.  See license for details. */
/*                                                                  */
/*
 * cesound.c - Windows CE NetHack sound support
 *
 *
 */

#include "hack.h"
#include <mmsystem.h>

#ifdef USER_SOUNDS

void
play_usersound(filename, volume)
const char *filename;
int volume;
{
    TCHAR wbuf[MAX_PATH + 1];
    /*    pline("play_usersound: %s (%d).", filename, volume); */
    ZeroMemory(wbuf, sizeof(wbuf));
    (void) sndPlaySound(NH_A2W(filename, wbuf, MAX_PATH),
                        SND_ASYNC | SND_NODEFAULT);
}

#endif /*USER_SOUNDS*/
/* cesound.c */
