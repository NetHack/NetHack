// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_stat.cpp -- bindings between the Qt 4 interface and the main code

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

namespace nethack_qt_ {

NetHackQtStatusWindow::NetHackQtStatusWindow() :
    // Notes:
    //  Alignment needs -2 init value, because -1 is an alignment.
    //  Armor Class is an schar, so 256 is out of range.
    //  Blank value is 0 and should never change.
    name(this,"(name)"),
    dlevel(this,"(dlevel)"),
    str(this, "Str"),
    dex(this, "Dex"),
    con(this, "Con"),
    intel(this, "Int"),
    wis(this, "Wis"),
    cha(this, "Cha"),
    gold(this,"Gold"),
    hp(this,"Hit Points"),
    power(this,"Power"),
    ac(this,"Armour Class"),
    level(this,"Level"),
    exp(this, "_"), // exp displayed as Xp/Exp but exp widget used for padding
    align(this,"Alignment"),
    time(this,"Time"),
    score(this,"Score"),
    hunger(this,""),
    encumber(this,""),
    stoned(this,"Stone"),
    slimed(this,"Slime"),
    strngld(this,"Strngl"),
    sick_fp(this,"FoodPois"),
    sick_il(this,"TermIll"),
    stunned(this,"Stun"),
    confused(this,"Conf"),
    hallu(this,"Hallu"),
    blind(this,""),
    deaf(this,"Deaf"),
    lev(this,"Lev"),
    fly(this,"Fly"),
    ride(this,"Ride"),
    hline1(this),
    hline2(this),
    hline3(this),
    first_set(true)
{
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

    str.setIcon(p_str);
    dex.setIcon(p_dex);
    con.setIcon(p_con);
    intel.setIcon(p_int);
    wis.setIcon(p_wis);
    cha.setIcon(p_cha);

    align.setIcon(p_neutral);
    hunger.setIcon(p_hungry);
    encumber.setIcon(p_encumber[0]);

    stoned.setIcon(p_stoned);
    slimed.setIcon(p_slimed);
    strngld.setIcon(p_strngld);
    sick_fp.setIcon(p_sick_fp);
    sick_il.setIcon(p_sick_il);
    stunned.setIcon(p_stunned);
    confused.setIcon(p_confused);
    hallu.setIcon(p_hallu);
    blind.setIcon(p_blind);
    deaf.setIcon(p_deaf);
    lev.setIcon(p_lev);
    fly.setIcon(p_fly);
    ride.setIcon(p_ride);

    hline1.setFrameStyle(QFrame::HLine|QFrame::Sunken);
    hline2.setFrameStyle(QFrame::HLine|QFrame::Sunken);
    hline3.setFrameStyle(QFrame::HLine|QFrame::Sunken);
    hline1.setLineWidth(1);
    hline2.setLineWidth(1);
    hline3.setLineWidth(1);

#if 1 //RLC
    name.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    dlevel.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    QVBoxLayout *vbox = new QVBoxLayout();
    vbox->setSpacing(0);
    vbox->addWidget(&name);
    vbox->addWidget(&dlevel);
    vbox->addWidget(&hline1);
    QHBoxLayout *atr1box = new QHBoxLayout();
	atr1box->addWidget(&str);
	atr1box->addWidget(&dex);
	atr1box->addWidget(&con);
	atr1box->addWidget(&intel);
	atr1box->addWidget(&wis);
	atr1box->addWidget(&cha);
    vbox->addLayout(atr1box);
    vbox->addWidget(&hline2);
    QHBoxLayout *atr2box = new QHBoxLayout();
	atr2box->addWidget(&gold);
	atr2box->addWidget(&hp);
	atr2box->addWidget(&power);
	atr2box->addWidget(&ac);
	atr2box->addWidget(&level);
	atr2box->addWidget(&exp);
    vbox->addLayout(atr2box);
    vbox->addWidget(&hline3);
    QHBoxLayout *timebox = new QHBoxLayout();
	timebox->addWidget(&time);
	timebox->addWidget(&score);
    vbox->addLayout(timebox);
    QHBoxLayout *statbox = new QHBoxLayout();
	statbox->addWidget(&align);
	statbox->addWidget(&hunger);
	statbox->addWidget(&encumber);
	statbox->addWidget(&stoned);
	statbox->addWidget(&slimed);
	statbox->addWidget(&strngld);
	statbox->addWidget(&sick_fp);
	statbox->addWidget(&sick_il);
	statbox->addWidget(&stunned);
	statbox->addWidget(&confused);
	statbox->addWidget(&hallu);
	statbox->addWidget(&blind);
	statbox->addWidget(&deaf);
	statbox->addWidget(&lev);
	statbox->addWidget(&fly);
	statbox->addWidget(&ride);
    statbox->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    vbox->addLayout(statbox);
    setLayout(vbox);
#endif

    connect(qt_settings,SIGNAL(fontChanged()),this,SLOT(doUpdate()));
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
    gold.setFont(normal);
    hp.setFont(normal);
    power.setFont(normal);
    ac.setFont(normal);
    level.setFont(normal);
    //exp.setFont(normal);
    align.setFont(normal);
    time.setFont(normal);
    score.setFont(normal);
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
 * kinds of funny values being displayed.
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
    //exp.dissipateHighlight();
    align.dissipateHighlight();

    time.dissipateHighlight();
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

    QString buf;
    const char *text;

    if (cursy != 0) return;    /* do a complete update when line 0 is done */

    int st = ACURR(A_STR);
    if (st > STR18(100)) {
        buf.sprintf("Str:%d", st - 100);        // 19..25
    } else if (st == STR18(100)) {
        buf.sprintf("Str:18/**");               // 18/100
    } else if (st > 18) {
        buf.sprintf("Str:18/%02d", st - 18);    // 18/01..18/99
    } else {
        buf.sprintf("Str:%d", st);              //  3..18
    }
    str.setLabel(buf, NetHackQtLabelledIcon::NoNum, (long) st);
    dex.setLabel("Dex:", (long) ACURR(A_DEX));
    con.setLabel("Con:", (long) ACURR(A_CON));
    intel.setLabel("Int:", (long) ACURR(A_INT));
    wis.setLabel("Wis:", (long) ACURR(A_WIS));
    cha.setLabel("Cha:", (long) ACURR(A_CHA));

    const char* hung=hu_stat[u.uhs];
    if (hung[0]==' ') {
	hunger.hide();
    } else {
	hunger.setIcon(u.uhs ? p_hungry : p_satiated);
	hunger.setLabel(hung);
	hunger.show();
    }
    const char *enc = enc_stat[near_capacity()];
    if (enc[0]==' ' || !enc[0]) {
	encumber.hide();
    } else {
	encumber.setIcon(p_encumber[near_capacity() - 1]);
	encumber.setLabel(enc);
	encumber.show();
    }
    if (Stoned) stoned.show(); else stoned.hide();
    if (Slimed) slimed.show(); else slimed.hide();
    if (Strangled) strngld.show(); else strngld.hide();
    if (Sick) {
        /* FoodPois or TermIll or both */
	if (u.usick_type & SICK_VOMITABLE) { /* food poisoning */
	    sick_fp.show();
	} else {
	    sick_fp.hide();
	}
	if (u.usick_type & SICK_NONVOMITABLE) { /* terminally ill */
	    sick_il.show();
	} else {
	    sick_il.hide();
	}
    } else {
	sick_fp.hide();
	sick_il.hide();
    }
    if (Stunned) stunned.show(); else stunned.hide();
    if (Confusion) confused.show(); else confused.hide();
    if (Hallucination) hallu.show(); else hallu.hide();
    if (Blind) {
	blind.setLabel("Blind");
	blind.show();
    } else {
	blind.hide();
    }
    if (Deaf) deaf.show(); else deaf.hide();
    // flying is blocked when levitating, so Lev and Fly are mutually exclusive
    if (Levitation) lev.show(); else lev.hide();
    if (Flying) fly.show(); else fly.hide();
    if (u.usteed) ride.show(); else ride.hide();

    if (Upolyd) {
	buf = nh_capitalize_words(mons[u.umonnum].mname);
    } else {
	buf = rank_of(u.ulevel, g.pl_character[0], ::flags.female);
    }
    QString buf2;
    buf2.sprintf("%s the %s", g.plname, buf.toLatin1().constData());
    name.setLabel(buf2, NetHackQtLabelledIcon::NoNum, u.ulevel);

    char buf3[BUFSZ];
    if (describe_level(buf3)) {
	dlevel.setLabel(buf3,true);
    } else {
	buf.sprintf("%s, level ", g.dungeons[u.uz.dnum].dname);
	dlevel.setLabel(buf,(long)::depth(&u.uz));
    }

    gold.setLabel("Au:", money_cnt(g.invent));

    if (Upolyd) {
        // You're a monster!
        buf.sprintf("/%d", u.mhmax);
        hp.setLabel("HP:", std::max((long) u.mh, 0L), buf);
        level.setLabel("HD:", (long) mons[u.umonnum].mlevel); // hit dice
        // Exp points are not shown when HD is displayed instead of Xp level
    } else {
        // You're normal.
        buf.sprintf("/%d", u.uhpmax);
        hp.setLabel("HP:", std::max((long) u.uhp, 0L), buf);
        // if Exp points are to be displayed, append them to Xp level;
        // up/down highlighting becomes tricky--don't try very hard
        if (::flags.showexp) {
            buf.sprintf("%ld/%ld", (long) u.ulevel, (long) u.uexp);
            level.setLabel("Level:" + buf,
                           NetHackQtLabelledIcon::NoNum, (long) u.uexp);
        } else {
            level.setLabel("Level:", (long) u.ulevel);
        }
    }
    buf.sprintf("/%d", u.uenmax);
    power.setLabel("Pow:", u.uen, buf);
    ac.setLabel("AC:",(long)u.uac);
    //if (::flags.showexp) {
    //    exp.setLabel("Exp:", (long) u.uexp);
    //} else {
        // 'exp' now only used to pad the line that Xp/Exp is displayed on
        exp.setLabel("");
    //}
    if (u.ualign.type==A_CHAOTIC) {
	align.setIcon(p_chaotic);
	text = "Chaotic";
    } else if (u.ualign.type==A_NEUTRAL) {
	align.setIcon(p_neutral);
	text = "Neutral";
    } else {
	align.setIcon(p_lawful);
	text = "Lawful";
    }
    align.setLabel(text);

    if (::flags.time)
        time.setLabel("Time:", (long) g.moves);
    else
        time.setLabel("");
#ifdef SCORE_ON_BOTL
    if (::flags.showscore) {
        score.setLabel("Score:", (long) botl_score());
    } else
#endif
    {
	score.setLabel("");
    }

    if (first_set) {
	first_set=false;

	name.highlightWhenChanging();
	dlevel.highlightWhenChanging();

	str.highlightWhenChanging();
	dex.highlightWhenChanging();
	con.highlightWhenChanging();
	intel.highlightWhenChanging();
	wis.highlightWhenChanging();
	cha.highlightWhenChanging();

	gold.highlightWhenChanging();
	hp.highlightWhenChanging();
	power.highlightWhenChanging();
	ac.highlightWhenChanging(); ac.lowIsGood();
	level.highlightWhenChanging();
        //exp.highlightWhenChanging(); -- 'exp' is just padding
	align.highlightWhenChanging();

        // don't highlight 'time' because it changes almost continuously
        //time.highlightWhenChanging();
	score.highlightWhenChanging();

	hunger.highlightWhenChanging();
	encumber.highlightWhenChanging();
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
	lev.highlightWhenChanging();
	fly.highlightWhenChanging();
	ride.highlightWhenChanging();
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
    // same code as NetHackQtInvUsageWindow::mousePressEvent except for func
    char cmdbuf[32];
    Strcpy(cmdbuf, "#");
    (void) cmdname_from_func(doattributes, &cmdbuf[1], FALSE);
    // queue up #attribues as if user had typed it; we don't execute
    // doattributes() directly because the program might not be ready
    // for a command right now
    QWidget *main = NetHackQtBind::mainWidget();
    (static_cast <NetHackQtMainWindow *> (main))->DollClickToKeys(cmdbuf);
}

} // namespace nethack_qt_
