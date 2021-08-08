// Copyright (c) Warwick Allison, 1999
// Inventory menu code (c) Kestrel Gregorich-Trevor, 2021.
// NetHack may be freely redistributed. See license for details.

// qt_nearby.cpp -- a nearby characters window

extern "C" {
#include "hack.h"
}

#include "qt_pre.h"
#include <QtGui/QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QtWidgets>
#endif
#include "qt_post.h"
#include "qt_nearby.h"
#include "qt_nearby.moc"
#include "qt_map.h"
#include "qt_set.h"
#include "qt_str.h"


namespace nethack_qt_ {

NetHackQtNearbyWindow::NetHackQtNearbyWindow() :
    list(new QListWidget()),
    scrollarea(new QScrollArea())
{
    list->setFocusPolicy(Qt::NoFocus);
    scrollarea->setFocusPolicy(Qt::NoFocus);
    scrollarea->takeWidget();
    ::iflags.window_inited = 1;
    map = 0;
    currgetmsg = 0;
    connect(qt_settings,SIGNAL(fontChanged()),this,SLOT(updateFont()));
    updateFont();
}

NetHackQtNearbyWindow::~NetHackQtNearbyWindow()
{
    ::iflags.window_inited = 0;
    delete list;
}

QWidget* NetHackQtNearbyWindow::Widget() {
    return list;
}

void NetHackQtNearbyWindow::setMap(NetHackQtMapWindow2* m)
{
    map = m;
    updateFont();
}

void NetHackQtNearbyWindow::updateFont()
{
    list->setFont(qt_settings->normalFont());
    if ( map )
	map->setFont(qt_settings->normalFont());
}

void NetHackQtNearbyWindow::Scroll(int dx UNUSED, int dy UNUSED)
{
    //RLC Is this necessary?
    //RLC list->Scroll(dx,dy);
}

void NetHackQtNearbyWindow::Display()
{
    if (changed) {
	list->repaint();
	changed=false;
    }
}

void NetHackQtNearbyWindow::Update()
{
}

}