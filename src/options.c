/*	SCCS Id: @(#)options.c	3.3	2000/08/01	*/
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

/*
 *  NOTE:  If you add (or delete) an option, please update the short
 *  options help (option_help()), the long options help (dat/opthelp),
 *  and the current options setting display function (doset()),
 *  and also the Guidebooks.
 */

static struct Bool_Opt
{
	const char *name;
	boolean	*addr, initvalue;
} boolopt[] = {
#ifdef AMIGA
	{"altmeta", &flags.altmeta, TRUE},
#else
	{"altmeta", (boolean *)0, TRUE},
#endif
#ifdef MFLOPPY
	{"asksavedisk", &flags.asksavedisk, FALSE},
#else
	{"asksavedisk", (boolean *)0, FALSE},
#endif
	{"autodig", &flags.autodig, FALSE},
	{"autopickup", &flags.pickup, TRUE},
	{"autoquiver", &flags.autoquiver, FALSE},
#if defined(MICRO) && !defined(AMIGA)
	{"BIOS", &iflags.BIOS, FALSE},
#else
	{"BIOS", (boolean *)0, FALSE},
#endif
#ifdef INSURANCE
	{"checkpoint", &flags.ins_chkpt, TRUE},
#else
	{"checkpoint", (boolean *)0, FALSE},
#endif
#ifdef MFLOPPY
	{"checkspace", &iflags.checkspace, TRUE},
#else
	{"checkspace", (boolean *)0, FALSE},
#endif
#ifdef TEXTCOLOR
# ifdef MICRO
	{"color", &iflags.use_color, TRUE},
# else	/* systems that support multiple terminals, many monochrome */
	{"color", &iflags.use_color, FALSE},
# endif
#else
	{"color", (boolean *)0, FALSE},
#endif
	{"confirm",&flags.confirm, TRUE},
#ifdef TERMLIB
	{"DECgraphics", &iflags.DECgraphics, FALSE},
#else
	{"DECgraphics", (boolean *)0, FALSE},
#endif
#ifdef TTY_GRAPHICS
	{"eight_bit_tty", &iflags.eight_bit_tty, FALSE},
	{"extmenu", &iflags.extmenu, FALSE},
#else
	{"eight_bit_tty", (boolean *)0, FALSE},
	{"extmenu", (boolean *)0, FALSE},
#endif
#ifdef OPT_DISPMAP
	{"fast_map", &flags.fast_map, TRUE},
#else
	{"fast_map", (boolean *)0, TRUE},
#endif
	{"female", &flags.female, FALSE},
	{"fixinv", &flags.invlet_constant, TRUE},
#ifdef AMIFLUSH
	{"flush", &flags.amiflush, FALSE},
#else
	{"flush", (boolean *)0, FALSE},
#endif
	{"help", &flags.help, TRUE},
	{"hilite_pet", &iflags.hilite_pet, FALSE},
#ifdef ASCIIGRAPH
	{"IBMgraphics", &iflags.IBMgraphics, FALSE},
#else
	{"IBMgraphics", (boolean *)0, FALSE},
#endif
	{"ignintr", &flags.ignintr, FALSE},
#ifdef MAC_GRAPHICS_ENV
	{"large_font", &iflags.large_font, FALSE},
#else
	{"large_font", (boolean *)0, FALSE},
#endif
	{"legacy", &flags.legacy, TRUE},
	{"lit_corridor", &flags.lit_corridor, FALSE},
#ifdef MAC_GRAPHICS_ENV
	{"Macgraphics", &iflags.MACgraphics, TRUE},
#else
	{"Macgraphics", (boolean *)0, FALSE},
#endif
#ifdef MAIL
	{"mail", &flags.biff, TRUE},
#else
	{"mail", (boolean *)0, TRUE},
#endif
#ifdef TTY_GRAPHICS
	{"msg_window", &iflags.prevmsg_window, FALSE},
#else
	{"msg_window", (boolean *)0, FALSE},
#endif
#ifdef NEWS
	{"news", &iflags.news, TRUE},
#else
	{"news", (boolean *)0, FALSE},
#endif
	{"null", &flags.null, TRUE},
	{"number_pad", &iflags.num_pad, FALSE},
#ifdef MAC
	{"page_wait", &flags.page_wait, TRUE},
#else
	{"page_wait", (boolean *)0, FALSE},
#endif
	{"perm_invent", &flags.perm_invent, FALSE},
#ifdef MAC
	{"popup_dialog", &iflags.popup_dialog, FALSE},
#else
	{"popup_dialog", (boolean *)0, FALSE},
#endif
	{"prayconfirm", &flags.prayconfirm, TRUE},
#if defined(MSDOS) && defined(USE_TILES)
	{"preload_tiles", &iflags.preload_tiles, TRUE},
#else
	{"preload_tiles", (boolean *)0, FALSE},
#endif
	{"pushweapon", &flags.pushweapon, FALSE},
#if defined(MICRO) && !defined(AMIGA)
	{"rawio", &iflags.rawio, FALSE},
#else
	{"rawio", (boolean *)0, FALSE},
#endif
	{"rest_on_space", &flags.rest_on_space, FALSE},
	{"safe_pet", &flags.safe_dog, TRUE},
#ifdef WIZARD
	{"sanity_check", &iflags.sanity_check, FALSE},
#else
	{"sanity_check", (boolean *)0, FALSE},
#endif
#ifdef EXP_ON_BOTL
	{"showexp", &flags.showexp, FALSE},
#else
	{"showexp", (boolean *)0, FALSE},
#endif
#ifdef SCORE_ON_BOTL
	{"showscore", &flags.showscore, FALSE},
#else
	{"showscore", (boolean *)0, FALSE},
#endif
	{"silent", &flags.silent, TRUE},
	{"sortpack", &flags.sortpack, TRUE},
	{"sound", &flags.soundok, TRUE},
	{"sparkle", &flags.sparkle, TRUE},
	{"standout", &flags.standout, FALSE},
	{"time", &flags.time, FALSE},
#ifdef TIMED_DELAY
	{"timed_delay", &flags.nap, TRUE},
#else
	{"timed_delay", (boolean *)0, FALSE},
#endif
	{"tombstone",&flags.tombstone, TRUE},
	{"toptenwin",&flags.toptenwin, FALSE},
	{"use_inverse", &iflags.use_inverse, FALSE},
	{"verbose", &flags.verbose, TRUE},
	{(char *)0, (boolean *)0, FALSE}
};

