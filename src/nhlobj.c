/* NetHack 3.7	nhlobj.c	$NHDT-Date: 1576097301 2019/12/11 20:48:21 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.0 $ */
/*      Copyright (c) 2019 by Pasi Kallinen */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "sp_lev.h"

struct _lua_obj {
    int state; /* UNUSED */
    struct obj *obj;
};

static struct _lua_obj *l_obj_check(lua_State *, int);
static int l_obj_add_to_container(lua_State *);
static int l_obj_gc(lua_State *);
static int l_obj_getcontents(lua_State *);
static int l_obj_isnull(lua_State *);
static int l_obj_new_readobjnam(lua_State *);
static int l_obj_nextobj(lua_State *);
static int l_obj_objects_to_table(lua_State *);
static int l_obj_placeobj(lua_State *);
static int l_obj_to_table(lua_State *);
static int l_obj_at(lua_State *);
static int l_obj_container(lua_State *);
static int l_obj_timer_has(lua_State *);
static int l_obj_timer_peek(lua_State *);
static int l_obj_timer_stop(lua_State *);
static int l_obj_timer_start(lua_State *);
static int l_obj_bury(lua_State *);

#define lobj_is_ok(lo) ((lo) && (lo)->obj && (lo)->obj->where != OBJ_LUAFREE)

static struct _lua_obj *
l_obj_check(lua_State *L, int indx)
{
    struct _lua_obj *lo;

    luaL_checktype(L, indx, LUA_TUSERDATA);
    lo = (struct _lua_obj *) luaL_checkudata(L, indx, "obj");
    if (!lo)
        nhl_error(L, "Obj error");
    return lo;
}

static int
l_obj_gc(lua_State *L)
{
    struct _lua_obj *lo = l_obj_check(L, 1);

    if (lo && lo->obj) {
        if (lo->obj->lua_ref_cnt > 0)
            lo->obj->lua_ref_cnt--;
        /* free-floating objects with no other refs are deallocated. */
        if (!lo->obj->lua_ref_cnt
            && (lo->obj->where == OBJ_FREE || lo->obj->where == OBJ_LUAFREE)) {
            if (Has_contents(lo->obj)) {
                struct obj *otmp;
                while ((otmp = lo->obj->cobj) != 0) {
                    obj_extract_self(otmp);
                    dealloc_obj(otmp);
                }
            }
            dealloc_obj(lo->obj);
        }
        lo->obj = NULL;
    }

    return 0;
}

static struct _lua_obj *
l_obj_push(lua_State *L, struct obj *otmp)
{
    struct _lua_obj *lo = (struct _lua_obj *)lua_newuserdata(L, sizeof(struct _lua_obj));
    luaL_getmetatable(L, "obj");
    lua_setmetatable(L, -2);

    lo->state = 0;
    lo->obj = otmp;
    if (otmp)
        otmp->lua_ref_cnt++;

    return lo;
}

void
nhl_push_obj(lua_State *L, struct obj *otmp)
{
    (void) l_obj_push(L, otmp);
}

/* local o = obj.new("large chest");
   local cobj = o:contents(); */
static int
l_obj_getcontents(lua_State *L)
{
    struct _lua_obj *lo = l_obj_check(L, 1);
    struct obj *obj = lo->obj;

    if (!obj)
        nhl_error(L, "l_obj_getcontents: no obj");

    (void) l_obj_push(L, obj->cobj);
    return 1;
}

/* Puts object inside another object. */
/* local box = obj.new("large chest");
   box.addcontent(obj.new("rock"));
*/
static int
l_obj_add_to_container(lua_State *L)
{
    struct _lua_obj *lobox = l_obj_check(L, 1);
    struct _lua_obj *lo = l_obj_check(L, 2);
    struct obj *otmp;
    int refs;

    if (!lobj_is_ok(lo) || !lobj_is_ok(lobox))
        return 0;

    refs = lo->obj->lua_ref_cnt;

    obj_extract_self(lo->obj);
    otmp = add_to_container(lobox->obj, lo->obj);

    /* was lo->obj merged? */
    if (otmp != lo->obj) {
        lo->obj->lua_ref_cnt += refs;
        lo->obj = otmp;
    }

    return 0;
}

/* Put object into player's inventory */
/* u.giveobj(obj.new("rock")); */
int
nhl_obj_u_giveobj(lua_State *L)
{
    struct _lua_obj *lo = l_obj_check(L, 1);
    struct obj *otmp;
    int refs;

    if (!lobj_is_ok(lo) || lo->obj->where == OBJ_INVENT)
        return 0;

    refs = lo->obj->lua_ref_cnt;

    obj_extract_self(lo->obj);
    otmp = addinv(lo->obj);

    if (otmp != lo->obj) {
        lo->obj->lua_ref_cnt += refs;
        lo->obj = otmp;
    }

    return 0;
}

