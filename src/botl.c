/*	SCCS Id: @(#)botl.c	3.4	2003/11/22	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

extern const char *hu_stat[];	/* defined in eat.c */

const char * const enc_stat[] = {
	"",
	"Burdened",
	"Stressed",
	"Strained",
	"Overtaxed",
	"Overloaded"
};

#ifdef STATUS_VIA_WINDOWPORT
STATIC_OVL void NDECL(init_blstats);
#else
STATIC_DCL void NDECL(bot1);
STATIC_DCL void NDECL(bot2);
#endif

/* MAXCO must hold longest uncompressed status line, and must be larger
 * than COLNO
 *
 * longest practical second status line at the moment is
 *	Astral Plane $:12345 HP:700(700) Pw:111(111) AC:-127 Xp:30/123456789
 *	T:123456 Satiated Conf FoodPois Ill Blind Stun Hallu Overloaded
 * -- or somewhat over 130 characters
 */
#if COLNO <= 140
#define MAXCO 160
#else
#define MAXCO (COLNO+20)
#endif

STATIC_OVL NEARDATA int mrank_sz = 0; /* loaded by max_rank_sz (from u_init) */
STATIC_DCL const char *NDECL(rank);

/* convert experience level (1..30) to rank index (0..8) */
int
xlev_to_rank(xlev)
int xlev;
{
	return (xlev <= 2) ? 0 : (xlev <= 30) ? ((xlev + 2) / 4) : 8;
}

#if 0	/* not currently needed */
/* convert rank index (0..8) to experience level (1..30) */
int
rank_to_xlev(rank)
int rank;
{
	return (rank <= 0) ? 1 : (rank <= 8) ? ((rank * 4) - 2) : 30;
}
#endif

const char *
rank_of(lev, monnum, female)
	int lev;
	short monnum;
	boolean female;
{
	register struct Role *role;
	register int i;


	/* Find the role */
	for (role = (struct Role *) roles; role->name.m; role++)
	    if (monnum == role->malenum || monnum == role->femalenum)
	    	break;
	if (!role->name.m)
	    role = &urole;

	/* Find the rank */
	for (i = xlev_to_rank((int)lev); i >= 0; i--) {
	    if (female && role->rank[i].f) return (role->rank[i].f);
	    if (role->rank[i].m) return (role->rank[i].m);
	}

	/* Try the role name, instead */
	if (female && role->name.f) return (role->name.f);
	else if (role->name.m) return (role->name.m);
	return ("Player");
}


STATIC_OVL const char *
rank()
{
	return(rank_of(u.ulevel, Role_switch, flags.female));
}

int
title_to_mon(str, rank_indx, title_length)
const char *str;
int *rank_indx, *title_length;
{
	register int i, j;


	/* Loop through each of the roles */
	for (i = 0; roles[i].name.m; i++)
	    for (j = 0; j < 9; j++) {
	    	if (roles[i].rank[j].m && !strncmpi(str,
	    			roles[i].rank[j].m, strlen(roles[i].rank[j].m))) {
	    	    if (rank_indx) *rank_indx = j;
	    	    if (title_length) *title_length = strlen(roles[i].rank[j].m);
	    	    return roles[i].malenum;
	    	}
	    	if (roles[i].rank[j].f && !strncmpi(str,
	    			roles[i].rank[j].f, strlen(roles[i].rank[j].f))) {
	    	    if (rank_indx) *rank_indx = j;
	    	    if (title_length) *title_length = strlen(roles[i].rank[j].f);
	    	    return ((roles[i].femalenum != NON_PM) ?
	    	    		roles[i].femalenum : roles[i].malenum);
	    	}
	    }
	return NON_PM;
}

void
max_rank_sz()
{
	register int i, r, maxr = 0;
	for (i = 0; i < 9; i++) {
	    if (urole.rank[i].m && (r = strlen(urole.rank[i].m)) > maxr) maxr = r;
	    if (urole.rank[i].f && (r = strlen(urole.rank[i].f)) > maxr) maxr = r;
	}
	mrank_sz = maxr;
	return;
}

