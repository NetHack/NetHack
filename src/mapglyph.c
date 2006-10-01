/*	SCCS Id: @(#)mapglyph.c	3.5	2006/10/01	*/
/* Copyright (c) David Cohrs, 1991				  */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#if defined(TTY_GRAPHICS)
#include "wintty.h"	/* for prototype of has_color() only */
#endif
#include "color.h"
#define HI_DOMESTIC CLR_WHITE	/* monst.c */

int explcolors[] = {
	CLR_BLACK,	/* dark    */
	CLR_GREEN,	/* noxious */
	CLR_BROWN,	/* muddy   */
	CLR_BLUE,	/* wet     */
	CLR_MAGENTA,	/* magical */
	CLR_ORANGE,	/* fiery   */
	CLR_WHITE,	/* frosty  */
};

#if !defined(TTY_GRAPHICS)
#define has_color(n)  TRUE
#endif

#ifdef TEXTCOLOR
#define zap_color(n)  color = iflags.use_color ? zapcolors[n] : NO_COLOR
#define cmap_color(n) color = iflags.use_color ? defsyms[n].color : NO_COLOR
#define obj_color(n)  color = iflags.use_color ? objects[n].oc_color : NO_COLOR
#define mon_color(n)  color = iflags.use_color ? mons[n].mcolor : NO_COLOR
#define invis_color(n) color = NO_COLOR
#define pet_color(n)  color = iflags.use_color ? mons[n].mcolor : NO_COLOR
#define warn_color(n) color = iflags.use_color ? def_warnsyms[n].color : NO_COLOR
#define explode_color(n) color = iflags.use_color ? explcolors[n] : NO_COLOR
# if defined(REINCARNATION) && defined(ASCIIGRAPH)
#  define ROGUE_COLOR
# endif

#else	/* no text color */

#define zap_color(n)
#define cmap_color(n)
#define obj_color(n)
#define mon_color(n)
#define invis_color(n)
#define pet_color(c)
#define warn_color(n)
#define explode_color(n)
#endif

# if defined(USE_TILES) && defined(MSDOS)
#define HAS_ROGUE_IBM_GRAPHICS (currentgraphics == ROGUESET && \
				SYMHANDLING(H_IBM) && !iflags.grmode)
# else
#define HAS_ROGUE_IBM_GRAPHICS (currentgraphics == ROGUESET && \
				SYMHANDLING(H_IBM))
# endif