/* Get a table of object class data. */
/* local odata = obj.class(otbl.otyp); */
/* local odata = obj.class(obj.new("rock")); */
/* local odata = o:class(); */
static int
l_obj_objects_to_table(lua_State *L)
{
    int argc = lua_gettop(L);
    int otyp = -1;
    struct objclass *o;

    if (argc != 1) {
        nhl_error(L, "l_obj_objects_to_table: Wrong args");
        return 0;
    }

    if (lua_type(L, 1) == LUA_TNUMBER) {
        otyp = (int) luaL_checkinteger(L, 1);
    } else if (lua_type(L, 1) == LUA_TUSERDATA) {
        struct _lua_obj *lo = l_obj_check(L, 1);
        if (lo && lo->obj)
            otyp = lo->obj->otyp;
    }
    lua_pop(L, 1);

    if (otyp == -1) {
        nhl_error(L, "l_obj_objects_to_table: Wrong args");
        return 0;
    }

    o = &objects[otyp];

    lua_newtable(L);

    if (OBJ_NAME(objects[otyp]))
        nhl_add_table_entry_str(L, "name", OBJ_NAME(objects[otyp]));
    if (OBJ_DESCR(objects[otyp]))
        nhl_add_table_entry_str(L, "descr",
                                OBJ_DESCR(objects[otyp]));
    if (o->oc_uname)
        nhl_add_table_entry_str(L, "uname", o->oc_uname);

    nhl_add_table_entry_int(L, "name_known", o->oc_name_known);
    nhl_add_table_entry_int(L, "merge", o->oc_merge);
    nhl_add_table_entry_int(L, "uses_known", o->oc_uses_known);
    nhl_add_table_entry_int(L, "pre_discovered", o->oc_pre_discovered);
    nhl_add_table_entry_int(L, "magic", o->oc_magic);
    nhl_add_table_entry_int(L, "charged", o->oc_charged);
    nhl_add_table_entry_int(L, "unique", o->oc_unique);
    nhl_add_table_entry_int(L, "nowish", o->oc_nowish);
    nhl_add_table_entry_int(L, "big", o->oc_big);
    /* TODO: oc_bimanual, oc_bulky */
    nhl_add_table_entry_int(L, "tough", o->oc_tough);
    nhl_add_table_entry_int(L, "dir", o->oc_dir); /* TODO: convert to text */
    nhl_add_table_entry_int(L, "material", o->oc_material); /* TODO: convert to text */
    /* TODO: oc_subtyp, oc_skill, oc_armcat */
    nhl_add_table_entry_int(L, "oprop", o->oc_oprop);
    nhl_add_table_entry_char(L, "class",
                             def_oc_syms[(uchar) o->oc_class].sym);
    nhl_add_table_entry_int(L, "delay", o->oc_delay);
    nhl_add_table_entry_int(L, "color", o->oc_color); /* TODO: text? */
    nhl_add_table_entry_int(L, "prob", o->oc_prob);
    nhl_add_table_entry_int(L, "weight", o->oc_weight);
    nhl_add_table_entry_int(L, "cost", o->oc_cost);
    nhl_add_table_entry_int(L, "damage_small", o->oc_wsdam);
    nhl_add_table_entry_int(L, "damage_large", o->oc_wldam);
    /* TODO: oc_oc1, oc_oc2, oc_hitbon, a_ac, a_can, oc_level */
    nhl_add_table_entry_int(L, "nutrition", o->oc_nutrition);

    return 1;
}

/* Create a lua table representation of the object, unpacking all the
   object fields.
   local o = obj.new("rock");
   local otbl = o:totable(); */
