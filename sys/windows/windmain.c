/* NetHack 5.0	windmain.c	$NHDT-Date: 1693359653 2023/08/30 01:40:53 $  $NHDT-Branch: keni-crashweb2 $:$NHDT-Revision: 1.189 $ */
/* Copyright (c) Derek S. Ray, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

/* main.c - Windows */

#include "win32api.h" /* for GetModuleFileName */

#include "hack.h"
#ifdef DLB
#include "dlb.h"
#endif
#include <sys\stat.h>
#include <errno.h>
#include <ShlObj.h>

static void nhusage(void);
char *exename(void);
boolean fakeconsole(void);
void freefakeconsole(void);
#if defined(MSWIN_GRAPHICS)
extern void mswin_destroy_reg(void);
#endif
#ifdef TTY_GRAPHICS
#ifdef WIN32CON
extern void backsp(void);
#endif
#endif
extern void term_clear_screen(void);

#ifdef update_file
#undef update_file
#endif
#if defined(TERMLIB) || defined(CURSES_GRAPHICS)
extern void (*decgraphics_mode_callback)(void);
#endif
#ifdef CURSES_GRAPHICS
extern void (*cursesgraphics_mode_callback)(void);
#endif
#ifdef ENHANCED_SYMBOLS
extern void (*utf8graphics_mode_callback)(void);
#endif

#ifdef WIN32CON
#ifdef _MSC_VER
#ifdef kbhit
#undef kbhit
#endif
#include <conio.h.>
#endif
#endif

#ifdef PC_LOCKING
static int eraseoldlocks(void);
#endif

int windows_nhgetch(void);
void windows_nhbell(void);
int windows_nh_poskey(int *, int *, int *);
void windows_raw_print(const char *);
char windows_yn_function(const char *, const char *, char);
/* static void windows_getlin(const char *, char *); */

#ifdef WIN32CON
extern int windows_console_custom_nhgetch(void);
int tty_self_recover_prompt(void);
#endif

int other_self_recover_prompt(void);

char orgdir[PATHLEN];
boolean getreturn_enabled;
int windows_startup_state = 0;    /* we flag whether to continue with this */
                                  /* 0 = keep starting up, everything is good */

extern int redirect_stdout;       /* from sys/share/pcsys.c */
extern int GUILaunched;
HANDLE hStdOut;
char default_window_sys[7] =
#if defined(MSWIN_GRAPHICS)
            "mswin";
#elif defined(TTY_GRAPHICS)
            "tty";
#endif
#ifdef WANT_GETHDATE
static struct stat hbuf;
#endif
#include <sys/stat.h>


extern char orgdir[];

extern void set_default_prefix_locations(const char *programPath);
void copy_sysconf_content(void);
void copy_config_content(void);
void copy_hack_content(void);
void copy_symbols_content(void);
void copy_file(const char *, const char *,
               const char *, const char *, boolean);
void update_file(const char *, const char *,
                 const char *, const char *, BOOL);
void windows_raw_print_bold(const char *);

void set_emergency_io(void);
staticfn void stdio_wait_synch(void);
staticfn void stdio_raw_print(const char *str);
staticfn void stdio_nonl_raw_print(const char *str);
staticfn void stdio_raw_print_bold(const char *str);
staticfn int stdio_nhgetch(void);

#ifdef PORT_HELP
void port_help(void);
#endif
void windows_raw_print(const char *str);

extern const char *known_handling[];     /* symbols.c */
extern const char *known_restrictions[]; /* symbols.c */

/* --------------------------------------------------------------------------- */

DISABLE_WARNING_UNREACHABLE_CODE

/*
 * NetHack main
 *
 * The following function is used in both the nongraphical nethack.exe, and
 * in the graphical nethackw.exe.
 *
 * The function below is called main() in the non-graphical build of
 * NetHack for Windows and is the primary entry point for the program.
 *
 * It is called nethackw_main() in the graphical build of NetHack for
 * Windows, where WinMain() is the primary entry point for the program
 * and this nethackw_main() is called as a sub function.
 *
 * The code in WinMain() (in win/win32/nethackw.c) is primarily focused
 * on setting up the graphical windows environment, and leaves the
 * NetHack-specific startup code to this function.
 *
 */

#if defined(MSWIN_GRAPHICS)
#define MAIN nethackw_main
int nethackw_main(int, char **);
#else
#define MAIN main
#endif

