/* NetHack 3.7	nhlua.c	$NHDT-Date: 1673740532 2023/01/14 23:55:32 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.95 $ */
/*      Copyright (c) 2018 by Pasi Kallinen */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "dlb.h"

#ifndef LUA_VERSION_RELEASE_NUM
#ifdef NHL_SANDBOX
#undef NHL_SANDBOX
#endif
#endif

#ifdef NHL_SANDBOX
#include <setjmp.h>
#endif

/*
#- include <lua5.3/lua.h>
#- include <lua5.3/lualib.h>
#- include <lua5.3/lauxlib.h>
*/

/*  */

/* lua_CFunction prototypes */
#ifdef DUMPLOG
static int nhl_dump_fmtstr(lua_State *);
#endif /* DUMPLOG */
static int nhl_dnum_name(lua_State *);
static int nhl_stairways(lua_State *);
static int nhl_pushkey(lua_State *);
static int nhl_doturn(lua_State *);
static int nhl_debug_flags(lua_State *);
static int nhl_timer_has_at(lua_State *);
static int nhl_timer_peek_at(lua_State *);
static int nhl_timer_stop_at(lua_State *);
static int nhl_timer_start_at(lua_State *);
static int nhl_get_cmd_key(lua_State *);
static int nhl_callback(lua_State *);
static int nhl_gamestate(lua_State *);
static int nhl_test(lua_State *);
static int nhl_getmap(lua_State *);
static char splev_typ2chr(schar);
static int nhl_gettrap(lua_State *);
static int nhl_deltrap(lua_State *);
#if 0
static int nhl_setmap(lua_State *);
#endif
static int nhl_impossible(lua_State *);
static int nhl_pline(lua_State *);
static int nhl_verbalize(lua_State *);
static int nhl_parse_config(lua_State *);
static int nhl_menu(lua_State *);
static int nhl_text(lua_State *);
static int nhl_getlin(lua_State *);
static int nhl_makeplural(lua_State *);
static int nhl_makesingular(lua_State *);
static int nhl_s_suffix(lua_State *);
static int nhl_ing_suffix(lua_State *);
static int nhl_an(lua_State *);
static int nhl_rn2(lua_State *);
static int nhl_random(lua_State *);
static int nhl_level_difficulty(lua_State *);
static void init_nhc_data(lua_State *);
static int nhl_push_anything(lua_State *, int, void *);
static int nhl_meta_u_index(lua_State *);
static int nhl_meta_u_newindex(lua_State *);
static int nhl_u_clear_inventory(lua_State *);
static int nhl_u_giveobj(lua_State *);
static void init_u_data(lua_State *);
#ifdef notyet
static int nhl_set_package_path(lua_State *, const char *);
#endif
static int traceback_handler(lua_State *);
#ifdef NHL_SANDBOX
static void nhlL_openlibs(lua_State *, uint32_t);
#endif
static lua_State *nhlL_newstate (nhl_sandbox_info *);
static void end_luapat(void);

static const char *const nhcore_call_names[NUM_NHCORE_CALLS] = {
    "start_new_game",
    "restore_old_game",
    "moveloop_turn",
    "game_exit",
    "getpos_tip",
};
static boolean nhcore_call_available[NUM_NHCORE_CALLS];

/* internal structure that hangs off L->ud (but use lua_getallocf() )
 * Note that we use it for both memory use tracking and instruction counting.
 */
typedef struct nhl_user_data {
    uint32_t  flags;       /* from nhl_sandbox_info */

    uint32_t  inuse;
    uint32_t  memlimit;

    uint32_t  steps;       /* current counter */
    uint32_t  osteps;      /* original steps value */
    uint32_t  perpcall;    /* per pcall steps value */
#ifdef NHL_SANDBOX
    jmp_buf  jb;
#endif
} nhl_user_data;

static lua_State *luapat;   /* instance for file pattern matching */

void
l_nhcore_init(void)
{
#if 1
    nhl_sandbox_info sbi = {NHL_SB_SAFE, 0, 0, 0};
#else
    /* Sample sbi for getting resource usage information. */
    nhl_sandbox_info sbi = {NHL_SB_SAFE|NHL_SB_REPORT2, 10000000, 10000000, 0};
#endif
    if ((gl.luacore = nhl_init(&sbi)) != 0) {
        if (!nhl_loadlua(gl.luacore, "nhcore.lua")) {
            gl.luacore = (lua_State *) 0;
        } else {
            int i;

            for (i = 0; i < NUM_NHCORE_CALLS; i++)
                nhcore_call_available[i] = TRUE;
        }
    } else
        impossible("l_nhcore_init failed");
}

void
l_nhcore_done(void)
{
    if (gl.luacore) {
        nhl_done(gl.luacore);
        gl.luacore = 0;
    }
    end_luapat();
}

void
l_nhcore_call(int callidx)
{
    int ltyp;

    if (callidx < 0 || callidx >= NUM_NHCORE_CALLS
        || !gl.luacore || !nhcore_call_available[callidx])
        return;

    lua_getglobal(gl.luacore, "nhcore");
    if (!lua_istable(gl.luacore, -1)) {
        /*impossible("nhcore is not a lua table");*/
        nhl_done(gl.luacore);
        gl.luacore = 0;
        return;
    }

    lua_getfield(gl.luacore, -1, nhcore_call_names[callidx]);
    ltyp = lua_type(gl.luacore, -1);
    if (ltyp == LUA_TFUNCTION) {
        lua_remove(gl.luacore, -2); /* nhcore_call_names[callidx] */
        lua_remove(gl.luacore, -2); /* nhcore */
        if (nhl_pcall(gl.luacore, 0, 1)) {
            impossible("Lua error: %s", lua_tostring(gl.luacore, -1));
        }
    } else {
        /*impossible("nhcore.%s is not a lua function",
          nhcore_call_names[callidx]);*/
        nhcore_call_available[callidx] = FALSE;
    }
}

DISABLE_WARNING_UNREACHABLE_CODE

ATTRNORETURN void
nhl_error(lua_State *L, const char *msg)
{
    lua_Debug ar;
    char buf[BUFSZ*2];

    lua_getstack(L, 1, &ar);
    lua_getinfo(L, "lS", &ar);
    Sprintf(buf, "%s (line %d ", msg, ar.currentline);
    Sprintf(eos(buf), "%.*s)",
            /* (max length of ar.short_src is actually LUA_IDSIZE
               so this is overkill for it, but crucial for ar.source) */
            (int) (sizeof buf - (strlen(buf) + sizeof ")")),
            ar.short_src); /* (used to be 'ar.source' here) */
    lua_pushstring(L, buf);
#if 0 /* defined(PANICTRACE) && !defined(NO_SIGNALS) */
    panictrace_setsignals(FALSE);
#endif
    (void) lua_error(L);
    /*NOTREACHED*/
}

RESTORE_WARNING_UNREACHABLE_CODE

/* Check that parameters are nothing but single table,
   or if no parameters given, put empty table there */
void
lcheck_param_table(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc < 1)
        lua_createtable(L, 0, 0);

    /* discard any extra arguments passed in */
    lua_settop(L, 1);

    luaL_checktype(L, 1, LUA_TTABLE);
}

DISABLE_WARNING_UNREACHABLE_CODE

schar
get_table_mapchr(lua_State *L, const char *name)
{
    char *ter;
    xint8 typ;

    ter = get_table_str(L, name);
    typ = check_mapchr(ter);
    if (typ == INVALID_TYPE)
        nhl_error(L, "Erroneous map char");
    if (ter)
        free(ter);
    return typ;
}

schar
get_table_mapchr_opt(lua_State *L, const char *name, schar defval)
{
    char *ter;
    xint8 typ;

    ter = get_table_str_opt(L, name, emptystr);
    if (name && ter && *ter) {
        typ = (xint8) check_mapchr(ter);
        if (typ == INVALID_TYPE)
            nhl_error(L, "Erroneous map char");
    } else
        typ = (xint8) defval;
    if (ter)
        free(ter);
    return typ;
}

short
nhl_get_timertype(lua_State *L, int idx)
{
    static const char *const timerstr[NUM_TIME_FUNCS+1] = {
        "rot-organic", "rot-corpse", "revive-mon", "zombify-mon",
        "burn-obj", "hatch-egg", "fig-transform", "melt-ice", "shrink-glob",
        NULL
    };
    short ret = luaL_checkoption(L, idx, NULL, timerstr);

    if (ret < 0 || ret >= NUM_TIME_FUNCS)
        nhl_error(L, "Unknown timer type");
    return ret;
}

RESTORE_WARNING_UNREACHABLE_CODE

void
nhl_add_table_entry_int(lua_State *L, const char *name, lua_Integer value)
{
    lua_pushstring(L, name);
    lua_pushinteger(L, value);
    lua_rawset(L, -3);
}

void
nhl_add_table_entry_char(lua_State *L, const char *name, char value)
{
    char buf[2];
    Sprintf(buf, "%c", value);
    lua_pushstring(L, name);
    lua_pushstring(L, buf);
    lua_rawset(L, -3);
}

void
nhl_add_table_entry_str(lua_State *L, const char *name, const char *value)
{
    lua_pushstring(L, name);
    lua_pushstring(L, value);
    lua_rawset(L, -3);
}
void
nhl_add_table_entry_bool(lua_State *L, const char *name, boolean value)
{
    lua_pushstring(L, name);
    lua_pushboolean(L, value);
    lua_rawset(L, -3);
}

void
nhl_add_table_entry_region(lua_State *L, const char *name,
                           coordxy x1, coordxy y1, coordxy x2, coordxy y2)
{
    lua_pushstring(L, name);
    lua_newtable(L);
    nhl_add_table_entry_int(L, "x1", x1);
    nhl_add_table_entry_int(L, "y1", y1);
    nhl_add_table_entry_int(L, "x2", x2);
    nhl_add_table_entry_int(L, "y2", y2);
    lua_rawset(L, -3);
}

/* converting from special level "map character" to levl location type
   and back. order here is important. */
const struct {
    char ch;
    schar typ;
} char2typ[] = {
                { ' ', STONE },
                { '#', CORR },
                { '.', ROOM },
                { '-', HWALL },
                { '-', TLCORNER },
                { '-', TRCORNER },
                { '-', BLCORNER },
                { '-', BRCORNER },
                { '-', CROSSWALL },
                { '-', TUWALL },
                { '-', TDWALL },
                { '-', TLWALL },
                { '-', TRWALL },
                { '-', DBWALL },
                { '|', VWALL },
                { '+', DOOR },
                { 'A', AIR },
                { 'C', CLOUD },
                { 'S', SDOOR },
                { 'H', SCORR },
                { '{', FOUNTAIN },
                { '\\', THRONE },
                { 'K', SINK },
                { '}', MOAT },
                { 'P', POOL },
                { 'L', LAVAPOOL },
                { 'Z', LAVAWALL },
                { 'I', ICE },
                { 'W', WATER },
                { 'T', TREE },
                { 'F', IRONBARS }, /* Fe = iron */
                { 'x', MAX_TYPE }, /* "see-through" */
                { 'B', CROSSWALL }, /* hack: boundary location */
                { 'w', MATCH_WALL }, /* IS_STWALL() */
                { '\0', STONE },
};

