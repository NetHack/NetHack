/* NetHack 3.7	symbols.c	$NHDT-Date: 1661295669 2022/08/23 23:01:09 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.90 $ */
/* Copyright (c) NetHack Development Team 2020.                   */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "tcap.h"

static void savedsym_add(const char *, const char *, int);
static struct _savedsym *savedsym_find(const char *, int);
#ifdef ENHANCED_SYMBOLS
static void purge_custom_entries(enum graphics_sets which_set);
#endif

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
#if defined(TTY_GRAPHICS) && defined(WIN32)
void (*ibmgraphics_mode_callback)(void) = 0;
#endif
#ifdef ENHANCED_SYMBOLS
void (*utf8graphics_mode_callback)(void) = 0; /* set in tty_start_screen and
                                               * found in unixtty,windtty,&c */
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
 *                     adjustable display symbols found in gp.primary_syms.
 *                     gp.primary_syms may have been loaded from an external
 *                     symbol file by config file options or interactively
 *                     in the Options menu.
 *
 * assign_graphics(arg)
 *
 *                     This is used in the game core to toggle in and
 *                     out of other {rogue} level display modes.
 *
 *                     If arg is ROGUESET, this places the rogue level
 *                     symbols from gr.rogue_syms into gs.showsyms.
 *
 *                     If arg is PRIMARYSET, this places the symbols
 *                     from gp.primary_syms into gs.showsyms.
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
        gs.showsyms[i + SYM_OFF_P] = defsyms[i].sym;
    for (i = 0; i < MAXOCLASSES; i++)
        gs.showsyms[i + SYM_OFF_O] = def_oc_syms[i].sym;
    for (i = 0; i < MAXMCLASSES; i++)
        gs.showsyms[i + SYM_OFF_M] = def_monsyms[i].sym;
    for (i = 0; i < WARNCOUNT; i++)
        gs.showsyms[i + SYM_OFF_W] = def_warnsyms[i].sym;
    for (i = 0; i < MAXOTHER; i++)
        gs.showsyms[i + SYM_OFF_X] = get_othersym(i, PRIMARYSET);
}

/* initialize defaults for the overrides to the rogue symset */
void
init_ov_rogue_symbols(void)
{
    register int i;

    for (i = 0; i < SYM_MAX; i++)
        go.ov_rogue_syms[i] = (nhsym) 0;
}
/* initialize defaults for the overrides to the primary symset */
void
init_ov_primary_symbols(void)
{
    register int i;

    for (i = 0; i < SYM_MAX; i++)
        go.ov_primary_syms[i] = (nhsym) 0;
}

nhsym
get_othersym(int idx, int which_set)
{
    nhsym sym = (nhsym) 0;
    int oidx = idx + SYM_OFF_X;

    if (which_set == ROGUESET)
        sym = go.ov_rogue_syms[oidx] ? go.ov_rogue_syms[oidx]
                                    : gr.rogue_syms[oidx];
    else
        sym = go.ov_primary_syms[oidx] ? go.ov_primary_syms[oidx]
                                      : gp.primary_syms[oidx];
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
        gp.primary_syms[i + SYM_OFF_P] = defsyms[i].sym;
    for (i = 0; i < MAXOCLASSES; i++)
        gp.primary_syms[i + SYM_OFF_O] = def_oc_syms[i].sym;
    for (i = 0; i < MAXMCLASSES; i++)
        gp.primary_syms[i + SYM_OFF_M] = def_monsyms[i].sym;
    for (i = 0; i < WARNCOUNT; i++)
        gp.primary_syms[i + SYM_OFF_W] = def_warnsyms[i].sym;
    for (i = 0; i < MAXOTHER; i++)
        gp.primary_syms[i + SYM_OFF_X] = get_othersym(i, PRIMARYSET);

    clear_symsetentry(PRIMARYSET, FALSE);
}

