/* NetHack 3.6	extern.h	$NHDT-Date: 1502753404 2017/08/14 23:30:04 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.600 $ */
/* Copyright (c) Steve Creps, 1988.				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef EXTERN_H
#define EXTERN_H

#define E extern

/* ### alloc.c ### */

#if 0
E long *alloc(unsigned int);
#endif
E char *fmt_ptr(const genericptr);

/* This next pre-processor directive covers almost the entire file,
 * interrupted only occasionally to pick up specific functions as needed. */
#if !defined(MAKEDEFS_C) && !defined(LEV_LEX_C)

/* ### allmain.c ### */

E void moveloop(boolean);
E void stop_occupation(void);
E void display_gamewindows(void);
E void newgame(void);
E void welcome(boolean);
E time_t get_realtime(void);

/* ### apply.c ### */

E int doapply(void);
E int dorub(void);
E int dojump(void);
E int jump(int);
E int number_leashed(void);
E void o_unleash(struct obj *);
E void m_unleash(struct monst *, boolean);
E void unleash_all(void);
E boolean next_to_u(void);
E struct obj *get_mleash(struct monst *);
E const char *beautiful(void);
E void check_leash(xchar, xchar);
E boolean um_dist(xchar, xchar, xchar);
E boolean snuff_candle(struct obj *);
E boolean snuff_lit(struct obj *);
E boolean catch_lit(struct obj *);
E void use_unicorn_horn(struct obj *);
E boolean tinnable(struct obj *);
E void reset_trapset(void);
E void fig_transform(ANY_P *, long);
E int unfixable_trouble_count(boolean);

/* ### artifact.c ### */

E void init_artifacts(void);
E void save_artifacts(int);
E void restore_artifacts(int);
E const char *artiname(int);
E struct obj *mk_artifact(struct obj *, aligntyp);
E const char *artifact_name(const char *, short *);
E boolean exist_artifact(int, const char *);
E void artifact_exists(struct obj *, const char *, boolean);
E int nartifact_exist(void);
E boolean arti_immune(struct obj *, int);
E boolean spec_ability(struct obj *, unsigned long);
E boolean confers_luck(struct obj *);
E boolean arti_reflects(struct obj *);
E boolean shade_glare(struct obj *);
E boolean restrict_name(struct obj *, const char *);
E boolean defends(int, struct obj *);
E boolean defends_when_carried(int, struct obj *);
E boolean protects(struct obj *, boolean);
E void set_artifact_intrinsic(struct obj *, boolean, long);
E int touch_artifact(struct obj *, struct monst *);
E int spec_abon(struct obj *, struct monst *);
E int spec_dbon(struct obj *, struct monst *, int);
E void discover_artifact(xchar);
E boolean undiscovered_artifact(xchar);
E int disp_artifact_discoveries(winid);
E boolean artifact_hit(struct monst *, struct monst *, struct obj *, int *, int);
E int doinvoke(void);
E boolean finesse_ahriman(struct obj *);
E void arti_speak(struct obj *);
E boolean artifact_light(struct obj *);
E long spec_m2(struct obj *);
E boolean artifact_has_invprop(struct obj *, uchar);
E long arti_cost(struct obj *);
E struct obj *what_gives(long *);
E const char *glow_color(int);
E void Sting_effects(int);
E int retouch_object(struct obj **, boolean);
E void retouch_equipment(int);

/* ### attrib.c ### */

E boolean adjattrib(int, int, int);
E void gainstr(struct obj *, int, boolean);
E void losestr(int);
E void poisontell(int, boolean);
E void poisoned(const char *, int, const char *, int, boolean);
E void change_luck(schar);
E int stone_luck(boolean);
E void set_moreluck(void);
E void restore_attrib(void);
E void exercise(int, boolean);
E void exerchk(void);
E void init_attr(int);
E void redist_attr(void);
E void adjabil(int, int);
E int newhp(void);
E schar acurr(int);
E schar acurrstr(void);
E boolean extremeattr(int);
E void adjalign(int);
E int is_innate(int);
E char *from_what(int);
E void uchangealign(int, int);

/* ### ball.c ### */

E void ballrelease(boolean);
E void ballfall(void);
E void placebc(void);
E void unplacebc(void);
E void set_bc(int);
E void move_bc(int, int, xchar, xchar, xchar, xchar);
E boolean drag_ball(xchar, xchar, int *, xchar *, xchar *,
                            xchar *, xchar *, boolean *, boolean);
E void drop_ball(xchar, xchar);
E void drag_down(void);

/* ### bones.c ### */

E void sanitize_name(char *);
E void drop_upon_death(struct monst *, struct obj *, int, int);
E boolean can_make_bones(void);
E void savebones(int, time_t, struct obj *);
E int getbones(void);

/* ### botl.c ### */

E char *do_statusline1(void);
E char *do_statusline2(void);
E int xlev_to_rank(int);
E int title_to_mon(const char *, int *, int *);
E void max_rank_sz(void);
#ifdef SCORE_ON_BOTL
E long botl_score(void);
#endif
E int describe_level(char *);
E const char *rank_of(int, short, boolean);
E void bot(void);
#ifdef STATUS_VIA_WINDOWPORT
E void status_initialize(boolean);
E void status_finish(void);
E void status_notify_windowport(boolean);
#ifdef STATUS_HILITES
E boolean set_status_hilites(char *op, boolean);
E void clear_status_hilites(boolean);
E char *get_status_hilites(char *, int);
E boolean status_hilite_menu(void);
#endif
#endif

/* ### cmd.c ### */

E boolean redraw_cmd(char);
#ifdef USE_TRAMPOLI
E int doextcmd(void);
E int domonability(void);
E int doprev_message(void);
E int timed_occupation(void);
E int doattributes(void);
E int wiz_detect(void);
E int wiz_genesis(void);
E int wiz_identify(void);
E int wiz_level_tele(void);
E int wiz_map(void);
E int wiz_where(void);
E int wiz_wish(void);
#endif /* USE_TRAMPOLI */
E void reset_occupations(void);
E void set_occupation(int (*)(void), const char *, int);
E char pgetchar(void);
E void pushch(char);
E void savech(char);
E const char *key2extcmddesc(uchar);
E boolean bind_specialkey(uchar, const char *);
E char txt2key(char *);
E void parseautocomplete(char *, boolean);
E void reset_commands(boolean);
E void rhack(char *);
E int doextlist(void);
E int extcmd_via_menu(void);
E int enter_explore_mode(void);
E void enlightenment(int, int);
E void youhiding(boolean, int);
E void show_conduct(int);
E boolean bind_key(uchar, const char *);
E void dokeylist(void);
E int xytod(schar, schar);
E void dtoxy(coord *, int);
E int movecmd(char);
E int dxdy_moveok(void);
E int getdir(const char *);
E void confdir(void);
E const char *directionname(int);
E int isok(int, int);
E int get_adjacent_loc
            (const char *, const char *, xchar, xchar, coord *);
E const char *click_to_cmd(int, int, int);
E char get_count(char *, char, long, long *, boolean);
#ifdef HANGUPHANDLING
E void hangup(int);
E void end_of_input(void);
#endif
E char readchar(void);
E void sanity_check(void);
E char* key2txt(uchar, char *);
E char yn_function(const char *, const char *, char);
E boolean paranoid_query(boolean, const char *);

/* ### dbridge.c ### */

E boolean is_pool(int, int);
E boolean is_lava(int, int);
E boolean is_pool_or_lava(int, int);
E boolean is_ice(int, int);
E boolean is_moat(int, int);
E schar db_under_typ(int);
E int is_drawbridge_wall(int, int);
E boolean is_db_wall(int, int);
E boolean find_drawbridge(int *, int *);
E boolean create_drawbridge(int, int, int, boolean);
E void open_drawbridge(int, int);
E void close_drawbridge(int, int);
E void destroy_drawbridge(int, int);

/* ### decl.c ### */

E void decl_init(void);

/* ### detect.c ### */

E boolean trapped_chest_at(int, int, int);
E boolean trapped_door_at(int, int, int);
E struct obj *o_in(struct obj *, char);
E struct obj *o_material(struct obj *, unsigned);
E int gold_detect(struct obj *);
E int food_detect(struct obj *);
E int object_detect(struct obj *, int);
E int monster_detect(struct obj *, int);
E int trap_detect(struct obj *);
E const char *level_distance(d_level *);
E void use_crystal_ball(struct obj **);
E void do_mapping(void);
E void do_vicinity_map(struct obj *);
E void cvt_sdoor_to_door(struct rm *);
#ifdef USE_TRAMPOLI
E void findone(int, int, genericptr_t);
E void openone(int, int, genericptr_t);
#endif
E int findit(void);
E int openit(void);
E boolean detecting(void (*)(int, int, genericptr));
E void find_trap(struct trap *);
E void warnreveal(void);
E int dosearch0(int);
E int dosearch(void);
E void sokoban_detect(void);
#ifdef DUMPLOG
E void dump_map(void);
#endif
E void reveal_terrain(int, int);

/* ### dig.c ### */

E int dig_typ(struct obj *, xchar, xchar);
E boolean is_digging(void);
#ifdef USE_TRAMPOLI
E int dig(void);
#endif
E int holetime(void);
E boolean dig_check(struct monst *, boolean, int, int);
E void digactualhole(int, int, struct monst *, int);
E boolean dighole(boolean, boolean, coord *);
E int use_pick_axe(struct obj *);
E int use_pick_axe2(struct obj *);
E boolean mdig_tunnel(struct monst *);
E void draft_message(boolean);
E void watch_dig(struct monst *, xchar, xchar, boolean);
E void zap_dig(void);
E struct obj *bury_an_obj(struct obj *, boolean *);
E void bury_objs(int, int);
E void unearth_objs(int, int);
E void rot_organic(ANY_P *, long);
E void rot_corpse(ANY_P *, long);
E struct obj *buried_ball(coord *);
E void buried_ball_to_punishment(void);
E void buried_ball_to_freedom(void);
E schar fillholetyp(int, int, boolean);
E void liquid_flow(xchar, xchar, schar, struct trap *, const char *);
E boolean conjoined_pits(struct trap *, struct trap *, boolean);
#if 0
E void bury_monst(struct monst *);
E void bury_you(void);
E void unearth_you(void);
E void escape_tomb(void);
E void bury_obj(struct obj *);
#endif

/* ### display.c ### */

E void magic_map_background(xchar, xchar, int);
E void map_background(xchar, xchar, int);
E void map_trap(struct trap *, int);
E void map_object(struct obj *, int);
E void map_invisible(xchar, xchar);
E void unmap_object(int, int);
E void map_location(int, int, int);
E void feel_newsym(xchar, xchar);
E void feel_location(xchar, xchar);
E void newsym(int, int);
E void newsym_force(int, int);
E void shieldeff(xchar, xchar);
E void tmp_at(int, int);
E void swallowed(int);
E void under_ground(int);
E void under_water(int);
E void see_monsters(void);
E void set_mimic_blocking(void);
E void see_objects(void);
E void see_traps(void);
E void curs_on_u(void);
E int doredraw(void);
E void docrt(void);
E void show_glyph(int, int, int);
E void clear_glyph_buffer(void);
E void row_refresh(int, int, int);
E void cls(void);
E void flush_screen(int);
E int back_to_glyph(xchar, xchar);
E int zapdir_to_glyph(int, int, int);
E int glyph_at(xchar, xchar);
E void set_wall_state(void);
E void unset_seenv(struct rm *, int, int, int, int);
E int warning_of(struct monst *);

/* ### do.c ### */

#ifdef USE_TRAMPOLI
E int drop(struct obj *);
E int wipeoff(void);
#endif
E int dodrop(void);
E boolean boulder_hits_pool(struct obj *, int, int, boolean);
E boolean flooreffects(struct obj *, int, int, const char *);
E void doaltarobj(struct obj *);
E boolean canletgo(struct obj *, const char *);
E void dropx(struct obj *);
E void dropy(struct obj *);
E void dropz(struct obj *, boolean);
E void obj_no_longer_held(struct obj *);
E int doddrop(void);
E int dodown(void);
E int doup(void);
#ifdef INSURANCE
E void save_currentstate(void);
#endif
E void goto_level(d_level *, boolean, boolean, boolean);
E void schedule_goto(d_level *, boolean, boolean, int,
                             const char *, const char *);
