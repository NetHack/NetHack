/*	SCCS Id: @(#)epri.h	3.5	2005/11/02	*/
/* Copyright (c) Izchak Miller, 1989.				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef EPRI_H
#define EPRI_H

struct epri {
	aligntyp shralign;	/* alignment of priest's shrine */
				/* leave as first field to match emin */
	schar shroom;		/* index in rooms */
	coord shrpos;		/* position of shrine */
	d_level shrlevel;	/* level (& dungeon) of shrine */
};

#define EPRI(mon)	((struct epri *)&(mon)->mextra[0])

/* note: roaming priests (no shrine) switch from ispriest to isminion
   (and emin extension) */

#endif /* EPRI_H */