schar
splev_chr2typ(char c)
{
    int i;

    for (i = 0; char2typ[i].ch; i++)
        if (c == char2typ[i].ch)
            return char2typ[i].typ;
    return (INVALID_TYPE);
}

schar
check_mapchr(const char *s)
{
    if (s && strlen(s) == 1)
        return splev_chr2typ(s[0]);
    return INVALID_TYPE;
}

static char
splev_typ2chr(schar typ)
{
    int i;

    for (i = 0; char2typ[i].typ < MAX_TYPE; i++)
        if (typ == char2typ[i].typ)
            return char2typ[i].ch;
    return 'x';
}

DISABLE_WARNING_UNREACHABLE_CODE

/* local t = nh.gettrap(x,y); */
/* local t = nh.gettrap({ x = 10, y = 10 }); */
static int
nhl_gettrap(lua_State *L)
{
    lua_Integer lx, ly;
    coordxy x, y;

    if (!nhl_get_xy_params(L, &lx, &ly)) {
        nhl_error(L, "Incorrect arguments");
        /*NOTREACHED*/
        return 0;
    }
    x = (coordxy) lx;
    y = (coordxy) ly;
    cvt_to_abscoord(&x, &y);

    if (isok(x, y)) {
            struct trap *ttmp = t_at(x,y);

            if (ttmp) {
                lua_newtable(L);

                nhl_add_table_entry_int(L, "tx", ttmp->tx);
                nhl_add_table_entry_int(L, "ty", ttmp->ty);
                nhl_add_table_entry_int(L, "ttyp", ttmp->ttyp);
                nhl_add_table_entry_str(L, "ttyp_name",
                                        get_trapname_bytype(ttmp->ttyp));
                nhl_add_table_entry_bool(L, "tseen", ttmp->tseen);
                nhl_add_table_entry_bool(L, "madeby_u", ttmp->madeby_u);
                switch (ttmp->ttyp) {
                case SQKY_BOARD:
                    nhl_add_table_entry_int(L, "tnote", ttmp->tnote);
                    break;
                case ROLLING_BOULDER_TRAP:
                    nhl_add_table_entry_int(L, "launchx", ttmp->launch.x);
                    nhl_add_table_entry_int(L, "launchy", ttmp->launch.y);
                    nhl_add_table_entry_int(L, "launch2x", ttmp->launch2.x);
                    nhl_add_table_entry_int(L, "launch2y", ttmp->launch2.y);
                    break;
                case PIT:
                case SPIKED_PIT:
                    nhl_add_table_entry_int(L, "conjoined", ttmp->conjoined);
                    break;
                }
                return 1;
            } else
                nhl_error(L, "No trap at location");
        } else
            nhl_error(L, "Coordinates out of range");
    return 0;
}

/* nh.deltrap(x,y); nh.deltrap({ x = 10, y = 15 }); */
static int
nhl_deltrap(lua_State *L)
{
    lua_Integer lx, ly;
    coordxy x, y;

    if (!nhl_get_xy_params(L, &lx, &ly)) {
        nhl_error(L, "Incorrect arguments");
        /*NOTREACHED*/
        return 0;
    }
    x = (coordxy) lx;
    y = (coordxy) ly;
    cvt_to_abscoord(&x, &y);

    if (isok(x, y)) {
        struct trap *ttmp = t_at(x,y);

        if (ttmp)
            deltrap(ttmp);
    }
    return 0;
}

RESTORE_WARNING_UNREACHABLE_CODE

/* get parameters (XX,YY) or ({ x = XX, y = YY }) or ({ XX, YY }),
   and set the x and y values.
   return TRUE if there are such params in the stack.
   Note that this does not adjust the values of x and y at all from what is
   specified in the level file; so, it returns absolute coordinates rather than
   map-relative coordinates. Callers of this function must decide if they want
   to interpret the values as absolute or as map-relative, and adjust
   accordingly.
 */
boolean
nhl_get_xy_params(lua_State *L, lua_Integer *x, lua_Integer *y)
{
    int argc = lua_gettop(L);
    boolean ret = FALSE;

    if (argc == 2) {
        *x = lua_tointeger(L, 1);
        *y = lua_tointeger(L, 2);
        ret = TRUE;
    } else if (argc == 1 && lua_type(L, 1) == LUA_TTABLE) {
        lua_Integer ax, ay;

        ret = get_coord(L, 1, &ax, &ay);
        *x = ax;
        *y = ay;
    }
    return ret;
}

DISABLE_WARNING_UNREACHABLE_CODE

/* local loc = nh.getmap(x,y); */
/* local loc = nh.getmap({ x = 10, y = 35 }); */
static int
nhl_getmap(lua_State *L)
{
    lua_Integer lx, ly;
    coordxy x, y;

    if (!nhl_get_xy_params(L, &lx, &ly)) {
        nhl_error(L, "Incorrect arguments");
        return 0;
    }
    x = (coordxy) lx;
    y = (coordxy) ly;
    cvt_to_abscoord(&x, &y);

    if (isok(x, y)) {
            char buf[BUFSZ];
            lua_newtable(L);

            /* FIXME: some should be boolean values */
            nhl_add_table_entry_int(L, "glyph", levl[x][y].glyph);
            nhl_add_table_entry_int(L, "typ", levl[x][y].typ);
            nhl_add_table_entry_str(L, "typ_name",
                                    levltyp_to_name(levl[x][y].typ));
            Sprintf(buf, "%c", splev_typ2chr(levl[x][y].typ));
            nhl_add_table_entry_str(L, "mapchr", buf);
            nhl_add_table_entry_int(L, "seenv", levl[x][y].seenv);
            nhl_add_table_entry_bool(L, "horizontal", levl[x][y].horizontal);
            nhl_add_table_entry_bool(L, "lit", levl[x][y].lit);
            nhl_add_table_entry_bool(L, "waslit", levl[x][y].waslit);
            nhl_add_table_entry_int(L, "roomno", levl[x][y].roomno);
            nhl_add_table_entry_bool(L, "edge", levl[x][y].edge);
            nhl_add_table_entry_bool(L, "candig", levl[x][y].candig);

            nhl_add_table_entry_bool(L, "has_trap", t_at(x,y) ? 1 : 0);

            /* TODO: FIXME: levl[x][y].flags */

            lua_pushliteral(L, "flags");
            lua_newtable(L);

            if (IS_DOOR(levl[x][y].typ)) {
                nhl_add_table_entry_bool(L, "nodoor",
                                         (levl[x][y].flags == D_NODOOR));
                nhl_add_table_entry_bool(L, "broken",
                                         (levl[x][y].flags & D_BROKEN));
                nhl_add_table_entry_bool(L, "isopen",
                                         (levl[x][y].flags & D_ISOPEN));
                nhl_add_table_entry_bool(L, "closed",
                                         (levl[x][y].flags & D_CLOSED));
                nhl_add_table_entry_bool(L, "locked",
                                         (levl[x][y].flags & D_LOCKED));
                nhl_add_table_entry_bool(L, "trapped",
                                         (levl[x][y].flags & D_TRAPPED));
            } else if (IS_ALTAR(levl[x][y].typ)) {
                /* TODO: bits 0, 1, 2 */
                nhl_add_table_entry_bool(L, "shrine",
                                         (levl[x][y].flags & AM_SHRINE));
            } else if (IS_THRONE(levl[x][y].typ)) {
                nhl_add_table_entry_bool(L, "looted",
                                         (levl[x][y].flags & T_LOOTED));
            } else if (levl[x][y].typ == TREE) {
                nhl_add_table_entry_bool(L, "looted",
                                         (levl[x][y].flags & TREE_LOOTED));
                nhl_add_table_entry_bool(L, "swarm",
                                         (levl[x][y].flags & TREE_SWARM));
            } else if (IS_FOUNTAIN(levl[x][y].typ)) {
                nhl_add_table_entry_bool(L, "looted",
                                         (levl[x][y].flags & F_LOOTED));
                nhl_add_table_entry_bool(L, "warned",
                                         (levl[x][y].flags & F_WARNED));
            } else if (IS_SINK(levl[x][y].typ)) {
                nhl_add_table_entry_bool(L, "pudding",
                                         (levl[x][y].flags & S_LPUDDING));
                nhl_add_table_entry_bool(L, "dishwasher",
                                         (levl[x][y].flags & S_LDWASHER));
                nhl_add_table_entry_bool(L, "ring",
                                         (levl[x][y].flags & S_LRING));
            }
            /* TODO: drawbridges, walls, ladders, room=>ICED_xxx */

            lua_settable(L, -3);

            return 1;
    } else {
        /* TODO: return zerorm instead? */
        nhl_error(L, "Coordinates out of range");
        return 0;
    }
}

/* impossible("Error!") */
static int
nhl_impossible(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc == 1)
        impossible("%s", luaL_checkstring(L, 1));
    else
        nhl_error(L, "Wrong args");
    return 0;
}

/* pline("It hits!") */
/* pline("It hits!", true) */
static int
nhl_pline(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc == 1 || argc == 2) {
        pline("%s", luaL_checkstring(L, 1));
        if (lua_toboolean(L, 2))
            display_nhwindow(WIN_MESSAGE, TRUE); /* --more-- */
    } else
        nhl_error(L, "Wrong args");

    return 0;
}

/* verbalize("Fool!") */
static int
nhl_verbalize(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc == 1)
        verbalize("%s", luaL_checkstring(L, 1));
    else
        nhl_error(L, "Wrong args");

    return 0;
}

/* parse_config("OPTIONS=!color") */
static int
nhl_parse_config(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc == 1)
        parse_conf_str(luaL_checkstring(L, 1), parse_config_line);
    else
        nhl_error(L, "Wrong args");

    return 0;
}

/* local windowtype = get_config("windowtype"); */
static int
nhl_get_config(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc == 1) {
        lua_pushstring(L, get_option_value(luaL_checkstring(L, 1), TRUE));
        return 1;
    } else
        nhl_error(L, "Wrong args");

    return 0;
}

/*
  str = getlin("What do you want to call this dungeon level?");
 */
static int
nhl_getlin(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc == 1) {
        const char *prompt = luaL_checkstring(L, 1);
        char buf[BUFSZ];

        getlin(prompt, buf);
        lua_pushstring(L, buf);
        return 1;
    }

    nhl_error(L, "Wrong args");
    /*NOTREACHED*/
    return 0;
}

