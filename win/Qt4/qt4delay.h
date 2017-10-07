// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt4delay.h -- implement a delay

#ifndef QT4DELAY_H
#define QT4DELAY_H

namespace nethack_qt4 {

class NetHackQtDelay : QObject {
private:
	int msec;
    int m_timer;
    QEventLoop m_loop;

public:
	NetHackQtDelay(int ms);
	void wait();
	virtual void timerEvent(QTimerEvent* timer);
};

} // namespace nethack_qt4

#endif
