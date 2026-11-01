/* NetHack 5.0	unixmain.c	$NHDT-Date: 1711213891 2024/03/23 17:11:31 $  $NHDT-Branch: NetHack-5.0 $:$NHDT-Revision: 1.127 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

/* main.c - Unix NetHack */

#include "hack.h"
#include "dlb.h"

#include <sys/stat.h>
#include <signal.h>
#include <pwd.h>
#ifndef O_RDONLY
#include <fcntl.h>
#endif

#ifdef NHUUID
/* for uuid */
#ifdef LINUX
#include <uuid/uuid.h>
#endif
#endif  /* NHUUID */

#if !defined(_BULL_SOURCE) && !defined(__sgi) && !defined(_M_UNIX)
#if !defined(SUNOS4) && !(defined(ULTRIX) && defined(__GNUC__))
#if defined(POSIX_TYPES) || defined(SVR4) || defined(HPUX)
extern struct passwd *getpwuid(uid_t);
#else
extern struct passwd *getpwuid(int);
#endif
#endif
#endif
extern struct passwd *getpwnam(const char *);
#ifdef CHDIR
void chdirx(const char *, boolean);
#endif /* CHDIR */
boolean whoami(void);
static void process_options(int, char **);

#ifdef SND_LIB_INTEGRATED
uint32_t soundlibchoice = soundlib_nosound;
#endif

#ifdef _M_UNIX
extern void check_sco_console(void);
extern void init_sco_cons(void);
#endif
#ifdef __linux__
extern void check_linux_console(void);
extern void init_linux_cons(void);
#endif

static void wd_message(void);
static struct passwd *get_unix_pw(void);

