/*	SCCS Id: @(#)options.c	3.4	2003/01/08	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifdef OPTION_LISTS_ONLY	/* (AMIGA) external program for opt lists */
#include "config.h"
#include "objclass.h"
#include "flag.h"
NEARDATA struct flag flags;	/* provide linkage */
NEARDATA struct instance_flags iflags;	/* provide linkage */
#define static
#else
#include "hack.h"
#include "tcap.h"
#include <ctype.h>
#endif

#define WINTYPELEN 16

#ifdef DEFAULT_WC_TILED_MAP
#define PREFER_TILED TRUE
#else
#define PREFER_TILED FALSE
#endif

/*
 *  NOTE:  If you add (or delete) an option, please update the short
 *  options help (option_help()), the long options help (dat/opthelp),
 *  and the current options setting display function (doset()),
 *  and also the Guidebooks.
 *
 *  The order matters.  If an option is a an initial substring of another
 *  option (e.g. time and timed_delay) the shorter one must come first.
 */

static struct Bool_Opt
{
	const char *name;
	boolean	*addr, initvalue;
	int optflags;
} boolopt[] = {
#ifdef AMIGA
	{"altmeta", &flags.altmeta, TRUE, DISP_IN_GAME},
#else
	{"altmeta", (boolean *)0, TRUE, DISP_IN_GAME},
#endif
	{"ascii_map",     &iflags.wc_ascii_map, !PREFER_TILED, SET_IN_GAME},	/*WC*/
#ifdef MFLOPPY
	{"asksavedisk", &flags.asksavedisk, FALSE, SET_IN_GAME},
#else
	{"asksavedisk", (boolean *)0, FALSE, SET_IN_FILE},
#endif
	{"autodig", &flags.autodig, FALSE, SET_IN_GAME},
	{"autopickup", &flags.pickup, TRUE, SET_IN_GAME},
	{"autoquiver", &flags.autoquiver, FALSE, SET_IN_GAME},
#if defined(MICRO) && !defined(AMIGA)
	{"BIOS", &iflags.BIOS, FALSE, SET_IN_FILE},
#else
	{"BIOS", (boolean *)0, FALSE, SET_IN_FILE},
#endif
#ifdef INSURANCE
	{"checkpoint", &flags.ins_chkpt, TRUE, SET_IN_GAME},
#else
	{"checkpoint", (boolean *)0, FALSE, SET_IN_FILE},
#endif
#ifdef MFLOPPY
	{"checkspace", &iflags.checkspace, TRUE, SET_IN_GAME},
#else
	{"checkspace", (boolean *)0, FALSE, SET_IN_FILE},
#endif
	{"cmdassist", &iflags.cmdassist, TRUE, SET_IN_GAME},
# if defined(MICRO) || defined(WIN32)
	{"color",         &iflags.wc_color,TRUE, SET_IN_GAME},		/*WC*/
# else	/* systems that support multiple terminals, many monochrome */
	{"color",         &iflags.wc_color, FALSE, SET_IN_GAME},	/*WC*/
# endif
	{"confirm",&flags.confirm, TRUE, SET_IN_GAME},
#if defined(TERMLIB) && !defined(MAC_GRAPHICS_ENV)
	{"DECgraphics", &iflags.DECgraphics, FALSE, SET_IN_GAME},
#else
	{"DECgraphics", (boolean *)0, FALSE, SET_IN_FILE},
#endif
	{"eight_bit_tty", &iflags.wc_eight_bit_input, FALSE, SET_IN_GAME},	/*WC*/
#ifdef TTY_GRAPHICS
	{"extmenu", &iflags.extmenu, FALSE, SET_IN_GAME},
#else
	{"extmenu", (boolean *)0, FALSE, SET_IN_FILE},
#endif
#ifdef OPT_DISPMAP
	{"fast_map", &flags.fast_map, TRUE, SET_IN_GAME},
#else
	{"fast_map", (boolean *)0, TRUE, SET_IN_FILE},
#endif
	{"female", &flags.female, FALSE, DISP_IN_GAME},
	{"fixinv", &flags.invlet_constant, TRUE, SET_IN_GAME},
#ifdef AMIFLUSH
	{"flush", &flags.amiflush, FALSE, SET_IN_GAME},
#else
	{"flush", (boolean *)0, FALSE, SET_IN_FILE},
#endif
	{"help", &flags.help, TRUE, SET_IN_GAME},
	{"hilite_pet",    &iflags.wc_hilite_pet, FALSE, SET_IN_GAME},	/*WC*/
#ifdef ASCIIGRAPH
	{"IBMgraphics", &iflags.IBMgraphics, FALSE, SET_IN_GAME},
#else
	{"IBMgraphics", (boolean *)0, FALSE, SET_IN_FILE},
#endif
#ifndef MAC
	{"ignintr", &flags.ignintr, FALSE, SET_IN_GAME},
#else
	{"ignintr", (boolean *)0, FALSE, SET_IN_FILE},
#endif
	{"large_font", &iflags.obsolete, FALSE, SET_IN_FILE},	/* OBSOLETE */
	{"legacy", &flags.legacy, TRUE, DISP_IN_GAME},
	{"lit_corridor", &flags.lit_corridor, FALSE, SET_IN_GAME},
	{"lootabc", &iflags.lootabc, FALSE, SET_IN_GAME},
#ifdef MAC_GRAPHICS_ENV
	{"Macgraphics", &iflags.MACgraphics, TRUE, SET_IN_GAME},
#else
	{"Macgraphics", (boolean *)0, FALSE, SET_IN_FILE},
#endif
#ifdef MAIL
	{"mail", &flags.biff, TRUE, SET_IN_GAME},
#else
	{"mail", (boolean *)0, TRUE, SET_IN_FILE},
#endif
#ifdef WIZARD
	/* for menu debugging only*/
	{"menu_tab_sep", &iflags.menu_tab_sep, FALSE, SET_IN_GAME},
#else
	{"menu_tab_sep", (boolean *)0, FALSE, SET_IN_FILE},
#endif
	{"mouse_support", &iflags.wc_mouse_support, TRUE, DISP_IN_GAME},	/*WC*/
#ifdef NEWS
	{"news", &iflags.news, TRUE, DISP_IN_GAME},
#else
	{"news", (boolean *)0, FALSE, SET_IN_FILE},
#endif
	{"null", &flags.null, TRUE, SET_IN_GAME},
	{"number_pad", &iflags.num_pad, FALSE, SET_IN_GAME},
#ifdef MAC
	{"page_wait", &flags.page_wait, TRUE, SET_IN_GAME},
#else
	{"page_wait", (boolean *)0, FALSE, SET_IN_FILE},
#endif
	{"perm_invent", &flags.perm_invent, FALSE, SET_IN_GAME},
	{"popup_dialog",  &iflags.wc_popup_dialog, FALSE, SET_IN_GAME},	/*WC*/
	{"prayconfirm", &flags.prayconfirm, TRUE, SET_IN_GAME},
	{"preload_tiles", &iflags.wc_preload_tiles, TRUE, DISP_IN_GAME},	/*WC*/
	{"pushweapon", &flags.pushweapon, FALSE, SET_IN_GAME},
#if defined(MICRO) && !defined(AMIGA)
	{"rawio", &iflags.rawio, FALSE, DISP_IN_GAME},
#else
	{"rawio", (boolean *)0, FALSE, SET_IN_FILE},
#endif
	{"rest_on_space", &flags.rest_on_space, FALSE, SET_IN_GAME},
	{"safe_pet", &flags.safe_dog, TRUE, SET_IN_GAME},
#ifdef WIZARD
	{"sanity_check", &iflags.sanity_check, FALSE, SET_IN_GAME},
#else
	{"sanity_check", (boolean *)0, FALSE, SET_IN_FILE},
#endif
#ifdef EXP_ON_BOTL
	{"showexp", &flags.showexp, FALSE, SET_IN_GAME},
#else
	{"showexp", (boolean *)0, FALSE, SET_IN_FILE},
#endif
	{"showrace", &iflags.showrace, FALSE, SET_IN_GAME},
#ifdef SCORE_ON_BOTL
	{"showscore", &flags.showscore, FALSE, SET_IN_GAME},
#else
	{"showscore", (boolean *)0, FALSE, SET_IN_FILE},
#endif
	{"silent", &flags.silent, TRUE, SET_IN_GAME},
	{"sortpack", &flags.sortpack, TRUE, SET_IN_GAME},
	{"sound", &flags.soundok, TRUE, SET_IN_GAME},
	{"sparkle", &flags.sparkle, TRUE, SET_IN_GAME},
	{"standout", &flags.standout, FALSE, SET_IN_GAME},
	{"splash_screen",     &iflags.wc_splash_screen, TRUE, DISP_IN_GAME},	/*WC*/
	{"tiled_map",     &iflags.wc_tiled_map, PREFER_TILED, DISP_IN_GAME},	/*WC*/
	{"time", &flags.time, FALSE, SET_IN_GAME},
#ifdef TIMED_DELAY
	{"timed_delay", &flags.nap, TRUE, SET_IN_GAME},
#else
	{"timed_delay", (boolean *)0, FALSE, SET_IN_GAME},
#endif
	{"tombstone",&flags.tombstone, TRUE, SET_IN_GAME},
	{"toptenwin",&flags.toptenwin, FALSE, SET_IN_GAME},
	{"travel", &iflags.travelcmd, TRUE, SET_IN_GAME},
	{"use_inverse",   &iflags.wc_inverse, FALSE, SET_IN_GAME},		/*WC*/
	{"verbose", &flags.verbose, TRUE, SET_IN_GAME},
	{(char *)0, (boolean *)0, FALSE, 0}
};

/* compound options, for option_help() and external programs like Amiga
 * frontend */
static struct Comp_Opt
{
	const char *name, *descr;
	int size;	/* for frontends and such allocating space --
			 * usually allowed size of data in game, but
			 * occasionally maximum reasonable size for
			 * typing when game maintains information in
			 * a different format */
	int optflags;
} compopt[] = {
	{ "align",    "your starting alignment (lawful, neutral, or chaotic)",
						8, DISP_IN_GAME },
	{ "align_message", "message window alignment", 20, DISP_IN_GAME }, 	/*WC*/
	{ "align_status", "status window alignment", 20, DISP_IN_GAME }, 	/*WC*/
	{ "boulder",  "the symbol to use for displaying boulders",
						1, SET_IN_GAME },
	{ "catname",  "the name of your (first) cat (e.g., catname:Tabby)",
						PL_PSIZ, DISP_IN_GAME },
	{ "disclose", "the kinds of information to disclose at end of game",
						sizeof(flags.end_disclose) * 2,
						SET_IN_GAME },
	{ "dogname",  "the name of your (first) dog (e.g., dogname:Fang)",
						PL_PSIZ, DISP_IN_GAME },
	{ "dungeon",  "the symbols to use in drawing the dungeon map",
						MAXDCHARS+1, SET_IN_FILE },
	{ "effects",  "the symbols to use in drawing special effects",
						MAXECHARS+1, SET_IN_FILE },
	{ "font_map", "the font to use in the map window", 40, DISP_IN_GAME },	/*WC*/
	{ "font_menu", "the font to use in menus", 40, DISP_IN_GAME },		/*WC*/
	{ "font_message", "the font to use in the message window",
						40, DISP_IN_GAME },		/*WC*/
	{ "font_size_map", "the size of the map font", 20, DISP_IN_GAME },	/*WC*/
	{ "font_size_menu", "the size of the map font", 20, DISP_IN_GAME },	/*WC*/
	{ "font_size_message", "the size of the map font", 20, DISP_IN_GAME },	/*WC*/
	{ "font_size_status", "the size of the map font", 20, DISP_IN_GAME },	/*WC*/
	{ "font_size_text", "the size of the map font", 20, DISP_IN_GAME },	/*WC*/
	{ "font_status", "the font to use in status window", 40, DISP_IN_GAME }, /*WC*/
	{ "font_text", "the font to use in text windows", 40, DISP_IN_GAME },	/*WC*/
	{ "fruit",    "the name of a fruit you enjoy eating",
						PL_FSIZ, SET_IN_GAME },
	{ "gender",   "your starting gender (male or female)",
						8, DISP_IN_GAME },
	{ "horsename", "the name of your (first) horse (e.g., horsename:Silver)",
						PL_PSIZ, DISP_IN_GAME },
	{ "map_mode", "map display mode under Windows", 20, DISP_IN_GAME },	/*WC*/
	{ "menustyle", "user interface for object selection",
						MENUTYPELEN, SET_IN_GAME },
	{ "menu_deselect_all", "deselect all items in a menu", 4, SET_IN_FILE },
	{ "menu_deselect_page", "deselect all items on this page of a menu",
						4, SET_IN_FILE },
	{ "menu_first_page", "jump to the first page in a menu",
						4, SET_IN_FILE },
	{ "menu_invert_all", "invert all items in a menu", 4, SET_IN_FILE },
	{ "menu_invert_page", "invert all items on this page of a menu",
						4, SET_IN_FILE },
	{ "menu_last_page", "jump to the last page in a menu", 4, SET_IN_FILE },
	{ "menu_next_page", "goto the next menu page", 4, SET_IN_FILE },
	{ "menu_previous_page", "goto the previous menu page", 4, SET_IN_FILE },
	{ "menu_search", "search for a menu item", 4, SET_IN_FILE },
	{ "menu_select_all", "select all items in a menu", 4, SET_IN_FILE },
	{ "menu_select_page", "select all items on this page of a menu",
						4, SET_IN_FILE },
	{ "monsters", "the symbols to use for monsters",
						MAXMCLASSES, SET_IN_FILE },
	{ "msghistory", "number of top line messages to save",
						5, DISP_IN_GAME },
# ifdef TTY_GRAPHICS
	{"msg_window", "the type of message window required",1, SET_IN_GAME},
# else
	{"msg_window", "the type of message window required", 1, SET_IN_FILE},
# endif
	{ "name",     "your character's name (e.g., name:Merlin-W)",
						PL_NSIZ, DISP_IN_GAME },
	{ "objects",  "the symbols to use for objects",
						MAXOCLASSES, SET_IN_FILE },
	{ "packorder", "the inventory order of the items in your pack",
						MAXOCLASSES, SET_IN_GAME },
#ifdef CHANGE_COLOR
	{ "palette",  "palette (00c/880/-fff is blue/yellow/reverse white)",
						15 , SET_IN_GAME },
# if defined(MAC)
	{ "hicolor",  "same as palette, only order is reversed",
						15, SET_IN_FILE },
# endif
#endif
	{ "pettype",  "your preferred initial pet type", 4, DISP_IN_GAME },
	{ "pickup_burden",  "maximum burden picked up before prompt",
						20, SET_IN_GAME },
	{ "pickup_types", "types of objects to pick up automatically",
						MAXOCLASSES, SET_IN_GAME },
	{ "player_selection", "choose character via dialog or prompts",
						12, DISP_IN_GAME },
	{ "race",     "your starting race (e.g., Human, Elf)",
						PL_CSIZ, DISP_IN_GAME },
	{ "role",     "your starting role (e.g., Barbarian, Valkyrie)",
						PL_CSIZ, DISP_IN_GAME },
	{ "runmode", "display updating frequency when `running' or `travelling'",
						sizeof "teleport", SET_IN_GAME },
	{ "scores",   "the parts of the score list you wish to see",
						32, SET_IN_GAME },
	{ "scroll_amount", "scroll the map this amount when scroll_margin is reached",
						20, DISP_IN_GAME }, /*WC*/
	{ "scroll_margin", "scroll map when this far from the edge", 20, DISP_IN_GAME }, /*WC*/
#ifdef MSDOS
	{ "soundcard", "type of sound card to use", 20, SET_IN_FILE },
#endif
	{ "suppress_alert", "suppress alerts about version-specific features",
						8, SET_IN_GAME },
	{ "tile_width", "width of tiles", 20, DISP_IN_GAME},	/*WC*/
	{ "tile_height", "height of tiles", 20, DISP_IN_GAME},	/*WC*/
	{ "tile_file", "name of tile file", 70, DISP_IN_GAME},	/*WC*/
	{ "traps",    "the symbols to use in drawing traps",
						MAXTCHARS+1, SET_IN_FILE },
	{ "vary_msgcount", "show more old messages at a time", 20, DISP_IN_GAME }, /*WC*/
#ifdef MSDOS
	{ "video",    "method of video updating", 20, SET_IN_FILE },
#endif
#ifdef VIDEOSHADES
	{ "videocolors", "color mappings for internal screen routines",
						40, DISP_IN_GAME },
	{ "videoshades", "gray shades to map to black/gray/white",
						32, DISP_IN_GAME },
#endif
	{ "windowcolors",  "the foreground/background colors of windows",	/*WC*/
						80, DISP_IN_GAME },
	{ "windowtype", "windowing system to use", WINTYPELEN, DISP_IN_GAME },
	{ (char *)0, (char *)0, 0, 0 }
};

