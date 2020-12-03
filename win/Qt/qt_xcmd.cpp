// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_xcmd.cpp -- extended command widget
//
// TODO:
//  Add button that toggles the grid of command names from column-oriented
//    to row-oriented and vice versa.
//  Add another button to filter out commands that can be invoked by a
//    'normal' keystroke (not Meta) with current key bindings.
//  If not those, move the [cancel] button from being absurdly wide at the
//    top of the popup to being ordinary width, right justified on same
//    line as the prompt where user's typed characters are shown.

extern "C" {
#include "hack.h"
#include "func_tab.h"
}

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

extern uchar keyValue(QKeyEvent *key_event); // from qt_menu.cpp

// temporary
void centerOnMain(QWidget *);
// end temporary

static inline bool
interesting_command(unsigned indx)
{
    return (!(extcmdlist[indx].flags & CMD_NOT_AVAILABLE)
            /* 'wizard' is #undef'd above because Qt uses that token
               so rely on its internals */
            && (flags.debug || !(extcmdlist[indx].flags & WIZMODECMD)));
}

NetHackQtExtCmdRequestor::NetHackQtExtCmdRequestor(QWidget *parent) :
    QDialog(parent)
{
    QVBoxLayout *l = new QVBoxLayout(this);

    QPushButton *can = new QPushButton("Cancel", this);
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
       the buttons were chopped off at the bottom of the grid */
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

    QVBoxLayout *bl = new QVBoxLayout(grid);
    bl->addSpacing(fm.height());
    QGridLayout *gl = new QGridLayout();
    bl->addLayout(gl);
    for (i = j = 0; extcmdlist[i].ef_txt; ++i) {
        if (interesting_command(i)) {
            QPushButton *pb = new QPushButton(extcmdlist[i].ef_txt, grid);
            pb->setMinimumSize(butw, pb->sizeHint().height());
            // force the button to have fixed width or it can move around a
            // pixel or two (tiny but visibly noticeable) when enableButtons()
            // hides whole columns [see stretch comment below]
            pb->setMaximumSize(pb->minimumSize());
            group->addButton(pb, i + 1);
            /*
             * by_column:
             *  0..R-1 down first column, R..2*R-1 down second column, ...
             * otherwise:
             *  0..C-1 across first row, C..2*C-1 across second row, ...
             */
            int row = by_column ? j % nrows : j / ncols;
            int col = by_column ? j / nrows : j % ncols;
            gl->addWidget(pb, row, col);
            // these stretch settings prevent the grid from becoming very
            // ugly when enableButtons() disables whole rows and/or columns
            // as typed characters reduce the pool of possible matches
            if (row == 0)
                gl->setColumnStretch(col, 1);
            if (col == 0)
                gl->setRowStretch(row, 1);

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

#define Ctrl(c) (0x1f & (c)) /* ASCII */
// Note: we don't necessarily have access to a terminal to query
// it for user's preferred kill character, so use hardcoded ^U.
#define KILL_CHAR Ctrl('u')

void NetHackQtExtCmdRequestor::keyPressEvent(QKeyEvent *event)
{
    QString promptstr = prompt->text();
    uchar uc = keyValue(event);
    if (!uc) {
        // shift or control or meta, another character should be coming
        QWidget::keyPressEvent(event);
    } else if (uc == '\033' || uc == KILL_CHAR) {
        // Escape when some response is already present kills that text
        // but keeps prompting; escape when response is empty cancels.
        // Kill gets rid of current text, if any, and always re-prompts.
        if (uc == '\033' && promptstr == "#")
            reject(); // cancel() if ESC used when string is empty
        prompt->setText("#");
        enableButtons();
    } else if (uc == '\b' || uc == '\177') {
	if (promptstr != "#")
	    prompt->setText(promptstr.left(promptstr.size() - 1));
        enableButtons();
    } else if ((uc < ' ' && !(uc == '\n' || uc == '\r'))
               || uc > std::max('z', 'Z')) {
	reject(); // done()
    } else {
        // <return> is necessary if one command is a leading substring
        // of another and superfluous otherwise
        boolean checkexact = (uc == '\n' || uc == '\r' || uc == ' ');
        if (!checkexact)
            promptstr += QChar(uc); // event()->text()
	QString typedstr = promptstr.mid(1); // skip the '#'
	unsigned matches = 0;
	unsigned matchindx = 0;
	for (unsigned i=0; extcmdlist[i].ef_txt; i++) {
            if (!interesting_command(i))
                continue;
            QString cmdtxt = QString(extcmdlist[i].ef_txt);
            if (cmdtxt.startsWith(typedstr)) {
                if (checkexact) {
                    if (cmdtxt == typedstr) {
                        matchindx = i;
                        matches = 1;
                        break;
                    }
                } else {
                    if (++matches >= 2)
                        break;
                    matchindx = i;
                }
	    }
	}
	if (matches == 1)
            done(matchindx + 1);
        else if (checkexact)
            reject();
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

// Enable only buttons that match the current prompt string
void NetHackQtExtCmdRequestor::enableButtons()
{
    QString typedstr = prompt->text().mid(1); // skip the '#'
    std::size_t len = typedstr.size();

    // This used to look really bad when whole rows became empty:  the
    // grid shrank and the one line prompt area expanded to fill the
    // vacated vertical space.  Hiding whole columns looked bad too,
    // remaining buttons were widened to take the space.  Now the grid is
    // forced to have fixed layout (via stretch settings in constructor).
    for (auto b = buttons.begin(); b != buttons.end(); ++b) {
        bool showit = ((*b)->text().left(len) == typedstr);
        (*b)->setVisible(showit);
    }
}

} // namespace nethack_qt_
