/* NetHack 3.6	light.c	$NHDT-Date: 1559994625 2019/06/08 11:50:25 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.30 $ */
/* Copyright (c) Dean Luick, 1994                                       */
/* NetHack may be freely redistributed.  See license for details.       */

#include "hack.h"
#include "lev.h" /* for checking save modes */

/*
 * Mobile light sources.
 *
 * This implementation minimizes memory at the expense of extra
 * recalculations.
 *
 * Light sources are "things" that have a physical position and range.
 * They have a type, which gives us information about them.  Currently
 * they are only attached to objects and monsters.  Note well:  the
 * polymorphed-player handling assumes that both youmonst.m_id and
 * youmonst.mx will always remain 0.
 *
 * Light sources, like timers, either follow game play (RANGE_GLOBAL) or
 * stay on a level (RANGE_LEVEL).  Light sources are unique by their
 * (type, id) pair.  For light sources attached to objects, this id
 * is a pointer to the object.
 *
 * The major working function is do_light_sources(). It is called
 * when the vision system is recreating its "could see" array.  Here
 * we add a flag (TEMP_LIT) to the array for all locations that are lit
 * via a light source.  The bad part of this is that we have to
 * re-calculate the LOS of each light source every time the vision
 * system runs.  Even if the light sources and any topology (vision blocking
 * positions) have not changed.  The good part is that no extra memory
 * is used, plus we don't have to figure out how far the sources have moved,
 * or if the topology has changed.
 *
 * The structure of the save/restore mechanism is amazingly similar to
 * the timer save/restore.  This is because they both have the same
 * principals of having pointers into objects that must be recalculated
 * across saves and restores.
 */

/* flags */
#define LSF_SHOW 0x1        /* display the light source */
#define LSF_NEEDS_FIXUP 0x2 /* need oid fixup */

static light_source *light_base = 0;

STATIC_DCL void FDECL(write_ls, (int, light_source *));
STATIC_DCL int FDECL(maybe_write_ls, (int, int, BOOLEAN_P));

/* imported from vision.c, for small circles */
extern char circle_data[];
extern char circle_start[];

/* Create a new light source.  */
void
new_light_source(x, y, range, type, id)
xchar x, y;
int range, type;
anything *id;
{
    light_source *ls;

    if (range > MAX_RADIUS || range < 1) {
        impossible("new_light_source:  illegal range %d", range);
        return;
    }

    ls = (light_source *) alloc(sizeof *ls);

    ls->next = light_base;
    ls->x = x;
    ls->y = y;
    ls->range = range;
    ls->type = type;
    ls->id = *id;
    ls->flags = 0;
    light_base = ls;

    vision_full_recalc = 1; /* make the source show up */
}

/*
 * Delete a light source. This assumes only one light source is attached
 * to an object at a time.
 */
void
del_light_source(type, id)
int type;
anything *id;
{
    light_source *curr, *prev;
    anything tmp_id;

    tmp_id = zeroany;
    /* need to be prepared for dealing a with light source which
       has only been partially restored during a level change
       (in particular: chameleon vs prot. from shape changers) */
    switch (type) {
    case LS_OBJECT:
        tmp_id.a_uint = id->a_obj->o_id;
        break;
    case LS_MONSTER:
        tmp_id.a_uint = id->a_monst->m_id;
        break;
    default:
        tmp_id.a_uint = 0;
        break;
    }

    for (prev = 0, curr = light_base; curr; prev = curr, curr = curr->next) {
        if (curr->type != type)
            continue;
        if (curr->id.a_obj
            == ((curr->flags & LSF_NEEDS_FIXUP) ? tmp_id.a_obj : id->a_obj)) {
            if (prev)
                prev->next = curr->next;
            else
                light_base = curr->next;

            free((genericptr_t) curr);
            vision_full_recalc = 1;
            return;
        }
    }
    impossible("del_light_source: not found type=%d, id=%s", type,
               fmt_ptr((genericptr_t) id->a_obj));
}

