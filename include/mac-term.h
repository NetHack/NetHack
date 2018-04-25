/* NetHack 3.6	mac-term.h	$NHDT-Date: 1432512775 2015/05/25 00:12:55 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 2003. */
/*-Copyright (c) Kevin Hugo, 2003. */
/* NetHack may be freely redistributed.  See license for details. */

/*	Compiler prefix file for the MacOS X Terminal.app port.
 *
 *	IMPORTANT: This file is intended only as a compiler prefix
 *	file and must NEVER be included by other source (.c or .h)
 *	files.
 *
 *	Usage for MacOS X Project Builder:
 *		Project menu -> Edit Active Target '_target_' ->
 *		target settings dialog -> Settings -> Simple View ->
 *		GCC Compiler Settings ->
 *		set "Prefix Header" to include/mac-term.h
 *
 *	Usage for Metrowerks CodeWarrior:
 *		Edit menu -> _target_ Settings -> Language Settings ->
 *		C/C++ Language ->
 *		set "Prefix File" to include/mac-term.h
 */

/* Stuff needed for the core of NetHack */
#undef MAC
#define UNIX
#define BSD
#define __FreeBSD__ /* Darwin is based on FreeBSD */
#define GCC_WARN

/* May already be defined by CodeWarrior as 0 or 1 */
#ifdef TARGET_API_MAC_CARBON
#undef TARGET_API_MAC_CARBON
#endif
#define TARGET_API_MAC_CARBON 0 /* Not Carbon */
