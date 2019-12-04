/* NetHack 3.6	windmain.c	$NHDT-Date: 1543465755 2018/11/29 04:29:15 $  $NHDT-Branch: NetHack-3.6.2-beta01 $:$NHDT-Revision: 1.101 $ */
/* Copyright (c) Derek S. Ray, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

/* main.c - Windows */

#include "win32api.h" /* for GetModuleFileName */
#include "hack.h"
#include "dlb.h"
#include <ctype.h>
#include <stdlib.h>
#include <sys\stat.h>
#include <errno.h>
#include <ShlObj.h>
#if !defined(VERSION_MAJOR)
#include "patchlevel.h"
#endif

#if !defined(SAFEPROCS)
#error You must #define SAFEPROCS to build windmain.c
#endif

#define E extern
static void FDECL(process_options, (int argc, char **argv));
static void NDECL(nhusage);
static char *NDECL(get_executable_path);
char *FDECL(translate_path_variables, (const char *, char *));
char *NDECL(exename);
boolean NDECL(fakeconsole);
void NDECL(freefakeconsole);
E void FDECL(nethack_exit, (int));
E char chosen_windowtype[WINTYPELEN];   /* flag.h */
#if defined(MSWIN_GRAPHICS)
E void NDECL(mswin_destroy_reg);
#endif
#ifdef TTY_GRAPHICS
extern void NDECL(backsp);
#endif
extern void NDECL(clear_screen);
#undef E

#ifdef _MSC_VER
#ifdef kbhit
#undef kbhit
#endif
#include <conio.h.>
#endif

#ifdef PC_LOCKING
static int NDECL(eraseoldlocks);
#endif
int NDECL(windows_nhgetch);
void NDECL(windows_nhbell);
int FDECL(windows_nh_poskey, (int *, int *, int *));
void FDECL(windows_raw_print, (const char *));
char FDECL(windows_yn_function, (const char *, const char *, CHAR_P));
static void FDECL(windows_getlin, (const char *, char *));
extern int NDECL(windows_console_custom_nhgetch);
void NDECL(safe_routines);
int NDECL(tty_self_recover_prompt);
int NDECL(other_self_recover_prompt);

char orgdir[PATHLEN];
boolean getreturn_enabled;
extern int redirect_stdout;       /* from sys/share/pcsys.c */
extern int GUILaunched;
HANDLE hStdOut;
#if defined(MSWIN_GRAPHICS)
char default_window_sys[] = "mswin";
#endif
#ifdef WANT_GETHDATE
static struct stat hbuf;
#endif
#include <sys/stat.h>


extern char orgdir[];

void
get_known_folder_path(
    const KNOWNFOLDERID * folder_id,
    char * path
    , size_t path_size)
{
    PWSTR wide_path;
    if (FAILED(SHGetKnownFolderPath(folder_id, 0, NULL, &wide_path)))
        error("Unable to get known folder path");

    size_t converted;
    errno_t err;

    err = wcstombs_s(&converted, path, path_size, wide_path, path_size - 1);

    CoTaskMemFree(wide_path);

    if (err != 0) error("Failed folder path string conversion");
}

void
create_directory(const char * path)
{
    HRESULT hr = CreateDirectoryA(path, NULL);

    if (FAILED(hr) && hr != ERROR_ALREADY_EXISTS)
        error("Unable to create directory '%s'", path);
}

void
build_known_folder_path(
    const KNOWNFOLDERID * folder_id,
    char * path,
    size_t path_size,
    boolean versioned)
{
    get_known_folder_path(folder_id, path, path_size);
    strcat(path, "\\NetHack\\");
    create_directory(path);
    if (versioned) {
        Sprintf(eos(path), "%d.%d\\", 
                    VERSION_MAJOR, VERSION_MINOR);
        create_directory(path);
    }
}

void
build_environment_path(
    const char * env_str,
    const char * folder,
    char * path,
    size_t path_size)
{
    path[0] = '\0';

    const char * root_path = nh_getenv(env_str);

    if (root_path == NULL) return;

    strcpy_s(path, path_size, root_path);

    char * colon = index(path, ';');
    if (colon != NULL) path[0] = '\0';

    if (strlen(path) == 0) return;

    append_slash(path);

    if (folder != NULL) {
        strcat_s(path, path_size, folder);
        strcat_s(path, path_size, "\\");
    }
}

boolean
folder_file_exists(const char * folder, const char * file_name)
{
    char path[MAX_PATH];

    if (folder[0] == '\0') return FALSE;

    strcpy(path, folder);
    strcat(path, file_name);
    return file_exists(path);
}

