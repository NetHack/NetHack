/*	SCCS Id: @(#)save.c	3.4	2002/01/19	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "lev.h"

#ifndef NO_SIGNAL
#include <signal.h>
#endif
#if !defined(LSC) && !defined(O_WRONLY) && !defined(AZTEC_C)
#include <fcntl.h>
#endif

#include "quest.h"

#ifdef MFLOPPY
long bytes_counted;
static int count_only;
#endif

#ifdef MICRO
int dotcnt, dotrow;	/* also used in restore */
#endif

#ifdef ZEROCOMP
STATIC_DCL void FDECL(bputc, (int));
#endif
STATIC_DCL void FDECL(savelevchn, (int,int));
STATIC_DCL void FDECL(savedamage, (int,int));
STATIC_DCL void FDECL(saveobjchn, (int,struct obj *,int));
STATIC_DCL void FDECL(savemonchn, (int,struct monst *,int));
STATIC_DCL void FDECL(savetrapchn, (int,struct trap *,int));
STATIC_DCL void FDECL(savegamestate, (int,int));
#ifdef MFLOPPY
STATIC_DCL void FDECL(savelev0, (int,XCHAR_P,int));
STATIC_DCL boolean NDECL(swapout_oldest);
STATIC_DCL void FDECL(copyfile, (char *,char *));
#endif /* MFLOPPY */
#ifdef GCC_WARN
static long nulls[10];
#else
#define nulls nul
#endif

#if defined(UNIX) || defined(VMS) || defined(__EMX__) || defined(WIN32)
#define HUP	if (!program_state.done_hup)
#else
#define HUP
#endif

/* need to preserve these during save to avoid accessing freed memory */
static unsigned ustuck_id = 0, usteed_id = 0;

int
dosave()
{
	clear_nhwindow(WIN_MESSAGE);
	if(yn("Really save?") == 'n') {
		clear_nhwindow(WIN_MESSAGE);
		if(multi > 0) nomul(0);
	} else {
		clear_nhwindow(WIN_MESSAGE);
		pline("Saving...");
#if defined(UNIX) || defined(VMS) || defined(__EMX__)
		program_state.done_hup = 0;
#endif
		if(dosave0()) {
			u.uhp = -1;		/* universal game's over indicator */
			/* make sure they see the Saving message */
			display_nhwindow(WIN_MESSAGE, TRUE);
			exit_nhwindows("Be seeing you...");
			terminate(EXIT_SUCCESS);
		} else (void)doredraw();
	}
	return 0;
}


#if defined(UNIX) || defined(VMS) || defined (__EMX__) || defined(WIN32)
/*ARGSUSED*/
void
hangup(sig_unused)  /* called as signal() handler, so sent at least one arg */
int sig_unused;
{
# ifdef NOSAVEONHANGUP
	(void) signal(SIGINT, SIG_IGN);
	clearlocks();
#  ifndef VMS
	terminate(EXIT_FAILURE);
#  endif
# else	/* SAVEONHANGUP */
	if (!program_state.done_hup++) {
	    if (program_state.something_worth_saving)
		(void) dosave0();
#  ifdef VMS
	    /* don't call exit when already within an exit handler;
	       that would cancel any other pending user-mode handlers */
	    if (!program_state.exiting)
#  endif
	    {
		clearlocks();
		terminate(EXIT_FAILURE);
	    }
	}
# endif
	return;
}
#endif

