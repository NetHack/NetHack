/*	SCCS Id: @(#)mail.c	3.4	2002/01/13	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifdef MAIL
#include "mail.h"

/*
 * Notify user when new mail has arrived.  Idea by Merlyn Leroy.
 *
 * The mail daemon can move with less than usual restraint.  It can:
 *	- move diagonally from a door
 *	- use secret and closed doors
 *	- run through a monster ("Gangway!", etc.)
 *	- run over pools & traps
 *
 * Possible extensions:
 *	- Open the file MAIL and do fstat instead of stat for efficiency.
 *	  (But sh uses stat, so this cannot be too bad.)
 *	- Examine the mail and produce a scroll of mail named "From somebody".
 *	- Invoke MAILREADER in such a way that only this single letter is read.
 *	- Do something to the text when the scroll is enchanted or cancelled.
 *	- Make the daemon always appear at a stairwell, and have it find a
 *	  path to the hero.
 *
 * Note by Olaf Seibert: On the Amiga, we usually don't get mail.  So we go
 *			 through most of the effects at 'random' moments.
 * Note by Paul Winner:  The MSDOS port also 'fakes' the mail daemon at
 *			 random intervals.
 */

STATIC_DCL boolean FDECL(md_start,(coord *));
STATIC_DCL boolean FDECL(md_stop,(coord *, coord *));
STATIC_DCL boolean FDECL(md_rush,(struct monst *,int,int));
STATIC_DCL void FDECL(newmail, (struct mail_info *));

extern char *viz_rmin, *viz_rmax;	/* line-of-sight limits (vision.c) */

#ifdef OVL0

# if !defined(UNIX) && !defined(VMS) && !defined(LAN_MAIL)
int mustgetmail = -1;
# endif

#endif /* OVL0 */
#ifdef OVLB

# ifdef UNIX
#include <sys/stat.h>
#include <pwd.h>
/* DON'T trust all Unices to declare getpwuid() in <pwd.h> */
#  if !defined(_BULL_SOURCE) && !defined(__sgi) && !defined(_M_UNIX)
#   if !defined(SUNOS4) && !(defined(ULTRIX) && defined(__GNUC__))
/* DO trust all SVR4 to typedef uid_t in <sys/types.h> (probably to a long) */
#    if defined(POSIX_TYPES) || defined(SVR4) || defined(HPUX)
extern struct passwd *FDECL(getpwuid,(uid_t));
#    else
extern struct passwd *FDECL(getpwuid,(int));
#    endif
#   endif
#  endif
static struct stat omstat,nmstat;
static char *mailbox = (char *)0;
static long laststattime;

# if !defined(MAILPATH) && defined(AMS)	/* Just a placeholder for AMS */
#  define MAILPATH "/dev/null"
# endif
# if !defined(MAILPATH) && (defined(LINUX) || defined(__osf__))
#  define MAILPATH "/var/spool/mail/"
# endif
# if !defined(MAILPATH) && defined(__FreeBSD__)
#  define MAILPATH "/var/mail/"
# endif
# if !defined(MAILPATH) && (defined(BSD) || defined(ULTRIX))
#  define MAILPATH "/usr/spool/mail/"
# endif
# if !defined(MAILPATH) && (defined(SYSV) || defined(HPUX))
#  define MAILPATH "/usr/mail/"
# endif

void
getmailstatus()
{
	if(!mailbox && !(mailbox = nh_getenv("MAIL"))) {
#  ifdef MAILPATH
#   ifdef AMS
	        struct passwd ppasswd;

		(void) memcpy(&ppasswd, getpwuid(getuid()), sizeof(struct passwd));
		if (ppasswd.pw_dir) {
		     mailbox = (char *) alloc((unsigned) strlen(ppasswd.pw_dir)+sizeof(AMS_MAILBOX));
		     Strcpy(mailbox, ppasswd.pw_dir);
		     Strcat(mailbox, AMS_MAILBOX);
		} else
		  return;
#   else
		const char *pw_name = getpwuid(getuid())->pw_name;
		mailbox = (char *) alloc(sizeof(MAILPATH)+strlen(pw_name));
		Strcpy(mailbox, MAILPATH);
		Strcat(mailbox, pw_name);
#  endif /* AMS */
#  else
		return;
#  endif
	}
	if(stat(mailbox, &omstat)){
#  ifdef PERMANENT_MAILBOX
		pline("Cannot get status of MAIL=\"%s\".", mailbox);
		mailbox = 0;
#  else
		omstat.st_mtime = 0;
#  endif
	}
}
# endif /* UNIX */

