// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_stat.h -- bindings between the Qt 4 interface and the main code

#ifndef QT4STAT_H
#define QT4STAT_H

#include "qt_win.h"
#include "qt_icon.h"

namespace nethack_qt_ {

class NetHackQtStatusWindow : QWidget, public NetHackQtWindow {
	Q_OBJECT
public:
	NetHackQtStatusWindow();

	virtual QWidget* Widget();

	virtual void Clear();
	virtual void Display(bool block);
	virtual void CursorTo(int x,int y);
	virtual void PutStr(int attr, const QString& text);

	void fadeHighlighting();

protected:
	virtual void mousePressEvent(QMouseEvent *event);
	//RLC void resizeEvent(QResizeEvent*);

private slots:
	void doUpdate();

private:
	enum { hilight_time=1 };

	QPixmap p_str;
	QPixmap p_dex;
	QPixmap p_con;
	QPixmap p_int;
	QPixmap p_wis;
	QPixmap p_cha;

	QPixmap p_chaotic;
	QPixmap p_neutral;
	QPixmap p_lawful;

	QPixmap p_satiated;
	QPixmap p_hungry;
	QPixmap p_encumber[5];

	QPixmap p_stoned;
	QPixmap p_slimed;
	QPixmap p_strngld;
	QPixmap p_sick_fp;
	QPixmap p_sick_il;
	QPixmap p_stunned;
	QPixmap p_confused;
	QPixmap p_hallu;
	QPixmap p_blind;
	QPixmap p_deaf;
	QPixmap p_lev;
	QPixmap p_fly;
	QPixmap p_ride;

	NetHackQtLabelledIcon name;
	NetHackQtLabelledIcon dlevel;

	NetHackQtLabelledIcon str;
	NetHackQtLabelledIcon dex;
	NetHackQtLabelledIcon con;
	NetHackQtLabelledIcon intel;
	NetHackQtLabelledIcon wis;
	NetHackQtLabelledIcon cha;

	NetHackQtLabelledIcon gold;
	NetHackQtLabelledIcon hp;
	NetHackQtLabelledIcon power;
	NetHackQtLabelledIcon ac;
        NetHackQtLabelledIcon level; // Xp level
        NetHackQtLabelledIcon exp;   // appended to Xp rather than separate
                                     // but still used to pad their line
        NetHackQtLabelledIcon align; // alignment is on Conditions line
                                     // because it has an icon above it
	NetHackQtLabelledIcon time;
	NetHackQtLabelledIcon score;

	NetHackQtLabelledIcon hunger;
	NetHackQtLabelledIcon encumber;

	NetHackQtLabelledIcon stoned;
	NetHackQtLabelledIcon slimed;
	NetHackQtLabelledIcon strngld;
	NetHackQtLabelledIcon sick_fp;
	NetHackQtLabelledIcon sick_il;
	NetHackQtLabelledIcon stunned;
	NetHackQtLabelledIcon confused;
	NetHackQtLabelledIcon hallu;
	NetHackQtLabelledIcon blind;
	NetHackQtLabelledIcon deaf;
	NetHackQtLabelledIcon lev;
	NetHackQtLabelledIcon fly;
	NetHackQtLabelledIcon ride;

        QLabel hpbar_health; // hit point bar, left half
        QLabel hpbar_injury; // hit point bar, right half

	QFrame hline1;
	QFrame hline2;
	QFrame hline3;

	int cursy;

	bool first_set;
        bool alreadyfullhp;

        QHBoxLayout *InitHitpointBar();
        void HitpointBar();
	void nullOut();
	void updateStats();
	void checkTurnEvents();
};

} // namespace nethack_qt_

#endif