/* Mark locations that are temporarily lit via mobile light sources. */
void
do_light_sources(cs_rows)
char **cs_rows;
{
    int x, y, min_x, max_x, max_y, offset;
    char *limits;
    short at_hero_range = 0;
    light_source *ls;
    char *row;

    for (ls = light_base; ls; ls = ls->next) {
        ls->flags &= ~LSF_SHOW;

        /*
         * Check for moved light sources.  It may be possible to
         * save some effort if an object has not moved, but not in
         * the current setup -- we need to recalculate for every
         * vision recalc.
         */
        if (ls->type == LS_OBJECT) {
            if (get_obj_location(ls->id.a_obj, &ls->x, &ls->y, 0))
                ls->flags |= LSF_SHOW;
        } else if (ls->type == LS_MONSTER) {
            if (get_mon_location(ls->id.a_monst, &ls->x, &ls->y, 0))
                ls->flags |= LSF_SHOW;
        }

        /* minor optimization: don't bother with duplicate light sources
           at hero */
        if (ls->x == u.ux && ls->y == u.uy) {
            if (at_hero_range >= ls->range)
                ls->flags &= ~LSF_SHOW;
            else
                at_hero_range = ls->range;
        }

        if (ls->flags & LSF_SHOW) {
            /*
             * Walk the points in the circle and see if they are
             * visible from the center.  If so, mark'em.
             *
             * Kevin's tests indicated that doing this brute-force
             * method is faster for radius <= 3 (or so).
             */
            limits = circle_ptr(ls->range);
            if ((max_y = (ls->y + ls->range)) >= ROWNO)
                max_y = ROWNO - 1;
            if ((y = (ls->y - ls->range)) < 0)
                y = 0;
            for (; y <= max_y; y++) {
                row = cs_rows[y];
                offset = limits[abs(y - ls->y)];
                if ((min_x = (ls->x - offset)) < 0)
                    min_x = 0;
                if ((max_x = (ls->x + offset)) >= COLNO)
                    max_x = COLNO - 1;

                if (ls->x == u.ux && ls->y == u.uy) {
                    /*
                     * If the light source is located at the hero, then
                     * we can use the COULD_SEE bits already calculated
                     * by the vision system.  More importantly than
                     * this optimization, is that it allows the vision
                     * system to correct problems with clear_path().
                     * The function clear_path() is a simple LOS
                     * path checker that doesn't go out of its way to
                     * make things look "correct".  The vision system
                     * does this.
                     */
                    for (x = min_x; x <= max_x; x++)
                        if (row[x] & COULD_SEE)
                            row[x] |= TEMP_LIT;
                } else {
                    for (x = min_x; x <= max_x; x++)
                        if ((ls->x == x && ls->y == y)
                            || clear_path((int) ls->x, (int) ls->y, x, y))
                            row[x] |= TEMP_LIT;
                }
            }
        }
    }
}

/* lit 'obj' has been thrown or kicked and is passing through x,y on the
   way to its destination; show its light so that hero has a chance to
   remember terrain, objects, and monsters being revealed */
void
show_transient_light(obj, x, y)
struct obj *obj;
int x, y;
{
    light_source *ls;
    struct monst *mon;
    int radius_squared;

    /* caller has verified obj->lamplit and that hero is not Blind;
       validate light source and obtain its radius (for monster sightings) */
    for (ls = light_base; ls; ls = ls->next) {
        if (ls->type != LS_OBJECT)
            continue;
        if (ls->id.a_obj == obj)
            break;
    }
    if (!ls || obj->where != OBJ_FREE) {
        impossible("transient light %s %s is not %s?",
                   obj->lamplit ? "lit" : "unlit", xname(obj),
                   !ls ? "a light source" : "free");
    } else {
        /* "expensive" but rare */
        place_object(obj, bhitpos.x, bhitpos.y); /* temporarily put on map */
        vision_recalc(0);
        flush_screen(0);
        delay_output();
        remove_object(obj); /* take back off of map */

        radius_squared = ls->range * ls->range;
        for (mon = fmon; mon; mon = mon->nmon) {
            if (DEADMONSTER(mon))
                continue;
            /* light range is the radius of a circle and we're limiting
               canseemon() to a square exclosing that circle, but setting
               mtemplit 'erroneously' for a seen monster is not a problem;
               it just flags monsters for another canseemon() check when
               'obj' has reached its destination after missile traversal */
            if (dist2(mon->mx, mon->my, x, y) <= radius_squared
                && canseemon(mon))
                mon->mtemplit = 1;
            /* [what about worm tails?] */
        }
    }
}