/* compound options, for option_help() and external programs like Amiga
 * frontend */
#define SET_IN_FILE	0 /* config file option only, not visible in game
			   * or via program */
#define SET_VIA_PROG	1 /* may be set via extern program, not seen in game */
#define DISP_IN_GAME	2 /* may be set via extern program, displayed in game */
#define SET_IN_GAME	3 /* may be set via extern program or set in the game */
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
#ifdef MAC
	{ "background", "the color of the background (black or white)",
						6, SET_IN_FILE },
#endif
	{ "boulder",  "the symbol to use for displaying boulders",
						1, SET_IN_GAME },
	{ "catname",  "the name of your (first) cat (e.g., catname:Tabby)",
						PL_PSIZ, DISP_IN_GAME },
	{ "disclose", "the kinds of information to disclose at end of game",
						sizeof(flags.end_disclose),
						SET_IN_GAME },
	{ "dogname",  "the name of your (first) dog (e.g., dogname:Fang)",
						PL_PSIZ, DISP_IN_GAME },
	{ "dungeon",  "the symbols to use in drawing the dungeon map",
						MAXDCHARS+1, SET_IN_FILE },
	{ "effects",  "the symbols to use in drawing special effects",
						MAXECHARS+1, SET_IN_FILE },
#ifdef MAC
	{ "fontmap", "the font to use in the map window", 40, SET_IN_FILE },
	{ "fontmessage", "the font to use in the message window",
						40, SET_IN_FILE },
	{ "fonttext", "the font to use in text windows", 40, SET_IN_FILE },
