/* NetHack 3.5	mac-qt.h	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.5	mac-qt.h	$Date: 2009/05/06 10:44:50 $  $Revision: 1.4 $ */
/*	SCCS Id: @(#)mac-qt.h	3.5	2003/06/01	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 2003. */
/* NetHack may be freely redistributed.  See license for details. */

/*	Compiler prefix file for the Macintosh Qt port.
 *
 *	IMPORTANT: This file is intended only as a compiler prefix
 *	file and must NEVER be included by other source (.c or .h)
 *	files.
 *
 *	Usage for MacOS X Project Builder:
 *		Project menu -> Edit Active Target '_target_' ->
 *		target settings dialog -> Settings -> Simple View ->
 *		GCC Compiler Settings ->
 *		set "Prefix Header" to include/mac-qt.h
 *
 *	Usage for Metrowerks CodeWarrior:
 *		Edit menu -> _target_ Settings -> Language Settings ->
 *		C/C++ Language ->
 *		set "Prefix File" to include/mac-qt.h
 */

#undef MAC
#define UNIX
#define BSD

/* May already be defined by CodeWarrior as 0 or 1 */
#ifdef TARGET_API_MAC_CARBON
# undef TARGET_API_MAC_CARBON
#endif
#define TARGET_API_MAC_CARBON 0

#define GCC_WARN