int
MAIN(int argc, char *argv[])
{
    boolean resuming = FALSE; /* assume new game */
    NHFILE *nhfp;
    char *windowtype = NULL;
    char fnamebuf[BUFSZ], encodedfnamebuf[BUFSZ];
    char failbuf[BUFSZ];
    int getlock_result = 0;
    HWND hwnd;
    HDC hdc;
    int bpp;
    char *dir = NULL;

#ifdef _MSC_VER
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    /* setting iflags.colorcount has to be after early_init()
     * because it zeros out all of iflags */
    hwnd = GetDesktopWindow();

#ifdef _MSC_VER
#ifdef DEBUG
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
#endif
#endif

    set_emergency_io();
#ifndef MSWIN_GRAPHICS
    early_init(argc, argv); /* already in WinMain for MSWIN_GRAPHICS */
#endif

    /* this must be done after early_init() because early_init()
       sets iflags to zero */
    hdc = GetDC(hwnd);
    if (hdc) {
        bpp = GetDeviceCaps(hdc, BITSPIXEL);
        iflags.colorcount = (bpp >= 16) ? 16777216 : (bpp >= 8) ? 256 : 16;
        ReleaseDC(hwnd, hdc);
    }
    gh.hname = "NetHack"; /* used for syntax messages */
    set_default_prefix_locations(
        argv[0]); /* must be re-done after initoptions_init()
                   * which clears out gp.fqn_prefix[] */
    copy_sysconf_content();
    copy_symbols_content();
    /* Now that sysconf has had a chance to set the TROUBLEPREFIX, don't
       allow it to be changed from here on out. */
    fqn_prefix_locked[TROUBLEPREFIX] = TRUE;
    copy_config_content();

    //   if (iflags.windowtype_deferred && gc.chosen_windowtype[0])
    //       windowtype = gc.chosen_windowtype;
    //   windowtype = gc.chosen_windowtype;

#if !defined(MSWIN_GRAPHICS)
    nethack_enter_consoletty();
    consoletty_open(1);
#endif

#ifdef EARLY_CONFIGFILE_PASS
    rcfile_interface_options();
    if (*gc.chosen_windowtype)
        windowtype = gc.chosen_windowtype;
#endif

    if (!windowtype) {
#ifdef MSWIN_GRAPHICS
        windowtype = "mswin";
#else
        windowtype = "tty";
#endif
    }
    choose_windows(
        windowtype); /* sets all the window port function pointers */

#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
    /* Save current directory and make sure it gets restored when
     * the game is exited.
     */
    if (getcwd(orgdir, sizeof orgdir) == (char *) 0)
        error("NetHack: current directory path too long");
#endif
    getreturn_enabled = TRUE;
    initoptions_init(); // This allows OPTIONS in syscf on Windows.
    set_default_prefix_locations(
        argv[0]); /* must be re-done after initoptions_init()
                   * which clears out gp.fqn_prefix[] */
    // iflags.windowtype_deferred = TRUE;

    program_state.early_options = 1;
    /* if (GUILaunched || IsDebuggerPresent()) */
    early_options(&argc, &argv, &dir);
    program_state.early_options = 0;


    initoptions();
#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
    chdir(gf.fqn_prefix[HACKPREFIX]);
#endif

    check_recordfile((char *) 0);
    /* did something earlier flag a need to exit without starting a game? */
    if (windows_startup_state > 0) {
        raw_printf("Exiting.");
        nethack_exit(EXIT_FAILURE);
    }

    /* Finished processing options, lock all directory paths */
    for (int i = 0; i < PREFIX_COUNT; i++)
        fqn_prefix_locked[i] = TRUE;

    if (!validate_prefix_locations(failbuf)) {
        raw_printf("Some invalid directory locations were specified:\n\t%s\n",
                   failbuf);
        nethack_exit(EXIT_FAILURE);
    }

    copy_hack_content();

    /*
     * It seems you really want to play.
     */
#ifndef CURSES_GRAPHICS
    if (argc >= 1 && !strcmpi(default_window_sys, "mswin")
        && (strstri(argv[0], "nethackw.exe") || GUILaunched))
        iflags.windowtype_locked = TRUE;
#endif
#ifdef WINCHAIN
    commit_windowchain();
#endif
    init_nhwindows(&argc, argv);
#ifdef TTY_GRAPHICS
    if WINDOWPORT(tty) {
        int i;

        for (i = 0; i < 5; ++i) {
            nh_delay_output();
        }

 /*        wait_synch(); */
    }
#endif
#ifdef DLB
    if (!dlb_init()) {
        pline("%s\n%s\n%s\n%s\n\n", copyright_banner_line(1),
              copyright_banner_line(2), copyright_banner_line(3),
              copyright_banner_line(4));
        pline("NetHack was unable to open the required file \"%s\"", DLBFILE);
        if (file_exists(DLBFILE))
            pline("\nAre you perhaps trying to run NetHack within a zip "
                  "utility?");
        error("dlb_init failure.");
    }
#endif

#if defined(SND_LIB_FMOD)
    assign_soundlib(soundlib_fmod);
#elif defined(SND_LIB_WINDSOUND)
    assign_soundlib(soundlib_windsound);
#endif

    u.uhp = 1; /* prevent RIP on early quits */
    u.ux = 0;  /* prevent flush_screen() */

    iflags.use_background_glyph = FALSE;
    if (WINDOWPORT(mswin))
        iflags.use_background_glyph = TRUE;

//    init_nhwindows(&argc, argv);

#ifdef WIN32CON
    if (WINDOWPORT(tty))
        toggle_mouse_support();
#endif

    if (gs.symset[PRIMARYSET].handling
        && !symset_is_compatible(gs.symset[PRIMARYSET].handling,
                                 windowprocs.wincap2)) {
        /* current symset handling and windowtype are
           not compatible, feature-wise. Use IBM defaults */
            load_symset("IBMGraphics_2", PRIMARYSET);
            load_symset("RogueEpyx", ROGUESET);
    }
    /* Has the callback for the symset been invoked? Config file processing to
       load a symset runs too early to accomplish that because
       the various *graphics_mode_callback pointers don't get set until
       term_start_screen, unfortunately */
#if defined(TERMLIB) || defined(CURSES_GRAPHICS)
    if (SYMHANDLING(H_DEC) && decgraphics_mode_callback)
        (*decgraphics_mode_callback)();
#endif     /* TERMLIB || CURSES */
#if 0
#ifdef CURSES_GRAPHICS
    if (WINDOWPORT(curses))
        (*cursesgraphics_mode_callback)();
#endif
#endif
#ifdef ENHANCED_SYMBOLS
    if (SYMHANDLING(H_UTF8) && utf8graphics_mode_callback)
        (*utf8graphics_mode_callback)();
#endif

    /* strip role,race,&c suffix; calls askname() if svp.plname[] is empty
       or holds a generic user name like "player" or "games" */
    plnamesuffix();
    set_playmode(); /* sets svp.plname to "wizard" for wizard mode */
    /* until the getlock code is resolved, override askname()'s
       setting of renameallowed; when False, player_selection()
       won't resent renaming as an option */
    iflags.renameallowed = FALSE;
    /* Obtain the name of the logged on user and incorporate
     * it into the name. */
    Sprintf(fnamebuf, "%s", svp.plname);
    (void) fname_encode(
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_-.", '%',
        fnamebuf, encodedfnamebuf, BUFSZ);
    Snprintf(gl.lock, sizeof gl.lock, "%s", encodedfnamebuf);
    /* regularize(lock); */ /* we encode now, rather than substitute */
    if ((getlock_result = getlock()) == 0)
        nethack_exit(EXIT_SUCCESS);

    if (getlock_result < 0) {
        if (program_state.in_self_recover) {
            program_state.in_self_recover = FALSE;
        }
        set_savefile_name(TRUE);
    }
    /* Set up level 0 file to keep the game state.
     */
    nhfp = create_levelfile(0, (char *) 0);
    if (!nhfp) {
        raw_print("Cannot create lock file");
    } else {
        svh.hackpid = GetCurrentProcessId();
        (void) write(nhfp->fd, (genericptr_t) &svh.hackpid, sizeof(svh.hackpid));
        close_nhfile(nhfp);
    }
    /*
     *  Initialize the vision system.  This must be before mklev() on a
     *  new game or before a level restore on a saved game.
     */
    vision_init();
    init_sound_disp_gamewindows();
    /*
     * First, try to find and restore a save file for specified character.
     * We'll return here if new game player_selection() renames the hero.
     */
attempt_restore:
    if ((getlock_result != -1) && (nhfp = restore_saved_game()) != 0) {
#ifdef NEWS
        if (iflags.news) {
            display_file(NEWS, FALSE);
            iflags.news = FALSE;
        }
#endif
        if (ge.early_raw_messages)
            raw_print("Restoring save file...");
        else
            pline("Restoring save file...");
        mark_synch(); /* flush output */
        if (dorecover(nhfp)) {
            resuming = TRUE; /* not starting new game */
            if (discover)
                You("are in non-scoring discovery mode.");
            if (discover || wizard) {
                if (y_n("Do you want to keep the save file?") == 'n')
                    (void) delete_savefile();
                else {
                    nh_compress(fqname(gs.SAVEF, SAVEPREFIX, 0));
                }
            }
        }
        if (program_state.in_self_recover) {
            program_state.in_self_recover = FALSE;
            set_savefile_name(TRUE);
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
                goto attempt_restore;
            }
        }
        newgame();
        if (discover)
            You("are in non-scoring discovery mode.");
    }

        // iflags.debug_fuzzer = TRUE;

        moveloop(resuming);
    nethack_exit(EXIT_SUCCESS);
    /*NOTREACHED*/
    return 0;
}