#ifdef OPTION_LISTS_ONLY
#undef static

#else	/* use rest of file */

static boolean need_redraw; /* for doset() */

#if defined(TOS) && defined(TEXTCOLOR)
extern boolean colors_changed;	/* in tos.c */
#endif

#ifdef VIDEOSHADES
extern char *shade[3];		  /* in sys/msdos/video.c */
extern char ttycolors[CLR_MAX];	  /* in sys/msdos/video.c */
#endif

static char def_inv_order[MAXOCLASSES] = {
	COIN_CLASS, AMULET_CLASS, WEAPON_CLASS, ARMOR_CLASS, FOOD_CLASS,
	SCROLL_CLASS, SPBOOK_CLASS, POTION_CLASS, RING_CLASS, WAND_CLASS,
	TOOL_CLASS, GEM_CLASS, ROCK_CLASS, BALL_CLASS, CHAIN_CLASS, 0,
};

/*
 * Default menu manipulation command accelerators.  These may _not_ be:
 *
 *	+ a number - reserved for counts
 *	+ an upper or lower case US ASCII letter - used for accelerators
 *	+ ESC - reserved for escaping the menu
 *	+ NULL, CR or LF - reserved for commiting the selection(s).  NULL
 *	  is kind of odd, but the tty's xwaitforspace() will return it if
 *	  someone hits a <ret>.
 *	+ a default object class symbol - used for object class accelerators
 *
 * Standard letters (for now) are:
 *
 *		<  back 1 page
 *		>  forward 1 page
 *		^  first page
 *		|  last page
 *		:  search
 *
 *		page		all
 *		 ,    select	 .
 *		 \    deselect	 -
 *		 ~    invert	 @
 *
 * The command name list is duplicated in the compopt array.
 */
typedef struct {
    const char *name;
    char cmd;
} menu_cmd_t;

#define NUM_MENU_CMDS 11
static const menu_cmd_t default_menu_cmd_info[NUM_MENU_CMDS] = {
/* 0*/	{ "menu_first_page",	MENU_FIRST_PAGE },
	{ "menu_last_page",	MENU_LAST_PAGE },
	{ "menu_next_page",	MENU_NEXT_PAGE },
	{ "menu_previous_page",	MENU_PREVIOUS_PAGE },
	{ "menu_select_all",	MENU_SELECT_ALL },
/* 5*/	{ "menu_deselect_all",	MENU_UNSELECT_ALL },
	{ "menu_invert_all",	MENU_INVERT_ALL },
	{ "menu_select_page",	MENU_SELECT_PAGE },
	{ "menu_deselect_page",	MENU_UNSELECT_PAGE },
	{ "menu_invert_page",	MENU_INVERT_PAGE },
/*10*/	{ "menu_search",		MENU_SEARCH },
};

/*
 * Allow the user to map incoming characters to various menu commands.
 * The accelerator list must be a valid C string.
 */
#define MAX_MENU_MAPPED_CMDS 32	/* some number */
       char mapped_menu_cmds[MAX_MENU_MAPPED_CMDS+1];	/* exported */
static char mapped_menu_op[MAX_MENU_MAPPED_CMDS+1];
static short n_menu_mapped = 0;


static boolean initial, from_file;

STATIC_DCL void FDECL(doset_add_menu, (winid,const char *,int));
STATIC_DCL void FDECL(nmcpy, (char *, const char *, int));
STATIC_DCL void FDECL(escapes, (const char *, char *));
STATIC_DCL void FDECL(rejectoption, (const char *));
STATIC_DCL void FDECL(badoption, (const char *));
STATIC_DCL char *FDECL(string_for_opt, (char *,BOOLEAN_P));
STATIC_DCL char *FDECL(string_for_env_opt, (const char *, char *,BOOLEAN_P));
STATIC_DCL void FDECL(bad_negation, (const char *,BOOLEAN_P));
STATIC_DCL int FDECL(change_inv_order, (char *));
STATIC_DCL void FDECL(oc_to_str, (char *, char *));
STATIC_DCL void FDECL(graphics_opts, (char *,const char *,int,int));
STATIC_DCL int FDECL(feature_alert_opts, (char *, const char *));
STATIC_DCL const char *FDECL(get_compopt_value, (const char *, char *));
STATIC_DCL boolean FDECL(special_handling, (const char *, BOOLEAN_P, BOOLEAN_P));
STATIC_DCL void FDECL(warning_opts, (char *,const char *));
STATIC_DCL void FDECL(duplicate_opt_detection, (const char *, int));

STATIC_OVL void FDECL(wc_set_font_name, (int, char *));
STATIC_OVL int FDECL(wc_set_window_colors, (char *));
STATIC_OVL boolean FDECL(is_wc_option, (const char *));
STATIC_OVL boolean FDECL(wc_supported, (const char *));

/* check whether a user-supplied option string is a proper leading
   substring of a particular option name; option string might have
   a colon or equals sign and arbitrary value appended to it */
boolean
match_optname(user_string, opt_name, min_length, val_allowed)
const char *user_string, *opt_name;
int min_length;
boolean val_allowed;
{
	int len = (int)strlen(user_string);

	if (val_allowed) {
	    const char *p = index(user_string, ':'),
		       *q = index(user_string, '=');

	    if (!p || (q && q < p)) p = q;
	    while(p && p > user_string && isspace(*(p-1))) p--;
	    if (p) len = (int)(p - user_string);
	}

	return (len >= min_length) && !strncmpi(opt_name, user_string, len);
}

/* most environment variables will eventually be printed in an error
 * message if they don't work, and most error message paths go through
 * BUFSZ buffers, which could be overflowed by a maliciously long
 * environment variable.  if a variable can legitimately be long, or
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
		return (char *)0;
}

void
initoptions()
{
	char *opts;
	int i;

	/* initialize the random number generator */
	setrandom();

	/* for detection of configfile options specified multiple times */
	iflags.opt_booldup = iflags.opt_compdup = (int *)0;
	
	for (i = 0; boolopt[i].name; i++) {
		if (boolopt[i].addr)
			*(boolopt[i].addr) = boolopt[i].initvalue;
	}
	flags.end_own = FALSE;
	flags.end_top = 3;
	flags.end_around = 2;
	iflags.runmode = RUN_LEAP;
	iflags.msg_history = 20;
#ifdef TTY_GRAPHICS
	iflags.prevmsg_window = 's';
#endif

	/* Use negative indices to indicate not yet selected */
	flags.initrole = -1;
	flags.initrace = -1;
	flags.initgend = -1;
	flags.initalign = -1;

	/* Set the default monster and object class symbols.  Don't use */
	/* memcpy() --- sizeof char != sizeof uchar on some machines.	*/
	for (i = 0; i < MAXOCLASSES; i++)
		oc_syms[i] = (uchar) def_oc_syms[i];
	for (i = 0; i < MAXMCLASSES; i++)
		monsyms[i] = (uchar) def_monsyms[i];
	for (i = 0; i < WARNCOUNT; i++)
		warnsyms[i] = def_warnsyms[i].sym;
	iflags.bouldersym = 0;
	flags.warnlevel = 1;
	flags.warntype = 0L;

     /* assert( sizeof flags.inv_order == sizeof def_inv_order ); */
	(void)memcpy((genericptr_t)flags.inv_order,
		     (genericptr_t)def_inv_order, sizeof flags.inv_order);
	flags.pickup_types[0] = '\0';
	flags.pickup_burden = MOD_ENCUMBER;

	for (i = 0; i < NUM_DISCLOSURE_OPTIONS; i++)
		flags.end_disclose[i] = DISCLOSE_PROMPT_DEFAULT_NO;
	switch_graphics(ASCII_GRAPHICS);	/* set default characters */
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
		switch_graphics(IBM_GRAPHICS);
# ifdef TEXTCOLOR
		iflags.use_color = TRUE;
# endif
	}
#endif /* UNIX && TTY_GRAPHICS */
#if defined(UNIX) || defined(VMS)
# ifdef TTY_GRAPHICS
	/* detect whether a "vt" terminal can handle alternate charsets */
	if ((opts = nh_getenv("TERM")) &&
	    !strncmpi(opts, "vt", 2) && AS && AE &&
	    index(AS, '\016') && index(AE, '\017')) {
		switch_graphics(DEC_GRAPHICS);
	}
# endif
#endif /* UNIX || VMS */

#ifdef MAC_GRAPHICS_ENV
	switch_graphics(MAC_GRAPHICS);
#endif /* MAC_GRAPHICS_ENV */
	flags.menu_style = MENU_FULL;

	/* since this is done before init_objects(), do partial init here */
	objects[SLIME_MOLD].oc_name_idx = SLIME_MOLD;
	nmcpy(pl_fruit, OBJ_NAME(objects[SLIME_MOLD]), PL_FSIZ);
#ifndef MAC
	opts = getenv("NETHACKOPTIONS");
	if (!opts) opts = getenv("HACKOPTIONS");
	if (opts) {
		if (*opts == '/' || *opts == '\\' || *opts == '@') {
			if (*opts == '@') opts++;	/* @filename */
			/* looks like a filename */
			if (strlen(opts) < BUFSZ/2)
			    read_config_file(opts);
		} else {
			read_config_file((char *)0);
			/* let the total length of options be long;
			 * parseoptions() will check each individually
			 */
			parseoptions(opts, TRUE, FALSE);
		}
	} else
#endif
		read_config_file((char *)0);

	(void)fruitadd(pl_fruit);
	/* Remove "slime mold" from list of object names; this will	*/
	/* prevent it from being wished unless it's actually present	*/
	/* as a named (or default) fruit.  Wishing for "fruit" will	*/
	/* result in the player's preferred fruit [better than "\033"].	*/
	obj_descr[SLIME_MOLD].oc_name = "fruit";

	return;
}

STATIC_OVL void
nmcpy(dest, src, maxlen)
	char	*dest;
	const char *src;
	int	maxlen;
{
	int	count;

	for(count = 1; count < maxlen; count++) {
		if(*src == ',' || *src == '\0') break; /*exit on \0 terminator*/
		*dest++ = *src++;
	}
	*dest = 0;
}

/*
 * escapes: escape expansion for showsyms. C-style escapes understood include
 * \n, \b, \t, \r, \xnnn (hex), \onnn (octal), \nnn (decimal). The ^-prefix
 * for control characters is also understood, and \[mM] followed by any of the
 * previous forms or by a character has the effect of 'meta'-ing the value (so
 * that the alternate character set will be enabled).
 */
