/* NetHack 3.6	macmenu.c	$NHDT-Date: 1432512797 2015/05/25 00:13:17 $  $NHDT-Branch: master $:$NHDT-Revision: 1.13 $ */
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
#include "macpopup.h"
#include "patchlevel.h"

/******** Toolbox Defines ********/
#if !TARGET_API_MAC_CARBON
#include <Menus.h>
#include <Devices.h>
#include <Resources.h>
#include <TextUtils.h>
#include <ToolUtils.h>
#include <Sound.h>
#endif

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

    /* standard minimum Edit menu items */

    /* Wizard */
    menuWizardAttributes = 1
};

/*
 * menuListRec data (preloaded and locked) specifies the number of menus in
 * the menu bar, the number of hierarchal or submenus and the menu IDs of
 * all of those menus.  menus that go into in the menu bar are specified by
 * 'MNU#' 128 and submenus are specified by 'MNU#' 129.  the fields of the
 * menuListRec are:
 * firstMenuID - the menu ID (not resource ID) of the 1st menu.  subsequent
 *     menus in the list are _forced_ to have consecutively incremented IDs.
 * numMenus - the total count of menus in a given list (and the extent of
 *     valid menu IDs).
 * mref[] - initially the MENU resource ID is stored in the placeholder for
 *     the resource handle.  after loading (GetResource), the menu handle
 *     is stored and the menu ID, in memory, is set as noted above.
 *
 * NOTE: a ResEdit template editor is supplied to edit the 'MNU#' resources.
 *
 * NOTE: the resource IDs do not need to match the menu IDs in a menu list
 * record although they have been originally set that way.
 *
 * NOTE: the menu ID's of menus in the submenu list record may be reset, as
 * noted above.  it is the programmers responsibility to make sure that
 * submenu references/IDs are valid.
 *
 * WARNING: the existence of the submenu list record is assumed even if the
 * number of submenus is zero.  also, no error checking is done on the
 * extents of the menu IDs.  this must be correctly setup by the programmer.
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
    "\pAbort: Bad \'MNU#\' resource!", /* errGetMenuList */
    "\pAbort: Bad \'MENU\' resource!", /* errGetMenu */
    "\pAbort: Bad \'DLOG\' resource!", /* errGetANDlogTemplate */
    "\pAbort: Bad \'DITL\' resource!", /* errGetANDlogItems */
    "\pAbort: Bad Dialog Allocation!", /* errGetANDialog */
    "\pAbort: Bad Menu Allocation!",   /* errANNewMenu */
};
static menuListPtr pMenuList[2];
static short theMenubar = mbarDA; /* force initial update */
static short kAdjustWizardMenu = 1;

/******** Prototypes ********/
#if !TARGET_API_MAC_CARBON
static void alignAD(Rect *, short);
#endif
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

