/* NetHack 3.7	topten.c	$NHDT-Date: 1606009004 2020/11/22 01:36:44 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.74 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "dlb.h"

#if defined(VMS) && !defined(UPDATE_RECORD_IN_PLACE)
/* We don't want to rewrite the whole file, because that entails
   creating a new version which requires that the old one be deletable.
   [Write and Delete are separate permissions on VMS.  'record' should
   be writable but not deletable there.]  */
#define UPDATE_RECORD_IN_PLACE
#endif

/*
 * Updating in place can leave junk at the end of the file in some
 * circumstances (if it shrinks and the O.S. doesn't have a straightforward
 * way to truncate it).  The trailing junk is harmless and the code
 * which reads the scores will ignore it.
 */
#ifdef UPDATE_RECORD_IN_PLACE
static long final_fpos; /* [note: do not move this to the 'g' struct] */
#endif

#define done_stopprint gp.program_state.stopprint

#define newttentry() (struct toptenentry *) alloc(sizeof (struct toptenentry))
#define dealloc_ttentry(ttent) free((genericptr_t) (ttent))
#ifndef NAMSZ
/* Changing NAMSZ can break your existing record/logfile */
#define NAMSZ 10
#endif
#define DTHSZ 100
#define ROLESZ 3

struct toptenentry {
    struct toptenentry *tt_next;
#ifdef UPDATE_RECORD_IN_PLACE
    long fpos;
#endif
    long points;
    int deathdnum, deathlev;
    int maxlvl, hp, maxhp, deaths;
    int ver_major, ver_minor, patchlevel;
    long deathdate, birthdate;
    int uid;
    char plrole[ROLESZ + 1];
    char plrace[ROLESZ + 1];
    char plgend[ROLESZ + 1];
    char plalign[ROLESZ + 1];
    char name[NAMSZ + 1];
    char death[DTHSZ + 1];
} *tt_head;
/* size big enough to read in all the string fields at once; includes
   room for separating space or trailing newline plus string terminator */
#define SCANBUFSZ (4 * (ROLESZ + 1) + (NAMSZ + 1) + (DTHSZ + 1) + 1)

static struct toptenentry zerott;

static void topten_print(const char *);
static void topten_print_bold(const char *);
static void outheader(void);
static void outentry(int, struct toptenentry *, boolean);
static void discardexcess(FILE *);
static void readentry(FILE *, struct toptenentry *);
static void writeentry(FILE *, struct toptenentry *);
#ifdef XLOGFILE
static void writexlentry(FILE *, struct toptenentry *, int);
static long encodexlogflags(void);
static long encodeconduct(void);
static long encodeachieve(boolean);
static void add_achieveX(char *, const char *, boolean);
static char *encode_extended_achievements(char *);
static char *encode_extended_conducts(char *);
#endif
static void free_ttlist(struct toptenentry *);
static int classmon(char *);
static int score_wanted(boolean, int, struct toptenentry *, int,
                        const char **, int);
#ifdef NO_SCAN_BRACK
static void nsb_mung_line(char *);
static void nsb_unmung_line(char *);
#endif

/* "killed by",&c ["an"] 'gk.killer.name' */
void
formatkiller(
    char *buf,
    unsigned siz,
    int how,
    boolean incl_helpless)
{
    static NEARDATA const char *const killed_by_prefix[] = {
        /* DIED, CHOKING, POISONING, STARVING, */
        "killed by ", "choked on ", "poisoned by ", "died of ",
        /* DROWNING, BURNING, DISSOLVED, CRUSHING, */
        "drowned in ", "burned by ", "dissolved in ", "crushed to death by ",
        /* STONING, TURNED_SLIME, GENOCIDED, */
        "petrified by ", "turned to slime by ", "killed by ",
        /* PANICKED, TRICKED, QUIT, ESCAPED, ASCENDED */
        "", "", "", "", ""
    };
    unsigned l;
    char c, *kname = gk.killer.name;

    buf[0] = '\0'; /* lint suppression */
    switch (gk.killer.format) {
    default:
        impossible("bad killer format? (%d)", gk.killer.format);
        /*FALLTHRU*/
    case NO_KILLER_PREFIX:
        break;
    case KILLED_BY_AN:
        kname = an(kname);
        /*FALLTHRU*/
    case KILLED_BY:
        (void) strncat(buf, killed_by_prefix[how], siz - 1);
        l = Strlen(buf);
        buf += l, siz -= l;
        break;
    }
    /* Copy kname into buf[].
     * Object names and named fruit have already been sanitized, but
     * monsters can have "called 'arbitrary text'" attached to them,
     * so make sure that that text can't confuse field splitting when
     * record, logfile, or xlogfile is re-read at some later point.
     */
    while (--siz > 0) {
        c = *kname++;
        if (!c)
            break;
        else if (c == ',')
            c = ';';
        /* 'xlogfile' doesn't really need protection for '=', but
           fixrecord.awk for corrupted 3.6.0 'record' does (only
           if using xlogfile rather than logfile to repair record) */
        else if (c == '=')
            c = '_';
        /* tab is not possible due to use of mungspaces() when naming;
           it would disrupt xlogfile parsing if it were present */
        else if (c == '\t')
            c = ' ';
        *buf++ = c;
    }
    *buf = '\0';

    if (incl_helpless && gm.multi) {
        /* X <= siz: 'sizeof "string"' includes 1 for '\0' terminator */
        if (gm.multi_reason
            && strlen(gm.multi_reason) + sizeof ", while " <= siz)
            Sprintf(buf, ", while %s", gm.multi_reason);
        /* either gm.multi_reason wasn't specified or wouldn't fit */
        else if (sizeof ", while helpless" <= siz)
            Strcpy(buf, ", while helpless");
        /* else extra death info won't fit, so leave it out */
    }
}

static void
topten_print(const char *x)
{
    if (gt.toptenwin == WIN_ERR)
        raw_print(x);
    else
        putstr(gt.toptenwin, ATR_NONE, x);
}

static void
topten_print_bold(const char *x)
{
    if (gt.toptenwin == WIN_ERR)
        raw_print_bold(x);
    else
        putstr(gt.toptenwin, ATR_BOLD, x);
}

int
observable_depth(d_level *lev)
{
#if 0
    /* if we ever randomize the order of the elemental planes, we
       must use a constant external representation in the record file */
    if (In_endgame(lev)) {
        if (Is_astralevel(lev))
            return -5;
        else if (Is_waterlevel(lev))
            return -4;
        else if (Is_firelevel(lev))
            return -3;
        else if (Is_airlevel(lev))
            return -2;
        else if (Is_earthlevel(lev))
            return -1;
        else
            return 0; /* ? */
    } else
#endif
    return depth(lev);
}

