/* NetHack 3.7	nhlua.c	$NHDT-Date: 1582675449 2020/02/26 00:04:09 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.20 $ */
/*      Copyright (c) 2018 by Pasi Kallinen */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "sp_lev.h"

/* lua_CFunction prototypes */
static int FDECL(l_selection_new, (lua_State *));
static int FDECL(l_selection_clone, (lua_State *));
static int FDECL(l_selection_getpoint, (lua_State *));
static int FDECL(l_selection_setpoint, (lua_State *));
static int FDECL(l_selection_not, (lua_State *));
static int FDECL(l_selection_filter_percent, (lua_State *));
static int FDECL(l_selection_rndcoord, (lua_State *));
static boolean FDECL(params_sel_2coords, (lua_State *, struct selectionvar **,
                                          schar *, schar *, schar *, schar *));
static int FDECL(l_selection_line, (lua_State *));
static int FDECL(l_selection_randline, (lua_State *));
static int FDECL(l_selection_rect, (lua_State *));
static int FDECL(l_selection_fillrect, (lua_State *));
static int FDECL(l_selection_fillrect, (lua_State *));
static int FDECL(l_selection_grow, (lua_State *));
static int FDECL(l_selection_filter_mapchar, (lua_State *));
static int FDECL(l_selection_flood, (lua_State *));
static int FDECL(l_selection_circle, (lua_State *));
static int FDECL(l_selection_ellipse, (lua_State *));
static int FDECL(l_selection_iterate, (lua_State *));
static int FDECL(l_selection_gc, (lua_State *));
static int FDECL(l_selection_not, (lua_State *));
static int FDECL(l_selection_and, (lua_State *));
static int FDECL(l_selection_or, (lua_State *));
static int FDECL(l_selection_xor, (lua_State *));
static int FDECL(l_selection_not, (lua_State *));
#if 0
/* the following do not appear to currently be
   used and because they are static, the OSX
   compiler is complaining about them. I've
   if ifdef'd out the prototype here and the
   function body below.
 */
static int FDECL(l_selection_gradient, (lua_State *));
static int FDECL(l_selection_add, (lua_State *));
static int FDECL(l_selection_sub, (lua_State *));
static int FDECL(l_selection_ipairs, (lua_State *));
static struct selectionvar *FDECL(l_selection_to, (lua_State *, int));
#endif

struct selectionvar *
l_selection_check(L, index)
lua_State *L;
int index;
{
    struct selectionvar *sel;

    luaL_checktype(L, index, LUA_TUSERDATA);
    sel = (struct selectionvar *) luaL_checkudata(L, index, "selection");
    if (!sel)
        nhl_error(L, "Selection error");
    return sel;
}

static int
l_selection_gc(L)
lua_State *L;
{
    struct selectionvar *sel = l_selection_check(L, 1);

    if (sel)
        selection_free(sel, FALSE);
    return 0;
}

#if 0
static struct selectionvar *
l_selection_to(L, index)
lua_State *L;
int index;
{
    struct selectionvar *sel = (struct selectionvar *)lua_touserdata(L, index);

    if (!sel)
        nhl_error(L, "Selection error");
    return sel;
}
#endif

static struct selectionvar *
l_selection_push(L)
lua_State *L;
{
    struct selectionvar *tmp = selection_new();
    struct selectionvar
        *sel = (struct selectionvar *) lua_newuserdata(L, sizeof(struct selectionvar));

    luaL_getmetatable(L, "selection");
    lua_setmetatable(L, -2);

    sel->wid = tmp->wid;
    sel->hei = tmp->hei;
    sel->map = dupstr(tmp->map);
    selection_free(tmp, TRUE);

    return sel;
}

/* local sel = selection.new(); */
static int
l_selection_new(L)
lua_State *L;
{
    (void) l_selection_push(L);
    return 1;
}

/* Replace the topmost selection in the stack with a clone of it. */
/* local sel = selection.clone(sel); */
static int
l_selection_clone(L)
lua_State *L;
{
    struct selectionvar *sel = l_selection_check(L, 1);
    struct selectionvar *tmp;
    lua_pop(L, 1);
    (void) l_selection_new(L);
    tmp = l_selection_check(L, 1);
    if (tmp->map)
        free(tmp->map);
    tmp->map = dupstr(sel->map);
    tmp->wid = sel->wid;
    tmp->hei = sel->hei;
    return 1;
}

