/* NetHack 3.6	ovlinit.c	$NHDT-Date: 1432512792 2015/05/25 00:13:12 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/* Copyright (c) NetHack PC Development Team 1995                 */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include <dos.h>
#include <stdio.h>

#ifdef _MSC_VER

#define RESERVED_PARAGRAPHS 5120 /* leave 80K for malloc and inits */
                                 /* subject to change before release */

/*
 * memavail() Returns the amount of RAM available (in paragraphs which are 16
 *  bytes) - the amount to be reserved for heap allocations.
 *
 */
unsigned
memavail(minovl)
unsigned minovl; /* minimum size of overlay heap */
{
    unsigned available;

    unsigned farparaavail;
    unsigned tmp;

    /*
     * _dos_allocmem will return the maximum block size available.
     * It uses DOS (int 21h) service 0x48.
     */

    _dos_allocmem(0xFFFF, &farparaavail);
    available = farparaavail - RESERVED_PARAGRAPHS;
    tmp = RESERVED_PARAGRAPHS + minovl;
    if (farparaavail < tmp) {
        panic("Not enough free RAM to begin a game of NetHack (%ld bytes)",
              (long) ((long) tmp * 16L));
    }
    return available;
}
#endif /*_MSC_VER*/

#ifdef __BORLANDC__

#define RSRVD_MALLOC 65 * 1024L /* malloc() calls use about 65K    */
#define RSRVD_CRTL 50 * 1024L   /* C runtime library uses 50K      */
#define RSRVD_TOTAL 115 * 1024L /* reserved for use in malloc()    */
                                /* as well as by C runtime library */
                                /* routines which allocate memory  */
                                /* after this routine runs.        */
#define MIN_OVRBUF 30 * 1024L   /* Overlay buffer gets minimum of  */
#define MAX_OVRBUF 200 * 1024L  /* 30K and maximum of 200K.        */

#define RESIZE_OVL
#ifdef RESIZE_OVL

extern unsigned _ovrbuffer = 0; /* Use default size initially */
unsigned appFail = 0;           /* Fail flag if not enough RAM */
unsigned memAlloc = 0;
unsigned long ProgramSize;
unsigned long runAlloc;
unsigned far *mem_top;
unsigned total;
signed long tmpbuffer;
int emsstatus;
int xmsstatus;

void NDECL(_resizeOvrBuffer);

void
_resizeOvrBuffer()
{
    mem_top = (unsigned far *) MK_FP(_psp, 0x02);
    total = *mem_top - _psp;

    ProgramSize = *(unsigned far *) MK_FP(_psp - 1, 0x03);
    tmpbuffer = total - ProgramSize - RSRVD_TOTAL / 16;
    memAlloc = min(MAX_OVRBUF / 16, tmpbuffer);
    if (tmpbuffer >= MIN_OVRBUF / 16)
        _ovrbuffer = memAlloc;
    else {
        _ovrbuffer = 1;
        appFail = 1;
    };

    /*
     * Remember, when inside this code, nothing has been setup on
     * the system, so do NOT call any RTL functions for I/O or
     * anything else that might rely on a startup function.  This
     * includes accessing any global objects as their constructors
     * have not been called yet.
     */
}

#pragma startup _resizeOvrBuffer 0 /* Put function in table */

void
startup()
{
    if (appFail) {
        printf("NetHack fits in memory, but it cannot allocate memory");
        printf(" for the overlay buffer\nand the runtime functions.  ");
        printf("Please free up just %ld more bytes.",
               (long) (MIN_OVRBUF - tmpbuffer * 16L));
        exit(-1);
    } else {
        /* Now try to use expanded memory for the overlay manager */
        /* If that doesn't work, we revert to extended memory */

        emsstatus = _OvrInitEms(0, 0, 0);
#ifdef RECOGNIZE_XMS
        xmsstatus = (emsstatus) ? _OvrInitExt(0, 0) : -1;
#endif
    }
}

void
show_borlandc_stats(win)
winid win;
{
    char buf[BUFSZ];

    putstr(win, 0, "");
    putstr(win, 0, "");
    putstr(win, 0, "Memory usage stats");
    putstr(win, 0, "");
    putstr(win, 0, "");
    Sprintf(buf, "Overlay buffer memory allocation: %ld bytes.",
            memAlloc * 16L);
    putstr(win, 0, buf);
    Sprintf(buf, "_ovrbuffer = %u.", _ovrbuffer);
    putstr(win, 0, buf);
    Sprintf(buf, "Startup memory usage: 0x%X", ProgramSize);
    putstr(win, 0, buf);
    runAlloc = *(unsigned far *) MK_FP(_psp - 1, 0x03);
    Sprintf(buf, "Current memory usage: 0x%X", runAlloc);
    putstr(win, 0, buf);
    if (emsstatus)
        Sprintf(buf, "EMS search failed (%d).", emsstatus);
    else
        Sprintf(buf, "EMS search successful.");
    putstr(win, 0, buf);
#ifdef RECOGNIZE_XMS
    if (xmsstatus)
        Sprintf(buf, "XMS search failed (%d).", xmsstatus);
    else
        Sprintf(buf, "XMS search successful.");
    putstr(win, 0, buf);
#endif
}

#endif /* #ifdef RESIZE_OVL */
#endif /* #ifdef __BORLANDC__ */

/*ovlinit.c*/
