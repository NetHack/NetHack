// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_menu.cpp -- a menu or text-list widget

extern "C" {
#include "hack.h"
}

#include "qt_pre.h"
#include <QtGui/QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QtWidgets>
#endif
#include "qt_post.h"
#include "qt_menu.h"
#include "qt_menu.moc"
#include "qt_glyph.h"
#include "qt_set.h"
#include "qt_streq.h"
#include "qt_str.h"

// temporary
extern "C" int qt_compact_mode;
// end temporary

extern "C" struct menucoloring *menu_colorings;

namespace nethack_qt_ {

// temporary
void centerOnMain( QWidget* w );
// end temporary

QSize NetHackQtTextListBox::sizeHint() const
{
    QScrollBar *hscroll = horizontalScrollBar();
    int hsize = hscroll ? hscroll->height() : 0;
    return QSize(TotalWidth()+hsize, TotalHeight()+hsize);
}

int NetHackQtMenuListBox::TotalWidth() const
{
    int width = 0;

    for (int col = 0; col < columnCount(); ++col) {
	width += columnWidth(col);
    }
    return width;
}

int NetHackQtMenuListBox::TotalHeight() const
{
    int height = 0;

    for (int row = 0; row < rowCount(); ++row) {
	height += rowHeight(row);
    }
    return height;
}

QSize NetHackQtMenuListBox::sizeHint() const
{
    QScrollBar *hscroll = horizontalScrollBar();
    int hsize = hscroll ? hscroll->height() : 0;
    return QSize(TotalWidth()+hsize, TotalHeight()+hsize);
}

// Table view columns:
// 
// [pick-count] [accel] [glyph] [string]
// 
// Maybe accel should be near string.  We'll see.
// pick-count normally blank.
//   double-clicking or click-on-count gives pop-up entry
// string is green when selected
//
NetHackQtMenuWindow::NetHackQtMenuWindow(QWidget *parent) :
    QDialog(parent),
    table(new NetHackQtMenuListBox()),
    prompt(0),
    counting(false)
{
    // setFont() was in SelectMenu(), in time to be rendered but too late
    // when measuring the width and height that will be needed
    QFont tablefont(qt_settings->normalFixedFont());
    table->setFont(tablefont);

    QGridLayout *grid = new QGridLayout();
    table->setColumnCount(5);
    table->setFrameStyle(QFrame::Panel|QFrame::Sunken);
    table->setLineWidth(2);
    table->setShowGrid(false);
    table->horizontalHeader()->hide();
    table->verticalHeader()->hide();

    ok=new QPushButton("Ok");
    connect(ok,SIGNAL(clicked()),this,SLOT(accept()));

    cancel=new QPushButton("Cancel");
    connect(cancel,SIGNAL(clicked()),this,SLOT(reject()));

    all=new QPushButton("All");
    connect(all,SIGNAL(clicked()),this,SLOT(All()));

    none=new QPushButton("None");
    connect(none,SIGNAL(clicked()),this,SLOT(ChooseNone()));

    invert=new QPushButton("Invert");
    connect(invert,SIGNAL(clicked()),this,SLOT(Invert()));

    search=new QPushButton("Search");
    connect(search,SIGNAL(clicked()),this,SLOT(Search()));

    QPoint pos(0,ok->height());
    move(pos);
    prompt.setParent(this,0);
    prompt.move(pos);

    grid->addWidget(ok, 0, 0);
    grid->addWidget(cancel, 0, 1);
    grid->addWidget(all, 0, 2);
    grid->addWidget(none, 0, 3);
    grid->addWidget(invert, 0, 4);
    grid->addWidget(search, 0, 5);
    grid->addWidget(&prompt, 1, 0, 1, 7);
    grid->addWidget(table, 2, 0, 1, 7);
    grid->setColumnStretch(6, 1);
    grid->setRowStretch(2, 1);
    setFocusPolicy(Qt::StrongFocus);
    table->setFocusPolicy(Qt::NoFocus);
    connect(table, SIGNAL(cellClicked(int,int)),
            this, SLOT(cellToggleSelect(int,int)));

    setLayout(grid);
}

NetHackQtMenuWindow::~NetHackQtMenuWindow()
{
}

QWidget* NetHackQtMenuWindow::Widget() { return this; }

void NetHackQtMenuWindow::StartMenu()
{
    table->setRowCount((itemcount=0));
    next_accel=0;
    has_glyphs=false;
}

NetHackQtMenuWindow::MenuItem::MenuItem() :
    str("")
{
}

NetHackQtMenuWindow::MenuItem::~MenuItem()
{
}

void NetHackQtMenuWindow::AddMenu(int glyph, const ANY_P* identifier,
	char ch, char gch, int attr, const QString& str, unsigned itemflags)
{
    bool presel = (itemflags & MENU_ITEMFLAGS_SELECTED) != 0;
    if (!ch && identifier->a_void!=0) {
	// Supply a keyboard accelerator.  Limited supply.
	static char accel[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	if (accel[next_accel]) {
	    ch=accel[next_accel++];
	}
    }

    if ((int)itemlist.size() < itemcount+1) {
	itemlist.resize(itemcount*4+10);
    }
    itemlist[itemcount].glyph=glyph;
    itemlist[itemcount].identifier=*identifier;
    itemlist[itemcount].ch=ch;
    itemlist[itemcount].gch=gch;
    itemlist[itemcount].attr=attr;
    itemlist[itemcount].str=str;
    itemlist[itemcount].selected=presel;
    itemlist[itemcount].itemflags=itemflags;
    itemlist[itemcount].count=-1;
    itemlist[itemcount].color = -1;
    // Display the boulder symbol correctly
    if (str.left(8) == "boulder\t") {
	int bracket = str.indexOf('[');
	if (bracket != -1) {
	    itemlist[itemcount].str = str.left(bracket+1)
		+ QChar(cp437(str.at(bracket+1).unicode()))
		+ str.mid(bracket+2);
	}
    }
    int mcolor, mattr;
    if (attr == 0
        && get_menu_coloring(str.toLatin1().constData(), &mcolor, &mattr)) {
	itemlist[itemcount].attr = mattr;
	itemlist[itemcount].color = mcolor;
    }
    ++itemcount;

    if (glyph!=NO_GLYPH) has_glyphs=true;
}

void NetHackQtMenuWindow::EndMenu(const QString& p)
{
    prompt.setText(p);
    promptstr = p;
}

int NetHackQtMenuWindow::SelectMenu(int h, MENU_ITEM_P **menu_list)
{
    table->setRowCount(itemcount);

    how=h;

    ok->setEnabled(how!=PICK_ONE); ok->setDefault(how!=PICK_ONE);
    cancel->setEnabled(true);
    all->setEnabled(how==PICK_ANY);
    none->setEnabled(how==PICK_ANY);
    invert->setEnabled(how==PICK_ANY);
    search->setEnabled(how!=PICK_NONE);

    setResult(-1);

    // Set contents of table
    QFontMetrics fm(table->font());
    for (int i = 0; i < 5; i++) {
	table->setColumnWidth(i, 0);
    }
    for (int i = 0; i < itemcount; i++) {
	AddRow(i, itemlist[i]);
    }

#define MENU_WIDTH_SLOP 10 /* this should not be necessary */
    // Determine column widths
    std::vector<int> col_widths;
    for (std::size_t i = 0; i < (size_t) itemlist.size(); ++i) {
	QStringList columns = itemlist[i].str.split("\t");
        if (!itemlist[i].Selectable() && columns.size() == 1) {
            // Nonselectable line with no column dividers.
            // Assume this is a section header
            // or ordinary text (^X feedback, for instance) rendered in
            // a menu because tty's paginated menus can be used to go
            // backward, unlike text windows which can only go forward.
            QTableWidgetItem *twi = table->item(i, 4);
            if (twi) {
                QString text = twi->text();
                WidenColumn(4, fm.width(text));
            }
            continue;
        }
	for (std::size_t j = 0U; j < (size_t) columns.size(); ++j) {
	    int w = fm.width(columns[j] + "  \t");
	    if (j >= col_widths.size()) {
		col_widths.push_back(w);
	    } else if (col_widths[j] < w) {
		col_widths[j] = w;
	    }
	}
    }

    // Pad each column to its column width
    for (std::size_t i = 0U; i < (size_t) itemlist.size(); ++i) {
	QTableWidgetItem *twi = table->item(i, 4);
	if (twi == NULL) { continue; }
	QString text = twi->text();
	QStringList columns = text.split("\t");
	for (std::size_t j = 0U; j+1U < (size_t) columns.size(); ++j) {
	    columns[j] += "\t";
	    int width = col_widths[j];
	    while (fm.width(columns[j]) < width) {
		columns[j] += "\t";
	    }
	}
	text = columns.join("");
	twi->setText(text);
	WidenColumn(4, fm.width(text) + MENU_WIDTH_SLOP);
    }

    // FIXME:  size for compact mode
    //resize(this->width(), parent()->height()*7/8);
    move(0, 0);
    adjustSize();
    centerOnMain(this);
    exec();
    int result=this->result();

    *menu_list=0;
    if (result>0 && how!=PICK_NONE) {
	if (how==PICK_ONE) {
	    int i;
	    for (i=0; i<itemcount && !isSelected(i); i++)
		;
	    if (i<itemcount) {
		*menu_list=(MENU_ITEM_P *)alloc(sizeof(MENU_ITEM_P)*1);
		(*menu_list)[0].item=itemlist[i].identifier;
		(*menu_list)[0].count=count(i);
		return 1;
	    } else {
		return 0;
	    }
	} else {
	    int selcount=0;
	    for (int i=0; i<itemcount; i++)
		if (isSelected(i)) selcount++;
	    if (selcount) {
		*menu_list=(MENU_ITEM_P *)alloc(sizeof(MENU_ITEM_P)*selcount);
		int j=0;
		for (int i=0; i<itemcount; i++) {
		    if (isSelected(i)) {
			(*menu_list)[j].item=itemlist[i].identifier;
			(*menu_list)[j].count=count(i);
			j++;
		    }
		}
		return selcount;
	    } else {
		return 0;
	    }
	}
    } else {
	return -1;
    }
}

void NetHackQtMenuWindow::AddRow(int row, const MenuItem& mi)
{
    static const QColor colors[] = {
	QColor(64, 64, 64),
	QColor(Qt::red),
	QColor(0, 191, 0),
	QColor(127, 127, 0),
	QColor(Qt::blue),
	QColor(Qt::magenta),
	QColor(Qt::cyan),
	QColor(Qt::gray),
	QColor(Qt::white),
	QColor(255, 127, 0),
	QColor(127, 255, 127),
	QColor(Qt::yellow),
	QColor(127, 127, 255),
	QColor(255, 127, 255),
	QColor(127, 255, 255),
	QColor(Qt::white)
    };
    QFontMetrics fm(table->font());
    QTableWidgetItem *twi;

    if (mi.Selectable() && how != PICK_NONE) {
	// Count
	twi = new QTableWidgetItem("");
	table->setItem(row, 0, twi);
	twi->setFlags(Qt::ItemIsEnabled);
	WidenColumn(0, fm.width("999999"));
	// Check box, set if selected
	QCheckBox *cb = new QCheckBox();
	cb->setChecked(mi.selected);
	cb->setFocusPolicy(Qt::NoFocus);
	if (how == PICK_ONE)
	    connect(cb, SIGNAL(clicked(bool)), this, SLOT(DoSelection(bool)));
	table->setCellWidget(row, 1, cb);
	WidenColumn(1, cb->width());
    }
    if (mi.glyph != NO_GLYPH) {
	// Icon
	QPixmap pm(qt_settings->glyphs().glyph(mi.glyph));
	twi = new QTableWidgetItem(QIcon(pm), "");
	table->setItem(row, 2, twi);
	twi->setFlags(Qt::ItemIsEnabled);
	WidenColumn(2, pm.width());
    }
    QString letter, text(mi.str);
    if (mi.ch != 0) {
	// Letter specified
	letter = QString(mi.ch) + " - ";
    }
    else {
	// Letter is left blank, except for skills display when # and * are
	// presented
	if (text.startsWith("    ")) {
	    // If mi.str starts with "    ", it's meant to line up with lines
	    // that have a letter; we don't want that here
	    text = text.mid(4);
	} else if (text.startsWith("   #") || text.startsWith("   *")) {
	    // Put the * or # in the letter column
	    letter = text.left(4);
	    text = text.mid(4);
	}
    }
    twi = new QTableWidgetItem(letter);
    table->setItem(row, 3, twi);
    table->item(row, 3)->setFlags(Qt::ItemIsEnabled);
    WidenColumn(3, fm.width(letter));
    twi = new QTableWidgetItem(text);
    table->setItem(row, 4, twi);
    table->item(row, 4)->setFlags(Qt::ItemIsEnabled);
    WidenColumn(4, fm.width(text));

    if ((int) mi.color != -1) {
	twi->setForeground(colors[mi.color]);
    }

    QFont itemfont(table->font());
    switch (mi.attr) {
    case ATR_BOLD:
	itemfont.setWeight(QFont::Bold);
	twi->setFont(itemfont);
	break;

    case ATR_DIM:
	twi->setFlags(Qt::NoItemFlags);
	break;

    case ATR_ULINE:
	itemfont.setUnderline(true);
	twi->setFont(itemfont);
	break;

    case ATR_INVERSE:
	{
	    QBrush fg = twi->foreground();
	    QBrush bg = twi->background();
	    if (fg == bg) {
		// default foreground and background come up the same for
		// some unknown reason
		twi->setForeground(Qt::white);
		twi->setBackground(Qt::black);
	    } else {
		twi->setForeground(bg);
		twi->setBackground(fg);
	    }
	}
	break;
    }
}

void NetHackQtMenuWindow::WidenColumn(int column, int width)
{
    // need to add a bit so the whole column displays
    width += 7;
    if (table->columnWidth(column) < width) {
	table->setColumnWidth(column, width);
    }
}

void NetHackQtMenuWindow::InputCount(char key)
{
    if (key == '\b')
    {
	if (counting)
	{
	    if (countstr.isEmpty())
		ClearCount();
	    else
		countstr = countstr.mid(0, countstr.size() - 1);
	}
    }
    else
    {
	counting = true;
	countstr += QChar(key);
    }
    if (counting)
	prompt.setText("Count: " + countstr);
}

void NetHackQtMenuWindow::ClearCount(void)
{
    counting = false;
    prompt.setText(promptstr);
    countstr = "";
}

void NetHackQtMenuWindow::keyPressEvent(QKeyEvent* event)
{
    QString text = event->text();

    const QChar *uni = text.unicode();
    for (unsigned k = 0; uni[k] != 0; k++) {
	unsigned key = uni[k].unicode();
	if (key=='\033') {
	    if (counting)
		ClearCount();
	    else
		reject();
	} else if (key=='\r' || key=='\n' || key==' ')
	    accept();
	else if (key==MENU_SEARCH)
	    Search();
	else if (key==MENU_SELECT_ALL || key==MENU_SELECT_PAGE)
	    All();
	else if (key==MENU_INVERT_ALL || key==MENU_INVERT_PAGE)
	    Invert();
	else if (key==MENU_UNSELECT_ALL || key==MENU_UNSELECT_PAGE)
	    ChooseNone();
	else if (('0' <= key && key <= '9') || key == '\b')
	    InputCount(key);
	else {
	    for (int i=0; i<itemcount; i++) {
		if ((unsigned int) itemlist[i].ch == key
                    || (unsigned int) itemlist[i].gch == key)
		    ToggleSelect(i);
	    }
	}
    }
}

void NetHackQtMenuWindow::All()
{
    if (how != PICK_ANY)
        return;

    bool didcheck = false;
    for (int i=0; i<itemcount; i++) {
	QTableWidgetItem *count = table->item(i, 0);
	if (count != NULL) count->setText("");

	QCheckBox *cb = dynamic_cast<QCheckBox *>(table->cellWidget(i, 1));
	if (cb != NULL) {
            cb->setChecked(true);
            didcheck = true;
        }
    }
    if (didcheck)
        table->repaint();
}
void NetHackQtMenuWindow::ChooseNone()
{
    if (how != PICK_ANY)
        return;

    bool diduncheck = false;
    for (int i=0; i<itemcount; i++) {
	QTableWidgetItem *count = table->item(i, 0);
	if (count != NULL) count->setText("");

	QCheckBox *cb = dynamic_cast<QCheckBox *>(table->cellWidget(i, 1));
	if (cb != NULL) {
            cb->setChecked(false);
            diduncheck = true;
        }
    }
    if (diduncheck)
        table->repaint();
}
void NetHackQtMenuWindow::Invert()
{
    if (how != PICK_ANY)
        return;

    boolean didtoggle = false;
    for (int i=0; i<itemcount; i++) {
        if (!menuitem_invert_test(0, itemlist[i].itemflags,
                                  itemlist[i].selected))
            continue;

	QTableWidgetItem *count = table->item(i, 0);
	if (count != NULL) count->setText("");

	QCheckBox *cb = dynamic_cast<QCheckBox *>(table->cellWidget(i, 1));
	if (cb != NULL) {
            cb->setChecked(cb->checkState() == Qt::Unchecked);
            didtoggle = true;
        }
    }
    if (didtoggle)
        table->repaint();
}
void NetHackQtMenuWindow::Search()
{
    if (how == PICK_NONE)
        return;

    NetHackQtStringRequestor requestor(this, "Search for:");
    char line[256];
    line[0] = '\0'; /* for EDIT_GETLIN */
    if (requestor.Get(line)) {
	for (int i=0; i<itemcount; i++) {
	    if (itemlist[i].str.contains(line))
		ToggleSelect(i);
	}
    }
}
void NetHackQtMenuWindow::ToggleSelect(int i)
{
    if (itemlist[i].Selectable()) {
	QCheckBox *cb = dynamic_cast<QCheckBox *>(table->cellWidget(i, 1));
	if (cb == NULL) return;

        cb->setChecked((counting && !countstr.isEmpty())
                       || cb->checkState() == Qt::Unchecked);

	QTableWidgetItem *count = table->item(i, 0);
	if (count != NULL) count->setText(countstr);

	ClearCount();

	if (how==PICK_ONE) {
	    accept();
        } else {
            table->repaint();
        }
    }
}

void NetHackQtMenuWindow::cellToggleSelect(int i, int j UNUSED)
{
    ToggleSelect(i);
}

void NetHackQtMenuWindow::DoSelection(bool)
{
    if (how == PICK_ONE) {
	accept();
    }
}

bool NetHackQtMenuWindow::isSelected(int row)
{
    QCheckBox *cb = dynamic_cast<QCheckBox *>(table->cellWidget(row, 1));
    return cb != NULL && cb->checkState() != Qt::Unchecked;
}

int NetHackQtMenuWindow::count(int row)
{
    QTableWidgetItem *count = table->item(row, 0);
    if (count == NULL) return -1;
    QString cstr = count->text();
    return cstr.isEmpty() ? -1 : cstr.toInt();
}

NetHackQtTextWindow::NetHackQtTextWindow(QWidget *parent) :
    QDialog(parent),
    use_rip(false),
    str_fixed(false),
    ok("Dismiss",this),
    search("Search",this),
    lines(new NetHackQtTextListBox(this)),
    rip(this)
{
    ok.setDefault(true);
    connect(&ok,SIGNAL(clicked()),this,SLOT(accept()));
    connect(&search,SIGNAL(clicked()),this,SLOT(Search()));
    connect(qt_settings,SIGNAL(fontChanged()),this,SLOT(doUpdate()));

    QVBoxLayout* vb = new QVBoxLayout(this);
    vb->addWidget(&rip);
    QHBoxLayout* hb = new QHBoxLayout();
    vb->addLayout(hb);
    hb->addWidget(&ok);
    hb->addWidget(&search);
    vb->addWidget(lines);
}

void NetHackQtTextWindow::doUpdate()
{
    update();
}


NetHackQtTextWindow::~NetHackQtTextWindow()
{

}

QWidget* NetHackQtTextWindow::Widget()
{
    return this;
}

bool NetHackQtTextWindow::Destroy()
{
    return !isVisible();
}

void NetHackQtTextWindow::UseRIP(int how, time_t when)
{
// Code from X11 windowport
#define STONE_LINE_LEN 16    /* # chars that fit on one line */
#define NAME_LINE 0	/* line # for player name */
#define GOLD_LINE 1	/* line # for amount of gold */
#define DEATH_LINE 2	/* line # for death description */
#define YEAR_LINE 6	/* line # for year */

static char** rip_line=0;
    if (!rip_line) {
	rip_line=new char*[YEAR_LINE+1];
	for (int i=0; i<YEAR_LINE+1; i++) {
	    rip_line[i]=new char[STONE_LINE_LEN+1];
	}
    }

    /* Follows same algorithm as genl_outrip() */

    char buf[BUFSZ];
    char *dpx;
    int line;

    /* Put name on stone */
    (void) snprintf(rip_line[NAME_LINE], STONE_LINE_LEN + 1,
                    "%.*s", STONE_LINE_LEN, g.plname);

    /* Put $ on stone;
       to keep things safe and relatively simple, impose an arbitrary
       upper limit that's the same for 64 bit and 32 bit configurations
       (also 16 bit configurations provided they use 32 bit long); the
       upper limit for directly carried gold is somewhat less than 300K
       due to carrying capacity, but end-of-game handling has already
       added in gold from containers, so the amount could be much more
       (simplest case: ~300K four times in a blessed bag of holding, so
       ~1.2M; in addition to the hassle of getting such a thing set up,
       it would need many gold-rich bones levels or wizard mode wishing) */
    long cash = std::max(g.done_money, 0L);
    /* force less that 10 digits to satisfy elaborate format checking;
       it's arbitrary but still way, way more than could ever be needed */
    if (cash > 999999999L)
        cash = 999999999L;
    (void) snprintf(rip_line[GOLD_LINE], STONE_LINE_LEN + 1, "%ld Au", cash);

    /* Put together death description */
    formatkiller(buf, sizeof buf, how, FALSE);
    //str_copy(buf, killer, SIZE(buf));

    /* Put death type on stone */
    for (line = DEATH_LINE, dpx = buf; line < YEAR_LINE; ++line) {
	char tmpchar;
	int i, i0 = (int) strlen(dpx);

	if (i0 > STONE_LINE_LEN) {
	    for (i = STONE_LINE_LEN; (i > 0) && (i0 > STONE_LINE_LEN); --i)
		if (dpx[i] == ' ')
                    i0 = i;
	    if (!i)
                i0 = STONE_LINE_LEN;
	}
	tmpchar = dpx[i0];
	dpx[i0] = 0;
	(void) str_copy(rip_line[line], dpx, STONE_LINE_LEN + 1);
	if (tmpchar != ' ') {
	    dpx[i0] = tmpchar;
	    dpx= &dpx[i0];
	} else {
            dpx= &dpx[i0 + 1];
        }
    }

    /* Put year on stone;
       64 bit configuration with 64 bit int is capable of overflowing
       STONE_LINE_LEN characters; a compiler might warn about that,
       so force a value that it can recognize as fitting within buffer's
       range ("%4d" imposes a minimum number of digits, not a maximum) */
    int year = (int) ((yyyymmdd(when) / 10000L) % 10000L); /* Y10K bug! */
    (void) snprintf(rip_line[YEAR_LINE], STONE_LINE_LEN + 1, "%4d", year);

    rip.setLines(rip_line, YEAR_LINE + 1);
    use_rip = true;
}

void NetHackQtTextWindow::Clear()
{
    lines->clear();
    use_rip=false;
    str_fixed=false;
}

void NetHackQtTextWindow::Display(bool block UNUSED)
{
    if (str_fixed) {
	lines->setFont(qt_settings->normalFixedFont());
    } else {
	lines->setFont(qt_settings->normalFont());
    }

    int h=0;
    if (use_rip) {
	h+=rip.height();
	ok.hide();
	search.hide();
	rip.show();
    } else {
	h+=ok.height()*2 + 7;
	ok.show();
	search.show();
	rip.hide();
    }
    int mh = QApplication::desktop()->height()*3/5;
    if ( (qt_compact_mode && lines->TotalHeight() > mh) || use_rip ) {
	// big, so make it fill
	showMaximized();
    } else {
	move(0, 0);
	adjustSize();
	centerOnMain(this);
	show();
    }
    exec();
}

void NetHackQtTextWindow::PutStr(int attr UNUSED, const QString& text)
{
    str_fixed=str_fixed || text.contains("    ");
    lines->addItem(text);
}

void NetHackQtTextWindow::Search()
{
    NetHackQtStringRequestor requestor(this, "Search for:");
    static char line[256]="";
    requestor.SetDefault(line);
    if (requestor.Get(line)) {
	int current=lines->currentRow();
	for (int i=1; i<lines->count(); i++) {
	    int lnum=(i+current)%lines->count();
	    QString str=lines->item(lnum)->text();
	    if (str.contains(line)) {
		lines->setCurrentRow(lnum);
		return;
	    }
	}
	lines->setCurrentItem(NULL);
    }
}

NetHackQtMenuOrTextWindow::NetHackQtMenuOrTextWindow(QWidget *parent_) :
    actual(0),
    parent(parent_)
{
}

QWidget* NetHackQtMenuOrTextWindow::Widget()
{
    if (!actual) impossible("Widget called before we know if Menu or Text");
    return actual->Widget();
}

// Text
void NetHackQtMenuOrTextWindow::Clear()
{
    if (!actual) impossible("Clear called before we know if Menu or Text");
    actual->Clear();
}
void NetHackQtMenuOrTextWindow::Display(bool block)
{
    if (!actual) impossible("Display called before we know if Menu or Text");
    actual->Display(block);
}
bool NetHackQtMenuOrTextWindow::Destroy()
{
    if (!actual) impossible("Destroy called before we know if Menu or Text");
    return actual->Destroy();
}

void NetHackQtMenuOrTextWindow::PutStr(int attr, const QString& text)
{
    if (!actual) actual=new NetHackQtTextWindow(parent);
    actual->PutStr(attr,text);
}

// Menu
void NetHackQtMenuOrTextWindow::StartMenu()
{
    if (!actual) actual=new NetHackQtMenuWindow(parent);
    actual->StartMenu();
}
void NetHackQtMenuOrTextWindow::AddMenu(int glyph, const ANY_P* identifier,
                                        char ch, char gch, int attr,
                                        const QString& str, unsigned itemflags)
{
    if (!actual) impossible("AddMenu called before we know if Menu or Text");
    actual->AddMenu(glyph,identifier,ch,gch,attr,str,itemflags);
}
void NetHackQtMenuOrTextWindow::EndMenu(const QString& prompt)
{
    if (!actual) impossible("EndMenu called before we know if Menu or Text");
    actual->EndMenu(prompt);
}
int NetHackQtMenuOrTextWindow::SelectMenu(int how, MENU_ITEM_P **menu_list)
{
    if (!actual) impossible("SelectMenu called before we know if Menu or Text");
    return actual->SelectMenu(how,menu_list);
}

} // namespace nethack_qt_
