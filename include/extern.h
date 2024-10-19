/* NetHack 3.7	extern.h	$NHDT-Date: 1723580890 2024/08/13 20:28:10 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.1435 $ */
/* Copyright (c) Steve Creps, 1988.                               */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef EXTERN_H
#define EXTERN_H

/*
 * The placements of the NONNULLARG* and NONNULLPTRS macros were done
 * using the following rules:
 * These were the rules that were followed when determining which function
 * parameters should be nonnull, and which are nullable:
 *
 *   1. If the first use of, or reference to, the pointer parameter in the
 *      function is a dereference, then the parameter will be considered
 *      nonnull.
 *
 *   2. If there is code in the function that tests for the pointer parameter
 *      being null, and adjusts the code-path accordingly so that no segfault
 *      will occur, then the parameter will not be considered nonnull (it can
 *      be null).
 *
 * Note that if an arg is declared nonnull, any tests inside the function
 * for the variable being null, will likely trigger a compiler warning
 * diagnostic about the unnecessary test.
 *
 * Description of the NONNULL macros:
 *
 *  NONNULL         The function return value is never NULL.
 *  NONNULLPTRS     Every pointer argument is declared nonnull.
 *  NONNULLARG1     The 1st argument is declared nonnull.
 *  NONNULLARG2     The 2nd argument is declared nonnull.
 *  NONNULLARG3     The 3rd argument is declared nonnull.
 *  NONNULLARG4     The 4th argument is declared nonnull (not used).
 *  NONNULLARG5     The 5th argument is declared nonnull.
 *  NONNULLARG6     The 6th argument is declared nonnull.
 *  NONNULLARG7     The 7th argument is declared nonnull (bhit).
 *  NONNULLARG12    The 1st and 2nd arguments are declared nonnull.
 *  NONNULLARG23    The 2nd and 3rd arguments are declared nonnull.
 *  NONNULLARG13    The 1st and 3rd arguments are declared nonnull.
 *  NONNULLARG123   The 1st, 2nd and 3rd arguments are declared nonnull.
 *  NONNULLARG14    The 1st and 4th arguments are declared nonnull.
 *  NONNULLARG134   The 1st, 3rd and 4th arguments are declared nonnull.
 *  NONNULLARG17    The 1st and 7th arguments are declared nonnull (this
 *                  was a special-case added for askchain(), where the
 *                  arguments are spread out that way. This macro
 *                  could be removed if the askchain arguments in the
 *                  prototype and callers were changed to make the
 *                  nonnull arguments side-by-side).
 *  NONNULLARG145   The 1st, 4th and 5th arguments are declared nonnull
 *                  (this was a special-case added for find_roll_to_hit(),
 *                  in uhitm.c, where the arguments are spread out that way.
 *                  We can't just use NONNULLPTRS there because the 3rd
 *                  argument 'weapon' can be NULL).
 *  NONNULLARG24    The 2nd and 4th arguments are declared nonnull (this
 *                  was a special-case added for query_objlist() in invent.c).
 *  NONNULLARG45    The 4th and 5th arguments are declared nonnull (this
 *                  was a special-case added for do_screen_description(),
 *                  in pager.c, where the arguments are spread out that way.
 *                  We can't just use NONNULLPTRS there because the 6th
 *                  argument can be NULL).
 *  NO_NNARGS       This macro expands to nothing. It is just used to
 *                  mark that analysis has been done on the function,
 *                  and concluded that none of the arguments could be
 *                  marked nonnull.That distinguishes a function that has
 *                  not been analyzed (yet), from one that has.
 *
 */

/* ### alloc.c ### */

#if 0
/* routines in alloc.c depend on MONITOR_HEAP and are declared in global.h */
extern long *alloc(unsigned int) NONNULL;
#endif
extern char *fmt_ptr(const void *) NONNULL;
/* moved from hacklib.c to alloc.c so that utility programs have access */
#define FITSint(x) FITSint_(x, __func__, __LINE__)
extern int FITSint_(long long, const char *, int) NONNULLARG2;
#define FITSuint(x) FITSuint_(x, __func__, __LINE__)
extern unsigned FITSuint_(unsigned long long, const char *, int) NONNULLARG2;
/* for Strlen() which returns unsigned instead of size_t and panics for
   strings of length INT_MAX (32K - 1) or longer */

#include "hacklib.h"

/* This next pre-processor directive covers almost the entire file,
 * interrupted only occasionally to pick up specific functions as needed. */
#if !defined(MAKEDEFS_C) && !defined(MDLIB_C) && !defined(CPPREGEX_C)

/* ### allmain.c ### */

extern void early_init(int, char *[]);
extern void moveloop_core(void);
extern void moveloop(boolean);
extern void stop_occupation(void);
extern void init_sound_disp_gamewindows(void);
extern void newgame(void);
extern void welcome(boolean);
extern int argcheck(int, char **, enum earlyarg);
extern long timet_to_seconds(time_t);
extern long timet_delta(time_t, time_t);

/* ### apply.c ### */

extern void do_blinding_ray(struct obj *) NONNULLPTRS;
extern int doapply(void);
extern int dorub(void);
extern int dojump(void);
extern int jump(int);
extern int number_leashed(void);
extern void o_unleash(struct obj *) NONNULLPTRS;
extern void m_unleash(struct monst *, boolean) NONNULLPTRS;
extern void unleash_all(void);
extern boolean leashable(struct monst *) NONNULLARG1;
extern boolean next_to_u(void);
extern struct obj *get_mleash(struct monst *) NONNULLARG1;
extern const char *beautiful(void);
extern void check_leash(coordxy, coordxy);
extern boolean um_dist(coordxy, coordxy, xint16);
extern boolean snuff_candle(struct obj *) NONNULLPTRS;
extern boolean snuff_lit(struct obj *) NONNULLPTRS;
extern boolean splash_lit(struct obj *) NONNULLPTRS;
extern boolean catch_lit(struct obj *) NONNULLPTRS;
extern void use_unicorn_horn(struct obj **);
extern boolean tinnable(struct obj *) NONNULLPTRS;
extern void reset_trapset(void);
extern int use_whip(struct obj *) NONNULLPTRS;
extern int use_pole(struct obj *, boolean) NONNULLPTRS;
extern void fig_transform(union any *, long) NONNULLARG1;
extern int unfixable_trouble_count(boolean);

/* ### artifact.c ### */

extern void init_artifacts(void);
extern void save_artifacts(NHFILE *);
extern void restore_artifacts(NHFILE *);
extern const char *artiname(int);
extern struct obj *mk_artifact(struct obj *, aligntyp);
extern const char *artifact_name(const char *, short *, boolean) NONNULLARG1;
extern boolean exist_artifact(int, const char *) NONNULLPTRS;
extern void artifact_exists(struct obj *, const char *, boolean, unsigned) ;
extern void found_artifact(int);
extern void find_artifact(struct obj *) NONNULLPTRS;
extern int nartifact_exist(void);
extern void artifact_origin(struct obj *, unsigned) NONNULLPTRS;
extern boolean arti_immune(struct obj *, int);
extern boolean spec_ability(struct obj *, unsigned long);
extern boolean confers_luck(struct obj *) NONNULLPTRS;
extern boolean arti_reflects(struct obj *);
extern boolean shade_glare(struct obj *) NONNULLPTRS;
extern boolean restrict_name(struct obj *, const char *) NONNULLPTRS;
extern boolean attacks(int, struct obj *);
extern boolean defends(int, struct obj *);
extern boolean defends_when_carried(int, struct obj *);
extern boolean protects(struct obj *, boolean);
extern void set_artifact_intrinsic(struct obj *, boolean, long);
extern int touch_artifact(struct obj *, struct monst *) NONNULLARG2;
extern int spec_abon(struct obj *, struct monst *) NONNULLARG2;
extern int spec_dbon(struct obj *, struct monst *, int) NONNULLARG2;
extern void discover_artifact(xint16);
extern boolean undiscovered_artifact(xint16);
extern int disp_artifact_discoveries(winid);
extern void dump_artifact_info(winid);
extern boolean artifact_hit(struct monst *, struct monst *, struct obj *,
                            int *, int) NONNULLARG2;
extern int doinvoke(void);
extern boolean finesse_ahriman(struct obj *);
extern int arti_speak(struct obj *);
extern boolean artifact_light(struct obj *);
extern long spec_m2(struct obj *);
extern boolean artifact_has_invprop(struct obj *, uchar);
extern long arti_cost(struct obj *) NONNULLARG1;
extern struct obj *what_gives(long *) NONNULLARG1;
extern const char *glow_color(int);
extern const char *glow_verb(int, boolean);
extern void Sting_effects(int);
extern int retouch_object(struct obj **, boolean) NONNULLARG1;
extern void retouch_equipment(int);
extern void mkot_trap_warn(void);
extern boolean is_magic_key(struct monst *, struct obj *);
extern struct obj *has_magic_key(struct monst *);
extern boolean is_art(struct obj *, int);

/* ### attrib.c ### */

extern boolean adjattrib(int, int, int);
extern void gainstr(struct obj *, int, boolean);
extern void losestr(int, const char *, schar);
extern void poison_strdmg(int, int, const char *, schar);
extern void poisontell(int, boolean);
extern void poisoned(const char *, int, const char *, int, boolean) NONNULLARG1;
extern void change_luck(schar);
extern int stone_luck(boolean);
extern void set_moreluck(void);
extern void restore_attrib(void);
extern void exercise(int, boolean);
extern void exerchk(void);
extern void init_attr(int);
extern void redist_attr(void);
extern void vary_init_attr(void);
extern void adjabil(int, int);
extern int newhp(void);
extern int minuhpmax(int);
extern void setuhpmax(int, boolean);
extern int adjuhploss(int, int);
extern schar acurr(int);
extern schar acurrstr(void);
extern boolean extremeattr(int);
extern void adjalign(int);
extern int is_innate(int);
extern char *from_what(int);
extern void uchangealign(int, int);

/* ### ball.c ### */

extern void ballrelease(boolean);
extern void ballfall(void);
#ifndef BREADCRUMBS
extern void placebc(void);
extern void unplacebc(void);
extern int unplacebc_and_covet_placebc(void);
extern void lift_covet_and_placebc(int);
#else
#define placebc() Placebc(__FUNCTION__, __LINE__)
#define unplacebc() Unplacebc(__FUNCTION__, __LINE__)
#define unplacebc_and_covet_placebc() \
            Unplacebc_and_covet_placebc(__FUNCTION__, __LINE__)
#define lift_covet_and_placebc(x) \
            Lift_covet_and_placebc(x, __FUNCTION__, __LINE__)
#endif
extern void set_bc(int);
extern void move_bc(int, int, coordxy, coordxy, coordxy, coordxy);
extern boolean drag_ball(coordxy, coordxy, int *, coordxy *, coordxy *,
                         coordxy *, coordxy *, boolean *, boolean) NONNULLPTRS;
extern void drop_ball(coordxy, coordxy);
extern void drag_down(void);
extern void bc_sanity_check(void);

/* ### bones.c ### */

extern void sanitize_name(char *) NONNULLARG1;
extern void drop_upon_death(struct monst *, struct obj *, coordxy, coordxy);
extern boolean can_make_bones(void);
extern void savebones(int, time_t, struct obj *);
extern int getbones(void);
extern boolean bones_include_name(const char *) NONNULLARG1;
extern void fix_ghostly_obj(struct obj *) NONNULLARG1;

/* ### botl.c ### */

extern char *do_statusline1(void);
extern void check_gold_symbol(void);
extern char *do_statusline2(void);
extern void bot(void);
extern void timebot(void);
extern int xlev_to_rank(int);
extern int rank_to_xlev(int);
extern const char *rank_of(int, short, boolean);
extern int title_to_mon(const char *, int *, int *);
extern void max_rank_sz(void);
#ifdef SCORE_ON_BOTL
extern long botl_score(void);
#endif
extern int describe_level(char *, int);
extern void status_initialize(boolean);
extern void status_finish(void);
extern boolean exp_percent_changing(void);
extern int stat_cap_indx(void);
extern int stat_hunger_indx(void);
extern const char *bl_idx_to_fldname(int);
extern void repad_with_dashes(char *);
extern void condopt(int, boolean *, boolean);
extern int parse_cond_option(boolean, char *);
extern boolean cond_menu(void);
extern boolean opt_next_cond(int, char *);
#ifdef STATUS_HILITES
extern void status_eval_next_unhilite(void);
extern void reset_status_hilites(void);
extern boolean parse_status_hl1(char *op, boolean);
extern void status_notify_windowport(boolean);
extern void clear_status_hilites(void);
extern int count_status_hilites(void);
extern void all_options_statushilites(strbuf_t *);
extern boolean status_hilite_menu(void);
#endif /* STATUS_HILITES */

/* ### calendar.c ### */

extern time_t getnow(void);
extern int getyear(void);
#if 0
extern char *yymmdd(time_t) NONNULL;
#endif
extern long yyyymmdd(time_t);
extern long hhmmss(time_t);
extern char *yyyymmddhhmmss(time_t) NONNULL;
extern time_t time_from_yyyymmddhhmmss(char *);
extern int phase_of_the_moon(void);
extern boolean friday_13th(void);
extern int night(void);
extern int midnight(void);

/* ### coloratt.c ### */

extern char *color_attr_to_str(color_attr *);
extern boolean color_attr_parse_str(color_attr *, char *);
extern int32 colortable_to_int32(struct nethack_color *);
extern int query_color(const char *, int) NO_NNARGS;
extern int query_attr(const char *, int) NO_NNARGS;
extern boolean query_color_attr(color_attr *, const char *) NONNULLARG1;
extern const char *attr2attrname(int);
extern void basic_menu_colors(boolean);
extern boolean add_menu_coloring_parsed(const char *, int, int);
extern const char *clr2colorname(int);
extern int match_str2clr(char *, boolean) NONNULLARG1;
extern int match_str2attr(const char *, boolean) NONNULLARG1;
extern boolean add_menu_coloring(char *) NONNULLARG1;
extern void free_one_menu_coloring(int);
extern void free_menu_coloring(void);
extern int count_menucolors(void);
extern int32 check_enhanced_colors(char *) NONNULLARG1;
extern const char *wc_color_name(int32) NONNULL;
extern int32_t rgbstr_to_int32(const char *rgbstr);
extern boolean closest_color(uint32_t lcolor, uint32_t *closecolor, uint16 *clridx);
extern int color_distance(uint32_t, uint32_t);
extern boolean onlyhexdigits(const char *buf);
extern uint32 get_nhcolor_from_256_index(int idx);
#ifdef CHANGE_COLOR
extern int count_alt_palette(void);
extern int alternative_palette(char *);
extern void change_palette(void);
#endif

/* ### cmd.c ### */

extern void set_move_cmd(int, int);
extern int do_move_west(void);
extern int do_move_northwest(void);
extern int do_move_north(void);
extern int do_move_northeast(void);
extern int do_move_east(void);
extern int do_move_southeast(void);
extern int do_move_south(void);
extern int do_move_southwest(void);
extern int do_rush_west(void);
extern int do_rush_northwest(void);
extern int do_rush_north(void);
extern int do_rush_northeast(void);
extern int do_rush_east(void);
extern int do_rush_southeast(void);
extern int do_rush_south(void);
extern int do_rush_southwest(void);
extern int do_run_west(void);
extern int do_run_northwest(void);
extern int do_run_north(void);
extern int do_run_northeast(void);
extern int do_run_east(void);
extern int do_run_southeast(void);
extern int do_run_south(void);
extern int do_run_southwest(void);
extern int do_reqmenu(void);
extern int do_rush(void);
extern int do_run(void);
extern int do_fight(void);
extern int do_repeat(void);
extern char randomkey(void);
extern void random_response(char *, int);
extern int rnd_extcmd_idx(void);
extern int domonability(void);
extern const struct ext_func_tab *ext_func_tab_from_func(int(*)(void));
extern char cmd_from_func(int(*)(void));
extern char cmd_from_dir(int, int);
extern char *cmd_from_ecname(const char *);
extern const char *cmdname_from_func(int(*)(void), char *, boolean);
extern boolean redraw_cmd(char);
extern const char *levltyp_to_name(int);
extern int dolookaround(void);
extern void reset_occupations(void);
extern void set_occupation(int(*)(void), const char *, cmdcount_nht);
extern void cmdq_add_ec(int, int(*)(void));
extern void cmdq_add_key(int, char);
extern void cmdq_add_dir(int, schar, schar, schar);
extern void cmdq_add_userinput(int);
extern void cmdq_add_int(int, int);
extern void cmdq_shift(int);
extern struct _cmd_queue *cmdq_reverse(struct _cmd_queue *);
extern struct _cmd_queue *cmdq_copy(int);
extern struct _cmd_queue *cmdq_pop(void);
extern struct _cmd_queue *cmdq_peek(int);
extern void cmdq_clear(int);
extern char pgetchar(void);
extern char extcmd_initiator(void);
extern int doextcmd(void);
extern struct ext_func_tab *extcmds_getentry(int);
extern int count_bind_keys(void);
extern int count_autocompletions(void);
extern void get_changed_key_binds(strbuf_t *);
extern void handler_rebind_keys(void);
extern void handler_change_autocompletions(void);
extern int extcmds_match(const char *, int, int **);
extern const char *key2extcmddesc(uchar);
extern boolean bind_specialkey(uchar, const char *);
extern void parseautocomplete(char *, boolean);
extern void all_options_autocomplete(strbuf_t *);
extern void lock_mouse_buttons(boolean);
extern void reset_commands(boolean);
extern void update_rest_on_space(void);
extern void rhack(int);
extern int doextlist(void);
extern int extcmd_via_menu(void);
extern int enter_explore_mode(void);
extern boolean bind_mousebtn(int, const char *);
extern boolean bind_key(uchar, const char *);
extern void dokeylist(void);
extern coordxy xytod(coordxy, coordxy);
extern void dtoxy(coord *, int);
extern int movecmd(char, int);
extern int dxdy_moveok(void);
extern int getdir(const char *);
extern void confdir(boolean);
extern const char *directionname(int);
extern int isok(coordxy, coordxy);
extern int get_adjacent_loc(const char *, const char *, coordxy, coordxy,
                            coord *);
extern void click_to_cmd(coordxy, coordxy, int);
extern char get_count(const char *, char, long, cmdcount_nht *, unsigned);
#ifdef HANGUPHANDLING
extern void hangup(int);
extern void end_of_input(void);
#endif
extern char readchar(void);
extern char readchar_poskey(coordxy *, coordxy *, int *);
extern char* key2txt(uchar, char *);
extern char yn_function(const char *, const char *, char, boolean);
extern char paranoid_ynq(boolean, const char *, boolean);
extern boolean paranoid_query(boolean, const char *);
extern void makemap_prepost(boolean, boolean);
extern const char *ecname_from_fn(int (*)(void));

/* ### date.c ### */

extern void populate_nomakedefs(struct version_info *) NONNULLARG1;
extern void free_nomakedefs(void);

/* ### dbridge.c ### */

extern boolean is_waterwall(coordxy, coordxy);
extern boolean is_pool(coordxy, coordxy);
extern boolean is_lava(coordxy, coordxy);
extern boolean is_pool_or_lava(coordxy, coordxy);
extern boolean is_ice(coordxy, coordxy);
extern boolean is_moat(coordxy, coordxy);
extern schar db_under_typ(int);
extern int is_drawbridge_wall(coordxy, coordxy);
extern boolean is_db_wall(coordxy, coordxy);
extern boolean find_drawbridge(coordxy *, coordxy *) NONNULLPTRS;
extern boolean create_drawbridge(coordxy, coordxy, int, boolean);
extern void open_drawbridge(coordxy, coordxy);
extern void close_drawbridge(coordxy, coordxy);
extern void destroy_drawbridge(coordxy, coordxy);

/* ### decl.c ### */

extern void decl_globals_init(void);
extern void sa_victual(volatile struct victual_info *);

/* ### detect.c ### */

extern boolean trapped_chest_at(int, coordxy, coordxy);
extern boolean trapped_door_at(int, coordxy, coordxy);
extern struct obj *o_in(struct obj *, char) NONNULLARG1;
extern struct obj *o_material(struct obj *, unsigned) NONNULLARG1;
extern int gold_detect(struct obj *) NONNULLARG1;
extern int food_detect(struct obj *);
extern int object_detect(struct obj *, int);
extern int monster_detect(struct obj *, int);
extern int trap_detect(struct obj *);
extern const char *level_distance(d_level *) NONNULL NONNULLARG1;
extern void use_crystal_ball(struct obj **) NONNULLARG1;
extern void show_map_spot(coordxy, coordxy, boolean);
extern void do_mapping(void);
extern void do_vicinity_map(struct obj *);
extern void cvt_sdoor_to_door(struct rm *) NONNULLARG1;
extern int findit(void);
extern int openit(void);
extern boolean detecting(void(*)(coordxy, coordxy, void *));
extern void find_trap(struct trap *) NONNULLARG1;
extern void warnreveal(void);
extern int dosearch0(int);
extern int dosearch(void);
extern void premap_detect(void);
#ifdef DUMPLOG
extern void dump_map(void);
#endif
extern void reveal_terrain(unsigned);
extern int wiz_mgender(void);

/* ### dig.c ### */

extern int dig_typ(struct obj *, coordxy, coordxy);
extern boolean is_digging(void);
extern int holetime(void);
extern enum digcheck_result dig_check(struct monst *, coordxy, coordxy);
extern void digcheck_fail_message(enum digcheck_result, struct monst *,
                                  coordxy, coordxy);
extern void digactualhole(coordxy, coordxy, struct monst *, int);
extern boolean dighole(boolean, boolean, coord *);
extern int use_pick_axe(struct obj *) NONNULLARG1;
extern int use_pick_axe2(struct obj *) NONNULLARG1;
extern boolean mdig_tunnel(struct monst *) NONNULLARG1;
extern void draft_message(boolean);
extern void watch_dig(struct monst *, coordxy, coordxy, boolean);
extern void zap_dig(void);
extern struct obj *bury_an_obj(struct obj *, boolean *) NONNULLARG1;
extern void bury_objs(int, int);
extern void unearth_objs(int, int);
extern void rot_organic(union any *, long) NONNULLARG1;
extern void rot_corpse(union any *, long) NONNULLARG1;
extern struct obj *buried_ball(coord *) NONNULLARG1;
extern void buried_ball_to_punishment(void);
extern void buried_ball_to_freedom(void);
extern schar fillholetyp(coordxy, coordxy, boolean);
extern void liquid_flow(coordxy, coordxy, schar, struct trap *, const char *);
extern boolean conjoined_pits(struct trap *, struct trap *, boolean);
#if 0
extern void bury_monst(struct monst *) NONNULLARG1;
extern void bury_you(void);
extern void unearth_you(void);
extern void escape_tomb(void);
extern void bury_obj(struct obj *) NONNULLARG1;
#endif
#ifdef DEBUG
extern int wiz_debug_cmd_bury(void);
#endif