/*
 selected = menu("prompt", default, pickX, { "a" = "option a", "b" = "option b", ...})
 pickX = 0,1,2, or "none", "one", "any" (PICK_X in code)

 selected = menu("prompt", default, pickX,
                { {key:"a", text:"option a"}, {key:"b", text:"option b"}, ... } ) */
static int
nhl_menu(lua_State *L)
{
    static const char *const pickX[] = {"none", "one", "any"}; /* PICK_x */
    int argc = lua_gettop(L);
    const char *prompt;
    const char *defval = "";
    int pick = PICK_ONE, pick_cnt;
    winid tmpwin;
    anything any;
    menu_item *picks = (menu_item *) 0;
    int clr = 0;

    if (argc < 2 || argc > 4) {
        nhl_error(L, "Wrong args");
        /*NOTREACHED*/
        return 0;
    }

    prompt = luaL_checkstring(L, 1);
    if (lua_isstring(L, 2))
        defval = luaL_checkstring(L, 2);
    if (lua_isstring(L, 3))
        pick = luaL_checkoption(L, 3, "one", pickX);
    luaL_checktype(L, argc, LUA_TTABLE);

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);

    lua_pushnil(L); /* first key */
    while (lua_next(L, argc) != 0) {
        const char *str = "";
        const char *key = "";

        /* key @ index -2, value @ index -1 */
        if (lua_istable(L, -1)) {
            lua_pushliteral(L, "key");
            lua_gettable(L, -2);
            key = lua_tostring(L, -1);
            lua_pop(L, 1);

            lua_pushliteral(L, "text");
            lua_gettable(L, -2);
            str = lua_tostring(L, -1);
            lua_pop(L, 1);

            /* TODO: glyph, attr, accel, group accel (all optional) */
        } else if (lua_isstring(L, -1)) {
            str = luaL_checkstring(L, -1);
            key = luaL_checkstring(L, -2);
        }

        any = cg.zeroany;
        if (*key)
            any.a_char = key[0];
        add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE, clr, str,
                 (*defval && *key && defval[0] == key[0])
                    ? MENU_ITEMFLAGS_SELECTED : MENU_ITEMFLAGS_NONE);

        lua_pop(L, 1); /* removes 'value'; keeps 'key' for next iteration */
    }

    end_menu(tmpwin, prompt);
    pick_cnt = select_menu(tmpwin, pick, &picks);
    destroy_nhwindow(tmpwin);

    if (pick_cnt > 0) {
        char buf[2];
        buf[0] = picks[0].item.a_char;

        if (pick == PICK_ONE && pick_cnt > 1
            && *defval && defval[0] == picks[0].item.a_char)
            buf[0] = picks[1].item.a_char;

        buf[1] = '\0';
        lua_pushstring(L, buf);
        /* TODO: pick any */
    } else {
        char buf[2];
        buf[0] = defval[0];
        buf[1] = '\0';
        lua_pushstring(L, buf);
    }

    return 1;
}

/* text("foo\nbar\nbaz") */
static int
nhl_text(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc > 0) {
        menu_item *picks = (menu_item *) 0;
        winid tmpwin;
        anything any = cg.zeroany;

        tmpwin = create_nhwindow(NHW_MENU);
        start_menu(tmpwin, MENU_BEHAVE_STANDARD);

        while (lua_gettop(L) > 0) {
            char *ostr = dupstr(luaL_checkstring(L, 1));
            char *ptr, *str = ostr;
            char *lstr = str + strlen(str) - 1;

            do {
                char *nlp = strchr(str, '\n');

                if (nlp && (nlp - str) <= 76) {
                    ptr = nlp;
                } else {
                    ptr = str + 76;
                    if (ptr > lstr)
                        ptr = lstr;
                }
                while ((ptr > str) && !(*ptr == ' ' || *ptr == '\n'))
                    ptr--;
                *ptr = '\0';
                add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE, 0,
                         str, MENU_ITEMFLAGS_NONE);
                str = ptr + 1;
            } while (*str && str <= lstr);
            lua_pop(L, 1);
            free(ostr);
        }

        end_menu(tmpwin, (char *) 0);
        (void) select_menu(tmpwin, PICK_NONE, &picks);
        destroy_nhwindow(tmpwin);
    }
    return 0;
}

/* makeplural("zorkmid") */
static int
nhl_makeplural(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc == 1)
        lua_pushstring(L, makeplural(luaL_checkstring(L, 1)));
    else
        nhl_error(L, "Wrong args");

    return 1;
}

/* makesingular("zorkmids") */
static int
nhl_makesingular(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc == 1)
        lua_pushstring(L, makesingular(luaL_checkstring(L, 1)));
    else
        nhl_error(L, "Wrong args");

    return 1;
}

/* s_suffix("foo") */
static int
nhl_s_suffix(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc == 1)
        lua_pushstring(L, s_suffix(luaL_checkstring(L, 1)));
    else
        nhl_error(L, "Wrong args");

    return 1;
}

/* ing_suffix("foo") */
static int
nhl_ing_suffix(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc == 1)
        lua_pushstring(L, ing_suffix(luaL_checkstring(L, 1)));
    else
        nhl_error(L, "Wrong args");

    return 1;
}

/* an("foo") */
static int
nhl_an(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc == 1)
        lua_pushstring(L, an(luaL_checkstring(L, 1)));
    else
        nhl_error(L, "Wrong args");

    return 1;
}

/* rn2(10) */
static int
nhl_rn2(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc == 1)
        lua_pushinteger(L, rn2((int) luaL_checkinteger(L, 1)));
    else
        nhl_error(L, "Wrong args");

    return 1;
}

/* random(10);  -- is the same as rn2(10); */
/* random(5,8); -- same as 5 + rn2(8); */
static int
nhl_random(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc == 1)
        lua_pushinteger(L, rn2((int) luaL_checkinteger(L, 1)));
    else if (argc == 2)
        lua_pushinteger(L, luaL_checkinteger(L, 1) + rn2((int) luaL_checkinteger(L, 2)));
    else
        nhl_error(L, "Wrong args");

    return 1;
}

/* level_difficulty() */
static int
nhl_level_difficulty(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc == 0) {
        lua_pushinteger(L, level_difficulty());
    }
    else {
        nhl_error(L, "level_difficulty should not have any args");
    }
    return 1;
}

RESTORE_WARNING_UNREACHABLE_CODE

/* get mandatory integer value from table */
int
get_table_int(lua_State *L, const char *name)
{
    int ret;

    lua_getfield(L, -1, name);
    ret = (int) luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    return ret;
}

/* get optional integer value from table */
int
get_table_int_opt(lua_State *L, const char *name, int defval)
{
    int ret = defval;

    lua_getfield(L, -1, name);
    if (!lua_isnil(L, -1)) {
        ret = (int) luaL_checkinteger(L, -1);
    }
    lua_pop(L, 1);
    return ret;
}

char *
get_table_str(lua_State *L, const char *name)
{
    char *ret;

    lua_getfield(L, -1, name);
    ret = dupstr(luaL_checkstring(L, -1));
    lua_pop(L, 1);
    return ret;
}

/* get optional string value from table.
   return value must be freed by caller. */
char *
get_table_str_opt(lua_State *L, const char *name, char *defval)
{
    const char *ret;

    lua_getfield(L, -1, name);
    ret = luaL_optstring(L, -1, defval);
    if (ret) {
        lua_pop(L, 1);
        return dupstr(ret);
    }
    lua_pop(L, 1);
    return NULL;
}

int
get_table_boolean(lua_State *L, const char *name)
{
    static const char *const boolstr[] = {
        "true", "false", "yes", "no", NULL
    };
    /* static const int boolstr2i[] = { TRUE, FALSE, TRUE, FALSE, -1 }; */
    int ltyp;
    int ret = -1;

    lua_getfield(L, -1, name);
    ltyp = lua_type(L, -1);
    if (ltyp == LUA_TSTRING) {
        ret = luaL_checkoption(L, -1, NULL, boolstr);
        /* nhUse(boolstr2i[0]); */
    } else if (ltyp == LUA_TBOOLEAN) {
        ret = lua_toboolean(L, -1);
    } else if (ltyp == LUA_TNUMBER) {
        ret = (int) luaL_checkinteger(L, -1);
        if ( ret < 0 || ret > 1)
            ret = -1;
    }
    lua_pop(L, 1);
    if (ret == -1)
        nhl_error(L, "Expected a boolean");
    return ret;
}

int
get_table_boolean_opt(lua_State *L, const char *name, int defval)
{
    int ret = defval;

    lua_getfield(L, -1, name);
    if (lua_type(L, -1) != LUA_TNIL) {
        lua_pop(L, 1);
        return get_table_boolean(L, name);
    }
    lua_pop(L, 1);
    return ret;
}

/* opts[] is a null-terminated list */
int
get_table_option(lua_State *L,
                 const char *name,
                 const char *defval,
                 const char *const opts[])
{
    int ret;

    lua_getfield(L, -1, name);
    ret = luaL_checkoption(L, -1, defval, opts);
    lua_pop(L, 1);
    return ret;
}

#ifdef DUMPLOG
/* local fname = dump_fmtstr("/tmp/nethack.%n.%d.log"); */
static int
nhl_dump_fmtstr(lua_State *L)
{
    int argc = lua_gettop(L);
    char buf[512];

    if (argc == 1)
        lua_pushstring(L, dump_fmtstr(luaL_checkstring(L, 1), buf, TRUE));
    else
        nhl_error(L, "Expected a string parameter");
    return 1;
}
#endif /* DUMPLOG */

/* local dungeon_name = dnum_name(u.dnum); */
static int
nhl_dnum_name(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc == 1) {
        lua_Integer dnum = luaL_checkinteger(L, 1);

        if (dnum >= 0 && dnum < gn.n_dgns)
            lua_pushstring(L, gd.dungeons[dnum].dname);
        else
            lua_pushstring(L, "");
    } else
        nhl_error(L, "Expected an integer parameter");
    return 1;
}

DISABLE_WARNING_UNREACHABLE_CODE
/* because nhl_error() does not return */

/* set or get variables which are saved and restored along with the game.
   nh.variable("test", 10);
   local ten = nh.variable("test"); */
