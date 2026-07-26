/* NetHack 5.0	macmenu.c	$NHDT-Date: 1432512797 2015/05/25 00:13:17 $  $NHDT-Branch: master $:$NHDT-Revision: 1.13 $ */
/*      Copyright (c) Macintosh NetHack Port Team, 1993.          */
/* NetHack may be freely redistributed.  See license for details. */

/****************************************\
 * Extended Macintosh menu support
 *
 * provides access to keyboard commands from cmd.c via logical menu groups
 * supports various menu highlighting modes
\****************************************/

/****************************************\
 * Edit History:
 *
 * 930512	- More bug fixes and getting tty to work again, Jon W{tte
 * 930508	- Bug fixes in-flight, Jon W{tte
 * 04/29/93 - 1st Release Draft, David Hairston
 * 04/11/93 - 1st Draft, David Hairston
\****************************************/

/******** Application Defines ********/
#include "hack.h"
#include "mactty.h"
#include "macwin.h"
#include "patchlevel.h"
#include "mactile.h"
#include "macmap.h"

/* Set to 1 by macwin's resume handler when tile-mode availability changes;
   cleared by mactile_menu_refresh() on the next idle pass. */
short gTileMenuNeedsUpdate = 0;

/******** Toolbox Defines ********/
#include <Menus.h>
#include <Devices.h>
#include <Resources.h>
#include <TextUtils.h>
#include <ToolUtils.h>
#include <Sound.h>

/* Borrowed from the Mac tty port */
extern WindowPtr _mt_window;

/******** Local Defines ********/

/* 'MNU#' (menu list record) */
typedef union menuRefUnn {
    short mresID;    /* MENU resource ID (before GetMenu) */
    MenuHandle mhnd; /* MENU handle (after GetMenu) */
} menuRefUnn;

typedef struct menuListRec {
    short firstMenuID;
    short numMenus;
    menuRefUnn mref[];
} menuListRec, *menuListPtr, **menuListHandle;

/* indices and resource IDs of the menu list data */
enum {
    listMenubar,
    listSubmenu,

    menuBarListID = 128,
    subMenuListID
};

/* the following mref[] indices are reserved */
enum {
    /* menu bar */
    menuApple,
    menuFile,
    menuEdit,
    menuGame,

    /* submenu */
    menuWizard = 0,
    menuPlayModeSub = 2 /* submenu list is wizard(0), current(1),
                           Play Mode(2) -- see MNU# 129 in nhmenus.r */
};

/* the following menu items are reserved */
enum {
    /* apple */
    menuAppleAboutBox = 1,
    ____Apple__1,

    /* File */
    menuFileSave = 1,
    ____File___1,
    menuFileQuit,

    /* Edit: the standard items are for desk accessories only;
       Preferences... is ours (HIG home for it) */
    menuEditUndo = 1,
    ____Edit___1,
    menuEditCut,
    menuEditCopy,
    menuEditPaste,
    menuEditClear,
    ____Edit___2,
    menuEditPrefs,

    /* Game */
    menuGameRedraw = 1,
    menuGamePrevMsg,
    menuGameReposition,
    menuGameTileMode,
    ____Game___1,
    menuGamePlayMode,

    /* Play Mode submenu */
    playModeRegular = 1,
    playModeExplore,
    playModeDebug,

    /* Wizard */
    menuWizardAttributes = 1
};

/*
 * menuListRec fields (preloaded and locked from 'MNU#' 128 menubar / 129 submenus):
 * firstMenuID - menu ID of the 1st menu; subsequent menus are _forced_ to
 *     consecutively incremented IDs.
 * numMenus - count of menus in the list.
 * mref[] - holds the MENU resource ID until GetResource, then the menu handle.
 *
 * WARNING: the submenu list record must exist even with zero submenus, and no
 * bounds checking is done on menu IDs.
 */

#define ID1_MBAR pMenuList[listMenubar]->firstMenuID
#define ID1_SUBM pMenuList[listSubmenu]->firstMenuID

#define NUM_MBAR pMenuList[listMenubar]->numMenus
#define NUM_SUBM pMenuList[listSubmenu]->numMenus

#define MHND_APPLE pMenuList[listMenubar]->mref[menuApple].mhnd
#define MHND_FILE pMenuList[listMenubar]->mref[menuFile].mhnd
#define MHND_EDIT pMenuList[listMenubar]->mref[menuEdit].mhnd

#define MBARHND(x) pMenuList[listMenubar]->mref[(x)].mhnd

#define MHND_GAME MBARHND(menuGame)

#define MHND_WIZ pMenuList[listSubmenu]->mref[menuWizard].mhnd
#define MHND_PLAYMODE pMenuList[listSubmenu]->mref[menuPlayModeSub].mhnd

/* mutually exclusive (and prioritized) menu bar states */
enum {
    mbarDim,
    mbarNoWindows,
    mbarDA,
    mbarNoMap,
    mbarRegular,
    mbarSpecial /* explore or debug mode */
};

/* menu routine error numbers */
enum {
    errGetMenuList,
    errGetMenu,
    errGetANDlogTemplate,
    errGetANDlogItems,
    errGetANDialog,
    errANNewMenu,
    err_Menu_total
};

/* menu 'STR#' comment char */
#define mstrEndChar 0xA5 /* '\245' or option-* or "bullet" */

/* 'ALRT' */
enum {
    alrt_Menu_start = 5000,
    alrtMenuNote = alrt_Menu_start,
    alrtMenu_NY,
    alrt_Menu_limit
};

#define beepMenuAlertErr 1 /* # of SysBeep()'s before exitting */
enum { bttnMenuAlertNo = 1, bttnMenuAlertYes };

