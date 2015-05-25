
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 1 "lev_comp.y"

/* NetHack 3.6  lev_comp.y	$NHDT-Date: 1432512787 2015/05/25 00:13:07 $  $NHDT-Branch: master $:$NHDT-Revision: 1.15 $ */
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

#define SPEC_LEV    /* for USE_OLDARGS (sp_lev.h) */
#include "hack.h"
#include "sp_lev.h"

#define ERR		(-1)
/* many types of things are put in chars for transference to NetHack.
 * since some systems will use signed chars, limit everybody to the
 * same number for portability.
 */
#define MAX_OF_TYPE	128

#define MAX_NESTED_IFS	20
#define MAX_SWITCH_CASES 20

#define New(type)		\
	(type *) memset((genericptr_t)alloc(sizeof(type)), 0, sizeof(type))
#define NewTab(type, size)	(type **) alloc(sizeof(type *) * size)
#define Free(ptr)		free((genericptr_t)ptr)

extern void VDECL(lc_error, (const char *, ...));
extern void VDECL(lc_warning, (const char *, ...));
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
extern void FDECL(scan_map, (char *, sp_lev *));
extern void FDECL(add_opcode, (sp_lev *, int, genericptr_t));
extern genericptr_t FDECL(get_last_opcode_data1, (sp_lev *, int));
extern genericptr_t FDECL(get_last_opcode_data2, (sp_lev *, int,int));
extern boolean FDECL(check_subrooms, (sp_lev *));
extern boolean FDECL(write_level_file, (char *,sp_lev *));
extern struct opvar *FDECL(set_opvar_int, (struct opvar *, long));
extern void VDECL(add_opvars, (sp_lev *, const char *, ...));
extern void FDECL(start_level_def, (sp_lev * *, char *));

extern struct lc_funcdefs *FDECL(funcdef_new,(long,char *));
extern void FDECL(funcdef_free_all,(struct lc_funcdefs *));
extern struct lc_funcdefs *FDECL(funcdef_defined,(struct lc_funcdefs *,char *, int));
extern char *FDECL(funcdef_paramtypes, (struct lc_funcdefs *));
extern char *FDECL(decode_parm_str, (char *));

extern struct lc_vardefs *FDECL(vardef_new,(long,char *));
extern void FDECL(vardef_free_all,(struct lc_vardefs *));
extern struct lc_vardefs *FDECL(vardef_defined,(struct lc_vardefs *,char *, int));

extern void NDECL(break_stmt_start);
extern void FDECL(break_stmt_end, (sp_lev *));
extern void FDECL(break_stmt_new, (sp_lev *, long));

extern void FDECL(splev_add_from, (sp_lev *, sp_lev *));

extern void FDECL(check_vardef_type, (struct lc_vardefs *, char *, long));
extern void FDECL(vardef_used, (struct lc_vardefs *, char *));
extern struct lc_vardefs *FDECL(add_vardef_type, (struct lc_vardefs *, char *, long));

extern int FDECL(reverse_jmp_opcode, (int));

struct coord {
	long x;
	long y;
};

struct forloopdef {
    char *varname;
    long jmp_point;
};
static struct forloopdef forloop_list[MAX_NESTED_IFS];
static short n_forloops = 0;


sp_lev *splev = NULL;

static struct opvar *if_list[MAX_NESTED_IFS];

static short n_if_list = 0;

unsigned int max_x_map, max_y_map;
int obj_containment = 0;

int in_container_obj = 0;

/* integer value is possibly an inconstant value (eg. dice notation or a variable) */
int is_inconstant_number = 0;

int in_switch_statement = 0;
static struct opvar *switch_check_jump = NULL;
static struct opvar *switch_default_case = NULL;
static struct opvar *switch_case_list[MAX_SWITCH_CASES];
static long switch_case_value[MAX_SWITCH_CASES];
int n_switch_case_list = 0;

int allow_break_statements = 0;
struct lc_breakdef *break_list = NULL;

extern struct lc_vardefs *variable_definitions;


struct lc_vardefs *function_tmp_var_defs = NULL;
extern struct lc_funcdefs *function_definitions;
struct lc_funcdefs *curr_function = NULL;
struct lc_funcdefs_parm * curr_function_param = NULL;
int in_function_definition = 0;
sp_lev *function_splev_backup = NULL;

extern int fatal_error;
extern int got_errors;
extern int line_number;
extern const char *fname;

extern char curr_token[512];



/* Line 189 of yacc.c  */
#line 221 "y.tab.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     CHAR = 258,
     INTEGER = 259,
     BOOLEAN = 260,
     PERCENT = 261,
     SPERCENT = 262,
     MINUS_INTEGER = 263,
     PLUS_INTEGER = 264,
     MAZE_GRID_ID = 265,
     SOLID_FILL_ID = 266,
     MINES_ID = 267,
     ROGUELEV_ID = 268,
     MESSAGE_ID = 269,
     MAZE_ID = 270,
     LEVEL_ID = 271,
     LEV_INIT_ID = 272,
     GEOMETRY_ID = 273,
     NOMAP_ID = 274,
     OBJECT_ID = 275,
     COBJECT_ID = 276,
     MONSTER_ID = 277,
     TRAP_ID = 278,
     DOOR_ID = 279,
     DRAWBRIDGE_ID = 280,
     object_ID = 281,
     monster_ID = 282,
     terrain_ID = 283,
     MAZEWALK_ID = 284,
     WALLIFY_ID = 285,
     REGION_ID = 286,
     FILLING = 287,
     IRREGULAR = 288,
     JOINED = 289,
     ALTAR_ID = 290,
     LADDER_ID = 291,
     STAIR_ID = 292,
     NON_DIGGABLE_ID = 293,
     NON_PASSWALL_ID = 294,
     ROOM_ID = 295,
     PORTAL_ID = 296,
     TELEPRT_ID = 297,
     BRANCH_ID = 298,
     LEV = 299,
     MINERALIZE_ID = 300,
     CORRIDOR_ID = 301,
     GOLD_ID = 302,
     ENGRAVING_ID = 303,
     FOUNTAIN_ID = 304,
     POOL_ID = 305,
     SINK_ID = 306,
     NONE = 307,
     RAND_CORRIDOR_ID = 308,
     DOOR_STATE = 309,
     LIGHT_STATE = 310,
     CURSE_TYPE = 311,
     ENGRAVING_TYPE = 312,
     DIRECTION = 313,
     RANDOM_TYPE = 314,
     RANDOM_TYPE_BRACKET = 315,
     A_REGISTER = 316,
     ALIGNMENT = 317,
     LEFT_OR_RIGHT = 318,
     CENTER = 319,
     TOP_OR_BOT = 320,
     ALTAR_TYPE = 321,
     UP_OR_DOWN = 322,
     SUBROOM_ID = 323,
     NAME_ID = 324,
     FLAGS_ID = 325,
     FLAG_TYPE = 326,
     MON_ATTITUDE = 327,
     MON_ALERTNESS = 328,
     MON_APPEARANCE = 329,
     ROOMDOOR_ID = 330,
     IF_ID = 331,
     ELSE_ID = 332,
     TERRAIN_ID = 333,
     HORIZ_OR_VERT = 334,
     REPLACE_TERRAIN_ID = 335,
     EXIT_ID = 336,
     SHUFFLE_ID = 337,
     QUANTITY_ID = 338,
     BURIED_ID = 339,
     LOOP_ID = 340,
     FOR_ID = 341,
     TO_ID = 342,
     SWITCH_ID = 343,
     CASE_ID = 344,
     BREAK_ID = 345,
     DEFAULT_ID = 346,
     ERODED_ID = 347,
     TRAPPED_ID = 348,
     RECHARGED_ID = 349,
     INVIS_ID = 350,
     GREASED_ID = 351,
     FEMALE_ID = 352,
     CANCELLED_ID = 353,
     REVIVED_ID = 354,
     AVENGE_ID = 355,
     FLEEING_ID = 356,
     BLINDED_ID = 357,
     PARALYZED_ID = 358,
     STUNNED_ID = 359,
     CONFUSED_ID = 360,
     SEENTRAPS_ID = 361,
     ALL_ID = 362,
     MONTYPE_ID = 363,
     GRAVE_ID = 364,
     ERODEPROOF_ID = 365,
     FUNCTION_ID = 366,
     MSG_OUTPUT_TYPE = 367,
     COMPARE_TYPE = 368,
     UNKNOWN_TYPE = 369,
     rect_ID = 370,
     fillrect_ID = 371,
     line_ID = 372,
     randline_ID = 373,
     grow_ID = 374,
     selection_ID = 375,
     flood_ID = 376,
     rndcoord_ID = 377,
     circle_ID = 378,
     ellipse_ID = 379,
     filter_ID = 380,
     complement_ID = 381,
     gradient_ID = 382,
     GRADIENT_TYPE = 383,
     LIMITED = 384,
     HUMIDITY_TYPE = 385,
     STRING = 386,
     MAP_ID = 387,
     NQSTRING = 388,
     VARSTRING = 389,
     CFUNC = 390,
     CFUNC_INT = 391,
     CFUNC_STR = 392,
     CFUNC_COORD = 393,
     CFUNC_REGION = 394,
     VARSTRING_INT = 395,
     VARSTRING_INT_ARRAY = 396,
     VARSTRING_STRING = 397,
     VARSTRING_STRING_ARRAY = 398,
     VARSTRING_VAR = 399,
     VARSTRING_VAR_ARRAY = 400,
     VARSTRING_COORD = 401,
     VARSTRING_COORD_ARRAY = 402,
     VARSTRING_REGION = 403,
     VARSTRING_REGION_ARRAY = 404,
     VARSTRING_MAPCHAR = 405,
     VARSTRING_MAPCHAR_ARRAY = 406,
     VARSTRING_MONST = 407,
     VARSTRING_MONST_ARRAY = 408,
     VARSTRING_OBJ = 409,
     VARSTRING_OBJ_ARRAY = 410,
     VARSTRING_SEL = 411,
     VARSTRING_SEL_ARRAY = 412,
     METHOD_INT = 413,
     METHOD_INT_ARRAY = 414,
     METHOD_STRING = 415,
     METHOD_STRING_ARRAY = 416,
     METHOD_VAR = 417,
     METHOD_VAR_ARRAY = 418,
     METHOD_COORD = 419,
     METHOD_COORD_ARRAY = 420,
     METHOD_REGION = 421,
     METHOD_REGION_ARRAY = 422,
     METHOD_MAPCHAR = 423,
     METHOD_MAPCHAR_ARRAY = 424,
     METHOD_MONST = 425,
     METHOD_MONST_ARRAY = 426,
     METHOD_OBJ = 427,
     METHOD_OBJ_ARRAY = 428,
     METHOD_SEL = 429,
     METHOD_SEL_ARRAY = 430,
     DICE = 431
   };
#endif
/* Tokens.  */
#define CHAR 258
#define INTEGER 259
#define BOOLEAN 260
#define PERCENT 261
#define SPERCENT 262
#define MINUS_INTEGER 263
#define PLUS_INTEGER 264
#define MAZE_GRID_ID 265
#define SOLID_FILL_ID 266
#define MINES_ID 267
#define ROGUELEV_ID 268
#define MESSAGE_ID 269
#define MAZE_ID 270
#define LEVEL_ID 271
#define LEV_INIT_ID 272
#define GEOMETRY_ID 273
#define NOMAP_ID 274
#define OBJECT_ID 275
#define COBJECT_ID 276
#define MONSTER_ID 277
#define TRAP_ID 278
#define DOOR_ID 279
#define DRAWBRIDGE_ID 280
#define object_ID 281
#define monster_ID 282
#define terrain_ID 283
#define MAZEWALK_ID 284
#define WALLIFY_ID 285
#define REGION_ID 286
#define FILLING 287
#define IRREGULAR 288
#define JOINED 289
#define ALTAR_ID 290
#define LADDER_ID 291
#define STAIR_ID 292
#define NON_DIGGABLE_ID 293
#define NON_PASSWALL_ID 294
#define ROOM_ID 295
#define PORTAL_ID 296
#define TELEPRT_ID 297
#define BRANCH_ID 298
#define LEV 299
#define MINERALIZE_ID 300
#define CORRIDOR_ID 301
#define GOLD_ID 302
#define ENGRAVING_ID 303
#define FOUNTAIN_ID 304
#define POOL_ID 305
#define SINK_ID 306
#define NONE 307
#define RAND_CORRIDOR_ID 308
#define DOOR_STATE 309
#define LIGHT_STATE 310
#define CURSE_TYPE 311
#define ENGRAVING_TYPE 312
#define DIRECTION 313
#define RANDOM_TYPE 314
#define RANDOM_TYPE_BRACKET 315
#define A_REGISTER 316
#define ALIGNMENT 317
#define LEFT_OR_RIGHT 318
#define CENTER 319
#define TOP_OR_BOT 320
#define ALTAR_TYPE 321
#define UP_OR_DOWN 322
#define SUBROOM_ID 323
#define NAME_ID 324
#define FLAGS_ID 325
#define FLAG_TYPE 326
#define MON_ATTITUDE 327
#define MON_ALERTNESS 328
#define MON_APPEARANCE 329
#define ROOMDOOR_ID 330
#define IF_ID 331
#define ELSE_ID 332
#define TERRAIN_ID 333
#define HORIZ_OR_VERT 334
#define REPLACE_TERRAIN_ID 335
#define EXIT_ID 336
#define SHUFFLE_ID 337
#define QUANTITY_ID 338
#define BURIED_ID 339
#define LOOP_ID 340
#define FOR_ID 341
#define TO_ID 342
#define SWITCH_ID 343
#define CASE_ID 344
#define BREAK_ID 345
#define DEFAULT_ID 346
#define ERODED_ID 347
#define TRAPPED_ID 348
#define RECHARGED_ID 349
#define INVIS_ID 350
#define GREASED_ID 351
#define FEMALE_ID 352
#define CANCELLED_ID 353
#define REVIVED_ID 354
#define AVENGE_ID 355
#define FLEEING_ID 356
#define BLINDED_ID 357
#define PARALYZED_ID 358
#define STUNNED_ID 359
#define CONFUSED_ID 360
#define SEENTRAPS_ID 361
#define ALL_ID 362
#define MONTYPE_ID 363
#define GRAVE_ID 364
#define ERODEPROOF_ID 365
#define FUNCTION_ID 366
#define MSG_OUTPUT_TYPE 367
#define COMPARE_TYPE 368
#define UNKNOWN_TYPE 369
#define rect_ID 370
#define fillrect_ID 371
#define line_ID 372
#define randline_ID 373
#define grow_ID 374
#define selection_ID 375
#define flood_ID 376
#define rndcoord_ID 377
#define circle_ID 378
#define ellipse_ID 379
#define filter_ID 380
#define complement_ID 381
#define gradient_ID 382
#define GRADIENT_TYPE 383
#define LIMITED 384
#define HUMIDITY_TYPE 385
#define STRING 386
#define MAP_ID 387
#define NQSTRING 388
#define VARSTRING 389
#define CFUNC 390
#define CFUNC_INT 391
#define CFUNC_STR 392
#define CFUNC_COORD 393
#define CFUNC_REGION 394
#define VARSTRING_INT 395
#define VARSTRING_INT_ARRAY 396
#define VARSTRING_STRING 397
#define VARSTRING_STRING_ARRAY 398
#define VARSTRING_VAR 399
#define VARSTRING_VAR_ARRAY 400
#define VARSTRING_COORD 401
#define VARSTRING_COORD_ARRAY 402
#define VARSTRING_REGION 403
#define VARSTRING_REGION_ARRAY 404
#define VARSTRING_MAPCHAR 405
#define VARSTRING_MAPCHAR_ARRAY 406
#define VARSTRING_MONST 407
#define VARSTRING_MONST_ARRAY 408
#define VARSTRING_OBJ 409
#define VARSTRING_OBJ_ARRAY 410
#define VARSTRING_SEL 411
#define VARSTRING_SEL_ARRAY 412
#define METHOD_INT 413
#define METHOD_INT_ARRAY 414
#define METHOD_STRING 415
#define METHOD_STRING_ARRAY 416
#define METHOD_VAR 417
#define METHOD_VAR_ARRAY 418
#define METHOD_COORD 419
#define METHOD_COORD_ARRAY 420
#define METHOD_REGION 421
#define METHOD_REGION_ARRAY 422
#define METHOD_MAPCHAR 423
#define METHOD_MAPCHAR_ARRAY 424
#define METHOD_MONST 425
#define METHOD_MONST_ARRAY 426
#define METHOD_OBJ 427
#define METHOD_OBJ_ARRAY 428
#define METHOD_SEL 429
#define METHOD_SEL_ARRAY 430
#define DICE 431




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 149 "lev_comp.y"

	long	i;
	char*	map;
	struct {
		long room;
		long wall;
		long door;
	} corpos;
    struct {
	long area;
	long x1;
	long y1;
	long x2;
	long y2;
    } lregn;
    struct {
	long x;
	long y;
    } crd;
    struct {
	long ter;
	long lit;
    } terr;
    struct {
	long height;
	long width;
    } sze;
    struct {
	long die;
	long num;
    } dice;
    struct {
	long cfunc;
	char *varstr;
    } meth;



