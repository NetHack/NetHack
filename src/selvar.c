/* NetHack 3.7	selvar.c	$NHDT-Date: 1709677544 2024/03/05 22:25:44 $  $NHDT-Branch: keni-mdlib-followup $:$NHDT-Revision: 1.360 $ */
/* Copyright (c) 2024 by Pasi Kallinen */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "selvar.h"
#include "sp_lev.h"

staticfn boolean sel_flood_havepoint(coordxy, coordxy, coordxy *, coordxy *,
                                    int);
staticfn long line_dist_coord(long, long, long, long, long, long);

/* selection */
struct selectionvar *
selection_new(void)
{
    struct selectionvar *tmps = (struct selectionvar *) alloc(sizeof *tmps);

    tmps->wid = COLNO;
    tmps->hei = ROWNO;
    tmps->bounds_dirty = FALSE;
    tmps->bounds.lx = COLNO;
    tmps->bounds.ly = ROWNO;
    tmps->bounds.hx = tmps->bounds.hy = 0;
    tmps->map = (char *) alloc((COLNO * ROWNO) + 1);
    (void) memset(tmps->map, 1, (COLNO * ROWNO));
    tmps->map[(COLNO * ROWNO)] = '\0';

    return tmps;
}

void
selection_free(struct selectionvar *sel, boolean freesel)
{
    if (sel) {
        if (sel->map)
            free(sel->map);
        sel->map = NULL;
        if (freesel)
            free((genericptr_t) sel);
        else
            (void) memset((genericptr_t) sel, 0, sizeof *sel);
    }
}

/* clear selection, setting all locations to value val */
void
selection_clear(struct selectionvar *sel, int val)
{
    (void) memset(sel->map, 1 + val, (COLNO * ROWNO));
    if (val) {
        sel->bounds.lx = 0;
        sel->bounds.ly = 0;
        sel->bounds.hx = COLNO - 1;
        sel->bounds.hy = ROWNO - 1;
    } else {
        sel->bounds.lx = COLNO;
        sel->bounds.ly = ROWNO;
        sel->bounds.hx = sel->bounds.hy = 0;
    }
    sel->bounds_dirty = FALSE;
}

struct selectionvar *
selection_clone(struct selectionvar *sel)
{
    struct selectionvar *tmps = (struct selectionvar *) alloc(sizeof *tmps);

    *tmps = *sel;
    tmps->map = dupstr(sel->map);

    return tmps;
}

/* get boundary rect of selection sel into b */
void
selection_getbounds(struct selectionvar *sel, NhRect *b)
{
    if (!sel || !b)
        return;

    selection_recalc_bounds(sel);

    if (sel->bounds.lx >= sel->wid) {
        b->lx = 0;
        b->ly = 0;
        b->hx = COLNO - 1;
        b->hy = ROWNO - 1;
    } else {
        b->lx = sel->bounds.lx;
        b->ly = sel->bounds.ly;
        b->hx = sel->bounds.hx;
        b->hy = sel->bounds.hy;
    }
}

/* recalc the boundary of selection, if necessary */
void
selection_recalc_bounds(struct selectionvar *sel)
{
    coordxy x, y;
    NhRect r;

    if (!sel->bounds_dirty)
        return;

    sel->bounds.lx = COLNO;
    sel->bounds.ly = ROWNO;
    sel->bounds.hx = sel->bounds.hy = 0;

    r.lx = r.ly = r.hx = r.hy = -1;

    /* left */
    for (x = 0; x < sel->wid; x++) {
        for (y = 0; y < sel->hei; y++) {
            if (selection_getpoint(x, y, sel)) {
                r.lx = x;
                break;
            }
        }
        if (r.lx > -1)
            break;
    }

    if (r.lx > -1) {
        /* right */
        for (x = sel->wid-1; x >= r.lx; x--) {
            for (y = 0; y < sel->hei; y++) {
                if (selection_getpoint(x, y, sel)) {
                    r.hx = x;
                    break;
                }
            }
            if (r.hx > -1)
                break;
        }

        /* top */
        for (y = 0; y < sel->hei; y++) {
            for (x = r.lx; x <= r.hx; x++) {
                if (selection_getpoint(x, y, sel)) {
                    r.ly = y;
                    break;
                }
            }
            if (r.ly > -1)
                break;
        }

        /* bottom */
        for (y = sel->hei-1; y >= r.ly; y--) {
            for (x = r.lx; x <= r.hx; x++) {
                if (selection_getpoint(x, y, sel)) {
                    r.hy = y;
                    break;
                }
            }
            if (r.hy > -1)
                break;
        }
        sel->bounds = r;
    }

    sel->bounds_dirty = FALSE;
}

