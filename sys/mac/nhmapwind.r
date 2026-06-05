/* nhmapwind.r — WIND resources for the dedicated map window.
 * 200 = documentProc (resizable, for Quadra+ / large screens, small_screen=false).
 * 201 = plainDBox    (borderless, fixed, for SE/30 screens,  small_screen=true).
 *
 * boundsRect coordinates are top, left, bottom, right.
 * Width = 80 cols * 6 px = 480; height_text = 21 rows * 14 px = 294.
 *
 * WIND 200 content top is y=40: the documentProc title bar (~11px) sits
 * above the content rect, so the title bar top lands at ~y=29, clearing
 * the 20px menu bar.  Height preserved: 334-40 = 294.
 */

#include "Multiverse.r"

resource 'WIND' (200, "Dungeon Map (document)", purgeable) {
    {40, 0, 334, 480},          /* content top y=40: title bar (~y29-40) clears the 20px menu bar */
    documentProc,               /* WDEF procID = 0 (doc, with grow) */
    invisible,                  /* visible flag — we ShowWindow later */
    noGoAway,                   /* no close box — use File→Quit */
    0x0,                        /* refCon */
    "Dungeon Map",
    noAutoCenter
};

resource 'WIND' (201, "Dungeon Map (borderless)", purgeable) {
    {20, 16, 314, 496},         /* SE/30: 480x294 area starting at y=20 */
    plainDBox,                  /* WDEF procID = 2 (no chrome) */
    invisible,
    noGoAway,
    0x0,
    "",
    noAutoCenter
};

resource 'WIND' (210, "Status (borderless)", purgeable) {
    {40, 0, 80, 480},
    plainDBox,                  /* WDEF procID = 2 (no chrome) */
    invisible,
    noGoAway,
    0x0,
    "",
    noAutoCenter
};

resource 'WIND' (211, "Messages (borderless)", purgeable) {
    {40, 0, 120, 480},
    plainDBox,
    invisible,
    noGoAway,
    0x0,
    "",
    noAutoCenter
};
