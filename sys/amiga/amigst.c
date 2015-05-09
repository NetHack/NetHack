/* NetHack 3.6	amigst.c	$NHDT-Date: 1431192783 2015/05/09 17:33:03 $  $NHDT-Branch: master $:$NHDT-Revision: 1.6 $ */
/* NetHack 3.6	amigst.c	$Date: 2009/05/06 10:48:29 $  $Revision: 1.3 $ */
/*    SCCS Id: @(#)amigst.c     3.5    1993/01/08
/*    Copyright (c) Gregg Wonderly, Naperville, IL, 1992, 1993	  */
/* NetHack may be freely redistributed.  See license for details. */

#include <stdio.h>
#include <exec/types.h>
#include <exec/io.h>
#include <exec/alerts.h>
#include <exec/devices.h>
#include <devices/console.h>
#include <devices/conunit.h>
#include <graphics/gfxbase.h>
#include <intuition/intuition.h>
#include <libraries/dosextens.h>
#include <ctype.h>
#undef strcmpi
#include <string.h>
#include <errno.h>

#ifdef __SASC
#include <dos.h> /* for __emit */
#include <string.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/diskfont.h>
#include <proto/console.h>
#endif

#include "hack.h"
#include "winprocs.h"
#include "winami.h"

#ifdef AZTEC
#include <functions.h>
#endif

#include "NH:sys/amiga/winami.p"
#include "NH:sys/amiga/amiwind.p"
#include "NH:sys/amiga/amidos.p"

/* end amigst.c */
