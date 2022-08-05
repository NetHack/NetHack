/* NetHack 3.7	extern.h	$NHDT-Date: 1657918089 2022/07/15 20:48:09 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.1132 $ */
/* Copyright (c) Steve Creps, 1988.				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef EXTERN_H
#define EXTERN_H

/* ### alloc.c ### */

#if 0
extern long *alloc(unsigned int);
#endif
extern char *fmt_ptr(const void *);

/* This next pre-processor directive covers almost the entire file,
 * interrupted only occasionally to pick up specific functions as needed. */
#if !defined(MAKEDEFS_C) && !defined(MDLIB_C)

/* ### allmain.c ### */

extern void early_init(void);
extern void moveloop_core(void);
extern void moveloop(boolean);
extern void stop_occupation(void);
extern void display_gamewindows(void);
extern void newgame(void);
extern void welcome(boolean);
extern int argcheck(int, char **, enum earlyarg);
extern long timet_to_seconds(time_t);
extern long timet_delta(time_t, time_t);
#ifndef NODUMPENUMS
extern void dump_enums(void);
#endif

/* ### apply.c ### */

extern int doapply(void);
extern int dorub(void);
extern int dojump(void);
extern int jump(int);
extern int number_leashed(void);
extern void o_unleash(struct obj *);
extern void m_unleash(struct monst *, boolean);
extern void unleash_all(void);
extern boolean leashable(struct monst *);
extern boolean next_to_u(void);
extern struct obj *get_mleash(struct monst *);
extern const char *beautiful(void);
extern void check_leash(coordxy, coordxy);
extern boolean um_dist(coordxy, coordxy, xint16);
extern boolean snuff_candle(struct obj *);
extern boolean snuff_lit(struct obj *);
extern boolean splash_lit(struct obj *);
extern boolean catch_lit(struct obj *);
extern void use_unicorn_horn(struct obj **);
extern boolean tinnable(struct obj *);
extern void reset_trapset(void);
extern int use_whip(struct obj *);
extern int use_pole(struct obj *, boolean);
extern void fig_transform(union any *, long);
extern int unfixable_trouble_count(boolean);

/* ### artifact.c ### */

extern void init_artifacts(void);
extern void save_artifacts(NHFILE *);
extern void restore_artifacts(NHFILE *);
extern const char *artiname(int);
extern struct obj *mk_artifact(struct obj *, aligntyp);
extern const char *artifact_name(const char *, short *, boolean);
extern boolean exist_artifact(int, const char *);
extern void artifact_exists(struct obj *, const char *, boolean, unsigned);
extern void found_artifact(int);
extern void find_artifact(struct obj *);
extern int nartifact_exist(void);
extern void artifact_origin(struct obj *, unsigned);
extern boolean arti_immune(struct obj *, int);
extern boolean spec_ability(struct obj *, unsigned long);
extern boolean confers_luck(struct obj *);
extern boolean arti_reflects(struct obj *);
extern boolean shade_glare(struct obj *);
extern boolean restrict_name(struct obj *, const char *);
extern boolean defends(int, struct obj *);
extern boolean defends_when_carried(int, struct obj *);
extern boolean protects(struct obj *, boolean);
extern void set_artifact_intrinsic(struct obj *, boolean, long);
extern int touch_artifact(struct obj *, struct monst *);
extern int spec_abon(struct obj *, struct monst *);
extern int spec_dbon(struct obj *, struct monst *, int);
extern void discover_artifact(xint16);
extern boolean undiscovered_artifact(xint16);
extern int disp_artifact_discoveries(winid);
extern void dump_artifact_info(winid);
extern boolean artifact_hit(struct monst *, struct monst *, struct obj *,
                            int *, int);
extern int doinvoke(void);
extern boolean finesse_ahriman(struct obj *);
extern void arti_speak(struct obj *);
extern boolean artifact_light(struct obj *);
extern long spec_m2(struct obj *);
extern boolean artifact_has_invprop(struct obj *, uchar);
extern long arti_cost(struct obj *);
extern struct obj *what_gives(long *);
extern const char *glow_color(int);
extern const char *glow_verb(int, boolean);
extern void Sting_effects(int);
extern int retouch_object(struct obj **, boolean);
extern void retouch_equipment(int);
extern void mkot_trap_warn(void);
extern boolean is_magic_key(struct monst *, struct obj *);
extern struct obj *has_magic_key(struct monst *);

/* ### attrib.c ### */

extern boolean adjattrib(int, int, int);
extern void gainstr(struct obj *, int, boolean);
extern void losestr(int);
extern void poisontell(int, boolean);
extern void poisoned(const char *, int, const char *, int, boolean);
extern void change_luck(schar);
extern int stone_luck(boolean);
extern void set_moreluck(void);
extern void restore_attrib(void);
extern void exercise(int, boolean);
extern void exerchk(void);
extern void init_attr(int);
extern void redist_attr(void);
extern void adjabil(int, int);
extern int newhp(void);
extern int minuhpmax(int);
extern void setuhpmax(int);
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
extern boolean drag_ball(coordxy, coordxy, int *, coordxy *, coordxy *, coordxy *,
                         coordxy *, boolean *, boolean);
extern void drop_ball(coordxy, coordxy);
extern void drag_down(void);
extern void bc_sanity_check(void);

/* ### bones.c ### */

extern void sanitize_name(char *);
extern void drop_upon_death(struct monst *, struct obj *, coordxy, coordxy);
extern boolean can_make_bones(void);
extern void savebones(int, time_t, struct obj *);
extern int getbones(void);
extern boolean bones_include_name(const char *);

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
extern void condopt(int, boolean *, boolean);
extern int parse_cond_option(boolean, char *);
extern void cond_menu(void);
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
extern const char *cmdname_from_func(int(*)(void), char *, boolean);
extern boolean redraw_cmd(char);
extern const char *levltyp_to_name(int);
extern void reset_occupations(void);
extern void set_occupation(int(*)(void), const char *, cmdcount_nht);
extern void cmdq_add_ec(int(*)(void));
extern void cmdq_add_key(char);
extern void cmdq_add_dir(schar, schar, schar);
extern void cmdq_add_userinput(void);
extern struct _cmd_queue *cmdq_pop(void);
extern struct _cmd_queue *cmdq_peek(void);
extern void cmdq_clear(void);
extern char pgetchar(void);
extern void pushch(char);
extern void savech(char);
extern void savech_extcmd(const char *, boolean);
extern char extcmd_initiator(void);
extern int doextcmd(void);
extern struct ext_func_tab *extcmds_getentry(int);
extern int count_bind_keys(void);
extern void get_changed_key_binds(strbuf_t *);
extern void handler_rebind_keys(void);
extern int extcmds_match(const char *, int, int **);
extern const char *key2extcmddesc(uchar);
extern boolean bind_specialkey(uchar, const char *);
extern void parseautocomplete(char *, boolean);
extern void reset_commands(boolean);
extern void update_rest_on_space(void);
extern void rhack(char *);
extern int doextlist(void);
extern int extcmd_via_menu(void);
extern int enter_explore_mode(void);
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
extern int get_adjacent_loc(const char *, const char *, coordxy, coordxy, coord *);
extern const char *click_to_cmd(coordxy, coordxy, int);
extern char get_count(const char *, char, long, cmdcount_nht *, unsigned);
#ifdef HANGUPHANDLING
extern void hangup(int);
extern void end_of_input(void);
#endif
extern char readchar(void);
extern char readchar_poskey(coordxy *, coordxy *, int *);
extern void sanity_check(void);
extern char* key2txt(uchar, char *);
extern char yn_function(const char *, const char *, char);
extern boolean paranoid_query(boolean, const char *);
extern void makemap_prepost(boolean, boolean);

/* ### date.c ### */

extern void populate_nomakedefs(struct version_info *);
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
extern boolean find_drawbridge(coordxy *, coordxy *);
extern boolean create_drawbridge(coordxy, coordxy, int, boolean);
extern void open_drawbridge(coordxy, coordxy);
extern void close_drawbridge(coordxy, coordxy);
extern void destroy_drawbridge(coordxy, coordxy);

/* ### decl.c ### */

extern void decl_globals_init(void);

/* ### detect.c ### */

extern boolean trapped_chest_at(int, coordxy, coordxy);
extern boolean trapped_door_at(int, coordxy, coordxy);
extern struct obj *o_in(struct obj *, char);
extern struct obj *o_material(struct obj *, unsigned);
extern int gold_detect(struct obj *);
extern int food_detect(struct obj *);
extern int object_detect(struct obj *, int);
extern int monster_detect(struct obj *, int);
extern int trap_detect(struct obj *);
extern const char *level_distance(d_level *);
extern void use_crystal_ball(struct obj **);
extern void do_mapping(void);
extern void do_vicinity_map(struct obj *);
extern void cvt_sdoor_to_door(struct rm *);
extern int findit(void);
extern int openit(void);
extern boolean detecting(void(*)(coordxy, coordxy, void *));
extern void find_trap(struct trap *);
extern void warnreveal(void);
extern int dosearch0(int);
extern int dosearch(void);
extern void sokoban_detect(void);
#ifdef DUMPLOG
extern void dump_map(void);
#endif
extern void reveal_terrain(int, int);
extern int wiz_mgender(void);

/* ### dig.c ### */

extern int dig_typ(struct obj *, coordxy, coordxy);
extern boolean is_digging(void);
extern int holetime(void);
extern boolean dig_check(struct monst *, boolean, coordxy, coordxy);
extern void digactualhole(coordxy, coordxy, struct monst *, int);
extern boolean dighole(boolean, boolean, coord *);
extern int use_pick_axe(struct obj *);
extern int use_pick_axe2(struct obj *);
extern boolean mdig_tunnel(struct monst *);
extern void draft_message(boolean);
extern void watch_dig(struct monst *, coordxy, coordxy, boolean);
extern void zap_dig(void);
extern struct obj *bury_an_obj(struct obj *, boolean *);
extern void bury_objs(int, int);
extern void unearth_objs(int, int);
extern void rot_organic(union any *, long);
extern void rot_corpse(union any *, long);
extern struct obj *buried_ball(coord *);
extern void buried_ball_to_punishment(void);
extern void buried_ball_to_freedom(void);
extern schar fillholetyp(coordxy, coordxy, boolean);
extern void liquid_flow(coordxy, coordxy, schar, struct trap *, const char *);
extern boolean conjoined_pits(struct trap *, struct trap *, boolean);
#if 0
extern void bury_monst(struct monst *);
extern void bury_you(void);
extern void unearth_you(void);
extern void escape_tomb(void);
extern void bury_obj(struct obj *);
#endif
#ifdef DEBUG
extern int wiz_debug_cmd_bury(void);
#endif

/* ### display.c ### */

extern int tp_sensemon(struct monst *);
extern int sensemon(struct monst *);
extern int mon_warning(struct monst *);
extern int mon_visible(struct monst *);
extern int see_with_infrared(struct monst *);
extern int canseemon(struct monst *);
extern int knowninvisible(struct monst *);
extern int is_safemon(struct monst *);
extern void magic_map_background(coordxy, coordxy, int);
extern void map_background(coordxy, coordxy, int);
extern void map_trap(struct trap *, int);
extern void map_object(struct obj *, int);
extern void map_invisible(coordxy, coordxy);
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
extern void see_traps(void);
extern void curs_on_u(void);
extern int doredraw(void);
extern void docrt(void);
extern void redraw_map(void);
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
extern int warning_of(struct monst *);
extern void map_glyphinfo(coordxy, coordxy, int, unsigned, glyph_info *);
extern void reset_glyphmap(enum glyphmap_change_triggers trigger);
extern int fn_cmap_to_glyph(int);

/* ### do.c ### */

extern int dodrop(void);
extern boolean boulder_hits_pool(struct obj *, coordxy, coordxy, boolean);
extern boolean flooreffects(struct obj *, coordxy, coordxy, const char *);
extern void doaltarobj(struct obj *);
extern void trycall(struct obj *);
extern boolean canletgo(struct obj *, const char *);
extern void dropx(struct obj *);
extern void dropy(struct obj *);
extern void dropz(struct obj *, boolean);
extern void obj_no_longer_held(struct obj *);
extern int doddrop(void);
extern int dodown(void);
extern int doup(void);
#ifdef INSURANCE
extern void save_currentstate(void);
#endif
extern void u_collide_m(struct monst *);
extern void goto_level(d_level *, boolean, boolean, boolean);
extern void hellish_smoke_mesg(void);
extern void maybe_lvltport_feedback(void);
extern void schedule_goto(d_level *, int, const char *, const char *);
extern void deferred_goto(void);
extern boolean revive_corpse(struct obj *);
extern void revive_mon(union any *, long);
extern void zombify_mon(union any *, long);
extern boolean cmd_safety_prevention(const char *, const char *, int *);
extern int donull(void);
extern int dowipe(void);
extern void legs_in_no_shape(const char *, boolean);
extern void set_wounded_legs(long, int);
extern void heal_legs(int);

/* ### do_name.c ### */

extern char *dxdy_to_dist_descr(coordxy, coordxy, boolean);
extern char *coord_desc(coordxy, coordxy, char *, char);
extern boolean getpos_menu(coord *, int);
extern int getpos(coord *, boolean, const char *);
extern void getpos_sethilite(void(*f)(int), boolean(*d)(coordxy,coordxy));
extern void new_mgivenname(struct monst *, int);
extern void free_mgivenname(struct monst *);
extern void new_oname(struct obj *, int);
extern void free_oname(struct obj *);
extern const char *safe_oname(struct obj *);
extern struct monst *christen_monst(struct monst *, const char *);
extern struct obj *oname(struct obj *, const char *, unsigned);
extern boolean objtyp_is_callable(int);
extern int name_ok(struct obj *);
extern int call_ok(struct obj *);
extern int docallcmd(void);
extern void docall(struct obj *);
extern const char *rndghostname(void);
extern char *x_monnam(struct monst *, int, const char *, int, boolean);
extern char *l_monnam(struct monst *);
extern char *mon_nam(struct monst *);
extern char *noit_mon_nam(struct monst *);
extern char *some_mon_nam(struct monst *);
extern char *Monnam(struct monst *);
extern char *noit_Monnam(struct monst *);
extern char *Some_Monnam(struct monst *);
extern char *noname_monnam(struct monst *, int);
extern char *m_monnam(struct monst *);
extern char *y_monnam(struct monst *);
extern char *Adjmonnam(struct monst *, const char *);
extern char *Amonnam(struct monst *);
extern char *a_monnam(struct monst *);
extern char *distant_monnam(struct monst *, int, char *);
extern char *mon_nam_too(struct monst *, struct monst *);
extern char *monverbself(struct monst *, char *, const char *, const char *);
extern char *minimal_monnam(struct monst *, boolean);
extern char *bogusmon(char *, char *);
extern char *rndmonnam(char *);
extern const char *hcolor(const char *);
extern const char *rndcolor(void);
extern const char *hliquid(const char *);
extern const char *roguename(void);
extern struct obj *realloc_obj(struct obj *, int, genericptr_t, int,
                               const char *);
