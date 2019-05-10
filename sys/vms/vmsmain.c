/* NetHack 3.6	vmsmain.c	$NHDT-Date: 1449801742 2015/12/11 02:42:22 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.32 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */
/* main.c - VMS NetHack */

#include "hack.h"
#include "dlb.h"

#include <signal.h>

static void NDECL(whoami);
static void FDECL(process_options, (int, char **));
static void NDECL(byebye);
#ifndef SAVE_ON_FATAL_ERROR
#ifndef __DECC
#define vms_handler_type int
#else
#define vms_handler_type unsigned int
#endif
extern void FDECL(VAXC$ESTABLISH,
                         (vms_handler_type (*) (genericptr_t, genericptr_t)));
static vms_handler_type FDECL(vms_handler, (genericptr_t, genericptr_t));
#include <ssdef.h> /* system service status codes */
#endif

static void NDECL(wd_message);
static boolean wiz_error_flag = FALSE;

int
main(argc, argv)
int argc;
char *argv[];
{
    register int fd;
#ifdef CHDIR
    register char *dir;
#endif
    boolean resuming = FALSE; /* assume new game */

#ifdef SECURE /* this should be the very first code executed */
    privoff();
    fflush((FILE *) 0); /* force stdio to init itself */
    privon();
#endif

    sys_early_init();

    atexit(byebye);
    hname = argv[0];
    hname = vms_basename(hname); /* name used in 'usage' type messages */
    hackpid = getpid();
    (void) umask(0);

    choose_windows(DEFAULT_WINDOW_SYS);

#ifdef CHDIR /* otherwise no chdir() */
    /*
     * See if we must change directory to the playground.
     * (Perhaps hack is installed with privs and playground is
     *  inaccessible for the player.)
     * The logical name HACKDIR is overridden by a
     *  -d command line option (must be the first option given)
     */
    dir = nh_getenv("NETHACKDIR");
    if (!dir)
        dir = nh_getenv("HACKDIR");
#endif
    if (argc > 1) {
#ifdef CHDIR
        if (!strncmp(argv[1], "-d", 2) && argv[1][2] != 'e') {
            /* avoid matching "-dec" for DECgraphics; since the man page
             * says -d directory, hope nobody's using -desomething_else
             */
            argc--;
            argv++;
            dir = argv[0] + 2;
            if (*dir == '=' || *dir == ':')
                dir++;
            if (!*dir && argc > 1) {
                argc--;
                argv++;
                dir = argv[0];
            }
            if (!*dir)
                error("Flag -d must be followed by a directory name.");
        }
        if (argc > 1)
#endif /* CHDIR */

            /*
             * Now we know the directory containing 'record' and
             * may do a prscore().
             */
            if (!strncmp(argv[1], "-s", 2)) {
#ifdef CHDIR
                chdirx(dir, FALSE);
#endif
#ifdef SYSCF
                initoptions();
#endif
                prscore(argc, argv);
                exit(EXIT_SUCCESS);
            }
    }

#ifdef CHDIR
    /* move to the playground directory; 'termcap' might be found there */
    chdirx(dir, TRUE);
#endif

#ifdef SECURE
    /* disable installed privs while loading nethack.cnf and termcap,
       and also while initializing terminal [$assign("TT:")]. */
    privoff();
#endif
    initoptions();
    init_nhwindows(&argc, argv);
    whoami();
#ifdef SECURE
    privon();
#endif

    /*
     * It seems you really want to play.
     */
    u.uhp = 1; /* prevent RIP on early quits */
#ifndef SAVE_ON_FATAL_ERROR
    /* used to clear hangup stuff while still giving standard traceback */
    VAXC$ESTABLISH(vms_handler);
#endif
    sethanguphandler(hangup);

    process_options(argc, argv); /* command line options */

    /* wizard mode access is deferred until here */
    set_playmode(); /* sets plname to "wizard" for wizard mode */
    /* strip role,race,&c suffix; calls askname() if plname[] is empty
       or holds a generic user name like "player" or "games" */
    plnamesuffix();

    if (wizard) {
        /* use character name rather than lock letter for file names */
        locknum = 0;
    } else {
        /* suppress interrupts while processing lock file */
        (void) signal(SIGQUIT, SIG_IGN);
        (void) signal(SIGINT, SIG_IGN);
    }
    /*
     * getlock() complains and quits if there is already a game
     * in progress for current character name (when locknum == 0)
     * or if there are too many active games (when locknum > 0).
     * When proceeding, it creates an empty <lockname>.0 file to
     * designate the current game.
     * getlock() constructs <lockname> based on the character
     * name (for !locknum) or on first available of alock, block,
     * clock, &c not currently in use in the playground directory
     * (for locknum > 0).
     */
    getlock();

    dlb_init(); /* must be before newgame() */

    /*
     *  Initialize the vision system.  This must be before mklev() on a
     *  new game or before a level restore on a saved game.
     */
    vision_init();

    display_gamewindows();

/*
 * First, try to find and restore a save file for specified character.
 * We'll return here if new game player_selection() renames the hero.
 */
attempt_restore:
    if ((fd = restore_saved_game()) >= 0) {
        const char *fq_save = fqname(SAVEF, SAVEPREFIX, 1);

        (void) chmod(fq_save, 0); /* disallow parallel restores */
        (void) signal(SIGINT, (SIG_RET_TYPE) done1);
#ifdef NEWS
        if (iflags.news) {
            display_file(NEWS, FALSE);
            iflags.news = FALSE; /* in case dorecover() fails */
        }
#endif
        pline("Restoring save file...");
        mark_synch(); /* flush output */
        if (dorecover(fd)) {
            resuming = TRUE; /* not starting new game */
            wd_message();
            if (discover || wizard) {
                if (yn("Do you want to keep the save file?") == 'n')
                    (void) delete_savefile();
                else
                    (void) chmod(fq_save, FCMASK); /* back to readable */
            }
        }
    }

    if (!resuming) {
        /* new game:  start by choosing role, race, etc;
           player might change the hero's name while doing that,
           in which case we try to restore under the new name
           and skip selection this time if that didn't succeed */
        if (!iflags.renameinprogress) {
            player_selection();
            if (iflags.renameinprogress) {
                /* player has renamed the hero while selecting role;
                   if locking alphabetically, the existing lock file
                   can still be used; otherwise, discard current one
                   and create another for the new character name */
                if (!locknum) {
                    delete_levelfile(0); /* remove empty lock file */
                    getlock();
                }
                goto attempt_restore;
            }
        }
        newgame();
        wd_message();
    }

    moveloop(resuming);
    exit(EXIT_SUCCESS);
    /*NOTREACHED*/
    return 0;
}