coordxy
selection_getpoint(
    coordxy x, coordxy y,
    struct selectionvar *sel)
{
    if (!sel || !sel->map)
        return 0;
    if (x < 0 || y < 0 || x >= sel->wid || y >= sel->hei)
        return 0;

    return (sel->map[sel->wid * y + x] - 1);
}

void
selection_setpoint(
    coordxy x, coordxy y,
    struct selectionvar *sel,
    int c)
{
    if (!sel || !sel->map)
        return;
    if (x < 0 || y < 0 || x >= sel->wid || y >= sel->hei)
        return;

    if (c && !sel->bounds_dirty) {
        if (sel->bounds.lx > x) sel->bounds.lx = x;
        if (sel->bounds.ly > y) sel->bounds.ly = y;
        if (sel->bounds.hx < x) sel->bounds.hx = x;
        if (sel->bounds.hy < y) sel->bounds.hy = y;
    } else {
        sel->bounds_dirty = TRUE;
    }

    sel->map[sel->wid * y + x] = (char) (c + 1);
}

struct selectionvar *
selection_not(struct selectionvar *s)
{
    int x, y;
    NhRect tmprect = cg.zeroNhRect;

    for (x = 0; x < s->wid; x++)
        for (y = 0; y < s->hei; y++)
            selection_setpoint(x, y, s, selection_getpoint(x, y, s) ? 0 : 1);
    selection_getbounds(s, &tmprect);
    return s;
}

struct selectionvar *
selection_filter_percent(
    struct selectionvar *ov,
    int percent)
{
    int x, y;
    struct selectionvar *ret;
    NhRect rect = cg.zeroNhRect;

    if (!ov)
        return NULL;

    ret = selection_new();

    selection_getbounds(ov, &rect);

    for (x = rect.lx; x <= rect.hx; x++)
        for (y = rect.ly; y <= rect.hy; y++)
            if (selection_getpoint(x, y, ov) && (rn2(100) < percent))
                selection_setpoint(x, y, ret, 1);

    return ret;
}

struct selectionvar *
selection_filter_mapchar(struct selectionvar *ov,  xint16 typ, int lit)
{
    int x, y;
    struct selectionvar *ret;
    NhRect rect = cg.zeroNhRect;

    if (!ov)
        return NULL;

    ret = selection_new();

    selection_getbounds(ov, &rect);

    for (x = rect.lx; x <= rect.hx; x++)
        for (y = rect.ly; y <= rect.hy; y++)
            if (selection_getpoint(x, y, ov)
                && match_maptyps(typ, levl[x][y].typ)) {
                switch (lit) {
                default:
                case -2:
                    selection_setpoint(x, y, ret, 1);
                    break;
                case -1:
                    selection_setpoint(x, y, ret, rn2(2));
                    break;
                case 0:
                case 1:
                    if (levl[x][y].lit == (unsigned int) lit)
                        selection_setpoint(x, y, ret, 1);
                    break;
                }
            }
    return ret;
}

int
selection_rndcoord(
    struct selectionvar *ov,
    coordxy *x, coordxy *y,
    boolean removeit)
{
    int idx = 0;
    int c;
    int dx, dy;
    NhRect rect = cg.zeroNhRect;

    selection_getbounds(ov, &rect);

    for (dx = rect.lx; dx <= rect.hx; dx++)
        for (dy = rect.ly; dy <= rect.hy; dy++)
            if (selection_getpoint(dx, dy, ov))
                idx++;

    if (idx) {
        c = rn2(idx);
        for (dx = rect.lx; dx <= rect.hx; dx++)
            for (dy = rect.ly; dy <= rect.hy; dy++)
                if (selection_getpoint(dx, dy, ov)) {
                    if (!c) {
                        *x = dx;
                        *y = dy;
                        if (removeit)
                            selection_setpoint(dx, dy, ov, 0);
                        return 1;
                    }
                    c--;
                }
    }
    *x = *y = -1;
    return 0;
}