static int
nhl_variable(lua_State *L)
{
    int argc = lua_gettop(L);
    int typ;
    const char *key;

    if (!gl.luacore) {
        nhl_error(L, "nh luacore not inited");
        /*NOTREACHED*/
        return 0;
    }

    lua_getglobal(gl.luacore, "nh_lua_variables");
    if (!lua_istable(gl.luacore, -1)) {
        impossible("nh_lua_variables is not a lua table");
        return 0;
    }

    if (argc == 1) {
        key = luaL_checkstring(L, 1);

        lua_getfield(gl.luacore, -1, key);
        typ = lua_type(gl.luacore, -1);
        if (typ == LUA_TSTRING)
            lua_pushstring(L, lua_tostring(gl.luacore, -1));
        else if (typ == LUA_TNIL)
            lua_pushnil(L);
        else if (typ == LUA_TBOOLEAN)
            lua_pushboolean(L, lua_toboolean(gl.luacore, -1));
        else if (typ == LUA_TNUMBER)
            lua_pushinteger(L, lua_tointeger(gl.luacore, -1));
        else if (typ == LUA_TTABLE) {
            lua_getglobal(gl.luacore, "nh_get_variables_string");
            lua_pushvalue(gl.luacore, -2);
            nhl_pcall(gl.luacore, 1, 1);
            luaL_loadstring(L, lua_tostring(gl.luacore, -1));
            nhl_pcall(L, 0, 1);
        } else
            nhl_error(L, "Cannot get variable of that type");
        return 1;
    } else if (argc == 2) {
        /* set nh_lua_variables[key] = value;
           nh.variable("key", value); */
        key = luaL_checkstring(L, 1);
        //pline("SETVAR:%s", key);
        typ = lua_type(L, -1);

        if (typ == LUA_TSTRING) {
            lua_pushstring(gl.luacore, lua_tostring(L, -1));
            lua_setfield(gl.luacore, -2, key);
        } else if (typ == LUA_TNIL) {
            lua_pushnil(gl.luacore);
            lua_setfield(gl.luacore, -2, key);
        } else if (typ == LUA_TBOOLEAN) {
            lua_pushboolean(gl.luacore, lua_toboolean(L, -1));
            lua_setfield(gl.luacore, -2, key);
        } else if (typ == LUA_TNUMBER) {
            lua_pushinteger(gl.luacore, lua_tointeger(L, -1));
            lua_setfield(gl.luacore, -2, key);
        } else if (typ == LUA_TTABLE) {
            lua_getglobal(L, "nh_set_variables_string");
            lua_pushstring(L, key);
            lua_pushvalue(L, -3); /* copy value to top */
            nhl_pcall(L, 2, 1);
            luaL_loadstring(gl.luacore, lua_tostring(L, -1));
            nhl_pcall(gl.luacore, 0, 0);
        } else
            nhl_error(L, "Cannot set variable of that type");
        return 0;
    } else
        nhl_error(L, "Wrong number of arguments");
    return 1;
}

/* return nh_lua_variable lua table as a string */
char *
get_nh_lua_variables(void)
{
    char *key = NULL;

    if (!gl.luacore) {
        nhl_error(gl.luacore, "nh luacore not inited");
        /*NOTREACHED*/
        return key;
    }

    lua_getglobal(gl.luacore, "nh_lua_variables");
    if (!lua_istable(gl.luacore, -1)) {
        impossible("nh_lua_variables is not a lua table");
        return key;
    }

    lua_getglobal(gl.luacore, "get_variables_string");
    if (lua_type(gl.luacore, -1) == LUA_TFUNCTION) {
        if (nhl_pcall(gl.luacore, 0, 1)) {
            impossible("Lua error: %s", lua_tostring(gl.luacore, -1));
            return key;
        }
        key = dupstr(lua_tostring(gl.luacore, -1));
    }
    return key;
}

RESTORE_WARNING_UNREACHABLE_CODE

/* save nh_lua_variables table to file */
void
save_luadata(NHFILE *nhfp)
{
    unsigned lua_data_len;
    char *lua_data = get_nh_lua_variables(); /* note: '\0' terminated */

    if (!lua_data)
        lua_data = dupstr(emptystr);
    lua_data_len = Strlen(lua_data) + 1; /* +1: include the terminator */
    bwrite(nhfp->fd, (genericptr_t) &lua_data_len,
           (unsigned) sizeof lua_data_len);
    bwrite(nhfp->fd, (genericptr_t) lua_data, lua_data_len);
    free(lua_data);
}

/* restore nh_lua_variables table from file */
void
restore_luadata(NHFILE *nhfp)
{
    unsigned lua_data_len;
    char *lua_data;

    mread(nhfp->fd, (genericptr_t) &lua_data_len,
          (unsigned) sizeof lua_data_len);
    lua_data = (char *) alloc(lua_data_len);
    mread(nhfp->fd, (genericptr_t) lua_data, lua_data_len);
    if (!gl.luacore)
        l_nhcore_init();
    luaL_loadstring(gl.luacore, lua_data);
    nhl_pcall(gl.luacore, 0, 0);
}

/* local stairs = stairways(); */
static int
nhl_stairways(lua_State *L)
{
    stairway *tmp = gs.stairs;
    int i = 1; /* lua arrays should start at 1 */

    lua_newtable(L);

    while (tmp) {

        lua_pushinteger(L, i);
        lua_newtable(L);

        nhl_add_table_entry_bool(L, "up", tmp->up);
        nhl_add_table_entry_bool(L, "ladder", tmp->isladder);
        nhl_add_table_entry_int(L, "x", tmp->sx);
        nhl_add_table_entry_int(L, "y", tmp->sy);
        nhl_add_table_entry_int(L, "dnum", tmp->tolev.dnum);
        nhl_add_table_entry_int(L, "dlevel", tmp->tolev.dlevel);

        lua_settable(L, -3);

        tmp = tmp->next;
        i++;
    }

    return 1;
}

/*
  test( { x = 123, y = 456 } );
*/
static int
nhl_test(lua_State *L)
{
    coordxy x, y;
    char *name, Player[] = "Player";

    /* discard any extra arguments passed in */
    lua_settop(L, 1);

    luaL_checktype(L, 1, LUA_TTABLE);

    x = (coordxy) get_table_int(L, "x");
    y = (coordxy) get_table_int(L, "y");
    name = get_table_str_opt(L, "name", Player);

    pline("TEST:{ x=%i, y=%i, name=\"%s\" }", (int) x, (int) y, name);

    free(name);

    return 1;
}

/* push a key into command queue */
/* nh.pushkey("i"); */
static int
nhl_pushkey(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc == 1) {
        const char *key = luaL_checkstring(L, 1);

        cmdq_add_key(CQ_CANNED, key[0]);
    }

    return 0;
}

/* do a turn of moveloop, or until gm.multi is done if param is true. */
/* nh.doturn(); nh.doturn(true); */
static int
nhl_doturn(lua_State *L)
{
    int argc = lua_gettop(L);
    boolean domulti = FALSE;

    if (argc == 1)
        domulti = lua_toboolean(L, 1);

    do {
        moveloop_core();
    } while (domulti && gm.multi);

    return 0;
}

/* set debugging flags. debugging use only, of course. */
/* nh.debug_flags({ mongen = false,
                    hunger = false,
                    overwrite_stairs = true }); */
static int
nhl_debug_flags(lua_State *L)
{
    int val;

    lcheck_param_table(L);

    /* disable monster generation */
    val = get_table_boolean_opt(L, "mongen", -1);
    if (val != -1) {
        iflags.debug_mongen = !(boolean)val; /* value in lua is negated */
        if (iflags.debug_mongen) {
            register struct monst *mtmp, *mtmp2;

            for (mtmp = fmon; mtmp; mtmp = mtmp2) {
                mtmp2 = mtmp->nmon;
                if (DEADMONSTER(mtmp))
                    continue;
                mongone(mtmp);
            }
        }
    }

    /* prevent hunger */
    val = get_table_boolean_opt(L, "hunger", -1);
    if (val != -1) {
        iflags.debug_hunger = !(boolean)val; /* value in lua is negated */
    }

    /* allow overwriting stairs */
    val = get_table_boolean_opt(L, "overwrite_stairs", -1);
    if (val != -1) {
        iflags.debug_overwrite_stairs = (boolean)val;
    }

    return 0;
}

DISABLE_WARNING_UNREACHABLE_CODE

/* does location at x,y have timer? */
/* local has_melttimer = nh.has_timer_at(x,y, "melt-ice"); */
/* local has_melttimer = nh.has_timer_at({x=4,y=7}, "melt-ice"); */
static int
nhl_timer_has_at(lua_State *L)
{
    boolean ret = FALSE;
    short timertype = nhl_get_timertype(L, -1);
    lua_Integer lx, ly;
    coordxy x, y;
    long when;

    lua_pop(L, 1); /* remove timertype */
    if (!nhl_get_xy_params(L, &lx, &ly)) {
        nhl_error(L, "nhl_timer_has_at: Wrong args");
        /*NOTREACHED*/
        return 0;
    }

    x = (coordxy) lx;
    y = (coordxy) ly;
    cvt_to_abscoord(&x, &y);

    if (isok(x, y)) {
        when = spot_time_expires(x, y, timertype);
        ret = (when > 0L);
    }
    lua_pushboolean(L, ret);
    return 1;
}

/* when does location at x,y timer trigger? */
/* local melttime = nh.peek_timer_at(x,y, "melt-ice"); */
/* local melttime = nh.peek_timer_at({x=5,y=6}, "melt-ice"); */
static int
nhl_timer_peek_at(lua_State *L)
{
    long when = 0L;
    short timertype = nhl_get_timertype(L, -1);
    lua_Integer lx, ly;
    coordxy x, y;

    lua_pop(L, 1); /* remove timertype */
    if (!nhl_get_xy_params(L, &lx, &ly)) {
        nhl_error(L, "nhl_timer_peek_at: Wrong args");
        /*NOTREACHED*/
        return 0;
    }

    x = (coordxy) lx;
    y = (coordxy) ly;
    cvt_to_abscoord(&x, &y);

    if (timer_is_pos(timertype) && isok(x, y))
        when = spot_time_expires(x, y, timertype);
    lua_pushinteger(L, when);
    return 1;
}

/* stop timer at location x,y */
/* nh.stop_timer_at(x,y, "melt-ice"); */
/* nh.stop_timer_at({x=6,y=8}, "melt-ice"); */
static int
nhl_timer_stop_at(lua_State *L)
{
    short timertype = nhl_get_timertype(L, -1);
    lua_Integer lx, ly;
    coordxy x, y;

    lua_pop(L, 1); /* remove timertype */
    if (!nhl_get_xy_params(L, &lx, &ly)) {
        nhl_error(L, "nhl_timer_stop_at: Wrong args");
        /*NOTREACHED*/
        return 0;
    }

    x = (coordxy) lx;
    y = (coordxy) ly;
    cvt_to_abscoord(&x, &y);

    if (timer_is_pos(timertype) && isok(x, y))
        spot_stop_timers(x, y, timertype);
    return 0;
}

/* start timer at location x,y */
/* nh.start_timer_at(x,y, "melt-ice", 10); */
static int
nhl_timer_start_at(lua_State *L)
{
    short timertype = nhl_get_timertype(L, -2);
    long when = lua_tointeger(L, -1);
    lua_Integer lx, ly;
    coordxy x, y;

    lua_pop(L, 2); /* remove when and timertype */
    if (!nhl_get_xy_params(L, &lx, &ly)) {
        nhl_error(L, "nhl_timer_start_at: Wrong args");
        /*NOTREACHED*/
        return 0;
    }

    x = (coordxy) lx;
    y = (coordxy) ly;
    cvt_to_abscoord(&x, &y);

    if (timer_is_pos(timertype) && isok(x, y)) {
        long where = ((long) x << 16) | (long) y;

        spot_stop_timers(x, y, timertype);
        (void) start_timer((long) when, TIMER_LEVEL, MELT_ICE_AWAY,
                           long_to_any(where));
    }
    return 0;
}