void
set_default_prefix_locations(const char *programPath)
{
    char *envp = NULL;
    char *sptr = NULL;

    static char executable_path[MAX_PATH];
    static char profile_path[MAX_PATH];
    static char versioned_profile_path[MAX_PATH];
    static char versioned_user_data_path[MAX_PATH];
    static char versioned_global_data_path[MAX_PATH];
    static char versioninfo[20];

    strcpy(executable_path, get_executable_path());
    append_slash(executable_path);

    build_known_folder_path(&FOLDERID_Profile, profile_path,
        sizeof(profile_path), FALSE);

    build_known_folder_path(&FOLDERID_Profile, versioned_profile_path,
        sizeof(profile_path), TRUE);

    build_known_folder_path(&FOLDERID_LocalAppData,
        versioned_user_data_path, sizeof(versioned_user_data_path), TRUE);

    build_known_folder_path(&FOLDERID_ProgramData,
        versioned_global_data_path, sizeof(versioned_global_data_path), TRUE);

    fqn_prefix[SYSCONFPREFIX] = versioned_global_data_path;
    fqn_prefix[CONFIGPREFIX] = profile_path;
    fqn_prefix[HACKPREFIX] = versioned_profile_path;
    fqn_prefix[SAVEPREFIX] = versioned_user_data_path;
    fqn_prefix[LEVELPREFIX] = versioned_user_data_path;
    fqn_prefix[BONESPREFIX] = versioned_global_data_path;
    fqn_prefix[SCOREPREFIX] = versioned_global_data_path;
    fqn_prefix[LOCKPREFIX] = versioned_global_data_path;
    fqn_prefix[TROUBLEPREFIX] = versioned_profile_path;
    fqn_prefix[DATAPREFIX] = executable_path;

}

/* copy file if destination does not exist */
void
copy_file(
    const char * dst_folder,
    const char * dst_name,
    const char * src_folder,
    const char * src_name)
{
    char dst_path[MAX_PATH];
    strcpy(dst_path, dst_folder);
    strcat(dst_path, dst_name);

    char src_path[MAX_PATH];
    strcpy(src_path, src_folder);
    strcat(src_path, src_name);

    if(!file_exists(src_path))
        error("Unable to copy file '%s' as it does not exist", src_path);

    if(file_exists(dst_path))
        return;

    BOOL success = CopyFileA(src_path, dst_path, TRUE);
    if(!success) error("Failed to copy '%s' to '%s'", src_path, dst_path);
}

/* update file copying if it does not exist or src is newer then dst */
void
update_file(
    const char * dst_folder,
    const char * dst_name,
    const char * src_folder,
    const char * src_name,
    BOOL save_copy)
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

    if(!file_exists(src_path))
        error("Unable to copy file '%s' as it does not exist", src_path);

    if (!file_newer(src_path, dst_path))
        return;

    if (file_exists(dst_path) && save_copy)
        CopyFileA(dst_path, save_path, FALSE);

    BOOL success = CopyFileA(src_path, dst_path, FALSE);
    if(!success) error("Failed to update '%s' to '%s'", src_path, dst_path);

}

void copy_sysconf_content()
{
    /* Using the SYSCONFPREFIX path, lock it so that it does not change */
    fqn_prefix_locked[SYSCONFPREFIX] = TRUE;

    update_file(fqn_prefix[SYSCONFPREFIX], SYSCF_TEMPLATE,
        fqn_prefix[DATAPREFIX], SYSCF_TEMPLATE, FALSE);

    update_file(fqn_prefix[SYSCONFPREFIX], SYMBOLS_TEMPLATE,
        fqn_prefix[DATAPREFIX], SYMBOLS_TEMPLATE, FALSE);

    /* If the required early game file does not exist, copy it */
    copy_file(fqn_prefix[SYSCONFPREFIX], SYSCF_FILE,
        fqn_prefix[DATAPREFIX], SYSCF_TEMPLATE);

    update_file(fqn_prefix[SYSCONFPREFIX], SYMBOLS,
        fqn_prefix[DATAPREFIX], SYMBOLS_TEMPLATE, TRUE);
}