#ifdef SCORE_ON_BOTL
long
botl_score()
{
    int deepest = deepest_lev_reached(FALSE);
#ifndef GOLDOBJ
    long ugold = u.ugold + hidden_gold();

    if ((ugold -= u.ugold0) < 0L) ugold = 0L;
    return ugold + u.urexp + (long)(50 * (deepest - 1))
#else
    long umoney = money_cnt(invent) + hidden_gold();

    if ((umoney -= u.umoney0) < 0L) umoney = 0L;
    return umoney + u.urexp + (long)(50 * (deepest - 1))
#endif
			  + (long)(deepest > 30 ? 10000 :
				   deepest > 20 ? 1000*(deepest - 20) : 0);
}
#endif

#ifndef STATUS_VIA_WINDOWPORT
STATIC_OVL void
bot1()
{
	char newbot1[MAXCO];
	register char *nb;
	register int i,j;
	Strcpy(newbot1, plname);
	if('a' <= newbot1[0] && newbot1[0] <= 'z') newbot1[0] += 'A'-'a';
	newbot1[10] = 0;
	Sprintf(nb = eos(newbot1)," the ");

	if (Upolyd) {
		char mbot[BUFSZ];
		int k = 0;

		Strcpy(mbot, mons[u.umonnum].mname);
		while(mbot[k] != 0) {
		    if ((k == 0 || (k > 0 && mbot[k-1] == ' ')) &&
					'a' <= mbot[k] && mbot[k] <= 'z')
			mbot[k] += 'A' - 'a';
		    k++;
		}
		Sprintf(nb = eos(nb), mbot);
	} else
		Sprintf(nb = eos(nb), rank());

	Sprintf(nb = eos(nb),"  ");
	i = mrank_sz + 15;
	j = (nb + 2) - newbot1; /* aka strlen(newbot1) but less computation */
	if((i - j) > 0)
		Sprintf(nb = eos(nb),"%*s", i-j, " ");	/* pad with spaces */
	if (ACURR(A_STR) > 18) {
		if (ACURR(A_STR) > STR18(100))
		    Sprintf(nb = eos(nb),"St:%2d ",ACURR(A_STR)-100);
		else if (ACURR(A_STR) < STR18(100))
		    Sprintf(nb = eos(nb), "St:18/%02d ",ACURR(A_STR)-18);
		else
		    Sprintf(nb = eos(nb),"St:18/** ");
	} else
		Sprintf(nb = eos(nb), "St:%-1d ",ACURR(A_STR));
	Sprintf(nb = eos(nb),
		"Dx:%-1d Co:%-1d In:%-1d Wi:%-1d Ch:%-1d",
		ACURR(A_DEX), ACURR(A_CON), ACURR(A_INT), ACURR(A_WIS), ACURR(A_CHA));
	Sprintf(nb = eos(nb), (u.ualign.type == A_CHAOTIC) ? "  Chaotic" :
			(u.ualign.type == A_NEUTRAL) ? "  Neutral" : "  Lawful");
#ifdef SCORE_ON_BOTL
	if (flags.showscore)
	    Sprintf(nb = eos(nb), " S:%ld", botl_score());
#endif
	curs(WIN_STATUS, 1, 0);
	putstr(WIN_STATUS, 0, newbot1);
}
#endif

/* provide the name of the current level for display by various ports */
int
describe_level(buf)
char *buf;
{
	int ret = 1;

	/* TODO:	Add in dungeon name */
	if (Is_knox(&u.uz))
		Sprintf(buf, "%s ", dungeons[u.uz.dnum].dname);
	else if (In_quest(&u.uz))
		Sprintf(buf, "Home %d ", dunlev(&u.uz));
	else if (In_endgame(&u.uz))
		Sprintf(buf,
			Is_astralevel(&u.uz) ? "Astral Plane " : "End Game ");
	else {
		/* ports with more room may expand this one */
		Sprintf(buf, "Dlvl:%-2d ", depth(&u.uz));
		ret = 0;
	}
	return ret;
}