int
main(int argc, char *argv[])
{
    char *dir = NULL;
    NHFILE *nhfp;
    boolean exact_username;
    boolean resuming = FALSE; /* assume new game */
    boolean plsel_once = FALSE;

    early_init(argc, argv);

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

    gh.hname = argv[0];
    svh.hackpid = getpid();
    (void) umask(0777 & ~FCMASK);

    choose_windows(DEFAULT_WINDOW_SYS);

#ifdef SND_LIB_INTEGRATED
    /* One of the soundlib interfaces was integrated on build.
     * We can leave a hint here for activate_chosen_soundlib later.
     * assign_soundlib() just sets an indicator, it doesn't initialize
     * any soundlib, and the indicator could be overturned before
     * activate_chosen_soundlib() gets called. Qt will place its own
     * hint if qt_init_nhwindow() is invoked.
     */
#if defined(SND_LIB_MACSOUND)
    soundlibchoice = soundlib_macsound;
    assign_soundlib(soundlibchoice);
#endif
#endif

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
#endif /* CHDIR */
    program_state.early_options = 1;
    /* handle -dalthackdir, -s <score stuff>, --version, --showpaths */
    early_options(&argc, &argv, &dir);
    program_state.early_options = 0;
#ifdef CHDIR
    /*
     * Change directories before we initialize the window system so
     * we can find the tile file.
     */
    chdirx(dir, TRUE);
#endif
#ifdef _M_UNIX
    check_sco_console();
#endif
#ifdef __linux__
    check_linux_console();
#endif

    initoptions();
#ifdef PANICTRACE
    ARGV0 = gh.hname; /* save for possible stack trace */
#ifndef NO_SIGNAL
    panictrace_setsignals(TRUE);
#endif
#endif
    exact_username = whoami();

    /*
     * It seems you really want to play.
     */
    u.uhp = 1; /* prevent RIP on early quits */
#if defined(HANGUPHANDLING)
    program_state.preserve_locks = 1;
#ifndef NO_SIGNAL
    sethanguphandler((SIG_RET_TYPE) hangup);
#endif
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
    if (!(gc.catmore = nh_getenv("NETHACKPAGER"))
        && !(gc.catmore = nh_getenv("HACKPAGER"))
        && !(gc.catmore = nh_getenv("PAGER")))
        gc.catmore = DEF_PAGER;
#endif
#ifdef MAIL
    getmailstatus();
#endif

    /* wizard mode access is deferred until here */
    set_playmode(); /* sets plname to "wizard" for wizard mode */
    /* hide any hyphens from plnamesuffix() */
    gp.plnamelen = exact_username ? (int) strlen(svp.plname) : 0;
    /* strip role,race,&c suffix; calls askname() if plname[] is empty
       or holds a generic user name like "player" or "games" */
    plnamesuffix();

    if (wizard) {
        /* use character name rather than lock letter for file names */
        gl.locknum = 0;
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

    init_sound_disp_gamewindows();

    /*
     * First, try to find and restore a save file for specified character.
     * We'll return here if new game player_selection() renames the hero.
     */
 attempt_restore:

    /*
     * getlock() complains and quits if there is already a game
     * in progress for current character name (when gl.locknum == 0)
     * or if there are too many active games (when gl.locknum > 0).
     * When proceeding, it creates an empty <lockname>.0 file to
     * designate the current game.
     * getlock() constructs <lockname> based on the character
     * name (for !gl.locknum) or on first available of alock, block,
     * clock, &c not currently in use in the playground directory
     * (for gl.locknum > 0).
     */
    if (*svp.plname) {
        getlock();
#if defined(HANGUPHANDLING)
        program_state.preserve_locks = 0; /* after getlock() */
#endif
    }

    if (*svp.plname && (nhfp = restore_saved_game()) != 0) {
        const char *fq_save = fqname(gs.SAVEF, SAVEPREFIX, 1);

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
        /* if there are early trouble-messages issued, let's
         * not go overtop of them with a pline just yet */
        if (ge.early_raw_messages)
            raw_print("Restoring save file...");
        else
            pline("Restoring save file...");
        mark_synch(); /* flush output */
        if (dorecover(nhfp)) {
            resuming = TRUE; /* not starting new game */
            wd_message();
            if (discover || wizard) {
                /* this seems like a candidate for paranoid_confirmation... */
                if (y_n("Do you want to keep the save file?") == 'n') {
                    (void) delete_savefile();
                } else {
                    (void) chmod(fq_save, FCMASK); /* back to readable */
                    nh_compress(fq_save);
                }
            }
        }
        if (program_state.in_self_recover) {
            program_state.in_self_recover = FALSE;
        }
    }

    if (!resuming) {
        boolean neednewlock = (!*svp.plname);
        /* new game:  start by choosing role, race, etc;
           player might change the hero's name while doing that,
           in which case we try to restore under the new name
           and skip selection this time if that didn't succeed */
        if (!iflags.renameinprogress || iflags.defer_plname || neednewlock) {
            if (!plsel_once)
                player_selection();
            plsel_once = TRUE;
            if (neednewlock && *svp.plname)
                goto attempt_restore;
            if (iflags.renameinprogress) {
                /* player has renamed the hero while selecting role;
                   if locking alphabetically, the existing lock file
                   can still be used; otherwise, discard current one
                   and create another for the new character name */
                if (!gl.locknum) {
                    delete_levelfile(0); /* remove empty lock file */
                    getlock();
                }
                goto attempt_restore;
            }
        }

#ifdef CHECK_PANIC_SAVE
        /* no save file; check for a panic save; if the check finds one,
           ask the player whether to proceed with a new game; it will
           quit instead of returning if the answer isn't yes */
        if (check_panic_save())
            ask_about_panic_save();
#endif

        /* no save file; start a new game */
        newgame();
        wd_message();
    }

    /* moveloop() never returns but isn't flagged NORETURN */
    moveloop(resuming);

    exit(EXIT_SUCCESS);
    /*NOTREACHED*/
    return 0;
}

/* caveat: argv elements might be arbitrarily long */
static void
process_options(int argc, char *argv[])
{
    char *arg, *origarg;
    int i, l;

    config_error_init(FALSE, "command line", FALSE);
    /*
     * Process options.
     *
     *  We don't support "-xyz" as shortcut for "-x -y -z" and we only
     *  simulate longopts by allowing "--foo" for "-foo" when the user
     *  specifies at least 2 characters of leading substring for "foo".
     *  If "foo" takes a value, both "--foo=value" and "--foo value" work.
     */
    while (argc > 1 && argv[1][0] == '-') {
        argv++;
        argc--;
        arg = origarg = argv[0];
        /* allow second dash if arg is longer than one character */
        if (arg[0] == '-' && arg[1] == '-' && arg[2] != '\0'
            /* "--a=b" violates the "--" ok when at least 2 chars long rule */
            && (arg[3] != '\0' && arg[3] != '=' && arg[3] != ':'))
            ++arg;
        l = (int) strlen(arg);
        if (l < 6 && !strncmp(arg, "-no-", 4))
            l = 6;
        else if (l < 4)
            l = 4; /* must supply at least 4 chars to match "-XXXgraphics" */

        switch (arg[1]) {
        case 'D':
        case 'd':
            if ((arg[1] == 'D' && !arg[2]) || !strcmpi(arg, "-debug")) {
                wizard = TRUE, discover = FALSE;
            } else if (!strncmpi(arg, "-DECgraphics", l)) {
                load_symset("DECGraphics", PRIMARYSET);
                switch_symbols(TRUE);
            } else {
                config_error_add("Unknown option: %.60s", origarg);
            }
            break;
        case 'X':
            discover = TRUE, wizard = FALSE;
            break;
        case 'n':
#ifdef NEWS
            if (!arg[2] || !strcmp(arg, "-no-news")) {
                iflags.news = FALSE;
                break;
            } else if (!strcmp(arg, "-news")) {
                /* in case RC has !news, allow 'nethack -news' to override */
                iflags.news = TRUE;
                break;
            }
#endif
            break;
        case 'u':
            if (arg[2]) {
                (void) strncpy(svp.plname, arg + 2, sizeof svp.plname - 1);
                gp.plnamelen = 0; /* plname[] might have -role-race attached */
            } else if (argc > 1) {
                argc--;
                argv++;
                (void) strncpy(svp.plname, argv[0], sizeof svp.plname - 1);
                gp.plnamelen = 0;
            } else {
                config_error_add("Character name expected after -u");
            }
            break;
        case 'I':
        case 'i':
            if (!strncmpi(arg, "-IBMgraphics", l)) {
                load_symset("IBMGraphics", PRIMARYSET);
                load_symset("RogueIBM", ROGUESET);
                switch_symbols(TRUE);
            } else {
                config_error_add("Unknown option: %.60s", origarg);
            }
            break;
        case 'l':
#ifdef LIVELOG
            if(!strncmp(arg, "-loglua", 7)){
                gl.loglua = 1;
            } else
#endif
                config_error_add("Unknown option: %.60s", origarg);
            break;
        case 'p': /* profession (role) */
            if (arg[2]) {
                if ((i = str2role(&arg[2])) >= 0)
                    flags.initrole = i;
            } else if (argc > 1) {
                argc--;
                argv++;
                if ((i = str2role(argv[0])) >= 0)
                    flags.initrole = i;
            }
            break;
        case 'r': /* race */
            if (arg[2]) {
                if ((i = str2race(&arg[2])) >= 0)
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
        case '-':
            /* "--" or "--x" or "--x=y"; need at least 2 chars after the
               dashes in order to accept "--x" as an alternative to "-x";
               don't just silently ignore it */
            config_error_add("Unknown option: %.60s", origarg);
            break;
        default:
            /* default for "-x" is to play as the role that starts with "x" */
            if ((i = str2role(&argv[0][1])) >= 0) {
                flags.initrole = i;
                break;
            }
            /* else config_error_add("Unknown option: %.60s", origarg); */
        }
    }

    if (argc > 1) {
        int mxplyrs = atoi(argv[1]);
        boolean mx_ok = (mxplyrs > 0);
#ifdef SYSCF
        config_error_add("%s%s%s",
                         mx_ok ? "MAXPLAYERS are set in sysconf file"
                               : "Expected MAXPLAYERS, found \"",
                         mx_ok ? "" : argv[1], mx_ok ? "" : "\"");
#else
        /* XXX This is deprecated in favor of SYSCF with MAXPLAYERS */
        if (mx_ok)
            gl.locknum = mxplyrs;
        else
            config_error_add("Invalid MAXPLAYERS \"%s\"", argv[1]);
#endif
    }
#ifdef MAX_NR_OF_PLAYERS
    /* limit to compile-time limit */
    if (!gl.locknum || gl.locknum > MAX_NR_OF_PLAYERS)
        gl.locknum = MAX_NR_OF_PLAYERS;
#endif
#ifdef SYSCF
    /* let syscf override compile-time limit */
    if (!gl.locknum || (sysopt.maxplayers && gl.locknum > sysopt.maxplayers))
        gl.locknum = sysopt.maxplayers;
#endif
    /* empty or "N errors on command line" */
    config_error_done();
    return;
}

#ifdef CHDIR
void
chdirx(const char *dir, boolean wr)
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

        /* FIXME: this allocation never gets freed.
         */
        gf.fqn_prefix[SCOREPREFIX] = (char *) alloc(len + 2);
        Strcpy(gf.fqn_prefix[SCOREPREFIX], VAR_PLAYGROUND);
        if (gf.fqn_prefix[SCOREPREFIX][len - 1] != '/') {
            gf.fqn_prefix[SCOREPREFIX][len] = '/';
            gf.fqn_prefix[SCOREPREFIX][len + 1] = '\0';
        }
#endif
    }

#ifdef HACKDIR
    if (!dir)
        dir = HACKDIR;
#endif

    if (dir && chdir(dir) < 0) {
        perror(dir);
        error("Cannot chdir to %s.", dir);
        /*NOTREACHED*/
    }

    /* warn the player if we can't write the record file
     * perhaps we should also test whether . is writable
     * unfortunately the access system-call is worthless.
     */
    if (wr) {
#ifdef VAR_PLAYGROUND
        /* FIXME: if termination cleanup ever frees fqn_prefix[0..N-1],
         * these will need to use dupstr() so that they have distinct
         * values that can be freed separately.  Or perhaps freeing
         * fqn_prefix[j] can check [j+1] through [N-1] for duplicated
         * pointer and just set the value to Null.
         */
        gf.fqn_prefix[LEVELPREFIX] = gf.fqn_prefix[SCOREPREFIX];
        gf.fqn_prefix[SAVEPREFIX] = gf.fqn_prefix[SCOREPREFIX];
        gf.fqn_prefix[BONESPREFIX] = gf.fqn_prefix[SCOREPREFIX];
        gf.fqn_prefix[LOCKPREFIX] = gf.fqn_prefix[SCOREPREFIX];
        gf.fqn_prefix[TROUBLEPREFIX] = gf.fqn_prefix[SCOREPREFIX];
#endif
        check_recordfile(dir);
    }
    return;
}
#endif /* CHDIR */

