%{
/* NetHack 3.6  dgn_comp.y	$NHDT-Date: 1432512785 2015/05/25 00:13:05 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/*	Copyright (c) 1989 by Jean-Christophe Collet */
/*	Copyright (c) 1990 by M. Stephenson				  */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file contains the Dungeon Compiler code
 */

/* In case we're using bison in AIX.  This definition must be
 * placed before any other C-language construct in the file
 * excluding comments and preprocessor directives (thanks IBM
 * for this wonderful feature...).
 *
 * Note: some cpps barf on this 'undefined control' (#pragma).
 * Addition of the leading space seems to prevent barfage for now,
 * and AIX will still see the directive in its non-standard locale.
 */

#ifdef _AIX
 #pragma alloca		/* keep leading space! */
#endif

#include "config.h"
#include "date.h"
#include "dgn_file.h"

void FDECL(yyerror, (const char *));
void FDECL(yywarning, (const char *));
int NDECL(yylex);
int NDECL(yyparse);
int FDECL(getchain, (char *));
int NDECL(check_dungeon);
int NDECL(check_branch);
int NDECL(check_level);
void NDECL(init_dungeon);
void NDECL(init_branch);
void NDECL(init_level);
void NDECL(output_dgn);

#define Free(ptr)		free((genericptr_t)ptr)

#ifdef AMIGA
# undef	printf
#ifndef	LATTICE
# define    memset(addr,val,len)    setmem(addr,len,val)
#endif
#endif

#define ERR		(-1)

static struct couple couple;
static struct tmpdungeon tmpdungeon[MAXDUNGEON];
static struct tmplevel tmplevel[LEV_LIMIT];
static struct tmpbranch tmpbranch[BRANCH_LIMIT];

static int in_dungeon = 0, n_dgns = -1, n_levs = -1, n_brs = -1;

extern int fatal_error;
extern const char *fname;
extern FILE *yyin, *yyout;	/* from dgn_lex.c */

%}

%union
{
	int	i;
	char*	str;
}

%token	<i>	INTEGER
%token	<i>	A_DUNGEON BRANCH CHBRANCH LEVEL RNDLEVEL CHLEVEL RNDCHLEVEL
%token	<i>	UP_OR_DOWN PROTOFILE DESCRIPTION DESCRIPTOR LEVELDESC
%token	<i>	ALIGNMENT LEVALIGN ENTRY STAIR NO_UP NO_DOWN PORTAL
%token	<str>	STRING
%type	<i>	optional_int direction branch_type bones_tag
%start	file

%%
file		: /* nothing */
		| dungeons
		  {
			output_dgn();
		  }
		;

dungeons	: dungeon
		| dungeons dungeon
		;

dungeon		: dungeonline
		| dungeondesc
		| branches
		| levels
		;

dungeonline	: A_DUNGEON ':' STRING bones_tag rcouple optional_int
		  {
			init_dungeon();
			Strcpy(tmpdungeon[n_dgns].name, $3);
			tmpdungeon[n_dgns].boneschar = (char)$4;
			tmpdungeon[n_dgns].lev.base = couple.base;
			tmpdungeon[n_dgns].lev.rand = couple.rand;
			tmpdungeon[n_dgns].chance = $6;
			Free($3);
		  }
		;

optional_int	: /* nothing */
		  {
			$$ = 0;
		  }
		| INTEGER
		  {
			$$ = $1;
		  }
		;

dungeondesc	: entry
		| descriptions
		| prototype
		;

entry		: ENTRY ':' INTEGER
		  {
			tmpdungeon[n_dgns].entry_lev = $3;
		  }
		;

descriptions	: desc
		;

desc		: DESCRIPTION ':' DESCRIPTOR
		  {
			if($<i>3 <= TOWN || $<i>3 >= D_ALIGN_CHAOTIC)
			    yyerror("Illegal description - ignoring!");
			else
			    tmpdungeon[n_dgns].flags |= $<i>3 ;
		  }
		| ALIGNMENT ':' DESCRIPTOR
		  {
			if($<i>3 && $<i>3 < D_ALIGN_CHAOTIC)
			    yyerror("Illegal alignment - ignoring!");
			else
			    tmpdungeon[n_dgns].flags |= $<i>3 ;
		  }
		;