/* initialize defaults for the rogue symset */
void
init_rogue_symbols(void)
{
    register int i;

    /* These are defaults that can get overwritten
       later by the roguesymbols option */

    for (i = 0; i < MAXPCHARS; i++)
        gr.rogue_syms[i + SYM_OFF_P] = defsyms[i].sym;
    gr.rogue_syms[S_vodoor] = gr.rogue_syms[S_hodoor]
        = gr.rogue_syms[S_ndoor] = '+';
    gr.rogue_syms[S_upstair] = gr.rogue_syms[S_dnstair] = '%';

    for (i = 0; i < MAXOCLASSES; i++)
        gr.rogue_syms[i + SYM_OFF_O] = def_r_oc_syms[i];
    for (i = 0; i < MAXMCLASSES; i++)
        gr.rogue_syms[i + SYM_OFF_M] = def_monsyms[i].sym;
    for (i = 0; i < WARNCOUNT; i++)
        gr.rogue_syms[i + SYM_OFF_W] = def_warnsyms[i].sym;
    for (i = 0; i < MAXOTHER; i++)
        gr.rogue_syms[i + SYM_OFF_X] = get_othersym(i, ROGUESET);

    clear_symsetentry(ROGUESET, FALSE);
    /* default on Rogue level is no color
     * but some symbol sets can override that
     */
    gs.symset[ROGUESET].nocolor = 1;
}

void
assign_graphics(int whichset)
{
    register int i;

    switch (whichset) {
    case ROGUESET:
        /* Adjust graphics display characters on Rogue levels */

        for (i = 0; i < SYM_MAX; i++)
            gs.showsyms[i] = go.ov_rogue_syms[i] ? go.ov_rogue_syms[i]
                                           : gr.rogue_syms[i];

#if defined(MSDOS) && defined(TILES_IN_GLYPHMAP)
        if (iflags.grmode)
            tileview(FALSE);
#endif
        gc.currentgraphics = ROGUESET;
        break;

    case PRIMARYSET:
    default:
        for (i = 0; i < SYM_MAX; i++)
            gs.showsyms[i] = go.ov_primary_syms[i] ? go.ov_primary_syms[i]
                                             : gp.primary_syms[i];

#if defined(MSDOS) && defined(TILES_IN_GLYPHMAP)
        if (iflags.grmode)
            tileview(TRUE);
#endif
        gc.currentgraphics = PRIMARYSET;
        break;
    }
    reset_glyphmap(gm_symchange);
}

void
switch_symbols(int nondefault)
{
    register int i;

    if (nondefault) {
        for (i = 0; i < SYM_MAX; i++)
            gs.showsyms[i] = go.ov_primary_syms[i] ? go.ov_primary_syms[i]
                                                 : gp.primary_syms[i];
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
#if defined(TTY_GRAPHICS) && defined(WIN32)
        if (SYMHANDLING(H_IBM) && ibmgraphics_mode_callback)
            (*ibmgraphics_mode_callback)();
#endif
#ifdef ENHANCED_SYMBOLS
        if (SYMHANDLING(H_UTF8) && utf8graphics_mode_callback)
            (*utf8graphics_mode_callback)();
#endif
    } else {
        init_primary_symbols();
        init_showsyms();
    }
}

void
update_ov_primary_symset(const struct symparse* symp, int val)
{
    go.ov_primary_syms[symp->idx] = val;
}

void
update_ov_rogue_symset(const struct symparse* symp, int val)
{
    go.ov_rogue_syms[symp->idx] = val;
}

void
update_primary_symset(const struct symparse* symp, int val)
{
    gp.primary_syms[symp->idx] = val;
}

void
update_rogue_symset(const struct symparse* symp, int val)
{
    gr.rogue_syms[symp->idx] = val;
}

void
clear_symsetentry(int which_set, boolean name_too)
{
    if (gs.symset[which_set].desc)
        free((genericptr_t) gs.symset[which_set].desc);
    gs.symset[which_set].desc = (char *) 0;

    gs.symset[which_set].handling = H_UNK;
    gs.symset[which_set].nocolor = 0;
    /* initialize restriction bits */
    gs.symset[which_set].primary = 0;
    gs.symset[which_set].rogue = 0;

    if (name_too) {
        if (gs.symset[which_set].name)
            free((genericptr_t) gs.symset[which_set].name);
        gs.symset[which_set].name = (char *) 0;
    }
#ifdef ENHANCED_SYMBOLS
    free_all_glyphmap_u();
    purge_custom_entries(which_set);
#endif
}

/* called from windmain.c */
boolean
symset_is_compatible(
    enum symset_handling_types handling,
    unsigned long wincap2)
{
#ifdef ENHANCED_SYMBOLS
#define WC2_utf8_bits (WC2_U_UTF8STR | WC2_U_24BITCOLOR)
    if (handling == H_UTF8 && ((wincap2 & WC2_utf8_bits) != WC2_utf8_bits))
        return FALSE;
#undef WC2_bits
#else
    nhUse(handling);
    nhUse(wincap2);
#endif
    return TRUE;
}

