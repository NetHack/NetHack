/* NetHack 3.7	mhinput.c	$NHDT-Date: 1596498350 2020/08/03 23:45:50 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.13 $ */
/* Copyright (C) 2001 by Alex Kompel */
/* NetHack may be freely redistributed.  See license for details. */

#include <assert.h>
#include "winMS.h"
#include "mhinput.h"

/* nethack input queue functions */

#define NH_INPUT_BUFFER_SIZE 64

/* as it stands right now we need only one slot
   since events are processed almost the same time as
   they occur but I like large round numbers */

static MSNHEvent nhi_input_buffer[NH_INPUT_BUFFER_SIZE];
static int nhi_init_input = 0;
static int nhi_read_pos = 0;
static int nhi_write_pos = 0;

/* initialize input queue */
void
mswin_nh_input_init(void)
{
    if (!nhi_init_input) {
        nhi_init_input = 1;

        ZeroMemory(nhi_input_buffer, sizeof(nhi_input_buffer));
        nhi_read_pos = 0;
        nhi_write_pos = 0;
    }
}

/* check for input */
int
mswin_have_input(void)
{
    return
#ifdef SAFERHANGUP
        /* we always have input (ESC) if hangup was requested */
        gp.program_state.done_hup ||
#endif
        (nhi_read_pos != nhi_write_pos);
}

/* add event to the queue */
void
mswin_input_push(PMSNHEvent event)
{
    int new_write_pos;

    if (!nhi_init_input)
        mswin_nh_input_init();

    new_write_pos = (nhi_write_pos + 1) % NH_INPUT_BUFFER_SIZE;

    if (new_write_pos != nhi_read_pos) {
        memcpy(nhi_input_buffer + nhi_write_pos, event, sizeof(*event));
        nhi_write_pos = new_write_pos;
    }
}

/* get event from the queue and delete it */
PMSNHEvent
mswin_input_pop(void)
{
    PMSNHEvent retval;

#ifdef SAFERHANGUP
    /* always return ESC when hangup was requested */
    if (gp.program_state.done_hup) {
        static MSNHEvent hangup_event;
        hangup_event.type = NHEVENT_CHAR;
        hangup_event.ei.kbd.ch = '\033';
        return &hangup_event;
    }
#endif

    if (!nhi_init_input)
        mswin_nh_input_init();

    if (nhi_read_pos != nhi_write_pos) {
        retval = &nhi_input_buffer[nhi_read_pos];
        nhi_read_pos = (nhi_read_pos + 1) % NH_INPUT_BUFFER_SIZE;
    } else {
        retval = NULL;
    }

    return retval;
}

/* get event from the queue but leave it there */
PMSNHEvent
mswin_input_peek(void)
{
    PMSNHEvent retval;

#ifdef SAFERHANGUP
    /* always return ESC when hangup was requested */
    if (gp.program_state.done_hup) {
        static MSNHEvent hangup_event;
        hangup_event.type = NHEVENT_CHAR;
        hangup_event.ei.kbd.ch = '\033';
        return &hangup_event;
    }
#endif

    if (!nhi_init_input)
        mswin_nh_input_init();

    if (nhi_read_pos != nhi_write_pos) {
        retval = &nhi_input_buffer[nhi_read_pos];
    } else {
        retval = NULL;
    }
    return retval;
}
