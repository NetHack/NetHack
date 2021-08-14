// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_icon.cpp -- a labelled icon for display in the status window
//
// TODO?
//  When the label specifies two values separated by a slash (curHP/maxHP,
//    curEn/maxEn, XpLevel/ExpPoints when 'showexp' is On), highlighting
//    for changes is all or nothing based on which field caller passes
//    as the value to use for comparison.  curHP and curEn go up and down
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
// FIXME:
//  Every LabelledIcon duplicates hl_better, hl_worse, hl_changd.
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
    comp_mode(BiggerIsBetter),
    prev_value(NoNum),
    turn_count(-1L)
{
    initHighlight();
}

NetHackQtLabelledIcon::NetHackQtLabelledIcon(QWidget *parent, const char *l,
                                             const QPixmap &i) :
    QWidget(parent),
    label(new QLabel(l,this)),
    icon(new QLabel(this)),
    comp_mode(BiggerIsBetter),
    prev_value(NoNum),
    turn_count(-1L)
{
    setIcon(i);
    initHighlight();
}

// set up the style sheet strings used to specify color for status field
// labels [done "once", but once for each LabelledIcon that's constructed,
// so about 30 copies overall with 3.6's status conditions]
void NetHackQtLabelledIcon::initHighlight()
{
    // note: string "green" is much darker than enum Qt::green
    // QColor("green")              => #00ff00
    // QColor(Qt::green)            => #008000
    // QColor("green").lighter(150) => #00c000  /* hitpoint bar's green */
    hl_better = "QLabel { background-color : #00c000 ; color : white }";
    hl_worse  = "QLabel { background-color : red     ; color : white }";
    hl_changd = "QLabel { background-color : blue    ; color : white }";
}

void NetHackQtLabelledIcon::setLabel(const QString &t, bool lower)
{
    if (!label) {
	label=new QLabel(this);
	label->setFont(font());
    }
    if (label->text() != t) {
	label->setText(t);
        ForceResize();
        if (comp_mode != NoCompare) {
            highlight((comp_mode == NeitherIsBetter) ? hl_changd
                      : (comp_mode == (lower ? SmallerIsBetter
                                             : BiggerIsBetter)) ? hl_better
                        : hl_worse);
        } else if (turn_count) {
            // if we don't want to highlight this status field but it is
            // currently highlighted (perhaps optional Score recently went
            // up and has just been toggled off), remove the highlight
            unhighlight();
        }
    }
}

void NetHackQtLabelledIcon::setLabel(const QString& t, long v, long cv,
                                     const QString& tail)
{
    QString buf;
    if (v==NoNum) {
	buf = "";
    } else {
	buf = QString::asprintf("%ld", v);
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

// used to highlight status conditions going from Off (blank) to On as "Worse"
void NetHackQtLabelledIcon::show()
{
    // Hunger and Encumbrance are worse when going from not shown
    // to anything and they're set to SmallerIsBetter, so both
    // BiggerIsBetter and SmallerIsBetter warrant hl_worse here.
    // Fly, Lev, and Ride are set NeitherIsBetter so that when
    // they appear they won't be classified as worse.
    if (isHidden() && comp_mode != NoCompare)
	highlight((comp_mode != NeitherIsBetter) ? hl_worse : hl_changd);
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
    turn_count = 0; // turn_count starts negative (as flag to not highlight)
}

// set comp_mode to one of NoCompare or {Bigger,Smaller,Neither}IsBetter
void NetHackQtLabelledIcon::setCompareMode(int newmode)
{
    comp_mode = newmode;
}

void NetHackQtLabelledIcon::unhighlight()
{
    if (label) { // Surely it is?!
        label->setStyleSheet("");
    }
    if (turn_count > 0)
        turn_count = 0;
}

void NetHackQtLabelledIcon::highlight(const QString& hl)
{
    if (label) { // Surely it is?!
        if (turn_count >= 0) {
            label->setStyleSheet(hl);
            turn_count = 4;
            // 4 includes this turn, so dissipates after 3 more keypresses.
        } else {
            unhighlight();
        }
    }
}

void NetHackQtLabelledIcon::dissipateHighlight()
{
    if (turn_count > 0) {
        if (!--turn_count)
            unhighlight();
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
