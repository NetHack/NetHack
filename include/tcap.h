/* NetHack 3.7	tcap.h	$NHDT-Date: 1596498562 2020/08/03 23:49:22 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.12 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1989. */
/*-Copyright (c) Kenneth Lorber, Kensington, Maryland, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

/* not named termcap.h because it may conflict with a system header */

#ifndef TCAP_H
#define TCAP_H

#ifndef MICRO
#define TERMLIB /* include termcap code */
#endif

/* might display need graphics code? */
#if !defined(AMIGA) && !defined(TOS) && !defined(MAC)
#if defined(TERMLIB) || defined(OS2) || defined(MSDOS)
#define ASCIIGRAPH
#endif
#endif

#ifndef DECL_H
extern struct tc_gbl_data { /* also declared in decl.h; defined in decl.c */
    char *tc_AS, *tc_AE;    /* graphics start and end (tty font swapping) */
    int tc_LI, tc_CO;       /* lines and columns */
} tc_gbl_data;
#define AS tc_gbl_data.tc_AS
#define AE tc_gbl_data.tc_AE
#define LI tc_gbl_data.tc_LI
#define CO tc_gbl_data.tc_CO
#endif

extern struct tc_lcl_data { /* defined and set up in termcap.c */
    char *tc_CM, *tc_ND, *tc_CD;
    char *tc_HI, *tc_HE, *tc_US, *tc_UE;
    boolean tc_ul_hack;
} tc_lcl_data;
/* some curses.h declare CM etc. */
#define nh_CM tc_lcl_data.tc_CM
#define nh_ND tc_lcl_data.tc_ND
#define nh_CD tc_lcl_data.tc_CD
#define nh_HI tc_lcl_data.tc_HI
#define nh_HE tc_lcl_data.tc_HE
#define nh_US tc_lcl_data.tc_US
#define nh_UE tc_lcl_data.tc_UE
#define ul_hack tc_lcl_data.tc_ul_hack

extern short ospeed; /* set up in termcap.c */

#ifdef TEXTCOLOR
#ifdef TOS
extern const char *hilites[CLR_MAX];
#else
extern NEARDATA char *hilites[CLR_MAX];
#endif
#endif

#endif /* TCAP_H */
