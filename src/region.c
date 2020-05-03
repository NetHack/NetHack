/* NetHack 3.6	region.c	$NHDT-Date: 1579655029 2020/01/22 01:03:49 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.60 $ */
/* Copyright (c) 1996 by Jean-Christophe Collet  */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/*
 * This should really go into the level structure, but
 * I'll start here for ease. It *WILL* move into the level
 * structure eventually.
 */

#define NO_CALLBACK (-1)

boolean FDECL(inside_gas_cloud, (genericptr, genericptr));
boolean FDECL(expire_gas_cloud, (genericptr, genericptr));
boolean FDECL(inside_rect, (NhRect *, int, int));
boolean FDECL(inside_region, (NhRegion *, int, int));
NhRegion *FDECL(create_region, (NhRect *, int));
void FDECL(add_rect_to_reg, (NhRegion *, NhRect *));
void FDECL(add_mon_to_reg, (NhRegion *, struct monst *));
void FDECL(remove_mon_from_reg, (NhRegion *, struct monst *));
boolean FDECL(mon_in_region, (NhRegion *, struct monst *));

#if 0
NhRegion *FDECL(clone_region, (NhRegion *));
#endif
void FDECL(free_region, (NhRegion *));
void FDECL(add_region, (NhRegion *));
void FDECL(remove_region, (NhRegion *));

#if 0
void FDECL(replace_mon_regions, (struct monst *,struct monst *));
void FDECL(remove_mon_from_regions, (struct monst *));
NhRegion *FDECL(create_msg_region, (XCHAR_P,XCHAR_P,XCHAR_P,XCHAR_P,
                                    const char *,const char *));
boolean FDECL(enter_force_field, (genericptr,genericptr));
NhRegion *FDECL(create_force_field, (XCHAR_P,XCHAR_P,int,long));
#endif

static void FDECL(reset_region_mids, (NhRegion *));

static const callback_proc callbacks[] = {
#define INSIDE_GAS_CLOUD 0
    inside_gas_cloud,
#define EXPIRE_GAS_CLOUD 1
    expire_gas_cloud
};

/* Should be inlined. */
boolean
inside_rect(r, x, y)
NhRect *r;
int x, y;
{
    return (boolean) (x >= r->lx && x <= r->hx && y >= r->ly && y <= r->hy);
}

/*
 * Check if a point is inside a region.
 */
boolean
inside_region(reg, x, y)
NhRegion *reg;
int x, y;
{
    int i;

    if (reg == (NhRegion *) 0 || !inside_rect(&(reg->bounding_box), x, y))
        return FALSE;
    for (i = 0; i < reg->nrects; i++)
        if (inside_rect(&(reg->rects[i]), x, y))
            return TRUE;
    return FALSE;
}

/*
 * Create a region. It does not activate it.
 */
NhRegion *
create_region(rects, nrect)
NhRect *rects;
int nrect;
{
    int i;
    NhRegion *reg;

    reg = (NhRegion *) alloc(sizeof(NhRegion));
    (void) memset((genericptr_t)reg, 0, sizeof(NhRegion));
    /* Determines bounding box */
    if (nrect > 0) {
        reg->bounding_box = rects[0];
    } else {
        reg->bounding_box.lx = COLNO;
        reg->bounding_box.ly = ROWNO;
        reg->bounding_box.hx = 0; /* 1 */
        reg->bounding_box.hy = 0;
    }
    reg->nrects = nrect;
    reg->rects = (nrect > 0) ? (NhRect *) alloc(nrect * sizeof (NhRect)) : 0;
    for (i = 0; i < nrect; i++) {
        if (rects[i].lx < reg->bounding_box.lx)
            reg->bounding_box.lx = rects[i].lx;
        if (rects[i].ly < reg->bounding_box.ly)
            reg->bounding_box.ly = rects[i].ly;
        if (rects[i].hx > reg->bounding_box.hx)
            reg->bounding_box.hx = rects[i].hx;
        if (rects[i].hy > reg->bounding_box.hy)
            reg->bounding_box.hy = rects[i].hy;
        reg->rects[i] = rects[i];
    }
    reg->ttl = -1L; /* Defaults */
    reg->attach_2_u = FALSE;
    reg->attach_2_m = 0;
    /* reg->attach_2_o = NULL; */
    reg->enter_msg = (const char *) 0;
    reg->leave_msg = (const char *) 0;
    reg->expire_f = NO_CALLBACK;
    reg->enter_f = NO_CALLBACK;
    reg->can_enter_f = NO_CALLBACK;
    reg->leave_f = NO_CALLBACK;
    reg->can_leave_f = NO_CALLBACK;
    reg->inside_f = NO_CALLBACK;
    clear_hero_inside(reg);
    clear_heros_fault(reg);
    reg->n_monst = 0;
    reg->max_monst = 0;
    reg->monsters = (unsigned int *) 0;
    reg->arg = cg.zeroany;
    return reg;
}

/*
 * Add rectangle to region.
 */
