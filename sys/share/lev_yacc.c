#ifndef lint
static char yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93";
#endif
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define yyclearin (yychar=(-1))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING (yyerrflag!=0)
#define YYPREFIX "yy"
/* NetHack 3.5  lev_comp.y	$Date: 2009/05/07 00:58:22 $  $Revision: 1.9 $ */
/*	SCCS Id: @(#)lev_yacc.c	3.5	2007/08/01	*/
/*	Copyright (c) 1989 by Jean-Christophe Collet */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file contains the Level Compiler code
 * It may handle special mazes & special room-levels
 */

/* In case we're using bison in AIX.  This definition must be
 * placed before any other C-language construct in the file
 * excluding comments and preprocessor directives (thanks IBM
 * for this wonderful feature...).
 *
 * Note: some cpps barf on this 'undefined control' (#pragma).
 * Addition of the leading space seems to prevent barfage for now,
 * and AIX will still see the directive.
 */
#ifdef _AIX
 #pragma alloca		/* keep leading space! */
#endif

#include "hack.h"
#include "sp_lev.h"

#define MAX_REGISTERS	10
#define ERR		(-1)
/* many types of things are put in chars for transference to NetHack.
 * since some systems will use signed chars, limit everybody to the
 * same number for portability.
 */
#define MAX_OF_TYPE	128

#define New(type)		\
	(type *) memset((genericptr_t)alloc(sizeof(type)), 0, sizeof(type))
#define NewTab(type, size)	(type **) alloc(sizeof(type *) * size)
#define Free(ptr)		free((genericptr_t)ptr)

extern void FDECL(yyerror, (const char *));
extern void FDECL(yywarning, (const char *));
extern int NDECL(yylex);
int NDECL(yyparse);

extern int FDECL(get_floor_type, (CHAR_P));
extern int FDECL(get_room_type, (char *));
extern int FDECL(get_trap_type, (char *));
extern int FDECL(get_monster_id, (char *,CHAR_P));
extern int FDECL(get_object_id, (char *,CHAR_P));
extern boolean FDECL(check_monster_char, (CHAR_P));
extern boolean FDECL(check_object_char, (CHAR_P));
extern char FDECL(what_map_char, (CHAR_P));
extern void FDECL(scan_map, (char *));
extern void NDECL(wallify_map);
extern boolean NDECL(check_subrooms);
extern void FDECL(check_coord, (int,int,const char *));
extern void NDECL(store_part);
extern void NDECL(store_room);
extern boolean FDECL(write_level_file, (char *,splev *,specialmaze *));
extern void FDECL(free_rooms, (splev *));

static struct reg {
	int x1, y1;
	int x2, y2;
}		current_region;

static struct coord {
	int x;
	int y;
}		current_coord, current_align;

static struct size {
	int height;
	int width;
}		current_size;

char tmpmessage[256];
digpos *tmppass[32];
char *tmpmap[ROWNO];

digpos *tmpdig[MAX_OF_TYPE];
region *tmpreg[MAX_OF_TYPE];
lev_region *tmplreg[MAX_OF_TYPE];
door *tmpdoor[MAX_OF_TYPE];
drawbridge *tmpdb[MAX_OF_TYPE];
walk *tmpwalk[MAX_OF_TYPE];

room_door *tmprdoor[MAX_OF_TYPE];
trap *tmptrap[MAX_OF_TYPE];
monster *tmpmonst[MAX_OF_TYPE];
object *tmpobj[MAX_OF_TYPE];
altar *tmpaltar[MAX_OF_TYPE];
lad *tmplad[MAX_OF_TYPE];
stair *tmpstair[MAX_OF_TYPE];
gold *tmpgold[MAX_OF_TYPE];
engraving *tmpengraving[MAX_OF_TYPE];
fountain *tmpfountain[MAX_OF_TYPE];
sink *tmpsink[MAX_OF_TYPE];
pool *tmppool[MAX_OF_TYPE];

mazepart *tmppart[10];
room *tmproom[MAXNROFROOMS*2];
corridor *tmpcor[MAX_OF_TYPE];

static specialmaze maze;
static splev special_lev;
static lev_init init_lev;

static char olist[MAX_REGISTERS], mlist[MAX_REGISTERS];
static struct coord plist[MAX_REGISTERS];

int n_olist = 0, n_mlist = 0, n_plist = 0;

unsigned int nlreg = 0, nreg = 0, ndoor = 0, ntrap = 0, nmons = 0, nobj = 0;
unsigned int ndb = 0, nwalk = 0, npart = 0, ndig = 0, nlad = 0, nstair = 0;
unsigned int naltar = 0, ncorridor = 0, nrooms = 0, ngold = 0, nengraving = 0;
unsigned int nfountain = 0, npool = 0, nsink = 0, npass = 0;

static int lev_flags = 0;

unsigned int max_x_map, max_y_map;

static xchar in_room;

extern int fatal_error;
extern int want_warnings;
extern const char *fname;

