/* NetHack 3.6	winext.h	$NHDT-Date: 1432512794 2015/05/25 00:13:14 $  $NHDT-Branch: master $:$NHDT-Revision: 1.7 $ */
/* Copyright (c) Gregg Wonderly, Naperville, Illinois,  1991,1992,1993. */
/* NetHack may be freely redistributed.  See license for details. */

extern int reclip;

#ifdef CLIPPING
extern int clipping;
extern int clipx;
extern int clipy;
extern int clipxmax;
extern int clipymax;
extern int xclipbord, yclipbord;
#endif

extern int CO;
extern int LI;
extern int scrollmsg;
extern int alwaysinvent;

extern unsigned short amii_defpens[20];
extern struct amii_DisplayDesc
    *amiIDisplay; /* the Amiga Intuition descriptor */
extern struct window_procs amii_procs;
extern struct window_procs amiv_procs;
/* Three similarly-named palette arrays.  Note the position of the
 * second underscore distinguishes them:
 *   amii_initmap  = working/runtime palette (mutated by tile/tomb load
 *                   and the in-game color editor).
 *   amii_init_map = AMII (text-mode) compile-time defaults, 8 entries.
 *   amiv_init_map = AMIV (tile-mode) compile-time defaults, 32 entries
 *                   (mutated by ReadImageFile when a tile/tomb IFF
 *                   carries its own CMAP).
 * The naming is historical; sysflags.amii_curmap is yet another related
 * array holding the user's saved color choices.
 */
extern unsigned short amii_initmap[AMII_MAXCOLORS];
extern unsigned short amiv_init_map[AMII_MAXCOLORS];
extern unsigned short amii_init_map[AMII_MAXCOLORS];
extern int bigscreen;
extern int amii_numcolors;
extern long amii_scrnmode;
extern winid amii_rawprwin;
extern struct Screen *HackScreen;
extern char Initialized;
extern int amii_msgAPen;
extern int amii_msgBPen;
extern int amii_statAPen;
extern int amii_statBPen;
extern int amii_menuAPen;
extern int amii_menuBPen;
extern int amii_textAPen;
extern int amii_textBPen;
extern int amii_otherAPen;
extern int amii_otherBPen;
/* All kinds of shared stuff */
extern struct TextAttr Hack160;
extern struct TextAttr Hack40;
extern struct TextAttr Hack80;
extern struct TextAttr TextsFont13;
extern struct Window *pr_WindowPtr;
extern struct Menu HackMenu[];
extern struct Menu *MenuStrip;
extern struct NewMenu GTHackMenu[];
extern APTR *VisualInfo;
extern int KbdBuffered;
extern struct TextFont *TextsFont;
extern struct TextFont *HackFont;
extern struct IOStdReq ConsoleIO;
extern struct MsgPort *HackPort;

extern int txwidth, txheight, txbaseline;

/* This gadget data is replicated for menu/text windows... */
extern struct PropInfo PropScroll;
extern struct Image Image1;
extern struct Gadget MenuScroll;

/* This gadget is for the message window... */
extern struct PropInfo MsgPropScroll;
extern struct Image MsgImage1;
extern struct Gadget MsgScroll;

extern struct TagItem tags[];

extern struct win_setup {
    struct NewWindow newwin;
    UWORD offx, offy, maxrow, rows, maxcol, cols; /* CHECK TYPES */
} new_wins[];

extern UWORD scrnpens[];
/* The last Window event is stored here for reference. */
extern WEVENT lastevent;
extern const char winpanicstr[];
extern struct TagItem scrntags[];
extern struct NewScreen NewHackScreen;

extern int topl_addspace;
extern char spaces[76];
extern int wincnt; /* # of nh windows opened */
extern struct Rectangle lastinvent, lastmsg;

typedef struct {
    UWORD w, h;
    WORD x, y;
    UBYTE nPlanes;
    UBYTE masking;
    UBYTE compression;
    UBYTE reserved1;
    UWORD transparentColor;
    UBYTE xAspect, yAspect;
    WORD pageWidth, pageHeight;
} BitMapHeader;

typedef enum {
    COL_MAZE_BRICK,
    COL_MAZE_STONE,
    COL_MAZE_HEAT,
    COL_MAZE_WOOD
} MazeType;
extern struct PDAT pictdata;
extern struct Hook fillhook;
extern struct TagItem wintags[];
#ifdef __PPC__
struct EmulLibEntry LayerFillHook;
#else
void LayerFillHook(void);
#endif
extern int mxsize, mysize;
