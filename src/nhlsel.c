/* NetHack 3.7	nhlua.c	$NHDT-Date: 1582675449 2020/02/26 00:04:09 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.20 $ */
/*      Copyright (c) 2018 by Pasi Kallinen */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "sp_lev.h"


struct selectionvar *l_selection_check(lua_State *, int);
static struct selectionvar *l_selection_push_new(lua_State *);

/* lua_CFunction prototypes */
static int l_selection_new(lua_State *);
static int l_selection_clone(lua_State *);
static int l_selection_numpoints(lua_State *);
static int l_selection_getpoint(lua_State *);
static int l_selection_setpoint(lua_State *);
static int l_selection_filter_percent(lua_State *);
static int l_selection_rndcoord(lua_State *);
static int l_selection_room(lua_State *);
static int l_selection_getbounds(lua_State *);
static boolean params_sel_2coords(lua_State *, struct selectionvar **,
                                  coordxy *, coordxy *, coordxy *, coordxy *);
static int l_selection_line(lua_State *);
static int l_selection_randline(lua_State *);
static int l_selection_rect(lua_State *);
static int l_selection_fillrect(lua_State *);
static int l_selection_grow(lua_State *);
static int l_selection_filter_mapchar(lua_State *);
static int l_selection_match(lua_State *);
static int l_selection_flood(lua_State *);
static int l_selection_circle(lua_State *);
static int l_selection_ellipse(lua_State *);
static int l_selection_gradient(lua_State *);
static int l_selection_iterate(lua_State *);
static int l_selection_gc(lua_State *);
static int l_selection_not(lua_State *);
static int l_selection_and(lua_State *);
static int l_selection_or(lua_State *);
static int l_selection_xor(lua_State *);
/* There doesn't seem to be a point in having a l_selection_add since it would
 * do the same thing as l_selection_or. The addition operator is mapped to
 * l_selection_or. */
static int l_selection_sub(lua_State *);
#if 0
/* the following do not appear to currently be
   used and because they are static, the OSX
   compiler is complaining about them. I've
   if ifdef'd out the prototype here and the
   function body below.
 */
static int l_selection_ipairs(lua_State *);
static struct selectionvar *l_selection_to(lua_State *, int);
#endif

struct selectionvar *
l_selection_check(lua_State *L, int index)
{
    struct selectionvar *sel;

    luaL_checktype(L, index, LUA_TUSERDATA);
    sel = (struct selectionvar *) luaL_checkudata(L, index, "selection");
    if (!sel)
        nhl_error(L, "Selection error");
    return sel;
}

static int
l_selection_gc(lua_State *L)
{
    struct selectionvar *sel = l_selection_check(L, 1);

    if (sel)
        selection_free(sel, FALSE);
    return 0;
}

#if 0
static struct selectionvar *
l_selection_to(lua_State *L, int index)
{
    struct selectionvar *sel = (struct selectionvar *)lua_touserdata(L, index);

    if (!sel)
        nhl_error(L, "Selection error");
    return sel;
}
#endif

/* push a new selection into lua stack, return the selectionvar */
static struct selectionvar *
l_selection_push_new(lua_State *L)
{
    struct selectionvar *tmp = selection_new();
    struct selectionvar
        *sel = (struct selectionvar *) lua_newuserdata(L, sizeof(struct selectionvar));

    luaL_getmetatable(L, "selection");
    lua_setmetatable(L, -2);

    *sel = *tmp;
    sel->map = dupstr(tmp->map);
    selection_free(tmp, TRUE);

    return sel;
}

/* push a copy of selectionvar tmp to lua stack */
void
l_selection_push_copy(lua_State *L, struct selectionvar *tmp)
{
    struct selectionvar
        *sel = (struct selectionvar *) lua_newuserdata(L, sizeof(struct selectionvar));

    luaL_getmetatable(L, "selection");
    lua_setmetatable(L, -2);

    *sel = *tmp;
    sel->map = dupstr(tmp->map);
}


/* local sel = selection.new(); */
static int
l_selection_new(lua_State *L)
{
    (void) l_selection_push_new(L);
    return 1;
}

/* Replace the topmost selection in the stack with a clone of it. */
/* local sel = selection.clone(sel); */
static int
l_selection_clone(lua_State *L)
{
    struct selectionvar *sel = l_selection_check(L, 1);
    struct selectionvar *tmp;

    (void) l_selection_new(L);
    tmp = l_selection_check(L, 2);
    if (tmp->map)
        free(tmp->map);
    *tmp = *sel;
    tmp->map = dupstr(sel->map);
    return 1;
}

