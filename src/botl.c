/* NetHack 3.6	botl.c	$NHDT-Date: 1469930895 2016/07/31 02:08:15 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.75 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include <limits.h>

extern const char *hu_stat[]; /* defined in eat.c */

const char *const enc_stat[] = { "",         "Burdened",  "Stressed",
                                 "Strained", "Overtaxed", "Overloaded" };

STATIC_OVL NEARDATA int mrank_sz = 0; /* loaded by max_rank_sz (from u_init) */
STATIC_DCL const char *NDECL(rank);

#ifndef STATUS_VIA_WINDOWPORT

STATIC_DCL void NDECL(bot1);
STATIC_DCL void NDECL(bot2);

STATIC_OVL void
bot1()
{
    char newbot1[MAXCO];
    register char *nb;
    register int i, j;

    Strcpy(newbot1, plname);
    if ('a' <= newbot1[0] && newbot1[0] <= 'z')
        newbot1[0] += 'A' - 'a';
    newbot1[10] = 0;
    Sprintf(nb = eos(newbot1), " the ");

    if (Upolyd) {
        char mbot[BUFSZ];
        int k = 0;

        Strcpy(mbot, mons[u.umonnum].mname);
        while (mbot[k] != 0) {
            if ((k == 0 || (k > 0 && mbot[k - 1] == ' ')) && 'a' <= mbot[k]
                && mbot[k] <= 'z')
                mbot[k] += 'A' - 'a';
            k++;
        }
        Strcpy(nb = eos(nb), mbot);
    } else
        Strcpy(nb = eos(nb), rank());

    Sprintf(nb = eos(nb), "  ");
    i = mrank_sz + 15;
    j = (int) ((nb + 2) - newbot1); /* strlen(newbot1) but less computation */
    if ((i - j) > 0)
        Sprintf(nb = eos(nb), "%*s", i - j, " "); /* pad with spaces */
    if (ACURR(A_STR) > 18) {
        if (ACURR(A_STR) > STR18(100))
            Sprintf(nb = eos(nb), "St:%2d ", ACURR(A_STR) - 100);
        else if (ACURR(A_STR) < STR18(100))
            Sprintf(nb = eos(nb), "St:18/%02d ", ACURR(A_STR) - 18);
        else
            Sprintf(nb = eos(nb), "St:18/** ");
    } else
        Sprintf(nb = eos(nb), "St:%-1d ", ACURR(A_STR));
    Sprintf(nb = eos(nb), "Dx:%-1d Co:%-1d In:%-1d Wi:%-1d Ch:%-1d",
            ACURR(A_DEX), ACURR(A_CON), ACURR(A_INT), ACURR(A_WIS),
            ACURR(A_CHA));
    Sprintf(nb = eos(nb),
            (u.ualign.type == A_CHAOTIC)
                ? "  Chaotic"
                : (u.ualign.type == A_NEUTRAL) ? "  Neutral" : "  Lawful");
#ifdef SCORE_ON_BOTL
    if (flags.showscore)
        Sprintf(nb = eos(nb), " S:%ld", botl_score());
#endif
    curs(WIN_STATUS, 1, 0);
    putstr(WIN_STATUS, 0, newbot1);
}

STATIC_OVL void
bot2()
{
    char newbot2[MAXCO], /* MAXCO: botl.h */
         /* dungeon location (and gold), hero health (HP, PW, AC),
            experience (HD if poly'd, else Exp level and maybe Exp points),
            time (in moves), varying number of status conditions */
         dloc[QBUFSZ], hlth[QBUFSZ], expr[QBUFSZ], tmmv[QBUFSZ], cond[QBUFSZ];
    register char *nb;
    unsigned dln, dx, hln, xln, tln, cln;
    int hp, hpmax, cap;
    long money;

    /*
     * Various min(x,9999)'s are to avoid having excessive values
     * violate the field width assumptions in botl.h and should not
     * impact normal play.  Particularly 64-bit long for gold which
     * could require many more digits if someone figures out a way
     * to get and carry a really large (or negative) amount of it.
     * Turn counter is also long, but we'll risk that.
     */

    /* dungeon location plus gold */
    (void) describe_level(dloc); /* includes at least one trailing space */
    if ((money = money_cnt(invent)) < 0L)
        money = 0L; /* ought to issue impossible() and then discard gold */
    Sprintf(eos(dloc), "%s:%-2ld", /* strongest hero can lift ~300000 gold */
            encglyph(objnum_to_glyph(GOLD_PIECE)), min(money, 999999L));
    dln = strlen(dloc);
    /* '$' encoded as \GXXXXNNNN is 9 chars longer than display will need */
    dx = strstri(dloc, "\\G") ? 9 : 0;

    /* health and armor class (has trailing space for AC 0..9) */
    hp = Upolyd ? u.mh : u.uhp;
    hpmax = Upolyd ? u.mhmax : u.uhpmax;
    if (hp < 0)
        hp = 0;
    Sprintf(hlth, "HP:%d(%d) Pw:%d(%d) AC:%-2d",
            min(hp, 9999), min(hpmax, 9999),
            min(u.uen, 9999), min(u.uenmax, 9999), u.uac);
    hln = strlen(hlth);

    /* experience */
    if (Upolyd)
        Sprintf(expr, "HD:%d", mons[u.umonnum].mlevel);
    else if (flags.showexp)
        Sprintf(expr, "Xp:%u/%-1ld", u.ulevel, u.uexp);
    else
        Sprintf(expr, "Exp:%u", u.ulevel);
    xln = strlen(expr);

    /* time/move counter */
    if (flags.time)
        Sprintf(tmmv, "T:%ld", moves);
    else
        tmmv[0] = '\0';
    tln = strlen(tmmv);

    /* status conditions; worst ones first */
    cond[0] = '\0'; /* once non-empty, cond will have a leading space */
    nb = cond;
    /*
     * Stoned, Slimed, Strangled, and both types of Sick are all fatal
     * unless remedied before timeout expires.  Should we order them by
     * shortest time left?  [Probably not worth the effort, since it's
     * unusual for more than one of them to apply at a time.]
     */
    if (Stoned)
        Strcpy(nb = eos(nb), " Stone");
    if (Slimed)
        Strcpy(nb = eos(nb), " Slime");
    if (Strangled)
        Strcpy(nb = eos(nb), " Strngl");
    if (Sick) {
        if (u.usick_type & SICK_VOMITABLE)
            Strcpy(nb = eos(nb), " FoodPois");
        if (u.usick_type & SICK_NONVOMITABLE)
            Strcpy(nb = eos(nb), " TermIll");
    }
    if (u.uhs != NOT_HUNGRY)
        Sprintf(nb = eos(nb), " %s", hu_stat[u.uhs]);
    if ((cap = near_capacity()) > UNENCUMBERED)
        Sprintf(nb = eos(nb), " %s", enc_stat[cap]);
    if (Blind)
        Strcpy(nb = eos(nb), " Blind");
    if (Deaf)
        Strcpy(nb = eos(nb), " Deaf");
    if (Stunned)
        Strcpy(nb = eos(nb), " Stun");
    if (Confusion)
        Strcpy(nb = eos(nb), " Conf");
    if (Hallucination)
        Strcpy(nb = eos(nb), " Hallu");
    /* levitation and flying are mutually exclusive; riding is not */
    if (Levitation)
        Strcpy(nb = eos(nb), " Lev");
    if (Flying)
        Strcpy(nb = eos(nb), " Fly");
    if (u.usteed)
        Strcpy(nb = eos(nb), " Ride");
    cln = strlen(cond);

    /*
     * Put the pieces together.  If they all fit, keep the traditional
     * sequence.  Otherwise, move least important parts to the end in
     * case the interface side of things has to truncate.  Note that
     * dloc[] contains '$' encoded in ten character sequence \GXXXXNNNN
     * so we want to test its display length rather than buffer length.
     *
     * We don't have an actual display limit here, so have to go by the
     * width of the map.  Since we're reordering rather than truncating,
     * wider displays can still show wider status than the map if the
     * interface supports that.
     */
    if ((dln - dx) + 1 + hln + 1 + xln + 1 + tln + 1 + cln <= COLNO) {
        Sprintf(newbot2, "%s %s %s %s %s", dloc, hlth, expr, tmmv, cond);
    } else {
        if (dln + 1 + hln + 1 + xln + 1 + tln + 1 + cln + 1 > MAXCO) {
            panic("bot2: second status line exceeds MAXCO (%u > %d)",
                  (dln + 1 + hln + 1 + xln + 1 + tln + 1 + cln + 1), MAXCO);
        } else if ((dln - dx) + 1 + hln + 1 + xln + 1 + cln <= COLNO) {
            Sprintf(newbot2, "%s %s %s %s %s", dloc, hlth, expr, cond, tmmv);
        } else if ((dln - dx) + 1 + hln + 1 + cln <= COLNO) {
            Sprintf(newbot2, "%s %s %s %s %s", dloc, hlth, cond, expr, tmmv);
        } else {
            Sprintf(newbot2, "%s %s %s %s %s", hlth, cond, dloc, expr, tmmv);
        }
        /* only two or three consecutive spaces available to squeeze out */
        mungspaces(newbot2);
    }

    curs(WIN_STATUS, 1, 1);
    putmixed(WIN_STATUS, 0, newbot2);
}