extern char *coyotename(struct monst *, char *);
extern char *rndorcname(char *);
extern struct monst *christen_orc(struct monst *, const char *, const char *);
extern const char *noveltitle(int *);
extern const char *lookup_novel(const char *, int *);
#ifndef PMNAME_MACROS
extern int Mgender(struct monst *);
extern const char *pmname(struct permonst *, int);
#endif
extern const char *mon_pmname(struct monst *);
extern const char *obj_pmname(struct obj *);

/* ### do_wear.c ### */

extern const char *fingers_or_gloves(boolean);
extern void off_msg(struct obj *);
extern void toggle_displacement(struct obj *, long, boolean);
extern void set_wear(struct obj *);
extern boolean donning(struct obj *);
extern boolean doffing(struct obj *);
extern void cancel_doff(struct obj *, long);
extern void cancel_don(void);
extern int stop_donning(struct obj *);
extern int Armor_off(void);
extern int Armor_gone(void);
extern int Helmet_off(void);
extern void wielding_corpse(struct obj *, struct obj *, boolean);
extern int Gloves_off(void);
extern int Boots_on(void);
extern int Boots_off(void);
extern int Cloak_off(void);
extern int Shield_off(void);
extern int Shirt_off(void);
extern void Amulet_off(void);
extern void Ring_on(struct obj *);
extern void Ring_off(struct obj *);
extern void Ring_gone(struct obj *);
extern void Blindf_on(struct obj *);
extern void Blindf_off(struct obj *);
extern int dotakeoff(void);
extern int doremring(void);
extern int cursed(struct obj *);
extern int armoroff(struct obj *);
extern int canwearobj(struct obj *, long *, boolean);
extern int dowear(void);
extern int doputon(void);
extern void find_ac(void);
extern void glibr(void);
extern struct obj *some_armor(struct monst *);
extern struct obj *stuck_ring(struct obj *, int);
extern struct obj *unchanger(void);
extern void reset_remarm(void);
extern int doddoremarm(void);
extern int remarm_swapwep(void);
extern int destroy_arm(struct obj *);
extern void adj_abon(struct obj *, schar);
extern boolean inaccessible_equipment(struct obj *, const char *, boolean);

/* ### dog.c ### */

extern void newedog(struct monst *);
extern void free_edog(struct monst *);
extern void initedog(struct monst *);
extern struct monst *make_familiar(struct obj *, coordxy, coordxy, boolean);
extern struct monst *makedog(void);
extern void update_mlstmv(void);
extern void losedogs(void);
extern void mon_arrive(struct monst *, int);
extern void mon_catchup_elapsed_time(struct monst *, long);
extern void keepdogs(boolean);
extern void migrate_to_level(struct monst *, coordxy, coordxy, coord *);
extern void discard_migrations(void);
extern int dogfood(struct monst *, struct obj *);
extern boolean tamedog(struct monst *, struct obj *);
extern void abuse_dog(struct monst *);
extern void wary_dog(struct monst *, boolean);

/* ### dogmove.c ### */

extern boolean cursed_object_at(coordxy, coordxy);
extern struct obj *droppables(struct monst *);
extern int dog_nutrition(struct monst *, struct obj *);
extern int dog_eat(struct monst *, struct obj *, coordxy, coordxy, boolean);
extern int dog_move(struct monst *, int);
extern void finish_meating(struct monst *);

/* ### dokick.c ### */

extern boolean ghitm(struct monst *, struct obj *);
extern void container_impact_dmg(struct obj *, coordxy, coordxy);
extern int dokick(void);
extern boolean ship_object(struct obj *, coordxy, coordxy, boolean);
extern void obj_delivery(boolean);
extern void deliver_obj_to_mon(struct monst *mtmp, int, unsigned long);
extern schar down_gate(coordxy, coordxy);
extern void impact_drop(struct obj *, coordxy, coordxy, coordxy);

/* ### dothrow.c ### */

extern int multishot_class_bonus(int, struct obj *, struct obj *);
extern int dothrow(void);
extern int dofire(void);
extern void endmultishot(boolean);
extern void hitfloor(struct obj *, boolean);
extern void hurtle(int, int, int, boolean);
extern void mhurtle(struct monst *, int, int, int);
extern boolean throwing_weapon(struct obj *);
extern void throwit(struct obj *, long, boolean, struct obj *);
extern int omon_adj(struct monst *, struct obj *, boolean);
extern int thitmonst(struct monst *, struct obj *);
extern int hero_breaks(struct obj *, coordxy, coordxy, unsigned);
extern int breaks(struct obj *, coordxy, coordxy);
extern void release_camera_demon(struct obj *, coordxy, coordxy);
extern void breakobj(struct obj *, coordxy, coordxy, boolean, boolean);
extern boolean breaktest(struct obj *);
extern boolean walk_path(coord *, coord *, boolean(*)(void *, coordxy, coordxy),
                         genericptr_t);
extern boolean hurtle_jump(genericptr_t, coordxy, coordxy);
extern boolean hurtle_step(genericptr_t, coordxy, coordxy);

/* ### drawing.c ### */

extern int def_char_to_objclass(char);
extern int def_char_to_monclass(char);
extern int def_char_is_furniture(char);

/* ### dungeon.c ### */

extern void save_dungeon(NHFILE *, boolean, boolean);
extern void restore_dungeon(NHFILE *);
extern void insert_branch(branch *, boolean);
extern void init_dungeons(void);
extern s_level *find_level(const char *);
extern s_level *Is_special(d_level *);
extern branch *Is_branchlev(d_level *);
extern boolean builds_up(d_level *);
extern xint16 ledger_no(d_level *);
extern xint16 maxledgerno(void);
extern schar depth(d_level *);
extern xint16 dunlev(d_level *);
extern xint16 dunlevs_in_dungeon(d_level *);
extern xint16 ledger_to_dnum(xint16);
extern xint16 ledger_to_dlev(xint16);
extern xint16 deepest_lev_reached(boolean);
extern boolean on_level(d_level *, d_level *);
extern void next_level(boolean);
extern void prev_level(boolean);
extern void u_on_newpos(coordxy, coordxy);
extern void u_on_rndspot(int);
extern void stairway_add(coordxy, coordxy, boolean, boolean, d_level *);
extern void stairway_print(void);
extern void stairway_free_all(void);
extern stairway *stairway_at(coordxy, coordxy);
extern stairway *stairway_find(d_level *);
extern stairway *stairway_find_from(d_level *, boolean);
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
extern void get_level(d_level *, int);
extern boolean Is_botlevel(d_level *);
extern boolean Can_fall_thru(d_level *);
extern boolean Can_dig_down(d_level *);
extern boolean Can_rise_up(coordxy, coordxy, d_level *);
extern boolean has_ceiling(d_level *);
extern boolean In_quest(d_level *);
extern boolean In_mines(d_level *);
extern branch *dungeon_branch(const char *);
extern boolean at_dgn_entrance(const char *);
extern boolean In_hell(d_level *);
extern boolean In_V_tower(d_level *);
extern boolean On_W_tower_level(d_level *);
extern boolean In_W_tower(coordxy, coordxy, d_level *);
extern void find_hell(d_level *);
extern void goto_hell(boolean, boolean);
extern boolean single_level_branch(d_level *);
extern void assign_level(d_level *, d_level *);
extern void assign_rnd_level(d_level *, d_level *, int);
extern unsigned int induced_align(int);
extern boolean Invocation_lev(d_level *);
extern xint16 level_difficulty(void);
extern schar lev_by_name(const char *);
extern boolean known_branch_stairs(stairway *);
extern char *stairs_description(stairway *, char *, boolean);
extern schar print_dungeon(boolean, schar *, xint16 *);
extern char *get_annotation(d_level *);
extern int donamelevel(void);
extern int dooverview(void);
extern void show_overview(int, int);
extern void rm_mapseen(int);
extern void init_mapseen(d_level *);
extern void recalc_mapseen(void);
extern void mapseen_temple(struct monst *);
extern void room_discovered(int);
extern void recbranch_mapseen(d_level *, d_level *);
extern void overview_stats(winid, const char *, long *, long *);
extern void remdun_mapseen(int);
extern const char *endgamelevelname(char *, int);

/* ### eat.c ### */

extern void eatmupdate(void);
extern boolean is_edible(struct obj *);
extern void init_uhunger(void);
extern int Hear_again(void);
extern boolean eating_glob(struct obj *);
extern void reset_eat(void);
extern unsigned obj_nutrition(struct obj *);
extern int doeat(void);
extern int use_tin_opener(struct obj *);
extern void gethungry(void);
extern void morehungry(int);
extern void lesshungry(int);
extern boolean is_fainted(void);
extern void reset_faint(void);
extern int corpse_intrinsic(struct permonst *);
extern void violated_vegetarian(void);
extern void newuhs(boolean);
extern struct obj *floorfood(const char *, int);
extern void vomit(void);
extern int eaten_stat(int, struct obj *);
extern void food_disappears(struct obj *);
extern void food_substitution(struct obj *, struct obj *);
extern long temp_resist(int);
extern void eating_conducts(struct permonst *);
extern int eat_brains(struct monst *, struct monst *, boolean, int *);
extern void fix_petrification(void);
extern boolean should_givit(int, struct permonst *);
extern void consume_oeaten(struct obj *, int);
extern boolean maybe_finished_meal(boolean);
extern void cant_finish_meal(struct obj *);
extern void set_tin_variety(struct obj *, int);
extern int tin_variety_txt(char *, int *);
extern void tin_details(struct obj *, int, char *);
extern boolean Popeye(int);

/* ### end.c ### */

extern void done1(int);
extern int done2(void);
extern void done_in_by(struct monst *, int);
extern void done_object_cleanup(void);
#endif /* !MAKEDEFS_C && MDLIB_C */
extern void panic(const char *, ...) PRINTF_F(1, 2) NORETURN;
#if !defined(MAKEDEFS_C) && !defined(MDLIB_C)
extern void done(int);
extern void container_contents(struct obj *, boolean, boolean, boolean);
extern void nh_terminate(int) NORETURN;
extern void delayed_killer(int, int, const char *);
extern struct kinfo *find_delayed_killer(int);
extern void dealloc_killer(struct kinfo *);
extern void save_killers(NHFILE *);
extern void restore_killers(NHFILE *);
extern char *build_english_list(char *);
#if defined(PANICTRACE) && !defined(NO_SIGNAL)
extern void panictrace_setsignals(boolean);
#endif

/* ### engrave.c ### */

extern char *random_engraving(char *);
extern void wipeout_text(char *, int, unsigned);
extern boolean can_reach_floor(boolean);
extern void cant_reach_floor(coordxy, coordxy, boolean, boolean);
extern const char *surface(coordxy, coordxy);
extern const char *ceiling(coordxy, coordxy);
extern struct engr *engr_at(coordxy, coordxy);
extern boolean sengr_at(const char *, coordxy, coordxy, boolean);
extern void u_wipe_engr(int);
extern void wipe_engr_at(coordxy, coordxy, coordxy, boolean);
extern void read_engr_at(coordxy, coordxy);
extern void make_engr_at(coordxy, coordxy, const char *, long, xint16);
extern void del_engr_at(coordxy, coordxy);
extern int freehand(void);
extern int doengrave(void);
extern void sanitize_engravings(void);
extern void save_engravings(NHFILE *);
extern void rest_engravings(NHFILE *);
extern void engr_stats(const char *, char *, long *, long *);
extern void del_engr(struct engr *);
extern void rloc_engr(struct engr *);
extern void make_grave(coordxy, coordxy, const char *);

/* ### exper.c ### */

extern long newuexp(int);
extern int newpw(void);
extern int experience(struct monst *, int);
extern void more_experienced(int, int);
extern void losexp(const char *);
extern void newexplevel(void);
extern void pluslvl(boolean);
extern long rndexp(boolean);

/* ### explode.c ### */

extern void explode(coordxy, coordxy, int, int, char, int);
extern long scatter(coordxy, coordxy, int, unsigned int, struct obj *);
extern void splatter_burning_oil(coordxy, coordxy, boolean);
extern void explode_oil(struct obj *, coordxy, coordxy);
extern int adtyp_to_expltype(const int);
extern void mon_explodes(struct monst *, struct attack *);

/* ### extralev.c ### */

extern void makeroguerooms(void);
extern void corr(coordxy, coordxy);
extern void makerogueghost(void);

/* ### files.c ### */