DISABLE_WARNING_UNREACHABLE_CODE

/* selection.set(sel, x, y); */
/* selection.set(sel, x, y, value); */
/* local sel = selection.set(); */
/* local sel = sel:set(); */
/* local sel = selection.set(sel); */
/* TODO: allow setting multiple coordinates at once: set({x,y}, {x,y}, ...); */
static int
l_selection_setpoint(lua_State *L)
{
    struct selectionvar *sel = (struct selectionvar *) 0;
    coordxy x = -1, y = -1;
    int val = 1;
    int argc = lua_gettop(L);
    long crd = 0L;

    if (argc == 0) {
        (void) l_selection_new(L);
    } else if (argc == 1) {
        sel = l_selection_check(L, 1);
    } else if (argc == 2) {
        x = (coordxy) luaL_checkinteger(L, 1);
        y = (coordxy) luaL_checkinteger(L, 2);
        lua_pop(L, 2);
        (void) l_selection_new(L);
        sel = l_selection_check(L, 1);
    } else {
        sel = l_selection_check(L, 1);
        x = (coordxy) luaL_checkinteger(L, 2);
        y = (coordxy) luaL_checkinteger(L, 3);
        val = (int) luaL_optinteger(L, 4, 1);
    }

    if (!sel || !sel->map) {
        nhl_error(L, "Selection setpoint error");
        /*NOTREACHED*/
        return 0;
    }

    if (x == -1 && y == -1)
        crd = SP_COORD_PACK_RANDOM(0);
    else
        crd = SP_COORD_PACK(x,y);
    get_location_coord(&x, &y, ANY_LOC,
                       gc.coder ? gc.coder->croom : NULL, crd);
    selection_setpoint(x, y, sel, val);
    lua_settop(L, 1);
    return 1;
}

/* local numpoints = selection.numpoints(sel); */
static int
l_selection_numpoints(lua_State *L)
{
    struct selectionvar *sel = l_selection_check(L, 1);
    coordxy x, y;
    int ret = 0;
    NhRect rect;

    selection_getbounds(sel, &rect);

    for (x = rect.lx; x <= rect.hx; x++)
        for (y = rect.ly; y <= rect.hy; y++)
            if (selection_getpoint(x, y, sel))
                ret++;

    lua_settop(L, 0);
    lua_pushinteger(L, ret);
    return 1;
}

/* local value = selection.get(sel, x, y); */
static int
l_selection_getpoint(lua_State *L)
{
    struct selectionvar *sel = l_selection_check(L, 1);
    coordxy x, y;
    lua_Integer ix, iy;
    int val;
    long crd;

    lua_remove(L, 1); /* sel */
    if (!nhl_get_xy_params(L, &ix, &iy)) {
        nhl_error(L, "l_selection_getpoint: Incorrect params");
        /*NOTREACHED*/
        return 0;
    }
    x = (coordxy) ix;
    y = (coordxy) iy;

    if (x == -1 && y == -1)
        crd = SP_COORD_PACK_RANDOM(0);
    else
        crd = SP_COORD_PACK(x,y);
    get_location_coord(&x, &y, ANY_LOC, gc.coder ? gc.coder->croom : NULL, crd);

    val = selection_getpoint(x, y, sel);
    lua_settop(L, 0);
    lua_pushnumber(L, val);
    return 1;
}

RESTORE_WARNING_UNREACHABLE_CODE

/* local s = selection.negate(sel); */
/* local s = selection.negate(); */
/* local s = sel:negate(); */
static int
l_selection_not(lua_State *L)
{
    int argc = lua_gettop(L);
    struct selectionvar *sel, *sel2;

    if (argc == 0) {
        (void) l_selection_new(L);
        sel = l_selection_check(L, 1);
        selection_clear(sel, 1);
    } else {
        sel = l_selection_check(L, 1);
        (void) l_selection_clone(L);
        sel2 = l_selection_check(L, 2);
        selection_not(sel2);
        lua_remove(L, 1);
    }
    return 1;
}