void
add_rect_to_reg(reg, rect)
NhRegion *reg;
NhRect *rect;
{
    NhRect *tmp_rect;

    tmp_rect = (NhRect *) alloc((reg->nrects + 1) * sizeof (NhRect));
    if (reg->nrects > 0) {
        (void) memcpy((genericptr_t) tmp_rect, (genericptr_t) reg->rects,
                      reg->nrects * sizeof (NhRect));
        free((genericptr_t) reg->rects);
    }
    tmp_rect[reg->nrects] = *rect;
    reg->nrects++;
    reg->rects = tmp_rect;
    /* Update bounding box if needed */
    if (reg->bounding_box.lx > rect->lx)
        reg->bounding_box.lx = rect->lx;
    if (reg->bounding_box.ly > rect->ly)
        reg->bounding_box.ly = rect->ly;
    if (reg->bounding_box.hx < rect->hx)
        reg->bounding_box.hx = rect->hx;
    if (reg->bounding_box.hy < rect->hy)
        reg->bounding_box.hy = rect->hy;
}

/*
 * Add a monster to the region
 */
void
add_mon_to_reg(reg, mon)
NhRegion *reg;
struct monst *mon;
{
    int i;
    unsigned *tmp_m;

    if (reg->max_monst <= reg->n_monst) {
        tmp_m = (unsigned *) alloc(sizeof (unsigned)
                                   * (reg->max_monst + MONST_INC));
        if (reg->max_monst > 0) {
            for (i = 0; i < reg->max_monst; i++)
                tmp_m[i] = reg->monsters[i];
            free((genericptr_t) reg->monsters);
        }
        reg->monsters = tmp_m;
        reg->max_monst += MONST_INC;
    }
    reg->monsters[reg->n_monst++] = mon->m_id;
}

/*
 * Remove a monster from the region list (it left or died...)
 */
void
remove_mon_from_reg(reg, mon)
NhRegion *reg;
struct monst *mon;
{
    register int i;

    for (i = 0; i < reg->n_monst; i++)
        if (reg->monsters[i] == mon->m_id) {
            reg->n_monst--;
            reg->monsters[i] = reg->monsters[reg->n_monst];
            return;
        }
}

/*
 * Check if a monster is inside the region.
 * It's probably quicker to check with the region internal list
 * than to check for coordinates.
 */
boolean
mon_in_region(reg, mon)
NhRegion *reg;
struct monst *mon;
{
    int i;

    for (i = 0; i < reg->n_monst; i++)
        if (reg->monsters[i] == mon->m_id)
            return TRUE;
    return FALSE;
}

#if 0
/* not yet used */

/*
 * Clone (make a standalone copy) the region.
 */
NhRegion *
clone_region(reg)
NhRegion *reg;
{
    NhRegion *ret_reg;

    ret_reg = create_region(reg->rects, reg->nrects);
    ret_reg->ttl = reg->ttl;
    ret_reg->attach_2_u = reg->attach_2_u;
    ret_reg->attach_2_m = reg->attach_2_m;
 /* ret_reg->attach_2_o = reg->attach_2_o; */
    ret_reg->expire_f = reg->expire_f;
    ret_reg->enter_f = reg->enter_f;
    ret_reg->can_enter_f = reg->can_enter_f;
    ret_reg->leave_f = reg->leave_f;
    ret_reg->can_leave_f = reg->can_leave_f;
    ret_reg->player_flags = reg->player_flags; /* set/clear_hero_inside,&c*/
    ret_reg->n_monst = reg->n_monst;
    if (reg->n_monst > 0) {
        ret_reg->monsters = (unsigned int *)
                                    alloc((sizeof (unsigned)) * reg->n_monst);
        (void) memcpy((genericptr_t) ret_reg->monsters,
                      (genericptr_t) reg->monsters,
                      sizeof (unsigned) * reg->n_monst);
    } else
        ret_reg->monsters = (unsigned int *) 0;
    return ret_reg;
}

#endif /*0*/

/*
 * Free mem from region.
 */
void
free_region(reg)
NhRegion *reg;
{
    if (reg) {
        if (reg->rects)
            free((genericptr_t) reg->rects);
        if (reg->monsters)
            free((genericptr_t) reg->monsters);
        if (reg->enter_msg)
            free((genericptr_t) reg->enter_msg);
        if (reg->leave_msg)
            free((genericptr_t) reg->leave_msg);
        free((genericptr_t) reg);
    }
}

/*
 * Add a region to the list.
 * This actually activates the region.
 */
