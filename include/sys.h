/* NetHack 3.7	sys.h	$NHDT-Date: 1646255373 2022/03/02 21:09:33 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.38 $ */
/* Copyright (c) Kenneth Lorber, Kensington, Maryland, 2008. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef SYS_H
#define SYS_H

struct sysopt {
    char *support; /* local support contact */
    char *recover; /* how to run recover - may be overridden by win port */
    char *wizards; /* space-separated list of usernames */
    char *fmtd_wizard_list; /* formatted version of wizards; null or "one"
                               or "one or two" or "one, two, or three", &c */
    char *explorers;  /* like wizards, but for access to explore mode */
    char *shellers;   /* like wizards, for ! command (-DSHELL); also ^Z */
    char *genericusers; /* usernames that prompt for user name */
    char *debugfiles; /* files to show debugplines in. '*' is all. */
#ifdef DUMPLOG
    char *dumplogfile; /* where the dump file is saved */
#endif
    int env_dbgfl;    /*  1: debugfiles comes from getenv("DEBUGFILES")
                       *     so sysconf's DEBUGFILES shouldn't override it;
                       *  0: getenv() hasn't been attempted yet;
                       * -1: getenv() didn't find a value for DEBUGFILES.
                       */
    int maxplayers;
    int seduce;
    int check_save_uid; /* restoring savefile checks UID? */
    int check_plname; /* use plname for checking wizards/explorers/shellers */
    int bones_pools;
    long livelog; /* LL_foo events to livelog */

    /* record file */
    int persmax;
    int pers_is_uid;
    int entrymax;
    int pointsmin;
    int tt_oname_maxrank;

    /* panic options */
    char *gdbpath;
    char *greppath;
    int panictrace_gdb;
    int panictrace_libc;

    /* save and bones format */
    int saveformat[2];    /* primary and onetime conversion */
    int bonesformat[2];   /* primary and onetime conversion */

    /* enable accessibility options */
    int accessibility;
#ifdef WIN32
    int portable_device_paths;  /* nethack config for a portable device */
#endif

    /* nethack's interactive help menu */
    int hideusage;      /* 0: include 'command-line usage' entry in help menu;
                         * 1: suppress it */
};

extern struct sysopt sysopt;

#define SYSOPT_SEDUCE sysopt.seduce

#endif /* SYS_H */