E void deferred_goto(void);
E boolean revive_corpse(struct obj *);
E void revive_mon(ANY_P *, long);
E int donull(void);
E int dowipe(void);
E void set_wounded_legs(long, int);
E void heal_legs(void);

/* ### do_name.c ### */

E char *coord_desc(int, int, char *, char);
E boolean getpos_menu(coord *, int);
E int getpos(coord *, boolean, const char *);
E void getpos_sethilite(void (*f)(int), boolean (*d)(int,int));
E void new_mname(struct monst *, int);
E void free_mname(struct monst *);
E void new_oname(struct obj *, int);
E void free_oname(struct obj *);
E const char *safe_oname(struct obj *);
E struct monst *christen_monst(struct monst *, const char *);
E struct obj *oname(struct obj *, const char *);
E boolean objtyp_is_callable(int);
E int docallcmd(void);
E void docall(struct obj *);
E const char *rndghostname(void);
E char *x_monnam(struct monst *, int, const char *, int, boolean);
E char *l_monnam(struct monst *);
E char *mon_nam(struct monst *);
E char *noit_mon_nam(struct monst *);
E char *Monnam(struct monst *);
E char *noit_Monnam(struct monst *);
E char *noname_monnam(struct monst *, int);
E char *m_monnam(struct monst *);
E char *y_monnam(struct monst *);
E char *Adjmonnam(struct monst *, const char *);
E char *Amonnam(struct monst *);
E char *a_monnam(struct monst *);
E char *distant_monnam(struct monst *, int, char *);
E char *rndmonnam(char *);
E const char *hcolor(const char *);
E const char *rndcolor(void);
E const char *hliquid(const char *);
E const char *roguename(void);
E struct obj *realloc_obj
                    (struct obj *, int, genericptr_t, int, const char *);
E char *coyotename(struct monst *, char *);
E const char *noveltitle(int *);
E const char *lookup_novel(const char *, int *);

/* ### do_wear.c ### */

#ifdef USE_TRAMPOLI
E int Armor_on(void);
E int Boots_on(void);
E int Gloves_on(void);
E int Helmet_on(void);
E int select_off(struct obj *);
E int take_off(void);
#endif
E void off_msg(struct obj *);
E void set_wear(struct obj *);
E boolean donning(struct obj *);
E boolean doffing(struct obj *);
E void cancel_doff(struct obj *, long);
E void cancel_don(void);
E int stop_donning(struct obj *);
E int Armor_off(void);
E int Armor_gone(void);
E int Helmet_off(void);
E int Gloves_off(void);
E int Boots_on(void);
E int Boots_off(void);
E int Cloak_off(void);
E int Shield_off(void);
E int Shirt_off(void);
E void Amulet_off(void);
E void Ring_on(struct obj *);
E void Ring_off(struct obj *);
E void Ring_gone(struct obj *);
E void Blindf_on(struct obj *);
E void Blindf_off(struct obj *);
E int dotakeoff(void);
E int doremring(void);
E int cursed(struct obj *);
E int armoroff(struct obj *);
E int canwearobj(struct obj *, long *, boolean);
E int dowear(void);
E int doputon(void);
E void find_ac(void);
E void glibr(void);
E struct obj *some_armor(struct monst *);
E struct obj *stuck_ring(struct obj *, int);
E struct obj *unchanger(void);
E void reset_remarm(void);
E int doddoremarm(void);
E int destroy_arm(struct obj *);
E void adj_abon(struct obj *, schar);
E boolean
inaccessible_equipment(struct obj *, const char *, boolean);

/* ### dog.c ### */

E void newedog(struct monst *);
E void free_edog(struct monst *);
E void initedog(struct monst *);
E struct monst *make_familiar(struct obj *, xchar, xchar, boolean);
E struct monst *makedog(void);
E void update_mlstmv(void);
E void losedogs(void);
E void mon_arrive(struct monst *, boolean);
E void mon_catchup_elapsed_time(struct monst *, long);
E void keepdogs(boolean);
E void migrate_to_level(struct monst *, xchar, xchar, coord *);
E int dogfood(struct monst *, struct obj *);
E boolean tamedog(struct monst *, struct obj *);
E void abuse_dog(struct monst *);
E void wary_dog(struct monst *, boolean);

/* ### dogmove.c ### */

E struct obj *droppables(struct monst *);
E int dog_nutrition(struct monst *, struct obj *);
E int dog_eat(struct monst *, struct obj *, int, int, boolean);
E int dog_move(struct monst *, int);
#ifdef USE_TRAMPOLI
E void wantdoor(int, int, genericptr_t);
#endif
E void finish_meating(struct monst *);

/* ### dokick.c ### */

E boolean ghitm(struct monst *, struct obj *);
E void container_impact_dmg(struct obj *, xchar, xchar);
E int dokick(void);
E boolean ship_object(struct obj *, xchar, xchar, boolean);
E void obj_delivery(boolean);
E schar down_gate(xchar, xchar);
E void impact_drop(struct obj *, xchar, xchar, xchar);

/* ### dothrow.c ### */

E int dothrow(void);
E int dofire(void);
E void endmultishot(boolean);
E void hitfloor(struct obj *);
E void hurtle(int, int, int, boolean);
E void mhurtle(struct monst *, int, int, int);
E boolean throwing_weapon(struct obj *);
E void throwit(struct obj *, long, boolean);
E int omon_adj(struct monst *, struct obj *, boolean);
E int thitmonst(struct monst *, struct obj *);
E int hero_breaks(struct obj *, xchar, xchar, boolean);
E int breaks(struct obj *, xchar, xchar);
E void release_camera_demon(struct obj *, xchar, xchar);
E void breakobj(struct obj *, xchar, xchar, boolean, boolean);
E boolean breaktest(struct obj *);
E boolean walk_path(coord *, coord *,
                            boolean (*)(genericptr, int, int), genericptr_t);
E boolean hurtle_step(genericptr_t, int, int);

/* ### drawing.c ### */
#endif /* !MAKEDEFS_C && !LEV_LEX_C */
E int def_char_to_objclass(char);
E int def_char_to_monclass(char);
#if !defined(MAKEDEFS_C) && !defined(LEV_LEX_C)
E void switch_symbols(int);
E void assign_graphics(int);
E void init_r_symbols(void);
E void init_symbols(void);
E void update_bouldersym(void);
E void init_showsyms(void);
E void init_l_symbols(void);
E void clear_symsetentry(int, boolean);
E void update_l_symset(struct symparse *, int);
E void update_r_symset(struct symparse *, int);
E boolean cursed_object_at(int, int);

/* ### dungeon.c ### */

E void save_dungeon(int, boolean, boolean);
E void restore_dungeon(int);
E void insert_branch(branch *, boolean);
E void init_dungeons(void);
E s_level *find_level(const char *);
E s_level *Is_special(d_level *);
E branch *Is_branchlev(d_level *);
E boolean builds_up(d_level *);
E xchar ledger_no(d_level *);
E xchar maxledgerno(void);
E schar depth(d_level *);
E xchar dunlev(d_level *);
E xchar dunlevs_in_dungeon(d_level *);
E xchar ledger_to_dnum(xchar);
E xchar ledger_to_dlev(xchar);
E xchar deepest_lev_reached(boolean);
E boolean on_level(d_level *, d_level *);
E void next_level(boolean);
E void prev_level(boolean);
E void u_on_newpos(int, int);
E void u_on_rndspot(int);
E void u_on_sstairs(int);
E void u_on_upstairs(void);
E void u_on_dnstairs(void);
E boolean On_stairs(xchar, xchar);
E void get_level(d_level *, int);
E boolean Is_botlevel(d_level *);
E boolean Can_fall_thru(d_level *);
E boolean Can_dig_down(d_level *);
E boolean Can_rise_up(int, int, d_level *);
E boolean has_ceiling(d_level *);
E boolean In_quest(d_level *);
E boolean In_mines(d_level *);
E branch *dungeon_branch(const char *);
E boolean at_dgn_entrance(const char *);
E boolean In_hell(d_level *);
E boolean In_V_tower(d_level *);
E boolean On_W_tower_level(d_level *);
E boolean In_W_tower(int, int, d_level *);
E void find_hell(d_level *);
E void goto_hell(boolean, boolean);
E void assign_level(d_level *, d_level *);
E void assign_rnd_level(d_level *, d_level *, int);
E int induced_align(int);
E boolean Invocation_lev(d_level *);
E xchar level_difficulty(void);
E schar lev_by_name(const char *);
E schar print_dungeon(boolean, schar *, xchar *);
E char *get_annotation(d_level *);
E int donamelevel(void);
E int dooverview(void);
E void show_overview(int, int);
E void forget_mapseen(int);
E void init_mapseen(d_level *);
E void recalc_mapseen(void);
E void mapseen_temple(struct monst *);
E void room_discovered(int);
E void recbranch_mapseen(d_level *, d_level *);
E void overview_stats(winid, const char *, long *, long *);
E void remdun_mapseen(int);

/* ### eat.c ### */

#ifdef USE_TRAMPOLI
E int eatmdone(void);
E int eatfood(void);
E int opentin(void);
E int unfaint(void);
#endif
E void eatmupdate(void);
E boolean is_edible(struct obj *);
E void init_uhunger(void);
E int Hear_again(void);
E void reset_eat(void);
E int doeat(void);
E int use_tin_opener(struct obj *);
E void gethungry(void);
E void morehungry(int);
E void lesshungry(int);
E boolean is_fainted(void);
E void reset_faint(void);
E void violated_vegetarian(void);
E void newuhs(boolean);
E struct obj *floorfood(const char *, int);
E void vomit(void);
E int eaten_stat(int, struct obj *);
E void food_disappears(struct obj *);
E void food_substitution(struct obj *, struct obj *);
E void eating_conducts(struct permonst *);
E int eat_brains(struct monst *, struct monst *, boolean, int *);
E void fix_petrification(void);
E void consume_oeaten(struct obj *, int);
E boolean maybe_finished_meal(boolean);
E void set_tin_variety(struct obj *, int);
E int tin_variety_txt(char *, int *);
E void tin_details(struct obj *, int, char *);
E boolean Popeye(int);

/* ### end.c ### */

E void done1(int);
E int done2(void);
#ifdef USE_TRAMPOLI
E void done_intr(int);
#endif
E void done_in_by(struct monst *, int);
#endif /* !MAKEDEFS_C && !LEV_LEX_C */
E void panic(const char *, ...) PRINTF_F(1, 2) NORETURN;
#if !defined(MAKEDEFS_C) && !defined(LEV_LEX_C)
E void done(int);
E void container_contents(struct obj *, boolean,
                                  boolean, boolean);
E void nh_terminate(int) NORETURN;
E int dovanquished(void);
E int num_genocides(void);
E void delayed_killer(int, int, const char *);
E struct kinfo *find_delayed_killer(int);
E void dealloc_killer(struct kinfo *);
E void save_killers(int, int);
E void restore_killers(int);
E char *build_english_list(char *);
#if defined(PANICTRACE) && !defined(NO_SIGNAL)
E void panictrace_setsignals(boolean);
#endif

/* ### engrave.c ### */

E char *random_engraving(char *);
E void wipeout_text(char *, int, unsigned);
E boolean can_reach_floor(boolean);
E void cant_reach_floor(int, int, boolean, boolean);
E const char *surface(int, int);
E const char *ceiling(int, int);
E struct engr *engr_at(xchar, xchar);
E int sengr_at(const char *, xchar, xchar, boolean);
E void u_wipe_engr(int);
E void wipe_engr_at(xchar, xchar, xchar, boolean);
E void read_engr_at(int, int);
E void make_engr_at(int, int, const char *, long, xchar);
E void del_engr_at(int, int);
E int freehand(void);
E int doengrave(void);
E void sanitize_engravings(void);
E void save_engravings(int, int);
E void rest_engravings(int);
E void engr_stats(const char *, char *, long *, long *);
E void del_engr(struct engr *);
E void rloc_engr(struct engr *);
E void make_grave(int, int, const char *);

/* ### exper.c ### */

E int newpw(void);
E int experience(struct monst *, int);
E void more_experienced(int, int);
E void losexp(const char *);
E void newexplevel(void);
E void pluslvl(boolean);
E long rndexp(boolean);