/******** Globals ********/
static unsigned char *menuErrStr[err_Menu_total] = {
    P_STRING_CONV("Abort: Bad 'MNU#' resource!"), /* errGetMenuList */
    P_STRING_CONV("Abort: Bad 'MENU' resource!"), /* errGetMenu */
    P_STRING_CONV("Abort: Bad 'DLOG' resource!"), /* errGetANDlogTemplate */
    P_STRING_CONV("Abort: Bad 'DITL' resource!"), /* errGetANDlogItems */
    P_STRING_CONV("Abort: Bad Dialog Allocation!"), /* errGetANDialog */
    P_STRING_CONV("Abort: Bad Menu Allocation!"),   /* errANNewMenu */
};
static menuListPtr pMenuList[2];
static short theMenubar = mbarDA; /* force initial update */
static short kWizMenuPruneNeeded = 1; /* one-shot: explore-mode pruning */

/******** Prototypes ********/
static void alignAD(Rect *, short);
static void mustGetMenuAlerts(void);
static void menuError(short);
static void aboutNetHack(void);
static void askSave(void);
static void askQuit(void);

/*** Askname dialog box ***/

#define RSRC_ASK 6000      /* Askname dialog and item list */
#define RSRC_ASK_PLAY 1    /*	Play button */
#define RSRC_ASK_QUIT 2    /*	Quit button */
#define RSRC_ASK_DEFAULT 3 /*	Default ring */
#define RSRC_ASK_ROLE 4    /*	Role popup menu */
#define RSRC_ASK_RACE 5    /*	Race popup menu */
#define RSRC_ASK_GEND 6    /*	Gender popup menu */
#define RSRC_ASK_ALIGN 7   /*	Alignment popup menu */
#define RSRC_ASK_MODE 8    /*	Mode popup menu */
#define RSRC_ASK_NAME 9    /*	Name text field */
#define RSRC_ASK_MAX 10    /*	Maximum enabled item */

#define KEY_MASK 0xff00
#define KEY_RETURN 0x2400
#define KEY_ENTER 0x4c00
#define KEY_ESCAPE 0x3500
#define CH_MASK 0x00ff
#define CH_RETURN 0x000d
#define CH_ENTER 0x0003
#define CH_ESCAPE 0x001b

static void ask_restring(const char *cstr, unsigned char *pstr);
static void ask_enable(DialogRef wind, short item, int enable);
static pascal void ask_redraw(DialogRef wind, DialogItemIndex item);
static pascal Boolean
ask_filter(DialogRef wind, EventRecord *event, DialogItemIndex *item);
#define noresource(t, n) \
    {                    \
        SysBeep(3);      \
        ExitToShell();   \
    }
#define fatal(s)       \
    {                  \
        SysBeep(3);    \
        ExitToShell(); \
    }

static MenuHandle askmenu[RSRC_ASK_MAX];
static int askselect[RSRC_ASK_MAX];
#define currrole askselect[RSRC_ASK_ROLE]
#define currrace askselect[RSRC_ASK_RACE]
#define currgend askselect[RSRC_ASK_GEND]
#define curralign askselect[RSRC_ASK_ALIGN]
#define currmode askselect[RSRC_ASK_MODE]

/* Dialog chrome colors for ask_redraw's RGBForeColor calls.  Safe from the
   8bpp CLUT-rewrite hazard only because the askname dialog runs before the
   tile palette is loaded; if this dialog is ever shown mid-game, switch to
   palette-safe drawing (PmForeColor, see mactile_cursor_clut_index). */
static RGBColor blackcolor = { 0x0000, 0x0000, 0x0000 },
    darkcolor = { 0x8000, 0x8000, 0x8000 },
                backcolor = { 0xdddd, 0xdddd, 0xdddd },
                lightcolor = { 0xffff, 0xffff, 0xffff },
                whitecolor = { 0xffff, 0xffff, 0xffff };

/* Convert a mixed-case C string to a Capitalized Pascal string */
static void
ask_restring(const char *cstr, unsigned char *pstr)
{
    int i;

    for (i = 0; *cstr && (i < 255); i++)
        pstr[i + 1] = *cstr++;
    pstr[0] = i;
    if ((pstr[1] >= 'a') && (pstr[1] <= 'z'))
        pstr[1] += 'A' - 'a';
    return;
}

/* Enable the dialog item with the given index */
static void
ask_enable(DialogRef wind, short item, int enable)
{
    short type;
    Handle handle;
    Rect rect;

    GetDialogItem(wind, item, &type, &handle, &rect);
    if (enable)
        type &= ~itemDisable;
    else
        type |= itemDisable;
    HiliteControl((ControlHandle) handle, enable ? 0 : 255);
    SetDialogItem(wind, item, type, handle, &rect);
    return;
}