/* local sel = selection.area(4,5, 40,10) & selection.rect(7,8, 60,14); */
static int
l_selection_and(lua_State *L)
{
    int x,y;
    struct selectionvar *sela = l_selection_check(L, 1);
    struct selectionvar *selb = l_selection_check(L, 2);
    struct selectionvar *selr = l_selection_push_new(L);
    NhRect rect;

    rect_bounds(sela->bounds, selb->bounds, &rect);

    for (x = rect.lx; x <= rect.hx; x++)
        for (y = rect.ly; y <= rect.hy; y++) {
            int val = selection_getpoint(x, y, sela) & selection_getpoint(x, y, selb);
            selection_setpoint(x, y, selr, val);
        }

    lua_remove(L, 1);
    lua_remove(L, 1);
    return 1;
}

/* local sel = selection.area(4,5, 40,10) | selection.rect(7,8, 60,14); */
static int
l_selection_or(lua_State *L)
{
    int x,y;
    struct selectionvar *sela = l_selection_check(L, 1);
    struct selectionvar *selb = l_selection_check(L, 2);
    struct selectionvar *selr = l_selection_push_new(L);
    NhRect rect;

    rect_bounds(sela->bounds, selb->bounds, &rect);

    for (x = rect.lx; x <= rect.hx; x++)
        for (y = rect.ly; y <= rect.hy; y++) {
            int val = selection_getpoint(x, y, sela) | selection_getpoint(x, y, selb);
            selection_setpoint(x, y, selr, val);
        }
    selr->bounds = rect;

    lua_remove(L, 1);
    lua_remove(L, 1);
    return 1;
}

/* local sel = selection.area(4,5, 40,10) ~ selection.rect(7,8, 60,14); */
static int
l_selection_xor(lua_State *L)
{
    int x,y;
    struct selectionvar *sela = l_selection_check(L, 1);
    struct selectionvar *selb = l_selection_check(L, 2);
    struct selectionvar *selr = l_selection_push_new(L);
    NhRect rect;

    rect_bounds(sela->bounds, selb->bounds, &rect);

    for (x = rect.lx; x <= rect.hx; x++)
        for (y = rect.ly; y <= rect.hy; y++) {
            int val = selection_getpoint(x, y, sela) ^ selection_getpoint(x, y, selb);
            selection_setpoint(x, y, selr, val);
        }

    lua_remove(L, 1);
    lua_remove(L, 1);
    return 1;
}

/* local sel = selection.area(10,10, 20,20) - selection.area(14,14, 17,17)
 *   - i.e. points that are in A but not in B */
static int
l_selection_sub(lua_State *L)
{
    int x,y;
    struct selectionvar *sela = l_selection_check(L, 1);
    struct selectionvar *selb = l_selection_check(L, 2);
    struct selectionvar *selr = l_selection_push_new(L);
    NhRect rect;

    rect_bounds(sela->bounds, selb->bounds, &rect);

    for (x = rect.lx; x <= rect.hx; x++)
        for (y = rect.ly; y <= rect.hy; y++) {
            coordxy a_pt = selection_getpoint(x, y, sela);
            coordxy b_pt = selection_getpoint(x, y, selb);
            int val = (a_pt ^ b_pt) & a_pt;
            selection_setpoint(x, y, selr, val);
        }

    lua_remove(L, 1);
    lua_remove(L, 1);
    return 1;
}

/* local s = selection.percentage(sel, 50); */
static int
l_selection_filter_percent(lua_State *L)
{
    int argc = lua_gettop(L);
    struct selectionvar *sel = l_selection_check(L, 1);
    int p = (int) luaL_checkinteger(L, 2);
    struct selectionvar *tmp;

    tmp = selection_filter_percent(sel, p);
    lua_pop(L, argc);
    l_selection_push_copy(L, tmp);
    selection_free(tmp, TRUE);

    return 1;
}

/* local pt = selection.rndcoord(sel); */
/* local pt = selection.rndcoord(sel, 1); */
static int
l_selection_rndcoord(lua_State *L)
{
    struct selectionvar *sel = l_selection_check(L, 1);
    int removeit = (int) luaL_optinteger(L, 2, 0);
    coordxy x = -1, y = -1;
    selection_rndcoord(sel, &x, &y, removeit);
    if (!(x == -1 && y == -1)) {
        update_croom();
        if (gc.coder && gc.coder->croom) {
            x -= gc.coder->croom->lx;
            y -= gc.coder->croom->ly;
        } else {
            x -= gx.xstart;
            y -= gy.ystart;
        }
    }
    lua_settop(L, 0);
    lua_newtable(L);
    nhl_add_table_entry_int(L, "x", x);
    nhl_add_table_entry_int(L, "y", y);
    return 1;
}

