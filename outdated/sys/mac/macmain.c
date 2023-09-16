/* NetHack 3.6	macmain.c	$NHDT-Date: 1432512796 2015/05/25 00:13:16 $  $NHDT-Branch: master $:$NHDT-Revision: 1.21 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2009. */
/* NetHack may be freely redistributed.  See license for details. */

/* main.c - Mac NetHack */

#include "hack.h"
#include "dlb.h"
#include "macwin.h"
#include "mactty.h"

#if 1 /*!TARGET_API_MAC_CARBON*/
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
main(void)
{
    register int fd = -1;
    int argc = 1;
    boolean resuming = FALSE; /* assume new game */

    early_init();
    windowprocs = mac_procs;
    InitMac();

    gh.hname = "Mac Hack";
    hackpid = getpid();

    setrandom();
    initoptions();
    init_nhwindows(&argc, (char **) &gh.hname);

    /*
     * It seems you really want to play.
     */
    u.uhp = 1; /* prevent RIP on early quits */

    finder_file_request();

    dlb_init(); /* must be before newgame() */

    /*
     *  Initialize the vision system.  This must be before mklev() on a
     *  new game or before a level restore on a saved game.
     */
    vision_init();

    init_sound_disp_gamewindows();

    set_playmode(); /* sets plname to "wizard" for wizard mode */
    /* strip role,race,&c suffix; calls askname() if plname[] is empty
       or holds a generic user name like "player" or "games" */
    plnamesuffix();
    /* unlike Unix where the game might be invoked with a script
       which forces a particular character name for each player
       using a shared account, we always allow player to rename
       the character during role/race/&c selection */
    iflags.renameallowed = TRUE;

    getlock();

/*
 * First, try to find and restore a save file for specified character.
 * We'll return here if new game player_selection() renames the hero.
 */
attempt_restore:
    if ((fd = restore_saved_game()) >= 0) {
#ifdef NEWS
        if (iflags.news) {
            display_file(NEWS, FALSE);
            iflags.news = FALSE; /* in case dorecover() fails */
        }
#endif
        pline("Restoring save file...");
        mark_synch(); /* flush output */
        game_active = 1;
        if (dorecover(fd)) {
            resuming = TRUE; /* not starting new game */
            if (discover)
                You("are in non-scoring discovery mode.");
            if (discover || wizard) {
                if (y_n("Do you want to keep the save file?") == 'n')
                    (void) delete_savefile();
                else {
                    nh_compress(fqname(gs.SAVEF, SAVEPREFIX, 0));
                }
            }
        }
    }

    if (!resuming) {
        /* new game:  start by choosing role, race, etc;
           player might change the hero's name while doing that,
           in which case we try to restore under the new name
           and skip selection this time if that didn't succeed */
        if (!iflags.renameinprogress) {
            player_selection();
            if (iflags.renameinprogress) {
                /* player has renamed the hero while selecting role;
                   discard current lock file and create another for
                   the new character name */
                delete_levelfile(0); /* remove empty lock file */
                getlock();
                goto attempt_restore;
            }
        }
        game_active = 1; /* done with selection, draw active game window */
        newgame();
        if (discover)
            You("are in non-scoring discovery mode.");
    }

    UndimMenuBar(); /* Yes, this is the place for it (!) */

    moveloop(resuming);

    exit(EXIT_SUCCESS);
    /*NOTREACHED*/
    return 0;
}

static OSErr
copy_file(short src_vol, long src_dir, short dst_vol, long dst_dir,
          Str255 fName,
          pascal OSErr (*opener)(short vRefNum, long dirID,
                                 ConstStr255Param fileName,
                                 signed char permission, short *refNum))
{
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
    HDelete(vol, dir, fName);
}

void
process_openfile(short src_vol, long src_dir, Str255 fName, OSType ftype)
{
    OSErr err = noErr;

    if (ftype != SAVE_TYPE)
        return; /* only deal with save files */

    if (src_vol != theDirs.dataRefNum
        || src_dir != theDirs.dataDirID
               && CatMove(src_vol, src_dir, fName, theDirs.dataDirID, "\p:")
                      != noErr) {
        HCreate(theDirs.dataRefNum, theDirs.dataDirID, fName, MAC_CREATOR,
                SAVE_TYPE);
        err =
            copy_file(src_vol, src_dir, theDirs.dataRefNum, theDirs.dataDirID,
                      fName, &HOpen); /* HOpenDF is only there under 7.0 */
        if (err == noErr)
            err = copy_file(src_vol, src_dir, theDirs.dataRefNum,
                            theDirs.dataDirID, fName, &HOpenRF);
        if (err == noErr)
            force_hdelete(src_vol, src_dir, fName);
        else
            HDelete(theDirs.dataRefNum, theDirs.dataDirID, fName);
    }

    if (err == noErr) {
        short ref;

        ref = HOpenResFile(theDirs.dataRefNum, theDirs.dataDirID, fName,
                           fsRdPerm);
        if (ref != -1) {
            Handle name = Get1Resource('STR ', PLAYER_NAME_RES_ID);
            if (name) {
                Str255 save_f_p;
                P2C(*(StringHandle) name, gp.plname);
                set_savefile_name(TRUE);
                C2P(fqname(gs.SAVEF, SAVEPREFIX, 0), save_f_p);
                force_hdelete(theDirs.dataRefNum, theDirs.dataDirID,
                              save_f_p);

                if (HRename(theDirs.dataRefNum, theDirs.dataDirID, fName,
                            save_f_p) == noErr)
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
        /* we're capable of handling Apple Events, so let's see if we have any
         */
        EventRecord event;
        long toWhen = TickCount()
                      + 20; /* wait a third of a second for all initial AE */

        while (TickCount() < toWhen) {
            if (WaitNextEvent(highLevelEventMask, &event, 3L, 0)) {
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

/* validate wizard mode if player has requested access to it */
boolean
authorize_wizard_mode()
{
    /* other ports validate user name or character name here */
    return TRUE;
}

/*macmain.c*/
