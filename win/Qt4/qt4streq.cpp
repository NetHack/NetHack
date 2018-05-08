// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt4streq.cpp -- string requestor

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
#include "qt4streq.h"
#include "qt4str.h"

namespace nethack_qt4 {

// temporary
void centerOnMain(QWidget *);
// end temporary

NetHackQtStringRequestor::NetHackQtStringRequestor(QWidget *parent, const char* p, const char* cancelstr) :
    QDialog(parent),
    prompt(QString::fromLatin1(p),this),
    input(this,"input")
{
    cancel=new QPushButton(cancelstr,this);
    connect(cancel,SIGNAL(clicked()),this,SLOT(reject()));

    okay=new QPushButton("Okay",this);
    connect(okay,SIGNAL(clicked()),this,SLOT(accept()));
    connect(&input,SIGNAL(returnPressed()),this,SLOT(accept()));
    okay->setDefault(true);

    setFocusPolicy(Qt::StrongFocus);
}

void NetHackQtStringRequestor::resizeEvent(QResizeEvent*)
{
    const int margin=5;
    const int gutter=5;

    int h=(height()-margin*2-gutter);

    if (prompt.text().size() > 16) {
	h/=3;
	prompt.setGeometry(margin,margin,width()-margin*2,h);
	input.setGeometry(width()*1/5,margin+h+gutter,
	    (width()-margin-2-gutter)*4/5,h);
    } else {
	h/=2;
	prompt.setGeometry(margin,margin,(width()-margin*2-gutter)*2/5,h);
	input.setGeometry(prompt.geometry().right()+gutter,margin,
	    (width()-margin-2-gutter)*3/5,h);
    }

    cancel->setGeometry(margin,input.geometry().bottom()+gutter,
	(width()-margin*2-gutter)/2,h);
    okay->setGeometry(cancel->geometry().right()+gutter,cancel->geometry().y(),
	cancel->width(),h);
}

void NetHackQtStringRequestor::SetDefault(const char* d)
{
    input.setText(d);
}

bool NetHackQtStringRequestor::Get(char* buffer, int maxchar)
{
    input.setMaxLength(maxchar);
    if (prompt.text().size() > 16) {
	resize(fontMetrics().width(prompt.text())+50,fontMetrics().height()*6);
    } else {
	resize(fontMetrics().width(prompt.text())*2+50,fontMetrics().height()*4);
    }

#ifdef EDIT_GETLIN
    input.setText(buffer);
#endif
    centerOnMain(this);
    show();
    input.setFocus();
    exec();

    if (result()) {
	str_copy(buffer,input.text().toLatin1().constData(),maxchar);
	return true;
    } else {
	return false;
    }
}

} // namespace nethack_qt4
