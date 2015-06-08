/* NetHack 3.6	trampoli.h	$NHDT-Date: 1433806581 2015/06/08 23:36:21 $  $NHDT-Branch: master $:$NHDT-Revision: 1.11 $ */
/* Copyright (c) 1989, by Norm Meluch and Stephen Spackman	  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef TRAMPOLI_H
#define TRAMPOLI_H

#ifdef USE_TRAMPOLI

/* ### apply.c ### */
#define dig() dig_()
#define doapply() doapply_()
#define dojump() dojump_()
#define dorub() dorub_()

/* ### artifact.c ### */
#define doinvoke() doinvoke_()

/* ### cmd.c ### */
#define doextcmd() doextcmd_()
#define doextlist() doextlist_()
#define domonability() domonability_()
#define enter_explore_mode() enter_explore_mode_()
#define doprev_message() doprev_message_()
#define timed_occupation() timed_occupation_()
#define wiz_attributes() wiz_attributes_()
#define wiz_detect() wiz_detect_()
#define wiz_genesis() wiz_genesis_()
#define wiz_identify() wiz_identify_()
#define wiz_level_tele() wiz_level_tele_()
#define wiz_map() wiz_map_()
#define wiz_where() wiz_where_()
#define wiz_wish() wiz_wish_()

/* ### display.c ### */
#define doredraw() doredraw_()

/* ### do.c ### */
#define doddrop() doddrop_()
#define dodown() dodown_()
#define dodrop() dodrop_()
#define donull() donull_()
#define doup() doup_()
#define dowipe() dowipe_()
#define drop(x) drop_(x)
#define wipeoff() wipeoff_()

/* ### do_name.c ### */
#define ddocall() ddocall_()
#define do_mname() do_mname_()

/* ### do_wear.c ### */
#define Armor_off() Armor_off_()
#define Boots_off() Boots_off_()
#define Gloves_off() Gloves_off_()
#define Helmet_off() Helmet_off_()
#define Armor_on() Armor_on_()
#define Boots_on() Boots_on_()
#define Gloves_on() Gloves_on_()
#define Helmet_on() Helmet_on_()
#define doddoremarm() doddoremarm_()
#define doputon() doputon_()
#define doremring() doremring_()
#define dotakeoff() dotakeoff_()
#define dowear() dowear_()
#define select_off(x) select_off_(x)
#define take_off() take_off_()

/* ### dogmove.c ### */
#define wantdoor(x, y, dummy) wantdoor_(x, y, dummy)

/* ### dokick.c ### */
#define dokick() dokick_()

/* ### dothrow.c ### */
#define dothrow() dothrow_()

/* ### eat.c ### */
#define Hear_again() Hear_again_()
#define eatmdone() eatmdone_()
#define doeat() doeat_()
#define eatfood() eatfood_()
#define opentin() opentin_()
#define unfaint() unfaint_()

/* ### end.c ### */
#define done1(sig) done1_(sig)
#define done2() done2_()
#define done_intr(sig) done_intr_(sig)
#if defined(UNIX) || defined(VMS) || defined(__EMX__)
#define done_hangup(sig) done_hangup_(sig)
#endif

/* ### engrave.c ### */
#define doengrave() doengrave_()

/* ### fountain.c ### */
#define gush(x, y, poolcnt) gush_(x, y, poolcnt)

/* ### hack.c ### */
#define dopickup() dopickup_()
#define identify(x) identify_(x)

/* ### invent.c ### */
#define ckunpaid(x) ckunpaid_(x)
#define ddoinv() ddoinv_()
#define dolook() dolook_()
#define dopramulet() dopramulet_()
#define doprarm() doprarm_()
#define doprgold() doprgold_()
#define doprring() doprring_()
#define doprtool() doprtool_()
#define doprwep() doprwep_()
#define dotypeinv() dotypeinv_()
#define doorganize() doorganize_()

/* ### ioctl.c ### */
#ifdef UNIX
#ifdef SUSPEND
#define dosuspend() dosuspend_()
#endif /* SUSPEND */
#endif /* UNIX */

/* ### lock.c ### */
#define doclose() doclose_()
#define doforce() doforce_()
#define doopen() doopen_()
#define forcelock() forcelock_()
#define picklock() picklock_()

/* ### mklev.c ### */
#define do_comp(x, y) comp_(x, y)

/* ### mondata.c ### */
/* See comment in trampoli.c before uncommenting canseemon. */
/* #define canseemon(x) canseemon_(x) */

/* ### muse.c ### */
#define mbhitm(x, y) mbhitm_(x, y)

/* ### o_init.c ### */
#define dodiscovered() dodiscovered_()

/* ### objnam.c ### */
#define doname(x) doname_(x)
#define xname(x) xname_(x)

/* ### options.c ### */
#define doset() doset_()
#define dotogglepickup() dotogglepickup_()

/* ### pager.c ### */
#define dohelp() dohelp_()
#define dohistory() dohistory_()
#ifdef UNIX
#define intruph() intruph_()
#endif /* UNIX */
#define dowhatdoes() dowhatdoes_()
#define dowhatis() dowhatis_()
#define doquickwhatis() doquickwhatis_()