#if !defined(CROSSCOMPILE) || defined(CROSSCOMPILE_TARGET)
extern int l_get_config_errors(lua_State *);
#endif
extern char *fname_encode(const char *, char, char *, char *, int);
extern char *fname_decode(char, char *, char *, int);
extern const char *fqname(const char *, int, int);
extern FILE *fopen_datafile(const char *, const char *, int);
extern void zero_nhfile(NHFILE *);
extern void close_nhfile(NHFILE *);
extern void rewind_nhfile(NHFILE *);
extern void set_levelfile_name(char *, int);
extern NHFILE *create_levelfile(int, char *);
extern NHFILE *open_levelfile(int, char *);
extern void delete_levelfile(int);
extern void clearlocks(void);
extern NHFILE *create_bonesfile(d_level *, char **, char *);
extern void commit_bonesfile(d_level *);
extern NHFILE *open_bonesfile(d_level *, char **);
extern int delete_bonesfile(d_level *);
extern void compress_bonesfile(void);
extern void set_savefile_name(boolean);
#ifdef INSURANCE
extern void save_savefile_name(NHFILE *);
#endif
#ifndef MICRO
extern void set_error_savefile(void);
#endif
extern NHFILE *create_savefile(void);
extern NHFILE *open_savefile(void);
extern int delete_savefile(void);
extern NHFILE *restore_saved_game(void);
extern void nh_compress(const char *);
extern void nh_uncompress(const char *);
extern boolean lock_file(const char *, int, int);
extern void unlock_file(const char *);
extern int do_write_config_file(void);
extern boolean parse_config_line(char *);
#ifdef USER_SOUNDS
extern boolean can_read_file(const char *);
#endif
extern void config_error_init(boolean, const char *, boolean);
extern void config_erradd(const char *);
extern int config_error_done(void);
extern boolean read_config_file(const char *, int);
extern void check_recordfile(const char *);
extern void read_wizkit(void);
extern boolean parse_conf_str(const char *str, boolean (*proc)(char *));
extern int read_sym_file(int);
extern int parse_sym_line(char *, int);
extern void paniclog(const char *, const char *);
extern void testinglog(const char *, const char *, const char *);
extern int validate_prefix_locations(char *);
#ifdef SELECTSAVED
extern char *plname_from_file(const char *);
#endif
extern char **get_saved_games(void);
extern void free_saved_games(char **);
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
extern boolean Death_quote(char *, int);
extern void livelog_add(long ll_type, const char *);

/* ### fountain.c ### */

extern void floating_above(const char *);
extern void dogushforth(int);
extern void dryup(coordxy, coordxy, boolean);
extern void drinkfountain(void);
extern void dipfountain(struct obj *);
extern void breaksink(coordxy, coordxy);
extern void drinksink(void);

/* ### hack.c ### */

extern boolean is_valid_travelpt(coordxy, coordxy);
extern anything *uint_to_any(unsigned);
extern anything *long_to_any(long);
extern anything *monst_to_any(struct monst *);
extern anything *obj_to_any(struct obj *);
extern boolean revive_nasty(coordxy, coordxy, const char *);
extern int still_chewing(coordxy, coordxy);
extern void movobj(struct obj *, coordxy, coordxy);
extern boolean may_dig(coordxy, coordxy);
extern boolean may_passwall(coordxy, coordxy);
extern boolean bad_rock(struct permonst *, coordxy, coordxy);
extern int cant_squeeze_thru(struct monst *);
extern boolean invocation_pos(coordxy, coordxy);
extern boolean test_move(coordxy, coordxy, coordxy, coordxy, int);
#ifdef DEBUG
extern int wiz_debug_cmd_traveldisplay(void);
#endif
extern boolean u_rooted(void);
extern boolean u_maybe_impaired(void);
extern const char *u_locomotion(const char *);
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
extern void losehp(int, const char *, boolean);
extern int weight_cap(void);
extern int inv_weight(void);
extern int near_capacity(void);
extern int calc_capacity(int);
extern int max_capacity(void);
extern boolean check_capacity(const char *);
extern int inv_cnt(boolean);
extern long money_cnt(struct obj *);
extern void spot_checks(coordxy, coordxy, schar);

/* ### hacklib.c ### */

extern boolean digit(char);
extern boolean letter(char);
extern char highc(char);
extern char lowc(char);
extern char *lcase(char *);
extern char *ucase(char *);
extern char *upstart(char *);
extern char *upwords(char *);
extern char *mungspaces(char *);
extern char *trimspaces(char *);
extern char *strip_newline(char *);
extern char *stripchars(char *, const char *, const char *);
extern char *stripdigits(char *);
extern char *eos(char *);
extern unsigned Strlen_(const char *, const char *, int);
extern boolean str_start_is(const char *, const char *, boolean);
extern boolean str_end_is(const char *, const char *);
extern int str_lines_maxlen(const char *);
extern char *strkitten(char *, char);
extern void copynchars(char *, const char *, int);
extern char chrcasecpy(int, int);
extern char *strcasecpy(char *, const char *);
extern char *s_suffix(const char *);
extern char *ing_suffix(const char *);
extern char *xcrypt(const char *, char *);
extern boolean onlyspace(const char *);
extern char *tabexpand(char *);
extern char *visctrl(char);
extern char *strsubst(char *, const char *, const char *);
extern int strNsubst(char *, const char *, const char *, int);
extern const char *ordin(int);
extern char *sitoa(int);
extern int sgn(int);
extern int rounddiv(long, int);
extern int dist2(coordxy, coordxy, coordxy, coordxy);
extern int isqrt(int);
extern int distmin(coordxy, coordxy, coordxy, coordxy);
extern boolean online2(int, int, int, int);
extern boolean pmatch(const char *, const char *);
extern boolean pmatchi(const char *, const char *);
extern boolean pmatchz(const char *, const char *);
#ifndef STRNCMPI
extern int strncmpi(const char *, const char *, int);
#endif
#ifndef STRSTRI
extern char *strstri(const char *, const char *);
#endif
extern boolean fuzzymatch(const char *, const char *, const char *, boolean);
extern void init_random(int(*fn)(int));
extern void reseed_random(int(*fn)(int));
extern time_t getnow(void);
extern int getyear(void);
#if 0
extern char *yymmdd(time_t);
#endif
extern long yyyymmdd(time_t);
extern long hhmmss(time_t);
extern char *yyyymmddhhmmss(time_t);
extern time_t time_from_yyyymmddhhmmss(char *);
extern int phase_of_the_moon(void);
extern boolean friday_13th(void);
extern int night(void);
extern int midnight(void);
extern void strbuf_init(strbuf_t *);
extern void strbuf_append(strbuf_t *, const char *);
extern void strbuf_reserve(strbuf_t *, int);
extern void strbuf_empty(strbuf_t *);
extern void strbuf_nl_to_crlf(strbuf_t *);
extern char *nonconst(const char *, char *, size_t);
extern int swapbits(int, int, int);
extern void shuffle_int_array(int *, int);
/* note: the snprintf CPP wrapper includes the "fmt" argument in "..."
   (__VA_ARGS__) to allow for zero arguments after fmt */
#define Snprintf(str, size, ...) \
    nh_snprintf(__func__, __LINE__, str, size, __VA_ARGS__)
extern void nh_snprintf(const char *func, int line, char *str, size_t size,
                        const char *fmt, ...) PRINTF_F(5, 6);
#define FITSint(x) FITSint_(x, __func__, __LINE__)
extern int FITSint_(long long, const char *, int);
#define FITSuint(x) FITSuint_(x, __func__, __LINE__)
extern unsigned FITSuint_(unsigned long long, const char *, int);
#ifdef ENHANCED_SYMBOLS
extern int unicodeval_to_utf8str(int, uint8 *, size_t);
#endif

/* ### insight.c ### */

extern int doattributes(void);
extern void enlightenment(int, int);
extern void youhiding(boolean, int);
extern char *trap_predicament(char *, int, boolean);
extern int doconduct(void);
extern void show_conduct(int);
extern void record_achievement(schar);
extern boolean remove_achievement(schar);
extern int count_achievements(void);
extern schar achieve_rank(int);
extern boolean sokoban_in_play(void);
extern int do_gamelog(void);
extern void show_gamelog(int);
extern int dovanquished(void);
extern int doborn(void);
extern void list_vanquished(char, boolean);
extern int num_genocides(void);
extern void list_genocided(char, boolean);
extern const char *align_str(aligntyp);
extern char *piousness(boolean, const char *);
extern void mstatusline(struct monst *);
extern void ustatusline(void);

/* ### invent.c ### */

extern void loot_classify(Loot *, struct obj *);
extern Loot *sortloot(struct obj **, unsigned, boolean,
                      boolean(*)(struct obj *));
extern void unsortloot(Loot **);
extern void assigninvlet(struct obj *);
extern struct obj *merge_choice(struct obj *, struct obj *);
extern int merged(struct obj **, struct obj **);
extern void addinv_core1(struct obj *);
extern void addinv_core2(struct obj *);
extern struct obj *addinv(struct obj *);
extern struct obj *addinv_before(struct obj *, struct obj *);
extern struct obj *hold_another_object(struct obj *, const char *,
                                       const char *, const char *);
extern void useupall(struct obj *);
extern void useup(struct obj *);
extern void consume_obj_charge(struct obj *, boolean);
extern void freeinv_core(struct obj *);
extern void freeinv(struct obj *);
extern void delallobj(coordxy, coordxy);
extern void delobj(struct obj *);
extern void delobj_core(struct obj *, boolean);
extern struct obj *sobj_at(int, coordxy, coordxy);
extern struct obj *nxtobj(struct obj *, int, boolean);
extern struct obj *carrying(int);
extern struct obj *u_carried_gloves(void);
extern struct obj *u_have_novel(void);
extern struct obj *o_on(unsigned int, struct obj *);
extern boolean obj_here(struct obj *, coordxy, coordxy);
extern boolean wearing_armor(void);
extern boolean is_worn(struct obj *);
extern struct obj *g_at(coordxy, coordxy);
extern boolean splittable(struct obj *);
extern int any_obj_ok(struct obj *);
extern struct obj *getobj(const char *, int(*)(struct obj *), unsigned int);
extern int ggetobj(const char *, int(*)(struct obj *), int, boolean,
                   unsigned *);
extern int askchain(struct obj **, const char *, int, int(*)(struct obj *),
                    int(*)(struct obj *), int, const char *);
extern void set_cknown_lknown(struct obj *);
extern void fully_identify_obj(struct obj *);
extern int identify(struct obj *);
extern int count_unidentified(struct obj *);
extern void identify_pack(int, boolean);
extern void learn_unseen_invent(void);
extern void update_inventory(void);
extern int doperminv(void);
extern void prinv(const char *, struct obj *, long);
extern char *xprname(struct obj *, const char *, char, boolean, long, long);
extern int ddoinv(void);
extern char display_inventory(const char *, boolean);
extern int display_binventory(coordxy, coordxy, boolean);
extern struct obj *display_cinventory(struct obj *);
extern struct obj *display_minventory(struct monst *, int, char *);
extern int dotypeinv(void);
extern const char *dfeature_at(coordxy, coordxy, char *);
extern int look_here(int, unsigned);
extern int dolook(void);
extern boolean will_feel_cockatrice(struct obj *, boolean);
extern void feel_cockatrice(struct obj *, boolean);
extern void stackobj(struct obj *);
extern boolean mergable(struct obj *, struct obj *);
extern int doprgold(void);
extern int doprwep(void);
extern int doprarm(void);
extern int doprring(void);
extern int dopramulet(void);
extern int doprtool(void);
extern int doprinuse(void);
extern void useupf(struct obj *, long);
extern char *let_to_name(char, boolean, boolean);
extern void free_invbuf(void);
extern void reassign(void);
extern boolean check_invent_gold(const char *);
extern int doorganize(void);
extern int adjust_split(void);
extern void free_pickinv_cache(void);
extern int count_unpaid(struct obj *);
extern int count_buc(struct obj *, int, boolean(*)(struct obj *));
extern void tally_BUCX(struct obj *, boolean, int *, int *, int *, int *,
                       int *, int *);
extern long count_contents(struct obj *, boolean, boolean, boolean, boolean);
extern void carry_obj_effects(struct obj *);
extern const char *currency(long);
extern void silly_thing(const char *, struct obj *);
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

extern void new_light_source(coordxy, coordxy, int, int, union any *);
extern void del_light_source(int, union any *);
extern void do_light_sources(seenV **);
extern void show_transient_light(struct obj *, coordxy, coordxy);
extern void transient_light_cleanup(void);
extern struct monst *find_mid(unsigned, unsigned);
extern void save_light_sources(NHFILE *, int);
extern void restore_light_sources(NHFILE *);
extern void light_stats(const char *, char *, long *, long *);
extern void relink_light_sources(boolean);
extern void light_sources_sanity_check(void);
extern void obj_move_light_source(struct obj *, struct obj *);
extern boolean any_light_source(void);
extern void snuff_light_source(coordxy, coordxy);
extern boolean obj_sheds_light(struct obj *);
extern boolean obj_is_burning(struct obj *);
extern void obj_split_light_source(struct obj *, struct obj *);
extern void obj_merge_light_sources(struct obj *, struct obj *);
extern void obj_adjust_light_radius(struct obj *, int);
extern int candle_light_range(struct obj *);
extern int arti_light_radius(struct obj *);
extern const char *arti_light_description(struct obj *);
extern int wiz_light_sources(void);

/* ### lock.c ### */

extern boolean picking_lock(coordxy *, coordxy *);
extern boolean picking_at(coordxy, coordxy);
extern void breakchestlock(struct obj *, boolean);
extern void reset_pick(void);
extern void maybe_reset_pick(struct obj *);
extern struct obj *autokey(boolean);
extern int pick_lock(struct obj *, coordxy, coordxy, struct obj *);
extern boolean u_have_forceable_weapon(void);
extern int doforce(void);
extern boolean boxlock(struct obj *, struct obj *);
extern boolean doorlock(struct obj *, coordxy, coordxy);
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

extern void dealloc_monst(struct monst *);
extern boolean is_home_elemental(struct permonst *);
extern struct monst *clone_mon(struct monst *, coordxy, coordxy);
extern int monhp_per_lvl(struct monst *);
extern void newmonhp(struct monst *, int);
extern struct mextra *newmextra(void);
extern void copy_mextra(struct monst *, struct monst *);
extern void dealloc_mextra(struct monst *);
extern struct monst *makemon(struct permonst *, coordxy, coordxy, mmflags_nht);
extern struct monst *unmakemon(struct monst *, mmflags_nht);
extern boolean create_critters(int, struct permonst *, boolean);
extern struct permonst *rndmonst(void);
extern struct permonst *mkclass(char, int);
extern struct permonst *mkclass_aligned(char, int, aligntyp);
extern int mkclass_poly(int);
extern int adj_lev(struct permonst *);
extern struct permonst *grow_up(struct monst *, struct monst *);
extern struct obj* mongets(struct monst *, int);
extern int golemhp(int);
extern boolean peace_minded(struct permonst *);
extern void set_malign(struct monst *);
extern void newmcorpsenm(struct monst *);
extern void freemcorpsenm(struct monst *);
extern void set_mimic_sym(struct monst *);
extern int mbirth_limit(int);
extern void mimic_hit_msg(struct monst *, short);
extern void mkmonmoney(struct monst *, long);
extern int bagotricks(struct obj *, boolean, int *);
extern boolean propagate(int, boolean, boolean);
extern boolean usmellmon(struct permonst *);

