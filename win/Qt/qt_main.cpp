// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_main.cpp -- the main window

extern "C" {
#include "hack.h"
}

#include "qt_pre.h"
#include <QtGui/QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QtWidgets>
#endif
#include "qt_post.h"
#include "qt_main.h"
#include "qt_main.moc"
#include "qt_bind.h"
#include "qt_glyph.h"
#include "qt_inv.h"
#include "qt_key.h"
#include "qt_map.h"
#include "qt_msg.h"
#include "qt_set.h"
#include "qt_stat.h"
#include "qt_str.h"

#ifndef KDE
#include "qt_kde0.moc"
#endif

// temporary
extern char *qt_tilewidth;
extern char *qt_tileheight;
extern int qt_compact_mode;
// end temporary

namespace nethack_qt_ {

// temporary
void centerOnMain( QWidget* w );
// end temporary

/* XPM */
static const char * nh_icon[] = {
"40 40 6 1",
" 	s None c none",
".	c #ffffff",
"X	c #dadab6",
"o	c #6c91b6",
"O	c #476c6c",
"+	c #000000",
"                                        ",
"                                        ",
"                                        ",
"        .      .X..XX.XX      X         ",
"        ..   .....X.XXXXXX   XX         ",
"        ... ....X..XX.XXXXX XXX         ",
"   ..   ..........X.XXXXXXXXXXX   XX    ",
"   .... ........X..XX.XXXXXXXXX XXXX    ",
"   .... ..........X.XXXXXXXXXXX XXXX    ",
"   ooOOO..ooooooOooOOoOOOOOOOXX+++OO++  ",
"   ooOOO..ooooooooOoOOOOOOOOOXX+++OO++  ",
"   ....O..ooooooOooOOoOOOOOOOXX+XXXX++  ",
"   ....O..ooooooooOoOOOOOOOOOXX+XXXX++  ",
"   ..OOO..ooooooOooOOoOOOOOOOXX+++XX++  ",
"    ++++..ooooooooOoOOOOOOOOOXX+++ +++  ",
"     +++..ooooooOooOOoOOOOOOOXX+++  +   ",
"      ++..ooooooooOoOOOOOOOOOXX+++      ",
"        ..ooooooOooOOoOOOOOOOXX+++      ",
"        ..ooooooooOoOOOOOOOOOXX+++      ",
"        ..ooooooOooOOoOOOOOOOXX+++      ",
"        ..ooooooooOoOOOOOOOOOXX+++      ",
"         ..oooooOooOOoOOOOOOXX+++       ",
"         ..oooooooOoOOOOOOOOXX+++       ",
"          ..ooooOooOOoOOOOOXX+++        ",
"          ..ooooooOoOOOOOOOXX++++       ",
"        ..o..oooOooOOoOOOOXX+XX+++      ",
"       ...o..oooooOoOOOOOXX++XXX++      ",
"      ....OO..ooOooOOoOOXX+++XXXX++     ",
"     ...oo..+..oooOoOOOXX++XXooXXX++    ",
"    ...ooo..++..OooOOoXX+++XXooOXXX+    ",
"   ..oooOOXX+++....XXXX++++XXOOoOOXX+   ",
"   ..oooOOXX+++ ...XXX+++++XXOOooOXX++  ",
"   ..oooOXXX+++  ..XX+++  +XXOOooOXX++  ",
"   .....XXX++++             XXXXXXX++   ",
"    ....XX++++              XXXXXXX+    ",
"     ...XX+++                XXXXX++    ",
"                                        ",
"                                        ",
"                                        ",
"                                        "};
/* XPM */
static const char * nh_icon_small[] = {
/* width height ncolors chars_per_pixel */
"16 16 16 1",
/* colors */
"  c #587070",
". c #D1D5C9",
"X c #8B8C84",
"o c #2A2A28",
"O c #9AABA9",
"+ c #6A8FB2",
"@ c #C4CAC4",
"# c #B6BEB6",
"$ c None",
"% c #54564E",
"& c #476C6C",
"* c #ADB2AB",
"= c #ABABA2",
"- c #5E8295",
"; c #8B988F",
": c #E8EAE7",
/* pixels */
"$$$$$$$$$$$$$$$$",
"$$$.$#::.#==*$$$",
"$.*:::::....#*=$",
"$@#:..@#*==#;XX;",
"$@O:+++- &&; X%X",
"$#%.+++- &&;% oX",
"$$o.++-- &&;%%X$",
"$$$:++-- &&;%%$$",
"$$$.O++- &&=o $$",
"$$$=:++- & XoX$$",
"$$*:@O--  ;%Xo$$",
"$*:O#$+--;oOOX $",
"$:+ =o::=oo=-;%X",
"$::.%o$*;X;##@%$",
"$$@# ;$$$$$=*;X$",
"$$$$$$$$$$$$$$$$"
};

#if 0 // RLC
/* XPM */
static const char * map_xpm[] = {
"12 13 4 1",
".	c None",
" 	c #000000000000",
"X	c #0000B6DAFFFF",
"o	c #69A69248B6DA",
"           .",
" XXXXX ooo  ",
" XoooX o    ",
" XoooX o o  ",
" XoooX ooo  ",
" XXoXX o    ",
"  oooooXXX  ",
" oo o oooX  ",
"    o XooX  ",
" oooo XooX  ",
" o  o XXXX  ",
"            ",
".           "};
/* XPM */
static const char * msg_xpm[] = {
"12 13 4 1",
".	c None",
" 	c #FFFFFFFFFFFF",
"X	c #69A69248B6DA",
"o	c #000000000000",
"           .",
" XXX XXX X o",
"           o",
" XXXXX XX  o",
"           o",
" XX XXXXX  o",
"           o",
" XXXXXX    o",
"           o",
" XX XXX XX o",
"           o",
"           o",
".ooooooooooo"};
/* XPM */
static const char * stat_xpm[] = {
"12 13 5 1",
"  c None",
".	c #FFFF00000000",
"X	c #000000000000",
"o	c #FFFFFFFF0000",
"O	c #69A6FFFF0000",
"            ",
"            ",
"...         ",
"...X        ",
"...X    ... ",
"oooX    oooX",
"oooXooo oooX",
"OOOXOOOXOOOX",
"OOOXOOOXOOOX",
"OOOXOOOXOOOX",
"OOOXOOOXOOOX",
"OOOXOOOXOOOX",
" XXXXXXXXXXX"};
#endif
/* XPM */
static const char * info_xpm[] = {
"12 13 4 1",
"  c None",
".	c #00000000FFFF",
"X	c #FFFFFFFFFFFF",
"o	c #000000000000",
"    ...     ",
"  .......   ",
" ...XXX...  ",
" .........o ",
"...XXXX.... ",
"....XXX....o",
"....XXX....o",
"....XXX....o",
" ...XXX...oo",
" ..XXXXX..o ",
"  .......oo ",
"   o...ooo  ",
"     ooo    "};


/* XPM */
static const char * again_xpm[] = {
"12 13 2 1",
" 	c None",
".	c #000000000000",
"    ..      ",
"     ..     ",
"   .....    ",
" .......    ",
"...  ..  .. ",
"..  ..   .. ",
"..        ..",
"..        ..",
"..        ..",
" ..      .. ",
" .......... ",
"   ......   ",
"            "};
/* XPM */
static const char * kick_xpm[] = {
"12 13 3 1",
" 	c None",
".	c #000000000000",
"X	c #FFFF6DB60000",
"            ",
"            ",
"   .  .  .  ",
"  ...  .  . ",
"   ...  .   ",
"    ...  .  ",
"     ...    ",
"XXX   ...   ",
"XXX.  ...   ",
"XXX. ...    ",
"XXX. ..     ",
" ...        ",
"            "};
/* XPM */
static const char * throw_xpm[] = {
"12 13 3 1",
" 	c None",
".	c #FFFF6DB60000",
"X	c #000000000000",
"            ",
"            ",
"            ",
"            ",
"....     X  ",
"....X     X ",
"....X XXXXXX",
"....X     X ",
" XXXX    X  ",
"            ",
"            ",
"            ",
"            "};
/* XPM */
static const char * fire_xpm[] = {
"12 13 5 1",
" 	c None",
".	c #B6DA45140000",
"X	c #FFFFB6DA9658",
"o	c #000000000000",
"O	c #FFFF6DB60000",
" .          ",
" X.         ",
" X .        ",
" X .o       ",
" X  .    o  ",
" X  .o    o ",
"OOOOOOOOoooo",
" X  .o    o ",
" X . o   o  ",
" X .o       ",
" X. o       ",
" . o        ",
"  o         "};
/* XPM */
static const char * get_xpm[] = {
"12 13 3 1",
" 	c None",
".	c #000000000000",
"X	c #FFFF6DB60000",
"            ",
"     .      ",
"    ...     ",
"   . . .    ",
"     .      ",
"     .      ",
"            ",
"   XXXXX    ",
"   XXXXX.   ",
"   XXXXX.   ",
"   XXXXX.   ",
"    .....   ",
"            "};
/* XPM */
static const char * drop_xpm[] = {
"12 13 3 1",
" 	c None",
".	c #FFFF6DB60000",
"X	c #000000000000",
"            ",
"   .....    ",
"   .....X   ",
"   .....X   ",
"   .....X   ",
"    XXXXX   ",
"            ",
"      X     ",
"      X     ",
"    X X X   ",
"     XXX    ",
"      X     ",
"            "};
/* XPM */
static const char * eat_xpm[] = {
"12 13 4 1",
" 	c None",
".	c #000000000000",
"X	c #FFFFB6DA9658",
"o	c #FFFF6DB60000",
"  .X.  ..   ",
"  .X.  ..   ",
"  .X.  ..   ",
"  .X.  ..   ",
"  ...  ..   ",
"   ..  ..   ",
"   ..  ..   ",
"   oo  oo   ",
"   oo  oo   ",
"   oo  oo   ",
"   oo  oo   ",
"   oo  oo   ",
"   oo  oo   "};
/* XPM */
static const char * rest_xpm[] = {
"12 13 2 1",
" 	c None",
".	c #000000000000",
"  .....     ",
"     .      ",
"    .       ",
"   .    ....",
"  .....   . ",
"         .  ",
"        ....",
"            ",
"     ....   ",
"       .    ",
"      .     ",
"     ....   ",
"            "};
/* XPM */
static const char * cast_a_xpm[] UNUSED = {
"12 13 3 1",
" 	c None",
".	c #FFFF6DB60000",
"X	c #000000000000",
"    .       ",
"    .       ",
"   ..       ",
"   ..       ",
"  ..  .     ",
"  ..  .     ",
" ......     ",
" .. ..  XX  ",
"    .. X  X ",
"   ..  X  X ",
"   ..  XXXX ",
"   .   X  X ",
"   .   X  X "};
/* XPM */
static const char * cast_b_xpm[] UNUSED = {
"12 13 3 1",
" 	c None",
".	c #FFFF6DB60000",
"X	c #000000000000",
"    .       ",
"    .       ",
"   ..       ",
"   ..       ",
"  ..  .     ",
"  ..  .     ",
" ......     ",
" .. .. XXX  ",
"    .. X  X ",
"   ..  XXX  ",
"   ..  X  X ",
"   .   X  X ",
"   .   XXX  "};
/* XPM */
static const char * cast_c_xpm[] UNUSED = {
"12 13 3 1",
" 	c None",
".	c #FFFF6DB60000",
"X	c #000000000000",
"    .       ",
"    .       ",
"   ..       ",
"   ..       ",
"  ..  .     ",
"  ..  .     ",
" ......     ",
" .. ..  XX  ",
"    .. X  X ",
"   ..  X    ",
"   ..  X    ",
"   .   X  X ",
"   .    XX  "};

static QString
aboutMsg()
{
    char *p, vbuf[BUFSZ];
    /* nethack's getversionstring() includes a final period
       but we're using it mid-sentence so strip period off */
    if ((p = strrchr(getversionstring(vbuf), '.')) != 0 && *(p + 1) == '\0')
        *p = '\0';
    QString msg;
    msg.sprintf(
        // format
        "Qt NetHack is a version of NetHack built using" // no newline
#ifdef KDE
        " KDE and"                                       // ditto
#endif
        " the Qt %d GUI toolkit.\n"                      // short Qt version
        "\n"
        "This is %s%s.\n"       // long nethack version and full Qt version
        "\n"
        "NetHack's Qt interface originally developed by Warwick Allison.\n"
        "\n"
#if 0
        "Homepage:\n     http://trolls.troll.no/warwick/nethack/\n" //obsolete
#endif
#ifdef KDE
        "KDE:\n     https://kde.org/\n"
#endif
#if 1
        "Qt:\n     https://qt.io/\n"
#else
        "Qt:\n     http://www.troll.no/\n"      // obsolete
#endif
        "NetHack:\n     %s\n", // DEVTEAM_URL
        // arguments
#ifdef QT_VERSION_MAJOR
        QT_VERSION_MAJOR,
#else
        5,              // Qt version macro should exist; if not, assume Qt5
#endif
        vbuf,           // nethack version
#ifdef QT_VERSION_STR
        " with Qt " QT_VERSION_STR,
#else
        "",
#endif
        DEVTEAM_URL);
    return msg;
}

class SmallToolButton : public QToolButton {
public:
    SmallToolButton(const QPixmap & pm, const QString &textLabel,
                 const QString& grouptext,
                 QObject * receiver, const char* slot,
                 QWidget * parent) :
	QToolButton(parent)
    {
	setIcon(QIcon(pm));
	setToolTip(textLabel);
	setStatusTip(grouptext);
	connect(this, SIGNAL(clicked(bool)), receiver, slot);
    }

