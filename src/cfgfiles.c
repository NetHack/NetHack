/* NetHack 5.0	cfgfiles.c	$NHDT-Date: 1740532826 2025/02/25 17:20:26 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.417 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Derek S. Ray, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

#define NEED_VARARGS

#include "hack.h"
#include "dlb.h"
#include <errno.h>

#if (!defined(MACOS9) && !defined(O_WRONLY) && !defined(AZTEC_C)) \
    || defined(USE_FCNTL)
#include <fcntl.h>
#endif

#define BIGBUFSZ (5 * BUFSZ) /* big enough to format a 4*BUFSZ string (from
                              * config file parsing) with modest decoration;
                              * result will then be truncated to BUFSZ-1 */

#ifdef USER_SOUNDS
extern char *sounddir; /* defined in sounds.c */
#endif

staticfn void vconfig_error_add(const char *, va_list);
staticfn FILE *fopen_config_file(const char *, int);
staticfn int get_uchars(char *, uchar *, boolean, int, const char *);
#ifdef NOCWD_ASSUMPTIONS
staticfn void adjust_prefix(char *, int);
#endif
staticfn char *choose_random_part(char *, char);
staticfn boolean config_error_nextline(const char *);
staticfn void free_config_sections(void);
staticfn char *is_config_section(char *);
staticfn boolean handle_config_section(char *);
boolean parse_config_line(char *);
staticfn char *find_optparam(char *);
#ifndef SFCTOOL
staticfn boolean cnf_line_OPTIONS(char *);
staticfn boolean cnf_line_AUTOPICKUP_EXCEPTION(char *);
staticfn boolean cnf_line_BINDINGS(char *);
staticfn boolean cnf_line_AUTOCOMPLETE(char *);
staticfn boolean cnf_line_MSGTYPE(char *);
staticfn boolean cnf_line_HACKDIR(char *);
staticfn boolean cnf_line_LEVELDIR(char *);
staticfn boolean cnf_line_SAVEDIR(char *);
staticfn boolean cnf_line_BONESDIR(char *);
staticfn boolean cnf_line_DATADIR(char *);
staticfn boolean cnf_line_SCOREDIR(char *);
staticfn boolean cnf_line_LOCKDIR(char *);
staticfn boolean cnf_line_CONFIGDIR(char *);
staticfn boolean cnf_line_TROUBLEDIR(char *);
staticfn boolean cnf_line_NAME(char *);
staticfn boolean cnf_line_ROLE(char *);
staticfn boolean cnf_line_dogname(char *);
staticfn boolean cnf_line_catname(char *);
#endif /* SFCTOOL */
#ifdef SYSCF
staticfn boolean cnf_line_WIZARDS(char *);
staticfn boolean cnf_line_SHELLERS(char *);
staticfn boolean cnf_line_MSGHANDLER(char *);
staticfn boolean cnf_line_EXPLORERS(char *);
staticfn boolean cnf_line_DEBUGFILES(char *);
staticfn boolean cnf_line_DUMPLOGFILE(char *);
staticfn boolean cnf_line_GENERICUSERS(char *);
staticfn boolean cnf_line_BONES_POOLS(char *);
staticfn boolean cnf_line_SUPPORT(char *);
staticfn boolean cnf_line_RECOVER(char *);
staticfn boolean cnf_line_CHECK_SAVE_UID(char *);
staticfn boolean cnf_line_CHECK_PLNAME(char *);
staticfn boolean cnf_line_SEDUCE(char *);
staticfn boolean cnf_line_HIDEUSAGE(char *);
staticfn boolean cnf_line_MAXPLAYERS(char *);
staticfn boolean cnf_line_PERSMAX(char *);
staticfn boolean cnf_line_PERS_IS_UID(char *);
staticfn boolean cnf_line_ENTRYMAX(char *);
staticfn boolean cnf_line_POINTSMIN(char *);
staticfn boolean cnf_line_MAX_STATUENAME_RANK(char *);
staticfn boolean cnf_line_LIVELOG(char *);
staticfn boolean cnf_line_PANICTRACE_LIBC(char *);
staticfn boolean cnf_line_PANICTRACE_GDB(char *);
staticfn boolean cnf_line_GDBPATH(char *);
staticfn boolean cnf_line_GREPPATH(char *);
staticfn boolean cnf_line_CRASHREPORTURL(char *);
staticfn boolean cnf_line_ACCESSIBILITY(char *);

staticfn boolean cnf_line_PORTABLE_DEVICE_PATHS(char *);
#endif /* SYSCF */
#ifndef SFCTOOL
staticfn boolean cnf_line_BOULDER(char *);
staticfn boolean cnf_line_MENUCOLOR(char *);
staticfn boolean cnf_line_HILITE_STATUS(char *);
staticfn boolean cnf_line_WARNINGS(char *);
staticfn boolean cnf_line_ROGUESYMBOLS(char *);
staticfn boolean cnf_line_SYMBOLS(char *);
staticfn boolean cnf_line_WIZKIT(char *);
#ifdef USER_SOUNDS
staticfn boolean cnf_line_SOUNDDIR(char *);
staticfn boolean cnf_line_SOUND(char *);
#endif
staticfn boolean cnf_line_QT_TILEWIDTH(char *);
staticfn boolean cnf_line_QT_TILEHEIGHT(char *);
staticfn boolean cnf_line_QT_FONTSIZE(char *);
staticfn boolean cnf_line_QT_COMPACT(char *);
#endif /* SFCTOOL */
struct _cnf_parser_state; /* defined below (far below...) */
staticfn void cnf_parser_init(struct _cnf_parser_state *parser);
staticfn void cnf_parser_done(struct _cnf_parser_state *parser);
staticfn void parse_conf_buf(struct _cnf_parser_state *parser,
                           boolean (*proc)(char *arg));
/* next one is in extern.h; why here too? */
boolean parse_conf_str(const char *str, boolean (*proc)(char *arg));
static boolean ignore_errors_on_unmatched = FALSE,
               ignore_statement_errors = FALSE;

#ifdef SFCTOOL
#ifdef wait_synch
#undef wait_synch
#endif
#define wait_synch()
#endif /* SFCTOOL */

/* ----------  BEGIN CONFIG FILE HANDLING ----------- */

/* used for messaging. Also used in options.c */
static const char *default_configfile =
#ifdef UNIX
    ".nethackrc";
#else
#if defined(MACOS9) || defined(__BEOS__)
    "NetHack Defaults";
#else
#if defined(MSDOS) || defined(WIN32)
    CONFIG_FILE;
#else
    "NetHack.cnf";
#endif
#endif
#endif
static char configfile[BUFSZ];

char *
get_configfile(void)
{
    return configfile;
}

const char *
get_default_configfile(void)
{
    return default_configfile;
}

#ifdef MSDOS
/* conflict with speed-dial under windows
 * for XXX.cnf file so support of NetHack.cnf
 * is for backward compatibility only.
 * Preferred name (and first tried) is now defaults.nh but
 * the game will try the old name if there
 * is no defaults.nh.
 */
const char *backward_compat_configfile = "nethack.cnf";
#endif

#ifndef SFCTOOL

/* #saveoptions - save config options into file */
int
do_write_config_file(void)
{
    FILE *fp;
    char tmp[BUFSZ];

    if (!configfile[0]) {
        pline("Strange, could not figure out config file name.");
        return ECMD_OK;
    }
    if (flags.suppress_alert < FEATURE_NOTICE_VER(3,7,0)) {
        pline("Warning: saveoptions is highly experimental!");
        wait_synch();
        pline("Some settings are not saved!");
        wait_synch();
        pline("All manual customization and comments are removed"
              " from the file!");
        wait_synch();
    }
#define overwrite_prompt "Overwrite config file %.*s?"
    Sprintf(tmp, overwrite_prompt,
            (int) (BUFSZ - sizeof overwrite_prompt - 2), configfile);
#undef overwrite_prompt
    if (!paranoid_query(TRUE, tmp))
        return ECMD_OK;

    fp = fopen(configfile, "w");
    if (fp) {
        size_t len, wrote;
        strbuf_t buf;

        strbuf_init(&buf);
        all_options_strbuf(&buf);
        len = strlen(buf.str);
        wrote = fwrite(buf.str, 1, len, fp);
        fclose(fp);
        strbuf_empty(&buf);
        if (wrote != len)
            pline("An error occurred, wrote only partial data (%zu/%zu).",
                  wrote, len);
    }
    return ECMD_OK;
}
#endif /* SFCTOOL */

