// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_xcmd.cpp -- extended command widget
//
// TODO:
//  Either disable [Layout] when prompt has a partial response, or
//    preserve that partial response across widget tear-down/rebuild.
//  Maybe make the number of grid columns user settable?  Or a way to
//    specify a different font (smaller might be necessary for some folks;
//    the font set up in "Qt settings" or "Preferences" applies to the
//    message and status windows but not to extended command choosing).
//

//
// Widget appearance (excluding window title bar):
// +----------------------------------------------------+
// | [Cancel]                 [Filter] [Layout] [Reset] |  control buttons
// |#                 Extended commands                 |  text entry & title
// +----------------------------------------------------+
// | [cmmnd_1] [cmnd_14]    ...     [cmnd_92] [cmd_105] |  boxed grid of...
// | [cmmnd_2] [cmnd_15]    ...     [cmnd_93] [cmd_106] |  ...command buttons
// ...
// | [cmnd_13] [cmnd_26]    ...     [cmd_104]   blank   |
// +----------------------------------------------------+
//
// Typed input gets appended to "#".  When enough to be unambiguous has
//   accumulated, the matching command is immediately chosen (except for
//   the prefix special case mentioned for [Cancel]);
// Title is centered and describes which [sub]set of commands are shown.
//   It shares the prompt line to conserve vertical space.
// [Cancel] is highlighted as the default and applies if <return> is typed,
//   with special handling when player has typed up to the end of one
//   command which is a prefix of another; there, <return> or <space> is
//   used to select the shorter while still providing opportunity to type
//   more of the longer command; (there are several such cases:
//   "#drop[type]", "#known[class]", "#takeoff[all]", "#version[short]");
//   button is left justitied (prior to addition of the filter/layout/reset
//   buttons, [Cancel] stretched all the way across the top of the widget);
// [Filter] toggles between normal and autocomplete when playing in normal
//   or explore mode, cycles through "all", "normal", "autocomplete", and
//   "wizard mode extra commands only" when playing in wizard mode; that's
//   kind of clumsy but probably not important enough to implement a more
//   sophisticated interface;
// [Layout] toggles between displaying the command buttons down columns
//   (as shown above) versus across rows ([cmd_1][cmd_2]...[cmd_9], &c);
// [Reset] clears typed partial response, if any, and sets filtering back
//   to "all commands" and/or toggles layout back to by-column if either
//   of those differ from their defaults;
// [cmd_N] are buttons labelled with command names; clicking returns the
//   index for the name.
//
// Changing filter or layout returns xcmdNoMatch to qt_get_ext_cmd() which
//   then calls us again (current filter and layout are kept in qt_settings
//   so persist, not just through return and call back but across games);
//   much simpler than reorganizing the button grid's contents on the fly.
// Current grid size with SHELL and SUSPEND enabled is 13x9 for all
//   commands, 13x7 for normal mode commands, and 7x4 (when by-column) or
//   4x7 (if by-row) for wizard mode commands.  Column counts are hardcoded
//   and row counts are adjusted to fit (the command list, not the screen).
// Maybe move prompt and title above control buttons?  However, menus have
//   their count entry feedback positioned between control buttons and the
//   rest of the information--current layout matches that.
// The popup is displayed as full-fledged window but the window title bar
//   is blank (at least on OSX).
// If clicking on [Filter] or [Layout] (or [Reset], but there isn't any
//   particular reason to try to run it twice in a row) places the pointer
//   inside any button, clicking again won't do anything unless the pointer
//   is moved (again, at least on OSX); a single pixel probably suffices.
//   Possibly because despite not moving it has effectively gone into a
//   whole new window since the old one gets torn down and is replaced by
//   a new one that uses revised filter or layout settings.
//

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
#include "qt_key.h" // for keyValue()
#include "qt_bind.h"
#include "qt_set.h"
#include "qt_str.h"

// temporary
extern int qt_compact_mode;
// end temporary

