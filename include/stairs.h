/* NetHack 3.7	stairs.h	$NHDT-Date: 1685863327 2023/06/04 07:22:07 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.47 $ */
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