/* ### pcsys.c ### */
#ifdef SHELL
#define dosh() dosh_()
#endif /* SHELL */

/* ### pickup.c ### */
#define ck_bag(x) ck_bag_(x)
#define doloot() doloot_()
#define in_container(x) in_container_(x)
#define out_container(x) out_container_(x)

/* ### potion.c ### */
#define dodrink() dodrink_()
#define dodip() dodip_()

/* ### pray.c ### */
#define doturn() doturn_()
#define dopray() dopray_()
#define prayer_done() prayer_done_()
#define dosacrifice() dosacrifice_()

/* ### read.c ### */
#define doread() doread_()
#define set_lit(x, y, val) set_lit_(x, y, val)

/* ### rip.c ### */
#define genl_outrip(tmpwin, how) genl_outrip_(tmpwin, how)

/* ### save.c ### */
#define dosave() dosave_()
#if defined(UNIX) || defined(VMS) || defined(__EMX__)
#define hangup(sig) hangup_(sig)
#endif

/* ### search.c ### */
#define doidtrap() doidtrap_()
#define dosearch() dosearch_()
#define findone(zx, zy, num) findone_(zx, zy, num)
#define openone(zx, zy, num) openone_(zx, zy, num)

/* ### shk.c ### */
#define dopay() dopay_()

/* ### sit.c ### */
#define dosit() dosit_()

/* ### sounds.c ### */
#define dotalk() dotalk_()

/* ### spell.c ### */
#define learn() learn_()
#define docast() docast_()
#define dovspell() dovspell_()

/* ### steal.c ### */
#define stealarm() stealarm_()

/* ### trap.c ### */
#define dotele() dotele_()
#define dountrap() dountrap_()
#define float_down() float_down_()

/* ### version.c ### */
#define doversion() doversion_()
#define doextversion() doextversion_()

/* ### wield.c ### */
#define dowield() dowield_()

/* ### zap.c ### */
#define bhitm(x, y) bhitm_(x, y)
#define bhito(x, y) bhito_(x, y)
#define dozap() dozap_()

/* ### getline.c ### */
#define tty_getlin(x, y) tty_getlin_(x, y)
#define tty_get_ext_cmd() tty_get_ext_cmd_()

/* ### termcap.c ### */
#define tty_nhbell() tty_nhbell_()
#define tty_number_pad(x) tty_number_pad_(x)
#define tty_delay_output() tty_delay_output_()
#define tty_start_screen() tty_start_screen_()
#define tty_end_screen() tty_end_screen_()

/* ### topl.c ### */
#define tty_doprev_message() tty_doprev_message_()
#define tty_yn_function(x, y, z) tty_yn_function_(x, y, z)

/* ### wintty.c ### */
#define tty_init_nhwindows(x, y) tty_init_nhwindows_(x, y)
#define tty_player_selection() tty_player_selection_()
#define tty_askname() tty_askname_()
#define tty_get_nh_event() tty_get_nh_event_()
#define tty_exit_nhwindows(x) tty_exit_nhwindows_(x)
#define tty_suspend_nhwindows(x) tty_suspend_nhwindows_(x)
#define tty_resume_nhwindows() tty_resume_nhwindows_()
#define tty_create_nhwindow(x) tty_create_nhwindow_(x)
#define tty_clear_nhwindow(x) tty_clear_nhwindow_(x)
#define tty_display_nhwindow(x, y) tty_display_nhwindow_(x, y)
#define tty_destroy_nhwindow(x) tty_destroy_nhwindow_(x)
#define tty_curs(x, y, z) tty_curs_(x, y, z)
#define tty_putstr(x, y, z) tty_putstr_(x, y, z)
#define tty_display_file(x, y) tty_display_file_(x, y)
#define tty_start_menu(x) tty_start_menu_(x)
#define tty_add_menu(a, b, c, d, e, f, g, h) \
    tty_add_menu_(a, b, c, d, e, f, g, h)
#define tty_end_menu(a, b) tty_end_menu_(a, b)
#define tty_select_menu(a, b, c) tty_select_menu_(a, b, c)
#define tty_update_inventory() tty_update_inventory_()
#define tty_mark_synch() tty_mark_synch_()
#define tty_wait_synch() tty_wait_synch_()
#ifdef CLIPPING
#define tty_cliparound(x, y) tty_cliparound_(x, y)
#endif
#ifdef POSITIONBAR
#define tty_update_positionbar(x) tty_update_positionbar_(x)
#endif
#define tty_print_glyph(a, b, c, d, e) tty_print_glyph_(a, b, c, d, e)
#define tty_raw_print(x) tty_raw_print_(x)
#define tty_raw_print_bold(x) tty_raw_print_bold_(x)
#define tty_nhgetch() tty_nhgetch_()
#define tty_nh_poskey(x, y, z) tty_nh_poskey_(x, y, z)

#endif /* USE_TRAMPOLI */

#endif /* TRAMPOLI_H */