/* ### display.c ### */

extern int tp_sensemon(struct monst *) NONNULLARG1;
extern int sensemon(struct monst *) NONNULLARG1;
extern int mon_warning(struct monst *) NONNULLARG1;
extern int mon_visible(struct monst *) NONNULLARG1;
extern int see_with_infrared(struct monst *) NONNULLARG1;
extern int canseemon(struct monst *) NONNULLARG1;
extern int knowninvisible(struct monst *) NONNULLARG1;
extern int is_safemon(struct monst *) NONNULLARG1;
extern void magic_map_background(coordxy, coordxy, int);
extern void map_background(coordxy, coordxy, int);
extern void map_trap(struct trap *, int) NONNULLARG1;
extern void map_object(struct obj *, int) NONNULLARG1;
extern void map_invisible(coordxy, coordxy);
extern void map_engraving(struct engr *, int);
extern boolean unmap_invisible(coordxy, coordxy);
extern void unmap_object(coordxy, coordxy);
extern void map_location(coordxy, coordxy, int);
extern boolean suppress_map_output(void);
extern void feel_newsym(coordxy, coordxy);
extern void feel_location(coordxy, coordxy);
extern void newsym(coordxy, coordxy);
extern void newsym_force(coordxy, coordxy);
extern void shieldeff(coordxy, coordxy);
extern void tmp_at(coordxy, coordxy);
extern void flash_glyph_at(coordxy, coordxy, int, int);
extern void swallowed(int);
extern void under_ground(int);
extern void under_water(int);
extern void see_monsters(void);
extern void set_mimic_blocking(void);
extern void see_objects(void);
extern void see_nearby_objects(void);
extern void see_traps(void);
extern void curs_on_u(void);
extern int doredraw(void);
extern void docrt(void);
extern void docrt_flags(int);
extern void redraw_map(boolean);
extern void show_glyph(coordxy, coordxy, int);
extern void clear_glyph_buffer(void);
extern void row_refresh(coordxy, coordxy, coordxy);
extern void cls(void);
extern void flush_screen(int);
extern int back_to_glyph(coordxy, coordxy);
extern int zapdir_to_glyph(int, int, int);
extern int glyph_at(coordxy, coordxy);
extern void reglyph_darkroom(void);
extern void xy_set_wall_state(coordxy, coordxy);
extern void set_wall_state(void);
extern void unset_seenv(struct rm *, coordxy, coordxy, coordxy, coordxy);
extern int warning_of(struct monst *) NONNULLARG1;
extern void map_glyphinfo(coordxy, coordxy, int, unsigned, glyph_info *) NONNULLPTRS;
extern void reset_glyphmap(enum glyphmap_change_triggers trigger);
extern int fn_cmap_to_glyph(int);

/* ### do.c ### */

extern int dodrop(void);
extern boolean boulder_hits_pool(struct obj *, coordxy, coordxy, boolean);
extern boolean flooreffects(struct obj *, coordxy, coordxy,
                            const char *) NONNULLPTRS;
extern void doaltarobj(struct obj *) NONNULLARG1;
extern void polymorph_sink(void);
extern void trycall(struct obj *) NONNULLARG1;
extern boolean canletgo(struct obj *, const char *) NONNULLPTRS;
extern void dropx(struct obj *) NONNULLARG1;
extern void dropy(struct obj *) NONNULLARG1;
extern void dropz(struct obj *, boolean) NONNULLARG1;
extern void obj_no_longer_held(struct obj *);
extern int doddrop(void);
extern int dodown(void);
extern int doup(void);
#ifdef INSURANCE
extern void save_currentstate(void);
#endif
extern void u_collide_m(struct monst *);
extern void goto_level(d_level *, boolean, boolean, boolean) NONNULLARG1;
extern void hellish_smoke_mesg(void);
extern void maybe_lvltport_feedback(void);
extern void schedule_goto(d_level *, int, const char *, const char *) NONNULLARG1;
extern void deferred_goto(void);
extern boolean revive_corpse(struct obj *) NONNULLARG1;
extern void revive_mon(union any *, long) NONNULLARG1;
extern void zombify_mon(union any *, long) NONNULLARG1;
extern boolean cmd_safety_prevention(const char *, const char *,
                                     const char *, int *) NONNULLPTRS;
extern int donull(void);
extern int dowipe(void);
extern void legs_in_no_shape(const char *, boolean) NONNULLARG1;
extern void set_wounded_legs(long, int);
extern void heal_legs(int);

/* ### do_name.c ### */

extern void new_mgivenname(struct monst *, int) NONNULLARG1;
extern void free_mgivenname(struct monst *) NONNULLARG1;
extern void new_oname(struct obj *, int) NONNULLARG1;
extern void free_oname(struct obj *) NONNULLARG1;
extern const char *safe_oname(struct obj *) NONNULLARG1;
extern struct monst *christen_monst(struct monst *, const char *) NONNULLARG1;
extern struct obj *oname(struct obj *, const char *, unsigned) NONNULLPTRS;
extern boolean objtyp_is_callable(int);
extern int name_ok(struct obj *);
extern int call_ok(struct obj *);
extern int docallcmd(void);
extern void docall(struct obj *) NONNULLARG1;
extern const char *rndghostname(void);
extern char *x_monnam(struct monst *, int, const char *, int, boolean) NONNULLARG1;
extern char *l_monnam(struct monst *) NONNULLARG1;
extern char *mon_nam(struct monst *) NONNULLARG1;
extern char *noit_mon_nam(struct monst *) NONNULLARG1;
extern char *some_mon_nam(struct monst *) NONNULLARG1;
extern char *Monnam(struct monst *) NONNULLARG1;
extern char *noit_Monnam(struct monst *) NONNULLARG1;
extern char *Some_Monnam(struct monst *) NONNULLARG1;
extern char *noname_monnam(struct monst *, int) NONNULLARG1;
extern char *m_monnam(struct monst *) NONNULLARG1;
extern char *y_monnam(struct monst *) NONNULLARG1;
extern char *YMonnam(struct monst *) NONNULLARG1;
extern char *Adjmonnam(struct monst *, const char *) NONNULLARG1;
extern char *Amonnam(struct monst *) NONNULLARG1;
extern char *a_monnam(struct monst *) NONNULLARG1;
extern char *distant_monnam(struct monst *, int, char *) NONNULLARG1;
extern char *mon_nam_too(struct monst *, struct monst *) NONNULLPTRS;
extern char *monverbself(struct monst *, char *,
                         const char *, const char *) NONNULLARG123;
extern char *minimal_monnam(struct monst *, boolean);
extern char *bogusmon(char *, char *) NONNULLARG1;
extern char *rndmonnam(char *);
extern const char *hcolor(const char *);
extern const char *rndcolor(void);
extern const char *hliquid(const char *);
extern const char *roguename(void);
/*
extern struct obj *realloc_obj(struct obj *, int, genericptr_t, int,
                               const char *);
*/
extern char *coyotename(struct monst *, char *);
extern char *rndorcname(char *);
extern struct monst *christen_orc(struct monst *, const char *,
                                  const char *) NONNULLARG1;
extern const char *noveltitle(int *);
extern const char *lookup_novel(const char *, int *) NONNULLARG1;
#ifndef PMNAME_MACROS
extern int Mgender(struct monst *) NONNULLARG1;
extern const char *pmname(struct permonst *, int) NONNULLARG1;
#endif
extern const char *mon_pmname(struct monst *) NONNULLARG1;
extern const char *obj_pmname(struct obj *) NONNULLARG1;

/* ### do_wear.c ### */

extern const char *fingers_or_gloves(boolean);
extern void off_msg(struct obj *) NONNULLARG1;
extern void toggle_displacement(struct obj *, long, boolean);
extern void set_wear(struct obj *);
extern boolean donning(struct obj *) NONNULLARG1;
extern boolean doffing(struct obj *) NONNULLARG1;
extern void cancel_doff(struct obj *, long) NONNULLARG1;
extern void cancel_don(void);
extern int stop_donning(struct obj *); /* doseduce() calls with NULL */
extern int Armor_off(void);
extern int Armor_gone(void);
extern int Helmet_off(void);
extern boolean hard_helmet(struct obj *);
extern void wielding_corpse(struct obj *, struct obj *, boolean);
extern int Gloves_off(void);
extern int Boots_on(void);
extern int Boots_off(void);
extern int Cloak_off(void);
extern int Shield_off(void);
extern int Shirt_off(void);
extern void Amulet_off(void);
extern void Ring_on(struct obj *) NONNULLARG1;
extern void Ring_off(struct obj *) NONNULLARG1;
extern void Ring_gone(struct obj *) NONNULLARG1;
extern void Blindf_on(struct obj *) NONNULLARG1;
extern void Blindf_off(struct obj *);
extern int dotakeoff(void);
extern int doremring(void);
extern int cursed(struct obj *);
extern int armoroff(struct obj *);
extern int canwearobj(struct obj *, long *, boolean) NONNULLPTRS;
extern int dowear(void);
extern int doputon(void);
extern void find_ac(void);
extern void glibr(void);
extern struct obj *some_armor(struct monst *) NONNULLARG1;
extern struct obj *stuck_ring(struct obj *, int);
extern struct obj *unchanger(void);
extern void reset_remarm(void);
extern int doddoremarm(void);
extern int remarm_swapwep(void);
extern int destroy_arm(struct obj *);
extern void adj_abon(struct obj *, schar) NONNULLARG1;
extern boolean inaccessible_equipment(struct obj *, const char *, boolean);

/* ### dog.c ### */

extern void newedog(struct monst *) NONNULLARG1;
extern void free_edog(struct monst *) NONNULLARG1;
extern void initedog(struct monst *) NONNULLARG1;
extern struct monst *make_familiar(struct obj *, coordxy, coordxy, boolean);
extern struct monst *makedog(void);
extern void update_mlstmv(void);
extern void losedogs(void);
extern void mon_arrive(struct monst *, int) NONNULLARG1;
extern void mon_catchup_elapsed_time(struct monst *, long) NONNULLARG1;
extern void keepdogs(boolean);
extern void migrate_to_level(struct monst *, xint16, xint16, coord *) NONNULLARG1;
extern void discard_migrations(void);
extern int dogfood(struct monst *, struct obj *) NONNULLPTRS;
extern boolean tamedog(struct monst *, struct obj *, boolean) NONNULLARG1;
extern void abuse_dog(struct monst *) NONNULLARG1;
extern void wary_dog(struct monst *, boolean) NONNULLARG1;

/* ### dogmove.c ### */

extern boolean cursed_object_at(coordxy, coordxy);
extern struct obj *droppables(struct monst *) NONNULLARG1;
extern int dog_nutrition(struct monst *, struct obj *) NONNULLPTRS;
extern int dog_eat(struct monst *, struct obj *,
                   coordxy, coordxy, boolean) NONNULLPTRS;
extern int pet_ranged_attk(struct monst *) NONNULLARG1;
extern int dog_move(struct monst *, int) NONNULLARG1;
extern boolean could_reach_item(struct monst *, coordxy, coordxy) NONNULLARG1;
extern void finish_meating(struct monst *) NONNULLARG1;
extern void quickmimic(struct monst *) NONNULLARG1;

/* ### dokick.c ### */

extern boolean ghitm(struct monst *, struct obj *) NONNULLPTRS;
extern void container_impact_dmg(struct obj *, coordxy, coordxy) NONNULLARG1;
extern int dokick(void);
extern boolean ship_object(struct obj *, coordxy, coordxy, boolean);
extern void obj_delivery(boolean);
extern void deliver_obj_to_mon(struct monst *mtmp, int, unsigned long) NONNULLARG1;
extern schar down_gate(coordxy, coordxy);
extern void impact_drop(struct obj *, coordxy, coordxy, xint16);

/* ### dothrow.c ### */

extern int multishot_class_bonus(int, struct obj *, struct obj *) NONNULLARG2;
extern int dothrow(void);
extern int dofire(void);
extern void endmultishot(boolean);
extern void hitfloor(struct obj *, boolean) NONNULLARG1;
extern boolean hurtle_jump(genericptr_t, coordxy, coordxy) NONNULLARG1;
extern boolean hurtle_step(genericptr_t, coordxy, coordxy) NONNULLARG1;
extern boolean will_hurtle(struct monst *, coordxy, coordxy) NONNULLARG1;
extern void hurtle(int, int, int, boolean);
extern void mhurtle(struct monst *, int, int, int) NONNULLARG1;
extern boolean harmless_missile(struct obj *) NONNULLARG1;
extern boolean throwing_weapon(struct obj *) NONNULLARG1;
extern void throwit(struct obj *, long, boolean, struct obj *) NONNULLARG1;
extern int omon_adj(struct monst *, struct obj *, boolean) NONNULLPTRS;
extern boolean should_mulch_missile(struct obj *);
extern int thitmonst(struct monst *, struct obj *) NONNULLPTRS;
extern int hero_breaks(struct obj *, coordxy, coordxy, unsigned);
extern int breaks(struct obj *, coordxy, coordxy) NONNULLARG1;
extern void release_camera_demon(struct obj *, coordxy, coordxy) NONNULLARG1;
extern int breakobj(struct obj *, coordxy, coordxy, boolean, boolean) NONNULLARG1;
extern boolean breaktest(struct obj *) NONNULLARG1;
extern boolean walk_path(coord *, coord *,
                         boolean(*)(void *, coordxy, coordxy), genericptr_t) NONNULLARG12;

/* ### drawing.c ### */

extern int def_char_to_objclass(char);
extern int def_char_to_monclass(char);
extern int def_char_is_furniture(char);

/* ### dungeon.c ### */

extern void save_dungeon(NHFILE *, boolean, boolean) NONNULLARG1;
extern void restore_dungeon(NHFILE *) NONNULLARG1;
extern void insert_branch(branch *, boolean) NONNULLARG1;
extern void init_dungeons(void);
extern s_level *find_level(const char *) NONNULLARG1;
extern s_level *Is_special(d_level *) NONNULLARG1;
extern branch *Is_branchlev(d_level *) NONNULLARG1;
extern boolean builds_up(d_level *) NONNULLARG1;
extern xint16 ledger_no(d_level *) NONNULLARG1;
extern xint16 maxledgerno(void);
extern schar depth(d_level *) NONNULLARG1;
extern xint16 dunlev(d_level *) NONNULLARG1;
extern xint16 dunlevs_in_dungeon(d_level *) NONNULLARG1;
extern xint16 ledger_to_dnum(xint16);
extern xint16 ledger_to_dlev(xint16);
extern xint16 deepest_lev_reached(boolean);
extern boolean on_level(d_level *, d_level *) NONNULLARG12;
extern void next_level(boolean);
extern void prev_level(boolean);
extern void u_on_newpos(coordxy, coordxy);
extern void u_on_rndspot(int);
extern void get_level(d_level *, int) NONNULLARG1;
extern boolean Is_botlevel(d_level *) NONNULLARG1;
extern boolean Can_fall_thru(d_level *) NONNULLARG1;
extern boolean Can_dig_down(d_level *) NONNULLARG1;
extern boolean Can_rise_up(coordxy, coordxy, d_level *) NONNULLARG3;
extern boolean has_ceiling(d_level *) NONNULLARG1;
extern boolean avoid_ceiling(d_level *) NONNULLARG1;
extern const char *surface(coordxy, coordxy);
extern const char *ceiling(coordxy, coordxy);
extern boolean In_quest(d_level *) NONNULLARG1;
extern boolean In_mines(d_level *) NONNULLARG1;
extern branch *dungeon_branch(const char *) NONNULL NONNULLARG1;
extern boolean at_dgn_entrance(const char *) NONNULLARG1;
extern boolean In_hell(d_level *) NONNULLARG1;
extern boolean In_V_tower(d_level *) NONNULLARG1;
extern boolean On_W_tower_level(d_level *) NONNULLARG1;
extern boolean In_W_tower(coordxy, coordxy, d_level *) NONNULLARG3;
extern void find_hell(d_level *) NONNULLARG1;
extern void goto_hell(boolean, boolean);
extern boolean single_level_branch(d_level *) NONNULLARG1;
extern void assign_level(d_level *, d_level *) NONNULLPTRS;
extern void assign_rnd_level(d_level *, d_level *, int) NONNULLARG12;
extern unsigned int induced_align(int);
extern boolean Invocation_lev(d_level *) NONNULLARG1;
extern xint16 level_difficulty(void);
extern schar lev_by_name(const char *);
extern schar print_dungeon(boolean, schar *, xint16 *);
extern void print_level_annotation(void);
extern int donamelevel(void);
extern void free_exclusions(void);
extern void save_exclusions(NHFILE *) NONNULLARG1;
extern void load_exclusions(NHFILE *) NONNULLARG1;
extern int dooverview(void);
extern void show_overview(int, int);
extern void rm_mapseen(int);
extern void init_mapseen(d_level *) NONNULLARG1;
extern void update_lastseentyp(coordxy, coordxy);
extern int update_mapseen_for(coordxy, coordxy);
extern void recalc_mapseen(void);
extern void mapseen_temple(struct monst *);
extern void room_discovered(int);
extern void recbranch_mapseen(d_level *, d_level *) NONNULLPTRS;
extern void overview_stats(winid, const char *, long *, long *) NONNULLPTRS;
extern void remdun_mapseen(int);
extern const char *endgamelevelname(char *, int);

/* ### eat.c ### */

extern void eatmupdate(void);
extern boolean is_edible(struct obj *) NONNULLARG1;
extern void init_uhunger(void);
extern int Hear_again(void);
extern boolean eating_glob(struct obj *);
extern void reset_eat(void);
extern unsigned obj_nutrition(struct obj *) NONNULLARG1;
extern int doeat(void);
extern int use_tin_opener(struct obj *) NONNULLARG1;
extern void gethungry(void);
extern void morehungry(int);
extern void lesshungry(int);
extern boolean is_fainted(void);
extern void reset_faint(void);
extern int corpse_intrinsic(struct permonst *) NONNULLARG1;
extern void violated_vegetarian(void);
extern void newuhs(boolean);
extern struct obj *floorfood(const char *, int) NONNULLARG1;
extern void vomit(void);
extern int eaten_stat(int, struct obj *) NONNULLARG2;
extern void food_disappears(struct obj *) NONNULLARG1;
extern void food_substitution(struct obj *, struct obj *) NONNULLPTRS;
extern long temp_resist(int);
extern boolean eating_dangerous_corpse(int);
extern void eating_conducts(struct permonst *) NONNULLARG1;
extern int eat_brains(struct monst *, struct monst *, boolean,
                                                          int *) NONNULLARG12;
extern void fix_petrification(void);
extern int intrinsic_possible(int, struct permonst *) NONNULLARG2;
extern boolean should_givit(int, struct permonst *) NONNULLARG2;
extern void consume_oeaten(struct obj *, int) NONNULLARG1;
extern boolean maybe_finished_meal(boolean);
extern void cant_finish_meal(struct obj *) NONNULLARG1;
extern void set_tin_variety(struct obj *, int) NONNULLARG1;
extern int tin_variety_txt(char *, int *);
extern void tin_details(struct obj *, int, char *);
extern boolean Popeye(int);
extern int Finish_digestion(void);

/* ### end.c ### */

extern void done1(int);
extern int done2(void);
extern void done_in_by(struct monst *, int) NONNULLARG1;
extern void done_object_cleanup(void);
extern void NH_abort(char *);
#endif /* !MAKEDEFS_C && MDLIB_C && !CPPREGEX_C */
#if !defined(CPPREGEX_C)
ATTRNORETURN extern void panic(const char *, ...) PRINTF_F(1, 2) NORETURN;
#endif
#if !defined(MAKEDEFS_C) && !defined(MDLIB_C) && !defined(CPPREGEX_C)
extern void done(int);
extern void container_contents(struct obj *, boolean, boolean, boolean);
ATTRNORETURN extern void nh_terminate(int) NORETURN;
extern void delayed_killer(int, int, const char *);
extern struct kinfo *find_delayed_killer(int);
extern void dealloc_killer(struct kinfo *);
extern void save_killers(NHFILE *) NONNULLARG1;
extern void restore_killers(NHFILE *) NONNULLARG1;
extern char *build_english_list(char *) NONNULLARG1;

/* ### engrave.c ### */

extern char *random_engraving(char *) NONNULLARG1;
extern void wipeout_text(char *, int, unsigned) NONNULLARG1;
extern boolean can_reach_floor(boolean);
extern void cant_reach_floor(coordxy, coordxy, boolean, boolean);
extern struct engr *engr_at(coordxy, coordxy);
extern struct engr *sengr_at(const char *, coordxy, coordxy, boolean) NONNULLARG1;
extern void u_wipe_engr(int);
extern void wipe_engr_at(coordxy, coordxy, xint16, boolean);
extern void read_engr_at(coordxy, coordxy);
extern void make_engr_at(coordxy, coordxy, const char *, long, int) NONNULLARG3;
extern void del_engr_at(coordxy, coordxy);
extern int freehand(void);
extern int doengrave(void);
extern void sanitize_engravings(void);
extern void engraving_sanity_check(void);
extern void save_engravings(NHFILE *) NONNULLARG1;
extern void rest_engravings(NHFILE *) NONNULLARG1;
extern void engr_stats(const char *, char *, long *, long *) NONNULLPTRS;
extern void del_engr(struct engr *) NONNULLARG1;
extern void rloc_engr(struct engr *) NONNULLARG1;
extern void make_grave(coordxy, coordxy, const char *);
extern void disturb_grave(coordxy, coordxy);
extern void see_engraving(struct engr *) NONNULLARG1;
extern void feel_engraving(struct engr *) NONNULLARG1;

/* ### exper.c ### */

extern long newuexp(int);
extern int newpw(void);
extern int experience(struct monst *, int) NONNULLARG1;
extern void more_experienced(int, int);
extern void losexp(const char *);
extern void newexplevel(void);
extern void pluslvl(boolean);
extern long rndexp(boolean);

/* ### explode.c ### */

extern void explode(coordxy, coordxy, int, int, char, int);
extern long scatter(coordxy, coordxy, int, unsigned int, struct obj *);
extern void splatter_burning_oil(coordxy, coordxy, boolean);
extern void explode_oil(struct obj *, coordxy, coordxy) NONNULLARG1;
extern int adtyp_to_expltype(const int);
extern void mon_explodes(struct monst *, struct attack *) NONNULLPTRS;

/* ### extralev.c ### */

extern void makeroguerooms(void);
extern void corr(coordxy, coordxy);
extern void makerogueghost(void);

/* ### files.c ### */