/* returns 1 if save successful */
int
dosave0()
{
	const char *fq_save;
	register int fd, ofd;
	xchar ltmp;
	d_level uz_save;

	if (!SAVEF[0])
		return 0;
	fq_save = fqname(SAVEF, SAVEPREFIX, 1);	/* level files take 0 */

#if defined(UNIX) || defined(VMS)
	(void) signal(SIGHUP, SIG_IGN);
#endif
#ifndef NO_SIGNAL
	(void) signal(SIGINT, SIG_IGN);
#endif

#if defined(MICRO) && defined(MFLOPPY)
	if (!saveDiskPrompt(0)) return 0;
#endif

	HUP if (iflags.window_inited) {
	    uncompress(fq_save);
	    fd = open_savefile();
	    if (fd > 0) {
		(void) close(fd);
		clear_nhwindow(WIN_MESSAGE);
		There("seems to be an old save file.");
		if (yn("Overwrite the old file?") == 'n') {
		    compress(fq_save);
		    return 0;
		}
	    }
	}

	HUP mark_synch();	/* flush any buffered screen output */

	fd = create_savefile();

	if(fd < 0) {
		HUP pline("Cannot open save file.");
		(void) delete_savefile();	/* ab@unido */
		return(0);
	}

	vision_recalc(2);	/* shut down vision to prevent problems
				   in the event of an impossible() call */
	
	/* undo date-dependent luck adjustments made at startup time */
	if(flags.moonphase == FULL_MOON)	/* ut-sally!fletcher */
		change_luck(-1);		/* and unido!ab */
	if(flags.friday13)
		change_luck(1);
	if(iflags.window_inited)
	    HUP clear_nhwindow(WIN_MESSAGE);

#ifdef MICRO
	dotcnt = 0;
	dotrow = 2;
	curs(WIN_MAP, 1, 1);
	if (strncmpi("X11", windowprocs.name, 3))
	  putstr(WIN_MAP, 0, "Saving:");
#endif
#ifdef MFLOPPY
	/* make sure there is enough disk space */
	if (iflags.checkspace) {
	    long fds, needed;

	    savelev(fd, ledger_no(&u.uz), COUNT_SAVE);
	    savegamestate(fd, COUNT_SAVE);
	    needed = bytes_counted;

	    for (ltmp = 1; ltmp <= maxledgerno(); ltmp++)
		if (ltmp != ledger_no(&u.uz) && level_info[ltmp].where)
		    needed += level_info[ltmp].size + (sizeof ltmp);
	    fds = freediskspace(fq_save);
	    if (needed > fds) {
		HUP {
		    There("is insufficient space on SAVE disk.");
		    pline("Require %ld bytes but only have %ld.", needed, fds);
		}
		flushout();
		(void) close(fd);
		(void) delete_savefile();
		return 0;
	    }

	    co_false();
	}
#endif /* MFLOPPY */

	store_version(fd);
	ustuck_id = (u.ustuck ? u.ustuck->m_id : 0);
#ifdef STEED
	usteed_id = (u.usteed ? u.usteed->m_id : 0);
#endif
	savelev(fd, ledger_no(&u.uz), WRITE_SAVE | FREE_SAVE);
	savegamestate(fd, WRITE_SAVE | FREE_SAVE);

	/* While copying level files around, zero out u.uz to keep
	 * parts of the restore code from completely initializing all
	 * in-core data structures, since all we're doing is copying.
	 * This also avoids at least one nasty core dump.
	 */
	uz_save = u.uz;
	u.uz.dnum = u.uz.dlevel = 0;
	/* these pointers are no longer valid, and at least u.usteed
	 * may mislead place_monster() on other levels
	 */
	u.ustuck = (struct monst *)0;
#ifdef STEED
	u.usteed = (struct monst *)0;
#endif

	for(ltmp = (xchar)1; ltmp <= maxledgerno(); ltmp++) {
		if (ltmp == ledger_no(&uz_save)) continue;
		if (!(level_info[ltmp].flags & LFILE_EXISTS)) continue;
#ifdef MICRO
		curs(WIN_MAP, 1 + dotcnt++, dotrow);
		if (dotcnt >= (COLNO - 1)) {
			dotrow++;
			dotcnt = 0;
		}
		if (strncmpi("X11", windowprocs.name, 3)){
		  putstr(WIN_MAP, 0, ".");
		}
		mark_synch();
#endif
		ofd = open_levelfile(ltmp);
		if (ofd < 0) {
		    HUP pline("Cannot read level %d.", ltmp);
		    (void) close(fd);
		    (void) delete_savefile();
		    HUP done(TRICKED);
		    return(0);
		}
		minit();	/* ZEROCOMP */
		getlev(ofd, hackpid, ltmp, FALSE);
		(void) close(ofd);
		bwrite(fd, (genericptr_t) &ltmp, sizeof ltmp); /* level number*/
		savelev(fd, ltmp, WRITE_SAVE | FREE_SAVE);     /* actual level*/
		delete_levelfile(ltmp);
	}
	bclose(fd);

	u.uz = uz_save;

	/* get rid of current level --jgm */
	delete_levelfile(ledger_no(&u.uz));
	delete_levelfile(0);
	compress(fq_save);
	return(1);
}

