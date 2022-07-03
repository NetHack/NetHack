/* NetHack 3.7	options.c	$NHDT-Date: 1655932898 2022/06/22 21:21:38 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.569 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2008. */
/* NetHack may be freely redistributed.  See license for details. */

#ifdef OPTION_LISTS_ONLY /* (AMIGA) external program for opt lists */
#include "config.h"
#include "objclass.h"
#include "flag.h"
NEARDATA struct flag flags; /* provide linkage */
NEARDATA struct instance_flags iflags; /* provide linkage */
#define static
#else
#include "hack.h"
#include "tcap.h"
#include <ctype.h>
#endif

#define BACKWARD_COMPAT

/* whether the 'msg_window' option is used to control ^P behavior */
#if defined(TTY_GRAPHICS) || defined(CURSES_GRAPHICS)
#define PREV_MSGS 1
#else
#define PREV_MSGS 0
#endif

/*
 *  NOTE:  If you add (or delete) an option, please review the following:
 *             doc/options.txt
 *
 *         It contains how-to info and outlines some required/suggested
 *         updates that should accompany your change.
 */

/*
 * include/optlist.h is utilized 3 successive times, for 3 different
 * objectives.
 *
 * The first time is with NHOPT_PROTO defined, to produce and include
 * the prototypes for the individual option processing functions.
 *
 * The second time is with NHOPT_ENUM defined, to produce the enum values
 * for the individual options that are used throughout options processing.
 * They are generally opt_optname, where optname is the name of the option.
 *
 * The third time is with NHOPT_PARSE defined, to produce the initializers
 * to fill out the allopt[] array of options (both boolean and compound).
 *
 */

#define NHOPT_PROTO
#include "optlist.h"
#undef NHOPT_PROTO

#define NHOPT_ENUM
enum opt {
    opt_prefix_only = -1,
#include "optlist.h"
    OPTCOUNT
};
#undef NHOPT_ENUM

#define NHOPT_PARSE
static struct allopt_t allopt_init[] = {
#include "optlist.h"
    {(const char *) 0, 0, 0, 0, set_in_sysconf, BoolOpt,
     No, No, No, No, 0, (boolean *) 0,
     (int (*)(int, int, boolean, char *, char *)) 0,
     (char *) 0, (const char *) 0, (const char *) 0, 0, 0, 0}
};
#undef NHOPT_PARSE


#if defined(USE_TILES) && defined(DEFAULT_WC_TILED_MAP)
#define PREFER_TILED TRUE
#else
#define PREFER_TILED FALSE
#endif

#define PILE_LIMIT_DFLT 5
#define rolestring(val, array, field) \
    ((val >= 0) ? array[val].field : (val == ROLE_RANDOM) ? randomrole : none)


enum window_option_types {
    MESSAGE_OPTION = 1,
    STATUS_OPTION,
    MAP_OPTION,
    MENU_OPTION,
    TEXT_OPTION
};

enum {optn_silenterr = -1, optn_err = 0, optn_ok};
enum requests {do_nothing, do_init, do_set, do_handler, get_val};

static struct allopt_t allopt[SIZE(allopt_init)];

#ifndef OPTION_LISTS_ONLY

/* use rest of file */

extern char configfile[]; /* for messages */
extern struct symparse loadsyms[];
#if defined(TOS) && defined(TEXTCOLOR)
extern boolean colors_changed;  /* in tos.c */
#endif
#ifdef VIDEOSHADES
extern char *shade[3];          /* in sys/msdos/video.c */
extern char ttycolors[CLR_MAX]; /* in sys/msdos/video.c */
#endif

static char empty_optstr[] = { '\0' };
boolean duplicate, using_alias;

static const char def_inv_order[MAXOCLASSES] = {
    COIN_CLASS, AMULET_CLASS, WEAPON_CLASS, ARMOR_CLASS, FOOD_CLASS,
    SCROLL_CLASS, SPBOOK_CLASS, POTION_CLASS, RING_CLASS, WAND_CLASS,
    TOOL_CLASS, GEM_CLASS, ROCK_CLASS, BALL_CLASS, CHAIN_CLASS, 0,
};

static const char none[] = "(none)", randomrole[] = "random",
                  to_be_done[] = "(to be done)",
                  defopt[] = "default", defbrief[] = "def";

/* paranoia[] - used by parseoptions() and handler_paranoid_confirmation() */
static const struct paranoia_opts {
    int flagmask;        /* which paranoid option */
    const char *argname; /* primary name */
    int argMinLen;       /* minimum number of letters to match */
    const char *synonym; /* alternate name (optional) */
    int synMinLen;
    const char *explain; /* for interactive menu */
} paranoia[] = {
    /* there are some initial-letter conflicts: "a"ttack vs "a"ll, "attack"
       takes precedence and "all" isn't present in the interactive menu,
       and "d"ie vs "d"eath, synonyms for each other so doesn't matter;
       (also "p"ray vs "P"aranoia, "pray" takes precedence since "Paranoia"
       is just a synonym for "Confirm"); "b"ones vs "br"eak-wand, the
       latter requires at least two letters; "e"at vs "ex"plore,
       "cont"inue eating vs "C"onfirm; "wand"-break vs "Were"-change,
       both require at least two letters during config processing and use
       case-senstivity for 'O's interactive menu */
    { PARANOID_CONFIRM, "Confirm", 1, "Paranoia", 2,
      "for \"yes\" confirmations, require \"no\" to reject" },
    { PARANOID_QUIT, "quit", 1, "explore", 2,
      "yes vs y to quit or to enter explore mode" },
    { PARANOID_DIE, "die", 1, "death", 2,
      "yes vs y to die (explore mode or debug mode)" },
    { PARANOID_BONES, "bones", 1, 0, 0,
      "yes vs y to save bones data when dying in debug mode" },
    { PARANOID_HIT, "attack", 1, "hit", 1,
      "yes vs y to attack a peaceful monster" },
    { PARANOID_BREAKWAND, "wand-break", 2, "break-wand", 2,
      "yes vs y to break a wand via (a)pply" },
    { PARANOID_EATING, "eat", 1, "continue", 4,
      "yes vs y to continue eating after first bite when satiated" },
    { PARANOID_WERECHANGE, "Were-change", 2, (const char *) 0, 0,
      "yes vs y to change form when lycanthropy is controllable" },
    { PARANOID_PRAY, "pray", 1, 0, 0,
      "y to pray (supersedes old \"prayconfirm\" option)" },
    { PARANOID_REMOVE, "Remove", 1, "Takeoff", 1,
      "always pick from inventory for Remove and Takeoff" },
    { PARANOID_SWIM, "swim", 1, 0, 0,
      "avoid walking into lava or water" },
    /* for config file parsing; interactive menu skips these */
    { 0, "none", 4, 0, 0, 0 }, /* require full word match */
    { ~0, "all", 3, 0, 0, 0 }, /* ditto */
};

static NEARDATA const char *menutype[][3] = { /* 'menustyle' settings */
    { "traditional",  "[prompt for object class(es), then",
                      " ask y/n for each item in those classes]" },
    { "combination",  "[prompt for object class(es), then",
                      " use menu for items in those classes]" },
    { "full",         "[use menu to choose class(es), then",
                      " use another menu for items in those]" },
    { "partial",      "[skip class filtering; always",
                      " use menu of all available items]" }
};
#if PREV_MSGS /* tty supports all four settings, curses just final two */
static NEARDATA const char *msgwind[][3] = { /* 'msg_window' settings */
    { "single",       "[show one old message at a time,",
                      " most recent first]" },
    { "combination",  "[for consecutive ^P requests, use",
                      " 'single' for first two, then 'full']" },
    { "full",         "[show all available messages,",
                      " oldest first and most recent last]" },
    { "reversed",     "[show all available messages,",
                      " most recent first]" }
};
#endif
/* autounlock settings */
static NEARDATA const char *unlocktypes[][2] = {
    { "none",      "" },
    { "untrap",    "(might fail)" },
    { "apply-key", "" },
    { "kick",      "(doors only)" },
    { "force",     "(chests/boxes only)" },
};
static NEARDATA const char *burdentype[] = {
    "unencumbered", "burdened",     "stressed",
    "strained",     "overtaxed",    "overloaded"
};
static NEARDATA const char *runmodes[] = {
    "teleport",     "run",          "walk",     "crawl"
};
static NEARDATA const char *sortltype[] = {
    "none",         "loot",         "full"
};

/*
 * Default menu manipulation command accelerators.  These may _not_ be:
 *
 *      + a number or '#' - reserved for counts
 *      + an upper or lower case US ASCII letter - used for accelerators
 *      + ESC - reserved for escaping the menu
 *      + NULL, CR or LF - reserved for commiting the selection(s).  NULL
 *        is kind of odd, but the tty's xwaitforspace() will return it if
 *        someone hits a <ret>.
 *      + a default object class symbol - used for object class accelerators
 *
 * Standard letters (for now) are:
 *
 *              <  back 1 page
 *              >  forward 1 page
 *              ^  first page
 *              |  last page
 *              :  search
 *
 *              page            all
 *               ,    select     .
 *               \    deselect   -
 *               ~    invert     @
 *
 * The command name list is duplicated in the compopt array.
 */
typedef struct {
    const char *name;
    char cmd;
    const char *desc;
} menu_cmd_t;

static const menu_cmd_t default_menu_cmd_info[] = {
    { "menu_next_page",     MENU_NEXT_PAGE,     "Go to next page" },
    { "menu_previous_page", MENU_PREVIOUS_PAGE, "Go to previous page" },
    { "menu_first_page",    MENU_FIRST_PAGE,    "Go to first page" },
    { "menu_last_page",     MENU_LAST_PAGE,     "Go to last page" },
    { "menu_select_all",    MENU_SELECT_ALL,
                            "Select all items in entire menu" },
    { "menu_invert_all",    MENU_INVERT_ALL,
                            "Invert selection for all items" },
    { "menu_deselect_all",  MENU_UNSELECT_ALL,
                            "Unselect all items in entire menu" },
    { "menu_select_page",   MENU_SELECT_PAGE,
                            "Select all items on current page" },
    { "menu_invert_page",   MENU_INVERT_PAGE,
                            "Invert current page's selections" },
    { "menu_deselect_page", MENU_UNSELECT_PAGE,
                            "Unselect all items on current page" },
    { "menu_search",        MENU_SEARCH,
                            "Search and invert matching items" },
    { "menu_shift_right",   MENU_SHIFT_RIGHT,
                            "Pan current page to right (perm_invent only)" },
    { "menu_shift_left",    MENU_SHIFT_LEFT,
                            "Pan current page to left (perm_invent only)" },
    { (char *) 0, '\0', (char *) 0 }
};

static void nmcpy(char *, const char *, int);
static void escapes(const char *, char *);
static void rejectoption(const char *);
static char *string_for_opt(char *, boolean);
static char *string_for_env_opt(const char *, char *, boolean);
static void bad_negation(const char *, boolean);
static int change_inv_order(char *);
static boolean warning_opts(char *, const char *);
static int feature_alert_opts(char *, const char *);
static boolean duplicate_opt_detection(int);
static void complain_about_duplicate(int);
static int length_without_val(const char *, int len);
static void determine_ambiguities(void);
static int check_misc_menu_command(char *, char *);
static int shared_menu_optfn(int, int, boolean, char *, char *);
static int spcfn_misc_menu_cmd(int, int, boolean, char *, char *);

static const char *attr2attrname(int);
static void basic_menu_colors(boolean);
static const char * msgtype2name(int);
static int query_msgtype(void);
static boolean msgtype_add(int, char *);
static void free_one_msgtype(int);
static int msgtype_count(void);
static boolean test_regex_pattern(const char *, const char *);
static boolean add_menu_coloring_parsed(const char *, int, int);
static void free_one_menu_coloring(int);
static int count_menucolors(void);
static boolean parse_role_opts(int, boolean, const char *,
                               char *, char **);
static unsigned int longest_option_name(int, int);
static void doset_add_menu(winid, const char *, int, int);
static int handle_add_list_remove(const char *, int);
static void remove_autopickup_exception(struct autopickup_exception *);
static int count_apes(void);
static int count_cond(void);
static void enhance_menu_text(char *, size_t, int, boolean *, struct allopt_t *);

static int handler_align_misc(int);
static int handler_autounlock(int);
static int handler_disclose(void);
static int handler_menu_headings(void);
static int handler_menustyle(void);
static int handler_msg_window(void);
static int handler_number_pad(void);
static int handler_paranoid_confirmation(void);
static int handler_pickup_burden(void);
static int handler_pickup_types(void);
static int handler_runmode(void);
static int handler_sortloot(void);
static int handler_symset(int);
static int handler_whatis_coord(void);
static int handler_whatis_filter(void);
/* next few are not allopts[] entries, so will only be called
   directly from doset, not from individual optfn's */
static int handler_autopickup_exception(void);
static int handler_menu_colors(void);
static int handler_msgtype(void);
#ifndef NO_VERBOSE_GRANULARITY
static int handler_verbose(int optidx);
#endif

static boolean is_wc_option(const char *);
static boolean wc_supported(const char *);
static boolean is_wc2_option(const char *);
static boolean wc2_supported(const char *);
static void wc_set_font_name(int, char *);
static int wc_set_window_colors(char *);
static boolean illegal_menu_cmd_key(uchar);
#ifdef CURSES_GRAPHICS
extern int curses_read_attrs(const char *attrs);
extern char *curses_fmt_attrs(char *);
#endif

/*
 **********************************
 *
 *   parseoptions
 *
 **********************************
 */
boolean
parseoptions(register char *opts, boolean tinitial, boolean tfrom_file)
{
    char *op;
    boolean negated, got_match = FALSE, pfx_match = FALSE;
#if 0
    boolean has_val = FALSE;
#endif
    int i, matchidx = -1, optresult = optn_err, optlen, optlen_wo_val;
    boolean retval = TRUE;

    duplicate = FALSE;
    using_alias = FALSE;
    g.opt_initial = tinitial;
    g.opt_from_file = tfrom_file;
    if ((op = index(opts, ',')) != 0) {
        *op++ = 0;
        if (!parseoptions(op, g.opt_initial, g.opt_from_file))
            retval = FALSE;
    }
    if (strlen(opts) > BUFSZ / 2) {
        config_error_add("Option too long, max length is %i characters",
                         (BUFSZ / 2));
        return FALSE;
    }

    /* strip leading and trailing white space */
    while (isspace((uchar) *opts))
        opts++;
    op = eos(opts);
    while (--op >= opts && isspace((uchar) *op))
        *op = '\0';

    if (!*opts) {
        config_error_add("Empty statement");
        return FALSE;
    }
    negated = FALSE;
    while ((*opts == '!') || !strncmpi(opts, "no", 2)) {
        if (*opts == '!')
            opts++;
        else
            opts += 2;
        negated = !negated;
    }
    optlen = (int) strlen(opts);
    optlen_wo_val = length_without_val(opts, optlen);
    if (optlen_wo_val < optlen) {
#if 0
        has_val = TRUE;
#endif
        optlen = optlen_wo_val;
#if 0
    } else {
        has_val = FALSE;
#endif
    }

    for (i = 0; i < OPTCOUNT; ++i) {
        got_match = FALSE;

        if (allopt[i].pfx) {
            if (str_start_is(opts, allopt[i].name, TRUE)) {
                matchidx = i;
                got_match = pfx_match = TRUE;
            }
        }
#if 0   /* this prevents "boolopt:True" &c */
        if (!got_match) {
            if (has_val && !allopt[i].valok)
                continue;
        }
#endif
        /*
         * During option initialization, the function
         *     determine_ambiguities()
         * figured out exactly how many characters are required to
         * unambiguously differentiate one option from all others, and it
         * placed that number into each option's alloption[n].minmatch.
         *
         */
        if (!got_match)
            got_match = match_optname(opts, allopt[i].name,
                                      allopt[i].minmatch, TRUE);
        if (got_match) {
            if (!allopt[i].pfx && optlen < allopt[i].minmatch) {
                config_error_add(
             "Ambiguous option %s, %d characters are needed to differentiate",
                                 opts, allopt[i].minmatch);
                break;
            }
            matchidx = i;
            break;
        }
    }

    if (!got_match) {
        /* spin through the aliases to see if there's a match in those.
           Note that if multiple delimited aliases for the same option
           becomes desireable in the future, this is where you'll need
           to split a delimited allopt[i].alias field into each
           individual alias */

        for (i = 0; i < OPTCOUNT; ++i) {
            if (!allopt[i].alias)
                continue;
            got_match = match_optname(opts, allopt[i].alias,
                                      (int) strlen(allopt[i].alias),
                                      TRUE);
            if (got_match) {
                matchidx = i;
                using_alias = TRUE;
                break;
            }
        }
    }

    /* allow optfn's to test whether they were called from parseoptions() */
    g.program_state.in_parseoptions++;

    if (got_match && matchidx >= 0) {
        duplicate = duplicate_opt_detection(matchidx);
        if (duplicate && !allopt[matchidx].dupeok)
            complain_about_duplicate(matchidx);

        /* check for bad negation, so option functions don't have to */
        if (negated && !allopt[matchidx].negateok) {
            bad_negation(allopt[matchidx].name, TRUE);
            return optn_err;
        }

        /*
         * Now call the option's associated function via the function
         * pointer for it in the allopt[] array, specifying a 'do_set' req.
         */
        if (allopt[matchidx].optfn) {
            op = string_for_opt(opts, TRUE);
            optresult = (*allopt[matchidx].optfn)(allopt[matchidx].idx,
                                                  do_set, negated, opts, op);
        }
    }

    if (g.program_state.in_parseoptions > 0)
        g.program_state.in_parseoptions--;

#if 0
    /* This specialization shouldn't be needed any longer because each of
       the individual options is part of the allopts[] list, thus already
       taken care of in the for-loop above */
    if (!got_match) {
        int res = check_misc_menu_command(opts, op);

        if (res >= 0)
            optresult = spcfn_misc_menu_cmd(res, do_set, negated, opts, op);
        if (optresult == optn_ok)
            got_match = TRUE;
    }
#endif

    if (!got_match) {
        /* Is it a symbol? */
        if (strstr(opts, "S_") == opts && parsesymbols(opts, PRIMARYSET)) {
            switch_symbols(TRUE);
            check_gold_symbol();
            optresult = optn_ok;
        }
    }

    if (optresult == optn_silenterr)
        return FALSE;
    if (pfx_match && optresult == optn_err) {
        char pfxbuf[BUFSZ], *pfxp;

        if (opts) {
            Snprintf(pfxbuf, sizeof pfxbuf, "%s", opts);
            if ((pfxp = index(pfxbuf, ':')) != 0)
                *pfxp = '\0';
            config_error_add("bad option suffix variation '%s'", pfxbuf);
        }
        return FALSE;
    }
    if (got_match && optresult == optn_err)
        return FALSE;
    if (optresult == optn_ok)
        return retval;

    /* out of valid options */
    config_error_add("Unknown option '%s'", opts);
    return FALSE;
}

static int
check_misc_menu_command(char *opts, char *op UNUSED)
{
    int i;
    const char *name_to_check;

    /* check for menu command mapping */
    for (i = 0; default_menu_cmd_info[i].name; i++) {
        name_to_check = default_menu_cmd_info[i].name;
        if (match_optname(opts, name_to_check,
                          (int) strlen(name_to_check), TRUE))
            return i;
    }
    return -1;
}

/*
 **********************************
 *
 *   Per-option Functions
 *
 **********************************
 */

static int
optfn_align(int optidx, int req, boolean negated, char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        if (parse_role_opts(optidx, negated, allopt[optidx].name, opts, &op)) {
            if ((flags.initalign = str2align(op)) == ROLE_NONE) {
                config_error_add("Unknown %s '%s'", allopt[optidx].name, op);
                return optn_err;
            }
        } else
            return optn_silenterr;
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s", rolestring(flags.initalign, aligns, adj));
        return optn_ok;
    }
    return optn_ok;
}


static int
optfn_align_message(
    int optidx, int req, boolean negated,
    char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* WINCAP align_message:[left|top|right|bottom] */

        op = string_for_opt(opts, negated);
        if ((op != empty_optstr) && !negated) {
            if (!strncmpi(op, "left", sizeof "left" - 1))
                iflags.wc_align_message = ALIGN_LEFT;
            else if (!strncmpi(op, "top", sizeof "top" - 1))
                iflags.wc_align_message = ALIGN_TOP;
            else if (!strncmpi(op, "right", sizeof "right" - 1))
                iflags.wc_align_message = ALIGN_RIGHT;
            else if (!strncmpi(op, "bottom", sizeof "bottom" - 1))
                iflags.wc_align_message = ALIGN_BOTTOM;
            else {
                config_error_add("Unknown %s parameter '%s'",
                                 allopt[optidx].name, op);
                return optn_err;
            }
        } else if (negated) {
            bad_negation(allopt[optidx].name, TRUE);
            return optn_err;
        }
        return optn_ok;
    }
    if (req == get_val) {
        int which;

        if (!opts)
            return optn_err;
        which = iflags.wc_align_message;
        Sprintf(opts, "%s",
                (which == ALIGN_TOP) ? "top"
                : (which == ALIGN_LEFT) ? "left"
                  : (which == ALIGN_BOTTOM) ? "bottom"
                    : (which == ALIGN_RIGHT) ? "right"
                      : defopt);
        return optn_ok;
    }
    if (req == do_handler) {
        return handler_align_misc(optidx);
    }
    return optn_ok;
}

static int
optfn_align_status(int optidx, int req, boolean negated, char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* WINCAP align_status:[left|top|right|bottom] */
        op = string_for_opt(opts, negated);
        if ((op != empty_optstr) && !negated) {
            if (!strncmpi(op, "left", sizeof "left" - 1))
                iflags.wc_align_status = ALIGN_LEFT;
            else if (!strncmpi(op, "top", sizeof "top" - 1))
                iflags.wc_align_status = ALIGN_TOP;
            else if (!strncmpi(op, "right", sizeof "right" - 1))
                iflags.wc_align_status = ALIGN_RIGHT;
            else if (!strncmpi(op, "bottom", sizeof "bottom" - 1))
                iflags.wc_align_status = ALIGN_BOTTOM;
            else {
                config_error_add("Unknown %s parameter '%s'",
                                 allopt[optidx].name, op);
                return optn_err;
            }
        } else if (negated) {
            bad_negation(allopt[optidx].name, TRUE);
            return optn_err;
        }
        return optn_ok;
    }
    if (req == get_val) {
        int which;

        if (!opts)
            return optn_err;
        which = iflags.wc_align_status;
        Sprintf(opts, "%s",
                (which == ALIGN_TOP) ? "top"
                : (which == ALIGN_LEFT) ? "left"
                  : (which == ALIGN_BOTTOM) ? "bottom"
                    : (which == ALIGN_RIGHT) ? "right"
                      : defopt);
        return optn_ok;
    }
    if (req == do_handler) {
        return handler_align_misc(optidx);
    }
    return optn_ok;
}

static int
optfn_altkeyhandling(
    int optidx UNUSED,
    int req,
    boolean negated,
    char *opts,
    char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* altkeyhandling:string */

#if defined(WIN32) && defined(TTY_GRAPHICS)
        if (op == empty_optstr || negated)
            return optn_err;
        set_altkeyhandling(op);
#else
        nhUse(negated);
        nhUse(op);
#endif
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        opts[0] = '\0';
#ifdef WIN32
        Sprintf(opts, "%s",
                (iflags.key_handling == nh340_keyhandling)
                    ? "340"
                    : (iflags.key_handling == ray_keyhandling)
                        ? "ray"
                        : "default");
#endif
        return optn_ok;
    }
#ifdef WIN32
    if (req == do_handler) {
        return set_keyhandling_via_option();
    }
#endif
    return optn_ok;
}

static int
optfn_autounlock(
    int optidx,
    int req,
    boolean negated,
    char *opts,
    char *op)
{
    if (req == do_init) {
        flags.autounlock = AUTOUNLOCK_APPLY_KEY;
        return optn_ok;
    }
    if (req == do_set) {
        /* autounlock:none or autounlock:untrap+apply-key+kick+force;
           autounlock without a value is same as autounlock:apply-key and
           !autounlock is same as autounlock:none; multiple values can be
           space separated or plus-sign separated but the same separation
           must be used for each element, not mix&match */
        char sep, *nxt;
        unsigned newflags;
        int i;

        if ((op = string_for_opt(opts, TRUE)) == empty_optstr) {
            flags.autounlock = negated ? 0 : AUTOUNLOCK_APPLY_KEY;
            return optn_ok;
        }
        newflags = 0;
        sep = index(op, '+') ? '+' : ' ';
        while (op) {
            op = trimspaces(op); /* might have leading space */
            if ((nxt = index(op, sep)) != 0) {
                *nxt++ = '\0';
                op = trimspaces(op); /* might have trailing space after
                                      * plus sign removal */
            }
            for (i = 0; i < SIZE(unlocktypes); ++i)
                if (!strncmpi(op, unlocktypes[i][0], Strlen(op))
                    /* fuzzymatch() doesn't match leading substrings but
                       this allows "apply_key" and "applykey" to match
                       "apply-key"; "apply key" too if part of foo+bar */
                    || fuzzymatch(op, unlocktypes[i][0], " -_", TRUE)) {
                    switch (*op) {
                    case 'n':
                        negated = TRUE;
                        break;
                    case 'u':
                        newflags |= AUTOUNLOCK_UNTRAP;
                        break;
                    case 'a':
                        newflags |= AUTOUNLOCK_APPLY_KEY;
                        break;
                    case 'k':
                        newflags |= AUTOUNLOCK_KICK;
                        break;
                    case 'f':
                        newflags |= AUTOUNLOCK_FORCE;
                        break;
                    default:
                        config_error_add("Invalid value for \"%s\": \"%s\"",
                                         allopt[optidx].name, op);
                        return optn_silenterr;
                    }
                }
            op = nxt;
        }
        if (negated && newflags != 0) {
            config_error_add(
                     "Invalid value combination for \"%s\": 'none' with some",
                             allopt[optidx].name);
            return optn_silenterr;
        }
        flags.autounlock = newflags;
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        if (!flags.autounlock) {
            Strcpy(opts, "none");
        } else {
            static const char plus[] = " + ";
            const char *p = "";

            *opts = '\0';
            if (flags.autounlock & AUTOUNLOCK_UNTRAP)
                Sprintf(eos(opts), "%s%s", p, unlocktypes[1][0]), p = plus;
            if (flags.autounlock & AUTOUNLOCK_APPLY_KEY)
                Sprintf(eos(opts), "%s%s", p, unlocktypes[2][0]), p = plus;
            if (flags.autounlock & AUTOUNLOCK_KICK)
                Sprintf(eos(opts), "%s%s", p, unlocktypes[3][0]), p = plus;
            if (flags.autounlock & AUTOUNLOCK_FORCE)
                Sprintf(eos(opts), "%s%s", p, unlocktypes[4][0]); /*no more p*/
        }
        return optn_ok;
    }
    if (req == do_handler) {
        return handler_autounlock(optidx);
    }
    return optn_ok;
}

static int
optfn_boulder(int optidx UNUSED, int req, boolean negated UNUSED,
              char *opts, char *op UNUSED)
{
#ifdef BACKWARD_COMPAT
    int clash = 0;
#endif

    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* boulder:symbol */

#ifdef BACKWARD_COMPAT

        /* if ((opts = string_for_env_opt(allopt[optidx].name, opts, FALSE))
               == empty_optstr)
         */
        if ((opts = string_for_opt(opts, FALSE)) == empty_optstr)
            return FALSE;
        escapes(opts, opts);
        /* note: dummy monclass #0 has symbol value '\0'; we allow that--
           attempting to set bouldersym to '^@'/'\0' will reset to default */
        if (def_char_to_monclass(opts[0]) != MAXMCLASSES)
            clash = opts[0] ? 1 : 0;
        else if (opts[0] >= '1' && opts[0] < WARNCOUNT + '0')
            clash = 2;
        if (opts[0] < ' ') {
            config_error_add("boulder symbol cannot be a control character");
            return optn_ok;
        } else if (clash) {
            /* symbol chosen matches a used monster or warning
               symbol which is not good - reject it */
            config_error_add("Badoption - boulder symbol '%s' would conflict "
                             "with a %s symbol",
                             visctrl(opts[0]),
                             (clash == 1) ? "monster" : "warning");
        } else {
            /*
             * Override the default boulder symbol.
             */
            g.ov_primary_syms[SYM_BOULDER + SYM_OFF_X] = (nhsym) opts[0];
            g.ov_rogue_syms[SYM_BOULDER + SYM_OFF_X] = (nhsym) opts[0];
            /* for 'initial', update of BOULDER symbol is done in
               initoptions_finish(), after all symset options
               have been processed */
            if (!g.opt_initial) {
                nhsym sym = get_othersym(SYM_BOULDER,
                                         Is_rogue_level(&u.uz) ? ROGUESET
                                                               : PRIMARYSET);

                if (sym)
                    g.showsyms[SYM_BOULDER + SYM_OFF_X] = sym;
                g.opt_need_redraw = TRUE;
            }
        }
        return optn_ok;
#else
        config_error_add("'%s' no longer supported; use S_boulder:c instead",
                         allopt[optidx].name);
        return optn_err;
#endif
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        opts[0] = '\0';
#ifdef BACKWARD_COMPAT
        Sprintf(opts, "%c",
                g.ov_primary_syms[SYM_BOULDER + SYM_OFF_X]
                  ? g.ov_primary_syms[SYM_BOULDER + SYM_OFF_X]
                  : g.showsyms[(int) objects[BOULDER].oc_class + SYM_OFF_O]);
#endif
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_catname(
    int optidx, int req, boolean negated UNUSED,
    char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        if ((op = string_for_env_opt(allopt[optidx].name, opts, FALSE))
            != empty_optstr) {
            nmcpy(g.catname, op, PL_PSIZ);
        } else {
            return optn_err;
        }
        sanitize_name(g.catname);
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s", g.catname[0] ? g.catname : none);
        return optn_ok;
    }
    return optn_ok;
}

#ifdef CURSES_GRAPHICS
static int
optfn_cursesgraphics(int optidx, int req, boolean negated,
                     char *opts, char *op UNUSED)
{
#ifdef BACKWARD_COMPAT
    boolean badflag = FALSE;
#endif

    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* "cursesgraphics" */

#ifdef BACKWARD_COMPAT
        if (!negated) {
            /* There is no rogue level cursesgraphics-specific set */
            if (g.symset[PRIMARYSET].name) {
                badflag = TRUE;
            } else {
                g.symset[PRIMARYSET].name = dupstr(allopt[optidx].name);
                if (!read_sym_file(PRIMARYSET)) {
                    badflag = TRUE;
                    clear_symsetentry(PRIMARYSET, TRUE);
                } else
                    switch_symbols(TRUE);
            }
            if (badflag) {
                config_error_add("Failure to load symbol set %s.",
                                 allopt[optidx].name);
                return optn_err;
            }
        }
        return optn_ok;
#else
        config_error_add("'%s' no longer supported; use 'symset:%s' instead",
                         allopt[optidx].name, allopt[optidx].name);
        return optn_err;
#endif
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        opts[0] = '\0';
        return optn_ok;
    }
    return optn_ok;
}
#endif