static pascal void
ask_redraw(DialogRef wind, DialogItemIndex item)
{
    short type;
    Handle handle;
    Rect rect;
    static char *modechar = "NED";

    GetDialogItem(wind, item, &type, &handle, &rect);
    switch (item) {
    case RSRC_ASK_DEFAULT:
        PenSize(3, 3);
        FrameRoundRect(&rect, 16, 16);
        break;

    case RSRC_ASK_ROLE:
    case RSRC_ASK_RACE:
    case RSRC_ASK_GEND:
    case RSRC_ASK_ALIGN:
    case RSRC_ASK_MODE:
        if (macFlags.color) {
            RGBForeColor(&blackcolor);
            RGBBackColor(&backcolor);
        }
        PenNormal();
        TextMode(srcOr);
        EraseRect(&rect);

        /* frame and drop shadow */
        rect.right--;
        rect.bottom--;
        FrameRect(&rect);
        MoveTo(rect.right, rect.top + 1);
        LineTo(rect.right, rect.bottom);
        LineTo(rect.left + 1, rect.bottom);

        /* menu character */
        MoveTo(rect.left + 4, rect.top + 12);
        switch (item) {
        case RSRC_ASK_ROLE:
            DrawText(roles[askselect[item]].filecode, 0, 3);
            break;
        case RSRC_ASK_RACE:
            DrawText(races[askselect[item]].filecode, 0, 3);
            break;
        case RSRC_ASK_GEND:
            DrawText(genders[askselect[item]].filecode, 0, 3);
            break;
        case RSRC_ASK_ALIGN:
            DrawText(aligns[askselect[item]].filecode, 0, 3);
            break;
        case RSRC_ASK_MODE:
            DrawChar(modechar[askselect[item]]);
            break;
        }

        /* popup symbol */
        MoveTo(rect.right - 16, rect.top + 5);
        LineTo(rect.right - 6, rect.top + 5);
        LineTo(rect.right - 11, rect.top + 10);
        LineTo(rect.right - 15, rect.top + 6);
        LineTo(rect.right - 8, rect.top + 6);
        LineTo(rect.right - 11, rect.top + 9);
        LineTo(rect.right - 13, rect.top + 7);
        LineTo(rect.right - 10, rect.top + 7);
        LineTo(rect.right - 11, rect.top + 8);

        /* shadow */
        InsetRect(&rect, 1, 1);
        if (macFlags.color) {
            RGBColor color;

            GetForeColor(&color);

            /* top and left */
            RGBForeColor(&lightcolor);
            MoveTo(rect.left, rect.bottom - 1);
            LineTo(rect.left, rect.top);
            LineTo(rect.right - 1, rect.top);

            /* bottom and right */
            RGBForeColor(&darkcolor);
            MoveTo(rect.right - 1, rect.top + 1);
            LineTo(rect.right - 1, rect.bottom - 1);
            LineTo(rect.left + 1, rect.bottom - 1);

            RGBForeColor(&color);
        }
        break;

    case RSRC_ASK_NAME:
        PenNormal();
        if (macFlags.color) {
            RGBForeColor(&whitecolor);
            RGBBackColor(&whitecolor);
            TextMode(srcOr);
        } else {
            PenMode(notPatCopy);
            TextMode(srcBic);
        }
        InsetRect(&rect, -1, -1);
        FrameRect(&rect);
        InsetRect(&rect, -1, -1);
        FrameRect(&rect);
        InsetRect(&rect, -2, -2);
        if (macFlags.color) {
            /* top and left */
            RGBForeColor(&darkcolor);
            MoveTo(rect.left, rect.bottom - 1);
            LineTo(rect.left, rect.top);
            LineTo(rect.right - 1, rect.top);

            /* bottom and right */
            RGBForeColor(&lightcolor);
            MoveTo(rect.right - 1, rect.top + 1);
            LineTo(rect.right - 1, rect.bottom - 1);
            LineTo(rect.left + 1, rect.bottom - 1);

            RGBForeColor(&blackcolor);
            RGBBackColor(&backcolor);
        }
        break;
    }
    return;
}

static pascal Boolean
ask_filter(DialogRef wind, EventRecord *event, DialogItemIndex *item)
{
    short ch, key;

    switch (event->what) {
    case keyDown:
    case autoKey:
        ch = event->message & CH_MASK;
        key = event->message & KEY_MASK;
        /* Handle equivalents for OK */
        if ((ch == CH_RETURN) || (key == KEY_RETURN) || (ch == CH_ENTER)
            || (key == KEY_ENTER)) {
            if (GetDialogTextEditHandle(wind)[0]->teLength) {
                FlashButton(wind, RSRC_ASK_PLAY);
                *item = RSRC_ASK_PLAY;
            } else
                *item = 0;
            return (TRUE);
        }
        /* Handle equivalents for Normal/Explore/Debug */
        if ((event->modifiers & cmdKey) && (ch == 'n')) {
            currmode = 0;
            ask_redraw(wind, RSRC_ASK_MODE);
            *item = RSRC_ASK_MODE;
            return (TRUE);
        }
        if ((event->modifiers & cmdKey) && (ch == 'e')) {
            currmode = 1;
            ask_redraw(wind, RSRC_ASK_MODE);
            *item = RSRC_ASK_MODE;
            return (TRUE);
        }
        if ((event->modifiers & cmdKey) && (ch == 'd')) {
            currmode = 2;
            ask_redraw(wind, RSRC_ASK_MODE);
            *item = RSRC_ASK_MODE;
            return (TRUE);
        }
        /* Handle equivalents for Cancel and Quit */
        if ((ch == CH_ESCAPE) || (key == KEY_ESCAPE)
            || ((event->modifiers & cmdKey) && (ch == 'q'))
            || ((event->modifiers & cmdKey) && (ch == '.'))) {
            FlashButton(wind, RSRC_ASK_QUIT);
            *item = RSRC_ASK_QUIT;
            return (TRUE);
        }
        return (FALSE);
    case updateEvt:
        ask_redraw(wind, RSRC_ASK_NAME);
        return (FALSE);
    default:
        return (FALSE);
    }
}

