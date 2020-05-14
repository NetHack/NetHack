/* NetHack 3.7	rumors.c	$NHDT-Date: 1583445339 2020/03/05 21:55:39 $  $NHDT-Branch: NetHack-3.6-Mar2020 $:$NHDT-Revision: 1.38 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "dlb.h"

/*      [note: this comment is fairly old, but still accurate for 3.1]
 * Rumors have been entirely rewritten to speed up the access.  This is
 * essential when working from floppies.  Using fseek() the way that's done
 * here means rumors following longer rumors are output more often than those
 * following shorter rumors.  Also, you may see the same rumor more than once
 * in a particular game (although the odds are highly against it), but
 * this also happens with real fortune cookies.  -dgk
 */

/*      3.6
 * The rumors file consists of a "do not edit" line, then a line containing
 * three sets of three counts (first two in decimal, third in hexadecimal).
 * The first set has the number of true rumors, the count in bytes for all
 * true rumors, and the file offset to the first one.  The second set has
 * the same group of numbers for the false rumors.  The third set has 0 for
 * count, 0 for size, and the file offset for end-of-file.  The offset of
 * the first true rumor plus the size of the true rumors matches the offset
 * of the first false rumor.  Likewise, the offset of the first false rumor
 * plus the size of the false rumors matches the offset for end-of-file.
 */

/*      3.1     [now obsolete for rumors but still accurate for oracles]
 * The rumors file consists of a "do not edit" line, a hexadecimal number
 * giving the number of bytes of useful/true rumors, followed by those
 * true rumors (one per line), followed by the useless/false/misleading/cute
 * rumors (also one per line).  Number of bytes of untrue rumors is derived
 * via fseek(EOF)+ftell().
 *
 * The oracles file consists of a "do not edit" comment, a decimal count N
 * and set of N+1 hexadecimal fseek offsets, followed by N multiple-line
 * records, separated by "---" lines.  The first oracle is a special case,
 * and placed there by 'makedefs'.
 */

static void FDECL(init_rumors, (dlb *));
static void FDECL(init_oracles, (dlb *));
static void FDECL(couldnt_open_file, (const char *));

static void
init_rumors(fp)
dlb *fp;
{
    static const char rumors_header[] = "%d,%ld,%lx;%d,%ld,%lx;0,0,%lx\n";
    int true_count, false_count; /* in file but not used here */
    unsigned long eof_offset;
    char line[BUFSZ];

    (void) dlb_fgets(line, sizeof line, fp); /* skip "don't edit" comment */
    (void) dlb_fgets(line, sizeof line, fp);
    if (sscanf(line, rumors_header, &true_count, &g.true_rumor_size,
               &g.true_rumor_start, &false_count, &g.false_rumor_size,
               &g.false_rumor_start, &eof_offset) == 7
        && g.true_rumor_size > 0L
        && g.false_rumor_size > 0L) {
        g.true_rumor_end = (long) g.true_rumor_start + g.true_rumor_size;
        /* assert( g.true_rumor_end == false_rumor_start ); */
        g.false_rumor_end = (long) g.false_rumor_start + g.false_rumor_size;
        /* assert( g.false_rumor_end == eof_offset ); */
    } else {
        g.true_rumor_size = -1L; /* init failed */
        (void) dlb_fclose(fp);
    }
}

/* exclude_cookie is a hack used because we sometimes want to get rumors in a
 * context where messages such as "You swallowed the fortune!" that refer to
 * cookies should not appear.  This has no effect for true rumors since none
 * of them contain such references anyway.
 */
