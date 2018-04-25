/* NetHack 3.6	botl.c	$NHDT-Date: 1506903619 2017/10/02 00:20:19 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.81 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2006. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include <limits.h>

extern const char *hu_stat[]; /* defined in eat.c */

const char *const enc_stat[] = { "",         "Burdened",  "Stressed",
                                 "Strained", "Overtaxed", "Overloaded" };

STATIC_OVL NEARDATA int mrank_sz = 0; /* loaded by max_rank_sz (from u_init) */
STATIC_DCL const char *NDECL(rank);
#ifdef STATUS_HILITES
STATIC_DCL void NDECL(bot_via_windowport);
#endif

static char *
get_strength_str()
{
    static char buf[32];
    int st = ACURR(A_STR);

    if (st > 18) {
        if (st > STR18(100))
            Sprintf(buf, "%2d", st - 100);
        else if (st < STR18(100))
            Sprintf(buf, "18/%02d", st - 18);
        else
            Sprintf(buf, "18/**");
    } else
        Sprintf(buf, "%-1d", st);

    return buf;
}

char *
do_statusline1()
{
    static char newbot1[BUFSZ];
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

    Sprintf(nb = eos(nb), "St:%s Dx:%-1d Co:%-1d In:%-1d Wi:%-1d Ch:%-1d",
            get_strength_str(),
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
    return newbot1;
}

void
check_gold_symbol()
{
    int goldch, goldoc;
    unsigned int goldos;
    int goldglyph = objnum_to_glyph(GOLD_PIECE);
    (void) mapglyph(goldglyph, &goldch, &goldoc, &goldos, 0, 0);
    iflags.invis_goldsym = ((char)goldch <= ' ');
}

char *
do_statusline2()
{
    static char newbot2[BUFSZ], /* MAXCO: botl.h */
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
            (iflags.in_dumplog || iflags.invis_goldsym) ? "$"
            : encglyph(objnum_to_glyph(GOLD_PIECE)),
            min(money, 999999L));
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
    return newbot2;
}

void
bot()
{
    if (youmonst.data && iflags.status_updates) {
#ifdef STATUS_HILITES
        bot_via_windowport();
#else
        curs(WIN_STATUS, 1, 0);
        putstr(WIN_STATUS, 0, do_statusline1());
        curs(WIN_STATUS, 1, 1);
        putmixed(WIN_STATUS, 0, do_statusline2());
#endif
    }
    context.botl = context.botlx = 0;
}

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

/* =======================================================================*/
/*  statusnew routines                                                    */
/* =======================================================================*/

/* structure that tracks the status details in the core */

#ifdef STATUS_HILITES
struct hilite_s {
    enum statusfields fld;
    boolean set;
    unsigned anytype;
    anything value;
    int behavior;
    char textmatch[QBUFSZ];
    enum relationships rel;
    int coloridx;
    struct hilite_s *next;
};

struct condmap {
    const char *id;
    unsigned long bitmask;
};
#endif /* STATUS_HILITES */

struct istat_s {
    const char *fldname;
    const char *fldfmt;
    long time;  /* moves when this field hilite times out */
    boolean chg; /* need to recalc time? */
    unsigned anytype;
    anything a;
    char *val;
    int valwidth;
    enum statusfields idxmax;
    enum statusfields fld;
#ifdef STATUS_HILITES
    struct hilite_s *thresholds;
#endif
};


STATIC_DCL void NDECL(init_blstats);
STATIC_DCL char *FDECL(anything_to_s, (char *, anything *, int));
STATIC_OVL int FDECL(percentage, (struct istat_s *, struct istat_s *));
STATIC_OVL int FDECL(compare_blstats, (struct istat_s *, struct istat_s *));
STATIC_DCL boolean FDECL(evaluate_and_notify_windowport_field,
                         (int, boolean *, int, int));
STATIC_DCL void FDECL(evaluate_and_notify_windowport, (boolean *, int, int));

#ifdef STATUS_HILITES
STATIC_DCL boolean FDECL(hilite_reset_needed, (struct istat_s *, long));
STATIC_DCL void FDECL(s_to_anything, (anything *, char *, int));
STATIC_DCL boolean FDECL(is_ltgt_percentnumber, (const char *));
STATIC_DCL boolean FDECL(has_ltgt_percentnumber, (const char *));
STATIC_DCL boolean FDECL(parse_status_hl2, (char (*)[QBUFSZ],BOOLEAN_P));
STATIC_DCL boolean FDECL(parse_condition, (char (*)[QBUFSZ], int));
STATIC_DCL void FDECL(merge_bestcolor, (int *, int));
STATIC_DCL void FDECL(get_hilite_color, (int, int, genericptr_t, int,
                                                int, int *));
STATIC_DCL unsigned long FDECL(match_str2conditionbitmask, (const char *));
STATIC_DCL unsigned long FDECL(str2conditionbitmask, (char *));
STATIC_DCL void FDECL(split_clridx, (int, int *, int *));
STATIC_DCL char *FDECL(hlattr2attrname, (int, char *, int));
STATIC_DCL void FDECL(status_hilite_linestr_add, (int, struct hilite_s *,
                                            unsigned long, const char *));

STATIC_DCL void NDECL(status_hilite_linestr_done);
STATIC_DCL int FDECL(status_hilite_linestr_countfield, (int));
STATIC_DCL void NDECL(status_hilite_linestr_gather_conditions);
STATIC_DCL void NDECL(status_hilite_linestr_gather);
STATIC_DCL char *FDECL(status_hilite2str, (struct hilite_s *));
STATIC_DCL boolean FDECL(status_hilite_menu_add, (int));
#define has_hilite(i) (blstats[0][(i)].thresholds)
#endif

#define INIT_BLSTAT(name, fmtstr, anytyp, wid, fld)                     \
    { name, fmtstr, 0L, FALSE, anytyp,  { (genericptr_t) 0 }, (char *) 0, \
      wid,  -1, fld }
#define INIT_BLSTATP(name, fmtstr, anytyp, wid, maxfld, fld)            \
    { name, fmtstr, 0L, FALSE, anytyp,  { (genericptr_t) 0 }, (char *) 0, \
      wid,  maxfld, fld }

/* If entries are added to this, botl.h will require updating too */
STATIC_DCL struct istat_s initblstats[MAXBLSTATS] = {
    INIT_BLSTAT("title", "%s", ANY_STR, 80, BL_TITLE),
    INIT_BLSTAT("strength", " St:%s", ANY_INT, 10, BL_STR),
    INIT_BLSTAT("dexterity", " Dx:%s", ANY_INT,  10, BL_DX),
    INIT_BLSTAT("constitution", " Co:%s", ANY_INT, 10, BL_CO),
    INIT_BLSTAT("intelligence", " In:%s", ANY_INT, 10, BL_IN),
    INIT_BLSTAT("wisdom", " Wi:%s", ANY_INT, 10, BL_WI),
    INIT_BLSTAT("charisma", " Ch:%s", ANY_INT, 10, BL_CH),
    INIT_BLSTAT("alignment", " %s", ANY_STR, 40, BL_ALIGN),
    INIT_BLSTAT("score", " S:%s", ANY_LONG, 20, BL_SCORE),
    INIT_BLSTAT("carrying-capacity", " %s", ANY_LONG, 20, BL_CAP),
    INIT_BLSTAT("gold", " %s", ANY_LONG, 30, BL_GOLD),
    INIT_BLSTATP("power", " Pw:%s", ANY_INT, 10, BL_ENEMAX, BL_ENE),
    INIT_BLSTAT("power-max", "(%s)", ANY_INT, 10, BL_ENEMAX),
    INIT_BLSTAT("experience-level", " Xp:%s", ANY_LONG, 10, BL_XP),
    INIT_BLSTAT("armor-class", " AC:%s", ANY_INT, 10, BL_AC),
    INIT_BLSTAT("HD", " HD:%s", ANY_INT, 10, BL_HD),
    INIT_BLSTAT("time", " T:%s", ANY_INT, 20, BL_TIME),
    INIT_BLSTAT("hunger", " %s", ANY_UINT, 40, BL_HUNGER),
    INIT_BLSTATP("hitpoints", " HP:%s", ANY_INT, 10, BL_HPMAX, BL_HP),
    INIT_BLSTAT("hitpoints-max", "(%s)", ANY_INT, 10, BL_HPMAX),
    INIT_BLSTAT("dungeon-level", "%s", ANY_STR, 80, BL_LEVELDESC),
    INIT_BLSTAT("experience", "/%s", ANY_LONG, 20, BL_EXP),
    INIT_BLSTAT("condition", "%s", ANY_MASK32, 0, BL_CONDITION)
};

#undef INIT_BLSTATP
#undef INIT_BLSTAT

struct istat_s blstats[2][MAXBLSTATS];
static boolean blinit = FALSE, update_all = FALSE;
static boolean valset[MAXBLSTATS];
unsigned long blcolormasks[CLR_MAX];
static long bl_hilite_moves = 0L;

/* we don't put this next declaration in #ifdef STATUS_HILITES.
 * In the absence of STATUS_HILITES, each array
 * element will be 0 however, and quite meaningless,
 * but we need to pass the first array element as
 * the final argument of status_update, with or
 * without STATUS_HILITES.
 */
unsigned long cond_hilites[BL_ATTCLR_MAX];

void
bot_via_windowport()
{
    char buf[BUFSZ];
    register char *nb;
    static int i, idx = 0, idx_p, cap;
    long money;

    if (!blinit)
        panic("bot before init.");

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
    blstats[idx][BL_STR].a.a_int = ACURR(A_STR);
    Strcpy(blstats[idx][BL_STR].val, get_strength_str());
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
        flags.showscore ? botl_score() :
#endif
        0L;

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
    blstats[idx][BL_HD].a.a_int = Upolyd ? (int) mons[u.umonnum].mlevel : 0;

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
    evaluate_and_notify_windowport(valset, idx, idx_p);
}

