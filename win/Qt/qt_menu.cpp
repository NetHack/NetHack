// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_menu.cpp -- a menu or text-list widget

//
// TODO:
//  search behaves weirdly unless you click in the line-edit dialog box
//    after clicking on the [search] button to pop that up;
//  implement next_page, prev_page, first_page, and last_page to work
//    like they do for X11:  scroll menu window as if it were paginated;
//  entering a count that uses more digits than the previous biggest count
//    widens count field but fails to widen the whole menu so a horizontal
//    scrollbar might appear;
//  create and use custom check-box icons to visually distinguish
//    pre-selected entries and numeric subset entries from ordinary select
//    and unselected.
//

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

/* for documentation rather than necessity; hack.h -> decl.h declares this */
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

// Table view columns (0..4):
// 
// [pick-count] [check-box] [glyph] [accel] [string]
// 
// pick-count is normally empty and gets wider as needed.
//
NetHackQtMenuWindow::NetHackQtMenuWindow(QWidget *parent) :
    QDialog(parent),
    table(new NetHackQtMenuListBox()),
    prompt(0),
    biggestcount(0L), // largest subset amount that user has entered
    countdigits(0),   // number of digits needed by biggestcount
    counting(false),  // user has typed a digit and more might follow
    searching(false)
{
    // setFont() was in SelectMenu(), in time to be rendered but too late
    // when measuring the width and height that will be needed
    QFont tablefont(qt_settings->normalFixedFont());
    table->setFont(tablefont);

    QGridLayout *grid = new QGridLayout();
    table->setColumnCount(5);
    table->setFrameStyle(QFrame::Panel|QFrame::Sunken);
    table->setLineWidth(2); // note: this is not row spacing
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
            this, SLOT(TableCellClicked(int,int)));

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

void NetHackQtMenuWindow::AddMenu(int glyph, const ANY_P *identifier,
                                  char ch, char gch, int attr,
                                  const QString& str, unsigned itemflags)
{
    bool presel = (itemflags & MENU_ITEMFLAGS_SELECTED) != 0;
    if (!ch && identifier->a_void!=0) {
	// Supply a keyboard accelerator.  Limited supply.
        static char accel[]
                    = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	if (accel[next_accel]) {
	    ch=accel[next_accel++];
	}
    }

    if ((int)itemlist.size() < itemcount+1) {
	itemlist.resize(itemcount*4+10);
    }
    itemlist[itemcount].glyph = glyph;
    itemlist[itemcount].identifier = *identifier;
    itemlist[itemcount].ch = ch;
    itemlist[itemcount].gch = gch;
    itemlist[itemcount].attr = attr;
    itemlist[itemcount].str = str;
    itemlist[itemcount].selected = itemlist[itemcount].preselected = presel;
    itemlist[itemcount].itemflags = itemflags;
    itemlist[itemcount].count = -1L;
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
    if (attr == 0 && ::iflags.use_menu_color
        && get_menu_coloring(str.toLatin1().constData(), &mcolor, &mattr)) {
	itemlist[itemcount].attr = mattr;
	itemlist[itemcount].color = mcolor;
    }
    ++itemcount;

    if (glyph != NO_GLYPH)
        has_glyphs = true;
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

    int presel_ct = 0;
    for (int i = 0; i < itemcount; ++i)
        if (itemlist[i].preselected)
            ++presel_ct;
    bool ok_ok = (how != PICK_ONE || presel_ct == 1);
    ok->setEnabled(ok_ok); ok->setDefault(ok_ok);
    cancel->setEnabled(true);
    all->setEnabled(how == PICK_ANY || (how == PICK_ONE && itemcount == 1));
    none->setEnabled(how == PICK_ANY || (how == PICK_ONE && presel_ct == 1));
    invert->setEnabled(how == PICK_ANY || (how == PICK_ONE && itemcount == 1));
    search->setEnabled(how != PICK_NONE);

    setResult(-1);

    // Set contents of table
    QFontMetrics fm(table->font());
    for (int col = 0; col < 5; ++col) {
        table->setColumnWidth(col, 0);
    }
    // default height is way too big; rows end up being double-spaced with it
    int rowheight = fm.height() * 3 / 5;
    for (int row = 0; row < itemcount; ++row) {
        AddRow(row, itemlist[row]);
        table->setRowHeight(row, rowheight);
    }
    PadMenuColumns(::iflags.menu_tab_sep ? true : false);

    //old FIXME:  size for compact mode
    //resize(this->width(), parent()->height()*7/8);
    move(0, 0);
    adjustSize();
    centerOnMain(this);

    exec();
    int result = this->result();

    *menu_list = (MENU_ITEM_P *) 0;
    if (result > 0 && how != PICK_NONE) {
        int n = 0;
        for (int i = 0; i < itemcount; ++i)
            if (itemlist[i].selected)
                ++n;
        if (n) {
            *menu_list = (MENU_ITEM_P *) alloc(n * sizeof(MENU_ITEM_P));
            n = 0;
            for (int i = 0; i < itemcount; ++i) {
                if (itemlist[i].selected) {
                    (*menu_list)[n].item = itemlist[i].identifier;
                    (*menu_list)[n].count = count(i);
                    ++n;
                }
            }
        }
        return n;
    } else {
	return -1;
    }
}