/* draw "remembered, unseen monster" glyph at locations where a monster
   was flagged for being visible during transient light movement but can't
   be seen now */
void
transient_light_cleanup()
{
    struct monst *mon;
    int mtempcount = 0;

    for (mon = fmon; mon; mon = mon->nmon) {
        if (DEADMONSTER(mon))
            continue;
        if (mon->mtemplit) {
            mon->mtemplit = 0;
            ++mtempcount;
            if (!canseemon(mon))
                map_invisible(mon->mx, mon->my);
        }
    }
    if (mtempcount) {
        vision_recalc(0);
        flush_screen(0);
    }
}

/* (mon->mx == 0) implies migrating */
#define mon_is_local(mon) ((mon)->mx > 0)

struct monst *
find_mid(nid, fmflags)
unsigned nid;
unsigned fmflags;
{
    struct monst *mtmp;

    if (!nid)
        return &youmonst;
    if (fmflags & FM_FMON)
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
            if (!DEADMONSTER(mtmp) && mtmp->m_id == nid)
                return mtmp;
    if (fmflags & FM_MIGRATE)
        for (mtmp = migrating_mons; mtmp; mtmp = mtmp->nmon)
            if (mtmp->m_id == nid)
                return mtmp;
    if (fmflags & FM_MYDOGS)
        for (mtmp = mydogs; mtmp; mtmp = mtmp->nmon)
            if (mtmp->m_id == nid)
                return mtmp;
    return (struct monst *) 0;
}

/* Save all light sources of the given range. */
void
save_light_sources(fd, mode, range)
int fd, mode, range;
{
    int count, actual, is_global;
    light_source **prev, *curr;

    if (perform_bwrite(mode)) {
        count = maybe_write_ls(fd, range, FALSE);
        bwrite(fd, (genericptr_t) &count, sizeof count);
        actual = maybe_write_ls(fd, range, TRUE);
        if (actual != count)
            panic("counted %d light sources, wrote %d! [range=%d]", count,
                  actual, range);
    }

    if (release_data(mode)) {
        for (prev = &light_base; (curr = *prev) != 0;) {
            if (!curr->id.a_monst) {
                impossible("save_light_sources: no id! [range=%d]", range);
                is_global = 0;
            } else
                switch (curr->type) {
                case LS_OBJECT:
                    is_global = !obj_is_local(curr->id.a_obj);
                    break;
                case LS_MONSTER:
                    is_global = !mon_is_local(curr->id.a_monst);
                    break;
                default:
                    is_global = 0;
                    impossible("save_light_sources: bad type (%d) [range=%d]",
                               curr->type, range);
                    break;
                }
            /* if global and not doing local, or vice versa, remove it */
            if (is_global ^ (range == RANGE_LEVEL)) {
                *prev = curr->next;
                free((genericptr_t) curr);
            } else {
                prev = &(*prev)->next;
            }
        }
    }
}

/*
 * Pull in the structures from disk, but don't recalculate the object
 * pointers.
 */
void
restore_light_sources(fd)
int fd;
{
    int count;
    light_source *ls;

    /* restore elements */
    mread(fd, (genericptr_t) &count, sizeof count);

    while (count-- > 0) {
        ls = (light_source *) alloc(sizeof(light_source));
        mread(fd, (genericptr_t) ls, sizeof(light_source));
        ls->next = light_base;
        light_base = ls;
    }
}

/* to support '#stats' wizard-mode command */
void
light_stats(hdrfmt, hdrbuf, count, size)
const char *hdrfmt;
char *hdrbuf;
long *count, *size;
{
    light_source *ls;

    Sprintf(hdrbuf, hdrfmt, (long) sizeof (light_source));
    *count = *size = 0L;
    for (ls = light_base; ls; ls = ls->next) {
        ++*count;
        *size += (long) sizeof *ls;
    }
}

