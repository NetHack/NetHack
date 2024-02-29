// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_yndlg.h -- yes/no dialog

#ifndef QT4YNDLG_H
#define QT4YNDLG_H

namespace nethack_qt_ {

class NetHackQtYnDialog : QDialog {
	Q_OBJECT
private:
	QString question;
	const char* choices;
	char def;
	char keypress;
        bool allow_count;
        QLineEdit *le;
        QPushButton *y_btn;

        // arbitrary size; might need to be more sophisticated someday
        char alt_answer[26 + 1], alt_result[26 + 1];

protected:
	virtual void keyPressEvent(QKeyEvent*);
        void AltChoice(char answer, char result);

private slots:
	void doneItem(int);

public:
	NetHackQtYnDialog(QWidget *,const QString&,const char*,char);

	char Exec();
};

} // namespace nethack_qt_

#endif
