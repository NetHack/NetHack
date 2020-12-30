// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_plsel.cpp -- player selector dialog

//
// TODO:
//  increase height so that no scrolling is needed for role list [needs
//    to be done properly instead of forcing logo string to be taller]
//  the [Random] button doesn't do anything;
//  make race first vs role first dynamically selectable (tty allows
//    gender first and alignment first too);
//  maybe add a set of radio buttons for normal mode vs explore mode
//    [vs wizard mode if eligible]
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
#include "qt_plsel.h"
#include "qt_plsel.moc"
#include "qt_bind.h"
#include "qt_glyph.h"
#include "qt_set.h"
#include "qt_str.h"

// Warwick prefers it this way...
#define QT_CHOOSE_RACE_FIRST

/* check whether plname[] is among the list of generic user names */
static bool generic_plname()
{
    if (*g.plname) {
        const char *sptr, *p;
        const char *genericusers = sysopt.genericusers;
        int ln = (int) strlen(g.plname);

        if (!genericusers || !*genericusers)
            genericusers = "player games";
        else if (!strcmp(genericusers, "*")) /* "*" => always ask for name */
            return true;

        while ((sptr = strstri(genericusers, g.plname)) != NULL) {
            /* check for full word: start of list or following a space */
            if ((sptr == genericusers || sptr[-1] == ' ')
                /* and also preceding a space or at end of list */
                && (sptr[ln] == ' ' || sptr[ln] == '\0'))
                return true;
            /* doesn't match full word, but maybe we got a false hit when
               looking for "play" in list "player play" so keep going */
            if ((p = strchr(sptr + 1, ' ')) == NULL)
                break;
            genericusers = p + 1;
        }
    }
    return false;
}

namespace nethack_qt_ {

// temporary
void centerOnMain( QWidget* w );
// end temporary

// hack: padded with blank lines by inserting breaks above and below in
// order to force window to be tall enough to show all the roles at once
static const char nh_attribution[] = "<br><center><big>NetHack %1</big>"
        "<br><small>by the NetHack DevTeam</small></center><br>";

class NhPSListViewItem : public QTableWidgetItem {
public:
    NhPSListViewItem( QTableWidget* parent UNUSED, const QString& name ) :
	QTableWidgetItem(name)
    {
    }