/* remember the name of the file we're accessing;
   if may be used in option reject messages */
void
set_configfile_name(const char *fname)
{
    (void) strncpy(configfile, fname, sizeof configfile - 1);
    configfile[sizeof configfile - 1] = '\0';
}

staticfn FILE *
fopen_config_file(const char *filename, int src)
{
    FILE *fp;
#if defined(UNIX) || defined(VMS)
    char tmp_config[BUFSZ];
    char *envp;
#endif

    if (src == set_in_sysconf) {
        /* SYSCF_FILE; if we can't open it, caller will bail */
        if (filename && *filename) {
            set_configfile_name(fqname(filename, SYSCONFPREFIX, 0));
            fp = fopen(configfile, "r");
        } else
            fp = (FILE *) 0;
        return  fp;
    }
    /* If src != set_in_sysconf, "filename" is an environment variable, so it
     * should hang around. If set, it is expected to be a full path name
     * (if relevant)
     */
    if (filename && *filename) {
        set_configfile_name(filename);
#ifdef UNIX
        if (!strncmp(configfile, "~/", 2) && (envp = nh_getenv("HOME")) != 0) {
            /* support for command line '--nethackrc=~/path' (or for
               NETHACKOPTIONS='@~/path'; we don't support ~user/path) */
            Snprintf(tmp_config, sizeof tmp_config, "%s/%s",
                     envp, configfile + 2); /* insert $HOME/ and remove ~/ */
            set_configfile_name(tmp_config);
        }
        if (access(configfile, 4) == -1) { /* 4 is R_OK on newer systems */
            /* nasty sneaky attempt to read file through
             * NetHack's setuid permissions -- this is the only
             * place a file name may be wholly under the player's
             * control (but SYSCF_FILE is not under the player's
             * control so it's OK).
             */
            raw_printf("Access to %s denied (%d).", configfile, errno);
            wait_synch();
            /* fall through to standard names */
        } else
#endif
        if ((fp = fopen(configfile, "r")) != (FILE *) 0) {
            return  fp;
#if defined(UNIX) || defined(VMS)
        } else {
            /* access() above probably caught most problems for UNIX */
            raw_printf("Couldn't open requested config file %s (%d).",
                       configfile, errno);
            wait_synch();
#endif
        }
    }
    /* fall through to standard names */

#if defined(MICRO) || defined(MACOS9) || defined(__BEOS__) || defined(WIN32)
    set_configfile_name(fqname(default_configfile, CONFIGPREFIX, 0));
    if ((fp = fopen(configfile, "r")) != (FILE *) 0) {
        return fp;
    } else if (strcmp(default_configfile, configfile)) {
        set_configfile_name(default_configfile);
        if ((fp = fopen(configfile, "r")) != (FILE *) 0)
            return fp;
    }
#ifdef MSDOS
    set_configfile_name(fqname(backward_compat_configfile, CONFIGPREFIX, 0));
    if ((fp = fopen(configfile, "r")) != (FILE *) 0) {
        return fp;
    } else if (strcmp(backward_compat_configfile, configfile)) {
        set_configfile_name(backward_compat_configfile);
        if ((fp = fopen(configfile, "r")) != (FILE *) 0)
            return fp;
    }
#endif
#else
/* constructed full path names don't need fqname() */
#ifdef VMS
    /* no punctuation, so might be a logical name */
    set_configfile_name("nethackini");
    if ((fp = fopen(configfile, "r")) != (FILE *) 0)
        return fp;
    set_configfile_name("sys$login:nethack.ini");
    if ((fp = fopen(configfile, "r")) != (FILE *) 0)
        return fp;

    envp = nh_getenv("HOME");
    if (!envp || !*envp)
        Strcpy(tmp_config, "NetHack.cnf");
    else
        Sprintf(tmp_config, "%s%s%s", envp,
                !strchr(":]>/", envp[strlen(envp) - 1]) ? "/" : "",
                "NetHack.cnf");
    set_configfile_name(tmp_config);
    if ((fp = fopen(configfile, "r")) != (FILE *) 0)
        return fp;
#else /* should be only UNIX left */
    envp = nh_getenv("HOME");
    if (!envp)
        Strcpy(tmp_config, ".nethackrc");
    else
        Sprintf(tmp_config, "%s/%s", envp, ".nethackrc");

    set_configfile_name(tmp_config);
    if ((fp = fopen(configfile, "r")) != (FILE *) 0)
        return fp;
#if defined(__APPLE__) /* UNIX+__APPLE__ => OSX || MacOS */
    /* try an alternative */
    if (envp) {
        /* keep 'tmp_config' intact here; if alternates fail, use it to
           restore configfile[] to its preferred setting (".nethackrc") */
        char alt_config[sizeof tmp_config];

        /* OSX-style configuration settings */
        Snprintf(alt_config, sizeof alt_config, "%s/%s", envp,
                 "Library/Preferences/NetHack Defaults");
        set_configfile_name(alt_config);
        if ((fp = fopen(configfile, "r")) != (FILE *) 0)
            return fp;
        /* may be easier for user to edit if filename has '.txt' suffix */
        Snprintf(alt_config, sizeof alt_config, "%s/%s", envp,
                 "Library/Preferences/NetHack Defaults.txt");
        set_configfile_name(alt_config);
        if ((fp = fopen(configfile, "r")) != (FILE *) 0)
            return fp;
        /* couldn't open either of the alternate names; for use in
           messages, put 'configfile' back to the normal value rather than
           leaving it set to last alternate; retry open() to reset 'errno' */
        set_configfile_name(tmp_config);
        if ((fp = fopen(configfile, "r")) != (FILE *) 0)
            return fp;
    }
#endif /*__APPLE__*/
    if (errno != ENOENT) {
        const char *details;

        /* e.g., problems when setuid NetHack can't search home
           directory restricted to user */
#if defined(NHSTDC) && !defined(NOTSTDC)
        if ((details = strerror(errno)) == 0)
#endif
            details = "";
        raw_printf("Couldn't open default config file %s %s(%d).",
                   configfile, details, errno);
        wait_synch();
    }
#endif /* !VMS => Unix */
#endif /* !(MICRO || MACOS9 || __BEOS__ || WIN32) */
    return (FILE *) 0;
}

/*
 * Retrieve a list of integers from buf into a uchar array.
 *
 * NOTE: zeros are inserted unless modlist is TRUE, in which case the list
 *  location is unchanged.  Callers must handle zeros if modlist is FALSE.
 */
staticfn int
get_uchars(char *bufp,       /* current pointer */
           uchar *list,      /* return list */
           boolean modlist,  /* TRUE: list is being modified in place */
           int size,         /* return list size */
           const char *name) /* name of option for error message */
{
    unsigned int num = 0;
    int count = 0;
    boolean havenum = FALSE;

    while (1) {
        switch (*bufp) {
        case ' ':
        case '\0':
        case '\t':
        case '\n':
            if (havenum) {
                /* if modifying in place, don't insert zeros */
                if (num || !modlist)
                    list[count] = num;
                count++;
                num = 0;
                havenum = FALSE;
            }
            if (count == size || !*bufp)
                return count;
            bufp++;
            break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            havenum = TRUE;
            num = num * 10 + (*bufp - '0');
            bufp++;
            break;

        case '\\':
            goto gi_error;
            break;

        default:
 gi_error:
            raw_printf("Syntax error in %s", name);
            wait_synch();
            return count;
        }
    }
    /*NOTREACHED*/
}

