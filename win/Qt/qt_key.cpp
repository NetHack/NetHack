// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_key.cpp -- a key buffer

extern "C" {
#include "hack.h"
}

#include "qt_pre.h"
#include <QtGui/QtGui>
#include "qt_post.h"
#include "qt_key.h"

namespace nethack_qt_ {

NetHackQtKeyBuffer::NetHackQtKeyBuffer() :
    in(0), out(0)
{
}

bool NetHackQtKeyBuffer::Empty() const { return in==out; }
bool NetHackQtKeyBuffer::Full() const { return (in+1)%maxkey==out; }

void NetHackQtKeyBuffer::Put(int k, int a, uint kbstate)
{
    //raw_printf("k:%3d a:%3d s:0x%08x", k, a, kbstate);
    if ( Full() ) return;	// Safety
    key[in] = k;
    ascii[in] = a;
    state[in] = (Qt::KeyboardModifiers) kbstate;
    in = (in + 1) % maxkey;
}

void NetHackQtKeyBuffer::Put(char a)
{
    Put(0, a, 0U);
}

void NetHackQtKeyBuffer::Put(const char* str)
{
    while (*str) Put(*str++);
}

int NetHackQtKeyBuffer::GetKey()
{
    if ( Empty() ) return 0;
    int r=TopKey();
    out=(out+1)%maxkey;
    return r;
}

int NetHackQtKeyBuffer::GetAscii()
{
    if ( Empty() ) return 0; // Safety
    int r=TopAscii();
    out=(out+1)%maxkey;
    return r;
}

Qt::KeyboardModifiers NetHackQtKeyBuffer::GetState()
{
    if ( Empty() ) return 0;
    Qt::KeyboardModifiers r=TopState();
    out=(out+1)%maxkey;
    return r;
}

int NetHackQtKeyBuffer::TopKey() const
{
    if ( Empty() ) return 0;
    return key[out];
}

int NetHackQtKeyBuffer::TopAscii() const
{
    if ( Empty() ) return 0;
    return ascii[out];
}

Qt::KeyboardModifiers NetHackQtKeyBuffer::TopState() const
{
    if ( Empty() ) return 0;
    return state[out];
}

} // namespace nethack_qt_
