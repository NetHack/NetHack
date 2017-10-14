// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt4plsel.cpp -- player selector dialog

extern "C" {
#include "hack.h"
}
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
#include "qt4plsel.h"
#include "qt4plsel.moc"
#include "qt4bind.h"
#include "qt4glyph.h"
#include "qt4set.h"
#include "qt4str.h"

// Warwick prefers it this way...
#define QT_CHOOSE_RACE_FIRST

namespace nethack_qt4 {

// temporary
void centerOnMain( QWidget* w );
// end temporary

static const char nh_attribution[] = "<center><big>NetHack %1</big>"
	"<br><small>by the NetHack DevTeam</small></center>";

class NhPSListViewItem : public QTableWidgetItem {
public:
    NhPSListViewItem( QTableWidget* parent, const QString& name ) :
	QTableWidgetItem(name)
    {
    }

    void setGlyph(int g)
    {
	NetHackQtGlyphs& glyphs = qt_settings->glyphs();
	int gw = glyphs.width();
	int gh = glyphs.height();
	QPixmap pm(gw,gh);
	QPainter p(&pm);
	glyphs.drawGlyph(p, g, 0, 0);
	p.end();
	setIcon(QIcon(pm));
	//RLC setHeight(std::max(pm.height()+1,height()));
    }

#if 0 //RLC
    void paintCell( QPainter *p, const QColorGroup &cg,
		    int column, int width, int alignment )
    {
	if ( isSelectable() ) {
	    QTableWidgetItem::paintCell( p, cg, column, width, alignment );
	} else {
	    QColorGroup disabled(
		cg.foreground().light(),
		cg.button().light(),
		cg.light(), cg.dark(), cg.mid(),
		Qt::gray, cg.base() );
	    QTableWidgetItem::paintCell( p, disabled, column, width, alignment );
	}
    }
#endif
};

class NhPSListViewRole : public NhPSListViewItem {
public:
    NhPSListViewRole( QTableWidget* parent, int id ) :
	NhPSListViewItem(parent,
#ifdef QT_CHOOSE_RACE_FIRST // Lowerize - looks better
	    QString(roles[id].name.m).toLower()
#else
	    roles[id].name.m
#endif
	)
    {
	setGlyph(monnum_to_glyph(roles[id].malenum));
    }
};

class NhPSListViewRace : public NhPSListViewItem {
public:
    NhPSListViewRace( QTableWidget* parent, int id ) :
	NhPSListViewItem(parent,
#ifdef QT_CHOOSE_RACE_FIRST // Capitalize - looks better
	    str_titlecase(races[id].noun)
#else
	    races[id].noun
#endif
	)
    {
	setGlyph(monnum_to_glyph(races[id].malenum));
    }
};

class NhPSListView : public QTableWidget {
public:
    NhPSListView( QWidget* parent ) :
	QTableWidget(parent)
    {
	setColumnCount(1);
	verticalHeader()->hide();
#if QT_VERSION >= 0x050000
	horizontalHeader()->setSectionsClickable(false);
#else
	horizontalHeader()->setClickable(false);
#endif
    }

    QSizePolicy sizePolicy() const
    {
	return QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    }

    QSize minimumSizeHint() const
    {
	return sizeHint();
    }