/* selection.set(sel, x, y); */
/* selection.set(sel, x, y, value); */
/* local sel = selection.set(); */
/* local sel = sel:set(); */
/* local sel = selection.set(sel); */
/* TODO: allow setting multiple coordinates at once: set({x,y}, {x,y}, ...); */
static int
l_selection_setpoint(L)
lua_State *L;
{
    struct selectionvar *sel = (struct selectionvar *) 0;
    schar x = -1, y = -1;
    int val = 1;
    int argc = lua_gettop(L);
    long coord = 0L;

    if (argc == 0) {
        (void) l_selection_new(L);
    } else if (argc == 1) {
        sel = l_selection_check(L, 1);
    } else if (argc == 2) {
        x = (schar) luaL_checkinteger(L, 1);
        y = (schar) luaL_checkinteger(L, 2);
        lua_pop(L, 2);
        (void) l_selection_new(L);
        sel = l_selection_check(L, 1);
    } else {
        sel = l_selection_check(L, 1);
        x = (schar) luaL_checkinteger(L, 2);
        y = (schar) luaL_checkinteger(L, 3);
        val = (int) luaL_optinteger(L, 4, 1);
    }

    if (!sel || !sel->map) {
        nhl_error(L, "Selection setpoint error");
        /*NOTREACHED*/
        return 0;
    }

    if (x == -1 && y == -1)
        coord = SP_COORD_PACK_RANDOM(0);
    else
        coord = SP_COORD_PACK(x,y);
    get_location_coord(&x, &y, ANY_LOC,
                       g.coder ? g.coder->croom : NULL, coord);
    selection_setpoint(x, y, sel, val);
    lua_settop(L, 1);
    return 1;
}

/* local value = selection.get(sel, x, y); */
static int
l_selection_getpoint(L)
lua_State *L;
{
    struct selectionvar *sel = l_selection_check(L, 1);
    schar x = (schar) luaL_checkinteger(L, 2);
    schar y = (schar) luaL_checkinteger(L, 3);
    int val;
    long coord;

    if (x == -1 && y == -1)
        coord = SP_COORD_PACK_RANDOM(0);
    else
        coord = SP_COORD_PACK(x,y);
    get_location_coord(&x, &y, ANY_LOC, g.coder ? g.coder->croom : NULL, coord);

    val = selection_getpoint(x, y, sel);
    lua_settop(L, 0);
    lua_pushnumber(L, val);
    return 1;
}

/* local s = selection.negate(sel); */
/* local s = selection.negate(); */
/* local s = sel:negate(); */
static int
l_selection_not(L)
lua_State *L;
{
    int argc = lua_gettop(L);
    struct selectionvar *sel, *sel2;

    if (argc == 0) {
        (void) l_selection_new(L);
        sel = l_selection_check(L, 1);
        selection_not(sel);
    } else {
        sel = l_selection_check(L, 1);
        (void) l_selection_clone(L);
        sel2 = l_selection_check(L, 1);
        selection_not(sel2);
    }
    lua_settop(L, 1);
    return 1;
}

/* local sel = selection.area(4,5, 40,10) & selection.rect(7,8, 60,14); */
static int
l_selection_and(L)
lua_State *L;
{
    int x,y;
    struct selectionvar *sela = l_selection_check(L, 1);
    struct selectionvar *selb = l_selection_check(L, 2);
    struct selectionvar *selr = l_selection_push(L);

    for (x = 0; x < selr->wid; x++)
        for (y = 0; y < selr->hei; y++) {
            int val = selection_getpoint(x, y, sela) & selection_getpoint(x, y, selb);
            selection_setpoint(x, y, selr, val);
        }

    lua_remove(L, 1);
    lua_remove(L, 1);
    return 1;
}

/* local sel = selection.area(4,5, 40,10) | selection.rect(7,8, 60,14); */
static int
l_selection_or(L)
lua_State *L;
{
    int x,y;
    struct selectionvar *sela = l_selection_check(L, 1);
    struct selectionvar *selb = l_selection_check(L, 2);
    struct selectionvar *selr = l_selection_push(L);

    for (x = 0; x < selr->wid; x++)
        for (y = 0; y < selr->hei; y++) {
            int val = selection_getpoint(x, y, sela) | selection_getpoint(x, y, selb);
            selection_setpoint(x, y, selr, val);
        }

    lua_remove(L, 1);
    lua_remove(L, 1);
    return 1;
}

