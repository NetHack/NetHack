/* NetHack 3.6	flag.h	$NHDT-Date: 1435002669 2015/06/22 19:51:09 $  $NHDT-Branch: master $:$NHDT-Revision: 1.89 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* If you change the flag structure make sure you increment EDITLEVEL in   */
/* patchlevel.h if needed.  Changing the instance_flags structure does	   */
/* not require incrementing EDITLEVEL.					   */

#ifndef FLAG_H
#define FLAG_H

/*
 * Persistent flags that are saved and restored with the game.
 *
 */

struct flag {
    boolean acoustics;  /* allow dungeon sound messages */
    boolean autodig;    /* MRKR: Automatically dig */
    boolean autoquiver; /* Automatically fill quiver */
    boolean autoopen;   /* open doors by walking into them */
    boolean beginner;
    boolean biff;      /* enable checking for mail */
    boolean bones;     /* allow saving/loading bones */
    boolean confirm;   /* confirm before hitting tame monsters */
    boolean dark_room; /* show shadows in lit rooms */
    boolean debug;     /* in debugging mode */
#define wizard flags.debug
    boolean end_own; /* list all own scores */
    boolean explore; /* in exploration mode */
#define discover flags.explore
    boolean female;
    boolean friday13;        /* it's Friday the 13th */
    boolean help;            /* look in data file for info about stuff */
    boolean ignintr;         /* ignore interrupts */
    boolean ins_chkpt;       /* checkpoint as appropriate; INSURANCE */
    boolean invlet_constant; /* let objects keep their inventory symbol */
    boolean legacy;          /* print game entry "story" */
    boolean lit_corridor;    /* show a dark corr as lit if it is in sight */
    boolean nap;             /* `timed_delay' option for display effects */
    boolean null;            /* OK to send nulls to the terminal */
    boolean perm_invent;     /* keep full inventories up until dismissed */
    boolean pickup;          /* whether you pickup or move and look */
    boolean pickup_thrown;   /* auto-pickup items you threw */
    boolean pushweapon; /* When wielding, push old weapon into second slot */
    boolean rest_on_space;   /* space means rest */
    boolean safe_dog;        /* give complete protection to the dog */
    boolean showexp;         /* show experience points */
    boolean showscore;       /* show score */
    boolean silent;          /* whether the bell rings or not */
    boolean sortloot;        /* sort items alphabetically when looting */
    boolean sortpack;        /* sorted inventory */
    boolean sparkle;         /* show "resisting" special FX (Scott Bigham) */
    boolean standout;        /* use standout for --More-- */
    boolean time;            /* display elapsed 'time' */
    boolean tombstone;       /* print tombstone */
    boolean verbose;         /* max battle info */
    int end_top, end_around; /* describe desired score list */
    unsigned moonphase;
    unsigned long suppress_alert;
#define NEW_MOON 0
#define FULL_MOON 4
    int paranoia_bits; /* alternate confirmation prompting */
#define PARANOID_CONFIRM 0x01
#define PARANOID_QUIT 0x02
#define PARANOID_DIE 0x04
#define PARANOID_BONES 0x08
#define PARANOID_HIT 0x10
#define PARANOID_PRAY 0x20
#define PARANOID_REMOVE 0x40
#define PARANOID_BREAKWAND 0x80
    int pickup_burden; /* maximum burden before prompt */
    int pile_limit;    /* controls feedback when walking over objects */
    char inv_order[MAXOCLASSES];
    char pickup_types[MAXOCLASSES];
#define NUM_DISCLOSURE_OPTIONS 6 /* i,a,v,g,c,o (decl.c) */
#define DISCLOSE_PROMPT_DEFAULT_YES 'y'
#define DISCLOSE_PROMPT_DEFAULT_NO 'n'
#define DISCLOSE_YES_WITHOUT_PROMPT '+'
#define DISCLOSE_NO_WITHOUT_PROMPT '-'
    char end_disclose[NUM_DISCLOSURE_OPTIONS + 1]; /* disclose various
                                                      info upon exit */
    char menu_style;    /* User interface style setting */
    boolean made_fruit; /* don't easily let the user overflow the number of
                           fruits */