extern const char *nh_basename(const char *, boolean) NONNULLARG1;
#if !defined(CROSSCOMPILE) || defined(CROSSCOMPILE_TARGET)
extern int l_get_config_errors(lua_State *) NONNULLARG1;
#endif
extern char *fname_encode(const char *, char,
                          char *, char *, int) NONNULLPTRS;
extern char *fname_decode(char, char *, char *, int) NONNULLPTRS;
extern const char *fqname(const char *, int, int);
extern FILE *fopen_datafile(const char *, const char *, int) NONNULLPTRS;
extern void zero_nhfile(NHFILE *) NONNULLARG1;
extern void close_nhfile(NHFILE *) NONNULLARG1;
extern void rewind_nhfile(NHFILE *) NONNULLARG1;
extern void set_levelfile_name(char *, int) NONNULLARG1;
extern NHFILE *create_levelfile(int, char *);
extern NHFILE *open_levelfile(int, char *);
extern void delete_levelfile(int);
extern void clearlocks(void);
extern NHFILE *create_bonesfile(d_level *, char **, char *) NONNULLARG12;
extern void commit_bonesfile(d_level *) NONNULLARG1;
extern NHFILE *open_bonesfile(d_level *, char **) NONNULLPTRS;
extern int delete_bonesfile(d_level *) NONNULLARG1;
extern void compress_bonesfile(void);
extern void set_savefile_name(boolean);
#ifdef INSURANCE
extern void save_savefile_name(NHFILE *) NONNULLARG1;
#endif
#ifndef MICRO
extern void set_error_savefile(void);
#endif
extern NHFILE *create_savefile(void);
extern NHFILE *open_savefile(void);
extern int delete_savefile(void);
extern NHFILE *restore_saved_game(void);
extern int check_panic_save(void);
#ifdef SELECTSAVED
extern char *plname_from_file(const char *, boolean) NONNULLARG1;
#endif
extern char **get_saved_games(void);
extern void free_saved_games(char **);
extern void nh_compress(const char *);
extern void nh_uncompress(const char *);
extern boolean lock_file(const char *, int, int) NONNULLARG1;
extern void unlock_file(const char *) NONNULLARG1;
extern int do_write_config_file(void);
extern boolean parse_config_line(char *) NONNULLARG1;
#ifdef USER_SOUNDS
extern boolean can_read_file(const char *) NONNULLARG1;
#endif
extern void config_error_init(boolean, const char *, boolean);
extern void config_erradd(const char *);
extern int config_error_done(void);
/* arg1 of read_config_file can be NULL to pass through
 * to fopen_config_file() to mean 'use the default config file name' */
extern boolean read_config_file(const char *, int);
extern void check_recordfile(const char *);
extern void read_wizkit(void);
extern boolean parse_conf_str(const char *str, boolean (*proc)(char *));
extern int read_sym_file(int);
extern void paniclog(const char *, const char *) NONNULLPTRS;
extern void testinglog(const char *, const char *, const char *);
extern int validate_prefix_locations(char *);
#ifdef SELF_RECOVER
extern boolean recover_savefile(void);
extern void assure_syscf_file(void);
#endif
#ifdef SYSCF_FILE
extern void assure_syscf_file(void);
#endif
extern int nhclose(int);
#ifdef DEBUG
extern boolean debugcore(const char *, boolean);
#endif
extern void reveal_paths(void);
extern boolean read_tribute(const char *, const char *, int, char *, int,
                            unsigned);
extern boolean Death_quote(char *, int) NONNULLARG1;
extern void livelog_add(long ll_type, const char *) NONNULLARG2;

/* ### fountain.c ### */

extern void floating_above(const char *) NONNULLARG1;
extern void dogushforth(int);
extern void dryup(coordxy, coordxy, boolean);
extern void drinkfountain(void);
extern void dipfountain(struct obj *) NONNULLARG1;
extern int wash_hands(void);
extern void breaksink(coordxy, coordxy);
extern void drinksink(void);
extern void dipsink(struct obj *) NONNULLARG1;
extern void sink_backs_up(coordxy, coordxy);

/* ### getpos.c ### */

extern char *dxdy_to_dist_descr(coordxy, coordxy, boolean);
extern char *coord_desc(coordxy, coordxy, char *, char) NONNULLARG3;
extern void auto_describe(coordxy, coordxy);
extern boolean getpos_menu(coord *, int) NONNULLARG1;
extern int getpos(coord *, boolean, const char *) NONNULLARG1;
extern void getpos_sethilite(void(*f)(boolean), boolean(*d)(coordxy,coordxy));
extern boolean mapxy_valid(coordxy, coordxy);
extern boolean gather_locs_interesting(coordxy, coordxy, int);

/* ### glyphs.c ### */

extern int glyphrep_to_custom_map_entries(const char *op,
                                          int *glyph) NONNULLPTRS;
extern int add_custom_urep_entry(const char *symset_name, int glyphidx,
                                 uint32 utf32ch, const uint8 *utf8str,
                                 enum graphics_sets which_set) NONNULLARG1;
extern int add_custom_nhcolor_entry(const char *customization_name,
                                    int glyphidx, uint32 nhcolor,
                                    enum graphics_sets which_set) NONNULLARG1;
struct customization_detail *find_matching_customization(
                                             const char *customization_name,
                                             enum customization_types custtype,
                                             enum graphics_sets which_set);
int set_map_customcolor(glyph_map *gm, uint32 nhcolor) NONNULLARG1;
extern int unicode_val(const char *);
extern int glyphrep(const char *) NONNULLARG1;
extern int match_glyph(char *) NONNULLARG1;
extern void dump_all_glyphids(FILE *fp) NONNULLARG1;
extern void wizcustom_glyphids(winid win);
extern void fill_glyphid_cache(void);
extern void free_glyphid_cache(void);
extern boolean glyphid_cache_status(void);
extern void apply_customizations(enum graphics_sets which_set,
                                 enum do_customizations docustomize);
extern void purge_custom_entries(enum graphics_sets which_set);
extern void purge_all_custom_entries(void);
extern void dump_glyphids(void);
extern void clear_all_glyphmap_colors(void);
extern void reset_customcolors(void);
extern int glyph_to_cmap(int);

/* ### hack.c ### */

extern boolean is_valid_travelpt(coordxy, coordxy);
extern anything *uint_to_any(unsigned);
extern anything *long_to_any(long);
extern anything *monst_to_any(struct monst *) NONNULLARG1;
extern anything *obj_to_any(struct obj *) NONNULLARG1;
extern boolean revive_nasty(coordxy, coordxy, const char *);
extern int still_chewing(coordxy, coordxy);
extern void movobj(struct obj *, coordxy, coordxy);
extern boolean may_dig(coordxy, coordxy);
extern boolean may_passwall(coordxy, coordxy);
extern boolean bad_rock(struct permonst *, coordxy, coordxy) NONNULLARG1;
extern int cant_squeeze_thru(struct monst *) NONNULLARG1;
extern boolean invocation_pos(coordxy, coordxy);
extern boolean test_move(coordxy, coordxy, coordxy, coordxy, int);
#ifdef DEBUG
extern int wiz_debug_cmd_traveldisplay(void);
#endif
extern boolean u_rooted(void);
extern void notice_mon(struct monst *) NONNULLARG1;
extern void notice_all_mons(boolean);
extern void impact_disturbs_zombies(struct obj *, boolean) NONNULLARG1;
extern void disturb_buried_zombies(coordxy, coordxy);
extern boolean u_maybe_impaired(void);
extern const char *u_locomotion(const char *) NONNULLARG1;
extern boolean handle_tip(int);
extern void domove(void);
extern void runmode_delay_output(void);
extern void overexert_hp(void);
extern boolean overexertion(void);
extern void invocation_message(void);
extern void switch_terrain(void);
extern void set_uinwater(int);
extern boolean pooleffects(boolean);
extern void spoteffects(boolean);
extern char *in_rooms(coordxy, coordxy, int);
extern boolean in_town(coordxy, coordxy);
extern void check_special_room(boolean);
extern int dopickup(void);
extern void lookaround(void);
extern boolean crawl_destination(coordxy, coordxy);
extern int monster_nearby(void);
extern void end_running(boolean);
extern void nomul(int);
extern void unmul(const char *);
extern int saving_grace(int);
extern void showdamage(int);
extern void losehp(int, const char *, schar) ;
extern int weight_cap(void);
extern int inv_weight(void);
extern int near_capacity(void);
extern int calc_capacity(int);
extern int max_capacity(void);
extern boolean check_capacity(const char *);
extern int inv_cnt(boolean);
/* sometimes money_cnt(gi.invent) which can be null */
extern long money_cnt(struct obj *) NO_NNARGS;
extern void spot_checks(coordxy, coordxy, schar);
extern int rounddiv(long, int);

/* ### strutil.c ### */

extern void strbuf_init(strbuf_t *) NONNULLARG1;
extern void strbuf_append(strbuf_t *, const char *) NONNULLPTRS;
extern void strbuf_reserve(strbuf_t *, int) NONNULLARG1;
extern void strbuf_empty(strbuf_t *) NONNULLARG1;
extern void strbuf_nl_to_crlf(strbuf_t *) NONNULLARG1;
extern unsigned Strlen_(const char *, const char *, int) NONNULLPTRS;
extern boolean pmatch(const char *, const char *) NONNULLPTRS;
extern boolean pmatchi(const char *, const char *) NONNULLPTRS;
/*
extern boolean pmatchz(const char *, const char *) NONNULLPTRS;
*/

/* ### insight.c ### */

extern int doattributes(void);
extern void enlightenment(int, int);
extern void youhiding(boolean, int);
extern char *trap_predicament(char *, int, boolean) NONNULLARG1;
extern int doconduct(void);
extern void show_conduct(int);
extern void record_achievement(schar);
extern boolean remove_achievement(schar);
extern int count_achievements(void);
extern schar achieve_rank(int);
extern boolean sokoban_in_play(void);
extern int do_gamelog(void);
extern void show_gamelog(int);
extern int set_vanq_order(boolean);
extern int dovanquished(void);
extern int doborn(void);
extern void list_vanquished(char, boolean);
extern int num_genocides(void);
extern void list_genocided(char, boolean);
extern int dogenocided(void);
extern const char *align_str(aligntyp);
extern char *piousness(boolean, const char *);
extern void mstatusline(struct monst *) NONNULLARG1;
extern void ustatusline(void);

/* ### invent.c ### */

extern void loot_classify(Loot *, struct obj *) NONNULLPTRS;
extern Loot *sortloot(struct obj **, unsigned, boolean,
                      boolean(*)(struct obj *)) NONNULLARG1;
extern void unsortloot(Loot **) NONNULLARG1;
extern void assigninvlet(struct obj *) NONNULLARG1;
extern struct obj *merge_choice(struct obj *, struct obj *) NONNULLARG2;
extern int merged(struct obj **, struct obj **) NONNULLPTRS;
extern void addinv_core1(struct obj *) NONNULLARG1;
extern void addinv_core2(struct obj *) NONNULLARG1;
extern struct obj *addinv(struct obj *) NONNULLARG1;
extern struct obj *addinv_before(struct obj *, struct obj *) NONNULLARG1;
extern struct obj *addinv_nomerge(struct obj *) NONNULLARG1;
extern struct obj *hold_another_object(struct obj *, const char *,
                                       const char *, const char *) NONNULLARG1;
/* nhlua.c calls useupall(gi.invent), but checks gi.invent against NULL
 * before doing so. useupall() won't handle NULL*/
extern void useupall(struct obj *) NONNULLARG1;
extern void useup(struct obj *) NONNULLARG1;
extern void consume_obj_charge(struct obj *, boolean) NONNULLARG1;
extern void freeinv_core(struct obj *) NONNULLARG1;
extern void freeinv(struct obj *) NONNULLARG1;
extern void delallobj(coordxy, coordxy);
extern void delobj(struct obj *) NONNULLARG1;
extern void delobj_core(struct obj *, boolean) NONNULLARG1;
extern struct obj *sobj_at(int, coordxy, coordxy);
extern struct obj *nxtobj(struct obj *, int, boolean) NONNULLARG1;
extern struct obj *carrying(int);
extern struct obj *u_carried_gloves(void);
extern struct obj *u_have_novel(void);
extern struct obj *o_on(unsigned int, struct obj *);
extern boolean obj_here(struct obj *, coordxy, coordxy) NONNULLARG1;
extern boolean wearing_armor(void);
extern boolean is_worn(struct obj *) NONNULLARG1;
extern boolean is_inuse(struct obj *) NONNULLARG1;
extern struct obj *g_at(coordxy, coordxy);
extern boolean splittable(struct obj *) NONNULLARG1;
extern int any_obj_ok(struct obj *);
extern struct obj *getobj(const char *, int(*)(struct obj *), unsigned int);
extern int ggetobj(const char *, int(*)(struct obj *), int, boolean,
                   unsigned *) NONNULLARG1;
extern int askchain(struct obj **, const char *, int, int(*)(struct obj *),
                    int(*)(struct obj *), int, const char *) NONNULLARG17;
extern void set_cknown_lknown(struct obj *) NONNULLARG1;
extern void fully_identify_obj(struct obj *) NONNULLARG1;
extern int identify(struct obj *) NONNULLARG1;
extern int count_unidentified(struct obj *) NO_NNARGS;
extern void identify_pack(int, boolean);
extern void learn_unseen_invent(void);
extern void update_inventory(void);
extern int doperminv(void);
extern void prinv(const char *, struct obj *, long) NONNULLARG2;
extern char *xprname(struct obj *, const char *, char, boolean, long, long);
extern int ddoinv(void);
extern char display_inventory(const char *, boolean);
extern int display_binventory(coordxy, coordxy, boolean);
extern struct obj *display_cinventory(struct obj *) NONNULLARG1;
extern struct obj *display_minventory(struct monst *, int, char *) NONNULLARG1;
extern int dotypeinv(void);
extern const char *dfeature_at(coordxy, coordxy, char *) NONNULLARG3;
extern int look_here(int, unsigned);
extern int dolook(void);
extern boolean will_feel_cockatrice(struct obj *, boolean) NONNULLARG1;
extern void feel_cockatrice(struct obj *, boolean) NONNULLARG1;
extern void stackobj(struct obj *) NONNULLARG1;
extern boolean mergable(struct obj *, struct obj *) NONNULLPTRS;
extern int doprgold(void);
extern int doprwep(void);
extern int doprarm(void);
extern int doprring(void);
extern int dopramulet(void);
extern int doprtool(void);
extern int doprinuse(void);
extern void useupf(struct obj *, long) NONNULLARG1;
extern char *let_to_name(char, boolean, boolean);
extern void free_invbuf(void);
extern void reassign(void);
extern boolean check_invent_gold(const char *) NONNULLARG1;
extern int doorganize(void);
extern int adjust_split(void);
extern void free_pickinv_cache(void);
/* sometimes count_unpaid(gi.invent) which can be null */
extern int count_unpaid(struct obj *) NO_NNARGS;
extern int count_buc(struct obj *, int, boolean(*)(struct obj *));
extern void tally_BUCX(struct obj *, boolean, int *, int *, int *, int *,
                       int *, int *);
extern long count_contents(struct obj *, boolean,
                           boolean, boolean, boolean) NONNULLARG1;
extern void carry_obj_effects(struct obj *) NONNULLARG1;
extern const char *currency(long);
extern void silly_thing(const char *, struct obj *) NONNULLARG1;
extern void sync_perminvent(void);
extern void perm_invent_toggled(boolean negated);
extern void prepare_perminvent(winid window);

/* ### ioctl.c ### */

#if defined(UNIX) || defined(__BEOS__)
extern void getwindowsz(void);
extern void getioctls(void);
extern void setioctls(void);
#ifdef SUSPEND
extern int dosuspend(void);
#endif /* SUSPEND */
#endif /* UNIX || __BEOS__ */

/* ### light.c ### */

extern void new_light_source(coordxy, coordxy,
                             int, int, union any *) NONNULLPTRS;
extern void del_light_source(int, union any *) NONNULLARG2;
extern void do_light_sources(seenV **) NONNULLARG1;
extern void show_transient_light(struct obj *, coordxy, coordxy);
extern void transient_light_cleanup(void);
extern struct monst *find_mid(unsigned, unsigned);
extern void save_light_sources(NHFILE *, int);
extern void restore_light_sources(NHFILE *) NONNULLARG1;
extern void light_stats(const char *, char *, long *, long *) NONNULLPTRS;
extern void relink_light_sources(boolean);
extern void light_sources_sanity_check(void);
extern void obj_move_light_source(struct obj *, struct obj *) NONNULLARG12;
extern boolean any_light_source(void);
extern void snuff_light_source(coordxy, coordxy);
extern boolean obj_sheds_light(struct obj *) NONNULLARG1;
extern boolean obj_is_burning(struct obj *) NONNULLARG1;
extern void obj_split_light_source(struct obj *, struct obj *) NONNULLARG12;
extern void obj_merge_light_sources(struct obj *, struct obj *) NONNULLARG12;
extern void obj_adjust_light_radius(struct obj *, int) NONNULLARG1;
extern int candle_light_range(struct obj *) NONNULLARG1;
extern int arti_light_radius(struct obj *) NONNULLARG1;
extern const char *arti_light_description(struct obj *) NONNULLARG1;
extern int wiz_light_sources(void);

/* ### lock.c ### */

extern boolean picking_lock(coordxy *, coordxy *);
extern boolean picking_at(coordxy, coordxy);
extern void breakchestlock(struct obj *, boolean) NONNULLARG1;
extern void reset_pick(void);
extern void maybe_reset_pick(struct obj *);
extern struct obj *autokey(boolean);
extern int pick_lock(struct obj *, coordxy, coordxy, struct obj *);
extern boolean u_have_forceable_weapon(void);
extern int doforce(void);
extern boolean boxlock(struct obj *, struct obj *) NONNULLARG12;
extern boolean doorlock(struct obj *, coordxy, coordxy) NONNULLARG1;
extern int doopen(void);
extern boolean stumble_on_door_mimic(coordxy, coordxy);
extern int doopen_indir(coordxy, coordxy);
extern int doclose(void);

#ifdef MAC
/* outdated functions removed */
/* ### macfile.c ### */
/* ### macmain.c ### */
/* ### macunix.c ### */
/* ### macwin.c ### */
/* ### mttymain.c ### */
#endif

/* ### mail.c ### */

#ifdef MAIL
#ifdef UNIX
extern void free_maildata(void);
extern void getmailstatus(void);
extern void ck_server_admin_msg(void);
#endif
extern void ckmailstatus(void);
extern void readmail(struct obj *);
#endif /* MAIL */

/* ### makemon.c ### */

extern boolean is_home_elemental(struct permonst *) NONNULLARG1;
extern struct monst *clone_mon(struct monst *, coordxy, coordxy) NONNULLARG1;
extern int monhp_per_lvl(struct monst *) NONNULLARG1;
extern void newmonhp(struct monst *, int) NONNULLARG1;
extern struct mextra *newmextra(void) NONNULL;
extern struct monst *makemon(struct permonst *, coordxy, coordxy, mmflags_nht);
extern struct monst *unmakemon(struct monst *, mmflags_nht) NONNULLARG1;
extern boolean create_critters(int, struct permonst *, boolean);
extern struct permonst *rndmonst_adj(int, int);
extern struct permonst *rndmonst(void);
extern struct permonst *mkclass(char, int);
extern struct permonst *mkclass_aligned(char, int, aligntyp);
extern int mkclass_poly(int);
extern int adj_lev(struct permonst *) NONNULLARG1;
extern struct permonst *grow_up(struct monst *, struct monst *) NONNULLARG1;
extern struct obj* mongets(struct monst *, int) NONNULLARG1;
extern int golemhp(int);
extern boolean peace_minded(struct permonst *) NONNULLARG1;
extern void set_malign(struct monst *) NONNULLARG1;
extern void newmcorpsenm(struct monst *) NONNULLARG1;
extern void freemcorpsenm(struct monst *) NONNULLARG1;
extern void set_mimic_sym(struct monst *) NO_NNARGS; /* tests for NULL mtmp */
extern int mbirth_limit(int);
extern void mkmonmoney(struct monst *, long) NONNULLARG1;
extern int bagotricks(struct obj *, boolean, int *);
extern boolean propagate(int, boolean, boolean);
extern void summon_furies(int);

/* ### mcastu.c ### */

extern int castmu(struct monst *, struct attack *,
                  boolean, boolean) NONNULLARG12;
extern void touch_of_death(struct monst *) NONNULLARG1;
extern char *death_inflicted_by(char *, const char *,
                                struct monst *) NONNULLARG12;
extern int buzzmu(struct monst *, struct attack *) NONNULLARG12;

/* ### mdlib.c ### */

extern void runtime_info_init(void);
extern const char *do_runtime_info(int *) NO_NNARGS;
extern void release_runtime_info(void);
extern char *mdlib_version_string(char *, const char *) NONNULL NONNULLPTRS;

/* ### mhitm.c ### */

extern int fightm(struct monst *) NONNULLARG1;
extern int mdisplacem(struct monst *, struct monst *, boolean);
extern int mattackm(struct monst *, struct monst *);
extern boolean failed_grab(struct monst *, struct monst *,
                           struct attack *) NONNULLPTRS;
extern boolean engulf_target(struct monst *, struct monst *) NONNULLARG12;
extern int mon_poly(struct monst *, struct monst *, int) NONNULLARG12;
extern void paralyze_monst(struct monst *, int) NONNULLARG1;
extern int sleep_monst(struct monst *, int, int) NONNULLARG1;
extern void slept_monst(struct monst *) NONNULLARG1;
extern void xdrainenergym(struct monst *, boolean) NONNULLARG1;
extern long attk_protection(int);
extern void rustm(struct monst *, struct obj *);

/* ### mhitu.c ### */

extern void hitmsg(struct monst *, struct attack *) NONNULLARG12;
extern const char *mswings_verb(struct obj *, boolean) NONNULLARG1;
extern const char *mpoisons_subj(struct monst *, struct attack *) NONNULLARG12;
extern void u_slow_down(void);
extern struct monst *cloneu(void);
extern void expels(struct monst *, struct permonst *, boolean) NONNULLARG12;
extern struct attack *getmattk(struct monst *, struct monst *, int, int *,
                               struct attack *) NONNULLARG12;
extern int mattacku(struct monst *) NONNULLARG1;
boolean diseasemu(struct permonst *) NONNULLARG1;
boolean u_slip_free(struct monst *, struct attack *) NONNULLARG12;
extern int magic_negation(struct monst *) NONNULLARG1;
extern boolean gulp_blnd_check(void);
extern int gazemu(struct monst *, struct attack *) NONNULLARG12;
extern void mdamageu(struct monst *, int) NONNULLARG1;
extern int could_seduce(struct monst *, struct monst *, struct attack *) NONNULLARG12;
extern int doseduce(struct monst *) NONNULLARG1;

/* ### minion.c ### */