#endif /* OVLB */
#ifdef OVL0

/*
 * Pick coordinates for a starting position for the mail daemon.  Called
 * from newmail() and newphone().
 */
STATIC_OVL boolean
md_start(startp)
    coord *startp;
{
    coord testcc;	/* scratch coordinates */
    int row;		/* current row we are checking */
    int lax;		/* if TRUE, pick a position in sight. */
    int dd;		/* distance to current point */
    int max_distance;	/* max distance found so far */

    /*
     * If blind and not telepathic, then it doesn't matter what we pick ---
     * the hero is not going to see it anyway.  So pick a nearby position.
     */
    if (Blind && !Blind_telepat) {
	if (!enexto(startp, u.ux, u.uy, (struct permonst *) 0))
	    return FALSE;	/* no good posiitons */
	return TRUE;
    }

    /*
     * Arrive at an up or down stairwell if it is in line of sight from the
     * hero.
     */
    if (couldsee(upstair.sx, upstair.sy)) {
	startp->x = upstair.sx;
	startp->y = upstair.sy;
	return TRUE;
    }
    if (couldsee(dnstair.sx, dnstair.sy)) {
	startp->x = dnstair.sx;
	startp->y = dnstair.sy;
	return TRUE;
    }

    /*
     * Try to pick a location out of sight next to the farthest position away
     * from the hero.  If this fails, try again, just picking the farthest
     * position that could be seen.  What we really ought to be doing is
     * finding a path from a stairwell...
     *
     * The arrays viz_rmin[] and viz_rmax[] are set even when blind.  These
     * are the LOS limits for each row.
     */
    lax = 0;	/* be picky */
    max_distance = -1;
retry:
    for (row = 0; row < ROWNO; row++) {
	if (viz_rmin[row] < viz_rmax[row]) {
	    /* There are valid positions on this row. */
	    dd = distu(viz_rmin[row],row);
	    if (dd > max_distance) {
		if (lax) {
		    max_distance = dd;
		    startp->y = row;
		    startp->x = viz_rmin[row];
		
		} else if (enexto(&testcc, (xchar)viz_rmin[row], row,
						(struct permonst *) 0) &&
			   !cansee(testcc.x, testcc.y) &&
			   couldsee(testcc.x, testcc.y)) {
		    max_distance = dd;
		    *startp = testcc;
		}
	    }
	    dd = distu(viz_rmax[row],row);
	    if (dd > max_distance) {
		if (lax) {
		    max_distance = dd;
		    startp->y = row;
		    startp->x = viz_rmax[row];
		
		} else if (enexto(&testcc, (xchar)viz_rmax[row], row,
						(struct permonst *) 0) &&
			   !cansee(testcc.x,testcc.y) &&
			   couldsee(testcc.x, testcc.y)) {

		    max_distance = dd;
		    *startp = testcc;
		}
	    }
	}
    }

    if (max_distance < 0) {
	if (!lax) {
	    lax = 1;		/* just find a position */
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
STATIC_OVL boolean
md_stop(stopp, startp)
    coord *stopp;	/* stopping position (we fill it in) */
    coord *startp;	/* starting positon (read only) */
{
    int x, y, distance, min_distance = -1;

    for (x = u.ux-1; x <= u.ux+1; x++)
	for (y = u.uy-1; y <= u.uy+1; y++) {
	    if (!isok(x, y) || (x == u.ux && y == u.uy)) continue;

	    if (ACCESSIBLE(levl[x][y].typ) && !MON_AT(x,y)) {
		distance = dist2(x,y,startp->x,startp->y);
		if (min_distance < 0 || distance < min_distance ||
			(distance == min_distance && rn2(2))) {
		    stopp->x = x;
		    stopp->y = y;
		    min_distance = distance;
		}
	    }
	}

    /* If we didn't find a good spot, try enexto(). */
    if (min_distance < 0 &&
		!enexto(stopp, u.ux, u.uy, &mons[PM_MAIL_DAEMON]))
	return FALSE;

    return TRUE;
}

/* Let the mail daemon have a larger vocabulary. */
static NEARDATA const char *mail_text[] = {
    "Gangway!",
    "Look out!",
    "Pardon me!"
};
#define md_exclamations()	(mail_text[rn2(3)])

/*
 * Make the mail daemon run through the dungeon.  The daemon will run over
 * any monsters that are in its path, but will replace them later.  Return
 * FALSE if the md gets stuck in a position where there is a monster.  Return
 * TRUE otherwise.
 */
STATIC_OVL boolean
md_rush(md,tx,ty)
    struct monst *md;
    register int tx, ty;		/* destination of mail daemon */
{
    struct monst *mon;			/* displaced monster */
    register int dx, dy;		/* direction counters */
    int fx = md->mx, fy = md->my;	/* current location */
    int nfx = fx, nfy = fy,		/* new location */
	d1, d2;				/* shortest distances */

    /*
     * It is possible that the monster at (fx,fy) is not the md when:
     * the md rushed the hero and failed, and is now starting back.
     */
    if (m_at(fx, fy) == md) {
	remove_monster(fx, fy);		/* pick up from orig position */
	newsym(fx, fy);
    }

    /*
     * At the beginning and exit of this loop, md is not placed in the
     * dungeon.
     */
    while (1) {
	/* Find a good location next to (fx,fy) closest to (tx,ty). */
	d1 = dist2(fx,fy,tx,ty);
	for (dx = -1; dx <= 1; dx++) for(dy = -1; dy <= 1; dy++)
	    if ((dx || dy) && isok(fx+dx,fy+dy) &&
				       !IS_STWALL(levl[fx+dx][fy+dy].typ)) {
		d2 = dist2(fx+dx,fy+dy,tx,ty);
		if (d2 < d1) {
		    d1 = d2;
		    nfx = fx+dx;
		    nfy = fy+dy;
		}
	    }

	/* Break if the md couldn't find a new position. */
	if (nfx == fx && nfy == fy) break;

	fx = nfx;			/* this is our new position */
	fy = nfy;

	/* Break if the md reaches its destination. */
	if (fx == tx && fy == ty) break;

	if ((mon = m_at(fx,fy)) != 0)	/* save monster at this position */
	    verbalize(md_exclamations());
	else if (fx == u.ux && fy == u.uy)
	    verbalize("Excuse me.");

	place_monster(md,fx,fy);	/* put md down */
	newsym(fx,fy);			/* see it */
	flush_screen(0);		/* make sure md shows up */
	delay_output();			/* wait a little bit */

	/* Remove md from the dungeon.  Restore original mon, if necessary. */
	if (mon) {
	    if ((mon->mx != fx) || (mon->my != fy))
		place_worm_seg(mon, fx, fy);
	    else
		place_monster(mon, fx, fy);
	} else
	    remove_monster(fx, fy);
	newsym(fx,fy);
    }

    /*
     * Check for a monster at our stopping position (this is possible, but
     * very unlikely).  If one exists, then have the md leave in disgust.
     */
    if ((mon = m_at(fx, fy)) != 0) {
	place_monster(md, fx, fy);	/* display md with text below */
	newsym(fx, fy);
	verbalize("This place's too crowded.  I'm outta here.");

	if ((mon->mx != fx) || (mon->my != fy))	/* put mon back */
	    place_worm_seg(mon, fx, fy);
	else
	    place_monster(mon, fx, fy);

	newsym(fx, fy);
	return FALSE;
    }

    place_monster(md, fx, fy);	/* place at final spot */
    newsym(fx, fy);
    flush_screen(0);
    delay_output();			/* wait a little bit */

    return TRUE;
}

/* Deliver a scroll of mail. */
/*ARGSUSED*/
STATIC_OVL void
newmail(info)
struct mail_info *info;
{
    struct monst *md;
    coord start, stop;
    boolean message_seen = FALSE;

    /* Try to find good starting and stopping places. */
    if (!md_start(&start) || !md_stop(&stop,&start)) goto give_up;

    /* Make the daemon.  Have it rush towards the hero. */
    if (!(md = makemon(&mons[PM_MAIL_DAEMON], start.x, start.y, NO_MM_FLAGS)))
	 goto give_up;
    if (!md_rush(md, stop.x, stop.y)) goto go_back;

    message_seen = TRUE;
    verbalize("%s, %s!  %s.", Hello(md), plname, info->display_txt);

    if (info->message_typ) {
	struct obj *obj = mksobj(SCR_MAIL, FALSE, FALSE);
	if (distu(md->mx,md->my) > 2)
	    verbalize("Catch!");
	display_nhwindow(WIN_MESSAGE, FALSE);
	if (info->object_nam) {
	    obj = oname(obj, info->object_nam);
	    if (info->response_cmd) {	/*(hide extension of the obj name)*/
		int namelth = info->response_cmd - info->object_nam - 1;
		if ( namelth <= 0 || namelth >= (int) obj->onamelth )
		    impossible("mail delivery screwed up");
		else
		    *(ONAME(obj) + namelth) = '\0';
		/* Note: renaming object will discard the hidden command. */
	    }
	}
	obj = hold_another_object(obj, "Oops!",
				  (const char *)0, (const char *)0);
    }

    /* zip back to starting location */
go_back:
    (void) md_rush(md, start.x, start.y);
    mongone(md);
    /* deliver some classes of messages even if no daemon ever shows up */
give_up:
    if (!message_seen && info->message_typ == MSG_OTHER)
	pline("Hark!  \"%s.\"", info->display_txt);
}

# if !defined(UNIX) && !defined(VMS) && !defined(LAN_MAIL)

void
ckmailstatus()
{
	if (u.uswallow || !flags.biff) return;
	if (mustgetmail < 0) {
#if defined(AMIGA) || defined(MSDOS) || defined(TOS)
	    mustgetmail=(moves<2000)?(100+rn2(2000)):(2000+rn2(3000));
#endif
	    return;
	}
	if (--mustgetmail <= 0) {
		static struct mail_info
			deliver = {MSG_MAIL,"I have some mail for you",0,0};
		newmail(&deliver);
		mustgetmail = -1;
	}
}

/*ARGSUSED*/
void
readmail(otmp)
struct obj *otmp;
{
    static char *junk[] = {
    "Please disregard previous letter.",
    "Welcome to NetHack.",
#ifdef AMIGA
    "Only Amiga makes it possible.",
    "CATS have all the answers.",
#endif
    "Report bugs to <devteam@nethack.org>.",
    "Invitation: Visit the NetHack web site at http://www.nethack.org!"
    };

    if (Blind) {
	pline("Unfortunately you cannot see what it says.");
    } else
	pline("It reads:  \"%s\"", junk[rn2(SIZE(junk))]);

}

# endif /* !UNIX && !VMS && !LAN_MAIL */

# ifdef UNIX

void
ckmailstatus()
{
	if(!mailbox || u.uswallow || !flags.biff
#  ifdef MAILCKFREQ
		    || moves < laststattime + MAILCKFREQ
#  endif
							)
		return;

	laststattime = moves;
	if(stat(mailbox, &nmstat)){
#  ifdef PERMANENT_MAILBOX
		pline("Cannot get status of MAIL=\"%s\" anymore.", mailbox);
		mailbox = 0;
#  else
		nmstat.st_mtime = 0;
#  endif
	} else if(nmstat.st_mtime > omstat.st_mtime) {
		if (nmstat.st_size) {
		    static struct mail_info deliver = {
#  ifndef NO_MAILREADER
			MSG_MAIL, "I have some mail for you",
#  else
			/* suppress creation and delivery of scroll of mail */
			MSG_OTHER, "You have some mail in the outside world",
#  endif
			0, 0
		    };
		    newmail(&deliver);
		}
		getmailstatus();	/* might be too late ... */
	}
}

/*ARGSUSED*/
void
readmail(otmp)
struct obj *otmp;
{
#  ifdef DEF_MAILREADER			/* This implies that UNIX is defined */
	register const char *mr = 0;

	display_nhwindow(WIN_MESSAGE, FALSE);
	if(!(mr = nh_getenv("MAILREADER")))
		mr = DEF_MAILREADER;

	if(child(1)){
		(void) execl(mr, mr, (char *)0);
		terminate(EXIT_FAILURE);
	}
#  else
#   ifndef AMS				/* AMS mailboxes are directories */
	display_file(mailbox, TRUE);
#   endif /* AMS */
#  endif /* DEF_MAILREADER */

	/* get new stat; not entirely correct: there is a small time
	   window where we do not see new mail */
	getmailstatus();
}

# endif /* UNIX */

# ifdef VMS

extern NDECL(struct mail_info *parse_next_broadcast);

volatile int broadcasts = 0;

void
ckmailstatus()
{
    struct mail_info *brdcst;

    if (u.uswallow || !flags.biff) return;

    while (broadcasts > 0) {	/* process all trapped broadcasts [until] */
	broadcasts--;
	if ((brdcst = parse_next_broadcast()) != 0) {
	    newmail(brdcst);
	    break;		/* only handle one real message at a time */
	}
    }
}

void
readmail(otmp)
struct obj *otmp;
{
#  ifdef SHELL	/* can't access mail reader without spawning subprocess */
    const char *txt, *cmd;
    char *p, buf[BUFSZ], qbuf[BUFSZ];
    int len;

    /* there should be a command hidden beyond the object name */
    txt = otmp->onamelth ? ONAME(otmp) : "";
    len = strlen(txt);
    cmd = (len + 1 < otmp->onamelth) ? txt + len + 1 : (char *) 0;
    if (!cmd || !*cmd) cmd = "SPAWN";

    Sprintf(qbuf, "System command (%s)", cmd);
    getlin(qbuf, buf);
    if (*buf != '\033') {
	for (p = eos(buf); p > buf; *p = '\0')
	    if (*--p != ' ') break;	/* strip trailing spaces */
	if (*buf) cmd = buf;		/* use user entered command */
	if (!strcmpi(cmd, "SPAWN") || !strcmp(cmd, "!"))
	    cmd = (char *) 0;		/* interactive escape */

	vms_doshell(cmd, TRUE);
	(void) sleep(1);
    }
#  endif /* SHELL */
}

# endif /* VMS */

# ifdef LAN_MAIL

void
ckmailstatus()
{
	static int laststattime = 0;
	
	if(u.uswallow || !flags.biff
#  ifdef MAILCKFREQ
		    || moves < laststattime + MAILCKFREQ
#  endif
							)
		return;

	laststattime = moves;
	if (lan_mail_check()) {
		    static struct mail_info deliver = {
#  ifndef NO_MAILREADER
			MSG_MAIL, "I have some mail for you",
#  else
			/* suppress creation and delivery of scroll of mail */
			MSG_OTHER, "You have some mail in the outside world",
#  endif
			0, 0
		    };
		    newmail(&deliver);
	}
}

/*ARGSUSED*/
void
readmail(otmp)
struct obj *otmp;
{
	lan_mail_read(otmp);
}

# endif /* LAN_MAIL */

#endif /* OVL0 */

#endif /* MAIL */

/*mail.c*/