/* throw away characters until current record has been entirely consumed */
static void
discardexcess(FILE *rfile)
{
    int c;

    do {
        c = fgetc(rfile);
    } while (c != '\n' && c != EOF);
}

DISABLE_WARNING_FORMAT_NONLITERAL

static void
readentry(FILE *rfile, struct toptenentry *tt)
{
    char inbuf[SCANBUFSZ], s1[SCANBUFSZ], s2[SCANBUFSZ], s3[SCANBUFSZ],
         s4[SCANBUFSZ], s5[SCANBUFSZ], s6[SCANBUFSZ];

#ifdef NO_SCAN_BRACK /* Version_ Pts DgnLevs_ Hp___ Died__Born id */
    static const char fmt[] = "%d %d %d %ld %d %d %d %d %d %d %ld %ld %d%*c";
    static const char fmt32[] = "%c%c %s %s%*c";
    static const char fmt33[] = "%s %s %s %s %s %s%*c";
#else
    static const char fmt[] = "%d.%d.%d %ld %d %d %d %d %d %d %ld %ld %d ";
    static const char fmt32[] = "%c%c %[^,],%[^\n]%*c";
    static const char fmt33[] = "%s %s %s %s %[^,],%[^\n]%*c";
#endif

#ifdef UPDATE_RECORD_IN_PLACE
    /* note: input below must read the record's terminating newline */
    final_fpos = tt->fpos = ftell(rfile);
#endif
#define TTFIELDS 13
    if (fscanf(rfile, fmt, &tt->ver_major, &tt->ver_minor, &tt->patchlevel,
               &tt->points, &tt->deathdnum, &tt->deathlev, &tt->maxlvl,
               &tt->hp, &tt->maxhp, &tt->deaths, &tt->deathdate,
               &tt->birthdate, &tt->uid) != TTFIELDS) {
#undef TTFIELDS
        tt->points = 0;
        discardexcess(rfile);
    } else {
        /* load remainder of record into a local buffer;
           this imposes an implicit length limit of SCANBUFSZ
           on every string field extracted from the buffer */
        if (!fgets(inbuf, sizeof inbuf, rfile)) {
            /* sscanf will fail and tt->points will be set to 0 */
            *inbuf = '\0';
        } else if (!strchr(inbuf, '\n')) {
            Strcpy(&inbuf[sizeof inbuf - 2], "\n");
            discardexcess(rfile);
        }
        /* Check for backwards compatibility */
        if (tt->ver_major < 3 || (tt->ver_major == 3 && tt->ver_minor < 3)) {
            int i;

            if (sscanf(inbuf, fmt32, tt->plrole, tt->plgend, s1, s2) == 4) {
                tt->plrole[1] = tt->plgend[1] = '\0'; /* read via %c */
                copynchars(tt->name, s1, (int) (sizeof tt->name) - 1);
                copynchars(tt->death, s2, (int) (sizeof tt->death) - 1);
            } else
                tt->points = 0;
            tt->plrole[1] = '\0';
            if ((i = str2role(tt->plrole)) >= 0)
                Strcpy(tt->plrole, roles[i].filecode);
            Strcpy(tt->plrace, "?");
            Strcpy(tt->plgend, (tt->plgend[0] == 'M') ? "Mal" : "Fem");
            Strcpy(tt->plalign, "?");
        } else if (sscanf(inbuf, fmt33, s1, s2, s3, s4, s5, s6) == 6) {
            copynchars(tt->plrole, s1, (int) (sizeof tt->plrole) - 1);
            copynchars(tt->plrace, s2, (int) (sizeof tt->plrace) - 1);
            copynchars(tt->plgend, s3, (int) (sizeof tt->plgend) - 1);
            copynchars(tt->plalign, s4, (int) (sizeof tt->plalign) - 1);
            copynchars(tt->name, s5, (int) (sizeof tt->name) - 1);
            copynchars(tt->death, s6, (int) (sizeof tt->death) - 1);
        } else
            tt->points = 0;
#ifdef NO_SCAN_BRACK
        if (tt->points > 0) {
            nsb_unmung_line(tt->name);
            nsb_unmung_line(tt->death);
        }
#endif
    }

    /* check old score entries for Y2K problem and fix whenever found */
    if (tt->points > 0) {
        if (tt->birthdate < 19000000L)
            tt->birthdate += 19000000L;
        if (tt->deathdate < 19000000L)
            tt->deathdate += 19000000L;
    }
}

static void
writeentry(FILE *rfile, struct toptenentry *tt)
{
    static const char fmt32[] = "%c%c ";        /* role,gender */
    static const char fmt33[] = "%s %s %s %s "; /* role,race,gndr,algn */
#ifndef NO_SCAN_BRACK
    static const char fmt0[] = "%d.%d.%d %ld %d %d %d %d %d %d %ld %ld %d ";
    static const char fmtX[] = "%s,%s\n";
#else /* NO_SCAN_BRACK */
    static const char fmt0[] = "%d %d %d %ld %d %d %d %d %d %d %ld %ld %d ";
    static const char fmtX[] = "%s %s\n";

    nsb_mung_line(tt->name);
    nsb_mung_line(tt->death);
#endif

    (void) fprintf(rfile, fmt0, tt->ver_major, tt->ver_minor, tt->patchlevel,
                   tt->points, tt->deathdnum, tt->deathlev, tt->maxlvl,
                   tt->hp, tt->maxhp, tt->deaths, tt->deathdate,
                   tt->birthdate, tt->uid);
    if (tt->ver_major < 3 || (tt->ver_major == 3 && tt->ver_minor < 3))
        (void) fprintf(rfile, fmt32, tt->plrole[0], tt->plgend[0]);
    else
        (void) fprintf(rfile, fmt33, tt->plrole, tt->plrace, tt->plgend,
                       tt->plalign);
    (void) fprintf(rfile, fmtX, onlyspace(tt->name) ? "_" : tt->name,
                   tt->death);

#ifdef NO_SCAN_BRACK
    nsb_unmung_line(tt->name);
    nsb_unmung_line(tt->death);
#endif
}

RESTORE_WARNING_FORMAT_NONLITERAL

#ifdef XLOGFILE

