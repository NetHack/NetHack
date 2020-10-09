// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_set.cpp -- Qt-specific settings, saved and restored by Qt so
//               persist not just across save/restore but into new games.

extern "C" {
#include "hack.h"
}

#include "qt_pre.h"
#include <QtGui/QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QtWidgets>
#endif
#include "qt_post.h"
#include "qt_set.h"
#include "qt_set.moc"
#include "qt_glyph.h"
#include "qt_main.h"
#include "qt_bind.h"
#include "qt_str.h"

// Dialog box accessed via "Qt Settings..." in the games menu (non-OSX)
// or via "Preferences..." in the application menu (OSX):
//--
//      "Qt NetHack Settings"
//  "Map:"    [ ] Zoomed  -- check box
//            tilewidth   -- number entry spinner
//            tileheight  -- ditto
//  "Invent:" [ ] Shown   -- check box
//            dollwidth   -- number entry spinner
//            dollheight  -- ditto
//  "Font:"   fontsize    -- Huge:18pt, Large:14, Medium:12, Small:10, Tiny:8
//            [dismiss]   -- button
//--
// Map remembers 2 size pairs, one for Zoomed unchecked, another for checked.
// (Player controls whether the box is checked, using the dialog to manually
// switch back and forth if desired; nothing forces the Zoomed setting to
// specify larger tile size than not-Zoomed.)
// Paper doll inventory subset is shown or suppressed depending on check box.
// (It only remembers one tile size pair and that only matters when shown.
// The size could be different from both map settings but it is highly
// recommended that it match one of those unless Zoomed is never toggled.)
// Font size is used for message window and for text in the status window.
// (TODO: support separate font sizes for the two windows.)
// There's no way to undo or avoid saving any changes which player makes but
// all of the fields can be manually reversed.

/* Used by tile/font-size patch below and in ../../src/files.c */
char *qt_tilewidth=NULL;
char *qt_tileheight=NULL;
char *qt_fontsize=NULL;
#if defined(QWS)
int qt_compact_mode = 1;
#else
int qt_compact_mode = 0;
#endif

namespace nethack_qt_ {

#define TILEWMIN 6
#define TILEHMIN 6

NetHackQtSettings::NetHackQtSettings() :
    settings(),
    whichsize("&Zoomed", this),
    tilewidth(this),
    tileheight(this),
    widthlbl("Tile &width:", this),
    heightlbl("Tile &height:", this),
#ifdef ENHANCED_PAPERDOLL
    dollshown("&Shown", this),
    dollwidth(this),
    dollheight(this),
    dollwidthlbl("&Doll width:", this),  // should "Doll tile width"...
    dollheightlbl("Doll height:", this), // ...but that's too verbose
#endif
    fontsize(this),
    normal("times"),
#ifdef WS_WIN
    normalfixed("courier new"),
#else
    normalfixed("courier"),
#endif
    large("times"),
    theglyphs(0)
{
    int default_fontsize;

    widthlbl.setBuddy(&tilewidth);
    tilewidth.setRange(TILEWMIN, 128);
    heightlbl.setBuddy(&tileheight);
    tileheight.setRange(TILEHMIN, 128);
    tilewidth.setValue(settings.value("tilewidth", 16).toInt());
    tileheight.setValue(settings.value("tileheight", 16).toInt());
#ifdef ENHANCED_PAPERDOLL
    dollwidthlbl.setBuddy(&dollwidth);
    dollwidth.setRange(TILEWMIN, 48);
    dollheightlbl.setBuddy(&dollheight);
    dollheight.setRange(TILEHMIN, 48);
    dollwidth.setValue(settings.value("dollwidth", 32).toInt());
    dollheight.setValue(settings.value("dollheight", 32).toInt());
    doll_is_shown = settings.value("dollShown", true).toBool();
    // needed the very first time
    settings.setValue("dollShown", QVariant(doll_is_shown));
#endif
    default_fontsize = settings.value("fontsize", 2).toInt();

    // Tile/font sizes read from .nethackrc
    if (qt_tilewidth != NULL) {
	tilewidth.setValue(atoi(qt_tilewidth));
	delete[] qt_tilewidth;
    }
    if (qt_tileheight != NULL) {
	tileheight.setValue(atoi(qt_tileheight));
	delete[] qt_tileheight;
    }
    if (qt_fontsize != NULL) {
	switch (tolower(qt_fontsize[0])) {
	  case 'h': default_fontsize = 0; break;
	  case 'l': default_fontsize = 1; break;
	  case 'm': default_fontsize = 2; break;
	  case 's': default_fontsize = 3; break;
	  case 't': default_fontsize = 4; break;
	}
	delete[] qt_fontsize;
    }

    theglyphs=new NetHackQtGlyphs();
    resizeTiles();

    connect(&whichsize, SIGNAL(toggled(bool)), this, SLOT(setGlyphSize(bool)));
    connect(&tilewidth, SIGNAL(valueChanged(int)), this, SLOT(resizeTiles()));
    connect(&tileheight, SIGNAL(valueChanged(int)), this, SLOT(resizeTiles()));

#ifdef ENHANCED_PAPERDOLL
    connect(&dollshown, SIGNAL(toggled(bool)), this, SLOT(setDollShown(bool)));
    connect(&dollwidth, SIGNAL(valueChanged(int)), this, SLOT(resizeDoll()));
    connect(&dollheight, SIGNAL(valueChanged(int)), this, SLOT(resizeDoll()));
#endif

    fontsize.addItem("Huge");
    fontsize.addItem("Large");
    fontsize.addItem("Medium");
    fontsize.addItem("Small");
    fontsize.addItem("Tiny");
    fontsize.setCurrentIndex(default_fontsize);
    connect(&fontsize, SIGNAL(activated(int)), this, SLOT(changedFont()));

    int row = 0; // used like X11-style XtSetArg(), ++argc
     QGridLayout *grid = new QGridLayout(this);
    // dialog box label, spans first two rows and all three columns
    QLabel *settings_label = new QLabel("Qt NetHack Settings\n", this);
    grid->addWidget(settings_label, row, 0, 2, 3), row += 2; // uses extra row
    settings_label->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    QLabel *map_label = new QLabel("&Map:", this);
    map_label->setBuddy(&whichsize);
    grid->addWidget(map_label, row, 0), // "Map: [ ]Zoomed"
        grid->addWidget(&whichsize, row, 1), ++row;
    grid->addWidget(&widthlbl, row, 1),
        grid->addWidget(&tilewidth, row, 2), ++row;
    grid->addWidget(&heightlbl, row, 1),
        grid->addWidget(&tileheight, row, 2), ++row;

#ifdef ENHANCED_PAPERDOLL
    dollshown.QAbstractButton::setChecked(doll_is_shown);
    QLabel *doll_label = new QLabel("&Invent:", this);
    doll_label->setBuddy(&dollshown);
    grid->addWidget(doll_label, row, 0), // "Invent: [ ]Shown"
        grid->addWidget(&dollshown, row, 1), ++row;
    grid->addWidget(&dollwidthlbl, row, 1),
        grid->addWidget(&dollwidth, row, 2), ++row;
    grid->addWidget(&dollheightlbl, row, 1),
        grid->addWidget(&dollheight, row, 2), ++row;
#endif

    QLabel *flabel = new QLabel("&Font:", this);
    flabel->setBuddy(&fontsize);
    grid->addWidget(flabel, row, 0),
        grid->addWidget(&fontsize, row, 1), ++row;

    QPushButton *dismiss = new QPushButton("Dismiss", this);
    dismiss->setDefault(true);
    grid->addWidget(dismiss, row, 0, 1, 3), ++row;
    grid->setRowStretch(row - 1, 0);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 2);
    grid->activate();

