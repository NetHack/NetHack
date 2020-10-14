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
#include "qt_main.h"
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
    // needed to enable tool tips
    setMouseTracking(true);

    for (int x = 0; x <= 2; ++x)
        for (int y = 0; y <= 5; ++y)
            tips[x][y] = NULL;
}

NetHackQtInvUsageWindow::~NetHackQtInvUsageWindow()
{
    for (int x = 0; x <= 2; ++x)
        for (int y = 0; y <= 5; ++y)
            if (tips[x][y])
                free((void *) tips[x][y]), tips[x][y] = NULL;
}

void NetHackQtInvUsageWindow::drawWorn(QPainter &painter, obj *nhobj,
                                       int x, int y, // cell index, not pixels
                                       const char *alttip, bool canbe)
{
    short int glyph;
    int border;

    if (nhobj) {
        border = BORDER_DEFAULT;
#ifdef ENHANCED_PAPERDOLL
        // color margin around cell containing item whose BUC state is known
        if (Role_if('P') && !Blind)
            nhobj->bknown = 1;
        if (nhobj->bknown)
            border = nhobj->cursed ? BORDER_CURSED
                     : !nhobj->blessed ? BORDER_UNCURSED
                       : BORDER_BLESSED;

        // set up a tool tip describing the item that will be displayed here
        char *itmnam = xprname(nhobj, (char *) 0, nhobj->invlet, TRUE, 0L, 0L);
        if (tips[x][y] && strlen(itmnam) > strlen(tips[x][y]))
            free((void *) tips[x][y]), tips[x][y] = NULL;

        if (tips[x][y])
            Strcpy(tips[x][y], itmnam);
        else
            tips[x][y] = dupstr(itmnam);
#endif
        glyph = obj_to_glyph(nhobj, rn2_on_display_rng);
    } else {
        border = NO_BORDER;
#ifdef ENHANCED_PAPERDOLL
        // caller passes an alternative tool tip for empty cells
        if (tips[x][y] && (!alttip || strlen(alttip) > strlen(tips[x][y])))
            free((void *) tips[x][y]), tips[x][y] = NULL;

        if (tips[x][y]) // above guarantees that test fails if alttip is Null
            Strcpy(tips[x][y], alttip);
        else if (alttip)
            tips[x][y] = dupstr(alttip);
#else
        nhUse(alttip);
#endif
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
    // set glyphs() for the paperdoll; might be different size than map's
    qt_settings->glyphs().setSize(qt_settings->dollWidth,
                                  qt_settings->dollHeight);

    /* for drawWorn()'s use of obj->invlet */
    if (!flags.invlet_constant)
        reassign();
#endif

    QPainter painter;
    painter.begin(this);

    // String argument is for a tool tip when the object in question is Null.
    //
    //  left column
    /* uswapwep slot varies depending upon dual-wielding state;
       shown in shield slot when actively wielded, so uswapwep slot is empty
       then and an alternate tool tip is used to explain that emptiness */
    if (!u.twoweap)
        drawWorn(painter, uswapwep, 0, 0, "no alternate weapon");
    else
        drawWorn(painter, NULL,     0, 0, "secondary weapon is wielded");
    /* shield slot varies depending upon weapon usage;
       no alt tool tip is needed for first two cases because object will
       never be Null when the corresponding tests pass */
    if (u.twoweap)
        drawWorn(painter, uswapwep, 0, 1, NULL); // secondary weapon, in use
    else if (uwep && bimanual(uwep))
        drawWorn(painter, uwep,     0, 1, NULL); // two-handed uwep shown twice
    else
        drawWorn(painter, uarms,    0, 1, "no shield");
    drawWorn(painter, uarmg,        0, 2, "no gloves");
    drawWorn(painter, uleft,        0, 3, "no left ring");
    /* light source and leash aren't unique and don't have pointers defined */
    drawWorn(painter, find_tool(LEASH), 0, 4, "no leashes in use");
    drawWorn(painter, NULL,         0, 5, NULL, false); // always blank

    //  middle column; no unused slots
    drawWorn(painter, uarmh,   1, 0, "no helmet");
    drawWorn(painter, uamul,   1, 1, "no amulet");
    drawWorn(painter, uarmc,   1, 2, "no cloak");
    drawWorn(painter, uarm,    1, 3, "no suit");
    drawWorn(painter, uarmu,   1, 4, "no shirt");
    drawWorn(painter, uarmf,   1, 5, "no boots");

    //  right column
    drawWorn(painter, ublindf, 2, 0, "no eyewear"); // blindfold/towel/lenses
    drawWorn(painter, uwep,    2, 1, "no weapon");
    drawWorn(painter, uquiver, 2, 2, "nothing readied for firing"); // quiver
    drawWorn(painter, uright,  2, 3, "no right ring");
    /* OIL_LAMP matches lit candles, lamps, lantern, and candelabrum
       (and might also duplicate Sunsword when it is wielded--hence lit--
       depending upon whether another light source precedes it in invent) */
    drawWorn(painter, find_tool(OIL_LAMP), 2, 4, "no active light sources");
    drawWorn(painter, NULL,    2, 5, NULL, false); // always blank

    painter.end();

#ifdef ENHANCED_PAPERDOLL
    // reset glyphs() to the ones being used for the map
    qt_settings->glyphs().setSize(qt_settings->tileWidth,
                                  qt_settings->tileHeight);
#endif
}

QSize NetHackQtInvUsageWindow::sizeHint(void) const
{
    if (qt_settings) {
        int w = 0, h = 1; // one pixel margin at top
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
        if (iflags.wc_tiled_map) {
            w = (1 + qt_settings->glyphs().width() + 1) * 3;
            h = (1 + qt_settings->glyphs().height() + 1) * 6;
        }
#endif
        return QSize(w, h);
    } else {
	return QWidget::sizeHint();
    }
}

// ENHANCED_PAPERDOLL - called when a tool tip is triggered by hovering mouse
bool NetHackQtInvUsageWindow::tooltip_event(QHelpEvent *tipevent)
{
#ifdef ENHANCED_PAPERDOLL
    if (iflags.wc_ascii_map)
        qt_settings->doll_is_shown = false;
    if (!qt_settings->doll_is_shown) {
        tipevent->ignore();
        return false;
    }
    int wd = qt_settings->dollWidth,
        ht = qt_settings->dollHeight;

    // inverse of drawBorderedCell();
    int yoffset = 1,  // tiny extra margin at top
        ex = tipevent->pos().x(),
        ey = tipevent->pos().y() - yoffset,
        // KISS: treat 1-pixel margin around cells as part of enclosed cell
        cellx = ex / (wd + 2),
        celly = ey / (ht + 2);

    const char *tip = (cellx >= 0 && cellx <= 2 && celly >= 0 && celly <= 5)
                      ? tips[cellx][celly] : NULL;
    if (tip && *tip) {
        QToolTip::showText(tipevent->globalPos(), QString(tip));
    } else {
        QToolTip::hideText();
        tipevent->ignore();
    }
#else
    nhUse(tipevent);
#endif  /* ENHANCED_PAPERDOLL */
    return true;
}

// ENHANCED_PAPERDOLL - event handler is necessary to support tool tips
bool NetHackQtInvUsageWindow::event(QEvent *event)
{
#ifdef ENHANCED_PAPERDOLL
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *tipevent = static_cast <QHelpEvent *> (event);
        return tooltip_event(tipevent);
    }
#endif
    // with this routine intercepting events, we need to pass along
    // paint and mouse-press events to have them handled
    return QWidget::event(event);

}

// ENHANCED_PAPERDOLL - clicking on the PaperDoll runs #seeall ('*')
void NetHackQtInvUsageWindow::mousePressEvent(QMouseEvent *event UNUSED)
{
#ifdef ENHANCED_PAPERDOLL
    QWidget *main = NetHackQtBind::mainWidget();
    (static_cast <NetHackQtMainWindow *> (main))->FuncAsCommand(doprinuse);
#endif
}

} // namespace nethack_qt_