STATIC_OVL void
savegamestate(fd, mode)
register int fd, mode;
{
	int uid;

#ifdef MFLOPPY
	count_only = (mode & COUNT_SAVE);
#endif
	uid = getuid();
	bwrite(fd, (genericptr_t) &uid, sizeof uid);
	bwrite(fd, (genericptr_t) &flags, sizeof(struct flag));
	bwrite(fd, (genericptr_t) &u, sizeof(struct you));

	/* must come before migrating_objs and migrating_mons are freed */
	save_timers(fd, mode, RANGE_GLOBAL);
	save_light_sources(fd, mode, RANGE_GLOBAL);

	saveobjchn(fd, invent, mode);
	saveobjchn(fd, migrating_objs, mode);
	savemonchn(fd, migrating_mons, mode);
	if (release_data(mode)) {
	    invent = 0;
	    migrating_objs = 0;
	    migrating_mons = 0;
	}
	bwrite(fd, (genericptr_t) mvitals, sizeof(mvitals));

	save_dungeon(fd, (boolean)!!perform_bwrite(mode),
			 (boolean)!!release_data(mode));
	savelevchn(fd, mode);
	bwrite(fd, (genericptr_t) &moves, sizeof moves);
	bwrite(fd, (genericptr_t) &monstermoves, sizeof monstermoves);
	bwrite(fd, (genericptr_t) &quest_status, sizeof(struct q_score));
	bwrite(fd, (genericptr_t) spl_book,
				sizeof(struct spell) * (MAXSPELL + 1));
	save_artifacts(fd);
	save_oracles(fd, mode);
	if(ustuck_id)
	    bwrite(fd, (genericptr_t) &ustuck_id, sizeof ustuck_id);
#ifdef STEED
	if(usteed_id)
	    bwrite(fd, (genericptr_t) &usteed_id, sizeof usteed_id);
#endif
	bwrite(fd, (genericptr_t) pl_character, sizeof pl_character);
	bwrite(fd, (genericptr_t) pl_fruit, sizeof pl_fruit);
	bwrite(fd, (genericptr_t) &current_fruit, sizeof current_fruit);
	savefruitchn(fd, mode);
	savenames(fd, mode);
	save_waterlevel(fd, mode);
	bflush(fd);
}

#ifdef INSURANCE
void
savestateinlock()
{
	int fd, hpid;
	static boolean havestate = TRUE;

	/* When checkpointing is on, the full state needs to be written
	 * on each checkpoint.  When checkpointing is off, only the pid
	 * needs to be in the level.0 file, so it does not need to be
	 * constantly rewritten.  When checkpointing is turned off during
	 * a game, however, the file has to be rewritten once to truncate
	 * it and avoid restoring from outdated information.
	 *
	 * Restricting havestate to this routine means that an additional
	 * noop pid rewriting will take place on the first "checkpoint" after
	 * the game is started or restored, if checkpointing is off.
	 */
	if (flags.ins_chkpt || havestate) {
		/* save the rest of the current game state in the lock file,
		 * following the original int pid, the current level number,
		 * and the current savefile name, which should not be subject
		 * to any internal compression schemes since they must be
		 * readable by an external utility
		 */
		fd = open_levelfile(0);
		if (fd < 0) {
		    pline("Cannot open level 0.");
		    pline("Probably someone removed it.");
		    done(TRICKED);
		    return;
		}

		(void) read(fd, (genericptr_t) &hpid, sizeof(hpid));
		if (hackpid != hpid) {
		    pline("Level 0 pid bad!");
		    done(TRICKED);
		}
		(void) close(fd);

		fd = create_levelfile(0);
		if (fd < 0) {
		    pline("Cannot rewrite level 0.");
		    done(TRICKED);
		    return;
		}
		(void) write(fd, (genericptr_t) &hackpid, sizeof(hackpid));
		if (flags.ins_chkpt) {
		    int currlev = ledger_no(&u.uz);

		    (void) write(fd, (genericptr_t) &currlev, sizeof(currlev));
		    save_savefile_name(fd);
		    store_version(fd);
		    ustuck_id = (u.ustuck ? u.ustuck->m_id : 0);
#ifdef STEED
		    usteed_id = (u.usteed ? u.usteed->m_id : 0);
#endif
		    savegamestate(fd, WRITE_SAVE);
		}
		bclose(fd);
	}
	havestate = flags.ins_chkpt;
}
#endif