#ifdef NOCWD_ASSUMPTIONS
staticfn void
adjust_prefix(char *bufp, int prefixid)
{
    char *ptr;

    if (!bufp)
        return;
#ifdef WIN32
    if (fqn_prefix_locked[prefixid])
        return;
#endif
    /* Backward compatibility, ignore trailing ;n */
    if ((ptr = strchr(bufp, ';')) != 0)
        *ptr = '\0';
    if (strlen(bufp) > 0) {
        gf.fqn_prefix[prefixid] = (char *) alloc(strlen(bufp) + 2);
        Strcpy(gf.fqn_prefix[prefixid], bufp);
        append_slash(gf.fqn_prefix[prefixid]);
    }
}
#endif

/* Choose at random one of the sep separated parts from str. Mangles str. */
staticfn char *
choose_random_part(char *str, char sep)
{
    int nsep = 1;
    int csep;
    int len = 0;
    char *begin = str;

    if (!str)
        return (char *) 0;

    while (*str) {
        if (*str == sep)
            nsep++;
        str++;
    }
#ifndef SFCTOOL
    csep = rn2(nsep);
#else
    nhUse(nsep);
    csep = 1;
#endif
    str = begin;
    while ((csep > 0) && *str) {
        str++;
        if (*str == sep)
            csep--;
    }
    if (*str) {
        if (*str == sep)
            str++;
        begin = str;
        while (*str && *str != sep) {
            str++;
            len++;
        }
        *str = '\0';
        if (len)
            return begin;
    }
    return (char *) 0;
}

staticfn void
free_config_sections(void)
{
    if (gc.config_section_chosen) {
        free(gc.config_section_chosen);
        gc.config_section_chosen = NULL;
    }
    if (gc.config_section_current) {
        free(gc.config_section_current);
        gc.config_section_current = NULL;
    }
}

/* check for " [ anything-except-bracket-or-empty ] # arbitrary-comment"
   with spaces optional; returns pointer to "anything-except..." (with
   trailing " ] #..." stripped) if ok, otherwise Null */
staticfn char *
is_config_section(
    char *str) /* trailing spaces are stripped, ']' too iff result is good */
{
    char *a, *c, *z;

    /* remove any spaces at start and end; won't significantly interfere
       with echoing the string in a config error message, if warranted */
    a = trimspaces(str);
    /* first character should be open square bracket; set pointer past it */
    if (*a++ != '[')
        return (char *) 0;
    /* last character should be close bracket, ignoring any comment */
    z = strchr(a, ']');
    if (!z)
        return (char *) 0;
    /* comment, if present, can be preceded by spaces */
    for (c = z + 1; *c == ' '; ++c)
        continue;
    if (*c && *c != '#')
        return (char *) 0;
    /* we now know that result is good; there won't be a config error
       message so we can modify the input string */
    *z = '\0';
    /* 'a' points past '[' and the string ends where ']' was; remove any
       spaces between '[' and choice-start and between choice-end and ']' */
    return trimspaces(a);
}

staticfn boolean
handle_config_section(char *buf)
{
    char *sect = is_config_section(buf);

    if (sect) {
        if (gc.config_section_current)
            free(gc.config_section_current), gc.config_section_current = 0;
        /* is_config_section() removed brackets from 'sect' */
        if (!gc.config_section_chosen) {
            config_error_add("Section \"[%s]\" without CHOOSE", sect);
            return TRUE;
        }
        if (*sect) { /* got a section name */
            gc.config_section_current = dupstr(sect);
            debugpline1("set config section: '%s'",
                        gc.config_section_current);
        } else { /* empty section name => end of sections */
            free_config_sections();
            debugpline0("unset config section");
        }
        return TRUE;
    }

    if (gc.config_section_current) {
        if (!gc.config_section_chosen)
            return TRUE;
        if (strcmp(gc.config_section_current, gc.config_section_chosen))
            return TRUE;
    }
    return FALSE;
}

#define match_varname(INP, NAM, LEN) match_optname(INP, NAM, LEN, TRUE)

/* find the '=' or ':' */
staticfn char *
find_optparam(char *buf)
{
    char *bufp, *altp;

    bufp = strchr(buf, '=');
    altp = strchr(buf, ':');
    if (!bufp || (altp && altp < bufp))
        bufp = altp;

    return bufp;
}

#ifndef SFCTOOL

staticfn boolean
cnf_line_OPTIONS(char *origbuf)
{
    char *bufp = find_optparam(origbuf);

    ++bufp; /* skip '='; parseoptions() handles spaces */
    return parseoptions(bufp, TRUE, TRUE);
}

staticfn boolean
cnf_line_AUTOPICKUP_EXCEPTION(char *bufp)
{
    add_autopickup_exception(bufp);
    return TRUE;
}

staticfn boolean
cnf_line_BINDINGS(char *bufp)
{
    return parsebindings(bufp);
}

staticfn boolean
cnf_line_AUTOCOMPLETE(char *bufp)
{
    parseautocomplete(bufp, TRUE);
    return TRUE;
}

staticfn boolean
cnf_line_MSGTYPE(char *bufp)
{
    return msgtype_parse_add(bufp);
}

staticfn boolean
cnf_line_HACKDIR(char *bufp)
{
#ifdef NOCWD_ASSUMPTIONS
    adjust_prefix(bufp, HACKPREFIX);
#else /*NOCWD_ASSUMPTIONS*/
#ifdef MICRO
    (void) strncpy(gh.hackdir, bufp, PATHLEN - 1);
#else /* MICRO */
    nhUse(bufp);
#endif /* MICRO */
#endif /*NOCWD_ASSUMPTIONS*/
    return TRUE;
}

staticfn boolean
cnf_line_LEVELDIR(char *bufp)
{
#ifdef NOCWD_ASSUMPTIONS
    adjust_prefix(bufp, LEVELPREFIX);
#else /*NOCWD_ASSUMPTIONS*/
#ifdef MICRO
    if (strlen(bufp) >= PATHLEN)
        bufp[PATHLEN - 1] = '\0';
    Strcpy(g.permbones, bufp);
    if (!ramdisk_specified || !*levels)
        Strcpy(levels, bufp);
    gr.ramdisk = (strcmp(g.permbones, levels) != 0);
#else /* MICRO */
    nhUse(bufp);
#endif /* MICRO */
#endif /*NOCWD_ASSUMPTIONS*/
    return TRUE;
}

staticfn boolean
cnf_line_SAVEDIR(char *bufp)
{
#ifdef NOCWD_ASSUMPTIONS
    adjust_prefix(bufp, SAVEPREFIX);
#else /*NOCWD_ASSUMPTIONS*/
#ifdef MICRO
    char *ptr;

    if ((ptr = strchr(bufp, ';')) != 0) {
        *ptr = '\0';
    }

    (void) strncpy(gs.SAVEP, bufp, SAVESIZE - 1);
    append_slash(gs.SAVEP);
#else /* MICRO */
    nhUse(bufp);
#endif /* MICRO */
#endif /*NOCWD_ASSUMPTIONS*/
    return TRUE;
}

staticfn boolean
cnf_line_BONESDIR(char *bufp)
{
#ifdef NOCWD_ASSUMPTIONS
    adjust_prefix(bufp, BONESPREFIX);
#else
    nhUse(bufp);
#endif
    return TRUE;
}

staticfn boolean
cnf_line_DATADIR(char *bufp)
{
#ifdef NOCWD_ASSUMPTIONS
    adjust_prefix(bufp, DATAPREFIX);
#else
    nhUse(bufp);
#endif
    return TRUE;
}

staticfn boolean
cnf_line_SCOREDIR(char *bufp)
{
#ifdef NOCWD_ASSUMPTIONS
    adjust_prefix(bufp, SCOREPREFIX);
#else
    nhUse(bufp);
#endif
    return TRUE;
}

staticfn boolean
cnf_line_LOCKDIR(char *bufp)
{
#ifdef NOCWD_ASSUMPTIONS
    adjust_prefix(bufp, LOCKPREFIX);
#else
    nhUse(bufp);
#endif
    return TRUE;
}

