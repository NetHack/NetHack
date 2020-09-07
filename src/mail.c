/* NetHack 3.7	mail.c	$NHDT-Date: 1596498174 2020/08/03 23:42:54 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.47 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Pasi Kallinen, 2018. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifdef MAIL
#ifdef SIMPLE_MAIL
# include <fcntl.h>
# include <errno.h>
#endif /* SIMPLE_MAIL */
#endif /* MAIL */
#ifdef MAIL_STRUCTURES
#include "mail.h"
#endif

#ifdef MAIL
/*
 * Notify user when new mail has arrived.  Idea by Merlyn Leroy.
 *
 * The mail daemon can move with less than usual restraint.  It can:
 *      - move diagonally from a door
 *      - use secret and closed doors
 *      - run through a monster ("Gangway!", etc.)
 *      - run over pools & traps
 *
 * Possible extensions:
 *      - Open the file MAIL and do fstat instead of stat for efficiency.
 *        (But sh uses stat, so this cannot be too bad.)
 *      - Examine the mail and produce a scroll of mail named "From somebody".
 *      - Invoke MAILREADER in such a way that only this single mail is read.
 *      - Do something to the text when the scroll is enchanted or cancelled.
 *      - Make the daemon always appear at a stairwell, and have it find a
 *        path to the hero.
 *
 * Note by Olaf Seibert: On the Amiga, we usually don't get mail.  So we go
 *                       through most of the effects at 'random' moments.
 * Note by Paul Winner:  The MSDOS port also 'fakes' the mail daemon at
 *                       random intervals.
 */

static boolean FDECL(md_start, (coord *));
static boolean FDECL(md_stop, (coord *, coord *));
static boolean FDECL(md_rush, (struct monst *, int, int));
static void FDECL(newmail, (struct mail_info *));

#if !defined(UNIX) && !defined(VMS)
int mustgetmail = -1;
#endif

#ifdef UNIX
#include <sys/stat.h>
#include <pwd.h>
/* DON'T trust all Unices to declare getpwuid() in <pwd.h> */
#if !defined(_BULL_SOURCE) && !defined(__sgi) && !defined(_M_UNIX)
#if !defined(SUNOS4) && !(defined(ULTRIX) && defined(__GNUC__))
/* DO trust all SVR4 to typedef uid_t in <sys/types.h> (probably to a long) */
#if defined(POSIX_TYPES) || defined(SVR4) || defined(HPUX)
extern struct passwd *FDECL(getpwuid, (uid_t));
#else
extern struct passwd *FDECL(getpwuid, (int));
#endif
#endif
#endif
static struct stat omstat, nmstat;
static char *mailbox = (char *) 0;
static long laststattime;

#if !defined(MAILPATH) && defined(AMS) /* Just a placeholder for AMS */
#define MAILPATH "/dev/null"
#endif
#if !defined(MAILPATH) && (defined(LINUX) || defined(__osf__))
#define MAILPATH "/var/spool/mail/"
#endif
#if !defined(MAILPATH) && defined(__FreeBSD__)
#define MAILPATH "/var/mail/"
#endif
#if !defined(MAILPATH) && (defined(BSD) || defined(ULTRIX))
#define MAILPATH "/usr/spool/mail/"
#endif
#if !defined(MAILPATH) && (defined(SYSV) || defined(HPUX))
#define MAILPATH "/usr/mail/"
#endif

void
free_maildata()
{
    if (mailbox)
        free((genericptr_t) mailbox), mailbox = (char *) 0;
}

