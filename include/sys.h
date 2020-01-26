/* NetHack 3.6	sys.h	$NHDT-Date: 1449296291 2015/12/05 06:18:11 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.27 $ */
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

    /* enable accessibility options */
    int accessibility;
#ifdef WIN32
    int portable_device_paths;  /* nethack configuration for a portable device */
#endif
};

extern struct sysopt sysopt;

#define SYSOPT_SEDUCE sysopt.seduce

#endif /* SYS_H */
