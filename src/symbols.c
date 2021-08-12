/* NetHack 3.7	symbols.c	$NHDT-Date: 1596498214 2020/08/03 23:43:34 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.77 $ */
/* Copyright (c) NetHack Development Team 2020.                   */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "tcap.h"

extern const uchar def_r_oc_syms[MAXOCLASSES];      /* drawing.c */

#if defined(TERMLIB) || defined(CURSES_GRAPHICS)
void (*decgraphics_mode_callback)(void) = 0; /* set in tty_start_screen() */
#endif /* TERMLIB || CURSES */

#ifdef PC9800
void (*ibmgraphics_mode_callback)(void) = 0; /* set in tty_start_screen() */
void (*ascgraphics_mode_callback)(void) = 0; /* set in tty_start_screen() */
#endif

#ifdef CURSES_GRAPHICS
void (*cursesgraphics_mode_callback)(void) = 0;
#endif
/*
 * Explanations of the functions found below:
 *
 * init_symbols()
 *                     Sets the current display symbols, the
 *                     loadable symbols to the default NetHack
 *                     symbols, including the rogue_syms rogue level
 *                     symbols. This would typically be done
 *                     immediately after execution begins. Any
 *                     previously loaded external symbol sets are
 *                     discarded.
 *
 * switch_symbols(arg)
 *                     Called to swap in new current display symbols
 *                     (showsyms) from either the default symbols,
 *                     or from the loaded symbols.
 *
 *                     If (arg == 0) then showsyms are taken from
 *                     defsyms, def_oc_syms, and def_monsyms.
 *
 *                     If (arg != 0), which is the normal expected
 *                     usage, then showsyms are taken from the
 *                     adjustable display symbols found in g.primary_syms.
 *                     g.primary_syms may have been loaded from an external
 *                     symbol file by config file options or interactively
 *                     in the Options menu.
 *
 * assign_graphics(arg)
 *
 *                     This is used in the game core to toggle in and
 *                     out of other {rogue} level display modes.
 *
 *                     If arg is ROGUESET, this places the rogue level
 *                     symbols from g.rogue_syms into g.showsyms.
 *
 *                     If arg is PRIMARY, this places the symbols
 *                     from g.primary_syms into g.showsyms.
 *
 * update_primary_symset()
 *                     Update a member of the primary(primary_*) symbol set.
 *
 * update_rogue_symset()
 *                     Update a member of the rogue (rogue_*) symbol set.
 *
 * update_ov_primary_symset()
 *                     Update a member of the overrides for primary symbol set.
 *
 * update_ov_rogue_symset()
 *                     Update a member of the overrides for rogue symbol set.
 *
 */

void
init_symbols(void)
{
    init_ov_primary_symbols();
    init_ov_rogue_symbols();
    init_primary_symbols();
    init_showsyms();
    init_rogue_symbols();
}

void
init_showsyms(void)
{
    register int i;

    for (i = 0; i < MAXPCHARS; i++)
        g.showsyms[i + SYM_OFF_P] = defsyms[i].sym;
    for (i = 0; i < MAXOCLASSES; i++)
        g.showsyms[i + SYM_OFF_O] = def_oc_syms[i].sym;
    for (i = 0; i < MAXMCLASSES; i++)
        g.showsyms[i + SYM_OFF_M] = def_monsyms[i].sym;
    for (i = 0; i < WARNCOUNT; i++)
        g.showsyms[i + SYM_OFF_W] = def_warnsyms[i].sym;
    for (i = 0; i < MAXOTHER; i++)
        g.showsyms[i + SYM_OFF_X] = get_othersym(i, PRIMARY);
}

/* initialize defaults for the overrides to the rogue symset */
void
init_ov_rogue_symbols(void)
{
    register int i;

    for (i = 0; i < SYM_MAX; i++)
        g.ov_rogue_syms[i] = (nhsym) 0;
}
/* initialize defaults for the overrides to the primary symset */
void
init_ov_primary_symbols(void)
{
    register int i;

    for (i = 0; i < SYM_MAX; i++)
        g.ov_primary_syms[i] = (nhsym) 0;
}

