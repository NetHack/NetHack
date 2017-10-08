// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt4stat.cpp -- bindings between the Qt 4 interface and the main code

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
#include "qt4stat.h"
#include "qt4stat.moc"
#include "qt4set.h"
#include "qt4str.h"
#include "qt_xpms.h"

extern const char *enc_stat[]; /* from botl.c */
extern const char *hu_stat[]; /* from eat.c */

namespace nethack_qt4 {

NetHackQtStatusWindow::NetHackQtStatusWindow() :
    // Notes:
    //  Alignment needs -2 init value, because -1 is an alignment.
    //  Armor Class is an schar, so 256 is out of range.
    //  Blank value is 0 and should never change.
    name(this,"(name)"),
    dlevel(this,"(dlevel)"),
    str(this,"STR"),
    dex(this,"DEX"),
    con(this,"CON"),
    intel(this,"INT"),
    wis(this,"WIS"),
    cha(this,"CHA"),
    gold(this,"Gold"),
    hp(this,"Hit Points"),
    power(this,"Power"),
    ac(this,"Armour Class"),
    level(this,"Level"),
    exp(this,"Experience"),
    align(this,"Alignment"),
    time(this,"Time"),
    score(this,"Score"),
    hunger(this,""),
    confused(this,"Confused"),
    sick_fp(this,"Sick"),
    sick_il(this,"Ill"),
    blind(this,""),
    stunned(this,"Stunned"),
    hallu(this,"Hallu"),
    encumber(this,""),
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

    p_confused = QPixmap(confused_xpm);
    p_sick_fp = QPixmap(sick_fp_xpm);
    p_sick_il = QPixmap(sick_il_xpm);
    p_blind = QPixmap(blind_xpm);
    p_stunned = QPixmap(stunned_xpm);
    p_hallu = QPixmap(hallu_xpm);

    p_encumber[0] = QPixmap(slt_enc_xpm);
    p_encumber[1] = QPixmap(mod_enc_xpm);
    p_encumber[2] = QPixmap(hvy_enc_xpm);
    p_encumber[3] = QPixmap(ext_enc_xpm);
    p_encumber[4] = QPixmap(ovr_enc_xpm);

    str.setIcon(p_str);
    dex.setIcon(p_dex);
    con.setIcon(p_con);
    intel.setIcon(p_int);
    wis.setIcon(p_wis);
    cha.setIcon(p_cha);

    align.setIcon(p_neutral);
    hunger.setIcon(p_hungry);

    confused.setIcon(p_confused);
    sick_fp.setIcon(p_sick_fp);
    sick_il.setIcon(p_sick_il);
    blind.setIcon(p_blind);
    stunned.setIcon(p_stunned);
    hallu.setIcon(p_hallu);

    encumber.setIcon(p_encumber[0]);

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
	statbox->addWidget(&confused);
	statbox->addWidget(&sick_fp);
	statbox->addWidget(&sick_il);
	statbox->addWidget(&blind);
	statbox->addWidget(&stunned);
	statbox->addWidget(&hallu);
	statbox->addWidget(&encumber);
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
    exp.setFont(normal);
    align.setFont(normal);
    time.setFont(normal);
    score.setFont(normal);
    hunger.setFont(normal);
    confused.setFont(normal);
    sick_fp.setFont(normal);
    sick_il.setFont(normal);
    blind.setFont(normal);
    stunned.setFont(normal);
    hallu.setFont(normal);
    encumber.setFont(normal);

    updateStats();
}

QWidget* NetHackQtStatusWindow::Widget() { return this; }

void NetHackQtStatusWindow::Clear()
{
}
void NetHackQtStatusWindow::Display(bool block)
{
}
void NetHackQtStatusWindow::CursorTo(int,int y)
{
    cursy=y;
}
void NetHackQtStatusWindow::PutStr(int attr, const QString& text)
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
    exp.setGeometry(x,y,iw,lh); x+=iw;
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
    confused.setGeometry(x,y,iw,lh); x+=iw;
    sick_fp.setGeometry(x,y,iw,lh); x+=iw;
    sick_il.setGeometry(x,y,iw,lh); x+=iw;
    blind.setGeometry(x,y,iw,lh); x+=iw;
    stunned.setGeometry(x,y,iw,lh); x+=iw;
    hallu.setGeometry(x,y,iw,lh); x+=iw;
    encumber.setGeometry(x,y,iw,lh); x+=iw;
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
    exp.dissipateHighlight();
    align.dissipateHighlight();

    time.dissipateHighlight();
    score.dissipateHighlight();

    hunger.dissipateHighlight();
    confused.dissipateHighlight();
    sick_fp.dissipateHighlight();
    sick_il.dissipateHighlight();
    blind.dissipateHighlight();
    stunned.dissipateHighlight();
    hallu.dissipateHighlight();
    encumber.dissipateHighlight();
}

/*
 * Update the displayed status.  The current code in botl.c updates
 * two lines of information.  Both lines are always updated one after
 * the other.  So only do our update when we update the second line.
 *
 * Information on the first line:
 *    name, attributes, alignment, score
 *
 * Information on the second line:
 *    dlvl, gold, hp, power, ac, {level & exp or HD **}
 *    status (hunger, conf, halu, stun, sick, blind), time, encumbrance
 *
 * [**] HD is shown instead of level and exp if mtimedone is non-zero.
 */
