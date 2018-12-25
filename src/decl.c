/* NetHack 3.6	decl.c	$NHDT-Date: 1446975463 2015/11/08 09:37:43 $  $NHDT-Branch: master $:$NHDT-Revision: 1.62 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2009. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* from xxxmain.c */
#if defined(UNIX) || defined(VMS)
int locknum = 0; /* max num of simultaneous users */
#endif
#ifdef DEF_PAGER
char *catmore = 0; /* default pager */
#endif

/*
 *      The following structure will be initialized at startup time with
 *      the level numbers of some "important" things in the game.
 */
struct dgn_topology dungeon_topology = { DUMMY };

NEARDATA struct kinfo killer = DUMMY;

const char quitchars[] = " \r\n\033";
const char vowels[] = "aeiouAEIOU";
const char ynchars[] = "yn";
const char ynqchars[] = "ynq";
const char ynaqchars[] = "ynaq";
const char ynNaqchars[] = "yn#aq";
NEARDATA long yn_number = 0L;

const char disclosure_options[] = "iavgco";

#if defined(MICRO) || defined(WIN32)
char hackdir[PATHLEN]; /* where rumors, help, record are */
#ifdef MICRO
char levels[PATHLEN]; /* where levels are */
#endif
#endif /* MICRO || WIN32 */

#ifdef MFLOPPY
char permbones[PATHLEN]; /* where permanent copy of bones go */
int ramdisk = FALSE;     /* whether to copy bones to levels or not */
int saveprompt = TRUE;
const char *alllevels = "levels.*";
const char *allbones = "bones*.*";
#endif

NEARDATA struct sinfo program_state;

/* x/y/z deltas for the 10 movement directions (8 compass pts, 2 up/down) */
const schar xdir[10] = { -1, -1, 0, 1, 1, 1, 0, -1, 0, 0 };
const schar ydir[10] = { 0, -1, -1, -1, 0, 1, 1, 1, 0, 0 };
const schar zdir[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, -1 };

NEARDATA struct mkroom rooms[(MAXNROFROOMS + 1) * 2] = { DUMMY };
NEARDATA struct mkroom *subrooms = &rooms[MAXNROFROOMS + 1];

dlevel_t level; /* level map */
NEARDATA struct monst youmonst = DUMMY;
NEARDATA struct context_info context = DUMMY;
NEARDATA struct flag flags = DUMMY;
#ifdef SYSFLAGS
NEARDATA struct sysflag sysflags = DUMMY;
#endif
NEARDATA struct instance_flags iflags = DUMMY;
NEARDATA struct you u = DUMMY;
NEARDATA time_t ubirthday = DUMMY;
NEARDATA struct u_realtime urealtime = DUMMY;

NEARDATA struct obj
    *invent = (struct obj *) 0,
    *uwep = (struct obj *) 0, *uarm = (struct obj *) 0,
    *uswapwep = (struct obj *) 0,
    *uquiver = (struct obj *) 0,       /* quiver */
        *uarmu = (struct obj *) 0,     /* under-wear, so to speak */
            *uskin = (struct obj *) 0, /* dragon armor, if a dragon */
                *uarmc = (struct obj *) 0, *uarmh = (struct obj *) 0,
    *uarms = (struct obj *) 0, *uarmg = (struct obj *) 0,
    *uarmf = (struct obj *) 0, *uamul = (struct obj *) 0,
    *uright = (struct obj *) 0, *uleft = (struct obj *) 0,
    *ublindf = (struct obj *) 0, *uchain = (struct obj *) 0,
    *uball = (struct obj *) 0;

#ifdef TEXTCOLOR
/*
 *  This must be the same order as used for buzz() in zap.c.
 */
const int zapcolors[NUM_ZAP] = {
    HI_ZAP,     /* 0 - missile */
    CLR_ORANGE, /* 1 - fire */
    CLR_WHITE,  /* 2 - frost */
    HI_ZAP,     /* 3 - sleep */
    CLR_BLACK,  /* 4 - death */
    CLR_WHITE,  /* 5 - lightning */
    CLR_YELLOW, /* 6 - poison gas */
    CLR_GREEN,  /* 7 - acid */
};
#endif /* text color */