void
bot()
{
    if (youmonst.data) {
        bot1();
        bot2();
    }
    context.botl = context.botlx = 0;
}

#endif /* !STATUS_VIA_WINDOWPORT */

/* convert experience level (1..30) to rank index (0..8) */
int
xlev_to_rank(xlev)
int xlev;
{
    return (xlev <= 2) ? 0 : (xlev <= 30) ? ((xlev + 2) / 4) : 8;
}

#if 0 /* not currently needed */
/* convert rank index (0..8) to experience level (1..30) */
int
rank_to_xlev(rank)
int rank;
{
    return (rank <= 0) ? 1 : (rank <= 8) ? ((rank * 4) - 2) : 30;
}
#endif

const char *
rank_of(lev, monnum, female)
int lev;
short monnum;
boolean female;
{
    register const struct Role *role;
    register int i;

    /* Find the role */
    for (role = roles; role->name.m; role++)
        if (monnum == role->malenum || monnum == role->femalenum)
            break;
    if (!role->name.m)
        role = &urole;

    /* Find the rank */
    for (i = xlev_to_rank((int) lev); i >= 0; i--) {
        if (female && role->rank[i].f)
            return role->rank[i].f;
        if (role->rank[i].m)
            return role->rank[i].m;
    }

    /* Try the role name, instead */
    if (female && role->name.f)
        return role->name.f;
    else if (role->name.m)
        return role->name.m;
    return "Player";
}

STATIC_OVL const char *
rank()
{
    return rank_of(u.ulevel, Role_switch, flags.female);
}

int
title_to_mon(str, rank_indx, title_length)
const char *str;
int *rank_indx, *title_length;
{
    register int i, j;

    /* Loop through each of the roles */
    for (i = 0; roles[i].name.m; i++)
        for (j = 0; j < 9; j++) {
            if (roles[i].rank[j].m
                && !strncmpi(str, roles[i].rank[j].m,
                             strlen(roles[i].rank[j].m))) {
                if (rank_indx)
                    *rank_indx = j;
                if (title_length)
                    *title_length = strlen(roles[i].rank[j].m);
                return roles[i].malenum;
            }
            if (roles[i].rank[j].f
                && !strncmpi(str, roles[i].rank[j].f,
                             strlen(roles[i].rank[j].f))) {
                if (rank_indx)
                    *rank_indx = j;
                if (title_length)
                    *title_length = strlen(roles[i].rank[j].f);
                return (roles[i].femalenum != NON_PM) ? roles[i].femalenum
                                                      : roles[i].malenum;
            }
        }
    return NON_PM;
}

void
max_rank_sz()
{
    register int i, r, maxr = 0;
    for (i = 0; i < 9; i++) {
        if (urole.rank[i].m && (r = strlen(urole.rank[i].m)) > maxr)
            maxr = r;
        if (urole.rank[i].f && (r = strlen(urole.rank[i].f)) > maxr)
            maxr = r;
    }
    mrank_sz = maxr;
    return;
}

#ifdef SCORE_ON_BOTL
long
botl_score()
{
    long deepest = deepest_lev_reached(FALSE);
    long utotal;

    utotal = money_cnt(invent) + hidden_gold();
    if ((utotal -= u.umoney0) < 0L)
        utotal = 0L;
    utotal += u.urexp + (50 * (deepest - 1))
          + (deepest > 30 ? 10000 : deepest > 20 ? 1000 * (deepest - 20) : 0);
    if (utotal < u.urexp)
        utotal = LONG_MAX; /* wrap around */
    return utotal;
}
#endif /* SCORE_ON_BOTL */

/* provide the name of the current level for display by various ports */
int
describe_level(buf)
char *buf;
{
    int ret = 1;

    /* TODO:    Add in dungeon name */
    if (Is_knox(&u.uz))
        Sprintf(buf, "%s ", dungeons[u.uz.dnum].dname);
    else if (In_quest(&u.uz))
        Sprintf(buf, "Home %d ", dunlev(&u.uz));
    else if (In_endgame(&u.uz))
        Sprintf(buf, Is_astralevel(&u.uz) ? "Astral Plane " : "End Game ");
    else {
        /* ports with more room may expand this one */
        Sprintf(buf, "Dlvl:%-2d ", depth(&u.uz));
        ret = 0;
    }
    return ret;
}

#ifdef STATUS_VIA_WINDOWPORT
/* =======================================================================*/

/* structure that tracks the status details in the core */
struct istat_s {
    long time;
    unsigned anytype;
    anything a;
    char *val;
    int valwidth;
    enum statusfields idxmax;
    enum statusfields fld;
};


STATIC_DCL void NDECL(init_blstats);
STATIC_DCL char *FDECL(anything_to_s, (char *, anything *, int));
STATIC_OVL int FDECL(percentage, (struct istat_s *, struct istat_s *));
STATIC_OVL int FDECL(compare_blstats, (struct istat_s *, struct istat_s *));
#ifdef STATUS_HILITES
STATIC_DCL void FDECL(s_to_anything, (anything *, char *, int));
STATIC_DCL boolean FDECL(assign_hilite, (char *, char *, char *, char *,
                                         BOOLEAN_P));
STATIC_DCL const char *FDECL(clridx_to_s, (char *, int));
#endif

