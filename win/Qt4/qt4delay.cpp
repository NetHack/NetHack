// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt4delay.cpp -- implement a delay

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
#include "qt4delay.h"

namespace nethack_qt4 {

// RLC Can we use QTimer::single_shot for this?
NetHackQtDelay::NetHackQtDelay(int ms) :
    msec(ms), m_timer(0), m_loop(this)
{
}

void NetHackQtDelay::wait()
{
    m_timer = startTimer(msec);
    m_loop.exec();
}

void NetHackQtDelay::timerEvent(QTimerEvent* timer)
{
    m_loop.exit();
    killTimer(m_timer);
    m_timer = 0;
}

} // namespace nethack_qt4
