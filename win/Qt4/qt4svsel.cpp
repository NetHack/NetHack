// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt4svsel.cpp -- saved game selector

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
#include "qt4svsel.h"
#include "qt4bind.h"
#include "qt4str.h"

namespace nethack_qt4 {

NetHackQtSavedGameSelector::NetHackQtSavedGameSelector(const char** saved) :
    QDialog(NetHackQtBind::mainWidget())
{
    QVBoxLayout *vbl = new QVBoxLayout(this);
    QHBoxLayout* hb;

    QLabel* logo = new QLabel(this); vbl->addWidget(logo);
    logo->setAlignment(Qt::AlignCenter);
    logo->setPixmap(QPixmap("nhsplash.xpm"));
    QLabel* attr = new QLabel("by the NetHack DevTeam",this);
    attr->setAlignment(Qt::AlignCenter);
    vbl->addWidget(attr);
    vbl->addStretch(2);
    /*
    QLabel* logo = new QLabel(hb);
    hb = new QHBox(this);
    vbl->addWidget(hb, Qt::AlignCenter);
    logo->setPixmap(QPixmap(nh_icon));
    logo->setAlignment(AlignRight|Qt::AlignVCenter);
    new QLabel(nh_attribution,hb);
    */

    hb = new QHBoxLayout(this);
    vbl->addLayout(hb, Qt::AlignCenter);
    QPushButton* q = new QPushButton("Quit",this);
    hb->addWidget(q);
    connect(q, SIGNAL(clicked()), this, SLOT(reject()));
    QPushButton* c = new QPushButton("New Game",this);
    hb->addWidget(c);
    connect(c, SIGNAL(clicked()), this, SLOT(accept()));
    c->setDefault(true);

    QGroupBox* box = new QGroupBox("Saved Characters",this);
    QButtonGroup *bg = new QButtonGroup(this);
    vbl->addWidget(box);
    QVBoxLayout *bgl = new QVBoxLayout(box);
    connect(bg, SIGNAL(buttonPressed(int)), this, SLOT(done(int)));
    for (int i=0; saved[i]; i++) {
	QPushButton* b = new QPushButton(saved[i],box);
	bg->addButton(b, i+2);
    }
}

int NetHackQtSavedGameSelector::choose()
{
#if defined(QWS) // probably safe with Qt 3, too (where show!=exec in QDialog).
    if ( qt_compact_mode )
	showMaximized();
#endif
    return exec()-2;
}

} // namespace nethack_qt4