static int
optfn_DECgraphics(int optidx, int req, boolean negated,
                  char *opts, char *op UNUSED)
{
#ifdef BACKWARD_COMPAT
    boolean badflag = FALSE;
#endif

    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* "DECgraphics" */

#ifdef BACKWARD_COMPAT
        if (!negated) {
            /* There is no rogue level DECgraphics-specific set */
            if (g.symset[PRIMARYSET].name) {
                badflag = TRUE;
            } else {
                g.symset[PRIMARYSET].name = dupstr(allopt[optidx].name);
                if (!read_sym_file(PRIMARYSET)) {
                    badflag = TRUE;
                    clear_symsetentry(PRIMARYSET, TRUE);
                } else
                    switch_symbols(TRUE);
            }
            if (badflag) {
                config_error_add("Failure to load symbol set %s.",
                                 allopt[optidx].name);
                return optn_err;
            }
        }
        return optn_ok;
#else
        config_error_add("'%s' no longer supported; use 'symset:%s' instead",
                         allopt[optidx].name, allopt[optidx].name);
        return optn_err;
#endif
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        opts[0] = '\0';
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_disclose(int optidx, int req, boolean negated, char *opts, char *op)
{
    int i, idx, prefix_val;
    unsigned num;

    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* things to disclose at end of game */

        /*
         * The order that the end_disclose options are stored:
         *      inventory, attribs, vanquished, genocided,
         *      conduct, overview.
         * There is an array in flags:
         *      end_disclose[NUM_DISCLOSURE_OPT];
         * with option settings for the each of the following:
         * iagvc [see disclosure_options in decl.c]:
         * Allowed setting values in that array are:
         *      DISCLOSE_PROMPT_DEFAULT_YES  ask with default answer yes
         *      DISCLOSE_PROMPT_DEFAULT_NO   ask with default answer no
         *      DISCLOSE_YES_WITHOUT_PROMPT  always disclose and don't ask
         *      DISCLOSE_NO_WITHOUT_PROMPT   never disclose and don't ask
         *      DISCLOSE_PROMPT_DEFAULT_SPECIAL  for 'vanquished' only...
         *      DISCLOSE_SPECIAL_WITHOUT_PROMPT  ...to set up sort order.
         *
         * Those setting values can be used in the option
         * string as a prefix to get the desired behaviour.
         *
         * For backward compatibility, no prefix is required,
         * and the presence of a i,a,g,v, or c without a prefix
         * sets the corresponding value to DISCLOSE_YES_WITHOUT_PROMPT.
         */

        op = string_for_opt(opts, TRUE);
        if (op != empty_optstr && negated) {
            bad_negation(allopt[optidx].name, TRUE);
            return optn_err;
        }
        /* "disclose" without a value means "all with prompting"
           and negated means "none without prompting" */
        if (op == empty_optstr || !strcmpi(op, "all")
            || !strcmpi(op, "none")) {
            if (op != empty_optstr && !strcmpi(op, "none"))
                negated = TRUE;
            for (num = 0; num < NUM_DISCLOSURE_OPTIONS; num++)
                flags.end_disclose[num] = negated
                                              ? DISCLOSE_NO_WITHOUT_PROMPT
                                              : DISCLOSE_PROMPT_DEFAULT_YES;
            return optn_ok;
        }

        num = 0;
        prefix_val = -1;
        while (*op && num < sizeof flags.end_disclose - 1) {
            static char valid_settings[] = { DISCLOSE_PROMPT_DEFAULT_YES,
                                             DISCLOSE_PROMPT_DEFAULT_NO,
                                             DISCLOSE_PROMPT_DEFAULT_SPECIAL,
                                             DISCLOSE_YES_WITHOUT_PROMPT,
                                             DISCLOSE_NO_WITHOUT_PROMPT,
                                             DISCLOSE_SPECIAL_WITHOUT_PROMPT,
                                             '\0' };
            register char c, *dop;

            c = lowc(*op);
            if (c == 'k')
                c = 'v'; /* killed -> vanquished */
            if (c == 'd')
                c = 'o'; /* dungeon -> overview */
            dop = index(disclosure_options, c);
            if (dop) {
                idx = (int) (dop - disclosure_options);
                if (idx < 0 || idx > NUM_DISCLOSURE_OPTIONS - 1) {
                    impossible("bad disclosure index %d %c", idx, c);
                    continue;
                }
                if (prefix_val != -1) {
                    if (*dop != 'v') {
                        if (prefix_val == DISCLOSE_PROMPT_DEFAULT_SPECIAL)
                            prefix_val = DISCLOSE_PROMPT_DEFAULT_YES;
                        if (prefix_val == DISCLOSE_SPECIAL_WITHOUT_PROMPT)
                            prefix_val = DISCLOSE_YES_WITHOUT_PROMPT;
                    }
                    flags.end_disclose[idx] = prefix_val;
                    prefix_val = -1;
                } else
                    flags.end_disclose[idx] = DISCLOSE_YES_WITHOUT_PROMPT;
            } else if (index(valid_settings, c)) {
                prefix_val = c;
            } else if (c == ' ') {
                ; /* do nothing */
            } else {
                config_error_add("Unknown %s parameter '%c'",
                                 allopt[optidx].name, *op);
                return optn_err;
            }
            op++;
        }
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;

        opts[0] = '\0';
        for (i = 0; i < NUM_DISCLOSURE_OPTIONS; i++) {
            if (i)
                (void) strkitten(opts, ' ');
            (void) strkitten(opts, flags.end_disclose[i]);
            (void) strkitten(opts, disclosure_options[i]);
        }
        return optn_ok;
    }
    if (req == do_handler) {
        return handler_disclose();
    }
    return optn_ok;
}

static int
optfn_dogname(int optidx UNUSED, int req, boolean negated UNUSED,
              char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        if (op != empty_optstr) {
            nmcpy(g.dogname, op, PL_PSIZ);
        } else {
            return optn_err;
        }
        sanitize_name(g.dogname);
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s", g.dogname[0] ? g.dogname : none);
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_dungeon(int optidx UNUSED, int req, boolean negated UNUSED,
              char *opts, char *op UNUSED)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s", to_be_done);
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_effects(int optidx UNUSED, int req, boolean negated UNUSED,
              char *opts, char *op UNUSED)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s", to_be_done);
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_font_map(int optidx, int req, boolean negated, char *opts, char *op)
{
    /* send them over to the prefix handling for font_ */
    return pfxfn_font(optidx, req, negated, opts, op);
}

static int
optfn_font_menu(int optidx, int req, boolean negated, char *opts, char *op)
{
    /* send them over to the prefix handling for font_ */
    return pfxfn_font(optidx, req, negated, opts, op);
}

static int
optfn_font_message(int optidx, int req, boolean negated, char *opts, char *op)
{
    /* send them over to the prefix handling for font_ */
    return pfxfn_font(optidx, req, negated, opts, op);
}

static int
optfn_font_size_map(
    int optidx, int req, boolean negated,
    char *opts, char *op)
{
    /* send them over to the prefix handling for font_ */
    return pfxfn_font(optidx, req, negated, opts, op);
}

static int
optfn_font_size_menu(
    int optidx, int req, boolean negated,
    char *opts, char *op)
{
    /* send them over to the prefix handling for font_ */
    return pfxfn_font(optidx, req, negated, opts, op);
}

static int
optfn_font_size_message(
    int optidx, int req, boolean negated,
    char *opts, char *op)
{
    /* send them over to the prefix handling for font_ */
    return pfxfn_font(optidx, req, negated, opts, op);
}

static int
optfn_font_size_status(
    int optidx, int req, boolean negated,
    char *opts, char *op)
{
    /* send them over to the prefix handling for font_ */
    return pfxfn_font(optidx, req, negated, opts, op);
}

static int
optfn_font_size_text(
    int optidx, int req, boolean negated,
    char *opts, char *op)
{
    /* send them over to the prefix handling for font_ */
    return pfxfn_font(optidx, req, negated, opts, op);
}

static int
optfn_font_status(int optidx, int req, boolean negated, char *opts, char *op)
{
    /* send them over to the prefix handling for font_ */
    return pfxfn_font(optidx, req, negated, opts, op);
}

static int
optfn_font_text(int optidx, int req, boolean negated, char *opts, char *op)
{
    /* send them over to the prefix handling for font_ */
    return pfxfn_font(optidx, req, negated, opts, op);
}

static int
optfn_fruit(int optidx UNUSED, int req, boolean negated,
            char *opts, char *op)
{
    struct fruit *forig = 0;

    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        op = string_for_opt(opts, negated || !g.opt_initial);
        if (negated) {
            if (op != empty_optstr) {
                bad_negation("fruit", TRUE);
                return optn_err;
            }
            op = empty_optstr;
            goto goodfruit;
        }
        if (op == empty_optstr)
            return optn_err;
        /* strip leading/trailing spaces, condense internal ones (3.6.2) */
        mungspaces(op);
        if (!g.opt_initial) {
            struct fruit *f;
            int fnum = 0;

            /* count number of named fruits; if 'op' is found among them,
               then the count doesn't matter because we won't be adding it */
            f = fruit_from_name(op, FALSE, &fnum);
            if (!f) {
                if (!flags.made_fruit)
                    forig = fruit_from_name(g.pl_fruit, FALSE, (int *) 0);

                if (!forig && fnum >= 100) {
                    config_error_add(
                        "Doing that so many times isn't very fruitful.");
                    return optn_ok;
                }
            }
        }
 goodfruit:
        nmcpy(g.pl_fruit, op, PL_FSIZ);
        sanitize_name(g.pl_fruit);
        /* OBJ_NAME(objects[SLIME_MOLD]) won't work for this after
           initialization; it gets changed to generic "fruit" */
        if (!*g.pl_fruit)
            nmcpy(g.pl_fruit, "slime mold", PL_FSIZ);
        if (!g.opt_initial) {
            /* if 'forig' is nonNull, we replace it rather than add
               a new fruit; it can only be nonNull if no fruits have
               been created since the previous name was put in place */
            (void) fruitadd(g.pl_fruit, forig);
            pline("Fruit is now \"%s\".", g.pl_fruit);
        }
        /* If initial, then initoptions is allowed to do it instead
         * of here (initoptions always has to do it even if there's
         * no fruit option at all.  Also, we don't want people
         * setting multiple fruits in their options.)
         */
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s", g.pl_fruit);
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_gender(int optidx, int req, boolean negated, char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* gender:string */
        if (parse_role_opts(optidx, negated, allopt[optidx].name, opts, &op)) {
            if ((flags.initgend = str2gend(op)) == ROLE_NONE) {
                config_error_add("Unknown %s '%s'", allopt[optidx].name, op);
                return optn_err;
            } else
                flags.female = flags.initgend;
        } else
            return optn_silenterr;
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s", rolestring(flags.initgend, genders, adj));
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_glyph(int optidx UNUSED, int req, boolean negated, char *opts, char *op)
{
#ifdef ENHANCED_SYMBOLS
    int glyph;
#endif

    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* OPTION=glyph:G_glyph/U+NNNN/r-g-b */
        if (negated) {
            if (op != empty_optstr) {
                bad_negation("glyph", TRUE);
                return optn_err;
            }
        }
        if (op == empty_optstr)
            return optn_err;
        /* strip leading/trailing spaces, condense internal ones (3.6.2) */
        mungspaces(op);
#ifdef ENHANCED_SYMBOLS
        if (!glyphrep_to_custom_map_entries(op, &glyph))
            return optn_err;
#endif
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s", to_be_done);
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_hilite_status(
    int optidx UNUSED,
    int req,
    boolean negated,
    char *opts,
    char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* hilite fields in status prompt */
#ifdef STATUS_HILITES
        op = string_for_opt(opts, TRUE);
        if (op != empty_optstr && negated) {
            clear_status_hilites();
            return optn_ok;
        } else if (op == empty_optstr) {
            config_error_add("Value is mandatory for hilite_status");
            return optn_err;
        }
        if (!parse_status_hl1(op, g.opt_from_file))
            return optn_err;
        return optn_ok;
#else
        nhUse(negated);
        nhUse(op);
        config_error_add("'%s' is not supported", allopt[optidx].name);
        return optn_err;
#endif
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
#ifdef STATUS_HILITES
        Strcpy(opts, count_status_hilites()
                     ? "(see \"status highlight rules\" below)"
                     : "(none)");
#else
        opts[0] = '\0';
#endif
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_horsename(
    int optidx, int req, boolean negated UNUSED,
    char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        if ((op = string_for_env_opt(allopt[optidx].name, opts, FALSE))
            != empty_optstr) {
            nmcpy(g.horsename, op, PL_PSIZ);
        } else {
            return optn_err;
        }
        sanitize_name(g.horsename);
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s", g.horsename[0] ? g.horsename : none);
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_IBMgraphics(int optidx, int req, boolean negated,
                  char *opts, char *op UNUSED)
{
#ifdef BACKWARD_COMPAT
    const char *sym_name = allopt[optidx].name;
    boolean badflag = FALSE;
    int i;
#endif

    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* "IBMgraphics" */

#ifdef BACKWARD_COMPAT

        if (!negated) {
            for (i = 0; i < NUM_GRAPHICS; ++i) {
                if (g.symset[i].name) {
                    badflag = TRUE;
                } else {
                    if (i == ROGUESET)
                        sym_name = "RogueIBM";
                    g.symset[i].name = dupstr(sym_name);
                    if (!read_sym_file(i)) {
                        badflag = TRUE;
                        clear_symsetentry(i, TRUE);
                        break;
                    }
                }
            }
            if (badflag) {
                config_error_add("Failure to load symbol set %s.", sym_name);
                return optn_err;
            } else {
                switch_symbols(TRUE);
                if (!g.opt_initial && Is_rogue_level(&u.uz))
                    assign_graphics(ROGUESET);
            }
        }
        return optn_ok;
#else
        config_error_add("'%s' no longer supported; use 'symset:%s' instead",
                         allopt[optidx].name, allopt[optidx].name);
        return optn_err;
#endif
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        opts[0] = '\0';
        return optn_ok;
    }
    return optn_ok;
}

#if defined(BACKWARD_COMPAT) && defined(MAC_GRAPHICS_ENV)
static int
optfn_MACgraphics(int optidx, int req, boolean negated, char *opts, char *op)
{
    boolean badflag = FALSE;

    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* "MACgraphics" */
        if (!negated) {
            if (g.symset[PRIMARYSET].name) {
                badflag = TRUE;
            } else {
                g.symset[PRIMARYSET].name = dupstr(allopt[optidx].name);
                if (!read_sym_file(PRIMARYSET)) {
                    badflag = TRUE;
                    clear_symsetentry(PRIMARYSET, TRUE);
                }
            }
            if (badflag) {
                config_error_add("Failure to load symbol set %s.",
                                 allopt[optidx].name);
                return FALSE;
            } else {
                switch_symbols(TRUE);
                if (!g.opt_initial && Is_rogue_level(&u.uz))
                    assign_graphics(ROGUESET);
            }
        }
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        opts[0] = '\0';
        return optn_ok;
    }
    return optn_ok;
}
#endif /* BACKWARD_COMPAT && MAC_GRAPHICS_ENV */

static int
optfn_map_mode(int optidx, int req, boolean negated, char *opts, char *op)
{
    int i;

    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* WINCAP
         *
         *  map_mode:[tiles|ascii4x6|ascii6x8|ascii8x8|ascii16x8|ascii7x12
         *            |ascii8x12|ascii16x12|ascii12x16|ascii10x18|fit_to_screen
         *            |ascii_fit_to_screen|tiles_fit_to_screen]
         */
        op = string_for_opt(opts, negated);
        if (op != empty_optstr && !negated) {
            if (!strcmpi(op, "tiles"))
                iflags.wc_map_mode = MAP_MODE_TILES;
            else if (!strncmpi(op, "ascii4x6", sizeof "ascii4x6" - 1))
                iflags.wc_map_mode = MAP_MODE_ASCII4x6;
            else if (!strncmpi(op, "ascii6x8", sizeof "ascii6x8" - 1))
                iflags.wc_map_mode = MAP_MODE_ASCII6x8;
            else if (!strncmpi(op, "ascii8x8", sizeof "ascii8x8" - 1))
                iflags.wc_map_mode = MAP_MODE_ASCII8x8;
            else if (!strncmpi(op, "ascii16x8", sizeof "ascii16x8" - 1))
                iflags.wc_map_mode = MAP_MODE_ASCII16x8;
            else if (!strncmpi(op, "ascii7x12", sizeof "ascii7x12" - 1))
                iflags.wc_map_mode = MAP_MODE_ASCII7x12;
            else if (!strncmpi(op, "ascii8x12", sizeof "ascii8x12" - 1))
                iflags.wc_map_mode = MAP_MODE_ASCII8x12;
            else if (!strncmpi(op, "ascii16x12", sizeof "ascii16x12" - 1))
                iflags.wc_map_mode = MAP_MODE_ASCII16x12;
            else if (!strncmpi(op, "ascii12x16", sizeof "ascii12x16" - 1))
                iflags.wc_map_mode = MAP_MODE_ASCII12x16;
            else if (!strncmpi(op, "ascii10x18", sizeof "ascii10x18" - 1))
                iflags.wc_map_mode = MAP_MODE_ASCII10x18;
            else if (!strncmpi(op, "fit_to_screen",
                               sizeof "fit_to_screen" - 1))
                iflags.wc_map_mode = MAP_MODE_ASCII_FIT_TO_SCREEN;
            else if (!strncmpi(op, "ascii_fit_to_screen",
                               sizeof "ascii_fit_to_screen" - 1))
                iflags.wc_map_mode = MAP_MODE_ASCII_FIT_TO_SCREEN;
            else if (!strncmpi(op, "tiles_fit_to_screen",
                               sizeof "tiles_fit_to_screen" - 1))
                iflags.wc_map_mode = MAP_MODE_TILES_FIT_TO_SCREEN;
            else {
                config_error_add("Unknown %s parameter '%s'",
                                 allopt[optidx].name, op);
                return optn_err;
            }
        } else if (negated) {
            bad_negation(allopt[optidx].name, TRUE);
            return optn_err;
        }
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        i = iflags.wc_map_mode;
        Sprintf(opts, "%s",
                (i == MAP_MODE_TILES) ? "tiles"
                : (i == MAP_MODE_ASCII4x6) ? "ascii4x6"
                  : (i == MAP_MODE_ASCII6x8) ? "ascii6x8"
                    : (i == MAP_MODE_ASCII8x8) ? "ascii8x8"
                      : (i == MAP_MODE_ASCII16x8) ? "ascii16x8"
                        : (i == MAP_MODE_ASCII7x12) ? "ascii7x12"
                          : (i == MAP_MODE_ASCII8x12) ? "ascii8x12"
                            : (i == MAP_MODE_ASCII16x12) ? "ascii16x12"
                              : (i == MAP_MODE_ASCII12x16) ? "ascii12x16"
                                : (i == MAP_MODE_ASCII10x18) ? "ascii10x18"
                                  : (i == MAP_MODE_ASCII_FIT_TO_SCREEN)
                                    ? "fit_to_screen"
                                    : defopt);
        return optn_ok;
    }
    return optn_ok;
}

/* all the key assignment options for menu_* commands are identical
   but optlist.h treats them as distinct rather than sharing one */
static int
shared_menu_optfn(int optidx UNUSED, int req, boolean negated UNUSED,
                   char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        int res = check_misc_menu_command(opts, op);

        if (res < 0)
            return optn_err;
        return spcfn_misc_menu_cmd(res, req, negated, opts, op);
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s", to_be_done);
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_menu_deselect_all(int optidx, int req, boolean negated,
                        char *opts, char *op)
{
    return shared_menu_optfn(optidx, req, negated, opts, op);
}

static int
optfn_menu_deselect_page(int optidx, int req, boolean negated,
                         char *opts, char *op)
{
    return shared_menu_optfn(optidx, req, negated, opts, op);
}

static int
optfn_menu_first_page(int optidx, int req, boolean negated,
                      char *opts, char *op)
{
    return shared_menu_optfn(optidx, req, negated, opts, op);
}

static int
optfn_menu_invert_all(int optidx, int req, boolean negated,
                      char *opts, char *op)
{
    return shared_menu_optfn(optidx, req, negated, opts, op);
}

static int
optfn_menu_invert_page(int optidx, int req, boolean negated,
                       char *opts, char *op)
{
    return shared_menu_optfn(optidx, req, negated, opts, op);
}

static int
optfn_menu_last_page(int optidx, int req, boolean negated,
                     char *opts, char *op)
{
    return shared_menu_optfn(optidx, req, negated, opts, op);
}

static int
optfn_menu_next_page(int optidx , int req, boolean negated,
                     char *opts, char *op)
{
    return shared_menu_optfn(optidx, req, negated, opts, op);
}

static int
optfn_menu_previous_page(int optidx, int req, boolean negated,
                         char *opts, char *op)
{
    return shared_menu_optfn(optidx, req, negated, opts, op);
}

static int
optfn_menu_search(int optidx, int req, boolean negated,
                  char *opts, char *op)
{
    return shared_menu_optfn(optidx, req, negated, opts, op);
}

static int
optfn_menu_select_all(int optidx, int req, boolean negated,
                      char *opts, char *op)
{
    return shared_menu_optfn(optidx, req, negated, opts, op);
}

static int
optfn_menu_select_page(int optidx, int req, boolean negated,
                       char *opts, char *op)
{
    return shared_menu_optfn(optidx, req, negated, opts, op);
}

static int
optfn_menu_shift_left(int optidx, int req, boolean negated,
                      char *opts, char *op)
{
    return shared_menu_optfn(optidx, req, negated, opts, op);
}

static int
optfn_menu_shift_right(int optidx, int req, boolean negated,
                       char *opts, char *op)
{
    return shared_menu_optfn(optidx, req, negated, opts, op);
}

/* end of shared key assignments for menu commands */

static int
optfn_menu_headings(int optidx, int req, boolean negated UNUSED,
                    char *opts, char *op UNUSED)
{
    int tmpattr;

    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        if ((opts = string_for_env_opt(allopt[optidx].name, opts, FALSE))
            == empty_optstr) {
            return optn_err;
        }
        tmpattr = match_str2attr(opts, TRUE);
        if (tmpattr == -1)
            return optn_err;
        iflags.menu_headings = tmpattr;
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s", attr2attrname(iflags.menu_headings));
        return optn_ok;
    }
    if (req == do_handler) {
        return handler_menu_headings();
    }
    return optn_ok;
}

static int
optfn_menuinvertmode(int optidx, int req, boolean negated UNUSED,
                     char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* menuinvertmode=0 or 1 or 2 (2 is experimental) */
        if (op != empty_optstr) {
            int mode = atoi(op);

            if (mode < 0 || mode > 2) {
                config_error_add("Illegal %s parameter '%s'",
                                 allopt[optidx].name, op);
                return optn_err;
            }
            iflags.menuinvertmode = mode;
        }
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%d", iflags.menuinvertmode);
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_menustyle(int optidx, int req, boolean negated, char *opts, char *op)
{
    int tmp;
    boolean val_required; /* no initializer based on opts because this can be
                             called with init and invalid opts and op */

    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* menustyle:traditional or combination or full or partial */

        val_required = (strlen(opts) > 5 && !negated);
        if ((op = string_for_opt(opts, !val_required)) == empty_optstr) {
            if (val_required)
                return optn_err; /* string_for_opt gave feedback */
            tmp = negated ? 'n' : 'f';
        } else {
            tmp = lowc(*op);
        }
        switch (tmp) {
        case 'n': /* none */
        case 't': /* traditional: prompt for class(es) by symbol,
                     prompt for each item within class(es) one at a time */
            flags.menu_style = MENU_TRADITIONAL;
            break;
        case 'c': /* combination: prompt for class(es) by symbol,
                     choose items within selected class(es) by menu */
            flags.menu_style = MENU_COMBINATION;
            break;
        case 'f': /* full: choose class(es) by first menu,
                     choose items within selected class(es) by second menu */
            flags.menu_style = MENU_FULL;
            break;
        case 'p': /* partial: skip class filtering, choose items among all
                     classes by menu */
            flags.menu_style = MENU_PARTIAL;
            break;
        default:
            config_error_add("Unknown %s parameter '%s'", allopt[optidx].name,
                             op);
            return optn_err;
        }
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s", menutype[(int) flags.menu_style][0]);
        return optn_ok;
    }
    if (req == do_handler) {
        return handler_menustyle();
    }
    return optn_ok;
}

static int
optfn_monsters(int optidx UNUSED, int req, boolean negated UNUSED,
               char *opts, char *op UNUSED)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        opts[0] = '\0';
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_mouse_support(
    int optidx, int req, boolean negated,
    char *opts, char *op)
{
    boolean compat;

    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        compat = (strlen(opts) <= 13);
        op = string_for_opt(opts, (compat || !g.opt_initial));
        if (op == empty_optstr) {
            if (compat || negated || g.opt_initial) {
                /* for backwards compatibility, "mouse_support" without a
                   value is a synonym for mouse_support:1 */
                iflags.wc_mouse_support = !negated;
            }
        } else {
            int mode = atoi(op);

            if (mode < 0 || mode > 2 || (mode == 0 && *op != '0')) {
                config_error_add("Illegal %s parameter '%s'",
                                 allopt[optidx].name, op);
                return optn_err;
            } else { /* mode >= 0 */
                iflags.wc_mouse_support = mode;
            }
        }
        return optn_ok;
    }
    if (req == get_val) {
#ifdef WIN32
#define MOUSEFIX1 ", QuickEdit off"
#define MOUSEFIX2 ", QuickEdit unchanged"
#else
#define MOUSEFIX1 ", O/S adjusted"
#define MOUSEFIX2 ", O/S unchanged"
#endif
        static const char *const mousemodes[][2] = {
            { "0=off", "" },
            { "1=on",  MOUSEFIX1 },
            { "2=on",  MOUSEFIX2 },
        };
#undef MOUSEFIX1
#undef MOUSEFIX2
        int ms = iflags.wc_mouse_support;

        if (!opts)
            return optn_err;

        if (ms >= 0 && ms <= 2)
            Sprintf(opts, "%s%s", mousemodes[ms][0], mousemodes[ms][1]);
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_msg_window(int optidx, int req, boolean negated, char *opts, char *op)
{
    int retval = optn_ok;
#if PREV_MSGS
    int tmp;
#else
    nhUse(optidx);
    nhUse(negated);
    nhUse(op);
#endif

    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* msg_window:single, combo, full or reversed */

        /* allow option to be silently ignored by non-tty ports */
#if PREV_MSGS
        if (op == empty_optstr) {
            tmp = negated ? 's' : 'f';
        } else {
            if (negated) {
                bad_negation(allopt[optidx].name, TRUE);
                return optn_err;
            }
            tmp = lowc(*op);
        }
        switch (tmp) {
        case 's': /* single message history cycle (default if negated) */
        case 'c': /* combination: first two as singles, then full page */
        case 'f': /* full page (default if specified without argument) */
        case 'r': /* full page in reverse order (LIFO; default for curses) */
            iflags.prevmsg_window = (char) tmp;
            break;
        default:
            config_error_add("Unknown %s parameter '%s'", allopt[optidx].name,
                             op);
            retval = optn_err;
        }
#endif
        return retval;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        opts[0] = '\0';
#if PREV_MSGS
        tmp = iflags.prevmsg_window;
        if (WINDOWPORT(curses)) {
            if (tmp == 's' || tmp == 'c')
                tmp = iflags.prevmsg_window = 'r';
        }
        Sprintf(opts, "%s", (tmp == 's') ? "single"
                            : (tmp == 'c') ? "combination"
                              : (tmp == 'f') ? "full"
                                : "reversed");
#endif
        return optn_ok;
    }
    if (req == do_handler) {
        return handler_msg_window();
    }
    return optn_ok;
}

static int
optfn_msghistory(int optidx, int req, boolean negated, char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        op = string_for_env_opt(allopt[optidx].name, opts, negated);
        if ((negated && op == empty_optstr)
            || (!negated && op != empty_optstr)) {
            iflags.msg_history = negated ? 0 : atoi(op);
        } else if (negated) {
            bad_negation(allopt[optidx].name, TRUE);
            return optn_err;
        }
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%u", iflags.msg_history);
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_name(int optidx, int req, boolean negated UNUSED, char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* name:string */

        if ((op = string_for_env_opt(allopt[optidx].name, opts, FALSE))
            != empty_optstr) {
            nmcpy(g.plname, op, PL_NSIZ);
        } else
            return optn_err;
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s", g.plname);
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_number_pad(int optidx, int req, boolean negated, char *opts, char *op)
{
    boolean compat;

    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        compat = (strlen(opts) <= 10);
        op = string_for_opt(opts, (compat || !g.opt_initial));
        if (op == empty_optstr) {
            if (compat || negated || g.opt_initial) {
                /* for backwards compatibility, "number_pad" without a
                   value is a synonym for number_pad:1 */
                iflags.num_pad = !negated;
                iflags.num_pad_mode = 0;
            }
        } else if (negated) {
            bad_negation(allopt[optidx].name, TRUE);
            return optn_err;
        } else {
            int mode = atoi(op);

            if (mode < -1 || mode > 4 || (mode == 0 && *op != '0')) {
                config_error_add("Illegal %s parameter '%s'",
                                 allopt[optidx].name, op);
                return optn_err;
            } else if (mode <= 0) {
                iflags.num_pad = FALSE;
                /* German keyboard; y and z keys swapped */
                iflags.num_pad_mode = (mode < 0); /* 0 or 1 */
            } else {                              /* mode > 0 */
                iflags.num_pad = TRUE;
                iflags.num_pad_mode = 0;
                /* PC Hack / MSDOS compatibility */
                if (mode == 2 || mode == 4)
                    iflags.num_pad_mode |= 1;
                /* phone keypad layout */
                if (mode == 3 || mode == 4)
                    iflags.num_pad_mode |= 2;
            }
        }
        reset_commands(FALSE);
        number_pad(iflags.num_pad ? 1 : 0);
        return optn_ok;
    }
    if (req == get_val) {
        static const char *const numpadmodes[] = {
            "0=off", "1=on", "2=on, MSDOS compatible",
            "3=on, phone-style layout",
            "4=on, phone layout, MSDOS compatible",
            "-1=off, y & z swapped", /*[5]*/
        };
        int indx = g.Cmd.num_pad
                       ? (g.Cmd.phone_layout ? (g.Cmd.pcHack_compat ? 4 : 3)
                                           : (g.Cmd.pcHack_compat ? 2 : 1))
                       : g.Cmd.swap_yz ? 5 : 0;

        if (!opts)
            return optn_err;
        Strcpy(opts, numpadmodes[indx]);
        return optn_ok;
    }
    if (req == do_handler) {
        return handler_number_pad();
    }
    return optn_ok;
}

static int
optfn_objects(int optidx UNUSED, int req, boolean negated UNUSED,
              char *opts, char *op UNUSED)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s", to_be_done);
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_packorder(int optidx UNUSED, int req, boolean negated UNUSED,
                char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        if (op == empty_optstr)
            return optn_err;
        if (!change_inv_order(op))
            return optn_err;
        return optn_ok;
    }
    if (req == get_val) {
        char ocl[MAXOCLASSES + 1];

        if (!opts)
            return optn_err;
        oc_to_str(flags.inv_order, ocl);
        Sprintf(opts, "%s", ocl);
        return optn_ok;
    }
    return optn_ok;
}

#ifdef CHANGE_COLOR
static int
optfn_palette(
    int optidx UNUSED, int req, boolean negated UNUSED,
    char *opts, char *op)
{
#ifndef WIN32
    int cnt, tmp, reverse;
    char *pt = op;
    long rgb;
#endif

    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        if (op == empty_optstr)
            return optn_err;
        /*
          Non-WIN32 variant
                 palette (00c/880/-fff is blue/yellow/reverse white)
          WIN32 variant
                 palette (adjust an RGB color in palette (color-R-G-B)
        */

        if (match_optname(opts, "palette", 3, TRUE)
#ifdef MAC
            || match_optname(opts, "hicolor", 3, TRUE)
#endif
            ) {
            int color_number, color_incr;

#ifndef WIN32
            if (duplicate)
                complain_about_duplicate(optidx);
#endif
#ifdef MAC
            if (match_optname(opts, "hicolor", 3, TRUE)) {
                color_number = CLR_MAX + 4; /* HARDCODED inverse number */
                color_incr = -1;
            } else
#endif
            {
                color_number = 0;
                color_incr = 1;
            }
#ifdef WIN32
            if (!alternative_palette(op)) {
                config_error_add("Error in palette parameter '%s'", op);
                return optn_err;
            }
#else
            while (*pt && color_number >= 0) {
                cnt = 3;
                rgb = 0L;
                if (*pt == '-') {
                    reverse = 1;
                    pt++;
                } else {
                    reverse = 0;
                }
                while (cnt-- > 0) {
                    if (*pt && *pt != '/') {
#ifdef AMIGA
                        rgb <<= 4;
#else
                        rgb <<= 8;
#endif
                        tmp = *pt++;
                        if (isalpha((uchar) tmp)) {
                            tmp = (tmp + 9) & 0xf; /* Assumes ASCII... */
                        } else {
                            tmp &= 0xf; /* Digits in ASCII too... */
                        }
#ifndef AMIGA
                        /* Add an extra so we fill f -> ff and 0 -> 00 */
                        rgb += tmp << 4;
#endif
                        rgb += tmp;
                    }
                }
                if (*pt == '/')
                    pt++;
                change_color(color_number, rgb, reverse);
                color_number += color_incr;
            }
#endif /* !WIN32 */
            if (!g.opt_initial) {
                g.opt_need_redraw = TRUE;
            }
        }
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        opts[0] = '\0';
        Sprintf(opts, "%s", get_color_string());
        return optn_ok;
    }
    return optn_ok;
}
#endif /* CHANGE_COLOR */

static int
optfn_paranoid_confirmation(
    int optidx, int req, boolean negated,
    char *opts, char *op)
{
    int i;

    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* user can change required response for some prompts (quit, die,
           hit), or add an extra prompt (pray, Remove) that isn't
           ordinarily there */

        if (strncmpi(opts, "prayconfirm", 4) != 0) { /* not prayconfirm */
            /* at present we don't complain about duplicates for this
               option, but we do throw away the old settings whenever
               we process a new one [clearing old flags is essential
               for handling default paranoid_confirm:pray sanely] */
            flags.paranoia_bits = 0; /* clear all */
            if (negated) {
                flags.paranoia_bits = 0; /* [now redundant...] */
            } else if (op != empty_optstr) {
                char *pp, buf[BUFSZ];

                strncpy(buf, op, sizeof buf - 1);
                buf[sizeof buf - 1] = '\0';
                op = mungspaces(buf);
                for (;;) {
                    /* We're looking to parse
                       "paranoid_confirm:whichone wheretwo whothree"
                       and "paranoid_confirm:" prefix has already
                       been stripped off by the time we get here */
                    pp = index(op, ' ');
                    if (pp)
                        *pp = '\0';
                    /* we aren't matching option names but match_optname()
                       does what we want once we've broken the space
                       delimited aggregate into separate tokens */
                    for (i = 0; i < SIZE(paranoia); ++i) {
                        if (match_optname(op, paranoia[i].argname,
                                          paranoia[i].argMinLen, FALSE)
                            || (paranoia[i].synonym
                                && match_optname(op, paranoia[i].synonym,
                                                 paranoia[i].synMinLen,
                                                 FALSE))) {
                            if (paranoia[i].flagmask)
                                flags.paranoia_bits |= paranoia[i].flagmask;
                            else /* 0 == "none", so clear all */
                                flags.paranoia_bits = 0;
                            break;
                        }
                    }
                    if (i == SIZE(paranoia)) {
                        /* didn't match anything, so arg is bad;
                           any flags already set will stay set */
                        config_error_add("Unknown %s parameter '%s'",
                                         allopt[optidx].name, op);
                        return optn_err;
                    }
                    /* move on to next token */
                    if (pp)
                        op = pp + 1;
                    else
                        break; /* no next token */
                }              /* for(;;) */
            } else
                return optn_err;
            return optn_ok;
        } else { /* prayconfirm */
            if (negated)
                flags.paranoia_bits &= ~PARANOID_PRAY;
            else
                flags.paranoia_bits |= PARANOID_PRAY;
        }
        return optn_ok;
    }
    if (req == get_val) {
        char tmpbuf[QBUFSZ];

        if (!opts)
            return optn_err;
        tmpbuf[0] = '\0';
        for (i = 0; paranoia[i].flagmask != 0; ++i)
            if (flags.paranoia_bits & paranoia[i].flagmask)
                Sprintf(eos(tmpbuf), " %s", paranoia[i].argname);
        Strcpy(opts, tmpbuf[0] ? &tmpbuf[1] : "none");
        return optn_ok;
    }
    if (req == do_handler) {
        return handler_paranoid_confirmation();
    }
    return optn_ok;
}

static int
optfn_petattr(int optidx, int req, boolean negated, char *opts, char *op)
{
    int retval = optn_ok;

    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* WINCAP2 petattr:string */

        op = string_for_opt(opts, negated);
        if (op != empty_optstr && negated) {
            bad_negation(allopt[optidx].name, TRUE);
            retval = optn_err;
        } else if (op != empty_optstr) {
#ifdef CURSES_GRAPHICS
            int itmp = curses_read_attrs(op);

            if (itmp == -1) {
                config_error_add("Unknown %s parameter '%s'",
                                 allopt[optidx].name, opts);
                retval = optn_err;
            } else
                iflags.wc2_petattr = itmp;
#else
            /* non-curses windowports will not use this flag anyway
             * but the above will not compile if we don't have curses.
             * Just set it to a sensible default: */
            iflags.wc2_petattr = ATR_INVERSE;
#endif
        } else if (negated) {
            iflags.wc2_petattr = ATR_NONE;
        }
        if (retval != optn_err) {
            iflags.hilite_pet = (iflags.wc2_petattr != ATR_NONE);
            if (!g.opt_initial)
                g.opt_need_redraw = TRUE;
        }
        return retval;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;

#ifdef CURSES_GRAPHICS
        if (WINDOWPORT(curses)) {
            char tmpbuf[QBUFSZ];

            Strcpy(opts, curses_fmt_attrs(tmpbuf));
        } else
#endif
        if (iflags.wc2_petattr != 0)
            Sprintf(opts, "0x%08x", iflags.wc2_petattr);
        else
            Strcpy(opts, defopt);
    }
    return optn_ok;
}

