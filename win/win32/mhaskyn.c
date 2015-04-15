/* NetHack 3.5	mhaskyn.c	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.5	mhaskyn.c	$Date: 2009/05/06 10:59:46 $  $Revision: 1.5 $ */
/*	SCCS Id: @(#)mhaskyn.c	3.5	2005/01/23	*/
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#include <assert.h>
#include "winMS.h"
#include "mhaskyn.h"

int mswin_yes_no_dialog( const char *question, const char *choices, int def)
{
	UNREFERENCED_PARAMETER(question);
	UNREFERENCED_PARAMETER(choices);
	UNREFERENCED_PARAMETER(def);
	return '\032';
}