#endif
	{ "fruit",    "the name of a fruit you enjoy eating",
						PL_FSIZ, SET_IN_GAME },
	{ "gender",   "your starting gender (male or female)",
						8, DISP_IN_GAME },
	{ "horsename", "the name of your (first) horse (e.g., horsename:Silver)",
						PL_PSIZ, DISP_IN_GAME },
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
	{ "race",     "your starting race (e.g., Human, Elf)",
						PL_CSIZ, DISP_IN_GAME },
	{ "role",     "your starting role (e.g., Barbarian, Valkyrie)",
						PL_CSIZ, DISP_IN_GAME },
	{ "scores",   "the parts of the score list you wish to see",
						32, SET_IN_GAME },
#ifdef MSDOS
	{ "soundcard", "type of sound card to use", 20, SET_IN_FILE },
#endif
	{ "suppress_alert", "suppress alerts about version-specific features",
						8, SET_IN_GAME },
	{ "traps",    "the symbols to use in drawing traps",
						MAXTCHARS+1, SET_IN_FILE },
#ifdef MAC
	{"use_stone", "use stone background patterns", 8, SET_IN_FILE },
#endif
#ifdef MSDOS
	{ "video",    "method of video updating", 20, SET_IN_FILE },
#endif
#ifdef VIDEOSHADES
	{ "videocolors", "color mappings for internal screen routines",
						40, DISP_IN_GAME },
	{ "videoshades", "gray shades to map to black/gray/white",
						32, DISP_IN_GAME },
#endif
#if 0
	{ "warnlevel", "minimum monster level to trigger warning", 4, SET_IN_GAME },
#endif
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
	GOLD_CLASS, AMULET_CLASS, WEAPON_CLASS, ARMOR_CLASS, FOOD_CLASS,
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
STATIC_DCL int FDECL(boolopt_only_initial, (int));
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
#if 0
STATIC_DCL int FDECL(warnlevel_opts, (char *, const char *));
#endif
STATIC_DCL void FDECL(duplicate_opt_detection, (const char *, int));

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
	iflags.msg_history = 20;

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
	if (!strncmp(nh_getenv("TERM"), "AT", 2)) {
		switch_graphics(IBM_GRAPHICS);
# ifdef TEXTCOLOR
		iflags.use_color = TRUE;
# endif
	}
