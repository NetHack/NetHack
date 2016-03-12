/* NetHack 3.6	extern.h	$NHDT-Date: 1449051498 2015/12/02 10:18:18 $  $NHDT-Branch: master $:$NHDT-Revision: 1.520 $ */
/* Copyright (c) Steve Creps, 1988.				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef EXTERN_H
#define EXTERN_H

#define E extern

/* ### alloc.c ### */

#if 0
E long *FDECL(alloc, (unsigned int));
#endif
E char *FDECL(fmt_ptr, (const genericptr));

/* This next pre-processor directive covers almost the entire file,
 * interrupted only occasionally to pick up specific functions as needed. */
#if !defined(MAKEDEFS_C) && !defined(LEV_LEX_C)

/* ### allmain.c ### */

E void moveloop(boolean);
E void stop_occupation(void);
E void display_gamewindows(void);
E void newgame(void);
E void FDECL(welcome, (boolean));
E time_t get_realtime(void);

/* ### apply.c ### */

E int doapply(void);
E int dorub(void);
E int dojump(void);
E int FDECL(jump, (int));
E int number_leashed(void);
E void FDECL(o_unleash, (struct obj *));
E void FDECL(m_unleash, (struct monst *, boolean));
E void unleash_all(void);
E boolean next_to_u(void);
E struct obj *FDECL(get_mleash, (struct monst *));
E const char *beautiful(void);
E void FDECL(check_leash, (xchar, xchar));
E boolean FDECL(um_dist, (xchar, xchar, xchar));
E boolean FDECL(snuff_candle, (struct obj *));
E boolean FDECL(snuff_lit, (struct obj *));
E boolean FDECL(catch_lit, (struct obj *));
E void FDECL(use_unicorn_horn, (struct obj *));
E boolean FDECL(tinnable, (struct obj *));
E void reset_trapset(void);
E void FDECL(fig_transform, (ANY_P *, long));
E int FDECL(unfixable_trouble_count, (boolean));

/* ### artifact.c ### */

E void init_artifacts(void);
E void FDECL(save_artifacts, (int));
E void FDECL(restore_artifacts, (int));
E const char *FDECL(artiname, (int));
E struct obj *FDECL(mk_artifact, (struct obj *, ALIGNTYP_P));
E const char *FDECL(artifact_name, (const char *, short *));
E boolean FDECL(exist_artifact, (int, const char *));
E void FDECL(artifact_exists, (struct obj *, const char *, boolean));
E int nartifact_exist(void);
E boolean FDECL(arti_immune, (struct obj *, int));
E boolean FDECL(spec_ability, (struct obj *, unsigned long));
E boolean FDECL(confers_luck, (struct obj *));
E boolean FDECL(arti_reflects, (struct obj *));
E boolean FDECL(shade_glare, (struct obj *));
E boolean FDECL(restrict_name, (struct obj *, const char *));
E boolean FDECL(defends, (int, struct obj *));
E boolean FDECL(defends_when_carried, (int, struct obj *));
E boolean FDECL(protects, (struct obj *, boolean));
E void FDECL(set_artifact_intrinsic, (struct obj *, boolean, long));
E int FDECL(touch_artifact, (struct obj *, struct monst *));
E int FDECL(spec_abon, (struct obj *, struct monst *));
E int FDECL(spec_dbon, (struct obj *, struct monst *, int));
E void FDECL(discover_artifact, (xchar));
E boolean FDECL(undiscovered_artifact, (xchar));
E int FDECL(disp_artifact_discoveries, (winid));
E boolean FDECL(artifact_hit,
                (struct monst *, struct monst *, struct obj *, int *, int));
E int doinvoke(void);
E boolean FDECL(finesse_ahriman, (struct obj *));
E void FDECL(arti_speak, (struct obj *));
E boolean FDECL(artifact_light, (struct obj *));
E long FDECL(spec_m2, (struct obj *));
E boolean FDECL(artifact_has_invprop, (struct obj *, uchar));
E long FDECL(arti_cost, (struct obj *));
E struct obj *FDECL(what_gives, (long *));
E const char *FDECL(glow_color, (int));
E void FDECL(Sting_effects, (int));
E int FDECL(retouch_object, (struct obj **, boolean));
E void FDECL(retouch_equipment, (int));

/* ### attrib.c ### */

E boolean FDECL(adjattrib, (int, int, int));
E void FDECL(gainstr, (struct obj *, int, boolean));
E void FDECL(losestr, (int));
E void FDECL(poisontell, (int, boolean));
E void FDECL(poisoned, (const char *, int, const char *, int, boolean));
E void FDECL(change_luck, (schar));
E int FDECL(stone_luck, (boolean));
E void set_moreluck(void);
E void restore_attrib(void);
E void FDECL(exercise, (int, boolean));
E void exerchk(void);
E void FDECL(init_attr, (int));
E void redist_attr(void);
E void FDECL(adjabil, (int, int));
E int newhp(void);
E schar FDECL(acurr, (int));
E schar acurrstr(void);
E boolean FDECL(extremeattr, (int));
E void FDECL(adjalign, (int));
E int FDECL(is_innate, (int));
E char *FDECL(from_what, (int));
E void FDECL(uchangealign, (int, int));

/* ### ball.c ### */

E void FDECL(ballrelease, (boolean));
E void ballfall(void);
E void placebc(void);
E void unplacebc(void);
E void FDECL(set_bc, (int));
E void FDECL(move_bc, (int, int, xchar, xchar, xchar, xchar));
E boolean FDECL(drag_ball, (xchar, xchar, int *, xchar *, xchar *,
                            xchar *, xchar *, boolean *, boolean));
E void FDECL(drop_ball, (xchar, xchar));
E void drag_down(void);

/* ### bones.c ### */

E void FDECL(sanitize_name, (char *));
E void FDECL(drop_upon_death, (struct monst *, struct obj *, int, int));
E boolean can_make_bones(void);
E void FDECL(savebones, (int, time_t, struct obj *));
E int getbones(void);

/* ### botl.c ### */

E int FDECL(xlev_to_rank, (int));
E int FDECL(title_to_mon, (const char *, int *, int *));
E void max_rank_sz(void);
#ifdef SCORE_ON_BOTL
E long botl_score(void);
#endif
E int FDECL(describe_level, (char *));
E const char *FDECL(rank_of, (int, short, boolean));
E void bot(void);
#ifdef STATUS_VIA_WINDOWPORT
E void FDECL(status_initialize, (boolean));
E void status_finish(void);
E void FDECL(status_notify_windowport, (boolean));
#ifdef STATUS_HILITES
E boolean FDECL(set_status_hilites, (char *op, boolean));
E void FDECL(clear_status_hilites, (boolean));
E char *FDECL(get_status_hilites, (char *, int));
E boolean status_hilite_menu(void);
#endif
#endif

/* ### cmd.c ### */

E boolean FDECL(redraw_cmd, (char));
#ifdef USE_TRAMPOLI
E int doextcmd(void);
E int domonability(void);
E int NDECL(doprev_message);
E int NDECL(timed_occupation);
E int NDECL(doattributes);
E int NDECL(wiz_detect);
E int NDECL(wiz_genesis);
E int NDECL(wiz_identify);
E int NDECL(wiz_level_tele);
E int NDECL(wiz_map);
E int NDECL(wiz_where);
E int NDECL(wiz_wish);
#endif /* USE_TRAMPOLI */
E void reset_occupations(void);
E void FDECL(set_occupation, (int (*)(void), const char *, int));
E char pgetchar(void);
E void FDECL(pushch, (char));
E void FDECL(savech, (char));
E void add_debug_extended_commands(void);
E void FDECL(reset_commands, (boolean));
E void FDECL(rhack, (char *));
E int doextlist(void);
E int extcmd_via_menu(void);
E int enter_explore_mode(void);
E void FDECL(enlightenment, (int, int));
E void FDECL(youhiding, (boolean, int));
E void FDECL(show_conduct, (int));
E int FDECL(xytod, (schar, schar));
E void FDECL(dtoxy, (coord *, int));
E int FDECL(movecmd, (char));
E int dxdy_moveok(void);
E int FDECL(getdir, (const char *));
E void confdir(void);
E const char *FDECL(directionname, (int));
E int FDECL(isok, (int, int));
E int FDECL(get_adjacent_loc,
            (const char *, const char *, xchar, xchar, coord *));
E const char *FDECL(click_to_cmd, (int, int, int));
#ifdef HANGUPHANDLING
E void FDECL(hangup, (int));
E void end_of_input(void);
#endif
E char readchar(void);
E void sanity_check(void);
E char FDECL(yn_function, (const char *, const char *, char));
E boolean FDECL(paranoid_query, (boolean, const char *));

/* ### dbridge.c ### */

E boolean FDECL(is_pool, (int, int));
E boolean FDECL(is_lava, (int, int));
E boolean FDECL(is_pool_or_lava, (int, int));
E boolean FDECL(is_ice, (int, int));
E boolean FDECL(is_moat, (int, int));
E schar FDECL(db_under_typ, (int));
E int FDECL(is_drawbridge_wall, (int, int));
E boolean FDECL(is_db_wall, (int, int));
E boolean FDECL(find_drawbridge, (int *, int *));
E boolean FDECL(create_drawbridge, (int, int, int, boolean));
E void FDECL(open_drawbridge, (int, int));
E void FDECL(close_drawbridge, (int, int));
E void FDECL(destroy_drawbridge, (int, int));

/* ### decl.c ### */

E void decl_init(void);

/* ### detect.c ### */

E struct obj *FDECL(o_in, (struct obj *, char));
E struct obj *FDECL(o_material, (struct obj *, unsigned));
E int FDECL(gold_detect, (struct obj *));
E int FDECL(food_detect, (struct obj *));
E int FDECL(object_detect, (struct obj *, int));
E int FDECL(monster_detect, (struct obj *, int));
E int FDECL(trap_detect, (struct obj *));
E const char *FDECL(level_distance, (d_level *));
E void FDECL(use_crystal_ball, (struct obj **));
E void do_mapping(void);
E void do_vicinity_map(void);
E void FDECL(cvt_sdoor_to_door, (struct rm *));
#ifdef USE_TRAMPOLI
E void FDECL(findone, (int, int, genericptr_t));
E void FDECL(openone, (int, int, genericptr_t));
#endif
E int findit(void);
E int openit(void);
E boolean FDECL(detecting, (void (*)(int, int, genericptr)));
E void FDECL(find_trap, (struct trap *));
E int FDECL(dosearch0, (int));
E int dosearch(void);
E void sokoban_detect(void);
E void FDECL(reveal_terrain, (int, int));

/* ### dig.c ### */

E int FDECL(dig_typ, (struct obj *, xchar, xchar));
E boolean is_digging(void);
#ifdef USE_TRAMPOLI
E int dig(void);
#endif
E int holetime(void);
E boolean FDECL(dig_check, (struct monst *, boolean, int, int));
E void FDECL(digactualhole, (int, int, struct monst *, int));
E boolean FDECL(dighole, (boolean, boolean, coord *));
E int FDECL(use_pick_axe, (struct obj *));
E int FDECL(use_pick_axe2, (struct obj *));
E boolean FDECL(mdig_tunnel, (struct monst *));
E void FDECL(draft_message, (boolean));
E void FDECL(watch_dig, (struct monst *, xchar, xchar, boolean));
E void zap_dig(void);
E struct obj *FDECL(bury_an_obj, (struct obj *, boolean *));
E void FDECL(bury_objs, (int, int));
E void FDECL(unearth_objs, (int, int));
E void FDECL(rot_organic, (ANY_P *, long));
E void FDECL(rot_corpse, (ANY_P *, long));
E struct obj *FDECL(buried_ball, (coord *));
E void buried_ball_to_punishment(void);
E void buried_ball_to_freedom(void);
E schar FDECL(fillholetyp, (int, int, boolean));
E void FDECL(liquid_flow,
             (xchar, xchar, schar, struct trap *, const char *));
E boolean FDECL(conjoined_pits, (struct trap *, struct trap *, boolean));
#if 0
E void FDECL(bury_monst, (struct monst *));
E void bury_you(void);
E void unearth_you(void);
E void escape_tomb(void);
E void FDECL(bury_obj, (struct obj *));
#endif

/* ### display.c ### */

E void FDECL(magic_map_background, (xchar, xchar, int));
E void FDECL(map_background, (xchar, xchar, int));
E void FDECL(map_trap, (struct trap *, int));
E void FDECL(map_object, (struct obj *, int));
E void FDECL(map_invisible, (xchar, xchar));
E void FDECL(unmap_object, (int, int));
E void FDECL(map_location, (int, int, int));
E void FDECL(feel_newsym, (xchar, xchar));
E void FDECL(feel_location, (xchar, xchar));
E void FDECL(newsym, (int, int));
E void FDECL(shieldeff, (xchar, xchar));
E void FDECL(tmp_at, (int, int));
E void FDECL(swallowed, (int));
E void FDECL(under_ground, (int));
E void FDECL(under_water, (int));
E void see_monsters(void);
E void set_mimic_blocking(void);
E void see_objects(void);
E void see_traps(void);
E void curs_on_u(void);
E int doredraw(void);
E void docrt(void);
E void FDECL(show_glyph, (int, int, int));
E void clear_glyph_buffer(void);
E void FDECL(row_refresh, (int, int, int));
E void cls(void);
E void FDECL(flush_screen, (int));
E int FDECL(back_to_glyph, (xchar, xchar));
E int FDECL(zapdir_to_glyph, (int, int, int));
E int FDECL(glyph_at, (xchar, xchar));
E void set_wall_state(void);
E void FDECL(unset_seenv, (struct rm *, int, int, int, int));

/* ### do.c ### */

#ifdef USE_TRAMPOLI
E int FDECL(drop, (struct obj *));
E int wipeoff(void);
#endif
E int dodrop(void);
E boolean FDECL(boulder_hits_pool, (struct obj *, int, int, boolean));
E boolean FDECL(flooreffects, (struct obj *, int, int, const char *));
E void FDECL(doaltarobj, (struct obj *));
E boolean FDECL(canletgo, (struct obj *, const char *));
E void FDECL(dropx, (struct obj *));
E void FDECL(dropy, (struct obj *));
E void FDECL(dropz, (struct obj *, boolean));
E void FDECL(obj_no_longer_held, (struct obj *));
E int doddrop(void);
E int dodown(void);
E int doup(void);
#ifdef INSURANCE
E void save_currentstate(void);
#endif
E void FDECL(goto_level, (d_level *, boolean, boolean, boolean));
E void FDECL(schedule_goto, (d_level *, boolean, boolean, int,
                             const char *, const char *));
E void deferred_goto(void);
E boolean FDECL(revive_corpse, (struct obj *));
E void FDECL(revive_mon, (ANY_P *, long));
E int donull(void);
E int dowipe(void);
E void FDECL(set_wounded_legs, (long, int));
E void heal_legs(void);