STATIC_OVL void
escapes(cp, tp)
const char	*cp;
char *tp;
{
    while (*cp)
    {
	int	cval = 0, meta = 0;

	if (*cp == '\\' && index("mM", cp[1])) {
		meta = 1;
		cp += 2;
	}
	if (*cp == '\\' && index("0123456789xXoO", cp[1]))
	{
	    const char *dp, *hex = "00112233445566778899aAbBcCdDeEfF";
	    int dcount = 0;

	    cp++;
	    if (*cp == 'x' || *cp == 'X')
		for (++cp; (dp = index(hex, *cp)) && (dcount++ < 2); cp++)
		    cval = (cval * 16) + (dp - hex) / 2;
	    else if (*cp == 'o' || *cp == 'O')
		for (++cp; (index("01234567",*cp)) && (dcount++ < 3); cp++)
		    cval = (cval * 8) + (*cp - '0');
	    else
		for (; (index("0123456789",*cp)) && (dcount++ < 3); cp++)
		    cval = (cval * 10) + (*cp - '0');
	}
	else if (*cp == '\\')		/* C-style character escapes */
	{
	    switch (*++cp)
	    {
	    case '\\': cval = '\\'; break;
	    case 'n': cval = '\n'; break;
	    case 't': cval = '\t'; break;
	    case 'b': cval = '\b'; break;
	    case 'r': cval = '\r'; break;
	    default: cval = *cp;
	    }
	    cp++;
	}
	else if (*cp == '^')		/* expand control-character syntax */
	{
	    cval = (*++cp & 0x1f);
	    cp++;
	}
	else
	    cval = *cp++;
	if (meta)
	    cval |= 0x80;
	*tp++ = cval;
    }
    *tp = '\0';
}

STATIC_OVL void
rejectoption(optname)
const char *optname;
{
#ifdef MICRO
	pline("\"%s\" settable only from %s.", optname, configfile);
#else
	pline("%s can be set only from NETHACKOPTIONS or %s.", optname,
			configfile);
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
	else return;
#endif

	if(from_file)
	    raw_printf("Bad syntax in OPTIONS in %s: %s.", configfile, opts);
	else
	    raw_printf("Bad syntax in NETHACKOPTIONS: %s.", opts);

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
	if (!colon || (equals && equals < colon)) colon = equals;

	if (!colon || !*++colon) {
		if (!val_optional) badoption(opts);
		return (char *)0;
	}
	return colon;
}

STATIC_OVL char *
string_for_env_opt(optname, opts, val_optional)
const char *optname;
char *opts;
boolean val_optional;
{
	if(!initial) {
		rejectoption(optname);
		return (char *)0;
	}
	return string_for_opt(opts, val_optional);
}

