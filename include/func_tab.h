/* NetHack 3.7	func_tab.h	$NHDT-Date: 1684791775 2023/05/22 21:42:55 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.24 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Pasi Kallinen, 2016. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef FUNC_TAB_H
#define FUNC_TAB_H

/* extended command flags */
#define IFBURIED     0x0001 /* can do command when buried */
#define AUTOCOMPLETE 0x0002 /* command autocompletes */
#define WIZMODECMD   0x0004 /* wizard-mode command */
#define GENERALCMD   0x0008 /* general command, does not take game time */
#define CMD_NOT_AVAILABLE 0x0010 /* recognized but non-functional (!SHELL,&c)*/
#define NOFUZZERCMD  0x0020 /* fuzzer cannot execute this command */
#define INTERNALCMD  0x0040 /* only for internal use, not for user */
#define CMD_M_PREFIX 0x0080 /* accepts menu prefix */
#define CMD_gGF_PREFIX 0x0100 /* accepts g/G/F prefix */
#define CMD_MOVE_PREFIXES  (CMD_M_PREFIX | CMD_gGF_PREFIX)
#define PREFIXCMD    0x0200 /* prefix command, requires another one after it */
#define MOVEMENTCMD  0x0400 /* used to move hero/cursor */
#define MOUSECMD     0x0800 /* cmd allowed to be bound to mouse button */
#define CMD_INSANE   0x1000 /* suppress sanity check (for ^P and ^R) */

/* flags for extcmds_match() */
#define ECM_NOFLAGS       0
#define ECM_IGNOREAC   0x01 /* ignore !autocomplete commands */
#define ECM_EXACTMATCH 0x02 /* needs exact match of findstr */
#define ECM_NO1CHARCMD 0x04 /* ignore commands like '?' and '#' */

struct ext_func_tab {
    uchar key;
    const char *ef_txt, *ef_desc;
    int (*ef_funct)(void); /* must return ECMD_foo flags */
    unsigned flags;
    const char *f_text;
};

extern struct ext_func_tab extcmdlist[];

#endif /* FUNC_TAB_H */
