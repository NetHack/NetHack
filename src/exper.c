/* NetHack 3.7	exper.c	$NHDT-Date: 1621380393 2021/05/18 23:26:33 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.46 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2007. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#ifndef LONG_MAX
#include <limits.h>
#endif

static int enermod(int);

long
newuexp(int lev)
{
    if (lev < 1) /* for newuexp(u.ulevel - 1) when u.ulevel is 1 */
        return 0L;
    if (lev < 10)
        return (10L * (1L << lev));
    if (lev < 20)
        return (10000L * (1L << (lev - 10)));
    return (10000000L * ((long) (lev - 19)));
}

static int
enermod(int en)
{
    switch (Role_switch) {
    case PM_CLERIC:
    case PM_WIZARD:
        return (2 * en);
    case PM_HEALER:
    case PM_KNIGHT:
        return ((3 * en) / 2);
    case PM_BARBARIAN:
    case PM_VALKYRIE:
        return ((3 * en) / 4);
    default:
        return en;
    }
}

/* calculate spell power/energy points for new level */
int
newpw(void)
{
    int en = 0, enrnd, enfix;

    if (u.ulevel == 0) {
        en = gu.urole.enadv.infix + gu.urace.enadv.infix;
        if (gu.urole.enadv.inrnd > 0)
            en += rnd(gu.urole.enadv.inrnd);
        if (gu.urace.enadv.inrnd > 0)
            en += rnd(gu.urace.enadv.inrnd);
    } else {
        enrnd = (int) ACURR(A_WIS) / 2;
        if (u.ulevel < gu.urole.xlev) {
            enrnd += gu.urole.enadv.lornd + gu.urace.enadv.lornd;
            enfix = gu.urole.enadv.lofix + gu.urace.enadv.lofix;
        } else {
            enrnd += gu.urole.enadv.hirnd + gu.urace.enadv.hirnd;
            enfix = gu.urole.enadv.hifix + gu.urace.enadv.hifix;
        }
        en = enermod(rn1(enrnd, enfix));
    }
    if (en <= 0)
        en = 1;
    if (u.ulevel < MAXULEV) {
        /* remember increment; future level drain could take it away again */
        u.ueninc[u.ulevel] = (xint16) en;
    } else {
        /* after level 30, throttle energy gains from extra experience;
           once max reaches 600, further increments will be just 1 more */
        char lim = 4 - u.uenmax / 200;

        lim = max(lim, 1);
        if (en > lim)
            en = lim;
    }
    return en;
}

/* return # of exp points for mtmp after nk killed */
int
experience(register struct monst *mtmp, register int nk)
{
    register struct permonst *ptr = mtmp->data;
    int i, tmp, tmp2;

    tmp = 1 + mtmp->m_lev * mtmp->m_lev;

    /*  For higher ac values, give extra experience */
    if ((i = find_mac(mtmp)) < 3)
        tmp += (7 - i) * ((i < 0) ? 2 : 1);

    /*  For very fast monsters, give extra experience */
    if (ptr->mmove > NORMAL_SPEED)
        tmp += (ptr->mmove > (3 * NORMAL_SPEED / 2)) ? 5 : 3;

    /*  For each "special" attack type give extra experience */
    for (i = 0; i < NATTK; i++) {
        tmp2 = ptr->mattk[i].aatyp;
        if (tmp2 > AT_BUTT) {
            if (tmp2 == AT_WEAP)
                tmp += 5;
            else if (tmp2 == AT_MAGC)
                tmp += 10;
            else
                tmp += 3;
        }
    }

    /*  For each "special" damage type give extra experience */
    for (i = 0; i < NATTK; i++) {
        tmp2 = ptr->mattk[i].adtyp;
        if (tmp2 > AD_PHYS && tmp2 < AD_BLND)
            tmp += 2 * mtmp->m_lev;
        else if ((tmp2 == AD_DRLI) || (tmp2 == AD_STON) || (tmp2 == AD_SLIM))
            tmp += 50;
        else if (tmp2 != AD_PHYS)
            tmp += mtmp->m_lev;
        /* extra heavy damage bonus */
        if ((int) (ptr->mattk[i].damd * ptr->mattk[i].damn) > 23)
            tmp += mtmp->m_lev;
        if (tmp2 == AD_WRAP && ptr->mlet == S_EEL && !Amphibious)
            tmp += 1000;
    }

    /*  For certain "extra nasty" monsters, give even more */
    if (extra_nasty(ptr))
        tmp += (7 * mtmp->m_lev);

    /*  For higher level monsters, an additional bonus is given */
    if (mtmp->m_lev > 8)
        tmp += 50;

#ifdef MAIL_STRUCTURES
    /* Mail daemons put up no fight. */
    if (mtmp->data == &mons[PM_MAIL_DAEMON])
        tmp = 1;
#endif

    if (mtmp->mrevived || mtmp->mcloned) {
        /*
         *      Reduce experience awarded for repeated killings of
         *      "the same monster".  Kill count includes all of this
         *      monster's type which have been killed--including the
         *      current monster--regardless of how they were created.
         *        1.. 20        full experience
         *       21.. 40        xp / 2
         *       41.. 80        xp / 4
         *       81..120        xp / 8
         *      121..180        xp / 16
         *      181..240        xp / 32
         *      241..255+       xp / 64
         */
        for (i = 0, tmp2 = 20; nk > tmp2 && tmp > 1; ++i) {
            tmp = (tmp + 1) / 2;
            nk -= tmp2;
            if (i & 1)
                tmp2 += 20;
        }
    }

    return (tmp);
}