    /* KMH, role patch -- Variables used during startup.
     *
     * If the user wishes to select a role, race, gender, and/or alignment
     * during startup, the choices should be recorded here.  This
     * might be specified through command-line options, environmental
     * variables, a popup dialog box, menus, etc.
     *
     * These values are each an index into an array.  They are not
     * characters or letters, because that limits us to 26 roles.
     * They are not booleans, because someday someone may need a neuter
     * gender.  Negative values are used to indicate that the user
     * hasn't yet specified that particular value.	If you determine
     * that the user wants a random choice, then you should set an
     * appropriate random value; if you just left the negative value,
     * the user would be asked again!
     *
     * These variables are stored here because the u structure is
     * cleared during character initialization, and because the
     * flags structure is restored for saved games.  Thus, we can
     * use the same parameters to build the role entry for both
     * new and restored games.
     *
     * These variables should not be referred to after the character
     * is initialized or restored (specifically, after role_init()
     * is called).
     */
    int initrole;  /* starting role      (index into roles[])   */
    int initrace;  /* starting race      (index into races[])   */
    int initgend;  /* starting gender    (index into genders[]) */
    int initalign; /* starting alignment (index into aligns[])  */
    int randomall; /* randomly assign everything not specified */
    int pantheon;  /* deity selection for priest character */
    /* Items which were in iflags in 3.4.x to preserve savefile compatibility
     */
    boolean lootabc;   /* use "a/b/c" rather than "o/i/b" when looting */
    boolean showrace;  /* show hero glyph by race rather than by role */
    boolean travelcmd; /* allow travel command */
    int runmode;       /* update screen display during run moves */
};

/*
 * System-specific flags that are saved with the game if SYSFLAGS is defined.
 */

#if defined(AMIFLUSH) || defined(AMII_GRAPHICS) || defined(OPT_DISPMAP)
#define SYSFLAGS
#else
#if defined(MFLOPPY) || defined(MAC)
#define SYSFLAGS
#endif
#endif

#ifdef SYSFLAGS
struct sysflag {
    char sysflagsid[10];
#ifdef AMIFLUSH
    boolean altmeta;  /* use ALT keys as META */
    boolean amiflush; /* kill typeahead */
#endif
#ifdef AMII_GRAPHICS
    int numcols;
    unsigned short
        amii_dripens[20]; /* DrawInfo Pens currently there are 13 in v39 */
    AMII_COLOR_TYPE amii_curmap[AMII_MAXCOLORS]; /* colormap */
#endif
#ifdef OPT_DISPMAP
    boolean fast_map; /* use optimized, less flexible map display */
#endif
#ifdef MFLOPPY
    boolean asksavedisk;
#endif
#ifdef MAC
    boolean page_wait; /* put up a --More-- after a page of messages */
#endif
};
#endif

/*
 * Flags that are set each time the game is started.
 * These are not saved with the game.
 *
 */

