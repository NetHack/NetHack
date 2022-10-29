// NetHack 3.6	qt_win.cpp	$NHDT-Date: 1596404695 2020/08/02 21:44:55 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.91 $
// Copyright (c) Warwick Allison, 1999.
// NetHack may be freely redistributed.  See license for details.

// Qt Binding for NetHack 3.4
//
// Copyright (C) 1996-2001 by Warwick W. Allison (warwick@troll.no)
// 
// Contributors:
//    Michael Hohmuth <hohmuth@inf.tu-dresden.de>
//       - Userid control
//    Svante Gerhard <svante@algonet.se>
//       - .nethackrc tile and font size settings
//    Dirk Schoenberger <schoenberger@signsoft.com>
//       - KDE support
//       - SlashEm support
//    and many others for bug reports.
// 
// Unfortunately, this doesn't use Qt as well as I would like,
// primarily because NetHack is fundamentally a getkey-type program
// rather than being event driven (hence the ugly key and click buffer)
// and also because this is my first major application of Qt.  
// 
// The problem of NetHack's getkey requirement is solved by intercepting
// key events by overiding QApplicion::notify(...), and putting them in
// a buffer.  Mouse clicks on the map window are treated with a similar
// buffer.  When the NetHack engine calls for a key, one is taken from
// the buffer, or if that is empty, QApplication::enter_loop() is called.
// Whenever keys or clicks go into the buffer, QApplication::exit_loop()
// is called.
//
// Another problem is that some NetHack players are decade-long players who
// demand complete keyboard control (while Qt and X11 conspire to make this
// difficult by having widget-based focus rather than application based -
// a good thing in general).  This problem is solved by again using the key
// event buffer.
//
// Out of all this hackery comes a silver lining however, as macros for
// the super-expert and menus for the ultra-newbie are also made possible
// by the key event buffer.
//

extern "C" {

// This includes all the definitions we need from the NetHack main
// engine.  We pretend MSC is a STDC compiler, because C++ is close
// enough, and we undefine NetHack macros which conflict with Qt
// identifiers.

#define alloc hide_alloc // avoid treading on STL symbol
#define lock hide_lock // avoid treading on STL symbol
#ifdef _MSC_VER
#define NHSTDC
#endif
#include "hack.h"
#include "func_tab.h"
#include "dlb.h"
#include "patchlevel.h"
#include "tile2x11.h"
#undef Invisible
#undef Warning
#undef red
#undef green
#undef blue
#undef Black
#undef curs
#undef TRUE
#undef FALSE
#undef min
#undef max
#undef alloc
#undef lock
#undef yn

}

#include "qt3_win.h"
#include <qregexp.h>
#include <qpainter.h>
#include <qdir.h>
#include <qbitmap.h>
#include <qkeycode.h>
#include <qmenubar.h>
#include <qpopupmenu.h>
#include <qlayout.h>
#include <qheader.h>
#include <qradiobutton.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qcombobox.h>
#include <qvbox.h>
#include <qdragobject.h>
#include <qtextbrowser.h>
#include <qhbox.h>
#include <qsignalmapper.h>
//#include <qgrid.h>
//#include <qlabelled.h>

#include <ctype.h>

#include "qt3_clust.h"
#include "qt3_xpms.h"

#include <dirent.h>
#ifdef Q_WS_MACX
#  include <sys/malloc.h>
#else
#  include <malloc.h>
#endif

#ifdef _WS_X11_
// For userid control
#include <unistd.h>
#endif

// Some distributors released Qt 2.1.0beta4
#if QT_VERSION < 220
# define nh_WX11BypassWM 0x01000000
#else
# define nh_WX11BypassWM WX11BypassWM
#endif

#ifdef USER_SOUNDS
# if QT_VERSION < 220
#  undef USER_SOUNDS
# else
#  include <qsound.h>
# endif
#endif


#ifdef USER_SOUNDS
extern "C" void play_sound_for_message(const char* str);
#endif

#ifdef SAFERHANGUP
#include <qtimer.h>
#endif

// Warwick prefers it this way...
#define QT_CHOOSE_RACE_FIRST

static const char nh_attribution[] = "<center><big>NetHack</big>"
	"<br><small>by the NetHack DevTeam</small></center>";

static QString
aboutMsg()
{
    char vbuf[BUFSZ];
    QString msg;
    msg.sprintf(
        // format
        "Qt NetHack is a version of NetHack\n"
        "built using"           // no newline
#ifdef KDE
        " KDE and"              // ditto
#endif
        " the Qt %d GUI toolkit.\n"
        "\nThis is NetHack %s%s.\n"
        "\nNetHack's Qt interface originally developed by Warwick Allison.\n"
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
        "NetHack:\n     %s\n",
        // arguments
#ifdef QT_VERSION_MAJOR
        QT_VERSION_MAJOR,
#else
        3,              // Qt version macro should exist; if not, assume Qt3
#endif
        version_string(vbuf), /* nethack version */
#ifdef QT_VERSION_STR
        " with Qt " QT_VERSION_STR,
#else
        "",
#endif
        DEVTEAM_URL);
    return msg;
}

static void
centerOnMain( QWidget* w )
{
    QWidget* m = qApp->mainWidget();
    if (!m) m = qApp->desktop();
    QPoint p = m->mapToGlobal(QPoint(0,0));
    w->move( p.x() + m->width()/2  - w->width()/2,
              p.y() + m->height()/2 - w->height()/2 );
}

NetHackQtLineEdit::NetHackQtLineEdit() :
    QLineEdit(0)
{
}

NetHackQtLineEdit::NetHackQtLineEdit(QWidget* parent, const char* name) :
    QLineEdit(parent,name)
{
}

void NetHackQtLineEdit::fakeEvent(int key, int ascii, int state)
{
    QKeyEvent fake(QEvent::KeyPress,key,ascii,state);
    keyPressEvent(&fake);
}

extern "C" {
/* Used by tile/font-size patch below and in ../../src/files.c */
char *qt_tilewidth=NULL;
char *qt_tileheight=NULL;
char *qt_fontsize=NULL;
#if defined(QWS)
int qt_compact_mode = 1;
#else
int qt_compact_mode = 0;
#endif
extern const char *enc_stat[]; /* from botl.c */
extern const char *hu_stat[]; /* from eat.c */
extern int total_tiles_used; // from tile.c
extern short glyph2tile[]; // from tile.c
}

static int tilefile_tile_W=16;
static int tilefile_tile_H=16;