/* local s = selection.room(); */
static int
l_selection_room(lua_State *L)
{
    struct selectionvar *sel;
    int argc = lua_gettop(L);
    struct mkroom *croom = NULL;

    if (argc == 1) {
        int i = luaL_checkinteger(L, -1);

        croom = (i >= 0 && i < gn.nroom) ? &gr.rooms[i] : NULL;
    }

    sel = selection_from_mkroom(croom);

    l_selection_push_copy(L, sel);
    selection_free(sel, TRUE);

    return 1;
}

/* local rect = sel:bounds(); */
static int
l_selection_getbounds(lua_State *L)
{
    struct selectionvar *sel = l_selection_check(L, 1);
    NhRect rect;

    selection_getbounds(sel, &rect);
    lua_settop(L, 0);
    lua_newtable(L);
    nhl_add_table_entry_int(L, "lx", rect.lx);
    nhl_add_table_entry_int(L, "ly", rect.ly);
    nhl_add_table_entry_int(L, "hx", rect.hx);
    nhl_add_table_entry_int(L, "hy", rect.hy);
    return 1;
}

/* internal function to get a selection and 4 integer values from lua stack.
   removes the integers from the stack.
   returns TRUE if params are good.
*/
/* function(selection, x1,y1, x2,y2) */
/* selection:function(x1,y1, x2,y2) */
static boolean
params_sel_2coords(lua_State *L, struct selectionvar **sel,
                   coordxy *x1, coordxy *y1, coordxy *x2, coordxy *y2)
{
    int argc = lua_gettop(L);

    if (argc == 4) {
        (void) l_selection_new(L);
        *x1 = (coordxy) luaL_checkinteger(L, 1);
        *y1 = (coordxy) luaL_checkinteger(L, 2);
        *x2 = (coordxy) luaL_checkinteger(L, 3);
        *y2 = (coordxy) luaL_checkinteger(L, 4);
        *sel = l_selection_check(L, 5);
        lua_remove(L, 1);
        lua_remove(L, 1);
        lua_remove(L, 1);
        lua_remove(L, 1);
        return TRUE;
    } else if (argc == 5) {
        *sel = l_selection_check(L, 1);
        *x1 = (coordxy) luaL_checkinteger(L, 2);
        *y1 = (coordxy) luaL_checkinteger(L, 3);
        *x2 = (coordxy) luaL_checkinteger(L, 4);
        *y2 = (coordxy) luaL_checkinteger(L, 5);
        lua_pop(L, 4);
        return TRUE;
    }
    return FALSE;
}

/* local s = selection.line(sel, x1,y1, x2,y2); */
/* local s = selection.line(x1,y1, x2,y2); */
/* s:line(x1,y1, x2,y2); */
static int
l_selection_line(lua_State *L)
{
    struct selectionvar *sel = NULL;
    coordxy x1, y1, x2, y2;

    if (!params_sel_2coords(L, &sel, &x1, &y1, &x2, &y2)) {
        nhl_error(L, "selection.line: illegal arguments");
    }

    get_location_coord(&x1, &y1, ANY_LOC, gc.coder ? gc.coder->croom : NULL, SP_COORD_PACK(x1,y1));
    get_location_coord(&x2, &y2, ANY_LOC, gc.coder ? gc.coder->croom : NULL, SP_COORD_PACK(x2,y2));

    (void) l_selection_clone(L);
    sel = l_selection_check(L, 2);
    selection_do_line(x1,y1,x2,y2, sel);
    return 1;
}

/* local s = selection.rect(sel, x1,y1, x2,y2); */
static int
l_selection_rect(lua_State *L)
{
    struct selectionvar *sel = NULL;
    coordxy x1, y1, x2, y2;

    if (!params_sel_2coords(L, &sel, &x1, &y1, &x2, &y2)) {
        nhl_error(L, "selection.rect: illegal arguments");
    }

    get_location_coord(&x1, &y1, ANY_LOC, gc.coder ? gc.coder->croom : NULL,
                       SP_COORD_PACK(x1, y1));
    get_location_coord(&x2, &y2, ANY_LOC, gc.coder ? gc.coder->croom : NULL,
                       SP_COORD_PACK(x2, y2));

    (void) l_selection_clone(L);
    sel = l_selection_check(L, 2);
    selection_do_line(x1, y1, x2, y1, sel);
    selection_do_line(x1, y1, x1, y2, sel);
    selection_do_line(x2, y1, x2, y2, sel);
    selection_do_line(x1, y2, x2, y2, sel);
    return 1;
}

