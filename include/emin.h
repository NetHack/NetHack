/*	SCCS Id: @(#)emin.h	3.5	2005/11/02	*/
/* Copyright (c) David Cohrs, 1990.				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef EMIN_H
#define EMIN_H

struct emin {
	aligntyp min_align;	/* alignment of minion */
	boolean  renegade;	/* hostile co-aligned priest or Angel */
};

#define EMIN(mon)	((struct emin *)&(mon)->mextra[0])

#endif /* EMIN_H */