extern void newemin(struct monst *) NONNULLARG1;
extern void free_emin(struct monst *) NONNULLARG1;
extern int monster_census(boolean);
extern int msummon(struct monst *);
extern void summon_minion(aligntyp, boolean);
extern int demon_talk(struct monst *) NONNULLARG1;
extern long bribe(struct monst *) NONNULLARG1;
extern int dprince(aligntyp);
extern int dlord(aligntyp);
extern int llord(void);
extern int ndemon(aligntyp);
extern int lminion(void);
extern void lose_guardian_angel(struct monst *);
extern void gain_guardian_angel(void);

/* ### mklev.c ### */

extern void sort_rooms(void);
extern void add_room(int, int, int, int, boolean, schar, boolean);
extern void add_subroom(struct mkroom *, int, int, int, int, boolean, schar,
                        boolean) NONNULLARG1;
extern void free_luathemes(enum lua_theme_group);
extern void makecorridors(void);
extern void add_door(coordxy, coordxy, struct mkroom *) NONNULLARG3;
extern void count_level_features(void);
extern void clear_level_structures(void);
extern void level_finalize_topology(void);
extern void mklev(void);
#ifdef SPECIALIZATION
extern void topologize(struct mkroom *, boolean) NONNULLARG1;
#else
extern void topologize(struct mkroom *) NONNULLARG1;
#endif
/* place_branch() has tests for NULL branch arg, preventing NONNULLARG1 */
extern void place_branch(branch *, coordxy, coordxy) NO_NNARGS;
extern boolean occupied(coordxy, coordxy);
extern int okdoor(coordxy, coordxy);
extern boolean maybe_sdoor(int);
extern void dodoor(coordxy, coordxy, struct mkroom *) NONNULLARG3;
extern void mktrap(int, unsigned, struct mkroom *, coord *) NO_NNARGS;
extern void mkstairs(coordxy, coordxy, char, struct mkroom *, boolean);
extern void mkinvokearea(void);
extern void mineralize(int, int, int, int, boolean);

/* ### mkmap.c ### */

extern void flood_fill_rm(int, int, int, boolean, boolean);
extern void remove_rooms(int, int, int, int);
extern boolean litstate_rnd(int);

/* ### mkmaze.c ### */

extern boolean set_levltyp(coordxy, coordxy, schar);
extern boolean set_levltyp_lit(coordxy, coordxy, schar, schar);
extern void create_maze(int, int, boolean);
extern void wallification(coordxy, coordxy, coordxy, coordxy);
extern void fix_wall_spines(coordxy, coordxy, coordxy, coordxy);
extern void walkfrom(coordxy, coordxy, schar);
extern void pick_vibrasquare_location(void);
extern void makemaz(const char *) NONNULLARG1;
extern void mazexy(coord *) NONNULLARG1;
extern void get_level_extends(coordxy *, coordxy *, coordxy *, coordxy *) NONNULLPTRS;
extern void bound_digging(void);
extern void mkportal(coordxy, coordxy, xint16, xint16);
extern boolean bad_location(coordxy, coordxy, coordxy, coordxy, coordxy,
                            coordxy);
extern boolean is_exclusion_zone(xint16, coordxy, coordxy);
/* dungeon.c u_on_rndspot() passes NULL final arg to place_lregion() */
extern void place_lregion(coordxy, coordxy, coordxy, coordxy, coordxy,
                          coordxy, coordxy, coordxy, xint16, d_level *) NO_NNARGS;
extern void fixup_special(void);
extern void fumaroles(void);
extern void movebubbles(void);
extern void water_friction(void);
extern void save_waterlevel(NHFILE *) NONNULLARG1;
extern void restore_waterlevel(NHFILE *) NONNULLARG1;
extern void maybe_adjust_hero_bubble(void);

/* ### mkobj.c ### */

extern struct oextra *newoextra(void) NONNULL;
extern void copy_oextra(struct obj *, struct obj *);
extern void dealloc_oextra(struct obj *) NONNULLARG1;
extern void newomonst(struct obj *) NONNULLARG1;
extern void free_omonst(struct obj *) NONNULLARG1;
extern void newomid(struct obj *) NONNULLARG1;
extern void free_omid(struct obj *) NONNULLARG1;
/*
extern void newolong(struct obj *);
extern void free_olong(struct obj *);
*/
extern void new_omailcmd(struct obj *, const char *) NONNULLPTRS;
extern void free_omailcmd(struct obj *) NONNULLARG1;
extern struct obj *mkobj_at(char, coordxy, coordxy, boolean);
extern struct obj *mksobj_at(int, coordxy, coordxy, boolean, boolean);
extern struct obj *mksobj_migr_to_species(int, unsigned, boolean, boolean);
extern struct obj *mkobj(int, boolean) NONNULL;
extern int rndmonnum_adj(int, int);
extern int rndmonnum(void);
extern boolean bogon_is_pname(char);
extern struct obj *splitobj(struct obj *, long) NONNULLARG1;
extern unsigned next_ident(void);
extern struct obj *unsplitobj(struct obj *) NONNULLARG1;
extern void clear_splitobjs(void);
extern void replace_object(struct obj *, struct obj *) NONNULLARG12;
extern struct obj *unknwn_contnr_contents(struct obj *) NONNULLARG1;
extern void bill_dummy_object(struct obj *) NONNULLARG1;
extern void costly_alteration(struct obj *, int) NONNULLARG1;
extern void clear_dknown(struct obj *);
extern void unknow_object(struct obj *);
extern struct obj *mksobj(int, boolean, boolean) NONNULL;
extern int bcsign(struct obj *) NONNULLARG1;
extern int weight(struct obj *) NONNULLARG1;
extern struct obj *mkgold(long, coordxy, coordxy);
extern void fixup_oil(struct obj *, struct obj *) NONNULLARG1;
extern struct obj *mkcorpstat(int, struct monst *, struct permonst *,
                              coordxy, coordxy, unsigned) NONNULL;
extern int corpse_revive_type(struct obj *) NONNULLARG1;
extern struct obj *obj_attach_mid(struct obj *, unsigned);
extern struct monst *get_mtraits(struct obj *, boolean) NONNULLARG1;
extern struct obj *mk_tt_object(int, coordxy, coordxy);
extern struct obj *mk_named_object(int, struct permonst *,
                                   coordxy, coordxy,
                                   const char *) ;
extern struct obj *rnd_treefruit_at(coordxy, coordxy);
extern void set_corpsenm(struct obj *, int) NONNULLARG1;
extern long rider_revival_time(struct obj *, boolean) NONNULLARG1;
extern void start_corpse_timeout(struct obj *) NONNULLARG1;
extern void start_glob_timeout(struct obj *, long) NONNULLARG1;
extern void shrink_glob(anything *, long) NONNULLARG1;
extern void maybe_adjust_light(struct obj *, int) NONNULLARG1;
extern void bless(struct obj *) NONNULLARG1;
extern void unbless(struct obj *) NONNULLARG1;
extern void curse(struct obj *) NONNULLARG1;
extern void uncurse(struct obj *) NONNULLARG1;
extern void blessorcurse(struct obj *, int) NONNULLARG1;
extern void set_bknown(struct obj *, unsigned) NONNULLARG1;
extern boolean is_flammable(struct obj *) NONNULLARG1;
extern boolean is_rottable(struct obj *) NONNULLARG1;
extern void place_object(struct obj *, coordxy, coordxy) NONNULLARG1;
extern void recreate_pile_at(coordxy, coordxy);
extern void remove_object(struct obj *) NONNULLARG1;
extern void discard_minvent(struct monst *, boolean) NONNULLARG1;
extern void obj_extract_self(struct obj *) NONNULLARG1;
extern void extract_nobj(struct obj *, struct obj **) NONNULLARG12;
extern void extract_nexthere(struct obj *, struct obj **) NONNULLARG12;
extern int add_to_minv(struct monst *, struct obj *) NONNULLARG12;
extern struct obj *add_to_container(struct obj *, struct obj *) NONNULLARG12;
extern void add_to_migration(struct obj *) NONNULLARG1;
extern void add_to_buried(struct obj *) NONNULLARG1;
extern void container_weight(struct obj *) NONNULLARG1;
extern void dealloc_obj(struct obj *) NONNULLARG1;
extern void obj_ice_effects(coordxy, coordxy, boolean);
extern long peek_at_iced_corpse_age(struct obj *) NONNULLARG1;
extern void dobjsfree(void);
extern int hornoplenty(struct obj *, boolean, struct obj *);
extern void obj_sanity_check(void);
extern struct obj *obj_nexto(struct obj *);
extern struct obj *obj_nexto_xy(struct obj *, coordxy, coordxy, boolean) NONNULLARG1;
extern struct obj *obj_absorb(struct obj **, struct obj **);
extern struct obj *obj_meld(struct obj **, struct obj **);
extern void pudding_merge_message(struct obj *, struct obj *) NONNULLARG12;
extern struct obj *init_dummyobj(struct obj *, short, long);

/* ### mkroom.c ### */

extern void do_mkroom(int);
extern void fill_zoo(struct mkroom *) NONNULLARG1;
extern struct permonst *antholemon(void);
extern boolean nexttodoor(int, int);
extern boolean has_dnstairs(struct mkroom *) NONNULLARG1;
extern boolean has_upstairs(struct mkroom *) NONNULLARG1;
extern int somex(struct mkroom *) NONNULLARG1;
extern int somey(struct mkroom *) NONNULLARG1;
extern boolean inside_room(struct mkroom *, coordxy, coordxy) NONNULLARG1;
extern boolean somexy(struct mkroom *, coord *) NONNULLARG12;
extern boolean somexyspace(struct mkroom *, coord *) NONNULLARG12;
extern void mkundead(coord *, boolean, int) NONNULLARG1;
extern struct permonst *courtmon(void);
extern void save_rooms(NHFILE *) NONNULLARG1;
extern void rest_rooms(NHFILE *) NONNULLARG1;
extern struct mkroom *search_special(schar);
extern int cmap_to_type(int);

/* ### mon.c ### */

extern void dealloc_monst(struct monst *) NONNULLARG1;
extern void copy_mextra(struct monst *, struct monst *);
extern void dealloc_mextra(struct monst *) NONNULLARG1;
extern void mon_sanity_check(void);
extern boolean zombie_maker(struct monst *) NONNULLARG1;
extern int zombie_form(struct permonst *) NONNULLARG1;
extern int m_poisongas_ok(struct monst *) NONNULLARG1;
extern int undead_to_corpse(int);
extern int genus(int, int);
extern int pm_to_cham(int);
extern int minliquid(struct monst *) NONNULLARG1;
extern boolean movemon_singlemon(struct monst *) NONNULLARG1;
extern int movemon(void);
extern void meatbox(struct monst *, struct obj *) NONNULLPTRS;
extern void m_consume_obj(struct monst *, struct obj *) NONNULLPTRS;
extern int meatmetal(struct monst *) NONNULLARG1;
extern int meatobj(struct monst *) NONNULLARG1;
extern int meatcorpse(struct monst *) NONNULLARG1;
extern void mon_give_prop(struct monst *, int) NONNULLARG1;
extern void mon_givit(struct monst *, struct permonst *) NONNULLARG12;
extern void mpickgold(struct monst *) NONNULLARG1;
extern boolean mpickstuff(struct monst *) NONNULLARG1;
extern int curr_mon_load(struct monst *) NONNULLARG1;
extern int max_mon_load(struct monst *) NONNULLARG1;
extern boolean can_touch_safely(struct monst *, struct obj *) NONNULLARG12;
extern int can_carry(struct monst *, struct obj *) NONNULLARG12;
extern long mon_allowflags(struct monst *) NONNULLARG1;
extern boolean m_in_air(struct monst *) NONNULLARG1;
extern int mfndpos(struct monst *, coord *, long *, long) NONNULLPTRS;
extern boolean monnear(struct monst *, coordxy, coordxy) NONNULLARG1;
extern void dmonsfree(void);
extern void elemental_clog(struct monst *) NONNULLARG1;
extern int mcalcmove(struct monst *, boolean) NONNULLARG1;
extern void mcalcdistress(void);
extern void replmon(struct monst *, struct monst *) NONNULLARG12;
extern void relmon(struct monst *, struct monst **) NONNULLARG1;
extern struct obj *mlifesaver(struct monst *) NONNULLARG1;
extern boolean corpse_chance(struct monst *, struct monst *, boolean) NONNULLARG1;
extern void mondead(struct monst *) NONNULLARG1;
extern void mondied(struct monst *) NONNULLARG1;
extern void mongone(struct monst *) NONNULLARG1;
extern void monstone(struct monst *) NONNULLARG1;
extern void monkilled(struct monst *, const char *, int) NONNULLARG1;
extern void set_ustuck(struct monst *);
extern void unstuck(struct monst *) NONNULLARG1;
extern void killed(struct monst *) NONNULLARG1;
extern void xkilled(struct monst *, int) NONNULLARG1;
extern void mon_to_stone(struct monst *) NONNULLARG1;
extern void m_into_limbo(struct monst *) NONNULLARG1;
extern void migrate_mon(struct monst *, xint16, xint16) NONNULLARG1;
extern void mnexto(struct monst *, unsigned) NONNULLARG1;
extern void deal_with_overcrowding(struct monst *) NONNULLARG1;
extern void maybe_mnexto(struct monst *) NONNULLARG1;
extern int mnearto(struct monst *, coordxy, coordxy, boolean, unsigned) NONNULLARG1;
extern void m_respond(struct monst *) NONNULLARG1;
extern void setmangry(struct monst *, boolean) NONNULLARG1;
extern void wake_msg(struct monst *, boolean) NONNULLARG1;
extern void wakeup(struct monst *, boolean) NONNULLARG1;
extern void wake_nearby(boolean);
extern void wake_nearto(coordxy, coordxy, int);
extern void seemimic(struct monst *) NONNULLARG1;
extern void normal_shape(struct monst *) NONNULLARG1;
extern void alloc_itermonarr(unsigned);
extern void iter_mons_safe(boolean (*)(struct monst *));
extern void iter_mons(void (*)(struct monst *));
extern struct monst *get_iter_mons(boolean (*)(struct monst *));
extern struct monst *get_iter_mons_xy(boolean (*)(struct monst *,
                                                  coordxy, coordxy),
                                      coordxy, coordxy);
extern void rescham(void);
extern void restartcham(void);
extern void restore_cham(struct monst *) NONNULLARG1;
extern void maybe_unhide_at(coordxy, coordxy);
extern boolean hideunder(struct monst *) NONNULLARG1;
extern void hide_monst(struct monst *) NONNULLARG1;
extern void mon_animal_list(boolean);
extern boolean valid_vampshiftform(int, int);
extern boolean validvamp(struct monst *, int *, int) NONNULLARG12;
extern int select_newcham_form(struct monst *) NONNULLARG1;
extern void mgender_from_permonst(struct monst *, struct permonst *) NONNULLARG12;
extern int newcham(struct monst *, struct permonst *, unsigned) NONNULLARG1;
extern int can_be_hatched(int);
extern int egg_type_from_parent(int, boolean);
extern boolean dead_species(int, boolean);
extern void kill_genocided_monsters(void);
extern void golemeffects(struct monst *, int, int);
extern boolean angry_guards(boolean);
extern void pacify_guards(void);
extern void decide_to_shapeshift(struct monst *) NONNULLARG1;
extern boolean vamp_stone(struct monst *) NONNULLARG1;
extern void check_gear_next_turn(struct monst *) NONNULLARG1;
extern void copy_mextra(struct monst *, struct monst *);
extern void dealloc_mextra(struct monst *);
extern boolean usmellmon(struct permonst *);
extern void mimic_hit_msg(struct monst *, short);
extern void adj_erinys(unsigned);

/* ### mondata.c ### */

extern void set_mon_data(struct monst *, struct permonst *) NONNULLARG12;
extern struct attack *attacktype_fordmg(struct permonst *, int, int) NONNULLARG1;
extern boolean attacktype(struct permonst *, int) NONNULLARG1;
extern boolean noattacks(struct permonst *) NONNULLARG1;
extern boolean poly_when_stoned(struct permonst *) NONNULLARG1;
extern boolean defended(struct monst *, int) NONNULLARG1;
extern boolean resists_drli(struct monst *) NONNULLARG1;
extern boolean resists_magm(struct monst *) NONNULLARG1;
extern boolean resists_blnd(struct monst *) NONNULLARG1;
extern boolean resists_blnd_by_arti(struct monst *) NONNULLARG1;
extern boolean can_blnd(struct monst *, struct monst *,
                        uchar, struct obj *) NONNULLARG2;
extern boolean ranged_attk(struct permonst *) NONNULLARG1;
extern boolean mon_hates_silver(struct monst *) NONNULLARG1;
extern boolean hates_silver(struct permonst *) NONNULLARG1;
extern boolean mon_hates_blessings(struct monst *) NONNULLARG1;
extern boolean hates_blessings(struct permonst *) NONNULLARG1;
extern boolean mon_hates_light(struct monst *) NONNULLARG1;
extern boolean passes_bars(struct permonst *) NONNULLARG1;
extern boolean can_blow(struct monst *) NONNULLARG1;
extern boolean can_chant(struct monst *) NONNULLARG1;
extern boolean can_be_strangled(struct monst *) NONNULLARG1;
extern boolean can_track(struct permonst *) NONNULLARG1;
extern boolean breakarm(struct permonst *) NONNULLARG1;
extern boolean sliparm(struct permonst *) NONNULLARG1;
extern boolean sticks(struct permonst *) NONNULLARG1;
extern boolean cantvomit(struct permonst *) NONNULLARG1;
extern int num_horns(struct permonst *) NONNULLARG1;
extern struct attack *dmgtype_fromattack(struct permonst *, int, int) NONNULLARG1;
extern boolean dmgtype(struct permonst *, int) NONNULLARG1;
extern int max_passive_dmg(struct monst *, struct monst *) NONNULLARG12;
extern boolean same_race(struct permonst *, struct permonst *) NONNULLARG12;
extern int name_to_mon(const char *, int *) NONNULLARG1;
extern int name_to_monplus(const char *, const char **, int *) NONNULLARG1;
extern int name_to_monclass(const char *, int *);
extern int gender(struct monst *) NONNULLARG1;
extern int pronoun_gender(struct monst *, unsigned) NONNULLARG1;
extern boolean levl_follower(struct monst *) NONNULLARG1;
extern int little_to_big(int);
extern int big_to_little(int);
extern boolean big_little_match(int, int);
extern const char *locomotion(const struct permonst *, const char *) NONNULLARG12;
extern const char *stagger(const struct permonst *, const char *) NONNULLARG12;
extern const char *on_fire(struct permonst *, struct attack *) NONNULLARG12;
extern const char *msummon_environ(struct permonst *, const char **) NONNULLARG12;
extern const struct permonst *raceptr(struct monst *) NONNULLARG1;
extern boolean olfaction(struct permonst *) NONNULLARG1;
unsigned long cvt_adtyp_to_mseenres(uchar);
unsigned long cvt_prop_to_mseenres(uchar);
extern void monstseesu(unsigned long);
extern void monstunseesu(unsigned long);
extern void give_u_to_m_resistances(struct monst *) NONNULLARG1;
extern boolean resist_conflict(struct monst *) NONNULLARG1;
extern boolean mon_knows_traps(struct monst *, int) NONNULLARG1;
extern void mon_learns_traps(struct monst *, int) NONNULLARG1;
extern void mons_see_trap(struct trap *) NONNULLARG1;
extern int get_atkdam_type(int);
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED) || defined(DEBUG)
extern int mstrength(struct permonst *) NONNULLARG1;
#endif

/* ### monmove.c ### */

extern boolean mon_would_take_item(struct monst *, struct obj *) NONNULLARG12;
extern boolean mon_would_consume_item(struct monst *, struct obj *) NONNULLARG12;
extern boolean itsstuck(struct monst *) NONNULLARG1;
extern boolean mb_trapped(struct monst *, boolean) NONNULLARG1;
extern void mon_track_add(struct monst *, coordxy, coordxy) NONNULLARG1;
extern void mon_track_clear(struct monst *) NONNULLARG1;
extern boolean monhaskey(struct monst *, boolean) NONNULLARG1;
extern void mon_regen(struct monst *, boolean) NONNULLARG1;
extern void m_everyturn_effect(struct monst *) NONNULLARG1;
extern void m_postmove_effect(struct monst *) NONNULLARG1;
extern int dochugw(struct monst *, boolean) NONNULLARG1;
extern boolean onscary(coordxy, coordxy, struct monst *) NONNULLARG3;
extern struct monst *find_pmmonst(int);
extern int bee_eat_jelly(struct monst *, struct obj *) NONNULLARG12;
extern void monflee(struct monst *, int, boolean, boolean) NONNULLARG1;
extern void mon_yells(struct monst *, const char *) NONNULLARG12;
extern boolean m_can_break_boulder(struct monst *) NONNULLARG1;
extern void m_break_boulder(struct monst *, coordxy, coordxy) NONNULLARG1;
extern int dochug(struct monst *) NONNULLARG1;
extern boolean m_digweapon_check(struct monst *, coordxy, coordxy) NONNULLARG1;
extern boolean m_avoid_kicked_loc(struct monst *, coordxy, coordxy) NONNULLARG1;
extern boolean m_avoid_soko_push_loc(struct monst *, coordxy, coordxy) NONNULLARG1;
extern int m_move(struct monst *, int) NONNULLARG1;
extern int m_move_aggress(struct monst *, coordxy, coordxy) NONNULLARG1;
extern void dissolve_bars(coordxy, coordxy);
extern boolean closed_door(coordxy, coordxy);
extern boolean accessible(coordxy, coordxy);
extern void set_apparxy(struct monst *) NONNULLARG1;
extern boolean can_ooze(struct monst *) NONNULLARG1;
extern boolean can_fog(struct monst *) NONNULLARG1;
extern boolean should_displace(struct monst *, coord *, long *, int, coordxy,
                               coordxy) NONNULLPTRS;
extern boolean undesirable_disp(struct monst *, coordxy, coordxy) NONNULLARG1;
extern boolean can_hide_under_obj(struct obj *);

/* ### monst.c ### */

extern void monst_globals_init(void);

/* ### mplayer.c ### */

extern struct monst *mk_mplayer(struct permonst *,
                                coordxy, coordxy, boolean) NONNULLARG1;
extern void create_mplayers(int, boolean);
extern void mplayer_talk(struct monst *) NONNULLARG1;

#if defined(MICRO) || defined(WIN32)

/* ### msdos.c,os2.c,tos.c,windsys.c ### */

