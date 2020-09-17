// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_svsel.cpp -- saved game selector

extern "C" {
#include "hack.h"
}

#include "qt_pre.h"
#include <QtGui/QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QtWidgets>
#endif
#include "qt_post.h"
#include "qt_svsel.h"
#include "qt_bind.h"
#include "qt_str.h"

namespace nethack_qt_ {

NetHackQtSavedGameSelector::NetHackQtSavedGameSelector(const char** saved) :
    QDialog(NetHackQtBind::mainWidget())
{
    QVBoxLayout *vbl = new QVBoxLayout(this);
    QHBoxLayout* hb;

#if 0   // this works but don't add it until the rest is working as intended
    char cvers[BUFSZ];
    QString qvers = QString("NetHack ") + QString(version_string(cvers));
    QLabel* vers = new QLabel(qvers, this);
    vers->setAlignment(Qt::AlignCenter);
    vbl->addWidget(vers);
#endif

    QLabel* logo = new QLabel(this);
    logo->setAlignment(Qt::AlignCenter);
    logo->setPixmap(QPixmap("nhsplash.xpm"));
    vbl->addWidget(logo);

    QLabel* attr = new QLabel("by the NetHack DevTeam",this);
    attr->setAlignment(Qt::AlignCenter);
    vbl->addWidget(attr);
    vbl->addStretch(2);

    /* With Qt5, this next line triggers a complaint to stderr:
QLayout: Attempting to add QLayout "" to QDialog "", which already has a layout
    hb = new QHBoxLayout(this);
     */
    hb = new QHBoxLayout((QWidget *) NULL);
    vbl->addLayout(hb, Qt::AlignCenter);

    QPushButton* q = new QPushButton("Quit",this);
    hb->addWidget(q);
    connect(q, SIGNAL(clicked()), this, SLOT(reject()));
    QPushButton* c = new QPushButton("New Game",this);
    hb->addWidget(c);
    connect(c, SIGNAL(clicked()), this, SLOT(accept()));
    c->setDefault(true);

    //
    // FIXME!
    //  The text "Saved Characters" is overwritten by all the
    //  filename buttons.  The last button added is the only one
    //  visible but clicking on it seems to activate the first
    //  one instead.
    //
    QGroupBox* box = new QGroupBox("Saved Characters",this);
    QButtonGroup *bg = new QButtonGroup(this);
    vbl->addWidget(box);
    QVBoxLayout *bgl UNUSED = new QVBoxLayout(box);
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

} // namespace nethack_qt_
