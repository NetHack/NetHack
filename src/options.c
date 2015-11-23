/* NetHack 3.6	options.c	$NHDT-Date: 1448241657 2015/11/23 01:20:57 $  $NHDT-Branch: master $:$NHDT-Revision: 1.243 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifdef OPTION_LISTS_ONLY /* (AMIGA) external program for opt lists */
#include "config.h"
#include "objclass.h"
#include "flag.h"
NEARDATA struct flag flags; /* provide linkage */
#ifdef SYSFLAGS
NEARDATA struct sysflag sysflags; /* provide linkage */
#endif
NEARDATA struct instance_flags iflags; /* provide linkage */
#define static
#else
#include "hack.h"
#include "tcap.h"
#include <ctype.h>
#endif

#define BACKWARD_COMPAT
#define WINTYPELEN 16

#ifdef DEFAULT_WC_TILED_MAP
#define PREFER_TILED TRUE
#else
#define PREFER_TILED FALSE
#endif

#define MESSAGE_OPTION 1
#define STATUS_OPTION 2
#define MAP_OPTION 3
#define MENU_OPTION 4
#define TEXT_OPTION 5

#define PILE_LIMIT_DFLT 5

/*
 *  NOTE:  If you add (or delete) an option, please update the short
 *  options help (option_help()), the long options help (dat/opthelp),
 *  and the current options setting display function (doset()),
 *  and also the Guidebooks.
 *
 *  The order matters.  If an option is a an initial substring of another
 *  option (e.g. time and timed_delay) the shorter one must come first.
 */

static struct Bool_Opt {
    const char *name;
    boolean *addr, initvalue;
    int optflags;
} boolopt[] = {
    { "acoustics", &flags.acoustics, TRUE, SET_IN_GAME },
#if defined(SYSFLAGS) && defined(AMIGA)
    /* Amiga altmeta causes Alt+key to be converted into Meta+key by
       low level nethack code; on by default, can be toggled off if
       Alt+key is needed for some ASCII chars on non-ASCII keyboard */
    { "altmeta", &sysflags.altmeta, TRUE, DISP_IN_GAME },
#else
#ifdef ALTMETA
    /* non-Amiga altmeta causes nethack's top level command loop to treat
       two character sequence "ESC c" as M-c, for terminals or emulators
       which send "ESC c" when Alt+c is pressed; off by default, enabling
       this can potentially make trouble if user types ESC when nethack
       is honoring this conversion request (primarily after starting a
       count prefix prior to a command and then deciding to cancel it) */
    { "altmeta", &iflags.altmeta, FALSE, SET_IN_GAME },
#else
    { "altmeta", (boolean *) 0, TRUE, DISP_IN_GAME },
#endif
#endif
    { "ascii_map", &iflags.wc_ascii_map, !PREFER_TILED, SET_IN_GAME }, /*WC*/
#if defined(SYSFLAGS) && defined(MFLOPPY)
    { "asksavedisk", &sysflags.asksavedisk, FALSE, SET_IN_GAME },
#else
    { "asksavedisk", (boolean *) 0, FALSE, SET_IN_FILE },
#endif
    { "autodig", &flags.autodig, FALSE, SET_IN_GAME },
    { "autoopen", &flags.autoopen, TRUE, SET_IN_GAME },
    { "autopickup", &flags.pickup, TRUE, SET_IN_GAME },
    { "autoquiver", &flags.autoquiver, FALSE, SET_IN_GAME },
#if defined(MICRO) && !defined(AMIGA)
    { "BIOS", &iflags.BIOS, FALSE, SET_IN_FILE },
#else
    { "BIOS", (boolean *) 0, FALSE, SET_IN_FILE },
#endif
    { "blind", &u.uroleplay.blind, FALSE, DISP_IN_GAME },
    { "bones", &flags.bones, TRUE, SET_IN_FILE },
#ifdef INSURANCE
    { "checkpoint", &flags.ins_chkpt, TRUE, SET_IN_GAME },
#else
    { "checkpoint", (boolean *) 0, FALSE, SET_IN_FILE },
#endif
#ifdef MFLOPPY
    { "checkspace", &iflags.checkspace, TRUE, SET_IN_GAME },
#else
    { "checkspace", (boolean *) 0, FALSE, SET_IN_FILE },
#endif
    { "clicklook", &iflags.clicklook, FALSE, SET_IN_GAME },
    { "cmdassist", &iflags.cmdassist, TRUE, SET_IN_GAME },
#if defined(MICRO) || defined(WIN32)
    { "color", &iflags.wc_color, TRUE, SET_IN_GAME }, /*WC*/
#else /* systems that support multiple terminals, many monochrome */
    { "color", &iflags.wc_color, FALSE, SET_IN_GAME }, /*WC*/
#endif
    { "confirm", &flags.confirm, TRUE, SET_IN_GAME },
    { "dark_room", &flags.dark_room, TRUE, SET_IN_GAME },
    { "eight_bit_tty", &iflags.wc_eight_bit_input, FALSE,
      SET_IN_GAME }, /*WC*/
#ifdef TTY_GRAPHICS
    { "extmenu", &iflags.extmenu, FALSE, SET_IN_GAME },
#else
    { "extmenu", (boolean *) 0, FALSE, SET_IN_FILE },
#endif
#ifdef OPT_DISPMAP
    { "fast_map", &flags.fast_map, TRUE, SET_IN_GAME },
#else
    { "fast_map", (boolean *) 0, TRUE, SET_IN_FILE },
#endif
    { "female", &flags.female, FALSE, DISP_IN_GAME },
    { "fixinv", &flags.invlet_constant, TRUE, SET_IN_GAME },
#if defined(SYSFLAGS) && defined(AMIFLUSH)
    { "flush", &sysflags.amiflush, FALSE, SET_IN_GAME },
#else
    { "flush", (boolean *) 0, FALSE, SET_IN_FILE },
#endif
    { "fullscreen", &iflags.wc2_fullscreen, FALSE, SET_IN_FILE },
    { "help", &flags.help, TRUE, SET_IN_GAME },
    { "hilite_pet", &iflags.wc_hilite_pet, FALSE, SET_IN_GAME }, /*WC*/
    { "hilite_pile", &iflags.hilite_pile, FALSE, SET_IN_GAME },
#ifndef MAC
    { "ignintr", &flags.ignintr, FALSE, SET_IN_GAME },
#else
    { "ignintr", (boolean *) 0, FALSE, SET_IN_FILE },
#endif
    { "implicit_uncursed", &iflags.implicit_uncursed, TRUE, SET_IN_GAME },
    { "large_font", &iflags.obsolete, FALSE, SET_IN_FILE }, /* OBSOLETE */
    { "legacy", &flags.legacy, TRUE, DISP_IN_GAME },
    { "lit_corridor", &flags.lit_corridor, FALSE, SET_IN_GAME },
    { "lootabc", &flags.lootabc, FALSE, SET_IN_GAME },
#ifdef MAIL
    { "mail", &flags.biff, TRUE, SET_IN_GAME },
#else
    { "mail", (boolean *) 0, TRUE, SET_IN_FILE },
#endif
    { "mention_walls", &iflags.mention_walls, FALSE, SET_IN_GAME },
    { "menucolors", &iflags.use_menu_color, FALSE, SET_IN_GAME },
    /* for menu debugging only*/
    { "menu_tab_sep", &iflags.menu_tab_sep, FALSE, SET_IN_GAME },
    { "menu_objsyms", &iflags.menu_head_objsym, FALSE, SET_IN_GAME },
    { "mouse_support", &iflags.wc_mouse_support, TRUE, DISP_IN_GAME }, /*WC*/
#ifdef NEWS
    { "news", &iflags.news, TRUE, DISP_IN_GAME },
#else
    { "news", (boolean *) 0, FALSE, SET_IN_FILE },
#endif
    { "nudist", &u.uroleplay.nudist, FALSE, DISP_IN_GAME },
    { "null", &flags.null, TRUE, SET_IN_GAME },
#if defined(SYSFLAGS) && defined(MAC)
    { "page_wait", &sysflags.page_wait, TRUE, SET_IN_GAME },
#else
    { "page_wait", (boolean *) 0, FALSE, SET_IN_FILE },
#endif
    { "perm_invent", &flags.perm_invent, FALSE, SET_IN_GAME },
    { "pickup_thrown", &flags.pickup_thrown, TRUE, SET_IN_GAME },
    { "popup_dialog", &iflags.wc_popup_dialog, FALSE, SET_IN_GAME },   /*WC*/
    { "preload_tiles", &iflags.wc_preload_tiles, TRUE, DISP_IN_GAME }, /*WC*/
    { "pushweapon", &flags.pushweapon, FALSE, SET_IN_GAME },
#if defined(MICRO) && !defined(AMIGA)
    { "rawio", &iflags.rawio, FALSE, DISP_IN_GAME },
#else
    { "rawio", (boolean *) 0, FALSE, SET_IN_FILE },
#endif
    { "rest_on_space", &flags.rest_on_space, FALSE, SET_IN_GAME },
#ifdef RLECOMP
    { "rlecomp", &iflags.rlecomp,
#if defined(COMPRESS) || defined(ZLIB_COMP)
      FALSE,
#else
      TRUE,
#endif
      DISP_IN_GAME },
#endif
    { "safe_pet", &flags.safe_dog, TRUE, SET_IN_GAME },
    { "sanity_check", &iflags.sanity_check, FALSE, SET_IN_GAME },
    { "selectsaved", &iflags.wc2_selectsaved, TRUE, DISP_IN_GAME }, /*WC*/
    { "showexp", &flags.showexp, FALSE, SET_IN_GAME },
    { "showrace", &flags.showrace, FALSE, SET_IN_GAME },
#ifdef SCORE_ON_BOTL
    { "showscore", &flags.showscore, FALSE, SET_IN_GAME },
#else
    { "showscore", (boolean *) 0, FALSE, SET_IN_FILE },
#endif
    { "silent", &flags.silent, TRUE, SET_IN_GAME },
    { "softkeyboard", &iflags.wc2_softkeyboard, FALSE, SET_IN_FILE },
    { "sortpack", &flags.sortpack, TRUE, SET_IN_GAME },
    { "sparkle", &flags.sparkle, TRUE, SET_IN_GAME },
    { "splash_screen", &iflags.wc_splash_screen, TRUE, DISP_IN_GAME }, /*WC*/
    { "standout", &flags.standout, FALSE, SET_IN_GAME },
#if defined(STATUS_VIA_WINDOWPORT) && defined(STATUS_HILITES)
    { "statushilites", &iflags.use_status_hilites, TRUE, SET_IN_GAME },
#else
    { "statushilites", &iflags.use_status_hilites, FALSE, DISP_IN_GAME },
#endif
    { "tiled_map", &iflags.wc_tiled_map, PREFER_TILED, DISP_IN_GAME }, /*WC*/
    { "time", &flags.time, FALSE, SET_IN_GAME },
#ifdef TIMED_DELAY
    { "timed_delay", &flags.nap, TRUE, SET_IN_GAME },
#else
    { "timed_delay", (boolean *) 0, FALSE, SET_IN_GAME },
#endif
    { "tombstone", &flags.tombstone, TRUE, SET_IN_GAME },
    { "toptenwin", &iflags.toptenwin, FALSE, SET_IN_GAME },
    { "travel", &flags.travelcmd, TRUE, SET_IN_GAME },
    { "use_darkgray", &iflags.wc2_darkgray, TRUE, SET_IN_FILE },
#ifdef WIN32
    { "use_inverse", &iflags.wc_inverse, TRUE, SET_IN_GAME }, /*WC*/
#else
    { "use_inverse", &iflags.wc_inverse, FALSE, SET_IN_GAME }, /*WC*/
#endif
    { "verbose", &flags.verbose, TRUE, SET_IN_GAME },
    { "wraptext", &iflags.wc2_wraptext, FALSE, SET_IN_GAME },
#ifdef ZEROCOMP
    { "zerocomp", &iflags.zerocomp,
#if defined(COMPRESS) || defined(ZLIB_COMP)
      FALSE,
#else
      TRUE,
#endif
      DISP_IN_GAME },
#endif
    { (char *) 0, (boolean *) 0, FALSE, 0 }
};

/* compound options, for option_help() and external programs like Amiga
 * frontend */
static struct Comp_Opt {
    const char *name, *descr;
    int size; /* for frontends and such allocating space --
               * usually allowed size of data in game, but
               * occasionally maximum reasonable size for
               * typing when game maintains information in
               * a different format */
    int optflags;
} compopt[] = {
    { "align", "your starting alignment (lawful, neutral, or chaotic)", 8,
      DISP_IN_GAME },
    { "align_message", "message window alignment", 20, DISP_IN_GAME }, /*WC*/
    { "align_status", "status window alignment", 20, DISP_IN_GAME },   /*WC*/
    { "altkeyhandler", "alternate key handler", 20, DISP_IN_GAME },
#ifdef BACKWARD_COMPAT
    { "boulder", "deprecated (use S_boulder in sym file instead)", 1,
      SET_IN_FILE },
#endif
    { "catname", "the name of your (first) cat (e.g., catname:Tabby)",
      PL_PSIZ, DISP_IN_GAME },
    { "disclose", "the kinds of information to disclose at end of game",
      sizeof(flags.end_disclose) * 2, SET_IN_GAME },
    { "dogname", "the name of your (first) dog (e.g., dogname:Fang)", PL_PSIZ,
      DISP_IN_GAME },
    { "dungeon", "the symbols to use in drawing the dungeon map",
      MAXDCHARS + 1, SET_IN_FILE },
    { "effects", "the symbols to use in drawing special effects",
      MAXECHARS + 1, SET_IN_FILE },
    { "font_map", "the font to use in the map window", 40,
      DISP_IN_GAME },                                              /*WC*/
    { "font_menu", "the font to use in menus", 40, DISP_IN_GAME }, /*WC*/
    { "font_message", "the font to use in the message window", 40,
      DISP_IN_GAME },                                                  /*WC*/
    { "font_size_map", "the size of the map font", 20, DISP_IN_GAME }, /*WC*/
    { "font_size_menu", "the size of the menu font", 20,
      DISP_IN_GAME }, /*WC*/
    { "font_size_message", "the size of the message font", 20,
      DISP_IN_GAME }, /*WC*/
    { "font_size_status", "the size of the status font", 20,
      DISP_IN_GAME }, /*WC*/
    { "font_size_text", "the size of the text font", 20,
      DISP_IN_GAME }, /*WC*/
    { "font_status", "the font to use in status window", 40,
      DISP_IN_GAME }, /*WC*/
    { "font_text", "the font to use in text windows", 40,
      DISP_IN_GAME }, /*WC*/
    { "fruit", "the name of a fruit you enjoy eating", PL_FSIZ, SET_IN_GAME },
    { "gender", "your starting gender (male or female)", 8, DISP_IN_GAME },
    { "horsename", "the name of your (first) horse (e.g., horsename:Silver)",
      PL_PSIZ, DISP_IN_GAME },
    { "map_mode", "map display mode under Windows", 20, DISP_IN_GAME }, /*WC*/
    { "menustyle", "user interface for object selection", MENUTYPELEN,
      SET_IN_GAME },
    { "menu_deselect_all", "deselect all items in a menu", 4, SET_IN_FILE },
    { "menu_deselect_page", "deselect all items on this page of a menu", 4,
      SET_IN_FILE },
    { "menu_first_page", "jump to the first page in a menu", 4, SET_IN_FILE },
    { "menu_headings", "text attribute for menu headings", 9, SET_IN_GAME },
    { "menu_invert_all", "invert all items in a menu", 4, SET_IN_FILE },
    { "menu_invert_page", "invert all items on this page of a menu", 4,
      SET_IN_FILE },
    { "menu_last_page", "jump to the last page in a menu", 4, SET_IN_FILE },
    { "menu_next_page", "goto the next menu page", 4, SET_IN_FILE },
    { "menu_previous_page", "goto the previous menu page", 4, SET_IN_FILE },
    { "menu_search", "search for a menu item", 4, SET_IN_FILE },
    { "menu_select_all", "select all items in a menu", 4, SET_IN_FILE },
    { "menu_select_page", "select all items on this page of a menu", 4,
      SET_IN_FILE },
    { "monsters", "the symbols to use for monsters", MAXMCLASSES,
      SET_IN_FILE },
    { "msghistory", "number of top line messages to save", 5, DISP_IN_GAME },
#ifdef TTY_GRAPHICS
    { "msg_window", "the type of message window required", 1, SET_IN_GAME },
#else
    { "msg_window", "the type of message window required", 1, SET_IN_FILE },
#endif
    { "name", "your character's name (e.g., name:Merlin-W)", PL_NSIZ,
      DISP_IN_GAME },
    { "number_pad", "use the number pad for movement", 1, SET_IN_GAME },
    { "objects", "the symbols to use for objects", MAXOCLASSES, SET_IN_FILE },
    { "packorder", "the inventory order of the items in your pack",
      MAXOCLASSES, SET_IN_GAME },
#ifdef CHANGE_COLOR
    { "palette",
#ifndef WIN32
      "palette (00c/880/-fff is blue/yellow/reverse white)", 15,
      SET_IN_GAME },
#else
      "palette (adjust an RGB color in palette (color-R-G-B)", 15,
      SET_IN_FILE },
#endif
#if defined(MAC)
    { "hicolor", "same as palette, only order is reversed", 15, SET_IN_FILE },
#endif
#endif
    { "paranoid_confirmation", "extra prompting in certain situations", 28,
      SET_IN_GAME },
    { "pettype", "your preferred initial pet type", 4, DISP_IN_GAME },
    { "pickup_burden", "maximum burden picked up before prompt", 20,
      SET_IN_GAME },
    { "pickup_types", "types of objects to pick up automatically",
      MAXOCLASSES, SET_IN_GAME },
    { "pile_limit", "threshold for \"there are many objects here\"", 24,
      SET_IN_GAME },
    { "playmode", "normal play, non-scoring explore mode, or debug mode", 8,
      DISP_IN_GAME },
    { "player_selection", "choose character via dialog or prompts", 12,
      DISP_IN_GAME },
    { "race", "your starting race (e.g., Human, Elf)", PL_CSIZ,
      DISP_IN_GAME },
    { "role", "your starting role (e.g., Barbarian, Valkyrie)", PL_CSIZ,
      DISP_IN_GAME },
    { "runmode", "display frequency when `running' or `travelling'",
      sizeof "teleport", SET_IN_GAME },
    { "scores", "the parts of the score list you wish to see", 32,
      SET_IN_GAME },
    { "scroll_amount", "amount to scroll map when scroll_margin is reached",
      20, DISP_IN_GAME }, /*WC*/
    { "scroll_margin", "scroll map when this far from the edge", 20,
      DISP_IN_GAME }, /*WC*/
    { "sortloot", "sort object selection lists by description", 4,
      SET_IN_GAME },
#ifdef MSDOS
    { "soundcard", "type of sound card to use", 20, SET_IN_FILE },
#endif
    { "symset", "load a set of display symbols from the symbols file", 70,
      SET_IN_GAME },
    { "roguesymset",
      "load a set of rogue display symbols from the symbols file", 70,
      SET_IN_GAME },
    { "suppress_alert", "suppress alerts about version-specific features", 8,
      SET_IN_GAME },
    { "tile_width", "width of tiles", 20, DISP_IN_GAME },   /*WC*/
    { "tile_height", "height of tiles", 20, DISP_IN_GAME }, /*WC*/
    { "tile_file", "name of tile file", 70, DISP_IN_GAME }, /*WC*/
    { "traps", "the symbols to use in drawing traps", MAXTCHARS + 1,
      SET_IN_FILE },
    { "vary_msgcount", "show more old messages at a time", 20,
      DISP_IN_GAME }, /*WC*/
#ifdef MSDOS
    { "video", "method of video updating", 20, SET_IN_FILE },
#endif
#ifdef VIDEOSHADES
    { "videocolors", "color mappings for internal screen routines", 40,
      DISP_IN_GAME },
    { "videoshades", "gray shades to map to black/gray/white", 32,
      DISP_IN_GAME },
#endif
#ifdef WIN32
    { "subkeyvalue", "override keystroke value", 7, SET_IN_FILE },
#endif
    { "windowcolors", "the foreground/background colors of windows", /*WC*/
      80, DISP_IN_GAME },
    { "windowtype", "windowing system to use", WINTYPELEN, DISP_IN_GAME },
#ifdef WINCHAIN
    { "windowchain", "window processor to use", WINTYPELEN, SET_IN_SYS },
#endif
#ifdef BACKWARD_COMPAT
    { "DECgraphics", "load DECGraphics display symbols", 70, SET_IN_FILE },
    { "IBMgraphics", "load IBMGraphics display symbols", 70, SET_IN_FILE },
#ifdef MAC_GRAPHICS_ENV
    { "Macgraphics", "load MACGraphics display symbols", 70, SET_IN_FILE },
#endif
#endif
    { (char *) 0, (char *) 0, 0, 0 }
};

#ifdef OPTION_LISTS_ONLY
#undef static

#else /* use rest of file */

extern struct symparse loadsyms[];
static boolean need_redraw; /* for doset() */

#if defined(TOS) && defined(TEXTCOLOR)
extern boolean colors_changed;  /* in tos.c */
#endif

#ifdef VIDEOSHADES
extern char *shade[3];          /* in sys/msdos/video.c */
extern char ttycolors[CLR_MAX]; /* in sys/msdos/video.c */
#endif

static char def_inv_order[MAXOCLASSES] = {
    COIN_CLASS, AMULET_CLASS, WEAPON_CLASS, ARMOR_CLASS, FOOD_CLASS,
    SCROLL_CLASS, SPBOOK_CLASS, POTION_CLASS, RING_CLASS, WAND_CLASS,
    TOOL_CLASS, GEM_CLASS, ROCK_CLASS, BALL_CLASS, CHAIN_CLASS, 0,
};