staticfn boolean
cnf_line_CONFIGDIR(char *bufp)
{
#ifdef NOCWD_ASSUMPTIONS
    adjust_prefix(bufp, CONFIGPREFIX);
#else
    nhUse(bufp);
#endif
    return TRUE;
}

staticfn boolean
cnf_line_TROUBLEDIR(char *bufp)
{
#ifdef NOCWD_ASSUMPTIONS
    adjust_prefix(bufp, TROUBLEPREFIX);
#else
    nhUse(bufp);
#endif
    return TRUE;
}

staticfn boolean
cnf_line_NAME(char *bufp)
{
    (void) strncpy(svp.plname, bufp, PL_NSIZ - 1);
    return TRUE;
}

staticfn boolean
cnf_line_ROLE(char *bufp)
{
    int len;

    if ((len = str2role(bufp)) >= 0)
        flags.initrole = len;
    return TRUE;
}

staticfn boolean
cnf_line_dogname(char *bufp)
{
    (void) strncpy(gd.dogname, bufp, PL_PSIZ - 1);
    return TRUE;
}

staticfn boolean
cnf_line_catname(char *bufp)
{
    (void) strncpy(gc.catname, bufp, PL_PSIZ - 1);
    return TRUE;
}
#endif /* SFCTOOL */

#ifdef SYSCF

staticfn boolean
cnf_line_WIZARDS(char *bufp)
{
    if (sysopt.wizards)
        free((genericptr_t) sysopt.wizards);
    sysopt.wizards = dupstr(bufp);
    if (strlen(sysopt.wizards) && strcmp(sysopt.wizards, "*")) {
        /* pre-format WIZARDS list now; it's displayed during a panic
           and since that panic might be due to running out of memory,
           we don't want to risk attempting to allocate any memory then */
        if (sysopt.fmtd_wizard_list)
            free((genericptr_t) sysopt.fmtd_wizard_list);
        sysopt.fmtd_wizard_list = build_english_list(sysopt.wizards);
    }
    return TRUE;
}

staticfn boolean
cnf_line_SHELLERS(char *bufp)
{
    if (sysopt.shellers)
        free((genericptr_t) sysopt.shellers);
    sysopt.shellers = dupstr(bufp);
    return TRUE;
}

staticfn boolean
cnf_line_MSGHANDLER(char *bufp)
{
    if (sysopt.msghandler)
        free((genericptr_t) sysopt.msghandler);
    sysopt.msghandler = dupstr(bufp);
    return TRUE;
}

staticfn boolean
cnf_line_EXPLORERS(char *bufp)
{
    if (sysopt.explorers)
        free((genericptr_t) sysopt.explorers);
    sysopt.explorers = dupstr(bufp);
    return TRUE;
}

staticfn boolean
cnf_line_DEBUGFILES(char *bufp)
{
    /* might already have a vaule from getenv("DEBUGFILES");
       if so, ignore this value from SYSCF */
    if (!sysopt.env_dbgfl) {
        if (sysopt.debugfiles)
            free((genericptr_t) sysopt.debugfiles);
        sysopt.debugfiles = dupstr(bufp);
    }
    return TRUE;
}

staticfn boolean
cnf_line_DUMPLOGFILE(char *bufp)
{
#ifdef DUMPLOG
    if (sysopt.dumplogfile)
        free((genericptr_t) sysopt.dumplogfile);
    sysopt.dumplogfile = dupstr(bufp);
#else
    nhUse(bufp);
#endif /*DUMPLOG*/
    return TRUE;
}

staticfn boolean
cnf_line_GENERICUSERS(char *bufp)
{
    if (sysopt.genericusers)
        free((genericptr_t) sysopt.genericusers);
    sysopt.genericusers = dupstr(bufp);
    return TRUE;
}

staticfn boolean
cnf_line_BONES_POOLS(char *bufp)
{
    /* max value of 10 guarantees (N % bones.pools) will be one digit
       so we don't lose control of the length of bones file names */
    int n = atoi(bufp);

    sysopt.bones_pools = (n <= 0) ? 0 : min(n, 10);
    /* note: right now bones_pools==0 is the same as bones_pools==1,
       but we could change that and make bones_pools==0 become an
       indicator to suppress bones usage altogether */
    return TRUE;
}

staticfn boolean
cnf_line_SUPPORT(char *bufp)
{
    if (sysopt.support)
        free((genericptr_t) sysopt.support);
    sysopt.support = dupstr(bufp);
    return TRUE;
}

staticfn boolean
cnf_line_RECOVER(char *bufp)
{
    if (sysopt.recover)
        free((genericptr_t) sysopt.recover);
    sysopt.recover = dupstr(bufp);
    return TRUE;
}

staticfn boolean
cnf_line_CHECK_SAVE_UID(char *bufp)
{
    int n = atoi(bufp);

    sysopt.check_save_uid = n;
    return TRUE;
}

staticfn boolean
cnf_line_CHECK_PLNAME(char *bufp)
{
    int n = atoi(bufp);

    sysopt.check_plname = n;
    return TRUE;
}

staticfn boolean
cnf_line_SEDUCE(char *bufp)
{
    int n = !!atoi(bufp); /* XXX this could be tighter */
#ifdef SYSCF
    int src = iflags.parse_config_file_src;
    boolean in_sysconf = (src == set_in_sysconf);
#else
    boolean in_sysconf = FALSE;
#endif

    /* allow anyone to disable it but can only enable it in sysconf
       or as a no-op for the user when sysconf hasn't disabled it */
    if (!in_sysconf && !sysopt.seduce && n != 0) {
        config_error_add("Illegal value in SEDUCE");
        n = 0;
    }
    sysopt.seduce = n;
    sysopt_seduce_set(sysopt.seduce);
    return TRUE;
}

staticfn boolean
cnf_line_HIDEUSAGE(char *bufp)
{
    int n = !!atoi(bufp);

    sysopt.hideusage = n;
    return TRUE;
}

staticfn boolean
cnf_line_MAXPLAYERS(char *bufp)
{
    int n = atoi(bufp);

    /* XXX to get more than 25, need to rewrite all lock code */
    if (n < 0 || n > 25) {
        config_error_add("Illegal value in MAXPLAYERS (maximum is 25)");
        n = 5;
    }
    sysopt.maxplayers = n;
    return TRUE;
}

staticfn boolean
cnf_line_PERSMAX(char *bufp)
{
    int n = atoi(bufp);

    if (n < 1) {
        config_error_add("Illegal value in PERSMAX (minimum is 1)");
        n = 0;
    }
    sysopt.persmax = n;
    return TRUE;
}

staticfn boolean
cnf_line_PERS_IS_UID(char *bufp)
{
    int n = atoi(bufp);

    if (n != 0 && n != 1) {
        config_error_add("Illegal value in PERS_IS_UID (must be 0 or 1)");
        n = 0;
    }
    sysopt.pers_is_uid = n;
    return TRUE;
}

staticfn boolean
cnf_line_ENTRYMAX(char *bufp)
{
    int n = atoi(bufp);

    if (n < 10) {
        config_error_add("Illegal value in ENTRYMAX (minimum is 10)");
        n = 10;
    }
    sysopt.entrymax = n;
    return TRUE;
}

staticfn boolean
cnf_line_POINTSMIN(char *bufp)
{
    int n = atoi(bufp);

    if (n < 1) {
        config_error_add("Illegal value in POINTSMIN (minimum is 1)");
        n = 100;
    }
    sysopt.pointsmin = n;
    return TRUE;
}

staticfn boolean
cnf_line_MAX_STATUENAME_RANK(char *bufp)
{
    int n = atoi(bufp);

    if (n < 1) {
        config_error_add("Illegal value in MAX_STATUENAME_RANK"
                         " (minimum is 1)");
        n = 10;
    }
    sysopt.tt_oname_maxrank = n;
    return TRUE;
}