void
getmailstatus()
{
    if (mailbox) {
        ; /* no need to repeat the setup */
    } else if ((mailbox = nh_getenv("MAIL")) != 0) {
        mailbox = dupstr(mailbox);
#ifdef MAILPATH
    } else  {
#ifdef AMS
        struct passwd ppasswd;

        (void) memcpy(&ppasswd, getpwuid(getuid()), sizeof (struct passwd));
        if (ppasswd.pw_dir) {
            /* note: 'sizeof "LITERAL"' includes +1 for terminating '\0' */
            mailbox = (char *) alloc((unsigned) (strlen(ppasswd.pw_dir)
                                                 + sizeof AMS_MAILBOX));
            Strcpy(mailbox, ppasswd.pw_dir);
            Strcat(mailbox, AMS_MAILBOX);
        }
#else
        const char *pw_name = getpwuid(getuid())->pw_name;

        /* note: 'sizeof "LITERAL"' includes +1 for terminating '\0' */
        mailbox = (char *) alloc((unsigned) (strlen(pw_name)
                                             + sizeof MAILPATH));
        Strcpy(mailbox, MAILPATH);
        Strcat(mailbox, pw_name);
#endif /* AMS */
#endif /* MAILPATH */
    }

    debugpline3("mailbox=%c%s%c",
                mailbox ? '\"' : '<',
                mailbox ? mailbox : "null",
                mailbox ? '\"' : '>');

    if (mailbox && stat(mailbox, &omstat)) {
#ifdef PERMANENT_MAILBOX
        pline("Cannot get status of MAIL=\"%s\".", mailbox);
        free_maildata(); /* set 'mailbox' to Null */
#else
        omstat.st_mtime = 0;
#endif
    }
}
#endif /* UNIX */

/*
 * Pick coordinates for a starting position for the mail daemon.  Called
 * from newmail() and newphone().
 */
static boolean
md_start(startp)
coord *startp;
{
    coord testcc;     /* scratch coordinates */
    int row;          /* current row we are checking */
    int lax;          /* if TRUE, pick a position in sight. */
    int dd;           /* distance to current point */
    int max_distance; /* max distance found so far */

    /*
     * If blind and not telepathic, then it doesn't matter what we pick ---
     * the hero is not going to see it anyway.  So pick a nearby position.
     */
    if (Blind && !Blind_telepat) {
        if (!enexto(startp, u.ux, u.uy, (struct permonst *) 0))
            return FALSE; /* no good positions */
        return TRUE;
    }

    /*
     * Arrive at an up or down stairwell if it is in line of sight from the
     * hero.
     */
    if (couldsee(g.upstair.sx, g.upstair.sy)) {
        startp->x = g.upstair.sx;
        startp->y = g.upstair.sy;
        return TRUE;
    }
    if (couldsee(g.dnstair.sx, g.dnstair.sy)) {
        startp->x = g.dnstair.sx;
        startp->y = g.dnstair.sy;
        return TRUE;
    }

    /*
     * Try to pick a location out of sight next to the farthest position away
     * from the hero.  If this fails, try again, just picking the farthest
     * position that could be seen.  What we really ought to be doing is
     * finding a path from a stairwell...
     *
     * The arrays g.viz_rmin[] and g.viz_rmax[] are set even when blind.  These
     * are the LOS limits for each row.
     */
    lax = 0; /* be picky */
    max_distance = -1;
 retry:
    for (row = 0; row < ROWNO; row++) {
        if (g.viz_rmin[row] < g.viz_rmax[row]) {
            /* There are valid positions on this row. */
            dd = distu(g.viz_rmin[row], row);
            if (dd > max_distance) {
                if (lax) {
                    max_distance = dd;
                    startp->y = row;
                    startp->x = g.viz_rmin[row];

                } else if (enexto(&testcc, (xchar) g.viz_rmin[row], row,
                                  (struct permonst *) 0)
                           && !cansee(testcc.x, testcc.y)
                           && couldsee(testcc.x, testcc.y)) {
                    max_distance = dd;
                    *startp = testcc;
                }
            }
            dd = distu(g.viz_rmax[row], row);
            if (dd > max_distance) {
                if (lax) {
                    max_distance = dd;
                    startp->y = row;
                    startp->x = g.viz_rmax[row];

                } else if (enexto(&testcc, (xchar) g.viz_rmax[row], row,
                                  (struct permonst *) 0)
                           && !cansee(testcc.x, testcc.y)
                           && couldsee(testcc.x, testcc.y)) {
                    max_distance = dd;
                    *startp = testcc;
                }
            }
        }
    }

    if (max_distance < 0) {
        if (!lax) {
            lax = 1; /* just find a position */
            goto retry;
        }
        return FALSE;
    }

    return TRUE;
}

/*
 * Try to choose a stopping point as near as possible to the starting
 * position while still adjacent to the hero.  If all else fails, try
 * enexto().  Use enexto() as a last resort because enexto() chooses
 * its point randomly, which is not what we want.
 */
