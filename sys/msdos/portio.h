/* NetHack 3.6	portio.h	$NHDT-Date: 1432512791 2015/05/25 00:13:11 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/*   Copyright (c) NetHack PC Development Team 1995                 */
/*   NetHack may be freely redistributed.  See license for details. */
/*                                                                  */
/*
 * portio.h - PC port I/O Hardware support definitions and other
 *            low-level definitions.
 *
 */

#ifndef PORTIO_H
#define PORTIO_H

#if defined(__GO32__) || defined(__DJGPP__)
#define __far
#include <go32.h>
#include <dpmi.h>
#include <sys/farptr.h>
#endif

#if defined(_MSC_VER)
#define outportb _outp
#define outportw _outpw
#define inportb _inp
#endif
#if defined(__BORLANDC__)
#define outportw outport
/* #define inportb inport */
#endif

#ifndef MK_PTR
/*
 * Depending on environment, this is a macro to construct either:
 *
 *     -  a djgpp long 32 bit pointer from segment & offset values
 *     -  a far pointer from segment and offset values
 *
 */
#if defined(_MSC_VER) || defined(__BORLANDC__)
#define MK_PTR(seg, offset)                    \
    (void __far *)(((unsigned long) seg << 16) \
                   + (unsigned long) (unsigned) offset)
#define READ_ABSOLUTE(x) *(x)
#define READ_ABSOLUTE_WORD(x) *(x)
#define WRITE_ABSOLUTE(x, y) *(x) = (y)
#define WRITE_ABSOLUTE_WORD(x, y) *(x) = (y)
#define WRITE_ABSOLUTE_DWORD(x, y) *(x) = (y)
#endif

#if defined(__GO32__) || defined(__DJGPP__)
#define MK_PTR(seg, offset) \
    (void *)(((unsigned) seg << 4) + (unsigned) offset)
#define READ_ABSOLUTE(x) \
    (_farpeekb(_go32_conventional_mem_selector(), (unsigned) x))
#define READ_ABSOLUTE_WORD(x) \
    (_farpeekw(_go32_conventional_mem_selector(), (unsigned) x))
#define WRITE_ABSOLUTE(x, y) \
    _farpokeb(_go32_conventional_mem_selector(), (unsigned) x, (y))
#define WRITE_ABSOLUTE_WORD(x, y) \
    _farpokew(_go32_conventional_mem_selector(), (unsigned) x, (y))
#define WRITE_ABSOLUTE_DWORD(x, y) \
    _farpokel(_go32_conventional_mem_selector(), (unsigned) x, (y))
#endif

#ifdef OBSOLETE /* old djgpp V1.x way of mapping 1st MB */
#define MK_PTR(seg, offset) \
    (void *)(0xE0000000 + ((((unsigned) seg << 4) + (unsigned) offset)))
#define READ_ABSOLUTE(x) *(x)
#define READ_ABSOLUTE_WORD(x) *(x)
#define WRITE_ABSOLUTE(x, y) *(x) = (y)
#define WRITE_ABSOLUTE_WORD(x, y) *(x) = (y)
#define WRITE_ABSOLUTE_DWORD(x, y) *(x) = (y)
#endif
#endif /* MK_PTR */

#endif /* PORTIO_H  */
/* portio.h */
