/* NetHack 3.7	vision.h	$NHDT-Date: 1666478832 2022/10/22 22:47:12 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.15 $ */
/* Copyright (c) Dean Luick, with acknowledgements to Dave Cohrs, 1990. */
/* NetHack may be freely redistributed.  See license for details.       */

#ifndef VISION_H
#define VISION_H

#define COULD_SEE 0x1 /* location could be seen, if it were lit */
#define IN_SIGHT 0x2  /* location can be seen */
#define TEMP_LIT 0x4  /* location is temporarily lit */

/*
 * Light source sources
 */
#define LS_OBJECT 0
#define LS_MONSTER 1

/*
 *  cansee()    - Returns true if the hero can see the location.
 *
 *  couldsee()  - Returns true if the hero has a clear line of sight to
 *                the location.
 */
#define cansee(x, y) ((g.viz_array[y][x] & IN_SIGHT) != 0)
#define couldsee(x, y) ((g.viz_array[y][x] & COULD_SEE) != 0)
#define templit(x, y) ((g.viz_array[y][x] & TEMP_LIT) != 0)

/*
 *  The following assume the monster is not blind.
 *
 *  m_cansee()  - Returns true if the monster can see the given location.
 *
 *  m_canseeu() - Returns true if the monster could see the hero.  Assumes
 *                that if the hero has a clear line of sight to the monster's
 *                location and the hero is visible, then monster can see the
 *                hero.
 */
#define m_cansee(mtmp, x2, y2) clear_path((mtmp)->mx, (mtmp)->my, (x2), (y2))

#if 0
#define m_canseeu(m) \
    ((!Invis || perceives((m)->data))                      \
     && !(Underwater || u.uburied || (m)->mburied)         \
     && couldsee((m)->mx, (m)->my))
#else   /* without 'uburied' and 'mburied' */
#define m_canseeu(m) \
    ((!Invis || perceives((m)->data))                      \
     && !Underwater                                        \
     && couldsee((m)->mx, (m)->my))
#endif

/*
 *  Circle information
 */
#define MAX_RADIUS 15 /* this is in points from the source */

/* Use this macro to get a list of distances of the edges (see vision.c). */
#define circle_ptr(z) (&circle_data[(int) circle_start[z]])

/* howmonseen() bitmask values */
#define MONSEEN_NORMAL   0x0001 /* normal vision */
#define MONSEEN_SEEINVIS 0x0002 /* seeing invisible */
#define MONSEEN_INFRAVIS 0x0004 /* via infravision */
#define MONSEEN_TELEPAT  0x0008 /* via telepathy */
#define MONSEEN_XRAYVIS  0x0010 /* via Xray vision */
#define MONSEEN_DETECT   0x0020 /* via extended monster detection */
#define MONSEEN_WARNMON  0x0040 /* via type-specific warning */

#endif /* VISION_H */
