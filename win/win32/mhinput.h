/* NetHack 3.7	mhinput.h	$NHDT-Date: 1596498351 2020/08/03 23:45:51 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.11 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MSWINInput_h
#define MSWINInput_h

/* nethack input queue - store/extract input events */
#include "winMS.h"

#define NHEVENT_CHAR 1
#define NHEVENT_MOUSE 2

union event_innards {
    struct {
        int ch;
    } kbd;

    struct {
       int mod;
       int x, y;
    } ms;
};

typedef struct mswin_event {
    int type;
    union event_innards ei;
} MSNHEvent, *PMSNHEvent;

#define NHEVENT_KBD(c)         \
    {                          \
        MSNHEvent e;           \
        e.type = NHEVENT_CHAR; \
        e.ei.kbd.ch = (c);        \
        mswin_input_push(&e);  \
    }
#define NHEVENT_MS(_mod, _x, _y) \
    {                            \
        MSNHEvent e;             \
        e.type = NHEVENT_MOUSE;  \
        e.ei.ms.mod = (_mod);       \
        e.ei.ms.x = (_x);           \
        e.ei.ms.y = (_y);           \
        mswin_input_push(&e);    \
    }

void mswin_nh_input_init(void);
int mswin_have_input(void);
void mswin_input_push(PMSNHEvent event);
PMSNHEvent mswin_input_pop(void);
PMSNHEvent mswin_input_peek(void);

#endif /* MSWINInput_h */