static int
optfn_pettype(int optidx, int req, boolean negated, char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        if ((op = string_for_env_opt(allopt[optidx].name, opts, negated))
            != empty_optstr) {
            switch (lowc(*op)) {
            case 'd': /* dog */
                g.preferred_pet = 'd';
                break;
            case 'c': /* cat */
            case 'f': /* feline */
                g.preferred_pet = 'c';
                break;
            case 'h': /* horse */
            case 'q': /* quadruped */
                /* avoids giving "unrecognized type of pet" but
                   pet_type(dog.c) won't actually honor this */
                g.preferred_pet = 'h';
                break;
            case 'n': /* no pet */
                g.preferred_pet = 'n';
                break;
            case '*': /* random */
                g.preferred_pet = '\0';
                break;
            default:
                config_error_add("Unrecognized pet type '%s'.", op);
                return optn_err;
                break;
            }
        } else if (negated)
            g.preferred_pet = 'n';
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s", (g.preferred_pet == 'c') ? "cat"
                           : (g.preferred_pet == 'd') ? "dog"
                             : (g.preferred_pet == 'h') ? "horse"
                               : (g.preferred_pet == 'n') ? "none"
                                 : "random");
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_pickup_burden(
    int optidx, int req, boolean negated UNUSED,
    char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* maximum burden picked up before prompt (Warren Cheung) */

        if ((op = string_for_env_opt(allopt[optidx].name, opts, FALSE))
            != empty_optstr) {
            switch (lowc(*op)) {
            case 'u': /* Unencumbered */
                flags.pickup_burden = UNENCUMBERED;
                break;
            case 'b': /* Burdened (slight encumbrance) */
                flags.pickup_burden = SLT_ENCUMBER;
                break;
            case 's': /* streSsed (moderate encumbrance) */
                flags.pickup_burden = MOD_ENCUMBER;
                break;
            case 'n': /* straiNed (heavy encumbrance) */
                flags.pickup_burden = HVY_ENCUMBER;
                break;
            case 'o': /* OverTaxed (extreme encumbrance) */
            case 't':
                flags.pickup_burden = EXT_ENCUMBER;
                break;
            case 'l': /* overLoaded */
                flags.pickup_burden = OVERLOADED;
                break;
            default:
                config_error_add("Unknown %s parameter '%s'",
                                 allopt[optidx].name, op);
                return optn_err;
            }
        } else
            return optn_err;
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s", burdentype[flags.pickup_burden]);
        return optn_ok;
    }
    if (req == do_handler) {
        return handler_pickup_burden();
    }
    return optn_ok;
}

static int
optfn_pickup_types(int optidx, int req, boolean negated, char *opts, char *op)
{
    char ocl[MAXOCLASSES + 1], tbuf[MAXOCLASSES + 1], qbuf[QBUFSZ],
        abuf[BUFSZ];
    int oc_sym;
    unsigned num;
    boolean badopt = FALSE, compat = (strlen(opts) <= 6), use_menu;

    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* types of objects to pick up automatically */

        oc_to_str(flags.pickup_types, tbuf);
        flags.pickup_types[0] = '\0'; /* all */
        op = string_for_opt(opts, (compat || !g.opt_initial));
        if (op == empty_optstr) {
            if (compat || negated || g.opt_initial) {
                /* for backwards compatibility, "pickup" without a
                   value is a synonym for autopickup of all types
                   (and during initialization, we can't prompt yet) */
                flags.pickup = !negated;
                return optn_ok;
            }
            oc_to_str(flags.inv_order, ocl);
            use_menu = TRUE;
            if (flags.menu_style == MENU_TRADITIONAL
                || flags.menu_style == MENU_COMBINATION) {
                boolean wasspace;

                use_menu = FALSE;
                Sprintf(qbuf, "New %s: [%s am] (%s)", allopt[optidx].name,
                        ocl, *tbuf ? tbuf : "all");
                abuf[0] = '\0';
                getlin(qbuf, abuf);
                wasspace = (abuf[0] == ' '); /* before mungspaces */
                op = mungspaces(abuf);
                if (wasspace && !abuf[0])
                    ; /* one or more spaces will remove old value */
                else if (!abuf[0] || abuf[0] == '\033')
                    op = tbuf; /* restore */
                else if (abuf[0] == 'm')
                    use_menu = TRUE;
                /* note: abuf[0]=='a' is already handled via clearing the
                   the old value (above) as a default action */
            }
            if (use_menu) {
                if (wizard && !index(ocl, VENOM_SYM))
                    strkitten(ocl, VENOM_SYM);
                (void) choose_classes_menu("Autopickup what?", 1, TRUE, ocl,
                                           tbuf);
                op = tbuf;
            }
        }
        if (negated) {
            bad_negation(allopt[optidx].name, TRUE);
            return optn_err;
        }
        while (*op == ' ')
            op++;
        if (*op != 'a' && *op != 'A') {
            num = 0;
            while (*op) {
                oc_sym = def_char_to_objclass(*op);
                /* make sure all are valid obj symbols occurring once */
                if (oc_sym != MAXOCLASSES
                    && !index(flags.pickup_types, oc_sym)) {
                    flags.pickup_types[num] = (char) oc_sym;
                    flags.pickup_types[++num] = '\0';
                } else
                    badopt = TRUE;
                op++;
            }
            if (badopt) {
                config_error_add("Unknown %s parameter '%s'",
                                 allopt[optidx].name, op);
                return optn_err;
            }
        }
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        oc_to_str(flags.pickup_types, ocl);
        Sprintf(opts, "%s", ocl[0] ? ocl : "all");
        return optn_ok;
    }
    if (req == do_handler) {
        return handler_pickup_types();
    }
    return optn_ok;
}

static int
optfn_pile_limit(int optidx, int req, boolean negated, char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* pile limit: when walking over objects, number which triggers
           "there are several/many objects here" instead of listing them
         */

        op = string_for_opt(opts, negated);
        if ((negated && op == empty_optstr)
            || (!negated && op != empty_optstr))
            flags.pile_limit = negated ? 0 : atoi(op);
        else if (negated) {
            bad_negation(allopt[optidx].name, TRUE);
            return optn_err;
        } else /* op == empty_optstr */
            flags.pile_limit = PILE_LIMIT_DFLT;
        /* sanity check */
        if (flags.pile_limit < 0)
            flags.pile_limit = PILE_LIMIT_DFLT;
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%d", flags.pile_limit);
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_player_selection(
    int optidx, int req, boolean negated,
    char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* WINCAP player_selection: dialog | prompt/prompts/prompting */

        op = string_for_opt(opts, negated);
        if (op != empty_optstr && !negated) {
            if (!strncmpi(op, "dialog", sizeof "dialog" - 1)) {
                iflags.wc_player_selection = VIA_DIALOG;
            } else if (!strncmpi(op, "prompt", sizeof "prompt" - 1)) {
                iflags.wc_player_selection = VIA_PROMPTS;
            } else {
                config_error_add("Unknown %s parameter '%s'",
                                 allopt[optidx].name, op);
                return optn_err;
            }
        }
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s",
                iflags.wc_player_selection ? "prompts" : "dialog");
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_playmode(int optidx, int req, boolean negated, char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* play mode: normal, explore/discovery, or debug/wizard */

        if (duplicate || negated)
            return optn_err;
        if (op == empty_optstr)
            return optn_err;
        if (!strncmpi(op, "normal", 6) || !strcmpi(op, "play")) {
            wizard = discover = FALSE;
        } else if (!strncmpi(op, "explore", 6)
                   || !strncmpi(op, "discovery", 6)) {
            wizard = FALSE, discover = TRUE;
        } else if (!strncmpi(op, "debug", 5) || !strncmpi(op, "wizard", 6)) {
            wizard = TRUE, discover = FALSE;
        } else {
            config_error_add("Invalid value for \"%s\":%s",
                             allopt[optidx].name, op);
            return optn_err;
        }
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Strcpy(opts, wizard ? "debug" : discover ? "explore" : "normal");
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_race(int optidx, int req, boolean negated, char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* race:string */
        if (parse_role_opts(optidx, negated, allopt[optidx].name, opts, &op)) {
            if ((flags.initrace = str2race(op)) == ROLE_NONE) {
                config_error_add("Unknown %s '%s'", allopt[optidx].name, op);
                return optn_err;
            } else /* Backwards compatibility */
                g.pl_race = *op;
        } else
            return optn_silenterr;
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s", rolestring(flags.initrace, races, noun));
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_roguesymset(int optidx, int req, boolean negated UNUSED,
                  char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        if (op != empty_optstr) {
            g.symset[ROGUESET].name = dupstr(op);
            if (!read_sym_file(ROGUESET)) {
                clear_symsetentry(ROGUESET, TRUE);
                config_error_add(
                    "Unable to load symbol set \"%s\" from \"%s\"", op,
                    SYMBOLS);
                return optn_err;
            } else {
                if (!g.opt_initial && Is_rogue_level(&u.uz))
                    assign_graphics(ROGUESET);
                g.opt_need_redraw = TRUE;
            }
        } else
            return optn_err;
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s",
                g.symset[ROGUESET].name ? g.symset[ROGUESET].name : "default");
        if (g.currentgraphics == ROGUESET && g.symset[ROGUESET].name)
            Strcat(opts, ", active");
        return optn_ok;
    }
    if (req == do_handler) {
        return handler_symset(optidx);
    }
    return optn_ok;
}

static int
optfn_role(int optidx, int req, boolean negated, char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        if (parse_role_opts(optidx, negated, allopt[optidx].name, opts, &op)) {
            if ((flags.initrole = str2role(op)) == ROLE_NONE) {
                config_error_add("Unknown %s '%s'", allopt[optidx].name, op);
                return optn_err;
            } else /* Backwards compatibility */
                nmcpy(g.pl_character, op, PL_NSIZ);
        } else
            return optn_silenterr;
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s", rolestring(flags.initrole, roles, name.m));
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_runmode(int optidx, int req, boolean negated, char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        if (negated) {
            flags.runmode = RUN_TPORT;
        } else if (op != empty_optstr) {
            if (str_start_is("teleport", op, TRUE))
                flags.runmode = RUN_TPORT;
            else if (str_start_is("run", op, TRUE))
                flags.runmode = RUN_LEAP;
            else if (str_start_is("walk", op, TRUE))
                flags.runmode = RUN_STEP;
            else if (str_start_is("crawl", op, TRUE))
                flags.runmode = RUN_CRAWL;
            else {
                config_error_add("Unknown %s parameter '%s'",
                                 allopt[optidx].name, op);
                return optn_err;
            }
        } else {
            config_error_add("Value is mandatory for %s", allopt[optidx].name);
            return optn_err;
        }
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s", runmodes[flags.runmode]);
        return optn_ok;
    }
    if (req == do_handler) {
        return handler_runmode();
    }
    return optn_ok;
}

static int
optfn_scores(int optidx, int req, boolean negated, char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* scores:5t[op] 5a[round] o[wn] */

        if ((op = string_for_opt(opts, FALSE)) == empty_optstr)
            return optn_err;

        /* 3.7: earlier versions left old values for unspecified arguments
           if player's scores:foo option only specified some of the three;
           in particular, attempting to use 'scores:own' rather than
           'scores:0 top/0 around/own' didn't work as intended */
        flags.end_top = flags.end_around = 0, flags.end_own = FALSE;

        if (negated)
            op = eos(op);

        while (*op) {
            int inum = 1;

            negated = (*op == '!');
            if (negated)
                op++;

            if (digit(*op)) {
                inum = atoi(op);
                while (digit(*op))
                    op++;
            }
            while (*op == ' ')
                op++;

            switch (lowc(*op)) {
            case 't':
                flags.end_top = negated ? 0 : inum;
                break;
            case 'a':
                flags.end_around = negated ? 0 : inum;
                break;
            case 'o':
                flags.end_own = (negated || !inum) ? FALSE : TRUE;
                break;
            case 'n': /* none */
                flags.end_top = flags.end_around = 0, flags.end_own = FALSE;
                break;
            case '-':
                if (digit(*(op + 1))) {
                    config_error_add(
                       "Values for %s:top and %s:around must not be negative",
                                     allopt[optidx].name,
                                     allopt[optidx].name);
                    return optn_silenterr;
                }
                /*FALLTHRU*/
            default:
                config_error_add("Unknown %s parameter '%s'",
                                 allopt[optidx].name, op);
                return optn_silenterr;
            }
            /* "3a" is sufficient but accept "3around" (or "3abracadabra") */
            while (letter(*op))
                op++;
            /* t, a, and o can be separated by space(s) or slash or both */
            while (*op == ' ')
                op++;
            if (*op == '/')
                op++;
        }
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        *opts = '\0';
        if (flags.end_top > 0)
            Sprintf(opts, "%d top", flags.end_top);
        if (flags.end_around > 0)
            Sprintf(eos(opts), "%s%d around",
                    (flags.end_top > 0) ? "/" : "", flags.end_around);
        if (flags.end_own)
            Sprintf(eos(opts), "%sown",
                    (flags.end_top > 0 || flags.end_around > 0) ? "/" : "");
        if (!*opts)
            Strcpy(opts, "none");
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_scroll_amount(
    int optidx, int req, boolean negated,
    char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* WINCAP scroll_amount:nn */

        op = string_for_opt(opts, negated);
        if ((negated && op == empty_optstr)
            || (!negated && op != empty_optstr)) {
            iflags.wc_scroll_amount = negated ? 1 : atoi(op);
        } else if (negated) {
            bad_negation(allopt[optidx].name, TRUE);
            return optn_err;
        }
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        if (iflags.wc_scroll_amount)
            Sprintf(opts, "%d", iflags.wc_scroll_amount);
        else
            Strcpy(opts, defopt);
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_scroll_margin(
    int optidx, int req, boolean negated,
    char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* WINCAP scroll_margin:nn */
        op = string_for_opt(opts, negated);
        if ((negated && op == empty_optstr)
            || (!negated && op != empty_optstr)) {
            iflags.wc_scroll_margin = negated ? 5 : atoi(op);
        } else if (negated) {
            bad_negation(allopt[optidx].name, TRUE);
            return optn_err;
        }
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        if (iflags.wc_scroll_margin)
            Sprintf(opts, "%d", iflags.wc_scroll_margin);
        else
            Strcpy(opts, defopt);
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_sortdiscoveries(
    int optidx, int req, boolean negated,
    char *opts, char *op)
{
    if (req == do_init) {
        flags.discosort = 'o';
        return optn_ok;
    }
    if (req == do_set) {
        op = string_for_env_opt(allopt[optidx].name, opts, FALSE);
        if (negated) {
            flags.discosort = 'o';
        } else if (op != empty_optstr) {
            switch (lowc(*op)) {
            case '0':
            case 'o': /* order of discovery */
                flags.discosort = 'o';
                break;
            case '1':
            case 's': /* sortloot order (subclasses for some classes) */
                flags.discosort = 's';
                break;
            case '2':
            case 'c': /* alphabetical within each class */
                flags.discosort = 'c';
                break;
            case '3':
            case 'a': /* alphabetical across all classes */
                flags.discosort = 'a';
                break;
            default:
                config_error_add("Unknown %s parameter '%s'",
                                 allopt[optidx].name, op);
                return optn_silenterr;
            }
        } else
            return optn_err;
        return optn_ok;
    }
    if (req == get_val) {
        extern const char *const disco_orders_descr[]; /* o_init.c */
        extern const char disco_order_let[];
        const char *p = index(disco_order_let, flags.discosort);

        if (!p)
            flags.discosort = 'o', p = disco_order_let;
        Strcpy(opts, disco_orders_descr[p - disco_order_let]);
        return optn_ok;
    }
    if (req == do_handler) {
        /* return handler_sortdiscoveries(); */
        (void) choose_disco_sort(0); /* o_init.c */
    }
    return optn_ok;
}

static int
optfn_sortloot(int optidx, int req, boolean negated UNUSED,
               char *opts, char *op)
{
    int i;

    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        op = string_for_env_opt(allopt[optidx].name, opts, FALSE);
        if (op != empty_optstr) {
            char c = lowc(*op);

            switch (c) {
            case 'n': /* none */
            case 'l': /* loot (pickup) */
            case 'f': /* full (pickup + invent) */
                flags.sortloot = c;
                break;
            default:
                config_error_add("Unknown %s parameter '%s'",
                                 allopt[optidx].name, op);
                return optn_err;
            }
        } else
            return optn_err;
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        for (i = 0; i < SIZE(sortltype); i++)
            if (flags.sortloot == sortltype[i][0]) {
                Strcpy(opts, sortltype[i]);
                break;
            }
        return optn_ok;
    }
    if (req == do_handler) {
        return handler_sortloot();
    }
    return optn_ok;
}

static int
optfn_statushilites(
    int optidx UNUSED,
    int req,
    boolean negated,
    char *opts,
    char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* control over whether highlights should be displayed (non-zero), and
           also for how long to show temporary ones (N turns; default 3) */

#ifdef STATUS_HILITES
        if (negated) {
            iflags.hilite_delta = 0L;
        } else {
            op = string_for_opt(opts, TRUE);
            iflags.hilite_delta = (op == empty_optstr || !*op) ? 3L
                                                               : atol(op);
            if (iflags.hilite_delta < 0L)
                iflags.hilite_delta = 1L;
        }
        if (!g.opt_from_file)
            reset_status_hilites();
        return optn_ok;
#else
        nhUse(negated);
        nhUse(op);
        config_error_add("'%s' is not supported", allopt[optidx].name);
        return optn_err;
#endif
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
#ifdef STATUS_HILITES
        if (!iflags.hilite_delta)
            Strcpy(opts, "0 (off: don't highlight status fields)");
        else
            Sprintf(opts, "%ld (on: highlight status for %ld turns)",
                    iflags.hilite_delta, iflags.hilite_delta);
#else
        Strcpy(opts, "unsupported");
#endif
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_statuslines(int optidx, int req, boolean negated, char *opts, char *op)
{
    int retval = optn_ok, itmp = 0;

    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* WINCAP2
         * statuslines:n */

        op = string_for_opt(opts, negated);
        if (negated) {
            bad_negation(allopt[optidx].name, TRUE);
            itmp = 2;
            retval = optn_err;
        } else if (op != empty_optstr) {
            itmp = atoi(op);
        }
        if (itmp < 2 || itmp > 3) {
            config_error_add("'%s:%s' is invalid; must be 2 or 3",
                             allopt[optidx].name, op);
            retval = optn_silenterr;
        } else {
            iflags.wc2_statuslines = itmp;
            if (!g.opt_initial)
                g.opt_need_redraw = TRUE;
        }
        return retval;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        if (wc2_supported(allopt[optidx].name))
            Strcpy(opts, (iflags.wc2_statuslines < 3) ? "2" : "3");
        else
            Strcpy(opts, "unknown");
        return optn_ok;
    }
    return optn_ok;
}

#ifdef WIN32
static int
optfn_subkeyvalue(int optidx UNUSED, int req, boolean negated UNUSED,
                  char *opts, char *op UNUSED)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        if (op == empty_optstr)
            return optn_err;
#ifdef TTY_GRAPHICS
        map_subkeyvalue(op);
#endif
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        opts[0] = '\0';
        return optn_ok;
    }
    return optn_ok;
}
#endif /* WIN32 */

static int
optfn_suppress_alert(int optidx, int req, boolean negated,
                     char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        if (negated) {
            bad_negation(allopt[optidx].name, FALSE);
            return optn_err;
        } else if (op != empty_optstr)
            (void) feature_alert_opts(op, allopt[optidx].name);
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        if (flags.suppress_alert == 0L)
            Strcpy(opts, none);
        else
            Sprintf(opts, "%lu.%lu.%lu", FEATURE_NOTICE_VER_MAJ,
                    FEATURE_NOTICE_VER_MIN, FEATURE_NOTICE_VER_PATCH);
        return optn_ok;
    }
    return optn_ok;
}

extern const char *known_handling[];     /* symbols.c */
extern const char *known_restrictions[]; /* symbols.c */

static int
optfn_symset(int optidx UNUSED, int req, boolean negated UNUSED, char *opts,
             char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        if (op != empty_optstr) {
            g.symset[PRIMARYSET].name = dupstr(op);
            if (!read_sym_file(PRIMARYSET)) {
                clear_symsetentry(PRIMARYSET, TRUE);
                config_error_add(
                    "Unable to load symbol set \"%s\" from \"%s\"", op,
                    SYMBOLS);
                return optn_err;
            } else {
                if (g.symset[PRIMARYSET].handling) {
#ifndef ENHANCED_SYMBOLS
                    if (g.symset[PRIMARYSET].handling == H_UTF8) {
                        config_error_add("Unavailable symset handler \"%s\" for %s",
                                         known_handling[H_UTF8], op);
                        load_symset("default", PRIMARYSET);
                    }
#endif
                    switch_symbols(g.symset[PRIMARYSET].name != (char *) 0);
                    g.opt_need_redraw = g.opt_need_glyph_reset = TRUE;
                }
            }
        } else
            return optn_err;
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s",
                g.symset[PRIMARYSET].name ? g.symset[PRIMARYSET].name : "default");
        if (g.currentgraphics == PRIMARYSET && g.symset[PRIMARYSET].name)
            Strcat(opts, ", active");
        if (g.symset[PRIMARYSET].handling) {
            Sprintf(eos(opts), ", handler=%s",
                    known_handling[g.symset[PRIMARYSET].handling]);
        }
        return optn_ok;
    }
    if (req == do_handler) {
        int reslt;

        if (g.symset[PRIMARYSET].handling == H_UTF8) {
#ifdef ENHANCED_SYMBOLS
            if (!glyphid_cache_status())
                fill_glyphid_cache();
#endif
        }
        reslt = handler_symset(optidx);
        if (g.symset[PRIMARYSET].handling == H_UTF8) {
#ifdef ENHANCED_SYMBOLS
            if (glyphid_cache_status())
                free_glyphid_cache();
#endif
        }
        return reslt;
    }
    return optn_ok;
}

static int
optfn_term_cols(int optidx, int req, boolean negated, char *opts, char *op)
{
    int retval = optn_ok;
    long ltmp;

    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* WINCAP2
         * term_cols:amount */

        if ((op = string_for_opt(opts, negated)) != empty_optstr) {
            ltmp = atol(op);
            /* just checks atol() sanity, not logical window size sanity
             */
            if (ltmp <= 0L || ltmp >= (long) LARGEST_INT) {
                config_error_add("Invalid %s: %ld", allopt[optidx].name,
                                 ltmp);
                retval = optn_err;
            } else {
                iflags.wc2_term_cols = (int) ltmp;
            }
        }
        return retval;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        if (iflags.wc2_term_cols)
            Sprintf(opts, "%d", iflags.wc2_term_cols);
        else
            Strcpy(opts, defopt);
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_term_rows(int optidx, int req, boolean negated, char *opts, char *op)
{
    int retval = optn_ok;
    long ltmp;

    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* WINCAP2
         * term_rows:amount */

        if ((op = string_for_opt(opts, negated)) != empty_optstr) {
            ltmp = atol(op);
            /* just checks atol() sanity, not logical window size sanity
             */
            if (ltmp <= 0L || ltmp >= (long) LARGEST_INT) {
                config_error_add("Invalid %s: %ld", allopt[optidx].name,
                                 ltmp);
                retval = optn_err;
            } else {
                iflags.wc2_term_rows = (int) ltmp;
            }
        }
        return retval;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        if (iflags.wc2_term_rows)
            Sprintf(opts, "%d", iflags.wc2_term_rows);
        else
            Strcpy(opts, defopt);
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_tile_file(int optidx UNUSED, int req, boolean negated UNUSED,
                char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* WINCAP tile_file:name */
        if (op != empty_optstr) {
            if (iflags.wc_tile_file)
                free(iflags.wc_tile_file);
            iflags.wc_tile_file = dupstr(op);
        } else
            return optn_err;
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s",
                iflags.wc_tile_file ? iflags.wc_tile_file : defopt);
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_tile_height(int optidx, int req, boolean negated, char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* WINCAP tile_height:nn */
        op = string_for_opt(opts, negated);
        if ((negated && op == empty_optstr)
            || (!negated && op != empty_optstr)) {
            iflags.wc_tile_height = negated ? 0 : atoi(op);
        } else if (negated) {
            bad_negation(allopt[optidx].name, TRUE);
            return optn_err;
        }
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        if (iflags.wc_tile_height)
            Sprintf(opts, "%d", iflags.wc_tile_height);
        else
            Strcpy(opts, defopt);
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_tile_width(int optidx, int req, boolean negated, char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* WINCAP tile_width:nn */
        op = string_for_opt(opts, negated);
        if ((negated && op == empty_optstr)
            || (!negated && op != empty_optstr)) {
            iflags.wc_tile_width = negated ? 0 : atoi(op);
        } else if (negated) {
            bad_negation(allopt[optidx].name, TRUE);
            return optn_err;
        }
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        if (iflags.wc_tile_width)
            Sprintf(opts, "%d", iflags.wc_tile_width);
        else
            Strcpy(opts, defopt);
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_traps(int optidx UNUSED, int req, boolean negated UNUSED,
            char *opts, char *op UNUSED)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s", to_be_done);
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_vary_msgcount(
    int optidx, int req, boolean negated,
    char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* WINCAP vary_msgcount:nn */
        op = string_for_opt(opts, negated);
        if ((negated && op == empty_optstr)
            || (!negated && op != empty_optstr)) {
            iflags.wc_vary_msgcount = negated ? 0 : atoi(op);
        } else if (negated) {
            bad_negation(allopt[optidx].name, TRUE);
            return optn_err;
        }
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        if (iflags.wc_vary_msgcount)
            Sprintf(opts, "%d", iflags.wc_vary_msgcount);
        else
            Strcpy(opts, defopt);
        return optn_ok;
    }
    return optn_ok;
}

#ifdef VIDEOSHADES
static int
optfn_videocolors(int optidx, int req, boolean negated UNUSED,
                  char *opts, char *op UNUSED)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* videocolors:string */

        if ((opts = string_for_env_opt(allopt[optidx].name, opts, FALSE))
            == empty_optstr) {
            return optn_err;
        }
        if (!assign_videocolors(opts)) {
            config_error_add("Unknown error handling '%s'",
                             allopt[optidx].name);
            return optn_err;
        }
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d",
                ttycolors[CLR_RED], ttycolors[CLR_GREEN],
                ttycolors[CLR_BROWN], ttycolors[CLR_BLUE],
                ttycolors[CLR_MAGENTA], ttycolors[CLR_CYAN],
                ttycolors[CLR_ORANGE], ttycolors[CLR_BRIGHT_GREEN],
                ttycolors[CLR_YELLOW], ttycolors[CLR_BRIGHT_BLUE],
                ttycolors[CLR_BRIGHT_MAGENTA], ttycolors[CLR_BRIGHT_CYAN]);
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_videoshades(int optidx, int req, boolean negated UNUSED,
                  char *opts, char *op UNUSED)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* videoshades:string */

        if ((opts = string_for_env_opt(allopt[optidx].name, opts, FALSE))
            == empty_optstr) {
            return optn_err;
        }
        if (!assign_videoshades(opts)) {
            config_error_add("Unknown error handling '%s'",
                             allopt[optidx].name);
            return optn_err;
        }
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s-%s-%s", shade[0], shade[1], shade[2]);
        return optn_ok;
    }
    return optn_ok;
}
#endif /* VIDEOSHADES */

#ifdef MSDOS
static int
optfn_video_width(int optidx UNUSED, int req, boolean negated,
                  char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        if ((op = string_for_opt(opts, negated)) != empty_optstr)
            iflags.wc_video_width = strtol(op, NULL, 10);
        else
            return optn_err;
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        opts[0] = '\0';
    }
    return optn_ok;
}

static int
optfn_video_height(int optidx UNUSED, int req, boolean negated,
                   char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        if ((op = string_for_opt(opts, negated)) != empty_optstr)
            iflags.wc_video_height = strtol(op, NULL, 10);
        else
            return optn_err;
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        opts[0] = '\0';
    }
    return optn_ok;
}

#ifdef NO_TERMS
static int
optfn_video(int optidx, int req, boolean negated UNUSED,
            char *opts, char *op UNUSED)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* video:string */
        if ((opts = string_for_env_opt(allopt[optidx].name, opts, FALSE))
            == empty_optstr) {
            return optn_err;
        }
        if (!assign_video(opts)) {
            config_error_add("Unknown error handling '%s'",
                             allopt[optidx].name);
            return optn_err;
        }
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s", to_be_done);
        return optn_ok;
    }
    return optn_ok;
}

#endif /* NO_TERMS */
#endif /* MSDOS */

static int
optfn_warnings(int optidx, int req, boolean negated UNUSED,
               char *opts, char *op UNUSED)
{
    int reslt;

    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        reslt = warning_opts(opts, allopt[optidx].name);
        return reslt ? optn_ok : optn_err;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        opts[0] = '\0';
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_whatis_coord(int optidx, int req, boolean negated, char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        if (negated) {
            iflags.getpos_coords = GPCOORDS_NONE;
            return optn_ok;
        } else if ((op = string_for_env_opt(allopt[optidx].name, opts, FALSE))
                   != empty_optstr) {
            static char gpcoords[] = { GPCOORDS_NONE,    GPCOORDS_COMPASS,
                                       GPCOORDS_COMFULL, GPCOORDS_MAP,
                                       GPCOORDS_SCREEN,  '\0' };
            char c = lowc(*op);

            if (c && index(gpcoords, c))
                iflags.getpos_coords = c;
            else {
                config_error_add("Unknown %s parameter '%s'",
                                 allopt[optidx].name, op);
                return optn_err;
            }
        } else
            return optn_err;
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s",
                (iflags.getpos_coords == GPCOORDS_MAP) ? "map"
                : (iflags.getpos_coords == GPCOORDS_COMPASS) ? "compass"
                : (iflags.getpos_coords == GPCOORDS_COMFULL) ? "full compass"
                : (iflags.getpos_coords == GPCOORDS_SCREEN) ? "screen"
                : "none");
        return optn_ok;
    }
    if (req == do_handler) {
        return handler_whatis_coord();
    }
    return optn_ok;
}

