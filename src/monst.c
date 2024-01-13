/* NetHack 3.7	monst.c	$NHDT-Date: 1705092160 2024/01/12 20:42:40 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.100 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2006. */
/* NetHack may be freely redistributed.  See license for details. */

#include "config.h"
#include "permonst.h"
#include "wintype.h"
#include "sym.h"

#include "color.h"

#define NO_ATTK { 0, 0, 0, 0 }

/* monster type with single name */
#define MON(nam, sym, lvl, gen, atk, siz, mr1, mr2, \
            flg1, flg2, flg3, d, col, bn)           \
    {                                                                   \
        nam, PM_##bn,                                                   \
        sym, lvl, gen, atk, siz, mr1, mr2, flg1, flg2, flg3, d, col     \
    }

/* LVL() and SIZ() collect several fields to cut down on number of args
 * for MON().  Using more than 15 would fail to conform to the C Standard.
 * ATTK() and A() are to avoid braces and commas within args to MON().
 * NAM() and NAMS() are used for both reasons.
 */
#define NAM(name) { (const char *) 0, (const char *) 0, name }
#define NAMS(namm, namf, namn) { namm, namf, namn }
#define LVL(lvl, mov, ac, mr, aln) lvl, mov, ac, mr, aln
#define SIZ(wt, nut, snd, siz) wt, nut, snd, siz
#define ATTK(at, ad, n, d) { at, ad, n, d }
#define A(a1, a2, a3, a4, a5, a6) { a1, a2, a3, a4, a5, a6 }

struct permonst mons_init[NUMMONS + 1] = {
#include "monsters.h"
    /*
     * Array terminator, added to the end of the entries in monsters.h.
     *
     * mons[NUMMONS] used to be all zero except "" instead of Null for
     * the name field.  Then the index field was added and the terminator
     * uses NON_PM for that.  Now, a few monster flags also get set.
     */
#undef MON
#define MON(nam, sym, lvl, gen, atk, siz, mr1, mr2, \
            flg1, flg2, flg3, d, col, bn)           \
    {                                                                   \
        nam, NON_PM,                                                    \
        sym, lvl, gen, atk, siz, mr1, mr2, flg1, flg2, flg3, d, col     \
    }
    MON(NAM(""), 0,
        LVL(0, 0, 0, 0, 0), G_NOGEN | G_NOCORPSE,
        A(NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(0, 0, 0, 0), 0, 0,
        0L,  M2_NOPOLY, 0,
        0, 0, 0),
};

#undef MON
#undef NAM
#undef NAMS

void monst_globals_init(void); /* in hack.h but we're using config.h */

struct permonst mons[SIZE(mons_init)];

void
monst_globals_init(void)
{
    memcpy(mons, mons_init, sizeof mons);
    return;
}

const struct attack c_sa_yes[NATTK] = SEDUCTION_ATTACKS_YES;
const struct attack c_sa_no[NATTK] = SEDUCTION_ATTACKS_NO;

/* for 'onefile' processing where end of this file isn't necessarily the
   end of the source code seen by the compiler */
#undef NO_ATTK
#undef LVL
#undef SIZ
#undef ATTK
#undef A

/*monst.c*/