#ifndef STATUS_VIA_WINDOWPORT
STATIC_OVL void
bot2()
{
	char  newbot2[MAXCO];
	register char *nb;
	int hp, hpmax;
	int cap = near_capacity();

	hp = Upolyd ? u.mh : u.uhp;
	hpmax = Upolyd ? u.mhmax : u.uhpmax;

	if(hp < 0) hp = 0;
	(void) describe_level(newbot2);
	Sprintf(nb = eos(newbot2),
		"%c:%-2ld HP:%d(%d) Pw:%d(%d) AC:%-2d", oc_syms[COIN_CLASS],
#ifndef GOLDOBJ
		u.ugold,
#else
		money_cnt(invent),
#endif
		hp, hpmax, u.uen, u.uenmax, u.uac);

	if (Upolyd)
		Sprintf(nb = eos(nb), " HD:%d", mons[u.umonnum].mlevel);
#ifdef EXP_ON_BOTL
	else if(flags.showexp)
		Sprintf(nb = eos(nb), " Xp:%u/%-1ld", u.ulevel,u.uexp);
#endif
	else
		Sprintf(nb = eos(nb), " Exp:%u", u.ulevel);

	if(flags.time)
	    Sprintf(nb = eos(nb), " T:%ld", moves);
	if(strcmp(hu_stat[u.uhs], "        ")) {
		Sprintf(nb = eos(nb), " ");
		Strcat(newbot2, hu_stat[u.uhs]);
	}
	if(Confusion)	   Sprintf(nb = eos(nb), " Conf");
	if(Sick) {
		if (u.usick_type & SICK_VOMITABLE)
			   Sprintf(nb = eos(nb), " FoodPois");
		if (u.usick_type & SICK_NONVOMITABLE)
			   Sprintf(nb = eos(nb), " Ill");
	}
	if(Blind)	   Sprintf(nb = eos(nb), " Blind");
	if(Stunned)	   Sprintf(nb = eos(nb), " Stun");
	if(Hallucination)  Sprintf(nb = eos(nb), " Hallu");
	if(Slimed)         Sprintf(nb = eos(nb), " Slime");
	if(cap > UNENCUMBERED)
		Sprintf(nb = eos(nb), " %s", enc_stat[cap]);
	curs(WIN_STATUS, 1, 1);
	putstr(WIN_STATUS, 0, newbot2);
}

void
bot()
{
	bot1();
	bot2();
	context.botl = context.botlx = 0;
}

#else	/* STATUS_VIA_WINDOWPORT */

/* These are used within botl.c only */
#define P_MASK 1
#define P_STR  2
#define P_INT  3
#define P_LNG  4
#define P_UINT 5

struct istat_s {
	long time;
	unsigned ptype;
	anything ptr;
	char *val;
	int valwidth;
	int idxmax;
};

#define percentage(current, maximum)	((100 * current) / maximum)
#define percentagel(current, maximum)	((int)((100L * current) / maximum))

/* If entries are added to this, botl.h will require updating too */
struct istat_s blstats[2][MAXBLSTATS] = {
	{
	{ 0L, P_STR, (genericptr_t)0, (char *)0, 80, 0},	/*  0 BL_TITLE */
	{ 0L, P_INT, (genericptr_t)0, (char *)0, 6,  0},	/*  1 BL_STR */
	{ 0L, P_INT, (genericptr_t)0, (char *)0, 6,  0},	/*  2 BL_DX */
	{ 0L, P_INT, (genericptr_t)0, (char *)0, 6,  0},	/*  3 BL_CO */
	{ 0L, P_INT, (genericptr_t)0, (char *)0, 6,  0},	/*  4 BL_IN */
	{ 0L, P_INT, (genericptr_t)0, (char *)0, 6,  0},	/*  5 BL_WI */
	{ 0L, P_INT, (genericptr_t)0, (char *)0, 6,  0},	/*  6 BL_CH */
	{ 0L, P_STR, (genericptr_t)0, (char *)0, 8,  0},	/*  7 BL_ALIGN */
	{ 0L, P_LNG, (genericptr_t)0, (char *)0, 20, 0},	/*  8 BL_SCORE */
	{ 0L, P_LNG, (genericptr_t)0, (char *)0, 20, 0},	/*  9 BL_CAP */
	{ 0L, P_LNG, (genericptr_t)0, (char *)0, 8,  0},	/* 10 BL_GOLD */
	{ 0L, P_INT, (genericptr_t)0, (char *)0, 6, BL_ENEMAX}, /* 11 BL_ENE */
	{ 0L, P_INT, (genericptr_t)0, (char *)0, 6,  0},	/* 12 BL_ENEMAX */
	{ 0L, P_LNG, (genericptr_t)0, (char *)0, 6,  0},	/* 13 BL_XP */
	{ 0L, P_INT, (genericptr_t)0, (char *)0, 6,  0},	/* 14 BL_AC */
	{ 0L, P_INT, (genericptr_t)0, (char *)0, 6,  0},	/* 15 BL_HD */
	{ 0L, P_INT, (genericptr_t)0, (char *)0, 10, 0},	/* 16 BL_TIME */
	{ 0L, P_UINT,(genericptr_t)0, (char *)0, 20, 0},	/* 17 BL_HUNGER */
	{ 0L, P_INT, (genericptr_t)0, (char *)0, 10,BL_HPMAX},	/* 18 BL_HP */
	{ 0L, P_INT, (genericptr_t)0, (char *)0, 10, 0},	/* 19 BL_HPMAX */
	{ 0L, P_STR, (genericptr_t)0, (char *)0, 80, 0},	/* 20 BL_LEVELDESC */
	{ 0L, P_LNG, (genericptr_t)0, (char *)0, 20, 0},	/* 21 BL_EXP */
	{ 0L, P_MASK,(genericptr_t)0, (char *)0, 0, 0},	/* 22 BL_CONDITION */
	}
};

