/* NetHack 3.5	rect.h	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.5	rect.h	$Date: 2009/05/06 10:45:03 $  $Revision: 1.4 $ */
/*	SCCS Id: @(#)rect.h	3.5	1990/02/22	*/
/* Copyright (c) 1990 by Jean-Christophe Collet			  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef RECT_H
#define RECT_H

typedef struct nhrect {
	xchar lx, ly;
	xchar hx, hy;
} NhRect;

#endif /* RECT_H */
