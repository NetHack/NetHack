// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt4line.cpp -- a one line input window

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
#include "qt4line.h"

namespace nethack_qt4 {

NetHackQtLineEdit::NetHackQtLineEdit() :
    QLineEdit(0)
{
}

NetHackQtLineEdit::NetHackQtLineEdit(QWidget* parent, const char* name) :
    QLineEdit(parent)
{
}

void NetHackQtLineEdit::fakeEvent(int key, int ascii, Qt::KeyboardModifiers state)
{
    QKeyEvent fake(QEvent::KeyPress,key,state,QChar(ascii));
    keyPressEvent(&fake);
}

} // namespace nethack_qt4
