/* NetHack 3.7	botl.c	$NHDT-Date: 1646171622 2022/03/01 21:53:42 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.209 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2006. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#ifndef LONG_MAX
#include <limits.h>
#endif

extern const char *hu_stat[]; /* defined in eat.c */

const char *const enc_stat[] = { "",         "Burdened",  "Stressed",
                                 "Strained", "Overtaxed", "Overloaded" };

static const char *rank(void);
static void bot_via_windowport(void);
static void stat_update_time(void);
#ifdef STATUS_HILITES
static unsigned long query_conditions(void);
static boolean status_hilite_remove(int);
static boolean status_hilite_menu_fld(int);
static void status_hilites_viewall(void);
#endif

static char *
get_strength_str(void)
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

void
check_gold_symbol(void)
{
    nhsym goldch = g.showsyms[COIN_CLASS + SYM_OFF_O];

    iflags.invis_goldsym = (goldch <= (nhsym) ' ');
}

char *
do_statusline1(void)
{
    static char newbot1[BUFSZ];
    register char *nb;
    register int i, j;

    if (suppress_map_output())
        return strcpy(newbot1, "");

    Strcpy(newbot1, g.plname);
    if ('a' <= newbot1[0] && newbot1[0] <= 'z')
        newbot1[0] += 'A' - 'a';
    newbot1[10] = 0;
    Sprintf(nb = eos(newbot1), " the ");

    if (Upolyd) {
        char mbot[BUFSZ];
        int k = 0;

        Strcpy(mbot, pmname(&mons[u.umonnum], Ugender));
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
    i = g.mrank_sz + 15;
    j = (int) ((nb + 2) - newbot1); /* strlen(newbot1) but less computation */
    if ((i - j) > 0)
        Sprintf(nb = eos(nb), "%*s", i - j, " "); /* pad with spaces */

    Sprintf(nb = eos(nb), "St:%s Dx:%-1d Co:%-1d In:%-1d Wi:%-1d Ch:%-1d",
            get_strength_str(),
            ACURR(A_DEX), ACURR(A_CON), ACURR(A_INT), ACURR(A_WIS),
            ACURR(A_CHA));
    Sprintf(nb = eos(nb), "%s",
            (u.ualign.type == A_CHAOTIC)
                ? "  Chaotic"
                : (u.ualign.type == A_NEUTRAL) ? "  Neutral" : "  Lawful");
#ifdef SCORE_ON_BOTL
    if (flags.showscore)
        Sprintf(nb = eos(nb), " S:%ld", botl_score());
#endif
    return newbot1;
}

char *
do_statusline2(void)
{
    static char newbot2[BUFSZ], /* MAXCO: botl.h */
         /* dungeon location (and gold), hero health (HP, PW, AC),
            experience (HD if poly'd, else Exp level and maybe Exp points),
            time (in moves), varying number of status conditions */
         dloc[QBUFSZ], hlth[QBUFSZ], expr[QBUFSZ], tmmv[QBUFSZ], cond[QBUFSZ];
    register char *nb;
    size_t dln, dx, hln, xln, tln, cln;
    int hp, hpmax, cap;
    long money;

    if (suppress_map_output())
        return strcpy(newbot2, "");

    /*
     * Various min(x,9999)'s are to avoid having excessive values
     * violate the field width assumptions in botl.h and should not
     * impact normal play.  Particularly 64-bit long for gold which
     * could require many more digits if someone figures out a way
     * to get and carry a really large (or negative) amount of it.
     * Turn counter is also long, but we'll risk that.
     */

    /* dungeon location plus gold */
    (void) describe_level(dloc, 1); /* includes at least one trailing space */
    if ((money = money_cnt(g.invent)) < 0L)
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
        Sprintf(expr, "Xp:%d/%-1ld", u.ulevel, u.uexp);
    else
        Sprintf(expr, "Exp:%d", u.ulevel);
    xln = strlen(expr);

    /* time/move counter */
    if (flags.time)
        Sprintf(tmmv, "T:%ld", g.moves);
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
        Snprintf(newbot2, sizeof(newbot2), "%s %s %s %s %s", dloc, hlth, expr,
                 tmmv, cond);
    } else {
        if (dln + 1 + hln + 1 + xln + 1 + tln + 1 + cln + 1 > MAXCO) {
            panic("bot2: second status line exceeds MAXCO (%u > %d)",
                  (unsigned)(dln + 1 + hln + 1 + xln + 1 + tln + 1 + cln + 1), MAXCO);
        } else if ((dln - dx) + 1 + hln + 1 + xln + 1 + cln <= COLNO) {
            Snprintf(newbot2, sizeof(newbot2), "%s %s %s %s %s", dloc, hlth,
                     expr, cond, tmmv);
        } else if ((dln - dx) + 1 + hln + 1 + cln <= COLNO) {
            Snprintf(newbot2, sizeof(newbot2), "%s %s %s %s %s", dloc, hlth,
                     cond, expr, tmmv);
        } else {
            Snprintf(newbot2, sizeof(newbot2), "%s %s %s %s %s", hlth, cond,
                     dloc, expr, tmmv);
        }
        /* only two or three consecutive spaces available to squeeze out */
        mungspaces(newbot2);
    }
    return newbot2;
}

void
bot(void)
{
    /* dosave() flags completion by setting u.uhp to -1; supprss_map_output()
       covers program_state.restoring and is used for status as well as map */
    if (u.uhp != -1 && g.youmonst.data
        && iflags.status_updates && !suppress_map_output()) {
        if (VIA_WINDOWPORT()) {
            bot_via_windowport();
        } else {
            curs(WIN_STATUS, 1, 0);
            putstr(WIN_STATUS, 0, do_statusline1());
            curs(WIN_STATUS, 1, 1);
            putmixed(WIN_STATUS, 0, do_statusline2());
        }
    }
    g.context.botl = g.context.botlx = iflags.time_botl = FALSE;
}

/* special purpose status update: move counter ('time' status) only */
void
timebot(void)
{
    /* we're called when iflags.time_botl is set and general g.context.botl
       is clear; iflags.time_botl gets set whenever g.moves changes value
       so there's no benefit in tracking previous value to decide whether
       to skip update; suppress_map_output() handles program_state.restoring
       and program_state.done_hup (tty hangup => no further output at all)
       and we use it for maybe skipping status as well as for the map */
    if (flags.time && iflags.status_updates && !suppress_map_output()) {
        if (VIA_WINDOWPORT()) {
            stat_update_time();
        } else {
            /* old status display updates everything */
            bot();
        }
    }
    iflags.time_botl = FALSE;
}

/* convert experience level (1..30) to rank index (0..8) */
int
xlev_to_rank(int xlev)
{
    /*
     *   1..2  => 0
     *   3..5  => 1
     *   6..9  => 2
     *  10..13 => 3
     *      ...
     *  26..29 => 7
     *    30   => 8
     * Conversion is precise but only partially reversible.
     */
    return (xlev <= 2) ? 0 : (xlev <= 30) ? ((xlev + 2) / 4) : 8;
}

/* convert rank index (0..8) to experience level (1..30) */
int
rank_to_xlev(int rank)
{
    /*
     *  0 =>  1..2
     *  1 =>  3..5
     *  2 =>  6..9
     *  3 => 10..13
     *      ...
     *  7 => 26..29
     *  8 =>   30
     * We return the low end of each range.
     */
    return (rank < 1) ? 1 : (rank < 2) ? 3
           : (rank < 8) ? ((rank * 4) - 2) : 30;
}