/* Line 214 of yacc.c  */
#line 648 "y.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 660 "y.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  9
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1033

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  194
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  160
/* YYNRULES -- Number of rules.  */
#define YYNRULES  406
/* YYNRULES -- Number of states.  */
#define YYNSTATES  866

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   431

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,   189,   193,     2,
     133,   134,   187,   185,   131,   186,   191,   188,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   132,     2,
       2,   190,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   135,     2,   136,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   137,   192,   138,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   139,   140,   141,   142,
     143,   144,   145,   146,   147,   148,   149,   150,   151,   152,
     153,   154,   155,   156,   157,   158,   159,   160,   161,   162,
     163,   164,   165,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   180,   181,   182,
     183,   184
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     6,     8,    11,    15,    19,    25,
      27,    29,    35,    41,    45,    62,    63,    66,    67,    70,
      71,    74,    76,    78,    79,    83,    87,    89,    90,    93,
      97,    99,   101,   103,   105,   107,   109,   111,   113,   115,
     117,   119,   121,   123,   125,   127,   129,   131,   133,   135,
     137,   139,   141,   143,   145,   147,   149,   151,   153,   155,
     157,   159,   161,   163,   165,   167,   169,   171,   173,   175,
     177,   179,   181,   183,   185,   187,   189,   191,   193,   195,
     197,   199,   201,   203,   205,   207,   209,   211,   213,   215,
     217,   219,   221,   223,   225,   227,   229,   231,   235,   239,
     245,   249,   255,   261,   267,   271,   275,   281,   287,   293,
     301,   309,   317,   323,   325,   329,   331,   335,   337,   341,
     343,   347,   349,   353,   355,   359,   361,   365,   366,   367,
     376,   381,   383,   384,   386,   388,   394,   398,   399,   400,
     410,   411,   414,   415,   421,   422,   427,   429,   432,   434,
     441,   442,   446,   447,   454,   455,   460,   461,   466,   468,
     469,   474,   478,   480,   484,   488,   494,   500,   508,   513,
     514,   525,   526,   539,   540,   543,   549,   551,   557,   559,
     565,   567,   573,   575,   585,   591,   593,   595,   597,   599,
     601,   605,   607,   609,   611,   619,   625,   627,   629,   631,
     633,   637,   638,   644,   649,   650,   654,   656,   658,   660,
     662,   665,   667,   669,   671,   673,   675,   679,   683,   687,
     689,   691,   695,   697,   699,   703,   707,   708,   714,   717,
     718,   722,   724,   728,   730,   734,   738,   740,   742,   746,
     748,   750,   752,   756,   758,   760,   762,   768,   776,   782,
     791,   793,   797,   803,   809,   817,   825,   832,   838,   839,
     842,   846,   850,   854,   856,   862,   872,   878,   882,   886,
     887,   898,   899,   901,   909,   915,   921,   925,   931,   939,
     949,   951,   953,   955,   957,   959,   960,   963,   965,   969,
     971,   973,   975,   977,   979,   981,   983,   985,   987,   989,
     991,   993,   997,   999,  1001,  1006,  1008,  1010,  1015,  1017,
    1019,  1024,  1026,  1031,  1037,  1039,  1043,  1045,  1049,  1051,
    1053,  1058,  1068,  1070,  1072,  1077,  1079,  1085,  1087,  1089,
    1094,  1096,  1098,  1104,  1106,  1108,  1110,  1115,  1117,  1119,
    1125,  1127,  1129,  1133,  1135,  1137,  1141,  1143,  1148,  1152,
    1156,  1160,  1164,  1168,  1172,  1174,  1176,  1180,  1182,  1186,
    1187,  1189,  1191,  1193,  1195,  1199,  1200,  1202,  1204,  1207,
    1210,  1215,  1222,  1227,  1234,  1241,  1248,  1255,  1258,  1265,
    1274,  1283,  1294,  1309,  1312,  1314,  1318,  1320,  1324,  1326,
    1328,  1330,  1332,  1334,  1336,  1338,  1340,  1342,  1344,  1346,
    1348,  1350,  1352,  1354,  1356,  1358,  1369
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     195,     0,    -1,    -1,   196,    -1,   197,    -1,   197,   196,
      -1,   198,   205,   207,    -1,    16,   132,   139,    -1,    15,
     132,   139,   131,   199,    -1,    59,    -1,     3,    -1,    17,
     132,    11,   131,   297,    -1,    17,   132,    10,   131,     3,
      -1,    17,   132,    13,    -1,    17,   132,    12,   131,     3,
     131,     3,   131,     5,   131,     5,   131,   316,   131,   204,
     203,    -1,    -1,   131,   129,    -1,    -1,   131,   323,    -1,
      -1,   131,     3,    -1,     5,    -1,    59,    -1,    -1,    70,
     132,   206,    -1,    71,   131,   206,    -1,    71,    -1,    -1,
     209,   207,    -1,   137,   207,   138,    -1,   250,    -1,   200,
      -1,   305,    -1,   306,    -1,   292,    -1,   252,    -1,   215,
      -1,   214,    -1,   300,    -1,   264,    -1,   284,    -1,   308,
      -1,   309,    -1,   294,    -1,   307,    -1,   230,    -1,   240,
      -1,   242,    -1,   246,    -1,   244,    -1,   227,    -1,   237,
      -1,   223,    -1,   226,    -1,   287,    -1,   269,    -1,   285,
      -1,   272,    -1,   278,    -1,   301,    -1,   296,    -1,   290,
      -1,   251,    -1,   302,    -1,   257,    -1,   255,    -1,   295,
      -1,   299,    -1,   298,    -1,   288,    -1,   289,    -1,   291,
      -1,   283,    -1,   286,    -1,   149,    -1,   151,    -1,   153,
      -1,   155,    -1,   157,    -1,   159,    -1,   161,    -1,   163,
      -1,   165,    -1,   148,    -1,   150,    -1,   152,    -1,   154,
      -1,   156,    -1,   158,    -1,   160,    -1,   162,    -1,   164,
      -1,   210,    -1,   211,    -1,   142,    -1,   142,    -1,   211,
      -1,    82,   132,   210,    -1,   212,   190,   335,    -1,   212,
     190,   120,   132,   344,    -1,   212,   190,   334,    -1,   212,
     190,   350,   132,   328,    -1,   212,   190,   349,   132,   330,
      -1,   212,   190,   348,   132,   332,    -1,   212,   190,   323,
      -1,   212,   190,   326,    -1,   212,   190,   137,   221,   138,
      -1,   212,   190,   137,   220,   138,    -1,   212,   190,   137,
     219,   138,    -1,   212,   190,   350,   132,   137,   218,   138,
      -1,   212,   190,   349,   132,   137,   217,   138,    -1,   212,
     190,   348,   132,   137,   216,   138,    -1,   212,   190,   137,
     222,   138,    -1,   333,    -1,   216,   131,   333,    -1,   331,
      -1,   217,   131,   331,    -1,   329,    -1,   218,   131,   329,
      -1,   327,    -1,   219,   131,   327,    -1,   324,    -1,   220,
     131,   324,    -1,   335,    -1,   221,   131,   335,    -1,   334,
      -1,   222,   131,   334,    -1,    -1,    -1,   111,   141,   133,
     224,   339,   134,   225,   208,    -1,   141,   133,   342,   134,
      -1,    81,    -1,    -1,     6,    -1,     6,    -1,   135,   335,
     113,   335,   136,    -1,   135,   335,   136,    -1,    -1,    -1,
      88,   231,   135,   322,   136,   232,   137,   233,   138,    -1,
      -1,   234,   233,    -1,    -1,    89,   346,   132,   235,   207,
      -1,    -1,    91,   132,   236,   207,    -1,    90,    -1,   191,
     191,    -1,    87,    -1,    86,   213,   190,   335,   238,   335,
      -1,    -1,   239,   241,   208,    -1,    -1,    85,   135,   322,
     136,   243,   208,    -1,    -1,   229,   132,   245,   209,    -1,
      -1,    76,   229,   247,   248,    -1,   208,    -1,    -1,   208,
     249,    77,   208,    -1,    14,   132,   334,    -1,    53,    -1,
      53,   132,   346,    -1,    53,   132,    59,    -1,    46,   132,
     253,   131,   253,    -1,    46,   132,   253,   131,   346,    -1,
     133,     4,   131,    58,   131,   268,   134,    -1,   311,   228,
     131,   316,    -1,    -1,    68,   132,   254,   131,   261,   131,
     263,   312,   256,   208,    -1,    -1,    40,   132,   254,   131,
     260,   131,   262,   131,   263,   312,   258,   208,    -1,    -1,
     131,     5,    -1,   133,     4,   131,     4,   134,    -1,    59,
      -1,   133,     4,   131,     4,   134,    -1,    59,    -1,   133,
     270,   131,   271,   134,    -1,    59,    -1,   133,     4,   131,
       4,   134,    -1,    59,    -1,    75,   132,   265,   131,   315,
     131,   266,   131,   268,    -1,    24,   132,   315,   131,   344,
      -1,     5,    -1,    59,    -1,   267,    -1,    59,    -1,    58,
      -1,    58,   192,   267,    -1,     4,    -1,    59,    -1,    19,
      -1,    18,   132,   270,   131,   271,   259,   140,    -1,    18,
     132,   323,   259,   140,    -1,    63,    -1,    64,    -1,    65,
      -1,    64,    -1,    22,   132,   274,    -1,    -1,    22,   132,
     274,   273,   208,    -1,   330,   131,   323,   275,    -1,    -1,
     275,   131,   276,    -1,   334,    -1,    72,    -1,    73,    -1,
     318,    -1,    74,   334,    -1,    97,    -1,    95,    -1,    98,
      -1,    99,    -1,   100,    -1,   101,   132,   322,    -1,   102,
     132,   322,    -1,   103,   132,   322,    -1,   104,    -1,   105,
      -1,   106,   132,   277,    -1,   139,    -1,   107,    -1,   139,
     192,   277,    -1,    20,   132,   280,    -1,    -1,    21,   132,
     280,   279,   208,    -1,   332,   281,    -1,    -1,   281,   131,
     282,    -1,    56,    -1,   108,   132,   330,    -1,   347,    -1,
      69,   132,   334,    -1,    83,   132,   322,    -1,    84,    -1,
      55,    -1,    92,   132,   322,    -1,   110,    -1,    54,    -1,
      93,    -1,    94,   132,   322,    -1,    95,    -1,    96,    -1,
     323,    -1,    23,   132,   310,   131,   323,    -1,    25,   132,
     323,   131,    58,   131,   315,    -1,    29,   132,   323,   131,
      58,    -1,    29,   132,   323,   131,    58,   131,     5,   203,
      -1,    30,    -1,    30,   132,   344,    -1,    36,   132,   323,
     131,    67,    -1,    37,   132,   323,   131,    67,    -1,    37,
     132,   352,   131,   352,   131,    67,    -1,    41,   132,   352,
     131,   352,   131,   139,    -1,    42,   132,   352,   131,   352,
     293,    -1,    43,   132,   352,   131,   352,    -1,    -1,   131,
      67,    -1,    49,   132,   344,    -1,    51,   132,   344,    -1,
      50,   132,   344,    -1,     3,    -1,   133,     3,   131,   316,
     134,    -1,    80,   132,   326,   131,   328,   131,   328,   131,
       7,    -1,    78,   132,   344,   131,   328,    -1,    38,   132,
     326,    -1,    39,   132,   326,    -1,    -1,    31,   132,   326,
     131,   316,   131,   311,   312,   303,   304,    -1,    -1,   208,
      -1,    35,   132,   323,   131,   317,   131,   319,    -1,   109,
     132,   323,   131,   334,    -1,   109,   132,   323,   131,    59,
      -1,   109,   132,   323,    -1,    47,   132,   335,   131,   323,
      -1,    48,   132,   323,   131,   351,   131,   334,    -1,    45,
     132,   322,   131,   322,   131,   322,   131,   322,    -1,    45,
      -1,   139,    -1,    59,    -1,   139,    -1,    59,    -1,    -1,
     131,   313,    -1,   314,    -1,   314,   131,   313,    -1,    32,
      -1,    33,    -1,    34,    -1,    54,    -1,    59,    -1,    55,
      -1,    59,    -1,    62,    -1,   320,    -1,    59,    -1,    62,
      -1,   320,    -1,    61,   132,    59,    -1,    66,    -1,    59,
      -1,    61,   135,     4,   136,    -1,   139,    -1,   150,    -1,
     151,   135,   335,   136,    -1,   335,    -1,   324,    -1,   122,
     133,   344,   134,    -1,   154,    -1,   155,   135,   335,   136,
      -1,   133,     4,   131,     4,   134,    -1,    59,    -1,    60,
     325,   136,    -1,   130,    -1,   130,   131,   325,    -1,   327,
      -1,   156,    -1,   157,   135,   335,   136,    -1,   133,     4,
     131,     4,   131,     4,   131,     4,   134,    -1,   329,    -1,
     158,    -1,   159,   135,   335,   136,    -1,     3,    -1,   133,
       3,   131,   316,   134,    -1,   331,    -1,   160,    -1,   161,
     135,   335,   136,    -1,   139,    -1,     3,    -1,   133,     3,
     131,   139,   134,    -1,    59,    -1,   333,    -1,   162,    -1,
     163,   135,   335,   136,    -1,   139,    -1,     3,    -1,   133,
       3,   131,   139,   134,    -1,    59,    -1,   321,    -1,   334,
     191,   321,    -1,     4,    -1,   345,    -1,   133,     8,   134,
      -1,   148,    -1,   149,   135,   335,   136,    -1,   335,   185,
     335,    -1,   335,   186,   335,    -1,   335,   187,   335,    -1,
     335,   188,   335,    -1,   335,   189,   335,    -1,   133,   335,
     134,    -1,   144,    -1,   145,    -1,   212,   132,   336,    -1,
     337,    -1,   338,   131,   337,    -1,    -1,   338,    -1,   335,
      -1,   334,    -1,   340,    -1,   341,   131,   340,    -1,    -1,
     341,    -1,   323,    -1,   115,   326,    -1,   116,   326,    -1,
     117,   323,   131,   323,    -1,   118,   323,   131,   323,   131,
     335,    -1,   119,   133,   344,   134,    -1,   119,   133,   267,
     131,   344,   134,    -1,   125,   133,     7,   131,   344,   134,
      -1,   125,   133,   344,   131,   344,   134,    -1,   125,   133,
     328,   131,   344,   134,    -1,   121,   323,    -1,   123,   133,
     323,   131,   335,   134,    -1,   123,   133,   323,   131,   335,
     131,    32,   134,    -1,   124,   133,   323,   131,   335,   131,
     335,   134,    -1,   124,   133,   323,   131,   335,   131,   335,
     131,    32,   134,    -1,   127,   133,   128,   131,   133,   335,
     186,   335,   201,   134,   131,   323,   202,   134,    -1,   126,
     343,    -1,   164,    -1,   133,   344,   134,    -1,   343,    -1,
     343,   193,   344,    -1,   184,    -1,     8,    -1,     9,    -1,
       4,    -1,     8,    -1,     9,    -1,     4,    -1,   345,    -1,
      26,    -1,    20,    -1,    27,    -1,    22,    -1,    28,    -1,
      78,    -1,    57,    -1,    59,    -1,   353,    -1,    44,   133,
       4,   131,     4,   131,     4,   131,     4,   134,    -1,   133,
       4,   131,     4,   131,     4,   131,     4,   134,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   276,   276,   277,   280,   281,   284,   306,   311,   332,
     336,   342,   352,   363,   369,   398,   401,   408,   412,   419,
     422,   429,   430,   434,   437,   443,   447,   454,   457,   463,
     469,   470,   471,   472,   473,   474,   475,   476,   477,   478,
     479,   480,   481,   482,   483,   484,   485,   486,   487,   488,
     489,   490,   491,   492,   493,   494,   495,   496,   497,   498,
     499,   500,   501,   502,   503,   504,   505,   506,   507,   508,
     509,   510,   511,   512,   515,   516,   517,   518,   519,   520,
     521,   522,   523,   526,   527,   528,   529,   530,   531,   532,
     533,   534,   537,   538,   539,   542,   543,   546,   558,   564,
     570,   576,   582,   588,   594,   600,   606,   614,   622,   630,
     638,   646,   654,   664,   669,   676,   681,   688,   693,   700,
     704,   710,   715,   722,   726,   732,   736,   743,   765,   742,
     779,   827,   834,   837,   843,   850,   854,   863,   867,   862,
     929,   930,   934,   933,   946,   945,   960,   970,   971,   974,
    1013,  1012,  1046,  1045,  1075,  1074,  1105,  1104,  1130,  1139,
    1138,  1165,  1171,  1176,  1181,  1188,  1195,  1204,  1212,  1224,
    1223,  1242,  1241,  1260,  1263,  1269,  1279,  1285,  1294,  1300,
    1305,  1311,  1316,  1322,  1333,  1339,  1340,  1343,  1344,  1347,
    1351,  1357,  1358,  1361,  1368,  1376,  1384,  1385,  1388,  1389,
    1392,  1397,  1396,  1410,  1417,  1423,  1431,  1436,  1442,  1448,
    1454,  1460,  1465,  1470,  1475,  1480,  1485,  1490,  1495,  1500,
    1505,  1510,  1518,  1525,  1529,  1542,  1549,  1548,  1564,  1572,
    1578,  1586,  1592,  1597,  1602,  1607,  1612,  1617,  1622,  1627,
    1632,  1643,  1648,  1653,  1658,  1663,  1670,  1676,  1705,  1710,
    1718,  1724,  1730,  1737,  1744,  1754,  1764,  1779,  1790,  1793,
    1799,  1805,  1811,  1817,  1822,  1829,  1836,  1842,  1848,  1855,
    1854,  1879,  1882,  1888,  1895,  1899,  1904,  1911,  1917,  1924,
    1928,  1935,  1943,  1946,  1956,  1960,  1963,  1969,  1973,  1980,
    1984,  1988,  1994,  1995,  1998,  1999,  2002,  2003,  2004,  2010,
    2011,  2012,  2018,  2019,  2022,  2031,  2036,  2043,  2053,  2059,
    2063,  2067,  2074,  2083,  2089,  2093,  2099,  2103,  2111,  2115,
    2122,  2131,  2142,  2146,  2153,  2162,  2171,  2182,  2186,  2193,
    2202,  2211,  2220,  2229,  2235,  2239,  2246,  2255,  2265,  2274,
    2283,  2290,  2291,  2297,  2301,  2305,  2309,  2317,  2326,  2330,
    2334,  2338,  2342,  2346,  2349,  2356,  2365,  2393,  2394,  2397,
    2398,  2401,  2405,  2412,  2419,  2430,  2433,  2441,  2445,  2449,
    2453,  2457,  2462,  2466,  2470,  2475,  2480,  2485,  2489,  2494,
    2499,  2503,  2507,  2512,  2516,  2523,  2529,  2533,  2539,  2546,
    2547,  2548,  2551,  2555,  2559,  2563,  2569,  2570,  2573,  2574,
    2577,  2578,  2581,  2582,  2585,  2589,  2607
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "CHAR", "INTEGER", "BOOLEAN", "PERCENT",
  "SPERCENT", "MINUS_INTEGER", "PLUS_INTEGER", "MAZE_GRID_ID",
  "SOLID_FILL_ID", "MINES_ID", "ROGUELEV_ID", "MESSAGE_ID", "MAZE_ID",
  "LEVEL_ID", "LEV_INIT_ID", "GEOMETRY_ID", "NOMAP_ID", "OBJECT_ID",
  "COBJECT_ID", "MONSTER_ID", "TRAP_ID", "DOOR_ID", "DRAWBRIDGE_ID",
  "object_ID", "monster_ID", "terrain_ID", "MAZEWALK_ID", "WALLIFY_ID",
  "REGION_ID", "FILLING", "IRREGULAR", "JOINED", "ALTAR_ID", "LADDER_ID",
  "STAIR_ID", "NON_DIGGABLE_ID", "NON_PASSWALL_ID", "ROOM_ID", "PORTAL_ID",
  "TELEPRT_ID", "BRANCH_ID", "LEV", "MINERALIZE_ID", "CORRIDOR_ID",
  "GOLD_ID", "ENGRAVING_ID", "FOUNTAIN_ID", "POOL_ID", "SINK_ID", "NONE",
  "RAND_CORRIDOR_ID", "DOOR_STATE", "LIGHT_STATE", "CURSE_TYPE",
  "ENGRAVING_TYPE", "DIRECTION", "RANDOM_TYPE", "RANDOM_TYPE_BRACKET",
  "A_REGISTER", "ALIGNMENT", "LEFT_OR_RIGHT", "CENTER", "TOP_OR_BOT",
  "ALTAR_TYPE", "UP_OR_DOWN", "SUBROOM_ID", "NAME_ID", "FLAGS_ID",
  "FLAG_TYPE", "MON_ATTITUDE", "MON_ALERTNESS", "MON_APPEARANCE",
  "ROOMDOOR_ID", "IF_ID", "ELSE_ID", "TERRAIN_ID", "HORIZ_OR_VERT",
  "REPLACE_TERRAIN_ID", "EXIT_ID", "SHUFFLE_ID", "QUANTITY_ID",
  "BURIED_ID", "LOOP_ID", "FOR_ID", "TO_ID", "SWITCH_ID", "CASE_ID",
  "BREAK_ID", "DEFAULT_ID", "ERODED_ID", "TRAPPED_ID", "RECHARGED_ID",
  "INVIS_ID", "GREASED_ID", "FEMALE_ID", "CANCELLED_ID", "REVIVED_ID",
  "AVENGE_ID", "FLEEING_ID", "BLINDED_ID", "PARALYZED_ID", "STUNNED_ID",
  "CONFUSED_ID", "SEENTRAPS_ID", "ALL_ID", "MONTYPE_ID", "GRAVE_ID",
  "ERODEPROOF_ID", "FUNCTION_ID", "MSG_OUTPUT_TYPE", "COMPARE_TYPE",
  "UNKNOWN_TYPE", "rect_ID", "fillrect_ID", "line_ID", "randline_ID",
  "grow_ID", "selection_ID", "flood_ID", "rndcoord_ID", "circle_ID",
  "ellipse_ID", "filter_ID", "complement_ID", "gradient_ID",
  "GRADIENT_TYPE", "LIMITED", "HUMIDITY_TYPE", "','", "':'", "'('", "')'",
  "'['", "']'", "'{'", "'}'", "STRING", "MAP_ID", "NQSTRING", "VARSTRING",
  "CFUNC", "CFUNC_INT", "CFUNC_STR", "CFUNC_COORD", "CFUNC_REGION",
  "VARSTRING_INT", "VARSTRING_INT_ARRAY", "VARSTRING_STRING",
  "VARSTRING_STRING_ARRAY", "VARSTRING_VAR", "VARSTRING_VAR_ARRAY",
  "VARSTRING_COORD", "VARSTRING_COORD_ARRAY", "VARSTRING_REGION",
  "VARSTRING_REGION_ARRAY", "VARSTRING_MAPCHAR", "VARSTRING_MAPCHAR_ARRAY",
  "VARSTRING_MONST", "VARSTRING_MONST_ARRAY", "VARSTRING_OBJ",
  "VARSTRING_OBJ_ARRAY", "VARSTRING_SEL", "VARSTRING_SEL_ARRAY",
  "METHOD_INT", "METHOD_INT_ARRAY", "METHOD_STRING", "METHOD_STRING_ARRAY",
  "METHOD_VAR", "METHOD_VAR_ARRAY", "METHOD_COORD", "METHOD_COORD_ARRAY",
  "METHOD_REGION", "METHOD_REGION_ARRAY", "METHOD_MAPCHAR",
  "METHOD_MAPCHAR_ARRAY", "METHOD_MONST", "METHOD_MONST_ARRAY",
  "METHOD_OBJ", "METHOD_OBJ_ARRAY", "METHOD_SEL", "METHOD_SEL_ARRAY",
  "DICE", "'+'", "'-'", "'*'", "'/'", "'%'", "'='", "'.'", "'|'", "'&'",
  "$accept", "file", "levels", "level", "level_def", "mazefiller",
  "lev_init", "opt_limited", "opt_coord_or_var", "opt_fillchar", "walled",
  "flags", "flag_list", "levstatements", "stmt_block", "levstatement",
  "any_var_array", "any_var", "any_var_or_arr", "any_var_or_unk",
  "shuffle_detail", "variable_define", "encodeobj_list",
  "encodemonster_list", "mapchar_list", "encoderegion_list",
  "encodecoord_list", "integer_list", "string_list", "function_define",
  "$@1", "$@2", "function_call", "exitstatement", "opt_percent",
  "comparestmt", "switchstatement", "$@3", "$@4", "switchcases",
  "switchcase", "$@5", "$@6", "breakstatement", "for_to_span",
  "forstmt_start", "forstatement", "$@7", "loopstatement", "$@8",
  "chancestatement", "$@9", "ifstatement", "$@10", "if_ending", "$@11",
  "message", "random_corridors", "corridor", "corr_spec", "room_begin",
  "subroom_def", "$@12", "room_def", "$@13", "roomfill", "room_pos",
  "subroom_pos", "room_align", "room_size", "door_detail", "secret",
  "door_wall", "dir_list", "door_pos", "map_definition", "h_justif",
  "v_justif", "monster_detail", "$@14", "monster_desc", "monster_infos",
  "monster_info", "seen_trap_mask", "object_detail", "$@15", "object_desc",
  "object_infos", "object_info", "trap_detail", "drawbridge_detail",
  "mazewalk_detail", "wallify_detail", "ladder_detail", "stair_detail",
  "stair_region", "portal_region", "teleprt_region", "branch_region",
  "teleprt_detail", "fountain_detail", "sink_detail", "pool_detail",
  "terrain_type", "replace_terrain_detail", "terrain_detail",
  "diggable_detail", "passwall_detail", "region_detail", "@16",
  "region_detail_end", "altar_detail", "grave_detail", "gold_detail",
  "engraving_detail", "mineralize", "trap_name", "room_type",
  "optroomregionflags", "roomregionflags", "roomregionflag", "door_state",
  "light_state", "alignment", "alignment_prfx", "altar_type", "a_register",
  "string_or_var", "integer_or_var", "coord_or_var", "encodecoord",
  "humidity_flags", "region_or_var", "encoderegion", "mapchar_or_var",
  "mapchar", "monster_or_var", "encodemonster", "object_or_var",
  "encodeobj", "string_expr", "math_expr_var", "func_param_type",
  "func_param_part", "func_param_list", "func_params_list",
  "func_call_param_part", "func_call_param_list", "func_call_params_list",
  "ter_selection_x", "ter_selection", "dice", "all_integers",
  "all_ints_push", "objectid", "monsterid", "terrainid", "engraving_type",
  "lev_region", "region", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,    44,    58,    40,    41,    91,    93,   123,   125,   386,
     387,   388,   389,   390,   391,   392,   393,   394,   395,   396,
     397,   398,   399,   400,   401,   402,   403,   404,   405,   406,
     407,   408,   409,   410,   411,   412,   413,   414,   415,   416,
     417,   418,   419,   420,   421,   422,   423,   424,   425,   426,
     427,   428,   429,   430,   431,    43,    45,    42,    47,    37,
      61,    46,   124,    38
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   194,   195,   195,   196,   196,   197,   198,   198,   199,
     199,   200,   200,   200,   200,   201,   201,   202,   202,   203,
     203,   204,   204,   205,   205,   206,   206,   207,   207,   208,
     209,   209,   209,   209,   209,   209,   209,   209,   209,   209,
     209,   209,   209,   209,   209,   209,   209,   209,   209,   209,
     209,   209,   209,   209,   209,   209,   209,   209,   209,   209,
     209,   209,   209,   209,   209,   209,   209,   209,   209,   209,
     209,   209,   209,   209,   210,   210,   210,   210,   210,   210,
     210,   210,   210,   211,   211,   211,   211,   211,   211,   211,
     211,   211,   212,   212,   212,   213,   213,   214,   215,   215,
     215,   215,   215,   215,   215,   215,   215,   215,   215,   215,
     215,   215,   215,   216,   216,   217,   217,   218,   218,   219,
     219,   220,   220,   221,   221,   222,   222,   224,   225,   223,
     226,   227,   228,   228,   229,   229,   229,   231,   232,   230,
     233,   233,   235,   234,   236,   234,   237,   238,   238,   239,
     241,   240,   243,   242,   245,   244,   247,   246,   248,   249,
     248,   250,   251,   251,   251,   252,   252,   253,   254,   256,
     255,   258,   257,   259,   259,   260,   260,   261,   261,   262,
     262,   263,   263,   264,   264,   265,   265,   266,   266,   267,
     267,   268,   268,   269,   269,   269,   270,   270,   271,   271,
     272,   273,   272,   274,   275,   275,   276,   276,   276,   276,
     276,   276,   276,   276,   276,   276,   276,   276,   276,   276,
     276,   276,   277,   277,   277,   278,   279,   278,   280,   281,
     281,   282,   282,   282,   282,   282,   282,   282,   282,   282,
     282,   282,   282,   282,   282,   282,   283,   284,   285,   285,
     286,   286,   287,   288,   289,   290,   291,   292,   293,   293,
     294,   295,   296,   297,   297,   298,   299,   300,   301,   303,
     302,   304,   304,   305,   306,   306,   306,   307,   308,   309,
     309,   310,   310,   311,   311,   312,   312,   313,   313,   314,
     314,   314,   315,   315,   316,   316,   317,   317,   317,   318,
     318,   318,   319,   319,   320,   321,   321,   321,   322,   323,
     323,   323,   323,   324,   324,   324,   325,   325,   326,   326,
     326,   327,   328,   328,   328,   329,   329,   330,   330,   330,
     331,   331,   331,   331,   332,   332,   332,   333,   333,   333,
     333,   334,   334,   335,   335,   335,   335,   335,   335,   335,
     335,   335,   335,   335,   336,   336,   337,   338,   338,   339,
     339,   340,   340,   341,   341,   342,   342,   343,   343,   343,
     343,   343,   343,   343,   343,   343,   343,   343,   343,   343,
     343,   343,   343,   343,   343,   343,   344,   344,   345,   346,
     346,   346,   347,   347,   347,   347,   348,   348,   349,   349,
     350,   350,   351,   351,   352,   352,   353
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     1,     1,     2,     3,     3,     5,     1,
       1,     5,     5,     3,    16,     0,     2,     0,     2,     0,
       2,     1,     1,     0,     3,     3,     1,     0,     2,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     3,     3,     5,
       3,     5,     5,     5,     3,     3,     5,     5,     5,     7,
       7,     7,     5,     1,     3,     1,     3,     1,     3,     1,
       3,     1,     3,     1,     3,     1,     3,     0,     0,     8,
       4,     1,     0,     1,     1,     5,     3,     0,     0,     9,
       0,     2,     0,     5,     0,     4,     1,     2,     1,     6,
       0,     3,     0,     6,     0,     4,     0,     4,     1,     0,
       4,     3,     1,     3,     3,     5,     5,     7,     4,     0,
      10,     0,    12,     0,     2,     5,     1,     5,     1,     5,
       1,     5,     1,     9,     5,     1,     1,     1,     1,     1,
       3,     1,     1,     1,     7,     5,     1,     1,     1,     1,
       3,     0,     5,     4,     0,     3,     1,     1,     1,     1,
       2,     1,     1,     1,     1,     1,     3,     3,     3,     1,
       1,     3,     1,     1,     3,     3,     0,     5,     2,     0,
       3,     1,     3,     1,     3,     3,     1,     1,     3,     1,
       1,     1,     3,     1,     1,     1,     5,     7,     5,     8,
       1,     3,     5,     5,     7,     7,     6,     5,     0,     2,
       3,     3,     3,     1,     5,     9,     5,     3,     3,     0,
      10,     0,     1,     7,     5,     5,     3,     5,     7,     9,
       1,     1,     1,     1,     1,     0,     2,     1,     3,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     3,     1,     1,     4,     1,     1,     4,     1,     1,
       4,     1,     4,     5,     1,     3,     1,     3,     1,     1,
       4,     9,     1,     1,     4,     1,     5,     1,     1,     4,
       1,     1,     5,     1,     1,     1,     4,     1,     1,     5,
       1,     1,     3,     1,     1,     3,     1,     4,     3,     3,
       3,     3,     3,     3,     1,     1,     3,     1,     3,     0,
       1,     1,     1,     1,     3,     0,     1,     1,     2,     2,
       4,     6,     4,     6,     6,     6,     6,     2,     6,     8,
       8,    10,    14,     2,     1,     3,     1,     3,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,    10,     9
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       2,     0,     0,     0,     3,     4,    23,     0,     0,     1,
       5,     0,    27,     0,     7,     0,   134,     0,     0,     0,
     193,     0,     0,     0,     0,     0,     0,     0,   250,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   280,
       0,     0,     0,     0,     0,     0,   162,     0,     0,     0,
       0,     0,   131,     0,     0,     0,   137,   146,     0,     0,
       0,     0,    94,    83,    74,    84,    75,    85,    76,    86,
      77,    87,    78,    88,    79,    89,    80,    90,    81,    91,
      82,    31,     6,    27,    92,    93,     0,    37,    36,    52,
      53,    50,     0,    45,    51,   150,    46,    47,    49,    48,
      30,    62,    35,    65,    64,    39,    55,    57,    58,    72,
      40,    56,    73,    54,    69,    70,    61,    71,    34,    43,
      66,    60,    68,    67,    38,    59,    63,    32,    33,    44,
      41,    42,     0,    26,    24,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   156,     0,     0,     0,
       0,    95,    96,     0,     0,     0,     0,   343,     0,   346,
       0,   388,     0,   344,   365,    28,     0,   154,     0,    10,
       9,     8,     0,   305,   306,     0,   341,   161,     0,     0,
       0,    13,   314,     0,   196,   197,     0,     0,   311,     0,
       0,   173,   309,   338,   340,     0,   337,   335,     0,   225,
     229,   334,   226,   331,   333,     0,   330,   328,     0,   200,
       0,   327,   282,   281,     0,   292,   293,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   384,   367,   386,   251,     0,   319,     0,     0,
     318,     0,     0,     0,     0,     0,     0,   404,   267,   268,
     284,   283,     0,   132,     0,     0,     0,     0,     0,   308,
       0,     0,     0,     0,   260,   262,   261,   391,   389,   390,
     164,   163,     0,   185,   186,     0,     0,     0,     0,    97,
       0,     0,     0,   276,   127,     0,     0,     0,     0,   136,
       0,     0,     0,     0,     0,   362,   361,   363,   366,     0,
     397,   399,   396,   398,   400,   401,     0,     0,     0,   104,
     105,   100,    98,     0,     0,     0,     0,    27,   151,    25,
       0,     0,     0,     0,     0,   316,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   228,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   368,   369,     0,     0,     0,
     377,     0,     0,     0,   383,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   133,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   158,   157,     0,     0,   152,     0,     0,     0,   359,
     345,   353,     0,     0,   348,   349,   350,   351,   352,     0,
     130,     0,   343,     0,     0,     0,     0,   121,   119,   125,
     123,     0,     0,     0,   155,     0,     0,   342,    12,   263,
       0,    11,     0,     0,   315,     0,     0,     0,   199,   198,
     173,   174,   195,     0,     0,     0,   227,     0,     0,   202,
     204,   246,   184,     0,   248,     0,     0,   189,     0,     0,
       0,     0,   325,     0,     0,   323,     0,     0,   322,     0,
       0,   385,   387,     0,     0,   294,   295,     0,   298,     0,
     296,     0,   297,   252,     0,     0,   253,     0,   176,     0,
       0,     0,     0,     0,   258,   257,     0,     0,   165,   166,
     277,   402,   403,     0,   178,     0,     0,     0,     0,     0,
     266,     0,     0,   148,     0,     0,   138,   275,   274,     0,
     357,   360,     0,   347,   135,   364,    99,     0,     0,   108,
       0,   107,     0,   106,     0,   112,     0,   103,     0,   102,
       0,   101,    29,   307,     0,     0,   317,   310,     0,   312,
       0,     0,   336,   394,   392,   393,   240,   237,   231,     0,
       0,   236,     0,   241,     0,   243,   244,     0,   239,   230,
     245,   395,   233,     0,   329,   203,     0,     0,   370,     0,
       0,     0,   372,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   320,     0,     0,     0,     0,     0,     0,     0,
       0,   168,     0,     0,     0,   256,     0,     0,     0,     0,
       0,     0,     0,     0,   153,   147,   149,     0,     0,     0,
     128,     0,   120,   122,   124,   126,     0,   113,     0,   115,
       0,   117,     0,     0,   313,   194,   339,     0,     0,     0,
       0,     0,   332,     0,   247,    19,     0,   190,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   285,     0,
     303,   302,   273,     0,     0,   254,     0,   180,     0,     0,
     255,   259,     0,     0,   278,     0,   182,     0,   285,   188,
       0,   187,   160,     0,   140,   354,   355,   356,   358,     0,
       0,   111,     0,   110,     0,   109,     0,     0,   234,   235,
     238,   242,   232,     0,   299,   207,   208,     0,   212,   211,
     213,   214,   215,     0,     0,     0,   219,   220,     0,   205,
     209,   300,   206,     0,   249,   371,   373,     0,   378,     0,
     374,     0,   324,   376,   375,     0,     0,     0,   269,   304,
       0,     0,     0,     0,     0,     0,   191,   192,     0,     0,
       0,   169,     0,     0,     0,     0,     0,   140,   129,   114,
     116,   118,   264,     0,     0,   210,     0,     0,     0,     0,
      20,     0,     0,   326,     0,     0,   289,   290,   291,   286,
     287,   271,     0,     0,   175,     0,   285,   279,   167,   177,
       0,     0,   183,   265,     0,   144,   139,   141,     0,   301,
     216,   217,   218,   223,   222,   221,   379,     0,   380,   349,
       0,     0,   272,   270,     0,     0,     0,   171,     0,   170,
     142,    27,     0,     0,     0,     0,     0,   321,   288,     0,
     406,   179,     0,   181,    27,   145,     0,   224,   381,    16,
       0,   405,   172,   143,     0,     0,     0,    17,    21,    22,
      19,     0,     0,    14,    18,   382
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     3,     4,     5,     6,   191,    81,   836,   862,   734,
     860,    12,   134,    82,   338,    83,    84,    85,    86,   173,
      87,    88,   636,   638,   640,   423,   424,   425,   426,    89,
     409,   699,    90,    91,   389,    92,    93,   174,   627,   766,
     767,   844,   831,    94,   525,    95,    96,   188,    97,   522,
      98,   336,    99,   296,   402,   518,   100,   101,   102,   281,
     272,   103,   801,   104,   842,   352,   500,   516,   679,   688,
     105,   295,   690,   468,   758,   106,   210,   450,   107,   359,
     229,   585,   729,   815,   108,   356,   219,   355,   579,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   615,
     119,   120,   121,   441,   122,   123,   124,   125,   126,   791,
     823,   127,   128,   129,   130,   131,   234,   273,   748,   789,
     790,   237,   487,   491,   730,   672,   492,   196,   278,   253,
     212,   346,   259,   260,   477,   478,   230,   231,   220,   221,
     315,   279,   697,   530,   531,   532,   317,   318,   319,   254,
     376,   183,   291,   582,   333,   334,   335,   513,   266,   267
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -654
static const yytype_int16 yypact[] =
{
     137,     7,    27,    83,  -654,   137,    24,   -15,    37,  -654,
    -654,    91,   733,   -10,  -654,   113,  -654,    99,   131,   133,
    -654,   161,   164,   166,   174,   183,   199,   214,   231,   233,
     239,   241,   243,   245,   247,   250,   268,   269,   275,   276,
     281,   295,   305,   317,   322,   323,   325,   326,   327,    28,
     340,   343,  -654,   345,   205,   757,  -654,  -654,   346,    49,
      66,   265,  -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,
    -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,
    -654,  -654,  -654,   733,  -654,  -654,   190,  -654,  -654,  -654,
    -654,  -654,   353,  -654,  -654,  -654,  -654,  -654,  -654,  -654,
    -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,
    -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,
    -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,
    -654,  -654,    59,   308,  -654,   -31,   406,   351,    58,    58,
     144,   -11,    57,   -18,   -18,   672,   -57,   -18,   -18,   248,
     -57,   -57,    -6,    -1,    -1,    -1,    66,   303,    66,   -18,
     672,   672,   672,   319,    -6,    51,  -654,   672,   -57,   751,
      66,  -654,  -654,   279,   339,   -18,   355,  -654,    29,  -654,
     344,  -654,   198,  -654,    56,  -654,    18,  -654,   359,  -654,
    -654,  -654,   113,  -654,  -654,   360,  -654,   309,   368,   371,
     372,  -654,  -654,   374,  -654,  -654,   375,   503,  -654,   378,
     379,   383,  -654,  -654,  -654,   506,  -654,  -654,   402,  -654,
    -654,  -654,  -654,  -654,  -654,   537,  -654,  -654,   413,   404,
     418,  -654,  -654,  -654,   420,  -654,  -654,   427,   439,   446,
     -57,   -57,   -18,   -18,   417,   -18,   436,   445,   449,   672,
     450,   508,  -654,  -654,   391,  -654,   582,  -654,   453,   458,
    -654,   459,   460,   466,   599,   473,   474,  -654,  -654,  -654,
    -654,  -654,   475,   601,   608,   482,   483,   489,   490,   387,
     618,   526,   208,   529,  -654,  -654,  -654,  -654,  -654,  -654,
    -654,  -654,   530,  -654,  -654,   534,   359,   538,   539,  -654,
     523,    66,    66,   540,  -654,   548,   342,    66,    66,  -654,
      66,    66,    66,    66,    66,   309,   387,  -654,   542,   549,
    -654,  -654,  -654,  -654,  -654,  -654,   554,    60,    32,  -654,
    -654,   309,   387,   555,   556,   558,   733,   733,  -654,  -654,
      66,   -31,   671,    46,   698,   571,   567,   672,   573,    66,
     215,   700,   566,   581,    66,   587,   359,   589,    66,   359,
     -18,   -18,   672,   663,   664,  -654,  -654,   592,   593,   521,
    -654,   -18,   -18,   307,  -654,   598,   596,   672,   597,    66,
      67,   186,   660,   730,   604,   669,    -1,     8,  -654,   606,
     607,    -1,    -1,    -1,    66,   609,    89,   -18,   141,    12,
      57,   665,  -654,    52,    52,  -654,   156,   605,   -38,   697,
    -654,  -654,   331,   347,   168,   168,  -654,  -654,  -654,    56,
    -654,   672,   612,   -52,   -50,     4,   140,  -654,  -654,   309,
     387,    55,   127,   158,  -654,   611,   358,  -654,  -654,  -654,
     743,  -654,   628,   374,  -654,   626,   761,   430,  -654,  -654,
     383,  -654,  -654,   627,   464,   266,  -654,   638,   492,  -654,
    -654,  -654,  -654,   636,   654,   -18,   -18,   600,   673,   666,
     675,   676,  -654,   679,   438,  -654,   667,   681,  -654,   685,
     686,  -654,  -654,   799,   522,  -654,  -654,   689,  -654,   687,
    -654,   693,  -654,  -654,   694,   824,  -654,   699,  -654,   825,
     701,    67,   827,   702,   703,  -654,   704,   779,  -654,  -654,
    -654,  -654,  -654,   707,  -654,   836,   710,   712,   786,   861,
    -654,   734,   359,  -654,   678,    66,  -654,  -654,   309,   735,
    -654,   739,   732,  -654,  -654,  -654,  -654,   867,   740,  -654,
      -8,  -654,    66,  -654,   -31,  -654,    21,  -654,    25,  -654,
      54,  -654,  -654,  -654,   741,   873,  -654,  -654,   744,  -654,
     737,   745,  -654,  -654,  -654,  -654,  -654,  -654,  -654,   748,
     769,  -654,   771,  -654,   788,  -654,  -654,   790,  -654,  -654,
    -654,  -654,  -654,   784,  -654,   792,    57,   919,  -654,   794,
     868,   672,  -654,    66,    66,   672,   796,    66,   672,   672,
     795,   798,  -654,    -6,   926,    90,   927,   -49,   865,   802,
      13,  -654,   803,   797,   870,  -654,    66,   804,   -31,   807,
      15,   254,   359,    52,  -654,  -654,   387,   805,   224,   697,
    -654,   -29,  -654,  -654,   387,   309,   151,  -654,   159,  -654,
     171,  -654,    67,   808,  -654,  -654,  -654,   -31,    66,    66,
      66,   144,  -654,   594,  -654,   809,    66,  -654,   810,   258,
     337,   811,    67,   528,   812,   813,    66,   937,   817,   814,
    -654,  -654,  -654,   818,   939,  -654,   947,  -654,   289,   821,
    -654,  -654,   822,    85,   309,   950,  -654,   951,   817,  -654,
     826,  -654,  -654,   828,   160,  -654,  -654,  -654,  -654,   359,
      21,  -654,    25,  -654,    54,  -654,   829,   953,   309,  -654,
    -654,  -654,  -654,   149,  -654,  -654,  -654,   -31,  -654,  -654,
    -654,  -654,  -654,   830,   832,   833,  -654,  -654,   834,  -654,
    -654,  -654,   309,   957,  -654,   387,  -654,   924,  -654,    66,
    -654,   835,  -654,  -654,  -654,   409,   837,   419,  -654,  -654,
     963,   839,   838,   840,    15,    66,  -654,  -654,   841,   842,
     843,  -654,    85,   954,   329,   845,   844,   160,  -654,  -654,
    -654,  -654,  -654,   847,   914,   309,    66,    66,    66,   -44,
    -654,   846,   304,  -654,    66,   975,  -654,  -654,  -654,  -654,
     850,   359,   852,   980,  -654,   215,   817,  -654,  -654,  -654,
     981,   359,  -654,  -654,   854,  -654,  -654,  -654,   982,  -654,
    -654,  -654,  -654,  -654,   800,  -654,  -654,   956,  -654,   217,
     855,   419,  -654,  -654,   986,   857,   859,  -654,   860,  -654,
    -654,   733,   864,   -44,   862,   869,   863,  -654,  -654,   866,
    -654,  -654,   359,  -654,   733,  -654,    67,  -654,  -654,  -654,
     871,  -654,  -654,  -654,   872,   -18,    68,   874,  -654,  -654,
     809,   -18,   875,  -654,  -654,  -654
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -654,  -654,   994,  -654,  -654,  -654,  -654,  -654,  -654,   146,
    -654,  -654,   815,   -83,  -290,   668,   848,   946,  -390,  -654,
    -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,
    -654,  -654,  -654,  -654,  -654,   959,  -654,  -654,  -654,   244,
    -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,
    -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,   614,
     849,  -654,  -654,  -654,  -654,   562,  -654,  -654,  -654,   260,
    -654,  -654,  -654,  -531,   253,  -654,   338,   223,  -654,  -654,
    -654,  -654,  -654,   187,  -654,  -654,   880,  -654,  -654,  -654,
    -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,
    -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,  -654,
    -654,  -654,  -654,  -654,  -654,  -654,  -654,   421,  -653,   200,
    -654,  -385,  -492,  -654,  -654,  -654,   369,   682,  -168,  -136,
    -312,   583,   150,  -308,  -386,  -485,  -418,  -473,   602,  -459,
    -132,   -55,  -654,   396,  -654,  -654,   610,  -654,  -654,   778,
    -135,   575,  -392,  -654,  -654,  -654,  -654,  -654,  -124,  -654
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -202
static const yytype_int16 yytable[] =
{
     185,   211,   300,   197,   509,   182,   401,   238,   239,   611,
     255,   261,   262,   265,   549,   517,   427,   520,   521,   529,
     428,   527,   177,   283,   213,   284,   285,   286,   223,   275,
     276,   277,   297,   177,    16,   761,   177,   305,   320,   303,
     321,   202,   203,   263,   322,   323,   324,   551,   232,   439,
     329,   202,   203,   270,   331,   472,   293,   472,   213,   657,
     177,   213,   189,   813,   422,   641,   456,   498,   305,   459,
     177,   514,   677,   858,   686,   639,   256,   202,   203,   538,
     214,   540,   674,     9,   224,   644,   539,   637,   541,   756,
     691,   202,   203,   287,    11,   814,   325,   288,   289,   257,
     258,   193,   667,   282,   206,   644,   367,   368,   193,   370,
     294,   235,   194,   195,   214,   207,   236,   214,   190,   194,
     195,   132,   485,   306,    13,   207,   486,   859,   233,   316,
     223,   332,   274,   271,   407,   542,   208,   209,   326,     7,
     206,   499,   543,   827,   757,   515,   678,   223,   687,   670,
     706,   327,     1,     2,   215,   328,   671,   193,   225,     8,
     216,   472,   178,    60,   226,   327,   179,   180,   194,   195,
     741,   193,   208,   209,   257,   258,    14,   179,   180,   440,
     179,   180,   194,   195,   133,   519,   224,   519,   215,   178,
     176,   215,   546,   178,   216,   193,   429,   216,   511,   178,
     512,   654,   181,   224,   179,   180,   194,   195,   179,   180,
     475,   476,   445,   181,   179,   180,   181,   217,   218,   771,
     217,   218,   280,    15,   460,   461,   506,   462,   633,   770,
     632,   135,   624,   712,   469,   470,   471,   693,   479,   529,
     181,   769,   482,   523,   181,   488,   406,   489,   490,   764,
     181,   765,   412,   413,   435,   414,   415,   416,   417,   418,
     225,   510,   497,   136,   548,   137,   226,   503,   504,   505,
     563,   544,   306,   430,   564,   565,   528,   225,   545,   448,
     449,   774,   700,   226,   604,   436,   536,   227,   228,   701,
     702,   519,   263,   138,   447,   550,   139,   703,   140,   454,
     268,   269,   704,   458,   227,   228,   141,   202,   203,   705,
     472,   308,   467,   689,   473,   142,   475,   476,   298,   580,
     566,   567,   568,   287,   484,   202,   203,   288,   289,   588,
     589,   143,   692,   287,   309,   569,   330,   288,   289,   397,
     170,   310,   311,   312,   313,   314,   144,   524,   835,   570,
     571,   -15,   204,   205,   854,   312,   313,   314,   572,   573,
     574,   575,   576,   145,   316,   146,   202,   203,   695,   696,
     206,   147,   804,   148,   577,   149,   578,   150,   290,   151,
     186,   264,   152,   310,   311,   312,   313,   314,   206,   737,
     365,   366,   738,   310,   311,   312,   313,   314,   184,   207,
     153,   154,   208,   209,   312,   313,   314,   155,   156,   768,
     202,   203,   635,   157,   204,   205,   198,   199,   200,   201,
     208,   209,   240,   241,   242,   243,   244,   158,   245,   206,
     246,   247,   248,   249,   250,   817,   280,   159,   818,   192,
     474,   596,   348,   310,   311,   312,   313,   314,   682,   160,
     181,   786,   787,   788,   161,   162,   658,   163,   164,   165,
     661,   208,   209,   664,   665,   475,   476,   533,   739,   301,
     626,   252,   167,   206,   302,   168,   411,   169,   175,   307,
     709,   710,   711,   534,   207,   187,   684,   634,   304,   310,
     311,   312,   313,   314,   553,   340,   337,   202,   203,   342,
     341,   822,   343,   344,   345,   208,   209,   348,   347,   353,
     350,   829,   348,   349,   351,   708,   310,   311,   312,   313,
     314,   732,   310,   311,   312,   313,   314,   310,   311,   312,
     313,   314,   310,   311,   312,   313,   314,   354,   659,   660,
     357,  -201,   663,   310,   311,   312,   313,   314,   358,   360,
     369,   361,   852,   240,   241,   242,   243,   244,   362,   245,
     206,   246,   247,   248,   249,   250,   559,   202,   203,   371,
     363,   251,   310,   311,   312,   313,   314,   364,   372,   467,
     202,   203,   373,   375,   377,   775,   378,   797,   379,   380,
     381,   382,   208,   209,   310,   784,   312,   313,   314,   383,
     562,   735,   252,   384,   385,   386,   387,   388,   810,   811,
     812,   745,   390,   391,   392,   310,   311,   312,   313,   314,
     393,   394,   395,   240,   241,   242,   243,   244,   584,   245,
     206,   246,   247,   248,   249,   250,   240,   241,   242,   243,
     244,   251,   245,   206,   246,   247,   248,   249,   250,   310,
     311,   312,   313,   314,   251,   713,   714,   396,   602,   405,
     398,   399,   208,   209,   742,   400,   715,   716,   717,   403,
     404,   408,   252,   419,   438,   208,   209,   310,   311,   312,
     313,   314,   410,   420,   782,   252,   421,   431,   432,   718,
     433,   719,   720,   721,   722,   723,   724,   725,   726,   727,
     728,   442,   443,   444,   446,   451,   452,   310,   311,   312,
     313,   314,   453,   310,   311,   312,   313,   314,   455,   857,
     457,   463,   464,   465,   466,   864,   480,   493,   483,   819,
     481,   202,   203,   193,   494,   495,   496,   501,   502,    16,
     507,   526,  -159,   537,   194,   195,   554,    17,   845,   552,
      18,    19,    20,    21,    22,    23,    24,    25,    26,   555,
     557,   853,    27,    28,    29,   558,   561,   586,    30,    31,
      32,    33,    34,    35,    36,    37,    38,   583,    39,    40,
      41,    42,    43,    44,    45,   587,    46,   240,   241,   242,
     243,   244,   590,   245,   206,   246,   247,   248,   249,   250,
     592,    47,   597,   601,   591,   251,   593,   594,    48,    49,
     595,    50,   598,    51,    52,    53,   599,   600,    54,    55,
     603,    56,   604,    57,   605,   606,   208,   209,   607,   609,
     608,   612,   610,   613,   614,   616,   252,   617,   618,    62,
     619,   620,    58,   621,    59,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,   622,   596,   623,   630,   628,    60,   625,
     629,   631,   642,   256,    61,    62,   643,   645,   644,   646,
     647,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,   171,
      64,   648,    66,   649,    68,    63,    70,    65,    72,    67,
      74,    69,    76,    71,    78,    73,    80,    75,   652,    77,
     650,    79,   651,   653,   655,   656,   467,   662,   666,   667,
     669,   673,   675,   676,   674,   683,   680,   681,   685,   707,
     733,   746,   694,   751,   736,   740,   743,   744,   747,   750,
     749,   752,   754,   755,   759,   760,   781,   762,   773,   763,
     780,   803,   776,   772,   777,   778,   779,   792,   785,   783,
     793,   795,   794,   809,   800,   798,   799,   805,   808,   820,
     816,   821,   806,   824,   825,   828,   830,   832,   834,   837,
     839,   840,   833,   841,   843,   846,   848,   850,   849,    10,
     851,   172,   855,   856,   434,   861,   863,   339,   166,   865,
     508,   807,   560,   292,   796,   802,   753,   299,   826,   222,
     847,   838,   731,   437,   668,   698,   556,   374,     0,   535,
     581,     0,     0,   547
};

static const yytype_int16 yycheck[] =
{
      83,   137,   170,   135,   396,    60,   296,   143,   144,   501,
     145,   147,   148,   149,   432,   400,   328,   403,   404,   409,
     328,    59,     4,   159,     3,   160,   161,   162,     3,   153,
     154,   155,   167,     4,     6,   688,     4,     8,    20,   175,
      22,    59,    60,    44,    26,    27,    28,   433,    59,     3,
     186,    59,    60,    59,   186,     3,     5,     3,     3,   590,
       4,     3,     3,   107,     4,   550,   356,    59,     8,   359,
       4,    59,    59,     5,    59,   548,   133,    59,    60,   131,
      59,   131,   131,     0,    59,   134,   138,   546,   138,     4,
     621,    59,    60,     4,    70,   139,    78,     8,     9,   156,
     157,   139,   131,   158,   122,   134,   242,   243,   139,   245,
      59,    54,   150,   151,    59,   133,    59,    59,    59,   150,
     151,   131,    55,   178,   139,   133,    59,    59,   139,   184,
       3,   186,   133,   139,   302,   131,   154,   155,   120,   132,
     122,   133,   138,   796,    59,   133,   133,     3,   133,    59,
     642,   133,    15,    16,   133,   137,    66,   139,   133,   132,
     139,     3,   133,   135,   139,   133,   148,   149,   150,   151,
     662,   139,   154,   155,   156,   157,   139,   148,   149,   133,
     148,   149,   150,   151,    71,   133,    59,   133,   133,   133,
     141,   133,   137,   133,   139,   139,   328,   139,    57,   133,
      59,   586,   184,    59,   148,   149,   150,   151,   148,   149,
     158,   159,   347,   184,   148,   149,   184,   162,   163,   704,
     162,   163,   133,   132,   360,   361,   394,   362,   540,   702,
     538,   132,   522,   651,   369,   371,   372,   623,   373,   629,
     184,   700,   377,    87,   184,    59,   301,    61,    62,    89,
     184,    91,   307,   308,   337,   310,   311,   312,   313,   314,
     133,   397,   386,   132,   137,   132,   139,   391,   392,   393,
       4,   131,   327,   328,     8,     9,   408,   133,   138,    64,
      65,   132,   131,   139,   135,   340,   421,   160,   161,   138,
     131,   133,    44,   132,   349,   137,   132,   138,   132,   354,
     150,   151,   131,   358,   160,   161,   132,    59,    60,   138,
       3,   113,    58,    59,     7,   132,   158,   159,   168,   455,
      54,    55,    56,     4,   379,    59,    60,     8,     9,   465,
     466,   132,   622,     4,   136,    69,   186,     8,     9,   131,
     135,   185,   186,   187,   188,   189,   132,   191,   131,    83,
      84,   134,    63,    64,   846,   187,   188,   189,    92,    93,
      94,    95,    96,   132,   419,   132,    59,    60,   144,   145,
     122,   132,   764,   132,   108,   132,   110,   132,    59,   132,
     190,   133,   132,   185,   186,   187,   188,   189,   122,   131,
     240,   241,   134,   185,   186,   187,   188,   189,   133,   133,
     132,   132,   154,   155,   187,   188,   189,   132,   132,   699,
      59,    60,   544,   132,    63,    64,    10,    11,    12,    13,
     154,   155,   115,   116,   117,   118,   119,   132,   121,   122,
     123,   124,   125,   126,   127,   131,   133,   132,   134,   131,
     133,     3,     4,   185,   186,   187,   188,   189,   616,   132,
     184,    32,    33,    34,   132,   132,   591,   132,   132,   132,
     595,   154,   155,   598,   599,   158,   159,   136,   131,   190,
     525,   164,   132,   122,   135,   132,   134,   132,   132,   135,
     648,   649,   650,   136,   133,   132,   618,   542,   133,   185,
     186,   187,   188,   189,   136,   135,   137,    59,    60,   131,
     191,   791,   131,   131,   130,   154,   155,     4,   133,     3,
     131,   801,     4,   135,   131,   647,   185,   186,   187,   188,
     189,   653,   185,   186,   187,   188,   189,   185,   186,   187,
     188,   189,   185,   186,   187,   188,   189,   135,   593,   594,
       3,   137,   597,   185,   186,   187,   188,   189,   135,   131,
     133,   131,   842,   115,   116,   117,   118,   119,   131,   121,
     122,   123,   124,   125,   126,   127,   136,    59,    60,   133,
     131,   133,   185,   186,   187,   188,   189,   131,   133,    58,
      59,    60,   133,   133,   193,   717,     4,   755,   135,   131,
     131,   131,   154,   155,   185,   186,   187,   188,   189,   133,
     136,   656,   164,     4,   131,   131,   131,     6,   776,   777,
     778,   666,     4,   131,   131,   185,   186,   187,   188,   189,
     131,   131,     4,   115,   116,   117,   118,   119,   136,   121,
     122,   123,   124,   125,   126,   127,   115,   116,   117,   118,
     119,   133,   121,   122,   123,   124,   125,   126,   127,   185,
     186,   187,   188,   189,   133,    61,    62,   131,   136,   136,
     131,   131,   154,   155,   136,   131,    72,    73,    74,   131,
     131,   131,   164,   131,     3,   154,   155,   185,   186,   187,
     188,   189,   134,   134,   739,   164,   132,   132,   132,    95,
     132,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,     3,   131,   136,   131,     5,   140,   185,   186,   187,
     188,   189,   131,   185,   186,   187,   188,   189,   131,   855,
     131,    58,    58,   131,   131,   861,   128,    67,   131,   784,
     134,    59,    60,   139,     4,   131,    67,   131,   131,     6,
     131,   136,    77,   131,   150,   151,     3,    14,   831,   138,
      17,    18,    19,    20,    21,    22,    23,    24,    25,   131,
     134,   844,    29,    30,    31,     4,   139,   131,    35,    36,
      37,    38,    39,    40,    41,    42,    43,   139,    45,    46,
      47,    48,    49,    50,    51,   131,    53,   115,   116,   117,
     118,   119,   192,   121,   122,   123,   124,   125,   126,   127,
     134,    68,   135,     4,   131,   133,   131,   131,    75,    76,
     131,    78,   131,    80,    81,    82,   131,   131,    85,    86,
     131,    88,   135,    90,   131,   131,   154,   155,     4,     4,
     131,     4,   131,   131,   131,   131,   164,    58,   131,   142,
       4,   131,   109,   131,   111,   148,   149,   150,   151,   152,
     153,   154,   155,   156,   157,   158,   159,   160,   161,   162,
     163,   164,   165,    77,     3,   131,   134,   132,   135,   191,
     131,     4,   131,   133,   141,   142,     3,   140,   134,   134,
     132,   148,   149,   150,   151,   152,   153,   154,   155,   156,
     157,   158,   159,   160,   161,   162,   163,   164,   165,   142,
     149,   132,   151,   132,   153,   148,   155,   150,   157,   152,
     159,   154,   161,   156,   163,   158,   165,   160,   134,   162,
     132,   164,   132,   131,     5,   131,    58,   131,   133,   131,
       4,     4,    67,   131,   131,   131,   139,    67,   131,   131,
     131,     4,   137,     4,   134,   134,   134,   134,   131,   131,
     136,     4,   131,   131,     4,     4,    32,   131,     5,   131,
       3,     7,   132,   134,   132,   132,   132,     4,   131,   134,
     131,   131,   134,    59,   131,   134,   134,   132,   131,     4,
     134,   131,   138,   131,     4,     4,   132,     5,    32,   134,
       4,   134,   192,   134,   134,   131,   134,   134,   129,     5,
     134,    55,   131,   131,   336,   131,   860,   192,    49,   134,
     396,   767,   450,   164,   754,   762,   678,   169,   795,   139,
     833,   821,   653,   341,   603,   629,   443,   249,    -1,   419,
     455,    -1,    -1,   431
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,    15,    16,   195,   196,   197,   198,   132,   132,     0,
     196,    70,   205,   139,   139,   132,     6,    14,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    29,    30,    31,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    45,
      46,    47,    48,    49,    50,    51,    53,    68,    75,    76,
      78,    80,    81,    82,    85,    86,    88,    90,   109,   111,
     135,   141,   142,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   200,   207,   209,   210,   211,   212,   214,   215,   223,
     226,   227,   229,   230,   237,   239,   240,   242,   244,   246,
     250,   251,   252,   255,   257,   264,   269,   272,   278,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   294,
     295,   296,   298,   299,   300,   301,   302,   305,   306,   307,
     308,   309,   131,    71,   206,   132,   132,   132,   132,   132,
     132,   132,   132,   132,   132,   132,   132,   132,   132,   132,
     132,   132,   132,   132,   132,   132,   132,   132,   132,   132,
     132,   132,   132,   132,   132,   132,   229,   132,   132,   132,
     135,   142,   211,   213,   231,   132,   141,     4,   133,   148,
     149,   184,   335,   345,   133,   207,   190,   132,   241,     3,
      59,   199,   131,   139,   150,   151,   321,   334,    10,    11,
      12,    13,    59,    60,    63,    64,   122,   133,   154,   155,
     270,   323,   324,     3,    59,   133,   139,   162,   163,   280,
     332,   333,   280,     3,    59,   133,   139,   160,   161,   274,
     330,   331,    59,   139,   310,    54,    59,   315,   323,   323,
     115,   116,   117,   118,   119,   121,   123,   124,   125,   126,
     127,   133,   164,   323,   343,   344,   133,   156,   157,   326,
     327,   323,   323,    44,   133,   323,   352,   353,   326,   326,
      59,   139,   254,   311,   133,   352,   352,   352,   322,   335,
     133,   253,   335,   323,   344,   344,   344,     4,     8,     9,
      59,   346,   254,     5,    59,   265,   247,   344,   326,   210,
     322,   190,   135,   323,   133,     8,   335,   135,   113,   136,
     185,   186,   187,   188,   189,   334,   335,   340,   341,   342,
      20,    22,    26,    27,    28,    78,   120,   133,   137,   323,
     326,   334,   335,   348,   349,   350,   245,   137,   208,   206,
     135,   191,   131,   131,   131,   130,   325,   133,     4,   135,
     131,   131,   259,     3,   135,   281,   279,     3,   135,   273,
     131,   131,   131,   131,   131,   326,   326,   323,   323,   133,
     323,   133,   133,   133,   343,   133,   344,   193,     4,   135,
     131,   131,   131,   133,     4,   131,   131,   131,     6,   228,
       4,   131,   131,   131,   131,     4,   131,   131,   131,   131,
     131,   208,   248,   131,   131,   136,   335,   322,   131,   224,
     134,   134,   335,   335,   335,   335,   335,   335,   335,   131,
     134,   132,     4,   219,   220,   221,   222,   324,   327,   334,
     335,   132,   132,   132,   209,   207,   335,   321,     3,     3,
     133,   297,     3,   131,   136,   344,   131,   335,    64,    65,
     271,     5,   140,   131,   335,   131,   208,   131,   335,   208,
     323,   323,   344,    58,    58,   131,   131,    58,   267,   344,
     323,   323,     3,     7,   133,   158,   159,   328,   329,   344,
     128,   134,   344,   131,   335,    55,    59,   316,    59,    61,
      62,   317,   320,    67,     4,   131,    67,   352,    59,   133,
     260,   131,   131,   352,   352,   352,   322,   131,   253,   346,
     323,    57,    59,   351,    59,   133,   261,   315,   249,   133,
     328,   328,   243,    87,   191,   238,   136,    59,   334,   212,
     337,   338,   339,   136,   136,   340,   344,   131,   131,   138,
     131,   138,   131,   138,   131,   138,   137,   332,   137,   330,
     137,   328,   138,   136,     3,   131,   325,   134,     4,   136,
     259,   139,   136,     4,     8,     9,    54,    55,    56,    69,
      83,    84,    92,    93,    94,    95,    96,   108,   110,   282,
     323,   345,   347,   139,   136,   275,   131,   131,   323,   323,
     192,   131,   134,   131,   131,   131,     3,   135,   131,   131,
     131,     4,   136,   131,   135,   131,   131,     4,   131,     4,
     131,   316,     4,   131,   131,   293,   131,    58,   131,     4,
     131,   131,    77,   131,   208,   191,   335,   232,   132,   131,
     134,     4,   327,   324,   335,   334,   216,   333,   217,   331,
     218,   329,   131,     3,   134,   140,   134,   132,   132,   132,
     132,   132,   134,   131,   315,     5,   131,   267,   344,   335,
     335,   344,   131,   335,   344,   344,   133,   131,   311,     4,
      59,    66,   319,     4,   131,    67,   131,    59,   133,   262,
     139,    67,   322,   131,   334,   131,    59,   133,   263,    59,
     266,   267,   208,   328,   137,   144,   145,   336,   337,   225,
     131,   138,   131,   138,   131,   138,   316,   131,   334,   322,
     322,   322,   330,    61,    62,    72,    73,    74,    95,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   276,
     318,   320,   334,   131,   203,   335,   134,   131,   134,   131,
     134,   316,   136,   134,   134,   335,     4,   131,   312,   136,
     131,     4,     4,   270,   131,   131,     4,    59,   268,     4,
       4,   312,   131,   131,    89,    91,   233,   234,   208,   333,
     331,   329,   134,     5,   132,   334,   132,   132,   132,   132,
       3,    32,   335,   134,   186,   131,    32,    33,    34,   313,
     314,   303,     4,   131,   134,   131,   263,   322,   134,   134,
     131,   256,   268,     7,   346,   132,   138,   233,   131,    59,
     322,   322,   322,   107,   139,   277,   134,   131,   134,   335,
       4,   131,   208,   304,   131,     4,   271,   312,     4,   208,
     132,   236,     5,   192,    32,   131,   201,   134,   313,     4,
     134,   134,   258,   134,   235,   207,   131,   277,   134,   129,
     134,   134,   208,   207,   316,   131,   131,   323,     5,    59,
     204,   131,   202,   203,   323,   134
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 6:

/* Line 1455 of yacc.c  */
#line 285 "lev_comp.y"
    {
			if (fatal_error > 0) {
				(void) fprintf(stderr,
				"%s: %d errors detected for level \"%s\". No output created!\n",
					       fname, fatal_error, (yyvsp[(1) - (3)].map));
				fatal_error = 0;
				got_errors++;
			} else if (!got_errors) {
				if (!write_level_file((yyvsp[(1) - (3)].map), splev)) {
				    lc_error("Can't write output file for '%s'!", (yyvsp[(1) - (3)].map));
				    exit(EXIT_FAILURE);
				}
			}
			Free((yyvsp[(1) - (3)].map));
			Free(splev);
			splev = NULL;
			vardef_free_all(variable_definitions);
			variable_definitions = NULL;
		  }
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 307 "lev_comp.y"
    {
		      start_level_def(&splev, (yyvsp[(3) - (3)].map));
		      (yyval.map) = (yyvsp[(3) - (3)].map);
		  }
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 312 "lev_comp.y"
    {
		      start_level_def(&splev, (yyvsp[(3) - (5)].map));
		      if ((yyvsp[(5) - (5)].i) == -1) {
			  add_opvars(splev, "iiiiiiiio",
				     VA_PASS9(LVLINIT_MAZEGRID,HWALL,0,0,
					      0,0,0,0, SPO_INITLEVEL));
		      } else {
			  long bg = what_map_char((char) (yyvsp[(5) - (5)].i));
			  add_opvars(splev, "iiiiiiiio",
				     VA_PASS9(LVLINIT_SOLIDFILL, bg, 0,0,
					      0,0,0,0, SPO_INITLEVEL));
		      }
		      add_opvars(splev, "io",
				 VA_PASS2(MAZELEVEL, SPO_LEVEL_FLAGS));
		      max_x_map = COLNO-1;
		      max_y_map = ROWNO;
		      (yyval.map) = (yyvsp[(3) - (5)].map);
		  }
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 333 "lev_comp.y"
    {
		      (yyval.i) = -1;
		  }
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 337 "lev_comp.y"
    {
		      (yyval.i) = what_map_char((char) (yyvsp[(1) - (1)].i));
		  }
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 343 "lev_comp.y"
    {
		      long filling = (yyvsp[(5) - (5)].terr).ter;
		      if (filling == INVALID_TYPE || filling >= MAX_TYPE)
			  lc_error("INIT_MAP: Invalid fill char type.");
		      add_opvars(splev, "iiiiiiiio",
				 LVLINIT_SOLIDFILL,filling,0,(long)(yyvsp[(5) - (5)].terr).lit, 0,0,0,0, SPO_INITLEVEL);
		      max_x_map = COLNO-1;
		      max_y_map = ROWNO;
		  }
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 353 "lev_comp.y"
    {
		      long filling = what_map_char((char) (yyvsp[(5) - (5)].i));
		      if (filling == INVALID_TYPE || filling >= MAX_TYPE)
			  lc_error("INIT_MAP: Invalid fill char type.");
		      add_opvars(splev, "iiiiiiiio",
				 VA_PASS9(LVLINIT_MAZEGRID,filling,0,0,
					  0,0,0,0, SPO_INITLEVEL));
		      max_x_map = COLNO-1;
		      max_y_map = ROWNO;
		  }
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 364 "lev_comp.y"
    {
		      add_opvars(splev, "iiiiiiiio",
				 VA_PASS9(LVLINIT_ROGUE,0,0,0,
					  0,0,0,0, SPO_INITLEVEL));
		  }
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 370 "lev_comp.y"
    {
		      long fg = what_map_char((char) (yyvsp[(5) - (16)].i));
		      long bg = what_map_char((char) (yyvsp[(7) - (16)].i));
		      long smoothed = (yyvsp[(9) - (16)].i);
		      long joined = (yyvsp[(11) - (16)].i);
		      long lit = (yyvsp[(13) - (16)].i);
		      long walled = (yyvsp[(15) - (16)].i);
		      long filling = (yyvsp[(16) - (16)].i);
		      if (fg == INVALID_TYPE || fg >= MAX_TYPE)
			  lc_error("INIT_MAP: Invalid foreground type.");
		      if (bg == INVALID_TYPE || bg >= MAX_TYPE)
			  lc_error("INIT_MAP: Invalid background type.");
		      if (joined && fg != CORR && fg != ROOM)
			  lc_error("INIT_MAP: Invalid foreground type for joined map.");

		      if (filling == INVALID_TYPE)
			  lc_error("INIT_MAP: Invalid fill char type.");

		      add_opvars(splev, "iiiiiiiio",
				 VA_PASS9(LVLINIT_MINES,filling,walled,lit,
					  joined,smoothed,bg,fg,
					  SPO_INITLEVEL));
			max_x_map = COLNO-1;
			max_y_map = ROWNO;
		  }
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 398 "lev_comp.y"
    {
		      (yyval.i) = 0;
		  }
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 402 "lev_comp.y"
    {
		      (yyval.i) = (yyvsp[(2) - (2)].i);
		  }
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 408 "lev_comp.y"
    {
		      add_opvars(splev, "o", VA_PASS1(SPO_COPY));
		      (yyval.i) = 0;
		  }
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 413 "lev_comp.y"
    {
		      (yyval.i) = 1;
		  }
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 419 "lev_comp.y"
    {
		      (yyval.i) = -1;
		  }
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 423 "lev_comp.y"
    {
		      (yyval.i) = what_map_char((char) (yyvsp[(2) - (2)].i));
		  }
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 434 "lev_comp.y"
    {
		      add_opvars(splev, "io", VA_PASS2(0, SPO_LEVEL_FLAGS));
		  }
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 438 "lev_comp.y"
    {
		      add_opvars(splev, "io", VA_PASS2((yyvsp[(3) - (3)].i), SPO_LEVEL_FLAGS));
		  }
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 444 "lev_comp.y"
    {
		      (yyval.i) = ((yyvsp[(1) - (3)].i) | (yyvsp[(3) - (3)].i));
		  }
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 448 "lev_comp.y"
    {
		      (yyval.i) = (yyvsp[(1) - (1)].i);
		  }
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 454 "lev_comp.y"
    {
		      (yyval.i) = 0;
		  }
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 458 "lev_comp.y"
    {
		      (yyval.i) = 1 + (yyvsp[(2) - (2)].i);
		  }
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 464 "lev_comp.y"
    {
		      (yyval.i) = (yyvsp[(2) - (3)].i);
		  }
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 547 "lev_comp.y"
    {
		      struct lc_vardefs *vd;
		      if ((vd = vardef_defined(variable_definitions, (yyvsp[(3) - (3)].map), 1))) {
			  if (!(vd->var_type & SPOVAR_ARRAY))
			      lc_error("Trying to shuffle non-array variable '%s'", (yyvsp[(3) - (3)].map));
		      } else lc_error("Trying to shuffle undefined variable '%s'", (yyvsp[(3) - (3)].map));
		      add_opvars(splev, "so", VA_PASS2((yyvsp[(3) - (3)].map), SPO_SHUFFLE_ARRAY));
		      Free((yyvsp[(3) - (3)].map));
		  }
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 559 "lev_comp.y"
    {
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[(1) - (3)].map), SPOVAR_INT);
		      add_opvars(splev, "iso", VA_PASS3(0, (yyvsp[(1) - (3)].map), SPO_VAR_INIT));
		      Free((yyvsp[(1) - (3)].map));
		  }
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 565 "lev_comp.y"
    {
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[(1) - (5)].map), SPOVAR_SEL);
		      add_opvars(splev, "iso", VA_PASS3(0, (yyvsp[(1) - (5)].map), SPO_VAR_INIT));
		      Free((yyvsp[(1) - (5)].map));
		  }
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 571 "lev_comp.y"
    {
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[(1) - (3)].map), SPOVAR_STRING);
		      add_opvars(splev, "iso", VA_PASS3(0, (yyvsp[(1) - (3)].map), SPO_VAR_INIT));
		      Free((yyvsp[(1) - (3)].map));
		  }
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 577 "lev_comp.y"
    {
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[(1) - (5)].map), SPOVAR_MAPCHAR);
		      add_opvars(splev, "iso", VA_PASS3(0, (yyvsp[(1) - (5)].map), SPO_VAR_INIT));
		      Free((yyvsp[(1) - (5)].map));
		  }
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 583 "lev_comp.y"
    {
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[(1) - (5)].map), SPOVAR_MONST);
		      add_opvars(splev, "iso", VA_PASS3(0, (yyvsp[(1) - (5)].map), SPO_VAR_INIT));
		      Free((yyvsp[(1) - (5)].map));
		  }
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 589 "lev_comp.y"
    {
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[(1) - (5)].map), SPOVAR_OBJ);
		      add_opvars(splev, "iso", VA_PASS3(0, (yyvsp[(1) - (5)].map), SPO_VAR_INIT));
		      Free((yyvsp[(1) - (5)].map));
		  }
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 595 "lev_comp.y"
    {
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[(1) - (3)].map), SPOVAR_COORD);
		      add_opvars(splev, "iso", VA_PASS3(0, (yyvsp[(1) - (3)].map), SPO_VAR_INIT));
		      Free((yyvsp[(1) - (3)].map));
		  }
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 601 "lev_comp.y"
    {
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[(1) - (3)].map), SPOVAR_REGION);
		      add_opvars(splev, "iso", VA_PASS3(0, (yyvsp[(1) - (3)].map), SPO_VAR_INIT));
		      Free((yyvsp[(1) - (3)].map));
		  }
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 607 "lev_comp.y"
    {
		      long n_items = (yyvsp[(4) - (5)].i);
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[(1) - (5)].map), SPOVAR_INT|SPOVAR_ARRAY);
		      add_opvars(splev, "iso",
				 VA_PASS3(n_items, (yyvsp[(1) - (5)].map), SPO_VAR_INIT));
		      Free((yyvsp[(1) - (5)].map));
		  }
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 615 "lev_comp.y"
    {
		      long n_items = (yyvsp[(4) - (5)].i);
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[(1) - (5)].map), SPOVAR_COORD|SPOVAR_ARRAY);
		      add_opvars(splev, "iso",
				 VA_PASS3(n_items, (yyvsp[(1) - (5)].map), SPO_VAR_INIT));
		      Free((yyvsp[(1) - (5)].map));
		  }
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 623 "lev_comp.y"
    {
		      long n_items = (yyvsp[(4) - (5)].i);
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[(1) - (5)].map), SPOVAR_REGION|SPOVAR_ARRAY);
		      add_opvars(splev, "iso",
				 VA_PASS3(n_items, (yyvsp[(1) - (5)].map), SPO_VAR_INIT));
		      Free((yyvsp[(1) - (5)].map));
		  }
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 631 "lev_comp.y"
    {
		      long n_items = (yyvsp[(6) - (7)].i);
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[(1) - (7)].map), SPOVAR_MAPCHAR|SPOVAR_ARRAY);
		      add_opvars(splev, "iso",
				 VA_PASS3(n_items, (yyvsp[(1) - (7)].map), SPO_VAR_INIT));
		      Free((yyvsp[(1) - (7)].map));
		  }
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 639 "lev_comp.y"
    {
		      long n_items = (yyvsp[(6) - (7)].i);
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[(1) - (7)].map), SPOVAR_MONST|SPOVAR_ARRAY);
		      add_opvars(splev, "iso",
				 VA_PASS3(n_items, (yyvsp[(1) - (7)].map), SPO_VAR_INIT));
		      Free((yyvsp[(1) - (7)].map));
		  }
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 647 "lev_comp.y"
    {
		      long n_items = (yyvsp[(6) - (7)].i);
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[(1) - (7)].map), SPOVAR_OBJ|SPOVAR_ARRAY);
		      add_opvars(splev, "iso",
				 VA_PASS3(n_items, (yyvsp[(1) - (7)].map), SPO_VAR_INIT));
		      Free((yyvsp[(1) - (7)].map));
		  }
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 655 "lev_comp.y"
    {
		      long n_items = (yyvsp[(4) - (5)].i);
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[(1) - (5)].map), SPOVAR_STRING|SPOVAR_ARRAY);
		      add_opvars(splev, "iso",
				 VA_PASS3(n_items, (yyvsp[(1) - (5)].map), SPO_VAR_INIT));
		      Free((yyvsp[(1) - (5)].map));
		  }
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 665 "lev_comp.y"
    {
		      add_opvars(splev, "O", VA_PASS1((yyvsp[(1) - (1)].i)));
		      (yyval.i) = 1;
		  }
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 670 "lev_comp.y"
    {
		      add_opvars(splev, "O", VA_PASS1((yyvsp[(3) - (3)].i)));
		      (yyval.i) = 1 + (yyvsp[(1) - (3)].i);
		  }
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 677 "lev_comp.y"
    {
		      add_opvars(splev, "M", VA_PASS1((yyvsp[(1) - (1)].i)));
		      (yyval.i) = 1;
		  }
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 682 "lev_comp.y"
    {
		      add_opvars(splev, "M", VA_PASS1((yyvsp[(3) - (3)].i)));
		      (yyval.i) = 1 + (yyvsp[(1) - (3)].i);
		  }
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 689 "lev_comp.y"
    {
		      add_opvars(splev, "m", VA_PASS1((yyvsp[(1) - (1)].i)));
		      (yyval.i) = 1;
		  }
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 694 "lev_comp.y"
    {
		      add_opvars(splev, "m", VA_PASS1((yyvsp[(3) - (3)].i)));
		      (yyval.i) = 1 + (yyvsp[(1) - (3)].i);
		  }
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 701 "lev_comp.y"
    {
		      (yyval.i) = 1;
		  }
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 705 "lev_comp.y"
    {
		      (yyval.i) = 1 + (yyvsp[(1) - (3)].i);
		  }
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 711 "lev_comp.y"
    {
		      add_opvars(splev, "c", VA_PASS1((yyvsp[(1) - (1)].i)));
		      (yyval.i) = 1;
		  }
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 716 "lev_comp.y"
    {
		      add_opvars(splev, "c", VA_PASS1((yyvsp[(3) - (3)].i)));
		      (yyval.i) = 1 + (yyvsp[(1) - (3)].i);
		  }
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 723 "lev_comp.y"
    {
		      (yyval.i) = 1;
		  }
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 727 "lev_comp.y"
    {
		      (yyval.i) = 1 + (yyvsp[(1) - (3)].i);
		  }
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 733 "lev_comp.y"
    {
		      (yyval.i) = 1;
		  }
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 737 "lev_comp.y"
    {
		      (yyval.i) = 1 + (yyvsp[(1) - (3)].i);
		  }
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 743 "lev_comp.y"
    {
		      struct lc_funcdefs *funcdef;

		      if (in_function_definition)
			  lc_error("Recursively defined functions not allowed (function %s).", (yyvsp[(2) - (3)].map));

		      in_function_definition++;

		      if (funcdef_defined(function_definitions, (yyvsp[(2) - (3)].map), 1))
			  lc_error("Function '%s' already defined once.", (yyvsp[(2) - (3)].map));

		      funcdef = funcdef_new(-1, (yyvsp[(2) - (3)].map));
		      funcdef->next = function_definitions;
		      function_definitions = funcdef;
		      function_splev_backup = splev;
		      splev = &(funcdef->code);
		      Free((yyvsp[(2) - (3)].map));
		      curr_function = funcdef;
		      function_tmp_var_defs = variable_definitions;
		      variable_definitions = NULL;
		  }
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 765 "lev_comp.y"
    {
		      /* nothing */
		  }
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 769 "lev_comp.y"
    {
		      add_opvars(splev, "io", VA_PASS2(0, SPO_RETURN));
		      splev = function_splev_backup;
		      in_function_definition--;
		      curr_function = NULL;
		      vardef_free_all(variable_definitions);
		      variable_definitions = function_tmp_var_defs;
		  }
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 780 "lev_comp.y"
    {
		      struct lc_funcdefs *tmpfunc;
		      tmpfunc = funcdef_defined(function_definitions, (yyvsp[(1) - (4)].map), 1);
		      if (tmpfunc) {
			  long l;
			  long nparams = strlen( (yyvsp[(3) - (4)].map) );
			  char *fparamstr = funcdef_paramtypes(tmpfunc);
			  if (strcmp((yyvsp[(3) - (4)].map), fparamstr)) {
			      char *tmps = strdup(decode_parm_str(fparamstr));
			      lc_error("Function '%s' requires params '%s', got '%s' instead.", (yyvsp[(1) - (4)].map), tmps, decode_parm_str((yyvsp[(3) - (4)].map)));
			      Free(tmps);
			  }
			  Free(fparamstr);
			  Free((yyvsp[(3) - (4)].map));
			  if (!(tmpfunc->n_called)) {
			      /* we haven't called the function yet, so insert it in the code */
			      struct opvar *jmp = New(struct opvar);
			      set_opvar_int(jmp, splev->n_opcodes+1);
			      add_opcode(splev, SPO_PUSH, jmp);
			      add_opcode(splev, SPO_JMP, NULL); /* we must jump past it first, then CALL it, due to RETURN. */

			      tmpfunc->addr = splev->n_opcodes;

			      { /* init function parameter variables */
				  struct lc_funcdefs_parm *tfp = tmpfunc->params;
				  while (tfp) {
				      add_opvars(splev, "iso",
						 VA_PASS3(0, tfp->name,
							  SPO_VAR_INIT));
				      tfp = tfp->next;
				  }
			      }

			      splev_add_from(splev, &(tmpfunc->code));
			      set_opvar_int(jmp, splev->n_opcodes - jmp->vardata.l);
			  }
			  l = tmpfunc->addr - splev->n_opcodes - 2;
			  add_opvars(splev, "iio",
				     VA_PASS3(nparams, l, SPO_CALL));
			  tmpfunc->n_called++;
		      } else {
			  lc_error("Function '%s' not defined.", (yyvsp[(1) - (4)].map));
		      }
		      Free((yyvsp[(1) - (4)].map));
		  }
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 828 "lev_comp.y"
    {
		      add_opcode(splev, SPO_EXIT, NULL);
		  }
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 834 "lev_comp.y"
    {
		      (yyval.i) = 100;
		  }
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 838 "lev_comp.y"
    {
		      (yyval.i) = (yyvsp[(1) - (1)].i);
		  }
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 844 "lev_comp.y"
    {
		      /* val > rn2(100) */
		      add_opvars(splev, "iio",
				 VA_PASS3((long)(yyvsp[(1) - (1)].i), 100, SPO_RN2));
		      (yyval.i) = SPO_JG;
                  }
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 851 "lev_comp.y"
    {
		      (yyval.i) = (yyvsp[(3) - (5)].i);
                  }
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 855 "lev_comp.y"
    {
		      /* boolean, explicit foo != 0 */
		      add_opvars(splev, "i", VA_PASS1(0));
		      (yyval.i) = SPO_JNE;
                  }
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 863 "lev_comp.y"
    {
		      is_inconstant_number = 0;
		  }
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 867 "lev_comp.y"
    {
		      struct opvar *chkjmp;
		      if (in_switch_statement > 0)
			  lc_error("Cannot nest switch-statements.");

		      in_switch_statement++;

		      n_switch_case_list = 0;
		      switch_default_case = NULL;

		      if (!is_inconstant_number)
			  add_opvars(splev, "o", VA_PASS1(SPO_RN2));
		      is_inconstant_number = 0;

		      chkjmp = New(struct opvar);
		      set_opvar_int(chkjmp, splev->n_opcodes+1);
		      switch_check_jump = chkjmp;
		      add_opcode(splev, SPO_PUSH, chkjmp);
		      add_opcode(splev, SPO_JMP, NULL);
		      break_stmt_start();
		  }
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 889 "lev_comp.y"
    {
		      struct opvar *endjump = New(struct opvar);
		      int i;

		      set_opvar_int(endjump, splev->n_opcodes+1);

		      add_opcode(splev, SPO_PUSH, endjump);
		      add_opcode(splev, SPO_JMP, NULL);

		      set_opvar_int(switch_check_jump,
			     splev->n_opcodes - switch_check_jump->vardata.l);

		      for (i = 0; i < n_switch_case_list; i++) {
			  add_opvars(splev, "oio",
				     VA_PASS3(SPO_COPY,
					      switch_case_value[i], SPO_CMP));
			  set_opvar_int(switch_case_list[i],
			 switch_case_list[i]->vardata.l - splev->n_opcodes-1);
			  add_opcode(splev, SPO_PUSH, switch_case_list[i]);
			  add_opcode(splev, SPO_JE, NULL);
		      }

		      if (switch_default_case) {
			  set_opvar_int(switch_default_case,
			 switch_default_case->vardata.l - splev->n_opcodes-1);
			  add_opcode(splev, SPO_PUSH, switch_default_case);
			  add_opcode(splev, SPO_JMP, NULL);
		      }

		      set_opvar_int(endjump, splev->n_opcodes - endjump->vardata.l);

		      break_stmt_end(splev);

		      add_opcode(splev, SPO_POP, NULL); /* get rid of the value in stack */
		      in_switch_statement--;


		  }
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 934 "lev_comp.y"
    {
		      if (n_switch_case_list < MAX_SWITCH_CASES) {
			  struct opvar *tmppush = New(struct opvar);
			  set_opvar_int(tmppush, splev->n_opcodes);
			  switch_case_value[n_switch_case_list] = (yyvsp[(2) - (3)].i);
			  switch_case_list[n_switch_case_list++] = tmppush;
		      } else lc_error("Too many cases in a switch.");
		  }
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 943 "lev_comp.y"
    {
		  }
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 946 "lev_comp.y"
    {
		      struct opvar *tmppush = New(struct opvar);

		      if (switch_default_case)
			  lc_error("Switch default case already used.");

		      set_opvar_int(tmppush, splev->n_opcodes);
		      switch_default_case = tmppush;
		  }
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 956 "lev_comp.y"
    {
		  }
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 961 "lev_comp.y"
    {
		      if (!allow_break_statements)
			  lc_error("Cannot use BREAK outside a statement block.");
		      else {
			  break_stmt_new(splev, splev->n_opcodes);
		      }
		  }
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 975 "lev_comp.y"
    {
		      char buf[256], buf2[256];

		      if (n_forloops >= MAX_NESTED_IFS) {
			  lc_error("FOR: Too deeply nested loops.");
			  n_forloops = MAX_NESTED_IFS - 1;
		      }

		      /* first, define a variable for the for-loop end value */
		      snprintf(buf, 255, "%s end", (yyvsp[(2) - (6)].map));
		      /* the value of which is already in stack (the 2nd math_expr) */
		      add_opvars(splev, "iso", VA_PASS3(0, buf, SPO_VAR_INIT));

		      variable_definitions = add_vardef_type(variable_definitions,
							     (yyvsp[(2) - (6)].map), SPOVAR_INT);
		      /* define the for-loop variable. value is in stack (1st math_expr) */
		      add_opvars(splev, "iso", VA_PASS3(0, (yyvsp[(2) - (6)].map), SPO_VAR_INIT));

		      /* calculate value for the loop "step" variable */
		      snprintf(buf2, 255, "%s step", (yyvsp[(2) - (6)].map));
		      /* end - start */
		      add_opvars(splev, "vvo",
				 VA_PASS3(buf, (yyvsp[(2) - (6)].map), SPO_MATH_SUB));
		      /* sign of that */
		      add_opvars(splev, "o", VA_PASS1(SPO_MATH_SIGN));
		      /* save the sign into the step var */
		      add_opvars(splev, "iso",
				 VA_PASS3(0, buf2, SPO_VAR_INIT));

		      forloop_list[n_forloops].varname = strdup((yyvsp[(2) - (6)].map));
		      forloop_list[n_forloops].jmp_point = splev->n_opcodes;

		      n_forloops++;
		      Free((yyvsp[(2) - (6)].map));
		  }
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 1013 "lev_comp.y"
    {
		      /* nothing */
		      break_stmt_start();
		  }
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 1018 "lev_comp.y"
    {
		      char buf[256], buf2[256];
		      n_forloops--;
		      snprintf(buf, 255, "%s step", forloop_list[n_forloops].varname);
		      snprintf(buf2, 255, "%s end", forloop_list[n_forloops].varname);
		      /* compare for-loop var to end value */
		      add_opvars(splev, "vvo",
				 VA_PASS3(forloop_list[n_forloops].varname,
					  buf2, SPO_CMP));
		      /* var + step */
		      add_opvars(splev, "vvo",
				VA_PASS3(buf, forloop_list[n_forloops].varname,
					 SPO_MATH_ADD));
		      /* for-loop var = (for-loop var + step) */
		      add_opvars(splev, "iso",
				 VA_PASS3(0, forloop_list[n_forloops].varname,
					  SPO_VAR_INIT));
		      /* jump back if compared values were not equal */
		      add_opvars(splev, "io",
				 VA_PASS2(
		    forloop_list[n_forloops].jmp_point - splev->n_opcodes - 1,
					  SPO_JNE));
		      Free(forloop_list[n_forloops].varname);
		      break_stmt_end(splev);
		  }
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 1046 "lev_comp.y"
    {
		      struct opvar *tmppush = New(struct opvar);

		      if (n_if_list >= MAX_NESTED_IFS) {
			  lc_error("LOOP: Too deeply nested conditionals.");
			  n_if_list = MAX_NESTED_IFS - 1;
		      }
		      set_opvar_int(tmppush, splev->n_opcodes);
		      if_list[n_if_list++] = tmppush;

		      add_opvars(splev, "o", VA_PASS1(SPO_DEC));
		      break_stmt_start();
		  }
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 1060 "lev_comp.y"
    {
		      struct opvar *tmppush;

		      add_opvars(splev, "oio", VA_PASS3(SPO_COPY, 0, SPO_CMP));

		      tmppush = (struct opvar *) if_list[--n_if_list];
		      set_opvar_int(tmppush, tmppush->vardata.l - splev->n_opcodes-1);
		      add_opcode(splev, SPO_PUSH, tmppush);
		      add_opcode(splev, SPO_JG, NULL);
		      add_opcode(splev, SPO_POP, NULL); /* get rid of the count value in stack */
		      break_stmt_end(splev);
		  }
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 1075 "lev_comp.y"
    {
		      struct opvar *tmppush2 = New(struct opvar);

		      if (n_if_list >= MAX_NESTED_IFS) {
			  lc_error("IF: Too deeply nested conditionals.");
			  n_if_list = MAX_NESTED_IFS - 1;
		      }

		      add_opcode(splev, SPO_CMP, NULL);

		      set_opvar_int(tmppush2, splev->n_opcodes+1);

		      if_list[n_if_list++] = tmppush2;

		      add_opcode(splev, SPO_PUSH, tmppush2);

		      add_opcode(splev, reverse_jmp_opcode( (yyvsp[(1) - (2)].i) ), NULL);

		  }
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1095 "lev_comp.y"
    {
		      if (n_if_list > 0) {
			  struct opvar *tmppush;
			  tmppush = (struct opvar *) if_list[--n_if_list];
			  set_opvar_int(tmppush, splev->n_opcodes - tmppush->vardata.l);
		      } else lc_error("IF: Huh?!  No start address?");
		  }
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 1105 "lev_comp.y"
    {
		      struct opvar *tmppush2 = New(struct opvar);

		      if (n_if_list >= MAX_NESTED_IFS) {
			  lc_error("IF: Too deeply nested conditionals.");
			  n_if_list = MAX_NESTED_IFS - 1;
		      }

		      add_opcode(splev, SPO_CMP, NULL);

		      set_opvar_int(tmppush2, splev->n_opcodes+1);

		      if_list[n_if_list++] = tmppush2;

		      add_opcode(splev, SPO_PUSH, tmppush2);

		      add_opcode(splev, reverse_jmp_opcode( (yyvsp[(2) - (2)].i) ), NULL);

		  }
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1125 "lev_comp.y"
    {
		     /* do nothing */
		  }
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 1131 "lev_comp.y"
    {
		      if (n_if_list > 0) {
			  struct opvar *tmppush;
			  tmppush = (struct opvar *) if_list[--n_if_list];
			  set_opvar_int(tmppush, splev->n_opcodes - tmppush->vardata.l);
		      } else lc_error("IF: Huh?!  No start address?");
		  }
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 1139 "lev_comp.y"
    {
		      if (n_if_list > 0) {
			  struct opvar *tmppush = New(struct opvar);
			  struct opvar *tmppush2;

			  set_opvar_int(tmppush, splev->n_opcodes+1);
			  add_opcode(splev, SPO_PUSH, tmppush);

			  add_opcode(splev, SPO_JMP, NULL);

			  tmppush2 = (struct opvar *) if_list[--n_if_list];

			  set_opvar_int(tmppush2, splev->n_opcodes - tmppush2->vardata.l);
			  if_list[n_if_list++] = tmppush;
		      } else lc_error("IF: Huh?!  No else-part address?");
		  }
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 1156 "lev_comp.y"
    {
		      if (n_if_list > 0) {
			  struct opvar *tmppush;
			  tmppush = (struct opvar *) if_list[--n_if_list];
			  set_opvar_int(tmppush, splev->n_opcodes - tmppush->vardata.l);
		      } else lc_error("IF: Huh?! No end address?");
		  }
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 1166 "lev_comp.y"
    {
		      add_opvars(splev, "o", VA_PASS1(SPO_MESSAGE));
		  }
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 1172 "lev_comp.y"
    {
		      add_opvars(splev, "iiiiiio",
			      VA_PASS7(-1,  0, -1, -1, -1, -1, SPO_CORRIDOR));
		  }
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 1177 "lev_comp.y"
    {
		      add_opvars(splev, "iiiiiio",
			      VA_PASS7(-1, (yyvsp[(3) - (3)].i), -1, -1, -1, -1, SPO_CORRIDOR));
		  }
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 1182 "lev_comp.y"
    {
		      add_opvars(splev, "iiiiiio",
			      VA_PASS7(-1, -1, -1, -1, -1, -1, SPO_CORRIDOR));
		  }
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 1189 "lev_comp.y"
    {
		      add_opvars(splev, "iiiiiio",
				 VA_PASS7((yyvsp[(3) - (5)].corpos).room, (yyvsp[(3) - (5)].corpos).door, (yyvsp[(3) - (5)].corpos).wall,
					  (yyvsp[(5) - (5)].corpos).room, (yyvsp[(5) - (5)].corpos).door, (yyvsp[(5) - (5)].corpos).wall,
					  SPO_CORRIDOR));
		  }
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 1196 "lev_comp.y"
    {
		      add_opvars(splev, "iiiiiio",
				 VA_PASS7((yyvsp[(3) - (5)].corpos).room, (yyvsp[(3) - (5)].corpos).door, (yyvsp[(3) - (5)].corpos).wall,
					  -1, -1, (long)(yyvsp[(5) - (5)].i),
					  SPO_CORRIDOR));
		  }
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 1205 "lev_comp.y"
    {
			(yyval.corpos).room = (yyvsp[(2) - (7)].i);
			(yyval.corpos).wall = (yyvsp[(4) - (7)].i);
			(yyval.corpos).door = (yyvsp[(6) - (7)].i);
		  }
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 1213 "lev_comp.y"
    {
		      if (((yyvsp[(2) - (4)].i) < 100) && ((yyvsp[(1) - (4)].i) == OROOM))
			  lc_error("Only typed rooms can have a chance.");
		      else {
			  add_opvars(splev, "iii",
				     VA_PASS3((long)(yyvsp[(1) - (4)].i), (long)(yyvsp[(2) - (4)].i), (long)(yyvsp[(4) - (4)].i)));
		      }
                  }
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 1224 "lev_comp.y"
    {
		      long rflags = (yyvsp[(8) - (8)].i);

		      if (rflags == -1) rflags = (1 << 0);
		      add_opvars(splev, "iiiiiiio",
				 VA_PASS8(rflags, ERR, ERR,
					  (yyvsp[(5) - (8)].crd).x, (yyvsp[(5) - (8)].crd).y, (yyvsp[(7) - (8)].sze).width, (yyvsp[(7) - (8)].sze).height,
					  SPO_SUBROOM));
		      break_stmt_start();
		  }
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 1235 "lev_comp.y"
    {
		      break_stmt_end(splev);
		      add_opcode(splev, SPO_ENDROOM, NULL);
		  }
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1242 "lev_comp.y"
    {
		      long rflags = (yyvsp[(8) - (10)].i);

		      if (rflags == -1) rflags = (1 << 0);
		      add_opvars(splev, "iiiiiiio",
				 VA_PASS8(rflags,
					  (yyvsp[(7) - (10)].crd).x, (yyvsp[(7) - (10)].crd).y, (yyvsp[(5) - (10)].crd).x, (yyvsp[(5) - (10)].crd).y,
					  (yyvsp[(9) - (10)].sze).width, (yyvsp[(9) - (10)].sze).height, SPO_ROOM));
		      break_stmt_start();
		  }
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 1253 "lev_comp.y"
    {
		      break_stmt_end(splev);
		      add_opcode(splev, SPO_ENDROOM, NULL);
		  }
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 1260 "lev_comp.y"
    {
			(yyval.i) = 1;
		  }
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 1264 "lev_comp.y"
    {
			(yyval.i) = (yyvsp[(2) - (2)].i);
		  }
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 1270 "lev_comp.y"
    {
			if ( (yyvsp[(2) - (5)].i) < 1 || (yyvsp[(2) - (5)].i) > 5 ||
			    (yyvsp[(4) - (5)].i) < 1 || (yyvsp[(4) - (5)].i) > 5 ) {
			    lc_error("Room positions should be between 1-5: (%li,%li)!", (yyvsp[(2) - (5)].i), (yyvsp[(4) - (5)].i));
			} else {
			    (yyval.crd).x = (yyvsp[(2) - (5)].i);
			    (yyval.crd).y = (yyvsp[(4) - (5)].i);
			}
		  }
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 1280 "lev_comp.y"
    {
			(yyval.crd).x = (yyval.crd).y = ERR;
		  }
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 1286 "lev_comp.y"
    {
			if ( (yyvsp[(2) - (5)].i) < 0 || (yyvsp[(4) - (5)].i) < 0) {
			    lc_error("Invalid subroom position (%li,%li)!", (yyvsp[(2) - (5)].i), (yyvsp[(4) - (5)].i));
			} else {
			    (yyval.crd).x = (yyvsp[(2) - (5)].i);
			    (yyval.crd).y = (yyvsp[(4) - (5)].i);
			}
		  }
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1295 "lev_comp.y"
    {
			(yyval.crd).x = (yyval.crd).y = ERR;
		  }
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 1301 "lev_comp.y"
    {
		      (yyval.crd).x = (yyvsp[(2) - (5)].i);
		      (yyval.crd).y = (yyvsp[(4) - (5)].i);
		  }
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 1306 "lev_comp.y"
    {
		      (yyval.crd).x = (yyval.crd).y = ERR;
		  }
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 1312 "lev_comp.y"
    {
			(yyval.sze).width = (yyvsp[(2) - (5)].i);
			(yyval.sze).height = (yyvsp[(4) - (5)].i);
		  }
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 1317 "lev_comp.y"
    {
			(yyval.sze).height = (yyval.sze).width = ERR;
		  }
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 1323 "lev_comp.y"
    {
			/* ERR means random here */
			if ((yyvsp[(7) - (9)].i) == ERR && (yyvsp[(9) - (9)].i) != ERR) {
			    lc_error("If the door wall is random, so must be its pos!");
			} else {
			    add_opvars(splev, "iiiio",
				       VA_PASS5((long)(yyvsp[(9) - (9)].i), (long)(yyvsp[(5) - (9)].i), (long)(yyvsp[(3) - (9)].i),
						(long)(yyvsp[(7) - (9)].i), SPO_ROOM_DOOR));
			}
		  }
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 1334 "lev_comp.y"
    {
		      add_opvars(splev, "io", VA_PASS2((long)(yyvsp[(3) - (5)].i), SPO_DOOR));
		  }
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 1348 "lev_comp.y"
    {
		      (yyval.i) = (yyvsp[(1) - (1)].i);
		  }
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 1352 "lev_comp.y"
    {
		      (yyval.i) = ((yyvsp[(1) - (3)].i) | (yyvsp[(3) - (3)].i));
		  }
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 1362 "lev_comp.y"
    {
		      add_opvars(splev, "ciisiio",
				 VA_PASS7(0, 0, 1, (char *)0, 0, 0, SPO_MAP));
		      max_x_map = COLNO-1;
		      max_y_map = ROWNO;
		  }
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 1369 "lev_comp.y"
    {
		      add_opvars(splev, "cii",
				 VA_PASS3(SP_COORD_PACK(((yyvsp[(3) - (7)].i)),((yyvsp[(5) - (7)].i))),
					  1, (long)(yyvsp[(6) - (7)].i)));
		      scan_map((yyvsp[(7) - (7)].map), splev);
		      Free((yyvsp[(7) - (7)].map));
		  }
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 1377 "lev_comp.y"
    {
		      add_opvars(splev, "ii", VA_PASS2(2, (long)(yyvsp[(4) - (5)].i)));
		      scan_map((yyvsp[(5) - (5)].map), splev);
		      Free((yyvsp[(5) - (5)].map));
		  }
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 1393 "lev_comp.y"
    {
		      add_opvars(splev, "io", VA_PASS2(0, SPO_MONSTER));
		  }
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 1397 "lev_comp.y"
    {
		      add_opvars(splev, "io", VA_PASS2(1, SPO_MONSTER));
		      in_container_obj++;
		      break_stmt_start();
		  }
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 1403 "lev_comp.y"
    {
		     break_stmt_end(splev);
		     in_container_obj--;
		     add_opvars(splev, "o", VA_PASS1(SPO_END_MONINVENT));
		 }
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 1411 "lev_comp.y"
    {
		      /* nothing */
		  }
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 1417 "lev_comp.y"
    {
		      struct opvar *stopit = New(struct opvar);
		      set_opvar_int(stopit, SP_M_V_END);
		      add_opcode(splev, SPO_PUSH, stopit);
		      (yyval.i) = 0x0000;
		  }
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 1424 "lev_comp.y"
    {
		      if (( (yyvsp[(1) - (3)].i) & (yyvsp[(3) - (3)].i) ))
			  lc_error("MONSTER extra info defined twice.");
		      (yyval.i) = ( (yyvsp[(1) - (3)].i) | (yyvsp[(3) - (3)].i) );
		  }
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 1432 "lev_comp.y"
    {
		      add_opvars(splev, "i", VA_PASS1(SP_M_V_NAME));
		      (yyval.i) = 0x0001;
		  }
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 1437 "lev_comp.y"
    {
		      add_opvars(splev, "ii",
				 VA_PASS2((long)(yyvsp[(1) - (1)].i), SP_M_V_PEACEFUL));
		      (yyval.i) = 0x0002;
		  }
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 1443 "lev_comp.y"
    {
		      add_opvars(splev, "ii",
				 VA_PASS2((long)(yyvsp[(1) - (1)].i), SP_M_V_ASLEEP));
		      (yyval.i) = 0x0004;
		  }
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 1449 "lev_comp.y"
    {
		      add_opvars(splev, "ii",
				 VA_PASS2((long)(yyvsp[(1) - (1)].i), SP_M_V_ALIGN));
		      (yyval.i) = 0x0008;
		  }
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 1455 "lev_comp.y"
    {
		      add_opvars(splev, "ii",
				 VA_PASS2((long)(yyvsp[(1) - (2)].i), SP_M_V_APPEAR));
		      (yyval.i) = 0x0010;
		  }
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 1461 "lev_comp.y"
    {
		      add_opvars(splev, "ii", VA_PASS2(1, SP_M_V_FEMALE));
		      (yyval.i) = 0x0020;
		  }
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 1466 "lev_comp.y"
    {
		      add_opvars(splev, "ii", VA_PASS2(1, SP_M_V_INVIS));
		      (yyval.i) = 0x0040;
		  }
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 1471 "lev_comp.y"
    {
		      add_opvars(splev, "ii", VA_PASS2(1, SP_M_V_CANCELLED));
		      (yyval.i) = 0x0080;
		  }
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 1476 "lev_comp.y"
    {
		      add_opvars(splev, "ii", VA_PASS2(1, SP_M_V_REVIVED));
		      (yyval.i) = 0x0100;
		  }
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 1481 "lev_comp.y"
    {
		      add_opvars(splev, "ii", VA_PASS2(1, SP_M_V_AVENGE));
		      (yyval.i) = 0x0200;
		  }
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 1486 "lev_comp.y"
    {
		      add_opvars(splev, "i", VA_PASS1(SP_M_V_FLEEING));
		      (yyval.i) = 0x0400;
		  }
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 1491 "lev_comp.y"
    {
		      add_opvars(splev, "i", VA_PASS1(SP_M_V_BLINDED));
		      (yyval.i) = 0x0800;
		  }
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 1496 "lev_comp.y"
    {
		      add_opvars(splev, "i", VA_PASS1(SP_M_V_PARALYZED));
		      (yyval.i) = 0x1000;
		  }
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 1501 "lev_comp.y"
    {
		      add_opvars(splev, "ii", VA_PASS2(1, SP_M_V_STUNNED));
		      (yyval.i) = 0x2000;
		  }
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 1506 "lev_comp.y"
    {
		      add_opvars(splev, "ii", VA_PASS2(1, SP_M_V_CONFUSED));
		      (yyval.i) = 0x4000;
		  }
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 1511 "lev_comp.y"
    {
		      add_opvars(splev, "ii",
				 VA_PASS2((long)(yyvsp[(3) - (3)].i), SP_M_V_SEENTRAPS));
		      (yyval.i) = 0x8000;
		  }
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 1519 "lev_comp.y"
    {
		      int token = get_trap_type((yyvsp[(1) - (1)].map));
		      if (token == ERR || token == 0)
			  lc_error("Unknown trap type '%s'!", (yyvsp[(1) - (1)].map));
		      (yyval.i) = (1L << (token - 1));
		  }
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 1526 "lev_comp.y"
    {
		      (yyval.i) = (long) ~0;
		  }
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 1530 "lev_comp.y"
    {
		      int token = get_trap_type((yyvsp[(1) - (3)].map));
		      if (token == ERR || token == 0)
			  lc_error("Unknown trap type '%s'!", (yyvsp[(1) - (3)].map));

		      if ((1L << (token - 1)) & (yyvsp[(3) - (3)].i))
			  lc_error("Monster seen_traps, trap '%s' listed twice.", (yyvsp[(1) - (3)].map));

		      (yyval.i) = ((1L << (token - 1)) | (yyvsp[(3) - (3)].i));
		  }
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 1543 "lev_comp.y"
    {
		      long cnt = 0;
		      if (in_container_obj) cnt |= SP_OBJ_CONTENT;
		      add_opvars(splev, "io", VA_PASS2(cnt, SPO_OBJECT));
		  }
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 1549 "lev_comp.y"
    {
		      long cnt = SP_OBJ_CONTAINER;
		      if (in_container_obj) cnt |= SP_OBJ_CONTENT;
		      add_opvars(splev, "io", VA_PASS2(cnt, SPO_OBJECT));
		      in_container_obj++;
		      break_stmt_start();
		  }
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 1557 "lev_comp.y"
    {
		     break_stmt_end(splev);
		     in_container_obj--;
		     add_opcode(splev, SPO_POP_CONTAINER, NULL);
		 }
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 1565 "lev_comp.y"
    {
		      if (( (yyvsp[(2) - (2)].i) & 0x4000) && in_container_obj) lc_error("Object cannot have a coord when contained.");
		      else if (!( (yyvsp[(2) - (2)].i) & 0x4000) && !in_container_obj) lc_error("Object needs a coord when not contained.");
		  }
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 1572 "lev_comp.y"
    {
		      struct opvar *stopit = New(struct opvar);
		      set_opvar_int(stopit, SP_O_V_END);
		      add_opcode(splev, SPO_PUSH, stopit);
		      (yyval.i) = 0x00;
		  }
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 1579 "lev_comp.y"
    {
		      if (( (yyvsp[(1) - (3)].i) & (yyvsp[(3) - (3)].i) ))
			  lc_error("OBJECT extra info '%s' defined twice.", curr_token);
		      (yyval.i) = ( (yyvsp[(1) - (3)].i) | (yyvsp[(3) - (3)].i) );
		  }
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 1587 "lev_comp.y"
    {
		      add_opvars(splev, "ii",
				 VA_PASS2((long)(yyvsp[(1) - (1)].i), SP_O_V_CURSE));
		      (yyval.i) = 0x0001;
		  }
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 1593 "lev_comp.y"
    {
		      add_opvars(splev, "i", VA_PASS1(SP_O_V_CORPSENM));
		      (yyval.i) = 0x0002;
		  }
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 1598 "lev_comp.y"
    {
		      add_opvars(splev, "i", VA_PASS1(SP_O_V_SPE));
		      (yyval.i) = 0x0004;
		  }
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 1603 "lev_comp.y"
    {
		      add_opvars(splev, "i", VA_PASS1(SP_O_V_NAME));
		      (yyval.i) = 0x0008;
		  }
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 1608 "lev_comp.y"
    {
		      add_opvars(splev, "i", VA_PASS1(SP_O_V_QUAN));
		      (yyval.i) = 0x0010;
		  }
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 1613 "lev_comp.y"
    {
		      add_opvars(splev, "ii", VA_PASS2(1, SP_O_V_BURIED));
		      (yyval.i) = 0x0020;
		  }
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 1618 "lev_comp.y"
    {
		      add_opvars(splev, "ii", VA_PASS2((long)(yyvsp[(1) - (1)].i), SP_O_V_LIT));
		      (yyval.i) = 0x0040;
		  }
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 1623 "lev_comp.y"
    {
		      add_opvars(splev, "i", VA_PASS1(SP_O_V_ERODED));
		      (yyval.i) = 0x0080;
		  }
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 1628 "lev_comp.y"
    {
		      add_opvars(splev, "ii", VA_PASS2(-1, SP_O_V_ERODED));
		      (yyval.i) = 0x0080;
		  }
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 1633 "lev_comp.y"
    {
		      if ((yyvsp[(1) - (1)].i) == D_LOCKED) {
			  add_opvars(splev, "ii", VA_PASS2(1, SP_O_V_LOCKED));
			  (yyval.i) = 0x0100;
		      } else if ((yyvsp[(1) - (1)].i) == D_BROKEN) {
			  add_opvars(splev, "ii", VA_PASS2(1, SP_O_V_BROKEN));
			  (yyval.i) = 0x0200;
		      } else
			  lc_error("DOOR state can only be locked or broken.");
		  }
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 1644 "lev_comp.y"
    {
		      add_opvars(splev, "ii", VA_PASS2(1, SP_O_V_TRAPPED));
		      (yyval.i) = 0x0400;
		  }
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 1649 "lev_comp.y"
    {
		      add_opvars(splev, "i", VA_PASS1(SP_O_V_RECHARGED));
		      (yyval.i) = 0x0800;
		  }
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 1654 "lev_comp.y"
    {
		      add_opvars(splev, "ii", VA_PASS2(1, SP_O_V_INVIS));
		      (yyval.i) = 0x1000;
		  }
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 1659 "lev_comp.y"
    {
		      add_opvars(splev, "ii", VA_PASS2(1, SP_O_V_GREASED));
		      (yyval.i) = 0x2000;
		  }
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 1664 "lev_comp.y"
    {
		      add_opvars(splev, "i", VA_PASS1(SP_O_V_COORD));
		      (yyval.i) = 0x4000;
		  }
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 1671 "lev_comp.y"
    {
		      add_opvars(splev, "io", VA_PASS2((long)(yyvsp[(3) - (5)].i), SPO_TRAP));
		  }
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 1677 "lev_comp.y"
    {
		       long dir, state = 0;

		       /* convert dir from a DIRECTION to a DB_DIR */
		       dir = (yyvsp[(5) - (7)].i);
		       switch (dir) {
		       case W_NORTH: dir = DB_NORTH; break;
		       case W_SOUTH: dir = DB_SOUTH; break;
		       case W_EAST:  dir = DB_EAST;  break;
		       case W_WEST:  dir = DB_WEST;  break;
		       default:
			   lc_error("Invalid drawbridge direction.");
			   break;
		       }

		       if ( (yyvsp[(7) - (7)].i) == D_ISOPEN )
			   state = 1;
		       else if ( (yyvsp[(7) - (7)].i) == D_CLOSED )
			   state = 0;
		       else if ( (yyvsp[(7) - (7)].i) == -1 )
			   state = -1;
		       else
			   lc_error("A drawbridge can only be open, closed or random!");
		       add_opvars(splev, "iio",
				  VA_PASS3(state, dir, SPO_DRAWBRIDGE));
		   }
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 1706 "lev_comp.y"
    {
		      add_opvars(splev, "iiio",
				 VA_PASS4((long)(yyvsp[(5) - (5)].i), 1, 0, SPO_MAZEWALK));
		  }
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 1711 "lev_comp.y"
    {
		      add_opvars(splev, "iiio",
				 VA_PASS4((long)(yyvsp[(5) - (8)].i), (long)(yyvsp[(7) - (8)].i),
					  (long)(yyvsp[(8) - (8)].i), SPO_MAZEWALK));
		  }
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 1719 "lev_comp.y"
    {
		      add_opvars(splev, "rio",
				 VA_PASS3(SP_REGION_PACK(-1,-1,-1,-1),
					  0, SPO_WALLIFY));
		  }
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 1725 "lev_comp.y"
    {
		      add_opvars(splev, "io", VA_PASS2(1, SPO_WALLIFY));
		  }
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 1731 "lev_comp.y"
    {
		      add_opvars(splev, "io",
				 VA_PASS2((long)(yyvsp[(5) - (5)].i), SPO_LADDER));
		  }
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 1738 "lev_comp.y"
    {
		      add_opvars(splev, "io",
				 VA_PASS2((long)(yyvsp[(5) - (5)].i), SPO_STAIR));
		  }
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 1745 "lev_comp.y"
    {
		      add_opvars(splev, "iiiii iiiii iiso",
				 VA_PASS14((yyvsp[(3) - (7)].lregn).x1, (yyvsp[(3) - (7)].lregn).y1, (yyvsp[(3) - (7)].lregn).x2, (yyvsp[(3) - (7)].lregn).y2, (yyvsp[(3) - (7)].lregn).area,
					   (yyvsp[(5) - (7)].lregn).x1, (yyvsp[(5) - (7)].lregn).y1, (yyvsp[(5) - (7)].lregn).x2, (yyvsp[(5) - (7)].lregn).y2, (yyvsp[(5) - (7)].lregn).area,
				      (long)(((yyvsp[(7) - (7)].i)) ? LR_UPSTAIR : LR_DOWNSTAIR),
					   0, (char *)0, SPO_LEVREGION));
		  }
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 1755 "lev_comp.y"
    {
		      add_opvars(splev, "iiiii iiiii iiso",
				 VA_PASS14((yyvsp[(3) - (7)].lregn).x1, (yyvsp[(3) - (7)].lregn).y1, (yyvsp[(3) - (7)].lregn).x2, (yyvsp[(3) - (7)].lregn).y2, (yyvsp[(3) - (7)].lregn).area,
					   (yyvsp[(5) - (7)].lregn).x1, (yyvsp[(5) - (7)].lregn).y1, (yyvsp[(5) - (7)].lregn).x2, (yyvsp[(5) - (7)].lregn).y2, (yyvsp[(5) - (7)].lregn).area,
					   LR_PORTAL, 0, (yyvsp[(7) - (7)].map), SPO_LEVREGION));
		      Free((yyvsp[(7) - (7)].map));
		  }
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 1765 "lev_comp.y"
    {
		      long rtyp = 0;
		      switch((yyvsp[(6) - (6)].i)) {
		      case -1: rtyp = LR_TELE; break;
		      case  0: rtyp = LR_DOWNTELE; break;
		      case  1: rtyp = LR_UPTELE; break;
		      }
		      add_opvars(splev, "iiiii iiiii iiso",
				 VA_PASS14((yyvsp[(3) - (6)].lregn).x1, (yyvsp[(3) - (6)].lregn).y1, (yyvsp[(3) - (6)].lregn).x2, (yyvsp[(3) - (6)].lregn).y2, (yyvsp[(3) - (6)].lregn).area,
					   (yyvsp[(5) - (6)].lregn).x1, (yyvsp[(5) - (6)].lregn).y1, (yyvsp[(5) - (6)].lregn).x2, (yyvsp[(5) - (6)].lregn).y2, (yyvsp[(5) - (6)].lregn).area,
					   rtyp, 0, (char *)0, SPO_LEVREGION));
		  }
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 1780 "lev_comp.y"
    {
		      add_opvars(splev, "iiiii iiiii iiso",
				 VA_PASS14((yyvsp[(3) - (5)].lregn).x1, (yyvsp[(3) - (5)].lregn).y1, (yyvsp[(3) - (5)].lregn).x2, (yyvsp[(3) - (5)].lregn).y2, (yyvsp[(3) - (5)].lregn).area,
					   (yyvsp[(5) - (5)].lregn).x1, (yyvsp[(5) - (5)].lregn).y1, (yyvsp[(5) - (5)].lregn).x2, (yyvsp[(5) - (5)].lregn).y2, (yyvsp[(5) - (5)].lregn).area,
					   (long)LR_BRANCH, 0,
					   (char *)0, SPO_LEVREGION));
		  }
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 1790 "lev_comp.y"
    {
			(yyval.i) = -1;
		  }
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 1794 "lev_comp.y"
    {
			(yyval.i) = (yyvsp[(2) - (2)].i);
		  }
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 1800 "lev_comp.y"
    {
		      add_opvars(splev, "o", VA_PASS1(SPO_FOUNTAIN));
		  }
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 1806 "lev_comp.y"
    {
		      add_opvars(splev, "o", VA_PASS1(SPO_SINK));
		  }
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 1812 "lev_comp.y"
    {
		      add_opvars(splev, "o", VA_PASS1(SPO_POOL));
		  }
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 1818 "lev_comp.y"
    {
		      (yyval.terr).lit = -2;
		      (yyval.terr).ter = what_map_char((char) (yyvsp[(1) - (1)].i));
		  }
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 1823 "lev_comp.y"
    {
		      (yyval.terr).lit = (yyvsp[(4) - (5)].i);
		      (yyval.terr).ter = what_map_char((char) (yyvsp[(2) - (5)].i));
		  }
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 1830 "lev_comp.y"
    {
		      add_opvars(splev, "io",
				 VA_PASS2((yyvsp[(9) - (9)].i), SPO_REPLACETERRAIN));
		  }
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 1837 "lev_comp.y"
    {
		     add_opvars(splev, "o", VA_PASS1(SPO_TERRAIN));
		 }
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 1843 "lev_comp.y"
    {
		      add_opvars(splev, "o", VA_PASS1(SPO_NON_DIGGABLE));
		  }
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 1849 "lev_comp.y"
    {
		      add_opvars(splev, "o", VA_PASS1(SPO_NON_PASSWALL));
		  }
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 1855 "lev_comp.y"
    {
		      long irr;
		      long rt = (yyvsp[(7) - (8)].i);
		      long rflags = (yyvsp[(8) - (8)].i);

		      if (rflags == -1) rflags = (1 << 0);
		      if (!(rflags & 1)) rt += MAXRTYPE+1;
		      irr = ((rflags & 2) != 0);
		      add_opvars(splev, "iiio",
				 VA_PASS4((long)(yyvsp[(5) - (8)].i), rt, rflags, SPO_REGION));
		      (yyval.i) = (irr || (rflags & 1) || rt != OROOM);
		      break_stmt_start();
		  }
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 1869 "lev_comp.y"
    {
		      break_stmt_end(splev);
		      if ( (yyvsp[(9) - (10)].i) ) {
			  add_opcode(splev, SPO_ENDROOM, NULL);
		      } else if ( (yyvsp[(10) - (10)].i) )
			  lc_error("Cannot use lev statements in non-permanent REGION");
		  }
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 1879 "lev_comp.y"
    {
		      (yyval.i) = 0;
		  }
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 1883 "lev_comp.y"
    {
		      (yyval.i) = (yyvsp[(1) - (1)].i);
		  }
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 1889 "lev_comp.y"
    {
		      add_opvars(splev, "iio",
				 VA_PASS3((long)(yyvsp[(7) - (7)].i), (long)(yyvsp[(5) - (7)].i), SPO_ALTAR));
		  }
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 1896 "lev_comp.y"
    {
		      add_opvars(splev, "io", VA_PASS2(2, SPO_GRAVE));
		  }
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 1900 "lev_comp.y"
    {
		      add_opvars(splev, "sio",
				 VA_PASS3((char *)0, 1, SPO_GRAVE));
		  }
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 1905 "lev_comp.y"
    {
		      add_opvars(splev, "sio",
				 VA_PASS3((char *)0, 0, SPO_GRAVE));
		  }
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 1912 "lev_comp.y"
    {
		      add_opvars(splev, "o", VA_PASS1(SPO_GOLD));
		  }
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 1918 "lev_comp.y"
    {
		      add_opvars(splev, "io",
				 VA_PASS2((long)(yyvsp[(5) - (7)].i), SPO_ENGRAVING));
		  }
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 1925 "lev_comp.y"
    {
		      add_opvars(splev, "o", VA_PASS1(SPO_MINERALIZE));
		  }
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 1929 "lev_comp.y"
    {
		      add_opvars(splev, "iiiio",
				 VA_PASS5(-1L, -1L, -1L, -1L, SPO_MINERALIZE));
		  }
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 1936 "lev_comp.y"
    {
			int token = get_trap_type((yyvsp[(1) - (1)].map));
			if (token == ERR)
			    lc_error("Unknown trap type '%s'!", (yyvsp[(1) - (1)].map));
			(yyval.i) = token;
			Free((yyvsp[(1) - (1)].map));
		  }
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 1947 "lev_comp.y"
    {
			int token = get_room_type((yyvsp[(1) - (1)].map));
			if (token == ERR) {
			    lc_warning("Unknown room type \"%s\"!  Making ordinary room...", (yyvsp[(1) - (1)].map));
				(yyval.i) = OROOM;
			} else
				(yyval.i) = token;
			Free((yyvsp[(1) - (1)].map));
		  }
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 1960 "lev_comp.y"
    {
			(yyval.i) = -1;
		  }
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 1964 "lev_comp.y"
    {
			(yyval.i) = (yyvsp[(2) - (2)].i);
		  }
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 1970 "lev_comp.y"
    {
			(yyval.i) = (yyvsp[(1) - (1)].i);
		  }
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 1974 "lev_comp.y"
    {
			(yyval.i) = (yyvsp[(1) - (3)].i) | (yyvsp[(3) - (3)].i);
		  }
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 1981 "lev_comp.y"
    {
		      (yyval.i) = ((yyvsp[(1) - (1)].i) << 0);
		  }
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 1985 "lev_comp.y"
    {
		      (yyval.i) = ((yyvsp[(1) - (1)].i) << 1);
		  }
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 1989 "lev_comp.y"
    {
		      (yyval.i) = ((yyvsp[(1) - (1)].i) << 2);
		  }
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 2005 "lev_comp.y"
    {
			(yyval.i) = - MAX_REGISTERS - 1;
		  }
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 2013 "lev_comp.y"
    {
			(yyval.i) = - MAX_REGISTERS - 1;
		  }
    break;

  case 304:

/* Line 1455 of yacc.c  */
#line 2023 "lev_comp.y"
    {
			if ( (yyvsp[(3) - (4)].i) >= 3 )
				lc_error("Register Index overflow!");
			else
				(yyval.i) = - (yyvsp[(3) - (4)].i) - 1;
		  }
    break;

  case 305:

/* Line 1455 of yacc.c  */
#line 2032 "lev_comp.y"
    {
		      add_opvars(splev, "s", VA_PASS1((yyvsp[(1) - (1)].map)));
		      Free((yyvsp[(1) - (1)].map));
		  }
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 2037 "lev_comp.y"
    {
		      check_vardef_type(variable_definitions, (yyvsp[(1) - (1)].map), SPOVAR_STRING);
		      vardef_used(variable_definitions, (yyvsp[(1) - (1)].map));
		      add_opvars(splev, "v", VA_PASS1((yyvsp[(1) - (1)].map)));
		      Free((yyvsp[(1) - (1)].map));
		  }
    break;

  case 307:

/* Line 1455 of yacc.c  */
#line 2044 "lev_comp.y"
    {
		      check_vardef_type(variable_definitions, (yyvsp[(1) - (4)].map), SPOVAR_STRING|SPOVAR_ARRAY);
		      vardef_used(variable_definitions, (yyvsp[(1) - (4)].map));
		      add_opvars(splev, "v", VA_PASS1((yyvsp[(1) - (4)].map)));
		      Free((yyvsp[(1) - (4)].map));
		  }
    break;

  case 308:

/* Line 1455 of yacc.c  */
#line 2054 "lev_comp.y"
    {
		      /* nothing */
		  }
    break;

  case 309:

/* Line 1455 of yacc.c  */
#line 2060 "lev_comp.y"
    {
		      add_opvars(splev, "c", VA_PASS1((yyvsp[(1) - (1)].i)));
		  }
    break;

  case 310:

/* Line 1455 of yacc.c  */
#line 2064 "lev_comp.y"
    {
		      add_opvars(splev, "o", VA_PASS1(SPO_SEL_RNDCOORD));
		  }
    break;

  case 311:

/* Line 1455 of yacc.c  */
#line 2068 "lev_comp.y"
    {
		      check_vardef_type(variable_definitions, (yyvsp[(1) - (1)].map), SPOVAR_COORD);
		      vardef_used(variable_definitions, (yyvsp[(1) - (1)].map));
		      add_opvars(splev, "v", VA_PASS1((yyvsp[(1) - (1)].map)));
		      Free((yyvsp[(1) - (1)].map));
		  }
    break;

  case 312:

/* Line 1455 of yacc.c  */
#line 2075 "lev_comp.y"
    {
		      check_vardef_type(variable_definitions, (yyvsp[(1) - (4)].map), SPOVAR_COORD|SPOVAR_ARRAY);
		      vardef_used(variable_definitions, (yyvsp[(1) - (4)].map));
		      add_opvars(splev, "v", VA_PASS1((yyvsp[(1) - (4)].map)));
		      Free((yyvsp[(1) - (4)].map));
		  }
    break;

  case 313:

/* Line 1455 of yacc.c  */
#line 2084 "lev_comp.y"
    {
		      if ((yyvsp[(2) - (5)].i) < 0 || (yyvsp[(4) - (5)].i) < 0 || (yyvsp[(2) - (5)].i) >= COLNO || (yyvsp[(4) - (5)].i) >= ROWNO)
			  lc_error("Coordinates (%li,%li) out of map range!", (yyvsp[(2) - (5)].i), (yyvsp[(4) - (5)].i));
		      (yyval.i) = SP_COORD_PACK((yyvsp[(2) - (5)].i), (yyvsp[(4) - (5)].i));
		  }
    break;

  case 314:

/* Line 1455 of yacc.c  */
#line 2090 "lev_comp.y"
    {
		      (yyval.i) = SP_COORD_PACK_RANDOM(0);
		  }
    break;

  case 315:

/* Line 1455 of yacc.c  */
#line 2094 "lev_comp.y"
    {
		      (yyval.i) = SP_COORD_PACK_RANDOM( (yyvsp[(2) - (3)].i) );
		  }
    break;

  case 316:

/* Line 1455 of yacc.c  */
#line 2100 "lev_comp.y"
    {
		      (yyval.i) = (yyvsp[(1) - (1)].i);
		  }
    break;

  case 317:

/* Line 1455 of yacc.c  */
#line 2104 "lev_comp.y"
    {
		      if (((yyvsp[(1) - (3)].i) & (yyvsp[(3) - (3)].i)))
			  lc_warning("Humidity flag used twice.");
		      (yyval.i) = ((yyvsp[(1) - (3)].i) | (yyvsp[(3) - (3)].i));
		  }
    break;

  case 318:

/* Line 1455 of yacc.c  */
#line 2112 "lev_comp.y"
    {
		      /* nothing */
		  }
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 2116 "lev_comp.y"
    {
		      check_vardef_type(variable_definitions, (yyvsp[(1) - (1)].map), SPOVAR_REGION);
		      vardef_used(variable_definitions, (yyvsp[(1) - (1)].map));
		      add_opvars(splev, "v", VA_PASS1((yyvsp[(1) - (1)].map)));
		      Free((yyvsp[(1) - (1)].map));
		  }
    break;

  case 320:

/* Line 1455 of yacc.c  */
#line 2123 "lev_comp.y"
    {
		      check_vardef_type(variable_definitions, (yyvsp[(1) - (4)].map), SPOVAR_REGION|SPOVAR_ARRAY);
		      vardef_used(variable_definitions, (yyvsp[(1) - (4)].map));
		      add_opvars(splev, "v", VA_PASS1((yyvsp[(1) - (4)].map)));
		      Free((yyvsp[(1) - (4)].map));
		  }
    break;

  case 321:

/* Line 1455 of yacc.c  */
#line 2132 "lev_comp.y"
    {
		      long r = SP_REGION_PACK((yyvsp[(2) - (9)].i), (yyvsp[(4) - (9)].i), (yyvsp[(6) - (9)].i), (yyvsp[(8) - (9)].i));
		      if ( (yyvsp[(2) - (9)].i) > (yyvsp[(6) - (9)].i) || (yyvsp[(4) - (9)].i) > (yyvsp[(8) - (9)].i) )
			  lc_error("Region start > end: (%li,%li,%li,%li)!", (yyvsp[(2) - (9)].i), (yyvsp[(4) - (9)].i), (yyvsp[(6) - (9)].i), (yyvsp[(8) - (9)].i));

		      add_opvars(splev, "r", VA_PASS1(r));
		      (yyval.i) = r;
		  }
    break;

  case 322:

/* Line 1455 of yacc.c  */
#line 2143 "lev_comp.y"
    {
		      add_opvars(splev, "m", VA_PASS1((yyvsp[(1) - (1)].i)));
		  }
    break;

  case 323:

/* Line 1455 of yacc.c  */
#line 2147 "lev_comp.y"
    {
		      check_vardef_type(variable_definitions, (yyvsp[(1) - (1)].map), SPOVAR_MAPCHAR);
		      vardef_used(variable_definitions, (yyvsp[(1) - (1)].map));
		      add_opvars(splev, "v", VA_PASS1((yyvsp[(1) - (1)].map)));
		      Free((yyvsp[(1) - (1)].map));
		  }
    break;

  case 324:

/* Line 1455 of yacc.c  */
#line 2154 "lev_comp.y"
    {
		      check_vardef_type(variable_definitions, (yyvsp[(1) - (4)].map), SPOVAR_MAPCHAR|SPOVAR_ARRAY);
		      vardef_used(variable_definitions, (yyvsp[(1) - (4)].map));
		      add_opvars(splev, "v", VA_PASS1((yyvsp[(1) - (4)].map)));
		      Free((yyvsp[(1) - (4)].map));
		  }
    break;

  case 325:

/* Line 1455 of yacc.c  */
#line 2163 "lev_comp.y"
    {
		      if (what_map_char((char) (yyvsp[(1) - (1)].i)) != INVALID_TYPE)
			  (yyval.i) = SP_MAPCHAR_PACK(what_map_char((char) (yyvsp[(1) - (1)].i)), -2);
		      else {
			  lc_error("Unknown map char type '%c'!", (yyvsp[(1) - (1)].i));
			  (yyval.i) = SP_MAPCHAR_PACK(STONE, -2);
		      }
		  }
    break;

  case 326:

/* Line 1455 of yacc.c  */
#line 2172 "lev_comp.y"
    {
		      if (what_map_char((char) (yyvsp[(2) - (5)].i)) != INVALID_TYPE)
			  (yyval.i) = SP_MAPCHAR_PACK(what_map_char((char) (yyvsp[(2) - (5)].i)), (yyvsp[(4) - (5)].i));
		      else {
			  lc_error("Unknown map char type '%c'!", (yyvsp[(2) - (5)].i));
			  (yyval.i) = SP_MAPCHAR_PACK(STONE, (yyvsp[(4) - (5)].i));
		      }
		  }
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 2183 "lev_comp.y"
    {
		      add_opvars(splev, "M", VA_PASS1((yyvsp[(1) - (1)].i)));
		  }
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 2187 "lev_comp.y"
    {
		      check_vardef_type(variable_definitions, (yyvsp[(1) - (1)].map), SPOVAR_MONST);
		      vardef_used(variable_definitions, (yyvsp[(1) - (1)].map));
		      add_opvars(splev, "v", VA_PASS1((yyvsp[(1) - (1)].map)));
		      Free((yyvsp[(1) - (1)].map));
		  }
    break;

  case 329:

/* Line 1455 of yacc.c  */
#line 2194 "lev_comp.y"
    {
		      check_vardef_type(variable_definitions, (yyvsp[(1) - (4)].map), SPOVAR_MONST|SPOVAR_ARRAY);
		      vardef_used(variable_definitions, (yyvsp[(1) - (4)].map));
		      add_opvars(splev, "v", VA_PASS1((yyvsp[(1) - (4)].map)));
		      Free((yyvsp[(1) - (4)].map));
		  }
    break;

  case 330:

/* Line 1455 of yacc.c  */
#line 2203 "lev_comp.y"
    {
                      long m = get_monster_id((yyvsp[(1) - (1)].map), (char)0);
                      if (m == ERR) {
                          lc_error("Unknown monster \"%s\"!", (yyvsp[(1) - (1)].map));
                          (yyval.i) = -1;
                      } else
                          (yyval.i) = SP_MONST_PACK(m, def_monsyms[(int)mons[m].mlet].sym);
                  }
    break;

  case 331:

/* Line 1455 of yacc.c  */
#line 2212 "lev_comp.y"
    {
                        if (check_monster_char((char) (yyvsp[(1) - (1)].i)))
                            (yyval.i) = SP_MONST_PACK(-1, (yyvsp[(1) - (1)].i));
                        else {
                            lc_error("Unknown monster class '%c'!", (yyvsp[(1) - (1)].i));
                            (yyval.i) = -1;
                        }
                  }
    break;

  case 332:

/* Line 1455 of yacc.c  */
#line 2221 "lev_comp.y"
    {
                      long m = get_monster_id((yyvsp[(4) - (5)].map), (char) (yyvsp[(2) - (5)].i));
                      if (m == ERR) {
                          lc_error("Unknown monster ('%c', \"%s\")!", (yyvsp[(2) - (5)].i), (yyvsp[(4) - (5)].map));
                          (yyval.i) = -1;
                      } else
                          (yyval.i) = SP_MONST_PACK(m, (yyvsp[(2) - (5)].i));
                  }
    break;

  case 333:

/* Line 1455 of yacc.c  */
#line 2230 "lev_comp.y"
    {
                      (yyval.i) = -1;
                  }
    break;

  case 334:

/* Line 1455 of yacc.c  */
#line 2236 "lev_comp.y"
    {
		      add_opvars(splev, "O", VA_PASS1((yyvsp[(1) - (1)].i)));
		  }
    break;

  case 335:

/* Line 1455 of yacc.c  */
#line 2240 "lev_comp.y"
    {
		      check_vardef_type(variable_definitions, (yyvsp[(1) - (1)].map), SPOVAR_OBJ);
		      vardef_used(variable_definitions, (yyvsp[(1) - (1)].map));
		      add_opvars(splev, "v", VA_PASS1((yyvsp[(1) - (1)].map)));
		      Free((yyvsp[(1) - (1)].map));
		  }
    break;

  case 336:

/* Line 1455 of yacc.c  */
#line 2247 "lev_comp.y"
    {
		      check_vardef_type(variable_definitions, (yyvsp[(1) - (4)].map), SPOVAR_OBJ|SPOVAR_ARRAY);
		      vardef_used(variable_definitions, (yyvsp[(1) - (4)].map));
		      add_opvars(splev, "v", VA_PASS1((yyvsp[(1) - (4)].map)));
		      Free((yyvsp[(1) - (4)].map));
		  }
    break;

  case 337:

/* Line 1455 of yacc.c  */
#line 2256 "lev_comp.y"
    {
		      long m = get_object_id((yyvsp[(1) - (1)].map), (char)0);
		      if (m == ERR) {
			  lc_error("Unknown object \"%s\"!", (yyvsp[(1) - (1)].map));
			  (yyval.i) = -1;
		      } else
			  (yyval.i) = SP_OBJ_PACK(m, 1); /* obj class != 0 to force generation of a specific item */

		  }
    break;

  case 338:

/* Line 1455 of yacc.c  */
#line 2266 "lev_comp.y"
    {
			if (check_object_char((char) (yyvsp[(1) - (1)].i)))
			    (yyval.i) = SP_OBJ_PACK(-1, (yyvsp[(1) - (1)].i));
			else {
			    lc_error("Unknown object class '%c'!", (yyvsp[(1) - (1)].i));
			    (yyval.i) = -1;
			}
		  }
    break;

  case 339:

/* Line 1455 of yacc.c  */
#line 2275 "lev_comp.y"
    {
		      long m = get_object_id((yyvsp[(4) - (5)].map), (char) (yyvsp[(2) - (5)].i));
		      if (m == ERR) {
			  lc_error("Unknown object ('%c', \"%s\")!", (yyvsp[(2) - (5)].i), (yyvsp[(4) - (5)].map));
			  (yyval.i) = -1;
		      } else
			  (yyval.i) = SP_OBJ_PACK(m, (yyvsp[(2) - (5)].i));
		  }
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 2284 "lev_comp.y"
    {
		      (yyval.i) = -1;
		  }
    break;

  case 341:

/* Line 1455 of yacc.c  */
#line 2290 "lev_comp.y"
    { }
    break;

  case 342:

/* Line 1455 of yacc.c  */
#line 2292 "lev_comp.y"
    {
		      add_opvars(splev, "o", VA_PASS1(SPO_MATH_ADD));
		  }
    break;

  case 343:

/* Line 1455 of yacc.c  */
#line 2298 "lev_comp.y"
    {
		      add_opvars(splev, "i", VA_PASS1((yyvsp[(1) - (1)].i)));
		  }
    break;

  case 344:

/* Line 1455 of yacc.c  */
#line 2302 "lev_comp.y"
    {
		      is_inconstant_number = 1;
		  }
    break;

  case 345:

/* Line 1455 of yacc.c  */
#line 2306 "lev_comp.y"
    {
		      add_opvars(splev, "i", VA_PASS1((yyvsp[(2) - (3)].i)));
		  }
    break;

  case 346:

/* Line 1455 of yacc.c  */
#line 2310 "lev_comp.y"
    {
		      check_vardef_type(variable_definitions, (yyvsp[(1) - (1)].map), SPOVAR_INT);
		      vardef_used(variable_definitions, (yyvsp[(1) - (1)].map));
		      add_opvars(splev, "v", VA_PASS1((yyvsp[(1) - (1)].map)));
		      Free((yyvsp[(1) - (1)].map));
		      is_inconstant_number = 1;
		  }
    break;

  case 347:

/* Line 1455 of yacc.c  */
#line 2318 "lev_comp.y"
    {
		      check_vardef_type(variable_definitions,
					(yyvsp[(1) - (4)].map), SPOVAR_INT|SPOVAR_ARRAY);
		      vardef_used(variable_definitions, (yyvsp[(1) - (4)].map));
		      add_opvars(splev, "v", VA_PASS1((yyvsp[(1) - (4)].map)));
		      Free((yyvsp[(1) - (4)].map));
		      is_inconstant_number = 1;
		  }
    break;

  case 348:

/* Line 1455 of yacc.c  */
#line 2327 "lev_comp.y"
    {
		      add_opvars(splev, "o", VA_PASS1(SPO_MATH_ADD));
		  }
    break;

  case 349:

/* Line 1455 of yacc.c  */
#line 2331 "lev_comp.y"
    {
		      add_opvars(splev, "o", VA_PASS1(SPO_MATH_SUB));
		  }
    break;

  case 350:

/* Line 1455 of yacc.c  */
#line 2335 "lev_comp.y"
    {
		      add_opvars(splev, "o", VA_PASS1(SPO_MATH_MUL));
		  }
    break;

  case 351:

/* Line 1455 of yacc.c  */
#line 2339 "lev_comp.y"
    {
		      add_opvars(splev, "o", VA_PASS1(SPO_MATH_DIV));
		  }
    break;

  case 352:

/* Line 1455 of yacc.c  */
#line 2343 "lev_comp.y"
    {
		      add_opvars(splev, "o", VA_PASS1(SPO_MATH_MOD));
		  }
    break;

  case 353:

/* Line 1455 of yacc.c  */
#line 2346 "lev_comp.y"
    { }
    break;

  case 354:

/* Line 1455 of yacc.c  */
#line 2350 "lev_comp.y"
    {
		      if (!strcmp("int", (yyvsp[(1) - (1)].map)) || !strcmp("integer", (yyvsp[(1) - (1)].map))) {
			  (yyval.i) = (int)'i';
		      } else
			  lc_error("Unknown function parameter type '%s'", (yyvsp[(1) - (1)].map));
		  }
    break;

  case 355:

/* Line 1455 of yacc.c  */
#line 2357 "lev_comp.y"
    {
		      if (!strcmp("str", (yyvsp[(1) - (1)].map)) || !strcmp("string", (yyvsp[(1) - (1)].map))) {
			  (yyval.i) = (int)'s';
		      } else
			  lc_error("Unknown function parameter type '%s'", (yyvsp[(1) - (1)].map));
		  }
    break;

  case 356:

/* Line 1455 of yacc.c  */
#line 2366 "lev_comp.y"
    {
		      struct lc_funcdefs_parm *tmp = New(struct lc_funcdefs_parm);

		      if (!curr_function) {
			  lc_error("Function parameters outside function definition.");
		      } else if (!tmp) {
			  lc_error("Could not alloc function params.");
		      } else {
			  long vt;
			  tmp->name = strdup((yyvsp[(1) - (3)].map));
			  tmp->parmtype = (char) (yyvsp[(3) - (3)].i);
			  tmp->next = curr_function->params;
			  curr_function->params = tmp;
			  curr_function->n_params++;
			  switch (tmp->parmtype) {
			  case 'i': vt = SPOVAR_INT; break;
			  case 's': vt = SPOVAR_STRING; break;
			  default: lc_error("Unknown func param conversion."); break;
			  }
			  variable_definitions = add_vardef_type(
							 variable_definitions,
								 (yyvsp[(1) - (3)].map), vt);
		      }
		      Free((yyvsp[(1) - (3)].map));
		  }
    break;

  case 361:

/* Line 1455 of yacc.c  */
#line 2402 "lev_comp.y"
    {
			      (yyval.i) = (int)'i';
			  }
    break;

  case 362:

/* Line 1455 of yacc.c  */
#line 2406 "lev_comp.y"
    {
			      (yyval.i) = (int)'s';
			  }
    break;

  case 363:

/* Line 1455 of yacc.c  */
#line 2413 "lev_comp.y"
    {
			      char tmpbuf[2];
			      tmpbuf[0] = (char) (yyvsp[(1) - (1)].i);
			      tmpbuf[1] = '\0';
			      (yyval.map) = strdup(tmpbuf);
			  }
    break;

  case 364:

/* Line 1455 of yacc.c  */
#line 2420 "lev_comp.y"
    {
			      long len = strlen( (yyvsp[(1) - (3)].map) );
			      char *tmp = (char *)alloc(len + 2);
			      sprintf(tmp, "%c%s", (char) (yyvsp[(3) - (3)].i), (yyvsp[(1) - (3)].map) );
			      Free( (yyvsp[(1) - (3)].map) );
			      (yyval.map) = tmp;
			  }
    break;

  case 365:

/* Line 1455 of yacc.c  */
#line 2430 "lev_comp.y"
    {
			      (yyval.map) = strdup("");
			  }
    break;

  case 366:

/* Line 1455 of yacc.c  */
#line 2434 "lev_comp.y"
    {
			      char *tmp = strdup( (yyvsp[(1) - (1)].map) );
			      Free( (yyvsp[(1) - (1)].map) );
			      (yyval.map) = tmp;
			  }
    break;

  case 367:

/* Line 1455 of yacc.c  */
#line 2442 "lev_comp.y"
    {
		      add_opvars(splev, "o", VA_PASS1(SPO_SEL_POINT));
		  }
    break;

  case 368:

/* Line 1455 of yacc.c  */
#line 2446 "lev_comp.y"
    {
		      add_opvars(splev, "o", VA_PASS1(SPO_SEL_RECT));
		  }
    break;

  case 369:

/* Line 1455 of yacc.c  */
#line 2450 "lev_comp.y"
    {
		      add_opvars(splev, "o", VA_PASS1(SPO_SEL_FILLRECT));
		  }
    break;

  case 370:

/* Line 1455 of yacc.c  */
#line 2454 "lev_comp.y"
    {
		      add_opvars(splev, "o", VA_PASS1(SPO_SEL_LINE));
		  }
    break;

  case 371:

/* Line 1455 of yacc.c  */
#line 2458 "lev_comp.y"
    {
		      /* randline (x1,y1),(x2,y2), roughness */
		      add_opvars(splev, "o", VA_PASS1(SPO_SEL_RNDLINE));
		  }
    break;

  case 372:

/* Line 1455 of yacc.c  */
#line 2463 "lev_comp.y"
    {
		      add_opvars(splev, "io", VA_PASS2(W_ANY, SPO_SEL_GROW));
		  }
    break;

  case 373:

/* Line 1455 of yacc.c  */
#line 2467 "lev_comp.y"
    {
		      add_opvars(splev, "io", VA_PASS2((yyvsp[(3) - (6)].i), SPO_SEL_GROW));
		  }
    break;

  case 374:

/* Line 1455 of yacc.c  */
#line 2471 "lev_comp.y"
    {
		      add_opvars(splev, "iio",
			     VA_PASS3((yyvsp[(3) - (6)].i), SPOFILTER_PERCENT, SPO_SEL_FILTER));
		  }
    break;

  case 375:

/* Line 1455 of yacc.c  */
#line 2476 "lev_comp.y"
    {
		      add_opvars(splev, "io",
			       VA_PASS2(SPOFILTER_SELECTION, SPO_SEL_FILTER));
		  }
    break;

  case 376:

/* Line 1455 of yacc.c  */
#line 2481 "lev_comp.y"
    {
		      add_opvars(splev, "io",
				 VA_PASS2(SPOFILTER_MAPCHAR, SPO_SEL_FILTER));
		  }
    break;

  case 377:

/* Line 1455 of yacc.c  */
#line 2486 "lev_comp.y"
    {
		      add_opvars(splev, "o", VA_PASS1(SPO_SEL_FLOOD));
		  }
    break;

  case 378:

/* Line 1455 of yacc.c  */
#line 2490 "lev_comp.y"
    {
		      add_opvars(splev, "oio",
				 VA_PASS3(SPO_COPY, 1, SPO_SEL_ELLIPSE));
		  }
    break;

  case 379:

/* Line 1455 of yacc.c  */
#line 2495 "lev_comp.y"
    {
		      add_opvars(splev, "oio",
				 VA_PASS3(SPO_COPY, (yyvsp[(7) - (8)].i), SPO_SEL_ELLIPSE));
		  }
    break;

  case 380:

/* Line 1455 of yacc.c  */
#line 2500 "lev_comp.y"
    {
		      add_opvars(splev, "io", VA_PASS2(1, SPO_SEL_ELLIPSE));
		  }
    break;

  case 381:

/* Line 1455 of yacc.c  */
#line 2504 "lev_comp.y"
    {
		      add_opvars(splev, "io", VA_PASS2((yyvsp[(9) - (10)].i), SPO_SEL_ELLIPSE));
		  }
    break;

  case 382:

/* Line 1455 of yacc.c  */
#line 2508 "lev_comp.y"
    {
		      add_opvars(splev, "iio",
				 VA_PASS3((yyvsp[(9) - (14)].i), (yyvsp[(3) - (14)].i), SPO_SEL_GRADIENT));
		  }
    break;

  case 383:

/* Line 1455 of yacc.c  */
#line 2513 "lev_comp.y"
    {
		      add_opvars(splev, "o", VA_PASS1(SPO_SEL_COMPLEMENT));
		  }
    break;

  case 384:

/* Line 1455 of yacc.c  */
#line 2517 "lev_comp.y"
    {
		      check_vardef_type(variable_definitions, (yyvsp[(1) - (1)].map), SPOVAR_SEL);
		      vardef_used(variable_definitions, (yyvsp[(1) - (1)].map));
		      add_opvars(splev, "v", VA_PASS1((yyvsp[(1) - (1)].map)));
		      Free((yyvsp[(1) - (1)].map));
		  }
    break;

  case 385:

/* Line 1455 of yacc.c  */
#line 2524 "lev_comp.y"
    {
		      /* nothing */
		  }
    break;

  case 386:

/* Line 1455 of yacc.c  */
#line 2530 "lev_comp.y"
    {
		      /* nothing */
		  }
    break;

  case 387:

/* Line 1455 of yacc.c  */
#line 2534 "lev_comp.y"
    {
		      add_opvars(splev, "o", VA_PASS1(SPO_SEL_ADD));
		  }
    break;

  case 388:

/* Line 1455 of yacc.c  */
#line 2540 "lev_comp.y"
    {
		      add_opvars(splev, "iio",
				 VA_PASS3((yyvsp[(1) - (1)].dice).num, (yyvsp[(1) - (1)].dice).die, SPO_DICE));
		  }
    break;

  case 392:

/* Line 1455 of yacc.c  */
#line 2552 "lev_comp.y"
    {
		      add_opvars(splev, "i", VA_PASS1((yyvsp[(1) - (1)].i)));
		  }
    break;

  case 393:

/* Line 1455 of yacc.c  */
#line 2556 "lev_comp.y"
    {
		      add_opvars(splev, "i", VA_PASS1((yyvsp[(1) - (1)].i)));
		  }
    break;

  case 394:

/* Line 1455 of yacc.c  */
#line 2560 "lev_comp.y"
    {
		      add_opvars(splev, "i", VA_PASS1((yyvsp[(1) - (1)].i)));
		  }
    break;

  case 395:

/* Line 1455 of yacc.c  */
#line 2564 "lev_comp.y"
    {
		      /* nothing */
		  }
    break;

  case 404:

/* Line 1455 of yacc.c  */
#line 2586 "lev_comp.y"
    {
			(yyval.lregn) = (yyvsp[(1) - (1)].lregn);
		  }
    break;

  case 405:

/* Line 1455 of yacc.c  */
#line 2590 "lev_comp.y"
    {
			if ((yyvsp[(3) - (10)].i) <= 0 || (yyvsp[(3) - (10)].i) >= COLNO)
			    lc_error("Region (%li,%li,%li,%li) out of level range (x1)!", (yyvsp[(3) - (10)].i), (yyvsp[(5) - (10)].i), (yyvsp[(7) - (10)].i), (yyvsp[(9) - (10)].i));
			else if ((yyvsp[(5) - (10)].i) < 0 || (yyvsp[(5) - (10)].i) >= ROWNO)
			    lc_error("Region (%li,%li,%li,%li) out of level range (y1)!", (yyvsp[(3) - (10)].i), (yyvsp[(5) - (10)].i), (yyvsp[(7) - (10)].i), (yyvsp[(9) - (10)].i));
			else if ((yyvsp[(7) - (10)].i) <= 0 || (yyvsp[(7) - (10)].i) >= COLNO)
			    lc_error("Region (%li,%li,%li,%li) out of level range (x2)!", (yyvsp[(3) - (10)].i), (yyvsp[(5) - (10)].i), (yyvsp[(7) - (10)].i), (yyvsp[(9) - (10)].i));
			else if ((yyvsp[(9) - (10)].i) < 0 || (yyvsp[(9) - (10)].i) >= ROWNO)
			    lc_error("Region (%li,%li,%li,%li) out of level range (y2)!", (yyvsp[(3) - (10)].i), (yyvsp[(5) - (10)].i), (yyvsp[(7) - (10)].i), (yyvsp[(9) - (10)].i));
			(yyval.lregn).x1 = (yyvsp[(3) - (10)].i);
			(yyval.lregn).y1 = (yyvsp[(5) - (10)].i);
			(yyval.lregn).x2 = (yyvsp[(7) - (10)].i);
			(yyval.lregn).y2 = (yyvsp[(9) - (10)].i);
			(yyval.lregn).area = 1;
		  }
    break;

  case 406:

/* Line 1455 of yacc.c  */
#line 2608 "lev_comp.y"
    {
/* This series of if statements is a hack for MSC 5.1.  It seems that its
   tiny little brain cannot compile if these are all one big if statement. */
			if ((yyvsp[(2) - (9)].i) < 0 || (yyvsp[(2) - (9)].i) > (int)max_x_map)
			    lc_error("Region (%li,%li,%li,%li) out of map range (x1)!", (yyvsp[(2) - (9)].i), (yyvsp[(4) - (9)].i), (yyvsp[(6) - (9)].i), (yyvsp[(8) - (9)].i));
			else if ((yyvsp[(4) - (9)].i) < 0 || (yyvsp[(4) - (9)].i) > (int)max_y_map)
			    lc_error("Region (%li,%li,%li,%li) out of map range (y1)!", (yyvsp[(2) - (9)].i), (yyvsp[(4) - (9)].i), (yyvsp[(6) - (9)].i), (yyvsp[(8) - (9)].i));
			else if ((yyvsp[(6) - (9)].i) < 0 || (yyvsp[(6) - (9)].i) > (int)max_x_map)
			    lc_error("Region (%li,%li,%li,%li) out of map range (x2)!", (yyvsp[(2) - (9)].i), (yyvsp[(4) - (9)].i), (yyvsp[(6) - (9)].i), (yyvsp[(8) - (9)].i));
			else if ((yyvsp[(8) - (9)].i) < 0 || (yyvsp[(8) - (9)].i) > (int)max_y_map)
			    lc_error("Region (%li,%li,%li,%li) out of map range (y2)!", (yyvsp[(2) - (9)].i), (yyvsp[(4) - (9)].i), (yyvsp[(6) - (9)].i), (yyvsp[(8) - (9)].i));
			(yyval.lregn).area = 0;
			(yyval.lregn).x1 = (yyvsp[(2) - (9)].i);
			(yyval.lregn).y1 = (yyvsp[(4) - (9)].i);
			(yyval.lregn).x2 = (yyvsp[(6) - (9)].i);
			(yyval.lregn).y2 = (yyvsp[(8) - (9)].i);
		  }
    break;



/* Line 1455 of yacc.c  */
#line 6144 "y.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 2628 "lev_comp.y"


/*lev_comp.y*/