/* If entries are added to this, botl.h will require updating too */
STATIC_DCL struct istat_s initblstats[MAXBLSTATS] = {
    { 0L, ANY_STR,  { (genericptr_t) 0 }, (char *) 0, 80,  0, BL_TITLE},
    { 0L, ANY_INT,  { (genericptr_t) 0 }, (char *) 0, 10,  0, BL_STR},
    { 0L, ANY_INT,  { (genericptr_t) 0 }, (char *) 0, 10,  0, BL_DX},
    { 0L, ANY_INT,  { (genericptr_t) 0 }, (char *) 0, 10,  0, BL_CO},
    { 0L, ANY_INT,  { (genericptr_t) 0 }, (char *) 0, 10,  0, BL_IN},
    { 0L, ANY_INT,  { (genericptr_t) 0 }, (char *) 0, 10,  0, BL_WI},
    { 0L, ANY_INT,  { (genericptr_t) 0 }, (char *) 0, 10,  0, BL_CH},
    { 0L, ANY_STR,  { (genericptr_t) 0 }, (char *) 0, 40,  0, BL_ALIGN},
    { 0L, ANY_LONG, { (genericptr_t) 0 }, (char *) 0, 20,  0, BL_SCORE},
    { 0L, ANY_LONG, { (genericptr_t) 0 }, (char *) 0, 20,  0, BL_CAP},
    { 0L, ANY_LONG, { (genericptr_t) 0 }, (char *) 0, 30,  0, BL_GOLD},
    { 0L, ANY_INT,  { (genericptr_t) 0 }, (char *) 0, 10,  BL_ENEMAX, BL_ENE},
    { 0L, ANY_INT,  { (genericptr_t) 0 }, (char *) 0, 10,  0, BL_ENEMAX},
    { 0L, ANY_LONG, { (genericptr_t) 0 }, (char *) 0, 10,  0, BL_XP},
    { 0L, ANY_INT,  { (genericptr_t) 0 }, (char *) 0, 10,  0, BL_AC},
    { 0L, ANY_INT,  { (genericptr_t) 0 }, (char *) 0, 10,  0, BL_HD},
    { 0L, ANY_INT,  { (genericptr_t) 0 }, (char *) 0, 20,  0, BL_TIME},
    { 0L, ANY_UINT, { (genericptr_t) 0 }, (char *) 0, 40,  0, BL_HUNGER},
    { 0L, ANY_INT,  { (genericptr_t) 0 }, (char *) 0, 10,  BL_HPMAX, BL_HP},
    { 0L, ANY_INT,  { (genericptr_t) 0 }, (char *) 0, 10,  0, BL_HPMAX},
    { 0L, ANY_STR,  { (genericptr_t) 0 }, (char *) 0, 80,  0, BL_LEVELDESC},
    { 0L, ANY_LONG, { (genericptr_t) 0 }, (char *) 0, 20,  0, BL_EXP},
    { 0L, ANY_MASK32,
                    { (genericptr_t) 0 }, (char *) 0,  0,  0, BL_CONDITION}
};

struct istat_s blstats[2][MAXBLSTATS];
static boolean blinit = FALSE, update_all = FALSE;