/*
 * If you are adding code somewhere to be able to recognize
 * particular types of symset "handling", define a
 * H_XXX macro in include/sym.h and add the name
 * to this array at the matching offset.
 */
const char *const known_handling[] = {
    "UNKNOWN", /* H_UNK  */
    "IBM",     /* H_IBM  */
    "DEC",     /* H_DEC  */
    "CURS",    /* H_CURS */
    "MAC",     /* H_MAC  -- pre-OSX MACgraphics */
    "UTF8",    /* H_UTF8 */
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
const char *const known_restrictions[] = {
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

boolean
proc_symset_line(char *buf)
{
    return !((boolean) parse_sym_line(buf, gs.symset_which_set));
}

/* returns 0 on error */
int
parse_sym_line(char *buf, int which_set)
{
    int val, i;
    const struct symparse *symp = (struct symparse *) 0;
    char *bufp, *commentp, *altp;
    int glyph = NO_GLYPH;
    boolean enhanced_unavailable = FALSE, is_glyph = FALSE;

    if (strlen(buf) >= BUFSZ)
        buf[BUFSZ - 1] = '\0';
    /* convert each instance of whitespace (tabs, consecutive spaces)
       into a single space; leading and trailing spaces are stripped */
    mungspaces(buf);

    /* remove trailing comment, if any (this isn't strictly needed for
       individual symbols, and it won't matter if "X#comment" without
       separating space slips through; for handling or set description,
       symbol set creator is responsible for preceding '#' with a space
       and that comment itself doesn't contain " #") */
    if ((commentp = strrchr(buf, '#')) != 0 && commentp[-1] == ' ')
        commentp[-1] = '\0';

    /* find the '=' or ':' */
    bufp = strchr(buf, '=');
    altp = strchr(buf, ':');
    if (!bufp || (altp && altp < bufp))
        bufp = altp;
    if (!bufp) {
        if (strncmpi(buf, "finish", 6) == 0) {
            /* end current graphics set */
            if (gc.chosen_symset_start)
                gc.chosen_symset_end = TRUE;
            gc.chosen_symset_start = FALSE;
            return 1;
        }
        config_error_add("No \"finish\"");
        return 0;
    }
    /* skip '=' and space which follows, if any */
    ++bufp;
    if (*bufp == ' ')
        ++bufp;


    symp = match_sym(buf);
    if (!symp && buf[0] == 'G' && buf[1] == '_') {
#ifdef ENHANCED_SYMBOLS
        if (gc.chosen_symset_start) {
            is_glyph = match_glyph(buf);
        } else {
            is_glyph = TRUE; /* report error only once */
        }
#else
        enhanced_unavailable = TRUE;
#endif
    }
    if (!symp && !is_glyph && !enhanced_unavailable) {
        config_error_add("Unknown sym keyword");
        return 0;
    }
    if (symp) {
        if (!gs.symset[which_set].name) {
            /* A null symset name indicates that we're just
               building a pick-list of possible symset
               values from the file, so only do that */
            if (symp->range == SYM_CONTROL) {
                struct symsetentry *tmpsp, *lastsp;

                for (lastsp = gs.symset_list; lastsp; lastsp = lastsp->next)
                    if (!lastsp->next)
                        break;
                switch (symp->idx) {
                case 0:
                    tmpsp = (struct symsetentry *) alloc(sizeof *tmpsp);
                    tmpsp->next = (struct symsetentry *) 0;
                    if (!lastsp)
                        gs.symset_list = tmpsp;
                    else
                        lastsp->next = tmpsp;
                    tmpsp->idx = gs.symset_count++;
                    tmpsp->name = dupstr(bufp);
                    tmpsp->desc = (char *) 0;
                    tmpsp->handling = H_UNK;
                    /* initialize restriction bits */
                    tmpsp->nocolor = 0;
                    tmpsp->primary = 0;
                    tmpsp->rogue = 0;
                    break;
                case 2:
                    /* handler type identified */
                    tmpsp = lastsp; /* most recent symset */
                    for (i = 0; known_handling[i]; ++i)
                        if (!strcmpi(known_handling[i], bufp)) {
                            tmpsp->handling = i;
                            break; /* for loop */
                        }
                    break;
                case 3:
                    /* description:something */
                    tmpsp = lastsp; /* most recent symset */
                    if (tmpsp && !tmpsp->desc)
                        tmpsp->desc = dupstr(bufp);
                    break;
                case 5:
                    /* restrictions: xxxx*/
                    tmpsp = lastsp; /* most recent symset */
                    for (i = 0; known_restrictions[i]; ++i) {
                        if (!strcmpi(known_restrictions[i], bufp)) {
                            switch (i) {
                            case 0:
                                tmpsp->primary = 1;
                                break;
                            case 1:
                                tmpsp->rogue = 1;
                                break;
                            }
                            break; /* while loop */
                        }
                    }
                    break;
                }
            }
            return 1;
        }
        if (symp->range && symp->range == SYM_CONTROL) {
            switch (symp->idx) {
            case 0:
                /* start of symset */
                if (!strcmpi(bufp, gs.symset[which_set].name)) {
                    /* matches desired one */
                    gc.chosen_symset_start = TRUE;
                    /* these init_*() functions clear symset fields too */
                    if (which_set == ROGUESET)
                        init_rogue_symbols();
                    else if (which_set == PRIMARYSET)
                        init_primary_symbols();
                }
                break;
            case 1:
                /* finish symset */
                if (gc.chosen_symset_start)
                    gc.chosen_symset_end = TRUE;
                gc.chosen_symset_start = FALSE;
                break;
            case 2:
                /* handler type identified */
                if (gc.chosen_symset_start)
                    set_symhandling(bufp, which_set);
                break;
            /* case 3: (description) is ignored here */
            case 4: /* color:off */
                if (gc.chosen_symset_start) {
                    if (bufp) {
                        if (!strcmpi(bufp, "true") || !strcmpi(bufp, "yes")
                            || !strcmpi(bufp, "on"))
                            gs.symset[which_set].nocolor = 0;
                        else if (!strcmpi(bufp, "false")
                                 || !strcmpi(bufp, "no")
                                 || !strcmpi(bufp, "off"))
                            gs.symset[which_set].nocolor = 1;
                    }
                }
                break;
            case 5: /* restrictions: xxxx*/
                if (gc.chosen_symset_start) {
                    int n = 0;

                    while (known_restrictions[n]) {
                        if (!strcmpi(known_restrictions[n], bufp)) {
                            switch (n) {
                            case 0:
                                gs.symset[which_set].primary = 1;
                                break;
                            case 1:
                                gs.symset[which_set].rogue = 1;
                                break;
                            }
                            break; /* while loop */
                        }
                        n++;
                    }
                }
                break;
            }
        } else {
            /* Not SYM_CONTROL */
            if (gs.symset[which_set].handling != H_UTF8) {
                val = sym_val(bufp);
                if (gc.chosen_symset_start) {
                    if (which_set == PRIMARYSET) {
                        update_primary_symset(symp, val);
                    } else if (which_set == ROGUESET) {
                        update_rogue_symset(symp, val);
                    }
                }
#ifdef ENHANCED_SYMBOLS
            } else {
                if (gc.chosen_symset_start) {
                    glyphrep_to_custom_map_entries(buf, &glyph);
                }
#endif
            }
        }
    }
#ifndef ENHANCED_SYMBOLS
    nhUse(glyph);
#endif
    return 1;
}

void
set_symhandling(char *handling, int which_set)
{
    int i = 0;

    gs.symset[which_set].handling = H_UNK;
    while (known_handling[i]) {
        if (!strcmpi(known_handling[i], handling)) {
            gs.symset[which_set].handling = i;
            return;
        }
        i++;
    }
}

/* bundle some common usage into one easy-to-use routine */
int
load_symset(const char *s, int which_set)
{
    clear_symsetentry(which_set, TRUE);

    if (gs.symset[which_set].name)
        free((genericptr_t) gs.symset[which_set].name);
    gs.symset[which_set].name = dupstr(s);

    if (read_sym_file(which_set)) {
        switch_symbols(TRUE);
    } else {
        clear_symsetentry(which_set, TRUE);
        return 0;
    }
    return 1;
}

void
free_symsets(void)
{
    clear_symsetentry(PRIMARYSET, TRUE);
    clear_symsetentry(ROGUESET, TRUE);

    /* symset_list is cleaned up as soon as it's used, so we shouldn't
       have to anything about it here */
    /* assert( symset_list == NULL ); */
}

struct _savedsym {
    char *name;
    char *val;
    int which_set;
    struct _savedsym *next;
};
struct _savedsym *saved_symbols = NULL;

void
savedsym_free(void)
{
    struct _savedsym *tmp = saved_symbols, *tmp2;

    while (tmp) {
        tmp2 = tmp->next;
        free(tmp->name);
        free(tmp->val);
        free(tmp);
        tmp = tmp2;
    }
}

static struct _savedsym *
savedsym_find(const char *name, int which_set)
{
    struct _savedsym *tmp = saved_symbols;

    while (tmp) {
        if (which_set == tmp->which_set && !strcmp(name, tmp->name))
            return tmp;
        tmp = tmp->next;
    }
    return NULL;
}

static void
savedsym_add(const char *name, const char *val, int which_set)
{
    struct _savedsym *tmp = NULL;

    if ((tmp = savedsym_find(name, which_set)) != 0) {
        free(tmp->val);
        tmp->val = dupstr(val);
    } else {
        tmp = (struct _savedsym *) alloc(sizeof *tmp);
        tmp->name = dupstr(name);
        tmp->val = dupstr(val);
        tmp->which_set = which_set;
        tmp->next = saved_symbols;
        saved_symbols = tmp;
    }
}

void
savedsym_strbuf(strbuf_t *sbuf)
{
    struct _savedsym *tmp = saved_symbols;
    char buf[BUFSZ];

    while (tmp) {
        Sprintf(buf, "%sSYMBOLS=%s:%s\n",
                (tmp->which_set == ROGUESET) ? "ROGUE" : "",
                tmp->name, tmp->val);
        strbuf_append(sbuf, buf);
        tmp = tmp->next;
    }
}

/* Parse the value of a SYMBOLS line from a config file */
boolean
parsesymbols(register char *opts, int which_set)
{
    int val;
    char *op, *symname, *strval;
    const struct symparse *symp;
    boolean is_glyph = FALSE;

    /*
     * FIXME:
     *  The parsing here (and next) yields incorrect results for
     *  "S_sample=','" or "S_sample=':'".
     */

    if ((op = strchr(opts, ',')) != 0) {
        *op++ = '\0';
        if (!parsesymbols(op, which_set))
            return FALSE;
    }

    /* S_sample:string */
    symname = opts;
    strval = strchr(opts, ':');
    if (!strval)
        strval = strchr(opts, '=');
    if (!strval)
        return FALSE;
    *strval++ = '\0';

    /* strip leading and trailing white space from symname and strval */
    mungspaces(symname);
    mungspaces(strval);

    symp = match_sym(symname);
#ifdef ENHANCED_SYMBOLS
    if (!symp && symname[0] == 'G' && symname[1] == '_') {
        is_glyph = match_glyph(symname);
    }
#endif
    if (!symp && !is_glyph)
        return FALSE;
    if (symp) {
        if (symp->range && symp->range != SYM_CONTROL) {
            if (gs.symset[which_set].handling == H_UTF8
                || (lowc(strval[0]) == 'u' && strval[1] == '+')) {
#ifdef ENHANCED_SYMBOLS
                char buf[BUFSZ];
                int glyph;

                Snprintf(buf, sizeof buf, "%s:%s", opts, strval);
                glyphrep_to_custom_map_entries(buf, &glyph);
#endif /* ENHANCED_SYMBOLS */
            } else {
                    val = sym_val(strval);
                    if (which_set == ROGUESET)
                        update_ov_rogue_symset(symp, val);
                    else
                        update_ov_primary_symset(symp, val);
            }
        }
    }
    savedsym_add(opts, strval, which_set);
    return TRUE;
}

const struct symparse *
match_sym(char *buf)
{
    int i;
    struct alternate_parse {
        const char *altnm;
        const char *nm;
    } alternates[] = {
        { "S_armour", "S_armor" },     { "S_explode1", "S_expl_tl" },
        { "S_explode2", "S_expl_tc" }, { "S_explode3", "S_expl_tr" },
        { "S_explode4", "S_expl_ml" }, { "S_explode5", "S_expl_mc" },
        { "S_explode6", "S_expl_mr" }, { "S_explode7", "S_expl_bl" },
        { "S_explode8", "S_expl_bc" }, { "S_explode9", "S_expl_br" },
    };
    size_t len = strlen(buf);
    const char *p = strchr(buf, ':'), *q = strchr(buf, '=');
    const struct symparse *sp = loadsyms;

    /* G_ lines will never match here */
    if ((buf[0] == 'G' || buf[0] == 'g') && buf[1] == '_')
        return (struct symparse *) 0;

    if (!p || (q && q < p))
        p = q;
    if (p) {
        /* note: there will be at most one space before the '='
           because caller has condensed buf[] with mungspaces() */
        if (p > buf && p[-1] == ' ')
            p--;
        len = (int) (p - buf);
    }
    while (sp->range) {
        if ((len >= strlen(sp->name)) && !strncmpi(buf, sp->name, len))
            return sp;
        sp++;
    }
    for (i = 0; i < SIZE(alternates); ++i) {
        if ((len >= strlen(alternates[i].altnm))
            && !strncmpi(buf, alternates[i].altnm, len)) {
            sp = loadsyms;
            while (sp->range) {
                if (!strcmp(alternates[i].nm, sp->name))
                    return sp;
                sp++;
            }
        }
    }
    return (struct symparse *) 0;
}

DISABLE_WARNING_FORMAT_NONLITERAL

/*
 * this is called from options.c to do the symset work.
 */
int
do_symset(boolean rogueflag)
{
    winid tmpwin;
    anything any;
    int n;
    char buf[BUFSZ];
    menu_item *symset_pick = (menu_item *) 0;
    boolean ready_to_switch = FALSE,
            nothing_to_do = FALSE;
    char *symset_name, fmtstr[20];
    struct symsetentry *sl;
    int res, which_set, setcount = 0, chosen = -2, defindx = 0;
    int clr = 0;

    which_set = rogueflag ? ROGUESET : PRIMARYSET;
    gs.symset_list = (struct symsetentry *) 0;
    /* clear symset[].name as a flag to read_sym_file() to build list */
    symset_name = gs.symset[which_set].name;
    gs.symset[which_set].name = (char *) 0;

    res = read_sym_file(which_set);
    /* put symset name back */
    gs.symset[which_set].name = symset_name;

    if (res && gs.symset_list) {
        int thissize,
            biggest = (int) (sizeof "Default Symbols" - sizeof ""),
            big_desc = 0;

        for (sl = gs.symset_list; sl; sl = sl->next) {
            /* check restrictions */
            if (rogueflag ? sl->primary : sl->rogue)
                continue;
#ifndef MAC_GRAPHICS_ENV
            if (sl->handling == H_MAC)
                continue;
#endif
#ifndef ENHANCED_SYMBOLS
            if (sl->handling == H_UTF8)
                continue;
#endif
            setcount++;
            /* find biggest name */
            thissize = sl->name ? (int) strlen(sl->name) : 0;
            if (thissize > biggest)
                biggest = thissize;
            thissize = sl->desc ? (int) strlen(sl->desc) : 0;
            if (thissize > big_desc)
                big_desc = thissize;
        }
        if (!setcount) {
            There("are no appropriate %s symbol sets available.",
                  rogueflag ? "rogue level" : "primary");
            return TRUE;
        }

        Sprintf(fmtstr, "%%-%ds %%s", biggest + 2);
        tmpwin = create_nhwindow(NHW_MENU);
        start_menu(tmpwin, MENU_BEHAVE_STANDARD);
        any = cg.zeroany;
        any.a_int = 1; /* -1 + 2 [see 'if (sl->name) {' below]*/
        if (!symset_name)
            defindx = any.a_int;
        add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                 clr, "Default Symbols",
                 (any.a_int == defindx) ? MENU_ITEMFLAGS_SELECTED
                                        : MENU_ITEMFLAGS_NONE);

        for (sl = gs.symset_list; sl; sl = sl->next) {
            /* check restrictions */
            if (rogueflag ? sl->primary : sl->rogue)
                continue;
#ifndef MAC_GRAPHICS_ENV
            if (sl->handling == H_MAC)
                continue;
#endif
#ifndef ENHANCED_SYMBOLS
            if (sl->handling == H_UTF8)
                continue;
#endif
            if (sl->name) {
                /* +2: sl->idx runs from 0 to N-1 for N symsets;
                   +1 because Defaults are implicitly in slot [0];
                   +1 again so that valid data is never 0 */
                any.a_int = sl->idx + 2;
                if (symset_name && !strcmpi(sl->name, symset_name))
                    defindx = any.a_int;
                Sprintf(buf, fmtstr, sl->name, sl->desc ? sl->desc : "");
                add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0,
                         ATR_NONE, clr, buf,
                         (any.a_int == defindx) ? MENU_ITEMFLAGS_SELECTED
                                                : MENU_ITEMFLAGS_NONE);
            }
        }
        Sprintf(buf, "Select %ssymbol set:",
                rogueflag ? "rogue level " : "");
        end_menu(tmpwin, buf);
        n = select_menu(tmpwin, PICK_ONE, &symset_pick);
        if (n > 0) {
            chosen = symset_pick[0].item.a_int;
            /* if picking non-preselected entry yields 2, make sure
               that we're going with the non-preselected one */
            if (n == 2 && chosen == defindx)
                chosen = symset_pick[1].item.a_int;
            chosen -= 2; /* convert menu index to symset index;
                          * "Default symbols" have index -1 */
            free((genericptr_t) symset_pick);
        } else if (n == 0 && defindx > 0) {
            chosen = defindx - 2;
        }
        destroy_nhwindow(tmpwin);

        if (chosen > -1) {
            /* chose an actual symset name from file */
            for (sl = gs.symset_list; sl; sl = sl->next)
                if (sl->idx == chosen)
                    break;
            if (sl) {
                /* free the now stale attributes */
                clear_symsetentry(which_set, TRUE);

                /* transfer only the name of the symbol set */
                gs.symset[which_set].name = dupstr(sl->name);
                ready_to_switch = TRUE;
            }
        } else if (chosen == -1) {
            /* explicit selection of defaults */
            /* free the now stale symset attributes */
            clear_symsetentry(which_set, TRUE);
        } else
            nothing_to_do = TRUE;
    } else if (!res) {
        /* The symbols file could not be accessed */
        pline("Unable to access \"%s\" file.", SYMBOLS);
        return TRUE;
    } else if (!gs.symset_list) {
        /* The symbols file was empty */
        There("were no symbol sets found in \"%s\".", SYMBOLS);
        return TRUE;
    }

    /* clean up */
    while ((sl = gs.symset_list) != 0) {
        gs.symset_list = sl->next;
        if (sl->name)
            free((genericptr_t) sl->name), sl->name = (char *) 0;
        if (sl->desc)
            free((genericptr_t) sl->desc), sl->desc = (char *) 0;
        free((genericptr_t) sl);
    }

    if (nothing_to_do)
        return TRUE;

    /* Set default symbols and clear the handling value */
    if (rogueflag)
        init_rogue_symbols();
    else
        init_primary_symbols();

    if (gs.symset[which_set].name) {
        /* non-default symbols */
        int ok;
#ifdef ENHANCED_SYMBOLS
        if (!glyphid_cache_status()) {
            fill_glyphid_cache();
        }
#endif
        ok = read_sym_file(which_set);
#ifdef ENHANCED_SYMBOLS
        if (glyphid_cache_status()) {
            free_glyphid_cache();
        }
#endif
        if (ok) {
            ready_to_switch = TRUE;
        } else {
            clear_symsetentry(which_set, TRUE);
            return TRUE;
        }
    }

    if (ready_to_switch)
        switch_symbols(TRUE);

    if (Is_rogue_level(&u.uz)) {
        if (rogueflag)
            assign_graphics(ROGUESET);
    } else if (!rogueflag)
        assign_graphics(PRIMARYSET);
#ifdef ENHANCED_SYMBOLS
    apply_customizations_to_symset(rogueflag ? ROGUESET : PRIMARYSET);
#endif
    preference_update("symset");
    return TRUE;
}

