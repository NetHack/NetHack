// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_inv.cpp -- inventory subset of equipment in use,
//               displayed in a rectangular grid of object tiles
//
// Essentially a "paper doll" style display.  [grep fodder]
//
// This is at the top center of the main window, between messages and
// status.  Qt settings (non-OSX) or Preferences (OSX) has a checkbox to
// show it or hide it, plus the tile size to use (independent of map's
// tile size).  Supported tile size is 6..48x6..48 with default of 32x32.
//
// TODO?
// When yn_function() is asking for an inventory letter (not sure whether
// that is currently discernable...), allow clicking on a cell in the
// paper doll grid to return the invlet of the item clicked upon.
//

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

    for (o = gi.invent; o; o = o->nobj) {
        if ((tooltyp == LEASH && o->otyp == LEASH && o->leashmon)
            // OIL_LAMP is used for candles, lamps, lantern, candelabrum too
            || (tooltyp == OIL_LAMP && o->lamplit))
            break;
    }
    return o;
}

NetHackQtInvUsageWindow::NetHackQtInvUsageWindow(QWidget *parent) :
    QWidget(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    // needed to enable tool tips
    setMouseTracking(true);

    /*
     * TODO:
     *  Add support for clicking on a paperdoll cell and have that
     *  run itemactions(invent.c) for the object shown in the cell.
     */

    // paperdoll is 6x3 but the indices are column oriented: 0..2x0..5
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

void NetHackQtInvUsageWindow::drawWorn(
    QPainter &painter,
    obj *nhobj,
    int x, int y, // cell index, not pixels
    const char *alttip,
    int flags)
{
    short int glyph;
    glyph_info gi;
    int border;
    char tipstr[1 + BUFSZ + 1]; // extra room for leading and trailing space
    bool rev = (flags == dollReverse),
         canbe = (flags != dollUnused);

    if (nhobj) {
        border = BORDER_DEFAULT;
        // don't expect this to happen but check just in case;
        // learn_unseen_invent() is normally called when regaining sight
        // and sets dknown and maybe bknown, then updates perm_invent (do
        // it regardless of ENHANCED_PAPERDOLL for same effect either way)
        if (!Blind && (!nhobj->dknown
                       || (Role_if(PM_CLERIC) && !nhobj->bknown)))
            ::learn_unseen_invent();
#ifdef ENHANCED_PAPERDOLL
        // color margin around cell containing item whose BUC state is known
        if (nhobj->bknown)
            border = nhobj->cursed ? BORDER_CURSED
                     : !nhobj->blessed ? BORDER_UNCURSED
                       : BORDER_BLESSED;

        // border color is used to indicate BUC state; make tip text match
        boolean save_implicit_uncursed = ::flags.implicit_uncursed;
        ::flags.implicit_uncursed = FALSE;
        // set up a tool tip describing the item that will be displayed here
        Sprintf(tipstr, " %s ", // extra spaces for enhanced readability
                // xprname: invlet, space, dash, space, object description
                xprname(nhobj, (char *) NULL, nhobj->invlet, FALSE, 0L, 0L));
        ::flags.implicit_uncursed = save_implicit_uncursed;

        // tips are managed with nethack's alloc(); we don't track allocation
        // amount; allocated buffers get reused when big enough (usual case
        // since paperdoll updates occur more often than equipment changes)
        if (tips[x][y] && strlen(tipstr) > strlen(tips[x][y]))
            free((void *) tips[x][y]), tips[x][y] = NULL;

        if (tips[x][y])
            Strcpy(tips[x][y], tipstr); // guaranteed to fit
        else
            tips[x][y] = dupstr(tipstr);
#endif
        glyph = obj_to_glyph(nhobj, rn2_on_display_rng);
    } else {
        border = NO_BORDER;
#ifdef ENHANCED_PAPERDOLL
        // caller usually passes an alternate tool tip for empty cells
        size_t altlen = alttip ? 1U + strlen(alttip) + 1U : 0U;
        if (tips[x][y] && (!alttip || altlen > strlen(tips[x][y])))
            free((void *) tips[x][y]), tips[x][y] = NULL;

        if (alttip) {
            Sprintf(tipstr, " %s ", alttip);
            if (tips[x][y])
                Strcpy(tips[x][y], tipstr); // guaranteed to fit
            else
                tips[x][y] = dupstr(tipstr);
        }
#else
        nhUse(alttip);
#endif
        // an empty slot is shown as floor tile unless it's always empty
        glyph = canbe ? fn_cmap_to_glyph(S_room) : GLYPH_UNEXPLORED;
    }
    map_glyphinfo(0, 0, glyph, 0, &gi); /* this skirts the defined
                                         * interface unfortunately */
    qt_settings->glyphs().drawBorderedCell(painter, glyph, gi.gm.tileidx,
                                           x, y, border, rev);
}

// called to update the paper doll inventory subset
void NetHackQtInvUsageWindow::paintEvent(QPaintEvent*)
{
    // Paper doll is a 6 row by 3 column grid of worn and wielded
    // equipment showing the map tiles that the inventory objects
    // would be displayed as if they were on the floor.
    //
    //    0 1 2           two-    dual
    //   [ old ]  normal  hander  wielding             legend
    // 0 [x H b]  q H b   q H b   q H b     q quiver    H helmet  b eyewear
    // 1 [S " w]  w " S   W " W   w " X     w weapon    " amulet  S shield
    // 2 [G C G]  x C G   x C G   . C G     x alt-weap  C cloak   G gloves
    // 3 [= A =]  = A =   = A =   = A =     = right rg  A suit    = left ring
    // 4 [. U .]  L U l   L U l   L U l     L light     U shirt   l leash
    // 5 [. F .]  . F .   . F .   . F .     . blank     F boots   . blank
    //                                      W wielded two-handed weapon
    //                                      X wielded secondary weapon
    //
    // 3.7: use a different layout (also different legend for it, above):
    //      invert so grid is facing player, with right hand on screen left;
    //      show gloves in only one slot;
    //      move alternate weapon to former right hand glove slot;
    //      move blindfold to former alternate weapon slot;
    //      add quiver to former blindfold slot;
    //      show secondary weapon in shield slot when two-weapon is active;
    //      show two-handed primary weapon in both shield and uwep slots;
    //      add lit lamp/lantern/candle/candelabrum on lower left side;
    //      add leash-in-use on lower right side
    //
    // Actually indexed by grid[column][row].

#ifdef ENHANCED_PAPERDOLL
    if (qt_settings->doll_is_shown && ::iflags.wc_ascii_map
        && qt_settings->glyphs().no_tiles)
        qt_settings->doll_is_shown = false;
    if (!qt_settings->doll_is_shown)
        return;
    // set glyphs() for the paper doll; might be different size than map's
    qt_settings->glyphs().setSize(qt_settings->dollWidth,
                                  qt_settings->dollHeight);

    /* for drawWorn()'s use of obj->invlet */
    if (!::flags.invlet_constant)
        reassign();
#endif

    QPainter painter;
    painter.begin(this);

    // String argument is for a tool tip when the object in question is Null.
    //
    //  left grid column (depicts hero's right side)
    drawWorn(painter, uquiver, 0, 0, "nothing readied for firing"); // quiver
    drawWorn(painter, uwep,    0, 1, "no weapon");
    /* uswapwep slot varies depending upon dual-wielding state;
       shown in shield slot when actively wielded, so uswapwep slot is empty
       then and an alternate tool tip is used to explain that emptiness */
    if (!u.twoweap)
        drawWorn(painter, uswapwep, 0, 2, "no alternate weapon");
    else
        drawWorn(painter, NULL,     0, 2, "secondary weapon is wielded");
    drawWorn(painter, uright,  0, 3, "no right ring");
    /* OIL_LAMP matches lit candles, lamps, lantern, and candelabrum
       (and might also duplicate Sunsword when it is wielded--hence lit--
       depending upon whether another light source precedes it in invent) */
    drawWorn(painter, find_tool(OIL_LAMP), 0, 4, "no active light sources");
    drawWorn(painter, NULL,    0, 5, NULL, dollUnused); // always blank

    //  middle grid column; no unused slots
    drawWorn(painter, uarmh,   1, 0, "no helmet");
    drawWorn(painter, uamul,   1, 1, "no amulet");
    drawWorn(painter, uarmc,   1, 2, "no cloak");
    drawWorn(painter, uarm,    1, 3, "no suit");
    drawWorn(painter, uarmu,   1, 4, "no shirt");
    drawWorn(painter, uarmf,   1, 5, "no boots");

    //  right grid column (depicts hero's left side)
    drawWorn(painter, ublindf,      2, 0, "no eyewear"); // bf|towel|lenses
    /* shield slot varies depending upon weapon usage;
       no alt tool tip is needed for first two cases because object will
       never be Null when the corresponding tests pass */
    if (u.twoweap)
        drawWorn(painter, uswapwep, 2, 1, NULL); // secondary weapon, in use
    else if (uwep && bimanual(uwep)) // show two-handed uwep twice
        drawWorn(painter, uwep,     2, 1, NULL, dollReverse); // uwep on right
    else
        drawWorn(painter, uarms,    2, 1, "no shield");
    drawWorn(painter, uarmg,        2, 2, "no gloves");
    drawWorn(painter, uleft,        2, 3, "no left ring");
    /* light source and leash aren't unique and don't have pointers defined */
    drawWorn(painter, find_tool(LEASH), 2, 4, "no leashes in use");
    drawWorn(painter, NULL,         2, 5, NULL, dollUnused); // always blank

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
            w += (1 + qt_settings->dollWidth + 1) * 3;
            h += (1 + qt_settings->dollHeight + 1) * 6;
        }
#else
        if (iflags.wc_tiled_map) {
            w += (1 + qt_settings->glyphs().width() + 1) * 3;
            h += (1 + qt_settings->glyphs().height() + 1) * 6;
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
