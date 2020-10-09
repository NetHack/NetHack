// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_line.cpp -- a one line input window

extern "C" {
#include "hack.h"
}

#include "qt_pre.h"
#include <QtGui/QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QtWidgets>
#endif
#include "qt_post.h"
#include "qt_line.h"

namespace nethack_qt_ {

NetHackQtLineEdit::NetHackQtLineEdit() :
    QLineEdit(0)
{
}

NetHackQtLineEdit::NetHackQtLineEdit(QWidget* parent, const char* name UNUSED) :
    QLineEdit(parent)
{
}

void NetHackQtLineEdit::fakeEvent(int key, int ascii, Qt::KeyboardModifiers state)
{
    QKeyEvent fake(QEvent::KeyPress,key,state,QChar(ascii));
    keyPressEvent(&fake);
}

} // namespace nethack_qt_