#ifdef ENHANCED_SYMBOLS

RESTORE_WARNING_FORMAT_NONLITERAL

struct customization_detail *find_display_sym_customization(
    const char *customization_name, const struct symparse *symparse,
    enum graphics_sets which_set);
struct customization_detail *find_matching_symset_customization(
    const char *customization_name, int custtype,
    enum graphics_sets which_set);
struct customization_detail *find_display_urep_customization(
    const char *customization_name, int glyphidx,
    enum graphics_sets which_set);
extern glyph_map glyphmap[MAX_GLYPH];
static void shuffle_customizations(void);

void
apply_customizations_to_symset(enum graphics_sets which_set)
{
    glyph_map *gmap;
    struct customization_detail *details;

    if (gs.symset[which_set].handling == H_UTF8
        && gs.sym_customizations[which_set].count
        && gs.sym_customizations[which_set].details) {
        /* These UTF-8 customizations get applied to the glyphmap array,
           not to symset entries */
        details = gs.sym_customizations[which_set].details;
        while (details) {
            gmap = &glyphmap[details->content.urep.glyphidx];
            (void) set_map_u(gmap,
                             details->content.urep.u.utf32ch,
                             details->content.urep.u.utf8str,
                             details->content.urep.u.ucolor);
            details = details->next;
        }
        shuffle_customizations();
    }
}
/* Shuffle the customizations to match shuffled object descriptions,
 * so a red potion isn't displayed with a blue customization, and so on.
 */