RESTORE_WARNING_UNREACHABLE_CODE

#if 0
static void
early_options(int argc, char *argv[])
{
    int i;

    if (argc > 1) {
        if (argcheck(argc, argv, ARG_VERSION) == 2)
            nethack_exit(EXIT_SUCCESS);

        if (argcheck(argc, argv, ARG_SHOWPATHS) == 2) {
            gd.deferred_showpaths = TRUE;
            /* gd.deferred_showpaths is not used by windows */
            return;
        }
#ifndef NODUMPENUMS
        if (argcheck(argc, argv, ARG_DUMPENUMS) == 2) {
            nethack_exit(EXIT_SUCCESS);
        }
#ifdef ENHANCED_SYMBOLS
        if (argcheck(argc, argv, ARG_DUMPGLYPHIDS) == 2) {
            nethack_exit(EXIT_SUCCESS);
        }
#endif
#endif
        if (argcheck(argc, argv, ARG_DUMPMONGEN) == 2) {
            nethack_exit(EXIT_SUCCESS);
        }
        if (argcheck(argc, argv, ARG_DUMPWEIGHTS) == 2) {
            nethack_exit(EXIT_SUCCESS);
        }
        if (argcheck(argc, argv, ARG_DEBUG) == 1) {
            argc--;
            argv++;
        }
        if (argcheck(argc, argv, ARG_WINDOWS) == 1) {
            argc--;
            argv++;
        }
#if defined(CRASHREPORT)
        if (argcheck(argc, argv, ARG_BIDSHOW) == 2) {
            nethack_exit(EXIT_SUCCESS);
        }
#endif
        if (argc > 1 && !strncmp(argv[1], "-d", 2) && argv[1][2] != 'e') {
            /* avoid matching "-dec" for DECgraphics; since the man page
             * says -d directory, hope nobody's using -desomething_else
             */
            argc--;
            argv++;
            const char *dir = argv[0] + 2;
            if (*dir == '=' || *dir == ':')
                dir++;
            if (!*dir && argc > 1) {
                argc--;
                argv++;
                dir = argv[0];
            }
            if (!*dir)
                error("Flag -d must be followed by a directory name.");
            Strcpy(gh.hackdir, dir);
        }

        if (argc > 1) {
            /*
             * Now we know the directory containing 'record' and
             * may do a prscore().
             */
            if (!strncmp(argv[1], "-s", 2)) {
#ifdef SYSCF
                initoptions();
#endif
                prscore(argc, argv);

                nethack_exit(EXIT_SUCCESS);
            }
#ifdef MSWIN_GRAPHICS
            if (GUILaunched) {
                if (!strncmpi(argv[1], "-clearreg", 6)) { /* clear registry */
                    mswin_destroy_reg();
                    nethack_exit(EXIT_SUCCESS);
                }
            }
#endif
            /* Don't initialize the full window system just to print usage */
            if (!strncmp(argv[1], "-?", 2) || !strncmp(argv[1], "/?", 2)) {
                nhusage();
                nethack_exit(EXIT_SUCCESS);
            }
        }
    }
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
                (void) strncpy(svp.plname, argv[0] + 2,
                               sizeof(svp.plname) - 1);
            else if (argc > 1) {
                argc--;
                argv++;
                (void) strncpy(svp.plname, argv[0], sizeof(svp.plname) - 1);
            } else
                raw_print("Player name expected after -u");
            break;
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
        case 'w': /* windowtype */
            config_error_init(FALSE, "command line", FALSE);
            if (strlen(&argv[0][2]) < (WINTYPELEN - 1))
                Strcpy(gc.chosen_windowtype, &argv[0][2]);
            config_error_done();
            break;
        case '@':
            flags.randomall = 1;
            break;
        default:
            if ((i = str2role(&argv[0][1])) >= 0) {
                flags.initrole = i;
                break;
            } else
                raw_printf("\nUnknown switch: %s", argv[0]);
            FALLTHROUGH;
        /* FALLTHRU */
        case '?':
            nhusage();
            nethack_exit(EXIT_SUCCESS);
        }
    }
}

