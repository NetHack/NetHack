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

// Allow changing 'statuslines:2' to 'statuslines:3' or vice versa
// while the game is running; deletes and re-creates the status window.
// [Used in qt_bind.cpp and qt_main.cpp, but not referenced in qt_stat.cpp.]
#define DYNAMIC_STATUSLINES

// NetHackQtBind::notify() doesn't see ^V on OSX
#ifdef MACOS
#define CTRL_V_HACK
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

	void fadeHighlighting(bool before_key);

        void FuncAsCommand(int (*func)(void));
        // this is unconditional in case qt_main.h comes before qt_set.h
        void resizePaperDoll(bool); // ENHANCED_PAPERDOLL
#ifdef DYNAMIC_STATUSLINES
        // called when 'statuslines' option has been changed
        NetHackQtWindow *redoStatus();
#endif

public slots:
	void doMenuItem(QAction *);
	void doQtSettings(bool);
	void doAbout(bool);
        void doQuit(bool);
	//RLC void doGuidebook(bool);
        void doKeys(const char *);
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
#ifdef CTRL_V_HACK
        void CtrlV();
#endif

private:
	void ShowIfReady();
        void AddToolButton(QToolBar *toolbar, QSignalMapper *sm,
                           const char *name, int (*func)(void), QPixmap xpm);

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
