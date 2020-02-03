/* NetHack 3.6	pcmain.c	$NHDT-Date: 1543465755 2018/11/29 04:29:15 $  $NHDT-Branch: NetHack-3.6.2-beta01 $:$NHDT-Revision: 1.101 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Derek S. Ray, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

/* main.c - MSDOS, OS/2, ST, Amiga NetHack */

#include "hack.h"
#include "dlb.h"

#ifndef NO_SIGNAL
#include <signal.h>
#endif

#include <ctype.h>

#if !defined(AMIGA) && !defined(__DJGPP__)
#include <sys\stat.h>
#else
#include <sys/stat.h>
#endif

#ifdef __DJGPP__
#include <unistd.h> /* for getcwd() prototype */
#endif

char orgdir[PATHLEN]; /* also used in pcsys.c, amidos.c */

#ifdef TOS
boolean run_from_desktop = TRUE; /* should we pause before exiting?? */
#ifdef __GNUC__
long _stksize = 16 * 1024;
#endif
#endif

#ifdef AMIGA
extern int bigscreen;
void NDECL(preserve_icon);
#endif

static void FDECL(process_options, (int argc, char **argv));
static void NDECL(nhusage);

#if defined(MICRO) || defined(OS2)
extern void FDECL(nethack_exit, (int));
#else
#define nethack_exit exit
#endif

#ifdef EXEPATH
static char *FDECL(exepath, (char *));
#endif

int FDECL(main, (int, char **));

extern boolean FDECL(pcmain, (int, char **));

#if defined(__BORLANDC__)
void NDECL(startup);
unsigned _stklen = STKSIZ;
#endif

/* If the graphics version is built, we don't need a main; it is skipped
 * to help MinGW decide which entry point to choose. If both main and
 * WinMain exist, the resulting executable won't work correctly.
 */
int
#ifndef __MINGW32__ 
main(argc, argv)
#else
mingw_main(argc, argv)
#endif
int argc;
char *argv[];
{
    boolean resuming;

    early_init();
    resuming = pcmain(argc, argv);
    moveloop(resuming);
    nethack_exit(EXIT_SUCCESS);
    /*NOTREACHED*/
    return 0;
}

boolean
pcmain(argc, argv)
int argc;
char *argv[];
{
    NHFILE *nhfp;
    register char *dir;
#if defined(MSDOS)
    char *envp = NULL;
    char *sptr = NULL;
#endif
#ifdef NOCWD_ASSUMPTIONS
    char failbuf[BUFSZ];
#endif
    boolean resuming = FALSE; /* assume new game */

#ifdef _MSC_VER
# ifdef DEBUG
    /* set these appropriately for VS debugging */
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ERROR,
                      _CRTDBG_MODE_DEBUG); /* | _CRTDBG_MODE_FILE);*/
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
/*| _CRTDBG_MODE_FILE | _CRTDBG_MODE_WNDW);*/
/* use STDERR by default
_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);*/
/* Heap Debugging
    _CrtSetDbgFlag( _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)
        | _CRTDBG_ALLOC_MEM_DF
        | _CRTDBG_CHECK_ALWAYS_DF
        | _CRTDBG_CHECK_CRT_DF
        | _CRTDBG_DELAY_FREE_MEM_DF
        | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetBreakAlloc(1423);
*/
# endif
#endif

#if defined(__BORLANDC__)
    startup();
#endif

#ifdef TOS
    long clock_time;
    if (*argv[0]) { /* only a CLI can give us argv[0] */
        g.hname = argv[0];
        run_from_desktop = FALSE;
    } else
#endif
        g.hname = "NetHack"; /* used for syntax messages */

    choose_windows(DEFAULT_WINDOW_SYS);

#if !defined(AMIGA) && !defined(GNUDOS)
    /* Save current directory and make sure it gets restored when
     * the game is exited.
     */
    if (getcwd(orgdir, sizeof orgdir) == (char *) 0)
        error("NetHack: current directory path too long");
#ifndef NO_SIGNAL
    signal(SIGINT,
           (SIG_RET_TYPE) nethack_exit); /* restore original directory */
