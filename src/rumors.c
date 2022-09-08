/* NetHack 3.7	rumors.c	$NHDT-Date: 1594370241 2020/07/10 08:37:21 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.56 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "dlb.h"

/*      [Note:  this comment is fairly old, but still accurate for 3.1;
 *       it's no longer accurate for 3.7 but may still be of interest.]
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

static void unpadline(char *);
static void init_rumors(dlb *);
static char *get_rnd_line(dlb *, char *, unsigned, int (*)(int),
                          long, long, unsigned);
static void init_oracles(dlb *);
static void others_check(const char *ftype, const char *, winid *);
static void couldnt_open_file(const char *);
static void init_CapMons(void);

/* used by CapitalMon(); set up by init_CapMons(), released by free_CapMons();
   there's no need for these to be put into 'struct instance_globals g' */
static unsigned CapMonstCnt = 0, CapBogonCnt = 0,
                CapMonSiz = 0; /* CapMonstCnt+CapBogonCnt+1 when non-zero */
static const char **CapMons = 0;

/* list of bogusmons prefixes used to indicate special monster type such as
   unique or always a particular gender; see dat/bogusmon.txt */
extern const char bogon_codes[]; /* from do_name.c */

/* makedefs pads short rumors, epitaphs, engravings, and hallucinatory
   monster names with trailing underscores; strip those off */
static void
unpadline(char *line)
{
    char *p = eos(line);

    /* remove newline if still present; caller should have stripped it */
    if (p > line && p[-1] == '\n')
        --p;

    /* remove padding */
    while (p > line && p[-1] == '_')
        --p;

    *p = '\0';
}

DISABLE_WARNING_FORMAT_NONLITERAL

static void
init_rumors(dlb *fp)
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

RESTORE_WARNING_FORMAT_NONLITERAL

/* exclude_cookie is a hack used because we sometimes want to get rumors in a
 * context where messages such as "You swallowed the fortune!" that refer to
 * cookies should not appear.  This has no effect for true rumors since none
 * of them contain such references anyway.
 */
char *
getrumor(
    int truth, /* 1=true, -1=false, 0=either */
    char *rumor_buf,
    boolean exclude_cookie)
{
    dlb *rumors;
    long beginning, ending;
    char line[BUFSZ];

    rumor_buf[0] = '\0';
    if (g.true_rumor_size < 0L) /* a previous try failed to open RUMORFILE */
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
                ending = g.true_rumor_end;
                break;
            case 0: /* once here, 0 => false rather than "either"*/
            case -1:
                beginning = (long) g.false_rumor_start;
                ending = g.false_rumor_end;
                break;
            default:
                impossible("strange truth value for rumor");
                return strcpy(rumor_buf, "Oops...");
            }
            Strcpy(rumor_buf,
                   get_rnd_line(rumors, line, (unsigned) sizeof line, rn2,
                                beginning, ending, MD_PAD_RUMORS));
        } while (count++ < 50 && (exclude_cookie
                                  && (strstri(rumor_buf, "fortune")
                                      || strstri(rumor_buf, "pity"))));
        (void) dlb_fclose(rumors);
        if (count >= 50)
            impossible("Can't find non-cookie rumor?");
        else if (!g.in_mklev) /* avoid exercizing wisdom for graffiti */
            exercise(A_WIS, (adjtruth > 0));
    } else {
        couldnt_open_file(RUMORFILE);
        g.true_rumor_size = -1; /* don't try to open it again */
    }
    return rumor_buf;
}

/* test that the true/false rumor boundaries are valid and show the first
   two and very last epitaphs, engravings, and bogus monsters */
void
rumor_check(void)
{
    dlb *rumors;
    winid tmpwin = WIN_ERR;
    char *endp, line[BUFSZ], xbuf[BUFSZ], rumor_buf[BUFSZ];

    rumors = (g.true_rumor_size >= 0) ? dlb_fopen(RUMORFILE, "r") : 0;
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

    /* if a previous attempt couldn't open file or rejected its contents,
       we didn't bother trying again this time */
    } else if (g.true_rumor_size < 0L) {
 no_rumors: /* file could be opened but init_rumors() didn't like it */
        pline("rumors not accessible.");
        /* engravings, epitaphs, and bogus monsters will still be shown,
           and in tmpwin rather than via additional pline() calls */
        display_nhwindow(WIN_MESSAGE, TRUE); /* --more-- */

    /* first attempt to open file has just failed */
    } else {
        couldnt_open_file(RUMORFILE);
        g.true_rumor_size = -1; /* don't try to open it again */
    }

    /* initial implementation of default epitaph/engraving/bogusmon
       contained an error; check those along with rumors */
    others_check("Engravings:", ENGRAVEFILE, &tmpwin);
    others_check("Epitaphs:", EPITAPHFILE, &tmpwin);
    others_check("Bogus monsters:", BOGUSMONFILE, &tmpwin);

    if (tmpwin != WIN_ERR) {
        display_nhwindow(tmpwin, TRUE);
        destroy_nhwindow(tmpwin);
    }
}