static void
nhusage(void)
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
                   gh.hname);
    ADD_USAGE(buf2);

    (void) Sprintf(
        buf2, "\n%s [-d dir] [-u name] [-r race] [-p profession] [-[DX]]",
        gh.hname);
    ADD_USAGE(buf2);
#ifdef NEWS
    ADD_USAGE(" [-n]");
#endif
    (void) Sprintf(buf2, "\n       or\n%s [--showpaths]",
        gh.hname);
    ADD_USAGE(buf2);
    if (!iflags.window_inited)
        raw_printf("%s\n", buf1);
    else
        (void) printf("%s\n", buf1);
#undef ADD_USAGE
}
#endif /* 0 */

/* copy file if destination does not exist */
void
copy_file(const char *dst_folder, const char *dst_name,
          const char *src_folder, const char *src_name,
          boolean copy_even_if_it_exists)
{
    char dst_path[MAX_PATH];
    strcpy(dst_path, dst_folder);
    strcat(dst_path, dst_name);

    char src_path[MAX_PATH];
    strcpy(src_path, src_folder);
    strcat(src_path, src_name);

    if (!file_exists(src_path))
        error("Unable to copy file '%s' as it does not exist", src_path);

    if (file_exists(dst_path) && !copy_even_if_it_exists)
        return;

    BOOL success = CopyFileA(src_path, dst_path, !copy_even_if_it_exists);
    if (!success)
        error("Failed to copy '%s' to '%s' (%d)", src_path, dst_path, errno);
}