/* ### mcastu.c ### */

extern int castmu(struct monst *, struct attack *, boolean, boolean);
extern void touch_of_death(void);
extern int buzzmu(struct monst *, struct attack *);

/* ### mdlib.c ### */

extern void runtime_info_init(void);
extern const char *do_runtime_info(int *);
extern void release_runtime_info(void);
#ifdef ENHANCED_SYMBOLS
extern void dump_glyphids(void);
#endif

/* ### mhitm.c ### */

extern int fightm(struct monst *);
extern int mdisplacem(struct monst *, struct monst *, boolean);
extern int mattackm(struct monst *, struct monst *);
extern boolean engulf_target(struct monst *, struct monst *);
extern int mon_poly(struct monst *, struct monst *, int);
extern void paralyze_monst(struct monst *, int);
extern int sleep_monst(struct monst *, int, int);
extern void slept_monst(struct monst *);
extern void xdrainenergym(struct monst *, boolean);
extern long attk_protection(int);
extern void rustm(struct monst *, struct obj *);

/* ### mhitu.c ### */

extern void hitmsg(struct monst *, struct attack *);
extern const char *mswings_verb(struct obj *, boolean);
extern const char *mpoisons_subj(struct monst *, struct attack *);
extern void u_slow_down(void);
extern struct monst *cloneu(void);
extern void expels(struct monst *, struct permonst *, boolean);
extern struct attack *getmattk(struct monst *, struct monst *, int, int *,
                               struct attack *);
extern int mattacku(struct monst *);
boolean diseasemu(struct permonst *);
boolean u_slip_free(struct monst *, struct attack *);
extern int magic_negation(struct monst *);
extern boolean gulp_blnd_check(void);
extern int gazemu(struct monst *, struct attack *);
extern void mdamageu(struct monst *, int);
extern int could_seduce(struct monst *, struct monst *, struct attack *);
extern int doseduce(struct monst *);

/* ### minion.c ### */

extern void newemin(struct monst *);
extern void free_emin(struct monst *);
extern int monster_census(boolean);
extern int msummon(struct monst *);
extern void summon_minion(aligntyp, boolean);
extern int demon_talk(struct monst *);
extern long bribe(struct monst *);
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
                        boolean);
extern void free_luathemes(boolean);
extern void makecorridors(void);
extern void add_door(coordxy, coordxy, struct mkroom *);
extern void clear_level_structures(void);
extern void level_finalize_topology(void);
extern void mklev(void);
#ifdef SPECIALIZATION
extern void topologize(struct mkroom *, boolean));
#else
extern void topologize(struct mkroom *);
#endif
extern void place_branch(branch *, coordxy, coordxy);
extern boolean occupied(coordxy, coordxy);
extern int okdoor(coordxy, coordxy);
extern void dodoor(coordxy, coordxy, struct mkroom *);
extern void mktrap(int, int, struct mkroom *, coord *);
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
extern void makemaz(const char *);
extern void mazexy(coord *);
extern void get_level_extends(coordxy *, coordxy *, coordxy *, coordxy *);
extern void bound_digging(void);
extern void mkportal(coordxy, coordxy, coordxy, coordxy);
extern boolean bad_location(coordxy, coordxy, coordxy, coordxy, coordxy, coordxy);
extern void place_lregion(coordxy, coordxy, coordxy, coordxy, coordxy, coordxy, coordxy,
                          coordxy, coordxy, d_level *);
extern void fixup_special(void);
extern void fumaroles(void);
extern void movebubbles(void);
extern void water_friction(void);
extern void save_waterlevel(NHFILE *);
extern void restore_waterlevel(NHFILE *);

/* ### mkobj.c ### */

extern struct oextra *newoextra(void);
extern void copy_oextra(struct obj *, struct obj *);
extern void dealloc_oextra(struct obj *);
extern void newomonst(struct obj *);
extern void free_omonst(struct obj *);
extern void newomid(struct obj *);
extern void free_omid(struct obj *);
extern void newolong(struct obj *);
extern void free_olong(struct obj *);
extern void new_omailcmd(struct obj *, const char *);
extern void free_omailcmd(struct obj *);
extern struct obj *mkobj_at(char, coordxy, coordxy, boolean);
extern struct obj *mksobj_at(int, coordxy, coordxy, boolean, boolean);
extern struct obj *mksobj_migr_to_species(int, unsigned, boolean, boolean);
extern struct obj *mkobj(int, boolean);
extern int rndmonnum(void);
extern boolean bogon_is_pname(char);
extern struct obj *splitobj(struct obj *, long);
extern unsigned next_ident(void);
extern struct obj *unsplitobj(struct obj *);
extern void clear_splitobjs(void);
extern void replace_object(struct obj *, struct obj *);
extern struct obj *unknwn_contnr_contents(struct obj *);
extern void bill_dummy_object(struct obj *);
extern void costly_alteration(struct obj *, int);
extern void clear_dknown(struct obj *);
extern void unknow_object(struct obj *);
extern struct obj *mksobj(int, boolean, boolean);
extern int bcsign(struct obj *);
extern int weight(struct obj *);
extern struct obj *mkgold(long, coordxy, coordxy);
extern struct obj *mkcorpstat(int, struct monst *, struct permonst *,
                              coordxy, coordxy, unsigned);
extern int corpse_revive_type(struct obj *);
extern struct obj *obj_attach_mid(struct obj *, unsigned);
extern struct monst *get_mtraits(struct obj *, boolean);
extern struct obj *mk_tt_object(int, coordxy, coordxy);
extern struct obj *mk_named_object(int, struct permonst *,
                                   coordxy, coordxy,
                                   const char *);
extern struct obj *rnd_treefruit_at(coordxy, coordxy);
extern void set_corpsenm(struct obj *, int);
extern long rider_revival_time(struct obj *, boolean);
extern void start_corpse_timeout(struct obj *);
extern void start_glob_timeout(struct obj *, long);
extern void shrink_glob(anything *, long);
extern void maybe_adjust_light(struct obj *, int);
extern void bless(struct obj *);
extern void unbless(struct obj *);
extern void curse(struct obj *);
extern void uncurse(struct obj *);
extern void blessorcurse(struct obj *, int);
extern void set_bknown(struct obj *, unsigned);
extern boolean is_flammable(struct obj *);
extern boolean is_rottable(struct obj *);
extern void place_object(struct obj *, coordxy, coordxy);
extern void remove_object(struct obj *);
extern void discard_minvent(struct monst *, boolean);
extern void obj_extract_self(struct obj *);
extern void extract_nobj(struct obj *, struct obj **);
extern void extract_nexthere(struct obj *, struct obj **);
extern int add_to_minv(struct monst *, struct obj *);
extern struct obj *add_to_container(struct obj *, struct obj *);
extern void add_to_migration(struct obj *);
extern void add_to_buried(struct obj *);
extern void dealloc_obj(struct obj *);
extern void obj_ice_effects(coordxy, coordxy, boolean);
extern long peek_at_iced_corpse_age(struct obj *);
extern int hornoplenty(struct obj *, boolean);
extern void obj_sanity_check(void);
extern struct obj *obj_nexto(struct obj *);
extern struct obj *obj_nexto_xy(struct obj *, coordxy, coordxy, boolean);
extern struct obj *obj_absorb(struct obj **, struct obj **);
extern struct obj *obj_meld(struct obj **, struct obj **);
extern void pudding_merge_message(struct obj *, struct obj *);
extern struct obj *init_dummyobj(struct obj *, short, long);

/* ### mkroom.c ### */

extern void do_mkroom(int);
extern void fill_zoo(struct mkroom *);
extern struct permonst *antholemon(void);
extern boolean nexttodoor(int, int);
extern boolean has_dnstairs(struct mkroom *);
extern boolean has_upstairs(struct mkroom *);
extern int somex(struct mkroom *);
extern int somey(struct mkroom *);
extern boolean inside_room(struct mkroom *, coordxy, coordxy);
extern boolean somexy(struct mkroom *, coord *);
extern boolean somexyspace(struct mkroom *, coord *);
extern void mkundead(coord *, boolean, int);
extern struct permonst *courtmon(void);
extern void save_rooms(NHFILE *);
extern void rest_rooms(NHFILE *);
extern struct mkroom *search_special(schar);
extern int cmap_to_type(int);

/* ### mon.c ### */

extern void mon_sanity_check(void);
extern boolean zombie_maker(struct monst *);
extern int zombie_form(struct permonst *);
extern int m_poisongas_ok(struct monst *);
extern int undead_to_corpse(int);
extern int genus(int, int);
extern int pm_to_cham(int);
extern int minliquid(struct monst *);
extern int movemon(void);
extern int meatmetal(struct monst *);
extern int meatobj(struct monst *);
extern int meatcorpse(struct monst *);
extern void mon_givit(struct monst *, struct permonst *);
extern void mpickgold(struct monst *);
extern boolean mpickstuff(struct monst *, const char *);
extern int curr_mon_load(struct monst *);
extern int max_mon_load(struct monst *);
extern int can_carry(struct monst *, struct obj *);
extern long mon_allowflags(struct monst *);
extern int mfndpos(struct monst *, coord *, long *, long);
extern boolean monnear(struct monst *, coordxy, coordxy);
extern void dmonsfree(void);
extern void elemental_clog(struct monst *);
extern int mcalcmove(struct monst *, boolean);
extern void mcalcdistress(void);
extern void replmon(struct monst *, struct monst *);
extern void relmon(struct monst *, struct monst **);
extern struct obj *mlifesaver(struct monst *);
extern boolean corpse_chance(struct monst *, struct monst *, boolean);
extern void mondead(struct monst *);
extern void mondied(struct monst *);
extern void mongone(struct monst *);
extern void monstone(struct monst *);
extern void monkilled(struct monst *, const char *, int);
extern void set_ustuck(struct monst *);
extern void unstuck(struct monst *);
extern void killed(struct monst *);
extern void xkilled(struct monst *, int);
extern void mon_to_stone(struct monst *);
extern void m_into_limbo(struct monst *);
extern void migrate_mon(struct monst *, coordxy, coordxy);
extern void mnexto(struct monst *, unsigned);
extern void maybe_mnexto(struct monst *);
extern int mnearto(struct monst *, coordxy, coordxy, boolean, unsigned);
extern void m_respond(struct monst *);
extern void setmangry(struct monst *, boolean);
extern void wake_msg(struct monst *, boolean);
extern void wakeup(struct monst *, boolean);
extern void wake_nearby(void);
extern void wake_nearto(coordxy, coordxy, int);
extern void seemimic(struct monst *);
extern void normal_shape(struct monst *);
extern void iter_mons(void (*)(struct monst *));
extern struct monst *get_iter_mons(boolean (*)(struct monst *));
extern struct monst *get_iter_mons_xy(boolean (*)(struct monst *, coordxy, coordxy), coordxy, coordxy);
extern void rescham(void);
extern void restartcham(void);
extern void restore_cham(struct monst *);
extern void maybe_unhide_at(coordxy, coordxy);
extern boolean hideunder(struct monst *);
extern void hide_monst(struct monst *);
extern void mon_animal_list(boolean);
extern boolean valid_vampshiftform(int, int);
extern boolean validvamp(struct monst *, int *, int);
extern int select_newcham_form(struct monst *);
extern void mgender_from_permonst(struct monst *, struct permonst *);
extern int newcham(struct monst *, struct permonst *, unsigned);
extern int can_be_hatched(int);
extern int egg_type_from_parent(int, boolean);
extern boolean dead_species(int, boolean);
extern void kill_genocided_monsters(void);
extern void golemeffects(struct monst *, int, int);
extern boolean angry_guards(boolean);
extern void pacify_guards(void);
extern void decide_to_shapeshift(struct monst *, int);
extern boolean vamp_stone(struct monst *);

/* ### mondata.c ### */

extern void set_mon_data(struct monst *, struct permonst *);
extern struct attack *attacktype_fordmg(struct permonst *, int, int);
extern boolean attacktype(struct permonst *, int);
extern boolean noattacks(struct permonst *);
extern boolean poly_when_stoned(struct permonst *);
extern boolean defended(struct monst *, int);
extern boolean resists_drli(struct monst *);
extern boolean resists_magm(struct monst *);
extern boolean resists_blnd(struct monst *);
extern boolean can_blnd(struct monst *, struct monst *, uchar, struct obj *);
extern boolean ranged_attk(struct permonst *);
extern boolean mon_hates_silver(struct monst *);
extern boolean hates_silver(struct permonst *);
extern boolean mon_hates_blessings(struct monst *);
extern boolean hates_blessings(struct permonst *);
extern boolean mon_hates_light(struct monst *);
extern boolean passes_bars(struct permonst *);
extern boolean can_blow(struct monst *);
extern boolean can_chant(struct monst *);
extern boolean can_be_strangled(struct monst *);
extern boolean can_track(struct permonst *);
extern boolean breakarm(struct permonst *);
extern boolean sliparm(struct permonst *);
extern boolean sticks(struct permonst *);
extern boolean cantvomit(struct permonst *);
extern int num_horns(struct permonst *);
extern struct attack *dmgtype_fromattack(struct permonst *, int, int);
extern boolean dmgtype(struct permonst *, int);
extern int max_passive_dmg(struct monst *, struct monst *);
extern boolean same_race(struct permonst *, struct permonst *);
extern int monsndx(struct permonst *);
extern int name_to_mon(const char *, int *);
extern int name_to_monplus(const char *, const char **, int *);
extern int name_to_monclass(const char *, int *);
extern int gender(struct monst *);
extern int pronoun_gender(struct monst *, unsigned);
extern boolean levl_follower(struct monst *);
extern int little_to_big(int);
extern int big_to_little(int);
extern boolean big_little_match(int, int);
extern const char *locomotion(const struct permonst *, const char *);
extern const char *stagger(const struct permonst *, const char *);
extern const char *on_fire(struct permonst *, struct attack *);
extern const char *msummon_environ(struct permonst *, const char **);
extern const struct permonst *raceptr(struct monst *);
extern boolean olfaction(struct permonst *);
unsigned long cvt_adtyp_to_mseenres(uchar);
extern void monstseesu(unsigned long);
extern boolean resist_conflict(struct monst *);

