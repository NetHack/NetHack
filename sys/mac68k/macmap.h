/* NetHack 5.0	macmap.h	*/
/* Copyright (c) Ingo Paschke, 2026. */
/* NetHack may be freely redistributed.  See license for details. */
/* macmap.h: separate map window for the Mac 68k port. */
#ifndef MACMAP_H
#define MACMAP_H

#include "macwin.h"

/* Lifecycle. */
extern Boolean macmap_create(NhWindow *map);
extern void    macmap_finalize(NhWindow *map);
extern void    macmap_destroy(NhWindow *map);

/* Mode control. */
extern Boolean macmap_set_mode(NhWindow *map, Boolean tile_mode);
extern Boolean macmap_get_mode(NhWindow *map);

/* NetHack windowprocs entry points. */
extern void    macmap_print_glyph(NhWindow *map, int x, int y,
                                  const glyph_info *gi);
extern void    macmap_clear(NhWindow *map);
extern void    macmap_cliparound(NhWindow *map, int x, int y);
extern void    macmap_curs(NhWindow *map, int x, int y);

/* Blit the pending dirty region to the window once (per-frame flush boundary). */
extern void    macmap_flush(void);

/* Map a core tileidx into the PICT sheet (statue slots fold to the one
   generic statue tile); shared with macwin.c's menu tiles. */
extern short   remap_tile_idx(int idx);

/* Mac event callbacks. */
extern void    macmap_update_event(NhWindow *map);
extern void    macmap_grow_event(NhWindow *map, long newSize);
extern void    macmap_fit(short avail_w, short avail_h, Boolean honor_saved);
extern Boolean macmap_click(NhWindow *map, Point pt, UInt32 modifiers);

/* Viewport queries. */
extern void    macmap_pixel_to_cell(NhWindow *map, Point pt,
                                    int *col, int *row);

/* Overview windoid: the whole level, a few pixels per cell.
   Show/hide are pure UI; persisting UiPrefs.overview_open is the
   caller's job (Game menu toggle / close box). */
extern void    macmap_overview_show(void);
extern void    macmap_overview_hide(void);
extern Boolean macmap_overview_visible(void);
extern WindowPtr macmap_overview_window(void); /* NULL until first shown */
/* content size in pixels; both are valid before the first show */
extern short   macmap_overview_width(void);
extern short   macmap_overview_height(void);
extern void    macmap_overview_update_event(WindowPtr w);
extern void    macmap_overview_float(void); /* keep it the frontmost window */

/* WRefCon sentinels for identifying our windows in event dispatch. */
#define MACMAP_REFCON  ((long) 0x4E486D70)  /* 'NHmp' */
#define MACOVERVIEW_REFCON ((long) 0x4E486F76)  /* 'NHov' */

#endif /* MACMAP_H */