void
add_region(reg)
NhRegion *reg;
{
    NhRegion **tmp_reg;
    int i, j;

    if (g.max_regions <= g.n_regions) {
        tmp_reg = g.regions;
        g.regions =
            (NhRegion **) alloc((g.max_regions + 10) * sizeof (NhRegion *));
        if (g.max_regions > 0) {
            (void) memcpy((genericptr_t) g.regions, (genericptr_t) tmp_reg,
                          g.max_regions * sizeof (NhRegion *));
            free((genericptr_t) tmp_reg);
        }
        g.max_regions += 10;
    }
    g.regions[g.n_regions] = reg;
    g.n_regions++;
    /* Check for monsters inside the region */
    for (i = reg->bounding_box.lx; i <= reg->bounding_box.hx; i++)
        for (j = reg->bounding_box.ly; j <= reg->bounding_box.hy; j++) {
            /* Some regions can cross the level boundaries */
            if (!isok(i, j))
                continue;
            if (MON_AT(i, j) && inside_region(reg, i, j))
                add_mon_to_reg(reg, g.level.monsters[i][j]);
            if (reg->visible) {
                /*block_point(i, j);*/
                if (cansee(i, j))
                    newsym(i, j);
            }
        }
    /* Check for player now... */
    if (inside_region(reg, u.ux, u.uy))
        set_hero_inside(reg);
    else
        clear_hero_inside(reg);
}

/*
 * Remove a region from the list & free it.
 */
void
remove_region(reg)
NhRegion *reg;
{
    register int i, x, y;

    for (i = 0; i < g.n_regions; i++)
        if (g.regions[i] == reg)
            break;
    if (i == g.n_regions)
        return;

    /* remove region before potential newsym() calls, but don't free it yet */
    if (--g.n_regions != i)
        g.regions[i] = g.regions[g.n_regions];
    g.regions[g.n_regions] = (NhRegion *) 0;

    /* Update screen if necessary */
    reg->ttl = -2L; /* for visible_region_at */
    if (reg->visible)
        for (x = reg->bounding_box.lx; x <= reg->bounding_box.hx; x++)
            for (y = reg->bounding_box.ly; y <= reg->bounding_box.hy; y++)
                if (isok(x, y) && inside_region(reg, x, y)) {
                    /*if (!sobj_at(BOULDER, x, y))
                          unblock_point(x, y);*/
                    if (cansee(x, y))
                        newsym(x, y);
                }

    free_region(reg);
}

/*
 * Remove all regions and clear all related data (This must be down
 * when changing level, for instance).
 */
void
clear_regions()
{
    register int i;

    for (i = 0; i < g.n_regions; i++)
        free_region(g.regions[i]);
    g.n_regions = 0;
    if (g.max_regions > 0)
        free((genericptr_t) g.regions);
    g.max_regions = 0;
    g.regions = (NhRegion **) 0;
}

/*
 * This function is called every turn.
 * It makes the regions age, if necessary and calls the appropriate
 * callbacks when needed.
 */
void
run_regions()
{
    register int i, j, k;
    int f_indx;

    /* End of life ? */
    /* Do it backward because the array will be modified */
    for (i = g.n_regions - 1; i >= 0; i--) {
        if (g.regions[i]->ttl == 0L) {
            if ((f_indx = g.regions[i]->expire_f) == NO_CALLBACK
                || (*callbacks[f_indx])(g.regions[i], (genericptr_t) 0))
                remove_region(g.regions[i]);
        }
    }

    /* Process remaining regions */
    for (i = 0; i < g.n_regions; i++) {
        /* Make the region age */
        if (g.regions[i]->ttl > 0L)
            g.regions[i]->ttl--;
        /* Check if player is inside region */
        f_indx = g.regions[i]->inside_f;
        if (f_indx != NO_CALLBACK && hero_inside(g.regions[i]))
            (void) (*callbacks[f_indx])(g.regions[i], (genericptr_t) 0);
        /* Check if any monster is inside region */
        if (f_indx != NO_CALLBACK) {
            for (j = 0; j < g.regions[i]->n_monst; j++) {
                struct monst *mtmp =
                    find_mid(g.regions[i]->monsters[j], FM_FMON);

                if (!mtmp || DEADMONSTER(mtmp)
                    || (*callbacks[f_indx])(g.regions[i], mtmp)) {
                    /* The monster died, remove it from list */
                    k = (g.regions[i]->n_monst -= 1);
                    g.regions[i]->monsters[j] = g.regions[i]->monsters[k];
                    g.regions[i]->monsters[k] = 0;
                    --j; /* current slot has been reused; recheck it next */
                }
            }
        }
    }
}

/*
 * check whether player enters/leaves one or more regions.
 */