    QSize sizeHint() const
    {
	// get just a couple more pixels for the map
	return QToolButton::sizeHint()-QSize(0,2);
    }
};

NetHackQtMainWindow::NetHackQtMainWindow(NetHackQtKeyBuffer& ks) :
    message(0), map(0), status(0), invusage(0),
    hsplitter(0), vsplitter(0),
    keysink(ks), dirkey(0)
{
    QToolBar* toolbar = new QToolBar(this);
    toolbar->setMovable(false);
    toolbar->setFocusPolicy(Qt::NoFocus);
    addToolBar(toolbar);
    menubar = menuBar();

    setWindowTitle("Qt NetHack");
    setWindowIcon(QIcon(QPixmap(qt_compact_mode ? nh_icon_small : nh_icon)));

#ifdef MACOSX
    /*
     * OSX Note:
     *  The toolbar on OSX starts with a system menu labeled with the
     *  Apple logo and an application menu labeled with the application's
     *  name (taken from Info.plist if present, otherwise the base name
     *  of the running program).  After that, application-specific menus
     *  (in our case "game",...,"help") follow.  Several menu entry
     *  names ("About", "Quit"/"Exit", "Preferences"/"Options"/
     *  "Settings"/"Setup"/"Config") get hijacked and placed in the
     *  application menu (and renamed in the process) even if the code
     *  here tries to put them in another menu.
     *  See QtWidgets/doc/qmenubar.html for slightly more information.
     *  setMenuRole() is supposed to be able to override this behavior.
     */
#endif
    QMenu* game=new QMenu;
    QMenu* apparel=new QMenu;
    QMenu* act1=new QMenu;
    QMenu* act2 = qt_compact_mode ? new QMenu : act1;
    QMenu* magic=new QMenu;
    QMenu* info=new QMenu;

    QMenu *help;
#ifdef KDE
    help = kapp->getHelpMenu( true, "" );
    help->addSeparator();
#else
    help = qt_compact_mode ? info : new QMenu;
#endif

    enum { OnDesktop=1, OnHandhelds=2 };
    struct Macro {
	QMenu* menu;
	const char* name;
	int flags;
        int NDECL((*funct));
    } item[] = {
        { game,    0, 3},
        { game,    "Version",            3, doversion},
        { game,    "Compilation",        3, doextversion},
        { game,    "History",            3, dohistory},
        { game,    "Redraw",             0, doredraw}, // useless
        { game,
#ifdef MACOSX
            /* Qt on OSX would rename "Options" to "Preferences..." and
               move it from intended destination to the application menu */
                   "Run-time &" // rely on adjacent string concatenation
#endif
                   "Options",            3, doset},
        { game,    "Explore mode",       3, enter_explore_mode},
        { game,    0, 3},
        { game,    "Save-and-exit",      3, dosave},
        { game,
#ifdef MACOSX
            /* need something to prevent matching leading "quit"
               so that it isn't hijacked for the application menu */
                   "_&"
#endif
                   "Quit-without-saving", 3, done2},

        { apparel, "Apparel off",        2, doddoremarm},
        { apparel, "Remove many",        1, doddoremarm},
        { apparel, 0, 3},
        { apparel, "Wield weapon",       3, dowield},
        { apparel, "Exchange weapons",   3, doswapweapon},
        { apparel, "Two weapon combat",  3, dotwoweapon},
        { apparel, "Load quiver",        3, dowieldquiver},
        { apparel, 0, 3},
        { apparel, "Wear armor",         3, dowear},
        { apparel, "Take off armor",     3, dotakeoff},
        { apparel, 0, 3},
        { apparel, "Put on accessories", 3, doputon},
        { apparel, "Remove accessories", 3, doremring},

        /* { act1,      "Again\tCtrl+A",           "\001", 2},
        { act1, 0, 0, 3}, */
        { act1, "Apply",             3, doapply},
        { act1, "Chat",              3, dotalk},
        { act1, "Close door",        3, doclose},
        { act1, "Down",              3, dodown},
        { act1, "Drop many",         2, doddrop},
        { act1, "Drop",              2, dodrop},
        { act1, "Eat",               2, doeat},
        { act1, "Engrave",           3, doengrave},
        /* { act1,      "Fight\tShift+F",             "F", 3}, */
        { act1, "Fire from quiver",  2, dofire},
        { act1, "Force",             3, doforce},
        { act1, "Get",               2, dopickup},
        { act1, "Jump",              3, dojump},
        { act2, "Kick",              2, dokick},
        { act2, "Loot",              3, doloot},
        { act2, "Open door",         3, doopen},
        { act2, "Pay",               3, dopay},
        { act2, "Rest",              2, donull},
        { act2, "Ride",              3, doride},
        { act2, "Search",            3, dosearch},
        { act2, "Sit",               3, dosit},
        { act2, "Throw",             2, dothrow},
        { act2, "Untrap",            3, dountrap},
        { act2, "Up",                3, doup},
        { act2, "Wipe face",         3, dowipe},

        { magic, "Quaff potion",     3, dodrink},
        { magic, "Read scroll/book", 3, doread},
        { magic, "Zap wand",         3, dozap},
        { magic, "Zap spell",        3, docast},
        { magic, "Dip",              3, dodip},
        { magic, "Rub",              3, dorub},
        { magic, "Invoke",           3, doinvoke},
        { magic, 0, 3},
        { magic, "Offer",            3, dosacrifice},
        { magic, "Pray",             3, dopray},
        { magic, 0, 3},
        { magic, "Teleport",         3, dotelecmd},
        { magic, "Monster action",   3, domonability},
        { magic, "Turn undead",      3, doturn},

        { help,  "Help",             3, dohelp},
        { help,  0, 3},
        { help,  "What is here",     3, dolook},
        { help,  "What is there",    3, doquickwhatis},
        { help,  "What is...",       2, dowhatis},
        { help,  0, 1},

        { info,  "Inventory",        3, ddoinv},
        { info,  "Attributes (extended status)", 3, doattributes },
        { info,  "Overview",         3, dooverview },
        { info,  "Conduct",          3, doconduct},
        { info,  "Discoveries",      3, dodiscovered},
        { info,  "List/reorder spells",  3, dovspell},
        { info,  "Adjust inventory letters", 3, doorganize },
        { info,  0, 3},
        { info,  "Name object or creature", 3, docallcmd},
        { info,  "Annotate level",   3, donamelevel },
        { info,  0, 3},
        { info,  "Skills",  3, enhance_weapon_skill},

	{ 0, 0, 0 }
    };

    int i;

    game->addAction(
#ifndef MACOSX
                    "Qt settings...",
#else
                    /* on OSX, put this in the application menu by using
                       a name that Qt will move to that menu */
                    "Preferences...",
#endif
                    this, SLOT(doQtSettings(bool)));
    /* on OSX, 'about' will end up in the application menu rather than
       the help menu (this had trailing "..." but that conflicts with
       the convention that an elipsis indicates the choice will bring
       up its own sub-menu) */
    help->addAction("About Qt NetHack", this, SLOT(doAbout(bool)));
    //help->addAction("NetHack Guidebook", this, SLOT(doGuidebook(bool)));
    help->addSeparator();

    for (i=0; item[i].menu; i++) {
	if ( item[i].flags & (qt_compact_mode ? 1 : 2) ) {
	    if (item[i].name) {
                char actchar[32];
                char menuitem[BUFSZ];
                actchar[0] = actchar[1] = '\0';
                if (item[i].funct) {
                    actchar[0] = cmd_from_func(item[i].funct);
                    if (actchar[0]
                        /* M-c won't work; translation between character
                           sets by the QString class can classify such
                           characters as erroneous and change them to '?' */
                        && ((actchar[0] & 0x7f) != actchar[0]
                        /* the vi movement keys won't work reliably
                           because toggling number_pad affects them but
                           doesn't redo these menus */
                            || strchr("hjklyubnHJKLYUBN", actchar[0])
                            || strchr("hjklyubn", (actchar[0] | 0x60))))
                        actchar[0] = '\0';
                }
                if (actchar[0] && !qt_compact_mode)
                    Sprintf(menuitem, "%.50s\t%.9s", item[i].name,
                            visctrl(actchar[0]));
                else
                    Sprintf(menuitem, "%s", item[i].name);

                if (item[i].funct && !actchar[0]) {
                    actchar[0] = '#';
                    (void) cmdname_from_func(item[i].funct,
                                             &actchar[1], FALSE);
                }
                if (actchar[0]) {
                    QString name = menuitem;
                    QAction *action = item[i].menu->addAction(name);
                    action->setData(actchar);
                }
	    } else {
		item[i].menu->addSeparator();
	    }
	}
    }

    game->setTitle("Game");
    menubar->addMenu(game);
    apparel->setTitle("Gear");
    menubar->addMenu(apparel);

    if ( qt_compact_mode ) {
	act1->setTitle("A-J");
	menubar->addMenu(act1);
	act2->setTitle("K-Z");
	menubar->addMenu(act2);
	magic->setTitle("Magic");
	menubar->addMenu(magic);
	info->setIcon(QIcon(QPixmap(info_xpm)));
	info->setTitle("Info");
	menubar->addMenu(info);
	//menubar->insertItem(QPixmap(map_xpm), this, SLOT(raiseMap()));
	//menubar->insertItem(QPixmap(msg_xpm), this, SLOT(raiseMessages()));
	//menubar->insertItem(QPixmap(stat_xpm), this, SLOT(raiseStatus()));
	info->addSeparator();
	info->addAction("Map", this, SLOT(raiseMap()));
	info->addAction("Messages", this, SLOT(raiseMessages()));
	info->addAction("Status", this, SLOT(raiseStatus()));
    } else {
	act1->setTitle("Action");
	menubar->addMenu(act1);
	magic->setTitle("Magic");
	menubar->addMenu(magic);
	info->setTitle("Info");
	menubar->addMenu(info);
	menubar->addSeparator();
	help->setTitle("Help");
	menubar->addMenu(help);
    }
#ifdef MACOSX
    /* for OSX, the attempt above to add "About Qt NetHack" went into
       the application menu instead of the help menu; we'll add it to
       the latter now and have two ways to access it; without the
       leading underscore (or some other spelling variation such as
       "'bout"), this one would get intercepted too and then evidently
       be discarded as a duplicate */
    help->addSeparator();
    help->addAction("_About_Qt_NetHack_", this, SLOT(doAbout(bool)));
    /* we also want a "Quit NetHack" entry in the application menu;
       when "_Quit-without-saving" was called "Quit" it got intercepted
       for that, but now it needs to be added separately; we'll use a
       handy menu and let the interception put it in the intended place */
    game->addAction("Quit NetHack", this, SLOT(doQuit(bool)));
#endif

    QSignalMapper* sm = new QSignalMapper(this);
    connect(sm, SIGNAL(mapped(const QString&)), this, SLOT(doKeys(const QString&)));
    QToolButton* tb;
    char actchar[32];
    tb = new SmallToolButton( QPixmap(again_xpm),"Again","Action", sm, SLOT(map()), toolbar );
    Sprintf(actchar, "%c", g.Cmd.spkeys[NHKF_DOAGAIN]);
    sm->setMapping(tb, actchar );
    toolbar->addWidget(tb);
    tb = new SmallToolButton( QPixmap(get_xpm),"Get","Action", sm, SLOT(map()), toolbar );
    Sprintf(actchar, "%c", cmd_from_func(dopickup));
    sm->setMapping(tb, actchar );
    toolbar->addWidget(tb);
    tb = new SmallToolButton( QPixmap(kick_xpm),"Kick","Action", sm, SLOT(map()), toolbar );
    Sprintf(actchar, "%c", cmd_from_func(dokick));
    sm->setMapping(tb, actchar );
    toolbar->addWidget(tb);
    tb = new SmallToolButton( QPixmap(throw_xpm),"Throw","Action", sm, SLOT(map()), toolbar );
    Sprintf(actchar, "%c", cmd_from_func(dothrow));
    sm->setMapping(tb, actchar );
    toolbar->addWidget(tb);
    tb = new SmallToolButton( QPixmap(fire_xpm),"Fire","Action", sm, SLOT(map()), toolbar );
    Sprintf(actchar, "%c", cmd_from_func(dofire));
    sm->setMapping(tb, actchar );
    toolbar->addWidget(tb);
    tb = new SmallToolButton( QPixmap(drop_xpm),"Drop","Action", sm, SLOT(map()), toolbar );
    Sprintf(actchar, "%c", cmd_from_func(doddrop));
    sm->setMapping(tb, actchar );
    toolbar->addWidget(tb);
    tb = new SmallToolButton( QPixmap(eat_xpm),"Eat","Action", sm, SLOT(map()), toolbar );
    Sprintf(actchar, "%c", cmd_from_func(doeat));
    sm->setMapping(tb, actchar );
    toolbar->addWidget(tb);
    tb = new SmallToolButton( QPixmap(rest_xpm),"Rest","Action", sm, SLOT(map()), toolbar );
    Sprintf(actchar, "%c", cmd_from_func(donull));
    sm->setMapping(tb, actchar );
    toolbar->addWidget(tb);

    connect(game,SIGNAL(triggered(QAction *)),this,SLOT(doMenuItem(QAction *)));
    connect(apparel,SIGNAL(triggered(QAction *)),this,SLOT(doMenuItem(QAction *)));
    connect(act1,SIGNAL(triggered(QAction *)),this,SLOT(doMenuItem(QAction *)));
    if (act2 != act1)
	connect(act2,SIGNAL(triggered(QAction *)),this,SLOT(doMenuItem(QAction *)));
    connect(magic,SIGNAL(triggered(QAction *)),this,SLOT(doMenuItem(QAction *)));
    connect(info,SIGNAL(triggered(QAction *)),this,SLOT(doMenuItem(QAction *)));
    connect(help,SIGNAL(triggered(QAction *)),this,SLOT(doMenuItem(QAction *)));

#ifdef KDE
    setMenu (menubar);
#endif

    int x=0,y=0;
    int w=QApplication::desktop()->width()-10; // XXX arbitrary extra space for frame
    int h=QApplication::desktop()->height()-50;

    int maxwn;
    int maxhn;
    if (qt_tilewidth != NULL) {
	maxwn = atoi(qt_tilewidth) * COLNO + 10;
    } else {
	maxwn = 1400;
    }
    if (qt_tileheight != NULL) {
	maxhn = atoi(qt_tileheight) * ROWNO * 6/4;
    } else {
	maxhn = 1024;
    }

    // Be exactly the size we want to be - full map...
    if (w>maxwn) {
	x+=(w-maxwn)/2;
	w=maxwn; // Doesn't need to be any wider
    }
    if (h>maxhn) {
	y+=(h-maxhn)/2;
	h=maxhn; // Doesn't need to be any taller
    }

    setGeometry(x,y,w,h);

    if ( qt_compact_mode ) {
	stack = new QStackedWidget(this);
	setCentralWidget(stack);
    } else {
	vsplitter = new QSplitter(Qt::Vertical);
	setCentralWidget(vsplitter);
	hsplitter = new QSplitter(Qt::Horizontal);
	invusage = new NetHackQtInvUsageWindow(hsplitter);
	vsplitter->insertWidget(0, hsplitter);
	hsplitter->insertWidget(1, invusage);
    }
}

void NetHackQtMainWindow::zoomMap()
{
    qt_settings->toggleGlyphSize();
}

void NetHackQtMainWindow::raiseMap()
{
    if ( stack->currentWidget() == map->Widget() ) {
	zoomMap();
    } else {
	stack->setCurrentWidget(map->Widget());
    }
}

void NetHackQtMainWindow::raiseMessages()
{
    stack->setCurrentWidget(message->Widget());
}

void NetHackQtMainWindow::raiseStatus()
{
    stack->setCurrentWidget(status->Widget());
}

#if 0 // RLC this isn't used
class NetHackMimeSourceFactory : public Q3MimeSourceFactory {
public:
    const QMimeSource* data(const QString& abs_name) const
    {
	const QMimeSource* r = 0;
	if ( (NetHackMimeSourceFactory *) this
             == Q3MimeSourceFactory::defaultFactory() )
	    r = Q3MimeSourceFactory::data(abs_name);
	else
	    r = Q3MimeSourceFactory::defaultFactory()->data(abs_name);
	if ( !r ) {
	    int sl = abs_name.length();
	    do {
		sl = abs_name.lastIndexOf('/',sl-1);
		QString name = sl>=0 ? abs_name.mid(sl+1) : abs_name;
		int dot = name.lastIndexOf('.');
		if ( dot >= 0 )
		    name = name.left(dot);
		if ( name == "map" )
		    r = new Q3ImageDrag(QImage(map_xpm));
		else if ( name == "msg" )
		    r = new Q3ImageDrag(QImage(msg_xpm));
		else if ( name == "stat" )
		    r = new Q3ImageDrag(QImage(stat_xpm));
	    } while (!r && sl>0);
	}
	return r;
    }
};
#endif

void NetHackQtMainWindow::doMenuItem(QAction *action)
{
    /* this converts meta characters to '?'; menu processing has been
       changed to send multi-character "#abc" instead (already needed
       for commands that didn't have either a regular keystroke or a
       meta shortcut); it must send just enough to disambiguate from
       other extended command names, otherwise the remainder would be
       left in the queue for subsequent handling as additional commands */
    doKeys(action->data().toString());
}

void NetHackQtMainWindow::doQtSettings(bool)
{
    centerOnMain(qt_settings);
    qt_settings->show();
}

void NetHackQtMainWindow::doAbout(bool)
{
    QMessageBox::about(this, "About Qt NetHack", aboutMsg());
}

// on OSX, "quit nethack" has been selected in the application menu or
// "Command+Q" has been typed -- user is asking to quit the application;
// unlike with the window's Close button, user has a chance to back out
void NetHackQtMainWindow::doQuit(bool)
{
    // there is a separate Quit-without-saving menu entry in the game menu
    // that leads to nethack's "Really quit?" prompt; OSX players can use
    // either one, other implementations only have that other one but this
    // routine is unconditional in case someone wants to change that
#ifdef MACOSX
    QString info;
    info.sprintf("This will end your NetHack session.%s",
                 !g.program_state.something_worth_saving ? ""
                 : "\n(Cancel quitting and use the Save command"
                   "\nto save your current game.)");
    /* this is similar to closeEvent but the details are different */
    int act = QMessageBox::information(this, "NetHack", info,
                                       "&Quit without saving",
                                       "&Cancel and return to game",
                                       0, 1);
    switch (act) {
    case 0:
        // quit -- bypass the prompting preformed by done2()
        g.program_state.stopprint++;
        ::done(QUIT);
        /*NOTREACHED*/
        break;
    case 1:
        // cancel
        break; // return to game
    }
#endif
    return;
}

#if 0 // RLC this isn't used
void NetHackQtMainWindow::doGuidebook(bool)
{
    QDialog dlg(this);
    new QVBoxLayout(&dlg);
    Q3TextBrowser browser(&dlg);
    NetHackMimeSourceFactory ms;
    browser.setMimeSourceFactory(&ms);
    browser.setSource(QDir::currentPath()+"/Guidebook.html");
    if ( qt_compact_mode )
	dlg.showMaximized();
    dlg.exec();
}
#endif

void NetHackQtMainWindow::doKeys(const char *cmds)
{
    keysink.Put(cmds);
    qApp->exit();
}

void NetHackQtMainWindow::doKeys(const QString& k)
{
    /* [this should probably be using toLocal8Bit();
       toAscii() is not offered as an alternative...] */
    doKeys(k.toLatin1().constData());
}

// queue up the command name for a function, as if user had typed it
void NetHackQtMainWindow::FuncAsCommand(int NDECL((*func)))
{
    char cmdbuf[32];
    Strcpy(cmdbuf, "#");
    (void) cmdname_from_func(func, &cmdbuf[1], FALSE);
    doKeys(cmdbuf);
}

void NetHackQtMainWindow::AddMessageWindow(NetHackQtMessageWindow* window)
{
    message=window;
    if (!qt_compact_mode)
        hsplitter->insertWidget(0, message->Widget());
    ShowIfReady();
}

NetHackQtMessageWindow * NetHackQtMainWindow::GetMessageWindow()
{
    return message;
}

void NetHackQtMainWindow::AddMapWindow(NetHackQtMapWindow2* window)
{

    map=window;
    if (!qt_compact_mode)
        vsplitter->insertWidget(1, map->Widget());
    ShowIfReady();
    connect(map,SIGNAL(resized()),this,SLOT(layout()));
}

void NetHackQtMainWindow::AddStatusWindow(NetHackQtStatusWindow* window)
{
    status=window;
    if (!qt_compact_mode)
        hsplitter->insertWidget(2, status->Widget());
    ShowIfReady();
}

void NetHackQtMainWindow::RemoveWindow(NetHackQtWindow* window)
{
    if (window==status) {
	status=0;
	ShowIfReady();
    } else if (window==map) {
	map=0;
	ShowIfReady();
    } else if (window==message) {
	message=0;
	ShowIfReady();
    }
}

void NetHackQtMainWindow::updateInventory()
{
    if (invusage) {
	invusage->repaint();
    }
}

void NetHackQtMainWindow::fadeHighlighting(bool before_key)
{
    if (before_key) {
        // status highlighting fades at start of turn
        if (status)
            status->fadeHighlighting();
    } else {
        // message highlighting fades after user has given input
        if (message && message->hilit_mesgs())
            message->unhighlight_mesgs();
    }
}

void NetHackQtMainWindow::layout()
{
#if 0
    if ( qt_compact_mode )
	return;
    if (message && map && status) {
	QSize maxs=map->Widget()->maximumSize();
	int maph=std::min(height()*2/3,maxs.height());

	QWidget* c = centralWidget();
	int h=c->height();
	int toph=h-maph;
	int iuw=3*qt_settings->glyphs().width();
	int topw=(c->width()-iuw)/2;

	message->Widget()->setGeometry(0,0,topw,toph);
	invusage->setGeometry(topw,0,iuw,toph);
	status->Widget()->setGeometry(topw+iuw,0,topw,toph);
	map->Widget()->setGeometry(std::max(0,(c->width()-maxs.width())/2),
				   toph,c->width(),maph);
    }
#endif

    if (qt_settings && !qt_compact_mode
        && map && message && status && invusage) {
        // For the initial PaperDoll sizing, message window
        // and/or status window might still be empty;
        // widen them before changing PaperDoll to use saved settings.
        QList<int> splittersizes = hsplitter->sizes();
#define MIN_WIN_WIDTH 400
        if (splittersizes[0] < MIN_WIN_WIDTH
            || splittersizes[2] < MIN_WIN_WIDTH) {
            if (splittersizes[0] < MIN_WIN_WIDTH)
                splittersizes[0] = MIN_WIN_WIDTH;
#ifndef ENHANCED_PAPERDOLL
            if (splittersizes[1] < 6) // TILEWMIN
                splittersizes[1] = 16; // 16x16
#endif
            if (splittersizes[2] < MIN_WIN_WIDTH)
                splittersizes[2] = MIN_WIN_WIDTH;
            hsplitter->setSizes(splittersizes);
        }
#ifdef ENHANCED_PAPERDOLL
        // call resizePaperDoll() indirectly...
        qt_settings->resizeDoll();
#endif
    }
}

void NetHackQtMainWindow::resizePaperDoll(bool showdoll)
{
#ifdef ENHANCED_PAPERDOLL
    // this is absurd...
    NetHackQtInvUsageWindow *w = static_cast <NetHackQtMainWindow *>
                                 (NetHackQtBind::mainWidget())->invusage;
    QList<int> hsplittersizes = hsplitter->sizes(),
              vsplittersizes = vsplitter->sizes();
    w->resize(w->sizeHint());

    int oldwidth = hsplittersizes[1],
        newwidth = w->width();
    if (newwidth != oldwidth) {
        if (oldwidth > newwidth)
            hsplittersizes[0] += (oldwidth - newwidth);
        else
            hsplittersizes[2] += (newwidth - oldwidth);
        hsplittersizes[1] = newwidth;
        hsplitter->setSizes(hsplittersizes);
    }

    // Height limit is 48 pixels per doll cell;
    // values greater than 44 need taller window which pushes the map down.
    // FIXME: this doesn't shrink the window back if size is reduced from 45+
    int oldheight = vsplittersizes[0],
        newheight = w->height();
    if (newheight > oldheight && oldheight > 0 && vsplittersizes[1] > 0) {
        vsplittersizes[0] = newheight;
        vsplitter->setSizes(vsplittersizes);
    }

    if (showdoll) {
        if (w->isHidden())
            w->show();
        else
            w->repaint();
    } else {
        if (w->isVisible())
            w->hide();
    }
#else
    nhUse(showdoll);
#endif /* ENHANCED_PAPERDOLL */
}

void NetHackQtMainWindow::resizeEvent(QResizeEvent*)
{
    layout();
#ifdef KDE
    updateRects();
#endif
}

void NetHackQtMainWindow::keyReleaseEvent(QKeyEvent* event)
{
    if ( dirkey ) {
	doKeys(QString(QChar(dirkey)));
	if ( !event->isAutoRepeat() )
	    dirkey = 0;
    }
}

void NetHackQtMainWindow::keyPressEvent(QKeyEvent* event)
{
    // Global key controls

    // For desktop, arrow keys scroll map, since we don't want players
    // to think that's the way to move. For handhelds, the normal way is to
    // click-to-travel, so we allow the cursor keys for fine movements.

    //  321
    //  4 0
    //  567

    if ( event->isAutoRepeat() &&
	event->key() >= Qt::Key_Left && event->key() <= Qt::Key_Down )
	return;

    const char* d = g.Cmd.dirchars;
    switch (event->key()) {
    case Qt::Key_Up:
	if ( dirkey == d[0] )
	    dirkey = d[1];
	else if ( dirkey == d[4] )
	    dirkey = d[3];
	else
	    dirkey = d[2];
        break;
    case Qt::Key_Down:
	if ( dirkey == d[0] )
	    dirkey = d[7];
	else if ( dirkey == d[4] )
	    dirkey = d[5];
	else
	    dirkey = d[6];
        break;
    case Qt::Key_Left:
	if ( dirkey == d[2] )
	    dirkey = d[1];
	else if ( dirkey == d[6] )
	    dirkey = d[7];
	else
	    dirkey = d[0];
        break;
    case Qt::Key_Right:
	if ( dirkey == d[2] )
	    dirkey = d[3];
	else if ( dirkey == d[6] )
	    dirkey = d[5];
	else
	    dirkey = d[4];
        break;
    case Qt::Key_PageUp:
	dirkey = 0;
	if (message) message->Scroll(0,-1);
        break;
    case Qt::Key_PageDown:
	dirkey = 0;
	if (message) message->Scroll(0,+1);
        break;
    case Qt::Key_Space:
        //if (flags.rest_on_space) {
        event->ignore(); // punt to NetHackQtBind::notify()
        return;
        //}
    case Qt::Key_Enter:
	if ( map )
	    map->clickCursor();
        break;
    default:
	dirkey = 0;
	event->ignore();
        break;
    }
}

// game window's Close button has been activated
void NetHackQtMainWindow::closeEvent(QCloseEvent *e UNUSED)
{
    int ok = 0;
    if ( g.program_state.something_worth_saving ) {
        /* this used to offer "Save" and "Cancel"
           but cancel (ignoring the close attempt) won't work
           if user has clicked on the window's Close button */
	int act = QMessageBox::information(this, "NetHack",
                              "This will end your NetHack session.",
                              "&Save and exit", "&Quit without saving", 0, 1);
	switch (act) {
        case 0:
            // See dosave() function
            ok = dosave0();
            break;
        case 1:
            // quit -- bypass the prompting preformed by done2()
            ok = 1;
            g.program_state.stopprint++;
            ::done(QUIT);
            /*NOTREACHED*/
            break;
	}
    } else {
        /* nothing worth saving; just close/quit */
        ok = 1;
    }
    /* if !ok, we should try to continue, but we don't... */
    u.uhp = -1;
    NetHackQtBind::qt_exit_nhwindows(0);
    nh_terminate(EXIT_SUCCESS);
}

void NetHackQtMainWindow::ShowIfReady()
{
    if (message && map && status) {
        QWidget* hp = qt_compact_mode ? static_cast<QWidget *>(stack)
                                      : static_cast<QWidget *>(hsplitter);
        QWidget* vp = qt_compact_mode ? static_cast<QWidget *>(stack)
                                      : static_cast<QWidget *>(vsplitter);
	message->Widget()->setParent(hp);
	map->Widget()->setParent(vp);
	status->Widget()->setParent(hp);
	if ( qt_compact_mode ) {
	    message->setMap(map);
	    stack->addWidget(map->Widget());
	    stack->addWidget(message->Widget());
	    stack->addWidget(status->Widget());
	    raiseMap();
	} else {
	    layout();
	}
	showMaximized();
    } else if (isVisible()) {
	hide();
    }
}

} // namespace nethack_qt_
