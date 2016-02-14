%{
/* NetHack 3.6  lev_comp.l	$NHDT-Date: 1455415007 2016/02/14 01:56:47 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.24 $ */
/*	Copyright (c) 1989 by Jean-Christophe Collet */
/* NetHack may be freely redistributed.  See license for details. */

#define LEV_LEX_C

#include "hack.h"
#include "lev_comp.h"
#include "sp_lev.h"

/* Most of these don't exist in flex, yywrap is macro and
 * yyunput is properly declared in flex.skel.
 */
#if !defined(FLEX_SCANNER) && !defined(FLEXHACK_SCANNER)
int FDECL(yyback, (int *,int));
int NDECL(yylook);
int NDECL(yyinput);
int NDECL(yywrap);
int NDECL(yylex);
	/* Traditional lexes let yyunput() and yyoutput() default to int;
	 * newer ones may declare them as void since they don't return
	 * values.  For even more fun, the lex supplied as part of the
	 * newer unbundled compiler for SunOS 4.x adds the void declarations
	 * (under __STDC__ or _cplusplus ifdefs -- otherwise they remain
	 * int) while the bundled lex and the one with the older unbundled
	 * compiler do not.  To detect this, we need help from outside --
	 * sys/unix/Makefile.utl.
	 *
	 * Digital UNIX is difficult and still has int in spite of all
	 * other signs.
	 */
# if defined(NeXT) || defined(SVR4) || defined(_AIX32)
#  define VOIDYYPUT
# endif
# if !defined(VOIDYYPUT) && defined(POSIX_TYPES)
#  if !defined(BOS) && !defined(HISX) && !defined(_M_UNIX) && !defined(VMS)
#   define VOIDYYPUT
#  endif
# endif
# if !defined(VOIDYYPUT) && defined(WEIRD_LEX)
#  if defined(SUNOS4) && defined(__STDC__) && (WEIRD_LEX > 1)
#   define VOIDYYPUT
#  endif
# endif
# if defined(VOIDYYPUT) && defined(__osf__)
#  undef VOIDYYPUT
# endif
# ifdef VOIDYYPUT
void FDECL(yyunput, (int));
void FDECL(yyoutput, (int));
# else
int FDECL(yyunput, (int));
int FDECL(yyoutput, (int));
# endif

#else	/* !FLEX_SCANNER && !FLEXHACK_SCANNER */
/* most recent flex allows suppressing yyunput() altogether when not needed */
#define YY_NO_UNPUT
#define YY_NO_INPUT
#endif

#if defined(FLEX_SCANNER) || defined(FLEXHACK_SCANNER)
/* older flex wants this */
#define YY_MALLOC_DECL  genericptr_t FDECL(malloc, (size_t)); \
                        genericptr_t FDECL(realloc, (genericptr_t, size_t));
/* newer flex assumes <stdlib.h> so needs this in case it's been suppressed */
YY_MALLOC_DECL
#endif

void FDECL(init_yyin, (FILE *));
void FDECL(init_yyout, (FILE *));

long NDECL(handle_varstring_check);
long FDECL(corefunc_str_check, (char *, long));

extern void VDECL(lc_error, (const char *, ...));
extern struct lc_vardefs *FDECL(vardef_defined,(struct lc_vardefs *,char *, int));

extern struct lc_vardefs *variable_definitions;

extern long FDECL(method_defined, (char *, long, long *));

void FDECL(savetoken, (char *));
void NDECL(newline);
void FDECL(advancepos, (char *));

/*
 * This doesn't always get put in lev_comp.h
 * (esp. when using older versions of bison).
 */
extern YYSTYPE yylval;

int nh_line_number = 1;
int token_start_pos = 0;
char curr_token[512];
static char map[4096];
static int map_cnt = 0;

FILE *orig_yyin = NULL;

#define ST_RET(x) do { savetoken(yytext); return x; } while (0);
#define ST_RETF(y, x) do { savetoken(yytext); y; return x; } while (0);

%}
%e 2500
%p 10000
%n 1500
%s MAPC
%%
<MAPC>ENDMAP	{
		  savetoken(yytext);
		  BEGIN(INITIAL);
		  yylval.map = (char *) alloc(map_cnt + 1);
		  (void) strncpy(yylval.map, map, map_cnt);
		  yylval.map[map_cnt] = 0;
		  map_cnt = 0;
		  return MAP_ID;
		}