/* returns True iff we set plname[] to username which contains a hyphen */
boolean
whoami(void)
{
    /*
     * Who am i? Algorithm: 1. Use name as specified in NETHACKOPTIONS
     *                      2. Use $USER or $LOGNAME    (if 1. fails)
     *                      3. Use getlogin()           (if 2. fails)
     * The resulting name is overridden by command line options.
     * If everything fails, or if the resulting name is some generic
     * account like "games", "play", "player", "hack" then eventually
     * we'll ask him.
     * Note that we trust the user here; it is possible to play under
     * somebody else's name.
     */
    if (!*svp.plname) {
        register const char *s;

        s = nh_getenv("USER");
        if (!s || !*s)
            s = nh_getenv("LOGNAME");
        if (!s || !*s)
            s = getlogin();

        if (s && *s) {
            (void) strncpy(svp.plname, s, sizeof svp.plname - 1);
            if (strchr(svp.plname, '-'))
                return TRUE;
        }
    }
    return FALSE;
}

void
sethanguphandler(void (*handler)(int))
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
#endif /* ?SA_RESTART */
}

#ifdef PORT_HELP
void
port_help(void)
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
authorize_wizard_mode(void)
{
    if (sysopt.wizards && sysopt.wizards[0]) {
        if (check_user_string(sysopt.wizards))
            return TRUE;
    }
    iflags.wiz_error_flag = TRUE; /* not being allowed into wizard mode */
    return FALSE;
}