#endif
#endif /* !AMIGA && !GNUDOS */

    dir = nh_getenv("NETHACKDIR");
    if (dir == (char *) 0)
        dir = nh_getenv("HACKDIR");
#ifdef EXEPATH
    if (dir == (char *) 0)
        dir = exepath(argv[0]);
#endif
#ifdef _MSC_VER
    if (IsDebuggerPresent()) {
        static char exepath[_MAX_PATH];
        /* check if we're running under the debugger so we can get to the right folder anyway */
        if (dir != (char *)0) {
            char *top = (char *)0;

            if (strlen(dir) < (_MAX_PATH - 1))
                strcpy(exepath, dir);
            top = strstr(exepath, "\\build\\.\\Debug");
            if (!top) top = strstr(exepath, "\\build\\.\\Release");
            if (top) {
                *top = '\0';
                if (strlen(exepath) < (_MAX_PATH - (strlen("\\binary\\") + 1))) {
                    Strcat(exepath, "\\binary\\");
                    if (strlen(exepath) < (PATHLEN - 1)) {
                        dir = exepath;
                    }
                }
            }
        }
    }
#endif
    if (dir != (char *)0) {
        int fd;
        boolean have_syscf = FALSE;

        (void) strncpy(g.hackdir, dir, PATHLEN - 1);
        g.hackdir[PATHLEN - 1] = '\0';
#ifdef NOCWD_ASSUMPTIONS
        {
            int prefcnt;

            g.fqn_prefix[0] = (char *) alloc(strlen(g.hackdir) + 2);
            Strcpy(g.fqn_prefix[0], g.hackdir);
            append_slash(g.fqn_prefix[0]);
            for (prefcnt = 1; prefcnt < PREFIX_COUNT; prefcnt++)
                g.fqn_prefix[prefcnt] = g.fqn_prefix[0];

#if defined(MSDOS)
            /* sysconf should be searched for in this location */
            envp = nh_getenv("COMMONPROGRAMFILES");
            if (envp) {
                if ((sptr = index(envp, ';')) != 0)
                    *sptr = '\0';
                if (strlen(envp) > 0) {
                    g.fqn_prefix[SYSCONFPREFIX] =
                        (char *) alloc(strlen(envp) + 10);
                    Strcpy(g.fqn_prefix[SYSCONFPREFIX], envp);
                    append_slash(g.fqn_prefix[SYSCONFPREFIX]);
                    Strcat(g.fqn_prefix[SYSCONFPREFIX], "NetHack\\");
                }
            }

            /* okay so we have the overriding and definitive locaton
            for sysconf, but only in the event that there is not a 
            sysconf file there (for whatever reason), check a secondary
            location rather than abort. */

            /* Is there a SYSCF_FILE there? */
            fd = open(fqname(SYSCF_FILE, SYSCONFPREFIX, 0), O_RDONLY);
            if (fd >= 0) {
                /* readable */
                close(fd);
                have_syscf = TRUE;
            }

            if (!have_syscf) {
                /* No SYSCF_FILE where there should be one, and
                   without an installer, a user may not be able
                   to place one there. So, let's try somewhere else... */
                g.fqn_prefix[SYSCONFPREFIX] = g.fqn_prefix[0];

                /* Is there a SYSCF_FILE there? */
                fd = open(fqname(SYSCF_FILE, SYSCONFPREFIX, 0), O_RDONLY);
                if (fd >= 0) {
                    /* readable */
                    close(fd);
                    have_syscf = TRUE;
                }
            }

            /* user's home directory should default to this - unless
             * overridden */
            envp = nh_getenv("USERPROFILE");
            if (envp) {
                if ((sptr = index(envp, ';')) != 0)
                    *sptr = '\0';
                if (strlen(envp) > 0) {
                    g.fqn_prefix[CONFIGPREFIX] =
                        (char *) alloc(strlen(envp) + 2);
                    Strcpy(g.fqn_prefix[CONFIGPREFIX], envp);
                    append_slash(g.fqn_prefix[CONFIGPREFIX]);
                }
            }
#endif
        }
#endif
#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
        chdirx(dir, 1);
#endif
    }
#ifdef AMIGA
#ifdef CHDIR
    /*
     * If we're dealing with workbench, change the directory.  Otherwise
     * we could get "Insert disk in drive 0" messages. (Must be done
     * before initoptions())....
     */
    if (argc == 0)
        chdirx(HACKDIR, 1);
