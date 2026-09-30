/* NetHack 5.0	maccompat.h	*/
/* Copyright (c) Ingo Paschke, 2026. */
/* NetHack may be freely redistributed.  See license for details. */
/* maccompat.h - the few shims the Retro68 System 7 build genuinely needs.
 *
 * Apple Universal Interfaces 3.4 with OPAQUE_TOOLBOX_STRUCTS=0 and
 * ACCESSOR_CALLS_ARE_FUNCTIONS=0 already provides most Carbon-style
 * accessors as macros (e.g. GetWindowPortBounds); use those, or native
 * struct access, at the call site.  Only APIs that the headers declare
 * but Interface.o cannot resolve on System 7 belong here.
 *
 * With TARGET_OS_MAC=1 and TARGET_CPU_68K=1 in CFLAGS, the Apple headers
 * generate proper inline trap code for Toolbox calls.  No manual trap
 * declarations are needed here.
 */

#ifndef MACCOMPAT_H
#define MACCOMPAT_H

#if defined(CROSS_TO_MAC68K) || defined(CROSS_TO_MACPPC)

/* InvalWindowRect/InvalWindowRgn are Window Manager 2.0 (Mac OS 8.5);
 * the System 7 equivalent is InvalRect/InvalRgn on the window's port. */
#undef InvalWindowRect
#define InvalWindowRect(win, r)   do { \
    GrafPtr _igp; GetPort(&_igp); SetPort((GrafPtr)(win)); \
    InvalRect(r); SetPort(_igp); } while(0)
#undef InvalWindowRgn
#define InvalWindowRgn(win, rgn)  do { \
    GrafPtr _igp; GetPort(&_igp); SetPort((GrafPtr)(win)); \
    InvalRgn(rgn); SetPort(_igp); } while(0)

/* The canonical way to hand a (B&W or color) port to CopyBits: CopyBits
 * detects a CGrafPort via the rowBytes high bits overlapping portBits. */
#undef GetPortBitMapForCopyBits
#define GetPortBitMapForCopyBits(port) (&((GrafPtr)(port))->portBits)

/* Scrollbar part codes (kControlUpButtonPart etc. and the classic
 * inUpButton aliases). */
#include <ControlDefinitions.h>

#endif /* CROSS_TO_MAC68K || CROSS_TO_MACPPC */

/* Build a Pascal string from a C-string LITERAL at the call site, with the
   length byte computed by sizeof() at compile time.  Constraints: (1)
   literal-only -- sizeof must see the array, so a char* variable won't work;
   (2) the result points at a compound literal with the enclosing block's
   lifetime, so use it inline (e.g. as a call argument), never store it for use
   after the block.  For runtime strings use C2P() (macfile.c) instead. */
#define P_STRING_CONV(X)                                              \
    _Generic((X) + 0,                                                 \
             char *: (StringPtr) &((struct {                          \
                 char len;                                            \
                 char s[sizeof(X) - 1];                               \
             }){ (char) (sizeof(X) - 1), (X) }).len)

/* Empty Pascal string (a single zero length byte), e.g. for unused
   ParamText slots. */
#define P_EMPTY_STRING ((StringPtr) "")

#endif /* MACCOMPAT_H */