/* ### do_name.c ### */

E int FDECL(getpos, (coord *, boolean, const char *));
E void FDECL(getpos_sethilite, (void (*f)(int)));
E void FDECL(new_mname, (struct monst *, int));
E void FDECL(free_mname, (struct monst *));
E void FDECL(new_oname, (struct obj *, int));
E void FDECL(free_oname, (struct obj *));
E const char *FDECL(safe_oname, (struct obj *));
E struct monst *FDECL(christen_monst, (struct monst *, const char *));
E struct obj *FDECL(oname, (struct obj *, const char *));
E boolean FDECL(objtyp_is_callable, (int));
E int docallcmd(void);
E void FDECL(docall, (struct obj *));
E const char *rndghostname(void);
E char *FDECL(x_monnam, (struct monst *, int, const char *, int, boolean));
E char *FDECL(l_monnam, (struct monst *));
E char *FDECL(mon_nam, (struct monst *));
E char *FDECL(noit_mon_nam, (struct monst *));
E char *FDECL(Monnam, (struct monst *));
E char *FDECL(noit_Monnam, (struct monst *));
E char *FDECL(m_monnam, (struct monst *));
E char *FDECL(y_monnam, (struct monst *));
E char *FDECL(Adjmonnam, (struct monst *, const char *));
E char *FDECL(Amonnam, (struct monst *));
E char *FDECL(a_monnam, (struct monst *));
E char *FDECL(distant_monnam, (struct monst *, int, char *));
E char *FDECL(rndmonnam, (char *));
E const char *FDECL(hcolor, (const char *));
E const char *rndcolor(void);
E const char *roguename(void);
E struct obj *FDECL(realloc_obj,
                    (struct obj *, int, genericptr_t, int, const char *));
E char *FDECL(coyotename, (struct monst *, char *));
E const char *FDECL(noveltitle, (int *));
E const char *FDECL(lookup_novel, (const char *, int *));

/* ### do_wear.c ### */

#ifdef USE_TRAMPOLI
E int Armor_on(void);
E int Boots_on(void);
E int Gloves_on(void);
E int Helmet_on(void);
E int FDECL(select_off, (struct obj *));
E int take_off(void);
#endif
E void FDECL(off_msg, (struct obj *));
E void FDECL(set_wear, (struct obj *));
E boolean FDECL(donning, (struct obj *));
E boolean FDECL(doffing, (struct obj *));
E void cancel_don(void);
E int FDECL(stop_donning, (struct obj *));
E int Armor_off(void);
E int Armor_gone(void);
E int Helmet_off(void);
E int Gloves_off(void);
E int Boots_off(void);
E int Cloak_off(void);
E int Shield_off(void);
E int Shirt_off(void);
E void Amulet_off(void);
E void FDECL(Ring_on, (struct obj *));
E void FDECL(Ring_off, (struct obj *));
E void FDECL(Ring_gone, (struct obj *));
E void FDECL(Blindf_on, (struct obj *));
E void FDECL(Blindf_off, (struct obj *));
E int dotakeoff(void);
E int doremring(void);
E int FDECL(cursed, (struct obj *));
E int FDECL(armoroff, (struct obj *));
E int FDECL(canwearobj, (struct obj *, long *, boolean));
E int dowear(void);
E int doputon(void);
E void find_ac(void);
E void glibr(void);
E struct obj *FDECL(some_armor, (struct monst *));
E struct obj *FDECL(stuck_ring, (struct obj *, int));
E struct obj *unchanger(void);
E void reset_remarm(void);
E int doddoremarm(void);
E int destroy_arm(struct obj *);
E void adj_abon(struct obj *, schar);
E boolean
inaccessible_equipment(struct obj *, const char *, boolean);

/* ### dog.c ### */

E void FDECL(newedog, (struct monst *));
E void FDECL(free_edog, (struct monst *));
E void FDECL(initedog, (struct monst *));
E struct monst *FDECL(make_familiar,
                      (struct obj *, xchar, xchar, boolean));
E struct monst *makedog(void);
E void update_mlstmv(void);
E void losedogs(void);
E void FDECL(mon_arrive, (struct monst *, boolean));
E void FDECL(mon_catchup_elapsed_time, (struct monst *, long));
E void FDECL(keepdogs, (boolean));
E void FDECL(migrate_to_level, (struct monst *, xchar, xchar, coord *));
E int FDECL(dogfood, (struct monst *, struct obj *));
E boolean FDECL(tamedog, (struct monst *, struct obj *));
E void FDECL(abuse_dog, (struct monst *));
E void FDECL(wary_dog, (struct monst *, boolean));

/* ### dogmove.c ### */

E struct obj *FDECL(droppables, (struct monst *));
E int FDECL(dog_nutrition, (struct monst *, struct obj *));
E int FDECL(dog_eat, (struct monst *, struct obj *, int, int, boolean));
E int FDECL(dog_move, (struct monst *, int));
#ifdef USE_TRAMPOLI
E void FDECL(wantdoor, (int, int, genericptr_t));
#endif
E void FDECL(finish_meating, (struct monst *));

/* ### dokick.c ### */

E boolean FDECL(ghitm, (struct monst *, struct obj *));
E void FDECL(container_impact_dmg, (struct obj *, xchar, xchar));
E int dokick(void);
E boolean FDECL(ship_object, (struct obj *, xchar, xchar, boolean));
E void FDECL(obj_delivery, (boolean));
E schar FDECL(down_gate, (xchar, xchar));
E void FDECL(impact_drop, (struct obj *, xchar, xchar, xchar));

/* ### dothrow.c ### */

E int dothrow(void);
E int dofire(void);
E void FDECL(endmultishot, (boolean));
E void FDECL(hitfloor, (struct obj *));
E void FDECL(hurtle, (int, int, int, boolean));
E void FDECL(mhurtle, (struct monst *, int, int, int));
E void FDECL(throwit, (struct obj *, long, boolean));
E int FDECL(omon_adj, (struct monst *, struct obj *, boolean));
E int FDECL(thitmonst, (struct monst *, struct obj *));
E int FDECL(hero_breaks, (struct obj *, xchar, xchar, boolean));
E int FDECL(breaks, (struct obj *, xchar, xchar));
E void FDECL(release_camera_demon, (struct obj *, xchar, xchar));
E void FDECL(breakobj,
             (struct obj *, xchar, xchar, boolean, boolean));
E boolean FDECL(breaktest, (struct obj *));
E boolean FDECL(walk_path, (coord *, coord *,
                            boolean (*)(genericptr, int, int), genericptr_t));
E boolean FDECL(hurtle_step, (genericptr_t, int, int));

/* ### drawing.c ### */
#endif /* !MAKEDEFS_C && !LEV_LEX_C */
E int FDECL(def_char_to_objclass, (char));
E int FDECL(def_char_to_monclass, (char));
#if !defined(MAKEDEFS_C) && !defined(LEV_LEX_C)
E void FDECL(switch_symbols, (int));
E void FDECL(assign_graphics, (int));
E void init_r_symbols(void);
E void init_symbols(void);
E void update_bouldersym(void);
E void init_showsyms(void);
E void init_l_symbols(void);
E void FDECL(clear_symsetentry, (int, boolean));
E void FDECL(update_l_symset, (struct symparse *, int));
E void FDECL(update_r_symset, (struct symparse *, int));
E boolean FDECL(cursed_object_at, (int, int));

/* ### dungeon.c ### */

E void FDECL(save_dungeon, (int, boolean, boolean));
E void FDECL(restore_dungeon, (int));
E void FDECL(insert_branch, (branch *, boolean));
E void init_dungeons(void);
E s_level *FDECL(find_level, (const char *));
E s_level *FDECL(Is_special, (d_level *));
E branch *FDECL(Is_branchlev, (d_level *));
E boolean FDECL(builds_up, (d_level *));
E xchar FDECL(ledger_no, (d_level *));
E xchar maxledgerno(void);
E schar FDECL(depth, (d_level *));
E xchar FDECL(dunlev, (d_level *));
E xchar FDECL(dunlevs_in_dungeon, (d_level *));
E xchar FDECL(ledger_to_dnum, (xchar));
E xchar FDECL(ledger_to_dlev, (xchar));
E xchar FDECL(deepest_lev_reached, (boolean));
E boolean FDECL(on_level, (d_level *, d_level *));
E void FDECL(next_level, (boolean));
E void FDECL(prev_level, (boolean));
E void FDECL(u_on_newpos, (int, int));
E void FDECL(u_on_rndspot, (int));
E void FDECL(u_on_sstairs, (int));
E void u_on_upstairs(void);
E void u_on_dnstairs(void);
E boolean FDECL(On_stairs, (xchar, xchar));
E void FDECL(get_level, (d_level *, int));
E boolean FDECL(Is_botlevel, (d_level *));
E boolean FDECL(Can_fall_thru, (d_level *));
E boolean FDECL(Can_dig_down, (d_level *));
E boolean FDECL(Can_rise_up, (int, int, d_level *));
E boolean FDECL(has_ceiling, (d_level *));
E boolean FDECL(In_quest, (d_level *));
E boolean FDECL(In_mines, (d_level *));
E branch *FDECL(dungeon_branch, (const char *));
E boolean FDECL(at_dgn_entrance, (const char *));
E boolean FDECL(In_hell, (d_level *));
E boolean FDECL(In_V_tower, (d_level *));
E boolean FDECL(On_W_tower_level, (d_level *));
E boolean FDECL(In_W_tower, (int, int, d_level *));
E void FDECL(find_hell, (d_level *));
E void FDECL(goto_hell, (boolean, boolean));
E void FDECL(assign_level, (d_level *, d_level *));
E void FDECL(assign_rnd_level, (d_level *, d_level *, int));
E int FDECL(induced_align, (int));
E boolean FDECL(Invocation_lev, (d_level *));
E xchar level_difficulty(void);
E schar FDECL(lev_by_name, (const char *));
E schar FDECL(print_dungeon, (boolean, schar *, xchar *));
E char *FDECL(get_annotation, (d_level *));
E int donamelevel(void);
E int dooverview(void);
E void FDECL(show_overview, (int, int));
E void FDECL(forget_mapseen, (int));
E void FDECL(init_mapseen, (d_level *));
E void recalc_mapseen(void);
E void FDECL(mapseen_temple, (struct monst *));
E void FDECL(room_discovered, (int));
E void FDECL(recbranch_mapseen, (d_level *, d_level *));
E void FDECL(remdun_mapseen, (int));

/* ### eat.c ### */

#ifdef USE_TRAMPOLI
E int eatmdone(void);
E int eatfood(void);
E int opentin(void);
E int unfaint(void);
#endif
E void eatmupdate(void);
E boolean FDECL(is_edible, (struct obj *));
E void init_uhunger(void);
E int Hear_again(void);
E void reset_eat(void);
E int doeat(void);
E void gethungry(void);
E void FDECL(morehungry, (int));
E void FDECL(lesshungry, (int));
E boolean is_fainted(void);
E void reset_faint(void);
E void violated_vegetarian(void);
E void FDECL(newuhs, (boolean));
E struct obj *FDECL(floorfood, (const char *, int));
E void vomit(void);
E int FDECL(eaten_stat, (int, struct obj *));
E void FDECL(food_disappears, (struct obj *));
E void FDECL(food_substitution, (struct obj *, struct obj *));
E void FDECL(eating_conducts, (struct permonst *));
E int FDECL(eat_brains, (struct monst *, struct monst *, boolean, int *));
E void fix_petrification(void);
E void FDECL(consume_oeaten, (struct obj *, int));
E boolean FDECL(maybe_finished_meal, (boolean));
E void FDECL(set_tin_variety, (struct obj *, int));
E int FDECL(tin_variety_txt, (char *, int *));
E void FDECL(tin_details, (struct obj *, int, char *));
E boolean FDECL(Popeye, (int));

/* ### end.c ### */

E void FDECL(done1, (int));
E int done2(void);
#ifdef USE_TRAMPOLI
E void FDECL(done_intr, (int));
#endif
E void FDECL(done_in_by, (struct monst *, int));
#endif /* !MAKEDEFS_C && !LEV_LEX_C */
E void VDECL(panic, (const char *, ...)) PRINTF_F(1, 2) NORETURN;
#if !defined(MAKEDEFS_C) && !defined(LEV_LEX_C)
E void FDECL(done, (int));
E void FDECL(container_contents, (struct obj *, boolean,
                                  boolean, boolean));
E void FDECL(terminate, (int)) NORETURN;
E int dovanquished(void);
E int num_genocides(void);
E void FDECL(delayed_killer, (int, int, const char *));
E struct kinfo *FDECL(find_delayed_killer, (int));
E void FDECL(dealloc_killer, (struct kinfo *));
E void FDECL(save_killers, (int, int));
E void FDECL(restore_killers, (int));
E char *FDECL(build_english_list, (char *));
#if defined(PANICTRACE) && !defined(NO_SIGNAL)
E void FDECL(panictrace_setsignals, (boolean));
#endif

/* ### engrave.c ### */

E char *FDECL(random_engraving, (char *));
E void FDECL(wipeout_text, (char *, int, unsigned));
E boolean FDECL(can_reach_floor, (boolean));
E void FDECL(cant_reach_floor, (int, int, boolean, boolean));
E const char *FDECL(surface, (int, int));
E const char *FDECL(ceiling, (int, int));
E struct engr *FDECL(engr_at, (xchar, xchar));
E int FDECL(sengr_at, (const char *, xchar, xchar, boolean));
E void FDECL(u_wipe_engr, (int));
E void FDECL(wipe_engr_at, (xchar, xchar, xchar, boolean));
E void FDECL(read_engr_at, (int, int));
E void FDECL(make_engr_at, (int, int, const char *, long, xchar));
E void FDECL(del_engr_at, (int, int));
E int freehand(void);
E int doengrave(void);
E void sanitize_engravings(void);
E void FDECL(save_engravings, (int, int));
E void FDECL(rest_engravings, (int));
E void FDECL(del_engr, (struct engr *));
E void FDECL(rloc_engr, (struct engr *));
E void FDECL(make_grave, (int, int, const char *));

/* ### exper.c ### */

E int newpw(void);
E int FDECL(experience, (struct monst *, int));
E void FDECL(more_experienced, (int, int));
E void FDECL(losexp, (const char *));
E void newexplevel(void);
E void FDECL(pluslvl, (boolean));
E long FDECL(rndexp, (boolean));

/* ### explode.c ### */

E void FDECL(explode, (int, int, int, int, char, int));
E long FDECL(scatter, (int, int, int, unsigned int, struct obj *));
E void FDECL(splatter_burning_oil, (int, int));
E void FDECL(explode_oil, (struct obj *, int, int));

/* ### extralev.c ### */

E void makeroguerooms(void);
E void corr(int, int);
E void makerogueghost(void);

/* ### files.c ### */

