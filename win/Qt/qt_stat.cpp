// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_stat.cpp -- status window, upper right portion of the overall window
//
// The Qt status window consists of many lines:
//
//      hitpoint bar (when enabled)
//      Title (plname the Rank or plname the MonsterSpecies)
//      Dungeon location (branch and level)
//      separator line
//      six icons (special 40x40 tiles, paired with...)
//      six characteristic texts ("Str:18/03", "Dex:15", &c)
//      separator line
//      five status fields without icons (some containing two values:
//        HP/HPmax, Energy/Enmax, AC, XpLevel/ExpPoints or HD, [blank], Gold)
//      separator line
//      line with two optional text fields (Time:1234, Score:89), maybe blank
//      varying number of icons (one or more, each paired with...)
//      corresponding text (Alignment plus zero or more status conditions
//        including Hunger if not "normal" and encumbrance if not "normal")
//
// The hitpoint bar spans the width of the status window when enabled.
// Title and location are centered.
// The icons and text for the six characteristics are evenly spaced;
//   this pair of lines is sometimes referred to as "row 1" below.
// The five main stats or slash-separated stat pairs are padded with an
//   empty slot between Xp and Gold; adding the sixth makes that row
//   line up with the characteristics; this line is sometimes referred
//   to as "row 2".
// Time and Score are spaced as if each were three fields wide; their
//   line is "row 3" relative to statuslines:2 vs statuslines:3.
// Icons and texts for alignment and conditions are left justified.
// The separator lines are thin and don't take up much vertical space.
// When enabled, the hitpoint bar bisects the margin above Title,
//   increasing the overall status height by 9 pixels; when disabled,
//   the status shifts up by those 9 pixels.
// When row 3 (Time, Score) is blank, it still takes up the vertical
//   space that would be used to show those values.
//
// The above is for statuslines:3, which used to be the default.  For
//   statuslines:2, rows 1 and 2 are extended from six to seven fields
//   and row 3 (optional Time, Score) is eliminated.  Alignment is
//   moved from the beginning of the Conditions pair (icon over text)
//   of lines up to the end of row 1, the Characteristics pair of lines,
//   with a separator between Cha:NN and it.  Time, when active, is
//   placed after Gold.  Score, if enabled and active, is shown in the
//   filler slot before Gold.  When there are no Conditions to display,
//   there is an an invisible fake one (blank icon over blank text)
//   rendered in order to preserve the vertical space they need.
//
// FIXME:
//  When hitpoint bar is shown, attempting to resize horizontally won't
//    do anything.  Toggling it off, then resizing, and back On works.
//    (Caused by specifying min-width and max-width constraints in the
//    style sheets used to control color, but removing those constraints
//    causes the bar display to get screwed up.)
//  There are separate icons for Satiated and Hungry, but Weak, Fainting,
//    and Fainted all share the Hungry one.  Weak should have its own,
//    Fainting+Fainted should have another.  The current two depict
//    plates with cutlery which is a bit of an anachronism.  Statiated
//    could be replaced by a figure in profile with a bulging belly,
//    Hungry similar but with a slightly concave belly, Weak either a
//    collapsing figure or a much larger concavity or both, Fainting/
//    Fainted a fully collapsed figure.
//
// TODO:
//  If/when status conditions become too wide for the status window, scale
//    down their icons and switch their text to a smaller font to match.
//  Title and Location are explicitly rendered with a bigger font than
//    the rest of status.  That takes up more space, which is ok, but it
//    also increases the vertical margin in between them by more than is
//    necessary.  Should squeeze some of that excess blank space out.
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
#include "qt_stat.h"
#include "qt_stat.moc"
#include "qt_set.h"
#include "qt_str.h"
#include "qt_xpms.h"

extern const char *enc_stat[]; /* from botl.c */
extern const char *hu_stat[]; /* from eat.c */

extern int qt_compact_mode;

