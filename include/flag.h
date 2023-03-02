/* NetHack 3.7	flag.h	$NHDT-Date: 1655161560 2022/06/13 23:06:00 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.201 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2006. */
/* NetHack may be freely redistributed.  See license for details. */

/* If you change the flag structure make sure you increment EDITLEVEL in   */
/* patchlevel.h if needed.  Changing the instance_flags structure does     */
/* not require incrementing EDITLEVEL.                                     */

#ifndef FLAG_H
#define FLAG_H

/*
 * Persistent flags that are saved and restored with the game.
 *
 */

struct flag {
    boolean acoustics;       /* allow dungeon sound messages */
    boolean autodig;         /* MRKR: Automatically dig */
    boolean autoquiver;      /* Automatically fill quiver */
    boolean autoopen;        /* open doors by walking into them */
    boolean beginner;        /* True early in each game; affects feedback */
    boolean biff;            /* enable checking for mail */
    boolean bones;           /* allow saving/loading bones */
    boolean confirm;         /* confirm before hitting tame monsters */
    boolean dark_room;       /* show shadows in lit rooms */
    boolean debug;           /* in debugging mode (aka wizard mode) */
#define wizard flags.debug
    boolean end_own;         /* list all own scores */
    boolean explore;         /* in exploration mode (aka discover mode) */
#define discover flags.explore
    boolean female;
    boolean friday13;        /* it's Friday the 13th */
    boolean goldX;           /* for BUCX filtering, whether gold is X or U */
    boolean help;            /* look in data file for info about stuff */
    boolean tips;            /* show helpful hints? */
    boolean tutorial;        /* ask if player wants tutorial level? */
    boolean ignintr;         /* ignore interrupts */
    boolean implicit_uncursed; /* maybe omit "uncursed" status in inventory */
    boolean ins_chkpt;       /* checkpoint as appropriate; INSURANCE */
    boolean invlet_constant; /* let objects keep their inventory symbol */
    boolean legacy;          /* print game entry "story" */
    boolean lit_corridor;    /* show a dark corr as lit if it is in sight */
    boolean mention_decor;   /* give feedback for unobscured furniture */
    boolean mention_walls;   /* give feedback when bumping walls */
    boolean nap;             /* `timed_delay' option for display effects */
    boolean null;            /* OK to send nulls to the terminal */
    boolean pickup;          /* whether you pickup or move and look */
    boolean pickup_thrown;   /* auto-pickup items you threw */
    boolean pushweapon; /* When wielding, push old weapon into second slot */
    boolean quick_farsight;  /* True disables map browsing during random
                              * clairvoyance */
    boolean rest_on_space;   /* space means rest */
    boolean safe_dog;        /* give complete protection to the dog */
    boolean safe_wait;       /* prevent wait or search next to hostile */
    boolean showexp;         /* show experience points */
    boolean showscore;       /* show score */
    boolean silent;          /* whether the bell rings or not */
    boolean sortpack;        /* sorted inventory */
    boolean sparkle;         /* show "resisting" special FX (Scott Bigham) */
    boolean standout;        /* use standout for --More-- */
    boolean time;            /* display elapsed 'time' */
    boolean tombstone;       /* print tombstone */
    boolean verbose;         /* max battle info */
    int end_top, end_around; /* describe desired score list */
    unsigned autounlock;     /* locked door/chest action */
#define AUTOUNLOCK_UNTRAP    1
#define AUTOUNLOCK_APPLY_KEY 2
#define AUTOUNLOCK_KICK      4
#define AUTOUNLOCK_FORCE     8
    unsigned moonphase;
    unsigned long suppress_alert;
#define NEW_MOON 0
#define FULL_MOON 4
    unsigned paranoia_bits; /* alternate confirmation prompting */
#define PARANOID_CONFIRM    0x0001
#define PARANOID_QUIT       0x0002
#define PARANOID_DIE        0x0004
#define PARANOID_BONES      0x0008
#define PARANOID_HIT        0x0010
#define PARANOID_PRAY       0x0020
#define PARANOID_REMOVE     0x0040
#define PARANOID_BREAKWAND  0x0080
#define PARANOID_WERECHANGE 0x0100
#define PARANOID_EATING     0x0200
#define PARANOID_SWIM       0x0400
    int pickup_burden; /* maximum burden before prompt */
    int pile_limit;    /* controls feedback when walking over objects */
    char discosort;    /* order of dodiscovery/doclassdisco output: o,s,c,a */
    char sortloot; /* 'n'=none, 'l'=loot (pickup), 'f'=full ('l'+invent) */
    uchar vanq_sortmode; /* [uint_8] order of vanquished monsters: 0..7 */
    char inv_order[MAXOCLASSES];
    char pickup_types[MAXOCLASSES];
#define NUM_DISCLOSURE_OPTIONS 6 /* i,a,v,g,c,o (decl.c) */
#define DISCLOSE_PROMPT_DEFAULT_YES 'y'
#define DISCLOSE_PROMPT_DEFAULT_NO 'n'
#define DISCLOSE_PROMPT_DEFAULT_SPECIAL '?' /* v, default a */
#define DISCLOSE_YES_WITHOUT_PROMPT '+'
#define DISCLOSE_NO_WITHOUT_PROMPT '-'
#define DISCLOSE_SPECIAL_WITHOUT_PROMPT '#' /* v, use a */
    char end_disclose[NUM_DISCLOSURE_OPTIONS + 1]; /* disclose various
                                                      info upon exit */
    char menu_style;    /* User interface style setting */
    boolean made_fruit; /* don't easily let user overflow fruit limit */

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
     * hasn't yet specified that particular value.  If you determine
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
 * Flags that are set each time the game is started.
 * These are not saved with the game.
 *
 */

/* values for iflags.getpos_coords */
#define GPCOORDS_NONE    'n'
#define GPCOORDS_MAP     'm'
#define GPCOORDS_COMPASS 'c'
#define GPCOORDS_COMFULL 'f'
#define GPCOORDS_SCREEN  's'

enum getloc_filters {
    GFILTER_NONE = 0,
    GFILTER_VIEW,
    GFILTER_AREA,