/* local sel = selection.area(4,5, 40,10) ~ selection.rect(7,8, 60,14); */
static int
l_selection_xor(L)
lua_State *L;
{
    int x,y;
    struct selectionvar *sela = l_selection_check(L, 1);
    struct selectionvar *selb = l_selection_check(L, 2);
    struct selectionvar *selr = l_selection_push(L);

    for (x = 0; x < selr->wid; x++)
        for (y = 0; y < selr->hei; y++) {
            int val = selection_getpoint(x, y, sela) ^ selection_getpoint(x, y, selb);
            selection_setpoint(x, y, selr, val);
        }

    lua_remove(L, 1);
    lua_remove(L, 1);
    return 1;
}


/* local s = selection.percentage(sel, 50); */
static int
l_selection_filter_percent(L)
lua_State *L;
{
    struct selectionvar *ret;
    int p;

    (void) l_selection_check(L, 1);
    p = (int) luaL_checkinteger(L, 2);
    lua_pop(L, 1);
    (void) l_selection_clone(L);
    ret = l_selection_check(L, 1);
    selection_filter_percent(ret, p);

    return 1;
}

/* local x,y = selection.rndcoord(sel); */
/* local x,y = selection.rndcoord(sel, 1); */
static int
l_selection_rndcoord(L)
lua_State *L;
{
    struct selectionvar *sel = l_selection_check(L, 1);
    int removeit = (int) luaL_optinteger(L, 2, 0);
    schar x, y;
    selection_rndcoord(sel, &x, &y, removeit);
    update_croom();
    if (g.coder && g.coder->croom) {
        x -= g.coder->croom->lx;
        y -= g.coder->croom->ly;
    } else {
        x -= g.xstart;
        y -= g.ystart;
    }
    lua_settop(L, 0);
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    return 2;
}

/* internal function to get a selection and 4 integer values from lua stack.
   removes the integers from the stack.
   returns TRUE if params are good.
*/
/* function(selection, x1,y1, x2,y2) */
/* selection:function(x1,y1, x2,y2) */
static boolean
params_sel_2coords(L, sel, x1,y1, x2,y2)
lua_State *L;
struct selectionvar **sel;
schar *x1, *y1, *x2, *y2;
{
    int argc = lua_gettop(L);

    if (argc == 4) {
        (void) l_selection_new(L);
        *x1 = (schar) luaL_checkinteger(L, 1);
        *y1 = (schar) luaL_checkinteger(L, 2);
        *x2 = (schar) luaL_checkinteger(L, 3);
        *y2 = (schar) luaL_checkinteger(L, 4);
        *sel = l_selection_check(L, 5);
        lua_remove(L, 1);
        lua_remove(L, 1);
        lua_remove(L, 1);
        lua_remove(L, 1);
        return TRUE;
    } else if (argc == 5) {
        *sel = l_selection_check(L, 1);
        *x1 = (schar) luaL_checkinteger(L, 2);
        *y1 = (schar) luaL_checkinteger(L, 3);
        *x2 = (schar) luaL_checkinteger(L, 4);
        *y2 = (schar) luaL_checkinteger(L, 5);
        lua_pop(L, 4);
        return TRUE;
    }
    return FALSE;
}

/* local s = selection.line(sel, x1,y1, x2,y2); */
/* local s = selection.line(x1,y1, x2,y2); */
/* s:line(x1,y1, x2,y2); */
static int
l_selection_line(L)
lua_State *L;
{
    struct selectionvar *sel;
    schar x1;
    schar y1;
    schar x2;
    schar y2;

    if (!params_sel_2coords(L, &sel, &x1, &y1, &x2, &y2)) {
        nhl_error(L, "selection.line: illegal arguments");
    }

    get_location_coord(&x1, &y1, ANY_LOC, g.coder ? g.coder->croom : NULL, SP_COORD_PACK(x1,y1));
    get_location_coord(&x2, &y2, ANY_LOC, g.coder ? g.coder->croom : NULL, SP_COORD_PACK(x2,y2));

    lua_settop(L, 1);
    (void) l_selection_clone(L);
    sel = l_selection_check(L, 1);
    selection_do_line(x1,y1,x2,y2, sel);
    return 1;
}