void
bot()
{
    char buf[BUFSZ];
    register char *nb;
    static int idx = 0, idx_p, idxmax;
    unsigned anytype;
    long money;
    int i, pc, chg, cap;
    struct istat_s *curr, *prev;
    boolean valset[MAXBLSTATS], chgval = FALSE, updated = FALSE;

    if (!blinit)
        panic("bot before init.");
    if (!youmonst.data) {
        context.botl = context.botlx = 0;
        update_all = FALSE;
        return;
    }

    idx_p = idx;
    idx = 1 - idx; /* 0 -> 1, 1 -> 0 */

    /* clear the "value set" indicators */
    (void) memset((genericptr_t) valset, 0, MAXBLSTATS * sizeof (boolean));

    /*
     * Note: min(x,9999) - we enforce the same maximum on hp, maxhp,
     * pw, maxpw, and gold as basic status formatting so that the two
     * modes of status display don't produce different information.
     */

    /*
     *  Player name and title.
     */
    Strcpy(nb = buf, plname);
    nb[0] = highc(nb[0]);
    nb[10] = '\0';
    Sprintf(nb = eos(nb), " the ");
    if (Upolyd) {
        for (i = 0, nb = strcpy(eos(nb), mons[u.umonnum].mname); nb[i]; i++)
            if (i == 0 || nb[i - 1] == ' ')
                nb[i] = highc(nb[i]);
    } else
        Strcpy(nb = eos(nb), rank());
    Sprintf(blstats[idx][BL_TITLE].val, "%-29s", buf);
    valset[BL_TITLE] = TRUE; /* indicate val already set */

    /* Strength */
    buf[0] = '\0';
    blstats[idx][BL_STR].a.a_int = ACURR(A_STR);
    if (ACURR(A_STR) > 18) {
        if (ACURR(A_STR) > STR18(100))
            Sprintf(buf, "%2d", ACURR(A_STR) - 100);
        else if (ACURR(A_STR) < STR18(100))
            Sprintf(buf, "18/%02d", ACURR(A_STR) - 18);
        else
            Sprintf(buf, "18/**");
    } else
        Sprintf(buf, "%-1d", ACURR(A_STR));
    Strcpy(blstats[idx][BL_STR].val, buf);
    valset[BL_STR] = TRUE; /* indicate val already set */

    /*  Dexterity, constitution, intelligence, wisdom, charisma. */
    blstats[idx][BL_DX].a.a_int = ACURR(A_DEX);
    blstats[idx][BL_CO].a.a_int = ACURR(A_CON);
    blstats[idx][BL_IN].a.a_int = ACURR(A_INT);
    blstats[idx][BL_WI].a.a_int = ACURR(A_WIS);
    blstats[idx][BL_CH].a.a_int = ACURR(A_CHA);

    /* Alignment */
    Strcpy(blstats[idx][BL_ALIGN].val, (u.ualign.type == A_CHAOTIC)
                                          ? "Chaotic"
                                          : (u.ualign.type == A_NEUTRAL)
                                               ? "Neutral"
                                               : "Lawful");

    /* Score */
    blstats[idx][BL_SCORE].a.a_long =
#ifdef SCORE_ON_BOTL
        botl_score()
#else
        0L
#endif
        ;

    /*  Hit points  */
    i = Upolyd ? u.mh : u.uhp;
    if (i < 0)
        i = 0;
    blstats[idx][BL_HP].a.a_int = min(i, 9999);
    i = Upolyd ? u.mhmax : u.uhpmax;
    blstats[idx][BL_HPMAX].a.a_int = min(i, 9999);

    /*  Dungeon level. */
    (void) describe_level(blstats[idx][BL_LEVELDESC].val);
    valset[BL_LEVELDESC] = TRUE; /* indicate val already set */

    /* Gold */
    if ((money = money_cnt(invent)) < 0L)
        money = 0L; /* ought to issue impossible() and then discard gold */
    blstats[idx][BL_GOLD].a.a_long = min(money, 999999L);
    /*
     * The tty port needs to display the current symbol for gold
     * as a field header, so to accommodate that we pass gold with
     * that already included. If a window port needs to use the text
     * gold amount without the leading "$:" the port will have to
     * skip past ':' to the value pointer it was passed in status_update()
     * for the BL_GOLD case.
     *
     * Another quirk of BL_GOLD is that the field display may have
     * changed if a new symbol set was loaded, or we entered or left
     * the rogue level.
     *
     * The currency prefix is encoded as ten character \GXXXXNNNN
     * sequence.
     */
    Sprintf(blstats[idx][BL_GOLD].val, "%s:%ld",
            encglyph(objnum_to_glyph(GOLD_PIECE)),
            blstats[idx][BL_GOLD].a.a_long);
    valset[BL_GOLD] = TRUE; /* indicate val already set */

    /* Power (magical energy) */
    blstats[idx][BL_ENE].a.a_int = min(u.uen, 9999);
    blstats[idx][BL_ENEMAX].a.a_int = min(u.uenmax, 9999);

    /* Armor class */
    blstats[idx][BL_AC].a.a_int = u.uac;

    /* Monster level (if Upolyd) */
    blstats[idx][BL_HD].a.a_int = Upolyd ? mons[u.umonnum].mlevel : 0;

    /* Experience */
    blstats[idx][BL_XP].a.a_int = u.ulevel;
    blstats[idx][BL_EXP].a.a_int = u.uexp;

    /* Time (moves) */
    blstats[idx][BL_TIME].a.a_long = moves;

    /* Hunger */
    blstats[idx][BL_HUNGER].a.a_uint = u.uhs;
    Strcpy(blstats[idx][BL_HUNGER].val,
           (u.uhs != NOT_HUNGRY) ? hu_stat[u.uhs] : "");
    valset[BL_HUNGER] = TRUE;

    /* Carrying capacity */
    cap = near_capacity();
    blstats[idx][BL_CAP].a.a_int = cap;
    Strcpy(blstats[idx][BL_CAP].val,
           (cap > UNENCUMBERED) ? enc_stat[cap] : "");
    valset[BL_CAP] = TRUE;

    /* Conditions */
    blstats[idx][BL_CONDITION].a.a_ulong = 0L;
    if (Stoned)
        blstats[idx][BL_CONDITION].a.a_ulong |= BL_MASK_STONE;
    if (Slimed)
        blstats[idx][BL_CONDITION].a.a_ulong |= BL_MASK_SLIME;
    if (Strangled)
        blstats[idx][BL_CONDITION].a.a_ulong |= BL_MASK_STRNGL;
    if (Sick && (u.usick_type & SICK_VOMITABLE) != 0)
        blstats[idx][BL_CONDITION].a.a_ulong |= BL_MASK_FOODPOIS;
    if (Sick && (u.usick_type & SICK_NONVOMITABLE) != 0)
        blstats[idx][BL_CONDITION].a.a_ulong |= BL_MASK_TERMILL;
    /*
     * basic formatting puts hunger status and encumbrance here
     */
    if (Blind)
        blstats[idx][BL_CONDITION].a.a_ulong |= BL_MASK_BLIND;
    if (Deaf)
        blstats[idx][BL_CONDITION].a.a_ulong |= BL_MASK_DEAF;
    if (Stunned)
        blstats[idx][BL_CONDITION].a.a_ulong |= BL_MASK_STUN;
    if (Confusion)
        blstats[idx][BL_CONDITION].a.a_ulong |= BL_MASK_CONF;
    if (Hallucination)
        blstats[idx][BL_CONDITION].a.a_ulong |= BL_MASK_HALLU;
    /* levitation and flying are mututally exclusive */
    if (Levitation)
        blstats[idx][BL_CONDITION].a.a_ulong |= BL_MASK_LEV;
    if (Flying)
        blstats[idx][BL_CONDITION].a.a_ulong |= BL_MASK_FLY;
    if (u.usteed)
        blstats[idx][BL_CONDITION].a.a_ulong |= BL_MASK_RIDE;

    /*
     *  Now pass the changed values to window port.
     */
    for (i = 0; i < MAXBLSTATS; i++) {
        if (((i == BL_SCORE) && !flags.showscore)
            || ((i == BL_EXP) && !flags.showexp)
            || ((i == BL_TIME) && !flags.time)
            || ((i == BL_HD) && !Upolyd)
            || ((i == BL_XP || i == BL_EXP) && Upolyd))
            continue;
        anytype = blstats[idx][i].anytype;
        curr = &blstats[idx][i];
        prev = &blstats[idx_p][i];
        chg = 0;
        if (update_all
            || ((chg = compare_blstats(prev, curr)) != 0)
            || ((chgval = (valset[i]
                           && strcmp(blstats[idx][i].val,
                                     blstats[idx_p][i].val))) != 0)) {
            idxmax = blstats[idx][i].idxmax;
            pc = (idxmax) ? percentage(curr, &blstats[idx][idxmax]) : 0;
            if (!valset[i])
                (void) anything_to_s(curr->val, &curr->a, anytype);
            if (anytype != ANY_MASK32) {
                status_update(i, (genericptr_t) curr->val,
                              valset[i] ? chgval : chg, pc);
            } else {
                /* send pointer to mask */
                status_update(i, (genericptr_t) &curr->a.a_ulong, chg, 0);
            }
            updated = TRUE;
        }
    }
    /*
     * It is possible to get here, with nothing having been pushed
     * to the window port, when none of the info has changed. In that
     * case, we need to force a call to status_update() when
     * context.botlx is set. The tty port in particular has a problem
     * if that isn't done, since it sets context.botlx when a menu or
     * text display obliterates the status line.
     *
     * To work around it, we call status_update() with fictitious
     * index of BL_FLUSH (-1).
     */
    if ((context.botlx && !updated)
        || windowprocs.win_status_update == genl_status_update)
        status_update(BL_FLUSH, (genericptr_t) 0, 0, 0);

    context.botl = context.botlx = 0;
    update_all = FALSE;
}