    NUM_GFILTER
};

#ifdef WIN32
enum windows_key_handling {
    no_keyhandling,
    default_keyhandling,
    ray_keyhandling,
    nh340_keyhandling
};
#endif

struct debug_flags {
    boolean test;
#ifdef TTY_GRAPHICS
    boolean ttystatus;
#endif
#ifdef WIN32
    boolean immediateflips;
#endif
};

/*
 * Stuff that really isn't option or platform related and does not
 * get saved and restored.  They are set and cleared during the game
 * to control the internal behavior of various NetHack functions
 * and probably warrant a structure of their own elsewhere some day.
 */
struct instance_flags {
    boolean debug_fuzzer;  /* fuzz testing */
    boolean defer_plname;  /* X11 hack: askname() might not set gp.plname */
    boolean herecmd_menu;  /* use menu when mouseclick on yourself */
    boolean invis_goldsym; /* gold symbol is ' '? */
    boolean in_lua;        /* executing a lua script */
    boolean lua_testing;   /* doing lua tests */
    boolean partly_eaten_hack; /* extra flag for xname() used when it's called
                                * indirectly so we can't use xname_flags() */
    boolean remember_getpos; /* save getpos() positioning in do-again queue */
    boolean sad_feeling;   /* unseen pet is dying */
    int at_midnight;       /* only valid during end of game disclosure */
    int at_night;          /* also only valid during end of game disclosure */
    int failing_untrap;    /* move_into_trap() -> spoteffects() -> dotrap() */
    int in_lava_effects;   /* hack for Boots_off() */
    int last_msg;          /* indicator of last message player saw */
    int override_ID;       /* true to force full identification of objects */
    int parse_config_file_src;  /* hack for parse_config_line() */
    int purge_monsters;    /* # of dead monsters still on fmon list */
    int suppress_price;    /* controls doname() for unpaid objects */
    int terrainmode; /* for getpos()'s autodescribe when #terrain is active */
#define TER_MAP    0x01
#define TER_TRP    0x02
#define TER_OBJ    0x04
#define TER_MON    0x08
#define TER_DETECT 0x10    /* detect_foo magic rather than #terrain */
    int getdir_click;      /* as input to getdir(): non-zero, accept simulated
                            * click that's not adjacent to or on hero;
                            * as output from getdir(): simulated button used
                            * 0 (none) or CLICK_1 (left) or CLICK_2 (right) */
    int getloc_filter;     /* GFILTER_foo */
    boolean bgcolors;      /* display background colors on a map position */
    boolean getloc_moveskip;
    boolean getloc_travelmode;
    boolean getloc_usemenu;
    coord travelcc;        /* coordinates for travel_cache */
    boolean trav_debug;    /* display travel path (#if DEBUG only) */
    boolean window_inited; /* true if init_nhwindows() completed */
    boolean vision_inited; /* true if vision is ready */
    boolean sanity_check;  /* run sanity checks */
    boolean debug_overwrite_stairs; /* debug: allow overwriting stairs */
    boolean debug_mongen;  /* debug: prevent monster generation */
    boolean debug_hunger;  /* debug: prevent hunger */
    boolean mon_polycontrol; /* debug: control monster polymorphs */
    boolean in_dumplog;    /* doing the dumplog right now? */
    boolean in_parse;      /* is a command being parsed? */
     /* suppress terminate during options parsing, for --showpaths */
    boolean initoptions_noterminate;