// pad the menu columns so that they all line up
void NetHackQtMenuWindow::PadMenuColumns(bool split_descr)
{
    QFontMetrics fm(table->font());
    QString col0width_str = "";
    if (biggestcount > 0L)
        col0width_str.sprintf("%*ld", std::max(countdigits, 1), biggestcount);
    int col0width_int = (int) fm.width(col0width_str) + MENU_WIDTH_SLOP;
    if (col0width_int > table->columnWidth(0))
	WidenColumn(0, col0width_int);

    // If the description (column 4) is a tab separated list, split
    // it into fields and figure out how wide each field should be.
    // Needs to be done at most once for any given menu instantiation.
    std::vector<int> col_widths(1, 0); // start with 1 element init'd to 0
    if (split_descr) {
        for (int row = 0; row < itemcount; ++row) {
            QTableWidgetItem *twi = table->item(row, 4); // description
            if (twi == NULL)
                continue;
            // if a header/footnote/&c with no sub-fields, don't inflate
            // the size of col_widths[0]
            if (!itemlist[row].Selectable()
                && !itemlist[row].str.contains(QChar('\t')))
                continue;
            // determine column widths of sub-fields within description
            QStringList columns = itemlist[row].str.split("\t");
            for (int fld = 0; fld < (int) columns.size(); ++fld) {
                bool lastcol = (fld == (int) columns.size() - 1);
                int w = fm.width(columns[fld] + (lastcol ? "" : "  "));
                if (fld >= (int) col_widths.size()) {
                    col_widths.push_back(w); // add another element
                } else if (col_widths[fld] < w) {
                    col_widths[fld] = w;
                }
            }
        }
    } // split_descr

    // Reformat all counts so that they line up right justified and
    // pad each field within description to fill that field's width.
    int widest4 = 0;
    for (int row = 0; row < (int) itemlist.size(); ++row) {
        // column 0 (subset count); format as right-justified number
        QTableWidgetItem *cnt = table->item(row, 0);
        if (cnt != NULL) {
            QString Amt = "";
            long amt = count(row); // fetch item(row,0) as a number
            if (amt > 0L)
                Amt.sprintf("%*ld", countdigits, amt);
            cnt->setText(Amt);
        }

        // column 4 (item description)
        QTableWidgetItem *twi = table->item(row, 4);
        if (twi == NULL)
            continue;
        QString text = twi->text();
        if (split_descr) {
            QStringList columns = text.split("\t");
            for (int fld = 0; fld < (int) columns.size() - 1; ++fld) {
                //columns[fld] += "\t"; /* (used to pad with tabs) */
                int width = col_widths[fld];
                while (fm.width(columns[fld]) < width)
                    columns[fld] += " "; //"\t";
            }
            text = columns.join("");
            twi->setText(text);
        }
        // TODO? if description needs to wrap, increase the height of this row
        int wid = fm.width(text) + MENU_WIDTH_SLOP;
        if (wid > widest4)
            widest4 = wid;
    }
    if (widest4 > table->columnWidth(4))
        WidenColumn(4, widest4);
}