#define TILEWMIN 1
#define TILEHMIN 1


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
static const char * cast_a_xpm[] = {
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
static const char * cast_b_xpm[] = {
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
static const char * cast_c_xpm[] = {
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

NetHackQtSettings::NetHackQtSettings(int w, int h) :
    tilewidth(TILEWMIN,64,1,this),
    tileheight(TILEHMIN,64,1,this),
    widthlbl(&tilewidth,"&Width:",this),
    heightlbl(&tileheight,"&Height:",this),
    whichsize("&Zoomed",this),
    fontsize(this),
    normal("times"),
#ifdef WS_WIN
    normalfixed("courier new"),
#else
    normalfixed("fixed"),
#endif
    large("times"),
    theglyphs(0)

{
    int default_fontsize;

    if (w<=300) {
	// ~240x320
	default_fontsize=4;
	tilewidth.setValue(8);
	tileheight.setValue(12);
    } else if (w<=700) {
	// ~640x480
	default_fontsize=3;
	tilewidth.setValue(8);
	tileheight.setValue(14);
    } else if (w<=900) {
	// ~800x600
	default_fontsize=3;
	tilewidth.setValue(10);
	tileheight.setValue(17);
    } else if (w<=1100) {
	// ~1024x768
	default_fontsize=2;
	tilewidth.setValue(12);
	tileheight.setValue(22);
    } else if (w<=1200) {
	// ~1152x900
	default_fontsize=1;
	tilewidth.setValue(14);
	tileheight.setValue(26);
    } else {
	// ~1280x1024 and larger
	default_fontsize=0;
	tilewidth.setValue(16);
	tileheight.setValue(30);
    }

    // Tile/font sizes read from .nethackrc
    if (qt_tilewidth != NULL) {
	tilewidth.setValue(atoi(qt_tilewidth));
	free(qt_tilewidth);
    }
    if (qt_tileheight != NULL) {
	tileheight.setValue(atoi(qt_tileheight));
	free(qt_tileheight);
    }
    if (qt_fontsize != NULL) {
	switch (tolower(qt_fontsize[0])) {
	  case 'h': default_fontsize = 0; break;
	  case 'l': default_fontsize = 1; break;
	  case 'm': default_fontsize = 2; break;
	  case 's': default_fontsize = 3; break;
	  case 't': default_fontsize = 4; break;
	}
	free(qt_fontsize); 
    }

    theglyphs=new NetHackQtGlyphs();
    resizeTiles();

    connect(&tilewidth,SIGNAL(valueChanged(int)),this,SLOT(resizeTiles()));
    connect(&tileheight,SIGNAL(valueChanged(int)),this,SLOT(resizeTiles()));
    connect(&whichsize,SIGNAL(toggled(bool)),this,SLOT(setGlyphSize(bool)));

    fontsize.insertItem("Huge");
    fontsize.insertItem("Large");
    fontsize.insertItem("Medium");
    fontsize.insertItem("Small");
    fontsize.insertItem("Tiny");
    fontsize.setCurrentItem(default_fontsize);
    connect(&fontsize,SIGNAL(activated(int)),this,SIGNAL(fontChanged()));

    QGridLayout* grid = new QGridLayout(this, 5, 2, 8);
    grid->addMultiCellWidget(&whichsize, 0, 0, 0, 1);
    grid->addWidget(&tilewidth, 1, 1);  grid->addWidget(&widthlbl, 1, 0);
    grid->addWidget(&tileheight, 2, 1); grid->addWidget(&heightlbl, 2, 0);
    QLabel* flabel=new QLabel(&fontsize, "&Font:",this);
    grid->addWidget(flabel, 3, 0); grid->addWidget(&fontsize, 3, 1);
    QPushButton* dismiss=new QPushButton("Dismiss",this);
    dismiss->setDefault(TRUE);
    grid->addMultiCellWidget(dismiss, 4, 4, 0, 1);
    grid->setRowStretch(4,0);
    grid->setColStretch(1,1);
    grid->setColStretch(2,2);
    grid->activate();

    connect(dismiss,SIGNAL(clicked()),this,SLOT(accept()));
    resize(150,140);
}

NetHackQtGlyphs& NetHackQtSettings::glyphs()
{
    return *theglyphs;
}

void NetHackQtSettings::resizeTiles()
{
    int w = tilewidth.value();
    int h = tileheight.value();

    theglyphs->setSize(w,h);
    emit tilesChanged();
}

void NetHackQtSettings::toggleGlyphSize()
{
    whichsize.toggle();
}

void NetHackQtSettings::setGlyphSize(bool which)
{
    QSize n = QSize(tilewidth.value(),tileheight.value());
    if ( othersize.isValid() ) {
	tilewidth.blockSignals(TRUE);
	tileheight.blockSignals(TRUE);
	tilewidth.setValue(othersize.width());
	tileheight.setValue(othersize.height());
	tileheight.blockSignals(FALSE);
	tilewidth.blockSignals(FALSE);
	resizeTiles();
    }
    othersize = n;
}

const QFont& NetHackQtSettings::normalFont()
{
    static int size[]={ 18, 14, 12, 10, 8 };
    normal.setPointSize(size[fontsize.currentItem()]);
    return normal;
}

const QFont& NetHackQtSettings::normalFixedFont()
{
    static int size[]={ 18, 14, 13, 10, 8 };
    normalfixed.setPointSize(size[fontsize.currentItem()]);
    return normalfixed;
}

const QFont& NetHackQtSettings::largeFont()
{
    static int size[]={ 24, 18, 14, 12, 10 };
    large.setPointSize(size[fontsize.currentItem()]);
    return large;
}

bool NetHackQtSettings::ynInMessages()
{
    return !qt_compact_mode;
}


NetHackQtSettings* qt_settings;



NetHackQtKeyBuffer::NetHackQtKeyBuffer() :
    in(0), out(0)
{
}

bool NetHackQtKeyBuffer::Empty() const { return in==out; }
bool NetHackQtKeyBuffer::Full() const { return (in+1)%maxkey==out; }

void NetHackQtKeyBuffer::Put(int k, int a, int state)
{
    if ( Full() ) return;	// Safety
    key[in]=k;
    ascii[in]=a;
    in=(in+1)%maxkey;
}

void NetHackQtKeyBuffer::Put(char a)
{
    Put(0,a,0);
}

void NetHackQtKeyBuffer::Put(const char* str)
{
    while (*str) Put(*str++);
}

int NetHackQtKeyBuffer::GetKey()
{
    if ( Empty() ) return 0;
    int r=TopKey();
    out=(out+1)%maxkey;
    return r;
}

int NetHackQtKeyBuffer::GetAscii()
{
    if ( Empty() ) return 0; // Safety
    int r=TopAscii();
    out=(out+1)%maxkey;
    return r;
}

int NetHackQtKeyBuffer::GetState()
{
    if ( Empty() ) return 0;
    int r=TopState();
    out=(out+1)%maxkey;
    return r;
}

int NetHackQtKeyBuffer::TopKey() const
{
    if ( Empty() ) return 0;
    return key[out];
}

int NetHackQtKeyBuffer::TopAscii() const
{
    if ( Empty() ) return 0;
    return ascii[out];
}

int NetHackQtKeyBuffer::TopState() const
{
    if ( Empty() ) return 0;
    return state[out];
}


NetHackQtClickBuffer::NetHackQtClickBuffer() :
    in(0), out(0)
{
}

bool NetHackQtClickBuffer::Empty() const { return in==out; }
bool NetHackQtClickBuffer::Full() const { return (in+1)%maxclick==out; }

void NetHackQtClickBuffer::Put(int x, int y, int mod)
{
    click[in].x=x;
    click[in].y=y;
    click[in].mod=mod;
    in=(in+1)%maxclick;
}

int NetHackQtClickBuffer::NextX() const { return click[out].x; }
int NetHackQtClickBuffer::NextY() const { return click[out].y; }
int NetHackQtClickBuffer::NextMod() const { return click[out].mod; }

void NetHackQtClickBuffer::Get()
{
    out=(out+1)%maxclick;
}

class NhPSListViewItem : public QListViewItem {
public:
    NhPSListViewItem( QListView* parent, const QString& name ) :
	QListViewItem(parent, name)
    {
    }

    void setGlyph(int g)
    {
	NetHackQtGlyphs& glyphs = qt_settings->glyphs();
	int gw = glyphs.width();
	int gh = glyphs.height();
	QPixmap pm(gw,gh);
	QPainter p(&pm);
	glyphs.drawGlyph(p, g, 0, 0);
	p.end();
	setPixmap(0,pm);
	setHeight(QMAX(pm.height()+1,height()));
    }

    void paintCell( QPainter *p, const QColorGroup &cg,
		    int column, int width, int alignment )
    {
	if ( isSelectable() ) {
	    QListViewItem::paintCell( p, cg, column, width, alignment );
	} else {
	    QColorGroup disabled(
		cg.foreground().light(),
		cg.button().light(),
		cg.light(), cg.dark(), cg.mid(),
		gray, cg.base() );
	    QListViewItem::paintCell( p, disabled, column, width, alignment );
	}
    }
};

class NhPSListViewRole : public NhPSListViewItem {
public:
    NhPSListViewRole( QListView* parent, int id ) :
	NhPSListViewItem(parent,
#ifdef QT_CHOOSE_RACE_FIRST // Lowerize - looks better
	    QString(QChar(roles[id].name.m[0])).lower()+QString(roles[id].name.m+1)
#else
	    roles[id].name.m
#endif
	)
    {
	setGlyph(monnum_to_glyph(roles[id].mnum));
    }
};

class NhPSListViewRace : public NhPSListViewItem {
public:
    NhPSListViewRace( QListView* parent, int id ) :
	NhPSListViewItem(parent,
#ifdef QT_CHOOSE_RACE_FIRST // Capitalize - looks better
	    QString(QChar(races[id].noun[0])).upper()+QString(races[id].noun+1)
#else
	    QString(QChar(races[id].noun[0])+QString(races[id].noun+1))
#endif
	)
    {
	setGlyph(monnum_to_glyph(races[id].mnum));
    }
};

class NhPSListView : public QListView {
public:
    NhPSListView( QWidget* parent ) :
	QListView(parent)
    {
	setSorting(-1); // order is identity
	header()->setClickEnabled(FALSE);
    }

    QSizePolicy sizePolicy() const
    {
	return QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    }

    QSize minimumSizeHint() const
    {
	return sizeHint();
    }

    QSize sizeHint() const
    {
	QListView::sizeHint();
	QSize sz = header()->sizeHint();
	int h=0;
	QListViewItem* c=firstChild();
	while (c) h+=c->height(),c = c->nextSibling();
	sz += QSize(frameWidth()*2, h+frameWidth()*2);
	return sz;
    }

    int selectedItemNumber() const
    {
	int i=0;
	QListViewItem* c = firstChild();
	while (c) {
	    if (c == selectedItem()) {
		return i;
	    }
	    i++;
	    c = c->nextSibling();
	}
	return -1;
    }

    void setSelectedItemNumber(int i)
    {
	QListViewItem* c=firstChild();
	while (i--)
	    c = c->nextSibling();
	c->setSelected(TRUE);
    }
};

NetHackQtPlayerSelector::NetHackQtPlayerSelector(NetHackQtKeyBuffer& ks) :
    QDialog(qApp->mainWidget(),"plsel",TRUE),
    keysource(ks),
    fully_specified_role(TRUE)
{
    /*
               0             1             2
	  + Name ------------------------------------+
	0 |                                          |
	  + ---- ------------------------------------+
	  + Role ---+   + Race ---+   + Gender ------+
	  |         |   |         |   |  * Male      |
	1 |         |   |         |   |  * Female    |
	  |         |   |         |   +--------------+
	  |         |   |         |   
	  |         |   |         |   + Alignment ---+
	2 |         |   |         |   |  * Male      |
	  |         |   |         |   |  * Female    |
	  |         |   |         |   +--------------+
	3 |         |   |         |   ...stretch...
	  |         |   |         |   
	4 |         |   |         |   [  Play  ]
	5 |         |   |         |   [  Quit  ]
	  +---------+   +---------+   
    */

    int marg=4;
    QGridLayout *l = new QGridLayout(this,6,3,marg,marg);

    QButtonGroup* namebox = new QButtonGroup(1,Horizontal,"Name",this);
    QLineEdit* name = new QLineEdit(namebox);
    name->setMaxLength(sizeof(g.plname)-1);
    if ( strncmp(g.plname,"player",6) && strncmp(g.plname,"games",5) )
	name->setText(g.plname);
    connect(name, SIGNAL(textChanged(const QString&)),
	    this, SLOT(selectName(const QString&)) );
    name->setFocus();
    QButtonGroup* genderbox = new QButtonGroup("Sex",this);
    QButtonGroup* alignbox = new QButtonGroup("Alignment",this);
    QVBoxLayout* vbgb = new QVBoxLayout(genderbox,3,1);
    vbgb->setAutoAdd(TRUE);
    vbgb->addSpacing(fontMetrics().height()*3/4);
    QVBoxLayout* vbab = new QVBoxLayout(alignbox,3,1);
    vbab->setAutoAdd(TRUE);
    vbab->addSpacing(fontMetrics().height());
    QLabel* logo = new QLabel(nh_attribution, this);

    l->addMultiCellWidget( namebox, 0,0,0,2 );
#ifdef QT_CHOOSE_RACE_FIRST
    race = new NhPSListView(this);
    role = new NhPSListView(this);
    l->addMultiCellWidget( race, 1,5,0,0 );
    l->addMultiCellWidget( role, 1,5,1,1 );
#else
    role = new NhPSListView(this);
    race = new NhPSListView(this);
    l->addMultiCellWidget( role, 1,5,0,0 );
    l->addMultiCellWidget( race, 1,5,1,1 );
#endif
    role->addColumn("Role");
    race->addColumn("Race");

    l->addWidget( genderbox, 1, 2 );
    l->addWidget( alignbox, 2, 2 );
    l->addWidget( logo, 3, 2, AlignCenter );
    l->setRowStretch( 3, 5 );

    int i;
    int nrole;

    for (nrole=0; roles[nrole].name.m; nrole++)
	;
    for (i=nrole-1; i>=0; i--) { // XXX QListView unsorted goes in rev.
	new NhPSListViewRole( role, i );
    }
    connect( role, SIGNAL(selectionChanged()), this, SLOT(selectRole()) );

    int nrace;
    for (nrace=0; races[nrace].noun; nrace++)
	;
    for (i=nrace-1; i>=0; i--) {
	new NhPSListViewRace( race, i );
    }
    connect( race, SIGNAL(selectionChanged()), this, SLOT(selectRace()) );

    gender = new QRadioButton*[ROLE_GENDERS];
    for (i=0; i<ROLE_GENDERS; i++) {
	gender[i] = new QRadioButton( genders[i].adj, genderbox );
    }
    connect( genderbox, SIGNAL(clicked(int)), this, SLOT(selectGender(int)) );

    alignment = new QRadioButton*[ROLE_ALIGNS];
    for (i=0; i<ROLE_ALIGNS; i++) {
	alignment[i] = new QRadioButton( aligns[i].adj, alignbox );
    }
    connect( alignbox, SIGNAL(clicked(int)), this, SLOT(selectAlignment(int)) );

    QPushButton* ok = new QPushButton("Play",this);
    l->addWidget( ok, 4, 2 );
    ok->setDefault(TRUE);
    connect( ok, SIGNAL(clicked()), this, SLOT(accept()) );

    QPushButton* cancel = new QPushButton("Quit",this);
    l->addWidget( cancel, 5, 2 );
    connect( cancel, SIGNAL(clicked()), this, SLOT(reject()) );

    // Randomize race and role, unless specified in config
    int ro = flags.initrole;
    if (ro == ROLE_NONE || ro == ROLE_RANDOM) {
	ro = rn2(nrole);
	if (flags.initrole != ROLE_RANDOM) {
	    fully_specified_role = FALSE;
	}
    }
    int ra = flags.initrace;
    if (ra == ROLE_NONE || ra == ROLE_RANDOM) {
	ra = rn2(nrace);
	if (flags.initrace != ROLE_RANDOM) {
	    fully_specified_role = FALSE;
	}
    }

    // make sure we have a valid combination, honoring 
    // the users request if possible.
    bool choose_race_first;
#ifdef QT_CHOOSE_RACE_FIRST
    choose_race_first = TRUE;
    if (flags.initrole >= 0 && flags.initrace < 0) {
	choose_race_first = FALSE;
    }
#else
    choose_race_first = FALSE;
    if (flags.initrace >= 0 && flags.initrole < 0) {
	choose_race_first = TRUE;
    }
#endif
    while (!validrace(ro,ra)) {
	if (choose_race_first) {
	    ro = rn2(nrole);
	    if (flags.initrole != ROLE_RANDOM) {
	        fully_specified_role = FALSE;
	    }
	} else {
	    ra = rn2(nrace);
	    if (flags.initrace != ROLE_RANDOM) {
	        fully_specified_role = FALSE;
	    }
	}
    }

    int g = flags.initgend;
    if (g == -1) {
	g = rn2(ROLE_GENDERS);
	fully_specified_role = FALSE;
    }
    while (!validgend(ro,ra,g)) {
	g = rn2(ROLE_GENDERS);
    }
    gender[g]->setChecked(TRUE);
    selectGender(g);

    int a = flags.initalign;
    if (a == -1) {
	a = rn2(ROLE_ALIGNS);
	fully_specified_role = FALSE;
    }
    while (!validalign(ro,ra,a)) {
	a = rn2(ROLE_ALIGNS);
    }
    alignment[a]->setChecked(TRUE);
    selectAlignment(a);

    QListViewItem* li;

    li = role->firstChild();
    while (ro--) li=li->nextSibling();
    role->setSelected(li,TRUE);

    li = race->firstChild();
    while (ra--) li=li->nextSibling();
    race->setSelected(li,TRUE);

    flags.initrace = race->selectedItemNumber();
    flags.initrole = role->selectedItemNumber();
}


void NetHackQtPlayerSelector::selectName(const QString& n)
{
    strncpy(g.plname,n.latin1(),sizeof(g.plname)-1);
}

void NetHackQtPlayerSelector::selectRole()
{
    int ra = race->selectedItemNumber();
    int ro = role->selectedItemNumber();
    if (ra == -1 || ro == -1) return;

#ifdef QT_CHOOSE_RACE_FIRST
    selectRace();
#else
    QListViewItem* i=role->currentItem();
    QListViewItem* valid=0;
    int j;
    NhPSListViewItem* item;
    item = (NhPSListViewItem*)role->firstChild();
    for (j=0; roles[j].name.m; j++) {
	bool v = validrace(j,ra);
	item->setSelectable(TRUE);
	if ( !valid && v ) valid = item;
	item=(NhPSListViewItem*)item->nextSibling();
    }
    if ( !validrace(role->selectedItemNumber(),ra) )
	i = valid;
    role->setSelected(i,TRUE);
    item = (NhPSListViewItem*)role->firstChild();
    for (j=0; roles[j].name.m; j++) {
	bool v = validrace(j,ra);
	item->setSelectable(v);
	item->repaint();
	item=(NhPSListViewItem*)item->nextSibling();
    }
#endif

    flags.initrole = role->selectedItemNumber();
    setupOthers();
}

void NetHackQtPlayerSelector::selectRace()
{
    int ra = race->selectedItemNumber();
    int ro = role->selectedItemNumber();
    if (ra == -1 || ro == -1) return;

#ifndef QT_CHOOSE_RACE_FIRST
    selectRole();
#else
    QListViewItem* i=race->currentItem();
    QListViewItem* valid=0;
    int j;
    NhPSListViewItem* item;
    item = (NhPSListViewItem*)race->firstChild();
    for (j=0; races[j].noun; j++) {
	bool v = validrace(ro,j);
	item->setSelectable(TRUE);
	if ( !valid && v ) valid = item;
	item=(NhPSListViewItem*)item->nextSibling();
    }
    if ( !validrace(ro,race->selectedItemNumber()) )
	i = valid;
    race->setSelected(i,TRUE);
    item = (NhPSListViewItem*)race->firstChild();
    for (j=0; races[j].noun; j++) {
	bool v = validrace(ro,j);
	item->setSelectable(v);
	item->repaint();
	item=(NhPSListViewItem*)item->nextSibling();
    }
#endif

    flags.initrace = race->selectedItemNumber();
    setupOthers();
}

void NetHackQtPlayerSelector::setupOthers()
{
    int ro = role->selectedItemNumber();
    int ra = race->selectedItemNumber();
    int valid=-1;
    int c=0;
    int j;
    for (j=0; j<ROLE_GENDERS; j++) {
	bool v = validgend(ro,ra,j);
	if ( gender[j]->isChecked() )
	    c = j;
	gender[j]->setEnabled(v);
	if ( valid<0 && v ) valid = j;
    }
    if ( !validgend(ro,ra,c) )
	c = valid;
    int k;
    for (k=0; k<ROLE_GENDERS; k++) {
	gender[k]->setChecked(c==k);
    }
    selectGender(c);

    valid=-1;
    for (j=0; j<ROLE_ALIGNS; j++) {
	bool v = validalign(ro,ra,j);
	if ( alignment[j]->isChecked() )
	    c = j;
	alignment[j]->setEnabled(v);
	if ( valid<0 && v ) valid = j;
    }
    if ( !validalign(ro,ra,c) )
	c = valid;
    for (k=0; k<ROLE_ALIGNS; k++) {
	alignment[k]->setChecked(c==k);
    }
    selectAlignment(c);
}

void NetHackQtPlayerSelector::selectGender(int i)
{
    flags.initgend = i;
}

void NetHackQtPlayerSelector::selectAlignment(int i)
{
    flags.initalign = i;
}


void NetHackQtPlayerSelector::done(int i)
{
    setResult(i);
    qApp->exit_loop();
}

void NetHackQtPlayerSelector::Quit()
{
    done(R_Quit);
    qApp->exit_loop();
}

void NetHackQtPlayerSelector::Random()
{
    done(R_Rand);
    qApp->exit_loop();
}

bool NetHackQtPlayerSelector::Choose()
{
    if (fully_specified_role) return TRUE;

#if defined(QWS) // probably safe with Qt 3, too (where show!=exec in QDialog).
    if ( qt_compact_mode ) {
	showMaximized();
    } else
#endif
    {
	adjustSize();
	centerOnMain(this);
    }

    if ( exec() ) {
	return TRUE;
    } else {
	return FALSE;
    }
}


NetHackQtStringRequestor::NetHackQtStringRequestor(NetHackQtKeyBuffer& ks, const char* p, const char* cancelstr) :
    QDialog(qApp->mainWidget(),"string",FALSE),
    prompt(p,this,"prompt"),
    input(this,"input"),
    keysource(ks)
{
    cancel=new QPushButton(cancelstr,this);
    connect(cancel,SIGNAL(clicked()),this,SLOT(reject()));

    okay=new QPushButton("Okay",this);
    connect(okay,SIGNAL(clicked()),this,SLOT(accept()));
    connect(&input,SIGNAL(returnPressed()),this,SLOT(accept()));
    okay->setDefault(TRUE);

    setFocusPolicy(StrongFocus);
}

void NetHackQtStringRequestor::resizeEvent(QResizeEvent*)
{
    const int margin=5;
    const int gutter=5;

    int h=(height()-margin*2-gutter);

    if (strlen(prompt.text()) > 16) {
	h/=3;
	prompt.setGeometry(margin,margin,width()-margin*2,h);
	input.setGeometry(width()*1/5,margin+h+gutter,
	    (width()-margin-2-gutter)*4/5,h);
    } else {
	h/=2;
	prompt.setGeometry(margin,margin,(width()-margin*2-gutter)*2/5,h);
	input.setGeometry(prompt.geometry().right()+gutter,margin,
	    (width()-margin-2-gutter)*3/5,h);
    }

    cancel->setGeometry(margin,input.geometry().bottom()+gutter,
	(width()-margin*2-gutter)/2,h);
    okay->setGeometry(cancel->geometry().right()+gutter,cancel->geometry().y(),
	cancel->width(),h);
}

void NetHackQtStringRequestor::SetDefault(const char* d)
{
    input.setText(d);
}

bool NetHackQtStringRequestor::Get(char* buffer, int maxchar)
{
    input.setMaxLength(maxchar);
    if (strlen(prompt.text()) > 16) {
	resize(fontMetrics().width(prompt.text())+50,fontMetrics().height()*6);
    } else {
	resize(fontMetrics().width(prompt.text())*2+50,fontMetrics().height()*4);
    }

    centerOnMain(this);
    show();
    input.setFocus();
    setResult(-1);
    while (result()==-1) {
	// Put keys in buffer (eg. from macros, from out-of-focus input)
	if (!keysource.Empty()) {
	    while (!keysource.Empty()) {
		int key=keysource.TopKey();
		int ascii=keysource.TopAscii();
		int state=keysource.GetState();
		if (ascii=='\r' || ascii=='\n') {
		    // CR or LF in buffer causes confirmation
		    strcpy(buffer,input.text());
		    return TRUE;
		} else if (ascii=='\033') {
		    return FALSE;
		} else {
		    input.fakeEvent(key,ascii,state);
		}
	    }
	}
	qApp->enter_loop();
    }
    // XXX Get rid of extra keys, since we couldn't get focus!
    while (!keysource.Empty()) keysource.GetKey();

    if (result()) {
	strcpy(buffer,input.text());
	return TRUE;
    } else {
	return FALSE;
    }
}
void NetHackQtStringRequestor::done(int i)
{
    setResult(i);
    qApp->exit_loop();
}


NetHackQtWindow::NetHackQtWindow()
{
}

NetHackQtWindow::~NetHackQtWindow()
{
}

// XXX Use "expected ..." for now, abort or default later.
//
void NetHackQtWindow::Clear() { puts("unexpected Clear"); }
void NetHackQtWindow::Display(bool block) { puts("unexpected Display"); }
bool NetHackQtWindow::Destroy() { return TRUE; }
void NetHackQtWindow::CursorTo(int x,int y) { puts("unexpected CursorTo"); }
void NetHackQtWindow::PutStr(int attr, const char* text) { puts("unexpected PutStr"); }
void NetHackQtWindow::StartMenu() { puts("unexpected StartMenu"); }
void NetHackQtWindow::AddMenu(int glyph, const ANY_P* identifier, char ch, char gch, int attr,
    const char* str, unsigned itemflags) { puts("unexpected AddMenu"); }
void NetHackQtWindow::EndMenu(const char* prompt) { puts("unexpected EndMenu"); }
int NetHackQtWindow::SelectMenu(int how, MENU_ITEM_P **menu_list) { puts("unexpected SelectMenu"); return 0; }
void NetHackQtWindow::ClipAround(int x,int y) { puts("unexpected ClipAround"); }
void NetHackQtWindow::PrintGlyph(int x,int y,int glyph) { puts("unexpected PrintGlyph"); }
//void NetHackQtWindow::PrintGlyphCompose(int x,int y,int,int) { puts("unexpected PrintGlyphCompose"); }
void NetHackQtWindow::UseRIP(int how, time_t when) { puts("unexpected UseRIP"); }



// XXX Hmmm... crash after saving bones file if Map window is
// XXX deleted.  Strange bug somewhere.
bool NetHackQtMapWindow::Destroy() { return FALSE; }

NetHackQtMapWindow::NetHackQtMapWindow(NetHackQtClickBuffer& click_sink) :
    clicksink(click_sink),
    change(10),
    rogue_font(0)
{
    viewport.addChild(this);

    setBackgroundColor(black);
    viewport.setBackgroundColor(black);

    pet_annotation = QPixmap(qt_compact_mode ? pet_mark_small_xpm : pet_mark_xpm);

    cursor.setX(0);
    cursor.setY(0);
    Clear();

    connect(qt_settings,SIGNAL(tilesChanged()),this,SLOT(updateTiles()));
    connect(&viewport, SIGNAL(contentsMoving(int,int)), this,
		SLOT(moveMessages(int,int)));

    updateTiles();
    //setFocusPolicy(StrongFocus);
#ifdef SAFERHANGUP
    QTimer* deadman = new QTimer(this);
    connect(deadman, SIGNAL(timeout()), SLOT(timeout()));
    deadman->start(2000);		// deadman timer every 2 seconds
#endif
}

#ifdef SAFERHANGUP
// The "deadman" timer is received by this slot
void NetHackQtMapWindow::timeout() {}
#endif

void NetHackQtMapWindow::moveMessages(int x, int y)
{
    QRect u = messages_rect;
    messages_rect.moveTopLeft(QPoint(x,y));
    u |= messages_rect;
    update(u);
}

void NetHackQtMapWindow::clearMessages()
{
    messages = "";
    update(messages_rect);
    messages_rect = QRect();
}

void NetHackQtMapWindow::putMessage(int attr, const char* text)
{
    if ( !messages.isEmpty() )
	messages += "\n";
    messages += text;
    QFontMetrics fm = fontMetrics();
    messages_rect = fm.boundingRect(viewport.contentsX(), viewport.contentsY(),
                                    viewport.width(), 0,
                                    WordBreak|AlignTop|AlignLeft|DontClip,
                                    messages);
    update(messages_rect);
}

void NetHackQtMapWindow::updateTiles()
{
    NetHackQtGlyphs& glyphs = qt_settings->glyphs();
    int gw = glyphs.width();
    int gh = glyphs.height();
    // Be exactly the size we want to be - full map...
    resize(COLNO*gw,ROWNO*gh);

    viewport.verticalScrollBar()->setSteps(gh,gh);
    viewport.horizontalScrollBar()->setSteps(gw,gw);
    /*
    viewport.setMaximumSize(
	gw*COLNO + viewport.verticalScrollBar()->width(),
	gh*ROWNO + viewport.horizontalScrollBar()->height()
    );
    */
    viewport.updateScrollBars();

    change.clear();
    change.add(0,0,COLNO,ROWNO);
    delete rogue_font; rogue_font = 0;
    Display(FALSE);

    emit resized();
}

NetHackQtMapWindow::~NetHackQtMapWindow()
{
    // Remove from viewport porthole, since that is a destructible member.
    viewport.removeChild(this);
    recreate(0,0,QPoint(0,0));
}

QWidget* NetHackQtMapWindow::Widget()
{
    return &viewport;
}

void NetHackQtMapWindow::Scroll(int dx, int dy)
{
    if (viewport.horizontalScrollBar()->isVisible()) {
	while (dx<0) { viewport.horizontalScrollBar()->subtractPage(); dx++; }
	while (dx>0) { viewport.horizontalScrollBar()->addPage(); dx--; }
    }
    if (viewport.verticalScrollBar()->isVisible()) {
	while (dy<0) { viewport.verticalScrollBar()->subtractPage(); dy++; }
	while (dy>0) { viewport.verticalScrollBar()->addPage(); dy--; }
    }
}

void NetHackQtMapWindow::Clear()
{
    unsigned short stone=cmap_to_glyph(S_stone);

    for (int j=0; j<ROWNO; j++) {
	for (int i=0; i<COLNO; i++) {
	    Glyph(i,j)=stone;
	}
    }

    change.clear();
    change.add(0,0,COLNO,ROWNO);
}

void NetHackQtMapWindow::clickCursor()
{
    clicksink.Put(cursor.x(),cursor.y(),CLICK_1);
    qApp->exit_loop();
}

void NetHackQtMapWindow::mousePressEvent(QMouseEvent* event)
{
    clicksink.Put(
	event->pos().x()/qt_settings->glyphs().width(),
	event->pos().y()/qt_settings->glyphs().height(),
	event->button()==LeftButton ? CLICK_1 : CLICK_2
    );
    qApp->exit_loop();
}

#ifdef TEXTCOLOR
static
const QPen& nhcolor_to_pen(int c)
{
    static QPen* pen=0;
    if ( !pen ) {
	pen = new QPen[17];
	pen[0] = QColor(24,24,24); // "black" on black
	pen[1] = Qt::red;
	pen[2] = QColor(0,191,0);
	pen[3] = QColor(127,127,0);
	pen[4] = Qt::blue;
	pen[5] = Qt::magenta;
	pen[6] = Qt::cyan;
	pen[7] = Qt::gray;
	pen[8] = Qt::white; // no color
	pen[9] = QColor(255,127,0);
	pen[10] = QColor(127,255,127);
	pen[11] = Qt::yellow;
	pen[12] = QColor(127,127,255);
	pen[13] = QColor(255,127,255);
	pen[14] = QColor(127,255,255);
	pen[15] = Qt::white;
	pen[16] = QColor(24,24,24); // "black" on black
    }

    return pen[c];
}
#endif

void NetHackQtMapWindow::paintEvent(QPaintEvent* event)
{
    QRect area=event->rect();
    QRect garea;
    garea.setCoords(
	QMAX(0,area.left()/qt_settings->glyphs().width()),
	QMAX(0,area.top()/qt_settings->glyphs().height()),
	QMIN(COLNO-1,area.right()/qt_settings->glyphs().width()),
	QMIN(ROWNO-1,area.bottom()/qt_settings->glyphs().height())
    );

    QPainter painter;

    painter.begin(this);

    if (Is_rogue_level(&u.uz) || iflags.wc_ascii_map)
    {
	// You enter a VERY primitive world!

	painter.setClipRect( event->rect() ); // (normally we don't clip)
	painter.fillRect( event->rect(), black );

	if ( !rogue_font ) {
	    // Find font...
	    int pts = 5;
	    QString fontfamily = iflags.wc_font_map
		? iflags.wc_font_map : "Courier";
	    bool bold = FALSE;
	    if ( fontfamily.right(5).lower() == "-bold" ) {
		fontfamily.truncate(fontfamily.length()-5);
		bold = TRUE;
	    }
	    while ( pts < 32 ) {
		QFont f(fontfamily, pts, bold ? QFont::Bold : QFont::Normal);
		painter.setFont(QFont(fontfamily, pts));
		QFontMetrics fm = painter.fontMetrics();
		if ( fm.width("M") > qt_settings->glyphs().width() )
		    break;
		if ( fm.height() > qt_settings->glyphs().height() )
		    break;
		pts++;
	    }
	    rogue_font = new QFont(fontfamily,pts-1);
	}
	painter.setFont(*rogue_font);

	for (int j=garea.top(); j<=garea.bottom(); j++) {
	    for (int i=garea.left(); i<=garea.right(); i++) {
		unsigned short g=Glyph(i,j);
		uchar ch;
		int color, och;
		unsigned special;

		painter.setPen( green );
		/* map glyph to character and color */
    		(void)mapglyph(g, &och, &color, &special, i, j, 0);
		ch = (uchar)och;
#ifdef TEXTCOLOR
		painter.setPen( nhcolor_to_pen(color) );
#endif
		painter.drawText(
		    i*qt_settings->glyphs().width(),
		    j*qt_settings->glyphs().height(),
		    qt_settings->glyphs().width(),
		    qt_settings->glyphs().height(),
		    AlignCenter,
		    (const char*)&ch, 1
		);
		if (glyph_is_pet(g)
#ifdef TEXTCOLOR
		    && ::iflags.hilite_pet
#endif
		) {
		    painter.drawPixmap(QPoint(i*qt_settings->glyphs().width(),
                                              j*qt_settings->glyphs().height()),
                                       pet_annotation);
		}
	    }
	}

	painter.setFont(font());
    } else {
	for (int j=garea.top(); j<=garea.bottom(); j++) {
	    for (int i=garea.left(); i<=garea.right(); i++) {
		unsigned short g=Glyph(i,j);
		qt_settings->glyphs().drawCell(painter, g, i, j);
		if (glyph_is_pet(g)
#ifdef TEXTCOLOR
		    && ::iflags.hilite_pet
#endif
		) {
		    painter.drawPixmap(QPoint(i*qt_settings->glyphs().width(),
                                              j*qt_settings->glyphs().height()),
                                       pet_annotation);
		}
	    }
	}
    }

    if (garea.contains(cursor)) {
	if (Is_rogue_level(&u.uz)) {
#ifdef TEXTCOLOR
	    painter.setPen( white );
#else
	    painter.setPen( green ); // REALLY primitive
#endif
	} else
	{
	    int hp100;
	    if (u.mtimedone) {
		hp100=u.mhmax ? u.mh*100/u.mhmax : 100;
	    } else {
		hp100=u.uhpmax ? u.uhp*100/u.uhpmax : 100;
	    }

	    if (hp100 > 75) painter.setPen(white);
	    else if (hp100 > 50) painter.setPen(yellow);
	    else if (hp100 > 25) painter.setPen(QColor(0xff,0xbf,0x00)); // orange
	    else if (hp100 > 10) painter.setPen(red);
	    else painter.setPen(magenta);
	}

	painter.drawRect(
	    cursor.x()*qt_settings->glyphs().width(),cursor.y()*qt_settings->glyphs().height(),
	    qt_settings->glyphs().width(),qt_settings->glyphs().height());
    }

    if (area.intersects(messages_rect)) {
	painter.setPen(black);
	painter.drawText(viewport.contentsX()+1,viewport.contentsY()+1,
	    viewport.width(),0, WordBreak|AlignTop|AlignLeft|DontClip, messages);
	painter.setPen(white);
	painter.drawText(viewport.contentsX(),viewport.contentsY(),
	    viewport.width(),0, WordBreak|AlignTop|AlignLeft|DontClip, messages);
    }

    painter.end();
}

void NetHackQtMapWindow::Display(bool block)
{
    for (int i=0; i<change.clusters(); i++) {
	const QRect& ch=change[i];
	repaint(
	    ch.x()*qt_settings->glyphs().width(),
	    ch.y()*qt_settings->glyphs().height(),
	    ch.width()*qt_settings->glyphs().width(),
	    ch.height()*qt_settings->glyphs().height(),
	    FALSE
	);
    }

    change.clear();

    if (block) {
	yn_function("Press a key when done viewing",0,'\0');
    }
}

void NetHackQtMapWindow::CursorTo(int x,int y)
{
    Changed(cursor.x(),cursor.y());
    cursor.setX(x);
    cursor.setY(y);
    Changed(cursor.x(),cursor.y());
}

void NetHackQtMapWindow::PutStr(int attr, const char* text)
{
    puts("unexpected PutStr in MapWindow");
}

void NetHackQtMapWindow::ClipAround(int x,int y)
{
    // Convert to pixel of center of tile
    x=x*qt_settings->glyphs().width()+qt_settings->glyphs().width()/2;
    y=y*qt_settings->glyphs().height()+qt_settings->glyphs().height()/2;

    // Then ensure that pixel is visible
    viewport.center(x,y,0.45,0.45);
}

void NetHackQtMapWindow::PrintGlyph(int x,int y,int glyph)
{
    Glyph(x,y)=glyph;
    Changed(x,y);
}

//void NetHackQtMapWindow::PrintGlyphCompose(int x,int y,int glyph1, int glyph2)
//{
    // TODO: composed graphics
//}

void NetHackQtMapWindow::Changed(int x, int y)
{
    change.add(x,y);
}


class NetHackQtScrollText : public QTableView {
    struct UData {
	UData() : text(0), attr(0) { }
	~UData() { if (text) free(text); }

	char* text;
	int attr;
    };
public:
    int uncleared;

    NetHackQtScrollText(int maxlength) :
	uncleared(0),
	maxitems(maxlength),
	first(0),
	count(0),
	item_cycle(maxlength)
    {
	setNumCols(1);
	setCellWidth(200);
	setCellHeight(fontMetrics().height());
	setBackgroundColor(white);
	setTableFlags(Tbl_vScrollBar
	    |Tbl_autoHScrollBar
	    |Tbl_clipCellPainting
	    |Tbl_smoothScrolling);
    }

    ~NetHackQtScrollText()
    {
    }

    void Scroll(int dx, int dy)
    {
	setXOffset(xOffset()+dx*viewWidth());
	setYOffset(yOffset()+dy*viewHeight());
    }

    void insertItem(int attr, const char* text)
    {
	setTopCell(count);

	setAutoUpdate(FALSE);

	int i;
	if (count<maxitems) {
	    i=count++;
	    setNumRows(count);
	} else {
	    i=count-1;
	    first=(first+1)%maxitems;
	}
	item(i).attr=attr;
	item(i).text=strdup(text);
	int w=datumWidth(item(i));

	if (w > cellWidth()) {
	    // Get wider.
	    setCellWidth(w);
	}
	setTopCell(count);

	setAutoUpdate(TRUE);

	if (viewHeight() >= totalHeight()-cellHeight()) {
	    repaint();
	} else {
	    scroll(0,cellHeight());
	}
    }

    virtual void setFont(const QFont& font)
    {
	QTableView::setFont(font);
	setCellHeight(fontMetrics().height());
    }

protected:

    UData& item(int i)
    {
	return item_cycle[(first+i)%maxitems];
    }

    const int maxitems;
    int first, count;
    QArray<UData> item_cycle;

    int datumWidth(const UData& uitem)
    {
	if (uitem.text) {
	    int width=fontMetrics().width(uitem.text)+3;
	    if (uitem.attr) {
		// XXX Too expensive to do properly, because
		// XXX we have to set the font of the widget
		// XXX just to get the font metrics information!
		// XXX Could hold a fake widget for that
		// XXX purpose, but this hack is less ugly.
		width+=width/10;
	    }
	    return width;
	} else {
	    return 0;
	}
    }

    virtual void setupPainter(QPainter *p)
    {
	// XXX This shouldn't be needed - we set the bg in the constructor.
	p->setBackgroundColor(white);
    }

    virtual void paintCell(QPainter *p, int row, int col)
    {
	bool sel=FALSE;
	UData& uitem=item(row);

	if (!sel && row < count-uncleared) {
	    p->setPen(darkGray);
	} else {
	    p->setPen(black);
	}

	if (uitem.attr) {
	    // XXX only bold
	    QFont bold(font().family(),font().pointSize(),QFont::Bold);
	    p->setFont(bold);
	}

	p->drawText(3, 0, cellWidth(), cellHeight(),
		AlignLeft|AlignVCenter, uitem.text);

	if (uitem.attr) {
	    p->setFont(font());
	}
    }
};

NetHackQtMessageWindow::NetHackQtMessageWindow() :
    list(new NetHackQtScrollText(::iflags.msg_history))
{
    ::iflags.window_inited = 1;
    map = 0;
    connect(qt_settings,SIGNAL(fontChanged()),this,SLOT(updateFont()));
    updateFont();
}

NetHackQtMessageWindow::~NetHackQtMessageWindow()
{
    ::iflags.window_inited = 0;
    delete list;
}

QWidget* NetHackQtMessageWindow::Widget() { return list; }

void NetHackQtMessageWindow::setMap(NetHackQtMapWindow* m)
{
    map = m;
    updateFont();
}

void NetHackQtMessageWindow::updateFont()
{
    list->setFont(qt_settings->normalFont());
    if ( map )
	map->setFont(qt_settings->normalFont());
}

void NetHackQtMessageWindow::Scroll(int dx, int dy)
{
    list->Scroll(dx,dy);
}

void NetHackQtMessageWindow::Clear()
{
    if ( map )
	map->clearMessages();
    if (list->uncleared) {
	list->uncleared=0;
	changed=TRUE;
	Display(FALSE);
    }
}

void NetHackQtMessageWindow::Display(bool block)
{
    if (changed) {
	list->repaint();
	changed=FALSE;
    }
}

void NetHackQtMessageWindow::PutStr(int attr, const char* text)
{
#ifdef USER_SOUNDS
    play_sound_for_message(text);
#endif

    changed=TRUE;
    list->uncleared++;
    list->insertItem(attr,text);

    // Force scrollbar to bottom
    // XXX list->setTopItem(list->count());

    if ( map )
	map->putMessage(attr, text);
}



NetHackQtLabelledIcon::NetHackQtLabelledIcon(QWidget* parent, const char* l) :
    QWidget(parent),
    low_is_good(FALSE),
    prev_value(-123),
    turn_count(-1),
    label(new QLabel(l,this)),
    icon(0)
{
    initHighlight();
}
NetHackQtLabelledIcon::NetHackQtLabelledIcon(QWidget* parent, const char* l, const QPixmap& i) :
    QWidget(parent),
    low_is_good(FALSE),
    prev_value(-123),
    turn_count(-1),
    label(new QLabel(l,this)),
    icon(new QLabel(this))
{
    setIcon(i);
    initHighlight();
}
void NetHackQtLabelledIcon::initHighlight()
{
    const QPalette& pal=palette();
    const QColorGroup& pa=pal.normal();
    //QColorGroup good(white,darkGreen,pa.light(),pa.dark(),pa.mid(),white,pa.base());
    QColorGroup good(black,green,pa.light(),pa.dark(),pa.mid(),black,pa.base());
    QColorGroup bad(white,red,pa.light(),pa.dark(),pa.mid(),white,pa.base());
    hl_good=pal.copy();
    hl_good.setNormal(good);
    hl_good.setActive(good);
    hl_bad=pal.copy();
    hl_bad.setNormal(bad);
    hl_bad.setActive(bad);
}

void NetHackQtLabelledIcon::setLabel(const char* t, bool lower)
{
    if (!label) {
	label=new QLabel(this);
	label->setFont(font());
	resizeEvent(0);
    }
    if (0!=strcmp(label->text(),t)) {
	label->setText(t);
	highlight(lower==low_is_good ? hl_good : hl_bad);
    }
}
void NetHackQtLabelledIcon::setLabel(const char* t, long v, long cv, const char* tail)
{
    char buf[BUFSZ];
    if (v==NoNum) {
	Sprintf(buf,"%s%s",t,tail);
    } else {
	Sprintf(buf,"%s%ld%s",t,v,tail);
    }
    setLabel(buf,cv<prev_value);
    prev_value=cv;
}
void NetHackQtLabelledIcon::setLabel(const char* t, long v, const char* tail)
{
    setLabel(t,v,v,tail);
}
void NetHackQtLabelledIcon::setIcon(const QPixmap& i)
{
    if (icon) icon->setPixmap(i);
    else { icon=new QLabel(this); icon->setPixmap(i); resizeEvent(0); }
    icon->resize(i.width(),i.height());
}
void NetHackQtLabelledIcon::setFont(const QFont& f)
{
    QWidget::setFont(f);
    if (label) label->setFont(f);
}
void NetHackQtLabelledIcon::show()
{
#if QT_VERSION >= 300
    if (isHidden())
#else
    if (!isVisible())
#endif
	highlight(hl_bad);
    QWidget::show();
}
void NetHackQtLabelledIcon::highlightWhenChanging()
{
    turn_count=0;
}
void NetHackQtLabelledIcon::lowIsGood()
{
    low_is_good=TRUE;
}
void NetHackQtLabelledIcon::dissipateHighlight()
{
    if (turn_count>0) {
	turn_count--;
	if (!turn_count)
	    unhighlight();
    }
}
void NetHackQtLabelledIcon::highlight(const QPalette& hl)
{
    if (label) { // Surely it is?!
	if (turn_count>=0) {
	    label->setPalette(hl);
	    turn_count=4;
	    // `4' includes this turn, so dissipates after
	    // 3 more keypresses.
	} else {
	    label->setPalette(palette());
	}
    }
}
void NetHackQtLabelledIcon::unhighlight()
{
    if (label) { // Surely it is?!
	label->setPalette(palette());
    }
}
void NetHackQtLabelledIcon::resizeEvent(QResizeEvent*)
{
    setAlignments();

    //int labw=label ? label->fontMetrics().width(label->text()) : 0;
    int labh=label ? label->fontMetrics().height() : 0;
    int icoh=icon ? icon->height() : 0;
    int h=icoh+labh;
    int icoy=(h>height() ? height()-labh-icoh : height()/2-h/2);
    int laby=icoy+icoh;
    if (icon) {
	icon->setGeometry(0,icoy,width(),icoh);
    }
    if (label) {
	label->setGeometry(0,laby,width(),labh);
    }
}

void NetHackQtLabelledIcon::setAlignments()
{
    if (label) label->setAlignment(AlignHCenter|AlignVCenter);
    if (icon) icon->setAlignment(AlignHCenter|AlignVCenter);
}

static void
tryload(QPixmap& pm, const char* fn)
{
    if (!pm.load(fn)) {
	QString msg;
	msg.sprintf("Cannot load \"%s\"", fn);
	QMessageBox::warning(0, "IO Error", msg);
    }
}

NetHackQtStatusWindow::NetHackQtStatusWindow() :
    // Notes:
    //  Alignment needs -2 init value, because -1 is an alignment.
    //  Armor Class is an schar, so 256 is out of range.
    //  Blank value is 0 and should never change.
    name(this,"(name)"),
    dlevel(this,"(dlevel)"),
    str(this,"STR"),
    dex(this,"DEX"),
    con(this,"CON"),
    intel(this,"INT"),
    wis(this,"WIS"),
    cha(this,"CHA"),
    gold(this,"Gold"),
    hp(this,"Hit Points"),
    power(this,"Power"),
    ac(this,"Armour Class"),
    level(this,"Level"),
    exp(this,"Experience"),
    align(this,"Alignment"),
    time(this,"Time"),
    score(this,"Score"),
    hunger(this,""),
    confused(this,"Confused"),
    sick_fp(this,"Sick"),
    sick_il(this,"Ill"),
    blind(this,"Blind"),
    stunned(this,"Stunned"),
    hallu(this,"Hallu"),
    encumber(this,""),
    hline1(this),
    hline2(this),
    hline3(this),
    first_set(TRUE)
{
    p_str = QPixmap(str_xpm);
    p_str = QPixmap(str_xpm);
    p_dex = QPixmap(dex_xpm);
    p_con = QPixmap(cns_xpm);
    p_int = QPixmap(int_xpm);
    p_wis = QPixmap(wis_xpm);
    p_cha = QPixmap(cha_xpm);

    p_chaotic = QPixmap(chaotic_xpm);
    p_neutral = QPixmap(neutral_xpm);
    p_lawful = QPixmap(lawful_xpm);

    p_satiated = QPixmap(satiated_xpm);
    p_hungry = QPixmap(hungry_xpm);

    p_confused = QPixmap(confused_xpm);
    p_sick_fp = QPixmap(sick_fp_xpm);
    p_sick_il = QPixmap(sick_il_xpm);
    p_blind = QPixmap(blind_xpm);
    p_stunned = QPixmap(stunned_xpm);
    p_hallu = QPixmap(hallu_xpm);

    p_encumber[0] = QPixmap(slt_enc_xpm);
    p_encumber[1] = QPixmap(mod_enc_xpm);
    p_encumber[2] = QPixmap(hvy_enc_xpm);
    p_encumber[3] = QPixmap(ext_enc_xpm);
    p_encumber[4] = QPixmap(ovr_enc_xpm);

    str.setIcon(p_str);
    dex.setIcon(p_dex);
    con.setIcon(p_con);
    intel.setIcon(p_int);
    wis.setIcon(p_wis);
    cha.setIcon(p_cha);

    align.setIcon(p_neutral);
    hunger.setIcon(p_hungry);

    confused.setIcon(p_confused);
    sick_fp.setIcon(p_sick_fp);
    sick_il.setIcon(p_sick_il);
    blind.setIcon(p_blind);
    stunned.setIcon(p_stunned);
    hallu.setIcon(p_hallu);

    encumber.setIcon(p_encumber[0]);

    hline1.setFrameStyle(QFrame::HLine|QFrame::Sunken);
    hline2.setFrameStyle(QFrame::HLine|QFrame::Sunken);
    hline3.setFrameStyle(QFrame::HLine|QFrame::Sunken);
    hline1.setLineWidth(1);
    hline2.setLineWidth(1);
    hline3.setLineWidth(1);

    connect(qt_settings,SIGNAL(fontChanged()),this,SLOT(doUpdate()));
    doUpdate();
}

void NetHackQtStatusWindow::doUpdate()
{
    const QFont& large=qt_settings->largeFont();
    name.setFont(large);
    dlevel.setFont(large);

    const QFont& normal=qt_settings->normalFont();
    str.setFont(normal);
    dex.setFont(normal);
    con.setFont(normal);
    intel.setFont(normal);
    wis.setFont(normal);
    cha.setFont(normal);
    gold.setFont(normal);
    hp.setFont(normal);
    power.setFont(normal);
    ac.setFont(normal);
    level.setFont(normal);
    exp.setFont(normal);
    align.setFont(normal);
    time.setFont(normal);
    score.setFont(normal);
    hunger.setFont(normal);
    confused.setFont(normal);
    sick_fp.setFont(normal);
    sick_il.setFont(normal);
    blind.setFont(normal);
    stunned.setFont(normal);
    hallu.setFont(normal);
    encumber.setFont(normal);

    updateStats();
}

QWidget* NetHackQtStatusWindow::Widget() { return this; }

void NetHackQtStatusWindow::Clear()
{
}
void NetHackQtStatusWindow::Display(bool block)
{
}
void NetHackQtStatusWindow::CursorTo(int,int y)
{
    cursy=y;
}
void NetHackQtStatusWindow::PutStr(int attr, const char* text)
{
    // do a complete update when line 0 is done (as per X11 fancy status)
    if (cursy==0) updateStats();
}

void NetHackQtStatusWindow::resizeEvent(QResizeEvent*)
{
    const float SP_name=0.13; //     <Name> the <Class> (large)
    const float SP_dlev=0.13; //   Level 3 in The Dungeons of Doom (large)
    const float SP_atr1=0.25; //  STR   DEX   CON   INT   WIS   CHA
    const float SP_hln1=0.02; // ---
    const float SP_atr2=0.09; //  Au    HP    PW    AC    LVL   EXP
    const float SP_hln2=0.02; // ---
    const float SP_time=0.09; //      time    score
    const float SP_hln3=0.02; // ---
    const float SP_stat=0.25; // Alignment, Poisoned, Hungry, Sick, etc.

    int h=height();
    int x=0,y=0;

    int iw; // Width of an item across line
    int lh; // Height of a line of values

    lh=int(h*SP_name);
    name.setGeometry(0,0,width(),lh); y+=lh;
    lh=int(h*SP_dlev);
    dlevel.setGeometry(0,y,width(),lh); y+=lh;

    lh=int(h*SP_hln1);
    hline1.setGeometry(0,y,width(),lh); y+=lh;

    lh=int(h*SP_atr1);
    iw=width()/6;
    str.setGeometry(x,y,iw,lh); x+=iw;
    dex.setGeometry(x,y,iw,lh); x+=iw;
    con.setGeometry(x,y,iw,lh); x+=iw;
    intel.setGeometry(x,y,iw,lh); x+=iw;
    wis.setGeometry(x,y,iw,lh); x+=iw;
    cha.setGeometry(x,y,iw,lh); x+=iw;
    x=0; y+=lh;

    lh=int(h*SP_hln2);
    hline2.setGeometry(0,y,width(),lh); y+=lh;

    lh=int(h*SP_atr2);
    iw=width()/6;
    gold.setGeometry(x,y,iw,lh); x+=iw;
    hp.setGeometry(x,y,iw,lh); x+=iw;
    power.setGeometry(x,y,iw,lh); x+=iw;
    ac.setGeometry(x,y,iw,lh); x+=iw;
    level.setGeometry(x,y,iw,lh); x+=iw;
    exp.setGeometry(x,y,iw,lh); x+=iw;
    x=0; y+=lh;

    lh=int(h*SP_hln3);
    hline3.setGeometry(0,y,width(),lh); y+=lh;

    lh=int(h*SP_time);
    iw=width()/3; x+=iw/2;
    time.setGeometry(x,y,iw,lh); x+=iw;
    score.setGeometry(x,y,iw,lh); x+=iw;
    x=0; y+=lh;

    lh=int(h*SP_stat);
    iw=width()/9;
    align.setGeometry(x,y,iw,lh); x+=iw;
    hunger.setGeometry(x,y,iw,lh); x+=iw;
    confused.setGeometry(x,y,iw,lh); x+=iw;
    sick_fp.setGeometry(x,y,iw,lh); x+=iw;
    sick_il.setGeometry(x,y,iw,lh); x+=iw;
    blind.setGeometry(x,y,iw,lh); x+=iw;
    stunned.setGeometry(x,y,iw,lh); x+=iw;
    hallu.setGeometry(x,y,iw,lh); x+=iw;
    encumber.setGeometry(x,y,iw,lh); x+=iw;
    x=0; y+=lh;
}


/*
 * Set all widget values to a null string.  This is used after all spacings
 * have been calculated so that when the window is popped up we don't get all
 * kinds of funny values being displayed.
 */
void NetHackQtStatusWindow::nullOut()
{
}

void NetHackQtStatusWindow::fadeHighlighting()
{
    name.dissipateHighlight();
    dlevel.dissipateHighlight();

    str.dissipateHighlight();
    dex.dissipateHighlight();
    con.dissipateHighlight();
    intel.dissipateHighlight();
    wis.dissipateHighlight();
    cha.dissipateHighlight();

    gold.dissipateHighlight();
    hp.dissipateHighlight();
    power.dissipateHighlight();
    ac.dissipateHighlight();
    level.dissipateHighlight();
    exp.dissipateHighlight();
    align.dissipateHighlight();

    time.dissipateHighlight();
    score.dissipateHighlight();

    hunger.dissipateHighlight();
    confused.dissipateHighlight();
    sick_fp.dissipateHighlight();
    sick_il.dissipateHighlight();
    blind.dissipateHighlight();
    stunned.dissipateHighlight();
    hallu.dissipateHighlight();
    encumber.dissipateHighlight();
}

/*
 * Update the displayed status.  The current code in botl.c updates
 * two lines of information.  Both lines are always updated one after
 * the other.  So only do our update when we update the second line.
 *
 * Information on the first line:
 *    name, attributes, alignment, score
 *
 * Information on the second line:
 *    dlvl, gold, hp, power, ac, {level & exp or HD **}
 *    status (hunger, conf, halu, stun, sick, blind), time, encumbrance
 *
 * [**] HD is shown instead of level and exp if mtimedone is non-zero.
 */
void NetHackQtStatusWindow::updateStats()
{
    if (!parentWidget()) return;

    char buf[BUFSZ];

    if (cursy != 0) return;    /* do a complete update when line 0 is done */

    if (ACURR(A_STR) > 118) {
	Sprintf(buf,"STR:%d",ACURR(A_STR)-100);
    } else if (ACURR(A_STR)==118) {
	Sprintf(buf,"STR:18/**");
    } else if(ACURR(A_STR) > 18) {
	Sprintf(buf,"STR:18/%02d",ACURR(A_STR)-18);
    } else {
	Sprintf(buf,"STR:%d",ACURR(A_STR));
    }
    str.setLabel(buf,NetHackQtLabelledIcon::NoNum,ACURR(A_STR));

    dex.setLabel("DEX:",(long)ACURR(A_DEX));
    con.setLabel("CON:",(long)ACURR(A_CON));
    intel.setLabel("INT:",(long)ACURR(A_INT));
    wis.setLabel("WIS:",(long)ACURR(A_WIS));
    cha.setLabel("CHA:",(long)ACURR(A_CHA));
    const char* hung=hu_stat[u.uhs];
    if (hung[0]==' ') {
	hunger.hide();
    } else {
	hunger.setIcon(u.uhs ? p_hungry : p_satiated);
	hunger.setLabel(hung);
	hunger.show();
    }
    if (Confusion) confused.show(); else confused.hide();
    if (Sick) {
	if (u.usick_type & SICK_VOMITABLE) {
	    sick_fp.show();
	} else {
	    sick_fp.hide();
	}
	if (u.usick_type & SICK_NONVOMITABLE) {
	    sick_il.show();
	} else {
	    sick_il.hide();
	}
    } else {
	sick_fp.hide();
	sick_il.hide();
    }
    if (Blind) blind.show(); else blind.hide();
    if (Stunned) stunned.show(); else stunned.hide();
    if (Hallucination) hallu.show(); else hallu.hide();
    const char* enc=enc_stat[near_capacity()];
    if (enc[0]==' ' || !enc[0]) {
	encumber.hide();
    } else {
	encumber.setIcon(p_encumber[near_capacity()-1]);
	encumber.setLabel(enc);
	encumber.show();
    }
    Strcpy(buf, g.plname);
    if ('a' <= buf[0] && buf[0] <= 'z') buf[0] += 'A'-'a';
    Strcat(buf, " the ");
    if (u.mtimedone) {
	char mname[BUFSZ];
	int k = 0;

	Strcpy(mname, mons[u.umonnum].mname);
	while(mname[k] != 0) {
	    if ((k == 0 || (k > 0 && mname[k-1] == ' '))
	     && 'a' <= mname[k] && mname[k] <= 'z')
	    {
		mname[k] += 'A' - 'a';
	    }
	    k++;
	}
	Strcat(buf, mname);
    } else {
	Strcat(buf, rank_of(u.ulevel, pl_character[0], ::flags.female));
    }
    name.setLabel(buf,NetHackQtLabelledIcon::NoNum,u.ulevel);

    if (describe_level(buf)) {
	dlevel.setLabel(buf,(bool)TRUE);
    } else {
	Sprintf(buf, "%s, level ", dungeons[u.uz.dnum].dname);
	dlevel.setLabel(buf,(long)depth(&u.uz));
    }

    gold.setLabel("Au:", money_cnt(g.invent));
    if (u.mtimedone) {
	// You're a monster!

	Sprintf(buf, "/%d", u.mhmax);
	hp.setLabel("HP:",u.mh  > 0 ? u.mh  : 0,buf);
	level.setLabel("HD:",(long)mons[u.umonnum].mlevel);
    } else {
	// You're normal.

	Sprintf(buf, "/%d", u.uhpmax);
	hp.setLabel("HP:",u.uhp > 0 ? u.uhp : 0,buf);
	level.setLabel("Level:",(long)u.ulevel);
    }
    Sprintf(buf, "/%d", u.uenmax);
    power.setLabel("Pow:",u.uen,buf);
    ac.setLabel("AC:",(long)u.uac);
    if (::flags.showexp) {
	exp.setLabel("Exp:",(long)u.uexp);
    } else
    {
	exp.setLabel("");
    }
    if (u.ualign.type==A_CHAOTIC) {
	align.setIcon(p_chaotic);
	align.setLabel("Chaotic");
    } else if (u.ualign.type==A_NEUTRAL) {
	align.setIcon(p_neutral);
	align.setLabel("Neutral");
    } else {
	align.setIcon(p_lawful);
	align.setLabel("Lawful");
    }

    if (::flags.time) time.setLabel("Time:",(long)g.moves);
    else time.setLabel("");
#ifdef SCORE_ON_BOTL
    if (::flags.showscore) {
	score.setLabel("Score:",(long)botl_score());
    } else
#endif
    {
	score.setLabel("");
    }

    if (first_set)
    {
	first_set=FALSE;

	name.highlightWhenChanging();
	dlevel.highlightWhenChanging();

	str.highlightWhenChanging();
	dex.highlightWhenChanging();
	con.highlightWhenChanging();
	intel.highlightWhenChanging();
	wis.highlightWhenChanging();
	cha.highlightWhenChanging();

	gold.highlightWhenChanging();
	hp.highlightWhenChanging();
	power.highlightWhenChanging();
	ac.highlightWhenChanging(); ac.lowIsGood();
	level.highlightWhenChanging();
	exp.highlightWhenChanging();
	align.highlightWhenChanging();

	//time.highlightWhenChanging();
	score.highlightWhenChanging();

	hunger.highlightWhenChanging();
	confused.highlightWhenChanging();
	sick_fp.highlightWhenChanging();
	sick_il.highlightWhenChanging();
	blind.highlightWhenChanging();
	stunned.highlightWhenChanging();
	hallu.highlightWhenChanging();
	encumber.highlightWhenChanging();
    }
}

/*
 * Turn off hilighted status values after a certain amount of turns.
 */
void NetHackQtStatusWindow::checkTurnEvents()
{
}



NetHackQtMenuDialog::NetHackQtMenuDialog() :
    QDialog(qApp->mainWidget(),0,FALSE)
{
}

void NetHackQtMenuDialog::resizeEvent(QResizeEvent*)
{
    emit Resized();
}

void NetHackQtMenuDialog::Accept()
{
    accept();
}

void NetHackQtMenuDialog::Reject()
{
    reject();
}

void NetHackQtMenuDialog::SetResult(int r)
{
    setResult(r);
}

void NetHackQtMenuDialog::done(int i)
{
    setResult(i);
    qApp->exit_loop();
}

// Table view columns:
// 
// [pick-count] [accel] [glyph] [string]
// 
// Maybe accel should be near string.  We'll see.
// pick-count normally blank.
//   double-clicking or click-on-count gives pop-up entry
// string is green when selected
//
NetHackQtMenuWindow::NetHackQtMenuWindow(NetHackQtKeyBuffer& ks) :
    QTableView(),
    keysource(ks),
    dialog(new NetHackQtMenuDialog()),
    prompt(0),
    pressed(-1)
{
    setNumCols(4);
    setCellHeight(QMAX(qt_settings->glyphs().height()+1,fontMetrics().height()));
    setBackgroundColor(lightGray);
    setFrameStyle(Panel|Sunken);
    setLineWidth(2);

    ok=new QPushButton("Ok",dialog);
    connect(ok,SIGNAL(clicked()),dialog,SLOT(accept()));

    cancel=new QPushButton("Cancel",dialog);
    connect(cancel,SIGNAL(clicked()),dialog,SLOT(reject()));

    all=new QPushButton("All",dialog);
    connect(all,SIGNAL(clicked()),this,SLOT(All()));

    none=new QPushButton("None",dialog);
    connect(none,SIGNAL(clicked()),this,SLOT(ChooseNone()));

    invert=new QPushButton("Invert",dialog);
    connect(invert,SIGNAL(clicked()),this,SLOT(Invert()));

    search=new QPushButton("Search",dialog);
    connect(search,SIGNAL(clicked()),this,SLOT(Search()));

    QPoint pos(0,ok->height());
    recreate(dialog,0,pos);
    prompt.recreate(dialog,0,pos);

    setBackgroundColor(lightGray);

    connect(dialog,SIGNAL(Resized()),this,SLOT(Layout()));

    setTableFlags(Tbl_autoHScrollBar|Tbl_autoVScrollBar
	    |Tbl_smoothScrolling|Tbl_clipCellPainting);
    setFocusPolicy(StrongFocus);
}

NetHackQtMenuWindow::~NetHackQtMenuWindow()
{
    // Remove from dialog before we destruct it
    recreate(0,0,QPoint(0,0));
    delete dialog;
}

void NetHackQtMenuWindow::focusInEvent(QFocusEvent *)
{
    // Don't repaint at all, since nothing is using the focus colour
}
void NetHackQtMenuWindow::focusOutEvent(QFocusEvent *)
{
    // Don't repaint at all, since nothing is using the focus colour
}

int NetHackQtMenuWindow::cellWidth(int col)
{
    switch (col) {
     case 0:
	return fontMetrics().width("All ");
    break; case 1:
	return fontMetrics().width(" m ");
    break; case 2:
	return qt_settings->glyphs().width();
    break; case 3:
	return str_width;
    }
    impossible("Extra column (#%d) in MenuWindow",col);
    return 0;
}

QWidget* NetHackQtMenuWindow::Widget() { return dialog; }

void NetHackQtMenuWindow::StartMenu()
{
    setNumRows((itemcount=0));
    str_width=200;
    str_fixed=FALSE;
    next_accel=0;
    has_glyphs=FALSE;
}

NetHackQtMenuWindow::MenuItem::MenuItem() :
    str(0)
{
}

NetHackQtMenuWindow::MenuItem::~MenuItem()
{
    if (str) free((void*)str);
}

#define STR_MARGIN 4

void NetHackQtMenuWindow::AddMenu(int glyph, const ANY_P* identifier,
	char ch, char gch, int attr, const char* str, unsigned itemflags)
{
    bool presel = (itemflags & MENU_ITEMFLAGS_SELECTED) != 0;
    if (!ch && identifier->a_void!=0) {
	// Supply a keyboard accelerator.  Limited supply.
	static char accel[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	if (accel[next_accel]) {
	    ch=accel[next_accel++];
	}
    }

    if ((int)item.size() < itemcount+1) {
	item.resize(itemcount*4+10);
    }
    item[itemcount].glyph=glyph;
    item[itemcount].identifier=*identifier;
    item[itemcount].ch=ch;
    item[itemcount].attr=attr;
    item[itemcount].str=strdup(str);
    item[itemcount].selected=presel;
    item[itemcount].itemflags=itemflags;
    item[itemcount].count=-1;
    ++itemcount;

    str_fixed=str_fixed || strstr(str,"     ");
    if (glyph!=NO_GLYPH) has_glyphs=TRUE;
}
void NetHackQtMenuWindow::EndMenu(const char* p)
{
    prompt.setText(p ? p : "");
}
void NetHackQtMenuWindow::Layout()
{
    int butw=totalWidth()/6; // 6 buttons
    int buth=fontMetrics().height()+8; // 8 for spacing & mitres
    int prompth=(prompt.text().isNull() ? 0 : buth);

    prompt.setGeometry(6,buth,dialog->width()-6,prompth);
    int h=dialog->height()-buth-prompth;
    setGeometry(0,buth+prompth, dialog->width(), h);

    // Below, we take care to use up full width
    int x=0;
    ok->setGeometry(x,0,butw,buth); x+=butw; butw=(dialog->width()-x)/5;
    cancel->setGeometry(x,0,butw,buth); x+=butw; butw=(dialog->width()-x)/4;
    all->setGeometry(x,0,butw,buth); x+=butw; butw=(dialog->width()-x)/3;
    none->setGeometry(x,0,butw,buth); x+=butw; butw=(dialog->width()-x)/2;
    invert->setGeometry(x,0,butw,buth); x+=butw; butw=(dialog->width()-x)/1;
    search->setGeometry(x,0,butw,buth);
}

int NetHackQtMenuWindow::SelectMenu(int h, MENU_ITEM_P **menu_list)
{
    setFont(str_fixed ?
	qt_settings->normalFixedFont() : qt_settings->normalFont());

    for (int i=0; i<itemcount; i++) {
	str_width=QMAX(str_width,
	    STR_MARGIN+fontMetrics().width(item[i].str));
    }

    setCellHeight(QMAX(qt_settings->glyphs().height()+1,fontMetrics().height()));
    setNumRows(itemcount);

    int buth=fontMetrics().height()+8; // 8 for spacing & mitres

    how=h;

    ok->setEnabled(how!=PICK_ONE);ok->setDefault(how!=PICK_ONE);
    cancel->setEnabled(TRUE);
    all->setEnabled(how==PICK_ANY);
    none->setEnabled(how==PICK_ANY);
    invert->setEnabled(how==PICK_ANY);
    search->setEnabled(how!=PICK_NONE);

    dialog->SetResult(-1);

    // 20 allows for scrollbar or spacing
    // 4 for frame borders
    int mh = QApplication::desktop()->height()*3/5;
    if ( qt_compact_mode && totalHeight() > mh ) {
	// big, so make it fill
	dialog->showMaximized();
    } else {
	dialog->resize(totalWidth()+20,
	    QMIN(totalHeight(), mh)+buth+4+(prompt.text().isNull() ? 0 : buth));
	if ( dialog->width() > QApplication::desktop()->width() )
	    dialog->resize(QApplication::desktop()->width(),dialog->height()+16);
	centerOnMain(dialog);
	dialog->show();
    }

    setFocus();
    while (dialog->result()<0) {
	// changed the defaults below to the values in wintype.h 000119 - azy
	if (!keysource.Empty()) {
	    char k=keysource.GetAscii();
	    k=map_menu_cmd(k); /* added 000119 - azy */
	    if (k=='\033')
		dialog->Reject();
	    else if (k=='\r' || k=='\n' || k==' ')
		dialog->Accept();
	    else if (k==MENU_SEARCH)
		Search();
	    else if (k==MENU_SELECT_ALL)
		All();
	    else if (k==MENU_INVERT_ALL)
		Invert();
	    else if (k==MENU_UNSELECT_ALL)
		ChooseNone();
	    else {
		for (int i=0; i<itemcount; i++) {
		    if (item[i].ch==k)
			ToggleSelect(i);
		}
	    }
	}
	if (dialog->result()<0)
	    qApp->enter_loop();
    }
    //if ( (nhid != WIN_INVEN || !iflags.perm_invent) ) // doesn't work yet
    {
	dialog->hide();
    }
    int result=dialog->result();

    // Consume ^M (which QDialog steals for default button)
    while (!keysource.Empty() &&
	(keysource.TopAscii()=='\n' || keysource.TopAscii()=='\r'))
	keysource.GetAscii();

    *menu_list=0;
    if (how==PICK_NONE)
	return result==0 ? -1 : 0;

    if (result>0) {
	if (how==PICK_ONE) {
	    int i;
	    for (i=0; i<itemcount && !item[i].selected; i++)
		;
	    if (i<itemcount) {
		*menu_list=(MENU_ITEM_P*)malloc(sizeof(MENU_ITEM_P));
		(*menu_list)[0].item=item[i].identifier;
		(*menu_list)[0].count=item[i].count;
		return 1;
	    } else {
		return 0;
	    }
	} else {
	    int count=0;
	    for (int i=0; i<itemcount; i++)
		if (item[i].selected) count++;
	    if (count) {
		*menu_list=(MENU_ITEM_P*)malloc(count*sizeof(MENU_ITEM_P));
		int j=0;
		for (int i=0; i<itemcount; i++) {
		    if (item[i].selected) {
			(*menu_list)[j].item=item[i].identifier;
			(*menu_list)[j].count=item[i].count;
			j++;
		    }
		}
		return count;
	    } else {
		return 0;
	    }
	}
    } else {
	return -1;
    }
}

void NetHackQtMenuWindow::keyPressEvent(QKeyEvent* event)
{
    if (viewHeight() < totalHeight() && !(event->state()&ShiftButton)) {
	if (event->key()==Key_Prior) {
	    setYOffset(yOffset()-viewHeight());
	} else if (event->key()==Key_Next) {
	    setYOffset(yOffset()+viewHeight());
	} else {
	    event->ignore();
	}
    } else {
	event->ignore();
    }
}

void NetHackQtMenuWindow::All()
{
    for (int i=0; i<itemcount; i++) {
	if (!item[i].selected) ToggleSelect(i);
    }
}
void NetHackQtMenuWindow::ChooseNone()
{
    for (int i=0; i<itemcount; i++) {
	if (item[i].selected) ToggleSelect(i);
    }
}

void NetHackQtMenuWindow::Invert()
{
    for (int i=0; i<itemcount; i++) {
	ToggleSelect(i);
    }
}

void NetHackQtMenuWindow::Search()
{
    NetHackQtStringRequestor requestor(keysource,"Search for:");
    char line[256];
    if (requestor.Get(line)) {
	for (int i=0; i<itemcount; i++) {
	    if (strstr(item[i].str,line))
		ToggleSelect(i);
	}
    }
}

void NetHackQtMenuWindow::ToggleSelect(int i)
{
    if (item[i].Selectable()) {
	item[i].selected = !item[i].selected;
	if ( !item[i].selected )
	    item[i].count=-1;
	updateCell(i,3);
	if (how==PICK_ONE) {
	    dialog->Accept();
	}
    }
}


void NetHackQtMenuWindow::paintCell(QPainter* painter, int row, int col)
{
    // [pick-count] [accel] [glyph] [string]

    MenuItem& i = item[row];

    painter->setPen(black);
    painter->setFont(font());

    if (i.selected) {
	painter->setPen(darkGreen);
    }

    switch (col) {
     case 0:
	if ( i.ch || i.attr!=ATR_INVERSE ) {
	    QString text;
	    if ( i.selected && i.count == -1 ) {
		if ( i.str[0]>='0' && i.str[0]<='9' )
		    text = "All";
		else
		    text = "*";
	    } else if ( i.count<0 ) {
		text = "-";
	    } else {
		text.sprintf("%d",i.count);
	    }
	    painter->drawText(0,0,cellWidth(col),cellHeight(),
	    AlignHCenter|AlignVCenter,text);
	}
    break; case 1:
	if ((signed char)i.ch >= 0) {
	    char text[2]={i.ch,0};
	    painter->drawText(0,0,cellWidth(col),cellHeight(),
		AlignHCenter|AlignVCenter,text);
	}
    break; case 2:
	if (i.glyph!=NO_GLYPH) {
	    // Centered in height
	    int y=(cellHeight()-qt_settings->glyphs().height())/2;
	    if (y<0) y=0;
	    qt_settings->glyphs().drawGlyph(*painter, i.glyph, 0, y);
	}
    break; case 3:
	// XXX should qt_settings have ALL the various fonts
	QFont newfont=font();

	if (i.attr) {
	    switch(i.attr) {
	     case ATR_ULINE:
		newfont.setUnderline(TRUE);
	    break; case ATR_BOLD:
		painter->setPen(red);
	    break; case ATR_BLINK:
		newfont.setItalic(TRUE);
	    break; case ATR_INVERSE:
		newfont=qt_settings->largeFont();
		newfont.setWeight(QFont::Bold);

		if (i.selected) {
		    painter->setPen(blue);
		} else {
		    painter->setPen(darkBlue);
		}
	    }
	}
	painter->setFont(newfont);

	painter->drawText(STR_MARGIN,0,cellWidth(col),cellHeight(),
	    AlignLeft|AlignVCenter,i.str);
    }
}

void NetHackQtMenuWindow::mousePressEvent(QMouseEvent* event)
{
    int col=findCol(event->pos().x());
    int row=findRow(event->pos().y());

    if (col<0 || row<0 || !item[row].Selectable()) return;

    if (how!=PICK_NONE) {
	if (col==0) {
	    // Changing count.
	    NetHackQtStringRequestor requestor(keysource,"Count:");
	    char buf[BUFSZ];

	    if (item[row].count>0)
		Sprintf(buf,"%d", item[row].count);
	    else
		Strcpy(buf, "");

	    requestor.SetDefault(buf);
	    if (requestor.Get(buf)) {
		item[row].count=atoi(buf);
		if (item[row].count==0) {
		    item[row].count=-1;
		    if (item[row].selected) ToggleSelect(row);
		} else {
		    if (!item[row].selected) ToggleSelect(row);
		}
		updateCell(row,0);
	    }
	} else {
	    pressed=row;
	    was_sel=item[row].selected;
	    ToggleSelect(row);
	    updateCell(row,0);
	}
    }
}

void NetHackQtMenuWindow::mouseReleaseEvent(QMouseEvent* event)
{
    if (pressed>=0) {
	int p=pressed;
	pressed=-1;
	updateCell(p,3);
    }
}

void NetHackQtMenuWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (pressed>=0) {
	int col=findCol(event->pos().x());
	int row=findRow(event->pos().y());

	if (row>=0 && col>=0) {
	    if (pressed!=row) {
		// reset to initial state
		if (item[pressed].selected!=was_sel)
		    ToggleSelect(pressed);
	    } else {
		// reset to new state
		if (item[pressed].selected==was_sel)
		    ToggleSelect(pressed);
	    }
	}
    }
}


class NetHackQtTextListBox : public QListBox {
public:
    NetHackQtTextListBox(QWidget* parent) : QListBox(parent) { }

    int TotalWidth()
    {
	doLayout();
	return contentsWidth();
    }
    int TotalHeight()
    {
	doLayout();
	return contentsHeight();
    }

    virtual void setFont(const QFont &font)
    {
	QListBox::setFont(font);
    }
    void keyPressEvent(QKeyEvent* e)
    {
	QListBox::keyPressEvent(e);
    }
};


QPixmap* NetHackQtRIP::pixmap=0;

NetHackQtRIP::NetHackQtRIP(QWidget* parent) :
    QWidget(parent)
{
    if (!pixmap) {
	pixmap=new QPixmap;
	tryload(*pixmap, "rip.xpm");
    }
    riplines=0;
    resize(pixmap->width(),pixmap->height());
    setFont(QFont("times",12)); // XXX may need to be configurable
}

void NetHackQtRIP::setLines(char** l, int n)
{
    line=l;
    riplines=n;
}

QSize NetHackQtRIP::sizeHint() const
{
    return pixmap->size();
}

void NetHackQtRIP::paintEvent(QPaintEvent* event)
{
    if ( riplines ) {
	int pix_x=(width()-pixmap->width())/2;
	int pix_y=(height()-pixmap->height())/2;

	// XXX positions based on RIP image
	int rip_text_x=pix_x+156;
	int rip_text_y=pix_y+67;
	int rip_text_h=94/riplines;

	QPainter painter;
	painter.begin(this);
	painter.drawPixmap(pix_x,pix_y,*pixmap);
	for (int i=0; i<riplines; i++) {
	    painter.drawText(rip_text_x-i/2,rip_text_y+i*rip_text_h,
		1,1,DontClip|AlignHCenter,line[i]);
	}
	painter.end();
    }
}

NetHackQtTextWindow::NetHackQtTextWindow(NetHackQtKeyBuffer& ks) :
    QDialog(qApp->mainWidget(),0,FALSE),
    keysource(ks),
    use_rip(FALSE),
    str_fixed(FALSE),
    ok("Dismiss",this),
    search("Search",this),
    lines(new NetHackQtTextListBox(this)),
    rip(this)
{
    ok.setDefault(TRUE);
    connect(&ok,SIGNAL(clicked()),this,SLOT(accept()));
    connect(&search,SIGNAL(clicked()),this,SLOT(Search()));
    connect(qt_settings,SIGNAL(fontChanged()),this,SLOT(doUpdate()));

    QVBoxLayout* vb = new QVBoxLayout(this);
    vb->addWidget(&rip);
    QHBoxLayout* hb = new QHBoxLayout(vb);
    hb->addWidget(&ok);
    hb->addWidget(&search);
    vb->addWidget(lines);
}

void NetHackQtTextWindow::doUpdate()
{
    update();
}


NetHackQtTextWindow::~NetHackQtTextWindow()
{

}

QWidget* NetHackQtTextWindow::Widget()
{
    return this;
}

bool NetHackQtTextWindow::Destroy()
{
    return !isVisible();
}

void NetHackQtTextWindow::UseRIP(int how, time_t when)
{
// Code from X11 windowport
#define STONE_LINE_LEN 16    /* # chars that fit on one line */
#define NAME_LINE 0	/* line # for player name */
#define GOLD_LINE 1	/* line # for amount of gold */
#define DEATH_LINE 2	/* line # for death description */
#define YEAR_LINE 6	/* line # for year */

static char** rip_line=0;
    if (!rip_line) {
	rip_line=new char*[YEAR_LINE+1];
	for (int i=0; i<YEAR_LINE+1; i++) {
	    rip_line[i]=new char[STONE_LINE_LEN+1];
	}
    }

    /* Follows same algorithm as genl_outrip() */

    char buf[BUFSZ];
    char *dpx;
    int line;
    long year;

    /* Put name on stone */
    Sprintf(rip_line[NAME_LINE], "%s", g.plname);

    /* Put $ on stone */
    Sprintf(rip_line[GOLD_LINE], "%ld Au", done_money);

    /* Put together death description */
    formatkiller(buf, sizeof buf, how, FALSE);

    /* Put death type on stone */
    for (line=DEATH_LINE, dpx = buf; line<YEAR_LINE; line++) {
	register int i,i0;
	char tmpchar;

	if ( (i0=strlen(dpx)) > STONE_LINE_LEN) {
	    for(i = STONE_LINE_LEN;
		((i0 > STONE_LINE_LEN) && i); i--)
		if(dpx[i] == ' ') i0 = i;
	    if(!i) i0 = STONE_LINE_LEN;
	}
	tmpchar = dpx[i0];
	dpx[i0] = 0;
	strcpy(rip_line[line], dpx);
	if (tmpchar != ' ') {
	    dpx[i0] = tmpchar;
	    dpx= &dpx[i0];
	} else  dpx= &dpx[i0+1];
    }

    /* Put year on stone */
    year = yyyymmdd(when) / 10000L;
    Sprintf(rip_line[YEAR_LINE], "%4ld", year);

    rip.setLines(rip_line,YEAR_LINE+1);

    use_rip=TRUE;
}

void NetHackQtTextWindow::Clear()
{
    lines->clear();
    use_rip=FALSE;
    str_fixed=FALSE;
}

void NetHackQtTextWindow::Display(bool block)
{
    if (str_fixed) {
	lines->setFont(qt_settings->normalFixedFont());
    } else {
	lines->setFont(qt_settings->normalFont());
    }

    int h=0;
    if (use_rip) {
	h+=rip.height();
	ok.hide();
	search.hide();
	rip.show();
    } else {
	h+=ok.height()*2;
	ok.show();
	search.show();
	rip.hide();
    }
    int mh = QApplication::desktop()->height()*3/5;
    if ( qt_compact_mode && (lines->TotalHeight() > mh || use_rip) ) {
	// big, so make it fill
	showMaximized();
    } else {
	resize(QMAX(use_rip ? rip.width() : 200,
		lines->TotalWidth()+24),
	    QMIN(mh, lines->TotalHeight()+h));
	centerOnMain(this);
	show();
    }
    if (block) {
	setResult(-1);
	while (result()==-1) {
	    qApp->enter_loop();
	    if (result()==-1 && !keysource.Empty()) {
		char k=keysource.GetAscii();
		if (k=='\033' || k==' ' || k=='\r' || k=='\n') {
		    accept();
		} else if (k=='/') {
		    Search();
		}
	    }
	}
    }
}

void NetHackQtTextWindow::PutStr(int attr, const char* text)
{
    str_fixed=str_fixed || strstr(text,"    ");
    lines->insertItem(text);
}

void NetHackQtTextWindow::done(int i)
{
    setResult(i+1000);
    hide();
    qApp->exit_loop();
}

void NetHackQtTextWindow::keyPressEvent(QKeyEvent* e)
{
    if ( e->ascii() != '\r' && e->ascii() != '\n' && e->ascii() != '\033' )
	lines->keyPressEvent(e);
    else
	QDialog::keyPressEvent(e);
}

void NetHackQtTextWindow::Search()
{
    NetHackQtStringRequestor requestor(keysource,"Search for:");
    static char line[256]="";
    requestor.SetDefault(line);
    if (requestor.Get(line)) {
	int current=lines->currentItem();
	for (uint i=1; i<lines->count(); i++) {
	    int lnum=(i+current)%lines->count();
	    QString str=lines->text(lnum);
	    if (str.contains(line)) {
		lines->setCurrentItem(lnum);
		lines->centerCurrentItem();
		return;
	    }
	}
	lines->setCurrentItem(-1);
    }
}


NetHackQtDelay::NetHackQtDelay(int ms) :
    msec(ms)
{
}

void NetHackQtDelay::wait()
{
    startTimer(msec);
    qApp->enter_loop();
}

void NetHackQtDelay::timerEvent(QTimerEvent* timer)
{
    qApp->exit_loop();
    killTimers();
}

NetHackQtInvUsageWindow::NetHackQtInvUsageWindow(QWidget* parent) :
    QWidget(parent)
{
}

void NetHackQtInvUsageWindow::drawWorn(QPainter& painter, obj* nhobj, int x, int y, bool canbe)
{
    short int glyph;
    if (nhobj)
	glyph=obj_to_glyph(nhobj, rn2_on_display_rng);
    else if (canbe)
	glyph=cmap_to_glyph(S_room);
    else
	glyph=cmap_to_glyph(S_stone);

    qt_settings->glyphs().drawCell(painter,glyph,x,y);
}

void NetHackQtInvUsageWindow::paintEvent(QPaintEvent*)
{
    //  012
    //
    //0 WhB   
    //1 s"w   
    //2 gCg   
    //3 =A=   
    //4  T    
    //5  S    

    QPainter painter;
    painter.begin(this);

    // Blanks
    drawWorn(painter,0,0,4,FALSE);
    drawWorn(painter,0,0,5,FALSE);
    drawWorn(painter,0,2,4,FALSE);
    drawWorn(painter,0,2,5,FALSE);

    drawWorn(painter,uarm,1,3); // Armour
    drawWorn(painter,uarmc,1,2); // Cloak
    drawWorn(painter,uarmh,1,0); // Helmet
    drawWorn(painter,uarms,0,1); // Shield
    drawWorn(painter,uarmg,0,2); // Gloves - repeated
    drawWorn(painter,uarmg,2,2); // Gloves - repeated
    drawWorn(painter,uarmf,1,5); // Shoes (feet)
    drawWorn(painter,uarmu,1,4); // Undershirt
    drawWorn(painter,uleft,0,3); // RingL
    drawWorn(painter,uright,2,3); // RingR

    drawWorn(painter,uwep,2,1); // Weapon
    drawWorn(painter,uswapwep,0,0); // Secondary weapon
    drawWorn(painter,uamul,1,1); // Amulet
    drawWorn(painter,ublindf,2,0); // Blindfold

    painter.end();
}

class SmallToolButton : public QToolButton {
public:
    SmallToolButton(const QPixmap & pm, const QString &textLabel,
                 const QString& grouptext,
                 QObject * receiver, const char* slot,
                 QToolBar * parent) :
	QToolButton(pm, textLabel,
#if QT_VERSION < 210
		QString::null,
#else
		grouptext,
#endif
		    receiver, slot, parent)
    {
    }

    QSize sizeHint() const
    {
	// get just a couple more pixels for the map
	return QToolButton::sizeHint()-QSize(0,2);
    }
};


NetHackQtMainWindow::NetHackQtMainWindow(NetHackQtKeyBuffer& ks) :
    message(0), map(0), status(0), invusage(0),
    keysink(ks), dirkey(0)
{
    QToolBar* toolbar = new QToolBar(this);
#if QT_VERSION >= 210
    setToolBarsMovable(FALSE);
    toolbar->setHorizontalStretchable(TRUE);
    toolbar->setVerticalStretchable(TRUE);
#endif
    addToolBar(toolbar);
    menubar = menuBar();

    setCaption("Qt NetHack");
    if ( qt_compact_mode )
	setIcon(QPixmap(nh_icon_small));
    else
	setIcon(QPixmap(nh_icon));

    QPopupMenu* game=new QPopupMenu;
    QPopupMenu* apparel=new QPopupMenu;
    QPopupMenu* act1=new QPopupMenu;
    QPopupMenu* act2 = qt_compact_mode ? new QPopupMenu : act1;
    QPopupMenu* magic=new QPopupMenu;
    QPopupMenu* info=new QPopupMenu;

    QPopupMenu *help;

#ifdef KDE
    help = kapp->getHelpMenu( TRUE, "" );
    help->insertSeparator();
#else
    help = qt_compact_mode ? info : new QPopupMenu;
#endif

    enum { OnDesktop=1, OnHandhelds=2 };
    struct Macro {
	QPopupMenu* menu;
	const char* name;
	const char* action;
	int flags;
    } item[] = {
	{ game,		0, 0, 3},
	{ game,		"Version\tv",           "v", 3},
	{ game,		"Compilation\tAlt+V",     "\366", 3},
	{ game,		"History\tShift+V",           "V", 3},
	{ game,		"Redraw\tCtrl+R",          "\022", 0}, // useless
	{ game,		"Options\tShift+O",           "O", 3},
	{ game,		"Explore mode\tShift+X",      "X", 3},
	{ game,		0, 0, 3},
	{ game,		"Save\tSy",              "Sy", 3},
	{ game,		"Quit\tAlt+Q",                "\361", 3},

	{ apparel,	"Apparel off\tShift+A",       "A", 2},
	{ apparel,	"Remove many\tShift+A",       "A", 1},
	{ apparel,	0, 0, 3},
	{ apparel,	"Wield weapon\tw",      "w", 3},
	{ apparel,	"Exchange weapons\tx",      "x", 3},
	{ apparel,	"Two weapon combat\t#two",      "#tw", 3},
	{ apparel,	"Load quiver\tShift+Q",       "Q", 3},
	{ apparel,	0, 0, 3},
	{ apparel,	"Wear armour\tShift+W",       "W", 3},
	{ apparel,	"Take off armour\tShift+T",   "T", 3},
	{ apparel,	0, 0, 3},
	{ apparel,	"Put on non-armour\tShift+P", "P", 3},
	{ apparel,	"Remove non-armour\tShift+R", "R", 3},

	{ act1,	"Again\tCtrl+A",        "\001", 2},
	{ act1,	0, 0, 3},
	{ act1,	"Apply\ta?",            "a?", 3},
	{ act1,	"Chat\tAlt+C",          "\343", 3},
	{ act1,	"Close door\tc",        "c", 3},
	{ act1,	"Down\t>",              ">", 3},
	{ act1,	"Drop many\tShift+D",   "D", 2},
	{ act1,	"Drop\td?",             "d?", 2},
	{ act1,	"Eat\te?",              "e?", 2},
	{ act1,	"Engrave\tShift+E",     "E", 3},
	{ act1,	"Fight\tShift+F",       "F", 3},
	{ act1,	"Fire from quiver\tf",  "f", 2},
	{ act1,	"Force\tAlt+F",         "\346", 3},
	{ act1,	"Get\t,",               ",", 2},
	{ act1,	"Jump\tAlt+J",          "\352", 3},
	{ act2,	"Kick\tCtrl+D",         "\004", 2},
	{ act2,	"Loot\tAlt+L",          "\354", 3},
	{ act2,	"Open door\to",         "o", 3},
	{ act2,	"Pay\tp",               "p", 3},
	{ act2,	"Rest\t.",              ".", 2},
	{ act2,	"Ride\t#ri",            "#ri", 3},
	{ act2,	"Search\ts",            "s", 3},
	{ act2,	"Sit\tAlt+S",           "\363", 3},
	{ act2,	"Throw\tt",             "t", 2},
	{ act2,	"Untrap\t#u",           "#u", 3},
	{ act2,	"Up\t<",                "<", 3},
	{ act2,	"Wipe face\tAlt+W",     "\367", 3},

	{ magic,	"Quaff potion\tq?",      "q?", 3},
	{ magic,	"Read scroll/book\tr?", "r?", 3},
	{ magic,	"Zap wand\tz?",         "z?", 3},
	{ magic,	"Zap spell\tShift+Z",        "Z", 3},
	{ magic,	"Dip\tAlt+D",             "\344", 3},
	{ magic,	"Rub\tAlt+R",             "\362", 3},
	{ magic,	"Invoke\tAlt+I",          "\351", 3},
	{ magic,	0, 0, 3},
	{ magic,	"Offer\tAlt+O",           "\357", 3},
	{ magic,	"Pray\tAlt+P",            "\360", 3},
	{ magic,	0, 0, 3},
	{ magic,	"Teleport\tCtrl+T",        "\024", 3},
	{ magic,	"Monster action\tAlt+M",  "\355", 3},
	{ magic,	"Turn undead\tAlt+T",     "\364", 3},

	{ help,		"Help\t?",              "?", 3},
	{ help,		0, 0, 3},
	{ help,		"What is here\t:",      ":", 3},
	{ help,		"What is there\t;",      ";", 3},
	{ help,		"What is...\t/y",        "/y", 2},
	{ help,		0, 0, 1},

	{ info,		"Inventory\ti",         "i", 3},
#ifdef SLASHEM
	{ info,		"Angbandish inventory\t*",    "*", 3},
#endif
	{ info,		"Conduct\t#co",         "#co", 3},
	{ info,		"Discoveries\t\\",      "\\", 3},
	{ info,		"List/reorder spells\t+",     "+", 3},
	{ info,		"Adjust letters\tAlt+A",  "\341", 2},
	{ info,		0, 0, 3},
	{ info,		"Name object\tAlt+N",    "\356y?", 3},
	{ info,		"Name object type\tAlt+N",    "\356n?", 3},
	{ info,		"Name creature\tShift+C",      "C", 3},
	{ info,		0, 0, 3},
	{ info,		"Qualifications\tAlt+E",  "\345", 3},

	{ 0, 0, 0, 0 }
    };

    int i;
    int count=0;
    for (i=0; item[i].menu; i++)
	if (item[i].name) count++;

    macro=new const char* [count];

    game->insertItem("Qt settings...",1000);
    help->insertItem("About Qt NetHack...",2000);
    //help->insertItem("NetHack Guidebook...",3000);
    help->insertSeparator();

    count=0;
    for (i=0; item[i].menu; i++) {
	if ( item[i].flags & (qt_compact_mode ? 1 : 2) ) {
	    if (item[i].name) {
		QString name = item[i].name;
		if ( qt_compact_mode ) // accelerators aren't
		    name.replace(QRegExp("\t.*"),"");
		item[i].menu->insertItem(name,count);
		macro[count++]=item[i].action;
	    } else {
		item[i].menu->insertSeparator();
	    }
	}
    }

    menubar->insertItem("Game",game);
    menubar->insertItem("Gear",apparel);

    if ( qt_compact_mode ) {
	menubar->insertItem("A-J",act1);
	menubar->insertItem("K-Z",act2);
	menubar->insertItem("Magic",magic);
	menubar->insertItem(QPixmap(info_xpm),info);
	menubar->insertItem(QPixmap(map_xpm), this, SLOT(raiseMap()));
	menubar->insertItem(QPixmap(msg_xpm), this, SLOT(raiseMessages()));
	menubar->insertItem(QPixmap(stat_xpm), this, SLOT(raiseStatus()));
    } else {
	menubar->insertItem("Action",act1);
	menubar->insertItem("Magic",magic);
	menubar->insertItem("Info",info);
	menubar->insertSeparator();
	menubar->insertItem("Help",help);
    }

    QSignalMapper* sm = new QSignalMapper(this);
    connect(sm, SIGNAL(mapped(const QString&)), this, SLOT(doKeys(const QString&)));
    QToolButton* tb;
    tb = new SmallToolButton( QPixmap(again_xpm),"Again","Action", sm, SLOT(map()), toolbar );
    sm->setMapping(tb, "\001" );
    tb = new SmallToolButton( QPixmap(get_xpm),"Get","Action", sm, SLOT(map()), toolbar );
    sm->setMapping(tb, "," );
    tb = new SmallToolButton( QPixmap(kick_xpm),"Kick","Action", sm, SLOT(map()), toolbar );
    sm->setMapping(tb, "\004" );
    tb = new SmallToolButton( QPixmap(throw_xpm),"Throw","Action", sm, SLOT(map()), toolbar );
    sm->setMapping(tb, "t" );
    tb = new SmallToolButton( QPixmap(fire_xpm),"Fire","Action", sm, SLOT(map()), toolbar );
    sm->setMapping(tb, "f" );
    tb = new SmallToolButton( QPixmap(drop_xpm),"Drop","Action", sm, SLOT(map()), toolbar );
    sm->setMapping(tb, "D" );
    tb = new SmallToolButton( QPixmap(eat_xpm),"Eat","Action", sm, SLOT(map()), toolbar );
    sm->setMapping(tb, "e" );
    tb = new SmallToolButton( QPixmap(rest_xpm),"Rest","Action", sm, SLOT(map()), toolbar );
    sm->setMapping(tb, "." );
    tb = new SmallToolButton( QPixmap(cast_a_xpm),"Cast A","Magic", sm, SLOT(map()), toolbar );
    sm->setMapping(tb, "Za" );
    tb = new SmallToolButton( QPixmap(cast_b_xpm),"Cast B","Magic", sm, SLOT(map()), toolbar );
    sm->setMapping(tb, "Zb" );
    tb = new SmallToolButton( QPixmap(cast_c_xpm),"Cast C","Magic", sm, SLOT(map()), toolbar );
    sm->setMapping(tb, "Zc" );
    if ( !qt_compact_mode ) {
	QWidget* filler = new QWidget(toolbar);
	filler->setBackgroundMode(PaletteButton);
	toolbar->setStretchableWidget(filler);
    }

    connect(menubar,SIGNAL(activated(int)),this,SLOT(doMenuItem(int)));

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
	stack = new QWidgetStack(this);
	setCentralWidget(stack);
    } else {
	setCentralWidget(new QWidget(this));
	invusage = new NetHackQtInvUsageWindow(centralWidget());
    }
}

void NetHackQtMainWindow::zoomMap()
{
    qt_settings->toggleGlyphSize();
}

void NetHackQtMainWindow::raiseMap()
{
    if ( stack->id(stack->visibleWidget()) == 0 ) {
	zoomMap();
    } else {
	stack->raiseWidget(0);
    }
}

void NetHackQtMainWindow::raiseMessages()
{
    stack->raiseWidget(1);
}

void NetHackQtMainWindow::raiseStatus()
{
    stack->raiseWidget(2);
}

class NetHackMimeSourceFactory : public QMimeSourceFactory {
public:
    const QMimeSource* data(const QString& abs_name) const
    {
	const QMimeSource* r = 0;
	if ( (NetHackMimeSourceFactory*)this == QMimeSourceFactory::defaultFactory() )
	    r = QMimeSourceFactory::data(abs_name);
	else
	    r = QMimeSourceFactory::defaultFactory()->data(abs_name);
	if ( !r ) {
	    int sl = abs_name.length();
	    do {
		sl = abs_name.findRev('/',sl-1);
		QString name = sl>=0 ? abs_name.mid(sl+1) : abs_name;
		int dot = name.findRev('.');
		if ( dot >= 0 )
		    name = name.left(dot);
		if ( name == "map" )
		    r = new QImageDrag(QImage(map_xpm));
		else if ( name == "msg" )
		    r = new QImageDrag(QImage(msg_xpm));
		else if ( name == "stat" )
		    r = new QImageDrag(QImage(stat_xpm));
	    } while (!r && sl>0);
	}
	return r;
    }
};

void NetHackQtMainWindow::doMenuItem(int id)
{
    switch (id) {
      case 1000:
	centerOnMain(qt_settings);
	qt_settings->show();
	break;
      case 2000:
	QMessageBox::about(this,  "About Qt NetHack", aboutMsg());
	break;
      case 3000: {
	    QDialog dlg(this,0,TRUE);
	    (new QVBoxLayout(&dlg))->setAutoAdd(TRUE);
	    QTextBrowser browser(&dlg);
	    NetHackMimeSourceFactory ms;
	    browser.setMimeSourceFactory(&ms);
	    browser.setSource(QDir::currentDirPath()+"/Guidebook.html");
	    if ( qt_compact_mode )
		dlg.showMaximized();
	    dlg.exec();
	}
	break;
      default:
	if ( id >= 0 )
	    doKeys(macro[id]);
    }
}

void NetHackQtMainWindow::doKeys(const QString& k)
{
    keysink.Put(k);
    qApp->exit_loop();
}

void NetHackQtMainWindow::AddMessageWindow(NetHackQtMessageWindow* window)
{
    message=window;
    ShowIfReady();
}

void NetHackQtMainWindow::AddMapWindow(NetHackQtMapWindow* window)
{
    map=window;
    ShowIfReady();
    connect(map,SIGNAL(resized()),this,SLOT(layout()));
}

void NetHackQtMainWindow::AddStatusWindow(NetHackQtStatusWindow* window)
{
    status=window;
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
    if ( invusage )
	invusage->repaint(FALSE);
}

void NetHackQtMainWindow::fadeHighlighting()
{
    if (status) {
	status->fadeHighlighting();
    }
}

void NetHackQtMainWindow::layout()
{
    if ( qt_compact_mode )
	return;
    if (message && map && status) {
	QSize maxs=map->Widget()->maximumSize();
	int maph=QMIN(height()*2/3,maxs.height());

	QWidget* c = centralWidget();
	int h=c->height();
	int toph=h-maph;
	int iuw=3*qt_settings->glyphs().width();
	int topw=(c->width()-iuw)/2;

	message->Widget()->setGeometry(0,0,topw,toph);
	invusage->setGeometry(topw,0,iuw,toph);
	status->Widget()->setGeometry(topw+iuw,0,topw,toph);
	map->Widget()->setGeometry(QMAX(0,(c->width()-maxs.width())/2),
				   toph,c->width(),maph);
    }
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
	event->key() >= Key_Left && event->key() <= Key_Down )
	return;

    const char* d = Cmd.dirchars;
    switch (event->key()) {
     case Key_Up:
	if ( dirkey == d[0] )
	    dirkey = d[1];
	else if ( dirkey == d[4] )
	    dirkey = d[3];
	else
	    dirkey = d[2];
    break; case Key_Down:
	if ( dirkey == d[0] )
	    dirkey = d[7];
	else if ( dirkey == d[4] )
	    dirkey = d[5];
	else
	    dirkey = d[6];
    break; case Key_Left:
	if ( dirkey == d[2] )
	    dirkey = d[1];
	else if ( dirkey == d[6] )
	    dirkey = d[7];
	else
	    dirkey = d[0];
    break; case Key_Right:
	if ( dirkey == d[2] )
	    dirkey = d[3];
	else if ( dirkey == d[6] )
	    dirkey = d[5];
	else
	    dirkey = d[4];
    break; case Key_Prior:
	dirkey = 0;
	if (message) message->Scroll(0,-1);
    break; case Key_Next:
	dirkey = 0;
	if (message) message->Scroll(0,+1);
    break; case Key_Space:
	if ( flags.rest_on_space ) {
	    event->ignore();
	    return;
	}
	case Key_Enter:
	if ( map )
	    map->clickCursor();
    break; default:
	dirkey = 0;
	event->ignore();
    }
}

void NetHackQtMainWindow::closeEvent(QCloseEvent* e)
{
    if ( g.program_state.something_worth_saving ) {
	switch ( QMessageBox::information( this, "NetHack",
	    "This will end your NetHack session",
	    "&Save", "&Cancel", 0, 1 ) )
	{
	    case 0:
		// See dosave() function
		if (dosave0()) {
		    u.uhp = -1;
		    NetHackQtBind::qt_exit_nhwindows(0);
		    nh_terminate(EXIT_SUCCESS);
		}
		break;
	    case 1:
		break; // ignore the event
	}
    } else {
	e->accept();
    }
}

void NetHackQtMainWindow::ShowIfReady()
{
    if (message && map && status) {
	QPoint pos(0,0);
	QWidget* p = qt_compact_mode ? stack : centralWidget();
	message->Widget()->recreate(p,0,pos);
	map->Widget()->recreate(p,0,pos);
	status->Widget()->recreate(p,0,pos);
	if ( qt_compact_mode ) {
	    message->setMap(map);
	    stack->addWidget(map->Widget(), 0);
	    stack->addWidget(message->Widget(), 1);
	    stack->addWidget(status->Widget(), 2);
	    raiseMap();
	} else {
	    layout();
	}
	showMaximized();
    } else if (isVisible()) {
	hide();
    }
}


NetHackQtYnDialog::NetHackQtYnDialog(NetHackQtKeyBuffer& keysrc,const char* q,const char* ch,char df) :
    QDialog(qApp->mainWidget(),0,FALSE),
    question(q), choices(ch), def(df),
    keysource(keysrc)
{
    setCaption("NetHack: Question");
}

char NetHackQtYnDialog::Exec()
{
    QString ch(choices);
    int ch_per_line=6;
    QString qlabel;
    QString enable;
    if ( qt_compact_mode && !choices ) {
	// expand choices from prompt
	// ##### why isn't choices set properly???
	const char* c=question;
	while ( *c && *c != '[' )
	    c++;
	qlabel = QString(question).left(c-question);
	if ( *c ) {
	    c++;
	    if ( *c == '-' )
		ch.append(*c++);
	    char from=0;
	    while ( *c && *c != ']' && *c != ' ' ) {
		if ( *c == '-' ) {
		    from = c[-1];
		} else if ( from ) {
		    for (char f=from+1; f<=*c; f++)
			ch.append(f);
		    from = 0;
		} else {
		    ch.append(*c);
		    from = 0;
		}
		c++;
	    }
	    if ( *c == ' ' ) {
		while ( *c && *c != ']' ) {
		    if ( *c == '*' || *c == '?' )
			ch.append(*c);
		    c++;
		}
	    }
	}
	if ( strstr(question, "what direction") ) {
	    // We replace this regardless, since sometimes you get choices.
	    const char* d = Cmd.dirchars;
	    enable=ch;
	    ch="";
	    ch.append(d[1]);
	    ch.append(d[2]);
	    ch.append(d[3]);
	    ch.append(d[0]);
	    ch.append('.');
	    ch.append(d[4]);
	    ch.append(d[7]);
	    ch.append(d[6]);
	    ch.append(d[5]);
	    ch.append(d[8]);
	    ch.append(d[9]);
	    ch_per_line = 3;
	    def = ' ';
	} else {
	    // Hmm... they'll have to use a virtual keyboard
	}
    } else {
	qlabel = question;
    }
    if (!ch.isNull()) {
	QVBoxLayout vb(this);
	vb.setAutoAdd(TRUE);
	bool bigq = qlabel.length()>40;
	if ( bigq ) {
	    QLabel* q = new QLabel(qlabel,this);
	    q->setAlignment(AlignLeft|WordBreak);
	    q->setMargin(4);
	}
	QButtonGroup group(ch_per_line, Horizontal,
	    bigq ? QString::null : qlabel, this);

	int nchoices=ch.length();

	bool allow_count=ch.contains('#');

	const int margin=8;
	const int gutter=8;
	const int extra=fontMetrics().height(); // Extra for group
	int x=margin, y=extra+margin;
	int butsize=fontMetrics().height()*2+5;

	QPushButton* button;
	for (int i=0; i<nchoices && ch[i]!='\033'; i++) {
	    button=new QPushButton(QString(ch[i]),&group);
	    if ( !enable.isNull() ) {
		if ( !enable.contains(ch[i]) )
		    button->setEnabled(FALSE);
	    }
	    button->setFixedSize(butsize,butsize); // Square
	    if (ch[i]==def) button->setDefault(TRUE);
	    if (i%10==9) {
		// last in row
		x=margin;
		y+=butsize+gutter;
	    } else {
		x+=butsize+gutter;
	    }
	}

	connect(&group,SIGNAL(clicked(int)),this,SLOT(doneItem(int)));

	QLabel* lb=0;
	QLineEdit* le=0;

	if (allow_count) {
	    QHBox *hb = new QHBox(this);
	    lb=new QLabel("Count: ",hb);
	    le=new QLineEdit(hb);
	}

	adjustSize();
	centerOnMain(this);
	show();
	char choice=0;
	char ch_esc=0;
	for (uint i=0; i<ch.length(); i++) {
	    if (ch[i].latin1()=='q') ch_esc='q';
	    else if (!ch_esc && ch[i].latin1()=='n') ch_esc='n';
	}
	setResult(-1);
	while (!choice) {
	    if (!keysource.Empty()) {
		char k=keysource.GetAscii();
		char ch_esc=0;
		for (uint i=0; i<ch.length(); i++)
		    if (ch[i].latin1()==k)
			choice=k;
		if (!choice) {
		    if (k=='\033' && ch_esc)
			choice=ch_esc;
		    else if (k==' ' || k=='\r' || k=='\n')
			choice=def;
		    // else choice remains 0
		}
	    } else if ( result() == 0 ) {
		choice = ch_esc ? ch_esc : def ? def : ' ';
	    } else if ( result() == 1 ) {
		choice = def ? def : ch_esc ? ch_esc : ' ';
	    } else if ( result() >= 1000 ) {
		choice = ch[result() - 1000].latin1();
	    }
	    if ( !choice )
		qApp->enter_loop();
	}
	hide();
	if (allow_count && !le->text().isEmpty()) {
	    yn_number=atoi(le->text());
	    choice='#';
	}
	return choice;
    } else {
	QLabel label(qlabel,this);
	QPushButton cancel("Dismiss",this);
	label.setFrameStyle(QFrame::Box|QFrame::Sunken);
	label.setAlignment(AlignCenter);
	label.resize(fontMetrics().width(qlabel)+60,30+fontMetrics().height());
	cancel.move(width()/2-cancel.width()/2,label.geometry().bottom()+8);
	connect(&cancel,SIGNAL(clicked()),this,SLOT(reject()));
	centerOnMain(this);
	setResult(-1);
	show();
	while (result()<0 && keysource.Empty()) {
	    qApp->enter_loop();
	}
	hide();
	if (keysource.Empty()) {
	    return '\033';
	} else {
	    return keysource.GetAscii();
	}
    }
}
void NetHackQtYnDialog::keyPressEvent(QKeyEvent* event)
{
    // Don't want QDialog's Return/Esc behavior
    event->ignore();
}

void NetHackQtYnDialog::doneItem(int i)
{
    done(i+1000);
}

void NetHackQtYnDialog::done(int i)
{
    setResult(i);
    qApp->exit_loop();
}

NetHackQtGlyphs::NetHackQtGlyphs()
{
    const char* tile_file = "nhtiles.bmp";
    if ( iflags.wc_tile_file )
	tile_file = iflags.wc_tile_file;

    if (!img.load(tile_file)) {
	tile_file = "x11tiles";
	if (!img.load(tile_file)) {
	    QString msg;
	    msg.sprintf("Cannot load x11tiles or nhtiles.bmp");
	    QMessageBox::warning(0, "IO Error", msg);
	} else {
	    tiles_per_row = TILES_PER_ROW;
	    if (img.width()%tiles_per_row) {
		impossible(
            "Tile file \"%s\" has %d columns, not multiple of row count (%d)",
                           tile_file, img.width(), tiles_per_row);
	    }
	}
    } else {
	tiles_per_row = 40;
    }

    if ( iflags.wc_tile_width )
	tilefile_tile_W = iflags.wc_tile_width;
    else
	tilefile_tile_W = img.width() / tiles_per_row;
    if ( iflags.wc_tile_height )
	tilefile_tile_H = iflags.wc_tile_height;
    else
	tilefile_tile_H = tilefile_tile_W;

    setSize(tilefile_tile_W, tilefile_tile_H);
}

void NetHackQtGlyphs::drawGlyph(QPainter& painter, int glyph, int x, int y)
{
    int tile = glyph2tile[glyph];
    int px = (tile%tiles_per_row)*width();
    int py = tile/tiles_per_row*height();

    painter.drawPixmap(
	x,
	y,
	pm,
	px,py,
	width(),height()
    );
}
void NetHackQtGlyphs::drawCell(QPainter& painter, int glyph, int cellx, int celly)
{
    drawGlyph(painter,glyph,cellx*width(),celly*height());
}
void NetHackQtGlyphs::setSize(int w, int h)
{
    if ( size == QSize(w,h) )
	return;

    bool was1 = size == pm1.size();
    size = QSize(w,h);
    if (!w || !h)
	return; // Still not decided

    if ( size == pm1.size() ) {
	pm = pm1;
	return;
    }
    if ( size == pm2.size() ) {
	pm = pm2;
	return;
    }

    if (w==tilefile_tile_W && h==tilefile_tile_H) {
	pm.convertFromImage(img);
    } else {
	QApplication::setOverrideCursor( Qt::waitCursor );
	QImage scaled = img.smoothScale(
	    w*img.width()/tilefile_tile_W,
	    h*img.height()/tilefile_tile_H
	);
	pm.convertFromImage(scaled,Qt::ThresholdDither|Qt::PreferDither);
	QApplication::restoreOverrideCursor();
    }
    (was1 ? pm2 : pm1) = pm;
}


//////////////////////////////////////////////////////////////
//
//  The ugly C binding classes...
//
//////////////////////////////////////////////////////////////


NetHackQtMenuOrTextWindow::NetHackQtMenuOrTextWindow(NetHackQtKeyBuffer& ks) :
    actual(0),
    keysource(ks)
{
}

QWidget* NetHackQtMenuOrTextWindow::Widget()
{
    if (!actual) impossible("Widget called before we know if Menu or Text");
    return actual->Widget();
}

// Text
void NetHackQtMenuOrTextWindow::Clear()
{
    if (!actual)
        impossible("Clear called before we know if Menu or Text");
    else
        actual->Clear();
}

void NetHackQtMenuOrTextWindow::Display(bool block)
{
    if (!actual)
        impossible("Display called before we know if Menu or Text");
    else
        actual->Display(block);
}

bool NetHackQtMenuOrTextWindow::Destroy()
{
    bool res = FALSE;

    if (!actual)
        impossible("Destroy called before we know if Menu or Text");
    else
        res = actual->Destroy();

    return res;
}

void NetHackQtMenuOrTextWindow::PutStr(int attr, const char* text)
{
    if (!actual) actual=new NetHackQtTextWindow(keysource);
    actual->PutStr(attr,text);
}

// Menu
void NetHackQtMenuOrTextWindow::StartMenu()
{
    if (!actual) actual=new NetHackQtMenuWindow(keysource);
    actual->StartMenu();
}

void NetHackQtMenuOrTextWindow::AddMenu(int glyph, const ANY_P* identifier, char ch, char gch, int attr,
	const char* str, bool presel)
{
    if (!actual) impossible("AddMenu called before we know if Menu or Text");
    actual->AddMenu(glyph,identifier,ch,gch,attr,str,presel);
}

void NetHackQtMenuOrTextWindow::EndMenu(const char* prompt)
{
    if (!actual) impossible("EndMenu called before we know if Menu or Text");
    actual->EndMenu(prompt);
}

int NetHackQtMenuOrTextWindow::SelectMenu(int how, MENU_ITEM_P **menu_list)
{
    if (!actual) impossible("SelectMenu called before we know if Menu or Text");
    return actual->SelectMenu(how,menu_list);
}


// XXX Should be from Options
//
// XXX Hmm.  Tricky part is that perhaps some macros should only be active
// XXX       when a key is about to be gotten.  For example, the user could
// XXX       define "-" to do "E-yyyyyyyy\r", but would still need "-" for
// XXX       other purposes.  Maybe just too bad.
//
struct {
    int key;
    int state;
    const char* macro;
} key_macro[]={
    { Qt::Key_F1, 0, "n100." }, // Rest (x100)
    { Qt::Key_F2, 0, "n20s" },  // Search (x20)
    { Qt::Key_F3, 0, "o8o4o6o2o8o4o6o2o8o4o6o2" }, // Open all doors (x3)
    { Qt::Key_Tab, 0, "\001" },
    { 0, 0, 0 }
};


NetHackQtBind::NetHackQtBind(int& argc, char** argv) :
#ifdef KDE
    KApplication(argc,argv)
#elif defined(QWS) // not quite the right condition
    QPEApplication(argc,argv)
#else
    QApplication(argc,argv)
#endif
{
    QPixmap pm("nhsplash.xpm");
    if ( iflags.wc_splash_screen && !pm.isNull() ) {
	QVBox *vb = new QVBox(0,0,
	    WStyle_Customize | WStyle_NoBorder | nh_WX11BypassWM | WStyle_StaysOnTop );
	splash = vb;
	QLabel *lsplash = new QLabel(vb);
	lsplash->setAlignment(AlignCenter);
	lsplash->setPixmap(pm);
	QLabel* capt = new QLabel("Loading...",vb);
	capt->setAlignment(AlignCenter);
	if ( pm.mask() ) {
	    lsplash->setFixedSize(pm.size());
	    lsplash->setMask(*pm.mask());
	}
	splash->move((QApplication::desktop()->width()-pm.width())/2,
		      (QApplication::desktop()->height()-pm.height())/2);
	//splash->setGeometry(0,0,100,100);
	if ( qt_compact_mode ) {
	    splash->showMaximized();
	} else {
	    vb->setFrameStyle(QFrame::WinPanel|QFrame::Raised);
	    vb->setMargin(10);
	    splash->adjustSize();
	    splash->show();
	}

	// force content refresh outside event loop
	splash->repaint(FALSE);
	lsplash->repaint(FALSE);
	capt->repaint(FALSE);
	qApp->flushX();

    } else {
	splash = 0;
    }
    main = new NetHackQtMainWindow(keybuffer);
#if defined(QWS) // not quite the right condition
    showMainWidget(main);
#else
    setMainWidget(main);
#endif
    qt_settings=new NetHackQtSettings(main->width(),main->height());
}

void NetHackQtBind::qt_init_nhwindows(int* argc, char** argv)
{
#ifdef UNIX
// Userid control
//
// Michael Hohmuth <hohmuth@inf.tu-dresden.de>...
//
// As the game runs setuid games, it must seteuid(getuid()) before
// calling XOpenDisplay(), and reset the euid afterwards.
// Otherwise, it can't read the $HOME/.Xauthority file and whines about
// not being able to open the X display (if a magic-cookie
// authorization mechanism is being used). 

    uid_t gamesuid=geteuid();
    seteuid(getuid());
#endif

    QApplication::setColorSpec(ManyColor);
    instance=new NetHackQtBind(*argc,argv);

#ifdef UNIX
    seteuid(gamesuid);
#endif

#ifdef _WS_WIN_
    // This nethack engine feature should be moved into windowport API
    nt_kbhit = NetHackQtBind::qt_kbhit;
#endif
}

int NetHackQtBind::qt_kbhit()
{
    return !keybuffer.Empty();
}

static bool have_asked = FALSE;

void NetHackQtBind::qt_player_selection()
{
    if ( !have_asked )
	qt_askname();
}

NetHackQtSavedGameSelector::NetHackQtSavedGameSelector(const char** saved) :
    QDialog(qApp->mainWidget(),"sgsel",TRUE)
{
    QVBoxLayout *vbl = new QVBoxLayout(this,6);
    QHBox* hb;

    QLabel* logo = new QLabel(this); vbl->addWidget(logo);
    logo->setAlignment(AlignCenter);
    logo->setPixmap(QPixmap("nhsplash.xpm"));
    QLabel* attr = new QLabel("by the NetHack DevTeam",this);
    attr->setAlignment(AlignCenter);
    vbl->addWidget(attr);
    vbl->addStretch(2);
    /*
    QLabel* logo = new QLabel(hb);
    hb = new QHBox(this);
    vbl->addWidget(hb, AlignCenter);
    logo->setPixmap(QPixmap(nh_icon));
    logo->setAlignment(AlignRight|AlignVCenter);
    new QLabel(nh_attribution,hb);
    */

    hb = new QHBox(this);
    vbl->addWidget(hb, AlignCenter);
    QPushButton* q = new QPushButton("Quit",hb);
    connect(q, SIGNAL(clicked()), this, SLOT(reject()));
    QPushButton* c = new QPushButton("New Game",hb);
    connect(c, SIGNAL(clicked()), this, SLOT(accept()));
    c->setDefault(TRUE);

    QButtonGroup* bg = new QButtonGroup(3, Horizontal, "Saved Characters",this);
    vbl->addWidget(bg);
    connect(bg, SIGNAL(clicked(int)), this, SLOT(done(int)));
    for (int i=0; saved[i]; i++) {
	QPushButton* b = new QPushButton(saved[i],bg);
	bg->insert(b, i+2);
    }
}

int NetHackQtSavedGameSelector::choose()
{
#if defined(QWS) // probably safe with Qt 3, too (where show!=exec in QDialog).
    if ( qt_compact_mode )
	showMaximized();
#endif
    return exec()-2;
}

void NetHackQtBind::qt_askname()
{
    have_asked = TRUE;

    // We do it all here, and nothing in askname

    char** saved = get_saved_games();
    int ch = -1;
    if ( saved && *saved ) {
	if ( splash ) splash->hide();
	NetHackQtSavedGameSelector sgsel((const char**)saved);
	ch = sgsel.choose();
	if ( ch >= 0 )
	    strcpy(g.plname,saved[ch]);
    }
    free_saved_games(saved);

    switch (ch) {
      case -1:
	if ( splash ) splash->hide();
	if (NetHackQtPlayerSelector(keybuffer).Choose())
	    return;
      case -2:
	break;
      default:
	return;
    }

    // Quit
    clearlocks();
    qt_exit_nhwindows(0);
    nh_terminate(0);
}

void NetHackQtBind::qt_get_nh_event()
{
}

#if defined(QWS)
// Kludge to access lastWindowClosed() signal.
class TApp : public QApplication {
public:
    TApp(int& c, char**v) : QApplication(c,v) {}
    void lwc() { emit lastWindowClosed(); }
};
#endif
 
void NetHackQtBind::qt_exit_nhwindows(const char *)
{
#if defined(QWS)
    // Avoids bug in SHARP SL5500
    ((TApp*)qApp)->lwc();
    qApp->quit();
#endif
 
    delete instance; // ie. qApp
}

void NetHackQtBind::qt_suspend_nhwindows(const char *)
{
}

void NetHackQtBind::qt_resume_nhwindows()
{
}

static QArray<NetHackQtWindow*> id_to_window;

winid NetHackQtBind::qt_create_nhwindow(int type)
{
    winid id;
    for (id = 0; id < (winid) id_to_window.size(); id++) {
	if ( !id_to_window[id] )
	    break;
    }
    if ( id == (winid) id_to_window.size() )
	id_to_window.resize(id+1);

    NetHackQtWindow* window=0;

    switch (type) {
     case NHW_MAP: {
	NetHackQtMapWindow* w=new NetHackQtMapWindow(clickbuffer);
	main->AddMapWindow(w);
	window=w;
    } break; case NHW_MESSAGE: {
	NetHackQtMessageWindow* w=new NetHackQtMessageWindow;
	main->AddMessageWindow(w);
	window=w;
    } break; case NHW_STATUS: {
	NetHackQtStatusWindow* w=new NetHackQtStatusWindow;
	main->AddStatusWindow(w);
	window=w;
    } break; case NHW_MENU:
	window=new NetHackQtMenuOrTextWindow(keybuffer);
    break; case NHW_TEXT:
	window=new NetHackQtTextWindow(keybuffer);
    }

    window->nhid = id;

    // Note: use of isHidden does not work with Qt 2.1
    if ( splash 
#if QT_VERSION >= 300
        && !main->isHidden()
#else
	&& main->isVisible()
#endif
	)
    {
	delete splash;
	splash = 0;
    }

    id_to_window[id] = window;
    return id;
}

void NetHackQtBind::qt_clear_nhwindow(winid wid)
{
    NetHackQtWindow* window=id_to_window[wid];
    window->Clear();
}

void NetHackQtBind::qt_display_nhwindow(winid wid, BOOLEAN_P block)
{
    NetHackQtWindow* window=id_to_window[wid];
    window->Display(block);
}

void NetHackQtBind::qt_destroy_nhwindow(winid wid)
{
    NetHackQtWindow* window=id_to_window[wid];
    main->RemoveWindow(window);
    if (window->Destroy())
	delete window;
    id_to_window[wid] = 0;
}

void NetHackQtBind::qt_curs(winid wid, int x, int y)
{
    NetHackQtWindow* window=id_to_window[wid];
    window->CursorTo(x,y);
}

void NetHackQtBind::qt_putstr(winid wid, int attr, const char *text)
{
    NetHackQtWindow* window=id_to_window[wid];
    window->PutStr(attr,text);
}

void NetHackQtBind::qt_display_file(const char *filename, BOOLEAN_P must_exist)
{
    NetHackQtTextWindow* window=new NetHackQtTextWindow(keybuffer);
    bool complain = FALSE;

#ifdef DLB
    {
	dlb *f;
	char buf[BUFSZ];
	char *cr;

	window->Clear();
	f = dlb_fopen(filename, "r");
	if (!f) {
	    complain = must_exist;
	} else {
	    while (dlb_fgets(buf, BUFSZ, f)) {
		if ((cr = strchr(buf, '\n')) != 0) *cr = 0;
#ifdef MSDOS
		if ((cr = strchr(buf, '\r')) != 0) *cr = 0;
#endif
		if (strchr(buf, '\t') != 0) (void) tabexpand(buf);
		window->PutStr(ATR_NONE, buf);
	    }
	    window->Display(FALSE);
	    (void) dlb_fclose(f);
	}
    }
#else
    QFile file(filename);

    if (file.open(IO_ReadOnly)) {
	char line[128];
	while (file.readLine(line,127) >= 0) {
	    line[strlen(line)-1]=0;// remove newline
	    window->PutStr(ATR_NONE,line);
	}
	window->Display(FALSE);
    } else {
	complain = must_exist;
    }
#endif

    if (complain) {
	QString message;
	message.sprintf("File not found: %s\n",filename);
	QMessageBox::message("File Error", (const char*)message, "Ignore");
    }
}

void NetHackQtBind::qt_start_menu(winid wid, unsigned long mbehavior)
{
    NetHackQtWindow* window=id_to_window[wid];
    window->StartMenu();
}

void NetHackQtBind::qt_add_menu(winid wid, int glyph,
    const ANY_P * identifier, CHAR_P ch, CHAR_P gch, int attr,
    const char *str, unsigned int itemflags)
{
    boolean presel = ((itemflags & MENU_ITEMFLAGS_SELECTED) != 0);
    NetHackQtWindow* window=id_to_window[wid];
    window->AddMenu(glyph, identifier, ch, gch, attr, str, presel);
}

void NetHackQtBind::qt_end_menu(winid wid, const char *prompt)
{
    NetHackQtWindow* window=id_to_window[wid];
    window->EndMenu(prompt);
}

int NetHackQtBind::qt_select_menu(winid wid, int how, MENU_ITEM_P **menu_list)
{
    NetHackQtWindow* window=id_to_window[wid];
    return window->SelectMenu(how,menu_list);
}

void NetHackQtBind::qt_update_inventory()
{
    if (main)
	main->updateInventory();
    /* doesn't work yet
    if (g.program_state.something_worth_saving && iflags.perm_invent)
        display_inventory(NULL, FALSE);
    */
}

void NetHackQtBind::qt_mark_synch()
{
}

void NetHackQtBind::qt_wait_synch()
{
}

void NetHackQtBind::qt_cliparound(int x, int y)
{
    // XXXNH - winid should be a parameter!
    qt_cliparound_window(WIN_MAP,x,y);
}

void NetHackQtBind::qt_cliparound_window(winid wid, int x, int y)
{
    NetHackQtWindow* window=id_to_window[wid];
    window->ClipAround(x,y);
}
void NetHackQtBind::qt_print_glyph(winid wid,XCHAR_P x,XCHAR_P y,int glyph, int bkglyph)
{
    NetHackQtWindow* window=id_to_window[wid];
    window->PrintGlyph(x,y,glyph);
}
//void NetHackQtBind::qt_print_glyph_compose(winid wid,XCHAR_P x,XCHAR_P y,int glyph1, int glyph2)
//{
    //NetHackQtWindow* window=id_to_window[wid];
    //window->PrintGlyphCompose(x,y,glyph1,glyph2);
//}

void NetHackQtBind::qt_raw_print(const char *str)
{
    puts(str);
}

void NetHackQtBind::qt_raw_print_bold(const char *str)
{
    puts(str);
}

int NetHackQtBind::qt_nhgetch()
{
    if (main)
	main->fadeHighlighting();

    // Process events until a key arrives.
    //
    while (keybuffer.Empty()
#ifdef SAFERHANGUP
	   && !g.program_state.done_hup
#endif
	   ) {
	qApp->enter_loop();
    }

#ifdef SAFERHANGUP
    if (g.program_state.done_hup && keybuffer.Empty()) return '\033';
#endif
    return keybuffer.GetAscii();
}

int NetHackQtBind::qt_nh_poskey(int *x, int *y, int *mod)
{
    if (main)
	main->fadeHighlighting();

    // Process events until a key or map-click arrives.
    //
    while (keybuffer.Empty() && clickbuffer.Empty()
#ifdef SAFERHANGUP
	   && !g.program_state.done_hup
#endif
	   ) {
	qApp->enter_loop();
    }
#ifdef SAFERHANGUP
    if (g.program_state.done_hup && keybuffer.Empty()) return '\033';
#endif
    if (!keybuffer.Empty()) {
	return keybuffer.GetAscii();
    } else {
	*x=clickbuffer.NextX();
	*y=clickbuffer.NextY();
	*mod=clickbuffer.NextMod();
	clickbuffer.Get();
	return 0;
    }
}

void NetHackQtBind::qt_nhbell()
{
    QApplication::beep();
}

int NetHackQtBind::qt_doprev_message()
{
    // Don't need it - uses scrollbar
    // XXX but could make this a shortcut
    return 0;
}

char NetHackQtBind::qt_yn_function(const char *question, const char *choices, CHAR_P def)
{
    if (qt_settings->ynInMessages() && WIN_MESSAGE!=WIN_ERR) {
	// Similar to X11 windowport `slow' feature.

	char message[BUFSZ];
	char yn_esc_map='\033';

	if (choices) {
	    char *cb, choicebuf[QBUFSZ];
	    Strcpy(choicebuf, choices);
	    if ((cb = strchr(choicebuf, '\033')) != 0) {
		// anything beyond <esc> is hidden
		*cb = '\0';
	    }
	    (void)strncpy(message, question, QBUFSZ-1);
	    message[QBUFSZ-1] = '\0';
	    Sprintf(eos(message), " [%s]", choicebuf);
	    if (def) Sprintf(eos(message), " (%c)", def);
	    Strcat(message, " ");
	    // escape maps to 'q' or 'n' or default, in that order
	    yn_esc_map = (strchr(choices, 'q') ? 'q' :
		     (strchr(choices, 'n') ? 'n' : def));
	} else {
	    Strcpy(message, question);
	}

#ifdef USE_POPUPS
	// Improve some special-cases (DIRKS 08/02/23)
	if (strcmp (choices,"ynq") == 0) {
	    switch (QMessageBox::information (qApp->mainWidget(),"NetHack",question,"&Yes","&No","&Quit",0,2))
	    {
	      case 0: return 'y'; 
	      case 1: return 'n'; 
	      case 2: return 'q'; 
	    }
	}

	if (strcmp (choices,"yn") == 0) {
	    switch (QMessageBox::information(qApp->mainWidget(),"NetHack",question,"&Yes", "&No",0,1))
	    {
	      case 0: return 'y';
	      case 1: return 'n'; 
	    }
	}
#endif

	NetHackQtBind::qt_putstr(WIN_MESSAGE, ATR_BOLD, message);

	int result=-1;
	while (result<0) {
	    char ch=NetHackQtBind::qt_nhgetch();
	    if (ch=='\033') {
		result=yn_esc_map;
	    } else if (choices && !strchr(choices,ch)) {
		if (def && (ch==' ' || ch=='\r' || ch=='\n')) {
		    result=def;
		} else {
		    NetHackQtBind::qt_nhbell();
		    // and try again...
		}
	    } else {
		result=ch;
	    }
	}

	NetHackQtBind::qt_clear_nhwindow(WIN_MESSAGE);

	return result;
    } else {
	NetHackQtYnDialog dialog(keybuffer,question,choices,def);
	return dialog.Exec();
    }
}

void NetHackQtBind::qt_getlin(const char *prompt, char *line)
{
    NetHackQtStringRequestor requestor(keybuffer,prompt);
    if (!requestor.Get(line)) {
	line[0]=0;
    }
}

NetHackQtExtCmdRequestor::NetHackQtExtCmdRequestor(NetHackQtKeyBuffer& ks) :
    QDialog(qApp->mainWidget(), "ext-cmd", FALSE),
    keysource(ks)
{
    int marg=4;
    QVBoxLayout *l = new QVBoxLayout(this,marg,marg);

    QPushButton* can = new QPushButton("Cancel", this);
    can->setDefault(TRUE);
    can->setMinimumSize(can->sizeHint());
    l->addWidget(can);

    QButtonGroup *group=new QButtonGroup("",0);
    QGroupBox *grid=new QGroupBox("Extended commands",this);
    l->addWidget(grid);

    int i;
    int butw=50;
    QFontMetrics fm = fontMetrics();
    for (i=0; extcmdlist[i].ef_txt; i++) {
	butw = QMAX(butw,30+fm.width(extcmdlist[i].ef_txt));
    }
    int ncols=4;
    int nrows=(i+ncols-1)/ncols;

    QVBoxLayout* bl = new QVBoxLayout(grid,marg);
    bl->addSpacing(fm.height());
    QGridLayout* gl = new QGridLayout(nrows,ncols,marg);
    bl->addLayout(gl);
    for (i=0; extcmdlist[i].ef_txt; i++) {
	QPushButton* pb=new QPushButton(extcmdlist[i].ef_txt, grid);
	pb->setMinimumSize(butw,pb->sizeHint().height());
	group->insert(pb);
	gl->addWidget(pb,i/ncols,i%ncols);
    }
    connect(group,SIGNAL(clicked(int)),this,SLOT(done(int)));

    bl->activate();
    l->activate();
    resize(1,1);

    connect(can,SIGNAL(clicked()),this,SLOT(cancel()));
}

void NetHackQtExtCmdRequestor::cancel()
{
    setResult(-1);
    qApp->exit_loop();
}

void NetHackQtExtCmdRequestor::done(int i)
{
    setResult(i);
    qApp->exit_loop();
}

int NetHackQtExtCmdRequestor::get()
{
    const int none = -10;
    char str[32];
    int cursor=0;
    resize(1,1); // pack
    centerOnMain(this);
    show();
    setResult(none);
    while (result()==none) {
	while (result()==none && !keysource.Empty()) {
	    char k=keysource.GetAscii();
	    if (k=='\r' || k=='\n' || k==' ' || k=='\033') {
		setResult(-1);
	    } else {
		str[cursor++] = k;
		int r=-1;
		for (int i=0; extcmdlist[i].ef_txt; i++) {
		    if (qstrnicmp(str, extcmdlist[i].ef_txt, cursor)==0) {
			if ( r == -1 )
			    r = i;
			else
			    r = -2;
		    }
		}
		if ( r == -1 ) { // no match!
		    QApplication::beep();
		    cursor=0;
		} else if ( r != -2 ) { // only one match
		    setResult(r);
		}
	    }
	}
	if (result()==none)
	    qApp->enter_loop();
    }
    hide();
    return result();
}


int NetHackQtBind::qt_get_ext_cmd()
{
    NetHackQtExtCmdRequestor requestor(keybuffer);
    return requestor.get();
}

void NetHackQtBind::qt_number_pad(int)
{
    // Ignore.
}

void NetHackQtBind::qt_delay_output()
{
    NetHackQtDelay delay(15);
    delay.wait();
}

void NetHackQtBind::qt_start_screen()
{
    // Ignore.
}

void NetHackQtBind::qt_end_screen()
{
    // Ignore.
}

void NetHackQtBind::qt_outrip(winid wid, int how, time_t when)
{
    NetHackQtWindow* window=id_to_window[wid];

    window->UseRIP(how, when);
}

bool NetHackQtBind::notify(QObject *receiver, QEvent *event)
{
    // Ignore Alt-key navigation to menubar, it's annoying when you
    // use Alt-Direction to move around.
    if ( main && event->type()==QEvent::KeyRelease && main==receiver
	    && ((QKeyEvent*)event)->key() == Key_Alt )
	return TRUE;

    bool result=QApplication::notify(receiver,event);
#ifdef SAFERHANGUP
    if (g.program_state.done_hup) {
	keybuffer.Put('\033');
	qApp->exit_loop();
	return TRUE;
    }
#endif
    if (event->type()==QEvent::KeyPress) {
	QKeyEvent* key_event=(QKeyEvent*)event;

	if (!key_event->isAccepted()) {
	    const int k=key_event->key();
	    bool macro=FALSE;
	    for (int i=0; !macro && key_macro[i].key; i++) {
		if (key_macro[i].key==k
		 && ((key_macro[i].state&key_event->state())==key_macro[i].state))
		{
		    keybuffer.Put(key_macro[i].macro);
		    macro=TRUE;
		}
	    }
	    char ch=key_event->ascii();
	    if ( !ch && (key_event->state() & Qt::ControlButton) ) {
		// On Mac, it aint-ncessarily-control
		if ( k>=Qt::Key_A && k<=Qt::Key_Z )
		    ch = k - Qt::Key_A + 1;
	    }
	    if (!macro && ch) {
		bool alt = (key_event->state()&AltButton) ||
		   (k >= Key_0 && k <= Key_9 && (key_event->state()&ControlButton));
		keybuffer.Put(key_event->key(),ch + (alt ? 128 : 0),
		    key_event->state());
		key_event->accept();
		result=TRUE;
	    }

	    if (ch || macro) {
		qApp->exit_loop();
	    }
	}
    }
    return result;
}

NetHackQtBind* NetHackQtBind::instance=0;
NetHackQtKeyBuffer NetHackQtBind::keybuffer;
NetHackQtClickBuffer NetHackQtBind::clickbuffer;
NetHackQtMainWindow* NetHackQtBind::main=0;
QWidget* NetHackQtBind::splash=0;


extern "C" struct window_procs Qt_procs;

struct window_procs Qt_procs = {
    "Qt",
    WC_COLOR|WC_HILITE_PET|
	WC_ASCII_MAP|WC_TILED_MAP|
	WC_FONT_MAP|WC_TILE_FILE|WC_TILE_WIDTH|WC_TILE_HEIGHT|
	WC_PLAYER_SELECTION|WC_SPLASH_SCREEN,
    0L,
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},   /* color availability */
    NetHackQtBind::qt_init_nhwindows,
    NetHackQtBind::qt_player_selection,
    NetHackQtBind::qt_askname,
    NetHackQtBind::qt_get_nh_event,
    NetHackQtBind::qt_exit_nhwindows,
    NetHackQtBind::qt_suspend_nhwindows,
    NetHackQtBind::qt_resume_nhwindows,
    NetHackQtBind::qt_create_nhwindow,
    NetHackQtBind::qt_clear_nhwindow,
    NetHackQtBind::qt_display_nhwindow,
    NetHackQtBind::qt_destroy_nhwindow,
    NetHackQtBind::qt_curs,
    NetHackQtBind::qt_putstr,
    genl_putmixed,
    NetHackQtBind::qt_display_file,
    NetHackQtBind::qt_start_menu,
    NetHackQtBind::qt_add_menu,
    NetHackQtBind::qt_end_menu,
    NetHackQtBind::qt_select_menu,
    genl_message_menu,      /* no need for X-specific handling */
    NetHackQtBind::qt_update_inventory,
    NetHackQtBind::qt_mark_synch,
    NetHackQtBind::qt_wait_synch,
#ifdef CLIPPING
    NetHackQtBind::qt_cliparound,
#endif
#ifdef POSITIONBAR
    donull,
#endif
    NetHackQtBind::qt_print_glyph,
    //NetHackQtBind::qt_print_glyph_compose,
    NetHackQtBind::qt_raw_print,
    NetHackQtBind::qt_raw_print_bold,
    NetHackQtBind::qt_nhgetch,
    NetHackQtBind::qt_nh_poskey,
    NetHackQtBind::qt_nhbell,
    NetHackQtBind::qt_doprev_message,
    NetHackQtBind::qt_yn_function,
    NetHackQtBind::qt_getlin,
    NetHackQtBind::qt_get_ext_cmd,
    NetHackQtBind::qt_number_pad,
    NetHackQtBind::qt_delay_output,
#ifdef CHANGE_COLOR     /* only a Mac option currently */
    donull,
    donull,
#endif
    /* other defs that really should go away (they're tty specific) */
    NetHackQtBind::qt_start_screen,
    NetHackQtBind::qt_end_screen,
#ifdef GRAPHIC_TOMBSTONE
    NetHackQtBind::qt_outrip,
#else
    genl_outrip,
#endif
    genl_preference_update,
    genl_getmsghistory,
    genl_putmsghistory,
    genl_status_init,
    genl_status_finish,
    genl_status_enablefield,
    genl_status_update,
    genl_can_suspend_yes,
};

extern "C" void play_usersound(const char* filename, int volume)
{
#ifdef USER_SOUNDS
#ifndef QT_NO_SOUND
    QSound::play(filename);
#endif
#endif
}

#include "qt3_win.moc"
#ifndef KDE
#include "qt3_kde0.moc"
#endif
#if QT_VERSION >= 300
#include "qt3tableview.moc"
#endif