struct instance_flags {
    /* stuff that really isn't option or platform related. They are
     * set and cleared during the game to control the internal
     * behaviour of various NetHack functions and probably warrant
     * a structure of their own elsewhere some day.
     */
    int in_lava_effects;   /* hack for Boots_off() */
    int last_msg;          /* indicator of last message player saw */
    int purge_monsters;    /* # of dead monsters still on fmon list */
    int override_ID;       /* true to force full identification of objects */
    int suppress_price;    /* controls doname() for unpaid objects */
    coord travelcc;        /* coordinates for travel_cache */
    boolean window_inited; /* true if init_nhwindows() completed */
    boolean vision_inited; /* true if vision is ready */
    boolean sanity_check;  /* run sanity checks */
    boolean mon_polycontrol; /* debug: control monster polymorphs */
    /* stuff that is related to options and/or user or platform preferences */
    unsigned msg_history; /* hint: # of top lines to save */
    int menu_headings;    /* ATR for menu headings */
    int *opt_booldup;     /* for duplication of boolean opts in config file */
    int *opt_compdup; /* for duplication of compound opts in config file */
#ifdef ALTMETA
    boolean altmeta; /* Alt-c sends ESC c rather than M-c */
#endif
    boolean cbreak;           /* in cbreak mode, rogue format */
    boolean deferred_X;       /* deferred entry into explore mode */
    boolean num_pad;          /* use numbers for movement commands */
    boolean news;             /* print news */
    boolean implicit_uncursed; /* maybe omit "uncursed" status in inventory */
    boolean mention_walls;    /* give feedback when bumping walls */
    boolean menu_tab_sep;     /* Use tabs to separate option menu fields */
    boolean menu_head_objsym; /* Show obj symbol in menu headings */
    boolean menu_requested;   /* Flag for overloaded use of 'm' prefix
                               * on some non-move commands */
    boolean renameallowed;    /* can change hero name during role selection */
    boolean renameinprogress; /* we are changing hero name */
    boolean toptenwin;        /* ending list in window instead of stdout */
    boolean zerocomp;         /* write zero-compressed save files */
    boolean rlecomp; /* run-length comp of levels when writing savefile */
    uchar num_pad_mode;
    boolean echo;             /* 1 to echo characters */
    boolean use_menu_color;       /* use color in menus; only if wc_color */
    boolean use_status_hilites;   /* use color in status line */
    boolean use_background_glyph; /* use background glyph when appropriate */
    boolean hilite_pile;          /* mark piles of objects with a hilite */
#if 0
	boolean  DECgraphics;	/* use DEC VT-xxx extended character set */
	boolean  IBMgraphics;	/* use IBM extended character set */
#ifdef MAC_GRAPHICS_ENV
	boolean  MACgraphics;	/* use Macintosh extended character set, as
				   as defined in the special font HackFont */
#endif
#endif
    uchar bouldersym; /* symbol for boulder display */
#ifdef TTY_GRAPHICS
    char prevmsg_window; /* type of old message window to use */
    boolean extmenu;     /* extended commands use menu interface */
#endif
#ifdef MFLOPPY
    boolean checkspace; /* check disk space before writing files */
                        /* (in iflags to allow restore after moving
                         * to >2GB partition) */
#endif
#ifdef MICRO
    boolean BIOS; /* use IBM or ST BIOS calls when appropriate */
#endif
#if defined(MICRO) || defined(WIN32)
    boolean rawio; /* whether can use rawio (IOCTL call) */
#endif
#ifdef MAC_GRAPHICS_ENV
    boolean MACgraphics; /* use Macintosh extended character set, as
                            as defined in the special font HackFont */
    unsigned use_stone;  /* use the stone ppats */
#endif
#if defined(MSDOS) || defined(WIN32)
    boolean hassound;     /* has a sound card */
    boolean usesound;     /* use the sound card */
    boolean usepcspeaker; /* use the pc speaker */
    boolean tile_view;
    boolean over_view;
    boolean traditional_view;
#endif
#ifdef MSDOS
    boolean hasvga; /* has a vga adapter */
    boolean usevga; /* use the vga adapter */
    boolean grmode; /* currently in graphics mode */
#endif
#ifdef LAN_FEATURES
    boolean lan_mail;         /* mail is initialized */
    boolean lan_mail_fetched; /* mail is awaiting display */
#endif
    /*
     * Window capability support.
     */
    boolean wc_color;         /* use color graphics                  */
    boolean wc_hilite_pet;    /* hilight pets                        */
    boolean wc_ascii_map;     /* show map using traditional ascii    */
    boolean wc_tiled_map;     /* show map using tiles                */
    boolean wc_preload_tiles; /* preload tiles into memory           */
    int wc_tile_width;        /* tile width                          */
    int wc_tile_height;       /* tile height                         */
    char *wc_tile_file;       /* name of tile file;overrides default */
    boolean wc_inverse;       /* use inverse video for some things   */
    int wc_align_status;      /*  status win at top|bot|right|left   */
    int wc_align_message;     /* message win at top|bot|right|left   */
    int wc_vary_msgcount;     /* show more old messages at a time    */
    char *wc_foregrnd_menu; /* points to foregrnd color name for menu win   */
    char *wc_backgrnd_menu; /* points to backgrnd color name for menu win   */
    char *wc_foregrnd_message; /* points to foregrnd color name for msg win */
    char *wc_backgrnd_message; /* points to backgrnd color name for msg win */
    char *
        wc_foregrnd_status; /* points to foregrnd color name for status win */
    char *
        wc_backgrnd_status; /* points to backgrnd color name for status win */
    char *wc_foregrnd_text; /* points to foregrnd color name for text win   */
    char *wc_backgrnd_text; /* points to backgrnd color name for text win   */
    char *wc_font_map;      /* points to font name for the map win */
    char *wc_font_message;  /* points to font name for message win */
    char *wc_font_status;   /* points to font name for status win  */
    char *wc_font_menu;     /* points to font name for menu win    */
    char *wc_font_text;     /* points to font name for text win    */
    int wc_fontsiz_map;     /* font size for the map win           */
    int wc_fontsiz_message; /* font size for the message window    */
    int wc_fontsiz_status;  /* font size for the status window     */
    int wc_fontsiz_menu;    /* font size for the menu window       */
    int wc_fontsiz_text;    /* font size for text windows          */
    int wc_scroll_amount;   /* scroll this amount at scroll_margin */
    int wc_scroll_margin;   /* scroll map when this far from
                                    the edge */
    int wc_map_mode;        /* specify map viewing options, mostly
                                    for backward compatibility */
    int wc_player_selection;    /* method of choosing character */
    boolean wc_splash_screen;   /* display an opening splash screen or not */
    boolean wc_popup_dialog;    /* put queries in pop up dialogs instead of
                                        in the message window */
    boolean wc_eight_bit_input; /* allow eight bit input               */
    boolean wc_mouse_support;   /* allow mouse support */
    boolean wc2_fullscreen;     /* run fullscreen */
    boolean wc2_softkeyboard;   /* use software keyboard */
    boolean wc2_wraptext;       /* wrap text */
    boolean wc2_selectsaved;    /* display a menu of user's saved games */
    boolean wc2_darkgray; /* try to use dark-gray color for black glyphs */
    boolean cmdassist;    /* provide detailed assistance for some commands */
    boolean clicklook;    /* allow right-clicking for look */
    boolean obsolete; /* obsolete options can point at this, it isn't used */
    struct autopickup_exception *autopickup_exceptions[2];
#define AP_LEAVE 0
#define AP_GRAB 1
#ifdef WIN32
#define MAX_ALTKEYHANDLER 25
    char altkeyhandler[MAX_ALTKEYHANDLER];
#endif
    /* copies of values in struct u, used during detection when the
       originals are temporarily cleared; kept here rather than
       locally so that they can be restored during a hangup save */
    Bitfield(save_uinwater, 1);
    Bitfield(save_uburied, 1);
};