nhsym
get_othersym(int idx, int which_set)
{
    nhsym sym = (nhsym) 0;
    int oidx = idx + SYM_OFF_X;

    if (which_set == ROGUESET)
        sym = g.ov_rogue_syms[oidx] ? g.ov_rogue_syms[oidx]
                                  : g.rogue_syms[oidx];
    else
        sym = g.ov_primary_syms[oidx] ? g.ov_primary_syms[oidx]
                                  : g.primary_syms[oidx];
    if (!sym) {
        switch(idx) {
            case SYM_NOTHING:
            case SYM_UNEXPLORED:
                sym = DEF_NOTHING;
                break;
            case SYM_BOULDER:
                sym = def_oc_syms[ROCK_CLASS].sym;
                break;
            case SYM_INVISIBLE:
                sym = DEF_INVISIBLE;
                break;
#if 0
            /* these intentionally have no defaults */
            case SYM_PET_OVERRIDE:
            case SYM_HERO_OVERRIDE:
                break;
#endif
        }
    }
    return sym;
}

/* initialize defaults for the primary symset */
void
init_primary_symbols(void)
{
    register int i;

    for (i = 0; i < MAXPCHARS; i++)
        g.primary_syms[i + SYM_OFF_P] = defsyms[i].sym;
    for (i = 0; i < MAXOCLASSES; i++)
        g.primary_syms[i + SYM_OFF_O] = def_oc_syms[i].sym;
    for (i = 0; i < MAXMCLASSES; i++)
        g.primary_syms[i + SYM_OFF_M] = def_monsyms[i].sym;
    for (i = 0; i < WARNCOUNT; i++)
        g.primary_syms[i + SYM_OFF_W] = def_warnsyms[i].sym;
    for (i = 0; i < MAXOTHER; i++)
        g.primary_syms[i + SYM_OFF_X] = get_othersym(i, PRIMARY);

    clear_symsetentry(PRIMARY, FALSE);
}

/* initialize defaults for the rogue symset */
void
init_rogue_symbols(void)
{
    register int i;

    /* These are defaults that can get overwritten
       later by the roguesymbols option */

    for (i = 0; i < MAXPCHARS; i++)
        g.rogue_syms[i + SYM_OFF_P] = defsyms[i].sym;
    g.rogue_syms[S_vodoor] = g.rogue_syms[S_hodoor] = g.rogue_syms[S_ndoor] = '+';
    g.rogue_syms[S_upstair] = g.rogue_syms[S_dnstair] = '%';

    for (i = 0; i < MAXOCLASSES; i++)
        g.rogue_syms[i + SYM_OFF_O] = def_r_oc_syms[i];
    for (i = 0; i < MAXMCLASSES; i++)
        g.rogue_syms[i + SYM_OFF_M] = def_monsyms[i].sym;
    for (i = 0; i < WARNCOUNT; i++)
        g.rogue_syms[i + SYM_OFF_W] = def_warnsyms[i].sym;
    for (i = 0; i < MAXOTHER; i++)
        g.rogue_syms[i + SYM_OFF_X] = get_othersym(i, ROGUESET);

    clear_symsetentry(ROGUESET, FALSE);
    /* default on Rogue level is no color
     * but some symbol sets can override that
     */
    g.symset[ROGUESET].nocolor = 1;
}

void
assign_graphics(int whichset)
{
    register int i;

    switch (whichset) {
    case ROGUESET:
        /* Adjust graphics display characters on Rogue levels */

        for (i = 0; i < SYM_MAX; i++)
            g.showsyms[i] = g.ov_rogue_syms[i] ? g.ov_rogue_syms[i]
                                           : g.rogue_syms[i];

#if defined(MSDOS) && defined(USE_TILES)
        if (iflags.grmode)
            tileview(FALSE);
#endif
        g.currentgraphics = ROGUESET;
        break;

    case PRIMARY:
    default:
        for (i = 0; i < SYM_MAX; i++)
            g.showsyms[i] = g.ov_primary_syms[i] ? g.ov_primary_syms[i]
                                             : g.primary_syms[i];

#if defined(MSDOS) && defined(USE_TILES)
        if (iflags.grmode)
            tileview(TRUE);
#endif
        g.currentgraphics = PRIMARY;
        break;
    }
}