/* Relink all lights that are so marked. */
void
relink_light_sources(ghostly)
boolean ghostly;
{
    char which;
    unsigned nid;
    light_source *ls;

    for (ls = light_base; ls; ls = ls->next) {
        if (ls->flags & LSF_NEEDS_FIXUP) {
            if (ls->type == LS_OBJECT || ls->type == LS_MONSTER) {
                if (ghostly) {
                    if (!lookup_id_mapping(ls->id.a_uint, &nid))
                        impossible("relink_light_sources: no id mapping");
                } else
                    nid = ls->id.a_uint;
                if (ls->type == LS_OBJECT) {
                    which = 'o';
                    ls->id.a_obj = find_oid(nid);
                } else {
                    which = 'm';
                    ls->id.a_monst = find_mid(nid, FM_EVERYWHERE);
                }
                if (!ls->id.a_monst)
                    impossible("relink_light_sources: cant find %c_id %d",
                               which, nid);
            } else
                impossible("relink_light_sources: bad type (%d)", ls->type);

            ls->flags &= ~LSF_NEEDS_FIXUP;
        }
    }
}

/*
 * Part of the light source save routine.  Count up the number of light
 * sources that would be written.  If write_it is true, actually write
 * the light source out.
 */
STATIC_OVL int
maybe_write_ls(fd, range, write_it)
int fd, range;
boolean write_it;
{
    int count = 0, is_global;
    light_source *ls;

    for (ls = light_base; ls; ls = ls->next) {
        if (!ls->id.a_monst) {
            impossible("maybe_write_ls: no id! [range=%d]", range);
            continue;
        }
        switch (ls->type) {
        case LS_OBJECT:
            is_global = !obj_is_local(ls->id.a_obj);
            break;
        case LS_MONSTER:
            is_global = !mon_is_local(ls->id.a_monst);
            break;
        default:
            is_global = 0;
            impossible("maybe_write_ls: bad type (%d) [range=%d]", ls->type,
                       range);
            break;
        }
        /* if global and not doing local, or vice versa, count it */
        if (is_global ^ (range == RANGE_LEVEL)) {
            count++;
            if (write_it)
                write_ls(fd, ls);
        }
    }

    return count;
}

void
light_sources_sanity_check()
{
    light_source *ls;
    struct monst *mtmp;
    struct obj *otmp;
    unsigned int auint;

    for (ls = light_base; ls; ls = ls->next) {
        if (!ls->id.a_monst)
            panic("insane light source: no id!");
        if (ls->type == LS_OBJECT) {
            otmp = (struct obj *) ls->id.a_obj;
            auint = otmp->o_id;
            if (find_oid(auint) != otmp)
                panic("insane light source: can't find obj #%u!", auint);
        } else if (ls->type == LS_MONSTER) {
            mtmp = (struct monst *) ls->id.a_monst;
            auint = mtmp->m_id;
            if (find_mid(auint, FM_EVERYWHERE) != mtmp)
                panic("insane light source: can't find mon #%u!", auint);
        } else {
            panic("insane light source: bad ls type %d", ls->type);
        }
    }
}

/* Write a light source structure to disk. */
STATIC_OVL void
write_ls(fd, ls)
int fd;
light_source *ls;
{
    anything arg_save;
    struct obj *otmp;
    struct monst *mtmp;

    if (ls->type == LS_OBJECT || ls->type == LS_MONSTER) {
        if (ls->flags & LSF_NEEDS_FIXUP) {
            bwrite(fd, (genericptr_t) ls, sizeof(light_source));
        } else {
            /* replace object pointer with id for write, then put back */
            arg_save = ls->id;
            if (ls->type == LS_OBJECT) {
                otmp = ls->id.a_obj;
                ls->id = zeroany;
                ls->id.a_uint = otmp->o_id;
                if (find_oid((unsigned) ls->id.a_uint) != otmp)
                    impossible("write_ls: can't find obj #%u!",
                               ls->id.a_uint);
            } else { /* ls->type == LS_MONSTER */
                mtmp = (struct monst *) ls->id.a_monst;
                ls->id = zeroany;
                ls->id.a_uint = mtmp->m_id;
                if (find_mid((unsigned) ls->id.a_uint, FM_EVERYWHERE) != mtmp)
                    impossible("write_ls: can't find mon #%u!",
                               ls->id.a_uint);
            }
            ls->flags |= LSF_NEEDS_FIXUP;
            bwrite(fd, (genericptr_t) ls, sizeof(light_source));
            ls->id = arg_save;
            ls->flags &= ~LSF_NEEDS_FIXUP;
        }
    } else {
        impossible("write_ls: bad type (%d)", ls->type);
    }
}

