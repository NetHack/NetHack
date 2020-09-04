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
    keypress('\033')
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
        if (!strcmp(choices, "rl")) {
            choices = lrq;
            if (!def)
                def = 'r';

        // if count is allowed, explicitly add the digits as valid
        } else if (!strncmp(choices, "yn#", (size_t) 3)) {
            ::yn_number = 0L;

            if (!strchr(choices, '9')) {
                copynchars(altchoices, choices, BUFSZ - 1);
                // duplicate # is intentional; explicitly separates \... and 0
                choices = strcat(altchoices, "\033#0123456789");
            }
        }
    }
}

char NetHackQtYnDialog::Exec()
{
    QString ch(QString::fromLatin1(choices));
    int ch_per_line=6;
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
		    for (unsigned f=from+1; f<=question[c]; f++)
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
	    ch_per_line = 3;
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
	QGroupBox *group = new QGroupBox(bigq ? QString::null : qlabel, this);
	vb->addWidget(group);
	QHBoxLayout *groupbox = new QHBoxLayout();
	group->setLayout(groupbox);
	QButtonGroup *bgroup = new QButtonGroup(group);

	int nchoices=ch.length();
        bool allow_count = (ch.left(3) == QString("yn#")),
             is_ynq = (ch == QString("ynq")), // [ Yes  ][  No  ][Cancel]
             is_yn  = (ch == QString("yn")),  // [Yes ][ No ]
             is_lr  = (ch == QString(lrq));   // [ Left ][Right ]

	const int margin=8;
	const int gutter=8;
	const int extra=fontMetrics().height(); // Extra for group
	int x=margin, y=extra+margin;
        int butheight = fontMetrics().height() * 2 + 5,
            butwidth = (butheight - 5)
                       * ((is_ynq || is_lr) ? 3 : is_yn ? 2 : 1) + 5;

        QPushButton *button;
        for (int i = 0; i < nchoices; ++i) {
            if (ch[i] == '\033')
                break; // ESC and anything after are hidden
            if (ch[i] == '#' && allow_count)
                continue; // don't show a button for '#'; has Count box instead
            QString button_name = QString(visctrl((char) ch[i].cell()));
            if (is_yn || is_ynq || is_lr) {
                switch (ch[i].cell()) {
                case 'y':
                    button_name = "Yes";
                    break;
                case 'n':
                    button_name = "No";
                    break;
                case 'q':
                    // FIXME: sometimes the 'q' choice is ''cancel current
                    // action'' but other times it is actually 'quit'.
                    if (question.left(10) == QString("Dump core?"))
                        button_name = "Quit";
                    else
                        button_name = "Cancel";
                    break;
                case 'l':
                    button_name = "Left";
                    break;
                case 'r':
                    button_name = "Right";
                    break;
                }
            }
            button=new QPushButton(button_name);
            if (!enable.isNull()) {
                if (!enable.contains(ch[i]))
                    button->setEnabled(false);
            }
            button->setFixedSize(butwidth, butheight);
            if (ch[i] == def)
                button->setDefault(true);
            // 'x' and 'y' don't seem to actually used anywhere
            // and limit of 10 buttons per row isn't enforced
            if (i % 10 == 9) {
                // last in row
                x = margin;
                y += butheight + gutter;
            } else {
                x += butwidth + gutter;
            }
	    groupbox->addWidget(button);
	    bgroup->addButton(button, i);
	}

        connect(bgroup, SIGNAL(buttonClicked(int)), this, SLOT(doneItem(int)));

        QLabel *lb = 0;
        QLineEdit *le = 0;
        if (allow_count) {
            // put the Count widget in between [y] and [n][a][q]
            lb = new QLabel("Count:");
            groupbox->insertWidget(1, lb); // [n] button is item #1
            le = new QLineEdit();
            groupbox->insertWidget(2, le); // [n] became #2, Count label #1
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
        // FIXME:  Unfortunately, it will also be selected, so typing
        // another digit replaces it instead of being the next digit in
        // a multiple-digit number.
        //
        // Theoretically typing '#' does this to, with a 0 preloaded
        // and intentionally selected, but the KeyPress bug (below) of
        // treating <shift> as a complete response prevents use of
        // shift+3 from being used to generate '#'.
        //
        bool retry; // for digit + re-activate widget + rest of number
        do {
            retry = false; // might have a second pass (but not a third)
            exec();
            int res = result();
            if (res == 0) {
                choice = is_lr ? '\033' : ch_esc ? ch_esc : def ? def : ' ';
            } else if (res == 1) {
                choice = def ? def : ch_esc ? ch_esc : ' ';
            } else if (res >= 1000) {
                choice = (char) ch[res - 1000].cell();

                if (allow_count && strchr("#0123456789", choice)) {
                    if (choice == '#') {
                        // 0 will be preselected; typing anything replaces it
                        le->insert(QString("0"));
                    } else {
                        le->insert(QString(choice));
                        //
                        // FIXME:  despite the documentation claiming that
                        // 'false' cancels any selection, the digit always
                        // starts out selected (from running exec() again?)
                        // so typing the next digit replaces it instead of
                        // being appended to it unless the player uses
                        // right-arrow to move the cursor.
                        //
                        le->end(false);
                    }
                    // (don't know whether this actually does anything useful)
                    le->setAttribute(Qt::WA_KeyboardFocusChange, true);
                    le->setFocus(Qt::ActiveWindowFocusReason);
                    retry = true;
                }
            }
        } while (retry);

        // non-Null 'le' implies 'allow_count'
        if (le && !le->text().isEmpty()) {
            ::yn_number = le->text().toInt();
            choice = '#';
        }
        keypress = choice;

    } else {
	QLabel label(qlabel,this);
	QPushButton cancel("Dismiss",this);
	label.setFrameStyle(QFrame::Box|QFrame::Sunken);
	label.setAlignment(Qt::AlignCenter);
	label.resize(fontMetrics().width(qlabel)+60,30+fontMetrics().height());
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

void NetHackQtYnDialog::keyPressEvent(QKeyEvent* event)
{
    //
    // FIXME:  on OSX (possibly elsewhere), this accepts <shift>
    // (and even <caps lock>) as the entire response before the user
    // has a chance to type any character to be shifted.
    //

    // Don't want QDialog's Return/Esc behaviour
    //RLC ...or do we?
    QString text(event->text());
    if (choices == NULL || choices[0] == 0) {
	if (text != "") {
	    keypress = text.toUcs4()[0];
	    done(1);
	}
    } else {
	int where = QString::fromLatin1(choices).indexOf(text);
	if (where != -1 && text != "#") {
	    done(where+1000);
	} else {
	    QDialog::keyPressEvent(event);
	}
    }
}

void NetHackQtYnDialog::doneItem(int i)
{
    done(i+1000);
}

} // namespace nethack_qt_