void
mac_askname(void)
{
    GrafPtr oldport;
    DialogRef askdialog;
    short i, j, item, type;
    Handle handle;
    Rect rect;
    Str255 str;
    Point pt;
    UserItemUPP redraw = NewUserItemUPP(ask_redraw);
    ModalFilterUPP filter = NewModalFilterUPP(ask_filter);

    if (!(askdialog = GetNewDialog(RSRC_ASK, NULL, (WindowRef) -1)))
        noresource('DLOG', RSRC_ASK);
    GetPort(&oldport);
    SetPortDialogPort(askdialog);

    ask_restring(svp.plname, str);
    if (svp.plname[0]) {
        GetDialogItem(askdialog, RSRC_ASK_NAME, &type, &handle, &rect);
        SetDialogItemText(handle, str);
    }
    SelectDialogItemText(askdialog, RSRC_ASK_NAME, 0, 32767);

    /* Initialize the role popup menu */
    if (!(askmenu[RSRC_ASK_ROLE] = NewMenu(RSRC_ASK_ROLE, P_EMPTY_STRING)))
        fatal("Cannot create role menu");
    for (i = 0; roles[i].name.m; i++) {
        ask_restring(roles[i].name.m, str);
        AppendMenu(askmenu[RSRC_ASK_ROLE], str);
    }
    InsertMenu(askmenu[RSRC_ASK_ROLE], hierMenu);
    if (flags.initrole >= 0)
        currrole = flags.initrole;
    /* Check for backward compatibility */
    else if ((currrole = str2role(svp.pl_character)) < 0)
        currrole = randrole(FALSE);

    /* Initialize the race popup menu */
    if (!(askmenu[RSRC_ASK_RACE] = NewMenu(RSRC_ASK_RACE, P_EMPTY_STRING)))
        fatal("Cannot create race menu");
    for (i = 0; races[i].noun; i++) {
        ask_restring(races[i].noun, str);
        AppendMenu(askmenu[RSRC_ASK_RACE], str);
    }
    InsertMenu(askmenu[RSRC_ASK_RACE], hierMenu);
    if (flags.initrace >= 0)
        currrace = flags.initrace;
    else
        currrace = randrace(currrole);

    /* Initialize the gender popup menu */
    if (!(askmenu[RSRC_ASK_GEND] = NewMenu(RSRC_ASK_GEND, P_EMPTY_STRING)))
        fatal("Cannot create gender menu");
    for (i = 0; i < ROLE_GENDERS; i++) {
        ask_restring(genders[i].adj, str);
        AppendMenu(askmenu[RSRC_ASK_GEND], str);
    }
    InsertMenu(askmenu[RSRC_ASK_GEND], hierMenu);
    if (flags.initgend >= 0)
        currgend = flags.initgend;
    else if (flags.female)
        currgend = 1;
    else
        currgend = randgend(currrole, currrace);

    /* Initialize the alignment popup menu */
    if (!(askmenu[RSRC_ASK_ALIGN] = NewMenu(RSRC_ASK_ALIGN, P_EMPTY_STRING)))
        fatal("Cannot create alignment menu");
    for (i = 0; i < ROLE_ALIGNS; i++) {
        ask_restring(aligns[i].adj, str);
        AppendMenu(askmenu[RSRC_ASK_ALIGN], str);
    }
    InsertMenu(askmenu[RSRC_ASK_ALIGN], hierMenu);
    if (flags.initalign >= 0)
        curralign = flags.initalign;
    else
        curralign = randalign(currrole, currrace);

    /* Initialize the mode popup menu */
    if (!(askmenu[RSRC_ASK_MODE] = NewMenu(RSRC_ASK_MODE, P_EMPTY_STRING)))
        fatal("Cannot create mode menu");
    AppendMenu(askmenu[RSRC_ASK_MODE], P_STRING_CONV("Normal"));
    AppendMenu(askmenu[RSRC_ASK_MODE], P_STRING_CONV("Explore"));
    AppendMenu(askmenu[RSRC_ASK_MODE], P_STRING_CONV("Debug"));
    InsertMenu(askmenu[RSRC_ASK_MODE], hierMenu);
    currmode = 0;

    /* install per-item redraw procs */
    for (item = RSRC_ASK_DEFAULT; item <= RSRC_ASK_MODE; item++) {
        GetDialogItem(askdialog, item, &type, &handle, &rect);
        SetDialogItem(askdialog, item, type, (Handle) redraw, &rect);
    }

    do {
        /* Adjust the Play button */
        ask_enable(askdialog, RSRC_ASK_PLAY,
                   GetDialogTextEditHandle(askdialog)[0]->teLength);

        /* Adjust the race popup menu */
        i = j = currrace;
        do {
            if (validrace(currrole, j)) {
                EnableItem(askmenu[RSRC_ASK_RACE], j + 1);
                CheckMenuItem(askmenu[RSRC_ASK_RACE], j + 1, currrace == j);
            } else {
                DisableItem(askmenu[RSRC_ASK_RACE], j + 1);
                CheckMenuItem(askmenu[RSRC_ASK_RACE], j + 1, FALSE);
                if ((currrace == j) && !races[++currrace].noun)
                    currrace = 0;
            }
            if (!races[++j].noun)
                j = 0;
        } while (i != j);
        if (currrace != i) {
            GetDialogItem(askdialog, RSRC_ASK_RACE, &type, &handle, &rect);
            InvalWindowRect(GetDialogWindow(askdialog), &rect);
        }

        /* Adjust the gender popup menu */
        i = j = currgend;
        do {
            if (validgend(currrole, currrace, j)) {
                EnableItem(askmenu[RSRC_ASK_GEND], j + 1);
                CheckMenuItem(askmenu[RSRC_ASK_GEND], j + 1, currgend == j);
            } else {
                DisableItem(askmenu[RSRC_ASK_GEND], j + 1);
                CheckMenuItem(askmenu[RSRC_ASK_GEND], j + 1, FALSE);
                if ((currgend == j) && (++currgend >= ROLE_GENDERS))
                    currgend = 0;
            }
            if (++j >= ROLE_GENDERS)
                j = 0;
        } while (i != j);
        if (currgend != i) {
            GetDialogItem(askdialog, RSRC_ASK_GEND, &type, &handle, &rect);
            InvalWindowRect(GetDialogWindow(askdialog), &rect);
        }

        /* Adjust the alignment popup menu */
        i = j = curralign;
        do {
            if (validalign(currrole, currrace, j)) {
                EnableItem(askmenu[RSRC_ASK_ALIGN], j + 1);
                CheckMenuItem(askmenu[RSRC_ASK_ALIGN], j + 1, curralign == j);
            } else {
                DisableItem(askmenu[RSRC_ASK_ALIGN], j + 1);
                CheckMenuItem(askmenu[RSRC_ASK_ALIGN], j + 1, FALSE);
                if ((curralign == j) && (++curralign >= ROLE_ALIGNS))
                    curralign = 0;
            }
            if (++j >= ROLE_ALIGNS)
                j = 0;
        } while (i != j);
        if (curralign != i) {
            GetDialogItem(askdialog, RSRC_ASK_ALIGN, &type, &handle, &rect);
            InvalWindowRect(GetDialogWindow(askdialog), &rect);
        }

        /* Adjust the role popup menu */
        for (i = 0; roles[i].name.m; i++) {
            ask_restring((currgend && roles[i].name.f) ? roles[i].name.f
                                                       : roles[i].name.m,
                         str);
            SetMenuItemText(askmenu[RSRC_ASK_ROLE], i + 1, str);
            CheckMenuItem(askmenu[RSRC_ASK_ROLE], i + 1, currrole == i);
        }

        /* Adjust the mode popup menu */
        CheckMenuItem(askmenu[RSRC_ASK_MODE], 1, currmode == 0);
        CheckMenuItem(askmenu[RSRC_ASK_MODE], 2, currmode == 1);
        CheckMenuItem(askmenu[RSRC_ASK_MODE], 3, currmode == 2);

        /* Wait for an action on an item */
        ModalDialog(filter, &item);
        switch (item) {
        case RSRC_ASK_PLAY:
            break;
        case RSRC_ASK_QUIT:
            currmode = -1;
            break;
        case RSRC_ASK_ROLE:
        case RSRC_ASK_RACE:
        case RSRC_ASK_ALIGN:
        case RSRC_ASK_GEND:
        case RSRC_ASK_MODE:
            GetDialogItem(askdialog, item, &type, &handle, &rect);
            pt.v = rect.top;   /* topLeft of rect, without type-punning */
            pt.h = rect.left;
            LocalToGlobal(&pt);
            if (!!(i = PopUpMenuSelect(askmenu[item], pt.v, pt.h,
                                       askselect[item] + 1)))
                askselect[item] = LoWord(i) - 1;
            InvalWindowRect(GetDialogWindow(askdialog), &rect);
            break;
        case RSRC_ASK_NAME:
            break;
        }
    } while ((item != RSRC_ASK_PLAY) && (item != RSRC_ASK_QUIT));

    GetDialogItem(askdialog, RSRC_ASK_NAME, &type, &handle, &rect);
    GetDialogItemText(handle, str);
    if (str[0] > PL_NSIZ - 1)
        str[0] = PL_NSIZ - 1;
    P2C(str, svp.plname);

    for (i = RSRC_ASK_ROLE; i <= RSRC_ASK_MODE; i++) {
        DeleteMenu(i);
        DisposeMenu(askmenu[i]);
    }
    SetPort(oldport);
    DisposeDialog(askdialog);
    DisposeModalFilterUPP(filter);
    DisposeUserItemUPP(redraw);

    wizard = discover = 0;
    switch (currmode) {
    case 0: /* Normal */
        break;
    case 1: /* Explore */
        discover = 1;
        break;
    case 2: /* Debug */
        wizard = 1;
        strcpy(svp.plname, WIZARD_NAME);
        break;
    default: /* Quit */
        ExitToShell();
    }

    strcpy(svp.pl_character, roles[currrole].name.m);
    flags.initrole = currrole;
    flags.initrace = currrace;
    flags.female = flags.initgend = currgend;
    flags.initalign = curralign;

    return;
}