#ifdef MFLOPPY
boolean
savelev(fd, lev, mode)
int fd;
xchar lev;
int mode;
{
	if (mode & COUNT_SAVE) {
		bytes_counted = 0;
		savelev0(fd, lev, COUNT_SAVE);
		/* probably bytes_counted will be filled in again by an
		 * immediately following WRITE_SAVE anyway, but we'll
		 * leave it out of checkspace just in case */
		if (iflags.checkspace) {
			while (bytes_counted > freediskspace(levels))
				if (!swapout_oldest())
					return FALSE;
		}
	}
	if (mode & (WRITE_SAVE | FREE_SAVE)) {
		bytes_counted = 0;
		savelev0(fd, lev, mode);
	}
	if (mode != FREE_SAVE) {
		level_info[lev].where = ACTIVE;
		level_info[lev].time = moves;
		level_info[lev].size = bytes_counted;
	}
	return TRUE;
}

STATIC_OVL void
savelev0(fd,lev,mode)
#else
void
savelev(fd,lev,mode)
#endif
int fd;
xchar lev;
int mode;
{
#ifdef TOS
	short tlev;
#endif

	/* if we're tearing down the current level without saving anything
	   (which happens upon entrance to the endgame or after an aborted
	   restore attempt) then we don't want to do any actual I/O */
	if (mode == FREE_SAVE) goto skip_lots;
	if (iflags.purge_monsters) {
		/* purge any dead monsters (necessary if we're starting
		 * a panic save rather than a normal one, or sometimes
		 * when changing levels without taking time -- e.g.
		 * create statue trap then immediately level teleport) */
		dmonsfree();
	}

	if(fd < 0) panic("Save on bad file!");	/* impossible */
#ifdef MFLOPPY
	count_only = (mode & COUNT_SAVE);
#endif
	if (lev >= 0 && lev <= maxledgerno())
	    level_info[lev].flags |= VISITED;
	bwrite(fd,(genericptr_t) &hackpid,sizeof(hackpid));
#ifdef TOS
	tlev=lev; tlev &= 0x00ff;
	bwrite(fd,(genericptr_t) &tlev,sizeof(tlev));
#else
	bwrite(fd,(genericptr_t) &lev,sizeof(lev));
#endif
#ifdef RLECOMP
	{
	    /* perform run-length encoding of rm structs */
	    struct rm *prm, *rgrm;
	    int x, y;
	    uchar match;

	    rgrm = &levl[0][0];		/* start matching at first rm */
	    match = 0;

	    for (y = 0; y < ROWNO; y++) {
		for (x = 0; x < COLNO; x++) {
		    prm = &levl[x][y];
		    if (prm->glyph == rgrm->glyph
			&& prm->typ == rgrm->typ
			&& prm->seenv == rgrm->seenv
			&& prm->horizontal == rgrm->horizontal
			&& prm->flags == rgrm->flags
			&& prm->lit == rgrm->lit
			&& prm->waslit == rgrm->waslit
			&& prm->roomno == rgrm->roomno
			&& prm->edge == rgrm->edge) {
			match++;
			if (match > 254) {
			    match = 254;	/* undo this match */
			    goto writeout;
			}
		    } else {
			/* the run has been broken,
			 * write out run-length encoding */
		    writeout:
			bwrite(fd, (genericptr_t)&match, sizeof(uchar));
			bwrite(fd, (genericptr_t)rgrm, sizeof(struct rm));
			/* start encoding again. we have at least 1 rm
			 * in the next run, viz. this one. */
			match = 1;
			rgrm = prm;
		    }
		}
	    }
	    if (match > 0) {
		bwrite(fd, (genericptr_t)&match, sizeof(uchar));
		bwrite(fd, (genericptr_t)rgrm, sizeof(struct rm));
	    }
	}
#else
	bwrite(fd,(genericptr_t) levl,sizeof(levl));
#endif /* RLECOMP */

	bwrite(fd,(genericptr_t) &monstermoves,sizeof(monstermoves));
	bwrite(fd,(genericptr_t) &upstair,sizeof(stairway));
	bwrite(fd,(genericptr_t) &dnstair,sizeof(stairway));
	bwrite(fd,(genericptr_t) &upladder,sizeof(stairway));
	bwrite(fd,(genericptr_t) &dnladder,sizeof(stairway));
	bwrite(fd,(genericptr_t) &sstairs,sizeof(stairway));
	bwrite(fd,(genericptr_t) &updest,sizeof(dest_area));
	bwrite(fd,(genericptr_t) &dndest,sizeof(dest_area));
	bwrite(fd,(genericptr_t) &level.flags,sizeof(level.flags));
	bwrite(fd, (genericptr_t) doors, sizeof(doors));
	save_rooms(fd);	/* no dynamic memory to reclaim */

	/* from here on out, saving also involves allocated memory cleanup */
 skip_lots:
	/* must be saved before mons, objs, and buried objs */
	save_timers(fd, mode, RANGE_LEVEL);
	save_light_sources(fd, mode, RANGE_LEVEL);

	savemonchn(fd, fmon, mode);
	save_worm(fd, mode);	/* save worm information */
	savetrapchn(fd, ftrap, mode);
	saveobjchn(fd, fobj, mode);
	saveobjchn(fd, level.buriedobjlist, mode);
	saveobjchn(fd, billobjs, mode);
	if (release_data(mode)) {
	    fmon = 0;
	    ftrap = 0;
	    fobj = 0;
	    level.buriedobjlist = 0;
	    billobjs = 0;
	}
	save_engravings(fd, mode);
	savedamage(fd, mode);
	save_regions(fd, mode);
	if (mode != FREE_SAVE) bflush(fd);
}

