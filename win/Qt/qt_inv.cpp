// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_inv.cpp -- inventory usage window
//
// Essentially a "paper doll" style display.  [grep fodder]
//
// This is at the top center of the main window

extern "C" {
#include "hack.h"
}

#include "qt_pre.h"
#include <QtGui/QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QtWidgets>
#endif
#include "qt_post.h"
#include "qt_inv.h"
#include "qt_glyph.h"
#include "qt_set.h"

namespace nethack_qt_ {

static struct obj *
find_tool(int tooltyp)
{
    struct obj *o;

    for (o = g.invent; o; o = o->nobj) {
        if ((tooltyp == LEASH && o->otyp == LEASH && o->leashmon)
            // OIL_LAMP is used for candles, lamps, lantern, candelabrum too
            || (tooltyp == OIL_LAMP && o->lamplit))
            break;
    }
    return o;
}

NetHackQtInvUsageWindow::NetHackQtInvUsageWindow(QWidget* parent) :
    QWidget(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void NetHackQtInvUsageWindow::drawWorn(QPainter& painter, obj* nhobj,
                                       int x, int y, bool canbe)
{
    short int glyph;
    int border;

    if (nhobj) {
        border = BORDER_DEFAULT;
        if (Role_if('P') && !Blind)
            nhobj->bknown = 1;
        if (nhobj->bknown)
            border = nhobj->cursed ? BORDER_CURSED
                     : !nhobj->blessed ? BORDER_UNCURSED
                       : BORDER_BLESSED;
        glyph = obj_to_glyph(nhobj, rn2_on_display_rng);
    } else {
        border = NO_BORDER;
        glyph = canbe ? cmap_to_glyph(S_room) : GLYPH_UNEXPLORED;
    }
    qt_settings->glyphs().drawBorderedCell(painter, glyph, x, y, border);
}

void NetHackQtInvUsageWindow::paintEvent(QPaintEvent*)
{
    //    0 1 2      two        dual
    //               hander     wielding
    // 0  x H b      x H b      . H b
    // 1  S " w      W " W      X " w
    // 2  G C q      G C q      G C q
    // 3  = A =      = A =      = A =
    // 4  l U L      l U L      l U L
    // 5  . F .      . F .      . F .
    //
    // 3.7: use a different legend for the layout
    //      show quiver instead of repeating gloves on both sides;
    //      show secondary weapon in shield slot when two-weapon is active;
    //      show two-handed primary weapon in both shield and uwep slots;
    //      show lit lamp/lantern/candle/candelabrum on lower right side;
    //      show leash-in-use on lower left side

#ifdef ENHANCED_PAPERDOLL
    if (iflags.wc_ascii_map)
        qt_settings->doll_is_shown = false;
    if (!qt_settings->doll_is_shown)
        return;
    qt_settings->glyphs().setSize(qt_settings->dollWidth,
                                  qt_settings->dollHeight);
#endif

    QPainter painter;
    painter.begin(this);

    // Blanks
    drawWorn(painter, 0, 0, 5, false);
    drawWorn(painter, 0, 2, 5, false);
    if (u.twoweap) // empty alt weapon slot, show uswapwep in shield slot
        drawWorn(painter, 0, 0, 0, false);

    // TODO: render differently if known to be non-removable (known cursed)
    drawWorn(painter, uarm,     1, 3); // Armour
    drawWorn(painter, uarmc,    1, 2); // Cloak
    drawWorn(painter, uarmh,    1, 0); // Helmet
    // shield slot varies depending upon weapon usage
    if (u.twoweap)
        drawWorn(painter, uswapwep, 0, 1); // Secondary weapon, in use
    else if (uwep && bimanual(uwep))
        drawWorn(painter, uwep,     0, 1); // Two-handed weapon shown twice
    else
        drawWorn(painter, uarms,    0, 1); // Shield (might be blank)
    drawWorn(painter, uarmg,    0, 2); // Gloves
    drawWorn(painter, uarmf,    1, 5); // Shoes (feet)
    drawWorn(painter, uarmu,    1, 4); // Undershirt
    drawWorn(painter, uleft,    0, 3); // RingL
    drawWorn(painter, uright,   2, 3); // RingR

    drawWorn(painter, uwep,     2, 1); // Weapon
    drawWorn(painter, !u.twoweap ? uswapwep : NULL, 0, 0); // Alternate weapon
    drawWorn(painter, uquiver,  2, 2); // Quiver
    drawWorn(painter, uamul,    1, 1); // Amulet
    drawWorn(painter, ublindf,  2, 0); // Blindfold/Towel/Lenses

    // light source and leash aren't unique and don't have pointers defined
    drawWorn(painter, find_tool(LEASH),    0, 4);
    // OIL_LAMP matches lit candles, lamps, lantern, and candelabrum (and will
    // also duplicate Sunsword when it is wielded and shown in the uwep slot)
    drawWorn(painter, find_tool(OIL_LAMP), 2, 4);

    painter.end();

#ifdef ENHANCED_PAPERDOLL
    qt_settings->glyphs().setSize(qt_settings->tileWidth,
                                  qt_settings->tileHeight);
#endif
}

QSize NetHackQtInvUsageWindow::sizeHint(void) const
{
    if (qt_settings) {
        int w = 0, h = 0;
        // 1+X+1: one pixel border surrounding each tile in the paper doll,
        // so +1 left and +1 right, also +1 above and +1 below
#ifdef ENHANCED_PAPERDOLL
        if (iflags.wc_ascii_map)
            qt_settings->doll_is_shown = false;
        if (qt_settings->doll_is_shown) {
            w = (1 + qt_settings->dollWidth + 1) * 3;
            h = (1 + qt_settings->dollHeight + 1) * 6;
        }
#else
        if (iflags.wc_tiles_map) {
            w = (1 + qt_settings->glyphs().width() + 1) * 3;
            h = (1 + qt_settings->glyphs().height() + 1) * 6;
        }
#endif
        return QSize(w, h);
    } else {
	return QWidget::sizeHint();
    }
}

} // namespace nethack_qt_
