/* NetHack 3.6	mhinput.h	$NHDT-Date: 1432512801 2015/05/25 00:13:21 $  $NHDT-Branch: master $:$NHDT-Revision: 1.11 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINInput_h
#define MSWINInput_h

/* nethack input queue - store/extract input events */
#include "winMS.h"

#define NHEVENT_CHAR 1
#define NHEVENT_MOUSE 2
typedef struct mswin_event {
    int type;
    union {
        struct {
            int ch;
        } kbd;

        struct {
            int mod;
            int x, y;
        } ms;
    };
} MSNHEvent, *PMSNHEvent;

#define NHEVENT_KBD(c)         \
    {                          \
        MSNHEvent e;           \
        e.type = NHEVENT_CHAR; \
        e.kbd.ch = (c);        \
        mswin_input_push(&e);  \
    }
#define NHEVENT_MS(_mod, _x, _y) \
    {                            \
        MSNHEvent e;             \
        e.type = NHEVENT_MOUSE;  \
        e.ms.mod = (_mod);       \
        e.ms.x = (_x);           \
        e.ms.y = (_y);           \
        mswin_input_push(&e);    \
    }

void mswin_nh_input_init();
int mswin_have_input();
void mswin_input_push(PMSNHEvent event);
PMSNHEvent mswin_input_pop();
PMSNHEvent mswin_input_peek();

#endif /* MSWINInput_h */