/* ### monmove.c ### */

extern boolean itsstuck(struct monst *);
extern boolean mb_trapped(struct monst *, boolean);
extern void mon_track_add(struct monst *, coordxy, coordxy);
extern void mon_track_clear(struct monst *);
extern boolean monhaskey(struct monst *, boolean);
extern void mon_regen(struct monst *, boolean);
extern int dochugw(struct monst *, boolean);
extern boolean onscary(coordxy, coordxy, struct monst *);
extern struct monst *find_pmmonst(int);
extern int bee_eat_jelly(struct monst *, struct obj *);
extern void monflee(struct monst *, int, boolean, boolean);
extern void mon_yells(struct monst *, const char *);
extern int dochug(struct monst *);
extern boolean m_digweapon_check(struct monst *, coordxy, coordxy);
extern int m_move(struct monst *, int);
extern int m_move_aggress(struct monst *, coordxy, coordxy);
extern void dissolve_bars(coordxy, coordxy);
extern boolean closed_door(coordxy, coordxy);
extern boolean accessible(coordxy, coordxy);
extern void set_apparxy(struct monst *);
extern boolean can_ooze(struct monst *);
extern boolean can_fog(struct monst *);
extern boolean should_displace(struct monst *, coord *, long *, int, coordxy,
                               coordxy);
extern boolean undesirable_disp(struct monst *, coordxy, coordxy);

/* ### monst.c ### */

extern void monst_globals_init(void);

/* ### mplayer.c ### */

extern struct monst *mk_mplayer(struct permonst *, coordxy, coordxy, boolean);
extern void create_mplayers(int, boolean);
extern void mplayer_talk(struct monst *);

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
#endif /* WIN32 */

#endif /* MICRO || WIN32 */

/* ### mthrowu.c ### */

extern const char *rnd_hallublast(void);
extern boolean m_has_launcher_and_ammo(struct monst *);
extern int thitu(int, int, struct obj **, const char *);
extern int ohitmon(struct monst *, struct obj *, int, boolean);
extern void thrwmu(struct monst *);
extern int spitmu(struct monst *, struct attack *);
extern int breamu(struct monst *, struct attack *);
extern boolean linedup_callback(coordxy, coordxy, coordxy, coordxy,
                                boolean(*)(coordxy, coordxy));
extern boolean linedup(coordxy, coordxy, coordxy, coordxy, int);
extern boolean lined_up(struct monst *);
extern struct obj *m_carrying(struct monst *, int);
extern int thrwmm(struct monst *, struct monst *);
extern int spitmm(struct monst *, struct attack *, struct monst *);
extern int breamm(struct monst *, struct attack *, struct monst *);
extern void m_useupall(struct monst *, struct obj *);
extern void m_useup(struct monst *, struct obj *);
extern void m_throw(struct monst *, coordxy, coordxy, coordxy, coordxy, int, struct obj *);
extern void hit_bars(struct obj **, coordxy, coordxy, coordxy, coordxy, unsigned);
extern boolean hits_bars(struct obj **, coordxy, coordxy, coordxy, coordxy, int, int);

/* ### muse.c ### */

extern boolean find_defensive(struct monst *);
extern int use_defensive(struct monst *);
extern int rnd_defensive_item(struct monst *);
extern boolean find_offensive(struct monst *);
extern int use_offensive(struct monst *);
extern int rnd_offensive_item(struct monst *);
extern boolean find_misc(struct monst *);
extern int use_misc(struct monst *);
extern int rnd_misc_item(struct monst *);
extern boolean searches_for_item(struct monst *, struct obj *);
extern boolean mon_reflects(struct monst *, const char *);
extern boolean ureflects(const char *, const char *);
extern void mcureblindness(struct monst *, boolean);
extern boolean munstone(struct monst *, boolean);
extern boolean munslime(struct monst *, boolean);

/* ### music.c ### */

extern void awaken_soldiers(struct monst *);
extern int do_play_instrument(struct obj *);

/* ### nhlsel.c ### */

#if !defined(CROSSCOMPILE) || defined(CROSSCOMPILE_TARGET)
extern struct selectionvar *l_selection_check(lua_State *, int);
extern int l_selection_register(lua_State *);
extern void nhl_push_obj(lua_State *, struct obj *);
extern int nhl_obj_u_giveobj(lua_State *);
extern int l_obj_register(lua_State *);
#endif

/* ### nhlobj.c ### */

#if !defined(CROSSCOMPILE) || defined(CROSSCOMPILE_TARGET)
extern void nhl_push_obj(lua_State *, struct obj *);
extern int nhl_obj_u_giveobj(lua_State *);
extern int l_obj_register(lua_State *);
#endif

/* ### nhlua.c ### */

#if !defined(CROSSCOMPILE) || defined(CROSSCOMPILE_TARGET)
extern void l_nhcore_init(void);
extern void l_nhcore_done(void);
extern void l_nhcore_call(int);
extern lua_State * nhl_init(nhl_sandbox_info *);
extern void nhl_done(lua_State *);
extern boolean nhl_loadlua(lua_State *, const char *);
extern int nhl_pcall(lua_State *, int, int);
extern boolean load_lua(const char *, nhl_sandbox_info *);
extern void nhl_error(lua_State *, const char *) NORETURN;
extern void lcheck_param_table(lua_State *);
extern schar get_table_mapchr(lua_State *, const char *);
extern schar get_table_mapchr_opt(lua_State *, const char *, schar);
extern short nhl_get_timertype(lua_State *, int);
extern boolean nhl_get_xy_params(lua_State *, lua_Integer *, lua_Integer *);
extern void nhl_add_table_entry_int(lua_State *, const char *, lua_Integer);
extern void nhl_add_table_entry_char(lua_State *, const char *, char);
extern void nhl_add_table_entry_str(lua_State *, const char *, const char *);
extern void nhl_add_table_entry_bool(lua_State *, const char *, boolean);
extern void nhl_add_table_entry_region(lua_State *, const char *,
                                       coordxy, coordxy, coordxy, coordxy);
extern schar splev_chr2typ(char);
extern schar check_mapchr(const char *);
extern int get_table_int(lua_State *, const char *);
extern int get_table_int_opt(lua_State *, const char *, int);
extern char *get_table_str(lua_State *, const char *);
extern char *get_table_str_opt(lua_State *, const char *, char *);
extern int get_table_boolean(lua_State *, const char *);
extern int get_table_boolean_opt(lua_State *, const char *, int);
extern int get_table_option(lua_State *, const char *, const char *,
                            const char *const *);
extern int str_lines_max_width(const char *);
extern char *stripdigits(char *);
extern const char *get_lua_version(void);
extern void nhl_pushhooked_open_table(lua_State *L);
#endif /* !CROSSCOMPILE || CROSSCOMPILE_TARGET */

/* ### nhregex.c ### */

extern struct nhregex *regex_init(void);
extern boolean regex_compile(const char *, struct nhregex *);
extern char *regex_error_desc(struct nhregex *, char *);
extern boolean regex_match(const char *, struct nhregex *);
extern void regex_free(struct nhregex *);

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
extern void obj_shuffle_range(int, int *, int *);
extern int find_skates(void);
extern boolean objdescr_is(struct obj *, const char *);
extern void oinit(void);
extern void savenames(NHFILE *);
extern void restnames(NHFILE *);
extern void discover_object(int, boolean, boolean);
extern void undiscover_object(int);
extern int choose_disco_sort(int);
extern int dodiscovered(void);
extern int doclassdisco(void);
extern void rename_disco(void);

/* ### objects.c ### */

extern void objects_globals_init(void);

/* ### objnam.c ### */

extern void maybereleaseobuf(char *);
extern char *obj_typename(int);
extern char *simple_typename(int);
extern char *safe_typename(int);
extern boolean obj_is_pname(struct obj *);
extern char *distant_name(struct obj *, char *(*)(struct obj *));
extern char *fruitname(boolean);
extern struct fruit *fruit_from_indx(int);
extern struct fruit *fruit_from_name(const char *, boolean, int *);
extern void reorder_fruit(boolean);
extern char *xname(struct obj *);
extern char *mshot_xname(struct obj *);
extern boolean the_unique_obj(struct obj *);
extern boolean the_unique_pm(struct permonst *);
extern boolean erosion_matters(struct obj *);
extern char *doname(struct obj *);
extern char *doname_with_price(struct obj *);
extern char *doname_vague_quan(struct obj *);
extern boolean not_fully_identified(struct obj *);
extern char *corpse_xname(struct obj *, const char *, unsigned);
extern char *cxname(struct obj *);
extern char *cxname_singular(struct obj *);
extern char *killer_xname(struct obj *);
extern char *short_oname(struct obj *, char *(*)(struct obj *),
                         char *(*)(struct obj *), unsigned);
extern const char *singular(struct obj *, char *(*)(struct obj *));
extern char *just_an(char *, const char *);
extern char *an(const char *);
extern char *An(const char *);
extern char *The(const char *);
extern char *the(const char *);
extern char *aobjnam(struct obj *, const char *);
extern char *yobjnam(struct obj *, const char *);
extern char *Yobjnam2(struct obj *, const char *);
extern char *Tobjnam(struct obj *, const char *);
extern char *otense(struct obj *, const char *);
extern char *vtense(const char *, const char *);
extern char *Doname2(struct obj *);
extern char *yname(struct obj *);
extern char *Yname2(struct obj *);
extern char *ysimple_name(struct obj *);
extern char *Ysimple_name2(struct obj *);
extern char *simpleonames(struct obj *);
extern char *ansimpleoname(struct obj *);
extern char *thesimpleoname(struct obj *);
extern char *actualoname(struct obj *);
extern char *bare_artifactname(struct obj *);
extern char *makeplural(const char *);
extern char *makesingular(const char *);
extern struct obj *readobjnam(char *, struct obj *);
extern int rnd_class(int, int);
extern const char *suit_simple_name(struct obj *);
extern const char *cloak_simple_name(struct obj *);
extern const char *helm_simple_name(struct obj *);
extern const char *gloves_simple_name(struct obj *);
extern const char *boots_simple_name(struct obj *);
extern const char *shield_simple_name(struct obj *);
extern const char *shirt_simple_name(struct obj *);
extern const char *mimic_obj_name(struct monst *);
extern char *safe_qbuf(char *, const char *, const char *, struct obj *,
                       char *(*)(struct obj *), char *(*)(struct obj *),
                       const char *);
extern int shiny_obj(char);

/* ### options.c ### */

extern boolean match_optname(const char *, const char *, int, boolean);
extern uchar txt2key(char *);
extern void initoptions(void);
extern void initoptions_init(void);
extern void initoptions_finish(void);
extern boolean parseoptions(char *, boolean, boolean);
extern char *get_option_value(const char *, boolean);
extern int doset_simple(void);
extern int doset(void);
extern int dotogglepickup(void);
extern void option_help(void);
extern void all_options_strbuf(strbuf_t *);
extern void next_opt(winid, const char *);
extern int fruitadd(char *, struct fruit *);
extern int choose_classes_menu(const char *, int, boolean, char *, char *);
extern boolean parsebindings(char *);
extern void oc_to_str(char *, char *);
extern void add_menu_cmd_alias(char, char);
extern char get_menu_cmd_key(char);
extern char map_menu_cmd(char);
extern char *collect_menu_keys(char *, unsigned, boolean);
extern void show_menu_controls(winid, boolean);
extern void assign_warnings(uchar *);
extern char *nh_getenv(const char *);
extern void reset_duplicate_opt_detection(void);
extern void set_wc_option_mod_status(unsigned long, int);
extern void set_wc2_option_mod_status(unsigned long, int);
extern void set_option_mod_status(const char *, int);
extern int add_autopickup_exception(const char *);
extern void free_autopickup_exceptions(void);
extern void set_playmode(void);
extern int sym_val(const char *);
extern int query_color(const char *);
extern int query_attr(const char *);
extern const char *clr2colorname(int);
extern int match_str2clr(char *);
extern int match_str2attr(const char *, boolean);
extern boolean add_menu_coloring(char *);
extern boolean get_menu_coloring(const char *, int *, int *);
extern void free_menu_coloring(void);
extern boolean msgtype_parse_add(char *);
extern int msgtype_type(const char *, boolean);
extern void hide_unhide_msgtypes(boolean, int);
extern void msgtype_free(void);

/* ### pager.c ### */

extern char *self_lookat(char *);
extern char *monhealthdescr(struct monst *mon, boolean, char *);
extern void mhidden_description(struct monst *, boolean, char *);
extern boolean object_from_map(int, coordxy, coordxy, struct obj **);
extern const char *waterbody_name(coordxy, coordxy);
extern int do_screen_description(coord, boolean, int, char *, const char **,
                                 struct permonst **);
extern int do_look(int, coord *);
extern int dowhatis(void);
extern int doquickwhatis(void);
extern int doidtrap(void);
extern int dowhatdoes(void);
extern char *dowhatdoes_core(char, char *);
extern int dohelp(void);
extern int dohistory(void);

/* ### xxmain.c ### */

#if defined(MICRO) || defined(WIN32)
#ifdef CHDIR
extern void chdirx(char *, boolean);
#endif /* CHDIR */
extern boolean authorize_wizard_mode(void);
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
extern void error(const char *, ...) PRINTF_F(1, 2);
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
                               boolean(*)(struct obj *), int *);
extern boolean rider_corpse_revival(struct obj *, boolean);
extern void deferred_decor(boolean);
extern boolean menu_class_present(int);
extern void add_valid_menu_class(int);
extern boolean allow_all(struct obj *);
extern boolean allow_category(struct obj *);
extern boolean is_worn_by_type(struct obj *);
extern int ck_bag(struct obj *);
extern void removed_from_icebox(struct obj *);
extern void reset_justpicked(struct obj *);
extern int count_justpicked(struct obj *);
extern struct obj *find_justpicked(struct obj *);
extern int pickup(int);
extern int pickup_object(struct obj *, long, boolean);
extern int query_category(const char *, struct obj *, int, menu_item **, int);
extern int query_objlist(const char *, struct obj **, int, menu_item **, int,
                         boolean(*)(struct obj *));
