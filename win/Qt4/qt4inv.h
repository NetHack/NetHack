// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt4inv.h -- inventory usage window
// This is at the top center of the main window

#ifndef QT4INV_H
#define QT4INV_H

namespace nethack_qt4 {

class NetHackQtInvUsageWindow : public QWidget {
public:
	NetHackQtInvUsageWindow(QWidget* parent);
	virtual void paintEvent(QPaintEvent*);
	virtual QSize sizeHint(void) const;

private:
	void drawWorn(QPainter& painter, obj*, int x, int y, bool canbe=true);
};

} // namespace nethack_qt4

#endif