// got a bigger count than previously; might need to widen column 0
// (or possibly had all counts removed and need to shrink column 0)
void NetHackQtMenuWindow::UpdateCountColumn(long newcount)
{
    if (newcount < 0L) {
        // this will happen if user clicks on [all],[none],[invert] buttons;
        // they clear all pending counts while selecting or unselecting
        biggestcount = 0L;
        countdigits = 0;
        table->setColumnWidth(0, 0);
        WidenColumn(0, MENU_WIDTH_SLOP);
    } else {
        biggestcount = std::max(biggestcount, newcount);
        QString num;
        num.sprintf("%*ld", std::max(countdigits, 1), biggestcount);
        int numlen = (int) num.length();
        if (numlen % 2)
            ++numlen;
        if (numlen <= countdigits)
            return;
        countdigits = numlen;
        // FIXME: neither adjusting the table size (below) nor the
        // menu widget size (also below) is making the menu widget
        // bigger after the count column has been expanded
    }

    PadMenuColumns(false);

    // Temporary? workaround for scrolling becoming wedged if using
    // all/none/invert removes all counts so we narrow a non-empty
    // count column to empty.  [That can take away the horizontal
    // scroll bar but should not be affecting the vertical one, yet
    // is (Qt 5.11.3).]  Typing any digit restored normal scrolling
    // and the only significant thing about that is that it updates
    // the prompt line which is outside the table of menu items where
    // scrolling takes place.  Oddly, both prompt changes are needed
    // (possibly the unnecessary space in the first is being optimized
    // away but the second call to remove it isn't aware of that).
    prompt.setText(promptstr + " ");
    prompt.setText(promptstr);

    // when this was just 'adjustSize()', our sizeHints() was not
    // being called so explicitly indicate the table widget
    table->adjustSize();
    this->adjustSize();
    table->repaint();
}

struct qcolor {
    QColor q;
    const char *nm;
};
// these match the tty colors, or better versions of same;
// [0] is used for black, and [8] (the first white) corresponds to "no color"
static const struct qcolor colors[] = {
    { QColor(64, 64, 64),    "64,64,64" },    // black
    { QColor(Qt::red),       "red" },
    { QColor(0, 191, 0),     "0,191,0" },     // green
    { QColor(127, 127, 0),   "127,127,0" },   // brownish
    { QColor(Qt::blue),      "blue" },
    { QColor(Qt::magenta),   "magenta" },
    { QColor(Qt::cyan),      "cyan" },
    { QColor(Qt::gray),      "gray" },
    // on tty, the "light" variations are "bright" instead; here they're paler
    { QColor(Qt::white),     "white" },       // no-color, so not rendered
    { QColor(255, 127, 0),   "255,127,0" },   // orange
    { QColor(127, 255, 127), "127,255,127" }, // light green
    { QColor(Qt::yellow),    "yellow" },
    { QColor(127, 127, 255), "127,127,255" }, // light blue
    { QColor(255, 127, 255), "255,127,255" }, // light magenta
    { QColor(127, 255, 255), "127,255,255" }, // light cyan
    { QColor(Qt::white),     "white" },
};

#if 0   /* available for debugging */
static const char *color_name(const QColor q)
{
    for (int i = 0; i < SIZE(colors); ++i)
        if (q == colors[i].q)
            return colors[i].nm;
    // these are all the enum GlobalColor values <qt5/QtCore/qnamespace.h>;
    // black and white have been moved in front of color0 and color1 here
    const char *nm = (q == Qt::black) ? "black"
                   : (q == Qt::white) ? "white"
                   : (q == Qt::color0) ? "color0" // doesn't duplicate white?
                   : (q == Qt::color1) ? "color1" // does duplicate black
                   : (q == Qt::darkGray) ? "darkGray"
                   : (q == Qt::gray) ? "gray"
                   : (q == Qt::lightGray) ? "lightGray"
                   : (q == Qt::red) ? "red"
                   : (q == Qt::green) ? "green"
                   : (q == Qt::blue) ? "blue"
                   : (q == Qt::cyan)  ? "cyan"
                   : (q == Qt::magenta) ? "magenta"
                   : (q == Qt::yellow) ? "yellow"
                   : (q == Qt::darkRed) ? "darkRed"
                   : (q == Qt::darkGreen) ? "darkGreen"
                   : (q == Qt::darkBlue) ? "darkBlue"
                   : (q == Qt::darkCyan) ? "darkCyan"
                   : (q == Qt::darkMagenta) ? "darkMagenta"
                   : (q == Qt::darkYellow) ? "darkYellow"
                   : (q == Qt::transparent) ? "transparent"
                   : "other";
    return nm;
}
#endif