/* as tab is never used in eg. gp.plname or death, no need to mangle those. */
static void
writexlentry(FILE *rfile, struct toptenentry *tt, int how)
{
#define Fprintf (void) fprintf
#define XLOG_SEP '\t' /* xlogfile field separator. */
    char buf[BUFSZ], tmpbuf[DTHSZ + 1];
    char achbuf[N_ACH * 40];

    Sprintf(buf, "version=%d.%d.%d", tt->ver_major, tt->ver_minor,
            tt->patchlevel);
    Sprintf(eos(buf), "%cpoints=%ld%cdeathdnum=%d%cdeathlev=%d", XLOG_SEP,
            tt->points, XLOG_SEP, tt->deathdnum, XLOG_SEP, tt->deathlev);
    Sprintf(eos(buf), "%cmaxlvl=%d%chp=%d%cmaxhp=%d", XLOG_SEP, tt->maxlvl,
            XLOG_SEP, tt->hp, XLOG_SEP, tt->maxhp);
    Sprintf(eos(buf), "%cdeaths=%d%cdeathdate=%ld%cbirthdate=%ld%cuid=%d",
            XLOG_SEP, tt->deaths, XLOG_SEP, tt->deathdate, XLOG_SEP,
            tt->birthdate, XLOG_SEP, tt->uid);
    Fprintf(rfile, "%s", buf);
    Sprintf(buf, "%crole=%s%crace=%s%cgender=%s%calign=%s", XLOG_SEP,
            tt->plrole, XLOG_SEP, tt->plrace, XLOG_SEP, tt->plgend, XLOG_SEP,
            tt->plalign);
    /* make a copy of death reason that doesn't include ", while helpless" */
    formatkiller(tmpbuf, sizeof tmpbuf, how, FALSE);
    Fprintf(rfile, "%s%cname=%s%cdeath=%s",
            buf, /* (already includes separator) */
            XLOG_SEP, gp.plname, XLOG_SEP, tmpbuf);
    if (gm.multi)
        Fprintf(rfile, "%cwhile=%s", XLOG_SEP,
                gm.multi_reason ? gm.multi_reason : "helpless");
    Fprintf(rfile, "%cconduct=0x%lx%cturns=%ld%cachieve=0x%lx", XLOG_SEP,
            encodeconduct(), XLOG_SEP, gm.moves, XLOG_SEP,
            encodeachieve(FALSE));
    Fprintf(rfile, "%cachieveX=%s", XLOG_SEP,
            encode_extended_achievements(achbuf));
    Fprintf(rfile, "%cconductX=%s", XLOG_SEP,
            encode_extended_conducts(buf)); /* reuse 'buf[]' */
    Fprintf(rfile, "%crealtime=%ld%cstarttime=%ld%cendtime=%ld", XLOG_SEP,
            urealtime.realtime, XLOG_SEP,
            timet_to_seconds(ubirthday), XLOG_SEP,
            timet_to_seconds(urealtime.finish_time));
    Fprintf(rfile, "%cgender0=%s%calign0=%s", XLOG_SEP,
            genders[flags.initgend].filecode, XLOG_SEP,
            aligns[1 - u.ualignbase[A_ORIGINAL]].filecode);
    Fprintf(rfile, "%cflags=0x%lx", XLOG_SEP, encodexlogflags());
    Fprintf(rfile, "%cgold=%ld", XLOG_SEP,
            money_cnt(gi.invent) + hidden_gold(TRUE));
    Fprintf(rfile, "%cwish_cnt=%ld", XLOG_SEP, u.uconduct.wishes);
    Fprintf(rfile, "%carti_wish_cnt=%ld", XLOG_SEP, u.uconduct.wisharti);
    Fprintf(rfile, "%cbones=%ld", XLOG_SEP, u.uroleplay.numbones);
    Fprintf(rfile, "\n");
#undef XLOG_SEP
}

static long
encodexlogflags(void)
{
    long e = 0L;

    if (wizard)
        e |= 1L << 0;
    if (discover)
        e |= 1L << 1;
    if (!u.uroleplay.numbones)
        e |= 1L << 2;

    return e;
}

static long
encodeconduct(void)
{
    long e = 0L;

    if (!u.uconduct.food)
        e |= 1L << 0;
    if (!u.uconduct.unvegan)
        e |= 1L << 1;
    if (!u.uconduct.unvegetarian)
        e |= 1L << 2;
    if (!u.uconduct.gnostic)
        e |= 1L << 3;
    if (!u.uconduct.weaphit)
        e |= 1L << 4;
    if (!u.uconduct.killer)
        e |= 1L << 5;
    if (!u.uconduct.literate)
        e |= 1L << 6;
    if (!u.uconduct.polypiles)
        e |= 1L << 7;
    if (!u.uconduct.polyselfs)
        e |= 1L << 8;
    if (!u.uconduct.wishes)
        e |= 1L << 9;
    if (!u.uconduct.wisharti)
        e |= 1L << 10;
    if (!num_genocides())
        e |= 1L << 11;
    /* one bit isn't really adequate for sokoban conduct:
       reporting "obeyed sokoban rules" is misleading if sokoban wasn't
       completed or at least attempted; however, suppressing that when
       sokoban was never entered, as we do here, risks reporting
       "violated sokoban rules" when no such thing occurred; this can
       be disambiguated in xlogfile post-processors by testing the
       entered-sokoban bit in the 'achieve' field */
    if (!u.uconduct.sokocheat && sokoban_in_play())
        e |= 1L << 12;

    return e;
}

static long
encodeachieve(
    boolean secondlong) /* False: handle achievements 1..31, True: 32..62 */
{
    int i, achidx, offset;
    long r = 0L;

    /*
     * 32: portable limit for 'long'.
     * Force 32 even on configurations that are using 64 bit longs.
     *
     * We use signed long and limit ourselves to 31 bits since tools
     * that post-process xlogfile might not be able to cope with
     * 'unsigned long'.
     */
    offset = secondlong ? (32 - 1) : 0;
    for (i = 0; u.uachieved[i]; ++i) {
        achidx = u.uachieved[i] - offset;
        if (achidx > 0 && achidx < 32) /* value 1..31 sets bit 0..30 */
            r |= 1L << (achidx - 1);
    }
    return r;
}

/* add the achievement or conduct comma-separated to string */
static void
add_achieveX(char *buf, const char *achievement, boolean condition)
{
    if (condition) {
        if (buf[0] != '\0') {
            Strcat(buf, ",");
        }
        Strcat(buf, achievement);
    }
}