const char *
rank_of(int lev, short monnum, boolean female)
{
    register const struct Role *role;
    register int i;

    /* Find the role */
    for (role = roles; role->name.m; role++)
        if (monnum == role->mnum)
            break;
    if (!role->name.m)
        role = &g.urole;

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

static const char *
rank(void)
{
    return rank_of(u.ulevel, Role_switch, flags.female);
}

int
title_to_mon(const char *str, int *rank_indx, int *title_length)
{
    register int i, j;

    /* Loop through each of the roles */
    for (i = 0; roles[i].name.m; i++) {
        /* loop through each of the rank titles for role #i */
        for (j = 0; j < 9; j++) {
            if (roles[i].rank[j].m
                && str_start_is(str, roles[i].rank[j].m, TRUE)) {
                if (rank_indx)
                    *rank_indx = j;
                if (title_length)
                    *title_length = Strlen(roles[i].rank[j].m);
                return roles[i].mnum;
            }
            if (roles[i].rank[j].f
                && str_start_is(str, roles[i].rank[j].f, TRUE)) {
                if (rank_indx)
                    *rank_indx = j;
                if (title_length)
                    *title_length = Strlen(roles[i].rank[j].f);
                return roles[i].mnum;
            }
        }
    }
    if (title_length)
        *title_length = 0;
    return NON_PM;
}

void
max_rank_sz(void)
{
    register int i;
    size_t r, maxr = 0;
    for (i = 0; i < 9; i++) {
        if (g.urole.rank[i].m && (r = strlen(g.urole.rank[i].m)) > maxr)
            maxr = r;
        if (g.urole.rank[i].f && (r = strlen(g.urole.rank[i].f)) > maxr)
            maxr = r;
    }
    g.mrank_sz = (int) maxr;
    return;
}

#ifdef SCORE_ON_BOTL
long
botl_score(void)
{
    long deepest = deepest_lev_reached(FALSE);
    long utotal;

    /* hidden_gold(False): only gold in containers whose contents are known */
    utotal = money_cnt(g.invent) + hidden_gold(FALSE);
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
describe_level(
    char *buf, /* output buffer */
    int dflgs) /* 1: append trailing space; 2: include dungeon branch name */
{
    boolean addspace = (dflgs & 1) != 0,  /* (used to be unconditional) */
            addbranch = (dflgs & 2) != 0; /* False: status, True: livelog */
    int ret = 1;

    if (Is_knox(&u.uz)) {
        Sprintf(buf, "%s", g.dungeons[u.uz.dnum].dname);
        addbranch = FALSE;
    } else if (In_quest(&u.uz)) {
        Sprintf(buf, "Home %d", dunlev(&u.uz));
    } else if (In_endgame(&u.uz)) {
        /* [3.6.2: this used to be "Astral Plane" or generic "End Game"] */
        (void) endgamelevelname(buf, depth(&u.uz));
        if (!addbranch)
            (void) strsubst(buf, "Plane of ", ""); /* just keep <element> */
        addbranch = FALSE;
    } else {
        /* ports with more room may expand this one */
        if (!addbranch)
            Sprintf(buf, "Dlvl:%-2d", depth(&u.uz));
        else
            Sprintf(buf, "level %d", depth(&u.uz));
        ret = 0;
    }
    if (addbranch) {
        Sprintf(eos(buf), ", %s", g.dungeons[u.uz.dnum].dname);
        (void) strsubst(buf, "The ", "the ");
    }
    if (addspace)
        Strcat(buf, " ");
    return ret;
}

/* =======================================================================*/
/*  statusnew routines                                                    */
/* =======================================================================*/

/* structure that tracks the status details in the core */

#ifdef STATUS_HILITES
#endif /* STATUS_HILITES */

static boolean eval_notify_windowport_field(int, boolean *, int);
static void evaluate_and_notify_windowport(boolean *, int);
static void init_blstats(void);
static int compare_blstats(struct istat_s *, struct istat_s *);
static char *anything_to_s(char *, anything *, int);
static int percentage(struct istat_s *, struct istat_s *);
static int exp_percentage(void);
static int QSORTCALLBACK cond_cmp(const genericptr, const genericptr);
static int QSORTCALLBACK menualpha_cmp(const genericptr, const genericptr);

#ifdef STATUS_HILITES
static void s_to_anything(anything *, char *, int);
static enum statusfields fldname_to_bl_indx(const char *);
static boolean hilite_reset_needed(struct istat_s *, long);
static boolean noneoftheabove(const char *);
static struct hilite_s *get_hilite(int, int, genericptr_t, int, int, int *);
static void split_clridx(int, int *, int *);
static boolean is_ltgt_percentnumber(const char *);
static boolean has_ltgt_percentnumber(const char *);
static int splitsubfields(char *, char ***, int);
static boolean is_fld_arrayvalues(const char *, const char *const *, int, int,
                                  int *);
static int query_arrayvalue(const char *, const char *const *, int, int);
static void status_hilite_add_threshold(int, struct hilite_s *);
static boolean parse_status_hl2(char (*)[QBUFSZ], boolean);
static char *conditionbitmask2str(unsigned long);
static unsigned long match_str2conditionbitmask(const char *);
static unsigned long str2conditionbitmask(char *);
static boolean parse_condition(char (*)[QBUFSZ], int);
static char *hlattr2attrname(int, char *, size_t);
static void status_hilite_linestr_add(int, struct hilite_s *, unsigned long,
                                      const char *);
static void status_hilite_linestr_done(void);
static int status_hilite_linestr_countfield(int);
static void status_hilite_linestr_gather_conditions(void);
static void status_hilite_linestr_gather(void);
static char *status_hilite2str(struct hilite_s *);
static int status_hilite_menu_choose_field(void);
static int status_hilite_menu_choose_behavior(int);
static int status_hilite_menu_choose_updownboth(int, const char *, boolean,
                                                boolean);
static boolean status_hilite_menu_add(int);
#define has_hilite(i) (g.blstats[0][(i)].thresholds)
/* TH_UPDOWN encompasses specific 'up' and 'down' also general 'changed' */
#define Is_Temp_Hilite(rule) ((rule) && (rule)->behavior == BL_TH_UPDOWN)

/* pointers to current hilite rule and list of this field's defined rules */
#define INIT_THRESH  , (struct hilite_s *) 0, (struct hilite_s *) 0
#else /* !STATUS_HILITES */
#define INIT_THRESH /*empty*/
#endif

#define INIT_BLSTAT(name, fmtstr, anytyp, wid, fld) \
    { name, fmtstr, 0L, FALSE, FALSE, 0, anytyp,                        \
      { (genericptr_t) 0 }, (char *) 0,                                 \
      wid,  -1, fld INIT_THRESH }
#define INIT_BLSTATP(name, fmtstr, anytyp, wid, maxfld, fld) \
    { name, fmtstr, 0L, FALSE, TRUE, 0, anytyp,                         \
      { (genericptr_t) 0 }, (char *) 0,                                 \
      wid,  maxfld, fld INIT_THRESH }

/* If entries are added to this, botl.h will require updating too.
   'max' value of BL_EXP gets special handling since the percentage
   involved isn't a direct 100*current/maximum calculation. */
static struct istat_s initblstats[MAXBLSTATS] = {
    INIT_BLSTAT("title", "%s", ANY_STR, MAXVALWIDTH, BL_TITLE),
    INIT_BLSTAT("strength", " St:%s", ANY_INT, 10, BL_STR),
    INIT_BLSTAT("dexterity", " Dx:%s", ANY_INT,  10, BL_DX),
    INIT_BLSTAT("constitution", " Co:%s", ANY_INT, 10, BL_CO),
    INIT_BLSTAT("intelligence", " In:%s", ANY_INT, 10, BL_IN),
    INIT_BLSTAT("wisdom", " Wi:%s", ANY_INT, 10, BL_WI),
    INIT_BLSTAT("charisma", " Ch:%s", ANY_INT, 10, BL_CH),
    INIT_BLSTAT("alignment", " %s", ANY_STR, 40, BL_ALIGN),
    INIT_BLSTAT("score", " S:%s", ANY_LONG, 20, BL_SCORE),
    INIT_BLSTAT("carrying-capacity", " %s", ANY_INT, 20, BL_CAP),
    INIT_BLSTAT("gold", " %s", ANY_LONG, 30, BL_GOLD),
    INIT_BLSTATP("power", " Pw:%s", ANY_INT, 10, BL_ENEMAX, BL_ENE),
    INIT_BLSTAT("power-max", "(%s)", ANY_INT, 10, BL_ENEMAX),
    INIT_BLSTATP("experience-level", " Xp:%s", ANY_INT, 10, BL_EXP, BL_XP),
    INIT_BLSTAT("armor-class", " AC:%s", ANY_INT, 10, BL_AC),
    INIT_BLSTAT("HD", " HD:%s", ANY_INT, 10, BL_HD),
    INIT_BLSTAT("time", " T:%s", ANY_LONG, 20, BL_TIME),
    /* hunger used to be 'ANY_UINT'; see note below in bot_via_windowport() */
    INIT_BLSTAT("hunger", " %s", ANY_INT, 40, BL_HUNGER),
    INIT_BLSTATP("hitpoints", " HP:%s", ANY_INT, 10, BL_HPMAX, BL_HP),
    INIT_BLSTAT("hitpoints-max", "(%s)", ANY_INT, 10, BL_HPMAX),
    INIT_BLSTAT("dungeon-level", "%s", ANY_STR, MAXVALWIDTH, BL_LEVELDESC),
    INIT_BLSTATP("experience", "/%s", ANY_LONG, 20, BL_EXP, BL_EXP),
    INIT_BLSTAT("condition", "%s", ANY_MASK32, 0, BL_CONDITION)
};

#undef INIT_BLSTATP
#undef INIT_BLSTAT
#undef INIT_THRESH

#ifdef STATUS_HILITES

const struct condmap condition_aliases[] = {
    { "strangled",      BL_MASK_STRNGL },
    { "all",            BL_MASK_BAREH | BL_MASK_BLIND | BL_MASK_BUSY
                        | BL_MASK_CONF | BL_MASK_DEAF | BL_MASK_ELF_IRON
                        | BL_MASK_FLY | BL_MASK_FOODPOIS | BL_MASK_GLOWHANDS
                        | BL_MASK_GRAB | BL_MASK_HALLU | BL_MASK_HELD
                        | BL_MASK_ICY | BL_MASK_INLAVA | BL_MASK_LEV
                        | BL_MASK_PARLYZ | BL_MASK_RIDE | BL_MASK_SLEEPING
                        | BL_MASK_SLIME | BL_MASK_SLIPPERY | BL_MASK_STONE
                        | BL_MASK_STRNGL | BL_MASK_STUN | BL_MASK_SUBMERGED
                        | BL_MASK_TERMILL | BL_MASK_TETHERED
                        | BL_MASK_TRAPPED | BL_MASK_UNCONSC
                        | BL_MASK_WOUNDEDL | BL_MASK_HOLDING },
    { "major_troubles", BL_MASK_FOODPOIS | BL_MASK_GRAB | BL_MASK_INLAVA
                        | BL_MASK_SLIME | BL_MASK_STONE | BL_MASK_STRNGL
                        | BL_MASK_TERMILL },
    { "minor_troubles", BL_MASK_BLIND | BL_MASK_CONF | BL_MASK_DEAF
                        | BL_MASK_HALLU | BL_MASK_PARLYZ | BL_MASK_SUBMERGED
                        | BL_MASK_STUN },
    { "movement",       BL_MASK_LEV | BL_MASK_FLY | BL_MASK_RIDE },
    { "opt_in",         BL_MASK_BAREH | BL_MASK_BUSY | BL_MASK_GLOWHANDS
                        | BL_MASK_HELD | BL_MASK_ICY | BL_MASK_PARLYZ
                        | BL_MASK_SLEEPING | BL_MASK_SLIPPERY
                        | BL_MASK_SUBMERGED | BL_MASK_TETHERED
                        | BL_MASK_TRAPPED
                        | BL_MASK_UNCONSC | BL_MASK_WOUNDEDL
                        | BL_MASK_HOLDING },
};

#endif /* STATUS_HILITES */

const struct conditions_t conditions[] = {
    /* ranking, mask, identifier, txt1, txt2, txt3 */
    { 20, BL_MASK_BAREH,     bl_bareh,     { "Bare",     "Bar",   "Bh"  } },
    { 10, BL_MASK_BLIND,     bl_blind,     { "Blind",    "Blnd",  "Bl"  } },
    { 20, BL_MASK_BUSY,      bl_busy,      { "Busy",     "Bsy",   "By"  } },
    { 10, BL_MASK_CONF,      bl_conf,      { "Conf",     "Cnf",   "Cf"  } },
    { 10, BL_MASK_DEAF,      bl_deaf,      { "Deaf",     "Def",   "Df"  } },
    { 15, BL_MASK_ELF_IRON,  bl_elf_iron,  { "Iron",     "Irn",   "Fe"  } },
    { 10, BL_MASK_FLY,       bl_fly,       { "Fly",      "Fly",   "Fl"  } },
    {  6, BL_MASK_FOODPOIS,  bl_foodpois,  { "FoodPois", "Fpois", "Poi" } },
    { 20, BL_MASK_GLOWHANDS, bl_glowhands, { "Glow",     "Glo",   "Gl"  } },
    {  2, BL_MASK_GRAB,      bl_grab,      { "Grab",     "Grb",   "Gr"  } },
    { 10, BL_MASK_HALLU,     bl_hallu,     { "Hallu",    "Hal",   "Hl"  } },
    { 20, BL_MASK_HELD,      bl_held,      { "Held",     "Hld",   "Hd"  } },
    { 20, BL_MASK_ICY,       bl_icy,       { "Icy",      "Icy",   "Ic"  } },
    {  8, BL_MASK_INLAVA,    bl_inlava,    { "Lava",     "Lav",   "La"  } },
    { 10, BL_MASK_LEV,       bl_lev,       { "Lev",      "Lev",   "Lv"  } },
    { 20, BL_MASK_PARLYZ,    bl_parlyz,    { "Parlyz",   "Para",  "Par" } },
    { 10, BL_MASK_RIDE,      bl_ride,      { "Ride",     "Rid",   "Rd"  } },
    { 20, BL_MASK_SLEEPING,  bl_sleeping,  { "Zzz",      "Zzz",   "Zz"  } },
    {  6, BL_MASK_SLIME,     bl_slime,     { "Slime",    "Slim",  "Slm" } },
    { 20, BL_MASK_SLIPPERY,  bl_slippery,  { "Slip",     "Sli",   "Sl"  } },
    {  6, BL_MASK_STONE,     bl_stone,     { "Stone",    "Ston",  "Sto" } },
    {  4, BL_MASK_STRNGL,    bl_strngl,    { "Strngl",   "Stngl", "Str" } },
    { 10, BL_MASK_STUN,      bl_stun,      { "Stun",     "Stun",  "St"  } },
    { 15, BL_MASK_SUBMERGED, bl_submerged, { "Sub",      "Sub",   "Sw"  } },
    {  6, BL_MASK_TERMILL,   bl_termill,   { "TermIll",  "Ill",   "Ill" } },
    { 20, BL_MASK_TETHERED,  bl_tethered,  { "Teth",     "Tth",   "Te"  } },
    { 20, BL_MASK_TRAPPED,   bl_trapped,   { "Trap",     "Trp",   "Tr"  } },
    { 20, BL_MASK_UNCONSC,   bl_unconsc,   { "Out",      "Out",   "KO"  } },
    { 20, BL_MASK_WOUNDEDL,  bl_woundedl,  { "Legs",     "Leg",   "Lg"  } },
    { 20, BL_MASK_HOLDING,   bl_holding,   { "UHold",    "UHld",  "UHd" } },
};

struct condtests_t condtests[CONDITION_COUNT] = {
    /* id, useropt, opt_in or out, enabled, configchoice, testresult */
    { bl_bareh,     "barehanded",  opt_in,  FALSE, FALSE, FALSE },
    { bl_blind,     "blind",       opt_out, TRUE,  FALSE, FALSE },
    { bl_busy,      "busy",        opt_in,  FALSE, FALSE, FALSE },
    { bl_conf,      "conf",        opt_out, TRUE,  FALSE, FALSE },
    { bl_deaf,      "deaf",        opt_out, TRUE,  FALSE, FALSE },
    { bl_elf_iron,  "iron",        opt_out, TRUE,  FALSE, FALSE },
    { bl_fly,       "fly",         opt_out, TRUE,  FALSE, FALSE },
    { bl_foodpois,  "foodPois",    opt_out, TRUE,  FALSE, FALSE },
    { bl_glowhands, "glowhands",   opt_in,  FALSE, FALSE, FALSE },
    { bl_grab,      "grab",        opt_out, TRUE,  FALSE, FALSE },
    { bl_hallu,     "hallucinat",  opt_out, TRUE,  FALSE, FALSE },
    { bl_held,      "held",        opt_in,  FALSE, FALSE, FALSE },
    { bl_icy,       "ice",         opt_in,  FALSE, FALSE, FALSE },
    { bl_inlava,    "lava",        opt_out, TRUE,  FALSE, FALSE },
    { bl_lev,       "levitate",    opt_out, TRUE,  FALSE, FALSE },
    { bl_parlyz,    "paralyzed",   opt_in,  FALSE, FALSE, FALSE },
    { bl_ride,      "ride",        opt_out, TRUE,  FALSE, FALSE },
    { bl_sleeping,  "sleep",       opt_in,  FALSE, FALSE, FALSE },
    { bl_slime,     "slime",       opt_out, TRUE,  FALSE, FALSE },
    { bl_slippery,  "slip",        opt_in,  FALSE, FALSE, FALSE },
    { bl_stone,     "stone",       opt_out, TRUE,  FALSE, FALSE },
    { bl_strngl,    "strngl",      opt_out, TRUE,  FALSE, FALSE },
    { bl_stun,      "stun",        opt_out, TRUE,  FALSE, FALSE },
    { bl_submerged, "submerged",   opt_in,  FALSE, FALSE, FALSE },
    { bl_termill,   "termIll",     opt_out, TRUE,  FALSE, FALSE },
    { bl_tethered,  "tethered",    opt_in,  FALSE, FALSE, FALSE },
    { bl_trapped,   "trap",        opt_in,  FALSE, FALSE, FALSE },
    { bl_unconsc,   "unconscious", opt_in,  FALSE, FALSE, FALSE },
    { bl_woundedl,  "woundedlegs", opt_in,  FALSE, FALSE, FALSE },
    { bl_holding,   "holding",     opt_in,  FALSE, FALSE, FALSE },
};
/* condition indexing */
int cond_idx[CONDITION_COUNT] = { 0 };

/* cache-related */
static boolean cache_avail[3] = { FALSE, FALSE, FALSE };
static boolean cache_reslt[3] = { FALSE, FALSE, FALSE };
static const char *cache_nomovemsg = NULL, *cache_multi_reason = NULL;

#define cond_cache_prepA()                                  \
do {                                                        \
    boolean clear_cache = FALSE, refresh_cache = FALSE;     \
                                                            \
    if (g.multi < 0) {                                      \
        if (g.nomovemsg || g.multi_reason) {                \
            if (cache_nomovemsg != g.nomovemsg)             \
                refresh_cache = TRUE;                       \
            if (cache_multi_reason != g.multi_reason)       \
                refresh_cache = TRUE;                       \
        } else {                                            \
            clear_cache = TRUE;                             \
        }                                                   \
    } else {                                                \
        clear_cache = TRUE;                                 \
    }                                                       \
    if (clear_cache) {                                      \
        cache_nomovemsg = (const char *) 0;                 \
        cache_multi_reason = (const char *) 0;              \
    }                                                       \
    if (refresh_cache) {                                    \
        cache_nomovemsg = g.nomovemsg;                      \
        cache_multi_reason = g.multi_reason;                \
    }                                                       \
    if (clear_cache || refresh_cache) {                     \
        cache_reslt[0] = cache_avail[0] = FALSE;            \
        cache_reslt[1] = cache_avail[1] = FALSE;            \
    }                                                       \
} while (0)

/* we don't put this next declaration in #ifdef STATUS_HILITES.
 * In the absence of STATUS_HILITES, each array
 * element will be 0 however, and quite meaningless,
 * but we need to pass the first array element as
 * the final argument of status_update, with or
 * without STATUS_HILITES.
 */

static void
bot_via_windowport(void)
{
    char buf[BUFSZ];
    const char *titl;
    register char *nb;
    int i, idx, cap;
    long money;

    if (!g.blinit)
        panic("bot before init.");

    /* toggle from previous iteration */
    idx = 1 - g.now_or_before_idx; /* 0 -> 1, 1 -> 0 */
    g.now_or_before_idx = idx;

    /* clear the "value set" indicators */
    (void) memset((genericptr_t) g.valset, 0, MAXBLSTATS * sizeof (boolean));

    /*
     * Note: min(x,9999) - we enforce the same maximum on hp, maxhp,
     * pw, maxpw, and gold as basic status formatting so that the two
     * modes of status display don't produce different information.
     */

    /*
     *  Player name and title.
     */
    Strcpy(nb = buf, g.plname);
    nb[0] = highc(nb[0]);
    titl = !Upolyd ? rank() : pmname(&mons[u.umonnum], Ugender);
    i = (int) (strlen(buf) + sizeof " the " + strlen(titl) - sizeof "");
    /* if "Name the Rank/monster" is too long, we truncate the name
       but always keep at least 10 characters of it; when hitpintbar is
       enabled, anything beyond 30 (long monster name) will be truncated */
    if (i > 30) {
        i = 30 - (int) (sizeof " the " + strlen(titl) - sizeof "");
        nb[max(i, 10)] = '\0';
    }
    Strcpy(nb = eos(nb), " the ");
    Strcpy(nb = eos(nb), titl);
    if (Upolyd) { /* when poly'd, capitalize monster name */
        for (i = 0; nb[i]; i++)
            if (i == 0 || nb[i - 1] == ' ')
                nb[i] = highc(nb[i]);
    }
    Sprintf(g.blstats[idx][BL_TITLE].val, "%-30s", buf);
    g.valset[BL_TITLE] = TRUE; /* indicate val already set */

    /* Strength */
    g.blstats[idx][BL_STR].a.a_int = ACURR(A_STR);
    Strcpy(g.blstats[idx][BL_STR].val, get_strength_str());
    g.valset[BL_STR] = TRUE; /* indicate val already set */

    /*  Dexterity, constitution, intelligence, wisdom, charisma. */
    g.blstats[idx][BL_DX].a.a_int = ACURR(A_DEX);
    g.blstats[idx][BL_CO].a.a_int = ACURR(A_CON);
    g.blstats[idx][BL_IN].a.a_int = ACURR(A_INT);
    g.blstats[idx][BL_WI].a.a_int = ACURR(A_WIS);
    g.blstats[idx][BL_CH].a.a_int = ACURR(A_CHA);

    /* Alignment */
    Strcpy(g.blstats[idx][BL_ALIGN].val, (u.ualign.type == A_CHAOTIC)
                                          ? "Chaotic"
                                          : (u.ualign.type == A_NEUTRAL)
                                               ? "Neutral"
                                               : "Lawful");

    /* Score */
    g.blstats[idx][BL_SCORE].a.a_long =
#ifdef SCORE_ON_BOTL
        flags.showscore ? botl_score() :
#endif
        0L;

    /*  Hit points  */
    i = Upolyd ? u.mh : u.uhp;
    if (i < 0)
        i = 0;
    g.blstats[idx][BL_HP].a.a_int = min(i, 9999);
    i = Upolyd ? u.mhmax : u.uhpmax;
    g.blstats[idx][BL_HPMAX].a.a_int = min(i, 9999);

    /*  Dungeon level. */
    (void) describe_level(g.blstats[idx][BL_LEVELDESC].val, 1);
    g.valset[BL_LEVELDESC] = TRUE; /* indicate val already set */

    /* Gold */
    if ((money = money_cnt(g.invent)) < 0L)
        money = 0L; /* ought to issue impossible() and then discard gold */
    g.blstats[idx][BL_GOLD].a.a_long = min(money, 999999L);
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
    Sprintf(g.blstats[idx][BL_GOLD].val, "%s:%ld",
            (iflags.in_dumplog || iflags.invis_goldsym) ? "$"
              : encglyph(objnum_to_glyph(GOLD_PIECE)),
            g.blstats[idx][BL_GOLD].a.a_long);
    g.valset[BL_GOLD] = TRUE; /* indicate val already set */

    /* Power (magical energy) */
    g.blstats[idx][BL_ENE].a.a_int = min(u.uen, 9999);
    g.blstats[idx][BL_ENEMAX].a.a_int = min(u.uenmax, 9999);

    /* Armor class */
    g.blstats[idx][BL_AC].a.a_int = u.uac;

    /* Monster level (if Upolyd) */
    g.blstats[idx][BL_HD].a.a_int = Upolyd ? (int) mons[u.umonnum].mlevel : 0;

    /* Experience */
    g.blstats[idx][BL_XP].a.a_int = u.ulevel;
    g.blstats[idx][BL_EXP].a.a_long = u.uexp;

    /* Time (moves) */
    g.blstats[idx][BL_TIME].a.a_long = g.moves;

    /* Hunger */
    /* note: u.uhs is unsigned, and 3.6.1's STATUS_HILITE defined
       BL_HUNGER to be ANY_UINT, but that was the only non-int/non-long
       numeric field so it's far simpler to treat it as plain int and
       not need ANY_UINT handling at all */
    g.blstats[idx][BL_HUNGER].a.a_int = (int) u.uhs;
    Strcpy(g.blstats[idx][BL_HUNGER].val,
           (u.uhs != NOT_HUNGRY) ? hu_stat[u.uhs] : "");
    g.valset[BL_HUNGER] = TRUE;

    /* Carrying capacity */
    cap = near_capacity();
    g.blstats[idx][BL_CAP].a.a_int = cap;
    Strcpy(g.blstats[idx][BL_CAP].val,
           (cap > UNENCUMBERED) ? enc_stat[cap] : "");
    g.valset[BL_CAP] = TRUE;

    /* Conditions */

    g.blstats[idx][BL_CONDITION].a.a_ulong = 0L;

    /* avoid anything that does string comparisons in here because this
       is called *extremely* often, for every screen update and the same
       string comparisons would be repeated, thus contributing toward
       performance degradation. If it is essential that string comparisons
       are needed for a particular condition, consider adding a caching
       mechanism to limit the string comparisons to the first occurrence
       for that cache lifetime. There is caching of that nature done for
       unconsc (1) and parlyz (2)  because the suggested way of being able
       to distinguish unconsc, parlyz, sleeping, and busy involves multiple
       string comparisons. */

#define test_if_enabled(c) if (condtests[(c)].enabled) condtests[(c)].test

    condtests[bl_foodpois].test = condtests[bl_termill].test = FALSE;
    if (Sick) {
        test_if_enabled(bl_foodpois) = (u.usick_type & SICK_VOMITABLE) != 0;
        test_if_enabled(bl_termill) = (u.usick_type & SICK_NONVOMITABLE) != 0;
    }
    condtests[bl_inlava].test = condtests[bl_tethered].test
        = condtests[bl_trapped].test = FALSE;
    if (u.utrap) {
        test_if_enabled(bl_inlava) = (u.utraptype == TT_LAVA);
        test_if_enabled(bl_tethered) = (u.utraptype == TT_BURIEDBALL);
        /* if in-lava or tethered is disabled and the condition applies,
           lump it in with trapped */
        test_if_enabled(bl_trapped) = (!condtests[bl_inlava].test
                                       && !condtests[bl_tethered].test);
    }
    condtests[bl_grab].test = condtests[bl_held].test
#if 0
        = condtests[bl_engulfed].test
#endif
        = condtests[bl_holding].test = FALSE;
    if (u.ustuck) {
        /* it is possible for a hero in sticks() form to be swallowed,
           so swallowed needs to be checked first; it is not possible for
           a hero in sticks() form to be held--sticky hero does the holding
           even if u.ustuck is also a holder */
        if (u.uswallow) {
            /* engulfed/swallowed isn't currently a tracked status condition;
               "held" might look odd for it but seems better than blank */
#if 0
            test_if_enabled(bl_engulfed) = TRUE;
#else
            test_if_enabled(bl_held) = TRUE;
#endif
        } else if (Upolyd && sticks(g.youmonst.data)) {
            test_if_enabled(bl_holding) = TRUE;
        } else {
            /* grab == hero is held by sea monster and about to be drowned;
               held == hero is held by something else and can't move away */
            test_if_enabled(bl_grab) = (u.ustuck->data->mlet == S_EEL);
            test_if_enabled(bl_held) = !condtests[bl_grab].test;
        }
    }
    condtests[bl_blind].test     = (Blind) ? TRUE : FALSE;
    condtests[bl_conf].test      = (Confusion) ? TRUE : FALSE;
    condtests[bl_deaf].test      = (Deaf) ? TRUE : FALSE;
    condtests[bl_fly].test       = (Flying) ? TRUE : FALSE;
    condtests[bl_glowhands].test = (u.umconf) ? TRUE : FALSE;
    condtests[bl_hallu].test     = (Hallucination) ? TRUE : FALSE;
    condtests[bl_lev].test       = (Levitation) ? TRUE : FALSE;
    condtests[bl_ride].test      = (u.usteed) ? TRUE : FALSE;
    condtests[bl_slime].test     = (Slimed) ? TRUE : FALSE;
    condtests[bl_stone].test     = (Stoned) ? TRUE : FALSE;
    condtests[bl_strngl].test    = (Strangled) ? TRUE : FALSE;
    condtests[bl_stun].test      = (Stunned) ? TRUE : FALSE;
    condtests[bl_submerged].test = (Underwater) ? TRUE : FALSE;
    test_if_enabled(bl_elf_iron) = (FALSE);
    test_if_enabled(bl_bareh)    = (!uarmg && !uwep);
    test_if_enabled(bl_icy)      = (levl[u.ux][u.uy].typ == ICE);
    test_if_enabled(bl_slippery) = (Glib) ? TRUE : FALSE;
    test_if_enabled(bl_woundedl) = (Wounded_legs) ? TRUE : FALSE;

    if (g.multi < 0) {
        cond_cache_prepA();
        if (condtests[bl_unconsc].enabled
            && cache_nomovemsg && !cache_avail[0]) {
                cache_reslt[0] = (!u.usleep && unconscious());
                cache_avail[0] = TRUE;
        }
        if (condtests[bl_parlyz].enabled
            && cache_multi_reason && !cache_avail[1]) {
                cache_reslt[1] = (!strncmp(cache_multi_reason, "paralyzed", 9)
                                 || !strncmp(cache_multi_reason, "frozen", 6));
                cache_avail[1] = TRUE;
        }
        if (cache_avail[0] && cache_reslt[0]) {
            condtests[bl_unconsc].test = cache_reslt[0];
        } else if (cache_avail[1] && cache_reslt[1]) {
            condtests[bl_parlyz].test = cache_reslt[1];
        } else if (condtests[bl_sleeping].enabled && u.usleep) {
            condtests[bl_sleeping].test = TRUE;
        } else if (condtests[bl_busy].enabled) {
            condtests[bl_busy].test = TRUE;
        }
    } else {
        condtests[bl_unconsc].test = condtests[bl_parlyz].test =
            condtests[bl_sleeping].test = condtests[bl_busy].test = FALSE;
    }

#define cond_bitset(c) \
        g.blstats[idx][BL_CONDITION].a.a_ulong |= conditions[(c)].mask;

    for (i = 0; i < CONDITION_COUNT; ++i) {
        if (condtests[i].enabled
             /* && i != bl_holding  */ /* uncomment to suppress UHold */
                && condtests[i].test)
            cond_bitset(i);
    }
    evaluate_and_notify_windowport(g.valset, idx);
}


/* update just the status lines' 'time' field */
static void
stat_update_time(void)
{
    int idx = g.now_or_before_idx; /* no 0/1 toggle */
    int fld = BL_TIME;

    /* Time (moves) */
    g.blstats[idx][fld].a.a_long = g.moves;
    g.valset[fld] = FALSE;

    eval_notify_windowport_field(fld, g.valset, idx);
    if ((windowprocs.wincap2 & WC2_FLUSH_STATUS) != 0L)
        status_update(BL_FLUSH, (genericptr_t) 0, 0, 0,
                      NO_COLOR, (unsigned long *) 0);
    return;
}

/* deal with player's choice to change processing of a condition */
void
condopt(int idx, boolean *addr, boolean negated)
{
    int i;

    /* sanity check */
    if ((idx < 0 || idx >= CONDITION_COUNT)
        || (addr && addr != &condtests[idx].choice))
        return;

    if (!addr) {
        /* special: indicates a request to init so
           set the choice values to match the defaults */
        g.condmenu_sortorder = 0;
        for (i = 0; i < CONDITION_COUNT; ++i) {
            cond_idx[i] = i;
            condtests[i].choice = condtests[i].enabled;
        }
        qsort((genericptr_t) cond_idx, CONDITION_COUNT,
              sizeof cond_idx[0], cond_cmp);
    } else {
        /* (addr == &condtests[idx].choice) */
        condtests[idx].enabled = negated ? FALSE : TRUE;
        condtests[idx].choice = condtests[idx].enabled;
        /* avoid lingering false positives if test is no longer run */
        if (!condtests[idx].enabled)
            condtests[idx].test = FALSE;
    }
}

/* qsort callback routine for sorting the condition index */
static int QSORTCALLBACK
cond_cmp(const genericptr vptr1, const genericptr vptr2)
{
    int indx1 = *(int *) vptr1, indx2 = *(int *) vptr2,
        c1 = conditions[indx1].ranking, c2 = conditions[indx2].ranking;

    if (c1 != c2)
        return c1 - c2;
    /* tie-breaker - visible alpha by name */
    return strcmpi(condtests[indx1].useroption, condtests[indx2].useroption);
}

/* qsort callback routine for alphabetical sorting of index */
static int QSORTCALLBACK
menualpha_cmp(const genericptr vptr1, const genericptr vptr2)
{
    int indx1 = *(int *) vptr1, indx2 = *(int *) vptr2;

    return strcmpi(condtests[indx1].useroption, condtests[indx2].useroption);
}

int
parse_cond_option(boolean negated, char *opts)
{
    int i, sl;
    const char *compareto, *uniqpart, prefix[] = "cond_";

    if (!opts || strlen(opts) <= sizeof prefix - 1)
        return 2;
    uniqpart = opts + (sizeof prefix - 1);
    for (i = 0; i < CONDITION_COUNT; ++i) {
        compareto = condtests[i].useroption;
        sl = Strlen(compareto);
        if (match_optname(uniqpart, compareto, (sl >= 4) ? 4 : sl, FALSE)) {
            condopt(i, &condtests[i].choice, negated);
            return 0;
        }
    }
    return 1;  /* !0 indicates error */
}

void
cond_menu(void)
{
    static const char *const menutitle[2] = { "alphabetically", "by ranking"};
    int i, res, idx = 0;
    int sequence[CONDITION_COUNT];
    winid tmpwin;
    anything any;
    menu_item *picks = (menu_item *) 0;
    char mbuf[QBUFSZ];
    boolean showmenu = TRUE;
    int clr = 0;

    do {
        for (i = 0; i < CONDITION_COUNT; ++i) {
            sequence[i] = i;
        }
        qsort((genericptr_t) sequence, CONDITION_COUNT,
              sizeof sequence[0],
              (g.condmenu_sortorder) ? cond_cmp : menualpha_cmp);

        tmpwin = create_nhwindow(NHW_MENU);
        start_menu(tmpwin, MENU_BEHAVE_STANDARD);

        any = cg.zeroany;
        any.a_int = 1;
        Sprintf(mbuf, "change sort order from \"%s\" to \"%s\"",
                menutitle[g.condmenu_sortorder],
                menutitle[1 - g.condmenu_sortorder]);
        add_menu(tmpwin, &nul_glyphinfo, &any, 'S', 0, ATR_NONE,
                 clr, mbuf, MENU_ITEMFLAGS_NONE);
        any = cg.zeroany;
        Sprintf(mbuf, "sorted %s", menutitle[g.condmenu_sortorder]);
        add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0,
                 iflags.menu_headings, clr, mbuf, MENU_ITEMFLAGS_NONE);
        for (i = 0; i < SIZE(condtests); i++) {
            idx = sequence[i];
            Sprintf(mbuf, "cond_%-14s", condtests[idx].useroption);
            any = cg.zeroany;
            any.a_int = idx + 2; /* avoid zero and the sort change pick */
            condtests[idx].choice = FALSE;
            add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                     clr, mbuf,
                     condtests[idx].enabled
                        ? MENU_ITEMFLAGS_SELECTED : MENU_ITEMFLAGS_NONE);
        }

        end_menu(tmpwin, "Choose status conditions to toggle");

        res = select_menu(tmpwin, PICK_ANY, &picks);
        destroy_nhwindow(tmpwin);
        showmenu = FALSE;
        if (res > 0) {
            for (i = 0; i < res; i++) {
                idx = picks[i].item.a_int;
                if (idx == 1) {
                   /* sort change requested */
                   g.condmenu_sortorder = 1 - g.condmenu_sortorder;
                   showmenu = TRUE;
                   break;       /* for loop */
                } else {
                    idx -= 2;
                    condtests[idx].choice = TRUE;
                }
            }
            free((genericptr_t) picks);
        }
    } while (showmenu);

    if (res > 0) {
        for (i = 0; i < CONDITION_COUNT; ++i)
            if (condtests[i].enabled != condtests[i].choice) {
                condtests[i].enabled = condtests[i].choice;
                g.context.botl = TRUE;
            }
    }
}

