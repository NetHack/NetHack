// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_plsel.h -- player selector dialog

#ifndef QT4PLSEL_H
#define QT4PLSEL_H

namespace nethack_qt_ {

class NetHackQtKeyBuffer;

class NetHackQtPlayerSelector : private QDialog {
	Q_OBJECT
public:
	enum { R_None=-1, R_Quit=-2, R_Rand=-3 };

	NetHackQtPlayerSelector(NetHackQtKeyBuffer&);

public slots:
	void Quit();
	void Random();
        void Randomize();
        void plnamePlayVsQuit();

        void populate_roles();
        void populate_races();

	void selectName(const QString& n);
	void selectRole(int current, int, int previous, int);
	void selectRace(int current, int, int previous, int);
	void setupOthers();
	void selectGender(int);
	void selectAlignment(int);

public:
	bool Choose();

private:
	QTableWidget* role;
	QTableWidget* race;
	QRadioButton **gender;
	QRadioButton **alignment;

	bool fully_specified_role;
        int chosen_gend;
        int chosen_align;
        int cleric_role_row;
        int human_race_row;

        QPushButton *rand_btn;
        QPushButton *play_btn;
        QPushButton *quit_btn;
};

} // namespace nethack_qt_

#endif