extern struct obj *pick_obj(struct obj *);
extern int encumber_msg(void);
extern int container_at(coordxy, coordxy, boolean);
extern int doloot(void);
extern void observe_quantum_cat(struct obj *, boolean, boolean);
extern boolean container_gone(int(*)(struct obj *));
extern boolean u_handsy(void);
extern int use_container(struct obj **, int, boolean);
extern int loot_mon(struct monst *, int *, boolean *);
extern int dotip(void);
extern struct autopickup_exception *check_autopickup_exceptions(struct obj *);
extern boolean autopick_testobj(struct obj *, boolean);

/* ### pline.c ### */

#ifdef DUMPLOG
extern void dumplogmsg(const char *);
extern void dumplogfreemessages(void);
#endif
extern void pline(const char *, ...) PRINTF_F(1, 2);
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
extern void change_sex(void);
extern void livelog_newform(boolean, int, int);
extern void polyself(int);
extern int polymon(int);
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
extern const char *mbodypart(struct monst *, int);
extern const char *body_part(int);
extern int poly_gender(void);
extern void ugolemeffects(int, int);
extern boolean ugenocided(void);
extern const char *udeadinside(void);

/* ### potion.c ### */

extern void set_itimeout(long *, long);
extern void incr_itimeout(long *, int);
extern void make_confused(long, boolean);
extern void make_stunned(long, boolean);
extern void make_sick(long, const char *, boolean, int);
extern void make_slimed(long, const char *);
extern void make_stoned(long, const char *, int, const char *);
extern void make_vomiting(long, boolean);
extern void make_blinded(long, boolean);
extern void toggle_blindness(void);
extern boolean make_hallucinated(long, boolean, long);
extern void make_deaf(long, boolean);
extern void make_glib(int);
extern void self_invis_message(void);
extern int dodrink(void);
extern int dopotion(struct obj *);
extern int peffects(struct obj *);
extern void healup(int, int, boolean, boolean);
extern void strange_feeling(struct obj *, const char *);
extern void impact_arti_light(struct obj *, boolean, boolean);
extern void potionhit(struct monst *, struct obj *, int);
extern void potionbreathe(struct obj *);
extern int dodip(void);
extern int dip_into(void); /* altdip */
extern void mongrantswish(struct monst **);
extern void djinni_from_bottle(struct obj *);
extern struct monst *split_mon(struct monst *, struct monst *);
extern const char *bottlename(void);

/* ### pray.c ### */

extern boolean critically_low_hp(boolean);
extern boolean stuck_in_wall(void);
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
                        coordxy, coordxy, coordxy, coordxy);
extern char temple_occupied(char *);
extern boolean inhistemple(struct monst *);
extern int pri_move(struct monst *);
extern void priestini(d_level *, struct mkroom *, int, int, boolean);
extern aligntyp mon_aligntyp(struct monst *);
extern char *priestname(struct monst *, int, char *);
extern boolean p_coaligned(struct monst *);
extern struct monst *findpriest(char);
extern void intemple(int);
extern void forget_temple_entry(struct monst *);
extern void priest_talk(struct monst *);
extern struct monst *mk_roamer(struct permonst *, aligntyp, coordxy, coordxy,
                               boolean);
extern void reset_hostility(struct monst *);
extern boolean in_your_sanctuary(struct monst *, coordxy, coordxy);
extern void ghod_hitsu(struct monst *);
extern void angry_priest(void);
extern void clearpriests(void);
extern void restpriest(struct monst *, boolean);
extern void newepri(struct monst *);
extern void free_epri(struct monst *);

/* ### quest.c ### */

extern void onquest(void);
extern void nemdead(void);
extern void leaddead(void);
extern void artitouch(struct obj *);
extern boolean ok_to_quest(void);
extern void leader_speaks(struct monst *);
extern void nemesis_speaks(void);
extern void quest_chat(struct monst *);
extern void quest_talk(struct monst *);
extern void quest_stat_check(struct monst *);
extern void finish_quest(struct obj *);

/* ### questpgr.c ### */

extern void load_qtlist(void);
extern void unload_qtlist(void);
extern short quest_info(int);
extern const char *ldrname(void);
extern boolean is_quest_artifact(struct obj *);
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

extern void learnscroll(struct obj *);
extern char *tshirt_text(struct obj *, char *);
extern char *hawaiian_motif(struct obj *, char *);
extern char *apron_text(struct obj *, char *);
extern const char *candy_wrapper_text(struct obj *);
extern void assign_candy_wrapper(struct obj *);
extern int doread(void);
extern int charge_ok(struct obj *);
extern void recharge(struct obj *, int);
extern boolean valid_cloud_pos(coordxy, coordxy);
extern int seffects(struct obj *);
extern void drop_boulder_on_player(boolean, boolean, boolean, boolean);
extern boolean drop_boulder_on_monster(coordxy, coordxy, boolean, boolean);
extern void wand_explode(struct obj *, int);
extern void litroom(boolean, struct obj *);
extern void do_genocide(int);
extern void punish(struct obj *);
extern void unpunish(void);
extern boolean cant_revive(int *, boolean, struct obj *);
extern boolean create_particular(void);

/* ### rect.c ### */

extern void init_rect(void);
extern void free_rect(void);
extern NhRect *get_rect(NhRect *);
extern NhRect *rnd_rect(void);
extern void remove_rect(NhRect *);
extern void add_rect(NhRect *);
extern void split_rects(NhRect *, NhRect *);

/* ## region.c ### */

extern boolean inside_region(NhRegion *, int, int);
extern void clear_regions(void);
extern void run_regions(void);
extern boolean in_out_region(coordxy, coordxy);
extern boolean m_in_out_region(struct monst *, coordxy, coordxy);
extern void update_player_regions(void);
extern void update_monster_region(struct monst *);
extern NhRegion *visible_region_at(coordxy, coordxy);
extern void show_region(NhRegion *, coordxy, coordxy);
extern void save_regions(NHFILE *);
extern void rest_regions(NHFILE *);
extern void region_stats(const char *, char *, long *, long *);
extern NhRegion *create_gas_cloud(coordxy, coordxy, int, int);
extern boolean region_danger(void);
extern void region_safety(void);

/* ### restore.c ### */

extern void inven_inuse(boolean);
extern int dorecover(NHFILE *);
extern void restcemetery(NHFILE *, struct cemetery **);
extern void trickery(char *);
extern void getlev(NHFILE *, int, xint8);
extern void get_plname_from_file(NHFILE *, char *);
#ifdef SELECTSAVED
extern int restore_menu(winid);
#endif
extern void minit(void);
extern boolean lookup_id_mapping(unsigned, unsigned *);
extern int validate(NHFILE *, const char *);
extern void reset_restpref(void);
extern void set_restpref(const char *);
extern void set_savepref(const char *);

/* ### rip.c ### */

extern void genl_outrip(winid, int, time_t);

/* ### rnd.c ### */

#ifdef USE_ISAAC64
extern void init_isaac64(unsigned long, int(*fn)(int));
extern long nhrand(void);
#endif
extern int rn2(int);
extern int rn2_on_display_rng(int);
extern int rnl(int);
extern int rnd(int);
extern int d(int, int);
extern int rne(int);
extern int rnz(int);

/* ### role.c ### */

extern boolean validrole(int);
extern boolean validrace(int, int);
extern boolean validgend(int, int, int);
extern boolean validalign(int, int, int);
extern int randrole(boolean);
extern int randrace(int);
extern int randgend(int, int);
extern int randalign(int, int);
extern int str2role(const char *);
extern int str2race(const char *);
extern int str2gend(const char *);
extern int str2align(const char *);
extern boolean ok_role(int, int, int, int);
extern int pick_role(int, int, int, int);
extern boolean ok_race(int, int, int, int);
extern int pick_race(int, int, int, int);
extern boolean ok_gend(int, int, int, int);
extern int pick_gend(int, int, int, int);
extern boolean ok_align(int, int, int, int);
extern int pick_align(int, int, int, int);
extern void rigid_role_checks(void);
extern boolean setrolefilter(const char *);
extern boolean gotrolefilter(void);
extern void clearrolefilter(void);
extern char *build_plselection_prompt(char *, int, int, int, int, int);
extern char *root_plselection_prompt(char *, int, int, int, int, int);
extern void plnamesuffix(void);
extern void role_selection_prolog(int, winid);
extern void role_menu_extra(int, winid, boolean);
extern void role_init(void);
extern const char *Hello(struct monst *);
extern const char *Goodbye(void);

/* ### rumors.c ### */

extern char *getrumor(int, char *, boolean);
extern char *get_rnd_text(const char *, char *, int(*)(int), unsigned);
extern void outrumor(int, int);
extern void outoracle(boolean, boolean);
extern void save_oracles(NHFILE *);
extern void restore_oracles(NHFILE *);
extern int doconsult(struct monst *);
extern void rumor_check(void);
extern boolean CapitalMon(const char *);
extern void free_CapMons(void);

/* ### save.c ### */

extern int dosave(void);
extern int dosave0(void);
extern boolean tricked_fileremoved(NHFILE *, char *);
#ifdef INSURANCE
extern void savestateinlock(void);
#endif
extern void savelev(NHFILE *, xint8);
extern genericptr_t mon_to_buffer(struct monst *, int *);
extern boolean close_check(int);
extern void savecemetery(NHFILE *, struct cemetery **);
extern void savefruitchn(NHFILE *);
extern void store_plname_in_file(NHFILE *);
extern void free_dungeons(void);
extern void freedynamicdata(void);
extern void store_savefileinfo(NHFILE *);
extern void store_savefileinfo(NHFILE *);
extern int nhdatatypes_size(void);
extern void assignlog(char *, char*, int);
extern FILE *getlog(NHFILE *);
extern void closelog(NHFILE *);

/* ### sfstruct.c ### */

extern void newread(NHFILE *, int, int, genericptr_t, unsigned);
extern void bufon(int);
extern void bufoff(int);
extern void bflush(int);
extern void bwrite(int, const genericptr_t, unsigned);
extern void mread(int, genericptr_t, unsigned);
extern void minit(void);
extern void bclose(int);
#if defined(ZEROCOMP)
extern void zerocomp_bclose(int);
#endif

/* ### shk.c ### */

extern void setpaid(struct monst *);
extern long money2mon(struct monst *, long);
extern void money2u(struct monst *, long);
extern void shkgone(struct monst *);
extern void set_residency(struct monst *, boolean);
extern void replshk(struct monst *, struct monst *);
extern void restshk(struct monst *, boolean);
extern char inside_shop(coordxy, coordxy);
extern void u_left_shop(char *, boolean);
extern void remote_burglary(coordxy, coordxy);
extern void u_entered_shop(char *);
extern void pick_pick(struct obj *);
extern boolean same_price(struct obj *, struct obj *);
extern void shopper_financial_report(void);
extern int inhishop(struct monst *);
extern struct monst *shop_keeper(char);
extern boolean tended_shop(struct mkroom *);
extern boolean is_unpaid(struct obj *);
extern void delete_contents(struct obj *);
extern void obfree(struct obj *, struct obj *);
extern void make_happy_shk(struct monst *, boolean);
extern void make_happy_shoppers(boolean);
extern void hot_pursuit(struct monst *);
extern void make_angry_shk(struct monst *, coordxy, coordxy);
extern int dopay(void);
extern boolean paybill(int, boolean);
extern void finish_paybill(void);
extern struct obj *find_oid(unsigned);
extern long contained_cost(struct obj *, struct monst *, long, boolean,
                           boolean);
extern long contained_gold(struct obj *, boolean);
extern void picked_container(struct obj *);
extern void gem_learned(int);
extern void alter_cost(struct obj *, long);
extern long unpaid_cost(struct obj *, boolean);
extern boolean billable(struct monst **, struct obj *, char, boolean);
extern void addtobill(struct obj *, boolean, boolean, boolean);
extern void splitbill(struct obj *, struct obj *);
extern void subfrombill(struct obj *, struct monst *);
extern long stolen_value(struct obj *, coordxy, coordxy, boolean, boolean);
extern void donate_gold(long, struct monst *, boolean);
extern void sellobj_state(int);
extern void sellobj(struct obj *, coordxy, coordxy);
extern int doinvbill(int);
extern struct monst *shkcatch(struct obj *, coordxy, coordxy);
extern void add_damage(coordxy, coordxy, long);
extern void fix_shop_damage(void);
extern int shk_move(struct monst *);
extern void after_shk_move(struct monst *);
extern boolean is_fshk(struct monst *);
extern void shopdig(int);
extern void pay_for_damage(const char *, boolean);
extern boolean costly_spot(coordxy, coordxy);
extern struct obj *shop_object(coordxy, coordxy);
extern void price_quote(struct obj *);
extern void shk_chat(struct monst *);
extern void check_unpaid_usage(struct obj *, boolean);
extern void check_unpaid(struct obj *);
extern void costly_gold(coordxy, coordxy, long, boolean);
extern long get_cost_of_shop_item(struct obj *, int *);
extern int oid_price_adjustment(struct obj *, unsigned);
extern boolean block_door(coordxy, coordxy);
extern boolean block_entry(coordxy, coordxy);
extern char *shk_your(char *, struct obj *);
extern char *Shk_Your(char *, struct obj *);
extern void globby_bill_fixup(struct obj *, struct obj *);
extern void globby_donation(struct obj *, struct obj *);
extern void credit_report(struct monst *shkp, int idx, boolean silent);

/* ### shknam.c ### */

extern void neweshk(struct monst *);
extern void free_eshk(struct monst *);
extern void stock_room(int, struct mkroom *);
extern boolean saleable(struct monst *, struct obj *);
extern int get_shop_item(int);
extern char *Shknam(struct monst *);
extern char *shkname(struct monst *);
extern boolean shkname_is_pname(struct monst *);
extern boolean is_izchak(struct monst *, boolean);

/* ### sit.c ### */

extern void take_gold(void);
extern int dosit(void);
extern void rndcurse(void);
extern void attrcurse(void);

/* ### sounds.c ### */

extern void dosounds(void);
extern const char *growl_sound(struct monst *);
extern void growl(struct monst *);
extern void yelp(struct monst *);
extern void whimper(struct monst *);
extern void beg(struct monst *);
extern const char *maybe_gasp(struct monst *);
extern int dotalk(void);
extern int tiphat(void);
#ifdef USER_SOUNDS
extern int add_sound_mapping(const char *);
extern void play_sound_for_message(const char *);
extern void maybe_play_sound(const char *);
extern void release_sound_mappings(void);
#endif /* USER SOUNDS */