void
switch_symbols(int nondefault)
{
    register int i;

    if (nondefault) {
        for (i = 0; i < SYM_MAX; i++)
            g.showsyms[i] = g.ov_primary_syms[i] ? g.ov_primary_syms[i]
                                             : g.primary_syms[i];
#ifdef PC9800
        if (SYMHANDLING(H_IBM) && ibmgraphics_mode_callback)
            (*ibmgraphics_mode_callback)();
        else if (SYMHANDLING(H_UNK) && ascgraphics_mode_callback)
            (*ascgraphics_mode_callback)();
#endif
#if defined(TERMLIB) || defined(CURSES_GRAPHICS)
        /* curses doesn't assign any routine to dec..._callback but
           probably does the expected initialization under the hood
           for terminals capable of rendering DECgraphics */
        if (SYMHANDLING(H_DEC) && decgraphics_mode_callback)
            (*decgraphics_mode_callback)();
# ifdef CURSES_GRAPHICS
        /* there aren't any symbol sets with CURS handling, and the
           curses interface never assigns a routine to curses..._callback */
        if (SYMHANDLING(H_CURS) && cursesgraphics_mode_callback)
            (*cursesgraphics_mode_callback)();
# endif
#endif
    } else {
        init_primary_symbols();
        init_showsyms();
    }
}

void
update_ov_primary_symset(struct symparse* symp, int val)
{
    g.ov_primary_syms[symp->idx] = val;
}

void
update_ov_rogue_symset(struct symparse* symp, int val)
{
    g.ov_rogue_syms[symp->idx] = val;
}

void
update_primary_symset(struct symparse* symp, int val)
{
    g.primary_syms[symp->idx] = val;
}

void
update_rogue_symset(struct symparse* symp, int val)
{
    g.rogue_syms[symp->idx] = val;
}

void
clear_symsetentry(int which_set, boolean name_too)
{
    if (g.symset[which_set].desc)
        free((genericptr_t) g.symset[which_set].desc);
    g.symset[which_set].desc = (char *) 0;

    g.symset[which_set].handling = H_UNK;
    g.symset[which_set].nocolor = 0;
    /* initialize restriction bits */
    g.symset[which_set].primary = 0;
    g.symset[which_set].rogue = 0;

    if (name_too) {
        if (g.symset[which_set].name)
            free((genericptr_t) g.symset[which_set].name);
        g.symset[which_set].name = (char *) 0;
    }
}

/*
 * If you are adding code somewhere to be able to recognize
 * particular types of symset "handling", define a
 * H_XXX macro in include/sym.h and add the name
 * to this array at the matching offset.
 */
const char *known_handling[] = {
    "UNKNOWN", /* H_UNK  */
    "IBM",     /* H_IBM  */
    "DEC",     /* H_DEC  */
    "CURS",    /* H_CURS */
    "MAC",     /* H_MAC  -- pre-OSX MACgraphics */
    (const char *) 0,
};

/*
 * Accepted keywords for symset restrictions.
 * These can be virtually anything that you want to
 * be able to test in the code someplace.
 * Be sure to:
 *    - add a corresponding Bitfield to the symsetentry struct in sym.h
 *    - initialize the field to zero in parse_sym_line in the SYM_CONTROL
 *      case 0 section of the idx switch. The location is prefaced with
 *      with a comment stating "initialize restriction bits".
 *    - set the value appropriately based on the index of your keyword
 *      under the case 5 sections of the same SYM_CONTROL idx switches.
 *    - add the field to clear_symsetentry()
 */
const char *known_restrictions[] = {
    "primary", "rogue", (const char *) 0,
};

const struct symparse loadsyms[] = {
    { SYM_CONTROL, 0, "start" },
    { SYM_CONTROL, 0, "begin" },
    { SYM_CONTROL, 1, "finish" },
    { SYM_CONTROL, 2, "handling" },
    { SYM_CONTROL, 3, "description" },
    { SYM_CONTROL, 4, "color" },
    { SYM_CONTROL, 4, "colour" },
    { SYM_CONTROL, 5, "restrictions" },
#define PCHAR_PARSE
#include "defsym.h"
#undef PCHAR_PARSE
#define OBJCLASS_PARSE
#include "defsym.h"
#undef OBJCLASS_PARSE
#define MONSYMS_PARSE
#include "defsym.h"
#undef MONSYMS_PARSE
    { SYM_OTH, SYM_NOTHING + SYM_OFF_X, "S_nothing" },
    { SYM_OTH, SYM_UNEXPLORED + SYM_OFF_X, "S_unexplored" },
    { SYM_OTH, SYM_BOULDER + SYM_OFF_X, "S_boulder" },
    { SYM_OTH, SYM_INVISIBLE + SYM_OFF_X, "S_invisible" },
    { SYM_OTH, SYM_PET_OVERRIDE + SYM_OFF_X, "S_pet_override" },
    { SYM_OTH, SYM_HERO_OVERRIDE + SYM_OFF_X, "S_hero_override" },
    { 0, 0, (const char *) 0 } /* fence post */
};

/*symbols.c*/