static RGBColor blackcolor = { 0x0000, 0x0000, 0x0000 },
                //	indentcolor = {0x4000, 0x4000, 0x4000},
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

    /* Enable or disable the appropriate item */
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

    /* Which item shall we redraw? */
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

        /* Draw the frame and drop shadow */
        rect.right--;
        rect.bottom--;
        FrameRect(&rect);
        MoveTo(rect.right, rect.top + 1);
        LineTo(rect.right, rect.bottom);
        LineTo(rect.left + 1, rect.bottom);

        /* Draw the menu character */
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

        /* Draw the popup symbol */
        MoveTo(rect.right - 16, rect.top + 5);
        LineTo(rect.right - 6, rect.top + 5);
        LineTo(rect.right - 11, rect.top + 10);
        LineTo(rect.right - 15, rect.top + 6);
        LineTo(rect.right - 8, rect.top + 6);
        LineTo(rect.right - 11, rect.top + 9);
        LineTo(rect.right - 13, rect.top + 7);
        LineTo(rect.right - 10, rect.top + 7);
        LineTo(rect.right - 11, rect.top + 8);

        /* Draw the shadow */
        InsetRect(&rect, 1, 1);
        if (macFlags.color) {
            RGBColor color;

            /* Save the foreground color */
            GetForeColor(&color);

            /* Draw the top and left */
            RGBForeColor(&lightcolor);
            MoveTo(rect.left, rect.bottom - 1);
            LineTo(rect.left, rect.top);
            LineTo(rect.right - 1, rect.top);

            /* Draw the bottom and right */
            RGBForeColor(&darkcolor);
            MoveTo(rect.right - 1, rect.top + 1);
            LineTo(rect.right - 1, rect.bottom - 1);
            LineTo(rect.left + 1, rect.bottom - 1);

            /* Restore the foreground color */
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
            /* Draw the top and left */
            RGBForeColor(&darkcolor);
            MoveTo(rect.left, rect.bottom - 1);
            LineTo(rect.left, rect.top);
            LineTo(rect.right - 1, rect.top);

            /* Draw the bottom and right */
            RGBForeColor(&lightcolor);
            MoveTo(rect.right - 1, rect.top + 1);
            LineTo(rect.right - 1, rect.bottom - 1);
            LineTo(rect.left + 1, rect.bottom - 1);

            /* Restore the colors */
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

    /* Create the dialog */
    if (!(askdialog = GetNewDialog(RSRC_ASK, NULL, (WindowRef) -1)))
        noresource('DLOG', RSRC_ASK);
    GetPort(&oldport);
    SetPortDialogPort(askdialog);

    /* Initialize the name text item */
    ask_restring(gp.plname, str);
    if (gp.plname[0]) {
        GetDialogItem(askdialog, RSRC_ASK_NAME, &type, &handle, &rect);
        SetDialogItemText(handle, str);
    }
#if 0
	{
	Str32 pName;
		pName [0] = 0;
		if (gp.plname && gp.plname [0]) {
			strcpy ((char *) pName, gp.plname);
			c2pstr ((char *) pName);
		} else {
			Handle h;
			h = GetResource ('STR ', -16096);
			if (((Handle) 0 != h) && (GetHandleSize (h) > 0)) {
				DetachResource (h);
				HLock (h);
				if (**h > 31) {
					**h = 31;
				}
				BlockMove (*h, pName, **h + 1);
				DisposeHandle (h);
			}
		}
		if (pName [0]) {
			GetDialogItem(askdialog, RSRC_ASK_NAME, &type, &handle, &rect);
			SetDialogItemText(handle, pName);
			if (pName [0] > 2 && pName [pName [0] - 1] == '-') {
			    short role = (*pANR).anMenu[anRole];
			    char suffix = (char) pName[pName[0]],
				*sfxindx = strchr(pl_classes, suffix);

			    if (sfxindx)
				role = (short) (sfxindx - pl_classes);
			    else if (suffix == '@')
				role = (short) rn2((int) strlen(pl_classes));
			    (*pANR).anMenu[anRole] = role;
			}
		}
	}
#endif
    SelectDialogItemText(askdialog, RSRC_ASK_NAME, 0, 32767);

    /* Initialize the role popup menu */
    if (!(askmenu[RSRC_ASK_ROLE] = NewMenu(RSRC_ASK_ROLE, "\p")))
        fatal("\pCannot create role menu");
    for (i = 0; roles[i].name.m; i++) {
        ask_restring(roles[i].name.m, str);
        AppendMenu(askmenu[RSRC_ASK_ROLE], str);
    }
    InsertMenu(askmenu[RSRC_ASK_ROLE], hierMenu);
    if (flags.initrole >= 0)
        currrole = flags.initrole;
    /* Check for backward compatibility */
    else if ((currrole = str2role(gp.pl_character)) < 0)
        currrole = randrole(FALSE);

    /* Initialize the race popup menu */
    if (!(askmenu[RSRC_ASK_RACE] = NewMenu(RSRC_ASK_RACE, "\p")))
        fatal("\pCannot create race menu");
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
    if (!(askmenu[RSRC_ASK_GEND] = NewMenu(RSRC_ASK_GEND, "\p")))
        fatal("\pCannot create gender menu");
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
    if (!(askmenu[RSRC_ASK_ALIGN] = NewMenu(RSRC_ASK_ALIGN, "\p")))
        fatal("\pCannot create alignment menu");
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
    if (!(askmenu[RSRC_ASK_MODE] = NewMenu(RSRC_ASK_MODE, "\p")))
        fatal("\pCannot create mode menu");
    AppendMenu(askmenu[RSRC_ASK_MODE], "\pNormal");
    AppendMenu(askmenu[RSRC_ASK_MODE], "\pExplore");
    AppendMenu(askmenu[RSRC_ASK_MODE], "\pDebug");
    InsertMenu(askmenu[RSRC_ASK_MODE], hierMenu);
    currmode = 0;

    /* Set the redraw procedures */
    for (item = RSRC_ASK_DEFAULT; item <= RSRC_ASK_MODE; item++) {
        GetDialogItem(askdialog, item, &type, &handle, &rect);
        SetDialogItem(askdialog, item, type, (Handle) redraw, &rect);
    }

    /* Handle dialog events */
    do {
        /* Adjust the Play button */
        ask_enable(askdialog, RSRC_ASK_PLAY,
                   GetDialogTextEditHandle(askdialog)[0]->teLength);

        /* Adjust the race popup menu */
        i = j = currrace;
        do {
            if (validrace(currrole, j)) {
                EnableMenuItem(askmenu[RSRC_ASK_RACE], j + 1);
                CheckMenuItem(askmenu[RSRC_ASK_RACE], j + 1, currrace == j);
            } else {
                DisableMenuItem(askmenu[RSRC_ASK_RACE], j + 1);
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
                EnableMenuItem(askmenu[RSRC_ASK_GEND], j + 1);
                CheckMenuItem(askmenu[RSRC_ASK_GEND], j + 1, currgend == j);
            } else {
                DisableMenuItem(askmenu[RSRC_ASK_GEND], j + 1);
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
                EnableMenuItem(askmenu[RSRC_ASK_ALIGN], j + 1);
                CheckMenuItem(askmenu[RSRC_ASK_ALIGN], j + 1, curralign == j);
            } else {
                DisableMenuItem(askmenu[RSRC_ASK_ALIGN], j + 1);
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
#if 0
	    /* limit the data here to 25 chars */
	    {
	    	short beepTEDelete = 1;

	    	while ((**dRec.textH).teLength > 25)
	    	{
	    		if (beepTEDelete++ <= 3)
	    			SysBeep(3);
	    		TEKey('\b', dRec.textH);
	    	}
	    }

	    /* special case filter (that doesn't plug all the holes!) */
	    if (((**dRec.textH).teLength == 1) && (**((**dRec.textH).hText) < 32))
	    	TEKey('\b', dRec.textH);
#endif
            break;
        }
    } while ((item != RSRC_ASK_PLAY) && (item != RSRC_ASK_QUIT));

    /* Process the name */
    GetDialogItem(askdialog, RSRC_ASK_NAME, &type, &handle, &rect);
    GetDialogItemText(handle, str);
    if (str[0] > PL_NSIZ - 1)
        str[0] = PL_NSIZ - 1;
    BlockMove(&str[1], gp.plname, str[0]);
    gp.plname[str[0]] = '\0';

    /* Destroy the dialog */
    for (i = RSRC_ASK_ROLE; i <= RSRC_ASK_MODE; i++) {
        DeleteMenu(i);
        DisposeMenu(askmenu[i]);
    }
    SetPort(oldport);
    DisposeDialog(askdialog);
    DisposeModalFilterUPP(filter);
    DisposeUserItemUPP(redraw);

    /* Process the mode */
    wizard = discover = 0;
    switch (currmode) {
    case 0: /* Normal */
        break;
    case 1: /* Explore */
        discover = 1;
        break;
    case 2: /* Debug */
        wizard = 1;
        strcpy(gp.plname, WIZARD_NAME);
        break;
    default: /* Quit */
        ExitToShell();
    }

    /* Process the role */
    strcpy(gp.pl_character, roles[currrole].name.m);
    flags.initrole = currrole;

    /* Process the race */
    flags.initrace = currrace;

    /* Process the gender */
    flags.female = flags.initgend = currgend;

    /* Process the alignment */
    flags.initalign = curralign;

    return;
}

/*** Menu bar routines ***/

#if !TARGET_API_MAC_CARBON
static void
alignAD(Rect *pRct, short vExempt)
{
    BitMap qbitmap;

    GetQDGlobalsScreenBits(&qbitmap);
    (*pRct).right -= (*pRct).left; /* width */
    (*pRct).bottom -= (*pRct).top; /* height */
    (*pRct).left = (qbitmap.bounds.right - (*pRct).right) / 2;
    (*pRct).top = (qbitmap.bounds.bottom - (*pRct).bottom - vExempt) / 2;
    (*pRct).top += vExempt;
    (*pRct).right += (*pRct).left;
    (*pRct).bottom += (*pRct).top;
}
#endif

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

#if !TARGET_API_MAC_CARBON
        alignAD(*hRct, GetMBarHeight());
#endif
    }
}

