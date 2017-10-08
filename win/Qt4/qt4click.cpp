// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt4click.cpp -- a mouse click buffer

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
#include "qt4click.h"

namespace nethack_qt4 {

NetHackQtClickBuffer::NetHackQtClickBuffer() :
    in(0), out(0)
{
}

bool NetHackQtClickBuffer::Empty() const { return in==out; }
bool NetHackQtClickBuffer::Full() const { return (in+1)%maxclick==out; }

void NetHackQtClickBuffer::Put(int x, int y, int mod)
{
    click[in].x=x;
    click[in].y=y;
    click[in].mod=mod;
    in=(in+1)%maxclick;
}

int NetHackQtClickBuffer::NextX() const { return click[out].x; }
int NetHackQtClickBuffer::NextY() const { return click[out].y; }
int NetHackQtClickBuffer::NextMod() const { return click[out].mod; }

void NetHackQtClickBuffer::Get()
{
    out=(out+1)%maxclick;
}

} // namespace nethack_qt4