    QSize sizeHint() const
    {
	return QSize(columnWidth(0), QTableWidget::sizeHint().height());
    }
};

NetHackQtPlayerSelector::NetHackQtPlayerSelector(NetHackQtKeyBuffer& ks) :
    QDialog(NetHackQtBind::mainWidget()),
    fully_specified_role(true)
{
    /*
               0             1             2
	  + Name ------------------------------------+
	0 |                                          |
	  + ---- ------------------------------------+
	  + Role ---+   + Race ---+   + Gender ------+
	  |         |   |         |   |  * Male      |
	1 |         |   |         |   |  * Female    |
	  |         |   |         |   +--------------+
	  |         |   |         |   
	  |         |   |         |   + Alignment ---+
	2 |         |   |         |   |  * Male      |
	  |         |   |         |   |  * Female    |
	  |         |   |         |   +--------------+
	3 |         |   |         |   ...stretch...
	  |         |   |         |   
	4 |         |   |         |   [ Random ]
	5 |         |   |         |   [  Play  ]
	6 |         |   |         |   [  Quit  ]
	  +---------+   +---------+   
    */

    QGridLayout *l = new QGridLayout(this);
    l->setColumnStretch(2, 1);
    sizePolicy().setHorizontalPolicy(QSizePolicy::Minimum);

    QGroupBox* namebox = new QGroupBox("Name", this);
    QVBoxLayout *namelayout = new QVBoxLayout(namebox);
    QLineEdit* name = new QLineEdit(namebox);
    namelayout->addWidget(name);
    name->setMaxLength(sizeof(plname)-1);
    if ( strncmp(plname,"player",6) && strncmp(plname,"games",5) )
	name->setText(plname);
    connect(name, SIGNAL(textChanged(const QString&)),
	    this, SLOT(selectName(const QString&)) );
    name->setFocus();
    QGroupBox* genderbox = new QGroupBox("Gender",this);
    QButtonGroup *gendergroup = new QButtonGroup(this);
    QGroupBox* alignbox = new QGroupBox("Alignment",this);
    QButtonGroup *aligngroup = new QButtonGroup(this);
    QVBoxLayout* vbgb = new QVBoxLayout(genderbox);
    QVBoxLayout* vbab = new QVBoxLayout(alignbox);
    char versionbuf[QBUFSZ];
    QLabel* logo = new QLabel(QString(nh_attribution).arg(version_string(versionbuf)), this);

    l->addWidget( namebox, 0,0,1,3 );
#ifdef QT_CHOOSE_RACE_FIRST
    race = new NhPSListView(this);
    role = new NhPSListView(this);
    l->addWidget( race, 1,0,6,1 );
    l->addWidget( role, 1,1,6,1 );
#else
    role = new NhPSListView(this);
    race = new NhPSListView(this);
    l->addWidget( role, 1,0,6,1 );
    l->addWidget( race, 1,1,6,1 );
#endif

    l->addWidget( genderbox, 1, 2 );
    l->addWidget( alignbox, 2, 2 );
    l->addWidget( logo, 3, 2, Qt::AlignCenter );
    l->setRowStretch( 3, 6 );

    int i;
    int nrole;

    chosen_gend = flags.initgend;
    chosen_align = flags.initalign;

    // XXX QListView unsorted goes in rev.
    for (nrole=0; roles[nrole].name.m; nrole++)
	;
    role->setRowCount(nrole);
    for (i=0; roles[i].name.m; i++) {
	QTableWidgetItem *item = new QTableWidgetItem(
		QIcon(qt_settings->glyphs().glyph(roles[i].malenum)),
		roles[i].name.m);
	item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
	role->setItem(i, 0, item);
    }
    connect( role, SIGNAL(currentCellChanged(int, int, int, int)), this, SLOT(selectRole(int, int, int, int)) );
    role->setHorizontalHeaderLabels(QStringList("Role"));
    role->resizeColumnToContents(0);

    int nrace;
    for (nrace=0; races[nrace].noun; nrace++)
	;
    race->setRowCount(nrace);
    for (i=0; races[i].noun; i++) {
	QTableWidgetItem *item = new QTableWidgetItem(
		QIcon(qt_settings->glyphs().glyph(races[i].malenum)),
		races[i].noun);
	item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
	race->setItem(i, 0, item);
    }
    connect( race, SIGNAL(currentCellChanged(int, int, int, int)), this, SLOT(selectRace(int, int, int, int)) );
    race->setHorizontalHeaderLabels(QStringList("Race"));
    race->resizeColumnToContents(0);

    gender = new QRadioButton*[ROLE_GENDERS];
    for (i=0; i<ROLE_GENDERS; i++) {
	gender[i] = new QRadioButton( genders[i].adj, genderbox );
	genderbox->layout()->addWidget(gender[i]);
	gendergroup->addButton(gender[i], i);
    }
    connect( gendergroup, SIGNAL(buttonPressed(int)), this, SLOT(selectGender(int)) );

    alignment = new QRadioButton*[ROLE_ALIGNS];
    for (i=0; i<ROLE_ALIGNS; i++) {
	alignment[i] = new QRadioButton( aligns[i].adj, alignbox );
	alignbox->layout()->addWidget(alignment[i]);
	aligngroup->addButton(alignment[i], i);
    }
    connect( aligngroup, SIGNAL(buttonPressed(int)), this, SLOT(selectAlignment(int)) );

    QPushButton* rnd = new QPushButton("Random",this);
    l->addWidget( rnd, 4, 2 );
    rnd->setDefault(false);
    connect( rnd, SIGNAL(clicked()), this, SLOT(Randomize()) );

    QPushButton* ok = new QPushButton("Play",this);
    l->addWidget( ok, 5, 2 );
    ok->setDefault(true);
    connect( ok, SIGNAL(clicked()), this, SLOT(accept()) );

    QPushButton* cancel = new QPushButton("Quit",this);
    l->addWidget( cancel, 6, 2 );
    connect( cancel, SIGNAL(clicked()), this, SLOT(reject()) );

    Randomize();
}

void NetHackQtPlayerSelector::Randomize()
{
    int nrole = role->rowCount();
    int nrace = race->rowCount();

    boolean picksomething = (flags.initrole == ROLE_NONE
                             || flags.initrace == ROLE_NONE
                             || flags.initgend == ROLE_NONE
                             || flags.initalign == ROLE_NONE);

    if (flags.randomall && picksomething) {
        if (flags.initrole == ROLE_NONE)
            flags.initrole = ROLE_RANDOM;
        if (flags.initrace == ROLE_NONE)
            flags.initrace = ROLE_RANDOM;
        if (flags.initgend == ROLE_NONE)
            flags.initgend = ROLE_RANDOM;
        if (flags.initalign == ROLE_NONE)
            flags.initalign = ROLE_RANDOM;
    }

    rigid_role_checks();

    // Randomize race and role, unless specified in config
    int ro = flags.initrole;
    if (ro == ROLE_NONE || ro == ROLE_RANDOM) {
	ro = rn2(nrole);
	if (flags.initrole != ROLE_RANDOM) {
	    fully_specified_role = false;
	}
    }
    int ra = flags.initrace;
    if (ra == ROLE_NONE || ra == ROLE_RANDOM) {
	ra = rn2(nrace);
	if (flags.initrace != ROLE_RANDOM) {
	    fully_specified_role = false;
	}
    }

    // make sure we have a valid combination, honoring
    // the users request if possible.
    bool choose_race_first;
#ifdef QT_CHOOSE_RACE_FIRST
    choose_race_first = true;
    if (flags.initrole >= 0 && flags.initrace < 0) {
	choose_race_first = false;
    }
#else
    choose_race_first = false;
    if (flags.initrace >= 0 && flags.initrole < 0) {
	choose_race_first = true;
    }
#endif
    while (!validrace(ro,ra)) {
	if (choose_race_first) {
	    ro = rn2(nrole);
	    if (flags.initrole != ROLE_RANDOM) {
	        fully_specified_role = false;
	    }
	} else {
	    ra = rn2(nrace);
	    if (flags.initrace != ROLE_RANDOM) {
	        fully_specified_role = false;
	    }
	}
    }

    int g = flags.initgend;
    if (g == -1) {
	g = rn2(ROLE_GENDERS);
	fully_specified_role = false;
    }
    while (!validgend(ro,ra,g)) {
	g = rn2(ROLE_GENDERS);
    }
    gender[g]->setChecked(true);
    selectGender(g);

    int a = flags.initalign;
    if (a == -1) {
	a = rn2(ROLE_ALIGNS);
	fully_specified_role = false;
    }
    while (!validalign(ro,ra,a)) {
	a = rn2(ROLE_ALIGNS);
    }
    alignment[a]->setChecked(true);
    selectAlignment(a);

    role->setCurrentCell(ro, 0);

    race->setCurrentCell(ra, 0);
}

void NetHackQtPlayerSelector::selectName(const QString& n)
{
    str_copy(plname,n.toLatin1().constData(),SIZE(plname));
}

void NetHackQtPlayerSelector::selectRole(int crow, int ccol, int prow, int pcol)
{
    int ra = race->currentRow();
    int ro = role->currentRow();
    if (ra == -1 || ro == -1) return;
    QTableWidgetItem* item;
    item = role->item(prow, 0);
    if (item != NULL)
       item->setSelected(false);

#ifdef QT_CHOOSE_RACE_FIRST
    selectRace(crow, ccol, prow, pcol);
#else
    QTableWidgetItem* i=role->currentItem();
    QTableWidgetItem* valid=0;
    int j;
    for (j=0; roles[j].name.m; j++) {
	bool v = validrace(j,ra);
	item = role->item(j, 0);
	item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
	if ( !valid && v ) valid = item;
    }
    if ( !validrace(role->currentRow(),ra) )
	i = valid;
    role->setCurrentItem(i, 0);
    for (j=0; roles[j].name.m; j++) {
	item = role->item(j, 0);
	item->setSelected(item == i);
	bool v = validrace(j,ra);
	item->setFlags(
		v ? Qt::ItemIsEnabled|Qt::ItemIsSelectable
		  : Qt::NoItemFlags);
    }
#endif

    //flags.initrole = role->currentRow();
    setupOthers();
}

void NetHackQtPlayerSelector::selectRace(int crow, int ccol, int prow, int pcol)
{
    int ra = race->currentRow();
    int ro = role->currentRow();
    if (ra == -1 || ro == -1) return;
    QTableWidgetItem* item;
    item = race->item(prow, 0);
    if (item != NULL)
       item->setSelected(false);

#ifndef QT_CHOOSE_RACE_FIRST
    selectRole(crow, ccol, prow, pcol);
#else
    QTableWidgetItem* i=race->currentItem();
    QTableWidgetItem* valid=0;
    int j;
    for (j=0; races[j].noun; j++) {
	bool v = validrace(ro,j);
	item = race->item(j, 0);
	item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
	if ( !valid && v ) valid = item;
    }
    if ( !validrace(ro,race->currentRow()) )
	i = valid;
    for (j=0; races[j].noun; j++) {
	item = race->item(j, 0);
	item->setSelected(item == i);
	bool v = validrace(ro,j);
	item->setFlags(
		v ? Qt::ItemIsEnabled|Qt::ItemIsSelectable
		  : Qt::NoItemFlags);
    }
#endif

    //flags.initrace = race->currentRow();
    setupOthers();
}

void NetHackQtPlayerSelector::setupOthers()
{
    int ro = role->currentRow();
    int ra = race->currentRow();
    int valid=-1;
    int c=0;
    int j;
    for (j=0; j<ROLE_GENDERS; j++) {
	bool v = validgend(ro,ra,j);
	if ( gender[j]->isChecked() )
	    c = j;
	gender[j]->setEnabled(v);
	if ( valid<0 && v ) valid = j;
    }
    if ( !validgend(ro,ra,c) )
	c = valid;
    int k;
    for (k=0; k<ROLE_GENDERS; k++) {
	gender[k]->setChecked(c==k);
    }
    selectGender(c);

    valid=-1;
    for (j=0; j<ROLE_ALIGNS; j++) {
	bool v = validalign(ro,ra,j);
	if ( alignment[j]->isChecked() )
	    c = j;
	alignment[j]->setEnabled(v);
	if ( valid<0 && v ) valid = j;
    }
    if ( !validalign(ro,ra,c) )
	c = valid;
    for (k=0; k<ROLE_ALIGNS; k++) {
	alignment[k]->setChecked(c==k);
    }
    selectAlignment(c);
}

void NetHackQtPlayerSelector::selectGender(int i)
{
    chosen_gend = i;
}

void NetHackQtPlayerSelector::selectAlignment(int i)
{
    chosen_align = i;
}

void NetHackQtPlayerSelector::Quit()
{
    done(R_Quit);
}

void NetHackQtPlayerSelector::Random()
{
    done(R_Rand);
}

bool NetHackQtPlayerSelector::Choose()
{
    if (fully_specified_role) return true;

#if defined(QWS) // probably safe with Qt 3, too (where show!=exec in QDialog).
    if ( qt_compact_mode ) {
	showMaximized();
    } else
#endif
    {
	adjustSize();
	centerOnMain(this);
    }

    if ( exec() ) {
        flags.initrace = race->currentRow();
        flags.initrole = role->currentRow();
        flags.initgend = chosen_gend;
        flags.initalign = chosen_align;
	return true;
    } else {
	return false;
    }
}

} // namespace nethack_qt4