static char *
encode_extended_achievements(char *buf)
{
    char rnkbuf[40];
    const char *achievement = NULL;
    int i, achidx, absidx;

    buf[0] = '\0';
    for (i = 0; u.uachieved[i]; i++) {
        achidx = u.uachieved[i];
        absidx = abs(achidx);
        switch (absidx) {
        case ACH_UWIN:
            achievement = "ascended";
            break;
        case ACH_ASTR:
            achievement = "entered_astral_plane";
            break;
        case ACH_ENDG:
            achievement = "entered_elemental_planes";
            break;
        case ACH_AMUL:
            achievement = "obtained_the_amulet_of_yendor";
            break;
        case ACH_INVK:
            achievement = "performed_the_invocation_ritual";
            break;
        case ACH_BOOK:
            achievement = "obtained_the_book_of_the_dead";
            break;
        case ACH_BELL:
            achievement = "obtained_the_bell_of_opening";
            break;
        case ACH_CNDL:
            achievement = "obtained_the_candelabrum_of_invocation";
            break;
        case ACH_HELL:
            achievement = "entered_gehennom";
            break;
        case ACH_MEDU:
            achievement = "defeated_medusa";
            break;
        case ACH_MINE_PRIZE:
            achievement = "obtained_the_luckstone_from_the_mines";
            break;
        case ACH_SOKO_PRIZE:
            achievement = "obtained_the_sokoban_prize";
            break;
        case ACH_ORCL:
            achievement = "consulted_the_oracle";
            break;
        case ACH_NOVL:
            achievement = "read_a_discworld_novel";
            break;
        case ACH_MINE:
            achievement = "entered_the_gnomish_mines";
            break;
        case ACH_TOWN:
            achievement = "entered_mine_town";
            break;
        case ACH_SHOP:
            achievement = "entered_a_shop";
            break;
        case ACH_TMPL:
            achievement = "entered_a_temple";
            break;
        case ACH_SOKO:
            achievement = "entered_sokoban";
            break;
        case ACH_BGRM:
            achievement = "entered_bigroom";
            break;
        case ACH_TUNE:
            achievement = "learned_castle_drawbridge_tune";
            break;
        /* rank 0 is the starting condition, not an achievement; 8 is Xp 30 */
        case ACH_RNK1: case ACH_RNK2: case ACH_RNK3: case ACH_RNK4:
        case ACH_RNK5: case ACH_RNK6: case ACH_RNK7: case ACH_RNK8:
            Sprintf(rnkbuf, "attained_the_rank_of_%s",
                    rank_of(rank_to_xlev(absidx - (ACH_RNK1 - 1)),
                            Role_switch, (achidx < 0) ? TRUE : FALSE));
            strNsubst(rnkbuf, " ", "_", 0); /* replace every ' ' with '_' */
            achievement = lcase(rnkbuf);
            break;
        default:
            continue;
        }
        add_achieveX(buf, achievement, TRUE);
    }

    return buf;
}

static char *
encode_extended_conducts(char *buf)
{
    buf[0] = '\0';
    add_achieveX(buf, "foodless",     !u.uconduct.food);
    add_achieveX(buf, "vegan",        !u.uconduct.unvegan);
    add_achieveX(buf, "vegetarian",   !u.uconduct.unvegetarian);
    add_achieveX(buf, "atheist",      !u.uconduct.gnostic);
    add_achieveX(buf, "weaponless",   !u.uconduct.weaphit);
    add_achieveX(buf, "pacifist",     !u.uconduct.killer);
    add_achieveX(buf, "illiterate",   !u.uconduct.literate);
    add_achieveX(buf, "polyless",     !u.uconduct.polypiles);
    add_achieveX(buf, "polyselfless", !u.uconduct.polyselfs);
    add_achieveX(buf, "wishless",     !u.uconduct.wishes);
    add_achieveX(buf, "artiwishless", !u.uconduct.wisharti);
    add_achieveX(buf, "genocideless", !num_genocides());
    if (sokoban_in_play())
        add_achieveX(buf, "sokoban",  !u.uconduct.sokocheat);
    add_achieveX(buf, "blind",        u.uroleplay.blind);
    add_achieveX(buf, "nudist",       u.uroleplay.nudist);
    add_achieveX(buf, "bonesless",    !flags.bones);

    return buf;
}

#endif /* XLOGFILE */

static void
free_ttlist(struct toptenentry *tt)
{
    struct toptenentry *ttnext;

    while (tt->points > 0) {
        ttnext = tt->tt_next;
        dealloc_ttentry(tt);
        tt = ttnext;
    }
    dealloc_ttentry(tt);
}