static boolean
md_stop(stopp, startp)
coord *stopp;  /* stopping position (we fill it in) */
coord *startp; /* starting position (read only) */
{
    int x, y, distance, min_distance = -1;

    for (x = u.ux - 1; x <= u.ux + 1; x++)
        for (y = u.uy - 1; y <= u.uy + 1; y++) {
            if (!isok(x, y) || (x == u.ux && y == u.uy))
                continue;

            if (accessible(x, y) && !MON_AT(x, y)) {
                distance = dist2(x, y, startp->x, startp->y);
                if (min_distance < 0 || distance < min_distance
                    || (distance == min_distance && rn2(2))) {
                    stopp->x = x;
                    stopp->y = y;
                    min_distance = distance;
                }
            }
        }

    /* If we didn't find a good spot, try enexto(). */
    if (min_distance < 0 && !enexto(stopp, u.ux, u.uy, &mons[PM_MAIL_DAEMON]))
        return FALSE;

    return TRUE;
}

/* Let the mail daemon have a larger vocabulary. */
static NEARDATA const char *mail_text[] = { "Gangway!", "Look out!",
                                            "Pardon me!" };
#define md_exclamations() (mail_text[rn2(3)])

/*
 * Make the mail daemon run through the dungeon.  The daemon will run over
 * any monsters that are in its path, but will replace them later.  Return
 * FALSE if the md gets stuck in a position where there is a monster.  Return
 * TRUE otherwise.
 */
static boolean
md_rush(md, tx, ty)
struct monst *md;
register int tx, ty; /* destination of mail daemon */
{
    struct monst *mon;            /* displaced monster */
    register int dx, dy;          /* direction counters */
    int fx = md->mx, fy = md->my; /* current location */
    int nfx = fx, nfy = fy,       /* new location */
        d1, d2;                   /* shortest distances */

    /*
     * It is possible that the monster at (fx,fy) is not the md when:
     * the md rushed the hero and failed, and is now starting back.
     */
    if (m_at(fx, fy) == md) {
        remove_monster(fx, fy); /* pick up from orig position */
        newsym(fx, fy);
    }

    /*
     * At the beginning and exit of this loop, md is not placed in the
     * dungeon.
     */
    while (1) {
        /* Find a good location next to (fx,fy) closest to (tx,ty). */
        d1 = dist2(fx, fy, tx, ty);
        for (dx = -1; dx <= 1; dx++)
            for (dy = -1; dy <= 1; dy++)
                if ((dx || dy) && isok(fx + dx, fy + dy)
                    && !IS_STWALL(levl[fx + dx][fy + dy].typ)) {
                    d2 = dist2(fx + dx, fy + dy, tx, ty);
                    if (d2 < d1) {
                        d1 = d2;
                        nfx = fx + dx;
                        nfy = fy + dy;
                    }
                }

        /* Break if the md couldn't find a new position. */
        if (nfx == fx && nfy == fy)
            break;

        fx = nfx; /* this is our new position */
        fy = nfy;

        /* Break if the md reaches its destination. */
        if (fx == tx && fy == ty)
            break;

        if ((mon = m_at(fx, fy)) != 0) /* save monster at this position */
            verbalize1(md_exclamations());
        else if (fx == u.ux && fy == u.uy)
            verbalize("Excuse me.");

        if (mon)
            remove_monster(fx, fy);
        place_monster(md, fx, fy); /* put md down */
        newsym(fx, fy);            /* see it */
        flush_screen(0);           /* make sure md shows up */
        delay_output();            /* wait a little bit */

        /* Remove md from the dungeon.  Restore original mon, if necessary. */
        remove_monster(fx, fy);
        if (mon) {
            if ((mon->mx != fx) || (mon->my != fy))
                place_worm_seg(mon, fx, fy);
            else
                place_monster(mon, fx, fy);
        }
        newsym(fx, fy);
    }

    /*
     * Check for a monster at our stopping position (this is possible, but
     * very unlikely).  If one exists, then have the md leave in disgust.
     */
    if ((mon = m_at(fx, fy)) != 0) {
        remove_monster(fx, fy);
        place_monster(md, fx, fy); /* display md with text below */
        newsym(fx, fy);
        verbalize("This place's too crowded.  I'm outta here.");
        remove_monster(fx, fy);

        if ((mon->mx != fx) || (mon->my != fy)) /* put mon back */
            place_worm_seg(mon, fx, fy);
        else
            place_monster(mon, fx, fy);

        newsym(fx, fy);
        return FALSE;
    }

    place_monster(md, fx, fy); /* place at final spot */
    newsym(fx, fy);
    flush_screen(0);
    delay_output(); /* wait a little bit */

    return TRUE;
}

