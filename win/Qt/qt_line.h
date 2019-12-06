// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_line.h -- a one line input window

#ifndef QT4LINE_H
#define QT4LINE_H

namespace nethack_qt_ {

class NetHackQtLineEdit : public QLineEdit {
public:
	NetHackQtLineEdit();
	NetHackQtLineEdit(QWidget* parent, const char* name);

	void fakeEvent(int key, int ascii, Qt::KeyboardModifiers state);
};

} // namespace nethack_qt_

#endif