#ifndef WIN32
extern int tgetch(void);
#endif
#ifndef TOS
extern char switchar(void);
#endif
#ifndef __GO32__
extern long freediskspace(char *);
#ifdef MSDOS
extern int findfirst_file(char *);
extern int findnext_file(void);
extern long filesize_nh(char *);
#else
extern int findfirst(char *);
extern int findnext(void);
extern long filesize(char *);
#endif /* MSDOS */
extern char *foundfile_buffer(void);
#endif /* __GO32__ */
extern void chdrive(char *);
#ifndef TOS
extern void disable_ctrlP(void);
extern void enable_ctrlP(void);
#endif
#if defined(MICRO) && !defined(WIN32)
extern void get_scr_size(void);
#ifndef TOS
extern void gotoxy(int, int);
#endif
#endif
#ifdef TOS
extern int _copyfile(char *, char *);
extern int kbhit(void);
extern void set_colors(void);
extern void restore_colors(void);
#ifdef SUSPEND
extern int dosuspend(void);
#endif
#endif /* TOS */
#ifdef WIN32
extern void nt_regularize(char *);
extern int(*nt_kbhit)(void);
extern void Delay(int);
# ifdef CRASHREPORT
struct CRctxt;
extern struct CRctxt *ctxp;
extern int win32_cr_helper(char, struct CRctxt *, void *, int);
extern int win32_cr_gettrace(int, char *, int);
extern int *win32_cr_shellexecute(const char *);
# endif
#endif /* WIN32 */

#endif /* MICRO || WIN32 */

/* ### mthrowu.c ### */

extern const char *rnd_hallublast(void);
extern boolean m_has_launcher_and_ammo(struct monst *) NONNULLARG1;
extern int thitu(int, int, struct obj **, const char *) NO_NNARGS;
extern boolean ohitmon(struct monst *, struct obj *,
                       int, boolean) NONNULLARG12;
extern void thrwmu(struct monst *) NONNULLARG1;
extern int spitmu(struct monst *, struct attack *) NONNULLPTRS;
extern int breamu(struct monst *, struct attack *) NONNULLPTRS;
extern boolean linedup_callback(coordxy, coordxy, coordxy, coordxy,
                                boolean(*)(coordxy, coordxy));
extern boolean linedup(coordxy, coordxy, coordxy, coordxy, int);
extern boolean lined_up(struct monst *) NONNULLARG1;
extern struct obj *m_carrying(struct monst *, int) NONNULLARG1;
extern int thrwmm(struct monst *, struct monst *) NONNULLARG12;
extern int spitmm(struct monst *, struct attack *, struct monst *) NONNULLPTRS;
extern int breamm(struct monst *, struct attack *, struct monst *) NONNULLPTRS;
extern void m_useupall(struct monst *, struct obj *) NONNULLARG12;
extern void m_useup(struct monst *, struct obj *) NONNULLARG12;
extern void m_throw(struct monst *, coordxy, coordxy, coordxy, coordxy,
                    int, struct obj *) NONNULLPTRS;
extern void hit_bars(struct obj **, coordxy, coordxy, coordxy, coordxy,
                     unsigned) NONNULLARG1;
extern boolean hits_bars(struct obj **, coordxy, coordxy, coordxy, coordxy,
                         int, int) NONNULLARG1;

/* ### muse.c ### */

extern boolean find_defensive(struct monst *, boolean) NONNULLARG1;
extern int use_defensive(struct monst *) NONNULLARG1;
extern int rnd_defensive_item(struct monst *) NONNULLARG1;
extern boolean find_offensive(struct monst *) NONNULLARG1;
extern int use_offensive(struct monst *) NONNULLARG1;
extern int rnd_offensive_item(struct monst *) NONNULLARG1;
extern boolean find_misc(struct monst *) NONNULLARG1;
extern int use_misc(struct monst *) NONNULLARG1;
extern int rnd_misc_item(struct monst *) NONNULLARG1;
extern boolean searches_for_item(struct monst *, struct obj *) NONNULLARG12;
extern boolean mon_reflects(struct monst *, const char *) NONNULLARG1;
extern boolean ureflects(const char *, const char *) NO_NNARGS;
extern void mcureblindness(struct monst *, boolean) NONNULLARG1;
extern boolean munstone(struct monst *, boolean) NONNULLARG1;
extern boolean munslime(struct monst *, boolean) NONNULLARG1;

/* ### music.c ### */

extern void awaken_soldiers(struct monst *) NONNULLARG1;
extern int do_play_instrument(struct obj *) NONNULLARG1;
enum instruments obj_to_instr(struct obj *) NONNULLARG1;

/* ### nhlsel.c ### */

#if !defined(CROSSCOMPILE) || defined(CROSSCOMPILE_TARGET)
extern struct selectionvar *l_selection_check(lua_State *, int) NONNULLARG1;
extern int l_selection_register(lua_State *) NONNULLARG1;
extern void l_selection_push_copy(lua_State *, struct selectionvar *) NONNULLARG12;
extern int l_obj_register(lua_State *) NONNULLARG1;
#endif

/* ### nhlobj.c ### */

#if !defined(CROSSCOMPILE) || defined(CROSSCOMPILE_TARGET)
extern void nhl_push_obj(lua_State *, struct obj *) NONNULLARG12;
extern int nhl_obj_u_giveobj(lua_State *) NONNULLARG1;
extern int l_obj_register(lua_State *) NONNULLARG1;
#endif

/* ### nhlua.c ### */

#if !defined(CROSSCOMPILE) || defined(CROSSCOMPILE_TARGET)
extern void l_nhcore_init(void);
extern void l_nhcore_done(void);
extern void l_nhcore_call(int);
extern lua_State * nhl_init(nhl_sandbox_info *) NONNULLARG1;
/* nhl_done contains a test for NULL arg1, preventing NONNULLARG1 */
extern void nhl_done(lua_State *) NO_NNARGS;
extern boolean nhl_loadlua(lua_State *, const char *) NONNULLARG12;
extern char *get_nh_lua_variables(void);
extern void save_luadata(NHFILE *) NONNULLARG1;
extern void restore_luadata(NHFILE *) NONNULLARG1;
extern int nhl_pcall(lua_State *, int, int, const char *) NONNULLARG1;
extern int nhl_pcall_handle(lua_State *, int, int, const char *,
                            NHL_pcall_action) NONNULLARG1;
extern boolean load_lua(const char *, nhl_sandbox_info *) NONNULLARG12;
ATTRNORETURN extern void nhl_error(lua_State *, const char *)
                                                        NORETURN NONNULLARG12;
extern void lcheck_param_table(lua_State *) NONNULLARG1;
extern schar get_table_mapchr(lua_State *, const char *) NONNULLARG12;
extern schar get_table_mapchr_opt(lua_State *, const char *, schar)
                                                                 NONNULLARG12;
extern short nhl_get_timertype(lua_State *, int) NONNULLARG1;
extern boolean nhl_get_xy_params(lua_State *, lua_Integer *, lua_Integer *)
                                                                NONNULLARG123;
extern void nhl_add_table_entry_int(lua_State *, const char *, lua_Integer)
                                                                 NONNULLARG12;
extern void nhl_add_table_entry_char(lua_State *, const char *, char)
                                                                 NONNULLARG12;
extern void nhl_add_table_entry_str(lua_State *, const char *, const char *)
                                                                NONNULLARG123;
extern void nhl_add_table_entry_bool(lua_State *, const char *, boolean)
                                                                 NONNULLARG12;
extern void nhl_add_table_entry_region(lua_State *, const char *,
                                       coordxy, coordxy, coordxy, coordxy)
                                                                 NONNULLARG12;
extern schar splev_chr2typ(char);
extern schar check_mapchr(const char *) NO_NNARGS;
extern int get_table_int(lua_State *, const char *) NONNULLARG12;
extern int get_table_int_opt(lua_State *, const char *, int) NONNULLARG12;
extern char *get_table_str(lua_State *, const char *) NONNULLARG12;
/* dungeon.c init_dungeon_levels() passes NULL to get_table_str_opt arg3 */
extern char *get_table_str_opt(lua_State *, const char *, char *) NONNULLARG12;
extern int get_table_boolean(lua_State *, const char *) NONNULLARG12;
extern int get_table_boolean_opt(lua_State *, const char *, int) NONNULLARG12;
/* lspo_feature calls get_table_option(L, "type", NULL, features),
   so arg3 can be NULL.  NONNULLARG124 is not currently defined */
extern int get_table_option(lua_State *, const char *, const char *,
                            const char *const *) NO_NNARGS;
/* extern int str_lines_max_width(const char *); */
extern const char *get_lua_version(void);
extern void nhl_pushhooked_open_table(lua_State *L) NONNULLARG1;
extern void free_tutorial(void);
extern void tutorial(boolean);
#endif /* !CROSSCOMPILE || CROSSCOMPILE_TARGET */

#endif /* MAKEDEFS_C MDLIB_C CPPREGEX_C */

/* ### {cpp,pmatch,posix}regex.c ### */

extern struct nhregex *regex_init(void);
extern boolean regex_compile(const char *, struct nhregex *) NONNULLARG1;
extern char *regex_error_desc(struct nhregex *, char *) NONNULLARG2;
extern boolean regex_match(const char *, struct nhregex *) NO_NNARGS;
extern void regex_free(struct nhregex *) NONNULLARG1;

#if !defined(MAKEDEFS_C) && !defined(MDLIB_C) && !defined(CPPREGEX_C)

/* ### consoletty.c ### */

#ifdef WIN32
extern void get_scr_size(void);
extern int consoletty_kbhit(void);
extern void consoletty_open(int);
extern void consoletty_rubout(void);
extern int tgetch(void);
extern int console_poskey(coordxy *, coordxy *, int *);
extern void set_output_mode(int);
extern void synch_cursor(void);
extern void nethack_enter_consoletty(void);
extern void consoletty_exit(void);
extern int set_keyhandling_via_option(void);
#ifdef ENHANCED_SYMBOLS
extern void tty_utf8graphics_fixup(void);
extern void tty_ibmgraphics_fixup(void);
#endif /* ENHANCED_SYMBOLS */
#endif /* WIN32 */

/* ### o_init.c ### */

extern void init_objects(void);
extern void init_oclass_probs(void);
extern void obj_shuffle_range(int, int *, int *) NONNULLPTRS;
/* objdescr_is() contains a test for NULL arg1, so can't be NONNULLARG12 */
extern boolean objdescr_is(struct obj *, const char *) NONNULLARG2;
extern void oinit(void);
extern void savenames(NHFILE *) NONNULLARG1;
extern void restnames(NHFILE *) NONNULLARG1;
extern void discover_object(int, boolean, boolean);
extern void undiscover_object(int);
extern boolean interesting_to_discover(int);
extern int choose_disco_sort(int);
extern int dodiscovered(void);
extern int doclassdisco(void);
extern void rename_disco(void);
extern void get_sortdisco(char *opts, boolean cnf) NONNULLARG1;

/* ### objects.c ### */

extern void objects_globals_init(void);

/* ### objnam.c ### */

extern void maybereleaseobuf(char *) NONNULLARG1;
extern char *obj_typename(int);
extern char *simple_typename(int);
extern char *safe_typename(int);
extern boolean obj_is_pname(struct obj *) NONNULLARG1;
extern char *distant_name(struct obj *, char *(*)(struct obj *)) NONNULLPTRS;
extern char *fruitname(boolean);
extern struct fruit *fruit_from_indx(int);
extern struct fruit *fruit_from_name(const char *, boolean, int *) NONNULLARG1;
extern void reorder_fruit(boolean);
extern char *xname(struct obj *) NONNULLARG1;
extern char *mshot_xname(struct obj *) NONNULLARG1;
extern boolean the_unique_obj(struct obj *) NONNULLARG1;
extern boolean the_unique_pm(struct permonst *) NONNULLARG1;
extern boolean erosion_matters(struct obj *) NONNULLARG1;
extern char *doname(struct obj *) NONNULLARG1;
extern char *doname_with_price(struct obj *) NONNULLARG1;
extern char *doname_vague_quan(struct obj *) NONNULLARG1;
extern boolean not_fully_identified(struct obj *) NONNULLARG1;
extern char *corpse_xname(struct obj *, const char *, unsigned) NONNULLARG1;
extern char *cxname(struct obj *) NONNULLARG1;
extern char *cxname_singular(struct obj *) NONNULLARG1;
extern char *killer_xname(struct obj *) NONNULLARG1;
extern char *short_oname(struct obj *, char *(*)(struct obj *),
                         char *(*)(struct obj *), unsigned) NONNULLARG12;
extern const char *singular(struct obj *, char *(*)(struct obj *)) NONNULLPTRS;
extern char *just_an(char *, const char *) NONNULL NONNULLARG12;
/* an(), the() contain tests for NULL arg, preventing NONNULLARG1 */
extern char *an(const char *) NONNULL NO_NNARGS;
extern char *An(const char *) NONNULL NO_NNARGS;
extern char *The(const char *) NONNULL NO_NNARGS;
extern char *the(const char *) NONNULL NO_NNARGS;
extern char *aobjnam(struct obj *, const char *) NONNULL NONNULLARG1;
extern char *yobjnam(struct obj *, const char *) NONNULL NONNULLARG1;
extern char *Yobjnam2(struct obj *, const char *) NONNULL NONNULLARG1;
extern char *Tobjnam(struct obj *, const char *) NONNULL NONNULLARG1;
extern char *otense(struct obj *, const char *) NONNULL NONNULLARG12;
extern char *vtense(const char *, const char *) NONNULL NONNULLARG2;
extern char *Doname2(struct obj *) NONNULL NONNULLARG1;
extern char *paydoname(struct obj *) NONNULL NONNULLARG1;
extern char *yname(struct obj *) NONNULL NONNULLARG1;
extern char *Yname2(struct obj *) NONNULL NONNULLARG1;
extern char *ysimple_name(struct obj *) NONNULL NONNULLARG1;
extern char *Ysimple_name2(struct obj *) NONNULL NONNULLARG1;
extern char *simpleonames(struct obj *) NONNULL NONNULLARG1;
extern char *ansimpleoname(struct obj *) NONNULL NONNULLARG1;
extern char *thesimpleoname(struct obj *) NONNULL NONNULLARG1;
extern char *actualoname(struct obj *) NONNULL NONNULLARG1;
extern char *bare_artifactname(struct obj *) NONNULL NONNULLARG1;
/* makeplural() and makesingular() never return NULL but have tests for NULL
   arg1, and code path that leads to impossible(), preventing NONNULLARG1 */
extern char *makeplural(const char *) NONNULL NO_NNARGS;
extern char *makesingular(const char *) NONNULL NO_NNARGS;
/* readobjnam() can return NULL and  allows a NULL to trigger code path for
   random object */
extern struct obj *readobjnam(char *, struct obj *) NO_NNARGS;
extern int rnd_class(int, int);
/* discover_object() passes NULL arg2 to Japanese_item_name(),
 * preventing NONNULLARG2 */
extern const char *Japanese_item_name(int, const char *) NO_NNARGS;
extern const char *armor_simple_name(struct obj *) NONNULL NONNULLARG1;
/* suit_simple_name has its code in a NULL arg test
   conditional block, preventing NONNULLARG1 */
extern const char *suit_simple_name(struct obj *) NONNULL NO_NNARGS;
/* cloak_simple_name has its code in a NULL arg test
   conditional block, preventing NONNULLARG1 */
extern const char *cloak_simple_name(struct obj *) NONNULL NO_NNARGS;
/* helm_simple_name always just returns hardcoded literals */
extern const char *helm_simple_name(struct obj *) NONNULL NO_NNARGS;
/* gloves_simple_name has its code in a NULL arg test
   conditional block, preventing NONNULLARG1 */
extern const char *gloves_simple_name(struct obj *) NONNULL NO_NNARGS;
/* boots_simple_name has its code in a NULL arg test
   conditional block, preventing NONNULLARG1 */
extern const char *boots_simple_name(struct obj *) NONNULL NO_NNARGS;
/* shield_simple_name has its code in a NULL arg test
   conditional block, preventing NONNULLARG1 */
extern const char *shield_simple_name(struct obj *) NONNULL NO_NNARGS;
/* shirt_simple_name always just returns hardcoded "shirt" */
extern const char *shirt_simple_name(struct obj *) NONNULL NO_NNARGS;
extern const char *mimic_obj_name(struct monst *) NONNULL NONNULLARG1;
/* safe_qbuf() contains tests for NULL arg2 and arg3, qprefix and qsuffix,
   preventing use of NONNULLPTRS. */
extern char *safe_qbuf(char *, const char *, const char *, struct obj *,
                       char * (*)(struct obj *), char * (*)(struct obj *),
                       const char *) NONNULL NONNULLARG14;
extern int shiny_obj(char);

/* ### options.c ### */

extern boolean ask_do_tutorial(void);
extern boolean match_optname(const char *, const char *, int, boolean) NONNULLARG12;
extern uchar txt2key(char *) NONNULLARG1;
extern void initoptions(void);
extern void initoptions_init(void);
extern void initoptions_finish(void);
extern boolean parseoptions(char *, boolean, boolean) NONNULLARG1;
extern void freeroleoptvals(void);
extern char *get_option_value(const char *, boolean) NONNULLARG1;
extern int doset_simple(void);
extern int doset(void);
extern int dotogglepickup(void);
extern void option_help(void);
extern void all_options_strbuf(strbuf_t *) NONNULLARG1;
extern void next_opt(winid, const char *) NONNULLARG2;
extern int fruitadd(char *, struct fruit *) NONNULLARG1;
extern boolean parsebindings(char *) NONNULLARG1;
extern void oc_to_str(char *, char *) NONNULLARG12;
extern void add_menu_cmd_alias(char, char);
extern char get_menu_cmd_key(char);
extern char map_menu_cmd(char);
extern char *collect_menu_keys(char *, unsigned, boolean) NONNULLARG1;
extern void show_menu_controls(winid, boolean);
extern void assign_warnings(uchar *) NONNULLARG1;
extern char *nh_getenv(const char *) NONNULLARG1;
extern void reset_duplicate_opt_detection(void);
extern void set_wc_option_mod_status(unsigned long, int);
extern void set_wc2_option_mod_status(unsigned long, int);
extern void set_option_mod_status(const char *, int) NONNULLARG1;
extern int add_autopickup_exception(const char *) NONNULLARG1;
extern void free_autopickup_exceptions(void);
extern void set_playmode(void);
extern int sym_val(const char *) NONNULLARG1;
extern boolean msgtype_parse_add(char *) NONNULLARG1;
extern int msgtype_type(const char *, boolean) NONNULLARG1;
extern void hide_unhide_msgtypes(boolean, int);
extern void msgtype_free(void);

/* ### pager.c ### */

extern char *self_lookat(char *) NONNULL NONNULLARG1;
extern char *monhealthdescr(struct monst *mon, boolean,
                            char *) NONNULL NONNULLARG3;
extern void mhidden_description(struct monst *, unsigned, char *) NONNULLPTRS;
extern boolean object_from_map(int, coordxy, coordxy,
                               struct obj **) NONNULLPTRS;
extern const char *waterbody_name(coordxy, coordxy) NONNULL;
extern char *ice_descr(coordxy, coordxy, char *) NONNULL NONNULLARG3;
extern boolean ia_checkfile(struct obj *) NONNULLARG1;
extern int do_screen_description(coord, boolean, int, char *, const char **,
                                 struct permonst **) NONNULLARG45;
extern int do_look(int, coord *);
extern int dowhatis(void);
extern int doquickwhatis(void);
extern int doidtrap(void);
extern int dowhatdoes(void);
extern char *dowhatdoes_core(char, char *) NONNULLARG2; /*might return NULL*/
extern int dohelp(void);
extern int dohistory(void);

/* ### xxmain.c ### */

#if defined(MICRO) || defined(WIN32)
#ifdef CHDIR
extern void chdirx(char *, boolean);
#endif /* CHDIR */
extern boolean authorize_wizard_mode(void);
extern boolean authorize_explore_mode(void);
#endif
#if defined(WIN32)
extern int getlock(void);
extern const char *get_portable_device(void);
#endif

/* ### pcsys.c ### */

#if defined(MICRO) || defined(WIN32)
extern void flushout(void);
extern int dosh(void);
extern void append_slash(char *);
extern void getreturn(const char *);
#ifndef AMIGA
extern void msmsg(const char *, ...) PRINTF_F(1, 2);
#endif
/* E FILE *fopenp(const char *, const char *); */
#endif /* MICRO || WIN2 */

/* ### pctty.c ### */

#if defined(MICRO) || defined(WIN32)
extern void gettty(void);
extern void settty(const char *);
extern void setftty(void);
ATTRNORETURN extern void error(const char *, ...) PRINTF_F(1, 2) NORETURN;
#if defined(TIMED_DELAY) && defined(_MSC_VER)
extern void msleep(unsigned);
#endif
#endif /* MICRO || WIN32 */

/* ### pcunix.c ### */
#if defined(MICRO)
extern void regularize(char *);
#if defined(PC_LOCKING)
extern void getlock(void);
#endif
#endif /* MICRO */

/* ### pickup.c ### */

extern int collect_obj_classes(char *, struct obj *, boolean,
                               boolean(*)(struct obj *), int *) NONNULLARG5;
extern boolean rider_corpse_revival(struct obj *, boolean) NO_NNARGS;
extern void force_decor(boolean);
extern void deferred_decor(boolean);
extern boolean menu_class_present(int);
extern void add_valid_menu_class(int);
extern boolean allow_all(struct obj *) NO_NNARGS;
extern boolean allow_category(struct obj *) NONNULLARG1;
extern boolean is_worn_by_type(struct obj *) NONNULLARG1;
extern int ck_bag(struct obj *) NONNULLARG1;
extern void removed_from_icebox(struct obj *) NONNULLARG1;
/* reset_justpicked() is sometimes passed gi.invent
 * which can be null */
extern void reset_justpicked(struct obj *) NO_NNARGS;
/* sometimes count_justpicked(gi.invent) which can be null */
extern int count_justpicked(struct obj *) NO_NNARGS;
/* sometimes find_justpicked(gi.invent) which can be null */
extern struct obj *find_justpicked(struct obj *) NO_NNARGS;
extern int pickup(int);
extern int pickup_object(struct obj *, long, boolean) NONNULLARG1;
extern int query_category(const char *, struct obj *, int, menu_item **, int) NONNULLARG14;
/* dotypeinv() call query_objlist with NULL arg1 */
extern int query_objlist(const char *, struct obj **, int, menu_item **, int,
                         boolean(*)(struct obj *)) NONNULLARG24;
extern struct obj *pick_obj(struct obj *) NONNULLARG1;
extern int encumber_msg(void);
extern int container_at(coordxy, coordxy, boolean);
extern int doloot(void);
extern void observe_quantum_cat(struct obj *, boolean, boolean) NONNULLARG1;
extern boolean container_gone(int(*)(struct obj *)) NONNULLARG1;
extern boolean u_handsy(void);
extern int use_container(struct obj **, boolean, boolean) NONNULLARG1;
extern int loot_mon(struct monst *, int *, boolean *) NO_NNARGS;
extern int dotip(void);
extern struct autopickup_exception *check_autopickup_exceptions(struct obj *) NONNULLARG1;
extern boolean autopick_testobj(struct obj *, boolean) NONNULLARG1;

