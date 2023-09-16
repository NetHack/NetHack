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
    list(new QListWidget()),
    scrollarea(new QScrollArea())
{
    list->setFocusPolicy(Qt::NoFocus);
    scrollarea->setFocusPolicy(Qt::NoFocus);
    scrollarea->takeWidget();
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

QWidget* NetHackQtMessageWindow::Widget() {
    return list;
}

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

void NetHackQtMessageWindow::Display(bool block)
{
    if (changed) {
	list->repaint();
	changed=false;
    }
    if (block) {
        // we don't care what the response is here
        (void) NetHackQtBind::qt_more();
    }
}

const char * NetHackQtMessageWindow::GetStr(bool init)
{
    if (init)
        currgetmsg = 0;

    QListWidgetItem *item = list->item(currgetmsg++);
    if (item) {
        QString str = item->text();
	if (str.toLatin1().length() < (int) sizeof historybuf) {
            return strcpy(historybuf, str.toLatin1().constData());
            //raw_printf("getstr[%d]='%s'", currgetmsg, result);
	}
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
    if (attr != ATR_NONE) {
        QListWidgetItem *item = new QListWidgetItem(text2);
        if (attr != ATR_DIM && attr != ATR_INVERSE) {
            QFont font = item->font();
            font.setUnderline(attr == ATR_ULINE);
            font.setWeight((attr == ATR_BOLD) ? QFont::Bold : QFont::Normal);
            item->setFont(font);
            // ATR_BLINK not supported
        } else {
            // ATR_DIM or ATR_INVERSE
            QBrush fg = item->foreground();
            QBrush bg = item->background();
            if (fg.color() == bg.color()) { // from menu coloring [AddRow()]
                // default foreground and background come up the same for
                // some unknown reason
                //[pr: both are set to 'Qt::color1' which has same RGB
                //     value as 'Qt::black'; X11 on OSX behaves similarly]
                if (fg.color() == Qt::color1) {
                    fg = Qt::black;
                    bg = Qt::white;
                } else {
                    fg = (bg.color() == Qt::white) ? Qt::black : Qt::white;
                }
            }
            if (attr == ATR_DIM) {
                QColor fg_clr = fg.color();
                fg_clr.setAlpha(fg_clr.alpha() / 2);
                item->setFlags(Qt::NoItemFlags);
            } else if (attr == ATR_INVERSE) {
                QBrush swapfb;
                swapfb = fg; fg = bg; bg = swapfb;
            }
            item->setForeground(fg);
            item->setBackground(bg);
        }
    }
#else
    nhUse(attr);
#endif

    if (list->count() >= (int) ::iflags.msg_history)
	delete list->item(0);
    list->addItem(text2);
    /* assert( list->count() > 0 ); */

    // force scrollbar to bottom;
    // selects most recent message, which causes it to be highlighted
    list->setCurrentRow(list->count() - 1);

    // if message window has been scrolled right, force back to left edge
    QScrollBar *sb = list->horizontalScrollBar();
    if (sb && sb->value() > 0) {
        sb->setValue(0);
        this->viewport()->update();
    }

    if (map)
	map->putMessage(attr, text2);
}

// append to the last message; usually the user's answer to a prompt
void NetHackQtMessageWindow::AddToStr(const char *answer)
{
    if (list) {
        QListWidgetItem *item = list->currentItem();
        int ct = 0;
        if (!item && (ct = list->count()) > 0) {
            list->setCurrentRow(ct - 1);
            item = list->currentItem();
        }
        if (item)
            item->setText(item->text() + QString(" %1").arg(answer));
        else // just in case...
            NetHackQtMessageWindow::PutStr(ATR_NONE, answer);
    }
}

// used when yn_function() or more() rejects player's input and tries again
void NetHackQtMessageWindow::RehighlightPrompt()
{
    // selects most recent message, which causes it to be highlighted
    if (list && list->count())
        list->setCurrentRow(list->count() - 1);
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