#ifdef ZEROCOMP
/* The runs of zero-run compression are flushed after the game state or a
 * level is written out.  This adds a couple bytes to a save file, where
 * the runs could be mashed together, but it allows gluing together game
 * state and level files to form a save file, and it means the flushing
 * does not need to be specifically called for every other time a level
 * file is written out.
 */

#define RLESC '\0'    /* Leading character for run of LRESC's */
#define flushoutrun(ln) (bputc(RLESC), bputc(ln), ln = -1)

#ifndef ZEROCOMP_BUFSIZ
# define ZEROCOMP_BUFSIZ BUFSZ
#endif
static NEARDATA unsigned char outbuf[ZEROCOMP_BUFSIZ];
static NEARDATA unsigned short outbufp = 0;
static NEARDATA short outrunlength = -1;
static NEARDATA int bwritefd;
static NEARDATA boolean compressing = FALSE;

/*dbg()
{
    HUP printf("outbufp %d outrunlength %d\n", outbufp,outrunlength);
}*/

STATIC_OVL void
bputc(c)
int c;
{
#ifdef MFLOPPY
    bytes_counted++;
    if (count_only)
      return;
#endif
    if (outbufp >= sizeof outbuf) {
	(void) write(bwritefd, outbuf, sizeof outbuf);
	outbufp = 0;
    }
    outbuf[outbufp++] = (unsigned char)c;
}

/*ARGSUSED*/
void
bufon(fd)
int fd;
{
    compressing = TRUE;
    return;
}

/*ARGSUSED*/
void
bufoff(fd)
int fd;
{
    if (outbufp) {
	outbufp = 0;
	panic("closing file with buffered data still unwritten");
    }
    outrunlength = -1;
    compressing = FALSE;
    return;
}

void
bflush(fd)  /* flush run and buffer */
register int fd;
{
    bwritefd = fd;
    if (outrunlength >= 0) {	/* flush run */
	flushoutrun(outrunlength);
    }
#ifdef MFLOPPY
    if (count_only) outbufp = 0;
#endif

    if (outbufp) {
	if (write(fd, outbuf, outbufp) != outbufp) {
#if defined(UNIX) || defined(VMS) || defined(__EMX__)
	    if (program_state.done_hup)
		terminate(EXIT_FAILURE);
	    else
#endif
		bclose(fd);	/* panic (outbufp != 0) */
	}
	outbufp = 0;
    }
}

void
bwrite(fd, loc, num)
int fd;
genericptr_t loc;
register unsigned num;
{
    register unsigned char *bp = (unsigned char *)loc;

    if (!compressing) {
#ifdef MFLOPPY
	bytes_counted += num;
	if (count_only) return;
#endif
	if ((unsigned) write(fd, loc, num) != num) {
#if defined(UNIX) || defined(VMS) || defined(__EMX__)
	    if (program_state.done_hup)
		terminate(EXIT_FAILURE);
	    else
#endif
		panic("cannot write %u bytes to file #%d", num, fd);
	}
    } else {
	bwritefd = fd;
	for (; num; num--, bp++) {
	    if (*bp == RLESC) {	/* One more char in run */
		if (++outrunlength == 0xFF) {
		    flushoutrun(outrunlength);
		}
	    } else {		/* end of run */
		if (outrunlength >= 0) {	/* flush run */
		    flushoutrun(outrunlength);
		}
		bputc(*bp);
	    }
	}
    }
}

void
bclose(fd)
int fd;
{
    bufoff(fd);
    (void) close(fd);
    return;
}

#else /* ZEROCOMP */