/*
 * Old deprecated names
 */
#ifdef TTY_GRAPHICS
#define eight_bit_tty wc_eight_bit_input
#endif
#define use_color wc_color
#define hilite_pet wc_hilite_pet
#define use_inverse wc_inverse
#ifdef MAC_GRAPHICS_ENV
#define large_font obsolete
#endif
#ifdef MAC
#define popup_dialog wc_popup_dialog
#endif
#define preload_tiles wc_preload_tiles

extern NEARDATA struct flag flags;
#ifdef SYSFLAGS
extern NEARDATA struct sysflag sysflags;
#endif
extern NEARDATA struct instance_flags iflags;

/* last_msg values */
#define PLNMSG_UNKNOWN 0             /* arbitrary */
#define PLNMSG_ONE_ITEM_HERE 1       /* "you see <single item> here" */
#define PLNMSG_TOWER_OF_FLAME 2      /* scroll of fire */
#define PLNMSG_CAUGHT_IN_EXPLOSION 3 /* explode() feedback */
#define PLNMSG_OBJ_GLOWS 4           /* "the <obj> glows <color>" */

/* runmode options */
#define RUN_TPORT 0 /* don't update display until movement stops */
#define RUN_LEAP 1  /* update display every 7 steps */
#define RUN_STEP 2  /* update display every single step */
#define RUN_CRAWL 3 /* walk w/ extra delay after each update */

/* paranoid confirmation prompting */
/* any yes confirmations also require explicit no (or ESC) to reject */
#define ParanoidConfirm ((flags.paranoia_bits & PARANOID_CONFIRM) != 0)
/* quit: yes vs y for "Really quit?" and "Enter explore mode?" */
#define ParanoidQuit ((flags.paranoia_bits & PARANOID_QUIT) != 0)
/* die: yes vs y for "Die?" (dying in explore mode or wizard mode) */
#define ParanoidDie ((flags.paranoia_bits & PARANOID_DIE) != 0)
/* hit: yes vs y for "Save bones?" in wizard mode */
#define ParanoidBones ((flags.paranoia_bits & PARANOID_BONES) != 0)
/* hit: yes vs y for "Really attack <the peaceful monster>?" */
#define ParanoidHit ((flags.paranoia_bits & PARANOID_HIT) != 0)
/* pray: ask "Really pray?" (accepts y answer, doesn't require yes),
   taking over for the old prayconfirm boolean option */
#define ParanoidPray ((flags.paranoia_bits & PARANOID_PRAY) != 0)
/* remove: remove ('R') and takeoff ('T') commands prompt for an inventory
   item even when only one accessory or piece of armor is currently worn */
#define ParanoidRemove ((flags.paranoia_bits & PARANOID_REMOVE) != 0)
/* breakwand: Applying a wand */
#define ParanoidBreakwand ((flags.paranoia_bits & PARANOID_BREAKWAND) != 0)

/* command parsing, mainly dealing with number_pad handling;
   not saved and restored */

#ifdef NHSTDC
/* forward declaration sufficient to declare pointers */
struct func_tab; /* from func_tab.h */
#endif

/* commands[] is used to directly access cmdlist[] instead of looping
   through it to find the entry for a given input character;
   move_X is the character used for moving one step in direction X;
   alphadirchars corresponds to old sdir,
   dirchars corresponds to ``iflags.num_pad ? ndir : sdir'';
   pcHack_compat and phone_layout only matter when num_pad is on,
   swap_yz only matters when it's off */
struct cmd {
    unsigned serialno;     /* incremented after each update */
    boolean num_pad;       /* same as iflags.num_pad except during updates */
    boolean pcHack_compat; /* for numpad:  affects 5, M-5, and M-0 */
    boolean phone_layout;  /* inverted keypad:  1,2,3 above, 7,8,9 below */
    boolean swap_yz;       /* German keyboards; use z to move NW, y to zap */
    char move_W, move_NW, move_N, move_NE, move_E, move_SE, move_S, move_SW;
    const char *dirchars;      /* current movement/direction characters */
    const char *alphadirchars; /* same as dirchars if !numpad */
    const struct func_tab *commands[256]; /* indexed by input character */
};

extern NEARDATA struct cmd Cmd;

#endif /* FLAG_H */
