// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_icon.cpp -- a labelled icon for display in the status window
//
// TODO?
//  When the label specifies two values separated by a slash (curHP/maxHP,
//    curEn/maxEn, XpLevel/ExpPoints when 'showexp' is On), highlighting
//    for changes is all or nothing.  curHP and curEn go up and down
//    without any change to the corresponding maximum all the time.  Much
//    rarer, but when maxHP and maxEn go up with level gain, the hero
//    could be injured by a passive counterattack or collateral damage
//    from an area effect--or much simpler, the casting cost of a spell
//    that killed a monster and produced the level gain--so the current
//    value could stay the same or even go down at same time max goes up.
//    Likewise, Exp goes up a lot but Xp relatively rarely.  (On the very
//    rare occasions where either goes down, they'll both do so.)
//    Highlighting two slash-separated values independently would be
//    worthwhile but with the 'single label using a style sheet for color'
//    approach it isn't going to happen.
//

extern "C" {
#include "hack.h"
}

#include "qt_pre.h"
#include <QtGui/QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QtWidgets>
#endif
#include "qt_post.h"
#include "qt_icon.h"

namespace nethack_qt_ {

NetHackQtLabelledIcon::NetHackQtLabelledIcon(QWidget *parent, const char *l) :
    QWidget(parent),
    label(new QLabel(l,this)),
    icon(NULL),
    low_is_good(false),
    prev_value(-123),
    turn_count(-1)
{
    initHighlight();
}

NetHackQtLabelledIcon::NetHackQtLabelledIcon(QWidget *parent, const char *l,
                                             const QPixmap &i) :
    QWidget(parent),
    label(new QLabel(l,this)),
    icon(new QLabel(this)),
    low_is_good(false),
    prev_value(-123),
    turn_count(-1)
{
    setIcon(i);
    initHighlight();
}

void NetHackQtLabelledIcon::initHighlight()
{
    // note: named 'green' is much darker than Qt::green
    hl_good = "QLabel { background-color : green; color : white }";
    hl_bad  = "QLabel { background-color : red  ; color : white }";
}

void NetHackQtLabelledIcon::setLabel(const QString& t, bool lower)
{
    if (!label) {
	label=new QLabel(this);
	label->setFont(font());
    }
    if (label->text() != t) {
	label->setText(t);
        ForceResize();
	highlight((lower == low_is_good) ? hl_good : hl_bad);
    }
}

void NetHackQtLabelledIcon::setLabel(const QString& t, long v, long cv,
                                     const QString& tail)
{
    QString buf;
    if (v==NoNum) {
	buf = "";
    } else {
	buf.sprintf("%ld", v);
    }
    setLabel(t + buf + tail, cv < prev_value);
    prev_value=cv;
}

void NetHackQtLabelledIcon::setLabel(const QString& t, long v,
                                     const QString& tail)
{
    setLabel(t,v,v,tail);
}

void NetHackQtLabelledIcon::setIcon(const QPixmap& i)
{
    if (!icon)
        icon = new QLabel(this);
    icon->setPixmap(i);
    ForceResize();
    icon->resize(i.width(), i.height());
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

QSize NetHackQtLabelledIcon::sizeHint() const
{
    QSize iconsize, textsize;

    if (label && !icon) return label->sizeHint();
    if (icon && !label) return icon->sizeHint();
    if (!label && !icon) return QWidget::sizeHint();

    iconsize = icon->sizeHint();
    textsize = label->sizeHint();
    return QSize(
	std::max(iconsize.width(), textsize.width()),
	iconsize.height() + textsize.height());
}

QSize NetHackQtLabelledIcon::minimumSizeHint() const
{
    QSize iconsize, textsize;

    if (label && !icon) return label->minimumSizeHint();
    if (icon && !label) return icon->minimumSizeHint();
    if (!label && !icon) return QWidget::minimumSizeHint();

    iconsize = icon->minimumSizeHint();
    textsize = label->minimumSizeHint();
    return QSize(
	std::max(iconsize.width(), textsize.width()),
	iconsize.height() + textsize.height());
}

void NetHackQtLabelledIcon::highlightWhenChanging()
{
    turn_count=0;
}

void NetHackQtLabelledIcon::lowIsGood()
{
    low_is_good=true;
}

void NetHackQtLabelledIcon::dissipateHighlight()
{
    if (turn_count>0) {
	turn_count--;
	if (!turn_count)
	    unhighlight();
    }
}

void NetHackQtLabelledIcon::highlight(const QString& hl)
{
    if (label) { // Surely it is?!
	if (turn_count>=0) {
	    label->setStyleSheet(hl);
	    turn_count=4;
	    // `4' includes this turn, so dissipates after
	    // 3 more keypresses.
	} else {
	    label->setStyleSheet("");
	}
    }
}

void NetHackQtLabelledIcon::unhighlight()
{
    if (label) { // Surely it is?!
	label->setStyleSheet("");
    }
}

// used when label (most status fields) or pixmap (alignment, hunger,
// encumbrance) changes value
void NetHackQtLabelledIcon::ForceResize()
{
    this->resizeEvent((QResizeEvent *) NULL);
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
    if (label) label->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
    if (icon) icon->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
}

} // namespace nethack_qt_