static int bw_fd = -1;
static FILE *bw_FILE = 0;
static boolean buffering = FALSE;

void
bufon(fd)
    int fd;
{
#ifdef UNIX
    if(bw_fd >= 0)
	panic("double buffering unexpected");
    bw_fd = fd;
    if((bw_FILE = fdopen(fd, "w")) == 0)
	panic("buffering of file %d failed", fd);
#endif
    buffering = TRUE;
}

void
bufoff(fd)
int fd;
{
    bflush(fd);
    buffering = FALSE;
}

void
bflush(fd)
    int fd;
{
#ifdef UNIX
    if(fd == bw_fd) {
	if(fflush(bw_FILE) == EOF)
	    panic("flush of savefile failed!");
    }
#endif
    return;
}

void
bwrite(fd,loc,num)
register int fd;
register genericptr_t loc;
register unsigned num;
{
	boolean failed;

#ifdef MFLOPPY
	bytes_counted += num;
	if (count_only) return;
#endif

#ifdef UNIX
	if (buffering) {
	    if(fd != bw_fd)
		panic("unbuffered write to fd %d (!= %d)", fd, bw_fd);

	    failed = (fwrite(loc, (int)num, 1, bw_FILE) != 1);
	} else
#endif /* UNIX */
	{
/* lint wants the 3rd arg of write to be an int; lint -p an unsigned */
#if defined(BSD) || defined(ULTRIX)
	    failed = (write(fd, loc, (int)num) != (int)num);
#else /* e.g. SYSV, __TURBOC__ */
	    failed = (write(fd, loc, num) != num);
#endif
	}

	if (failed) {
#if defined(UNIX) || defined(VMS) || defined(__EMX__)
	    if (program_state.done_hup)
		terminate(EXIT_FAILURE);
	    else
#endif
		panic("cannot write %u bytes to file #%d", num, fd);
	}
}

void
bclose(fd)
    int fd;
{
    bufoff(fd);
#ifdef UNIX
    if (fd == bw_fd) {
	(void) fclose(bw_FILE);
	bw_fd = -1;
	bw_FILE = 0;
    } else
#endif
	(void) close(fd);
    return;
}
#endif /* ZEROCOMP */

STATIC_OVL void
savelevchn(fd, mode)
register int fd, mode;
{
	s_level	*tmplev, *tmplev2;
	int cnt = 0;

	for (tmplev = sp_levchn; tmplev; tmplev = tmplev->next) cnt++;
	if (perform_bwrite(mode))
	    bwrite(fd, (genericptr_t) &cnt, sizeof(int));

	for (tmplev = sp_levchn; tmplev; tmplev = tmplev2) {
	    tmplev2 = tmplev->next;
	    if (perform_bwrite(mode))
		bwrite(fd, (genericptr_t) tmplev, sizeof(s_level));
	    if (release_data(mode))
		free((genericptr_t) tmplev);
	}
	if (release_data(mode))
	    sp_levchn = 0;
}

STATIC_OVL void
savedamage(fd, mode)
register int fd, mode;
{
	register struct damage *damageptr, *tmp_dam;
	unsigned int xl = 0;

	damageptr = level.damagelist;
	for (tmp_dam = damageptr; tmp_dam; tmp_dam = tmp_dam->next)
	    xl++;
	if (perform_bwrite(mode))
	    bwrite(fd, (genericptr_t) &xl, sizeof(xl));

	while (xl--) {
	    if (perform_bwrite(mode))
		bwrite(fd, (genericptr_t) damageptr, sizeof(*damageptr));
	    tmp_dam = damageptr;
	    damageptr = damageptr->next;
	    if (release_data(mode))
		free((genericptr_t)tmp_dam);
	}
	if (release_data(mode))
	    level.damagelist = 0;
}

STATIC_OVL void
saveobjchn(fd, otmp, mode)
register int fd, mode;
register struct obj *otmp;
{
	register struct obj *otmp2;
	unsigned int xl;
	int minusone = -1;

	while(otmp) {
	    otmp2 = otmp->nobj;
	    if (perform_bwrite(mode)) {
		xl = otmp->oxlth + otmp->onamelth;
		bwrite(fd, (genericptr_t) &xl, sizeof(int));
		bwrite(fd, (genericptr_t) otmp, xl + sizeof(struct obj));
	    }
	    if (Has_contents(otmp))
		saveobjchn(fd,otmp->cobj,mode);
	    if (release_data(mode)) {
		if (otmp->oclass == FOOD_CLASS) food_disappears(otmp);
		if (otmp->oclass == SPBOOK_CLASS) book_disappears(otmp);
		otmp->where = OBJ_FREE;	/* set to free so dealloc will work */
		otmp->timed = 0;	/* not timed any more */
		otmp->lamplit = 0;	/* caller handled lights */
		dealloc_obj(otmp);
	    }
	    otmp = otmp2;
	}
	if (perform_bwrite(mode))
	    bwrite(fd, (genericptr_t) &minusone, sizeof(int));
}