/* local s = selection.fillrect(sel, x1,y1, x2,y2); */
/* local s = selection.fillrect(x1,y1, x2,y2); */
/* s:fillrect(x1,y1, x2,y2); */
/* selection.area(x1,y1, x2,y2); */
static int
l_selection_fillrect(lua_State *L)
{
    struct selectionvar *sel = NULL;
    int y;
    coordxy x1, y1, x2, y2;

    if (!params_sel_2coords(L, &sel, &x1, &y1, &x2, &y2)) {
        nhl_error(L, "selection.fillrect: illegal arguments");
    }

    get_location_coord(&x1, &y1, ANY_LOC, gc.coder ? gc.coder->croom : NULL,
                       SP_COORD_PACK(x1, y1));
    get_location_coord(&x2, &y2, ANY_LOC, gc.coder ? gc.coder->croom : NULL,
                       SP_COORD_PACK(x2, y2));

    (void) l_selection_clone(L);
    sel = l_selection_check(L, 2);
    if (x1 == x2) {
        for (y = y1; y <= y2; y++)
            selection_setpoint(x1, y, sel, 1);
    } else {
        for (y = y1; y <= y2; y++)
            selection_do_line(x1, y, x2, y, sel);
    }
    return 1;
}

/* local s = selection.randline(sel, x1,y1, x2,y2, roughness); */
/* local s = selection.randline(x1,y1, x2,y2, roughness); */
/* TODO: selection.randline(x1,y1, x2,y2, roughness); */
/* TODO: selection.randline({x1,y1}, {x2,y2}, roughness); */
static int
l_selection_randline(lua_State *L)
{
    int argc = lua_gettop(L);
    struct selectionvar *sel = (struct selectionvar *) 0;
    coordxy x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    int roughness = 7;

    if (argc == 6) {
        sel = l_selection_check(L, 1);
        x1 = (coordxy) luaL_checkinteger(L, 2);
        y1 = (coordxy) luaL_checkinteger(L, 3);
        x2 = (coordxy) luaL_checkinteger(L, 4);
        y2 = (coordxy) luaL_checkinteger(L, 5);
        roughness = (int) luaL_checkinteger(L, 6);
        lua_pop(L, 5);
    } else if (argc == 5 && lua_type(L, 1) == LUA_TNUMBER) {
        x1 = (coordxy) luaL_checkinteger(L, 1);
        y1 = (coordxy) luaL_checkinteger(L, 2);
        x2 = (coordxy) luaL_checkinteger(L, 3);
        y2 = (coordxy) luaL_checkinteger(L, 4);
        roughness = (int) luaL_checkinteger(L, 5);
        lua_pop(L, 5);
        (void) l_selection_new(L);
        sel = l_selection_check(L, 1);
    }

    get_location_coord(&x1, &y1, ANY_LOC,
                       gc.coder ? gc.coder->croom : NULL, SP_COORD_PACK(x1, y1));
    get_location_coord(&x2, &y2, ANY_LOC,
                       gc.coder ? gc.coder->croom : NULL, SP_COORD_PACK(x2, y2));

    (void) l_selection_clone(L);
    sel = l_selection_check(L, 2);
    selection_do_randline(x1, y1, x2, y2, roughness, 12, sel);
    return 1;
}

/* local s = selection.grow(sel); */
/* local s = selection.grow(sel, "north"); */
static int
l_selection_grow(lua_State *L)
{
    int argc = lua_gettop(L);
    const char *const growdirs[] = { "all", "random", "north", "west", "east", "south", NULL };
    const int growdirs2i[] = { W_ANY, W_RANDOM, W_NORTH, W_WEST, W_EAST, W_SOUTH, 0 };

    struct selectionvar *sel = l_selection_check(L, 1);
    int dir = growdirs2i[luaL_checkoption(L, 2, "all", growdirs)];

    if (argc == 2)
        lua_pop(L, 1); /* get rid of growdir */

    (void) l_selection_clone(L);
    sel = l_selection_check(L, 2);
    selection_do_grow(sel, dir);
    return 1;
}


/* local s = selection.filter_mapchar(sel, mapchar, lit); */
static int
l_selection_filter_mapchar(lua_State *L)
{
    int argc = lua_gettop(L);
    struct selectionvar *sel = l_selection_check(L, 1);
    char *mapchr = dupstr(luaL_checkstring(L, 2));
    coordxy typ = check_mapchr(mapchr);
    int lit = (int) luaL_optinteger(L, 3, -2); /* TODO: special lit values */
    struct selectionvar *tmp;

    if (typ == INVALID_TYPE)
        nhl_error(L, "Erroneous map char");

    tmp = selection_filter_mapchar(sel, typ, lit);
    lua_pop(L, argc);
    l_selection_push_copy(L, tmp);
    selection_free(tmp, TRUE);

    if (mapchr)
        free(mapchr);

    return 1;
}