static int
optfn_whatis_filter(
    int optidx, int req, boolean negated,
    char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        if (negated) {
            iflags.getloc_filter = GFILTER_NONE;
            return optn_ok;
        } else if ((op = string_for_env_opt(allopt[optidx].name, opts, FALSE))
                   != empty_optstr) {
            char c = lowc(*op);

            switch (c) {
            case 'n':
                iflags.getloc_filter = GFILTER_NONE;
                break;
            case 'v':
                iflags.getloc_filter = GFILTER_VIEW;
                break;
            case 'a':
                iflags.getloc_filter = GFILTER_AREA;
                break;
            default: {
                config_error_add("Unknown %s parameter '%s'",
                                 allopt[optidx].name, op);
                return optn_err;
            }
            }
        } else
            return optn_err;
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s",
                (iflags.getloc_filter == GFILTER_VIEW) ? "view"
                : (iflags.getloc_filter == GFILTER_AREA) ? "area"
                : "none");
        return optn_ok;
    }
    if (req == do_handler) {
        return handler_whatis_filter();
    }
    return optn_ok;
}

static int
optfn_windowborders(
    int optidx, int req, boolean negated,
    char *opts, char *op)
{
    int retval = optn_ok;

    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        op = string_for_opt(opts, negated);
        if (negated && op != empty_optstr) {
            bad_negation(allopt[optidx].name, TRUE);
            retval = optn_err;
        } else {
            int itmp;

            if (negated)
                itmp = 0; /* Off */
            else if (op == empty_optstr)
                itmp = 1; /* On */
            else /* Value supplied; expect 0 (off), 1 (on), 2 (auto)
                  * or 3 (on for most windows, off for perm_invent)
                  * or 4 (auto for most windows, off for perm_invent) */
                itmp = atoi(op);

            if (itmp < 0 || itmp > 4) {
                config_error_add("Invalid %s (should be within 0 to 4): %s",
                                 allopt[optidx].name, opts);
                retval = optn_silenterr;
            } else {
                iflags.wc2_windowborders = itmp;
            }
        }
        return retval;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s",
                (iflags.wc2_windowborders == 0) ? "0=off"
                : (iflags.wc2_windowborders == 1) ? "1=on"
                  : (iflags.wc2_windowborders == 2) ? "2=auto"
                    : (iflags.wc2_windowborders == 3)
                      ? "3=on, except off for perm_invent"
                      : (iflags.wc2_windowborders == 4)
                        ? "4=auto, except off for perm_invent"
                        : defopt);
        return optn_ok;
    }
    return optn_ok;
}

#ifdef WINCHAIN
static int
optfn_windowchain(int optidx, int req, boolean negated UNUSED, char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        if ((op = string_for_env_opt(allopt[optidx].name, opts, FALSE))
            != empty_optstr) {
            char buf[WINTYPELEN];

            nmcpy(buf, op, WINTYPELEN);
            addto_windowchain(buf);
        } else
            return optn_err;
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        opts[0] = '\0';
        return optn_ok;
    }
    return optn_ok;
}
#endif

static int
optfn_windowcolors(int optidx, int req, boolean negated UNUSED,
                   char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /* WINCAP
         * setting window colors
         * syntax: windowcolors=menu foregrnd/backgrnd text
         * foregrnd/backgrnd
         */
        if ((op = string_for_opt(opts, FALSE)) != empty_optstr) {
            if (!wc_set_window_colors(op)) {
                config_error_add("Could not set %s '%s'", allopt[optidx].name,
                                 op);
                return optn_err;
            }
        }
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(
            opts, "%s/%s %s/%s %s/%s %s/%s",
            iflags.wc_foregrnd_menu ? iflags.wc_foregrnd_menu : defbrief,
            iflags.wc_backgrnd_menu ? iflags.wc_backgrnd_menu : defbrief,
            iflags.wc_foregrnd_message ? iflags.wc_foregrnd_message
                                       : defbrief,
            iflags.wc_backgrnd_message ? iflags.wc_backgrnd_message
                                       : defbrief,
            iflags.wc_foregrnd_status ? iflags.wc_foregrnd_status : defbrief,
            iflags.wc_backgrnd_status ? iflags.wc_backgrnd_status : defbrief,
            iflags.wc_foregrnd_text ? iflags.wc_foregrnd_text : defbrief,
            iflags.wc_backgrnd_text ? iflags.wc_backgrnd_text : defbrief);
        return optn_ok;
    }
    return optn_ok;
}

static int
optfn_windowtype(int optidx, int req, boolean negated UNUSED,
                 char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        /*
         * windowtype:  option to choose the interface for binaries built
         * with support for more than one interface (tty + X11, for
         * instance).
         *
         * Ideally, 'windowtype' should be processed first, because it
         * causes the wc_ and wc2_ flags to be set up.
         * For user, making it be first in a config file is trivial, use
         * OPTIONS=windowtype:Foo
         * as the first non-comment line of the file.
         * Making it first in NETHACKOPTIONS requires it to be at the
         * _end_ because comma-separated option strings are processed from
         * right to left.
         */
        if (iflags.windowtype_locked)
            return optn_ok;

        if ((op = string_for_env_opt(allopt[optidx].name, opts, FALSE))
            != empty_optstr) {
            nmcpy(g.chosen_windowtype, op, WINTYPELEN);
            if (!iflags.windowtype_deferred) {
                choose_windows(g.chosen_windowtype);
            }
        } else
            return optn_err;
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s", windowprocs.name);
        return optn_ok;
    }
    return optn_ok;
}

/*
 *    Prefix-handling functions
 */

static int
pfxfn_cond_(int optidx UNUSED, int req, boolean negated,
            char *opts, char *op UNUSED)
{
    int reslt;

    if (req == do_init) {
        condopt(0, (boolean *) 0, 0); /* make the choices match defaults */
        return optn_ok;
    }
    if (req == do_set) {
        if ((reslt = parse_cond_option(negated, opts)) != 0) {
            switch (reslt) {
            case 3:
                config_error_add("Ambiguous condition option %s", opts);
                break;
            case 1:
            case 2:
            default:
                config_error_add("Unknown condition option %s (%d)", opts,
                                 reslt);
                break;
            }
            return optn_err;
        }
        g.opt_need_redraw = TRUE;
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        opts[0] = '\0';
        return optn_ok;
    }
    if (req == do_handler) {
        cond_menu();    /* in botl.c */
        return optn_ok;
    }
    return optn_ok;
}

static int
pfxfn_font(int optidx, int req, boolean negated, char *opts, char *op)
{
    int opttype = -1;

    if (req == do_init) {
        return optn_ok;
    }

    if (req == do_set) {
        /* WINCAP setting font options  */
        if (optidx == opt_font_map)
            opttype = MAP_OPTION;
        else if (optidx == opt_font_message)
            opttype = MESSAGE_OPTION;
        else if (optidx == opt_font_text)
            opttype = TEXT_OPTION;
        else if (optidx == opt_font_menu)
            opttype = MENU_OPTION;
        else if (optidx == opt_font_status)
            opttype = STATUS_OPTION;
        else if (optidx == opt_font_size_map
                || optidx == opt_font_size_message
                || optidx == opt_font_size_text
                || optidx == opt_font_size_menu
                || optidx == opt_font_size_status) {
            if (optidx == opt_font_size_map)
                opttype = MAP_OPTION;
            else if (optidx == opt_font_size_message)
                opttype = MESSAGE_OPTION;
            else if (optidx == opt_font_size_text)
                opttype = TEXT_OPTION;
            else if (optidx == opt_font_size_menu)
                opttype = MENU_OPTION;
            else if (optidx == opt_font_size_status)
                opttype = STATUS_OPTION;
            else {
                config_error_add("Unknown %s parameter '%s'",
                             allopt[optidx].name, opts);
                return optn_err;
            }
            if (duplicate)
                complain_about_duplicate(optidx);
            if (opttype > 0 && !negated
                && (op = string_for_opt(opts, FALSE)) != empty_optstr) {
                switch (opttype) {
                case MAP_OPTION:
                    iflags.wc_fontsiz_map = atoi(op);
                    break;
                case MESSAGE_OPTION:
                    iflags.wc_fontsiz_message = atoi(op);
                    break;
                case TEXT_OPTION:
                    iflags.wc_fontsiz_text = atoi(op);
                    break;
                case MENU_OPTION:
                    iflags.wc_fontsiz_menu = atoi(op);
                    break;
                case STATUS_OPTION:
                    iflags.wc_fontsiz_status = atoi(op);
                    break;
                }
            }
            return optn_ok;
        } else {
            config_error_add("Unknown %s parameter '%s'",
                             "font", opts);
            return FALSE;
        }
        if (opttype > 0
            && (op = string_for_opt(opts, FALSE)) != empty_optstr) {
            wc_set_font_name(opttype, op);
#ifdef MAC
            set_font_name(opttype, op);
#endif
            return optn_ok;
        } else if (negated) {
            bad_negation(allopt[optidx].name, TRUE);
            return optn_err;
        }
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;

        if (optidx == opt_font_map) {
            Sprintf(opts, "%s",
                    iflags.wc_font_map ? iflags.wc_font_map : defopt);
        } else if (optidx == opt_font_message) {
            Sprintf(opts, "%s",
                iflags.wc_font_message ? iflags.wc_font_message : defopt);
        } else if (optidx == opt_font_status) {
            Sprintf(opts, "%s",
                iflags.wc_font_status ? iflags.wc_font_status : defopt);
        } else if (optidx == opt_font_menu) {
            Sprintf(opts, "%s",
                iflags.wc_font_menu ? iflags.wc_font_menu : defopt);
        } else if (optidx == opt_font_text) {
            Sprintf(opts, "%s",
                iflags.wc_font_text ? iflags.wc_font_text : defopt);
        } else if (optidx == opt_font_size_map) {
            if (iflags.wc_fontsiz_map)
                Sprintf(opts, "%d", iflags.wc_fontsiz_map);
            else
                Strcpy(opts, defopt);
        } else if (optidx == opt_font_size_message) {
            if (iflags.wc_fontsiz_message)
                Sprintf(opts, "%d", iflags.wc_fontsiz_message);
            else
                Strcpy(opts, defopt);
        } else if (optidx == opt_font_size_status) {
            if (iflags.wc_fontsiz_status)
                Sprintf(opts, "%d", iflags.wc_fontsiz_status);
            else
                Strcpy(opts, defopt);
        } else if (optidx == opt_font_size_menu) {
            if (iflags.wc_fontsiz_menu)
                Sprintf(opts, "%d", iflags.wc_fontsiz_menu);
            else
                Strcpy(opts, defopt);
        } else if (optidx == opt_font_size_text) {
            if (iflags.wc_fontsiz_text)
                Sprintf(opts, "%d", iflags.wc_fontsiz_text);
            else
                Strcpy(opts, defopt);
        }
        return optn_ok;
    }
    return optn_ok;
}

#if defined(MICRO) && !defined(AMIGA)
int
pfxfn_IBM_(int optidx UNUSED, int req, boolean negated UNUSED,
           char *opts, char *op UNUSED)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        opts[0] = '\0';
        return optn_ok;
    }
    return optn_ok;
}
#endif

#ifndef NO_VERBOSE_GRANULARITY
int pfxfn_verbose(int optidx UNUSED, int req, boolean negated,
           char *opts, char *op)
{
    long ltmp = 0;
    int reslt;
    char *p;
    boolean param_optional = FALSE;

    if (req == do_init) {
        flags.verbose = allopt[optidx].opt_in_out;
        return optn_ok;
    }
    if (req == do_set) {
        if (opts) {
            if (!strncmp(opts, "verbose", 7)) {
                p = index("01234", *(opts + 7));
                if (p && *p == '\0')  /* plain verbose, not verboseN */
                    param_optional = TRUE;
                if ((op = string_for_opt(opts, param_optional)) != empty_optstr) {
                    ltmp = atol(op);
                    int idx;

                    if (p && (*p != '\0')) {
                        idx = *p - '0';
                        if (idx >= 0 && idx < vb_elements)
                            verbosity_suppressions[idx] = ltmp;
                        return optn_ok;
                    }
                } else {
                    if (param_optional) {
                        /* indicates plain verbose, not verboseN */
                        flags.verbose = (negated ? 0 : 1);
                        return optn_ok;
                    }
                }
            }
        }
        return optn_err;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, "%s,%ld,%ld,%ld,%ld,%ld", flags.verbose ? "On" : "Off",
                verbosity_suppressions[0], verbosity_suppressions[1],
                verbosity_suppressions[2], verbosity_suppressions[3],
                verbosity_suppressions[4]);
        return optn_ok;
    }
    if (req == do_handler) {
        reslt = handler_verbose(optidx);
        return reslt;
    }
    return optn_ok;
}
#endif

/*
 *    General boolean option handler
 *    (Use optidx to reference the specific option)
 */

static int
optfn_boolean(int optidx, int req, boolean negated, char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        boolean nosexchange = FALSE;

        if (!allopt[optidx].addr)
            return optn_ok; /* silent retreat */

        /* option that must come from config file? */
        if (!g.opt_initial && (allopt[optidx].setwhere == set_in_config))
            return optn_err;

        op = string_for_opt(opts, TRUE);
        if (op != empty_optstr) {
            int ln;

            if (negated) {
                config_error_add(
                           "Negated boolean '%s' should not have a parameter",
                                 allopt[optidx].name);
                return optn_silenterr;
            }
            /* length is greater than 0 or we wouldn't have gotten here */
            ln = (int) strlen(op);
            if (!strncmpi(op, "true", ln)
                || !strncmpi(op, "yes", ln)
                || !strcmpi(op, "on")
                || (digit(*op) && atoi(op) == 1)) {
                negated = FALSE;
            } else if (!strncmpi(op, "false", ln)
                       || !strncmpi(op, "no", ln)
                       || !strcmpi(op, "off")
                       || (digit(*op) && atoi(op) == 0)) {
                negated = TRUE;
            } else if (!allopt[optidx].valok) {
                config_error_add("'%s' is not valid for a boolean", opts);
                return optn_silenterr;
            }
        }
        if (iflags.debug_fuzzer && !g.opt_initial) {
            /* don't randomly toggle this/these */
            if ((optidx == opt_silent)
                || (optidx == opt_perm_invent))
                return optn_ok;
        }
        /* Before the change */
        switch (optidx) {
        case opt_female:
            if (!strncmpi(opts, "female", 3)) {
                if (!g.opt_initial && flags.female == negated) {
                    nosexchange = TRUE;
                } else {
                    flags.initgend = flags.female = !negated;
                    return optn_ok;
                }
            }
            if (!strncmpi(opts, "male", 3)) {
                if (!g.opt_initial && flags.female != negated) {
                    nosexchange = TRUE;
                } else {
                    flags.initgend = flags.female = negated;
                    return optn_ok;
                }
            }
            break;
        case opt_perm_invent:
#ifdef TTY_PERM_INVENT
            /* if attempting to enable perm_invent fails, say so and return
               before "'perm_invent' option toggled on" would be given below;
               perm_invent_toggled() and routines it calls don't check
               iflags.perm_invent so it doesn't matter that 'SET IT HERE'
               hasn't been executed yet */
            if (WINDOWPORT(tty) && !g.opt_initial && !negated) {
                perm_invent_toggled(FALSE);
                /* perm_invent_toggled()
                   -> sync_perminvent()
                          -> tty_create_nhwindow(NHW_PERMINVENT)
                   gives feedback for failure (terminal too small) */
                if (WIN_INVEN == WIN_ERR)
                    return optn_silenterr;
            }
#endif
            break; /* from opt_perm_invent */
        default:
            break;
        }
        /* this dates from when 'O' prompted for a line of options text
           rather than use a menu to control access to which options can
           be modified during play; it was possible to attempt to use
           'O' to specify female or negate male when playing as male or
           to specify male or negate female when playing as female;
           options processing rejects that for !opt_initial; not possible
           now but kept in case someone brings the old 'O' behavior back */
        if (nosexchange) {
            /* can't arbitrarily change sex after game has started;
               magic (amulet or polymorph) is required for that */
            config_error_add("'%s' is not anatomically possible.", opts);
            return optn_silenterr;
        }

        *(allopt[optidx].addr) = !negated;    /* <==== SET IT HERE */

        /* After the change */
        switch (optidx) {
        case opt_ascii_map:
            iflags.wc_tiled_map = negated;
            break;
        case opt_tiled_map:
            iflags.wc_ascii_map = negated;
            break;
        default:
            break;
        }

        /* only do processing below if setting with doset() */

        if (g.opt_initial)
            return optn_ok;

        switch (optidx) {
        case opt_time:
#ifdef SCORE_ON_BOTL
        case opt_showscore:
#endif
        case opt_showexp:
            if (VIA_WINDOWPORT())
                status_initialize(REASSESS_ONLY);
            g.context.botl = TRUE;
            break;
        case opt_fixinv:
        case opt_sortpack:
        case opt_implicit_uncursed:
            if (!flags.invlet_constant)
                reassign();
            update_inventory();
            break;
        case opt_lit_corridor:
        case opt_dark_room:
            /*
             * All corridor squares seen via night vision or
             * candles & lamps change.  Update them by calling
             * newsym() on them.  Don't do this if we are
             * initializing the options --- the vision system
             * isn't set up yet.
             */
            vision_recalc(2);         /* shut down vision */
            g.vision_full_recalc = 1; /* delayed recalc */
            if (iflags.use_color)
                g.opt_need_redraw = TRUE; /* darkroom refresh */
            break;
        case opt_wizmgender:
        case opt_showrace:
        case opt_use_inverse:
        case opt_hilite_pile:
        case opt_perm_invent:
        case opt_ascii_map:
        case opt_tiled_map:
            g.opt_need_redraw = TRUE;
            g.opt_need_glyph_reset = TRUE;
            break;
        case opt_hilite_pet:
#ifdef CURSES_GRAPHICS
            if (WINDOWPORT(curses)) {
                /* if we're enabling hilite_pet and petattr isn't set,
                   set it to Inverse; if we're disabling, leave petattr
                   alone so that re-enabling will get current value back
                 */
                if (iflags.hilite_pet && !iflags.wc2_petattr)
                    iflags.wc2_petattr = curses_read_attrs("I");
            }
#endif
            g.opt_need_redraw = TRUE;
            break;
        case opt_hitpointbar:
            if (VIA_WINDOWPORT()) {
                /* [is reassessment really needed here?] */
                status_initialize(REASSESS_ONLY);
                g.opt_need_redraw = TRUE;
#ifdef QT_GRAPHICS
            } else if (WINDOWPORT(Qt)) {
                /* Qt doesn't support HILITE_STATUS or FLUSH_STATUS so fails
                   VIA_WINDOWPORT(), but it does support WC2_HITPOINTBAR */
                g.context.botlx = TRUE;
#endif
            }
            break;
        case opt_color:
#ifdef TOS
            if (iflags.BIOS) {
                if (colors_changed)
                    restore_colors();
                else
                    set_colors();
            }
#endif
            g.opt_need_redraw = TRUE;
            g.opt_need_glyph_reset = TRUE;
            break;
        case opt_menucolors:
        case opt_guicolor:
            update_inventory();
            break;
        case opt_mention_decor:
            iflags.prev_decor = STONE;
            break;
        case opt_rest_on_space:
            update_rest_on_space();
            break;
        default:
            break;
        }

        /* boolean value has been toggled but some option changes can
           still be pending at this point (mainly for opt_need_redraw);
           give the toggled message now regardless */
        pline("'%s' option toggled %s.", allopt[optidx].name,
              !negated ? "on" : "off");

        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        opts[0] = '\0';
        return optn_ok;
    }
    return optn_ok;
}

static int
spcfn_misc_menu_cmd(int midx, int req, boolean negated, char *opts, char *op)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
        if (negated) {
            bad_negation(default_menu_cmd_info[midx].name, FALSE);
            return optn_err;
        } else if ((op = string_for_opt(opts, FALSE)) != empty_optstr) {
            char c = txt2key(op);

            if (illegal_menu_cmd_key((uchar) c))
                return optn_err;
            add_menu_cmd_alias(c, default_menu_cmd_info[midx].cmd);
        }
        return optn_ok;
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        opts[0] = '\0';
        return optn_ok;
    }
    return optn_ok;
}


/*
 **********************************
 *
 *   Special per-option handlers
 *
 **********************************
 */

static int
handler_menustyle(void)
{
    winid tmpwin;
    anything any;
    boolean chngd;
    int i, n, old_menu_style = flags.menu_style;
    char buf[BUFSZ], sep = iflags.menu_tab_sep ? '\t' : ' ';
    menu_item *style_pick = (menu_item *) 0;
    int clr = 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);
    any = cg.zeroany;
    for (i = 0; i < SIZE(menutype); i++) {
        Sprintf(buf, "%-12.12s%c%.60s", menutype[i][0], sep, menutype[i][1]);
        any.a_int = i + 1;
        add_menu(tmpwin, &nul_glyphinfo, &any, *buf, 0, ATR_NONE, clr, buf,
                 (i == flags.menu_style) ? MENU_ITEMFLAGS_SELECTED
                                         : MENU_ITEMFLAGS_NONE);
        /* second line is prefixed by spaces that "c - " would use */
        Sprintf(buf, "%4s%-12.12s%c%.60s", "", "", sep, menutype[i][2]);
        any.a_int = 0;
        add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE, clr, buf,
                 MENU_ITEMFLAGS_NONE);
    }
    end_menu(tmpwin, "Select menustyle:");
    n = select_menu(tmpwin, PICK_ONE, &style_pick);
    if (n > 0) {
        i = style_pick[0].item.a_int - 1;
        /* if there are two picks, use the one that wasn't pre-selected */
        if (n > 1 && i == old_menu_style)
            i = style_pick[1].item.a_int - 1;
        flags.menu_style = i;
        free((genericptr_t) style_pick);
    }
    destroy_nhwindow(tmpwin);
    chngd = (flags.menu_style != old_menu_style);
    if (chngd || Verbose(2, handler_menustyle))
        pline("'menustyle' %s \"%s\".", chngd ? "changed to" : "is still",
              menutype[(int) flags.menu_style][0]);
    return optn_ok;
}

static int
handler_align_misc(int optidx)
{
    winid tmpwin;
    anything any;
    menu_item *window_pick = (menu_item *) 0;
    char abuf[BUFSZ];
    int clr = 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);
    any = cg.zeroany;
    any.a_int = ALIGN_TOP;
    add_menu(tmpwin, &nul_glyphinfo, &any, 't', 0, ATR_NONE, clr, "top",
             MENU_ITEMFLAGS_NONE);
    any.a_int = ALIGN_BOTTOM;
    add_menu(tmpwin, &nul_glyphinfo, &any, 'b', 0, ATR_NONE, clr, "bottom",
             MENU_ITEMFLAGS_NONE);
    any.a_int = ALIGN_LEFT;
    add_menu(tmpwin, &nul_glyphinfo, &any, 'l', 0, ATR_NONE, clr, "left",
             MENU_ITEMFLAGS_NONE);
    any.a_int = ALIGN_RIGHT;
    add_menu(tmpwin, &nul_glyphinfo, &any, 'r', 0, ATR_NONE, clr, "right",
             MENU_ITEMFLAGS_NONE);
    Sprintf(abuf, "Select %s window placement relative to the map:",
            (optidx == opt_align_message) ? "message" : "status");
    end_menu(tmpwin, abuf);
    if (select_menu(tmpwin, PICK_ONE, &window_pick) > 0) {
        if (optidx == opt_align_message)
            iflags.wc_align_message = window_pick->item.a_int;
        else
            iflags.wc_align_status = window_pick->item.a_int;
        free((genericptr_t) window_pick);
    }
    destroy_nhwindow(tmpwin);
    return optn_ok;
}

static int
handler_autounlock(int optidx)
{
    winid tmpwin;
    anything any;
    boolean chngd;
    unsigned oldflags = flags.autounlock;
    const char *optname = allopt[optidx].name;
    char buf[BUFSZ], sep = iflags.menu_tab_sep ? '\t' : ' ';
    menu_item *window_pick = (menu_item *) 0;
    int i, n, presel, res = optn_ok;
    int clr = 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);
    any = cg.zeroany;
    for (i = 0; i < SIZE(unlocktypes); ++i) {
        Sprintf(buf, "%-10.10s%c%.40s",
                unlocktypes[i][0], sep, unlocktypes[i][1]);
        presel = !i ? !flags.autounlock : (flags.autounlock & (1 << (i - 1)));
        any.a_int = i + 1;
        add_menu(tmpwin, &nul_glyphinfo, &any, *unlocktypes[i][0], 0,
                 ATR_NONE, clr, buf,
                 ((presel ? MENU_ITEMFLAGS_SELECTED : MENU_ITEMFLAGS_NONE)
                  | (!i ? MENU_ITEMFLAGS_SKIPINVERT : 0)));
    }
    Sprintf(buf, "Select '%.20s' actions:", optname);
    end_menu(tmpwin, buf);
    n = select_menu(tmpwin, PICK_ANY, &window_pick);
    if (n > 0) {
        int k;
        boolean wasnone = !flags.autounlock;
        unsigned newflags = 0, noflags = 0;

        for (i = 0; i < n; ++i) {
            k = window_pick[i].item.a_int - 1;
            if (k)
                newflags |= (1 << (k - 1));
            else
                noflags = 1;
        }
        /* wasnone: 'none' is preselected;
           !wasnone: don't force it to be unselected */
        if (newflags && noflags && !wasnone) {
            config_error_add(
                     "Invalid value combination for \"%s\": 'none' with some",
                             optname);
            res = optn_silenterr;
        } else {
            flags.autounlock = newflags;
        }
        free((genericptr_t) window_pick);
    } else if (n == 0) { /* nothing was picked but menu wasn't cancelled */
        /* something that was preselected got unselected, leaving nothing;
           treat that as picking 'none' (even though 'none' might be what
           got unselected) */
        flags.autounlock = 0;
    }
    destroy_nhwindow(tmpwin);
    chngd = (flags.autounlock != oldflags);
    if (chngd || Verbose(2, handler_autounlock)) {
        optfn_autounlock(optidx, get_val, FALSE, buf, (char *) NULL);
        pline("'%s' %s '%s'.", optname,
              chngd ? "changed to" : "is still", buf);
    }
    return res;
}

static int
handler_disclose(void)
{
    winid tmpwin;
    anything any;
    int i, n;
    char buf[BUFSZ];
    /* order of disclose_names[] must correspond to
       disclosure_options in decl.c */
    static const char *const disclosure_names[] = {
        "inventory", "attributes", "vanquished",
        "genocides", "conduct",    "overview",
    };
    int disc_cat[NUM_DISCLOSURE_OPTIONS];
    int pick_cnt, pick_idx, opt_idx;
    char c;
    menu_item *disclosure_pick = (menu_item *) 0;
    int clr = 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);
    any = cg.zeroany;
    for (i = 0; i < NUM_DISCLOSURE_OPTIONS; i++) {
        Sprintf(buf, "%-12s[%c%c]", disclosure_names[i],
                flags.end_disclose[i], disclosure_options[i]);
        any.a_int = i + 1;
        add_menu(tmpwin, &nul_glyphinfo, &any, disclosure_options[i],
                 0, ATR_NONE, clr, buf, MENU_ITEMFLAGS_NONE);
        disc_cat[i] = 0;
    }
    end_menu(tmpwin, "Change which disclosure options categories:");
    pick_cnt = select_menu(tmpwin, PICK_ANY, &disclosure_pick);
    if (pick_cnt > 0) {
        for (pick_idx = 0; pick_idx < pick_cnt; ++pick_idx) {
            opt_idx = disclosure_pick[pick_idx].item.a_int - 1;
            disc_cat[opt_idx] = 1;
        }
        free((genericptr_t) disclosure_pick);
        disclosure_pick = (menu_item *) 0;
    }
    destroy_nhwindow(tmpwin);

    for (i = 0; i < NUM_DISCLOSURE_OPTIONS; i++) {
        if (disc_cat[i]) {
            c = flags.end_disclose[i];
            Sprintf(buf, "Disclosure options for %s:",
                    disclosure_names[i]);
            tmpwin = create_nhwindow(NHW_MENU);
            start_menu(tmpwin, MENU_BEHAVE_STANDARD);
            any = cg.zeroany;
            /* 'y','n',and '+' work as alternate selectors; '-' doesn't */
            any.a_char = DISCLOSE_NO_WITHOUT_PROMPT;
            add_menu(tmpwin, &nul_glyphinfo, &any, 0,
                     any.a_char, ATR_NONE, clr,
                     "Never disclose, without prompting",
                     (c == any.a_char) ? MENU_ITEMFLAGS_SELECTED
                                       : MENU_ITEMFLAGS_NONE);
            any.a_char = DISCLOSE_YES_WITHOUT_PROMPT;
            add_menu(tmpwin, &nul_glyphinfo, &any, 0,
                     any.a_char, ATR_NONE, clr,
                     "Always disclose, without prompting",
                     (c == any.a_char) ? MENU_ITEMFLAGS_SELECTED
                                       : MENU_ITEMFLAGS_NONE);
            if (*disclosure_names[i] == 'v') {
                any.a_char = DISCLOSE_SPECIAL_WITHOUT_PROMPT; /* '#' */
                add_menu(tmpwin, &nul_glyphinfo, &any, 0,
                         any.a_char, ATR_NONE, clr,
                         "Always disclose, pick sort order from menu",
                         (c == any.a_char) ? MENU_ITEMFLAGS_SELECTED
                                           : MENU_ITEMFLAGS_NONE);
            }
            any.a_char = DISCLOSE_PROMPT_DEFAULT_NO;
            add_menu(tmpwin, &nul_glyphinfo, &any, 0,
                     any.a_char, ATR_NONE, clr,
                     "Prompt, with default answer of \"No\"",
                     (c == any.a_char) ? MENU_ITEMFLAGS_SELECTED
                                       : MENU_ITEMFLAGS_NONE);
            any.a_char = DISCLOSE_PROMPT_DEFAULT_YES;
            add_menu(tmpwin, &nul_glyphinfo, &any, 0,
                     any.a_char, ATR_NONE, clr,
                     "Prompt, with default answer of \"Yes\"",
                     (c == any.a_char) ? MENU_ITEMFLAGS_SELECTED
                                       : MENU_ITEMFLAGS_NONE);
            if (*disclosure_names[i] == 'v') {
                any.a_char = DISCLOSE_PROMPT_DEFAULT_SPECIAL; /* '?' */
                add_menu(tmpwin, &nul_glyphinfo, &any, 0,
                         any.a_char, ATR_NONE, clr,
            "Prompt, with default answer of \"Ask\" to request sort menu",
                         (c == any.a_char) ? MENU_ITEMFLAGS_SELECTED
                                           : MENU_ITEMFLAGS_NONE);
            }
            end_menu(tmpwin, buf);
            n = select_menu(tmpwin, PICK_ONE, &disclosure_pick);
            if (n > 0) {
                flags.end_disclose[i] = disclosure_pick[0].item.a_char;
                if (n > 1 && flags.end_disclose[i] == c)
                    flags.end_disclose[i] = disclosure_pick[1].item.a_char;
                free((genericptr_t) disclosure_pick);
            }
            destroy_nhwindow(tmpwin);
        }
    }
    return optn_ok;
}

static int
handler_menu_headings(void)
{
    int mhattr = query_attr("How to highlight menu headings:");

    if (mhattr != -1) {
        iflags.menu_headings = mhattr;
        /* header highlighting affects persistent inventory display */
        if (iflags.perm_invent)
            update_inventory();
    }
    return optn_ok;
}

static int
handler_msg_window(void)
{
#if PREV_MSGS /* tty or curses */
    winid tmpwin;
    anything any;
    boolean is_tty = WINDOWPORT(tty), is_curses = WINDOWPORT(curses);
    int clr = 0;

    if (is_tty || is_curses) {
        /* by Christian W. Cooper */
        boolean chngd;
        int i, n;
        char buf[BUFSZ], c,
             sep = iflags.menu_tab_sep ? '\t' : ' ',
             old_prevmsg_window = iflags.prevmsg_window;
        menu_item *window_pick = (menu_item *) 0;

        tmpwin = create_nhwindow(NHW_MENU);
        start_menu(tmpwin, MENU_BEHAVE_STANDARD);
        any = cg.zeroany;

        for (i = 0; i < SIZE(menutype); i++) {
            if (i < 2 && is_curses)
                continue;
            Sprintf(buf, "%-12.12s%c%.60s", msgwind[i][0], sep, msgwind[i][1]);
            any.a_char = c = *msgwind[i][0];
            add_menu(tmpwin, &nul_glyphinfo, &any, *buf, 0, ATR_NONE, clr, buf,
                     (c == iflags.prevmsg_window) ? MENU_ITEMFLAGS_SELECTED
                                                  : MENU_ITEMFLAGS_NONE);
            /* second line is prefixed by spaces that "c - " would use */
            Sprintf(buf, "%4s%-12.12s%c%.60s", "", "", sep, msgwind[i][2]);
            any.a_char = '\0';
            add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE, clr, buf,
                     MENU_ITEMFLAGS_NONE);
        }
        end_menu(tmpwin, "Select message history display type:");
        n = select_menu(tmpwin, PICK_ONE, &window_pick);
        if (n > 0) {
            c = window_pick[0].item.a_char;
            /* if there are two picks, use the one that wasn't pre-selected */
            if (n > 1 && c == old_prevmsg_window)
                c = window_pick[1].item.a_char;
            iflags.prevmsg_window = c;
            free((genericptr_t) window_pick);
        }
        destroy_nhwindow(tmpwin);
        chngd = (iflags.prevmsg_window != old_prevmsg_window);
        if (chngd || Verbose(2, handler_msg_window)) {
            (void) optfn_msg_window(opt_msg_window, get_val,
                                    FALSE, buf, empty_optstr);
            pline("'msg_window' %.20s \"%.20s\".",
                  chngd ? "changed to" : "is still", buf);
        }
    } else