STATIC_OVL boolean
evaluate_and_notify_windowport_field(fld, valsetlist, idx, idx_p)
int fld, idx, idx_p;
boolean *valsetlist;
{
    static int oldrndencode = 0;
    int pc, chg, color = NO_COLOR;
    unsigned anytype;
    boolean updated = FALSE, reset;
    struct istat_s *curr = NULL, *prev = NULL;
    enum statusfields idxmax;

    /*
     *  Now pass the changed values to window port.
     */
    anytype = blstats[idx][fld].anytype;
    curr = &blstats[idx][fld];
    prev = &blstats[idx_p][fld];
    color = NO_COLOR;

    chg = update_all ? 0 : compare_blstats(prev, curr);

    /* Temporary? hack: moveloop()'s prolog for a new game sets
     * context.rndencode after the status window has been init'd,
     * so $:0 has already been encoded and cached by the window
     * port.  Without this hack, gold's \G sequence won't be
     * recognized and ends up being displayed as-is for 'update_all'.
     */
    if (context.rndencode != oldrndencode && fld == BL_GOLD) {
        chg = 2;
        oldrndencode = context.rndencode;
    }

    reset = FALSE;
#ifdef STATUS_HILITES
    if (!update_all && !chg) {
        reset = hilite_reset_needed(prev, bl_hilite_moves);
        if (reset)
          curr->time = prev->time = 0L;
    }
#endif

    if (update_all || chg || reset) {
        idxmax = curr->idxmax;
        pc = (idxmax > BL_FLUSH) ? percentage(curr, &blstats[idx][idxmax]) : 0;

        if (!valsetlist[fld])
            (void) anything_to_s(curr->val, &curr->a, anytype);

        if (anytype != ANY_MASK32) {
#ifdef STATUS_HILITES
            if ((chg || *curr->val)) {
                get_hilite_color(idx, fld, (genericptr_t)&curr->a,
                                 chg, pc, &color);
                if (chg == 2) {
                    color = NO_COLOR;
                    chg = 0;
                }
            }
#endif /* STATUS_HILITES */
            status_update(fld, (genericptr_t) curr->val,
                          chg, pc, color, &cond_hilites[0]);
        } else {
            /* Color for conditions is done through cond_hilites[] */
            status_update(fld, (genericptr_t) &curr->a.a_ulong, chg, pc,
                          color, &cond_hilites[0]);
        }
        curr->chg = prev->chg = TRUE;
        updated = TRUE;
    }
    return updated;
}

static void
evaluate_and_notify_windowport(valsetlist, idx, idx_p)
int idx, idx_p;
boolean *valsetlist;
{
    int i;
    boolean updated = FALSE;

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
        if (evaluate_and_notify_windowport_field(i, valsetlist, idx, idx_p))
            updated = TRUE;
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
        || (windowprocs.wincap2 & WC2_FLUSH_STATUS) != 0L)
        status_update(BL_FLUSH, (genericptr_t) 0, 0, 0,
                      NO_COLOR, &cond_hilites[0]);

    context.botl = context.botlx = 0;
    update_all = FALSE;
}

void
status_eval_next_unhilite()
{
    int i;
    struct istat_s *curr = NULL;
    long next_unhilite, this_unhilite;

    bl_hilite_moves = moves;
    /* figure out when the next unhilight needs to be performed */
    next_unhilite = 0L;
    for (i = 0; i < MAXBLSTATS; ++i) {
        curr = &blstats[0][i]; /* blstats[0][*].time == blstats[1][*].time */

        if (curr->chg) {
            struct istat_s *prev = &blstats[1][i];

#ifdef STATUS_HILITES
            curr->time = prev->time = (bl_hilite_moves + iflags.hilite_delta);
#endif
            curr->chg = prev->chg = FALSE;
        }

        this_unhilite = curr->time;
        if (this_unhilite > 0L
            && (next_unhilite == 0L || this_unhilite < next_unhilite)
#ifdef STATUS_HILITES
            && hilite_reset_needed(curr, this_unhilite + 1L)
#endif
            )
            next_unhilite = this_unhilite;
    }
    if (next_unhilite > 0L && next_unhilite < bl_hilite_moves)
        context.botl = TRUE;
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
        if (blinit)
            impossible("2nd status_initialize with full init.");
        init_blstats();
        (*windowprocs.win_status_init)();
        blinit = TRUE;
    }
    for (i = 0; i < MAXBLSTATS; ++i) {
        enum statusfields fld = initblstats[i].fld;
        boolean fldenabled = (fld == BL_SCORE) ? flags.showscore
            : (fld == BL_XP) ? (boolean) !Upolyd
            : (fld == BL_HD) ? (boolean) Upolyd
            : (fld == BL_TIME) ? flags.time
            : (fld == BL_EXP) ? (boolean) (flags.showexp && !Upolyd)
            : TRUE;

        fieldname = initblstats[i].fldname;
        fieldfmt = initblstats[i].fldfmt;
        status_enablefield(fld, fieldname, fieldfmt, fldenabled);
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
            free((genericptr_t) blstats[0][i].val), blstats[0][i].val = 0;
        if (blstats[1][i].val)
            free((genericptr_t) blstats[1][i].val), blstats[1][i].val = 0;
#ifdef STATUS_HILITES
        if (blstats[0][i].thresholds) {
            struct hilite_s *temp = blstats[0][i].thresholds,
                            *next = (struct hilite_s *)0;
            while (temp) {
                next = temp->next;
                free(temp);
                blstats[0][i].thresholds = (struct hilite_s *)0;
                blstats[1][i].thresholds = blstats[0][i].thresholds;
                temp = next;
            }
	}
#endif /* STATUS_HILITES */
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
#ifdef STATUS_HILITES
            struct hilite_s *keep_hilite_chain = blstats[i][j].thresholds;
#endif
            blstats[i][j] = initblstats[j];
            blstats[i][j].a = zeroany;
            if (blstats[i][j].valwidth) {
                blstats[i][j].val = (char *) alloc(blstats[i][j].valwidth);
                blstats[i][j].val[0] = '\0';
            } else
                blstats[i][j].val = (char *) 0;
#ifdef STATUS_HILITES
            if (keep_hilite_chain)
                blstats[i][j].thresholds = keep_hilite_chain;
#endif
        }
    }
}

/*
 * This compares the previous stat with the current stat,
 * and returns one of the following results based on that:
 *
 *   if prev_value < new_value (stat went up, increased)
 *      return 1
 *
 *   if prev_value > new_value (stat went down, decreased)
 *      return  -1
 *
 *   if prev_value == new_value (stat stayed the same)
 *      return 0
 *
 *   Special cases:
 *     - for bitmasks, 0 = stayed the same, 1 = changed
 *     - for strings,  0 = stayed the same, 1 = changed
 *
 */
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
        result = sgn(strcmp(bl1->val, bl2->val));
        break;
    case ANY_MASK32:
        result = (bl1->a.a_ulong != bl2->a.a_ulong);
        break;
    default:
        result = 1;
    }
    return result;
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
#endif /* STATUS_HILITES */

STATIC_OVL int
percentage(bl, maxbl)
struct istat_s *bl, *maxbl;
{
    int result = 0;
    int anytype;
    int ival;
    long lval;
    unsigned uval;
    unsigned long ulval;

    if (!bl || !maxbl) {
        impossible("percentage: bad istat pointer %s, %s",
                   fmt_ptr((genericptr_t) bl), fmt_ptr((genericptr_t) maxbl));
        return 0;
    }

    ival = 0, lval = 0L, uval = 0U, ulval = 0UL;
    anytype = bl->anytype;
    if (maxbl->a.a_void) {
        switch (anytype) {
        case ANY_INT:
            ival = bl->a.a_int;
            result = ((100 * ival) / maxbl->a.a_int);
            break;
        case ANY_LONG:
            lval  = bl->a.a_long;
            result = (int) ((100L * lval) / maxbl->a.a_long);
            break;
        case ANY_UINT:
            uval = bl->a.a_uint;
            result = (int) ((100U * uval) / maxbl->a.a_uint);
            break;
        case ANY_ULONG:
            ulval = bl->a.a_ulong;
            result = (int) ((100UL * ulval) / maxbl->a.a_ulong);
            break;
        case ANY_IPTR:
            ival = *bl->a.a_iptr;
            result = ((100 * ival) / (*maxbl->a.a_iptr));
            break;
        case ANY_LPTR:
            lval = *bl->a.a_lptr;
            result = (int) ((100L * lval) / (*maxbl->a.a_lptr));
            break;
        case ANY_UPTR:
            uval = *bl->a.a_uptr;
            result = (int) ((100U * uval) / (*maxbl->a.a_uptr));
            break;
        case ANY_ULPTR:
            ulval = *bl->a.a_ulptr;
            result = (int) ((100UL * ulval) / (*maxbl->a.a_ulptr));
            break;
        }
    }
    /* don't let truncation from integer division produce a zero result
       from a non-zero input; note: if we ever change to something like
       ((((1000 * val) / max) + 5) / 10) for a rounded result, we'll
       also need to check for and convert false 100 to 99 */
    if (result == 0 && (ival != 0 || lval != 0L || uval != 0U || ulval != 0UL))
        result = 1;

    return result;
}


#ifdef STATUS_HILITES

/****************************************************************************/
/* Core status hiliting support */
/****************************************************************************/

struct hilite_s status_hilites[MAXBLSTATS];

static struct fieldid_t {
    const char *fieldname;
    enum statusfields fldid;
} fieldids_alias[] = {
    {"characteristics", BL_CHARACTERISTICS},
    {"dx",       BL_DX},
    {"co",       BL_CO},
    {"con",      BL_CO},
    {"points",   BL_SCORE},
    {"cap",      BL_CAP},
    {"pw",       BL_ENE},
    {"pw-max",   BL_ENEMAX},
    {"xl",       BL_XP},
    {"xplvl",    BL_XP},
    {"ac",       BL_AC},
    {"hit-dice", BL_HD},
    {"turns",    BL_TIME},
    {"hp",       BL_HP},
    {"hp-max",   BL_HPMAX},
    {"dgn",      BL_LEVELDESC},
    {"xp",       BL_EXP},
    {"exp",      BL_EXP},
    {"flags",    BL_CONDITION},
    {0,          BL_FLUSH}
};

/* field name to bottom line index */
STATIC_OVL enum statusfields
fldname_to_bl_indx(name)
const char *name;
{
    int i, nmatches = 0, fld = 0;

    if (name && *name) {
        /* check matches to canonical names */
        for (i = 0; i < SIZE(initblstats); i++)
            if (fuzzymatch(initblstats[i].fldname, name, " -_", TRUE)) {
                fld = initblstats[i].fld;
                nmatches++;
            }

        if (!nmatches) {
            /* check aliases */
            for (i = 0; fieldids_alias[i].fieldname; i++)
                if (fuzzymatch(fieldids_alias[i].fieldname, name,
                               " -_", TRUE)) {
                    fld = fieldids_alias[i].fldid;
                    nmatches++;
                }
        }

        if (!nmatches) {
            /* check partial matches to canonical names */
            int len = (int) strlen(name);
            for (i = 0; i < SIZE(initblstats); i++)
                if (!strncmpi(name, initblstats[i].fldname, len)) {
                    fld = initblstats[i].fld;
                    nmatches++;
                }
        }

    }
    return (nmatches == 1) ? fld : BL_FLUSH;
}