/* update file copying if it does not exist or src is newer then dst */
void
update_file(const char *dst_folder, const char *dst_name,
            const char *src_folder, const char *src_name, BOOL save_copy)
{
    char dst_path[MAX_PATH];
    strcpy(dst_path, dst_folder);
    strcat(dst_path, dst_name);

    char src_path[MAX_PATH];
    strcpy(src_path, src_folder);
    strcat(src_path, src_name);

    char save_path[MAX_PATH];
    strcpy(save_path, dst_folder);
    strcat(save_path, dst_name);
    strcat(save_path, ".save");

    if (!file_exists(src_path))
        error("Unable to copy file '%s' as it does not exist", src_path);

    if (!file_newer(src_path, dst_path))
        return;

    if (file_exists(dst_path) && save_copy)
        CopyFileA(dst_path, save_path, FALSE);

    BOOL success = CopyFileA(src_path, dst_path, FALSE);
    if (!success)
        error("Failed to update '%s' to '%s'", src_path, dst_path);
}

void
copy_symbols_content(void)
{
    char dst_path[MAX_PATH], interim_path[MAX_PATH], orig_path[MAX_PATH];

    boolean no_template = FALSE;

    /* Using the SYSCONFPREFIX path, lock it so that it does not change */
    fqn_prefix_locked[SYSCONFPREFIX] = TRUE;

    strcpy(orig_path, gf.fqn_prefix[DATAPREFIX]);
    strcat(orig_path, SYMBOLS_TEMPLATE);
    strcpy(interim_path, gf.fqn_prefix[SYSCONFPREFIX]);
    strcat(interim_path, SYMBOLS_TEMPLATE);
    strcpy(dst_path, gf.fqn_prefix[SYSCONFPREFIX]);
    strcat(dst_path, SYMBOLS);

    if (!file_exists(orig_path)) {
        char alt_orig_path[MAX_PATH];

        strcpy(alt_orig_path, gf.fqn_prefix[DATAPREFIX]);
        strcat(alt_orig_path, SYMBOLS);
        if (file_exists(alt_orig_path)) {
            no_template = TRUE;
            /* <dist>symbols -> <dist>symbols.template */
            copy_file(gf.fqn_prefix[DATAPREFIX], SYMBOLS_TEMPLATE,
                      gf.fqn_prefix[DATAPREFIX], SYMBOLS, TRUE);
        }
    }
    if (!file_exists(interim_path) || file_newer(orig_path, interim_path)) {
        /* <dist>symbols.template -> <playground>symbols.template */
        copy_file(gf.fqn_prefix[SYSCONFPREFIX], SYMBOLS_TEMPLATE,
                  gf.fqn_prefix[DATAPREFIX], SYMBOLS_TEMPLATE, TRUE);
    }
    if (!file_exists(dst_path) || file_newer(interim_path, dst_path)) {
        /* <playground>symbols.template -> <playground>symbols */
        copy_file(gf.fqn_prefix[SYSCONFPREFIX], SYMBOLS,
                  gf.fqn_prefix[SYSCONFPREFIX], SYMBOLS_TEMPLATE, TRUE);
    }
    nhUse(no_template);
}

void
copy_sysconf_content(void)
{
    /* Using the SYSCONFPREFIX path, lock it so that it does not change */
    fqn_prefix_locked[SYSCONFPREFIX] = TRUE;

    update_file(gf.fqn_prefix[SYSCONFPREFIX], SYSCF_TEMPLATE,
                gf.fqn_prefix[DATAPREFIX], SYSCF_TEMPLATE, FALSE);

    /* If the required early game file does not exist, copy it */
    copy_file(gf.fqn_prefix[SYSCONFPREFIX], SYSCF_FILE,
              gf.fqn_prefix[DATAPREFIX], SYSCF_TEMPLATE, FALSE);
}

void
copy_config_content(void)
{
    /* Using the CONFIGPREFIX path, lock it so that it does not change */
    fqn_prefix_locked[CONFIGPREFIX] = TRUE;

    /* Keep templates up to date */
    update_file(gf.fqn_prefix[CONFIGPREFIX], CONFIG_TEMPLATE,
                gf.fqn_prefix[DATAPREFIX], CONFIG_TEMPLATE, FALSE);

    /* If the required early game file does not exist, copy it */
    /* NOTE: We never replace .nethackrc or sysconf */
    copy_file(gf.fqn_prefix[CONFIGPREFIX], CONFIG_FILE,
              gf.fqn_prefix[DATAPREFIX], CONFIG_TEMPLATE, FALSE);
}

void
copy_hack_content(void)
{
    nhassert(fqn_prefix_locked[HACKPREFIX]);

    /* Keep Guidebook and opthelp up to date */
    update_file(gf.fqn_prefix[HACKPREFIX], GUIDEBOOK_FILE,
                gf.fqn_prefix[DATAPREFIX], GUIDEBOOK_FILE, FALSE);
    update_file(gf.fqn_prefix[HACKPREFIX], OPTIONFILE,
                gf.fqn_prefix[DATAPREFIX], OPTIONFILE, FALSE);
}