#endif
    ami_wininit_data();
#endif
    initoptions();

#ifdef NOCWD_ASSUMPTIONS
    if (!validate_prefix_locations(failbuf)) {
        raw_printf("Some invalid directory locations were specified:\n\t%s\n",
                   failbuf);
        nethack_exit(EXIT_FAILURE);
    }
#endif

#if defined(TOS) && defined(TEXTCOLOR)
    if (iflags.BIOS && iflags.use_color)
        set_colors();
#endif
    if (!g.hackdir[0])
#if !defined(LATTICE) && !defined(AMIGA)
        Strcpy(g.hackdir, orgdir);
#else
        Strcpy(g.hackdir, HACKDIR);
#endif
    if (argc > 1) {
        if (argcheck(argc, argv, ARG_VERSION) == 2)
            nethack_exit(EXIT_SUCCESS);

        if (argcheck(argc, argv, ARG_DEBUG) == 1) {
            argc--;
            argv++;
	}

        if (argc > 1 && !strncmp(argv[1], "-d", 2) && argv[1][2] != 'e') {
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
            Strcpy(g.hackdir, dir);
        }
        if (argc > 1) {
            /*
             * Now we know the directory containing 'record' and
             * may do a prscore().
             */
            if (!strncmp(argv[1], "-s", 2)) {
#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
                chdirx(g.hackdir, 0);
#endif
#ifdef SYSCF
                initoptions();
#endif
                prscore(argc, argv);
                nethack_exit(EXIT_SUCCESS);
            }

            /* Don't initialize the window system just to print usage */
            if (!strncmp(argv[1], "-?", 2) || !strncmp(argv[1], "/?", 2)) {
                nhusage();
                nethack_exit(EXIT_SUCCESS);
            }
        }
    }
/*
 * It seems you really want to play.
 */
#ifdef TOS
    if (comp_times((long) time(&clock_time)))
        error("Your clock is incorrectly set!");
#endif
    if (!dlb_init()) {
        pline(
            "%s\n%s\n%s\n%s\n\nNetHack was unable to open the required file "
            "\"%s\".%s",
            copyright_banner_line(1), copyright_banner_line(2),
            copyright_banner_line(3), copyright_banner_line(4), DLBFILE,
            "");
        error("dlb_init failure.");
    }

    u.uhp = 1; /* prevent RIP on early quits */
    u.ux = 0;  /* prevent flush_screen() */

/* chdir shouldn't be called before this point to keep the
 * code parallel to other ports.
 */
#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
    chdirx(g.hackdir, 1);
#endif

#if defined(MSDOS)
    /* In 3.6.0, several ports process options before they init
     * the window port. This allows settings that impact window
     * ports to be specified or read from the sys or user config files.
     */
    process_options(argc, argv);

#endif /* MSDOS */

#if defined(MSDOS)
    init_nhwindows(&argc, argv);
#else
    init_nhwindows(&argc, argv);
    process_options(argc, argv);
#endif

#ifdef MFLOPPY
    set_lock_and_bones();
#ifndef AMIGA
    copybones(FROMPERM);
#endif
#endif

    /* strip role,race,&c suffix; calls askname() if plname[] is empty
       or holds a generic user name like "player" or "games" */
    plnamesuffix();
    set_playmode(); /* sets plname to "wizard" for wizard mode */
#if 0
    /* unlike Unix where the game might be invoked with a script
       which forces a particular character name for each player
       using a shared account, we always allow player to rename
       the character during role/race/&c selection */
    iflags.renameallowed = TRUE;
#else
    /* until the getlock code is resolved, override askname()'s
       setting of renameallowed; when False, player_selection()
       won't resent renaming as an option */
    iflags.renameallowed = FALSE;
#endif

#if defined(PC_LOCKING)
/* 3.3.0 added this to support detection of multiple games
 * under the same plname on the same machine in a windowed
 * or multitasking environment.
 *
 * That allows user confirmation prior to overwriting the
 * level files of a game in progress.
 *
 * Also prevents an aborted game's level files from being
 * overwritten without confirmation when a user starts up
 * another game with the same player name.
 */
    Strcpy(g.lock, g.plname);
    regularize(g.lock);
    getlock();