staticfn boolean
cnf_line_LIVELOG(char *bufp)
{
    /* using 0 for base accepts "dddd" as decimal provided that first 'd'
       isn't '0', "0xhhhh" as hexadecimal, and "0oooo" as octal; ignores
       any trailing junk, including '8' or '9' for leading '0' octal */
    long L = strtol(bufp, NULL, 0);

    if (L < 0L || L > 0xffffL) {
        config_error_add("Illegal value for LIVELOG"
                         " (must be between 0 and 0xFFFF).");
        return 0;
    }
    sysopt.livelog = L;
    return TRUE;
}

staticfn boolean
cnf_line_PANICTRACE_LIBC(char *bufp)
{
    int n = atoi(bufp);

#if defined(PANICTRACE) && defined(PANICTRACE_LIBC)
    if (n < 0 || n > 2) {
        config_error_add("Illegal value in PANICTRACE_LIBC (not 0,1,2)");
        n = 0;
    }
#endif
    sysopt.panictrace_libc = n;
    return TRUE;
}

staticfn boolean
cnf_line_PANICTRACE_GDB(char *bufp)
{
    int n = atoi(bufp);

#if defined(PANICTRACE)
    if (n < 0 || n > 2) {
        config_error_add("Illegal value in PANICTRACE_GDB (not 0,1,2)");
        n = 0;
    }
#endif
    sysopt.panictrace_gdb = n;
    return TRUE;
}

staticfn boolean
cnf_line_GDBPATH(char *bufp)
{
#if defined(PANICTRACE) && !defined(VMS)
    if (!file_exists(bufp)) {
        config_error_add("File specified in GDBPATH does not exist");
        return FALSE;
    }
#endif
    if (sysopt.gdbpath)
        free((genericptr_t) sysopt.gdbpath);
    sysopt.gdbpath = dupstr(bufp);
    return TRUE;
}

staticfn boolean
cnf_line_GREPPATH(char *bufp)
{
#if defined(PANICTRACE) && !defined(VMS)
    if (!file_exists(bufp)) {
        config_error_add("File specified in GREPPATH does not exist");
        return FALSE;
    }
#endif
    if (sysopt.greppath)
        free((genericptr_t) sysopt.greppath);
    sysopt.greppath = dupstr(bufp);
    return TRUE;
}

staticfn boolean
cnf_line_CRASHREPORTURL(char *bufp)
{
    if (sysopt.crashreporturl)
        free((genericptr_t) sysopt.crashreporturl);
    sysopt.crashreporturl = dupstr(bufp);
    return TRUE;
}

staticfn boolean
cnf_line_ACCESSIBILITY(char *bufp)
{
    int n = atoi(bufp);

    if (n < 0 || n > 1) {
        config_error_add("Illegal value in ACCESSIBILITY (not 0,1)");
        n = 0;
    }
    sysopt.accessibility = n;
    return TRUE;
}

staticfn boolean
cnf_line_PORTABLE_DEVICE_PATHS(char *bufp)
{
#ifdef WIN32
    int n = atoi(bufp);

    if (n < 0 || n > 1) {
        config_error_add("Illegal value in PORTABLE_DEVICE_PATHS"
                         " (not 0 or 1)");
        n = 0;
    }
    sysopt.portable_device_paths = n;
#else   /* Windows-only directive encountered by non-Windows config */
    nhUse(bufp);
    config_error_add("PORTABLE_DEVICE_PATHS is not supported");
#endif
    return TRUE;
}
#endif  /* SYSCF */

#ifndef SFCTOOL

staticfn boolean
cnf_line_BOULDER(char *bufp)
{
    (void) get_uchars(bufp, &go.ov_primary_syms[SYM_BOULDER + SYM_OFF_X],
                      TRUE, 1, "BOULDER");
    return TRUE;
}

staticfn boolean
cnf_line_MENUCOLOR(char *bufp)
{
    return add_menu_coloring(bufp);
}

staticfn boolean
cnf_line_HILITE_STATUS(char *bufp)
{
#ifdef STATUS_HILITES
    return parse_status_hl1(bufp, TRUE);
#else
    nhUse(bufp);
    return TRUE;
#endif
}

staticfn boolean
cnf_line_WARNINGS(char *bufp)
{
    uchar translate[MAXPCHARS];

    (void) get_uchars(bufp, translate, FALSE, WARNCOUNT, "WARNINGS");
    assign_warnings(translate);
    return TRUE;
}

staticfn boolean
cnf_line_ROGUESYMBOLS(char *bufp)
{
    if (parsesymbols(bufp, ROGUESET)) {
        switch_symbols(TRUE);
        return TRUE;
    }
    config_error_add("Error in ROGUESYMBOLS definition '%s'", bufp);
    return FALSE;
}

staticfn boolean
cnf_line_SYMBOLS(char *bufp)
{
    if (parsesymbols(bufp, PRIMARYSET)) {
        switch_symbols(TRUE);
        return TRUE;
    }
    if (!config_unmatched_ignored())
        config_error_add("Error in SYMBOLS definition '%s'", bufp);
    return FALSE;
}

staticfn boolean
cnf_line_WIZKIT(char *bufp)
{
    (void) strncpy(gw.wizkit, bufp, WIZKIT_MAX - 1);
    return TRUE;
}

#ifdef USER_SOUNDS
staticfn boolean
cnf_line_SOUNDDIR(char *bufp)
{
    if (sounddir)
        free((genericptr_t) sounddir);
    sounddir = dupstr(bufp);
    return TRUE;
}

staticfn boolean
cnf_line_SOUND(char *bufp)
{
    add_sound_mapping(bufp);
    return TRUE;
}
#endif /*USER_SOUNDS*/

staticfn boolean
cnf_line_QT_TILEWIDTH(char *bufp)
{
#ifdef QT_GRAPHICS
    extern char *qt_tilewidth;

    if (qt_tilewidth == NULL)
        qt_tilewidth = dupstr(bufp);
#else
    nhUse(bufp);
#endif
    return TRUE;
}

staticfn boolean
cnf_line_QT_TILEHEIGHT(char *bufp)
{
#ifdef QT_GRAPHICS
    extern char *qt_tileheight;

    if (qt_tileheight == NULL)
        qt_tileheight = dupstr(bufp);
#else
    nhUse(bufp);
#endif
    return TRUE;
}

staticfn boolean
cnf_line_QT_FONTSIZE(char *bufp)
{
#ifdef QT_GRAPHICS
    extern char *qt_fontsize;

    if (qt_fontsize == NULL)
        qt_fontsize = dupstr(bufp);
#else
    nhUse(bufp);
#endif
    return TRUE;
}

staticfn boolean
cnf_line_QT_COMPACT(char *bufp)
{
#ifdef QT_GRAPHICS
    extern int qt_compact_mode;

    qt_compact_mode = atoi(bufp);
#else
    nhUse(bufp);
#endif
    return TRUE;
}
#endif /* SFCTOOL */

typedef boolean (*config_line_stmt_func)(char *);

/* normal */
#define CNFL_N(n, l) { #n, l, FALSE, FALSE, cnf_line_##n }
/* normal, alias */
#define CNFL_NA(n, l, f) { #n, l, FALSE, FALSE, cnf_line_##f }
/* sysconf only */
#define CNFL_S(n, l) { #n, l, TRUE, FALSE, cnf_line_##n }