void
status_initialize(reassessment)
boolean
    reassessment; /* TRUE = just reassess fields w/o other initialization*/
{
    int i;
    const char *fieldfmt = (const char *) 0;
    const char *fieldname = (const char *) 0;

    if (!reassessment) {
        init_blstats();
        (*windowprocs.win_status_init)();
        blinit = TRUE;
#ifdef STATUS_HILITES
        status_notify_windowport(TRUE);
#endif
    }
    for (i = 0; i < MAXBLSTATS; ++i) {
        enum statusfields fld = initblstats[i].fld;

        switch (fld) {
        case BL_TITLE:
            fieldfmt = "%s";
            fieldname = "title";
            status_enablefield(fld, fieldname, fieldfmt, TRUE);
            break;
        case BL_STR:
            fieldfmt = " St:%s";
            fieldname = "strength";
            status_enablefield(fld, fieldname, fieldfmt, TRUE);
            break;
        case BL_DX:
            fieldfmt = " Dx:%s";
            fieldname = "dexterity";
            status_enablefield(fld, fieldname, fieldfmt, TRUE);
            break;
        case BL_CO:
            fieldfmt = " Co:%s";
            fieldname = "constitution";
            status_enablefield(fld, fieldname, fieldfmt, TRUE);
            break;
        case BL_IN:
            fieldfmt = " In:%s";
            fieldname = "intelligence";
            status_enablefield(fld, fieldname, fieldfmt, TRUE);
            break;
        case BL_WI:
            fieldfmt = " Wi:%s";
            fieldname = "wisdom";
            status_enablefield(fld, fieldname, fieldfmt, TRUE);
            break;
        case BL_CH:
            fieldfmt = " Ch:%s";
            fieldname = "charisma";
            status_enablefield(fld, fieldname, fieldfmt, TRUE);
            break;
        case BL_ALIGN:
            fieldfmt = " %s";
            fieldname = "alignment";
            status_enablefield(fld, fieldname, fieldfmt, TRUE);
            break;
        case BL_SCORE:
            fieldfmt = " S:%s";
            fieldname = "score";
            status_enablefield(fld, fieldname, fieldfmt,
                               (!flags.showscore) ? FALSE : TRUE);
            break;
        case BL_CAP:
            fieldfmt = " %s";
            fieldname = "carrying-capacity";
            status_enablefield(fld, fieldname, fieldfmt, TRUE);
            break;
        case BL_GOLD:
            fieldfmt = " %s";
            fieldname = "gold";
            status_enablefield(fld, fieldname, fieldfmt, TRUE);
            break;
        case BL_ENE:
            fieldfmt = " Pw:%s";
            fieldname = "power";
            status_enablefield(fld, fieldname, fieldfmt, TRUE);
            break;
        case BL_ENEMAX:
            fieldfmt = "(%s)";
            fieldname = "power-max";
            status_enablefield(fld, fieldname, fieldfmt, TRUE);
            break;
        case BL_XP:
            fieldfmt = " Xp:%s";
            fieldname = "experience-level";
            status_enablefield(fld, fieldname, fieldfmt,
                                   (Upolyd) ? FALSE : TRUE);
            break;
        case BL_AC:
            fieldfmt = " AC:%s";
            fieldname = "armor-class";
            status_enablefield(fld, fieldname, fieldfmt, TRUE);
            break;
        case BL_HD:
            fieldfmt = " HD:%s";
            fieldname = "HD";
            status_enablefield(fld, fieldname, fieldfmt,
                                   (!Upolyd) ? FALSE : TRUE);
            break;
        case BL_TIME:
            fieldfmt = " T:%s";
            fieldname = "time";
            status_enablefield(fld, fieldname, fieldfmt,
                                   (!flags.time) ? FALSE : TRUE);
            break;
        case BL_HUNGER:
            fieldfmt = " %s";
            fieldname = "hunger";
            status_enablefield(fld, fieldname, fieldfmt, TRUE);
            break;
        case BL_HP:
            fieldfmt = " HP:%s";
            fieldname = "hitpoints";
            status_enablefield(fld, fieldname, fieldfmt, TRUE);
            break;
        case BL_HPMAX:
            fieldfmt = "(%s)";
            fieldname = "hitpoint-max";
            status_enablefield(fld, fieldname, fieldfmt, TRUE);
            break;
        case BL_LEVELDESC:
            fieldfmt = "%s";
            fieldname = "dungeon-level";
            status_enablefield(fld, fieldname, fieldfmt, TRUE);
            break;
        case BL_EXP:
            fieldfmt = "/%s";
            fieldname = "experience";
            status_enablefield(fld, fieldname, fieldfmt,
                                  (!flags.showexp || Upolyd) ? FALSE : TRUE);
            break;
        case BL_CONDITION:
            fieldfmt = "%s";
            fieldname = "condition";
            status_enablefield(fld, fieldname, fieldfmt, TRUE);
            break;
        case BL_FLUSH:
        default:
            break;
        }
    }
    update_all = TRUE;
}

void
status_finish()
{
    int i;

    /* call the window port cleanup routine first */
    (*windowprocs.win_status_finish)();

    /* free memory that we alloc'd now */
    for (i = 0; i < MAXBLSTATS; ++i) {
        if (blstats[0][i].val)
            free((genericptr_t) blstats[0][i].val);
        if (blstats[1][i].val)
            free((genericptr_t) blstats[1][i].val);
    }
}

STATIC_OVL void
init_blstats()
{
    static boolean initalready = FALSE;
    int i, j;

    if (initalready) {
        impossible("init_blstats called more than once.");
        return;
    }

    initalready = TRUE;
    for (i = BEFORE; i <= NOW; ++i) {
        for (j = 0; j < MAXBLSTATS; ++j) {
            blstats[i][j] = initblstats[j];
            blstats[i][j].a = zeroany;
            if (blstats[i][j].valwidth) {
                blstats[i][j].val = (char *) alloc(blstats[i][j].valwidth);
                blstats[i][j].val[0] = '\0';
            } else
                blstats[i][j].val = (char *) 0;
        }
    }
}

STATIC_OVL char *
anything_to_s(buf, a, anytype)
char *buf;
anything *a;
int anytype;
{
    if (!buf)
        return (char *) 0;

    switch (anytype) {
    case ANY_ULONG:
        Sprintf(buf, "%lu", a->a_ulong);
        break;
    case ANY_MASK32:
        Sprintf(buf, "%lx", a->a_ulong);
        break;
    case ANY_LONG:
        Sprintf(buf, "%ld", a->a_long);
        break;
    case ANY_INT:
        Sprintf(buf, "%d", a->a_int);
        break;
    case ANY_UINT:
        Sprintf(buf, "%u", a->a_uint);
        break;
    case ANY_IPTR:
        Sprintf(buf, "%d", *a->a_iptr);
        break;
    case ANY_LPTR:
        Sprintf(buf, "%ld", *a->a_lptr);
        break;
    case ANY_ULPTR:
        Sprintf(buf, "%lu", *a->a_ulptr);
        break;
    case ANY_UPTR:
        Sprintf(buf, "%u", *a->a_uptr);
        break;
    case ANY_STR: /* do nothing */
        ;
        break;
    default:
        buf[0] = '\0';
    }
    return buf;
}

#ifdef STATUS_HILITES

STATIC_OVL void
s_to_anything(a, buf, anytype)
anything *a;
char *buf;
int anytype;
{
    if (!buf || !a)
        return;

    switch (anytype) {
    case ANY_LONG:
        a->a_long = atol(buf);
        break;
    case ANY_INT:
        a->a_int = atoi(buf);
        break;
    case ANY_UINT:
        a->a_uint = (unsigned) atoi(buf);
        break;
    case ANY_ULONG:
        a->a_ulong = (unsigned long) atol(buf);
        break;
    case ANY_IPTR:
        if (a->a_iptr)
            *a->a_iptr = atoi(buf);
        break;
    case ANY_UPTR:
        if (a->a_uptr)
            *a->a_uptr = (unsigned) atoi(buf);
        break;
    case ANY_LPTR:
        if (a->a_lptr)
            *a->a_lptr = atol(buf);
        break;
    case ANY_ULPTR:
        if (a->a_ulptr)
            *a->a_ulptr = (unsigned long) atol(buf);
        break;
    case ANY_MASK32:
        a->a_ulong = (unsigned long) atol(buf);
        break;
    default:
        a->a_void = 0;
        break;
    }
    return;
}

#endif

STATIC_OVL int
compare_blstats(bl1, bl2)
struct istat_s *bl1, *bl2;
{
    int anytype, result = 0;

    if (!bl1 || !bl2) {
        panic("compare_blstat: bad istat pointer %s, %s",
              fmt_ptr((genericptr_t) bl1), fmt_ptr((genericptr_t) bl2));
    }

    anytype = bl1->anytype;
    if ((!bl1->a.a_void || !bl2->a.a_void)
        && (anytype == ANY_IPTR || anytype == ANY_UPTR || anytype == ANY_LPTR
            || anytype == ANY_ULPTR)) {
        panic("compare_blstat: invalid pointer %s, %s",
              fmt_ptr((genericptr_t) bl1->a.a_void),
              fmt_ptr((genericptr_t) bl2->a.a_void));
    }