#endif /* UNIX && TTY_GRAPHICS */
#if defined(UNIX) || defined(VMS)
# ifdef TTY_GRAPHICS
	/* detect whether a "vt" terminal can handle alternate charsets */
	if (!strncmpi(nh_getenv("TERM"), "vt", 2) && (AS && AE) &&
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

/* some boolean options can only be set on start-up */
STATIC_OVL int
boolopt_only_initial(i)
int i;
{
	return (boolopt[i].addr == &flags.female
	     || boolopt[i].addr == &flags.legacy
#if defined(MICRO) && !defined(AMIGA)
	     || boolopt[i].addr == &iflags.rawio
	     || boolopt[i].addr == &iflags.BIOS
#endif
#if defined(MSDOS) && defined(USE_TILES)
	     || boolopt[i].addr == &iflags.preload_tiles
#endif
	);
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
	buf[num++] = GOLD_CLASS;
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
	    warnsyms[i] = graph_chars[i];
}

#if 0
/* warnlevel is unnecessary with the new warning introduced in 3.3.1 */
STATIC_OVL int
warnlevel_opts(op, optn)
char *op;
const char *optn;
{
	char buf[BUFSZ];
	int twarnlevel;
	boolean rejectlevel = FALSE;

	if (op) {
		twarnlevel = atoi(op);
		if (twarnlevel >= WARNCOUNT || twarnlevel < 1)
			rejectlevel = TRUE;
		else {
			flags.warnlevel = twarnlevel;
			see_monsters();
		}
	}
	if (rejectlevel) {
		if (!initial)
			pline("warnlevel must be 1 to %d.", WARNCOUNT - 1);
		else {
			Sprintf(buf,
			    "\n%s=%s Invalid warnlevel ignored (must be 1 to %d)",
				optn, op, WARNCOUNT - 1);
			badoption(buf);
		}
		return 0;
	}
	if (!initial) {
		if (flags.warnlevel < WARNCOUNT -1)
			Sprintf(buf, "s %d to %d", flags.warnlevel, WARNCOUNT - 1);
		else
			Sprintf(buf, " %d", flags.warnlevel);
		pline("Warning level%s will be displayed.", buf);
	}
	return 1;
}
#endif

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
			    pline("Unrecognized pet type '%s'", op);
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

	fullname = "msghistory";
	if (match_optname(opts, fullname, 3, TRUE)) {
		op = string_for_env_opt(fullname, opts, negated);
		if ((negated && !op) || (!negated && op)) {
			iflags.msg_history = negated ? 0 : atoi(op);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}

#ifdef CHANGE_COLOR
#ifdef MAC
	fullname = "use_stone";
	if (match_optname(opts, fullname, 6, TRUE)) {
		op = string_for_env_opt(fullname, opts, negated);
		if ((negated && !op) || (!negated && op)) {
			iflags.use_stone = negated ? 0 : atoi(op);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}

	fullname = "background";
	if (match_optname(opts, fullname, 5,TRUE))
	{
		if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0)
		{
			if (!strncmpi (op, "white", 5))
				change_background (1);
			else if (!strncmpi (op, "black", 5))
				change_background (0);
		}
		return;
	}
	
	fullname = "font";
	if (!strncmpi(opts, fullname, 4))
	{	int wintype = -1;
	
		opts += 4;
		if (!strncmpi (opts, "map", 3))
			wintype = NHW_MAP;
		else if (!strncmpi (opts, "message", 7))
			wintype = NHW_MESSAGE;
		else if (!strncmpi (opts, "text", 4))
			wintype = NHW_TEXT;
				
		if (wintype > 0 && (op = string_for_env_opt(fullname, opts, FALSE)) != 0)
		{	set_font_name (wintype, op);
		}
		return;
	}
#endif
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
#endif

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
#if 0
	fullname = "warnlevel";
	if (match_optname(opts, fullname, 5, TRUE)) {
	    op = string_for_opt(opts, negated);
	    if (negated) bad_negation(fullname, FALSE);
	    else if (op) (void) warnlevel_opts(op,fullname);
	    return;
	}
#endif
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

	/* align:string */
	fullname = "align";
	if (match_optname(opts, fullname, 4, TRUE)) {
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

	/* things to disclose at end of game */
	if (match_optname(opts, "disclose", 4, TRUE)) {
		flags.end_disclose[0] = '\0';	/* all */
		if (!(op = string_for_opt(opts, TRUE))) {
			/* for backwards compatibility, "disclose" without a
			 * value means all (was inventory and attributes,
			 * the only things available then), but negated
			 * it means "none"
			 * (note "none" contains none of "iavkgc")
			 */
			if (negated) Strcpy(flags.end_disclose, "none");
			return;
		}
		if (negated) {
			bad_negation("disclose", TRUE);
			return;
		}
		num = 0;
		while (*op && num < sizeof flags.end_disclose - 1) {
			register char c;
			c = lowc(*op);
			if (c == 'k') c = 'v';	/* killed -> vanquished */
			if (!index(flags.end_disclose, c)) {
				flags.end_disclose[num++] = c;
				flags.end_disclose[num] = '\0';	/* for index */
			}
			op++;
		}
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
			if (!initial && boolopt_only_initial(i)) {
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
			else if ((boolopt[i].addr) == &iflags.use_inverse) {
			    need_redraw = TRUE;
			}
			else if ((boolopt[i].addr) == &iflags.hilite_pet) {
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
	pline("out of menu map space");
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


#if defined(MICRO) || defined(MAC)
# define OPTIONS_HEADING "OPTIONS"
#else
# define OPTIONS_HEADING "NETHACKOPTIONS"
#endif

static char fmtstr_doset_add_menu[] = "%s%-15s [%s]   "; 

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
    Sprintf(buf, fmtstr_doset_add_menu, (any.a_int ? "" : "    "), option, value);
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
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
		 "Booleans (selecting will toggle value):", MENU_UNSELECTED);
	any.a_int = 0;
	/* first list any other non-modifiable booleans, then modifiable ones */
	for (pass = 0; pass <= 1; pass++)
	    for (i = 0; boolopt[i].name; i++)
		if ((bool_p = boolopt[i].addr) != 0 &&
			(boolopt_only_initial(i) ^ pass)) {
		    if (bool_p == &flags.female) continue;  /* obsolete */
#ifdef WIZARD
		    if (bool_p == &iflags.sanity_check && !wizard) continue;
#endif
		    any.a_int = (pass == 0) ? 0 : i + 1;
		    Sprintf(buf, "%s%-13s [%s]",
			    pass == 0 ? "    " : "",
			    boolopt[i].name, *bool_p ? "true" : "false");
		    add_menu(tmpwin, NO_GLYPH, &any, 0, 0,
			     ATR_NONE, buf, MENU_UNSELECTED);
		}

	boolcount = i;
	indexoffset = boolcount;
	any.a_void = 0;
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
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
		    	else
				doset_add_menu(tmpwin, compopt[i].name,
					(pass == DISP_IN_GAME) ? 0 : indexoffset);
		}
#ifdef PREFIXES_IN_USE
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
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
		} else {
		    /* compound option */
		    opt_indx -= boolcount;

		    if (!special_handling(compopt[opt_indx].name,
							setinitial, fromfile)) {
			Sprintf(buf, "Set %s to what?", compopt[opt_indx].name);
			getlin(buf, buf2);
			Sprintf(buf, "%s:%s", compopt[opt_indx].name, buf2);
			/* pass the buck */
			parseoptions(buf, setinitial, fromfile);
		    }
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
    
    /* Special handling of menustyle, pickup_burden, and pickup_types. */
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
		     to_be_done[] = "(to be done)";
#ifdef PREFIXES_IN_USE
	int i;
#endif

	buf[0] = '\0';
	if (!strcmp(optname,"align"))
		Sprintf(buf, "%s", rolestring(flags.initalign, aligns, adj));
	else if (!strcmp(optname, "boulder"))
		Sprintf(buf, "%c", iflags.bouldersym ?
			iflags.bouldersym : oc_syms[(int)objects[BOULDER].oc_class]);
	else if (!strcmp(optname, "catname")) 
		Sprintf(buf, "%s", catname[0] ? catname : none );
	else if (!strcmp(optname, "disclose")) 
		Sprintf(buf, "%s",
			flags.end_disclose[0] ? flags.end_disclose : "all" );
	else if (!strcmp(optname, "dogname")) 
		Sprintf(buf, "%s", dogname[0] ? dogname : none );
	else if (!strcmp(optname, "dungeon"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "effects"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "fruit")) 
		Sprintf(buf, "%s", pl_fruit);
	else if (!strcmp(optname, "gender"))
		Sprintf(buf, "%s", rolestring(flags.initgend, genders, adj));
	else if (!strcmp(optname, "horsename")) 
		Sprintf(buf, "%s", horsename[0] ? horsename : none);
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
	else if (!strcmp(optname, "scores")) {
		Sprintf(buf, "%d top/%d around%s", flags.end_top,
				flags.end_around, flags.end_own ? "/own" : "");
	     }
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
	} else if (!strcmp(optname, "traps"))
		Sprintf(buf, "%s", to_be_done);
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
#if 0
	else if (!strcmp(optname, "warnlevel"))
		Sprintf(buf, "%d", flags.warnlevel);
#endif
	else if (!strcmp(optname, "windowtype"))
		Sprintf(buf, "%s", windowprocs.name);
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

#endif	/* OPTION_LISTS_ONLY */

/*options.c*/