/*** Menu bar routines ***/

static void
alignAD(Rect *pRct, short vExempt)
{
    /* center on the main screen (qd.screenBits, valid after InitGraf) */
    (*pRct).right -= (*pRct).left; /* width */
    (*pRct).bottom -= (*pRct).top; /* height */
    (*pRct).left = (qd.screenBits.bounds.right - (*pRct).right) / 2;
    (*pRct).top = (qd.screenBits.bounds.bottom - (*pRct).bottom - vExempt) / 2;
    (*pRct).top += vExempt;
    (*pRct).right += (*pRct).left;
    (*pRct).bottom += (*pRct).top;
}

static void
mustGetMenuAlerts(void)
{
    short i;
    Rect **hRct;

    for (i = alrt_Menu_start; i < alrt_Menu_limit; i++) {
        if (!(hRct = (Rect **) GetResource('ALRT', i))) /* AlertTHndl */
        {
            for (i = 0; i < beepMenuAlertErr; i++)
                SysBeep(3);
            ExitToShell();
        }

        alignAD(*hRct, GetMBarHeight());
    }
}

static void
menuError(short menuErr)
{
    short i;

    for (i = 0; i < beepMenuAlertErr; i++)
        SysBeep(3);

    ParamText(menuErrStr[menuErr], P_EMPTY_STRING, P_EMPTY_STRING, P_EMPTY_STRING);
    (void) Alert(alrtMenuNote, (ModalFilterUPP) 0L);

    ExitToShell();
}

void
InitMenuRes(void)
{
    static Boolean was_inited = 0;
    short i, j;
    menuListHandle mlHnd;
    MenuHandle menu;

    if (was_inited)
        return;
    was_inited = 1;

    mustGetMenuAlerts();

    for (i = listMenubar; i <= listSubmenu; i++) {
        if (!(mlHnd =
                  (menuListHandle) GetResource('MNU#', (menuBarListID + i))))
            menuError(errGetMenuList);

        pMenuList[i] = (menuListPtr) NewPtr(GetHandleSize((Handle) mlHnd));
        if (!pMenuList[i])
            menuError(errANNewMenu);
        *pMenuList[i] = **mlHnd;

        for (j = 0; j < pMenuList[i]->numMenus; j++) {
            if (!(menu = (MenuHandle) GetMenu((**mlHnd).mref[j].mresID))) {
                Str255 d; /* NumToString's output param is Str255 */
                NumToString((**mlHnd).mref[j].mresID, d);
                menuError(errGetMenu);
            }

            pMenuList[i]->mref[j].mhnd = menu;
            (**menu).menuID = j + (**mlHnd).firstMenuID; /* consecutive IDs */

            /* expand apple menu */
            if ((i == listMenubar) && (j == menuApple)) {
                AppendResMenu(menu, 'DRVR');
            }

            InsertMenu(menu, ((i == listSubmenu) ? hierMenu : 0));
        }
    }

    DrawMenuBar();
    return;
}

