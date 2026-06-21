/* NetHack 5.0	rect.h	$NHDT-Date: 1781973086 2026/06/20 16:31:26 $  $NHDT-Branch: NetHack-5.0 $:$NHDT-Revision: 1.13 $ */
/* Copyright (c) 1990 by Jean-Christophe Collet                   */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef RECT_H
#define RECT_H

typedef struct nhrect {
    coordxy lx, ly;
    coordxy hx, hy;
} NhRect;

#endif /* RECT_H */