static boolean update_all = FALSE;

void
status_initialize(reassessment)
boolean reassessment;	/* TRUE = just reassess fields w/o other initialization*/
{
	int i;
	const char *fieldfmts[] = {
		"%s"," St:%s"," Dx:%s"," Co:%s"," In:%s"," Wi:%s"," Ch:%s",
		" %s"," S:%s"," %s"," %s"," Pw:%s", "(%s)"," Xp:%s", " AC:%s",
		" HD:%s"," T:%s"," %s"," HP:%s","(%s)","%s","/%s","%s"
		};
	const char *fieldnames[] = {
		"title", "strength", "dexterity", "constitution", "intelligence",
		"wisdom", "charisma", "alignment", "score", "capacity",
		"gold", "power", "power-max", "experience-level", "armor-class",
		"HD", "time", "hunger", "hitpoints", "hitpoints-max",
		"dungeon-level", "experience", "condition"
		};
	if (!reassessment) {
		init_blstats();
		(*windowprocs.win_status_init)();
	}
	for (i = 0; i < MAXBLSTATS; ++i) {
		if ((i == BL_SCORE && !flags.showscore)	||
		    (i == BL_EXP   && !flags.showexp)	||
		    (i == BL_TIME  && !flags.time)	||
		    (i == BL_HD    && !Upolyd)		||
		    ((i == BL_XP || i == BL_EXP) && Upolyd))
			status_enablefield(i, fieldnames[i], fieldfmts[i], FALSE);
		else
			status_enablefield(i, fieldnames[i], fieldfmts[i], TRUE);
	}
	update_all = TRUE;
}

void
status_finish()
{
	int i;

	/* call the window port cleanup routine first */
	(*windowprocs.win_status_finish)();

	/* free memory that we alloc'd now */
	for (i = 0; i < MAXBLSTATS; ++i) {
		free(blstats[0][i].ptr.a_void);
		free(blstats[1][i].ptr.a_void);
		if (blstats[0][i].val) free((genericptr_t)blstats[0][i].val);
		if (blstats[1][i].val) free((genericptr_t)blstats[1][i].val);
	}
}

STATIC_OVL void
init_blstats()
{
	int i;

	for (i = 0; i < MAXBLSTATS; ++i) {
		switch(blstats[0][i].ptype) {
		    case P_INT:
			blstats[0][i].ptr.a_iptr = (int *)alloc(sizeof(int));
			blstats[1][i].ptr.a_iptr = (int *)alloc(sizeof(int));
			*blstats[0][i].ptr.a_iptr = 0;
			*blstats[1][i].ptr.a_iptr = 0;
			break;
		    case P_LNG:
		    case P_MASK:
			blstats[0][i].ptr.a_lptr = (long *)alloc(sizeof(long));
			blstats[1][i].ptr.a_lptr = (long *)alloc(sizeof(long));
			*blstats[0][i].ptr.a_lptr = 0L;
			*blstats[1][i].ptr.a_lptr = 0L;
			break;
		    case P_UINT:
			blstats[0][i].ptr.a_uptr = (unsigned *)alloc(sizeof(unsigned));
			blstats[1][i].ptr.a_uptr = (unsigned *)alloc(sizeof(unsigned));
			*blstats[0][i].ptr.a_uptr = 0;
			*blstats[1][i].ptr.a_uptr = 0;
			break;
		}
		if (blstats[0][i].valwidth) {
		    blstats[0][i].val = (char *)alloc(blstats[0][i].valwidth);
		    blstats[1][i].val = (char *)alloc(blstats[0][i].valwidth);
		} else {
		    blstats[0][i].val = (char *)0;
		    blstats[1][i].val = (char *)0;
		}
		blstats[1][i].ptype = blstats[0][i].ptype;
	}
}