#ifdef PORT_HELP
void
port_help(void)
{
    /* display port specific help file */
    display_file(PORT_HELP, 1);
}
#endif /* PORT_HELP */

/* validate wizard mode if player has requested access to it */
boolean
authorize_wizard_mode(void)
{
    if (!strcmp(svp.plname, WIZARD_NAME))
        return TRUE;
    return FALSE;
}

/* similar to above, validate explore mode access */
boolean
authorize_explore_mode(void)
{
    return TRUE; /* no restrictions on explore mode */
}

#ifndef PATH_SEPARATOR
#define PATH_SEPARATOR '\\'
#endif

#if defined(WIN32) && !defined(WIN32CON)
static char exenamebuf[PATHLEN];
HANDLE hConIn;
HANDLE hConOut;
boolean has_fakeconsole;

char *
exename(void)
{
    int bsize = PATHLEN;
    char *tmp = exenamebuf, *tmp2;

#ifdef UNICODE
    {
        TCHAR wbuf[PATHLEN * 4];
        GetModuleFileName((HANDLE) 0, wbuf, PATHLEN * 4);
        WideCharToMultiByte(CP_ACP, 0, wbuf, -1, tmp, bsize, NULL, NULL);
    }
#else
    *(tmp + GetModuleFileName((HANDLE) 0, tmp, bsize)) = '\0';
#endif
    tmp2 = strrchr(tmp, PATH_SEPARATOR);
    if (tmp2)
        *tmp2 = '\0';
    tmp2++;
    return tmp2;
}

boolean
fakeconsole(void)
{
    if (!hStdOut) {
        HANDLE fkhStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
        HANDLE fkhStdIn = GetStdHandle(STD_INPUT_HANDLE);

        if (!fkhStdOut && !fkhStdIn) {
            /* Bool rval; */
            AllocConsole();
            AttachConsole(GetCurrentProcessId());
            /*  rval = SetStdHandle(STD_OUTPUT_HANDLE, hWrite); */
            (void) freopen("CON", "w", stdout);
            (void) freopen("CON", "r", stdin);
        }
        has_fakeconsole = TRUE;
    }

    /* Obtain handles for the standard Console I/O devices */
    hConIn = GetStdHandle(STD_INPUT_HANDLE);
    hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
#if 0
    if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE) CtrlHandler, TRUE)) {
        /* Unable to set control handler */
        cmode = 0; /* just to have a statement to break on for debugger */
    }
#endif
    return has_fakeconsole;
}
void freefakeconsole(void)
{
    if (has_fakeconsole) {
        FreeConsole();
    }
}
#endif

/*ARGSUSED*/
void
windows_raw_print(const char *str)
{
    if (str)
        fprintf(stdout, "%s\n", str);
    windows_nhgetch();
    return;
}

/*ARGSUSED*/
void
windows_raw_print_bold(const char *str)
{
    windows_raw_print(str);
    return;
}

int
windows_nhgetch(void)
{
    return getchar();
}


void
windows_nhbell(void)
{
    return;
}

/*ARGSUSED*/
int
windows_nh_poskey(int *x UNUSED, int *y UNUSED, int *mod UNUSED)
{
    return '\033';
}

/*ARGSUSED*/
char
windows_yn_function(const char *query UNUSED, const char *resp UNUSED,
                    char def UNUSED)
{
    return '\033';
}

/*ARGSUSED*/
#if 0
static void
windows_getlin(const char *prompt UNUSED, char *outbuf)
{
    Strcpy(outbuf, "\033");
}
#endif

#ifdef PC_LOCKING
static int
eraseoldlocks(void)
{
    int i;

    /* cannot use maxledgerno() here, because we need to find a lock name
     * before starting everything (including the dungeon initialization
     * that sets astral_level, needed for maxledgerno()) up
     */
    for (i = 1; i <= MAXDUNGEON * MAXLEVEL + 1; i++) {
        /* try to remove all */
        set_levelfile_name(gl.lock, i);
        (void) unlink(fqname(gl.lock, LEVELPREFIX, 0));
    }
    set_levelfile_name(gl.lock, 0);
#ifdef HOLD_LOCKFILE_OPEN
    really_close();
#endif
    if (unlink(fqname(gl.lock, LEVELPREFIX, 0)))
        return 0; /* cannot remove it */
    return (1);   /* success! */
}

DISABLE_WARNING_UNREACHABLE_CODE