<MAPC>[-|}{+xABCISHKPLWTF\\#. 0123456789]*\r?\n	{
		  int len = yyleng;
		  savetoken(yytext);
		  /* convert \r\n to \n */
		  if (len >= 2 && yytext[len - 2] == '\r')
		      len -= 1;
		  (void) strncpy(map + map_cnt, yytext, len);
		  map_cnt += len;
		  map[map_cnt - 1] = '\n';
		  map[map_cnt] = '\0';
		  newline();
		}
^[ \t]*#.*\n	{ savetoken(yytext); newline(); }
MESSAGE		ST_RET(MESSAGE_ID);
NOMAP		ST_RET(NOMAP_ID);
MAZE		ST_RET(MAZE_ID);
LEVEL		ST_RET(LEVEL_ID);
INIT_MAP	ST_RET(LEV_INIT_ID);
mazegrid	ST_RET(MAZE_GRID_ID);
solidfill	ST_RET(SOLID_FILL_ID);
mines		ST_RET(MINES_ID);
rogue		ST_RET(ROGUELEV_ID);
FLAGS		ST_RET(FLAGS_ID);
GEOMETRY	ST_RET(GEOMETRY_ID);
^MAP\r?\n	{ savetoken(yytext); BEGIN(MAPC); newline(); }
obj(ect)?	ST_RET(object_ID);
OBJECT		ST_RET(OBJECT_ID);
CONTAINER	ST_RET(COBJECT_ID);
MONSTER		ST_RET(MONSTER_ID);
monster		ST_RET(monster_ID);
TRAP		ST_RET(TRAP_ID);
DOOR		ST_RET(DOOR_ID);
ROOMDOOR	ST_RET(ROOMDOOR_ID);
DRAWBRIDGE	ST_RET(DRAWBRIDGE_ID);
MAZEWALK	ST_RET(MAZEWALK_ID);
WALLIFY		ST_RET(WALLIFY_ID);
REGION		ST_RET(REGION_ID);
ALTAR		ST_RET(ALTAR_ID);
LADDER		ST_RET(LADDER_ID);
STAIR		ST_RET(STAIR_ID);
PORTAL		ST_RET(PORTAL_ID);
TELEPORT_REGION	ST_RET(TELEPRT_ID);
BRANCH		ST_RET(BRANCH_ID);
FOUNTAIN	ST_RET(FOUNTAIN_ID);
SINK		ST_RET(SINK_ID);
POOL		ST_RET(POOL_ID);
NON_DIGGABLE	ST_RET(NON_DIGGABLE_ID);
NON_PASSWALL	ST_RET(NON_PASSWALL_ID);
IF		ST_RET(IF_ID);
ELSE		ST_RET(ELSE_ID);
EXIT		ST_RET(EXIT_ID);
ROOM		ST_RET(ROOM_ID);
SUBROOM		ST_RET(SUBROOM_ID);
RANDOM_CORRIDORS	ST_RET(RAND_CORRIDOR_ID);
CORRIDOR	ST_RET(CORRIDOR_ID);
TERRAIN		ST_RET(TERRAIN_ID);
terrain		ST_RET(terrain_ID);
REPLACE_TERRAIN	ST_RET(REPLACE_TERRAIN_ID);
GOLD		ST_RET(GOLD_ID);
GRAVE		ST_RET(GRAVE_ID);
ENGRAVING	ST_RET(ENGRAVING_ID);
MINERALIZE	ST_RET(MINERALIZE_ID);
(NAME|name)	ST_RET(NAME_ID);
FOR		ST_RET(FOR_ID);
TO		ST_RET(TO_ID);
LOOP		ST_RET(LOOP_ID);
SWITCH		ST_RET(SWITCH_ID);
CASE		ST_RET(CASE_ID);
BREAK		ST_RET(BREAK_ID);
DEFAULT		ST_RET(DEFAULT_ID);
FUNCTION	ST_RET(FUNCTION_ID);
SHUFFLE		ST_RET(SHUFFLE_ID);
montype		ST_RET(MONTYPE_ID);
selection	ST_RET(selection_ID);
rect		ST_RET(rect_ID);
fillrect	ST_RET(fillrect_ID);
line		ST_RET(line_ID);
randline	ST_RET(randline_ID);
grow		ST_RET(grow_ID);
floodfill	ST_RET(flood_ID);
rndcoord	ST_RET(rndcoord_ID);
circle		ST_RET(circle_ID);
ellipse		ST_RET(ellipse_ID);
filter		ST_RET(filter_ID);
gradient	ST_RET(gradient_ID);
complement	ST_RET(complement_ID);
radial		{ savetoken(yytext); yylval.i=SEL_GRADIENT_RADIAL;  return GRADIENT_TYPE; }
square		{ savetoken(yytext); yylval.i=SEL_GRADIENT_SQUARE;  return GRADIENT_TYPE; }
dry		{ savetoken(yytext); yylval.i=DRY;  return HUMIDITY_TYPE; }
wet		{ savetoken(yytext); yylval.i=WET;  return HUMIDITY_TYPE; }
hot		{ savetoken(yytext); yylval.i=HOT;  return HUMIDITY_TYPE; }
solid		{ savetoken(yytext); yylval.i=SOLID;  return HUMIDITY_TYPE; }
any		{ savetoken(yytext); yylval.i=ANY_LOC;  return HUMIDITY_TYPE; }
levregion	ST_RET(LEV);
quantity	ST_RET(QUANTITY_ID);
buried		ST_RET(BURIED_ID);
eroded		ST_RET(ERODED_ID);
erodeproof	ST_RET(ERODEPROOF_ID);
trapped     { savetoken(yytext); yylval.i=1; return TRAPPED_STATE; }
not_trapped { savetoken(yytext); yylval.i=0; return TRAPPED_STATE; }
recharged	ST_RET(RECHARGED_ID);
invisible	ST_RET(INVIS_ID);
greased		ST_RET(GREASED_ID);
female		ST_RET(FEMALE_ID);
cancelled	ST_RET(CANCELLED_ID);
revived		ST_RET(REVIVED_ID);
avenge		ST_RET(AVENGE_ID);
fleeing		ST_RET(FLEEING_ID);
blinded		ST_RET(BLINDED_ID);
paralyzed	ST_RET(PARALYZED_ID);
stunned		ST_RET(STUNNED_ID);
confused	ST_RET(CONFUSED_ID);
seen_traps	ST_RET(SEENTRAPS_ID);
all		ST_RET(ALL_ID);
horizontal	ST_RETF((yylval.i=1), HORIZ_OR_VERT);
vertical	{ savetoken(yytext); yylval.i=2; return HORIZ_OR_VERT; }
open		{ savetoken(yytext); yylval.i=D_ISOPEN; return DOOR_STATE; }
closed		{ savetoken(yytext); yylval.i=D_CLOSED; return DOOR_STATE; }
locked		{ savetoken(yytext); yylval.i=D_LOCKED; return DOOR_STATE; }
nodoor		{ savetoken(yytext); yylval.i=D_NODOOR; return DOOR_STATE; }
broken		{ savetoken(yytext); yylval.i=D_BROKEN; return DOOR_STATE; }
secret		{ savetoken(yytext); yylval.i=D_SECRET; return DOOR_STATE; }
north		{ savetoken(yytext); yylval.i=W_NORTH; return DIRECTION; }
east		{ savetoken(yytext); yylval.i=W_EAST; return DIRECTION; }
south		{ savetoken(yytext); yylval.i=W_SOUTH; return DIRECTION; }
west		{ savetoken(yytext); yylval.i=W_WEST; return DIRECTION; }
random		{ savetoken(yytext); yylval.i = -1; return RANDOM_TYPE; }
random\[	{ savetoken(yytext); yylval.i = -1; return RANDOM_TYPE_BRACKET; }
none		{ savetoken(yytext); yylval.i = -2; return NONE; }
align		ST_RET(A_REGISTER);
left		{ savetoken(yytext); yylval.i=1; return LEFT_OR_RIGHT; }
half-left	{ savetoken(yytext); yylval.i=2; return LEFT_OR_RIGHT; }
center		{ savetoken(yytext); yylval.i=3; return CENTER; }
half-right	{ savetoken(yytext); yylval.i=4; return LEFT_OR_RIGHT; }
right		{ savetoken(yytext); yylval.i=5; return LEFT_OR_RIGHT; }
top		{ savetoken(yytext); yylval.i=1; return TOP_OR_BOT; }
bottom		{ savetoken(yytext); yylval.i=5; return TOP_OR_BOT; }
lit		{ savetoken(yytext); yylval.i=1; return LIGHT_STATE; }
unlit		{ savetoken(yytext); yylval.i=0; return LIGHT_STATE; }
filled		{ savetoken(yytext); yylval.i=1; return FILLING; }
unfilled	{ savetoken(yytext); yylval.i=0; return FILLING; }
regular		{ savetoken(yytext); yylval.i=0; return IRREGULAR; }
irregular	{ savetoken(yytext); yylval.i=1; return IRREGULAR; }
unjoined	{ savetoken(yytext); yylval.i=1; return JOINED; }
joined		{ savetoken(yytext); yylval.i=0; return JOINED; }
limited		{ savetoken(yytext); yylval.i=1; return LIMITED; }
unlimited	{ savetoken(yytext); yylval.i=0; return LIMITED; }
noalign		{ savetoken(yytext); yylval.i= AM_NONE; return ALIGNMENT; }
law		{ savetoken(yytext); yylval.i= AM_LAWFUL; return ALIGNMENT; }
neutral		{ savetoken(yytext); yylval.i= AM_NEUTRAL; return ALIGNMENT; }
chaos		{ savetoken(yytext); yylval.i= AM_CHAOTIC; return ALIGNMENT; }
coaligned	{ savetoken(yytext); yylval.i= AM_SPLEV_CO; return ALIGNMENT; }
noncoaligned	{ savetoken(yytext); yylval.i= AM_SPLEV_NONCO; return ALIGNMENT; }
peaceful	{ savetoken(yytext); yylval.i=1; return MON_ATTITUDE; }
hostile		{ savetoken(yytext); yylval.i=0; return MON_ATTITUDE; }
asleep		{ savetoken(yytext); yylval.i=1; return MON_ALERTNESS; }
awake		{ savetoken(yytext); yylval.i=0; return MON_ALERTNESS; }
m_feature	{ savetoken(yytext); yylval.i= M_AP_FURNITURE; return MON_APPEARANCE; }
m_monster	{ savetoken(yytext); yylval.i= M_AP_MONSTER;   return MON_APPEARANCE; }
m_object	{ savetoken(yytext); yylval.i= M_AP_OBJECT;    return MON_APPEARANCE; }
sanctum		{ savetoken(yytext); yylval.i=2; return ALTAR_TYPE; }
shrine		{ savetoken(yytext); yylval.i=1; return ALTAR_TYPE; }
altar		{ savetoken(yytext); yylval.i=0; return ALTAR_TYPE; }
up		{ savetoken(yytext); yylval.i=1; return UP_OR_DOWN; }
down		{ savetoken(yytext); yylval.i=0; return UP_OR_DOWN; }
false		{ savetoken(yytext); yylval.i=0; return BOOLEAN; }
true		{ savetoken(yytext); yylval.i=1; return BOOLEAN; }
dust		{ savetoken(yytext); yylval.i=DUST; return ENGRAVING_TYPE; }
engrave		{ savetoken(yytext); yylval.i=ENGRAVE; return ENGRAVING_TYPE; }
burn		{ savetoken(yytext); yylval.i=BURN; return ENGRAVING_TYPE; }
mark		{ savetoken(yytext); yylval.i=MARK; return ENGRAVING_TYPE; }
blood		{ savetoken(yytext); yylval.i=ENGR_BLOOD; return ENGRAVING_TYPE; }
blessed		{ savetoken(yytext); yylval.i=1; return CURSE_TYPE; }
uncursed	{ savetoken(yytext); yylval.i=2; return CURSE_TYPE; }
cursed		{ savetoken(yytext); yylval.i=3; return CURSE_TYPE; }
noteleport	{ savetoken(yytext); yylval.i=NOTELEPORT; return FLAG_TYPE; }
hardfloor	{ savetoken(yytext); yylval.i=HARDFLOOR; return FLAG_TYPE; }
nommap		{ savetoken(yytext); yylval.i=NOMMAP; return FLAG_TYPE; }
arboreal	{ savetoken(yytext); yylval.i=ARBOREAL; return FLAG_TYPE; } /* KMH */
shortsighted	{ savetoken(yytext); yylval.i=SHORTSIGHTED; return FLAG_TYPE; }
mazelevel	{ savetoken(yytext); yylval.i=MAZELEVEL; return FLAG_TYPE; }
premapped	{ savetoken(yytext); yylval.i=PREMAPPED; return FLAG_TYPE; }
shroud		{ savetoken(yytext); yylval.i=SHROUD; return FLAG_TYPE; }
graveyard	{ savetoken(yytext); yylval.i=GRAVEYARD; return FLAG_TYPE; }
icedpools	{ savetoken(yytext); yylval.i=ICEDPOOLS; return FLAG_TYPE; }
solidify	{ savetoken(yytext); yylval.i=SOLIDIFY; return FLAG_TYPE; }
corrmaze        { savetoken(yytext); yylval.i=CORRMAZE; return FLAG_TYPE; }
inaccessibles   { savetoken(yytext); yylval.i=CHECK_INACCESSIBLES; return FLAG_TYPE; }
[0-9]+d[0-9]+	{ char *p = index(yytext, 'd');
		  savetoken(yytext);
		  if (p) {
			*p++ = '\0';
			yylval.dice.num = atoi(yytext);
			yylval.dice.die = atoi(p);
		  } else {
			yylval.dice.num = yylval.dice.die = 1;
		  }
		  return DICE;
		}
\[\ *[0-9]+\%\ *\] { savetoken(yytext); yylval.i = atoi(yytext + 1);
		     if (yylval.i < 0 || yylval.i > 100)
			lc_error("Unexpected percentile '%li%%'", yylval.i);
		     return PERCENT; }
-[0-9]+		{ savetoken(yytext); yylval.i=atoi(yytext); return MINUS_INTEGER; }
\+[0-9]+	{ savetoken(yytext); yylval.i=atoi(yytext); return PLUS_INTEGER; }
[0-9]+\%	{ savetoken(yytext); yylval.i = atoi(yytext);
		  if (yylval.i < 0 || yylval.i > 100)
		     lc_error("Unexpected percentile '%li%%'", yylval.i);
		  return SPERCENT; }
[0-9]+		{ savetoken(yytext); yylval.i=atoi(yytext); return INTEGER; }
\"[^"]*\"	{ savetoken(yytext);
		  yytext[yyleng - 1] = '\0'; /* discard the trailing \" */
		  yylval.map = dupstr(yytext + 1); /* skip the first \" */
		  return STRING; }