prototype	: PROTOFILE ':' STRING
		  {
			Strcpy(tmpdungeon[n_dgns].protoname, $3);
			Free($3);
		  }
		;

levels		: level1
		| level2
		| levdesc
		| chlevel1
		| chlevel2
		;

level1		: LEVEL ':' STRING bones_tag '@' acouple
		  {
			init_level();
			Strcpy(tmplevel[n_levs].name, $3);
			tmplevel[n_levs].boneschar = (char)$4;
			tmplevel[n_levs].lev.base = couple.base;
			tmplevel[n_levs].lev.rand = couple.rand;
			tmpdungeon[n_dgns].levels++;
			Free($3);
		  }
		| RNDLEVEL ':' STRING bones_tag '@' acouple INTEGER
		  {
			init_level();
			Strcpy(tmplevel[n_levs].name, $3);
			tmplevel[n_levs].boneschar = (char)$4;
			tmplevel[n_levs].lev.base = couple.base;
			tmplevel[n_levs].lev.rand = couple.rand;
			tmplevel[n_levs].rndlevs = $7;
			tmpdungeon[n_dgns].levels++;
			Free($3);
		  }
		;

level2		: LEVEL ':' STRING bones_tag '@' acouple INTEGER
		  {
			init_level();
			Strcpy(tmplevel[n_levs].name, $3);
			tmplevel[n_levs].boneschar = (char)$4;
			tmplevel[n_levs].lev.base = couple.base;
			tmplevel[n_levs].lev.rand = couple.rand;
			tmplevel[n_levs].chance = $7;
			tmpdungeon[n_dgns].levels++;
			Free($3);
		  }
		| RNDLEVEL ':' STRING bones_tag '@' acouple INTEGER INTEGER
		  {
			init_level();
			Strcpy(tmplevel[n_levs].name, $3);
			tmplevel[n_levs].boneschar = (char)$4;
			tmplevel[n_levs].lev.base = couple.base;
			tmplevel[n_levs].lev.rand = couple.rand;
			tmplevel[n_levs].chance = $7;
			tmplevel[n_levs].rndlevs = $8;
			tmpdungeon[n_dgns].levels++;
			Free($3);
		  }
		;

levdesc		: LEVELDESC ':' DESCRIPTOR
		  {
			if($<i>3 >= D_ALIGN_CHAOTIC)
			    yyerror("Illegal description - ignoring!");
			else
			    tmplevel[n_levs].flags |= $<i>3 ;
		  }
		| LEVALIGN ':' DESCRIPTOR
		  {
			if($<i>3 && $<i>3 < D_ALIGN_CHAOTIC)
			    yyerror("Illegal alignment - ignoring!");
			else
			    tmplevel[n_levs].flags |= $<i>3 ;
		  }
		;

chlevel1	: CHLEVEL ':' STRING bones_tag STRING '+' rcouple
		  {
			init_level();
			Strcpy(tmplevel[n_levs].name, $3);
			tmplevel[n_levs].boneschar = (char)$4;
			tmplevel[n_levs].chain = getchain($5);
			tmplevel[n_levs].lev.base = couple.base;
			tmplevel[n_levs].lev.rand = couple.rand;
			if(!check_level()) n_levs--;
			else tmpdungeon[n_dgns].levels++;
			Free($3);
			Free($5);
		  }
		| RNDCHLEVEL ':' STRING bones_tag STRING '+' rcouple INTEGER
		  {
			init_level();
			Strcpy(tmplevel[n_levs].name, $3);
			tmplevel[n_levs].boneschar = (char)$4;
			tmplevel[n_levs].chain = getchain($5);
			tmplevel[n_levs].lev.base = couple.base;
			tmplevel[n_levs].lev.rand = couple.rand;
			tmplevel[n_levs].rndlevs = $8;
			if(!check_level()) n_levs--;
			else tmpdungeon[n_dgns].levels++;
			Free($3);
			Free($5);
		  }
		;