E char *FDECL(fname_encode, (const char *, char, char *, char *, int));
E char *FDECL(fname_decode, (char, char *, char *, int));
E const char *FDECL(fqname, (const char *, int, int));
E FILE *FDECL(fopen_datafile, (const char *, const char *, int));
#ifdef MFLOPPY
E void set_lock_and_bones(void);
#endif
E void FDECL(set_levelfile_name, (char *, int));
E int FDECL(create_levelfile, (int, char *));
E int FDECL(open_levelfile, (int, char *));
E void FDECL(delete_levelfile, (int));
E void clearlocks(void);
E int FDECL(create_bonesfile, (d_level *, char **, char *));
#ifdef MFLOPPY
E void cancel_bonesfile(void);
#endif
E void FDECL(commit_bonesfile, (d_level *));
E int FDECL(open_bonesfile, (d_level *, char **));
E int FDECL(delete_bonesfile, (d_level *));
E void compress_bonesfile(void);
E void FDECL(set_savefile_name, (boolean));
#ifdef INSURANCE
E void FDECL(save_savefile_name, (int));
#endif
#ifndef MICRO
E void set_error_savefile(void);
#endif
E int create_savefile(void);
E int open_savefile(void);
E int delete_savefile(void);
E int restore_saved_game(void);
E void FDECL(nh_compress, (const char *));
E void FDECL(nh_uncompress, (const char *));
E boolean FDECL(lock_file, (const char *, int, int));
E void FDECL(unlock_file, (const char *));
#ifdef USER_SOUNDS
E boolean FDECL(can_read_file, (const char *));
#endif
E boolean FDECL(read_config_file, (const char *, int));
E void FDECL(check_recordfile, (const char *));
E void read_wizkit(void);
E int FDECL(read_sym_file, (int));
E int FDECL(parse_sym_line, (char *, int));
E void FDECL(paniclog, (const char *, const char *));
E int FDECL(validate_prefix_locations, (char *));
#ifdef SELECTSAVED
E char *FDECL(plname_from_file, (const char *));
#endif
E char **NDECL(get_saved_games);
E void FDECL(free_saved_games, (char **));
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
E boolean FDECL(debugcore, (const char *, boolean));
#endif
E boolean FDECL(read_tribute, (const char *, const char *, int,
                               char *, int, unsigned));
E boolean FDECL(Death_quote, (char *, int));

/* ### fountain.c ### */

E void FDECL(floating_above, (const char *));
E void FDECL(dogushforth, (int));
#ifdef USE_TRAMPOLI
E void FDECL(gush, (int, int, genericptr_t));
#endif
E void FDECL(dryup, (xchar, xchar, boolean));
E void drinkfountain(void);
E void FDECL(dipfountain, (struct obj *));
E void FDECL(breaksink, (int, int));
E void drinksink(void);

/* ### hack.c ### */

E anything *FDECL(uint_to_any, (unsigned));
E anything *FDECL(long_to_any, (long));
E anything *FDECL(monst_to_any, (struct monst *));
E anything *FDECL(obj_to_any, (struct obj *));
E boolean FDECL(revive_nasty, (int, int, const char *));
E void FDECL(movobj, (struct obj *, xchar, xchar));
E boolean FDECL(may_dig, (xchar, xchar));
E boolean FDECL(may_passwall, (xchar, xchar));
E boolean FDECL(bad_rock, (struct permonst *, xchar, xchar));
E int FDECL(cant_squeeze_thru, (struct monst *));
E boolean FDECL(invocation_pos, (xchar, xchar));
E boolean FDECL(test_move, (int, int, int, int, int));
#ifdef DEBUG
E int wiz_debug_cmd_traveldisplay(void);
#endif
E boolean NDECL(u_rooted);
E void NDECL(domove);
E boolean NDECL(overexertion);
E void NDECL(invocation_message);
E boolean FDECL(pooleffects, (boolean));
E void FDECL(spoteffects, (boolean));
E char *FDECL(in_rooms, (xchar, xchar, int));
E boolean FDECL(in_town, (int, int));
E void FDECL(check_special_room, (boolean));
E int NDECL(dopickup);
E void NDECL(lookaround);
E boolean FDECL(crawl_destination, (int, int));
E int NDECL(monster_nearby);
E void FDECL(nomul, (int));
E void FDECL(unmul, (const char *));
E void FDECL(losehp, (int, const char *, boolean));
E int NDECL(weight_cap);
E int NDECL(inv_weight);
E int NDECL(near_capacity);
E int FDECL(calc_capacity, (int));
E int NDECL(max_capacity);
E boolean FDECL(check_capacity, (const char *));
E int FDECL(inv_cnt, (boolean));
E long FDECL(money_cnt, (struct obj *));

/* ### hacklib.c ### */

E boolean FDECL(digit, (char));
E boolean FDECL(letter, (char));
E char FDECL(highc, (char));
E char FDECL(lowc, (char));
E char *FDECL(lcase, (char *));
E char *FDECL(ucase, (char *));
E char *FDECL(upstart, (char *));
E char *FDECL(mungspaces, (char *));
E char *FDECL(eos, (char *));
E boolean FDECL(str_end_is, (const char *, const char *));
E char *FDECL(strkitten, (char *, char));
E void FDECL(copynchars, (char *, const char *, int));
E char FDECL(chrcasecpy, (int, int));
E char *FDECL(strcasecpy, (char *, const char *));
E char *FDECL(s_suffix, (const char *));
E char *FDECL(ing_suffix, (const char *));
E char *FDECL(xcrypt, (const char *, char *));
E boolean FDECL(onlyspace, (const char *));
E char *FDECL(tabexpand, (char *));
E char *FDECL(visctrl, (char));
E char *FDECL(strsubst, (char *, const char *, const char *));
E const char *FDECL(ordin, (int));
E char *FDECL(sitoa, (int));
E int FDECL(sgn, (int));
E int FDECL(rounddiv, (long, int));
E int FDECL(dist2, (int, int, int, int));
E int FDECL(isqrt, (int));
E int FDECL(distmin, (int, int, int, int));
E boolean FDECL(online2, (int, int, int, int));
E boolean FDECL(pmatch, (const char *, const char *));
E boolean FDECL(pmatchi, (const char *, const char *));
E boolean FDECL(pmatchz, (const char *, const char *));
#ifndef STRNCMPI
E int FDECL(strncmpi, (const char *, const char *, int));
#endif
#ifndef STRSTRI
E char *FDECL(strstri, (const char *, const char *));
#endif
E boolean
FDECL(fuzzymatch, (const char *, const char *, const char *, boolean));
E void NDECL(setrandom);
E time_t NDECL(getnow);
E int NDECL(getyear);
#if 0
E char *FDECL(yymmdd, (time_t));
#endif
E long FDECL(yyyymmdd, (time_t));
E long FDECL(hhmmss, (time_t));
E char *FDECL(yyyymmddhhmmss, (time_t));
E time_t FDECL(time_from_yyyymmddhhmmss, (char *));
E int NDECL(phase_of_the_moon);
E boolean NDECL(friday_13th);
E int NDECL(night);
E int NDECL(midnight);

/* ### invent.c ### */

E struct obj **FDECL(objarr_init, (int));
E void FDECL(objarr_set, (struct obj *, int, struct obj **, boolean));
E void FDECL(assigninvlet, (struct obj *));
E struct obj *FDECL(merge_choice, (struct obj *, struct obj *));
E int FDECL(merged, (struct obj **, struct obj **));
#ifdef USE_TRAMPOLI
E int FDECL(ckunpaid, (struct obj *));
#endif
E void FDECL(addinv_core1, (struct obj *));
E void FDECL(addinv_core2, (struct obj *));
E struct obj *FDECL(addinv, (struct obj *));
E struct obj *FDECL(hold_another_object,
                    (struct obj *, const char *, const char *, const char *));
E void FDECL(useupall, (struct obj *));
E void FDECL(useup, (struct obj *));
E void FDECL(consume_obj_charge, (struct obj *, boolean));
E void FDECL(freeinv_core, (struct obj *));
E void FDECL(freeinv, (struct obj *));
E void FDECL(delallobj, (int, int));
E void FDECL(delobj, (struct obj *));
E struct obj *FDECL(sobj_at, (int, int, int));
E struct obj *FDECL(nxtobj, (struct obj *, int, boolean));
E struct obj *FDECL(carrying, (int));
E boolean NDECL(have_lizard);
E struct obj *NDECL(u_have_novel);
E struct obj *FDECL(o_on, (unsigned int, struct obj *));
E boolean FDECL(obj_here, (struct obj *, int, int));
E boolean NDECL(wearing_armor);
E boolean FDECL(is_worn, (struct obj *));
E struct obj *FDECL(g_at, (int, int));
E struct obj *FDECL(getobj, (const char *, const char *));
E int FDECL(ggetobj, (const char *, int (*)(OBJ_P), int,
                      boolean, unsigned *));
E int FDECL(askchain, (struct obj **, const char *, int, int (*)(OBJ_P),
                       int (*)(OBJ_P), int, const char *));
E void FDECL(fully_identify_obj, (struct obj *));
E int FDECL(identify, (struct obj *));
E void FDECL(identify_pack, (int, boolean));
E void NDECL(learn_unseen_invent);
E void FDECL(prinv, (const char *, struct obj *, long));
E char *FDECL(xprname,
              (struct obj *, const char *, char, boolean, long, long));
E int NDECL(ddoinv);
E char FDECL(display_inventory, (const char *, boolean));
E int FDECL(display_binventory, (int, int, boolean));
E struct obj *FDECL(display_cinventory, (struct obj *));
E struct obj *FDECL(display_minventory, (struct monst *, int, char *));
E int NDECL(dotypeinv);
E const char *FDECL(dfeature_at, (int, int, char *));
E int FDECL(look_here, (int, boolean));
E int NDECL(dolook);
E boolean FDECL(will_feel_cockatrice, (struct obj *, boolean));
E void FDECL(feel_cockatrice, (struct obj *, boolean));
E void FDECL(stackobj, (struct obj *));
E int NDECL(doprgold);
E int NDECL(doprwep);
E int NDECL(doprarm);
E int NDECL(doprring);
E int NDECL(dopramulet);
E int NDECL(doprtool);
E int NDECL(doprinuse);
E void FDECL(useupf, (struct obj *, long));
E char *FDECL(let_to_name, (char, boolean, boolean));
E void NDECL(free_invbuf);
E void NDECL(reassign);
E int NDECL(doorganize);
E void NDECL(free_pickinv_cache);
E int FDECL(count_unpaid, (struct obj *));
E int FDECL(count_buc, (struct obj *, int));
E long FDECL(count_contents, (struct obj *, boolean, boolean, boolean));
E void FDECL(carry_obj_effects, (struct obj *));
E const char *FDECL(currency, (long));
E void FDECL(silly_thing, (const char *, struct obj *));

/* ### ioctl.c ### */

#if defined(UNIX) || defined(__BEOS__)
E void NDECL(getwindowsz);
E void NDECL(getioctls);
E void NDECL(setioctls);
#ifdef SUSPEND
E int NDECL(dosuspend);
#endif /* SUSPEND */
#endif /* UNIX || __BEOS__ */

/* ### light.c ### */

E void FDECL(new_light_source, (xchar, xchar, int, int, ANY_P *));
E void FDECL(del_light_source, (int, ANY_P *));
E void FDECL(do_light_sources, (char **));
E struct monst *FDECL(find_mid, (unsigned, unsigned));
E void FDECL(save_light_sources, (int, int, int));
E void FDECL(restore_light_sources, (int));
E void FDECL(relink_light_sources, (boolean));
E void NDECL(light_sources_sanity_check);
E void FDECL(obj_move_light_source, (struct obj *, struct obj *));
E boolean NDECL(any_light_source);
E void FDECL(snuff_light_source, (int, int));
E boolean FDECL(obj_sheds_light, (struct obj *));
E boolean FDECL(obj_is_burning, (struct obj *));
E void FDECL(obj_split_light_source, (struct obj *, struct obj *));
E void FDECL(obj_merge_light_sources, (struct obj *, struct obj *));
E void FDECL(obj_adjust_light_radius, (struct obj *, int));
E int FDECL(candle_light_range, (struct obj *));
E int FDECL(arti_light_radius, (struct obj *));
E const char *FDECL(arti_light_description, (struct obj *));
E int NDECL(wiz_light_sources);

/* ### lock.c ### */

#ifdef USE_TRAMPOLI
E int NDECL(forcelock);
E int NDECL(picklock);
#endif
E boolean FDECL(picking_lock, (int *, int *));
E boolean FDECL(picking_at, (int, int));
E void FDECL(breakchestlock, (struct obj *, boolean));
E void NDECL(reset_pick);
E int FDECL(pick_lock, (struct obj *));
E int NDECL(doforce);
E boolean FDECL(boxlock, (struct obj *, struct obj *));
E boolean FDECL(doorlock, (struct obj *, int, int));
E int NDECL(doopen);
E boolean FDECL(stumble_on_door_mimic, (int, int));
E int FDECL(doopen_indir, (int, int));
E int NDECL(doclose);

#ifdef MAC
/* These declarations are here because the main code calls them. */

/* ### macfile.c ### */

E int FDECL(maccreat, (const char *, long));
E int FDECL(macopen, (const char *, int, long));
E int FDECL(macclose, (int));
E int FDECL(macread, (int, void *, unsigned));
E int FDECL(macwrite, (int, void *, unsigned));
E long FDECL(macseek, (int, long, short));
E int FDECL(macunlink, (const char *));

/* ### macmain.c ### */

E boolean NDECL(authorize_wizard_mode);

/* ### macsnd.c ### */

E void FDECL(mac_speaker, (struct obj *, char *));

/* ### macunix.c ### */

E void FDECL(regularize, (char *));
E void NDECL(getlock);

/* ### macwin.c ### */

E void FDECL(lock_mouse_cursor, (Boolean));
E int NDECL(SanePositions);

/* ### mttymain.c ### */

E void FDECL(getreturn, (char *));
E void VDECL(msmsg, (const char *, ...));
E void NDECL(gettty);
E void NDECL(setftty);
E void FDECL(settty, (const char *));
E int NDECL(tgetch);
E void FDECL(cmov, (int x, int y));
E void FDECL(nocmov, (int x, int y));

#endif /* MAC */

/* ### mail.c ### */

#ifdef MAIL
#ifdef UNIX
E void NDECL(getmailstatus);
#endif
E void NDECL(ckmailstatus);
E void FDECL(readmail, (struct obj *));
#endif /* MAIL */

/* ### makemon.c ### */