void
AdjustMenus(short dimMenubar)
{
    short newMenubar = mbarRegular;
    WindowRef win = FrontWindow();
    short i;

    /* determine the new menubar state */
    if (dimMenubar)
        newMenubar = mbarDim;
    else if (!win)
        newMenubar = mbarNoWindows;
    else if (GetWindowKind(win) < 0)
        newMenubar = mbarDA;
    else if (WIN_MAP == WIN_ERR
             || !theWindows[WIN_MAP].its_window
             || !IsWindowVisible(theWindows[WIN_MAP].its_window))
        newMenubar = mbarNoMap;

    if (newMenubar != mbarRegular)
        ; /* we've already found its state */
    else if (wizard) {
        newMenubar = mbarSpecial;
    }

    else if (discover) {
        newMenubar = mbarSpecial;

        /* one-shot: prune the wizard menu down to Attributes for explore
           mode (full menu stays in wizard mode) */
        if (kWizMenuPruneNeeded) {
            kWizMenuPruneNeeded = 0;

            for (i = CountMenuItems(MHND_WIZ); i > menuWizardAttributes; i--)
                DeleteMenuItem(MHND_WIZ, i);
        }
    }

    /* adjust the menubar, if there's a state change */
    if (theMenubar != newMenubar) {
        switch (theMenubar = newMenubar) {
        case mbarDim:
            /* disable all menus (except the apple menu) */
            for (i = menuFile; i < NUM_MBAR; i++)
                DisableItem(MBARHND(i), 0);
            break;

        case mbarNoWindows:
        case mbarDA:
        case mbarNoMap:
            /* enable the file menu (Save/Quit stay available), but ... */
            EnableItem(MHND_FILE, 0);

            /* ... disable the rest of the menus (Game included) */
            for (i = menuEdit; i < NUM_MBAR; i++)
                DisableItem(MBARHND(i), 0);

            /* Edit stays up for Preferences...; the standard items only
               work while a desk accessory is in front */
            EnableItem(MHND_EDIT, 0);
            for (i = menuEditUndo; i <= menuEditClear; i++) {
                if (theMenubar == mbarDA)
                    EnableItem(MHND_EDIT, i);
                else
                    DisableItem(MHND_EDIT, i);
            }
            EnableItem(MHND_EDIT, menuEditPrefs);

            break;

        case mbarRegular:
        case mbarSpecial:
            /* enable all menus ... */
            for (i = menuFile; i < NUM_MBAR; i++)
                EnableItem(MBARHND(i), 0);

            /* ... Edit included, for Preferences...; its standard items
               stay dim outside desk accessories */
            for (i = menuEditUndo; i <= menuEditClear; i++)
                DisableItem(MHND_EDIT, i);
            EnableItem(MHND_EDIT, menuEditPrefs);

            /* Game menu: Tile Mode enabled iff tiles available; checkmark
               tracks live state (AdjustMenus(0) runs before MenuSelect, so
               a pending transition lands before the menus pull down) */
            if (mactile_available())
                EnableItem(MHND_GAME, menuGameTileMode);
            else
                DisableItem(MHND_GAME, menuGameTileMode);
            {
                NhWindow *_am_map = (WIN_MAP != WIN_ERR)
                                    ? &theWindows[WIN_MAP] : NULL;
                SetItemMark(MHND_GAME, menuGameTileMode,
                            macmap_get_mode(_am_map) ? checkMark : noMark);
            }

            /* Play Mode submenu: checkmark on the current mode; Explore is
               the only actionable item (one-way, regular mode only) */
            SetItemMark(MHND_PLAYMODE, playModeRegular,
                        (!wizard && !discover) ? checkMark : noMark);
            SetItemMark(MHND_PLAYMODE, playModeExplore,
                        discover ? checkMark : noMark);
            SetItemMark(MHND_PLAYMODE, playModeDebug,
                        wizard ? checkMark : noMark);
            DisableItem(MHND_PLAYMODE, playModeRegular);
            DisableItem(MHND_PLAYMODE, playModeDebug);
            if (theMenubar == mbarRegular)
                EnableItem(MHND_PLAYMODE, playModeExplore);
            else
                DisableItem(MHND_PLAYMODE, playModeExplore);

            break;
        }

        DrawMenuBar();
    }
}

