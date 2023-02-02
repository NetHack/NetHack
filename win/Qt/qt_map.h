// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_map.h -- the map window

#ifndef QT4MAP_H
#define QT4MAP_H

#include "qt_win.h"
#include "qt_clust.h"

namespace nethack_qt_ {

class NetHackQtClickBuffer;

class NetHackQtMapViewport : public QWidget {
	Q_OBJECT
public:
	NetHackQtMapViewport(NetHackQtClickBuffer& click_sink);
	~NetHackQtMapViewport(void);

protected:
	virtual void paintEvent(QPaintEvent* event);
        bool DrawWalls(QPainter& painter, int x, int y,
                       int w, int h, unsigned short ch);
	virtual QSize sizeHint() const;
	virtual QSize minimumSizeHint() const;
	virtual void mousePressEvent(QMouseEvent* event);

private:
        QFont *rogue_font;
        unsigned short glyph[ROWNO][COLNO];
        unsigned short &Glyph(int x, int y) {
            return glyph[y][x];
        }
        char32_t glyphttychar[ROWNO][COLNO];
        char32_t &Glyphttychar(int x, int y) {
            return glyphttychar[y][x];
        }
        uint32 glyphcolor[ROWNO][COLNO];
        uint32 &Glyphcolor(int x, int y) {
            return glyphcolor[y][x];
        }
        uint32 glyphframecolor[ROWNO][COLNO];
        uint32 &GlyphFramecolor(int x, int y) {
            return glyphframecolor[y][x];
        }
        unsigned int glyphflags[ROWNO][COLNO];
        unsigned int &Glyphflags(int x, int y) {
            return glyphflags[y][x];
        }
        unsigned short glyphtileidx[ROWNO][COLNO];
        unsigned short &Glyphtileidx(int x, int y) {
            return glyphtileidx[y][x];
        }
        QPoint cursor;
        QPixmap pet_annotation;
        QPixmap pile_annotation;
        NetHackQtClickBuffer &clicksink;
        Clusterizer change;

	void clickCursor();
	void Clear();
	void Display(bool block);
	void CursorTo(int x,int y);
	void PrintGlyph(int x,int y, const glyph_info *glyphinfo, const glyph_info *bkglyphinfo);
	void Changed(int x, int y);
	void updateTiles();
        void SetupTextmapFont(QPainter &painter);

	// NetHackQtMapWindow2 passes through many calls to the viewport
	friend class NetHackQtMapWindow2;
};

class NetHackQtMapWindow2 : public QScrollArea, public NetHackQtWindow {
	Q_OBJECT
public:
	NetHackQtMapWindow2(NetHackQtClickBuffer& click_sink);
	void clearMessages();
	void putMessage(int attr, const QString& text);
	void clickCursor();
	virtual QWidget *Widget();

	virtual void Clear();
	virtual void Display(bool block);
	virtual void CursorTo(int x,int y);
	virtual void PutStr(int attr, const QString& text);
	virtual void ClipAround(int x,int y);
	virtual void PrintGlyph(int x,int y, const glyph_info *glyphinfo, const glyph_info *bkglyphinfo);

signals:
	void resized();

private slots:
	void updateTiles();

private:
	NetHackQtMapViewport *m_viewport;
	QRect messages_rect;
	QString messages;
};

} // namespace nethack_qt_

#endif