static void
shuffle_customizations(void)
{
    static const int offsets[2] = { GLYPH_OBJ_OFF, GLYPH_OBJ_PILETOP_OFF };
    int j;

    for (j = 0; j < SIZE(offsets); j++) {
        glyph_map *obj_glyphs = glyphmap + offsets[j];
        int i;
        struct unicode_representation *tmp_u[NUM_OBJECTS];
        int duplicate[NUM_OBJECTS];

        for (i = 0; i < NUM_OBJECTS; i++) {
            duplicate[i] = -1;
            tmp_u[i] = (struct unicode_representation *) 0;
        }
        for (i = 0; i < NUM_OBJECTS; i++) {
            int idx = objects[i].oc_descr_idx;

            /*
             * Shuffling gem appearances can cause the same oc_descr_idx to
             * appear more than once. Detect this condition and ensure that
             * each pointer points to a unique allocation.
             */
            if (duplicate[idx] >= 0) {
                /* Current structure already appears in tmp_u */
                struct unicode_representation *other = tmp_u[duplicate[idx]];

                tmp_u[i] = (struct unicode_representation *)
                           alloc(sizeof *tmp_u[i]);
                *tmp_u[i] = *other;
                if (other->utf8str != NULL) {
                    tmp_u[i]->utf8str = (uint8 *)
                                        dupstr((const char *) other->utf8str);
                }
            } else {
                tmp_u[i] = obj_glyphs[idx].u;
                if (obj_glyphs[idx].u != NULL)  {
                    duplicate[idx] = i;
                    obj_glyphs[idx].u = NULL;
                }
            }
        }
        for (i = 0; i < NUM_OBJECTS; i++) {
            /* Some glyphmaps may not have been transferred */
            if (obj_glyphs[i].u != NULL) {
                free(obj_glyphs[i].u->utf8str);
                free(obj_glyphs[i].u);
            }
            obj_glyphs[i].u = tmp_u[i];
        }
    }
}