/* returns the visual interpretation of the key bound to an extended command,
   or the ext cmd name if not bound to any key */
/* local helpkey = eckey("help"); */
static int
nhl_get_cmd_key(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc == 1) {
        const char *cmd = luaL_checkstring(L, 1);
        char *key = cmd_from_ecname(cmd);

        lua_pushstring(L, key);
        return 1;
    }

    return 0;
}

/* add or remove a lua function callback */
/* callback("level_enter", "function_name"); */
/* callback("level_enter", "function_name", true); */
/* level_enter, level_leave, cmd_before */
static int
nhl_callback(lua_State *L)
{
    int argc = lua_gettop(L);
    int i;

    if (argc == 2) {
        const char *fn = luaL_checkstring(L, -1);
        const char *cb = luaL_checkstring(L, -2);

        if (!gl.luacore) {
            nhl_error(L, "nh luacore not inited");
            /*NOTREACHED*/
            return 0;
        }

        for (i = 0; i < NUM_NHCB; i++)
            if (!strcmp(cb, nhcb_name[i]))
                break;

        if (i >= NUM_NHCB)
            return 0;

        nhcb_counts[i]++;

        lua_getglobal(gl.luacore, "nh_callback_set");
        lua_pushstring(gl.luacore, cb);
        lua_pushstring(gl.luacore, fn);
        nhl_pcall(gl.luacore, 2, 0);
    } else if (argc == 3) {
        boolean rm = lua_toboolean(L, -1);
        const char *fn = luaL_checkstring(L, -2);
        const char *cb = luaL_checkstring(L, -3);

        if (!gl.luacore) {
            nhl_error(L, "nh luacore not inited");
            /*NOTREACHED*/
            return 0;
        }

        for (i = 0; i < NUM_NHCB; i++)
            if (!strcmp(cb, nhcb_name[i]))
                break;

        if (i >= NUM_NHCB)
            return 0;

        if (rm) {
            nhcb_counts[i]--;
            if (nhcb_counts[i] < 0)
                impossible("nh.callback counts are wrong");
        } else {
            nhcb_counts[i]++;
        }

        lua_getglobal(gl.luacore, rm ? "nh_callback_rm" : "nh_callback_set");
        lua_pushstring(gl.luacore, cb);
        lua_pushstring(gl.luacore, fn);
        nhl_pcall(gl.luacore, 2, 0);
    }

    return 0;
}

/* store or restore game state */
/* NOTE: doesn't work when saving/restoring the game */
/* currently handles inventory and turns. */
/* gamestate(); -- save state */
/* gamestate(true); -- restore state */
static int
nhl_gamestate(lua_State *L)
{
    int argc = lua_gettop(L);
    boolean reststate = argc > 0 ? lua_toboolean(L, -1) : FALSE;
    static struct obj *invent = NULL;
    static long moves = 0;
    static boolean stored = FALSE;

    if (reststate && stored) {
        /* restore game state */
        gm.moves = moves;
        gl.lastinvnr = 51;
        while (gi.invent)
            useupall(gi.invent);
        while (invent) {
            struct obj *otmp = invent;
            long wornmask = otmp->owornmask;
            otmp->owornmask = 0L;
            extract_nobj(otmp, &invent);
            addinv(otmp);
            if (wornmask)
                setworn(otmp, wornmask);
        }
        init_uhunger();
        stored = FALSE;
    } else {
        /* store game state */
        while (gi.invent) {
            struct obj *otmp = gi.invent;
            long wornmask = otmp->owornmask;
            setnotworn(otmp);
            freeinv(otmp);
            otmp->nobj = invent;
            otmp->owornmask = wornmask;
            invent = otmp;
        }
        gl.lastinvnr = 51;
        moves = gm.moves;
        stored = TRUE;
    }
    update_inventory();
    return 0;
}

RESTORE_WARNING_UNREACHABLE_CODE

static const struct luaL_Reg nhl_functions[] = {
    {"test", nhl_test},

    {"getmap", nhl_getmap},
#if 0
    {"setmap", nhl_setmap},
#endif
    {"gettrap", nhl_gettrap},
    {"deltrap", nhl_deltrap},

    {"has_timer_at", nhl_timer_has_at},
    {"peek_timer_at", nhl_timer_peek_at},
    {"stop_timer_at", nhl_timer_stop_at},
    {"start_timer_at", nhl_timer_start_at},

    {"abscoord", nhl_abs_coord},

    {"impossible", nhl_impossible},
    {"pline", nhl_pline},
    {"verbalize", nhl_verbalize},
    {"menu", nhl_menu},
    {"text", nhl_text},
    {"getlin", nhl_getlin},
    {"eckey", nhl_get_cmd_key},
    {"callback", nhl_callback},
    {"gamestate", nhl_gamestate},

    {"makeplural", nhl_makeplural},
    {"makesingular", nhl_makesingular},
    {"s_suffix", nhl_s_suffix},
    {"ing_suffix", nhl_ing_suffix},
    {"an", nhl_an},
    {"rn2", nhl_rn2},
    {"random", nhl_random},
    {"level_difficulty", nhl_level_difficulty},
    {"parse_config", nhl_parse_config},
    {"get_config", nhl_get_config},
    {"get_config_errors", l_get_config_errors},
#ifdef DUMPLOG
    {"dump_fmtstr", nhl_dump_fmtstr},
#endif /* DUMPLOG */
    {"dnum_name", nhl_dnum_name},
    {"variable", nhl_variable},
    {"stairways", nhl_stairways},
    {"pushkey", nhl_pushkey},
    {"doturn", nhl_doturn},
    {"debug_flags", nhl_debug_flags},
    {NULL, NULL}
};

static const struct {
    const char *name;
    long value;
} nhl_consts[] = {
    { "COLNO",  COLNO },
    { "ROWNO",  ROWNO },
#ifdef DLB
    { "DLB", 1},
#else
    { "DLB", 0},
#endif /* DLB */
    { NULL, 0 },
};

/* register and init the constants table */
static void
init_nhc_data(lua_State *L)
{
    int i;

    lua_newtable(L);

    for (i = 0; nhl_consts[i].name; i++) {
        lua_pushstring(L, nhl_consts[i].name);
        lua_pushinteger(L, nhl_consts[i].value);
        lua_rawset(L, -3);
    }

    lua_setglobal(L, "nhc");
}

static int
nhl_push_anything(lua_State *L, int anytype, void *src)
{
    anything any = cg.zeroany;

    switch (anytype) {
    case ANY_INT: any.a_int = *(int *) src;
        lua_pushinteger(L, any.a_int);
        break;
    case ANY_UCHAR: any.a_uchar = *(uchar *) src;
        lua_pushinteger(L, any.a_uchar);
        break;
    case ANY_SCHAR: any.a_schar = *(schar *) src;
        lua_pushinteger(L, any.a_schar);
        break;
    }
    return 1;
}

DISABLE_WARNING_UNREACHABLE_CODE

static int
nhl_meta_u_index(lua_State *L)
{
    static const struct {
        const char *name;
        void *ptr;
        int type;
    } ustruct[] = {
        { "ux", &(u.ux), ANY_UCHAR },
        { "uy", &(u.uy), ANY_UCHAR },
        { "dx", &(u.dx), ANY_SCHAR },
        { "dy", &(u.dy), ANY_SCHAR },
        { "dz", &(u.dz), ANY_SCHAR },
        { "tx", &(u.tx), ANY_UCHAR },
        { "ty", &(u.ty), ANY_UCHAR },
        { "ulevel", &(u.ulevel), ANY_INT },
        { "ulevelmax", &(u.ulevelmax), ANY_INT },
        { "uhunger", &(u.uhunger), ANY_INT },
        { "nv_range", &(u.nv_range), ANY_INT },
        { "xray_range", &(u.xray_range), ANY_INT },
        { "umonster", &(u.umonster), ANY_INT },
        { "umonnum", &(u.umonnum), ANY_INT },
        { "mh", &(u.mh), ANY_INT },
        { "mhmax", &(u.mhmax), ANY_INT },
        { "mtimedone", &(u.mtimedone), ANY_INT },
        { "dlevel", &(u.uz.dlevel), ANY_SCHAR }, /* actually coordxy */
        { "dnum", &(u.uz.dnum), ANY_SCHAR }, /* actually coordxy */
        { "uluck", &(u.uluck), ANY_SCHAR },
        { "uhp", &(u.uhp), ANY_INT },
        { "uhpmax", &(u.uhpmax), ANY_INT },
        { "uen", &(u.uen), ANY_INT },
        { "uenmax", &(u.uenmax), ANY_INT },
    };
    const char *tkey = luaL_checkstring(L, 2);
    int i;

    /* FIXME: doesn't really work, eg. negative values for u.dx */
    for (i = 0; i < SIZE(ustruct); i++)
        if (!strcmp(tkey, ustruct[i].name)) {
            return nhl_push_anything(L, ustruct[i].type, ustruct[i].ptr);
        }

    if (!strcmp(tkey, "inventory")) {
        nhl_push_obj(L, gi.invent);
        return 1;
    } else if (!strcmp(tkey, "role")) {
        lua_pushstring(L, gu.urole.name.m);
        return 1;
    } else if (!strcmp(tkey, "moves")) {
        lua_pushinteger(L, gm.moves);
        return 1;
    } else if (!strcmp(tkey, "uhave_amulet")) {
        lua_pushinteger(L, u.uhave.amulet);
        return 1;
    } else if (!strcmp(tkey, "depth")) {
        lua_pushinteger(L, depth(&u.uz));
        return 1;
    } else if (!strcmp(tkey, "invocation_level")) {
        lua_pushboolean(L, Invocation_lev(&u.uz));
        return 1;
    }

    nhl_error(L, "Unknown u table index");
    /*NOTREACHED*/
    return 0;
}

static int
nhl_meta_u_newindex(lua_State *L)
{
    nhl_error(L, "Cannot set u table values");
    /*NOTREACHED*/
    return 0;
}

RESTORE_WARNING_UNREACHABLE_CODE

static int
nhl_u_clear_inventory(lua_State *L UNUSED)
{
    while (gi.invent)
        useupall(gi.invent);
    return 0;
}

/* Put object into player's inventory */
/* u.giveobj(obj.new("rock")); */
static int
nhl_u_giveobj(lua_State *L)
{
    return nhl_obj_u_giveobj(L);
}