void
topten(int how, time_t when)
{
    register struct toptenentry *t0, *tprev;
    struct toptenentry *t1;
    FILE *rfile;
#ifdef LOGFILE
    FILE *lfile;
#endif
#ifdef XLOGFILE
    FILE *xlfile;
#endif
    int uid = getuid();
    int rank, rank0 = -1, rank1 = 0;
    int occ_cnt = sysopt.persmax;
    int flg = 0;
    boolean t0_used, skip_scores;

#ifdef UPDATE_RECORD_IN_PLACE
    final_fpos = 0L;
#endif
    /* If we are in the midst of a panic, cut out topten entirely.
     * topten uses alloc() several times, which will lead to
     * problems if the panic was the result of an alloc() failure.
     */
    if (gp.program_state.panicking)
        return;

    if (iflags.toptenwin) {
        gt.toptenwin = create_nhwindow(NHW_TEXT);
    }

#if defined(HANGUPHANDLING)
#define HUP if (!gp.program_state.done_hup)
#else
#define HUP
#endif

#ifdef TOS
    restore_colors(); /* make sure the screen is black on white */
#endif
    /* create a new 'topten' entry */
    t0_used = FALSE;
    t0 = newttentry();
    *t0 = zerott;
    t0->ver_major = VERSION_MAJOR;
    t0->ver_minor = VERSION_MINOR;
    t0->patchlevel = PATCHLEVEL;
    t0->points = u.urexp;
    t0->deathdnum = u.uz.dnum;
    /* deepest_lev_reached() is in terms of depth(), and reporting the
     * deepest level reached in the dungeon death occurred in doesn't
     * seem right, so we have to report the death level in depth() terms
     * as well (which also seems reasonable since that's all the player
     * sees on the screen anyway)
     */
    t0->deathlev = observable_depth(&u.uz);
    t0->maxlvl = deepest_lev_reached(TRUE);
    t0->hp = u.uhp;
    t0->maxhp = u.uhpmax;
    t0->deaths = u.umortality;
    t0->uid = uid;
    copynchars(t0->plrole, gu.urole.filecode, ROLESZ);
    copynchars(t0->plrace, gu.urace.filecode, ROLESZ);
    copynchars(t0->plgend, genders[flags.female].filecode, ROLESZ);
    copynchars(t0->plalign, aligns[1 - u.ualign.type].filecode, ROLESZ);
    copynchars(t0->name, gp.plname, NAMSZ);
    formatkiller(t0->death, sizeof t0->death, how, TRUE);
    t0->birthdate = yyyymmdd(ubirthday);
    t0->deathdate = yyyymmdd(when);
    t0->tt_next = 0;
#ifdef UPDATE_RECORD_IN_PLACE
    t0->fpos = -1L;
#endif

#ifdef LOGFILE /* used for debugging (who dies of what, where) */
    if (lock_file(LOGFILE, SCOREPREFIX, 10)) {
        if (!(lfile = fopen_datafile(LOGFILE, "a", SCOREPREFIX))) {
            HUP raw_print("Cannot open log file!");
        } else {
            writeentry(lfile, t0);
            (void) fclose(lfile);
        }
        unlock_file(LOGFILE);
    }
#endif /* LOGFILE */
#ifdef XLOGFILE
    if (lock_file(XLOGFILE, SCOREPREFIX, 10)) {
        if (!(xlfile = fopen_datafile(XLOGFILE, "a", SCOREPREFIX))) {
            HUP raw_print("Cannot open extended log file!");
        } else {
            writexlentry(xlfile, t0, how);
            (void) fclose(xlfile);
        }
        unlock_file(XLOGFILE);
    }
#endif /* XLOGFILE */

    if (wizard || discover) {
        if (how != PANICKED)
            HUP {
                char pbuf[BUFSZ];

                topten_print("");
                Sprintf(pbuf,
             "Since you were in %s mode, the score list will not be checked.",
                        wizard ? "wizard" : "discover");
                topten_print(pbuf);
            }
        goto showwin;
    }

    if (!lock_file(RECORD, SCOREPREFIX, 60))
        goto destroywin;

#ifdef UPDATE_RECORD_IN_PLACE
    rfile = fopen_datafile(RECORD, "r+", SCOREPREFIX);
#else
    rfile = fopen_datafile(RECORD, "r", SCOREPREFIX);
#endif

    if (!rfile) {
        HUP raw_print("Cannot open record file!");
        unlock_file(RECORD);
        goto destroywin;
    }

    HUP topten_print("");

    /* assure minimum number of points */
    if (t0->points < sysopt.pointsmin)
        t0->points = 0;

    t1 = tt_head = newttentry();
    tprev = 0;
    /* rank0: -1 undefined, 0 not_on_list, n n_th on list */
    for (rank = 1; ; ) {
        readentry(rfile, t1);
        if (t1->points < sysopt.pointsmin)
            t1->points = 0;
        if (rank0 < 0 && t1->points < t0->points) {
            rank0 = rank++;
            if (tprev == 0)
                tt_head = t0;
            else
                tprev->tt_next = t0;
            t0->tt_next = t1;
#ifdef UPDATE_RECORD_IN_PLACE
            t0->fpos = t1->fpos; /* insert here */
#endif
            t0_used = TRUE;
            occ_cnt--;
            flg++; /* ask for a rewrite */
        } else
            tprev = t1;

        if (t1->points == 0)
            break;
        if ((sysopt.pers_is_uid ? t1->uid == t0->uid
                                : strncmp(t1->name, t0->name, NAMSZ) == 0)
            && !strncmp(t1->plrole, t0->plrole, ROLESZ) && --occ_cnt <= 0) {
            if (rank0 < 0) {
                rank0 = 0;
                rank1 = rank;
                HUP {
                    char pbuf[BUFSZ];

                    Sprintf(pbuf,
                         "You didn't beat your previous score of %ld points.",
                            t1->points);
                    topten_print(pbuf);
                    topten_print("");
                }
            }
            if (occ_cnt < 0) {
                flg++;
                continue;
            }
        }
        if (rank <= sysopt.entrymax) {
            t1->tt_next = newttentry();
            t1 = t1->tt_next;
            rank++;
        }
        if (rank > sysopt.entrymax) {
            t1->points = 0;
            break;
        }
    }
    if (flg) { /* rewrite record file */
#ifdef UPDATE_RECORD_IN_PLACE
        (void) fseek(rfile, (t0->fpos >= 0) ? t0->fpos : final_fpos, SEEK_SET);
#else
        (void) fclose(rfile);
        if (!(rfile = fopen_datafile(RECORD, "w", SCOREPREFIX))) {
            HUP raw_print("Cannot write record file");
            unlock_file(RECORD);
            free_ttlist(tt_head);
            goto destroywin;
        }
#endif /* UPDATE_RECORD_IN_PLACE */
        if (!done_stopprint)
            if (rank0 > 0) {
                if (rank0 <= 10) {
                    topten_print("You made the top ten list!");
                } else {
                    char pbuf[BUFSZ];

                    Sprintf(pbuf,
                            "You reached the %d%s place on the top %d list.",
                            rank0, ordin(rank0), sysopt.entrymax);
                    topten_print(pbuf);
                }
                topten_print("");
            }
    }
    skip_scores = !flags.end_top && !flags.end_around && !flags.end_own;
    if (rank0 == 0)
        rank0 = rank1;
    if (rank0 <= 0)
        rank0 = rank;
    if (!skip_scores && !done_stopprint)
        outheader();
    for (t1 = tt_head, rank = 1; t1->points != 0; t1 = t1->tt_next, ++rank) {
        if (flg
#ifdef UPDATE_RECORD_IN_PLACE
            && rank >= rank0
#endif
            )
            writeentry(rfile, t1);
        if (skip_scores || done_stopprint)
            continue;
        if (rank <= flags.end_top
            || (rank >= rank0 - flags.end_around
                && rank <= rank0 + flags.end_around)
            || (flags.end_own && (sysopt.pers_is_uid
                                  ? t1->uid == t0->uid
                                  : !strncmp(t1->name, t0->name, NAMSZ)))) {
            if (rank == rank0 - flags.end_around
                && rank0 > flags.end_top + flags.end_around + 1
                && !flags.end_own)
                topten_print("");

            if (rank != rank0) {
                outentry(rank, t1, FALSE);
            } else if (!rank1) {
                outentry(rank, t1, TRUE);
            } else {
                outentry(rank, t1, TRUE);
                outentry(0, t0, TRUE);
            }
        }
    }
    if (rank0 >= rank)
        if (!skip_scores && !done_stopprint)
            outentry(0, t0, TRUE);
#ifdef UPDATE_RECORD_IN_PLACE
    if (flg) {
#ifdef TRUNCATE_FILE
        /* if a reasonable way to truncate a file exists, use it */
        truncate_file(rfile);
#else
        /* use sentinel record rather than relying on truncation */
        *t1 = zerott;
        t1->points = 0L; /* [redundant] terminates file when read back in */
        t1->plrole[0] = t1->plrace[0] = t1->plgend[0] = t1->plalign[0] = '-';
        t1->birthdate = t1->deathdate = yyyymmdd((time_t) 0L);
        Strcpy(t1->name, "@");
        Strcpy(t1->death, "<eod>\n"); /* end of data */
        writeentry(rfile, t1);
        /* note: there might be junk (if file has shrunk due to shorter
           entries supplanting longer ones) after this dummy entry, but
           reading and/or updating will ignore it */
        (void) fflush(rfile);
#endif /* TRUNCATE_FILE */
    }
#endif /* UPDATE_RECORD_IN_PLACE */
    (void) fclose(rfile);
    unlock_file(RECORD);
    free_ttlist(tt_head);

 showwin:
    if (!done_stopprint) {
        if (iflags.toptenwin) {
            display_nhwindow(gt.toptenwin, TRUE);
        } else {
            /* when not a window, we need something comparable to more()
               but can't use it directly because we aren't dealing with
               the message window */
            ;
        }
    }
 destroywin:
    if (!t0_used)
        dealloc_ttentry(t0);
    if (iflags.toptenwin) {
        destroy_nhwindow(gt.toptenwin);
        gt.toptenwin = WIN_ERR;
    }
}