void copy_config_content()
{
    /* Using the CONFIGPREFIX path, lock it so that it does not change */
    fqn_prefix_locked[CONFIGPREFIX] = TRUE;

    /* Keep templates up to date */
    update_file(fqn_prefix[CONFIGPREFIX], CONFIG_TEMPLATE,
        fqn_prefix[DATAPREFIX], CONFIG_TEMPLATE, FALSE);

    /* If the required early game file does not exist, copy it */
    /* NOTE: We never replace .nethackrc or sysconf */
    copy_file(fqn_prefix[CONFIGPREFIX], CONFIG_FILE,
        fqn_prefix[DATAPREFIX], CONFIG_TEMPLATE);
}

void
copy_hack_content()
{
    nhassert(fqn_prefix_locked[HACKPREFIX]);

    /* Keep Guidebook and opthelp up to date */
    update_file(fqn_prefix[HACKPREFIX], GUIDEBOOK_FILE,
        fqn_prefix[DATAPREFIX], GUIDEBOOK_FILE, FALSE);
    update_file(fqn_prefix[HACKPREFIX], OPTIONFILE,
        fqn_prefix[DATAPREFIX], OPTIONFILE, FALSE);
}

/*
 * __MINGW32__ Note
 * If the graphics version is built, we don't need a main; it is skipped
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
    boolean resuming = FALSE; /* assume new game */
    int fd;
    char *windowtype = NULL;
    char *envp = NULL;
    char *sptr = NULL;
    char fnamebuf[BUFSZ], encodedfnamebuf[BUFSZ];
    char failbuf[BUFSZ];

    /*
     * Get a set of valid safe windowport function
     * pointers during early startup initialization.
     */
    safe_routines();
    sys_early_init();
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

    hname = "NetHack"; /* used for syntax messages */

#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
    /* Save current directory and make sure it gets restored when
     * the game is exited.
     */
    if (getcwd(orgdir, sizeof orgdir) == (char *) 0)
        error("NetHack: current directory path too long");
#endif

    set_default_prefix_locations(argv[0]);

#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
    chdir(fqn_prefix[HACKPREFIX]);
#endif

    if (GUILaunched || IsDebuggerPresent())
        getreturn_enabled = TRUE;

    check_recordfile((char *) 0);
    iflags.windowtype_deferred = TRUE;
    copy_sysconf_content();
    initoptions();

    /* Now that sysconf has had a chance to set the TROUBLEPREFIX, don't
       allow it to be changed from here on out. */
    fqn_prefix_locked[TROUBLEPREFIX] = TRUE;

    copy_config_content();
    process_options(argc, argv);

    /* Finished processing options, lock all directory paths */
    for(int i = 0; i < PREFIX_COUNT; i++)
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
    if (argc >= 1
        && !strcmpi(default_window_sys, "mswin")
        && (strstri(argv[0], "nethackw.exe") || GUILaunched))
        iflags.windowtype_locked = TRUE;
    windowtype = default_window_sys;

    if (!dlb_init()) {
        pline("%s\n%s\n%s\n%s\n\n",
              copyright_banner_line(1), copyright_banner_line(2),
              copyright_banner_line(3), copyright_banner_line(4));
        pline("NetHack was unable to open the required file \"%s\"",DLBFILE);
        if (file_exists(DLBFILE))
            pline("\nAre you perhaps trying to run NetHack within a zip utility?");
        error("dlb_init failure.");
    }

    if (!iflags.windowtype_locked) {
#if defined(TTY_GRAPHICS)
        Strcpy(default_window_sys, "tty");
#else
#if defined(CURSES_GRAPHICS)
        Strcpy(default_window_sys, "curses");
#endif /* CURSES */
#endif /* TTY */
        if (iflags.windowtype_deferred && chosen_windowtype[0])
            windowtype = chosen_windowtype;
    }
    choose_windows(windowtype);

    u.uhp = 1; /* prevent RIP on early quits */
    u.ux = 0;  /* prevent flush_screen() */

    nethack_enter(argc, argv);
    iflags.use_background_glyph = FALSE;
    if (WINDOWPORT("mswin"))
        iflags.use_background_glyph = TRUE;
    if (WINDOWPORT("tty"))
        nttty_open(1);

    init_nhwindows(&argc, argv);

    if (WINDOWPORT("tty"))
        toggle_mouse_support();

    /* strip role,race,&c suffix; calls askname() if plname[] is empty
       or holds a generic user name like "player" or "games" */
    plnamesuffix();
    set_playmode(); /* sets plname to "wizard" for wizard mode */
    /* until the getlock code is resolved, override askname()'s
       setting of renameallowed; when False, player_selection()
       won't resent renaming as an option */
    iflags.renameallowed = FALSE;
    /* Obtain the name of the logged on user and incorporate
     * it into the name. */
    Sprintf(fnamebuf, "%s", plname);
    (void) fname_encode(
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_-.", '%',
        fnamebuf, encodedfnamebuf, BUFSZ);
    Sprintf(lock, "%s", encodedfnamebuf);
    /* regularize(lock); */ /* we encode now, rather than substitute */
    if (getlock() == 0)
        nethack_exit(EXIT_SUCCESS);

    /* Set up level 0 file to keep the game state.
     */
    fd = create_levelfile(0, (char *) 0);
    if (fd < 0) {
        raw_print("Cannot create lock file");
    } else {
        hackpid = GetCurrentProcessId();
        write(fd, (genericptr_t) &hackpid, sizeof(hackpid));
        nhclose(fd);
    }
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
#ifdef NEWS
        if (iflags.news) {
            display_file(NEWS, FALSE);
            iflags.news = FALSE;
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
                    nh_compress(fqname(SAVEF, SAVEPREFIX, 0));
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
                goto attempt_restore;
            }
        }
        newgame();
        if (discover)
            You("are in non-scoring discovery mode.");
    }

	//	iflags.debug_fuzzer = TRUE;

	moveloop(resuming);
    nethack_exit(EXIT_SUCCESS);
    /*NOTREACHED*/
    return 0;
}

