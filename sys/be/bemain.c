/*	SCCS Id: @(#)bemain.c	3.4	1998/07/15	*/
/* Copyright (c) Dean Luick, 1996. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "dlb.h"
#include <fcntl.h>

static void whoami(void);
static void process_options(int argc, char **argv);
static void chdirx(const char *dir);
static void getlock(void);

#ifdef __begui__
	#define MAIN nhmain
	int nhmain(int argc, char **argv);
#else
	#define MAIN main
#endif


int MAIN(int argc, char **argv)
{
	int fd;
	char *dir;	

	dir = nh_getenv("NETHACKDIR");
	if (!dir) dir = nh_getenv("HACKDIR");

	choose_windows(DEFAULT_WINDOW_SYS);
	chdirx(dir);
	initoptions();

	init_nhwindows(&argc, argv);
	whoami();

	/*
	 * It seems you really want to play.
	 */
	u.uhp = 1;	/* prevent RIP on early quits */
	process_options(argc, argv);	/* command line options */


#ifdef WIZARD
	if (wizard)
		Strcpy(plname, "wizard");
	else
#endif
	if(!*plname || !strncmp(plname, "player", 4)
		    || !strncmp(plname, "games", 4))
		askname();
	plnamesuffix();		/* strip suffix from name; calls askname() */
						/* again if suffix was whole name */
						/* accepts any suffix */

	Sprintf(lock,"%d%s", getuid(), plname);
	getlock();


	dlb_init();			/* must be before newgame() */

	/*
	 * Initialization of the boundaries of the mazes
	 * Both boundaries have to be even.
	 */
	x_maze_max = COLNO-1;
	if (x_maze_max % 2)
		x_maze_max--;
	y_maze_max = ROWNO-1;
	if (y_maze_max % 2)
		y_maze_max--;

	/*
	 * Initialize the vision system.  This must be before mklev() on a
	 * new game or before a level restore on a saved game.
	 */
	vision_init();

	display_gamewindows();

	if ((fd = restore_saved_game()) >= 0) {
#ifdef WIZARD
		/* Since wizard is actually flags.debug, restoring might
		 * overwrite it.
		 */
		boolean remember_wiz_mode = wizard;
#endif
#ifdef NEWS
		if(iflags.news) {
			display_file(NEWS, FALSE);
			iflags.news = FALSE;	/* in case dorecover() fails */
		}
#endif
		pline("Restoring save file...");
		mark_synch();	/* flush output */
		if(!dorecover(fd))
			goto not_recovered;
#ifdef WIZARD
		if(!wizard && remember_wiz_mode) wizard = TRUE;
#endif
		check_special_room(FALSE);
		if (discover)
			You("are in non-scoring discovery mode.");

		if (discover || wizard) {
			if(yn("Do you want to keep the save file?") == 'n')
			    (void) delete_savefile();
			else {
			    compress(fqname(SAVEF, SAVEPREFIX, 0));
			}
		}

		flags.move = 0;
	} else {
not_recovered:
		player_selection();
		newgame();
		if (discover)
			You("are in non-scoring discovery mode.");

		flags.move = 0;
		set_wear();
		(void) pickup(1);
	}

	moveloop();
	return 0;
}

static void whoami(void)
{
	/*
	 * Who am i? Algorithm: 1. Use name as specified in NETHACKOPTIONS
	 *                      2. Use $USER or $LOGNAME        (if 1. fails)
	 * The resulting name is overridden by command line options.
	 * If everything fails, or if the resulting name is some generic
	 * account like "games", "play", "player", "hack" then eventually
	 * we'll ask him.
	 */
	char *s;

	if (*plname) return;
	if (s = nh_getenv("USER")) {
		(void) strncpy(plname, s, sizeof(plname)-1);
		return;
	}
	if (s = nh_getenv("LOGNAME")) {
		(void) strncpy(plname, s, sizeof(plname)-1);
		return;
	}
}

/* normalize file name - we don't like .'s, /'s, spaces */
void regularize(char *s)
{
	register char *lp;

	while((lp=strchr(s, '.')) || (lp=strchr(s, '/')) || (lp=strchr(s,' ')))
		*lp = '_';
}

static void process_options(int argc, char **argv)
{
	int i;

	while (argc > 1 && argv[1][0] == '-') {
		argv++;
		argc--;
		switch (argv[0][1]) {
		case 'D':
#ifdef WIZARD
			wizard = TRUE;
			break;
#endif
			/* otherwise fall thru to discover */
		case 'X':
			discover = TRUE;
			break;
#ifdef NEWS
		case 'n':
			iflags.news = FALSE;
			break;
#endif
		case 'u':
			if(argv[0][2])
				(void) strncpy(plname, argv[0]+2, sizeof(plname)-1);
			else if (argc > 1) {
				argc--;
				argv++;
				(void) strncpy(plname, argv[0], sizeof(plname)-1);
			} else
				raw_print("Player name expected after -u");
			break;
		case 'p': /* profession (role) */
			if (argv[0][2]) {
			    if ((i = str2role(&argv[0][2])) >= 0)
			    	flags.initrole = i;
			} else if (argc > 1) {
				argc--;
				argv++;
			    if ((i = str2role(argv[0])) >= 0)
			    	flags.initrole = i;
			}
			break;
		case 'r': /* race */
			if (argv[0][2]) {
			    if ((i = str2race(&argv[0][2])) >= 0)
			    	flags.initrace = i;
			} else if (argc > 1) {
				argc--;
				argv++;
			    if ((i = str2race(argv[0])) >= 0)
			    	flags.initrace = i;
			}
			break;
		case '@':
			flags.randomall = 1;
			break;
		default:
			raw_printf("Unknown option: %s", *argv);
			break;
		}
	}
}

static void chdirx(const char *dir)
{
	if (!dir) dir = HACKDIR;

	if (chdir(dir) < 0)
		error("Cannot chdir to %s.", dir);

	/* Warn the player if we can't write the record file */
	/* perhaps we should also test whether . is writable */
	check_recordfile(dir);
}

void getlock(void)
{
	int fd;

	regularize(lock);
	set_levelfile_name(lock, 0);
	fd = creat(lock, FCMASK);
	if(fd == -1) {
		error("cannot creat lock file.");
	} else {
		if(write(fd, (genericptr_t) &hackpid, sizeof(hackpid))
		    != sizeof(hackpid)){
			error("cannot write lock");
		}
		if(close(fd) == -1) {
			error("cannot close lock");
		}
	}
}

#ifndef __begui__
/*
 * If we are not using the Be GUI, then just exit -- we don't need to
 * do anything extra.
 */
void nethack_exit(int status);
void nethack_exit(int status)
{
	exit(status);
}
#endif /* !__begui__ */