typedef union
{
	int	i;
	char*	map;
	struct {
		xchar room;
		xchar wall;
		xchar door;
	} corpos;
} YYSTYPE;
#define CHAR 257
#define INTEGER 258
#define BOOLEAN 259
#define PERCENT 260
#define MESSAGE_ID 261
#define MAZE_ID 262
#define LEVEL_ID 263
#define LEV_INIT_ID 264
#define GEOMETRY_ID 265
#define NOMAP_ID 266
#define OBJECT_ID 267
#define COBJECT_ID 268
#define MONSTER_ID 269
#define TRAP_ID 270
#define DOOR_ID 271
#define DRAWBRIDGE_ID 272
#define MAZEWALK_ID 273
#define WALLIFY_ID 274
#define REGION_ID 275
#define FILLING 276
#define RANDOM_OBJECTS_ID 277
#define RANDOM_MONSTERS_ID 278
#define RANDOM_PLACES_ID 279
#define ALTAR_ID 280
#define LADDER_ID 281
#define STAIR_ID 282
#define NON_DIGGABLE_ID 283
#define NON_PASSWALL_ID 284
#define ROOM_ID 285
#define PORTAL_ID 286
#define TELEPRT_ID 287
#define BRANCH_ID 288
#define LEV 289
#define CHANCE_ID 290
#define CORRIDOR_ID 291
#define GOLD_ID 292
#define ENGRAVING_ID 293
#define FOUNTAIN_ID 294
#define POOL_ID 295
#define SINK_ID 296
#define NONE 297
#define RAND_CORRIDOR_ID 298
#define DOOR_STATE 299
#define LIGHT_STATE 300
#define CURSE_TYPE 301
#define ENGRAVING_TYPE 302
#define DIRECTION 303
#define RANDOM_TYPE 304
#define O_REGISTER 305
#define M_REGISTER 306
#define P_REGISTER 307
#define A_REGISTER 308
#define ALIGNMENT 309
#define LEFT_OR_RIGHT 310
#define CENTER 311
#define TOP_OR_BOT 312
#define ALTAR_TYPE 313
#define UP_OR_DOWN 314
#define SUBROOM_ID 315
#define NAME_ID 316
#define FLAGS_ID 317
#define FLAG_TYPE 318
#define MON_ATTITUDE 319
#define MON_ALERTNESS 320
#define MON_APPEARANCE 321
#define CONTAINED 322
#define STRING 323
#define MAP_ID 324
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,    0,   36,   36,   37,   37,   38,   39,   32,   23,
   23,   23,   14,   14,   19,   19,   20,   20,   40,   40,
   45,   42,   42,   46,   46,   43,   43,   49,   49,   44,
   44,   51,   52,   52,   53,   53,   35,   50,   50,   56,
   54,   10,   10,   59,   59,   57,   57,   60,   60,   58,
   58,   55,   55,   61,   61,   61,   61,   61,   61,   61,
   61,   61,   61,   61,   61,   61,   62,   63,   64,   15,
   15,   13,   13,   12,   12,   31,   11,   11,   41,   41,
   75,   76,   76,   79,    1,    1,    2,    2,   77,   77,
   80,   80,   80,   47,   47,   48,   48,   81,   83,   81,
   78,   78,   84,   84,   84,   84,   84,   84,   84,   84,
   84,   84,   84,   84,   84,   84,   84,   84,   84,   84,
   84,   84,   99,   65,   98,   98,  100,  100,  100,  100,
  100,   66,   66,  102,  101,  103,  103,  104,  104,  104,
  104,  105,  105,  106,  107,  107,  108,  108,  108,   85,
   67,   86,   92,   93,   94,   74,  109,   88,  110,   89,
  111,  113,   90,  114,   91,  112,  112,   22,   22,   69,
   70,   71,   95,   96,   87,   68,   72,   73,   25,   25,
   25,   28,   28,   28,   33,   33,   34,   34,    3,    3,
    4,    4,   21,   21,   21,   97,   97,   97,    5,    5,
    6,    6,    7,    7,    7,    8,    8,  117,   29,   26,
    9,   82,   24,   27,   30,   16,   16,   17,   17,   18,
   18,  116,  115,
};
short yylen[] = {                                         2,
    0,    1,    1,    2,    1,    1,    5,    7,    3,    0,
   13,   15,    1,    1,    0,    3,    3,    1,    0,    2,
    3,    0,    2,    3,    3,    0,    1,    1,    2,    1,
    1,    1,    0,    2,    5,    5,    7,    2,    2,   12,
   12,    0,    2,    5,    1,    5,    1,    5,    1,    5,
    1,    0,    2,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    3,    3,    9,    1,
    1,    1,    1,    1,    1,    5,    1,    1,    1,    2,
    3,    1,    2,    5,    1,    1,    1,    1,    0,    2,
    3,    3,    3,    1,    3,    1,    3,    1,    0,    4,
    0,    2,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    0,   10,    0,    2,    2,    2,    2,    2,
    3,    2,    2,    0,    9,    1,    1,    0,    7,    5,
    5,    1,    1,    1,    1,    1,    0,    2,    2,    5,
    6,    7,    5,    1,    5,    5,    0,    8,    0,    8,
    0,    0,    8,    0,    6,    0,    2,    1,   10,    3,
    3,    3,    3,    3,    8,    7,    5,    7,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    0,    2,    4,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    4,    4,    4,
    4,    1,    1,    1,    1,    1,    1,    0,    1,    1,
    1,    5,    9,
};
short yydefred[] = {                                      0,
    0,    0,    0,    0,    0,    2,    0,    5,    6,    0,
    0,    0,    0,    0,    4,  215,    0,    9,    0,    0,
    0,    0,    0,    0,   16,    0,    0,    0,    0,   22,
   77,   78,   76,    0,    0,    0,    0,   82,    7,    0,
   89,    0,   20,    0,   17,    0,   21,    0,   80,    0,
   83,    0,    0,    0,    0,    0,   23,   27,    0,   52,
   52,    0,   85,   86,    0,    0,    0,    0,    0,   90,
    0,    0,    0,    0,   32,    8,   30,    0,   29,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  154,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  103,  104,  106,  113,
  114,  119,  120,  118,  102,  105,  107,  108,  109,  110,
  111,  112,  115,  116,  117,  121,  122,  214,    0,   24,
  213,    0,   25,  192,    0,  191,    0,    0,   34,    0,
    0,    0,    0,    0,    0,   53,   54,   55,   56,   57,
   58,   59,   60,   61,   62,   63,   64,   65,   66,    0,
   88,   87,   84,   91,   93,    0,   92,    0,  212,  219,
    0,  132,  133,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  199,  200,    0,
  198,    0,    0,  196,  197,    0,    0,    0,    0,    0,
    0,    0,  157,    0,  168,  173,  174,  159,  161,  164,
  216,  217,    0,    0,  170,   95,   97,  201,  202,    0,
    0,    0,    0,   70,   71,    0,   68,  172,  171,   67,
    0,    0,    0,  183,    0,  182,    0,  184,  180,    0,
  179,    0,  181,  190,    0,  189,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  100,    0,    0,    0,    0,    0,  150,    0,    0,  153,
    0,    0,  205,    0,  203,    0,  204,  155,    0,    0,
    0,  156,    0,    0,    0,  177,  220,  221,    0,   45,
    0,    0,   47,    0,    0,    0,   36,   35,    0,    0,
  222,    0,  188,  187,  134,    0,  186,  185,    0,  151,
  208,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  162,  165,    0,    0,    0,    0,    0,    0,    0,    0,
  209,    0,  210,    0,  152,    0,    0,    0,  207,  206,
  176,    0,    0,    0,    0,  178,    0,   49,    0,    0,
    0,   51,    0,    0,    0,   72,   73,    0,   13,   14,
    0,    0,  123,    0,    0,  175,  211,    0,  158,  160,
    0,  163,    0,    0,    0,    0,    0,    0,   74,   75,
    0,    0,    0,  137,  136,    0,  125,    0,    0,    0,
  167,   44,    0,    0,   46,    0,    0,   37,   69,   12,
    0,  135,    0,    0,    0,    0,    0,    0,   41,    0,
   40,  143,  142,  144,    0,    0,    0,  126,  223,  195,
    0,   48,   43,   50,    0,    0,  128,  129,    0,  130,
  127,  169,  146,  145,    0,    0,    0,  131,    0,    0,
  140,  141,    0,  148,  149,  139,
};
short yydgoto[] = {                                       3,
   65,  163,  265,  135,  210,  240,  306,  371,  307,  439,
   33,  411,  388,  391,  246,  233,  171,  319,   13,   25,
  396,  223,   21,  132,  262,  263,  129,  257,  258,  136,
    4,    5,  339,  335,  243,    6,    7,    8,    9,   28,
   39,   44,   56,   76,   29,   57,  130,  133,   58,   59,
   77,   78,  139,   60,   80,   61,  325,  384,  322,  380,
  146,  147,  148,  149,  150,  151,  152,  153,  154,  155,
  156,  157,  158,  159,   40,   41,   50,   69,   42,   70,
  167,  168,  204,  115,  116,  117,  118,  119,  120,  121,
  122,  123,  124,  125,  126,  127,  224,  433,  417,  448,
  172,  362,  416,  432,  445,  446,  466,  471,  277,  279,
  280,  402,  375,  281,  225,  214,  215,
};
short yysindex[] = {                                   -149,
   -5,    1,    0, -269, -269,    0, -149,    0,    0, -242,
 -242,   12, -154, -154,    0,    0,   50,    0, -200,   41,
 -137, -137, -232,  103,    0, -103,   99, -132, -137,    0,
    0,    0,    0, -200,  126, -141,  128,    0,    0, -132,
    0, -136,    0, -251,    0,  -68,    0, -158,    0, -139,
    0,  129,  132,  134,  135, -100,    0,    0, -253,    0,
    0,  151,    0,    0,  155,  144,  146,  147, -109,    0,
  -51,  -50, -260, -260,    0,    0,    0,  -83,    0, -165,
 -165,  -49, -162,  -51,  -50,  174,  -44,  -44,  -44,  -44,
  159,  160,  161,    0,  162,  163,  166,  167,  168,  170,
  171,  172,  173,  175,  176,  177,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  165,    0,
    0,  183,    0,    0,  188,    0,  192,  179,    0,  180,
  181,  182,  185,  186,  187,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  197,
    0,    0,    0,    0,    0,  -12,    0,    0,    0,    0,
  189,    0,    0,  190,  191, -212,   46,   46,  210,   46,
   46,   58,  210,  210,  -37,  -37,  -37, -230,   46,   46,
  -51,  -50, -193, -193,  211, -237,   46,   -4,   46,   46,
 -242,   -6,  212,  213, -221, -233, -258,    0,    0,  214,
    0,  164,  215,    0,    0,  216,    3,  218,  219,  226,
  267,   96,    0,  312,    0,    0,    0,    0,    0,    0,
    0,    0,  313,  317,    0,    0,    0,    0,    0,  319,
  322,  110,  325,    0,    0,  326,    0,    0,    0,    0,
  329,  130,  174,    0,  296,    0,  365,    0,    0,  321,
    0,  369,    0,    0,  376,    0,   46,  178,  119,  120,
  383, -193, -208,  115,  193,  403,  404,  136,  409,  410,
  411,   46, -161,   -1,   28,  412,  -35, -212, -193,  416,
    0,  194, -257,  200, -228,   46,    0,  366,  417,    0,
  202,  418,    0,  388,    0,  423,    0,    0,  448,  235,
  -37,    0,  -37,  -37,  -37,    0,    0,    0,  454,    0,
  241,  456,    0,  244,  459,  228,    0,    0,  488,  493,
    0,  449,    0,    0,    0,  458,    0,    0,  494,    0,
    0, -212,  497, -260,  291, -259,  294,   49,  509,  510,
    0,    0, -242,  511,   40,  512,   42,  514, -148, -219,
    0,  517,    0,   46,    0,  304,  519,  471,    0,    0,
    0,  521,  252, -242,  524,    0,  311,    0, -158,  526,
  316,    0,  318,  527, -227,    0,    0,  531,    0,    0,
  533,   38,    0,  534,  303,    0,    0,  323,    0,    0,
  268,    0,  542,  540,   42,  544,  543, -242,    0,    0,
  545, -227,  330,    0,    0,  546,    0,  333,  548,  549,
    0,    0, -162,  550,    0,  337,  550,    0,    0,    0,
 -263,    0,  552,  547,  339,  341,  559,  345,    0,  567,
    0,    0,    0,    0,  565,  570, -108,    0,    0,    0,
  574,    0,    0,    0, -235, -225,    0,    0, -242,    0,
    0,    0,    0,    0,  575,  576,  576,    0, -225, -262,
    0,    0,  576,    0,    0,    0,
};
short yyrindex[] = {                                    616,
    0,    0,    0, -129,  349,    0,  621,    0,    0,    0,
    0,    0, -210,  406,    0,    0,    0,    0,    0,    0,
  -98,  408,    0,  282,    0,    0,    0,    0,  367,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    2,
    0,    0,    0,   57,    0,    0,    0,    0,    0,  539,
    0,    0,    0,    0,    0,    4,    0,    0,  123,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  169,    0,
    0,    0,    0,    0,    0,    0,    0,   89,    0,  426,
  445,    0,    0,    0,    0,    0,  564,  564,  564,  564,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  203,    0,
    0,  242,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  506,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  572,    0,    0,    0,
    0,    0,    0,    0,  607,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  340,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    6,    0,    0,  641,    0,
    0,    0,    0,  148,    0,    0,  148,    0,    0,    0,
    0,    0,   43,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  109,  109,    0,    0,    0,
    0,    0,  109,    0,    0,    0,
};
short yygindex[] = {                                      0,
  245,  205,    0,  -60, -267, -181,  195,    0,    0,  196,
    0,  223,    0,    0,    0,    0,   91,    0,  631,  603,
    0, -169,  625,  437,    0,    0,  441,    0,    0,  -10,
    0,    0,    0,    0,  361,  642,    0,    0,    0,   20,
  610,    0,    0,    0,    0,    0,  -72,  -70,  592,    0,
    0,    0,    0,    0,  593,    0,    0,  248,    0,    0,
    0,    0,    0,    0,  587,  588,  590,  591,  594,    0,
    0,  597,  604,  605,    0,    0,    0,    0,    0,    0,
  419,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0, -170,    0,    0,    0,
  573,    0,    0,    0,    0,  207, -419, -415,    0,    0,
    0,    0,    0,    0,  -63,  -77,    0,
};
#define YYTABLESIZE 935
short yytable[] = {                                      17,
   18,   79,  217,   33,  242,  138,  213,  216,  169,  219,
  220,  164,  241,  137,  165,  228,  229,  230,  234,  235,
  329,  244,  463,  131,   31,   52,   53,  231,  248,  249,
  409,   54,  463,   54,  474,  128,  467,  442,  321,  389,
  443,   30,  124,  134,  369,  264,  333,   12,   43,  473,
   10,  472,   10,  370,   10,   10,   26,  476,   11,  444,
  475,   55,   16,   55,   16,   16,  245,  324,  464,   19,
  259,   32,  260,  232,  365,  337,  410,  166,  464,  379,
   16,  383,  254,  255,  390,  166,  208,  444,   31,  331,
  302,  209,  366,   23,   16,  303,  297,  222,   26,  304,
  305,   87,   88,   89,   90,  140,  238,  330,  147,   20,
  239,  316,    1,    2,   96,  218,  141,   24,  236,  226,
  227,  237,   28,   27,  142,  340,  104,  105,  106,  143,
  144,   15,   37,   38,   15,   15,   15,   66,   67,   68,
  317,  349,  318,  350,  351,  352,   34,   42,  161,  162,
  145,   63,   64,   35,  386,  387,   36,   87,   88,   89,
   90,   91,   92,   93,   94,   95,   19,   19,   81,   46,
   96,   97,   98,   99,  100,  169,  101,  102,  103,  174,
  175,   47,  104,  105,  106,   48,   71,   51,   62,   72,
  250,   73,   74,  393,   82,  303,  266,   75,   83,  304,
  305,   84,   94,   85,   86,  128,  131,  138,  191,  160,
  457,  458,  459,  166,   16,  170,  176,  177,  178,  179,
  180,  415,  327,  181,  182,  183,  192,  184,  185,  186,
  187,  193,  188,  189,  190,  194,  195,  196,  197,  198,
  202,   96,  199,  200,  201,  203,  205,  206,  207,  217,
  242,  221,  251,  247,  268,  252,  253,  267,  269,  270,
  271,  272,  273,   79,   79,   33,   33,  138,  138,  274,
  138,  138,  138,  138,  138,  138,  138,  138,  138,  138,
  138,   18,  334,  367,  338,  138,  138,  138,  138,  138,
  138,  138,  138,  138,   33,  138,  138,  138,  138,  138,
  138,  138,  320,  138,  124,  124,  275,  124,  124,  124,
  124,  124,  124,  124,  124,  124,  124,  124,   26,   26,
  138,  138,  124,  124,  124,  124,  124,  124,  124,  124,
  124,  323,  124,  124,  124,  124,  124,  124,  124,   11,
  124,  211,  376,  378,  212,  382,  221,   26,   15,  211,
   31,   31,  212,  276,   26,  278,  282,  124,  124,  414,
  283,  211,  284,  400,  212,  285,   19,  286,  287,  288,
  147,  147,  289,  147,  147,  147,  147,  147,  147,  147,
  147,  147,  147,  147,   28,   28,  292,  290,  147,  147,
  147,  147,  147,  147,  147,  147,  147,  427,  147,  147,
  147,  147,  147,  147,  147,   10,  147,   19,  293,   42,
   42,  294,  295,   28,   42,   42,   42,   42,   42,  296,
   28,  299,  300,  147,  147,   38,  301,   42,  308,   42,
   81,   81,   42,   81,   81,  298,  461,   42,   42,   42,
   42,   42,   42,   42,   39,   42,  310,  311,  468,  312,
  309,  332,  313,  314,  315,  326,  331,  336,  341,  343,
  342,  344,   42,   42,   94,   94,  346,   94,   94,   94,
   94,   94,   94,   94,   94,   94,   94,   94,  345,   94,
   94,   94,   94,   94,   94,   94,   94,   94,   94,   94,
   94,  347,  348,   94,   94,   94,   94,  353,  354,  355,
   94,  356,  357,   96,   96,   98,   96,   96,   96,   96,
   96,   96,   96,   96,   96,   96,   96,   94,   96,   96,
   96,   96,   96,   96,   96,   96,   96,   96,   96,   96,
  358,  359,   96,   96,   96,   96,  360,  364,  101,   96,
  366,  361,   18,   18,   18,   18,   18,   18,  368,   99,
  363,  372,  373,  374,  377,  381,   96,  385,   18,   18,
  392,  394,  395,  397,  398,  399,   18,  401,  403,  405,
  408,  193,   18,  406,  412,  407,  413,  418,  419,   18,
  420,  421,  422,  423,  425,  428,  426,  449,  430,  431,
  434,  435,  436,  438,  440,  447,   18,  450,  451,  452,
   11,   11,   11,  453,   11,   11,  166,  454,  455,   15,
   15,   15,   15,  456,  462,    1,   11,   11,  469,  470,
    3,  218,  441,  404,   11,   15,   15,  437,   19,   19,
   11,   19,   19,   15,  429,   14,   45,   11,   22,   15,
  194,  460,  261,   19,   19,  256,   15,  328,   15,   49,
   79,   19,  424,   81,   11,  107,  108,   19,  109,  110,
  173,  465,  111,   15,   19,  112,   10,   10,   10,   19,
   19,  291,  113,  114,    0,    0,    0,    0,    0,    0,
    0,   19,   10,   10,   19,   19,    0,   38,   38,    0,
   10,    0,   19,    0,    0,    0,   10,    0,   19,    0,
    0,    0,    0,   10,    0,   19,   39,   39,    0,    0,
   38,    0,    0,    0,    0,    0,   38,    0,    0,    0,
   10,    0,   19,   38,    0,    0,    0,    0,    0,   39,
    0,    0,    0,    0,    0,   39,    0,    0,    0,    0,
   38,    0,   39,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   39,
    0,    0,    0,    0,    0,    0,    0,   98,   98,    0,
   98,   98,   98,   98,   98,   98,   98,   98,   98,   98,
   98,    0,   98,   98,   98,   98,   98,   98,   98,   98,
    0,   98,   98,   98,    0,    0,    0,   98,   98,   98,
  101,  101,    0,  101,  101,  101,  101,  101,  101,  101,
  101,  101,  101,  101,    0,    0,    0,    0,  101,  101,
  101,  101,  101,    0,  101,  101,  101,    0,    0,    0,
  101,  101,  101,  193,  193,    0,  193,  193,  193,  193,
  193,  193,  193,  193,  193,  193,  193,    0,    0,    0,
    0,  193,  193,  193,  193,  193,    0,  193,  193,  193,
    0,    0,    0,  193,  193,  193,    0,    0,  166,  166,
    0,  166,  166,  166,  166,  166,  166,  166,  166,  166,
  166,  166,    0,    0,    0,    0,  166,  166,  166,  166,
  166,    0,  166,  166,  166,    0,    0,    0,  166,  166,
  166,    0,  194,  194,    0,  194,  194,  194,  194,  194,
  194,  194,  194,  194,  194,  194,    0,    0,    0,    0,
  194,  194,  194,  194,  194,    0,  194,  194,  194,    0,
    0,    0,  194,  194,  194,
};
short yycheck[] = {                                      10,
   11,    0,   40,    0,   40,    0,  177,  178,   86,  180,
  181,   84,  194,   74,   85,  185,  186,  187,  189,  190,
  288,  259,  258,  257,  257,  277,  278,  258,  199,  200,
  258,  285,  258,  285,  297,  257,  456,  301,   40,  259,
  304,   22,    0,  304,  304,  304,  304,  317,   29,  469,
  261,  467,   58,  313,  265,  266,    0,  473,   58,  323,
  323,  315,  323,  315,  323,  323,  304,   40,  304,   58,
  304,  304,  306,  304,  342,  304,  304,   40,  304,   40,
  323,   40,  304,  305,  304,   40,  299,  323,    0,   41,
  272,  304,   44,   44,  323,  304,  267,   40,   58,  308,
  309,  267,  268,  269,  270,  271,  300,  289,    0,  264,
  304,  282,  262,  263,  280,  179,  282,  318,  191,  183,
  184,  192,    0,  261,  290,  296,  292,  293,  294,  295,
  296,  261,  265,  266,  264,  265,  266,  277,  278,  279,
  302,  311,  304,  313,  314,  315,   44,    0,  311,  312,
  316,  310,  311,  257,  303,  304,   58,  267,  268,  269,
  270,  271,  272,  273,  274,  275,  265,  266,    0,   44,
  280,  281,  282,  283,  284,  253,  286,  287,  288,   89,
   90,  323,  292,  293,  294,   58,   58,  324,  257,   58,
  201,   58,   58,  364,   44,  304,  207,  298,   44,  308,
  309,   58,    0,   58,   58,  257,  257,  291,   44,  259,
  319,  320,  321,   40,  323,  260,   58,   58,   58,   58,
   58,  392,  258,   58,   58,   58,   44,   58,   58,   58,
   58,   44,   58,   58,   58,   44,   58,   58,   58,   58,
   44,    0,   58,   58,   58,  258,   58,   58,   58,   40,
   40,  289,  259,  258,   91,   44,   44,   44,   44,   44,
  258,   44,   44,  262,  263,  262,  263,  262,  263,   44,
  265,  266,  267,  268,  269,  270,  271,  272,  273,  274,
  275,    0,  293,  344,  295,  280,  281,  282,  283,  284,
  285,  286,  287,  288,  291,  290,  291,  292,  293,  294,
  295,  296,  304,  298,  262,  263,   40,  265,  266,  267,
  268,  269,  270,  271,  272,  273,  274,  275,  262,  263,
  315,  316,  280,  281,  282,  283,  284,  285,  286,  287,
  288,  304,  290,  291,  292,  293,  294,  295,  296,    0,
  298,  304,  353,  304,  307,  304,  289,  291,    0,  304,
  262,  263,  307,  258,  298,   44,   44,  315,  316,  322,
   44,  304,   44,  374,  307,   44,    0,  258,   44,   44,
  262,  263,   44,  265,  266,  267,  268,  269,  270,  271,
  272,  273,  274,  275,  262,  263,   91,  258,  280,  281,
  282,  283,  284,  285,  286,  287,  288,  408,  290,  291,
  292,  293,  294,  295,  296,    0,  298,    0,   44,  262,
  263,   91,   44,  291,  267,  268,  269,  270,  271,   44,
  298,  303,  303,  315,  316,    0,   44,  280,  314,  282,
  262,  263,  285,  265,  266,  258,  447,  290,  291,  292,
  293,  294,  295,  296,    0,  298,   44,   44,  459,  314,
  258,  258,   44,   44,   44,   44,   41,  258,   93,  258,
   44,   44,  315,  316,  262,  263,   44,  265,  266,  267,
  268,  269,  270,  271,  272,  273,  274,  275,   91,  277,
  278,  279,  280,  281,  282,  283,  284,  285,  286,  287,
  288,   44,  258,  291,  292,  293,  294,   44,  258,   44,
  298,  258,   44,  262,  263,    0,  265,  266,  267,  268,
  269,  270,  271,  272,  273,  274,  275,  315,  277,  278,
  279,  280,  281,  282,  283,  284,  285,  286,  287,  288,
  303,   44,  291,  292,  293,  294,   44,   44,    0,  298,
   44,   93,  261,  262,  263,  264,  265,  266,  258,   44,
   93,  258,   44,   44,   44,   44,  315,   44,  277,  278,
   44,  258,   44,   93,   44,  314,  285,   44,  258,   44,
   44,    0,  291,  258,   44,  258,   44,   44,  276,  298,
  258,  314,   41,   44,   41,   41,   44,   41,  259,   44,
  258,   44,   44,   44,  258,   44,  315,  259,  258,   41,
  261,  262,  263,  259,  265,  266,    0,   41,   44,  261,
  262,  263,  264,   44,   41,    0,  277,  278,   44,   44,
    0,   58,  427,  379,  285,  277,  278,  423,  262,  263,
  291,  265,  266,  285,  412,    5,   34,  298,   14,  291,
    0,  447,  206,  277,  278,  205,  298,  287,    7,   40,
   59,  285,  405,   61,  315,   69,   69,  291,   69,   69,
   88,  455,   69,  315,  298,   69,  261,  262,  263,  262,
  263,  253,   69,   69,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  315,  277,  278,  277,  278,   -1,  262,  263,   -1,
  285,   -1,  285,   -1,   -1,   -1,  291,   -1,  291,   -1,
   -1,   -1,   -1,  298,   -1,  298,  262,  263,   -1,   -1,
  285,   -1,   -1,   -1,   -1,   -1,  291,   -1,   -1,   -1,
  315,   -1,  315,  298,   -1,   -1,   -1,   -1,   -1,  285,
   -1,   -1,   -1,   -1,   -1,  291,   -1,   -1,   -1,   -1,
  315,   -1,  298,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  315,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  262,  263,   -1,
  265,  266,  267,  268,  269,  270,  271,  272,  273,  274,
  275,   -1,  277,  278,  279,  280,  281,  282,  283,  284,
   -1,  286,  287,  288,   -1,   -1,   -1,  292,  293,  294,
  262,  263,   -1,  265,  266,  267,  268,  269,  270,  271,
  272,  273,  274,  275,   -1,   -1,   -1,   -1,  280,  281,
  282,  283,  284,   -1,  286,  287,  288,   -1,   -1,   -1,
  292,  293,  294,  262,  263,   -1,  265,  266,  267,  268,
  269,  270,  271,  272,  273,  274,  275,   -1,   -1,   -1,
   -1,  280,  281,  282,  283,  284,   -1,  286,  287,  288,
   -1,   -1,   -1,  292,  293,  294,   -1,   -1,  262,  263,
   -1,  265,  266,  267,  268,  269,  270,  271,  272,  273,
  274,  275,   -1,   -1,   -1,   -1,  280,  281,  282,  283,
  284,   -1,  286,  287,  288,   -1,   -1,   -1,  292,  293,
  294,   -1,  262,  263,   -1,  265,  266,  267,  268,  269,
  270,  271,  272,  273,  274,  275,   -1,   -1,   -1,   -1,
  280,  281,  282,  283,  284,   -1,  286,  287,  288,   -1,
   -1,   -1,  292,  293,  294,
};
#define YYFINAL 3
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 324
#if YYDEBUG
char *yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,"'('","')'",0,0,"','",0,0,0,0,0,0,0,0,0,0,0,0,0,"':'",0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'['",0,"']'",0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"CHAR",
"INTEGER","BOOLEAN","PERCENT","MESSAGE_ID","MAZE_ID","LEVEL_ID","LEV_INIT_ID",
"GEOMETRY_ID","NOMAP_ID","OBJECT_ID","COBJECT_ID","MONSTER_ID","TRAP_ID",
"DOOR_ID","DRAWBRIDGE_ID","MAZEWALK_ID","WALLIFY_ID","REGION_ID","FILLING",
"RANDOM_OBJECTS_ID","RANDOM_MONSTERS_ID","RANDOM_PLACES_ID","ALTAR_ID",
"LADDER_ID","STAIR_ID","NON_DIGGABLE_ID","NON_PASSWALL_ID","ROOM_ID",
"PORTAL_ID","TELEPRT_ID","BRANCH_ID","LEV","CHANCE_ID","CORRIDOR_ID","GOLD_ID",
"ENGRAVING_ID","FOUNTAIN_ID","POOL_ID","SINK_ID","NONE","RAND_CORRIDOR_ID",
"DOOR_STATE","LIGHT_STATE","CURSE_TYPE","ENGRAVING_TYPE","DIRECTION",
"RANDOM_TYPE","O_REGISTER","M_REGISTER","P_REGISTER","A_REGISTER","ALIGNMENT",
"LEFT_OR_RIGHT","CENTER","TOP_OR_BOT","ALTAR_TYPE","UP_OR_DOWN","SUBROOM_ID",
"NAME_ID","FLAGS_ID","FLAG_TYPE","MON_ATTITUDE","MON_ALERTNESS",
"MON_APPEARANCE","CONTAINED","STRING","MAP_ID",
};
char *yyrule[] = {
"$accept : file",
"file :",
"file : levels",
"levels : level",
"levels : level levels",
"level : maze_level",
"level : room_level",
"maze_level : maze_def flags lev_init messages regions",
"room_level : level_def flags lev_init messages rreg_init rooms corridors_def",
"level_def : LEVEL_ID ':' string",
"lev_init :",
"lev_init : LEV_INIT_ID ':' CHAR ',' CHAR ',' BOOLEAN ',' BOOLEAN ',' light_state ',' walled",
"lev_init : LEV_INIT_ID ':' CHAR ',' CHAR ',' BOOLEAN ',' BOOLEAN ',' light_state ',' walled ',' BOOLEAN",
"walled : BOOLEAN",
"walled : RANDOM_TYPE",
"flags :",
"flags : FLAGS_ID ':' flag_list",
"flag_list : FLAG_TYPE ',' flag_list",
"flag_list : FLAG_TYPE",
"messages :",
"messages : message messages",
"message : MESSAGE_ID ':' STRING",
"rreg_init :",
"rreg_init : rreg_init init_rreg",
"init_rreg : RANDOM_OBJECTS_ID ':' object_list",
"init_rreg : RANDOM_MONSTERS_ID ':' monster_list",
"rooms :",
"rooms : roomlist",
"roomlist : aroom",
"roomlist : aroom roomlist",
"corridors_def : random_corridors",
"corridors_def : corridors",
"random_corridors : RAND_CORRIDOR_ID",
"corridors :",
"corridors : corridors corridor",
"corridor : CORRIDOR_ID ':' corr_spec ',' corr_spec",
"corridor : CORRIDOR_ID ':' corr_spec ',' INTEGER",
"corr_spec : '(' INTEGER ',' DIRECTION ',' door_pos ')'",
"aroom : room_def room_details",
"aroom : subroom_def room_details",
"subroom_def : SUBROOM_ID ':' room_type ',' light_state ',' subroom_pos ',' room_size ',' string roomfill",
"room_def : ROOM_ID ':' room_type ',' light_state ',' room_pos ',' room_align ',' room_size roomfill",
"roomfill :",
"roomfill : ',' BOOLEAN",
"room_pos : '(' INTEGER ',' INTEGER ')'",
"room_pos : RANDOM_TYPE",
"subroom_pos : '(' INTEGER ',' INTEGER ')'",
"subroom_pos : RANDOM_TYPE",
"room_align : '(' h_justif ',' v_justif ')'",
"room_align : RANDOM_TYPE",
"room_size : '(' INTEGER ',' INTEGER ')'",
"room_size : RANDOM_TYPE",
"room_details :",
"room_details : room_details room_detail",
"room_detail : room_name",
"room_detail : room_chance",
"room_detail : room_door",
"room_detail : monster_detail",
"room_detail : object_detail",
"room_detail : trap_detail",
"room_detail : altar_detail",
"room_detail : fountain_detail",
"room_detail : sink_detail",
"room_detail : pool_detail",
"room_detail : gold_detail",
"room_detail : engraving_detail",
"room_detail : stair_detail",
"room_name : NAME_ID ':' string",
"room_chance : CHANCE_ID ':' INTEGER",
"room_door : DOOR_ID ':' secret ',' door_state ',' door_wall ',' door_pos",
"secret : BOOLEAN",
"secret : RANDOM_TYPE",
"door_wall : DIRECTION",
"door_wall : RANDOM_TYPE",
"door_pos : INTEGER",
"door_pos : RANDOM_TYPE",
"maze_def : MAZE_ID ':' string ',' filling",
"filling : CHAR",
"filling : RANDOM_TYPE",
"regions : aregion",
"regions : aregion regions",
"aregion : map_definition reg_init map_details",
"map_definition : NOMAP_ID",
"map_definition : map_geometry MAP_ID",
"map_geometry : GEOMETRY_ID ':' h_justif ',' v_justif",
"h_justif : LEFT_OR_RIGHT",
"h_justif : CENTER",
"v_justif : TOP_OR_BOT",
"v_justif : CENTER",
"reg_init :",
"reg_init : reg_init init_reg",
"init_reg : RANDOM_OBJECTS_ID ':' object_list",
"init_reg : RANDOM_PLACES_ID ':' place_list",
"init_reg : RANDOM_MONSTERS_ID ':' monster_list",
"object_list : object",
"object_list : object ',' object_list",
"monster_list : monster",
"monster_list : monster ',' monster_list",
"place_list : place",
"$$1 :",
"place_list : place $$1 ',' place_list",
"map_details :",
"map_details : map_details map_detail",
"map_detail : monster_detail",
"map_detail : object_detail",
"map_detail : door_detail",
"map_detail : trap_detail",
"map_detail : drawbridge_detail",
"map_detail : region_detail",
"map_detail : stair_region",
"map_detail : portal_region",
"map_detail : teleprt_region",
"map_detail : branch_region",
"map_detail : altar_detail",
"map_detail : fountain_detail",
"map_detail : mazewalk_detail",
"map_detail : wallify_detail",
"map_detail : ladder_detail",
"map_detail : stair_detail",
"map_detail : gold_detail",
"map_detail : engraving_detail",
"map_detail : diggable_detail",
"map_detail : passwall_detail",
"$$2 :",
"monster_detail : MONSTER_ID chance ':' monster_c ',' m_name ',' coordinate $$2 monster_infos",
"monster_infos :",
"monster_infos : monster_infos monster_info",
"monster_info : ',' string",
"monster_info : ',' MON_ATTITUDE",
"monster_info : ',' MON_ALERTNESS",
"monster_info : ',' alignment",
"monster_info : ',' MON_APPEARANCE string",
"object_detail : OBJECT_ID object_desc",
"object_detail : COBJECT_ID object_desc",
"$$3 :",
"object_desc : chance ':' object_c ',' o_name $$3 ',' object_where object_infos",
"object_where : coordinate",
"object_where : CONTAINED",
"object_infos :",
"object_infos : ',' curse_state ',' monster_id ',' enchantment optional_name",
"object_infos : ',' curse_state ',' enchantment optional_name",
"object_infos : ',' monster_id ',' enchantment optional_name",
"curse_state : RANDOM_TYPE",
"curse_state : CURSE_TYPE",
"monster_id : STRING",
"enchantment : RANDOM_TYPE",
"enchantment : INTEGER",
"optional_name :",
"optional_name : ',' NONE",
"optional_name : ',' STRING",
"door_detail : DOOR_ID ':' door_state ',' coordinate",
"trap_detail : TRAP_ID chance ':' trap_name ',' coordinate",
"drawbridge_detail : DRAWBRIDGE_ID ':' coordinate ',' DIRECTION ',' door_state",
"mazewalk_detail : MAZEWALK_ID ':' coordinate ',' DIRECTION",
"wallify_detail : WALLIFY_ID",
"ladder_detail : LADDER_ID ':' coordinate ',' UP_OR_DOWN",
"stair_detail : STAIR_ID ':' coordinate ',' UP_OR_DOWN",
"$$4 :",
"stair_region : STAIR_ID ':' lev_region $$4 ',' lev_region ',' UP_OR_DOWN",
"$$5 :",
"portal_region : PORTAL_ID ':' lev_region $$5 ',' lev_region ',' string",
"$$6 :",
"$$7 :",
"teleprt_region : TELEPRT_ID ':' lev_region $$6 ',' lev_region $$7 teleprt_detail",
"$$8 :",
"branch_region : BRANCH_ID ':' lev_region $$8 ',' lev_region",
"teleprt_detail :",
"teleprt_detail : ',' UP_OR_DOWN",
"lev_region : region",
"lev_region : LEV '(' INTEGER ',' INTEGER ',' INTEGER ',' INTEGER ')'",
"fountain_detail : FOUNTAIN_ID ':' coordinate",
"sink_detail : SINK_ID ':' coordinate",
"pool_detail : POOL_ID ':' coordinate",
"diggable_detail : NON_DIGGABLE_ID ':' region",
"passwall_detail : NON_PASSWALL_ID ':' region",
"region_detail : REGION_ID ':' region ',' light_state ',' room_type prefilled",
"altar_detail : ALTAR_ID ':' coordinate ',' alignment ',' altar_type",
"gold_detail : GOLD_ID ':' amount ',' coordinate",
"engraving_detail : ENGRAVING_ID ':' coordinate ',' engraving_type ',' string",
"monster_c : monster",
"monster_c : RANDOM_TYPE",
"monster_c : m_register",
"object_c : object",
"object_c : RANDOM_TYPE",
"object_c : o_register",
"m_name : string",
"m_name : RANDOM_TYPE",
"o_name : string",
"o_name : RANDOM_TYPE",
"trap_name : string",
"trap_name : RANDOM_TYPE",
"room_type : string",
"room_type : RANDOM_TYPE",
"prefilled :",
"prefilled : ',' FILLING",
"prefilled : ',' FILLING ',' BOOLEAN",
"coordinate : coord",
"coordinate : p_register",
"coordinate : RANDOM_TYPE",
"door_state : DOOR_STATE",
"door_state : RANDOM_TYPE",
"light_state : LIGHT_STATE",
"light_state : RANDOM_TYPE",
"alignment : ALIGNMENT",
"alignment : a_register",
"alignment : RANDOM_TYPE",
"altar_type : ALTAR_TYPE",
"altar_type : RANDOM_TYPE",
"p_register : P_REGISTER '[' INTEGER ']'",
"o_register : O_REGISTER '[' INTEGER ']'",
"m_register : M_REGISTER '[' INTEGER ']'",
"a_register : A_REGISTER '[' INTEGER ']'",
"place : coord",
"monster : CHAR",
"object : CHAR",
"string : STRING",
"amount : INTEGER",
"amount : RANDOM_TYPE",
"chance :",
"chance : PERCENT",
"engraving_type : ENGRAVING_TYPE",
"engraving_type : RANDOM_TYPE",
"coord : '(' INTEGER ',' INTEGER ')'",
"region : '(' INTEGER ',' INTEGER ',' INTEGER ',' INTEGER ')'",
};
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 500
#define YYMAXDEPTH 500
#endif
#endif
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short yyss[YYSTACKSIZE];
YYSTYPE yyvs[YYSTACKSIZE];
#define yystacksize YYSTACKSIZE