/* local s = selection.match([[...]]); */
static int
l_selection_match(lua_State *L)
{
    int argc = lua_gettop(L);
    struct selectionvar *sel = (struct selectionvar *) 0;
    struct mapfragment *mf = (struct mapfragment *) 0;
    int x, y;

    if (argc == 1) {
        const char *err;
        char *mapstr = dupstr(luaL_checkstring(L, 1));
        lua_pop(L, 1);
        (void) l_selection_new(L);
        sel = l_selection_check(L, 1);

        mf = mapfrag_fromstr(mapstr);
        free(mapstr);

        if ((err = mapfrag_error(mf)) != NULL) {
            nhl_error(L, err);
            /*NOTREACHED*/
        }

    } else {
        nhl_error(L, "wrong parameters");
        /*NOTREACHED*/
    }

    for (y = 0; y <= sel->hei; y++)
        for (x = 1; x < sel->wid; x++)
            selection_setpoint(x, y, sel, mapfrag_match(mf, x,y) ? 1 : 0);

    mapfrag_free(&mf);

    return 1;
}


/* local s = selection.floodfill(x,y); */
/* local s = selection.floodfill(x,y, diagonals); */
static int
l_selection_flood(lua_State *L)
{
    int argc = lua_gettop(L);
    struct selectionvar *sel = (struct selectionvar *) 0;
    coordxy x = 0, y = 0;
    boolean diagonals = FALSE;

    if (argc == 2 || argc == 3) {
        x = (coordxy) luaL_checkinteger(L, 1);
        y = (coordxy) luaL_checkinteger(L, 2);
        if (argc == 3)
            diagonals = lua_toboolean(L, 3);
        lua_pop(L, argc);
        (void) l_selection_new(L);
        sel = l_selection_check(L, 1);
    } else {
        nhl_error(L, "wrong parameters");
        /*NOTREACHED*/
    }

    get_location_coord(&x, &y, ANY_LOC,
                       gc.coder ? gc.coder->croom : NULL, SP_COORD_PACK(x, y));

    if (isok(x, y)) {
        set_floodfillchk_match_under(levl[x][y].typ);
        selection_floodfill(sel, x, y, diagonals);
    }
    return 1;
}


/* local s = selection.circle(x,y, radius); */
/* local s = selection.circle(x, y, radius, filled); */
/* local s = selection.circle(sel, x, y, radius); */
/* local s = selection.circle(sel, x, y, radius, filled); */
static int
l_selection_circle(lua_State *L)
{
    int argc = lua_gettop(L);
    struct selectionvar *sel = (struct selectionvar *) 0;
    coordxy x = 0, y = 0;
    int r = 0, filled = 0;

    if (argc == 3) {
        x = (coordxy) luaL_checkinteger(L, 1);
        y = (coordxy) luaL_checkinteger(L, 2);
        r = (int) luaL_checkinteger(L, 3);
        lua_pop(L, 3);
        (void) l_selection_new(L);
        sel = l_selection_check(L, 1);
        filled = 0;
    } else if (argc == 4 && lua_type(L, 1) == LUA_TNUMBER) {
        x = (coordxy) luaL_checkinteger(L, 1);
        y = (coordxy) luaL_checkinteger(L, 2);
        r = (int) luaL_checkinteger(L, 3);
        filled = (int) luaL_checkinteger(L, 4); /* TODO: boolean*/
        lua_pop(L, 4);
        (void) l_selection_new(L);
        sel = l_selection_check(L, 1);
    } else if (argc == 4 || argc == 5) {
        sel = l_selection_check(L, 1);
        x = (coordxy) luaL_checkinteger(L, 2);
        y = (coordxy) luaL_checkinteger(L, 3);
        r = (int) luaL_checkinteger(L, 4);
        filled = (int) luaL_optinteger(L, 5, 0); /* TODO: boolean */
    } else {
        nhl_error(L, "wrong parameters");
        /*NOTREACHED*/
    }

    get_location_coord(&x, &y, ANY_LOC,
                       gc.coder ? gc.coder->croom : NULL, SP_COORD_PACK(x, y));

    selection_do_ellipse(sel, x, y, r, r, !filled);

    lua_settop(L, 1);
    return 1;
}