#endif /* PREV_MSGS (for tty or curses) */
        pline("'%s' option is not supported for '%s'.",
              allopt[opt_msg_window].name, windowprocs.name);
    return optn_ok;
}

static int
handler_number_pad(void)
{
    winid tmpwin;
    anything any;
    int i;
    static const char *const npchoices[] = {
        " 0 (off)", " 1 (on)", " 2 (on, MSDOS compatible)",
        " 3 (on, phone-style digit layout)",
        " 4 (on, phone-style layout, MSDOS compatible)",
        "-1 (off, 'z' to move upper-left, 'y' to zap wands)"
    };
    menu_item *mode_pick = (menu_item *) 0;
    int clr = 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);
    any = cg.zeroany;
    for (i = 0; i < SIZE(npchoices); i++) {
        any.a_int = i + 1;
        add_menu(tmpwin, &nul_glyphinfo, &any, 'a' + i, '0' + i,
                 ATR_NONE, clr, npchoices[i], MENU_ITEMFLAGS_NONE);
    }
    end_menu(tmpwin, "Select number_pad mode:");
    if (select_menu(tmpwin, PICK_ONE, &mode_pick) > 0) {
        switch (mode_pick->item.a_int - 1) {
        case 0:
            iflags.num_pad = FALSE;
            iflags.num_pad_mode = 0;
            break;
        case 1:
            iflags.num_pad = TRUE;
            iflags.num_pad_mode = 0;
            break;
        case 2:
            iflags.num_pad = TRUE;
            iflags.num_pad_mode = 1;
            break;
        case 3:
            iflags.num_pad = TRUE;
            iflags.num_pad_mode = 2;
            break;
        case 4:
            iflags.num_pad = TRUE;
            iflags.num_pad_mode = 3;
            break;
        /* last menu choice: number_pad == -1 */
        case 5:
            iflags.num_pad = FALSE;
            iflags.num_pad_mode = 1;
            break;
        }
        reset_commands(FALSE);
        number_pad(iflags.num_pad ? 1 : 0);
        free((genericptr_t) mode_pick);
    }
    destroy_nhwindow(tmpwin);
    return optn_ok;
}

static int
handler_paranoid_confirmation(void)
{
    winid tmpwin;
    anything any;
    int i;
    menu_item *paranoia_picks = (menu_item *) 0;
    int clr = 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);
    any = cg.zeroany;
    for (i = 0; paranoia[i].flagmask != 0; ++i) {
        if (paranoia[i].flagmask == PARANOID_BONES && !wizard)
            continue;
        any.a_int = paranoia[i].flagmask;
        add_menu(tmpwin, &nul_glyphinfo, &any, *paranoia[i].argname,
                 0, ATR_NONE, clr, paranoia[i].explain,
                 (flags.paranoia_bits & paranoia[i].flagmask)
                     ? MENU_ITEMFLAGS_SELECTED
                     : MENU_ITEMFLAGS_NONE);
    }
    end_menu(tmpwin, "Actions requiring extra confirmation:");
    i = select_menu(tmpwin, PICK_ANY, &paranoia_picks);
    if (i >= 0) {
        /* player didn't cancel; we reset all the paranoia options
           here even if there were no items picked, since user
           could have toggled off preselected ones to end up with 0 */
        flags.paranoia_bits = 0;
        if (i > 0) {
            /* at least 1 item set, either preselected or newly picked */
            while (--i >= 0)
                flags.paranoia_bits |= paranoia_picks[i].item.a_int;
            free((genericptr_t) paranoia_picks);
        }
    }
    destroy_nhwindow(tmpwin);
    return optn_ok;
}

static int
handler_pickup_burden(void)
{
    winid tmpwin;
    anything any;
    int i;
    const char *burden_name, *burden_letters = "ubsntl";
    menu_item *burden_pick = (menu_item *) 0;
    int clr = 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);
    any = cg.zeroany;
    for (i = 0; i < SIZE(burdentype); i++) {
        burden_name = burdentype[i];
        any.a_int = i + 1;
        add_menu(tmpwin, &nul_glyphinfo, &any, burden_letters[i],
                 0, ATR_NONE, clr, burden_name, MENU_ITEMFLAGS_NONE);
    }
    end_menu(tmpwin, "Select encumbrance level:");
    if (select_menu(tmpwin, PICK_ONE, &burden_pick) > 0) {
        flags.pickup_burden = burden_pick->item.a_int - 1;
        free((genericptr_t) burden_pick);
    }
    destroy_nhwindow(tmpwin);
    return optn_ok;
}

static int
handler_pickup_types(void)
{
    char buf[BUFSZ];

    /* parseoptions will prompt for the list of types */
    (void) parseoptions(strcpy(buf, "pickup_types"), FALSE, FALSE);
    return optn_ok;
}

static int
handler_runmode(void)
{
    winid tmpwin;
    anything any;
    int i;
    const char *mode_name;
    menu_item *mode_pick = (menu_item *) 0;
    int clr = 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);
    any = cg.zeroany;
    for (i = 0; i < SIZE(runmodes); i++) {
        mode_name = runmodes[i];
        any.a_int = i + 1;
        add_menu(tmpwin, &nul_glyphinfo, &any, *mode_name,
                 0, ATR_NONE, clr, mode_name, MENU_ITEMFLAGS_NONE);
    }
    end_menu(tmpwin, "Select run/travel display mode:");
    if (select_menu(tmpwin, PICK_ONE, &mode_pick) > 0) {
        flags.runmode = mode_pick->item.a_int - 1;
        free((genericptr_t) mode_pick);
    }
    destroy_nhwindow(tmpwin);
    return optn_ok;
}

static int
handler_sortloot(void)
{
    winid tmpwin;
    anything any;
    int i, n;
    const char *sortl_name;
    menu_item *sortl_pick = (menu_item *) 0;
    int clr = 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);
    any = cg.zeroany;
    for (i = 0; i < SIZE(sortltype); i++) {
        sortl_name = sortltype[i];
        any.a_char = *sortl_name;
        add_menu(tmpwin, &nul_glyphinfo, &any, *sortl_name,
                 0, ATR_NONE, clr,
                 sortl_name, (flags.sortloot == *sortl_name)
                                ? MENU_ITEMFLAGS_SELECTED
                                : MENU_ITEMFLAGS_NONE);
    }
    end_menu(tmpwin, "Select loot sorting type:");
    n = select_menu(tmpwin, PICK_ONE, &sortl_pick);
    if (n > 0) {
        char c = sortl_pick[0].item.a_char;

        if (n > 1 && c == flags.sortloot)
            c = sortl_pick[1].item.a_char;
        flags.sortloot = c;
        /* changing to or from 'f' affects persistent inventory display */
        if (iflags.perm_invent)
            update_inventory();
        free((genericptr_t) sortl_pick);
    }
    destroy_nhwindow(tmpwin);
    return optn_ok;
}

static int
handler_whatis_coord(void)
{
    winid tmpwin;
    anything any;
    char buf[BUFSZ];
    menu_item *window_pick = (menu_item *) 0;
    int pick_cnt;
    char gp = iflags.getpos_coords;
    int clr = 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);
    any = cg.zeroany;
    any.a_char = GPCOORDS_COMPASS;
    add_menu(tmpwin, &nul_glyphinfo, &any, GPCOORDS_COMPASS,
             0, ATR_NONE, clr,
             "compass ('east' or '3s' or '2n,4w')",
             (gp == GPCOORDS_COMPASS)
                ? MENU_ITEMFLAGS_SELECTED : MENU_ITEMFLAGS_NONE);
    any.a_char = GPCOORDS_COMFULL;
    add_menu(tmpwin, &nul_glyphinfo, &any, GPCOORDS_COMFULL,
             0, ATR_NONE, clr,
             "full compass ('east' or '3south' or '2north,4west')",
             (gp == GPCOORDS_COMFULL)
                ? MENU_ITEMFLAGS_SELECTED : MENU_ITEMFLAGS_NONE);
    any.a_char = GPCOORDS_MAP;
    add_menu(tmpwin, &nul_glyphinfo, &any, GPCOORDS_MAP,
             0, ATR_NONE, clr, "map <x,y>",
             (gp == GPCOORDS_MAP)
                ? MENU_ITEMFLAGS_SELECTED : MENU_ITEMFLAGS_NONE);
    any.a_char = GPCOORDS_SCREEN;
    add_menu(tmpwin, &nul_glyphinfo, &any, GPCOORDS_SCREEN,
             0, ATR_NONE, clr, "screen [row,column]",
             (gp == GPCOORDS_SCREEN)
                ? MENU_ITEMFLAGS_SELECTED : MENU_ITEMFLAGS_NONE);
    any.a_char = GPCOORDS_NONE;
    add_menu(tmpwin, &nul_glyphinfo, &any, GPCOORDS_NONE,
             0, ATR_NONE, clr, "none (no coordinates displayed)",
             (gp == GPCOORDS_NONE)
                ? MENU_ITEMFLAGS_SELECTED : MENU_ITEMFLAGS_NONE);
    any.a_long = 0L;
    add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE, clr,
             "", MENU_ITEMFLAGS_NONE);
    Sprintf(buf, "map: upper-left: <%d,%d>, lower-right: <%d,%d>%s",
            1, 0, COLNO - 1, ROWNO - 1,
            Verbose(2, handler_whatis_coord1)
                ? "; column 0 unused, off left edge" : "");
    add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0,
             ATR_NONE, clr, buf, MENU_ITEMFLAGS_NONE);
    if (strcmp(windowprocs.name, "tty")) /* only show for non-tty */
        add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE, clr,
   "screen: row is offset to accommodate tty interface's use of top line",
                 MENU_ITEMFLAGS_NONE);
#if COLNO == 80
#define COL80ARG Verbose(2, handler_whatis_coord2) ? "; column 80 is not used" : ""
#else
#define COL80ARG ""
#endif
    Sprintf(buf, "screen: upper-left: [%02d,%02d], lower-right: [%d,%d]%s",
            0 + 2, 1, ROWNO - 1 + 2, COLNO - 1, COL80ARG);
#undef COL80ARG
    add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE, clr,
             buf, MENU_ITEMFLAGS_NONE);
    add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE, clr,
             "", MENU_ITEMFLAGS_NONE);
    end_menu(tmpwin,
        "Select coordinate display when auto-describing a map position:");
    if ((pick_cnt = select_menu(tmpwin, PICK_ONE, &window_pick)) > 0) {
        iflags.getpos_coords = window_pick[0].item.a_char;
        /* PICK_ONE doesn't unselect preselected entry when
           selecting another one */
        if (pick_cnt > 1 && iflags.getpos_coords == gp)
            iflags.getpos_coords = window_pick[1].item.a_char;
        free((genericptr_t) window_pick);
    }
    destroy_nhwindow(tmpwin);
    return optn_ok;
}

static int
handler_whatis_filter(void)
{
    winid tmpwin;
    anything any;
    menu_item *window_pick = (menu_item *) 0;
    int pick_cnt;
    char gf = iflags.getloc_filter;
    int clr = 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);
    any = cg.zeroany;
    any.a_char = (GFILTER_NONE + 1);
    add_menu(tmpwin, &nul_glyphinfo, &any, 'n',
             0, ATR_NONE, clr, "no filtering",
             (gf == GFILTER_NONE)
                ? MENU_ITEMFLAGS_SELECTED : MENU_ITEMFLAGS_NONE);
    any.a_char = (GFILTER_VIEW + 1);
    add_menu(tmpwin, &nul_glyphinfo, &any, 'v',
             0, ATR_NONE, clr, "in view only",
             (gf == GFILTER_VIEW)
                ? MENU_ITEMFLAGS_SELECTED : MENU_ITEMFLAGS_NONE);
    any.a_char = (GFILTER_AREA + 1);
    add_menu(tmpwin, &nul_glyphinfo, &any, 'a',
             0, ATR_NONE, clr, "in same area",
             (gf == GFILTER_AREA)
                ? MENU_ITEMFLAGS_SELECTED : MENU_ITEMFLAGS_NONE);
    end_menu(tmpwin,
      "Select location filtering when going for next/previous map position:");
    if ((pick_cnt = select_menu(tmpwin, PICK_ONE, &window_pick)) > 0) {
        iflags.getloc_filter = (window_pick[0].item.a_char - 1);
        /* PICK_ONE doesn't unselect preselected entry when
           selecting another one */
        if (pick_cnt > 1 && iflags.getloc_filter == gf)
            iflags.getloc_filter = (window_pick[1].item.a_char - 1);
        free((genericptr_t) window_pick);
    }
    destroy_nhwindow(tmpwin);
    return optn_ok;
}

static int
handler_symset(int optidx)
{
    int reslt;

    reslt = do_symset(optidx == opt_roguesymset);   /* symbols.c */
    g.opt_need_redraw = TRUE;
    return reslt;
}

static int
handler_autopickup_exception(void)
{
    winid tmpwin;
    anything any;
    int i;
    int opt_idx, numapes = 0;
    char apebuf[2 + BUFSZ]; /* so &apebuf[1] is BUFSZ long for getlin() */
    struct autopickup_exception *ape;
    int clr = 0;

 ape_again:
    numapes = count_apes();
    opt_idx = handle_add_list_remove("autopickup exception", numapes);
    if (opt_idx == 3) { /* done */
        return TRUE;
    } else if (opt_idx == 0) { /* add new */
        /* EDIT_GETLIN:  assume user doesn't user want previous
           exception used as default input string for this one... */
        apebuf[0] = apebuf[1] = '\0';
        getlin("What new autopickup exception pattern?", &apebuf[1]);
        mungspaces(&apebuf[1]); /* regularize whitespace */
        if (apebuf[1] == '\033')
            return TRUE;
        if (apebuf[1]) {
            apebuf[0] = '\"';
            /* guarantee room for \" prefix and \"\0 suffix;
               -2 is good enough for apebuf[] but -3 makes
               sure the whole thing fits within normal BUFSZ */
            apebuf[sizeof apebuf - 2] = '\0';
            Strcat(apebuf, "\"");
            add_autopickup_exception(apebuf);
        }
        goto ape_again;
    } else { /* list (1) or remove (2) */
        int pick_idx, pick_cnt;
        menu_item *pick_list = (menu_item *) 0;

        tmpwin = create_nhwindow(NHW_MENU);
        start_menu(tmpwin, MENU_BEHAVE_STANDARD);
        if (numapes) {
            ape = g.apelist;
            any = cg.zeroany;
            add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0,
                     iflags.menu_headings, clr,
                     "Always pickup '<'; never pickup '>'",
                     MENU_ITEMFLAGS_NONE);
            for (i = 0; i < numapes && ape; i++) {
                any.a_void = (opt_idx == 1) ? 0 : ape;
                /* length of pattern plus quotes (plus '<'/'>') is
                   less than BUFSZ */
                Sprintf(apebuf, "\"%c%s\"", ape->grab ? '<' : '>',
                        ape->pattern);
                add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0,
                         ATR_NONE, clr, apebuf, MENU_ITEMFLAGS_NONE);
                ape = ape->next;
            }
        }
        Sprintf(apebuf, "%s autopickup exceptions",
                (opt_idx == 1) ? "List of" : "Remove which");
        end_menu(tmpwin, apebuf);
        pick_cnt = select_menu(tmpwin,
                               (opt_idx == 1) ? PICK_NONE : PICK_ANY,
                               &pick_list);
        if (pick_cnt > 0) {
            for (pick_idx = 0; pick_idx < pick_cnt; ++pick_idx)
                remove_autopickup_exception(
                                     (struct autopickup_exception *)
                                         pick_list[pick_idx].item.a_void);
            free((genericptr_t) pick_list), pick_list = (menu_item *) 0;
        }
        destroy_nhwindow(tmpwin);
        if (pick_cnt >= 0)
            goto ape_again;
    }
    return optn_ok;
}

static int
handler_menu_colors(void)
{
    winid tmpwin;
    anything any;
    char buf[BUFSZ];
    int opt_idx, nmc, mcclr, mcattr;
    char mcbuf[BUFSZ];
    int clr = 0;

 menucolors_again:
    nmc = count_menucolors();
    opt_idx = handle_add_list_remove("menucolor", nmc);
    if (opt_idx == 3) { /* done */
 menucolors_done:
        /* in case we've made a change which impacts current persistent
           inventory window; we don't track whether an actual changed
           occurred, so just assume there was one and that it matters;
           if we're wrong, a redundant update is cheap... */
        if (iflags.use_menu_color) {
            if (iflags.perm_invent)
                update_inventory();

        /* menu colors aren't being used yet; if any MENUCOLOR rules are
           defined, remind player how to activate them */
        } else if (nmc > 0) {
            pline(
    "To have menu colors become active, toggle 'menucolors' option to True.");
        }
        return optn_ok;

    } else if (opt_idx == 0) { /* add new */
        mcbuf[0] = '\0';
        getlin("What new menucolor pattern?", mcbuf);
        if (*mcbuf == '\033')
            goto menucolors_done;
        if (*mcbuf
            && test_regex_pattern(mcbuf, "MENUCOLORS regex")
            && (mcclr = query_color((char *) 0)) != -1
            && (mcattr = query_attr((char *) 0)) != -1
            && !add_menu_coloring_parsed(mcbuf, mcclr, mcattr)) {
            pline("Error adding the menu color.");
            wait_synch();
        }
        goto menucolors_again;

    } else { /* list (1) or remove (2) */
        int pick_idx, pick_cnt;
        int mc_idx;
        unsigned ln;
        const char *sattr, *sclr;
        menu_item *pick_list = (menu_item *) 0;
        struct menucoloring *tmp = g.menu_colorings;
        char clrbuf[QBUFSZ];

        tmpwin = create_nhwindow(NHW_MENU);
        start_menu(tmpwin, MENU_BEHAVE_STANDARD);
        any = cg.zeroany;
        mc_idx = 0;
        while (tmp) {
            sattr = attr2attrname(tmp->attr);
            sclr = strcpy(clrbuf, clr2colorname(tmp->color));
            (void) strNsubst(clrbuf, " ", "-", 0);
            any.a_int = ++mc_idx;
            /* construct suffix */
            Sprintf(buf, "\"\"=%s%s%s", sclr,
                    (tmp->attr != ATR_NONE) ? "&" : "",
                    (tmp->attr != ATR_NONE) ? sattr : "");
            /* now main string */
            ln = sizeof buf - Strlen(buf) - 1; /* length available */
            Strcpy(mcbuf, "\"");
            if (strlen(tmp->origstr) > ln)
                Strcat(strncat(mcbuf, tmp->origstr, ln - 3), "...");
            else
                Strcat(mcbuf, tmp->origstr);
            /* combine main string and suffix */
            Strcat(mcbuf, &buf[1]); /* skip buf[]'s initial quote */
            add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0,
                     ATR_NONE, clr, mcbuf, MENU_ITEMFLAGS_NONE);
            tmp = tmp->next;
        }
        Sprintf(mcbuf, "%s menu colors",
                (opt_idx == 1) ? "List of" : "Remove which");
        end_menu(tmpwin, mcbuf);
        pick_cnt = select_menu(tmpwin,
                               (opt_idx == 1) ? PICK_NONE : PICK_ANY,
                               &pick_list);
        if (pick_cnt > 0) {
            for (pick_idx = 0; pick_idx < pick_cnt; ++pick_idx)
                free_one_menu_coloring(pick_list[pick_idx].item.a_int - 1
                                       - pick_idx);
            free((genericptr_t) pick_list), pick_list = (menu_item *) 0;
        }
        destroy_nhwindow(tmpwin);
        if (pick_cnt >= 0)
            goto menucolors_again;
    }
    return optn_ok;
}

static int
handler_msgtype(void)
{
    winid tmpwin;
    anything any;
    int opt_idx, nmt, mttyp;
    char mtbuf[BUFSZ];

 msgtypes_again:
    nmt = msgtype_count();
    opt_idx = handle_add_list_remove("message type", nmt);
    if (opt_idx == 3) { /* done */
        return TRUE;
    } else if (opt_idx == 0) { /* add new */
        mtbuf[0] = '\0';
        getlin("What new message pattern?", mtbuf);
        if (*mtbuf == '\033')
            return TRUE;
        if (*mtbuf
            && test_regex_pattern(mtbuf, "MSGTYPE regex")
            && (mttyp = query_msgtype()) != -1
            && !msgtype_add(mttyp, mtbuf)) {
            pline("Error adding the message type.");
            wait_synch();
        }
        goto msgtypes_again;
    } else { /* list (1) or remove (2) */
        int pick_idx, pick_cnt;
        int mt_idx;
        unsigned ln;
        const char *mtype;
        menu_item *pick_list = (menu_item *) 0;
        struct plinemsg_type *tmp = g.plinemsg_types;
        int clr = 0;

        tmpwin = create_nhwindow(NHW_MENU);
        start_menu(tmpwin, MENU_BEHAVE_STANDARD);
        any = cg.zeroany;
        mt_idx = 0;
        while (tmp) {
            mtype = msgtype2name(tmp->msgtype);
            any.a_int = ++mt_idx;
            Sprintf(mtbuf, "%-5s \"", mtype);
            ln = sizeof mtbuf - Strlen(mtbuf) - sizeof "\"";
            if (strlen(tmp->pattern) > ln)
                Strcat(strncat(mtbuf, tmp->pattern, ln - 3), "...\"");
            else
                Strcat(strcat(mtbuf, tmp->pattern), "\"");
            add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0,
                     ATR_NONE, clr, mtbuf, MENU_ITEMFLAGS_NONE);
            tmp = tmp->next;
        }
        Sprintf(mtbuf, "%s message types",
                (opt_idx == 1) ? "List of" : "Remove which");
        end_menu(tmpwin, mtbuf);
        pick_cnt = select_menu(tmpwin,
                               (opt_idx == 1) ? PICK_NONE : PICK_ANY,
                               &pick_list);
        if (pick_cnt > 0) {
            for (pick_idx = 0; pick_idx < pick_cnt; ++pick_idx)
                free_one_msgtype(pick_list[pick_idx].item.a_int - 1
                                       - pick_idx);
            free((genericptr_t) pick_list), pick_list = (menu_item *) 0;
        }
        destroy_nhwindow(tmpwin);
        if (pick_cnt >= 0)
            goto msgtypes_again;
    }
    return optn_ok;
}

#ifndef NO_VERBOSE_GRANULARITY

DISABLE_WARNING_FORMAT_NONLITERAL

static int
handler_verbose(int optidx)
{
    winid tmpwin;
    anything any;
    char buf[BUFSZ];
    int pick_cnt;
    int i;
    menu_item *picks = (menu_item *) 0;
    static const char *const vbstrings[] = {
        " verbose toggle (currently %s)",
        " verbose_suppressor[%d] =%08X",
    };
    char vbbuf[QBUFSZ];
    int clr = 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);
    any = cg.zeroany;
    Snprintf(vbbuf, sizeof vbbuf, vbstrings[0], flags.verbose ? "On" : "Off");
    any.a_int = 1;
    add_menu(tmpwin, &nul_glyphinfo, &any, 'a', 0, ATR_NONE, clr,
             vbbuf, MENU_ITEMFLAGS_NONE);
    for (i = 0; i < vb_elements; i++) {
        Snprintf(vbbuf, sizeof vbbuf, vbstrings[1], i,
                 verbosity_suppressions[i]);
        any.a_int = i + 2;
        add_menu(tmpwin, &nul_glyphinfo, &any, 'b' + i, 0,
                 ATR_NONE, clr, vbbuf, MENU_ITEMFLAGS_NONE);
    }
    end_menu(tmpwin, "Select verbosity choices:");

    pick_cnt = select_menu(tmpwin, PICK_ANY, &picks);
    destroy_nhwindow(tmpwin);
    if (pick_cnt > 0) {
        int j;
        /* PICK_ANY, with one preselected entry (ATR_NONE) which
           should be excluded if any other choices were picked */
        for (i = 0; i < pick_cnt; ++i) {
            char abuf[BUFSZ];
            j = picks[i].item.a_int - 2;
            if (j < 0) {
                flags.verbose = !flags.verbose;
            } else {
                Sprintf(buf,
                    "Set verbose_suppressor[%d] (%ld) to what new decimal value ?",
                    j, verbosity_suppressions[j]);
                abuf[0] = '\0';
                getlin(buf, abuf);
                if (abuf[0] == '\033')
                    continue;
                Sprintf(buf, "%s%d:", allopt[optidx].name, j);
                (void) strncat(eos(buf), abuf, (sizeof buf - 1 - strlen(buf)));
                /* pass the buck */
                (void) parseoptions(buf, TRUE, TRUE);
            }
        }
        free((genericptr_t) picks), picks = (menu_item *) 0;
    }
    return optn_ok;
}

RESTORE_WARNING_FORMAT_NONLITERAL

#endif

/*
 **********************************
 *
 *   Parsing Support Functions
 *
 **********************************
 */

static char *
string_for_opt(char *opts, boolean val_optional)
{
    char *colon, *equals;

    colon = index(opts, ':');
    equals = index(opts, '=');
    if (!colon || (equals && equals < colon))
        colon = equals;

    if (!colon || !*++colon) {
        if (!val_optional)
            config_error_add("Missing parameter for '%s'", opts);
        return empty_optstr;
    }
    return colon;
}

static char *
string_for_env_opt(const char *optname, char *opts, boolean val_optional)
{
    if (!g.opt_initial) {
        rejectoption(optname);
        return empty_optstr;
    }
    return string_for_opt(opts, val_optional);
}

static void
bad_negation(const char *optname, boolean with_parameter)
{
    pline_The("%s option may not %sbe negated.", optname,
              with_parameter ? "both have a value and " : "");
}

/* go through all of the options and set the minmatch value
   based on what is needed for uniqueness of each individual
   option. Set a minimum of 3 characters. */
static void
determine_ambiguities(void)
{
    int i, j, len, tmpneeded, needed[SIZE(allopt)];
    const char *p1, *p2;

    for (i = 0; i < SIZE(allopt) - 1; ++i) {
        needed[i] = 0;
    }

    for (i = 0; i < SIZE(allopt) - 1; ++i) {
        for (j = 0; j < SIZE(allopt) - 1; ++j) {
            if (j == i)
                continue;

            p1 = allopt[i].name; /* back to the start */
            p2 = allopt[j].name;
            tmpneeded = 1;
            while (*p1 && *p2 && lowc(*p1) == lowc(*p2)) {
                ++tmpneeded;
                ++p1;
                ++p2;
            }
            if (tmpneeded > needed[i])
                needed[i] = tmpneeded;
            if (tmpneeded > needed[j])
                needed[j] = tmpneeded;
        }
    }
    for (i = 0; i < SIZE(allopt) - 1; ++i) {
        len = Strlen(allopt[i].name);
        allopt[i].minmatch = (needed[i] < 3) ? 3
                                : (needed[i] <= len) ? needed[i] : len;
    }
}

static int
length_without_val(const char *user_string, int len)
{
    const char *p = index(user_string, ':'),
               *q = index(user_string, '=');

    if (!p || (q && q < p))
        p = q;
    if (p) {
        /* 'user_string' hasn't necessarily been through mungspaces()
           so might have tabs or consecutive spaces */
        while (p > user_string && isspace((uchar) *(p - 1)))
            p--;
        len = (int) (p - user_string);
    }
    return len;
}

/* check whether a user-supplied option string is a proper leading
   substring of a particular option name; option string might have
   a colon or equals sign and arbitrary value appended to it */
boolean
match_optname(const char *user_string, const char *optn_name,
              int min_length, boolean val_allowed)
{
    int len = (int) strlen(user_string);

    if (val_allowed)
        len = length_without_val(user_string, len);

    return (boolean) (len >= min_length
                      && !strncmpi(optn_name, user_string, len));
}

void
reset_duplicate_opt_detection(void)
{
    int k;

    for (k = 0; k < OPTCOUNT; ++k)
        allopt[k].dupdetected = 0;
}

static boolean
duplicate_opt_detection(int optidx)
{
    if (g.opt_initial && g.opt_from_file)
        return allopt[optidx].dupdetected++;
    return FALSE;
}

static void
complain_about_duplicate(int optidx)
{
    char buf[BUFSZ];

#ifdef MAC
    /* the Mac has trouble dealing with the output of messages while
     * processing the config file.  That should get fixed one day.
     * For now just return.
     */
#else /* !MAC */
    buf[0] = '\0';
    if (using_alias)
        Sprintf(buf, " (via alias: %s)", allopt[optidx].alias);
    config_error_add("%s option specified multiple times: %s%s",
                     (allopt[optidx].opttyp == CompOpt) ? "compound"
                                                        : "boolean",
                     allopt[optidx].name, buf);
#endif /* ?MAC */
    return;
}

static void
rejectoption(const char *optname)
{
#ifdef MICRO
    pline("\"%s\" settable only from %s.", optname, configfile);
#else
    pline("%s can be set only from NETHACKOPTIONS or %s.", optname,
          configfile);
#endif
}

/*

# errors:
OPTIONS=aaaaaaaaaa[ more than 247 (255 - 8 for 'OPTIONS=') total ]aaaaaaaaaa
OPTIONS
OPTIONS=
MSGTYPE=stop"You swap places with "
MSGTYPE=st.op "You swap places with "
MSGTYPE=stop "You swap places with \"
MENUCOLOR=" blessed "green&none
MENUCOLOR=" holy " = green&reverse
MENUCOLOR=" cursed " = red&uline
MENUCOLOR=" unholy " = reed
OPTIONS=!legacy:true,fooo
OPTIONS=align:!pin
OPTIONS=gender

*/
/* most environment variables will eventually be printed in an error
 * message if they don't work, and most error message paths go through
 * BUFSZ buffers, which could be overflowed by a maliciously long
 * environment variable.  If a variable can legitimately be long, or
 * if it's put in a smaller buffer, the responsible code will have to
 * bounds-check itself.
 */
char *
nh_getenv(const char *ev)
{
    char *getev = getenv(ev);

    if (getev && strlen(getev) <= (BUFSZ / 2))
        return getev;
    else
        return (char *) 0;
}

/* copy up to maxlen-1 characters; 'dest' must be able to hold maxlen;
   treat comma as alternate end of 'src' */
static void
nmcpy(char *dest, const char *src, int maxlen)
{
    int count;

    for (count = 1; count < maxlen; count++) {
        if (*src == ',' || *src == '\0')
            break; /*exit on \0 terminator*/
        *dest++ = *src++;
    }
    *dest = '\0';
}

/*
 * escapes(): escape expansion for showsyms.  C-style escapes understood
 * include \n, \b, \t, \r, \xnnn (hex), \onnn (octal), \nnn (decimal).
 * (Note: unlike in C, leading digit 0 is not used to indicate octal;
 * the letter o (either upper or lower case) is used for that.
 * The ^-prefix for control characters is also understood, and \[mM]
 * has the effect of 'meta'-ing the value which follows (so that the
 * alternate character set will be enabled).
 *
 * X     normal key X
 * ^X    control-X
 * \mX   meta-X
 *
 * For 3.4.3 and earlier, input ending with "\M", backslash, or caret
 * prior to terminating '\0' would pull that '\0' into the output and then
 * keep processing past it, potentially overflowing the output buffer.
 * Now, trailing \ or ^ will act like \\ or \^ and add '\\' or '^' to the
 * output and stop there; trailing \M will fall through to \<other> and
 * yield 'M', then stop.  Any \X or \O followed by something other than
 * an appropriate digit will also fall through to \<other> and yield 'X'
 * or 'O', plus stop if the non-digit is end-of-string.
 */
