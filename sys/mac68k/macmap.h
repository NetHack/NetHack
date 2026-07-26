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

/* Mac event callbacks. */
extern void    macmap_update_event(NhWindow *map);
extern void    macmap_grow_event(NhWindow *map, long newSize);
extern void    macmap_fit(short avail_w, short avail_h);
extern Boolean macmap_click(NhWindow *map, Point pt, UInt32 modifiers);

/* Viewport queries. */
extern void    macmap_pixel_to_cell(NhWindow *map, Point pt,
                                    int *col, int *row);

/* WRefCon sentinel for identifying the map window in event dispatch. */
#define MACMAP_REFCON  ((long) 0x4E486D70)  /* 'NHmp' */

#endif /* MACMAP_H */