/* local s = selection.ellipse(x, y, radius1, radius2); */
/* local s = selection.ellipse(x, y, radius1, radius2, filled); */
/* local s = selection.ellipse(sel, x, y, radius1, radius2); */
/* local s = selection.ellipse(sel, x, y, radius1, radius2, filled); */
static int
l_selection_ellipse(lua_State *L)
{
    int argc = lua_gettop(L);
    struct selectionvar *sel = (struct selectionvar *) 0;
    coordxy x = 0, y = 0;
    int r1 = 0, r2 = 0, filled = 0;

    if (argc == 4) {
        x = (coordxy) luaL_checkinteger(L, 1);
        y = (coordxy) luaL_checkinteger(L, 2);
        r1 = (int) luaL_checkinteger(L, 3);
        r2 = (int) luaL_checkinteger(L, 4);
        lua_pop(L, 4);
        (void) l_selection_new(L);
        sel = l_selection_check(L, 1);
        filled = 0;
    } else if (argc == 5 && lua_type(L, 1) == LUA_TNUMBER) {
        x = (coordxy) luaL_checkinteger(L, 1);
        y = (coordxy) luaL_checkinteger(L, 2);
        r1 = (int) luaL_checkinteger(L, 3);
        r2 = (int) luaL_checkinteger(L, 4);
        filled = (int) luaL_optinteger(L, 5, 0); /* TODO: boolean */
        lua_pop(L, 5);
        (void) l_selection_new(L);
        sel = l_selection_check(L, 1);
    } else if (argc == 5 || argc == 6) {
        sel = l_selection_check(L, 1);
        x = (coordxy) luaL_checkinteger(L, 2);
        y = (coordxy) luaL_checkinteger(L, 3);
        r1 = (int) luaL_checkinteger(L, 4);
        r2 = (int) luaL_checkinteger(L, 5);
        filled = (int) luaL_optinteger(L, 6, 0); /* TODO: boolean */
    } else {
        nhl_error(L, "wrong parameters");
        /*NOTREACHED*/
    }

    get_location_coord(&x, &y, ANY_LOC,
                       gc.coder ? gc.coder->croom : NULL, SP_COORD_PACK(x, y));

    selection_do_ellipse(sel, x, y, r1, r2, !filled);

    lua_settop(L, 1);
    return 1;
}

/* Gradients are versatile enough, with so many independently optional
 * arguments, that it doesn't seem helpful to provide a non-table form with
 * non-obvious argument order. */
/* selection.gradient({ type = "radial", x = 3, y = 5, x2 = 10, y2 = 12,
 *                      mindist = 4, maxdist = 10, limited = false });    */
static int
l_selection_gradient(lua_State *L)
{
    int argc = lua_gettop(L);
    struct selectionvar *sel = (struct selectionvar *) 0;
    /* if x2 and y2 aren't set, the gradient has a single center point of x,y;
     * if they are set, the gradient is centered on a (x,y) to (x2,y2) line */
    coordxy x = 0, y = 0, x2 = -1, y2 = -1;
    /* points are always added within mindist of the center; the chance for a
     * point between mindist and maxdist to be added to the selection starts at
     * 100% at mindist and decreases linearly to 0% at maxdist */
    coordxy mindist = 0, maxdist = 0;
    long type = 0;
    static const char *const gradtypes[] = {
        "radial", "square", NULL
    };
    static const int gradtypes2i[] = {
        SEL_GRADIENT_RADIAL, SEL_GRADIENT_SQUARE, -1
    };

    if (argc == 1 && lua_type(L, 1) == LUA_TTABLE) {
        lcheck_param_table(L);
        type = gradtypes2i[get_table_option(L, "type", "radial", gradtypes)];
        x = (coordxy) get_table_int(L, "x");
        y = (coordxy) get_table_int(L, "y");
        x2 = (coordxy) get_table_int_opt(L, "x2", -1);
        y2 = (coordxy) get_table_int_opt(L, "y2", -1);
        cvt_to_abscoord(&x, &y);
        cvt_to_abscoord(&x2, &y2);
        /* maxdist is required because there's no obvious default value for it,
         * whereas mindist has an obvious defalt of 0 */
        maxdist = get_table_int(L, "maxdist");
        mindist = get_table_int_opt(L, "mindist", 0);

        lua_pop(L, 1);
        (void) l_selection_new(L);
        sel = l_selection_check(L, 1);
    } else {
        nhl_error(L, "selection.gradient requires table argument");
        /* NOTREACHED */
    }

    /* someone might conceivably want to draw a gradient somewhere off-map. So
     * the only coordinate that's "illegal" for that is (-1,-1).
     * If a level designer really needs to draw a gradient line using that
     * coordinate, they can do so by setting regular x and y to -1. */
    if (x2 == -1 && y2 == -1) {
        x2 = x;
        y2 = y;
    }

    selection_do_gradient(sel, x, y, x2, y2, type, mindist, maxdist);
    lua_settop(L, 1);
    return 1;
}