static void
escapes(const char *cp, /* might be 'tp', updating in place */
        char *tp)       /* result is never longer than 'cp' */
{
    static NEARDATA const char oct[] = "01234567", dec[] = "0123456789",
                               hex[] = "00112233445566778899aAbBcCdDeEfF";
    const char *dp;
    int cval, meta, dcount;

    while (*cp) {
        /* \M has to be followed by something to do meta conversion,
           otherwise it will just be \M which ultimately yields 'M' */
        meta = (*cp == '\\' && (cp[1] == 'm' || cp[1] == 'M') && cp[2]);
        if (meta)
            cp += 2;

        cval = dcount = 0; /* for decimal, octal, hexadecimal cases */
        if ((*cp != '\\' && *cp != '^') || !cp[1]) {
            /* simple character, or nothing left for \ or ^ to escape */
            cval = *cp++;
        } else if (*cp == '^') { /* expand control-character syntax */
            cval = (*++cp & 0x1f);
            ++cp;

        /* remaining cases are all for backslash; we know cp[1] is not \0 */
        } else if (index(dec, cp[1])) {
            ++cp; /* move past backslash to first digit */
            do {
                cval = (cval * 10) + (*cp - '0');
            } while (*++cp && index(dec, *cp) && ++dcount < 3);
        } else if ((cp[1] == 'o' || cp[1] == 'O') && cp[2]
                   && index(oct, cp[2])) {
            cp += 2; /* move past backslash and 'O' */
            do {
                cval = (cval * 8) + (*cp - '0');
            } while (*++cp && index(oct, *cp) && ++dcount < 3);
        } else if ((cp[1] == 'x' || cp[1] == 'X') && cp[2]
                   && (dp = index(hex, cp[2])) != 0) {
            cp += 2; /* move past backslash and 'X' */
            do {
                cval = (cval * 16) + ((int) (dp - hex) / 2);
            } while (*++cp && (dp = index(hex, *cp)) != 0 && ++dcount < 2);
        } else { /* C-style character escapes */
            switch (*++cp) {
            case '\\':
                cval = '\\';
                break;
            case 'n':
                cval = '\n';
                break;
            case 't':
                cval = '\t';
                break;
            case 'b':
                cval = '\b';
                break;
            case 'r':
                cval = '\r';
                break;
            default:
                cval = *cp;
            }
            ++cp;
        }

        if (meta)
            cval |= 0x80;
        *tp++ = (char) cval;
    }
    *tp = '\0';
}

/* returns a one-byte character from the text; may change txt[];
   moved from cmd.c in order to get access to escapes() */
uchar
txt2key(char *txt)
{
    uchar uc;
    boolean makemeta = FALSE;

    txt = trimspaces(txt);
    if (!*txt)
        return '\0';

    /* simple character */
    if (!txt[1])
        return (uchar) txt[0];

    /* a few special entries */
    if (!strcmp(txt, "<enter>"))
        return '\n';
    if (!strcmp(txt, "<space>"))
        return ' ';
    if (!strcmp(txt, "<esc>"))
        return '\033';

    /* handle things like \b and \7 and \mX */
    if (*txt == '\\') {
        char tbuf[QBUFSZ];

        if (strlen(txt) >= sizeof tbuf)
            txt[sizeof tbuf - 1] = '\0';
        escapes(txt, tbuf);
        return *tbuf;
    }

    /* control and meta keys */
    if (highc(*txt) == 'M') {
        /*
         * M <nothing>             return 'M'
         * M - <nothing>           return M-'-'
         * M <other><nothing>      return M-<other>
         * otherwise M is pending until after ^/C- processing.
         * Since trailing spaces are discarded, the only way to
         * specify M-' ' is via "160".
         */
        if (!txt[1])
            return (uchar) *txt;
        /* skip past 'M' or 'm' and maybe '-' */
        ++txt;
        if (*txt == '-' && txt[1])
            ++txt;
        if (!txt[1])
            return M((uchar) *txt);
        makemeta = TRUE;
    }
    if (*txt == '^' || highc(*txt) == 'C') {
        /*
         * C <nothing>             return 'C' or M-'C'
         * C - <nothing>           return '-' or M-'-'
         * C [-] <other><nothing>  return C-<other> or M-C-<other>
         * C [-] ?                 return <rubout>
         * otherwise return C-<other> or M-C-<other>
         */
        uc = (uchar) *txt;
        if (!txt[1])
            return makemeta ? M(uc) : uc;
        ++txt;
        /* unlike M-x, lots of values of x are invalid for C-x;
           checking and rejecting them is not worthwhile; GIGO;
           we do accept "^-x" as synonym for "^x" or "C-x" */
        if (*txt == '-' && txt[1])
            ++txt;
        /* and accept ^?, which gets used despite not being a control char */
        if (*txt == '?')
            return (uchar) (makemeta ? '\377' : '\177'); /* rubout/delete */
        uc = C((uchar) *txt);
        return makemeta ? M(uc) : uc;
    }
    if (makemeta && *txt)
        return M((uchar) *txt);

    /* FIXME: should accept single-quote single-character single-quote
       and probably single-quote backslash octal-digits single-quote;
       if we do that, the M- and C- results should be pending until
       after, so that C-'X' becomes valid for ^X */

    /* ascii codes: must be three-digit decimal */
    if (*txt >= '0' && *txt <= '9') {
        uchar key = 0;
        int i;

        for (i = 0; i < 3; i++) {
            if (txt[i] < '0' || txt[i] > '9')
                return '\0';
            key = 10 * key + txt[i] - '0';
        }
        return key;
    }

    return '\0';
}

/*
 **********************************
 *
 *   Options Initialization
 *
 **********************************
 */

/* process options, possibly including SYSCF */
void
initoptions(void)
{
#ifdef SYSCF_FILE
    int i;
#endif

    initoptions_init();
#ifdef SYSCF
/* someday there may be other SYSCF alternatives besides text file */
#ifdef SYSCF_FILE
    /* If SYSCF_FILE is specified, it _must_ exist... */
    assure_syscf_file();
    config_error_init(TRUE, SYSCF_FILE, FALSE);

    /* Call each option function with an init flag and give it a chance
       to make any preparations that it might require. We do this
       whether or not the option itself is ever specified; that's
       irrelevant for the init call. Doing this allows the prep code for
       option settings to remain adjacent to, and in the same function as,
       the code that processes those options */

    for (i = 0; i < OPTCOUNT; ++i) {
        if (allopt[i].optfn)
            (*allopt[i].optfn)(i, do_init, FALSE, empty_optstr, empty_optstr);
    }

    /* ... and _must_ parse correctly. */
    if (!read_config_file(SYSCF_FILE, set_in_sysconf)) {
        if (config_error_done() && !iflags.initoptions_noterminate)
            nh_terminate(EXIT_FAILURE);
    }
    config_error_done();
    /*
     * TODO [maybe]: parse the sysopt entries which are space-separated
     * lists of usernames into arrays with one name per element.
     */
#endif
#endif /* SYSCF */
    initoptions_finish();
}

/* set up default values for options where 0 or False isn't sufficient */
void
initoptions_init(void)
{
#if (defined(UNIX) || defined(VMS)) && defined(TTY_GRAPHICS)
    char *opts;
#endif
    int i;

    memcpy(allopt, allopt_init, sizeof(allopt));
    determine_ambiguities();

#ifdef ENHANCED_SYMBOLS
    /* make any symbol parsing quicker */
    if (!glyphid_cache_status())
        fill_glyphid_cache();
#endif

    /* set up the command parsing */
    reset_commands(TRUE); /* init */

    /* initialize the random number generator(s) */
    init_random(rn2);
    init_random(rn2_on_display_rng);

    for (i = 0; allopt[i].name; i++) {
        if (allopt[i].addr)
            *(allopt[i].addr) = allopt[i].initval;
    }

    flags.end_own = FALSE;
    flags.end_top = 3;
    flags.end_around = 2;
    flags.paranoia_bits = PARANOID_PRAY; /* old prayconfirm=TRUE */
    flags.pile_limit = PILE_LIMIT_DFLT;  /* 5 */
    flags.runmode = RUN_LEAP;
    iflags.msg_history = 20;
    /* msg_window has conflicting defaults for multi-interface binary */
#ifdef TTY_GRAPHICS
    iflags.prevmsg_window = 's';
#else
#ifdef CURSES_GRAPHICS
    iflags.prevmsg_window = 'r';
#endif
#endif

    iflags.menu_headings = ATR_INVERSE;
    iflags.getpos_coords = GPCOORDS_NONE;

    /* hero's role, race, &c haven't been chosen yet */
    flags.initrole = flags.initrace = flags.initgend = flags.initalign
        = ROLE_NONE;

    init_ov_primary_symbols();
    init_ov_rogue_symbols();
    /* Set the default monster and object class symbols. */
    init_symbols();
    for (i = 0; i < WARNCOUNT; i++)
        g.warnsyms[i] = def_warnsyms[i].sym;

    /* assert( sizeof flags.inv_order == sizeof def_inv_order ); */
    (void) memcpy((genericptr_t) flags.inv_order,
                  (genericptr_t) def_inv_order, sizeof flags.inv_order);
    flags.pickup_types[0] = '\0';
    flags.pickup_burden = MOD_ENCUMBER;
    flags.sortloot = 'l'; /* sort only loot by default */

    for (i = 0; i < NUM_DISCLOSURE_OPTIONS; i++)
        flags.end_disclose[i] = DISCLOSE_PROMPT_DEFAULT_NO;
    switch_symbols(FALSE); /* set default characters */
    init_rogue_symbols();
#if defined(UNIX) && defined(TTY_GRAPHICS)
    /*
     * Set defaults for some options depending on what we can
     * detect about the environment's capabilities.
     * This has to be done after the global initialization above
     * and before reading user-specific initialization via
     * config file/environment variable below.
     */
    /* this detects the IBM-compatible console on most 386 boxes */
    if ((opts = nh_getenv("TERM")) && !strncmp(opts, "AT", 2)) {
        if (!g.symset[PRIMARYSET].explicitly)
            load_symset("IBMGraphics", PRIMARYSET);
        if (!g.symset[ROGUESET].explicitly)
            load_symset("RogueIBM", ROGUESET);
        switch_symbols(TRUE);
#ifdef TEXTCOLOR
        iflags.use_color = TRUE;
#endif
    }
#endif /* UNIX && TTY_GRAPHICS */
#if defined(UNIX) || defined(VMS)
#ifdef TTY_GRAPHICS
    /* detect whether a "vt" terminal can handle alternate charsets */
    if ((opts = nh_getenv("TERM"))
        /* [could also check "xterm" which emulates vtXXX by default] */
        && !strncmpi(opts, "vt", 2)
        && AS && AE && index(AS, '\016') && index(AE, '\017')) {
        if (!g.symset[PRIMARYSET].explicitly)
            load_symset("DECGraphics", PRIMARYSET);
        switch_symbols(TRUE);
    }
#endif
#endif /* UNIX || VMS */

#if defined(MSDOS) || defined(WIN32)
    /* Use IBM defaults. Can be overridden via config file */
    if (!g.symset[PRIMARYSET].explicitly)
        load_symset("IBMGraphics_2", PRIMARYSET);
    if (!g.symset[ROGUESET].explicitly)
        load_symset("RogueEpyx", ROGUESET);
#endif
#ifdef MAC_GRAPHICS_ENV
    if (!g.symset[PRIMARYSET].explicitly)
        load_symset("MACGraphics", PRIMARYSET);
    switch_symbols(TRUE);
#endif /* MAC_GRAPHICS_ENV */
    flags.menu_style = MENU_FULL;

    iflags.wc_align_message = ALIGN_TOP;
    iflags.wc_align_status = ALIGN_BOTTOM;
    /* used by tty and curses */
    iflags.wc2_statuslines = 2;
    /* only used by curses */
    iflags.wc2_windowborders = 2; /* 'Auto' */

    /*
     * A few menus have certain items (typically operate-on-everything or
     * change-subset or sort or help entries) flagged as 'skip-invert' to
     * control how whole-page and whole-menu operations affect them.
     * 'menuinvertmode' controls how that functions:
     * 0: ignore 'skip-invert' flag on menu items (used to be the default);
     * 1: don't toggle 'skip-invert' items On for set-all/set-page/invert-
     *    all/invert-page but do toggle Off if already set (default);
     * 2: don't toggle 'skip-invert' items either On of Off for set-all/
     *    set-page/unset-all/unset-page/invert-all/invert-page.
     */
    iflags.menuinvertmode = 1;

    /* since this is done before init_objects(), do partial init here */
    objects[SLIME_MOLD].oc_name_idx = SLIME_MOLD;
    nmcpy(g.pl_fruit, OBJ_NAME(objects[SLIME_MOLD]), PL_FSIZ);
}

/*
 *  Process user's run-time configuration file:
 *    get value of NETHACKOPTIONS;
 *    if command line specified -nethackrc=filename, use that;
 *      if NETHACKOPTIONS is present,
 *        honor it if it has a list of options to set
 *        or ignore it if it specifies a file name;
 *    else if not specified on command line and NETHACKOPTIONS names a file,
 *      use that as the config file;
 *      no extra options (normal use of NETHACKOPTIONS) will be set;
 *    otherwise (not on command line and either no NETHACKOPTIONS or that
 *        isn't a file name),
 *      pass Null to read_config_file() so that it will read ~/.nethackrc
 *        by default,
 *      then process the value of NETHACKOPTIONS as extra options.
 */
void
initoptions_finish(void)
{
    nhsym sym = 0;
    char *opts = 0, *xtraopts = 0;
#ifndef MAC
    const char *envname, *namesrc, *nameval;

    /* getenv() instead of nhgetenv(): let total length of options be long;
       parseoptions() will check each individually */
    envname = "NETHACKOPTIONS";
    opts = getenv(envname);
    if (!opts) {
        /* fall back to original name; discouraged */
        envname = "HACKOPTIONS";
        opts = getenv(envname);
    }

    if (g.cmdline_rcfile) {
        namesrc = "command line";
        nameval = g.cmdline_rcfile;
        free((genericptr_t) g.cmdline_rcfile), g.cmdline_rcfile = 0;
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
    } else
#endif /* !MAC */
    /*else*/ {
        /* either no NETHACKOPTIONS or it wasn't a file name;
           read the default configuration file */
        nameval = namesrc = 0;
        xtraopts = opts;
    }

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
        config_error_init(FALSE, envname, FALSE);
        (void) parseoptions(xtraopts, TRUE, FALSE);
        config_error_done();
    }
    /*[end of nethackrc handling]*/

    (void) fruitadd(g.pl_fruit, (struct fruit *) 0);
    /*
     * Remove "slime mold" from list of object names.  This will
     * prevent it from being wished unless it's actually present
     * as a named (or default) fruit.  Wishing for "fruit" will
     * result in the player's preferred fruit.  [Once upon a time
     * the override value used was "\033" which prevented wishing
     * for the slime mold object at all except by asking for a
     * specific named fruit.]  Note that there are multiple fruit
     * object types (apple, melon, &c) but the "fruit" object is
     * slime mold or whatever custom name player assigns to that.
     */
    obj_descr[SLIME_MOLD].oc_name = "fruit";

    sym = get_othersym(SYM_BOULDER,
                Is_rogue_level(&u.uz) ? ROGUESET : PRIMARYSET);
    if (sym)
        g.showsyms[SYM_BOULDER + SYM_OFF_X] = sym;
    reglyph_darkroom();
    reset_glyphmap(gm_optionchange);
#ifdef STATUS_HILITES
    /*
     * A multi-interface binary might only support status highlighting
     * for some of the interfaces; check whether we asked for it but are
     * using one which doesn't.
     *
     * Option processing can take place before a user-decided WindowPort
     * is even initialized, so check for that too.
     */
    if (!WINDOWPORT(safestartup)) {
        if (iflags.hilite_delta && !wc2_supported("statushilites")) {
            raw_printf("Status highlighting not supported for %s interface.",
                       windowprocs.name);
            iflags.hilite_delta = 0;
        }
    }
#endif
    update_rest_on_space();
#ifdef ENHANCED_SYMBOLS
    if (glyphid_cache_status())
        free_glyphid_cache();
    apply_customizations_to_symset(g.currentgraphics);
#endif
    g.opt_initial = FALSE;
    return;
}

/*
 *******************************************
 *
 * Support Functions for Individual Options
 *
 *******************************************
 */

/*
 * Change the inventory order, using the given string as the new order.
 * Missing characters in the new order are filled in at the end from
 * the current inv_order, except for gold, which is forced to be first
 * if not explicitly present.
 *
 * This routine returns 1 unless there is a duplicate or bad char in
 * the string.
 *
 * Used by: optfn_packorder()
 *
 */
static int
change_inv_order(char *op)
{
    int oc_sym, num;
    char *sp, buf[QBUFSZ];
    int retval = 1;

    num = 0;
    if (!index(op, GOLD_SYM))
        buf[num++] = COIN_CLASS;

    for (sp = op; *sp; sp++) {
        boolean fail = FALSE;
        oc_sym = def_char_to_objclass(*sp);
        /* reject bad or duplicate entries */
        if (oc_sym == MAXOCLASSES) { /* not an object class char */
            config_error_add("Not an object class '%c'", *sp);
            retval = 0;
            fail = TRUE;
        } else if (!index(flags.inv_order, oc_sym)) {
            /* VENOM_CLASS, RANDOM_CLASS, and ILLOBJ_CLASS are excluded
               because they aren't in def_inv_order[] so don't make it
               into flags.inv_order, hence always fail this index() test */
            config_error_add("Object class '%c' not allowed", *sp);
            retval = 0;
            fail = TRUE;
        } else if (index(sp + 1, *sp)) {
            config_error_add("Duplicate object class '%c'", *sp);
            retval = 0;
            fail = TRUE;
        }
        /* retain good ones */
        if (!fail)
            buf[num++] = (char) oc_sym;
    }
    buf[num] = '\0';

    /* fill in any omitted classes, using previous ordering */
    for (sp = flags.inv_order; *sp; sp++)
        if (!index(buf, *sp))
            (void) strkitten(&buf[num++], *sp);
    buf[MAXOCLASSES - 1] = '\0';

    Strcpy(flags.inv_order, buf);
    return retval;
}


/*
 * Support functions for "warning"
 *
 * Used by: optfn_warnings()
 *
 */

static boolean
warning_opts(char *opts, const char *optype)
{
    uchar translate[WARNCOUNT];
    int length, i;

    if ((opts = string_for_env_opt(optype, opts, FALSE)) == empty_optstr)
        return FALSE;
    escapes(opts, opts);

    length = (int) strlen(opts);
    /* match the form obtained from PC configuration files */
    for (i = 0; i < WARNCOUNT; i++)
        translate[i] = (i >= length) ? 0
                                     : opts[i] ? (uchar) opts[i]
                                               : def_warnsyms[i].sym;
    assign_warnings(translate);
    return TRUE;
}

void
assign_warnings(uchar *graph_chars)
{
    int i;

    for (i = 0; i < WARNCOUNT; i++)
        if (graph_chars[i])
            g.warnsyms[i] = graph_chars[i];
}

/*
 * Support functions for "suppress_alert"
 *
 * Used by: optfn_suppress_alert()
 *
 */

static int
feature_alert_opts(char *op, const char *optn)
{
    char buf[BUFSZ];
    unsigned long fnv = get_feature_notice_ver(op); /* version.c */

    if (fnv == 0L)
        return 0;
    if (fnv > get_current_feature_ver()) {
        if (!g.opt_initial) {
            You_cant("disable new feature alerts for future versions.");
        } else {
            config_error_add(
                        "%s=%s Invalid reference to a future version ignored",
                             optn, op);
        }
        return 0;
    }

    flags.suppress_alert = fnv;
    if (!g.opt_initial) {
        Sprintf(buf, "%lu.%lu.%lu", FEATURE_NOTICE_VER_MAJ,
                FEATURE_NOTICE_VER_MIN, FEATURE_NOTICE_VER_PATCH);
        pline(
          "Feature change alerts disabled for NetHack %s features and prior.",
              buf);
    }
    return 1;
}

/*
 * This is used by parse_config_line() in files.c
 *
 */

/* parse key:command[,key2:command2,...] after BINDINGS= prefix has been
   stripped; returns False if any problem seen, True if every binding in
   the comma-separated list is successful */
boolean
parsebindings(char *bindings)
{
    char *bind;
    uchar key;
    int i;
    boolean ret = TRUE; /* assume success */

    /* look for first comma, then decide whether it is the key being bound
       or a list element separator; if it's a key, find separator beyond it */
    if ((bind = index(bindings, ',')) != 0) {
        /* at start so it represents a key */
        if (bind == bindings)
            bind = index(bind + 1, ',');

        /* to get here, bind is non-Null and not equal to bindings,
           so it is greater than bindings and bind[-1] is valid; check
           whether current comma happens to be for "\,:cmd" or "',':cmd"
           (":cmd" part is assumed if the comma has expected quoting) */
        else if (bind[-1] == '\\' || (bind[-1] == '\'' && bind[1] == '\''))
            bind = index(bind + 2, ',');
    }
    /* if a comma separator has been found, break off first binding from rest;
       parse the rest and then handle this first one when recursion returns */
    if (bind) {
        *bind++ = '\0';
        if (!parsebindings(bind))
            ret = FALSE;
    }
    
    /* parse a single binding: first split around : */
    if (! (bind = index(bindings, ':')))
        return FALSE; /* it's not a binding */
    *bind++ = 0;

    /* read the key to be bound */
    key = txt2key(bindings);
    if (!key) {
        config_error_add("Unknown key binding key '%s'", bindings);
        return FALSE;
    }

    bind = trimspaces(bind);

    /* is it a special key? */
    if (bind_specialkey(key, bind))
        return ret;

    /* is it a menu command? */
    for (i = 0; default_menu_cmd_info[i].name; i++) {
        if (!strcmp(default_menu_cmd_info[i].name, bind)) {
            if (illegal_menu_cmd_key(key)) {
                config_error_add("Bad menu key %s:%s", visctrl(key), bind);
                return FALSE;
            } else {
                add_menu_cmd_alias((char) key, default_menu_cmd_info[i].cmd);
            }
            return ret;
        }
    }

    /* extended command? */
    if (!bind_key(key, bind)) {
        config_error_add("Unknown key binding command '%s'", bind);
        return FALSE;
    }
    return ret;
}

/*
 * Color support functions and data for "color"
 *
 * Used by: optfn_()
 *
 */

static const struct color_names {
    const char *name;
    int color;
} colornames[] = {
    { "black", CLR_BLACK },
    { "red", CLR_RED },
    { "green", CLR_GREEN },
    { "brown", CLR_BROWN },
    { "blue", CLR_BLUE },
    { "magenta", CLR_MAGENTA },
    { "cyan", CLR_CYAN },
    { "gray", CLR_GRAY },
    { "orange", CLR_ORANGE },
    { "light green", CLR_BRIGHT_GREEN },
    { "yellow", CLR_YELLOW },
    { "light blue", CLR_BRIGHT_BLUE },
    { "light magenta", CLR_BRIGHT_MAGENTA },
    { "light cyan", CLR_BRIGHT_CYAN },
    { "white", CLR_WHITE },
    { "no color", NO_COLOR },
    { NULL, CLR_BLACK }, /* everything after this is an alias */
    { "transparent", NO_COLOR },
    { "purple", CLR_MAGENTA },
    { "light purple", CLR_BRIGHT_MAGENTA },
    { "bright purple", CLR_BRIGHT_MAGENTA },
    { "grey", CLR_GRAY },
    { "bright red", CLR_ORANGE },
    { "bright green", CLR_BRIGHT_GREEN },
    { "bright blue", CLR_BRIGHT_BLUE },
    { "bright magenta", CLR_BRIGHT_MAGENTA },
    { "bright cyan", CLR_BRIGHT_CYAN }
};

static const struct attr_names {
    const char *name;
    int attr;
} attrnames[] = {
    { "none", ATR_NONE },
    { "bold", ATR_BOLD },
    { "dim", ATR_DIM },
    { "underline", ATR_ULINE },
    { "blink", ATR_BLINK },
    { "inverse", ATR_INVERSE },
    { NULL, ATR_NONE }, /* everything after this is an alias */
    { "normal", ATR_NONE },
    { "uline", ATR_ULINE },
    { "reverse", ATR_INVERSE },
};

const char *
clr2colorname(int clr)
{
    int i;

    for (i = 0; i < SIZE(colornames); i++)
        if (colornames[i].name && colornames[i].color == clr)
            return colornames[i].name;
    return (char *) 0;
}

int
match_str2clr(char *str)
{
    int i, c = CLR_MAX;

    /* allow "lightblue", "light blue", and "light-blue" to match "light blue"
       (also junk like "_l i-gh_t---b l u e" but we won't worry about that);
       also copes with trailing space; caller has removed any leading space */
    for (i = 0; i < SIZE(colornames); i++)
        if (colornames[i].name
            && fuzzymatch(str, colornames[i].name, " -_", TRUE)) {
            c = colornames[i].color;
            break;
        }
    if (i == SIZE(colornames) && digit(*str))
        c = atoi(str);

    if (c < 0 || c >= CLR_MAX) {
        config_error_add("Unknown color '%.60s'", str);
        c = CLR_MAX; /* "none of the above" */
    }
    return c;
}

static const char *
attr2attrname(int attr)
{
    int i;

    for (i = 0; i < SIZE(attrnames); i++)
        if (attrnames[i].attr == attr)
            return attrnames[i].name;
    return (char *) 0;
}

int
match_str2attr(const char *str, boolean complain)
{
    int i, a = -1;

    for (i = 0; i < SIZE(attrnames); i++)
        if (attrnames[i].name
            && fuzzymatch(str, attrnames[i].name, " -_", TRUE)) {
            a = attrnames[i].attr;
            break;
        }

    if (a == -1 && complain)
        config_error_add("Unknown text attribute '%.50s'", str);

    return a;
}

extern const char regex_id[]; /* from sys/share/<various>regex.{c,cpp} */

DISABLE_WARNING_FORMAT_NONLITERAL

/* True: temporarily replace menu color entries with a fake set of menu
   colors, { "light blue"=light_blue, "blue"=blue, "red"=red, &c }, that
   illustrates most colors for use when the pick-a-color menu is rendered;
   suppresses black and white because one of those will likely be invisible
   due to matching the background; False: restore user-specified colorings */
static void
basic_menu_colors(boolean load_colors)
{
    if (load_colors) {
        /* replace normal menu colors with a set specifically for colors */
        g.save_menucolors = iflags.use_menu_color;
        g.save_colorings = g.menu_colorings;

        iflags.use_menu_color = TRUE;
        if (g.color_colorings) {
            /* use the alternate colorings which were set up previously */
            g.menu_colorings = g.color_colorings;
        } else {
            /* create the alternate colorings once */
            char cnm[QBUFSZ];
            int i, c;
            boolean pmatchregex = !strcmpi(regex_id, "pmatchregex");
            const char *patternfmt = pmatchregex ? "*%s" : "%s";

            /* menu_colorings pointer has been saved; clear it in order
               to add the alternate entries as if from scratch */
            g.menu_colorings = (struct menucoloring *) 0;

            /* this orders the patterns last-in/first-out; that means
               that the "light <foo>" variations come before the basic
               "<foo>" ones, which is exactly what we want */
            for (i = 0; i < SIZE(colornames); ++i) {
                if (!colornames[i].name) /* first alias entry has no name */
                    break;
                c = colornames[i].color;
                if (c == CLR_BLACK || c == CLR_WHITE || c == NO_COLOR)
                    continue; /* skip these */
                Sprintf(cnm, patternfmt, colornames[i].name);
                add_menu_coloring_parsed(cnm, c, ATR_NONE);
            }

            /* right now, menu_colorings contains the alternate color list;
               remember that list for future pick-a-color instances and
               also keep it as is for this instance */
            g.color_colorings = g.menu_colorings;
        }
    } else {
        /* restore normal user-specified menu colors */
        iflags.use_menu_color = g.save_menucolors;
        g.menu_colorings = g.save_colorings;
    }
}

RESTORE_WARNING_FORMAT_NONLITERAL

int
query_color(const char *prompt)
{
    winid tmpwin;
    anything any;
    int i, pick_cnt;
    menu_item *picks = (menu_item *) 0;
    int clr = 0;

    /* replace user patterns with color name ones and force 'menucolors' On */
    basic_menu_colors(TRUE);

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);
    any = cg.zeroany;
    for (i = 0; i < SIZE(colornames); i++) {
        if (!colornames[i].name)
            break;
        any.a_int = i + 1;
        add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0,
                 ATR_NONE, clr, colornames[i].name,
                 (colornames[i].color == NO_COLOR) ? MENU_ITEMFLAGS_SELECTED
                                                   : MENU_ITEMFLAGS_NONE);
    }
    end_menu(tmpwin, (prompt && *prompt) ? prompt : "Pick a color");
    pick_cnt = select_menu(tmpwin, PICK_ONE, &picks);
    destroy_nhwindow(tmpwin);

    /* remove temporary color name patterns and restore user-specified ones;
       reset 'menucolors' option to its previous value */
    basic_menu_colors(FALSE);

    if (pick_cnt > 0) {
        i = colornames[picks[0].item.a_int - 1].color;
        /* pick_cnt==2: explicitly picked something other than the
           preselected entry */
        if (pick_cnt == 2 && i == NO_COLOR)
            i = colornames[picks[1].item.a_int - 1].color;
        free((genericptr_t) picks);
        return i;
    } else if (pick_cnt == 0) {
        /* pick_cnt==0: explicitly picking preselected entry toggled it off */
        return NO_COLOR;
    }
    return -1;
}

/* ask about highlighting attribute; for menu headers and menu
   coloring patterns, only one attribute at a time is allowed;
   for status highlighting, multiple attributes are allowed [overkill;
   life would be much simpler if that were restricted to one also...] */
int
query_attr(const char *prompt)
{
    winid tmpwin;
    anything any;
    int i, pick_cnt;
    menu_item *picks = (menu_item *) 0;
    boolean allow_many = (prompt && !strncmpi(prompt, "Choose", 6));
    int default_attr = ATR_NONE;
    int clr = 0;

    if (prompt && strstri(prompt, "menu headings"))
        default_attr = iflags.menu_headings;
    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);
    any = cg.zeroany;
    for (i = 0; i < SIZE(attrnames); i++) {
        if (!attrnames[i].name)
            break;
        any.a_int = i + 1;
        add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0,
                 attrnames[i].attr, clr, attrnames[i].name,
                 (attrnames[i].attr == default_attr) ? MENU_ITEMFLAGS_SELECTED
                                                     : MENU_ITEMFLAGS_NONE);
    }
    end_menu(tmpwin, (prompt && *prompt) ? prompt : "Pick an attribute");
    pick_cnt = select_menu(tmpwin, allow_many ? PICK_ANY : PICK_ONE, &picks);
    destroy_nhwindow(tmpwin);
    if (pick_cnt > 0) {
        int j, k = 0;

        if (allow_many) {
            /* PICK_ANY, with one preselected entry (ATR_NONE) which
               should be excluded if any other choices were picked */
            for (i = 0; i < pick_cnt; ++i) {
                j = picks[i].item.a_int - 1;
                if (attrnames[j].attr != ATR_NONE || pick_cnt == 1) {
                    switch (attrnames[j].attr) {
                    case ATR_DIM:
                        k |= HL_DIM;
                        break;
                    case ATR_BLINK:
                        k |= HL_BLINK;
                        break;
                    case ATR_ULINE:
                        k |= HL_ULINE;
                        break;
                    case ATR_INVERSE:
                        k |= HL_INVERSE;
                        break;
                    case ATR_BOLD:
                        k |= HL_BOLD;
                        break;
                    case ATR_NONE:
                        k = HL_NONE;
                        break;
                    }
                }
            }
        } else {
            /* PICK_ONE, but might get 0 or 2 due to preselected entry */
            j = picks[0].item.a_int - 1;
            /* pick_cnt==2: explicitly picked something other than the
               preselected entry */
            if (pick_cnt == 2 && attrnames[j].attr == default_attr)
                j = picks[1].item.a_int - 1;
            k = attrnames[j].attr;
        }
        free((genericptr_t) picks);
        return k;
    } else if (pick_cnt == 0 && !allow_many) {
        /* PICK_ONE, preselected entry explicitly chosen */
        return default_attr;
    }
    /* either ESC to explicitly cancel (pick_cnt==-1) or
       PICK_ANY with preselected entry toggled off and nothing chosen */
    return -1;
}

static const struct {
    const char *name;
    xint8 msgtyp;
    const char *descr;
} msgtype_names[] = {
    { "show", MSGTYP_NORMAL, "Show message normally" },
    { "hide", MSGTYP_NOSHOW, "Hide message" },
    { "noshow", MSGTYP_NOSHOW, NULL },
    { "stop", MSGTYP_STOP, "Prompt for more after the message" },
    { "more", MSGTYP_STOP, NULL },
    { "norep", MSGTYP_NOREP, "Do not repeat the message" }
};

static const char *
msgtype2name(int typ)
{
    int i;

    for (i = 0; i < SIZE(msgtype_names); i++)
        if (msgtype_names[i].descr && msgtype_names[i].msgtyp == typ)
            return msgtype_names[i].name;
    return (char *) 0;
}

static int
query_msgtype(void)
{
    winid tmpwin;
    anything any;
    int i, pick_cnt;
    menu_item *picks = (menu_item *) 0;
    int clr = 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);
    any = cg.zeroany;
    for (i = 0; i < SIZE(msgtype_names); i++)
        if (msgtype_names[i].descr) {
            any.a_int = msgtype_names[i].msgtyp + 1;
            add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0,
                     ATR_NONE, clr,
                     msgtype_names[i].descr, MENU_ITEMFLAGS_NONE);
        }
    end_menu(tmpwin, "How to show the message");
    pick_cnt = select_menu(tmpwin, PICK_ONE, &picks);
    destroy_nhwindow(tmpwin);
    if (pick_cnt > 0) {
        i = picks->item.a_int - 1;
        free((genericptr_t) picks);
        return i;
    }
    return -1;
}