int
getlock(void)
{
    int fd, ern = 0, prompt_result = 1;
    int fcmask = FCMASK;
#ifndef SELF_RECOVER
    char tbuf[BUFSZ];
#endif
    const char *fq_lock;
#define OOPS_BUFSZ 512
    char oops[OOPS_BUFSZ];
#ifdef WIN32CON
    boolean istty = WINDOWPORT(tty);
#endif

    /* we ignore QUIT and INT at this point */
    if (!lock_file(HLOCK, LOCKPREFIX, 10)) {
        wait_synch();
#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
        chdirx(orgdir, 0);
#endif
        error("Quitting.");
    }

    /* regularize(lock); */ /* already done in pcmain */
    /*Sprintf(tbuf, "%s", fqname(gl.lock, LEVELPREFIX, 0)); */
    set_levelfile_name(gl.lock, 0);
    fq_lock = fqname(gl.lock, LEVELPREFIX, 1);
    if ((fd = open(fq_lock, 0)) == -1) {
        if (errno == ENOENT)
            goto gotlock; /* no such file */
#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
        chdirx(orgdir, 0);
#endif
        error("Bad directory or name: %s\n%s\n", fq_lock,
                  strerror(errno));
        unlock_file(HLOCK);
        Sprintf(oops, "Cannot open %s", fq_lock);
        raw_print(oops);
        nethack_exit(EXIT_FAILURE);
    }

    (void) nhclose(fd);

#ifdef WIN32CON
    if (WINDOWPORT(tty))
        prompt_result = tty_self_recover_prompt();
    else
#endif
        prompt_result = other_self_recover_prompt();
    /*
     * prompt_result == 1  means recover old game.
     * prompt_result == -1 means willfully destroy the old game.
     * prompt_result == 0 should just exit.
     */
    Sprintf(oops, "You chose to %s.",
                (prompt_result == -1)
                    ? "destroy the old game and start a new one"
                    : (prompt_result == 1)
                        ? "recover the old game"
                        : "not start a new game");
#ifdef WIN32CON
    if (istty)
        term_clear_screen();
#endif
    raw_printf("%s", oops);
    if (prompt_result == 1) {          /* recover */
        if (recover_savefile()) {
#if 0
            if (istty)
                term_clear_screen(); /* display gets fouled up otherwise */
#endif
            goto gotlock;
        } else {
            unlock_file(HLOCK);
#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
            chdirx(orgdir, 0);
#endif
            raw_print("Couldn't recover the old game.");
        }
    } else if (prompt_result < 0) {    /* destroy old game */
        if (eraseoldlocks()) {
#ifdef WIN32CON
            if (istty)
                term_clear_screen(); /* display gets fouled up otherwise */
#endif
            goto gotlock;
        } else {
            unlock_file(HLOCK);
#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
            chdirx(orgdir, 0);
#endif
            raw_print("Couldn't destroy the old game.");
            return 0;
        }
    } else {
        unlock_file(HLOCK);
#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
        chdirx(orgdir, 0);
#endif
        return 0;
    }

gotlock:
    fd = creat(fq_lock, fcmask);
    if (fd == -1)
        ern = errno;
    unlock_file(HLOCK);
    if (fd == -1) {
#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
        chdirx(orgdir, 0);
#endif
        Sprintf(oops, "cannot creat file (%s.)\n%s\n%s\"%s\" exists?\n", fq_lock,
              strerror(ern), " Are you sure that the directory",
              gf.fqn_prefix[LEVELPREFIX]);
        raw_print(oops);
    } else {
        if (write(fd, (char *) &svh.hackpid, sizeof(svh.hackpid))
            != sizeof(svh.hackpid)) {
#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
            chdirx(orgdir, 0);
#endif
            error("cannot write lock (%s)", fq_lock);
        }
        if (nhclose(fd) == -1) {
#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
            chdirx(orgdir, 0);
#endif
            error("cannot close lock (%s)", fq_lock);
        }
    }
    return prompt_result;
}
#endif /* PC_LOCKING */

RESTORE_WARNING_UNREACHABLE_CODE

/*
  file_newer returns TRUE if the file at a_path is newer then the file
  at b_path.  If a_path does not exist, it returns FALSE.  If b_path
  does not exist, it returns TRUE.
 */
boolean
file_newer(const char *a_path, const char *b_path)
{
    struct stat a_sb = { 0 };
    struct stat b_sb = { 0 };
    double timediff;

    if (stat(a_path, &a_sb))
        return FALSE;

    if (stat(b_path, &b_sb))
        return TRUE;
    timediff = difftime(a_sb.st_mtime, b_sb.st_mtime);

    if(timediff > 0)
        return TRUE;
    return FALSE;
}

#ifdef WIN32CON
/*
 * returns:
 *     1 if game should be recovered
 *    -1 if old game should be destroyed, allowing new game to proceed.
 */
