// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_icon.cpp -- a labelled icon

#ifndef QT4ICON_H
#define QT4ICON_H

namespace nethack_qt_ {

enum CompareMode {
    NoCompare = -1, BiggerIsBetter = 0,
    SmallerIsBetter = 1, NeitherIsBetter = 2
};

class NetHackQtLabelledIcon : public QWidget {
public:
        NetHackQtLabelledIcon(QWidget *parent, const char *label);
        NetHackQtLabelledIcon(QWidget *parent, const char *label,
                              const QPixmap &icon);

        enum { NoNum = -99999L };
        void setLabel(const QString &, bool lower=true); // string
        void setLabel(const QString &, long, const QString &tail=""); // number
        void setLabel(const QString &, long show_value,
                      long comparative_value, const QString &tail="");
        void setIcon(const QPixmap &, const QString &tooltip=NULL);
        virtual void setFont(const QFont &);
        //QString labelText() { return QString(this->label->text()); }

	void highlightWhenChanging();
        void setCompareMode(int newmode);
	void dissipateHighlight();
        void ForceResize();

	virtual void show();
	virtual QSize sizeHint() const;
	virtual QSize minimumSizeHint() const;

        QLabel *label;
        QLabel *icon;

protected:
	void resizeEvent(QResizeEvent*);

private:
	void initHighlight();
	void setAlignments();
	void highlight(const QString& highlight);
	void unhighlight();

        int comp_mode;          /* compareMode; default is BiggerIsBetter */
        long prev_value;
        long turn_count;        /* last time the value changed */

        QString hl_better;
        QString hl_worse;
        QString hl_changd;
};

} // namespace nethack_qt_

#endif