static boolean
msgtype_add(int typ, char *pattern)
{
    static const char *const re_error = "MSGTYPE regex error";
    struct plinemsg_type *tmp = (struct plinemsg_type *) alloc(sizeof *tmp);

    tmp->msgtype = typ;
    tmp->regex = regex_init();
    /* test_regex_pattern() has already validated this regexp but parsing
       it again could conceivably run out of memory */
    if (!regex_compile(pattern, tmp->regex)) {
        char errbuf[BUFSZ];
        char *re_error_desc = regex_error_desc(tmp->regex, errbuf);

        /* free first in case reason for failure was insufficient memory */
        regex_free(tmp->regex);
        free((genericptr_t) tmp);
        config_error_add("%s: %s", re_error, re_error_desc);
        return FALSE;
    }
    tmp->pattern = dupstr(pattern);
    tmp->next = g.plinemsg_types;
    g.plinemsg_types = tmp;
    return TRUE;
}

void
msgtype_free(void)
{
    struct plinemsg_type *tmp, *tmp2 = 0;

    for (tmp = g.plinemsg_types; tmp; tmp = tmp2) {
        tmp2 = tmp->next;
        free((genericptr_t) tmp->pattern);
        regex_free(tmp->regex);
        free((genericptr_t) tmp);
    }
    g.plinemsg_types = (struct plinemsg_type *) 0;
}

static void
free_one_msgtype(int idx) /* 0 .. */
{
    struct plinemsg_type *tmp = g.plinemsg_types;
    struct plinemsg_type *prev = NULL;

    while (tmp) {
        if (idx == 0) {
            struct plinemsg_type *next = tmp->next;

            regex_free(tmp->regex);
            free((genericptr_t) tmp->pattern);
            free((genericptr_t) tmp);
            if (prev)
                prev->next = next;
            else
                g.plinemsg_types = next;
            return;
        }
        idx--;
        prev = tmp;
        tmp = tmp->next;
    }
}

int
msgtype_type(const char *msg,
             boolean norepeat) /* called from Norep(via pline) */
{
    struct plinemsg_type *tmp = g.plinemsg_types;

    while (tmp) {
        /* we don't exclude entries with negative msgtype values
           because then the msg might end up matching a later pattern */
        if (regex_match(msg, tmp->regex))
            return tmp->msgtype;
        tmp = tmp->next;
    }
    return norepeat ? MSGTYP_NOREP : MSGTYP_NORMAL;
}

/* negate one or more types of messages so that their type handling will
   be disabled or re-enabled; MSGTYPE_NORMAL (value 0) is not affected */
void
hide_unhide_msgtypes(boolean hide, int hide_mask)
{
    struct plinemsg_type *tmp;
    int mt;

    /* negative msgtype value won't be recognized by pline, so does nothing */
    for (tmp = g.plinemsg_types; tmp; tmp = tmp->next) {
        mt = tmp->msgtype;
        if (!hide)
            mt = -mt; /* unhide: negate negative, yielding positive */
        if (mt > 0 && ((1 << mt) & hide_mask))
            tmp->msgtype = -tmp->msgtype;
    }
}

static int
msgtype_count(void)
{
    int c = 0;
    struct plinemsg_type *tmp = g.plinemsg_types;

    while (tmp) {
        c++;
        tmp = tmp->next;
    }
    return c;
}

boolean
msgtype_parse_add(char *str)
{
    char pattern[256];
    char msgtype[11];

    if (sscanf(str, "%10s \"%255[^\"]\"", msgtype, pattern) == 2) {
        int typ = -1;
        int i;

        for (i = 0; i < SIZE(msgtype_names); i++)
            if (str_start_is(msgtype_names[i].name, msgtype, TRUE)) {
                typ = msgtype_names[i].msgtyp;
                break;
            }
        if (typ != -1)
            return msgtype_add(typ, pattern);
        else
            config_error_add("Unknown message type '%s'", msgtype);
    } else {
        config_error_add("Malformed MSGTYPE");
    }
    return FALSE;
}

/* parse 'str' as a regular expression to check whether it's valid;
   compiled regexp gets thrown away regardless of the outcome */
static boolean
test_regex_pattern(const char *str, const char *errmsg)
{
    static const char def_errmsg[] = "NHregex error";
    struct nhregex *match;
    char *re_error_desc, errbuf[BUFSZ];
    boolean retval;

    if (!str)
        return FALSE;
    if (!errmsg)
        errmsg = def_errmsg;

    match = regex_init();
    if (!match) {
        config_error_add("%s", errmsg);
        return FALSE;
    }

    retval = regex_compile(str, match);
    /* get potential error message before freeing regexp and free regexp
       before issuing message in case the error is "ran out of memory"
       since message delivery might need to allocate some memory */
    re_error_desc = !retval ? regex_error_desc(match, errbuf) : 0;
    /* discard regexp; caller will re-parse it after validating other stuff */
    regex_free(match);
    /* if returning failure, tell player */
    if (!retval)
        config_error_add("%s: %s", errmsg, re_error_desc);

    return retval;
}

static boolean
add_menu_coloring_parsed(const char *str, int c, int a)
{
    static const char re_error[] = "Menucolor regex error";
    struct menucoloring *tmp;

    if (!str)
        return FALSE;
    tmp = (struct menucoloring *) alloc(sizeof *tmp);
    tmp->match = regex_init();
    /* test_regex_pattern() has already validated this regexp but parsing
       it again could conceivably run out of memory */
    if (!regex_compile(str, tmp->match)) {
        char errbuf[BUFSZ];
        char *re_error_desc = regex_error_desc(tmp->match, errbuf);

        /* free first in case reason for regcomp failure was out-of-memory */
        regex_free(tmp->match);
        free((genericptr_t) tmp);
        config_error_add("%s: %s", re_error, re_error_desc);
        return FALSE;
    }
    tmp->next = g.menu_colorings;
    tmp->origstr = dupstr(str);
    tmp->color = c;
    tmp->attr = a;
    g.menu_colorings = tmp;
    return TRUE;
}

/* parse '"regex_string"=color&attr' and add it to menucoloring */
boolean
add_menu_coloring(char *tmpstr) /* never Null but could be empty */
{
    int c = NO_COLOR, a = ATR_NONE;
    char *tmps, *cs, *amp;
    char str[BUFSZ];

    (void) strncpy(str, tmpstr, sizeof str - 1);
    str[sizeof str - 1] = '\0';

    if ((cs = index(str, '=')) == 0) {
        config_error_add("Malformed MENUCOLOR");
        return FALSE;
    }

    tmps = cs + 1; /* advance past '=' */
    mungspaces(tmps);
    if ((amp = index(tmps, '&')) != 0)
        *amp = '\0';

    c = match_str2clr(tmps);
    if (c >= CLR_MAX)
        return FALSE;

    if (amp) {
        tmps = amp + 1; /* advance past '&' */
        a = match_str2attr(tmps, TRUE);
        if (a == -1)
            return FALSE;
    }

    /* the regexp portion here has not been condensed by mungspaces() */
    *cs = '\0';
    tmps = str;
    if (*tmps == '"' || *tmps == '\'') {
        cs--;
        while (isspace((uchar) *cs))
            cs--;
        if (*cs == *tmps) {
            *cs = '\0';
            tmps++;
        }
    }
    return add_menu_coloring_parsed(tmps, c, a);
}

boolean
get_menu_coloring(const char *str, int *color, int *attr)
{
    struct menucoloring *tmpmc;

    if (iflags.use_menu_color)
        for (tmpmc = g.menu_colorings; tmpmc; tmpmc = tmpmc->next)
            if (regex_match(str, tmpmc->match)) {
                *color = tmpmc->color;
                *attr = tmpmc->attr;
                return TRUE;
            }
    return FALSE;
}

/* release all menu color patterns */
void
free_menu_coloring(void)
{
    /* either menu_colorings or color_colorings or both might need to
       be freed or already be Null; do-loop will iterate at most twice */
    do {
        struct menucoloring *tmp, *tmp2;

        for (tmp = g.menu_colorings; tmp; tmp = tmp2) {
            tmp2 = tmp->next;
            regex_free(tmp->match);
            free((genericptr_t) tmp->origstr);
            free((genericptr_t) tmp);
        }
        g.menu_colorings = g.color_colorings;
        g.color_colorings = (struct menucoloring *) 0;
    } while (g.menu_colorings);
}

/* release a specific menu color pattern; not used for color_colorings */
static void
free_one_menu_coloring(int idx) /* 0 .. */
{
    struct menucoloring *tmp = g.menu_colorings;
    struct menucoloring *prev = NULL;

    while (tmp) {
        if (idx == 0) {
            struct menucoloring *next = tmp->next;

            regex_free(tmp->match);
            free((genericptr_t) tmp->origstr);
            free((genericptr_t) tmp);
            if (prev)
                prev->next = next;
            else
                g.menu_colorings = next;
            return;
        }
        idx--;
        prev = tmp;
        tmp = tmp->next;
    }
}

static int
count_menucolors(void)
{
    struct menucoloring *tmp;
    int count = 0;

    for (tmp = g.menu_colorings; tmp; tmp = tmp->next)
        count++;
    return count;
}

static boolean
parse_role_opts(int optidx, boolean negated, const char *fullname,
                char *opts, char **opp)
{
    char *op = *opp;

    if (negated) {
        bad_negation(fullname, FALSE);
    } else if ((op = string_for_env_opt(fullname, opts, FALSE))
                                        != empty_optstr) {
        boolean val_negated = FALSE;

        while ((*op == '!') || !strncmpi(op, "no", 2)) {
            if (*op == '!')
                op++;
            else
                op += 2;
            val_negated = !val_negated;
        }
        if (val_negated) {
            if (!setrolefilter(op)) {
                config_error_add("Unknown negated parameter '%s'", op);
                return FALSE;
            }
        } else {
            if (duplicate && !allopt[optidx].dupeok)
                complain_about_duplicate(optidx);
            *opp = op;
            return TRUE;
        }
    }
    return FALSE;
}

/* Check if character c is illegal as a menu command key */
static boolean
illegal_menu_cmd_key(uchar c)
{
    if (c == 0 || c == '\r' || c == '\n' || c == '\033' || c == ' '
        || digit((char) c) || (letter((char) c) && c != '@')) {
        config_error_add("Reserved menu command key '%s'", visctrl((char) c));
        return TRUE;
    } else { /* reject default object class symbols */
        int j;

        for (j = 1; j < MAXOCLASSES; j++)
            if (c == (uchar) def_oc_syms[j].sym) {
                config_error_add("Menu command key '%s' is an object class",
                                 visctrl((char) c));
                return TRUE;
            }
    }
    return FALSE;
}


/*
 * Convert the given string of object classes to a string of default object
 * symbols.
 */
void
oc_to_str(char *src, char *dest)
{
    int i;

    while ((i = (int) *src++) != 0) {
        if (i < 0 || i >= MAXOCLASSES)
            impossible("oc_to_str:  illegal object class %d", i);
        else
            *dest++ = def_oc_syms[i].sym;
    }
    *dest = '\0';
}

/*
 * Add the given mapping to the menu command map list.  Always keep the
 * maps valid C strings.
 */
void
add_menu_cmd_alias(char from_ch, char to_ch)
{
    if (g.n_menu_mapped >= MAX_MENU_MAPPED_CMDS) {
        pline("out of menu map space.");
    } else {
        g.mapped_menu_cmds[g.n_menu_mapped] = from_ch;
        g.mapped_menu_op[g.n_menu_mapped] = to_ch;
        g.n_menu_mapped++;
        g.mapped_menu_cmds[g.n_menu_mapped] = '\0';
        g.mapped_menu_op[g.n_menu_mapped] = '\0';
    }
}

char
get_menu_cmd_key(char ch)
{
    char *found = index(g.mapped_menu_op, ch);

    if (found) {
        int idx = (int) (found - g.mapped_menu_op);

        ch = g.mapped_menu_cmds[idx];
    }
    return ch;
}

/*
 * Map the given character to its corresponding menu command.  If it
 * doesn't match anything, just return the original.
 */
char
map_menu_cmd(char ch)
{
    char *found = index(g.mapped_menu_cmds, ch);

    if (found) {
        int idx = (int) (found - g.mapped_menu_cmds);

        ch = g.mapped_menu_op[idx];
    }
    return ch;
}

/* get keystrokes that are used for menu scrolling operations which apply;
   printable: for use in a prompt, non-printable: for yn_function() choices */
char *
collect_menu_keys(
    char *outbuf,        /* at least big enough for 6 "M-^X" sequences +'\0'*/
    unsigned scrollmask, /* 1: backwards, "^<"; 2: forwards, ">|";
                          * 4: left, "{";       8: right, "}"; */
    boolean printable)   /* False: output is string of raw characters,
                          * True: output is a string of visctrl() sequences;
                          * matters iff user has mapped any menu scrolling
                          * commands to control or meta characters */
{
    struct menuscrollinfo {
        char cmdkey;
        uchar maskindx;
    };
    static const struct menuscrollinfo scroll_keys[] = {
        { MENU_FIRST_PAGE,    1 },
        { MENU_PREVIOUS_PAGE, 1 },
        { MENU_NEXT_PAGE,     2 },
        { MENU_LAST_PAGE,     2 },
        { MENU_SHIFT_LEFT,    4 },
        { MENU_SHIFT_RIGHT,   8 },
    };
    int i;

    outbuf[0] = '\0';
    for (i = 0; i < SIZE(scroll_keys); ++i) {
        if (scrollmask & scroll_keys[i].maskindx) {
            char c = get_menu_cmd_key(scroll_keys[i].cmdkey);

            if (printable)
                Strcat(outbuf, visctrl(c));
            else
                (void) strkitten(outbuf, c);
        }
    }
    return outbuf;
}

/* Returns the fid of the fruit type; if that type already exists, it
 * returns the fid of that one; if it does not exist, it adds a new fruit
 * type to the chain and returns the new one.
 * If replace_fruit is sent in, replace the fruit in the chain rather than
 * adding a new entry--for user specified fruits only.
 */
int
fruitadd(char *str, struct fruit *replace_fruit)
{
    register int i;
    register struct fruit *f;
    int highest_fruit_id = 0, globpfx;
    char buf[PL_FSIZ], altname[PL_FSIZ];
    boolean user_specified = (str == g.pl_fruit);
    /* if not user-specified, then it's a fruit name for a fruit on
     * a bones level or from orctown raider's loot...
     */

    /* Note: every fruit has an id (kept in obj->spe) of at least 1;
     * 0 is an error.
     */
    if (user_specified) {
        boolean found = FALSE, numeric = FALSE;

        /* force fruit to be singular; this handling is not
           needed--or wanted--for fruits from bones because
           they already received it in their original game;
           str==pl_fruit but makesingular() creates a copy
           so we need to copy that back into pl_fruit */
        nmcpy(g.pl_fruit, makesingular(str), PL_FSIZ);

        /* disallow naming after other foods (since it'd be impossible
         * to tell the difference); globs might have a size prefix which
         * needs to be skipped in order to match the object type name
         */
        globpfx = (!strncmp(g.pl_fruit, "small ", 6)
                   || !strncmp(g.pl_fruit, "large ", 6)) ? 6
                  : (!strncmp(g.pl_fruit, "medium ", 7)) ? 7
                    : (!strncmp(g.pl_fruit, "very large ", 11)) ? 11
                      : 0;
        for (i = g.bases[FOOD_CLASS]; objects[i].oc_class == FOOD_CLASS; i++) {
            if (!strcmp(OBJ_NAME(objects[i]), g.pl_fruit)
                || (globpfx > 0
                    && !strcmp(OBJ_NAME(objects[i]), &g.pl_fruit[globpfx]))) {
                found = TRUE;
                break;
            }
        }
        if (!found) {
            char *c;

            for (c = g.pl_fruit; *c >= '0' && *c <= '9'; c++)
                continue;
            if (!*c || isspace((uchar) *c))
                numeric = TRUE;
        }
        if (found || numeric
            /* these checks for applying food attributes to actual items
               are case sensitive; "glob of foo" is caught by 'found'
               if 'foo' is a valid glob; when not valid, allow it as-is */
            || !strncmp(g.pl_fruit, "cursed ", 7)
            || !strncmp(g.pl_fruit, "uncursed ", 9)
            || !strncmp(g.pl_fruit, "blessed ", 8)
            || !strncmp(g.pl_fruit, "partly eaten ", 13)
            || (!strncmp(g.pl_fruit, "tin of ", 7)
                && (!strcmp(g.pl_fruit + 7, "spinach")
                    || name_to_mon(g.pl_fruit + 7, (int *) 0) >= LOW_PM))
            || !strcmp(g.pl_fruit, "empty tin")
            || (!strcmp(g.pl_fruit, "glob")
                || (globpfx > 0 && !strcmp("glob", &g.pl_fruit[globpfx])))
            || ((str_end_is(g.pl_fruit, " corpse")
                 || str_end_is(g.pl_fruit, " egg"))
                && name_to_mon(g.pl_fruit, (int *) 0) >= LOW_PM)) {
            Strcpy(buf, g.pl_fruit);
            Strcpy(g.pl_fruit, "candied ");
            nmcpy(g.pl_fruit + 8, buf, PL_FSIZ - 8);
        }
        *altname = '\0';
        /* This flag indicates that a fruit has been made since the
         * last time the user set the fruit.  If it hasn't, we can
         * safely overwrite the current fruit, preventing the user from
         * setting many fruits in a row and overflowing.
         * Possible expansion: check for specific fruit IDs, not for
         * any fruit.
         */
        flags.made_fruit = FALSE;
        if (replace_fruit) {
            /* replace_fruit is already part of the fruit chain;
               update it in place rather than looking it up again */
            f = replace_fruit;
            copynchars(f->fname, g.pl_fruit, PL_FSIZ - 1);
            goto nonew;
        }
    } else {
        /* not user_supplied, so assumed to be from bones (or orc gang) */
        copynchars(altname, str, PL_FSIZ - 1);
        sanitize_name(altname);
        flags.made_fruit = TRUE; /* for safety.  Any fruit name added from a
                                  * bones level should exist anyway. */
    }
    f = fruit_from_name(*altname ? altname : str, FALSE, &highest_fruit_id);
    if (f)
        goto nonew;

    /* Maximum number of named fruits is 127, even if obj->spe can
       handle bigger values.  If adding another fruit would overflow,
       use a random fruit instead... we've got a lot to choose from.
       current_fruit remains as is. */
    if (highest_fruit_id >= 127)
        return rnd(127);

    f = newfruit();
    (void) memset((genericptr_t) f, 0, sizeof (struct fruit));
    copynchars(f->fname, *altname ? altname : str, PL_FSIZ - 1);
    f->fid = ++highest_fruit_id;
    /* we used to go out of our way to add it at the end of the list,
       but the order is arbitrary so use simpler insertion at start */
    f->nextf = g.ffruit;
    g.ffruit = f;
 nonew:
    if (user_specified)
        g.context.current_fruit = f->fid;
    return f->fid;
}

/*
 **********************************
 *
 * Option-setting, menus,
 *  displaying option values
 *
 **********************************
 */


#if defined(MICRO) || defined(MAC) || defined(WIN32)
#define OPTIONS_HEADING "OPTIONS"
#else
#define OPTIONS_HEADING "NETHACKOPTIONS"
#endif

static char fmtstr_doset[] = "%s%-15s [%s]   ";
static char fmtstr_doset_tab[] = "%s\t[%s]";
static char n_currently_set[] = "(%d currently set)";

DISABLE_WARNING_FORMAT_NONLITERAL   /* RESTORE is after show_menucontrols() */

static int
optfn_o_autopickup_exceptions(
    int optidx UNUSED, int req, boolean negated UNUSED,
    char *opts, char *op UNUSED)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, n_currently_set, count_apes());
        return optn_ok;
    }
    if (req == do_handler) {
        return handler_autopickup_exception();
    }
    return optn_ok;
}

static int
optfn_o_menu_colors(int optidx UNUSED, int req, boolean negated UNUSED,
              char *opts, char *op UNUSED)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, n_currently_set, count_menucolors());
        return optn_ok;
    }
    if (req == do_handler) {
        return handler_menu_colors();
    }
    return optn_ok;
}

static int
optfn_o_message_types(int optidx UNUSED, int req, boolean negated UNUSED,
              char *opts, char *op UNUSED)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, n_currently_set, msgtype_count());
        return optn_ok;
    }
    if (req == do_handler) {
        return handler_msgtype();
    }
    return optn_ok;
}

static int
optfn_o_status_cond(int optidx UNUSED, int req, boolean negated UNUSED,
              char *opts, char *op UNUSED)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, n_currently_set, count_cond());
        return optn_ok;
    }
    if (req == do_handler) {
        cond_menu();
        return optn_ok;
    }
    return optn_ok;
}

#ifdef STATUS_HILITES
static int
optfn_o_status_hilites(
    int optidx UNUSED,
    int req,
    boolean negated UNUSED,
    char *opts,
    char *op UNUSED)
{
    if (req == do_init) {
        return optn_ok;
    }
    if (req == do_set) {
    }
    if (req == get_val) {
        if (!opts)
            return optn_err;
        Sprintf(opts, n_currently_set, count_status_hilites());
        return optn_ok;
    }
    if (req == do_handler) {
        if (!status_hilite_menu()) {
            return optn_err; /*pline("Bad status hilite(s) specified.");*/
        } else {
            if (wc2_supported("hilite_status"))
                preference_update("hilite_status");
        }
        return optn_ok;
    }
    return optn_ok;
}
#endif /*STATUS_HILITES*/

/* Get string value of configuration option.
 * Currently handles only boolean and compound options.
 */
char *
get_option_value(const char *optname)
{
    static char retbuf[BUFSZ];
    boolean *bool_p;
    int i;

    for (i = 0; allopt[i].name != 0; i++)
        if (!strcmp(optname, allopt[i].name)) {
            if (allopt[i].opttyp == BoolOpt
                && (bool_p = allopt[i].addr) != 0) {
                Sprintf(retbuf, "%s", *bool_p ? "true" : "false");
                return retbuf;
            } else if (allopt[i].opttyp == CompOpt && allopt[i].optfn) {
                int reslt = optn_err;

                reslt = (*allopt[i].optfn)(allopt[i].idx, get_val,
                                           FALSE, retbuf, empty_optstr);
                if (reslt == optn_ok && retbuf[0])
                    return retbuf;
                return (char *) 0;
            }
        }
    return (char *) 0;
}

static unsigned int
longest_option_name(int startpass, int endpass)
{
    /* spin through the options to find the longest name */
    unsigned longest_name_len = 0;
    int i, pass, optflags;
    const char *name;

    for (pass = 0; pass < 2; pass++)
        for (i = 0; (name = allopt[i].name) != 0; i++) {
            if (pass == 0
                && (allopt[i].opttyp != BoolOpt || !allopt[i].addr))
                continue;
            optflags = allopt[i].setwhere;
            if (optflags < startpass || optflags > endpass)
                continue;
            if ((is_wc_option(name) && !wc_supported(name))
                || (is_wc2_option(name) && !wc2_supported(name)))
                continue;

            unsigned len = Strlen(name);
            if (len > longest_name_len)
                longest_name_len = len;
        }
    return longest_name_len;
}

/* the #options command */
int
doset(void) /* changing options via menu by Per Liboriussen */
{
    static boolean made_fmtstr = FALSE;
    char buf[BUFSZ];
    const char *name;
    int i = 0, pass, pick_cnt, pick_idx, opt_indx;
    boolean *bool_p;
    winid tmpwin;
    anything any;
    menu_item *pick_list;
    int indexoffset, startpass, endpass;
    boolean setinitial = FALSE, fromfile = FALSE,
            gavehelp = FALSE, skiphelp = !iflags.cmdassist;
    int clr = 0;

    /* if we offer '?' as a choice and it is the only thing chosen,
       we'll end up coming back here after showing the explanatory text */
 rerun:
    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);

    /* offer novices a chance to request helpful [sic] advice */
    if (!skiphelp) {
        /* help text surrounding '?' choice should have exactly one NULL */
        static const char *const helptext[] = {
            "For a brief explanation of how this works, type '?' to select",
            "the next menu choice, then press <enter> or <return>.",
            NULL, /* actual '?' menu entry gets inserted here */
            "[To suppress this menu help, toggle off the 'cmdassist' option.]",
            "",
        };
        any = cg.zeroany;
        for (i = 0; i < SIZE(helptext); ++i) {
            if (helptext[i]) {
                Sprintf(buf, "%4s%.75s", "", helptext[i]);
                any.a_int = 0;
                add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, FALSE, clr,
                         buf, MENU_ITEMFLAGS_NONE);
            } else {
                any.a_int = '?' + 1; /* processing pick_list subtracts 1 */
                add_menu(tmpwin, &nul_glyphinfo, &any, '?', '?', ATR_NONE,
                         clr, "view help for options menu",
                         MENU_ITEMFLAGS_SKIPINVERT);
            }
        }
    }

#ifdef notyet /* SYSCF */
    /* XXX I think this is still fragile.  Fixing initial/from_file and/or
       changing the SET_* etc to bitmaps will let me make this better. */
    if (wizard)
        startpass = set_in_sysconf;
    else
#endif
        startpass = set_gameview;
    endpass = (wizard) ? set_wizonly : set_in_game;

    if (!made_fmtstr && !iflags.menu_tab_sep) {
        Sprintf(fmtstr_doset, "%%s%%-%us [%%s]",
                longest_option_name(startpass, endpass));
        made_fmtstr = TRUE;
    }

    indexoffset = 1;
    any = cg.zeroany;
    add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, iflags.menu_headings, clr,
             "Booleans (selecting will toggle value):", MENU_ITEMFLAGS_NONE);
    any.a_int = 0;
    /* first list any other non-modifiable booleans, then modifiable ones */
    for (pass = 0; pass <= 1; pass++)
        for (i = 0; (name = allopt[i].name) != 0; i++)
            if (allopt[i].opttyp == BoolOpt
                && (bool_p = allopt[i].addr) != 0
                && ((allopt[i].setwhere <= set_gameview && pass == 0)
                    || (allopt[i].setwhere >= set_in_game && pass == 1))) {
                if (bool_p == &flags.female)
                    continue; /* obsolete */
                if (allopt[i].setwhere == set_wizonly && !wizard)
                    continue;
                if ((is_wc_option(name) && !wc_supported(name))
                    || (is_wc2_option(name) && !wc2_supported(name)))
                    continue;

                any.a_int = (pass == 0) ? 0 : i + 1 + indexoffset;
                if (!iflags.menu_tab_sep)
                    Sprintf(buf, fmtstr_doset, (pass == 0) ? "    " : "",
                            name, *bool_p ? "true" : "false");
                else
                    Sprintf(buf, fmtstr_doset_tab,
                            name, *bool_p ? "true" : "false");
                if (pass == 0)
                    enhance_menu_text(buf, sizeof buf, pass, bool_p, &allopt[i]);
                add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0,
                         ATR_NONE, clr, buf, MENU_ITEMFLAGS_NONE);
            }
    any = cg.zeroany;
    add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0,
             ATR_NONE, clr, "", MENU_ITEMFLAGS_NONE);
    add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, iflags.menu_headings, clr,
             "Compounds (selecting will prompt for new value):",
             MENU_ITEMFLAGS_NONE);

    /* deliberately put playmode, name, role+race+gender+align first */
    doset_add_menu(tmpwin, "playmode", opt_playmode, 0);
    doset_add_menu(tmpwin, "name", opt_name, 0);
    doset_add_menu(tmpwin, "role", opt_role, 0);
    doset_add_menu(tmpwin, "race", opt_race, 0);
    doset_add_menu(tmpwin, "gender", opt_gender, 0);
    doset_add_menu(tmpwin, "align", opt_align, 0);

    for (pass = startpass; pass <= endpass; pass++)
        for (i = 0; (name = allopt[i].name) != 0; i++) {
            if (allopt[i].opttyp != CompOpt)
                continue;
            if ((int) allopt[i].setwhere == pass) {
                if (!strcmp(name, "playmode")  || !strcmp(name, "name")
                    || !strcmp(name, "role")   || !strcmp(name, "race")
                    || !strcmp(name, "gender") || !strcmp(name, "align"))
                    continue;
                if ((is_wc_option(name) && !wc_supported(name))
                    || (is_wc2_option(name) && !wc2_supported(name)))
                    continue;

                doset_add_menu(tmpwin, name, i,
                               (pass == set_gameview) ? 0 : indexoffset);
            }
        }

    any = cg.zeroany;
    add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0,
             ATR_NONE, clr, "", MENU_ITEMFLAGS_NONE);
    add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0,
             iflags.menu_headings, clr,
             "Other settings:", MENU_ITEMFLAGS_NONE);

    for (pass = startpass; pass <= endpass; pass++)
        for (i = 0; (name = allopt[i].name) != 0; i++) {
            if (allopt[i].opttyp != OthrOpt)
                continue;
            if ((int) allopt[i].setwhere == pass) {
                if ((is_wc_option(name) && !wc_supported(name))
                    || (is_wc2_option(name) && !wc2_supported(name)))
                    continue;

                doset_add_menu(tmpwin, name, i,
                               (pass == set_gameview) ? 0 : indexoffset);
            }
        }

#ifdef PREFIXES_IN_USE
    any = cg.zeroany;
    add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0,
             ATR_NONE, clr, "", MENU_ITEMFLAGS_NONE);
    add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0,
             iflags.menu_headings, clr,
             "Variable playground locations:", MENU_ITEMFLAGS_NONE);
    for (i = 0; i < PREFIX_COUNT; i++)
        doset_add_menu(tmpwin, fqn_prefix_names[i], -1, 0);
#endif
    end_menu(tmpwin, "Set what options?");
    g.opt_need_redraw = FALSE;
    g.opt_need_glyph_reset = FALSE;
    if ((pick_cnt = select_menu(tmpwin, PICK_ANY, &pick_list)) > 0) {
        /*
         * Walk down the selection list and either invert the booleans
         * or prompt for new values. In most cases, call parseoptions()
         * to take care of options that require special attention, like
         * redraws.
         */
        for (pick_idx = 0; pick_idx < pick_cnt; ++pick_idx) {
            opt_indx = pick_list[pick_idx].item.a_int - 1;
            if (opt_indx == '?') {
                display_file(OPTMENUHELP, FALSE);
                gavehelp = TRUE;
                continue; /* just handled '?'; there might be more picks */
            }
            if (opt_indx < -1)
                opt_indx++; /* -1 offset for select_menu() */
            opt_indx -= indexoffset;
            if (allopt[opt_indx].opttyp == BoolOpt) {
                /* boolean option */
                Sprintf(buf, "%s%s", *allopt[opt_indx].addr ? "!" : "",
                        allopt[opt_indx].name);
                (void) parseoptions(buf, setinitial, fromfile);
            } else {
                /* compound option */
                int k = opt_indx, reslt UNUSED;

                if (allopt[k].has_handler && allopt[k].optfn) {
                    reslt = (*allopt[k].optfn)(allopt[k].idx, do_handler,
                                               FALSE, empty_optstr,
                                               empty_optstr);
                } else {
                    char abuf[BUFSZ];

                    Sprintf(buf, "Set %s to what?", allopt[opt_indx].name);
                    abuf[0] = '\0';
                    getlin(buf, abuf);
                    if (abuf[0] == '\033')
                        continue;
                    Sprintf(buf, "%s:", allopt[opt_indx].name);
                    (void) strncat(eos(buf), abuf,
                                   (sizeof buf - 1 - strlen(buf)));
                    /* pass the buck */
                    (void) parseoptions(buf, setinitial, fromfile);
                }
            }
            if (wc_supported(allopt[opt_indx].name)
                || wc2_supported(allopt[opt_indx].name))
                preference_update(allopt[opt_indx].name);
        }
        free((genericptr_t) pick_list), pick_list = (menu_item *) 0;
    }

    destroy_nhwindow(tmpwin);

    if (pick_cnt == 1 && gavehelp) {
        /* when '?' is only the thing selected, go back and pick all
           over again without it as an available choice second time */
        skiphelp = TRUE;
        gavehelp = FALSE; /* currently True; reset for second pass */
        goto rerun;
    }

    if (g.opt_need_glyph_reset) {
        reset_glyphmap(gm_optionchange);
    }
    if (g.opt_need_redraw) {
        check_gold_symbol();
        reglyph_darkroom();
        (void) doredraw();
    }
    if (g.context.botl || g.context.botlx) {
        bot();
    }
    return ECMD_OK;
}

