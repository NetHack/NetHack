// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_menu.cpp -- a menu or text-list widget

#ifndef QT4MENU_H
#define QT4MENU_H

#include "qt_win.h"
#include "qt_rip.h"

// some menu fields aren't wide enough even though sized for measured text
#define MENU_WIDTH_SLOP 10 /* this should not be necessary */

namespace nethack_qt_ {

class NetHackQtTextListBox : public QListWidget {
public:
    NetHackQtTextListBox(QWidget* parent = NULL) : QListWidget(parent) { }

    int TotalWidth() const
    {
	int width = 0;
	QFontMetrics fm(font());
	for (int i = 0; i < count(); i++) {
	    int lwidth = fm.QFM_WIDTH(item(i)->text());
	    width = std::max(width, lwidth);
	}
	return width;
    }
    int TotalHeight() const
    {
	QFontMetrics fm(font());
	return fm.height() * count();
    }

    virtual QSize sizeHint() const;
};

class NetHackQtMenuListBox : public QTableWidget {
public:
    NetHackQtMenuListBox(QWidget* parent = NULL) : QTableWidget(parent) { }

    int TotalWidth() const;
    int TotalHeight() const;

    virtual QSize sizeHint() const;
};

class NetHackQtMenuWindow : public QDialog, public NetHackQtWindow {
	Q_OBJECT
public:
	NetHackQtMenuWindow(QWidget *parent = NULL);
	~NetHackQtMenuWindow();

	virtual QWidget* Widget();

	virtual void StartMenu(bool using_WIN_INVEN = false);
        virtual void AddMenu(int glyph, const ANY_P *identifier,
                             char ch, char gch, int attr,
                             const QString& str, unsigned itemflags);
	virtual void EndMenu(const QString& prompt);
	virtual int SelectMenu(int how, MENU_ITEM_P **menu_list);

        bool is_invent;         // using core's WIN_INVEN

public slots:
	void All();
	void ChooseNone();
	void Invert();
	void Search();

        void ToggleSelect(int row, bool alyready_checked);
        void TableCellClicked(int row, int col);
        void CheckboxClicked(bool on_off);

protected:
	virtual void keyPressEvent(QKeyEvent*);

private:
	struct MenuItem {
		MenuItem();
		~MenuItem();

		int glyph;
		ANY_P identifier;
		int attr;
		QString str;
                long count;
		char ch;
                char gch;
                bool selected;      // True if checkbox is set
                bool preselected;   // True if caller told us to set checkbox
                unsigned itemflags;
                unsigned color;

		bool Selectable() const { return identifier.a_void!=0; }
	};

	QVector<MenuItem> itemlist;

	int itemcount;
	int next_accel;

	QTableWidget* table;
	QPushButton* ok;
	QPushButton* cancel;
	QPushButton* all;
	QPushButton* none;
	QPushButton* invert;
	QPushButton* search;
	QLabel prompt;

	// Count replaces prompt while it is being input
	QString promptstr;
	QString countstr;
        long biggestcount;      // determines width of field #0
        int countdigits;        // number of digits to format biggestcount
        bool counting;          // in midst of entering a count
        bool searching;         // in midst of entering a search string
	void InputCount(char key);
	void ClearCount(void);

        int how;                // pick-none, pick-one, pick-any
        bool has_glyphs;        // at least one item specified a glyph

	bool isSelected(int row);
        long count(int row);

	void AddRow(int row, const MenuItem& mi);
	void WidenColumn(int column, int width);
        void PadMenuColumns(bool split_descr);
        void MenuResize();
        void UpdateCountColumn(long newcount);

        void ClearSearch();
};

class NetHackQtTextWindow : public QDialog, public NetHackQtWindow {
	Q_OBJECT
public:
	NetHackQtTextWindow(QWidget *parent = NULL);
	~NetHackQtTextWindow();

	virtual QWidget* Widget();

	virtual void Clear();
	virtual bool Destroy();
	virtual void Display(bool block);
	virtual void PutStr(int attr, const QString& text);
	virtual void UseRIP(int how, time_t when);

public slots:
	void Search();

private slots:
        void doDismiss();
	void doUpdate();

protected:
	virtual void keyPressEvent(QKeyEvent *);

private:
	bool use_rip;
	bool str_fixed;
        bool textsearching;

	QPushButton ok;
	QPushButton search;
	NetHackQtTextListBox* lines;
        char target[BUFSZ];

	NetHackQtRIP rip;
};

class NetHackQtMenuOrTextWindow : public NetHackQtWindow {
private:
	NetHackQtWindow* actual;
        QWidget *parent;

        static void MenuOrText_too_soon_warning(const char *);

public:
	NetHackQtMenuOrTextWindow(QWidget *parent = NULL);

	virtual QWidget* Widget();

	// Text
	virtual void Clear();
	virtual bool Destroy();
	virtual void Display(bool block);
	virtual void PutStr(int attr, const QString& text);

	// Menu
        virtual void StartMenu(bool using_WIN_INVENT = false);
        virtual void AddMenu(int glyph, const ANY_P *identifier,
                             char ch, char gch, int attr,
                             const QString& str, unsigned itemflags);
	virtual void EndMenu(const QString& prompt);
	virtual int SelectMenu(int how, MENU_ITEM_P **menu_list);

};

} // namespace nethack_qt_

#endif