E void FDECL(dealloc_monst, (struct monst *));
E boolean FDECL(is_home_elemental, (struct permonst *));
E struct monst *FDECL(clone_mon, (struct monst *, xchar, xchar));
E int FDECL(monhp_per_lvl, (struct monst *));
E void FDECL(newmonhp, (struct monst *, int));
E struct mextra *NDECL(newmextra);
E void FDECL(copy_mextra, (struct monst *, struct monst *));
E void FDECL(dealloc_mextra, (struct monst *));
E struct monst *FDECL(makemon, (struct permonst *, int, int, int));
E boolean FDECL(create_critters, (int, struct permonst *, boolean));
E struct permonst *NDECL(rndmonst);
E void FDECL(reset_rndmonst, (int));
E struct permonst *FDECL(mkclass, (char, int));
E int FDECL(mkclass_poly, (int));
E int FDECL(adj_lev, (struct permonst *));
E struct permonst *FDECL(grow_up, (struct monst *, struct monst *));
E int FDECL(mongets, (struct monst *, int));
E int FDECL(golemhp, (int));
E boolean FDECL(peace_minded, (struct permonst *));
E void FDECL(set_malign, (struct monst *));
E void FDECL(newmcorpsenm, (struct monst *));
E void FDECL(freemcorpsenm, (struct monst *));
E void FDECL(set_mimic_sym, (struct monst *));
E int FDECL(mbirth_limit, (int));
E void FDECL(mimic_hit_msg, (struct monst *, short));
E void FDECL(mkmonmoney, (struct monst *, long));
E int FDECL(bagotricks, (struct obj *, boolean, int *));
E boolean FDECL(propagate, (int, boolean, boolean));
E boolean FDECL(usmellmon, (struct permonst *));

/* ### mapglyph.c ### */

E int FDECL(mapglyph, (int, int *, int *, unsigned *, int, int));
E char *FDECL(encglyph, (int));
E void FDECL(genl_putmixed, (winid, int, const char *));

/* ### mcastu.c ### */

E int FDECL(castmu, (struct monst *, struct attack *, boolean, boolean));
E int FDECL(buzzmu, (struct monst *, struct attack *));

/* ### mhitm.c ### */

E int FDECL(fightm, (struct monst *));
E int FDECL(mattackm, (struct monst *, struct monst *));
E boolean FDECL(engulf_target, (struct monst *, struct monst *));
E int FDECL(mdisplacem, (struct monst *, struct monst *, boolean));
E void FDECL(paralyze_monst, (struct monst *, int));
E int FDECL(sleep_monst, (struct monst *, int, int));
E void FDECL(slept_monst, (struct monst *));
E void FDECL(xdrainenergym, (struct monst *, boolean));
E long FDECL(attk_protection, (int));
E void FDECL(rustm, (struct monst *, struct obj *));

/* ### mhitu.c ### */

E const char *FDECL(mpoisons_subj, (struct monst *, struct attack *));
E void NDECL(u_slow_down);
E struct monst *NDECL(cloneu);
E void FDECL(expels, (struct monst *, struct permonst *, boolean));
E struct attack *FDECL(getmattk,
                       (struct permonst *, int, int *, struct attack *));
E int FDECL(mattacku, (struct monst *));
E int FDECL(magic_negation, (struct monst *));
E boolean NDECL(gulp_blnd_check);
E int FDECL(gazemu, (struct monst *, struct attack *));
E void FDECL(mdamageu, (struct monst *, int));
E int FDECL(could_seduce, (struct monst *, struct monst *, struct attack *));
E int FDECL(doseduce, (struct monst *));

/* ### minion.c ### */

E void FDECL(newemin, (struct monst *));
E void FDECL(free_emin, (struct monst *));
E int FDECL(monster_census, (boolean));
E int FDECL(msummon, (struct monst *));
E void FDECL(summon_minion, (ALIGNTYP_P, boolean));
E int FDECL(demon_talk, (struct monst *));
E long FDECL(bribe, (struct monst *));
E int FDECL(dprince, (ALIGNTYP_P));
E int FDECL(dlord, (ALIGNTYP_P));
E int NDECL(llord);
E int FDECL(ndemon, (ALIGNTYP_P));
E int NDECL(lminion);
E void FDECL(lose_guardian_angel, (struct monst *));
E void NDECL(gain_guardian_angel);

/* ### mklev.c ### */

#ifdef USE_TRAMPOLI
E int FDECL(do_comp, (genericptr_t, genericptr_t));
#endif
E void NDECL(sort_rooms);
E void FDECL(add_room, (int, int, int, int, boolean, schar, boolean));
E void FDECL(add_subroom, (struct mkroom *, int, int, int, int, boolean,
                           schar, boolean));
E void NDECL(makecorridors);
E void FDECL(add_door, (int, int, struct mkroom *));
E void NDECL(mklev);
#ifdef SPECIALIZATION
E void FDECL(topologize, (struct mkroom *, boolean));
#else
E void FDECL(topologize, (struct mkroom *));
#endif
E void FDECL(place_branch, (branch *, xchar, xchar));
E boolean FDECL(occupied, (xchar, xchar));
E int FDECL(okdoor, (xchar, xchar));
E void FDECL(dodoor, (int, int, struct mkroom *));
E void FDECL(mktrap, (int, int, struct mkroom *, coord *));
E void FDECL(mkstairs, (xchar, xchar, char, struct mkroom *));
E void NDECL(mkinvokearea);
E void FDECL(mineralize, (int, int, int, int, boolean));

/* ### mkmap.c ### */

E void FDECL(flood_fill_rm, (int, int, int, boolean, boolean));
E void FDECL(remove_rooms, (int, int, int, int));
/* E void FDECL(mkmap, (lev_init *)); -- need sp_lev.h for lev_init */

/* ### mkmaze.c ### */

E void FDECL(wallification, (int, int, int, int));
E void FDECL(walkfrom, (int, int, schar));
E void FDECL(makemaz, (const char *));
E void FDECL(mazexy, (coord *));
E void NDECL(bound_digging);
E void FDECL(mkportal, (xchar, xchar, xchar, xchar));
E boolean FDECL(bad_location, (xchar, xchar, xchar, xchar,
                               xchar, xchar));
E void FDECL(place_lregion, (xchar, xchar, xchar, xchar, xchar,
                             xchar, xchar, xchar, xchar, d_level *));
E void NDECL(fumaroles);
E void NDECL(movebubbles);
E void NDECL(water_friction);
E void FDECL(save_waterlevel, (int, int));
E void FDECL(restore_waterlevel, (int));
E const char *FDECL(waterbody_name, (xchar, xchar));

/* ### mkobj.c ### */

E struct oextra *NDECL(newoextra);
E void FDECL(copy_oextra, (struct obj *, struct obj *));
E void FDECL(dealloc_oextra, (struct obj *));
E void FDECL(newomonst, (struct obj *));
E void FDECL(free_omonst, (struct obj *));
E void FDECL(newomid, (struct obj *));
E void FDECL(free_omid, (struct obj *));
E void FDECL(newolong, (struct obj *));
E void FDECL(free_olong, (struct obj *));
E void FDECL(new_omailcmd, (struct obj *, const char *));
E void FDECL(free_omailcmd, (struct obj *));
E struct obj *FDECL(mkobj_at, (char, int, int, boolean));
E struct obj *FDECL(mksobj_at, (int, int, int, boolean, boolean));
E struct obj *FDECL(mkobj, (char, boolean));
E int NDECL(rndmonnum);
E boolean FDECL(bogon_is_pname, (char));
E struct obj *FDECL(splitobj, (struct obj *, long));
E struct obj *FDECL(unsplitobj, (struct obj *));
E void NDECL(clear_splitobjs);
E void FDECL(replace_object, (struct obj *, struct obj *));
E void FDECL(bill_dummy_object, (struct obj *));
E void FDECL(costly_alteration, (struct obj *, int));
E struct obj *FDECL(mksobj, (int, boolean, boolean));
E int FDECL(bcsign, (struct obj *));
E int FDECL(weight, (struct obj *));
E struct obj *FDECL(mkgold, (long, int, int));
E struct obj *FDECL(mkcorpstat, (int, struct monst *, struct permonst *, int,
                                 int, unsigned));
E int FDECL(corpse_revive_type, (struct obj *));
E struct obj *FDECL(obj_attach_mid, (struct obj *, unsigned));
E struct monst *FDECL(get_mtraits, (struct obj *, boolean));
E struct obj *FDECL(mk_tt_object, (int, int, int));
E struct obj *FDECL(mk_named_object,
                    (int, struct permonst *, int, int, const char *));
E struct obj *FDECL(rnd_treefruit_at, (int, int));
E void FDECL(set_corpsenm, (struct obj *, int));
E void FDECL(start_corpse_timeout, (struct obj *));
E void FDECL(bless, (struct obj *));
E void FDECL(unbless, (struct obj *));
E void FDECL(curse, (struct obj *));
E void FDECL(uncurse, (struct obj *));
E void FDECL(blessorcurse, (struct obj *, int));
E boolean FDECL(is_flammable, (struct obj *));
E boolean FDECL(is_rottable, (struct obj *));
E void FDECL(place_object, (struct obj *, int, int));
E void FDECL(remove_object, (struct obj *));
E void FDECL(discard_minvent, (struct monst *));
E void FDECL(obj_extract_self, (struct obj *));
E void FDECL(extract_nobj, (struct obj *, struct obj **));
E void FDECL(extract_nexthere, (struct obj *, struct obj **));
E int FDECL(add_to_minv, (struct monst *, struct obj *));
E struct obj *FDECL(add_to_container, (struct obj *, struct obj *));
E void FDECL(add_to_migration, (struct obj *));
E void FDECL(add_to_buried, (struct obj *));
E void FDECL(dealloc_obj, (struct obj *));
E void FDECL(obj_ice_effects, (int, int, boolean));
E long FDECL(peek_at_iced_corpse_age, (struct obj *));
E int FDECL(hornoplenty, (struct obj *, boolean));
E void NDECL(obj_sanity_check);
E struct obj *FDECL(obj_nexto, (struct obj *));
E struct obj *FDECL(obj_nexto_xy, (int, int, int, unsigned));
E struct obj *FDECL(obj_absorb, (struct obj **, struct obj **));
E struct obj *FDECL(obj_meld, (struct obj **, struct obj **));

/* ### mkroom.c ### */

E void FDECL(mkroom, (int));
E void FDECL(fill_zoo, (struct mkroom *));
E struct permonst *NDECL(antholemon);
E boolean FDECL(nexttodoor, (int, int));
E boolean FDECL(has_dnstairs, (struct mkroom *));
E boolean FDECL(has_upstairs, (struct mkroom *));
E int FDECL(somex, (struct mkroom *));
E int FDECL(somey, (struct mkroom *));
E boolean FDECL(inside_room, (struct mkroom *, xchar, xchar));
E boolean FDECL(somexy, (struct mkroom *, coord *));
E void FDECL(mkundead, (coord *, boolean, int));
E struct permonst *NDECL(courtmon);
E void FDECL(save_rooms, (int));
E void FDECL(rest_rooms, (int));
E struct mkroom *FDECL(search_special, (schar));
E int FDECL(cmap_to_type, (int));

/* ### mon.c ### */

E void NDECL(mon_sanity_check);
E int FDECL(undead_to_corpse, (int));
E int FDECL(genus, (int, int));
E int FDECL(pm_to_cham, (int));
E int FDECL(minliquid, (struct monst *));
E int NDECL(movemon);
E int FDECL(meatmetal, (struct monst *));
E int FDECL(meatobj, (struct monst *));
E void FDECL(mpickgold, (struct monst *));
E boolean FDECL(mpickstuff, (struct monst *, const char *));
E int FDECL(curr_mon_load, (struct monst *));
E int FDECL(max_mon_load, (struct monst *));
E int FDECL(can_carry, (struct monst *, struct obj *));
E int FDECL(mfndpos, (struct monst *, coord *, long *, long));
E boolean FDECL(monnear, (struct monst *, int, int));
E void NDECL(dmonsfree);
E int FDECL(mcalcmove, (struct monst *));
E void NDECL(mcalcdistress);
E void FDECL(replmon, (struct monst *, struct monst *));
E void FDECL(relmon, (struct monst *, struct monst **));
E struct obj *FDECL(mlifesaver, (struct monst *));
E boolean FDECL(corpse_chance, (struct monst *, struct monst *, boolean));
E void FDECL(mondead, (struct monst *));
E void FDECL(mondied, (struct monst *));
E void FDECL(mongone, (struct monst *));
E void FDECL(monstone, (struct monst *));
E void FDECL(monkilled, (struct monst *, const char *, int));
E void FDECL(unstuck, (struct monst *));
E void FDECL(killed, (struct monst *));
E void FDECL(xkilled, (struct monst *, int));
E void FDECL(mon_to_stone, (struct monst *));
E void FDECL(mnexto, (struct monst *));
E void FDECL(maybe_mnexto, (struct monst *));
E boolean FDECL(mnearto, (struct monst *, xchar, xchar, boolean));
E void FDECL(m_respond, (struct monst *));
E void FDECL(setmangry, (struct monst *));
E void FDECL(wakeup, (struct monst *));
E void NDECL(wake_nearby);
E void FDECL(wake_nearto, (int, int, int));
E void FDECL(seemimic, (struct monst *));
E void NDECL(rescham);
E void NDECL(restartcham);
E void FDECL(restore_cham, (struct monst *));
E boolean FDECL(hideunder, (struct monst *));
E void FDECL(hide_monst, (struct monst *));
E void FDECL(mon_animal_list, (boolean));
E int FDECL(select_newcham_form, (struct monst *));
E void FDECL(mgender_from_permonst, (struct monst *, struct permonst *));
E int FDECL(newcham,
            (struct monst *, struct permonst *, boolean, boolean));
E int FDECL(can_be_hatched, (int));
E int FDECL(egg_type_from_parent, (int, boolean));
E boolean FDECL(dead_species, (int, boolean));
E void NDECL(kill_genocided_monsters);
E void FDECL(golemeffects, (struct monst *, int, int));
E boolean FDECL(angry_guards, (boolean));
E void NDECL(pacify_guards);
E void FDECL(decide_to_shapeshift, (struct monst *, int));

/* ### mondata.c ### */