/*lev_comp.y*/
#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab
int
yyparse()
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register char *yys;
    extern char *getenv();

    if ((yys = getenv("YYDEBUG")) != 0)
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) != 0 && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yyss + yystacksize - 1)
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) != 0 && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
#ifdef lint
    goto yynewerror;
#endif
yynewerror:
    yyerror("syntax error");
#ifdef lint
    goto yyerrlab;
#endif
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) != 0 && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yyss + yystacksize - 1)
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 7:
{
			unsigned i;

			if (fatal_error > 0) {
				(void) fprintf(stderr,
				"%s : %d errors detected. No output created!\n",
					fname, fatal_error);
			} else {
				maze.flags = yyvsp[-3].i;
				(void) memcpy((genericptr_t)&(maze.init_lev),
						(genericptr_t)&(init_lev),
						sizeof(lev_init));
				maze.numpart = npart;
				maze.parts = NewTab(mazepart, npart);
				for(i=0;i<npart;i++)
				    maze.parts[i] = tmppart[i];
				if (!write_level_file(yyvsp[-4].map, (splev *)0, &maze)) {
					yyerror("Can't write output file!!");
					exit(EXIT_FAILURE);
				}
				npart = 0;
			}
			Free(yyvsp[-4].map);
		  }
break;
case 8:
{
			unsigned i;

			if (fatal_error > 0) {
			    (void) fprintf(stderr,
			      "%s : %d errors detected. No output created!\n",
					fname, fatal_error);
			} else {
				special_lev.flags = (long) yyvsp[-5].i;
				(void) memcpy(
					(genericptr_t)&(special_lev.init_lev),
					(genericptr_t)&(init_lev),
					sizeof(lev_init));
				special_lev.nroom = nrooms;
				special_lev.rooms = NewTab(room, nrooms);
				for(i=0; i<nrooms; i++)
				    special_lev.rooms[i] = tmproom[i];
				special_lev.ncorr = ncorridor;
				special_lev.corrs = NewTab(corridor, ncorridor);
				for(i=0; i<ncorridor; i++)
				    special_lev.corrs[i] = tmpcor[i];
				if (check_subrooms()) {
				    if (!write_level_file(yyvsp[-6].map, &special_lev,
							  (specialmaze *)0)) {
					yyerror("Can't write output file!!");
					exit(EXIT_FAILURE);
				    }
				}
				free_rooms(&special_lev);
				nrooms = 0;
				ncorridor = 0;
			}
			Free(yyvsp[-6].map);
		  }