/* Deliver a scroll of mail. */
/*ARGSUSED*/
static void
newmail(info)
struct mail_info *info;
{
    struct monst *md;
    coord start, stop;
    boolean message_seen = FALSE;

    /* Try to find good starting and stopping places. */
    if (!md_start(&start) || !md_stop(&stop, &start))
        goto give_up;

    /* Make the daemon.  Have it rush towards the hero. */
    if (!(md = makemon(&mons[PM_MAIL_DAEMON], start.x, start.y, NO_MM_FLAGS)))
        goto give_up;
    if (!md_rush(md, stop.x, stop.y))
        goto go_back;

    message_seen = TRUE;
    verbalize("%s, %s!  %s.", Hello(md), g.plname, info->display_txt);

    if (info->message_typ) {
        struct obj *obj = mksobj(SCR_MAIL, FALSE, FALSE);

        if (info->object_nam)
            obj = oname(obj, info->object_nam);
        if (info->response_cmd)
            new_omailcmd(obj, info->response_cmd);

        if (distu(md->mx, md->my) > 2)
            verbalize("Catch!");
        display_nhwindow(WIN_MESSAGE, FALSE);
        obj = hold_another_object(obj, "Oops!", (const char *) 0,
                                  (const char *) 0);
        nhUse(obj);
    }

 go_back:
    /* zip back to starting location */
    if (!md_rush(md, start.x, start.y))
        md->mx = md->my = 0; /* for mongone, md is not on map */
    mongone(md);

 give_up:
    /* deliver some classes of messages even if no daemon ever shows up */
    if (!message_seen && info->message_typ == MSG_OTHER)
        pline("Hark!  \"%s.\"", info->display_txt);
}

#if !defined(UNIX) && !defined(VMS)

void
ckmailstatus()
{
    if (u.uswallow || !flags.biff)
        return;
    if (mustgetmail < 0) {
#if defined(AMIGA) || defined(MSDOS) || defined(TOS)
        mustgetmail = (g.moves < 2000) ? (100 + rn2(2000)) : (2000 + rn2(3000));
#endif
        return;
    }
    if (--mustgetmail <= 0) {
        static struct mail_info deliver = {
            MSG_MAIL, "I have some mail for you", 0, 0
        };
        newmail(&deliver);
        mustgetmail = -1;
    }
}

/*ARGSUSED*/
void
readmail(otmp)
struct obj *otmp UNUSED;
{
    static const char *junk[] = {
        "Report bugs to <%s>.", /*** must be first entry ***/
        "Please disregard previous letter.",
        "Welcome to NetHack.",
#ifdef AMIGA
        "Only Amiga makes it possible.",
        "CATS have all the answers.",
#endif
        "This mail complies with the Yendorian Anti-Spam Act (YASA)",
        "Please find enclosed a small token to represent your Owlbear",
        "**FR33 P0T10N 0F FULL H34L1NG**",
        "Please return to sender (Asmodeus)",
        /* when enclosed by "It reads:  \"...\"", this is too long
           for an ordinary 80-column display so wraps to a second line
           (suboptimal but works correctly);
           dollar sign and fractional zorkmids are inappropriate within
           nethack but are suitable for typical dysfunctional spam mail */
     "Buy a potion of gain level for only $19.99!  Guaranteed to be blessed!",
        /* DEVTEAM_URL will be substituted for "%s"; terminating punctuation
           (formerly "!") has deliberately been omitted so that it can't be
           mistaken for part of the URL (unfortunately that is still followed
           by a closing quote--in the pline below, not the data here) */
        "Invitation: Visit the NetHack web site at %s"
    };