static int
l_obj_to_table(lua_State *L)
{
    struct _lua_obj *lo = l_obj_check(L, 1);
    struct obj *obj = lo->obj;

    lua_newtable(L);

    if (!obj || obj->where == OBJ_LUAFREE) {
        nhl_add_table_entry_int(L, "NO_OBJ", 1);
        return 1;
    }

    nhl_add_table_entry_int(L, "has_contents", Has_contents(obj));
    nhl_add_table_entry_int(L, "is_container", Is_container(obj));
    nhl_add_table_entry_int(L, "o_id", obj->o_id);
    nhl_add_table_entry_int(L, "ox", obj->ox);
    nhl_add_table_entry_int(L, "oy", obj->oy);
    nhl_add_table_entry_int(L, "otyp", obj->otyp);
    if (OBJ_NAME(objects[obj->otyp]))
        nhl_add_table_entry_str(L, "otyp_name", OBJ_NAME(objects[obj->otyp]));
    if (OBJ_DESCR(objects[obj->otyp]))
        nhl_add_table_entry_str(L, "otyp_descr",
                                OBJ_DESCR(objects[obj->otyp]));
    nhl_add_table_entry_int(L, "owt", obj->owt);
    nhl_add_table_entry_int(L, "quan", obj->quan);
    nhl_add_table_entry_int(L, "spe", obj->spe);

    if (obj->otyp == STATUE)
        nhl_add_table_entry_int(L, "historic",
                                (obj->spe & CORPSTAT_HISTORIC) != 0);
    if (obj->otyp == CORPSE || obj->otyp == STATUE) {
        nhl_add_table_entry_int(L, "male",
                                (obj->spe & CORPSTAT_MALE) != 0);
        nhl_add_table_entry_int(L, "female",
                                (obj->spe & CORPSTAT_FEMALE) != 0);
    }

    nhl_add_table_entry_char(L, "oclass",
                             def_oc_syms[(uchar) obj->oclass].sym);
    nhl_add_table_entry_char(L, "invlet", obj->invlet);
    /* TODO: nhl_add_table_entry_char(L, "oartifact", obj->oartifact);*/
    nhl_add_table_entry_int(L, "where", obj->where);
    /* TODO: nhl_add_table_entry_int(L, "timed", obj->timed); */
    nhl_add_table_entry_int(L, "cursed", obj->cursed);
    nhl_add_table_entry_int(L, "blessed", obj->blessed);
    nhl_add_table_entry_int(L, "unpaid", obj->unpaid);
    nhl_add_table_entry_int(L, "no_charge", obj->no_charge);
    nhl_add_table_entry_int(L, "known", obj->known);
    nhl_add_table_entry_int(L, "dknown", obj->dknown);
    nhl_add_table_entry_int(L, "bknown", obj->bknown);
    nhl_add_table_entry_int(L, "rknown", obj->rknown);
    if (obj->oclass == POTION_CLASS)
        nhl_add_table_entry_int(L, "odiluted", obj->odiluted);
    else
        nhl_add_table_entry_int(L, "oeroded", obj->oeroded);
    nhl_add_table_entry_int(L, "oeroded2", obj->oeroded2);
    /* TODO: orotten, norevive */
    nhl_add_table_entry_int(L, "oerodeproof", obj->oerodeproof);
    nhl_add_table_entry_int(L, "olocked", obj->olocked);
    nhl_add_table_entry_int(L, "obroken", obj->obroken);
    if (is_poisonable(obj))
        nhl_add_table_entry_int(L, "opoisoned", obj->opoisoned);
    else
        nhl_add_table_entry_int(L, "otrapped", obj->otrapped);
    /* TODO: degraded_horn */
    nhl_add_table_entry_int(L, "recharged", obj->recharged);
    /* TODO: on_ice */
    nhl_add_table_entry_int(L, "lamplit", obj->lamplit);
    nhl_add_table_entry_int(L, "globby", obj->globby);
    nhl_add_table_entry_int(L, "greased", obj->greased);
    nhl_add_table_entry_int(L, "nomerge", obj->nomerge);
    nhl_add_table_entry_int(L, "was_thrown", obj->was_thrown);
    nhl_add_table_entry_int(L, "in_use", obj->in_use);
    nhl_add_table_entry_int(L, "bypass", obj->bypass);
    nhl_add_table_entry_int(L, "cknown", obj->cknown);
    nhl_add_table_entry_int(L, "lknown", obj->lknown);
    nhl_add_table_entry_int(L, "corpsenm", obj->corpsenm);
    if (obj->corpsenm != NON_PM
        && (obj->otyp == TIN || obj->otyp == CORPSE || obj->otyp == EGG
            || obj->otyp == FIGURINE || obj->otyp == STATUE))
        nhl_add_table_entry_str(L, "corpsenm_name",
                                mons[obj->corpsenm].pmnames[NEUTRAL]);
    /* TODO: leashmon, fromsink, novelidx, record_achieve_special */
    nhl_add_table_entry_int(L, "usecount", obj->usecount);
    /* TODO: spestudied */
    nhl_add_table_entry_int(L, "oeaten", obj->oeaten);
    nhl_add_table_entry_int(L, "age", obj->age);
    nhl_add_table_entry_int(L, "owornmask", obj->owornmask);
    /* TODO: more of oextra */
    nhl_add_table_entry_int(L, "has_oname", has_oname(obj));
    if (has_oname(obj))
        nhl_add_table_entry_str(L, "oname", ONAME(obj));

    return 1;
}