const int shield_static[SHIELD_COUNT] = {
    S_ss1, S_ss2, S_ss3, S_ss2, S_ss1, S_ss2, S_ss4, /* 7 per row */
    S_ss1, S_ss2, S_ss3, S_ss2, S_ss1, S_ss2, S_ss4,
    S_ss1, S_ss2, S_ss3, S_ss2, S_ss1, S_ss2, S_ss4,
};

NEARDATA long moves = 1L, monstermoves = 1L;
/* These diverge when player is Fast */
NEARDATA long wailmsg = 0L;

/* objects that are moving to another dungeon level */
NEARDATA struct obj *migrating_objs = (struct obj *) 0;
/* objects not yet paid for */
NEARDATA struct obj *billobjs = (struct obj *) 0;

/* used to zero all elements of a struct obj and a struct monst */
NEARDATA struct obj zeroobj = DUMMY;
NEARDATA struct monst zeromonst = DUMMY;
/* used to zero out union any; initializer deliberately omitted */
NEARDATA anything zeroany;

NEARDATA struct c_color_names c_color_names = {
    "black",  "amber", "golden", "light blue", "red",   "green",
    "silver", "blue",  "purple", "white",      "orange"
};

const char *c_obj_colors[] = {
    "black",          /* CLR_BLACK */
    "red",            /* CLR_RED */
    "green",          /* CLR_GREEN */
    "brown",          /* CLR_BROWN */
    "blue",           /* CLR_BLUE */
    "magenta",        /* CLR_MAGENTA */
    "cyan",           /* CLR_CYAN */
    "gray",           /* CLR_GRAY */
    "transparent",    /* no_color */
    "orange",         /* CLR_ORANGE */
    "bright green",   /* CLR_BRIGHT_GREEN */
    "yellow",         /* CLR_YELLOW */
    "bright blue",    /* CLR_BRIGHT_BLUE */
    "bright magenta", /* CLR_BRIGHT_MAGENTA */
    "bright cyan",    /* CLR_BRIGHT_CYAN */
    "white",          /* CLR_WHITE */
};

struct c_common_strings c_common_strings = { "Nothing happens.",
                                             "That's enough tries!",
                                             "That is a silly thing to %s.",
                                             "shudder for a moment.",
                                             "something",
                                             "Something",
                                             "You can move again.",
                                             "Never mind.",
                                             "vision quickly clears.",
                                             { "the", "your" } };

/* NOTE: the order of these words exactly corresponds to the
   order of oc_material values #define'd in objclass.h. */
const char *materialnm[] = { "mysterious", "liquid",  "wax",        "organic",
                             "flesh",      "paper",   "cloth",      "leather",
                             "wooden",     "bone",    "dragonhide", "iron",
                             "metal",      "copper",  "silver",     "gold",
                             "platinum",   "mithril", "plastic",    "glass",
                             "gemstone",   "stone" };

/* Global windowing data, defined here for multi-window-system support */
NEARDATA winid WIN_MESSAGE = WIN_ERR;
NEARDATA winid WIN_STATUS = WIN_ERR;
NEARDATA winid WIN_MAP = WIN_ERR, WIN_INVEN = WIN_ERR;

/* Windowing stuff that's really tty oriented, but present for all ports */
struct tc_gbl_data tc_gbl_data = { 0, 0, 0, 0 }; /* AS,AE, LI,CO */

char *fqn_prefix[PREFIX_COUNT] = { (char *) 0, (char *) 0, (char *) 0,
                                   (char *) 0, (char *) 0, (char *) 0,
                                   (char *) 0, (char *) 0, (char *) 0,
                                   (char *) 0 };

