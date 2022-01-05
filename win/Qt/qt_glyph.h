// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_glyph.h -- class to manage the glyphs in a tile set

#ifndef QT4GLYPH_H
#define QT4GLYPH_H

namespace nethack_qt_ {

enum border_code {
    NO_BORDER, BORDER_DEFAULT,
    BORDER_CURSED, BORDER_UNCURSED, BORDER_BLESSED
};

class NetHackQtGlyphs {
public:
        bool no_tiles;

	NetHackQtGlyphs();

	int width() const { return size.width(); }
	int height() const { return size.height(); }
	void toggleSize();
	void setSize(int w, int h);

        void drawGlyph(QPainter &, int glyph, int tileidx,
                       int pixelx, int pixely,
                       bool reversed = false);
        void drawCell(QPainter &, int glyph, int tileidx,
                      int cellx, int celly);
        void drawBorderedCell(QPainter &, int glyph, int tileidx,
                              int cellx, int celly, int bordercode,
                              bool reversed);
        QPixmap glyph(int glyphindx, int tileidx);
        QPixmap reversed_pixmap(int glyphindx, int tileidx);

private:
	QImage img;
	QPixmap pm,pm1, pm2;
	QSize size;
	int tiles_per_row;
        //QTransform *mirrormatrix;
};

} // namespace nethack_qt_

#endif
