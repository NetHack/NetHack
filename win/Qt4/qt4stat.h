// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt4stat.h -- bindings between the Qt 4 interface and the main code

#ifndef QT4STAT_H
#define QT4STAT_H

#include "qt4win.h"
#include "qt4icon.h"

namespace nethack_qt4 {

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

	QPixmap p_confused;
	QPixmap p_sick_fp;
	QPixmap p_sick_il;
	QPixmap p_blind;
	QPixmap p_stunned;
	QPixmap p_hallu;

	QPixmap p_encumber[5];

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
	NetHackQtLabelledIcon level;
	NetHackQtLabelledIcon exp;
	NetHackQtLabelledIcon align;

	NetHackQtLabelledIcon time;
	NetHackQtLabelledIcon score;

	NetHackQtLabelledIcon hunger;
	NetHackQtLabelledIcon confused;
	NetHackQtLabelledIcon sick_fp;
	NetHackQtLabelledIcon sick_il;
	NetHackQtLabelledIcon blind;
	NetHackQtLabelledIcon stunned;
	NetHackQtLabelledIcon hallu;
	NetHackQtLabelledIcon encumber;

	QFrame hline1;
	QFrame hline2;
	QFrame hline3;

	int cursy;

	bool first_set;

	void nullOut();
	void updateStats();
	void checkTurnEvents();
};

} // namespace nethack_qt4

#endif