STATIC_OVL boolean
hilite_reset_needed(bl_p, augmented_time)
struct istat_s *bl_p;
long augmented_time;
{
    struct hilite_s *tl = bl_p->thresholds;

    /*
     * This 'multi' handling may need some tuning...
     */
    if (multi)
        return FALSE;

    if (bl_p->time == 0 || bl_p->time >= augmented_time)
        return FALSE;

    while (tl) {
        /* only this style times out */
        if (tl->behavior == BL_TH_UPDOWN)
            return TRUE;
        tl = tl->next;
    }

    return FALSE;
}

/* called by options handling when 'statushilites' boolean is toggled */
void
reset_status_hilites()
{
    if (iflags.hilite_delta) {
        int i;

        for (i = 0; i < MAXBLSTATS; ++i)
            blstats[0][i].time = blstats[1][i].time = 0L;
        update_all = TRUE;
    }
    context.botlx = TRUE;
}

STATIC_OVL void
merge_bestcolor(bestcolor, newcolor)
int *bestcolor;
int newcolor;
{
    int natr = HL_UNDEF, nclr = NO_COLOR;

    split_clridx(newcolor, &nclr, &natr);

    if (nclr != NO_COLOR)
        *bestcolor = (*bestcolor & 0xff00) | nclr;

    if (natr != HL_UNDEF) {
        if (natr == HL_NONE)
            *bestcolor = *bestcolor & 0x00ff; /* reset all attributes */
        else
            *bestcolor |= (natr << 8); /* merge attributes */
    }
}

/*
 * get_hilite_color
 * 
 * Figures out, based on the value and the
 * direction it is moving, the color that the field
 * should be displayed in.
 *
 *
 * Provide get_hilite_color() with the following
 * to work with:
 *     actual value vp
 *          useful for BL_TH_VAL_ABSOLUTE
 *     indicator of down, up, or the same (-1, 1, 0) chg
 *          useful for BL_TH_UPDOWN or change detection
 *     percentage (current value percentage of max value) pc
 *          useful for BL_TH_VAL_PERCENTAGE
 *
 * Get back:
 *     color     based on user thresholds set in config file.
 *               The rightmost 8 bits contain a color index.
 *               The 8 bits to the left of that contain
 *               the attribute bits.
 *                   color = 0x00FF
 *                   attrib= 0xFF00
 */

STATIC_OVL void
get_hilite_color(idx, fldidx, vp, chg, pc, colorptr)
int idx, fldidx, chg, pc;
genericptr_t vp;
int *colorptr;
{
    int bestcolor = NO_COLOR;
    struct hilite_s *hl;
    anything *value = (anything *)vp;
    char *txtstr, *cmpstr;

    if (!colorptr || fldidx < 0 || fldidx >= MAXBLSTATS)
        return;

    if (blstats[idx][fldidx].thresholds) {
        /* there are hilites set here */
        int max_pc = 0, min_pc = 100;
        int max_val = 0, min_val = LARGEST_INT;
        boolean exactmatch = FALSE;

        hl = blstats[idx][fldidx].thresholds;

        while (hl) {
            switch (hl->behavior) {
            case BL_TH_VAL_PERCENTAGE:
                if (hl->rel == EQ_VALUE && pc == hl->value.a_int) {
                    merge_bestcolor(&bestcolor, hl->coloridx);
                    min_pc = max_pc = hl->value.a_int;
                    exactmatch = TRUE;
                } else if (hl->rel == LT_VALUE && !exactmatch
                           && (hl->value.a_int >= pc)
                           && (hl->value.a_int <= min_pc)) {
                    merge_bestcolor(&bestcolor, hl->coloridx);
                    min_pc = hl->value.a_int;
                } else if (hl->rel == GT_VALUE && !exactmatch
                           && (hl->value.a_int <= pc)
                           && (hl->value.a_int >= max_pc)) {
                    merge_bestcolor(&bestcolor, hl->coloridx);
                    max_pc = hl->value.a_int;
                }
                break;
            case BL_TH_UPDOWN:
                if (chg < 0 && hl->rel == LT_VALUE) {
                    merge_bestcolor(&bestcolor, hl->coloridx);
                } else if (chg > 0 && hl->rel == GT_VALUE) {
                    merge_bestcolor(&bestcolor, hl->coloridx);
                } else if (hl->rel == EQ_VALUE && chg) {
                    merge_bestcolor(&bestcolor, hl->coloridx);
                    min_val = max_val = hl->value.a_int;
                }
                break;
            case BL_TH_VAL_ABSOLUTE:
                if (hl->rel == EQ_VALUE && hl->value.a_int == value->a_int) {
                    merge_bestcolor(&bestcolor, hl->coloridx);
                    min_val = max_val = hl->value.a_int;
                    exactmatch = TRUE;
                } else if (hl->rel == LT_VALUE && !exactmatch
                           && (hl->value.a_int >= value->a_int)
                           && (hl->value.a_int < min_val)) {
                    merge_bestcolor(&bestcolor, hl->coloridx);
                    min_val = hl->value.a_int;
                } else if (hl->rel == GT_VALUE && !exactmatch
                           && (hl->value.a_int <= value->a_int)
                           && (hl->value.a_int > max_val)) {
                    merge_bestcolor(&bestcolor, hl->coloridx);
                    max_val = hl->value.a_int;
                }
                break;
            case BL_TH_TEXTMATCH:
                txtstr = dupstr(blstats[idx][fldidx].val);
                cmpstr = txtstr;
                if (fldidx == BL_TITLE) {
                    int len = (strlen(plname) + sizeof(" the"));
                    cmpstr += len;
                }
                (void) trimspaces(cmpstr);
                if (hl->rel == TXT_VALUE && hl->textmatch[0] &&
                    !strcmpi(hl->textmatch, cmpstr)) {
                    merge_bestcolor(&bestcolor, hl->coloridx);
                }
                free(txtstr);
                break;
            case BL_TH_ALWAYS_HILITE:
                merge_bestcolor(&bestcolor, hl->coloridx);
                break;
            case BL_TH_NONE:
                break;
            default:
                break;
            }
            hl = hl->next;
	}
    }
    *colorptr = bestcolor;
    return;
}

STATIC_OVL void
split_clridx(idx, coloridx, attrib)
int idx;
int *coloridx, *attrib;
{
    if (idx && coloridx && attrib) {
           *coloridx = idx & 0x00FF;
           *attrib = (idx & 0xFF00) >> 8;
    }
}


/*
 * This is the parser for the hilite options
 *
 * parse_status_hl1() separates each hilite entry into
 * a set of field threshold/action component strings,
 * then calls parse_status_hl2() to parse further
 * and configure the hilite.
 */
boolean
parse_status_hl1(op, from_configfile)
char *op;
boolean from_configfile;
{
#define MAX_THRESH 21
    char hsbuf[MAX_THRESH][QBUFSZ];
    boolean rslt, badopt = FALSE;
    int i, fldnum, ccount = 0;
    char c;

    fldnum = 0;
    for (i = 0; i < MAX_THRESH; ++i) {
        hsbuf[i][0] = '\0';
    }
    while (*op && fldnum < MAX_THRESH && ccount < (QBUFSZ - 2)) {
        c = lowc(*op);
        if (c == ' ') {
            if (fldnum >= 1) {
                rslt = parse_status_hl2(hsbuf, from_configfile);
                if (!rslt) {
                    badopt = TRUE;
                    break;
                }
            }
            for (i = 0; i < MAX_THRESH; ++i) {
                hsbuf[i][0] = '\0';
            }
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
    if (fldnum >= 1 && !badopt) {
        rslt = parse_status_hl2(hsbuf, from_configfile);
        if (!rslt)
            badopt = TRUE;
    }
    if (badopt)
        return FALSE;
    return TRUE;
}

/* is str in the format of "(<>)?[0-9]+%?" regex */
STATIC_OVL boolean
is_ltgt_percentnumber(str)
const char *str;
{
    const char *s = str;

    if (*s == '<' || *s == '>') s++;
    while (digit(*s)) s++;
    if (*s == '%') s++;

    return (*s == '\0');
}

/* does str only contain "<>0-9%" chars */
STATIC_OVL boolean
has_ltgt_percentnumber(str)
const char *str;
{
    const char *s = str;

    while (*s) {
        if (!index("<>0123456789%", *s))
            return FALSE;
        s++;
    }
    return TRUE;
}

/* splitsubfields(): splits str in place into '+' or '&' separated strings.
 * returns number of strings, or -1 if more than maxsf or MAX_SUBFIELDS
 */
#define MAX_SUBFIELDS 16
STATIC_OVL int
splitsubfields(str, sfarr, maxsf)
char *str;
char ***sfarr;
int maxsf;
{
    static char *subfields[MAX_SUBFIELDS];
    char *st = (char *) 0;
    int sf = 0;

    if (!str)
        return 0;
    for (sf = 0; sf < MAX_SUBFIELDS; ++sf)
        subfields[sf] = (char *) 0;

    maxsf = (maxsf == 0) ? MAX_SUBFIELDS : min(maxsf, MAX_SUBFIELDS);

    if (index(str, '+') || index(str, '&')) {
        char *c = str;

        sf = 0;
        st = c;
        while (*c && sf < maxsf) {
            if (*c == '&' || *c == '+') {
                *c = '\0';
                subfields[sf] = st;
                st = c+1;
                sf++;
            }
            c++;
        }
        if (sf >= maxsf - 1)
            return -1;
        if (!*c && c != st)
            subfields[sf++] = st;
    } else {
        sf = 1;
        subfields[0] = str;
    }
    *sfarr = subfields;
    return sf;
}
#undef MAX_SUBFIELDS

boolean
is_fld_arrayvalues(str, arr, arrmin, arrmax, retidx)
const char *str;
const char *const *arr;
int arrmin, arrmax;
int *retidx;
{
    int i;

    for (i = arrmin; i < arrmax; i++)
        if (!strcmpi(str, arr[i])) {
            *retidx = i;
            return TRUE;
        }
    return FALSE;
}

int
query_arrayvalue(querystr, arr, arrmin, arrmax)
const char *querystr;
const char *const *arr;
int arrmin, arrmax;
{
    int i, res, ret = arrmin - 1;
    winid tmpwin;
    anything any;
    menu_item *picks = (menu_item *) 0;
    int adj = (arrmin > 0) ? 1 : arrmax;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin);

    for (i = arrmin; i < arrmax; i++) {
        any = zeroany;
        any.a_int = i + adj;
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
                 arr[i], MENU_UNSELECTED);
    }

    end_menu(tmpwin, querystr);

    res = select_menu(tmpwin, PICK_ONE, &picks);
    destroy_nhwindow(tmpwin);
    if (res > 0) {
        ret = picks->item.a_int - adj;
        free((genericptr_t) picks);
    }

    return ret;
}

