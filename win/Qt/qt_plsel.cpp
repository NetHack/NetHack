// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_plsel.cpp -- player selector dialog

//
// TODO:
//  increase height so that no scrolling is needed for role list [needs
//    to be done properly instead of forcing logo string to be taller]
//  make race first vs role first dynamically selectable (tty allows
//    gender first and alignment first too);
//  maybe add a set of radio buttons for normal mode vs explore mode
//    [vs wizard mode if eligible]
//  gray out character name / disable name specification box if player
//    shouldn't be allowed to change it ("wizard" for debug mode, use
//    of some forced value for multi-user system).
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

//
// None of these extra classes seem to be used except for NhPSListView. [pr]
// If NhPSListViewRole and NhPSListViewRace ever start being used,
// they'll need access to NetHackQtPlayerSelector::chosen_gend to use
// the correct tile for the icon.
//

class NhPSListViewItem : public QTableWidgetItem {
public:
    NhPSListViewItem( QTableWidget* parent UNUSED, const QString& name ) :
	QTableWidgetItem(name)
    {
    }

    void setGlyph(int g, int tileidx)
    {
	NetHackQtGlyphs& glyphs = qt_settings->glyphs();
	int gw = glyphs.width();
	int gh = glyphs.height();
	QPixmap pm(gw,gh);
	QPainter p(&pm);
        glyphs.drawGlyph(p, g, tileidx, 0, 0, false);
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
	glyph_info gi;
	int glyph = monnum_to_glyph(roles[id].mnum, MALE);
	map_glyphinfo(0, 0, glyph, 0, &gi);
	setGlyph(glyph, gi.gm.tileidx);
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
	glyph_info gi;
	int glyph = monnum_to_glyph(races[id].mnum, MALE);
	map_glyphinfo(0, 0, glyph, 0, &gi);
	setGlyph(glyph, gi.gm.tileidx);
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

// constructor for player's name+role/race/gender/alignment selection
NetHackQtPlayerSelector::NetHackQtPlayerSelector(
        NetHackQtKeyBuffer& ks UNUSED) :
    QDialog(NetHackQtBind::mainWidget()),
    fully_specified_role(true),
    chosen_gend(ROLE_NONE),
    chosen_align(ROLE_NONE),
    cleric_role_row(0),
    human_race_row(0),
    rand_btn(new QPushButton("Random")),
    play_btn(new QPushButton("Play")),
    quit_btn(new QPushButton("Quit"))
{
    /*
               0              1              2
	  + Name --------------------------------------+
	0 |                                            |
	  + -------------------------------------------+
	  + Race ----+   + Role ----+   + Gender ------+
	  | human    |   | Archeolog|   |  * Male      |
	1 | elf      |   | Barbarian|   |  * Female    |
	  | dwarf    |   |          |   +--------------+
	  | gnome    |   |          |
	  | orc      |   |          |   + Alignment ---+
	2 |          |   |  .       |   |  * Lawful    |
	  |          |   |  .       |   |  * Neutral   |
	  |          |   |  .       |   |  * Chaotic   |
	  |          |   |          |   +--------------+
	3 |          |   |          |   ...stretch...
	  |          |   |          |
	4 |          |   | Valkyrie |   [    Random    ]
	5 |          |   | Wizard   |   [     Play     ]
	6 |          |   |          |   [     Quit     ]
	  +----------+   +----------+
     *
     * Both Race and Role entries are actually two-part:  an icon (the map
     *   tile for the corresponding monster) and text (race or role name);
     * Race column is as tall as role one but mostly blank;
     * Role names aren't truncated in the actual display, just here; the
     *   race and role columns do have equal width;
     * Roles with gender-specific names get changed to match chosen gender;
     * Each of the four role/race/gender/alignment categories always has
     *   one entry checked; [Random] will change to a new set;
     * [Play] is selected by default if [Name] is non-empty but grayed out
     *   if it is empty;
     * [Quit] is selected by default when [Name] is empty;
     * ...stretch... is "NetHack x.y.z" in large text over
     *   "by the NetHack DevTeam" is smaller text with blank space above
     *   and below the two lines of text.
     *
     * If currently selected race isn't allowed to be some roles, they'll
     * be grayed out.  To switch to one of those, first check "human" which
     * offers access to all roles (except Valk which will be grayed out if
     * "male" is checked), pick the role of interest, and then re-pick race
     * (some of which will be grayed out if chosen role doesn't allow them).
     * To pick alignment first, check "human" and "priest[ess]" to make
     * all three alignments accessible, pick the one of interest, then pick
     * among the races and roles that are acceptable for that alignment.
     * Gender can be picked at any time, except for male when Valkyrie is
     * selected.
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
                        version_string(versionbuf, sizeof versionbuf)), this);

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

    QTableWidgetItem *item;
    int i, nrole, nrace;

    chosen_gend = flags.initgend;
    chosen_align = flags.initalign;

    // XXX QListView unsorted goes in rev.
    for (nrole=0; roles[nrole].name.m; nrole++)
	;
    role->setRowCount(nrole);
    for (i = 0; i < nrole; ++i) {
        item = new QTableWidgetItem();
        role->setItem(i, 0, item);

        if (roles[i].mnum == PM_CLERIC)
            cleric_role_row = i; // for populate_races()
    }

    for (nrace=0; races[nrace].noun; nrace++)
	;
    race->setRowCount(nrace);
    for (i = 0; i < nrace; ++i) {
        item = new QTableWidgetItem();
        race->setItem(i, 0, item);

        if (races[i].mnum == PM_HUMAN)
            human_race_row = i; // (always i==0) for populate_roles()
    }

#ifdef QT_CHOOSE_RACE_FIRST
    populate_races();
    populate_roles();
#else
    populate_roles();
    populate_races();
#endif

    connect(role, SIGNAL(currentCellChanged(int, int, int, int)),
            this, SLOT(selectRole(int, int, int, int)));
    role->setHorizontalHeaderLabels(QStringList("Role"));
    role->resizeColumnToContents(0);

    connect(race, SIGNAL(currentCellChanged(int, int, int, int)),
            this, SLOT(selectRace(int, int, int, int)));
    race->setHorizontalHeaderLabels(QStringList("Race"));
    race->resizeColumnToContents(0);

    // TODO:
    //  Render the alignment and gender labels smaller to match the
    //  horizontal header labels for role and race.  (Getting the font from
    //  race table above and setting it for labels below made no difference.)

    QLabel *gendlabel = new QLabel("Gender");
    genderbox->layout()->addWidget(gendlabel);
    gender = new QRadioButton*[ROLE_GENDERS];
    for (i=0; i<ROLE_GENDERS; i++) {
	gender[i] = new QRadioButton( genders[i].adj, genderbox );
	genderbox->layout()->addWidget(gender[i]);
	gendergroup->addButton(gender[i], i);
    }
    connect(gendergroup, SIGNAL(buttonClicked(int)),
            this, SLOT(selectGender(int)));

    QLabel *alignlabel = new QLabel("Alignment");
    alignbox->layout()->addWidget(alignlabel);
    alignment = new QRadioButton*[ROLE_ALIGNS];
    for (i=0; i<ROLE_ALIGNS; i++) {
	alignment[i] = new QRadioButton( aligns[i].adj, alignbox );
	alignbox->layout()->addWidget(alignment[i]);
	aligngroup->addButton(alignment[i], i);
    }
    connect(aligngroup, SIGNAL(buttonClicked(int)),
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

// update the role column in the PlayerSelector widget
void
NetHackQtPlayerSelector::populate_roles()
{
    //
    // entry for each row in the role column shows a player-character tile
    // (always gender-specific) and role's name (sometimes gender-specific)
    //
    QTableWidgetItem *item;
    const char *rolename;
    glyph_info gi;
    int v, gf, gn = chosen_gend, al = chosen_align;
    // if no race yet, we use human for gender check (gender doesn't affect
    // race but we need a valid race when filtering Valkyrie out or back in)
    int ra = race->currentRow(), hu = human_race_row;
    bool is_f = (gn == 1);
    NetHackQtGlyphs& glyphs = qt_settings->glyphs();
    for (int i = 0; roles[i].name.m; ++i) {
        rolename = (is_f && roles[i].name.f) ? roles[i].name.f
                                             : roles[i].name.m;
        gf = monnum_to_glyph(roles[i].mnum, is_f ? FEMALE : MALE);
        map_glyphinfo(0, 0, gf, 0, &gi);
        v = ((ra < 0 || validrace(i, ra))
             && (gn < 0 || validgend(i, (ra >= 0) ? ra : hu, gn))
             && (al < 0 || validalign(i, (ra >= 0) ? ra : hu, al)));
        item = role->item(i, 0);
        item->setText(rolename);
        item->setIcon(QIcon(glyphs.glyph(gf, gi.gm.tileidx)));
        item->setFlags(v ? Qt::ItemIsEnabled | Qt::ItemIsSelectable
                         : Qt::NoItemFlags);
    }
    role->repaint(); // role->update() seems to be inadequate
}

// update the race column in the PlayerSelector widget
void
NetHackQtPlayerSelector::populate_races()
{
    //
    // entry for each row in race column shows race's generic monster tile
    // (always gender-specific) and race's name (never gender-specific)
    //
    QTableWidgetItem *item;
    glyph_info gi;
    int v, gf, gn = chosen_gend, al = chosen_align;
    // if no role yet, use cleric so that alignment won't rule anything out
    int ro = role->currentRow(), cl = cleric_role_row;
    bool is_f = (gn == 1);
    NetHackQtGlyphs& glyphs = qt_settings->glyphs();
    for (int j = 0; races[j].noun; ++j) {
        gf = monnum_to_glyph(races[j].mnum, is_f ? FEMALE : MALE);
        map_glyphinfo(0, 0, gf, 0, &gi);
        v = ((ro < 0 || validrace(ro, j))
             && (al < 0 || validalign((ro >= 0) ? ro : cl, j, al)));
        item = race->item(j, 0);
        item->setText(races[j].noun);
        item->setIcon(QIcon(glyphs.glyph(gf, gi.gm.tileidx)));
        item->setFlags(v ? Qt::ItemIsEnabled | Qt::ItemIsSelectable
                         : Qt::NoItemFlags);
    }
    race->repaint();
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
    if (g < 0) {
	g = rn2(ROLE_GENDERS);
	fully_specified_role = false;
    }
    while (!validgend(ro,ra,g)) {
	g = rn2(ROLE_GENDERS);
    }
    gender[g]->setChecked(true);
    selectGender(g);

    int a = flags.initalign;
    if (a < 0) {
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
    if (ra == -1 || ro == -1)
        return;
    QTableWidgetItem *item = role->item(prow, 0);
    if (item != NULL)
       item->setSelected(false);

#ifdef QT_CHOOSE_RACE_FIRST
    selectRace(crow, ccol, prow, pcol);
#else
    QTableWidgetItem *i = role->currentItem();
    QTableWidgetItem *valid = 0;
    for (int j = 0; roles[j].name.m; ++j) {
	if (!valid && (ra < 0 || validrace(j, ra))) {
            valid = role->item(j, 0);
            break;
        }
    }
    if (!validrace(role->currentRow(), ra))
	i = valid;
    role->setCurrentItem(i, 0);
    for (int j = 0; roles[j].name.m; ++j) {
	item = role->item(j, 0);
	item->setSelected(item == i);
        /* used to call setFlags here, but setupOthers() -> selectGender()
           (and selectAlignment()) -> populate_roles() takes care of that */
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
    if (ra == -1 || ro == -1)
        return;
    QTableWidgetItem *item = race->item(prow, 0);
    if (item != NULL)
       item->setSelected(false);

#ifndef QT_CHOOSE_RACE_FIRST
    selectRole(crow, ccol, prow, pcol);
#else
    QTableWidgetItem *i = race->currentItem();
    QTableWidgetItem *valid = 0;
    for (int j = 0; races[j].noun; ++j) {
	if (!valid && (ro < 0 || validrace(ro, j))) {
            valid = race->item(j, 0);
            break;
        }
    }
    if (!validrace(ro, race->currentRow()))
	i = valid;
    for (int j = 0; races[j].noun; ++j) {
	item = race->item(j, 0);
	item->setSelected(item == i);
        /* used to call setFlags here, but setupOthers() -> selectGender()
           (and selectAlignment()) -> populate_races() takes care of that */
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
    // if gender has changed, tiles and some role titles will change
    populate_roles();
    populate_races();
}

void NetHackQtPlayerSelector::selectAlignment(int i)
{
    chosen_align = i;
    // if alignment has changed, some roles or races may no longer be
    // available and some previously excluded ones might become available
    populate_roles();
    populate_races();
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
    if (fully_specified_role)
        return true;

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