static void
outheader(void)
{
    char linebuf[BUFSZ];
    register char *bp;

    Strcpy(linebuf, " No  Points     Name");
    bp = eos(linebuf);
    while (bp < linebuf + COLNO - 9)
        *bp++ = ' ';
    Strcpy(bp, "Hp [max]");
    topten_print(linebuf);
}

DISABLE_WARNING_FORMAT_NONLITERAL

/* so>0: standout line; so=0: ordinary line */
static void
outentry(int rank, struct toptenentry *t1, boolean so)
{
    boolean second_line = TRUE;
    char linebuf[BUFSZ];
    char *bp, hpbuf[24], linebuf3[BUFSZ];
    int hppos, lngr;

    linebuf[0] = '\0';
    if (rank)
        Sprintf(eos(linebuf), "%3d", rank);
    else
        Strcat(linebuf, "   ");

    Sprintf(eos(linebuf), " %10ld  %.10s", t1->points ? t1->points : u.urexp,
            t1->name);
    Sprintf(eos(linebuf), "-%s", t1->plrole);
    if (t1->plrace[0] != '?')
        Sprintf(eos(linebuf), "-%s", t1->plrace);
    /* Printing of gender and alignment is intentional.  It has been
     * part of the NetHack Geek Code, and illustrates a proper way to
     * specify a character from the command line.
     */
    Sprintf(eos(linebuf), "-%s", t1->plgend);
    if (t1->plalign[0] != '?')
        Sprintf(eos(linebuf), "-%s ", t1->plalign);
    else
        Strcat(linebuf, " ");
    if (!strncmp("escaped", t1->death, 7)) {
        Sprintf(eos(linebuf), "escaped the dungeon %s[max level %d]",
                !strncmp(" (", t1->death + 7, 2) ? t1->death + 7 + 2 : "",
                t1->maxlvl);
        /* fixup for closing paren in "escaped... with...Amulet)[max..." */
        if ((bp = strchr(linebuf, ')')) != 0)
            *bp = (t1->deathdnum == astral_level.dnum) ? '\0' : ' ';
        second_line = FALSE;
    } else if (!strncmp("ascended", t1->death, 8)) {
        Sprintf(eos(linebuf), "ascended to demigod%s-hood",
                (t1->plgend[0] == 'F') ? "dess" : "");
        second_line = FALSE;
    } else {
        if (!strncmp(t1->death, "quit", 4)) {
            Strcat(linebuf, "quit");
            second_line = FALSE;
        } else if (!strncmp(t1->death, "died of st", 10)) {
            Strcat(linebuf, "starved to death");
            second_line = FALSE;
        } else if (!strncmp(t1->death, "choked", 6)) {
            Sprintf(eos(linebuf), "choked on h%s food",
                    (t1->plgend[0] == 'F') ? "er" : "is");
        } else if (!strncmp(t1->death, "poisoned", 8)) {
            Strcat(linebuf, "was poisoned");
        } else if (!strncmp(t1->death, "crushed", 7)) {
            Strcat(linebuf, "was crushed to death");
        } else if (!strncmp(t1->death, "petrified by ", 13)) {
            Strcat(linebuf, "turned to stone");
        } else
            Strcat(linebuf, "died");

        if (t1->deathdnum == astral_level.dnum) {
            const char *arg, *fmt = " on the Plane of %s";

            switch (t1->deathlev) {
            case -5:
                fmt = " on the %s Plane";
                arg = "Astral";
                break;
            case -4:
                arg = "Water";
                break;
            case -3:
                arg = "Fire";
                break;
            case -2:
                arg = "Air";
                break;
            case -1:
                arg = "Earth";
                break;
            default:
                arg = "Void";
                break;
            }
            Sprintf(eos(linebuf), fmt, arg);
        } else {
            Sprintf(eos(linebuf), " in %s", gd.dungeons[t1->deathdnum].dname);
            if (t1->deathdnum != knox_level.dnum)
                Sprintf(eos(linebuf), " on level %d", t1->deathlev);
            if (t1->deathlev != t1->maxlvl)
                Sprintf(eos(linebuf), " [max %d]", t1->maxlvl);
        }

        /* kludge for "quit while already on Charon's boat" */
        if (!strncmp(t1->death, "quit ", 5))
            Strcat(linebuf, t1->death + 4);
    }
    Strcat(linebuf, ".");

    /* Quit, starved, ascended, and escaped contain no second line */
    if (second_line) {
        bp = eos(linebuf);
        Sprintf(bp, "  %c%s.", highc(*(t1->death)), t1->death + 1);
        /* fix up "Killed by Mr. Asidonhopo; the shopkeeper"; that starts
           with a comma but has it changed to semi-colon to keep the comma
           out of 'record'; change it back for display */
        (void) strsubst(bp, "; the ", ", the ");
    }

    lngr = (int) strlen(linebuf);
    if (t1->hp <= 0)
        hpbuf[0] = '-', hpbuf[1] = '\0';
    else
        Sprintf(hpbuf, "%d", t1->hp);
    /* beginning of hp column after padding (not actually padded yet) */
    hppos = COLNO - (int) (sizeof "  Hp [max]" - sizeof "");
    while (lngr >= hppos) {
        for (bp = eos(linebuf); !(*bp == ' ' && bp - linebuf < hppos); bp--)
            ;
        /* special case: word is too long, wrap in the middle */
        if (linebuf + 15 >= bp)
            bp = linebuf + hppos - 1;
        /* special case: if about to wrap in the middle of maximum
           dungeon depth reached, wrap in front of it instead */
        if (bp > linebuf + 5 && !strncmp(bp - 5, " [max", 5))
            bp -= 5;
        if (*bp != ' ')
            Strcpy(linebuf3, bp);
        else
            Strcpy(linebuf3, bp + 1);
        *bp = '\0';
        if (so) {
            while (bp < linebuf + (COLNO - 1))
                *bp++ = ' ';
            *bp = '\0';
            topten_print_bold(linebuf);
        } else
            topten_print(linebuf);
        Snprintf(linebuf, sizeof(linebuf), "%15s %s", "", linebuf3);
        lngr = Strlen(linebuf);
    }
    /* beginning of hp column not including padding */
    hppos = COLNO - 7 - (int) strlen(hpbuf);
    bp = eos(linebuf);

    if (bp <= linebuf + hppos) {
        /* pad any necessary blanks to the hit point entry */
        while (bp < linebuf + hppos)
            *bp++ = ' ';
        Strcpy(bp, hpbuf);
        Sprintf(eos(bp), " %s[%d]",
                (t1->maxhp < 10) ? "  " : (t1->maxhp < 100) ? " " : "",
                t1->maxhp);
    }

    if (so) {
        bp = eos(linebuf);
        while (bp < linebuf + (COLNO - 1))
            *bp++ = ' ';
        *bp = '\0';
        topten_print_bold(linebuf);
    } else
        topten_print(linebuf);
}