void
status_hilite_add_threshold(fld, hilite)
int fld;
struct hilite_s *hilite;
{
    struct hilite_s *new_hilite;

    if (!hilite)
        return;

    /* alloc and initialize a new hilite_s struct */
    new_hilite = (struct hilite_s *) alloc(sizeof(struct hilite_s));
    *new_hilite = *hilite;   /* copy struct */

    new_hilite->set = TRUE;
    new_hilite->fld = fld;
    new_hilite->next = (struct hilite_s *)0;

    /* Does that status field currently have any hilite thresholds? */
    if (!blstats[0][fld].thresholds) {
        blstats[0][fld].thresholds = new_hilite;
    } else {
        struct hilite_s *temp_hilite = blstats[0][fld].thresholds;
        new_hilite->next = temp_hilite;
        blstats[0][fld].thresholds = new_hilite;
        /* sort_hilites(fld) */
    }
    /* current and prev must both point at the same hilites */
    blstats[1][fld].thresholds = blstats[0][fld].thresholds;
}


STATIC_OVL boolean
parse_status_hl2(s, from_configfile)
char (*s)[QBUFSZ];
boolean from_configfile;
{
    char *tmp, *how;
    int sidx = 0, i = -1, dt = -1;
    int coloridx = -1, successes = 0;
    int disp_attrib = 0;
    boolean percent = FALSE, changed = FALSE, numeric = FALSE;
    boolean down= FALSE, up = FALSE;
    boolean gt = FALSE, lt = FALSE, eq = FALSE, neq = FALSE;
    boolean txtval = FALSE;
    boolean always = FALSE;
    const char *txt;
    enum statusfields fld = BL_FLUSH;
    struct hilite_s hilite;
    char tmpbuf[BUFSZ];
    const char *aligntxt[] = {"chaotic", "neutral", "lawful"};
    /* hu_stat[] from eat.c has trailing spaces which foul up comparisons */
    const char *hutxt[] = {"Satiated", "", "Hungry", "Weak",
                           "Fainting", "Fainted", "Starved"};

    /* Examples:
        3.6.1:
      OPTION=hilite_status: hitpoints/<10%/red
      OPTION=hilite_status: hitpoints/<10%/red/<5%/purple/1/red+blink+inverse
      OPTION=hilite_status: experience/down/red/up/green
      OPTION=hilite_status: cap/strained/yellow/overtaxed/orange
      OPTION=hilite_status: title/always/blue
      OPTION=hilite_status: title/blue
    */

    /* field name to statusfield */
    fld = fldname_to_bl_indx(s[sidx]);

    if (fld == BL_CHARACTERISTICS) {
        boolean res = FALSE;

        /* recursively set each of strength, dexterity, constitution, &c */
        for (fld = BL_STR; fld <= BL_CH; fld++) {
            Strcpy(s[sidx], initblstats[fld].fldname);
            res = parse_status_hl2(s, from_configfile);
            if (!res)
                return FALSE;
        }
        return TRUE;
    }
    if (fld == BL_FLUSH) {
        config_error_add("Unknown status field '%s'", s[sidx]);
        return FALSE;
    }
    if (fld == BL_CONDITION)
        return parse_condition(s, sidx);

    ++sidx;
    while(s[sidx]) {
        char buf[BUFSZ], **subfields;
        int sf = 0;     /* subfield count */
        int kidx;

        txt = (const char *)0;
        percent = changed = numeric = FALSE;
        down = up = FALSE;
        gt = eq = lt = neq = txtval = FALSE;
        always = FALSE;

        /* threshold value */
        if (!s[sidx][0])
            return TRUE;

        memset((genericptr_t) &hilite, 0, sizeof(struct hilite_s));
        hilite.set = FALSE; /* mark it "unset" */
        hilite.fld = fld;

        if (*s[sidx+1] == '\0' || !strcmpi(s[sidx], "always")) {
            /* "field/always/color" OR "field/color" */
            always = TRUE;
            if (*s[sidx+1] == '\0')
                sidx--;
            goto do_rel;
        } else if (!strcmpi(s[sidx], "up") || !strcmpi(s[sidx], "down")) {
            if (!strcmpi(s[sidx], "down"))
                down = TRUE;
            else
                up = TRUE;
            changed = TRUE;
            goto do_rel;
	} else if (fld == BL_CAP
                   && is_fld_arrayvalues(s[sidx], enc_stat,
                                         SLT_ENCUMBER, OVERLOADED+1, &kidx)) {
            txt = enc_stat[kidx];
            txtval = TRUE;
	    goto do_rel;
        } else if (fld == BL_ALIGN
                   && is_fld_arrayvalues(s[sidx], aligntxt, 0, 3, &kidx)) {
            txt = aligntxt[kidx];
            txtval = TRUE;
            goto do_rel;
        } else if (fld == BL_HUNGER
                   && is_fld_arrayvalues(s[sidx], hutxt,
                                         SATIATED, STARVED+1, &kidx)) {
            txt = hu_stat[kidx];   /* store hu_stat[] val, not hutxt[] */
            txtval = TRUE;
            goto do_rel;
        } else if (!strcmpi(s[sidx], "changed")) {
            changed = TRUE;
            goto do_rel;
        } else if (is_ltgt_percentnumber(s[sidx])) {
            tmp = s[sidx];
            if (strchr(tmp, '%'))
               percent = TRUE;
            if (strchr(tmp, '<'))
                lt = TRUE;
            if (strchr(tmp, '>'))
                gt = TRUE;
            (void) stripchars(tmpbuf, "%<>", tmp);
            tmp = tmpbuf;
            while (*tmp) {
                if (!index("0123456789", *tmp))
                    return FALSE;
                tmp++;
            }
            numeric = TRUE;
            tmp = tmpbuf;
            if (strlen(tmp) > 0) {
                dt = initblstats[fld].anytype;
                if (percent)
                    dt = ANY_INT;
                (void) s_to_anything(&hilite.value, tmp, dt);
            } else
                return FALSE;
            if (!hilite.value.a_void && (strcmp(tmp, "0") != 0))
               return FALSE;
        } else if (initblstats[fld].anytype == ANY_STR) {
            txt = s[sidx];
            txtval = TRUE;
            goto do_rel;
        } else {
            config_error_add(has_ltgt_percentnumber(s[sidx])
                 ? "Wrong format '%s', expected a threshold number or percent"
                 : "Unknown behavior '%s'", s[sidx]);
            return FALSE;
        }
do_rel:
        /* relationships { LT_VALUE, GT_VALUE, EQ_VALUE} */
        if (gt)
            hilite.rel = GT_VALUE;
        else if (eq)
            hilite.rel = EQ_VALUE;
        else if (lt)
            hilite.rel = LT_VALUE;
        else if (percent)
            hilite.rel = EQ_VALUE;
        else if (numeric)
            hilite.rel = EQ_VALUE;
        else if (down)
            hilite.rel = LT_VALUE;
        else if (up)
            hilite.rel = GT_VALUE;
        else if (changed)
            hilite.rel = EQ_VALUE;
        else if (txtval)
            hilite.rel = TXT_VALUE;
        else
            hilite.rel = LT_VALUE;

        if (initblstats[fld].anytype == ANY_STR
            && (percent || numeric)) {
            config_error_add("Field '%s' does not support numeric values",
                             initblstats[fld].fldname);
            return FALSE;
        }

        if (percent) {
            if (initblstats[fld].idxmax <= BL_FLUSH) {
                config_error_add("Cannot use percent with '%s'",
                                 initblstats[fld].fldname);
                return FALSE;
            } else if ((hilite.value.a_int < 0)
                       || (hilite.value.a_int == 0
                           && hilite.rel == LT_VALUE)
                       || (hilite.value.a_int > 100)
                       || (hilite.value.a_int == 100
                           && hilite.rel == GT_VALUE)) {
                config_error_add("Illegal percentage value");
                return FALSE;
            }
        }

        /* actions */
        sidx++;
        how = s[sidx];
        if (!how) {
            if (!successes)
                return FALSE;
        }
        coloridx = -1;
        Strcpy(buf, how);
        sf = splitsubfields(buf, &subfields, 0);

        if (sf < 1)
            return FALSE;

        disp_attrib = HL_UNDEF;

        for (i = 0; i < sf; ++i) {
            int a = match_str2attr(subfields[i], FALSE);
            if (a == ATR_DIM)
                disp_attrib |= HL_DIM;
            else if (a == ATR_BLINK)
                disp_attrib |= HL_BLINK;
            else if (a == ATR_ULINE)
                disp_attrib |= HL_ULINE;
            else if (a == ATR_INVERSE)
                disp_attrib |= HL_INVERSE;
            else if (a == ATR_BOLD)
                disp_attrib |= HL_BOLD;
            else if (a == ATR_NONE)
                disp_attrib = HL_NONE;
            else {
                int c = match_str2clr(subfields[i]);

                if (c >= CLR_MAX || coloridx != -1)
                    return FALSE;
                coloridx = c;
	    }
        }
        if (coloridx == -1)
            coloridx = NO_COLOR;

        /* Assign the values */
        hilite.coloridx = coloridx | (disp_attrib << 8);

        if (always)
            hilite.behavior = BL_TH_ALWAYS_HILITE;
        else if (percent)
            hilite.behavior = BL_TH_VAL_PERCENTAGE;
        else if (changed)
            hilite.behavior = BL_TH_UPDOWN;
        else if (numeric)
            hilite.behavior = BL_TH_VAL_ABSOLUTE;
        else if (txtval)
            hilite.behavior = BL_TH_TEXTMATCH;
        else if (hilite.value.a_void)
            hilite.behavior = BL_TH_VAL_ABSOLUTE;
       else
            hilite.behavior = BL_TH_NONE;

        hilite.anytype = dt;

        if (hilite.behavior == BL_TH_TEXTMATCH && txt
            && strlen(txt) < QBUFSZ-1) {
            Strcpy(hilite.textmatch, txt);
            (void) trimspaces(hilite.textmatch);
        }

        status_hilite_add_threshold(fld, &hilite);

        successes++;
        sidx++;
    }

    return TRUE;
}



