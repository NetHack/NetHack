/*	SCCS Id: @(#)music.c	3.4	2003/05/25	*/
/*	Copyright (c) 1989 by Jean-Christophe Collet */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file contains the different functions designed to manipulate the
 * musical instruments and their various effects.
 *
 * Actually the list of instruments / effects is :
 *
 * (wooden) flute	may calm snakes if player has enough dexterity
 * magic flute		may put monsters to sleep:  area of effect depends
 *			on player level.
 * (tooled) horn	Will awaken monsters:  area of effect depends on player
 *			level.  May also scare monsters.
 * fire horn		Acts like a wand of fire.
 * frost horn		Acts like a wand of cold.
 * bugle		Will awaken soldiers (if any):  area of effect depends
 *			on player level.
 * (wooden) harp	May calm nymph if player has enough dexterity.
 * magic harp		Charm monsters:  area of effect depends on player
 *			level.
 * (leather) drum	Will awaken monsters like the horn.
 * drum of earthquake	Will initiate an earthquake whose intensity depends
 *			on player level.  That is, it creates random pits
 *			called here chasms.
 */

#include "hack.h"

STATIC_DCL void FDECL(awaken_monsters,(int));
STATIC_DCL void FDECL(put_monsters_to_sleep,(int));
STATIC_DCL void FDECL(charm_snakes,(int));
STATIC_DCL void FDECL(calm_nymphs,(int));
STATIC_DCL void FDECL(charm_monsters,(int));
STATIC_DCL void FDECL(do_earthquake,(int));
STATIC_DCL int FDECL(do_improvisation,(struct obj *));

#ifdef UNIX386MUSIC
STATIC_DCL int NDECL(atconsole);
STATIC_DCL void FDECL(speaker,(struct obj *,char *));
#endif
#ifdef VPIX_MUSIC
extern int sco_flag_console;	/* will need changing if not _M_UNIX */
STATIC_DCL void NDECL(playinit);
STATIC_DCL void FDECL(playstring, (char *,size_t));
STATIC_DCL void FDECL(speaker,(struct obj *,char *));
#endif
#ifdef PCMUSIC
void FDECL( pc_speaker, ( struct obj *, char * ) );
#endif
#ifdef AMIGA
void FDECL( amii_speaker, ( struct obj *, char *, int ) );
#endif

/*
 * Wake every monster in range...
 */

STATIC_OVL void
awaken_monsters(distance)
int distance;
{
	register struct monst *mtmp = fmon;
	register int distm;

	while(mtmp) {
	    if (!DEADMONSTER(mtmp)) {
		distm = distu(mtmp->mx, mtmp->my);
		if (distm < distance) {
		    mtmp->msleeping = 0;
		    mtmp->mcanmove = 1;
		    mtmp->mfrozen = 0;
		    /* May scare some monsters */
		    if (distm < distance/3 &&
			    !resist(mtmp, TOOL_CLASS, 0, NOTELL))
			monflee(mtmp, 0, FALSE, TRUE);
		}
	    }
	    mtmp = mtmp->nmon;
	}
}

/*
 * Make monsters fall asleep.  Note that they may resist the spell.
 */

STATIC_OVL void
put_monsters_to_sleep(distance)
int distance;
{
	register struct monst *mtmp = fmon;

	while(mtmp) {
		if (!DEADMONSTER(mtmp) && distu(mtmp->mx, mtmp->my) < distance &&
			sleep_monst(mtmp, d(10,10), TOOL_CLASS)) {
		    mtmp->msleeping = 1; /* 10d10 turns + wake_nearby to rouse */
		    slept_monst(mtmp);
		}
		mtmp = mtmp->nmon;
	}
}

/*
 * Charm snakes in range.  Note that the snakes are NOT tamed.
 */

STATIC_OVL void
charm_snakes(distance)
int distance;
{
	register struct monst *mtmp = fmon;
	int could_see_mon, was_peaceful;

	while (mtmp) {
	    if (!DEADMONSTER(mtmp) && mtmp->data->mlet == S_SNAKE && mtmp->mcanmove &&
		    distu(mtmp->mx, mtmp->my) < distance) {
		was_peaceful = mtmp->mpeaceful;
		mtmp->mpeaceful = 1;
		mtmp->mavenge = 0;
		could_see_mon = canseemon(mtmp);
		mtmp->mundetected = 0;
		newsym(mtmp->mx, mtmp->my);
		if (canseemon(mtmp)) {
		    if (!could_see_mon)
			You("notice %s, swaying with the music.",
			    a_monnam(mtmp));
		    else
			pline("%s freezes, then sways with the music%s.",
			      Monnam(mtmp),
			      was_peaceful ? "" : ", and now seems quieter");
		}
	    }
	    mtmp = mtmp->nmon;
	}
}