static void
menuError(short menuErr)
{
    short i;

    for (i = 0; i < beepMenuAlertErr; i++)
        SysBeep(3);

    ParamText(menuErrStr[menuErr], "\p", "\p", "\p");
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
            SetMenuID(menu, j + (**mlHnd).firstMenuID); /* consecutive IDs */

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

    /*
     *	if (windowprocs != mac_procs) {
     *		return;
     *	}
     */
    /* determine the new menubar state */
    if (dimMenubar)
        newMenubar = mbarDim;
    else if (!win)
        newMenubar = mbarNoWindows;
    else if (GetWindowKind(win) < 0)
        newMenubar = mbarDA;
    else if (!IsWindowVisible(_mt_window))
        newMenubar = mbarNoMap;

    if (newMenubar != mbarRegular)
        ; /* we've already found its state */
    else if (wizard) {
        newMenubar = mbarSpecial;

        if (kAdjustWizardMenu) {
            kAdjustWizardMenu = 0;

            SetMenuItemText(MHND_FILE, menuFilePlayMode, "\pDebug");
        }
    }

    else if (discover) {
        newMenubar = mbarSpecial;

        if (kAdjustWizardMenu) {
            kAdjustWizardMenu = 0;

            SetMenuItemText(MHND_FILE, menuFilePlayMode, "\pExplore");

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
                DisableMenuItem(MBARHND(i), 0);
            break;

        case mbarNoWindows:
        case mbarDA:
        case mbarNoMap:
            /* enable the file menu, but ... */
            EnableMenuItem(MHND_FILE, 0);

            /* ... disable the window commands! */
            for (i = menuFileRedraw; i <= menuFileEnterExplore; i++)
                DisableMenuItem(MHND_FILE, i);

            /* ... and disable the rest of the menus */
            for (i = menuEdit; i < NUM_MBAR; i++)
                DisableMenuItem(MBARHND(i), 0);

            if (theMenubar == mbarDA)
                EnableMenuItem(MHND_EDIT, 0);

            break;

        case mbarRegular:
        case mbarSpecial:
            /* enable all menus ... */
            for (i = menuFile; i < NUM_MBAR; i++)
                EnableMenuItem(MBARHND(i), 0);

            /* ... except the unused Edit menu */
            DisableMenuItem(MHND_EDIT, 0);

            /* ... enable the window commands */
            for (i = menuFileRedraw; i <= menuFileEnterExplore; i++)
                EnableMenuItem(MHND_FILE, i);

            if (theMenubar == mbarRegular)
                DisableMenuItem(MHND_FILE, menuFilePlayMode);
            else
                DisableMenuItem(MHND_FILE, menuFileEnterExplore);

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
#if !TARGET_API_MAC_CARBON
        else {
            unsigned char daName[32];

            GetMenuItemText(MHND_APPLE, menuItem, *(Str255 *) daName);
            (void) OpenDeskAcc(daName);
        }
#endif
        break;

    /*
     * Those direct calls are ugly: they should be installed into cmd.c .
     * Those AddToKeyQueue() calls are also ugly: they should be put into
     * the 'STR#' resource.
     */
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
        }
        break;

    case menuEdit:
#if !TARGET_API_MAC_CARBON
        (void) SystemEdit(menuItem - 1);
#endif
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
        (void) doversion(); /* is this necessary? */
    } else {
        unsigned char aboutStr[32] = "\pNetHack 3.4.";

        if (PATCHLEVEL > 10) {
            aboutStr[++aboutStr[0]] = '0' + PATCHLEVEL / 10;
        }

        aboutStr[++aboutStr[0]] = '0' + (PATCHLEVEL % 10);

        ParamText(aboutStr, "\p\rdevteam@www.nethack.org", "\p", "\p");
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

        ParamText("\pReally Save?", "\p", "\p", "\p");
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

        ParamText("\pReally Quit?", "\p", "\p", "\p");
        itemHit = Alert(alrtMenu_NY, (ModalFilterUPP) 0L);
        ResetAlertStage();

        if (itemHit != bttnMenuAlertYes) {
            doQuit = 0;
        } else {
            doYes = 1;
        }
    }
    if (doQuit) {
        /* MWM -- forgive me lord, an even uglier kludge to deal with
           differences
                in command input handling
         */
        if (winMac)
            quitinput = "#quit\r";
        else
            quitinput = "#q\r";

        /* KMH -- Ugly kludge */
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