static const struct luaL_Reg nhl_u_functions[] = {
    { "clear_inventory", nhl_u_clear_inventory },
    { "giveobj", nhl_u_giveobj },
    { NULL, NULL }
};

static void
init_u_data(lua_State *L)
{
    lua_newtable(L);
    luaL_setfuncs(L, nhl_u_functions, 0);
    lua_newtable(L);
    lua_pushcfunction(L, nhl_meta_u_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, nhl_meta_u_newindex);
    lua_setfield(L, -2, "__newindex");
    lua_setmetatable(L, -2);
    lua_setglobal(L, "u");
}

#ifdef notyet
static int
nhl_set_package_path(lua_State *L, const char *path)
{
    if (LUA_TTABLE != lua_getglobal(L, "package")) {
        impossible("package not a table in nhl_set_package_path");
        return 1;
    };
    lua_pushstring(L, path);
    lua_setfield(L, -2, "path");
    lua_pop(L, 1);
    return 0;
}
#endif

static int
traceback_handler(lua_State *L)
{
    luaL_traceback(L, L, lua_tostring(L, 1), 0);
    /* TODO: call impossible() if fuzzing? */
    return 1;
}

/* lua_pcall with our traceback handler and instruction step limiting.
 *  On error, traceback will be on top of stack */
int
nhl_pcall(lua_State *L, int nargs, int nresults)
{
    struct nhl_user_data *nud;
    int rv;

    lua_pushcfunction(L, traceback_handler);
    lua_insert(L, 1);
    (void)lua_getallocf(L, (void **)&nud);
#ifdef NHL_SANDBOX
    if (nud && (nud->steps || nud->perpcall)) {
        if (nud->perpcall)
            nud->steps = nud->perpcall;
        if (setjmp(nud->jb)) {
            /* panic, because we don't know if the game state is corrupt */
            panic("time exceeded");
        }
    }
#endif

    rv = lua_pcall(L, nargs, nresults, 1);
    lua_remove(L, 1); /* remove handler */

#ifdef NHL_SANDBOX
    if (nud && (nud->flags & (NHL_SB_REPORT | NHL_SB_REPORT2)) != 0
        && (nud->memlimit || nud->osteps || nud->perpcall)) {
        if (nud->flags & NHL_SB_REPORT2)
            lua_gc(L, LUA_GCCOLLECT);
        pline("Lua context=%p RAM: %lu STEPS:%lu", (void *) L,
              (unsigned long) nud->inuse,
              (unsigned long) (nud->perpcall
                               ? (nud->perpcall - nud->steps)
                               : (nud->osteps - nud->steps)));
    }
#endif

    return rv;
}

/* XXX impossible() should be swappable with pline or nothing via flag */
/* read lua code/data from a dlb module or an external file
   into a string buffer and feed that to lua */
boolean
nhl_loadlua(lua_State *L, const char *fname)
{
#define LOADCHUNKSIZE (1L << 13) /* 8K */
    boolean ret = TRUE;
    dlb *fh;
    char *buf = (char *) 0, *bufin, *bufout, *p, *nl, *altfname;
    long buflen, ct, cnt;
    int llret;

    altfname = (char *) alloc(Strlen(fname) + 3); /* 3: '('...')\0' */
    /* don't know whether 'fname' is inside a dlb container;
       if we did, we could choose between "nhdat(<fname>)" and "<fname>"
       but since we don't, compromise */
    Sprintf(altfname, "(%s)", fname);
    fh = dlb_fopen(fname, RDBMODE);
    if (!fh) {
        impossible("nhl_loadlua: Error opening %s", altfname);
        ret = FALSE;
        goto give_up;
    }

    dlb_fseek(fh, 0L, SEEK_END);
    buflen = dlb_ftell(fh);
    dlb_fseek(fh, 0L, SEEK_SET);

    /* extra +1: room to add final '\n' if missing */
    buf = bufout = (char *) alloc(FITSint(buflen + 1 + 1));
    buf[0] = '\0';
    bufin = bufout = buf;

    ct = 0L;
    while (buflen > 0 || ct) {
        /*
         * Semi-arbitrarily limit reads to 8K at a time.  That's big
         * enough to cover the majority of our Lua files in one bite
         * but small enough to fully exercise the partial record
         * handling (when processing the castle's level description).
         *
         * [For an external file (non-DLB), VMS may only be able to
         * read at most 32K-1 at a time depending on the file format
         * in use, and fseek(SEEK_END) only yields an upper bound on
         * the actual amount of data in that situation.]
         */
        if ((cnt = dlb_fread(bufin, 1, min((int)buflen, LOADCHUNKSIZE), fh)) < 0L)
            break;
        buflen -= cnt; /* set up for next iteration, if any */
        if (cnt == 0L) {
            *bufin = '\n'; /* very last line is unterminated? */
            cnt = 1;
        }
        bufin[cnt] = '\0'; /* fread() doesn't do this */

        /* in case partial line was leftover from previous fread */
        bufin -= ct, cnt += ct, ct = 0;

        while (cnt > 0) {
            if ((nl = strchr(bufin, '\n')) != 0) {
                /* normal case, newline is present */
                ct = (long) (nl - bufin + 1L); /* +1: keep the newline */
                for (p = bufin; p <= nl; ++p)
                    *bufout++ = *bufin++;
                if (*bufin == '\r')
                    ++bufin, ++ct;
                /* update for next loop iteration */
                cnt -= ct;
                ct = 0;
            } else if (strlen(bufin) < LOADCHUNKSIZE) {
                /* no newline => partial record; move unprocessed chars
                   to front of input buffer (bufin portion of buf[]) */
                ct = cnt = (long) (eos(bufin) - bufin);
                for (p = bufout; cnt > 0; --cnt)
                    *p++ = *bufin++;
                *p = '\0';
                bufin = p; /* next fread() populates buf[] starting here */
                /* cnt==0 so inner loop will terminate */
            } else {
                /* LOADCHUNKSIZE portion of buffer already completely full */
                impossible("(%s) line too long", altfname);
                goto give_up;
            }
        }
    }
    *bufout = '\0';
    (void) dlb_fclose(fh);

    llret = luaL_loadbuffer(L, buf, strlen(buf), altfname);
    if (llret != LUA_OK) {
        impossible("luaL_loadbuffer: Error loading %s: %s",
                   altfname, lua_tostring(L, -1));
        ret = FALSE;
        goto give_up;
    } else {
        if (nhl_pcall(L, 0, LUA_MULTRET)) {
            impossible("Lua error: %s", lua_tostring(L, -1));
            ret = FALSE;
            goto give_up;
        }
    }

 give_up:
    if (altfname)
        free((genericptr_t) altfname);
    if (buf)
        free((genericptr_t) buf);
    return ret;
}

DISABLE_WARNING_CONDEXPR_IS_CONSTANT

lua_State *
nhl_init(nhl_sandbox_info *sbi)
{
    /* It would be nice to import EXPECTED from each build system. XXX */
    /* And it would be nice to do it only once, but it's cheap. */
#ifndef NHL_VERSION_EXPECTED
#define NHL_VERSION_EXPECTED 50404
#endif

#ifdef NHL_SANDBOX
    if (NHL_VERSION_EXPECTED != LUA_VERSION_RELEASE_NUM) {
        panic(
             "sandbox doesn't know this Lua version: this=%d != expected=%d ",
              LUA_VERSION_RELEASE_NUM, NHL_VERSION_EXPECTED);
    }
#endif

    lua_State *L = nhlL_newstate(sbi);

    iflags.in_lua = TRUE;
    /* Temporary for development XXX */
    /* Turn this off in config.h to disable the sandbox. */
#ifdef NHL_SANDBOX
    nhlL_openlibs(L, sbi->flags);
#else
    luaL_openlibs(L);
#endif

#ifdef notyet
    if (sbi->flags & NHL_SB_PACKAGE) {
        /* XXX Is this still needed? */
        if (nhl_set_package_path(L, "./?.lua"))
            return 0;
    }
#endif

    /* register nh -table, and functions for it */
    lua_newtable(L);
    luaL_setfuncs(L, nhl_functions, 0);
    lua_setglobal(L, "nh");

    /* init nhc -table */
    init_nhc_data(L);

    /* init u -table */
    init_u_data(L);

    l_selection_register(L);
    l_register_des(L);

    l_obj_register(L);

    /* nhlib.lua assumes the math table exists. */
    if (LUA_TTABLE != lua_getglobal(L, "math")) {
        lua_newtable(L);
        lua_setglobal(L, "math");
    }

    if (!nhl_loadlua(L, "nhlib.lua")) {
        nhl_done(L);
        return (lua_State *) 0;
    }

    return L;
}

RESTORE_WARNING_CONDEXPR_IS_CONSTANT

void
nhl_done(lua_State *L)
{
    if (L)
        lua_close(L);
    iflags.in_lua = FALSE;
}

boolean
load_lua(const char *name, nhl_sandbox_info *sbi)
{
    boolean ret = TRUE;
    lua_State *L = nhl_init(sbi);

    if (!L) {
        ret = FALSE;
        goto give_up;
    }

    if (!nhl_loadlua(L, name)) {
        ret = FALSE;
        goto give_up;
    }

 give_up:
    nhl_done(L);

    return ret;
}

DISABLE_WARNING_CONDEXPR_IS_CONSTANT

const char *
get_lua_version(void)
{
    nhl_sandbox_info sbi = {NHL_SB_VERSION, 0, 0, 0};

    if (gl.lua_ver[0] == 0) {
        lua_State *L = nhl_init(&sbi);

        if (L) {
            size_t len = 0;
            const char *vs = (const char *) 0;

            /* LUA_VERSION yields "<major>.<minor>" although we check to see
               whether it is "Lua-<major>.<minor>" and strip prefix if so;
               LUA_RELEASE is <LUA_VERSION>.<LUA_VERSION_RELEASE> but doesn't
               get set up as a lua global */
            lua_getglobal(L, "_RELEASE");
            if (lua_isstring(L, -1))
                vs = lua_tolstring (L, -1, &len);
#ifdef LUA_RELEASE
            else
                vs = LUA_RELEASE, len = strlen(vs);
#endif
            if (!vs) {
                lua_getglobal(L, "_VERSION");
                if (lua_isstring(L, -1))
                    vs = lua_tolstring (L, -1, &len);
#ifdef LUA_VERSION
                else
                    vs = LUA_VERSION, len = strlen(vs);
#endif
            }
            if (vs && len < sizeof gl.lua_ver) {
                if (!strncmpi(vs, "Lua", 3)) {
                    vs += 3;
                    if (*vs == '-' || *vs == ' ')
                        vs += 1;
                }
                Strcpy(gl.lua_ver, vs);
            }
        }
        nhl_done(L);
#ifdef LUA_COPYRIGHT
        if (sizeof LUA_COPYRIGHT <= sizeof gl.lua_copyright)
            Strcpy(gl.lua_copyright, LUA_COPYRIGHT);
#endif
    }
    return (const char *) gl.lua_ver;
}