char *
getrumor(truth, rumor_buf, exclude_cookie)
int truth; /* 1=true, -1=false, 0=either */
char *rumor_buf;
boolean exclude_cookie;
{
    dlb *rumors;
    long tidbit, beginning;
    char *endp, line[BUFSZ], xbuf[BUFSZ];

    rumor_buf[0] = '\0';
    if (g.true_rumor_size < 0L) /* we couldn't open RUMORFILE */
        return rumor_buf;

    rumors = dlb_fopen(RUMORFILE, "r");

    if (rumors) {
        int count = 0;
        int adjtruth;

        do {
            rumor_buf[0] = '\0';
            if (g.true_rumor_size == 0L) { /* if this is 1st outrumor() */
                init_rumors(rumors);
                if (g.true_rumor_size < 0L) { /* init failed */
                    Sprintf(rumor_buf, "Error reading \"%.80s\".", RUMORFILE);
                    return rumor_buf;
                }
            }
            /*
             *  input:      1    0   -1
             *   rn2 \ +1  2=T  1=T  0=F
             *   adj./ +0  1=T  0=F -1=F
             */
            switch (adjtruth = truth + rn2(2)) {
            case 2: /*(might let a bogus input arg sneak thru)*/
            case 1:
                beginning = (long) g.true_rumor_start;
                tidbit = rn2(g.true_rumor_size);
                break;
            case 0: /* once here, 0 => false rather than "either"*/
            case -1:
                beginning = (long) g.false_rumor_start;
                tidbit = rn2(g.false_rumor_size);
                break;
            default:
                impossible("strange truth value for rumor");
                return strcpy(rumor_buf, "Oops...");
            }
            (void) dlb_fseek(rumors, beginning + tidbit, SEEK_SET);
            (void) dlb_fgets(line, sizeof line, rumors);
            if (!dlb_fgets(line, sizeof line, rumors)
                || (adjtruth > 0 && dlb_ftell(rumors) > g.true_rumor_end)) {
                /* reached end of rumors -- go back to beginning */
                (void) dlb_fseek(rumors, beginning, SEEK_SET);
                (void) dlb_fgets(line, sizeof line, rumors);
            }
            if ((endp = index(line, '\n')) != 0)
                *endp = 0;
            Strcat(rumor_buf, xcrypt(line, xbuf));
        } while (
            count++ < 50 && exclude_cookie
            && (strstri(rumor_buf, "fortune") || strstri(rumor_buf, "pity")));
        (void) dlb_fclose(rumors);
        if (count >= 50)
            impossible("Can't find non-cookie rumor?");
        else if (!g.in_mklev) /* avoid exercizing wisdom for graffiti */
            exercise(A_WIS, (adjtruth > 0));
    } else {
        couldnt_open_file(RUMORFILE);
        g.true_rumor_size = -1; /* don't try to open it again */
    }

    /* this is safe either way, so do it always since we can't get the
     * definition out of makedefs.c
     */
#define PAD_RUMORS_TO
#ifdef PAD_RUMORS_TO
    /* remove padding */
    {
        char *x = eos(rumor_buf) - 1;

        while (x > rumor_buf && *x == '_')
            x--;
        *++x = '\n';
        *x = '\0';
    }
#endif
    return rumor_buf;
}

/*
 * test that the true/false rumor boundaries are valid.
 */
