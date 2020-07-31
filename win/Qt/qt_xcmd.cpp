// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_xcmd.cpp -- extended command widget

#include "hack.h"
#include "func_tab.h"
#undef Invisible
#undef Warning
#undef index
#undef msleep
#undef rindex
#undef wizard
#undef yn
#undef min
#undef max

#include "qt_pre.h"
#include <QtGui/QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QtWidgets>
#endif
#include "qt_post.h"
#include "qt_xcmd.h"
#include "qt_xcmd.moc"
#include "qt_bind.h"
#include "qt_set.h"
#include "qt_str.h"

namespace nethack_qt_ {

// temporary
void centerOnMain(QWidget *);
// end temporary

static inline bool
interesting_command(unsigned indx)
{
    return (!(extcmdlist[indx].flags & CMD_NOT_AVAILABLE)
            /* 'wizard' is #undef'd above [why?] so rely on its internals */
            && (flags.debug || !(extcmdlist[indx].flags & WIZMODECMD)));
}

NetHackQtExtCmdRequestor::NetHackQtExtCmdRequestor(QWidget *parent) :
    QDialog(parent)
{
    QVBoxLayout *l = new QVBoxLayout(this);

    QPushButton* can = new QPushButton("Cancel", this);
    can->setDefault(true);
    can->setMinimumSize(can->sizeHint());
    l->addWidget(can);

    prompt = new QLabel("#", this);
    l->addWidget(prompt);

    QButtonGroup *group=new QButtonGroup(this);
    QGroupBox *grid=new QGroupBox("Extended commands",this);
    l->addWidget(grid);

    unsigned i, j, ncmds = 0;
    int butw = 50;
    QFontMetrics fm = fontMetrics();
    for (i = 0; extcmdlist[i].ef_txt; ++i) {
        if (interesting_command(i)) {
            ++ncmds;
            butw = std::max(butw, 30 + fm.width(extcmdlist[i].ef_txt));
        }
    }

    /* 'ncols' should be calculated to fit (or enable a vertical scrollbar
       when resulting 'nrows' is too big, if GroupBox supports that);
       it used to be hardcoded 4 but after every command became accessible
       as an extended command, that resulted in so many rows that some of
       the buttoms were chopped off at the bottom of the grid */
    unsigned ncols = !flags.debug ? 6 : 8,
             nrows = (ncmds + ncols - 1) / ncols;
    /*
     * Choose grid layout.  This ought to selected via a button that can
     * be used to toggle the setting back and forth.
     *
     *  by row  vs  by column
     *   a b         a e
     *   c d         b f
     *   e f         c g
     *   g           d
     *
     * Prior to 3.7, it was always by-row, but by-column is more natural
     * for an alphabetized list.
     */
    bool by_column = true;

    QVBoxLayout* bl = new QVBoxLayout(grid);
    bl->addSpacing(fm.height());
    QGridLayout* gl = new QGridLayout();
    bl->addLayout(gl);
    for (i = j = 0; extcmdlist[i].ef_txt; ++i) {
        if (interesting_command(i)) {
            QPushButton *pb = new QPushButton(extcmdlist[i].ef_txt, grid);
            pb->setMinimumSize(butw, pb->sizeHint().height());
            group->addButton(pb, i + 1);
            if (by_column)
                /* 0..R-1 down first column, R..2*R-1 down second column,...*/
                gl->addWidget(pb, j % nrows, j / nrows);
            else
                /* 0..C-1 across first row, C..2*C-1 across second row, ... */
                gl->addWidget(pb, j / ncols, j % ncols);
            buttons.append(pb);
            ++j;
        }
    }
    group->addButton(can, 0);
    connect(group,SIGNAL(buttonPressed(int)),this,SLOT(done(int)));

    bl->activate();
    l->activate();
    resize(1,1);
}

void NetHackQtExtCmdRequestor::cancel()
{
    reject();
}

void NetHackQtExtCmdRequestor::keyPressEvent(QKeyEvent *event)
{
    QString text = event->text();
    if (text == "\r" || text == "\n" || text == " " || text == "\033")
    {
	reject();
    }
    else if (text == "\b" || text == "\177")
    {
	QString promptstr = prompt->text();
	if (promptstr != "#")
	    prompt->setText(promptstr.left(promptstr.size()-1));
        enableButtons();
    }
    else
    {
	QString promptstr = prompt->text() + text;
	QString typedstr = promptstr.mid(1); // skip the '#'
	unsigned matches = 0;
	unsigned match = 0;
	for (unsigned i=0; extcmdlist[i].ef_txt; i++) {
            if (!interesting_command(i))
                continue;
	    if (QString(extcmdlist[i].ef_txt).startsWith(typedstr)) {
		++matches;
		if (matches >= 2)
		    break;
		match = i;
	    }
	}
	if (matches == 1)
	    done(match+1);
	else if (matches >= 2)
	    prompt->setText(promptstr);
        enableButtons();
    }
}

int NetHackQtExtCmdRequestor::get()
{
    const int none = -10;
    resize(1,1); // pack
    centerOnMain(this);
    // Add any keys presently buffered to the prompt
    setResult(none);
    while (NetHackQtBind::qt_kbhit() && result() == none) {
	int ch = NetHackQtBind::qt_nhgetch();
	QKeyEvent event(QEvent::KeyPress, 0, Qt::NoModifier, QChar(ch));
	keyPressEvent(&event);
    }
    if (result() == none)
	exec();
    return result()-1;
}

/*
 * FIXME:
 *  This looks terrible.  [Possibly a difference between initial
 *  implementation using Qt2 and the current Qt version?]
 */
// Enable only buttons that match the current prompt string
void NetHackQtExtCmdRequestor::enableButtons()
{
    QString typedstr = prompt->text().mid(1); // skip the '#'
    std::size_t len = typedstr.size();

    for (auto b = buttons.begin(); b != buttons.end(); ++b) {
        (*b)->setVisible((*b)->text().left(len) == typedstr);
    }
}

} // namespace nethack_qt_
