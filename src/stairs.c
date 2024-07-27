/* NetHack 3.7	stairs.c	$NHDT-Date: 1704043695 2023/12/31 17:28:15 $  $NHDT-Branch: keni-luabits2 $:$NHDT-Revision: 1.207 $ */
/* Copyright (c) 2024 by Pasi Kallinen */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

void
stairway_add(
    coordxy x, coordxy y,
    boolean up, boolean isladder,
    d_level *dest)
{
    stairway *tmp = (stairway *) alloc(sizeof (stairway));

    (void) memset((genericptr_t) tmp, 0, sizeof (stairway));
    tmp->sx = x;
    tmp->sy = y;
    tmp->up = up;
    tmp->isladder = isladder;
    tmp->u_traversed = FALSE;
    assign_level(&(tmp->tolev), dest);
    tmp->next = gs.stairs;
    gs.stairs = tmp;
}

void
stairway_free_all(void)
{
    stairway *tmp = gs.stairs;

    while (tmp) {
        stairway *tmp2 = tmp->next;
        free(tmp);
        tmp = tmp2;
    }
    gs.stairs = NULL;
}

stairway *
stairway_at(coordxy x, coordxy y)
{
    stairway *tmp = gs.stairs;

    while (tmp && !(tmp->sx == x && tmp->sy == y))
        tmp = tmp->next;
    return tmp;
}

stairway *
stairway_find(d_level *fromdlev)
{
    stairway *tmp = gs.stairs;

    while (tmp) {
        if (tmp->tolev.dnum == fromdlev->dnum
            && tmp->tolev.dlevel == fromdlev->dlevel)
            break; /* return */
        tmp = tmp->next;
    }
    return tmp;
}

stairway *
stairway_find_from(d_level *fromdlev, boolean isladder)
{
    stairway *tmp = gs.stairs;

    while (tmp) {
        if (tmp->tolev.dnum == fromdlev->dnum
            && tmp->tolev.dlevel == fromdlev->dlevel
            && tmp->isladder == isladder)
            break; /* return */
        tmp = tmp->next;
    }
    return tmp;
}

stairway *
stairway_find_dir(boolean up)
{
    stairway *tmp = gs.stairs;

    while (tmp && !(tmp->up == up))
        tmp = tmp->next;
    return tmp;
}

stairway *
stairway_find_type_dir(boolean isladder, boolean up)
{
    stairway *tmp = gs.stairs;

    while (tmp && !(tmp->isladder == isladder && tmp->up == up))
        tmp = tmp->next;
    return tmp;
}

stairway *
stairway_find_special_dir(boolean up)
{
    stairway *tmp = gs.stairs;

    while (tmp) {
        if (tmp->tolev.dnum != u.uz.dnum && tmp->up != up)
            return tmp;
        tmp = tmp->next;
    }
    return tmp;
}

/* place you on the special staircase */
void
u_on_sstairs(int upflag)
{
    stairway *stway = stairway_find_special_dir(upflag);

    if (stway)
        u_on_newpos(stway->sx, stway->sy);
    else
        u_on_rndspot(upflag);
}

/* place you on upstairs (or special equivalent) */
void
u_on_upstairs(void)
{
    stairway *stway = stairway_find_dir(TRUE);

    if (stway)
        u_on_newpos(stway->sx, stway->sy);
    else
        u_on_sstairs(0); /* destination upstairs implies moving down */
}

/* place you on dnstairs (or special equivalent) */
void
u_on_dnstairs(void)
{
    stairway *stway = stairway_find_dir(FALSE);

    if (stway)
        u_on_newpos(stway->sx, stway->sy);
    else
        u_on_sstairs(1); /* destination dnstairs implies moving up */
}

boolean
On_stairs(coordxy x, coordxy y)
{
    return (stairway_at(x, y) != NULL);
}

boolean
On_ladder(coordxy x, coordxy y)
{
    stairway *stway = stairway_at(x, y);

    return (boolean) (stway && stway->isladder);
}

boolean
On_stairs_up(coordxy x, coordxy y)
{
    stairway *stway = stairway_at(x, y);

    return (boolean) (stway && stway->up);
}

boolean
On_stairs_dn(coordxy x, coordxy y)
{
    stairway *stway = stairway_at(x, y);

    return (boolean) (stway && !stway->up);
}

/* return True if 'sway' is a branch staircase and hero has used these stairs
   to visit the branch */
boolean
known_branch_stairs(stairway *sway)
{
    return (sway && sway->tolev.dnum != u.uz.dnum && sway->u_traversed);
}

/* describe staircase 'sway' based on whether hero knows the destination */
char *
stairs_description(
    stairway *sway, /* stairs/ladder to describe */
    char *outbuf,   /* result buffer */
    boolean stcase) /* True: "staircase" or "ladder", always singular;
                     * False: "stairs" or "ladder"; caller needs to deal
                     * with singular vs plural when forming a sentence */
{
    d_level tolev;
    const char *stairs, *updown;

    tolev = sway->tolev;
    stairs = sway->isladder ? "ladder" : stcase ? "staircase" : "stairs";
    updown = sway->up ? "up" : "down";

    if (!known_branch_stairs(sway)) {
        /* ordinary stairs or branch stairs to not-yet-visited branch */
        Sprintf(outbuf, "%s %s", stairs, updown);
        if (sway->u_traversed) {
            boolean specialdepth = (tolev.dnum == quest_dnum
                                    || single_level_branch(&tolev)); /* knox */
            int to_dlev = specialdepth ? dunlev(&tolev) : depth(&tolev);

            Sprintf(eos(outbuf), " to level %d", to_dlev);
        }
    } else if (u.uz.dnum == 0 && u.uz.dlevel == 1 && sway->up) {
        /* stairs up from level one are a special case; they are marked
           as having been traversed because the hero obviously started
           the game by coming down them, but the remote side varies
           depending on whether the Amulet is being carried */
        Sprintf(outbuf, "%s%s %s %s",
                !u.uhave.amulet ? "" : "branch ",
                stairs, updown,
                !u.uhave.amulet ? "out of the dungeon"
                /* minimize our expectations about what comes next */
                : (on_level(&tolev, &earth_level)
                   || on_level(&tolev, &air_level)
                   || on_level(&tolev, &fire_level)
                   || on_level(&tolev, &water_level))
                  ? "to the Elemental Planes"
                  : "to the end game");
    } else {
        /* known branch stairs; tacking on destination level is too verbose */
        Sprintf(outbuf, "branch %s %s to %s",
                stairs, updown, svd.dungeons[tolev.dnum].dname);
        /* dungeons[].dname is capitalized; undo that for "The <Branch>" */
        (void) strsubst(outbuf, "The ", "the ");
    }
    return outbuf;
}

/*stairs.c*/