/* ### pline.c ### */

#ifdef DUMPLOG_CORE
extern void dumplogmsg(const char *);
extern void dumplogfreemessages(void);
#endif
extern void pline(const char *, ...) PRINTF_F(1, 2);
extern void pline_dir(int, const char *, ...) PRINTF_F(2, 3);
extern void pline_xy(coordxy, coordxy, const char *, ...) PRINTF_F(3, 4);
extern void pline_mon(struct monst *, const char *, ...) PRINTF_F(2, 3) NONNULLARG1;
extern void set_msg_dir(int);
extern void set_msg_xy(coordxy, coordxy);
extern void custompline(unsigned, const char *, ...) PRINTF_F(2, 3);
extern void urgent_pline(const char *, ...) PRINTF_F(1, 2);
extern void Norep(const char *, ...) PRINTF_F(1, 2);
extern void free_youbuf(void);
extern void You(const char *, ...) PRINTF_F(1, 2);
extern void Your(const char *, ...) PRINTF_F(1, 2);
extern void You_feel(const char *, ...) PRINTF_F(1, 2);
extern void You_cant(const char *, ...) PRINTF_F(1, 2);
extern void You_hear(const char *, ...) PRINTF_F(1, 2);
extern void You_see(const char *, ...) PRINTF_F(1, 2);
extern void pline_The(const char *, ...) PRINTF_F(1, 2);
extern void There(const char *, ...) PRINTF_F(1, 2);
extern void verbalize(const char *, ...) PRINTF_F(1, 2);
extern void gamelog_add(long, long, const char *);
extern void livelog_printf(long, const char *, ...) PRINTF_F(2, 3);
extern void raw_printf(const char *, ...) PRINTF_F(1, 2);
extern void impossible(const char *, ...) PRINTF_F(1, 2);
extern void config_error_add(const char *, ...) PRINTF_F(1, 2);
extern void nhassert_failed(const char *, const char *, int);

/* ### polyself.c ### */

extern void set_uasmon(void);
extern void float_vs_flight(void);
extern void steed_vs_stealth(void);
extern void change_sex(void);
extern void livelog_newform(boolean, int, int);
extern void polyself(int);
extern int polymon(int);
extern schar uasmon_maxStr(void);
extern void rehumanize(void);
extern int dobreathe(void);
extern int dospit(void);
extern int doremove(void);
extern int dospinweb(void);
extern int dosummon(void);
extern int dogaze(void);
extern int dohide(void);
extern int dopoly(void);
extern int domindblast(void);
extern void uunstick(void);
extern void skinback(boolean);
extern const char *mbodypart(struct monst *, int) NONNULLARG1;
extern const char *body_part(int);
extern int poly_gender(void);
extern void ugolemeffects(int, int);
extern boolean ugenocided(void);
extern const char *udeadinside(void);

/* ### potion.c ### */

extern void set_itimeout(long *, long) NONNULLARG1;
extern void incr_itimeout(long *, int) NONNULLARG1;
extern void make_confused(long, boolean);
extern void make_stunned(long, boolean);
extern void make_sick(long, const char *, boolean, int) NO_NNARGS;
extern void make_slimed(long, const char *) NO_NNARGS;
extern void make_stoned(long, const char *, int, const char *) NO_NNARGS;
extern void make_vomiting(long, boolean);
extern void make_blinded(long, boolean);
extern void toggle_blindness(void);
extern boolean make_hallucinated(long, boolean, long);
extern void make_deaf(long, boolean);
extern void make_glib(int);
extern void self_invis_message(void);
extern int dodrink(void);
extern int dopotion(struct obj *) NONNULLARG1;
extern int peffects(struct obj *) NONNULLARG1;
extern void healup(int, int, boolean, boolean);
extern void strange_feeling(struct obj *, const char *) NO_NNARGS;
extern void impact_arti_light(struct obj *, boolean, boolean) NONNULLARG1;
extern void potionhit(struct monst *, struct obj *, int) NONNULLARG12;
extern void potionbreathe(struct obj *) NONNULLARG1;
extern int dodip(void);
extern int dip_into(void); /* altdip */
extern void mongrantswish(struct monst **) NONNULLARG1;
extern void djinni_from_bottle(struct obj *) NONNULLARG1;
extern struct monst *split_mon(struct monst *, struct monst *) NONNULLARG1;
extern const char *bottlename(void);
extern void speed_up(long);

/* ### pray.c ### */

extern boolean critically_low_hp(boolean);
extern boolean stuck_in_wall(void);
extern void desecrate_altar(boolean, aligntyp);
extern int dosacrifice(void);
extern boolean can_pray(boolean);
extern int dopray(void);
extern const char *u_gname(void);
extern int doturn(void);
extern int altarmask_at(coordxy, coordxy);
extern const char *a_gname(void);
extern const char *a_gname_at(coordxy x, coordxy y);
extern const char *align_gname(aligntyp);
extern const char *halu_gname(aligntyp);
extern const char *align_gtitle(aligntyp);
extern void altar_wrath(coordxy, coordxy);

/* ### priest.c ### */

extern int move_special(struct monst *, boolean, schar, boolean, boolean,
                        coordxy, coordxy, coordxy, coordxy) NONNULLARG1;
extern char temple_occupied(char *) NONNULLARG1;
extern boolean inhistemple(struct monst *) NO_NNARGS;
extern int pri_move(struct monst *) NONNULLARG1;
extern void priestini(d_level *, struct mkroom *, int, int, boolean) NONNULLARG12;
extern aligntyp mon_aligntyp(struct monst *) NONNULLARG1;
extern char *priestname(struct monst *, int, boolean, char *) NONNULLARG1;
extern boolean p_coaligned(struct monst *) NONNULLARG1;
extern struct monst *findpriest(char);
extern void intemple(int);
extern void forget_temple_entry(struct monst *) NONNULLARG1;
extern void priest_talk(struct monst *) NONNULLARG1;
extern struct monst *mk_roamer(struct permonst *, aligntyp, coordxy, coordxy,
                               boolean) NO_NNARGS;
extern void reset_hostility(struct monst *) NONNULLARG1;
extern boolean in_your_sanctuary(struct monst *, coordxy, coordxy) NO_NNARGS;
extern void ghod_hitsu(struct monst *) NONNULLARG1;
extern void angry_priest(void);
extern void clearpriests(void);
extern void restpriest(struct monst *, boolean) NONNULLARG1;
extern void newepri(struct monst *) NONNULLARG1;
extern void free_epri(struct monst *) NONNULLARG1;

/* ### quest.c ### */

extern void onquest(void);
extern void nemdead(void);
extern void leaddead(void);
extern void artitouch(struct obj *) NONNULLARG1;
extern boolean ok_to_quest(void);
extern void leader_speaks(struct monst *) NONNULLARG1;
extern void nemesis_speaks(void);
extern void nemesis_stinks(coordxy, coordxy);
extern void quest_chat(struct monst *) NONNULLARG1;
extern void quest_talk(struct monst *) NONNULLARG1;
extern void quest_stat_check(struct monst *) NONNULLARG1;
extern void finish_quest(struct obj *) NO_NNARGS;

/* ### questpgr.c ### */

extern void load_qtlist(void);
extern void unload_qtlist(void);
extern short quest_info(int);
extern const char *ldrname(void);
extern boolean is_quest_artifact(struct obj *) NONNULLARG1;
extern struct obj *find_quest_artifact(unsigned);
extern int stinky_nemesis(struct monst *);
extern void com_pager(const char *);
extern void qt_pager(const char *);
extern struct permonst *qt_montype(void);
extern void deliver_splev_message(void);

/* ### random.c ### */

#if defined(RANDOM) && !defined(__GO32__) /* djgpp has its own random */
#ifndef CROSS_TO_AMIGA
extern void srandom(unsigned);
extern char *initstate(unsigned, char *, int);
extern char *setstate(char *);
extern long random(void);
#endif /* CROSS_TO_AMIGA */
#endif /* RANDOM */

/* ### read.c ### */

extern void learnscroll(struct obj *) NONNULLARG1;
extern char *tshirt_text(struct obj *, char *) NONNULLARG12;
extern char *hawaiian_motif(struct obj *, char *) NONNULLARG12;
extern char *apron_text(struct obj *, char *) NONNULLARG12;
extern const char *candy_wrapper_text(struct obj *) NONNULLARG1;
extern void assign_candy_wrapper(struct obj *) NONNULLARG1;
extern int doread(void);
extern int charge_ok(struct obj *) NO_NNARGS;
extern void recharge(struct obj *, int) NONNULLARG1;
extern boolean valid_cloud_pos(coordxy, coordxy);
extern int seffects(struct obj *) NONNULLARG1;
extern void drop_boulder_on_player(boolean, boolean, boolean, boolean);
extern boolean drop_boulder_on_monster(coordxy, coordxy, boolean, boolean);
extern void wand_explode(struct obj *, int) NONNULLARG1;
extern void litroom(boolean, struct obj *) NO_NNARGS;
extern void do_genocide(int);
extern void punish(struct obj *) NO_NNARGS;
extern void unpunish(void);
extern boolean cant_revive(int *, boolean, struct obj *) NO_NNARGS;
extern boolean create_particular(void);

/* ### rect.c ### */

extern void init_rect(void);
extern void free_rect(void);
extern NhRect *get_rect(NhRect *) NONNULLARG1;
extern NhRect *rnd_rect(void);
extern void rect_bounds(NhRect, NhRect, NhRect *) NONNULLARG3;
extern void remove_rect(NhRect *) NONNULLARG1;
extern void add_rect(NhRect *) NONNULLARG1;
extern void split_rects(NhRect *, NhRect *) NONNULLARG12;

/* ## region.c ### */

extern boolean inside_region(NhRegion *, int, int) NO_NNARGS;
extern void clear_regions(void);
extern void run_regions(void);
extern boolean in_out_region(coordxy, coordxy);
extern boolean m_in_out_region(struct monst *, coordxy, coordxy) NONNULLARG1;
extern void update_player_regions(void);
extern void update_monster_region(struct monst *) NONNULLARG1;
extern int reg_damg(NhRegion *) NONNULLARG1;
extern boolean any_visible_region(void);
extern void visible_region_summary(winid);
extern NhRegion *visible_region_at(coordxy, coordxy);
extern void show_region(NhRegion *, coordxy, coordxy) NONNULLARG1;
extern void save_regions(NHFILE *) NONNULLARG1;
extern void rest_regions(NHFILE *) NONNULLARG1;
extern void region_stats(const char *, char *, long *, long *) NONNULLPTRS;
extern NhRegion *create_gas_cloud(coordxy, coordxy, int, int);
extern NhRegion *create_gas_cloud_selection(struct selectionvar *, int);
extern boolean region_danger(void);
extern void region_safety(void);

/* ### report.c ### */

#ifdef CRASHREPORT
extern boolean submit_web_report(int, const char *, const char *);
extern boolean submit_web_report(int, const char *, const char *);
extern void crashreport_init(int, char *[]);
extern void crashreport_bidshow(void);
extern boolean swr_add_uricoded(const char *, char **, int *, char *);
extern int dobugreport(void);
#endif /* CRASHREPORT */
# ifndef NO_SIGNAL
extern void panictrace_handler(int);
# endif
#ifdef PANICTRACE
extern const char *get_saved_pline(int);
extern boolean NH_panictrace_libc(void);
extern boolean NH_panictrace_gdb(void);
#if defined(PANICTRACE) && !defined(NO_SIGNAL)
extern void panictrace_setsignals(boolean);
#endif
#endif /* PANICTRACE */

/* ### restore.c ### */

extern void inven_inuse(boolean);
extern int dorecover(NHFILE *) NONNULLARG1;
extern void restcemetery(NHFILE *, struct cemetery **) NONNULLARG12;
extern void trickery(char *) NO_NNARGS;
extern void getlev(NHFILE *, int, xint8) NONNULLARG1;
extern void get_plname_from_file(NHFILE *, char *, boolean) NONNULLARG12;
#ifdef SELECTSAVED
extern int restore_menu(winid);
#endif
extern void minit(void);
extern boolean lookup_id_mapping(unsigned, unsigned *) NONNULLARG2;
extern int validate(NHFILE *, const char *, boolean) NONNULLARG1;
/* extern void reset_restpref(void); */
/* extern void set_restpref(const char *); */
/* extern void set_savepref(const char *); */

/* ### rip.c ### */

extern void genl_outrip(winid, int, time_t);

/* ### rnd.c ### */

#ifdef USE_ISAAC64
extern void init_isaac64(unsigned long, int(*fn)(int));
extern long nhrand(void);
#endif
extern int rn2(int);
extern int rn2_on_display_rng(int);
extern int rnd_on_display_rng(int);
extern int rnl(int);
extern int rnd(int);
extern int d(int, int);
extern int rne(int);
extern int rnz(int);
extern void init_random(int(*fn)(int));
extern void reseed_random(int(*fn)(int));
extern void shuffle_int_array(int *, int) NONNULLARG1;

/* ### role.c ### */

extern boolean validrole(int);
extern boolean validrace(int, int);
extern boolean validgend(int, int, int);
extern boolean validalign(int, int, int);
extern int randrole(boolean);
extern int randrace(int);
extern int randgend(int, int);
extern int randalign(int, int);
extern int str2role(const char *) NO_NNARGS;
extern int str2race(const char *) NO_NNARGS;
extern int str2gend(const char *) NO_NNARGS;
extern int str2align(const char *) NO_NNARGS;
extern boolean ok_role(int, int, int, int);
extern int pick_role(int, int, int, int);
extern boolean ok_race(int, int, int, int);
extern int pick_race(int, int, int, int);
extern boolean ok_gend(int, int, int, int);
extern int pick_gend(int, int, int, int);
extern boolean ok_align(int, int, int, int);
extern int pick_align(int, int, int, int);
extern void rigid_role_checks(void);
extern boolean setrolefilter(const char *) NONNULLARG1;
extern boolean gotrolefilter(void);
extern char *rolefilterstring(char *, int) NONNULLARG1;
extern void clearrolefilter(int);
extern char *root_plselection_prompt(char *, int, int, int, int, int) NO_NNARGS;
extern char *build_plselection_prompt(char *, int, int, int, int, int) NONNULLARG1;
extern void plnamesuffix(void);
extern void role_selection_prolog(int, winid);
extern void role_menu_extra(int, winid, boolean);
extern void role_init(void);
extern const char *Hello(struct monst *) NO_NNARGS;
extern const char *Goodbye(void);
extern const struct Race *character_race(short);
extern void genl_player_selection(void);
extern int genl_player_setup(int);

/* ### rumors.c ### */

extern char *getrumor(int, char *, boolean) NONNULLARG2;
extern char *get_rnd_text(const char *, char *, int(*)(int),
                          unsigned) NONNULLPTRS;
extern void outrumor(int, int);
extern void outoracle(boolean, boolean);
extern void save_oracles(NHFILE *) NONNULLARG1;
extern void restore_oracles(NHFILE *) NONNULLARG1;
extern int doconsult(struct monst *) NO_NNARGS;
extern void rumor_check(void);
extern boolean CapitalMon(const char *) NO_NNARGS;
extern void free_CapMons(void);

/* ### save.c ### */

extern int dosave(void);
extern int dosave0(void);
extern boolean tricked_fileremoved(NHFILE *, char *) NONNULLARG2;
#ifdef INSURANCE
extern void savestateinlock(void);
#endif
extern void savelev(NHFILE *, xint8) NONNULLARG1;
/* extern genericptr_t mon_to_buffer(struct monst *, int *); */
extern void savecemetery(NHFILE *, struct cemetery **) NONNULLARG12;
extern void savefruitchn(NHFILE *) NONNULLARG1;
extern void store_plname_in_file(NHFILE *) NONNULLARG1;
extern void free_dungeons(void);
extern void freedynamicdata(void);
extern void store_savefileinfo(NHFILE *) NONNULLARG1;
extern void store_savefileinfo(NHFILE *) NONNULLARG1;
extern int nhdatatypes_size(void);
#if 0
extern void assignlog(char *, char*, int);
extern FILE *getlog(NHFILE *);
extern void closelog(NHFILE *);
#endif

/* ### selvar.c ### */

extern struct selectionvar *selection_new(void);
extern void selection_free(struct selectionvar *, boolean) NO_NNARGS;
extern void selection_clear(struct selectionvar *, int) NONNULLARG1;
extern struct selectionvar *selection_clone(struct selectionvar *) NONNULLARG1;
extern void selection_getbounds(struct selectionvar *, NhRect *) NO_NNARGS;
extern void selection_recalc_bounds(struct selectionvar *) NONNULLARG1;
extern coordxy selection_getpoint(coordxy, coordxy, struct selectionvar *) NO_NNARGS;
extern void selection_setpoint(coordxy, coordxy, struct selectionvar *, int);
extern struct selectionvar * selection_not(struct selectionvar *);
extern struct selectionvar *selection_filter_percent(struct selectionvar *,
                                                     int);
extern struct selectionvar *selection_filter_mapchar(struct selectionvar *,
                                                     xint16, int);
extern int selection_rndcoord(struct selectionvar *, coordxy *, coordxy *,
                              boolean);
extern void selection_do_grow(struct selectionvar *, int);
extern void set_selection_floodfillchk(int(*)(coordxy, coordxy));
extern void selection_floodfill(struct selectionvar *, coordxy, coordxy,
                                boolean);
extern void selection_do_ellipse(struct selectionvar *, int, int, int, int,
                                 int);
extern void selection_do_gradient(struct selectionvar *, long, long, long,
                                  long, long, long, long);
extern void selection_do_line(coordxy, coordxy, coordxy, coordxy,
                              struct selectionvar *);
extern void selection_do_randline(coordxy, coordxy, coordxy, coordxy,
                                  schar, schar, struct selectionvar *);
extern void selection_iterate(struct selectionvar *, select_iter_func,
                              genericptr_t);
extern boolean selection_is_irregular(struct selectionvar *);
extern char *selection_size_description(struct selectionvar *, char *);
extern struct selectionvar *selection_from_mkroom(struct mkroom *) NO_NNARGS;
extern void selection_force_newsyms(struct selectionvar *) NONNULLARG1;

/* ### sfstruct.c ### */

extern boolean close_check(int);
/* extern void newread(NHFILE *, int, int, genericptr_t, unsigned); */
extern void bufon(int);
extern void bufoff(int);
extern void bflush(int);
extern void bwrite(int, const genericptr_t, unsigned) NONNULLARG2;
extern void mread(int, genericptr_t, unsigned) NONNULLARG2;
extern void minit(void);
extern void bclose(int);
#if defined(ZEROCOMP)
extern void zerocomp_bclose(int);
#endif

/* ### shk.c ### */

/* setpaid() has a conditional code block near the end of the
   function, where arg1 is tested for NULL, preventing NONNULLARG1 */
extern void setpaid(struct monst *) NO_NNARGS;
extern long money2mon(struct monst *, long) NONNULLARG1;
extern void money2u(struct monst *, long) NONNULLARG1;
extern void shkgone(struct monst *) NONNULLARG1;
extern void set_residency(struct monst *, boolean) NONNULLARG1;
extern void replshk(struct monst *, struct monst *) NONNULLARG12;
extern void restshk(struct monst *, boolean) NONNULLARG1;
extern char inside_shop(coordxy, coordxy);
extern void u_left_shop(char *, boolean) NONNULLARG1;
extern void remote_burglary(coordxy, coordxy);
extern void u_entered_shop(char *);
extern void pick_pick(struct obj *) NONNULLARG1;
extern boolean same_price(struct obj *, struct obj *) NONNULLARG12;
extern void shopper_financial_report(void);
extern int inhishop(struct monst *) NONNULLARG1;
extern struct monst *shop_keeper(char);
extern struct monst *find_objowner(struct obj *,
                                   coordxy x, coordxy y) NONNULLARG1;
extern boolean tended_shop(struct mkroom *) NONNULLARG1;
extern boolean onshopbill(struct obj *, struct monst *, boolean) NONNULLARG1;
extern boolean is_unpaid(struct obj *) NONNULLARG1;
extern void delete_contents(struct obj *) NONNULLARG1;
extern void obfree(struct obj *, struct obj *) NONNULLARG1;
extern void make_happy_shk(struct monst *, boolean) NONNULLARG1;
extern void make_happy_shoppers(boolean);
extern void hot_pursuit(struct monst *) NONNULLARG1;
extern void make_angry_shk(struct monst *, coordxy, coordxy) NONNULLARG1;
extern int dopay(void);
extern boolean paybill(int, boolean);
extern void finish_paybill(void);
extern struct obj *find_oid(unsigned);
extern long contained_cost(struct obj *, struct monst *, long, boolean,
                           boolean) NONNULLARG12;
extern long contained_gold(struct obj *, boolean) NONNULLARG1;
extern void picked_container(struct obj *) NONNULLARG1;
extern void gem_learned(int);
extern void alter_cost(struct obj *, long) NONNULLARG1;
extern long unpaid_cost(struct obj *, uchar) NONNULLARG1;
extern boolean billable(struct monst **, struct obj *, char,
                        boolean) NONNULLARG12;
extern void addtobill(struct obj *, boolean, boolean, boolean) NONNULLARG1;
extern void splitbill(struct obj *, struct obj *) NONNULLARG12;
extern void subfrombill(struct obj *, struct monst *) NONNULLARG12;
extern long stolen_value(struct obj *, coordxy, coordxy,
                         boolean, boolean) NONNULLARG1;
extern void donate_gold(long, struct monst *, boolean) NONNULLARG2;
extern void sellobj_state(int);
extern void sellobj(struct obj *, coordxy, coordxy) NONNULLARG1;
extern int doinvbill(int);
extern struct monst *shkcatch(struct obj *, coordxy, coordxy) NONNULLARG1;
extern void add_damage(coordxy, coordxy, long);
extern void fix_shop_damage(void);
extern int shk_move(struct monst *) NONNULLARG1;
extern void after_shk_move(struct monst *) NONNULLARG1;
extern boolean is_fshk(struct monst *) NONNULLARG1;
extern void shopdig(int);
extern void pay_for_damage(const char *, boolean);
extern boolean costly_spot(coordxy, coordxy);
/* costly_adjacent() has checks for null 1st arg, and an early return,
   so it cannot be NONNULLARG1 */