static boolean
eval_notify_windowport_field(int fld, boolean *valsetlist, int idx)
{
    static int oldrndencode = 0;
    static nhsym oldgoldsym = 0;
    int pc, chg, color = NO_COLOR;
    unsigned anytype;
    boolean updated = FALSE, reset;
    struct istat_s *curr, *prev;
    enum statusfields fldmax;

    /*
     *  Now pass the changed values to window port.
     */
    anytype = g.blstats[idx][fld].anytype;
    curr = &g.blstats[idx][fld];
    prev = &g.blstats[1 - idx][fld];
    color = NO_COLOR;

    chg = g.update_all ? 0 : compare_blstats(prev, curr);
    /*
     * TODO:
     *  Dynamically update 'percent_matters' as rules are added or
     *  removed to track whether any of them are precentage rules.
     *  Then there'll be no need to assume that non-Null 'thresholds'
     *  means that percentages need to be kept up to date.
     *  [Affects exp_percent_changing() too.]
     */
    if (((chg || g.update_all || fld == BL_XP)
         && curr->percent_matters
#ifdef STATUS_HILITES
         && curr->thresholds
#endif
        )
        /* when 'hitpointbar' is On, percent matters even if HP
           hasn't changed and has no percentage rules (in case HPmax
           has changed when HP hasn't, where we ordinarily wouldn't
           update HP so would miss an update of the hitpoint bar) */
        || (fld == BL_HP && iflags.wc2_hitpointbar)) {
        fldmax = curr->idxmax;
        pc = (fldmax == BL_EXP) ? exp_percentage()
             : (fldmax >= 0) ? percentage(curr, &g.blstats[idx][fldmax])
               : 0; /* bullet proofing; can't get here */
        if (pc != prev->percent_value)
            chg = 1;
        curr->percent_value = pc;
    } else {
        pc = 0;
    }

    /* Temporary? hack: moveloop()'s prolog for a new game sets
     * g.context.rndencode after the status window has been init'd,
     * so $:0 has already been encoded and cached by the window
     * port.  Without this hack, gold's \G sequence won't be
     * recognized and ends up being displayed as-is for 'g.update_all'.
     *
     * Also, even if g.context.rndencode hasn't changed and the
     * gold amount itself hasn't changed, the glyph portion of the
     * encoding may have changed if a new symset was put into effect.
     *
     *  \GXXXXNNNN:25
     *  XXXX = the g.context.rndencode portion
     *  NNNN = the glyph portion
     *  25   = the gold amount
     *
     * Setting 'chg = 2' is enough to render the field properly, but
     * not to honor an initial highlight, so force 'g.update_all = TRUE'.
     */
    if (fld == BL_GOLD
        && (g.context.rndencode != oldrndencode
            || g.showsyms[COIN_CLASS + SYM_OFF_O] != oldgoldsym)) {
        g.update_all = TRUE; /* chg = 2; */
        oldrndencode = g.context.rndencode;
        oldgoldsym = g.showsyms[COIN_CLASS + SYM_OFF_O];
    }

    reset = FALSE;
#ifdef STATUS_HILITES
    if (!g.update_all && !chg && curr->time) {
        reset = hilite_reset_needed(prev, g.bl_hilite_moves);
        if (reset)
            curr->time = prev->time = 0L;
    }
#endif

   if (g.update_all || chg || reset) {
        if (!valsetlist[fld])
            (void) anything_to_s(curr->val, &curr->a, anytype);

        if (anytype != ANY_MASK32) {
#ifdef STATUS_HILITES
            if (chg || *curr->val) {
                /* if Xp percentage changed, we set 'chg' to 1 above;
                   reset that if the Xp value hasn't actually changed
                   or possibly went down rather than up (level loss) */
                if (chg == 1 && fld == BL_XP)
                    chg = compare_blstats(prev, curr);

                curr->hilite_rule = get_hilite(idx, fld,
                                               (genericptr_t) &curr->a,
                                               chg, pc, &color);
                prev->hilite_rule = curr->hilite_rule;
                if (chg == 2) {
                    color = NO_COLOR;
                    chg = 0;
                }
            }
#endif /* STATUS_HILITES */
            status_update(fld, (genericptr_t) curr->val,
                          chg, pc, color, (unsigned long *) 0);
        } else {
            /* Color for conditions is done through g.cond_hilites[] */
            status_update(fld, (genericptr_t) &curr->a.a_ulong,
                          chg, pc, color, g.cond_hilites);
        }
        curr->chg = prev->chg = TRUE;
        updated = TRUE;
    }
    return updated;
}

