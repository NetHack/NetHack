/* NetHack 3.7	mhaskyn.c	$NHDT-Date: 1596498346 2020/08/03 23:45:46 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.11 $ */
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