/* ### explode.c ### */

E void explode(int, int, int, int, char, int);
E long scatter(int, int, int, unsigned int, struct obj *);
E void splatter_burning_oil(int, int);
E void explode_oil(struct obj *, int, int);

/* ### extralev.c ### */

E void makeroguerooms(void);
E void corr(int, int);
E void makerogueghost(void);

/* ### files.c ### */

E char *fname_encode(const char *, char, char *, char *, int);
E char *fname_decode(char, char *, char *, int);
E const char *fqname(const char *, int, int);
E FILE *fopen_datafile(const char *, const char *, int);
#ifdef MFLOPPY
E void set_lock_and_bones(void);
#endif
E void set_levelfile_name(char *, int);
E int create_levelfile(int, char *);
E int open_levelfile(int, char *);
E void delete_levelfile(int);
E void clearlocks(void);
E int create_bonesfile(d_level *, char **, char *);
#ifdef MFLOPPY
E void cancel_bonesfile(void);
#endif
E void commit_bonesfile(d_level *);
E int open_bonesfile(d_level *, char **);
E int delete_bonesfile(d_level *);
E void compress_bonesfile(void);
E void set_savefile_name(boolean);
#ifdef INSURANCE
E void save_savefile_name(int);
#endif
#ifndef MICRO
E void set_error_savefile(void);
#endif
E int create_savefile(void);
E int open_savefile(void);
E int delete_savefile(void);
E int restore_saved_game(void);
E void nh_compress(const char *);
E void nh_uncompress(const char *);
E boolean lock_file(const char *, int, int);
E void unlock_file(const char *);
#ifdef USER_SOUNDS
E boolean can_read_file(const char *);
#endif
E void config_error_init(boolean, const char *);
E void config_error_add(const char *, ...) PRINTF_F(1, 2);
E int config_error_done(void);
E boolean read_config_file(const char *, int);
E void check_recordfile(const char *);
E void read_wizkit(void);
E int read_sym_file(int);
E int parse_sym_line(char *, int);
E void paniclog(const char *, const char *);
E int validate_prefix_locations(char *);
#ifdef SELECTSAVED
E char *plname_from_file(const char *);
#endif
E char **get_saved_games(void);
E void free_saved_games(char **);
#ifdef SELF_RECOVER
E boolean recover_savefile(void);
#endif
#ifdef SYSCF_FILE
E void assure_syscf_file(void);
#endif
E int nhclose(int);
#ifdef HOLD_LOCKFILE_OPEN
E void really_close(void);
#endif
#ifdef DEBUG
E boolean debugcore(const char *, boolean);
#endif
E boolean read_tribute(const char *, const char *, int,
                               char *, int, unsigned);
E boolean Death_quote(char *, int);

/* ### fountain.c ### */

E void floating_above(const char *);
E void dogushforth(int);
#ifdef USE_TRAMPOLI
E void gush(int, int, genericptr_t);
#endif
E void dryup(xchar, xchar, boolean);
E void drinkfountain(void);
E void dipfountain(struct obj *);
E void breaksink(int, int);
E void drinksink(void);

/* ### hack.c ### */

E boolean is_valid_travelpt(int,int);
E anything *uint_to_any(unsigned);
E anything *long_to_any(long);
E anything *monst_to_any(struct monst *);
E anything *obj_to_any(struct obj *);
E boolean revive_nasty(int, int, const char *);
E void movobj(struct obj *, xchar, xchar);
E boolean may_dig(xchar, xchar);
E boolean may_passwall(xchar, xchar);
E boolean bad_rock(struct permonst *, xchar, xchar);
E int cant_squeeze_thru(struct monst *);
E boolean invocation_pos(xchar, xchar);
E boolean test_move(int, int, int, int, int);
#ifdef DEBUG
E int wiz_debug_cmd_traveldisplay(void);
#endif
E boolean u_rooted(void);
E void domove(void);
E boolean overexertion(void);
E void invocation_message(void);
E boolean pooleffects(boolean);
E void spoteffects(boolean);
E char *in_rooms(xchar, xchar, int);
E boolean in_town(int, int);
E void check_special_room(boolean);
E int dopickup(void);
E void lookaround(void);
E boolean crawl_destination(int, int);
E int monster_nearby(void);
E void nomul(int);
E void unmul(const char *);
E void losehp(int, const char *, boolean);
E int weight_cap(void);
E int inv_weight(void);
E int near_capacity(void);
E int calc_capacity(int);
E int max_capacity(void);
E boolean check_capacity(const char *);
E int inv_cnt(boolean);
E long money_cnt(struct obj *);

/* ### hacklib.c ### */

E boolean digit(char);
E boolean letter(char);
E char highc(char);
E char lowc(char);
E char *lcase(char *);
E char *ucase(char *);
E char *upstart(char *);
E char *mungspaces(char *);
E char *trimspaces(char *);
E char *strip_newline(char *);
E char *eos(char *);
E boolean str_end_is(const char *, const char *);
E char *strkitten(char *, char);
E void copynchars(char *, const char *, int);
E char chrcasecpy(int, int);
E char *strcasecpy(char *, const char *);
E char *s_suffix(const char *);
E char *ing_suffix(const char *);
E char *xcrypt(const char *, char *);
E boolean onlyspace(const char *);
E char *tabexpand(char *);
E char *visctrl(char);
E char *strsubst(char *, const char *, const char *);
E int strNsubst(char *, const char *, const char *, int);
E const char *ordin(int);
E char *sitoa(int);
E int sgn(int);
E int rounddiv(long, int);
E int dist2(int, int, int, int);
E int isqrt(int);
E int distmin(int, int, int, int);
E boolean online2(int, int, int, int);
E boolean pmatch(const char *, const char *);
E boolean pmatchi(const char *, const char *);
E boolean pmatchz(const char *, const char *);
#ifndef STRNCMPI
E int strncmpi(const char *, const char *, int);
#endif
#ifndef STRSTRI
E char *strstri(const char *, const char *);
#endif
E boolean
fuzzymatch(const char *, const char *, const char *, boolean);
E void setrandom(void);
E time_t getnow(void);
E int getyear(void);
#if 0
E char *yymmdd(time_t);
#endif
E long yyyymmdd(time_t);
E long hhmmss(time_t);
E char *yyyymmddhhmmss(time_t);
E time_t time_from_yyyymmddhhmmss(char *);
E int phase_of_the_moon(void);
E boolean friday_13th(void);
E int night(void);
E int midnight(void);

/* ### invent.c ### */

E void sortloot(struct obj **, unsigned, boolean);
E void assigninvlet(struct obj *);
E struct obj *merge_choice(struct obj *, struct obj *);
E int merged(struct obj **, struct obj **);
#ifdef USE_TRAMPOLI
E int ckunpaid(struct obj *);
#endif
E void addinv_core1(struct obj *);
E void addinv_core2(struct obj *);
E struct obj *addinv(struct obj *);
E struct obj *hold_another_object
                    (struct obj *, const char *, const char *, const char *);
E void useupall(struct obj *);
E void useup(struct obj *);
E void consume_obj_charge(struct obj *, boolean);
E void freeinv_core(struct obj *);
E void freeinv(struct obj *);
E void delallobj(int, int);
E void delobj(struct obj *);
E struct obj *sobj_at(int, int, int);
E struct obj *nxtobj(struct obj *, int, boolean);
E struct obj *carrying(int);
E boolean have_lizard(void);
E struct obj *u_have_novel(void);
E struct obj *o_on(unsigned int, struct obj *);
E boolean obj_here(struct obj *, int, int);
E boolean wearing_armor(void);
E boolean is_worn(struct obj *);
E struct obj *g_at(int, int);
E boolean splittable(struct obj *);
E struct obj *getobj(const char *, const char *);
E int ggetobj(const char *, int (*)(OBJ_P), int,
                      boolean, unsigned *);
E int askchain(struct obj **, const char *, int, int (*)(OBJ_P),
                       int (*)(OBJ_P), int, const char *);
E void fully_identify_obj(struct obj *);
E int identify(struct obj *);
E void identify_pack(int, boolean);
E void learn_unseen_invent(void);
E void prinv(const char *, struct obj *, long);
E char *xprname
              (struct obj *, const char *, char, boolean, long, long);
E int ddoinv(void);
E char display_inventory(const char *, boolean);
E int display_binventory(int, int, boolean);
E struct obj *display_cinventory(struct obj *);
E struct obj *display_minventory(struct monst *, int, char *);
E int dotypeinv(void);
E const char *dfeature_at(int, int, char *);
E int look_here(int, boolean);
E int dolook(void);
E boolean will_feel_cockatrice(struct obj *, boolean);
E void feel_cockatrice(struct obj *, boolean);
E void stackobj(struct obj *);
E boolean mergable(struct obj *, struct obj *);
E int doprgold(void);
E int doprwep(void);
E int doprarm(void);
E int doprring(void);
E int dopramulet(void);
E int doprtool(void);
E int doprinuse(void);
E void useupf(struct obj *, long);
E char *let_to_name(char, boolean, boolean);
E void free_invbuf(void);
E void reassign(void);
E int doorganize(void);
E void free_pickinv_cache(void);
E int count_unpaid(struct obj *);
E int count_buc(struct obj *, int, boolean (*)(OBJ_P));
E void tally_BUCX(struct obj *, boolean,
                          int *, int *, int *, int *, int *);
E long count_contents(struct obj *, boolean, boolean, boolean);
E void carry_obj_effects(struct obj *);
E const char *currency(long);
E void silly_thing(const char *, struct obj *);

/* ### ioctl.c ### */

#if defined(UNIX) || defined(__BEOS__)
E void getwindowsz(void);
E void getioctls(void);
E void setioctls(void);
#ifdef SUSPEND
E int dosuspend(void);
#endif /* SUSPEND */
#endif /* UNIX || __BEOS__ */

/* ### light.c ### */

E void new_light_source(xchar, xchar, int, int, ANY_P *);
E void del_light_source(int, ANY_P *);
E void do_light_sources(char **);
E struct monst *find_mid(unsigned, unsigned);
E void save_light_sources(int, int, int);
E void restore_light_sources(int);
E void light_stats(const char *, char *, long *, long *);
E void relink_light_sources(boolean);
E void light_sources_sanity_check(void);
E void obj_move_light_source(struct obj *, struct obj *);
E boolean any_light_source(void);
E void snuff_light_source(int, int);
E boolean obj_sheds_light(struct obj *);
E boolean obj_is_burning(struct obj *);
E void obj_split_light_source(struct obj *, struct obj *);
E void obj_merge_light_sources(struct obj *, struct obj *);
E void obj_adjust_light_radius(struct obj *, int);
E int candle_light_range(struct obj *);
E int arti_light_radius(struct obj *);
E const char *arti_light_description(struct obj *);
E int wiz_light_sources(void);

/* ### lock.c ### */

#ifdef USE_TRAMPOLI
E int forcelock(void);
E int picklock(void);
#endif
E boolean picking_lock(int *, int *);
E boolean picking_at(int, int);
E void breakchestlock(struct obj *, boolean);
E void reset_pick(void);
E void maybe_reset_pick(void);
E int pick_lock(struct obj *);
E int doforce(void);
E boolean boxlock(struct obj *, struct obj *);
E boolean doorlock(struct obj *, int, int);
E int doopen(void);
E boolean stumble_on_door_mimic(int, int);
E int doopen_indir(int, int);
E int doclose(void);

#ifdef MAC
/* These declarations are here because the main code calls them. */

/* ### macfile.c ### */

E int maccreat(const char *, long);
E int macopen(const char *, int, long);
E int macclose(int);
E int macread(int, void *, unsigned);
E int macwrite(int, void *, unsigned);
E long macseek(int, long, short);
E int macunlink(const char *);

/* ### macmain.c ### */

E boolean authorize_wizard_mode(void);

/* ### macsnd.c ### */

E void mac_speaker(struct obj *, char *);

/* ### macunix.c ### */

E void regularize(char *);
E void getlock(void);

/* ### macwin.c ### */

E void lock_mouse_cursor(Boolean);
E int SanePositions(void);

/* ### mttymain.c ### */