static void
evaluate_and_notify_windowport(boolean *valsetlist, int idx)
{
    int i, updated = 0, notpresent = 0;

    /*
     *  Now pass the changed values to window port.
     */
    for (i = 0; i < MAXBLSTATS; i++) {
        if (((i == BL_SCORE) && !flags.showscore)
            || ((i == BL_EXP) && !flags.showexp)
            || ((i == BL_TIME) && !flags.time)
            || ((i == BL_HD) && !Upolyd)
            || ((i == BL_XP || i == BL_EXP) && Upolyd)) {
            notpresent++;
            continue;
        }
        if (eval_notify_windowport_field(i, valsetlist, idx))
            updated++;
    }
    /*
     * Notes:
     *  1. It is possible to get here, with nothing having been pushed
     *     to the window port, when none of the info has changed.
     *
     *  2. Some window ports are also known to optimize by only drawing
     *     fields that have changed since the previous update.
     *
     * In both of those situations, we need to force updates to
     * all of the fields when g.context.botlx is set. The tty port in
     * particular has a problem if that isn't done, since the core sets
     * g.context.botlx when a menu or text display obliterates the status
     * line.
     *
     * For those situations, to trigger the full update of every field
     * whether changed or not, call status_update() with BL_RESET.
     *
     * For regular processing and to notify the window port that a
     * bot() round has finished and it's time to trigger a flush of
     * all buffered changes received thus far but not reflected in
     * the display, call status_update() with BL_FLUSH.
     *
     */
    if (g.context.botlx && (windowprocs.wincap2 & WC2_RESET_STATUS) != 0L)
        status_update(BL_RESET, (genericptr_t) 0, 0, 0,
                      NO_COLOR, (unsigned long *) 0);
    else if ((updated || g.context.botlx)
             && (windowprocs.wincap2 & WC2_FLUSH_STATUS) != 0L)
        status_update(BL_FLUSH, (genericptr_t) 0, 0, 0,
                      NO_COLOR, (unsigned long *) 0);

    g.context.botl = g.context.botlx = iflags.time_botl = FALSE;
    g.update_all = FALSE;
}

void
status_initialize(
    boolean reassessment) /* TRUE: just recheck fields without other
                           * initialization */
{
    enum statusfields fld;
    boolean fldenabl;
    int i;
    const char *fieldfmt, *fieldname;

    if (!reassessment) {
        if (g.blinit)
            impossible("2nd status_initialize with full init.");
        init_blstats();
        (*windowprocs.win_status_init)();
        g.blinit = TRUE;
    } else if (!g.blinit) {
        panic("status 'reassess' before init");
    }
    for (i = 0; i < MAXBLSTATS; ++i) {
        fld = initblstats[i].fld;
        fldenabl = (fld == BL_SCORE) ? flags.showscore
                   : (fld == BL_TIME) ? flags.time
                     : (fld == BL_EXP) ? (boolean) (flags.showexp && !Upolyd)
                       : (fld == BL_XP) ? (boolean) !Upolyd
                         : (fld == BL_HD) ? (boolean) Upolyd
                           : TRUE;

        fieldname = initblstats[i].fldname;
        fieldfmt = (fld == BL_TITLE && iflags.wc2_hitpointbar) ? "%-30.30s"
                   : initblstats[i].fldfmt;
        status_enablefield(fld, fieldname, fieldfmt, fldenabl);
    }
    g.update_all = TRUE;
    g.context.botlx = TRUE;
}

void
status_finish(void)
{
    int i;

    /* call the window port cleanup routine first */
    if (windowprocs.win_status_finish)
        (*windowprocs.win_status_finish)();

    /* free memory that we alloc'd now */
    for (i = 0; i < MAXBLSTATS; ++i) {
        if (g.blstats[0][i].val)
            free((genericptr_t) g.blstats[0][i].val), g.blstats[0][i].val = 0;
        if (g.blstats[1][i].val)
            free((genericptr_t) g.blstats[1][i].val), g.blstats[1][i].val = 0;
#ifdef STATUS_HILITES
        /* pointer to an entry in thresholds list; Null it out since
           that list is about to go away */
        g.blstats[0][i].hilite_rule = g.blstats[1][i].hilite_rule = 0;
        if (g.blstats[0][i].thresholds) {
            struct hilite_s *temp, *next;

            for (temp = g.blstats[0][i].thresholds; temp; temp = next) {
                next = temp->next;
                free((genericptr_t) temp);
            }
            g.blstats[0][i].thresholds = g.blstats[1][i].thresholds = 0;
        }
#endif /* STATUS_HILITES */
    }
}

static void
init_blstats(void)
{
    static boolean initalready = FALSE;
    int i, j;

    if (initalready) {
        impossible("init_blstats called more than once.");
        return;
    }
    for (i = 0; i <= 1; ++i) {
        for (j = 0; j < MAXBLSTATS; ++j) {
#ifdef STATUS_HILITES
            struct hilite_s *keep_hilite_chain = g.blstats[i][j].thresholds;
#endif

            g.blstats[i][j] = initblstats[j];
            g.blstats[i][j].a = cg.zeroany;
            if (g.blstats[i][j].valwidth) {
                g.blstats[i][j].val = (char *) alloc(g.blstats[i][j].valwidth);
                g.blstats[i][j].val[0] = '\0';
            } else
                g.blstats[i][j].val = (char *) 0;
#ifdef STATUS_HILITES
            g.blstats[i][j].thresholds = keep_hilite_chain;
#endif
        }
    }
    initalready = TRUE;
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
static int
compare_blstats(struct istat_s *bl1, struct istat_s*bl2)
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

static char *
anything_to_s(char *buf, anything *a, int anytype)
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
static void
s_to_anything(anything *a, char *buf, int anytype)
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

static int
percentage(struct istat_s *bl, struct istat_s *maxbl)
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
    if (result == 0
        && (ival != 0 || lval != 0L || uval != 0U || ulval != 0UL))
        result = 1;

    return result;
}

/* percentage for both xp (level) and exp (points) is the percentage for
   (curr_exp - this_level_start) in (next_level_start - this_level_start) */
static int
exp_percentage(void)
{
    int res = 0;

    if (u.ulevel < 30) {
        long exp_val, nxt_exp_val, curlvlstart;

        curlvlstart = newuexp(u.ulevel - 1);
        exp_val = u.uexp - curlvlstart;
        nxt_exp_val = newuexp(u.ulevel) - curlvlstart;
        if (exp_val == nxt_exp_val - 1L) {
            /*
             * Full 100% is unattainable since hero gains a level
             * and the threshold for next level increases, but treat
             * (next_level_start - 1 point) as a special case.  It's a
             * key value after being level drained so is something that
             * some players would like to be able to highlight distinctly.
             */
            res = 100;
        } else {
            struct istat_s curval, maxval;

            curval.anytype = maxval.anytype = ANY_LONG;
            curval.a = maxval.a = cg.zeroany;
            curval.a.a_long = exp_val;
            maxval.a.a_long = nxt_exp_val;
            /* maximum delta between levels is 10000000; calculation of
               100 * (10000000 - N) / 10000000 fits within 32-bit long */
            res = percentage(&curval, &maxval);
        }
    }
    return res;
}

/* experience points have changed but experience level hasn't; decide whether
   botl update is needed for a different percentage highlight rule for Xp */
boolean
exp_percent_changing(void)
{
    int pc;
    anything a;
#ifdef STATUS_HILITES
    int color_dummy;
    struct hilite_s *rule;
#endif
    struct istat_s *curr;

    /* if status update is already requested, skip this processing */
    if (!g.context.botl) {
        /*
         * Status update is warranted iff percent integer changes and the new
         * percentage results in a different highlighting rule being selected.
         */
        curr = &g.blstats[g.now_or_before_idx][BL_XP];
        /* TODO: [see eval_notify_windowport_field() about percent_matters
           and the check against 'thresholds'] */
        if (curr->percent_matters
#ifdef STATUS_HILITES
            && curr->thresholds
#endif
            && (pc = exp_percentage()) != curr->percent_value) {
            a = cg.zeroany;
            a.a_int = (int) u.ulevel;
#ifdef STATUS_HILITES
            rule = get_hilite(g.now_or_before_idx, BL_XP,
                              (genericptr_t) &a, 0, pc, &color_dummy);
            if (rule != curr->hilite_rule)
                return TRUE; /* caller should set 'g.context.botl' to True */
#endif
        }
    }
    return FALSE;
}

/* callback so that interface can get capacity index rather than trying
   to reconstruct that from the encumbrance string or asking the general
   core what the value is */
int
stat_cap_indx(void)
{
    int cap;

#ifdef STATUS_HILITES
    cap = g.blstats[g.now_or_before_idx][BL_CAP].a.a_int;
#else
    cap = near_capacity();
#endif
    return cap;
}

/* callback so that interface can get hunger index rather than trying to
   reconstruct that from the hunger string or dipping into core internals */
int
stat_hunger_indx(void)
{
    int uhs;

#ifdef STATUS_HILITES
    uhs = g.blstats[g.now_or_before_idx][BL_HUNGER].a.a_int;
#else
    uhs = (int) u.uhs;
#endif
    return uhs;
}

/* used by X11 for "tty status" even when STATUS_HILITES is disabled */
const char *
bl_idx_to_fldname(int idx)
{
    if (idx >= 0 && idx < MAXBLSTATS)
        return initblstats[idx].fldname;
    return (const char *) 0;
}

#ifdef STATUS_HILITES

/****************************************************************************/
/* Core status hiliting support */
/****************************************************************************/

static struct fieldid_t {
    const char *fieldname;
    enum statusfields fldid;
} fieldids_alias[] = {
    { "characteristics",   BL_CHARACTERISTICS },
    { "encumbrance",       BL_CAP },
    { "experience-points", BL_EXP },
    { "dx",       BL_DX },
    { "co",       BL_CO },
    { "con",      BL_CO },
    { "points",   BL_SCORE },
    { "cap",      BL_CAP },
    { "pw",       BL_ENE },
    { "pw-max",   BL_ENEMAX },
    { "xl",       BL_XP },
    { "xplvl",    BL_XP },
    { "ac",       BL_AC },
    { "hit-dice", BL_HD },
    { "turns",    BL_TIME },
    { "hp",       BL_HP },
    { "hp-max",   BL_HPMAX },
    { "dgn",      BL_LEVELDESC },
    { "xp",       BL_EXP },
    { "exp",      BL_EXP },
    { "flags",    BL_CONDITION },
    {0,           BL_FLUSH }
};

/* format arguments */
static const char threshold_value[] = "hilite_status threshold ",
                  is_out_of_range[] = " is out of range";


/* field name to bottom line index */
static enum statusfields
fldname_to_bl_indx(const char *name)
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

static boolean
hilite_reset_needed(struct istat_s *bl_p,
                    long augmented_time) /* no longer augmented; it once
                                          * encoded fractional amounts for
                                          * multiple moves within same turn */
{
    /*
     * This 'multi' handling may need some tuning...
     */
    if (g.multi)
        return FALSE;

    if (!Is_Temp_Hilite(bl_p->hilite_rule))
        return FALSE;

    if (bl_p->time == 0 || bl_p->time >= augmented_time)
        return FALSE;

    return TRUE;
}