/*
 * Default menu manipulation command accelerators.  These may _not_ be:
 *
 *      + a number - reserved for counts
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
} menu_cmd_t;

#define NUM_MENU_CMDS 11
static const menu_cmd_t default_menu_cmd_info[NUM_MENU_CMDS] = {
/* 0*/  { "menu_first_page", MENU_FIRST_PAGE },
        { "menu_last_page", MENU_LAST_PAGE },
        { "menu_next_page", MENU_NEXT_PAGE },
        { "menu_previous_page", MENU_PREVIOUS_PAGE },
        { "menu_select_all", MENU_SELECT_ALL },
/* 5*/  { "menu_deselect_all", MENU_UNSELECT_ALL },
        { "menu_invert_all", MENU_INVERT_ALL },
        { "menu_select_page", MENU_SELECT_PAGE },
        { "menu_deselect_page", MENU_UNSELECT_PAGE },
        { "menu_invert_page", MENU_INVERT_PAGE },
/*10*/  { "menu_search", MENU_SEARCH },
};

/*
 * Allow the user to map incoming characters to various menu commands.
 * The accelerator list must be a valid C string.
 */
#define MAX_MENU_MAPPED_CMDS 32 /* some number */
char mapped_menu_cmds[MAX_MENU_MAPPED_CMDS + 1]; /* exported */
static char mapped_menu_op[MAX_MENU_MAPPED_CMDS + 1];
static short n_menu_mapped = 0;

static boolean initial, from_file;

STATIC_DCL void FDECL(doset_add_menu, (winid, const char *, int));
STATIC_DCL void FDECL(nmcpy, (char *, const char *, int));
STATIC_DCL void FDECL(escapes, (const char *, char *));
STATIC_DCL void FDECL(rejectoption, (const char *));
STATIC_DCL void FDECL(badoption, (const char *));
STATIC_DCL char *FDECL(string_for_opt, (char *, BOOLEAN_P));
STATIC_DCL char *FDECL(string_for_env_opt, (const char *, char *, BOOLEAN_P));
STATIC_DCL void FDECL(bad_negation, (const char *, BOOLEAN_P));
STATIC_DCL int FDECL(change_inv_order, (char *));
STATIC_DCL void FDECL(oc_to_str, (char *, char *));
STATIC_DCL int FDECL(feature_alert_opts, (char *, const char *));
STATIC_DCL const char *FDECL(get_compopt_value, (const char *, char *));
STATIC_DCL boolean FDECL(special_handling, (const char *,
                                            BOOLEAN_P, BOOLEAN_P));
STATIC_DCL void FDECL(warning_opts, (char *, const char *));
STATIC_DCL boolean FDECL(duplicate_opt_detection, (const char *, int));
STATIC_DCL void FDECL(complain_about_duplicate, (const char *, int));

STATIC_OVL void FDECL(wc_set_font_name, (int, char *));
STATIC_OVL int FDECL(wc_set_window_colors, (char *));
STATIC_OVL boolean FDECL(is_wc_option, (const char *));
STATIC_OVL boolean FDECL(wc_supported, (const char *));
STATIC_OVL boolean FDECL(is_wc2_option, (const char *));
STATIC_OVL boolean FDECL(wc2_supported, (const char *));
STATIC_DCL void FDECL(remove_autopickup_exception,
                      (struct autopickup_exception *));
STATIC_OVL int FDECL(count_ape_maps, (int *, int *));
STATIC_DCL const char *FDECL(attr2attrname, (int));
STATIC_DCL int NDECL(query_color);
STATIC_DCL int NDECL(query_msgtype);
STATIC_DCL int FDECL(query_attr, (const char *));
STATIC_DCL const char * FDECL(msgtype2name, (int));
STATIC_DCL boolean FDECL(msgtype_add, (int, char *));
STATIC_DCL void FDECL(free_one_msgtype, (int));
STATIC_DCL int NDECL(msgtype_count);
STATIC_DCL boolean FDECL(add_menu_coloring_parsed, (char *, int, int));
STATIC_DCL void FDECL(free_one_menu_coloring, (int));
STATIC_DCL int NDECL(count_menucolors);
STATIC_DCL int FDECL(handle_add_list_remove, (const char *, int));

void
reglyph_darkroom()
{
    xchar x, y;

    for (x = 0; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++) {
            struct rm *lev = &levl[x][y];

            if (!flags.dark_room || !iflags.use_color
                || Is_rogue_level(&u.uz)) {
                if (lev->glyph == cmap_to_glyph(S_darkroom))
                    lev->glyph = lev->waslit ? cmap_to_glyph(S_room)
                                             : cmap_to_glyph(S_stone);
            } else {
                if (lev->glyph == cmap_to_glyph(S_room) && lev->seenv
                    && lev->waslit && !cansee(x, y))
                    lev->glyph = cmap_to_glyph(S_darkroom);
                else if (lev->glyph == cmap_to_glyph(S_stone)
                         && lev->typ == ROOM && lev->seenv && !cansee(x, y))
                    lev->glyph = cmap_to_glyph(S_darkroom);
            }
        }
    if (flags.dark_room && iflags.use_color)
        showsyms[S_darkroom] = showsyms[S_room];
    else
        showsyms[S_darkroom] = showsyms[S_stone];
}

/* check whether a user-supplied option string is a proper leading
   substring of a particular option name; option string might have
   a colon or equals sign and arbitrary value appended to it */
boolean
match_optname(user_string, opt_name, min_length, val_allowed)
const char *user_string, *opt_name;
int min_length;
boolean val_allowed;
{
    int len = (int) strlen(user_string);

    if (val_allowed) {
        const char *p = index(user_string, ':'),
                   *q = index(user_string, '=');

        if (!p || (q && q < p))
            p = q;
        while (p && p > user_string && isspace((uchar) * (p - 1)))
            p--;
        if (p)
            len = (int) (p - user_string);
    }

    return (boolean) (len >= min_length
                      && !strncmpi(opt_name, user_string, len));
}

/* most environment variables will eventually be printed in an error
 * message if they don't work, and most error message paths go through
 * BUFSZ buffers, which could be overflowed by a maliciously long
 * environment variable.  If a variable can legitimately be long, or
 * if it's put in a smaller buffer, the responsible code will have to
 * bounds-check itself.
 */
char *
nh_getenv(ev)
const char *ev;
{
    char *getev = getenv(ev);

    if (getev && strlen(getev) <= (BUFSZ / 2))
        return getev;
    else
        return (char *) 0;
}

/* process options, possibly including SYSCF */
void
initoptions()
{
    initoptions_init();
#ifdef SYSCF
/* someday there may be other SYSCF alternatives besides text file */
#ifdef SYSCF_FILE
    /* If SYSCF_FILE is specified, it _must_ exist... */
    assure_syscf_file();
    /* ... and _must_ parse correctly. */
    if (!read_config_file(SYSCF_FILE, SET_IN_SYS)) {
        raw_printf("Error(s) found in SYSCF_FILE, quitting.");
        terminate(EXIT_FAILURE);
    }
    /*
     * TODO [maybe]: parse the sysopt entries which are space-separated
     * lists of usernames into arrays with one name per element.
     */
#endif
#endif
    initoptions_finish();
}

void
initoptions_init()
{
#if defined(UNIX) || defined(VMS)
    char *opts;
#endif
    int i;

    /* set up the command parsing */
    reset_commands(TRUE); /* init */

    /* initialize the random number generator */
    setrandom();

    /* for detection of configfile options specified multiple times */
    iflags.opt_booldup = iflags.opt_compdup = (int *) 0;

    for (i = 0; boolopt[i].name; i++) {
        if (boolopt[i].addr)
            *(boolopt[i].addr) = boolopt[i].initvalue;
    }
#if defined(COMPRESS) || defined(ZLIB_COMP)
    set_savepref("externalcomp");
    set_restpref("externalcomp");
#ifdef RLECOMP
    set_savepref("!rlecomp");
    set_restpref("!rlecomp");
#endif
#else
#ifdef ZEROCOMP
    set_savepref("zerocomp");
    set_restpref("zerocomp");
#endif
#ifdef RLECOMP
    set_savepref("rlecomp");
    set_restpref("rlecomp");
#endif
#endif
#ifdef SYSFLAGS
    Strcpy(sysflags.sysflagsid, "sysflags");
    sysflags.sysflagsid[9] = (char) sizeof(struct sysflag);
#endif
    flags.end_own = FALSE;
    flags.end_top = 3;
    flags.end_around = 2;
    flags.paranoia_bits = PARANOID_PRAY; /* old prayconfirm=TRUE */
    flags.pile_limit = PILE_LIMIT_DFLT;  /* 5 */
    flags.runmode = RUN_LEAP;
    iflags.msg_history = 20;
#ifdef TTY_GRAPHICS
    iflags.prevmsg_window = 's';
#endif
    iflags.menu_headings = ATR_INVERSE;

    /* hero's role, race, &c haven't been chosen yet */
    flags.initrole = flags.initrace = flags.initgend = flags.initalign =
        ROLE_NONE;

    /* Set the default monster and object class symbols. */
    init_symbols();
    for (i = 0; i < WARNCOUNT; i++)
        warnsyms[i] = def_warnsyms[i].sym;
    iflags.bouldersym = 0;

    iflags.travelcc.x = iflags.travelcc.y = -1;

    /* assert( sizeof flags.inv_order == sizeof def_inv_order ); */
    (void) memcpy((genericptr_t) flags.inv_order,
                  (genericptr_t) def_inv_order, sizeof flags.inv_order);
    flags.pickup_types[0] = '\0';
    flags.pickup_burden = MOD_ENCUMBER;
    flags.sortloot = 'l'; /* sort only loot by default */

    for (i = 0; i < NUM_DISCLOSURE_OPTIONS; i++)
        flags.end_disclose[i] = DISCLOSE_PROMPT_DEFAULT_NO;
    switch_symbols(FALSE); /* set default characters */
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
        if (!symset[PRIMARY].name)
            load_symset("IBMGraphics", PRIMARY);
        if (!symset[ROGUESET].name)
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
        if (!symset[PRIMARY].name)
            load_symset("DECGraphics", PRIMARY);
        switch_symbols(TRUE);
    }
#endif
#endif /* UNIX || VMS */

#ifdef MAC_GRAPHICS_ENV
    if (!symset[PRIMARY].name)
        load_symset("MACGraphics", PRIMARY);
    switch_symbols(TRUE);
#endif /* MAC_GRAPHICS_ENV */
    flags.menu_style = MENU_FULL;

    /* since this is done before init_objects(), do partial init here */
    objects[SLIME_MOLD].oc_name_idx = SLIME_MOLD;
    nmcpy(pl_fruit, OBJ_NAME(objects[SLIME_MOLD]), PL_FSIZ);
}

void
initoptions_finish()
{
#ifndef MAC
    char *opts = getenv("NETHACKOPTIONS");

    if (!opts)
        opts = getenv("HACKOPTIONS");
    if (opts) {
        if (*opts == '/' || *opts == '\\' || *opts == '@') {
            if (*opts == '@')
                opts++; /* @filename */
            /* looks like a filename */
            if (strlen(opts) < BUFSZ / 2)
                read_config_file(opts, SET_IN_FILE);
        } else {
            read_config_file((char *) 0, SET_IN_FILE);
            /* let the total length of options be long;
             * parseoptions() will check each individually
             */
            parseoptions(opts, TRUE, FALSE);
        }
    } else
#endif
        read_config_file((char *) 0, SET_IN_FILE);

    (void) fruitadd(pl_fruit, (struct fruit *) 0);
    /*
     * Remove "slime mold" from list of object names.  This will
     * prevent it from being wished unless it's actually present
     * as a named (or default) fruit.  Wishing for "fruit" will
     * result in the player's preferred fruit [better than "\033"].
     */
    obj_descr[SLIME_MOLD].oc_name = "fruit";

    if (iflags.bouldersym)
        update_bouldersym();
    reglyph_darkroom();
    return;
}

STATIC_OVL void
nmcpy(dest, src, maxlen)
char *dest;
const char *src;
int maxlen;
{
    int count;

    for (count = 1; count < maxlen; count++) {
        if (*src == ',' || *src == '\0')
            break; /*exit on \0 terminator*/
        *dest++ = *src++;
    }
    *dest = 0;
}

/*
 * escapes(): escape expansion for showsyms.  C-style escapes understood
 * include \n, \b, \t, \r, \xnnn (hex), \onnn (octal), \nnn (decimal).
 * The ^-prefix for control characters is also understood, and \[mM]
 * has the effect of 'meta'-ing the value which follows (so that the
 * alternate character set will be enabled).
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
STATIC_OVL void
escapes(cp, tp)
const char *cp;
char *tp;
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
            /* remaining cases are all for backslash and we know cp[1] is not
             * \0 */
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

STATIC_OVL void
rejectoption(optname)
const char *optname;
{
#ifdef MICRO
    pline("\"%s\" settable only from %s.", optname, lastconfigfile);
#else
    pline("%s can be set only from NETHACKOPTIONS or %s.", optname,
          lastconfigfile);
#endif
}

STATIC_OVL void
badoption(opts)
const char *opts;
{
    if (!initial) {
        if (!strncmp(opts, "h", 1) || !strncmp(opts, "?", 1))
            option_help();
        else
            pline("Bad syntax: %s.  Enter \"?g\" for help.", opts);
        return;
    }
#ifdef MAC
    else
        return;
#endif

    if (from_file)
        raw_printf("Bad syntax in OPTIONS in %s: %s%s.\n", lastconfigfile,
#ifdef WIN32
                    "\n",
#else
                    "",
#endif
                    opts);
    else
        raw_printf("Bad syntax in NETHACKOPTIONS: %s%s.\n",
#ifdef WIN32
                    "\n",
#else
                    "",
#endif
                    opts);
    wait_synch();
}

STATIC_OVL char *
string_for_opt(opts, val_optional)
char *opts;
boolean val_optional;
{
    char *colon, *equals;

    colon = index(opts, ':');
    equals = index(opts, '=');
    if (!colon || (equals && equals < colon))
        colon = equals;

    if (!colon || !*++colon) {
        if (!val_optional)
            badoption(opts);
        return (char *) 0;
    }
    return colon;
}

STATIC_OVL char *
string_for_env_opt(optname, opts, val_optional)
const char *optname;
char *opts;
boolean val_optional;
{
    if (!initial) {
        rejectoption(optname);
        return (char *) 0;
    }
    return string_for_opt(opts, val_optional);
}

STATIC_OVL void
bad_negation(optname, with_parameter)
const char *optname;
boolean with_parameter;
{
    pline_The("%s option may not %sbe negated.", optname,
              with_parameter ? "both have a value and " : "");
}

/*
 * Change the inventory order, using the given string as the new order.
 * Missing characters in the new order are filled in at the end from
 * the current inv_order, except for gold, which is forced to be first
 * if not explicitly present.
 *
 * This routine returns 1 unless there is a duplicate or bad char in
 * the string.
 */
STATIC_OVL int
change_inv_order(op)
char *op;
{
    int oc_sym, num;
    char *sp, buf[BUFSZ];

    num = 0;
    /*  !!!! probably unnecessary with gold as normal inventory */

    for (sp = op; *sp; sp++) {
        oc_sym = def_char_to_objclass(*sp);
        /* reject bad or duplicate entries */
        if (oc_sym == MAXOCLASSES || oc_sym == RANDOM_CLASS
            || oc_sym == ILLOBJ_CLASS || !index(flags.inv_order, oc_sym)
            || index(sp + 1, *sp))
            return 0;
        /* retain good ones */
        buf[num++] = (char) oc_sym;
    }
    buf[num] = '\0';

    /* fill in any omitted classes, using previous ordering */
    for (sp = flags.inv_order; *sp; sp++)
        if (!index(buf, *sp)) {
            buf[num++] = *sp;
            buf[num] = '\0'; /* explicitly terminate for next index() */
        }

    Strcpy(flags.inv_order, buf);
    return 1;
}

STATIC_OVL void
warning_opts(opts, optype)
register char *opts;
const char *optype;
{
    uchar translate[WARNCOUNT];
    int length, i;

    if (!(opts = string_for_env_opt(optype, opts, FALSE)))
        return;
    escapes(opts, opts);

    length = (int) strlen(opts);
    /* match the form obtained from PC configuration files */
    for (i = 0; i < WARNCOUNT; i++)
        translate[i] = (i >= length) ? 0
                                     : opts[i] ? (uchar) opts[i]
                                               : def_warnsyms[i].sym;
    assign_warnings(translate);
}

void
assign_warnings(graph_chars)
register uchar *graph_chars;
{
    int i;

    for (i = 0; i < WARNCOUNT; i++)
        if (graph_chars[i])
            warnsyms[i] = graph_chars[i];
}

STATIC_OVL int
feature_alert_opts(op, optn)
char *op;
const char *optn;
{
    char buf[BUFSZ];
    boolean rejectver = FALSE;
    unsigned long fnv = get_feature_notice_ver(op); /* version.c */

    if (fnv == 0L)
        return 0;
    if (fnv > get_current_feature_ver())
        rejectver = TRUE;
    else
        flags.suppress_alert = fnv;
    if (rejectver) {
        if (!initial) {
            You_cant("disable new feature alerts for future versions.");
        } else {
            Sprintf(buf,
                    "\n%s=%s Invalid reference to a future version ignored",
                    optn, op);
            badoption(buf);
        }
        return 0;
    }
    if (!initial) {
        Sprintf(buf, "%lu.%lu.%lu", FEATURE_NOTICE_VER_MAJ,
                FEATURE_NOTICE_VER_MIN, FEATURE_NOTICE_VER_PATCH);
        pline(
          "Feature change alerts disabled for NetHack %s features and prior.",
              buf);
    }
    return 1;
}

void
set_duplicate_opt_detection(on_or_off)
int on_or_off;
{
    int k, *optptr;

    if (on_or_off != 0) {
        /*-- ON --*/
        if (iflags.opt_booldup)
            impossible("iflags.opt_booldup already on (memory leak)");
        iflags.opt_booldup = (int *) alloc(SIZE(boolopt) * sizeof(int));
        optptr = iflags.opt_booldup;
        for (k = 0; k < SIZE(boolopt); ++k)
            *optptr++ = 0;

        if (iflags.opt_compdup)
            impossible("iflags.opt_compdup already on (memory leak)");
        iflags.opt_compdup = (int *) alloc(SIZE(compopt) * sizeof(int));
        optptr = iflags.opt_compdup;
        for (k = 0; k < SIZE(compopt); ++k)
            *optptr++ = 0;
    } else {
        /*-- OFF --*/
        if (iflags.opt_booldup)
            free((genericptr_t) iflags.opt_booldup);
        iflags.opt_booldup = (int *) 0;
        if (iflags.opt_compdup)
            free((genericptr_t) iflags.opt_compdup);
        iflags.opt_compdup = (int *) 0;
    }
}

STATIC_OVL boolean
duplicate_opt_detection(opts, iscompound)
const char *opts;
int iscompound; /* 0 == boolean option, 1 == compound */
{
    int i, *optptr;

    if (!iscompound && iflags.opt_booldup && initial && from_file) {
        for (i = 0; boolopt[i].name; i++) {
            if (match_optname(opts, boolopt[i].name, 3, FALSE)) {
                optptr = iflags.opt_booldup + i;
                *optptr += 1;
                if (*optptr > 1)
                    return TRUE;
                else
                    return FALSE;
            }
        }
    } else if (iscompound && iflags.opt_compdup && initial && from_file) {
        for (i = 0; compopt[i].name; i++) {
            if (match_optname(opts, compopt[i].name, strlen(compopt[i].name),
                              TRUE)) {
                optptr = iflags.opt_compdup + i;
                *optptr += 1;
                if (*optptr > 1)
                    return TRUE;
                else
                    return FALSE;
            }
        }
    }
    return FALSE;
}

STATIC_OVL void
complain_about_duplicate(opts, iscompound)
const char *opts;
int iscompound; /* 0 == boolean option, 1 == compound */
{
#ifdef MAC
    /* the Mac has trouble dealing with the output of messages while
     * processing the config file.  That should get fixed one day.
     * For now just return.
     */
#else /* !MAC */
    raw_printf("\nWarning - %s option specified multiple times: %s.\n",
               iscompound ? "compound" : "boolean", opts);
    wait_synch();
#endif /* ?MAC */
    return;
}

/* paranoia[] - used by parseoptions() and special_handling() */
STATIC_VAR const struct paranoia_opts {
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
       is just a synonym for "Confirm") */
    { PARANOID_CONFIRM, "Confirm", 1, "Paranoia", 2,
      "for \"yes\" confirmations, require \"no\" to reject" },
    { PARANOID_QUIT, "quit", 1, "explore", 1,
      "yes vs y to quit or to enter explore mode" },
    { PARANOID_DIE, "die", 1, "death", 2,
      "yes vs y to die (explore mode or debug mode)" },
    { PARANOID_BONES, "bones", 1, 0, 0,
      "yes vs y to save bones data when dying in debug mode" },
    { PARANOID_HIT, "attack", 1, "hit", 1,
      "yes vs y to attack a peaceful monster" },
    { PARANOID_PRAY, "pray", 1, 0, 0,
      "y to pray (supersedes old \"prayconfirm\" option)" },
    { PARANOID_REMOVE, "Remove", 1, "Takeoff", 1,
      "always pick from inventory for Remove and Takeoff" },
    { PARANOID_BREAKWAND, "wand", 1, "breakwand", 2,
      "yes vs y to break a wand" },
    /* for config file parsing; interactive menu skips these */
    { 0, "none", 4, 0, 0, 0 }, /* require full word match */
    { ~0, "all", 3, 0, 0, 0 }, /* ditto */
};

