// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_msg.h -- a message window

#ifndef QT4MSG_H
#define QT4MSG_H

#include "qt_win.h"

namespace nethack_qt_ {

class NetHackQtMapWindow2;

class NetHackQtMessageWindow : QObject, public NetHackQtWindow {
	Q_OBJECT
public:
	NetHackQtMessageWindow();
	~NetHackQtMessageWindow();

	virtual QWidget* Widget();
	virtual void Clear();
	virtual void Display(bool block);
        virtual const char *GetStr(bool init);
	virtual void PutStr(int attr, const QString& text);

	void Scroll(int dx, int dy);
        void ClearMessages();

	void setMap(NetHackQtMapWindow2*);

private:
	QListWidget* list;
	bool changed;
        int currgetmsg;
	NetHackQtMapWindow2* map;

private slots:
	void updateFont();
};

} // namespace nethack_qt_

#endif