/* called from moveloop(); sets context.botl if temp hilites have timed out */
void
status_eval_next_unhilite(void)
{
    int i;
    struct istat_s *curr;
    long next_unhilite, this_unhilite;

    g.bl_hilite_moves = g.moves; /* simpllfied; at one point we used to try
                                  * to encode fractional amounts for multiple
                                  * moves within same turn */
    /* figure out whether an unhilight needs to be performed now */
    next_unhilite = 0L;
    for (i = 0; i < MAXBLSTATS; ++i) {
        curr = &g.blstats[0][i]; /* blstats[0][*].time == blstats[1][*].time */

        if (curr->chg) {
            struct istat_s *prev = &g.blstats[1][i];

            if (Is_Temp_Hilite(curr->hilite_rule))
                curr->time = prev->time = (g.bl_hilite_moves
                                           + iflags.hilite_delta);
            else
                curr->time = prev->time = 0L;

            curr->chg = prev->chg = FALSE;
            g.context.botl = TRUE;
        }
        if (g.context.botl)
            continue; /* just process other g.blstats[][].time and .chg */

        this_unhilite = curr->time;
        if (this_unhilite > 0L
            && (next_unhilite == 0L || this_unhilite < next_unhilite)
            && hilite_reset_needed(curr, this_unhilite + 1L)) {
            next_unhilite = this_unhilite;
            if (next_unhilite < g.bl_hilite_moves)
                g.context.botl = TRUE;
        }
    }
}

/* called by options handling when 'statushilites' value is changed */
void
reset_status_hilites(void)
{
    if (iflags.hilite_delta) {
        int i;

        for (i = 0; i < MAXBLSTATS; ++i)
            g.blstats[0][i].time = g.blstats[1][i].time = 0L;
        g.update_all = TRUE;
    }
    g.context.botlx = TRUE;
}

/* test whether the text from a title rule matches the string for
   title-while-polymorphed in the 'textmatch' menu */
static boolean
noneoftheabove(const char *hl_text)
{
    if (fuzzymatch(hl_text, "none of the above", "\" -_", TRUE)
        || fuzzymatch(hl_text, "(polymorphed)", "\"()", TRUE)
        || fuzzymatch(hl_text, "none of the above (polymorphed)",
                      "\" -_()", TRUE))
        return TRUE;
    return FALSE;
}

/*
 * get_hilite
 *
 * Returns, based on the value and the direction it is moving,
 * the highlight rule that applies to the specified field.
 *
 * Provide get_hilite() with the following to work with:
 *     actual value vp
 *          useful for BL_TH_VAL_ABSOLUTE
 *     indicator of down, up, or the same (-1, 1, 0) chg
 *          useful for BL_TH_UPDOWN or change detection
 *     percentage (current value percentage of max value) pc
 *          useful for BL_TH_VAL_PERCENTAGE
 *
 * Get back:
 *     pointer to rule that applies; Null if no rule does.
 */
static struct hilite_s *
get_hilite(int idx, int fldidx, genericptr_t vp, int chg, int pc,
           int *colorptr)
{
    struct hilite_s *hl, *rule = 0;
    anything *value = (anything *) vp;
    char *txtstr;

    if (fldidx < 0 || fldidx >= MAXBLSTATS)
        return (struct hilite_s *) 0;

    if (has_hilite(fldidx)) {
        int dt;
        /* there are hilites set here */
        int max_pc = -1, min_pc = 101;
        /* LARGEST_INT isn't INT_MAX; it fits within 16 bits, but that
           value is big enough to handle all 'int' status fields */
        int max_ival = -LARGEST_INT, min_ival = LARGEST_INT;
        /* LONG_MAX comes from <limits.h> which might not be available for
           ancient configurations; we don't need LONG_MIN */
        long max_lval = -LONG_MAX, min_lval = LONG_MAX;
        boolean exactmatch = FALSE, updown = FALSE, changed = FALSE,
                perc_or_abs = FALSE;

        /* min_/max_ are used to track best fit */
        for (hl = g.blstats[0][fldidx].thresholds; hl; hl = hl->next) {
            dt = initblstats[fldidx].anytype; /* only needed for 'absolute' */
            /* if we've already matched a temporary highlight, it takes
               precedence over all persistent ones; we still process
               updown rules to get the last one which qualifies */
            if ((updown || changed) && hl->behavior != BL_TH_UPDOWN)
                continue;
            /* among persistent highlights, if a 'percentage' or 'absolute'
               rule has been matched, it takes precedence over 'always' */
            if (perc_or_abs && hl->behavior == BL_TH_ALWAYS_HILITE)
                continue;

            switch (hl->behavior) {
            case BL_TH_VAL_PERCENTAGE: /* percent values are always ANY_INT */
                if (hl->rel == EQ_VALUE && pc == hl->value.a_int) {
                    rule = hl;
                    min_pc = max_pc = hl->value.a_int;
                    exactmatch = perc_or_abs = TRUE;
                } else if (exactmatch) {
                    ; /* already found best fit, skip lt,ge,&c */
                } else if (hl->rel == LT_VALUE
                           && (pc < hl->value.a_int)
                           && (hl->value.a_int <= min_pc)) {
                    rule = hl;
                    min_pc = hl->value.a_int;
                    perc_or_abs = TRUE;
                } else if (hl->rel == LE_VALUE
                           && (pc <= hl->value.a_int)
                           && (hl->value.a_int <= min_pc)) {
                    rule = hl;
                    min_pc = hl->value.a_int;
                    perc_or_abs = TRUE;
                } else if (hl->rel == GT_VALUE
                           && (pc > hl->value.a_int)
                           && (hl->value.a_int >= max_pc)) {
                    rule = hl;
                    max_pc = hl->value.a_int;
                    perc_or_abs = TRUE;
                } else if (hl->rel == GE_VALUE
                           && (pc >= hl->value.a_int)
                           && (hl->value.a_int >= max_pc)) {
                    rule = hl;
                    max_pc = hl->value.a_int;
                    perc_or_abs = TRUE;
                }
                break;
            case BL_TH_UPDOWN: /* uses 'chg' (set by caller), not 'dt' */
                /* specific 'up' or 'down' takes precedence over general
                   'changed' regardless of their order in the rule set */
                if (chg < 0 && hl->rel == LT_VALUE) {
                    rule = hl;
                    updown = TRUE;
                } else if (chg > 0 && hl->rel == GT_VALUE) {
                    rule = hl;
                    updown = TRUE;
                } else if (chg != 0 && hl->rel == EQ_VALUE && !updown) {
                    rule = hl;
                    changed = TRUE;
                }
                break;
            case BL_TH_VAL_ABSOLUTE: /* either ANY_INT or ANY_LONG */
                /*
                 * The int and long variations here are identical aside from
                 * union field and min_/max_ variable names.  If you change
                 * one, be sure to make a corresponding change in the other.
                 */
                if (dt == ANY_INT) {
                    if (hl->rel == EQ_VALUE
                        && hl->value.a_int == value->a_int) {
                        rule = hl;
                        min_ival = max_ival = hl->value.a_int;
                        exactmatch = perc_or_abs = TRUE;
                    } else if (exactmatch) {
                        ; /* already found best fit, skip lt,ge,&c */
                    } else if (hl->rel == LT_VALUE
                               && (value->a_int < hl->value.a_int)
                               && (hl->value.a_int <= min_ival)) {
                        rule = hl;
                        min_ival = hl->value.a_int;
                        perc_or_abs = TRUE;
                    } else if (hl->rel == LE_VALUE
                               && (value->a_int <= hl->value.a_int)
                               && (hl->value.a_int <= min_ival)) {
                        rule = hl;
                        min_ival = hl->value.a_int;
                        perc_or_abs = TRUE;
                    } else if (hl->rel == GT_VALUE
                               && (value->a_int > hl->value.a_int)
                               && (hl->value.a_int >= max_ival)) {
                        rule = hl;
                        max_ival = hl->value.a_int;
                        perc_or_abs = TRUE;
                    } else if (hl->rel == GE_VALUE
                               && (value->a_int >= hl->value.a_int)
                               && (hl->value.a_int >= max_ival)) {
                        rule = hl;
                        max_ival = hl->value.a_int;
                        perc_or_abs = TRUE;
                    }
                } else { /* ANY_LONG */
                    if (hl->rel == EQ_VALUE
                        && hl->value.a_long == value->a_long) {
                        rule = hl;
                        min_lval = max_lval = hl->value.a_long;
                        exactmatch = perc_or_abs = TRUE;
                    } else if (exactmatch) {
                        ; /* already found best fit, skip lt,ge,&c */
                    } else if (hl->rel == LT_VALUE
                               && (value->a_long < hl->value.a_long)
                               && (hl->value.a_long <= min_lval)) {
                        rule = hl;
                        min_lval = hl->value.a_long;
                        perc_or_abs = TRUE;
                    } else if (hl->rel == LE_VALUE
                               && (value->a_long <= hl->value.a_long)
                               && (hl->value.a_long <= min_lval)) {
                        rule = hl;
                        min_lval = hl->value.a_long;
                        perc_or_abs = TRUE;
                    } else if (hl->rel == GT_VALUE
                               && (value->a_long > hl->value.a_long)
                               && (hl->value.a_long >= max_lval)) {
                        rule = hl;
                        max_lval = hl->value.a_long;
                        perc_or_abs = TRUE;
                    } else if (hl->rel == GE_VALUE
                               && (value->a_long >= hl->value.a_long)
                               && (hl->value.a_long >= max_lval)) {
                        rule = hl;
                        max_lval = hl->value.a_long;
                        perc_or_abs = TRUE;
                    }
                }
                break;
            case BL_TH_TEXTMATCH: /* ANY_STR */
                txtstr = g.blstats[idx][fldidx].val;
                if (fldidx == BL_TITLE)
                    /* "<name> the <rank-title>", skip past "<name> the " */
                    txtstr += (strlen(g.plname) + sizeof " the " - sizeof "");
                if (hl->rel == TXT_VALUE && hl->textmatch[0]) {
                    if (fuzzymatch(hl->textmatch, txtstr, "\" -_", TRUE)) {
                        rule = hl;
                        exactmatch = TRUE;
                    } else if (exactmatch) {
                        ; /* already found best fit, skip "noneoftheabove" */
                    } else if (fldidx == BL_TITLE
                               && Upolyd && noneoftheabove(hl->textmatch)) {
                        rule = hl;
                    }
                }
                break;
            case BL_TH_ALWAYS_HILITE:
                rule = hl;
                break;
            case BL_TH_NONE:
                break;
            default:
                break;
            }
        }
    }
    *colorptr = rule ? rule->coloridx : NO_COLOR;
    return rule;
}

static void
split_clridx(int idx, int *coloridx, int *attrib)
{
    if (coloridx)
        *coloridx = idx & 0x00FF;
    if (attrib)
        *attrib = (idx >> 8) & 0x00FF;
}

/*
 * This is the parser for the hilite options.
 *
 * parse_status_hl1() separates each hilite entry into
 * a set of field threshold/action component strings,
 * then calls parse_status_hl2() to parse further
 * and configure the hilite.
 */
boolean
parse_status_hl1(char *op, boolean from_configfile)
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
                if (fldnum == 1 && strcmpi(hsbuf[0], "title") == 0) {
                    /* spaces are allowed in title */
                    hsbuf[fldnum][ccount++] = c;
                    hsbuf[fldnum][ccount] = '\0';
                    op++;
                    continue;
                }
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

/* is str in the format of "[<>]?=?[-+]?[0-9]+%?" regex */
static boolean
is_ltgt_percentnumber(const char *str)
{
    const char *s = str;

    if (*s == '<' || *s == '>')
        s++;
    if (*s == '=')
        s++;
    if (*s == '-' || *s == '+')
        s++;
    if (!digit(*s))
        return FALSE;
    while (digit(*s))
        s++;
    if (*s == '%')
        s++;
    return (*s == '\0');
}

/* does str only contain "<>=-+0-9%" chars */
static boolean
has_ltgt_percentnumber(const char *str)
{
    const char *s = str;

    while (*s) {
        if (!index("<>=-+0123456789%", *s))
            return FALSE;
        s++;
    }
    return TRUE;
}

/* splitsubfields(): splits str in place into '+' or '&' separated strings.
 * returns number of strings, or -1 if more than maxsf or MAX_SUBFIELDS
 */
#define MAX_SUBFIELDS 16
static int
splitsubfields(char *str, char ***sfarr, int maxsf)
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

static boolean
is_fld_arrayvalues(
    const char *str,
    const char *const *arr,
    int arrmin, int arrmax,
    int *retidx)
{
    int i;

    for (i = arrmin; i < arrmax; i++)
        if (!strcmpi(str, arr[i])) {
            *retidx = i;
            return TRUE;
        }
    return FALSE;
}

static int
query_arrayvalue(
    const char *querystr,
    const char *const *arr,
    int arrmin, int arrmax)
{
    int i, res, ret = arrmin - 1;
    winid tmpwin;
    anything any;
    menu_item *picks = (menu_item *) 0;
    int adj = (arrmin > 0) ? 1 : arrmax;
    int clr = 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);

    for (i = arrmin; i < arrmax; i++) {
        any = cg.zeroany;
        any.a_int = i + adj;
        add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                 clr, arr[i], MENU_ITEMFLAGS_NONE);
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

static void
status_hilite_add_threshold(int fld, struct hilite_s *hilite)
{
    struct hilite_s *new_hilite, *old_hilite;

    if (!hilite)
        return;

    /* alloc and initialize a new hilite_s struct */
    new_hilite = (struct hilite_s *) alloc(sizeof (struct hilite_s));
    *new_hilite = *hilite;   /* copy struct */

    new_hilite->set = TRUE;
    new_hilite->fld = fld;
    new_hilite->next = (struct hilite_s *) 0;
    /* insert new entry at the end of the list */
    if (!g.blstats[0][fld].thresholds) {
        g.blstats[0][fld].thresholds = new_hilite;
    } else {
        for (old_hilite = g.blstats[0][fld].thresholds; old_hilite->next;
             old_hilite = old_hilite->next)
            continue;
        old_hilite->next = new_hilite;
    }
    /* sort_hilites(fld) */

    /* current and prev must both point at the same hilites */
    g.blstats[1][fld].thresholds = g.blstats[0][fld].thresholds;
}