static const struct match_config_line_stmt {
    const char *name;
    int len;
    boolean syscnf_only;
    boolean origbuf;
    config_line_stmt_func fn;
} config_line_stmt[] = {
#ifndef SFCTOOL
    /* OPTIONS handled separately */
    { "OPTIONS", 4, FALSE, TRUE, cnf_line_OPTIONS },
    CNFL_N(AUTOPICKUP_EXCEPTION, 5),
    CNFL_N(BINDINGS, 4),
    CNFL_N(AUTOCOMPLETE, 5),
    CNFL_N(MSGTYPE, 7),
    CNFL_N(HACKDIR, 4),
    CNFL_N(LEVELDIR, 4),
    CNFL_NA(LEVELS, 4, LEVELDIR),
    CNFL_N(SAVEDIR, 4),
    CNFL_N(BONESDIR, 5),
    CNFL_N(DATADIR, 4),
    CNFL_N(SCOREDIR, 4),
    CNFL_N(LOCKDIR, 4),
    CNFL_N(CONFIGDIR, 4),
    CNFL_N(TROUBLEDIR, 4),
    CNFL_N(NAME, 4),
    CNFL_N(ROLE, 4),
    CNFL_NA(CHARACTER, 4, ROLE),
    CNFL_N(dogname, 3),
    CNFL_N(catname, 3),
#endif /* SFCTOOL */
#ifdef SYSCF
    CNFL_S(WIZARDS, 7),
    CNFL_S(SHELLERS, 8),
    CNFL_S(MSGHANDLER, 9),
    CNFL_S(EXPLORERS, 7),
    CNFL_S(DEBUGFILES, 5),
    CNFL_S(DUMPLOGFILE, 7),
    CNFL_S(GENERICUSERS, 12),
    CNFL_S(BONES_POOLS, 10),
    CNFL_S(SUPPORT, 7),
    CNFL_S(RECOVER, 7),
    CNFL_S(CHECK_SAVE_UID, 14),
    CNFL_S(CHECK_PLNAME, 12),
    CNFL_S(SEDUCE, 6),
    CNFL_S(HIDEUSAGE, 9),
    CNFL_S(MAXPLAYERS, 10),
    CNFL_S(PERSMAX, 7),
    CNFL_S(PERS_IS_UID, 11),
    CNFL_S(ENTRYMAX, 8),
    CNFL_S(POINTSMIN, 9),
    CNFL_S(MAX_STATUENAME_RANK, 10),
    CNFL_S(LIVELOG, 7),
    CNFL_S(PANICTRACE_LIBC, 15),
    CNFL_S(PANICTRACE_GDB, 14),
    CNFL_S(CRASHREPORTURL, 13),
    CNFL_S(GDBPATH, 7),
    CNFL_S(GREPPATH, 7),
    CNFL_S(ACCESSIBILITY, 13),
    CNFL_S(PORTABLE_DEVICE_PATHS, 8),
#endif /*SYSCF*/
#ifndef SFCTOOL
    CNFL_N(BOULDER, 3),
    CNFL_N(MENUCOLOR, 9),
    CNFL_N(HILITE_STATUS, 6),
    CNFL_N(WARNINGS, 5),
    CNFL_N(ROGUESYMBOLS, 4),
    CNFL_N(SYMBOLS, 4),
    CNFL_N(WIZKIT, 6),
#ifdef USER_SOUNDS
    CNFL_N(SOUNDDIR, 8),
    CNFL_N(SOUND, 5),
#endif /*USER_SOUNDS*/
    CNFL_N(QT_TILEWIDTH, 12),
    CNFL_N(QT_TILEHEIGHT, 13),
    CNFL_N(QT_FONTSIZE, 11),
    CNFL_N(QT_COMPACT, 10)
#endif /* SFCTOOL */
};

#undef CNFL_N
#undef CNFL_NA
#undef CNFL_S

static boolean disregarded_config_lines[SIZE(config_line_stmt)];

boolean
parse_config_line(char *origbuf)
{
#if defined(MICRO) && !defined(NOCWD_ASSUMPTIONS)
    static boolean ramdisk_specified = FALSE;
#endif
#ifdef SYSCF
    int src = iflags.parse_config_file_src;
    boolean in_sysconf = (src == set_in_sysconf);
#endif
    char *bufp, buf[4 * BUFSZ];
    int i;

    while (*origbuf == ' ' || *origbuf == '\t') /* skip leading whitespace */
        ++origbuf;                   /* (caller probably already did this) */
    (void) strncpy(buf, origbuf, sizeof buf - 1);
    buf[sizeof buf - 1] = '\0'; /* strncpy not guaranteed to NUL terminate */
    /* convert any tab to space, condense consecutive spaces into one,
       remove leading and trailing spaces (exception: if there is nothing
       but spaces, one of them will be kept even though it leads/trails) */
    mungspaces(buf);

    /* find the '=' or ':' */
    bufp = find_optparam(buf);
    if (!bufp) {
        if (!ignore_statement_errors)
            config_error_add("Not a config statement, missing '='");
        return FALSE;
    }
    /* skip past '=', then space between it and value, if any */
    ++bufp;
    if (*bufp == ' ')
        ++bufp;

    for (i = 0; i < SIZE(config_line_stmt); i++) {
#ifdef SYSCF
        if (config_line_stmt[i].syscnf_only && !in_sysconf)
            continue;
#endif
        if (match_varname(buf, config_line_stmt[i].name,
                          config_line_stmt[i].len)) {
            char *parm = config_line_stmt[i].origbuf ? origbuf : bufp;

            if (!disregarded_config_lines[i])
                return config_line_stmt[i].fn(parm);
        }
    }

    if (!ignore_errors_on_unmatched)
        config_error_add("Unknown config statement");
    return FALSE;
}

#ifdef USER_SOUNDS
boolean
can_read_file(const char *filename)
{
    return (boolean) (access(filename, 4) == 0);
}
#endif /* USER_SOUNDS */

struct _config_error_errmsg {
    int line_num;
    char *errormsg;
    struct _config_error_errmsg *next;
};

struct _config_error_frame {
    int line_num;
    int num_errors;
    boolean origline_shown;
    boolean fromfile;
    boolean secure;
    char origline[4 * BUFSZ];
    char source[BUFSZ];
    struct _config_error_frame *next;
};

static struct _config_error_frame *config_error_data = 0;
static struct _config_error_errmsg *config_error_msg = 0;

void
config_error_init(boolean from_file, const char *sourcename, boolean secure)
{
    struct _config_error_frame *tmp = (struct _config_error_frame *)
                                                           alloc(sizeof *tmp);

    tmp->line_num = 0;
    tmp->num_errors = 0;
    tmp->origline_shown = FALSE;
    tmp->fromfile = from_file;
    tmp->secure = secure;
    tmp->origline[0] = '\0';
    if (sourcename && sourcename[0]) {
        (void) strncpy(tmp->source, sourcename, sizeof (tmp->source) - 1);
        tmp->source[sizeof (tmp->source) - 1] = '\0';
    } else
        tmp->source[0] = '\0';

    tmp->next = config_error_data;
    config_error_data = tmp;
    program_state.config_error_ready = TRUE;
}

staticfn boolean
config_error_nextline(const char *line)
{
    struct _config_error_frame *ced = config_error_data;

    if (!ced)
        return FALSE;

    if (ced->num_errors && ced->secure)
        return FALSE;

    ced->line_num++;
    ced->origline_shown = FALSE;
    if (line && line[0]) {
        (void) strncpy(ced->origline, line, sizeof (ced->origline) - 1);
        ced->origline[sizeof (ced->origline) - 1] = '\0';
    } else
        ced->origline[0] = '\0';

    return TRUE;
}

#ifndef SFCTOOL
int
l_get_config_errors(lua_State *L)
{
    struct _config_error_errmsg *dat = config_error_msg;
    struct _config_error_errmsg *tmp;
    int idx = 1;

    lua_newtable(L);

    while (dat) {
        lua_pushinteger(L, idx++);
        lua_newtable(L);
        nhl_add_table_entry_int(L, "line", dat->line_num);
        nhl_add_table_entry_str(L, "error", dat->errormsg);
        lua_settable(L, -3);
        tmp = dat->next;
        free(dat->errormsg);
        dat->errormsg = (char *) 0;
        free(dat);
        dat = tmp;
    }
    config_error_msg = (struct _config_error_errmsg *) 0;

    return 1;
}
#endif /* SFCTOOL */