/* local s = selection.rect(sel, x1,y1, x2,y2); */
static int
l_selection_rect(L)
lua_State *L;
{
    struct selectionvar *sel;
    schar x1;
    schar y1;
    schar x2;
    schar y2;

    if (!params_sel_2coords(L, &sel, &x1, &y1, &x2, &y2)) {
        nhl_error(L, "selection.rect: illegal arguments");
    }

    get_location_coord(&x1, &y1, ANY_LOC, g.coder ? g.coder->croom : NULL,
                       SP_COORD_PACK(x1, y1));
    get_location_coord(&x2, &y2, ANY_LOC, g.coder ? g.coder->croom : NULL,
                       SP_COORD_PACK(x2, y2));

    lua_settop(L, 1);
    (void) l_selection_clone(L);
    sel = l_selection_check(L, 1);
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
l_selection_fillrect(L)
lua_State *L;
{
    struct selectionvar *sel;
    int y;
    schar x1;
    schar y1;
    schar x2;
    schar y2;

    if (!params_sel_2coords(L, &sel, &x1, &y1, &x2, &y2)) {
        nhl_error(L, "selection.fillrect: illegal arguments");
    }

    get_location_coord(&x1, &y1, ANY_LOC, g.coder ? g.coder->croom : NULL,
                       SP_COORD_PACK(x1, y1));
    get_location_coord(&x2, &y2, ANY_LOC, g.coder ? g.coder->croom : NULL,
                       SP_COORD_PACK(x2, y2));

    lua_settop(L, 1);
    (void) l_selection_clone(L);
    sel = l_selection_check(L, 1);
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
l_selection_randline(L)
lua_State *L;
{
    int argc = lua_gettop(L);
    struct selectionvar *sel = (struct selectionvar *) 0;
    schar x1, y1, x2, y2;
    int roughness = 7;

    if (argc == 6) {
        sel = l_selection_check(L, 1);
        x1 = (schar) luaL_checkinteger(L, 2);
        y1 = (schar) luaL_checkinteger(L, 3);
        x2 = (schar) luaL_checkinteger(L, 4);
        y2 = (schar) luaL_checkinteger(L, 5);
        roughness = (int) luaL_checkinteger(L, 6);
        lua_pop(L, 5);
    } else if (argc == 5 && lua_type(L, 1) == LUA_TNUMBER) {
        x1 = (schar) luaL_checkinteger(L, 1);
        y1 = (schar) luaL_checkinteger(L, 2);
        x2 = (schar) luaL_checkinteger(L, 3);
        y2 = (schar) luaL_checkinteger(L, 4);
        roughness = (int) luaL_checkinteger(L, 5);
        lua_pop(L, 5);
        (void) l_selection_new(L);
        sel = l_selection_check(L, 1);
    }

    get_location_coord(&x1, &y1, ANY_LOC,
                       g.coder ? g.coder->croom : NULL, SP_COORD_PACK(x1, y1));
    get_location_coord(&x2, &y2, ANY_LOC,
                       g.coder ? g.coder->croom : NULL, SP_COORD_PACK(x2, y2));

    (void) l_selection_clone(L);
    sel = l_selection_check(L, 1);
    selection_do_randline(x1, y1, x2, y2, roughness, 12, sel);
    return 1;
}

/* local s = selection.grow(sel); */
/* local s = selection.grow(sel, "north"); */
static int
l_selection_grow(L)
lua_State *L;
{
    int argc = lua_gettop(L);
    const char *const growdirs[] = { "all", "random", "north", "west", "east", "south", NULL };
    const int growdirs2i[] = { W_ANY, -1, W_NORTH, W_WEST, W_EAST, W_SOUTH, 0 };

    struct selectionvar *sel = l_selection_check(L, 1);
    int dir = growdirs2i[luaL_checkoption(L, 2, "all", growdirs)];

    if (argc == 2)
        lua_pop(L, 1); /* get rid of growdir */

    (void) l_selection_clone(L);
    sel = l_selection_check(L, 1);
    selection_do_grow(sel, dir);
    lua_settop(L, 1);
    return 1;
}


/* local s = selection.filter_mapchar(sel, mapchar, lit); */
static int
l_selection_filter_mapchar(L)
lua_State *L;
{
    int argc = lua_gettop(L);
    struct selectionvar *sel = l_selection_check(L, 1);
    char *mapchr = dupstr(luaL_checkstring(L, 2));
    xchar typ = check_mapchr(mapchr);
    int lit = (int) luaL_optinteger(L, 3, -2); /* TODO: special lit values */
    struct selectionvar *tmp, *tmp2;

    if (typ == INVALID_TYPE)
        nhl_error(L, "Erroneous map char");

    if (argc > 1)
        lua_pop(L, argc - 1);

    tmp = l_selection_push(L);
    tmp2 = selection_filter_mapchar(sel, typ, lit);

    free(tmp->map);
    tmp->map = tmp2->map;
    tmp2->map = NULL;
    selection_free(tmp2, TRUE);

    lua_remove(L, 1);

    if (mapchr)
        free(mapchr);

    return 1;
}


/* local s = selection.floodfill(x,y); */
static int
l_selection_flood(L)
lua_State *L;
{
    int argc = lua_gettop(L);
    struct selectionvar *sel = (struct selectionvar *) 0;
    schar x, y;

    if (argc == 2) {
        x = (schar) luaL_checkinteger(L, 1);
        y = (schar) luaL_checkinteger(L, 2);
        lua_pop(L, 2);
        (void) l_selection_new(L);
        sel = l_selection_check(L, 1);
    } else {
        nhl_error(L, "wrong parameters");
        /*NOTREACHED*/
    }

    get_location_coord(&x, &y, ANY_LOC,
                       g.coder ? g.coder->croom : NULL, SP_COORD_PACK(x, y));

    if (isok(x, y)) {
        set_floodfillchk_match_under(levl[x][y].typ);
        selection_floodfill(sel, x, y, FALSE);
    }
    return 1;
}


/* local s = selection.circle(x,y, radius); */
/* local s = selection.circle(x, y, radius, filled); */
/* local s = selection.circle(sel, x, y, radius); */
/* local s = selection.circle(sel, x, y, radius, filled); */
static int
l_selection_circle(L)
lua_State *L;
{
    int argc = lua_gettop(L);
    struct selectionvar *sel = (struct selectionvar *) 0;
    schar x = 0, y = 0;
    int r = 0, filled = 0;

    if (argc == 3) {
        x = (schar) luaL_checkinteger(L, 1);
        y = (schar) luaL_checkinteger(L, 2);
        r = (int) luaL_checkinteger(L, 3);
        lua_pop(L, 3);
        (void) l_selection_new(L);
        sel = l_selection_check(L, 1);
        filled = 0;
    } else if (argc == 4 && lua_type(L, 1) == LUA_TNUMBER) {
        x = (schar) luaL_checkinteger(L, 1);
        y = (schar) luaL_checkinteger(L, 2);
        r = (int) luaL_checkinteger(L, 3);
        filled = (int) luaL_checkinteger(L, 4); /* TODO: boolean*/
        lua_pop(L, 4);
        (void) l_selection_new(L);
        sel = l_selection_check(L, 1);
    } else if (argc == 4 || argc == 5) {
        sel = l_selection_check(L, 1);
        x = (schar) luaL_checkinteger(L, 2);
        y = (schar) luaL_checkinteger(L, 3);
        r = (int) luaL_checkinteger(L, 4);
        filled = (int) luaL_optinteger(L, 5, 0); /* TODO: boolean */
    } else {
        nhl_error(L, "wrong parameters");
        /*NOTREACHED*/
    }

    get_location_coord(&x, &y, ANY_LOC,
                       g.coder ? g.coder->croom : NULL, SP_COORD_PACK(x, y));

    selection_do_ellipse(sel, x, y, r, r, !filled);

    lua_settop(L, 1);
    return 1;
}

/* local s = selection.ellipse(x, y, radius1, radius2); */
/* local s = selection.ellipse(x, y, radius1, radius2, filled); */
/* local s = selection.ellipse(sel, x, y, radius1, radius2); */
/* local s = selection.ellipse(sel, x, y, radius1, radius2, filled); */
static int
l_selection_ellipse(L)
lua_State *L;
{
    int argc = lua_gettop(L);
    struct selectionvar *sel = (struct selectionvar *) 0;
    schar x = 0, y = 0;
    int r1 = 0, r2 = 0, filled = 0;

    if (argc == 4) {
        x = (schar) luaL_checkinteger(L, 1);
        y = (schar) luaL_checkinteger(L, 2);
        r1 = (int) luaL_checkinteger(L, 3);
        r2 = (int) luaL_checkinteger(L, 4);
        lua_pop(L, 4);
        (void) l_selection_new(L);
        sel = l_selection_check(L, 1);
        filled = 0;
    } else if (argc == 5 && lua_type(L, 1) == LUA_TNUMBER) {
        x = (schar) luaL_checkinteger(L, 1);
        y = (schar) luaL_checkinteger(L, 2);
        r1 = (int) luaL_checkinteger(L, 3);
        r2 = (int) luaL_checkinteger(L, 4);
        filled = (int) luaL_optinteger(L, 5, 0); /* TODO: boolean */
        lua_pop(L, 5);
        (void) l_selection_new(L);
        sel = l_selection_check(L, 1);
    } else if (argc == 5 || argc == 6) {
        sel = l_selection_check(L, 1);
        x = (schar) luaL_checkinteger(L, 2);
        y = (schar) luaL_checkinteger(L, 3);
        r1 = (int) luaL_checkinteger(L, 4);
        r2 = (int) luaL_checkinteger(L, 5);
        filled = (int) luaL_optinteger(L, 6, 0); /* TODO: boolean */
    } else {
        nhl_error(L, "wrong parameters");
        /*NOTREACHED*/
    }

    get_location_coord(&x, &y, ANY_LOC,
                       g.coder ? g.coder->croom : NULL, SP_COORD_PACK(x, y));

    selection_do_ellipse(sel, x, y, r1, r2, !filled);

    lua_settop(L, 1);
    return 1;
}

/* sel:iterate(function(x,y) ... end); */
static int
l_selection_iterate(L)
lua_State *L;
{
    int argc = lua_gettop(L);
    struct selectionvar *sel = (struct selectionvar *) 0;
    int x, y;

    if (argc == 2 && lua_type(L, 2) == LUA_TFUNCTION) {
        sel = l_selection_check(L, 1);
        lua_remove(L, 1);
        for (y = 0; y < sel->hei; y++)
            for (x = 0; x < sel->wid; x++)
                if (selection_getpoint(x, y, sel)) {
                    lua_pushvalue(L, 1);
                    lua_pushinteger(L, x - g.xstart);
                    lua_pushinteger(L, y - g.ystart);
                    lua_call(L, 2, 0);
                }
    } else {
        nhl_error(L, "wrong parameters");
        /*NOTREACHED*/
    }
    return 0;
}


static const struct luaL_Reg l_selection_methods[] = {
    { "new", l_selection_new },
    { "clone", l_selection_clone },
    { "get", l_selection_getpoint },
    { "set", l_selection_setpoint },
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
    { "floodfill", l_selection_flood },
    { "circle", l_selection_circle },
    { "ellipse", l_selection_ellipse },
    { "iterate", l_selection_iterate },
    /* TODO:
       { "gradient", l_selection_gradient },
    */
    { NULL, NULL }
};

static const luaL_Reg l_selection_meta[] = {
    { "__gc", l_selection_gc },
    { "__unm", l_selection_not },
    { "__band", l_selection_and },
    { "__bor", l_selection_or },
    { "__bxor", l_selection_xor },
    { "__bnot", l_selection_not },
    /* TODO: http://lua-users.org/wiki/MetatableEvents
       { "__add", l_selection_add },
       { "__sub", l_selection_sub },
       { "__ipairs", l_selection_ipairs },
    */
    { NULL, NULL }
};

int
l_selection_register(L)
lua_State *L;
{
    int lib_id, meta_id;

    /* newclass = {} */
    lua_createtable(L, 0, 0);
    lib_id = lua_gettop(L);

    /* metatable = {} */
    luaL_newmetatable(L, "selection");
    meta_id = lua_gettop(L);
    luaL_setfuncs(L, l_selection_meta, 0);

    /* metatable.__index = _methods */
    luaL_newlib(L, l_selection_methods);
    lua_setfield(L, meta_id, "__index");

    /* metatable.__metatable = _meta */
    luaL_newlib(L, l_selection_meta);
    lua_setfield(L, meta_id, "__metatable");

    /* class.__metatable = metatable */
    lua_setmetatable(L, lib_id);

    /* _G["selection"] = newclass */
    lua_setglobal(L, "selection");

    return 0;
}