    connect(dismiss, SIGNAL(clicked()), this, SLOT(accept()));
    resize(150, 140);
}

NetHackQtGlyphs& NetHackQtSettings::glyphs()
{
    // Caveat:
    //  'theglyphs' will be Null if the tiles file couldn't be loaded;
    //  the game can still procede with an ascii map in that situation.
    return *theglyphs;
}

void NetHackQtSettings::changedFont()
{
    settings.setValue("fontsize", fontsize.currentIndex());
    emit fontChanged();
}

void NetHackQtSettings::resizeTiles()
{
    tileWidth = tilewidth.value();
    tileHeight = tileheight.value();

    settings.setValue("tilewidth", tileWidth);
    settings.setValue("tileheight", tileHeight);

    if (theglyphs) {
        theglyphs->setSize(tileWidth, tileHeight);
        emit tilesChanged();
    }
}

void NetHackQtSettings::toggleGlyphSize()
{
    whichsize.toggle();
}

void NetHackQtSettings::setGlyphSize(bool which UNUSED)
{
    QSize n = QSize(tilewidth.value(),tileheight.value());
    if ( othersize.isValid() ) {
	tilewidth.blockSignals(true);
	tileheight.blockSignals(true);
	tilewidth.setValue(othersize.width());
	tileheight.setValue(othersize.height());
	tileheight.blockSignals(false);
	tilewidth.blockSignals(false);
	resizeTiles();
    }
    othersize = n;
}

#ifdef ENHANCED_PAPERDOLL
void NetHackQtSettings::resizeDoll()
{
    dollWidth = dollwidth.value();
    dollHeight = dollheight.value();

    settings.setValue("dollwidth", dollWidth);
    settings.setValue("dollheight", dollHeight);
    settings.setValue("dollShown", doll_is_shown);

    //NetHackQtMainWindow::resizePaperDoll(doll_is_shown);
    NetHackQtMainWindow *w = static_cast <NetHackQtMainWindow *>
                             (NetHackQtBind::mainWidget());
    w->resizePaperDoll(doll_is_shown);
}

void NetHackQtSettings::toggleDollShown()
{
    dollshown.toggle();
}

void NetHackQtSettings::setDollShown(bool on_off)
{
    if (on_off != doll_is_shown) {
        dollshown.QAbstractButton::setChecked(on_off);
        doll_is_shown = on_off;
        resizeDoll();
    }
}
#endif

const QFont& NetHackQtSettings::normalFont()
{
    static int size[]={ 18, 14, 12, 10, 8 };
    normal.setPointSize(size[fontsize.currentIndex()]);
    return normal;
}

const QFont& NetHackQtSettings::normalFixedFont()
{
    static int size[]={ 18, 14, 13, 10, 8 };
    normalfixed.setPointSize(size[fontsize.currentIndex()]);
    return normalfixed;
}

const QFont& NetHackQtSettings::largeFont()
{
    static int size[]={ 24, 18, 14, 12, 10 };
    large.setPointSize(size[fontsize.currentIndex()]);
    return large;
}

bool NetHackQtSettings::ynInMessages()
{
    return !qt_compact_mode && !iflags.wc_popup_dialog;
}

NetHackQtSettings* qt_settings;

} // namespace nethack_qt_