/*ARGSUSED*/
int
mapglyph(glyph, ochar, ocolor, ospecial, x, y)
int glyph, *ocolor, x, y;
int *ochar;
unsigned *ospecial;
{
	register int offset, idx;
#if defined(TEXTCOLOR) || defined(ROGUE_COLOR)
	int color = NO_COLOR;
#endif
	uchar ch;
	unsigned special = 0;
	/* condense multiple tests in macro version down to single */
	boolean has_rogue_ibm_graphics = HAS_ROGUE_IBM_GRAPHICS;
#ifdef ROGUE_COLOR
	boolean has_rogue_color = (has_rogue_ibm_graphics &&
				   (symset[currentgraphics].nocolor == 0));
#endif

    /*
     *  Map the glyph back to a character and color.
     *
     *  Warning:  For speed, this makes an assumption on the order of
     *		  offsets.  The order is set in display.h.
     */
    if ((offset = (glyph - GLYPH_STATUE_OFF)) >= 0) {   /* a statue */
	idx = mons[offset].mlet + SYM_OFF_M;
# ifdef ROGUE_COLOR
	if (has_rogue_color)
		color = CLR_RED;
	else
# endif
	obj_color(STATUE);
	special |= MG_STATUE;
    } else if ((offset = (glyph - GLYPH_WARNING_OFF)) >= 0) {	/* a warning flash */
	idx = offset + SYM_OFF_W;
# ifdef ROGUE_COLOR
	if (has_rogue_color)
	    color = NO_COLOR;
	else
# endif
	    warn_color(offset);
    } else if ((offset = (glyph - GLYPH_SWALLOW_OFF)) >= 0) {	/* swallow */
	/* see swallow_to_glyph() in display.c */
	idx = (S_sw_tl + (offset & 0x7)) + SYM_OFF_P;
#ifdef ROGUE_COLOR
	if (has_rogue_color && iflags.use_color)
	    color = NO_COLOR;
	else
#endif
	    mon_color(offset >> 3);
    } else if ((offset = (glyph - GLYPH_ZAP_OFF)) >= 0) {	/* zap beam */
	/* see zapdir_to_glyph() in display.c */
	idx = (S_vbeam + (offset & 0x3)) + SYM_OFF_P;
#ifdef ROGUE_COLOR
	if (has_rogue_color && iflags.use_color)
	    color = NO_COLOR;
	else
#endif
	    zap_color((offset >> 2));
    } else if ((offset = (glyph - GLYPH_EXPLODE_OFF)) >= 0) {	/* explosion */
	idx = ((offset % MAXEXPCHARS) + S_explode1) + SYM_OFF_P;
	explode_color(offset / MAXEXPCHARS);
    } else if ((offset = (glyph - GLYPH_CMAP_OFF)) >= 0) {	/* cmap */
	idx = offset + SYM_OFF_P;
#ifdef ROGUE_COLOR
	if (has_rogue_color && iflags.use_color) {
	    if (offset >= S_vwall && offset <= S_hcdoor)
		color = CLR_BROWN;
	    else if (offset >= S_arrow_trap && offset <= S_polymorph_trap)
		color = CLR_MAGENTA;
	    else if (offset == S_corr || offset == S_litcorr)
		color = CLR_GRAY;
	    else if (offset >= S_room && offset <= S_water)
		color = CLR_GREEN;
	    else
		color = NO_COLOR;
	} else
#endif
#ifdef TEXTCOLOR
	    /* provide a visible difference if normal and lit corridor
	     * use the same symbol */
	    if (iflags.use_color && offset == S_litcorr &&
		showsyms[idx] == showsyms[S_corr + SYM_OFF_P])
		color = CLR_WHITE;
	    else
#endif
	    cmap_color(offset);
    } else if ((offset = (glyph - GLYPH_OBJ_OFF)) >= 0) {	/* object */
	idx = objects[offset].oc_class + SYM_OFF_O;
	if (offset == BOULDER && iflags.bouldersym)
		idx = SYM_BOULDER + SYM_OFF_X;
#ifdef ROGUE_COLOR
	if (has_rogue_color && iflags.use_color) {
	    switch(objects[offset].oc_class) {
		case COIN_CLASS: color = CLR_YELLOW; break;
		case FOOD_CLASS: color = CLR_RED; break;
		default: color = CLR_BRIGHT_BLUE; break;
	    }
	} else
#endif
	    obj_color(offset);
    } else if ((offset = (glyph - GLYPH_RIDDEN_OFF)) >= 0) {	/* mon ridden */
	idx = mons[offset].mlet + SYM_OFF_M;
#ifdef ROGUE_COLOR
	if (has_rogue_color)
	    /* This currently implies that the hero is here -- monsters */
	    /* don't ride (yet...).  Should we set it to yellow like in */
	    /* the monster case below?  There is no equivalent in rogue. */
	    color = NO_COLOR;	/* no need to check iflags.use_color */
	else
#endif
	    mon_color(offset);
	    special |= MG_RIDDEN;
    } else if ((offset = (glyph - GLYPH_BODY_OFF)) >= 0) {	/* a corpse */
	idx = objects[CORPSE].oc_class + SYM_OFF_O;
#ifdef ROGUE_COLOR
	if (has_rogue_color && iflags.use_color)
	    color = CLR_RED;
	else
#endif
	    mon_color(offset);
	    special |= MG_CORPSE;
    } else if ((offset = (glyph - GLYPH_DETECT_OFF)) >= 0) {	/* mon detect */
	idx = mons[offset].mlet + SYM_OFF_M;
#ifdef ROGUE_COLOR
	if (has_rogue_color)
	    color = NO_COLOR;	/* no need to check iflags.use_color */
	else
#endif
	    mon_color(offset);
	/* Disabled for now; anyone want to get reverse video to work? */
	/* is_reverse = TRUE; */
	    special |= MG_DETECT;
    } else if ((offset = (glyph - GLYPH_INVIS_OFF)) >= 0) {	/* invisible */
	idx = SYM_INVISIBLE + SYM_OFF_X;
#ifdef ROGUE_COLOR
	if (has_rogue_color)
	    color = NO_COLOR;	/* no need to check iflags.use_color */
	else
#endif
	    invis_color(offset);
	    special |= MG_INVIS;
    } else if ((offset = (glyph - GLYPH_PET_OFF)) >= 0) {	/* a pet */
	idx = mons[offset].mlet + SYM_OFF_M;
#ifdef ROGUE_COLOR
	if (has_rogue_color)
	    color = NO_COLOR;	/* no need to check iflags.use_color */
	else
#endif
	    pet_color(offset);
	    special |= MG_PET;
    } else {							/* a monster */
	idx = mons[glyph].mlet + SYM_OFF_M;
#ifdef ROGUE_COLOR
	if (has_rogue_color && iflags.use_color) {
	    if (x == u.ux && y == u.uy)
		/* actually player should be yellow-on-gray if in a corridor */
		color = CLR_YELLOW;
	    else
		color = NO_COLOR;
	} else
#endif
	{
	    mon_color(glyph);
	    /* special case the hero for `showrace' option */
#ifdef TEXTCOLOR
	    if (iflags.use_color && x == u.ux && y == u.uy &&
		    flags.showrace && !Upolyd)
		color = HI_DOMESTIC;
#endif
	}
    }

    ch = showsyms[idx];
#ifdef TEXTCOLOR
    /* Turn off color if no color defined, or rogue level w/o PC graphics. */
# ifdef REINCARNATION
#  ifdef ROGUE_COLOR
    if (!has_color(color) || (Is_rogue_level(&u.uz) && !has_rogue_color))
#  else
    if (!has_color(color) || Is_rogue_level(&u.uz))
#  endif
# else
    if (!has_color(color))
# endif
	color = NO_COLOR;
#endif

    *ochar = (int)ch;
    *ospecial = special;
#ifdef TEXTCOLOR
    *ocolor = color;
#endif
    return idx;
}

/*mapglyph.c*/
