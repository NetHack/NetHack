// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt4icon.cpp -- a labelled icon

#ifndef QT4ICON_H
#define QT4ICON_H

namespace nethack_qt4 {

class NetHackQtLabelledIcon : public QWidget {
public:
	NetHackQtLabelledIcon(QWidget* parent, const char* label);
	NetHackQtLabelledIcon(QWidget* parent, const char* label, const QPixmap& icon);

	enum { NoNum=-99999 };
	void setLabel(const QString&, bool lower=true); // a string
	void setLabel(const QString&, long, const QString& tail=""); // a number
	void setLabel(const QString&, long show_value, long comparative_value, const QString& tail="");
	void setIcon(const QPixmap&);
	virtual void setFont(const QFont&);

	void highlightWhenChanging();
	void lowIsGood();
	void dissipateHighlight();

	virtual void show();
	virtual QSize sizeHint() const;
	virtual QSize minimumSizeHint() const;

protected:
	void resizeEvent(QResizeEvent*);

private:
	void initHighlight();
	void setAlignments();
	void highlight(const QString& highlight);
	void unhighlight();

	bool low_is_good;
	int prev_value;
	int turn_count;		/* last time the value changed */
	QString hl_good;
	QString hl_bad;

	QLabel* label;
	QLabel* icon;
};

} // namespace nethack_qt4

#endif
