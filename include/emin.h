/*	SCCS Id: @(#)emin.h	3.4	1997/05/01	*/
/* Copyright (c) David Cohrs, 1990.				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef EMIN_H
#define EMIN_H

struct emin {
	aligntyp min_align;	/* alignment of minion */
};

#define EMIN(mon)	((struct emin *)&(mon)->mextra[0])

#endif /* EMIN_H */