#else        /* What follows is !PC_LOCKING */
#ifdef AMIGA /* We'll put the bones & levels in the user specified directory \
                -jhsa */
    Strcat(g.lock, g.plname);
    Strcat(g.lock, ".99");
#else
#ifndef MFLOPPY
    /* I'm not sure what, if anything, is left here, but MFLOPPY has
     * conflicts with set_lock_and_bones() in files.c.
     */
    Strcpy(g.lock, g.plname);
    Strcat(g.lock, ".99");
    regularize(g.lock); /* is this necessary? */
                      /* not compatible with full path a la AMIGA */
#endif
#endif
#endif /* PC_LOCKING */

    /* Set up level 0 file to keep the game state.
     */
    nhfp = create_levelfile(0, (char *) 0);
    if (!nhfp) {
        raw_print("Cannot create lock file");
    } else {
        g.hackpid = 1;
        if (nhfp->structlevel)
            write(nhfp->fd, (genericptr_t) &g.hackpid, sizeof(g.hackpid));
        close_nhfile(nhfp);
    }
#ifdef MFLOPPY
    level_info[0].where = ACTIVE;
#endif

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
    if ((nhfp = restore_saved_game()) > 0) {
#ifndef NO_SIGNAL
        (void) signal(SIGINT, (SIG_RET_TYPE) done1);
#endif
#ifdef NEWS
        if (iflags.news) {
            display_file(NEWS, FALSE);
            iflags.news = FALSE;
        }
#endif
        pline("Restoring save file...");
        mark_synch(); /* flush output */

        if (dorecover(nhfp)) {
            resuming = TRUE; /* not starting new game */
            if (discover)
                You("are in non-scoring discovery mode.");
            if (discover || wizard) {
                if (yn("Do you want to keep the save file?") == 'n')
                    (void) delete_savefile();
                else {
                    nh_compress(fqname(g.SAVEF, SAVEPREFIX, 0));
                }
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
   discard current lock file and create another for
   the new character name */
#if 0 /* this needs to be reconciled with the getlock mess above... */
            delete_levelfile(0); /* remove empty lock file */
            getlock();
#endif
                goto attempt_restore;
            }
        }
        newgame();
        if (discover)
            You("are in non-scoring discovery mode.");
    }

#ifndef NO_SIGNAL
    (void) signal(SIGINT, SIG_IGN);
#endif
#ifdef OS2
    gettty(); /* somehow ctrl-P gets turned back on during startup ... */
#endif
    return resuming;
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
        case 'a':
            if (argv[0][2]) {
                if ((i = str2align(&argv[0][2])) >= 0)
                    flags.initalign = i;
            } else if (argc > 1) {
                argc--;
                argv++;
                if ((i = str2align(argv[0])) >= 0)
                    flags.initalign = i;
            }
            break;
        case 'D':
            wizard = TRUE, discover = FALSE;
            break;
        case 'X':
            discover = TRUE, wizard = FALSE;
            break;
#ifdef NEWS
        case 'n':
            iflags.news = FALSE;
            break;
#endif
        case 'u':
            if (argv[0][2])
                (void) strncpy(g.plname, argv[0] + 2, sizeof(g.plname) - 1);
            else if (argc > 1) {
                argc--;
                argv++;
                (void) strncpy(g.plname, argv[0], sizeof(g.plname) - 1);
            } else
                raw_print("Player name expected after -u");
            break;
#ifndef AMIGA
        case 'I':
        case 'i':
            if (!strncmpi(argv[0] + 1, "IBM", 3)) {
                load_symset("IBMGraphics", PRIMARY);
                load_symset("RogueIBM", ROGUESET);
                switch_symbols(TRUE);
            }
            break;
        /*	case 'D': */
        case 'd':
            if (!strncmpi(argv[0] + 1, "DEC", 3)) {
                load_symset("DECGraphics", PRIMARY);
                switch_symbols(TRUE);
            }
            break;