/*
 * Calm nymphs in range.
 */

STATIC_OVL void
calm_nymphs(distance)
int distance;
{
	register struct monst *mtmp = fmon;

	while (mtmp) {
	    if (!DEADMONSTER(mtmp) && mtmp->data->mlet == S_NYMPH && mtmp->mcanmove &&
		    distu(mtmp->mx, mtmp->my) < distance) {
		mtmp->msleeping = 0;
		mtmp->mpeaceful = 1;
		mtmp->mavenge = 0;
		if (canseemon(mtmp))
		    pline(
		     "%s listens cheerfully to the music, then seems quieter.",
			  Monnam(mtmp));
	    }
	    mtmp = mtmp->nmon;
	}
}

/* Awake only soldiers of the level. */

void
awaken_soldiers()
{
	register struct monst *mtmp = fmon;

	while(mtmp) {
	    if (!DEADMONSTER(mtmp) &&
			is_mercenary(mtmp->data) && mtmp->data != &mons[PM_GUARD]) {
		mtmp->mpeaceful = mtmp->msleeping = mtmp->mfrozen = 0;
		mtmp->mcanmove = 1;
		if (canseemon(mtmp))
		    pline("%s is now ready for battle!", Monnam(mtmp));
		else
		    Norep("You hear the rattle of battle gear being readied.");
	    }
	    mtmp = mtmp->nmon;
	}
}

/* Charm monsters in range.  Note that they may resist the spell.
 * If swallowed, range is reduced to 0.
 */

STATIC_OVL void
charm_monsters(distance)
int distance;
{
	struct monst *mtmp, *mtmp2;

	if (u.uswallow) {
	    if (!resist(u.ustuck, TOOL_CLASS, 0, NOTELL))
		(void) tamedog(u.ustuck, (struct obj *) 0);
	} else {
	    for (mtmp = fmon; mtmp; mtmp = mtmp2) {
		mtmp2 = mtmp->nmon;
		if (DEADMONSTER(mtmp)) continue;

		if (distu(mtmp->mx, mtmp->my) <= distance) {
		    if (!resist(mtmp, TOOL_CLASS, 0, NOTELL))
			(void) tamedog(mtmp, (struct obj *) 0);
		}
	    }
	}

}

/* Generate earthquake :-) of desired force.
 * That is:  create random chasms (pits).
 */

