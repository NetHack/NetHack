/* NetHack 3.7	optlist.h */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef OPTLIST_H
#define OPTLIST_H

/*
 *  NOTE:  If you add (or delete) an option, please review:
 *             doc/options.txt
 *
 *         It contains how-to info and outlines some required/suggested
 *         updates that should accompany your change.
 */

static int optfn_boolean(int, int, boolean, char *, char *);
enum OptType {BoolOpt, CompOpt, OthrOpt};
enum Y_N {No, Yes};
enum Off_On {Off, On};

struct allopt_t {
    const char *name;
    int minmatch;
    int expectedbuf;
    int idx;
    enum optset_restrictions setwhere;
    enum OptType opttyp;
    enum Y_N negateok;
    enum Y_N valok;
    enum Y_N dupeok;
    enum Y_N pfx;
    boolean opt_in_out, *addr;
    int (*optfn)(int, int, boolean, char *, char *);
    const char *alias;
    const char *descr;
    const char *prefixgw;
    boolean initval, has_handler, dupdetected;
};

#endif /* OPTLIST_H */

#if defined(NHOPT_PROTO) || defined(NHOPT_ENUM) || defined(NHOPT_PARSE)

#define NoAlias ((const char *) 0)

#if defined(NHOPT_PROTO)
#define NHOPTB(a, b, c, s, i, n, v, d, al, bp)
#define NHOPTC(a, b, c, s, n, v, d, h, al, z) \
static int optfn_##a(int, int, boolean, char *, char *);
#define NHOPTP(a, b, c, s, n, v, d, h, al, z) \
static int pfxfn_##a(int, int, boolean, char *, char *);
#define NHOPTO(m, a, b, c, s, n, v, d, al, z) \
static int optfn_##a(int, int, boolean, char *, char *);

#elif defined(NHOPT_ENUM)
#define NHOPTB(a, b, c, s, i, n, v, d, al, bp) \
opt_##a,
#define NHOPTC(a, b, c, s, n, v, d, h, al, z) \
opt_##a,
#define NHOPTP(a, b, c, s, n, v, d, h, al, z) \
pfx_##a,
#define NHOPTO(m, a, b, c, s, n, v, d, al, z) \
opt_##a,

#elif defined(NHOPT_PARSE)
#define NHOPTB(a, b, c, s, i, n, v, d, al, bp) \
{ #a, 0, b, opt_##a, s, BoolOpt, n, v, d, No, c, bp, &optfn_boolean, \
 al, (const char *) 0, (const char *) 0, i, 0, 0 },
#define NHOPTC(a, b, c, s, n, v, d, h, al, z) \
{ #a, 0, b, opt_##a, s, CompOpt, n, v, d, No, c, (boolean *) 0, &optfn_##a, \
 al, z, (const char *) 0, Off, h, 0 },