boolean
in_out_region(x, y)
xchar x, y;
{
    int i, f_indx = 0;

    /* First check if hero can do the move */
    for (i = 0; i < g.n_regions; i++) {
        if (g.regions[i]->attach_2_u)
            continue;
        if (inside_region(g.regions[i], x, y)
            ? (!hero_inside(g.regions[i])
               && (f_indx = g.regions[i]->can_enter_f) != NO_CALLBACK)
            : (hero_inside(g.regions[i])
               && (f_indx = g.regions[i]->can_leave_f) != NO_CALLBACK)) {
            if (!(*callbacks[f_indx])(g.regions[i], (genericptr_t) 0))
                return FALSE;
        }
    }

    /* Callbacks for the regions hero does leave */
    for (i = 0; i < g.n_regions; i++) {
        if (g.regions[i]->attach_2_u)
            continue;
        if (hero_inside(g.regions[i])
            && !inside_region(g.regions[i], x, y)) {
            clear_hero_inside(g.regions[i]);
            if (g.regions[i]->leave_msg != (const char *) 0)
                pline1(g.regions[i]->leave_msg);
            if ((f_indx = g.regions[i]->leave_f) != NO_CALLBACK)
                (void) (*callbacks[f_indx])(g.regions[i], (genericptr_t) 0);
        }
    }

    /* Callbacks for the regions hero does enter */
    for (i = 0; i < g.n_regions; i++) {
        if (g.regions[i]->attach_2_u)
            continue;
        if (!hero_inside(g.regions[i])
            && inside_region(g.regions[i], x, y)) {
            set_hero_inside(g.regions[i]);
            if (g.regions[i]->enter_msg != (const char *) 0)
                pline1(g.regions[i]->enter_msg);
            if ((f_indx = g.regions[i]->enter_f) != NO_CALLBACK)
                (void) (*callbacks[f_indx])(g.regions[i], (genericptr_t) 0);
        }
    }

    return TRUE;
}

/*
 * check whether a monster enters/leaves one or more regions.
 */
boolean
m_in_out_region(mon, x, y)
struct monst *mon;
xchar x, y;
{
    int i, f_indx = 0;

    /* First check if mon can do the move */
    for (i = 0; i < g.n_regions; i++) {
        if (g.regions[i]->attach_2_m == mon->m_id)
            continue;
        if (inside_region(g.regions[i], x, y)
            ? (!mon_in_region(g.regions[i], mon)
               && (f_indx = g.regions[i]->can_enter_f) != NO_CALLBACK)
            : (mon_in_region(g.regions[i], mon)
               && (f_indx = g.regions[i]->can_leave_f) != NO_CALLBACK)) {
            if (!(*callbacks[f_indx])(g.regions[i], mon))
                return FALSE;
        }
    }

    /* Callbacks for the regions mon does leave */
    for (i = 0; i < g.n_regions; i++) {
        if (g.regions[i]->attach_2_m == mon->m_id)
            continue;
        if (mon_in_region(g.regions[i], mon)
            && !inside_region(g.regions[i], x, y)) {
            remove_mon_from_reg(g.regions[i], mon);
            if ((f_indx = g.regions[i]->leave_f) != NO_CALLBACK)
                (void) (*callbacks[f_indx])(g.regions[i], mon);
        }
    }

    /* Callbacks for the regions mon does enter */
    for (i = 0; i < g.n_regions; i++) {
        if (g.regions[i]->attach_2_m == mon->m_id)
            continue;
        if (!mon_in_region(g.regions[i], mon)
            && inside_region(g.regions[i], x, y)) {
            add_mon_to_reg(g.regions[i], mon);
            if ((f_indx = g.regions[i]->enter_f) != NO_CALLBACK)
                (void) (*callbacks[f_indx])(g.regions[i], mon);
        }
    }

    return TRUE;
}

/*
 * Checks player's regions after a teleport for instance.
 */
void
update_player_regions()
{
    register int i;

    for (i = 0; i < g.n_regions; i++)
        if (!g.regions[i]->attach_2_u
            && inside_region(g.regions[i], u.ux, u.uy))
            set_hero_inside(g.regions[i]);
        else
            clear_hero_inside(g.regions[i]);
}

/*
 * Ditto for a specified monster.
 */
void
update_monster_region(mon)
struct monst *mon;
{
    register int i;

    for (i = 0; i < g.n_regions; i++) {
        if (inside_region(g.regions[i], mon->mx, mon->my)) {
            if (!mon_in_region(g.regions[i], mon))
                add_mon_to_reg(g.regions[i], mon);
        } else {
            if (mon_in_region(g.regions[i], mon))
                remove_mon_from_reg(g.regions[i], mon);
        }
    }
}

#if 0
/* not yet used */

/*
 * Change monster pointer in g.regions
 * This happens, for instance, when a monster grows and
 * need a new structure (internally that is).
 */
void
replace_mon_regions(monold, monnew)
struct monst *monold, *monnew;
{
    register int i;

    for (i = 0; i < g.n_regions; i++)
        if (mon_in_region(g.regions[i], monold)) {
            remove_mon_from_reg(g.regions[i], monold);
            add_mon_to_reg(g.regions[i], monnew);
        }
}

/*
 * Remove monster from all regions it was in (ie monster just died)
 */
void
remove_mon_from_regions(mon)
struct monst *mon;
{
    register int i;

    for (i = 0; i < g.n_regions; i++)
        if (mon_in_region(g.regions[i], mon))
            remove_mon_from_reg(g.regions[i], mon);
}

#endif /*0*/

/*
 * Check if a spot is under a visible region (eg: gas cloud).
 * Returns NULL if not, otherwise returns region.
 */