STATIC_OVL void
do_earthquake(force)
int force;
{
	register int x,y;
	struct monst *mtmp;
	struct obj *otmp;
	struct trap *chasm;
	int start_x, start_y, end_x, end_y;

	start_x = u.ux - (force * 2);
	start_y = u.uy - (force * 2);
	end_x = u.ux + (force * 2);
	end_y = u.uy + (force * 2);
	if (start_x < 1) start_x = 1;
	if (start_y < 1) start_y = 1;
	if (end_x >= COLNO) end_x = COLNO - 1;
	if (end_y >= ROWNO) end_y = ROWNO - 1;
	for (x=start_x; x<=end_x; x++) for (y=start_y; y<=end_y; y++) {
	    if ((mtmp = m_at(x,y)) != 0) {
		wakeup(mtmp);	/* peaceful monster will become hostile */
		if (mtmp->mundetected && is_hider(mtmp->data)) {
		    mtmp->mundetected = 0;
		    if (cansee(x,y))
			pline("%s is shaken loose from the ceiling!",
							    Amonnam(mtmp));
		    else
			You_hear("a thumping sound.");
		    if (x==u.ux && y==u.uy)
			You("easily dodge the falling %s.",
							    mon_nam(mtmp));
		    newsym(x,y);
		}
	    }
	    if (!rn2(14 - force)) switch (levl[x][y].typ) {
		  case FOUNTAIN : /* Make the fountain disappear */
			if (cansee(x,y))
				pline_The("fountain falls into a chasm.");
			goto do_pit;
#ifdef SINKS
		  case SINK :
			if (cansee(x,y))
				pline_The("kitchen sink falls into a chasm.");
			goto do_pit;
#endif
		  case ALTAR :
			if (Is_astralevel(&u.uz) || Is_sanctum(&u.uz)) break;

			if (cansee(x,y))
				pline_The("altar falls into a chasm.");
			goto do_pit;
		  case GRAVE :
			if (cansee(x,y))
				pline_The("headstone topples into a chasm.");
			goto do_pit;
		  case THRONE :
			if (cansee(x,y))
				pline_The("throne falls into a chasm.");
			/* Falls into next case */
		  case ROOM :
		  case CORR : /* Try to make a pit */
do_pit:		    chasm = maketrap(x,y,PIT);
		    if (!chasm) break;	/* no pit if portal at that location */
		    chasm->tseen = 1;

		    levl[x][y].doormask = 0;

		    mtmp = m_at(x,y);

		    if ((otmp = sobj_at(BOULDER, x, y)) != 0) {
			if (cansee(x, y))
			   pline("KADOOM! The boulder falls into a chasm%s!",
			      ((x == u.ux) && (y == u.uy)) ? " below you" : "");
			if (mtmp)
				mtmp->mtrapped = 0;
			obj_extract_self(otmp);
			(void) flooreffects(otmp, x, y, "");
			break;
		    }

		    /* We have to check whether monsters or player
		       falls in a chasm... */

		    if (mtmp) {
			if(!is_flyer(mtmp->data) && !is_clinger(mtmp->data)) {
			    mtmp->mtrapped = 1;
			    if(cansee(x,y))
				pline("%s falls into a chasm!", Monnam(mtmp));
			    else if (flags.soundok && humanoid(mtmp->data))
				You_hear("a scream!");
			    mselftouch(mtmp, "Falling, ", TRUE);
			    if (mtmp->mhp > 0)
				if ((mtmp->mhp -= rnd(6)) <= 0) {
				    if(!cansee(x,y))
					pline("It is destroyed!");
				    else {
					You("destroy %s!", mtmp->mtame ?
					    x_monnam(mtmp, ARTICLE_THE, "poor",
				mtmp->mnamelth ? SUPPRESS_SADDLE : 0, FALSE):
					    mon_nam(mtmp));
				    }
				    xkilled(mtmp,0);
				}
			}
		    } else if (x == u.ux && y == u.uy) {
			    if (Levitation || Flying ||
						is_clinger(youmonst.data)) {
				    pline("A chasm opens up under you!");
				    You("don't fall in!");
			    } else {
				    You("fall into a chasm!");
				    u.utrap = rn1(6,2);
				    u.utraptype = TT_PIT;
				    losehp(rnd(6),"fell into a chasm",
					NO_KILLER_PREFIX);
				    selftouch("Falling, you");
			    }
		    } else newsym(x,y);
		    break;
		  case DOOR : /* Make the door collapse */
		    if (levl[x][y].doormask == D_NODOOR) goto do_pit;
		    if (cansee(x,y))
			pline_The("door collapses.");
		    if (*in_rooms(x, y, SHOPBASE))
			add_damage(x, y, 0L);
		    levl[x][y].doormask = D_NODOOR;
		    unblock_point(x,y);
		    newsym(x,y);
		    break;
	    }
	}
}

/*
 * The player is trying to extract something from his/her instrument.
 */

STATIC_OVL int
do_improvisation(instr)
struct obj *instr;
{
	int damage, do_spec = !Confusion;
#if defined(MAC) || defined(AMIGA) || defined(VPIX_MUSIC) || defined (PCMUSIC)
	struct obj itmp;

	itmp = *instr;
	/* if won't yield special effect, make sound of mundane counterpart */
	if (!do_spec || instr->spe <= 0)
	    while (objects[itmp.otyp].oc_magic) itmp.otyp -= 1;
# ifdef MAC
	mac_speaker(&itmp, "C");
# endif
# ifdef AMIGA
	amii_speaker(&itmp, "Cw", AMII_OKAY_VOLUME);
# endif
# ifdef VPIX_MUSIC
	if (sco_flag_console)
	    speaker(&itmp, "C");
# endif
#ifdef PCMUSIC
	  pc_speaker ( &itmp, "C");
#endif
#endif /* MAC || AMIGA || VPIX_MUSIC || PCMUSIC */