    void setGlyph(int g, bool fem)
    {
	NetHackQtGlyphs& glyphs = qt_settings->glyphs();
	int gw = glyphs.width();
	int gh = glyphs.height();
	QPixmap pm(gw,gh);
	QPainter p(&pm);
	glyphs.drawGlyph(p, g, 0, 0, fem);
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
	setGlyph(monnum_to_glyph(roles[id].malenum), false);
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
	setGlyph(monnum_to_glyph(races[id].malenum), false);
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

NetHackQtPlayerSelector::NetHackQtPlayerSelector(NetHackQtKeyBuffer& ks UNUSED) :
    QDialog(NetHackQtBind::mainWidget()),
    fully_specified_role(true),
    chosen_gend(ROLE_NONE),
    chosen_align(ROLE_NONE),
    rand_btn(new QPushButton("Random")),
    play_btn(new QPushButton("Play")),
    quit_btn(new QPushButton("Quit"))
{
    /*
               0             1             2
	  + Name ------------------------------------+
	0 |                                          |
	  + ---- ------------------------------------+
	  + Race ---+   + Role ---+   + Gender ------+
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
    name->setMaxLength(PL_NSIZ - 1);
    name->setPlaceholderText(QString("  (required)")); // grayed out

    // if plname[] contains a generic user name, clear it
    if (generic_plname())
        *g.plname = '\0';
    name->setText(g.plname);
    connect(name, SIGNAL(textChanged(const QString&)),
            this, SLOT(selectName(const QString&)));
    name->setFocus();

    // changed to move gender and alignment labels inside their boxes (below)
    QGroupBox *genderbox = new QGroupBox();
    QButtonGroup *gendergroup = new QButtonGroup(this);
    QGroupBox *alignbox = new QGroupBox();
    QButtonGroup *aligngroup = new QButtonGroup(this);
    // these two QVBoxLayout pointers aren't used, the vertical box layouts
    // being assigned to them are...
    QVBoxLayout* vbgb UNUSED = new QVBoxLayout(genderbox);
    QVBoxLayout* vbab UNUSED = new QVBoxLayout(alignbox);
    char versionbuf[QBUFSZ];
    QLabel *logo = new QLabel(QString(nh_attribution).arg(
                                           version_string(versionbuf)), this);

    l->addWidget( namebox, 0,0,1,3 );
    role = new NhPSListView(this);
    race = new NhPSListView(this);
#ifdef QT_CHOOSE_RACE_FIRST
    l->addWidget( race, 1,0,6,1 );
    l->addWidget( role, 1,1,6,1 );
#else
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
    bool fem = (chosen_gend > ROLE_NONE);

    // XXX QListView unsorted goes in rev.
    for (nrole=0; roles[nrole].name.m; nrole++)
	;
    role->setRowCount(nrole);
    for (i=0; roles[i].name.m; i++) {
	QTableWidgetItem *item = new QTableWidgetItem(
                QIcon(qt_settings->glyphs().glyph(roles[i].malenum, fem)),
                roles[i].name.m);
	item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
	role->setItem(i, 0, item);
    }
    connect(role, SIGNAL(currentCellChanged(int, int, int, int)),
            this, SLOT(selectRole(int, int, int, int)));
    role->setHorizontalHeaderLabels(QStringList("Role"));
    role->resizeColumnToContents(0);

    int nrace;
    for (nrace=0; races[nrace].noun; nrace++)
	;
    race->setRowCount(nrace);
    for (i=0; races[i].noun; i++) {
	QTableWidgetItem *item = new QTableWidgetItem(
                QIcon(qt_settings->glyphs().glyph(races[i].malenum, fem)),
		races[i].noun);
	item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
	race->setItem(i, 0, item);
    }
    connect(race, SIGNAL(currentCellChanged(int, int, int, int)),
            this, SLOT(selectRace(int, int, int, int)));
    race->setHorizontalHeaderLabels(QStringList("Race"));
    race->resizeColumnToContents(0);

    // TODO:
    //  Render the alignment and gender labels smaller to match the
    //  horizontal header labels for role and race; getting the font from
    //  race table above and setting it for labels below made no difference.
    //
    // Maybe, if the order of choosing becomes more dynamic:
    //  Replace the role and race glyphs when gender gets set.

    QLabel *gendlabel = new QLabel("Gender");
    genderbox->layout()->addWidget(gendlabel);
    gender = new QRadioButton*[ROLE_GENDERS];
    for (i=0; i<ROLE_GENDERS; i++) {
	gender[i] = new QRadioButton( genders[i].adj, genderbox );
	genderbox->layout()->addWidget(gender[i]);
	gendergroup->addButton(gender[i], i);
    }
    connect(gendergroup, SIGNAL(buttonPressed(int)),
            this, SLOT(selectGender(int)));

    QLabel *alignlabel = new QLabel("Alignment");
    alignbox->layout()->addWidget(alignlabel);
    alignment = new QRadioButton*[ROLE_ALIGNS];
    for (i=0; i<ROLE_ALIGNS; i++) {
	alignment[i] = new QRadioButton( aligns[i].adj, alignbox );
	alignbox->layout()->addWidget(alignment[i]);
	aligngroup->addButton(alignment[i], i);
    }
    connect(aligngroup, SIGNAL(buttonPressed(int)),
            this, SLOT(selectAlignment(int)));

    l->addWidget(rand_btn, 4, 2);
    connect(rand_btn, SIGNAL(clicked()), this, SLOT(Randomize()));
    l->addWidget(play_btn, 5, 2);
    connect(play_btn, SIGNAL(clicked()), this, SLOT(accept()));
    l->addWidget(quit_btn, 6, 2);
    connect(quit_btn, SIGNAL(clicked()), this, SLOT(reject()));
    // if plname[] is non-empty, the Play button is enabled and the default;
    // otherwise, Play is disabled and Quit is the default
    plnamePlayVsQuit();

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
    // the user's request if possible.
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

// if plname[] is empty, disable [Play], otherwise [Play] is the default
void NetHackQtPlayerSelector::plnamePlayVsQuit()
{
    if (*g.plname) {
        play_btn->setEnabled(true);
        play_btn->setDefault(true);
        //quit_btn->setDefault(false);
    } else {
        play_btn->setEnabled(false); // [Play] still visible but grayed out
        //play_btn->setDefault(false);
        quit_btn->setDefault(true);
    }
}

// the line edit widget for the name field has received input
void NetHackQtPlayerSelector::selectName(const QString& n)
{
    const char *name_str = n.toLatin1().constData();
    // skip any leading spaces
    // (it would be better to set up a validator that rejects leading spaces)
    while (*name_str == ' ')
        ++name_str;
    str_copy(g.plname, name_str, PL_NSIZ);
    // possibly enable or disable the [Play] button
    plnamePlayVsQuit();
}

void NetHackQtPlayerSelector::selectRole(int crow, int ccol,
                                         int prow, int pcol)
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
        item->setFlags(v ? Qt::ItemIsEnabled|Qt::ItemIsSelectable
                         : Qt::NoItemFlags);
    }
    nhUse(crow);
    nhUse(ccol);
    nhUse(pcol);
#endif

    //flags.initrole = role->currentRow();
    setupOthers();
}

void NetHackQtPlayerSelector::selectRace(int crow, int ccol,
                                         int prow, int pcol)
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
        item->setFlags(v ? Qt::ItemIsEnabled|Qt::ItemIsSelectable
                         : Qt::NoItemFlags);
    }
    nhUse(crow);
    nhUse(ccol);
    nhUse(pcol);
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

} // namespace nethack_qt_