/* Change light source's ID from src to dest. */
void
obj_move_light_source(src, dest)
struct obj *src, *dest;
{
    light_source *ls;

    for (ls = light_base; ls; ls = ls->next)
        if (ls->type == LS_OBJECT && ls->id.a_obj == src)
            ls->id.a_obj = dest;
    src->lamplit = 0;
    dest->lamplit = 1;
}

/* return true if there exist any light sources */
boolean
any_light_source()
{
    return (boolean) (light_base != (light_source *) 0);
}

/*
 * Snuff an object light source if at (x,y).  This currently works
 * only for burning light sources.
 */
void
snuff_light_source(x, y)
int x, y;
{
    light_source *ls;
    struct obj *obj;

    for (ls = light_base; ls; ls = ls->next)
        /*
         * Is this position check valid??? Can I assume that the positions
         * will always be correct because the objects would have been
         * updated with the last vision update?  [Is that recent enough???]
         */
        if (ls->type == LS_OBJECT && ls->x == x && ls->y == y) {
            obj = ls->id.a_obj;
            if (obj_is_burning(obj)) {
                /* The only way to snuff Sunsword is to unwield it.  Darkness
                 * scrolls won't affect it.  (If we got here because it was
                 * dropped or thrown inside a monster, this won't matter
                 * anyway because it will go out when dropped.)
                 */
                if (artifact_light(obj))
                    continue;
                end_burn(obj, obj->otyp != MAGIC_LAMP);
                /*
                 * The current ls element has just been removed (and
                 * ls->next is now invalid).  Return assuming that there
                 * is only one light source attached to each object.
                 */
                return;
            }
        }
}

/* Return TRUE if object sheds any light at all. */
boolean
obj_sheds_light(obj)
struct obj *obj;
{
    /* so far, only burning objects shed light */
    return obj_is_burning(obj);
}

/* Return TRUE if sheds light AND will be snuffed by end_burn(). */
boolean
obj_is_burning(obj)
struct obj *obj;
{
    return (boolean) (obj->lamplit && (obj->otyp == MAGIC_LAMP
                                       || ignitable(obj)
                                       || artifact_light(obj)));
}

/* copy the light source(s) attached to src, and attach it/them to dest */
void
obj_split_light_source(src, dest)
struct obj *src, *dest;
{
    light_source *ls, *new_ls;

    for (ls = light_base; ls; ls = ls->next)
        if (ls->type == LS_OBJECT && ls->id.a_obj == src) {
            /*
             * Insert the new source at beginning of list.  This will
             * never interfere us walking down the list - we are already
             * past the insertion point.
             */
            new_ls = (light_source *) alloc(sizeof(light_source));
            *new_ls = *ls;
            if (Is_candle(src)) {
                /* split candles may emit less light than original group */
                ls->range = candle_light_range(src);
                new_ls->range = candle_light_range(dest);
                vision_full_recalc = 1; /* in case range changed */
            }
            new_ls->id.a_obj = dest;
            new_ls->next = light_base;
            light_base = new_ls;
            dest->lamplit = 1; /* now an active light source */
        }
}

/* light source `src' has been folded into light source `dest';
   used for merging lit candles and adding candle(s) to lit candelabrum */
void
obj_merge_light_sources(src, dest)
struct obj *src, *dest;
{
    light_source *ls;

    /* src == dest implies adding to candelabrum */
    if (src != dest)
        end_burn(src, TRUE); /* extinguish candles */

    for (ls = light_base; ls; ls = ls->next)
        if (ls->type == LS_OBJECT && ls->id.a_obj == dest) {
            ls->range = candle_light_range(dest);
            vision_full_recalc = 1; /* in case range changed */
            break;
        }
}