void
more_experienced(register int exper, register int rexp)
{
    long oldexp = u.uexp,
         oldrexp = u.urexp,
         newexp = oldexp + exper,
         rexpincr = 4 * exper + rexp,
         newrexp = oldrexp + rexpincr;

    /* cap experience and score on wraparound */
    if (newexp < 0 && exper > 0)
        newexp = LONG_MAX;
    if (newrexp < 0 && rexpincr > 0)
        newrexp = LONG_MAX;

    if (newexp != oldexp) {
        u.uexp = newexp;
        if (flags.showexp)
            gc.context.botl = TRUE;
        /* even when experience points aren't being shown, experience level
           might be highlighted with a percentage highlight rule and that
           percentage depends upon experience points */
        if (!gc.context.botl && exp_percent_changing())
            gc.context.botl = TRUE;
    }
    /* newrexp will always differ from oldrexp unless they're LONG_MAX */
    if (newrexp != oldrexp) {
        u.urexp = newrexp;
#ifdef SCORE_ON_BOTL
        if (flags.showscore)
            gc.context.botl = TRUE;
#endif
    }
    if (u.urexp >= (Role_if(PM_WIZARD) ? 1000 : 2000))
        flags.beginner = 0;
}

/* e.g., hit by drain life attack */
void
losexp(
    const char *drainer) /* cause of death, if drain should be fatal */
{
    int num, uhpmin, olduhpmax;

    /* override life-drain resistance when handling an explicit
       wizard mode request to reduce level; never fatal though */
    if (drainer && !strcmp(drainer, "#levelchange"))
        drainer = 0;
    else if (resists_drli(&gy.youmonst))
        return;

    /* level-loss message; "Goodbye level 1." is fatal; divine anger
       (drainer==NULL) resets a level 1 character to 0 experience points
       without reducing level and that isn't fatal so suppress the message
       in that situation */
    if (u.ulevel > 1 || drainer)
        pline("%s level %d.", Goodbye(), u.ulevel);
    if (u.ulevel > 1) {
        u.ulevel -= 1;
        /* remove intrinsic abilities */
        adjabil(u.ulevel + 1, u.ulevel);
        livelog_printf(LL_MINORAC, "lost experience level %d", u.ulevel + 1);
    } else {
        if (drainer) {
            gk.killer.format = KILLED_BY;
            if (gk.killer.name != drainer)
                Strcpy(gk.killer.name, drainer);
            done(DIED);
        }
        /* no drainer or lifesaved */
        u.uexp = 0;
        livelog_printf(LL_MINORAC, "lost all experience");
    }

    olduhpmax = u.uhpmax;
    uhpmin = minuhpmax(10); /* same minimum as is used by life-saving */
    num = (int) u.uhpinc[u.ulevel];
    u.uhpmax -= num;
    if (u.uhpmax < uhpmin)
        setuhpmax(uhpmin);
    /* uhpmax might try to go up if it has previously been reduced by
       strength loss or by a fire trap or by an attack by Death which
       all use a different minimum than life-saving or experience loss;
       we don't allow it to go up because that contradicts assumptions
       elsewhere (such as healing wielder who drains with Strombringer) */
    if (u.uhpmax > olduhpmax)
        setuhpmax(olduhpmax);

    u.uhp -= num;
    if (u.uhp < 1)
        u.uhp = 1;
    else if (u.uhp > u.uhpmax)
        u.uhp = u.uhpmax;

    num = (int) u.ueninc[u.ulevel];
    u.uenmax -= num;
    if (u.uenmax < 0)
        u.uenmax = 0;
    u.uen -= num;
    if (u.uen < 0)
        u.uen = 0;
    else if (u.uen > u.uenmax)
        u.uen = u.uenmax;

    if (u.uexp > 0)
        u.uexp = newuexp(u.ulevel) - 1;

    if (Upolyd) {
        num = monhp_per_lvl(&gy.youmonst);
        u.mhmax -= num;
        u.mh -= num;
        if (u.mh <= 0)
            rehumanize();
    }

    gc.context.botl = TRUE;
}

