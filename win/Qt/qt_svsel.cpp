// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_svsel.cpp -- saved game selector
//
// Popup's layout:
//----
//         NetHack 3.7.x          (literal text w/ dynamic version number)
//    /----------------------\    (text to prevent multi-line comment warning)
//    |                      |
//    |                      |
//    |     splash image     |    nhsplash.xpm (red dragon w/ rider)
//    |                      |
//    |                      |
//    \----------------------/
//     by the NetHack DevTeam     (literal text)
//      [  Quit  ] [New-Game]     side-by-side buttons
//      Saved characters          (literal text in small font)
//      [   character one   ]     button with character name
//      [   character two   ]     button with another character name
//      [  character three  ]     button with yet another character name
//              ...               as many buttons as needed
//----
//
// TODO?
//  Character names are sorted alphabetically.  It would be useful to
//  be able to sort by role or by game start date or by save date.
//  The core fetches character names from inside the files; it could
//  obtain the information needed for alternate sorting.  Simpler
//  enchancement:  instead of just showing the character name, show
//  "name-role-race-gender-alignment".
//
// Note:
//  The code in this file is not used if the program is built without
//  having SELECTSAVED defined or if the run-time option 'selectsaved'
//  is False.  SELECTSAVED used to be forced for Qt but isn't any more.
//  Howver, we include this code unconditionally.
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
#include "qt_svsel.h"
#include "qt_bind.h"
#include "qt_str.h"

namespace nethack_qt_ {

//
// popup dialog at start of game to choose between a new game or which save
// file to load if user has at least one saved game and either hasn't supplied
// any character name or has supplied one which doesn't have a save file
//
NetHackQtSavedGameSelector::NetHackQtSavedGameSelector(const char** saved) :
    QDialog(NetHackQtBind::mainWidget())
{
    QVBoxLayout *vbl = new QVBoxLayout(this);
    QHBoxLayout* hb;

    char cvers[BUFSZ];
    QString qvers = QString("NetHack ") + QString(version_string(cvers, sizeof cvers));
    QLabel *vers = new QLabel(qvers, this);
    vers->setAlignment(Qt::AlignCenter);
    vbl->addWidget(vers);

    QLabel *logo = new QLabel(this);
    logo->setAlignment(Qt::AlignCenter);
    logo->setPixmap(QPixmap("nhsplash.xpm"));
    vbl->addWidget(logo);

    QLabel *attr = new QLabel("by the NetHack DevTeam", this);
    attr->setAlignment(Qt::AlignCenter);
    vbl->addWidget(attr);
    vbl->addStretch(2);

    /* With Qt5, this next line triggers a complaint to stderr:
QLayout: Attempting to add QLayout "" to QDialog "", which already has a layout
    hb = new QHBoxLayout(this);
     */
    hb = new QHBoxLayout((QWidget *) NULL);
    vbl->addLayout(hb, Qt::AlignCenter);

    QPushButton *q = new QPushButton("Quit", this);
    hb->addWidget(q);
    connect(q, SIGNAL(clicked()), this, SLOT(reject()));
    QPushButton *c = new QPushButton("New Game", this);
    hb->addWidget(c);
    connect(c, SIGNAL(clicked()), this, SLOT(accept()));
    c->setDefault(true);

    QGroupBox *box = new QGroupBox("Saved Characters", this);
    QVBoxLayout *bgl = new QVBoxLayout();
    QButtonGroup *bg = new QButtonGroup();
    connect(bg, SIGNAL(buttonPressed(int)), this, SLOT(done(int)));
    for (int i = 0; saved[i]; ++i) {
        QPushButton *b = new QPushButton(saved[i]);
        bgl->addWidget(b);
        bg->addButton(b, i + 2);
    }
    box->setLayout(bgl);
    vbl->addWidget(box);
}

int NetHackQtSavedGameSelector::choose()
{
#if defined(QWS) // probably safe with Qt 3, too (where show!=exec in QDialog).
    if ( qt_compact_mode )
	showMaximized();
#endif
    return exec() - 2;
}

} // namespace nethack_qt_
