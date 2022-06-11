// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_yndlg.cpp -- yes/no dialog

extern "C" {
#include "hack.h"
}

#include "qt_pre.h"
#include <QtGui/QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QtWidgets>
#endif
#include "qt_post.h"
#include "qt_yndlg.h"
#include "qt_yndlg.moc"
#include "qt_key.h" // for keyValue()
#include "qt_str.h"

// temporary
extern int qt_compact_mode;
// end temporary

namespace nethack_qt_ {

static const char lrq[] = "lr\033LRq";
char altchoices[BUFSZ + 12];

// temporary
void centerOnMain(QWidget *);
// end temporary

NetHackQtYnDialog::NetHackQtYnDialog(QWidget *parent, const QString &q,
                                     const char *ch, char df) :
    QDialog(parent),
    question(q), choices(ch), def(df),
    keypress('\033'),
    allow_count(false),
    le((QLineEdit *) NULL),
    y_btn((QPushButton *) NULL)
{
    setWindowTitle("NetHack: Question");

    // plain prompt doesn't show any room for an answer (answer won't be
    // echoed but the fact that a prompt is pending and accepts typed
    // input as an alternative to mouse click seems clearer when there
    // is some space available to accept it)
    if (!question.endsWith(" ") && !question.endsWith("_"))
        question += " _"; // an underlined space would be better

    if (choices) {
        // special handling for wearing rings; prompt asks "right or left?"
        // but side-by-side buttons look better with [left][right] instead
        // (assumes that we're using left to right layout)
        if (!strcmp(choices, "rl")) {
            choices = lrq;
            if (!def)
                def = 'r';

        // if count is allowed, explicitly add the digits as valid
        } else if (!strncmp(choices, "yn#", (size_t) 3)) {
            ::yn_number = 0L;
            allow_count = true;

            if (!strchr(choices, '9')) {
                copynchars(altchoices, choices, BUFSZ - 1);
                // duplicate # is intentional; explicitly separates \... and 0
                choices = strcat(altchoices, "\033#0123456789");
            }
        }
    }
    alt_answer[0] = alt_result[0] = '\0';
}

char NetHackQtYnDialog::Exec()
{
    QString ch(QString::fromLatin1(choices));
//    int ch_per_line=6;
    QString qlabel;
    QString enable;
    if ( qt_compact_mode && !choices ) {
        ch = "";
	// expand choices from prompt
	// ##### why isn't choices set properly???
        int c = question.indexOf(QChar('['));
	qlabel = QString(question).left(c);
	if ( c >= 0 ) {
	    c++;
	    if ( question[c] == '-' )
		ch.append(question[c++]);
	    unsigned from=0;
            while (c < question.size()
                   && question[c] != ']' && question[c] != ' ') {
		if ( question[c] == '-' ) {
		    from = question[c - 1].cell();
		} else if ( from != 0 ) {
		    for (unsigned f=from+1; QChar(f)<=question[c]; f++)
			ch.append(QChar(f));
		    from = 0;
		} else {
		    ch.append(question[c]);
		    from = 0;
		}
		c++;
	    }
	    if ( question[c] == ' ' ) {
		while ( c < question.size() && question[c] != ']' ) {
		    if ( question[c] == '*' || question[c] == '?' )
			ch.append(question[c]);
		    c++;
		}
	    }
	}
	if ( question.indexOf("what direction") >= 0 ) {
	    // We replace this regardless, since sometimes you get choices.
	    const char* d = g.Cmd.dirchars;
	    enable=ch;
	    ch="";
	    ch.append(d[1]);
	    ch.append(d[2]);
	    ch.append(d[3]);
	    ch.append(d[0]);
	    ch.append('.');
	    ch.append(d[4]);
	    ch.append(d[7]);
	    ch.append(d[6]);
	    ch.append(d[5]);
	    ch.append(d[8]);
	    ch.append(d[9]);
//	    ch_per_line = 3;
	    def = ' ';
	} else {
	    // Hmm... they'll have to use a virtual keyboard
	}
    } else {
        ch = QString::fromLatin1(choices);
	qlabel = question.replace(QChar(0x200B), QString(""));
    }
    if (!ch.isNull()) {
	QVBoxLayout *vb = new QVBoxLayout;
        bool bigq = (qlabel.length() > (qt_compact_mode ? 40 : 60));
        if (bigq) {
            QLabel *q = new QLabel(qlabel, this);
	    q->setAlignment(Qt::AlignLeft);
	    q->setWordWrap(true);
	    q->setMargin(4);
	    vb->addWidget(q);
	}
	QGroupBox *group = new QGroupBox(bigq ? QString() : qlabel, this);
	vb->addWidget(group);
	QHBoxLayout *groupbox = new QHBoxLayout();
	group->setLayout(groupbox);
	QButtonGroup *bgroup = new QButtonGroup(group);

	int nchoices=ch.length();
        // note: is_ynaq covers nyaq too because the choices string is
        // "ynaq" for both; only the default differs; likewise for nyNaq
        bool is_ynaq = (ch == QString("ynaq") // [Yes ][ No ][All ][Stop]
                        || ch == QString("yn#aq")
                        || ch == altchoices), // alternate "yn#aq"
             is_ynq = (ch == QString("ynq")), // [ Yes  ][  No  ][Cancel]
             is_yn  = (ch == QString("yn")),  // [Yes ][ No ]
             is_lr  = (ch == QString(lrq));   // [ Left ][Right ]

#if 0
	const int margin=8;
	const int gutter=8;
	const int extra=fontMetrics().height(); // Extra for group
	int x=margin, y=extra+margin;
#endif
	int butheight = fontMetrics().height() * 2 + 5,
            butwidth = (butheight - 5) * ((is_ynq || is_lr) ? 3
                                          : (is_ynaq || is_yn) ? 2 : 1) + 5;
        if (butwidth == butheight) { // square, enough room for C or ^C
            // some characters will be labelled by name rather than by
            // keystroke so will need wider buttons
            for (int i = 0; i < nchoices; ++i) {
                if (ch[i] == '\033')
                    break; // ESC and anything after are hidden
                if (ch[i] == ' ' || ch[i] == '\n' || ch[i] == '\r') {
                    butwidth = (butheight - 5) * 2 + 5;
                    break;
                }
            }
        }

        QPushButton *button;
        for (int i = 0; i < nchoices; ++i) {
            bool making_y = false;
            if (ch[i] == '\033')
                break; // ESC and anything after are hidden
            if (ch[i] == '#' && allow_count)
                continue; // don't show a button for '#'; has Count box instead
            QString button_name = QString(visctrl((char) ch[i].cell()));
            if (is_yn || is_ynq || is_ynaq || is_lr) {
                // FIXME: a better way to recognize which labels should
                // use alterate text is needed
                switch (ch[i].cell()) {
                case 'y':
                    button_name = "Yes";
                    making_y = true;
                    break;
                case 'n':
                    button_name = "No";
                    break;
                case 'a':
                    // the display of vanquished monsters uses "ynaq" for
                    // convenience, where 'a' requests a sort-by menu;
                    // show "sort" instead of "all" and allow player to
                    // type either 'a' or 's' when not clicking on button
                    if (question.contains(QString("vanquished?")))
                        button_name = "Sort", AltChoice('s', 'a');
                    else
                        button_name = "All";
                    break;
                case 'q':
                    // most 'q' replies are actually for "cancel" but
                    // for "ynaq" (where "all" is a choice) it's "stop"
                    // and for end of game disclosure it really is "quit"
                    if (question.left(10) == QString("Dump core?")
                        || (::g.program_state.gameover
                            && question.left(11) == QString("Do you want")))
                        button_name = "Quit";
                    else if (is_ynaq)
                        button_name = "Stop", AltChoice('s', 'q');
                    else
                        button_name = "Cancel", AltChoice('c', 'q');
                    break;
                case 'l':
                    button_name = "Left";
                    break;
                case 'r':
                    button_name = "Right";
                    break;
                }
            } else {
                // special characters usually aren't listed among choices
                // but if they are, label the buttons for them with sensible
                // names; we want to avoid "^J" and "^M" for \n and \r;
                // <Enter> and <Return> are equivalent to each other but
                // labelling \n as newline or line-feed seems confusing;
                switch (ch[i].cell()) {
                case ' ':
                    button_name = "Spc";
                    break;
                case '\n':
                    button_name = "Ent";
                    break;
                case '\r':
                    button_name = "Ret";
                    break;
                case '\033': // won't happen; ESC is hidden
                    button_name = "Esc";
                    break;
                case '&':
                    // ampersand is used as a hidden quote char to flag
                    // next character as a keyboard shortcut associated
                    // with the current action--that's inappropriate here;
                    // two consecutive ampersands are needed to display
                    // one in a button label; first check whether caller
                    // has already done that, skip this one if so
                    if (i > 0 && ch[i - 1].cell() == '&')
                        continue; // next i
                    button_name = "&&";
                    break;
                }
            }
            button=new QPushButton(button_name);
            if (making_y && allow_count)
                y_btn = button; // to change default in keyPressEvent()
            if (!enable.isNull()) {
                if (!enable.contains(ch[i]))
                    button->setEnabled(false);
            }
            button->setFixedSize(butwidth, butheight);
            if (ch[i] == def)
                button->setDefault(true);
#if 0
            // 'x' and 'y' don't seem to actually used anywhere
            // and limit of 10 buttons per row isn't enforced
            if (i % 10 == 9) {
                // last in row
                x = margin;
                y += butheight + gutter;
            } else {
                x += butwidth + gutter;
            }
#endif
	    groupbox->addWidget(button);
	    bgroup->addButton(button, i);
	}

        connect(bgroup, SIGNAL(buttonClicked(int)), this, SLOT(doneItem(int)));

        QLabel *lb = 0;
        if (allow_count) {
            // insert Count widget in front of [n], between [y] and [n][a][q]
            lb = new QLabel("Count:");
            groupbox->insertWidget(1, lb); // [y] button is item #0, [n] is #1
            le = new QLineEdit();
            groupbox->insertWidget(2, le); // [n] became #2, Count label is #1
            le->setPlaceholderText(QString("#")); // grayed out
	}
        // add an invisible right-most field to left justify the buttons
        groupbox->addStretch(80);

	setLayout(vb);
	adjustSize();
	centerOnMain(this);
	show();
	char choice=0;
	char ch_esc=0;
        for (int i = 0; i < ch.length(); ++i) {
            if (ch[i].cell() == 'q')
                ch_esc = 'q';
            else if (!ch_esc && ch[i].cell() == 'n')
                ch_esc = 'n';
	}

        //
        // When a count is allowed, clicking on the count widget then
        // typing in digits followed by <return> is 'normal' operation.
        // However, typing a digit without clicking first will set focus
        // to the count widget with that typed digit preloaded.
        //
        exec();
        int res = result();
        if (res == 0) {
            choice = is_lr ? '\033' : ch_esc ? ch_esc : def ? def : ' ';
        } else if (res == 1) {
            if (keypress)
                choice = keypress;
            else
                choice = def ? def : ch_esc ? ch_esc : ' ';
        } else if (res >= 1000) {
            choice = (char) ch[res - 1000].cell();
        }

        // non-Null 'le' implies 'allow_count'; having a grayed-out '#'
        // present in the QLineEdit widget doesn't affect its isEmpty() test
        if (le && !le->text().isEmpty()) {
            QString text(le->text());
            if (text.at(0) == QChar('#'))
                text = text.mid(1); // rest of string past [0]
            ::yn_number = text.toLong();
            choice = '#';
        }
        keypress = choice;

    } else {
	QLabel label(qlabel,this);
	QPushButton cancel("Dismiss",this);
#if __cplusplus >= 202002L
	label.setFrameStyle(static_cast<int>(QFrame::Box)
                                | static_cast<int>(QFrame::Sunken));
#else
	label.setFrameStyle(QFrame::Box|QFrame::Sunken);
#endif
	label.setAlignment(Qt::AlignCenter);
	label.resize(fontMetrics().QFM_WIDTH(qlabel)+60,30+fontMetrics().height());
	cancel.move(width()/2-cancel.width()/2,label.geometry().bottom()+8);
	connect(&cancel,SIGNAL(clicked()),this,SLOT(reject()));
	centerOnMain(this);
	setResult(-1);
	show();
	keypress = '\033';
	exec();
    }
    return keypress;
}

void NetHackQtYnDialog::AltChoice(char ans, char res)
{
    if (ans && !strchr(alt_answer, ans)) {
        (void) strkitten(alt_answer, ans);
        (void) strkitten(alt_result, res);
    }
}

void NetHackQtYnDialog::keyPressEvent(QKeyEvent *event)
{
    keypress = keyValue(event);
    if (!keypress)
        return;

    char *p = NULL;
    if (*alt_answer && (p = strchr(alt_answer, keypress)) != 0)
        keypress = alt_result[p - alt_answer];

    if (!choices || !*choices || !keypress) {
        this->done(1);

    } else {
	int where = QString::fromLatin1(choices).indexOf(QChar(keypress));

        if (allow_count && strchr("#0123456789", keypress)) {
            if (keypress == '#') {
                // 0 will be preselected; typing anything replaces it
                le->setText(QString("0"));
                le->home(true);
            } else {
                // digit will not be preselected; typing another appends
                le->setText(QChar(keypress));
                le->end(false);
            }
            // (don't know whether this actually does anything useful)
            le->setAttribute(Qt::WA_KeyboardFocusChange, true);
            // this is definitely useful...
            le->setFocus(Qt::ActiveWindowFocusReason);
            // change default button from 'n' to 'y'
            if (y_btn)
                y_btn->setDefault(true);
	} else if (where != -1) {
            this->done(where + 1000);

	} else {
	    QDialog::keyPressEvent(event);
	}
    }
}

void NetHackQtYnDialog::doneItem(int i)
{
    this->done(i + 1000);
}

} // namespace nethack_qt_