/*
 * Make experience gaining similar to AD&D(tm), whereby you can at most go
 * up by one level at a time, extra expr possibly helping you along.
 * After all, how much real experience does one get shooting a wand of death
 * at a dragon created with a wand of polymorph??
 */
void
newexplevel(void)
{
    if (u.ulevel < MAXULEV && u.uexp >= newuexp(u.ulevel))
        pluslvl(TRUE);
}

void
pluslvl(
    boolean incr) /* True: incremental experience growth;
                   * False: potion of gain level or wraith corpse */
{
    int hpinc, eninc;

    if (!incr)
        You_feel("more experienced.");

    /* increase hit points (when polymorphed, do monster form first
       in order to retain normal human/whatever increase for later) */
    if (Upolyd) {
        hpinc = monhp_per_lvl(&gy.youmonst);
        u.mhmax += hpinc;
        u.mh += hpinc;
    }
    hpinc = newhp();
    setuhpmax(u.uhpmax + hpinc);
    u.uhp += hpinc;

    /* increase spell power/energy points */
    eninc = newpw();
    u.uenmax += eninc;
    if (u.uenmax > u.uenpeak)
        u.uenpeak = u.uenmax;
    u.uen += eninc;

    /* increase level (unless already maxxed) */
    if (u.ulevel < MAXULEV) {
        int old_ach_cnt, newrank, oldrank = xlev_to_rank(u.ulevel);

        /* increase experience points to reflect new level */
        if (incr) {
            long tmp = newuexp(u.ulevel + 1);

            if (u.uexp >= tmp)
                u.uexp = tmp - 1;
        } else {
            u.uexp = newuexp(u.ulevel);
        }
        ++u.ulevel;
        pline("Welcome %sto experience level %d.",
              (u.ulevelmax < u.ulevel) ? "" : "back ",
              u.ulevel);
        if (u.ulevelmax < u.ulevel)
            u.ulevelmax = u.ulevel;
        adjabil(u.ulevel - 1, u.ulevel); /* give new intrinsics */

        old_ach_cnt = count_achievements();
        newrank = xlev_to_rank(u.ulevel);
        if (newrank > oldrank)
            record_achievement(achieve_rank(newrank));
        /* a new rank achievement will log its own message; log a simpler
           message here if we didn't just get an achievement (so when rank
           hasn't changed or hero just regained a lost level and the rank
           achievement doesn't get repeated) */
        if (count_achievements() == old_ach_cnt)
            livelog_printf(LL_MINORAC, "%sgained experience level %d",
                           (u.ulevel <= u.ulevelpeak) ? "re" : "", u.ulevel);
        if (u.ulevel > u.ulevelpeak)
            u.ulevelpeak = u.ulevel;
    }
    gc.context.botl = TRUE;
}

/* compute a random amount of experience points suitable for the hero's
   experience level:  base number of points needed to reach the current
   level plus a random portion of what it takes to get to the next level */
long
rndexp(boolean gaining) /* gaining XP via potion vs setting XP for polyself */
{
    long minexp, maxexp, diff, factor, result;

    minexp = (u.ulevel == 1) ? 0L : newuexp(u.ulevel - 1);
    maxexp = newuexp(u.ulevel);
    diff = maxexp - minexp, factor = 1L;
    /* make sure that `diff' is an argument which rn2() can handle */
    while (diff >= (long) LARGEST_INT)
        diff /= 2L, factor *= 2L;
    result = minexp + factor * (long) rn2((int) diff);
    /* 3.4.1:  if already at level 30, add to current experience
       points rather than to threshold needed to reach the current
       level; otherwise blessed potions of gain level can result
       in lowering the experience points instead of raising them */
    if (u.ulevel == MAXULEV && gaining) {
        result += (u.uexp - minexp);
        /* avoid wrapping (over 400 blessed potions needed for that...) */
        if (result < u.uexp)
            result = u.uexp;
    }
    return result;
}

/*exper.c*/