\$[a-zA-Z_]+	{ savetoken(yytext); return handle_varstring_check(); }
"=="		{ savetoken(yytext); yylval.i = SPO_JE; return COMPARE_TYPE; }
"!="		{ savetoken(yytext); yylval.i = SPO_JNE; return COMPARE_TYPE; }
"<>"		{ savetoken(yytext); yylval.i = SPO_JNE; return COMPARE_TYPE; }
"<="		{ savetoken(yytext); yylval.i = SPO_JLE; return COMPARE_TYPE; }
">="		{ savetoken(yytext); yylval.i = SPO_JGE; return COMPARE_TYPE; }
"<"		{ savetoken(yytext); yylval.i = SPO_JL; return COMPARE_TYPE; }
">"		{ savetoken(yytext); yylval.i = SPO_JG; return COMPARE_TYPE; }
\r?\n		{ newline(); }
[ \t]+		{ advancepos(yytext); }
'\\.'		{ savetoken(yytext); yylval.i = yytext[2]; return CHAR; }
'.'		{ savetoken(yytext); yylval.i = yytext[1]; return CHAR; }
[-_a-zA-Z0-9]+	ST_RET(UNKNOWN_TYPE);
.		{ savetoken(yytext); return yytext[0]; }
%%
#ifdef	AMIGA
long *
alloc(n)
    unsigned n;
{
    return (long *) malloc(n);
}
#endif

