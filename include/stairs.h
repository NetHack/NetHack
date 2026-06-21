/* NetHack 5.0	stairs.h	$NHDT-Date: 1781973088 2026/06/20 16:31:28 $  $NHDT-Branch: NetHack-5.0 $:$NHDT-Revision: 1.1 $ */
/* Copyright (c) 2024 by Pasi Kallinen */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef STAIRS_H
#define STAIRS_H

typedef struct stairway { /* basic stairway identifier */
    coordxy sx, sy;         /* x / y location of the stair */
    d_level tolev;        /* where does it go */
    boolean up;           /* up or down? */
    boolean isladder;     /* ladder or stairway? */
    boolean u_traversed;  /* hero has traversed this stair */
    struct stairway *next;
} stairway;

#endif /* STAIRS_H */

