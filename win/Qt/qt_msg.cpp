// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_msg.cpp -- a message window

extern "C" {
#include "hack.h"
}

#include "qt_pre.h"
#include <QtGui/QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QtWidgets>
#endif
#include "qt_post.h"
#include "qt_msg.h"
#include "qt_msg.moc"
#include "qt_map.h"
#include "qt_set.h"
#include "qt_str.h"

namespace nethack_qt_ {

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

void NetHackQtMessageWindow::Scroll(int dx UNUSED, int dy UNUSED)
{
    //RLC Is this necessary?
    //RLC list->Scroll(dx,dy);
}

void NetHackQtMessageWindow::Clear()
{
    if (list)
        NetHackQtMessageWindow::unhighlight_mesgs();

    if (map)
	map->clearMessages();
}

void NetHackQtMessageWindow::ClearMessages()
{
    if (list)
        list->clear();
}

void NetHackQtMessageWindow::Display(bool block UNUSED)
{
    //
    // FIXME: support for 'block' is necessary for MSGTYPE=stop
    //
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
        const char *result = str.toLatin1().constData();
        //raw_printf("getstr[%d]='%s'", currgetmsg, result);
        return result;
    }
    return NULL;
}

void NetHackQtMessageWindow::PutStr(int attr, const QString& text)
{
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

    if (attr == ATR_DIM || attr == ATR_INVERSE) {
        QColor fg = item->foreground().color();
        QColor bg = item->background().color();
        if (attr == ATR_DIM) {
            fg.setAlpha(fg.alpha() / 2);
            new_fgbg = true;
        }
        if (attr == ATR_INVERSE) {
            QColor swap;
            swap = fg; fg = bg; bg = swap;
        }
        item->setForeground(fg);
        item->setBackground(bg);
    }
    // ATR_BLINK not supported
#endif

    if (list->count() >= (int) ::iflags.msg_history)
	delete list->item(0);
    list->addItem(text2);

    // Force scrollbar to bottom
    list->setCurrentRow(list->count() - 1);

    if (map)
	map->putMessage(attr, text2);
}

// append the user's answer to a prompt message
void NetHackQtMessageWindow::AddToStr(const char *answer)
{
    if (list) {
        QListWidgetItem *item = list->currentItem();
        if (item)
            item->setText(item->text() + QString(" %1").arg(answer));
    }
}

// are there any highlighted messages?
bool NetHackQtMessageWindow::hilit_mesgs()
{
    // PutStr() uses setCurrentRow() to select the last message line;
    // being selected causes that line to be highlighted.
    //
    // We could/should keep track of whether anything is currently
    // highlighted instead of just assuming that last message still is.
    if (list && list->count())
        return true;
    return false;
}

// unhighlight any highlighted messages
void NetHackQtMessageWindow::unhighlight_mesgs()
{
    if (list)
        list->clearSelection();
}

} // namespace nethack_qt_