/* varargs 'config_error_add()' moved to pline.c */
void
config_erradd(const char *buf)
{
    char lineno[QBUFSZ];
    const char *punct;

    if (!buf || !*buf)
        buf = "Unknown error";

    /* if buf[] doesn't end in a period, exclamation point, or question mark,
       we'll include a period (in the message, not appended to buf[]) */
    punct = c_eos((char *) buf) - 1; /* eos(buf)-1 is valid */
    punct = strchr(".!?", *punct) ? "" : ".";

    if (!program_state.config_error_ready) {
        /* either very early, where pline() will use raw_print(), or
           player gave bad value when prompted by interactive 'O' command */
        pline("%s%s%s", !iflags.window_inited ? "config_error_add: " : "",
              buf, punct);
        wait_synch();
        return;
    }

    if (iflags.in_lua) {
        struct _config_error_errmsg *dat
                         = (struct _config_error_errmsg *) alloc(sizeof *dat);

        dat->next = config_error_msg;
        dat->line_num = config_error_data->line_num;
        dat->errormsg = dupstr(buf);
        config_error_msg = dat;
        return;
    }

    config_error_data->num_errors++;
    if (!config_error_data->origline_shown && !config_error_data->secure) {
        pline("\n%s", config_error_data->origline);
        config_error_data->origline_shown = TRUE;
    }
    if (config_error_data->line_num > 0 && !config_error_data->secure) {
        Sprintf(lineno, "Line %d: ", config_error_data->line_num);
    } else
        lineno[0] = '\0';

    pline("%s %s%s%s", config_error_data->secure ? "Error:" : " *",
          lineno, buf, punct);
}

int
config_error_done(void)
{
    int n;
    struct _config_error_frame *tmp = config_error_data;

    if (!config_error_data)
        return 0;
    n = config_error_data->num_errors;
#ifndef USER_SOUNDS
    if (gn.no_sound_notified > 0) {
        /* no USER_SOUNDS; config_error_add() was called once for first
           SOUND or SOUNDDIR entry seen, then skipped for any others;
           include those skipped ones in the total error count */
        n += (gn.no_sound_notified - 1);
        gn.no_sound_notified = 0;
    }
#endif
    if (n) {
        boolean cmdline = !strcmp(config_error_data->source, "command line");

        pline("\n%d error%s %s %s.\n", n, plur(n), cmdline ? "on" : "in",
              *config_error_data->source ? config_error_data->source
                                         : configfile);
        wait_synch();
    }
    config_error_data = tmp->next;
    free(tmp);
    program_state.config_error_ready = (config_error_data != 0);
    return n;
}

boolean
read_config_file(const char *filename, int src)
{
    FILE *fp;
    boolean rv = TRUE;

    if (!(fp = fopen_config_file(filename, src)))
        return FALSE;
#ifndef SFCTOOL
    /* begin detection of duplicate configfile options */
    reset_duplicate_opt_detection();
#endif /* SFCTOOL */
    free_config_sections();
    iflags.parse_config_file_src = src;

    rv = parse_conf_file(fp, parse_config_line);
    (void) fclose(fp);

    free_config_sections();
#ifndef SFCTOOL
    /* turn off detection of duplicate configfile options */
    reset_duplicate_opt_detection();
#endif /* SFCTOOL */
    return rv;
}

struct _cnf_parser_state {
    char *inbuf;
    unsigned inbufsz;
    int rv;
    char *ep;
    char *buf;
    boolean skip, morelines;
    boolean cont;
    boolean pbreak;
};

/* Initialize config parser data */
staticfn void
cnf_parser_init(struct _cnf_parser_state *parser)
{
    parser->rv = TRUE; /* assume successful parse */
    parser->ep = parser->buf = (char *) 0;
    parser->skip = FALSE;
    parser->morelines = FALSE;
    parser->inbufsz = 4 * BUFSZ;
    parser->inbuf = (char *) alloc(parser->inbufsz);
    parser->cont = FALSE;
    parser->pbreak = FALSE;
    memset(parser->inbuf, 0, parser->inbufsz);
}

/* caller has finished with 'parser' (except for 'rv' so leave that intact) */
staticfn void
cnf_parser_done(struct _cnf_parser_state *parser)
{
    parser->ep = 0; /* points into parser->inbuf, so becoming stale */
    if (parser->inbuf)
        free(parser->inbuf), parser->inbuf = 0;
    if (parser->buf)
        free(parser->buf), parser->buf = 0;
}

/*
 * Parse config buffer, handling comments, empty lines, config sections,
 * CHOOSE, and line continuation, calling proc for every valid line.
 *
 * Continued lines are merged together with one space in between.
 */
staticfn void
parse_conf_buf(struct _cnf_parser_state *p, boolean (*proc)(char *arg))
{
    p->cont = FALSE;
    p->pbreak = FALSE;
    p->ep = strchr(p->inbuf, '\n');
    if (p->skip) { /* in case previous line was too long */
        if (p->ep)
            p->skip = FALSE; /* found newline; next line is normal */
    } else {
        if (!p->ep) {  /* newline missing */
            if (strlen(p->inbuf) < (p->inbufsz - 2)) {
                /* likely the last line of file is just
                   missing a newline; process it anyway  */
                p->ep = eos(p->inbuf);
            } else {
                config_error_add("Line too long, skipping");
                p->skip = TRUE; /* discard next fgets */
            }
        } else {
            *p->ep = '\0'; /* remove newline */
        }
        if (p->ep) {
            char *tmpbuf = (char *) 0;
            int len;
            boolean ignoreline = FALSE;
            boolean oldline = FALSE;

            /* line continuation (trailing '\') */
            p->morelines = (--p->ep >= p->inbuf && *p->ep == '\\');
            if (p->morelines)
                *p->ep = '\0';

            /* trim off spaces at end of line */
            while (p->ep >= p->inbuf
                   && (*p->ep == ' ' || *p->ep == '\t' || *p->ep == '\r'))
                *p->ep-- = '\0';

            if (!config_error_nextline(p->inbuf)) {
                p->rv = FALSE;
                if (p->buf)
                    free(p->buf), p->buf = (char *) 0;
                p->pbreak = TRUE;
                return;
            }

            p->ep = p->inbuf;
            while (*p->ep == ' ' || *p->ep == '\t')
                ++p->ep;

            /* ignore empty lines and full-line comment lines */
            if (!*p->ep || *p->ep == '#')
                ignoreline = TRUE;

            if (p->buf)
                oldline = TRUE;

            /* merge now read line with previous ones, if necessary */
            if (!ignoreline) {
                len = (int) strlen(p->ep) + 1; /* +1: final '\0' */
                if (p->buf)
                    len += (int) strlen(p->buf) + 1; /* +1: space */
                tmpbuf = (char *) alloc(len);
                *tmpbuf = '\0';
                if (p->buf) {
                    Strcat(strcpy(tmpbuf, p->buf), " ");
                    free(p->buf), p->buf = 0;
                }
                p->buf = strcat(tmpbuf, p->ep);
                if (strlen(p->buf) >= p->inbufsz)
                    p->buf[p->inbufsz - 1] = '\0';
            }

            if (p->morelines || (ignoreline && !oldline))
                return;

            if (handle_config_section(p->buf)) {
                free(p->buf), p->buf = (char *) 0;
                return;
            }

            /* from here onwards, we'll handle buf only */

            if (match_varname(p->buf, "CHOOSE", 6)) {
                char *section;
                char *bufp = find_optparam(p->buf);

                if (!bufp) {
                    config_error_add("Format is CHOOSE=section1"
                                     ",section2,...");
                    p->rv = FALSE;
                    free(p->buf), p->buf = (char *) 0;
                    return;
                }
                bufp++;
                if (gc.config_section_chosen)
                    free(gc.config_section_chosen),
                        gc.config_section_chosen = 0;
                section = choose_random_part(bufp, ',');
                if (section) {
                    gc.config_section_chosen = dupstr(section);
                } else {
                    config_error_add("No config section to choose");
                    p->rv = FALSE;
                }
                free(p->buf), p->buf = (char *) 0;
                return;
            }

            if (!(*proc)(p->buf))
                p->rv = FALSE;

            free(p->buf), p->buf = (char *) 0;
        }
    }
}