static boolean
parse_status_hl2(char (*s)[QBUFSZ], boolean from_configfile)
{
    char *tmp, *how;
    int sidx = 0, i = -1, dt = -1;
    int coloridx = -1, successes = 0;
    int disp_attrib = 0;
    boolean percent, changed, numeric, down, up,
            gt, lt, ge, le, eq, txtval, always;
    const char *txt;
    enum statusfields fld = BL_FLUSH;
    struct hilite_s hilite;
    char tmpbuf[BUFSZ];
    static const char *const aligntxt[] = { "chaotic", "neutral", "lawful" };
    /* hu_stat[] from eat.c has trailing spaces which foul up comparisons */
    static const char *const hutxt[] = { "Satiated", "", "Hungry", "Weak",
                                   "Fainting", "Fainted", "Starved" };

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
    while (s[sidx][0]) {
        char buf[BUFSZ], **subfields;
        int sf = 0;     /* subfield count */
        int kidx;

        txt = (const char *)0;
        percent = numeric = always = FALSE;
        down = up = changed = FALSE;
        gt = ge = eq = le = lt = txtval = FALSE;
#if 0
        /* threshold value - return on empty string */
        if (!s[sidx][0])
            return TRUE;
#endif
        memset((genericptr_t) &hilite, 0, sizeof (struct hilite_s));
        hilite.set = FALSE; /* mark it "unset" */
        hilite.fld = fld;

        if (*s[sidx + 1] == '\0' || !strcmpi(s[sidx], "always")) {
            /* "field/always/color" OR "field/color" */
            always = TRUE;
            if (*s[sidx + 1] == '\0')
                sidx--;
        } else if (!strcmpi(s[sidx], "up") || !strcmpi(s[sidx], "down")) {
            if (initblstats[fld].anytype == ANY_STR)
                /* ordered string comparison is supported but LT/GT for
                   the string fields (title, dungeon-level, alignment)
                   is pointless; treat 'up' or 'down' for string fields
                   as 'changed' rather than rejecting them outright */
                ;
            else if (!strcmpi(s[sidx], "down"))
                down = TRUE;
            else
                up = TRUE;
            changed = TRUE;
        } else if (fld == BL_CAP
                   && is_fld_arrayvalues(s[sidx], enc_stat,
                                         SLT_ENCUMBER, OVERLOADED + 1,
                                         &kidx)) {
            txt = enc_stat[kidx];
            txtval = TRUE;
        } else if (fld == BL_ALIGN
                   && is_fld_arrayvalues(s[sidx], aligntxt, 0, 3, &kidx)) {
            txt = aligntxt[kidx];
            txtval = TRUE;
        } else if (fld == BL_HUNGER
                   && is_fld_arrayvalues(s[sidx], hutxt,
                                         SATIATED, STARVED + 1, &kidx)) {
            txt = hu_stat[kidx];   /* store hu_stat[] val, not hutxt[] */
            txtval = TRUE;
        } else if (!strcmpi(s[sidx], "changed")) {
            changed = TRUE;
        } else if (is_ltgt_percentnumber(s[sidx])) {
            const char *op;

            tmp = s[sidx]; /* is_ltgt_() guarantees [<>]?=?[-+]?[0-9]+%? */
            if (strchr(tmp, '%'))
               percent = TRUE;
            if (*tmp == '<') {
                if (tmp[1] == '=')
                    le = TRUE;
                else
                    lt = TRUE;
            } else if (*tmp == '>') {
                if (tmp[1] == '=')
                    ge = TRUE;
                else
                    gt = TRUE;
            }
            /* '%', '<', '>' have served their purpose, '=' is either
               part of '<' or '>' or optional for '=N', unary '+' is
               just decorative, so get rid of them, leaving -?[0-9]+ */
            tmp = stripchars(tmpbuf, "%<>=+", tmp);
            numeric = TRUE;
            dt = percent ? ANY_INT : initblstats[fld].anytype;
            (void) s_to_anything(&hilite.value, tmp, dt);

            op = gt ? ">" : ge ? ">=" : lt ? "<" : le ? "<=" : "=";
            if (dt == ANY_INT
                /* AC is the only field where negative values make sense but
                   accept >-1 for other fields; reject <0 for non-AC */
                && (hilite.value.a_int
                    < ((fld == BL_AC) ? -128 : gt ? -1 : lt ? 1 : 0)
                /* percentages have another more comprehensive check below */
                    || hilite.value.a_int > (percent ? (lt ? 101 : 100)
                                                     : LARGEST_INT))) {
                config_error_add("%s'%s%d%s'%s", threshold_value,
                                 op, hilite.value.a_int, percent ? "%" : "",
                                 is_out_of_range);
                return FALSE;
            } else if (dt == ANY_LONG
                       && (hilite.value.a_long < (gt ? -1L : lt ? 1L : 0L))) {
                config_error_add("%s'%s%ld'%s", threshold_value,
                                 op, hilite.value.a_long, is_out_of_range);
                return FALSE;
            }
        } else if (initblstats[fld].anytype == ANY_STR) {
            txt = s[sidx];
            txtval = TRUE;
        } else {
            config_error_add(has_ltgt_percentnumber(s[sidx])
                 ? "Wrong format '%s', expected a threshold number or percent"
                 : "Unknown behavior '%s'",
                             s[sidx]);
            return FALSE;
        }

        /* relationships {LT_VALUE, LE_VALUE, EQ_VALUE, GE_VALUE, GT_VALUE} */
        if (gt || up)
            hilite.rel = GT_VALUE;
        else if (lt || down)
            hilite.rel = LT_VALUE;
        else if (ge)
            hilite.rel = GE_VALUE;
        else if (le)
            hilite.rel = LE_VALUE;
        else if (eq  || percent || numeric || changed)
            hilite.rel = EQ_VALUE;
        else if (txtval)
            hilite.rel = TXT_VALUE;
        else
            hilite.rel = LT_VALUE;

        if (initblstats[fld].anytype == ANY_STR && (percent || numeric)) {
            config_error_add("Field '%s' does not support numeric values",
                             initblstats[fld].fldname);
            return FALSE;
        }

        if (percent) {
            if (initblstats[fld].idxmax < 0) {
                config_error_add("Cannot use percent with '%s'",
                                 initblstats[fld].fldname);
                return FALSE;
            } else if ((hilite.value.a_int < -1)
                       || (hilite.value.a_int == -1
                           && hilite.value.a_int != GT_VALUE)
                       || (hilite.value.a_int == 0
                           && hilite.rel == LT_VALUE)
                       || (hilite.value.a_int == 100
                           && hilite.rel == GT_VALUE)
                       || (hilite.value.a_int == 101
                           && hilite.value.a_int != LT_VALUE)
                       || (hilite.value.a_int > 101)) {
                config_error_add(
                           "hilite_status: invalid percentage value '%s%d%%'",
                                 (hilite.rel == LT_VALUE) ? "<"
                                   : (hilite.rel == LE_VALUE) ? "<="
                                     : (hilite.rel == GT_VALUE) ? ">"
                                       : (hilite.rel == GE_VALUE) ? ">="
                                         : "=",
                                 hilite.value.a_int);
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

                if (c >= CLR_MAX || coloridx != -1) {
                    config_error_add("bad color '%d %d'", c, coloridx);
                    return FALSE;
                }
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

        if (hilite.behavior == BL_TH_TEXTMATCH && txt) {
            (void) strncpy(hilite.textmatch, txt, sizeof hilite.textmatch);
            hilite.textmatch[sizeof hilite.textmatch - 1] = '\0';
            (void) trimspaces(hilite.textmatch);
        }

        status_hilite_add_threshold(fld, &hilite);

        successes++;
        sidx++;
    }

    return (successes > 0);
}
#endif /* STATUS_HILITES */

#ifdef STATUS_HILITES

static unsigned long
query_conditions(void)
{
    int i,res;
    unsigned long ret = 0UL;
    winid tmpwin;
    anything any;
    menu_item *picks = (menu_item *) 0;
    int clr = 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);

    for (i = 0; i < SIZE(conditions); i++) {
        any = cg.zeroany;
        any.a_ulong = conditions[i].mask;
        add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                 clr, conditions[i].text[0], MENU_ITEMFLAGS_NONE);
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

static char *
conditionbitmask2str(unsigned long ul)
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

    for (i = 0; i < SIZE(conditions); i++)
        if ((conditions[i].mask & ul) != 0UL) {
            Sprintf(eos(buf), "%s%s", (first) ? "" : "+",
                    conditions[i].text[0]);
            first = FALSE;
        }

    if (!first && alias)
        Sprintf(buf, "%s", alias);

    return buf;
}

static unsigned long
match_str2conditionbitmask(const char *str)
{
    int i, nmatches = 0;
    unsigned long mask = 0UL;

    if (str && *str) {
        /* check matches to canonical names */
        for (i = 0; i < SIZE(conditions); i++)
            if (fuzzymatch(conditions[i].text[0], str, " -_", TRUE)) {
                mask |= conditions[i].mask;
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

static unsigned long
str2conditionbitmask(char *str)
{
    unsigned long conditions_bitmask = 0UL;
    char **subfields;
    int i, sf;

    sf = splitsubfields(str, &subfields, SIZE(conditions));

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

static boolean
parse_condition(char (*s)[QBUFSZ], int sidx)
{
    int i;
    int coloridx = NO_COLOR;
    char *tmp, *how;
    unsigned long conditions_bitmask = 0UL;
    boolean result = FALSE;

    if (!s)
        return FALSE;

    /*3.6.1:
      OPTION=hilite_status: condition/stone+slime+foodPois/red&inverse */

    /*
     * TODO?
     *  It would be simpler to treat each condition (also hunger state
     *  and encumbrance level) as if it were a separate field.  That
     *  way they could have either or both 'changed' temporary rule and
     *  'always' persistent rule and wouldn't need convoluted access to
     *  the intended color and attributes.
     */

    sidx++;
    if (!s[sidx][0]) {
        config_error_add("Missing condition(s)");
        return FALSE;
    }
    while (s[sidx][0]) {
        int sf = 0;     /* subfield count */
        char buf[BUFSZ], **subfields;

        tmp = s[sidx];
        Strcpy(buf, tmp);
        conditions_bitmask = str2conditionbitmask(buf);

        if (!conditions_bitmask)
            return FALSE;

        /*
         * We have the conditions_bitmask with bits set for
         * each ailment we want in a particular color and/or
         * attribute, but we need to assign it to an array of
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
                g.cond_hilites[HL_ATTCLR_DIM] |= conditions_bitmask;
            else if (a == ATR_BLINK)
                g.cond_hilites[HL_ATTCLR_BLINK] |= conditions_bitmask;
            else if (a == ATR_ULINE)
                g.cond_hilites[HL_ATTCLR_ULINE] |= conditions_bitmask;
            else if (a == ATR_INVERSE)
                g.cond_hilites[HL_ATTCLR_INVERSE] |= conditions_bitmask;
            else if (a == ATR_BOLD)
                g.cond_hilites[HL_ATTCLR_BOLD] |= conditions_bitmask;
            else if (a == ATR_NONE) {
                g.cond_hilites[HL_ATTCLR_DIM] &= ~conditions_bitmask;
                g.cond_hilites[HL_ATTCLR_BLINK] &= ~conditions_bitmask;
                g.cond_hilites[HL_ATTCLR_ULINE] &= ~conditions_bitmask;
                g.cond_hilites[HL_ATTCLR_INVERSE] &= ~conditions_bitmask;
                g.cond_hilites[HL_ATTCLR_BOLD] &= ~conditions_bitmask;
            } else {
                int k = match_str2clr(subfields[i]);

                if (k >= CLR_MAX) {
                    config_error_add("bad color %d", k);
                    return FALSE;
                }
                coloridx = k;
            }
        }
        /* set the bits in the appropriate member of the
           condition array according to color chosen as index */

        g.cond_hilites[coloridx] |= conditions_bitmask;
        result = TRUE;
        sidx++;
    }
    return result;
}

void
clear_status_hilites(void)
{
    int i;

    for (i = 0; i < MAXBLSTATS; ++i) {
        struct hilite_s *temp, *next;

        for (temp = g.blstats[0][i].thresholds; temp; temp = next) {
            next = temp->next;
            free(temp);
        }
        g.blstats[0][i].thresholds = g.blstats[1][i].thresholds = 0;
        /* pointer into thresholds list, now stale */
        g.blstats[0][i].hilite_rule = g.blstats[1][i].hilite_rule = 0;
    }
}

static char *
hlattr2attrname(int attrib, char *buf, size_t bufsz)
{
    if (attrib && buf) {
        char attbuf[BUFSZ];
        int first = 0;
        size_t k;

        attbuf[0] = '\0';
        if (attrib == HL_NONE) {
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
        if (k < (size_t)(bufsz - 1))
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

static struct _status_hilite_line_str *status_hilite_str = 0;
static int status_hilite_str_id = 0;

static void
status_hilite_linestr_add(int fld, struct hilite_s *hl,
                          unsigned long mask, const char *str)
{
    struct _status_hilite_line_str *tmp, *nxt;

    tmp = (struct _status_hilite_line_str *) alloc(sizeof *tmp);
    (void) memset(tmp, 0, sizeof *tmp);
    tmp->next = (struct _status_hilite_line_str *) 0;

    tmp->id = ++status_hilite_str_id;
    tmp->fld = fld;
    tmp->hl = hl;
    tmp->mask = mask;
    if (fld == BL_TITLE)
        Strcpy(tmp->str, str);
    else
        (void) stripchars(tmp->str, " ", str);

    if ((nxt = status_hilite_str) != 0) {
        while (nxt->next)
            nxt = nxt->next;
        nxt->next = tmp;
    } else {
        status_hilite_str = tmp;
    }
}

static void
status_hilite_linestr_done(void)
{
    struct _status_hilite_line_str *nxt, *tmp = status_hilite_str;

    while (tmp) {
        nxt = tmp->next;
        free(tmp);
        tmp = nxt;
    }
    status_hilite_str = (struct _status_hilite_line_str *) 0;
    status_hilite_str_id = 0;
}

static int
status_hilite_linestr_countfield(int fld)
{
    struct _status_hilite_line_str *tmp;
    boolean countall = (fld == BL_FLUSH);
    int count = 0;

    for (tmp = status_hilite_str; tmp; tmp = tmp->next) {
        if (countall || tmp->fld == fld)
            count++;
    }
    return count;
}

/* used by options handling, doset(options.c) */
int
count_status_hilites(void)
{
    int count;

    status_hilite_linestr_gather();
    count = status_hilite_linestr_countfield(BL_FLUSH);
    status_hilite_linestr_done();
    return count;
}

static void
status_hilite_linestr_gather_conditions(void)
{
    int i;
    struct _cond_map {
        unsigned long bm;
        unsigned int clratr;
    } cond_maps[SIZE(conditions)];

    (void) memset(cond_maps, 0,
                  SIZE(conditions) * sizeof (struct _cond_map));

    for (i = 0; i < SIZE(conditions); i++) {
        int clr = NO_COLOR;
        int atr = HL_NONE;
        int j;

        for (j = 0; j < CLR_MAX; j++)
            if (g.cond_hilites[j] & conditions[i].mask) {
                clr = j;
                break;
            }
        if (g.cond_hilites[HL_ATTCLR_DIM] & conditions[i].mask)
            atr |= HL_DIM;
        if (g.cond_hilites[HL_ATTCLR_BOLD] & conditions[i].mask)
            atr |= HL_BOLD;
        if (g.cond_hilites[HL_ATTCLR_BLINK] & conditions[i].mask)
            atr |= HL_BLINK;
        if (g.cond_hilites[HL_ATTCLR_ULINE] & conditions[i].mask)
            atr |= HL_ULINE;
        if (g.cond_hilites[HL_ATTCLR_INVERSE] & conditions[i].mask)
            atr |= HL_INVERSE;
        if (atr != HL_NONE)
            atr &= ~HL_NONE;

        if (clr != NO_COLOR || atr != HL_NONE) {
            unsigned int ca = clr | (atr << 8);
            boolean added_condmap = FALSE;

            for (j = 0; j < SIZE(conditions); j++)
                if (cond_maps[j].clratr == ca) {
                    cond_maps[j].bm |= conditions[i].mask;
                    added_condmap = TRUE;
                    break;
                }
            if (!added_condmap) {
                for (j = 0; j < SIZE(conditions); j++)
                    if (!cond_maps[j].bm) {
                        cond_maps[j].bm = conditions[i].mask;
                        cond_maps[j].clratr = ca;
                        break;
                    }
            }
        }
    }

    for (i = 0; i < SIZE(conditions); i++)
        if (cond_maps[i].bm) {
            int clr = NO_COLOR, atr = HL_NONE;

            split_clridx(cond_maps[i].clratr, &clr, &atr);
            if (clr != NO_COLOR || atr != HL_NONE) {
                char clrbuf[BUFSZ];
                char attrbuf[BUFSZ];
                char condbuf[BUFSZ];
                char *tmpattr;

                (void) strNsubst(strcpy(clrbuf, clr2colorname(clr)),
                                 " ", "-", 0);
                tmpattr = hlattr2attrname(atr, attrbuf, BUFSZ);
                if (tmpattr)
                    Sprintf(eos(clrbuf), "&%s", tmpattr);
                Snprintf(condbuf, sizeof(condbuf), "condition/%s/%s",
                         conditionbitmask2str(cond_maps[i].bm), clrbuf);
                status_hilite_linestr_add(BL_CONDITION, 0,
                                          cond_maps[i].bm, condbuf);
            }
        }
}

static void
status_hilite_linestr_gather(void)
{
    int i;
    struct hilite_s *hl;

    status_hilite_linestr_done();

    for (i = 0; i < MAXBLSTATS; i++) {
        hl = g.blstats[0][i].thresholds;
        while (hl) {
            status_hilite_linestr_add(i, hl, 0UL, status_hilite2str(hl));
            hl = hl->next;
        }
    }

    status_hilite_linestr_gather_conditions();
}


static char *
status_hilite2str(struct hilite_s *hl)
{
    static char buf[BUFSZ];
    int clr = 0, attr = 0;
    char behavebuf[BUFSZ];
    char clrbuf[BUFSZ];
    char attrbuf[BUFSZ];
    char *tmpattr;
    const char *op;

    if (!hl)
        return (char *) 0;

    behavebuf[0] = '\0';
    clrbuf[0] = '\0';
    op = (hl->rel == LT_VALUE) ? "<"
           : (hl->rel == LE_VALUE) ? "<="
             : (hl->rel == GT_VALUE) ? ">"
               : (hl->rel == GE_VALUE) ? ">="
                 : (hl->rel == EQ_VALUE) ? "="
                   : 0;

    switch (hl->behavior) {
    case BL_TH_VAL_PERCENTAGE:
        if (op)
            Sprintf(behavebuf, "%s%d%%", op, hl->value.a_int);
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
        if (op)
            Sprintf(behavebuf, "%s%d", op, hl->value.a_int);
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
    (void) strNsubst(strcpy(clrbuf, clr2colorname(clr)), " ", "-", 0);
    if (attr != HL_UNDEF) {
        if ((tmpattr = hlattr2attrname(attr, attrbuf, BUFSZ)) != 0)
            Sprintf(eos(clrbuf), "&%s", tmpattr);
    }
    Snprintf(buf, sizeof(buf), "%s/%s/%s", initblstats[hl->fld].fldname,
             behavebuf, clrbuf);

    return buf;
}

static int
status_hilite_menu_choose_field(void)
{
    winid tmpwin;
    int i, res, fld = BL_FLUSH;
    anything any;
    menu_item *picks = (menu_item *) 0;
    int clr = 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);

    for (i = 0; i < MAXBLSTATS; i++) {
#ifndef SCORE_ON_BOTL
        if (initblstats[i].fld == BL_SCORE
            && !g.blstats[0][BL_SCORE].thresholds)
            continue;
#endif
        any = cg.zeroany;
        any.a_int = (i + 1);
        add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                 clr, initblstats[i].fldname, MENU_ITEMFLAGS_NONE);
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

static int
status_hilite_menu_choose_behavior(int fld)
{
    winid tmpwin;
    int res = 0, beh = BL_TH_NONE-1;
    anything any;
    menu_item *picks = (menu_item *) 0;
    char buf[BUFSZ];
    int at;
    int onlybeh = BL_TH_NONE, nopts = 0;
    int clr = 0;

    if (fld < 0 || fld >= MAXBLSTATS)
        return BL_TH_NONE;

    at = initblstats[fld].anytype;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);

    if (fld != BL_CONDITION) {
        any = cg.zeroany;
        any.a_int = onlybeh = BL_TH_ALWAYS_HILITE;
        Sprintf(buf, "Always highlight %s", initblstats[fld].fldname);
        add_menu(tmpwin, &nul_glyphinfo, &any, 'a', 0, ATR_NONE,
                 clr, buf, MENU_ITEMFLAGS_NONE);
        nopts++;
    }

    if (fld == BL_CONDITION) {
        any = cg.zeroany;
        any.a_int = onlybeh = BL_TH_CONDITION;
        add_menu(tmpwin, &nul_glyphinfo, &any, 'b', 0, ATR_NONE,
                 clr, "Bitmask of conditions", MENU_ITEMFLAGS_NONE);
        nopts++;
    }

    if (fld != BL_CONDITION) {
        any = cg.zeroany;
        any.a_int = onlybeh = BL_TH_UPDOWN;
        Sprintf(buf, "%s value changes", initblstats[fld].fldname);
        add_menu(tmpwin, &nul_glyphinfo, &any, 'c', 0, ATR_NONE,
                 clr, buf, MENU_ITEMFLAGS_NONE);
        nopts++;
    }

    if (fld != BL_CAP && fld != BL_HUNGER
        && (at == ANY_INT || at == ANY_LONG)) {
        any = cg.zeroany;
        any.a_int = onlybeh = BL_TH_VAL_ABSOLUTE;
        add_menu(tmpwin, &nul_glyphinfo, &any, 'n', 0, ATR_NONE,
                 clr, "Number threshold", MENU_ITEMFLAGS_NONE);
        nopts++;
    }

    if (initblstats[fld].idxmax >= 0) {
        any = cg.zeroany;
        any.a_int = onlybeh = BL_TH_VAL_PERCENTAGE;
        add_menu(tmpwin, &nul_glyphinfo, &any, 'p', 0, ATR_NONE,
                 clr, "Percentage threshold", MENU_ITEMFLAGS_NONE);
        nopts++;
    }

    if (initblstats[fld].anytype == ANY_STR
        || fld == BL_CAP || fld == BL_HUNGER) {
        any = cg.zeroany;
        any.a_int = onlybeh = BL_TH_TEXTMATCH;
        Sprintf(buf, "%s text match", initblstats[fld].fldname);
        add_menu(tmpwin, &nul_glyphinfo, &any, 't', 0, ATR_NONE,
                 clr, buf, MENU_ITEMFLAGS_NONE);
        nopts++;
    }

    Sprintf(buf, "Select %s field hilite behavior:",
            initblstats[fld].fldname);
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

static int
status_hilite_menu_choose_updownboth(int fld, const char *str,
                                     boolean ltok, boolean gtok)
{
    int res, ret = NO_LTEQGT;
    winid tmpwin;
    char buf[BUFSZ];
    anything any;
    menu_item *picks = (menu_item *) 0;
    int clr = 0;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);

    if (ltok) {
        if (str)
            Sprintf(buf, "%s than %s",
                    (fld == BL_AC) ? "Better (lower)" : "Less", str);
        else
            Sprintf(buf, "Value goes down");
        any = cg.zeroany;
        any.a_int = 10 + LT_VALUE;
        add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                 clr, buf, MENU_ITEMFLAGS_NONE);

        if (str) {
            Sprintf(buf, "%s or %s",
                    str, (fld == BL_AC) ? "better (lower)" : "less");
            any = cg.zeroany;
            any.a_int = 10 + LE_VALUE;
            add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                     clr, buf, MENU_ITEMFLAGS_NONE);
        }
    }

    if (str)
        Sprintf(buf, "Exactly %s", str);
    else
        Sprintf(buf, "Value changes");
    any = cg.zeroany;
    any.a_int = 10 + EQ_VALUE;
    add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
             clr, buf, MENU_ITEMFLAGS_NONE);

    if (gtok) {
        if (str) {
            Sprintf(buf, "%s or %s",
                    str, (fld == BL_AC) ? "worse (higher)" : "more");
            any = cg.zeroany;
            any.a_int = 10 + GE_VALUE;
            add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                     clr, buf, MENU_ITEMFLAGS_NONE);
        }

        if (str)
            Sprintf(buf, "%s than %s",
                    (fld == BL_AC) ? "Worse (higher)" : "More", str);
        else
            Sprintf(buf, "Value goes up");
        any = cg.zeroany;
        any.a_int = 10 + GT_VALUE;
        add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
             clr, buf, MENU_ITEMFLAGS_NONE);
    }
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

static boolean
status_hilite_menu_add(int origfld)
{
    int fld;
    int behavior;
    int lt_gt_eq;
    int clr = NO_COLOR, atr = HL_UNDEF;
    struct hilite_s hilite;
    unsigned long cond = 0UL;
    char colorqry[BUFSZ];
    char attrqry[BUFSZ];

 choose_field:
    fld = origfld;
    if (fld == BL_FLUSH) {
        fld = status_hilite_menu_choose_field();
        /* isn't this redundant given what follows? */
        if (fld == BL_FLUSH)
            return FALSE;
    }

    if (fld == BL_FLUSH)
        return FALSE;

    colorqry[0] = '\0';
    attrqry[0] = '\0';

    memset((genericptr_t) &hilite, 0, sizeof (struct hilite_s));
    hilite.next = (struct hilite_s *) 0;
    hilite.set = FALSE; /* mark it "unset" */
    hilite.fld = fld;

 choose_behavior:
    behavior = status_hilite_menu_choose_behavior(fld);

    if (behavior == (BL_TH_NONE - 1)) {
        return FALSE;
    } else if (behavior == BL_TH_NONE) {
        if (origfld == BL_FLUSH)
            goto choose_field;
        return FALSE;
    }

    hilite.behavior = behavior;

 choose_value:
    if (behavior == BL_TH_VAL_PERCENTAGE
        || behavior == BL_TH_VAL_ABSOLUTE) {
        char inbuf[BUFSZ], buf[BUFSZ];
        anything aval;
        int val, dt;
        boolean gotnum = FALSE, percent = (behavior == BL_TH_VAL_PERCENTAGE);
        char *inp, *numstart;
        const char *op;

        lt_gt_eq = NO_LTEQGT; /* not set up yet */
        inbuf[0] = '\0';
        Sprintf(buf, "Enter %svalue for %s threshold:",
                percent ? "percentage " : "",
                initblstats[fld].fldname);
        getlin(buf, inbuf);
        if (inbuf[0] == '\0' || inbuf[0] == '\033')
            goto choose_behavior;

        inp = numstart = trimspaces(inbuf);
        if (!*inp)
            goto choose_behavior;

        /* allow user to enter "<50%" or ">50" or just "50"
           or <=50% or >=50 or =50 */
        if (*inp == '>' || *inp == '<' || *inp == '=') {
            lt_gt_eq = (*inp == '>') ? ((inp[1] == '=') ? GE_VALUE : GT_VALUE)
                     : (*inp == '<') ? ((inp[1] == '=') ? LE_VALUE : LT_VALUE)
                       : EQ_VALUE;
            *inp++ = ' ';
            numstart++;
            if (lt_gt_eq == GE_VALUE || lt_gt_eq == LE_VALUE) {
                *inp++ = ' ';
                numstart++;
            }
        }
        if (*inp == '-') {
            inp++;
        } else if (*inp == '+') {
            *inp++ = ' ';
            numstart++;
        }
        while (digit(*inp)) {
            inp++;
            gotnum = TRUE;
        }
        if (*inp == '%') {
            if (!percent) {
                pline("Not expecting a percentage.");
                goto choose_behavior;
            }
            *inp = '\0'; /* strip '%' [this accepts trailing junk!] */
        } else if (*inp) {
            /* some random characters */
            pline("\"%s\" is not a recognized number.", inp);
            goto choose_value;
        }
        if (!gotnum) {
            pline("Is that an invisible number?");
            goto choose_value;
        }
        op = (lt_gt_eq == LT_VALUE) ? "<"
               : (lt_gt_eq == LE_VALUE) ? "<="
                 : (lt_gt_eq == GT_VALUE) ? ">"
                   : (lt_gt_eq == GE_VALUE) ? ">="
                     : (lt_gt_eq == EQ_VALUE) ? "="
                       : ""; /* didn't specify lt_gt_eq with number */

        aval = cg.zeroany;
        dt = percent ? ANY_INT : initblstats[fld].anytype;
        (void) s_to_anything(&aval, numstart, dt);

        if (percent) {
            val = aval.a_int;
            if (initblstats[fld].idxmax == -1) {
                pline("Field '%s' does not support percentage values.",
                      initblstats[fld].fldname);
                behavior = BL_TH_VAL_ABSOLUTE;
                goto choose_value;
            }
            /* if player only specified a number then lt_gt_eq isn't set
               up yet and the >-1 and <101 exceptions can't be honored;
               deliberate use of those should be uncommon enough for
               that to be palatable; for 0 and 100, choose_updown_both()
               will prevent useless operations */
            if ((val < 0 && (val != -1 || lt_gt_eq != GT_VALUE))
                || (val == 0 && lt_gt_eq == LT_VALUE)
                || (val == 100 && lt_gt_eq == GT_VALUE)
                || (val > 100 && (val != 101 || lt_gt_eq != LT_VALUE))) {
                pline("'%s%d%%' is not a valid percent value.", op, val);
                goto choose_value;
            }
            /* restore suffix for use in color and attribute prompts */
            if (!index(numstart, '%'))
                Strcat(numstart, "%");

        /* reject negative values except for AC and >-1; reject 0 for < */
        } else if (dt == ANY_INT
                   && (aval.a_int < ((fld == BL_AC) ? -128
                                     : (lt_gt_eq == GT_VALUE) ? -1
                                       : (lt_gt_eq == LT_VALUE) ? 1 : 0))) {
            pline("%s'%s%d'%s", threshold_value,
                  op, aval.a_int, is_out_of_range);
            goto choose_value;
        } else if (dt == ANY_LONG
                   && (aval.a_long < ((lt_gt_eq == GT_VALUE) ? -1L
                                      : (lt_gt_eq == LT_VALUE) ? 1L : 0L))) {
            pline("%s'%s%ld'%s", threshold_value,
                  op, aval.a_long, is_out_of_range);
            goto choose_value;
        }

        if (lt_gt_eq == NO_LTEQGT) {
            boolean ltok = ((dt == ANY_INT)
                            ? (aval.a_int > 0 || fld == BL_AC)
                            : (aval.a_long > 0L)),
                    gtok = (!percent || aval.a_long < 100);

            lt_gt_eq = status_hilite_menu_choose_updownboth(fld, inbuf,
                                                            ltok, gtok);
            if (lt_gt_eq == NO_LTEQGT)
                goto choose_value;
        }

        Sprintf(colorqry, "Choose a color for when %s is %s%s%s:",
                initblstats[fld].fldname,
                (lt_gt_eq == LT_VALUE) ? "less than "
                  : (lt_gt_eq == GT_VALUE) ? "more than "
                    : "",
                numstart,
                (lt_gt_eq == LE_VALUE) ? " or less"
                  : (lt_gt_eq == GE_VALUE) ? " or more"
                    : "");
        Sprintf(attrqry, "Choose attribute for when %s is %s%s%s:",
                initblstats[fld].fldname,
                (lt_gt_eq == LT_VALUE) ? "less than "
                  : (lt_gt_eq == GT_VALUE) ? "more than "
                    : "",
                numstart,
                (lt_gt_eq == LE_VALUE) ? " or less"
                  : (lt_gt_eq == GE_VALUE) ? " or more"
                    : "");

        hilite.rel = lt_gt_eq;
        hilite.value = aval;
    } else if (behavior == BL_TH_UPDOWN) {
        if (initblstats[fld].anytype != ANY_STR) {
            boolean ltok = (fld != BL_TIME), gtok = TRUE;

            lt_gt_eq = status_hilite_menu_choose_updownboth(fld, (char *)0,
                                                            ltok, gtok);
            if (lt_gt_eq == NO_LTEQGT)
                goto choose_behavior;
        } else { /* ANY_STR */
            /* player picked '<field> value changes' in outer menu;
               ordered string comparison is supported but LT/GT for the
               string status fields (title, dungeon level, alignment)
               is pointless; rather than calling ..._choose_updownboth()
               with ltok==False plus gtok=False and having a menu with a
               single choice, skip it altogether and just use 'changed' */
            lt_gt_eq = EQ_VALUE;
        }
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
            return FALSE;
        }
        Snprintf(colorqry, sizeof(colorqry),
                "Choose a color for conditions %s:",
                conditionbitmask2str(cond));
        Snprintf(attrqry, sizeof(attrqry),
                "Choose attribute for conditions %s:",
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
                                      SLT_ENCUMBER, OVERLOADED + 1);

            if (rv < SLT_ENCUMBER)
                goto choose_behavior;

            hilite.rel = TXT_VALUE;
            Strcpy(hilite.textmatch, enc_stat[rv]);
        } else if (fld == BL_ALIGN) {
            static const char *const aligntxt[] = {
                "chaotic", "neutral", "lawful"
            };
            int rv = query_arrayvalue(qry_buf,
                                      aligntxt, 0, 2 + 1);

            if (rv < 0)
                goto choose_behavior;

            hilite.rel = TXT_VALUE;
            Strcpy(hilite.textmatch, aligntxt[rv]);
        } else if (fld == BL_HUNGER) {
            static const char *const hutxt[] = {
                "Satiated", (char *) 0, "Hungry", "Weak",
                "Fainting", "Fainted", "Starved"
            };
            int rv = query_arrayvalue(qry_buf, hutxt, SATIATED, STARVED + 1);

            if (rv < SATIATED)
                goto choose_behavior;

            hilite.rel = TXT_VALUE;
            Strcpy(hilite.textmatch, hutxt[rv]);
        } else if (fld == BL_TITLE) {
            const char *rolelist[3 * 9 + 1];
            char mbuf[MAXVALWIDTH], fbuf[MAXVALWIDTH], obuf[MAXVALWIDTH];
            int i, j, rv;

            for (i = j = 0; i < 9; i++) {
                Sprintf(mbuf, "\"%s\"", g.urole.rank[i].m);
                if (g.urole.rank[i].f) {
                    Sprintf(fbuf, "\"%s\"", g.urole.rank[i].f);
                    Snprintf(obuf, sizeof obuf, "%s or %s",
                            flags.female ? fbuf : mbuf,
                            flags.female ? mbuf : fbuf);
                } else {
                    fbuf[0] = obuf[0] = '\0';
                }
                if (flags.female) {
                    if (*fbuf)
                        rolelist[j++] = dupstr(fbuf);
                    rolelist[j++] = dupstr(mbuf);
                    if (*obuf)
                        rolelist[j++] = dupstr(obuf);
                } else {
                    rolelist[j++] = dupstr(mbuf);
                    if (*fbuf)
                        rolelist[j++] = dupstr(fbuf);
                    if (*obuf)
                        rolelist[j++] = dupstr(obuf);
                }
            }
            rolelist[j++] = dupstr("\"none of the above (polymorphed)\"");

            rv = query_arrayvalue(qry_buf, rolelist, 0, j);
            if (rv >= 0) {
                hilite.rel = TXT_VALUE;
                Strcpy(hilite.textmatch, rolelist[rv]);
            }
            for (i = 0; i < j; i++)
                free((genericptr_t) rolelist[i]), rolelist[i] = 0;
            if (rv < 0)
                goto choose_behavior;
        } else {
            char inbuf[BUFSZ];

            inbuf[0] = '\0';
            getlin(qry_buf, inbuf);
            if (inbuf[0] == '\0' || inbuf[0] == '\033')
                goto choose_behavior;

            hilite.rel = TXT_VALUE;
            if (strlen(inbuf) < sizeof hilite.textmatch)
                Strcpy(hilite.textmatch, inbuf);
            else
                return FALSE;
        }
        Sprintf(colorqry, "Choose a color for when %s is '%s':",
                initblstats[fld].fldname, hilite.textmatch);
        Sprintf(attrqry, "Choose attribute for when %s is '%s':",
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
    atr = query_attr(attrqry);
    if (atr == -1)
        goto choose_color;

    if (behavior == BL_TH_CONDITION) {
        char clrbuf[BUFSZ];
        char attrbuf[BUFSZ];
        char *tmpattr;

        if (atr & HL_DIM)
            g.cond_hilites[HL_ATTCLR_DIM] |= cond;
        if (atr & HL_BLINK)
            g.cond_hilites[HL_ATTCLR_BLINK] |= cond;
        if (atr & HL_ULINE)
            g.cond_hilites[HL_ATTCLR_ULINE] |= cond;
        if (atr & HL_INVERSE)
            g.cond_hilites[HL_ATTCLR_INVERSE] |= cond;
        if (atr & HL_BOLD)
            g.cond_hilites[HL_ATTCLR_BOLD] |= cond;
        if (atr == HL_NONE) {
            g.cond_hilites[HL_ATTCLR_DIM] &= ~cond;
            g.cond_hilites[HL_ATTCLR_BLINK] &= ~cond;
            g.cond_hilites[HL_ATTCLR_ULINE] &= ~cond;
            g.cond_hilites[HL_ATTCLR_INVERSE] &= ~cond;
            g.cond_hilites[HL_ATTCLR_BOLD] &= ~cond;
        }
        g.cond_hilites[clr] |= cond;
        (void) strNsubst(strcpy(clrbuf, clr2colorname(clr)), " ", "-", 0);
        tmpattr = hlattr2attrname(atr, attrbuf, BUFSZ);
        if (tmpattr)
            Sprintf(eos(clrbuf), "&%s", tmpattr);
        pline("Added hilite condition/%s/%s",
              conditionbitmask2str(cond), clrbuf);
    } else {
        char *p, *q;

        hilite.coloridx = clr | (atr << 8);
        hilite.anytype = initblstats[fld].anytype;

        if (fld == BL_TITLE && (p = strstri(hilite.textmatch, " or ")) != 0) {
            /* split menu choice "male-rank or female-rank" into two distinct
               but otherwise identical rules, "male-rank" and "female-rank" */
            *p = '\0'; /* chop off " or female-rank" */
            /* new rule for male-rank */
            status_hilite_add_threshold(fld, &hilite);
            pline("Added hilite %s", status_hilite2str(&hilite));
            /* transfer female-rank to start of hilite.textmatch buffer */
            p += sizeof " or " - sizeof "";
            q = hilite.textmatch;
            while ((*q++ = *p++) != '\0')
                continue;
            /* proceed with normal addition of new rule */
        }
        status_hilite_add_threshold(fld, &hilite);
        pline("Added hilite %s", status_hilite2str(&hilite));
    }
    reset_status_hilites();
    return TRUE;
}

static boolean
status_hilite_remove(int id)
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
            g.cond_hilites[i] &= ~hlstr->mask;
        g.cond_hilites[HL_ATTCLR_DIM] &= ~hlstr->mask;
        g.cond_hilites[HL_ATTCLR_BOLD] &= ~hlstr->mask;
        g.cond_hilites[HL_ATTCLR_BLINK] &= ~hlstr->mask;
        g.cond_hilites[HL_ATTCLR_ULINE] &= ~hlstr->mask;
        g.cond_hilites[HL_ATTCLR_INVERSE] &= ~hlstr->mask;
        return TRUE;
    } else {
        int fld = hlstr->fld;
        struct hilite_s *hl, *hlprev = (struct hilite_s *) 0;

        for (hl = g.blstats[0][fld].thresholds; hl; hl = hl->next) {
            if (hlstr->hl == hl) {
                if (hlprev) {
                    hlprev->next = hl->next;
                } else {
                    g.blstats[0][fld].thresholds = hl->next;
                    g.blstats[1][fld].thresholds
                        = g.blstats[0][fld].thresholds;
                }
                if (g.blstats[0][fld].hilite_rule == hl) {
                    g.blstats[0][fld].hilite_rule
                        = g.blstats[1][fld].hilite_rule
                        = (struct hilite_s *) 0;
                    g.blstats[0][fld].time = g.blstats[1][fld].time = 0L;
                }
                free((genericptr_t) hl);
                return TRUE;
            }
            hlprev = hl;
        }
    }
    return FALSE;
}

static boolean
status_hilite_menu_fld(int fld)
{
    winid tmpwin;
    int i, res;
    menu_item *picks = (menu_item *) 0;
    anything any;
    int count = status_hilite_linestr_countfield(fld);
    struct _status_hilite_line_str *hlstr;
    char buf[BUFSZ];
    boolean acted;
    int clr = 0;

    if (!count) {
        if (status_hilite_menu_add(fld)) {
            status_hilite_linestr_done();
            status_hilite_linestr_gather();
            count = status_hilite_linestr_countfield(fld);
        } else
            return FALSE;
    }

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);

    if (count) {
        hlstr = status_hilite_str;
        while (hlstr) {
            if (hlstr->fld == fld) {
                any = cg.zeroany;
                any.a_int = hlstr->id;
                add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                         clr, hlstr->str, MENU_ITEMFLAGS_NONE);
            }
            hlstr = hlstr->next;
        }
    } else {
        any = cg.zeroany;
        Sprintf(buf, "No current hilites for %s", initblstats[fld].fldname);
        add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE, clr, buf,
                 MENU_ITEMFLAGS_NONE);
    }

    /* separator line */
    any = cg.zeroany;
    add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE, clr, "",
             MENU_ITEMFLAGS_NONE);

    if (count) {
        any = cg.zeroany;
        any.a_int = -1;
        add_menu(tmpwin, &nul_glyphinfo, &any, 'X', 0, ATR_NONE,
                 clr, "Remove selected hilites",
                 MENU_ITEMFLAGS_NONE);
    }