STATIC_OVL void
process_options(argc, argv)
int argc;
char *argv[];
{
    int i;

    /*
     * Process options.
     */
    if (argc > 1) {
        if (argcheck(argc, argv, ARG_VERSION) == 2)
            nethack_exit(EXIT_SUCCESS);

        if (argcheck(argc, argv, ARG_SHOWPATHS) == 2) {
            iflags.initoptions_noterminate = TRUE;
            initoptions();
            iflags.initoptions_noterminate = FALSE;
            reveal_paths();
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
        if (argc > 1 && !strncmp(argv[1], "-d", 2) && argv[1][2] != 'e') {
            /* avoid matching "-dec" for DECgraphics; since the man page
             * says -d directory, hope nobody's using -desomething_else
             */
            argc--;
            argv++;
            const char * dir = argv[0] + 2;
            if (*dir == '=' || *dir == ':')
                dir++;
            if (!*dir && argc > 1) {
                argc--;
                argv++;
                dir = argv[0];
            }
            if (!*dir)
                error("Flag -d must be followed by a directory name.");
            Strcpy(hackdir, dir);
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
            if (GUILaunched) {
                if (!strncmpi(argv[1], "-clearreg", 6)) { /* clear registry */
                    mswin_destroy_reg();
                    nethack_exit(EXIT_SUCCESS);
                }
            }
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
                (void) strncpy(plname, argv[0] + 2, sizeof(plname) - 1);
            else if (argc > 1) {
                argc--;
                argv++;
                (void) strncpy(plname, argv[0], sizeof(plname) - 1);
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
                Strcpy(chosen_windowtype, &argv[0][2]);
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
        /* FALL THROUGH */
        case '?':
            nhusage();
            nethack_exit(EXIT_SUCCESS);
        }
    }
}

STATIC_OVL void
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
                   hname);
    ADD_USAGE(buf2);

    (void) Sprintf(
        buf2, "\n%s [-d dir] [-u name] [-r race] [-p profession] [-[DX]]",
        hname);
    ADD_USAGE(buf2);
#ifdef NEWS
    ADD_USAGE(" [-n]");
#endif
    (void) Sprintf(buf2, "\n       or\n%s [--showpaths]",
        hname);
    ADD_USAGE(buf2);
    if (!iflags.window_inited)
        raw_printf("%s\n", buf1);
    else
        (void) printf("%s\n", buf1);
#undef ADD_USAGE
}

void
safe_routines(VOID_ARGS)
{
    /*
     * Get a set of valid safe windowport function
     * pointers during early startup initialization.
     */
    if (!WINDOWPORT("safe-startup"))
        windowprocs = *get_safe_procs(1);
    if (!GUILaunched)
        windowprocs.win_nhgetch = windows_console_custom_nhgetch;
}

#ifdef PORT_HELP
void
port_help()
{
    /* display port specific help file */
    display_file(PORT_HELP, 1);
}
#endif /* PORT_HELP */

/* validate wizard mode if player has requested access to it */
boolean
authorize_wizard_mode()
{
    if (!strcmp(plname, WIZARD_NAME))
        return TRUE;
    return FALSE;
}

#define PATH_SEPARATOR '\\'