void
bot()
{
	char buf[BUFSZ];
	register char *nb;
	static boolean init = FALSE;
	static int idx = 0, idx_p, idxmax;
	boolean updated = FALSE;
	unsigned ptype;
	int i, pc, cap = near_capacity();
	anything curr, prev;
	boolean valset[MAXBLSTATS];

	idx_p = idx;
	idx   = 1 - idx;	/* 0 -> 1, 1 -> 0 */

	/* clear the "value set" indicators */
	(void) memset((genericptr_t)valset, 0, MAXBLSTATS * sizeof(boolean));

	/*
	 *  Player name and title.
	 */
	buf[0] = '\0';
	Strcpy(buf, plname);
	if('a' <= buf[0] && buf[0] <= 'z') buf[0] += 'A'-'a';
	buf[10] = 0;
	Sprintf(nb = eos(buf)," the ");
	if (Upolyd) {
		char mbot[BUFSZ];
		int k = 0;

		Strcpy(mbot, mons[u.umonnum].mname);
		while(mbot[k] != 0) {
		    if ((k == 0 || (k > 0 && mbot[k-1] == ' ')) &&
					'a' <= mbot[k] && mbot[k] <= 'z')
			mbot[k] += 'A' - 'a';
		    k++;
		}
		Sprintf(nb = eos(nb), mbot);
	} else
		Sprintf(nb = eos(nb), rank());
	Sprintf(blstats[idx][BL_TITLE].val, "%-29s", buf);

	/* Strength */

	buf[0] = '\0';
	*(blstats[idx][BL_STR].ptr.a_iptr) = ACURR(A_STR);
	if (ACURR(A_STR) > 18) {
		if (ACURR(A_STR) > STR18(100))
		    Sprintf(buf,"%2d",ACURR(A_STR)-100);
		else if (ACURR(A_STR) < STR18(100))
		    Sprintf(buf, "18/%02d",ACURR(A_STR)-18);
		else
		    Sprintf(buf,"18/**");
	} else
		Sprintf(buf, "%-1d",ACURR(A_STR));
	Strcpy(blstats[idx][BL_STR].val, buf);
	valset[BL_STR] = TRUE;		/* indicate val already set */


	/*  Dexterity, constitution, intelligence, wisdom, charisma. */
	 
	*(blstats[idx][BL_DX].ptr.a_iptr) = ACURR(A_DEX);
	*(blstats[idx][BL_CO].ptr.a_iptr) = ACURR(A_CON);
	*(blstats[idx][BL_IN].ptr.a_iptr) = ACURR(A_INT);
	*(blstats[idx][BL_WI].ptr.a_iptr) = ACURR(A_WIS);
	*(blstats[idx][BL_CH].ptr.a_iptr) = ACURR(A_CHA);

	/* Alignment */

	Sprintf(blstats[idx][BL_ALIGN].val,
			(u.ualign.type == A_CHAOTIC) ? "Chaotic" :
			(u.ualign.type == A_NEUTRAL) ? "Neutral" : "Lawful");

	/* Score */

	*(blstats[idx][BL_SCORE].ptr.a_lptr) =
#ifdef SCORE_ON_BOTL
		botl_score();
#else
		0;
#endif
	/*  Hit points  */

	*(blstats[idx][BL_HP].ptr.a_iptr) = Upolyd ? u.mh : u.uhp;
	*(blstats[idx][BL_HPMAX].ptr.a_iptr) = Upolyd ? u.mhmax : u.uhpmax;
	if( *(blstats[idx][BL_HP].ptr.a_iptr) < 0)
		*(blstats[idx][BL_HP].ptr.a_iptr) = 0;

	/*  Dungeon level. */

	(void) describe_level(blstats[idx][BL_LEVELDESC].val);

	/* Gold */

	*(blstats[idx][BL_GOLD].ptr.a_lptr) =
#ifndef GOLDOBJ
		   u.ugold;
#else
		   money_cnt(invent);
#endif
	/*
	 * The tty port needs to display the current symbol for gold
	 * as a field header, so to accomodate that we pass gold with
	 * that already included. If a window port needs to use the text
	 * gold amount without the leading "$:" the port will have to
	 * add 2 to the value pointer it was passed in status_update()
	 * for the BL_GOLD case.
	 */
	Sprintf(blstats[idx][BL_GOLD].val, "%c:%ld",
			oc_syms[COIN_CLASS], *(blstats[idx][BL_GOLD].ptr.a_lptr));
	valset[BL_GOLD] = TRUE;		/* indicate val already set */

	/* Power (magical energy) */

	*(blstats[idx][BL_ENE].ptr.a_iptr) = u.uen;
	*(blstats[idx][BL_ENEMAX].ptr.a_iptr) = u.uenmax;

	/* Armor class */

	*(blstats[idx][BL_AC].ptr.a_iptr) = u.uac;

	/* Monster level (if Upolyd) */

	if (Upolyd)
		*(blstats[idx][BL_HD].ptr.a_iptr) = mons[u.umonnum].mlevel;
	else
		*(blstats[idx][BL_HD].ptr.a_iptr) = 0;

	/* Experience */

	*(blstats[idx][BL_XP].ptr.a_iptr) = u.ulevel;
	*(blstats[idx][BL_EXP].ptr.a_lptr) = u.uexp;

	/* Time (moves) */

	*(blstats[idx][BL_TIME].ptr.a_lptr)  = moves;

	/* Hunger */

	*(blstats[idx][BL_HUNGER].ptr.a_uptr) = u.uhs;
	*(blstats[idx][BL_HUNGER].val) = '\0';
	if(strcmp(hu_stat[u.uhs], "        ") != 0)
		Strcpy(blstats[idx][BL_HUNGER].val, hu_stat[u.uhs]);
	valset[BL_HUNGER] = TRUE;

	/* Carrying capacity */

	*(blstats[idx][BL_CAP].val) = '\0';
	*(blstats[idx][BL_CAP].ptr.a_iptr) = cap;
	if(cap > UNENCUMBERED)
		Strcpy(blstats[idx][BL_CAP].val, enc_stat[cap]);
	valset[BL_CAP] = TRUE;

	/* Conditions */

	if (Blind) *(blstats[idx][BL_CONDITION].ptr.a_lptr) |= BL_MASK_BLIND;
	else *(blstats[idx][BL_CONDITION].ptr.a_lptr) &= ~BL_MASK_BLIND;
	
	if (Confusion) *(blstats[idx][BL_CONDITION].ptr.a_lptr) |= BL_MASK_CONF;
	else *(blstats[idx][BL_CONDITION].ptr.a_lptr) &= ~BL_MASK_CONF;

	if (Sick && u.usick_type & SICK_VOMITABLE)
		*(blstats[idx][BL_CONDITION].ptr.a_lptr) |= BL_MASK_FOODPOIS;
	else *(blstats[idx][BL_CONDITION].ptr.a_lptr) &= ~BL_MASK_FOODPOIS;

	if (Sick && u.usick_type & SICK_NONVOMITABLE)
		*(blstats[idx][BL_CONDITION].ptr.a_lptr) |= BL_MASK_ILL;
	else *(blstats[idx][BL_CONDITION].ptr.a_lptr) &= ~BL_MASK_ILL;
				
	if (Hallucination) *(blstats[idx][BL_CONDITION].ptr.a_lptr) |= BL_MASK_HALLU;
	else *(blstats[idx][BL_CONDITION].ptr.a_lptr) &= ~BL_MASK_HALLU;
	
	if (Stunned) *(blstats[idx][BL_CONDITION].ptr.a_lptr) |= BL_MASK_STUNNED;
	else *(blstats[idx][BL_CONDITION].ptr.a_lptr) &= ~BL_MASK_STUNNED;

	if (Slimed) *(blstats[idx][BL_CONDITION].ptr.a_lptr) |= BL_MASK_SLIMED;
	else *(blstats[idx][BL_CONDITION].ptr.a_lptr) &= ~BL_MASK_SLIMED;
	
	/*
	 *  Now pass the changed values to window port.
	 */
	for (i = 0; i < MAXBLSTATS; i++) {
		if (((i == BL_SCORE) && !flags.showscore) ||
		    ((i == BL_EXP)   && !flags.showexp)   ||
		    ((i == BL_TIME)  && !flags.time)      ||
		    ((i == BL_HD)    && !Upolyd)	  ||
		    ((i == BL_XP || i == BL_EXP) && Upolyd))
			continue;
		ptype = blstats[idx][i].ptype;
		switch (ptype) {
		    case P_INT:
			    curr.a_iptr = blstats[idx][i].ptr.a_iptr;
			    prev.a_iptr = blstats[idx_p][i].ptr.a_iptr;
			    if (update_all || (*curr.a_iptr != *prev.a_iptr)) {
				idxmax = blstats[idx][i].idxmax;
				pc = (idxmax) ? percentage(*curr.a_iptr,
				 	*(blstats[idx][idxmax].ptr.a_iptr)) : 0;
				if (!valset[i])
					Sprintf(blstats[idx][i].val,
						"%d", *curr.a_iptr);
				status_update(i,
				    (genericptr_t)blstats[idx][i].val,
				    (update_all || *curr.a_iptr > *prev.a_iptr) ?
					1 : -1, pc);
				updated = TRUE;
			    }
			    break;
		    case P_LNG:
			    curr.a_lptr = blstats[idx][i].ptr.a_lptr;
			    prev.a_lptr = blstats[idx_p][i].ptr.a_lptr;
			    if (update_all || (*curr.a_lptr != *prev.a_lptr)) {
				idxmax = blstats[idx][i].idxmax;
				pc = (idxmax) ? percentagel(*curr.a_lptr,
					*(blstats[idx][idxmax].ptr.a_lptr)) : 0;
				if (!valset[i])
					Sprintf(blstats[idx][i].val,
						"%-1ld", *curr.a_lptr);
				status_update(i,
				    (genericptr_t)blstats[idx][i].val,
				    (update_all || *curr.a_lptr > *prev.a_lptr) ?
					1 : -1, pc);
				updated = TRUE;
			    }
			    break;
		    case P_UINT:
			    curr.a_uptr = blstats[idx][i].ptr.a_uptr;
			    prev.a_uptr = blstats[idx_p][i].ptr.a_uptr;
			    if (update_all || (*curr.a_uptr != *prev.a_uptr)) {
			/*
			 *	idxmax = blstats[idx][i].idxmax);
			 *	pc = (idxmax) ? percentage(*curr.a_uptr,
			 *		*(blstats[idx][idxmax].ptr.a_uptr)) : 0;
			 *	status_via_win(i, val,
			 *	     (*curr.a_uptr > *prev.a_uptr) ? 1 : -1, pc);
			 */
				if (!valset[i])
					Sprintf(blstats[idx][i].val,
						"%u", *curr.a_uptr);
				status_update(i,
				    (genericptr_t)blstats[idx][i].val,
				     (update_all || *curr.a_uptr > *prev.a_uptr) ?
					1 : -1, 0);
				updated = TRUE;
			    }
			    break;
		    case P_STR:
			    if (update_all ||
			    	strcmp(blstats[idx][i].val, blstats[idx_p][i].val)) {
				status_update(i,
				    (genericptr_t) blstats[idx][i].val,0,0);
				updated = TRUE;
			    }
			    break;
		    case P_MASK:
			    curr.a_lptr = blstats[idx][i].ptr.a_lptr;
			    prev.a_lptr = blstats[idx_p][i].ptr.a_lptr;
			    if (update_all || (*curr.a_lptr != *prev.a_lptr)) {
				status_update(i,
				    /* send the actual mask, not a pointer to it */
				    (genericptr_t)*curr.a_lptr,0,0);
				updated = TRUE;
			    }
			    break;
		}
	}
	/*
	 * It is possible to get here, with nothing having been pushed
	 * to the window port, when none of the info has changed. In that
	 * case, we need to force a call to status_update() when
	 * context.botlx is set. The tty port in particular has a problem
	 * if that isn't done, since it sets context.botlx when a menu or
	 * text display obliterates the status line.
	 *
	 * To work around it, we call status_update() with ficticious
	 * index of BL_FLUSH (-1).
	 */
	if (context.botlx && !updated)
		status_update(BL_FLUSH,(genericptr_t)0,0,0);

	context.botl = context.botlx = 0;
	update_all = FALSE;
}