void
selection_do_grow(struct selectionvar *ov, int dir)
{
    coordxy x, y;
    struct selectionvar *tmp;
    NhRect rect = cg.zeroNhRect;

    if (!ov)
        return;

    tmp = selection_new();

    if (dir == W_RANDOM)
        dir = random_wdir();

    selection_getbounds(ov, &rect);

    for (x = max(0, rect.lx-1); x <= min(COLNO-1, rect.hx+1); x++)
        for (y = max(0, rect.ly-1); y <= min(ROWNO-1, rect.hy+1); y++) {
            /* note:  dir is a mask of multiple directions, but the only
               way to specify diagonals is by including the two adjacent
               orthogonal directions, which effectively specifies three-
               way growth [WEST|NORTH => WEST plus WEST|NORTH plus NORTH] */
            if (((dir & W_WEST) && selection_getpoint(x + 1, y, ov))
                || (((dir & (W_WEST | W_NORTH)) == (W_WEST | W_NORTH))
                    && selection_getpoint(x + 1, y + 1, ov))
                || ((dir & W_NORTH) && selection_getpoint(x, y + 1, ov))
                || (((dir & (W_NORTH | W_EAST)) == (W_NORTH | W_EAST))
                    && selection_getpoint(x - 1, y + 1, ov))
                || ((dir & W_EAST) && selection_getpoint(x - 1, y, ov))
                || (((dir & (W_EAST | W_SOUTH)) == (W_EAST | W_SOUTH))
                    && selection_getpoint(x - 1, y - 1, ov))
                || ((dir & W_SOUTH) && selection_getpoint(x, y - 1, ov))
                ||  (((dir & (W_SOUTH | W_WEST)) == (W_SOUTH | W_WEST))
                     && selection_getpoint(x + 1, y - 1, ov))) {
                selection_setpoint(x, y, tmp, 1);
            }
        }

    selection_getbounds(tmp, &rect);

    for (x = rect.lx; x <= rect.hx; x++)
        for (y = rect.ly; y <= rect.hy; y++)
            if (selection_getpoint(x, y, tmp))
                selection_setpoint(x, y, ov, 1);

    selection_free(tmp, TRUE);
}

staticfn int (*selection_flood_check_func)(coordxy, coordxy);

void
set_selection_floodfillchk(int (*f)(coordxy, coordxy))
{
    selection_flood_check_func = f;
}

/* check whethere <x,y> is already in xs[],ys[] */
staticfn boolean
sel_flood_havepoint(
    coordxy x, coordxy y,
    coordxy xs[], coordxy ys[],
    int n)
{
    coordxy xx = x, yy = y;

    while (n > 0) {
        --n;
        if (xs[n] == xx && ys[n] == yy)
            return TRUE;
    }
    return FALSE;
}