NhRegion *
visible_region_at(x, y)
xchar x, y;
{
    register int i;

    for (i = 0; i < g.n_regions; i++) {
        if (!g.regions[i]->visible || g.regions[i]->ttl == -2L)
            continue;
        if (inside_region(g.regions[i], x, y))
            return g.regions[i];
    }
    return (NhRegion *) 0;
}

void
show_region(reg, x, y)
NhRegion *reg;
xchar x, y;
{
    show_glyph(x, y, reg->glyph);
}

/**
 * save_regions :
 */
void
save_regions(nhfp)
NHFILE *nhfp;
{
    int i, j;
    unsigned n;

    if (!perform_bwrite(nhfp))
        goto skip_lots;
    if (nhfp->structlevel) {
        bwrite(nhfp->fd, (genericptr_t) &g.moves, sizeof (g.moves));	/* timestamp */
        bwrite(nhfp->fd, (genericptr_t) &g.n_regions, sizeof (g.n_regions));
    }
    for (i = 0; i < g.n_regions; i++) {
        if (nhfp->structlevel) {
            bwrite(nhfp->fd, (genericptr_t) &g.regions[i]->bounding_box, sizeof (NhRect));
            bwrite(nhfp->fd, (genericptr_t) &g.regions[i]->nrects, sizeof (short));
        }
        for (j = 0; j < g.regions[i]->nrects; j++) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) &g.regions[i]->rects[j], sizeof (NhRect));
        }
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &g.regions[i]->attach_2_u, sizeof (boolean));
        n = 0;

        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &g.regions[i]->attach_2_m, sizeof (unsigned));

        n = g.regions[i]->enter_msg != (const char *)0 ?
                                        strlen(g.regions[i]->enter_msg) : 0;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &n, sizeof n);

        if (n > 0) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) g.regions[i]->enter_msg, n);
        }
        n = g.regions[i]->leave_msg != (const char *)0 ?
                                        strlen(g.regions[i]->leave_msg) : 0;
        if (nhfp->structlevel)
            bwrite(nhfp->fd, (genericptr_t) &n, sizeof n);

        if (n > 0) {
            if (nhfp->structlevel) {
                bwrite(nhfp->fd, (genericptr_t) g.regions[i]->leave_msg, n);
            }
        }
        if (nhfp->structlevel) {
            bwrite(nhfp->fd, (genericptr_t) &g.regions[i]->ttl, sizeof (long));
            bwrite(nhfp->fd, (genericptr_t) &g.regions[i]->expire_f, sizeof (short));
            bwrite(nhfp->fd, (genericptr_t) &g.regions[i]->can_enter_f, sizeof (short));
            bwrite(nhfp->fd, (genericptr_t) &g.regions[i]->enter_f, sizeof (short));
            bwrite(nhfp->fd, (genericptr_t) &g.regions[i]->can_leave_f, sizeof (short));
            bwrite(nhfp->fd, (genericptr_t) &g.regions[i]->leave_f, sizeof (short));
            bwrite(nhfp->fd, (genericptr_t) &g.regions[i]->inside_f, sizeof (short));
            bwrite(nhfp->fd, (genericptr_t) &g.regions[i]->player_flags, sizeof (unsigned int));
            bwrite(nhfp->fd, (genericptr_t) &g.regions[i]->n_monst, sizeof (short));
        }
        for (j = 0; j < g.regions[i]->n_monst; j++) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) &g.regions[i]->monsters[j],
                        sizeof (unsigned));
        }
        if (nhfp->structlevel) {
            bwrite(nhfp->fd, (genericptr_t) &g.regions[i]->visible, sizeof (boolean));
            bwrite(nhfp->fd, (genericptr_t) &g.regions[i]->glyph, sizeof (int));
            bwrite(nhfp->fd, (genericptr_t) &g.regions[i]->arg, sizeof (anything));
        }
    }
  
  skip_lots:
    if (release_data(nhfp))
        clear_regions();
}