STATIC_OVL void
savemonchn(fd, mtmp, mode)
register int fd, mode;
register struct monst *mtmp;
{
	register struct monst *mtmp2;
	unsigned int xl;
	int minusone = -1;
	struct permonst *monbegin = &mons[0];

	if (perform_bwrite(mode))
	    bwrite(fd, (genericptr_t) &monbegin, sizeof(monbegin));

	while (mtmp) {
	    mtmp2 = mtmp->nmon;
	    if (perform_bwrite(mode)) {
		xl = mtmp->mxlth + mtmp->mnamelth;
		bwrite(fd, (genericptr_t) &xl, sizeof(int));
		bwrite(fd, (genericptr_t) mtmp, xl + sizeof(struct monst));
	    }
	    if (mtmp->minvent)
		saveobjchn(fd,mtmp->minvent,mode);
	    if (release_data(mode))
		dealloc_monst(mtmp);
	    mtmp = mtmp2;
	}
	if (perform_bwrite(mode))
	    bwrite(fd, (genericptr_t) &minusone, sizeof(int));
}

STATIC_OVL void
savetrapchn(fd, trap, mode)
register int fd, mode;
register struct trap *trap;
{
	register struct trap *trap2;

	while (trap) {
	    trap2 = trap->ntrap;
	    if (perform_bwrite(mode))
		bwrite(fd, (genericptr_t) trap, sizeof(struct trap));
	    if (release_data(mode))
		dealloc_trap(trap);
	    trap = trap2;
	}
	if (perform_bwrite(mode))
	    bwrite(fd, (genericptr_t)nulls, sizeof(struct trap));
}

/* save all the fruit names and ID's; this is used only in saving whole games
 * (not levels) and in saving bones levels.  When saving a bones level,
 * we only want to save the fruits which exist on the bones level; the bones
 * level routine marks nonexistent fruits by making the fid negative.
 */
void
savefruitchn(fd, mode)
register int fd, mode;
{
	register struct fruit *f2, *f1;

	f1 = ffruit;
	while (f1) {
	    f2 = f1->nextf;
	    if (f1->fid >= 0 && perform_bwrite(mode))
		bwrite(fd, (genericptr_t) f1, sizeof(struct fruit));
	    if (release_data(mode))
		dealloc_fruit(f1);
	    f1 = f2;
	}
	if (perform_bwrite(mode))
	    bwrite(fd, (genericptr_t)nulls, sizeof(struct fruit));
	if (release_data(mode))
	    ffruit = 0;
}

/* also called by prscore(); this probably belongs in dungeon.c... */
void
free_dungeons()
{
#ifdef FREE_ALL_MEMORY
	savelevchn(0, FREE_SAVE);
	save_dungeon(0, FALSE, TRUE);
#endif
	return;
}