    switch (anytype) {
    case ANY_INT:
        result = (bl1->a.a_int < bl2->a.a_int)
                     ? 1
                     : (bl1->a.a_int > bl2->a.a_int) ? -1 : 0;
        break;
    case ANY_IPTR:
        result = (*bl1->a.a_iptr < *bl2->a.a_iptr)
                     ? 1
                     : (*bl1->a.a_iptr > *bl2->a.a_iptr) ? -1 : 0;
        break;
    case ANY_LONG:
        result = (bl1->a.a_long < bl2->a.a_long)
                     ? 1
                     : (bl1->a.a_long > bl2->a.a_long) ? -1 : 0;
        break;
    case ANY_LPTR:
        result = (*bl1->a.a_lptr < *bl2->a.a_lptr)
                     ? 1
                     : (*bl1->a.a_lptr > *bl2->a.a_lptr) ? -1 : 0;
        break;
    case ANY_UINT:
        result = (bl1->a.a_uint < bl2->a.a_uint)
                     ? 1
                     : (bl1->a.a_uint > bl2->a.a_uint) ? -1 : 0;
        break;
    case ANY_UPTR:
        result = (*bl1->a.a_uptr < *bl2->a.a_uptr)
                     ? 1
                     : (*bl1->a.a_uptr > *bl2->a.a_uptr) ? -1 : 0;
        break;
    case ANY_ULONG:
        result = (bl1->a.a_ulong < bl2->a.a_ulong)
                     ? 1
                     : (bl1->a.a_ulong > bl2->a.a_ulong) ? -1 : 0;
        break;
    case ANY_ULPTR:
        result = (*bl1->a.a_ulptr < *bl2->a.a_ulptr)
                     ? 1
                     : (*bl1->a.a_ulptr > *bl2->a.a_ulptr) ? -1 : 0;
        break;
    case ANY_STR:
        if (strcmp(bl1->val, bl2->val) == 0)
            result = 0;
        else
            result = 1;
        break;
    case ANY_MASK32:
        if (bl1->a.a_ulong == bl2->a.a_ulong)
            result = 0;
        else
            result = 1;
        break;
    default:
        result = 1;
    }
    return result;
}

STATIC_OVL int
percentage(bl, maxbl)
struct istat_s *bl, *maxbl;
{
    int result = 0;
    int anytype;

    if (!bl || !maxbl) {
        impossible("percentage: bad istat pointer %s, %s",
                   fmt_ptr((genericptr_t) bl), fmt_ptr((genericptr_t) maxbl));
        return 0;
    }

    anytype = bl->anytype;
    if (maxbl->a.a_void) {
        switch (anytype) {
        case ANY_INT:
            result = ((100 * bl->a.a_int) / maxbl->a.a_int);
            break;
        case ANY_LONG:
            result = (int) ((100L * bl->a.a_long) / maxbl->a.a_long);
            break;
        case ANY_UINT:
            result = (int) ((100U * bl->a.a_uint) / maxbl->a.a_uint);
            break;
        case ANY_ULONG:
            result = (int) ((100UL * bl->a.a_ulong) / maxbl->a.a_ulong);
            break;
        case ANY_IPTR:
            result = ((100 * (*bl->a.a_iptr)) / (*maxbl->a.a_iptr));
            break;
        case ANY_LPTR:
            result = (int) ((100L * (*bl->a.a_lptr)) / (*maxbl->a.a_lptr));
            break;
        case ANY_UPTR:
            result = (int) ((100U * (*bl->a.a_uptr)) / (*maxbl->a.a_uptr));
            break;
        case ANY_ULPTR:
            result = (int) ((100UL * (*bl->a.a_ulptr)) / (*maxbl->a.a_ulptr));
            break;
        }
    }
    return result;
}


#ifdef STATUS_HILITES

/****************************************************************************/
/* Core status hiliting support */
/****************************************************************************/

static struct fieldid_t {
    const char *fieldname;
    enum statusfields fldid;
} fieldids[] = {
    {"title",               BL_TITLE},
    {"strength",            BL_STR},
    {"dexterity",           BL_DX},
    {"constitution",        BL_CO},
    {"intelligence",        BL_IN},
    {"wisdom",              BL_WI},
    {"charisma",            BL_CH},
    {"alignment",           BL_ALIGN},
    {"score",               BL_SCORE},
    {"carrying-capacity",   BL_CAP},
    {"gold",                BL_GOLD},
    {"power",               BL_ENE},
    {"power-max",           BL_ENEMAX},
    {"experience-level",    BL_XP},
    {"armor-class",         BL_AC},
    {"HD",                  BL_HD},
    {"time",                BL_TIME},
    {"hunger",              BL_HUNGER},
    {"hitpoints",           BL_HP},
    {"hitpoints-max",       BL_HPMAX},
    {"dungeon-level",       BL_LEVELDESC},
    {"experience",          BL_EXP},
    {"condition",           BL_CONDITION},
};

struct hilite_s {
    boolean set;
    unsigned anytype;
    anything threshold;
    int behavior;
    int coloridx[2];
};

struct hilite_s status_hilites[MAXBLSTATS];

/*
 * This is the parser for the hilite options
 * Example:
 *    OPTION=hilite_status: hitpoints/10%/red/normal
 *
 * set_hilite_status() separates each hilite entry into its 4 component
 * strings, then calls assign_hilite() to make the adjustments.
 */
boolean
set_status_hilites(op, from_configfile)
char *op;
boolean from_configfile;
{
    char hsbuf[4][QBUFSZ];
    boolean rslt, badopt = FALSE;
    int fldnum, num = 0, ccount = 0;
    char c;

    num = fldnum = 0;
    hsbuf[0][0] = hsbuf[1][0] = hsbuf[2][0] = hsbuf[3][0] = '\0';
    while (*op && fldnum < 4 && ccount < (QBUFSZ - 2)) {
        c = lowc(*op);
        if (c == ' ') {
            if (fldnum >= 2) {
                rslt = assign_hilite(&hsbuf[0][0], &hsbuf[1][0], &hsbuf[2][0],
                                     &hsbuf[3][0], from_configfile);
                if (!rslt) {
                    badopt = TRUE;
                    break;
                }
            }
            hsbuf[0][0] = hsbuf[1][0] = '\0';
            hsbuf[2][0] = hsbuf[3][0] = '\0';
            fldnum = 0;
            ccount = 0;
        } else if (c == '/') {
            fldnum++;
            ccount = 0;
        } else {
            hsbuf[fldnum][ccount++] = c;
            hsbuf[fldnum][ccount] = '\0';
        }
        op++;
    }
    if (fldnum >= 2 && !badopt) {
        rslt = assign_hilite(&hsbuf[0][0], &hsbuf[1][0], &hsbuf[2][0],
                             &hsbuf[3][0], from_configfile);
        if (!rslt)
            badopt = TRUE;
    }
    if (badopt)
        return FALSE;
    return TRUE;
}

void
clear_status_hilites(from_configfile)
boolean from_configfile;
{
    int i;
    anything it;

    it = zeroany;
    for (i = 0; i < MAXBLSTATS; ++i) {
        (void) memset((genericptr_t) &status_hilites[i], 0,
                      sizeof(struct hilite_s));
        /* notify window port */
        if (!from_configfile)
            status_threshold(i, blstats[0][i].anytype, it, 0, 0, 0);
    }
}