extern boolean costly_adjacent(struct monst *, coordxy, coordxy) NO_NNARGS;
extern struct obj *shop_object(coordxy, coordxy);
extern void price_quote(struct obj *) NONNULLARG1;
extern void shk_chat(struct monst *) NONNULLARG1;
extern void check_unpaid_usage(struct obj *, boolean) NONNULLARG1;
extern void check_unpaid(struct obj *) NONNULLARG1;
extern void costly_gold(coordxy, coordxy, long, boolean);
extern long get_cost_of_shop_item(struct obj *, int *) NONNULLARG1;
extern int oid_price_adjustment(struct obj *, unsigned) NONNULLARG1;
extern boolean block_door(coordxy, coordxy);
extern boolean block_entry(coordxy, coordxy);
extern char *shk_your(char *, struct obj *) NONNULLPTRS;
extern char *Shk_Your(char *, struct obj *) NONNULLPTRS;
extern void globby_bill_fixup(struct obj *, struct obj *) NONNULLARG12;
/*extern void globby_donation(struct obj *, struct obj *); */
extern void credit_report(struct monst *shkp, int idx,
                          boolean silent) NONNULLARG1;
extern void use_unpaid_trapobj(struct obj *, coordxy, coordxy) NONNULLARG1;

/* ### shknam.c ### */

extern void neweshk(struct monst *) NONNULLARG1;
extern void free_eshk(struct monst *) NONNULLARG1;
extern void stock_room(int, struct mkroom *) NONNULLARG2;
extern boolean saleable(struct monst *, struct obj *) NONNULLARG12;
extern int get_shop_item(int);
extern char *Shknam(struct monst *) NONNULLARG1;
extern char *shkname(struct monst *) NONNULLARG1;
extern boolean shkname_is_pname(struct monst *) NONNULLARG1;
extern boolean is_izchak(struct monst *, boolean) NONNULLARG1;

/* ### sit.c ### */

extern void take_gold(void);
extern int dosit(void);
extern void rndcurse(void);
extern int attrcurse(void);

/* ### sounds.c ### */

extern void dosounds(void);
extern const char *growl_sound(struct monst *) NONNULLARG1;
extern void growl(struct monst *) NONNULLARG1;
extern void yelp(struct monst *) NONNULLARG1;
extern void whimper(struct monst *) NONNULLARG1;
extern void beg(struct monst *) NONNULLARG1;
extern const char *maybe_gasp(struct monst *) NONNULLARG1;
extern const char *cry_sound(struct monst *) NONNULLARG1;
extern int dotalk(void);
extern int tiphat(void);
#ifdef USER_SOUNDS
extern int add_sound_mapping(const char *) NONNULLARG1;
extern void play_sound_for_message(const char *) NONNULLARG1;
extern void maybe_play_sound(const char *) NONNULLARG1;
extern void release_sound_mappings(void);
#if defined(WIN32) || defined(QT_GRAPHICS)
extern void play_usersound(const char *, int);
#endif
#if defined(TTY_SOUND_ESCCODES)
extern void play_usersound_via_idx(int, int);
#endif
#endif /* USER SOUNDS */
extern void assign_soundlib(int);
extern void activate_chosen_soundlib(void);
extern void get_soundlib_name(char *dest, int maxlen) NONNULLARG1;
#ifdef SND_SOUNDEFFECTS_AUTOMAP
extern char *get_sound_effect_filename(int32_t seidint,
                                       char *buf, size_t bufsz, int32_t);
#endif
extern char *base_soundname_to_filename(char *, char *, size_t, int32_t) NONNULLARG1;
extern void set_voice(struct monst *, int32_t, int32_t, int32_t) NO_NNARGS;
extern void sound_speak(const char *) NO_NNARGS;

/* ### sp_lev.c ### */

#if !defined(CROSSCOMPILE) || defined(CROSSCOMPILE_TARGET)
extern boolean match_maptyps(xint16, xint16);
extern void create_des_coder(void);
extern void reset_xystart_size(void);
extern struct mapfragment *mapfrag_fromstr(char *) NONNULLARG1;
extern void mapfrag_free(struct mapfragment **) NO_NNARGS;
extern schar mapfrag_get(struct mapfragment *, int, int) NONNULLARG1;
extern boolean mapfrag_canmatch(struct mapfragment *) NONNULLARG1;
extern const char * mapfrag_error(struct mapfragment *) NO_NNARGS;
extern boolean mapfrag_match(struct mapfragment *, int, int) NONNULLARG1;
extern void flip_level(int, boolean);
extern void flip_level_rnd(int, boolean);
extern boolean check_room(coordxy *, coordxy *, coordxy *, coordxy *, boolean) NONNULLPTRS;
extern boolean create_room(coordxy, coordxy, coordxy, coordxy,
                           coordxy, coordxy, xint16, xint16);
extern boolean dig_corridor(coord *, coord *, boolean, schar, schar) NONNULLARG12;
extern void fill_special_room(struct mkroom *) NO_NNARGS;
extern void wallify_map(coordxy, coordxy, coordxy, coordxy);
extern boolean load_special(const char *) NONNULLARG1;
extern coordxy random_wdir(void);
extern boolean pm_good_location(coordxy, coordxy, struct permonst *) NONNULLARG3;
extern void get_location_coord(coordxy *, coordxy *, int, struct mkroom *,
                               long) NONNULLARG12;
extern void set_floodfillchk_match_under(coordxy);
extern int lspo_reset_level(lua_State *) NO_NNARGS; /* wiz_load_splua NULL */
/* lspo_finalize_level() has tests for whether arg1 L is null, and chooses
   code paths to follow based on that. Also preventing NONNULLARG1 is it
   being called from wiz_load_splua() with a NULL arg.
   Side note: The parameter is also marked as UNUSED, but apparently it is */
extern int lspo_finalize_level(lua_State *) NO_NNARGS;
extern boolean get_coord(lua_State *, int, lua_Integer *, lua_Integer *) NONNULLPTRS;
extern void cvt_to_abscoord(coordxy *, coordxy *) NONNULLPTRS;
extern void cvt_to_relcoord(coordxy *, coordxy *) NONNULLPTRS;
extern int nhl_abs_coord(lua_State *) NONNULLARG1;
extern void update_croom(void);
extern const char *get_trapname_bytype(int);
extern void l_register_des(lua_State *) NONNULLARG1;
#endif /* !CROSSCOMPILE || CROSSCOMPILE_TARGET */

/* ### spell.c ### */

extern void book_cursed(struct obj *) NONNULLARG1;
extern int study_book(struct obj *) NONNULLARG1;
extern void book_disappears(struct obj *) NONNULLARG1;
extern void book_substitution(struct obj *, struct obj *) NONNULLARG12;
extern void age_spells(void);
extern int dowizcast(void);
extern int docast(void);
extern int spell_skilltype(int);
extern int spelleffects(int, boolean, boolean);
extern int tport_spell(int);
extern void losespells(void);
extern int dovspell(void);
extern void initialspell(struct obj *) NONNULLARG1;
extern int known_spell(short);
extern int spell_idx(short);
extern char force_learn_spell(short);
extern int num_spells(void);
extern void skill_based_spellbook_id(void);

/* ### stairs.c ### */

extern void stairway_add(coordxy, coordxy,
                         boolean, boolean, d_level *) NONNULLPTRS;
extern void stairway_free_all(void);
extern stairway *stairway_at(coordxy, coordxy);
extern stairway *stairway_find(d_level *) NONNULLARG1;
extern stairway *stairway_find_from(d_level *, boolean) NONNULLARG1;
extern stairway *stairway_find_dir(boolean);
extern stairway *stairway_find_type_dir(boolean, boolean);
extern stairway *stairway_find_special_dir(boolean);
extern void u_on_sstairs(int);
extern void u_on_upstairs(void);
extern void u_on_dnstairs(void);
extern boolean On_stairs(coordxy, coordxy);
extern boolean On_ladder(coordxy, coordxy);
extern boolean On_stairs_up(coordxy, coordxy);
extern boolean On_stairs_dn(coordxy, coordxy);
extern boolean known_branch_stairs(stairway *);
extern char *stairs_description(stairway *, char *, boolean) NONNULLARG1;

/* ### steal.c ### */

extern long somegold(long);
extern void stealgold(struct monst *) NONNULLARG1;
extern void thiefdead(void);
extern boolean unresponsive(void);
extern void remove_worn_item(struct obj *, boolean) NONNULLARG1;
extern int steal(struct monst *, char *) NONNULLARG1;
/* mpickobj() contains a test for NULL arg2 obj and a code path
   that leads to impossible(). Prevents NONNULLARG12. */
extern int mpickobj(struct monst *, struct obj *) NONNULLARG1;
extern void stealamulet(struct monst *) NONNULLARG1;
extern void maybe_absorb_item(struct monst *, struct obj *, int, int) NONNULLARG12;
extern void mdrop_obj(struct monst *, struct obj *, boolean) NONNULLARG12;
extern void mdrop_special_objs(struct monst *) NONNULLARG1;
extern void relobj(struct monst *, int, boolean) NONNULLARG1;
extern struct obj *findgold(struct obj *) NO_NNARGS;

/* ### steed.c ### */

extern void rider_cant_reach(void);
extern boolean can_saddle(struct monst *) NONNULLARG1;
extern int use_saddle(struct obj *) NONNULLARG1;
extern void put_saddle_on_mon(struct obj *, struct monst *) NONNULLARG12;
extern boolean can_ride(struct monst *) NONNULLARG1;
extern int doride(void);
extern boolean mount_steed(struct monst *, boolean) NO_NNARGS;
extern void exercise_steed(void);
extern void kick_steed(void);
extern void dismount_steed(int);
extern void place_monster(struct monst *, coordxy, coordxy) NONNULLARG1;
extern void poly_steed(struct monst *, struct permonst *) NONNULLARG12;
extern boolean stucksteed(boolean);

/* ### symbols.c ### */

extern void switch_symbols(int);
extern void assign_graphics(int);
extern void init_symbols(void);
extern void init_showsyms(void);
extern void init_primary_symbols(void);
extern void init_rogue_symbols(void);
extern void init_ov_primary_symbols(void);
extern void init_ov_rogue_symbols(void);
extern void clear_symsetentry(int, boolean);
extern void update_primary_symset(const struct symparse *, int) NONNULLARG1;
extern void update_rogue_symset(const struct symparse *, int) NONNULLARG1;
extern void update_ov_primary_symset(const struct symparse *, int) NONNULLARG1;
extern void update_ov_rogue_symset(const struct symparse *, int) NONNULLARG1;
extern int parse_sym_line(char *, int) NONNULLARG1;
extern nhsym get_othersym(int, int);
extern boolean symset_is_compatible(enum symset_handling_types, unsigned long);
extern void set_symhandling(char *handling, int which_set) NONNULLARG1;
extern boolean proc_symset_line(char *) NONNULLARG1;
extern int do_symset(boolean);
extern int load_symset(const char *, int) NONNULLARG1;
extern void free_symsets(void);
extern const struct symparse *match_sym(char *) NONNULLARG1;
extern void savedsym_free(void);
extern void savedsym_strbuf(strbuf_t *) NONNULLARG1;
extern boolean parsesymbols(char *, int) NONNULLARG1;

/* ### sys.c ### */

extern void sys_early_init(void);
extern void sysopt_release(void);
extern void sysopt_seduce_set(int);

/* ### teleport.c ### */

extern boolean noteleport_level(struct monst *) NONNULLARG1;
/* rloc_engr() passes NULL monst arg to goodpos()*/
extern boolean goodpos(coordxy, coordxy, struct monst *,
                       mmflags_nht) NO_NNARGS;
extern boolean enexto(coord *, coordxy, coordxy,
                      struct permonst *) NONNULLARG1;
extern boolean enexto_gpflags(coord *, coordxy, coordxy, struct permonst *,
                           mmflags_nht) NONNULLARG1;
extern boolean enexto_core(coord *, coordxy, coordxy, struct permonst *,
                           mmflags_nht) NONNULLARG1;
extern void teleds(coordxy, coordxy, int);
extern int collect_coords(coord *, coordxy, coordxy, int, unsigned,
                          boolean (*)(coordxy, coordxy)) NONNULLARG1;
extern boolean safe_teleds(int);
extern boolean teleport_pet(struct monst *, boolean) NONNULLARG1;
extern void tele(void);
extern void scrolltele(struct obj *) NO_NNARGS;
extern int dotelecmd(void);
extern int dotele(boolean);
extern void level_tele(void);
extern void domagicportal(struct trap *) NONNULLARG1;
extern void tele_trap(struct trap *) NONNULLARG1;
extern void level_tele_trap(struct trap *, unsigned) NONNULLARG1;
extern void rloc_to(struct monst *, coordxy, coordxy) NONNULLARG1;
extern void rloc_to_flag(struct monst *, coordxy, coordxy,
                         unsigned) NONNULLARG1;
extern boolean rloc(struct monst *, unsigned) NONNULLARG1;
extern boolean control_mon_tele(struct monst *, coord *cc, unsigned,
                                boolean) NONNULLARG1;
extern boolean tele_restrict(struct monst *) NONNULLARG1;
extern void mtele_trap(struct monst *, struct trap *, int) NONNULLARG12;
extern int mlevel_tele_trap(struct monst *, struct trap *,
                            boolean, int) NONNULLARG1;
extern boolean rloco(struct obj *) NONNULLARG1;
extern int random_teleport_level(void);
extern boolean u_teleport_mon(struct monst *, boolean) NONNULLARG1;

/* ### timeout.c ### */

extern const char *property_by_index(int, int *) NO_NNARGS;
extern void burn_away_slime(void);
extern void nh_timeout(void);
extern void fall_asleep(int, boolean);
extern void attach_egg_hatch_timeout(struct obj *, long) NONNULLARG1;
extern void attach_fig_transform_timeout(struct obj *) NONNULLARG1;
extern void kill_egg(struct obj *) NONNULLARG1;
extern void hatch_egg(union any *, long) NONNULLARG1;
extern void learn_egg_type(int);
extern void burn_object(union any *, long) NONNULLARG1;
extern void begin_burn(struct obj *, boolean) NONNULLARG1;
extern void end_burn(struct obj *, boolean) NONNULLARG1;
extern void do_storms(void);
extern boolean start_timer(long, short, short, union any *) NONNULLARG4;
extern long stop_timer(short, union any *) NONNULLARG2;
extern long peek_timer(short, union any *) NONNULLARG2;
extern void run_timers(void);
extern void obj_move_timers(struct obj *, struct obj *) NONNULLARG12;
extern void obj_split_timers(struct obj *, struct obj *) NONNULLARG12;
extern void obj_stop_timers(struct obj *) NONNULLARG1;
extern boolean obj_has_timer(struct obj *, short) NONNULLARG1;
extern void spot_stop_timers(coordxy, coordxy, short);
extern long spot_time_expires(coordxy, coordxy, short);
extern long spot_time_left(coordxy, coordxy, short);
extern boolean obj_is_local(struct obj *) NONNULLARG1;
extern void save_timers(NHFILE *, int) NONNULLARG1;
extern void restore_timers(NHFILE *, int, long) NONNULLARG1;
extern void timer_stats(const char *, char *, long *, long *) NONNULLPTRS;
extern void relink_timers(boolean);
extern int wiz_timeout_queue(void);
extern void timer_sanity_check(void);

/* ### topten.c ### */

extern void formatkiller(char *, unsigned, int, boolean) NONNULLARG1;
extern int observable_depth(d_level *) NONNULLARG1;
extern void topten(int, time_t);
extern void prscore(int, char **);
extern struct toptenentry *get_rnd_toptenentry(void);
extern struct obj *tt_oname(struct obj *) NO_NNARGS;
extern int tt_doppel(struct monst *) NONNULLARG1;

/* ### track.c ### */

extern void initrack(void);
extern void settrack(void);
extern coord *gettrack(coordxy, coordxy);
extern boolean hastrack(coordxy, coordxy);
extern void save_track(NHFILE *) NONNULLARG1;
extern void rest_track(NHFILE *) NONNULLARG1;

/* ### trap.c ### */

extern boolean burnarmor(struct monst *) NO_NNARGS;
extern int erode_obj(struct obj *, const char *, int, int) NO_NNARGS;
extern boolean grease_protect(struct obj *, const char *,
                              struct monst *) NONNULLARG1;
extern struct trap *maketrap(coordxy, coordxy, int);
extern d_level *clamp_hole_destination(d_level *) NONNULLARG1;
extern void fall_through(boolean, unsigned);
extern struct monst *animate_statue(struct obj *, coordxy, coordxy,
                                    int, int *) NONNULLARG1;
extern struct monst *activate_statue_trap(struct trap *, coordxy, coordxy,
                                          boolean) NONNULLARG1;
extern int immune_to_trap(struct monst *, unsigned) NO_NNARGS; /* revisit */
extern void set_utrap(unsigned, unsigned);
extern void reset_utrap(boolean);
extern boolean m_harmless_trap(struct monst *, struct trap *) NONNULLPTRS;
extern void dotrap(struct trap *, unsigned) NONNULLARG1;
extern void seetrap(struct trap *) NONNULLARG1;
extern void feeltrap(struct trap *) NONNULLARG1;
extern int mintrap(struct monst *, unsigned) NONNULLARG1;
extern void instapetrify(const char *) NO_NNARGS;
extern void minstapetrify(struct monst *, boolean) NONNULLARG1;
extern void selftouch(const char *) NONNULLARG1;
extern void mselftouch(struct monst *, const char *, boolean) NONNULLARG1;
extern void float_up(void);
extern void fill_pit(coordxy, coordxy);
extern int float_down(long, long);
extern void climb_pit(void);
extern boolean fire_damage(struct obj *, boolean,
                           coordxy, coordxy) NONNULLARG1;
extern int fire_damage_chain(struct obj *, boolean, boolean,
                            coordxy, coordxy) NO_NNARGS;
extern boolean lava_damage(struct obj *, coordxy, coordxy) NONNULLARG1;
/* acid_damage() has a test for NULL arg and early return if so,
   preventing NONNULLARG1 */
extern void acid_damage(struct obj *) NO_NNARGS;
extern int water_damage(struct obj *, const char *, boolean) NO_NNARGS;
extern void water_damage_chain(struct obj *, boolean) NO_NNARGS;
extern boolean rnd_nextto_goodpos(coordxy *, coordxy *,
                                  struct monst *) NONNULLPTRS;
extern void back_on_ground(boolean);
extern void rescued_from_terrain(int);
extern boolean drown(void);
extern void drain_en(int, boolean);
extern int dountrap(void);
extern int could_untrap(boolean, boolean);
extern void cnv_trap_obj(int, int, struct trap *, boolean) NONNULLARG3;
extern int untrap(boolean, coordxy, coordxy, struct obj *) NO_NNARGS;
extern boolean openholdingtrap(struct monst *, boolean *) NO_NNARGS;
extern boolean closeholdingtrap(struct monst *, boolean *) NO_NNARGS;
extern boolean openfallingtrap(struct monst *, boolean, boolean *) NONNULLARG3;
extern boolean chest_trap(struct obj *, int, boolean) NONNULLARG1;
extern void deltrap(struct trap *) NONNULLARG1;
extern boolean delfloortrap(struct trap *) NO_NNARGS;
extern struct trap *t_at(coordxy, coordxy);
extern int count_traps(int);
extern void b_trapped(const char *, int) NONNULLARG1;
extern boolean unconscious(void);
extern void blow_up_landmine(struct trap *) NONNULLARG1;
extern int launch_obj(short, coordxy, coordxy, coordxy, coordxy, int);
extern boolean launch_in_progress(void);
extern void force_launch_placement(void);
extern boolean uteetering_at_seen_pit(struct trap *) NO_NNARGS;
extern boolean uescaped_shaft(struct trap *) NO_NNARGS;
extern boolean lava_effects(void);
extern void sink_into_lava(void);
extern void sokoban_guilt(void);
extern const char * trapname(int, boolean);
extern void ignite_items(struct obj *) NO_NNARGS;
extern void trap_ice_effects(coordxy x, coordxy y, boolean ice_is_melting);
extern void trap_sanity_check(void);

/* ### u_init.c ### */

extern void u_init(void);

/* ### uhitm.c ### */

extern void dynamic_multi_reason(struct monst *, const char *, boolean) NONNULLARG12;
extern void erode_armor(struct monst *, int) NONNULLARG1;
extern boolean attack_checks(struct monst *, struct obj *) NONNULLARG1;
extern void check_caitiff(struct monst *) NONNULLARG1;
extern void mon_maybe_unparalyze(struct monst *) NONNULLARG1;
extern int find_roll_to_hit(struct monst *, uchar, struct obj *,
                            int *, int *) NONNULLARG145;
extern boolean force_attack(struct monst *, boolean) NONNULLARG1;
extern boolean do_attack(struct monst *) NONNULLARG1;
extern boolean hmon(struct monst *, struct obj *, int, int) NONNULLARG1;
extern boolean shade_miss(struct monst *, struct monst *, struct obj *,
                          boolean, boolean) NONNULLARG12;