static void
process_options(argc, argv)
int argc;
char *argv[];
{
    int i;

    /*
     * Process options.
     */
    while (argc > 1 && argv[1][0] == '-') {
        argv++;
        argc--;
        switch (argv[0][1]) {
        case 'D':
            wizard = TRUE, discover = FALSE;
            break;
        case 'X':
        case 'x':
            discover = TRUE, wizard = FALSE;
            break;
#ifdef NEWS
        case 'n':
            iflags.news = FALSE;
            break;
#endif
        case 'u':
            if (argv[0][2])
                (void) strncpy(plname, argv[0] + 2, sizeof(plname) - 1);
            else if (argc > 1) {
                argc--;
                argv++;
                (void) strncpy(plname, argv[0], sizeof(plname) - 1);
            } else
                raw_print("Player name expected after -u");
            break;
        case 'I':
        case 'i':
            if (!strncmpi(argv[0] + 1, "IBM", 3)) {
                load_symset("IBMGraphics", PRIMARY);
                load_symset("RogueIBM", ROGUESET);
                switch_symbols(TRUE);
            }
            break;
        /*  case 'D': */
        case 'd':
            if (!strncmpi(argv[0] + 1, "DEC", 3)) {
                load_symset("DECGraphics", PRIMARY);
                switch_symbols(TRUE);
            }
            break;
        case 'p': /* profession (role) */
            if (argv[0][2]) {
                if ((i = str2role(&argv[0][2])) >= 0)
                    flags.initrole = i;
            } else if (argc > 1) {
                argc--;
                argv++;
                if ((i = str2role(argv[0])) >= 0)
                    flags.initrole = i;
            }
            break;
        case 'r': /* race */
            if (argv[0][2]) {
                if ((i = str2race(&argv[0][2])) >= 0)
                    flags.initrace = i;
            } else if (argc > 1) {
                argc--;
                argv++;
                if ((i = str2race(argv[0])) >= 0)
                    flags.initrace = i;
            }
            break;
        case '@':
            flags.randomall = 1;
            break;
        default:
            if ((i = str2role(&argv[0][1])) >= 0) {
                flags.initrole = i;
                break;
            }
            /* else raw_printf("Unknown option: %s", *argv); */
        }
    }

    if (argc > 1)
        locknum = atoi(argv[1]);
#ifdef MAX_NR_OF_PLAYERS
    if (!locknum || locknum > MAX_NR_OF_PLAYERS)
        locknum = MAX_NR_OF_PLAYERS;
#endif
}

