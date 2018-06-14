/* NetHack 3.6	unixmain.c	$NHDT-Date: 1432512788 2015/05/25 00:13:08 $  $NHDT-Branch: master $:$NHDT-Revision: 1.52 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

/* main.c - Unix NetHack */

#include "hack.h"
#include "dlb.h"

#include <ctype.h>
#include <sys/stat.h>
#include <signal.h>
#include <pwd.h>
#ifndef O_RDONLY
#include <fcntl.h>
#endif

#if !defined(_BULL_SOURCE) && !defined(__sgi) && !defined(_M_UNIX)
#if !defined(SUNOS4) && !(defined(ULTRIX) && defined(__GNUC__))
#if defined(POSIX_TYPES) || defined(SVR4) || defined(HPUX)
extern struct passwd *FDECL(getpwuid, (uid_t));
#else
extern struct passwd *FDECL(getpwuid, (int));
#endif
#endif
#endif
extern struct passwd *FDECL(getpwnam, (const char *));
#ifdef CHDIR
static void FDECL(chdirx, (const char *, BOOLEAN_P));
#endif /* CHDIR */
static boolean NDECL(whoami);
static void FDECL(process_options, (int, char **));

#ifdef _M_UNIX
extern void NDECL(check_sco_console);
extern void NDECL(init_sco_cons);
#endif
#ifdef __linux__
extern void NDECL(check_linux_console);
extern void NDECL(init_linux_cons);
#endif

static void NDECL(wd_message);
static boolean wiz_error_flag = FALSE;
static struct passwd *NDECL(get_unix_pw);

int
main(argc, argv)
int argc;
char *argv[];
{
    register int fd;
#ifdef CHDIR
    register char *dir;
#endif
    boolean exact_username;
    boolean resuming = FALSE; /* assume new game */
    boolean plsel_once = FALSE;

    sys_early_init();

#if defined(__APPLE__)
    {
/* special hack to change working directory to a resource fork when
   running from finder --sam */
#define MAC_PATH_VALUE ".app/Contents/MacOS/"
        char mac_cwd[1024], *mac_exe = argv[0], *mac_tmp;
        int arg0_len = strlen(mac_exe), mac_tmp_len, mac_lhs_len = 0;
        getcwd(mac_cwd, 1024);
        if (mac_exe[0] == '/' && !strcmp(mac_cwd, "/")) {
            if ((mac_exe = strrchr(mac_exe, '/')))
                mac_exe++;
            else
                mac_exe = argv[0];
            mac_tmp_len = (strlen(mac_exe) * 2) + strlen(MAC_PATH_VALUE);
            if (mac_tmp_len <= arg0_len) {
                mac_tmp = malloc(mac_tmp_len + 1);
                sprintf(mac_tmp, "%s%s%s", mac_exe, MAC_PATH_VALUE, mac_exe);
                if (!strcmp(argv[0] + (arg0_len - mac_tmp_len), mac_tmp)) {
                    mac_lhs_len =
                        (arg0_len - mac_tmp_len) + strlen(mac_exe) + 5;
                    if (mac_lhs_len > mac_tmp_len - 1)
                        mac_tmp = realloc(mac_tmp, mac_lhs_len);
                    strncpy(mac_tmp, argv[0], mac_lhs_len);
                    mac_tmp[mac_lhs_len] = '\0';
                    chdir(mac_tmp);
                }
                free(mac_tmp);
            }
        }
    }
#endif

    hname = argv[0];
    hackpid = getpid();
    (void) umask(0777 & ~FCMASK);

    choose_windows(DEFAULT_WINDOW_SYS);

#ifdef CHDIR /* otherwise no chdir() */
    /*
     * See if we must change directory to the playground.
     * (Perhaps hack runs suid and playground is inaccessible
     *  for the player.)
     * The environment variable HACKDIR is overridden by a
     *  -d command line option (must be the first option given).
     */
    dir = nh_getenv("NETHACKDIR");
    if (!dir)
        dir = nh_getenv("HACKDIR");

    if (argc > 1) {
        if (argcheck(argc, argv, ARG_VERSION) == 2)
            exit(EXIT_SUCCESS);

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
        }
    }
#endif /* CHDIR */

    if (argc > 1) {
        /*
         * Now we know the directory containing 'record' and
         * may do a prscore().  Exclude `-style' - it's a Qt option.
         */
        if (!strncmp(argv[1], "-s", 2) && strncmp(argv[1], "-style", 6)) {
#ifdef CHDIR
            chdirx(dir, 0);
#endif
#ifdef SYSCF
            initoptions();
#endif
#ifdef PANICTRACE
            ARGV0 = hname; /* save for possible stack trace */
#ifndef NO_SIGNAL
            panictrace_setsignals(TRUE);
#endif
#endif
            prscore(argc, argv);
            /* FIXME: shouldn't this be using nh_terminate() to free
               up any memory allocated by initoptions() */
            exit(EXIT_SUCCESS);
        }
    } /* argc > 1 */