void
DoMenuEvt(long menuEntry)
{
    short menuID = HiWord(menuEntry);
    short menuItem = LoWord(menuEntry);

    switch (menuID - ID1_MBAR) /* all submenus are default case */
    {
    case menuApple:
        if (menuItem == menuAppleAboutBox)
            aboutNetHack();
        else {
            Str255 daName; /* GetMenuItemText may write up to 256 bytes */

            GetMenuItemText(MHND_APPLE, menuItem, daName);
            (void) OpenDeskAcc(daName);
        }
        break;

    case menuFile:
        switch (menuItem) {
        case menuFileSave:
            askSave();
            break;

        case menuFileQuit:
            askQuit();
            break;
        }
        break;

    case menuEdit:
        if (menuItem == menuEditPrefs)
            macprefs_dialog();
        else
            (void) SystemEdit(menuItem - 1);
        break;

    case menuGame:
        switch (menuItem) {
        case menuGameRedraw:
            AddToKeyQueue('R' & 0x1f, 1);
            break;

        case menuGamePrevMsg:
            AddToKeyQueue('P' & 0x1f, 1);
            break;

        case menuGameReposition:
            ResetWindowPositions();
            /* queue ^R: synchronous redraw from menu-handler context doesn't
               take (window port unsettled), so redraw in the command loop */
            AddToKeyQueue('R' & 0x1f, 1);
            break;

        case menuGameTileMode: {
            NhWindow *map = (WIN_MAP != WIN_ERR) ? &theWindows[WIN_MAP] : NULL;
            if (!map) break;
            Boolean newOn = !macmap_get_mode(map);
            if (newOn && !macmap_set_mode(map, true)) {
                SysBeep(1);
                break;
            } else if (!newOn) {
                macmap_set_mode(map, false);
            }
            SetItemMark(MHND_GAME, menuGameTileMode,
                        newOn ? checkMark : noMark);
            iflags.wc_tiled_map = newOn;
            /* no ^R needed: print_glyph keeps both caches, so set_mode's
               repaint_full_viewport + InvalWindowRect already redrew the
               map; a queued redraw would repaint every cell a second time */
            break;
        }
        }
        break;

    default: /* submenus and STR#-dispatched command menus */
        if (menuID == ID1_SUBM + menuPlayModeSub) {
            if (menuItem == playModeExplore) {
                /* queue "#exploremode" like the STR# command menus: '#'
                   opens the extended-command prompt and mac_get_ext_cmd
                   consumes the queued remainder as the answer.  (The old
                   File item queued 'X', which has meant #twoweapon since
                   3.6 -- a latent bug faithfully migrated, fixed here.) */
                const char *cmd = "#exploremode";
                short ci;

                /* all or nothing: a partial queue would run as something
                   else */
                if ((short) strlen(cmd) > KeyQueueFree()) {
                    nhbell();
                    break;
                }
                for (ci = 0; cmd[ci]; ci++)
                    AddToKeyQueue(cmd[ci], false);
            }
            break; /* exits the outer switch; Regular/Debug are status
                      indicators, never enabled as actions */
        }

    /* get associated string and add to key queue */
    {
        Str255 mstr;
        short i;

        GetIndString(mstr, menuID, menuItem);
        if (mstr[0] > QUEUE_LEN)
            mstr[0] = QUEUE_LEN;

        /* all or nothing: a partial queue would run as something else
           (mstrEndChar may stop earlier, in which case it certainly fits) */
        if ((short) mstr[0] > KeyQueueFree()) {
            nhbell();
            break;
        }
        for (i = 1; ((i <= mstr[0]) && (mstr[i] != mstrEndChar)); i++)
            AddToKeyQueue(mstr[i], false);
    } break;
    }

    HiliteMenu(0);
}

/* About dialog (DLOG/DITL 6200 in nhmenus.r): big title, a row of
   tiles when the sheet is usable, version and credits -- the shape the
   Atari/GEM port's about box has (graphic + text + OK). */
#define RSRC_ABOUT 6200
#define RSRC_ABOUT_PICT 6201 /* NETHACK sword title (nhtitle.r) */
enum {
    abtOK = 1,
    abtArt,     /* user item: title lettering + tile row */
    abtVersion, /* static text filled at runtime */
    abtCredits,
    abtContact,
    abtRing     /* bold outline around OK */
};

static pascal void
about_redraw(DialogRef dlog, DialogItemIndex item)
{
    short type;
    Handle h;
    Rect r;

    GetDialogItem(dlog, item, &type, &h, &r);
    if (item == abtRing) {
        PenSize(3, 3);
        FrameRoundRect(&r, 16, 16);
        PenSize(1, 1);
        return;
    }
    if (item != abtArt)
        return;
    {
        /* the Atari port's NETHACK sword title ('PICT' 6201),
           aspect-fit into the art area.  1-bit screens get the
           lettering instead (the dithered logo is mush there). */
        PicHandle pic = (mac_main_depth() >= 4)
                            ? GetPicture(RSRC_ABOUT_PICT) : (PicHandle) 0;

        if (pic) {
            Rect pf = (**pic).picFrame;
            Rect dst;
            long pw = pf.right - pf.left, ph = pf.bottom - pf.top;
            long aw = r.right - r.left, ah = r.bottom - r.top;
            long w, h;

            if (pw > 0 && ph > 0 && aw > 0 && ah > 0) {
                w = aw;
                h = ph * aw / pw;
                if (h > ah) {
                    h = ah;
                    w = pw * ah / ph;
                }
                dst.left = (short) (r.left + (aw - w) / 2);
                dst.top = r.top;
                dst.right = (short) (dst.left + w);
                dst.bottom = (short) (dst.top + h);
                DrawPicture(pic, &dst);
            }
            ReleaseResource((Handle) pic);
        } else {
            TextFont(kFontIDGeneva);
            TextSize(36);
            TextFace(bold | shadow);
            {
                unsigned char *title = P_STRING_CONV("NetHack");
                short w = StringWidth(title);

                MoveTo((short) (r.left + (r.right - r.left - w) / 2),
                       (short) (r.top + 56));
                DrawString(title);
            }
            TextFont(systemFont);
            TextSize(0);
            TextFace(normal);
        }
    }
}

static pascal Boolean
about_filter(DialogRef wind, EventRecord *event, DialogItemIndex *item)
{
    char ch;

    /* movable modal: route background updates and track the title bar
       drag ourselves -- ModalDialog does neither */
    if (event->what == updateEvt
        && (WindowPtr) event->message != GetDialogWindow(wind)) {
        mac_handle_update_event(event);
        return FALSE;
    }
    if (event->what == mouseDown) {
        WindowPtr w;
        short part = FindWindow(event->where, &w);

        if (part == inDrag && w == GetDialogWindow(wind)) {
            Rect limits = qd.screenBits.bounds;

            InsetRect(&limits, 4, 4);
            DragWindow(w, event->where, &limits);
            *item = 0;
            return TRUE;
        }
    }
    if (event->what == keyDown || event->what == autoKey) {
        ch = (char) (event->message & CH_MASK);
        if (ch == CH_RETURN || ch == CH_ENTER || ch == CH_ESCAPE) {
            FlashButton(wind, abtOK);
            *item = abtOK;
            return TRUE;
        }
    }
    return FALSE;
}