#endif
        case 'g':
            if (argv[0][2]) {
                if ((i = str2gend(&argv[0][2])) >= 0)
                    flags.initgend = i;
            } else if (argc > 1) {
                argc--;
                argv++;
                if ((i = str2gend(argv[0])) >= 0)
                    flags.initgend = i;
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
#ifdef MFLOPPY
#ifndef AMIGA
        /* Player doesn't want to use a RAM disk
         */
        case 'R':
            g.ramdisk = FALSE;
            break;
#endif
#endif
#ifdef AMIGA
        /* interlaced and non-interlaced screens */
        case 'L':
            bigscreen = 1;
            break;
        case 'l':
            bigscreen = -1;
            break;
#endif
        case '@':
            flags.randomall = 1;
            break;
        default:
            if ((i = str2role(&argv[0][1])) >= 0) {
                flags.initrole = i;
                break;
            } else
                raw_printf("\nUnknown switch: %s", argv[0]);
        /* FALL THROUGH */
        case '?':
            nhusage();
            nethack_exit(EXIT_SUCCESS);
        }
    }
}

static void
nhusage()
{
    char buf1[BUFSZ], buf2[BUFSZ], *bufptr;

    buf1[0] = '\0';
    bufptr = buf1;

#define ADD_USAGE(s)                              \
    if ((strlen(buf1) + strlen(s)) < (BUFSZ - 1)) \
        Strcat(bufptr, s);

    /* -role still works for those cases which aren't already taken, but
     * is deprecated and will not be listed here.
     */
    (void) Sprintf(buf2, "\nUsage:\n%s [-d dir] -s [-r race] [-p profession] "
                         "[maxrank] [name]...\n       or",
                   g.hname);
    ADD_USAGE(buf2);

    (void) Sprintf(
        buf2, "\n%s [-d dir] [-u name] [-r race] [-p profession] [-[DX]]",
        g.hname);
    ADD_USAGE(buf2);
#ifdef NEWS
    ADD_USAGE(" [-n]");
#endif
#ifndef AMIGA
    ADD_USAGE(" [-I] [-i] [-d]");
#endif
#ifdef MFLOPPY
#ifndef AMIGA
    ADD_USAGE(" [-R]");
#endif
#endif
#ifdef AMIGA
    ADD_USAGE(" [-[lL]]");
#endif
    if (!iflags.window_inited)
        raw_printf("%s\n", buf1);
    else
        (void) printf("%s\n", buf1);
#undef ADD_USAGE
}

#ifdef CHDIR
void
chdirx(dir, wr)
char *dir;
boolean wr;
{
#ifdef AMIGA
    static char thisdir[] = "";
#else
    static char thisdir[] = ".";
#endif
    if (dir && chdir(dir) < 0) {
        error("Cannot chdir to %s.", dir);
    }

#ifndef AMIGA
    /* Change the default drive as well.
     */
    chdrive(dir);
#endif

    /* warn the player if we can't write the record file */
    /* perhaps we should also test whether . is writable */
    /* unfortunately the access system-call is worthless */
    if (wr)
        check_recordfile(dir ? dir : thisdir);
}
#endif /* CHDIR */

#ifdef PORT_HELP
#if defined(MSDOS)
void
port_help()
{
    /* display port specific help file */
    display_file(PORT_HELP, 1);
}
#endif /* MSDOS */
#endif /* PORT_HELP */

/* validate wizard mode if player has requested access to it */
boolean
authorize_wizard_mode()
{
    if (!strcmp(g.plname, WIZARD_NAME))
        return TRUE;
    return FALSE;
}

#ifdef EXEPATH
#ifdef __DJGPP__
#define PATH_SEPARATOR '/'
#else
#define PATH_SEPARATOR '\\'
#endif

#define EXEPATHBUFSZ 256
char exepathbuf[EXEPATHBUFSZ];

char *
exepath(str)
char *str;
{
    char *tmp, *tmp2;
    int bsize;

    if (!str)
        return (char *) 0;
    bsize = EXEPATHBUFSZ;
    tmp = exepathbuf;
    Strcpy(tmp, str);
    tmp2 = strrchr(tmp, PATH_SEPARATOR);
    if (tmp2)
        *tmp2 = '\0';
    return tmp;
}
#endif /* EXEPATH */

/*pcmain.c*/