E void getreturn(char *);
E void msmsg(const char *, ...);
E void gettty(void);
E void setftty(void);
E void settty(const char *);
E int tgetch(void);
E void cmov(int x, int y);
E void nocmov(int x, int y);

#endif /* MAC */

/* ### mail.c ### */

#ifdef MAIL
#ifdef UNIX
E void free_maildata(void);
E void getmailstatus(void);
E void ck_server_admin_msg(void);
#endif
E void ckmailstatus(void);
E void readmail(struct obj *);
#endif /* MAIL */

/* ### makemon.c ### */

E void dealloc_monst(struct monst *);
E boolean is_home_elemental(struct permonst *);
E struct monst *clone_mon(struct monst *, xchar, xchar);
E int monhp_per_lvl(struct monst *);
E void newmonhp(struct monst *, int);
E struct mextra *newmextra(void);
E void copy_mextra(struct monst *, struct monst *);
E void dealloc_mextra(struct monst *);
E struct monst *makemon(struct permonst *, int, int, int);
E boolean create_critters(int, struct permonst *, boolean);
E struct permonst *rndmonst(void);
E void reset_rndmonst(int);
E struct permonst *mkclass(char, int);
E int mkclass_poly(int);
E int adj_lev(struct permonst *);
E struct permonst *grow_up(struct monst *, struct monst *);
E int mongets(struct monst *, int);
E int golemhp(int);
E boolean peace_minded(struct permonst *);
E void set_malign(struct monst *);
E void newmcorpsenm(struct monst *);
E void freemcorpsenm(struct monst *);
E void set_mimic_sym(struct monst *);
E int mbirth_limit(int);
E void mimic_hit_msg(struct monst *, short);
E void mkmonmoney(struct monst *, long);
E int bagotricks(struct obj *, boolean, int *);
E boolean propagate(int, boolean, boolean);
E boolean usmellmon(struct permonst *);

/* ### mapglyph.c ### */

E int mapglyph(int, int *, int *, unsigned *, int, int);
E char *encglyph(int);
E void genl_putmixed(winid, int, const char *);

/* ### mcastu.c ### */

E int castmu(struct monst *, struct attack *, boolean, boolean);
E int buzzmu(struct monst *, struct attack *);

/* ### mhitm.c ### */

E int fightm(struct monst *);
E int mattackm(struct monst *, struct monst *);
E boolean engulf_target(struct monst *, struct monst *);
E int mdisplacem(struct monst *, struct monst *, boolean);
E void paralyze_monst(struct monst *, int);
E int sleep_monst(struct monst *, int, int);
E void slept_monst(struct monst *);
E void xdrainenergym(struct monst *, boolean);
E long attk_protection(int);
E void rustm(struct monst *, struct obj *);

/* ### mhitu.c ### */

E const char *mpoisons_subj(struct monst *, struct attack *);
E void u_slow_down(void);
E struct monst *cloneu(void);
E void expels(struct monst *, struct permonst *, boolean);
E struct attack *getmattk(struct monst *, struct monst *, int, int *, struct attack *);
E int mattacku(struct monst *);
E int magic_negation(struct monst *);
E boolean gulp_blnd_check(void);
E int gazemu(struct monst *, struct attack *);
E void mdamageu(struct monst *, int);
E int could_seduce(struct monst *, struct monst *, struct attack *);
E int doseduce(struct monst *);

/* ### minion.c ### */

E void newemin(struct monst *);
E void free_emin(struct monst *);
E int monster_census(boolean);
E int msummon(struct monst *);
E void summon_minion(aligntyp, boolean);
E int demon_talk(struct monst *);
E long bribe(struct monst *);
E int dprince(aligntyp);
E int dlord(aligntyp);
E int llord(void);
E int ndemon(aligntyp);
E int lminion(void);
E void lose_guardian_angel(struct monst *);
E void gain_guardian_angel(void);

/* ### mklev.c ### */

#ifdef USE_TRAMPOLI
E int do_comp(genericptr_t, genericptr_t);
#endif
E void sort_rooms(void);
E void add_room(int, int, int, int, boolean, schar, boolean);
E void add_subroom(struct mkroom *, int, int, int, int, boolean,
                           schar, boolean);
E void makecorridors(void);
E void add_door(int, int, struct mkroom *);
E void mklev(void);
#ifdef SPECIALIZATION
E void topologize(struct mkroom *, boolean);
#else
E void topologize(struct mkroom *);
#endif
E void place_branch(branch *, xchar, xchar);
E boolean occupied(xchar, xchar);
E int okdoor(xchar, xchar);
E void dodoor(int, int, struct mkroom *);
E void mktrap(int, int, struct mkroom *, coord *);
E void mkstairs(xchar, xchar, char, struct mkroom *);
E void mkinvokearea(void);
E void mineralize(int, int, int, int, boolean);

/* ### mkmap.c ### */

E void flood_fill_rm(int, int, int, boolean, boolean);
E void remove_rooms(int, int, int, int);
/* E void mkmap(lev_init *); -- need sp_lev.h for lev_init */

/* ### mkmaze.c ### */

E void wallification(int, int, int, int);
E void fix_wall_spines(int, int, int, int);
E void walkfrom(int, int, schar);
E void makemaz(const char *);
E void mazexy(coord *);
E void bound_digging(void);
E void mkportal(xchar, xchar, xchar, xchar);
E boolean bad_location(xchar, xchar, xchar, xchar,
                               xchar, xchar);
E void place_lregion(xchar, xchar, xchar, xchar, xchar,
                             xchar, xchar, xchar, xchar, d_level *);
E void fixup_special(void);
E void fumaroles(void);
E void movebubbles(void);
E void water_friction(void);
E void save_waterlevel(int, int);
E void restore_waterlevel(int);
E const char *waterbody_name(xchar, xchar);

/* ### mkobj.c ### */

E struct oextra *newoextra(void);
E void copy_oextra(struct obj *, struct obj *);
E void dealloc_oextra(struct obj *);
E void newomonst(struct obj *);
E void free_omonst(struct obj *);
E void newomid(struct obj *);
E void free_omid(struct obj *);
E void newolong(struct obj *);
E void free_olong(struct obj *);
E void new_omailcmd(struct obj *, const char *);
E void free_omailcmd(struct obj *);
E struct obj *mkobj_at(char, int, int, boolean);
E struct obj *mksobj_at(int, int, int, boolean, boolean);
E struct obj *mkobj(char, boolean);
E int rndmonnum(void);
E boolean bogon_is_pname(char);
E struct obj *splitobj(struct obj *, long);
E struct obj *unsplitobj(struct obj *);
E void clear_splitobjs(void);
E void replace_object(struct obj *, struct obj *);
E void bill_dummy_object(struct obj *);
E void costly_alteration(struct obj *, int);
E struct obj *mksobj(int, boolean, boolean);
E int bcsign(struct obj *);
E int weight(struct obj *);
E struct obj *mkgold(long, int, int);
E struct obj *mkcorpstat(int, struct monst *, struct permonst *, int,
                                 int, unsigned);
E int corpse_revive_type(struct obj *);
E struct obj *obj_attach_mid(struct obj *, unsigned);
E struct monst *get_mtraits(struct obj *, boolean);
E struct obj *mk_tt_object(int, int, int);
E struct obj *mk_named_object
                    (int, struct permonst *, int, int, const char *);
E struct obj *rnd_treefruit_at(int, int);
E void set_corpsenm(struct obj *, int);
E void start_corpse_timeout(struct obj *);
E void bless(struct obj *);
E void unbless(struct obj *);
E void curse(struct obj *);
E void uncurse(struct obj *);
E void blessorcurse(struct obj *, int);
E boolean is_flammable(struct obj *);
E boolean is_rottable(struct obj *);
E void place_object(struct obj *, int, int);
E void remove_object(struct obj *);
E void discard_minvent(struct monst *);
E void obj_extract_self(struct obj *);
E void extract_nobj(struct obj *, struct obj **);
E void extract_nexthere(struct obj *, struct obj **);
E int add_to_minv(struct monst *, struct obj *);
E struct obj *add_to_container(struct obj *, struct obj *);
E void add_to_migration(struct obj *);
E void add_to_buried(struct obj *);
E void dealloc_obj(struct obj *);
E void obj_ice_effects(int, int, boolean);
E long peek_at_iced_corpse_age(struct obj *);
E int hornoplenty(struct obj *, boolean);
E void obj_sanity_check(void);
E struct obj *obj_nexto(struct obj *);
E struct obj *obj_nexto_xy(struct obj *, int, int, boolean);
E struct obj *obj_absorb(struct obj **, struct obj **);
E struct obj *obj_meld(struct obj **, struct obj **);
E void pudding_merge_message(struct obj *, struct obj *);

/* ### mkroom.c ### */

E void mkroom(int);
E void fill_zoo(struct mkroom *);
E struct permonst *antholemon(void);
E boolean nexttodoor(int, int);
E boolean has_dnstairs(struct mkroom *);
E boolean has_upstairs(struct mkroom *);
E int somex(struct mkroom *);
E int somey(struct mkroom *);
E boolean inside_room(struct mkroom *, xchar, xchar);
E boolean somexy(struct mkroom *, coord *);
E void mkundead(coord *, boolean, int);
E struct permonst *courtmon(void);
E void save_rooms(int);
E void rest_rooms(int);
E struct mkroom *search_special(schar);
E int cmap_to_type(int);

/* ### mon.c ### */

E void mon_sanity_check(void);
E int undead_to_corpse(int);
E int genus(int, int);
E int pm_to_cham(int);
E int minliquid(struct monst *);
E int movemon(void);
E int meatmetal(struct monst *);
E int meatobj(struct monst *);
E void mpickgold(struct monst *);
E boolean mpickstuff(struct monst *, const char *);
E int curr_mon_load(struct monst *);
E int max_mon_load(struct monst *);
E int can_carry(struct monst *, struct obj *);
E int mfndpos(struct monst *, coord *, long *, long);
E boolean monnear(struct monst *, int, int);
E void dmonsfree(void);
E int mcalcmove(struct monst *);
E void mcalcdistress(void);
E void replmon(struct monst *, struct monst *);
E void relmon(struct monst *, struct monst **);
E struct obj *mlifesaver(struct monst *);
E boolean corpse_chance(struct monst *, struct monst *, boolean);
E void mondead(struct monst *);
E void mondied(struct monst *);
E void mongone(struct monst *);
E void monstone(struct monst *);
E void monkilled(struct monst *, const char *, int);
E void unstuck(struct monst *);
E void killed(struct monst *);
E void xkilled(struct monst *, int);
E void mon_to_stone(struct monst *);
E void m_into_limbo(struct monst *);
E void mnexto(struct monst *);
E void maybe_mnexto(struct monst *);
E boolean mnearto(struct monst *, xchar, xchar, boolean);
E void m_respond(struct monst *);
E void setmangry(struct monst *, boolean);
E void wakeup(struct monst *, boolean);
E void wake_nearby(void);
E void wake_nearto(int, int, int);
E void seemimic(struct monst *);
E void rescham(void);
E void restartcham(void);
E void restore_cham(struct monst *);
E boolean hideunder(struct monst *);
E void hide_monst(struct monst *);
E void mon_animal_list(boolean);
E int select_newcham_form(struct monst *);
E void mgender_from_permonst(struct monst *, struct permonst *);
E int newcham
            (struct monst *, struct permonst *, boolean, boolean);
E int can_be_hatched(int);
E int egg_type_from_parent(int, boolean);
E boolean dead_species(int, boolean);
E void kill_genocided_monsters(void);
E void golemeffects(struct monst *, int, int);
E boolean angry_guards(boolean);
E void pacify_guards(void);
E void decide_to_shapeshift(struct monst *, int);
E boolean vamp_stone(struct monst *);

/* ### mondata.c ### */