void
rumor_check()
{
    dlb *rumors = 0;
    winid tmpwin;
    char *endp, line[BUFSZ], xbuf[BUFSZ], rumor_buf[BUFSZ];

    if (g.true_rumor_size < 0L) { /* we couldn't open RUMORFILE */
 no_rumors:
        pline("rumors not accessible.");
        return;
    }

    rumors = dlb_fopen(RUMORFILE, "r");

    if (rumors) {
        long ftell_rumor_start = 0L;

        rumor_buf[0] = '\0';
        if (g.true_rumor_size == 0L) { /* if this is 1st outrumor() */
            init_rumors(rumors);
            if (g.true_rumor_size < 0L) {
                rumors = (dlb *) 0; /* init_rumors() closes it upon failure */
                goto no_rumors; /* init failed */
            }
        }
        tmpwin = create_nhwindow(NHW_TEXT);

        /*
         * reveal the values.
         */
        Sprintf(rumor_buf,
               "T start=%06ld (%06lx), end=%06ld (%06lx), size=%06ld (%06lx)",
            (long) g.true_rumor_start, g.true_rumor_start,
            g.true_rumor_end, (unsigned long) g.true_rumor_end,
            g.true_rumor_size,(unsigned long) g.true_rumor_size);
        putstr(tmpwin, 0, rumor_buf);
        Sprintf(rumor_buf,
               "F start=%06ld (%06lx), end=%06ld (%06lx), size=%06ld (%06lx)",
            (long) g.false_rumor_start, g.false_rumor_start,
            g.false_rumor_end, (unsigned long) g.false_rumor_end,
            g.false_rumor_size, (unsigned long) g.false_rumor_size);
        putstr(tmpwin, 0, rumor_buf);

        /*
         * check the first rumor (start of true rumors) by
         * skipping the first two lines.
         *
         * Then seek to the start of the false rumors (based on
         * the value read in rumors, and display it.
         */
        rumor_buf[0] = '\0';
        (void) dlb_fseek(rumors, (long) g.true_rumor_start, SEEK_SET);
        ftell_rumor_start = dlb_ftell(rumors);
        (void) dlb_fgets(line, sizeof line, rumors);
        if ((endp = index(line, '\n')) != 0)
            *endp = 0;
        Sprintf(rumor_buf, "T %06ld %s", ftell_rumor_start,
                xcrypt(line, xbuf));
        putstr(tmpwin, 0, rumor_buf);
        /* find last true rumor */
        while (dlb_fgets(line, sizeof line, rumors)
               && dlb_ftell(rumors) < g.true_rumor_end)
            continue;
        if ((endp = index(line, '\n')) != 0)
            *endp = 0;
        Sprintf(rumor_buf, "  %6s %s", "", xcrypt(line, xbuf));
        putstr(tmpwin, 0, rumor_buf);

        rumor_buf[0] = '\0';
        (void) dlb_fseek(rumors, (long) g.false_rumor_start, SEEK_SET);
        ftell_rumor_start = dlb_ftell(rumors);
        (void) dlb_fgets(line, sizeof line, rumors);
        if ((endp = index(line, '\n')) != 0)
            *endp = 0;
        Sprintf(rumor_buf, "F %06ld %s", ftell_rumor_start,
                xcrypt(line, xbuf));
        putstr(tmpwin, 0, rumor_buf);
        /* find last false rumor */
        while (dlb_fgets(line, sizeof line, rumors)
               && dlb_ftell(rumors) < g.false_rumor_end)
            continue;
        if ((endp = index(line, '\n')) != 0)
            *endp = 0;
        Sprintf(rumor_buf, "  %6s %s", "", xcrypt(line, xbuf));
        putstr(tmpwin, 0, rumor_buf);

        (void) dlb_fclose(rumors);
        display_nhwindow(tmpwin, TRUE);
        destroy_nhwindow(tmpwin);
    } else {
        couldnt_open_file(RUMORFILE);
        g.true_rumor_size = -1; /* don't try to open it again */
    }
}

/* Gets a random line of text from file 'fname', and returns it.
   rng is the random number generator to use, and should act like rn2 does. */
char *
get_rnd_text(fname, buf, rng)
const char *fname;
char *buf;
int FDECL((*rng), (int));
{
    dlb *fh;

    buf[0] = '\0';
    fh = dlb_fopen(fname, "r");
    if (fh) {
        /* TODO: cache sizetxt, starttxt, endtxt. maybe cache file contents? */
        long sizetxt = 0L, starttxt = 0L, endtxt = 0L, tidbit = 0L;
        char *endp, line[BUFSZ], xbuf[BUFSZ];

        /* skip "don't edit" comment */
        (void) dlb_fgets(line, sizeof line, fh);

        (void) dlb_fseek(fh, 0L, SEEK_CUR);
        starttxt = dlb_ftell(fh);
        (void) dlb_fseek(fh, 0L, SEEK_END);
        endtxt = dlb_ftell(fh);
        sizetxt = endtxt - starttxt;
        /* might be zero (only if file is empty); should complain in that
           case but if could happen over and over, also the suggestion
           that save and restore might fix the problem wouldn't be useful */
        if (sizetxt < 1L)
            return buf;
        tidbit = (*rng)(sizetxt);

        (void) dlb_fseek(fh, starttxt + tidbit, SEEK_SET);
        (void) dlb_fgets(line, sizeof line, fh);
        if (!dlb_fgets(line, sizeof line, fh)) {
            (void) dlb_fseek(fh, starttxt, SEEK_SET);
            (void) dlb_fgets(line, sizeof line, fh);
        }
        if ((endp = index(line, '\n')) != 0)
            *endp = 0;
        Strcat(buf, xcrypt(line, xbuf));
        (void) dlb_fclose(fh);
    } else {
        couldnt_open_file(fname);
    }

    return buf;
}