namespace nethack_qt_ {

NetHackQtStatusWindow::NetHackQtStatusWindow() :
    /* first three rows:  hitpoint bar, title (plname the Rank), location */
    hpbar_health(this),
    hpbar_injury(this),
    name(this,"(name)"),
    dlevel(this,"(dlevel)"),
    /* next two rows:  icon over text label for the six characteristics */
    str(this, "Str"),
    dex(this, "Dex"),
    con(this, "Con"),
    intel(this, "Int"),
    wis(this, "Wis"),
    cha(this, "Cha"),
    /* sixth row, text only:  some contain two slash-separated values */
    hp(this,"Hit Points"),
    power(this,"Power"),
    ac(this,"Armor Class"),
    level(this,"Level"), // Xp level, with "/"+Exp points optionally appended
    blank1(this, ""),    // used for padding to align columns (was once 'exp')
    gold(this,"Gold"),   // gold used to be this row's first column, now last
    /* seventh row:  two optionally displayed values (just text, no icons) */
    time(this,"Time"),   // if 'time' option On
    score(this,"Score"), // if SCORE_ON_BOTL defined and 'showscore' option On
    /* last two rows:  alignment followed by conditions (icons over text) */
    align(this,"Alignment"),
    blank2(this, " "),   // used to prevent Conditions row from being empty
    hunger(this,""),
    encumber(this,""),
    stoned(this,"Stone"),     // major conditions
    slimed(this,"Slime"),
    strngld(this,"Strngl"),
    sick_fp(this,"FoodPois"),
    sick_il(this,"TermIll"),
    stunned(this,"Stun"),     // minor conditions
    confused(this,"Conf"),
    hallu(this,"Hallu"),
    blind(this,"Blind"),
    deaf(this,"Deaf"),
    lev(this,"Lev"),          // 'other' conditions
    fly(this,"Fly"),
    ride(this,"Ride"),
    hline1(this),             // separators
    hline2(this),
    hline3(this),
    vline1(this), // vertical separator between Characteristics and Alignment
    vline2(this), // padding for row 2 to match row 1's separator; not shown
    /* miscellaneous; not display fields */
    cursy(0),
    first_set(true),
    alreadyfullhp(false),
    was_polyd(false),
    had_exp(false),
    had_score(false)
{
    if (!qt_compact_mode) {
        int w = NetHackQtBind::mainWidget()->width();
        setMaximumWidth(w / 2);
    }
    // for tool tips; they mostly work without this but sometimes changes
    // to hunger or encumbrance seemed to cause tip display to stop
    setMouseTracking(true);

    p_str = QPixmap(str_xpm);
    p_str = QPixmap(str_xpm);
    p_dex = QPixmap(dex_xpm);
    p_con = QPixmap(cns_xpm);
    p_int = QPixmap(int_xpm);
    p_wis = QPixmap(wis_xpm);
    p_cha = QPixmap(cha_xpm);

    p_chaotic = QPixmap(chaotic_xpm);
    p_neutral = QPixmap(neutral_xpm);
    p_lawful = QPixmap(lawful_xpm);
    p_blank2 = QPixmap(blank_xpm);

    p_satiated = QPixmap(satiated_xpm);
    p_hungry = QPixmap(hungry_xpm);

    p_encumber[0] = QPixmap(slt_enc_xpm);
    p_encumber[1] = QPixmap(mod_enc_xpm);
    p_encumber[2] = QPixmap(hvy_enc_xpm);
    p_encumber[3] = QPixmap(ext_enc_xpm);
    p_encumber[4] = QPixmap(ovr_enc_xpm);

    p_stoned = QPixmap(stone_xpm);
    p_slimed = QPixmap(slime_xpm);
    p_strngld = QPixmap(strngl_xpm);
    p_sick_fp = QPixmap(sick_fp_xpm);
    p_sick_il = QPixmap(sick_il_xpm);
    p_stunned = QPixmap(stunned_xpm);
    p_confused = QPixmap(confused_xpm);
    p_hallu = QPixmap(hallu_xpm);
    p_blind = QPixmap(blind_xpm);
    p_deaf = QPixmap(deaf_xpm);
    p_lev = QPixmap(lev_xpm);
    p_fly = QPixmap(fly_xpm);
    p_ride = QPixmap(ride_xpm);

    str.setIcon(p_str, "strength");
    dex.setIcon(p_dex, "dexterity");
    con.setIcon(p_con, "constitution");
    intel.setIcon(p_int, "intelligence");
    wis.setIcon(p_wis, "wisdom");
    cha.setIcon(p_cha, "charisma");

    align.setIcon(p_neutral);
    blank2.setIcon(p_blank2); // used for spacing when Conditions row is empty
    hunger.setIcon(p_hungry);
    encumber.setIcon(p_encumber[0]);

    stoned.setIcon(p_stoned, "turning to stone");
    slimed.setIcon(p_slimed, "turning into slime");
    strngld.setIcon(p_strngld, "being strangled");
    sick_fp.setIcon(p_sick_fp, "severe food poisoning");
    sick_il.setIcon(p_sick_il, "terminal illness");
    stunned.setIcon(p_stunned, "stunned");
    confused.setIcon(p_confused, "confused");
    hallu.setIcon(p_hallu, "hallucinating");
    blind.setIcon(p_blind, "cannot see");
    deaf.setIcon(p_deaf, "cannot hear");
    lev.setIcon(p_lev, "levitating");
    fly.setIcon(p_fly, "flying");
    ride.setIcon(p_ride, "riding");

    // separator lines
    hline1.setFrameStyle(QFrame::HLine | QFrame::Sunken);
    hline2.setFrameStyle(QFrame::HLine | QFrame::Sunken);
    hline3.setFrameStyle(QFrame::HLine | QFrame::Sunken);
    hline1.setLineWidth(1);
    hline2.setLineWidth(1);
    hline3.setLineWidth(1);
    // vertical separators for condensed layout (statuslines:2)
    vline1.setFrameStyle(QFrame::VLine | QFrame::Sunken);
    vline2.setFrameStyle(QFrame::VLine | QFrame::Sunken);
    vline1.setLineWidth(1); // separates Alignment from Charisma
    vline2.setLineWidth(1);
    vline2.hide(); // padding to keep row 2 aligned with row 1, never shown

    // set up last but shown first (above name) via layout below */
    QHBoxLayout *hpbar = InitHitpointBar();

    // 'statuslines' takes a value of 2 or 3; we use 3 as a request to put
    // Alignment in front of status conditions so that line is never empty
    // and to show Time and/or Score on their own line which might be empty
    boolean spreadout = (::iflags.wc2_statuslines != 2);

#if 1 //RLC
    name.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    dlevel.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    QVBoxLayout *vbox = new QVBoxLayout();
    vbox->setSpacing(0);
    vbox->addLayout(hpbar); // when 'hitpointbar' is enabled, it comes first
    vbox->addWidget(&name);
    vbox->addWidget(&dlevel);
    vbox->addWidget(&hline1);
    QHBoxLayout *charbox = new QHBoxLayout(); // Characteristics
        charbox->addWidget(&str);
        charbox->addWidget(&dex);
        charbox->addWidget(&con);
        charbox->addWidget(&intel);
        charbox->addWidget(&wis);
        charbox->addWidget(&cha);
        if (!spreadout) {
            // when condensed, include Alignment with Characteristics
            charbox->addWidget(&vline1); // show a short vertical separator
            charbox->addWidget(&align);
        }
    vbox->addLayout(charbox);
    vbox->addWidget(&hline2);
    QHBoxLayout *statbox = new QHBoxLayout(); // core status fields
        statbox->addWidget(&hp);
        statbox->addWidget(&power);
        statbox->addWidget(&ac);
        statbox->addWidget(&level);
        if (spreadout) {
            // when not condensed, put a blank field in front of Gold;
            // Time and Score will be shown on their own separate line
            statbox->addWidget(&blank1); // empty column #5 of 6
            statbox->addWidget(&gold);
        } else {
            // when condensed, display Time and Score on HP,...,Gold row
#ifndef SCORE_ON_BOTL
            statbox->addWidget(&blank1); // empty column #5 of 7
#else
            statbox->addWidget(&score);  // usually empty column #5
#endif
            statbox->addWidget(&gold);   // columns 6 and maybe empty 7
            statbox->addWidget(&vline2); // padding between 6 and 7; not shown
            statbox->addWidget(&time);
        }
    vbox->addLayout(statbox);
    vbox->addWidget(&hline3); // separtor before Time+Score or Conditions
    if (spreadout) {
        // when not condensed, put Time and Score on an extra row; since
        // they're both optionally displayed, their row might be empty
        // TODO? when neither will be shown, set their heights smaller
        // and if either gets toggled On, set height back to normal
        QHBoxLayout *timebox = new QHBoxLayout();
            timebox->addWidget(&time);
            timebox->addWidget(&score);
        vbox->addLayout(timebox);
    }
    QHBoxLayout *condbox = new QHBoxLayout(); // Conditions
        if (spreadout) {
            // when not condensed, include Alignment with Conditions to
            // spread things out and also so that their row is never empty
            condbox->addWidget(&align);
        } else {
            // otherwise place a padding widget on this row; it will be
            // hidden if any Conditions are shown, or shown (with blank
            // icon and empty text) when there aren't any, reserving
            // space (the height of the row) for later conditions
            condbox->addWidget(&blank2);
        }
        condbox->addWidget(&hunger);
        condbox->addWidget(&encumber);
        condbox->addWidget(&stoned);
        condbox->addWidget(&slimed);
        condbox->addWidget(&strngld);
        condbox->addWidget(&sick_fp);
        condbox->addWidget(&sick_il);
        condbox->addWidget(&stunned);
        condbox->addWidget(&confused);
        condbox->addWidget(&hallu);
        condbox->addWidget(&blind);
        condbox->addWidget(&deaf);
        condbox->addWidget(&lev);
        condbox->addWidget(&fly);
        condbox->addWidget(&ride);
    condbox->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    vbox->addLayout(condbox);
    setLayout(vbox);
#endif

    connect(qt_settings, SIGNAL(fontChanged()), this, SLOT(doUpdate()));
    doUpdate();
}

void NetHackQtStatusWindow::doUpdate()
{
    const QFont& large=qt_settings->largeFont();
    name.setFont(large);
    dlevel.setFont(large);

    const QFont& normal=qt_settings->normalFont();
    str.setFont(normal);
    dex.setFont(normal);
    con.setFont(normal);
    intel.setFont(normal);
    wis.setFont(normal);
    cha.setFont(normal);
    hp.setFont(normal);
    power.setFont(normal);
    ac.setFont(normal);
    level.setFont(normal);
    blank1.setFont(normal); // padding
    gold.setFont(normal);
    time.setFont(normal);
    score.setFont(normal);
    align.setFont(normal);
    // blank2 is used as a dummy condition when Alignment has been moved
    // elsewhere (statuslines:2) and no other conditions currently apply;
    // it has a blank icon with a label of a single space (if the label
    // is completely empty, the rest of status shifts down a little when
    // one or more real conditions replace it and shifts up again when
    // all conditions are removed and this one is reinstated--as if "" is
    // slightly taller than " ")
    blank2.setFont(normal);
    hunger.setFont(normal);
    encumber.setFont(normal);
    stoned.setFont(normal);
    slimed.setFont(normal);
    strngld.setFont(normal);
    sick_fp.setFont(normal);
    sick_il.setFont(normal);
    stunned.setFont(normal);
    confused.setFont(normal);
    hallu.setFont(normal);
    blind.setFont(normal);
    deaf.setFont(normal);
    lev.setFont(normal);
    fly.setFont(normal);
    ride.setFont(normal);

    updateStats();
}

QWidget* NetHackQtStatusWindow::Widget() { return this; }

void NetHackQtStatusWindow::Clear()
{
}
void NetHackQtStatusWindow::Display(bool block UNUSED)
{
}
void NetHackQtStatusWindow::CursorTo(int,int y)
{
    cursy=y;
}
void NetHackQtStatusWindow::PutStr(int attr UNUSED, const QString& text UNUSED)
{
    // do a complete update when line 0 is done (as per X11 fancy status)
    if (cursy==0) updateStats();
}

#if 0 // RLC
void NetHackQtStatusWindow::resizeEvent(QResizeEvent*)
{
#if 0
    const float SP_name=0.13; //     <Name> the <Class> (large)
    const float SP_dlev=0.13; //   Level 3 in The Dungeons of Doom (large)
    const float SP_atr1=0.25; //  STR   DEX   CON   INT   WIS   CHA
    const float SP_hln1=0.02; // ---
    const float SP_atr2=0.09; //  Au    HP    PW    AC    LVL   EXP
    const float SP_hln2=0.02; // ---
    const float SP_time=0.09; //      time    score
    const float SP_hln3=0.02; // ---
    const float SP_stat=0.25; // Alignment, Poisoned, Hungry, Sick, etc.

    int h=height();
    int x=0,y=0;

    int iw; // Width of an item across line
    int lh; // Height of a line of values

    lh=int(h*SP_name);
    name.setGeometry(0,0,width(),lh); y+=lh;
    lh=int(h*SP_dlev);
    dlevel.setGeometry(0,y,width(),lh); y+=lh;

    lh=int(h*SP_hln1);
    hline1.setGeometry(0,y,width(),lh); y+=lh;

    lh=int(h*SP_atr1);
    iw=width()/6;
    str.setGeometry(x,y,iw,lh); x+=iw;
    dex.setGeometry(x,y,iw,lh); x+=iw;
    con.setGeometry(x,y,iw,lh); x+=iw;
    intel.setGeometry(x,y,iw,lh); x+=iw;
    wis.setGeometry(x,y,iw,lh); x+=iw;
    cha.setGeometry(x,y,iw,lh); x+=iw;
    x=0; y+=lh;

    lh=int(h*SP_hln2);
    hline2.setGeometry(0,y,width(),lh); y+=lh;

    lh=int(h*SP_atr2);
    iw=width()/6;
    gold.setGeometry(x,y,iw,lh); x+=iw;
    hp.setGeometry(x,y,iw,lh); x+=iw;
    power.setGeometry(x,y,iw,lh); x+=iw;
    ac.setGeometry(x,y,iw,lh); x+=iw;
    level.setGeometry(x,y,iw,lh); x+=iw;
    //exp.setGeometry(x,y,iw,lh); x+=iw;
    x=0; y+=lh;

    lh=int(h*SP_hln3);
    hline3.setGeometry(0,y,width(),lh); y+=lh;

    lh=int(h*SP_time);
    iw=width()/3; x+=iw/2;
    time.setGeometry(x,y,iw,lh); x+=iw;
    score.setGeometry(x,y,iw,lh); x+=iw;
    x=0; y+=lh;

    lh=int(h*SP_stat);
    iw=width()/9;
    align.setGeometry(x,y,iw,lh); x+=iw;
    hunger.setGeometry(x,y,iw,lh); x+=iw;
    encumber.setGeometry(x,y,iw,lh); x+=iw;
    stoned.setGeometry(x,y,iw,lh); x+=iw;
    slimed.setGeometry(x,y,iw,lh); x+=iw;
    strngld.setGeometry(x,y,iw,lh); x+=iw;
    sick_fp.setGeometry(x,y,iw,lh); x+=iw;
    sick_il.setGeometry(x,y,iw,lh); x+=iw;
    stunned.setGeometry(x,y,iw,lh); x+=iw;
    confused.setGeometry(x,y,iw,lh); x+=iw;
    hallu.setGeometry(x,y,iw,lh); x+=iw;
    blind.setGeometry(x,y,iw,lh); x+=iw;
    deaf.setGeometry(x,y,iw,lh); x+=iw;
    lev.setGeometry(x,y,iw,lh); x+=iw;
    fly.setGeometry(x,y,iw,lh); x+=iw;
    ride.setGeometry(x,y,iw,lh); x+=iw;
    x=0; y+=lh;
#else
    // This is clumsy.  But QLayout objects are proving balky.

    int row[10];

    row[0] = name.sizeHint().height();
    row[1] = dlevel.sizeHint().height();
    row[2] = h.sizeHint().height();
#endif
}
#endif


/*
 * Set all widget values to a null string.  This is used after all spacings
 * have been calculated so that when the window is popped up we don't get all
 * kinds of funny values being displayed.  [Actually it isn't used at all.]
 */
void NetHackQtStatusWindow::nullOut()
{
}

void NetHackQtStatusWindow::fadeHighlighting()
{
    name.dissipateHighlight();
    dlevel.dissipateHighlight();

    str.dissipateHighlight();
    dex.dissipateHighlight();
    con.dissipateHighlight();
    intel.dissipateHighlight();
    wis.dissipateHighlight();
    cha.dissipateHighlight();

    gold.dissipateHighlight();
    hp.dissipateHighlight();
    power.dissipateHighlight();
    ac.dissipateHighlight();
    level.dissipateHighlight();
    align.dissipateHighlight();

    //time.dissipateHighlight();
    score.dissipateHighlight();

    hunger.dissipateHighlight();
    encumber.dissipateHighlight();
    stoned.dissipateHighlight();
    slimed.dissipateHighlight();
    strngld.dissipateHighlight();
    sick_fp.dissipateHighlight();
    sick_il.dissipateHighlight();
    stunned.dissipateHighlight();
    confused.dissipateHighlight();
    hallu.dissipateHighlight();
    blind.dissipateHighlight();
    deaf.dissipateHighlight();
    lev.dissipateHighlight();
    fly.dissipateHighlight();
    ride.dissipateHighlight();
}

// hitpointbar: two panels: left==current health, right==missing max health
QHBoxLayout *NetHackQtStatusWindow::InitHitpointBar()
{
    hpbar_health.setFrameStyle(QFrame::NoFrame);
    hpbar_health.setMaximumHeight(9);
    hpbar_health.setAutoFillBackground(true);
    if (!iflags.wc2_hitpointbar)
        hpbar_health.hide();

    hpbar_injury.setFrameStyle(QFrame::NoFrame);
    /* health portion has thickness 9, injury portion just 3 */
    hpbar_injury.setMaximumHeight(3);
    hpbar_injury.setContentsMargins(0, 3, 0, 3); // left,top,right,bottom
    hpbar_injury.setAutoFillBackground(true);
    hpbar_injury.hide(); // only shown when hitpointbar is On and uhp < uhpmax

    QHBoxLayout *hpbar = new QHBoxLayout;
    hpbar->setSpacing(0);
#if QT_VERSION < 0x060000
    hpbar->setMargin(0);
#endif
    hpbar->addWidget(&hpbar_health);
    hpbar->setAlignment(&hpbar_health, Qt::AlignLeft);
    hpbar->addWidget(&hpbar_injury);
    hpbar->setAlignment(&hpbar_injury, Qt::AlignRight);
    return hpbar; // caller will add our result to vbox layout
}

DISABLE_WARNING_FORMAT_NONLITERAL

// when hitpoint bar is enabled, calculate and draw it, otherwise remove it
void NetHackQtStatusWindow::HitpointBar()
{
    // a style sheet is used to specify color for otherwise blank labels;
    // barcolors[][*]: column [0=left] is current health, [1=right] is injury
    static const char
    *styleformat = "QLabel { background-color : %s ; color : transparent ;"
                           " min-width : %d ; max-width %d }",
    *barcolors[6][2] = {
        { "black",   "black"     },  // 100%   /* second black never shown */
        { "blue",    "darkBlue"  },  //75..99
        /* gray is darker than darkGray for some reason (at least on OSX);
           default green is too dark compared to blue, yellow, orange,
           and red so is changed here to green.lighter(150) */
        { "#00c000", "gray"      },  //50..74  /* "green"=="#008000" */
        { "yellow",  "darkGray"  },  //25..49
        { "orange",  "lightGray" },  //10..24
        { "red",     "white"     },  // 0..9
    };

    /*
     * tty and curses use inverse video characters in the left portion
     * of the name+rank string to reflect hero's health.  We draw a
     * separate line above the name+rank field instead.  The left side
     * of the line indicates current health.  The right side is only
     * shown when injured and indicates missing amount of maximum health.
     */
    if (iflags.wc2_hitpointbar) {
        int colorindx, w,
            ihp = Upolyd ? u.mh : u.uhp,
            ihpmax = Upolyd ? u.mhmax : u.uhpmax;
        ihp = std::max(std::min(ihp, ihpmax), 0);
        int pct = 100 * ihp / ihpmax,
            lox = hline1.x(),
            hix = lox + hline1.width() - 1;
        QRect geoH = hpbar_health.geometry(),
              geoI = hpbar_injury.geometry();
        QString styleH, styleI;

        if (ihp < ihpmax) {
            // health is less than full;
            // use red for extreme low health even if the percentage is
            // above the usual threshold (which will happen when maximum
            // health is very low); do a similar threshold override for
            // orange even though it can be distracting for low level hero
            colorindx = (pct < 10 || ihp < 5) ? 5       // red    | white
                        : (pct < 25 || ihp < 10 ) ? 4   // orange | lightGray
                          : (pct < 50) ? 3              // yellow | darkGray*
                            : (pct < 75) ? 2            // green  | gray*
                              : 1;                      // blue   | darkBlue

            int pxl_health = (hix - lox + 1) * ihp / ihpmax;
            geoH.setRight(std::min(lox + pxl_health - 1, hix));
            hpbar_health.setGeometry(geoH);
            w = geoH.right() - geoH.left() + 1; // might yield 0 (ie, if dead)
            styleH = QString::asprintf(styleformat, barcolors[colorindx][0],
                                       w, w);
            hpbar_health.setStyleSheet(styleH);
            // when healing, having the old injury-side shown while the new
            // health-side expands pushes the injury farther right and it's
            // momentarily visible there before it gets recalculated+redrawn
            hpbar_injury.hide(); // will re-show below
            hpbar_health.show(); // don't need to hide() if/when width is 0

            int oldleft = geoI.left();
            geoI.setLeft(geoH.right() + 1);
            geoI.setRight(hix);
            hpbar_injury.setGeometry(geoI);
            w = geoI.right() - geoI.left() + 1;
            styleI = QString::asprintf(styleformat, barcolors[colorindx][1],
                                       w, w);
            hpbar_injury.setStyleSheet(styleI);
            if (geoI.left() != oldleft)
                hpbar_injury.move(geoI.left(), geoI.top());
            hpbar_injury.show();

            alreadyfullhp = false;
        } else if (!alreadyfullhp) { // skip if unchanged
            // health is full
            colorindx = 0; // black | (not used)

            hpbar_injury.hide();
            geoI.setLeft(hix); // hix + 1
            hpbar_injury.setGeometry(geoI);

            geoH.setRight(hix);
            hpbar_health.setGeometry(geoH);
            w = geoH.right() - geoH.left() + 1;
            styleH = QString::asprintf(styleformat, barcolors[colorindx][0],
                                       w, w);
            hpbar_health.setStyleSheet(styleH);
            hpbar_health.show();

            alreadyfullhp = true;
        }
    } else {
        // hitpoint bar is disabled
        hpbar_health.hide();
        hpbar_injury.hide();
        alreadyfullhp = false;
    }
}

RESTORE_WARNING_FORMAT_NONLITERAL

/*
 * Update the displayed status.  The current code in botl.c updates
 * two lines of information.  Both lines are always updated one after
 * the other.  So only do our update when we update the second line.
 *
 * Information on the first line:
 *    name, Str/Dex/&c characteristics, alignment, score
 *
 * Information on the second line:
 *    dlvl, gold, hp, power, ac, {level & exp or HD **}
 *    status (hunger, encumbrance, sick, stun, conf, halu, blind), time
 *
 * [**] HD is shown instead of level and exp when hero is polymorphed.
 */
void NetHackQtStatusWindow::updateStats()
{
    if (!parentWidget()) return;
    if (cursy != 0) return; /* do a complete update when line 0 is done */

    QString buf;

    if (first_set) {
        // set toggle-detection flags for optional fields
        was_polyd = Upolyd ? true : false;
        had_exp = ::flags.showexp ? true : false;
        // not conditionalized upon '#ifdef SCORE_ON_BOTL' here
        had_score = ::flags.showscore ? true : false; // false when disabled
        score.setLabel(""); // init if enabled, one-time set if disabled
    }
    // display hitpoint bar if it is active; it isn't subject to field
    // highlighting so we don't track whether it has just been toggled On|Off
    HitpointBar();

    int st = ACURR(A_STR);
    if (st > STR18(100)) {
        buf = QString::asprintf("Str:%d", st - 100);        // 19..25
    } else if (st == STR18(100)) {
        buf = QString::asprintf("Str:18/**");               // 18/100
    } else if (st > 18) {
        buf = QString::asprintf("Str:18/%02d", st - 18);    // 18/01..18/99
    } else {
        buf = QString::asprintf("Str:%d", st);              //  3..18
    }
    str.setLabel(buf, NetHackQtLabelledIcon::NoNum, (long) st);
    dex.setLabel("Dex:", (long) ACURR(A_DEX));
    con.setLabel("Con:", (long) ACURR(A_CON));
    intel.setLabel("Int:", (long) ACURR(A_INT));
    wis.setLabel("Wis:", (long) ACURR(A_WIS));
    cha.setLabel("Cha:", (long) ACURR(A_CHA));

    boolean spreadout = (::iflags.wc2_statuslines != 2);
    int k = 0; // number of conditions shown

    long qt_uhs = 0L;
    const char *hung = hu_stat[u.uhs];
    QString qhung = QString(hung).trimmed();
    if (hung[0]==' ') {
        if (!hunger.isHidden()) {
            hunger.setLabel("", NetHackQtLabelledIcon::NoNum, qt_uhs);
            hunger.hide();
        }
    } else {
        // satiated is worse (due to risk of death from overeating)
        // than not-hungry and we'll treat it as also worse than hungry,
        // but better than weak or fainting; the u.uhs enum values
        // order them differently so we jump through a hoop
        switch (u.uhs) {
        case NOT_HUNGRY: qt_uhs = 0L; break;
        case HUNGRY:     qt_uhs = 1L; break;
        case SATIATED:   qt_uhs = 2L; break;
        case WEAK:       qt_uhs = 3L; break;
        case FAINTING:   qt_uhs = 4L; break;
        default:         qt_uhs = 5L; break; // fainted, starved
        }
        hunger.setIcon(u.uhs ? p_hungry : p_satiated, qhung.toLower());
        hunger.setLabel(qhung, NetHackQtLabelledIcon::NoNum, qt_uhs);
        hunger.ForceResize();
	++k, hunger.show();
    }
    long encindx = (long) near_capacity();
    const char *enc = enc_stat[encindx];
    if (enc[0]==' ' || !enc[0]) {
        if (!encumber.isHidden()) {
            encumber.setLabel("", NetHackQtLabelledIcon::NoNum, encindx);
            encumber.hide();
        }
    } else {
        encumber.setIcon(p_encumber[encindx - 1], QString(enc).toLower());
        encumber.setLabel(enc, NetHackQtLabelledIcon::NoNum, encindx);
        encumber.ForceResize();
	++k, encumber.show();
    }

    if (Stoned) ++k, stoned.show(); else stoned.hide();
    if (Slimed) ++k, slimed.show(); else slimed.hide();
    if (Strangled) ++k, strngld.show(); else strngld.hide();
    if (Sick) {
        /* FoodPois or TermIll or both */
	if (u.usick_type & SICK_VOMITABLE) { /* food poisoning */
	    ++k, sick_fp.show();
	} else {
	    sick_fp.hide();
	}
	if (u.usick_type & SICK_NONVOMITABLE) { /* terminally ill */
	    ++k, sick_il.show();
	} else {
	    sick_il.hide();
	}
    } else {
	sick_fp.hide();
	sick_il.hide();
    }
    if (Stunned) ++k, stunned.show(); else stunned.hide();
    if (Confusion) ++k, confused.show(); else confused.hide();
    if (Hallucination) ++k, hallu.show(); else hallu.hide();
    if (Blind) ++k, blind.show(); else blind.hide();
    if (Deaf) ++k, deaf.show(); else deaf.hide();

    // flying is blocked when levitating, so Lev and Fly are mutually exclusive
    if (Levitation) ++k, lev.show(); else lev.hide();
    if (Flying) ++k, fly.show(); else fly.hide();
    if (u.usteed) ++k, ride.show(); else ride.hide();

    if (Upolyd) {
	buf = nh_capitalize_words(pmname(&mons[u.umonnum],
                                  ::flags.female ? FEMALE : MALE));
    } else {
	buf = rank_of(u.ulevel, g.pl_character[0], ::flags.female);
    }
    QString buf2;
    char buf3[BUFSZ];
    buf2 = QString::asprintf("%s the %s", upstart(strcpy(buf3, g.plname)),
                 buf.toLatin1().constData());
    name.setLabel(buf2, NetHackQtLabelledIcon::NoNum, u.ulevel);

    if (!describe_level(buf3)) {
	Sprintf(buf3, "%s, level %d",
                g.dungeons[u.uz.dnum].dname, ::depth(&u.uz));
    }
    dlevel.setLabel(buf3);

    int poly_toggled = !was_polyd ^ !Upolyd;
    int exp_toggled = !had_exp ^ !::flags.showexp;
    if (poly_toggled)
        // for this update, changed values aren't better|worse, just different
        hp.setCompareMode(NeitherIsBetter);
    if (poly_toggled || exp_toggled)
        level.setCompareMode(NeitherIsBetter);
    if (Upolyd) {
        // You're a monster!
        buf = QString::asprintf("/%d", u.mhmax);
        hp.setLabel("HP:", std::max((long) u.mh, 0L), buf);
        level.setLabel("HD:", (long) mons[u.umonnum].mlevel); // hit dice
        // Exp points are not shown when HD is displayed instead of Xp level
    } else {
        // You're normal.
        buf = QString::asprintf("/%d", u.uhpmax);
        hp.setLabel("HP:", std::max((long) u.uhp, 0L), buf);
        // if Exp points are to be displayed, append them to Xp level;
        // up/down highlighting becomes tricky--don't try very hard;
        // depending upon font size and status layout, "Level:NN/nnnnnnnn"
        // might be too wide to fit
        static const char *const lvllbl[3] = { "Level:", "Lvl:", "L:" };
        QFontMetrics fm(level.label->font());
        for (int i = ::flags.showexp ? 0 : 3; i < 4; ++i) {
            // passes 0,1,2 are with Exp, 3 is without Exp and always fits
            if (i < 3) {
                buf = QString::asprintf("%s%ld/%ld", lvllbl[i],
                                        (long) u.ulevel, u.uexp);
            } else {
                buf = QString::asprintf("%s%ld", lvllbl[i - 3],
                                        (long) u.ulevel);
            }
            // +2: allow a couple of pixels at either end to be clipped off
            if (fm.size(0, buf).width() <= (2 + level.label->width() + 2))
                break;
        }
        level.setLabel(buf, NetHackQtLabelledIcon::NoNum,
                       // if we intended to show Exp but must settle
                       // for Xp due to width, we still want to use
                       // Exp for setLabel()'s Up|Down highlighting
                       ::flags.showexp ? u.uexp : (long) u.ulevel);
    }
    if (poly_toggled)
        // for next update, changed values will be better|worse as usual
        hp.setCompareMode(BiggerIsBetter);
    if (poly_toggled || exp_toggled)
        level.setCompareMode(BiggerIsBetter);
    was_polyd = Upolyd ? true : false;
    had_exp = (::flags.showexp && !was_polyd) ? true : false;

    buf = QString::asprintf("/%d", u.uenmax);
    power.setLabel("Pow:", (long) u.uen, buf);
    ac.setLabel("AC:", (long) u.uac);
    // gold prefix used to be "Au:", tty uses "$:"; never too wide to fit;
    // practical limit due to carrying capacity limit is less than 300K
    long goldamt = money_cnt(g.invent);
    goldamt = std::max(goldamt, 0L); // sanity; core's botl() does likewise
    goldamt = std::min(goldamt, 99999999L); // ditto
    gold.setLabel("Gold:", goldamt);

    const char *text;
    QString qtext;
    QPixmap *pxmp;
    if (u.ualign.type == A_LAWFUL) {
        pxmp = &p_lawful;
        text = "Lawful";
    } else if (u.ualign.type == A_NEUTRAL) {
        pxmp = &p_neutral;
        text = "Neutral";
    } else {
        pxmp = &p_chaotic;
        // Unaligned should never happen
        text = (u.ualign.type == A_CHAOTIC) ? "Chaotic"
               : (u.ualign.type == A_NONE) ? "unaligned"
                 : "other?";
    }
    qtext = QString::asprintf("%sly aligned", text);
    align.setIcon(*pxmp, qtext.toLower());
    align.setLabel(QString(text));
    // without this, the ankh pixmap shifts from centered to left
    // justified relative to the label text for some unknown reason...
    align.ForceResize();
    if (spreadout)
        ++k; // when not condensed, Alignment is shown on the Conditions row

    if (!k) {
        blank2.show(); // for vertical spacing: force the row to be non-empty
    } else
        blank2.hide();

    // Time isn't highlighted (due to constantly changing) so we don't keep
    // track of whether it has just been toggled On or Off
    if (::flags.time) {
        // hypothetically Time could grow to enough digits to have trouble
        // fitting, but it's not worth worrying about
        time.setLabel("Time:", (long) g.moves);
    } else {
        time.setLabel("");
    }
#ifdef SCORE_ON_BOTL
    int score_toggled = !had_score ^ !::flags.showscore;
    if (::flags.showscore) {
        if (score_toggled) // toggled On
            score.setCompareMode(NeitherIsBetter);
        long pts = botl_score();
        if (spreadout) {
            // plenty of room; Time and Score both have the width of 3 fields
            score.setLabel("Score:", pts);
        } else {
            // depending upon font size and status layout, "Score:nnnnnnnn"
            // might be too wide to fit (simpler version of Level:NN/nnnnnnnn)
            static const char *const scrlbl[3] = { "Score:", "Scr:", "S:" };
            QFontMetrics fm(score.label->font());
            for (int i = 0; i < 3; ++i) {
                buf = QString::asprintf("%s%ld", scrlbl[i], pts);
                // +2: allow couple of pixels at either end to be clipped off
                if (fm.size(0, buf).width() <= (2 + score.width() + 2))
                    break;
            }
            score.setLabel(buf, NetHackQtLabelledIcon::NoNum, pts);
            // with Xp/Exp, we fallback to Xp if the shortest label prefix
            // is still too long; here we just show a clipped value and
            // let user either live with it or turn 'showscore' off (or
            // set statuslines:3 to take advantage of the extra room that
            // the spread out status layout provides)
        }
    } else {
        if (score_toggled) { // toggled Off; if already Off, no need to set ""
            score.setCompareMode(NoCompare);
            score.setLabel(""); // blank when not active
        }
    }
    if (score_toggled)
        score.setCompareMode(BiggerIsBetter);
    had_score = ::flags.showscore ? true : false;
#endif /* SCORE_ON_BOTL */

    if (first_set) {
	first_set=false;

        /*
         * Default compareMode is BiggerIsBetter:  an increased value
         * is an improvement.
         */
	name.highlightWhenChanging();
        dlevel.highlightWhenChanging();
            dlevel.setCompareMode(NeitherIsBetter);

	str.highlightWhenChanging();
	dex.highlightWhenChanging();
	con.highlightWhenChanging();
	intel.highlightWhenChanging();
	wis.highlightWhenChanging();
	cha.highlightWhenChanging();

	hp.highlightWhenChanging();
	power.highlightWhenChanging();
        ac.highlightWhenChanging();
            ac.setCompareMode(SmallerIsBetter);
	level.highlightWhenChanging();
	gold.highlightWhenChanging();

        // don't highlight 'time' because it changes almost continuously
        // [if we did highlight it, we wouldn't show increase as 'Better']
        //time.highlightWhenChanging(); time.setCompareMode(NeitherIsBetter);
	score.highlightWhenChanging();

        align.highlightWhenChanging();
            align.setCompareMode(NeitherIsBetter);

	hunger.highlightWhenChanging();
            hunger.setCompareMode(SmallerIsBetter);
	encumber.highlightWhenChanging();
            encumber.setCompareMode(SmallerIsBetter);
	stoned.highlightWhenChanging();
	slimed.highlightWhenChanging();
	strngld.highlightWhenChanging();
	sick_fp.highlightWhenChanging();
	sick_il.highlightWhenChanging();
	stunned.highlightWhenChanging();
	confused.highlightWhenChanging();
	hallu.highlightWhenChanging();
	blind.highlightWhenChanging();
	deaf.highlightWhenChanging();
        // the default behavior is to highlight a newly shown condition
        // as "worse" but that isn't appropriate for 'other' conds;
        // NetHackQtLabelledIcon::show() uses NeitherIsBetter to handle it
	lev.highlightWhenChanging();
            lev.setCompareMode(NeitherIsBetter);
	fly.highlightWhenChanging();
            fly.setCompareMode(NeitherIsBetter);
	ride.highlightWhenChanging();
            ride.setCompareMode(NeitherIsBetter);
    }
}

/*
 * Turn off hilighted status values after a certain amount of turns.
 */
void NetHackQtStatusWindow::checkTurnEvents()
{
}

// clicking on status window runs #attributes (^X)
void NetHackQtStatusWindow::mousePressEvent(QMouseEvent *event UNUSED)
{
    QWidget *main = NetHackQtBind::mainWidget();
    (static_cast <NetHackQtMainWindow *> (main))->FuncAsCommand(doattributes);
}

} // namespace nethack_qt_
