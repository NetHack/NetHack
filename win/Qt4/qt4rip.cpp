// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt4rip.cpp -- tombstone window

#include "hack.h"
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
#include "qt4rip.h"
#include "qt4bind.h"
#include "qt4str.h"

namespace nethack_qt4 {

QPixmap* NetHackQtRIP::pixmap=0;

// Debian uses a separate PIXMAPDIR
#ifndef PIXMAPDIR
# ifdef HACKDIR
#  define PIXMAPDIR HACKDIR
# else
#  define PIXMAPDIR "."
# endif
#endif

static void
tryload(QPixmap& pm, const char* fn)
{
    if (!pm.load(fn)) {
	QString msg;
	msg.sprintf("Cannot load \"%s\"", fn);
	QMessageBox::warning(NetHackQtBind::mainWidget(), "IO Error", msg);
    }
}

NetHackQtRIP::NetHackQtRIP(QWidget* parent) :
    QWidget(parent)
{
    if (!pixmap) {
	pixmap=new QPixmap;
	tryload(*pixmap, PIXMAPDIR "/rip.xpm");
    }
    riplines=0;
    resize(pixmap->width(),pixmap->height());
    setFont(QFont("times",12)); // XXX may need to be configurable
}

void NetHackQtRIP::setLines(char** l, int n)
{
    line=l;
    riplines=n;
}

QSize NetHackQtRIP::sizeHint() const
{
    return pixmap->size();
}

void NetHackQtRIP::paintEvent(QPaintEvent* event)
{
    if ( riplines ) {
	int pix_x=(width()-pixmap->width())/2;
	int pix_y=(height()-pixmap->height())/2;

	// XXX positions based on RIP image
	int rip_text_x=pix_x+156;
	int rip_text_y=pix_y+67;
	int rip_text_h=94/riplines;

	QPainter painter;
	painter.begin(this);
	painter.drawPixmap(pix_x,pix_y,*pixmap);
	for (int i=0; i<riplines; i++) {
	    painter.drawText(rip_text_x-i/2,rip_text_y+i*rip_text_h,
		1,1,Qt::TextDontClip|Qt::AlignHCenter,line[i]);
	}
	painter.end();
    }
}

} // namespace nethack_qt4