void
selection_floodfill(
    struct selectionvar *ov,
    coordxy x, coordxy y,
    boolean diagonals)
{
    struct selectionvar *tmp = selection_new();
#define SEL_FLOOD_STACK (COLNO * ROWNO)
#define SEL_FLOOD(nx, ny) \
    do {                                      \
        if (idx < SEL_FLOOD_STACK) {          \
            dx[idx] = (nx);                   \
            dy[idx] = (ny);                   \
            idx++;                            \
        } else                                \
            panic(floodfill_stack_overrun);   \
    } while (0)
#define SEL_FLOOD_CHKDIR(mx, my, sel) \
    do {                                                        \
        if (isok((mx), (my))                                    \
            && (*selection_flood_check_func)((mx), (my))        \
            && !selection_getpoint((mx), (my), (sel))           \
            && !sel_flood_havepoint((mx), (my), dx, dy, idx))   \
            SEL_FLOOD((mx), (my));                              \
    } while (0)
    static const char floodfill_stack_overrun[] = "floodfill stack overrun";
    int idx = 0;
    coordxy dx[SEL_FLOOD_STACK];
    coordxy dy[SEL_FLOOD_STACK];

    if (selection_flood_check_func == (int (*)(coordxy, coordxy)) 0) {
        selection_free(tmp, TRUE);
        return;
    }
    SEL_FLOOD(x, y);
    do {
        idx--;
        x = dx[idx];
        y = dy[idx];
        if (isok(x, y)) {
            selection_setpoint(x, y, ov, 1);
            selection_setpoint(x, y, tmp, 1);
        }
        SEL_FLOOD_CHKDIR((x + 1), y, tmp);
        SEL_FLOOD_CHKDIR((x - 1), y, tmp);
        SEL_FLOOD_CHKDIR(x, (y + 1), tmp);
        SEL_FLOOD_CHKDIR(x, (y - 1), tmp);
        if (diagonals) {
            SEL_FLOOD_CHKDIR((x + 1), (y + 1), tmp);
            SEL_FLOOD_CHKDIR((x - 1), (y - 1), tmp);
            SEL_FLOOD_CHKDIR((x - 1), (y + 1), tmp);
            SEL_FLOOD_CHKDIR((x + 1), (y - 1), tmp);
        }
    } while (idx > 0);
#undef SEL_FLOOD
#undef SEL_FLOOD_STACK
#undef SEL_FLOOD_CHKDIR
    selection_free(tmp, TRUE);
}

/* McIlroy's Ellipse Algorithm */
void
selection_do_ellipse(
    struct selectionvar *ov,
    int xc, int yc,
    int a, int b,
    int filled)
{ /* e(x,y) = b^2*x^2 + a^2*y^2 - a^2*b^2 */
    int x = 0, y = b;
    long a2 = (long) a * a, b2 = (long) b * b;
    long crit1 = -(a2 / 4 + a % 2 + b2);
    long crit2 = -(b2 / 4 + b % 2 + a2);
    long crit3 = -(b2 / 4 + b % 2);
    long t = -a2 * y; /* e(x+1/2,y-1/2) - (a^2+b^2)/4 */
    long dxt = 2 * b2 * x, dyt = -2 * a2 * y;
    long d2xt = 2 * b2, d2yt = 2 * a2;
    long width = 1;
    long i;

    if (!ov)
        return;

    filled = !filled;

    if (!filled) {
        while (y >= 0 && x <= a) {
            selection_setpoint(xc + x, yc + y, ov, 1);
            if (x != 0 || y != 0)
                selection_setpoint(xc - x, yc - y, ov, 1);
            if (x != 0 && y != 0) {
                selection_setpoint(xc + x, yc - y, ov, 1);
                selection_setpoint(xc - x, yc + y, ov, 1);
            }
            if (t + b2 * x <= crit1       /* e(x+1,y-1/2) <= 0 */
                || t + a2 * y <= crit3) { /* e(x+1/2,y) <= 0 */
                x++;
                dxt += d2xt;
                t += dxt;
            } else if (t - a2 * y > crit2) { /* e(x+1/2,y-1) > 0 */
                y--;
                dyt += d2yt;
                t += dyt;
            } else {
                x++;
                dxt += d2xt;
                t += dxt;
                y--;
                dyt += d2yt;
                t += dyt;
            }
        }
    } else {
        while (y >= 0 && x <= a) {
            if (t + b2 * x <= crit1       /* e(x+1,y-1/2) <= 0 */
                || t + a2 * y <= crit3) { /* e(x+1/2,y) <= 0 */
                x++;
                dxt += d2xt;
                t += dxt;
                width += 2;
            } else if (t - a2 * y > crit2) { /* e(x+1/2,y-1) > 0 */
                for (i = 0; i < width; i++)
                    selection_setpoint(xc - x + i, yc - y, ov, 1);
                if (y != 0)
                    for (i = 0; i < width; i++)
                        selection_setpoint(xc - x + i, yc + y, ov, 1);
                y--;
                dyt += d2yt;
                t += dyt;
            } else {
                for (i = 0; i < width; i++)
                    selection_setpoint(xc - x + i, yc - y, ov, 1);
                if (y != 0)
                    for (i = 0; i < width; i++)
                        selection_setpoint(xc - x + i, yc + y, ov, 1);
                x++;
                dxt += d2xt;
                t += dxt;
                y--;
                dyt += d2yt;
                t += dyt;
                width += 2;
            }
        }
    }
}