/*****************************************************************************/
/* genl backward compat stuff - probably doesn't belong in botl.c any longer */
/*****************************************************************************/

const char *fieldnm[MAXBLSTATS];
const char *fieldfmt[MAXBLSTATS];
char *vals[MAXBLSTATS];
boolean activefields[MAXBLSTATS];
NEARDATA winid WIN_STATUS;

void
genl_status_init()
{
	int i;
	for (i = 0; i < MAXBLSTATS; ++i) {
		vals[i] = (char *)alloc(MAXCO);
		*vals[i] = '\0';
		activefields[i] = FALSE;
		fieldfmt[i] = FALSE;
	}
	/* Use a window for the genl version; backward port compatibility */
	WIN_STATUS = create_nhwindow(NHW_STATUS);
	display_nhwindow(WIN_STATUS, FALSE);
}

void
genl_status_finish()
{
	/* tear down routine */
	int i;

	/* free alloc'd memory here */
	for (i = 0; i < MAXBLSTATS; ++i)
		free((genericptr_t)vals[i]);
}

void
genl_status_enablefield(fieldidx, nm, fmt, enable)
int fieldidx;
const char *nm;
const char *fmt;
boolean enable;
{
	fieldfmt[fieldidx] = fmt;
	fieldnm[fieldidx] = nm;
	activefields[fieldidx] = enable;
}

