/*
 * $NHDT-Header: NetHack 5.0 nhvers.r 2026/09/30_14:47:34 NetHack-5.0:1.3 $
 * Copyright (c) Ingo Paschke 2026.
 * NetHack may be freely redistributed.  See license for details.
 *
 * nhvers.r: 'vers' resources for the Finder's Get Info window.
 * vers 1 = this file's version; vers 2 = product family line.
 */
#include "Multiverse.r"

resource 'vers' (1, purgeable) {
    0x05, 0x00, release, 0x00, verUS,
    "5.0.0",
    "5.0.0, \0xA9 1995-2026" /* MacRoman copyright sign */
};

resource 'vers' (2, purgeable) {
    0x05, 0x00, release, 0x00, verUS,
    "5.0.0",
    "NetHack 5.0 for 68k Macintosh"
};