/* square of distance from line segment (x1,y1, x2,y2) to point (x3,y3) */
staticfn long
line_dist_coord(long x1, long y1, long x2, long y2, long x3, long y3)
{
    long px = x2 - x1;
    long py = y2 - y1;
    long s = px * px + py * py;
    long x, y, dx, dy, distsq = 0;
    float lu = 0;

    if (x1 == x2 && y1 == y2)
        return dist2(x1, y1, x3, y3);

    lu = ((x3 - x1) * px + (y3 - y1) * py) / (float) s;
    if (lu > 1)
        lu = 1;
    else if (lu < 0)
        lu = 0;

    x = x1 + lu * px;
    y = y1 + lu * py;
    dx = x - x3;
    dy = y - y3;
    distsq = dx * dx + dy * dy;

    return distsq;
}

/* guts of l_selection_gradient */
void
selection_do_gradient(
    struct selectionvar *ov,
    long x, long y,
    long x2,long y2,
    long gtyp,
    long mind, long maxd)
{
    long dx, dy, dofs;

    if (mind > maxd) {
        long tmp = mind;
        mind = maxd;
        maxd = tmp;
    }

    dofs = maxd * maxd - mind * mind;
    if (dofs < 1)
        dofs = 1;

    switch (gtyp) {
    default:
        impossible("Unrecognized gradient type! Defaulting to radial...");
        /* FALLTHRU */
    case SEL_GRADIENT_RADIAL: {
        for (dx = 0; dx < COLNO; dx++)
            for (dy = 0; dy < ROWNO; dy++) {
                long d0 = line_dist_coord(x, y, x2, y2, dx, dy);

                if (d0 <= mind * mind
                    || (d0 <= maxd * maxd && d0 - mind * mind < rn2(dofs)))
                    selection_setpoint(dx, dy, ov, 1);
            }
        break;
    }
    case SEL_GRADIENT_SQUARE: {
        for (dx = 0; dx < COLNO; dx++)
            for (dy = 0; dy < ROWNO; dy++) {
                long d1 = line_dist_coord(x, y, x2, y2, x, dy);
                long d2 = line_dist_coord(x, y, x2, y2, dx, y);
                long d3 = line_dist_coord(x, y, x2, y2, x2, dy);
                long d4 = line_dist_coord(x, y, x2, y2, dx, y2);
                long d5 = line_dist_coord(x, y, x2, y2, dx, dy);
                long d0 = min(d5, min(max(d1, d2), max(d3, d4)));

                if (d0 <= mind * mind
                    || (d0 <= maxd * maxd && d0 - mind * mind < rn2(dofs)))
                    selection_setpoint(dx, dy, ov, 1);
            }
        break;
    } /*case*/
    } /*switch*/
}

/* bresenham line algo */
void
selection_do_line(
    coordxy x1, coordxy y1,
    coordxy x2, coordxy y2,
    struct selectionvar *ov)
{
    int d0, dx, dy, ai, bi, xi, yi;

    if (x1 < x2) {
        xi = 1;
        dx = x2 - x1;
    } else {
        xi = -1;
        dx = x1 - x2;
    }
    if (y1 < y2) {
        yi = 1;
        dy = y2 - y1;
    } else {
        yi = -1;
        dy = y1 - y2;
    }

    selection_setpoint(x1, y1, ov, 1);

    if (!dx && !dy) {
        /* single point - already all done */
        ;
    } else if (dx > dy) {
        ai = (dy - dx) * 2;
        bi = dy * 2;
        d0 = bi - dx;
        do {
            if (d0 >= 0) {
                y1 += yi;
                d0 += ai;
            } else
                d0 += bi;
            x1 += xi;
            selection_setpoint(x1, y1, ov, 1);
        } while (x1 != x2);
    } else {
        ai = (dx - dy) * 2;
        bi = dx * 2;
        d0 = bi - dy;
        do {
            if (d0 >= 0) {
                x1 += xi;
                d0 += ai;
            } else
                d0 += bi;
            y1 += yi;
            selection_setpoint(x1, y1, ov, 1);
        } while (y1 != y2);
    }
}

