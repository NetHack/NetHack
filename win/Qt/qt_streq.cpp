// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_streq.cpp -- string requestor

extern "C" {
#include "hack.h"
}

#include "qt_pre.h"
#include <QtGui/QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QtWidgets>
#endif
#include "qt_post.h"
#include "qt_streq.h"
#include "qt_str.h"
#include "qt_set.h"

namespace nethack_qt_ {

// temporary
void centerOnMain(QWidget *);
// end temporary

NetHackQtStringRequestor::NetHackQtStringRequestor(QWidget *parent,
        const char *p, const char *cancelstr, const char *okaystr) :
    QDialog(parent),
    prompt(QString::fromLatin1(p),this),
    input(this,"input")
{
    if (qt_settings)
        input.setFont(qt_settings->normalFixedFont());

    cancel=new QPushButton(cancelstr,this);
    connect(cancel,SIGNAL(clicked()),this,SLOT(reject()));

    okay = new QPushButton(okaystr, this);
    connect(okay,SIGNAL(clicked()),this,SLOT(accept()));
    connect(&input,SIGNAL(returnPressed()),this,SLOT(accept()));
    okay->setDefault(true);

    setFocusPolicy(Qt::StrongFocus);
}

void NetHackQtStringRequestor::resizeEvent(QResizeEvent*)
{
    const int margin=5;
    const int gutter=5;

    int h = (height() - margin * 2 - gutter);
    int w = (width() - margin * 2 - gutter);
    int ifw = input.hasFrame() ? 3 : 0; // hack alert for input.frameWidth()
    if (prompt.text().size() > 16) {
        h /= 3;
        prompt.setGeometry(margin + ifw * 2 + 1, margin, w + gutter, h);
        input.setGeometry(width() * 1 / 5 - ifw, margin + h + gutter,
                          w * 4 / 5, h);
    } else {
        h /= 2;
        prompt.setGeometry(margin + ifw * 2 + 1, margin, w * 2 / 5, h);
        input.setGeometry(prompt.geometry().right() + gutter
                           - (ifw * 2 + 1) - ifw * 2,
                          margin, w * 3 / 5, h);
    }

    cancel->setGeometry(margin, input.geometry().bottom() + gutter, w / 2, h);
    okay->setGeometry(cancel->geometry().right() + gutter,
                      cancel->geometry().y(), w / 2, h);
}

void NetHackQtStringRequestor::SetDefault(const char *d)
{
    input.setText(d);
}

bool NetHackQtStringRequestor::Get(char *buffer, int maxchar, int minchar)
{
    input.setMaxLength(maxchar - 1);

    const QString &txt = prompt.text();
    int pw = fontMetrics().QFM_WIDTH(txt),
        ww = minchar * input.fontMetrics().QFM_WIDTH(QChar('X'));
    int heightfactor = ((txt.size() > 16) ? 3 : 2) * 2; // 2 or 3 lines high
    int widthfudge = (((txt.size() > 16) ? 1 : 2) * 5) * 2; // 5: margn, guttr
    resize(pw + ww + widthfudge, fontMetrics().height() * heightfactor);

#ifdef EDIT_GETLIN
    input.setText(buffer);
#endif
    centerOnMain(this);
    show();
    // Make sure that setFocus() really does change keyboard focus.
    // This allows typing to go directly to the NetHackQtLineEdit
    // widget without clicking on or in it first.  Not needed for
    // qt_getline() but is needed for menu Search to prevent typed
    // characters being treated as making menu selections.
    if (!input.isActiveWindow())
        input.activateWindow();
    input.setFocus();
    exec();

    if (result()) {
        str_copy(buffer, input.text().toLatin1().constData(), maxchar);
	return true;
    } else {
	return false;
    }
}

} // namespace nethack_qt_
