// Copyright (c) Warwick Allison, 1999.
// Nearby window copyright (c) Kestrel Gregorich-Trevor, 2021.
// NetHack may be freely redistributed.  See license for details.

// qt_msg.h -- a message window

#ifndef QT4NEARBY_H
#define QT4NEARBY_H

#include "qt_win.h"
namespace nethack_qt_ {

class NetHackQtMapWindow2;

class NetHackQtNearbyWindow : QScrollArea, public NetHackQtWindow {
	Q_OBJECT
public:
	NetHackQtNearbyWindow();
	~NetHackQtNearbyWindow();

	virtual void Update(int toggle);

	virtual QWidget* Widget();

	void Scroll(int dx, int dy);

	void setMap(NetHackQtMapWindow2*);

private:
        QListWidget *list;
        QScrollArea *scrollarea;
		int monsters;
	NetHackQtMapWindow2* map;

private slots:
	void updateFont();
};

} // namespace nethack_qt_

#endif