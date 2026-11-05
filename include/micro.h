/* $NHDT-Header: NetHack 5.0 micro.h 2026/06/20_22:26:43 NetHack-5.0:1.16 $ */
/*      Copyright (c) 2015 by Kenneth Lorber              */
/* NetHack may be freely redistributed.  See license for details. */

/* micro.h - function declarations for various microcomputers */

#ifndef MICRO_H
#define MICRO_H

#ifndef C
#define C(c) (0x1f & (c))
#endif
#ifndef M
#define M(c) (((char) 0x80) | (c))
#endif
#define ABORT C('a')

#endif /* MICRO_H */
