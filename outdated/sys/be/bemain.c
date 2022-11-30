/* NetHack 3.6	bemain.c	$NHDT-Date: 1447844549 2015/11/18 11:02:29 $  $NHDT-Branch: master $:$NHDT-Revision: 1.18 $ */
/* Copyright (c) Dean Luick, 1996. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "dlb.h"
#include <fcntl.h>

static void whoami(void);
static void process_options(int argc, char **argv);
static void chdirx(const char *dir);
static void getlock(void);

#ifdef __begui__
#define MAIN nhmain
int nhmain(int argc, char **argv);
#else
#define MAIN main
#endif

int
MAIN(int argc, char **argv)
{
    int fd;
    char *dir;
    boolean resuming = FALSE; /* assume new game */

    early_init();

    dir = nh_getenv("NETHACKDIR");
    if (!dir)
        dir = nh_getenv("HACKDIR");

    choose_windows(DEFAULT_WINDOW_SYS);
    chdirx(dir);
    initoptions();

    init_nhwindows(&argc, argv);
    whoami();

    /*
     * It seems you really want to play.
     */
    u.uhp = 1;                   /* prevent RIP on early quits */
    process_options(argc, argv); /* command line options */

    set_playmode(); /* sets plname to "wizard" for wizard mode */
    /* strip role,race,&c suffix; calls askname() if plname[] is empty
       or holds a generic user name like "player" or "games" */
    plnamesuffix();
    /* unlike Unix where the game might be invoked with a script
       which forces a particular character name for each player
       using a shared account, we always allow player to rename
       the character during role/race/&c selection */
    iflags.renameallowed = TRUE;

    getlock();

    dlb_init(); /* must be before newgame() */

    /*
     * Initialize the vision system.  This must be before mklev() on a
     * new game or before a level restore on a saved game.
     */
    vision_init();

    display_gamewindows();

    /*
     * First, try to find and restore a save file for specified character.
     * We'll return here if new game player_selection() renames the hero.
     */
attempt_restore:
    if ((fd = restore_saved_game()) >= 0) {
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
            if (discover)
                You("are in non-scoring discovery mode.");
            if (discover || wizard) {
                if (yn("Do you want to keep the save file?") == 'n')
                    (void) delete_savefile();
                else {
                    nh_compress(fqname(gs.SAVEF, SAVEPREFIX, 0));
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
                delete_levelfile(0); /* remove empty lock file */
                getlock();
                goto attempt_restore;
            }
        }
        newgame();
        if (discover)
            You("are in non-scoring discovery mode.");
    }

    moveloop(resuming);
    return 0;
}

static void
whoami(void)
{
    /*
     * Who am i? Algorithm: 1. Use name as specified in NETHACKOPTIONS
     *                      2. Use $USER or $LOGNAME        (if 1. fails)
     * The resulting name is overridden by command line options.
     * If everything fails, or if the resulting name is some generic
     * account like "games", "play", "player", "hack" then eventually
     * we'll ask him.
     */
    char *s;

    if (*gp.plname)
        return;
    if (s = nh_getenv("USER")) {
        (void) strncpy(gp.plname, s, sizeof(gp.plname) - 1);
        return;
    }
    if (s = nh_getenv("LOGNAME")) {
        (void) strncpy(gp.plname, s, sizeof(gp.plname) - 1);
        return;
    }
}

/* normalize file name - we don't like .'s, /'s, spaces */
void
regularize(char *s)
{
    register char *lp;

    while ((lp = strchr(s, '.')) || (lp = strchr(s, '/'))
           || (lp = strchr(s, ' ')))
        *lp = '_';
}

static void
process_options(int argc, char **argv)
{
    int i;

    while (argc > 1 && argv[1][0] == '-') {
        argv++;
        argc--;
        switch (argv[0][1]) {
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
                (void) strncpy(gp.plname, argv[0] + 2, sizeof(gp.plname) - 1);
            else if (argc > 1) {
                argc--;
                argv++;
                (void) strncpy(gp.plname, argv[0], sizeof(gp.plname) - 1);
            } else
                raw_print("Player name expected after -u");
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
            raw_printf("Unknown option: %s", *argv);
            break;
        }
    }
}

static void
chdirx(const char *dir)
{
    if (!dir)
        dir = HACKDIR;

    if (chdir(dir) < 0)
        error("Cannot chdir to %s.", dir);

    /* Warn the player if we can't write the record file */
    /* perhaps we should also test whether . is writable */
    check_recordfile(dir);
}

void
getlock(void)
{
    int fd;

    Sprintf(gl.lock, "%d%s", getuid(), gp.plname);
    regularize(gl.lock);
    set_levelfile_name(gl.lock, 0);
    fd = creat(gl.lock, FCMASK);
    if (fd == -1) {
        error("cannot creat lock file.");
    } else {
        if (write(fd, (genericptr_t) &gh.hackpid, sizeof(gh.hackpid))
            != sizeof(gh.hackpid)) {
            error("cannot write lock");
        }
        if (close(fd) == -1) {
            error("cannot close lock");
        }
    }
}

/* validate wizard mode if player has requested access to it */
boolean
authorize_wizard_mode()
{
    /* other ports validate user name or character name here */
    return TRUE;
}

#ifndef __begui__
/*
 * If we are not using the Be GUI, then just exit -- we don't need to
 * do anything extra.
 */
void nethack_exit(int status);

void
nethack_exit(int status)
{
    exit(status);
}
#endif /* !__begui__ */

/*bemain.c*/
