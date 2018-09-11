// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt4xcmd.cpp -- extended command widget

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

#include <QtGui/QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QtWidgets>
#endif
#include "qt4xcmd.h"
#include "qt4xcmd.moc"
#include "qt4bind.h"
#include "qt4set.h"
#include "qt4str.h"

namespace nethack_qt4 {

// temporary
void centerOnMain(QWidget *);
// end temporary

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

    int i;
    int butw=50;
    QFontMetrics fm = fontMetrics();
    for (i=0; extcmdlist[i].ef_txt; i++) {
	butw = std::max(butw,30+fm.width(extcmdlist[i].ef_txt));
    }
    int ncols=4;

    QVBoxLayout* bl = new QVBoxLayout(grid);
    bl->addSpacing(fm.height());
    QGridLayout* gl = new QGridLayout();
    bl->addLayout(gl);
    for (i=0; extcmdlist[i].ef_txt; i++) {
	QPushButton* pb=new QPushButton(extcmdlist[i].ef_txt, grid);
	pb->setMinimumSize(butw,pb->sizeHint().height());
	group->addButton(pb, i+1);
	gl->addWidget(pb,i/ncols,i%ncols);
        buttons.append(pb);
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
    else if (text == "\b")
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

// Enable only buttons that match the current prompt string
void NetHackQtExtCmdRequestor::enableButtons()
{
    QString typedstr = prompt->text().mid(1); // skip the '#'
    std::size_t len = typedstr.size();

    for (auto b = buttons.begin(); b != buttons.end(); ++b) {
        (*b)->setVisible((*b)->text().left(len) == typedstr);
    }
}

} // namespace nethack_qt4
