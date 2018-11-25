/* NetHack 3.6	lev.h	$NHDT-Date: 1432512781 2015/05/25 00:13:01 $  $NHDT-Branch: master $:$NHDT-Revision: 1.12 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2006. */
/* NetHack may be freely redistributed.  See license for details. */

/*	Common include file for save and restore routines */

#ifndef LEV_H
#define LEV_H

#define COUNT_SAVE 0x1
#define WRITE_SAVE 0x2
#define FREE_SAVE 0x4

/* operations of the various saveXXXchn & co. routines */
#define perform_bwrite(mode) ((mode) & (COUNT_SAVE | WRITE_SAVE))
#define release_data(mode) ((mode) &FREE_SAVE)

#endif /* LEV_H */