chlevel2	: CHLEVEL ':' STRING bones_tag STRING '+' rcouple INTEGER
		  {
			init_level();
			Strcpy(tmplevel[n_levs].name, $3);
			tmplevel[n_levs].boneschar = (char)$4;
			tmplevel[n_levs].chain = getchain($5);
			tmplevel[n_levs].lev.base = couple.base;
			tmplevel[n_levs].lev.rand = couple.rand;
			tmplevel[n_levs].chance = $8;
			if(!check_level()) n_levs--;
			else tmpdungeon[n_dgns].levels++;
			Free($3);
			Free($5);
		  }
		| RNDCHLEVEL ':' STRING bones_tag STRING '+' rcouple INTEGER INTEGER
		  {
			init_level();
			Strcpy(tmplevel[n_levs].name, $3);
			tmplevel[n_levs].boneschar = (char)$4;
			tmplevel[n_levs].chain = getchain($5);
			tmplevel[n_levs].lev.base = couple.base;
			tmplevel[n_levs].lev.rand = couple.rand;
			tmplevel[n_levs].chance = $8;
			tmplevel[n_levs].rndlevs = $9;
			if(!check_level()) n_levs--;
			else tmpdungeon[n_dgns].levels++;
			Free($3);
			Free($5);
		  }
		;

branches	: branch
		| chbranch
		;

branch		: BRANCH ':' STRING '@' acouple branch_type direction
		  {
			init_branch();
			Strcpy(tmpbranch[n_brs].name, $3);
			tmpbranch[n_brs].lev.base = couple.base;
			tmpbranch[n_brs].lev.rand = couple.rand;
			tmpbranch[n_brs].type = $6;
			tmpbranch[n_brs].up = $7;
			if(!check_branch()) n_brs--;
			else tmpdungeon[n_dgns].branches++;
			Free($3);
		  }
		;

chbranch	: CHBRANCH ':' STRING STRING '+' rcouple branch_type direction
		  {
			init_branch();
			Strcpy(tmpbranch[n_brs].name, $3);
			tmpbranch[n_brs].chain = getchain($4);
			tmpbranch[n_brs].lev.base = couple.base;
			tmpbranch[n_brs].lev.rand = couple.rand;
			tmpbranch[n_brs].type = $7;
			tmpbranch[n_brs].up = $8;
			if(!check_branch()) n_brs--;
			else tmpdungeon[n_dgns].branches++;
			Free($3);
			Free($4);
		  }
		;

branch_type	: /* nothing */
		  {
			$$ = TBR_STAIR;	/* two way stair */
		  }
		| STAIR
		  {
			$$ = TBR_STAIR;	/* two way stair */
		  }
		| NO_UP
		  {
			$$ = TBR_NO_UP;	/* no up staircase */
		  }
		| NO_DOWN
		  {
			$$ = TBR_NO_DOWN;	/* no down staircase */
		  }
		| PORTAL
		  {
			$$ = TBR_PORTAL;	/* portal connection */
		  }
		;

direction	: /* nothing */
		  {
			$$ = 0;	/* defaults to down */
		  }
		| UP_OR_DOWN
		  {
			$$ = $1;
		  }
		;

bones_tag	: STRING
		  {
			char *p = $1;
			if (strlen(p) != 1) {
			    if (strcmp(p, "none") != 0)
		   yyerror("Bones marker must be a single char, or \"none\"!");
			    *p = '\0';
			}
			$$ = *p;
			Free(p);
		  }
		;

/*
 *	acouple rules:
 *
 *	(base, range) where:
 *
 *	    base is either a positive or negative integer with a value
 *	    less than or equal to MAXLEVEL.
 *	    base > 0 indicates the base level.
 *	    base < 0 indicates reverse index (-1 == lowest level)
 *
 *	    range is the random component.
 *	    if range is zero, there is no random component.
 *	    if range is -1 the dungeon loader will randomize between
 *	    the base and the end of the dungeon.
 *	    during dungeon load, range is always *added* to the base,
 *	    therefore range + base(converted) must not exceed MAXLEVEL.
 */
acouple		: '(' INTEGER ',' INTEGER ')'
		  {
			if ($2 < -MAXLEVEL || $2 > MAXLEVEL) {
			    yyerror("Abs base out of dlevel range - zeroing!");
			    couple.base = couple.rand = 0;
			} else if ($4 < -1 ||
				(($2 < 0) ? (MAXLEVEL + $2 + $4 + 1) > MAXLEVEL :
					($2 + $4) > MAXLEVEL)) {
			    yyerror("Abs range out of dlevel range - zeroing!");
			    couple.base = couple.rand = 0;
			} else {
			    couple.base = $2;
			    couple.rand = $4;
			}
		  }
		;