	if (!do_spec)
	    pline("What you produce is quite far from music...");
	else
	    You("start playing %s.", the(xname(instr)));

	switch (instr->otyp) {
	case MAGIC_FLUTE:		/* Make monster fall asleep */
	    if (do_spec && instr->spe > 0) {
		consume_obj_charge(instr, TRUE);

		You("produce soft music.");
		put_monsters_to_sleep(u.ulevel * 5);
		exercise(A_DEX, TRUE);
		break;
	    } /* else FALLTHRU */
	case WOODEN_FLUTE:		/* May charm snakes */
	    do_spec &= (rn2(ACURR(A_DEX)) + u.ulevel > 25);
	    pline("%s.", Tobjnam(instr, do_spec ? "trill" : "toot"));
	    if (do_spec) charm_snakes(u.ulevel * 3);
	    exercise(A_DEX, TRUE);
	    break;
	case FROST_HORN:		/* Idem wand of cold */
	case FIRE_HORN:			/* Idem wand of fire */
	    if (do_spec && instr->spe > 0) {
		consume_obj_charge(instr, TRUE);

		if (!getdir((char *)0)) {
		    pline("%s.", Tobjnam(instr, "vibrate"));
		    break;
		} else if (!u.dx && !u.dy && !u.dz) {
		    if ((damage = zapyourself(instr, TRUE)) != 0) {
			char buf[BUFSZ];
			Sprintf(buf, "using a magical horn on %sself", uhim());
			losehp(damage, buf, KILLED_BY);
		    }
		} else {
		    buzz((instr->otyp == FROST_HORN) ? AD_COLD-1 : AD_FIRE-1,
			 rn1(6,6), u.ux, u.uy, u.dx, u.dy);
		}
		makeknown(instr->otyp);
		break;
	    } /* else FALLTHRU */
	case TOOLED_HORN:		/* Awaken or scare monsters */
	    You("produce a frightful, grave sound.");
	    awaken_monsters(u.ulevel * 30);
	    exercise(A_WIS, FALSE);
	    break;
	case BUGLE:			/* Awaken & attract soldiers */
	    You("extract a loud noise from %s.", the(xname(instr)));
	    awaken_soldiers();
	    exercise(A_WIS, FALSE);
	    break;
	case MAGIC_HARP:		/* Charm monsters */
	    if (do_spec && instr->spe > 0) {
		consume_obj_charge(instr, TRUE);

		pline("%s very attractive music.", Tobjnam(instr, "produce"));
		charm_monsters((u.ulevel - 1) / 3 + 1);
		exercise(A_DEX, TRUE);
		break;
	    } /* else FALLTHRU */
	case WOODEN_HARP:		/* May calm Nymph */
	    do_spec &= (rn2(ACURR(A_DEX)) + u.ulevel > 25);
	    pline("%s %s.", The(xname(instr)),
		  do_spec ? "produces a lilting melody" : "twangs");
	    if (do_spec) calm_nymphs(u.ulevel * 3);
	    exercise(A_DEX, TRUE);
	    break;
	case DRUM_OF_EARTHQUAKE:	/* create several pits */
	    if (do_spec && instr->spe > 0) {
		consume_obj_charge(instr, TRUE);

		You("produce a heavy, thunderous rolling!");
		pline_The("entire dungeon is shaking around you!");
		do_earthquake((u.ulevel - 1) / 3 + 1);
		/* shake up monsters in a much larger radius... */
		awaken_monsters(ROWNO * COLNO);
		makeknown(DRUM_OF_EARTHQUAKE);
		break;
	    } /* else FALLTHRU */
	case LEATHER_DRUM:		/* Awaken monsters */
	    You("beat a deafening row!");
	    awaken_monsters(u.ulevel * 40);
	    exercise(A_WIS, FALSE);
	    break;
	default:
	    impossible("What a weird instrument (%d)!", instr->otyp);
	    break;
	}
	return 2;		/* That takes time */
}

/*
 * So you want music...
 */

int
do_play_instrument(instr)
struct obj *instr;
{
    char buf[BUFSZ], c = 'y';
    char *s;
    int x,y;
    boolean ok;