E void FDECL(set_mon_data, (struct monst *, struct permonst *, int));
E struct attack *FDECL(attacktype_fordmg, (struct permonst *, int, int));
E boolean FDECL(attacktype, (struct permonst *, int));
E boolean FDECL(noattacks, (struct permonst *));
E boolean FDECL(poly_when_stoned, (struct permonst *));
E boolean FDECL(resists_drli, (struct monst *));
E boolean FDECL(resists_magm, (struct monst *));
E boolean FDECL(resists_blnd, (struct monst *));
E boolean
FDECL(can_blnd, (struct monst *, struct monst *, uchar, struct obj *));
E boolean FDECL(ranged_attk, (struct permonst *));
E boolean FDECL(hates_silver, (struct permonst *));
E boolean FDECL(mon_hates_silver, (struct monst *));
E boolean FDECL(passes_bars, (struct permonst *));
E boolean FDECL(can_blow, (struct monst *));
E boolean FDECL(can_be_strangled, (struct monst *));
E boolean FDECL(can_track, (struct permonst *));
E boolean FDECL(breakarm, (struct permonst *));
E boolean FDECL(sliparm, (struct permonst *));
E boolean FDECL(sticks, (struct permonst *));
E boolean FDECL(cantvomit, (struct permonst *));
E int FDECL(num_horns, (struct permonst *));
/* E boolean FDECL(canseemon, (struct monst *)); */
E struct attack *FDECL(dmgtype_fromattack, (struct permonst *, int, int));
E boolean FDECL(dmgtype, (struct permonst *, int));
E int FDECL(max_passive_dmg, (struct monst *, struct monst *));
E boolean FDECL(same_race, (struct permonst *, struct permonst *));
E int FDECL(monsndx, (struct permonst *));
E int FDECL(name_to_mon, (const char *));
E int FDECL(name_to_monclass, (const char *, int *));
E int FDECL(gender, (struct monst *));
E int FDECL(pronoun_gender, (struct monst *));
E boolean FDECL(levl_follower, (struct monst *));
E int FDECL(little_to_big, (int));
E int FDECL(big_to_little, (int));
E const char *FDECL(locomotion, (const struct permonst *, const char *));
E const char *FDECL(stagger, (const struct permonst *, const char *));
E const char *FDECL(on_fire, (struct permonst *, struct attack *));
E const struct permonst *FDECL(raceptr, (struct monst *));
E boolean FDECL(olfaction, (struct permonst *));

/* ### monmove.c ### */

E boolean FDECL(itsstuck, (struct monst *));
E boolean FDECL(mb_trapped, (struct monst *));
E boolean FDECL(monhaskey, (struct monst *, boolean));
E void FDECL(mon_regen, (struct monst *, boolean));
E int FDECL(dochugw, (struct monst *));
E boolean FDECL(onscary, (int, int, struct monst *));
E void FDECL(monflee, (struct monst *, int, boolean, boolean));
E void FDECL(mon_yells, (struct monst *, const char *));
E int FDECL(dochug, (struct monst *));
E int FDECL(m_move, (struct monst *, int));
E void FDECL(dissolve_bars, (int, int));
E boolean FDECL(closed_door, (int, int));
E boolean FDECL(accessible, (int, int));
E void FDECL(set_apparxy, (struct monst *));
E boolean FDECL(can_ooze, (struct monst *));
E boolean FDECL(can_fog, (struct monst *));
E boolean FDECL(should_displace,
                (struct monst *, coord *, long *, int, xchar, xchar));
E boolean FDECL(undesirable_disp, (struct monst *, xchar, xchar));

/* ### monst.c ### */

E void NDECL(monst_init);

/* ### monstr.c ### */

E void NDECL(monstr_init);

/* ### mplayer.c ### */

E struct monst *FDECL(mk_mplayer,
                      (struct permonst *, xchar, xchar, boolean));
E void FDECL(create_mplayers, (int, boolean));
E void FDECL(mplayer_talk, (struct monst *));

#if defined(MICRO) || defined(WIN32)

/* ### msdos.c,os2.c,tos.c,winnt.c ### */

#ifndef WIN32
E int NDECL(tgetch);
#endif
#ifndef TOS
E char NDECL(switchar);
#endif
#ifndef __GO32__
E long FDECL(freediskspace, (char *));
#ifdef MSDOS
E int FDECL(findfirst_file, (char *));
E int NDECL(findnext_file);
E long FDECL(filesize_nh, (char *));
#else
E int FDECL(findfirst, (char *));
E int NDECL(findnext);
E long FDECL(filesize, (char *));
#endif /* MSDOS */
E char *NDECL(foundfile_buffer);
#endif /* __GO32__ */
E void FDECL(chdrive, (char *));
#ifndef TOS
E void NDECL(disable_ctrlP);
E void NDECL(enable_ctrlP);
#endif
#if defined(MICRO) && !defined(WINNT)
E void NDECL(get_scr_size);
#ifndef TOS
E void FDECL(gotoxy, (int, int));
#endif
#endif
#ifdef TOS
E int FDECL(_copyfile, (char *, char *));
E int NDECL(kbhit);
E void NDECL(set_colors);
E void NDECL(restore_colors);
#ifdef SUSPEND
E int NDECL(dosuspend);
#endif
#endif /* TOS */
#ifdef WIN32
E char *FDECL(get_username, (int *));
E void FDECL(nt_regularize, (char *));
E int NDECL((*nt_kbhit));
E void FDECL(Delay, (int));
#endif /* WIN32 */
#endif /* MICRO || WIN32 */

/* ### mthrowu.c ### */

E int FDECL(thitu, (int, int, struct obj *, const char *));
E int FDECL(ohitmon, (struct monst *, struct obj *, int, boolean));
E void FDECL(thrwmu, (struct monst *));
E int FDECL(spitmu, (struct monst *, struct attack *));
E int FDECL(breamu, (struct monst *, struct attack *));
E boolean FDECL(linedup, (xchar, xchar, xchar, xchar, int));
E boolean FDECL(lined_up, (struct monst *));
E struct obj *FDECL(m_carrying, (struct monst *, int));
E void FDECL(m_useupall, (struct monst *, struct obj *));
E void FDECL(m_useup, (struct monst *, struct obj *));
E void FDECL(m_throw,
             (struct monst *, int, int, int, int, int, struct obj *));
E boolean FDECL(hits_bars, (struct obj **, int, int, int, int));

/* ### muse.c ### */

E boolean FDECL(find_defensive, (struct monst *));
E int FDECL(use_defensive, (struct monst *));
E int FDECL(rnd_defensive_item, (struct monst *));
E boolean FDECL(find_offensive, (struct monst *));
#ifdef USE_TRAMPOLI
E int FDECL(mbhitm, (struct monst *, struct obj *));
#endif
E int FDECL(use_offensive, (struct monst *));
E int FDECL(rnd_offensive_item, (struct monst *));
E boolean FDECL(find_misc, (struct monst *));
E int FDECL(use_misc, (struct monst *));
E int FDECL(rnd_misc_item, (struct monst *));
E boolean FDECL(searches_for_item, (struct monst *, struct obj *));
E boolean FDECL(mon_reflects, (struct monst *, const char *));
E boolean FDECL(ureflects, (const char *, const char *));
E boolean FDECL(munstone, (struct monst *, boolean));
E boolean FDECL(munslime, (struct monst *, boolean));

/* ### music.c ### */

E void FDECL(awaken_soldiers, (struct monst *));
E int FDECL(do_play_instrument, (struct obj *));

/* ### nhlan.c ### */
#ifdef LAN_FEATURES
E void NDECL(init_lan_features);
E char *NDECL(lan_username);
#endif

/* ### nhregex.c ### */
E struct nhregex *NDECL(regex_init);
E boolean FDECL(regex_compile, (const char *, struct nhregex *));
E const char *FDECL(regex_error_desc, (struct nhregex *));
E boolean FDECL(regex_match, (const char *, struct nhregex *));
E void FDECL(regex_free, (struct nhregex *));

/* ### nttty.c ### */

#ifdef WIN32
E void NDECL(get_scr_size);
E int NDECL(nttty_kbhit);
E void FDECL(nttty_open, (int));
E void NDECL(nttty_rubout);
E int NDECL(tgetch);
E int FDECL(ntposkey, (int *, int *, int *));
E void FDECL(set_output_mode, (int));
E void NDECL(synch_cursor);
#endif

/* ### o_init.c ### */

E void NDECL(init_objects);
E void FDECL(obj_shuffle_range, (int, int *, int *));
E int NDECL(find_skates);
E void NDECL(oinit);
E void FDECL(savenames, (int, int));
E void FDECL(restnames, (int));
E void FDECL(discover_object, (int, boolean, boolean));
E void FDECL(undiscover_object, (int));
E int NDECL(dodiscovered);
E int NDECL(doclassdisco);
E void NDECL(rename_disco);

/* ### objects.c ### */

E void NDECL(objects_init);

/* ### objnam.c ### */

E char *FDECL(obj_typename, (int));
E char *FDECL(simple_typename, (int));
E boolean FDECL(obj_is_pname, (struct obj *));
E char *FDECL(distant_name, (struct obj *, char *(*)(OBJ_P)));
E char *FDECL(fruitname, (boolean));
E char *FDECL(xname, (struct obj *));
E char *FDECL(mshot_xname, (struct obj *));
E boolean FDECL(the_unique_obj, (struct obj *));
E boolean FDECL(the_unique_pm, (struct permonst *));
E char *FDECL(doname, (struct obj *));
E char *FDECL(doname_with_price, (struct obj *));
E boolean FDECL(not_fully_identified, (struct obj *));
E char *FDECL(corpse_xname, (struct obj *, const char *, unsigned));
E char *FDECL(cxname, (struct obj *));
E char *FDECL(cxname_singular, (struct obj *));
E char *FDECL(killer_xname, (struct obj *));
E char *FDECL(short_oname,
              (struct obj *, char *(*)(OBJ_P), char *(*)(OBJ_P), unsigned));
E const char *FDECL(singular, (struct obj *, char *(*)(OBJ_P)));
E char *FDECL(an, (const char *));
E char *FDECL(An, (const char *));
E char *FDECL(The, (const char *));
E char *FDECL(the, (const char *));
E char *FDECL(aobjnam, (struct obj *, const char *));
E char *FDECL(yobjnam, (struct obj *, const char *));
E char *FDECL(Yobjnam2, (struct obj *, const char *));
E char *FDECL(Tobjnam, (struct obj *, const char *));
E char *FDECL(otense, (struct obj *, const char *));
E char *FDECL(vtense, (const char *, const char *));
E char *FDECL(Doname2, (struct obj *));
E char *FDECL(yname, (struct obj *));
E char *FDECL(Yname2, (struct obj *));
E char *FDECL(ysimple_name, (struct obj *));
E char *FDECL(Ysimple_name2, (struct obj *));
E char *FDECL(simpleonames, (struct obj *));
E char *FDECL(ansimpleoname, (struct obj *));
E char *FDECL(thesimpleoname, (struct obj *));
E char *FDECL(bare_artifactname, (struct obj *));
E char *FDECL(makeplural, (const char *));
E char *FDECL(makesingular, (const char *));
E struct obj *FDECL(readobjnam, (char *, struct obj *));
E int FDECL(rnd_class, (int, int));
E const char *FDECL(suit_simple_name, (struct obj *));
E const char *FDECL(cloak_simple_name, (struct obj *));
E const char *FDECL(helm_simple_name, (struct obj *));
E const char *FDECL(mimic_obj_name, (struct monst *));
E char *FDECL(safe_qbuf, (char *, const char *, const char *, struct obj *,
                          char *(*)(OBJ_P), char *(*)(OBJ_P), const char *));

/* ### options.c ### */

E void NDECL(reglyph_darkroom);
E boolean FDECL(match_optname, (const char *, const char *, int, boolean));
E void NDECL(initoptions);
E void NDECL(initoptions_init);
E void NDECL(initoptions_finish);
E void FDECL(parseoptions, (char *, boolean, boolean));
E int NDECL(doset);
E int NDECL(dotogglepickup);
E void NDECL(option_help);
E void FDECL(next_opt, (winid, const char *));
E int FDECL(fruitadd, (char *, struct fruit *));
E int FDECL(choose_classes_menu,
            (const char *, int, boolean, char *, char *));
E void FDECL(add_menu_cmd_alias, (char, char));
E char FDECL(map_menu_cmd, (char));
E void FDECL(assign_warnings, (uchar *));
E char *FDECL(nh_getenv, (const char *));
E void FDECL(set_duplicate_opt_detection, (int));
E void FDECL(set_wc_option_mod_status, (unsigned long, int));
E void FDECL(set_wc2_option_mod_status, (unsigned long, int));
E void FDECL(set_option_mod_status, (const char *, int));
E int FDECL(add_autopickup_exception, (const char *));
E void NDECL(free_autopickup_exceptions);
E int FDECL(load_symset, (const char *, int));
E void NDECL(free_symsets);
E void FDECL(parsesymbols, (char *));
E struct symparse *FDECL(match_sym, (char *));
E void NDECL(set_playmode);
E int FDECL(sym_val, (char *));
E const char *FDECL(clr2colorname, (int));
E boolean FDECL(add_menu_coloring, (char *));
E boolean FDECL(get_menu_coloring, (char *, int *, int *));
E void NDECL(free_menu_coloring);
E boolean FDECL(msgtype_parse_add, (char *));
E int FDECL(msgtype_type, (const char *));
E void NDECL(msgtype_free);

/* ### pager.c ### */

E char *FDECL(self_lookat, (char *));
E boolean FDECL(object_from_map, (int,int,int,struct obj **));
E int NDECL(dowhatis);
E int NDECL(doquickwhatis);
E int NDECL(doidtrap);
E int NDECL(dowhatdoes);
E char *FDECL(dowhatdoes_core, (char, char *));
E int NDECL(dohelp);
E int NDECL(dohistory);
E int FDECL(do_screen_description,
            (coord, boolean, int, char *, const char **));
E int FDECL(do_look, (int, coord *));

/* ### pcmain.c ### */

#if defined(MICRO) || defined(WIN32)
#ifdef CHDIR
E void FDECL(chdirx, (char *, boolean));
#endif /* CHDIR */
E boolean NDECL(authorize_wizard_mode);
#endif /* MICRO || WIN32 */

/* ### pcsys.c ### */

#if defined(MICRO) || defined(WIN32)
E void NDECL(flushout);
E int NDECL(dosh);
#ifdef MFLOPPY
E void FDECL(eraseall, (const char *, const char *));
E void FDECL(copybones, (int));
E void NDECL(playwoRAMdisk);
E int FDECL(saveDiskPrompt, (int));
E void NDECL(gameDiskPrompt);
#endif
E void FDECL(append_slash, (char *));
E void FDECL(getreturn, (const char *));
#ifndef AMIGA
E void VDECL(msmsg, (const char *, ...));
#endif
E FILE *FDECL(fopenp, (const char *, const char *));
#endif /* MICRO || WIN32 */

/* ### pctty.c ### */

#if defined(MICRO) || defined(WIN32)
E void NDECL(gettty);
E void FDECL(settty, (const char *));
E void NDECL(setftty);
E void VDECL(error, (const char *, ...));
#if defined(TIMED_DELAY) && defined(_MSC_VER)
E void FDECL(msleep, (unsigned));
#endif
#endif /* MICRO || WIN32 */

/* ### pcunix.c ### */

#if defined(MICRO)
E void FDECL(regularize, (char *));
#endif /* MICRO */
#if defined(PC_LOCKING)
E void NDECL(getlock);
#endif

/* ### pickup.c ### */

E int FDECL(collect_obj_classes, (char *, struct obj *, boolean,
                                  boolean FDECL((*), (OBJ_P)), int *));