    /* stuff that is related to options and/or user or platform preferences
     */
    unsigned msg_history; /* hint: # of top lines to save */
    int getpos_coords;    /* show coordinates when getting cursor position */
    int menuinvertmode;  /* 0 = invert toggles every item;
                            1 = invert skips 'all items' item */
    int menu_headings;    /* ATR for menu headings */
    uint32_t colorcount;    /* store how many colors terminal is capable of */
    boolean use_truecolor;  /* force use of truecolor */
#ifdef ALTMETA
    boolean altmeta;      /* Alt-c sends ESC c rather than M-c */
#endif
    boolean autodescribe;     /* autodescribe mode in getpos() */
    boolean cbreak;           /* in cbreak mode, rogue format */
    boolean deferred_X;       /* deferred entry into explore mode */
    boolean defer_decor;      /* terrain change message vs slipping on ice */
    boolean echo;             /* 1 to echo characters */
    boolean force_invmenu;    /* always menu when handling inventory */
    boolean hilite_pile;      /* mark piles of objects with a hilite */
    boolean menu_head_objsym; /* Show obj symbol in menu headings */
    boolean menu_overlay;     /* Draw menus over the map */
    boolean menu_requested;   /* Flag for overloaded use of 'm' prefix
                               * on some non-move commands */
    boolean menu_tab_sep;     /* Use tabs to separate option menu fields */
    boolean news;             /* print news */
    boolean num_pad;          /* use numbers for movement commands */
    boolean perm_invent;      /* keep full inventories up until dismissed */
    boolean renameallowed;    /* can change hero name during role selection */
    boolean renameinprogress; /* we are changing hero name */
    boolean sounds;           /* master on/off switch for using soundlib */
    boolean status_updates;   /* allow updates to bottom status lines;
                               * disable to avoid excessive noise when using
                               * a screen reader (use ^X to review status) */
    boolean toptenwin;        /* ending list in window instead of stdout */
    boolean tux_penalty;      /* True iff hero is a monk and wearing a suit */
    boolean use_background_glyph; /* use background glyph when appropriate */
    boolean use_menu_color;   /* use color in menus; only if wc_color */
#ifdef STATUS_HILITES
    long hilite_delta;        /* number of moves to leave a temp hilite lit */
    long unhilite_deadline; /* time when oldest temp hilite should be unlit */
#endif
    boolean voices;           /* enable text-to-speech or other talking */
    boolean zerocomp;         /* write zero-compressed save files */
    boolean rlecomp;          /* alternative to zerocomp; run-length encoding
                               * compression of levels when writing savefile */
    schar prev_decor;         /* 'mention_decor' just mentioned this */
    uchar num_pad_mode;
    uchar bouldersym;         /* symbol for boulder display */
    char prevmsg_window;      /* type of old message window to use */
    boolean extmenu;          /* extended commands use menu interface */
#ifdef MICRO
    boolean BIOS; /* use IBM or ST BIOS calls when appropriate */
#endif
#if defined(MICRO) || defined(WIN32)
    boolean rawio; /* whether can use rawio (IOCTL call) */
#endif
#if defined(MSDOS) || defined(WIN32)
    boolean tile_view;
    boolean over_view;
    boolean traditional_view;
#endif
#ifdef MSDOS
    boolean hasvga; /* has a vga adapter */
    boolean usevga; /* use the vga adapter */
    boolean hasvesa; /* has a VESA-capable VGA adapter */
    boolean usevesa; /* use the VESA-capable VGA adapter */
    boolean grmode; /* currently in graphics mode */
#endif
#ifdef LAN_FEATURES
    boolean lan_mail;         /* mail is initialized */
    boolean lan_mail_fetched; /* mail is awaiting display */
#endif
#ifdef TTY_TILES_ESCCODES
    boolean vt_tiledata;     /* output console codes for tile support in TTY */
#endif
#ifdef TTY_SOUND_ESCCODES
    boolean vt_sounddata;    /* output console codes for sound support in TTY*/
#endif
    boolean cmdassist;       /* provide detailed assistance for some comnds */
    boolean fireassist;      /* autowield launcher when using fire-command */
    boolean time_botl;       /* context.botl for 'time' (moves) only */
    boolean wizweight;       /* display weight of everything in wizard mode */
    boolean wizmgender;      /* test gender info from core in window port */
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
    char *wc_foregrnd_status; /* points to foregrnd color name for status   */
    char *wc_backgrnd_status; /* points to backgrnd color name for status   */
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
    int wc_scroll_margin;   /* scroll map when this far from the edge */
    int wc_map_mode;        /* specify map viewing options, mostly
                             * for backward compatibility */
    int wc_player_selection;    /* method of choosing character */
#if defined(MSDOS)
    unsigned wc_video_width;    /* X resolution of screen */
    unsigned wc_video_height;   /* Y resolution of screen */
#endif
    boolean wc_splash_screen;   /* display an opening splash screen or not */
    boolean wc_popup_dialog;    /* put queries in pop up dialogs instead of
                                 * in the message window */
    boolean wc_eight_bit_input; /* allow eight bit input               */
    boolean wc2_fullscreen;     /* run fullscreen */
    boolean wc2_softkeyboard;   /* use software keyboard */
    boolean wc2_wraptext;       /* wrap text */
    boolean wc2_selectsaved;    /* display a menu of user's saved games */
    boolean wc2_darkgray;    /* try to use dark-gray color for black glyphs */
    boolean wc2_hitpointbar;  /* show graphical bar representing hit points */
    boolean wc2_guicolor;       /* allow colours in gui (outside map) */
    int wc_mouse_support;       /* allow mouse support */
    int wc2_term_cols;          /* terminal width, in characters */
    int wc2_term_rows;          /* terminal height, in characters */
    int wc2_statuslines;        /* default = 2, curses can handle 3 */
    int wc2_windowborders;      /* display borders on NetHack windows */
    int wc2_petattr;            /* text attributes for pet */
#ifdef WIN32
#define MAX_ALTKEYHANDLING 25
    char altkeyhandling[MAX_ALTKEYHANDLING];
    enum windows_key_handling key_handling;
#endif
    /* copies of values in struct u, used during detection when the
       originals are temporarily cleared; kept here rather than
       locally so that they can be restored during a hangup save */
    Bitfield(save_uswallow, 1);
    Bitfield(save_uinwater, 1);
    Bitfield(save_uburied, 1);
    struct debug_flags debug;
    boolean windowtype_locked;   /* windowtype can't change from configfile */
    boolean windowtype_deferred; /* pick a windowport and store it in
                                    chosen_windowport[], but do not switch to
                                    it in the midst of options processing */
    genericptr_t returning_missile; /* 'struct obj *'; Mjollnir or aklys */
    boolean obsolete;  /* obsolete options can point at this, it isn't used */
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
extern NEARDATA struct instance_flags iflags;

/* last_msg values
 * Usage:
 *  pline("some message");
 *    pline: vsprintf + putstr + iflags.last_msg = PLNMSG_UNKNOWN;
 *  iflags.last_msg = PLNMSG_some_message;
 * and subsequent code can adjust the next message if it is affected
 * by some_message.  The next message will clear iflags.last_msg.
 */
enum plnmsg_types {
    PLNMSG_UNKNOWN = 0,         /* arbitrary */
    PLNMSG_ONE_ITEM_HERE,       /* "you see <single item> here" */
    PLNMSG_TOWER_OF_FLAME,      /* scroll of fire */
    PLNMSG_CAUGHT_IN_EXPLOSION, /* explode() feedback */
    PLNMSG_OBJ_GLOWS,           /* "the <obj> glows <color>" */
    PLNMSG_OBJNAM_ONLY,         /* xname/doname only, for #tip */
    PLNMSG_OK_DONT_DIE,         /* overriding death in explore/wizard mode */
    PLNMSG_BACK_ON_GROUND,      /* leaving water */
    PLNMSG_GROWL,               /* growl() gave some message */
    PLNMSG_enum /* allows inserting new entries with unconditional trailing comma */
};

/* runmode options */
enum runmode_types {
    RUN_TPORT = 0, /* don't update display until movement stops */
    RUN_LEAP,      /* update display every 7 steps */
    RUN_STEP,      /* update display every single step */
    RUN_CRAWL      /* walk w/ extra delay after each update */
};

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
/* werechange: accepting randomly timed werecreature change to transform
   from human to creature or vice versa while having polymorph control */
#define ParanoidWerechange ((flags.paranoia_bits & PARANOID_WERECHANGE) != 0)
/* continue eating: prompt given _after_first_bite_ when eating something
   while satiated */
#define ParanoidEating ((flags.paranoia_bits & PARANOID_EATING) != 0)
/* Prevent going into lava or water without explicitly forcing it */
#define ParanoidSwim ((flags.paranoia_bits & PARANOID_SWIM) != 0)

/* command parsing, mainly dealing with number_pad handling;
   not saved and restored */

#ifdef NHSTDC
/* forward declaration sufficient to declare pointers */
struct ext_func_tab; /* from func_tab.h */
#endif

enum gloctypes {
    GLOC_MONS = 0,
    GLOC_OBJS,
    GLOC_DOOR,
    GLOC_EXPLORE,
    GLOC_INTERESTING,
    GLOC_VALID,

    NUM_GLOCS
};

#endif /* FLAG_H */
