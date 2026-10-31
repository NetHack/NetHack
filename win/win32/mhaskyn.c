/* $NHDT-Header: NetHack 5.0 mhaskyn.c 2026/06/20_22:26:43 NetHack-5.0:1.16 $ */
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