E boolean FDECL(rider_corpse_revival, (struct obj *, boolean));
E void FDECL(add_valid_menu_class, (int));
E boolean FDECL(allow_all, (struct obj *));
E boolean FDECL(allow_category, (struct obj *));
E boolean FDECL(is_worn_by_type, (struct obj *));
E int FDECL(ck_bag, (struct obj *));
#ifdef USE_TRAMPOLI
E int FDECL(in_container, (struct obj *));
E int FDECL(out_container, (struct obj *));
#endif
E int FDECL(pickup, (int));
E int FDECL(pickup_object, (struct obj *, long, boolean));
E int FDECL(query_category,
            (const char *, struct obj *, int, menu_item **, int));
E int FDECL(query_objlist, (const char *, struct obj *, int, menu_item **,
                            int, boolean (*)(OBJ_P)));
E struct obj *FDECL(pick_obj, (struct obj *));
E int NDECL(encumber_msg);
E int NDECL(doloot);
E boolean FDECL(container_gone, (int (*)(OBJ_P)));
E boolean NDECL(u_handsy);
E int FDECL(use_container, (struct obj **, int));
E int FDECL(loot_mon, (struct monst *, int *, boolean *));
E int NDECL(dotip);
E boolean FDECL(is_autopickup_exception, (struct obj *, boolean));

/* ### pline.c ### */

E void VDECL(pline, (const char *, ...)) PRINTF_F(1, 2);
E void VDECL(Norep, (const char *, ...)) PRINTF_F(1, 2);
E void NDECL(free_youbuf);
E void VDECL(You, (const char *, ...)) PRINTF_F(1, 2);
E void VDECL(Your, (const char *, ...)) PRINTF_F(1, 2);
E void VDECL(You_feel, (const char *, ...)) PRINTF_F(1, 2);
E void VDECL(You_cant, (const char *, ...)) PRINTF_F(1, 2);
E void VDECL(You_hear, (const char *, ...)) PRINTF_F(1, 2);
E void VDECL(You_see, (const char *, ...)) PRINTF_F(1, 2);
E void VDECL(pline_The, (const char *, ...)) PRINTF_F(1, 2);
E void VDECL(There, (const char *, ...)) PRINTF_F(1, 2);
E void VDECL(verbalize, (const char *, ...)) PRINTF_F(1, 2);
E void VDECL(raw_printf, (const char *, ...)) PRINTF_F(1, 2);
E void VDECL(impossible, (const char *, ...)) PRINTF_F(1, 2);
E const char *FDECL(align_str, (ALIGNTYP_P));
E void FDECL(mstatusline, (struct monst *));
E void NDECL(ustatusline);
E void NDECL(self_invis_message);
E void FDECL(pudding_merge_message, (struct obj *, struct obj *));

/* ### polyself.c ### */

E void NDECL(set_uasmon);
E void NDECL(float_vs_flight);
E void NDECL(change_sex);
E void FDECL(polyself, (int));
E int FDECL(polymon, (int));
E void NDECL(rehumanize);
E int NDECL(dobreathe);
E int NDECL(dospit);
E int NDECL(doremove);
E int NDECL(dospinweb);
E int NDECL(dosummon);
E int NDECL(dogaze);
E int NDECL(dohide);
E int NDECL(dopoly);
E int NDECL(domindblast);
E void FDECL(skinback, (boolean));
E const char *FDECL(mbodypart, (struct monst *, int));
E const char *FDECL(body_part, (int));
E int NDECL(poly_gender);
E void FDECL(ugolemeffects, (int, int));

/* ### potion.c ### */

E void FDECL(set_itimeout, (long *, long));
E void FDECL(incr_itimeout, (long *, int));
E void FDECL(make_confused, (long, boolean));
E void FDECL(make_stunned, (long, boolean));
E void FDECL(make_blinded, (long, boolean));
E void FDECL(make_sick, (long, const char *, boolean, int));
E void FDECL(make_slimed, (long, const char *));
E void FDECL(make_stoned, (long, const char *, int, const char *));
E void FDECL(make_vomiting, (long, boolean));
E boolean FDECL(make_hallucinated, (long, boolean, long));
E void FDECL(make_deaf, (long, boolean));
E int NDECL(dodrink);
E int FDECL(dopotion, (struct obj *));
E int FDECL(peffects, (struct obj *));
E void FDECL(healup, (int, int, boolean, boolean));
E void FDECL(strange_feeling, (struct obj *, const char *));
E void FDECL(potionhit, (struct monst *, struct obj *, boolean));
E void FDECL(potionbreathe, (struct obj *));
E int NDECL(dodip);
E void FDECL(mongrantswish, (struct monst **));
E void FDECL(djinni_from_bottle, (struct obj *));
E struct monst *FDECL(split_mon, (struct monst *, struct monst *));
E const char *NDECL(bottlename);

/* ### pray.c ### */

E boolean FDECL(critically_low_hp, (boolean));
#ifdef USE_TRAMPOLI
E int NDECL(prayer_done);
#endif
E int NDECL(dosacrifice);
E boolean FDECL(can_pray, (boolean));
E int NDECL(dopray);
E const char *NDECL(u_gname);
E int NDECL(doturn);
E const char *NDECL(a_gname);
E const char *FDECL(a_gname_at, (xchar x, xchar y));
E const char *FDECL(align_gname, (ALIGNTYP_P));
E const char *FDECL(halu_gname, (ALIGNTYP_P));
E const char *FDECL(align_gtitle, (ALIGNTYP_P));
E void FDECL(altar_wrath, (int, int));

/* ### priest.c ### */

E int FDECL(move_special, (struct monst *, boolean, schar, boolean,
                           boolean, xchar, xchar, xchar, xchar));
E char FDECL(temple_occupied, (char *));
E boolean FDECL(inhistemple, (struct monst *));
E int FDECL(pri_move, (struct monst *));
E void FDECL(priestini, (d_level *, struct mkroom *, int, int, boolean));
E aligntyp FDECL(mon_aligntyp, (struct monst *));
E char *FDECL(priestname, (struct monst *, char *));
E boolean FDECL(p_coaligned, (struct monst *));
E struct monst *FDECL(findpriest, (char));
E void FDECL(intemple, (int));
E void FDECL(forget_temple_entry, (struct monst *));
E void FDECL(priest_talk, (struct monst *));
E struct monst *FDECL(mk_roamer, (struct permonst *, ALIGNTYP_P, xchar,
                                  xchar, boolean));
E void FDECL(reset_hostility, (struct monst *));
E boolean FDECL(in_your_sanctuary, (struct monst *, xchar, xchar));
E void FDECL(ghod_hitsu, (struct monst *));
E void NDECL(angry_priest);
E void NDECL(clearpriests);
E void FDECL(restpriest, (struct monst *, boolean));
E void FDECL(newepri, (struct monst *));
E void FDECL(free_epri, (struct monst *));

/* ### quest.c ### */

E void NDECL(onquest);
E void NDECL(nemdead);
E void FDECL(artitouch, (struct obj *));
E boolean NDECL(ok_to_quest);
E void FDECL(leader_speaks, (struct monst *));
E void NDECL(nemesis_speaks);
E void FDECL(quest_chat, (struct monst *));
E void FDECL(quest_talk, (struct monst *));
E void FDECL(quest_stat_check, (struct monst *));
E void FDECL(finish_quest, (struct obj *));

/* ### questpgr.c ### */

E void NDECL(load_qtlist);
E void NDECL(unload_qtlist);
E short FDECL(quest_info, (int));
E const char *NDECL(ldrname);
E boolean FDECL(is_quest_artifact, (struct obj *));
E void FDECL(com_pager, (int));
E void FDECL(qt_pager, (int));
E struct permonst *NDECL(qt_montype);
E void NDECL(deliver_splev_message);

/* ### random.c ### */

#if defined(RANDOM) && !defined(__GO32__) /* djgpp has its own random */
E void FDECL(srandom, (unsigned));
E char *FDECL(initstate, (unsigned, char *, int));
E char *FDECL(setstate, (char *));
E long NDECL(random);
#endif /* RANDOM */

/* ### read.c ### */

E void FDECL(learnscroll, (struct obj *));
E char *FDECL(tshirt_text, (struct obj *, char *));
E int NDECL(doread);
E boolean FDECL(is_chargeable, (struct obj *));
E void FDECL(recharge, (struct obj *, int));
E void FDECL(forget_objects, (int));
E void FDECL(forget_levels, (int));
E void NDECL(forget_traps);
E void FDECL(forget_map, (int));
E int FDECL(seffects, (struct obj *));
E void FDECL(drop_boulder_on_player,
             (boolean, boolean, boolean, boolean));
E boolean FDECL(drop_boulder_on_monster, (int, int, boolean, boolean));
E void FDECL(wand_explode, (struct obj *, int));
#ifdef USE_TRAMPOLI
E void FDECL(set_lit, (int, int, genericptr_t));
#endif
E void FDECL(litroom, (boolean, struct obj *));
E void FDECL(do_genocide, (int));
E void FDECL(punish, (struct obj *));
E void NDECL(unpunish);
E boolean FDECL(cant_revive, (int *, boolean, struct obj *));
E boolean NDECL(create_particular);

/* ### rect.c ### */

E void NDECL(init_rect);
E NhRect *FDECL(get_rect, (NhRect *));
E NhRect *NDECL(rnd_rect);
E void FDECL(remove_rect, (NhRect *));
E void FDECL(add_rect, (NhRect *));
E void FDECL(split_rects, (NhRect *, NhRect *));

/* ## region.c ### */
E void NDECL(clear_regions);
E void NDECL(run_regions);
E boolean FDECL(in_out_region, (xchar, xchar));
E boolean FDECL(m_in_out_region, (struct monst *, xchar, xchar));
E void NDECL(update_player_regions);
E void FDECL(update_monster_region, (struct monst *));
E NhRegion *FDECL(visible_region_at, (xchar, xchar));
E void FDECL(show_region, (NhRegion *, xchar, xchar));
E void FDECL(save_regions, (int, int));
E void FDECL(rest_regions, (int, boolean));
E NhRegion *FDECL(create_gas_cloud, (xchar, xchar, int, int));
E boolean NDECL(region_danger);
E void NDECL(region_safety);

/* ### restore.c ### */

E void FDECL(inven_inuse, (boolean));
E int FDECL(dorecover, (int));
E void FDECL(restcemetery, (int, struct cemetery **));
E void FDECL(trickery, (char *));
E void FDECL(getlev, (int, int, xchar, boolean));
E void FDECL(get_plname_from_file, (int, char *));
#ifdef SELECTSAVED
E int FDECL(restore_menu, (winid));
#endif
E void NDECL(minit);
E boolean FDECL(lookup_id_mapping, (unsigned, unsigned *));
E void FDECL(mread, (int, genericptr_t, unsigned int));
E int FDECL(validate, (int, const char *));
E void NDECL(reset_restpref);
E void FDECL(set_restpref, (const char *));
E void FDECL(set_savepref, (const char *));

/* ### rip.c ### */

E void FDECL(genl_outrip, (winid, int, time_t));

/* ### rnd.c ### */

E int FDECL(rn2, (int));
E int FDECL(rnl, (int));
E int FDECL(rnd, (int));
E int FDECL(d, (int, int));
E int FDECL(rne, (int));
E int FDECL(rnz, (int));

/* ### role.c ### */

E boolean FDECL(validrole, (int));
E boolean FDECL(validrace, (int, int));
E boolean FDECL(validgend, (int, int, int));
E boolean FDECL(validalign, (int, int, int));
E int NDECL(randrole);
E int FDECL(randrace, (int));
E int FDECL(randgend, (int, int));
E int FDECL(randalign, (int, int));
E int FDECL(str2role, (const char *));
E int FDECL(str2race, (const char *));
E int FDECL(str2gend, (const char *));
E int FDECL(str2align, (const char *));
E boolean FDECL(ok_role, (int, int, int, int));
E int FDECL(pick_role, (int, int, int, int));
E boolean FDECL(ok_race, (int, int, int, int));
E int FDECL(pick_race, (int, int, int, int));
E boolean FDECL(ok_gend, (int, int, int, int));
E int FDECL(pick_gend, (int, int, int, int));
E boolean FDECL(ok_align, (int, int, int, int));
E int FDECL(pick_align, (int, int, int, int));
E void NDECL(rigid_role_checks);
E boolean FDECL(setrolefilter, (const char *));
E boolean NDECL(gotrolefilter);
E void NDECL(clearrolefilter);
E char *FDECL(build_plselection_prompt, (char *, int, int, int, int, int));
E char *FDECL(root_plselection_prompt, (char *, int, int, int, int, int));
E void NDECL(plnamesuffix);
E void FDECL(role_selection_prolog, (int, winid));
E void FDECL(role_menu_extra, (int, winid));
E void NDECL(role_init);
E const char *FDECL(Hello, (struct monst *));
E const char *NDECL(Goodbye);

/* ### rumors.c ### */

E char *FDECL(getrumor, (int, char *, boolean));
E char *FDECL(get_rnd_text, (const char *, char *));
E void FDECL(outrumor, (int, int));
E void FDECL(outoracle, (boolean, boolean));
E void FDECL(save_oracles, (int, int));
E void FDECL(restore_oracles, (int));
E int FDECL(doconsult, (struct monst *));
E void NDECL(rumor_check);

/* ### save.c ### */

E int NDECL(dosave);
E int NDECL(dosave0);
E boolean FDECL(tricked_fileremoved, (int, char *));
#ifdef INSURANCE
E void NDECL(savestateinlock);
#endif
#ifdef MFLOPPY
E boolean FDECL(savelev, (int, xchar, int));
E boolean FDECL(swapin_file, (int));
E void NDECL(co_false);
#else
E void FDECL(savelev, (int, xchar, int));
#endif
E genericptr_t FDECL(mon_to_buffer, (struct monst *, int *));
E void FDECL(bufon, (int));
E void FDECL(bufoff, (int));
E void FDECL(bflush, (int));
E void FDECL(bwrite, (int, genericptr_t, unsigned int));
E void FDECL(bclose, (int));
E void FDECL(def_bclose, (int));
#if defined(ZEROCOMP)
E void FDECL(zerocomp_bclose, (int));
#endif
E void FDECL(savecemetery, (int, int, struct cemetery **));
E void FDECL(savefruitchn, (int, int));
E void FDECL(store_plname_in_file, (int));
E void NDECL(free_dungeons);
E void NDECL(freedynamicdata);
E void FDECL(store_savefileinfo, (int));

/* ### shk.c ### */