/* similar to above, validate explore mode access */
boolean
authorize_explore_mode(void)
{
#ifdef SYSCF
    if (sysopt.explorers && sysopt.explorers[0]) {
        if (check_user_string(sysopt.explorers))
            return TRUE;
    }
    iflags.explore_error_flag = TRUE; /* not allowed into explore mode */
    return FALSE;
#else
    return TRUE; /* if sysconf disabled, no restrictions on explore mode */
#endif
}

static void
wd_message(void)
{
    if (iflags.wiz_error_flag) {
        if (sysopt.wizards && sysopt.wizards[0]) {
            char *tmp = build_english_list(sysopt.wizards);
            pline("Only user%s %s may access debug (wizard) mode.",
                  strchr(sysopt.wizards, ' ') ? "s" : "", tmp);
            free(tmp);
        } else {
            You("cannot access debug (wizard) mode.");
        }
        wizard = FALSE; /* (paranoia) */
        if (!iflags.explore_error_flag)
            pline("Entering explore/discovery mode instead.");
    } else if (iflags.explore_error_flag) {
        You("cannot access explore mode."); /* same as enter_explore_mode */
        discover = iflags.deferred_X = FALSE; /* (more paranoia) */
    } else if (discover)
        You("are in non-scoring explore/discovery mode.");
}

