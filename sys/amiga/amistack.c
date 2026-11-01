/* NetHack 3.6	amistack.c	$NHDT-Date: 1432512795 2015/05/25 00:13:15 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/* Copyright (c) Janne Salmij�rvi, Tampere, Finland, 2000		*/
/* NetHack may be freely redistributed.  See license for details.	*/

/*
 * Increase stack size to allow deep recursions.
 *
 * Note: This is SAS/C specific, using other compiler probably
 * requires another method for increasing stack.
 *
 */

#ifdef __SASC_60
#include <dos.h>
#endif

/*
 * Increase stack size to allow deep recursions.
 * NetHack 5.0 with Lua needs significantly more stack than 3.6.
 */

#ifdef __SASC_60
long __stack = 256 * 1024;
#else
/* For GCC with -noixemul (libnix), __stack is also recognized */
unsigned long __stack = 256 * 1024;
#endif