void
rest_regions(nhfp)
NHFILE *nhfp;
{
    int i, j;
    unsigned n = 0;
    long tmstamp = 0L;
    char *msg_buf;
    boolean ghostly = (nhfp->ftype == NHF_BONESFILE);

    clear_regions();		/* Just for security */
    if (nhfp->structlevel)
        mread(nhfp->fd, (genericptr_t) &tmstamp, sizeof (tmstamp));
    if (ghostly)
        tmstamp = 0;
    else
        tmstamp = (g.moves - tmstamp);

    if (nhfp->structlevel)
        mread(nhfp->fd, (genericptr_t) &g.n_regions, sizeof (g.n_regions));

    g.max_regions = g.n_regions;
    if (g.n_regions > 0)
        g.regions = (NhRegion **) alloc(sizeof (NhRegion *) * g.n_regions);
    for (i = 0; i < g.n_regions; i++) {
        g.regions[i] = (NhRegion *) alloc(sizeof (NhRegion));
        if (nhfp->structlevel) {
            mread(nhfp->fd, (genericptr_t) &g.regions[i]->bounding_box, sizeof (NhRect));
            mread(nhfp->fd, (genericptr_t) &g.regions[i]->nrects, sizeof (short));
        }
        if (g.regions[i]->nrects > 0)
            g.regions[i]->rects = (NhRect *)
                                 alloc(sizeof (NhRect) * g.regions[i]->nrects);
        for (j = 0; j < g.regions[i]->nrects; j++) {
            if (nhfp->structlevel)
                mread(nhfp->fd, (genericptr_t) &g.regions[i]->rects[j], sizeof (NhRect));
        }
        if (nhfp->structlevel) {
            mread(nhfp->fd, (genericptr_t) &g.regions[i]->attach_2_u, sizeof (boolean));
            mread(nhfp->fd, (genericptr_t) &g.regions[i]->attach_2_m, sizeof (unsigned));
        }

        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) &n, sizeof n);
        if (n > 0) {
            msg_buf = (char *) alloc(n + 1);
            if (nhfp->structlevel) {
                mread(nhfp->fd, (genericptr_t) msg_buf, n);
            }
            msg_buf[n] = '\0';
            g.regions[i]->enter_msg = (const char *) msg_buf;
        } else
            g.regions[i]->enter_msg = (const char *)0;

        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) &n, sizeof n);
         if (n > 0) {
            msg_buf = (char *) alloc(n + 1);
            if (nhfp->structlevel) {
                mread(nhfp->fd, (genericptr_t) msg_buf, n);
            }
            msg_buf[n] = '\0';
	    g.regions[i]->leave_msg = (const char *) msg_buf;
        } else
            g.regions[i]->leave_msg = (const char *)0;

        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) &g.regions[i]->ttl, sizeof (long));
        /* check for expired region */
        if (g.regions[i]->ttl >= 0L)
            g.regions[i]->ttl =
		(g.regions[i]->ttl > tmstamp) ? g.regions[i]->ttl - tmstamp : 0L;
        if (nhfp->structlevel) {
            mread(nhfp->fd, (genericptr_t) &g.regions[i]->expire_f,
                    sizeof (short));
            mread(nhfp->fd, (genericptr_t) &g.regions[i]->can_enter_f,
                    sizeof (short));
            mread(nhfp->fd, (genericptr_t) &g.regions[i]->enter_f,
                    sizeof (short));
            mread(nhfp->fd, (genericptr_t) &g.regions[i]->can_leave_f,
                    sizeof (short));
            mread(nhfp->fd, (genericptr_t) &g.regions[i]->leave_f,
                    sizeof (short));
            mread(nhfp->fd, (genericptr_t) &g.regions[i]->inside_f,
                    sizeof (short));
            mread(nhfp->fd, (genericptr_t) &g.regions[i]->player_flags,
                    sizeof (unsigned int));
        }
        if (ghostly) {	/* settings pertained to old player */
            clear_hero_inside(g.regions[i]);
            clear_heros_fault(g.regions[i]);
        }
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t) &g.regions[i]->n_monst, sizeof (short));
        if (g.regions[i]->n_monst > 0)
            g.regions[i]->monsters =
                        (unsigned *) alloc(sizeof (unsigned) * g.regions[i]->n_monst);
        else
            g.regions[i]->monsters = (unsigned int *)0;
        g.regions[i]->max_monst = g.regions[i]->n_monst;
        for (j = 0; j < g.regions[i]->n_monst; j++) {
            if (nhfp->structlevel)
                mread(nhfp->fd, (genericptr_t) &g.regions[i]->monsters[j],
                        sizeof (unsigned));
        }
        if (nhfp->structlevel) {
            mread(nhfp->fd, (genericptr_t) &g.regions[i]->visible, sizeof (boolean));
            mread(nhfp->fd, (genericptr_t) &g.regions[i]->glyph, sizeof (int));
            mread(nhfp->fd, (genericptr_t) &g.regions[i]->arg, sizeof (anything));
        }
    }
    /* remove expired regions, do not trigger the expire_f callback (yet!);
       also update monster lists if this data is coming from a bones file */
    for (i = g.n_regions - 1; i >= 0; i--)
        if (g.regions[i]->ttl == 0L)
            remove_region(g.regions[i]);
        else if (ghostly && g.regions[i]->n_monst > 0)
            reset_region_mids(g.regions[i]);
}

/* to support '#stats' wizard-mode command */
void
region_stats(hdrfmt, hdrbuf, count, size)
const char *hdrfmt;
char *hdrbuf;
long *count, *size;
{
    NhRegion *rg;
    int i;

    /* other stats formats take one parameter; this takes two */
    Sprintf(hdrbuf, hdrfmt, (long) sizeof (NhRegion), (long) sizeof (NhRect));
    *count = (long) g.n_regions; /* might be 0 even though g.max_regions isn't */
    *size = (long) g.max_regions * (long) sizeof (NhRegion);
    for (i = 0; i < g.n_regions; ++i) {
        rg = g.regions[i];
        *size += (long) rg->nrects * (long) sizeof (NhRect);
        if (rg->enter_msg)
            *size += (long) (strlen(rg->enter_msg) + 1);
        if (rg->leave_msg)
            *size += (long) (strlen(rg->leave_msg) + 1);
        *size += (long) rg->max_monst * (long) sizeof *rg->monsters;
    }
    /* ? */
}

