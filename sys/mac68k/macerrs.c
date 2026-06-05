/* NetHack 5.0	macerrs.c	*/
/* Copyright (c) Michael Hamel, 1991. */
/*-Copyright (c) Ingo Paschke, 2026. */
/* NetHack may be freely redistributed.  See license for details. */
/* Rewritten for the Retro68 revival: the original displayed errors via
   an ALRT resource, which the fatal-error path cannot rely on. */

#include "hack.h"
#include "macwin.h"
#include <Dialogs.h>
#include <TextUtils.h>
#include <Resources.h>

/* Fatal-error display.  Deliberately a bare dBoxProc window with DrawText
   rather than Alert(): error() can fire before resource setup completes,
   or when the resource fork itself is the problem, so it must not depend
   on an ALRT template being loadable. */
void
error(const char *format, ...)
{
    char cbuf[512];
    int len;
    va_list ap;

    va_start(ap, format);
    vsnprintf(cbuf, sizeof cbuf, format, ap);
    va_end(ap);
    len = strlen(cbuf);

    /* Show error and wait for click before exiting */
    {
        WindowPtr w;
        Rect r = {80, 40, 200, 472};

        w = NewWindow(NULL, &r, P_STRING_CONV("NetHack Error"), true,
                      dBoxProc, (WindowPtr)-1, false, 0);
        if (w) {
            SetPortWindowPort(w);
            MoveTo(10, 30);
            if (len > 0)
                DrawText(cbuf, 0, len);
            MoveTo(10, 60);
            DrawString(P_STRING_CONV("Click to exit."));
            while (!Button())
                ;
            DisposeWindow(w);
        }
    }
    ExitToShell();
}