RESTORE_WARNING_FORMAT_NONLITERAL

static int
score_wanted(
    boolean current_ver,
    int rank,
    struct toptenentry *t1,
    int playerct,
    const char **players,
    int uid)
{
    const char *arg, *nxt;
    int i;

    if (current_ver && (t1->ver_major != VERSION_MAJOR
                        || t1->ver_minor != VERSION_MINOR
                        || t1->patchlevel != PATCHLEVEL))
        return 0;

    if (sysopt.pers_is_uid && !playerct && t1->uid == uid)
        return 1;

    /*
     * FIXME:
     *  This selection produces a union (OR) of criteria rather than
     *  an intersection (AND).  So
     *    nethack -s -u igor -p Cav -r Hum
     *  will list all entries for name igor regardless of role or race
     *  plus all entries for cave dwellers regardless of name or race
     *  plus all entries for humans regardless of name or role.
     *
     *  It would be more useful if it only chose human cave dwellers
     *  named igor.  That would be pretty straightforward if only one
     *  instance of each of the criteria were possible, but
     *    nethack -s -u igor -u ayn -p Cav -p Pri -r Hum -r Dwa
     *  should list human cave dwellers named igor and human cave
     *  dwellers named ayn plus dwarven cave dwellers named igor and
     *  dwarven cave dwellers named ayn plus human priest[esse]s named
     *  igor and human priest[esse]s named ayn (the combination of
     *  dwarven priest[esse]s doesn't occur but the selection can test
     *  entries without being aware of such; it just won't find any
     *  matches for that).  An extra initial pass of the command line
     *  to collect all criteria before testing any entry is needed to
     *  accomplish this.  And we might need to drop support for
     *  pre-3.3.0 entries (old elf role) depending on how the criteria
     *  matching is performed.
     *
     *  It also ought to extended to handle
     *    nethack -s -u igor-Cav-Hum
     *  Alignment and gender could be useful too but no one has ever
     *  clamored for them.  Presumably if they care they postprocess
     *  with some custom tool.
     */

    for (i = 0; i < playerct; i++) {
        arg = players[i];
        if (arg[0] == '-' && arg[1] == 'u' && arg[2] != '\0')
            arg += 2; /* handle '-uname' */

        if (arg[0] == '-' && strchr("pru", arg[1]) && !arg[2]
            && i + 1 < playerct) {
            nxt = players[i + 1];
            if ((arg[1] == 'p' && str2role(nxt) == str2role(t1->plrole))
                || (arg[1] == 'r' && str2race(nxt) == str2race(t1->plrace))
                /* handle '-u name' */
                || (arg[1] == 'u' && (!strcmp(nxt, "all")
                                      || !strncmp(t1->name, nxt, NAMSZ))))
                return 1;
            i++;
        } else if (!strcmp(arg, "all")
                   || !strncmp(t1->name, arg, NAMSZ)
                   || (arg[0] == '-' && arg[1] == t1->plrole[0] && !arg[2])
                   || (digit(arg[0]) && rank <= atoi(arg)))
            return 1;
    }
    return 0;
}

/*
 * print selected parts of score list.
 * argc >= 2, with argv[0] untrustworthy (directory names, et al.),
 * and argv[1] starting with "-s".
 * caveat: some shells might allow argv elements to be arbitrarily long.
 */