/* update monster IDs for region being loaded from bones; `ghostly' implied */
static void
reset_region_mids(reg)
NhRegion *reg;
{
    int i = 0, n = reg->n_monst;
    unsigned *mid_list = reg->monsters;

    while (i < n)
        if (!lookup_id_mapping(mid_list[i], &mid_list[i])) {
            /* shrink list to remove missing monster; order doesn't matter */
            mid_list[i] = mid_list[--n];
        } else {
            /* move on to next monster */
            ++i;
        }
    reg->n_monst = n;
    return;
}

#if 0
/* not yet used */

/*--------------------------------------------------------------*
 *                                                              *
 *                      Create Region with just a message       *
 *                                                              *
 *--------------------------------------------------------------*/

NhRegion *
create_msg_region(x, y, w, h, msg_enter, msg_leave)
xchar x, y;
xchar w, h;
const char *msg_enter;
const char *msg_leave;
{
    NhRect tmprect;
    NhRegion *reg = create_region((NhRect *) 0, 0);

    if (msg_enter)
        reg->enter_msg = dupstr(msg_enter);
    if (msg_leave)
        reg->leave_msg = dupstr(msg_leave);
    tmprect.lx = x;
    tmprect.ly = y;
    tmprect.hx = x + w;
    tmprect.hy = y + h;
    add_rect_to_reg(reg, &tmprect);
    reg->ttl = -1L;
    return reg;
}


/*--------------------------------------------------------------*
 *                                                              *
 *                      Force Field Related Cod                 *
 *                      (unused yet)                            *
 *--------------------------------------------------------------*/

boolean
enter_force_field(p1, p2)
genericptr_t p1;
genericptr_t p2;
{
    struct monst *mtmp;

    if (p2 == (genericptr_t) 0) { /* That means the player */
        if (!Blind)
            You("bump into %s.  Ouch!",
                Hallucination ? "an invisible tree"
                              : "some kind of invisible wall");
        else
            pline("Ouch!");
    } else {
        mtmp = (struct monst *) p2;
        if (canseemon(mtmp))
            pline("%s bumps into %s!", Monnam(mtmp), something);
    }
    return FALSE;
}

NhRegion *
create_force_field(x, y, radius, ttl)
xchar x, y;
int radius;
long ttl;
{
    int i;
    NhRegion *ff;
    int nrect;
    NhRect tmprect;

    ff = create_region((NhRect *) 0, 0);
    nrect = radius;
    tmprect.lx = x;
    tmprect.hx = x;
    tmprect.ly = y - (radius - 1);
    tmprect.hy = y + (radius - 1);
    for (i = 0; i < nrect; i++) {
        add_rect_to_reg(ff, &tmprect);
        tmprect.lx--;
        tmprect.hx++;
        tmprect.ly++;
        tmprect.hy--;
    }
    ff->ttl = ttl;
    if (!g.in_mklev && !g.context.mon_moving)
        set_heros_fault(ff); /* assume player has created it */
 /* ff->can_enter_f = enter_force_field; */
 /* ff->can_leave_f = enter_force_field; */
    add_region(ff);
    return ff;
}

#endif /*0*/

/*--------------------------------------------------------------*
 *                                                              *
 *                      Gas cloud related code                  *
 *                                                              *
 *--------------------------------------------------------------*/

/*
 * Here is an example of an expire function that may prolong
 * region life after some mods...
 */
/*ARGSUSED*/
boolean
expire_gas_cloud(p1, p2)
genericptr_t p1;
genericptr_t p2 UNUSED;
{
    NhRegion *reg;
    int damage;

    reg = (NhRegion *) p1;
    damage = reg->arg.a_int;

    /* If it was a thick cloud, it dissipates a little first */
    if (damage >= 5) {
        damage /= 2; /* It dissipates, let's do less damage */
        reg->arg = cg.zeroany;
        reg->arg.a_int = damage;
        reg->ttl = 2L; /* Here's the trick : reset ttl */
        return FALSE;  /* THEN return FALSE, means "still there" */
    }
    return TRUE; /* OK, it's gone, you can free it! */
}

