/* NetHack 3.6	func_tab.h	$NHDT-Date: 1432512775 2015/05/25 00:12:55 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Pasi Kallinen, 2016. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef FUNC_TAB_H
#define FUNC_TAB_H

/* extended command flags */
#define IFBURIED     0x01 /* can do command when buried */
#define AUTOCOMPLETE 0x02 /* command autocompletes */
#define WIZMODECMD   0x04 /* wizard-mode command */
#define GENERALCMD   0x08 /* general command, does not take game time */

struct ext_func_tab {
    uchar key;
    const char *ef_txt, *ef_desc;
    int NDECL((*ef_funct));
    int flags;
    const char *f_text;
};

extern struct ext_func_tab extcmdlist[];

#endif /* FUNC_TAB_H */
