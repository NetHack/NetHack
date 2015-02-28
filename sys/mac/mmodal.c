/* NetHack 3.5	mmodal.c	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.5	mmodal.c	$Date: 2009/05/06 10:49:18 $  $Revision: 1.7 $ */
/*	SCCS Id: @(#)mmodal.c	3.5	1993/01/24		  */
/* Copyright (c) Jon W{tte, Hao-Yang Wang, Jonathan Handler 1992. */
/* NetHack may be freely redistributed.  See license for details. */

#if 1 /*!TARGET_API_MAC_CARBON*/
# include <Dialogs.h>
# include <ControlDefinitions.h>
#else
# include <Carbon/Carbon.h>
#endif

#include "macpopup.h"

/* Flash a dialog button when its accelerator key is pressed */
void
FlashButton(DialogRef wind, short item) {
	short type;
	Handle handle;
	Rect rect;
	unsigned long ticks;

	/* Apple recommends 8 ticks */
	GetDialogItem(wind, item, &type, &handle, &rect);
	HiliteControl((ControlHandle)handle, kControlButtonPart);
	Delay(8, &ticks);
	HiliteControl((ControlHandle)handle, 0);
	return;
}