extern struct menucoloring *menu_colorings;

static const struct {
    const char *name;
    const int color;
} colornames[] = {
    { "black", CLR_BLACK },
    { "red", CLR_RED },
    { "green", CLR_GREEN },
    { "brown", CLR_BROWN },
    { "blue", CLR_BLUE },
    { "magenta", CLR_MAGENTA },
    { "cyan", CLR_CYAN },
    { "gray", CLR_GRAY },
    { "grey", CLR_GRAY },
    { "orange", CLR_ORANGE },
    { "light green", CLR_BRIGHT_GREEN },
    { "yellow", CLR_YELLOW },
    { "light blue", CLR_BRIGHT_BLUE },
    { "light magenta", CLR_BRIGHT_MAGENTA },
    { "light cyan", CLR_BRIGHT_CYAN },
    { "white", CLR_WHITE }
};

static const struct {
    const char *name;
    const int attr;
} attrnames[] = {
    { "none", ATR_NONE },
    { "bold", ATR_BOLD },
    { "dim", ATR_DIM },
    { "underline", ATR_ULINE },
    { "blink", ATR_BLINK },
    { "inverse", ATR_INVERSE }
};

const char *
clr2colorname(clr)
int clr;
{
    int i;

    for (i = 0; i < SIZE(colornames); i++)
        if (colornames[i].color == clr)
            return colornames[i].name;
    return (char *) 0;
}

const char *
attr2attrname(attr)
int attr;
{
    int i;

    for (i = 0; i < SIZE(attrnames); i++)
        if (attrnames[i].attr == attr)
            return attrnames[i].name;
    return (char *) 0;
}

int
query_color()
{
    winid tmpwin;
    anything any;
    int i, pick_cnt;
    menu_item *picks = (menu_item *) 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin);
    any = zeroany;
    for (i = 0; i < SIZE(colornames); i++) {
        if (!strcmp(colornames[i].name, "grey"))
            continue;
        any.a_int = i + 1;
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, colornames[i].name,
                 MENU_UNSELECTED);
    }
    end_menu(tmpwin, "Pick a color");
    pick_cnt = select_menu(tmpwin, PICK_ONE, &picks);
    destroy_nhwindow(tmpwin);
    if (pick_cnt > 0) {
        i = colornames[picks->item.a_int - 1].color;
        free((genericptr_t) picks);
        return i;
    }
    return -1;
}

int
query_attr(prompt)
const char *prompt;
{
    winid tmpwin;
    anything any;
    int i, pick_cnt;
    menu_item *picks = (menu_item *) 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin);
    any = zeroany;
    for (i = 0; i < SIZE(attrnames); i++) {
        any.a_int = i + 1;
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, attrnames[i].attr,
                 attrnames[i].name, MENU_UNSELECTED);
    }
    end_menu(tmpwin, prompt ? prompt : "Pick an attribute");
    pick_cnt = select_menu(tmpwin, PICK_ONE, &picks);
    destroy_nhwindow(tmpwin);
    if (pick_cnt > 0) {
        i = attrnames[picks->item.a_int - 1].attr;
        free((genericptr_t) picks);
        return i;
    }
    return -1;
}

static const struct {
    const char *name;
    const xchar msgtyp;
    const char *descr;
} msgtype_names[] = {
    { "show", MSGTYP_NORMAL, "Show message normally" },
    { "hide", MSGTYP_NOSHOW, "Hide message" },
    { "noshow", MSGTYP_NOSHOW, NULL },
    { "stop", MSGTYP_STOP, "Prompt for more after the message" },
    { "more", MSGTYP_STOP, NULL },
    { "norep", MSGTYP_NOREP, "Do not repeat the message" }
};

const char *
msgtype2name(typ)
int typ;
{
    int i;

    for (i = 0; i < SIZE(msgtype_names); i++)
        if (msgtype_names[i].descr && msgtype_names[i].msgtyp == typ)
            return msgtype_names[i].name;
    return (char *) 0;
}

int
query_msgtype()
{
    winid tmpwin;
    anything any;
    int i, pick_cnt;
    menu_item *picks = (menu_item *) 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin);
    any = zeroany;
    for (i = 0; i < SIZE(msgtype_names); i++)
        if (msgtype_names[i].descr) {
            any.a_int = msgtype_names[i].msgtyp + 1;
            add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
                 msgtype_names[i].descr, MENU_UNSELECTED);
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

boolean
msgtype_add(typ, pattern)
int typ;
char *pattern;
{
    struct plinemsg_type *tmp
              = (struct plinemsg_type *) alloc(sizeof (struct plinemsg_type));

    if (!tmp)
        return FALSE;
    tmp->msgtype = typ;
    tmp->regex = regex_init();
    if (!regex_compile(pattern, tmp->regex)) {
        static const char *re_error = "MSGTYPE regex error";

        if (!iflags.window_inited)
            raw_printf("\n%s: %s\n", re_error, regex_error_desc(tmp->regex));
        else
            pline("%s: %s", re_error, regex_error_desc(tmp->regex));
        wait_synch();
        regex_free(tmp->regex);
        free((genericptr_t) tmp);
        return FALSE;
    }
    tmp->pattern = dupstr(pattern);
    tmp->next = plinemsg_types;
    plinemsg_types = tmp;
    return TRUE;
}

void
msgtype_free()
{
    struct plinemsg_type *tmp, *tmp2 = 0;

    for (tmp = plinemsg_types; tmp; tmp = tmp2) {
        tmp2 = tmp->next;
        free((genericptr_t) tmp->pattern);
        regex_free(tmp->regex);
        free((genericptr_t) tmp);
    }
    plinemsg_types = (struct plinemsg_type *) 0;
}

void
free_one_msgtype(idx)
int idx; /* 0 .. */
{
    struct plinemsg_type *tmp = plinemsg_types;
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
                plinemsg_types = next;
            return;
        }
        idx--;
        prev = tmp;
        tmp = tmp->next;
    }
}

int
msgtype_type(msg)
const char *msg;
{
    struct plinemsg_type *tmp = plinemsg_types;

    while (tmp) {
        if (regex_match(msg, tmp->regex))
            return tmp->msgtype;
        tmp = tmp->next;
    }
    return MSGTYP_NORMAL;
}

int
msgtype_count()
{
    int c = 0;
    struct plinemsg_type *tmp = plinemsg_types;

    while (tmp) {
        c++;
        tmp = tmp->next;
    }
    return c;
}

boolean
msgtype_parse_add(str)
char *str;
{
    char pattern[256];
    char msgtype[11];

    if (sscanf(str, "%10s \"%255[^\"]\"", msgtype, pattern) == 2) {
        int typ = -1;
        int i;

        for (i = 0; i < SIZE(msgtype_names); i++)
            if (!strncmpi(msgtype_names[i].name, msgtype, strlen(msgtype))) {
                typ = msgtype_names[i].msgtyp;
                break;
            }
        if (typ != -1)
            return msgtype_add(typ, pattern);
    }
    return FALSE;
}

boolean
add_menu_coloring_parsed(str, c, a)
char *str;
int c, a;
{
    static const char re_error[] = "Menucolor regex error";
    struct menucoloring *tmp;

    if (!str)
        return FALSE;
    tmp = (struct menucoloring *) alloc(sizeof (struct menucoloring));
    tmp->match = regex_init();
    if (!regex_compile(str, tmp->match)) {
        if (!iflags.window_inited)
            raw_printf("\n%s: %s\n", re_error, regex_error_desc(tmp->match));
        else
            pline("%s: %s", re_error, regex_error_desc(tmp->match));
        wait_synch();
        regex_free(tmp->match);
        free(tmp);
        return FALSE;
    } else {
        tmp->next = menu_colorings;
        tmp->origstr = dupstr(str);
        tmp->color = c;
        tmp->attr = a;
        menu_colorings = tmp;
        return TRUE;
    }
}

/* parse '"regex_string"=color&attr' and add it to menucoloring */
boolean
add_menu_coloring(str)
char *str;
{
    int i, c = NO_COLOR, a = ATR_NONE;
    char *tmps, *cs, *amp;

    if (!str || (cs = index(str, '=')) == 0)
        return FALSE;

    tmps = cs + 1; /* advance past '=' */
    mungspaces(tmps);
    if ((amp = index(tmps, '&')) != 0)
        *amp = '\0';

    /* allow "lightblue", "light blue", and "light-blue" to match "light blue"
       (also junk like "_l i-gh_t---b l u e" but we won't worry about that);
       also copes with trailing space; mungspaces removed any leading space */
    for (i = 0; i < SIZE(colornames); i++)
        if (fuzzymatch(tmps, colornames[i].name, " -_", TRUE)) {
            c = colornames[i].color;
            break;
        }
    if (i == SIZE(colornames) && (*tmps >= '0' && *tmps <= '9'))
        c = atoi(tmps);

    if (c > 15)
        return FALSE;

    if (amp) {
        tmps = amp + 1; /* advance past '&' */
        /* unlike colors, none of he attribute names has any embedded spaces,
           but use of fuzzymatch() allows us ignore the presence of leading
           and/or trailing (and also embedded) spaces in the user's string;
           dash and underscore skipping could be omitted but does no harm */
        for (i = 0; i < SIZE(attrnames); i++)
            if (fuzzymatch(tmps, attrnames[i].name, " -_", TRUE)) {
                a = attrnames[i].attr;
                break;
            }
        if (i == SIZE(attrnames) && (*tmps >= '0' && *tmps <= '9'))
            a = atoi(tmps);
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
get_menu_coloring(str, color, attr)
char *str;
int *color, *attr;
{
    struct menucoloring *tmpmc;

    if (iflags.use_menu_color)
        for (tmpmc = menu_colorings; tmpmc; tmpmc = tmpmc->next)
            if (regex_match(str, tmpmc->match)) {
                *color = tmpmc->color;
                *attr = tmpmc->attr;
                return TRUE;
            }
    return FALSE;
}

void
free_menu_coloring()
{
    struct menucoloring *tmp = menu_colorings;

    while (tmp) {
        struct menucoloring *tmp2 = tmp->next;

        regex_free(tmp->match);
        free((genericptr_t) tmp->origstr);
        free((genericptr_t) tmp);
        tmp = tmp2;
    }
}

void
free_one_menu_coloring(idx)
int idx; /* 0 .. */
{
    struct menucoloring *tmp = menu_colorings;
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
                menu_colorings = next;
            return;
        }
        idx--;
        prev = tmp;
        tmp = tmp->next;
    }
}

int
count_menucolors()
{
    int count = 0;
    struct menucoloring *tmp = menu_colorings;

    while (tmp) {
        count++;
        tmp = tmp->next;
    }
    return count;
}