STATIC_OVL boolean
assign_hilite(sa, sb, sc, sd, from_configfile)
char *sa, *sb, *sc, *sd;
boolean from_configfile;
{
    char *tmp, *how;
    int i = -1, dt = -1, idx = -1;
    int coloridx[2] = { -1, -1 };
    boolean inverse[2] = { FALSE, FALSE };
    boolean bold[2] = { FALSE, FALSE };
    boolean normal[2] = { 0, 0 };
    boolean percent = FALSE, down_up = FALSE, changed = FALSE;
    anything threshold;
    enum statusfields fld = BL_FLUSH;
    threshold.a_void = 0;

    /* Example:
     *  hilite_status: hitpoints/10%/red/normal
     */

    /* field name to statusfield */
    for (i = 0; sa && i < SIZE(fieldids); ++i) {
        if (strcmpi(sa, fieldids[i].fieldname) == 0) {
            idx = i;
            fld = fieldids[i].fldid;
            break;
        }
    }
    if (idx == -1)
        return FALSE;
    status_hilites[idx].set = FALSE; /* mark it "unset" */

    /* threshold */
    if (!sb)
        return FALSE;
    if ((strcmpi(sb, "updown") == 0) || (strcmpi(sb, "downup") == 0)
        || (strcmpi(sb, "up") == 0) || (strcmpi(sb, "down") == 0)) {
        down_up = TRUE;
    } else if ((strcmpi(sb, "changed") == 0)
               && (fld == BL_TITLE || fld == BL_ALIGN || fld == BL_LEVELDESC
                   || fld == BL_CONDITION)) {
        changed = TRUE; /* changed is only thing allowed */
    } else {
        tmp = sb;
        while (*tmp) {
            if (*tmp == '%') {
                *tmp = '\0';
                percent = TRUE;
                break;
            } else if (!index("0123456789", *tmp))
                return FALSE;
            tmp++;
        }
        if (strlen(sb) > 0) {
            dt = blstats[0][idx].anytype;
            if (percent)
                dt = ANY_INT;
            (void) s_to_anything(&threshold, sb, dt);
        } else
            return FALSE;
        if (percent && (threshold.a_int < 1 || threshold.a_int > 100))
            return FALSE;
        if (!threshold.a_void && (strcmp(sb, "0") != 0))
            return FALSE;
    }

    /* actions */
    for (i = 0; i < 2; ++i) {
        how = !i ? sc : sd;
        if (!how) {
            if (!i)
                return FALSE;
            break; /* sc is mandatory; sd is not */
        }

        if (strcmpi(how, "bold") == 0) {
            bold[i] = TRUE;
        } else if (strcmpi(how, "inverse") == 0) {
            inverse[i] = TRUE;
        } else if (strcmpi(how, "normal") == 0) {
            normal[i] = TRUE;
        } else {
            int k = match_str2clr(how);

            if (k >= CLR_MAX)
                return FALSE;
            coloridx[i] = k;
        }
    }

    /* Assign the values */

    for (i = 0; i < 2; ++i) {
        if (inverse[i])
            status_hilites[idx].coloridx[i] = BL_HILITE_INVERSE;
        else if (bold[i])
            status_hilites[idx].coloridx[i] = BL_HILITE_BOLD;
        else if (coloridx[i])
            status_hilites[idx].coloridx[i] = coloridx[i];
        else
            status_hilites[idx].coloridx[i] = BL_HILITE_NONE;
    }

    if (percent)
        status_hilites[idx].behavior = BL_TH_VAL_PERCENTAGE;
    else if (down_up)
        status_hilites[idx].behavior = BL_TH_UPDOWN;
    else if (threshold.a_void)
        status_hilites[idx].behavior = BL_TH_VAL_ABSOLUTE;
    else
        status_hilites[idx].behavior = BL_TH_NONE;

    if (status_hilites[idx].behavior != BL_TH_NONE) {
        status_hilites[idx].threshold = threshold;
        status_hilites[idx].set = TRUE;
    }
    status_hilites[idx].anytype = dt;

    /* Now finally, we notify the window port */
    if (!from_configfile)
        status_threshold(idx, status_hilites[idx].anytype,
                         status_hilites[idx].threshold,
                         status_hilites[idx].behavior,
                         status_hilites[idx].coloridx[0],
                         status_hilites[idx].coloridx[1]);

    return TRUE;
}

/*ARGUSED*/
void
status_notify_windowport(all)
boolean all UNUSED;
{
    int idx;
    anything it;

    it = zeroany;
    for (idx = 0; idx < MAXBLSTATS; ++idx) {
        if (status_hilites[idx].set)
            status_threshold(idx, status_hilites[idx].anytype,
                             status_hilites[idx].threshold,
                             status_hilites[idx].behavior,
                             status_hilites[idx].coloridx[0],
                             status_hilites[idx].coloridx[1]);
        else
            status_threshold(idx, blstats[0][idx].anytype, it, 0, 0, 0);

    }
}

/*
 * get_status_hilites
 *
 * Returns a string containing all the status hilites in the
 * same format that is used to specify a status hilite preference
 * in the config file.
 */
char *
get_status_hilites(buf, bufsiz)
char *buf;
int bufsiz;
{
    int i, j, k, coloridx;
    const char *text = (char *) 0;
    char tmp[BUFSZ], colorname[BUFSZ];
    boolean val_percentage, val_absolute, up_down;
    boolean added_one = FALSE;

    if (!buf)
        return (char *) 0;
    *buf = '\0';

    bufsiz--; /* required trailing null */
    for (i = 0; i < MAXBLSTATS; ++i) {
        val_percentage = val_absolute = up_down = FALSE;
        if (status_hilites[i].set) {
            if (!added_one)
                added_one = TRUE;
            else {
                Strcat(buf, " ");
                bufsiz--;
            }
            k = strlen(fieldids[i].fieldname);
            if (k < bufsiz) {
                Strcat(buf, fieldids[i].fieldname);
                bufsiz -= k;
            }
            if (bufsiz > 1) {
                Strcat(buf, "/");
                bufsiz--;
            }
            if (status_hilites[i].behavior == BL_TH_VAL_PERCENTAGE) {
                val_percentage = TRUE;
            } else if (status_hilites[i].behavior == BL_TH_VAL_ABSOLUTE) {
                val_absolute = TRUE;
            } else if (status_hilites[i].behavior == BL_TH_UPDOWN) {
                up_down = TRUE;
                text = "updown";
            }

            if (status_hilites[i].behavior != BL_TH_UPDOWN) {
                anything_to_s(tmp, &status_hilites[i].threshold,
                          blstats[0][i].anytype);
                text = tmp;
            }
            k = strlen(text);
            if (k < (bufsiz - 1)) {
                Strcat(buf, text);
                if (val_percentage)
                    Strcat(buf, "%"), k++;
                bufsiz -= k;
            }
            for (j = 0; j < 2; ++j) {
                if (bufsiz > 1) {
                    Strcat(buf, "/");
                    bufsiz--;
                }
                coloridx = status_hilites[i].coloridx[j];
                if (coloridx < 0) {
                    if (coloridx == BL_HILITE_BOLD)
                        text = "bold";
                    else if (coloridx == BL_HILITE_INVERSE)
                        text = "inverse";
                    else
                        text = "normal";
                } else {
                    char *blank;

                    (void) strcpy(colorname, c_obj_colors[coloridx]);
                    for (blank = index(colorname, ' '); blank;
                         blank = index(colorname, ' '))
                        *blank = '-';
                    text = colorname;
                }
                k = strlen(text);
                if (k < bufsiz) {
                    Strcat(buf, text);
                    bufsiz -= k;
                }
            }
        }
    }
    return buf;
}

STATIC_OVL const char *
clridx_to_s(buf, idx)
char *buf;
int idx;
{
    static const char *a[] = { "bold", "inverse", "normal" };
    char* p = 0;

    if (buf) {
        buf[0] = '\0';
        if (idx < 0 && idx >= BL_HILITE_BOLD)
            Strcpy(buf, a[idx + 3]);
        else if (idx >= 0 && idx < CLR_MAX)
            Strcpy(buf, c_obj_colors[idx]);
        /* replace spaces with - */
        for(p = buf; *p; p++)
            if(*p == ' ') *p = '-';
    }
    return buf;
}