RESTORE_WARNING_CONDEXPR_IS_CONSTANT

/***
 *** SANDBOX / HARDENING CODE
 ***/
#ifdef NHL_SANDBOX

enum ewhen {NEVER, IFFLAG, EOT};
struct e {
    enum ewhen when;
    const char *fnname;
};

/* NHL_BASE_BASE - safe things */
static struct e ct_base_base[] = {
    {IFFLAG, "ipairs"},
    {IFFLAG, "next"},
    {IFFLAG, "pairs"},
    {IFFLAG, "pcall"},
    {IFFLAG, "select"},
    {IFFLAG, "tonumber"},
    {IFFLAG, "tostring"},
    {IFFLAG, "type"},
    {IFFLAG, "xpcall"},
    {EOT, NULL}
};

/* NHL_BASE_ERROR - not really safe - might not want Lua to kill the process */
static struct e ct_base_error[] = {
    {IFFLAG, "assert"},   /* ok, calls error */
    {IFFLAG, "error"},    /* ok, calls G->panic */
    {NEVER, "print"}, /*not ok - calls lua_writestring/lua_writeline -> stdout*/
    {NEVER, "warn"}, /*not ok - calls lua_writestringerror -> stderr*/
    {EOT, NULL}
};

/* NHL_BASE_META - metatable access */
static struct e ct_base_meta[] = {
    {IFFLAG, "getmetatable"},
    {IFFLAG, "rawequal"},
    {IFFLAG, "rawget"},
    {IFFLAG, "rawlen"},
    {IFFLAG, "rawset"},
    {IFFLAG, "setmetatable"},
    {EOT, NULL}
};

/* NHL_BASE_GC - questionable safety */
static struct e ct_base_iffy[] = {
    {IFFLAG, "collectgarbage"},
    {EOT, NULL}
};

/* NHL_BASE_UNSAFE - include only if required */
/* TODO: if NHL_BASE_UNSAFE is ever used, we need to wrap lua_load with
 * something to forbid mode=="b" */
static struct e ct_base_unsafe[] = {
    {IFFLAG, "dofile"},
    {IFFLAG, "loadfile"},
    {IFFLAG, "load"},
    {EOT, NULL}
};

/* no ct_co_ tables; all fns at same level of concern */
/* no ct_string_ tables; all fns at same level of concern */
/* no ct_table_ tables; all fns at same level of concern (but
   sort can take a lot of time and can't be caught by the step limit */
/* no ct_utf8_ tables; all fns at same level of concern */


/* possible ct_debug tables - likely to need changes */
static struct e ct_debug_debug[] = {
    {NEVER,  "debug"},          /* uses normal I/O so needs re-write */
    {IFFLAG, "getuservalue"},
    {NEVER,  "gethook"},        /* see sethook */
    {IFFLAG, "getinfo"},
    {IFFLAG, "getlocal"},
    {IFFLAG, "getregistry"},
    {IFFLAG, "getmetatable"},
    {IFFLAG, "getupvalue"},
    {IFFLAG, "upvaluejoin"},
    {IFFLAG, "upvalueid"},
    {IFFLAG, "setuservalue"},
    {NEVER,  "sethook"},        /* used for memory and step limits */
    {IFFLAG, "setlocal"},
    {IFFLAG, "setmetatable"},
    {IFFLAG, "setupvalue"},
    {IFFLAG, "setcstacklimit"},
    {EOT, NULL}
};
static struct e ct_debug_safe[] = {
    {IFFLAG, "traceback"},
    {EOT, NULL}
};

/* possible ct_os_ tables */
static struct e ct_os_time[] = {
    {IFFLAG, "clock"},          /* is this portable? XXX */
    {IFFLAG, "date"},
    {IFFLAG, "difftime"},
    {IFFLAG, "time"},
    {EOT, NULL}
};

static struct e ct_os_files[] = {
    {NEVER, "execute"},         /* not portable */
    {NEVER, "exit"},
    {NEVER, "getenv"},
    {IFFLAG, "remove"},
    {IFFLAG, "rename"},
    {NEVER, "setlocale"},
    {NEVER, "tmpname"},
    {EOT, NULL}
};


#define DROPIF(flag, lib, ct) \
    nhl_clearfromtable(L, !!(lflags & flag), lib, ct)

static void
nhl_clearfromtable(lua_State *L, int flag, int tndx, struct e *todo)
{
    while (todo->when != EOT) {
        lua_pushnil(L);
        /* if we load the library at all, NEVER items must be erased
         * and IFFLAG items should be erased if !flag */
        if (todo->when == NEVER || !flag) {
            lua_setfield(L, tndx, todo->fnname);
        }
        todo++;
    }
}
#endif

/*
XXX
registry["org.nethack.nethack.sb.fs"][N]=
    CODEOBJECT
    {
        modepat: PATTERN,
        filepat: PATTERN
    }
CODEOBJECT
    if string then if pcall(string,mode, dir, file)
    if table then if mode matches pattern and filepat ma....
or do we use a real regex engine? (which we don't have and I just
 argued against adding)

return values from "call it":
    accept - file access granted
    reject - file access denied
    continue - try next element
    fail - error.  deny and call impossible/panic
*/

/* stack indexes:
 * -1       table to index with ename
 * params   file
 * params+1 [mode]
 */
/*
 * Problem: NetHack doesn't have a regex engine and Lua doesn't give
 *  C access to pattern matching.  There are 3 poor solutions:
 * 1. Import ~5K lines of code in a dozen files from FreeBSD. (Upside - we
 *    could use it in other places in NetHack.)
 * 2. Hack up lstrlib.c to give C direct access to the pattern matching code.
 * 3. Create a Lua state just to do pattern matching.
 * We're going to do #3.
 */
#ifdef notyet
static boolean
start_luapat(void)
{
    int rv;
/* XXX set memory and step limits */
    nhl_sandbox_info sbi = {NHL_SB_STRING, 0, 0, 0};

    if ((luapat = nhl_init(&sbi)) == NULL)
        return FALSE;

    /* load a pattern matching function */
    rv = luaL_loadstring(luapat,
                "function matches(s,p) return not not stringm.match(s,p) end");
    if (rv != LUA_OK) {
        panic("start_luapat: %d",rv);
    }
    return TRUE;
}
#endif

static void
end_luapat(void)
{
    if (luapat) {
        lua_close(luapat);
        luapat = NULL;
    }
}

#ifdef notyet
static int
opencheckpat(lua_State *L, const char *ename, int param)
{
    /* careful - we're using 2 different and unrelated Lua states */
    const char *string;
    int rv;

    lua_pushliteral(luapat, "matches");     /* function             -0,+1 */

    string = lua_tolstring(L, param, NULL); /* mode or filename     -0,+0 */
    lua_pushstring(luapat, string);                              /* -0,+1 */


    (void)lua_getfield(L, -1, ename);       /* pattern              -0,+1 */
    lua_pop(L, 1);                                               /* -1,+0 */
    string = lua_tolstring(L, -1, NULL);                         /* -0,+0 */
    lua_pushstring(luapat, string);                              /* -0,+1 */

    if (nhl_pcall(luapat, 2, 1)) {                               /* -3,+1 */
        /* impossible("access check internal error"); */
        return NHL_SBRV_FAIL;
    }
    rv = lua_toboolean(luapat, -1);                              /* -0,+0 */
#if 0
    if (lua_resetthread(luapat) != LUA_OK)
        return NHL_SBRV_FAIL;
is pop sufficient? XXX or wrong - look at the balance
#else
    lua_pop(luapat, 1);                                          /* -1,+0 */
#endif
    return rv ? NHL_SBRV_ACCEPT : NHL_SBRV_DENY;
}
#endif

/* put the table open uses to check its arguments on the top of the stack,
 * creating it if needed
 */
#define HOOKTBLNAME "org.nethack.nethack.sb.fs"
#ifdef notyet
static int (*io_open)(lua_State *) = NULL; /* XXX this may have to be in g TBD */
#endif

void
nhl_pushhooked_open_table(lua_State *L)
{
    int hot = lua_getfield(L, LUA_REGISTRYINDEX, HOOKTBLNAME);
    if (hot == LUA_TNONE) {
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, HOOKTBLNAME);
    }
}

#ifdef notyet
static int
hooked_open(lua_State *L)
{
    const char *mode;
    static boolean never = TRUE;
    const char *filename;
    int params;
    int hot;

    if (never) {
        if (!start_luapat())
            return NHL_SBRV_FAIL;
        never = FALSE;
    }
    filename = luaL_checkstring(L, 1);

    /* Unlike io.open, we want to treat mode as non-optional. */
    if (lua_gettop(L) < 2) {
        lua_pushstring(L, "r");
    }
    mode = luaL_optstring(L, 2, "r");

    /* sandbox checks */
    /* Do we need some ud from the calling state to let this be different for
       each call without redoing the HO table??  Maybe for version 2. XXX */

    params = lua_gettop(L)-1;  /* point at first param */
    nhl_pushhooked_open_table(L);
    hot = lua_gettop(L);

    if (lua_type(L, hot) == LUA_TTABLE) {
        int idx;
        for (idx = 1;
             lua_pushinteger(L, idx),
                 lua_geti(L, hot, idx),
                 !lua_isnoneornil(L, -1);
             ++idx) {
            /* top of stack is our configtbl[idx] */
            switch (lua_type(L, -1)) {
                /* lots of options to expand this with other types XXX */
            case LUA_TTABLE: {
                int moderv, filerv;
                moderv = opencheckpat(L, "modepat", params+1);
                if (moderv == NHL_SBRV_FAIL)
                    return moderv;
                filerv = opencheckpat(L, "filepat", params);
                if (filerv == NHL_SBRV_FAIL)
                    return moderv;
                if (filerv == moderv) {
                    if (filerv == NHL_SBRV_DENY)
                        return NHL_SBRV_DENY;
                    if (filerv == NHL_SBRV_ACCEPT)
                        goto doopen;
                }
                break; /* try next entry */
            }
            default:
                return NHL_SBRV_FAIL;
            }
        }
    } else
        return NHL_SBRV_DENY; /* default to "no" */

 doopen:
    lua_settop(L, params+1);
    return (*io_open)(L);
}

static boolean
hook_open(lua_State *L)
{
    boolean rv = FALSE;
    if (!io_open) {
        int tos = lua_gettop(L);
        lua_pushglobaltable(L);
        if (lua_getfield(L, -1, "io") != LUA_TTABLE)
            goto out;
        lua_getfield(L, -1, "open");
        /* The only way this can happen is if someone is messing with us,
         * and I'm not sure even that is possible. */
        if (!lua_iscfunction(L, -1))
            goto out;
        /* XXX This is fragile: C11 says casting func* to void*
         * doesn't have to work, but POSIX says it does.  So it
         * _should_ work everywhere but all we can do without messing
         * around inside Lua is to try to keep the compiler quiet. */
        io_open = (int (*)(lua_State *))lua_topointer(L, -1);
        lua_pushcfunction(L, hooked_open);
        lua_setfield(L, -1, "open");
        rv = TRUE;
 out:
        lua_settop(L, tos);
    }
    return rv;
}
#endif

