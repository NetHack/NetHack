// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt4yndlg.cpp -- yes/no dialog

#include "hack.h"
#undef Invisible
#undef Warning
#undef index
#undef msleep
#undef rindex
#undef wizard
#undef yn
#undef min
#undef max

#include <QtGui/QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QtWidgets>
#endif
#include "qt4yndlg.h"
#include "qt4yndlg.moc"
#include "qt4str.h"

// temporary
extern int qt_compact_mode;
// end temporary

namespace nethack_qt4 {

// temporary
void centerOnMain(QWidget *);
// end temporary

NetHackQtYnDialog::NetHackQtYnDialog(QWidget *parent,const QString& q,const char* ch,char df) :
    QDialog(parent),
    question(q), choices(ch), def(df),
    keypress('\033')
{
    setWindowTitle("NetHack: Question");
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
	    while ( c < question.size() && question[c] != ']' && question[c] != ' ' ) {
		if ( question[c] == '-' ) {
		    from = question[c-1].unicode();
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
	    const char* d = Cmd.dirchars;
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
	bool bigq = qlabel.length()>40;
	if ( bigq ) {
	    QLabel* q = new QLabel(qlabel,this);
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

	bool allow_count=ch.contains('#');
	QString yn = "yn", ynq = "ynq";
	bool is_ynq = ch == yn || ch == ynq;

	const int margin=8;
	const int gutter=8;
	const int extra=fontMetrics().height(); // Extra for group
	int x=margin, y=extra+margin;
	int butsize=fontMetrics().height()*2+5;

	QPushButton* button;
	for (int i=0; i<nchoices && ch[i]!='\033'; i++) {
	    QString button_name = QString(ch[i]);
	    if (is_ynq) {
		if (button_name == ynq.mid(0, 1)) {
		    button_name = "Yes";
		} else if (button_name == ynq.mid(1, 1)) {
		    button_name = "No";
		} else if (button_name == ynq.mid(2, 1)) {
		    button_name = "Cancel";
		}
	    }
	    button=new QPushButton(button_name);
	    if ( !enable.isNull() ) {
		if ( !enable.contains(ch[i]) )
		    button->setEnabled(false);
	    }
	    button->setFixedSize(butsize,butsize); // Square
	    if (ch[i]==def) button->setDefault(true);
	    if (i%10==9) {
		// last in row
		x=margin;
		y+=butsize+gutter;
	    } else {
		x+=butsize+gutter;
	    }
	    groupbox->addWidget(button);
	    bgroup->addButton(button, i);
	}

	connect(bgroup,SIGNAL(buttonClicked(int)),this,SLOT(doneItem(int)));

	QLabel* lb=0;
	QLineEdit* le=0;

	if (allow_count) {
	    QHBoxLayout *hb = new QHBoxLayout(this);
	    lb=new QLabel("Count: ");
	    hb->addWidget(lb);
	    le=new QLineEdit();
	    hb->addWidget(le);
	    vb->addLayout(hb);
	}

	setLayout(vb);
	adjustSize();
	centerOnMain(this);
	show();
	char choice=0;
	char ch_esc=0;
	for (uint i=0; i<ch.length(); i++) {
	    if (ch[i].unicode()=='q') ch_esc='q';
	    else if (!ch_esc && ch[i].unicode()=='n') ch_esc='n';
	}
	exec();
	if ( result() == 0) {
	    choice = ch_esc ? ch_esc : def ? def : ' ';
	} else if ( result() == 1 ) {
	    choice = def ? def : ch_esc ? ch_esc : ' ';
	} else if ( result() >= 1000 ) {
	    choice = ch[result() - 1000].unicode();
	}
	if (allow_count && !le->text().isEmpty()) {
	    yn_number=le->text().toInt();
	    choice='#';
	}
	return choice;
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
	return keypress;
    }
}

void NetHackQtYnDialog::keyPressEvent(QKeyEvent* event)
{
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

} // namespace nethack_qt4
