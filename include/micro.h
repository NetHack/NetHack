/* NetHack 3.7	micro.h	$NHDT-Date: 1596498546 2020/08/03 23:49:06 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.12 $ */
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