    /* XXX replace with more general substitution code and add local
     * contact message.
     *
     * FIXME:  this allocated memory is never freed.  However, if the
     * game is restarted, the junk[] update will be a no-op for second
     * and subsequent runs and this updated text will still be appropriate.
     */
    if (index(junk[0], '%')) {
        char *tmp;
        int i;

        for (i = 0; i < SIZE(junk); ++i) {
            if (index(junk[i], '%')) {
                if (i == 0) {
                    /* +2 from '%s' in junk[0] suffices as substitute
                       for usual +1 for terminator */
                    tmp = (char *) alloc(strlen(junk[0])
                                         + strlen(DEVTEAM_EMAIL));
                    Sprintf(tmp, junk[0], DEVTEAM_EMAIL);
                    junk[0] = tmp;
                } else if (strstri(junk[i], "web site")) {
                    /* as with junk[0], room for terminator is present */
                    tmp = (char *) alloc(strlen(junk[i])
                                         + strlen(DEVTEAM_URL));
                    Sprintf(tmp, junk[i], DEVTEAM_URL);
                    junk[i] = tmp;
                } else {
                    /* could check for "%%" but unless that becomes needed,
                       handling it is more complicated than necessary */
                    impossible("fake mail #%d has undefined substitution", i);
                    junk[i] = "Bad fake mail...";
                }
            }
        }
    }
    if (Blind) {
        pline("Unfortunately you cannot see what it says.");
    } else
        pline("It reads:  \"%s\"", junk[rn2(SIZE(junk))]);
}

#endif /* !UNIX && !VMS */

#ifdef UNIX

void
ckmailstatus()
{
    ck_server_admin_msg();

    if (!mailbox || u.uswallow || !flags.biff
#ifdef MAILCKFREQ
        || g.moves < laststattime + MAILCKFREQ
#endif
        )
        return;

    laststattime = g.moves;
    if (stat(mailbox, &nmstat)) {
#ifdef PERMANENT_MAILBOX
        pline("Cannot get status of MAIL=\"%s\" anymore.", mailbox);
        free_maildata();
#else
        nmstat.st_mtime = 0;
#endif
    } else if (nmstat.st_mtime > omstat.st_mtime) {
        if (nmstat.st_size) {
            static struct mail_info deliver = {
#ifndef NO_MAILREADER
                MSG_MAIL, "I have some mail for you",
#else
                /* suppress creation and delivery of scroll of mail */
                MSG_OTHER, "You have some mail in the outside world",
#endif
                0, 0
            };
            newmail(&deliver);
        }
        getmailstatus(); /* might be too late ... */
    }
}

#if defined(SIMPLE_MAIL) || defined(SERVER_ADMIN_MSG)
void
read_simplemail(mbox, adminmsg)
char *mbox;
boolean adminmsg;
{
    FILE* mb = fopen(mbox, "r");
    char curline[128], *msg;
    boolean seen_one_already = FALSE;
#ifdef SIMPLE_MAIL
    struct flock fl = { 0 };
#endif
    const char *msgfrom = adminmsg
        ? "The voice of %s booms through the caverns:"
        : "This message is from '%s'.";

    if (!mb)
        goto bail;

#ifdef SIMPLE_MAIL
    fl.l_type = F_RDLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    errno = 0;
#endif

    /* Allow this call to block. */
    if (!adminmsg
#ifdef SIMPLE_MAIL
        && fcntl (fileno (mb), F_SETLKW, &fl) == -1
#endif
        )
        goto bail;

    while (fgets(curline, 128, mb) != NULL) {
        if (!adminmsg) {
#ifdef SIMPLE_MAIL
            fl.l_type = F_UNLCK;
            fcntl (fileno(mb), F_UNLCK, &fl);
#endif
            pline("There is a%s message on this scroll.",
                  seen_one_already ? "nother" : "");
        }
        msg = strchr(curline, ':');

        if (!msg)
            goto bail;

        *msg = '\0';
        msg++;
        msg[strlen(msg) - 1] = '\0'; /* kill newline */

        pline(msgfrom, curline);
        if (adminmsg)
            verbalize(msg);
        else
            pline("It reads: \"%s\".", msg);

        seen_one_already = TRUE;
#ifdef SIMPLE_MAIL
        errno = 0;
        if (!adminmsg) {
            fl.l_type = F_RDLCK;
            fcntl(fileno(mb), F_SETLKW, &fl);
        }
#endif
    }

#ifdef SIMPLE_MAIL
    if (!adminmsg) {
        fl.l_type = F_UNLCK;
        fcntl(fileno(mb), F_UNLCK, &fl);
    }
#endif
    fclose(mb);
    if (adminmsg)
        display_nhwindow(WIN_MESSAGE, TRUE);
    else
        unlink(mailbox);
    return;
 bail:
    /* bail out _professionally_ */
    if (!adminmsg)
        pline("It appears to be all gibberish.");
}
#endif /* SIMPLE_MAIL */