/* sel:iterate(function(x,y) ... end);
 * The x, y coordinates passed to the function are map- or room-relative
 * rather than absolute, unless there has been no previous map or room defined.
 */
static int
l_selection_iterate(lua_State *L)
{
    int argc = lua_gettop(L);
    struct selectionvar *sel = (struct selectionvar *) 0;
    int x, y;
    NhRect rect;

    if (argc == 2 && lua_type(L, 2) == LUA_TFUNCTION) {
        sel = l_selection_check(L, 1);
        selection_getbounds(sel, &rect);
        for (y = rect.ly; y <= rect.hy; y++)
            for (x = max(1,rect.lx); x <= rect.hx; x++)
                if (selection_getpoint(x, y, sel)) {
                    coordxy tmpx = x, tmpy = y;
                    cvt_to_relcoord(&tmpx, &tmpy);
                    lua_pushvalue(L, 2);
                    lua_pushinteger(L, tmpx);
                    lua_pushinteger(L, tmpy);
                    if (nhl_pcall(L, 2, 0)) {
                        impossible("Lua error: %s", lua_tostring(L, -1));
                        /* abort the loops to prevent possible error cascade */
                        goto out;
                    }
                }
    } else {
        nhl_error(L, "wrong parameters");
        /*NOTREACHED*/
    }
out:
    return 0;
}


static const struct luaL_Reg l_selection_methods[] = {
    { "new", l_selection_new },
    { "clone", l_selection_clone },
    { "get", l_selection_getpoint },
    { "set", l_selection_setpoint },
    { "numpoints", l_selection_numpoints },
    { "negate", l_selection_not },
    { "percentage", l_selection_filter_percent },
    { "rndcoord", l_selection_rndcoord },
    { "line", l_selection_line },
    { "randline", l_selection_randline },
    { "rect", l_selection_rect },
    { "fillrect", l_selection_fillrect },
    { "area", l_selection_fillrect },
    { "grow", l_selection_grow },
    { "filter_mapchar", l_selection_filter_mapchar },
    { "match", l_selection_match },
    { "floodfill", l_selection_flood },
    { "circle", l_selection_circle },
    { "ellipse", l_selection_ellipse },
    { "gradient", l_selection_gradient },
    { "iterate", l_selection_iterate },
    { "bounds", l_selection_getbounds },
    { "room", l_selection_room },
    { NULL, NULL }
};

static const luaL_Reg l_selection_meta[] = {
    { "__gc", l_selection_gc },
    { "__unm", l_selection_not },
    { "__band", l_selection_and },
    { "__bor", l_selection_or },
    { "__bxor", l_selection_xor },
    { "__bnot", l_selection_not },
    { "__add", l_selection_or }, /* this aliases + to be the same as | */
    { "__sub", l_selection_sub },
    /* TODO: http://lua-users.org/wiki/MetatableEvents
       { "__ipairs", l_selection_ipairs },
    */
    { NULL, NULL }
};

int
l_selection_register(lua_State *L)
{
    /* Table of instance methods and static methods. */
    luaL_newlib(L, l_selection_methods);

    /* metatable = { __name = "selection", __gc = l_selection_gc } */
    luaL_newmetatable(L, "selection");
    luaL_setfuncs(L, l_selection_meta, 0);

    /* metatable.__index points at the selection method table. */
    lua_pushvalue(L, -2);
    lua_setfield(L, -2, "__index");

    /* Don't let lua code mess with the real metatable.
       Instead offer a fake one that only contains __gc. */
    luaL_newlib(L, l_selection_meta);
    lua_setfield(L, -2, "__metatable");

    /* We don't need the metatable anymore. It's safe in the
       Lua registry for use by luaL_setmetatable. */
    lua_pop(L, 1);

    /* global selection = the method table we created at the start */
    lua_setglobal(L, "selection");

    return 0;
}
