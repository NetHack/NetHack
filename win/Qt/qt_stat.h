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
        QPixmap p_blank2; // conditionally used for vertical spacing

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
        QPixmap p_vers; // version, when shown, is like a pseudo-condition

        /*
         *  Status fields, in display order (the three separator lines
         *  are exceptions).  Hitpoint bar is optionally displayed and
         *  contains two side-by-side parts; neither part is labelled.
         */
        QLabel hpbar_health;          // hit point bar, left half
        QLabel hpbar_injury;          // hit point bar, right half
        NetHackQtLabelledIcon name;   // (aka title) centered on its own row
        NetHackQtLabelledIcon dlevel; // (aka location) likewise

        /* the six characteristics; each is shown with a 40x40 icon above
           and a text label below, so implicitly two rows */
	NetHackQtLabelledIcon str;
	NetHackQtLabelledIcon dex;
	NetHackQtLabelledIcon con;
	NetHackQtLabelledIcon intel;
	NetHackQtLabelledIcon wis;
	NetHackQtLabelledIcon cha;

        /* five various status fields, some showing two values, shown as
           a row of text only; 'exp' used to be a separate field but is
           now displayed with 'level', with a blank field where it was so
           that there continue to be six columns which line up beneath the
           characteristics; gold used to be left-most but doesn't warrant
           that position; Xp or Xp/Exp is replaced by HD when polymorphed */
        NetHackQtLabelledIcon hp;     // current HP / maximum HP
        NetHackQtLabelledIcon power;  // current energy / maximum energy
        NetHackQtLabelledIcon ac;     // armor class
        NetHackQtLabelledIcon level;  // Xp level / Exp points (if 'showexp')
        NetHackQtLabelledIcon blank1; // pads the line to six columns
        NetHackQtLabelledIcon gold;   // used to come before HP

        /* next row:  two more fields, possibly blank; when present, each
           is sized as if for three fields, so their centered values line
           up with 2nd and 5th columns of the rows above */
        NetHackQtLabelledIcon time;   // moves counter (if 'time' is set)
        NetHackQtLabelledIcon score;  // tentative score (if compiled with
                                      // SCORE_ON_BOTL and 'showscore' is set)

        /* last rows:  alignment and zero or more status conditions;
           like the characteristics, they are shown as if in two rows with
           a 40x40 icon above and text lebel below; blank values are omitted
           and non-blank values are left justified */
        NetHackQtLabelledIcon align;    // w/ alignment-specific ankh icon
        NetHackQtLabelledIcon blank2;   // used for spacing if Align is moved
        NetHackQtLabelledIcon hunger;   // blank if 'normal'
        NetHackQtLabelledIcon encumber; // blank if 'unencumbered' ('normal')
        /* zero or more status conditions; in major, minor, 'other' order */
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
        /* to right of conditions, right justified */
        NetHackQtLabelledIcon vers;   // version

        QFrame hline1; // between dlevel and characteristics
        QFrame hline2; // between characteristics and regular status fields
        QFrame hline3; // between regular fields and time,score or conditions
        QFrame vline1; // for statuslines:2, between Cha and Alignment
        QFrame vline2; // for statuslines:2, padding between Gold and Time

	int cursy;
	bool first_set;
        bool alreadyfullhp;

        // for some fields, we need to know more than just "changed since
        // last update"; there's no 'had_time' because Time isn't highlighted
        bool was_polyd;
        bool had_exp;
        bool had_score;
        unsigned prev_versinfo;

        QHBoxLayout *InitHitpointBar();
        void HitpointBar();
	void nullOut();
	void updateStats();
	void checkTurnEvents();
};

} // namespace nethack_qt_

#endif
