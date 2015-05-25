/* NetHack 3.6	mhaskyn.c	$NHDT-Date: 1432512812 2015/05/25 00:13:32 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#include <assert.h>
#include "winMS.h"
#include "mhaskyn.h"

int
mswin_yes_no_dialog(const char *question, const char *choices, int def)
{
    UNREFERENCED_PARAMETER(question);
    UNREFERENCED_PARAMETER(choices);
    UNREFERENCED_PARAMETER(def);
    return '\032';
}
