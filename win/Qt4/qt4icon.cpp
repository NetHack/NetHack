// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt4icon.cpp -- a labelled icon

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
#include "qt4icon.h"

namespace nethack_qt4 {

NetHackQtLabelledIcon::NetHackQtLabelledIcon(QWidget* parent, const char* l) :
    QWidget(parent),
    low_is_good(false),
    prev_value(-123),
    turn_count(-1),
    label(new QLabel(l,this)),
    icon(0)
{
    initHighlight();
}

NetHackQtLabelledIcon::NetHackQtLabelledIcon(QWidget* parent, const char* l, const QPixmap& i) :
    QWidget(parent),
    low_is_good(false),
    prev_value(-123),
    turn_count(-1),
    label(new QLabel(l,this)),
    icon(new QLabel(this))
{
    setIcon(i);
    initHighlight();
}

void NetHackQtLabelledIcon::initHighlight()
{
    hl_good = "QLabel { background-color : green; color : white }";
    hl_bad  = "QLabel { background-color : red  ; color : white }";
}

void NetHackQtLabelledIcon::setLabel(const QString& t, bool lower)
{
    if (!label) {
	label=new QLabel(this);
	label->setFont(font());
	resizeEvent(0);
    }
    if (label->text() != t) {
	label->setText(t);
	highlight(lower==low_is_good ? hl_good : hl_bad);
    }
}

void NetHackQtLabelledIcon::setLabel(const QString& t, long v, long cv, const QString& tail)
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

void NetHackQtLabelledIcon::setLabel(const QString& t, long v, const QString& tail)
{
    setLabel(t,v,v,tail);
}

void NetHackQtLabelledIcon::setIcon(const QPixmap& i)
{
    if (icon) icon->setPixmap(i);
    else { icon=new QLabel(this); icon->setPixmap(i); resizeEvent(0); }
    icon->resize(i.width(),i.height());
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

} // namespace nethack_qt4