E void set_mon_data(struct monst *, struct permonst *, int);
E struct attack *attacktype_fordmg(struct permonst *, int, int);
E boolean attacktype(struct permonst *, int);
E boolean noattacks(struct permonst *);
E boolean poly_when_stoned(struct permonst *);
E boolean resists_drli(struct monst *);
E boolean resists_magm(struct monst *);
E boolean resists_blnd(struct monst *);
E boolean
can_blnd(struct monst *, struct monst *, uchar, struct obj *);
E boolean ranged_attk(struct permonst *);
E boolean hates_silver(struct permonst *);
E boolean mon_hates_silver(struct monst *);
E boolean passes_bars(struct permonst *);
E boolean can_blow(struct monst *);
E boolean can_be_strangled(struct monst *);
E boolean can_track(struct permonst *);
E boolean breakarm(struct permonst *);
E boolean sliparm(struct permonst *);
E boolean sticks(struct permonst *);
E boolean cantvomit(struct permonst *);
E int num_horns(struct permonst *);
/* E boolean canseemon(struct monst *); */
E struct attack *dmgtype_fromattack(struct permonst *, int, int);
E boolean dmgtype(struct permonst *, int);
E int max_passive_dmg(struct monst *, struct monst *);
E boolean same_race(struct permonst *, struct permonst *);
E int monsndx(struct permonst *);
E int name_to_mon(const char *);
E int name_to_monclass(const char *, int *);
E int gender(struct monst *);
E int pronoun_gender(struct monst *);
E boolean levl_follower(struct monst *);
E int little_to_big(int);
E int big_to_little(int);
E boolean big_little_match(int, int);
E const char *locomotion(const struct permonst *, const char *);
E const char *stagger(const struct permonst *, const char *);
E const char *on_fire(struct permonst *, struct attack *);
E const struct permonst *raceptr(struct monst *);
E boolean olfaction(struct permonst *);

/* ### monmove.c ### */

E boolean itsstuck(struct monst *);
E boolean mb_trapped(struct monst *);
E boolean monhaskey(struct monst *, boolean);
E void mon_regen(struct monst *, boolean);
E int dochugw(struct monst *);
E boolean onscary(int, int, struct monst *);
E void monflee(struct monst *, int, boolean, boolean);
E void mon_yells(struct monst *, const char *);
E int dochug(struct monst *);
E boolean m_digweapon_check(struct monst *, xchar, xchar);
E int m_move(struct monst *, int);
E void dissolve_bars(int, int);
E boolean closed_door(int, int);
E boolean accessible(int, int);
E void set_apparxy(struct monst *);
E boolean can_ooze(struct monst *);
E boolean can_fog(struct monst *);
E boolean should_displace
                (struct monst *, coord *, long *, int, xchar, xchar);
E boolean undesirable_disp(struct monst *, xchar, xchar);

/* ### monst.c ### */

E void monst_init(void);

/* ### monstr.c ### */

E void monstr_init(void);

/* ### mplayer.c ### */

E struct monst *mk_mplayer(struct permonst *, xchar, xchar, boolean);
E void create_mplayers(int, boolean);
E void mplayer_talk(struct monst *);

#if defined(MICRO) || defined(WIN32)

/* ### msdos.c,os2.c,tos.c,winnt.c ### */

#ifndef WIN32
E int tgetch(void);
#endif
#ifndef TOS
E char switchar(void);
#endif
#ifndef __GO32__
E long freediskspace(char *);
#ifdef MSDOS
E int findfirst_file(char *);
E int findnext_file(void);
E long filesize_nh(char *);
#else
E int findfirst(char *);
E int findnext(void);
E long filesize(char *);
#endif /* MSDOS */
E char *foundfile_buffer(void);
#endif /* __GO32__ */
E void chdrive(char *);
#ifndef TOS
E void disable_ctrlP(void);
E void enable_ctrlP(void);
#endif
#if defined(MICRO) && !defined(WINNT)
E void get_scr_size(void);
#ifndef TOS
E void gotoxy(int, int);
#endif
#endif
#ifdef TOS
E int _copyfile(char *, char *);
E int kbhit(void);
E void set_colors(void);
E void restore_colors(void);
#ifdef SUSPEND
E int dosuspend(void);
#endif
#endif /* TOS */
#ifdef WIN32
E char *get_username(int *);
E void nt_regularize(char *);
E int (*nt_kbhit)(void);
E void Delay(int);
#endif /* WIN32 */
#endif /* MICRO || WIN32 */

/* ### mthrowu.c ### */

E int thitu(int, int, struct obj *, const char *);
E int ohitmon(struct monst *, struct obj *, int, boolean);
E void thrwmu(struct monst *);
E int spitmu(struct monst *, struct attack *);
E int breamu(struct monst *, struct attack *);
E boolean linedup(xchar, xchar, xchar, xchar, int);
E boolean lined_up(struct monst *);
E struct obj *m_carrying(struct monst *, int);
E int thrwmm(struct monst *, struct monst *);
E int spitmm(struct monst *, struct attack *, struct monst *);
E int breamm(struct monst *, struct attack *, struct monst *);
E void m_useupall(struct monst *, struct obj *);
E void m_useup(struct monst *, struct obj *);
E void m_throw(struct monst *, int, int, int, int, int, struct obj *);
E void hit_bars(struct obj **, int, int, int, int,
                        boolean, boolean);
E boolean hits_bars(struct obj **, int, int, int, int, int, int);

/* ### muse.c ### */

E boolean find_defensive(struct monst *);
E int use_defensive(struct monst *);
E int rnd_defensive_item(struct monst *);
E boolean find_offensive(struct monst *);
#ifdef USE_TRAMPOLI
E int mbhitm(struct monst *, struct obj *);
#endif
E int use_offensive(struct monst *);
E int rnd_offensive_item(struct monst *);
E boolean find_misc(struct monst *);
E int use_misc(struct monst *);
E int rnd_misc_item(struct monst *);
E boolean searches_for_item(struct monst *, struct obj *);
E boolean mon_reflects(struct monst *, const char *);
E boolean ureflects(const char *, const char *);
E void mcureblindness(struct monst *, boolean);
E boolean munstone(struct monst *, boolean);
E boolean munslime(struct monst *, boolean);

/* ### music.c ### */

E void awaken_soldiers(struct monst *);
E int do_play_instrument(struct obj *);

/* ### nhlan.c ### */
#ifdef LAN_FEATURES
E void init_lan_features(void);
E char *lan_username(void);
#endif

/* ### nhregex.c ### */
E struct nhregex *regex_init(void);
E boolean regex_compile(const char *, struct nhregex *);
E const char *regex_error_desc(struct nhregex *);
E boolean regex_match(const char *, struct nhregex *);
E void regex_free(struct nhregex *);

/* ### nttty.c ### */

#ifdef WIN32
E void get_scr_size(void);
E int nttty_kbhit(void);
E void nttty_open(int);
E void nttty_rubout(void);
E int tgetch(void);
E int ntposkey(int *, int *, int *);
E void set_output_mode(int);
E void synch_cursor(void);
#endif

/* ### o_init.c ### */

E void init_objects(void);
E void obj_shuffle_range(int, int *, int *);
E int find_skates(void);
E void oinit(void);
E void savenames(int, int);
E void restnames(int);
E void discover_object(int, boolean, boolean);
E void undiscover_object(int);
E int dodiscovered(void);
E int doclassdisco(void);
E void rename_disco(void);

/* ### objects.c ### */

E void objects_init(void);

/* ### objnam.c ### */

E char *obj_typename(int);
E char *simple_typename(int);
E boolean obj_is_pname(struct obj *);
E char *distant_name(struct obj *, char *(*)(OBJ_P));
E char *fruitname(boolean);
E struct fruit *fruit_from_indx(int);
E struct fruit *fruit_from_name(const char *, boolean, int *);
E void reorder_fruit(boolean);
E char *xname(struct obj *);
E char *mshot_xname(struct obj *);
E boolean the_unique_obj(struct obj *);
E boolean the_unique_pm(struct permonst *);
E boolean erosion_matters(struct obj *);
E char *doname(struct obj *);
E char *doname_with_price(struct obj *);
E char *doname_vague_quan(struct obj *);
E boolean not_fully_identified(struct obj *);
E char *corpse_xname(struct obj *, const char *, unsigned);
E char *cxname(struct obj *);
E char *cxname_singular(struct obj *);
E char *killer_xname(struct obj *);
E char *short_oname
              (struct obj *, char *(*)(OBJ_P), char *(*)(OBJ_P), unsigned);
E const char *singular(struct obj *, char *(*)(OBJ_P));
E char *an(const char *);
E char *An(const char *);
E char *The(const char *);
E char *the(const char *);
E char *aobjnam(struct obj *, const char *);
E char *yobjnam(struct obj *, const char *);
E char *Yobjnam2(struct obj *, const char *);
E char *Tobjnam(struct obj *, const char *);
E char *otense(struct obj *, const char *);
E char *vtense(const char *, const char *);
E char *Doname2(struct obj *);
E char *yname(struct obj *);
E char *Yname2(struct obj *);
E char *ysimple_name(struct obj *);
E char *Ysimple_name2(struct obj *);
E char *simpleonames(struct obj *);
E char *ansimpleoname(struct obj *);
E char *thesimpleoname(struct obj *);
E char *bare_artifactname(struct obj *);
E char *makeplural(const char *);
E char *makesingular(const char *);
E struct obj *readobjnam(char *, struct obj *);
E int rnd_class(int, int);
E const char *suit_simple_name(struct obj *);
E const char *cloak_simple_name(struct obj *);
E const char *helm_simple_name(struct obj *);
E const char *mimic_obj_name(struct monst *);
E char *safe_qbuf(char *, const char *, const char *, struct obj *,
                          char *(*)(OBJ_P), char *(*)(OBJ_P), const char *);

/* ### options.c ### */

E void reglyph_darkroom(void);
E boolean match_optname(const char *, const char *, int, boolean);
E void initoptions(void);
E void initoptions_init(void);
E void initoptions_finish(void);
E boolean parseoptions(char *, boolean, boolean);
E int doset(void);
E int dotogglepickup(void);
E void option_help(void);
E void next_opt(winid, const char *);
E int fruitadd(char *, struct fruit *);
E int choose_classes_menu(const char *, int, boolean,
                          char *, char *);
E boolean parsebindings(char *);
E void add_menu_cmd_alias(char, char);
E char get_menu_cmd_key(char);
E char map_menu_cmd(char);
E void show_menu_controls(winid, boolean);
E void assign_warnings(uchar *);
E char *nh_getenv(const char *);
E void set_duplicate_opt_detection(int);
E void set_wc_option_mod_status(unsigned long, int);
E void set_wc2_option_mod_status(unsigned long, int);
E void set_option_mod_status(const char *, int);
E int add_autopickup_exception(const char *);
E void free_autopickup_exceptions(void);
E int load_symset(const char *, int);
E void free_symsets(void);
E boolean parsesymbols(char *);
E struct symparse *match_sym(char *);
E void set_playmode(void);
E int sym_val(const char *);
E const char *clr2colorname(int);
E int match_str2clr(char *);
E boolean add_menu_coloring(char *);
E boolean get_menu_coloring(char *, int *, int *);
E void free_menu_coloring(void);
E boolean msgtype_parse_add(char *);
E int msgtype_type(const char *, boolean);
E void hide_unhide_msgtypes(boolean, int);
E void msgtype_free(void);

/* ### pager.c ### */

E char *self_lookat(char *);
E void mhidden_description(struct monst *, boolean, char *);
E boolean object_from_map(int,int,int,struct obj **);
E int do_screen_description(coord, boolean, int, char *,
                            const char **);
E int do_look(int, coord *);
E int dowhatis(void);
E int doquickwhatis(void);
E int doidtrap(void);
E int dowhatdoes(void);
E char *dowhatdoes_core(char, char *);
E int dohelp(void);
E int dohistory(void);

/* ### pcmain.c ### */

#if defined(MICRO) || defined(WIN32)
#ifdef CHDIR
E void chdirx(char *, boolean);
#endif /* CHDIR */
E boolean authorize_wizard_mode(void);
#endif /* MICRO || WIN32 */

/* ### pcsys.c ### */

#if defined(MICRO) || defined(WIN32)
E void flushout(void);
E int dosh(void);
#ifdef MFLOPPY
E void eraseall(const char *, const char *);
E void copybones(int);
E void playwoRAMdisk(void);
E int saveDiskPrompt(int);
E void gameDiskPrompt(void);
#endif
E void append_slash(char *);
E void getreturn(const char *);
#ifndef AMIGA
E void msmsg(const char *, ...);
#endif
E FILE *fopenp(const char *, const char *);
#endif /* MICRO || WIN32 */

/* ### pctty.c ### */