STATIC_OVL void
bad_negation(optname, with_parameter)
const char *optname;
boolean with_parameter;
{
	pline_The("%s option may not %sbe negated.",
		optname,
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
#ifndef GOLDOBJ
    if (!index(op, GOLD_SYM))
	buf[num++] = COIN_CLASS;
#else
    /*  !!!! probably unnecessary with gold as normal inventory */
#endif

    for (sp = op; *sp; sp++) {
	oc_sym = def_char_to_objclass(*sp);
	/* reject bad or duplicate entries */
	if (oc_sym == MAXOCLASSES ||
		oc_sym == RANDOM_CLASS || oc_sym == ILLOBJ_CLASS ||
		!index(flags.inv_order, oc_sym) || index(sp+1, *sp))
	    return 0;
	/* retain good ones */
	buf[num++] = (char) oc_sym;
    }
    buf[num] = '\0';

    /* fill in any omitted classes, using previous ordering */
    for (sp = flags.inv_order; *sp; sp++)
	if (!index(buf, *sp)) {
	    buf[num++] = *sp;
	    buf[num] = '\0';	/* explicitly terminate for next index() */
	}

    Strcpy(flags.inv_order, buf);
    return 1;
}

STATIC_OVL void
graphics_opts(opts, optype, maxlen, offset)
register char *opts;
const char *optype;
int maxlen, offset;
{
	uchar translate[MAXPCHARS+1];
	int length, i;

	if (!(opts = string_for_env_opt(optype, opts, FALSE)))
		return;
	escapes(opts, opts);

	length = strlen(opts);
	if (length > maxlen) length = maxlen;
	/* match the form obtained from PC configuration files */
	for (i = 0; i < length; i++)
		translate[i] = (uchar) opts[i];
	assign_graphics(translate, length, maxlen, offset);
}

STATIC_OVL void
warning_opts(opts, optype)
register char *opts;
const char *optype;
{
	uchar translate[MAXPCHARS+1];
	int length, i;

	if (!(opts = string_for_env_opt(optype, opts, FALSE)))
		return;
	escapes(opts, opts);

	length = strlen(opts);
	if (length > WARNCOUNT) length = WARNCOUNT;
	/* match the form obtained from PC configuration files */
	for (i = 0; i < length; i++)
	     translate[i] = (((i < WARNCOUNT) && opts[i]) ?
			   (uchar) opts[i] : def_warnsyms[i].sym);
	assign_warnings(translate);
}

void
assign_warnings(graph_chars)
register uchar *graph_chars;
{
	int i;
	for (i = 0; i < WARNCOUNT; i++)
	    if (graph_chars[i]) warnsyms[i] = graph_chars[i];
}

STATIC_OVL int
feature_alert_opts(op, optn)
char *op;
const char *optn;
{
	char buf[BUFSZ];
	boolean rejectver = FALSE;
	unsigned long fnv = get_feature_notice_ver(op);		/* version.c */
	if (fnv == 0L) return 0;
	if (fnv > get_current_feature_ver())
		rejectver = TRUE;
	else
		flags.suppress_alert = fnv;
	if (rejectver) {
		if (!initial)
			You_cant("disable new feature alerts for future versions.");
		else {
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
		pline("Feature change alerts disabled for NetHack %s features and prior.",
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
		iflags.opt_booldup = (int *)alloc(SIZE(boolopt) * sizeof(int));
		optptr = iflags.opt_booldup;
		for (k = 0; k < SIZE(boolopt); ++k)
			*optptr++ = 0;
			
		if (iflags.opt_compdup)
			impossible("iflags.opt_compdup already on (memory leak)");
		iflags.opt_compdup = (int *)alloc(SIZE(compopt) * sizeof(int));
		optptr = iflags.opt_compdup;
		for (k = 0; k < SIZE(compopt); ++k)
			*optptr++ = 0;
	} else {
		/*-- OFF --*/
		if (iflags.opt_booldup) free((genericptr_t) iflags.opt_booldup);
		iflags.opt_booldup = (int *)0;
		if (iflags.opt_compdup) free((genericptr_t) iflags.opt_compdup);
		iflags.opt_compdup = (int *)0;
	} 
}

STATIC_OVL void
duplicate_opt_detection(opts, bool_or_comp)
const char *opts;
int bool_or_comp;	/* 0 == boolean option, 1 == compound */
{
	int i, *optptr;
#if defined(MAC)
	/* the Mac has trouble dealing with the output of messages while
	 * processing the config file.  That should get fixed one day.
	 * For now just return.
	 */
	return;
#endif
	if ((bool_or_comp == 0) && iflags.opt_booldup && initial && from_file) {
	    for (i = 0; boolopt[i].name; i++) {
		if (match_optname(opts, boolopt[i].name, 3, FALSE)) {
			optptr = iflags.opt_booldup + i;
			if (*optptr == 1) {
			    raw_printf(
				"\nWarning - Boolean option specified multiple times: %s.\n",
					opts);
			        wait_synch();
			}
			*optptr += 1;
			break; /* don't match multiple options */
		}
	    }
	} else if ((bool_or_comp == 1) && iflags.opt_compdup && initial && from_file) {
	    for (i = 0; compopt[i].name; i++) {
		if (match_optname(opts, compopt[i].name, strlen(compopt[i].name), TRUE)) {
			optptr = iflags.opt_compdup + i;
			if (*optptr == 1) {
			    raw_printf(
				"\nWarning - compound option specified multiple times: %s.\n",
					compopt[i].name);
			        wait_synch();
			}
			*optptr += 1;
			break; /* don't match multiple options */
		}
	    }
	}
}

void
parseoptions(opts, tinitial, tfrom_file)
register char *opts;
boolean tinitial, tfrom_file;
{
	register char *op;
	unsigned num;
	boolean negated;
	int i;
	const char *fullname;

	initial = tinitial;
	from_file = tfrom_file;
	if ((op = index(opts, ',')) != 0) {
		*op++ = 0;
		parseoptions(op, initial, from_file);
	}
	if (strlen(opts) > BUFSZ/2) {
		badoption("option too long");
		return;
	}

	/* strip leading and trailing white space */
	while (isspace(*opts)) opts++;
	op = eos(opts);
	while (--op >= opts && isspace(*op)) *op = '\0';

	if (!*opts) return;
	negated = FALSE;
	while ((*opts == '!') || !strncmpi(opts, "no", 2)) {
		if (*opts == '!') opts++; else opts += 2;
		negated = !negated;
	}

	/* variant spelling */

	if (match_optname(opts, "colour", 5, FALSE))
		Strcpy(opts, "color");	/* fortunately this isn't longer */

	duplicate_opt_detection(opts, 1);	/* 1 means compound opts */

	/* special boolean options */

	if (match_optname(opts, "female", 3, FALSE)) {
		if(!initial && flags.female == negated)
			pline("That is not anatomically possible.");
		else
			flags.initgend = flags.female = !negated;
		return;
	}

	if (match_optname(opts, "male", 4, FALSE)) {
		if(!initial && flags.female != negated)
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

	fullname = "pettype";
	if (match_optname(opts, fullname, 3, TRUE)) {
		if ((op = string_for_env_opt(fullname, opts, negated)) != 0) {
		    if (negated) bad_negation(fullname, TRUE);
		    else switch (*op) {
			case 'd':	/* dog */
			case 'D':
			    preferred_pet = 'd';
			    break;
			case 'c':	/* cat */
			case 'C':
			case 'f':	/* feline */
			case 'F':
			    preferred_pet = 'c';
			    break;
			case 'n':	/* no pet */
			case 'N':
			    preferred_pet = 'n';
			    break;
			default:
			    pline("Unrecognized pet type '%s'.", op);
			    break;
		    }
		} else if (negated) preferred_pet = 'n';
		return;
	}

	fullname = "catname";
	if (match_optname(opts, fullname, 3, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0)
			nmcpy(catname, op, PL_PSIZ);
		return;
	}

	fullname = "dogname";
	if (match_optname(opts, fullname, 3, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0)
			nmcpy(dogname, op, PL_PSIZ);
		return;
	}

	fullname = "horsename";
	if (match_optname(opts, fullname, 5, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0)
			nmcpy(horsename, op, PL_PSIZ);
		return;
	}

	fullname = "runmode";
	if (match_optname(opts, fullname, 4, TRUE)) {
		if (negated) {
			iflags.runmode = RUN_TPORT;
		} else if ((op = string_for_opt(opts, FALSE)) != 0) {
		    if (!strncmpi(op, "teleport", strlen(op)))
			iflags.runmode = RUN_TPORT;
		    else if (!strncmpi(op, "run", strlen(op)))
			iflags.runmode = RUN_LEAP;
		    else if (!strncmpi(op, "walk", strlen(op)))
			iflags.runmode = RUN_STEP;
		    else if (!strncmpi(op, "crawl", strlen(op)))
			iflags.runmode = RUN_CRAWL;
		    else
			badoption(opts);
		}
		return;
	}

	fullname = "msghistory";
	if (match_optname(opts, fullname, 3, TRUE)) {
		op = string_for_env_opt(fullname, opts, negated);
		if ((negated && !op) || (!negated && op)) {
			iflags.msg_history = negated ? 0 : atoi(op);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}

	fullname="msg_window";
	/* msg_window:single, combo, full or reversed */
	if (match_optname(opts, fullname, 4, TRUE)) {
	/* allow option to be silently ignored by non-tty ports */
#ifdef TTY_GRAPHICS
		int tmp;
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
			case 's':	/* single message history cycle (default if negated) */
				iflags.prevmsg_window = 's';
				break;
			case 'c':	/* combination: two singles, then full page reversed */
				iflags.prevmsg_window = 'c';
				break;
			case 'f':	/* full page (default if no opts) */
				iflags.prevmsg_window = 'f';
				break;
			case 'r':	/* full page (reversed) */
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
	if (!strncmpi(opts, fullname, 4))
	{
		int wintype = -1;
		char *fontopts = opts + 4;

		if (!strncmpi(fontopts, "map", 3) ||
		    !strncmpi(fontopts, "_map", 4))
			wintype = NHW_MAP;
		else if (!strncmpi(fontopts, "message", 7) ||
			 !strncmpi(fontopts, "_message", 8))
			wintype = NHW_MESSAGE;
		else if (!strncmpi(fontopts, "text", 4) ||
			 !strncmpi(fontopts, "_text", 5))
			wintype = NHW_TEXT;			
		else if (!strncmpi(fontopts, "menu", 4) ||
			 !strncmpi(fontopts, "_menu", 5))
			wintype = NHW_MENU;
		else if (!strncmpi(fontopts, "status", 6) ||
			 !strncmpi(fontopts, "_status", 7))
			wintype = NHW_STATUS;
		else if (!strncmpi(fontopts, "_size", 5)) {
			if (!strncmpi(fontopts, "_size_map", 8))
				wintype = NHW_MAP;
			else if (!strncmpi(fontopts, "_size_message", 12))
				wintype = NHW_MESSAGE;
			else if (!strncmpi(fontopts, "_size_text", 9))
				wintype = NHW_TEXT;
			else if (!strncmpi(fontopts, "_size_menu", 9))
				wintype = NHW_MENU;
			else if (!strncmpi(fontopts, "_size_status", 11))
				wintype = NHW_STATUS;
			else {
				badoption(opts);
				return;
			}
			if (wintype > 0 && !negated &&
			    (op = string_for_opt(opts, FALSE)) != 0) {
			    switch(wintype)  {
			    	case NHW_MAP:
					iflags.wc_fontsiz_map = atoi(op);
					break;
			    	case NHW_MESSAGE:
					iflags.wc_fontsiz_message = atoi(op);
					break;
			    	case NHW_TEXT:
					iflags.wc_fontsiz_text = atoi(op);
					break;
			    	case NHW_MENU:
					iflags.wc_fontsiz_menu = atoi(op);
					break;
			    	case NHW_STATUS:
					iflags.wc_fontsiz_status = atoi(op);
					break;
			    }
			}
			return;
		} else {
			badoption(opts);
		}
		if (wintype > 0 &&
		    (op = string_for_opt(opts, FALSE)) != 0) {
			wc_set_font_name(wintype, op);
#ifdef MAC
			set_font_name (wintype, op);
#endif
			return;
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}
#ifdef CHANGE_COLOR
	if (match_optname(opts, "palette", 3, TRUE)
# ifdef MAC
	    || match_optname(opts, "hicolor", 3, TRUE)
# endif
							) {
	    int color_number, color_incr;

# ifdef MAC
	    if (match_optname(opts, "hicolor", 3, TRUE)) {
		if (negated) {
		    bad_negation("hicolor", FALSE);
		    return;
		}
		color_number = CLR_MAX + 4;	/* HARDCODED inverse number */
		color_incr = -1;
	    } else {
# endif
		if (negated) {
		    bad_negation("palette", FALSE);
		    return;
		}
		color_number = 0;
		color_incr = 1;
# ifdef MAC
	    }
# endif
	    if ((op = string_for_opt(opts, FALSE)) != (char *)0) {
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
# ifdef AMIGA
			    rgb <<= 4;
# else
			    rgb <<= 8;
# endif
			    tmp = *(pt++);
			    if (isalpha(tmp)) {
				tmp = (tmp + 9) & 0xf;	/* Assumes ASCII... */
			    } else {
				tmp &= 0xf;	/* Digits in ASCII too... */
			    }
# ifndef AMIGA
			    /* Add an extra so we fill f -> ff and 0 -> 00 */
			    rgb += tmp << 4;
# endif
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
	    if (!initial) {
		need_redraw = TRUE;
	    }
	    return;
	}
#endif /* CHANGE_COLOR */

	if (match_optname(opts, "fruit", 2, TRUE)) {
		char empty_str = '\0';
		op = string_for_opt(opts, negated);
		if (negated) {
		    if (op) {
			bad_negation("fruit", TRUE);
			return;
		    }
		    op = &empty_str;
		    goto goodfruit;
		}
		if (!op) return;
		if (!initial) {
		    struct fruit *f;

		    num = 0;
		    for(f=ffruit; f; f=f->nextf) {
			if (!strcmp(op, f->fname)) goto goodfruit;
			num++;
		    }
		    if (num >= 100) {
			pline("Doing that so many times isn't very fruitful.");
			return;
		    }
		}
goodfruit:
		nmcpy(pl_fruit, op, PL_FSIZ);
	/* OBJ_NAME(objects[SLIME_MOLD]) won't work after initialization */
		if (!*pl_fruit)
		    nmcpy(pl_fruit, "slime mold", PL_FSIZ);
		if (!initial)
		    (void)fruitadd(pl_fruit);
		/* If initial, then initoptions is allowed to do it instead
		 * of here (initoptions always has to do it even if there's
		 * no fruit option at all.  Also, we don't want people
		 * setting multiple fruits in their options.)
		 */
		return;
	}

	/* graphics:string */
	fullname = "graphics";
	if (match_optname(opts, fullname, 2, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else graphics_opts(opts, fullname, MAXPCHARS, 0);
		return;
	}
	fullname = "dungeon";
	if (match_optname(opts, fullname, 2, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else graphics_opts(opts, fullname, MAXDCHARS, 0);
		return;
	}
	fullname = "traps";
	if (match_optname(opts, fullname, 2, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else graphics_opts(opts, fullname, MAXTCHARS, MAXDCHARS);
		return;
	}
	fullname = "effects";
	if (match_optname(opts, fullname, 2, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else
		 graphics_opts(opts, fullname, MAXECHARS, MAXDCHARS+MAXTCHARS);
		return;
	}

	/* objects:string */
	fullname = "objects";
	if (match_optname(opts, fullname, 7, TRUE)) {
		int length;

		if (negated) {
		    bad_negation(fullname, FALSE);
		    return;
		}
		if (!(opts = string_for_env_opt(fullname, opts, FALSE)))
			return;
		escapes(opts, opts);

		/*
		 * Override the default object class symbols.  The first
		 * object in the object class is the "random object".  I
		 * don't want to use 0 as an object class, so the "random
		 * object" is basically a place holder.
		 *
		 * The object class symbols have already been initialized in
		 * initoptions().
		 */
		length = strlen(opts);
		if (length >= MAXOCLASSES)
		    length = MAXOCLASSES-1;	/* don't count RANDOM_OBJECT */

		for (i = 0; i < length; i++)
		    oc_syms[i+1] = (uchar) opts[i];
		return;
	}

	/* monsters:string */
	fullname = "monsters";
	if (match_optname(opts, fullname, 8, TRUE)) {
		int length;

		if (negated) {
		    bad_negation(fullname, FALSE);
		    return;
		}
		if (!(opts = string_for_env_opt(fullname, opts, FALSE)))
			return;
		escapes(opts, opts);

		/* Override default mon class symbols set in initoptions(). */
		length = strlen(opts);
		if (length >= MAXMCLASSES)
		    length = MAXMCLASSES-1;	/* mon class 0 unused */

		for (i = 0; i < length; i++)
		    monsyms[i+1] = (uchar) opts[i];
		return;
	}
	fullname = "warnings";
	if (match_optname(opts, fullname, 5, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else warning_opts(opts, fullname);
		return;
	}
	/* boulder:symbol */
	fullname = "boulder";
	if (match_optname(opts, fullname, 7, TRUE)) {
		if (negated) {
		    bad_negation(fullname, FALSE);
		    return;
		}
/*		if (!(opts = string_for_env_opt(fullname, opts, FALSE))) */
		if (!(opts = string_for_opt(opts, FALSE)))
			return;
		escapes(opts, opts);

		/*
		 * Override the default boulder symbol.
		 */
		iflags.bouldersym = (uchar) opts[0];
		if (!initial) need_redraw = TRUE;
		return;
	}

	/* name:string */
	fullname = "name";
	if (match_optname(opts, fullname, 4, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0)
			nmcpy(plname, op, PL_NSIZ);
		return;
	}

	/* role:string or character:string */
	fullname = "role";
	if (match_optname(opts, fullname, 4, TRUE) ||
	    match_optname(opts, (fullname = "character"), 4, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0) {
			if ((flags.initrole = str2role(op)) == ROLE_NONE)
				badoption(opts);
			else  /* Backwards compatibility */
				nmcpy(pl_character, op, PL_NSIZ);
		}
		return;
	}

	/* race:string */
	fullname = "race";
	if (match_optname(opts, fullname, 4, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0) {
			if ((flags.initrace = str2race(op)) == ROLE_NONE)
				badoption(opts);
			else /* Backwards compatibility */
				pl_race = *op;
		}
		return;
	}

	/* gender:string */
	fullname = "gender";
	if (match_optname(opts, fullname, 4, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0) {
			if ((flags.initgend = str2gend(op)) == ROLE_NONE)
				badoption(opts);
			else
				flags.female = flags.initgend;
		}
		return;
	}

	/* WINCAP
	 * align_status:[left|top|right|bottom] */
	fullname = "align_status";
	if (match_optname(opts, fullname, sizeof("align_status")-1, TRUE)) {
		op = string_for_opt(opts, negated);
		if (op && !negated) {
		    if (!strncmpi (op, "left", sizeof("left")-1))
			iflags.wc_align_status = ALIGN_LEFT;
		    else if (!strncmpi (op, "top", sizeof("top")-1))
			iflags.wc_align_status = ALIGN_TOP;
		    else if (!strncmpi (op, "right", sizeof("right")-1))
			iflags.wc_align_status = ALIGN_RIGHT;
		    else if (!strncmpi (op, "bottom", sizeof("bottom")-1))
			iflags.wc_align_status = ALIGN_BOTTOM;
		    else
			badoption(opts);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}
	/* WINCAP
	 * align_message:[left|top|right|bottom] */
	fullname = "align_message";
	if (match_optname(opts, fullname, sizeof("align_message")-1, TRUE)) {
		op = string_for_opt(opts, negated);
		if (op && !negated) {
		    if (!strncmpi (op, "left", sizeof("left")-1))
			iflags.wc_align_message = ALIGN_LEFT;
		    else if (!strncmpi (op, "top", sizeof("top")-1))
			iflags.wc_align_message = ALIGN_TOP;
		    else if (!strncmpi (op, "right", sizeof("right")-1))
			iflags.wc_align_message = ALIGN_RIGHT;
		    else if (!strncmpi (op, "bottom", sizeof("bottom")-1))
			iflags.wc_align_message = ALIGN_BOTTOM;
		    else
			badoption(opts);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}
	/* align:string */
	fullname = "align";
	if (match_optname(opts, fullname, sizeof("align")-1, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0)
			if ((flags.initalign = str2align(op)) == ROLE_NONE)
				badoption(opts);
		return;
	}

	/* the order to list the pack */
	fullname = "packorder";
	if (match_optname(opts, fullname, 4, TRUE)) {
		if (negated) {
		    bad_negation(fullname, FALSE);
		    return;
		} else if (!(op = string_for_opt(opts, FALSE))) return;

		if (!change_inv_order(op))
			badoption(opts);
		return;
	}

	/* maximum burden picked up before prompt (Warren Cheung) */
	fullname = "pickup_burden";
	if (match_optname(opts, fullname, 8, TRUE)) {
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
		char ocl[MAXOCLASSES + 1], tbuf[MAXOCLASSES + 1],
		     qbuf[QBUFSZ], abuf[BUFSZ];
		int oc_sym;
		boolean badopt = FALSE, compat = (strlen(opts) <= 6), use_menu;

		oc_to_str(flags.pickup_types, tbuf);
		flags.pickup_types[0] = '\0';	/* all */
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
		    if (flags.menu_style == MENU_TRADITIONAL ||
			    flags.menu_style == MENU_COMBINATION) {
			use_menu = FALSE;
			Sprintf(qbuf, "New pickup_types: [%s am] (%s)",
				ocl, *tbuf ? tbuf : "all");
			getlin(qbuf, abuf);
			op = mungspaces(abuf);
			if (abuf[0] == '\0' || abuf[0] == '\033')
			    op = tbuf;		/* restore */
			else if (abuf[0] == 'm')
			    use_menu = TRUE;
		    }
		    if (use_menu) {
			(void) choose_classes_menu("Auto-Pickup what?", 1,
						   TRUE, ocl, tbuf);
			op = tbuf;
		    }
		}
		if (negated) {
		    bad_negation("pickup_types", TRUE);
		    return;
		}
		while (*op == ' ') op++;
		if (*op != 'a' && *op != 'A') {
		    num = 0;
		    while (*op) {
			oc_sym = def_char_to_objclass(*op);
			/* make sure all are valid obj symbols occuring once */
			if (oc_sym != MAXOCLASSES &&
			    !index(flags.pickup_types, oc_sym)) {
			    flags.pickup_types[num] = (char)oc_sym;
			    flags.pickup_types[++num] = '\0';
			} else
			    badopt = TRUE;
			op++;
		    }
		    if (badopt) badoption(opts);
		}
		return;
	}
	/* WINCAP
	 * player_selection: dialog | prompts */
	fullname = "player_selection";
	if (match_optname(opts, fullname, sizeof("player_selection")-1, TRUE)) {
		op = string_for_opt(opts, negated);
		if (op && !negated) {
		    if (!strncmpi (op, "dialog", sizeof("dialog")-1))
			iflags.wc_player_selection = VIA_DIALOG;
		    else if (!strncmpi (op, "prompt", sizeof("prompt")-1))
			iflags.wc_player_selection = VIA_PROMPTS;
		    else
		    	badoption(opts);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}

	/* things to disclose at end of game */
	if (match_optname(opts, "disclose", 7, TRUE)) {
		/*
		 * The order that the end_disclore options are stored:
		 * inventory, attribs, vanquished, genocided, conduct
		 * There is an array in flags:
		 *	end_disclose[NUM_DISCLOSURE_OPT];
		 * with option settings for the each of the following:
		 * iagvc [see disclosure_options in decl.c]:
		 * Legal setting values in that array are:
		 *	DISCLOSE_PROMPT_DEFAULT_YES  ask with default answer yes
		 *	DISCLOSE_PROMPT_DEFAULT_NO   ask with default answer no
		 *	DISCLOSE_YES_WITHOUT_PROMPT  always disclose and don't ask
		 *	DISCLOSE_NO_WITHOUT_PROMPT   never disclose and don't ask
		 *
		 * Those setting values can be used in the option
		 * string as a prefix to get the desired behaviour.
		 *
		 * For backward compatibility, no prefix is required,
		 * and the presence of a i,a,g,v, or c without a
		 * prefix sets the corresponding value to DISCLOSE_YES_WITHOUT_PROMPT;
		 */
		boolean badopt = FALSE;
		int idx, prefix_val;
		if (!(op = string_for_opt(opts, TRUE))) {
			/* for backwards compatibility, "disclose" without a
			 * value means all (was inventory and attributes,
			 * the only things available then), but negated
			 * it means "none"
			 * (note "none" contains none of "iavkgc")
			 */
			for (num = 0; num < NUM_DISCLOSURE_OPTIONS; num++) {
				if (negated)
				    flags.end_disclose[num] = DISCLOSE_NO_WITHOUT_PROMPT;
			 	else flags.end_disclose[num] = DISCLOSE_PROMPT_DEFAULT_YES;
			}
			return;
		}
		if (negated) {
			bad_negation("disclose", TRUE);
			return;
		}
		num = 0;
		prefix_val = -1;
		while (*op && num < sizeof flags.end_disclose - 1) {
			register char c, *dop;
			static char valid_settings[] = {
				DISCLOSE_PROMPT_DEFAULT_YES,
				DISCLOSE_PROMPT_DEFAULT_NO,
				DISCLOSE_YES_WITHOUT_PROMPT,
				DISCLOSE_NO_WITHOUT_PROMPT,
				'\0'
			};
			c = lowc(*op);
			if (c == 'k') c = 'v';	/* killed -> vanquished */
			dop = index(disclosure_options, c);
			if (dop) {
				idx = dop - disclosure_options;
				if (idx < 0 || idx > NUM_DISCLOSURE_OPTIONS - 1) {
				    impossible("bad disclosure index %d %c",
							idx, c);
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
				/* do nothing */
			} else
				badopt = TRUE;				
			op++;
		}
		if (badopt) badoption(opts);
		return;
	}

	/* scores:5t[op] 5a[round] o[wn] */
	if (match_optname(opts, "scores", 4, TRUE)) {
	    if (negated) {
		bad_negation("scores", FALSE);
		return;
	    }
	    if (!(op = string_for_opt(opts, FALSE))) return;

	    while (*op) {
		int inum = 1;

		if (digit(*op)) {
		    inum = atoi(op);
		    while (digit(*op)) op++;
		} else if (*op == '!') {
		    negated = !negated;
		    op++;
		}
		while (*op == ' ') op++;

		switch (*op) {
		 case 't':
		 case 'T':  flags.end_top = inum;
			    break;
		 case 'a':
		 case 'A':  flags.end_around = inum;
			    break;
		 case 'o':
		 case 'O':  flags.end_own = !negated;
			    break;
		 default:   badoption(opts);
			    return;
		}
		while (letter(*++op) || *op == ' ') continue;
		if (*op == '/') op++;
	    }
	    return;
	}

	fullname = "suppress_alert";
	if (match_optname(opts, fullname, 4, TRUE)) {
		op = string_for_opt(opts, negated);
		if (negated) bad_negation(fullname, FALSE);
		else if (op) (void) feature_alert_opts(op,fullname);
		return;
	}
	
#ifdef VIDEOSHADES
	/* videocolors:string */
	fullname = "videocolors";
	if (match_optname(opts, fullname, 6, TRUE) ||
	    match_optname(opts, "videocolours", 10, TRUE)) {
		if (negated) {
			bad_negation(fullname, FALSE);
			return;
		}
		else if (!(opts = string_for_env_opt(fullname, opts, FALSE))) {
			return;
		}
		if (!assign_videocolors(opts))
			badoption(opts);
		return;
	}
	/* videoshades:string */
	fullname = "videoshades";
	if (match_optname(opts, fullname, 6, TRUE)) {
		if (negated) {
			bad_negation(fullname, FALSE);
			return;
		}
		else if (!(opts = string_for_env_opt(fullname, opts, FALSE))) {
			return;
		}
		if (!assign_videoshades(opts))
			badoption(opts);
		return;
	}
#endif /* VIDEOSHADES */
#ifdef MSDOS
# ifdef NO_TERMS
	/* video:string -- must be after longer tests */
	fullname = "video";
	if (match_optname(opts, fullname, 5, TRUE)) {
		if (negated) {
			bad_negation(fullname, FALSE);
			return;
		}
		else if (!(opts = string_for_env_opt(fullname, opts, FALSE))) {
			return;
		}
		if (!assign_video(opts))
			badoption(opts);
		return;
	}
# endif /* NO_TERMS */
	/* soundcard:string -- careful not to match boolean 'sound' */
	fullname = "soundcard";
	if (match_optname(opts, fullname, 6, TRUE)) {
		if (negated) {
			bad_negation(fullname, FALSE);
			return;
		}
		else if (!(opts = string_for_env_opt(fullname, opts, FALSE))) {
			return;
		}
		if (!assign_soundcard(opts))
			badoption(opts);
		return;
	}
#endif /* MSDOS */

	/* WINCAP
	 * map_mode:[tiles|ascii4x6|ascii6x8|ascii8x8|ascii16x8|ascii7x12|ascii8x12|
			ascii16x12|ascii12x16|ascii10x18|fit_to_screen] */
	fullname = "map_mode";
	if (match_optname(opts, fullname, sizeof("map_mode")-1, TRUE)) {
		op = string_for_opt(opts, negated);
		if (op && !negated) {
		    if (!strncmpi (op, "tiles", sizeof("tiles")-1))
			iflags.wc_map_mode = MAP_MODE_TILES;
		    else if (!strncmpi (op, "ascii4x6", sizeof("ascii4x6")-1))
			iflags.wc_map_mode = MAP_MODE_ASCII4x6;
		    else if (!strncmpi (op, "ascii6x8", sizeof("ascii6x8")-1))
			iflags.wc_map_mode = MAP_MODE_ASCII6x8;
		    else if (!strncmpi (op, "ascii8x8", sizeof("ascii8x8")-1))
			iflags.wc_map_mode = MAP_MODE_ASCII8x8;
		    else if (!strncmpi (op, "ascii16x8", sizeof("ascii16x8")-1))
			iflags.wc_map_mode = MAP_MODE_ASCII16x8;
		    else if (!strncmpi (op, "ascii7x12", sizeof("ascii7x12")-1))
			iflags.wc_map_mode = MAP_MODE_ASCII7x12;
		    else if (!strncmpi (op, "ascii8x12", sizeof("ascii8x12")-1))
			iflags.wc_map_mode = MAP_MODE_ASCII8x12;
		    else if (!strncmpi (op, "ascii16x12", sizeof("ascii16x12")-1))
			iflags.wc_map_mode = MAP_MODE_ASCII16x12;
		    else if (!strncmpi (op, "ascii12x16", sizeof("ascii12x16")-1))
			iflags.wc_map_mode = MAP_MODE_ASCII12x16;
		    else if (!strncmpi (op, "ascii10x18", sizeof("ascii10x18")-1))
			iflags.wc_map_mode = MAP_MODE_ASCII10x18;
		    else if (!strncmpi (op, "fit_to_screen", sizeof("fit_to_screen")-1))
			iflags.wc_map_mode = MAP_MODE_ASCII_FIT_TO_SCREEN;
		    else
		    	badoption(opts);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}
	/* WINCAP
	 * scroll_amount:nn */
	fullname = "scroll_amount";
	if (match_optname(opts, fullname, sizeof("scroll_amount")-1, TRUE)) {
		op = string_for_opt(opts, negated);
		if ((negated && !op) || (!negated && op)) {
			iflags.wc_scroll_amount = negated ? 1 : atoi(op);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}
	/* WINCAP
	 * scroll_margin:nn */
	fullname = "scroll_margin";
	if (match_optname(opts, fullname, sizeof("scroll_margin")-1, TRUE)) {
		op = string_for_opt(opts, negated);
		if ((negated && !op) || (!negated && op)) {
			iflags.wc_scroll_margin = negated ? 5 : atoi(op);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}
	/* WINCAP
	 * tile_width:nn */
	fullname = "tile_width";
	if (match_optname(opts, fullname, sizeof("tile_width")-1, TRUE)) {
		op = string_for_opt(opts, negated);
		if ((negated && !op) || (!negated && op)) {
			iflags.wc_tile_width = negated ? 0 : atoi(op);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}
	/* WINCAP
	 * tile_file:name */
	fullname = "tile_file";
	if (match_optname(opts, fullname, sizeof("tile_file")-1, TRUE)) {
		if ((op = string_for_opt(opts, FALSE)) != 0) {
			if (iflags.wc_tile_file) free(iflags.wc_tile_file);
			iflags.wc_tile_file = (char *)alloc(strlen(op) + 1);
			Strcpy(iflags.wc_tile_file, op);
		}
		return;
	}
	/* WINCAP
	 * tile_height:nn */
	fullname = "tile_height";
	if (match_optname(opts, fullname, sizeof("tile_height")-1, TRUE)) {
		op = string_for_opt(opts, negated);
		if ((negated && !op) || (!negated && op)) {
			iflags.wc_tile_height = negated ? 0 : atoi(op);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}
	/* WINCAP
	 * vary_msgcount:nn */
	fullname = "vary_msgcount";
	if (match_optname(opts, fullname, sizeof("vary_msgcount")-1, TRUE)) {
		op = string_for_opt(opts, negated);
		if ((negated && !op) || (!negated && op)) {
			iflags.wc_vary_msgcount = negated ? 0 : atoi(op);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}

	fullname = "windowtype";
	if (match_optname(opts, fullname, 3, TRUE)) {
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

	/* WINCAP
	 * setting window colors
         * syntax: windowcolors=menu foregrnd/backgrnd text foregrnd/backgrnd
         */
	fullname = "windowcolors";
	if (match_optname(opts, fullname, 7, TRUE)) {
		if ((op = string_for_opt(opts, FALSE)) != 0) {
			if (!wc_set_window_colors(op))
				badoption(opts);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}

	/* menustyle:traditional or combo or full or partial */
	if (match_optname(opts, "menustyle", 4, TRUE)) {
		int tmp;
		boolean val_required = (strlen(opts) > 5 && !negated);

		if (!(op = string_for_opt(opts, !val_required))) {
		    if (val_required) return; /* string_for_opt gave feedback */
		    tmp = negated ? 'n' : 'f';
		} else {
		    tmp = tolower(*op);
		}
		switch (tmp) {
			case 'n':	/* none */
			case 't':	/* traditional */
				flags.menu_style = MENU_TRADITIONAL;
				break;
			case 'c':	/* combo: trad.class sel+menu */
				flags.menu_style = MENU_COMBINATION;
				break;
			case 'p':	/* partial: no class menu */
				flags.menu_style = MENU_PARTIAL;
				break;
			case 'f':	/* full: class menu + menu */
				flags.menu_style = MENU_FULL;
				break;
			default:
				badoption(opts);
		}
		return;
	}

	/* check for menu command mapping */
	for (i = 0; i < NUM_MENU_CMDS; i++) {
	    fullname = default_menu_cmd_info[i].name;
	    if (match_optname(opts, fullname, (int)strlen(fullname), TRUE)) {
		if (negated)
		    bad_negation(fullname, FALSE);
		else if ((op = string_for_opt(opts, FALSE)) != 0) {
		    int j;
		    char c, op_buf[BUFSZ];
		    boolean isbad = FALSE;

		    escapes(op, op_buf);
		    c = *op_buf;

		    if (c == 0 || c == '\r' || c == '\n' || c == '\033' ||
			    c == ' ' || digit(c) || (letter(c) && c != '@'))
			isbad = TRUE;
		    else	/* reject default object class symbols */
			for (j = 1; j < MAXOCLASSES; j++)
			    if (c == def_oc_syms[i]) {
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

			duplicate_opt_detection(boolopt[i].name, 0);

#if defined(TERMLIB) || defined(ASCIIGRAPH) || defined(MAC_GRAPHICS_ENV)
			if (FALSE
# ifdef TERMLIB
				 || (boolopt[i].addr) == &iflags.DECgraphics
# endif
# ifdef ASCIIGRAPH
				 || (boolopt[i].addr) == &iflags.IBMgraphics
# endif
# ifdef MAC_GRAPHICS_ENV
				 || (boolopt[i].addr) == &iflags.MACgraphics
# endif
				) {
# ifdef REINCARNATION
			    if (!initial && Is_rogue_level(&u.uz))
				assign_rogue_graphics(FALSE);
# endif
			    need_redraw = TRUE;
# ifdef TERMLIB
			    if ((boolopt[i].addr) == &iflags.DECgraphics)
				switch_graphics(iflags.DECgraphics ?
						DEC_GRAPHICS : ASCII_GRAPHICS);
# endif
# ifdef ASCIIGRAPH
			    if ((boolopt[i].addr) == &iflags.IBMgraphics)
				switch_graphics(iflags.IBMgraphics ?
						IBM_GRAPHICS : ASCII_GRAPHICS);
# endif
# ifdef MAC_GRAPHICS_ENV
			    if ((boolopt[i].addr) == &iflags.MACgraphics)
				switch_graphics(iflags.MACgraphics ?
						MAC_GRAPHICS : ASCII_GRAPHICS);
# endif
# ifdef REINCARNATION
			    if (!initial && Is_rogue_level(&u.uz))
				assign_rogue_graphics(TRUE);
# endif
			}
#endif /* TERMLIB || ASCIIGRAPH || MAC_GRAPHICS_ENV */

			/* only do processing below if setting with doset() */
			if (initial) return;

			if ((boolopt[i].addr) == &flags.time
#ifdef EXP_ON_BOTL
			 || (boolopt[i].addr) == &flags.showexp
#endif
#ifdef SCORE_ON_BOTL
			 || (boolopt[i].addr) == &flags.showscore
#endif
			    )
			    flags.botl = TRUE;

			else if ((boolopt[i].addr) == &flags.invlet_constant) {
			    if (flags.invlet_constant) reassign();
			}
#ifdef LAN_MAIL
			else if ((boolopt[i].addr) == &flags.biff) {
			    if (flags.biff) lan_mail_init();
			    else lan_mail_finish();
			}
#endif
			else if ((boolopt[i].addr) == &iflags.num_pad)
			    number_pad(iflags.num_pad ? 1 : 0);

			else if ((boolopt[i].addr) == &flags.lit_corridor) {
			    /*
			     * All corridor squares seen via night vision or
			     * candles & lamps change.  Update them by calling
			     * newsym() on them.  Don't do this if we are
			     * initializing the options --- the vision system
			     * isn't set up yet.
			     */
			    vision_recalc(2);		/* shut down vision */
			    vision_full_recalc = 1;	/* delayed recalc */
			}
			else if ((boolopt[i].addr) == &iflags.use_inverse ||
					(boolopt[i].addr) == &iflags.showrace ||
					(boolopt[i].addr) == &iflags.hilite_pet) {
			    need_redraw = TRUE;
			}
#ifdef TEXTCOLOR
			else if ((boolopt[i].addr) == &iflags.use_color) {
			    need_redraw = TRUE;
# ifdef TOS
			    if ((boolopt[i].addr) == &iflags.use_color
				&& iflags.BIOS) {
				if (colors_changed)
				    restore_colors();
				else
				    set_colors();
			    }
# endif
			}
#endif

			return;
		}
	}

	/* out of valid options */
	badoption(opts);
}


static NEARDATA const char *menutype[] = {
	"traditional", "combination", "partial", "full"
};

static NEARDATA const char *burdentype[] = {
	"unencumbered", "burdened", "stressed",
	"strained", "overtaxed", "overloaded"
};

static NEARDATA const char *runmodes[] = {
	"teleport", "run", "walk", "crawl"
};

/*
 * Convert the given string of object classes to a string of default object
 * symbols.
 */
STATIC_OVL void
oc_to_str(src,dest)
    char *src, *dest;
{
    int i;

    while ((i = (int) *src++) != 0) {
	if (i < 0 || i >= MAXOCLASSES)
	    impossible("oc_to_str:  illegal object class %d", i);
	else
	    *dest++ = def_oc_syms[i];
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
    if (n_menu_mapped >= MAX_MENU_MAPPED_CMDS)
	pline("out of menu map space.");
    else {
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
	int idx = found - mapped_menu_cmds;
	ch = mapped_menu_op[idx];
    }
    return ch;
}


#if defined(MICRO) || defined(MAC) || defined(WIN32)
# define OPTIONS_HEADING "OPTIONS"
#else
# define OPTIONS_HEADING "NETHACKOPTIONS"
#endif

static char fmtstr_doset_add_menu[] = "%s%-15s [%s]   "; 
static char fmtstr_doset_add_menu_tab[] = "%s\t[%s]";

STATIC_OVL void
doset_add_menu(win, option, indexoffset)
    winid win;			/* window to add to */
    const char *option;		/* option name */
    int indexoffset;		/* value to add to index in compopt[], or zero
				   if option cannot be changed */
{
    const char *value = "unknown";		/* current value */
    char buf[BUFSZ], buf2[BUFSZ];
    anything any;
    int i;

    any.a_void = 0;
    if (indexoffset == 0) {
	any.a_int = 0;
	value = get_compopt_value(option, buf2);
    } else {
	for (i=0; compopt[i].name; i++)
	    if (strcmp(option, compopt[i].name) == 0) break;

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
	Sprintf(buf, fmtstr_doset_add_menu, any.a_int ? "" : "    ", option, value);
    else
	Sprintf(buf, fmtstr_doset_add_menu_tab, option, value);
    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, MENU_UNSELECTED);
}

/* Changing options via menu by Per Liboriussen */
int
doset()
{
	char buf[BUFSZ], buf2[BUFSZ];
	int i, pass, boolcount, pick_cnt, pick_idx, opt_indx;
	boolean *bool_p;
	winid tmpwin;
	anything any;
	menu_item *pick_list;
	int indexoffset, startpass, endpass;
	boolean setinitial = FALSE, fromfile = FALSE;
	int biggest_name = 0;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);

	any.a_void = 0;
 add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD,
		 "Booleans (selecting will toggle value):", MENU_UNSELECTED);
	any.a_int = 0;
	/* first list any other non-modifiable booleans, then modifiable ones */
	for (pass = 0; pass <= 1; pass++)
	    for (i = 0; boolopt[i].name; i++)
		if ((bool_p = boolopt[i].addr) != 0 &&
			((boolopt[i].optflags == DISP_IN_GAME && pass == 0) ||
			 (boolopt[i].optflags == SET_IN_GAME && pass == 1))) {
		    if (bool_p == &flags.female) continue;  /* obsolete */
#ifdef WIZARD
		    if (bool_p == &iflags.sanity_check && !wizard) continue;
		    if (bool_p == &iflags.menu_tab_sep && !wizard) continue;
#endif
		    if (is_wc_option(boolopt[i].name) &&
			!wc_supported(boolopt[i].name)) continue;
		    any.a_int = (pass == 0) ? 0 : i + 1;
		    if (!iflags.menu_tab_sep)
			Sprintf(buf, "%s%-13s [%s]",
			    pass == 0 ? "    " : "",
			    boolopt[i].name, *bool_p ? "true" : "false");
 		    else
			Sprintf(buf, "%s\t[%s]",
			    boolopt[i].name, *bool_p ? "true" : "false");
		    add_menu(tmpwin, NO_GLYPH, &any, 0, 0,
			     ATR_NONE, buf, MENU_UNSELECTED);
		}

	boolcount = i;
	indexoffset = boolcount;
	any.a_void = 0;
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
 add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD,
		 "Compounds (selecting will prompt for new value):",
		 MENU_UNSELECTED);

	startpass = DISP_IN_GAME;
	endpass = SET_IN_GAME;

	/* spin through the options to find the biggest name
           and adjust the format string accordingly if needed */
	biggest_name = 0;
	for (i = 0; compopt[i].name; i++)
		if (compopt[i].optflags >= startpass && compopt[i].optflags <= endpass &&
		    strlen(compopt[i].name) > (unsigned) biggest_name)
			biggest_name = (int) strlen(compopt[i].name);
	if (biggest_name > 30) biggest_name = 30;
	if (!iflags.menu_tab_sep)
		Sprintf(fmtstr_doset_add_menu, "%%s%%-%ds [%%s]", biggest_name);
	
	/* deliberately put `name', `role', `race', `gender' first */
	doset_add_menu(tmpwin, "name", 0);
	doset_add_menu(tmpwin, "role", 0);
	doset_add_menu(tmpwin, "race", 0);
	doset_add_menu(tmpwin, "gender", 0);

	for (pass = startpass; pass <= endpass; pass++) 
	    for (i = 0; compopt[i].name; i++)
		if (compopt[i].optflags == pass) {
 		    	if (!strcmp(compopt[i].name, "name") ||
		    	    !strcmp(compopt[i].name, "role") ||
		    	    !strcmp(compopt[i].name, "race") ||
		    	    !strcmp(compopt[i].name, "gender"))
		    	    	continue;
		    	else if (is_wc_option(compopt[i].name) &&
					!wc_supported(compopt[i].name))
		    		continue;
		    	else
				doset_add_menu(tmpwin, compopt[i].name,
					(pass == DISP_IN_GAME) ? 0 : indexoffset);
		}
#ifdef PREFIXES_IN_USE
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
 add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD,
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
		if (opt_indx < boolcount) {
		    /* boolean option */
		    Sprintf(buf, "%s%s", *boolopt[opt_indx].addr ? "!" : "",
			    boolopt[opt_indx].name);
		    parseoptions(buf, setinitial, fromfile);
		    if (wc_supported(boolopt[opt_indx].name))
			preference_update(boolopt[opt_indx].name);
		} else {
		    /* compound option */
		    opt_indx -= boolcount;

		    if (!special_handling(compopt[opt_indx].name,
							setinitial, fromfile)) {
			Sprintf(buf, "Set %s to what?", compopt[opt_indx].name);
			getlin(buf, buf2);
			if (buf2[0] == '\033')
			    continue;
			Sprintf(buf, "%s:%s", compopt[opt_indx].name, buf2);
			/* pass the buck */
			parseoptions(buf, setinitial, fromfile);
		    }
		    if (wc_supported(compopt[opt_indx].name))
			preference_update(compopt[opt_indx].name);
		}
	    }
	    free((genericptr_t)pick_list);
	    pick_list = (menu_item *)0;
	}

	destroy_nhwindow(tmpwin);
	if (need_redraw)
	    (void) doredraw();
	return 0;
}

STATIC_OVL boolean
special_handling(optname, setinitial, setfromfile)
const char *optname;
boolean setinitial,setfromfile;
{
    winid tmpwin;
    anything any;
    int i;
    char buf[BUFSZ];
    boolean retval = FALSE;
    
    /* Special handling of menustyle, pickup_burden, and pickup_types,
       disclose, runmode, and msg_window options. */
    if (!strcmp("menustyle", optname)) {
	const char *style_name;
	menu_item *style_pick = (menu_item *)0;
        tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	for (i = 0; i < SIZE(menutype); i++) {
		style_name = menutype[i];
    		/* note: separate `style_name' variable used
		   to avoid an optimizer bug in VAX C V2.3 */
		any.a_int = i + 1;
		add_menu(tmpwin, NO_GLYPH, &any, *style_name, 0,
			 ATR_NONE, style_name, MENU_UNSELECTED);
        }
	end_menu(tmpwin, "Select menustyle:");
	if (select_menu(tmpwin, PICK_ONE, &style_pick) > 0) {
		flags.menu_style = style_pick->item.a_int - 1;
		free((genericptr_t)style_pick);
        }
	destroy_nhwindow(tmpwin);
        retval = TRUE;
    } else if (!strcmp("pickup_burden", optname)) {
	const char *burden_name, *burden_letters = "ubsntl";
	menu_item *burden_pick = (menu_item *)0;
        tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	for (i = 0; i < SIZE(burdentype); i++) {
		burden_name = burdentype[i];
		any.a_int = i + 1;
		add_menu(tmpwin, NO_GLYPH, &any, burden_letters[i], 0,
			 ATR_NONE, burden_name, MENU_UNSELECTED);
        }
	end_menu(tmpwin, "Select encumbrance level:");
	if (select_menu(tmpwin, PICK_ONE, &burden_pick) > 0) {
		flags.pickup_burden = burden_pick->item.a_int - 1;
		free((genericptr_t)burden_pick);
	}
	destroy_nhwindow(tmpwin);
	retval = TRUE;
    } else if (!strcmp("pickup_types", optname)) {
	/* parseoptions will prompt for the list of types */
	parseoptions(strcpy(buf, "pickup_types"), setinitial, setfromfile);
	retval = TRUE;
    } else if (!strcmp("disclose", optname)) {
	int pick_cnt, pick_idx, opt_idx;
	menu_item *disclosure_category_pick = (menu_item *)0;
	/*
	 * The order of disclose_names[]
         * must correspond to disclosure_options in decl.h
         */
	static const char *disclosure_names[] = {
		"inventory", "attributes", "vanquished", "genocides", "conduct"
	};
	int disc_cat[NUM_DISCLOSURE_OPTIONS];
	const char *disclosure_name;

        tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	for (i = 0; i < NUM_DISCLOSURE_OPTIONS; i++) {
		disclosure_name = disclosure_names[i];
		any.a_int = i + 1;
		add_menu(tmpwin, NO_GLYPH, &any, disclosure_options[i], 0,
			 ATR_NONE, disclosure_name, MENU_UNSELECTED);
		disc_cat[i] = 0;
        }
	end_menu(tmpwin, "Change which disclosure options categories:");
	if ((pick_cnt = select_menu(tmpwin, PICK_ANY, &disclosure_category_pick)) > 0) {
	    for (pick_idx = 0; pick_idx < pick_cnt; ++pick_idx) {
		opt_idx = disclosure_category_pick[pick_idx].item.a_int - 1;
		disc_cat[opt_idx] = 1;
	    }
	    free((genericptr_t)disclosure_category_pick);
	    disclosure_category_pick = (menu_item *)0;
	}
	destroy_nhwindow(tmpwin);

	for (i = 0; i < NUM_DISCLOSURE_OPTIONS; i++) {
	    if (disc_cat[i]) {
	    	char dbuf[BUFSZ];
		menu_item *disclosure_option_pick = (menu_item *)0;
		Sprintf(dbuf, "Disclosure options for %s:", disclosure_names[i]);
	        tmpwin = create_nhwindow(NHW_MENU);
		start_menu(tmpwin);
		any.a_char = DISCLOSE_NO_WITHOUT_PROMPT;
		add_menu(tmpwin, NO_GLYPH, &any, 'a', 0,
			ATR_NONE,"Never disclose and don't prompt", MENU_UNSELECTED);
		any.a_void = 0;
		any.a_char = DISCLOSE_YES_WITHOUT_PROMPT;
		add_menu(tmpwin, NO_GLYPH, &any, 'b', 0,
			ATR_NONE,"Always disclose and don't prompt", MENU_UNSELECTED);
		any.a_void = 0;
		any.a_char = DISCLOSE_PROMPT_DEFAULT_NO;
		add_menu(tmpwin, NO_GLYPH, &any, 'c', 0,
			ATR_NONE,"Prompt and default answer to \"No\"", MENU_UNSELECTED);
		any.a_void = 0;
		any.a_char = DISCLOSE_PROMPT_DEFAULT_YES;
		add_menu(tmpwin, NO_GLYPH, &any, 'd', 0,
			ATR_NONE,"Prompt and default answer to \"Yes\"", MENU_UNSELECTED);
		end_menu(tmpwin, dbuf);
		if (select_menu(tmpwin, PICK_ONE, &disclosure_option_pick) > 0) {
			flags.end_disclose[i] = disclosure_option_pick->item.a_char;
			free((genericptr_t)disclosure_option_pick);
		}
		destroy_nhwindow(tmpwin);
	    }
	}
	retval = TRUE;
    } else if (!strcmp("runmode", optname)) {
	const char *mode_name;
	menu_item *mode_pick = (menu_item *)0;
	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	for (i = 0; i < SIZE(runmodes); i++) {
		mode_name = runmodes[i];
		any.a_int = i + 1;
		add_menu(tmpwin, NO_GLYPH, &any, *mode_name, 0,
			 ATR_NONE, mode_name, MENU_UNSELECTED);
	}
	end_menu(tmpwin, "Select run/travel display mode:");
	if (select_menu(tmpwin, PICK_ONE, &mode_pick) > 0) {
		iflags.runmode = mode_pick->item.a_int - 1;
		free((genericptr_t)mode_pick);
	}
	destroy_nhwindow(tmpwin);
	retval = TRUE;
    } 
#ifdef TTY_GRAPHICS
      else if (!strcmp("msg_window", optname)) {
	/* by Christian W. Cooper */
	menu_item *window_pick = (menu_item *)0;
	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_char = 's';
	add_menu(tmpwin, NO_GLYPH, &any, 's', 0,
		ATR_NONE, "single", MENU_UNSELECTED);
	any.a_char = 'c';
	add_menu(tmpwin, NO_GLYPH, &any, 'c', 0,
		ATR_NONE, "combination", MENU_UNSELECTED);
	any.a_char = 'f';
	add_menu(tmpwin, NO_GLYPH, &any, 'f', 0,
		ATR_NONE, "full", MENU_UNSELECTED);
	any.a_char = 'r';
	add_menu(tmpwin, NO_GLYPH, &any, 'r', 0,
		ATR_NONE, "reversed", MENU_UNSELECTED);
	end_menu(tmpwin, "Select message history display type:");
	if (select_menu(tmpwin, PICK_ONE, &window_pick) > 0) {
		iflags.prevmsg_window = window_pick->item.a_char;
		free((genericptr_t)window_pick);
	}
	destroy_nhwindow(tmpwin);
        retval = TRUE;
    }
#endif
     else if (!strcmp("align_message", optname) ||
		!strcmp("align_status", optname)) {
	menu_item *window_pick = (menu_item *)0;
	char abuf[BUFSZ];
	boolean msg = (*(optname+6) == 'm');

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_int = ALIGN_TOP;
	add_menu(tmpwin, NO_GLYPH, &any, 't', 0,
		ATR_NONE, "top", MENU_UNSELECTED);
	any.a_int = ALIGN_BOTTOM;
	add_menu(tmpwin, NO_GLYPH, &any, 'b', 0,
		ATR_NONE, "bottom", MENU_UNSELECTED);
	any.a_int = ALIGN_LEFT;
	add_menu(tmpwin, NO_GLYPH, &any, 'l', 0,
		ATR_NONE, "left", MENU_UNSELECTED);
	any.a_int = ALIGN_RIGHT;
	add_menu(tmpwin, NO_GLYPH, &any, 'r', 0,
		ATR_NONE, "right", MENU_UNSELECTED);
	Sprintf(abuf, "Select %s window placement relative to the map:",
		msg ? "message" : "status");
	end_menu(tmpwin, abuf);
	if (select_menu(tmpwin, PICK_ONE, &window_pick) > 0) {		
		if (msg) iflags.wc_align_message = window_pick->item.a_int;
		else iflags.wc_align_status = window_pick->item.a_int;
		free((genericptr_t)window_pick);
	}
	destroy_nhwindow(tmpwin);
        retval = TRUE;
    }
    return retval;
}

#define rolestring(val,array,field) ((val >= 0) ? array[val].field : \
				     (val == ROLE_RANDOM) ? randomrole : none)

/* This is ugly. We have all the option names in the compopt[] array,
   but we need to look at each option individually to get the value. */
STATIC_OVL const char *
get_compopt_value(optname, buf)
const char *optname;
char *buf;
{
	char ocl[MAXOCLASSES+1];
	static const char none[] = "(none)", randomrole[] = "random",
		     to_be_done[] = "(to be done)",
		     defopt[] = "default",
		     defbrief[] = "def";
	int i;

	buf[0] = '\0';
	if (!strcmp(optname,"align_message"))
		Sprintf(buf, "%s", iflags.wc_align_message == ALIGN_TOP     ? "top" :
				   iflags.wc_align_message == ALIGN_LEFT    ? "left" :
				   iflags.wc_align_message == ALIGN_BOTTOM  ? "bottom" :
				   iflags.wc_align_message == ALIGN_RIGHT   ? "right" :
				   defopt);
	else if (!strcmp(optname,"align_status"))
		Sprintf(buf, "%s", iflags.wc_align_status == ALIGN_TOP     ? "top" :
				   iflags.wc_align_status == ALIGN_LEFT    ? "left" :
				   iflags.wc_align_status == ALIGN_BOTTOM  ? "bottom" :
				   iflags.wc_align_status == ALIGN_RIGHT   ? "right" :
				   defopt);
	else if (!strcmp(optname,"align"))
		Sprintf(buf, "%s", rolestring(flags.initalign, aligns, adj));
	else if (!strcmp(optname, "boulder"))
		Sprintf(buf, "%c", iflags.bouldersym ?
			iflags.bouldersym : oc_syms[(int)objects[BOULDER].oc_class]);
	else if (!strcmp(optname, "catname")) 
		Sprintf(buf, "%s", catname[0] ? catname : none );
	else if (!strcmp(optname, "disclose")) {
		for (i = 0; i < NUM_DISCLOSURE_OPTIONS; i++) {
			char topt[2];
			if (i) Strcat(buf," ");
			topt[1] = '\0';
			topt[0] = flags.end_disclose[i];
			Strcat(buf, topt);
			topt[0] = disclosure_options[i];
			Strcat(buf, topt);
		}
	}
	else if (!strcmp(optname, "dogname")) 
		Sprintf(buf, "%s", dogname[0] ? dogname : none );
	else if (!strcmp(optname, "dungeon"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "effects"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "font_map"))
		Sprintf(buf, "%s", iflags.wc_font_map ? iflags.wc_font_map : defopt);
	else if (!strcmp(optname, "font_message"))
		Sprintf(buf, "%s", iflags.wc_font_message ? iflags.wc_font_message : defopt);
	else if (!strcmp(optname, "font_status"))
		Sprintf(buf, "%s", iflags.wc_font_status ? iflags.wc_font_status : defopt);
	else if (!strcmp(optname, "font_menu"))
		Sprintf(buf, "%s", iflags.wc_font_menu ? iflags.wc_font_menu : defopt);
	else if (!strcmp(optname, "font_text"))
		Sprintf(buf, "%s", iflags.wc_font_text ? iflags.wc_font_text : defopt);
	else if (!strcmp(optname, "font_size_map")) {
		if (iflags.wc_fontsiz_map) Sprintf(buf, "%d", iflags.wc_fontsiz_map);
		else Strcpy(buf, defopt);
	}
	else if (!strcmp(optname, "font_size_message")) {
		if (iflags.wc_fontsiz_message) Sprintf(buf, "%d",
							iflags.wc_fontsiz_message);
		else Strcpy(buf, defopt);
	}
	else if (!strcmp(optname, "font_size_status")) {
		if (iflags.wc_fontsiz_status) Sprintf(buf, "%d", iflags.wc_fontsiz_status);
		else Strcpy(buf, defopt);
	}
	else if (!strcmp(optname, "font_size_menu")) {
		if (iflags.wc_fontsiz_menu) Sprintf(buf, "%d", iflags.wc_fontsiz_menu);
		else Strcpy(buf, defopt);
	}
	else if (!strcmp(optname, "font_size_text")) {
		if (iflags.wc_fontsiz_text) Sprintf(buf, "%d",iflags.wc_fontsiz_text);
		else Strcpy(buf, defopt);
	}
	else if (!strcmp(optname, "fruit")) 
		Sprintf(buf, "%s", pl_fruit);
	else if (!strcmp(optname, "gender"))
		Sprintf(buf, "%s", rolestring(flags.initgend, genders, adj));
	else if (!strcmp(optname, "horsename")) 
		Sprintf(buf, "%s", horsename[0] ? horsename : none);
	else if (!strcmp(optname, "map_mode"))
		Sprintf(buf, "%s",
			iflags.wc_map_mode == MAP_MODE_TILES      ? "tiles" :
			iflags.wc_map_mode == MAP_MODE_ASCII4x6   ? "ascii4x6" :
			iflags.wc_map_mode == MAP_MODE_ASCII6x8   ? "ascii6x8" :
			iflags.wc_map_mode == MAP_MODE_ASCII8x8   ? "ascii8x8" :
			iflags.wc_map_mode == MAP_MODE_ASCII16x8  ? "ascii16x8" :
			iflags.wc_map_mode == MAP_MODE_ASCII7x12  ? "ascii7x12" :
			iflags.wc_map_mode == MAP_MODE_ASCII8x12  ? "ascii8x12" :
			iflags.wc_map_mode == MAP_MODE_ASCII16x12 ? "ascii16x12" :
			iflags.wc_map_mode == MAP_MODE_ASCII12x16 ? "ascii12x16" :
			iflags.wc_map_mode == MAP_MODE_ASCII10x18 ? "ascii10x18" :
			iflags.wc_map_mode == MAP_MODE_ASCII_FIT_TO_SCREEN ?
			"fit_to_screen" : defopt);
	else if (!strcmp(optname, "menustyle")) 
		Sprintf(buf, "%s", menutype[(int)flags.menu_style] );
	else if (!strcmp(optname, "menu_deselect_all"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "menu_deselect_page"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "menu_first_page"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "menu_invert_all"))
		Sprintf(buf, "%s", to_be_done);
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
	else if (!strcmp(optname, "monsters"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "msghistory"))
		Sprintf(buf, "%u", iflags.msg_history);
#ifdef TTY_GRAPHICS
	else if (!strcmp(optname, "msg_window"))
		Sprintf(buf, "%s", (iflags.prevmsg_window=='s') ? "single" :
					(iflags.prevmsg_window=='c') ? "combination" :
					(iflags.prevmsg_window=='f') ? "full" : "reversed");
#endif
	else if (!strcmp(optname, "name"))
		Sprintf(buf, "%s", plname);
	else if (!strcmp(optname, "objects"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "packorder")) {
		oc_to_str(flags.inv_order, ocl);
		Sprintf(buf, "%s", ocl);
	     }
#ifdef CHANGE_COLOR
	else if (!strcmp(optname, "palette")) 
		Sprintf(buf, "%s", get_color_string());
#endif
	else if (!strcmp(optname, "pettype")) 
		Sprintf(buf, "%s", (preferred_pet == 'c') ? "cat" :
				(preferred_pet == 'd') ? "dog" :
				(preferred_pet == 'n') ? "none" : "random");
	else if (!strcmp(optname, "pickup_burden"))
		Sprintf(buf, "%s", burdentype[flags.pickup_burden] );
	else if (!strcmp(optname, "pickup_types")) {
		oc_to_str(flags.pickup_types, ocl);
		Sprintf(buf, "%s", ocl[0] ? ocl : "all" );
	     }
	else if (!strcmp(optname, "race"))
		Sprintf(buf, "%s", rolestring(flags.initrace, races, noun));
	else if (!strcmp(optname, "role"))
		Sprintf(buf, "%s", rolestring(flags.initrole, roles, name.m));
	else if (!strcmp(optname, "runmode"))
		Sprintf(buf, "%s", runmodes[iflags.runmode]);
	else if (!strcmp(optname, "scores")) {
		Sprintf(buf, "%d top/%d around%s", flags.end_top,
				flags.end_around, flags.end_own ? "/own" : "");
	}
	else if (!strcmp(optname, "scroll_amount")) {
		if (iflags.wc_scroll_amount) Sprintf(buf, "%d",iflags.wc_scroll_amount);
		else Strcpy(buf, defopt);
	}
	else if (!strcmp(optname, "scroll_margin")) {
		if (iflags.wc_scroll_margin) Sprintf(buf, "%d",iflags.wc_scroll_margin);
		else Strcpy(buf, defopt);
	}
	else if (!strcmp(optname, "player_selection"))
		Sprintf(buf, "%s", iflags.wc_player_selection ? "prompts" : "dialog");
#ifdef MSDOS
	else if (!strcmp(optname, "soundcard"))
		Sprintf(buf, "%s", to_be_done);
#endif
	else if (!strcmp(optname, "suppress_alert")) {
	    if (flags.suppress_alert == 0L)
		Strcpy(buf, none);
	    else
		Sprintf(buf, "%lu.%lu.%lu",
			FEATURE_NOTICE_VER_MAJ,
			FEATURE_NOTICE_VER_MIN,
			FEATURE_NOTICE_VER_PATCH);
	}
	else if (!strcmp(optname, "tile_file"))
		Sprintf(buf, "%s", iflags.wc_tile_file ? iflags.wc_tile_file : defopt);
	else if (!strcmp(optname, "tile_height")) {
		if (iflags.wc_tile_height) Sprintf(buf, "%d",iflags.wc_tile_height);
		else Strcpy(buf, defopt);
	}
	else if (!strcmp(optname, "tile_width")) {
		if (iflags.wc_tile_width) Sprintf(buf, "%d",iflags.wc_tile_width);
		else Strcpy(buf, defopt);
	}
	else if (!strcmp(optname, "traps"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "vary_msgcount")) {
		if (iflags.wc_vary_msgcount) Sprintf(buf, "%d",iflags.wc_vary_msgcount);
		else Strcpy(buf, defopt);
	}
#ifdef MSDOS
	else if (!strcmp(optname, "video"))
		Sprintf(buf, "%s", to_be_done);
#endif
#ifdef VIDEOSHADES
	else if (!strcmp(optname, "videoshades"))
		Sprintf(buf, "%s-%s-%s", shade[0],shade[1],shade[2]);
	else if (!strcmp(optname, "videocolors"))
		Sprintf(buf, "%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d",
			ttycolors[CLR_RED], ttycolors[CLR_GREEN],
			ttycolors[CLR_BROWN], ttycolors[CLR_BLUE],
			ttycolors[CLR_MAGENTA], ttycolors[CLR_CYAN],
			ttycolors[CLR_ORANGE], ttycolors[CLR_BRIGHT_GREEN],
			ttycolors[CLR_YELLOW], ttycolors[CLR_BRIGHT_BLUE],
			ttycolors[CLR_BRIGHT_MAGENTA],
			ttycolors[CLR_BRIGHT_CYAN]);
#endif /* VIDEOSHADES */
	else if (!strcmp(optname, "windowtype"))
		Sprintf(buf, "%s", windowprocs.name);
	else if (!strcmp(optname, "windowcolors"))
		Sprintf(buf, "%s/%s %s/%s %s/%s %s/%s",
			iflags.wc_foregrnd_menu    ? iflags.wc_foregrnd_menu : defbrief,
			iflags.wc_backgrnd_menu    ? iflags.wc_backgrnd_menu : defbrief,
			iflags.wc_foregrnd_message ? iflags.wc_foregrnd_message : defbrief,
			iflags.wc_backgrnd_message ? iflags.wc_backgrnd_message : defbrief,
			iflags.wc_foregrnd_status  ? iflags.wc_foregrnd_status : defbrief,
			iflags.wc_backgrnd_status  ? iflags.wc_backgrnd_status : defbrief,
			iflags.wc_foregrnd_text    ? iflags.wc_foregrnd_text : defbrief,
			iflags.wc_backgrnd_text    ? iflags.wc_backgrnd_text : defbrief);
#ifdef PREFIXES_IN_USE
	else {
	    for (i = 0; i < PREFIX_COUNT; ++i)
		if (!strcmp(optname, fqn_prefix_names[i]) && fqn_prefix[i])
			Sprintf(buf, "%s", fqn_prefix[i]);
	}
#endif

	if (buf[0]) return buf;
	else return "unknown";
}

int
dotogglepickup()
{
	char buf[BUFSZ], ocl[MAXOCLASSES+1];

	flags.pickup = !flags.pickup;
	if (flags.pickup) {
	    oc_to_str(flags.pickup_types, ocl);
	    Sprintf(buf, "ON, for %s objects", ocl[0] ? ocl : "all");
	} else {
	    Strcpy(buf, "OFF");
	}
	pline("Autopickup: %s.", buf);
	return 0;
}

/* data for option_help() */
static const char *opt_intro[] = {
	"",
	"                 NetHack Options Help:",
	"",
#define CONFIG_SLOT 3	/* fill in next value at run-time */
	(char *)0,
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
	(char *)0
};

static const char *opt_epilog[] = {
	"",
 "Some of the options can be set only before the game is started; those",
	"items will not be selectable in the 'O' command's menu.",
	(char *)0
};

void
option_help()
{
    char buf[BUFSZ], buf2[BUFSZ];
    register int i;
    winid datawin;

    datawin = create_nhwindow(NHW_TEXT);
    Sprintf(buf, "Set options as OPTIONS=<options> in %s", configfile);
    opt_intro[CONFIG_SLOT] = (const char *) buf;
    for (i = 0; opt_intro[i]; i++)
	putstr(datawin, 0, opt_intro[i]);

    /* Boolean options */
    for (i = 0; boolopt[i].name; i++) {
	if (boolopt[i].addr) {
#ifdef WIZARD
	    if (boolopt[i].addr == &iflags.sanity_check && !wizard) continue;
	    if (boolopt[i].addr == &iflags.menu_tab_sep && !wizard) continue;
#endif
	    next_opt(datawin, boolopt[i].name);
	}
    }
    next_opt(datawin, "");

    /* Compound options */
    putstr(datawin, 0, "Compound options:");
    for (i = 0; compopt[i].name; i++) {
	Sprintf(buf2, "`%s'", compopt[i].name);
	Sprintf(buf, "%-20s - %s%c", buf2, compopt[i].descr,
		compopt[i+1].name ? ',' : '.');
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

	if (!buf) *(buf = (char *)alloc(BUFSZ)) = '\0';

	if (!*str) {
		s = eos(buf);
		if (s > &buf[1] && s[-2] == ',')
		    Strcpy(s - 2, ".");	/* replace last ", " */
		i = COLNO;	/* (greater than COLNO - 2) */
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
		free(buf),  buf = 0;
	}
	return;
}

/* Returns the fid of the fruit type; if that type already exists, it
 * returns the fid of that one; if it does not exist, it adds a new fruit
 * type to the chain and returns the new one.
 */
int
fruitadd(str)
char *str;
{
	register int i;
	register struct fruit *f;
	struct fruit *lastf = 0;
	int highest_fruit_id = 0;
	char buf[PL_FSIZ];
	boolean user_specified = (str == pl_fruit);
	/* if not user-specified, then it's a fruit name for a fruit on
	 * a bones level...
	 */

	/* Note: every fruit has an id (spe for fruit objects) of at least
	 * 1; 0 is an error.
	 */
	if (user_specified) {
		/* disallow naming after other foods (since it'd be impossible
		 * to tell the difference)
		 */

		boolean found = FALSE, numeric = FALSE;

		for (i = bases[FOOD_CLASS]; objects[i].oc_class == FOOD_CLASS;
						i++) {
			if (!strcmp(OBJ_NAME(objects[i]), pl_fruit)) {
				found = TRUE;
				break;
			}
		}
		{
		    char *c;

		    c = pl_fruit;

		    for(c = pl_fruit; *c >= '0' && *c <= '9'; c++)
			;
		    if (isspace(*c) || *c == 0) numeric = TRUE;
		}
		if (found || numeric ||
		    !strncmp(str, "cursed ", 7) ||
		    !strncmp(str, "uncursed ", 9) ||
		    !strncmp(str, "blessed ", 8) ||
		    !strncmp(str, "partly eaten ", 13) ||
		    (!strncmp(str, "tin of ", 7) &&
			(!strcmp(str+7, "spinach") ||
			 name_to_mon(str+7) >= LOW_PM)) ||
		    !strcmp(str, "empty tin") ||
		    ((!strncmp(eos(str)-7," corpse",7) ||
			    !strncmp(eos(str)-4, " egg",4)) &&
			name_to_mon(str) >= LOW_PM))
			{
				Strcpy(buf, pl_fruit);
				Strcpy(pl_fruit, "candied ");
				nmcpy(pl_fruit+8, buf, PL_FSIZ-8);
		}
	}
	for(f=ffruit; f; f = f->nextf) {
		lastf = f;
		if(f->fid > highest_fruit_id) highest_fruit_id = f->fid;
		if(!strncmp(str, f->fname, PL_FSIZ))
			goto nonew;
	}
	/* if adding another fruit would overflow spe, use a random
	   fruit instead... we've got a lot to choose from. */
	if (highest_fruit_id >= 127) return rnd(127);
	highest_fruit_id++;
	f = newfruit();
	if (ffruit) lastf->nextf = f;
	else ffruit = f;
	Strcpy(f->fname, str);
	f->fid = highest_fruit_id;
	f->nextf = 0;
nonew:
	if (user_specified) current_fruit = highest_fruit_id;
	return f->fid;
}

/*
 * This is a somewhat generic menu for taking a list of NetHack style
 * class choices and presenting them via a description
 * rather than the traditional NetHack characters.
 * (Benefits users whose first exposure to NetHack is via tiles).
 *
 * prompt
 *	     The title at the top of the menu.
 *
 * category: 0 = monster class
 *           1 = object  class
 *
 * way
 *	     FALSE = PICK_ONE, TRUE = PICK_ANY
 *
 * class_list
 *	     a null terminated string containing the list of choices.
 *
 * class_selection
 *	     a null terminated string containing the selected characters.
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
    menu_item *pick_list = (menu_item *)0;
    winid win;
    anything any;
    char buf[BUFSZ];
    int i, n;
    int ret;
    int next_accelerator, accelerator;

    if (class_list == (char *)0 || class_select == (char *)0) return 0;
    accelerator = 0;
    next_accelerator = 'a';
    any.a_void = 0;
    win = create_nhwindow(NHW_MENU);
    start_menu(win);
    while (*class_list) {
	const char *text;
	boolean selected;

	text = (char *)0;
	selected = FALSE;
	switch (category) {
		case 0:
			text = monexplain[def_char_to_monclass(*class_list)];
			accelerator = *class_list;
			Sprintf(buf, "%s", text);
			break;
		case 1:
			text = objexplain[def_char_to_objclass(*class_list)];
			accelerator = next_accelerator;
			Sprintf(buf, "%c  %s", *class_list, text);
			break;
		default:
			impossible("choose_classes_menu: invalid category %d",
					category);
	}
	if (way && *class_select) {	/* Selections there already */
		if (index(class_select, *class_list)) {
			selected = TRUE;
		}
	}
	any.a_int = *class_list;
	add_menu(win, NO_GLYPH, &any, accelerator,
		  category ? *class_list : 0,
		  ATR_NONE, buf, selected);
	++class_list;
	if (category > 0) {
		++next_accelerator;
		if (next_accelerator == ('z' + 1)) next_accelerator = 'A';
		if (next_accelerator == ('Z' + 1)) break;
	}
    }
    end_menu(win, prompt);
    n = select_menu(win, way ? PICK_ANY : PICK_ONE, &pick_list);
    destroy_nhwindow(win);
    if (n > 0) {
	for (i = 0; i < n; ++i)
	    *class_select++ = (char)pick_list[i].item.a_int;
	free((genericptr_t)pick_list);
	ret = n;
    } else if (n == -1) {
	class_select = eos(class_select);
	ret = -1;
    } else
	ret = 0;
    *class_select = '\0';
    return ret;
}

struct wc_Opt wc_options[] = {
	{"ascii_map", WC_ASCII_MAP},
	{"color", WC_COLOR},
	{"eight_bit_tty", WC_EIGHT_BIT_IN},
	{"hilite_pet", WC_HILITE_PET},
	{"popup_dialog", WC_POPUP_DIALOG},
	{"player_selection", WC_PLAYER_SELECTION},
	{"preload_tiles", WC_PRELOAD_TILES},
	{"tiled_map", WC_TILED_MAP},
	{"tile_file", WC_TILE_FILE},
	{"tile_width", WC_TILE_WIDTH},
	{"tile_height", WC_TILE_HEIGHT},
	{"use_inverse", WC_INVERSE},
	{"align_message", WC_ALIGN_MESSAGE},
	{"align_status", WC_ALIGN_STATUS},
	{"font_map", WC_FONT_MAP},
	{"font_menu", WC_FONT_MENU},
	{"font_message",WC_FONT_MESSAGE},
#if 0
	{"perm_invent",WC_PERM_INVENT},
#endif
	{"font_size_map", WC_FONTSIZ_MAP},
	{"font_size_menu", WC_FONTSIZ_MENU},
	{"font_size_message", WC_FONTSIZ_MESSAGE},
	{"font_size_status", WC_FONTSIZ_STATUS},
	{"font_size_text", WC_FONTSIZ_TEXT},
	{"font_status", WC_FONT_STATUS},
	{"font_text", WC_FONT_TEXT},
	{"map_mode", WC_MAP_MODE},
	{"scroll_amount", WC_SCROLL_AMOUNT},
	{"scroll_margin", WC_SCROLL_MARGIN},
	{"splash_screen", WC_SPLASH_SCREEN},
	{"vary_msgcount",WC_VARY_MSGCOUNT},
	{"windowcolors", WC_WINDOWCOLORS},
	{"mouse_support", WC_MOUSE_SUPPORT},
	{(char *)0, 0L}
};


/*
 * If a port wants to change or ensure that the
 * SET_IN_FILE, DISP_IN_GAME, or SET_IN_GAME status of an option is
 * correct (for controlling its display in the option menu) call
 * set_option_mod_status()
 * with the second argument of 0,2, or 3 respectively.
 */
void
set_option_mod_status(optnam, status)
const char *optnam;
int status;
{
	int k;
	if (status < SET_IN_FILE || status > SET_IN_GAME) {
		impossible("set_option_mod_status: status out of range %d.",
			   status);
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
 *    example: set_wc_option_mod_status(WC_COLOR|WC_SCROLL_MARGIN, SET_IN_GAME);
 */
void
set_wc_option_mod_status(optmask, status)
unsigned long optmask;
int status;
{
	int k = 0;
	if (status < SET_IN_FILE || status > SET_IN_GAME) {
		impossible("set_option_mod_status: status out of range %d.",
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
		if (!strcmp(wc_options[k].wc_name, optnam) &&
		    (windowprocs.wincap & wc_options[k].wc_bit))
			return TRUE;
		k++;
	}
	return FALSE;
}

STATIC_OVL void
wc_set_font_name(wtype, fontname)
int wtype;
char *fontname;
{
	char **fn = (char **)0;
	if (!fontname) return;
	switch(wtype) {
	    case NHW_MAP:
	    		fn = &iflags.wc_font_map;
			break;
	    case NHW_MESSAGE:
	    		fn = &iflags.wc_font_message;
			break;
	    case NHW_TEXT:
	    		fn = &iflags.wc_font_text;
			break;
	    case NHW_MENU:
	    		fn = &iflags.wc_font_menu;
			break;
	    case NHW_STATUS:
	    		fn = &iflags.wc_font_status;
			break;
	    default:
	    		return;
	}
	if (fn) {
		if (*fn) free(*fn);
		*fn = (char *)alloc(strlen(fontname) + 1);
		Strcpy(*fn, fontname);
	}
	return;
}

STATIC_OVL int
wc_set_window_colors(op)
char *op;
{
	/* syntax:
	 *  menu white/black message green/yellow status white/blue text white/black
	 */

	int j;
	char buf[BUFSZ];
	char *wn, *tfg, *tbg, *newop;
	static const char *wnames[] = { "menu", "message", "status", "text" };
	static const char *shortnames[] = { "mnu", "msg", "sts", "txt" };
	static char **fgp[] = {
		&iflags.wc_foregrnd_menu,
		&iflags.wc_foregrnd_message,
		&iflags.wc_foregrnd_status,
		&iflags.wc_foregrnd_text
	};
	static char **bgp[] = {
		&iflags.wc_backgrnd_menu,
		&iflags.wc_backgrnd_message,
		&iflags.wc_backgrnd_status,
		&iflags.wc_backgrnd_text
	};

	Strcpy(buf, op);
	newop = mungspaces(buf);
	while (newop && *newop) {

		wn = tfg = tbg = (char *)0;

		/* until first non-space in case there's leading spaces - before colorname*/
		while(*newop && isspace(*newop)) newop++;
		if (*newop) wn = newop;
		else return 0;

		/* until first space - colorname*/
		while(*newop && !isspace(*newop)) newop++;
		if (*newop) *newop = '\0';
		else return 0;
		newop++;

		/* until first non-space - before foreground*/
		while(*newop && isspace(*newop)) newop++;
		if (*newop) tfg = newop;
		else return 0;

		/* until slash - foreground */
		while(*newop && *newop != '/') newop++;
		if (*newop) *newop = '\0';
		else return 0;
		newop++;

		/* until first non-space (in case there's leading space after slash) - before background */
		while(*newop && isspace(*newop)) newop++;
		if (*newop) tbg = newop;
		else return 0;

		/* until first space - background */
		while(*newop && !isspace(*newop)) newop++;
		if (*newop) *newop++ = '\0';

		for (j = 0; j < 4; ++j) {
			if (!strcmpi(wn, wnames[j]) ||
			    !strcmpi(wn, shortnames[j])) {
				if (tfg && !strstri(tfg, " ")) {
					if (*fgp[j]) free(*fgp[j]);
					*fgp[j] = (char *)alloc(strlen(tfg) + 1);
					Strcpy(*fgp[j], tfg);
				}
				if (tbg && !strstri(tbg, " ")) {
					if (*bgp[j]) free(*bgp[j]);
					*bgp[j] = (char *)alloc(strlen(tbg) + 1);
					Strcpy(*bgp[j], tbg);
				}
 				break;
			}
		}
	}
	return 1;
}

#endif	/* OPTION_LISTS_ONLY */

/*options.c*/
