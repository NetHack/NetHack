/* NetHack 3.6	amistack.c	$NHDT-Date: 1432512795 2015/05/25 00:13:15 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/* Copyright (c) Janne Salmijärvi, Tampere, Finland, 2000		*/
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

/*
 * At the moment 90*1024 would suffice, but just to be on the safe side ...
 */

long __stack = 128 * 1024;
#endif