/*
 * Add a slash to any name not ending in /. There must
 * be room for the /
 */
void
append_slash(char *name)
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
check_user_string(const char *optstr)
{
    struct passwd *pw;
    int pwlen;
    const char *eop, *w;
    char *pwname = 0;

    if (optstr[0] == '*')
        return TRUE; /* allow any user */
    if (sysopt.check_plname)
        pwname = svp.plname;
    else if ((pw = get_unix_pw()) != 0)
        pwname = pw->pw_name;
    if (!pwname || !*pwname)
        return FALSE;
    pwlen = (int) strlen(pwname);
    eop = eos((char *) optstr); /* temporarily cast away 'const' */
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
get_unix_pw(void)
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
get_login_name(void)
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
port_insert_pastebuf(char *buf)
{
    /* This should be replaced when there is a Cocoa port. */
    const char *errarg;
    size_t len;
    FILE *PB = popen("/usr/bin/pbcopy", "w");

    if (!PB) {
        errarg = "Unable to start pbcopy";
        goto error;
    }

    len = strlen(buf);
    /* Remove the trailing \n, carefully. */
    if (len > 0 && buf[len - 1] == '\n')
        len--;

    /* XXX Sorry, I'm too lazy to write a loop for output this short. */
    if (len != fwrite(buf, 1, len, PB)) {
        errarg = "Error sending data to pbcopy";
        goto error;
    }

    if (pclose(PB) != -1) {
        return;
    }
    errarg = "Error finishing pbcopy";

 error:
    raw_printf("%s: %s (%d)\n", errarg, strerror(errno), errno);
}
#endif /* __APPLE__ */

unsigned long
sys_random_seed(void)
{
    unsigned long seed = 0L;
    unsigned long pid = (unsigned long) getpid();
    boolean no_seed = TRUE;
#ifdef DEV_RANDOM
    FILE *fptr;

    fptr = fopen(DEV_RANDOM, "r");
    if (fptr) {
        fread(&seed, sizeof (long), 1, fptr);
        has_strong_rngseed = TRUE;  /* decl.c */
        no_seed = FALSE;
        (void) fclose(fptr);
    } else {
        /* leaves clue, doesn't exit */
        paniclog("sys_random_seed", "falling back to weak seed");
    }
#endif
    if (no_seed) {
        seed = (unsigned long) getnow(); /* time((TIME_type) 0) */
        /* Quick dirty band-aid to prevent PRNG prediction */
        if (pid) {
            if (!(pid & 3L))
                pid -= 1L;
            seed *= pid;
        }
    }
    return seed;
}

#if defined(MACOS) && defined(NHUUID)
extern void get_macos_uuid(char *); /* sys/unix/macuuid.m */
#endif

void
get_nhuuid(void)
{
#if defined(NHUUID)
    char struuid[NHUUIDSZ] = { 0 };
#if defined(LINUX) && defined(NHUUID)
    uuid_t binuuid;
#endif

    if (svn.nhuuid[0])
        return;

#if defined(MACOS)
    get_macos_uuid(&struuid[0]);
#elif defined(LINUX)
    uuid_generate_random(binuuid);
    uuid_unparse(binuuid, struuid);
#endif /* MACOS || LINUX */

    if (struuid[0])
        Snprintf(svn.nhuuid, sizeof svn.nhuuid, "%s", struuid);

#endif  /* NHUUID */
}

void
free_nhuuid(void)
{
    int i;

    for (i = 0; i < SIZE(svn.nhuuid); i++) {
        svn.nhuuid[i] = 0;
    }
}
/*unixmain.c*/
