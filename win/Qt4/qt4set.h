// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt4set.h -- the Qt settings

#ifndef QT4SET_H
#define QT4SET_H

namespace nethack_qt4 {

class NetHackQtGlyphs;

class NetHackQtSettings : public QDialog {
	Q_OBJECT
public:
	// Size of window - used to decide default sizes
	NetHackQtSettings(int width, int height);

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

private:
	QSettings settings;
	QSpinBox tilewidth;
	QSpinBox tileheight;
	QLabel widthlbl;
	QLabel heightlbl;
	QCheckBox whichsize;
	QSize othersize;

	QComboBox fontsize;

	QFont normal, normalfixed, large;

	NetHackQtGlyphs* theglyphs;

private slots:
	void resizeTiles();
	void changedFont();
};

extern NetHackQtSettings* qt_settings;

} // namespace nethack_qt4

#endif
