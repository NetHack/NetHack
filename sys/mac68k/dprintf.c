/* NetHack 5.0	dprintf.c	$NHDT-Date: 1432512798 2015/05/25 00:13:18 $  $NHDT-Branch: master $:$NHDT-Revision: 1.7 $ */
/* Copyright (c) Jon W{tte, 1993.				  */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "macwin.h"

static Boolean
KeyDown(unsigned short code)
{
    unsigned char keys[16];

    GetKeys((void *) keys);
    return ((keys[code >> 3] >> (code & 7)) & 1) != 0;
}

enum { DO_NOTHING, DO_DEBUGSTR, DO_PLINE };

/* Debug diagnostics sink, active only in wizard mode (flags.debug) so a
   release game never creates files or interrupts play.  In wizard mode every
   message is appended to dprintf.log in the game folder; in addition, with
   Caps Lock down it is sent to the debugger (DebugStr), or with Control down
   pline()d into the message window. */
void
mac_dprintf(char *format, ...)
{
    static FILE *log_fp = NULL;
    static int   log_tried = 0;
    char buffer[512];
    size_t plen;
    va_list list;
    int doit;

    if (!flags.debug)
        return;

    /* open lazily once, give up silently on failure */
    if (!log_tried) {
        log_tried = 1;
        log_fp = fopen("dprintf.log", "w");
    }
    if (log_fp) {
        va_start(list, format);
        vsnprintf(buffer, sizeof buffer, format, list);
        va_end(list);
        fputs(buffer, log_fp);
        if (buffer[0] && buffer[strlen(buffer) - 1] != '\n')
            fputc('\n', log_fp);
        fflush(log_fp);
    }

    doit = DO_NOTHING;
    if (macFlags.hasDebugger && KeyDown(0x39)) { /* Caps Lock */
        doit = DO_DEBUGSTR;
    } else if (KeyDown(0x3B) && iflags.window_inited && /* Control */
               (WIN_MESSAGE != -1)
               && theWindows[WIN_MESSAGE].its_window) {
        doit = DO_PLINE;
    }

    if (doit != DO_NOTHING) {
        va_start(list, format);
        vsnprintf(&buffer[1], sizeof buffer - 1, format, list);
        va_end(list);

        if (doit == DO_DEBUGSTR) {
            plen = strlen(&buffer[1]);
            if (plen > 255) /* Str255 length byte */
                plen = 255;
            buffer[0] = (char) plen;
            DebugStr((uchar *) buffer);
        } else if (doit == DO_PLINE)
            pline("%s", &buffer[1]);
    }
}
