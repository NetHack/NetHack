/*    SCCS Id: @(#)sys.c      3.5     2008/01/30      */
/* Copyright (c) Kenneth Lorber, Kensington, Maryland, 2008. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

struct sysopt sysopt;

void
sys_early_init(){
	sysopt.support = NULL;
	sysopt.recover = NULL;
#ifdef notyet
	/* replace use of WIZARD vs WIZARD_NAME vs KR1ED, by filling this in */
#endif
	sysopt.wizards = NULL;
}

