/* $NHDT-Header: NetHack 5.0 amistack.c 2015/05/25_00:13:15 NetHack-5.0:1.17 $ */
/* Copyright (c) Janne Salmijärvi, Tampere, Finland, 2000		*/
/* NetHack may be freely redistributed.  See license for details.	*/

/*
 * Increase stack size to allow deep recursions.
 * NetHack 5.0 with Lua needs significantly more stack than 3.6.
 *
 * libnix needs __stkinit referenced to pull swapstack.o from libnix.a.
 */

unsigned long __stack = 256 * 1024;
#ifdef CROSS_TO_AMIGA
extern void __stkinit(void);
void *__nh_force_stkinit = __stkinit;
#endif
