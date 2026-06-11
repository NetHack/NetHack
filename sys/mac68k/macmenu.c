/* NetHack 5.0	macmenu.c	$NHDT-Date: 1432512797 2015/05/25 00:13:17 $  $NHDT-Branch: master $:$NHDT-Revision: 1.13 $ */
/*      Copyright (c) Macintosh NetHack Port Team, 1993.          */
/* NetHack may be freely redistributed.  See license for details. */

/****************************************\
 * Extended Macintosh menu support
 *
 * provides access to all keyboard commands from cmd.c
 * provides control key functionality for classic keyboards
 * provides key equivalent references and logical menu groups
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

    /* submenu */
    menuWizard = 0
};

/* the following menu items are reserved */
enum {
    /* apple */
    menuAppleAboutBox = 1,
    ____Apple__1,

    /* File */
    menuFileRedraw = 1,
    menuFilePrevMsg,
    menuFileCleanup,
    ____File___1,
    menuFilePlayMode,
    menuFileEnterExplore,
    ____File___2,
    menuFileSave,
    ____File___3,
    menuFileQuit,
    /* appended dynamically by InitMenuRes: */
    menuFileTileMode,   /* item 11 — "Tile Mode" toggle */

    /* standard minimum Edit menu items */

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

#define MHND_WIZ pMenuList[listSubmenu]->mref[menuWizard].mhnd

/* mutually exclusive (and prioritized) menu bar states */
enum {
    mbarDim,
    mbarNoWindows,
    mbarDA,
    mbarNoMap,
    mbarRegular,
    mbarSpecial /* explore or debug mode */
};

#define WKND_MAP (WIN_BASE_KIND + NHW_MAP)

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
static short kAdjustWizardMenu = 1;

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
mac_askname()
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
            pt = *(Point *) &rect;
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
mustGetMenuAlerts()
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
InitMenuRes()
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
        *pMenuList[i] = **mlHnd;

