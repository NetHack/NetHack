/* SCCS Id: @(#)amistack.c	3.4	2000/05/03			*/
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

long __stack = 128*1024; 
#endif
