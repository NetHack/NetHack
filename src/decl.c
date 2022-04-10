/* NetHack 3.7	decl.c	$NHDT-Date: 1645000574 2022/02/16 08:36:14 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.248 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2009. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

const char quitchars[] = " \r\n\033";
const char vowels[] = "aeiouAEIOU";
const char ynchars[] = "yn";
const char ynqchars[] = "ynq";
const char ynaqchars[] = "ynaq";
const char ynNaqchars[] = "yn#aq";
NEARDATA long yn_number = 0L;

const char disclosure_options[] = "iavgco";

/* x/y/z deltas for the 10 movement directions (8 compass pts, 2 down/up) */
const schar xdir[N_DIRS_Z] = { -1, -1,  0,  1,  1,  1,  0, -1, 0,  0 };
const schar ydir[N_DIRS_Z] = {  0, -1, -1, -1,  0,  1,  1,  1, 0,  0 };
const schar zdir[N_DIRS_Z] = {  0,  0,  0,  0,  0,  0,  0,  0, 1, -1 };
/* redordered directions, cardinals first */
const schar dirs_ord[N_DIRS] =
    { DIR_W, DIR_N, DIR_E, DIR_S, DIR_NW, DIR_NE, DIR_SE, DIR_SW };

NEARDATA struct flag flags;
NEARDATA boolean has_strong_rngseed = FALSE;
NEARDATA struct instance_flags iflags;
NEARDATA struct you u;
NEARDATA time_t ubirthday;
NEARDATA struct u_realtime urealtime;

NEARDATA struct obj *uwep, *uarm, *uswapwep,
    *uquiver, /* quiver */
    *uarmu, /* under-wear, so to speak */
    *uskin, /* dragon armor, if a dragon */
    *uarmc, *uarmh, *uarms, *uarmg,*uarmf, *uamul,
    *uright, *uleft, *ublindf, *uchain, *uball;

struct engr *head_engr;

const int shield_static[SHIELD_COUNT] = {
    S_ss1, S_ss2, S_ss3, S_ss2, S_ss1, S_ss2, S_ss4, /* 7 per row */
    S_ss1, S_ss2, S_ss3, S_ss2, S_ss1, S_ss2, S_ss4,
    S_ss1, S_ss2, S_ss3, S_ss2, S_ss1, S_ss2, S_ss4,
};


