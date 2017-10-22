// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt4glyph.cpp -- class to manage the glyphs in a tile set

extern "C" {
#include "hack.h"
}
#include "tile2x11.h"
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
#include "qt4glyph.h"
#include "qt4set.h"
#include "qt4str.h"

extern short glyph2tile[]; // from tile.c

namespace nethack_qt4 {

static int tilefile_tile_W=16;
static int tilefile_tile_H=16;

// Debian uses a separate PIXMAPDIR
#ifndef PIXMAPDIR
# ifdef HACKDIR
#  define PIXMAPDIR HACKDIR
# else
#  define PIXMAPDIR "."
# endif
#endif

NetHackQtGlyphs::NetHackQtGlyphs()
{
    const char* tile_file = PIXMAPDIR "/nhtiles.bmp";
    if ( iflags.wc_tile_file )
	tile_file = iflags.wc_tile_file;

    if (!img.load(tile_file)) {
	tile_file = PIXMAPDIR "/x11tiles";
	if (!img.load(tile_file)) {
	    QString msg;
	    msg.sprintf("Cannot load x11tiles or nhtiles.bmp");
	    QMessageBox::warning(0, "IO Error", msg);
	} else {
	    tiles_per_row = TILES_PER_ROW;
	    if (img.width()%tiles_per_row) {
		impossible("Tile file \"%s\" has %d columns, not multiple of row count (%d)",
			tile_file, img.width(), tiles_per_row);
	    }
	}
    } else {
	tiles_per_row = 40;
    }

    if ( iflags.wc_tile_width )
	tilefile_tile_W = iflags.wc_tile_width;
    else
	tilefile_tile_W = img.width() / tiles_per_row;
    if ( iflags.wc_tile_height )
	tilefile_tile_H = iflags.wc_tile_height;
    else
	tilefile_tile_H = tilefile_tile_W;

    setSize(tilefile_tile_W, tilefile_tile_H);
}

void NetHackQtGlyphs::drawGlyph(QPainter& painter, int glyph, int x, int y)
{
    int tile = glyph2tile[glyph];
    int px = (tile%tiles_per_row)*width();
    int py = tile/tiles_per_row*height();

    painter.drawPixmap(
	x,
	y,
	pm,
	px,py,
	width(),height()
    );
}
void NetHackQtGlyphs::drawCell(QPainter& painter, int glyph, int cellx, int celly)
{
    drawGlyph(painter,glyph,cellx*width(),celly*height());
}
QPixmap NetHackQtGlyphs::glyph(int glyph)
{
    int tile = glyph2tile[glyph];
    int px = (tile%tiles_per_row)*tilefile_tile_W;
    int py = tile/tiles_per_row*tilefile_tile_H;

    return QPixmap::fromImage(img.copy(px, py, tilefile_tile_W, tilefile_tile_H));
}
void NetHackQtGlyphs::setSize(int w, int h)
{
    if ( size == QSize(w,h) )
	return;

    bool was1 = size == pm1.size();
    size = QSize(w,h);
    if (!w || !h)
	return; // Still not decided

    if ( size == pm1.size() ) {
	pm = pm1;
	return;
    }
    if ( size == pm2.size() ) {
	pm = pm2;
	return;
    }

    if (w==tilefile_tile_W && h==tilefile_tile_H) {
	pm.convertFromImage(img);
    } else {
	QApplication::setOverrideCursor( Qt::WaitCursor );
	QImage scaled = img.scaled(
	    w*img.width()/tilefile_tile_W,
	    h*img.height()/tilefile_tile_H,
	    Qt::IgnoreAspectRatio,
	    Qt::FastTransformation
	);
	pm.convertFromImage(scaled,Qt::ThresholdDither|Qt::PreferDither);
	QApplication::restoreOverrideCursor();
    }
    (was1 ? pm2 : pm1) = pm;
}

} // namespace nethack_qt4