/* doset('O' command) menu entries for compound options */
static void
doset_add_menu(winid win,          /* window to add to */
               const char *option, /* option name */
               int idx,            /* index in allopt[] */
               int indexoffset)    /* value to add to index in allopt[],
                                      or zero if option cannot be changed */
{
    const char *value = "unknown"; /* current value */
    char buf[BUFSZ], buf2[BUFSZ];
    anything any;
    int i = idx, reslt = optn_err;
#ifdef PREFIXES_IN_USE
    int j;
#endif
    int clr = 0;

    buf2[0] = '\0';  /* per opt functs may not guarantee this, so do it */
    any = cg.zeroany;
    if (i >= 0 && i < OPTCOUNT && allopt[i].name && allopt[i].optfn) {
        any.a_int = (indexoffset == 0) ? 0 : i + 1 + indexoffset;
        if (allopt[i].optfn)
            reslt = (*allopt[i].optfn)(allopt[i].idx, get_val,
                                       FALSE, buf2, empty_optstr);
        if (reslt == optn_ok && buf2[0])
            value = (const char *) buf2;
    } else {
        /* We are trying to add an option not found in allopt[].
           This is almost certainly bad, but we'll let it through anyway
           (with a zero value, so it can't be selected). */
        any.a_int = 0;
#ifdef PREFIXES_IN_USE
        for (j = 0; j < PREFIX_COUNT; ++j)
            if (!strcmp(option, fqn_prefix_names[j]) && g.fqn_prefix[j])
                Sprintf(buf2, "%s", g.fqn_prefix[j]);
#endif
        if (!buf2[0])
            Strcpy(buf2, "unknown");
        value = (const char *) buf2;
    }

    /* "    " replaces "a - " -- assumes menus follow that style */
    if (!iflags.menu_tab_sep)
        Sprintf(buf, fmtstr_doset, any.a_int ? "" : "    ", option,
                value);
    else
        Sprintf(buf, fmtstr_doset_tab, option, value);
    add_menu(win, &nul_glyphinfo, &any, 0, 0,
             ATR_NONE, clr, buf, MENU_ITEMFLAGS_NONE);
}


/* display keys for menu actions; used by cmd.c '?i' and pager.c '?k' */
void
show_menu_controls(winid win, boolean dolist)
{
    struct xtra_cntrls {
        const char *key, *desc;
    };
    static const struct xtra_cntrls hardcoded[] = {
        { "Return", "Accept current choice(s) and dismiss menu" },
        { "Enter",  "Same as Return" },
        { "Space",  "If not on last page, advance one page;" },
        { "     ",  "when on last page, treat like Return" },
        { "Escape", "Cancel menu without making any choice(s)" },
        { (char *) 0, (char *) 0}
    };
    static const char mc_fmt[] = "%8s     %-6s %s",
                      mc_altfmt[] = "%9s  %-6s %s";
    char buf[BUFSZ];
    const char *fmt, *arg;
    const struct xtra_cntrls *xcp;
    boolean has_menu_shift = wc2_supported("menu_shift");

    /*
     * Relies on spaces to line things up in columns, so must be rendered
     * with a fixed-width font or will look dreadful.
     */

    putstr(win, 0, "Menu control keys:");
    if (dolist) { /* key bindings help: '?i' */
        int i;
        char ch;

        fmt = "%-7s %s";
        for (i = 0; default_menu_cmd_info[i].desc; i++) {
            ch = default_menu_cmd_info[i].cmd;
            if ((ch == MENU_SHIFT_RIGHT
                 || ch == MENU_SHIFT_LEFT) && !has_menu_shift)
                continue;
            Sprintf(buf, fmt,
                    visctrl(get_menu_cmd_key(ch)),
                    default_menu_cmd_info[i].desc);
            putstr(win, 0, buf);
        }
        /* no separator before hardcoded */
        fmt = "%s%-7s %s"; /* extra specifier to absorb 'arg' */
        arg = ""; /* no extra prefix for 'dolist' */
    } else { /* menu controls help: '?k' */
        putstr(win, 0, "");
        Sprintf(buf, mc_altfmt, "", "Whole", "Current");
        putstr(win, 0, buf);
        Sprintf(buf, mc_altfmt, "", " Menu", " Page");
        putstr(win, 0, buf);
        Sprintf(buf, mc_fmt, "Select",
                visctrl(get_menu_cmd_key(MENU_SELECT_ALL)),
                visctrl(get_menu_cmd_key(MENU_SELECT_PAGE)));
        putstr(win, 0, buf);
        Sprintf(buf, mc_fmt, "Invert",
                visctrl(get_menu_cmd_key(MENU_INVERT_ALL)),
                visctrl(get_menu_cmd_key(MENU_INVERT_PAGE)));
        putstr(win, 0, buf);
        Sprintf(buf, mc_fmt, "Deselect",
                visctrl(get_menu_cmd_key(MENU_UNSELECT_ALL)),
                visctrl(get_menu_cmd_key(MENU_UNSELECT_PAGE)));
        putstr(win, 0, buf);
        putstr(win, 0, "");
        Sprintf(buf, mc_fmt, "Go to",
                visctrl(get_menu_cmd_key(MENU_NEXT_PAGE)),
                "Next page");
        putstr(win, 0, buf);
        Sprintf(buf, mc_fmt, "",
                visctrl(get_menu_cmd_key(MENU_PREVIOUS_PAGE)),
                "Previous page");
        putstr(win, 0, buf);
        Sprintf(buf, mc_fmt, "",
                visctrl(get_menu_cmd_key(MENU_FIRST_PAGE)),
                "First page");
        putstr(win, 0, buf);
        Sprintf(buf, mc_fmt, "",
                visctrl(get_menu_cmd_key(MENU_LAST_PAGE)),
                "Last page");
        putstr(win, 0, buf);
        if (has_menu_shift) {
            Sprintf(buf, mc_fmt, "Pan view",
                    visctrl(get_menu_cmd_key(MENU_SHIFT_RIGHT)),
                    "Right (perm_invent only)");
            putstr(win, 0, buf);
            Sprintf(buf, mc_fmt, "",
                    visctrl(get_menu_cmd_key(MENU_SHIFT_LEFT)),
                    "Left");
            putstr(win, 0, buf);
        }
        putstr(win, 0, "");
        Sprintf(buf, mc_fmt, "Search",
                visctrl(get_menu_cmd_key(MENU_SEARCH)),
                "Exter a target string and invert all matching entries");
        putstr(win, 0, buf);
        /* separator before hardcoded */
        putstr(win, 0, "");
        fmt = "%9s  %-8s %s";
        arg = "Other "; /* prefix for first hardcoded[] entry, then reset */
    }
    for (xcp = hardcoded; xcp->key; ++xcp) {
        Sprintf(buf, fmt, arg, xcp->key, xcp->desc);
        putstr(win, 0, buf);
        arg = "";
    }
}

RESTORE_WARNING_FORMAT_NONLITERAL

static int
count_cond(void)
{
    int i, cnt = 0;

    for (i = 0; i < CONDITION_COUNT; ++i) {
        if (condtests[i].enabled)
            cnt++;
    }
    return cnt;
}

static int
count_apes(void)
{
    int numapes = 0;
    struct autopickup_exception *ape = g.apelist;

    while (ape) {
      numapes++;
      ape = ape->next;
    }

    return numapes;
}

DISABLE_WARNING_FORMAT_NONLITERAL

/* common to msg-types, menu-colors, autopickup-exceptions */
static int
handle_add_list_remove(const char *optname, int numtotal)
{
    winid tmpwin;
    anything any;
    int i, pick_cnt, opt_idx;
    menu_item *pick_list = (menu_item *) 0;
    static const struct action {
        char letr;
        const char *desc;
    } action_titles[] = {
        { 'a', "add new %s" },         /* [0] */
        { 'l', "list %s" },            /* [1] */
        { 'r', "remove existing %s" }, /* [2] */
        { 'x', "exit this menu" },     /* [3] */
    };
    int clr = 0;

    opt_idx = 0;
    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);
    any = cg.zeroany;
    for (i = 0; i < SIZE(action_titles); i++) {
        char tmpbuf[BUFSZ];

        any.a_int++;
        /* omit list and remove if there aren't any yet */
        if (!numtotal && (i == 1 || i == 2))
            continue;
        Sprintf(tmpbuf, action_titles[i].desc,
                (i == 1) ? makeplural(optname) : optname);
        add_menu(tmpwin, &nul_glyphinfo,&any, action_titles[i].letr,
                 0, ATR_NONE, clr, tmpbuf,
                 (i == 3) ? MENU_ITEMFLAGS_SELECTED : MENU_ITEMFLAGS_NONE);
    }
    end_menu(tmpwin, "Do what?");
    if ((pick_cnt = select_menu(tmpwin, PICK_ONE, &pick_list)) > 0) {
        opt_idx = pick_list[0].item.a_int - 1;
        if (pick_cnt > 1 && opt_idx == 3)
            opt_idx = pick_list[1].item.a_int - 1;
        free((genericptr_t) pick_list);
    } else
        opt_idx = 3; /* none selected, exit menu */
    destroy_nhwindow(tmpwin);
    return opt_idx;
}

RESTORE_WARNING_FORMAT_NONLITERAL

int
dotogglepickup(void)
{
    char buf[BUFSZ], ocl[MAXOCLASSES + 1];

    flags.pickup = !flags.pickup;
    if (flags.pickup) {
        oc_to_str(flags.pickup_types, ocl);
        Sprintf(buf, "ON, for %s objects%s", ocl[0] ? ocl : "all",
                (g.apelist)
                    ? ((count_apes() == 1)
                           ? ", with one exception"
                           : ", with some exceptions")
                    : "");
    } else {
        Strcpy(buf, "OFF");
    }
    pline("Autopickup: %s.", buf);
    return ECMD_OK;
}

int
add_autopickup_exception(const char *mapping)
{
    static const char
        APE_regex_error[] = "regex error in AUTOPICKUP_EXCEPTION",
        APE_syntax_error[] = "syntax error in AUTOPICKUP_EXCEPTION";

    struct autopickup_exception *ape;
    char text[256], end;
    int n;
    boolean grab = FALSE;

    /* scan length limit used to be 255, but smaller size allows the
       quoted value to fit within BUFSZ, simplifying formatting elsewhere;
       this used to ignore the possibility of trailing junk but now checks
       for it, accepting whitespace but rejecting anything else unless it
       starts with '#" for a comment */
    end = '\0';
    if ((n = sscanf(mapping, "\"<%253[^\"]\" %c", text, &end)) == 1
        || (n == 2 && end == '#')) {
        grab = TRUE;
    } else if ((n = sscanf(mapping, "\">%253[^\"]\" %c", text, &end)) == 1
               || (n = sscanf(mapping, "\"%253[^\"]\" %c", text, &end)) == 1
               || (n == 2 && end == '#')) {
        grab = FALSE;
    } else {
        config_error_add("%s", APE_syntax_error);
        return 0;
    }

    ape = (struct autopickup_exception *) alloc(sizeof *ape);
    ape->regex = regex_init();
    if (!regex_compile(text, ape->regex)) {
        char errbuf[BUFSZ];
        char *re_error_desc = regex_error_desc(ape->regex, errbuf);

        /* free first in case reason for failure was insufficient memory */
        regex_free(ape->regex);
        free((genericptr_t) ape);
        config_error_add("%s: %s", APE_regex_error, re_error_desc);
        return 0;
    }
    ape->pattern = dupstr(text);
    ape->grab = grab;
    ape->next = g.apelist;
    g.apelist = ape;
    return 1;
}

static void
remove_autopickup_exception(struct autopickup_exception *whichape)
{
    struct autopickup_exception *ape, *freeape, *prev = 0;

    for (ape = g.apelist; ape;) {
        if (ape == whichape) {
            freeape = ape;
            ape = ape->next;
            if (prev)
                prev->next = ape;
            else
                g.apelist = ape;
            regex_free(freeape->regex);
            free((genericptr_t) freeape->pattern);
            free((genericptr_t) freeape);
        } else {
            prev = ape;
            ape = ape->next;
        }
    }
}

void
free_autopickup_exceptions(void)
{
    struct autopickup_exception *ape;

    while ((ape = g.apelist) != 0) {
        free((genericptr_t) ape->pattern);
        regex_free(ape->regex);
        g.apelist = ape->next;
        free((genericptr_t) ape);
    }
}

int
sym_val(const char *strval) /* up to 4*BUFSZ-1 long; only first few
                               chars matter */
{
    char buf[QBUFSZ], tmp[QBUFSZ]; /* to hold trucated copy of 'strval' */

    buf[0] = '\0';
    if (!strval[0] || !strval[1]) { /* empty, or single character */
        /* if single char is space or tab, leave buf[0]=='\0' */
        if (!isspace((uchar) strval[0]))
            buf[0] = strval[0];
    } else if (strval[0] == '\'') { /* single quote */
        /* simple matching single quote; we know strval[1] isn't '\0' */
        if (strval[2] == '\'' && !strval[3]) {
            /* accepts '\' as backslash and ''' as single quote */
            buf[0] = strval[1];

        /* if backslash, handle single or double quote or second backslash */
        } else if (strval[1] == '\\' && strval[2] && strval[3] == '\''
            && index("'\"\\", strval[2]) && !strval[4]) {
            buf[0] = strval[2];

        /* not simple quote or basic backslash;
           strip closing quote and let escapes() deal with it */
        } else {
            char *p;

            /* +1: skip opening single quote */
            (void) strncpy(tmp, strval + 1, sizeof tmp - 1);
            tmp[sizeof tmp - 1] = '\0';
            if ((p = rindex(tmp, '\'')) != 0) {
                *p = '\0';
                escapes(tmp, buf);
            } /* else buf[0] stays '\0' */
        }
    } else { /* not lone char nor single quote */
        (void) strncpy(tmp, strval, sizeof tmp - 1);
        tmp[sizeof tmp - 1] = '\0';
        escapes(tmp, buf);
    }

    return (int) *buf;
}

/* data for option_help() */
static const char *opt_intro[] = {
    "",
    "                 NetHack Options Help:", "",
#define CONFIG_SLOT 3 /* fill in next value at run-time */
    (char *) 0,
#if !defined(MICRO) && !defined(MAC)
    "or use `NETHACKOPTIONS=\"<options>\"' in your environment",
#endif
    "(<options> is a list of options separated by commas)",
#ifdef VMS
    "-- for example, $ DEFINE NETHACKOPTIONS \"noautopickup,fruit:kumquat\"",
#endif
    "or press \"O\" while playing and use the menu.",
    "",
 "Boolean options (which can be negated by prefixing them with '!' or \"no\"):",
    (char *) 0
};

static const char *const opt_epilog[] = {
    "",
    "Some of the options can only be set before the game is started;",
    "those items will not be selectable in the 'O' command's menu.",
    "Some options are stored in a game's save file, and will keep saved",
    "values when restoring that game even if you have updated your config-",
    "uration file to change them.  Such changes will matter for new games.",
    "The \"other settings\" can be set with 'O', but when set within the",
    "configuration file they use their own directives rather than OPTIONS.",
    "See NetHack's \"Guidebook\" for details.",
    (char *) 0
};

void
option_help(void)
{
    char buf[BUFSZ], buf2[BUFSZ];
    const char *optname;
    register int i;
    winid datawin;

    datawin = create_nhwindow(NHW_TEXT);
    Sprintf(buf, "Set options as OPTIONS=<options> in %s", configfile);
    opt_intro[CONFIG_SLOT] = (const char *) buf;
    for (i = 0; opt_intro[i]; i++)
        putstr(datawin, 0, opt_intro[i]);

    /* Boolean options */
    for (i = 0; allopt[i].name; i++) {
        if ((allopt[i].opttyp != BoolOpt || !allopt[i].addr)
            || (allopt[i].setwhere == set_wizonly && !wizard))
            continue;
        optname = allopt[i].name;
        if ((is_wc_option(optname) && !wc_supported(optname))
            || (is_wc2_option(optname) && !wc2_supported(optname)))
            continue;
        next_opt(datawin, optname);
    }
    next_opt(datawin, "");

    /* Compound options */
    putstr(datawin, 0, "Compound options:");
    for (i = 0; allopt[i].name; i++) {
        if (allopt[i].opttyp != CompOpt
            || (allopt[i].setwhere == set_wizonly && !wizard))
            continue;
        optname = allopt[i].name;
        if ((is_wc_option(optname) && !wc_supported(optname))
            || (is_wc2_option(optname) && !wc2_supported(optname)))
            continue;
        Sprintf(buf2, "`%s'", optname);
        Snprintf(buf, sizeof(buf), "%-20s - %s%c", buf2, allopt[i].descr,
                 allopt[i + 1].name ? ',' : '.');
        putstr(datawin, 0, buf);
    }
    putstr(datawin, 0, "");

    /* Compound options */
    putstr(datawin, 0, "Other settings:");
    for (i = 0; allopt[i].name; i++) {
        if (allopt[i].opttyp != OthrOpt)
            continue;
        Sprintf(buf, " %s", allopt[i].name);
        putstr(datawin, 0, buf);
    }

    putstr(datawin, 0, "");

    for (i = 0; opt_epilog[i]; i++)
        putstr(datawin, 0, opt_epilog[i]);

    /*
     * TODO:
     *  briefly describe interface-specific option-like settings for
     *  the currently active interface:
     *    X11 uses X-specific "application defaults" from NetHack.ad;
     *    Qt has menu accessible "game -> Qt settings" (non-OSX) or
     *      "nethack -> Preferences" (OSX) to maintain a few options
     *      (font size, map tile size, paperdoll show/hide flag and
     *      tile size) which persist across games;
     *    Windows GUI also has some port-specific menus;
     *    tty and curses: anything?
     *  Best done via a new windowprocs function rather than plugging
     *  in details here.
     *
     * Maybe:
     *  switch from text window to pick-none menu so that user can
     *  scroll back up.  (Not necessary for Qt where text windows are
     *  already scrollable.)
     */

    display_nhwindow(datawin, FALSE);
    destroy_nhwindow(datawin);
    return;
}

/*
 * prints the next boolean option, on the same line if possible, on a new
 * line if not. End with next_opt("").
 */
void
next_opt(winid datawin, const char *str)
{
    static char *buf = 0;
    int i;
    char *s;

    if (!buf)
        *(buf = (char *) alloc(COLBUFSZ)) = '\0';

    if (!*str) {
        s = eos(buf);
        if (s > &buf[1] && s[-2] == ',')
            Strcpy(s - 2, "."); /* replace last ", " */
        i = COLNO;              /* (greater than COLNO - 2) */
    } else {
        i = Strlen(buf) + Strlen(str) + 2;
    }

    if (i > COLNO - 2) { /* rule of thumb */
        putstr(datawin, 0, buf);
        buf[0] = 0;
    }
    if (*str) {
        Strcat(buf, str);
        Strcat(buf, ", ");
    } else {
        putstr(datawin, 0, str);
        free((genericptr_t) buf), buf = 0;
    }
    return;
}

/*
 * This is a somewhat generic menu for taking a list of NetHack style
 * class choices and presenting them via a description
 * rather than the traditional NetHack characters.
 * (Benefits users whose first exposure to NetHack is via tiles).
 *
 * prompt
 *           The title at the top of the menu.
 *
 * category: 0 = monster class
 *           1 = object  class
 *
 * way
 *           FALSE = PICK_ONE, TRUE = PICK_ANY
 *
 * class_list
 *           a null terminated string containing the list of choices.
 *
 * class_selection
 *           a null terminated string containing the selected characters.
 *
 * Returns number selected.
 */
int
choose_classes_menu(const char *prompt,
                    int category,
                    boolean way,
                    char *class_list,
                    char *class_select)
{
    menu_item *pick_list = (menu_item *) 0;
    winid win;
    anything any;
    char buf[BUFSZ];
    int i, n;
    int ret;
    int next_accelerator, accelerator;
    int clr = 0;

    if (class_list == (char *) 0 || class_select == (char *) 0)
        return 0;
    accelerator = 0;
    next_accelerator = 'a';
    any = cg.zeroany;
    win = create_nhwindow(NHW_MENU);
    start_menu(win, MENU_BEHAVE_STANDARD);
    while (*class_list) {
        const char *text;
        boolean selected;

        text = (char *) 0;
        selected = FALSE;
        switch (category) {
        case 0:
            text = def_monsyms[def_char_to_monclass(*class_list)].explain;
            accelerator = *class_list;
            Sprintf(buf, "%s", text);
            break;
        case 1:
            text = def_oc_syms[def_char_to_objclass(*class_list)].explain;
            accelerator = next_accelerator;
            Sprintf(buf, "%c  %s", *class_list, text);
            break;
        default:
            impossible("choose_classes_menu: invalid category %d", category);
        }
        if (way && *class_select) { /* Selections there already */
            if (index(class_select, *class_list)) {
                selected = TRUE;
            }
        }
        any.a_int = *class_list;
        add_menu(win, &nul_glyphinfo, &any, accelerator,
                 category ? *class_list : 0, ATR_NONE, clr, buf,
                 selected ? MENU_ITEMFLAGS_SELECTED : MENU_ITEMFLAGS_NONE);
        ++class_list;
        if (category > 0) {
            ++next_accelerator;
            if (next_accelerator == ('z' + 1))
                next_accelerator = 'A';
            if (next_accelerator == ('Z' + 1))
                break;
        }
    }
    if (category == 1 && next_accelerator <= 'z') {
        /* for objects, add "A - ' '  all classes", after a separator */
        any = cg.zeroany;
        add_menu(win, &nul_glyphinfo, &any, 0, 0,
                 ATR_NONE, clr, "", MENU_ITEMFLAGS_NONE);
        any.a_int = (int) ' ';
        Sprintf(buf, "%c  %s", (char) any.a_int, "all classes of objects");
        /* we won't preselect this even if the incoming list is empty;
           having it selected means that it would have to be explicitly
           de-selected in order to select anything else */
        add_menu(win, &nul_glyphinfo, &any, 'A', 0,
                 ATR_NONE, clr, buf, MENU_ITEMFLAGS_NONE);
    }
    end_menu(win, prompt);
    n = select_menu(win, way ? PICK_ANY : PICK_ONE, &pick_list);
    destroy_nhwindow(win);
    if (n > 0) {
        if (category == 1) {
            /* for object classes, first check for 'all'; it means 'use
               a blank list' rather than 'collect every possible choice' */
            for (i = 0; i < n; ++i)
                if (pick_list[i].item.a_int == ' ') {
                    pick_list[0].item.a_int = ' ';
                    n = 1; /* return 1; also an implicit 'break;' */
                }
        }
        for (i = 0; i < n; ++i)
            *class_select++ = (char) pick_list[i].item.a_int;
        free((genericptr_t) pick_list);
        ret = n;
    } else if (n == -1) {
        class_select = eos(class_select);
        ret = -1;
    } else
        ret = 0;
    *class_select = '\0';
    return ret;
}

static struct wc_Opt wc_options[] = {
    { "ascii_map", WC_ASCII_MAP },
    { "color", WC_COLOR },
    { "eight_bit_tty", WC_EIGHT_BIT_IN },
    { "hilite_pet", WC_HILITE_PET },
    { "perm_invent", WC_PERM_INVENT },
    { "popup_dialog", WC_POPUP_DIALOG },
    { "player_selection", WC_PLAYER_SELECTION },
    { "preload_tiles", WC_PRELOAD_TILES },
    { "tiled_map", WC_TILED_MAP },
    { "tile_file", WC_TILE_FILE },
    { "tile_width", WC_TILE_WIDTH },
    { "tile_height", WC_TILE_HEIGHT },
    { "align_message", WC_ALIGN_MESSAGE },
    { "align_status", WC_ALIGN_STATUS },
    { "font_map", WC_FONT_MAP },
    { "font_menu", WC_FONT_MENU },
    { "font_message", WC_FONT_MESSAGE },
    { "font_size_map", WC_FONTSIZ_MAP },
    { "font_size_menu", WC_FONTSIZ_MENU },
    { "font_size_message", WC_FONTSIZ_MESSAGE },
    { "font_size_status", WC_FONTSIZ_STATUS },
    { "font_size_text", WC_FONTSIZ_TEXT },
    { "font_status", WC_FONT_STATUS },
    { "font_text", WC_FONT_TEXT },
    { "map_mode", WC_MAP_MODE },
    { "scroll_amount", WC_SCROLL_AMOUNT },
    { "scroll_margin", WC_SCROLL_MARGIN },
    { "splash_screen", WC_SPLASH_SCREEN },
    { "use_inverse", WC_INVERSE },
    { "vary_msgcount", WC_VARY_MSGCOUNT },
    { "windowcolors", WC_WINDOWCOLORS },
    { "mouse_support", WC_MOUSE_SUPPORT },
    { (char *) 0, 0L }
};
static struct wc_Opt wc2_options[] = {
    { "fullscreen", WC2_FULLSCREEN },
    { "guicolor", WC2_GUICOLOR },
    { "hilite_status", WC2_HILITE_STATUS },
    { "hitpointbar", WC2_HITPOINTBAR },
    { "menu_shift", WC2_MENU_SHIFT },
    { "petattr", WC2_PETATTR },
    { "softkeyboard", WC2_SOFTKEYBOARD },
    /* name shown in 'O' menu is different */
    { "status hilite rules", WC2_HILITE_STATUS },
    /* statushilites doesn't have its own bit */
    { "statushilites", WC2_HILITE_STATUS },
    { "statuslines", WC2_STATUSLINES },
    { "term_cols", WC2_TERM_SIZE },
    { "term_rows", WC2_TERM_SIZE },
    { "use_darkgray", WC2_DARKGRAY },
    { "windowborders", WC2_WINDOWBORDERS },
    { "wraptext", WC2_WRAPTEXT },
    { (char *) 0, 0L }
};

/*
 * If a port wants to change or ensure that the set_in_sysconf,
 * set_in_config, set_gameview, or set_in_game status of an option is
 * correct (for controlling its display in the option menu) call
 * set_option_mod_status()
 * with the appropriate second argument.
 */
void
set_option_mod_status(const char *optnam, int status)
{
    int k;

    if (SET__IS_VALUE_VALID(status)) {
        impossible("set_option_mod_status: status out of range %d.", status);
        return;
    }
    for (k = 0; allopt[k].name; k++) {
        if (str_start_is(allopt[k].name, optnam, TRUE)) {
            allopt[k].setwhere = status;
            return;
        }
    }
}

/*
 * You can set several wc_options in one call to
 * set_wc_option_mod_status() by setting
 * the appropriate bits for each option that you
 * are setting in the optmask argument
 * prior to calling.
 *    example: set_wc_option_mod_status(WC_COLOR|WC_SCROLL_MARGIN,
 * set_in_game);
 */
void
set_wc_option_mod_status(unsigned long optmask, int status)
{
    int k = 0;

    if (SET__IS_VALUE_VALID(status)) {
        impossible("set_wc_option_mod_status: status out of range %d.",
                   status);
        return;
    }
    while (wc_options[k].wc_name) {
        if (optmask & wc_options[k].wc_bit) {
            set_option_mod_status(wc_options[k].wc_name, status);
        }
        k++;
    }
}

static boolean
is_wc_option(const char *optnam)
{
    int k = 0;

    while (wc_options[k].wc_name) {
        if (strcmp(wc_options[k].wc_name, optnam) == 0)
            return TRUE;
        k++;
    }
    return FALSE;
}

static boolean
wc_supported(const char *optnam)
{
    int k;

    for (k = 0; wc_options[k].wc_name; ++k) {
        if (!strcmp(wc_options[k].wc_name, optnam))
            return (windowprocs.wincap & wc_options[k].wc_bit) ? TRUE : FALSE;
    }
    return FALSE;
}

/*
 * You can set several wc2_options in one call to
 * set_wc2_option_mod_status() by setting
 * the appropriate bits for each option that you
 * are setting in the optmask argument
 * prior to calling.
 *    example:
 * set_wc2_option_mod_status(WC2_FULLSCREEN|WC2_SOFTKEYBOARD|WC2_WRAPTEXT,
 * set_in_config);
 */

void
set_wc2_option_mod_status(unsigned long optmask, int status)
{
    int k = 0;

    if (SET__IS_VALUE_VALID(status)) {
        impossible("set_wc2_option_mod_status: status out of range %d.",
                   status);
        return;
    }
    while (wc2_options[k].wc_name) {
        if (optmask & wc2_options[k].wc_bit) {
            set_option_mod_status(wc2_options[k].wc_name, status);
        }
        k++;
    }
}

static boolean
is_wc2_option(const char *optnam)
{
    int k = 0;

    while (wc2_options[k].wc_name) {
        if (strcmp(wc2_options[k].wc_name, optnam) == 0)
            return TRUE;
        k++;
    }
    return FALSE;
}

static boolean
wc2_supported(const char *optnam)
{
    int k;

    for (k = 0; wc2_options[k].wc_name; ++k) {
        if (!strcmp(wc2_options[k].wc_name, optnam))
            return (windowprocs.wincap2 & wc2_options[k].wc_bit) ? TRUE
                                                                 : FALSE;
    }
    return FALSE;
}

static void
wc_set_font_name(int opttype, char *fontname)
{
    char **fn = (char **) 0;

    if (!fontname)
        return;
    switch (opttype) {
    case MAP_OPTION:
        fn = &iflags.wc_font_map;
        break;
    case MESSAGE_OPTION:
        fn = &iflags.wc_font_message;
        break;
    case TEXT_OPTION:
        fn = &iflags.wc_font_text;
        break;
    case MENU_OPTION:
        fn = &iflags.wc_font_menu;
        break;
    case STATUS_OPTION:
        fn = &iflags.wc_font_status;
        break;
    default:
        return;
    }
    if (fn) {
        if (*fn)
            free((genericptr_t) *fn);
        *fn = dupstr(fontname);
    }
    return;
}

static int
wc_set_window_colors(char *op)
{
    /* syntax:
     *  menu white/black message green/yellow status white/blue text
     * white/black
     */
    int j;
    char buf[BUFSZ];
    char *wn, *tfg, *tbg, *newop;
    static const char *const wnames[] = {
        "menu", "message", "status", "text"
    };
    static const char *const shortnames[] = { "mnu", "msg", "sts", "txt" };
    static char **fgp[] = { &iflags.wc_foregrnd_menu,
                            &iflags.wc_foregrnd_message,
                            &iflags.wc_foregrnd_status,
                            &iflags.wc_foregrnd_text };
    static char **bgp[] = { &iflags.wc_backgrnd_menu,
                            &iflags.wc_backgrnd_message,
                            &iflags.wc_backgrnd_status,
                            &iflags.wc_backgrnd_text };

    Strcpy(buf, op);
    newop = mungspaces(buf);
    while (*newop) {
        wn = tfg = tbg = (char *) 0;

        /* until first non-space in case there's leading spaces - before
           colorname*/
        if (*newop == ' ')
            newop++;
        if (!*newop)
            return 0;
        wn = newop;

        /* until first space - colorname*/
        while (*newop && *newop != ' ')
            newop++;
        if (!*newop)
            return 0;
        *newop++ = '\0';

        /* until first non-space - before foreground*/
        if (*newop == ' ')
            newop++;
        if (!*newop)
            return 0;
        tfg = newop;

        /* until slash - foreground */
        while (*newop && *newop != '/')
            newop++;
        if (!*newop)
            return 0;
        *newop++ = '\0';

        /* until first non-space (in case there's leading space after slash) -
         * before background */
        if (*newop == ' ')
            newop++;
        if (!*newop)
            return 0;
        tbg = newop;

        /* until first space - background */
        while (*newop && *newop != ' ')
            newop++;
        if (*newop)
            *newop++ = '\0';

        for (j = 0; j < 4; ++j) {
            if (!strcmpi(wn, wnames[j]) || !strcmpi(wn, shortnames[j])) {
                if (!strstri(tfg, " ")) {
                    if (*fgp[j])
                        free((genericptr_t) *fgp[j]);
                    *fgp[j] = dupstr(tfg);
                }
                if (!strstri(tbg, " ")) {
                    if (*bgp[j])
                        free((genericptr_t) *bgp[j]);
                    *bgp[j] = dupstr(tbg);
                }
                break;
            }
        }
    }
    return 1;
}

/* set up for wizard mode if player or save file has requested to it;
   called from port-specific startup code to handle `nethack -D' or
   OPTIONS=playmode:debug, or from dorecover()'s restgamestate() if
   restoring a game which was saved in wizard mode */
void
set_playmode(void)
{
    if (wizard) {
        if (authorize_wizard_mode())
            g.plnamelen = (int) strlen(strcpy(g.plname, "wizard"));
        else
            wizard = FALSE; /* not allowed or not available */
        /* force explore mode if we didn't make it into wizard mode */
        discover = !wizard;
        iflags.deferred_X = FALSE;
    }
    /* don't need to do anything special for explore mode or normal play */
}
void
enhance_menu_text(
    char *buf,
    size_t sz,
    int whichpass UNUSED,
    boolean *bool_p,
    struct allopt_t *thisopt)
{
    size_t nowsz, availsz;

    if (!buf)
        return;
    nowsz = strlen(buf) + 1;
    availsz = sz - nowsz;

#ifdef TTY_PERM_INVENT
    if (bool_p == &iflags.perm_invent && WINDOWPORT(tty)) {
        if (thisopt->setwhere == set_gameview)
            Snprintf(eos(buf), availsz, " *terminal size is too small");
    }
#else
    nhUse(availsz);
    nhUse(bool_p);
    nhUse(thisopt);
#endif
    return;
}


#endif /* OPTION_LISTS_ONLY */

/*options.c*/