DISABLE_WARNING_FORMAT_NONLITERAL

/* 3.7: augments rumors_check(); test 'engrave' or 'epitaph' or 'bogusmon' */
static void
others_check(
    const char *ftype, /* header: "{Engravings|Epitaphs|Bogus monsters}:" */
    const char *fname, /* filename: {ENGRAVEFILE|EPITAPHFILE|BOGUSMONFILE} */
    winid *winptr)     /* text window for output; created here if necessary */
{
    static const char errfmt[] = "others_check(\"%s\"): %s";
    dlb *fh;
    char line[BUFSZ], xbuf[BUFSZ], *endp;
    winid tmpwin = *winptr;
    int entrycount = 0;

    fh = dlb_fopen(fname, "r");
    if (fh) {
        if (tmpwin == WIN_ERR) {
            *winptr = tmpwin = create_nhwindow(NHW_TEXT);
            if (tmpwin == WIN_ERR) {
                /* should panic, but won't for wizard mode check operation */
                impossible(errfmt, fname, "can't create temporary window");
                goto closeit;
            }
        }
        putstr(tmpwin, 0, "");
        putstr(tmpwin, 0, ftype);
        /* "don't edit" comment */
        *line = '\0';
        if (!dlb_fgets(line, sizeof line, fh)) {
            Sprintf(xbuf, errfmt, fname, "error; can't read comment line");
            putstr(tmpwin, 0, xbuf);
            goto closeit;
        }
        if (*line != '#') {
            Sprintf(xbuf, errfmt, fname,
                    "malformed; first line is not a comment line:");
            putstr(tmpwin, 0, xbuf);
            /* show the bad line; we don't know whether it has been
               encrypted via xcrypt() so show it both ways */
            if ((endp = index(line, '\n')) != 0)
                *endp = 0;
            putstr(tmpwin, 0, "- first line, as is");
            putstr(tmpwin, 0, line);
            putstr(tmpwin, 0, "- xcrypt of first line");
            putstr(tmpwin, 0, xcrypt(line, xbuf));
            goto closeit;
        }
        /* first line; should be default one inserted by makedefs when
           building the file but we don't have the expected value so
           can only require a line to exist */
        *line = '\0';
        if (!dlb_fgets(line, sizeof line, fh) || *line == '\n') {
            Sprintf(xbuf, errfmt, fname,
                    !*line ? "can't read first non-comment line"
                           : "first non-comment line is empty");
            putstr(tmpwin, 0, xbuf);
            goto closeit;
        }
        ++entrycount;
        if ((endp = index(line, '\n')) != 0)
            *endp = 0;
        putstr(tmpwin, 0, xcrypt(line, xbuf));
        if (!dlb_fgets(line, sizeof line, fh)) {
            putstr(tmpwin, 0, "(no second entry)");
        } else {
            ++entrycount;
            if ((endp = index(line, '\n')) != 0)
                *endp = 0;
            putstr(tmpwin, 0, xcrypt(line, xbuf));
            while (dlb_fgets(line, sizeof line, fh)) {
                ++entrycount;
                if ((endp = index(line, '\n')) != 0)
                    *endp = 0;
                (void) xcrypt(line, xbuf);
            }
            /* count will be 2 if the default entry and the first ordinary
               entry are the only ones present (if either of those were
               missing, we wouldn't have gotten here...) */
            if (entrycount == 2) {
                putstr(tmpwin, 0, "(only two entries)");
            } else {
                /* showing an elipsis avoids ambiguity about whether
                   there are other lines; doing so three times (once for
                   each file) results in total output being 24 lines,
                   forcing a --More-- prompt if using a 24 line screen;
                   displaying 23 lines and --More-- followed by second
                   page with 1 line doesn't look very good but isn't
                   incorrect, and taller screens where that won't be an
                   issue are more common than 24 line terminals nowadays */
                if (entrycount > 3)
                    putstr(tmpwin, 0, " ...");
                putstr(tmpwin, 0, xbuf); /* already decrypted */
            }
        }

 closeit:
        (void) dlb_fclose(fh);
    } else {
        /* since this comes out via impossible(), it won't be integrated
           with the text window of values, but it shouldn't ever happen
           so we won't waste effort integrating it */
        couldnt_open_file(fname);
    }
}

