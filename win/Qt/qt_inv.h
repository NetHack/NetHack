// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_inv.h -- inventory usage window
// This is at the top center of the main window

#ifndef QT4INV_H
#define QT4INV_H

namespace nethack_qt_ {

// for calls to drawWorn
enum drawWornFlag { dollNoFlag = 0, dollUnused = 1, dollReverse = 2 };

class NetHackQtInvUsageWindow : public QWidget {
public:
	NetHackQtInvUsageWindow(QWidget* parent);
	virtual ~NetHackQtInvUsageWindow();
	virtual void paintEvent(QPaintEvent*);
	virtual QSize sizeHint(void) const;

protected:
        virtual bool event(QEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);

private:
        void drawWorn(QPainter &painter, obj *nhobj, int x, int y,
                      const char *alttip, int flags = dollNoFlag);
        bool tooltip_event(QHelpEvent *tipevent);

        char *tips[3][6]; // PAPERDOLL is a grid of 3x6 cells for tiles
};

} // namespace nethack_qt_

#endif