DISABLE_WARNING_CONDEXPR_IS_CONSTANT

#ifdef NHL_SANDBOX
static void
nhlL_openlibs(lua_State *L, uint32_t lflags)
{
    /* translate lflags from user-friendly to internal */
    if (NHL_SB_DEBUGGING & lflags) {
        lflags |= NHL_SB_DB_SAFE;
    }
    /* only for debugging the sandbox integration */
    if (NHL_SB_ALL & lflags) {
        lflags = -1;
    } else if (NHL_SB_SAFE & lflags) {
        lflags |= NHL_SB_BASE_BASE;
        lflags |= NHL_SB_COROUTINE;
        lflags |= NHL_SB_TABLE;
        lflags |= NHL_SB_STRING;
        lflags |= NHL_SB_MATH;
        lflags |= NHL_SB_UTF8;
    } else if (NHL_SB_VERSION) {
        lflags |= NHL_SB_BASE_BASE;
    }
#ifdef notyet
/*  Handling I/O is complex, so it's not available yet.  I'll
finish it if and when we need it. (keni)
    - hooked open; array of tuples of (r/w/rw/a/etc, directory pat, file pat)

{"close", io_close},    but with no args closes default output, so needs hook
{"flush", io_flush},
{"lines", io_lines},    hook due to filename
{"open", io_open},  hooked version:
    only safe if mode not present or == "r"
        or WRITEIO
    only safe if path has no slashes
        XXX probably need to be: matches port-specific list of paths
        WRITEIO needs a different list
    dlb integration?
    may need to #define l_getc (but that wouldn't hook core)
        may need to #define fopen/fread/fwrite/feof/ftell (etc?)
        ugh: lauxlib.c uses getc() below luaL_loadfilex
            override in lua.h?
        ugh: liolib.c uses getc() below g_read->test_eof
            override in lua.h?
{"read", io_read},
{"type", io_type},
{"input", io_input},    safe with a complex hook, but may be needed for read?
WRITEIO:    needs changes to hooked open?
{"output", io_output},  do we want to allow access to default output?
  {"write", io_write},
UNSAFEIO:
  {"popen", io_popen},
  {"tmpfile", io_tmpfile},
*/
#endif

    if (lflags & NHL_SB_BASEMASK) {
        int baselib;
        /* load the entire library ... */
        luaL_requiref(L, LUA_GNAME, luaopen_base, 1);

        baselib = lua_gettop(L);

        /* ... and remove anything unsupported or not requested */
        DROPIF(NHL_SB_BASE_BASE, baselib, ct_base_base);
        DROPIF(NHL_SB_BASE_ERROR, baselib, ct_base_error);
        DROPIF(NHL_SB_BASE_META, baselib, ct_base_meta);
        DROPIF(NHL_SB_BASE_GC, baselib, ct_base_iffy);
        DROPIF(NHL_SB_BASE_UNSAFE, baselib, ct_base_unsafe);

        lua_pop(L, 1);
    }

    if (lflags & NHL_SB_COROUTINE) {
        luaL_requiref(L, LUA_COLIBNAME, luaopen_coroutine, 1);
        lua_pop(L, 1);
    }
    if (lflags & NHL_SB_TABLE) {
        luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1);
        lua_pop(L, 1);
    }
#ifdef notyet
    if (lflags & NHL_SB_IO) {
        luaL_requiref(L, LUA_IOLIBNAME, luaopen_io, 1);
        lua_pop(L, 1);
        if (!hook_open(L))
            panic("can't hook io.open");
    }
#endif
    if (lflags & NHL_SB_OSMASK) {
        int oslib;
        luaL_requiref(L, LUA_OSLIBNAME, luaopen_os, 1);
        oslib = lua_gettop(L);
        DROPIF(NHL_SB_OS_TIME, oslib, ct_os_time);
        DROPIF(NHL_SB_OS_FILES, oslib, ct_os_files);
        lua_pop(L, 1);
    }

    if (lflags & NHL_SB_STRING) {
        luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1);
        lua_pop(L, 1);
    }
    if (lflags & NHL_SB_MATH) {
        luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1);
        /* XXX Note that math.random uses Lua's built-in xoshiro256**
         * algorithm regardless of what the rest of the game uses.
         * Fixing this would require changing lmathlib.c. */
        lua_pop(L, 1);
    }
    if (lflags & NHL_SB_UTF8) {
        luaL_requiref(L, LUA_UTF8LIBNAME, luaopen_utf8, 1);
        lua_pop(L, 1);
    }
    if (lflags & NHL_SB_DBMASK) {
        int dblib;
        luaL_requiref(L, LUA_DBLIBNAME, luaopen_debug, 1);
        dblib = lua_gettop(L);
        DROPIF(NHL_SB_DB_DB, dblib, ct_debug_debug);
        DROPIF(NHL_SB_DB_SAFE, dblib, ct_debug_safe);
        lua_pop(L, 1);
    }
}
#endif

RESTORE_WARNING_CONDEXPR_IS_CONSTANT

/*
 * All we can do is approximate the amount of storage used.  Every allocator
 * has different overhead and uses that overhead differently.  Since we're
 * really just trying to prevent egregious use, we default to a minimum
 * allocation size of 16 and if you know better about your allocator (and
 * it's worth the processing time), it can be overridden.
 */
#ifndef NHL_ALLOC_ADJUST
#define NHL_ALLOC_ADJUST(d) d = (((d) + 15) & ~15)
#endif
static void *
nhl_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
    nhl_user_data *nud = ud;

    if (nud && nud->memlimit) { /* this state is size limited */
        uint32_t delta = !ptr ? nsize : nsize - osize;

        NHL_ALLOC_ADJUST(delta);
        nud->inuse += delta;
        if (nud->inuse > nud->memlimit)
            return 0;
    }

    if (nsize == 0) {
        free(ptr);
        return NULL;
    }

    return re_alloc(ptr, nsize);
}

DISABLE_WARNING_UNREACHABLE_CODE

static int
nhl_panic(lua_State *L)
{
    const char *msg = lua_tostring(L, -1);

    if (msg == NULL)
        msg = "error object is not a string";
    panic("unprotected error in call to Lua API (%s)\n", msg);
    /*NOTREACHED*/
    return 0; /* return to Lua to abort */
}

RESTORE_WARNING_UNREACHABLE_CODE

/* called when lua issues a warning message; the text of the message
   is passed to us in pieces across multiple function calls */
static void
nhl_warn(
    void *userdata UNUSED,
    const char *msg_fragment,
    int to_be_continued) /* 0: last fragment; 1: more to come */
{
    size_t fraglen, buflen = strlen(gl.lua_warnbuf);

    if (msg_fragment && buflen < sizeof gl.lua_warnbuf - 1) {
        fraglen = strlen(msg_fragment);
        if (buflen + fraglen > sizeof gl.lua_warnbuf - 1)
            fraglen = sizeof gl.lua_warnbuf - 1 - buflen;
        (void) strncat(gl.lua_warnbuf, msg_fragment, fraglen);
    }
    if (!to_be_continued) {
        paniclog("[lua]", gl.lua_warnbuf);
        gl.lua_warnbuf[0] = '\0';
    }
}

#ifdef NHL_SANDBOX
static void
nhl_hookfn(lua_State *L, lua_Debug *ar UNUSED)
{
    nhl_user_data *nud;

    (void) lua_getallocf(L, (void **) &nud);

    if (nud->steps <= NHL_SB_STEPSIZE)
        longjmp(nud->jb, 1);

    nud->steps -= NHL_SB_STEPSIZE;
}
#endif

static lua_State *
nhlL_newstate(nhl_sandbox_info *sbi)
{
    nhl_user_data *nud = 0;

    if (sbi->memlimit || sbi->steps) {
        nud = nhl_alloc(NULL, NULL, 0, sizeof (struct nhl_user_data));
        if (!nud)
            return 0;
        nud->memlimit = sbi->memlimit;
        nud->perpcall = 0; /* set up below, if needed */
        nud->steps = 0;
        nud->osteps = 0;
        nud->flags = sbi->flags; /* save reporting flags */
        uint32_t sz = sizeof (struct nhl_user_data);
        NHL_ALLOC_ADJUST(sz);
        nud->inuse = sz;
    }

    lua_State *L = lua_newstate(nhl_alloc, nud);

    lua_atpanic(L, nhl_panic);
#if LUA_VERSION_NUM == 504
    lua_setwarnf(L, nhl_warn, L);
#endif

#ifdef NHL_SANDBOX
    if (nud && (sbi->steps || sbi->perpcall)) {
        if (sbi->steps && sbi->perpcall)
            impossible("steps and perpcall both non-zero");
        if (sbi->perpcall) {
            nud->perpcall = sbi->perpcall;
        } else {
            nud->steps = sbi->steps;
            nud->osteps = sbi->steps;
        }
        lua_sethook(L, nhl_hookfn, LUA_MASKCOUNT, NHL_SB_STEPSIZE);
    }
#endif

    return L;
}

/*
(See end of comment for conclusion.)
to make packages safe, we need something like:
    if setuid/setgid (but does NH drop privs before we can check? TBD)
        unsetenv LUA_CPATH, LUA_CPATH_5_4 (and this needs to change with
          version) maybe more
    luaopen_package calls getenv
        unsetenv(LUA_PATH_VAR)
        unsetenv(LUA_CPATH_VAR)
        unsetenv(LUA_PATH_VAR LUA_VERSUFFIX)
        unsetenv(LUA_CPATH_VAR LUA_VERSUFFIX)
                    package.config
                    oackage[fieldname] = path
            NB: LUA_PATH_DEFAULT and LUA_CPATH_DEFAULT must be safe
                or we must setenv LUA_PATH_VAR and LUA_CPATH_VAR to something
                safe
            or we could just clean out the searchers table?
                package.searchers[preload,Lua,C,Croot]
also, can setting package.path to something odd get Lua to load files
 it shouldn't? (see docs package.searchers)
set (and disallow changing) package.cpath (etc?)
loadlib.c:
    lsys_load -> dlopen     Kill with undef LUA_USE_DLOPEN LUA_DL_DLL
    searchpath -> readable -> fopen
        <- ll_searchpath
        <- findfile <- {searchers C, Croot, Lua}
Probably the best thing to do is replace G.require with our own function
that does whatever it is we need and completely ignore the package library.
*/
/*
TODO:
docs
unfinished functionality & design
commit, cleanup, commit with SHA1 of full code version
BUT how do we compact the current history?
    new branch, then compress there
XXX
*/

/*nhlua.c*/
