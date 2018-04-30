// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt4rip.h -- tombstone window

#ifndef QT4RIP_H
#define QT4RIP_H

namespace nethack_qt4 {

class NetHackQtRIP : public QWidget {
private:
	static QPixmap* pixmap;
	char** line;
	int riplines;

public:
	NetHackQtRIP(QWidget* parent);

	void setLines(char** l, int n);

protected:
	virtual void paintEvent(QPaintEvent* event);
	QSize sizeHint() const;
};

} // namespace nethack_qt4

#endif
