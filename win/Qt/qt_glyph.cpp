// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_glyph.cpp -- class to manage the glyphs in a tile set

extern "C" {
#include "hack.h"
#include "tile2x11.h" /* x11tiles is potential fallback for nhtiles.bmp */
}

#include "qt_pre.h"
#include <QtGui/QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QtWidgets>
#endif
#include "qt_post.h"
#include "qt_glyph.h"
#include "qt_bind.h"
#include "qt_set.h"
#include "qt_inv.h"
#include "qt_map.h"
#include "qt_str.h"

extern short glyph2tile[]; // from tile.c

namespace nethack_qt_ {

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
    boolean tilesok = TRUE, user_tiles = (::iflags.wc_tile_file != NULL);
    char cbuf[BUFSZ];
    const char *tile_file = NULL, *tile_list[2];

    this->no_tiles = false;
    tiles_per_row = TILES_PER_ROW;

    if (user_tiles) {
        tile_list[0] = ::iflags.wc_tile_file;
        Snprintf(cbuf, sizeof cbuf, "%s/%s", PIXMAPDIR, tile_list[0]);
        tile_list[1] = cbuf;
    } else {
        tile_list[0] = PIXMAPDIR "/nhtiles.bmp";
        tile_list[1] = PIXMAPDIR "/x11tiles";
    }

    if (img.load(tile_list[0]))
        tile_file = tile_list[0];
    else if (img.load(tile_list[1]))
        tile_file = tile_list[1];

    if (!tile_file) {
        tilesok = FALSE;
        QString msg = nh_qsprintf("Cannot load '%s'.",
                                  user_tiles ? tile_list[0]
                                    // mismatched quotes match format
                                    : "nhtiles.bmp' or 'x11tiles");
        QMessageBox::warning(0, "IO Error", msg);
    } else {
        if (img.width() % tiles_per_row) {
            tilesok = FALSE;
            impossible(
            "Tile file \"%s\" has %d columns, not multiple of row count (%d)",
                       tile_file, img.width(), tiles_per_row);
        }
    }

    if (!tilesok) {
        this->no_tiles = true;
        /* tiles wouldn't load so force ascii map */
        ::iflags.wc_ascii_map = 1;
        ::iflags.wc_tiled_map = 0;
        /* tiles wouldn't load so don't allow toggling to tiled map */
        ::set_wc_option_mod_status(WC_ASCII_MAP | WC_TILED_MAP,
                                   ::set_in_config);
        tiles_per_row = 40; // arbitrary to avoid potential divide-by-0
    }

    if (iflags.wc_tile_width)
	tilefile_tile_W = iflags.wc_tile_width;
    else if (iflags.wc_ascii_map)
	tilefile_tile_W = 16;
    else
	tilefile_tile_W = img.width() / tiles_per_row;

    if (iflags.wc_tile_height)
	tilefile_tile_H = iflags.wc_tile_height;
    else
	tilefile_tile_H = tilefile_tile_W;

    setSize(tilefile_tile_W, tilefile_tile_H);
}

// display a map tile somewhere other than the map;
// used for paper doll and also role/race selection
void
NetHackQtGlyphs::drawGlyph(
    QPainter& painter,
    int glyph, int tileidx,
    int x, int y,
    bool reversed)
{
    if (!reversed) {
#if 0
        int tile = glyph2tile[glyph];
#else
	int tile = tileidx;
#endif
        int px = (tile % tiles_per_row) * width();
        int py = tile / tiles_per_row * height();

        painter.drawPixmap(x, y, pm, px, py, width(), height());
    } else {
        // for paper doll; mirrored image for left side of two-handed weapon
        painter.drawPixmap(x, y, reversed_pixmap(glyph, tileidx),
                           0, 0, width(), height());
    }
}

// draw a tile into the paper doll
void
NetHackQtGlyphs::drawCell(
    QPainter& painter,
    int glyph, int tileidx,
    int cellx, int celly)
{
    drawGlyph(painter, glyph, tileidx, cellx * width(), celly * height(),
              false);
}

