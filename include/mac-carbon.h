/* NetHack 3.7	mac-carbon.h	$NHDT-Date: 1596498540 2020/08/03 23:49:00 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.9 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 2003. */
/*-Copyright (c) Kevin Hugo, 2003. */
/* NetHack may be freely redistributed.  See license for details. */

/*	Compiler prefix file for the Macintosh Carbon port.
 *
 *	IMPORTANT: This file is intended only as a compiler prefix
 *	file and must NEVER be included by other source (.c or .h)
 *	files.
 *
 *	Usage for MacOS X Project Builder:
 *		Project menu -> Edit Active Target '_target_' ->
 *		target settings dialog -> Settings -> Simple View ->
 *		GCC Compiler Settings ->
 *		set "Prefix Header" to include/mac-carbon.h
 *
 *	Usage for Metrowerks CodeWarrior:
 *		Edit menu -> _target_ Settings -> Language Settings ->
 *		C/C++ Language ->
 *		set "Prefix File" to include/mac-carbon.h
 */

#define MAC
#undef UNIX

/* May already be defined by CodeWarrior as 0 or 1 */
#ifdef TARGET_API_MAC_CARBON
#undef TARGET_API_MAC_CARBON
#endif
#define TARGET_API_MAC_CARBON 1

#define GCC_WARN
