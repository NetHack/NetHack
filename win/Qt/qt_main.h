// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_main.h -- the main window

#ifndef QT4MAIN_H
#define QT4MAIN_H

#ifdef KDE
#include <kapp.h>
#include <ktopwidget.h>
#else
#include "qt_kde0.h"
#endif

namespace nethack_qt_ {

class NetHackQtInvUsageWindow;
class NetHackQtKeyBuffer;
class NetHackQtMapWindow2;
class NetHackQtMessageWindow;
class NetHackQtStatusWindow;
class NetHackQtWindow;

// This class is the main widget for NetHack
//
// It is a collection of Message, Map, and Status windows.  In the current
// version of nethack there is only one of each, and this class makes this
// assumption, not showing itself until all are inserted.
//
// This class simply knows how to layout such children sensibly.
//
// Since it is only responsible for layout, the class does not
// note the actual class of the windows.
//

class NetHackQtMainWindow : public KTopLevelWidget {
	Q_OBJECT
public:
	NetHackQtMainWindow(NetHackQtKeyBuffer&);

	void AddMessageWindow(NetHackQtMessageWindow* window);
	NetHackQtMessageWindow * GetMessageWindow();
	void AddMapWindow(NetHackQtMapWindow2* window);
	void AddStatusWindow(NetHackQtStatusWindow* window);
	void RemoveWindow(NetHackQtWindow* window);
	void updateInventory();

	void fadeHighlighting();

        // this is unconditional in case qt_main.h comes before qt_set.h
        void resizePaperDoll(bool); // ENHANCED_PAPERDOLL

public slots:
	void doMenuItem(QAction *);
	void doQtSettings(bool);
	void doAbout(bool);
        void doQuit(bool);
	//RLC void doGuidebook(bool);
	void doKeys(const QString&);

protected:
	virtual void resizeEvent(QResizeEvent*);
	virtual void keyPressEvent(QKeyEvent*);
	virtual void keyReleaseEvent(QKeyEvent* event);
	virtual void closeEvent(QCloseEvent*);

private slots:
	void layout();
	void raiseMap();
	void zoomMap();
	void raiseMessages();
	void raiseStatus();

private:
	void ShowIfReady();

#ifdef KDE
	KMenuBar* menubar;
#else
	QMenuBar* menubar;
#endif
	NetHackQtMessageWindow* message;
	NetHackQtMapWindow2* map;
	NetHackQtStatusWindow* status;
	NetHackQtInvUsageWindow* invusage;

	QSplitter *hsplitter;
	QSplitter *vsplitter;

	NetHackQtKeyBuffer& keysink;
	QStackedWidget* stack;
	int dirkey;
};

} // namespace nethack_qt_

#endif