extern void mhitm_ad_rust(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_corr(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_dcay(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_dren(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_drli(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_fire(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_cold(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_elec(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_acid(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_sgld(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_tlpt(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
/* gazemm() calls mhitm_ad_blnd with a NULL 4th arg */
extern void mhitm_ad_blnd(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLARG123;
extern void mhitm_ad_curs(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_drst(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_drin(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_stck(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_wrap(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_plys(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_slee(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_slim(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_ench(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_slow(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_conf(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_poly(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_famn(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_pest(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_deth(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_halu(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_phys(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_ston(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_were(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_heal(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_stun(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_legs(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_dgst(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_samu(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_dise(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_sedu(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_ad_ssex(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *) NONNULLPTRS;
extern void mhitm_adtyping(struct monst *, struct attack *, struct monst *,
                           struct mhitm_data *) NONNULLPTRS;
extern boolean do_stone_u(struct monst *) NONNULLARG1;
extern void do_stone_mon(struct monst *, struct attack *, struct monst *,
                         struct mhitm_data *) NONNULLARG134;
extern int damageum(struct monst *, struct attack *, int) NONNULLARG12;
/* domove_fight_empty passes NULL to explum arg1 */
extern int explum(struct monst *, struct attack *) NONNULLARG2;
extern void missum(struct monst *, struct attack *, boolean) NONNULLARG12;
extern boolean m_is_steadfast(struct monst *) NONNULLARG1;
extern boolean mhitm_knockback(struct monst *, struct monst *,struct attack *,
                               int *, boolean) NONNULLPTRS;
extern int passive(struct monst *, struct obj *, boolean, boolean, uchar,
                   boolean) NONNULLARG1;
extern void passive_obj(struct monst *, struct obj *, struct attack *) NONNULLARG1;
extern void stumble_onto_mimic(struct monst *) NONNULLARG1;
extern int flash_hits_mon(struct monst *, struct obj *) NONNULLARG12;
extern void light_hits_gremlin(struct monst *, int) NONNULLARG1;


/* ### unixmain.c ### */
#ifdef UNIX
#ifdef PORT_HELP
extern void port_help(void);
#endif
extern void sethanguphandler(void(*)(int));
extern boolean authorize_wizard_mode(void);
extern boolean authorize_explore_mode(void);
extern void append_slash(char *) NONNULLARG1;
extern boolean check_user_string(const char *) NONNULLARG1;
extern char *get_login_name(void);
extern unsigned long sys_random_seed(void);
#endif /* UNIX */

/* ### unixtty.c ### */

#if defined(UNIX) || defined(__BEOS__)
extern void gettty(void);
extern void settty(const char *) NO_NNARGS;
extern void setftty(void);
extern void intron(void);
extern void introff(void);
ATTRNORETURN extern void error(const char *, ...) PRINTF_F(1, 2) NORETURN;
#ifdef ENHANCED_SYMBOLS
extern void tty_utf8graphics_fixup(void);
#endif
#endif /* UNIX || __BEOS__ */

/* ### unixunix.c ### */

#ifdef UNIX
extern void getlock(void);
extern void ask_about_panic_save(void);
extern void regularize(char *) NONNULLARG1;
#if defined(TIMED_DELAY) && !defined(msleep) && defined(SYSV)
extern void msleep(unsigned);
#endif
#ifdef SHELL
extern int dosh(void);
#endif /* SHELL */
#if defined(SHELL) || defined(DEF_PAGER) || defined(DEF_MAILREADER)
extern int child(int);
#endif
#ifdef PANICTRACE
extern boolean file_exists(const char *) NONNULLARG1;
#endif
#endif /* UNIX */

/* ### unixres.c ### */

#ifdef UNIX
#ifdef GNOME_GRAPHICS
extern int hide_privileges(boolean);
#endif
#endif /* UNIX */

/* ### utf8map.c ### */

#ifdef ENHANCED_SYMBOLS
extern char *mixed_to_utf8(char *buf, size_t bufsz, const char *str,
                           int *) NONNULLARG1;
void free_all_glyphmap_u(void);
int set_map_u(glyph_map *gm, uint32 utf32ch, const uint8 *utf8str) NONNULLPTRS;
#endif /* ENHANCED_SYMBOLS */
extern void reset_customsymbols(void);

/* ### vault.c ### */

extern void newegd(struct monst *) NONNULLARG1;
extern void free_egd(struct monst *) NONNULLARG1;
extern boolean grddead(struct monst *) NONNULLARG1;
extern struct monst *findgd(void);
extern void vault_summon_gd(void);
extern char vault_occupied(char *) NONNULLARG1;
extern void uleftvault(struct monst *); /* NULL leads to impossible() */
extern void invault(void);
extern int gd_move(struct monst *) NONNULLARG1;
extern void paygd(boolean);
extern long hidden_gold(boolean);
extern boolean gd_sound(void);
extern void vault_gd_watching(unsigned int);

/* ### version.c ### */

extern char *version_string(char *, size_t bufsz) NONNULL NONNULLARG1;
extern char *getversionstring(char *, size_t bufsz) NONNULL NONNULLARG1;
extern char *status_version(char *, size_t, boolean) NONNULL NONNULLARG1;
extern int doversion(void);
extern int doextversion(void);
#ifdef MICRO
extern boolean comp_times(long);
#endif
extern boolean check_version(struct version_info *, const char *, boolean,
                             unsigned long) NONNULLARG12;
extern boolean uptodate(NHFILE *, const char *, unsigned long) NONNULLARG1;
extern void store_formatindicator(NHFILE *) NONNULLARG1;
extern void store_version(NHFILE *) NONNULLARG1;
extern unsigned long get_feature_notice_ver(char *) NO_NNARGS;
extern unsigned long get_current_feature_ver(void);
extern const char *copyright_banner_line(int) NONNULL;
extern void early_version_info(boolean);
extern void dump_version_info(void);

/* ### video.c ### */

#ifdef MSDOS
extern int assign_video(char *) NONNULLARG1;
#ifdef NO_TERMS
extern void gr_init(void);
extern void gr_finish(void);
#endif
extern void tileview(boolean);
#endif
#ifdef VIDEOSHADES
extern int assign_videoshades(char *) NONNULLARG1;
extern int assign_videocolors(char *) NONNULLARG1;
#endif

/* ### vision.c ### */

extern void vision_init(void);
extern int does_block(int, int, struct rm *) NONNULLARG3;
extern void vision_reset(void);
extern void vision_recalc(int);
extern void block_point(int, int);
extern void unblock_point(int, int);
extern boolean clear_path(int, int, int, int);
extern void do_clear_area(coordxy, coordxy, int,
                          void(*)(coordxy, coordxy, void *), genericptr_t);
extern unsigned howmonseen(struct monst *) NONNULLARG1;

#ifdef VMS

/* ### vmsfiles.c ### */

extern int vms_link(const char *, const char *);
extern int vms_unlink(const char *);
extern int vms_creat(const char *, unsigned int);
extern int vms_open(const char *, int, unsigned int);
extern boolean same_dir(const char *, const char *);
extern int c__translate(int);
extern char *vms_basename(const char *, boolean);

/* ### vmsmail.c ### */

extern unsigned long init_broadcast_trapping(void);
extern unsigned long enable_broadcast_trapping(void);
extern unsigned long disable_broadcast_trapping(void);
#if 0
extern struct mail_info *parse_next_broadcast(void);
#endif /*0*/

/* ### vmsmain.c ### */

extern int main(int, char **);
#ifdef CHDIR
extern void chdirx(const char *, boolean);
#endif /* CHDIR */
extern void sethanguphandler(void(*)(int));
extern boolean authorize_wizard_mode(void);
extern boolean authorize_explore_mode(void);

/* ### vmsmisc.c ### */

ATTRNORETURN extern void vms_abort(void) NORETURN;
ATTRNORETURN extern void vms_exit(int) NORETURN;
#ifdef PANICTRACE
extern void vms_traceback(int);
#endif

/* ### vmstty.c ### */

extern int vms_getchar(void);
extern void gettty(void);
extern void settty(const char *);
extern void shuttty(const char *);
extern void setftty(void);
extern void intron(void);
extern void introff(void);
ATTRNORETURN extern void error (const char *, ...) PRINTF_F(1, 2) NORETURN;
#ifdef TIMED_DELAY
extern void msleep(unsigned);
#endif
#ifdef SIGWINCH
extern void getwindowsz(void);
#endif
#ifdef ENHANCED_SYMBOLS
extern void tty_utf8graphics_fixup(void);
#endif

/* ### vmsunix.c ### */

extern void getlock(void);
extern void regularize(char *);
extern int vms_getuid(void);
extern boolean file_is_stmlf(int);
extern int vms_define(const char *, const char *, int);
extern int vms_putenv(const char *);
extern char *verify_termcap(void);
#if defined(CHDIR) || defined(SHELL) || defined(SECURE)
extern void privoff(void);
extern void privon(void);
#endif
#ifdef SYSCF
extern boolean check_user_string(const char *);
#endif
#ifdef SHELL
extern int dosh(void);
#endif
#if defined(SHELL) || defined(MAIL)
extern int vms_doshell(const char *, boolean);
#endif
#ifdef SUSPEND
extern int dosuspend(void);
#endif
#ifdef SELECTSAVED
extern int vms_get_saved_games(const char *, char ***);
#endif

#endif /* VMS */

/* ### weapon.c ### */

extern const char *weapon_descr(struct obj *) NONNULLARG1;
extern int hitval(struct obj *, struct monst *) NONNULLARG12;
extern int dmgval(struct obj *, struct monst *) NONNULLARG12;
extern int special_dmgval(struct monst *, struct monst *, long, long *) NONNULLARG12;
extern void silver_sears(struct monst *, struct monst *, long) NONNULLARG2;
extern struct obj *select_rwep(struct monst *) NONNULLARG1;
extern boolean monmightthrowwep(struct obj *) NONNULLARG1;
extern struct obj *select_hwep(struct monst *) NONNULLARG1;
extern void possibly_unwield(struct monst *, boolean) NONNULLARG1;
extern int mon_wield_item(struct monst *) NONNULLARG1;
extern void mwepgone(struct monst *) NONNULLARG1;
extern int abon(void);
extern int dbon(void);
extern void wet_a_towel(struct obj *, int, boolean) NONNULLARG1;
extern void dry_a_towel(struct obj *, int, boolean) NONNULLARG1;
extern char *skill_level_name(int, char *) NONNULLARG2;
extern const char *skill_name(int);
extern boolean can_advance(int, boolean);
extern int enhance_weapon_skill(void);
extern void unrestrict_weapon_skill(int);
extern void use_skill(int, int);
extern void add_weapon_skill(int);
extern void lose_weapon_skill(int);
extern void drain_weapon_skill(int);
extern int weapon_type(struct obj *) NO_NNARGS;
extern int uwep_skill_type(void);
/* find_roll_to_hit() calls weapon_hit_bonus() with a NULL argument,
   preventing NONNULLARG1 */
extern int weapon_hit_bonus(struct obj *) NO_NNARGS;
extern int weapon_dam_bonus(struct obj *) NO_NNARGS;
extern void skill_init(const struct def_skill *) NONNULLARG1;
extern void setmnotwielded(struct monst *, struct obj *) NONNULLARG1;

/* ### were.c ### */

extern void were_change(struct monst *) NONNULLARG1;
extern int counter_were(int);
extern int were_beastie(int);
extern void new_were(struct monst *) NONNULLARG1;
extern int were_summon(struct permonst *, boolean, int *, char *) NONNULLARG13;
extern void you_were(void);
extern void you_unwere(boolean);
extern void set_ulycn(int);

/* ### wield.c ### */

extern void setuwep(struct obj *) NO_NNARGS; /* NULL:ball.c, do.c */
extern const char *empty_handed(void);
extern void setuqwep(struct obj *) NO_NNARGS;  /* NULL:ball.c, do.c */
extern void setuswapwep(struct obj *) NO_NNARGS; /* NULL: ball.c, do.c */
extern int dowield(void);
extern int doswapweapon(void);
extern int dowieldquiver(void);
extern int doquiver_core(const char *) NONNULLARG1;
extern boolean wield_tool(struct obj *, const char *) NONNULLARG1;
extern int can_twoweapon(void);
extern void drop_uswapwep(void);
extern int dotwoweapon(void);
extern void uwepgone(void);
extern void uswapwepgone(void);
extern void uqwepgone(void);
extern void set_twoweap(boolean);
extern void untwoweapon(void);
extern int chwepon(struct obj *, int) NO_NNARGS;
extern int welded(struct obj *) NO_NNARGS;
extern void weldmsg(struct obj *) NONNULLARG1;
extern boolean mwelded(struct obj *) NO_NNARGS;

/* ### windows.c ### */

extern void choose_windows(const char *) NONNULLARG1;
#ifdef WINCHAIN
void addto_windowchain(const char *s) NONNULLARG1;
void commit_windowchain(void);
#endif
#ifdef TTY_GRAPHICS
extern boolean check_tty_wincap(unsigned long);
extern boolean check_tty_wincap2(unsigned long);
#endif
extern boolean genl_can_suspend_no(void);
extern boolean genl_can_suspend_yes(void);
extern char genl_message_menu(char, int, const char *) NONNULLARG3;
extern void genl_preference_update(const char *) NO_NNARGS;
extern char *genl_getmsghistory(boolean);
extern void genl_putmsghistory(const char *, boolean) NONNULLARG1;
#ifdef HANGUPHANDLING
extern void nhwindows_hangup(void);
#endif
extern void genl_status_init(void);
extern void genl_status_finish(void);
extern void genl_status_enablefield(int, const char *, const char *,
                                    boolean) NONNULLPTRS;
extern void genl_status_update(int, genericptr_t, int, int, int,
                               unsigned long *) NONNULLARG2;
#ifdef DUMPLOG
extern char *dump_fmtstr(const char *, char *, boolean) NONNULLPTRS;
#endif
extern void dump_open_log(time_t);
extern void dump_close_log(void);
extern void dump_redirect(boolean);
extern void dump_forward_putstr(winid, int, const char*, int) NONNULLARG3;
extern int has_color(int);
extern int glyph2ttychar(int);
extern int glyph2symidx(int);
extern char *encglyph(int);
extern int decode_glyph(const char *str, int *glyph_ptr) NONNULLPTRS;
extern char *decode_mixed(char *, const char *) NONNULLARG1;
extern void genl_putmixed(winid, int, const char *) NONNULLARG3;
extern void genl_display_file(const char *, boolean) NONNULLARG1;
extern boolean menuitem_invert_test(int, unsigned, boolean);
extern const char *mixed_to_glyphinfo(const char *str,
                                      glyph_info *gip) NO_NNARGS;
extern void adjust_menu_promptstyle(winid, color_attr *) NONNULLARG2;
extern int choose_classes_menu(const char *, int, boolean,
                               char *, char *) NONNULLARG1;
extern void add_menu(winid, const glyph_info *, const ANY_P *,
                     char, char, int, int, const char *, unsigned int);
extern void add_menu_heading(winid, const char *) NONNULLARG2;
extern void add_menu_str(winid, const char *) NONNULLARG2;
extern int select_menu(winid, int, menu_item **) NONNULLARG3;
extern void getlin(const char *, char *) NONNULLARG2;

/* ### windsys.c ### */

#ifdef WIN32
extern void nethack_enter_windows(void);
#endif

/* ### wizard.c ### */

extern void amulet(void);
extern int mon_has_amulet(struct monst *) NONNULLARG1;
extern int mon_has_special(struct monst *) NONNULLARG1;
extern void choose_stairs(coordxy *, coordxy *, boolean) NONNULLARG12;
extern int tactics(struct monst *) NONNULLARG1;
extern boolean has_aggravatables(struct monst *) NONNULLARG1;
extern void aggravate(void);
extern void clonewiz(void);
extern int pick_nasty(int);
extern int nasty(struct monst *) NO_NNARGS;
extern void resurrect(void);
extern void intervene(void);
extern void wizdeadorgone(void);
extern void cuss(struct monst *) NONNULLARG1;

/* ### wizcmds.c ### */

extern int wiz_custom(void);
extern int wiz_detect(void);
extern int wiz_flip_level(void);
extern int wiz_fuzzer(void);
extern int wiz_genesis(void);
extern int wiz_identify(void);
extern int wiz_intrinsic(void);
extern int wiz_kill(void);
extern int wiz_level_change(void);
extern int wiz_level_tele(void);
extern int wiz_load_lua(void);
extern int wiz_load_splua(void);
extern int wiz_makemap(void);
extern int wiz_map(void);
extern int wiz_migrate_mons(void);
extern int wiz_panic(void);
extern int wiz_polyself(void);
extern int wiz_rumor_check(void);
extern int wiz_show_seenv(void);
extern int wiz_show_stats(void);
extern int wiz_show_vision(void);
extern int wiz_show_wmodes(void);
extern int wiz_smell(void);
extern int wiz_telekinesis(void);
extern int wiz_where(void);
extern int wiz_wish(void);
extern void makemap_remove_mons(void);
extern void wiz_levltyp_legend(void);
extern void wiz_map_levltyp(void);
extern void wizcustom_callback(winid win, int glyphnum, char *id);
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED) || defined(DEBUG)
extern int wiz_display_macros(void);
extern int wiz_mon_diff(void);
#endif
extern void sanity_check(void);

/* ### worm.c ### */

extern int get_wormno(void);
extern void initworm(struct monst *, int) NONNULLARG1;
extern void worm_move(struct monst *) NONNULLARG1;
extern void worm_nomove(struct monst *) NONNULLARG1;
extern void wormgone(struct monst *) NONNULLARG1;
extern int wormhitu(struct monst *) NONNULLARG1;
extern void cutworm(struct monst *, coordxy, coordxy, boolean) NONNULLARG1;
extern void see_wsegs(struct monst *) NONNULLARG1;
extern void detect_wsegs(struct monst *, boolean) NONNULLARG1;
extern void save_worm(NHFILE *) NONNULLARG1;
extern void rest_worm(NHFILE *) NONNULLARG1;
extern void place_wsegs(struct monst *, struct monst *) NONNULLARG1;
extern void sanity_check_worm(struct monst *);  /* NULL leads to impossible */
extern void wormno_sanity_check(void);
extern void remove_worm(struct monst *) NONNULLARG1;
extern void place_worm_tail_randomly(struct monst *, coordxy, coordxy) NONNULLARG1;
extern int size_wseg(struct monst *) NONNULLARG1;
extern int count_wsegs(struct monst *) NONNULLARG1;
extern boolean worm_known(struct monst *) NONNULLARG1;
extern boolean worm_cross(int, int, int, int);
extern int wseg_at(struct monst *, int, int) NO_NNARGS;
extern void flip_worm_segs_vertical(struct monst *, int, int) NONNULLARG1;
extern void flip_worm_segs_horizontal(struct monst *, int, int) NONNULLARG1;

/* ### worn.c ### */

extern void recalc_telepat_range(void);
extern void setworn(struct obj *, long) NO_NNARGS; /* has tests for obj */
extern void setnotworn(struct obj *) NO_NNARGS; /* has tests for obj */
extern void allunworn(void);
extern struct obj *wearmask_to_obj(long);
extern long wearslot(struct obj *) NONNULLARG1;
extern void check_wornmask_slots(void);
extern void mon_set_minvis(struct monst *) NONNULLARG1;
extern void mon_adjust_speed(struct monst *, int, struct obj *) NONNULLARG1;
extern void update_mon_extrinsics(struct monst *, struct obj *, boolean,
                                  boolean) NONNULLARG12;
extern int find_mac(struct monst *) NONNULLARG1;
extern void m_dowear(struct monst *, boolean) NONNULLARG1;
extern struct obj *which_armor(struct monst *, long) NONNULLARG1;
extern void mon_break_armor(struct monst *, boolean) NONNULLARG1;
extern void bypass_obj(struct obj *) NONNULLARG1;
extern void clear_bypasses(void);
/* callers don't check gi.invent before passing to bypass_objlist */
extern void bypass_objlist(struct obj *, boolean) NO_NNARGS;
extern struct obj *nxt_unbypassed_obj(struct obj *) NO_NNARGS;
extern struct obj *nxt_unbypassed_loot(Loot *, struct obj *) NONNULLARG1;
extern int racial_exception(struct monst *, struct obj *) NONNULLARG12;
extern void extract_from_minvent(struct monst *, struct obj *, boolean,
                                 boolean) NONNULLARG12;

/* ### write.c ### */

extern int dowrite(struct obj *) NONNULLARG1;

/* ### zap.c ### */

extern void learnwand(struct obj *) NONNULLARG1;
extern int bhitm(struct monst *, struct obj *) NONNULLARG12;
extern void release_hold(void);
extern void probe_monster(struct monst *) NONNULLARG1;
extern boolean get_obj_location(struct obj *, coordxy *, coordxy *,
                                int) NONNULLPTRS;
extern boolean get_mon_location(struct monst *, coordxy *, coordxy *,
                                int) NONNULLPTRS;
extern struct monst *get_container_location(struct obj *,
                                            int *, int *) NONNULLARG2;
extern struct monst *montraits(struct obj *, coord *, boolean) NONNULLARG12;
extern struct monst *revive(struct obj *, boolean) NONNULLARG1;
extern int unturn_dead(struct monst *) NONNULLARG1;
extern void unturn_you(void);
extern void cancel_item(struct obj *) NONNULLARG1;
extern void blank_novel(struct obj *) NONNULLARG1;
extern boolean drain_item(struct obj *, boolean) NO_NNARGS; /* tests !obj */
extern boolean obj_unpolyable(struct obj *) NONNULLARG1;
extern struct obj *poly_obj(struct obj *, int) NONNULLARG1;
extern boolean obj_resists(struct obj *, int, int) NONNULLARG1;
extern boolean obj_shudders(struct obj *) NONNULLARG1;
extern void do_osshock(struct obj *) NONNULLARG1;
extern int bhito(struct obj *, struct obj *) NONNULLARG12;
extern int bhitpile(struct obj *, int(*)(struct obj *, struct obj *),
                    coordxy, coordxy, schar) NONNULLARG12;
extern int zappable(struct obj *) NONNULLARG1;
extern void do_enlightenment_effect(void);
extern void zapnodir(struct obj *) NONNULLARG1;
extern int dozap(void);
extern int zapyourself(struct obj *, boolean) NONNULLARG1;
extern void ubreatheu(struct attack *) NONNULLARG1;
extern int lightdamage(struct obj *, boolean, int) NONNULLARG1;
extern boolean flashburn(long, boolean);
extern boolean cancel_monst(struct monst *, struct obj *, boolean, boolean,
                            boolean) NONNULLARG12;
extern void zapsetup(void);
extern void zapwrapup(void);
extern void weffects(struct obj *) NONNULLARG1;
extern int spell_damage_bonus(int);
extern const char *exclam(int force) NONNULL;
extern void hit(const char *, struct monst *, const char *) NONNULLPTRS;
extern void miss(const char *, struct monst *) NONNULLPTRS;
extern struct monst *bhit(coordxy, coordxy, int, enum bhit_call_types,
                          int(*)(struct monst *, struct obj *),
                          int(*)(struct obj *, struct obj *),
                          struct obj **) NONNULLARG7;
extern struct monst *boomhit(struct obj *, coordxy, coordxy) NONNULLARG1;
extern int zhitm(struct monst *, int, int, struct obj **) NONNULLPTRS;
extern int burn_floor_objects(coordxy, coordxy, boolean, boolean);
extern void ubuzz(int, int);
extern void buzz(int, int, coordxy, coordxy, int, int);
extern void dobuzz(int, int, coordxy, coordxy, int, int, boolean);
extern void melt_ice(coordxy, coordxy, const char *) NO_NNARGS;
extern void start_melt_ice_timeout(coordxy, coordxy, long);
extern void melt_ice_away(union any *, long) NONNULLARG1;
extern int zap_over_floor(coordxy, coordxy, int, boolean *,
                          boolean, short) NONNULLARG4;
extern void mon_spell_hits_spot(struct monst *, int, coordxy x, coordxy y);
extern void fracture_rock(struct obj *) NONNULLARG1;
extern boolean break_statue(struct obj *) NONNULLARG1;
extern int u_adtyp_resistance_obj(int);
extern boolean inventory_resistance_check(int);
extern char *item_what(int);
extern int destroy_items(struct monst *, int, int) NONNULLARG1;
extern int resist(struct monst *, char, int, int) NONNULLARG1;
extern void makewish(void);
extern const char *flash_str(int, boolean) NONNULL;

/* ### unixmain.c, windsys.c ### */

#ifdef RUNTIME_PORT_ID
extern char *get_port_id(char *);
#endif
#ifdef RUNTIME_PASTEBUF_SUPPORT
extern void port_insert_pastebuf(char *);
#endif

#endif /* !MAKEDEFS_C && !MDLIB_C */

#endif /* EXTERN_H */

/*extern.h*/