NEARDATA const struct c_color_names c_color_names = {
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

const struct c_common_strings c_common_strings = { "Nothing happens.",
                                             "That's enough tries!",
                                             "That is a silly thing to %s.",
                                             "shudder for a moment.",
                                             "something",
                                             "Something",
                                             "You can move again.",
                                             "Never mind.",
                                             "vision quickly clears.",
                                             { "the", "your" },
                                             { "mon", "you" } };

/* NOTE: the order of these words exactly corresponds to the
   order of oc_material values #define'd in objclass.h. */
const char *materialnm[] = { "mysterious", "liquid",  "wax",        "organic",
                             "flesh",      "paper",   "cloth",      "leather",
                             "wooden",     "bone",    "dragonhide", "iron",
                             "metal",      "copper",  "silver",     "gold",
                             "platinum",   "mithril", "plastic",    "glass",
                             "gemstone",   "stone" };

char emptystr[] = {0};       /* non-const */

/* Global windowing data, defined here for multi-window-system support */
NEARDATA winid WIN_MESSAGE, WIN_STATUS, WIN_MAP, WIN_INVEN;
#ifdef WIN32
boolean fqn_prefix_locked[PREFIX_COUNT] = { FALSE, FALSE, FALSE,
                                            FALSE, FALSE, FALSE,
                                            FALSE, FALSE, FALSE,
                                            FALSE };
#endif

#ifdef PREFIXES_IN_USE
const char *fqn_prefix_names[PREFIX_COUNT] = {
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

#define IVMAGIC 0xdeadbeef

#ifdef GCC_WARN
#pragma GCC diagnostic ignored "-Wmissing-braces"
#endif

const struct Role urole_init_data = {
    { "Undefined", 0 },
    { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
      { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
    "L", "N", "C",
    "Xxx", "home", "locate",
    NON_PM, NON_PM, NON_PM, NON_PM, NON_PM, NON_PM, NON_PM,
    0, 0, 0, 0,
    /* Str Int Wis Dex Con Cha */
    { 7, 7, 7, 7, 7, 7 },
    { 20, 15, 15, 20, 20, 10 },
    /* Init   Lower  Higher */
    { 10, 0, 0, 8, 1, 0 }, /* Hit points */
    { 2, 0, 0, 2, 0, 3 },
    14, /* Energy */
     0,
    10,
     0,
     0,
     4,
    A_INT,
     0,
    -3
};

const struct Race urace_init_data = {
    "something",
    "undefined",
    "something",
    "Xxx",
    { 0, 0 },
    NON_PM,
    NON_PM,
    NON_PM,
    0,
    0,
    0,
    0,
    /*    Str     Int Wis Dex Con Cha */
    { 3, 3, 3, 3, 3, 3 },
    { STR18(100), 18, 18, 18, 18, 18 },
    /* Init   Lower  Higher */
    { 2, 0, 0, 2, 1, 0 }, /* Hit points */
    { 1, 0, 2, 0, 2, 0 }  /* Energy */
};

const struct instance_globals g_init = {

    NULL, /* command_queue */

    /* apply.c */
    0,  /* jumping_is_magic */
    -1, /* polearm_range_min */
    -1, /* polearm_range_max  */
    UNDEFINED_VALUES, /* trapinfo */

    /* artifact.c */
    0,  /* spec_dbon_applies */
    0, /* mkot_trap_warn_count */

    /* botl.c */
    0,  /* mrank_sz */
    UNDEFINED_VALUES, /* blstats */
    FALSE, /* blinit */
    FALSE, /* update_all */
    UNDEFINED_VALUES, /* valset */
#ifdef STATUS_HILITES
    0, /* bl_hilite_moves */
#endif
    UNDEFINED_VALUES, /* cond_hilites */
    0, /* now_or_before_idx */
    0, /* condmenu_sortorder */

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
    UNDEFINED_VALUE, /* last_command_count */
    NULL, /* ext_tlist */

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
    UNDEFINED_VALUES, /* command_line */
    0, /* command_count */
    NULL, /* multi_reason */
    /* multi_reason usually points to a string literal (when not Null)
       but multireasonbuf[] is available for when it needs to be dynamic */
    DUMMY, /* multireasonbuf[] */
    0, /* nroom */
    0, /* nsubroom */
    0, /* occtime */
    0, /* warn_obj_cnt */
    /* maze limits must be even; masking off lowest bit guarantees that */
    (COLNO - 1) & ~1, /* x_maze_max */
    (ROWNO - 1) & ~1, /* y_maze_max */
    UNDEFINED_VALUE, /* otg_temp */
    0, /* in_doagain */
    NULL, /* stairs */
    DUMMY, /* smeq */
    0, /* doorindex */
    0, /* done_money */
    0L, /* domove_attempting */
    0L, /* domove_succeeded */
    NULL, /* nomovemsg */
    DUMMY, /* plname */
    0, /* plnamelen */
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
    DUMMY, /* dungeon_topology */
    DUMMY, /* killer */
    DUMMY, /* rooms */
    NULL, /* subrooms */
    UNDEFINED_VALUES, /* level */
    1L, /* moves; misnamed turn counter */
    1L << 3, /* hero_seq: sequence number for hero movement, 'moves*8 + n'
              * where n is usually 1, sometimes 2 when Fast/Very_fast, maybe
              * higher if polymorphed into something that's even faster */
    0L, /* wailmsg */
    NULL, /* migrating_objs */
    NULL, /* billobjs */
#if defined(MICRO) || defined(WIN32)
    UNDEFINED_VALUES, /* hackdir */
#endif /* MICRO || WIN32 */
    DUMMY, /* youmonst */
    NULL, /* invent */
    DUMMY, /* context */
    DUMMY, /* fqn_prefix */
    DUMMY, /* tc_gbl_data */
#if defined(UNIX) || defined(VMS)
    0, /* locknum */
#endif
#ifdef DEF_PAGER
    NULL, /* catmore */
#endif
#ifdef MICRO
    UNDEFINED_VALUES, /* levels */
#endif /* MICRO */
    UNDEFINED_VALUES, /* program_state */

    /* detect.c */
    0, /* already_found_flag */

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
    0, /* did_nothing_flag */
    { 0, 0 }, /* save_dlevel */

    /* do_name.c */
    NULL, /* gloc_filter_map */
    UNDEFINED_VALUE, /* gloc_filter_floodfill_match_glyph */

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
    NULL, /* apelist */
    UNDEFINED_VALUES, /* mvitals */

    /* dokick.c */
    UNDEFINED_PTR, /* maploc */
    UNDEFINED_VALUES, /* nowhere */
    UNDEFINED_PTR, /* gate_str */

    /* drawing.c */
    DUMMY, /* symset */
    0, /* currentgraphics */
    DUMMY, /* showsyms */
    DUMMY, /* primary_syms */
    DUMMY, /* rogue_syms */
    DUMMY, /* ov_primary_syms */
    DUMMY, /* ov_rogue_syms */
    DUMMY, /* warnsyms */

    /* dungeon.c */
    0, /* n_dgns */
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
    NULL, /* cmdline_rcfile */
    UNDEFINED_VALUES, /* wizkit */
    UNDEFINED_VALUE, /* lockptr */
    NULL, /* config_section_chosen */
    NULL, /* config_section_current */
    0, /* nesting */
    0, /* no_sound_notified */
    0, /* symset_count */
    FALSE, /* chosen_symset_start */
    FALSE, /* chosen_symset_end */
    0, /* symset_which_set */
    DUMMY, /* SAVEF */
#ifdef MICRO
    DUMMY, /* SAVEP */
#endif
    BONESINIT, /* bones */
    LOCKNAMEINIT, /* lock */


    /* hack.c */
    UNDEFINED_VALUES, /* tmp_anything */
    UNDEFINED_VALUE, /* wc */
    NULL, /* travelmap */

    /* invent.c */
    51, /* lastinvr */
    0, /* sortloogmode */
    NULL, /* invbuf */
    0, /* inbufsize */
    WIN_ERR, /* cached_pickinv_win */
    0, /* this_type */
    NULL, /* this_title */
    UNDEFINED_VALUES, /* only (coord) */

    /* light.c */
    NULL, /* light_source */

    /* lock.c */
    UNDEFINED_VALUES,

    /* makemon.c */

    /* mhitm.c */
    0L, /* noisetime */
    FALSE, /* far_noise */
    FALSE, /* vis */
    FALSE, /* skipdrin */

    /* mhitu.c */
    UNDEFINED_VALUE, /* mhitu_dieroll */

    /* mklev.c */
    UNDEFINED_VALUES, /* luathemes[] */
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
    UNDEFINED_PTR, /* bbubbles */
    UNDEFINED_PTR, /* ebubbles */
    UNDEFINED_PTR, /* wportal */
    UNDEFINED_VALUE, /* xmin */
    UNDEFINED_VALUE, /* ymin */
    UNDEFINED_VALUE, /* xmax */
    UNDEFINED_VALUE, /* ymax */
    0, /* ransacked */

    /* mkobj.c */
    FALSE, /* mkcorpstat_norevive */

    /* mon.c */
    FALSE, /* vamp_rise_msg */
    FALSE, /* disintegested */
    FALSE, /* zombify */
    NULL, /* animal_list */
    UNDEFINED_VALUE, /* animal_list_count */

    /* mthrowu.c */
    UNDEFINED_VALUE, /* mesg_given */
    NULL, /* mtarget */
    NULL, /* marcher */

    /* muse.c */
    FALSE, /* m_using */
    UNDEFINED_VALUE, /* trapx */
    UNDEFINED_VALUE, /* trapy */
    FALSE, /* zap_oseen */
    UNDEFINED_VALUES, /* m */

    /* nhlan.c */
#ifdef MAX_LAN_USERNAME
    UNDEFINED_VALUES, /* lusername */
    MAX_LAN_USERNAME, /* lusername_size */
#endif /* MAX_LAN_USERNAME */

    /* nhlua.c */
    UNDEFINED_VALUE, /* luacore */

    /* o_init.c */
    DUMMY, /* disco */
    DUMMY, /* oclass_prob_totals */

    /* objname.c */
    0, /* distantname */

    /* options.c */
    (struct symsetentry *) 0, /* symset_list */
    UNDEFINED_VALUES, /* mapped_menu_cmds */
    UNDEFINED_VALUES, /* mapped_menu_op */
    0, /* n_menu_mapped */
    FALSE, /* opt_initial */
    FALSE, /* opt_from_file */
    FALSE, /* opt_need_redraw */
    FALSE, /* opt_need_glyph_reset */
    FALSE, /* save_menucolors */
    (struct menucoloring *) 0, /* save_colorings */
    (struct menucoloring *) 0, /* color_colorings */

    /* pickup.c */
    0,  /* oldcap */
    (struct obj *) 0, /* current_container */
    UNDEFINED_VALUE, /* abort_looting */
    UNDEFINED_VALUE, /* val_for_n_or_more */
    UNDEFINED_VALUES, /* valid_menu_classes */
    UNDEFINED_VALUE, /* class_filter */
    UNDEFINED_VALUE, /* bucx_filter */
    UNDEFINED_VALUE, /* shop_filter */
    UNDEFINED_VALUE, /* picked_filter */
    UNDEFINED_VALUE, /* loot_reset_justpicked */

    /* pline.c */
    0, /* pline_flags */
    UNDEFINED_VALUES, /* prevmsg */
#ifdef DUMPLOG
    0, /* saved_pline_index */
    UNDEFINED_VALUES,
#endif
    (char *) 0, /* you_buf */
    0, /* you_buf_siz */
    NULL, /* gamelog */

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
    UNDEFINED_VALUES, /* nambuf */

    /* read.c */
    UNDEFINED_VALUE, /* known */

    /* region.c */
    NULL, /* regions */
    0, /* n_regions */
    0, /* max_regions */
    FALSE, /* gas_cloud_diss_within */
    0, /* gas_cloud_diss_seen */

    /* restore.c */
    0, /* n_ids_mapped */
    0, /* id_map */
    UNDEFINED_PTR, /* oldfruit */
    UNDEFINED_VALUE, /* omoves */

    /* rip.c */
    UNDEFINED_PTR, /* rip */

    /* role.c */
    UNDEFINED_VALUES, /* urole */
    UNDEFINED_VALUES, /* urace */
    UNDEFINED_VALUES, /* role_pa */
    UNDEFINED_VALUE, /* role_post_attrib */
    UNDEFINED_VALUES, /* rfilter */

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
    (struct obj *) 0, /* looseball */
    (struct obj *) 0, /* loosechain */

    /* shk.c */
    'a', /* sell_response */
    SELL_NORMAL, /* sell_how */
    FALSE, /* auto_credit */
    UNDEFINED_VALUES, /* repo */
    UNDEFINED_VALUE, /* followmsg */

    /* sp_lev.c */
    NULL, /* lev_message */
    NULL, /* lregions */
    0, /* num_lregions */
    NULL, /* coder */
    UNDEFINED_VALUE, /* xstart */
    UNDEFINED_VALUE, /* ystart */
    UNDEFINED_VALUE, /* xsize */
    UNDEFINED_VALUE, /* ysize */
    FALSE, /* in_mk_themerooms */
    FALSE, /* themeroom_failed */

    /* spells.c */
    0, /* spl_sortmode */
    NULL, /* spl_orderindx */

    /* steal.c */
    0, /* stealoid */
    0, /* stealmid */

    /* teleport.c */

    /* timeout.c */
    UNDEFINED_PTR, /* timer_base */
    1, /* timer_id */

    /* topten.c */
    WIN_ERR, /* toptenwin */

    /* trap.c */
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

    /* new */
    DUMMY,   /* lua_ver[LUA_VER_BUFSIZ] */
    DUMMY,   /* lua_copyright[LUA_COPYRIGHT_BUFSIZ] */

    /* per-level glyph mapping flags */
    0L,     /* glyphmap_perlevel_flags */

    IVMAGIC  /* used to validate that structure layout has been preserved */
};

struct instance_globals g;

const struct const_globals cg = {
    DUMMY, /* zeroobj */
    DUMMY, /* zeromonst */
    DUMMY, /* zeroany */
};

#define ZERO(x) memset(&x, 0, sizeof(x))

void
decl_globals_init(void)
{
    g = g_init;

    g.valuables[0].list = g.gems;
    g.valuables[0].size = SIZE(g.gems);
    g.valuables[1].list = g.amulets;
    g.valuables[1].size = SIZE(g.amulets);
    g.valuables[2].list = NULL;
    g.valuables[2].size = 0;

    if (g_init.magic != IVMAGIC) {
        raw_printf(
                 "decl_globals_init: g_init.magic in unexpected state (%lx).",
                   g_init.magic);
        exit(1);
    }
    if (g_init.havestate != TRUE) {
        raw_print("decl_globals_init: g_init.havestate not True.");
        exit(1);
    }

    sfcap = default_sfinfo;
    sfrestinfo = default_sfinfo;
    sfsaveinfo = default_sfinfo;

    g.subrooms = &g.rooms[MAXNROFROOMS + 1];

    ZERO(flags);
    ZERO(iflags);
    ZERO(u);
    ZERO(ubirthday);
    ZERO(urealtime);

    uwep = uarm = uswapwep = uquiver = uarmu = uskin = uarmc = NULL;
    uarmh = uarms = uarmg = uarmf = uamul = uright = uleft = NULL;
    ublindf = uchain = uball = NULL;

    WIN_MESSAGE =  WIN_STATUS =  WIN_MAP = WIN_INVEN = WIN_ERR;

    g.urole = urole_init_data;
    g.urace = urace_init_data;
}

/*decl.c*/