void NetHackQtMenuWindow::AddRow(int row, const MenuItem& mi)
{
    QFontMetrics fm(table->font());
    QTableWidgetItem *twi;

    if (mi.Selectable() && how != PICK_NONE) {
	// Count
	twi = new QTableWidgetItem("");
	table->setItem(row, 0, twi);
	twi->setFlags(Qt::ItemIsEnabled);
#if 0   // active count field now widened as needed rather than preset
        WidenColumn(0, fm.width("999999") + MENU_WIDTH_SLOP);
#else
        WidenColumn(0, MENU_WIDTH_SLOP);
#endif

        // Check box, set if pre-selected
	QCheckBox *cb = new QCheckBox();
        cb->setChecked(mi.preselected ? Qt::Checked : Qt::Unchecked);
	cb->setFocusPolicy(Qt::NoFocus);
        // CheckboxClicked() will call ToggleSelect() for item whose checkbox
        // gets clicked upon
        connect(cb, SIGNAL(clicked(bool)), this, SLOT(CheckboxClicked(bool)));
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
    } else {
	// Letter is left blank, except for skills display when # and * are
	// presented (note: they're just displayed, not become selectors)
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
    // add extra padding because the measured width comes out too narrow;
    // for the normal case of "a - ", the trailing space hid the fact that
    // the column wasn't wide enough for four characters; for the "   #"
    // and "   *" cases, the last character was replaced by very tiny "..."
    int w = (int) fm.width(letter);
    if (w)
        w += MENU_WIDTH_SLOP / 2;
    WidenColumn(3, w);

    twi = new QTableWidgetItem(text);
    table->setItem(row, 4, twi);
    table->item(row, 4)->setFlags(Qt::ItemIsEnabled);
    WidenColumn(4, fm.width(text));

    if ((int) mi.color != -1) {
	twi->setForeground(colors[mi.color].q);
    }

    if (mi.attr != ATR_NONE) {
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
        case ATR_INVERSE: {
            QBrush fg = twi->foreground();
            QBrush bg = twi->background();
            if (fg.color() == bg.color()) {
                // default foreground and background come up the same for
                // some unknown reason
                //[pr: both are set to 'Qt::color1' which has same RGB
                //     value as 'Qt::black'; X11 on OSX behaves similarly]
                if (fg.color() == Qt::color1) {
                    fg = Qt::black;
                    bg = Qt::white;
                } else {
                    fg = (bg.color() == Qt::white) ? Qt::black : Qt::white;
                }
            }
            twi->setForeground(bg);
            twi->setBackground(fg);
            break;
        }
        case ATR_BLINK:
            // not supported
            break;
        } /* switch */
    } /* if mi.attr != ATR_NONE */
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
    if (key == '\b' || key == '\177') {
	if (counting) {
	    if (countstr.isEmpty())
		ClearCount();
	    else
		countstr = countstr.mid(0, countstr.size() - 1);
	}
    } else {
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

void NetHackQtMenuWindow::keyPressEvent(QKeyEvent *key_event)
{
    // key_event manipulation derived from NetHackQtBind::notify()
    const int k = key_event->key();
    Qt::KeyboardModifiers mod = key_event->modifiers();
    QChar ch = !key_event->text().isEmpty() ? key_event->text().at(0) : 0;
    if (ch > 128)
        ch = 0;
    // on OSX, ascii control codes are not sent, force them
    if (ch == 0 && (mod & Qt::ControlModifier) != 0) {
        if (k >= Qt::Key_A && k <= Qt::Key_Underscore)
            ch = (QChar) (k - (Qt::Key_A - 1));
    }
    uchar key = (char) ch.cell();

    // only one possible match for key==ch, and if one occurs it takes
    // precedence over any other match (for instance, some menus might
    // contain '-' to represent bare hands vs '-' for menu_unselect_page,
    // or ':' to look into a container vs ':' for menu_search)
    for (int row = 0; row < itemcount; ++row)
        if (key == (uchar) itemlist[row].ch) {
            ToggleSelect(row, false);
            return;
        }

    if (key == '\033') {
        if (counting)
            ClearCount();
        else if (searching)
            ClearSearch();
        else
            reject();
    } else if (key == '\r' || key == '\n' || key == ' ') {
        accept();
    } else if (('0' <= key && key <= '9') || key == '\b' || key == '\177') {
        InputCount(key);
    } else if (key == MENU_SELECT_ALL || key == MENU_SELECT_PAGE) {
        All();
    } else if (key == MENU_INVERT_ALL || key == MENU_INVERT_PAGE) {
        Invert();
    } else if (key == MENU_UNSELECT_ALL || key == MENU_UNSELECT_PAGE) {
        ChooseNone();
    } else if (key == MENU_SEARCH) {
        Search();
    } else {
        // multiple matches are possible with gch
        for (int row = 0; row < itemcount; ++row)
            if (key == (uchar) itemlist[row].gch)
                ToggleSelect(row, false);
    }
}

void NetHackQtMenuWindow::All()
{
    if (how != PICK_ANY && (how != PICK_ONE || itemcount != 1))
        return;

    if (counting)
        ClearCount(); // discard any pending count
    for (int row = 0; row < itemcount; ++row) {
        itemlist[row].selected = itemlist[row].preselected = false;
        if (!itemlist[row].Selectable())
            continue;
        itemlist[row].selected = true;

        QTableWidgetItem *count = table->item(row, 0);
        if (count != NULL) {
            count->setText("");
        }
        QCheckBox *cb = dynamic_cast<QCheckBox *> (table->cellWidget(row, 1));
        if (cb != NULL) {
            cb->setChecked(Qt::Checked);
        }
    }
    if (biggestcount > 0L) { // had one or more counts
        UpdateCountColumn(-1L); // all counts are now gone
    } else {
        table->repaint();
    }
}

void NetHackQtMenuWindow::ChooseNone()
{
    if (how == PICK_NONE)
        return;

    if (counting)
        ClearCount(); // discard any pending count
    for (int row = 0; row < itemcount; ++row) {
        itemlist[row].selected = itemlist[row].preselected = false;
        if (!itemlist[row].Selectable())
            continue;

        QTableWidgetItem *count = table->item(row, 0);
        if (count != NULL) {
            count->setText("");
        }
        QCheckBox *cb = dynamic_cast<QCheckBox *> (table->cellWidget(row, 1));
        if (cb != NULL) {
            cb->setChecked(Qt::Unchecked);
        }
    }
    if (biggestcount > 0L) { // had one or more counts
        UpdateCountColumn(-1L); // all counts are now gone
    } else {
        table->repaint();
    }
}

void NetHackQtMenuWindow::Invert()
{
    if (how == PICK_NONE)
        return;

    if (counting)
        ClearCount(); // discard any pending count
    for (int row = 0; row < itemcount; ++row) {
        if (!menuitem_invert_test(0, itemlist[row].itemflags,
                                  itemlist[row].preselected))
            continue;
        ToggleSelect(row, false);
    }
    if (biggestcount > 0L) { // had one or more counts
        UpdateCountColumn(-1L); // all counts are now gone
    } else {
        table->repaint();
    }
}

void NetHackQtMenuWindow::Search()
{
    if (how == PICK_NONE)
        return;

    searching = true;
    NetHackQtStringRequestor requestor(this, "Search for:");
    char line[BUFSZ];
    line[0] = '\0'; /* for EDIT_GETLIN */
    if (requestor.Get(line)) {
	for (int i=0; i<itemcount; i++) {
	    if (itemlist[i].str.contains(line))
		ToggleSelect(i, false);
	}
    }
    searching = false;
}

void NetHackQtMenuWindow::ClearSearch()
{
    searching = false;
}

void NetHackQtMenuWindow::ToggleSelect(int row, bool already_toggled)
{
    if (itemlist[row].Selectable()) {
        QCheckBox *cb = dynamic_cast<QCheckBox *> (table->cellWidget(row, 1));
        if (cb == NULL)
            return;

        if (how == PICK_ONE) {
            // explicitly picking a preselected item in a pick-one menu
            // chooses that item rather than toggling preselection off;
            // by clearing whole menu, the code below will select item #row
            ChooseNone();
            already_toggled = false;
            // FIXME?  this won't handle a pending count properly;
            // are there any pick-one menus with preselected choice
            // where a count is useful?
        } else {
            itemlist[row].preselected = false; // flag is now out-of-date
        }

        QTableWidgetItem *countfield = table->item(row, 0);
        if (!counting) {
            if (!already_toggled)
                cb->setChecked((cb->checkState() == Qt::Unchecked) // toggle
                               ? Qt::Checked : Qt::Unchecked);
            itemlist[row].selected = (cb->checkState() != Qt::Unchecked);
            if (countfield != NULL)
                countfield->setText("");
        } else {
            long amt = 1L;
            if (countfield != NULL) {
                countfield->setText(countstr); // store in item(row,0)
                amt = count(row); // fetch item(row,0) as a number
                QString Amt = "";
                if (amt > 0L)
                    Amt.sprintf("%*ld", countdigits, amt);
                countfield->setText(Amt); // store right-justified value
            }
            ClearCount(); // blank out 'countstr' and reset 'counting'

            // TODO: use a custom icon for check mark, like tty's '#' vs '+'
            // [maybe not necessary since unlike tty menus, count is visible]

            // uncheck if count is explicitly 0, otherwise check
            cb->setChecked((amt > 0L) ? Qt::Checked : Qt::Unchecked);
            itemlist[row].selected = (cb->checkState() != Qt::Unchecked);

            // if this count is larger than the biggest we've seen
            // so far, it might need more digits to render; if so,
            // all pending counts should be reformatted with new width
            if (amt > biggestcount)
                UpdateCountColumn(amt);
        }

        if (how == PICK_ONE && isSelected(row))
            accept();
        else
            table->repaint();
    }
}

// table cell click handler for cells (*,col) where col != 1
void NetHackQtMenuWindow::TableCellClicked(int row, int col UNUSED)
{
    ToggleSelect(row, false);
}

// checkbox click handler for table cells (*,1);
// presence of a checkbox in the clicked cell prevents the table cell click
// handler above from being called even if this handler doesn't get set up
void NetHackQtMenuWindow::CheckboxClicked(bool on_off UNUSED)
{
    // find which checkbox just got toggled by looking for one whose
    // check state doesn't match corresponding itemlist[].selected flag
    // [there's got to be a more direct way of achieving this...]
    for (int row = 0; row < itemcount; ++row) {
        // for pick-one menu, ToggleSelect() will return to menu's caller
        if (itemlist[row].Selectable()) {
            if (!isSelected(row) ^ !itemlist[row].selected) {
                // signal dispatcher has already checked or unchecked this box
                ToggleSelect(row, true);
                return;
            }
        }
    }
    // didn't find any changed checkbox; should never happen; what to do?
}

bool NetHackQtMenuWindow::isSelected(int row)
{
    QCheckBox *cb = dynamic_cast<QCheckBox *> (table->cellWidget(row, 1));
    return cb ? (cb->checkState() != Qt::Unchecked) : false;
}

// convert text count to numeric for final result
long NetHackQtMenuWindow::count(int row)
{
    QTableWidgetItem *count = table->item(row, 0);
    if (count == NULL)
        return -1L;
    QString cstr = count->text();
    return cstr.isEmpty() ? -1L : cstr.toLong();
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
