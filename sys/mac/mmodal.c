/*	SCCS Id: @(#)mmodal.c	3.1	93/01/24		  */
/* Copyright (c) Jon W{tte, Hao-Yang Wang, Jonathan Handler 1992. */
/* NetHack may be freely redistributed.  See license for details. */

#if !TARGET_API_MAC_CARBON
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


