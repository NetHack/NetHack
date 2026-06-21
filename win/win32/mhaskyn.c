/* NetHack 5.0	mhaskyn.c	$NHDT-Date: 1781973101 2026/06/20 16:31:41 $  $NHDT-Branch: NetHack-5.0 $:$NHDT-Revision: 1.14 $ */
/* Copyright (C) 2001 by Alex Kompel */
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