RESTORE_WARNING_FORMAT_NONLITERAL

/* load one randomly chosen line from a section of a file; undoes
   decryption and strips trailing underscore padding and final newline;
   if padlength is non-zero, every line is expected to be at least that
   long and every line in the file will have an equal chance of being
   chosen; however, if padlength is 0, lines following long lines are
   more likely than average to be picked, and lines after short lines
   are less likely */
static char *
get_rnd_line(
    dlb *fh,            /* already opened file */
    char *buf,          /* output buffer */
    unsigned bufsiz,    /* (unsigned) sizeof buf */
    int (*rng)(int),    /* random number routine; rn2(N) or similar, 0..N-1 */
    long startpos,      /* location in file of first line of interest */
    long endpos,        /* location one byte past last line of interest;
                         * if 0, end-of-file will be used */
    unsigned padlength) /* expected line length; 0 if no expectations */
{
    char *newl, *xbufp, xbuf[BUFSZ];
    long filechunksize, chunkoffset;
    int trylimit;

    *buf = '\0';
    if (!endpos) {
        (void) dlb_fseek(fh, 0L, SEEK_END);
        endpos = dlb_ftell(fh);
    }
    filechunksize = endpos - startpos;

    /* might be zero (only if file is empty); should complain in that
       case but it could happen over and over, also the suggestion
       that save and restore might fix the problem wouldn't be useful */
    if (filechunksize < 1L)
        return buf;
    /* 'rumors' is about 3/4 of the way to the limit on a 16-bit config
       for the whole, roughly 3/8 of the way for either half; all active
       configuations these days are at least 32-bits anyway */
    nhassert(filechunksize <= INT_MAX); /* essential for rn2() */

    /*
     * Position randomly which will probably be in the middle of a line.
     * (Occasionally by chance it will happen to be at the very start of
     * a line, but we'll have no way of knowing that so have to behave
     * as if it were positioned in the middle.)
     * Read the rest of that line, then use the next one.  If there's no
     * next line (ie, end of file), go back to beginning and use first.
     *
     * When short lines have been padded to length N, only accept long
     * lines if we land within last N+1 characters (+1 is for newline
     * which hasn't been stripped away yet), effectively shortening
     * them to normal length.  That yields even selection distribution.
     */
    for (trylimit = 10; trylimit > 0; --trylimit) {
        chunkoffset = (long) (*rng)((int) filechunksize);
        (void) dlb_fseek(fh, startpos + chunkoffset, SEEK_SET);
        (void) dlb_fgets(buf, bufsiz, fh);
        /* if padlength is 0, accept any position; when non-zero,
           padlength does not count the newline but strlen(buf) does */
        if (!padlength || (unsigned) strlen(buf) <= padlength + 1)
            break;
    }
    /* use next line; for rumors, caller takes care of whether startpos
       and endpos cover just true rumors or just false rumors; reaching
       endpos is equivalent to end-of-file in order to avoid using the
       first false rumor if fseek for a true one lands within the last one */
    if (dlb_ftell(fh) >= endpos || !dlb_fgets(buf, bufsiz, fh)) {
        /* assume failure is due to end-of-file; go back to start */
        (void) dlb_fseek(fh, startpos, SEEK_SET);
        (void) dlb_fgets(buf, bufsiz, fh);
    }
    if ((newl = index(buf, '\n')) != 0)
        *newl = '\0';
    /* decrypt line; make sure that our intermediate buffer is big enough */
    xbufp = (strlen(buf) <= sizeof xbuf - 1) ? &xbuf[0]
            : (char *) alloc((unsigned) strlen(buf) + 1);
    Strcpy(buf, xcrypt(buf, xbufp));
    if (xbufp != &xbuf[0])
        free((genericptr_t) xbufp);
    /* strip padding that makedefs adds to short lines */
    if (padlength)
        unpadline(buf);
    return buf;
}

/* Gets a random line of text from file 'fname', and returns it.
   rng is the random number generator to use, and should act like rn2 does. */
char *
get_rnd_text(
    const char *fname,
    char *buf,
    int (*rng)(int),
    unsigned padlength)
{
    dlb *fh = dlb_fopen(fname, "r");

    buf[0] = '\0';
    if (fh) {
        long starttxt = 0L;
        char line[BUFSZ];

        /* skip "don't edit" comment */
        (void) dlb_fgets(line, sizeof line, fh);
        /* obtain current file position */
        (void) dlb_fseek(fh, 0L, SEEK_CUR);
        starttxt = dlb_ftell(fh);

        /* get a randomly chosen line; it comes back decrypted and unpadded */
        Strcpy(buf, get_rnd_line(fh, line, (unsigned) sizeof line, rng,
                                 starttxt, 0L, padlength));
        (void) dlb_fclose(fh);
    } else {
        couldnt_open_file(fname);
    }
    return buf;
}

