/* NetHack 5.0	macstat.c	*/
/* NetHack may be freely redistributed.  See license for details. */

/* Native status renderer for the Mac 68k port.  Replaces the
 * genl_status_update -> putstr -> tty path so STATUS_HILITES per-field
 * colors and attributes can be drawn (spec:
 * docs/specs/2026-06-10-mac-status-hilites-design.md).
 * This file starts as a delegation skeleton; the renderer lands next. */

#include "hack.h"
#include "macwin.h"

/* genl field tables (src/windows.c); allocated by genl_status_init() */
extern const char *status_fieldfmt[MAXBLSTATS];
extern const char *status_fieldnm[MAXBLSTATS];
extern char *status_vals[MAXBLSTATS];
extern boolean status_activefields[MAXBLSTATS];

extern WindowPtr _mt_window; /* mttymain.c */

static Boolean stat_inited = false;

/* TRUE once mac_status_init has run and WIN_STATUS is bound to _mt_window;
   gates the update-event/flush rerouting in macwin.c */
Boolean
macstat_active(void)
{
    return stat_inited && WIN_STATUS >= 0 && WIN_STATUS < NUM_MACWINDOWS
           && theWindows
           && theWindows[WIN_STATUS].its_window == _mt_window;
}

void
mac_status_init(void)
{
    /* genl_status_init allocates the field tables and creates+displays
       WIN_STATUS (the mac create path binds it to _mt_window) */
    genl_status_init();
    stat_inited = true;
}

void
mac_status_finish(void)
{
    stat_inited = false;
    genl_status_finish(); /* frees status_vals[]; WIN_STATUS stays set but
                             stat_inited=false blocks macstat_active() */
}

void
mac_status_enablefield(
    int fieldidx, const char *nm, const char *fmt, boolean enable)
{
    genl_status_enablefield(fieldidx, nm, fmt, enable);
}

void
mac_status_update(
    int idx, genericptr_t ptr, int chg, int percent,
    int color, unsigned long *colormasks)
{
    genl_status_update(idx, ptr, chg, percent, color, colormasks);
}

void
macstat_redraw(void)
{
    /* no-op until the native renderer lands (Task 3) */
}