const struct condmap valid_conditions[] = {
    {"stone",    BL_MASK_STONE},
    {"slime",    BL_MASK_SLIME},
    {"strngl",   BL_MASK_STRNGL},
    {"foodPois", BL_MASK_FOODPOIS},
    {"termIll",  BL_MASK_TERMILL},
    {"blind",    BL_MASK_BLIND},
    {"deaf",     BL_MASK_DEAF},
    {"stun",     BL_MASK_STUN},
    {"conf",     BL_MASK_CONF},
    {"hallu",    BL_MASK_HALLU},
    {"lev",      BL_MASK_LEV},
    {"fly",      BL_MASK_FLY},
    {"ride",     BL_MASK_RIDE},
};

const struct condmap condition_aliases[] = {
    {"strangled",      BL_MASK_STRNGL},
    {"all",            BL_MASK_STONE | BL_MASK_SLIME | BL_MASK_STRNGL |
                       BL_MASK_FOODPOIS | BL_MASK_TERMILL |
                       BL_MASK_BLIND | BL_MASK_DEAF | BL_MASK_STUN |
                       BL_MASK_CONF | BL_MASK_HALLU |
                       BL_MASK_LEV | BL_MASK_FLY | BL_MASK_RIDE },
    {"major_troubles", BL_MASK_STONE | BL_MASK_SLIME | BL_MASK_STRNGL |
                       BL_MASK_FOODPOIS | BL_MASK_TERMILL},
    {"minor_troubles", BL_MASK_BLIND | BL_MASK_DEAF | BL_MASK_STUN |
                       BL_MASK_CONF | BL_MASK_HALLU},
    {"movement",       BL_MASK_LEV | BL_MASK_FLY | BL_MASK_RIDE}
};

unsigned long
query_conditions()
{
    int i,res;
    unsigned long ret = 0UL;
    winid tmpwin;
    anything any;
    menu_item *picks = (menu_item *) 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin);

    for (i = 0; i < SIZE(valid_conditions); i++) {
        any = zeroany;
        any.a_ulong = valid_conditions[i].bitmask;
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
                 valid_conditions[i].id, MENU_UNSELECTED);
    }

    end_menu(tmpwin, "Choose status conditions");

    res = select_menu(tmpwin, PICK_ANY, &picks);
    destroy_nhwindow(tmpwin);
    if (res > 0) {
        for (i = 0; i < res; i++)
            ret |= picks[i].item.a_ulong;
        free((genericptr_t) picks);
    }
    return ret;
}

STATIC_OVL char *
conditionbitmask2str(ul)
unsigned long ul;
{
    static char buf[BUFSZ];
    int i;
    boolean first = TRUE;
    const char *alias = (char *) 0;


    buf[0] = '\0';
    if (!ul)
        return buf;

    for (i = 1; i < SIZE(condition_aliases); i++)
        if (condition_aliases[i].bitmask == ul)
            alias = condition_aliases[i].id;

    for (i = 0; i < SIZE(valid_conditions); i++)
        if ((valid_conditions[i].bitmask & ul) != 0UL) {
            Sprintf(eos(buf), "%s%s", (first) ? "" : "+",
                    valid_conditions[i].id);
            first = FALSE;
        }

    if (!first && alias)
        Sprintf(buf, "%s", alias);

    return buf;
}

STATIC_OVL unsigned long
match_str2conditionbitmask(str)
const char *str;
{
    int i, nmatches = 0;
    unsigned long mask = 0UL;

    if (str && *str) {
        /* check matches to canonical names */
        for (i = 0; i < SIZE(valid_conditions); i++)
            if (fuzzymatch(valid_conditions[i].id, str, " -_", TRUE)) {
                mask |= valid_conditions[i].bitmask;
                nmatches++;
            }

        if (!nmatches) {
            /* check aliases */
            for (i = 0; i < SIZE(condition_aliases); i++)
                if (fuzzymatch(condition_aliases[i].id, str, " -_", TRUE)) {
                    mask |= condition_aliases[i].bitmask;
                    nmatches++;
                }
        }

        if (!nmatches) {
            /* check partial matches to aliases */
            int len = (int) strlen(str);
            for (i = 0; i < SIZE(condition_aliases); i++)
                if (!strncmpi(str, condition_aliases[i].id, len)) {
                    mask |= condition_aliases[i].bitmask;
                    nmatches++;
                }
        }
    }

    return mask;
}

STATIC_OVL unsigned long
str2conditionbitmask(str)
char *str;
{
    unsigned long conditions_bitmask = 0UL;
    char **subfields;
    int i, sf;

    sf = splitsubfields(str, &subfields, SIZE(valid_conditions));

    if (sf < 1)
        return 0UL;

    for (i = 0; i < sf; ++i) {
        unsigned long bm = match_str2conditionbitmask(subfields[i]);

        if (!bm) {
            config_error_add("Unknown condition '%s'", subfields[i]);
            return 0UL;
        }
        conditions_bitmask |= bm;
    }
    return conditions_bitmask;
}

STATIC_OVL boolean
parse_condition(s, sidx)
char (*s)[QBUFSZ];
int sidx;
{
    int i;
    int coloridx = NO_COLOR;
    char *tmp, *how;
    unsigned long conditions_bitmask = 0UL;
    boolean success = FALSE;

    if (!s)
        return FALSE;

    /*3.6.1:
      OPTION=hilite_status: condition/stone+slime+foodPois/red&inverse */

    sidx++;
    while(s[sidx]) {
        int sf = 0;     /* subfield count */
        char buf[BUFSZ], **subfields;

        tmp = s[sidx];
        if (!*tmp) {
            if (!success)
                config_error_add("Missing condition(s)");
            return success;
	}

        Strcpy(buf, tmp);
        conditions_bitmask = str2conditionbitmask(buf);

        if (!conditions_bitmask)
            return FALSE;

        /*
         * We have the conditions_bitmask with bits set for
         * each ailment we want in a particular color and/or
         * attribute, but we need to assign it to an arry of
         * bitmasks indexed by the color chosen
         *        (0 to (CLR_MAX - 1))
         * and/or attributes chosen
         *        (HL_ATTCLR_DIM to (BL_ATTCLR_MAX - 1))
         * We still have to parse the colors and attributes out.
         */

        /* actions */
        sidx++;
        how = s[sidx];
        if (!how || !*how) {
            config_error_add("Missing color+attribute");
            return FALSE;
        }

        Strcpy(buf, how);
        sf = splitsubfields(buf, &subfields, 0);

        /*
         * conditions_bitmask now has bits set representing
         * the conditions that player wants represented, but
         * now we parse out *how* they will be represented.
         *
         * Only 1 colour is allowed, but potentially multiple
         * attributes are allowed.
         *
         * We have the following additional array offsets to
         * use for storing the attributes beyond the end of
         * the color indexes, all of which are less than CLR_MAX.
         * HL_ATTCLR_DIM        = CLR_MAX
         * HL_ATTCLR_BLINK      = CLR_MAX + 1
         * HL_ATTCLR_ULINE      = CLR_MAX + 2
         * HL_ATTCLR_INVERSE    = CLR_MAX + 3
         * HL_ATTCLR_BOLD       = CLR_MAX + 4
         * HL_ATTCLR_MAX        = CLR_MAX + 5 (this is past array boundary)
         *
         */

        for (i = 0; i < sf; ++i) {
            int a = match_str2attr(subfields[i], FALSE);
            if (a == ATR_DIM)
                cond_hilites[HL_ATTCLR_DIM] |= conditions_bitmask;
            else if (a == ATR_BLINK)
                cond_hilites[HL_ATTCLR_BLINK] |= conditions_bitmask;
            else if (a == ATR_ULINE)
                cond_hilites[HL_ATTCLR_ULINE] |= conditions_bitmask;
            else if (a == ATR_INVERSE)
                cond_hilites[HL_ATTCLR_INVERSE] |= conditions_bitmask;
            else if (a == ATR_BOLD)
                cond_hilites[HL_ATTCLR_BOLD] |= conditions_bitmask;
            else if (a == ATR_NONE) {
                cond_hilites[HL_ATTCLR_DIM]     = 0UL;
                cond_hilites[HL_ATTCLR_BLINK]   = 0UL;
                cond_hilites[HL_ATTCLR_ULINE]   = 0UL;
                cond_hilites[HL_ATTCLR_INVERSE] = 0UL;
                cond_hilites[HL_ATTCLR_BOLD]    = 0UL;
            } else {
                int k = match_str2clr(subfields[i]);

                if (k >= CLR_MAX)
                    return FALSE;
                coloridx = k;
	    }
        }
        /* set the bits in the appropriate member of the
           condition array according to color chosen as index */

        cond_hilites[coloridx] |= conditions_bitmask;
        success = TRUE;
        sidx++;
    }
    return TRUE;
}

void
clear_status_hilites()
{
    int i;

    for (i = 0; i < MAXBLSTATS; ++i) {
        if (blstats[0][i].thresholds) {
            struct hilite_s *temp = blstats[0][i].thresholds,
                            *next = (struct hilite_s *)0;
            while (temp) {
                next = temp->next;
                free(temp);
                blstats[0][i].thresholds = (struct hilite_s *)0;
                blstats[1][i].thresholds = blstats[0][i].thresholds;
                temp = next;
            }
	}
    }
}

STATIC_OVL char *
hlattr2attrname(attrib, buf, bufsz)
int attrib, bufsz;
char *buf;
{
    if (attrib && buf) {
        char attbuf[BUFSZ];
        int k, first = 0;

        attbuf[0] = '\0';
        if (attrib & HL_NONE) {
            Strcpy(buf, "normal");
            return buf;
        }

        if (attrib & HL_BOLD)
            Strcat(attbuf, first++ ? "+bold" : "bold");
        if (attrib & HL_INVERSE)
            Strcat(attbuf, first++ ? "+inverse" : "inverse");
        if (attrib & HL_ULINE)
            Strcat(attbuf, first++ ? "+underline" : "underline");
        if (attrib & HL_BLINK)
            Strcat(attbuf, first++ ? "+blink" : "blink");
        if (attrib & HL_DIM)
            Strcat(attbuf, first++ ? "+dim" : "dim");

        k = strlen(attbuf);
        if (k < (bufsz - 1))
            Strcpy(buf, attbuf);
        return buf;
    }
    return (char *) 0;
}