#if defined(MICRO) || defined(WIN32)
E void gettty(void);
E void settty(const char *);
E void setftty(void);
E void error(const char *, ...);
#if defined(TIMED_DELAY) && defined(_MSC_VER)
E void msleep(unsigned);
#endif
#endif /* MICRO || WIN32 */

/* ### pcunix.c ### */

#if defined(MICRO)
E void regularize(char *);
#endif /* MICRO */
#if defined(PC_LOCKING)
E void getlock(void);
#endif

/* ### pickup.c ### */

E int collect_obj_classes(char *, struct obj *, boolean,
                                  boolean (*)(OBJ_P), int *);
E boolean rider_corpse_revival(struct obj *, boolean);
E boolean menu_class_present(int);
E void add_valid_menu_class(int);
E boolean allow_all(struct obj *);
E boolean allow_category(struct obj *);
E boolean is_worn_by_type(struct obj *);
E int ck_bag(struct obj *);
#ifdef USE_TRAMPOLI
E int in_container(struct obj *);
E int out_container(struct obj *);
#endif
E int pickup(int);
E int pickup_object(struct obj *, long, boolean);
E int query_category(const char *, struct obj *, int,
                             menu_item **, int);
E int query_objlist(const char *, struct obj **, int,
                            menu_item **, int, boolean (*)(OBJ_P));
E struct obj *pick_obj(struct obj *);
E int encumber_msg(void);
E int container_at(int, int, boolean);
E int doloot(void);
E boolean container_gone(int (*)(OBJ_P));
E boolean u_handsy(void);
E int use_container(struct obj **, int, boolean);
E int loot_mon(struct monst *, int *, boolean *);
E int dotip(void);
E boolean is_autopickup_exception(struct obj *, boolean);

/* ### pline.c ### */

#ifdef DUMPLOG
E void dumplogmsg(const char *);
E void dumplogfreemessages(void);
#endif
E void pline(const char *, ...) PRINTF_F(1, 2);
E void custompline(unsigned, const char *, ...) PRINTF_F(2, 3);
E void Norep(const char *, ...) PRINTF_F(1, 2);
E void free_youbuf(void);
E void You(const char *, ...) PRINTF_F(1, 2);
E void Your(const char *, ...) PRINTF_F(1, 2);
E void You_feel(const char *, ...) PRINTF_F(1, 2);
E void You_cant(const char *, ...) PRINTF_F(1, 2);
E void You_hear(const char *, ...) PRINTF_F(1, 2);
E void You_see(const char *, ...) PRINTF_F(1, 2);
E void pline_The(const char *, ...) PRINTF_F(1, 2);
E void There(const char *, ...) PRINTF_F(1, 2);
E void verbalize(const char *, ...) PRINTF_F(1, 2);
E void raw_printf(const char *, ...) PRINTF_F(1, 2);
E void impossible(const char *, ...) PRINTF_F(1, 2);

/* ### polyself.c ### */

E void set_uasmon(void);
E void float_vs_flight(void);
E void change_sex(void);
E void polyself(int);
E int polymon(int);
E void rehumanize(void);
E int dobreathe(void);
E int dospit(void);
E int doremove(void);
E int dospinweb(void);
E int dosummon(void);
E int dogaze(void);
E int dohide(void);
E int dopoly(void);
E int domindblast(void);
E void skinback(boolean);
E const char *mbodypart(struct monst *, int);
E const char *body_part(int);
E int poly_gender(void);
E void ugolemeffects(int, int);

/* ### potion.c ### */

E void set_itimeout(long *, long);
E void incr_itimeout(long *, int);
E void make_confused(long, boolean);
E void make_stunned(long, boolean);
E void make_blinded(long, boolean);
E void make_sick(long, const char *, boolean, int);
E void make_slimed(long, const char *);
E void make_stoned(long, const char *, int, const char *);
E void make_vomiting(long, boolean);
E boolean make_hallucinated(long, boolean, long);
E void make_deaf(long, boolean);
E void self_invis_message(void);
E int dodrink(void);
E int dopotion(struct obj *);
E int peffects(struct obj *);
E void healup(int, int, boolean, boolean);
E void strange_feeling(struct obj *, const char *);
E void potionhit(struct monst *, struct obj *, boolean);
E void potionbreathe(struct obj *);
E int dodip(void);
E void mongrantswish(struct monst **);
E void djinni_from_bottle(struct obj *);
E struct monst *split_mon(struct monst *, struct monst *);
E const char *bottlename(void);

/* ### pray.c ### */

E boolean critically_low_hp(boolean);
#ifdef USE_TRAMPOLI
E int prayer_done(void);
#endif
E int dosacrifice(void);
E boolean can_pray(boolean);
E int dopray(void);
E const char *u_gname(void);
E int doturn(void);
E const char *a_gname(void);
E const char *a_gname_at(xchar x, xchar y);
E const char *align_gname(aligntyp);
E const char *halu_gname(aligntyp);
E const char *align_gtitle(aligntyp);
E void altar_wrath(int, int);

/* ### priest.c ### */

E int move_special(struct monst *, boolean, schar, boolean,
                           boolean, xchar, xchar, xchar, xchar);
E char temple_occupied(char *);
E boolean inhistemple(struct monst *);
E int pri_move(struct monst *);
E void priestini(d_level *, struct mkroom *, int, int, boolean);
E aligntyp mon_aligntyp(struct monst *);
E char *priestname(struct monst *, char *);
E boolean p_coaligned(struct monst *);
E struct monst *findpriest(char);
E void intemple(int);
E void forget_temple_entry(struct monst *);
E void priest_talk(struct monst *);
E struct monst *mk_roamer(struct permonst *, aligntyp, xchar,
                                  xchar, boolean);
E void reset_hostility(struct monst *);
E boolean in_your_sanctuary(struct monst *, xchar, xchar);
E void ghod_hitsu(struct monst *);
E void angry_priest(void);
E void clearpriests(void);
E void restpriest(struct monst *, boolean);
E void newepri(struct monst *);
E void free_epri(struct monst *);
E const char *align_str(aligntyp);
E char *piousness(boolean, const char *);
E void mstatusline(struct monst *);
E void ustatusline(void);

/* ### quest.c ### */

E void onquest(void);
E void nemdead(void);
E void artitouch(struct obj *);
E boolean ok_to_quest(void);
E void leader_speaks(struct monst *);
E void nemesis_speaks(void);
E void quest_chat(struct monst *);
E void quest_talk(struct monst *);
E void quest_stat_check(struct monst *);
E void finish_quest(struct obj *);

/* ### questpgr.c ### */

E void load_qtlist(void);
E void unload_qtlist(void);
E short quest_info(int);
E const char *ldrname(void);
E boolean is_quest_artifact(struct obj *);
E void com_pager(int);
E void qt_pager(int);
E struct permonst *qt_montype(void);
E void deliver_splev_message(void);

/* ### random.c ### */

#if defined(RANDOM) && !defined(__GO32__) /* djgpp has its own random */
E void srandom(unsigned);
E char *initstate(unsigned, char *, int);
E char *setstate(char *);
E long random(void);
#endif /* RANDOM */

/* ### read.c ### */

E void learnscroll(struct obj *);
E char *tshirt_text(struct obj *, char *);
E int doread(void);
E boolean is_chargeable(struct obj *);
E void recharge(struct obj *, int);
E void forget_objects(int);
E void forget_levels(int);
E void forget_traps(void);
E void forget_map(int);
E int seffects(struct obj *);
E void drop_boulder_on_player(boolean, boolean, boolean, boolean);
E boolean drop_boulder_on_monster(int, int, boolean, boolean);
E void wand_explode(struct obj *, int);
#ifdef USE_TRAMPOLI
E void set_lit(int, int, genericptr_t);
#endif
E void litroom(boolean, struct obj *);
E void do_genocide(int);
E void punish(struct obj *);
E void unpunish(void);
E boolean cant_revive(int *, boolean, struct obj *);
E boolean create_particular(void);

/* ### rect.c ### */

E void init_rect(void);
E NhRect *get_rect(NhRect *);
E NhRect *rnd_rect(void);
E void remove_rect(NhRect *);
E void add_rect(NhRect *);
E void split_rects(NhRect *, NhRect *);

/* ## region.c ### */
E void clear_regions(void);
E void run_regions(void);
E boolean in_out_region(xchar, xchar);
E boolean m_in_out_region(struct monst *, xchar, xchar);
E void update_player_regions(void);
E void update_monster_region(struct monst *);
E NhRegion *visible_region_at(xchar, xchar);
E void show_region(NhRegion *, xchar, xchar);
E void save_regions(int, int);
E void rest_regions(int, boolean);
E void region_stats(const char *, char *, long *, long *);
E NhRegion *create_gas_cloud(xchar, xchar, int, int);
E boolean region_danger(void);
E void region_safety(void);

/* ### restore.c ### */

E void inven_inuse(boolean);
E int dorecover(int);
E void restcemetery(int, struct cemetery **);
E void trickery(char *);
E void getlev(int, int, xchar, boolean);
E void get_plname_from_file(int, char *);
#ifdef SELECTSAVED
E int restore_menu(winid);
#endif
E void minit(void);
E boolean lookup_id_mapping(unsigned, unsigned *);
E void mread(int, genericptr_t, unsigned int);
E int validate(int, const char *);
E void reset_restpref(void);
E void set_restpref(const char *);
E void set_savepref(const char *);

/* ### rip.c ### */

E void genl_outrip(winid, int, time_t);

/* ### rnd.c ### */

E int rn2(int);
E int rnl(int);
E int rnd(int);
E int d(int, int);
E int rne(int);
E int rnz(int);

/* ### role.c ### */

E boolean validrole(int);
E boolean validrace(int, int);
E boolean validgend(int, int, int);
E boolean validalign(int, int, int);
E int randrole(void);
E int randrace(int);
E int randgend(int, int);
E int randalign(int, int);
E int str2role(const char *);
E int str2race(const char *);
E int str2gend(const char *);
E int str2align(const char *);
E boolean ok_role(int, int, int, int);
E int pick_role(int, int, int, int);
E boolean ok_race(int, int, int, int);
E int pick_race(int, int, int, int);
E boolean ok_gend(int, int, int, int);
E int pick_gend(int, int, int, int);
E boolean ok_align(int, int, int, int);
E int pick_align(int, int, int, int);
E void rigid_role_checks(void);
E boolean setrolefilter(const char *);
E boolean gotrolefilter(void);
E void clearrolefilter(void);
E char *build_plselection_prompt(char *, int, int, int, int, int);
E char *root_plselection_prompt(char *, int, int, int, int, int);
E void plnamesuffix(void);
E void role_selection_prolog(int, winid);
E void role_menu_extra(int, winid, boolean);
E void role_init(void);
E const char *Hello(struct monst *);
E const char *Goodbye(void);

/* ### rumors.c ### */

E char *getrumor(int, char *, boolean);
E char *get_rnd_text(const char *, char *);
E void outrumor(int, int);
E void outoracle(boolean, boolean);
E void save_oracles(int, int);
E void restore_oracles(int);
E int doconsult(struct monst *);
E void rumor_check(void);

/* ### save.c ### */

E int dosave(void);
E int dosave0(void);
E boolean tricked_fileremoved(int, char *);
#ifdef INSURANCE
E void savestateinlock(void);
#endif
#ifdef MFLOPPY
E boolean savelev(int, xchar, int);
E boolean swapin_file(int);
E void co_false(void);
#else
E void savelev(int, xchar, int);
#endif
E genericptr_t mon_to_buffer(struct monst *, int *);
E void bufon(int);
E void bufoff(int);
E void bflush(int);
E void bwrite(int, genericptr_t, unsigned int);
E void bclose(int);
E void def_bclose(int);
#if defined(ZEROCOMP)
E void zerocomp_bclose(int);
#endif
E void savecemetery(int, int, struct cemetery **);
E void savefruitchn(int, int);
E void store_plname_in_file(int);
E void free_dungeons(void);
E void freedynamicdata(void);
E void store_savefileinfo(int);

/* ### shk.c ### */