#ifndef SCORE_ON_BOTL
    if (fld == BL_SCORE) {
        /* suppress 'Z - Add a new hilite' for 'score' when SCORE_ON_BOTL
           is disabled; we wouldn't be called for 'score' unless it has
           hilite rules from the config file, so count must be positive
           (hence there's no risk that we're putting up an empty menu) */
        ;
    } else
#endif
    {
        any = cg.zeroany;
        any.a_int = -2;
        add_menu(tmpwin, &nul_glyphinfo, &any, 'Z', 0, ATR_NONE,
                 clr, "Add new hilites", MENU_ITEMFLAGS_NONE);
    }

    Sprintf(buf, "Current %s hilites:", initblstats[fld].fldname);
    end_menu(tmpwin, buf);

    acted = FALSE;
    if ((res = select_menu(tmpwin, PICK_ANY, &picks)) > 0) {
        int idx;
        unsigned mode = 0;

        for (i = 0; i < res; i++) {
            idx = picks[i].item.a_int;
            if (idx == -1)
                mode |= 1; /* delete selected hilites */
            else if (idx == -2)
                mode |= 2; /* create new hilites */
        }
        if (mode & 1) { /* delete selected hilites */
            for (i = 0; i < res; i++) {
                idx = picks[i].item.a_int;
                if (idx > 0 && status_hilite_remove(idx))
                    acted = TRUE;
            }
        }
        if (mode & 2) { /* create new hilites */
            while (status_hilite_menu_add(fld))
                acted = TRUE;
        }
        free((genericptr_t) picks), picks = 0;
    }
    destroy_nhwindow(tmpwin);
    return acted;
}

