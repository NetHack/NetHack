// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt4inv.cpp -- inventory usage window
// This is at the top center of the main window

extern "C" {
#include "hack.h"
}
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
#include "qt4inv.h"
#include "qt4glyph.h"
#include "qt4set.h"

namespace nethack_qt4 {

NetHackQtInvUsageWindow::NetHackQtInvUsageWindow(QWidget* parent) :
    QWidget(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void NetHackQtInvUsageWindow::drawWorn(QPainter& painter, obj* nhobj, int x, int y, bool canbe)
{
    short int glyph;
    if (nhobj)
	glyph=obj_to_glyph(nhobj, rn2_on_display_rng);
    else if (canbe)
	glyph=cmap_to_glyph(S_room);
    else
	glyph=cmap_to_glyph(S_stone);

    qt_settings->glyphs().drawCell(painter,glyph,x,y);
}

void NetHackQtInvUsageWindow::paintEvent(QPaintEvent*)
{
    //  012
    //
    //0 WhB   
    //1 s"w   
    //2 gCg   
    //3 =A=   
    //4  T    
    //5  S    

    QPainter painter;
    painter.begin(this);

    // Blanks
    drawWorn(painter,0,0,4,false);
    drawWorn(painter,0,0,5,false);
    drawWorn(painter,0,2,4,false);
    drawWorn(painter,0,2,5,false);

    drawWorn(painter,uarm,1,3); // Armour
    drawWorn(painter,uarmc,1,2); // Cloak
    drawWorn(painter,uarmh,1,0); // Helmet
    drawWorn(painter,uarms,0,1); // Shield
    drawWorn(painter,uarmg,0,2); // Gloves - repeated
    drawWorn(painter,uarmg,2,2); // Gloves - repeated
    drawWorn(painter,uarmf,1,5); // Shoes (feet)
    drawWorn(painter,uarmu,1,4); // Undershirt
    drawWorn(painter,uleft,0,3); // RingL
    drawWorn(painter,uright,2,3); // RingR

    drawWorn(painter,uwep,2,1); // Weapon
    drawWorn(painter,uswapwep,0,0); // Secondary weapon
    drawWorn(painter,uamul,1,1); // Amulet
    drawWorn(painter,ublindf,2,0); // Blindfold

    painter.end();
}

QSize NetHackQtInvUsageWindow::sizeHint(void) const
{
    if (qt_settings) {
	return QSize(qt_settings->glyphs().width()*3,
		     qt_settings->glyphs().height()*6);
    } else {
	return QWidget::sizeHint();
    }
}

} // namespace nethack_qt4