/* light source `obj' is being made brighter or dimmer */
void
obj_adjust_light_radius(obj, new_radius)
struct obj *obj;
int new_radius;
{
    light_source *ls;

    for (ls = light_base; ls; ls = ls->next)
        if (ls->type == LS_OBJECT && ls->id.a_obj == obj) {
            if (new_radius != ls->range)
                vision_full_recalc = 1;
            ls->range = new_radius;
            return;
        }
    impossible("obj_adjust_light_radius: can't find %s", xname(obj));
}

/* Candlelight is proportional to the number of candles;
   minimum range is 2 rather than 1 for playability. */
int
candle_light_range(obj)
struct obj *obj;
{
    int radius;

    if (obj->otyp == CANDELABRUM_OF_INVOCATION) {
        /*
         *      The special candelabrum emits more light than the
         *      corresponding number of candles would.
         *       1..3 candles, range 2 (minimum range);
         *       4..6 candles, range 3 (normal lamp range);
         *          7 candles, range 4 (bright).
         */
        radius = (obj->spe < 4) ? 2 : (obj->spe < 7) ? 3 : 4;
    } else if (Is_candle(obj)) {
        /*
         *      Range is incremented by powers of 7 so that it will take
         *      wizard mode quantities of candles to get more light than
         *      from a lamp, without imposing an arbitrary limit.
         *       1..6   candles, range 2;
         *       7..48  candles, range 3;
         *      49..342 candles, range 4; &c.
         */
        long n = obj->quan;

        radius = 1; /* always incremented at least once */
        do {
            radius++;
            n /= 7L;
        } while (n > 0L);
    } else {
        /* we're only called for lit candelabrum or candles */
        /* impossible("candlelight for %d?", obj->otyp); */
        radius = 3; /* lamp's value */
    }
    return radius;
}

/* light emitting artifact's range depends upon its curse/bless state */
int
arti_light_radius(obj)
struct obj *obj;
{
    /*
     * Used by begin_burn() when setting up a new light source
     * (obj->lamplit will already be set by this point) and
     * also by bless()/unbless()/uncurse()/curse() to decide
     * whether to call obj_adjust_light_radius().
     */

    /* sanity check [simplifies usage by bless()/curse()/&c] */
    if (!obj->lamplit || !artifact_light(obj))
        return 0;

    /* cursed radius of 1 is not noticeable for an item that's
       carried by the hero but is if it's carried by a monster
       or left lit on the floor (not applicable for Sunsword) */
    return (obj->blessed ? 3 : !obj->cursed ? 2 : 1);
}

/* adverb describing lit artifact's light; depends on curse/bless state */
const char *
arti_light_description(obj)
struct obj *obj;
{
    switch (arti_light_radius(obj)) {
    case 3:
        return "brilliantly"; /* blessed */
    case 2:
        return "brightly"; /* uncursed */
    case 1:
        return "dimly"; /* cursed */
    default:
        break;
    }
    return "strangely";
}

int
wiz_light_sources()
{
    winid win;
    char buf[BUFSZ];
    light_source *ls;

    win = create_nhwindow(NHW_MENU); /* corner text window */
    if (win == WIN_ERR)
        return 0;

    Sprintf(buf, "Mobile light sources: hero @ (%2d,%2d)", u.ux, u.uy);
    putstr(win, 0, buf);
    putstr(win, 0, "");

    if (light_base) {
        putstr(win, 0, "location range flags  type    id");
        putstr(win, 0, "-------- ----- ------ ----  -------");
        for (ls = light_base; ls; ls = ls->next) {
            Sprintf(buf, "  %2d,%2d   %2d   0x%04x  %s  %s", ls->x, ls->y,
                    ls->range, ls->flags,
                    (ls->type == LS_OBJECT
                       ? "obj"
                       : ls->type == LS_MONSTER
                          ? (mon_is_local(ls->id.a_monst)
                             ? "mon"
                             : (ls->id.a_monst == &youmonst)
                                ? "you"
                                /* migrating monster */
                                : "<m>")
                          : "???"),
                    fmt_ptr(ls->id.a_void));
            putstr(win, 0, buf);
        }
    } else
        putstr(win, 0, "<none>");

    display_nhwindow(win, FALSE);
    destroy_nhwindow(win);

    return 0;
}

/*light.c*/