struct _status_hilite_line_str {
    int id;
    int fld;
    struct hilite_s *hl;
    unsigned long mask;
    char str[BUFSZ];
    struct _status_hilite_line_str *next;
};

struct _status_hilite_line_str *status_hilite_str =
    (struct _status_hilite_line_str *) 0;
static int status_hilite_str_id = 0;

STATIC_OVL void
status_hilite_linestr_add(fld, hl, mask, str)
int fld;
struct hilite_s *hl;
unsigned long mask;
const char *str;
{
    struct _status_hilite_line_str *tmp = (struct _status_hilite_line_str *)
        alloc(sizeof(struct _status_hilite_line_str));
    struct _status_hilite_line_str *nxt = status_hilite_str;

    (void) memset(tmp, 0, sizeof(struct _status_hilite_line_str));

    ++status_hilite_str_id;
    tmp->fld = fld;
    tmp->hl = hl;
    tmp->mask = mask;
    (void) stripchars(tmp->str, " ", str);

    tmp->id = status_hilite_str_id;

    if (nxt) {
        while (nxt && nxt->next)
            nxt = nxt->next;
        nxt->next = tmp;
    } else {
        tmp->next = (struct _status_hilite_line_str *) 0;
        status_hilite_str = tmp;
    }
}

STATIC_OVL void
status_hilite_linestr_done()
{
    struct _status_hilite_line_str *tmp = status_hilite_str;
    struct _status_hilite_line_str *nxt;

    while (tmp) {
        nxt = tmp->next;
        free(tmp);
        tmp = nxt;
    }
    status_hilite_str = (struct _status_hilite_line_str *) 0;
    status_hilite_str_id = 0;
}

STATIC_OVL int
status_hilite_linestr_countfield(fld)
int fld;
{
    struct _status_hilite_line_str *tmp = status_hilite_str;
    int count = 0;

    while (tmp) {
        if (tmp->fld == fld || fld == BL_FLUSH)
            count++;
        tmp = tmp->next;
    }
    return count;
}

int
count_status_hilites(VOID_ARGS)
{
    int count;
    status_hilite_linestr_gather();
    count = status_hilite_linestr_countfield(BL_FLUSH);
    status_hilite_linestr_done();
    return count;
}

STATIC_OVL void
status_hilite_linestr_gather_conditions()
{
    int i;
    struct _cond_map {
        unsigned long bm;
        unsigned long clratr;
    } cond_maps[SIZE(valid_conditions)];

    (void)memset(cond_maps, 0,
                 sizeof(struct _cond_map) * SIZE(valid_conditions));

    for (i = 0; i < SIZE(valid_conditions); i++) {
        int clr = NO_COLOR;
        int atr = HL_NONE;
        int j;
        for (j = 0; j < CLR_MAX; j++)
            if (cond_hilites[j] & valid_conditions[i].bitmask)
                clr = j;
        if (cond_hilites[HL_ATTCLR_DIM] & valid_conditions[i].bitmask)
            atr |= HL_DIM;
        if (cond_hilites[HL_ATTCLR_BOLD] & valid_conditions[i].bitmask)
            atr |= HL_BOLD;
        if (cond_hilites[HL_ATTCLR_BLINK] & valid_conditions[i].bitmask)
            atr |= HL_BLINK;
        if (cond_hilites[HL_ATTCLR_ULINE] & valid_conditions[i].bitmask)
            atr |= HL_ULINE;
        if (cond_hilites[HL_ATTCLR_INVERSE] & valid_conditions[i].bitmask)
            atr |= HL_INVERSE;

        if (clr != NO_COLOR || atr != HL_NONE) {
            unsigned long ca = clr | (atr << 8);
            boolean added_condmap = FALSE;
            for (j = 0; j < SIZE(valid_conditions); j++)
                if (cond_maps[j].clratr == ca) {
                    cond_maps[j].bm |= valid_conditions[i].bitmask;
                    added_condmap = TRUE;
                    break;
                }
            if (!added_condmap) {
                for (j = 0; j < SIZE(valid_conditions); j++)
                    if (!cond_maps[j].bm) {
                        cond_maps[j].bm = valid_conditions[i].bitmask;
                        cond_maps[j].clratr = ca;
                        break;
                    }
            }
        }
    }

    for (i = 0; i < SIZE(valid_conditions); i++)
        if (cond_maps[i].bm) {
            int clr = NO_COLOR, atr = HL_NONE;
            split_clridx(cond_maps[i].clratr, &clr, &atr);
            if (clr != NO_COLOR || atr != HL_NONE) {
                char clrbuf[BUFSZ];
                char attrbuf[BUFSZ];
                char condbuf[BUFSZ];
                char *tmpattr;
                (void) stripchars(clrbuf, " ", clr2colorname(clr));
                tmpattr = hlattr2attrname(atr, attrbuf, BUFSZ);
                if (tmpattr)
                    Sprintf(eos(clrbuf), "&%s", tmpattr);
                Sprintf(condbuf, "condition/%s/%s",
                        conditionbitmask2str(cond_maps[i].bm), clrbuf);
                status_hilite_linestr_add(BL_CONDITION, 0,
                                          cond_maps[i].bm, condbuf);
            }
        }
}

STATIC_OVL void
status_hilite_linestr_gather()
{
    int i;
    struct hilite_s *hl;

    status_hilite_linestr_done();

    for (i = 0; i < MAXBLSTATS; i++) {
        hl = blstats[0][i].thresholds;
        while (hl) {
            status_hilite_linestr_add(i, hl, 0UL, status_hilite2str(hl));
            hl = hl->next;
        }
    }

    status_hilite_linestr_gather_conditions();
}


char *
status_hilite2str(hl)
struct hilite_s *hl;
{
    static char buf[BUFSZ];
    int clr = 0, attr = 0;
    char behavebuf[BUFSZ];
    char clrbuf[BUFSZ];
    char attrbuf[BUFSZ];
    char *tmpattr;

    if (!hl)
        return (char *) 0;

    behavebuf[0] = '\0';
    clrbuf[0] = '\0';

    switch (hl->behavior) {
    case BL_TH_VAL_PERCENTAGE:
        if (hl->rel == LT_VALUE)
            Sprintf(behavebuf, "<%i%%", hl->value.a_int);
        else if (hl->rel == GT_VALUE)
            Sprintf(behavebuf, ">%i%%", hl->value.a_int);
        else if (hl->rel == EQ_VALUE)
            Sprintf(behavebuf, "%i%%", hl->value.a_int);
        else
            impossible("hl->behavior=percentage, rel error");
        break;
    case BL_TH_UPDOWN:
        if (hl->rel == LT_VALUE)
            Sprintf(behavebuf, "down");
        else if (hl->rel == GT_VALUE)
            Sprintf(behavebuf, "up");
        else if (hl->rel == EQ_VALUE)
            Sprintf(behavebuf, "changed");
        else
            impossible("hl->behavior=updown, rel error");
        break;
    case BL_TH_VAL_ABSOLUTE:
        if (hl->rel == LT_VALUE)
            Sprintf(behavebuf, "<%i", hl->value.a_int);
        else if (hl->rel == GT_VALUE)
            Sprintf(behavebuf, ">%i", hl->value.a_int);
        else if (hl->rel == EQ_VALUE)
            Sprintf(behavebuf, "%i", hl->value.a_int);
        else
            impossible("hl->behavior=absolute, rel error");
        break;
    case BL_TH_TEXTMATCH:
        if (hl->rel == TXT_VALUE && hl->textmatch[0])
            Sprintf(behavebuf, "%s", hl->textmatch);
        else
            impossible("hl->behavior=textmatch, rel or textmatch error");
        break;
    case BL_TH_CONDITION:
        if (hl->rel == EQ_VALUE)
            Sprintf(behavebuf, "%s", conditionbitmask2str(hl->value.a_ulong));
        else
            impossible("hl->behavior=condition, rel error");
        break;
    case BL_TH_ALWAYS_HILITE:
        Sprintf(behavebuf, "always");
        break;
    case BL_TH_NONE:
        break;
    default:
        break;
    }

    split_clridx(hl->coloridx, &clr, &attr);
    if (clr != NO_COLOR)
        (void) stripchars(clrbuf, " ", clr2colorname(clr));
    if (attr != HL_UNDEF) {
        tmpattr = hlattr2attrname(attr, attrbuf, BUFSZ);
        if (tmpattr)
            Sprintf(eos(clrbuf), "%s%s",
                    (clr != NO_COLOR) ? "&" : "",
                    tmpattr);
    }
    Sprintf(buf, "%s/%s/%s", initblstats[hl->fld].fldname, behavebuf, clrbuf);

    return buf;
}

int
status_hilite_menu_choose_field()
{
    winid tmpwin;
    int i, res, fld = BL_FLUSH;
    anything any;
    menu_item *picks = (menu_item *) 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin);

    for (i = 0; i < MAXBLSTATS; i++) {
        any = zeroany;
        any.a_int = (i+1);
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
                 initblstats[i].fldname, MENU_UNSELECTED);
    }

    end_menu(tmpwin, "Select a hilite field:");

    res = select_menu(tmpwin, PICK_ONE, &picks);
    destroy_nhwindow(tmpwin);
    if (res > 0) {
        fld = picks->item.a_int - 1;
        free((genericptr_t) picks);
    }
    return fld;
}