/*
 * Change directories before we initialize the window system so
 * we can find the tile file.
 */
#ifdef CHDIR
    chdirx(dir, 1);
#endif

#ifdef _M_UNIX
    check_sco_console();
#endif
#ifdef __linux__
    check_linux_console();
#endif
    initoptions();
#ifdef PANICTRACE
    ARGV0 = hname; /* save for possible stack trace */
#ifndef NO_SIGNAL
    panictrace_setsignals(TRUE);
#endif
#endif
    exact_username = whoami();

    /*
     * It seems you really want to play.
     */
    u.uhp = 1; /* prevent RIP on early quits */
    program_state.preserve_locks = 1;
#ifndef NO_SIGNAL
    sethanguphandler((SIG_RET_TYPE) hangup);
#endif

    process_options(argc, argv); /* command line options */
#ifdef WINCHAIN
    commit_windowchain();
#endif
    init_nhwindows(&argc, argv); /* now we can set up window system */
#ifdef _M_UNIX
    init_sco_cons();
#endif
#ifdef __linux__
    init_linux_cons();
#endif

#ifdef DEF_PAGER
    if (!(catmore = nh_getenv("HACKPAGER"))
        && !(catmore = nh_getenv("PAGER")))
        catmore = DEF_PAGER;
#endif
#ifdef MAIL
    getmailstatus();