E long FDECL(money2mon, (struct monst *, long));
E void FDECL(money2u, (struct monst *, long));
E void FDECL(shkgone, (struct monst *));
E void FDECL(set_residency, (struct monst *, boolean));
E void FDECL(replshk, (struct monst *, struct monst *));
E void FDECL(restshk, (struct monst *, boolean));
E char FDECL(inside_shop, (xchar, xchar));
E void FDECL(u_left_shop, (char *, boolean));
E void FDECL(remote_burglary, (xchar, xchar));
E void FDECL(u_entered_shop, (char *));
E void FDECL(pick_pick, (struct obj *));
E boolean FDECL(same_price, (struct obj *, struct obj *));
E void NDECL(shopper_financial_report);
E int FDECL(inhishop, (struct monst *));
E struct monst *FDECL(shop_keeper, (char));
E boolean FDECL(tended_shop, (struct mkroom *));
E boolean FDECL(is_unpaid, (struct obj *));
E void FDECL(delete_contents, (struct obj *));
E void FDECL(obfree, (struct obj *, struct obj *));
E void FDECL(home_shk, (struct monst *, boolean));
E void FDECL(make_happy_shk, (struct monst *, boolean));
E void FDECL(make_happy_shoppers, (boolean));
E void FDECL(hot_pursuit, (struct monst *));
E void FDECL(make_angry_shk, (struct monst *, xchar, xchar));
E int NDECL(dopay);
E boolean FDECL(paybill, (int));
E void NDECL(finish_paybill);
E struct obj *FDECL(find_oid, (unsigned));
E long FDECL(contained_cost,
             (struct obj *, struct monst *, long, boolean, boolean));
E long FDECL(contained_gold, (struct obj *));
E void FDECL(picked_container, (struct obj *));
E void FDECL(alter_cost, (struct obj *, long));
E long FDECL(unpaid_cost, (struct obj *, boolean));
E boolean FDECL(billable, (struct monst **, struct obj *, char, boolean));
E void FDECL(addtobill, (struct obj *, boolean, boolean, boolean));
E void FDECL(splitbill, (struct obj *, struct obj *));
E void FDECL(subfrombill, (struct obj *, struct monst *));
E long FDECL(stolen_value,
             (struct obj *, xchar, xchar, boolean, boolean));
E void FDECL(sellobj_state, (int));
E void FDECL(sellobj, (struct obj *, xchar, xchar));
E int FDECL(doinvbill, (int));
E struct monst *FDECL(shkcatch, (struct obj *, xchar, xchar));
E void FDECL(add_damage, (xchar, xchar, long));
E int FDECL(repair_damage, (struct monst *, struct damage *, boolean));
E int FDECL(shk_move, (struct monst *));
E void FDECL(after_shk_move, (struct monst *));
E boolean FDECL(is_fshk, (struct monst *));
E void FDECL(shopdig, (int));
E void FDECL(pay_for_damage, (const char *, boolean));
E boolean FDECL(costly_spot, (xchar, xchar));
E struct obj *FDECL(shop_object, (xchar, xchar));
E void FDECL(price_quote, (struct obj *));
E void FDECL(shk_chat, (struct monst *));
E void FDECL(check_unpaid_usage, (struct obj *, boolean));
E void FDECL(check_unpaid, (struct obj *));
E void FDECL(costly_gold, (xchar, xchar, long));
E long FDECL(get_cost_of_shop_item, (struct obj *));
E boolean FDECL(block_door, (xchar, xchar));
E boolean FDECL(block_entry, (xchar, xchar));
E char *FDECL(shk_your, (char *, struct obj *));
E char *FDECL(Shk_Your, (char *, struct obj *));

/* ### shknam.c ### */

E void FDECL(neweshk, (struct monst *));
E void FDECL(free_eshk, (struct monst *));
E void FDECL(stock_room, (int, struct mkroom *));
E boolean FDECL(saleable, (struct monst *, struct obj *));
E int FDECL(get_shop_item, (int));
E const char *FDECL(shkname, (struct monst *));
E boolean FDECL(shkname_is_pname, (struct monst *));
E boolean FDECL(is_izchak, (struct monst *, boolean));

/* ### sit.c ### */

E void NDECL(take_gold);
E int NDECL(dosit);
E void NDECL(rndcurse);
E void NDECL(attrcurse);

/* ### sounds.c ### */

E void NDECL(dosounds);
E const char *FDECL(growl_sound, (struct monst *));
E void FDECL(growl, (struct monst *));
E void FDECL(yelp, (struct monst *));
E void FDECL(whimper, (struct monst *));
E void FDECL(beg, (struct monst *));
E int NDECL(dotalk);
#ifdef USER_SOUNDS
E int FDECL(add_sound_mapping, (const char *));
E void FDECL(play_sound_for_message, (const char *));
#endif

/* ### sys.c ### */

E void NDECL(sys_early_init);
E void NDECL(sysopt_release);
E void FDECL(sysopt_seduce_set, (int));

/* ### sys/msdos/sound.c ### */

#ifdef MSDOS
E int FDECL(assign_soundcard, (char *));
#endif

/* ### sp_lev.c ### */

E boolean FDECL(check_room, (xchar *, xchar *, xchar *, xchar *, boolean));
E boolean FDECL(create_room, (xchar, xchar, xchar, xchar, xchar,
                              xchar, xchar, xchar));
E void FDECL(create_secret_door, (struct mkroom *, xchar));
E boolean
FDECL(dig_corridor, (coord *, coord *, boolean, schar, schar));
E void FDECL(fill_room, (struct mkroom *, boolean));
E boolean FDECL(load_special, (const char *));

/* ### spell.c ### */

#ifdef USE_TRAMPOLI
E int NDECL(learn);
#endif
E int FDECL(study_book, (struct obj *));
E void FDECL(book_disappears, (struct obj *));
E void FDECL(book_substitution, (struct obj *, struct obj *));
E void NDECL(age_spells);
E int NDECL(docast);
E int FDECL(spell_skilltype, (int));
E int FDECL(spelleffects, (int, boolean));
E void NDECL(losespells);
E int NDECL(dovspell);
E void FDECL(initialspell, (struct obj *));

/* ### steal.c ### */

#ifdef USE_TRAMPOLI
E int NDECL(stealarm);
#endif
E long FDECL(somegold, (long));
E void FDECL(stealgold, (struct monst *));
E void FDECL(remove_worn_item, (struct obj *, boolean));
E int FDECL(steal, (struct monst *, char *));
E int FDECL(mpickobj, (struct monst *, struct obj *));
E void FDECL(stealamulet, (struct monst *));
E void FDECL(maybe_absorb_item, (struct monst *, struct obj *, int, int));
E void FDECL(mdrop_obj, (struct monst *, struct obj *, boolean));
E void FDECL(mdrop_special_objs, (struct monst *));
E void FDECL(relobj, (struct monst *, int, boolean));
E struct obj *FDECL(findgold, (struct obj *));

/* ### steed.c ### */

E void NDECL(rider_cant_reach);
E boolean FDECL(can_saddle, (struct monst *));
E int FDECL(use_saddle, (struct obj *));
E boolean FDECL(can_ride, (struct monst *));
E int NDECL(doride);
E boolean FDECL(mount_steed, (struct monst *, boolean));
E void NDECL(exercise_steed);
E void NDECL(kick_steed);
E void FDECL(dismount_steed, (int));
E void FDECL(place_monster, (struct monst *, int, int));
E boolean FDECL(stucksteed, (boolean));

/* ### teleport.c ### */

E boolean FDECL(goodpos, (int, int, struct monst *, unsigned));
E boolean FDECL(enexto, (coord *, xchar, xchar, struct permonst *));
E boolean
FDECL(enexto_core, (coord *, xchar, xchar, struct permonst *, unsigned));
E void FDECL(teleds, (int, int, boolean));
E boolean FDECL(safe_teleds, (boolean));
E boolean FDECL(teleport_pet, (struct monst *, boolean));
E void NDECL(tele);
E boolean FDECL(scrolltele, (struct obj *));
E int NDECL(dotele);
E void NDECL(level_tele);
E void FDECL(domagicportal, (struct trap *));
E void FDECL(tele_trap, (struct trap *));
E void FDECL(level_tele_trap, (struct trap *));
E void FDECL(rloc_to, (struct monst *, int, int));
E boolean FDECL(rloc, (struct monst *, boolean));
E boolean FDECL(tele_restrict, (struct monst *));
E void FDECL(mtele_trap, (struct monst *, struct trap *, int));
E int FDECL(mlevel_tele_trap,
            (struct monst *, struct trap *, boolean, int));
E boolean FDECL(rloco, (struct obj *));
E int NDECL(random_teleport_level);
E boolean FDECL(u_teleport_mon, (struct monst *, boolean));

/* ### tile.c ### */
#ifdef USE_TILES
E void FDECL(substitute_tiles, (d_level *));
#endif

/* ### timeout.c ### */

E void NDECL(burn_away_slime);
E void NDECL(nh_timeout);
E void FDECL(fall_asleep, (int, boolean));
E void FDECL(attach_egg_hatch_timeout, (struct obj *, long));
E void FDECL(attach_fig_transform_timeout, (struct obj *));
E void FDECL(kill_egg, (struct obj *));
E void FDECL(hatch_egg, (ANY_P *, long));
E void FDECL(learn_egg_type, (int));
E void FDECL(burn_object, (ANY_P *, long));
E void FDECL(begin_burn, (struct obj *, boolean));
E void FDECL(end_burn, (struct obj *, boolean));
E void NDECL(do_storms);
E boolean FDECL(start_timer, (long, short, short, ANY_P *));
E long FDECL(stop_timer, (short, ANY_P *));
E long FDECL(peek_timer, (short, ANY_P *));
E void NDECL(run_timers);
E void FDECL(obj_move_timers, (struct obj *, struct obj *));
E void FDECL(obj_split_timers, (struct obj *, struct obj *));
E void FDECL(obj_stop_timers, (struct obj *));
E boolean FDECL(obj_has_timer, (struct obj *, short));
E void FDECL(spot_stop_timers, (xchar, xchar, short));
E long FDECL(spot_time_expires, (xchar, xchar, short));
E long FDECL(spot_time_left, (xchar, xchar, short));
E boolean FDECL(obj_is_local, (struct obj *));
E void FDECL(save_timers, (int, int, int));
E void FDECL(restore_timers, (int, int, boolean, long));
E void FDECL(relink_timers, (boolean));
E int NDECL(wiz_timeout_queue);
E void NDECL(timer_sanity_check);

/* ### topten.c ### */

E void FDECL(formatkiller, (char *, unsigned, int));
E void FDECL(topten, (int, time_t));
E void FDECL(prscore, (int, char **));
E struct obj *FDECL(tt_oname, (struct obj *));

/* ### track.c ### */

E void NDECL(initrack);
E void NDECL(settrack);
E coord *FDECL(gettrack, (int, int));

/* ### trap.c ### */

E boolean FDECL(burnarmor, (struct monst *));
E int FDECL(erode_obj, (struct obj *, const char *, int, int));
E boolean FDECL(grease_protect, (struct obj *, const char *, struct monst *));
E struct trap *FDECL(maketrap, (int, int, int));
E void FDECL(fall_through, (boolean));
E struct monst *FDECL(animate_statue,
                      (struct obj *, xchar, xchar, int, int *));
E struct monst *FDECL(activate_statue_trap,
                      (struct trap *, xchar, xchar, boolean));
E void FDECL(dotrap, (struct trap *, unsigned));
E void FDECL(seetrap, (struct trap *));
E void FDECL(feeltrap, (struct trap *));
E int FDECL(mintrap, (struct monst *));
E void FDECL(instapetrify, (const char *));
E void FDECL(minstapetrify, (struct monst *, boolean));
E void FDECL(selftouch, (const char *));
E void FDECL(mselftouch, (struct monst *, const char *, boolean));
E void NDECL(float_up);
E void FDECL(fill_pit, (int, int));
E int FDECL(float_down, (long, long));
E void NDECL(climb_pit);
E boolean FDECL(fire_damage, (struct obj *, boolean, xchar, xchar));
E int FDECL(fire_damage_chain,
            (struct obj *, boolean, boolean, xchar, xchar));
E void acid_damage(struct obj *);
E int FDECL(water_damage, (struct obj *, const char *, boolean));
E void FDECL(water_damage_chain, (struct obj *, boolean));
E boolean NDECL(drown);
E void FDECL(drain_en, (int));
E int NDECL(dountrap);
E void FDECL(cnv_trap_obj, (int, int, struct trap *, boolean));
E int FDECL(untrap, (boolean));
E boolean FDECL(openholdingtrap, (struct monst *, boolean *));
E boolean FDECL(closeholdingtrap, (struct monst *, boolean *));
E boolean FDECL(openfallingtrap, (struct monst *, boolean, boolean *));
E boolean FDECL(chest_trap, (struct obj *, int, boolean));
E void FDECL(deltrap, (struct trap *));
E boolean FDECL(delfloortrap, (struct trap *));
E struct trap *FDECL(t_at, (int, int));
E void FDECL(b_trapped, (const char *, int));
E boolean NDECL(unconscious);
E void FDECL(blow_up_landmine, (struct trap *));
E int FDECL(launch_obj, (short, int, int, int, int, int));
E boolean NDECL(launch_in_progress);
E void NDECL(force_launch_placement);
E boolean FDECL(uteetering_at_seen_pit, (struct trap *));
E boolean NDECL(lava_effects);
E void NDECL(sink_into_lava);
E void NDECL(sokoban_guilt);

/* ### u_init.c ### */

E void NDECL(u_init);

/* ### uhitm.c ### */

E void FDECL(erode_armor, (struct monst *, int));
E boolean FDECL(attack_checks, (struct monst *, struct obj *));
E void FDECL(check_caitiff, (struct monst *));
E int FDECL(find_roll_to_hit,
            (struct monst *, uchar, struct obj *, int *, int *));
E boolean FDECL(attack, (struct monst *));
E boolean FDECL(hmon, (struct monst *, struct obj *, int));
E int FDECL(damageum, (struct monst *, struct attack *));
E void FDECL(missum, (struct monst *, struct attack *, boolean));
E int FDECL(passive, (struct monst *, boolean, int, uchar, boolean));
E void FDECL(passive_obj, (struct monst *, struct obj *, struct attack *));
E void FDECL(stumble_onto_mimic, (struct monst *));
E int FDECL(flash_hits_mon, (struct monst *, struct obj *));
E void FDECL(light_hits_gremlin, (struct monst *, int));

/* ### unixmain.c ### */

#ifdef UNIX
#ifdef PORT_HELP
E void NDECL(port_help);
#endif
E void FDECL(sethanguphandler, (void (*)(int)));
E boolean NDECL(authorize_wizard_mode);
E boolean FDECL(check_user_string, (char *));
#endif /* UNIX */

/* ### unixtty.c ### */

#if defined(UNIX) || defined(__BEOS__)
E void NDECL(gettty);
E void FDECL(settty, (const char *));
E void NDECL(setftty);
E void NDECL(intron);
E void NDECL(introff);
E void VDECL(error, (const char *, ...)) PRINTF_F(1, 2);
#endif /* UNIX || __BEOS__ */

