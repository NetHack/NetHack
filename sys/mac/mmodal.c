/* NetHack 5.0	mmodal.c	$NHDT-Date: 1432512797 2015/05/25 00:13:17 $  $NHDT-Branch: master $:$NHDT-Revision: 1.11 $ */
/* Copyright (c) Jon W{tte, Hao-Yang Wang, Jonathan Handler 1992. */
/* NetHack may be freely redistributed.  See license for details. */

#include <Dialogs.h>
#ifndef CROSS_TO_MAC68K
#include <ControlDefinitions.h>
#endif


#ifndef kControlButtonPart
#define kControlButtonPart 10
#endif

/* Flash a dialog button when its accelerator key is pressed */
void
FlashButton(DialogRef wind, short item)
{
    short type;
    Handle handle;
    Rect rect;
    unsigned long ticks;

    /* Apple recommends 8 ticks */
    GetDialogItem(wind, item, &type, &handle, &rect);
    HiliteControl((ControlHandle) handle, kControlButtonPart);
    Delay(8, &ticks);
    HiliteControl((ControlHandle) handle, 0);
    return;
}