E void setpaid(struct monst *);
E long money2mon(struct monst *, long);
E void money2u(struct monst *, long);
E void shkgone(struct monst *);
E void set_residency(struct monst *, boolean);
E void replshk(struct monst *, struct monst *);
E void restshk(struct monst *, boolean);
E char inside_shop(xchar, xchar);
E void u_left_shop(char *, boolean);
E void remote_burglary(xchar, xchar);
E void u_entered_shop(char *);
E void pick_pick(struct obj *);
E boolean same_price(struct obj *, struct obj *);
E void shopper_financial_report(void);
E int inhishop(struct monst *);
E struct monst *shop_keeper(char);
E boolean tended_shop(struct mkroom *);
E boolean is_unpaid(struct obj *);
E void delete_contents(struct obj *);
E void obfree(struct obj *, struct obj *);
E void home_shk(struct monst *, boolean);
E void make_happy_shk(struct monst *, boolean);
E void make_happy_shoppers(boolean);
E void hot_pursuit(struct monst *);
E void make_angry_shk(struct monst *, xchar, xchar);
E int dopay(void);
E boolean paybill(int);
E void finish_paybill(void);
E struct obj *find_oid(unsigned);
E long contained_cost
             (struct obj *, struct monst *, long, boolean, boolean);
E long contained_gold(struct obj *);
E void picked_container(struct obj *);
E void alter_cost(struct obj *, long);
E long unpaid_cost(struct obj *, boolean);
E boolean billable(struct monst **, struct obj *, char, boolean);
E void addtobill(struct obj *, boolean, boolean, boolean);
E void splitbill(struct obj *, struct obj *);
E void subfrombill(struct obj *, struct monst *);
E long stolen_value
             (struct obj *, xchar, xchar, boolean, boolean);
E void sellobj_state(int);
E void sellobj(struct obj *, xchar, xchar);
E int doinvbill(int);
E struct monst *shkcatch(struct obj *, xchar, xchar);
E void add_damage(xchar, xchar, long);
E int repair_damage(struct monst *, struct damage *, boolean);
E int shk_move(struct monst *);
E void after_shk_move(struct monst *);
E boolean is_fshk(struct monst *);
E void shopdig(int);
E void pay_for_damage(const char *, boolean);
E boolean costly_spot(xchar, xchar);
E struct obj *shop_object(xchar, xchar);
E void price_quote(struct obj *);
E void shk_chat(struct monst *);
E void check_unpaid_usage(struct obj *, boolean);
E void check_unpaid(struct obj *);
E void costly_gold(xchar, xchar, long);
E long get_cost_of_shop_item(struct obj *);
E boolean block_door(xchar, xchar);
E boolean block_entry(xchar, xchar);
E char *shk_your(char *, struct obj *);
E char *Shk_Your(char *, struct obj *);

/* ### shknam.c ### */

E void neweshk(struct monst *);
E void free_eshk(struct monst *);
E void stock_room(int, struct mkroom *);
E boolean saleable(struct monst *, struct obj *);
E int get_shop_item(int);
E char *Shknam(struct monst *);
E char *shkname(struct monst *);
E boolean shkname_is_pname(struct monst *);
E boolean is_izchak(struct monst *, boolean);

/* ### sit.c ### */

E void take_gold(void);
E int dosit(void);
E void rndcurse(void);
E void attrcurse(void);

/* ### sounds.c ### */

E void dosounds(void);
E const char *growl_sound(struct monst *);
E void growl(struct monst *);
E void yelp(struct monst *);
E void whimper(struct monst *);
E void beg(struct monst *);
E int dotalk(void);
#ifdef USER_SOUNDS
E int add_sound_mapping(const char *);
E void play_sound_for_message(const char *);
#endif

/* ### sys.c ### */

E void sys_early_init(void);
E void sysopt_release(void);
E void sysopt_seduce_set(int);

/* ### sys/msdos/sound.c ### */

#ifdef MSDOS
E int assign_soundcard(char *);
#endif

/* ### sp_lev.c ### */

E boolean check_room(xchar *, xchar *, xchar *, xchar *, boolean);
E boolean create_room(xchar, xchar, xchar, xchar, xchar,
                              xchar, xchar, xchar);
E void create_secret_door(struct mkroom *, xchar);
E boolean
dig_corridor(coord *, coord *, boolean, schar, schar);
E void fill_room(struct mkroom *, boolean);
E boolean load_special(const char *);
E xchar selection_getpoint(int, int, struct opvar *);
E struct opvar *selection_opvar(char *);
E void opvar_free_x(struct opvar *);
E void set_selection_floodfillchk(int (*)(int,int));
E void selection_floodfill(struct opvar *, int, int, boolean);

/* ### spell.c ### */

E void book_cursed(struct obj *);
#ifdef USE_TRAMPOLI
E int learn(void);
#endif
E int study_book(struct obj *);
E void book_disappears(struct obj *);
E void book_substitution(struct obj *, struct obj *);
E void age_spells(void);
E int docast(void);
E int spell_skilltype(int);
E int spelleffects(int, boolean);
E void losespells(void);
E int dovspell(void);
E void initialspell(struct obj *);

/* ### steal.c ### */

#ifdef USE_TRAMPOLI
E int stealarm(void);
#endif
E long somegold(long);
E void stealgold(struct monst *);
E void remove_worn_item(struct obj *, boolean);
E int steal(struct monst *, char *);
E int mpickobj(struct monst *, struct obj *);
E void stealamulet(struct monst *);
E void maybe_absorb_item(struct monst *, struct obj *, int, int);
E void mdrop_obj(struct monst *, struct obj *, boolean);
E void mdrop_special_objs(struct monst *);
E void relobj(struct monst *, int, boolean);
E struct obj *findgold(struct obj *);

/* ### steed.c ### */

E void rider_cant_reach(void);
E boolean can_saddle(struct monst *);
E int use_saddle(struct obj *);
E void put_saddle_on_mon(struct obj *, struct monst *);
E boolean can_ride(struct monst *);
E int doride(void);
E boolean mount_steed(struct monst *, boolean);
E void exercise_steed(void);
E void kick_steed(void);
E void dismount_steed(int);
E void place_monster(struct monst *, int, int);
E boolean stucksteed(boolean);

/* ### teleport.c ### */

E boolean goodpos(int, int, struct monst *, unsigned);
E boolean enexto(coord *, xchar, xchar, struct permonst *);
E boolean enexto_core(coord *, xchar, xchar,
                              struct permonst *, unsigned);
E void teleds(int, int, boolean);
E boolean safe_teleds(boolean);
E boolean teleport_pet(struct monst *, boolean);
E void tele(void);
E boolean scrolltele(struct obj *);
E int dotele(void);
E void level_tele(void);
E void domagicportal(struct trap *);
E void tele_trap(struct trap *);
E void level_tele_trap(struct trap *, unsigned);
E void rloc_to(struct monst *, int, int);
E boolean rloc(struct monst *, boolean);
E boolean tele_restrict(struct monst *);
E void mtele_trap(struct monst *, struct trap *, int);
E int mlevel_tele_trap(struct monst *, struct trap *,
                               boolean, int);
E boolean rloco(struct obj *);
E int random_teleport_level(void);
E boolean u_teleport_mon(struct monst *, boolean);

/* ### tile.c ### */
#ifdef USE_TILES
E void substitute_tiles(d_level *);
#endif

/* ### timeout.c ### */

E void burn_away_slime(void);
E void nh_timeout(void);
E void fall_asleep(int, boolean);
E void attach_egg_hatch_timeout(struct obj *, long);
E void attach_fig_transform_timeout(struct obj *);
E void kill_egg(struct obj *);
E void hatch_egg(ANY_P *, long);
E void learn_egg_type(int);
E void burn_object(ANY_P *, long);
E void begin_burn(struct obj *, boolean);
E void end_burn(struct obj *, boolean);
E void do_storms(void);
E boolean start_timer(long, short, short, ANY_P *);
E long stop_timer(short, ANY_P *);
E long peek_timer(short, ANY_P *);
E void run_timers(void);
E void obj_move_timers(struct obj *, struct obj *);
E void obj_split_timers(struct obj *, struct obj *);
E void obj_stop_timers(struct obj *);
E boolean obj_has_timer(struct obj *, short);
E void spot_stop_timers(xchar, xchar, short);
E long spot_time_expires(xchar, xchar, short);
E long spot_time_left(xchar, xchar, short);
E boolean obj_is_local(struct obj *);
E void save_timers(int, int, int);
E void restore_timers(int, int, boolean, long);
E void timer_stats(const char *, char *, long *, long *);
E void relink_timers(boolean);
E int wiz_timeout_queue(void);
E void timer_sanity_check(void);

/* ### topten.c ### */

E void formatkiller(char *, unsigned, int, boolean);
E void topten(int, time_t);
E void prscore(int, char **);
E struct toptenentry *get_rnd_toptenentry(void);
E struct obj *tt_oname(struct obj *);

/* ### track.c ### */

E void initrack(void);
E void settrack(void);
E coord *gettrack(int, int);

/* ### trap.c ### */

E boolean burnarmor(struct monst *);
E int erode_obj(struct obj *, const char *, int, int);
E boolean grease_protect(struct obj *, const char *, struct monst *);
E struct trap *maketrap(int, int, int);
E void fall_through(boolean);
E struct monst *animate_statue(struct obj *, xchar, xchar, int, int *);
E struct monst *activate_statue_trap(struct trap *, xchar, xchar, boolean);
E void dotrap(struct trap *, unsigned);
E void seetrap(struct trap *);
E void feeltrap(struct trap *);
E int mintrap(struct monst *);
E void instapetrify(const char *);
E void minstapetrify(struct monst *, boolean);
E void selftouch(const char *);
E void mselftouch(struct monst *, const char *, boolean);
E void float_up(void);
E void fill_pit(int, int);
E int float_down(long, long);
E void climb_pit(void);
E boolean fire_damage(struct obj *, boolean, xchar, xchar);
E int fire_damage_chain(struct obj *, boolean, boolean, xchar, xchar);
E void acid_damage(struct obj *);
E int water_damage(struct obj *, const char *, boolean);
E void water_damage_chain(struct obj *, boolean);
E boolean drown(void);
E void drain_en(int);
E int dountrap(void);
E void cnv_trap_obj(int, int, struct trap *, boolean);
E int untrap(boolean);
E boolean openholdingtrap(struct monst *, boolean *);
E boolean closeholdingtrap(struct monst *, boolean *);
E boolean openfallingtrap(struct monst *, boolean, boolean *);
E boolean chest_trap(struct obj *, int, boolean);
E void deltrap(struct trap *);
E boolean delfloortrap(struct trap *);
E struct trap *t_at(int, int);
E void b_trapped(const char *, int);
E boolean unconscious(void);
E void blow_up_landmine(struct trap *);
E int launch_obj(short, int, int, int, int, int);
E boolean launch_in_progress(void);
E void force_launch_placement(void);
E boolean uteetering_at_seen_pit(struct trap *);
E boolean lava_effects(void);
E void sink_into_lava(void);
E void sokoban_guilt(void);

/* ### u_init.c ### */

E void u_init(void);

/* ### uhitm.c ### */

E void erode_armor(struct monst *, int);
E boolean attack_checks(struct monst *, struct obj *);
E void check_caitiff(struct monst *);
E int find_roll_to_hit(struct monst *, uchar, struct obj *, int *, int *);
E boolean attack(struct monst *);
E boolean hmon(struct monst *, struct obj *, int);
E int damageum(struct monst *, struct attack *);
E void missum(struct monst *, struct attack *, boolean);
E int passive(struct monst *, boolean, int, uchar, boolean);
E void passive_obj(struct monst *, struct obj *, struct attack *);
E void stumble_onto_mimic(struct monst *);
E int flash_hits_mon(struct monst *, struct obj *);
E void light_hits_gremlin(struct monst *, int);

/* ### unixmain.c ### */

#ifdef UNIX
#ifdef PORT_HELP
E void port_help(void);
#endif
E void sethanguphandler(void (*)(int));
E boolean authorize_wizard_mode(void);
E boolean check_user_string(char *);
#endif /* UNIX */

/* ### unixtty.c ### */

#if defined(UNIX) || defined(__BEOS__)
E void gettty(void);
E void settty(const char *);
E void setftty(void);
E void intron(void);
E void introff(void);
E void error(const char *, ...) PRINTF_F(1, 2);
#endif /* UNIX || __BEOS__ */