/* ### unixunix.c ### */

#ifdef UNIX
E void NDECL(getlock);
E void FDECL(regularize, (char *));
#if defined(TIMED_DELAY) && !defined(msleep) && defined(SYSV)
E void FDECL(msleep, (unsigned));
#endif
#ifdef SHELL
E int NDECL(dosh);
#endif /* SHELL */
#if defined(SHELL) || defined(DEF_PAGER) || defined(DEF_MAILREADER)
E int FDECL(child, (int));
#endif
#ifdef PANICTRACE
E boolean FDECL(file_exists, (const char *));
#endif
#endif /* UNIX */

/* ### unixres.c ### */

#ifdef UNIX
#ifdef GNOME_GRAPHICS
E int FDECL(hide_privileges, (boolean));
#endif
#endif /* UNIX */

/* ### vault.c ### */

E void FDECL(newegd, (struct monst *));
E void FDECL(free_egd, (struct monst *));
E boolean FDECL(grddead, (struct monst *));
E char FDECL(vault_occupied, (char *));
E void NDECL(invault);
E int FDECL(gd_move, (struct monst *));
E void NDECL(paygd);
E long NDECL(hidden_gold);
E boolean NDECL(gd_sound);
E void FDECL(vault_gd_watching, (unsigned int));

/* ### version.c ### */

E char *FDECL(version_string, (char *));
E char *FDECL(getversionstring, (char *));
E int NDECL(doversion);
E int NDECL(doextversion);
#ifdef MICRO
E boolean FDECL(comp_times, (long));
#endif
E boolean
FDECL(check_version, (struct version_info *, const char *, boolean));
E boolean FDECL(uptodate, (int, const char *));
E void FDECL(store_version, (int));
E unsigned long FDECL(get_feature_notice_ver, (char *));
E unsigned long NDECL(get_current_feature_ver);
E const char *FDECL(copyright_banner_line, (int));

#ifdef RUNTIME_PORT_ID
E void FDECL(append_port_id, (char *));
#endif

/* ### video.c ### */

#ifdef MSDOS
E int FDECL(assign_video, (char *));
#ifdef NO_TERMS
E void NDECL(gr_init);
E void NDECL(gr_finish);
#endif
E void FDECL(tileview, (boolean));
#endif
#ifdef VIDEOSHADES
E int FDECL(assign_videoshades, (char *));
E int FDECL(assign_videocolors, (char *));
#endif

/* ### vis_tab.c ### */

#ifdef VISION_TABLES
E void NDECL(vis_tab_init);
#endif

/* ### vision.c ### */

E void NDECL(vision_init);
E int FDECL(does_block, (int, int, struct rm *));
E void NDECL(vision_reset);
E void FDECL(vision_recalc, (int));
E void FDECL(block_point, (int, int));
E void FDECL(unblock_point, (int, int));
E boolean FDECL(clear_path, (int, int, int, int));
E void FDECL(do_clear_area, (int, int, int,
                             void (*)(int, int, genericptr), genericptr_t));
E unsigned FDECL(howmonseen, (struct monst *));

#ifdef VMS

/* ### vmsfiles.c ### */

E int FDECL(vms_link, (const char *, const char *));
E int FDECL(vms_unlink, (const char *));
E int FDECL(vms_creat, (const char *, unsigned int));
E int FDECL(vms_open, (const char *, int, unsigned int));
E boolean FDECL(same_dir, (const char *, const char *));
E int FDECL(c__translate, (int));
E char *FDECL(vms_basename, (const char *));

/* ### vmsmail.c ### */

E unsigned long NDECL(init_broadcast_trapping);
E unsigned long NDECL(enable_broadcast_trapping);
E unsigned long NDECL(disable_broadcast_trapping);
#if 0
E struct mail_info *NDECL(parse_next_broadcast);
#endif /*0*/

/* ### vmsmain.c ### */

E int FDECL(main, (int, char **));
#ifdef CHDIR
E void FDECL(chdirx, (const char *, boolean));
#endif /* CHDIR */
E void FDECL(sethanguphandler, (void (*)(int)));
E boolean NDECL(authorize_wizard_mode);

/* ### vmsmisc.c ### */

E void NDECL(vms_abort) NORETURN;
E void FDECL(vms_exit, (int)) NORETURN;
#ifdef PANICTRACE
E void FDECL(vms_traceback, (int));
#endif

/* ### vmstty.c ### */

E int NDECL(vms_getchar);
E void NDECL(gettty);
E void FDECL(settty, (const char *));
E void FDECL(shuttty, (const char *));
E void NDECL(setftty);
E void NDECL(intron);
E void NDECL(introff);
E void VDECL(error, (const char *, ...)) PRINTF_F(1, 2);
#ifdef TIMED_DELAY
E void FDECL(msleep, (unsigned));
#endif

/* ### vmsunix.c ### */

E void NDECL(getlock);
E void FDECL(regularize, (char *));
E int NDECL(vms_getuid);
E boolean FDECL(file_is_stmlf, (int));
E int FDECL(vms_define, (const char *, const char *, int));
E int FDECL(vms_putenv, (const char *));
E char *NDECL(verify_termcap);
#if defined(CHDIR) || defined(SHELL) || defined(SECURE)
E void NDECL(privoff);
E void NDECL(privon);
#endif
#ifdef SHELL
E int NDECL(dosh);
#endif
#if defined(SHELL) || defined(MAIL)
E int FDECL(vms_doshell, (const char *, boolean));
#endif
#ifdef SUSPEND
E int NDECL(dosuspend);
#endif
#ifdef SELECTSAVED
E int FDECL(vms_get_saved_games, (const char *, char ***));
#endif

#endif /* VMS */

/* ### weapon.c ### */

E const char *FDECL(weapon_descr, (struct obj *));
E int FDECL(hitval, (struct obj *, struct monst *));
E int FDECL(dmgval, (struct obj *, struct monst *));
E struct obj *FDECL(select_rwep, (struct monst *));
E struct obj *FDECL(select_hwep, (struct monst *));
E void FDECL(possibly_unwield, (struct monst *, boolean));
E void FDECL(mwepgone, (struct monst *));
E int FDECL(mon_wield_item, (struct monst *));
E int NDECL(abon);
E int NDECL(dbon);
E void FDECL(wet_a_towel, (struct obj *, int, boolean));
E void FDECL(dry_a_towel, (struct obj *, int, boolean));
E int NDECL(enhance_weapon_skill);
E void FDECL(unrestrict_weapon_skill, (int));
E void FDECL(use_skill, (int, int));
E void FDECL(add_weapon_skill, (int));
E void FDECL(lose_weapon_skill, (int));
E int FDECL(weapon_type, (struct obj *));
E int NDECL(uwep_skill_type);
E int FDECL(weapon_hit_bonus, (struct obj *));
E int FDECL(weapon_dam_bonus, (struct obj *));
E void FDECL(skill_init, (const struct def_skill *));

/* ### were.c ### */

E void FDECL(were_change, (struct monst *));
E int FDECL(counter_were, (int));
E int FDECL(were_beastie, (int));
E void FDECL(new_were, (struct monst *));
E int FDECL(were_summon, (struct permonst *, boolean, int *, char *));
E void NDECL(you_were);
E void FDECL(you_unwere, (boolean));

/* ### wield.c ### */

E void FDECL(setuwep, (struct obj *));
E void FDECL(setuqwep, (struct obj *));
E void FDECL(setuswapwep, (struct obj *));
E int NDECL(dowield);
E int NDECL(doswapweapon);
E int NDECL(dowieldquiver);
E boolean FDECL(wield_tool, (struct obj *, const char *));
E int NDECL(can_twoweapon);
E void NDECL(drop_uswapwep);
E int NDECL(dotwoweapon);
E void NDECL(uwepgone);
E void NDECL(uswapwepgone);
E void NDECL(uqwepgone);
E void NDECL(untwoweapon);
E int FDECL(chwepon, (struct obj *, int));
E int FDECL(welded, (struct obj *));
E void FDECL(weldmsg, (struct obj *));
E void FDECL(setmnotwielded, (struct monst *, struct obj *));
E boolean FDECL(mwelded, (struct obj *));

/* ### windows.c ### */

E void FDECL(choose_windows, (const char *));
#ifdef WINCHAIN
void FDECL(addto_windowchain, (const char *s));
void NDECL(commit_windowchain);
#endif
E boolean NDECL(genl_can_suspend_no);
E boolean NDECL(genl_can_suspend_yes);
E char FDECL(genl_message_menu, (char, int, const char *));
E void FDECL(genl_preference_update, (const char *));
E char *FDECL(genl_getmsghistory, (boolean));
E void FDECL(genl_putmsghistory, (const char *, boolean));
#ifdef HANGUPHANDLING
E void NDECL(nhwindows_hangup);
#endif
#ifdef STATUS_VIA_WINDOWPORT
E void NDECL(genl_status_init);
E void NDECL(genl_status_finish);
E void FDECL(genl_status_enablefield,
             (int, const char *, const char *, boolean));
E void FDECL(genl_status_update, (int, genericptr_t, int, int));
#ifdef STATUS_HILITES
E void FDECL(genl_status_threshold, (int, int, anything, int, int, int));
#endif
#endif

/* ### wizard.c ### */

E void NDECL(amulet);
E int FDECL(mon_has_amulet, (struct monst *));
E int FDECL(mon_has_special, (struct monst *));
E int FDECL(tactics, (struct monst *));
E void NDECL(aggravate);
E void NDECL(clonewiz);
E int NDECL(pick_nasty);
E int FDECL(nasty, (struct monst *));
E void NDECL(resurrect);
E void NDECL(intervene);
E void NDECL(wizdead);
E void FDECL(cuss, (struct monst *));

/* ### worm.c ### */

E int NDECL(get_wormno);
E void FDECL(initworm, (struct monst *, int));
E void FDECL(worm_move, (struct monst *));
E void FDECL(worm_nomove, (struct monst *));
E void FDECL(wormgone, (struct monst *));
E void FDECL(wormhitu, (struct monst *));
E void FDECL(cutworm, (struct monst *, xchar, xchar, struct obj *));
E void FDECL(see_wsegs, (struct monst *));
E void FDECL(detect_wsegs, (struct monst *, boolean));
E void FDECL(save_worm, (int, int));
E void FDECL(rest_worm, (int));
E void FDECL(place_wsegs, (struct monst *));
E void FDECL(remove_worm, (struct monst *));
E void FDECL(place_worm_tail_randomly, (struct monst *, xchar, xchar));
E int FDECL(count_wsegs, (struct monst *));
E boolean FDECL(worm_known, (struct monst *));
E boolean FDECL(worm_cross, (int, int, int, int));

/* ### worn.c ### */

E void FDECL(setworn, (struct obj *, long));
E void FDECL(setnotworn, (struct obj *));
E long FDECL(wearslot, (struct obj *));
E void FDECL(mon_set_minvis, (struct monst *));
E void FDECL(mon_adjust_speed, (struct monst *, int, struct obj *));
E void FDECL(update_mon_intrinsics,
             (struct monst *, struct obj *, boolean, boolean));
E int FDECL(find_mac, (struct monst *));
E void FDECL(m_dowear, (struct monst *, boolean));
E struct obj *FDECL(which_armor, (struct monst *, long));
E void FDECL(mon_break_armor, (struct monst *, boolean));
E void FDECL(bypass_obj, (struct obj *));
E void NDECL(clear_bypasses);
E void FDECL(bypass_objlist, (struct obj *, boolean));
E struct obj *FDECL(nxt_unbypassed_obj, (struct obj *));
E int FDECL(racial_exception, (struct monst *, struct obj *));

/* ### write.c ### */

E int FDECL(dowrite, (struct obj *));

/* ### zap.c ### */

E void FDECL(learnwand, (struct obj *));
E int FDECL(bhitm, (struct monst *, struct obj *));
E void FDECL(probe_monster, (struct monst *));
E boolean FDECL(get_obj_location, (struct obj *, xchar *, xchar *, int));
E boolean FDECL(get_mon_location, (struct monst *, xchar *, xchar *, int));
E struct monst *FDECL(get_container_location,
                      (struct obj * obj, int *, int *));
E struct monst *FDECL(montraits, (struct obj *, coord *));
E struct monst *FDECL(revive, (struct obj *, boolean));
E int FDECL(unturn_dead, (struct monst *));
E void FDECL(cancel_item, (struct obj *));
E boolean FDECL(drain_item, (struct obj *));
E struct obj *FDECL(poly_obj, (struct obj *, int));
E boolean FDECL(obj_resists, (struct obj *, int, int));
E boolean FDECL(obj_shudders, (struct obj *));
E void FDECL(do_osshock, (struct obj *));
E int FDECL(bhito, (struct obj *, struct obj *));
E int FDECL(bhitpile,
            (struct obj *, int (*)(OBJ_P, OBJ_P), int, int, schar));
E int FDECL(zappable, (struct obj *));
E void FDECL(zapnodir, (struct obj *));
E int NDECL(dozap);
E int FDECL(zapyourself, (struct obj *, boolean));
E void FDECL(ubreatheu, (struct attack *));
E int FDECL(lightdamage, (struct obj *, boolean, int));
E boolean FDECL(flashburn, (long));
E boolean FDECL(cancel_monst, (struct monst *, struct obj *, boolean,
                               boolean, boolean));
E void NDECL(zapsetup);
E void NDECL(zapwrapup);
E void FDECL(weffects, (struct obj *));
E int FDECL(spell_damage_bonus, (int));
E const char *FDECL(exclam, (int force));
E void FDECL(hit, (const char *, struct monst *, const char *));
E void FDECL(miss, (const char *, struct monst *));
E struct monst *FDECL(bhit, (int, int, int, int, int (*)(MONST_P, OBJ_P),
                             int (*)(OBJ_P, OBJ_P), struct obj **));
E struct monst *FDECL(boomhit, (struct obj *, int, int));
E int FDECL(zhitm, (struct monst *, int, int, struct obj **));
E int FDECL(burn_floor_objects, (int, int, boolean, boolean));
E void FDECL(buzz, (int, int, xchar, xchar, int, int));
E void FDECL(melt_ice, (xchar, xchar, const char *));
E void FDECL(start_melt_ice_timeout, (xchar, xchar, long));
E void FDECL(melt_ice_away, (ANY_P *, long));
E int FDECL(zap_over_floor, (xchar, xchar, int, boolean *, short));
E void FDECL(fracture_rock, (struct obj *));
E boolean FDECL(break_statue, (struct obj *));
E void FDECL(destroy_item, (int, int));
E int FDECL(destroy_mitem, (struct monst *, int, int));
E int FDECL(resist, (struct monst *, char, int, int));
E void NDECL(makewish);

#endif /* !MAKEDEFS_C && !LEV_LEX_C */

#undef E

#endif /* EXTERN_H */