void
outrumor(truth, mechanism)
int truth; /* 1=true, -1=false, 0=either */
int mechanism;
{
    static const char fortune_msg[] =
        "This cookie has a scrap of paper inside.";
    const char *line;
    char buf[BUFSZ];
    boolean reading = (mechanism == BY_COOKIE || mechanism == BY_PAPER);

    if (reading) {
        /* deal with various things that prevent reading */
        if (is_fainted() && mechanism == BY_COOKIE)
            return;
        else if (Blind) {
            if (mechanism == BY_COOKIE)
                pline(fortune_msg);
            pline("What a pity that you cannot read it!");
            return;
        }
    }
    line = getrumor(truth, buf, reading ? FALSE : TRUE);
    if (!*line)
        line = "NetHack rumors file closed for renovation.";
    switch (mechanism) {
    case BY_ORACLE:
        /* Oracle delivers the rumor */
        pline("True to her word, the Oracle %ssays: ",
              (!rn2(4) ? "offhandedly "
                       : (!rn2(3) ? "casually "
                                  : (rn2(2) ? "nonchalantly " : ""))));
        verbalize1(line);
        /* [WIS exercized by getrumor()] */
        return;
    case BY_COOKIE:
        pline(fortune_msg);
    /* FALLTHRU */
    case BY_PAPER:
        pline("It reads:");
        break;
    }
    pline1(line);
}

static void
init_oracles(fp)
dlb *fp;
{
    register int i;
    char line[BUFSZ];
    int cnt = 0;

    /* this assumes we're only called once */
    (void) dlb_fgets(line, sizeof line, fp); /* skip "don't edit" comment*/
    (void) dlb_fgets(line, sizeof line, fp);
    if (sscanf(line, "%5d\n", &cnt) == 1 && cnt > 0) {
        g.oracle_cnt = (unsigned) cnt;
        g.oracle_loc = (unsigned long *) alloc((unsigned) cnt * sizeof(long));
        for (i = 0; i < cnt; i++) {
            (void) dlb_fgets(line, sizeof line, fp);
            (void) sscanf(line, "%5lx\n", &g.oracle_loc[i]);
        }
    }
    return;
}

void
save_oracles(nhfp)
NHFILE *nhfp;
{
    if (perform_bwrite(nhfp)) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t) &g.oracle_cnt,
                       sizeof g.oracle_cnt);
            if (g.oracle_cnt) {
                if (nhfp->structlevel) {
                    bwrite(nhfp->fd, (genericptr_t) g.oracle_loc,
                           g.oracle_cnt * sizeof (long));
                }
            }
    }
    if (release_data(nhfp)) {
        if (g.oracle_cnt) {
            free((genericptr_t) g.oracle_loc);
            g.oracle_loc = 0, g.oracle_cnt = 0, g.oracle_flg = 0;
        }
    }
}

void
restore_oracles(nhfp)
NHFILE *nhfp;
{
    if (nhfp->structlevel)
        mread(nhfp->fd, (genericptr_t) &g.oracle_cnt, sizeof g.oracle_cnt);

    if (g.oracle_cnt) {
        g.oracle_loc = (unsigned long *) alloc(g.oracle_cnt * sizeof(long));
        if (nhfp->structlevel) {
            mread(nhfp->fd, (genericptr_t) g.oracle_loc,
                  g.oracle_cnt * sizeof (long));
        }
        g.oracle_flg = 1; /* no need to call init_oracles() */
    }
}