/* ### unixunix.c ### */

#ifdef UNIX
E void getlock(void);
E void regularize(char *);
#if defined(TIMED_DELAY) && !defined(msleep) && defined(SYSV)
E void msleep(unsigned);
#endif
#ifdef SHELL
E int dosh(void);
#endif /* SHELL */
#if defined(SHELL) || defined(DEF_PAGER) || defined(DEF_MAILREADER)
E int child(int);
#endif
#ifdef PANICTRACE
E boolean file_exists(const char *);
#endif
#endif /* UNIX */

/* ### unixres.c ### */

#ifdef UNIX
#ifdef GNOME_GRAPHICS
E int hide_privileges(boolean);
#endif
#endif /* UNIX */

/* ### vault.c ### */

E void newegd(struct monst *);
E void free_egd(struct monst *);
E boolean grddead(struct monst *);
E void vault_summon_gd(void);
E char vault_occupied(char *);
E void invault(void);
E int gd_move(struct monst *);
E void paygd(void);
E long hidden_gold(void);
E boolean gd_sound(void);
E void vault_gd_watching(unsigned int);

/* ### version.c ### */

E char *version_string(char *);
E char *getversionstring(char *);
E int doversion(void);
E int doextversion(void);
#ifdef MICRO
E boolean comp_times(long);
#endif
E boolean
check_version(struct version_info *, const char *, boolean);
E boolean uptodate(int, const char *);
E void store_version(int);
E unsigned long get_feature_notice_ver(char *);
E unsigned long get_current_feature_ver(void);
E const char *copyright_banner_line(int);

#ifdef RUNTIME_PORT_ID
E void append_port_id(char *);
#endif

/* ### video.c ### */

#ifdef MSDOS
E int assign_video(char *);
#ifdef NO_TERMS
E void gr_init(void);
E void gr_finish(void);
#endif
E void tileview(boolean);
#endif
#ifdef VIDEOSHADES
E int assign_videoshades(char *);
E int assign_videocolors(char *);
#endif

/* ### vis_tab.c ### */

#ifdef VISION_TABLES
E void vis_tab_init(void);
#endif

/* ### vision.c ### */

E void vision_init(void);
E int does_block(int, int, struct rm *);
E void vision_reset(void);
E void vision_recalc(int);
E void block_point(int, int);
E void unblock_point(int, int);
E boolean clear_path(int, int, int, int);
E void do_clear_area(int, int, int,
                             void (*)(int, int, genericptr), genericptr_t);
E unsigned howmonseen(struct monst *);

#ifdef VMS

/* ### vmsfiles.c ### */

E int vms_link(const char *, const char *);
E int vms_unlink(const char *);
E int vms_creat(const char *, unsigned int);
E int vms_open(const char *, int, unsigned int);
E boolean same_dir(const char *, const char *);
E int c__translate(int);
E char *vms_basename(const char *);

/* ### vmsmail.c ### */

E unsigned long init_broadcast_trapping(void);
E unsigned long enable_broadcast_trapping(void);
E unsigned long disable_broadcast_trapping(void);
#if 0
E struct mail_info *parse_next_broadcast(void);
#endif /*0*/

/* ### vmsmain.c ### */

E int main(int, char **);
#ifdef CHDIR
E void chdirx(const char *, boolean);
#endif /* CHDIR */
E void sethanguphandler(void (*)(int));
E boolean authorize_wizard_mode(void);

/* ### vmsmisc.c ### */

E void vms_abort(void) NORETURN;
E void vms_exit(int) NORETURN;
#ifdef PANICTRACE
E void vms_traceback(int);
#endif

/* ### vmstty.c ### */

E int vms_getchar(void);
E void gettty(void);
E void settty(const char *);
E void shuttty(const char *);
E void setftty(void);
E void intron(void);
E void introff(void);
E void error(const char *, ...) PRINTF_F(1, 2);
#ifdef TIMED_DELAY
E void msleep(unsigned);
#endif

/* ### vmsunix.c ### */

E void getlock(void);
E void regularize(char *);
E int vms_getuid(void);
E boolean file_is_stmlf(int);
E int vms_define(const char *, const char *, int);
E int vms_putenv(const char *);
E char *verify_termcap(void);
#if defined(CHDIR) || defined(SHELL) || defined(SECURE)
E void privoff(void);
E void privon(void);
#endif
#ifdef SHELL
E int dosh(void);
#endif
#if defined(SHELL) || defined(MAIL)
E int vms_doshell(const char *, boolean);
#endif
#ifdef SUSPEND
E int dosuspend(void);
#endif
#ifdef SELECTSAVED
E int vms_get_saved_games(const char *, char ***);
#endif

#endif /* VMS */

/* ### weapon.c ### */

E const char *weapon_descr(struct obj *);
E int hitval(struct obj *, struct monst *);
E int dmgval(struct obj *, struct monst *);
E struct obj *select_rwep(struct monst *);
E struct obj *select_hwep(struct monst *);
E void possibly_unwield(struct monst *, boolean);
E void mwepgone(struct monst *);
E int mon_wield_item(struct monst *);
E int abon(void);
E int dbon(void);
E void wet_a_towel(struct obj *, int, boolean);
E void dry_a_towel(struct obj *, int, boolean);
E int enhance_weapon_skill(void);
E void unrestrict_weapon_skill(int);
E void use_skill(int, int);
E void add_weapon_skill(int);
E void lose_weapon_skill(int);
E int weapon_type(struct obj *);
E int uwep_skill_type(void);
E int weapon_hit_bonus(struct obj *);
E int weapon_dam_bonus(struct obj *);
E void skill_init(const struct def_skill *);

/* ### were.c ### */

E void were_change(struct monst *);
E int counter_were(int);
E int were_beastie(int);
E void new_were(struct monst *);
E int were_summon(struct permonst *, boolean, int *, char *);
E void you_were(void);
E void you_unwere(boolean);
E void set_ulycn(int);

/* ### wield.c ### */

E void setuwep(struct obj *);
E void setuqwep(struct obj *);
E void setuswapwep(struct obj *);
E int dowield(void);
E int doswapweapon(void);
E int dowieldquiver(void);
E boolean wield_tool(struct obj *, const char *);
E int can_twoweapon(void);
E void drop_uswapwep(void);
E int dotwoweapon(void);
E void uwepgone(void);
E void uswapwepgone(void);
E void uqwepgone(void);
E void untwoweapon(void);
E int chwepon(struct obj *, int);
E int welded(struct obj *);
E void weldmsg(struct obj *);
E void setmnotwielded(struct monst *, struct obj *);
E boolean mwelded(struct obj *);

/* ### windows.c ### */

E void choose_windows(const char *);
#ifdef WINCHAIN
void addto_windowchain(const char *s);
void commit_windowchain(void);
#endif
E boolean genl_can_suspend_no(void);
E boolean genl_can_suspend_yes(void);
E char genl_message_menu(char, int, const char *);
E void genl_preference_update(const char *);
E char *genl_getmsghistory(boolean);
E void genl_putmsghistory(const char *, boolean);
#ifdef HANGUPHANDLING
E void nhwindows_hangup(void);
#endif
#ifdef STATUS_VIA_WINDOWPORT
E void genl_status_init(void);
E void genl_status_finish(void);
E void genl_status_enablefield
             (int, const char *, const char *, boolean);
E void genl_status_update(int, genericptr_t, int, int);
#ifdef STATUS_HILITES
E void genl_status_threshold(int, int, anything, int, int, int);
#endif
#endif

E void dump_open_log(time_t);
E void dump_close_log(void);
E void dump_redirect(boolean);
E void dump_forward_putstr(winid, int, const char*, int);

/* ### wizard.c ### */

E void amulet(void);
E int mon_has_amulet(struct monst *);
E int mon_has_special(struct monst *);
E int tactics(struct monst *);
E boolean has_aggravatables(struct monst *);
E void aggravate(void);
E void clonewiz(void);
E int pick_nasty(void);
E int nasty(struct monst *);
E void resurrect(void);
E void intervene(void);
E void wizdead(void);
E void cuss(struct monst *);

/* ### worm.c ### */

E int get_wormno(void);
E void initworm(struct monst *, int);
E void worm_move(struct monst *);
E void worm_nomove(struct monst *);
E void wormgone(struct monst *);
E void wormhitu(struct monst *);
E void cutworm(struct monst *, xchar, xchar, struct obj *);
E void see_wsegs(struct monst *);
E void detect_wsegs(struct monst *, boolean);
E void save_worm(int, int);
E void rest_worm(int);
E void place_wsegs(struct monst *);
E void sanity_check_worm(struct monst *);
E void remove_worm(struct monst *);
E void place_worm_tail_randomly(struct monst *, xchar, xchar);
E int size_wseg(struct monst *);
E int count_wsegs(struct monst *);
E boolean worm_known(struct monst *);
E boolean worm_cross(int, int, int, int);
E int wseg_at(struct monst *, int, int);

/* ### worn.c ### */

E void setworn(struct obj *, long);
E void setnotworn(struct obj *);
E long wearslot(struct obj *);
E void mon_set_minvis(struct monst *);
E void mon_adjust_speed(struct monst *, int, struct obj *);
E void update_mon_intrinsics(struct monst *, struct obj *, boolean, boolean);
E int find_mac(struct monst *);
E void m_dowear(struct monst *, boolean);
E struct obj *which_armor(struct monst *, long);
E void mon_break_armor(struct monst *, boolean);
E void bypass_obj(struct obj *);
E void clear_bypasses(void);
E void bypass_objlist(struct obj *, boolean);
E struct obj *nxt_unbypassed_obj(struct obj *);
E int racial_exception(struct monst *, struct obj *);

/* ### write.c ### */

E int dowrite(struct obj *);

/* ### zap.c ### */

E void learnwand(struct obj *);
E int bhitm(struct monst *, struct obj *);
E void probe_monster(struct monst *);
E boolean get_obj_location(struct obj *, xchar *, xchar *, int);
E boolean get_mon_location(struct monst *, xchar *, xchar *, int);
E struct monst *get_container_location
                      (struct obj * obj, int *, int *);
E struct monst *montraits(struct obj *, coord *);
E struct monst *revive(struct obj *, boolean);
E int unturn_dead(struct monst *);
E void cancel_item(struct obj *);
E boolean drain_item(struct obj *, boolean);
E struct obj *poly_obj(struct obj *, int);
E boolean obj_resists(struct obj *, int, int);
E boolean obj_shudders(struct obj *);
E void do_osshock(struct obj *);
E int bhito(struct obj *, struct obj *);
E int bhitpile
            (struct obj *, int (*)(OBJ_P, OBJ_P), int, int, schar);
E int zappable(struct obj *);
E void zapnodir(struct obj *);
E int dozap(void);
E int zapyourself(struct obj *, boolean);
E void ubreatheu(struct attack *);
E int lightdamage(struct obj *, boolean, int);
E boolean flashburn(long);
E boolean cancel_monst(struct monst *, struct obj *, boolean,
                               boolean, boolean);
E void zapsetup(void);
E void zapwrapup(void);
E void weffects(struct obj *);
E int spell_damage_bonus(int);
E const char *exclam(int force);
E void hit(const char *, struct monst *, const char *);
E void miss(const char *, struct monst *);
E struct monst *bhit(int, int, int, int, int (*)(MONST_P, OBJ_P),
                             int (*)(OBJ_P, OBJ_P), struct obj **);
E struct monst *boomhit(struct obj *, int, int);
E int zhitm(struct monst *, int, int, struct obj **);
E int burn_floor_objects(int, int, boolean, boolean);
E void buzz(int, int, xchar, xchar, int, int);
E void dobuzz(int, int, xchar, xchar, int, int, boolean);
E void melt_ice(xchar, xchar, const char *);
E void start_melt_ice_timeout(xchar, xchar, long);
E void melt_ice_away(ANY_P *, long);
E int zap_over_floor(xchar, xchar, int, boolean *, short);
E void fracture_rock(struct obj *);
E boolean break_statue(struct obj *);
E void destroy_item(int, int);
E int destroy_mitem(struct monst *, int, int);
E int resist(struct monst *, char, int, int);
E void makewish(void);

#endif /* !MAKEDEFS_C && !LEV_LEX_C */

#undef E

#endif /* EXTERN_H */