#if defined(WIN32) && !defined(WIN32CON)
static char exenamebuf[PATHLEN];
extern HANDLE hConIn;
extern HANDLE hConOut;
boolean has_fakeconsole;

char *
exename()
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
        HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
        HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);

        if (!hStdOut && !hStdIn) {
            /* Bool rval; */
            AllocConsole();
            AttachConsole(GetCurrentProcessId());
            /* 	rval = SetStdHandle(STD_OUTPUT_HANDLE, hWrite); */
            freopen("CON", "w", stdout);
            freopen("CON", "r", stdin);
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
void freefakeconsole()
{
    if (has_fakeconsole) {
        FreeConsole();
    }
}
#endif

char *
get_executable_path()
{
    static char path_buffer[MAX_PATH];

#ifdef UNICODE
    {
        TCHAR wbuf[BUFSZ];
        GetModuleFileName((HANDLE) 0, wbuf, BUFSZ);
        WideCharToMultiByte(CP_ACP, 0, wbuf, -1, path_buffer, sizeof(path_buffer), NULL, NULL);
    }
#else
    DWORD length = GetModuleFileName((HANDLE) 0, path_buffer, MAX_PATH);
    if (length == ERROR_INSUFFICIENT_BUFFER) error("Unable to get module name");
    path_buffer[length] = '\0';
#endif

    char  * seperator = strrchr(path_buffer, PATH_SEPARATOR);
    if (seperator)
        *seperator = '\0';

    return path_buffer;
}

char *
translate_path_variables(str, buf)
const char *str;
char *buf;
{
    const char *src;
    char evar[BUFSZ], *dest, *envp, *eptr = (char *) 0;
    boolean in_evar;
    size_t ccount, ecount, destcount, slen = str ? strlen(str) : 0;

    if (!slen || !buf) {
        if (buf)
            *buf = '\0';
        return buf;
    }

    dest = buf;
    src = str;
    in_evar = FALSE;
    destcount = ecount = 0;
    for (ccount = 0; ccount < slen && destcount < (BUFSZ - 1) &&
                     ecount < (BUFSZ - 1); ++ccount, ++src) {
        if (*src == '%') {
            if (in_evar) {
                *eptr = '\0';
                envp = nh_getenv(evar);
                if (envp) {
                    size_t elen = strlen(envp);

                    if ((elen + destcount) < (size_t) (BUFSZ - 1)) {
                        Strcpy(dest, envp);
                        dest += elen;
                        destcount += elen;
                    }
                }
            } else {
                eptr = evar;
                ecount = 0;
            }
            in_evar = !in_evar;
            continue;
        }
        if (in_evar) {
            *eptr++ = *src;
            ecount++;
        } else {
            *dest++ = *src;
            destcount++;
        }
    }
    *dest = '\0';
    return buf;
}


/*ARGSUSED*/
void
windows_raw_print(str)
const char *str;
{
    if (str)
        fprintf(stdout, "%s\n", str);
    windows_nhgetch();
    return;
}

/*ARGSUSED*/
void
windows_raw_print_bold(str)
const char *str;
{
    windows_raw_print(str);
    return;
}

int
windows_nhgetch()
{
    return getchar();
}


void
windows_nhbell()
{
    return;
}

/*ARGSUSED*/
int
windows_nh_poskey(x, y, mod)
int *x, *y, *mod;
{
    return '\033';
}

/*ARGSUSED*/
char
windows_yn_function(query, resp, def)
const char *query;
const char *resp;
char def;
{
    return '\033';
}

/*ARGSUSED*/
static void
windows_getlin(prompt, outbuf)
const char *prompt UNUSED;
char *outbuf;
{
    Strcpy(outbuf, "\033");
}

#ifdef PC_LOCKING
static int
eraseoldlocks()
{
    register int i;

    /* cannot use maxledgerno() here, because we need to find a lock name
     * before starting everything (including the dungeon initialization
     * that sets astral_level, needed for maxledgerno()) up
     */
    for (i = 1; i <= MAXDUNGEON * MAXLEVEL + 1; i++) {
        /* try to remove all */
        set_levelfile_name(lock, i);
        (void) unlink(fqname(lock, LEVELPREFIX, 0));
    }
    set_levelfile_name(lock, 0);
#ifdef HOLD_LOCKFILE_OPEN
    really_close();
#endif
    if (unlink(fqname(lock, LEVELPREFIX, 0)))
        return 0; /* cannot remove it */
    return (1);   /* success! */
}

int
getlock()
{
    register int fd, ern, prompt_result = 0;
    int fcmask = FCMASK;
    char tbuf[BUFSZ];
    const char *fq_lock;
#define OOPS_BUFSZ 512
    char oops[OOPS_BUFSZ];
    boolean istty = WINDOWPORT("tty");

    /* we ignore QUIT and INT at this point */
    if (!lock_file(HLOCK, LOCKPREFIX, 10)) {
        wait_synch();
#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
        chdirx(orgdir, 0);
#endif
        error("Quitting.");
    }

    /* regularize(lock); */ /* already done in pcmain */
    Sprintf(tbuf, "%s", fqname(lock, LEVELPREFIX, 0));
    set_levelfile_name(lock, 0);
    fq_lock = fqname(lock, LEVELPREFIX, 1);
    if ((fd = open(fq_lock, 0)) == -1) {
        if (errno == ENOENT)
            goto gotlock; /* no such file */
#if defined(CHDIR) && !defined(NOCWD_ASSUMPTIONS)
        chdirx(orgdir, 0);
#endif
#if defined(HOLD_LOCKFILE_OPEN)
        if (errno == EACCES) {
            Strcpy(
                oops,
                "\nThere are files from a game in progress under your name.");
            Strcat(oops, "\nThe files are locked or inaccessible.");
            Strcat(oops, " Is the other game still running?\n");
            if (strlen(fq_lock) < ((OOPS_BUFSZ - 16) - strlen(oops)))
                Sprintf(eos(oops), "Cannot open %s", fq_lock);
            Strcat(oops, "\n");
            unlock_file(HLOCK);
            raw_print(oops);
        } else
#endif
            error("Bad directory or name: %s\n%s\n", fq_lock,
                  strerror(errno));
        unlock_file(HLOCK);
        Sprintf(oops, "Cannot open %s", fq_lock);
        raw_print(oops);
        nethack_exit(EXIT_FAILURE);
    }

    (void) nhclose(fd);

    if (WINDOWPORT("tty"))
        prompt_result = tty_self_recover_prompt();
    else
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
    if (istty)
        clear_screen();
    pline(oops);
    if (prompt_result == 1) {          /* recover */
        if (recover_savefile()) {
#if 0
            if (istty)
                clear_screen(); /* display gets fouled up otherwise */
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
            if (istty)
                clear_screen(); /* display gets fouled up otherwise */
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
              fqn_prefix[LEVELPREFIX]);
        raw_print(oops);
    } else {
        if (write(fd, (char *) &hackpid, sizeof(hackpid))
            != sizeof(hackpid)) {
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
    return 1;
}
#endif /* PC_LOCKING */

boolean
file_exists(path)
const char *path;
{
    struct stat sb;

    /* Just see if it's there */
    if (stat(path, &sb)) {
        return FALSE;
    }
    return TRUE;
}

/* 
  file_newer returns TRUE if the file at a_path is newer then the file
  at b_path.  If a_path does not exist, it returns FALSE.  If b_path
  does not exist, it returns TRUE.
 */
boolean
file_newer(a_path, b_path)
const char * a_path;
const char * b_path;
{
    struct stat a_sb;
    struct stat b_sb;

    if (stat(a_path, &a_sb))
        return FALSE;

    if (stat(b_path, &b_sb))
        return TRUE;

    if(difftime(a_sb.st_mtime, b_sb.st_mtime) < 0)
        return TRUE;
    return FALSE;
}

/*
 * returns:
 *     1 if game should be recovered
 *    -1 if old game should be destroyed, allowing new game to proceed.
 */
int
tty_self_recover_prompt()
{
    register int c, ci, ct, pl, retval = 0;
    /* for saving/replacing functions, if needed */
    struct window_procs saved_procs = {0};

    pl = 1;
    c = 'n';
    ct = 0;
    saved_procs = windowprocs;
    if (!WINDOWPORT("safe-startup"))
        windowprocs = *get_safe_procs(2); /* arg 2 uses no-newline variant */
    windowprocs.win_nhgetch = windows_console_custom_nhgetch;
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

int
other_self_recover_prompt()
{
    register int c, ci, ct, pl, retval = 0;
    boolean ismswin = WINDOWPORT("mswin"),
            iscurses = WINDOWPORT("curses");

    pl = 1;
    c = 'n';
    ct = 0;
    if (iflags.window_inited || WINDOWPORT("curses")) {
        c = yn("There are files from a game in progress under your name. "
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
        c = yn("Are you sure you wish to destroy the old game, rather than try to "
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
/*windmain.c*/