int
status_hilite_menu_choose_behavior(fld)
int fld;
{
    winid tmpwin;
    int res = 0, beh = BL_TH_NONE-1;
    anything any;
    menu_item *picks = (menu_item *) 0;
    char buf[BUFSZ];
    int at;
    int onlybeh = BL_TH_NONE, nopts = 0;

    if (fld <= BL_FLUSH || fld >= MAXBLSTATS)
        return BL_TH_NONE;

    at = initblstats[fld].anytype;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin);

    if (fld != BL_CONDITION) {
        any = zeroany;
        any.a_int = onlybeh = BL_TH_ALWAYS_HILITE;
        Sprintf(buf, "Always highlight %s", initblstats[fld].fldname);
        add_menu(tmpwin, NO_GLYPH, &any, 'a', 0, ATR_NONE,
                 buf, MENU_UNSELECTED);
        nopts++;
    }

    if (fld == BL_CONDITION) {
        any = zeroany;
        any.a_int = onlybeh = BL_TH_CONDITION;
        add_menu(tmpwin, NO_GLYPH, &any, 'b', 0, ATR_NONE,
                 "Bitmask of conditions", MENU_UNSELECTED);
        nopts++;
    }

    if (fld != BL_CONDITION) {
        any = zeroany;
        any.a_int = onlybeh = BL_TH_UPDOWN;
        Sprintf(buf, "%s value changes", initblstats[fld].fldname);
        add_menu(tmpwin, NO_GLYPH, &any, 'c', 0, ATR_NONE,
                 buf, MENU_UNSELECTED);
        nopts++;
    }

    if (fld != BL_CAP && fld != BL_HUNGER && (at == ANY_INT || at == ANY_LONG || at == ANY_UINT)) {
        any = zeroany;
        any.a_int = onlybeh = BL_TH_VAL_ABSOLUTE;
        add_menu(tmpwin, NO_GLYPH, &any, 'n', 0, ATR_NONE,
                 "Number threshold", MENU_UNSELECTED);
        nopts++;
    }

    if (initblstats[fld].idxmax > BL_FLUSH) {
        any = zeroany;
        any.a_int = onlybeh = BL_TH_VAL_PERCENTAGE;
        add_menu(tmpwin, NO_GLYPH, &any, 'p', 0, ATR_NONE,
                 "Percentage threshold", MENU_UNSELECTED);
        nopts++;
    }

    if (initblstats[fld].anytype == ANY_STR || fld == BL_CAP || fld == BL_HUNGER) {
        any = zeroany;
        any.a_int = onlybeh = BL_TH_TEXTMATCH;
        Sprintf(buf, "%s text match", initblstats[fld].fldname);
        add_menu(tmpwin, NO_GLYPH, &any, 't', 0, ATR_NONE,
                 buf, MENU_UNSELECTED);
        nopts++;
    }

    Sprintf(buf, "Select %s field hilite behavior:", initblstats[fld].fldname);
    end_menu(tmpwin, buf);

    if (nopts > 1) {
        res = select_menu(tmpwin, PICK_ONE, &picks);
        if (res == 0) /* none chosen*/
            beh = BL_TH_NONE;
        else if (res == -1) /* menu cancelled */
            beh = (BL_TH_NONE - 1);
    } else if (onlybeh != BL_TH_NONE)
        beh = onlybeh;
    destroy_nhwindow(tmpwin);
    if (res > 0) {
        beh = picks->item.a_int;
        free((genericptr_t) picks);
    }
    return beh;
}

int
status_hilite_menu_choose_updownboth(fld, str)
int fld;
const char *str;
{
    int res, ret = -2;
    winid tmpwin;
    char buf[BUFSZ];
    anything any;
    menu_item *picks = (menu_item *) 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin);

    if (str)
        Sprintf(buf, "%s or less", str);
    else
        Sprintf(buf, "Value goes down");
    any = zeroany;
    any.a_int = 10 + LT_VALUE;
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
             buf, MENU_UNSELECTED);

    if (str)
        Sprintf(buf, "Exactly %s", str);
    else
        Sprintf(buf, "Value changes");
    any = zeroany;
    any.a_int = 10 + EQ_VALUE;
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
             buf, MENU_UNSELECTED);

    if (str)
        Sprintf(buf, "%s or more", str);
    else
        Sprintf(buf, "Value goes up");
    any = zeroany;
    any.a_int = 10 + GT_VALUE;
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
             buf, MENU_UNSELECTED);

    Sprintf(buf, "Select field %s value:", initblstats[fld].fldname);
    end_menu(tmpwin, buf);

    res = select_menu(tmpwin, PICK_ONE, &picks);
    destroy_nhwindow(tmpwin);
    if (res > 0) {
        ret = picks->item.a_int - 10;
        free((genericptr_t) picks);
    }

    return ret;
}

STATIC_OVL boolean
status_hilite_menu_add(origfld)
int origfld;
{
    int fld;
    int behavior;
    int lt_gt_eq = 0;
    int clr = NO_COLOR, atr = HL_UNDEF;
    struct hilite_s hilite;
    unsigned long cond = 0UL;
    char colorqry[BUFSZ];
    char attrqry[BUFSZ];

choose_field:
    fld = origfld;
    if (fld == BL_FLUSH) {
        fld = status_hilite_menu_choose_field();
        if (fld == BL_FLUSH)
            return FALSE;
    }

    if (fld == BL_FLUSH)
        return FALSE;

    colorqry[0] = '\0';
    attrqry[0] = '\0';

    memset((genericptr_t) &hilite, 0, sizeof(struct hilite_s));
    hilite.set = FALSE; /* mark it "unset" */
    hilite.fld = fld;

choose_behavior:

    behavior = status_hilite_menu_choose_behavior(fld);

    if (behavior == (BL_TH_NONE-1)) {
        return FALSE;
    } else if (behavior == BL_TH_NONE) {
        if (origfld == BL_FLUSH)
            goto choose_field;
        else
            return FALSE;
    }

    hilite.behavior = behavior;

choose_value:

    if (behavior == BL_TH_VAL_PERCENTAGE
        || behavior == BL_TH_VAL_ABSOLUTE) {
        char inbuf[BUFSZ] = DUMMY, buf[BUFSZ];
        int val;
        boolean skipltgt = FALSE;
        boolean gotnum = FALSE;
        char *inp = inbuf;
        char *numstart = inbuf;

        inbuf[0] = '\0';
        Sprintf(buf, "Enter %svalue for %s threshold:",
                (behavior == BL_TH_VAL_PERCENTAGE) ? "percentage " : "",
                initblstats[fld].fldname);
        getlin(buf, inbuf);
        if (inbuf[0] == '\0' || inbuf[0] == '\033')
            goto choose_behavior;

        inp = trimspaces(inbuf);
        if (!*inp)
            goto choose_behavior;

        /* allow user to enter "<50%" or ">50" or just "50" */
        if (*inp == '>' || *inp == '<' || *inp == '=') {
            lt_gt_eq = (*inp == '>') ? GT_VALUE
                : (*inp == '<') ? LT_VALUE : EQ_VALUE;
            skipltgt = TRUE;
            *inp = ' ';
            inp++;
            numstart++;
        }
        while (digit(*inp)) {
            inp++;
            gotnum = TRUE;
        }
        if (*inp == '%') {
            behavior = BL_TH_VAL_PERCENTAGE;
            *inp = '\0';
        } else if (!*inp) {
            behavior = BL_TH_VAL_ABSOLUTE;
        } else {
            /* some random characters */
            pline("\"%s\" is not a recognized number.", inp);
            goto choose_value;
        }
        if (!gotnum) {
            pline("Is that an invisible number?");
            goto choose_value;
        }

        val = atoi(numstart);
        if (behavior == BL_TH_VAL_PERCENTAGE) {
            if (initblstats[fld].idxmax == -1) {
                pline("Field '%s' does not support percentage values.",
                      initblstats[fld].fldname);
                behavior = BL_TH_VAL_ABSOLUTE;
                goto choose_value;
            }
            if (val < 0 || val > 100) {
                pline("Not a valid percent value.");
                goto choose_value;
            }
        }

        if (!skipltgt) {
            lt_gt_eq = status_hilite_menu_choose_updownboth(fld, inbuf);
            if (lt_gt_eq == -2)
                goto choose_value;
        }

        Sprintf(colorqry, "Choose a color for when %s is %s%s:",
                initblstats[fld].fldname,
                numstart,
                (lt_gt_eq == EQ_VALUE) ? ""
                : (lt_gt_eq == LT_VALUE) ? " or less"
                : " or more");

        Sprintf(attrqry, "Choose attribute for when %s is %s%s:",
                initblstats[fld].fldname,
                inbuf,
                (lt_gt_eq == EQ_VALUE) ? ""
                : (lt_gt_eq == LT_VALUE) ? " or less"
                : " or more");

        hilite.rel = lt_gt_eq;
        hilite.value.a_int = val;
    } else if (behavior == BL_TH_UPDOWN) {
        lt_gt_eq = status_hilite_menu_choose_updownboth(fld, (char *)0);
        if (lt_gt_eq == -2)
            goto choose_behavior;
        Sprintf(colorqry, "Choose a color for when %s %s:",
                initblstats[fld].fldname,
                (lt_gt_eq == EQ_VALUE) ? "changes"
                : (lt_gt_eq == LT_VALUE) ? "decreases"
                : "increases");
        Sprintf(attrqry, "Choose attribute for when %s %s:",
                initblstats[fld].fldname,
                (lt_gt_eq == EQ_VALUE) ? "changes"
                : (lt_gt_eq == LT_VALUE) ? "decreases"
                : "increases");
        hilite.rel = lt_gt_eq;
    } else if (behavior == BL_TH_CONDITION) {
        cond = query_conditions();
        if (!cond) {
            if (origfld == BL_FLUSH)
                goto choose_field;
            else
                return FALSE;
        }
        Sprintf(colorqry, "Choose a color for conditions %s:",
                conditionbitmask2str(cond));
        Sprintf(attrqry, "Choose attribute for conditions %s:",
                conditionbitmask2str(cond));
    } else if (behavior == BL_TH_TEXTMATCH) {
        char qry_buf[BUFSZ];
        Sprintf(qry_buf, "%s %s text value to match:",
                (fld == BL_CAP
                 || fld == BL_ALIGN
                 || fld == BL_HUNGER
                 || fld == BL_TITLE) ? "Choose" : "Enter",
                initblstats[fld].fldname);
        if (fld == BL_CAP) {
            int rv = query_arrayvalue(qry_buf,
                                      enc_stat,
                                      SLT_ENCUMBER, OVERLOADED+1);
            if (rv < SLT_ENCUMBER)
                goto choose_behavior;

            hilite.rel = TXT_VALUE;
            Strcpy(hilite.textmatch, enc_stat[rv]);
        } else if (fld == BL_ALIGN) {
            const char *aligntxt[] = {"chaotic", "neutral", "lawful"};
            int rv = query_arrayvalue(qry_buf,
                                      aligntxt, 0, 3);
            if (rv < 0)
                goto choose_behavior;

            hilite.rel = TXT_VALUE;
            Strcpy(hilite.textmatch, aligntxt[rv]);
        } else if (fld == BL_HUNGER) {
            const char *hutxt[] = {"Satiated", (char *)0, "Hungry", "Weak",
                                   "Fainting", "Fainted", "Starved"};
            int rv = query_arrayvalue(qry_buf,
                                      hutxt,
                                      SATIATED, STARVED+1);
            if (rv < SATIATED)
                goto choose_behavior;

            hilite.rel = TXT_VALUE;
            Strcpy(hilite.textmatch, hutxt[rv]);
        } else if (fld == BL_TITLE) {
            const char *rolelist[9];
            int i, rv;

            for (i = 0; i < 9; i++)
                rolelist[i] = (flags.female && urole.rank[i].f)
                    ? urole.rank[i].f : urole.rank[i].m;

            rv = query_arrayvalue(qry_buf, rolelist, 0, 9);
            if (rv < 0)
                goto choose_behavior;

            hilite.rel = TXT_VALUE;
            Strcpy(hilite.textmatch, rolelist[rv]);
        } else {
            char inbuf[BUFSZ] = DUMMY;

            inbuf[0] = '\0';
            getlin(qry_buf, inbuf);
            if (inbuf[0] == '\0' || inbuf[0] == '\033')
                goto choose_behavior;

            hilite.rel = TXT_VALUE;
            if (strlen(inbuf) < QBUFSZ-1)
                Strcpy(hilite.textmatch, inbuf);
            else
                return FALSE;
        }
        Sprintf(colorqry, "Choose a color for when %s is '%s':",
                initblstats[fld].fldname, hilite.textmatch);
        Sprintf(colorqry, "Choose attribute for when %s is '%s':",
                initblstats[fld].fldname, hilite.textmatch);
    } else if (behavior == BL_TH_ALWAYS_HILITE) {
        Sprintf(colorqry, "Choose a color to always hilite %s:",
                initblstats[fld].fldname);
        Sprintf(attrqry, "Choose attribute to always hilite %s:",
                initblstats[fld].fldname);
    }

choose_color:

    clr = query_color(colorqry);
    if (clr == -1) {
        if (behavior != BL_TH_ALWAYS_HILITE)
            goto choose_value;
        else
            goto choose_behavior;
    }

    atr = query_attr(attrqry); /* FIXME: pick multiple attrs */
    if (atr == -1)
        goto choose_color;
    if (atr == ATR_DIM)
        atr = HL_DIM;
    else if (atr == ATR_BLINK)
        atr = HL_BLINK;
    else if (atr == ATR_ULINE)
        atr = HL_ULINE;
    else if (atr == ATR_INVERSE)
        atr = HL_INVERSE;
    else if (atr == ATR_BOLD)
        atr = HL_BOLD;
    else if (atr == ATR_NONE)
        atr = HL_NONE;
    else
        atr = HL_UNDEF;

    if (clr == -1)
        clr = NO_COLOR;

    if (behavior == BL_TH_CONDITION) {
        char clrbuf[BUFSZ];
        char attrbuf[BUFSZ];
        char *tmpattr;
        if (atr == HL_DIM)
            cond_hilites[HL_ATTCLR_DIM] |= cond;
        else if (atr == HL_BLINK)
            cond_hilites[HL_ATTCLR_BLINK] |= cond;
        else if (atr == HL_ULINE)
            cond_hilites[HL_ATTCLR_ULINE] |= cond;
        else if (atr == HL_INVERSE)
            cond_hilites[HL_ATTCLR_INVERSE] |= cond;
        else if (atr == HL_BOLD)
            cond_hilites[HL_ATTCLR_BOLD] |= cond;
        else if (atr == HL_NONE) {
            cond_hilites[HL_ATTCLR_DIM]     = 0UL;
            cond_hilites[HL_ATTCLR_BLINK]   = 0UL;
            cond_hilites[HL_ATTCLR_ULINE]   = 0UL;
            cond_hilites[HL_ATTCLR_INVERSE] = 0UL;
            cond_hilites[HL_ATTCLR_BOLD]    = 0UL;
        }
        cond_hilites[clr] |= cond;
        (void) stripchars(clrbuf, " ", clr2colorname(clr));
        tmpattr = hlattr2attrname(atr, attrbuf, BUFSZ);
        if (tmpattr)
            Sprintf(eos(clrbuf), "&%s", tmpattr);
        pline("Added hilite condition/%s/%s",
              conditionbitmask2str(cond), clrbuf);
    } else {
        hilite.coloridx = clr | (atr << 8);
        hilite.anytype = initblstats[fld].anytype;

        status_hilite_add_threshold(fld, &hilite);
        pline("Added hilite %s", status_hilite2str(&hilite));
    }
    reset_status_hilites();
    return TRUE;
}