namespace nethack_qt_ {

/* 'wizard' is #undef'd above (now in qt_pre.h) because a Qt header uses
   that token, so create our own based on knowledge of 'wizard's internals */
#define WizardMode (::flags.debug)

extern uchar keyValue(QKeyEvent *key_event); // from qt_menu.cpp

// temporary
void centerOnMain(QWidget *);
// end temporary

static /*inline*/ bool
interesting_command(unsigned indx, int cmds)
{
    bool skip_wizard = !WizardMode || cmds == normal_cmds;

    // entry 0 is a no-op; don't bother displaying it in the command grid
    if (indx == 0 && !strcmp("#", extcmdlist[indx].ef_txt))
        return false;
    // treat '?' as both normal mode and debug mode
    if (!strcmp("?", extcmdlist[indx].ef_txt))
        return true;
    // some commands might have been compiled-out; don't show them
    if ((extcmdlist[indx].flags & (CMD_NOT_AVAILABLE|INTERNALCMD)) != 0)
        return false;
    // if picking from normal mode-only don't show wizard mode commands
    // or if picking from wizard mode-only don't show normal commands
    if ((skip_wizard && (extcmdlist[indx].flags & WIZMODECMD) != 0)
     || (cmds == wizard_cmds && (extcmdlist[indx].flags & WIZMODECMD) == 0))
        return false;
    // autocomplete subset is essentially the traditional set of extended
    // commands; many can be invoked by Alt+char but not by ordinary char
    // or Ctrl+char; [X11's extended command selection uses this subset]
    if (cmds == autocomplete_cmds
        && (extcmdlist[indx].flags & AUTOCOMPLETE) == 0)
        return false;
    // if we've gotten here, this command isn't filtered away, so show it
    return true;
}

NetHackQtExtCmdRequestor::NetHackQtExtCmdRequestor(QWidget *parent) :
    QDialog(parent),
    prompt(new QLabel("#", this)),
    cancel_btn(new QPushButton("Cancel", this)),
    byRow(qt_settings->xcmd_by_row),
    set(qt_settings->xcmd_set),
    butoffset(0),
    exactmatchindx(xcmdNoMatch)
{
    if (!WizardMode && set != normal_cmds)
        set = autocomplete_cmds; // {all,wizard}_cmds are wizard mode only

    QVBoxLayout *xl = new QVBoxLayout(this);  // overall xcmd layout
    int butw = 50; // initial button width; will be increased if too small
    // should probably use the qt_settings font size as a spacing hint;
    // tiny font, tiny internal margins; small font, small margins;
    // medium or bigger, default margins (9 or 11?)
    int spacing = qt_compact_mode ? 3 : -1;  // 0 would abut; -1 gives default

    // first, the popup's controls: a row of buttons along the top;
    // the two padding widgets make the control buttons line up better
    // with the grid of xcmd choice buttons (closer but not exactly)
    QHBoxLayout *ctrls = new QHBoxLayout();
    ctrls->setSpacing(spacing); // only seems to affect horizontal, not vert.
    ctrls->addWidget(new QLabel(" ")); // padding
    // Cancel, created during constructor setup (accessed in other routines)
    DefaultActionIsCancel(true); /* cancel_btn->setDefault(true); */
    cancel_btn->setMinimumSize(cancel_btn->sizeHint());
    butw = std::max(butw, cancel_btn->width());
    ctrls->addWidget(cancel_btn);
    ctrls->addStretch(0); // Cancel will be left justified, others far right
    // Filter: change the [sub]set of commands that get shown;
    // presently only useful when running in wizard mode
    QPushButton *filter_btn = new QPushButton("Filter", this);
#if 0   /* [later] normal vs autocomplete matters regardless of wizard mode */
    if (!WizardMode) { // nothing to filter if not in wizard mode
        filter_btn->setEnabled(false); // gray the [Filter] button out
#if 0   /* This works but makes [Reset] seem to be redundant. */
        // graying out may not be adequate; conceal [Filter] so that
        // players without access to wizard mode won't become concerned
        // about something that seems to them to always be disabled
        filter_btn->hide();
#endif
    }
#endif
    filter_btn->setMinimumSize(filter_btn->sizeHint());
    butw = std::max(butw, filter_btn->width());
    ctrls->addWidget(filter_btn);
    // Layout: switch from by-column grid to by-row grid or vice versa
    QPushButton *layout_btn = new QPushButton("Layout", this);
    layout_btn->setMinimumSize(layout_btn->sizeHint());
    butw = std::max(butw, layout_btn->width());
    ctrls->addWidget(layout_btn);
    // Reset: switch filter back to all commands and layout back to by-column
    QPushButton *reset__btn = new QPushButton("Reset", this);
    reset__btn->setMinimumSize(reset__btn->sizeHint());
    butw = std::max(butw, reset__btn->width());
    ctrls->addWidget(reset__btn);
    ctrls->addWidget(new QLabel(" ")); // padding
    xl->addLayout(ctrls);

    // text entry takes place below the row of control buttons and above
    // the grid of command buttons; show typed text in fixed-width font
    prompt->setFont(qt_settings->normalFixedFont());

    // grid title rather than overall popup title
    const char *ctitle = ((set == all_cmds) // implies wizard mode
                          ? "All commands"
                          : (set == normal_cmds)
                            ? (WizardMode ? "Normal mode commands"
                                          : "Available commands")
                            : (set == autocomplete_cmds)
                              ? "Traditional extended commands"
                              : (set == wizard_cmds)
                                ? "Debug mode commands"
                                : "(unknown)"); // won't happen
    const QString &qtitle = QString(ctitle);
    // rectangular grid to hold a button for each extended command name
    QGroupBox *grid = new QGroupBox(); /* new QGroupBox(title, this); */
    // used to connect the buttons with their click handling routine
    QButtonGroup *group = new QButtonGroup(this);

    // put grid title on same line as prompt (the padding simplifies centering)
    QHBoxLayout *pl = new QHBoxLayout(); // prompt line
    pl->addWidget(prompt);
    QLabel *tt = new QLabel(qtitle);
    tt->setAlignment(Qt::AlignHCenter); // center title horizontally
    pl->addWidget(tt);
    pl->addWidget(new QLabel(" ")); // padding to balance prompt
    xl->addLayout(pl);
    xl->addWidget(grid);

    // having the controls in the same button group as the extended command
    // choices means that they use the same click callback [a holdover from
    // when Cancel was the only one; using a separate group and separate
    // click callback would eliminate the need for butoffset in Button()]
    group->addButton(cancel_btn, butoffset), ++butoffset; // (,0), 1
    group->addButton(filter_btn, butoffset), ++butoffset; // (,1), 2
    group->addButton(layout_btn, butoffset), ++butoffset; // (,2), 3
    group->addButton(reset__btn, butoffset), ++butoffset; // (,3), 4

    unsigned i, j, ncmds = 0;
    QFontMetrics fm = fontMetrics();
    // count the number of commands in current [sub]set and find the size of
    // the widest choice button; starting size is from widest control button
    for (i = 0; extcmdlist[i].ef_txt; ++i) {
        if (interesting_command(i, set)) {
            ++ncmds;
            butw = std::max(butw, 30 + fm.QFM_WIDTH(extcmdlist[i].ef_txt));
        }
    }
    // if any of the choice buttons were bigger than the control buttons,
    // make the control buttons bigger to match
    if (cancel_btn->width() < butw)
        cancel_btn->setMinimumWidth(butw);
    if (filter_btn->width() < butw)
        filter_btn->setMinimumWidth(butw);
    if (layout_btn->width() < butw)
        layout_btn->setMinimumWidth(butw);
    if (reset__btn->width() < butw)
        reset__btn->setMinimumWidth(butw);

    QVBoxLayout *bl = new QVBoxLayout(grid);
    QGridLayout *gl = new QGridLayout();
    // for qt_compact_mode, put buttons closer together
    gl->setSpacing(spacing);
    bl->addLayout(gl);
    // could grow the buttons[] vector one element at a time but since we
    // know the ultimate size, grow to that in one operation
    buttons.resize((int) ncmds);

    /* 'ncols' could be calculated to fit (or enable a vertical scrollbar
       when resulting 'nrows' is too big, if GroupBox supports that);
       it used to be hardcoded 4, but once every command became accessible
       as an extended command, that resulted in so many rows that some of
       the grid was chopped off at the bottom of the screen and the buttons
       in that portion were out of reach */
    unsigned ncols = (set == all_cmds) ? 9
                     : (set == normal_cmds) ? 8
                       : (set == autocomplete_cmds) ? (WizardMode ? 6 : 5)
                         : (set == wizard_cmds) ? (byRow ? 6 : 5)
                           : 1; // can't happen
    unsigned nrows = (ncmds + ncols - 1) / ncols;
    /*
     * Grid layout:  by-column is the default.  Can be toggled by clicking
     * on the [Layout] control button.
     *
     *  by-row  vs  by-column
     *   a b c       a e i
     *   d e f       b f j
     *   g h i       c g -
     *   j - -       d h -
     */
    for (i = j = 0; extcmdlist[i].ef_txt; ++i) {
        if (interesting_command(i, set)) {
            QString btn_lbl = extcmdlist[i].ef_txt;
            if (btn_lbl == "wait")
                btn_lbl += " (rest)";
            QPushButton *pb = new QPushButton(btn_lbl, grid);
            pb->setMinimumSize(butw, pb->sizeHint().height());
            // force the button to have fixed width or it can move around a
            // pixel or two (tiny but visibly noticeable) when enableButtons()
            // hides whole columns [see stretch comment below]
            pb->setMaximumSize(pb->minimumSize());
            // i+butoffset is value that will be passed to the click handler
            group->addButton(pb, i + butoffset);
            // gray out "repeat" because picking it would just repeat the "#"
            // that caused us to be called rather than whatever came before;
            // it can still be chosen by typing "rep" but appears grayed out
            // while in the process so having it not behave usefully shouldn't
            // come as much of a surprise
            if (btn_lbl == "repeat")
                pb->setEnabled(false);
            /*
             * by column: xcmd_by_row==false, the default
             *  0..R-1 down first column, R..2*R-1 down second column, ...
             * otherwise: by row
             *  0..C-1 across first row, C..2*C-1 across second row, ...
             */
            unsigned row = !byRow ? j % nrows : j / ncols;
            unsigned col = !byRow ? j / nrows : j % ncols;
            gl->addWidget(pb, row, col);
            // these stretch settings prevent the grid from becoming very
            // ugly when enableButtons() disables whole rows and/or columns
            // as typed characters reduce the pool of possible matches
            if (row == 0)
                gl->setColumnStretch(col, 1);
            if (col == 0)
                gl->setRowStretch(row, 1);

            // buttons[] vector is used by enableButtons()
            buttons[j] = pb; // buttons.append(pb);
            ++j;
        }
    }

    connect(group, SIGNAL(buttonPressed(int)), this, SLOT(Button(int)));

    bl->activate();
    xl->activate();
    resize(1,1);
}

// Click handler for the ExtCmdRequestor widget
int NetHackQtExtCmdRequestor::Button(int butnum)
{
    // 0..3 are control buttons, 4..N+3 are choice buttons.
    // Widget return value is -1 for cancel (via reject), xcmdNoMatch
    // for filter, layout, reset (via accept if circumstances warrant),
    // 1..N for command choices (choice 0 is '#' and it isn't shown as
    // a candidate since picking it is not useful).
    switch (butnum) {
    case 0:
        Cancel();
        /*NOTREACHED*/
        break;
    case 1:
        Filter();
        break;
    case 2:
        Layout();
        break;
    case 3:
        Reset();
        break;
    default:
        // 4..N-3 are the extended commands
        done(butnum - butoffset + 1);
        /*NOTREACHED*/
        break;
    }
    return 0;
}

// Respond to a click on the [Cancel] button
void NetHackQtExtCmdRequestor::Cancel()
{
    reject();
    /*NOTREACHED*/
}

// Respond to a click on the [Filter] button
void NetHackQtExtCmdRequestor::Filter()
{
    do {
        if (++set > std::max(std::max(all_cmds, normal_cmds),
                             std::max(autocomplete_cmds, wizard_cmds)))
            set = std::min(std::min(all_cmds, normal_cmds),
                           std::min(autocomplete_cmds, wizard_cmds));
        if (WizardMode)
            break;
    } while (set != normal_cmds && set != autocomplete_cmds);

    if (set != qt_settings->xcmd_set) {
        Retry();
        /*NOTREACHED*/
    }
    return;
}

// Respond to a click on the [Layout] button
void NetHackQtExtCmdRequestor::Layout()
{
    byRow = !byRow;
    Retry();
    /*NOTREACHED*/
}

// Respond to a click on the [Reset] button
void NetHackQtExtCmdRequestor::Reset()
{
    // clear any typed text first (in case future changes are made to
    // remember it across accept (retry); that would be intended for the
    // [Layout] case rather than for [Reset])
    bool clearprompt = (prompt->text() != "#");
    if (clearprompt)
        prompt->setText("#");

    int teardown = 0;
    if (set != (WizardMode ? all_cmds : normal_cmds))
        set = all_cmds, ++teardown;
    if (byRow)
        byRow = false, ++teardown;
    if (teardown) {
        Retry();
        /*NOTREACHED*/
    }

    if (clearprompt) {   // was a subset, now need to show full set
        DefaultActionIsCancel(true); // in case keyPressEvent cleared it
        enableButtons(); // redraws the grid after discarding typed text above
    }
}

// Return to ExtCmdRequestor::get()'s caller in order to be called back
void NetHackQtExtCmdRequestor::Retry()
{
    // remember the current settings; they'll persist until changed again
    qt_settings->updateXcmd(byRow, set);

    // return to qt_get_ext_cmd() and have it run ExtCmdRequestor again;
    // current selection grid will be torn down, then new one created;
    setResult(xcmdNoMatch);
    accept();
    /*NOTREACHED*/
}

#define Ctrl(c) (0x1f & (c)) /* ASCII */
// Note: we don't necessarily have access to a terminal to query
// it for user's preferred kill character, so use hardcoded ^U.
// Player who prefers something else can cope by using ESC instead.
#define KILL_CHAR Ctrl('u')

// used by keyPressEvent() and enableButtons()
static const QString &rest = "rest"; // informal synonym for "wait"

// Receive the next character of typed input
void NetHackQtExtCmdRequestor::keyPressEvent(QKeyEvent *event)
{
    /*
     * This will select a command outright--as soon as enough chars
     * to not be ambiguous are entered--but also narrows down visible
     * choices [via enableButtons()] in the grid of clickable command
     * buttons as the text-so-far makes many become impossible to match.
     * Use of backspace/delete or ESC/kill restores suppressed choices
     * to the grid as they become matchable again.
     */

    unsigned saveexactmatchindx = exactmatchindx;
    // if previous KeyPressEvent cleared default, restore it now
    DefaultActionIsCancel(true);
    QString promptstr = prompt->text();
    uchar uc = keyValue(event);

    if (!uc) {
        // shift or control or meta, another character should be coming
        QWidget::keyPressEvent(event);
    } else if (uc == '\033' || uc == KILL_CHAR) {
        // <escape> when partial response is present kills that text
        // but keeps prompting; <escape> when response is empty cancels.
        // Kill gets rid of pending text, if any, and always re-prompts.
        if (uc == '\033' && promptstr == "#")
            reject(); // cancel() if ESC used when string is empty
        prompt->setText("#"); // reset to empty ('#' is always present)
        enableButtons();
    } else if (uc == '\b' || uc == '\177') {
	if (promptstr != "#")
	    prompt->setText(promptstr.left(promptstr.size() - 1));
        enableButtons();
    } else if ((uc < ' ' && !(uc == '\n' || uc == '\r'))
               || uc > std::max('z', 'Z')) {
	reject(); // done()
    } else {
        /*
         * <return> or <space> is necessary if one command is a
         * leading substring of another and superfluous otherwise.
         * (When a command is not a prefix of another, it will have
         * been selected before reaching its last letter.)
         *
         * If we got an exact match with the last key, we're expecting
         * a <return> or <space> to explicitly choose it now but might
         * get next letter of the longer command (or get a backspace).
         *
         * If we get an exact match this time, then stop showing
         * [Cancel] as the default action for <return>.
         * (That's a visual cue for the player, not a requirement to
         * prevent <return> from triggering the default action.)
         * If it's not the default action upon the next key press,
         * we'll change that back (done above).
         *
         * TODO?
         *  Implement <tab> support:  if promptstr matches multiple
         *  commands but they all have the next one or more letters in
         *  common, allow <tab> to add the common letters to promptstr.
         */
        bool checkexact = (uc == '\n' || uc == '\r' || uc == ' ');
        if (!checkexact) {
            // force lower case instead of rejecting upper case
            if (isupper(uc))
                uc = tolower(uc);
            promptstr += QChar(uc); // add new char to typed text
        }
        QString typedstr = promptstr.mid(1); // skip the '#'
        if (typedstr == rest)
            typedstr = "wait";
        std::size_t len = typedstr.size();
        unsigned matches = 0;
        unsigned matchindx = 0;
        for (unsigned i = 0; extcmdlist[i].ef_txt; ++i) {
            if (!interesting_command(i, set))
                continue;
            const QString &cmdtxt = QString(extcmdlist[i].ef_txt);
            if (cmdtxt.startsWith(typedstr)) {
                bool is_exact = (cmdtxt == typedstr);
                if (checkexact) {
                    if (is_exact) {
                        matchindx = i;
                        matches = 1;
                        break;
                    }
                } else {
                    if (is_exact)
                        DefaultActionIsCancel(false, i); // clear default
                    if (++matches >= 2)
                        break;
                    matchindx = i;
                }
	    }
	}
	if (matches == 1) {
            done(matchindx + 1);
        } else if (checkexact) {
            // <return> or <space> without a pending exact match; cancel
            reject();
        } else if (matches >= 2
                   || promptstr.mid(1, len) == rest.left(len)) {
            // update the text-so-far
            prompt->setText(promptstr);
        } else if (saveexactmatchindx != xcmdNoMatch) {
            // had a pending exact match but typed something other than
            // <return> which didn't yield another match; prompt string
            // hasn't been updated so still have a pending exact match
            DefaultActionIsCancel(false, saveexactmatchindx);
        }
        enableButtons();
    }
}

// Actual widget execution, used after calling NetHackQtExtCmdRequestor().
int NetHackQtExtCmdRequestor::get()
{
    resize(1,1); // pack
    centerOnMain(this);
    // Add any keys presently buffered to the prompt
    setResult(xcmdNone);
    while (NetHackQtBind::qt_kbhit() && result() == xcmdNone) {
	int ch = NetHackQtBind::qt_nhgetch();
	QKeyEvent event(QEvent::KeyPress, 0, Qt::NoModifier, QChar(ch));
	keyPressEvent(&event);
    }
    if (result() == xcmdNone)
	exec();
    return result() - 1;
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
        const QString &buttext = (*b)->text();
        bool showit = (buttext.left(len) == typedstr
                       || (buttext.contains("(rest)")
                           && typedstr == rest.left(len)));
        (*b)->setVisible(showit);
    }
}

} // namespace nethack_qt_