void
freedynamicdata()
{
	unload_qtlist();
	free_invbuf();	/* let_to_name (invent.c) */
	free_youbuf();	/* You_buf,&c (pline.c) */
	tmp_at(DISP_FREEMEM, 0);	/* temporary display effects */
#ifdef FREE_ALL_MEMORY
# define freeobjchn(X)	(saveobjchn(0, X, FREE_SAVE),  X = 0)
# define freemonchn(X)	(savemonchn(0, X, FREE_SAVE),  X = 0)
# define freetrapchn(X)	(savetrapchn(0, X, FREE_SAVE), X = 0)
# define freefruitchn()	 savefruitchn(0, FREE_SAVE)
# define freenames()	 savenames(0, FREE_SAVE)
# define free_oracles()	save_oracles(0, FREE_SAVE)
# define free_waterlevel() save_waterlevel(0, FREE_SAVE)
# define free_worm()	 save_worm(0, FREE_SAVE)
# define free_timers(R)	 save_timers(0, FREE_SAVE, R)
# define free_light_sources(R) save_light_sources(0, FREE_SAVE, R);
# define free_engravings() save_engravings(0, FREE_SAVE)
# define freedamage()	 savedamage(0, FREE_SAVE)
# define free_animals()	 mon_animal_list(FALSE)

	/* move-specific data */
	dmonsfree();		/* release dead monsters */

	/* level-specific data */
	free_timers(RANGE_LEVEL);
	free_light_sources(RANGE_LEVEL);
	freemonchn(fmon);
	free_worm();		/* release worm segment information */
	freetrapchn(ftrap);
	freeobjchn(fobj);
	freeobjchn(level.buriedobjlist);
	freeobjchn(billobjs);
	free_engravings();
	freedamage();

	/* game-state data */
	free_timers(RANGE_GLOBAL);
	free_light_sources(RANGE_GLOBAL);
	freeobjchn(invent);
	freeobjchn(migrating_objs);
	freemonchn(migrating_mons);
	freemonchn(mydogs);		/* ascension or dungeon escape */
     /* freelevchn();	[folded into free_dungeons()] */
	free_animals();
	free_oracles();
	freefruitchn();
	freenames();
	free_waterlevel();
	free_dungeons();

	/* some pointers in iflags */
	if (iflags.wc_font_map) free(iflags.wc_font_map);
	if (iflags.wc_font_message) free(iflags.wc_font_message);
	if (iflags.wc_font_text) free(iflags.wc_font_text);
	if (iflags.wc_font_menu) free(iflags.wc_font_menu);
	if (iflags.wc_font_status) free(iflags.wc_font_status);
	if (iflags.wc_tile_file) free(iflags.wc_tile_file);

#endif	/* FREE_ALL_MEMORY */
	return;
}

#ifdef MFLOPPY
boolean
swapin_file(lev)
int lev;
{
	char to[PATHLEN], from[PATHLEN];

	Sprintf(from, "%s%s", permbones, alllevels);
	Sprintf(to, "%s%s", levels, alllevels);
	set_levelfile_name(from, lev);
	set_levelfile_name(to, lev);
	if (iflags.checkspace) {
		while (level_info[lev].size > freediskspace(to))
			if (!swapout_oldest())
				return FALSE;
	}
# ifdef WIZARD
	if (wizard) {
		pline("Swapping in `%s'", from);
		wait_synch();
	}
# endif
	copyfile(from, to);
	(void) unlink(from);
	level_info[lev].where = ACTIVE;
	return TRUE;
}

STATIC_OVL boolean
swapout_oldest() {
	char to[PATHLEN], from[PATHLEN];
	int i, oldest;
	long oldtime;

	if (!ramdisk)
		return FALSE;
	for (i = 1, oldtime = 0, oldest = 0; i <= maxledgerno(); i++)
		if (level_info[i].where == ACTIVE
		&& (!oldtime || level_info[i].time < oldtime)) {
			oldest = i;
			oldtime = level_info[i].time;
		}
	if (!oldest)
		return FALSE;
	Sprintf(from, "%s%s", levels, alllevels);
	Sprintf(to, "%s%s", permbones, alllevels);
	set_levelfile_name(from, oldest);
	set_levelfile_name(to, oldest);
# ifdef WIZARD
	if (wizard) {
		pline("Swapping out `%s'.", from);
		wait_synch();
	}
# endif
	copyfile(from, to);
	(void) unlink(from);
	level_info[oldest].where = SWAPPED;
	return TRUE;
}

STATIC_OVL void
copyfile(from, to)
char *from, *to;
{
# ifdef TOS

	if (_copyfile(from, to))
		panic("Can't copy %s to %s", from, to);
# else
	char buf[BUFSIZ];	/* this is system interaction, therefore
				 * BUFSIZ instead of NetHack's BUFSZ */
	int nfrom, nto, fdfrom, fdto;

	if ((fdfrom = open(from, O_RDONLY | O_BINARY, FCMASK)) < 0)
		panic("Can't copy from %s !?", from);
	if ((fdto = open(to, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, FCMASK)) < 0)
		panic("Can't copy to %s", to);
	do {
		nfrom = read(fdfrom, buf, BUFSIZ);
		nto = write(fdto, buf, nfrom);
		if (nto != nfrom)
			panic("Copyfile failed!");
	} while (nfrom == BUFSIZ);
	(void) close(fdfrom);
	(void) close(fdto);
# endif /* TOS */
}

void
co_false()	    /* see comment in bones.c */
{
    count_only = FALSE;
    return;
}

#endif /* MFLOPPY */

/*save.c*/