/*
 *	rcouple rules:
 *
 *	(base, range) where:
 *
 *	    base is either a positive or negative integer with a value
 *	    less than or equal to MAXLEVEL.
 *	    base > 0 indicates a forward index.
 *	    base < 0 indicates a reverse index.
 *	    base == 0 indicates on the parent level.
 *
 *	    range is the random component.
 *	    if range is zero, there is no random component.
 *	    during dungeon load, range is always *added* to the base,
 *	    range + base(converted) may be very large.  The dungeon
 *	    loader will then correct to "between here and the top/bottom".
 *
 *	    There is no practical way of specifying "between here and the
 *	    nth / nth last level".
 */
rcouple		: '(' INTEGER ',' INTEGER ')'
		  {
			if ($2 < -MAXLEVEL || $2 > MAXLEVEL) {
			    yyerror("Rel base out of dlevel range - zeroing!");
			    couple.base = couple.rand = 0;
			} else {
			    couple.base = $2;
			    couple.rand = $4;
			}
		  }
		;
%%

void
init_dungeon()
{
	if(++n_dgns > MAXDUNGEON) {
	    (void) fprintf(stderr, "FATAL - Too many dungeons (limit: %d).\n",
		    MAXDUNGEON);
	    (void) fprintf(stderr, "To increase the limit edit MAXDUNGEON in global.h\n");
	    exit(EXIT_FAILURE);
	}

	in_dungeon = 1;
	tmpdungeon[n_dgns].lev.base = 0;
	tmpdungeon[n_dgns].lev.rand = 0;
	tmpdungeon[n_dgns].chance = 100;
	Strcpy(tmpdungeon[n_dgns].name, "");
	Strcpy(tmpdungeon[n_dgns].protoname, "");
	tmpdungeon[n_dgns].flags = 0;
	tmpdungeon[n_dgns].levels = 0;
	tmpdungeon[n_dgns].branches = 0;
	tmpdungeon[n_dgns].entry_lev = 0;
}

void
init_level()
{
	if(++n_levs > LEV_LIMIT) {

		yyerror("FATAL - Too many special levels defined.");
		exit(EXIT_FAILURE);
	}
	tmplevel[n_levs].lev.base = 0;
	tmplevel[n_levs].lev.rand = 0;
	tmplevel[n_levs].chance = 100;
	tmplevel[n_levs].rndlevs = 0;
	tmplevel[n_levs].flags = 0;
	Strcpy(tmplevel[n_levs].name, "");
	tmplevel[n_levs].chain = -1;
}

void
init_branch()
{
	if(++n_brs > BRANCH_LIMIT) {

		yyerror("FATAL - Too many special levels defined.");
		exit(EXIT_FAILURE);
	}
	tmpbranch[n_brs].lev.base = 0;
	tmpbranch[n_brs].lev.rand = 0;
	Strcpy(tmpbranch[n_brs].name, "");
	tmpbranch[n_brs].chain = -1;
}

int
getchain(s)
	char	*s;
{
	int i;

	if(strlen(s)) {

	    for(i = n_levs - tmpdungeon[n_dgns].levels + 1; i <= n_levs; i++)
		if(!strcmp(tmplevel[i].name, s)) return i;

	    yyerror("Can't locate the specified chain level.");
	    return(-2);
	}
	return(-1);
}

/*
 *	Consistancy checking routines:
 *
 *	- A dungeon must have a unique name.
 *	- A dungeon must have a originating "branch" command
 *	  (except, of course, for the first dungeon).
 *	- A dungeon must have a proper depth (at least (1, 0)).
 */

int
check_dungeon()
{
	int i;

	for(i = 0; i < n_dgns; i++)
	    if(!strcmp(tmpdungeon[i].name, tmpdungeon[n_dgns].name)) {
		yyerror("Duplicate dungeon name.");
		return(0);
	    }

	if(n_dgns)
	  for(i = 0; i < n_brs - tmpdungeon[n_dgns].branches; i++) {
	    if(!strcmp(tmpbranch[i].name, tmpdungeon[n_dgns].name)) break;

	    if(i >= n_brs - tmpdungeon[n_dgns].branches) {
		yyerror("Dungeon cannot be reached.");
		return(0);
	    }
	  }

	if(tmpdungeon[n_dgns].lev.base <= 0 ||
	   tmpdungeon[n_dgns].lev.rand < 0) {
		yyerror("Invalid dungeon depth specified.");
		return(0);
	}
	return(1);	/* OK */
}

