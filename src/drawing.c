/* NetHack 3.7	drawing.c	$NHDT-Date: 1596498163 2020/08/03 23:42:43 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.78 $ */
/* Copyright (c) NetHack Development Team 1992.                   */
/* NetHack may be freely redistributed.  See license for details. */

#include "config.h"
#include "color.h"
#include "rm.h"
#include "objclass.h"
#include "wintype.h"
#include "sym.h"

/* Relevant header information in rm.h, objclass.h, sym.h, defsym.h. */

#ifdef C
#undef C
#endif

#ifdef TEXTCOLOR
#define C(n) n
#else
#define C(n)
#endif

/* Default object class symbols.  See objclass.h.
 * {symbol, name, explain}
 *     name:    used in object_detect().
 *     explain: used in do_look().
 */
const struct class_sym def_oc_syms[MAXOCLASSES] = {
    { '\0', "", "" }, /* placeholder for the "random class" */
#define OBJCLASS_DRAWING
#include "defsym.h"
#undef OBJCLASS_DRAWING
};

/* Default monster class symbols.  See sym.h and defsym.h. */
const struct class_sym def_monsyms[MAXMCLASSES] = {
    { '\0', "", "" },
#define MONSYMS_DRAWING
#include "defsym.h"
#undef MONSYMS_DRAWING
};

const struct symdef def_warnsyms[WARNCOUNT] = {
    /* white warning  */
    { '0', "unknown creature causing you worry",    C(CLR_WHITE) },
    /* pink warning   */
    { '1', "unknown creature causing you concern",  C(CLR_RED) },
    /* red warning    */
    { '2', "unknown creature causing you anxiety",  C(CLR_RED) },
    /* ruby warning   */
    { '3', "unknown creature causing you disquiet", C(CLR_RED) },
    /* purple warning */
    { '4', "unknown creature causing you alarm",    C(CLR_MAGENTA) },
    /* black warning  */
    { '5', "unknown creature causing you dread",    C(CLR_BRIGHT_MAGENTA) },
};

/*
 *  Default screen symbols with explanations and colors.
 *
 *  If adding to or removing from this list, please note that,
 *  for builds with tile support, there is an array called altlabels[] in
 *  win/share/tilemap.c that requires the same number of elements as
 *  this, in the same order. It is used for tile name matching when
 *  parsing other.txt because some of the useful tile names don't exist
 *  within NetHack itself.
 */
const struct symdef defsyms[MAXPCHARS] = {
#define PCHAR_DRAWING
#include "defsym.h"
#undef PCHAR_DRAWING
};

/* default rogue level symbols */
const uchar def_r_oc_syms[MAXOCLASSES] = {
/* 0*/ '\0', ILLOBJ_SYM, WEAPON_SYM, ']', /* armor */
       RING_SYM,
/* 5*/ ',',                     /* amulet */
       TOOL_SYM, ':',           /* food */
       POTION_SYM, SCROLL_SYM,
/*10*/ SPBOOK_SYM, WAND_SYM,
       GEM_SYM,                /* gold -- yes it's the same as gems */
       GEM_SYM, ROCK_SYM,
/*15*/ BALL_SYM, CHAIN_SYM, VENOM_SYM
};

#undef C

/*
 * Convert the given character to an object class.  If the character is not
 * recognized, then MAXOCLASSES is returned.  Used in detect.c, invent.c,
 * objnam.c, options.c, pickup.c, sp_lev.c, lev_main.c, and tilemap.c.
 */
int
def_char_to_objclass(char ch)
{
    int i;

    for (i = 1; i < MAXOCLASSES; i++)
        if (ch == def_oc_syms[i].sym)
            break;
    return i;
}

/*
 * Convert a character into a monster class.  This returns the _first_
 * match made.  If there are are no matches, return MAXMCLASSES.
 * Used in detect.c, options.c, read.c, sp_lev.c, and lev_main.c
 */
int
def_char_to_monclass(char ch)
{
    int i;

    for (i = 1; i < MAXMCLASSES; i++)
        if (ch == def_monsyms[i].sym)
            break;
    return i;
}

/* does 'ch' represent a furniture character?  returns index into defsyms[] */
int
def_char_is_furniture(char ch)
{
    /* note: these refer to defsyms[] order which is much different from
       levl[][].typ order but both keep furniture in a contiguous block */
    static const char first_furniture[] = "stair", /* "staircase up" */
                      last_furniture[] = "fountain";
    int i;
    boolean furniture = FALSE;

    for (i = 0; i < SIZE(defsyms); ++i) {
        if (!furniture) {
            if (!strncmp(defsyms[i].explanation, first_furniture, 5))
                furniture = TRUE;
        }
        if (furniture) {
            if (defsyms[i].sym == (uchar) ch)
                return i;
            if (!strcmp(defsyms[i].explanation, last_furniture))
                break; /* reached last furniture */
        }
    }
    return -1;
}

/*drawing.c*/