#endif

    /* wizard mode access is deferred until here */
    set_playmode(); /* sets plname to "wizard" for wizard mode */
    if (exact_username) {
        /*
         * FIXME: this no longer works, ever since 3.3.0
         * when plnamesuffix() was changed to find
         * Name-Role-Race-Gender-Alignment.  It removes
         * all dashes rather than just the last one,
         * regardless of whether whatever follows each
         * dash matches role, race, gender, or alignment.
         */
        /* guard against user names with hyphens in them */
        int len = (int) strlen(plname);
        /* append the current role, if any, so that last dash is ours */
        if (++len < (int) sizeof plname)
            (void) strncat(strcat(plname, "-"), pl_character,
                           sizeof plname - len - 1);
    }
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
    if (*plname) {
        getlock();
        program_state.preserve_locks = 0; /* after getlock() */
    }

    if (*plname && (fd = restore_saved_game()) >= 0) {
        const char *fq_save = fqname(SAVEF, SAVEPREFIX, 1);

        (void) chmod(fq_save, 0); /* disallow parallel restores */
#ifndef NO_SIGNAL
        (void) signal(SIGINT, (SIG_RET_TYPE) done1);
#endif
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
                /* this seems like a candidate for paranoid_confirmation... */
                if (yn("Do you want to keep the save file?") == 'n') {
                    (void) delete_savefile();
                } else {
                    (void) chmod(fq_save, FCMASK); /* back to readable */
                    nh_compress(fq_save);
                }
            }
        }
    }

    if (!resuming) {
        boolean neednewlock = (!*plname);
        /* new game:  start by choosing role, race, etc;
           player might change the hero's name while doing that,
           in which case we try to restore under the new name
           and skip selection this time if that didn't succeed */
        if (!iflags.renameinprogress || iflags.defer_plname || neednewlock) {
            if (!plsel_once)
                player_selection();
            plsel_once = TRUE;
            if (neednewlock && *plname)
                goto attempt_restore;
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

    /* moveloop() never returns but isn't flagged NORETURN */
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
    int i, l;

    /*
     * Process options.
     */
    while (argc > 1 && argv[1][0] == '-') {
        argv++;
        argc--;
        l = (int) strlen(*argv);
        /* must supply at least 4 chars to match "-XXXgraphics" */
        if (l < 4)
            l = 4;

        switch (argv[0][1]) {
        case 'D':
        case 'd':
            if ((argv[0][1] == 'D' && !argv[0][2])
                || !strcmpi(*argv, "-debug")) {
                wizard = TRUE, discover = FALSE;
            } else if (!strncmpi(*argv, "-DECgraphics", l)) {
                load_symset("DECGraphics", PRIMARY);
                switch_symbols(TRUE);
            } else {
                raw_printf("Unknown option: %s", *argv);
            }
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
            if (argv[0][2]) {
                (void) strncpy(plname, argv[0] + 2, sizeof plname - 1);
            } else if (argc > 1) {
                argc--;
                argv++;
                (void) strncpy(plname, argv[0], sizeof plname - 1);
            } else {
                raw_print("Player name expected after -u");
            }
            break;
        case 'I':
        case 'i':
            if (!strncmpi(*argv, "-IBMgraphics", l)) {
                load_symset("IBMGraphics", PRIMARY);
                load_symset("RogueIBM", ROGUESET);
                switch_symbols(TRUE);
            } else {
                raw_printf("Unknown option: %s", *argv);
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
        case 'w': /* windowtype */
            config_error_init(FALSE, "command line", FALSE);
            choose_windows(&argv[0][2]);
            config_error_done();
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

#ifdef SYSCF
    if (argc > 1)
        raw_printf("MAXPLAYERS are set in sysconf file.\n");
#else
    /* XXX This is deprecated in favor of SYSCF with MAXPLAYERS */
    if (argc > 1)
        locknum = atoi(argv[1]);
#endif
#ifdef MAX_NR_OF_PLAYERS
    /* limit to compile-time limit */
    if (!locknum || locknum > MAX_NR_OF_PLAYERS)
        locknum = MAX_NR_OF_PLAYERS;
#endif
#ifdef SYSCF
    /* let syscf override compile-time limit */
    if (!locknum || (sysopt.maxplayers && locknum > sysopt.maxplayers))
        locknum = sysopt.maxplayers;
#endif
}

#ifdef CHDIR
static void
chdirx(dir, wr)
const char *dir;
boolean wr;
{
    if (dir /* User specified directory? */
#ifdef HACKDIR
        && strcmp(dir, HACKDIR) /* and not the default? */
#endif
        ) {
#ifdef SECURE
        (void) setgid(getgid());
        (void) setuid(getuid()); /* Ron Wessels */
#endif
    } else {
        /* non-default data files is a sign that scores may not be
         * compatible, or perhaps that a binary not fitting this
         * system's layout is being used.
         */
#ifdef VAR_PLAYGROUND
        int len = strlen(VAR_PLAYGROUND);

        fqn_prefix[SCOREPREFIX] = (char *) alloc(len + 2);
        Strcpy(fqn_prefix[SCOREPREFIX], VAR_PLAYGROUND);
        if (fqn_prefix[SCOREPREFIX][len - 1] != '/') {
            fqn_prefix[SCOREPREFIX][len] = '/';
            fqn_prefix[SCOREPREFIX][len + 1] = '\0';
        }
#endif
    }

#ifdef HACKDIR
    if (dir == (const char *) 0)
        dir = HACKDIR;
#endif

    if (dir && chdir(dir) < 0) {
        perror(dir);
        error("Cannot chdir to %s.", dir);
    }

    /* warn the player if we can't write the record file
     * perhaps we should also test whether . is writable
     * unfortunately the access system-call is worthless.
     */
    if (wr) {
#ifdef VAR_PLAYGROUND
        fqn_prefix[LEVELPREFIX] = fqn_prefix[SCOREPREFIX];
        fqn_prefix[SAVEPREFIX] = fqn_prefix[SCOREPREFIX];
        fqn_prefix[BONESPREFIX] = fqn_prefix[SCOREPREFIX];
        fqn_prefix[LOCKPREFIX] = fqn_prefix[SCOREPREFIX];
        fqn_prefix[TROUBLEPREFIX] = fqn_prefix[SCOREPREFIX];
#endif
        check_recordfile(dir);
    }
}
#endif /* CHDIR */

/* returns True iff we set plname[] to username which contains a hyphen */
static boolean
whoami()
{
    /*
     * Who am i? Algorithm: 1. Use name as specified in NETHACKOPTIONS
     *			2. Use $USER or $LOGNAME	(if 1. fails)
     *			3. Use getlogin()		(if 2. fails)
     * The resulting name is overridden by command line options.
     * If everything fails, or if the resulting name is some generic
     * account like "games", "play", "player", "hack" then eventually
     * we'll ask him.
     * Note that we trust the user here; it is possible to play under
     * somebody else's name.
     */
    if (!*plname) {
        register const char *s;

        s = nh_getenv("USER");
        if (!s || !*s)
            s = nh_getenv("LOGNAME");
        if (!s || !*s)
            s = getlogin();

        if (s && *s) {
            (void) strncpy(plname, s, sizeof plname - 1);
            if (index(plname, '-'))
                return TRUE;
        }
    }
    return FALSE;
}

void
sethanguphandler(handler)
void FDECL((*handler), (int));
{
#ifdef SA_RESTART
    /* don't want reads to restart.  If SA_RESTART is defined, we know
     * sigaction exists and can be used to ensure reads won't restart.
     * If it's not defined, assume reads do not restart.  If reads restart
     * and a signal occurs, the game won't do anything until the read
     * succeeds (or the stream returns EOF, which might not happen if
     * reading from, say, a window manager). */
    struct sigaction sact;

    (void) memset((genericptr_t) &sact, 0, sizeof sact);
    sact.sa_handler = (SIG_RET_TYPE) handler;
    (void) sigaction(SIGHUP, &sact, (struct sigaction *) 0);
#ifdef SIGXCPU
    (void) sigaction(SIGXCPU, &sact, (struct sigaction *) 0);
#endif
#else /* !SA_RESTART */
    (void) signal(SIGHUP, (SIG_RET_TYPE) handler);
#ifdef SIGXCPU
    (void) signal(SIGXCPU, (SIG_RET_TYPE) handler);
#endif
#ifdef WHEREIS_FILE
    (void) signal(SIGUSR1, (SIG_RET_TYPE) signal_whereis);
#endif
#endif /* ?SA_RESTART */
}

#ifdef PORT_HELP
void
port_help()
{
    /*
     * Display unix-specific help.   Just show contents of the helpfile
     * named by PORT_HELP.
     */
    display_file(PORT_HELP, TRUE);
}
#endif

/* validate wizard mode if player has requested access to it */
boolean
authorize_wizard_mode()
{
    struct passwd *pw = get_unix_pw();

    if (pw && sysopt.wizards && sysopt.wizards[0]) {
        if (check_user_string(sysopt.wizards))
            return TRUE;
    }
    wiz_error_flag = TRUE; /* not being allowed into wizard mode */
    return FALSE;
}

static void
wd_message()
{
    if (wiz_error_flag) {
        if (sysopt.wizards && sysopt.wizards[0]) {
            char *tmp = build_english_list(sysopt.wizards);
            pline("Only user%s %s may access debug (wizard) mode.",
                  index(sysopt.wizards, ' ') ? "s" : "", tmp);
            free(tmp);
        } else
            pline("Entering explore/discovery mode instead.");
        wizard = 0, discover = 1; /* (paranoia) */
    } else if (discover)
        You("are in non-scoring explore/discovery mode.");
}

/*
 * Add a slash to any name not ending in /. There must
 * be room for the /
 */
void
append_slash(name)
char *name;
{
    char *ptr;

    if (!*name)
        return;
    ptr = name + (strlen(name) - 1);
    if (*ptr != '/') {
        *++ptr = '/';
        *++ptr = '\0';
    }
    return;
}

boolean
check_user_string(optstr)
char *optstr;
{
    struct passwd *pw = get_unix_pw();
    int pwlen;
    char *eop, *w;
    char *pwname;

    if (optstr[0] == '*')
        return TRUE; /* allow any user */
    if (!pw)
        return FALSE;
    if (sysopt.check_plname)
        pwname = plname;
    else
        pwname = pw->pw_name;
    pwlen = strlen(pwname);
    eop = eos(optstr);
    w = optstr;
    while (w + pwlen <= eop) {
        if (!*w)
            break;
        if (isspace(*w)) {
            w++;
            continue;
        }
        if (!strncmp(w, pwname, pwlen)) {
            if (!w[pwlen] || isspace(w[pwlen]))
                return TRUE;
        }
        while (*w && !isspace(*w))
            w++;
    }
    return FALSE;
}

static struct passwd *
get_unix_pw()
{
    char *user;
    unsigned uid;
    static struct passwd *pw = (struct passwd *) 0;

    if (pw)
        return pw; /* cache answer */

    uid = (unsigned) getuid();
    user = getlogin();
    if (user) {
        pw = getpwnam(user);
        if (pw && ((unsigned) pw->pw_uid != uid))
            pw = 0;
    }
    if (pw == 0) {
        user = nh_getenv("USER");
        if (user) {
            pw = getpwnam(user);
            if (pw && ((unsigned) pw->pw_uid != uid))
                pw = 0;
        }
        if (pw == 0) {
            pw = getpwuid(uid);
        }
    }
    return pw;
}

char *
get_login_name()
{
    static char buf[BUFSZ];
    struct passwd *pw = get_unix_pw();

    buf[0] = '\0';

    if (pw)
        (void)strcpy(buf, pw->pw_name);

    return buf;
}

#ifdef __APPLE__
extern int errno;

void
port_insert_pastebuf(buf)
char *buf;
{
    /* This should be replaced when there is a Cocoa port. */
    const char *errfmt;
    size_t len;
    FILE *PB = popen("/usr/bin/pbcopy","w");
    if(!PB){
	errfmt = "Unable to start pbcopy (%d)\n";
	goto error;
    }

    len = strlen(buf);
    /* Remove the trailing \n, carefully. */
    if(buf[len-1] == '\n') len--;

    /* XXX Sorry, I'm too lazy to write a loop for output this short. */
    if(len!=fwrite(buf,1,len,PB)){
	errfmt = "Error sending data to pbcopy (%d)\n";
	goto error;
    }

    if(pclose(PB)!=-1){
	return;
    }
    errfmt = "Error finishing pbcopy (%d)\n";

error:
    raw_printf(errfmt,strerror(errno));
}
#endif

/*unixmain.c*/