#define NHOPTP(a, b, c, s, n, v, d, h, al, z) \
{ #a, 0, b, pfx_##a, s, CompOpt, n, v, d, Yes, c, (boolean *) 0, &pfxfn_##a, \
 al, z, #a, Off, h, 0 },
#define NHOPTO(m, a, b, c, s, n, v, d, al, z) \
{ m, 0, b, opt_##a, s, OthrOpt, n, v, d, No, c, (boolean *) 0, &optfn_##a, \
 al, z, (const char *) 0, On, On, 0 },

#ifdef USE_TILES
#define tiled_map_Def On
#define ascii_map_Def Off
#else
#define ascii_map_Def On
#define tiled_map_Def Off
#endif
#endif

/* B:nm, ln, opt_*, setwhere?, on?, negat?, val?, dup?, hndlr? Alias, bool_p */
/* C:nm, ln, opt_*, setwhere?, negateok?, valok?, dupok?, hndlr? Alias, desc */
/* P:pfx, ln, opt_*, setwhere?, negateok?, valok?, dupok?, hndlr? Alias, desc*/

    NHOPTB(acoustics, 0, opt_out, set_in_game, On, Yes, No, No, NoAlias,
               &flags.acoustics)
    NHOPTC(align, 8, opt_in, set_gameview, No, Yes, No, No, NoAlias,
               "your starting alignment (lawful, neutral, or chaotic)")
    NHOPTC(align_message, 20, opt_in, set_gameview, Yes, Yes, No, Yes, NoAlias,
               "message window alignment")
    NHOPTC(align_status, 20, opt_in, set_gameview, No, Yes, No, Yes, NoAlias,
               "status window alignment")
#ifdef WIN32
    NHOPTC(altkeyhandling, 20, opt_in, set_in_game, No, Yes, No, Yes,
               "altkeyhandler", "alternative key handling")
#else
    NHOPTC(altkeyhandling, 20, opt_in, set_in_config, No, Yes, No, Yes,
               "altkeyhandler", "(not applicable)")
#endif
#ifdef ALTMETA
    NHOPTB(altmeta, 0, opt_out, set_in_game, Off, Yes, No, No, NoAlias,
               &iflags.altmeta)
#else
    NHOPTB(altmeta, 0, opt_out, set_in_config, Off, Yes, No, No, NoAlias,
               (boolean *) 0)
#endif
    NHOPTB(ascii_map, 0, opt_in, set_in_game, ascii_map_Def, Yes, No, No,
                NoAlias, &iflags.wc_ascii_map)
    NHOPTB(autodescribe, 0, opt_out, set_in_game, On, Yes, No, No, NoAlias,
                &iflags.autodescribe)
    NHOPTB(autodig, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &flags.autodig)
    NHOPTB(autoopen, 0, opt_out, set_in_game, On, Yes, No, No, NoAlias,
                &flags.autoopen)
    NHOPTB(autopickup, 0, opt_out, set_in_game, Off, Yes, No, No, NoAlias,
                &flags.pickup)
    NHOPTO("autopickup exceptions", o_autopickup_exceptions, BUFSZ, opt_in,
           set_in_game, No, Yes, No, NoAlias, "edit autopickup exceptions")
    NHOPTB(autoquiver, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &flags.autoquiver)
    NHOPTC(autounlock,
                (sizeof "none" + sizeof "untrap" + sizeof "apply-key"
                 + sizeof "kick" + sizeof "force" + 20),
                opt_out, set_in_game, Yes, Yes, No, Yes, NoAlias,
                "action to take when encountering locked door or chest")
#if defined(MICRO) && !defined(AMIGA)
    NHOPTB(BIOS, 0, opt_in, set_in_config, Off, Yes, No, No, NoAlias,
                &iflags.BIOS)
#else
    NHOPTB(BIOS, 0, opt_in, set_in_config, Off, No, No, No, NoAlias,
                (boolean *) 0)
#endif
    NHOPTB(blind, 0, opt_in, set_in_config, Off, Yes, No, No, NoAlias,
                &u.uroleplay.blind)
    NHOPTB(bones, 0, opt_out, set_in_config, On, Yes, No, No, NoAlias,
                &flags.bones)
#ifdef BACKWARD_COMPAT
    NHOPTC(boulder, 1, opt_in, set_in_game , No, Yes, No, No, NoAlias,
                "deprecated (use S_boulder in sym file instead)")
#endif
    NHOPTC(catname, PL_PSIZ, opt_in, set_gameview, No, Yes, No, No, NoAlias,
                "name of your starting pet if it is a kitten")
#ifdef INSURANCE
    NHOPTB(checkpoint, 0, opt_out, set_in_game, On, Yes, No, No, NoAlias,
                &flags.ins_chkpt)
#else
    NHOPTB(checkpoint, 0, opt_out, set_in_game, Off, No, No, No, NoAlias,
                (boolean *) 0)
#endif
    NHOPTB(clicklook, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &iflags.clicklook)
    NHOPTB(cmdassist, 0, opt_out, set_in_game, On, Yes, No, No, NoAlias,
                &iflags.cmdassist)
    NHOPTB(color, 0, opt_in, set_in_game, On, Yes, No, No, "colour",
                &iflags.wc_color)
    NHOPTB(confirm, 0, opt_out, set_in_game, On, Yes, No, No, NoAlias,
                &flags.confirm)
#ifdef CURSES_GRAPHICS
    NHOPTC(cursesgraphics, 70, opt_in, set_in_config, No, Yes, No, No, NoAlias,
                "load curses display symbols")
#endif
    NHOPTB(dark_room, 0, opt_out, set_in_game, On, Yes, No, No, NoAlias,
                &flags.dark_room)
#ifdef BACKWARD_COMPAT
    NHOPTC(DECgraphics, 70, opt_in, set_in_config, Yes, Yes, No, No, NoAlias,
                "load DECGraphics display symbols")
#endif
    NHOPTC(disclose, sizeof flags.end_disclose * 2,
                opt_in, set_in_game, Yes, Yes, No, Yes, NoAlias,
                "the kinds of information to disclose at end of game")
    NHOPTC(dogname, PL_PSIZ, opt_in, set_gameview, No, Yes, No, No, NoAlias,
                "name of your starting pet if it is a little dog")
    NHOPTC(dungeon, MAXDCHARS + 1,opt_in, set_in_config, No, Yes, No, No,
                NoAlias, "list of symbols to use in drawing the dungeon map")
    NHOPTC(effects, MAXECHARS + 1, opt_in, set_in_config, No, Yes, No, No,
                NoAlias, "list of symbols to use in drawing special effects")
    NHOPTB(eight_bit_tty, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &iflags.wc_eight_bit_input)
    NHOPTB(extmenu, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &iflags.extmenu)
    NHOPTB(female, 0, opt_in, set_in_config, Off, Yes, No, No, "male",
                &flags.female)
    NHOPTB(fireassist, 0, opt_out, set_in_game, On, Yes, No, No, NoAlias,
                &iflags.fireassist)
    NHOPTB(fixinv, 0, opt_out, set_in_game, On, Yes, No, No, NoAlias,
                &flags.invlet_constant)
    NHOPTC(font_map, 40, opt_in, set_gameview, Yes, Yes, Yes, No, NoAlias,
                "font to use in the map window")
    NHOPTC(font_menu, 40, opt_in, set_gameview, Yes, Yes, Yes, No, NoAlias,
                "font to use in menus")
    NHOPTC(font_message, 40, opt_in, set_gameview, Yes, Yes, Yes, No, NoAlias,
                "font to use in the message window")
    NHOPTC(font_size_map, 20, opt_in, set_gameview, Yes, Yes, Yes, No, NoAlias,
                "size of the map font")
    NHOPTC(font_size_menu, 20, opt_in, set_gameview, Yes, Yes, Yes, No,
                NoAlias, "size of the menu font")
    NHOPTC(font_size_message, 20, opt_in, set_gameview, Yes, Yes, Yes, No,
                NoAlias, "size of the message font")
    NHOPTC(font_size_status, 20, opt_in, set_gameview, Yes, Yes, Yes, No,
                NoAlias, "size of the status font")
    NHOPTC(font_size_text, 20, opt_in, set_gameview, Yes, Yes, Yes, No,
                NoAlias, "size of the text font")
    NHOPTC(font_status, 40, opt_in, set_gameview, Yes, Yes, Yes, No, NoAlias,
                "font to use in status window")
    NHOPTC(font_text, 40, opt_in, set_gameview, Yes, Yes, Yes, No, NoAlias,
                "font to use in text windows")
    NHOPTB(force_invmenu, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &iflags.force_invmenu)
    NHOPTC(fruit, PL_FSIZ, opt_in, set_in_game, No, Yes, No, No, NoAlias,
                "name of a fruit you enjoy eating")
    NHOPTB(fullscreen, 0, opt_in, set_in_config, Off, Yes, No, No, NoAlias,
                &iflags.wc2_fullscreen)
    NHOPTC(gender, 8, opt_in, set_gameview, No, Yes, No, No, NoAlias,
                "your starting gender (male or female)")
    NHOPTC(glyph, 40, opt_in, set_in_game, No, Yes, Yes, No, NoAlias,
                "set representation of a glyph to a unicode value and color")
    NHOPTB(goldX, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &flags.goldX)
    NHOPTB(guicolor, 0, opt_out, set_in_game, On, Yes, No, No,  NoAlias,
                &iflags.wc2_guicolor)
    NHOPTB(help, 0, opt_out, set_in_game, On, Yes, No, No, NoAlias,
                &flags.help)
    NHOPTB(herecmd_menu, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &iflags.herecmd_menu)
#if defined(MAC)
    NHOPTC(hicolor, 15, opt_in, set_in_config, No, Yes, No, No, NoAlias,
                "same as palette, only order is reversed")
#endif
    NHOPTB(hilite_pet, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &iflags.wc_hilite_pet)
    NHOPTB(hilite_pile, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &iflags.hilite_pile)
#ifdef STATUS_HILITES
    NHOPTC(hilite_status, 13, opt_out, set_in_game, Yes, Yes, Yes, No, NoAlias,
                "a status highlighting rule (can occur multiple times)")
#else
    NHOPTC(hilite_status, 13, opt_out, set_in_config, Yes, Yes, Yes, No,
                NoAlias, "(not available)")
#endif
    NHOPTB(hitpointbar, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &iflags.wc2_hitpointbar)
    NHOPTC(horsename, PL_PSIZ, opt_in, set_gameview, No, Yes, No, No, NoAlias,
                "name of your starting pet if it is a pony")
#ifdef BACKWARD_COMPAT
    NHOPTC(IBMgraphics, 70, opt_in, set_in_config, Yes, Yes, No, No, NoAlias,
                "load IBMGraphics display symbols")
#endif
#ifndef MAC
    NHOPTB(ignintr, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &flags.ignintr)
#else
    NHOPTB(ignintr, 0, opt_in, set_in_config, Off, Yes, No, No, NoAlias,
                (boolean *) 0)
#endif
    NHOPTB(implicit_uncursed, 0, opt_out, set_in_game, On, Yes, No, No,
                NoAlias, &flags.implicit_uncursed)
#if 0   /* obsolete - pre-OSX Mac */
    NHOPTB(large_font, 0, opt_in, set_in_config, Off, Yes, No, No, NoAlias,
                &iflags.obsolete)
#endif
    NHOPTB(legacy, 0, opt_out, set_in_config, On, Yes, No, No, NoAlias,
                &flags.legacy)
    NHOPTB(lit_corridor, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &flags.lit_corridor)
    NHOPTB(lootabc, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &flags.lootabc)
#if defined(BACKWARD_COMPAT) && defined(MAC_GRAPHICS_ENV)
    NHOPTC(Macgraphics, 70, opt_in, set_in_config, No, Yes, No, No, NoAlias,
                "load MACGraphics display symbols")
#endif
    NHOPTB(mail, 0, opt_out, set_in_game, On, Yes, No, No, NoAlias,
                &flags.biff)
    NHOPTC(map_mode, 20, opt_in, set_gameview, Yes, Yes, No, No, NoAlias,
                "map display mode under Windows")
    NHOPTB(mention_decor, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &flags.mention_decor)
    NHOPTB(mention_walls, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &flags.mention_walls)
    NHOPTC(menu_deselect_all, 4, opt_in, set_in_config, No, Yes, No, No,
                NoAlias, "deselect all items in a menu")
    NHOPTC(menu_deselect_page, 4, opt_in, set_in_config, No, Yes, No, No,
                NoAlias, "deselect all items on this page of a menu")
    NHOPTC(menu_first_page, 4, opt_in, set_in_config, No, Yes, No, No,
                NoAlias, "jump to the first page in a menu")
    NHOPTC(menu_headings, 4, opt_in, set_in_game, No, Yes, No, Yes, NoAlias,
                "display style for menu headings")
    NHOPTC(menu_invert_all, 4, opt_in, set_in_config, No, Yes, No, No, NoAlias,
                "invert all items in a menu")
    NHOPTC(menu_invert_page, 4, opt_in, set_in_config, No, Yes, No, No,
                NoAlias, "invert all items on this page of a menu")
    NHOPTC(menu_last_page, 4, opt_in, set_in_config, No, Yes, No, No, NoAlias,
                "jump to the last page in a menu")
    NHOPTC(menu_next_page, 4, opt_in, set_in_config, No, Yes, No, No, NoAlias,
                "go to the next menu page")
    NHOPTB(menu_objsyms, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &iflags.menu_head_objsym)
#ifdef TTY_GRAPHICS
    NHOPTB(menu_overlay, 0, opt_in, set_in_game, On, Yes, No, No, NoAlias,
                &iflags.menu_overlay)
#else
    NHOPTB(menu_overlay, 0, opt_in, set_in_config, Off, No, No, No, NoAlias,
                (boolean *) 0)
#endif
    NHOPTC(menu_previous_page, 4, opt_in, set_in_config, No, Yes, No, No,
                NoAlias, "go to the previous menu page")
    NHOPTC(menu_search, 4, opt_in, set_in_config, No, Yes, No, No, NoAlias,
                "search for a menu item")
    NHOPTC(menu_select_all, 4, opt_in, set_in_config, No, Yes, No, No, NoAlias,
                "select all items in a menu")
    NHOPTC(menu_select_page, 4, opt_in, set_in_config, No, Yes, No, No,
                NoAlias, "select all items on this page of a menu")
    NHOPTC(menu_shift_left, 4, opt_in, set_in_config, No, Yes, No, No,
                NoAlias, "pan current menu page left")
    NHOPTC(menu_shift_right, 4, opt_in, set_in_config, No, Yes, No, No,
                NoAlias, "pan current menu page right")
    NHOPTB(menu_tab_sep, 0, opt_in, set_wizonly, Off, Yes, No, No, NoAlias,
                &iflags.menu_tab_sep)
    NHOPTB(menucolors, 0, opt_in, set_in_game, Off, Yes, Yes, No, NoAlias,
                &iflags.use_menu_color)
    NHOPTO("menu colors", o_menu_colors, BUFSZ, opt_in, set_in_game,
           No, Yes, No, NoAlias, "edit menu colors")
    NHOPTC(menuinvertmode, 5, opt_in, set_in_game, No, Yes, No, No, NoAlias,
                "experimental behaviour of menu inverts")
    NHOPTC(menustyle, MENUTYPELEN, opt_in, set_in_game, Yes, Yes, No, Yes,
                NoAlias, "user interface for object selection")
    NHOPTO("message types", o_message_types, BUFSZ, opt_in, set_in_game,
           No, Yes, No, NoAlias, "edit message types")
    NHOPTB(monpolycontrol, 0, opt_in, set_wizonly, Off, Yes, No, No, NoAlias,
                &iflags.mon_polycontrol)
    NHOPTC(monsters, MAXMCLASSES, opt_in, set_in_config, No, Yes, No, No,
                NoAlias, "list of symbols to use for monsters")
    NHOPTC(mouse_support, 0, opt_in, set_in_game, No, Yes, No, No, NoAlias,
                "game receives click info from mouse")
#if PREV_MSGS /* tty or curses */
    NHOPTC(msg_window, 1, opt_in, set_in_game, Yes, Yes, No, Yes, NoAlias,
                "control of \"view previous message(s)\" (^P) behavior")
#else
    NHOPTC(msg_window, 1, opt_in, set_in_config, Yes, Yes, No, Yes, NoAlias,
                "control of \"view previous message(s)\" (^P) behavior")
#endif
    NHOPTC(msghistory, 5, opt_in, set_gameview, Yes, Yes, No, No, NoAlias,
                "number of top line messages to save")
    NHOPTC(name, PL_NSIZ, opt_in, set_gameview, No, Yes, No, No, NoAlias,
                "your character's name (e.g., name:Merlin-W)")
#ifdef NEWS
    NHOPTB(news, 0, opt_in, set_in_config, Off, Yes, No, No, NoAlias,
                &iflags.news)
#else
    NHOPTB(news, 0, opt_in, set_in_config, Off, No, No, No, NoAlias,
                (boolean *) 0)
#endif
    NHOPTB(nudist, 0, opt_in, set_in_config, Off, Yes, No, No, NoAlias,
                &u.uroleplay.nudist)
    NHOPTB(null, 0, opt_out, set_in_game, On, Yes, No, No, NoAlias,
                &flags.null)
    NHOPTC(number_pad, 1, opt_in, set_in_game, No, Yes, No, Yes, NoAlias,
                "use the number pad for movement")
    NHOPTC(objects, MAXOCLASSES, opt_in, set_in_config, No, Yes, No, No,
                NoAlias, "list of symbols to use for objects")
    NHOPTC(packorder, MAXOCLASSES, opt_in, set_in_game, No, Yes, No, No,
                NoAlias, "the inventory order of the items in your pack")
#ifdef CHANGE_COLOR
#ifndef WIN32
    NHOPTC(palette, 15, opt_in, set_in_game, No, Yes, No, No, "hicolor",
                "palette (00c/880/-fff is blue/yellow/reverse white)")
#else
    NHOPTC(palette, 15, opt_in, set_in_config, No, Yes, No, No, "hicolor",
                "palette (adjust an RGB color in palette (color-R-G-B)")
#endif
#endif
    NHOPTC(paranoid_confirmation, 28, opt_in, set_in_game, Yes, Yes, Yes, Yes,
                "prayconfirm", "extra prompting in certain situations")
    NHOPTB(perm_invent, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &iflags.perm_invent)
    NHOPTC(petattr, 88, opt_in, set_in_game, No, Yes, No, No, NoAlias,
                "attributes for highlighting pets")
    NHOPTC(pettype, 4, opt_in, set_gameview, Yes, Yes, No, No, "pet",
                "your preferred initial pet type")
    NHOPTC(pickup_burden, 20, opt_in, set_in_game, No, Yes, No, Yes, NoAlias,
                "maximum burden picked up before prompt")
    NHOPTB(pickup_thrown, 0, opt_out, set_in_game, On, Yes, No, No, NoAlias,
                &flags.pickup_thrown)
    NHOPTC(pickup_types, MAXOCLASSES, opt_in, set_in_game, No, Yes, No, Yes,
                NoAlias,  "types of objects to pick up automatically")
    NHOPTC(pile_limit, 24, opt_in, set_in_game, Yes, Yes, No, No, NoAlias,
                "threshold for \"there are many objects here\"")
    NHOPTC(player_selection, 12, opt_in, set_gameview, No, Yes, No, No,
                NoAlias, "choose character via dialog or prompts")
    NHOPTC(playmode, 8, opt_in, set_gameview, No, Yes, No, No, NoAlias,
                "normal play, non-scoring explore mode, or debug mode")
    NHOPTB(popup_dialog, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &iflags.wc_popup_dialog)
    NHOPTB(preload_tiles, 0, opt_out, set_in_config, On, Yes, No, No, NoAlias,
                &iflags.wc_preload_tiles)
    NHOPTB(pushweapon, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &flags.pushweapon)
    NHOPTB(quick_farsight, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &flags.quick_farsight)
    NHOPTC(race, PL_CSIZ, opt_in, set_gameview, No, Yes, No, No, NoAlias,
                "your starting race (e.g., Human, Elf)")
#ifdef MICRO
    NHOPTB(rawio, 0, opt_in, set_in_config, Off, Yes, No, No, NoAlias,
                &iflags.rawio)
#else
    NHOPTB(rawio, 0, opt_in, set_in_config, Off, No, No, No, NoAlias,
                (boolean *) 0)
#endif
    NHOPTB(rest_on_space, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &flags.rest_on_space)
    NHOPTC(roguesymset, 70, opt_in, set_in_game, No, Yes, No, Yes, NoAlias,
                "load a set of rogue display symbols from symbols file")
    NHOPTC(role, PL_CSIZ, opt_in, set_gameview, No, Yes, No, No, "character",
                "your starting role (e.g., Barbarian, Valkyrie)")
    NHOPTC(runmode, sizeof "teleport", opt_in, set_in_game, Yes, Yes, No, Yes,
                NoAlias, "display frequency when `running' or `travelling'")
    NHOPTB(safe_pet, 0, opt_out, set_in_game, On, Yes, No, No, NoAlias,
                &flags.safe_dog)
    NHOPTB(safe_wait, 0, opt_out, set_in_game, On, Yes, No, No, NoAlias,
                &flags.safe_wait)
    NHOPTB(sanity_check, 0, opt_in, set_wizonly, Off, Yes, No, No, NoAlias,
                &iflags.sanity_check)
    NHOPTC(scores, 32, opt_in, set_in_game, No, Yes, No, No, NoAlias,
                "the parts of the score list you wish to see")
    NHOPTC(scroll_amount, 20, opt_in, set_gameview, Yes, Yes, No, No, NoAlias,
                "amount to scroll map when scroll_margin is reached")
    NHOPTC(scroll_margin, 20, opt_in, set_gameview, Yes, Yes, No, No, NoAlias,
                "scroll map when this far from the edge")
    NHOPTB(selectsaved, 0, opt_out, set_in_config, On, Yes, No, No, NoAlias,
                &iflags.wc2_selectsaved)
    NHOPTB(showexp, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &flags.showexp)
    NHOPTB(showrace, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &flags.showrace)
#ifdef SCORE_ON_BOTL
    NHOPTB(showscore, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &flags.showscore)
#else
    NHOPTB(showscore, 0, opt_in, set_in_config, Off, Yes, No, No, NoAlias,
                (boolean *) 0)
#endif
    NHOPTB(silent, 0, opt_out, set_in_game, On, Yes, No, No, NoAlias,
                &flags.silent)
    NHOPTB(softkeyboard, 0, opt_in, set_in_config, Off, Yes, No, No, NoAlias,
                &iflags.wc2_softkeyboard)
    NHOPTC(sortdiscoveries, 0, opt_in, set_in_game, Yes, Yes, No, Yes,
                NoAlias, "preferred order when displaying discovered objects")
    NHOPTC(sortloot, 4, opt_in, set_in_game, No, Yes, No, Yes, NoAlias,
                "sort object selection lists by description")
    NHOPTB(sortpack, 0, opt_out, set_in_game, On, Yes, No, No, NoAlias,
                &flags.sortpack)
    NHOPTB(sparkle, 0, opt_out, set_in_game, On, Yes, No, No, NoAlias,
                &flags.sparkle)
    NHOPTB(splash_screen, 0, opt_out, set_in_config, On, Yes, No, No, NoAlias,
                &iflags.wc_splash_screen)
    NHOPTB(standout, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &flags.standout)
    NHOPTB(status_updates, 0, opt_out, set_in_config, On, Yes, No, No, NoAlias,
                &iflags.status_updates)
    NHOPTO("status condition fields", o_status_cond, BUFSZ, opt_in,
           set_in_game, No, Yes, No, NoAlias, "edit status condition fields")
#ifdef STATUS_HILITES
    NHOPTC(statushilites, 20, opt_in, set_in_game, Yes, Yes, Yes, No, NoAlias,
                "0=no status highlighting, N=show highlights for N turns")
    NHOPTO("status highlight rules", o_status_hilites, BUFSZ, opt_in,
           set_in_game, No, Yes, No, NoAlias, "edit status hilites")
#else
    NHOPTC(statushilites, 20, opt_in, set_in_config, Yes, Yes, Yes, No,
                NoAlias, "highlight control")
#endif
    NHOPTC(statuslines, 20, opt_in, set_in_game, No, Yes, No, No, NoAlias,
                "2 or 3 lines for status display")
#ifdef WIN32
    NHOPTC(subkeyvalue, 7, opt_in, set_in_config, No, Yes, Yes, No, NoAlias,
                "override keystroke value")
#endif
    NHOPTC(suppress_alert, 8, opt_in, set_in_game, No, Yes, Yes, No, NoAlias,
                "suppress alerts about version-specific features")
    NHOPTC(symset, 70, opt_in, set_in_game, No, Yes, No, Yes, NoAlias,
                "load a set of display symbols from symbols file")
    NHOPTC(term_cols, 6, opt_in, set_in_config, No, Yes, No, No, "termcolumns",
                "number of columns")
    NHOPTC(term_rows, 6, opt_in, set_in_config, No, Yes, No, No, NoAlias,
                "number of rows")
    NHOPTC(tile_file, 70, opt_in, set_gameview, No, Yes, No, No, NoAlias,
                "name of tile file")
    NHOPTC(tile_height, 20, opt_in, set_gameview, Yes, Yes, No, No, NoAlias,
                "height of tiles")
    NHOPTC(tile_width, 20, opt_in, set_gameview, Yes, Yes, No, No, NoAlias,
                "width of tiles")
    NHOPTB(tiled_map, 0, opt_in, set_in_game, tiled_map_Def, Yes, No, No,
                NoAlias, &iflags.wc_tiled_map)
    NHOPTB(time, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &flags.time)
#ifdef TIMED_DELAY
    NHOPTB(timed_delay, 0, opt_out, set_in_game, Off, Yes, No, No, NoAlias,
                &flags.nap)
#else
    NHOPTB(timed_delay, 0, opt_in, set_in_game, Off, No, No, No, NoAlias,
                (boolean *) 0)
#endif
    NHOPTB(tombstone, 0, opt_out, set_in_game, On, Yes, No, No, NoAlias,
                &flags.tombstone)
    NHOPTB(toptenwin, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &iflags.toptenwin)
    NHOPTC(traps, MAXTCHARS + 1, opt_in, set_in_config, No, Yes, No, No,
                NoAlias, "list of symbols to use in drawing traps")
    NHOPTB(travel, 0, opt_out, set_in_game, On, Yes, No, No, NoAlias,
                &flags.travelcmd)
#ifdef DEBUG
    NHOPTB(travel_debug, 0, opt_out, set_wizonly, Off, Yes, No, No, NoAlias,
                &iflags.trav_debug)
#else
    NHOPTB(travel_debug, 0, opt_out, set_wizonly, Off, No, No, No, NoAlias,
                (boolean *) 0)
#endif
    NHOPTB(use_darkgray, 0, opt_out, set_in_config, On, Yes, No, No, NoAlias,
                &iflags.wc2_darkgray)
    NHOPTB(use_inverse, 0, opt_out, set_in_game, On, Yes, No, No, NoAlias,
                &iflags.wc_inverse)
    NHOPTB(use_truecolor, 0, opt_in, set_in_config, Off, Yes, No, No,
                "use_truecolour", &iflags.use_truecolor)
    NHOPTC(vary_msgcount, 20, opt_in, set_gameview, No, Yes, No, No, NoAlias,
                "show more old messages at a time")
#if defined(NO_VERBOSE_GRANULARITY)
    NHOPTB(verbose, 0, opt_out, set_in_game, On, Yes, No, No, NoAlias,
                &flags.verbose)
#endif
#ifdef MSDOS
    NHOPTC(video, 20, opt_in, set_in_config, No, Yes, No, No, NoAlias,
                "method of video updating")
#endif
#ifdef VIDEOSHADES
    NHOPTC(videocolors, 40, opt_in, set_gameview, No, Yes, No, No,
                "videocolours", "color mappings for internal screen routines")
    NHOPTC(videoshades, 32, opt_in, set_gameview, No, Yes, No, No, NoAlias,
                "gray shades to map to black/gray/white")
#endif
#ifdef MSDOS
    NHOPTC(video_width, 10, opt_in, set_gameview, No, Yes, No, No, NoAlias,
                "video width")
    NHOPTC(video_height, 10, opt_in, set_gameview, No, Yes, No, No, NoAlias,
                "video height")
#endif
#ifdef TTY_TILES_ESCCODES
    NHOPTB(vt_tiledata, 0, opt_in, set_in_config, Off, Yes, No, No, NoAlias,
                &iflags.vt_tiledata)
#else
    NHOPTB(vt_tiledata, 0, opt_in, set_in_config, Off, Yes, No, No, NoAlias,
                (boolean *) 0)
#endif
#ifdef TTY_SOUND_ESCCODES
    NHOPTB(vt_sounddata, 0, opt_in, set_in_config, Off, Yes, No, No, NoAlias,
                &iflags.vt_sounddata)
#else
    NHOPTB(vt_sounddata, 0, opt_in, set_in_config, Off, Yes, No, No, NoAlias,
                (boolean *) 0)
#endif
    NHOPTC(warnings, 10, opt_in, set_in_config, No, Yes, No, No, NoAlias,
                "display characters for warnings")
    NHOPTC(whatis_coord, 1, opt_in, set_in_game, Yes, Yes, No, Yes, NoAlias,
                "show coordinates when auto-describing cursor position")
    NHOPTC(whatis_filter, 1, opt_in, set_in_game, Yes, Yes, No, Yes, NoAlias,
                "filter coordinate locations when targeting next or previous")
    NHOPTB(whatis_menu, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &iflags.getloc_usemenu)
    NHOPTB(whatis_moveskip, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &iflags.getloc_moveskip)
    NHOPTC(windowborders, 9, opt_in, set_in_game, Yes, Yes, No, No, NoAlias,
                "0 (off), 1 (on), 2 (auto)")
#ifdef WINCHAIN
    NHOPTC(windowchain, WINTYPELEN, opt_in, set_in_sysconf, No, Yes, No, No,
                NoAlias, "window processor to use")
#endif
    NHOPTC(windowcolors, 80, opt_in, set_gameview, No, Yes, No, No, NoAlias,
                "the foreground/background colors of windows")
    NHOPTC(windowtype, WINTYPELEN, opt_in, set_gameview, No, Yes, No, No,
                NoAlias, "windowing system to use (should be specified first)")
    NHOPTB(wizmgender, 0, opt_in, set_wizonly, Off, Yes, No, No, NoAlias,
                &iflags.wizmgender)
    NHOPTB(wizweight, 0, opt_in, set_wizonly, Off, Yes, No, No, NoAlias,
                &iflags.wizweight)
    NHOPTB(wraptext, 0, opt_in, set_in_game, Off, Yes, No, No, NoAlias,
                &iflags.wc2_wraptext)

    /*
     * Prefix-based Options
     */

    NHOPTP(cond_, 0, opt_in, set_hidden, No, No, Yes, Yes, NoAlias,
                "prefix for cond_ options")
    NHOPTP(font, 0, opt_in, set_hidden, Yes, Yes, Yes, No, NoAlias,
                "prefix for font options")
#if defined(MICRO) && !defined(AMIGA)
    /* included for compatibility with old NetHack.cnf files */
    NHOPTP(IBM_, 0, opt_in, set_hidden, No, No, Yes, No, NoAlias,
                "prefix for old micro IBM_ options")
#endif /* MICRO */
#if !defined(NO_VERBOSE_GRANULARITY)
    NHOPTP(verbose, 0, opt_out, set_in_game, Yes, Yes, Yes, Yes, NoAlias,
                "suppress verbose messages")
#endif
#undef NoAlias
#undef NHOPTB
#undef NHOPTC
#undef NHOPTP
#undef NHOPTO

#endif /* NHOPT_PROTO || NHOPT_ENUM || NHOPT_PARSE */

/* end of optlist */