boolean
parse_conf_str(const char *str, boolean (*proc)(char *arg))
{
    size_t len;
    struct _cnf_parser_state parser;

    cnf_parser_init(&parser);
    free_config_sections();
    config_error_init(FALSE, "parse_conf_str", FALSE);
    while (str && *str) {
        len = 0;
        while (*str && len < (parser.inbufsz-1)) {
            parser.inbuf[len] = *str;
            len++;
            str++;
            if (parser.inbuf[len-1] == '\n')
                break;
        }
        parser.inbuf[len] = '\0';
        parse_conf_buf(&parser, proc);
        if (parser.pbreak)
            break;
    }
    cnf_parser_done(&parser);

    free_config_sections();
    config_error_done();
    return parser.rv;
}

/* parse_conf_file
 *
 * Read from file fp, calling parse_conf_buf for each line.
 */
boolean
parse_conf_file(FILE *fp, boolean (*proc)(char *arg))
{
    struct _cnf_parser_state parser;

    cnf_parser_init(&parser);
    free_config_sections();

    while (fgets(parser.inbuf, parser.inbufsz, fp)) {
        parse_conf_buf(&parser, proc);
        if (parser.pbreak)
            break;
    }
    cnf_parser_done(&parser);

    free_config_sections();
    return parser.rv;
}

DISABLE_WARNING_FORMAT_NONLITERAL

void
config_error_add(const char *str, ...)
{
    va_list the_args;

    va_start(the_args, str);
    vconfig_error_add(str, the_args);
    va_end(the_args);
}

staticfn void
vconfig_error_add(const char *str, va_list the_args)
{ /* start of vconf...() or of nested block in USE_OLDARG's conf...() */
    int vlen = 0;
    char buf[BIGBUFSZ]; /* will be chopped down to BUFSZ-1 if longer */

    vlen = vsnprintf(buf, sizeof buf, str, the_args);
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED) && defined(DEBUG)
    if (vlen >= (int) sizeof buf)
        panic("%s: truncation of buffer at %zu of %d bytes",
              "config_error_add", sizeof buf, vlen);
#else
    nhUse(vlen);
#endif
    buf[BUFSZ - 1] = '\0';
    config_erradd(buf);
}

#ifndef SFCTOOL
void
rcfile(void)
{
    char *opts = 0, *xtraopts = 0;
    const char *envname, *namesrc, *nameval;

    go.opt_phase = environ_opt;
    /* getenv() instead of nhgetenv(): let total length of options be long;
       parseoptions() will check each individually */
    envname = "NETHACKOPTIONS";
    opts = getenv(envname);
    if (!opts) {
        /* fall back to original name; discouraged */
        envname = "HACKOPTIONS";
        opts = getenv(envname);
    }

    if (gc.cmdline_rcfile) {
        namesrc = "command line";
        nameval = gc.cmdline_rcfile;
        xtraopts = opts;
        if (opts && (*opts == '/' || *opts == '\\' || *opts == '@'))
            xtraopts = 0; /* NETHACKOPTIONS is a file name; ignore it */
    } else if (opts && (*opts == '/' || *opts == '\\' || *opts == '@')) {
        /* NETHACKOPTIONS is a file name; use that instead of the default */
        if (*opts == '@')
            ++opts; /* @filename */
        namesrc = envname;
        nameval = opts;
        xtraopts = 0;
    } else {
        /* either no NETHACKOPTIONS or it wasn't a file name;
           read the default configuration file */
        nameval = namesrc = 0;
        xtraopts = opts;
    }

    go.opt_phase = rc_file_opt;
    /* seemingly arbitrary name length restriction is to prevent error
       messages, if any were to be delivered while accessing the file,
       from potentially overflowing buffers */
    if (nameval && (int) strlen(nameval) >= BUFSZ / 2) {
        config_error_init(TRUE, namesrc, FALSE);
        config_error_add(
            "nethackrc file name \"%.40s\"... too long; using default",
            nameval);
        config_error_done();
        nameval = namesrc = 0; /* revert to default nethackrc */
    }

    config_error_init(TRUE, nameval, nameval ? CONFIG_ERROR_SECURE : FALSE);
    (void) read_config_file(nameval, set_in_config);
    config_error_done();
    if (xtraopts) {
        /* NETHACKOPTIONS is present and not a file name */
        go.opt_phase = environ_opt;
        config_error_init(FALSE, envname, FALSE);
        (void) parseoptions(xtraopts, TRUE, FALSE);
        config_error_done();
    }

    if (gc.cmdline_rcfile)
        free((genericptr_t) gc.cmdline_rcfile), gc.cmdline_rcfile = 0;
    /*[end of nethackrc handling]*/
}

void
rcfile_interface_options(void)
{
    allopt_array_init();
    disregard_all_options();
    disregard_all_config_statements();
    heed_this_option(opt_windowtype);
    heed_this_option(opt_soundlib);
    set_ignore_errors_on_unmatched();
    ignore_statement_errors = TRUE;
    rcfile();
    heed_all_config_statements();
    heed_all_options();
    disregard_this_option(opt_windowtype);
    disregard_this_option(opt_soundlib);
    clear_ignore_errors_on_unmatched();
    ignore_statement_errors = FALSE;
}

void
heed_all_config_statements(void)
{
    int i;

    for (i = 0; i < SIZE(disregarded_config_lines); i++) {
        disregarded_config_lines[i] = FALSE;
    }
}
void
disregard_all_config_statements(void)
{
    int i;

    for (i = 0; i < SIZE(disregarded_config_lines); i++) {
        disregarded_config_lines[i] = TRUE;
    }
}
void
heed_this_config_statement(int statement_idx)
{
    if (statement_idx >= 0 && statement_idx < SIZE(disregarded_config_lines))
        disregarded_config_lines[statement_idx] = FALSE;
}
void
disregard_this_config_statement(int statement_idx)
{
    if (statement_idx >= 0 && statement_idx < SIZE(disregarded_config_lines))
        disregarded_config_lines[statement_idx] = TRUE;
}

void
clear_ignore_errors_on_unmatched(void)
{
    ignore_errors_on_unmatched = FALSE;
}
void
set_ignore_errors_on_unmatched(void)
{
    ignore_errors_on_unmatched = TRUE;
}
boolean
config_unmatched_ignored(void)
{
    if (ignore_errors_on_unmatched)
        return TRUE;
    return FALSE;
}
#endif /* SFCTOOL */

#ifdef SYSCF
#ifdef SYSCF_FILE
void
assure_syscf_file(void)
{
    int fd;

#ifdef WIN32
    /* We are checking that the sysconf exists ... lock the path */
    fqn_prefix_locked[SYSCONFPREFIX] = TRUE;
#endif
    /*
     * All we really care about is the end result - can we read the file?
     * So just check that directly.
     *
     * Not tested on most of the old platforms (which don't attempt
     * to implement SYSCF).
     * Some ports don't like open()'s optional third argument;
     * VMS overrides open() usage with a macro which requires it.
     */
#ifndef VMS
#if defined(NOCWD_ASSUMPTIONS) && defined(WIN32)
    fd = open(fqname(SYSCF_FILE, SYSCONFPREFIX, 0), O_RDONLY);
#else
    fd = open(SYSCF_FILE, O_RDONLY);
#endif
#else   /* VMS */
    fd = open(SYSCF_FILE, O_RDONLY, 0);
#endif  /* VMS */
    if (fd >= 0) {
        /* readable */
        close(fd);
        return;
    }
#ifndef SFCTOOL
    if (gd.deferred_showpaths)
        do_deferred_showpaths(1); /* does not return */
#endif
    raw_printf("Unable to open SYSCF_FILE.\n");
    exit(EXIT_FAILURE);
}

#endif /* SYSCF_FILE */
#endif /* SYSCF */

/* ----------  END CONFIG FILE HANDLING ----------- */

/*cfgfiles.c*/