// draw a tile into the paper doll and then draw a BUC border around it
void
NetHackQtGlyphs::drawBorderedCell(
    QPainter& painter,
    int glyph, int tileidx,
    int cellx, int celly,
    int border, bool reversed)
{
    int wd = width(),
        ht = height(),
        yoffset = 1,  // tiny extra margin at top
        lox = cellx * (wd + 2),
        loy = celly * (ht + 2) + yoffset;

    drawGlyph(painter, glyph, tileidx, lox + 1, loy + 1, reversed);

#ifdef TEXTCOLOR
    if (border != NO_BORDER) {
        // gray would be a better mid-point between red and cyan but it
        // doesn't show up well enough against the wall tile background
        painter.setPen((border == BORDER_CURSED) ? Qt::red
                       : (border == BORDER_UNCURSED) ? Qt::yellow
                         : (border == BORDER_BLESSED) ? Qt::cyan
                           : Qt::white); // BORDER_DEFAULT
        // assuming 32x32, draw 34x34 rectangle from 0..33x0..33, outside glyph
#if 0   /* Qt 5.11 drawRect(x,y,width,height) seems to have an off by 1 bug;
         * drawRect(0,0,34,34) is drawing at 0..34x0..34 which is 35x35;
         * should subtract 1 when adding width and/or height to base coord;
         * the relevant code in QtCore/QRect.h is correct so this observable
         * misbehavior is a mystery... */
        painter.drawRect(lox, loy, wd + 2, ht + 2);
#else
        painter.drawLine(lox, loy, lox + wd + 1, loy); // 0,0->33,0
        painter.drawLine(lox, loy + ht + 1, lox + wd + 1, loy + ht + 1);
        painter.drawLine(lox, loy, lox, loy + ht + 1); // 0,0->0,33
        painter.drawLine(lox + wd + 1, loy, lox + wd + 1, loy + ht + 1);
#endif
        if (border != BORDER_DEFAULT) {
            // assuming 32x32, draw rectangle from 1..32x1..32, inside glyph
#if 0       /* (see above) */
            painter.drawRect(lox + 1, loy + 1, wd, ht);
#else
            painter.drawLine(lox + 1, loy + 1, lox + wd, loy + 1); // 1,1->32,1
            painter.drawLine(lox + 1, loy + ht, lox + wd, loy + ht);
            painter.drawLine(lox + 1, loy + 1, lox + 1, loy + ht); // 1,1->1,32
            painter.drawLine(lox + wd, loy + 1, lox + wd, loy + ht);
#endif
            for (int i = lox + 2; i < lox + wd - 1; i += 2) {
                // assuming 32x32, draw points along <2..31,2> and <2..31,31>
                painter.drawPoint(i, loy + 2);
                painter.drawPoint(i + 1, loy + ht - 1);
            }
            for (int j = loy + 2; j < loy + ht - 1; j += 2) {
                // assuming 32x32, draw points along <2,2..31> and <31,2..31>
                painter.drawPoint(lox + 2, j);
                painter.drawPoint(lox + wd - 1, j + 1);
            }
        }
    }
#else
    nhUse(border);
#endif
}

// mis-named routine to get the pixmap for a particular glyph
QPixmap
NetHackQtGlyphs::glyph(
    int glyphindx UNUSED,
    int tileidx)
{
#if 0
    int tile = glyph2tile[glyphindx];
#else
    int tile = tileidx;
#endif
    int px = (tile % tiles_per_row) * tilefile_tile_W;
    int py = tile / tiles_per_row * tilefile_tile_H;

    return QPixmap::fromImage(img.copy(px, py,
                                       tilefile_tile_W, tilefile_tile_H));
}

// transpose a glyph's tile horizontally, scaled for use in paper doll
QPixmap
NetHackQtGlyphs::reversed_pixmap(
    int glyphindx, int tileidx)
{
    QPixmap pxmp = glyph(glyphindx, tileidx);
#ifdef ENHANCED_PAPERDOLL
    qreal wid = (qreal) pxmp.width(),
          //hgt = (qreal) pxmp.height(),
          xscale = (qreal) qt_settings->dollWidth / (qreal) tilefile_tile_W,
          yscale = (qreal) qt_settings->dollHeight / (qreal) tilefile_tile_H;
    QTransform *mirrormatrix = new QTransform(
            // negate x coordinates to flip the image across the y-axis
            -1.0 * xscale, 0.0, 0.0, yscale,
            // slide flipped image to the right to make things positive again
            wid * xscale, 0.0
    );
    return pxmp.transformed(*mirrormatrix);
#else
    return pxmp;
#endif
}

void NetHackQtGlyphs::setSize(int w, int h)
{
    if (size == QSize(w, h))
	return;
    size = QSize(w, h);
    if (!w || !h)
	return; // Still not decided

    if (size == pm1.size()) { // not zoomed
	pm = pm1;
	return;
    }
    if (size == pm2.size()) { // zoomed
	pm = pm2;
	return;
    }

    bool was1 = (size == pm1.size());
    if (w == tilefile_tile_W && h == tilefile_tile_H) {
	pm.convertFromImage(img);
    } else {
	QApplication::setOverrideCursor(Qt::WaitCursor);
	QImage scaled = img.scaled(
	    w * img.width() / tilefile_tile_W,
	    h * img.height() / tilefile_tile_H,
	    Qt::IgnoreAspectRatio,
	    Qt::FastTransformation
	);
	pm.convertFromImage(scaled, Qt::ThresholdDither | Qt::PreferDither);
	QApplication::restoreOverrideCursor();
    }
    (was1 ? pm2 : pm1) = pm;
}

} // namespace nethack_qt_