static void
aboutNetHack(void)
{
    GrafPtr oldport;
    DialogRef dlog;
    short item, type;
    Handle h;
    Rect r;
    char tmp[64];
    Str255 vers;
    UserItemUPP redraw = NewUserItemUPP(about_redraw);
    ModalFilterUPP filter = NewModalFilterUPP(about_filter);

    dlog = GetNewDialog(RSRC_ABOUT, NULL, (WindowRef) -1);
    if (!dlog) {
        DisposeUserItemUPP(redraw);
        DisposeModalFilterUPP(filter);
        return;
    }
    GetPort(&oldport);
    SetPortDialogPort(dlog);

/* which slice of the fat binary this object was compiled into */
#ifdef CROSS_TO_MACPPC
#define ABOUT_ARCH "PowerPC"
#else
#define ABOUT_ARCH "68k"
#endif
    snprintf(tmp, sizeof tmp, "Version %d.%d.%d for the Macintosh (%s)",
             VERSION_MAJOR, VERSION_MINOR, PATCHLEVEL, ABOUT_ARCH);
    C2P(tmp, vers);
    GetDialogItem(dlog, abtVersion, &type, &h, &r);
    SetDialogItemText(h, vers);

    {
        char big[128];
        /* years from the authoritative banner ("NetHack, Copyright
           1985-20xx"), so this line tracks upstream updates;
           \251 = Mac Roman copyright sign */
        const char *yrs = strstr(COPYRIGHT_BANNER_A, "1985");

        snprintf(big, sizeof big,
                 "\251 %s Stichting Mathematisch Centrum, Amsterdam,"
                 " and the NetHack DevTeam.",
                 yrs ? yrs : "1985");
        C2P(big, vers);
        GetDialogItem(dlog, abtCredits, &type, &h, &r);
        SetDialogItemText(h, vers);

        /* \245 = Mac Roman bullet */
        snprintf(big, sizeof big, "Built %s \245 %s",
                 nomakedefs.build_date ? nomakedefs.build_date : "?",
                 nomakedefs.git_branch ? nomakedefs.git_branch
                                       : "unknown branch");
        C2P(big, vers);
        GetDialogItem(dlog, abtContact, &type, &h, &r);
        SetDialogItemText(h, vers);
    }

    GetDialogItem(dlog, abtArt, &type, &h, &r);
    SetDialogItem(dlog, abtArt, type, (Handle) redraw, &r);
    GetDialogItem(dlog, abtRing, &type, &h, &r);
    SetDialogItem(dlog, abtRing, type, (Handle) redraw, &r);
    /* the initial draw ran before the user-item procs were installed */
    GetWindowPortBounds(GetDialogWindow(dlog), &r);
    InvalWindowRect(GetDialogWindow(dlog), &r);

    do {
        ModalDialog(filter, &item);
    } while (item != abtOK);

    DisposeDialog(dlog);
    SetPort(oldport);
    DisposeUserItemUPP(redraw);
    DisposeModalFilterUPP(filter);
}

static void
askSave(void)
{
    Boolean doSave = 1;
    Boolean doYes = 0;

    if (theMenubar < mbarRegular) {
        short itemHit;

        ParamText(P_STRING_CONV("Really Save?"), P_EMPTY_STRING, P_EMPTY_STRING, P_EMPTY_STRING);
        itemHit = Alert(alrtMenu_NY, (ModalFilterUPP) 0L);
        ResetAlertStage();

        if (itemHit != bttnMenuAlertYes) {
            doSave = 0;
        } else {
            doYes = 1;
        }
    }
    if (doSave) {
        AddToKeyQueue('S', 1);
        if (doYes) {
            AddToKeyQueue('y', 1);
        }
    }
}

static void
askQuit(void)
{
    Boolean doQuit = 1;
    Boolean doYes = 0;
    Boolean winMac;
    char *quitinput;

    if (!strcmp(windowprocs.name, "mac"))
        winMac = 1;
    else
        winMac = 0;

    if (theMenubar < mbarRegular) {
        short itemHit;

        ParamText(P_STRING_CONV("Really Quit?"), P_EMPTY_STRING, P_EMPTY_STRING, P_EMPTY_STRING);
        itemHit = Alert(alrtMenu_NY, (ModalFilterUPP) 0L);
        ResetAlertStage();

        if (itemHit != bttnMenuAlertYes) {
            doQuit = 0;
        } else {
            doYes = 1;
        }
    }
    if (doQuit) {
        /* command input handling differs between the mac and tty window ports */
        if (winMac)
            quitinput = "#quit\r";
        else
            quitinput = "#q\r";

        while (*quitinput)
            AddToKeyQueue(*quitinput++, 1);
        if (doYes) {
            if (winMac)
                quitinput = "y\rq\r\r\r";
            else
                quitinput = "yq\r";
            while (*quitinput)
                AddToKeyQueue(*quitinput++, 1);
        }
    }
}

/*
 * Called from the idle path (HandleEvent default / mac_get_nh_event) to
 * keep the Tile Mode item's enable/check state in sync with the current
 * tile-mode availability and window state.
 */
void
mactile_menu_refresh(void)
{
    if (!gTileMenuNeedsUpdate) return;
    gTileMenuNeedsUpdate = 0;
    NhWindow *map = (WIN_MAP != WIN_ERR) ? &theWindows[WIN_MAP] : NULL;
    if (!map) return;
    if (mactile_available())
        EnableItem(MHND_GAME, menuGameTileMode);
    else
        DisableItem(MHND_GAME, menuGameTileMode);
    SetItemMark(MHND_GAME, menuGameTileMode,
                macmap_get_mode(map) ? checkMark : noMark);
}