boolean
status_hilite_menu()
{
    int i, j, k, pick_cnt, pick_idx, opt_idx;
    menu_item *statfield_picks = (menu_item *) 0;
    const char *fieldname;
    int field_picks[MAXBLSTATS], res;
    struct hilite_s hltemp[MAXBLSTATS];
    char buf[BUFSZ], thresholdbuf[BUFSZ], below[BUFSZ], above[BUFSZ];
    winid tmpwin;
    anything any;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin);
    for (i = 0; i < MAXBLSTATS; i++) {
        (void) memset(&hltemp[i], 0, sizeof(struct hilite_s));
        fieldname = fieldids[i].fieldname;
        any.a_int = i + 1;
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, fieldname,
                 MENU_UNSELECTED);
        field_picks[i] = 0;
    }
    end_menu(tmpwin, "Change hilite on which status field(s):");
    if ((pick_cnt = select_menu(tmpwin, PICK_ANY, &statfield_picks)) > 0) {
        for (pick_idx = 0; pick_idx < pick_cnt; ++pick_idx) {
            opt_idx = statfield_picks[pick_idx].item.a_int - 1;
            field_picks[opt_idx] = 1;
        }
        free((genericptr_t) statfield_picks);
        statfield_picks = (menu_item *) 0;
    }
    destroy_nhwindow(tmpwin);
    if (pick_cnt < 0)
        return FALSE;

    for (i = 0; i < MAXBLSTATS; i++) {
        if (field_picks[i]) {
            menu_item *pick = (menu_item *) 0;

            Sprintf(buf, "Threshold behavior options for %s:",
                    fieldids[i].fieldname);
            tmpwin = create_nhwindow(NHW_MENU);
            start_menu(tmpwin);
            if (i == BL_CONDITION) {
                any = zeroany;
                any.a_int = BL_TH_CONDITION + 1;
                add_menu(tmpwin, NO_GLYPH, &any, 'c', 0, ATR_NONE,
                         "Condition bitmask threshold.", MENU_UNSELECTED);
            }
            any = zeroany;
            any.a_int = BL_TH_NONE + 1;
            add_menu(tmpwin, NO_GLYPH, &any, 'n', 0, ATR_NONE, "None",
                     MENU_UNSELECTED);
            if (i != BL_CONDITION) {
                if (blstats[0][i].idxmax > 0) {
                    any = zeroany;
                    any.a_int = BL_TH_VAL_PERCENTAGE + 1;
                    add_menu(tmpwin, NO_GLYPH, &any, 'p', 0, ATR_NONE,
                             "Percentage threshold.", MENU_UNSELECTED);
                }
                any = zeroany;
                any.a_int = BL_TH_UPDOWN + 1;
                add_menu(tmpwin, NO_GLYPH, &any, 'u', 0, ATR_NONE,
                         "UpDown threshold.", MENU_UNSELECTED);
                any = zeroany;
                any.a_int = BL_TH_VAL_ABSOLUTE + 1;
                add_menu(tmpwin, NO_GLYPH, &any, 'v', 0, ATR_NONE,
                         "Value threshold.", MENU_UNSELECTED);
            }
            end_menu(tmpwin, buf);
            if ((res = select_menu(tmpwin, PICK_ONE, &pick)) > 0) {
                hltemp[i].behavior = pick->item.a_int - 1;
                free((genericptr_t) pick);
            }
            destroy_nhwindow(tmpwin);
            if (res < 0)
                return FALSE;

            if (hltemp[i].behavior == BL_TH_UPDOWN) {
                Sprintf(below, "%s decreases", fieldids[i].fieldname);
                Sprintf(above, "%s increases", fieldids[i].fieldname);
            } else if (hltemp[i].behavior) {
                /* Have them enter the threshold*/
                Sprintf(
                    buf, "Set %s threshold to what%s?", fieldids[i].fieldname,
                    (hltemp[i].behavior == BL_TH_VAL_PERCENTAGE)
                        ? " percentage"
                        : (hltemp[i].behavior == BL_TH_CONDITION) ? " mask"
                                                                  : "");
                getlin(buf, thresholdbuf);
                if (thresholdbuf[0] == '\033')
                    return FALSE;
                (void) s_to_anything(&hltemp[i].threshold, thresholdbuf,
                                     blstats[0][i].anytype);
                if (!hltemp[i].threshold.a_void)
                    return FALSE;

                Sprintf(below, "%s falls below %s%s", fieldids[i].fieldname,
                        thresholdbuf,
                        (hltemp[i].behavior == BL_TH_VAL_PERCENTAGE) ? "%"
                                                                     : "");
                Sprintf(above, "%s rises above %s%s", fieldids[i].fieldname,
                        thresholdbuf,
                        (hltemp[i].behavior == BL_TH_VAL_PERCENTAGE) ? "%"
                                                                     : "");
            }
            for (j = 0; j < 2 && (hltemp[i].behavior != BL_TH_NONE); ++j) {
                char prompt[QBUFSZ];
                /* j == 0 below, j == 1 above */
                menu_item *pick2 = (menu_item *) 0;

                Sprintf(prompt, "Display how when %s?", j ? above : below);
                tmpwin = create_nhwindow(NHW_MENU);
                start_menu(tmpwin);
                for (k = -3; k < CLR_MAX; ++k) {
                    /* if (k == -1) continue; */
                    any = zeroany;
                    any.a_int = (k >= 0) ? k + 1 : k;
                    if (k > 0)
                        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
                                 c_obj_colors[k], MENU_UNSELECTED);
                    else if (k == -1)
                        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
                                 "normal", MENU_UNSELECTED);
                    else if (k == -2)
                        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
                                 "inverse", MENU_UNSELECTED);
                    else if (k == -3)
                        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
                                 "bold", MENU_UNSELECTED);
                }
                end_menu(tmpwin, prompt);
                if ((res = select_menu(tmpwin, PICK_ONE, &pick2)) > 0) {
                    hltemp[i].coloridx[j] = (pick2->item.a_char > 0)
                                                ? pick2->item.a_int - 1
                                                : pick2->item.a_int;
                    free((genericptr_t) pick2);
                }
                destroy_nhwindow(tmpwin);
                if (res < 0)
                    return FALSE;
            }
        }
    }
    buf[0] = '\0';
    for (i = 0; i < MAXBLSTATS; i++) {
        if (field_picks[i]) {
            Sprintf(eos(buf), "%s/%s%s/", fieldids[i].fieldname,
                    (hltemp[i].behavior == BL_TH_UPDOWN)
                        ? "updown"
                        : anything_to_s(thresholdbuf, &hltemp[i].threshold,
                                        blstats[0][i].anytype),
                    (hltemp[i].behavior == BL_TH_VAL_PERCENTAGE) ? "%" : "");
            /* borrow thresholdbuf for use with these last two */
            Sprintf(eos(buf), "%s/",
                    clridx_to_s(thresholdbuf, hltemp[i].coloridx[0]));
            Sprintf(eos(buf), "%s ",
                    clridx_to_s(thresholdbuf, hltemp[i].coloridx[1]));
        }
    }
    return set_status_hilites(buf, FALSE);
}
#endif /*STATUS_HILITES*/
#endif /*STATUS_VIA_WINDOWPORT*/

/*botl.c*/
