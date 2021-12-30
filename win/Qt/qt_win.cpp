// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// Qt Windowing interface for NetHack 3.7
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
// key events by overiding QApplication::notify(...), and putting them in
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
// identifiers (now done in qt_pre.h).

#define QT_DEPRECATED_WARNINGS
extern "C" {
#include "hack.h"
}

#include "qt_pre.h"
#include <QtGui/QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QtWidgets>
#endif
#include "qt_post.h"

// Many of these headers are not needed here.  It's a holdover
// from when most of the Qt code was in one big file.
#include "qt_win.h"
#include "qt_bind.h"
#include "qt_click.h"
#include "qt_glyph.h"
#include "qt_inv.h"
#include "qt_key.h"
#include "qt_icon.h"
#include "qt_map.h"
#include "qt_menu.h"
#include "qt_msg.h"
#include "qt_set.h"

#include "qt_clust.h"

namespace nethack_qt_ {

void
centerOnMain(QWidget *w)
{
    QPoint p(0, 0);
    QWidget *m = NetHackQtBind::mainWidget();
    if (m)
        p = m->mapToGlobal(p);
    w->move(p.x() + m->width() / 2  - w->width() / 2,
            p.y() + m->height() / 2 - w->height() / 2);
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
void NetHackQtWindow::Display(bool block UNUSED) { puts("unexpected Display"); }
bool NetHackQtWindow::Destroy() { return true; }
void NetHackQtWindow::CursorTo(int x UNUSED,int y UNUSED) { puts("unexpected CursorTo"); }
void NetHackQtWindow::PutStr(int attr UNUSED, const QString& text UNUSED) { puts("unexpected PutStr"); }
void NetHackQtWindow::StartMenu(bool using_WIN_INVEN UNUSED)
                                { puts("unexpected StartMenu"); }
void NetHackQtWindow::AddMenu(int glyph UNUSED, const ANY_P* identifier UNUSED,
                              char ch UNUSED, char gch UNUSED, int attr UNUSED,
                              const QString& str UNUSED, unsigned itemflags UNUSED)
                              { puts("unexpected AddMenu"); }
void NetHackQtWindow::EndMenu(const QString& prompt UNUSED) { puts("unexpected EndMenu"); }
int NetHackQtWindow::SelectMenu(int how UNUSED, MENU_ITEM_P **menu_list UNUSED)
                               { puts("unexpected SelectMenu"); return 0; }
void NetHackQtWindow::ClipAround(int x UNUSED,int y UNUSED) { puts("unexpected ClipAround"); }
void NetHackQtWindow::PrintGlyph(int x UNUSED,int y UNUSED,const glyph_info *glyphinfo UNUSED) { puts("unexpected PrintGlyph"); }
//void NetHackQtWindow::PrintGlyphCompose(int x,int y,int,int) { puts("unexpected PrintGlyphCompose"); }
void NetHackQtWindow::UseRIP(int how UNUSED, time_t when UNUSED) { puts("unexpected UseRIP"); }

} // namespace nethack_qt_