#ifdef PREFIXES_IN_USE
char *fqn_prefix_names[PREFIX_COUNT] = {
    "hackdir",  "leveldir", "savedir",    "bonesdir",  "datadir",
    "scoredir", "lockdir",  "sysconfdir", "configdir", "troubledir"
};
#endif

const struct savefile_info default_sfinfo = {
#ifdef NHSTDC
    0x00000000UL
#else
    0x00000000L
#endif
#if defined(COMPRESS) || defined(ZLIB_COMP)
        | SFI1_EXTERNALCOMP
#endif
#if defined(ZEROCOMP)
        | SFI1_ZEROCOMP
#endif
#if defined(RLECOMP)
        | SFI1_RLECOMP
#endif
    ,
#ifdef NHSTDC
    0x00000000UL, 0x00000000UL
#else
    0x00000000L, 0x00000000L
#endif
};

NEARDATA struct savefile_info sfcap, sfrestinfo, sfsaveinfo;

#ifdef PANICTRACE
const char *ARGV0;
#endif

/* support for lint.h */
unsigned nhUse_dummy = 0;

#define IVMAGIC 0xdeadbeef

#ifdef GCC_WARN
#pragma GCC diagnostic ignored "-Wmissing-braces"
#endif

const struct instance_globals g_init = {
    /* apply.c */
    0,  /* jumping_is_magic */
    -1, /* polearm_range_min */
    -1, /* polearm_range_max  */
    UNDEFINED_VALUES, /* trapinfo */

    /* artifact.c */
    0,  /* spec_dbon_applies */
    UNDEFINED_VALUES, /* artiexist */
    UNDEFINED_VALUES, /* artdisco */
    0, /* mkot_trap_warn_count */

    /* botl.c */
    0,  /* mrank_sz */
    UNDEFINED_VALUES, /* blstats */
    FALSE, /* blinit */
    FALSE, /* update_all */
    UNDEFINED_VALUES, /* valset */
    0, /* bl_hilite_moves */
    UNDEFINED_VALUES, /* cond_hilites */

    /* cmd.c */
    UNDEFINED_VALUES, /* Cmd */
    UNDEFINED_VALUES, /* pushq */
    UNDEFINED_VALUES, /* saveq */
    UNDEFINED_VALUE, /* phead */
    UNDEFINED_VALUE, /* ptail */
    UNDEFINED_VALUE, /* shead */
    UNDEFINED_VALUE, /* stail */
    UNDEFINED_VALUES, /* clicklook_cc */
    WIN_ERR, /* en_win */
    FALSE, /* en_via_menu */
    UNDEFINED_VALUE, /* last_multi */

    /* dbridge.c */
    UNDEFINED_VALUES, /* occupants */

    /* decl.c */
    UNDEFINED_PTR, /* occupation */
    UNDEFINED_PTR, /* afternmv */
    UNDEFINED_PTR, /* hname */
    0, /* hackpid */
    UNDEFINED_VALUES, /* chosen_windowtype */
    DUMMY, /* bases */
    0, /* multi */
    NULL, /* g.multi_reason */
    0, /* nroom */
    0, /* nsubroom */
    0, /* occtime */
    0, /* warn_obj_cnt */
    /* maze limits must be even; masking off lowest bit guarantees that */
    (COLNO - 1) & ~1, /* x_maze_max */
    (ROWNO - 1) & ~1, /* y_maze_max */
    UNDEFINED_VALUE, /* otg_temp */
    0, /* in_doagain */
    DUMMY, /* dnstair */
    DUMMY, /* upstair */
    DUMMY, /* dnladder */
    DUMMY, /* upladder */
    DUMMY, /* smeq */
    0, /* doorindex */
    NULL, /* save_cm */
    0, /* done_money */
    NULL, /* nomovemsg */
    DUMMY, /* plname */
    DUMMY, /* pl_character */
    '\0', /* pl_race */
    DUMMY, /* pl_fruit */
    NULL, /* ffruit */
    DUMMY, /* tune */
    NULL, /* occtxt */
    0, /* tbx */
    0, /* tby */
    UNDEFINED_PTR, /* sp_levchn */
    { 0, 0, STRANGE_OBJECT, FALSE }, /* m_shot */
    UNDEFINED_VALUES, /* dungeons */
    { 0, 0, { 0, 0 }, 0 }, /* sstairs */
    { 0, 0, 0, 0, 0, 0, 0, 0 }, /* updest */
    { 0, 0, 0, 0, 0, 0, 0, 0 }, /* dndest */
    { 0, 0} , /* inv_pos */
    FALSE, /* defer_see_monsters */
    FALSE, /* in_mklev */
    FALSE, /* stoned */
    FALSE, /* unweapon */
    FALSE, /* mrg_to_wielded */
    NULL, /* plinemsg_types */
    UNDEFINED_VALUES, /* toplines */
    UNDEFINED_PTR, /* upstairs_room */
    UNDEFINED_PTR, /* dnstairs_room */
    UNDEFINED_PTR, /* sstairs_room */
    DUMMY, /* bhitpos */
    FALSE, /* in_steed_dismounting */
    DUMMY, /* doors */
    NULL, /* menu_colorings */
    DUMMY, /* lastseentyp */
    DUMMY, /* spl_book */
    UNDEFINED_VALUES, /* level_info */
    NULL, /* ftrap */
    NULL, /* current_wand */
    NULL, /* thrownobj */
    NULL, /* kickedobj */

    /* dig.c */
    UNDEFINED_VALUE, /* did_dig_msg */

    /* display.c */
    UNDEFINED_VALUES, /* gbuf */
    UNDEFINED_VALUES, /* gbuf_start */
    UNDEFINED_VALUES, /* gbug_stop */

    /* do.c */
    FALSE, /* at_ladder */
    NULL, /* dfr_pre_msg */
    NULL, /* dfr_post_msg */
    { 0, 0 }, /* save_dlevel */

    /* do_name.c */
    NULL, /* gloc_filter_map */
    UNDEFINED_VALUE, /* gloc_filter_floodfill_match_glyph */
    0, /* via_naming */

    /* do_wear.c */
    FALSE, /* initial_don */

    /* dog.c */
    0,  /* petname_used */
    UNDEFINED_VALUE, /* gtyp */
    UNDEFINED_VALUE, /* gx */
    UNDEFINED_VALUE, /* gy */
    DUMMY, /* dogname */
    DUMMY, /* catname */
    DUMMY, /* horsename */
    UNDEFINED_VALUE, /* preferred_pet */
    NULL, /* mydogs */
    NULL, /* migrating_mons */
    UNDEFINED_VALUES, /* mvitals */

    /* dokick.c */
    UNDEFINED_PTR, /* maploc */
    UNDEFINED_VALUES, /* nowhere */
    UNDEFINED_PTR, /* gate_str */

    /* drawing.c */
    DUMMY, /* symset */
    0, /* currentgraphics */
    DUMMY, /* showsyms */
    DUMMY, /* l_syms */
    DUMMY, /* r_syms */
    DUMMY, /* warnsyms */

    /* dungeon.c */
    UNDEFINED_VALUE, /* n_dgns */
    NULL, /* branches */
    NULL, /* mapseenchn */

    /* eat.c */
    FALSE, /* force_save_hs */
    NULL, /* eatmbuf */

    /* end.c */
    UNDEFINED_VALUES,
    UNDEFINED_VALUES,
    UNDEFINED_VALUES,
    VANQ_MLVL_MNDX,

    /* extralev.c */
    UNDEFINED_VALUES,

    /* files.c */
    UNDEFINED_VALUES, /* wizkit */
    UNDEFINED_VALUE, /* lockptr */
    NULL, /* config_section_chosen */
    NULL, /* config_section_current */
    0, /* nesting */
    0, /* symset_count */
    FALSE, /* chosen_symset_start */
    FALSE, /* chosen_symset_end */
    0, /* symset_which_set */

    /* hack.c */
    UNDEFINED_VALUES,
    UNDEFINED_VALUE,

    /* invent.c */
    51, /* lastinvr */
    0, /* sortloogmode */
    NULL, /* invbuf */
    0, /* inbufsize */
    WIN_ERR, /* cached_pickinv_win */
    UNDEFINED_VALUE,
    UNDEFINED_VALUES,

    /* light.c */
    NULL, /* light_source */

    /* lock.c */
    UNDEFINED_VALUES,

    /* makemon.c */
    { -1, /* choice_count */
     { 0 } }, /* mchoices */

    /* mhitm.c */
    UNDEFINED_VALUE, /* vis */
    UNDEFINED_VALUE, /* far_noise */
    UNDEFINED_VALUE, /* noisetime */
    UNDEFINED_PTR, /* otmp */
    UNDEFINED_VALUE, /* dieroll */

    /* mhitu.c */
    UNDEFINED_VALUE, /* mhitu_dieroll */

    /* mklev.c */
    UNDEFINED_VALUE, /* vault_x */
    UNDEFINED_VALUE, /* vault_y */
    UNDEFINED_VALUE, /* made_branch */

    /* mkmap.c */
    UNDEFINED_PTR, /* new_locations */
    UNDEFINED_VALUE, /* min_rx */
    UNDEFINED_VALUE, /* max_rx */
    UNDEFINED_VALUE, /* min_ry */
    UNDEFINED_VALUE, /* max_ry */
    UNDEFINED_VALUE, /* n_loc_filled */

    /* mkmaze.c */
    { {COLNO, ROWNO, 0, 0}, {COLNO, ROWNO, 0, 0} }, /* bughack */
    UNDEFINED_VALUE, /* was_waterlevel */
    UNDEFINED_PTR, /* bbubbles */
    UNDEFINED_PTR, /* ebubbles */
    UNDEFINED_PTR, /* wportal */
    UNDEFINED_VALUE, /* xmin */
    UNDEFINED_VALUE, /* ymin */
    UNDEFINED_VALUE, /* xmax */
    UNDEFINED_VALUE, /* ymax */
    0, /* ransacked */

    /* mon.c */
    UNDEFINED_VALUE, /* vamp_rise_msg */
    UNDEFINED_VALUE, /* disintegested */
    NULL, /* animal_list */
    UNDEFINED_VALUE, /* animal_list_count */

    /* muse.c */
    FALSE, /* m_using */
    UNDEFINED_VALUE, /* trapx */
    UNDEFINED_VALUE, /* trapy */
    UNDEFINED_VALUE, /* zap_oseen */
    UNDEFINED_VALUES, /* m */

    /* o_init.c */
    DUMMY, /* disco */

    /* objname.c */
    0, /* distantname */

    /* options.c */
    NULL, /* symset_list */

    /* pickup.c */
    0,  /* oldcap */
    UNDEFINED_PTR, /* current_container */
    UNDEFINED_VALUE, /* abort_looting */
    UNDEFINED_VALUE, /* val_for_n_or_more */
    UNDEFINED_VALUES, /* valid_menu_classes */
    UNDEFINED_VALUE, /* class_filter */
    UNDEFINED_VALUE, /* bucx_filter */
    UNDEFINED_VALUE, /* shop_filter */

    /* pline.c */
    0, /* pline_flags */
    UNDEFINED_VALUES, /* prevmsg */
#ifdef DUMPLOG
    0, /* saved_pline_index */
    UNDEFINED_VALUES,
#endif
    NULL, /* you_buf */
    0, /* you_buf_siz */

    /* polyself.c */
    0, /* sex_change_ok */

    /* potion.c */
    FALSE, /* notonhead */
    UNDEFINED_VALUE, /* potion_nothing */
    UNDEFINED_VALUE, /* potion_unkn */

    /* pray.c */
    UNDEFINED_VALUE, /* p_aligntyp */
    UNDEFINED_VALUE, /* p_trouble */
    UNDEFINED_VALUE, /* p_type */

    /* quest.c */
    DUMMY, /* quest_status */

    /* questpgr.c */
    UNDEFINED_VALUES, /* cvt_buf */
    UNDEFINED_VALUES, /* qt_list */
    UNDEFINED_PTR, /* msg_file */
    UNDEFINED_VALUES, /* nambuf */

    /* read.c */
    UNDEFINED_VALUE, /* known */

    /* restore.c */
    0, /* n_ids_mapped */
    0, /* id_map */
    FALSE, /* restoring */
    UNDEFINED_PTR, /* oldfruit */
    UNDEFINED_VALUE, /* omoves */

    /* rumors.c */
    0, /* true_rumor_size */
    0, /* false_rumor_size */
    UNDEFINED_VALUE, /* true_rumor_start*/
    UNDEFINED_VALUE, /* false_rumor_start*/
    UNDEFINED_VALUE, /* true_rumor_end */
    UNDEFINED_VALUE, /* false_rumor_end */
    0, /* oracle_flag */
    0, /* oracle_cnt */
    NULL, /* oracle_loc */

    /* save.c */
    TRUE, /* havestate*/
    0, /* ustuck_id */
    0, /* usteed_id */

    /* shk.c */
    'a', /* sell_response */
    SELL_NORMAL, /* sell_how */
    FALSE, /* auto_credit */
    UNDEFINED_VALUES, /* repo */

    /* sp_lev.c */
    NULL, /* lev_message */
    NULL, /* lregions */
    0, /* num_lregions */
    UNDEFINED_VALUES, /* SplLev_Map */
    UNDEFINED_VALUE, /* xstart */
    UNDEFINED_VALUE, /* ystart */
    UNDEFINED_VALUE, /* xsize */
    UNDEFINED_VALUE, /* ysize */
    FALSE, /* splev_init_present */
    FALSE, /* icedpools */
    0, /* mines_prize_count */
    0, /* soki_prize_count */
    { UNDEFINED_PTR }, /* container_obj */
    0, /* container_idx */
    NULL, /* invent_carrying_monster */
    { AM_CHAOTIC, AM_NEUTRAL, AM_LAWFUL }, /* ralign */

    /* spells.c */
    0, /* spl_sortmode */
    NULL, /* spl_orderindx */

    /* timeout.c */
    UNDEFINED_PTR, /* timer_base */
    1, /* timer_id */

    /* topten.c */
    WIN_ERR, /* topten */

    /* trap.c */
    0, /* force_mintrap */
    { 0, 0, FALSE },
    UNDEFINED_VALUES,

    /* u_init.c */
    STRANGE_OBJECT, /* nocreate */
    STRANGE_OBJECT, /* nocreate2 */
    STRANGE_OBJECT, /* nocreate3 */
    STRANGE_OBJECT, /* nocreate4 */

    /* uhitm.c */
    UNDEFINED_VALUE, /* override_confirmation */

    /* vision.c */
    NULL, /* viz_array */
    NULL, /* viz_rmin */
    NULL, /* viz_rmax */
    FALSE, /* vision_full_recalc */

    /* weapon.c */
    UNDEFINED_PTR, /* propellor */

    /* windows.c */
    NULL, /* last_winchoice */

    /* zap.c */
    UNDEFINED_VALUE, /* poly_zap */
    UNDEFINED_VALUE,  /* obj_zapped */

    IVMAGIC  /* used to validate that structure layout has been preserved */
};

struct instance_globals g;

void
decl_globals_init()
{
    g = g_init;

    g.valuables[0].list = g.gems;
    g.valuables[0].size = SIZE(g.gems);
    g.valuables[1].list = g.amulets;
    g.valuables[1].size = SIZE(g.amulets);
    g.valuables[2].list = NULL;
    g.valuables[2].size = 0;

    nhassert(g_init.magic == IVMAGIC);
    nhassert(g_init.havestate == TRUE);

    sfcap = default_sfinfo;
    sfrestinfo = default_sfinfo;
    sfsaveinfo = default_sfinfo;
}

/*decl.c*/
