/*	SCCS Id: @(#)macmain.c	3.1	97/01/22	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* main.c - Mac NetHack */

#include "hack.h"
#include "dlb.h"
#include "macwin.h"
#include "mactty.h"

#if !TARGET_API_MAC_CARBON
#include <OSUtils.h>
#include <files.h>
#include <Types.h>
#include <Dialogs.h>
#include <Packages.h>
#include <ToolUtils.h>
#include <Resources.h>
#include <Errors.h>
#endif

#ifndef O_RDONLY
#include <fcntl.h>
#endif

static void finder_file_request(void);
int main(void);

#if __SC__ || __MRC__
QDGlobals qd;
#endif


int
main (void)
{
	register int fd = -1;
	int argc = 1;

	windowprocs = mac_procs;
	InitMac ();

	hname = "Mac Hack";
	hackpid = getpid();

	/*
	 * Initialisation of the boundaries of the mazes
	 * Both boundaries have to be even.
	 */

	x_maze_max = COLNO-1;
	if (x_maze_max % 2)
		x_maze_max--;
	y_maze_max = ROWNO-1;
	if (y_maze_max % 2)
		y_maze_max--;

	setrandom();
	initoptions();
	init_nhwindows(&argc, (char **)&hname);

	/*
	 * It seems you really want to play.
	 */
	u.uhp = 1;	/* prevent RIP on early quits */

	finder_file_request ();

	dlb_init();		/* must be before newgame() */

	/*
	 *  Initialize the vision system.  This must be before mklev() on a
	 *  new game or before a level restore on a saved game.
	 */
	vision_init();

	display_gamewindows();

#ifdef WIZARD
	if (wizard)
		Strcpy(plname, "wizard");
	else
#endif
	if(!*plname || !strncmp(plname, "player", 4) || !strncmp(plname, "games", 4))
		askname();
	plnamesuffix();		/* strip suffix from name; calls askname() */
				/* again if suffix was whole name */
				/* accepts any suffix */

	Sprintf (lock, "%d%s", getuid (), plname);
	getlock ();

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
		game_active = 1;
		if (dorecover(fd)) {
#ifdef WIZARD
			if(!wizard && remember_wiz_mode) wizard = TRUE;
#endif
			check_special_room(FALSE);

			if (discover || wizard) {
				if(yn("Do you want to keep the save file?") == 'n')
					(void) delete_savefile();
				else {
					compress(fqname(SAVEF, SAVEPREFIX, 0));
				}
			}
		}
		else {
			fd = -1; /* set bad status */
		}
	}
	if (fd < 0) {
		player_selection();
		game_active = 1;	/* done with selection, draw active game window */
		newgame();
		set_wear();
		(void) pickup(1);
	}

	if (discover)
		You("are in non-scoring discovery mode.");
	flags.move = 0;

	UndimMenuBar (); /* Yes, this is the place for it (!) */
	
	moveloop();

	exit(EXIT_SUCCESS);
	/*NOTREACHED*/
	return 0;
}


static OSErr
copy_file(short src_vol, long src_dir, short dst_vol, long dst_dir,
		Str255 fName,
		pascal OSErr (*opener)(short vRefNum, long dirID,
								ConstStr255Param fileName,
								signed char permission, short *refNum)) {
	short src_ref, dst_ref;
	OSErr err = (*opener)(src_vol, src_dir, fName, fsRdPerm, &src_ref);
	if (err == noErr) {
		err = (*opener)(dst_vol, dst_dir, fName, fsWrPerm, &dst_ref);
		if (err == noErr) {

			long file_len;
			err = GetEOF(src_ref, &file_len);
			if (err == noErr) {
				Handle buf;
				long count = MaxBlock();
				if (count > file_len)
					count = file_len;

				buf = NewHandle(count);
				err = MemError();
				if (err == noErr) {

					while (count > 0) {
						OSErr rd_err = FSRead(src_ref, &count, *buf);
						err = FSWrite(dst_ref, &count, *buf);
						if (err == noErr)
							err = rd_err;
						file_len -= count;
					}
					if (file_len == 0)
						err = noErr;

					DisposeHandle(buf);

				}
			}
			FSClose(dst_ref);
		}
		FSClose(src_ref);
	}

	return err;
}

static void
force_hdelete(short vol, long dir, Str255 fName)
{
	HRstFLock(vol, dir, fName);
	HDelete (vol, dir, fName);
}


void
process_openfile (short src_vol, long src_dir, Str255 fName, OSType ftype)
{
	OSErr	err = noErr;
	
	if (ftype != SAVE_TYPE)
		return;		/* only deal with save files */
		
	if (src_vol != theDirs.dataRefNum || src_dir != theDirs.dataDirID &&
		 CatMove(src_vol, src_dir, fName, theDirs.dataDirID, "\p:") != noErr) {

		HCreate(theDirs.dataRefNum, theDirs.dataDirID, fName, MAC_CREATOR, SAVE_TYPE);
		err = copy_file(src_vol, src_dir, theDirs.dataRefNum, theDirs.dataDirID,
						fName, &HOpen); /* HOpenDF is only there under 7.0 */
		if (err == noErr)
			err = copy_file(src_vol, src_dir, theDirs.dataRefNum, theDirs.dataDirID,
							fName, &HOpenRF);
		if (err == noErr)
			force_hdelete(src_vol, src_dir, fName);
		else
			HDelete(theDirs.dataRefNum, theDirs.dataDirID, fName);
	}

	if (err == noErr) {
		short ref;

		ref = HOpenResFile(theDirs.dataRefNum, theDirs.dataDirID, fName, fsRdPerm);
		if (ref != -1) {
			Handle name = Get1Resource('STR ', PLAYER_NAME_RES_ID);
			if (name) {
				Str255 save_f_p;
				P2C(*(StringHandle)name, plname);
				set_savefile_name();
				C2P(fqname(SAVEF, SAVEPREFIX, 0), save_f_p);
				force_hdelete(theDirs.dataRefNum, theDirs.dataDirID, save_f_p);

				if (HRename(theDirs.dataRefNum, theDirs.dataDirID, fName, save_f_p) == noErr)
					macFlags.gotOpen = 1;
			}
			CloseResFile(ref);
		}
	}
}


static void
finder_file_request(void)
{
	if (macFlags.hasAE) {
		/* we're capable of handling Apple Events, so let's see if we have any */
		EventRecord event;
		long toWhen = TickCount () + 20;	/* wait a third of a second for all initial AE */

		while (TickCount () < toWhen) {
			if (WaitNextEvent (highLevelEventMask, &event, 3L, 0)) {
				AEProcessAppleEvent(&event);
				if (macFlags.gotOpen)
					break;
			}
		}	
	}
#if 0
#ifdef MAC68K
	else {
		short finder_msg, file_count;
		CountAppFiles(&finder_msg, &file_count);
		if (finder_msg == appOpen && file_count == 1) {
			OSErr	err;
			AppFile src;
			FSSpec filespec;

			GetAppFiles(1, &src);
			err = FSMakeFSSpec(src.vRefNum, 0, src.fName, &filespec);
			if (err == noErr && src.fType == SAVE_TYPE) {
				process_openfile (filespec.vRefNum, filespec.parID, filespec.name, src.fType);
				if (macFlags.gotOpen)
					ClrAppFiles(1);
			}
		}
	}
#endif /* MAC68K */
#endif /* 0 */
}

/*macmain.c*/
