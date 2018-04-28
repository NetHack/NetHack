/* NetHack 3.6	macwin.h	$NHDT-Date: 1447755970 2015/11/17 10:26:10 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Kevin Hugo, 2003. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MACWIN_H
#define MACWIN_H
#undef red /* undef internal color const strings from decl */
#undef green
#undef blue

#ifndef __MACH__
#include <windows.h>
#include <dialogs.h>
#endif

/* more headers */
#ifdef THINK_C
#include <pascal.h> /* for CtoPStr and PtoCStr */
#endif

/* resources */
#define PLAYER_NAME_RES_ID 1001

/* fake some things if we don't have universal headers.. */
#if 0 /*ndef NewUserItemProc*/
typedef pascal void (*UserItemProcPtr)(WindowPtr theWindow, short itemNo);
typedef UserItemProcPtr UserItemUPP;
#define NewUserItemProc(p) (UserItemUPP)(p)

typedef pascal void (*ControlActionProcPtr)(ControlHandle theControl,
                                            short partCode);
typedef ControlActionProcPtr ControlActionUPP;
#define NewControlActionProc(p) (ControlActionUPP)(p)

typedef ModalFilterProcPtr ModalFilterUPP;
#define DisposeRoutineDescriptor(p)
#endif

/* misc */
#ifdef __MWERKS__
#define ResumeProcPtr long /* for call to InitDialogs */
#endif

/* working dirs structure */
typedef struct macdirs {
    Str32 dataName;
    short dataRefNum;
    long dataDirID;

    Str32 saveName;
    short saveRefNum;
    long saveDirID;

    Str32 levelName;
    short levelRefNum;
    long levelDirID;
} MacDirs;

typedef struct macflags {
    Bitfield(processes, 1);
    Bitfield(color, 1);
    Bitfield(folders, 1);
    Bitfield(tempMem, 1);
    Bitfield(help, 1);
    Bitfield(fsSpec, 1);
    Bitfield(trueType, 1);
    Bitfield(aux, 1);
    Bitfield(alias, 1);
    Bitfield(standardFile, 1);
    Bitfield(hasDebugger, 1);
    Bitfield(hasAE, 1);
    Bitfield(gotOpen, 1);
} MacFlags;

extern MacDirs theDirs; /* used in macfile.c */
extern MacFlags macFlags;

/*
 * Mac windows
 */
#define NUM_MACWINDOWS 15
#define TEXT_BLOCK 512L

/* Window constants */
#define kMapWindow 0
#define kStatusWindow 1
#define kMessageWindow 2
#define kTextWindow 3
#define kMenuWindow 4
#define kLastWindowKind kMenuWindow

/*
 * This determines the minimum logical line length in text windows
 * That is; even if physical width is less, this is where line breaks
 * go at the minimum. 350 is about right for score lines with a
 * geneva 10 pt font.
 */
#define MIN_RIGHT 350

typedef struct {
    anything id;
    char accelerator;
    char groupAcc;
    short line;
} MacMHMenuItem;

typedef struct NhWindow {
    WindowPtr its_window;

    short font_number;
    short font_size;
    short char_width;
    short row_height;
    short ascent_height;

    short x_size;
    short y_size;
    short x_curs;
    short y_curs;

    short last_more_lin; /* Used by message window */
    short save_lin;      /* Used by message window */

    short miSize;             /* size of menu items arrays */
    short miLen;              /* number of menu items in array */
    MacMHMenuItem **menuInfo; /* Used by menus (array handle) */
    char menuChar;            /* next menu accelerator to use */
    short **menuSelected;     /* list of selected elements from list */
    short miSelLen;           /* number of items selected */
    short how;                /* menu mode */

    char drawn;
    Handle windowText;
    long windowTextLen;
    short scrollPos;
    ControlHandle scrollBar;
} NhWindow;

extern Boolean CheckNhWin(WindowPtr mac_win);