/* ### sp_lev.c ### */

#if !defined(CROSSCOMPILE) || defined(CROSSCOMPILE_TARGET)
extern void create_des_coder(void);
extern void reset_xystart_size(void);
extern struct mapfragment *mapfrag_fromstr(char *);
extern void mapfrag_free(struct mapfragment **);
extern schar mapfrag_get(struct mapfragment *, int, int);
extern boolean mapfrag_canmatch(struct mapfragment *);
extern const char * mapfrag_error(struct mapfragment *);
extern boolean mapfrag_match(struct mapfragment *, int, int);
extern void flip_level(int, boolean);
extern void flip_level_rnd(int, boolean);
extern boolean check_room(coordxy *, coordxy *, coordxy *, coordxy *, boolean);
extern boolean create_room(coordxy, coordxy, coordxy, coordxy, coordxy, coordxy, xint16,
                           xint16);
extern boolean dig_corridor(coord *, coord *, boolean, schar, schar);
extern void fill_special_room(struct mkroom *);
extern void wallify_map(coordxy, coordxy, coordxy, coordxy);
extern boolean load_special(const char *);
extern coordxy selection_getpoint(coordxy, coordxy, struct selectionvar *);
extern struct selectionvar *selection_new(void);
extern void selection_free(struct selectionvar *, boolean);
extern struct selectionvar *selection_clone(struct selectionvar *);
extern void set_selection_floodfillchk(int(*)(coordxy, coordxy));
extern void selection_floodfill(struct selectionvar *, coordxy, coordxy, boolean);
extern boolean pm_good_location(coordxy, coordxy, struct permonst *);
extern void get_location_coord(coordxy *, coordxy *, int, struct mkroom *, long);
extern void selection_setpoint(coordxy, coordxy, struct selectionvar *, int);
extern struct selectionvar * selection_not(struct selectionvar *);
extern void selection_filter_percent(struct selectionvar *, int);
extern int selection_rndcoord(struct selectionvar *, coordxy *, coordxy *,
                              boolean);
extern void selection_do_grow(struct selectionvar *, int);
extern void selection_do_line(coordxy, coordxy, coordxy, coordxy,
                              struct selectionvar *);
extern void selection_do_randline(coordxy, coordxy, coordxy, coordxy, schar, schar,
                                  struct selectionvar *);
extern struct selectionvar *selection_filter_mapchar(struct selectionvar *,
                                                     xint16, int);
extern void set_floodfillchk_match_under(coordxy);
extern void selection_do_ellipse(struct selectionvar *, int, int, int, int,
                                 int);
extern void selection_do_gradient(struct selectionvar *, long, long, long,
                                  long, long, long, long, long);
extern int lspo_reset_level(lua_State *);
extern int lspo_finalize_level(lua_State *);
extern boolean get_coord(lua_State *, int, lua_Integer *, lua_Integer *);
extern int nhl_abs_coord(lua_State *);
extern void update_croom(void);
extern const char *get_trapname_bytype(int);
extern void l_register_des(lua_State *);
#endif /* !CROSSCOMPILE || CROSSCOMPILE_TARGET */

/* ### spell.c ### */

extern void book_cursed(struct obj *);
extern int study_book(struct obj *);
extern void book_disappears(struct obj *);
extern void book_substitution(struct obj *, struct obj *);
extern void age_spells(void);
extern int docast(void);
extern int spell_skilltype(int);
extern int spelleffects(int, boolean);
extern int tport_spell(int);
extern void losespells(void);
extern int dovspell(void);
extern void initialspell(struct obj *);
extern int known_spell(short);
extern int spell_idx(short);
extern char force_learn_spell(short);
extern int num_spells(void);

/* ### steal.c ### */

extern long somegold(long);
extern void stealgold(struct monst *);
extern void thiefdead(void);
extern void remove_worn_item(struct obj *, boolean);
extern int steal(struct monst *, char *);
extern int mpickobj(struct monst *, struct obj *);
extern void stealamulet(struct monst *);
extern void maybe_absorb_item(struct monst *, struct obj *, int, int);
extern void mdrop_obj(struct monst *, struct obj *, boolean);
extern void mdrop_special_objs(struct monst *);
extern void relobj(struct monst *, int, boolean);
extern struct obj *findgold(struct obj *);

/* ### steed.c ### */

extern void rider_cant_reach(void);
extern boolean can_saddle(struct monst *);
extern int use_saddle(struct obj *);
extern void put_saddle_on_mon(struct obj *, struct monst *);
extern boolean can_ride(struct monst *);
extern int doride(void);
extern boolean mount_steed(struct monst *, boolean);
extern void exercise_steed(void);
extern void kick_steed(void);
extern void dismount_steed(int);
extern void place_monster(struct monst *, int, int);
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
extern void update_primary_symset(const struct symparse *, int);
extern void update_rogue_symset(const struct symparse *, int);
extern void update_ov_primary_symset(const struct symparse *, int);
extern void update_ov_rogue_symset(const struct symparse *, int);
extern nhsym get_othersym(int, int);
extern boolean symset_is_compatible(enum symset_handling_types, unsigned long);
extern void set_symhandling(char *handling, int which_set);
extern boolean proc_symset_line(char *);
extern int do_symset(boolean);
extern int load_symset(const char *, int);
extern void free_symsets(void);
extern const struct symparse *match_sym(char *);
extern void savedsym_free(void);
extern void savedsym_strbuf(strbuf_t *);
extern boolean parsesymbols(char *, int);
#ifdef ENHANCED_SYMBOLS
extern struct customization_detail *find_matching_symset_customization(
               const char *symset_name, int custtype,
               enum graphics_sets which_set);
extern void apply_customizations_to_symset(enum graphics_sets which_set);
#endif

/* ### sys.c ### */

extern void sys_early_init(void);
extern void sysopt_release(void);
extern void sysopt_seduce_set(int);

/* ### teleport.c ### */

extern boolean noteleport_level(struct monst *);
extern boolean goodpos(coordxy, coordxy, struct monst *, mmflags_nht);
extern boolean enexto(coord *, coordxy, coordxy, struct permonst *);
extern boolean enexto_core(coord *, coordxy, coordxy, struct permonst *,
                           mmflags_nht);
extern void teleds(coordxy, coordxy, int);
extern boolean safe_teleds(int);
extern boolean teleport_pet(struct monst *, boolean);
extern void tele(void);
extern void scrolltele(struct obj *);
extern int dotelecmd(void);
extern int dotele(boolean);
extern void level_tele(void);
extern void domagicportal(struct trap *);
extern void tele_trap(struct trap *);
extern void level_tele_trap(struct trap *, unsigned);
extern void rloc_to(struct monst *, coordxy, coordxy);
extern void rloc_to_flag(struct monst *, coordxy, coordxy, unsigned);
extern boolean rloc(struct monst *, unsigned);
extern boolean tele_restrict(struct monst *);
extern void mtele_trap(struct monst *, struct trap *, int);
extern int mlevel_tele_trap(struct monst *, struct trap *, boolean, int);
extern boolean rloco(struct obj *);
extern int random_teleport_level(void);
extern boolean u_teleport_mon(struct monst *, boolean);

/* ### tile.c ### */

#ifdef USE_TILES
extern void substitute_tiles(d_level *);
#endif

/* ### timeout.c ### */

extern void burn_away_slime(void);
extern void nh_timeout(void);
extern void fall_asleep(int, boolean);
extern void attach_egg_hatch_timeout(struct obj *, long);
extern void attach_fig_transform_timeout(struct obj *);
extern void kill_egg(struct obj *);
extern void hatch_egg(union any *, long);
extern void learn_egg_type(int);
extern void burn_object(union any *, long);
extern void begin_burn(struct obj *, boolean);
extern void end_burn(struct obj *, boolean);
extern void do_storms(void);
extern boolean start_timer(long, short, short, union any *);
extern long stop_timer(short, union any *);
extern long peek_timer(short, union any *);
extern void run_timers(void);
extern void obj_move_timers(struct obj *, struct obj *);
extern void obj_split_timers(struct obj *, struct obj *);
extern void obj_stop_timers(struct obj *);
extern boolean obj_has_timer(struct obj *, short);
extern void spot_stop_timers(coordxy, coordxy, short);
extern long spot_time_expires(coordxy, coordxy, short);
extern long spot_time_left(coordxy, coordxy, short);
extern boolean obj_is_local(struct obj *);
extern void save_timers(NHFILE *, int);
extern void restore_timers(NHFILE *, int, long);
extern void timer_stats(const char *, char *, long *, long *);
extern void relink_timers(boolean);
extern int wiz_timeout_queue(void);
extern void timer_sanity_check(void);

/* ### topten.c ### */

extern void formatkiller(char *, unsigned, int, boolean);
extern int observable_depth(d_level *);
extern void topten(int, time_t);
extern void prscore(int, char **);
extern struct toptenentry *get_rnd_toptenentry(void);
extern struct obj *tt_oname(struct obj *);

/* ### track.c ### */

extern void initrack(void);
extern void settrack(void);
extern coord *gettrack(int, int);

/* ### trap.c ### */

extern boolean burnarmor(struct monst *);
extern int erode_obj(struct obj *, const char *, int, int);
extern boolean grease_protect(struct obj *, const char *, struct monst *);
extern struct trap *maketrap(coordxy, coordxy, int);
extern void fall_through(boolean, unsigned);
extern struct monst *animate_statue(struct obj *, coordxy, coordxy, int, int *);
extern struct monst *activate_statue_trap(struct trap *, coordxy, coordxy,
                                          boolean);
extern void set_utrap(unsigned, unsigned);
extern void reset_utrap(boolean);
extern void dotrap(struct trap *, unsigned);
extern void seetrap(struct trap *);
extern void feeltrap(struct trap *);
extern int mintrap(struct monst *, unsigned);
extern void instapetrify(const char *);
extern void minstapetrify(struct monst *, boolean);
extern void selftouch(const char *);
extern void mselftouch(struct monst *, const char *, boolean);
extern void float_up(void);
extern void fill_pit(coordxy, coordxy);
extern int float_down(long, long);
extern void climb_pit(void);
extern boolean fire_damage(struct obj *, boolean, coordxy, coordxy);
extern int fire_damage_chain(struct obj *, boolean, boolean, coordxy, coordxy);
extern boolean lava_damage(struct obj *, coordxy, coordxy);
extern void acid_damage(struct obj *);
extern int water_damage(struct obj *, const char *, boolean);
extern void water_damage_chain(struct obj *, boolean);
extern boolean rnd_nextto_goodpos(coordxy *, coordxy *, struct monst *);
extern boolean drown(void);
extern void drain_en(int);
extern int dountrap(void);
extern int could_untrap(boolean, boolean);
extern void cnv_trap_obj(int, int, struct trap *, boolean);
extern int untrap(boolean, coordxy, coordxy, struct obj *);
extern boolean openholdingtrap(struct monst *, boolean *);
extern boolean closeholdingtrap(struct monst *, boolean *);
extern boolean openfallingtrap(struct monst *, boolean, boolean *);
extern boolean chest_trap(struct obj *, int, boolean);
extern void deltrap(struct trap *);
extern boolean delfloortrap(struct trap *);
extern struct trap *t_at(coordxy, coordxy);
extern int count_traps(int);
extern void b_trapped(const char *, int);
extern boolean unconscious(void);
extern void blow_up_landmine(struct trap *);
extern int launch_obj(short, coordxy, coordxy, coordxy, coordxy, int);
extern boolean launch_in_progress(void);
extern void force_launch_placement(void);
extern boolean uteetering_at_seen_pit(struct trap *);
extern boolean uescaped_shaft(struct trap *);
extern boolean lava_effects(void);
extern void sink_into_lava(void);
extern void sokoban_guilt(void);
extern const char * trapname(int, boolean);
extern void ignite_items(struct obj *);
extern void trap_ice_effects(coordxy x, coordxy y, boolean ice_is_melting);
extern void trap_sanity_check(void);

/* ### u_init.c ### */

extern void u_init(void);

/* ### uhitm.c ### */

extern void dynamic_multi_reason(struct monst *, const char *, boolean);
extern void erode_armor(struct monst *, int);
extern boolean attack_checks(struct monst *, struct obj *);
extern void check_caitiff(struct monst *);
extern void mon_maybe_wakeup_on_hit(struct monst *);
extern int find_roll_to_hit(struct monst *, uchar, struct obj *, int *, int *);
extern boolean force_attack(struct monst *, boolean);
extern boolean do_attack(struct monst *);
extern boolean hmon(struct monst *, struct obj *, int, int);
extern boolean shade_miss(struct monst *, struct monst *, struct obj *,
                          boolean, boolean);