void
prscore(int argc, char **argv)
{
    const char **players, *player0;
    int i, playerct, rank;
    register struct toptenentry *t1;
    FILE *rfile;
    char pbuf[BUFSZ], *p;
    unsigned ln;
    int uid = -1;
    boolean current_ver = TRUE, init_done = FALSE, match_found = FALSE;

    /* expect "-s" or "--scores"; "-s<anything> is accepted */
    ln = (argc < 2) ? 0U
         : ((p = strchr(argv[1], ' ')) != 0) ? (unsigned) (p - argv[1])
           : Strlen(argv[1]);
    if (ln < 2 || (strncmp(argv[1], "-s", 2)
                   && strcmp(argv[1], "--scores"))) {
        raw_printf("prscore: bad arguments (%d)", argc);
        return;
    }

    rfile = fopen_datafile(RECORD, "r", SCOREPREFIX);
    if (!rfile) {
        raw_print("Cannot open record file!");
        return;
    }

#ifdef AMIGA
    {
        extern winid amii_rawprwin;

        init_nhwindows(&argc, argv);
        amii_rawprwin = create_nhwindow(NHW_TEXT);
    }
#endif

    /* If the score list isn't after a game, we never went through
     * initialization. */
    if (wiz1_level.dlevel == 0) {
        dlb_init();
        init_dungeons();
        init_done = TRUE;
    }

    /* to get here, argv[1] either starts with "-s" or is "--scores" without
       trailing stuff; for "-s<anything>" treat <anything> as separate arg */
    if (argv[1][1] == '-' || !argv[1][2]) {
        argc--;
        argv++;
    } else { /* concatenated arg string; use up "-s" but keep argc,argv */
        argv[1] += 2;
    }
    /* -v doesn't take a version number arg; it means 'all versions present
       in the file' instead of the default of only the current version;
       unlike -s, we don't accept "-v<anything>" for non-empty <anything> */
    if (argc > 1 && !strcmp(argv[1], "-v")) {
        current_ver = FALSE;
        argc--;
        argv++;
    }

    if (argc <= 1) {
        if (sysopt.pers_is_uid) {
            uid = getuid();
            playerct = 0;
            players = (const char **) 0;
        } else {
            player0 = gp.plname;
            if (!*player0)
                player0 = "all"; /* if no plname[], show all scores
                                  * (possibly filtered by '-v') */
            playerct = 1;
            players = &player0;
        }
    } else {
        playerct = --argc;
        players = (const char **) ++argv;
    }
    raw_print("");

    t1 = tt_head = newttentry();
    for (rank = 1; ; rank++) {
        readentry(rfile, t1);
        if (t1->points == 0)
            break;
        if (!match_found
            && score_wanted(current_ver, rank, t1, playerct, players, uid))
            match_found = TRUE;
        t1->tt_next = newttentry();
        t1 = t1->tt_next;
    }

    (void) fclose(rfile);
    if (init_done) {
        free_dungeons();
        dlb_cleanup();
    }

    if (match_found) {
        outheader();
        t1 = tt_head;
        for (rank = 1; t1->points != 0; rank++, t1 = t1->tt_next) {
            if (score_wanted(current_ver, rank, t1, playerct, players, uid))
                (void) outentry(rank, t1, FALSE);
        }
    } else {
        Sprintf(pbuf, "Cannot find any %sentries for ",
                current_ver ? "current " : "");
        if (playerct < 1) {
            Strcat(pbuf, "you");
        } else {
            /* minor bug: 'nethack -s -u ziggy' will say "any of"
               even though the '-u' doesn't indicate multiple names */
            if (playerct > 1)
                Strcat(pbuf, "any of ");
            for (i = 0; i < playerct; i++) {
                /* accept '-u name' and '-uname' as well as just 'name'
                   so skip '-u' for the none-found feedback */
                if (!strncmp(players[i], "-u", 2)) {
                    if (!players[i][2])
                        continue;
                    players[i] += 2;
                }
                /* stop printing players if there are too many to fit */
                if (strlen(pbuf) + strlen(players[i]) + 2 >= BUFSZ) {
                    if (strlen(pbuf) < BUFSZ - 4)
                        Strcat(pbuf, "...");
                    else
                        Strcpy(pbuf + strlen(pbuf) - 4, "...");
                    break;
                }
                Strcat(pbuf, players[i]);
                if (i < playerct - 1) {
                    if (players[i][0] == '-' && strchr("pr", players[i][1])
                        && players[i][2] == 0)
                        Strcat(pbuf, " ");
                    else
                        Strcat(pbuf, ":");
                }
            }
        }
        /* append end-of-sentence punctuation if there is room */
        if (strlen(pbuf) < BUFSZ - 1)
            Strcat(pbuf, ".");
        raw_print(pbuf);
        raw_printf("Usage: %s -s [-v] <playertypes> [maxrank] [playernames]",
                   gh.hname);
        raw_printf("Player types are: [-p role] [-r race]");
    }
    free_ttlist(tt_head);
#ifdef AMIGA
    {
        extern winid amii_rawprwin;

        display_nhwindow(amii_rawprwin, 1);
        destroy_nhwindow(amii_rawprwin);
        amii_rawprwin = WIN_ERR;
    }
#endif
}

static int
classmon(char *plch)
{
    int i;

    /* Look for this role in the role table */
    for (i = 0; roles[i].name.m; i++) {
        if (!strncmp(plch, roles[i].filecode, ROLESZ)) {
            if (roles[i].mnum != NON_PM)
                return roles[i].mnum;
            else
                return PM_HUMAN;
        }
    }
    /* this might be from a 3.2.x score for former Elf class */
    if (!strcmp(plch, "E"))
        return PM_RANGER;

    impossible("What weird role is this? (%s)", plch);
    return  PM_HUMAN_MUMMY;
}

/*
 * Get a random player name and class from the high score list,
 */
struct toptenentry *
get_rnd_toptenentry(void)
{
    int rank, i;
    FILE *rfile;
    register struct toptenentry *tt;
    static struct toptenentry tt_buf;

    rfile = fopen_datafile(RECORD, "r", SCOREPREFIX);
    if (!rfile) {
        impossible("Cannot open record file!");
        return NULL;
    }

    tt = &tt_buf;
    rank = rnd(sysopt.tt_oname_maxrank);
 pickentry:
    for (i = rank; i; i--) {
        readentry(rfile, tt);
        if (tt->points == 0)
            break;
    }

    if (tt->points == 0) {
        if (rank > 1) {
            rank = 1;
            rewind(rfile);
            goto pickentry;
        }
        tt = NULL;
    }

    (void) fclose(rfile);
    return tt;
}


/*
 * Attach random player name and class from high score list
 * to an object (for statues or morgue corpses).
 */
struct obj *
tt_oname(struct obj *otmp)
{
    struct toptenentry *tt;
    if (!otmp)
        return (struct obj *) 0;

    tt = get_rnd_toptenentry();

    if (!tt)
        return (struct obj *) 0;

    set_corpsenm(otmp, classmon(tt->plrole));
    if (tt->plgend[0] == 'F')
        otmp->spe = CORPSTAT_FEMALE;
    else if (tt->plgend[0] == 'M')
        otmp->spe = CORPSTAT_MALE;
    otmp = oname(otmp, tt->name, ONAME_NO_FLAGS);

    return otmp;
}

#ifdef NO_SCAN_BRACK
/* Lattice scanf isn't up to reading the scorefile.  What */
/* follows deals with that; I admit it's ugly. (KL) */
/* Now generally available (KL) */
static void
nsb_mung_line(p)
char *p;
{
    while ((p = strchr(p, ' ')) != 0)
        *p = '|';
}

static void
nsb_unmung_line(p)
char *p;
{
    while ((p = strchr(p, '|')) != 0)
        *p = ' ';
}
#endif /* NO_SCAN_BRACK */

/*topten.c*/