    if (Underwater) {
	You_cant("play music underwater!");
	return(0);
    }
    if (instr->otyp != LEATHER_DRUM && instr->otyp != DRUM_OF_EARTHQUAKE) {
	c = yn("Improvise?");
    }
    if (c == 'n') {
	if (u.uevent.uheard_tune == 2 && yn("Play the passtune?") == 'y') {
	    Strcpy(buf, tune);
	} else {
	    getlin("What tune are you playing? [5 notes, A-G]", buf);
	    (void)mungspaces(buf);
	    /* convert to uppercase and change any "H" to the expected "B" */
	    for (s = buf; *s; s++) {
#ifndef AMIGA
		*s = highc(*s);
#else
		/* The AMIGA supports two octaves of notes */
		if (*s == 'h') *s = 'b';
#endif
		if (*s == 'H') *s = 'B';
	    }
	}
	You("extract a strange sound from %s!", the(xname(instr)));
#ifdef UNIX386MUSIC
	/* if user is at the console, play through the console speaker */
	if (atconsole())
	    speaker(instr, buf);
#endif
#ifdef VPIX_MUSIC
	if (sco_flag_console)
	    speaker(instr, buf);
#endif
#ifdef MAC
	mac_speaker ( instr , buf ) ;
#endif
#ifdef PCMUSIC
	pc_speaker ( instr, buf );
#endif
#ifdef AMIGA
	{
		char nbuf[ 20 ];
		int i;
		for( i = 0; buf[i] && i < 5; ++i )
		{
			nbuf[ i*2 ] = buf[ i ];
			nbuf[ (i*2)+1 ] = 'h';
		}
		nbuf[ i*2 ] = 0;
		amii_speaker ( instr , nbuf, AMII_OKAY_VOLUME ) ;
	}
#endif
	/* Check if there was the Stronghold drawbridge near
	 * and if the tune conforms to what we're waiting for.
	 */
	if(Is_stronghold(&u.uz)) {
	    exercise(A_WIS, TRUE);		/* just for trying */
	    if(!strcmp(buf,tune)) {
		/* Search for the drawbridge */
		for(y=u.uy-1; y<=u.uy+1; y++)
		    for(x=u.ux-1;x<=u.ux+1;x++)
			if(isok(x,y))
			if(find_drawbridge(&x,&y)) {
			    u.uevent.uheard_tune = 2; /* tune now fully known */
			    if(levl[x][y].typ == DRAWBRIDGE_DOWN)
				close_drawbridge(x,y);
			    else
				open_drawbridge(x,y);
			    return 0;
			}
	    } else if(flags.soundok) {
		if (u.uevent.uheard_tune < 1) u.uevent.uheard_tune = 1;
		/* Okay, it wasn't the right tune, but perhaps
		 * we can give the player some hints like in the
		 * Mastermind game */
		ok = FALSE;
		for(y = u.uy-1; y <= u.uy+1 && !ok; y++)
		    for(x = u.ux-1; x <= u.ux+1 && !ok; x++)
			if(isok(x,y))
			if(IS_DRAWBRIDGE(levl[x][y].typ) ||
			   is_drawbridge_wall(x,y) >= 0)
				ok = TRUE;
		if(ok) { /* There is a drawbridge near */
		    int tumblers, gears;
		    boolean matched[5];

		    tumblers = gears = 0;
		    for(x=0; x < 5; x++)
			matched[x] = FALSE;

		    for(x=0; x < (int)strlen(buf); x++)
			if(x < 5) {
			    if(buf[x] == tune[x]) {
				gears++;
				matched[x] = TRUE;
			    } else
				for(y=0; y < 5; y++)
				    if(!matched[y] &&
				       buf[x] == tune[y] &&
				       buf[y] != tune[y]) {
					tumblers++;
					matched[y] = TRUE;
					break;
				    }
			}
		    if(tumblers)
			if(gears)
			    You_hear("%d tumbler%s click and %d gear%s turn.",
				tumblers, plur(tumblers), gears, plur(gears));
			else
			    You_hear("%d tumbler%s click.",
				tumblers, plur(tumblers));
		    else if(gears) {
			You_hear("%d gear%s turn.", gears, plur(gears));
			/* could only get `gears == 5' by playing five
			   correct notes followed by excess; otherwise,
			   tune would have matched above */
			if (gears == 5) u.uevent.uheard_tune = 2;
		    }
		}
	    }
	  }
	return 1;
    } else
	    return do_improvisation(instr);
}

