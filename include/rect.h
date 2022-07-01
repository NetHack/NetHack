/* NetHack 3.7	rect.h	$NHDT-Date: 1596498557 2020/08/03 23:49:17 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.9 $ */
/* Copyright (c) 1990 by Jean-Christophe Collet			  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef RECT_H
#define RECT_H

typedef struct nhrect {
    coordxy lx, ly;
    coordxy hx, hy;
} NhRect;

#endif /* RECT_H */
