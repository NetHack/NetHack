// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_set.h -- the Qt settings

#ifndef QT4SET_H
#define QT4SET_H

#define ENHANCED_PAPERDOLL /* separate size from map tiles, can be hidden */

#include "qt_bind.h" // needed for mainWidget() for updateInventory()

namespace nethack_qt_ {

class NetHackQtGlyphs;
class NetHackQtMainWindow;

class NetHackQtSettings : public QDialog {
	Q_OBJECT
public:
        int tileWidth = 16, tileHeight = 16;
#ifdef ENHANCED_PAPERDOLL
        int dollWidth = 32, dollHeight = 32;
        bool doll_is_shown = true;
#endif
	// dialog box for Qt-specific settings
	NetHackQtSettings();

	NetHackQtGlyphs& glyphs();
	const QFont& normalFont();
	const QFont& normalFixedFont();
	const QFont& largeFont();

	bool ynInMessages();

signals:
	void fontChanged();
	void tilesChanged();

public slots:
	void toggleGlyphSize();
	void setGlyphSize(bool);
#ifdef ENHANCED_PAPERDOLL
	void toggleDollShown();
        void setDollShown(bool);
        void resizeDoll();
#endif

private:
	QSettings settings;

	QCheckBox whichsize;
	QSpinBox tilewidth;
	QSpinBox tileheight;
	QLabel widthlbl;
	QLabel heightlbl;
	QSize othersize;
#ifdef ENHANCED_PAPERDOLL
	QCheckBox dollshown;
	QSpinBox dollwidth;
	QSpinBox dollheight;
	QLabel dollwidthlbl;
	QLabel dollheightlbl;
#endif

	QComboBox fontsize;

	QFont normal, normalfixed, large;

	NetHackQtGlyphs* theglyphs;
#if 0
        void updateInventory()
        {
            static_cast <NetHackQtMainWindow *> (NetHackQtBind::mainWidget())
                ->updateInventory();
        }
#endif

private slots:
	void resizeTiles();
	void changedFont();
};

extern NetHackQtSettings* qt_settings;

} // namespace nethack_qt_

#endif
