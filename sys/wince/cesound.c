/* NetHack 3.6	cesound.c	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.6	cesound.c	$Date: 2009/05/06 10:52:00 $  $Revision: 1.4 $ */
/*   SCCS Id: @(#)cesound.c   3.5     $NHDT-Date$                        */
/*   SCCS Id: @(#)cesound.c   3.5     $Date: 2009/05/06 10:52:00 $                        */
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

void play_usersound(filename, volume)
const char* filename;
int volume;
{
	TCHAR wbuf[MAX_PATH+1];
/*    pline("play_usersound: %s (%d).", filename, volume); */
	ZeroMemory(wbuf, sizeof(wbuf));
	(void)sndPlaySound(NH_A2W(filename, wbuf, MAX_PATH), SND_ASYNC | SND_NODEFAULT);
}

#endif /*USER_SOUNDS*/
/* cesound.c */