void
genl_status_update(idx, ptr, chg, percent)
int idx, chg, percent;
genericptr_t ptr;
{
	char newbot1[MAXCO], newbot2[MAXCO];
	static int init = FALSE;
	long cond;
	register int i;
	int fieldorder1[] = {
		 BL_TITLE, BL_STR, BL_DX,BL_CO, BL_IN,
		 BL_WI, BL_CH,BL_ALIGN, BL_SCORE, -1
		};
	int fieldorder2[] = {
		 BL_LEVELDESC, BL_GOLD, BL_HP, BL_HPMAX,
		 BL_ENE, BL_ENEMAX, BL_AC, BL_XP, BL_EXP, BL_HD,
		 BL_TIME, BL_HUNGER,BL_CAP, BL_CONDITION, -1
		};

	if (idx != BL_FLUSH) {
	    if (!activefields[idx]) return;
	    switch(idx) {
		case BL_CONDITION:
	    		cond = (long)ptr;
	    		*vals[idx] = '\0';
	    		if (cond & BL_MASK_BLIND) Strcat(vals[idx], " Blind");
	    		if (cond & BL_MASK_CONF) Strcat(vals[idx], " Conf");
	    		if (cond & BL_MASK_FOODPOIS)
					Strcat(vals[idx], " FoodPois");
	    		if (cond & BL_MASK_ILL) Strcat(vals[idx], " Ill");
	    		if (cond & BL_MASK_STUNNED) Strcat(vals[idx], " Hallu");
	    		if (cond & BL_MASK_HALLU) Strcat(vals[idx], " Stun");
	    		if (cond & BL_MASK_SLIMED) Strcat(vals[idx], " Slime");
	    		break;
		default:
	    		(void) Sprintf(vals[idx],
					fieldfmt[idx] ? fieldfmt[idx] : "%s",
					(char *)ptr, MAXCO);
	    		break;
	    }
	}

	/* This genl version updates everything on the display, evertime */
	newbot1[0] = '\0';
	for (i = 0; fieldorder1[i] >= 0; ++i) {
	    int idx = fieldorder1[i];
	    if (activefields[idx])
	    	Strcat(newbot1, vals[idx]);
	}
	newbot2[0] = '\0';
	for (i = 0; fieldorder2[i] >= 0; ++i) {
	    int idx = fieldorder2[i];
	    if (activefields[idx])
	    	Strcat(newbot2, vals[idx]);
	}
	curs(WIN_STATUS, 1, 0);
	putstr(WIN_STATUS, 0, newbot1);
	curs(WIN_STATUS, 1, 1);
	putstr(WIN_STATUS, 0, newbot2);
}
#endif /*STATUS_VIA_WINDOWPORT*/

/*botl.c*/