/*
 *	- A level must have a unique level name.
 *	- If chained, the level used as reference for the chain
 *	  must be in this dungeon, must be previously defined, and
 *	  the level chained from must be "non-probabilistic" (ie.
 *	  have a 100% chance of existing).
 */

int
check_level()
{
	int i;

	if(!in_dungeon) {
		yyerror("Level defined outside of dungeon.");
		return(0);
	}

	for(i = 0; i < n_levs; i++)
	    if(!strcmp(tmplevel[i].name, tmplevel[n_levs].name)) {
		yyerror("Duplicate level name.");
		return(0);
	    }

	if(tmplevel[i].chain == -2) {
		yyerror("Invaild level chain reference.");
		return(0);
	} else if(tmplevel[i].chain != -1) {	/* there is a chain */
	    /* KMH -- tmplevel[tmpbranch[i].chain].chance was in error */
	    if(tmplevel[tmplevel[i].chain].chance != 100) {
		yyerror("Level cannot chain from a probabilistic level.");
		return(0);
	    } else if(tmplevel[i].chain == n_levs) {
		yyerror("A level cannot chain to itself!");
		return(0);
	    }
	}
	return(1);	/* OK */
}

/*
 *	- A branch may not branch backwards - to avoid branch loops.
 *	- A branch name must be unique.
 *	  (ie. You can only have one entry point to each dungeon).
 *	- If chained, the level used as reference for the chain
 *	  must be in this dungeon, must be previously defined, and
 *	  the level chained from must be "non-probabilistic" (ie.
 *	  have a 100% chance of existing).
 */

int
check_branch()
{
	int i;

	if(!in_dungeon) {
		yyerror("Branch defined outside of dungeon.");
		return(0);
	}

	for(i = 0; i < n_dgns; i++)
	    if(!strcmp(tmpdungeon[i].name, tmpbranch[n_brs].name)) {

		yyerror("Reverse branching not allowed.");
		return(0);
	    }

	if(tmpbranch[i].chain == -2) {

		yyerror("Invaild branch chain reference.");
		return(0);
	} else if(tmpbranch[i].chain != -1) {	/* it is chained */

	    if(tmplevel[tmpbranch[i].chain].chance != 100) {
		yyerror("Branch cannot chain from a probabilistic level.");
		return(0);
	    }
	}
	return(1);	/* OK */
}

/*
 *	Output the dungon definition into a file.
 *
 *	The file will have the following format:
 *
 *	[ nethack version ID ]
 *	[ number of dungeons ]
 *	[ first dungeon struct ]
 *	[ levels for the first dungeon ]
 *	  ...
 *	[ branches for the first dungeon ]
 *	  ...
 *	[ second dungeon struct ]
 *	  ...
 */

void
output_dgn()
{
	int	nd, cl = 0, nl = 0,
		    cb = 0, nb = 0;
	static struct version_info version_data = {
			VERSION_NUMBER, VERSION_FEATURES,
			VERSION_SANITY1, VERSION_SANITY2, VERSION_SANITY3
	};

	if(++n_dgns <= 0) {
	    yyerror("FATAL - no dungeons were defined.");
	    exit(EXIT_FAILURE);
	}

	if (fwrite((char *)&version_data, sizeof version_data, 1, yyout) != 1) {
	    yyerror("FATAL - output failure.");
	    exit(EXIT_FAILURE);
	}

	(void) fwrite((char *)&n_dgns, sizeof(int), 1, yyout);
	for (nd = 0; nd < n_dgns; nd++) {
	    (void) fwrite((char *)&tmpdungeon[nd], sizeof(struct tmpdungeon),
							1, yyout);

	    nl += tmpdungeon[nd].levels;
	    for(; cl < nl; cl++)
		(void) fwrite((char *)&tmplevel[cl], sizeof(struct tmplevel),
							1, yyout);

	    nb += tmpdungeon[nd].branches;
	    for(; cb < nb; cb++)
		(void) fwrite((char *)&tmpbranch[cb], sizeof(struct tmpbranch),
							1, yyout);
	}
	/* apparently necessary for Think C 5.x, otherwise harmless */
	(void) fflush(yyout);
}

/*dgn_comp.y*/