int
tty_self_recover_prompt(void)
{
    int c, ci, ct, pl, retval = 0;
    /* for saving/replacing functions, if needed */
    struct window_procs saved_procs = {0};

    pl = 1;
    c = 'n';
    ct = 0;
    saved_procs = windowprocs;

    raw_print("\n");
    raw_print("\n");
    raw_print("\n");
    raw_print("\n");
    raw_print("\n");
    raw_print("There are files from a game in progress under your name. ");
    raw_print("Recover? [yn] ");

 tty_ask_again:

    while ((ci = nhgetch()) && !(ci == '\n' || ci == 13)) {
        if (ct > 0) {
            /* invalid answer */
            raw_print("\b \b");
            ct = 0;
            c = 'n';
        }
        if (ci == 'y' || ci == 'n' || ci == 'Y' || ci == 'N') {
            ct = 1;
            c = ci;
#ifdef _MSC_VER
            _putch(ci);
#endif
        }
    }

    if (pl == 1 && (c == 'n' || c == 'N')) {
        /* no to recover */
        raw_print("\n\nAre you sure you wish to destroy the old game rather than try to\n");
        raw_print("recover it? [yn] ");
        c = 'n';
        ct = 0;
        pl = 2;
        goto tty_ask_again;
    }

    if (pl == 2 && (c == 'n' || c == 'N')) {
        /* no to destruction of old game */
        retval = 0;
    } else {
        /* only yes answers get here */
        if (pl == 2)
            retval = -1;  /* yes, do destroy the old game anyway */
        else
            retval = 1;   /* yes, do recover the old game */
    }
    if (saved_procs.name[0]) {
        windowprocs = saved_procs;
        raw_clear_screen();
    }
    return retval;
}
#endif

int
other_self_recover_prompt(void)
{
    int c, ci, ct, pl, retval = 0;
    boolean ismswin = WINDOWPORT(mswin),
            iscurses = WINDOWPORT(curses);

    pl = 1;
    c = 'n';
    ct = 0;
    if (iflags.window_inited || WINDOWPORT(curses)) {
        c = y_n("There are files from a game in progress under your name. "
               "Recover?");
    } else {
        c = 'n';
        ct = 0;
        raw_print("There are files from a game in progress under your name. "
              "Recover? [yn]");
    }

 other_ask_again:

    if (!ismswin && !iscurses) {
        while ((ci = nhgetch()) && !(ci == '\n' || ci == 13)) {
            if (ct > 0) {
                /* invalid answer */
                raw_print("\b \b");
                ct = 0;
                c = 'n';
            }
            if (ci == 'y' || ci == 'n' || ci == 'Y' || ci == 'N') {
                ct = 1;
                c = ci;
            }
        }
    }
    if (pl == 1 && (c == 'n' || c == 'N')) {
        /* no to recover */
        c = y_n("Are you sure you wish to destroy the old game, rather than try to "
                  "recover it? [yn] ");
        pl = 2;
        if (!ismswin && !iscurses) {
            c = 'n';
            ct = 0;
            goto other_ask_again;
        }
    }
    if (pl == 2 && (c == 'n' || c == 'N')) {
        /* no to destruction of old game */
        retval = 0;
    } else {
        /* only yes answers get here */
        if (pl == 2)
            retval = -1;  /* yes, do destroy the old game anyway */
        else
            retval = 1;   /* yes, do recover the old game */
    }
    return retval;
}

#ifdef CHDIR
void
chdirx(const char *dir, boolean wr)
{
    static char thisdir[] = ".";

    if (dir && chdir(dir) < 0) {
        error("Cannot chdir to %s.", dir);
    }

    /* warn the player if we can't write the record file */
    /* perhaps we should also test whether . is writable */
    /* unfortunately the access system-call is worthless */
    if (wr)
        check_recordfile(dir ? dir : thisdir);
}
#endif /* CHDIR */

void
set_emergency_io(void)
{
    windowprocs.win_raw_print = stdio_raw_print;
    windowprocs.win_raw_print_bold = stdio_raw_print_bold;
    windowprocs.win_nhgetch = stdio_nhgetch;
    windowprocs.win_wait_synch = stdio_wait_synch;
}


/* Add to your code: windowprocs.win_raw_print = stdio_wait_synch; */
void
stdio_wait_synch(void)
{
    char valid[] = { ' ', '\n', '\r', '\033', '\0' };

    fprintf(stdout, "--More--");
    (void) fflush(stdout);
    while (!strchr(valid, nhgetch()))
        ;
}

/* Add to your code: windowprocs.win_raw_print = stdio_raw_print; */
void
stdio_raw_print(const char *str)
{
    if (str)
        fprintf(stdout, "%s\n", str);
    return;
}

/* no newline variation, add to your code:
    windowprocs.win_raw_print = stdio_nonl_raw_print;  */
void
stdio_nonl_raw_print(const char *str)
{
    if (str)
        fprintf(stdout, "%s", str);
    return;
}

/* Add to your code: windowprocs.win_raw_print_bold = stdio_raw_print_bold; */
void
stdio_raw_print_bold(const char *str)
{
    stdio_raw_print(str);
    return;
}

/* Add to your code: windowprocs.win_nhgetch = stdio_nhgetch; */
int
stdio_nhgetch(void)
{
    return getchar();
}

    /*windmain.c*/