/* returns True if p2 is killed by region p1, False otherwise */
boolean
inside_gas_cloud(p1, p2)
genericptr_t p1;
genericptr_t p2;
{
    NhRegion *reg = (NhRegion *) p1;
    struct monst *mtmp = (struct monst *) p2;
    int dam = reg->arg.a_int;

    /*
     * Gas clouds can't be targetted at water locations, but they can
     * start next to water and spread over it.
     */

    if (dam < 1)
        return FALSE; /* if no damage then there's nothing to do here... */

    if (!mtmp) { /* hero is indicated by Null rather than by &youmonst */
        if (m_poisongas_ok(&g.youmonst) == M_POISONGAS_OK)
            return FALSE;
        if (!Blind) {
            Your("%s sting.", makeplural(body_part(EYE)));
            make_blinded(1L, FALSE);
        }
        if (!Poison_resistance) {
            pline("%s is burning your %s!", Something,
                  makeplural(body_part(LUNG)));
            You("cough and spit blood!");
            dam = Maybe_Half_Phys(rnd(dam) + 5);
            if (Half_gas_damage) /* worn towel */
                dam = (dam + 1) / 2;
            losehp(dam, "gas cloud", KILLED_BY_AN);
            return FALSE;
        } else {
            You("cough!");
            return FALSE;
        }
    } else { /* A monster is inside the cloud */
        mtmp = (struct monst *) p2;

        if (m_poisongas_ok(mtmp) != M_POISONGAS_OK) {
            if (cansee(mtmp->mx, mtmp->my))
                pline("%s coughs!", Monnam(mtmp));
            if (heros_fault(reg))
                setmangry(mtmp, TRUE);
            if (haseyes(mtmp->data) && mtmp->mcansee) {
                mtmp->mblinded = 1;
                mtmp->mcansee = 0;
            }
            if (resists_poison(mtmp))
                return FALSE;
            mtmp->mhp -= rnd(dam) + 5;
            if (DEADMONSTER(mtmp)) {
                if (heros_fault(reg))
                    killed(mtmp);
                else
                    monkilled(mtmp, "gas cloud", AD_DRST);
                if (DEADMONSTER(mtmp)) { /* not lifesaved */
                    return TRUE;
                }
            }
        }
    }
    return FALSE; /* Monster is still alive */
}

NhRegion *
create_gas_cloud(x, y, radius, damage)
xchar x, y;
int radius;
int damage;
{
    NhRegion *cloud;
    int i, nrect;
    NhRect tmprect;

    cloud = create_region((NhRect *) 0, 0);
    nrect = radius;
    tmprect.lx = x;
    tmprect.hx = x;
    tmprect.ly = y - (radius - 1);
    tmprect.hy = y + (radius - 1);
    for (i = 0; i < nrect; i++) {
        add_rect_to_reg(cloud, &tmprect);
        tmprect.lx--;
        tmprect.hx++;
        tmprect.ly++;
        tmprect.hy--;
    }
    cloud->ttl = rn1(3, 4);
    if (!g.in_mklev && !g.context.mon_moving)
        set_heros_fault(cloud); /* assume player has created it */
    cloud->inside_f = INSIDE_GAS_CLOUD;
    cloud->expire_f = EXPIRE_GAS_CLOUD;
    cloud->arg = cg.zeroany;
    cloud->arg.a_int = damage;
    cloud->visible = TRUE;
    cloud->glyph = cmap_to_glyph(damage ? S_poisoncloud : S_cloud);
    add_region(cloud);
    return cloud;
}

/* for checking troubles during prayer; is hero at risk? */
boolean
region_danger()
{
    int i, f_indx, n = 0;

    for (i = 0; i < g.n_regions; i++) {
        /* only care about regions that hero is in */
        if (!hero_inside(g.regions[i]))
            continue;
        f_indx = g.regions[i]->inside_f;
        /* the only type of region we understand is gas_cloud */
        if (f_indx == INSIDE_GAS_CLOUD) {
            /* completely harmless if you don't need to breathe */
            if (nonliving(g.youmonst.data) || Breathless)
                continue;
            /* minor inconvenience if you're poison resistant;
               not harmful enough to be a prayer-level trouble */
            if (Poison_resistance)
                continue;
            ++n;
        }
    }
    return n ? TRUE : FALSE;
}

/* for fixing trouble at end of prayer;
   danger detected at start of prayer might have expired by now */
void
region_safety()
{
    NhRegion *r = 0;
    int i, f_indx, n = 0;

    for (i = 0; i < g.n_regions; i++) {
        /* only care about regions that hero is in */
        if (!hero_inside(g.regions[i]))
            continue;
        f_indx = g.regions[i]->inside_f;
        /* the only type of region we understand is gas_cloud */
        if (f_indx == INSIDE_GAS_CLOUD) {
            if (!n++ && g.regions[i]->ttl >= 0)
                r = g.regions[i];
        }
    }

    if (n > 1 || (n == 1 && !r)) {
        /* multiple overlapping cloud regions or non-expiring one */
        safe_teleds(FALSE);
    } else if (r) {
        remove_region(r);
        pline_The("gas cloud enveloping you dissipates.");
    } else {
        /* cloud dissipated on its own, so nothing needs to be done */
        pline_The("gas cloud has dissipated.");
    }
    /* maybe cure blindness too */
    if ((Blinded & TIMEOUT) == 1L)
        make_blinded(0L, TRUE);
}

/*region.c*/