#ifdef UNIX386MUSIC
/*
 * Play audible music on the machine's speaker if appropriate.
 */

STATIC_OVL int
atconsole()
{
    /*
     * Kluge alert: This code assumes that your [34]86 has no X terminals
     * attached and that the console tty type is AT386 (this is always true
     * under AT&T UNIX for these boxen). The theory here is that your remote
     * ttys will have terminal type `ansi' or something else other than
     * `AT386' or `xterm'. We'd like to do better than this, but testing
     * to see if we're running on the console physical terminal is quite
     * difficult given the presence of virtual consoles and other modern
     * UNIX impedimenta...
     */
    char	*termtype = nh_getenv("TERM");

     return(!strcmp(termtype, "AT386") || !strcmp(termtype, "xterm"));
}

STATIC_OVL void
speaker(instr, buf)
struct obj *instr;
char	*buf;
{
    /*
     * For this to work, you need to have installed the PD speaker-control
     * driver for PC-compatible UNIX boxes that I (esr@snark.thyrsus.com)
     * posted to comp.sources.unix in Feb 1990.  A copy should be included
     * with your nethack distribution.
     */
    int	fd;

    if ((fd = open("/dev/speaker", 1)) != -1)
    {
	/* send a prefix to modify instrumental `timbre' */
	switch (instr->otyp)
	{
	case WOODEN_FLUTE:
	case MAGIC_FLUTE:
	    (void) write(fd, ">ol", 1); /* up one octave & lock */
	    break;
	case TOOLED_HORN:
	case FROST_HORN:
	case FIRE_HORN:
	    (void) write(fd, "<<ol", 2); /* drop two octaves & lock */
	    break;
	case BUGLE:
	    (void) write(fd, "ol", 2); /* octave lock */
	    break;
	case WOODEN_HARP:
	case MAGIC_HARP:
	    (void) write(fd, "l8mlol", 4); /* fast, legato, octave lock */
	    break;
	}
	(void) write(fd, buf, strlen(buf));
	(void) close(fd);
    }
}
#endif /* UNIX386MUSIC */

#ifdef VPIX_MUSIC

# if 0
#include <sys/types.h>
#include <sys/console.h>
#include <sys/vtkd.h>
# else
#define KIOC ('K' << 8)
#define KDMKTONE (KIOC | 8)
# endif

#define noDEBUG

STATIC_OVL void tone(hz, ticks)
/* emit tone of frequency hz for given number of ticks */
unsigned int hz, ticks;
{
    ioctl(0,KDMKTONE,hz|((ticks*10)<<16));
# ifdef DEBUG
    printf("TONE: %6d %6d\n",hz,ticks * 10);
# endif
    nap(ticks * 10);
}

STATIC_OVL void rest(ticks)
/* rest for given number of ticks */
int	ticks;
{
    nap(ticks * 10);
# ifdef DEBUG
    printf("REST:        %6d\n",ticks * 10);
# endif
}


#include "interp.c"	/* from snd86unx.shr */


STATIC_OVL void
speaker(instr, buf)
struct obj *instr;
char	*buf;
{
    /* emit a prefix to modify instrumental `timbre' */
    playinit();
    switch (instr->otyp)
    {
	case WOODEN_FLUTE:
	case MAGIC_FLUTE:
	    playstring(">ol", 1); /* up one octave & lock */
	    break;
	case TOOLED_HORN:
	case FROST_HORN:
	case FIRE_HORN:
	    playstring("<<ol", 2); /* drop two octaves & lock */
	    break;
	case BUGLE:
	    playstring("ol", 2); /* octave lock */
	    break;
	case WOODEN_HARP:
	case MAGIC_HARP:
	    playstring("l8mlol", 4); /* fast, legato, octave lock */
	    break;
    }
    playstring( buf, strlen(buf));
}

# ifdef DEBUG
main(argc,argv)
char *argv[];
{
    if (argc == 2) {
	playinit();
	playstring(argv[1], strlen(argv[1]));
    }
}
# endif
#endif	/* VPIX_MUSIC */

/*music.c*/