void NetHackQtStatusWindow::updateStats()
{
    if (!parentWidget()) return;

    QString buf;
    const char *text;

    if (cursy != 0) return;    /* do a complete update when line 0 is done */

    if (ACURR(A_STR) > 118) {
	buf.sprintf("STR:%d",ACURR(A_STR)-100);
    } else if (ACURR(A_STR)==118) {
	buf.sprintf("STR:18/**");
    } else if(ACURR(A_STR) > 18) {
	buf.sprintf("STR:18/%02d",ACURR(A_STR)-18);
    } else {
	buf.sprintf("STR:%d",ACURR(A_STR));
    }
    str.setLabel(buf,NetHackQtLabelledIcon::NoNum,ACURR(A_STR));

    dex.setLabel("DEX:",(long)ACURR(A_DEX));
    con.setLabel("CON:",(long)ACURR(A_CON));
    intel.setLabel("INT:",(long)ACURR(A_INT));
    wis.setLabel("WIS:",(long)ACURR(A_WIS));
    cha.setLabel("CHA:",(long)ACURR(A_CHA));
    const char* hung=hu_stat[u.uhs];
    if (hung[0]==' ') {
	hunger.hide();
    } else {
	hunger.setIcon(u.uhs ? p_hungry : p_satiated);
	hunger.setLabel(hung);
	hunger.show();
    }
    if (Confusion) confused.show(); else confused.hide();
    if (Sick) {
	if (u.usick_type & SICK_VOMITABLE) {
	    sick_fp.show();
	} else {
	    sick_fp.hide();
	}
	if (u.usick_type & SICK_NONVOMITABLE) {
	    sick_il.show();
	} else {
	    sick_il.hide();
	}
    } else {
	sick_fp.hide();
	sick_il.hide();
    }
    if (Blind) {
	blind.setLabel("Blind");
	blind.show();
    } else {
	blind.hide();
    }
    if (Stunned) stunned.show(); else stunned.hide();
    if (Hallucination) hallu.show(); else hallu.hide();
    const char* enc=enc_stat[near_capacity()];
    if (enc[0]==' ' || !enc[0]) {
	encumber.hide();
    } else {
	encumber.setIcon(p_encumber[near_capacity()-1]);
	encumber.setLabel(enc);
	encumber.show();
    }
    if (u.mtimedone) {
	buf = nh_capitalize_words(mons[u.umonnum].mname);
    } else {
	buf = rank_of(u.ulevel, pl_character[0], ::flags.female);
    }
    QString buf2;
    buf2.sprintf("%s the %s", plname, buf.toLatin1().constData());
    name.setLabel(buf2, NetHackQtLabelledIcon::NoNum, u.ulevel);

    char buf3[BUFSZ];
    if (describe_level(buf3)) {
	dlevel.setLabel(buf3,true);
    } else {
	buf.sprintf("%s, level ", dungeons[u.uz.dnum].dname);
	dlevel.setLabel(buf,(long)::depth(&u.uz));
    }

    gold.setLabel("Au:", money_cnt(invent));

    if (u.mtimedone) {
	// You're a monster!

	buf.sprintf("/%d", u.mhmax);
	hp.setLabel("HP:", u.mh  > 0 ? u.mh  : 0, buf);
	level.setLabel("HD:",(long)mons[u.umonnum].mlevel);
    } else {
	// You're normal.

	buf.sprintf("/%d", u.uhpmax);
	hp.setLabel("HP:", u.uhp > 0 ? u.uhp : 0, buf);
	level.setLabel("Level:",(long)u.ulevel);
    }
    buf.sprintf("/%d", u.uenmax);
    power.setLabel("Pow:", u.uen, buf);
    ac.setLabel("AC:",(long)u.uac);
#ifdef EXP_ON_BOTL
    if (::flags.showexp) {
	exp.setLabel("Exp:",(long)u.uexp);
    } else
#endif
    {
	exp.setLabel("");
    }
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

    if (::flags.time) time.setLabel("Time:",(long)moves);
    else time.setLabel("");
#ifdef SCORE_ON_BOTL
    if (::flags.showscore) {
	score.setLabel("Score:",(long)botl_score());
    } else
#endif
    {
	score.setLabel("");
    }

    if (first_set)
    {
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
	exp.highlightWhenChanging();
	align.highlightWhenChanging();

	//time.highlightWhenChanging();
	score.highlightWhenChanging();

	hunger.highlightWhenChanging();
	confused.highlightWhenChanging();
	sick_fp.highlightWhenChanging();
	sick_il.highlightWhenChanging();
	blind.highlightWhenChanging();
	stunned.highlightWhenChanging();
	hallu.highlightWhenChanging();
	encumber.highlightWhenChanging();
    }
}

/*
 * Turn off hilighted status values after a certain amount of turns.
 */
void NetHackQtStatusWindow::checkTurnEvents()
{
}

} // namespace nethack_qt4