void
ck_server_admin_msg()
{
#ifdef SERVER_ADMIN_MSG
    static struct stat ost,nst;
    static long lastchk = 0;

    if (g.moves < lastchk + SERVER_ADMIN_MSG_CKFREQ) return;
    lastchk = g.moves;

    if (!stat(SERVER_ADMIN_MSG, &nst)) {
        if (nst.st_mtime > ost.st_mtime)
            read_simplemail(SERVER_ADMIN_MSG, TRUE);
        ost.st_mtime = nst.st_mtime;
    }
#endif /* SERVER_ADMIN_MSG */
}

/*ARGSUSED*/
void
readmail(otmp)
struct obj *otmp UNUSED;
{
#ifdef DEF_MAILREADER /* This implies that UNIX is defined */
    register const char *mr = 0;
#endif /* DEF_MAILREADER */
#ifdef SIMPLE_MAIL
    read_simplemail(mailbox, FALSE);
    return;
#endif /* SIMPLE_MAIL */
#ifdef DEF_MAILREADER /* This implies that UNIX is defined */
    if (iflags.debug_fuzzer)
        return;
    display_nhwindow(WIN_MESSAGE, FALSE);
    if (!(mr = nh_getenv("MAILREADER")))
        mr = DEF_MAILREADER;

    if (child(1)) {
        (void) execl(mr, mr, (char *) 0);
        nh_terminate(EXIT_FAILURE);
    }
#else
#ifndef AMS /* AMS mailboxes are directories */
    display_file(mailbox, TRUE);
#endif /* AMS */
#endif /* DEF_MAILREADER */

    /* get new stat; not entirely correct: there is a small time
       window where we do not see new mail */
    getmailstatus();
}

#endif /* UNIX */

#ifdef VMS

extern NDECL(struct mail_info *parse_next_broadcast);

volatile int broadcasts = 0;

void
ckmailstatus()
{
    struct mail_info *brdcst;

    if (iflags.debug_fuzzer)
        return;
    if (u.uswallow || !flags.biff)
        return;

    while (broadcasts > 0) { /* process all trapped broadcasts [until] */
        broadcasts--;
        if ((brdcst = parse_next_broadcast()) != 0) {
            newmail(brdcst);
            break; /* only handle one real message at a time */
        }
    }
}

void
readmail(otmp)
struct obj *otmp;
{
#ifdef SHELL /* can't access mail reader without spawning subprocess */
    const char *txt, *cmd;
    char *p, buf[BUFSZ] = DUMMY, qbuf[BUFSZ];
    int len;

    /* there should be a command in OMAILCMD */
    if (has_oname(otmp))
        txt = ONAME(otmp);
    else
        txt = "";
    len = strlen(txt);
    if (has_omailcmd(otmp))
        cmd = OMAILCMD(otmp);
    if (!cmd || !*cmd)
        cmd = "SPAWN";

    Sprintf(qbuf, "System command (%s)", cmd);
    getlin(qbuf, buf);
    if (*buf != '\033') {
        for (p = eos(buf); p > buf; *p = '\0')
            if (*--p != ' ')
                break; /* strip trailing spaces */
        if (*buf)
            cmd = buf; /* use user entered command */
        if (!strcmpi(cmd, "SPAWN") || !strcmp(cmd, "!"))
            cmd = (char *) 0; /* interactive escape */

        vms_doshell(cmd, TRUE);
        (void) sleep(1);
    }
#else
    nhUse(otmp);
#endif /* SHELL */
}

#endif /* VMS */
#endif /* MAIL */

/*mail.c*/
