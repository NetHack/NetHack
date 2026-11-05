/* $NHDT-Header: NetHack 5.0 rect.h 2026/06/20_22:26:43 NetHack-5.0:1.15 $ */
/* Copyright (c) 1990 by Jean-Christophe Collet                   */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef RECT_H
#define RECT_H

typedef struct nhrect {
    coordxy lx, ly;
    coordxy hx, hy;
} NhRect;

#endif /* RECT_H */