void
selection_do_randline(
    coordxy x1, coordxy y1,
    coordxy x2, coordxy y2,
    schar rough,
    schar rec,
    struct selectionvar *ov)
{
    int mx, my;
    int dx, dy;

    if (rec < 1 || (x2 == x1 && y2 == y1))
        return;

    if (rough > max(abs(x2 - x1), abs(y2 - y1)))
        rough = max(abs(x2 - x1), abs(y2 - y1));

    if (rough < 2) {
        mx = ((x1 + x2) / 2);
        my = ((y1 + y2) / 2);
    } else {
        do {
            dx = rn2(rough) - (rough / 2);
            dy = rn2(rough) - (rough / 2);
            mx = ((x1 + x2) / 2) + dx;
            my = ((y1 + y2) / 2) + dy;
        } while ((mx > COLNO - 1 || mx < 0 || my < 0 || my > ROWNO - 1));
    }

    if (!selection_getpoint(mx, my, ov)) {
        selection_setpoint(mx, my, ov, 1);
    }

    rough = (rough * 2) / 3;

    rec--;

    selection_do_randline(x1, y1, mx, my, rough, rec, ov);
    selection_do_randline(mx, my, x2, y2, rough, rec, ov);

    selection_setpoint(x2, y2, ov, 1);
}

void
selection_iterate(
    struct selectionvar *ov,
    select_iter_func func,
    genericptr_t arg)
{
    coordxy x, y;
    NhRect rect = cg.zeroNhRect;

    if (!ov)
        return;

    selection_getbounds(ov, &rect);

    for (x = rect.lx; x <= rect.hx; x++)
        for (y = rect.ly; y <= rect.hy; y++)
            if (isok(x,y) && selection_getpoint(x, y, ov))
                (*func)(x, y, arg);
}

/* selection is not rectangular, or has holes in it */
boolean
selection_is_irregular(struct selectionvar *sel)
{
    coordxy x, y;
    NhRect rect = cg.zeroNhRect;

    selection_getbounds(sel, &rect);

    for (x = rect.lx; x <= rect.hx; x++)
        for (y = rect.ly; y <= rect.hy; y++)
            if (isok(x,y) && !selection_getpoint(x, y, sel))
                return TRUE;

    return FALSE;
}

/* return a description of the selection size */
char *
selection_size_description(struct selectionvar *sel, char *buf)
{
    NhRect rect = cg.zeroNhRect;
    coordxy dx, dy;

    selection_getbounds(sel, &rect);
    dx = rect.hx - rect.lx + 1;
    dy = rect.hy - rect.ly + 1;
    Sprintf(buf, "%s %i by %i", selection_is_irregular(sel) ? "irregularly shaped"
            : (dx == dy) ? "square"
            : "rectangular",
            dx, dy);
    return buf;
}

struct selectionvar *
selection_from_mkroom(struct mkroom *croom)
{
    struct selectionvar *sel = selection_new();
    coordxy x, y;
    unsigned rmno;

    if (!croom && gc.coder && gc.coder->croom)
        croom = gc.coder->croom;
    if (!croom)
        return sel;

    rmno = (unsigned)((croom - svr.rooms) + ROOMOFFSET);
    for (y = croom->ly; y <= croom->hy; y++)
        for (x = croom->lx; x <= croom->hx; x++)
            if (isok(x, y) && !levl[x][y].edge
                && levl[x][y].roomno == rmno)
                selection_setpoint(x, y, sel, 1);
    return sel;
}

void
selection_force_newsyms(struct selectionvar *sel)
{
    coordxy x, y;

    for (x = 1; x < sel->wid; x++)
        for (y = 0; y < sel->hei; y++)
            if (selection_getpoint(x, y, sel))
                newsym_force(x, y);
}

/*selvar.c*/