void
parseoptions(opts, tinitial, tfrom_file)
register char *opts;
boolean tinitial, tfrom_file;
{
    register char *op;
    unsigned num;
    boolean negated, val_negated, duplicate;
    int i;
    const char *fullname;

    initial = tinitial;
    from_file = tfrom_file;
    if ((op = index(opts, ',')) != 0) {
        *op++ = 0;
        parseoptions(op, initial, from_file);
    }
    if (strlen(opts) > BUFSZ / 2) {
        badoption("option too long");
        return;
    }

    /* strip leading and trailing white space */
    while (isspace((uchar) *opts))
        opts++;
    op = eos(opts);
    while (--op >= opts && isspace((uchar) *op))
        *op = '\0';

    if (!*opts)
        return;
    negated = FALSE;
    while ((*opts == '!') || !strncmpi(opts, "no", 2)) {
        if (*opts == '!')
            opts++;
        else
            opts += 2;
        negated = !negated;
    }

    /* variant spelling */

    if (match_optname(opts, "colour", 5, FALSE))
        Strcpy(opts, "color"); /* fortunately this isn't longer */

    /* special boolean options */

    if (match_optname(opts, "female", 3, FALSE)) {
        if (duplicate_opt_detection(opts, 0))
            complain_about_duplicate(opts, 0);
        if (!initial && flags.female == negated)
            pline("That is not anatomically possible.");
        else
            flags.initgend = flags.female = !negated;
        return;
    }

    if (match_optname(opts, "male", 4, FALSE)) {
        if (duplicate_opt_detection(opts, 0))
            complain_about_duplicate(opts, 0);
        if (!initial && flags.female != negated)
            pline("That is not anatomically possible.");
        else
            flags.initgend = flags.female = negated;
        return;
    }

#if defined(MICRO) && !defined(AMIGA)
    /* included for compatibility with old NetHack.cnf files */
    if (match_optname(opts, "IBM_", 4, FALSE)) {
        iflags.BIOS = !negated;
        return;
    }
#endif /* MICRO */

    /* compound options */

    /* This first batch can be duplicated if their values are negated */

    /* align:string */
    fullname = "align";
    if (match_optname(opts, fullname, sizeof("align") - 1, TRUE)) {
        if (negated) {
            bad_negation(fullname, FALSE);
        } else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0) {
            val_negated = FALSE;
            while ((*op == '!') || !strncmpi(op, "no", 2)) {
                if (*op == '!')
                    op++;
                else
                    op += 2;
                val_negated = !val_negated;
            }
            if (val_negated) {
                if (!setrolefilter(op))
                    badoption(opts);
            } else {
                if (duplicate_opt_detection(opts, 1))
                    complain_about_duplicate(opts, 1);
                if ((flags.initalign = str2align(op)) == ROLE_NONE)
                    badoption(opts);
            }
        }
        return;
    }

    /* role:string or character:string */
    fullname = "role";
    if (match_optname(opts, fullname, 4, TRUE)
        || match_optname(opts, (fullname = "character"), 4, TRUE)) {
        if (negated) {
            bad_negation(fullname, FALSE);
        } else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0) {
            val_negated = FALSE;
            while ((*op == '!') || !strncmpi(op, "no", 2)) {
                if (*op == '!')
                    op++;
                else
                    op += 2;
                val_negated = !val_negated;
            }
            if (val_negated) {
                if (!setrolefilter(op))
                    badoption(opts);
            } else {
                if (duplicate_opt_detection(opts, 1))
                    complain_about_duplicate(opts, 1);
                if ((flags.initrole = str2role(op)) == ROLE_NONE)
                    badoption(opts);
                else /* Backwards compatibility */
                    nmcpy(pl_character, op, PL_NSIZ);
            }
        }
        return;
    }

    /* race:string */
    fullname = "race";
    if (match_optname(opts, fullname, 4, TRUE)) {
        if (negated) {
            bad_negation(fullname, FALSE);
        } else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0) {
            val_negated = FALSE;
            while ((*op == '!') || !strncmpi(op, "no", 2)) {
                if (*op == '!')
                    op++;
                else
                    op += 2;
                val_negated = !val_negated;
            }
            if (val_negated) {
                if (!setrolefilter(op))
                    badoption(opts);
            } else {
                if (duplicate_opt_detection(opts, 1))
                    complain_about_duplicate(opts, 1);
                if ((flags.initrace = str2race(op)) == ROLE_NONE)
                    badoption(opts);
                else /* Backwards compatibility */
                    pl_race = *op;
            }
        }
        return;
    }

    /* gender:string */
    fullname = "gender";
    if (match_optname(opts, fullname, 4, TRUE)) {
        if (negated) {
            bad_negation(fullname, FALSE);
        } else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0) {
            val_negated = FALSE;
            while ((*op == '!') || !strncmpi(op, "no", 2)) {
                if (*op == '!')
                    op++;
                else
                    op += 2;
                val_negated = !val_negated;
            }
            if (val_negated) {
                if (!setrolefilter(op))
                    badoption(opts);
            } else {
                if (duplicate_opt_detection(opts, 1))
                    complain_about_duplicate(opts, 1);
                if ((flags.initgend = str2gend(op)) == ROLE_NONE)
                    badoption(opts);
                else
                    flags.female = flags.initgend;
            }
        }
        return;
    }

    /* We always check for duplicates on the remaining compound options,
       although individual option processing can choose to complain or not */

    duplicate =
        duplicate_opt_detection(opts, 1); /* 1 means check compounds */

    fullname = "pettype";
    if (match_optname(opts, fullname, 3, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        if ((op = string_for_env_opt(fullname, opts, negated)) != 0) {
            if (negated)
                bad_negation(fullname, TRUE);
            else
                switch (lowc(*op)) {
                case 'd': /* dog */
                    preferred_pet = 'd';
                    break;
                case 'c': /* cat */
                case 'f': /* feline */
                    preferred_pet = 'c';
                    break;
                case 'h': /* horse */
                case 'q': /* quadruped */
                    /* avoids giving "unrecognized type of pet" but
                       pet_type(dog.c) won't actually honor this */
                    preferred_pet = 'h';
                    break;
                case 'n': /* no pet */
                    preferred_pet = 'n';
                    break;
                case '*': /* random */
                    preferred_pet = '\0';
                    break;
                default:
                    pline("Unrecognized pet type '%s'.", op);
                    break;
                }
        } else if (negated)
            preferred_pet = 'n';
        return;
    }

    fullname = "catname";
    if (match_optname(opts, fullname, 3, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (negated)
            bad_negation(fullname, FALSE);
        else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0)
            nmcpy(catname, op, PL_PSIZ);
        sanitize_name(catname);
        return;
    }

    fullname = "dogname";
    if (match_optname(opts, fullname, 3, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (negated)
            bad_negation(fullname, FALSE);
        else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0)
            nmcpy(dogname, op, PL_PSIZ);
        sanitize_name(dogname);
        return;
    }

    fullname = "horsename";
    if (match_optname(opts, fullname, 5, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (negated)
            bad_negation(fullname, FALSE);
        else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0)
            nmcpy(horsename, op, PL_PSIZ);
        sanitize_name(horsename);
        return;
    }

    fullname = "number_pad";
    if (match_optname(opts, fullname, 10, TRUE)) {
        boolean compat = (strlen(opts) <= 10);

        if (duplicate)
            complain_about_duplicate(opts, 1);
        op = string_for_opt(opts, (compat || !initial));
        if (!op) {
            if (compat || negated || initial) {
                /* for backwards compatibility, "number_pad" without a
                   value is a synonym for number_pad:1 */
                iflags.num_pad = !negated;
                iflags.num_pad_mode = 0;
            }
        } else if (negated) {
            bad_negation("number_pad", TRUE);
            return;
        } else {
            int mode = atoi(op);

            if (mode < -1 || mode > 4 || (mode == 0 && *op != '0')) {
                badoption(opts);
                return;
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
        return;
    }

    fullname = "roguesymset";
    if (match_optname(opts, fullname, 7, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (negated) {
            bad_negation(fullname, FALSE);
        } else if ((op = string_for_opt(opts, FALSE)) != 0) {
            symset[ROGUESET].name = dupstr(op);
            if (!read_sym_file(ROGUESET)) {
                clear_symsetentry(ROGUESET, TRUE);
                raw_printf("Unable to load symbol set \"%s\" from \"%s\".",
                           op, SYMBOLS);
                wait_synch();
            } else {
                if (!initial && Is_rogue_level(&u.uz))
                    assign_graphics(ROGUESET);
                need_redraw = TRUE;
            }
        }
        return;
    }

    fullname = "symset";
    if (match_optname(opts, fullname, 6, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (negated) {
            bad_negation(fullname, FALSE);
        } else if ((op = string_for_opt(opts, FALSE)) != 0) {
            symset[PRIMARY].name = dupstr(op);
            if (!read_sym_file(PRIMARY)) {
                clear_symsetentry(PRIMARY, TRUE);
                raw_printf("Unable to load symbol set \"%s\" from \"%s\".",
                           op, SYMBOLS);
                wait_synch();
            } else {
                switch_symbols(TRUE);
                need_redraw = TRUE;
            }
        }
        return;
    }

    fullname = "runmode";
    if (match_optname(opts, fullname, 4, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (negated) {
            flags.runmode = RUN_TPORT;
        } else if ((op = string_for_opt(opts, FALSE)) != 0) {
            if (!strncmpi(op, "teleport", strlen(op)))
                flags.runmode = RUN_TPORT;
            else if (!strncmpi(op, "run", strlen(op)))
                flags.runmode = RUN_LEAP;
            else if (!strncmpi(op, "walk", strlen(op)))
                flags.runmode = RUN_STEP;
            else if (!strncmpi(op, "crawl", strlen(op)))
                flags.runmode = RUN_CRAWL;
            else
                badoption(opts);
        }
        return;
    }

    /* menucolor:"regex_string"=color */
    fullname = "menucolor";
    if (match_optname(opts, fullname, 9, TRUE)) {
        if (negated)
            bad_negation(fullname, FALSE);
        else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0)
            if (!add_menu_coloring(op))
                badoption(opts);
        return;
    }

    fullname = "msghistory";
    if (match_optname(opts, fullname, 3, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        op = string_for_env_opt(fullname, opts, negated);
        if ((negated && !op) || (!negated && op)) {
            iflags.msg_history = negated ? 0 : atoi(op);
        } else if (negated)
            bad_negation(fullname, TRUE);
        return;
    }

    fullname = "msg_window";
    /* msg_window:single, combo, full or reversed */
    if (match_optname(opts, fullname, 4, TRUE)) {
/* allow option to be silently ignored by non-tty ports */
#ifdef TTY_GRAPHICS
        int tmp;

        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (!(op = string_for_opt(opts, TRUE))) {
            tmp = negated ? 's' : 'f';
        } else {
            if (negated) {
                bad_negation(fullname, TRUE);
                return;
            }
            tmp = tolower(*op);
        }
        switch (tmp) {
        case 's': /* single message history cycle (default if negated) */
            iflags.prevmsg_window = 's';
            break;
        case 'c': /* combination: two singles, then full page reversed */
            iflags.prevmsg_window = 'c';
            break;
        case 'f': /* full page (default if no opts) */
            iflags.prevmsg_window = 'f';
            break;
        case 'r': /* full page (reversed) */
            iflags.prevmsg_window = 'r';
            break;
        default:
            badoption(opts);
        }
#endif
        return;
    }

    /* WINCAP
     * setting font options  */
    fullname = "font";
    if (!strncmpi(opts, fullname, 4)) {
        int opttype = -1;
        char *fontopts = opts + 4;

        if (!strncmpi(fontopts, "map", 3) || !strncmpi(fontopts, "_map", 4))
            opttype = MAP_OPTION;
        else if (!strncmpi(fontopts, "message", 7)
                 || !strncmpi(fontopts, "_message", 8))
            opttype = MESSAGE_OPTION;
        else if (!strncmpi(fontopts, "text", 4)
                 || !strncmpi(fontopts, "_text", 5))
            opttype = TEXT_OPTION;
        else if (!strncmpi(fontopts, "menu", 4)
                 || !strncmpi(fontopts, "_menu", 5))
            opttype = MENU_OPTION;
        else if (!strncmpi(fontopts, "status", 6)
                 || !strncmpi(fontopts, "_status", 7))
            opttype = STATUS_OPTION;
        else if (!strncmpi(fontopts, "_size", 5)) {
            if (!strncmpi(fontopts, "_size_map", 8))
                opttype = MAP_OPTION;
            else if (!strncmpi(fontopts, "_size_message", 12))
                opttype = MESSAGE_OPTION;
            else if (!strncmpi(fontopts, "_size_text", 9))
                opttype = TEXT_OPTION;
            else if (!strncmpi(fontopts, "_size_menu", 9))
                opttype = MENU_OPTION;
            else if (!strncmpi(fontopts, "_size_status", 11))
                opttype = STATUS_OPTION;
            else {
                badoption(opts);
                return;
            }
            if (duplicate)
                complain_about_duplicate(opts, 1);
            if (opttype > 0 && !negated
                && (op = string_for_opt(opts, FALSE)) != 0) {
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
            return;
        } else {
            badoption(opts);
        }
        if (opttype > 0 && (op = string_for_opt(opts, FALSE)) != 0) {
            wc_set_font_name(opttype, op);
#ifdef MAC
            set_font_name(opttype, op);
#endif
            return;
        } else if (negated)
            bad_negation(fullname, TRUE);
        return;
    }
#ifdef CHANGE_COLOR
    if (match_optname(opts, "palette", 3, TRUE)
#ifdef MAC
        || match_optname(opts, "hicolor", 3, TRUE)
#endif
            ) {
        int color_number, color_incr;

#ifndef WIN32
        if (duplicate)
            complain_about_duplicate(opts, 1);
#endif
#ifdef MAC
        if (match_optname(opts, "hicolor", 3, TRUE)) {
            if (negated) {
                bad_negation("hicolor", FALSE);
                return;
            }
            color_number = CLR_MAX + 4; /* HARDCODED inverse number */
            color_incr = -1;
        } else {
#endif
            if (negated) {
                bad_negation("palette", FALSE);
                return;
            }
            color_number = 0;
            color_incr = 1;
#ifdef MAC
        }
#endif
#ifdef WIN32
        op = string_for_opt(opts, TRUE);
        if (!alternative_palette(op))
            badoption(opts);
#else
        if ((op = string_for_opt(opts, FALSE)) != (char *) 0) {
            char *pt = op;
            int cnt, tmp, reverse;
            long rgb;

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
                        tmp = *(pt++);
                        if (isalpha(tmp)) {
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
                if (*pt == '/') {
                    pt++;
                }
                change_color(color_number, rgb, reverse);
                color_number += color_incr;
            }
        }
#endif /* !WIN32 */
        if (!initial) {
            need_redraw = TRUE;
        }
        return;
    }
#endif /* CHANGE_COLOR */

    if (match_optname(opts, "fruit", 2, TRUE)) {
        struct fruit *forig = 0;
        char empty_str = '\0';

        if (duplicate)
            complain_about_duplicate(opts, 1);
        op = string_for_opt(opts, negated);
        if (negated) {
            if (op) {
                bad_negation("fruit", TRUE);
                return;
            }
            op = &empty_str;
            goto goodfruit;
        }
        if (!op)
            return;
        if (!initial) {
            struct fruit *f;

            num = 0;
            for (f = ffruit; f; f = f->nextf) {
                if (!strcmp(op, f->fname))
                    break;
                num++;
            }
            if (!flags.made_fruit) {
                for (forig = ffruit; forig; forig = forig->nextf) {
                    if (!strcmp(pl_fruit, forig->fname)) {
                        break;
                    }
                }
            }
            if (!forig && num >= 100) {
                pline("Doing that so many times isn't very fruitful.");
                return;
            }
        }
    goodfruit:
        nmcpy(pl_fruit, op, PL_FSIZ);
        sanitize_name(pl_fruit);
        /* OBJ_NAME(objects[SLIME_MOLD]) won't work after initialization */
        if (!*pl_fruit)
            nmcpy(pl_fruit, "slime mold", PL_FSIZ);
        if (!initial) {
            (void) fruitadd(pl_fruit, forig);
            pline("Fruit is now \"%s\".", pl_fruit);
        }
        /* If initial, then initoptions is allowed to do it instead
         * of here (initoptions always has to do it even if there's
         * no fruit option at all.  Also, we don't want people
         * setting multiple fruits in their options.)
         */
        return;
    }

    fullname = "warnings";
    if (match_optname(opts, fullname, 5, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (negated)
            bad_negation(fullname, FALSE);
        else
            warning_opts(opts, fullname);
        return;
    }

#ifdef BACKWARD_COMPAT
    /* boulder:symbol */
    fullname = "boulder";
    if (match_optname(opts, fullname, 7, TRUE)) {
        int clash = 0;
        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (negated) {
            bad_negation(fullname, FALSE);
            return;
        }
        /* if (!(opts = string_for_env_opt(fullname, opts, FALSE)))
         */
        if (!(opts = string_for_opt(opts, FALSE)))
            return;
        escapes(opts, opts);
        if (def_char_to_monclass(opts[0]) != MAXMCLASSES)
            clash = 1;
        else if (opts[0] >= '1' && opts[0] <= '5')
            clash = 2;
        if (clash) {
            /* symbol chosen matches a used monster or warning
               symbol which is not good - reject it*/
            pline(
                "Badoption - boulder symbol '%c' conflicts with a %s symbol.",
                opts[0], (clash == 1) ? "monster" : "warning");
        } else {
            /*
             * Override the default boulder symbol.
             */
            iflags.bouldersym = (uchar) opts[0];
        }
        if (!initial)
            need_redraw = TRUE;
        return;
    }
#endif

    /* name:string */
    fullname = "name";
    if (match_optname(opts, fullname, 4, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (negated)
            bad_negation(fullname, FALSE);
        else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0)
            nmcpy(plname, op, PL_NSIZ);
        return;
    }

    /* altkeyhandler:string */
    fullname = "altkeyhandler";
    if (match_optname(opts, fullname, 4, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (negated) {
            bad_negation(fullname, FALSE);
        } else if ((op = string_for_opt(opts, negated)) != 0) {
#ifdef WIN32
            (void) strncpy(iflags.altkeyhandler, op, MAX_ALTKEYHANDLER - 5);
            load_keyboard_handler();
#endif
        }
        return;
    }

    /* WINCAP
     * align_status:[left|top|right|bottom] */
    fullname = "align_status";
    if (match_optname(opts, fullname, sizeof("align_status") - 1, TRUE)) {
        op = string_for_opt(opts, negated);
        if (op && !negated) {
            if (!strncmpi(op, "left", sizeof("left") - 1))
                iflags.wc_align_status = ALIGN_LEFT;
            else if (!strncmpi(op, "top", sizeof("top") - 1))
                iflags.wc_align_status = ALIGN_TOP;
            else if (!strncmpi(op, "right", sizeof("right") - 1))
                iflags.wc_align_status = ALIGN_RIGHT;
            else if (!strncmpi(op, "bottom", sizeof("bottom") - 1))
                iflags.wc_align_status = ALIGN_BOTTOM;
            else
                badoption(opts);
        } else if (negated)
            bad_negation(fullname, TRUE);
        return;
    }
    /* WINCAP
     * align_message:[left|top|right|bottom] */
    fullname = "align_message";
    if (match_optname(opts, fullname, sizeof("align_message") - 1, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        op = string_for_opt(opts, negated);
        if (op && !negated) {
            if (!strncmpi(op, "left", sizeof("left") - 1))
                iflags.wc_align_message = ALIGN_LEFT;
            else if (!strncmpi(op, "top", sizeof("top") - 1))
                iflags.wc_align_message = ALIGN_TOP;
            else if (!strncmpi(op, "right", sizeof("right") - 1))
                iflags.wc_align_message = ALIGN_RIGHT;
            else if (!strncmpi(op, "bottom", sizeof("bottom") - 1))
                iflags.wc_align_message = ALIGN_BOTTOM;
            else
                badoption(opts);
        } else if (negated)
            bad_negation(fullname, TRUE);
        return;
    }
    /* the order to list the pack */
    fullname = "packorder";
    if (match_optname(opts, fullname, 4, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (negated) {
            bad_negation(fullname, FALSE);
            return;
        } else if (!(op = string_for_opt(opts, FALSE)))
            return;

        if (!change_inv_order(op))
            badoption(opts);
        return;
    }

    /* user can change required response for some prompts (quit, die, hit),
       or add an extra prompt (pray, Remove) that isn't ordinarily there */
    fullname = "paranoid_confirmation";
    if (match_optname(opts, fullname, 8, TRUE)) {
        /* at present we don't complain about duplicates for this
           option, but we do throw away the old settings whenever
           we process a new one [clearing old flags is essential
           for handling default paranoid_confirm:pray sanely] */
        flags.paranoia_bits = 0; /* clear all */
        if (negated) {
            flags.paranoia_bits = 0; /* [now redundant...] */
        } else if ((op = string_for_opt(opts, TRUE)) != 0) {
            char *pp, buf[BUFSZ];

            op = mungspaces(strcpy(buf, op));
            for (;;) {
                /* We're looking to parse
                   "paranoid_confirm:whichone wheretwo whothree"
                   and "paranoid_confirm:" prefix has already
                   been stripped off by the time we get here */
                pp = index(op, ' ');
                if (pp)
                    *pp = '\0';
                /* we aren't matching option names but match_optname
                   does what we want once we've broken the space
                   delimited aggregate into separate tokens */
                for (i = 0; i < SIZE(paranoia); ++i) {
                    if (match_optname(op, paranoia[i].argname,
                                      paranoia[i].argMinLen, FALSE)
                        || (paranoia[i].synonym
                            && match_optname(op, paranoia[i].synonym,
                                             paranoia[i].synMinLen, FALSE))) {
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
                    badoption(opts);
                    break;
                }
                /* move on to next token */
                if (pp)
                    op = pp + 1;
                else
                    break; /* no next token */
            }              /* for(;;) */
        }
        return;
    }

    /* accept deprecated boolean; superseded by paranoid_confirm:pray */
    fullname = "prayconfirm";
    if (match_optname(opts, fullname, 4, FALSE)) {
        if (negated)
            flags.paranoia_bits &= ~PARANOID_PRAY;
        else
            flags.paranoia_bits |= PARANOID_PRAY;
        return;
    }

    /* maximum burden picked up before prompt (Warren Cheung) */
    fullname = "pickup_burden";
    if (match_optname(opts, fullname, 8, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (negated) {
            bad_negation(fullname, FALSE);
            return;
        } else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0) {
            switch (tolower(*op)) {
            /* Unencumbered */
            case 'u':
                flags.pickup_burden = UNENCUMBERED;
                break;
            /* Burdened (slight encumbrance) */
            case 'b':
                flags.pickup_burden = SLT_ENCUMBER;
                break;
            /* streSsed (moderate encumbrance) */
            case 's':
                flags.pickup_burden = MOD_ENCUMBER;
                break;
            /* straiNed (heavy encumbrance) */
            case 'n':
                flags.pickup_burden = HVY_ENCUMBER;
                break;
            /* OverTaxed (extreme encumbrance) */
            case 'o':
            case 't':
                flags.pickup_burden = EXT_ENCUMBER;
                break;
            /* overLoaded */
            case 'l':
                flags.pickup_burden = OVERLOADED;
                break;
            default:
                badoption(opts);
            }
        }
        return;
    }

    /* types of objects to pick up automatically */
    if (match_optname(opts, "pickup_types", 8, TRUE)) {
        char ocl[MAXOCLASSES + 1], tbuf[MAXOCLASSES + 1], qbuf[QBUFSZ],
            abuf[BUFSZ];
        int oc_sym;
        boolean badopt = FALSE, compat = (strlen(opts) <= 6), use_menu;

        if (duplicate)
            complain_about_duplicate(opts, 1);
        oc_to_str(flags.pickup_types, tbuf);
        flags.pickup_types[0] = '\0'; /* all */
        op = string_for_opt(opts, (compat || !initial));
        if (!op) {
            if (compat || negated || initial) {
                /* for backwards compatibility, "pickup" without a
                   value is a synonym for autopickup of all types
                   (and during initialization, we can't prompt yet) */
                flags.pickup = !negated;
                return;
            }
            oc_to_str(flags.inv_order, ocl);
            use_menu = TRUE;
            if (flags.menu_style == MENU_TRADITIONAL
                || flags.menu_style == MENU_COMBINATION) {
                use_menu = FALSE;
                Sprintf(qbuf, "New pickup_types: [%s am] (%s)", ocl,
                        *tbuf ? tbuf : "all");
                getlin(qbuf, abuf);
                op = mungspaces(abuf);
                if (abuf[0] == '\0' || abuf[0] == '\033')
                    op = tbuf; /* restore */
                else if (abuf[0] == 'm')
                    use_menu = TRUE;
            }
            if (use_menu) {
                (void) choose_classes_menu("Auto-Pickup what?", 1, TRUE, ocl,
                                           tbuf);
                op = tbuf;
            }
        }
        if (negated) {
            bad_negation("pickup_types", TRUE);
            return;
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
            if (badopt)
                badoption(opts);
        }
        return;
    }

    /* pile limit: when walking over objects, number which triggers
       "there are several/many objects here" instead of listing them */
    fullname = "pile_limit";
    if (match_optname(opts, fullname, 4, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        op = string_for_opt(opts, negated);
        if ((negated && !op) || (!negated && op))
            flags.pile_limit = negated ? 0 : atoi(op);
        else if (negated)
            bad_negation(fullname, TRUE);
        else /* !op */
            flags.pile_limit = PILE_LIMIT_DFLT;
        /* sanity check */
        if (flags.pile_limit < 0)
            flags.pile_limit = PILE_LIMIT_DFLT;
        return;
    }

    /* play mode: normal, explore/discovery, or debug/wizard */
    fullname = "playmode";
    if (match_optname(opts, fullname, 4, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (negated)
            bad_negation(fullname, FALSE);
        if (duplicate || negated)
            return;
        op = string_for_opt(opts, TRUE);
        if (!strncmpi(op, "normal", 6) || !strcmpi(op, "play")) {
            wizard = discover = FALSE;
        } else if (!strncmpi(op, "explore", 6)
                   || !strncmpi(op, "discovery", 6)) {
            wizard = FALSE, discover = TRUE;
        } else if (!strncmpi(op, "debug", 5) || !strncmpi(op, "wizard", 6)) {
            wizard = TRUE, discover = FALSE;
        } else {
            raw_printf("Invalid value for \"%s\":%s.", fullname, op);
        }
        return;
    }

    /* WINCAP
     * player_selection: dialog | prompts */
    fullname = "player_selection";
    if (match_optname(opts, fullname, sizeof("player_selection") - 1, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        op = string_for_opt(opts, negated);
        if (op && !negated) {
            if (!strncmpi(op, "dialog", sizeof("dialog") - 1))
                iflags.wc_player_selection = VIA_DIALOG;
            else if (!strncmpi(op, "prompt", sizeof("prompt") - 1))
                iflags.wc_player_selection = VIA_PROMPTS;
            else
                badoption(opts);
        } else if (negated)
            bad_negation(fullname, TRUE);
        return;
    }

    /* things to disclose at end of game */
    if (match_optname(opts, "disclose", 7, TRUE)) {
        /*
         * The order that the end_disclose options are stored:
         *      inventory, attribs, vanquished, genocided,
         *      conduct, overview.
         * There is an array in flags:
         *      end_disclose[NUM_DISCLOSURE_OPT];
         * with option settings for the each of the following:
         * iagvc [see disclosure_options in decl.c]:
         * Legal setting values in that array are:
         *      DISCLOSE_PROMPT_DEFAULT_YES  ask with default answer yes
         *      DISCLOSE_PROMPT_DEFAULT_NO   ask with default answer no
         *      DISCLOSE_YES_WITHOUT_PROMPT  always disclose and don't ask
         *      DISCLOSE_NO_WITHOUT_PROMPT   never disclose and don't ask
         *
         * Those setting values can be used in the option
         * string as a prefix to get the desired behaviour.
         *
         * For backward compatibility, no prefix is required,
         * and the presence of a i,a,g,v, or c without a prefix
         * sets the corresponding value to DISCLOSE_YES_WITHOUT_PROMPT.
         */
        boolean badopt = FALSE;
        int idx, prefix_val;

        if (duplicate)
            complain_about_duplicate(opts, 1);
        op = string_for_opt(opts, TRUE);
        if (op && negated) {
            bad_negation("disclose", TRUE);
            return;
        }
        /* "disclose" without a value means "all with prompting"
           and negated means "none without prompting" */
        if (!op || !strcmpi(op, "all") || !strcmpi(op, "none")) {
            if (op && !strcmpi(op, "none"))
                negated = TRUE;
            for (num = 0; num < NUM_DISCLOSURE_OPTIONS; num++)
                flags.end_disclose[num] = negated
                                              ? DISCLOSE_NO_WITHOUT_PROMPT
                                              : DISCLOSE_PROMPT_DEFAULT_YES;
            return;
        }

        num = 0;
        prefix_val = -1;
        while (*op && num < sizeof flags.end_disclose - 1) {
            static char valid_settings[] = {
                DISCLOSE_PROMPT_DEFAULT_YES, DISCLOSE_PROMPT_DEFAULT_NO,
                DISCLOSE_YES_WITHOUT_PROMPT, DISCLOSE_NO_WITHOUT_PROMPT, '\0'
            };
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
                    flags.end_disclose[idx] = prefix_val;
                    prefix_val = -1;
                } else
                    flags.end_disclose[idx] = DISCLOSE_YES_WITHOUT_PROMPT;
            } else if (index(valid_settings, c)) {
                prefix_val = c;
            } else if (c == ' ') {
                ; /* do nothing */
            } else
                badopt = TRUE;
            op++;
        }
        if (badopt)
            badoption(opts);
        return;
    }

    /* scores:5t[op] 5a[round] o[wn] */
    if (match_optname(opts, "scores", 4, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (negated) {
            bad_negation("scores", FALSE);
            return;
        }
        if (!(op = string_for_opt(opts, FALSE)))
            return;

        while (*op) {
            int inum = 1;

            if (digit(*op)) {
                inum = atoi(op);
                while (digit(*op))
                    op++;
            } else if (*op == '!') {
                negated = !negated;
                op++;
            }
            while (*op == ' ')
                op++;

            switch (*op) {
            case 't':
            case 'T':
                flags.end_top = inum;
                break;
            case 'a':
            case 'A':
                flags.end_around = inum;
                break;
            case 'o':
            case 'O':
                flags.end_own = !negated;
                break;
            default:
                badoption(opts);
                return;
            }
            while (letter(*++op) || *op == ' ')
                continue;
            if (*op == '/')
                op++;
        }
        return;
    }

    fullname = "sortloot";
    if (match_optname(opts, fullname, 4, TRUE)) {
        op = string_for_env_opt(fullname, opts, FALSE);
        if (op) {
            switch (tolower(*op)) {
            case 'n':
            case 'l':
            case 'f':
                flags.sortloot = tolower(*op);
                break;
            default:
                badoption(opts);
                return;
            }
        }
        return;
    }

    fullname = "suppress_alert";
    if (match_optname(opts, fullname, 4, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        op = string_for_opt(opts, negated);
        if (negated)
            bad_negation(fullname, FALSE);
        else if (op)
            (void) feature_alert_opts(op, fullname);
        return;
    }

#ifdef VIDEOSHADES
    /* videocolors:string */
    fullname = "videocolors";
    if (match_optname(opts, fullname, 6, TRUE)
        || match_optname(opts, "videocolours", 10, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (negated) {
            bad_negation(fullname, FALSE);
            return;
        } else if (!(opts = string_for_env_opt(fullname, opts, FALSE))) {
            return;
        }
        if (!assign_videocolors(opts))
            badoption(opts);
        return;
    }
    /* videoshades:string */
    fullname = "videoshades";
    if (match_optname(opts, fullname, 6, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (negated) {
            bad_negation(fullname, FALSE);
            return;
        } else if (!(opts = string_for_env_opt(fullname, opts, FALSE))) {
            return;
        }
        if (!assign_videoshades(opts))
            badoption(opts);
        return;
    }
#endif /* VIDEOSHADES */
#ifdef MSDOS
#ifdef NO_TERMS
    /* video:string -- must be after longer tests */
    fullname = "video";
    if (match_optname(opts, fullname, 5, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (negated) {
            bad_negation(fullname, FALSE);
            return;
        } else if (!(opts = string_for_env_opt(fullname, opts, FALSE))) {
            return;
        }
        if (!assign_video(opts))
            badoption(opts);
        return;
    }
#endif /* NO_TERMS */
    /* soundcard:string -- careful not to match boolean 'sound' */
    fullname = "soundcard";
    if (match_optname(opts, fullname, 6, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (negated) {
            bad_negation(fullname, FALSE);
            return;
        } else if (!(opts = string_for_env_opt(fullname, opts, FALSE))) {
            return;
        }
        if (!assign_soundcard(opts))
            badoption(opts);
        return;
    }
#endif /* MSDOS */

    /* WINCAP
     *
     *  map_mode:[tiles|ascii4x6|ascii6x8|ascii8x8|ascii16x8|ascii7x12|
     *            ascii8x12|ascii16x12|ascii12x16|ascii10x18|fit_to_screen]
     */
    fullname = "map_mode";
    if (match_optname(opts, fullname, sizeof("map_mode") - 1, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        op = string_for_opt(opts, negated);
        if (op && !negated) {
            if (!strncmpi(op, "tiles", sizeof("tiles") - 1))
                iflags.wc_map_mode = MAP_MODE_TILES;
            else if (!strncmpi(op, "ascii4x6", sizeof("ascii4x6") - 1))
                iflags.wc_map_mode = MAP_MODE_ASCII4x6;
            else if (!strncmpi(op, "ascii6x8", sizeof("ascii6x8") - 1))
                iflags.wc_map_mode = MAP_MODE_ASCII6x8;
            else if (!strncmpi(op, "ascii8x8", sizeof("ascii8x8") - 1))
                iflags.wc_map_mode = MAP_MODE_ASCII8x8;
            else if (!strncmpi(op, "ascii16x8", sizeof("ascii16x8") - 1))
                iflags.wc_map_mode = MAP_MODE_ASCII16x8;
            else if (!strncmpi(op, "ascii7x12", sizeof("ascii7x12") - 1))
                iflags.wc_map_mode = MAP_MODE_ASCII7x12;
            else if (!strncmpi(op, "ascii8x12", sizeof("ascii8x12") - 1))
                iflags.wc_map_mode = MAP_MODE_ASCII8x12;
            else if (!strncmpi(op, "ascii16x12", sizeof("ascii16x12") - 1))
                iflags.wc_map_mode = MAP_MODE_ASCII16x12;
            else if (!strncmpi(op, "ascii12x16", sizeof("ascii12x16") - 1))
                iflags.wc_map_mode = MAP_MODE_ASCII12x16;
            else if (!strncmpi(op, "ascii10x18", sizeof("ascii10x18") - 1))
                iflags.wc_map_mode = MAP_MODE_ASCII10x18;
            else if (!strncmpi(op, "fit_to_screen",
                               sizeof("fit_to_screen") - 1))
                iflags.wc_map_mode = MAP_MODE_ASCII_FIT_TO_SCREEN;
            else
                badoption(opts);
        } else if (negated)
            bad_negation(fullname, TRUE);
        return;
    }
    /* WINCAP
     * scroll_amount:nn */
    fullname = "scroll_amount";
    if (match_optname(opts, fullname, sizeof("scroll_amount") - 1, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        op = string_for_opt(opts, negated);
        if ((negated && !op) || (!negated && op)) {
            iflags.wc_scroll_amount = negated ? 1 : atoi(op);
        } else if (negated)
            bad_negation(fullname, TRUE);
        return;
    }
    /* WINCAP
     * scroll_margin:nn */
    fullname = "scroll_margin";
    if (match_optname(opts, fullname, sizeof("scroll_margin") - 1, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        op = string_for_opt(opts, negated);
        if ((negated && !op) || (!negated && op)) {
            iflags.wc_scroll_margin = negated ? 5 : atoi(op);
        } else if (negated)
            bad_negation(fullname, TRUE);
        return;
    }
    fullname = "subkeyvalue";
    if (match_optname(opts, fullname, 5, TRUE)) {
        /* no duplicate complaint here */
        if (negated) {
            bad_negation(fullname, FALSE);
        } else {
#if defined(WIN32)
            op = string_for_opt(opts, 0);
            map_subkeyvalue(op);
#endif
        }
        return;
    }
    /* WINCAP
     * tile_width:nn */
    fullname = "tile_width";
    if (match_optname(opts, fullname, sizeof("tile_width") - 1, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        op = string_for_opt(opts, negated);
        if ((negated && !op) || (!negated && op)) {
            iflags.wc_tile_width = negated ? 0 : atoi(op);
        } else if (negated)
            bad_negation(fullname, TRUE);
        return;
    }
    /* WINCAP
     * tile_file:name */
    fullname = "tile_file";
    if (match_optname(opts, fullname, sizeof("tile_file") - 1, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        if ((op = string_for_opt(opts, FALSE)) != 0) {
            if (iflags.wc_tile_file)
                free(iflags.wc_tile_file);
            iflags.wc_tile_file = (char *) alloc(strlen(op) + 1);
            Strcpy(iflags.wc_tile_file, op);
        }
        return;
    }
    /* WINCAP
     * tile_height:nn */
    fullname = "tile_height";
    if (match_optname(opts, fullname, sizeof("tile_height") - 1, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        op = string_for_opt(opts, negated);
        if ((negated && !op) || (!negated && op)) {
            iflags.wc_tile_height = negated ? 0 : atoi(op);
        } else if (negated)
            bad_negation(fullname, TRUE);
        return;
    }
    /* WINCAP
     * vary_msgcount:nn */
    fullname = "vary_msgcount";
    if (match_optname(opts, fullname, sizeof("vary_msgcount") - 1, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        op = string_for_opt(opts, negated);
        if ((negated && !op) || (!negated && op)) {
            iflags.wc_vary_msgcount = negated ? 0 : atoi(op);
        } else if (negated)
            bad_negation(fullname, TRUE);
        return;
    }
    fullname = "windowtype";
    if (match_optname(opts, fullname, 3, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (negated) {
            bad_negation(fullname, FALSE);
            return;
        } else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0) {
            char buf[WINTYPELEN];
            nmcpy(buf, op, WINTYPELEN);
            choose_windows(buf);
        }
        return;
    }
#ifdef WINCHAIN
    fullname = "windowchain";
    if (match_optname(opts, fullname, 3, TRUE)) {
        if (negated) {
            bad_negation(fullname, FALSE);
            return;
        } else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0) {
            char buf[WINTYPELEN];
            nmcpy(buf, op, WINTYPELEN);
            addto_windowchain(buf);
        }
        return;
    }
#endif

    /* WINCAP
     * setting window colors
     * syntax: windowcolors=menu foregrnd/backgrnd text foregrnd/backgrnd
     */
    fullname = "windowcolors";
    if (match_optname(opts, fullname, 7, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        if ((op = string_for_opt(opts, FALSE)) != 0) {
            if (!wc_set_window_colors(op))
                badoption(opts);
        } else if (negated)
            bad_negation(fullname, TRUE);
        return;
    }

    /* menustyle:traditional or combination or full or partial */
    if (match_optname(opts, "menustyle", 4, TRUE)) {
        int tmp;
        boolean val_required = (strlen(opts) > 5 && !negated);

        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (!(op = string_for_opt(opts, !val_required))) {
            if (val_required)
                return; /* string_for_opt gave feedback */
            tmp = negated ? 'n' : 'f';
        } else {
            tmp = tolower(*op);
        }
        switch (tmp) {
        case 'n': /* none */
        case 't': /* traditional */
            flags.menu_style = MENU_TRADITIONAL;
            break;
        case 'c': /* combo: trad.class sel+menu */
            flags.menu_style = MENU_COMBINATION;
            break;
        case 'p': /* partial: no class menu */
            flags.menu_style = MENU_PARTIAL;
            break;
        case 'f': /* full: class menu + menu */
            flags.menu_style = MENU_FULL;
            break;
        default:
            badoption(opts);
        }
        return;
    }

    fullname = "menu_headings";
    if (match_optname(opts, fullname, 12, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (negated) {
            bad_negation(fullname, FALSE);
            return;
        } else if (!(opts = string_for_env_opt(fullname, opts, FALSE))) {
            return;
        }
        for (i = 0; i < SIZE(attrnames); i++)
            if (!strcmpi(opts, attrnames[i].name)) {
                iflags.menu_headings = attrnames[i].attr;
                return;
            }
        badoption(opts);
        return;
    }

    /* check for menu command mapping */
    for (i = 0; i < NUM_MENU_CMDS; i++) {
        fullname = default_menu_cmd_info[i].name;
        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (match_optname(opts, fullname, (int) strlen(fullname), TRUE)) {
            if (negated) {
                bad_negation(fullname, FALSE);
            } else if ((op = string_for_opt(opts, FALSE)) != 0) {
                int j;
                char c, op_buf[BUFSZ];
                boolean isbad = FALSE;

                escapes(op, op_buf);
                c = *op_buf;

                if (c == 0 || c == '\r' || c == '\n' || c == '\033'
                    || c == ' ' || digit(c) || (letter(c) && c != '@'))
                    isbad = TRUE;
                else /* reject default object class symbols */
                    for (j = 1; j < MAXOCLASSES; j++)
                        if (c == def_oc_syms[i].sym) {
                            isbad = TRUE;
                            break;
                        }

                if (isbad)
                    badoption(opts);
                else
                    add_menu_cmd_alias(c, default_menu_cmd_info[i].cmd);
            }
            return;
        }
    }
#if defined(STATUS_VIA_WINDOWPORT) && defined(STATUS_HILITES)
    /* hilite fields in status prompt */
    if (match_optname(opts, "hilite_status", 13, TRUE)) {
        if (duplicate)
            complain_about_duplicate(opts, 1);
        op = string_for_opt(opts, TRUE);
        if (op && negated) {
            clear_status_hilites(tfrom_file);
            return;
        } else if (!op) {
            /* a value is mandatory */
            badoption(opts);
            return;
        }
        if (!set_status_hilites(op, tfrom_file))
            badoption(opts);
        return;
    }
#endif

#if defined(BACKWARD_COMPAT)
    fullname = "DECgraphics";
    if (match_optname(opts, fullname, 3, TRUE)) {
        boolean badflag = FALSE;

        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (!negated) {
            /* There is no rogue level DECgraphics-specific set */
            if (symset[PRIMARY].name) {
                badflag = TRUE;
            } else {
                symset[PRIMARY].name = dupstr(fullname);
                if (!read_sym_file(PRIMARY)) {
                    badflag = TRUE;
                    clear_symsetentry(PRIMARY, TRUE);
                } else
                    switch_symbols(TRUE);
            }
            if (badflag) {
                pline("Failure to load symbol set %s.", fullname);
                wait_synch();
            }
        }
        return;
    }
    fullname = "IBMgraphics";
    if (match_optname(opts, fullname, 3, TRUE)) {
        const char *sym_name = fullname;
        boolean badflag = FALSE;

        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (!negated) {
            for (i = 0; i < NUM_GRAPHICS; ++i) {
                if (symset[i].name) {
                    badflag = TRUE;
                } else {
                    if (i == ROGUESET)
                        sym_name = "RogueIBM";
                    symset[i].name = dupstr(sym_name);
                    if (!read_sym_file(i)) {
                        badflag = TRUE;
                        clear_symsetentry(i, TRUE);
                        break;
                    }
                }
            }
            if (badflag) {
                pline("Failure to load symbol set %s.", sym_name);
                wait_synch();
            } else {
                switch_symbols(TRUE);
                if (!initial && Is_rogue_level(&u.uz))
                    assign_graphics(ROGUESET);
            }
        }
        return;
    }
#endif
#ifdef MAC_GRAPHICS_ENV
    fullname = "MACgraphics";
    if (match_optname(opts, fullname, 3, TRUE)) {
        boolean badflag = FALSE;

        if (duplicate)
            complain_about_duplicate(opts, 1);
        if (!negated) {
            if (symset[PRIMARY].name) {
                badflag = TRUE;
            } else {
                symset[PRIMARY].name = dupstr(fullname);
                if (!read_sym_file(PRIMARY)) {
                    badflag = TRUE;
                    clear_symsetentry(PRIMARY, TRUE);
                }
            }
            if (badflag) {
                pline("Failure to load symbol set %s.", fullname);
                wait_synch();
            } else {
                switch_symbols(TRUE);
                if (!initial && Is_rogue_level(&u.uz))
                    assign_graphics(ROGUESET);
            }
        }
        return;
    }
#endif

    /* OK, if we still haven't recognized the option, check the boolean
     * options list
     */
    for (i = 0; boolopt[i].name; i++) {
        if (match_optname(opts, boolopt[i].name, 3, FALSE)) {
            /* options that don't exist */
            if (!boolopt[i].addr) {
                if (!initial && !negated)
                    pline_The("\"%s\" option is not available.",
                              boolopt[i].name);
                return;
            }
            /* options that must come from config file */
            if (!initial && (boolopt[i].optflags == SET_IN_FILE)) {
                rejectoption(boolopt[i].name);
                return;
            }

            *(boolopt[i].addr) = !negated;

            /* 0 means boolean opts */
            if (duplicate_opt_detection(boolopt[i].name, 0))
                complain_about_duplicate(boolopt[i].name, 0);

#ifdef RLECOMP
            if ((boolopt[i].addr) == &iflags.rlecomp) {
                if (*boolopt[i].addr)
                    set_savepref("rlecomp");
                else
                    set_savepref("!rlecomp");
            }
#endif
#ifdef ZEROCOMP
            if ((boolopt[i].addr) == &iflags.zerocomp) {
                if (*boolopt[i].addr)
                    set_savepref("zerocomp");
                else
                    set_savepref("externalcomp");
            }
#endif
            /* only do processing below if setting with doset() */
            if (initial)
                return;

            if ((boolopt[i].addr) == &flags.time
                || (boolopt[i].addr) == &flags.showexp
#ifdef SCORE_ON_BOTL
                || (boolopt[i].addr) == &flags.showscore
#endif
                ) {
#ifdef STATUS_VIA_WINDOWPORT
                status_initialize(REASSESS_ONLY);
#endif
                context.botl = TRUE;
            } else if ((boolopt[i].addr) == &flags.invlet_constant) {
                if (flags.invlet_constant)
                    reassign();
            } else if (((boolopt[i].addr) == &flags.lit_corridor)
                       || ((boolopt[i].addr) == &flags.dark_room)) {
                /*
                 * All corridor squares seen via night vision or
                 * candles & lamps change.  Update them by calling
                 * newsym() on them.  Don't do this if we are
                 * initializing the options --- the vision system
                 * isn't set up yet.
                 */
                vision_recalc(2);       /* shut down vision */
                vision_full_recalc = 1; /* delayed recalc */
                if (iflags.use_color)
                    need_redraw = TRUE; /* darkroom refresh */
            } else if ((boolopt[i].addr) == &iflags.use_inverse
                       || (boolopt[i].addr) == &flags.showrace
                       || (boolopt[i].addr) == &iflags.hilite_pet) {
                need_redraw = TRUE;
#ifdef TEXTCOLOR
            } else if ((boolopt[i].addr) == &iflags.use_color) {
                need_redraw = TRUE;
#ifdef TOS
                if ((boolopt[i].addr) == &iflags.use_color && iflags.BIOS) {
                    if (colors_changed)
                        restore_colors();
                    else
                        set_colors();
                }
#endif
#endif /* TEXTCOLOR */
            }
            return;
        }
    }

    /* out of valid options */
    badoption(opts);
}

static NEARDATA const char *menutype[] = { "traditional", "combination",
                                           "full", "partial" };

static NEARDATA const char *burdentype[] = { "unencumbered", "burdened",
                                             "stressed",     "strained",
                                             "overtaxed",    "overloaded" };

static NEARDATA const char *runmodes[] = { "teleport", "run", "walk",
                                           "crawl" };

static NEARDATA const char *sortltype[] = { "none", "loot", "full" };

/*
 * Convert the given string of object classes to a string of default object
 * symbols.
 */
STATIC_OVL void
oc_to_str(src, dest)
char *src, *dest;
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
add_menu_cmd_alias(from_ch, to_ch)
char from_ch, to_ch;
{
    if (n_menu_mapped >= MAX_MENU_MAPPED_CMDS) {
        pline("out of menu map space.");
    } else {
        mapped_menu_cmds[n_menu_mapped] = from_ch;
        mapped_menu_op[n_menu_mapped] = to_ch;
        n_menu_mapped++;
        mapped_menu_cmds[n_menu_mapped] = 0;
        mapped_menu_op[n_menu_mapped] = 0;
    }
}

/*
 * Map the given character to its corresponding menu command.  If it
 * doesn't match anything, just return the original.
 */
char
map_menu_cmd(ch)
char ch;
{
    char *found = index(mapped_menu_cmds, ch);
    if (found) {
        int idx = (int) (found - mapped_menu_cmds);
        ch = mapped_menu_op[idx];
    }
    return ch;
}

#if defined(MICRO) || defined(MAC) || defined(WIN32)
#define OPTIONS_HEADING "OPTIONS"
#else
#define OPTIONS_HEADING "NETHACKOPTIONS"
#endif

static char fmtstr_doset_add_menu[] = "%s%-15s [%s]   ";
static char fmtstr_doset_add_menu_tab[] = "%s\t[%s]";

STATIC_OVL void
doset_add_menu(win, option, indexoffset)
winid win;          /* window to add to */
const char *option; /* option name */
int indexoffset;    /* value to add to index in compopt[], or zero
                       if option cannot be changed */
{
    const char *value = "unknown"; /* current value */
    char buf[BUFSZ], buf2[BUFSZ];
    anything any;
    int i;

    any = zeroany;
    if (indexoffset == 0) {
        any.a_int = 0;
        value = get_compopt_value(option, buf2);
    } else {
        for (i = 0; compopt[i].name; i++)
            if (strcmp(option, compopt[i].name) == 0)
                break;

        if (compopt[i].name) {
            any.a_int = i + 1 + indexoffset;
            value = get_compopt_value(option, buf2);
        } else {
            /* We are trying to add an option not found in compopt[].
               This is almost certainly bad, but we'll let it through anyway
               (with a zero value, so it can't be selected). */
            any.a_int = 0;
        }
    }
    /* "    " replaces "a - " -- assumes menus follow that style */
    if (!iflags.menu_tab_sep)
        Sprintf(buf, fmtstr_doset_add_menu, any.a_int ? "" : "    ", option,
                value);
    else
        Sprintf(buf, fmtstr_doset_add_menu_tab, option, value);
    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, MENU_UNSELECTED);
}

/* Changing options via menu by Per Liboriussen */
int
doset()
{
    char buf[BUFSZ], buf2[BUFSZ];
    int i = 0, pass, boolcount, pick_cnt, pick_idx, opt_indx;
    boolean *bool_p;
    winid tmpwin;
    anything any;
    menu_item *pick_list;
    int indexoffset, startpass, endpass;
    boolean setinitial = FALSE, fromfile = FALSE;
    int biggest_name = 0;
    const char *n_currently_set = "(%d currently set)";

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin);

    any = zeroany;
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, iflags.menu_headings,
             "Booleans (selecting will toggle value):", MENU_UNSELECTED);
    any.a_int = 0;
    /* first list any other non-modifiable booleans, then modifiable ones */
    for (pass = 0; pass <= 1; pass++)
        for (i = 0; boolopt[i].name; i++)
            if ((bool_p = boolopt[i].addr) != 0
                && ((boolopt[i].optflags == DISP_IN_GAME && pass == 0)
                    || (boolopt[i].optflags == SET_IN_GAME && pass == 1))) {
                if (bool_p == &flags.female)
                    continue; /* obsolete */
                if (bool_p == &iflags.sanity_check && !wizard)
                    continue;
                if (bool_p == &iflags.menu_tab_sep && !wizard)
                    continue;
                if (is_wc_option(boolopt[i].name)
                    && !wc_supported(boolopt[i].name))
                    continue;
                if (is_wc2_option(boolopt[i].name)
                    && !wc2_supported(boolopt[i].name))
                    continue;
                any.a_int = (pass == 0) ? 0 : i + 1;
                if (!iflags.menu_tab_sep)
                    Sprintf(buf, "%s%-17s [%s]", pass == 0 ? "    " : "",
                            boolopt[i].name, *bool_p ? "true" : "false");
                else
                    Sprintf(buf, "%s\t[%s]", boolopt[i].name,
                            *bool_p ? "true" : "false");
                add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf,
                         MENU_UNSELECTED);
            }

    boolcount = i;
    indexoffset = boolcount;
    any = zeroany;
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, iflags.menu_headings,
             "Compounds (selecting will prompt for new value):",
             MENU_UNSELECTED);

#ifdef notyet /* SYSCF */
    /* XXX I think this is still fragile.  Fixing initial/from_file and/or
     changing
     the SET_* etc to bitmaps will let me make this better. */
    if (wizard)
        startpass = SET_IN_SYS;
    else
#endif
        startpass = DISP_IN_GAME;
    endpass = SET_IN_GAME;

    /* spin through the options to find the biggest name
       and adjust the format string accordingly if needed */
    biggest_name = 0;
    for (i = 0; compopt[i].name; i++)
        if (compopt[i].optflags >= startpass && compopt[i].optflags <= endpass
            && strlen(compopt[i].name) > (unsigned) biggest_name)
            biggest_name = (int) strlen(compopt[i].name);
    if (biggest_name > 30)
        biggest_name = 30;
    if (!iflags.menu_tab_sep)
        Sprintf(fmtstr_doset_add_menu, "%%s%%-%ds [%%s]", biggest_name);

    /* deliberately put `playmode', `name', `role', `race', `gender' first
       (also alignment if anything ever comes before it in compopt[]) */
    doset_add_menu(tmpwin, "playmode", 0);
    doset_add_menu(tmpwin, "name", 0);
    doset_add_menu(tmpwin, "role", 0);
    doset_add_menu(tmpwin, "race", 0);
    doset_add_menu(tmpwin, "gender", 0);

    for (pass = startpass; pass <= endpass; pass++)
        for (i = 0; compopt[i].name; i++)
            if (compopt[i].optflags == pass) {
                if (!strcmp(compopt[i].name, "playmode")
                    || !strcmp(compopt[i].name, "name")
                    || !strcmp(compopt[i].name, "role")
                    || !strcmp(compopt[i].name, "race")
                    || !strcmp(compopt[i].name, "gender"))
                    continue;
                else if (is_wc_option(compopt[i].name)
                         && !wc_supported(compopt[i].name))
                    continue;
                else if (is_wc2_option(compopt[i].name)
                         && !wc2_supported(compopt[i].name))
                    continue;
                else
                    doset_add_menu(tmpwin, compopt[i].name,
                                   (pass == DISP_IN_GAME) ? 0 : indexoffset);
            }
    any.a_int = -4;
    Sprintf(buf2, n_currently_set, msgtype_count());
    Sprintf(buf, fmtstr_doset_add_menu, any.a_int ? "" : "    ",
            "message types", buf2);
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, MENU_UNSELECTED);
    any.a_int = -3;
    Sprintf(buf2, n_currently_set, count_menucolors());
    Sprintf(buf, fmtstr_doset_add_menu, any.a_int ? "" : "    ",
            "menucolors", buf2);
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, MENU_UNSELECTED);
#ifdef STATUS_VIA_WINDOWPORT
#ifdef STATUS_HILITES
    any.a_int = -2;
    get_status_hilites(buf2, 60);
    if (!*buf2)
        Sprintf(buf2, "%s", "(none)");
    if (!iflags.menu_tab_sep)
        Sprintf(buf, fmtstr_doset_add_menu, any.a_int ? "" : "    ",
                "status_hilites", buf2);
    else
        Sprintf(buf, fmtstr_doset_add_menu_tab, "status_hilites", buf2);
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, MENU_UNSELECTED);
#endif
#endif
    any.a_int = -1;
    Sprintf(buf2, n_currently_set, count_ape_maps((int *) 0, (int *) 0));
    Sprintf(buf, fmtstr_doset_add_menu, any.a_int ? "" : "    ",
            "autopickup exceptions", buf2);
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, MENU_UNSELECTED);
#ifdef PREFIXES_IN_USE
    any = zeroany;
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, iflags.menu_headings,
             "Variable playground locations:", MENU_UNSELECTED);
    for (i = 0; i < PREFIX_COUNT; i++)
        doset_add_menu(tmpwin, fqn_prefix_names[i], 0);
#endif
    end_menu(tmpwin, "Set what options?");
    need_redraw = FALSE;
    if ((pick_cnt = select_menu(tmpwin, PICK_ANY, &pick_list)) > 0) {
        /*
         * Walk down the selection list and either invert the booleans
         * or prompt for new values. In most cases, call parseoptions()
         * to take care of options that require special attention, like
         * redraws.
         */
        for (pick_idx = 0; pick_idx < pick_cnt; ++pick_idx) {
            opt_indx = pick_list[pick_idx].item.a_int - 1;
            if (opt_indx == -2) {
                /* -2 due to -1 offset for select_menu() */
                (void) special_handling("autopickup_exception", setinitial,
                                        fromfile);
#ifdef STATUS_VIA_WINDOWPORT
#ifdef STATUS_HILITES
            } else if (opt_indx == -3) {
                /* -3 due to -1 offset for select_menu() */
                if (!status_hilite_menu()) {
                    pline("Bad status hilite(s) specified.");
                } else {
                    if (wc2_supported("status_hilites"))
                        preference_update("status_hilites");
                }
#endif
#endif
            } else if (opt_indx == -4) {
                    (void) special_handling("menucolors", setinitial,
                                            fromfile);
            } else if (opt_indx == -5) {
                    (void) special_handling("msgtype", setinitial, fromfile);
            } else if (opt_indx < boolcount) {
                /* boolean option */
                Sprintf(buf, "%s%s", *boolopt[opt_indx].addr ? "!" : "",
                        boolopt[opt_indx].name);
                parseoptions(buf, setinitial, fromfile);
                if (wc_supported(boolopt[opt_indx].name)
                    || wc2_supported(boolopt[opt_indx].name))
                    preference_update(boolopt[opt_indx].name);
            } else {
                /* compound option */
                opt_indx -= boolcount;

                if (!special_handling(compopt[opt_indx].name, setinitial,
                                      fromfile)) {
                    Sprintf(buf, "Set %s to what?", compopt[opt_indx].name);
                    getlin(buf, buf2);
                    if (buf2[0] == '\033')
                        continue;
                    Sprintf(buf, "%s:%s", compopt[opt_indx].name, buf2);
                    /* pass the buck */
                    parseoptions(buf, setinitial, fromfile);
                }
                if (wc_supported(compopt[opt_indx].name)
                    || wc2_supported(compopt[opt_indx].name))
                    preference_update(compopt[opt_indx].name);
            }
        }
        free((genericptr_t) pick_list);
        pick_list = (menu_item *) 0;
    }

    destroy_nhwindow(tmpwin);
    if (need_redraw) {
        reglyph_darkroom();
        (void) doredraw();
    }
    return 0;
}

int
handle_add_list_remove(optname, numtotal)
const char *optname;
int numtotal;
{
    winid tmpwin;
    anything any;
    int i, pick_cnt, pick_idx, opt_idx;
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

    opt_idx = 0;
    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin);
    any = zeroany;
    for (i = 0; i < SIZE(action_titles); i++) {
        char tmpbuf[BUFSZ];
        any.a_int++;
        /* omit list and remove if there aren't any yet */
        if (!numtotal && (i == 1 || i == 2))
            continue;
        Sprintf(tmpbuf, action_titles[i].desc,
                (i == 1) ? makeplural(optname) : optname);
        add_menu(tmpwin, NO_GLYPH, &any, action_titles[i].letr, 0, ATR_NONE,
                 tmpbuf,
#if 0 /* this ought to work but doesn't... */
                 (action_titles[i].letr == 'x') ? MENU_SELECTED :
#endif
                 MENU_UNSELECTED);
    }
    end_menu(tmpwin, "Do what?");
    if ((pick_cnt = select_menu(tmpwin, PICK_ONE, &pick_list)) > 0) {
        for (pick_idx = 0; pick_idx < pick_cnt; ++pick_idx) {
            opt_idx = pick_list[pick_idx].item.a_int - 1;
        }
        free((genericptr_t) pick_list);
        pick_list = (menu_item *) 0;
    }
    destroy_nhwindow(tmpwin);

    if (pick_cnt < 1)
        opt_idx = 3; /* none selected, exit menu */
    return opt_idx;
}

struct symsetentry *symset_list = 0; /* files.c will populate this with
                                              list of available sets */

STATIC_OVL boolean
special_handling(optname, setinitial, setfromfile)
const char *optname;
boolean setinitial, setfromfile;
{
    winid tmpwin;
    anything any;
    int i;
    char buf[BUFSZ];

    /* Special handling of menustyle, pickup_burden, pickup_types,
     * disclose, runmode, msg_window, menu_headings, sortloot,
     * and number_pad options.
     * Also takes care of interactive autopickup_exception_handling changes.
     */
    if (!strcmp("menustyle", optname)) {
        const char *style_name;
        menu_item *style_pick = (menu_item *) 0;
        tmpwin = create_nhwindow(NHW_MENU);
        start_menu(tmpwin);
        any = zeroany;
        for (i = 0; i < SIZE(menutype); i++) {
            style_name = menutype[i];
            /* note: separate `style_name' variable used
               to avoid an optimizer bug in VAX C V2.3 */
            any.a_int = i + 1;
            add_menu(tmpwin, NO_GLYPH, &any, *style_name, 0, ATR_NONE,
                     style_name, MENU_UNSELECTED);
        }
        end_menu(tmpwin, "Select menustyle:");
        if (select_menu(tmpwin, PICK_ONE, &style_pick) > 0) {
            flags.menu_style = style_pick->item.a_int - 1;
            free((genericptr_t) style_pick);
        }
        destroy_nhwindow(tmpwin);
    } else if (!strcmp("paranoid_confirmation", optname)) {
        menu_item *paranoia_picks = (menu_item *) 0;

        tmpwin = create_nhwindow(NHW_MENU);
        start_menu(tmpwin);
        any = zeroany;
        for (i = 0; paranoia[i].flagmask != 0; ++i) {
            if (paranoia[i].flagmask == PARANOID_BONES && !wizard)
                continue;
            any.a_int = paranoia[i].flagmask;
            add_menu(tmpwin, NO_GLYPH, &any, *paranoia[i].argname, 0,
                     ATR_NONE, paranoia[i].explain,
                     (flags.paranoia_bits & paranoia[i].flagmask)
                         ? MENU_SELECTED
                         : MENU_UNSELECTED);
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
    } else if (!strcmp("pickup_burden", optname)) {
        const char *burden_name, *burden_letters = "ubsntl";
        menu_item *burden_pick = (menu_item *) 0;

        tmpwin = create_nhwindow(NHW_MENU);
        start_menu(tmpwin);
        any = zeroany;
        for (i = 0; i < SIZE(burdentype); i++) {
            burden_name = burdentype[i];
            any.a_int = i + 1;
            add_menu(tmpwin, NO_GLYPH, &any, burden_letters[i], 0, ATR_NONE,
                     burden_name, MENU_UNSELECTED);
        }
        end_menu(tmpwin, "Select encumbrance level:");
        if (select_menu(tmpwin, PICK_ONE, &burden_pick) > 0) {
            flags.pickup_burden = burden_pick->item.a_int - 1;
            free((genericptr_t) burden_pick);
        }
        destroy_nhwindow(tmpwin);
    } else if (!strcmp("pickup_types", optname)) {
        /* parseoptions will prompt for the list of types */
        parseoptions(strcpy(buf, "pickup_types"), setinitial, setfromfile);
    } else if (!strcmp("disclose", optname)) {
        /* order of disclose_names[] must correspond to
           disclosure_options in decl.c */
        static const char *disclosure_names[] = {
            "inventory", "attributes", "vanquished",
            "genocides", "conduct",    "overview",
        };
        int disc_cat[NUM_DISCLOSURE_OPTIONS];
        int pick_cnt, pick_idx, opt_idx;
        menu_item *disclosure_pick = (menu_item *) 0;

        tmpwin = create_nhwindow(NHW_MENU);
        start_menu(tmpwin);
        any = zeroany;
        for (i = 0; i < NUM_DISCLOSURE_OPTIONS; i++) {
            Sprintf(buf, "%-12s[%c%c]", disclosure_names[i],
                    flags.end_disclose[i], disclosure_options[i]);
            any.a_int = i + 1;
            add_menu(tmpwin, NO_GLYPH, &any, disclosure_options[i], 0,
                     ATR_NONE, buf, MENU_UNSELECTED);
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
                Sprintf(buf, "Disclosure options for %s:",
                        disclosure_names[i]);
                tmpwin = create_nhwindow(NHW_MENU);
                start_menu(tmpwin);
                any = zeroany;
                /* 'y','n',and '+' work as alternate selectors; '-' doesn't */
                any.a_char = DISCLOSE_NO_WITHOUT_PROMPT;
                add_menu(tmpwin, NO_GLYPH, &any, 'a', any.a_char, ATR_NONE,
                         "Never disclose, without prompting",
                         MENU_UNSELECTED);
                any.a_char = DISCLOSE_YES_WITHOUT_PROMPT;
                add_menu(tmpwin, NO_GLYPH, &any, 'b', any.a_char, ATR_NONE,
                         "Always disclose, without prompting",
                         MENU_UNSELECTED);
                any.a_char = DISCLOSE_PROMPT_DEFAULT_NO;
                add_menu(tmpwin, NO_GLYPH, &any, 'c', any.a_char, ATR_NONE,
                         "Prompt, with default answer of \"No\"",
                         MENU_UNSELECTED);
                any.a_char = DISCLOSE_PROMPT_DEFAULT_YES;
                add_menu(tmpwin, NO_GLYPH, &any, 'd', any.a_char, ATR_NONE,
                         "Prompt, with default answer of \"Yes\"",
                         MENU_UNSELECTED);
                end_menu(tmpwin, buf);
                if (select_menu(tmpwin, PICK_ONE, &disclosure_pick) > 0) {
                    flags.end_disclose[i] = disclosure_pick->item.a_char;
                    free((genericptr_t) disclosure_pick);
                }
                destroy_nhwindow(tmpwin);
            }
        }
    } else if (!strcmp("runmode", optname)) {
        const char *mode_name;
        menu_item *mode_pick = (menu_item *) 0;

        tmpwin = create_nhwindow(NHW_MENU);
        start_menu(tmpwin);
        any = zeroany;
        for (i = 0; i < SIZE(runmodes); i++) {
            mode_name = runmodes[i];
            any.a_int = i + 1;
            add_menu(tmpwin, NO_GLYPH, &any, *mode_name, 0, ATR_NONE,
                     mode_name, MENU_UNSELECTED);
        }
        end_menu(tmpwin, "Select run/travel display mode:");
        if (select_menu(tmpwin, PICK_ONE, &mode_pick) > 0) {
            flags.runmode = mode_pick->item.a_int - 1;
            free((genericptr_t) mode_pick);
        }
        destroy_nhwindow(tmpwin);
    } else if (!strcmp("msg_window", optname)) {
#ifdef TTY_GRAPHICS
        /* by Christian W. Cooper */
        menu_item *window_pick = (menu_item *) 0;

        tmpwin = create_nhwindow(NHW_MENU);
        start_menu(tmpwin);
        any = zeroany;
        any.a_char = 's';
        add_menu(tmpwin, NO_GLYPH, &any, 's', 0, ATR_NONE, "single",
                 MENU_UNSELECTED);
        any.a_char = 'c';
        add_menu(tmpwin, NO_GLYPH, &any, 'c', 0, ATR_NONE, "combination",
                 MENU_UNSELECTED);
        any.a_char = 'f';
        add_menu(tmpwin, NO_GLYPH, &any, 'f', 0, ATR_NONE, "full",
                 MENU_UNSELECTED);
        any.a_char = 'r';
        add_menu(tmpwin, NO_GLYPH, &any, 'r', 0, ATR_NONE, "reversed",
                 MENU_UNSELECTED);
        end_menu(tmpwin, "Select message history display type:");
        if (select_menu(tmpwin, PICK_ONE, &window_pick) > 0) {
            iflags.prevmsg_window = window_pick->item.a_char;
            free((genericptr_t) window_pick);
        }
        destroy_nhwindow(tmpwin);
#endif
    } else if (!strcmp("sortloot", optname)) {
        const char *sortl_name;
        menu_item *sortl_pick = (menu_item *) 0;

        tmpwin = create_nhwindow(NHW_MENU);
        start_menu(tmpwin);
        any = zeroany;
        for (i = 0; i < SIZE(sortltype); i++) {
            sortl_name = sortltype[i];
            any.a_char = *sortl_name;
            add_menu(tmpwin, NO_GLYPH, &any, *sortl_name, 0, ATR_NONE,
                     sortl_name, MENU_UNSELECTED);
        }
        end_menu(tmpwin, "Select loot sorting type:");
        if (select_menu(tmpwin, PICK_ONE, &sortl_pick) > 0) {
            flags.sortloot = sortl_pick->item.a_char;
            free((genericptr_t) sortl_pick);
        }
        destroy_nhwindow(tmpwin);
    } else if (!strcmp("align_message", optname)
               || !strcmp("align_status", optname)) {
        menu_item *window_pick = (menu_item *) 0;
        char abuf[BUFSZ];
        boolean msg = (*(optname + 6) == 'm');

        tmpwin = create_nhwindow(NHW_MENU);
        start_menu(tmpwin);
        any = zeroany;
        any.a_int = ALIGN_TOP;
        add_menu(tmpwin, NO_GLYPH, &any, 't', 0, ATR_NONE, "top",
                 MENU_UNSELECTED);
        any.a_int = ALIGN_BOTTOM;
        add_menu(tmpwin, NO_GLYPH, &any, 'b', 0, ATR_NONE, "bottom",
                 MENU_UNSELECTED);
        any.a_int = ALIGN_LEFT;
        add_menu(tmpwin, NO_GLYPH, &any, 'l', 0, ATR_NONE, "left",
                 MENU_UNSELECTED);
        any.a_int = ALIGN_RIGHT;
        add_menu(tmpwin, NO_GLYPH, &any, 'r', 0, ATR_NONE, "right",
                 MENU_UNSELECTED);
        Sprintf(abuf, "Select %s window placement relative to the map:",
                msg ? "message" : "status");
        end_menu(tmpwin, abuf);
        if (select_menu(tmpwin, PICK_ONE, &window_pick) > 0) {
            if (msg)
                iflags.wc_align_message = window_pick->item.a_int;
            else
                iflags.wc_align_status = window_pick->item.a_int;
            free((genericptr_t) window_pick);
        }
        destroy_nhwindow(tmpwin);
    } else if (!strcmp("number_pad", optname)) {
        static const char *npchoices[] = {
            " 0 (off)", " 1 (on)", " 2 (on, MSDOS compatible)",
            " 3 (on, phone-style digit layout)",
            " 4 (on, phone-style layout, MSDOS compatible)",
            "-1 (off, 'z' to move upper-left, 'y' to zap wands)"
        };
        menu_item *mode_pick = (menu_item *) 0;

        tmpwin = create_nhwindow(NHW_MENU);
        start_menu(tmpwin);
        any = zeroany;
        for (i = 0; i < SIZE(npchoices); i++) {
            any.a_int = i + 1;
            add_menu(tmpwin, NO_GLYPH, &any, 'a' + i, 0, ATR_NONE,
                     npchoices[i], MENU_UNSELECTED);
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
    } else if (!strcmp("menu_headings", optname)) {
        int mhattr = query_attr("How to highlight menu headings:");

        if (mhattr != -1)
            iflags.menu_headings = mhattr;
    } else if (!strcmp("msgtype", optname)) {
        int opt_idx, nmt, mttyp;
        char mtbuf[BUFSZ];

    msgtypes_again:
        nmt = msgtype_count();
        opt_idx = handle_add_list_remove("message type", nmt);
        if (opt_idx == 3) {
            ; /* done--fall through to function exit */
        } else if (opt_idx == 0) { /* add new */
            getlin("What new message pattern?", mtbuf);
            if (*mtbuf == '\033' || !*mtbuf)
                goto msgtypes_again;
            mttyp = query_msgtype();
            if (mttyp == -1)
                goto msgtypes_again;
            if (!msgtype_add(mttyp, mtbuf)) {
                pline("Error adding the message type.");
                wait_synch();
                goto msgtypes_again;
            }
        } else { /* list or remove */
            int pick_idx, pick_cnt;
            int mt_idx;
            menu_item *pick_list = (menu_item *) 0;
            struct plinemsg_type *tmp = plinemsg_types;

            tmpwin = create_nhwindow(NHW_MENU);
            start_menu(tmpwin);
            any = zeroany;
            mt_idx = 0;
            while (tmp) {
                const char *mtype = msgtype2name(tmp->msgtype);

                any.a_int = ++mt_idx;
                Sprintf(mtbuf, "%-5s \"%s\"", mtype, tmp->pattern);
                add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, mtbuf,
                         MENU_UNSELECTED);
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
    } else if (!strcmp("menucolors", optname)) {
        int opt_idx, nmc, mcclr, mcattr;
        char mcbuf[BUFSZ];

    menucolors_again:
        nmc = count_menucolors();
        opt_idx = handle_add_list_remove("menucolor", nmc);
        if (opt_idx == 3) {
            ;                      /* done--fall through to function exit */
        } else if (opt_idx == 0) { /* add new */
            getlin("What new menucolor pattern?", mcbuf);
            if (*mcbuf == '\033' || !*mcbuf)
                goto menucolors_again;
            mcclr = query_color();
            if (mcclr == -1)
                goto menucolors_again;
            mcattr = query_attr(NULL);
            if (mcattr == -1)
                goto menucolors_again;
            if (!add_menu_coloring_parsed(mcbuf, mcclr, mcattr)) {
                pline("Error adding the menu color.");
                wait_synch();
                goto menucolors_again;
            }
        } else { /* list or remove */
            int pick_idx, pick_cnt;
            int mc_idx;
            menu_item *pick_list = (menu_item *) 0;
            struct menucoloring *tmp = menu_colorings;

            tmpwin = create_nhwindow(NHW_MENU);
            start_menu(tmpwin);
            any = zeroany;
            mc_idx = 0;
            while (tmp) {
                const char *sattr = attr2attrname(tmp->attr);
                const char *sclr = clr2colorname(tmp->color);

                any.a_int = (++mc_idx);
                Sprintf(mcbuf, "\"%s\"=%s%s%s", tmp->origstr, sclr,
                        (tmp->attr != ATR_NONE) ? " & " : "",
                        (tmp->attr != ATR_NONE) ? sattr : "");
                add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, mcbuf,
                         MENU_UNSELECTED);
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
    } else if (!strcmp("autopickup_exception", optname)) {
        int opt_idx, pass, totalapes = 0, numapes[2] = { 0, 0 };
        char apebuf[1 + BUFSZ]; /* so &apebuf[1] is BUFSZ long for getlin() */
        struct autopickup_exception *ape;

    ape_again:
        totalapes = count_ape_maps(&numapes[AP_LEAVE], &numapes[AP_GRAB]);
        opt_idx = handle_add_list_remove("autopickup exception", totalapes);
        if (opt_idx == 3) {
            ;                      /* done--fall through to function exit */
        } else if (opt_idx == 0) { /* add new */
            getlin("What new autopickup exception pattern?", &apebuf[1]);
            mungspaces(&apebuf[1]); /* regularize whitespace */
            if (apebuf[1] == '\033') {
                ; /* fall through to function exit */
            } else {
                if (apebuf[1]) {
                    apebuf[0] = '\"';
                    /* guarantee room for \" prefix and \"\0 suffix;
                       -2 is good enough for apebuf[] but -3 makes
                       sure the whole thing fits within normal BUFSZ */
                    apebuf[sizeof apebuf - 3] = '\0';
                    Strcat(apebuf, "\"");
                    add_autopickup_exception(apebuf);
                }
                goto ape_again;
            }
        } else { /* list or remove */
            int pick_idx, pick_cnt;
            menu_item *pick_list = (menu_item *) 0;

            tmpwin = create_nhwindow(NHW_MENU);
            start_menu(tmpwin);
            for (pass = AP_LEAVE; pass <= AP_GRAB; ++pass) {
                if (numapes[pass] == 0)
                    continue;
                ape = iflags.autopickup_exceptions[pass];
                any = zeroany;
                add_menu(tmpwin, NO_GLYPH, &any, 0, 0, iflags.menu_headings,
                         (pass == 0) ? "Never pickup" : "Always pickup",
                         MENU_UNSELECTED);
                for (i = 0; i < numapes[pass] && ape; i++) {
                    any.a_void = (opt_idx == 1) ? 0 : ape;
                    Sprintf(apebuf, "\"%s\"", ape->pattern);
                    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, apebuf,
                             MENU_UNSELECTED);
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
    } else if (!strcmp("symset", optname)
               || !strcmp("roguesymset", optname)) {
        menu_item *symset_pick = (menu_item *) 0;
        boolean primaryflag = (*optname == 's'),
                rogueflag = (*optname == 'r'),
                ready_to_switch = FALSE,
                nothing_to_do = FALSE;
        char *symset_name, fmtstr[20];
        struct symsetentry *sl;
        int res, which_set, setcount = 0, chosen = -2;

        if (rogueflag)
            which_set = ROGUESET;
        else
            which_set = PRIMARY;

        /* clear symset[].name as a flag to read_sym_file() to build list */
        symset_name = symset[which_set].name;
        symset[which_set].name = (char *) 0;
        symset_list = (struct symsetentry *) 0;

        res = read_sym_file(which_set);
        if (res && symset_list) {
            char symsetchoice[BUFSZ];
            int let = 'a', biggest = 0, thissize = 0;

            sl = symset_list;
            while (sl) {
                /* check restrictions */
                if ((!rogueflag && sl->rogue)
                    || (!primaryflag && sl->primary)) {
                    sl = sl->next;
                    continue;
                }
                setcount++;
                /* find biggest name */
                if (sl->name)
                    thissize = strlen(sl->name);
                if (thissize > biggest)
                    biggest = thissize;
                sl = sl->next;
            }
            if (!setcount) {
                pline("There are no appropriate %ssymbol sets available.",
                      (rogueflag) ? "rogue level "
                                  : (primaryflag) ? "primary " : "");
                return TRUE;
            }

            Sprintf(fmtstr, "%%-%ds %%s", biggest + 5);
            tmpwin = create_nhwindow(NHW_MENU);
            start_menu(tmpwin);
            any = zeroany;
            any.a_int = 1;
            add_menu(tmpwin, NO_GLYPH, &any, let++, 0, ATR_NONE,
                     "Default Symbols", MENU_UNSELECTED);

            sl = symset_list;
            while (sl) {
                /* check restrictions */
                if ((!rogueflag && sl->rogue)
                    || (!primaryflag && sl->primary)) {
                    sl = sl->next;
                    continue;
                }
                if (sl->name) {
                    any.a_int = sl->idx + 2;
                    Sprintf(symsetchoice, fmtstr, sl->name,
                            sl->desc ? sl->desc : "");
                    add_menu(tmpwin, NO_GLYPH, &any, let, 0, ATR_NONE,
                             symsetchoice, MENU_UNSELECTED);
                    if (let == 'z')
                        let = 'A';
                    else
                        let++;
                }
                sl = sl->next;
            }
            end_menu(tmpwin, "Select symbol set:");
            if (select_menu(tmpwin, PICK_ONE, &symset_pick) > 0) {
                chosen = symset_pick->item.a_int - 2;
                free((genericptr_t) symset_pick);
            }
            destroy_nhwindow(tmpwin);

            if (chosen > -1) {
                /* chose an actual symset name from file */
                sl = symset_list;
                while (sl) {
                    if (sl->idx == chosen) {
                        if (symset_name) {
                            free((genericptr_t) symset_name);
                            symset_name = (char *) 0;
                        }
                        /* free the now stale attributes */
                        clear_symsetentry(which_set, TRUE);

                        /* transfer only the name of the symbol set */
                        symset[which_set].name = dupstr(sl->name);
                        ready_to_switch = TRUE;
                        break;
                    }
                    sl = sl->next;
                }
            } else if (chosen == -1) {
                /* explicit selection of defaults */
                /* free the now stale symset attributes */
                if (symset_name) {
                    free((genericptr_t) symset_name);
                    symset_name = (char *) 0;
                }
                clear_symsetentry(which_set, TRUE);
            } else
                nothing_to_do = TRUE;
        } else if (!res) {
            /* The symbols file could not be accessed */
            pline("Unable to access \"%s\" file.", SYMBOLS);
            return TRUE;
        } else if (!symset_list) {
            /* The symbols file was empty */
            pline("There were no symbol sets found in \"%s\".", SYMBOLS);
            return TRUE;
        }

        /* clean up */
        while (symset_list) {
            sl = symset_list;
            if (sl->name)
                free((genericptr_t) sl->name);
            sl->name = (char *) 0;

            if (sl->desc)
                free((genericptr_t) sl->desc);
            sl->desc = (char *) 0;

            symset_list = sl->next;
            free((genericptr_t) sl);
        }

        if (nothing_to_do)
            return TRUE;

        if (!symset[which_set].name && symset_name)
            symset[which_set].name = symset_name; /* not dupstr() here */

        /* Set default symbols and clear the handling value */
        if (rogueflag)
            init_r_symbols();
        else
            init_l_symbols();

        if (symset[which_set].name) {
            if (read_sym_file(which_set)) {
                ready_to_switch = TRUE;
            } else {
                clear_symsetentry(which_set, TRUE);
                return TRUE;
            }
        }

        if (ready_to_switch)
            switch_symbols(TRUE);

        if (Is_rogue_level(&u.uz)) {
            if (rogueflag)
                assign_graphics(ROGUESET);
        } else if (!rogueflag)
            assign_graphics(PRIMARY);
        need_redraw = TRUE;
        return TRUE;

    } else {
        /* didn't match any of the special options */
        return FALSE;
    }
    return TRUE;
}

#define rolestring(val, array, field) \
    ((val >= 0) ? array[val].field : (val == ROLE_RANDOM) ? randomrole : none)

/* This is ugly. We have all the option names in the compopt[] array,
   but we need to look at each option individually to get the value. */
STATIC_OVL const char *
get_compopt_value(optname, buf)
const char *optname;
char *buf;
{
    char ocl[MAXOCLASSES + 1];
    static const char none[] = "(none)", randomrole[] = "random",
                      to_be_done[] = "(to be done)", defopt[] = "default",
                      defbrief[] = "def";
    int i;

    buf[0] = '\0';
    if (!strcmp(optname, "align_message"))
        Sprintf(buf, "%s",
                iflags.wc_align_message == ALIGN_TOP
                    ? "top"
                    : iflags.wc_align_message == ALIGN_LEFT
                          ? "left"
                          : iflags.wc_align_message == ALIGN_BOTTOM
                                ? "bottom"
                                : iflags.wc_align_message == ALIGN_RIGHT
                                      ? "right"
                                      : defopt);
    else if (!strcmp(optname, "align_status"))
        Sprintf(buf, "%s",
                iflags.wc_align_status == ALIGN_TOP
                    ? "top"
                    : iflags.wc_align_status == ALIGN_LEFT
                          ? "left"
                          : iflags.wc_align_status == ALIGN_BOTTOM
                                ? "bottom"
                                : iflags.wc_align_status == ALIGN_RIGHT
                                      ? "right"
                                      : defopt);
    else if (!strcmp(optname, "align"))
        Sprintf(buf, "%s", rolestring(flags.initalign, aligns, adj));
#ifdef WIN32
    else if (!strcmp(optname, "altkeyhandler"))
        Sprintf(buf, "%s",
                iflags.altkeyhandler[0] ? iflags.altkeyhandler : "default");
#endif
#ifdef BACKWARD_COMPAT
    else if (!strcmp(optname, "boulder"))
        Sprintf(buf, "%c",
                iflags.bouldersym
                    ? iflags.bouldersym
                    : showsyms[(int) objects[BOULDER].oc_class + SYM_OFF_O]);
#endif
    else if (!strcmp(optname, "catname"))
        Sprintf(buf, "%s", catname[0] ? catname : none);
    else if (!strcmp(optname, "disclose"))
        for (i = 0; i < NUM_DISCLOSURE_OPTIONS; i++) {
            if (i)
                (void) strkitten(buf, ' ');
            (void) strkitten(buf, flags.end_disclose[i]);
            (void) strkitten(buf, disclosure_options[i]);
        }
    else if (!strcmp(optname, "dogname"))
        Sprintf(buf, "%s", dogname[0] ? dogname : none);
    else if (!strcmp(optname, "dungeon"))
        Sprintf(buf, "%s", to_be_done);
    else if (!strcmp(optname, "effects"))
        Sprintf(buf, "%s", to_be_done);
    else if (!strcmp(optname, "font_map"))
        Sprintf(buf, "%s", iflags.wc_font_map ? iflags.wc_font_map : defopt);
    else if (!strcmp(optname, "font_message"))
        Sprintf(buf, "%s",
                iflags.wc_font_message ? iflags.wc_font_message : defopt);
    else if (!strcmp(optname, "font_status"))
        Sprintf(buf, "%s",
                iflags.wc_font_status ? iflags.wc_font_status : defopt);
    else if (!strcmp(optname, "font_menu"))
        Sprintf(buf, "%s",
                iflags.wc_font_menu ? iflags.wc_font_menu : defopt);
    else if (!strcmp(optname, "font_text"))
        Sprintf(buf, "%s",
                iflags.wc_font_text ? iflags.wc_font_text : defopt);
    else if (!strcmp(optname, "font_size_map")) {
        if (iflags.wc_fontsiz_map)
            Sprintf(buf, "%d", iflags.wc_fontsiz_map);
        else
            Strcpy(buf, defopt);
    } else if (!strcmp(optname, "font_size_message")) {
        if (iflags.wc_fontsiz_message)
            Sprintf(buf, "%d", iflags.wc_fontsiz_message);
        else
            Strcpy(buf, defopt);
    } else if (!strcmp(optname, "font_size_status")) {
        if (iflags.wc_fontsiz_status)
            Sprintf(buf, "%d", iflags.wc_fontsiz_status);
        else
            Strcpy(buf, defopt);
    } else if (!strcmp(optname, "font_size_menu")) {
        if (iflags.wc_fontsiz_menu)
            Sprintf(buf, "%d", iflags.wc_fontsiz_menu);
        else
            Strcpy(buf, defopt);
    } else if (!strcmp(optname, "font_size_text")) {
        if (iflags.wc_fontsiz_text)
            Sprintf(buf, "%d", iflags.wc_fontsiz_text);
        else
            Strcpy(buf, defopt);
    } else if (!strcmp(optname, "fruit"))
        Sprintf(buf, "%s", pl_fruit);
    else if (!strcmp(optname, "gender"))
        Sprintf(buf, "%s", rolestring(flags.initgend, genders, adj));
    else if (!strcmp(optname, "horsename"))
        Sprintf(buf, "%s", horsename[0] ? horsename : none);
    else if (!strcmp(optname, "map_mode"))
        Sprintf(buf, "%s",
                iflags.wc_map_mode == MAP_MODE_TILES
                  ? "tiles"
                  : iflags.wc_map_mode == MAP_MODE_ASCII4x6
                     ? "ascii4x6"
                     : iflags.wc_map_mode == MAP_MODE_ASCII6x8
                        ? "ascii6x8"
                        : iflags.wc_map_mode == MAP_MODE_ASCII8x8
                           ? "ascii8x8"
                           : iflags.wc_map_mode == MAP_MODE_ASCII16x8
                              ? "ascii16x8"
                              : iflags.wc_map_mode == MAP_MODE_ASCII7x12
                                 ? "ascii7x12"
                                 : iflags.wc_map_mode == MAP_MODE_ASCII8x12
                                    ? "ascii8x12"
                                    : iflags.wc_map_mode
                                      == MAP_MODE_ASCII16x12
                                       ? "ascii16x12"
                                       : iflags.wc_map_mode
                                         == MAP_MODE_ASCII12x16
                                          ? "ascii12x16"
                                          : iflags.wc_map_mode
                                            == MAP_MODE_ASCII10x18
                                             ? "ascii10x18"
                                             : iflags.wc_map_mode
                                               == MAP_MODE_ASCII_FIT_TO_SCREEN
                                                ? "fit_to_screen"
                                                : defopt);
    else if (!strcmp(optname, "menustyle"))
        Sprintf(buf, "%s", menutype[(int) flags.menu_style]);
    else if (!strcmp(optname, "menu_deselect_all"))
        Sprintf(buf, "%s", to_be_done);
    else if (!strcmp(optname, "menu_deselect_page"))
        Sprintf(buf, "%s", to_be_done);
    else if (!strcmp(optname, "menu_first_page"))
        Sprintf(buf, "%s", to_be_done);
    else if (!strcmp(optname, "menu_invert_all"))
        Sprintf(buf, "%s", to_be_done);
    else if (!strcmp(optname, "menu_headings"))
        Sprintf(buf, "%s", attr2attrname(iflags.menu_headings));
    else if (!strcmp(optname, "menu_invert_page"))
        Sprintf(buf, "%s", to_be_done);
    else if (!strcmp(optname, "menu_last_page"))
        Sprintf(buf, "%s", to_be_done);
    else if (!strcmp(optname, "menu_next_page"))
        Sprintf(buf, "%s", to_be_done);
    else if (!strcmp(optname, "menu_previous_page"))
        Sprintf(buf, "%s", to_be_done);
    else if (!strcmp(optname, "menu_search"))
        Sprintf(buf, "%s", to_be_done);
    else if (!strcmp(optname, "menu_select_all"))
        Sprintf(buf, "%s", to_be_done);
    else if (!strcmp(optname, "menu_select_page"))
        Sprintf(buf, "%s", to_be_done);
    else if (!strcmp(optname, "monsters")) {
        Sprintf(buf, "%s", to_be_done);
    } else if (!strcmp(optname, "msghistory")) {
        Sprintf(buf, "%u", iflags.msg_history);
#ifdef TTY_GRAPHICS
    } else if (!strcmp(optname, "msg_window")) {
        Sprintf(buf, "%s", (iflags.prevmsg_window == 's')
                               ? "single"
                               : (iflags.prevmsg_window == 'c')
                                     ? "combination"
                                     : (iflags.prevmsg_window == 'f')
                                           ? "full"
                                           : "reversed");
#endif
    } else if (!strcmp(optname, "name")) {
        Sprintf(buf, "%s", plname);
    } else if (!strcmp(optname, "number_pad")) {
        static const char *numpadmodes[] = {
            "0=off", "1=on", "2=on, MSDOS compatible",
            "3=on, phone-style layout",
            "4=on, phone layout, MSDOS compatible",
            "-1=off, y & z swapped", /*[5]*/
        };
        int indx = Cmd.num_pad
                       ? (Cmd.phone_layout ? (Cmd.pcHack_compat ? 4 : 3)
                                           : (Cmd.pcHack_compat ? 2 : 1))
                       : Cmd.swap_yz ? 5 : 0;

        Strcpy(buf, numpadmodes[indx]);
    } else if (!strcmp(optname, "objects")) {
        Sprintf(buf, "%s", to_be_done);
    } else if (!strcmp(optname, "packorder")) {
        oc_to_str(flags.inv_order, ocl);
        Sprintf(buf, "%s", ocl);
#ifdef CHANGE_COLOR
    } else if (!strcmp(optname, "palette")) {
        Sprintf(buf, "%s", get_color_string());
#endif
    } else if (!strcmp(optname, "paranoid_confirmation")) {
        char tmpbuf[QBUFSZ];

        tmpbuf[0] = '\0';
        if (ParanoidConfirm)
            Strcat(tmpbuf, " Confirm");
        if (ParanoidQuit)
            Strcat(tmpbuf, " quit");
        if (ParanoidDie)
            Strcat(tmpbuf, " die");
        if (ParanoidBones)
            Strcat(tmpbuf, " bones");
        if (ParanoidHit)
            Strcat(tmpbuf, " attack");
        if (ParanoidPray)
            Strcat(tmpbuf, " pray");
        if (ParanoidRemove)
            Strcat(tmpbuf, " Remove");
        Strcpy(buf, tmpbuf[0] ? &tmpbuf[1] : "none");
    } else if (!strcmp(optname, "pettype")) {
        Sprintf(buf, "%s", (preferred_pet == 'c') ? "cat"
                           : (preferred_pet == 'd') ? "dog"
                             : (preferred_pet == 'h') ? "horse"
                               : (preferred_pet == 'n') ? "none"
                                 : "random");
    } else if (!strcmp(optname, "pickup_burden")) {
        Sprintf(buf, "%s", burdentype[flags.pickup_burden]);
    } else if (!strcmp(optname, "pickup_types")) {
        oc_to_str(flags.pickup_types, ocl);
        Sprintf(buf, "%s", ocl[0] ? ocl : "all");
    } else if (!strcmp(optname, "pile_limit")) {
        Sprintf(buf, "%d", flags.pile_limit);
    } else if (!strcmp(optname, "playmode")) {
        Strcpy(buf, wizard ? "debug" : discover ? "explore" : "normal");
    } else if (!strcmp(optname, "race")) {
        Sprintf(buf, "%s", rolestring(flags.initrace, races, noun));
    } else if (!strcmp(optname, "roguesymset")) {
        Sprintf(buf, "%s",
                symset[ROGUESET].name ? symset[ROGUESET].name : "default");
        if (currentgraphics == ROGUESET && symset[ROGUESET].name)
            Strcat(buf, ", active");
    } else if (!strcmp(optname, "role")) {
        Sprintf(buf, "%s", rolestring(flags.initrole, roles, name.m));
    } else if (!strcmp(optname, "runmode")) {
        Sprintf(buf, "%s", runmodes[flags.runmode]);
    } else if (!strcmp(optname, "scores")) {
        Sprintf(buf, "%d top/%d around%s", flags.end_top, flags.end_around,
                flags.end_own ? "/own" : "");
    } else if (!strcmp(optname, "scroll_amount")) {
        if (iflags.wc_scroll_amount)
            Sprintf(buf, "%d", iflags.wc_scroll_amount);
        else
            Strcpy(buf, defopt);
    } else if (!strcmp(optname, "scroll_margin")) {
        if (iflags.wc_scroll_margin)
            Sprintf(buf, "%d", iflags.wc_scroll_margin);
        else
            Strcpy(buf, defopt);
    } else if (!strcmp(optname, "sortloot")) {
        for (i = 0; i < SIZE(sortltype); i++)
            if (flags.sortloot == sortltype[i][0]) {
                Strcpy(buf, sortltype[i]);
                break;
            }
    } else if (!strcmp(optname, "player_selection")) {
        Sprintf(buf, "%s", iflags.wc_player_selection ? "prompts" : "dialog");
#ifdef MSDOS
    } else if (!strcmp(optname, "soundcard")) {
        Sprintf(buf, "%s", to_be_done);
#endif
    } else if (!strcmp(optname, "suppress_alert")) {
        if (flags.suppress_alert == 0L)
            Strcpy(buf, none);
        else
            Sprintf(buf, "%lu.%lu.%lu", FEATURE_NOTICE_VER_MAJ,
                    FEATURE_NOTICE_VER_MIN, FEATURE_NOTICE_VER_PATCH);
    } else if (!strcmp(optname, "symset")) {
        Sprintf(buf, "%s",
                symset[PRIMARY].name ? symset[PRIMARY].name : "default");
        if (currentgraphics == PRIMARY && symset[PRIMARY].name)
            Strcat(buf, ", active");
    } else if (!strcmp(optname, "tile_file")) {
        Sprintf(buf, "%s",
                iflags.wc_tile_file ? iflags.wc_tile_file : defopt);
    } else if (!strcmp(optname, "tile_height")) {
        if (iflags.wc_tile_height)
            Sprintf(buf, "%d", iflags.wc_tile_height);
        else
            Strcpy(buf, defopt);
    } else if (!strcmp(optname, "tile_width")) {
        if (iflags.wc_tile_width)
            Sprintf(buf, "%d", iflags.wc_tile_width);
        else
            Strcpy(buf, defopt);
    } else if (!strcmp(optname, "traps")) {
        Sprintf(buf, "%s", to_be_done);
    } else if (!strcmp(optname, "vary_msgcount")) {
        if (iflags.wc_vary_msgcount)
            Sprintf(buf, "%d", iflags.wc_vary_msgcount);
        else
            Strcpy(buf, defopt);
#ifdef MSDOS
    } else if (!strcmp(optname, "video")) {
        Sprintf(buf, "%s", to_be_done);
#endif
#ifdef VIDEOSHADES
    } else if (!strcmp(optname, "videoshades")) {
        Sprintf(buf, "%s-%s-%s", shade[0], shade[1], shade[2]);
    } else if (!strcmp(optname, "videocolors")) {
        Sprintf(buf, "%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d",
                ttycolors[CLR_RED], ttycolors[CLR_GREEN],
                ttycolors[CLR_BROWN], ttycolors[CLR_BLUE],
                ttycolors[CLR_MAGENTA], ttycolors[CLR_CYAN],
                ttycolors[CLR_ORANGE], ttycolors[CLR_BRIGHT_GREEN],
                ttycolors[CLR_YELLOW], ttycolors[CLR_BRIGHT_BLUE],
                ttycolors[CLR_BRIGHT_MAGENTA], ttycolors[CLR_BRIGHT_CYAN]);
#endif /* VIDEOSHADES */
    } else if (!strcmp(optname, "windowtype")) {
        Sprintf(buf, "%s", windowprocs.name);
    } else if (!strcmp(optname, "windowcolors")) {
        Sprintf(
            buf, "%s/%s %s/%s %s/%s %s/%s",
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
#ifdef PREFIXES_IN_USE
    } else {
        for (i = 0; i < PREFIX_COUNT; ++i)
            if (!strcmp(optname, fqn_prefix_names[i]) && fqn_prefix[i])
                Sprintf(buf, "%s", fqn_prefix[i]);
#endif
    }

    if (buf[0])
        return buf;
    else
        return "unknown";
}

int
dotogglepickup()
{
    char buf[BUFSZ], ocl[MAXOCLASSES + 1];

    flags.pickup = !flags.pickup;
    if (flags.pickup) {
        oc_to_str(flags.pickup_types, ocl);
        Sprintf(buf, "ON, for %s objects%s", ocl[0] ? ocl : "all",
                (iflags.autopickup_exceptions[AP_LEAVE]
                 || iflags.autopickup_exceptions[AP_GRAB])
                    ? ((count_ape_maps((int *) 0, (int *) 0) == 1)
                           ? ", with one exception"
                           : ", with some exceptions")
                    : "");
    } else {
        Strcpy(buf, "OFF");
    }
    pline("Autopickup: %s.", buf);
    return 0;
}

int
add_autopickup_exception(mapping)
const char *mapping;
{
    struct autopickup_exception *ape, **apehead;
    char text[256], *text2;
    boolean grab = FALSE;

    if (sscanf(mapping, "\"%255[^\"]\"", text) == 1) {
        text2 = &text[0];
        if (*text2 == '<') { /* force autopickup */
            grab = TRUE;
            ++text2;
        } else if (*text2 == '>') { /* default - Do not pickup */
            grab = FALSE;
            ++text2;
        }
        apehead = (grab) ? &iflags.autopickup_exceptions[AP_GRAB]
                         : &iflags.autopickup_exceptions[AP_LEAVE];
        ape = (struct autopickup_exception *) alloc(
                                        sizeof (struct autopickup_exception));
        ape->regex = regex_init();
        if (!regex_compile(text2, ape->regex)) {
            raw_print("regex error in AUTOPICKUP_EXCEPTION");
            regex_free(ape->regex);
            free((genericptr_t) ape);
            return 0;
        }
        ape->pattern = (char *) alloc(strlen(text2) + 1);
        strcpy(ape->pattern, text2);
        ape->grab = grab;
        ape->next = *apehead;
        *apehead = ape;
    } else {
        raw_print("syntax error in AUTOPICKUP_EXCEPTION");
        return 0;
    }
    return 1;
}

STATIC_OVL void
remove_autopickup_exception(whichape)
struct autopickup_exception *whichape;
{
    struct autopickup_exception *ape, *prev = 0;
    int chain = whichape->grab ? AP_GRAB : AP_LEAVE;

    for (ape = iflags.autopickup_exceptions[chain]; ape;) {
        if (ape == whichape) {
            struct autopickup_exception *freeape = ape;

            ape = ape->next;
            if (prev)
                prev->next = ape;
            else
                iflags.autopickup_exceptions[chain] = ape;
            regex_free(freeape->regex);
            free((genericptr_t) freeape->pattern);
            free((genericptr_t) freeape);
        } else {
            prev = ape;
            ape = ape->next;
        }
    }
}

STATIC_OVL int
count_ape_maps(leave, grab)
int *leave, *grab;
{
    struct autopickup_exception *ape;
    int pass, totalapes, numapes[2] = { 0, 0 };

    for (pass = AP_LEAVE; pass <= AP_GRAB; ++pass) {
        ape = iflags.autopickup_exceptions[pass];
        while (ape) {
            ape = ape->next;
            numapes[pass]++;
        }
    }
    totalapes = numapes[AP_LEAVE] + numapes[AP_GRAB];
    if (leave)
        *leave = numapes[AP_LEAVE];
    if (grab)
        *grab = numapes[AP_GRAB];
    return totalapes;
}

void
free_autopickup_exceptions()
{
    struct autopickup_exception *ape;
    int pass;

    for (pass = AP_LEAVE; pass <= AP_GRAB; ++pass) {
        while ((ape = iflags.autopickup_exceptions[pass]) != 0) {
            regex_free(ape->regex);
            free((genericptr_t) ape->pattern);
            iflags.autopickup_exceptions[pass] = ape->next;
            free((genericptr_t) ape);
        }
    }
}

/* bundle some common usage into one easy-to-use routine */
int
load_symset(s, which_set)
const char *s;
int which_set;
{
    clear_symsetentry(which_set, TRUE);

    if (symset[which_set].name)
        free((genericptr_t) symset[which_set].name);
    symset[which_set].name = dupstr(s);

    if (read_sym_file(which_set)) {
        switch_symbols(TRUE);
    } else {
        clear_symsetentry(which_set, TRUE);
        return 0;
    }
    return 1;
}

void
free_symsets()
{
    clear_symsetentry(PRIMARY, TRUE);
    clear_symsetentry(ROGUESET, TRUE);

    /* symset_list is cleaned up as soon as it's used, so we shouldn't
       have to anything about it here */
    /* assert( symset_list == NULL ); */
}

/* Parse the value of a SYMBOLS line from a config file */
void
parsesymbols(opts)
register char *opts;
{
    int val;
    char *op, *symname, *strval;
    struct symparse *symp;

    if ((op = index(opts, ',')) != 0) {
        *op++ = 0;
        parsesymbols(op);
    }

    /* S_sample:string */
    symname = opts;
    strval = index(opts, ':');
    if (!strval)
        strval = index(opts, '=');
    if (!strval)
        return;
    *strval++ = '\0';

    /* strip leading and trailing white space from symname and strval */
    mungspaces(symname);
    mungspaces(strval);

    symp = match_sym(symname);
    if (!symp)
        return;

    if (symp->range && symp->range != SYM_CONTROL) {
        val = sym_val(strval);
        update_l_symset(symp, val);
    }
}

struct symparse *
match_sym(buf)
char *buf;
{
    size_t len = strlen(buf);
    const char *p = index(buf, ':'), *q = index(buf, '=');
    struct symparse *sp = loadsyms;

    if (!p || (q && q < p))
        p = q;
    if (p) {
        /* note: there will be at most one space before the '='
           because caller has condensed buf[] with mungspaces() */
        if (p > buf && p[-1] == ' ')
            p--;
        len = (int) (p - buf);
    }
    while (sp->range) {
        if ((len >= strlen(sp->name)) && !strncmpi(buf, sp->name, len))
            return sp;
        sp++;
    }
    return (struct symparse *) 0;
}

int
sym_val(strval)
char *strval;
{
    char buf[QBUFSZ];
    buf[0] = '\0';
    escapes(strval, buf);
    return (int) *buf;
}

/* data for option_help() */
static const char *opt_intro[] = {
    "", "                 NetHack Options Help:", "",
#define CONFIG_SLOT 3 /* fill in next value at run-time */
    (char *) 0,
#if !defined(MICRO) && !defined(MAC)
    "or use `NETHACKOPTIONS=\"<options>\"' in your environment",
#endif
    "(<options> is a list of options separated by commas)",
#ifdef VMS
    "-- for example, $ DEFINE NETHACKOPTIONS \"noautopickup,fruit:kumquat\"",
#endif
    "or press \"O\" while playing and use the menu.", "",
 "Boolean options (which can be negated by prefixing them with '!' or \"no\"):",
    (char *) 0
};

static const char *opt_epilog[] = {
    "",
    "Some of the options can be set only before the game is started; those",
    "items will not be selectable in the 'O' command's menu.", (char *) 0
};

void
option_help()
{
    char buf[BUFSZ], buf2[BUFSZ];
    register int i;
    winid datawin;

    datawin = create_nhwindow(NHW_TEXT);
    Sprintf(buf, "Set options as OPTIONS=<options> in %s", lastconfigfile);
    opt_intro[CONFIG_SLOT] = (const char *) buf;
    for (i = 0; opt_intro[i]; i++)
        putstr(datawin, 0, opt_intro[i]);

    /* Boolean options */
    for (i = 0; boolopt[i].name; i++) {
        if (boolopt[i].addr) {
            if (boolopt[i].addr == &iflags.sanity_check && !wizard)
                continue;
            if (boolopt[i].addr == &iflags.menu_tab_sep && !wizard)
                continue;
            next_opt(datawin, boolopt[i].name);
        }
    }
    next_opt(datawin, "");

    /* Compound options */
    putstr(datawin, 0, "Compound options:");
    for (i = 0; compopt[i].name; i++) {
        Sprintf(buf2, "`%s'", compopt[i].name);
        Sprintf(buf, "%-20s - %s%c", buf2, compopt[i].descr,
                compopt[i + 1].name ? ',' : '.');
        putstr(datawin, 0, buf);
    }

    for (i = 0; opt_epilog[i]; i++)
        putstr(datawin, 0, opt_epilog[i]);

    display_nhwindow(datawin, FALSE);
    destroy_nhwindow(datawin);
    return;
}

/*
 * prints the next boolean option, on the same line if possible, on a new
 * line if not. End with next_opt("").
 */
void
next_opt(datawin, str)
winid datawin;
const char *str;
{
    static char *buf = 0;
    int i;
    char *s;

    if (!buf)
        *(buf = (char *) alloc(BUFSZ)) = '\0';

    if (!*str) {
        s = eos(buf);
        if (s > &buf[1] && s[-2] == ',')
            Strcpy(s - 2, "."); /* replace last ", " */
        i = COLNO;              /* (greater than COLNO - 2) */
    } else {
        i = strlen(buf) + strlen(str) + 2;
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

/* Returns the fid of the fruit type; if that type already exists, it
 * returns the fid of that one; if it does not exist, it adds a new fruit
 * type to the chain and returns the new one.
 * If replace_fruit is sent in, replace the fruit in the chain rather than
 * adding a new entry--for user specified fruits only.
 */
int
fruitadd(str, replace_fruit)
char *str;
struct fruit *replace_fruit;
{
    register int i;
    register struct fruit *f;
    int highest_fruit_id = 0;
    char buf[PL_FSIZ], altname[PL_FSIZ];
    boolean user_specified = (str == pl_fruit);
    /* if not user-specified, then it's a fruit name for a fruit on
     * a bones level...
     */

    /* Note: every fruit has an id (kept in obj->spe) of at least 1;
     * 0 is an error.
     */
    if (user_specified) {
        boolean found = FALSE, numeric = FALSE;

        /* force fruit to be singular; this handling is not
           needed--or wanted--for fruits from bones because
           they already received it in their original game */
        nmcpy(pl_fruit, makesingular(str), PL_FSIZ);
        /* assert( str == pl_fruit ); */

        /* disallow naming after other foods (since it'd be impossible
         * to tell the difference)
         */

        for (i = bases[FOOD_CLASS]; objects[i].oc_class == FOOD_CLASS; i++) {
            if (!strcmp(OBJ_NAME(objects[i]), pl_fruit)) {
                found = TRUE;
                break;
            }
        }
        {
            char *c;

            c = pl_fruit;

            for (c = pl_fruit; *c >= '0' && *c <= '9'; c++)
                ;
            if (isspace((uchar) *c) || *c == 0)
                numeric = TRUE;
        }
        if (found || numeric || !strncmp(str, "cursed ", 7)
            || !strncmp(str, "uncursed ", 9) || !strncmp(str, "blessed ", 8)
            || !strncmp(str, "partly eaten ", 13)
            || (!strncmp(str, "tin of ", 7)
                && (!strcmp(str + 7, "spinach")
                    || name_to_mon(str + 7) >= LOW_PM))
            || !strcmp(str, "empty tin")
            || ((str_end_is(str, " corpse")
                 || str_end_is(str, " egg"))
                && name_to_mon(str) >= LOW_PM)) {
            Strcpy(buf, pl_fruit);
            Strcpy(pl_fruit, "candied ");
            nmcpy(pl_fruit + 8, buf, PL_FSIZ - 8);
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
            for (f = ffruit; f; f = f->nextf) {
                if (f == replace_fruit) {
                    copynchars(f->fname, str, PL_FSIZ - 1);
                    goto nonew;
                }
            }
        }
    } else {
        /* not user_supplied, so assumed to be from bones */
        copynchars(altname, str, PL_FSIZ - 1);
        sanitize_name(altname);
        flags.made_fruit = TRUE; /* for safety.  Any fruit name added from a
                                    bones level should exist anyway. */
    }
    for (f = ffruit; f; f = f->nextf) {
        if (f->fid > highest_fruit_id)
            highest_fruit_id = f->fid;
        if (!strncmp(str, f->fname, PL_FSIZ - 1)
            || (*altname && !strcmp(altname, f->fname)))
            goto nonew;
    }
    /* if adding another fruit would overflow spe, use a random
       fruit instead... we've got a lot to choose from.
       current_fruit remains as is. */
    if (highest_fruit_id >= 127)
        return rnd(127);

    f = newfruit();
    copynchars(f->fname, *altname ? altname : str, PL_FSIZ - 1);
    f->fid = ++highest_fruit_id;
    /* we used to go out of our way to add it at the end of the list,
       but the order is arbitrary so use simpler insertion at start */
    f->nextf = ffruit;
    ffruit = f;
nonew:
    if (user_specified)
        context.current_fruit = f->fid;
    return f->fid;
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
choose_classes_menu(prompt, category, way, class_list, class_select)
const char *prompt;
int category;
boolean way;
char *class_list;
char *class_select;
{
    menu_item *pick_list = (menu_item *) 0;
    winid win;
    anything any;
    char buf[BUFSZ];
    int i, n;
    int ret;
    int next_accelerator, accelerator;

    if (class_list == (char *) 0 || class_select == (char *) 0)
        return 0;
    accelerator = 0;
    next_accelerator = 'a';
    any = zeroany;
    win = create_nhwindow(NHW_MENU);
    start_menu(win);
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
        add_menu(win, NO_GLYPH, &any, accelerator, category ? *class_list : 0,
                 ATR_NONE, buf, selected);
        ++class_list;
        if (category > 0) {
            ++next_accelerator;
            if (next_accelerator == ('z' + 1))
                next_accelerator = 'A';
            if (next_accelerator == ('Z' + 1))
                break;
        }
    }
    end_menu(win, prompt);
    n = select_menu(win, way ? PICK_ANY : PICK_ONE, &pick_list);
    destroy_nhwindow(win);
    if (n > 0) {
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

struct wc_Opt wc_options[] = { { "ascii_map", WC_ASCII_MAP },
                               { "color", WC_COLOR },
                               { "eight_bit_tty", WC_EIGHT_BIT_IN },
                               { "hilite_pet", WC_HILITE_PET },
                               { "popup_dialog", WC_POPUP_DIALOG },
                               { "player_selection", WC_PLAYER_SELECTION },
                               { "preload_tiles", WC_PRELOAD_TILES },
                               { "tiled_map", WC_TILED_MAP },
                               { "tile_file", WC_TILE_FILE },
                               { "tile_width", WC_TILE_WIDTH },
                               { "tile_height", WC_TILE_HEIGHT },
                               { "use_inverse", WC_INVERSE },
                               { "align_message", WC_ALIGN_MESSAGE },
                               { "align_status", WC_ALIGN_STATUS },
                               { "font_map", WC_FONT_MAP },
                               { "font_menu", WC_FONT_MENU },
                               { "font_message", WC_FONT_MESSAGE },
#if 0
                               {"perm_invent", WC_PERM_INVENT},
#endif
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
                               { "vary_msgcount", WC_VARY_MSGCOUNT },
                               { "windowcolors", WC_WINDOWCOLORS },
                               { "mouse_support", WC_MOUSE_SUPPORT },
                               { (char *) 0, 0L } };

struct wc_Opt wc2_options[] = { { "fullscreen", WC2_FULLSCREEN },
                                { "softkeyboard", WC2_SOFTKEYBOARD },
                                { "wraptext", WC2_WRAPTEXT },
                                { "use_darkgray", WC2_DARKGRAY },
#ifdef STATUS_VIA_WINDOWPORT
                                { "hilite_status", WC2_HILITE_STATUS },
#endif
                                { (char *) 0, 0L } };

/*
 * If a port wants to change or ensure that the SET_IN_SYS,
 * SET_IN_FILE, DISP_IN_GAME, or SET_IN_GAME status of an option is
 * correct (for controlling its display in the option menu) call
 * set_option_mod_status()
 * with the appropriate second argument.
 */
void
set_option_mod_status(optnam, status)
const char *optnam;
int status;
{
    int k;

    if (SET__IS_VALUE_VALID(status)) {
        impossible("set_option_mod_status: status out of range %d.", status);
        return;
    }
    for (k = 0; boolopt[k].name; k++) {
        if (!strncmpi(boolopt[k].name, optnam, strlen(optnam))) {
            boolopt[k].optflags = status;
            return;
        }
    }
    for (k = 0; compopt[k].name; k++) {
        if (!strncmpi(compopt[k].name, optnam, strlen(optnam))) {
            compopt[k].optflags = status;
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
 * SET_IN_GAME);
 */
void
set_wc_option_mod_status(optmask, status)
unsigned long optmask;
int status;
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

STATIC_OVL boolean
is_wc_option(optnam)
const char *optnam;
{
    int k = 0;

    while (wc_options[k].wc_name) {
        if (strcmp(wc_options[k].wc_name, optnam) == 0)
            return TRUE;
        k++;
    }
    return FALSE;
}

STATIC_OVL boolean
wc_supported(optnam)
const char *optnam;
{
    int k = 0;

    while (wc_options[k].wc_name) {
        if (!strcmp(wc_options[k].wc_name, optnam)
            && (windowprocs.wincap & wc_options[k].wc_bit))
            return TRUE;
        k++;
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
 * SET_IN_FILE);
 */

void
set_wc2_option_mod_status(optmask, status)
unsigned long optmask;
int status;
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

STATIC_OVL boolean
is_wc2_option(optnam)
const char *optnam;
{
    int k = 0;

    while (wc2_options[k].wc_name) {
        if (strcmp(wc2_options[k].wc_name, optnam) == 0)
            return TRUE;
        k++;
    }
    return FALSE;
}

STATIC_OVL boolean
wc2_supported(optnam)
const char *optnam;
{
    int k = 0;

    while (wc2_options[k].wc_name) {
        if (!strcmp(wc2_options[k].wc_name, optnam)
            && (windowprocs.wincap2 & wc2_options[k].wc_bit))
            return TRUE;
        k++;
    }
    return FALSE;
}

STATIC_OVL void
wc_set_font_name(opttype, fontname)
int opttype;
char *fontname;
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

STATIC_OVL int
wc_set_window_colors(op)
char *op;
{
    /* syntax:
     *  menu white/black message green/yellow status white/blue text
     * white/black
     */
    int j;
    char buf[BUFSZ];
    char *wn, *tfg, *tbg, *newop;
    static const char *wnames[] = { "menu", "message", "status", "text" };
    static const char *shortnames[] = { "mnu", "msg", "sts", "txt" };
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
    while (newop && *newop) {
        wn = tfg = tbg = (char *) 0;

        /* until first non-space in case there's leading spaces - before
         * colorname*/
        if (*newop == ' ')
            newop++;
        if (*newop)
            wn = newop;
        else
            return 0;

        /* until first space - colorname*/
        while (*newop && *newop != ' ')
            newop++;
        if (*newop)
            *newop = '\0';
        else
            return 0;
        newop++;

        /* until first non-space - before foreground*/
        if (*newop == ' ')
            newop++;
        if (*newop)
            tfg = newop;
        else
            return 0;

        /* until slash - foreground */
        while (*newop && *newop != '/')
            newop++;
        if (*newop)
            *newop = '\0';
        else
            return 0;
        newop++;

        /* until first non-space (in case there's leading space after slash) -
         * before background */
        if (*newop == ' ')
            newop++;
        if (*newop)
            tbg = newop;
        else
            return 0;

        /* until first space - background */
        while (*newop && *newop != ' ')
            newop++;
        if (*newop)
            *newop++ = '\0';

        for (j = 0; j < 4; ++j) {
            if (!strcmpi(wn, wnames[j]) || !strcmpi(wn, shortnames[j])) {
                if (tfg && !strstri(tfg, " ")) {
                    if (*fgp[j])
                        free((genericptr_t) *fgp[j]);
                    *fgp[j] = dupstr(tfg);
                }
                if (tbg && !strstri(tbg, " ")) {
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
set_playmode()
{
    if (wizard) {
        if (authorize_wizard_mode())
            Strcpy(plname, "wizard");
        else
            wizard = FALSE; /* not allowed or not available */
        /* force explore mode if we didn't make it into wizard mode */
        discover = !wizard;
        iflags.deferred_X = FALSE;
    }
    /* don't need to do anything special for explore mode or normal play */
}

#endif /* OPTION_LISTS_ONLY */

/*options.c*/
