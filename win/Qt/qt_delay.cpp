// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_delay.cpp -- implement a delay

extern "C" {
#include "hack.h"
}

#include "qt_pre.h"
#include <QtGui/QtGui>
#include "qt_post.h"
#include "qt_delay.h"

namespace nethack_qt_ {

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

void NetHackQtDelay::timerEvent(QTimerEvent* timer UNUSED)
{
    m_loop.exit();
    killTimer(m_timer);
    m_timer = 0;
}

} // namespace nethack_qt_
