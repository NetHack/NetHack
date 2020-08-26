// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_key.h -- a key buffer

#ifndef QT4KEY_H
#define QT4KEY_H

namespace nethack_qt_ {

class NetHackQtKeyBuffer {
public:
	NetHackQtKeyBuffer();

	bool Empty() const;
	bool Full() const;

	void Put(int k, int ascii, uint state);
	void Put(char a);
	void Put(const char* str);
	int GetKey();
	int GetAscii();
	Qt::KeyboardModifiers GetState();

	int TopKey() const;
	int TopAscii() const;
	Qt::KeyboardModifiers TopState() const;

private:
	enum { maxkey=64 };
	int key[maxkey];
	int ascii[maxkey];
	Qt::KeyboardModifiers state[maxkey];
	int in,out;
};

} // namespace nethack_qt_

#endif