        for (j = 0; j < pMenuList[i]->numMenus; j++) {
            if (!(menu = (MenuHandle) GetMenu((**mlHnd).mref[j].mresID))) {
                Str31 d;
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

    /* Append the "Tile Mode" toggle to the File menu (item menuFileTileMode).
       The MENU resource only has items 1-10; this adds item 11 at runtime. */
    AppendMenu(MHND_FILE, P_STRING_CONV("Tile Mode"));
    /* Start disabled; mactile_menu_refresh() will enable when available. */
    DisableItem(MHND_FILE, menuFileTileMode);

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

        if (kAdjustWizardMenu) {
            kAdjustWizardMenu = 0;

            SetMenuItemText(MHND_FILE, menuFilePlayMode, P_STRING_CONV("Debug"));
        }
    }

    else if (discover) {
        newMenubar = mbarSpecial;

        if (kAdjustWizardMenu) {
            kAdjustWizardMenu = 0;

            SetMenuItemText(MHND_FILE, menuFilePlayMode, P_STRING_CONV("Explore"));

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
            /* enable the file menu, but ... */
            EnableItem(MHND_FILE, 0);

            /* ... disable the window commands! */
            for (i = menuFileRedraw; i <= menuFileEnterExplore; i++)
                DisableItem(MHND_FILE, i);

            /* ... also disable Tile Mode (no map window yet) */
            DisableItem(MHND_FILE, menuFileTileMode);

            /* ... and disable the rest of the menus */
            for (i = menuEdit; i < NUM_MBAR; i++)
                DisableItem(MBARHND(i), 0);

            if (theMenubar == mbarDA)
                EnableItem(MHND_EDIT, 0);

            break;

        case mbarRegular:
        case mbarSpecial:
            /* enable all menus ... */
            for (i = menuFile; i < NUM_MBAR; i++)
                EnableItem(MBARHND(i), 0);

            /* ... except the unused Edit menu */
            DisableItem(MHND_EDIT, 0);

            /* ... enable the window commands */
            for (i = menuFileRedraw; i <= menuFileEnterExplore; i++)
                EnableItem(MHND_FILE, i);

            if (theMenubar == mbarRegular)
                DisableItem(MHND_FILE, menuFilePlayMode);
            else
                DisableItem(MHND_FILE, menuFileEnterExplore);

            /* Enable Tile Mode iff tiles available; sync check mark to live
               state here since AdjustMenus runs before menu pulldown. */
            if (mactile_available())
                EnableItem(MHND_FILE, menuFileTileMode);
            else
                DisableItem(MHND_FILE, menuFileTileMode);
            {
                NhWindow *_am_map = (WIN_MAP != WIN_ERR)
                                    ? &theWindows[WIN_MAP] : NULL;
                SetItemMark(MHND_FILE, menuFileTileMode,
                            macmap_get_mode(_am_map) ? checkMark : noMark);
            }

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
        case menuFileRedraw:
            AddToKeyQueue('R' & 0x1f, 1);
            break;

        case menuFilePrevMsg:
            AddToKeyQueue('P' & 0x1f, 1);
            break;

        case menuFileCleanup:
            (void) SanePositions();
            /* queue ^R: synchronous redraw from menu-handler context doesn't
               take (window port unsettled), so redraw in the command loop */
            AddToKeyQueue('R' & 0x1f, 1);
            break;

        case menuFileEnterExplore:
            AddToKeyQueue('X', 1);
            break;

        case menuFileSave:
            askSave();
            break;

        case menuFileQuit:
            askQuit();
            break;

        case menuFileTileMode: {
            NhWindow *map = (WIN_MAP != WIN_ERR) ? &theWindows[WIN_MAP] : NULL;
            if (!map) break;
            Boolean newOn = !macmap_get_mode(map);
            if (newOn && !macmap_set_mode(map, true)) {
                SysBeep(1);
                break;
            } else if (!newOn) {
                macmap_set_mode(map, false);
            }
            SetItemMark(MHND_FILE, menuFileTileMode,
                        newOn ? checkMark : noMark);
            iflags.wc_tiled_map = newOn;
            /* queue ^R: synchronous redraw from menu-handler context doesn't
               take; command-loop redraw re-emits print_glyph for the new mode */
            AddToKeyQueue('R' & 0x1f, 1);
            break;
        }
        }
        break;

    case menuEdit:
        (void) SystemEdit(menuItem - 1);
        break;

    default: /* get associated string and add to key queue */
    {
        Str255 mstr;
        short i;

        GetIndString(mstr, menuID, menuItem);
        if (mstr[0] > QUEUE_LEN)
            mstr[0] = QUEUE_LEN;

        for (i = 1; ((i <= mstr[0]) && (mstr[i] != mstrEndChar)); i++)
            AddToKeyQueue(mstr[i], false);
    } break;
    }

    HiliteMenu(0);
}

static void
aboutNetHack()
{
    if (theMenubar >= mbarRegular) {
        (void) doversion();
    } else {
        unsigned char aboutStr[32];
        char tmp[32];

        /* snprintf truncates to 31 chars + NUL, so C2P's 1 + strlen(tmp)
           bytes always fit aboutStr */
        snprintf(tmp, sizeof tmp, "NetHack %d.%d.%d",
                 VERSION_MAJOR, VERSION_MINOR, PATCHLEVEL);
        C2P(tmp, aboutStr);

        ParamText(aboutStr, P_STRING_CONV("\rdevteam@www.nethack.org"), P_EMPTY_STRING, P_EMPTY_STRING);
        (void) Alert(alrtMenuNote, (ModalFilterUPP) 0L);
        ResetAlertStage();
    }
}

static void
askSave()
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
askQuit()
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
        EnableItem(MHND_FILE, menuFileTileMode);
    else
        DisableItem(MHND_FILE, menuFileTileMode);
    SetItemMark(MHND_FILE, menuFileTileMode,
                macmap_get_mode(map) ? checkMark : noMark);
}