/* create a new object via wishing routine */
/* local o = obj.new("rock"); */
static int
l_obj_new_readobjnam(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc == 1) {
        char buf[BUFSZ];
        struct obj *otmp;
        Sprintf(buf, "%s", luaL_checkstring(L, 1));
        lua_pop(L, 1);
        otmp = readobjnam(buf, NULL);
        (void) l_obj_push(L, otmp);
        return 1;
    } else
        nhl_error(L, "l_obj_new_readobjname: Wrong args");
    return 0;
}

/* Get the topmost object on the map at x,y */
/* local o = obj.at(x, y); */
static int
l_obj_at(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc == 2) {
        int x, y;

        x = (int) luaL_checkinteger(L, 1);
        y = (int) luaL_checkinteger(L, 2);
        lua_pop(L, 2);
        (void) l_obj_push(L, g.level.objects[x][y]);
        return 1;
    } else
        nhl_error(L, "l_obj_at: Wrong args");
    return 0;
}

/* Place an object on the map at (x,y).
   local o = obj.new("rock");
   o:placeobj(u.ux, u.uy); */
static int
l_obj_placeobj(lua_State *L)
{
    int argc = lua_gettop(L);
    struct _lua_obj *lo = l_obj_check(L, 1);
    int x, y;

    if (argc != 3)
        nhl_error(L, "l_obj_placeobj: Wrong args");

    x = (int) luaL_checkinteger(L, 2);
    y = (int) luaL_checkinteger(L, 3);
    lua_pop(L, 3);

    if (lobj_is_ok(lo)) {
        obj_extract_self(lo->obj);
        place_object(lo->obj, x, y);
        newsym(x, y);
    }

    return 0;
}

/* Get the next object in the object chain */
/* local o = obj.at(x, y);
   local o2 = o:next(true);
   local firstobj = obj.next();
*/
static int
l_obj_nextobj(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc == 0) {
        (void) l_obj_push(L, fobj);
    } else {
        struct _lua_obj *lo = l_obj_check(L, 1);
        boolean use_nexthere = FALSE;

        if (argc == 2)
            use_nexthere = lua_toboolean(L, 2);

        if (lo && lo->obj)
            (void) l_obj_push(L, (use_nexthere && lo->obj->where == OBJ_FLOOR)
                                  ? lo->obj->nexthere
                                  : lo->obj->nobj);
    }
    return 1;
}

/* Get the container object is in */
/* local box = o:container(); */
static int
l_obj_container(lua_State *L)
{
    struct _lua_obj *lo = l_obj_check(L, 1);

    if (lo && lo->obj && lo->obj->where == OBJ_CONTAINED)
        (void) l_obj_push(L, lo->obj->ocontainer);
    else
        (void) l_obj_push(L, NULL);
    return 1;
}

/* Is the object a null? */
/* local badobj = o:isnull(); */
static int
l_obj_isnull(lua_State *L)
{
    struct _lua_obj *lo = l_obj_check(L, 1);

    lua_pushboolean(L, !lobj_is_ok(lo));
    return 1;
}

/* does object have a timer of certain type? */
/* local hastimer = o:has_timer("rot-organic"); */
static int
l_obj_timer_has(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc == 2) {
        struct _lua_obj *lo = l_obj_check(L, 1);
        short timertype = nhl_get_timertype(L, 2);

        if (timer_is_obj(timertype) && lo && lo->obj) {
            lua_pushboolean(L, obj_has_timer(lo->obj, timertype));
            return 1;
        } else {
            lua_pushboolean(L, FALSE);
            return 1;
        }
    } else
        nhl_error(L, "l_obj_timer_has: Wrong args");
    return 0;
}

/* peek at an object timer. return the turn when timer triggers.
   returns 0 if no such timer attached to the object. */
/* local timeout = o:peek_timer("hatch-egg"); */
static int
l_obj_timer_peek(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc == 2) {
        struct _lua_obj *lo = l_obj_check(L, 1);
        short timertype = nhl_get_timertype(L, 2);

        if (timer_is_obj(timertype) && lo && lo->obj) {
            lua_pushinteger(L, peek_timer(timertype, obj_to_any(lo->obj)));
            return 1;
        } else {
            lua_pushinteger(L, 0);
            return 1;
        }
    } else
        nhl_error(L, "l_obj_timer_peek: Wrong args");
    return 0;
}