#ifdef CHDIR
void
chdirx(dir, wr)
const char *dir;
boolean wr;
{
#ifndef HACKDIR
    static const char *defdir = ".";
#else
    static const char *defdir = HACKDIR;

    if (dir == (const char *) 0)
        dir = defdir;
    else if (wr && !same_dir(HACKDIR, dir))
        /* If we're playing anywhere other than HACKDIR, turn off any
           privs we may have been installed with. */
        privoff();
#endif

    if (dir && chdir(dir) < 0) {
        perror(dir);
        error("Cannot chdir to %s.", dir);
    }

    /* warn the player if we can't write the record file */
    if (wr)
        check_recordfile(dir);

    defdir = dir;
}
#endif /* CHDIR */

static void
whoami()
{
    /*
     * Who am i? Algorithm: 1. Use name as specified in NETHACKOPTIONS;
     *                      2. Use lowercase of $USER (if 1. fails).
     * The resulting name is overridden by command line options.
     * If everything fails, or if the resulting name is some generic
     * account like "games" then eventually we'll ask him.
     * Note that we trust the user here; it is possible to play under
     * somebody else's name.
     */
    register char *s;

    if (!*plname && (s = nh_getenv("USER")))
        (void) lcase(strncpy(plname, s, sizeof(plname) - 1));
}

static void
byebye()
{
    void FDECL((*hup), (int) );
#ifdef SHELL
    extern unsigned long dosh_pid, mail_pid;
    extern unsigned long FDECL(sys$delprc,
                               (unsigned long *, const genericptr_t));

    /* clean up any subprocess we've spawned that may still be hanging around
     */
    if (dosh_pid)
        (void) sys$delprc(&dosh_pid, (genericptr_t) 0), dosh_pid = 0;
    if (mail_pid)
        (void) sys$delprc(&mail_pid, (genericptr_t) 0), mail_pid = 0;
#endif

    /* SIGHUP doesn't seem to do anything on VMS, so we fudge it here... */
    hup = (void FDECL((*), (int) )) signal(SIGHUP, SIG_IGN);
    if (!program_state.exiting++ && hup != (void FDECL((*), (int) )) SIG_DFL
        && hup != (void FDECL((*), (int) )) SIG_IGN) {
        (*hup)(SIGHUP);
    }

#ifdef CHDIR
    (void) chdir(getenv("PATH"));
#endif
}

#ifndef SAVE_ON_FATAL_ERROR
/* Condition handler to prevent byebye's hangup simulation
   from saving the game after a fatal error has occurred.  */
/*ARGSUSED*/
static vms_handler_type         /* should be `unsigned long', but the -*/
vms_handler(sigargs, mechargs)  /*+ prototype in <signal.h> is screwed */
genericptr_t sigargs, mechargs; /* [0] is argc, [1..argc] are the real args */
{
    unsigned long condition = ((unsigned long *) sigargs)[1];

    if (condition == SS$_ACCVIO /* access violation */
        || (condition >= SS$_ASTFLT && condition <= SS$_TBIT)
        || (condition >= SS$_ARTRES && condition <= SS$_INHCHME)) {
        program_state.done_hup = TRUE; /* pretend hangup has been attempted */
#if (NH_DEVEL_STATUS == NH_STATUS_RELEASED)
        if (wizard)
#endif
            abort(); /* enter the debugger */
    }
    return SS$_RESIGNAL;
}
#endif

void
sethanguphandler(handler)
void FDECL((*handler), (int));
{
    (void) signal(SIGHUP, (SIG_RET_TYPE) handler);
}

#ifdef PORT_HELP
void
port_help()
{
    /*
     * Display VMS-specific help.   Just show contents of the helpfile
     * named by PORT_HELP.
     */
    display_file(PORT_HELP, TRUE);
}
#endif /* PORT_HELP */

/* validate wizard mode if player has requested access to it */
boolean
authorize_wizard_mode()
{
    if (!strcmpi(nh_getenv("USER"), WIZARD_NAME))
        return TRUE;
    wiz_error_flag = TRUE; /* not being allowed into wizard mode */
    return FALSE;
}

static void
wd_message()
{
    if (wiz_error_flag) {
        pline("Only user \"%s\" may access debug (wizard) mode.",
              WIZARD_NAME);
        pline("Entering explore/discovery mode instead.");
        wizard = 0, discover = 1; /* (paranoia) */
    } else if (discover)
        You("are in non-scoring explore/discovery mode.");
}

unsigned long
sys_random_seed()
{
    unsigned long seed;
    unsigned long pid = (unsigned long) getpid();

    seed = (unsigned long) getnow(); /* time((TIME_type) 0) */
    /* Quick dirty band-aid to prevent PRNG prediction */
    if (pid) {
        if (!(pid & 3L))
            pid -= 1L;
        seed *= pid;
    }
    return seed;
}

/*vmsmain.c*/