break;
case 9:
{
			if (index(yyvsp[0].map, '.'))
			    yyerror("Invalid dot ('.') in level name.");
			if ((int) strlen(yyvsp[0].map) > 8)
			    yyerror("Level names limited to 8 characters.");
			yyval.map = yyvsp[0].map;
			special_lev.nrmonst = special_lev.nrobjects = 0;
			n_mlist = n_olist = 0;
		  }
break;
case 10:
{
			/* in case we're processing multiple files,
			   explicitly clear any stale settings */
			(void) memset((genericptr_t) &init_lev, 0,
					sizeof init_lev);
			init_lev.init_present = FALSE;
			yyval.i = 0;
		  }
break;
case 11:
{
			init_lev.init_present = TRUE;
			init_lev.fg = what_map_char((char) yyvsp[-10].i);
			if (init_lev.fg == INVALID_TYPE)
			    yyerror("Invalid foreground type.");
			init_lev.bg = what_map_char((char) yyvsp[-8].i);
			if (init_lev.bg == INVALID_TYPE)
			    yyerror("Invalid background type.");
			init_lev.smoothed = yyvsp[-6].i;
			init_lev.joined = yyvsp[-4].i;
			if (init_lev.joined &&
			    init_lev.fg != CORR && init_lev.fg != ROOM)
			    yyerror("Invalid foreground type for joined map.");
			init_lev.lit = yyvsp[-2].i;
			init_lev.walled = yyvsp[0].i;
			init_lev.icedpools = FALSE;
			yyval.i = 1;
		  }
