// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
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
// the buffer, or if that is empty, QApplication::exec() is called.
// Whenever keys or clicks go into the buffer, QApplication::exit()
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

// This includes all the definitions we need from the NetHack main
// engine.  We pretend MSC is a STDC compiler, because C++ is close
// enough, and we undefine NetHack macros which conflict with Qt
// identifiers.

#define QT_DEPRECATED_WARNINGS
#include "hack.h"
#undef Invisible
#undef Warning
#undef index
#undef msleep
#undef rindex
#undef wizard
#undef yn
#undef min
#undef max

#include <QtGui/QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QtWidgets>
#endif
#include "qt4win.h"
#include "qt4bind.h"
#include "qt4click.h"
#include "qt4glyph.h"
#include "qt4inv.h"
#include "qt4key.h"
#include "qt4icon.h"
#include "qt4map.h"
#include "qt4menu.h"
#include "qt4msg.h"
#include "qt4set.h"

#include <ctype.h>

#include "qt4clust.h"

#include <dirent.h>

#ifdef _WS_X11_
// For userid control
#include <unistd.h>
#endif

#ifdef USER_SOUNDS
#if QT_VERSION >= 0x050000
#  include <QtMultimedia/QSound>
# else
#  include <QtGui/QSound>
# endif
#endif


#ifdef USER_SOUNDS
extern void play_sound_for_message(const std::string& str);
#endif

namespace nethack_qt4 {

void
centerOnMain( QWidget* w )
{
    QWidget* m = NetHackQtBind::mainWidget();
    if (!m) m = qApp->desktop();
    QPoint p = m->mapToGlobal(QPoint(0,0));
    w->move( p.x() + m->width()/2  - w->width()/2,
              p.y() + m->height()/2 - w->height()/2 );
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
bool NetHackQtWindow::Destroy() { return true; }
void NetHackQtWindow::CursorTo(int x,int y) { puts("unexpected CursorTo"); }
void NetHackQtWindow::PutStr(int attr, const QString& text) { puts("unexpected PutStr"); }
void NetHackQtWindow::StartMenu() { puts("unexpected StartMenu"); }
void NetHackQtWindow::AddMenu(int glyph, const ANY_P* identifier, char ch, char gch, int attr,
    const QString& str, bool presel) { puts("unexpected AddMenu"); }
void NetHackQtWindow::EndMenu(const QString& prompt) { puts("unexpected EndMenu"); }
int NetHackQtWindow::SelectMenu(int how, MENU_ITEM_P **menu_list) { puts("unexpected SelectMenu"); return 0; }
void NetHackQtWindow::ClipAround(int x,int y) { puts("unexpected ClipAround"); }
void NetHackQtWindow::PrintGlyph(int x,int y,int glyph) { puts("unexpected PrintGlyph"); }
//void NetHackQtWindow::PrintGlyphCompose(int x,int y,int,int) { puts("unexpected PrintGlyphCompose"); }
void NetHackQtWindow::UseRIP(int how, time_t when) { puts("unexpected UseRIP"); }

} // namespace nethack_qt4
