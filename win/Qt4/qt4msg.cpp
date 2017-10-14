// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt4msg.cpp -- a message window

extern "C" {
#include "hack.h"
}
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
#include "qt4msg.h"
#include "qt4msg.moc"
#include "qt4map.h"
#include "qt4set.h"
#include "qt4str.h"

namespace nethack_qt4 {

NetHackQtMessageWindow::NetHackQtMessageWindow() :
    list(new QListWidget())
{
    list->setFocusPolicy(Qt::NoFocus);
    ::iflags.window_inited = 1;
    map = 0;
    currgetmsg = 0;
    connect(qt_settings,SIGNAL(fontChanged()),this,SLOT(updateFont()));
    updateFont();
}

NetHackQtMessageWindow::~NetHackQtMessageWindow()
{
    ::iflags.window_inited = 0;
    delete list;
}

QWidget* NetHackQtMessageWindow::Widget() { return list; }

void NetHackQtMessageWindow::setMap(NetHackQtMapWindow2* m)
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
    //RLC Is this necessary?
    //RLC list->Scroll(dx,dy);
}

void NetHackQtMessageWindow::Clear()
{
    if ( map )
	map->clearMessages();
}

void NetHackQtMessageWindow::ClearMessages()
{
    if (list)
        list->clear();
}

void NetHackQtMessageWindow::Display(bool block)
{
    if (changed) {
	list->repaint();
	changed=false;
    }
}

const char * NetHackQtMessageWindow::GetStr(bool init)
{
    if (init)
        currgetmsg = 0;

    QListWidgetItem *item = list->item(currgetmsg++);
    if (item) {
        QString str = item->text();
        //raw_printf("getstr[%i]='%s'", currgetmsg, str.toLatin1().constData());
        return str.toLatin1().constData();
    }
    return NULL;
}

void NetHackQtMessageWindow::PutStr(int attr, const QString& text)
{
#ifdef USER_SOUNDS
    play_sound_for_message(text.toLatin1().constData());
#endif

    changed=true;

    // If the line is output from the "/" command, map the first character
    // as a symbol
    QString text2;
    if (text.mid(1, 3) == "   ") {
	text2 = QChar(cp437(text.at(0).unicode())) + text.mid(1);
    } else {
	text2 = text;
    }
#if 0
    QListWidgetItem *item = new QListWidgetItem(text2);

    QFont font = item->font();
    font.setUnderline(attr == ATR_ULINE);
    font.setWeight((attr == ATR_BOLD) ? QFont::Bold : QFont::Normal);
    item->setFont(font);

    QColor fg = item->foreground().color();
    QColor bg = item->background().color();
    if (attr == ATR_DIM)
    {
	fg.setAlpha(fg.alpha() / 2);
    }
    if (attr == ATR_INVERSE)
    {
	QColor swap;
	swap = fg; fg = bg; bg = swap;
    }
    item->setForeground(fg);
    item->setBackground(bg);
#endif

    // ATR_BLINK not supported
    if (list->count() >= ::iflags.msg_history)
	delete list->item(0);
    list->addItem(text2);

    // Force scrollbar to bottom
    list->setCurrentRow(list->count()-1);

    if ( map )
	map->putMessage(attr, text2);
}

} // namespace nethack_qt4