break;
case 12:
{
			init_lev.init_present = TRUE;
			init_lev.fg = what_map_char((char) yyvsp[-12].i);
			if (init_lev.fg == INVALID_TYPE)
			    yyerror("Invalid foreground type.");
			init_lev.bg = what_map_char((char) yyvsp[-10].i);
			if (init_lev.bg == INVALID_TYPE)
			    yyerror("Invalid background type.");
			init_lev.smoothed = yyvsp[-8].i;
			init_lev.joined = yyvsp[-6].i;
			if (init_lev.joined &&
			    init_lev.fg != CORR && init_lev.fg != ROOM)
			    yyerror("Invalid foreground type for joined map.");
			init_lev.lit = yyvsp[-4].i;
			init_lev.walled = yyvsp[-2].i;
			init_lev.icedpools = yyvsp[0].i;
			yyval.i = 1;
		  }
break;
case 15:
{
			yyval.i = 0;
		  }
break;
case 16:
{
			yyval.i = lev_flags;
			lev_flags = 0;	/* clear for next user */
		  }
break;
case 17:
{
			lev_flags |= yyvsp[-2].i;
		  }
break;
case 18:
{
			lev_flags |= yyvsp[0].i;
		  }
break;
case 21:
{
			int i, j;

			i = (int) strlen(yyvsp[0].map) + 1;
			j = (int) strlen(tmpmessage);
			if (i + j > 255) {
			   yyerror("Message string too long (>256 characters)");
			} else {
			    if (j) tmpmessage[j++] = '\n';
			    (void) strncpy(tmpmessage+j, yyvsp[0].map, i - 1);
			    tmpmessage[j + i - 1] = 0;
			}
			Free(yyvsp[0].map);
		  }
break;
case 24:
{
			if(special_lev.nrobjects) {
			    yyerror("Object registers already initialized!");
			} else {
			    special_lev.nrobjects = n_olist;
			    special_lev.robjects = (char *) alloc(n_olist);
			    (void) memcpy((genericptr_t)special_lev.robjects,
					  (genericptr_t)olist, n_olist);
			}
		  }
break;
case 25:
{
			if(special_lev.nrmonst) {
			    yyerror("Monster registers already initialized!");
			} else {
			    special_lev.nrmonst = n_mlist;
			    special_lev.rmonst = (char *) alloc(n_mlist);
			    (void) memcpy((genericptr_t)special_lev.rmonst,
					  (genericptr_t)mlist, n_mlist);
			  }
		  }
break;
case 26:
{
			tmproom[nrooms] = New(room);
			tmproom[nrooms]->name = (char *) 0;
			tmproom[nrooms]->parent = (char *) 0;
			tmproom[nrooms]->rtype = 0;
			tmproom[nrooms]->rlit = 0;
			tmproom[nrooms]->xalign = ERR;
			tmproom[nrooms]->yalign = ERR;
			tmproom[nrooms]->x = 0;
			tmproom[nrooms]->y = 0;
			tmproom[nrooms]->w = 2;
			tmproom[nrooms]->h = 2;
			in_room = 1;
		  }
break;
case 32:
{
			tmpcor[0] = New(corridor);
			tmpcor[0]->src.room = -1;
			ncorridor = 1;
		  }
break;
case 35:
{
			tmpcor[ncorridor] = New(corridor);
			tmpcor[ncorridor]->src.room = yyvsp[-2].corpos.room;
			tmpcor[ncorridor]->src.wall = yyvsp[-2].corpos.wall;
			tmpcor[ncorridor]->src.door = yyvsp[-2].corpos.door;
			tmpcor[ncorridor]->dest.room = yyvsp[0].corpos.room;
			tmpcor[ncorridor]->dest.wall = yyvsp[0].corpos.wall;
			tmpcor[ncorridor]->dest.door = yyvsp[0].corpos.door;
			ncorridor++;
			if (ncorridor >= MAX_OF_TYPE) {
				yyerror("Too many corridors in level!");
				ncorridor--;
			}
		  }
break;
case 36:
{
			tmpcor[ncorridor] = New(corridor);
			tmpcor[ncorridor]->src.room = yyvsp[-2].corpos.room;
			tmpcor[ncorridor]->src.wall = yyvsp[-2].corpos.wall;
			tmpcor[ncorridor]->src.door = yyvsp[-2].corpos.door;
			tmpcor[ncorridor]->dest.room = -1;
			tmpcor[ncorridor]->dest.wall = yyvsp[0].i;
			ncorridor++;
			if (ncorridor >= MAX_OF_TYPE) {
				yyerror("Too many corridors in level!");
				ncorridor--;
			}
		  }
break;
case 37:
{
			if ((unsigned) yyvsp[-5].i >= nrooms)
			    yyerror("Wrong room number!");
			yyval.corpos.room = yyvsp[-5].i;
			yyval.corpos.wall = yyvsp[-3].i;
			yyval.corpos.door = yyvsp[-1].i;
		  }
break;
case 38:
{
			store_room();
		  }
break;
case 39:
{
			store_room();
		  }
break;
case 40:
{
			tmproom[nrooms] = New(room);
			tmproom[nrooms]->parent = yyvsp[-1].map;
			tmproom[nrooms]->name = (char *) 0;
			tmproom[nrooms]->rtype = yyvsp[-9].i;
			tmproom[nrooms]->rlit = yyvsp[-7].i;
			tmproom[nrooms]->filled = yyvsp[0].i;
			tmproom[nrooms]->xalign = ERR;
			tmproom[nrooms]->yalign = ERR;
			tmproom[nrooms]->x = current_coord.x;
			tmproom[nrooms]->y = current_coord.y;
			tmproom[nrooms]->w = current_size.width;
			tmproom[nrooms]->h = current_size.height;
			in_room = 1;
		  }