boolean
status_hilite_remove(id)
int id;
{
    struct _status_hilite_line_str *hlstr = status_hilite_str;

    while (hlstr && hlstr->id != id) {
        hlstr = hlstr->next;
    }

    if (!hlstr)
        return FALSE;

    if (hlstr->fld == BL_CONDITION) {
        int i;

        for (i = 0; i < CLR_MAX; i++)
            cond_hilites[i] &= ~hlstr->mask;
        cond_hilites[HL_ATTCLR_DIM] &= ~hlstr->mask;
        cond_hilites[HL_ATTCLR_BOLD] &= ~hlstr->mask;
        cond_hilites[HL_ATTCLR_BLINK] &= ~hlstr->mask;
        cond_hilites[HL_ATTCLR_ULINE] &= ~hlstr->mask;
        cond_hilites[HL_ATTCLR_INVERSE] &= ~hlstr->mask;
        return TRUE;
    } else {
        int fld = hlstr->fld;
        struct hilite_s *hl = blstats[0][fld].thresholds;
        struct hilite_s *hlprev = (struct hilite_s *) 0;

        if (hl) {
            while (hl) {
                if (hlstr->hl == hl) {
                    if (hlprev)
                        hlprev->next = hl->next;
                    else {
                        blstats[0][fld].thresholds = hl->next;
                        blstats[1][fld].thresholds =
                            blstats[0][fld].thresholds;
                    }
                    free(hl);
                    return TRUE;
                }
                hlprev = hl;
                hl = hl->next;
            }
        }
    }
    return FALSE;
}

boolean
status_hilite_menu_fld(fld)
int fld;
{
    winid tmpwin;
    int i, res;
    menu_item *picks = (menu_item *) 0;
    anything any;
    int count = status_hilite_linestr_countfield(fld);
    struct _status_hilite_line_str *hlstr;
    char buf[BUFSZ];
    boolean acted = FALSE;

    if (!count) {
        if (status_hilite_menu_add(fld)) {
            status_hilite_linestr_done();
            status_hilite_linestr_gather();
            count = status_hilite_linestr_countfield(fld);
        } else
            return FALSE;
    }

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin);

    if (count) {
        hlstr = status_hilite_str;
        while (hlstr) {
            if (hlstr->fld == fld) {
                any = zeroany;
                any.a_int = hlstr->id;
                add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
                         hlstr->str, MENU_UNSELECTED);
            }
            hlstr = hlstr->next;
        }
    } else {
        any = zeroany;
        Sprintf(buf, "No current hilites for %s", initblstats[fld].fldname);
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, MENU_UNSELECTED);
    }

    /* separator line */
    any = zeroany;
    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);

    if (count) {
        any = zeroany;
        any.a_int = -1;
        add_menu(tmpwin, NO_GLYPH, &any, 'X', 0, ATR_NONE,
                 "Remove selected hilites", MENU_UNSELECTED);
    }

    any = zeroany;
    any.a_int = -2;
    add_menu(tmpwin, NO_GLYPH, &any, 'Z', 0, ATR_NONE,
             "Add a new hilite", MENU_UNSELECTED);


    Sprintf(buf, "Current %s hilites:", initblstats[fld].fldname);
    end_menu(tmpwin, buf);

    if ((res = select_menu(tmpwin, PICK_ANY, &picks)) > 0) {
        int mode = 0;
        for (i = 0; i < res; i++) {
            int idx = picks[i].item.a_int;
            if (idx == -1) {
                /* delete selected hilites */
                if (mode)
                    goto shlmenu_free;
                mode = -1;
                break;
            } else if (idx == -2) {
                /* create a new hilite */
                if (mode)
                    goto shlmenu_free;
                mode = -2;
                break;
            }
        }

        if (mode == -1) {
            /* delete selected hilites */
            for (i = 0; i < res; i++) {
                int idx = picks[i].item.a_int;
                if (idx > 0)
                    (void) status_hilite_remove(idx);
            }
            reset_status_hilites();
            acted = TRUE;
        } else if (mode == -2) {
            /* create a new hilite */
            if (status_hilite_menu_add(fld))
                acted = TRUE;
        }

        free((genericptr_t) picks);
    }

shlmenu_free:

    picks = (menu_item *) 0;
    destroy_nhwindow(tmpwin);
    return acted;
}

void
status_hilites_viewall()
{
    winid datawin;
    struct _status_hilite_line_str *hlstr = status_hilite_str;
    char buf[BUFSZ];

    datawin = create_nhwindow(NHW_TEXT);

    while (hlstr) {
        Sprintf(buf, "OPTIONS=hilite_status: %.*s",
                (int)(BUFSZ - sizeof "OPTIONS=hilite_status: " - 1),
                hlstr->str);
        putstr(datawin, 0, buf);
        hlstr = hlstr->next;
    }

    display_nhwindow(datawin, FALSE);
    destroy_nhwindow(datawin);
}

boolean
status_hilite_menu()
{
    winid tmpwin;
    int i, res;
    menu_item *picks = (menu_item *) 0;
    anything any;
    boolean redo;
    int countall;

shlmenu_redo:
    redo = FALSE;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin);

    status_hilite_linestr_gather();

    countall = status_hilite_linestr_countfield(BL_FLUSH);

    if (countall) {
        any = zeroany;
        any.a_int = -1;
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
                 "View all hilites in config format", MENU_UNSELECTED);

        any = zeroany;
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
    }

    for (i = 0; i < MAXBLSTATS; i++) {
        int count = status_hilite_linestr_countfield(i);
        char buf[BUFSZ];

        any = zeroany;
        any.a_int = (i+1);
        if (count)
            Sprintf(buf, "%-18s (%i defined)",
                    initblstats[i].fldname, count);
        else
            Sprintf(buf, "%-18s", initblstats[i].fldname);
        add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
                 buf, MENU_UNSELECTED);
    }


    end_menu(tmpwin, "Status hilites:");
    if ((res = select_menu(tmpwin, PICK_ONE, &picks)) > 0) {
        i = picks->item.a_int - 1;
        if (i < 0)
            status_hilites_viewall();
        else
            (void) status_hilite_menu_fld(i);
        free((genericptr_t) picks);
        redo = TRUE;
    }

    picks = (menu_item *) 0;
    destroy_nhwindow(tmpwin);
    status_hilite_linestr_done();

    if (redo)
        goto shlmenu_redo;

    return TRUE;
}

#endif /* STATUS_HILITES */

/*botl.c*/
