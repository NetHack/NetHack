/* NetHack 5.0	macmain.c	$NHDT-Date: 1432512796 2015/05/25 00:13:16 $  $NHDT-Branch: master $:$NHDT-Revision: 1.21 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2009. */
/* NetHack may be freely redistributed.  See license for details. */

/* main.c - Mac NetHack */

#include "hack.h"
#include "dlb.h"
#include "macwin.h"
#include "mactty.h"
#include "maccompat.h"

#include <OSUtils.h>
#include <Files.h>
#include <Types.h>
#include <Dialogs.h>
#include <Packages.h>
#include <ToolUtils.h>
#include <Resources.h>
#include <Errors.h>

#ifndef O_RDONLY
#include <fcntl.h>
#endif

static void finder_file_request(void);
int main(void);
extern void macalloc_stats(const char *tag); /* profiling hook; no-op unless NHMAC_ALLOC_STATS */


int
main(void)
{
    NHFILE *nhfp;
    int argc = 1;
    boolean resuming = FALSE; /* assume new game */

    early_init(argc, (char **) 0);
    choose_windows("mac");
    InitMac();
    macalloc_stats("boot");   /* baseline; ignore its dt */

    gh.hname = "Mac Hack";
    svh.hackpid = getpid();
    init_nhwindows(&argc, (char **) &gh.hname);

    initoptions();
    macprefs_apply_startup(); /* Preferences-file UI settings override the
                                 config file; game windows don't exist yet */
    macalloc_stats("initoptions");
    iflags.bgcolors = TRUE;
    iflags.use_background_glyph = TRUE;

    u.uhp = 1;
    finder_file_request();

    dlb_init();

    vision_init();
    init_sound_disp_gamewindows();
    macalloc_stats("gamewindows");   /* map window + tile-sheet load (if any) */
    set_playmode();
    plnamesuffix();
    iflags.renameallowed = TRUE;

    getlock();

/* try to restore a save file; re-entered if player_selection() renames the hero */
attempt_restore:
    if (*svp.plname && (nhfp = restore_saved_game()) != 0) {
#ifdef NEWS
        if (iflags.news) {
            display_file(NEWS, FALSE);
            iflags.news = FALSE; /* in case dorecover() fails */
        }
#endif
        pline("Restoring save file...");
        mark_synch(); /* flush output */
        if (dorecover(nhfp)) {
            resuming = TRUE;
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
        /* new game: a rename during role selection re-attempts restore under
           the new name */
        if (!iflags.renameinprogress) {
            player_selection();
            if (iflags.renameinprogress) {
                /* renamed during selection: drop lock file, relock under new name */
                delete_levelfile(0);
                getlock();
                goto attempt_restore;
            }
        }
        macalloc_stats("selection");   /* dt incl. user think-time; resets base */
        newgame();
        macalloc_stats("NEWGAME");     /* <-- the post-selection pause */
        if (discover)
            You("are in non-scoring discovery mode.");
    }

    set_savefile_name(TRUE); /* ensure SAVEF is set for dosave */
    UndimMenuBar();

    macalloc_stats("premoveloop");
    moveloop(resuming);

    exit(EXIT_SUCCESS);
    /*NOTREACHED*/
    return 0;
}

static OSErr
copy_file(FSSpec *src, FSSpec *dst, Boolean rsrc_fork)
{
    short src_ref, dst_ref;
    OSErr err = rsrc_fork ? FSpOpenRF(src, fsRdPerm, &src_ref)
                          : FSpOpenDF(src, fsRdPerm, &src_ref);
    if (err == noErr) {
        err = rsrc_fork ? FSpOpenRF(dst, fsWrPerm, &dst_ref)
                        : FSpOpenDF(dst, fsWrPerm, &dst_ref);
        if (err == noErr) {
            long file_len;
            /* truncate: the destination may exist from an earlier failed
               copy, and a shorter source must not leave stale bytes */
            err = SetEOF(dst_ref, 0L);
            if (err == noErr)
                err = GetEOF(src_ref, &file_len);
            if (err == noErr) {
                Handle buf;
                long count = MaxBlock();
                if (count > file_len)
                    count = file_len;

                buf = NewHandle(count);
                err = MemError();
                if (err == noErr) {
                    long buf_size = count;
                    HLock(buf);
                    while (file_len > 0) {
                        count = (file_len > buf_size) ? buf_size : file_len;
                        OSErr rd_err = FSRead(src_ref, &count, *buf);
                        if (count <= 0) {
                            err = rd_err ? rd_err : ioErr;
                            break;
                        }
                        err = FSWrite(dst_ref, &count, *buf);
                        if (err != noErr)
                            break;
                        if (rd_err != noErr && rd_err != eofErr) {
                            err = rd_err;
                            break;
                        }
                        file_len -= count;
                    }
                    HUnlock(buf);
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
force_hdelete(FSSpec *spec)
{
    FSpRstFLock(spec);
    FSpDelete(spec);
}

void
process_openfile(FSSpec *src, OSType ftype)
{
    OSErr err = noErr;
    FSSpec dst;

    if (ftype != SAVE_TYPE)
        return; /* only deal with save files */

    /* same name, but in the data (game) directory */
    mac_fsspec(&dst, theDirs.dataRefNum, theDirs.dataDirID, src->name);

    if (src->vRefNum != theDirs.dataRefNum
        || src->parID != theDirs.dataDirID) {
        /* FSpCatMove's dst names the target directory itself */
        FSSpec dstdir;

        mac_fsspec(&dstdir, theDirs.dataRefNum, theDirs.dataDirID,
                   P_STRING_CONV(":"));
        if (FSpCatMove(src, &dstdir) != noErr) {
            FSpCreate(&dst, MAC_CREATOR, SAVE_TYPE, smSystemScript);
            err = copy_file(src, &dst, false);
            if (err == noErr)
                err = copy_file(src, &dst, true);
            if (err == noErr)
                force_hdelete(src);
            else
                FSpDelete(&dst);
        }
    }

    if (err == noErr) {
        short ref;

        ref = FSpOpenResFile(&dst, fsRdPerm);
        if (ref != -1) {
            Handle name = Get1Resource('STR ', PLAYER_NAME_RES_ID);
            if (name) {
                Str255 save_f_p;
                /* the resource comes from the save file; clamp it so a
                   corrupt/foreign string can't overflow plname[PL_NSIZ] */
                if ((*(StringHandle) name)[0] > PL_NSIZ - 1)
                    (*(StringHandle) name)[0] = PL_NSIZ - 1;
                P2C(*(StringHandle) name, svp.plname);
                set_savefile_name(TRUE);
                C2P(fqname(gs.SAVEF, SAVEPREFIX, 0), save_f_p);
                {
                    FSSpec oldsave;

                    mac_fsspec(&oldsave, theDirs.dataRefNum,
                               theDirs.dataDirID, save_f_p);
                    force_hdelete(&oldsave);
                }
                if (FSpRename(&dst, save_f_p) == noErr)
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
        EventRecord event;
        long toWhen = TickCount() + 20; /* ~1/3 sec to collect initial Apple Events */

        while (TickCount() < toWhen) {
            if (WaitNextEvent(highLevelEventMask, &event, 3L, 0)) {
                AEProcessAppleEvent(&event);
                if (macFlags.gotOpen)
                    break;
            }
        }
    }
}

/* validate wizard mode if player has requested access to it */
boolean
authorize_wizard_mode(void)
{
    return TRUE;
}

boolean
authorize_explore_mode(void)
{
    return TRUE;
}

void
get_nhuuid(void)
{
    uchar bytes[16];
    int i;

    if (svn.nhuuid[0])
        return;

    for (i = 0; i < 16; i++)
        bytes[i] = (uchar) rn2(256);
    /* RFC 4122: version=4 (random), variant=10. */
    bytes[6] = (bytes[6] & 0x0F) | 0x40;
    bytes[8] = (bytes[8] & 0x3F) | 0x80;

    Snprintf(svn.nhuuid, sizeof svn.nhuuid,
             "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-"
             "%02x%02x%02x%02x%02x%02x",
             bytes[0], bytes[1], bytes[2], bytes[3],
             bytes[4], bytes[5],
             bytes[6], bytes[7],
             bytes[8], bytes[9],
             bytes[10], bytes[11], bytes[12], bytes[13],
             bytes[14], bytes[15]);
}

void
free_nhuuid(void)
{
    int i;

    for (i = 0; i < SIZE(svn.nhuuid); i++)
        svn.nhuuid[i] = 0;
}

/*macmain.c*/