break;
case 41:
{
			tmproom[nrooms] = New(room);
			tmproom[nrooms]->name = (char *) 0;
			tmproom[nrooms]->parent = (char *) 0;
			tmproom[nrooms]->rtype = yyvsp[-9].i;
			tmproom[nrooms]->rlit = yyvsp[-7].i;
			tmproom[nrooms]->filled = yyvsp[0].i;
			tmproom[nrooms]->xalign = current_align.x;
			tmproom[nrooms]->yalign = current_align.y;
			tmproom[nrooms]->x = current_coord.x;
			tmproom[nrooms]->y = current_coord.y;
			tmproom[nrooms]->w = current_size.width;
			tmproom[nrooms]->h = current_size.height;
			in_room = 1;
		  }
break;
case 42:
{
			yyval.i = 1;
		  }
break;
case 43:
{
			yyval.i = yyvsp[0].i;
		  }
break;
case 44:
{
			if ( yyvsp[-3].i < 1 || yyvsp[-3].i > 5 ||
			    yyvsp[-1].i < 1 || yyvsp[-1].i > 5 ) {
			    yyerror("Room position should be between 1 & 5!");
			} else {
			    current_coord.x = yyvsp[-3].i;
			    current_coord.y = yyvsp[-1].i;
			}
		  }
break;
case 45:
{
			current_coord.x = current_coord.y = ERR;
		  }
break;
case 46:
{
			if ( yyvsp[-3].i < 0 || yyvsp[-1].i < 0) {
			    yyerror("Invalid subroom position !");
			} else {
			    current_coord.x = yyvsp[-3].i;
			    current_coord.y = yyvsp[-1].i;
			}
		  }
break;
case 47:
{
			current_coord.x = current_coord.y = ERR;
		  }
break;
case 48:
{
			current_align.x = yyvsp[-3].i;
			current_align.y = yyvsp[-1].i;
		  }
break;
case 49:
{
			current_align.x = current_align.y = ERR;
		  }
break;
case 50:
{
			current_size.width = yyvsp[-3].i;
			current_size.height = yyvsp[-1].i;
		  }
break;
case 51:
{
			current_size.height = current_size.width = ERR;
		  }
break;
case 67:
{
			if (tmproom[nrooms]->name)
			    yyerror("This room already has a name!");
			else
			    tmproom[nrooms]->name = yyvsp[0].map;
		  }
break;
case 68:
{
			if (tmproom[nrooms]->chance)
			    yyerror("This room already assigned a chance!");
			else if (tmproom[nrooms]->rtype == OROOM)
			    yyerror("Only typed rooms can have a chance!");
			else if (yyvsp[0].i < 1 || yyvsp[0].i > 99)
			    yyerror("The chance is supposed to be percentile.");
			else
			    tmproom[nrooms]->chance = yyvsp[0].i;
		   }
break;
case 69:
{
			/* ERR means random here */
			if (yyvsp[-2].i == ERR && yyvsp[0].i != ERR) {
		     yyerror("If the door wall is random, so must be its pos!");
			} else {
			    tmprdoor[ndoor] = New(room_door);
			    tmprdoor[ndoor]->secret = yyvsp[-6].i;
			    tmprdoor[ndoor]->mask = yyvsp[-4].i;
			    tmprdoor[ndoor]->wall = yyvsp[-2].i;
			    tmprdoor[ndoor]->pos = yyvsp[0].i;
			    ndoor++;
			    if (ndoor >= MAX_OF_TYPE) {
				    yyerror("Too many doors in room!");
				    ndoor--;
			    }
			}
		  }
break;
case 76:
{
			maze.filling = (schar) yyvsp[0].i;
			if (index(yyvsp[-2].map, '.'))
			    yyerror("Invalid dot ('.') in level name.");
			if ((int) strlen(yyvsp[-2].map) > 8)
			    yyerror("Level names limited to 8 characters.");
			yyval.map = yyvsp[-2].map;
			in_room = 0;
			n_plist = n_mlist = n_olist = 0;
		  }
break;
case 77:
{
			yyval.i = get_floor_type((char)yyvsp[0].i);
		  }
break;
case 78:
{
			yyval.i = -1;
		  }
break;
case 81:
{
			store_part();
		  }
break;
case 82:
{
			tmppart[npart] = New(mazepart);
			tmppart[npart]->halign = 1;
			tmppart[npart]->valign = 1;
			tmppart[npart]->nrobjects = 0;
			tmppart[npart]->nloc = 0;
			tmppart[npart]->nrmonst = 0;
			tmppart[npart]->xsize = 1;
			tmppart[npart]->ysize = 1;
			tmppart[npart]->map = (char **) alloc(sizeof(char *));
			tmppart[npart]->map[0] = (char *) alloc(1);
			tmppart[npart]->map[0][0] = STONE;
			max_x_map = COLNO-1;
			max_y_map = ROWNO;
		  }
break;
case 83:
{
			tmppart[npart] = New(mazepart);
			tmppart[npart]->halign = yyvsp[-1].i % 10;
			tmppart[npart]->valign = yyvsp[-1].i / 10;
			tmppart[npart]->nrobjects = 0;
			tmppart[npart]->nloc = 0;
			tmppart[npart]->nrmonst = 0;
			scan_map(yyvsp[0].map);
			Free(yyvsp[0].map);
		  }
break;
case 84:
{
			yyval.i = yyvsp[-2].i + (yyvsp[0].i * 10);
		  }
break;
case 91:
{
			if (tmppart[npart]->nrobjects) {
			    yyerror("Object registers already initialized!");
			} else {
			    tmppart[npart]->robjects = (char *)alloc(n_olist);
			    (void) memcpy((genericptr_t)tmppart[npart]->robjects,
					  (genericptr_t)olist, n_olist);
			    tmppart[npart]->nrobjects = n_olist;
			}
		  }
break;
case 92:
{
			if (tmppart[npart]->nloc) {
			    yyerror("Location registers already initialized!");
			} else {
			    register int i;
			    tmppart[npart]->rloc_x = (char *) alloc(n_plist);
			    tmppart[npart]->rloc_y = (char *) alloc(n_plist);
			    for(i=0;i<n_plist;i++) {
				tmppart[npart]->rloc_x[i] = plist[i].x;
				tmppart[npart]->rloc_y[i] = plist[i].y;
			    }
			    tmppart[npart]->nloc = n_plist;
			}
		  }
break;
case 93:
{
			if (tmppart[npart]->nrmonst) {
			    yyerror("Monster registers already initialized!");
			} else {
			    tmppart[npart]->rmonst = (char *) alloc(n_mlist);
			    (void) memcpy((genericptr_t)tmppart[npart]->rmonst,
					  (genericptr_t)mlist, n_mlist);
			    tmppart[npart]->nrmonst = n_mlist;
			}
		  }
break;
case 94:
{
			if (n_olist < MAX_REGISTERS)
			    olist[n_olist++] = yyvsp[0].i;
			else
			    yyerror("Object list too long!");
		  }
break;
case 95:
{
			if (n_olist < MAX_REGISTERS)
			    olist[n_olist++] = yyvsp[-2].i;
			else
			    yyerror("Object list too long!");
		  }
break;
case 96:
{
			if (n_mlist < MAX_REGISTERS)
			    mlist[n_mlist++] = yyvsp[0].i;
			else
			    yyerror("Monster list too long!");
		  }
break;
case 97:
{
			if (n_mlist < MAX_REGISTERS)
			    mlist[n_mlist++] = yyvsp[-2].i;
			else
			    yyerror("Monster list too long!");
		  }
break;
case 98:
{
			if (n_plist < MAX_REGISTERS)
			    plist[n_plist++] = current_coord;
			else
			    yyerror("Location list too long!");
		  }
break;
case 99:
{
			if (n_plist < MAX_REGISTERS)
			    plist[n_plist++] = current_coord;
			else
			    yyerror("Location list too long!");
		  }
break;
case 123:
{
			tmpmonst[nmons] = New(monster);
			tmpmonst[nmons]->x = current_coord.x;
			tmpmonst[nmons]->y = current_coord.y;
			tmpmonst[nmons]->class = yyvsp[-4].i;
			tmpmonst[nmons]->peaceful = -1; /* no override */
			tmpmonst[nmons]->asleep = -1;
			tmpmonst[nmons]->align = - MAX_REGISTERS - 2;
			tmpmonst[nmons]->name.str = 0;
			tmpmonst[nmons]->appear = 0;
			tmpmonst[nmons]->appear_as.str = 0;
			tmpmonst[nmons]->chance = yyvsp[-6].i;
			tmpmonst[nmons]->id = NON_PM;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Monster");
			if (yyvsp[-2].map) {
			    int token = get_monster_id(yyvsp[-2].map, (char) yyvsp[-4].i);
			    if (token == ERR)
				yywarning(
			      "Invalid monster name!  Making random monster.");
			    else
				tmpmonst[nmons]->id = token;
			    Free(yyvsp[-2].map);
			}
		  }
break;
case 124:
{
			if (++nmons >= MAX_OF_TYPE) {
			    yyerror("Too many monsters in room or mazepart!");
			    nmons--;
			}
		  }
break;
case 127:
{
			tmpmonst[nmons]->name.str = yyvsp[0].map;
		  }
break;
case 128:
{
			tmpmonst[nmons]->peaceful = yyvsp[0].i;
		  }
break;
case 129:
{
			tmpmonst[nmons]->asleep = yyvsp[0].i;
		  }
break;
case 130:
{
			tmpmonst[nmons]->align = yyvsp[0].i;
		  }
break;
case 131:
{
			tmpmonst[nmons]->appear = yyvsp[-1].i;
			tmpmonst[nmons]->appear_as.str = yyvsp[0].map;
		  }
break;
case 132:
{
		  }
break;
case 133:
{
			/* 1: is contents of preceeding object with 2 */
			/* 2: is a container */
			/* 0: neither */
			tmpobj[nobj-1]->containment = 2;
		  }
break;
case 134:
{
			tmpobj[nobj] = New(object);
			tmpobj[nobj]->class = yyvsp[-2].i;
			tmpobj[nobj]->corpsenm = NON_PM;
			tmpobj[nobj]->curse_state = -1;
			tmpobj[nobj]->name.str = 0;
			tmpobj[nobj]->chance = yyvsp[-4].i;
			tmpobj[nobj]->id = -1;
			if (yyvsp[0].map) {
			    int token = get_object_id(yyvsp[0].map, yyvsp[-2].i);
			    if (token == ERR)
				yywarning(
				"Illegal object name!  Making random object.");
			     else
				tmpobj[nobj]->id = token;
			    Free(yyvsp[0].map);
			}
		  }
break;
case 135:
{
			if (++nobj >= MAX_OF_TYPE) {
			    yyerror("Too many objects in room or mazepart!");
			    nobj--;
			}
		  }
break;
case 136:
{
			tmpobj[nobj]->containment = 0;
			tmpobj[nobj]->x = current_coord.x;
			tmpobj[nobj]->y = current_coord.y;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Object");
		  }