/* routine to switch to another input file; needed for flex */
void
init_yyin( input_f )
FILE *input_f;
{
#if defined(FLEX_SCANNER) || defined(FLEXHACK_SCANNER)
    if (yyin)
        yyrestart(input_f);
    else
#endif
        yyin = input_f;

    if (!orig_yyin)
        orig_yyin = yyin;
}
/* analogous routine (for completeness) */
void
init_yyout( output_f )
FILE *output_f;
{
    yyout = output_f;
}

long
handle_varstring_check()
{
    struct lc_vardefs *vd;

    yylval.map = dupstr(yytext);
    if ((vd = vardef_defined(variable_definitions, yytext, 1)) != 0) {
        long l = vd->var_type;
        int a = ((l & SPOVAR_ARRAY) == SPOVAR_ARRAY);

        l &= ~SPOVAR_ARRAY;
        if (l == SPOVAR_INT)
	    return (a ? VARSTRING_INT_ARRAY : VARSTRING_INT);
        if (l == SPOVAR_STRING)
	    return (a ? VARSTRING_STRING_ARRAY : VARSTRING_STRING);
        if (l == SPOVAR_VARIABLE)
	    return (a ? VARSTRING_VAR_ARRAY : VARSTRING_VAR);
        if (l == SPOVAR_COORD)
	    return (a ? VARSTRING_COORD_ARRAY : VARSTRING_COORD);
        if (l == SPOVAR_REGION)
	    return (a ? VARSTRING_REGION_ARRAY : VARSTRING_REGION);
        if (l == SPOVAR_MAPCHAR)
	    return (a ? VARSTRING_MAPCHAR_ARRAY : VARSTRING_MAPCHAR);
        if (l == SPOVAR_MONST)
	    return (a ? VARSTRING_MONST_ARRAY : VARSTRING_MONST);
        if (l == SPOVAR_OBJ)
	    return (a ? VARSTRING_OBJ_ARRAY : VARSTRING_OBJ);
        if (l == SPOVAR_SEL)
	    return (a ? VARSTRING_SEL_ARRAY : VARSTRING_SEL);
    }
    return VARSTRING;
}

void
newline()
{
    nh_line_number++;
    token_start_pos = 0;
    (void) memset((genericptr_t) curr_token, 0, 512);
}

void
savetoken(s)
char *s;
{
    Sprintf(curr_token, "%s", s);
    advancepos(s);
}

void
advancepos(s)
char *s;
{
    token_start_pos += strlen(s);
}

/*lev_comp.l*/
