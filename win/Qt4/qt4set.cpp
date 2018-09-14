// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt4set.cpp -- the Qt settings

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
#include "qt4set.h"
#include "qt4set.moc"
#include "qt4glyph.h"
#include "qt4str.h"

/* Used by tile/font-size patch below and in ../../src/files.c */
char *qt_tilewidth=NULL;
char *qt_tileheight=NULL;
char *qt_fontsize=NULL;
#if defined(QWS)
int qt_compact_mode = 1;
#else
int qt_compact_mode = 0;
#endif

namespace nethack_qt4 {

#define TILEWMIN 6
#define TILEHMIN 6

NetHackQtSettings::NetHackQtSettings(int w, int h) :
    settings(),
    tilewidth(this),
    tileheight(this),
    widthlbl("&Width:",this),
    heightlbl("&Height:",this),
    whichsize("&Zoomed",this),
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

    connect(&tilewidth,SIGNAL(valueChanged(int)),this,SLOT(resizeTiles()));
    connect(&tileheight,SIGNAL(valueChanged(int)),this,SLOT(resizeTiles()));
    connect(&whichsize,SIGNAL(toggled(bool)),this,SLOT(setGlyphSize(bool)));

    fontsize.addItem("Huge");
    fontsize.addItem("Large");
    fontsize.addItem("Medium");
    fontsize.addItem("Small");
    fontsize.addItem("Tiny");
    fontsize.setCurrentIndex(default_fontsize);
    connect(&fontsize,SIGNAL(activated(int)),this,SLOT(changedFont()));

    QGridLayout* grid = new QGridLayout(this);
    grid->addWidget(&whichsize, 0, 0, 1, 2);
    grid->addWidget(&tilewidth, 1, 1); grid->addWidget(&widthlbl, 1, 0);
    grid->addWidget(&tileheight, 2, 1); grid->addWidget(&heightlbl, 2, 0);
    QLabel* flabel=new QLabel("&Font:",this);
    flabel->setBuddy(&fontsize);
    grid->addWidget(flabel, 3, 0); grid->addWidget(&fontsize, 3, 1);
    QPushButton* dismiss=new QPushButton("Dismiss",this);
    dismiss->setDefault(true);
    grid->addWidget(dismiss, 4, 0, 1, 2);
    grid->setRowStretch(4,0);
    grid->setColumnStretch(1,1);
    grid->setColumnStretch(2,2);
    grid->activate();

    connect(dismiss,SIGNAL(clicked()),this,SLOT(accept()));
    resize(150,140);
}

NetHackQtGlyphs& NetHackQtSettings::glyphs()
{
    return *theglyphs;
}

void NetHackQtSettings::changedFont()
{
    settings.setValue("fontsize", fontsize.currentIndex());
    emit fontChanged();
}

void NetHackQtSettings::resizeTiles()
{
    int w = tilewidth.value();
    int h = tileheight.value();

    settings.setValue("tilewidth", tilewidth.value());
    settings.setValue("tileheight", tileheight.value());
    theglyphs->setSize(w,h);
    emit tilesChanged();
}

void NetHackQtSettings::toggleGlyphSize()
{
    whichsize.toggle();
}

void NetHackQtSettings::setGlyphSize(bool which)
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

} // namespace nethack_qt4