void
outrumor(
    int truth, /* 1=true, -1=false, 0=either */
    int mechanism)
{
    static const char fortune_msg[] =
        "This cookie has a scrap of paper inside.";
    const char *line;
    char buf[BUFSZ];
    boolean reading = (mechanism == BY_COOKIE || mechanism == BY_PAPER);

    if (reading) {
        /* deal with various things that prevent reading */
        if (is_fainted() && mechanism == BY_COOKIE) {
            return;
        } else if (Blind) {
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
init_oracles(dlb *fp)
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
save_oracles(NHFILE *nhfp)
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
restore_oracles(NHFILE *nhfp)
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
outoracle(boolean special, boolean delphi)
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
doconsult(struct monst *oracl)
{
    long umoney;
    int u_pay, minor_cost = 50, major_cost = 500 + 50 * u.ulevel;
    int add_xpts;
    char qbuf[QBUFSZ];

    g.multi = 0;
    umoney = money_cnt(g.invent);

    if (!oracl) {
        There("is no one here to consult.");
        return ECMD_OK;
    } else if (!oracl->mpeaceful) {
        pline("%s is in no mood for consultations.", Monnam(oracl));
        return ECMD_OK;
    } else if (!umoney) {
        You("have no gold.");
        return ECMD_OK;
    }

    Sprintf(qbuf, "\"Wilt thou settle for a minor consultation?\" (%d %s)",
            minor_cost, currency((long) minor_cost));
    switch (ynq(qbuf)) {
    default:
    case 'q':
        return ECMD_OK;
    case 'y':
        if (umoney < (long) minor_cost) {
            You("don't even have enough gold for that!");
            return ECMD_OK;
        }
        u_pay = minor_cost;
        break;
    case 'n':
        if (umoney <= (long) minor_cost /* don't even ask */
            || (g.oracle_cnt == 1 || g.oracle_flg < 0))
            return ECMD_OK;
        Sprintf(qbuf, "\"Then dost thou desire a major one?\" (%d %s)",
                major_cost, currency((long) major_cost));
        if (yn(qbuf) != 'y')
            return ECMD_OK;
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
    return ECMD_TIME;
}

static void
couldnt_open_file(const char *filename)
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

/* is 'word' a capitalized monster name that should be preceded by "the"?
   (non-unique monster like Mordor Orc, or capitalized title like Norn
   rather than a name); used by the() on a string without any context;
   this sets up a list of names rather than scan all of mons[] every time
   the decision is needed (resulting list currently contains 27 monster
   entries and 20 hallucination entries) */
boolean
CapitalMon(
    const char *word) /* potential monster name; a name might be followed by
                       * something like " corpse" */
{
    const char *nam;
    unsigned i, wln, nln;

    if (!word || !*word || *word == lowc(*word))
        return FALSE; /* 'word' is not a capitalized monster name */

    if (!CapMons)
        init_CapMons();

    wln = (unsigned) strlen(word);
    for (i = 0; i < CapMonSiz - 1; ++i) {
        nam = CapMons[i];
        nln = (unsigned) strlen(nam);
        if (wln < nln)
            continue;
        /*
         * Unlike name_to_mon(), we don't need to find the longest match
         * or return the gender or a pointer to trailing stuff.  We do
         * check full words though: "Foo" matches "Foo" and "Foo bar" and
         * "Foo's bar" but not "Foobar".  We use case-sensitive matching.
         */
        if (!strncmp(nam, word, nln)
            && (!word[nln] || word[nln] == ' ' || word[nln] == '\''))
            return TRUE; /* 'word' is a capitalized monster name */
    }
    return FALSE;
}

/* one-time initialization of CapMons[], a list of non-unique monsters
   having a capitalized type name like Green-elf or Archon, plus unique
   monsters whose "name" is a title rather than a personal name, plus
   hallucinatory monster names that fall into either of those categories */
static void
init_CapMons(void)
{
    unsigned pass;
    dlb *bogonfile = dlb_fopen(BOGUSMONFILE, "r");

    if (CapMons) /* sanity precaution */
        free_CapMons();

    /* first pass: count the number of relevant monster names, then
       allocate memory for CapMons[]; second pass: populate CapMons[] */
    for (pass = 1; pass <= 2; ++pass) {
        struct permonst *mptr;
        const char *nam;
        unsigned mndx, mgend;

        /* the first CapMonstCnt entries come from mons[].pmnames[] and
           the next CapBogonCnt entries from from the 'bogusmons' file;
           there is an extra entry for Null at the end, but that is only
           useful to force non-zero array size in case both mons[] and
           bogusmons get modified to have no applicable monster names */
        CapMonstCnt = CapBogonCnt = 0;

        /* gather applicable actual monsters */
        for (mndx = LOW_PM; mndx < NUMMONS; ++mndx) {
            mptr = &mons[mndx];
            if ((mptr->geno & G_UNIQ) != 0 && !the_unique_pm(mptr))
                continue;
            for (mgend = MALE; mgend < NUM_MGENDERS; ++mgend) {
                nam = mptr->pmnames[mgend];
                if (nam && *nam != lowc(*nam)) {
                    if (pass == 2)
                        CapMons[CapMonstCnt] = nam;
                    ++CapMonstCnt;
                }
            }
        }

        /* now gather applicable hallucinatory monsters */
        if (bogonfile) {
            char hline[BUFSZ], xbuf[BUFSZ], *endp, *startp, code;

            /* rewind; effectively a no-op for pass 1; essential for pass 2 */
            (void) dlb_fseek(bogonfile, 0L, SEEK_SET);
            /* skip "don't edit" comment (first line of file) */
            (void) dlb_fgets(hline, sizeof hline, bogonfile);

            /* one monster name per line in rudimentary encrypted format;
               some are prefixed by a classification code to indicate
               gender and/or to distinguish an individual from a type
               (code is a single punctuation character when present) */
            while (dlb_fgets(hline, sizeof hline, bogonfile)) {
                if ((endp = index(hline, '\n')) != 0)
                    *endp = '\0'; /* strip newline */
                (void) xcrypt(hline, xbuf);
                unpadline(xbuf);

                if (!xbuf[0] || !index(bogon_codes, xbuf[0]))
                    code = '\0', startp = &xbuf[0]; /* ordinary */
                else
                    code = xbuf[0], startp = &xbuf[1]; /* special */

                if (*startp != lowc(*startp) && !bogon_is_pname(code)) {
                    if (pass == 2)
                        CapMons[CapMonstCnt + CapBogonCnt] = dupstr(startp);
                    ++CapBogonCnt;
                }
            }
        }

        /* finish the current pass */
        if (pass == 1) {
            CapMonSiz = CapMonstCnt + CapBogonCnt + 1; /* +1: terminator */
            CapMons = (const char **) alloc(CapMonSiz * sizeof *CapMons);
        } else { /* pass == 2 */
            /* terminator; not strictly needed */
            CapMons[CapMonSiz - 1] = (const char *) 0;

            if (bogonfile)
                (void) dlb_fclose(bogonfile), bogonfile = (dlb *) 0;
        }
    }
#ifdef DEBUG
    /*
     * CapMons[] init doesn't kick in until needed.  To force this name
     * dump, set DEBUGFILES to "CapMons" in your environment (or in
     * sysconf) prior to starting nethack, wish for a statue of an Archon
     * and drop it if held, then step away and apply a stethoscope towards
     * it to trigger a message that passes "Archon" to the() which will
     * then call CapitalMon() which in turn will call init_CapMons().
     */
    if (wizard && explicitdebug("CapMons")) {
        char buf[BUFSZ];
        unsigned i;
        winid tmpwin = create_nhwindow(NHW_TEXT);

        putstr(tmpwin, 0,
              "Capitalized monster type names normally preceded by \"the\":");
        for (i = 0; i < CapMonSiz - 1; ++i) {
            Sprintf(buf, "  %.77s", CapMons[i]);
            putstr(tmpwin, 0, buf);
        }
        display_nhwindow(tmpwin, TRUE);
        destroy_nhwindow(tmpwin);
    }
#endif
    return;
}

/* release memory allocated for the list of capitalized monster type names */
void
free_CapMons(void)
{
    /* note: some elements of CapMons[] are string literals from
       mons[].pmnames[] and should not be freed, others are dynamically
       allocated copies of hallucinatory monster names and should be freed */
    if (CapMons) {
        unsigned idx;

        /* skip 0..MonstCnt-1, free MonstCnt..(MonstCnt+BogonCnt-1) */
        for (idx = CapMonstCnt; idx < CapMonSiz - 1; ++idx)
            free((genericptr_t) CapMons[idx]); /* cast: discard 'const' */
        free((genericptr_t) CapMons), CapMons = (const char **) 0;
    }
    CapMonSiz = 0;
}

/*rumors.c*/
