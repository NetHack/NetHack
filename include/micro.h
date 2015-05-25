/* NetHack 3.6	micro.h	$NHDT-Date: 1432512781 2015/05/25 00:13:01 $  $NHDT-Branch: master $:$NHDT-Revision: 1.9 $ */
/* micro.h - function declarations for various microcomputers */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MICRO_H
#define MICRO_H

extern const char *alllevels, *allbones;
extern char levels[], bones[], permbones[], hackdir[];

extern int ramdisk;

#ifndef C
#define C(c) (0x1f & (c))
#endif
#ifndef M
#define M(c) (((char) 0x80) | (c))
#endif
#define ABORT C('a')

#endif /* MICRO_H */
