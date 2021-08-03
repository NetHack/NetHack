/* NetHack 3.7	func_tab.h	$NHDT-Date: 1596498537 2020/08/03 23:48:57 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.14 $ */
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
#define CMD_NOT_AVAILABLE 0x10 /* recognized but non-functional (!SHELL,&c) */
#define NOFUZZERCMD  0x20 /* fuzzer cannot execute this command */
#define INTERNALCMD  0x40 /* only for internal use, not for user */

struct ext_func_tab {
    uchar key;
    const char *ef_txt, *ef_desc;
    int (*ef_funct)(void);
    int flags;
    const char *f_text;
};

extern struct ext_func_tab extcmdlist[];

#endif /* FUNC_TAB_H */
