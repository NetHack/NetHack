/*	SCCS Id: @(#)botl.h 3.4	$Date$	*/
/* Copyright (c) Michael Allison, 2003				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef BOTL_H
#define BOTL_H

#define BL_FLUSH	-1
#define BL_TITLE	0
#define BL_STR		1
#define BL_DX		2
#define BL_CO		3
#define BL_IN		4
#define BL_WI		5
#define BL_CH		6
#define BL_ALIGN	7
#define BL_SCORE	8
#define BL_CAP		9
#define BL_GOLD		10
#define BL_ENE		11
#define BL_ENEMAX	12
#define BL_XP		13
#define BL_AC		14
#define BL_HD		15
#define BL_TIME		16
#define BL_HUNGER	17
#define BL_HP		18
#define BL_HPMAX	19
#define BL_LEVELDESC	20
#define BL_EXP		21
#define BL_CONDITION	22
#define MAXBLSTATS 	23

/* Boolean condition bits for the condition mask */

#define BL_MASK_BLIND		0x00000001L
#define BL_MASK_CONF		0x00000002L
#define BL_MASK_FOODPOIS	0x00000004L
#define BL_MASK_ILL		0x00000008L
#define BL_MASK_HALLU		0x00000010L
#define BL_MASK_STUNNED		0x00000020L
#define BL_MASK_SLIMED		0x00000040L

#endif /* BOTL_H */