struct customization_detail *
find_matching_symset_customization(
    const char *customization_name,
    int custtype,
    enum graphics_sets which_set)
{
    struct symset_customization *gdc = &gs.sym_customizations[which_set];

    if ((gdc->custtype == custtype)
        && (strcmp(customization_name, gdc->customization_name) != 0))
        return gdc->details;
    return (struct customization_detail *) 0;
}

static void
purge_custom_entries(enum graphics_sets which_set)
{
    struct symset_customization *gdc = &gs.sym_customizations[which_set];
    struct customization_detail *details = gdc->details, *next;

    while (details) {
        next = details->next;
        if (gdc->custtype == custom_ureps) {
            if (details->content.urep.u.utf8str)
                free(details->content.urep.u.utf8str);
            details->content.urep.u.utf8str = (uint8 *) 0;
            details->content.urep.u.ucolor = 0L;
            details->content.urep.u.u256coloridx = 0L;
        } else if (gdc->custtype == custom_symbols) {
            details->content.sym.symparse = (struct symparse *) 0;
            details->content.sym.val = 0;
        }
        free(details);
        details = next;
    }
    gdc->details = 0;
    gdc->details_end = 0;
    if (gdc->customization_name) {
        free((genericptr_t) gdc->customization_name);
        gdc->customization_name = 0;
    }
    gdc->count = 0;
}

struct customization_detail *
find_display_sym_customization(
    const char *customization_name,
    const struct symparse *symparse,
    enum graphics_sets which_set)
{
    struct symset_customization *gdc = &gs.sym_customizations[which_set];
    struct customization_detail *symdetails;

    if ((gdc->custtype == custom_symbols)
        && (strcmp(customization_name, gdc->customization_name) == 0)) {
        symdetails = gdc->details;
        while (symdetails) {
            if (symdetails->content.sym.symparse == symparse)
                return symdetails;
            symdetails = symdetails->next;
        }
    }
    return (struct customization_detail *) 0;
}
#endif /* ENHANCED_SYMBOLS */

/*symbols.c*/