extern void mhitm_ad_rust(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_corr(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_dcay(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_dren(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_drli(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_fire(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_cold(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_elec(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_acid(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_sgld(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_tlpt(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_blnd(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_curs(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_drst(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_drin(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_stck(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_wrap(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_plys(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_slee(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_slim(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_ench(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_slow(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_conf(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_poly(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_famn(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_pest(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_deth(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_halu(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_phys(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_ston(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_were(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_heal(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_stun(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_legs(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_dgst(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_samu(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_dise(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_sedu(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_ad_ssex(struct monst *, struct attack *, struct monst *,
                          struct mhitm_data *);
extern void mhitm_adtyping(struct monst *, struct attack *, struct monst *,
                           struct mhitm_data *);
extern boolean do_stone_u(struct monst *);
extern void do_stone_mon(struct monst *, struct attack *, struct monst *,
                         struct mhitm_data *);
extern int damageum(struct monst *, struct attack *, int);
extern int explum(struct monst *, struct attack *);
extern void missum(struct monst *, struct attack *, boolean);
extern boolean mhitm_knockback(struct monst *, struct monst *,struct attack *,
                               int *, boolean);
extern int passive(struct monst *, struct obj *, boolean, boolean, uchar,
                   boolean);
extern void passive_obj(struct monst *, struct obj *, struct attack *);
extern void stumble_onto_mimic(struct monst *);
extern int flash_hits_mon(struct monst *, struct obj *);
extern void light_hits_gremlin(struct monst *, int);


/* ### unixmain.c ### */
#ifdef UNIX
#ifdef PORT_HELP
extern void port_help(void);
#endif
extern void sethanguphandler(void(*)(int));
extern boolean authorize_wizard_mode(void);
extern void append_slash(char *);
extern boolean check_user_string(const char *);
extern char *get_login_name(void);
extern unsigned long sys_random_seed(void);
#endif /* UNIX */

/* ### unixtty.c ### */

#if defined(UNIX) || defined(__BEOS__)
extern void gettty(void);
extern void settty(const char *);
extern void setftty(void);
extern void intron(void);
extern void introff(void);
extern void error (const char *, ...) PRINTF_F(1, 2);
#ifdef ENHANCED_SYMBOLS
extern void tty_utf8graphics_fixup(void);
#endif
#endif /* UNIX || __BEOS__ */

/* ### unixunix.c ### */

#ifdef UNIX
extern void getlock(void);
extern void regularize(char *);
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
extern boolean file_exists(const char *);
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
extern int glyphrep(const char *);
extern char *mixed_to_utf8(char *buf, size_t bufsz, const char *str, int *);
extern int match_glyph(char *);
extern void dump_all_glyphids(FILE *fp);
extern void fill_glyphid_cache(void);
extern void free_glyphid_cache(void);
extern boolean glyphid_cache_status(void);
extern int glyphrep_to_custom_map_entries(const char *op, int *glyph);
void free_all_glyphmap_u(void);
int add_custom_urep_entry(const char *symset_name, int glyphidx,
                          const uint8 *utf8str, long ucolor,
                          enum graphics_sets which_set);
int set_map_u(glyph_map *gm, const uint8 *utf8str, long ucolor);
#endif /* ENHANCED_SYMBOLS */

/* ### vault.c ### */

extern void newegd(struct monst *);
extern void free_egd(struct monst *);
extern boolean grddead(struct monst *);
extern struct monst *findgd(void);
extern void vault_summon_gd(void);
extern char vault_occupied(char *);
extern void uleftvault(struct monst *);
extern void invault(void);
extern int gd_move(struct monst *);
extern void paygd(boolean);
extern long hidden_gold(boolean);
extern boolean gd_sound(void);
extern void vault_gd_watching(unsigned int);

/* ### version.c ### */

extern char *version_string(char *, size_t bufsz);
extern char *getversionstring(char *, size_t bufsz);
extern int doversion(void);
extern int doextversion(void);
#ifdef MICRO
extern boolean comp_times(long);
#endif
extern boolean check_version(struct version_info *, const char *, boolean,
                             unsigned long);
extern boolean uptodate(NHFILE *, const char *, unsigned long);
extern void store_formatindicator(NHFILE *);
extern void store_version(NHFILE *);
extern unsigned long get_feature_notice_ver(char *);
extern unsigned long get_current_feature_ver(void);
extern const char *copyright_banner_line(int);
extern void early_version_info(boolean);
#ifdef RUNTIME_PORT_ID
extern char *get_port_id(char *);
#endif
#ifdef RUNTIME_PASTEBUF_SUPPORT
extern void port_insert_pastebuf(char *);
#endif

/* ### video.c ### */

#ifdef MSDOS
extern int assign_video(char *);
#ifdef NO_TERMS
extern void gr_init(void);
extern void gr_finish(void);
#endif
extern void tileview(boolean);
#endif
#ifdef VIDEOSHADES
extern int assign_videoshades(char *);
extern int assign_videocolors(char *);
#endif

/* ### vision.c ### */

extern void vision_init(void);
extern int does_block(int, int, struct rm *);
extern void vision_reset(void);
extern void vision_recalc(int);
extern void block_point(int, int);
extern void unblock_point(int, int);
extern boolean clear_path(int, int, int, int);
extern void do_clear_area(coordxy, coordxy, int,
                          void(*)(coordxy, coordxy, void *), genericptr_t);
extern unsigned howmonseen(struct monst *);

#ifdef VMS

/* ### vmsfiles.c ### */

extern int vms_link(const char *, const char *);
extern int vms_unlink(const char *);
extern int vms_creat(const char *, unsigned int);
extern int vms_open(const char *, int, unsigned int);
extern boolean same_dir(const char *, const char *);
extern int c__translate(int);
extern char *vms_basename(const char *);

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

/* ### vmsmisc.c ### */

extern void vms_abort(void) NORETURN;
extern void vms_exit(int) NORETURN;
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
extern void error (const char *, ...) PRINTF_F(1, 2);
#ifdef TIMED_DELAY
extern void msleep(unsigned);
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

extern const char *weapon_descr(struct obj *);
extern int hitval(struct obj *, struct monst *);
extern int dmgval(struct obj *, struct monst *);
extern int special_dmgval(struct monst *, struct monst *, long, long *);
extern void silver_sears(struct monst *, struct monst *, long);
extern struct obj *select_rwep(struct monst *);
extern boolean monmightthrowwep(struct obj *);
extern struct obj *select_hwep(struct monst *);
extern void possibly_unwield(struct monst *, boolean);
extern void mwepgone(struct monst *);
extern int mon_wield_item(struct monst *);
extern int abon(void);
extern int dbon(void);
extern void wet_a_towel(struct obj *, int, boolean);
extern void dry_a_towel(struct obj *, int, boolean);
extern char *skill_level_name(int, char *);
extern const char *skill_name(int);
extern boolean can_advance(int, boolean);
extern int enhance_weapon_skill(void);
extern void unrestrict_weapon_skill(int);
extern void use_skill(int, int);
extern void add_weapon_skill(int);
extern void lose_weapon_skill(int);
extern void drain_weapon_skill(int);
extern int weapon_type(struct obj *);
extern int uwep_skill_type(void);
extern int weapon_hit_bonus(struct obj *);
extern int weapon_dam_bonus(struct obj *);
extern void skill_init(const struct def_skill *);

/* ### were.c ### */

extern void were_change(struct monst *);
extern int counter_were(int);
extern int were_beastie(int);
extern void new_were(struct monst *);
extern int were_summon(struct permonst *, boolean, int *, char *);
extern void you_were(void);
extern void you_unwere(boolean);
extern void set_ulycn(int);

/* ### wield.c ### */

extern void setuwep(struct obj *);
extern const char *empty_handed(void);
extern void setuqwep(struct obj *);
extern void setuswapwep(struct obj *);
extern int dowield(void);
extern int doswapweapon(void);
extern int dowieldquiver(void);
extern int doquiver_core(const char *);
extern boolean wield_tool(struct obj *, const char *);
extern int can_twoweapon(void);
extern void drop_uswapwep(void);
extern int dotwoweapon(void);
extern void uwepgone(void);
extern void uswapwepgone(void);
extern void uqwepgone(void);
extern void set_twoweap(boolean);
extern void untwoweapon(void);
extern int chwepon(struct obj *, int);
extern int welded(struct obj *);
extern void weldmsg(struct obj *);
extern void setmnotwielded(struct monst *, struct obj *);
extern boolean mwelded(struct obj *);

/* ### windows.c ### */

extern void choose_windows(const char *);
#ifdef WINCHAIN
void addto_windowchain(const char *s);
void commit_windowchain(void);
#endif
extern boolean genl_can_suspend_no(void);
extern boolean genl_can_suspend_yes(void);
extern char genl_message_menu(char, int, const char *);
extern void genl_preference_update(const char *);
extern char *genl_getmsghistory(boolean);
extern void genl_putmsghistory(const char *, boolean);
#ifdef HANGUPHANDLING
extern void nhwindows_hangup(void);
#endif
extern void genl_status_init(void);
extern void genl_status_finish(void);
extern void genl_status_enablefield(int, const char *, const char *, boolean);
extern void genl_status_update(int, genericptr_t, int, int, int,
                               unsigned long *);
#ifdef DUMPLOG
extern char *dump_fmtstr(const char *, char *, boolean);
#endif
extern void dump_open_log(time_t);
extern void dump_close_log(void);
extern void dump_redirect(boolean);
extern void dump_forward_putstr(winid, int, const char*, int);
extern int has_color(int);
extern int glyph2ttychar(int);
extern int glyph2symidx(int);
extern char *encglyph(int);
extern int decode_glyph(const char *str, int *glyph_ptr);
extern char *decode_mixed(char *, const char *);
extern void genl_putmixed(winid, int, const char *);
extern boolean menuitem_invert_test(int, unsigned, boolean);

/* ### windows.c ### */

#ifdef WIN32
extern void nethack_enter_windows(void);
#endif

/* ### wizard.c ### */

extern void amulet(void);
extern int mon_has_amulet(struct monst *);
extern int mon_has_special(struct monst *);
extern void choose_stairs(coordxy *, coordxy *, boolean);
extern int tactics(struct monst *);
extern boolean has_aggravatables(struct monst *);
extern void aggravate(void);
extern void clonewiz(void);
extern int pick_nasty(int);
extern int nasty(struct monst *);
extern void resurrect(void);
extern void intervene(void);
extern void wizdead(void);
extern void cuss(struct monst *);

/* ### worm.c ### */

extern int get_wormno(void);
extern void initworm(struct monst *, int);
extern void worm_move(struct monst *);
extern void worm_nomove(struct monst *);
extern void wormgone(struct monst *);
extern int wormhitu(struct monst *);
extern void cutworm(struct monst *, coordxy, coordxy, boolean);
extern void see_wsegs(struct monst *);
extern void detect_wsegs(struct monst *, boolean);
extern void save_worm(NHFILE *);
extern void rest_worm(NHFILE *);
extern void place_wsegs(struct monst *, struct monst *);
extern void sanity_check_worm(struct monst *);
extern void wormno_sanity_check(void);
extern void remove_worm(struct monst *);
extern void place_worm_tail_randomly(struct monst *, coordxy, coordxy);
extern int size_wseg(struct monst *);
extern int count_wsegs(struct monst *);
extern boolean worm_known(struct monst *);
extern boolean worm_cross(int, int, int, int);
extern int wseg_at(struct monst *, int, int);
extern void flip_worm_segs_vertical(struct monst *, int, int);
extern void flip_worm_segs_horizontal(struct monst *, int, int);

/* ### worn.c ### */

extern void setworn(struct obj *, long);
extern void setnotworn(struct obj *);
extern void allunworn(void);
extern struct obj *wearmask_to_obj(long);
extern long wearslot(struct obj *);
extern void mon_set_minvis(struct monst *);
extern void mon_adjust_speed(struct monst *, int, struct obj *);
extern void update_mon_intrinsics(struct monst *, struct obj *, boolean,
                                  boolean);
extern int find_mac(struct monst *);
extern void m_dowear(struct monst *, boolean);
extern struct obj *which_armor(struct monst *, long);
extern void mon_break_armor(struct monst *, boolean);
extern void bypass_obj(struct obj *);
extern void clear_bypasses(void);
extern void bypass_objlist(struct obj *, boolean);
extern struct obj *nxt_unbypassed_obj(struct obj *);
extern struct obj *nxt_unbypassed_loot(Loot *, struct obj *);
extern int racial_exception(struct monst *, struct obj *);
extern void extract_from_minvent(struct monst *, struct obj *, boolean,
                                 boolean);

/* ### write.c ### */

extern int dowrite(struct obj *);

/* ### zap.c ### */

extern void learnwand(struct obj *);
extern int bhitm(struct monst *, struct obj *);
extern void release_hold(void);
extern void probe_monster(struct monst *);
extern boolean get_obj_location(struct obj *, coordxy *, coordxy *, int);
extern boolean get_mon_location(struct monst *, coordxy *, coordxy *, int);
extern struct monst *get_container_location(struct obj * obj, int *, int *);
extern struct monst *montraits(struct obj *, coord *, boolean);
extern struct monst *revive(struct obj *, boolean);
extern int unturn_dead(struct monst *);
extern void unturn_you(void);
extern void cancel_item(struct obj *);
extern boolean drain_item(struct obj *, boolean);
extern boolean obj_unpolyable(struct obj *);
extern struct obj *poly_obj(struct obj *, int);
extern boolean obj_resists(struct obj *, int, int);
extern boolean obj_shudders(struct obj *);
extern void do_osshock(struct obj *);
extern int bhito(struct obj *, struct obj *);
extern int bhitpile(struct obj *, int(*)(struct obj *, struct obj *),
                    coordxy, coordxy, schar);
extern int zappable(struct obj *);
extern void do_enlightenment_effect(void);
extern void zapnodir(struct obj *);
extern int dozap(void);
extern int zapyourself(struct obj *, boolean);
extern void ubreatheu(struct attack *);
extern int lightdamage(struct obj *, boolean, int);
extern boolean flashburn(long);
extern boolean cancel_monst(struct monst *, struct obj *, boolean, boolean,
                            boolean);
extern void zapsetup(void);
extern void zapwrapup(void);
extern void weffects(struct obj *);
extern int spell_damage_bonus(int);
extern const char *exclam(int force);
extern void hit(const char *, struct monst *, const char *);
extern void miss(const char *, struct monst *);
extern struct monst *bhit(coordxy, coordxy, int, enum bhit_call_types,
                          int(*)(struct monst *, struct obj *),
                          int(*)(struct obj *, struct obj *), struct obj **);
extern struct monst *boomhit(struct obj *, coordxy, coordxy);
extern int zhitm(struct monst *, int, int, struct obj **);
extern int burn_floor_objects(coordxy, coordxy, boolean, boolean);
extern void ubuzz(int, int);
extern void buzz(int, int, coordxy, coordxy, int, int);
extern void dobuzz(int, int, coordxy, coordxy, int, int, boolean);
extern void melt_ice(coordxy, coordxy, const char *);
extern void start_melt_ice_timeout(coordxy, coordxy, long);
extern void melt_ice_away(union any *, long);
extern int zap_over_floor(coordxy, coordxy, int, boolean *, short);
extern void fracture_rock(struct obj *);
extern boolean break_statue(struct obj *);
extern boolean u_adtyp_resistance_obj(int);
extern char *item_what(int);
extern void destroy_item(int, int);
extern int destroy_mitem(struct monst *, int, int);
extern int resist(struct monst *, char, int, int);
extern void makewish(void);
extern const char *flash_str(int, boolean);

#endif /* !MAKEDEFS_C && !MDLIB_C */

#endif /* EXTERN_H */