/* stop object timer(s). return the turn when timer triggers.
   returns 0 if no such timer attached to the object.
   without a timer type parameter, stops all timers for the object. */
/* local timeout = o:stop_timer("rot-organic"); */
/* o:stop_timer(); */
static int
l_obj_timer_stop(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc == 1) {
        struct _lua_obj *lo = l_obj_check(L, 1);

        if (lo && lo->obj)
            obj_stop_timers(lo->obj);
        return 0;

    } else if (argc == 2) {
        struct _lua_obj *lo = l_obj_check(L, 1);
        short timertype = nhl_get_timertype(L, 2);

        if (timer_is_obj(timertype) && lo && lo->obj) {
            lua_pushinteger(L, stop_timer(timertype, obj_to_any(lo->obj)));
            return 1;
        } else {
            lua_pushinteger(L, 0);
            return 1;
        }
    } else
        nhl_error(L, "l_obj_timer_stop: Wrong args");
    return 0;
}

/* start an object timer. */
/* o:start_timer("hatch-egg", 10); */
static int
l_obj_timer_start(lua_State *L)
{
    int argc = lua_gettop(L);

    if (argc == 3) {
        struct _lua_obj *lo = l_obj_check(L, 1);
        short timertype = nhl_get_timertype(L, 2);
        long when = luaL_checkinteger(L, 3);

        if (timer_is_obj(timertype) && lo && lo->obj && when > 0) {
            if (obj_has_timer(lo->obj, timertype))
                stop_timer(timertype, obj_to_any(lo->obj));
            start_timer(when, TIMER_OBJECT, timertype, obj_to_any(lo->obj));
        }
    } else
        nhl_error(L, "l_obj_timer_start: Wrong args");
    return 0;
}

/* bury an obj. returns true if object is gone (merged with ground),
   false otherwise. */
/* local ogone = o:bury(); */
/* local ogone = o:bury(5,5); */
static int
l_obj_bury(lua_State *L)
{
    int argc = lua_gettop(L);
    boolean dealloced = FALSE;
    struct _lua_obj *lo = l_obj_check(L, 1);
    xchar x = 0, y = 0;

    if (argc == 1) {
        x = lo->obj->ox;
        y = lo->obj->oy;
    } else if (argc == 3) {
        x = (xchar) lua_tointeger(L, 2);
        y = (xchar) lua_tointeger(L, 3);
    } else
        nhl_error(L, "l_obj_bury: Wrong args");

    if (lobj_is_ok(lo) && isok(x, y)) {
        lo->obj->ox = x;
        lo->obj->oy = y;
        (void) bury_an_obj(lo->obj, &dealloced);
    }
    lua_pushboolean(L, dealloced);
    return 1;
}

static const struct luaL_Reg l_obj_methods[] = {
    { "new", l_obj_new_readobjnam },
    { "isnull", l_obj_isnull },
    { "at", l_obj_at },
    { "next", l_obj_nextobj },
    { "totable", l_obj_to_table },
    { "class", l_obj_objects_to_table },
    { "placeobj", l_obj_placeobj },
    { "container", l_obj_container },
    { "contents", l_obj_getcontents },
    { "addcontent", l_obj_add_to_container },
    { "has_timer", l_obj_timer_has },
    { "peek_timer", l_obj_timer_peek },
    { "stop_timer", l_obj_timer_stop },
    { "start_timer", l_obj_timer_start },
    { "bury", l_obj_bury },
    { NULL, NULL }
};

static const luaL_Reg l_obj_meta[] = {
    { "__gc", l_obj_gc },
    { NULL, NULL }
};

int
l_obj_register(lua_State *L)
{
    int lib_id, meta_id;

    /* newclass = {} */
    lua_createtable(L, 0, 0);
    lib_id = lua_gettop(L);

    /* metatable = {} */
    luaL_newmetatable(L, "obj");
    meta_id = lua_gettop(L);
    luaL_setfuncs(L, l_obj_meta, 0);

    /* metatable.__index = _methods */
    luaL_newlib(L, l_obj_methods);
    lua_setfield(L, meta_id, "__index");

    /* metatable.__metatable = _meta */
    luaL_newlib(L, l_obj_meta);
    lua_setfield(L, meta_id, "__metatable");

    /* class.__metatable = metatable */
    lua_setmetatable(L, lib_id);

    /* _G["obj"] = newclass */
    lua_setglobal(L, "obj");

    return 0;
}