static void
status_hilites_viewall(void)
{
    winid datawin;
    struct _status_hilite_line_str *hlstr = status_hilite_str;
    char buf[BUFSZ];

    datawin = create_nhwindow(NHW_TEXT);

    while (hlstr) {
        Sprintf(buf, "OPTIONS=hilite_status: %.*s",
                (int) (BUFSZ - sizeof "OPTIONS=hilite_status: " - 1),
                hlstr->str);
        putstr(datawin, 0, buf);
        hlstr = hlstr->next;
    }

    display_nhwindow(datawin, FALSE);
    destroy_nhwindow(datawin);
}

boolean
status_hilite_menu(void)
{
    winid tmpwin;
    int i, res;
    menu_item *picks = (menu_item *) 0;
    anything any;
    boolean redo;
    int countall;
    int clr = 0;

 shlmenu_redo:
    redo = FALSE;

    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin, MENU_BEHAVE_STANDARD);

    status_hilite_linestr_gather();
    countall = status_hilite_linestr_countfield(BL_FLUSH);
    if (countall) {
        any = cg.zeroany;
        any.a_int = -1;
        add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                 clr, "View all hilites in config format",
                 MENU_ITEMFLAGS_NONE);

        any = cg.zeroany;
        add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                 clr, "", MENU_ITEMFLAGS_NONE);
    }

    for (i = 0; i < MAXBLSTATS; i++) {
        int count = status_hilite_linestr_countfield(i);
        char buf[BUFSZ];

#ifndef SCORE_ON_BOTL
        /* config file might contain rules for highlighting 'score'
           even when SCORE_ON_BOTL is disabled; if so, 'O' command
           menus will show them and allow deletions but not additions,
           otherwise, it won't show 'score' at all */
        if (initblstats[i].fld == BL_SCORE && !count)
            continue;
#endif
        any = cg.zeroany;
        any.a_int = i + 1;
        Sprintf(buf, "%-18s", initblstats[i].fldname);
        if (count)
            Sprintf(eos(buf), " (%d defined)", count);
        add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE,
                 clr, buf, MENU_ITEMFLAGS_NONE);
    }

    end_menu(tmpwin, "Status hilites:");
    if ((res = select_menu(tmpwin, PICK_ONE, &picks)) > 0) {
        i = picks->item.a_int - 1;
        if (i < 0) {
            status_hilites_viewall();
        } else {
            if (status_hilite_menu_fld(i))
                reset_status_hilites();
        }
        free((genericptr_t) picks), picks = (menu_item *) 0;
        redo = TRUE;
    }

    destroy_nhwindow(tmpwin);
    countall = status_hilite_linestr_countfield(BL_FLUSH);
    status_hilite_linestr_done();

    if (redo)
        goto shlmenu_redo;

    /* hilite_delta=='statushilites' does double duty:  it is the
       number of turns for temporary highlights to remain visible
       and also when non-zero it is the flag to enable highlighting */
    if (countall > 0 && !iflags.hilite_delta)
        pline(
 "To have highlights become active, set 'statushilites' option to non-zero.");

    return TRUE;
}

#endif /* STATUS_HILITES */

/*botl.c*/