break;
case 137:
{
			tmpobj[nobj]->containment = 1;
			/* random coordinate, will be overridden anyway */
			tmpobj[nobj]->x = -MAX_REGISTERS-1;
			tmpobj[nobj]->y = -MAX_REGISTERS-1;
		  }
break;
case 138:
{
			tmpobj[nobj]->spe = -127;
	/* Note below: we're trying to make as many of these optional as
	 * possible.  We clearly can't make curse_state, enchantment, and
	 * monster_id _all_ optional, since ",random" would be ambiguous.
	 * We can't even just make enchantment mandatory, since if we do that
	 * alone, ",random" requires too much lookahead to parse.
	 */
		  }
break;
case 139:
{
		  }
break;
case 140:
{
		  }
break;
case 141:
{
		  }
break;
case 142:
{
			tmpobj[nobj]->curse_state = -1;
		  }
break;
case 143:
{
			tmpobj[nobj]->curse_state = yyvsp[0].i;
		  }
break;
case 144:
{
			int token = get_monster_id(yyvsp[0].map, (char)0);
			if (token == ERR)	/* "random" */
			    tmpobj[nobj]->corpsenm = NON_PM - 1;
			else
			    tmpobj[nobj]->corpsenm = token;
			Free(yyvsp[0].map);
		  }
break;
case 145:
{
			tmpobj[nobj]->spe = -127;
		  }
break;
case 146:
{
			tmpobj[nobj]->spe = yyvsp[0].i;
		  }
break;
case 148:
{
		  }
break;
case 149:
{
			tmpobj[nobj]->name.str = yyvsp[0].map;
		  }
break;
case 150:
{
			tmpdoor[ndoor] = New(door);
			tmpdoor[ndoor]->x = current_coord.x;
			tmpdoor[ndoor]->y = current_coord.y;
			tmpdoor[ndoor]->mask = yyvsp[-2].i;
			if(current_coord.x >= 0 && current_coord.y >= 0 &&
			   tmpmap[current_coord.y][current_coord.x] != DOOR &&
			   tmpmap[current_coord.y][current_coord.x] != SDOOR)
			    yyerror("Door decl doesn't match the map");
			ndoor++;
			if (ndoor >= MAX_OF_TYPE) {
				yyerror("Too many doors in mazepart!");
				ndoor--;
			}
		  }
break;
case 151:
{
			tmptrap[ntrap] = New(trap);
			tmptrap[ntrap]->x = current_coord.x;
			tmptrap[ntrap]->y = current_coord.y;
			tmptrap[ntrap]->type = yyvsp[-2].i;
			tmptrap[ntrap]->chance = yyvsp[-4].i;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Trap");
			if (++ntrap >= MAX_OF_TYPE) {
				yyerror("Too many traps in room or mazepart!");
				ntrap--;
			}
		  }
break;
case 152:
{
		        int x, y, dir;

			tmpdb[ndb] = New(drawbridge);
			x = tmpdb[ndb]->x = current_coord.x;
			y = tmpdb[ndb]->y = current_coord.y;
			/* convert dir from a DIRECTION to a DB_DIR */
			dir = yyvsp[-2].i;
			switch(dir) {
			case W_NORTH: dir = DB_NORTH; y--; break;
			case W_SOUTH: dir = DB_SOUTH; y++; break;
			case W_EAST:  dir = DB_EAST;  x++; break;
			case W_WEST:  dir = DB_WEST;  x--; break;
			default:
			    yyerror("Invalid drawbridge direction");
			    break;
			}
			tmpdb[ndb]->dir = dir;
			if (current_coord.x >= 0 && current_coord.y >= 0 &&
			    !IS_WALL(tmpmap[y][x])) {
			    char ebuf[60];
			    Sprintf(ebuf,
				    "Wall needed for drawbridge (%02d, %02d)",
				    current_coord.x, current_coord.y);
			    yyerror(ebuf);
			}

			if ( yyvsp[0].i == D_ISOPEN )
			    tmpdb[ndb]->db_open = 1;
			else if ( yyvsp[0].i == D_CLOSED )
			    tmpdb[ndb]->db_open = 0;
			else if (yyvsp[0].i == -1)	/* RANDOM_TYPE */
			    tmpdb[ndb]->db_open = 127;	/* random */
			else
			    yyerror("A drawbridge can only be open, closed, or random!");
			ndb++;
			if (ndb >= MAX_OF_TYPE) {
				yyerror("Too many drawbridges in mazepart!");
				ndb--;
			}
		   }
break;
case 153:
{
			tmpwalk[nwalk] = New(walk);
			tmpwalk[nwalk]->x = current_coord.x;
			tmpwalk[nwalk]->y = current_coord.y;
			tmpwalk[nwalk]->dir = yyvsp[0].i;
			nwalk++;
			if (nwalk >= MAX_OF_TYPE) {
				yyerror("Too many mazewalks in mazepart!");
				nwalk--;
			}
		  }
break;
case 154:
{
			wallify_map();
		  }
break;
case 155:
{
			tmplad[nlad] = New(lad);
			tmplad[nlad]->x = current_coord.x;
			tmplad[nlad]->y = current_coord.y;
			tmplad[nlad]->up = yyvsp[0].i;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Ladder");
			nlad++;
			if (nlad >= MAX_OF_TYPE) {
				yyerror("Too many ladders in mazepart!");
				nlad--;
			}
		  }
break;
case 156:
{
			tmpstair[nstair] = New(stair);
			tmpstair[nstair]->x = current_coord.x;
			tmpstair[nstair]->y = current_coord.y;
			tmpstair[nstair]->up = yyvsp[0].i;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Stairway");
			nstair++;
			if (nstair >= MAX_OF_TYPE) {
				yyerror("Too many stairs in room or mazepart!");
				nstair--;
			}
		  }
break;
case 157:
{
			tmplreg[nlreg] = New(lev_region);
			tmplreg[nlreg]->in_islev = yyvsp[0].i;
			tmplreg[nlreg]->inarea.x1 = current_region.x1;
			tmplreg[nlreg]->inarea.y1 = current_region.y1;
			tmplreg[nlreg]->inarea.x2 = current_region.x2;
			tmplreg[nlreg]->inarea.y2 = current_region.y2;
		  }
break;
case 158:
{
			tmplreg[nlreg]->del_islev = yyvsp[-2].i;
			tmplreg[nlreg]->delarea.x1 = current_region.x1;
			tmplreg[nlreg]->delarea.y1 = current_region.y1;
			tmplreg[nlreg]->delarea.x2 = current_region.x2;
			tmplreg[nlreg]->delarea.y2 = current_region.y2;
			if(yyvsp[0].i)
			    tmplreg[nlreg]->rtype = LR_UPSTAIR;
			else
			    tmplreg[nlreg]->rtype = LR_DOWNSTAIR;
			tmplreg[nlreg]->rname.str = 0;
			nlreg++;
			if (nlreg >= MAX_OF_TYPE) {
				yyerror("Too many levregions in mazepart!");
				nlreg--;
			}
		  }
break;
case 159:
{
			tmplreg[nlreg] = New(lev_region);
			tmplreg[nlreg]->in_islev = yyvsp[0].i;
			tmplreg[nlreg]->inarea.x1 = current_region.x1;
			tmplreg[nlreg]->inarea.y1 = current_region.y1;
			tmplreg[nlreg]->inarea.x2 = current_region.x2;
			tmplreg[nlreg]->inarea.y2 = current_region.y2;
		  }
break;
case 160:
{
			tmplreg[nlreg]->del_islev = yyvsp[-2].i;
			tmplreg[nlreg]->delarea.x1 = current_region.x1;
			tmplreg[nlreg]->delarea.y1 = current_region.y1;
			tmplreg[nlreg]->delarea.x2 = current_region.x2;
			tmplreg[nlreg]->delarea.y2 = current_region.y2;
			tmplreg[nlreg]->rtype = LR_PORTAL;
			tmplreg[nlreg]->rname.str = yyvsp[0].map;
			nlreg++;
			if (nlreg >= MAX_OF_TYPE) {
				yyerror("Too many levregions in mazepart!");
				nlreg--;
			}
		  }
break;
case 161:
{
			tmplreg[nlreg] = New(lev_region);
			tmplreg[nlreg]->in_islev = yyvsp[0].i;
			tmplreg[nlreg]->inarea.x1 = current_region.x1;
			tmplreg[nlreg]->inarea.y1 = current_region.y1;
			tmplreg[nlreg]->inarea.x2 = current_region.x2;
			tmplreg[nlreg]->inarea.y2 = current_region.y2;
		  }
break;
case 162:
{
			tmplreg[nlreg]->del_islev = yyvsp[0].i;
			tmplreg[nlreg]->delarea.x1 = current_region.x1;
			tmplreg[nlreg]->delarea.y1 = current_region.y1;
			tmplreg[nlreg]->delarea.x2 = current_region.x2;
			tmplreg[nlreg]->delarea.y2 = current_region.y2;
		  }
break;
case 163:
{
			switch(yyvsp[0].i) {
			case -1: tmplreg[nlreg]->rtype = LR_TELE; break;
			case 0: tmplreg[nlreg]->rtype = LR_DOWNTELE; break;
			case 1: tmplreg[nlreg]->rtype = LR_UPTELE; break;
			}
			tmplreg[nlreg]->rname.str = 0;
			nlreg++;
			if (nlreg >= MAX_OF_TYPE) {
				yyerror("Too many levregions in mazepart!");
				nlreg--;
			}
		  }
break;
case 164:
{
			tmplreg[nlreg] = New(lev_region);
			tmplreg[nlreg]->in_islev = yyvsp[0].i;
			tmplreg[nlreg]->inarea.x1 = current_region.x1;
			tmplreg[nlreg]->inarea.y1 = current_region.y1;
			tmplreg[nlreg]->inarea.x2 = current_region.x2;
			tmplreg[nlreg]->inarea.y2 = current_region.y2;
		  }
break;
case 165:
{
			tmplreg[nlreg]->del_islev = yyvsp[0].i;
			tmplreg[nlreg]->delarea.x1 = current_region.x1;
			tmplreg[nlreg]->delarea.y1 = current_region.y1;
			tmplreg[nlreg]->delarea.x2 = current_region.x2;
			tmplreg[nlreg]->delarea.y2 = current_region.y2;
			tmplreg[nlreg]->rtype = LR_BRANCH;
			tmplreg[nlreg]->rname.str = 0;
			nlreg++;
			if (nlreg >= MAX_OF_TYPE) {
				yyerror("Too many levregions in mazepart!");
				nlreg--;
			}
		  }
break;
case 166:
{
			yyval.i = -1;
		  }
break;
case 167:
{
			yyval.i = yyvsp[0].i;
		  }
break;
case 168:
{
			yyval.i = 0;
		  }