#define NUM_STAT_ROWS 2
#define NUM_ROWS 22
#define NUM_COLS 80 /* We shouldn't use column 0 */
#define QUEUE_LEN 24

extern NhWindow *theWindows;

extern struct window_procs mac_procs;

#define NHW_BASE 0
extern winid BASE_WINDOW, WIN_MAP, WIN_MESSAGE, WIN_INVEN, WIN_STATUS;

/*
 * External declarations for the window routines.
 */

#define E extern

/* ### dprintf.c ### */

extern void dprintf(char *, ...);

/* ### maccurs.c ### */

extern Boolean RetrievePosition(short, short *, short *);
extern Boolean RetrieveSize(short, short, short, short *, short *);
extern void SaveWindowPos(WindowPtr);
extern void SaveWindowSize(WindowPtr);
extern Boolean FDECL(RetrieveWinPos, (WindowPtr, short *, short *));

/* ### macerrs.c ### */

extern void showerror(char *, const char *);
extern Boolean itworked(short);
extern void mustwork(short);
extern void attemptingto(char *);
/* appear to be unused
extern void comment(char *,long);
extern void pushattemptingto(char *);
extern void popattempt(void);
*/
/* ### macfile.c ### */

/* extern char *macgets(int fd, char *ptr, unsigned len); unused */
extern void FDECL(C2P, (const char *c, unsigned char *p));
extern void FDECL(P2C, (const unsigned char *p, char *c));

/* ### macmenu.c ### */

extern void DoMenuEvt(long);
extern void InitMenuRes(void);
extern void AdjustMenus(short);
#define DimMenuBar() AdjustMenus(1)
#define UndimMenuBar() AdjustMenus(0)

/* ### macmain.c ### */

extern void FDECL(process_openfile,
                  (short s_vol, long s_dir, Str255 fNm, OSType ft));

/* ### macwin.c ### */

extern void AddToKeyQueue(unsigned char, Boolean);
extern unsigned char GetFromKeyQueue(void);
void trans_num_keys(EventRecord *);
extern void NDECL(InitMac);
int FDECL(try_key_queue, (char *));
void FDECL(enter_topl_mode, (char *));
void FDECL(leave_topl_mode, (char *));
void FDECL(topl_set_resp, (char *, char));
Boolean FDECL(topl_key, (unsigned char, Boolean));
E void FDECL(HandleEvent, (EventRecord *)); /* used in mmodal.c */
extern void NDECL(port_help);

extern Boolean small_screen;

E void FDECL(mac_init_nhwindows, (int *, char **));
E void NDECL(mac_askname);
E void NDECL(mac_get_nh_event);
E void FDECL(mac_exit_nhwindows, (const char *));
E winid FDECL(mac_create_nhwindow, (int));
E void FDECL(mac_clear_nhwindow, (winid));
E void FDECL(mac_display_nhwindow, (winid, BOOLEAN_P));
E void FDECL(mac_destroy_nhwindow, (winid));
E void FDECL(mac_curs, (winid, int, int));
E void FDECL(mac_putstr, (winid, int, const char *));
E void FDECL(mac_start_menu, (winid));
E void FDECL(mac_add_menu, (winid, int, const anything *, CHAR_P, CHAR_P, int,
                            const char *, BOOLEAN_P));
E void FDECL(mac_end_menu, (winid, const char *));
E int FDECL(mac_select_menu, (winid, int, menu_item **));
#ifdef CLIPPING
E void FDECL(mac_cliparound, (int, int));
#endif
E int NDECL(mac_nhgetch);
E int FDECL(mac_nh_poskey, (int *, int *, int *));
E int NDECL(mac_doprev_message);
E char FDECL(mac_yn_function, (const char *, const char *, CHAR_P));
E void FDECL(mac_getlin, (const char *, char *));
E int NDECL(mac_get_ext_cmd);
E void FDECL(mac_number_pad, (int));
E void NDECL(mac_delay_output);

#undef E

#endif /* ! MACWIN_H */