void
outoracle(special, delphi)
boolean special;
boolean delphi;
{
    winid tmpwin;
    dlb *oracles;
    int oracle_idx;
    char *endp, line[COLNO], xbuf[BUFSZ];

    /* early return if we couldn't open ORACLEFILE on previous attempt,
       or if all the oracularities are already exhausted */
    if (g.oracle_flg < 0 || (g.oracle_flg > 0 && g.oracle_cnt == 0))
        return;

    oracles = dlb_fopen(ORACLEFILE, "r");

    if (oracles) {
        if (g.oracle_flg == 0) { /* if this is the first outoracle() */
            init_oracles(oracles);
            g.oracle_flg = 1;
            if (g.oracle_cnt == 0)
                goto close_oracles;
        }
        /* oracle_loc[0] is the special oracle;
           oracle_loc[1..oracle_cnt-1] are normal ones */
        if (g.oracle_cnt <= 1 && !special)
            goto close_oracles; /*(shouldn't happen)*/
        oracle_idx = special ? 0 : rnd((int) g.oracle_cnt - 1);
        (void) dlb_fseek(oracles, (long) g.oracle_loc[oracle_idx], SEEK_SET);
        if (!special) /* move offset of very last one into this slot */
            g.oracle_loc[oracle_idx] = g.oracle_loc[--g.oracle_cnt];

        tmpwin = create_nhwindow(NHW_TEXT);
        if (delphi)
            putstr(tmpwin, 0,
                   special
                     ? "The Oracle scornfully takes all your gold and says:"
                     : "The Oracle meditates for a moment and then intones:");
        else
            putstr(tmpwin, 0, "The message reads:");
        putstr(tmpwin, 0, "");

        while (dlb_fgets(line, COLNO, oracles) && strcmp(line, "---\n")) {
            if ((endp = index(line, '\n')) != 0)
                *endp = 0;
            putstr(tmpwin, 0, xcrypt(line, xbuf));
        }
        display_nhwindow(tmpwin, TRUE);
        destroy_nhwindow(tmpwin);
 close_oracles:
        (void) dlb_fclose(oracles);
    } else {
        couldnt_open_file(ORACLEFILE);
        g.oracle_flg = -1; /* don't try to open it again */
    }
}

int
doconsult(oracl)
struct monst *oracl;
{
    long umoney;
    int u_pay, minor_cost = 50, major_cost = 500 + 50 * u.ulevel;
    int add_xpts;
    char qbuf[QBUFSZ];

    g.multi = 0;
    umoney = money_cnt(g.invent);

    if (!oracl) {
        There("is no one here to consult.");
        return 0;
    } else if (!oracl->mpeaceful) {
        pline("%s is in no mood for consultations.", Monnam(oracl));
        return 0;
    } else if (!umoney) {
        You("have no gold.");
        return 0;
    }

    Sprintf(qbuf, "\"Wilt thou settle for a minor consultation?\" (%d %s)",
            minor_cost, currency((long) minor_cost));
    switch (ynq(qbuf)) {
    default:
    case 'q':
        return 0;
    case 'y':
        if (umoney < (long) minor_cost) {
            You("don't even have enough gold for that!");
            return 0;
        }
        u_pay = minor_cost;
        break;
    case 'n':
        if (umoney <= (long) minor_cost /* don't even ask */
            || (g.oracle_cnt == 1 || g.oracle_flg < 0))
            return 0;
        Sprintf(qbuf, "\"Then dost thou desire a major one?\" (%d %s)",
                major_cost, currency((long) major_cost));
        if (yn(qbuf) != 'y')
            return 0;
        u_pay = (umoney < (long) major_cost) ? (int) umoney : major_cost;
        break;
    }
    money2mon(oracl, (long) u_pay);
    g.context.botl = 1;
    if (!u.uevent.major_oracle && !u.uevent.minor_oracle)
        record_achievement(ACH_ORCL);
    add_xpts = 0; /* first oracle of each type gives experience points */
    if (u_pay == minor_cost) {
        outrumor(1, BY_ORACLE);
        if (!u.uevent.minor_oracle)
            add_xpts = u_pay / (u.uevent.major_oracle ? 25 : 10);
        /* 5 pts if very 1st, or 2 pts if major already done */
        u.uevent.minor_oracle = TRUE;
    } else {
        boolean cheapskate = u_pay < major_cost;

        outoracle(cheapskate, TRUE);
        if (!cheapskate && !u.uevent.major_oracle)
            add_xpts = u_pay / (u.uevent.minor_oracle ? 25 : 10);
        /* ~100 pts if very 1st, ~40 pts if minor already done */
        u.uevent.major_oracle = TRUE;
        exercise(A_WIS, !cheapskate);
    }
    if (add_xpts) {
        more_experienced(add_xpts, u_pay / 50);
        newexplevel();
    }
    return 1;
}

static void
couldnt_open_file(filename)
const char *filename;
{
    int save_something = g.program_state.something_worth_saving;

    /* most likely the file is missing, so suppress impossible()'s
       "saving and restoring might fix this" (unless the fuzzer,
       which escalates impossible to panic, is running) */
    if (!iflags.debug_fuzzer)
        g.program_state.something_worth_saving = 0;

    impossible("Can't open '%s' file.", filename);
    g.program_state.something_worth_saving = save_something;
}

/*rumors.c*/