break;
case 169:
{
/* This series of if statements is a hack for MSC 5.1.  It seems that its
   tiny little brain cannot compile if these are all one big if statement. */
			if (yyvsp[-7].i <= 0 || yyvsp[-7].i >= COLNO)
				yyerror("Region out of level range!");
			else if (yyvsp[-5].i < 0 || yyvsp[-5].i >= ROWNO)
				yyerror("Region out of level range!");
			else if (yyvsp[-3].i <= 0 || yyvsp[-3].i >= COLNO)
				yyerror("Region out of level range!");
			else if (yyvsp[-1].i < 0 || yyvsp[-1].i >= ROWNO)
				yyerror("Region out of level range!");
			current_region.x1 = yyvsp[-7].i;
			current_region.y1 = yyvsp[-5].i;
			current_region.x2 = yyvsp[-3].i;
			current_region.y2 = yyvsp[-1].i;
			yyval.i = 1;
		  }
break;
case 170:
{
			tmpfountain[nfountain] = New(fountain);
			tmpfountain[nfountain]->x = current_coord.x;
			tmpfountain[nfountain]->y = current_coord.y;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Fountain");
			nfountain++;
			if (nfountain >= MAX_OF_TYPE) {
			    yyerror("Too many fountains in room or mazepart!");
			    nfountain--;
			}
		  }
break;
case 171:
{
			tmpsink[nsink] = New(sink);
			tmpsink[nsink]->x = current_coord.x;
			tmpsink[nsink]->y = current_coord.y;
			nsink++;
			if (nsink >= MAX_OF_TYPE) {
				yyerror("Too many sinks in room!");
				nsink--;
			}
		  }
break;
case 172:
{
			tmppool[npool] = New(pool);
			tmppool[npool]->x = current_coord.x;
			tmppool[npool]->y = current_coord.y;
			npool++;
			if (npool >= MAX_OF_TYPE) {
				yyerror("Too many pools in room!");
				npool--;
			}
		  }
break;
case 173:
{
			tmpdig[ndig] = New(digpos);
			tmpdig[ndig]->x1 = current_region.x1;
			tmpdig[ndig]->y1 = current_region.y1;
			tmpdig[ndig]->x2 = current_region.x2;
			tmpdig[ndig]->y2 = current_region.y2;
			ndig++;
			if (ndig >= MAX_OF_TYPE) {
				yyerror("Too many diggables in mazepart!");
				ndig--;
			}
		  }
break;
case 174:
{
			tmppass[npass] = New(digpos);
			tmppass[npass]->x1 = current_region.x1;
			tmppass[npass]->y1 = current_region.y1;
			tmppass[npass]->x2 = current_region.x2;
			tmppass[npass]->y2 = current_region.y2;
			npass++;
			if (npass >= 32) {
				yyerror("Too many passwalls in mazepart!");
				npass--;
			}
		  }
break;
case 175:
{
			tmpreg[nreg] = New(region);
			tmpreg[nreg]->x1 = current_region.x1;
			tmpreg[nreg]->y1 = current_region.y1;
			tmpreg[nreg]->x2 = current_region.x2;
			tmpreg[nreg]->y2 = current_region.y2;
			tmpreg[nreg]->rlit = yyvsp[-3].i;
			tmpreg[nreg]->rtype = yyvsp[-1].i;
			if(yyvsp[0].i & 1) tmpreg[nreg]->rtype += MAXRTYPE+1;
			tmpreg[nreg]->rirreg = ((yyvsp[0].i & 2) != 0);
			if(current_region.x1 > current_region.x2 ||
			   current_region.y1 > current_region.y2)
			   yyerror("Region start > end!");
			if(tmpreg[nreg]->rtype == VAULT &&
			   (tmpreg[nreg]->rirreg ||
			    (tmpreg[nreg]->x2 - tmpreg[nreg]->x1 != 1) ||
			    (tmpreg[nreg]->y2 - tmpreg[nreg]->y1 != 1)))
				yyerror("Vaults must be exactly 2x2!");
			if(want_warnings && !tmpreg[nreg]->rirreg &&
			   current_region.x1 > 0 && current_region.y1 > 0 &&
			   current_region.x2 < (int)max_x_map &&
			   current_region.y2 < (int)max_y_map) {
			    /* check for walls in the room */
			    char ebuf[60];
			    register int x, y, nrock = 0;

			    for(y=current_region.y1; y<=current_region.y2; y++)
				for(x=current_region.x1;
				    x<=current_region.x2; x++)
				    if(IS_ROCK(tmpmap[y][x]) ||
				       IS_DOOR(tmpmap[y][x])) nrock++;
			    if(nrock) {
				Sprintf(ebuf,
					"Rock in room (%02d,%02d,%02d,%02d)?!",
					current_region.x1, current_region.y1,
					current_region.x2, current_region.y2);
				yywarning(ebuf);
			    }
			    if (
		!IS_ROCK(tmpmap[current_region.y1-1][current_region.x1-1]) ||
		!IS_ROCK(tmpmap[current_region.y2+1][current_region.x1-1]) ||
		!IS_ROCK(tmpmap[current_region.y1-1][current_region.x2+1]) ||
		!IS_ROCK(tmpmap[current_region.y2+1][current_region.x2+1])) {
				Sprintf(ebuf,
				"NonRock edge in room (%02d,%02d,%02d,%02d)?!",
					current_region.x1, current_region.y1,
					current_region.x2, current_region.y2);
				yywarning(ebuf);
			    }
			} else if(tmpreg[nreg]->rirreg &&
		!IS_ROOM(tmpmap[current_region.y1][current_region.x1])) {
			    char ebuf[60];
			    Sprintf(ebuf,
				    "Rock in irregular room (%02d,%02d)?!",
				    current_region.x1, current_region.y1);
			    yyerror(ebuf);
			}
			nreg++;
			if (nreg >= MAX_OF_TYPE) {
				yyerror("Too many regions in mazepart!");
				nreg--;
			}
		  }
break;
case 176:
{
			tmpaltar[naltar] = New(altar);
			tmpaltar[naltar]->x = current_coord.x;
			tmpaltar[naltar]->y = current_coord.y;
			tmpaltar[naltar]->align = yyvsp[-2].i;
			tmpaltar[naltar]->shrine = yyvsp[0].i;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Altar");
			naltar++;
			if (naltar >= MAX_OF_TYPE) {
				yyerror("Too many altars in room or mazepart!");
				naltar--;
			}
		  }
break;
case 177:
{
			tmpgold[ngold] = New(gold);
			tmpgold[ngold]->x = current_coord.x;
			tmpgold[ngold]->y = current_coord.y;
			tmpgold[ngold]->amount = yyvsp[-2].i;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Gold");
			ngold++;
			if (ngold >= MAX_OF_TYPE) {
				yyerror("Too many golds in room or mazepart!");
				ngold--;
			}
		  }
break;
case 178:
{
			tmpengraving[nengraving] = New(engraving);
			tmpengraving[nengraving]->x = current_coord.x;
			tmpengraving[nengraving]->y = current_coord.y;
			tmpengraving[nengraving]->engr.str = yyvsp[0].map;
			tmpengraving[nengraving]->etype = yyvsp[-2].i;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Engraving");
			nengraving++;
			if (nengraving >= MAX_OF_TYPE) {
			    yyerror("Too many engravings in room or mazepart!");
			    nengraving--;
			}
		  }
break;
case 180:
{
			yyval.i = - MAX_REGISTERS - 1;
		  }
break;
case 183:
{
			yyval.i = - MAX_REGISTERS - 1;
		  }
break;
case 186:
{
			yyval.map = (char *) 0;
		  }
break;
case 188:
{
			yyval.map = (char *) 0;
		  }
break;
case 189:
{
			int token = get_trap_type(yyvsp[0].map);
			if (token == ERR)
				yyerror("Unknown trap type!");
			yyval.i = token;
			Free(yyvsp[0].map);
		  }
break;
case 191:
{
			int token = get_room_type(yyvsp[0].map);
			if (token == ERR) {
				yywarning("Unknown room type!  Making ordinary room...");
				yyval.i = OROOM;
			} else
				yyval.i = token;
			Free(yyvsp[0].map);
		  }
break;
case 193:
{
			yyval.i = 0;
		  }
break;
case 194:
{
			yyval.i = yyvsp[0].i;
		  }
break;
case 195:
{
			yyval.i = yyvsp[-2].i + (yyvsp[0].i << 1);
		  }
break;
case 198:
{
			current_coord.x = current_coord.y = -MAX_REGISTERS-1;
		  }
break;
case 205:
{
			yyval.i = - MAX_REGISTERS - 1;
		  }
break;
case 208:
{
			if ( yyvsp[-1].i >= MAX_REGISTERS )
				yyerror("Register Index overflow!");
			else
				current_coord.x = current_coord.y = - yyvsp[-1].i - 1;
		  }
break;
case 209:
{
			if ( yyvsp[-1].i >= MAX_REGISTERS )
				yyerror("Register Index overflow!");
			else
				yyval.i = - yyvsp[-1].i - 1;
		  }
break;
case 210:
{
			if ( yyvsp[-1].i >= MAX_REGISTERS )
				yyerror("Register Index overflow!");
			else
				yyval.i = - yyvsp[-1].i - 1;
		  }
break;
case 211:
{
			if ( yyvsp[-1].i >= 3 )
				yyerror("Register Index overflow!");
			else
				yyval.i = - yyvsp[-1].i - 1;
		  }
break;
case 213:
{
			if (check_monster_char((char) yyvsp[0].i))
				yyval.i = yyvsp[0].i ;
			else {
				yyerror("Unknown monster class!");
				yyval.i = ERR;
			}
		  }
break;
case 214:
{
			char c = yyvsp[0].i;
			if (check_object_char(c))
				yyval.i = c;
			else {
				yyerror("Unknown char class!");
				yyval.i = ERR;
			}
		  }
break;
case 218:
{
			yyval.i = 100;	/* default is 100% */
		  }
break;
case 219:
{
			if (yyvsp[0].i <= 0 || yyvsp[0].i > 100)
			    yyerror("Expected percentile chance.");
			yyval.i = yyvsp[0].i;
		  }
break;
case 222:
{
			if (!in_room && !init_lev.init_present &&
			    (yyvsp[-3].i < 0 || yyvsp[-3].i > (int)max_x_map ||
			     yyvsp[-1].i < 0 || yyvsp[-1].i > (int)max_y_map))
			    yyerror("Coordinates out of map range!");
			current_coord.x = yyvsp[-3].i;
			current_coord.y = yyvsp[-1].i;
		  }
break;
case 223:
{
/* This series of if statements is a hack for MSC 5.1.  It seems that its
   tiny little brain cannot compile if these are all one big if statement. */
			if (yyvsp[-7].i < 0 || yyvsp[-7].i > (int)max_x_map)
				yyerror("Region out of map range!");
			else if (yyvsp[-5].i < 0 || yyvsp[-5].i > (int)max_y_map)
				yyerror("Region out of map range!");
			else if (yyvsp[-3].i < 0 || yyvsp[-3].i > (int)max_x_map)
				yyerror("Region out of map range!");
			else if (yyvsp[-1].i < 0 || yyvsp[-1].i > (int)max_y_map)
				yyerror("Region out of map range!");
			current_region.x1 = yyvsp[-7].i;
			current_region.y1 = yyvsp[-5].i;
			current_region.x2 = yyvsp[-3].i;
			current_region.y2 = yyvsp[-1].i;
		  }
break;
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) != 0 && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yyss + yystacksize - 1)
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
