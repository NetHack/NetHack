/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

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
#define YYBISON_VERSION "3.0.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 1 "lev_comp.y" /* yacc.c:339  */

/* NetHack 3.5  lev_comp.y	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.5  lev_comp.y	$Date: 2009/05/06 10:54:31 $  $Revision: 1.8 $ */
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


#line 212 "y.tab.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
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

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 148 "lev_comp.y" /* yacc.c:355  */

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

#line 641 "y.tab.c" /* yacc.c:355  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_Y_TAB_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 656 "y.tab.c" /* yacc.c:358  */

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
#else
typedef signed char yytype_int8;
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
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
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
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
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

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  9
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1031

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  194
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  160
/* YYNRULES -- Number of rules.  */
#define YYNRULES  406
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  866

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   431

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
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
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   275,   275,   276,   279,   280,   283,   305,   310,   326,
     330,   336,   345,   354,   358,   384,   387,   394,   398,   405,
     408,   415,   416,   420,   423,   429,   433,   440,   443,   449,
     455,   456,   457,   458,   459,   460,   461,   462,   463,   464,
     465,   466,   467,   468,   469,   470,   471,   472,   473,   474,
     475,   476,   477,   478,   479,   480,   481,   482,   483,   484,
     485,   486,   487,   488,   489,   490,   491,   492,   493,   494,
     495,   496,   497,   498,   501,   502,   503,   504,   505,   506,
     507,   508,   509,   512,   513,   514,   515,   516,   517,   518,
     519,   520,   523,   524,   525,   528,   529,   532,   544,   550,
     556,   562,   568,   574,   580,   586,   592,   599,   606,   613,
     620,   627,   634,   643,   648,   655,   660,   667,   672,   679,
     683,   689,   694,   701,   705,   711,   715,   722,   744,   721,
     758,   803,   810,   813,   819,   825,   829,   838,   842,   837,
     899,   900,   904,   903,   916,   915,   930,   940,   941,   944,
     977,   976,  1002,  1001,  1031,  1030,  1061,  1060,  1086,  1095,
    1094,  1121,  1127,  1131,  1135,  1141,  1148,  1157,  1165,  1176,
    1175,  1191,  1190,  1207,  1210,  1216,  1226,  1232,  1241,  1247,
    1252,  1258,  1263,  1269,  1278,  1284,  1285,  1288,  1289,  1292,
    1296,  1302,  1303,  1306,  1312,  1318,  1326,  1327,  1330,  1331,
    1334,  1339,  1338,  1352,  1359,  1365,  1373,  1378,  1383,  1388,
    1393,  1398,  1403,  1408,  1413,  1418,  1423,  1428,  1433,  1438,
    1443,  1448,  1455,  1462,  1466,  1479,  1486,  1485,  1501,  1509,
    1515,  1523,  1528,  1533,  1538,  1543,  1548,  1553,  1558,  1563,
    1568,  1579,  1584,  1589,  1594,  1599,  1606,  1612,  1639,  1644,
    1651,  1655,  1661,  1667,  1673,  1683,  1693,  1708,  1718,  1721,
    1727,  1733,  1739,  1745,  1750,  1757,  1763,  1769,  1775,  1782,
    1781,  1805,  1808,  1814,  1820,  1824,  1829,  1836,  1842,  1849,
    1853,  1859,  1867,  1870,  1880,  1884,  1887,  1893,  1897,  1904,
    1908,  1912,  1918,  1919,  1922,  1923,  1926,  1927,  1928,  1934,
    1935,  1936,  1942,  1943,  1946,  1955,  1960,  1967,  1977,  1983,
    1987,  1991,  1998,  2007,  2013,  2017,  2023,  2027,  2035,  2039,
    2046,  2055,  2066,  2070,  2077,  2086,  2095,  2106,  2110,  2117,
    2126,  2135,  2144,  2153,  2159,  2163,  2170,  2179,  2189,  2198,
    2207,  2214,  2215,  2221,  2222,  2223,  2224,  2232,  2240,  2241,
    2242,  2243,  2244,  2245,  2248,  2254,  2262,  2291,  2292,  2295,
    2296,  2299,  2303,  2310,  2317,  2328,  2331,  2339,  2343,  2347,
    2351,  2355,  2360,  2364,  2368,  2372,  2376,  2380,  2384,  2388,
    2392,  2396,  2400,  2404,  2408,  2415,  2421,  2425,  2431,  2437,
    2438,  2439,  2442,  2446,  2450,  2454,  2460,  2461,  2464,  2465,
    2468,  2469,  2472,  2473,  2476,  2480,  2498
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
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
  "lev_region", "region", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
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

#define YYPACT_NINF -654

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-654)))

#define YYTABLE_NINF -202

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
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
     -18,   -18,   672,   663,   664,  -654,  -654,   541,   544,   521,
    -654,   -18,   -18,   307,  -654,   595,   590,   672,   597,    66,
      67,   186,   659,   730,   604,   669,    -1,     8,  -654,   606,
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

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
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
    -312,   583,   150,  -308,  -386,  -485,  -418,  -473,   596,  -459,
    -132,   -55,  -654,   396,  -654,  -654,   610,  -654,  -654,   781,
    -135,   576,  -392,  -654,  -654,  -654,  -654,  -654,  -124,  -654
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

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
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
     457,   463,   464,   480,   481,   864,   493,   465,   483,   819,
     466,   202,   203,   193,   494,   495,   496,   501,   502,    16,
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
     847,   838,   731,   437,   668,   698,   556,   547,     0,   535,
     374,   581
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
     131,    58,    58,   128,   134,   861,    67,   186,   131,   784,
     186,    59,    60,   139,     4,   131,    67,   131,   131,     6,
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
     833,   821,   653,   341,   603,   629,   443,   431,    -1,   419,
     249,   455
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
     323,   323,   344,    58,    58,   186,   186,    58,   267,   344,
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

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
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


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

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
#ifndef YYINITDEPTH
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
static YYSIZE_T
yystrlen (const char *yystr)
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
static char *
yystpcpy (char *yydest, const char *yysrc)
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

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
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
  int yytoken = 0;
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

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
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
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
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
      if (yytable_value_is_error (yyn))
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
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

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
     '$$ = $1'.

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
#line 284 "lev_comp.y" /* yacc.c:1646  */
    {
			if (fatal_error > 0) {
				(void) fprintf(stderr,
				"%s: %d errors detected for level \"%s\". No output created!\n",
					       fname, fatal_error, (yyvsp[-2].map));
				fatal_error = 0;
				got_errors++;
			} else if (!got_errors) {
				if (!write_level_file((yyvsp[-2].map), splev)) {
				    lc_error("Can't write output file for '%s'!", (yyvsp[-2].map));
				    exit(EXIT_FAILURE);
				}
			}
			Free((yyvsp[-2].map));
			Free(splev);
			splev = NULL;
			vardef_free_all(variable_definitions);
			variable_definitions = NULL;
		  }
#line 2466 "y.tab.c" /* yacc.c:1646  */
    break;

  case 7:
#line 306 "lev_comp.y" /* yacc.c:1646  */
    {
		      start_level_def(&splev, (yyvsp[0].map));
		      (yyval.map) = (yyvsp[0].map);
		  }
#line 2475 "y.tab.c" /* yacc.c:1646  */
    break;

  case 8:
#line 311 "lev_comp.y" /* yacc.c:1646  */
    {
		      start_level_def(&splev, (yyvsp[-2].map));
		      if ((yyvsp[0].i) == -1) {
			  add_opvars(splev, "iiiiiiiio", LVLINIT_MAZEGRID,HWALL,0,0, 0,0,0,0, SPO_INITLEVEL);
		      } else {
			  long bg = what_map_char((char) (yyvsp[0].i));
			  add_opvars(splev, "iiiiiiiio", LVLINIT_SOLIDFILL, bg, 0,0, 0,0,0,0, SPO_INITLEVEL);
		      }
		      add_opvars(splev, "io", MAZELEVEL, SPO_LEVEL_FLAGS);
		      max_x_map = COLNO-1;
		      max_y_map = ROWNO;
		      (yyval.map) = (yyvsp[-2].map);
		  }
#line 2493 "y.tab.c" /* yacc.c:1646  */
    break;

  case 9:
#line 327 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = -1;
		  }
#line 2501 "y.tab.c" /* yacc.c:1646  */
    break;

  case 10:
#line 331 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = what_map_char((char) (yyvsp[0].i));
		  }
#line 2509 "y.tab.c" /* yacc.c:1646  */
    break;

  case 11:
#line 337 "lev_comp.y" /* yacc.c:1646  */
    {
		      long filling = (yyvsp[0].terr).ter;
		      if (filling == INVALID_TYPE || filling >= MAX_TYPE)
			  lc_error("INIT_MAP: Invalid fill char type.");
		      add_opvars(splev, "iiiiiiiio", LVLINIT_SOLIDFILL,filling,0,(long)(yyvsp[0].terr).lit, 0,0,0,0, SPO_INITLEVEL);
		      max_x_map = COLNO-1;
		      max_y_map = ROWNO;
		  }
#line 2522 "y.tab.c" /* yacc.c:1646  */
    break;

  case 12:
#line 346 "lev_comp.y" /* yacc.c:1646  */
    {
		      long filling = what_map_char((char) (yyvsp[0].i));
		      if (filling == INVALID_TYPE || filling >= MAX_TYPE)
			  lc_error("INIT_MAP: Invalid fill char type.");
		      add_opvars(splev, "iiiiiiiio", LVLINIT_MAZEGRID,filling,0,0, 0,0,0,0, SPO_INITLEVEL);
		      max_x_map = COLNO-1;
		      max_y_map = ROWNO;
		  }
#line 2535 "y.tab.c" /* yacc.c:1646  */
    break;

  case 13:
#line 355 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "iiiiiiiio", LVLINIT_ROGUE,0,0,0,0,0,0,0, SPO_INITLEVEL);
		  }
#line 2543 "y.tab.c" /* yacc.c:1646  */
    break;

  case 14:
#line 359 "lev_comp.y" /* yacc.c:1646  */
    {
		      long fg = what_map_char((char) (yyvsp[-11].i));
		      long bg = what_map_char((char) (yyvsp[-9].i));
		      long smoothed = (yyvsp[-7].i);
		      long joined = (yyvsp[-5].i);
		      long lit = (yyvsp[-3].i);
		      long walled = (yyvsp[-1].i);
		      long filling = (yyvsp[0].i);
		      if (fg == INVALID_TYPE || fg >= MAX_TYPE)
			  lc_error("INIT_MAP: Invalid foreground type.");
		      if (bg == INVALID_TYPE || bg >= MAX_TYPE)
			  lc_error("INIT_MAP: Invalid background type.");
		      if (joined && fg != CORR && fg != ROOM)
			  lc_error("INIT_MAP: Invalid foreground type for joined map.");

		      if (filling == INVALID_TYPE)
			  lc_error("INIT_MAP: Invalid fill char type.");

		      add_opvars(splev, "iiiiiiiio", LVLINIT_MINES,filling,walled,lit, joined,smoothed,bg,fg, SPO_INITLEVEL);
			max_x_map = COLNO-1;
			max_y_map = ROWNO;
		  }
#line 2570 "y.tab.c" /* yacc.c:1646  */
    break;

  case 15:
#line 384 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = 0;
		  }
#line 2578 "y.tab.c" /* yacc.c:1646  */
    break;

  case 16:
#line 388 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = (yyvsp[0].i);
		  }
#line 2586 "y.tab.c" /* yacc.c:1646  */
    break;

  case 17:
#line 394 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "o", SPO_COPY);
		      (yyval.i) = 0;
		  }
#line 2595 "y.tab.c" /* yacc.c:1646  */
    break;

  case 18:
#line 399 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = 1;
		  }
#line 2603 "y.tab.c" /* yacc.c:1646  */
    break;

  case 19:
#line 405 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = -1;
		  }
#line 2611 "y.tab.c" /* yacc.c:1646  */
    break;

  case 20:
#line 409 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = what_map_char((char) (yyvsp[0].i));
		  }
#line 2619 "y.tab.c" /* yacc.c:1646  */
    break;

  case 23:
#line 420 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "io", 0, SPO_LEVEL_FLAGS);
		  }
#line 2627 "y.tab.c" /* yacc.c:1646  */
    break;

  case 24:
#line 424 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "io", (yyvsp[0].i), SPO_LEVEL_FLAGS);
		  }
#line 2635 "y.tab.c" /* yacc.c:1646  */
    break;

  case 25:
#line 430 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = ((yyvsp[-2].i) | (yyvsp[0].i));
		  }
#line 2643 "y.tab.c" /* yacc.c:1646  */
    break;

  case 26:
#line 434 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = (yyvsp[0].i);
		  }
#line 2651 "y.tab.c" /* yacc.c:1646  */
    break;

  case 27:
#line 440 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = 0;
		  }
#line 2659 "y.tab.c" /* yacc.c:1646  */
    break;

  case 28:
#line 444 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = 1 + (yyvsp[0].i);
		  }
#line 2667 "y.tab.c" /* yacc.c:1646  */
    break;

  case 29:
#line 450 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = (yyvsp[-1].i);
		  }
#line 2675 "y.tab.c" /* yacc.c:1646  */
    break;

  case 97:
#line 533 "lev_comp.y" /* yacc.c:1646  */
    {
		      struct lc_vardefs *vd;
		      if ((vd = vardef_defined(variable_definitions, (yyvsp[0].map), 1))) {
			  if (!(vd->var_type & SPOVAR_ARRAY))
			      lc_error("Trying to shuffle non-array variable '%s'", (yyvsp[0].map));
		      } else lc_error("Trying to shuffle undefined variable '%s'", (yyvsp[0].map));
		      add_opvars(splev, "so", (yyvsp[0].map), SPO_SHUFFLE_ARRAY);
		      Free((yyvsp[0].map));
		  }
#line 2689 "y.tab.c" /* yacc.c:1646  */
    break;

  case 98:
#line 545 "lev_comp.y" /* yacc.c:1646  */
    {
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[-2].map), SPOVAR_INT);
		      add_opvars(splev, "iso", 0, (yyvsp[-2].map), SPO_VAR_INIT);
		      Free((yyvsp[-2].map));
		  }
#line 2699 "y.tab.c" /* yacc.c:1646  */
    break;

  case 99:
#line 551 "lev_comp.y" /* yacc.c:1646  */
    {
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[-4].map), SPOVAR_SEL);
		      add_opvars(splev, "iso", 0, (yyvsp[-4].map), SPO_VAR_INIT);
		      Free((yyvsp[-4].map));
		  }
#line 2709 "y.tab.c" /* yacc.c:1646  */
    break;

  case 100:
#line 557 "lev_comp.y" /* yacc.c:1646  */
    {
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[-2].map), SPOVAR_STRING);
		      add_opvars(splev, "iso", 0, (yyvsp[-2].map), SPO_VAR_INIT);
		      Free((yyvsp[-2].map));
		  }
#line 2719 "y.tab.c" /* yacc.c:1646  */
    break;

  case 101:
#line 563 "lev_comp.y" /* yacc.c:1646  */
    {
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[-4].map), SPOVAR_MAPCHAR);
		      add_opvars(splev, "iso", 0, (yyvsp[-4].map), SPO_VAR_INIT);
		      Free((yyvsp[-4].map));
		  }
#line 2729 "y.tab.c" /* yacc.c:1646  */
    break;

  case 102:
#line 569 "lev_comp.y" /* yacc.c:1646  */
    {
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[-4].map), SPOVAR_MONST);
		      add_opvars(splev, "iso", 0, (yyvsp[-4].map), SPO_VAR_INIT);
		      Free((yyvsp[-4].map));
		  }
#line 2739 "y.tab.c" /* yacc.c:1646  */
    break;

  case 103:
#line 575 "lev_comp.y" /* yacc.c:1646  */
    {
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[-4].map), SPOVAR_OBJ);
		      add_opvars(splev, "iso", 0, (yyvsp[-4].map), SPO_VAR_INIT);
		      Free((yyvsp[-4].map));
		  }
#line 2749 "y.tab.c" /* yacc.c:1646  */
    break;

  case 104:
#line 581 "lev_comp.y" /* yacc.c:1646  */
    {
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[-2].map), SPOVAR_COORD);
		      add_opvars(splev, "iso", 0, (yyvsp[-2].map), SPO_VAR_INIT);
		      Free((yyvsp[-2].map));
		  }
#line 2759 "y.tab.c" /* yacc.c:1646  */
    break;

  case 105:
#line 587 "lev_comp.y" /* yacc.c:1646  */
    {
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[-2].map), SPOVAR_REGION);
		      add_opvars(splev, "iso", 0, (yyvsp[-2].map), SPO_VAR_INIT);
		      Free((yyvsp[-2].map));
		  }
#line 2769 "y.tab.c" /* yacc.c:1646  */
    break;

  case 106:
#line 593 "lev_comp.y" /* yacc.c:1646  */
    {
		      long n_items = (yyvsp[-1].i);
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[-4].map), SPOVAR_INT|SPOVAR_ARRAY);
		      add_opvars(splev, "iso", n_items, (yyvsp[-4].map), SPO_VAR_INIT);
		      Free((yyvsp[-4].map));
		  }
#line 2780 "y.tab.c" /* yacc.c:1646  */
    break;

  case 107:
#line 600 "lev_comp.y" /* yacc.c:1646  */
    {
		      long n_items = (yyvsp[-1].i);
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[-4].map), SPOVAR_COORD|SPOVAR_ARRAY);
		      add_opvars(splev, "iso", n_items, (yyvsp[-4].map), SPO_VAR_INIT);
		      Free((yyvsp[-4].map));
		  }
#line 2791 "y.tab.c" /* yacc.c:1646  */
    break;

  case 108:
#line 607 "lev_comp.y" /* yacc.c:1646  */
    {
		      long n_items = (yyvsp[-1].i);
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[-4].map), SPOVAR_REGION|SPOVAR_ARRAY);
		      add_opvars(splev, "iso", n_items, (yyvsp[-4].map), SPO_VAR_INIT);
		      Free((yyvsp[-4].map));
		  }
#line 2802 "y.tab.c" /* yacc.c:1646  */
    break;

  case 109:
#line 614 "lev_comp.y" /* yacc.c:1646  */
    {
		      long n_items = (yyvsp[-1].i);
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[-6].map), SPOVAR_MAPCHAR|SPOVAR_ARRAY);
		      add_opvars(splev, "iso", n_items, (yyvsp[-6].map), SPO_VAR_INIT);
		      Free((yyvsp[-6].map));
		  }
#line 2813 "y.tab.c" /* yacc.c:1646  */
    break;

  case 110:
#line 621 "lev_comp.y" /* yacc.c:1646  */
    {
		      long n_items = (yyvsp[-1].i);
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[-6].map), SPOVAR_MONST|SPOVAR_ARRAY);
		      add_opvars(splev, "iso", n_items, (yyvsp[-6].map), SPO_VAR_INIT);
		      Free((yyvsp[-6].map));
		  }
#line 2824 "y.tab.c" /* yacc.c:1646  */
    break;

  case 111:
#line 628 "lev_comp.y" /* yacc.c:1646  */
    {
		      long n_items = (yyvsp[-1].i);
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[-6].map), SPOVAR_OBJ|SPOVAR_ARRAY);
		      add_opvars(splev, "iso", n_items, (yyvsp[-6].map), SPO_VAR_INIT);
		      Free((yyvsp[-6].map));
		  }
#line 2835 "y.tab.c" /* yacc.c:1646  */
    break;

  case 112:
#line 635 "lev_comp.y" /* yacc.c:1646  */
    {
		      long n_items = (yyvsp[-1].i);
		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[-4].map), SPOVAR_STRING|SPOVAR_ARRAY);
		      add_opvars(splev, "iso", n_items, (yyvsp[-4].map), SPO_VAR_INIT);
		      Free((yyvsp[-4].map));
		  }
#line 2846 "y.tab.c" /* yacc.c:1646  */
    break;

  case 113:
#line 644 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "O", (yyvsp[0].i));
		      (yyval.i) = 1;
		  }
#line 2855 "y.tab.c" /* yacc.c:1646  */
    break;

  case 114:
#line 649 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "O", (yyvsp[0].i));
		      (yyval.i) = 1 + (yyvsp[-2].i);
		  }
#line 2864 "y.tab.c" /* yacc.c:1646  */
    break;

  case 115:
#line 656 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "M", (yyvsp[0].i));
		      (yyval.i) = 1;
		  }
#line 2873 "y.tab.c" /* yacc.c:1646  */
    break;

  case 116:
#line 661 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "M", (yyvsp[0].i));
		      (yyval.i) = 1 + (yyvsp[-2].i);
		  }
#line 2882 "y.tab.c" /* yacc.c:1646  */
    break;

  case 117:
#line 668 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "m", (yyvsp[0].i));
		      (yyval.i) = 1;
		  }
#line 2891 "y.tab.c" /* yacc.c:1646  */
    break;

  case 118:
#line 673 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "m", (yyvsp[0].i));
		      (yyval.i) = 1 + (yyvsp[-2].i);
		  }
#line 2900 "y.tab.c" /* yacc.c:1646  */
    break;

  case 119:
#line 680 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = 1;
		  }
#line 2908 "y.tab.c" /* yacc.c:1646  */
    break;

  case 120:
#line 684 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = 1 + (yyvsp[-2].i);
		  }
#line 2916 "y.tab.c" /* yacc.c:1646  */
    break;

  case 121:
#line 690 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "c", (yyvsp[0].i));
		      (yyval.i) = 1;
		  }
#line 2925 "y.tab.c" /* yacc.c:1646  */
    break;

  case 122:
#line 695 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "c", (yyvsp[0].i));
		      (yyval.i) = 1 + (yyvsp[-2].i);
		  }
#line 2934 "y.tab.c" /* yacc.c:1646  */
    break;

  case 123:
#line 702 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = 1;
		  }
#line 2942 "y.tab.c" /* yacc.c:1646  */
    break;

  case 124:
#line 706 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = 1 + (yyvsp[-2].i);
		  }
#line 2950 "y.tab.c" /* yacc.c:1646  */
    break;

  case 125:
#line 712 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = 1;
		  }
#line 2958 "y.tab.c" /* yacc.c:1646  */
    break;

  case 126:
#line 716 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = 1 + (yyvsp[-2].i);
		  }
#line 2966 "y.tab.c" /* yacc.c:1646  */
    break;

  case 127:
#line 722 "lev_comp.y" /* yacc.c:1646  */
    {
		      struct lc_funcdefs *funcdef;

		      if (in_function_definition)
			  lc_error("Recursively defined functions not allowed (function %s).", (yyvsp[-1].map));

		      in_function_definition++;

		      if (funcdef_defined(function_definitions, (yyvsp[-1].map), 1))
			  lc_error("Function '%s' already defined once.", (yyvsp[-1].map));

		      funcdef = funcdef_new(-1, (yyvsp[-1].map));
		      funcdef->next = function_definitions;
		      function_definitions = funcdef;
		      function_splev_backup = splev;
		      splev = &(funcdef->code);
		      Free((yyvsp[-1].map));
		      curr_function = funcdef;
		      function_tmp_var_defs = variable_definitions;
		      variable_definitions = NULL;
		  }
#line 2992 "y.tab.c" /* yacc.c:1646  */
    break;

  case 128:
#line 744 "lev_comp.y" /* yacc.c:1646  */
    {
		      /* nothing */
		  }
#line 3000 "y.tab.c" /* yacc.c:1646  */
    break;

  case 129:
#line 748 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "io", 0, SPO_RETURN);
		      splev = function_splev_backup;
		      in_function_definition--;
		      curr_function = NULL;
		      vardef_free_all(variable_definitions);
		      variable_definitions = function_tmp_var_defs;
		  }
#line 3013 "y.tab.c" /* yacc.c:1646  */
    break;

  case 130:
#line 759 "lev_comp.y" /* yacc.c:1646  */
    {
		      struct lc_funcdefs *tmpfunc;
		      tmpfunc = funcdef_defined(function_definitions, (yyvsp[-3].map), 1);
		      if (tmpfunc) {
			  long l;
			  long nparams = strlen( (yyvsp[-1].map) );
			  char *fparamstr = funcdef_paramtypes(tmpfunc);
			  if (strcmp((yyvsp[-1].map), fparamstr)) {
			      char *tmps = strdup(decode_parm_str(fparamstr));
			      lc_error("Function '%s' requires params '%s', got '%s' instead.", (yyvsp[-3].map), tmps, decode_parm_str((yyvsp[-1].map)));
			      Free(tmps);
			  }
			  Free(fparamstr);
			  Free((yyvsp[-1].map));
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
				      add_opvars(splev, "iso", 0, tfp->name, SPO_VAR_INIT);
				      tfp = tfp->next;
				  }
			      }

			      splev_add_from(splev, &(tmpfunc->code));
			      set_opvar_int(jmp, splev->n_opcodes - jmp->vardata.l);
			  }
			  l = tmpfunc->addr - splev->n_opcodes - 2;
			  add_opvars(splev, "iio", nparams, l, SPO_CALL);
			  tmpfunc->n_called++;
		      } else {
			  lc_error("Function '%s' not defined.", (yyvsp[-3].map));
		      }
		      Free((yyvsp[-3].map));
		  }
#line 3060 "y.tab.c" /* yacc.c:1646  */
    break;

  case 131:
#line 804 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opcode(splev, SPO_EXIT, NULL);
		  }
#line 3068 "y.tab.c" /* yacc.c:1646  */
    break;

  case 132:
#line 810 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = 100;
		  }
#line 3076 "y.tab.c" /* yacc.c:1646  */
    break;

  case 133:
#line 814 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = (yyvsp[0].i);
		  }
#line 3084 "y.tab.c" /* yacc.c:1646  */
    break;

  case 134:
#line 820 "lev_comp.y" /* yacc.c:1646  */
    {
		      /* val > rn2(100) */
		      add_opvars(splev, "iio", (long)(yyvsp[0].i), 100, SPO_RN2);
		      (yyval.i) = SPO_JG;
                  }
#line 3094 "y.tab.c" /* yacc.c:1646  */
    break;

  case 135:
#line 826 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = (yyvsp[-2].i);
                  }
#line 3102 "y.tab.c" /* yacc.c:1646  */
    break;

  case 136:
#line 830 "lev_comp.y" /* yacc.c:1646  */
    {
		      /* boolean, explicit foo != 0 */
		      add_opvars(splev, "i", 0);
		      (yyval.i) = SPO_JNE;
                  }
#line 3112 "y.tab.c" /* yacc.c:1646  */
    break;

  case 137:
#line 838 "lev_comp.y" /* yacc.c:1646  */
    {
		      is_inconstant_number = 0;
		  }
#line 3120 "y.tab.c" /* yacc.c:1646  */
    break;

  case 138:
#line 842 "lev_comp.y" /* yacc.c:1646  */
    {
		      struct opvar *chkjmp;
		      if (in_switch_statement > 0)
			  lc_error("Cannot nest switch-statements.");

		      in_switch_statement++;

		      n_switch_case_list = 0;
		      switch_default_case = NULL;

		      if (!is_inconstant_number)
			  add_opvars(splev, "o", SPO_RN2);
		      is_inconstant_number = 0;

		      chkjmp = New(struct opvar);
		      set_opvar_int(chkjmp, splev->n_opcodes+1);
		      switch_check_jump = chkjmp;
		      add_opcode(splev, SPO_PUSH, chkjmp);
		      add_opcode(splev, SPO_JMP, NULL);
		      break_stmt_start();
		  }
#line 3146 "y.tab.c" /* yacc.c:1646  */
    break;

  case 139:
#line 864 "lev_comp.y" /* yacc.c:1646  */
    {
		      struct opvar *endjump = New(struct opvar);
		      int i;

		      set_opvar_int(endjump, splev->n_opcodes+1);

		      add_opcode(splev, SPO_PUSH, endjump);
		      add_opcode(splev, SPO_JMP, NULL);

		      set_opvar_int(switch_check_jump, splev->n_opcodes - switch_check_jump->vardata.l);

		      for (i = 0; i < n_switch_case_list; i++) {
			  add_opvars(splev, "oio", SPO_COPY, switch_case_value[i], SPO_CMP);
			  set_opvar_int(switch_case_list[i], switch_case_list[i]->vardata.l - splev->n_opcodes-1);
			  add_opcode(splev, SPO_PUSH, switch_case_list[i]);
			  add_opcode(splev, SPO_JE, NULL);
		      }

		      if (switch_default_case) {
			  set_opvar_int(switch_default_case, switch_default_case->vardata.l - splev->n_opcodes-1);
			  add_opcode(splev, SPO_PUSH, switch_default_case);
			  add_opcode(splev, SPO_JMP, NULL);
		      }

		      set_opvar_int(endjump, splev->n_opcodes - endjump->vardata.l);

		      break_stmt_end(splev);

		      add_opcode(splev, SPO_POP, NULL); /* get rid of the value in stack */
		      in_switch_statement--;


		  }
#line 3184 "y.tab.c" /* yacc.c:1646  */
    break;

  case 142:
#line 904 "lev_comp.y" /* yacc.c:1646  */
    {
		      if (n_switch_case_list < MAX_SWITCH_CASES) {
			  struct opvar *tmppush = New(struct opvar);
			  set_opvar_int(tmppush, splev->n_opcodes);
			  switch_case_value[n_switch_case_list] = (yyvsp[-1].i);
			  switch_case_list[n_switch_case_list++] = tmppush;
		      } else lc_error("Too many cases in a switch.");
		  }
#line 3197 "y.tab.c" /* yacc.c:1646  */
    break;

  case 143:
#line 913 "lev_comp.y" /* yacc.c:1646  */
    {
		  }
#line 3204 "y.tab.c" /* yacc.c:1646  */
    break;

  case 144:
#line 916 "lev_comp.y" /* yacc.c:1646  */
    {
		      struct opvar *tmppush = New(struct opvar);

		      if (switch_default_case)
			  lc_error("Switch default case already used.");

		      set_opvar_int(tmppush, splev->n_opcodes);
		      switch_default_case = tmppush;
		  }
#line 3218 "y.tab.c" /* yacc.c:1646  */
    break;

  case 145:
#line 926 "lev_comp.y" /* yacc.c:1646  */
    {
		  }
#line 3225 "y.tab.c" /* yacc.c:1646  */
    break;

  case 146:
#line 931 "lev_comp.y" /* yacc.c:1646  */
    {
		      if (!allow_break_statements)
			  lc_error("Cannot use BREAK outside a statement block.");
		      else {
			  break_stmt_new(splev, splev->n_opcodes);
		      }
		  }
#line 3237 "y.tab.c" /* yacc.c:1646  */
    break;

  case 149:
#line 945 "lev_comp.y" /* yacc.c:1646  */
    {
		      char buf[256], buf2[256];

		      if (n_forloops >= MAX_NESTED_IFS) {
			  lc_error("FOR: Too deeply nested loops.");
			  n_forloops = MAX_NESTED_IFS - 1;
		      }

		      /* first, define a variable for the for-loop end value */
		      snprintf(buf, 255, "%s end", (yyvsp[-4].map));
		      /* the value of which is already in stack (the 2nd math_expr) */
		      add_opvars(splev, "iso", 0, buf, SPO_VAR_INIT);

		      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[-4].map), SPOVAR_INT);
		      /* define the for-loop variable. value is in stack (1st math_expr) */
		      add_opvars(splev, "iso", 0, (yyvsp[-4].map), SPO_VAR_INIT);

		      /* calculate value for the loop "step" variable */
		      snprintf(buf2, 255, "%s step", (yyvsp[-4].map));
		      add_opvars(splev, "vvo", buf, (yyvsp[-4].map), SPO_MATH_SUB); /* end - start */
		      add_opvars(splev, "o", SPO_MATH_SIGN); /* sign of that */
		      add_opvars(splev, "iso", 0, buf2, SPO_VAR_INIT); /* save the sign into the step var */

		      forloop_list[n_forloops].varname = strdup((yyvsp[-4].map));
		      forloop_list[n_forloops].jmp_point = splev->n_opcodes;

		      n_forloops++;
		      Free((yyvsp[-4].map));
		  }
#line 3271 "y.tab.c" /* yacc.c:1646  */
    break;

  case 150:
#line 977 "lev_comp.y" /* yacc.c:1646  */
    {
		      /* nothing */
		      break_stmt_start();
		  }
#line 3280 "y.tab.c" /* yacc.c:1646  */
    break;

  case 151:
#line 982 "lev_comp.y" /* yacc.c:1646  */
    {
		      char buf[256], buf2[256];
		      n_forloops--;
		      snprintf(buf, 255, "%s step", forloop_list[n_forloops].varname);
		      snprintf(buf2, 255, "%s end", forloop_list[n_forloops].varname);
		      /* compare for-loop var to end value */
		      add_opvars(splev, "vvo", forloop_list[n_forloops].varname, buf2, SPO_CMP);
		      /* var + step */
		      add_opvars(splev, "vvo", buf,
				 forloop_list[n_forloops].varname, SPO_MATH_ADD);
		      /* for-loop var = (for-loop var + step) */
		      add_opvars(splev, "iso", 0, forloop_list[n_forloops].varname, SPO_VAR_INIT);
		      /* jump back if compared values were not equal */
		      add_opvars(splev, "io", forloop_list[n_forloops].jmp_point - splev->n_opcodes - 1, SPO_JNE);
		      Free(forloop_list[n_forloops].varname);
		      break_stmt_end(splev);
		  }
#line 3302 "y.tab.c" /* yacc.c:1646  */
    break;

  case 152:
#line 1002 "lev_comp.y" /* yacc.c:1646  */
    {
		      struct opvar *tmppush = New(struct opvar);

		      if (n_if_list >= MAX_NESTED_IFS) {
			  lc_error("LOOP: Too deeply nested conditionals.");
			  n_if_list = MAX_NESTED_IFS - 1;
		      }
		      set_opvar_int(tmppush, splev->n_opcodes);
		      if_list[n_if_list++] = tmppush;

		      add_opvars(splev, "o", SPO_DEC);
		      break_stmt_start();
		  }
#line 3320 "y.tab.c" /* yacc.c:1646  */
    break;

  case 153:
#line 1016 "lev_comp.y" /* yacc.c:1646  */
    {
		      struct opvar *tmppush;

		      add_opvars(splev, "oio", SPO_COPY, 0, SPO_CMP);

		      tmppush = (struct opvar *) if_list[--n_if_list];
		      set_opvar_int(tmppush, tmppush->vardata.l - splev->n_opcodes-1);
		      add_opcode(splev, SPO_PUSH, tmppush);
		      add_opcode(splev, SPO_JG, NULL);
		      add_opcode(splev, SPO_POP, NULL); /* get rid of the count value in stack */
		      break_stmt_end(splev);
		  }
#line 3337 "y.tab.c" /* yacc.c:1646  */
    break;

  case 154:
#line 1031 "lev_comp.y" /* yacc.c:1646  */
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

		      add_opcode(splev, reverse_jmp_opcode( (yyvsp[-1].i) ), NULL);

		  }
#line 3361 "y.tab.c" /* yacc.c:1646  */
    break;

  case 155:
#line 1051 "lev_comp.y" /* yacc.c:1646  */
    {
		      if (n_if_list > 0) {
			  struct opvar *tmppush;
			  tmppush = (struct opvar *) if_list[--n_if_list];
			  set_opvar_int(tmppush, splev->n_opcodes - tmppush->vardata.l);
		      } else lc_error("IF: Huh?!  No start address?");
		  }
#line 3373 "y.tab.c" /* yacc.c:1646  */
    break;

  case 156:
#line 1061 "lev_comp.y" /* yacc.c:1646  */
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

		      add_opcode(splev, reverse_jmp_opcode( (yyvsp[0].i) ), NULL);

		  }
#line 3397 "y.tab.c" /* yacc.c:1646  */
    break;

  case 157:
#line 1081 "lev_comp.y" /* yacc.c:1646  */
    {
		     /* do nothing */
		  }
#line 3405 "y.tab.c" /* yacc.c:1646  */
    break;

  case 158:
#line 1087 "lev_comp.y" /* yacc.c:1646  */
    {
		      if (n_if_list > 0) {
			  struct opvar *tmppush;
			  tmppush = (struct opvar *) if_list[--n_if_list];
			  set_opvar_int(tmppush, splev->n_opcodes - tmppush->vardata.l);
		      } else lc_error("IF: Huh?!  No start address?");
		  }
#line 3417 "y.tab.c" /* yacc.c:1646  */
    break;

  case 159:
#line 1095 "lev_comp.y" /* yacc.c:1646  */
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
#line 3438 "y.tab.c" /* yacc.c:1646  */
    break;

  case 160:
#line 1112 "lev_comp.y" /* yacc.c:1646  */
    {
		      if (n_if_list > 0) {
			  struct opvar *tmppush;
			  tmppush = (struct opvar *) if_list[--n_if_list];
			  set_opvar_int(tmppush, splev->n_opcodes - tmppush->vardata.l);
		      } else lc_error("IF: Huh?! No end address?");
		  }
#line 3450 "y.tab.c" /* yacc.c:1646  */
    break;

  case 161:
#line 1122 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "o", SPO_MESSAGE);
		  }
#line 3458 "y.tab.c" /* yacc.c:1646  */
    break;

  case 162:
#line 1128 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "iiiiiio", -1,  0, -1, -1, -1, -1, SPO_CORRIDOR);
		  }
#line 3466 "y.tab.c" /* yacc.c:1646  */
    break;

  case 163:
#line 1132 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "iiiiiio", -1, (yyvsp[0].i), -1, -1, -1, -1, SPO_CORRIDOR);
		  }
#line 3474 "y.tab.c" /* yacc.c:1646  */
    break;

  case 164:
#line 1136 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "iiiiiio", -1, -1, -1, -1, -1, -1, SPO_CORRIDOR);
		  }
#line 3482 "y.tab.c" /* yacc.c:1646  */
    break;

  case 165:
#line 1142 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "iiiiiio",
				 (yyvsp[-2].corpos).room, (yyvsp[-2].corpos).door, (yyvsp[-2].corpos).wall,
				 (yyvsp[0].corpos).room, (yyvsp[0].corpos).door, (yyvsp[0].corpos).wall,
				 SPO_CORRIDOR);
		  }
#line 3493 "y.tab.c" /* yacc.c:1646  */
    break;

  case 166:
#line 1149 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "iiiiiio",
				 (yyvsp[-2].corpos).room, (yyvsp[-2].corpos).door, (yyvsp[-2].corpos).wall,
				 -1, -1, (long)(yyvsp[0].i),
				 SPO_CORRIDOR);
		  }
#line 3504 "y.tab.c" /* yacc.c:1646  */
    break;

  case 167:
#line 1158 "lev_comp.y" /* yacc.c:1646  */
    {
			(yyval.corpos).room = (yyvsp[-5].i);
			(yyval.corpos).wall = (yyvsp[-3].i);
			(yyval.corpos).door = (yyvsp[-1].i);
		  }
#line 3514 "y.tab.c" /* yacc.c:1646  */
    break;

  case 168:
#line 1166 "lev_comp.y" /* yacc.c:1646  */
    {
		      if (((yyvsp[-2].i) < 100) && ((yyvsp[-3].i) == OROOM))
			  lc_error("Only typed rooms can have a chance.");
		      else {
			  add_opvars(splev, "iii", (long)(yyvsp[-3].i), (long)(yyvsp[-2].i), (long)(yyvsp[0].i));
		      }
                  }
#line 3526 "y.tab.c" /* yacc.c:1646  */
    break;

  case 169:
#line 1176 "lev_comp.y" /* yacc.c:1646  */
    {
		      long flags = (yyvsp[0].i);
		      if (flags == -1) flags = (1 << 0);
		      add_opvars(splev, "iiiiiiio", flags, ERR, ERR,
				 (yyvsp[-3].crd).x, (yyvsp[-3].crd).y, (yyvsp[-1].sze).width, (yyvsp[-1].sze).height, SPO_SUBROOM);
		      break_stmt_start();
		  }
#line 3538 "y.tab.c" /* yacc.c:1646  */
    break;

  case 170:
#line 1184 "lev_comp.y" /* yacc.c:1646  */
    {
		      break_stmt_end(splev);
		      add_opcode(splev, SPO_ENDROOM, NULL);
		  }
#line 3547 "y.tab.c" /* yacc.c:1646  */
    break;

  case 171:
#line 1191 "lev_comp.y" /* yacc.c:1646  */
    {
		      long flags = (yyvsp[-2].i);
		      if (flags == -1) flags = (1 << 0);
		      add_opvars(splev, "iiiiiiio", flags,
				 (yyvsp[-3].crd).x, (yyvsp[-3].crd).y, (yyvsp[-5].crd).x, (yyvsp[-5].crd).y,
				 (yyvsp[-1].sze).width, (yyvsp[-1].sze).height, SPO_ROOM);
		      break_stmt_start();
		  }
#line 3560 "y.tab.c" /* yacc.c:1646  */
    break;

  case 172:
#line 1200 "lev_comp.y" /* yacc.c:1646  */
    {
		      break_stmt_end(splev);
		      add_opcode(splev, SPO_ENDROOM, NULL);
		  }
#line 3569 "y.tab.c" /* yacc.c:1646  */
    break;

  case 173:
#line 1207 "lev_comp.y" /* yacc.c:1646  */
    {
			(yyval.i) = 1;
		  }
#line 3577 "y.tab.c" /* yacc.c:1646  */
    break;

  case 174:
#line 1211 "lev_comp.y" /* yacc.c:1646  */
    {
			(yyval.i) = (yyvsp[0].i);
		  }
#line 3585 "y.tab.c" /* yacc.c:1646  */
    break;

  case 175:
#line 1217 "lev_comp.y" /* yacc.c:1646  */
    {
			if ( (yyvsp[-3].i) < 1 || (yyvsp[-3].i) > 5 ||
			    (yyvsp[-1].i) < 1 || (yyvsp[-1].i) > 5 ) {
			    lc_error("Room positions should be between 1-5: (%li,%li)!", (yyvsp[-3].i), (yyvsp[-1].i));
			} else {
			    (yyval.crd).x = (yyvsp[-3].i);
			    (yyval.crd).y = (yyvsp[-1].i);
			}
		  }
#line 3599 "y.tab.c" /* yacc.c:1646  */
    break;

  case 176:
#line 1227 "lev_comp.y" /* yacc.c:1646  */
    {
			(yyval.crd).x = (yyval.crd).y = ERR;
		  }
#line 3607 "y.tab.c" /* yacc.c:1646  */
    break;

  case 177:
#line 1233 "lev_comp.y" /* yacc.c:1646  */
    {
			if ( (yyvsp[-3].i) < 0 || (yyvsp[-1].i) < 0) {
			    lc_error("Invalid subroom position (%li,%li)!", (yyvsp[-3].i), (yyvsp[-1].i));
			} else {
			    (yyval.crd).x = (yyvsp[-3].i);
			    (yyval.crd).y = (yyvsp[-1].i);
			}
		  }
#line 3620 "y.tab.c" /* yacc.c:1646  */
    break;

  case 178:
#line 1242 "lev_comp.y" /* yacc.c:1646  */
    {
			(yyval.crd).x = (yyval.crd).y = ERR;
		  }
#line 3628 "y.tab.c" /* yacc.c:1646  */
    break;

  case 179:
#line 1248 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.crd).x = (yyvsp[-3].i);
		      (yyval.crd).y = (yyvsp[-1].i);
		  }
#line 3637 "y.tab.c" /* yacc.c:1646  */
    break;

  case 180:
#line 1253 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.crd).x = (yyval.crd).y = ERR;
		  }
#line 3645 "y.tab.c" /* yacc.c:1646  */
    break;

  case 181:
#line 1259 "lev_comp.y" /* yacc.c:1646  */
    {
			(yyval.sze).width = (yyvsp[-3].i);
			(yyval.sze).height = (yyvsp[-1].i);
		  }
#line 3654 "y.tab.c" /* yacc.c:1646  */
    break;

  case 182:
#line 1264 "lev_comp.y" /* yacc.c:1646  */
    {
			(yyval.sze).height = (yyval.sze).width = ERR;
		  }
#line 3662 "y.tab.c" /* yacc.c:1646  */
    break;

  case 183:
#line 1270 "lev_comp.y" /* yacc.c:1646  */
    {
			/* ERR means random here */
			if ((yyvsp[-2].i) == ERR && (yyvsp[0].i) != ERR) {
			    lc_error("If the door wall is random, so must be its pos!");
			} else {
			    add_opvars(splev, "iiiio", (long)(yyvsp[0].i), (long)(yyvsp[-4].i), (long)(yyvsp[-6].i), (long)(yyvsp[-2].i), SPO_ROOM_DOOR);
			}
		  }
#line 3675 "y.tab.c" /* yacc.c:1646  */
    break;

  case 184:
#line 1279 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "io", (long)(yyvsp[-2].i), SPO_DOOR);
		  }
#line 3683 "y.tab.c" /* yacc.c:1646  */
    break;

  case 189:
#line 1293 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = (yyvsp[0].i);
		  }
#line 3691 "y.tab.c" /* yacc.c:1646  */
    break;

  case 190:
#line 1297 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = ((yyvsp[-2].i) | (yyvsp[0].i));
		  }
#line 3699 "y.tab.c" /* yacc.c:1646  */
    break;

  case 193:
#line 1307 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "ciisiio", 0, 0, 1, (char *)0, 0, 0, SPO_MAP);
		      max_x_map = COLNO-1;
		      max_y_map = ROWNO;
		  }
#line 3709 "y.tab.c" /* yacc.c:1646  */
    break;

  case 194:
#line 1313 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "cii", SP_COORD_PACK(((yyvsp[-4].i)),((yyvsp[-2].i))), 1, (long)(yyvsp[-1].i));
		      scan_map((yyvsp[0].map), splev);
		      Free((yyvsp[0].map));
		  }
#line 3719 "y.tab.c" /* yacc.c:1646  */
    break;

  case 195:
#line 1319 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "ii", 2, (long)(yyvsp[-1].i));
		      scan_map((yyvsp[0].map), splev);
		      Free((yyvsp[0].map));
		  }
#line 3729 "y.tab.c" /* yacc.c:1646  */
    break;

  case 200:
#line 1335 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "io", 0, SPO_MONSTER);
		  }
#line 3737 "y.tab.c" /* yacc.c:1646  */
    break;

  case 201:
#line 1339 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "io", 1, SPO_MONSTER);
		      in_container_obj++;
		      break_stmt_start();
		  }
#line 3747 "y.tab.c" /* yacc.c:1646  */
    break;

  case 202:
#line 1345 "lev_comp.y" /* yacc.c:1646  */
    {
		     break_stmt_end(splev);
		     in_container_obj--;
		     add_opvars(splev, "o", SPO_END_MONINVENT);
		 }
#line 3757 "y.tab.c" /* yacc.c:1646  */
    break;

  case 203:
#line 1353 "lev_comp.y" /* yacc.c:1646  */
    {
		      /* nothing */
		  }
#line 3765 "y.tab.c" /* yacc.c:1646  */
    break;

  case 204:
#line 1359 "lev_comp.y" /* yacc.c:1646  */
    {
		      struct opvar *stopit = New(struct opvar);
		      set_opvar_int(stopit, SP_M_V_END);
		      add_opcode(splev, SPO_PUSH, stopit);
		      (yyval.i) = 0x0000;
		  }
#line 3776 "y.tab.c" /* yacc.c:1646  */
    break;

  case 205:
#line 1366 "lev_comp.y" /* yacc.c:1646  */
    {
		      if (( (yyvsp[-2].i) & (yyvsp[0].i) ))
			  lc_error("MONSTER extra info defined twice.");
		      (yyval.i) = ( (yyvsp[-2].i) | (yyvsp[0].i) );
		  }
#line 3786 "y.tab.c" /* yacc.c:1646  */
    break;

  case 206:
#line 1374 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "i", SP_M_V_NAME);
		      (yyval.i) = 0x0001;
		  }
#line 3795 "y.tab.c" /* yacc.c:1646  */
    break;

  case 207:
#line 1379 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "ii", (long)(yyvsp[0].i), SP_M_V_PEACEFUL);
		      (yyval.i) = 0x0002;
		  }
#line 3804 "y.tab.c" /* yacc.c:1646  */
    break;

  case 208:
#line 1384 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "ii", (long)(yyvsp[0].i), SP_M_V_ASLEEP);
		      (yyval.i) = 0x0004;
		  }
#line 3813 "y.tab.c" /* yacc.c:1646  */
    break;

  case 209:
#line 1389 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "ii", (long)(yyvsp[0].i), SP_M_V_ALIGN);
		      (yyval.i) = 0x0008;
		  }
#line 3822 "y.tab.c" /* yacc.c:1646  */
    break;

  case 210:
#line 1394 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "ii", (long)(yyvsp[-1].i), SP_M_V_APPEAR);
		      (yyval.i) = 0x0010;
		  }
#line 3831 "y.tab.c" /* yacc.c:1646  */
    break;

  case 211:
#line 1399 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "ii", 1, SP_M_V_FEMALE);
		      (yyval.i) = 0x0020;
		  }
#line 3840 "y.tab.c" /* yacc.c:1646  */
    break;

  case 212:
#line 1404 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "ii", 1, SP_M_V_INVIS);
		      (yyval.i) = 0x0040;
		  }
#line 3849 "y.tab.c" /* yacc.c:1646  */
    break;

  case 213:
#line 1409 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "ii", 1, SP_M_V_CANCELLED);
		      (yyval.i) = 0x0080;
		  }
#line 3858 "y.tab.c" /* yacc.c:1646  */
    break;

  case 214:
#line 1414 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "ii", 1, SP_M_V_REVIVED);
		      (yyval.i) = 0x0100;
		  }
#line 3867 "y.tab.c" /* yacc.c:1646  */
    break;

  case 215:
#line 1419 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "ii", 1, SP_M_V_AVENGE);
		      (yyval.i) = 0x0200;
		  }
#line 3876 "y.tab.c" /* yacc.c:1646  */
    break;

  case 216:
#line 1424 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "i", SP_M_V_FLEEING);
		      (yyval.i) = 0x0400;
		  }
#line 3885 "y.tab.c" /* yacc.c:1646  */
    break;

  case 217:
#line 1429 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "i", SP_M_V_BLINDED);
		      (yyval.i) = 0x0800;
		  }
#line 3894 "y.tab.c" /* yacc.c:1646  */
    break;

  case 218:
#line 1434 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "i", SP_M_V_PARALYZED);
		      (yyval.i) = 0x1000;
		  }
#line 3903 "y.tab.c" /* yacc.c:1646  */
    break;

  case 219:
#line 1439 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "ii", 1, SP_M_V_STUNNED);
		      (yyval.i) = 0x2000;
		  }
#line 3912 "y.tab.c" /* yacc.c:1646  */
    break;

  case 220:
#line 1444 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "ii", 1, SP_M_V_CONFUSED);
		      (yyval.i) = 0x4000;
		  }
#line 3921 "y.tab.c" /* yacc.c:1646  */
    break;

  case 221:
#line 1449 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "ii", (long)(yyvsp[0].i), SP_M_V_SEENTRAPS);
		      (yyval.i) = 0x8000;
		  }
#line 3930 "y.tab.c" /* yacc.c:1646  */
    break;

  case 222:
#line 1456 "lev_comp.y" /* yacc.c:1646  */
    {
		      int token = get_trap_type((yyvsp[0].map));
		      if (token == ERR || token == 0)
			  lc_error("Unknown trap type '%s'!", (yyvsp[0].map));
		      (yyval.i) = (1L << (token - 1));
		  }
#line 3941 "y.tab.c" /* yacc.c:1646  */
    break;

  case 223:
#line 1463 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = (long) ~0;
		  }
#line 3949 "y.tab.c" /* yacc.c:1646  */
    break;

  case 224:
#line 1467 "lev_comp.y" /* yacc.c:1646  */
    {
		      int token = get_trap_type((yyvsp[-2].map));
		      if (token == ERR || token == 0)
			  lc_error("Unknown trap type '%s'!", (yyvsp[-2].map));

		      if ((1L << (token - 1)) & (yyvsp[0].i))
			  lc_error("Monster seen_traps, trap '%s' listed twice.", (yyvsp[-2].map));

		      (yyval.i) = ((1L << (token - 1)) | (yyvsp[0].i));
		  }
#line 3964 "y.tab.c" /* yacc.c:1646  */
    break;

  case 225:
#line 1480 "lev_comp.y" /* yacc.c:1646  */
    {
		      long cnt = 0;
		      if (in_container_obj) cnt |= SP_OBJ_CONTENT;
		      add_opvars(splev, "io", cnt, SPO_OBJECT);
		  }
#line 3974 "y.tab.c" /* yacc.c:1646  */
    break;

  case 226:
#line 1486 "lev_comp.y" /* yacc.c:1646  */
    {
		      long cnt = SP_OBJ_CONTAINER;
		      if (in_container_obj) cnt |= SP_OBJ_CONTENT;
		      add_opvars(splev, "io", cnt, SPO_OBJECT);
		      in_container_obj++;
		      break_stmt_start();
		  }
#line 3986 "y.tab.c" /* yacc.c:1646  */
    break;

  case 227:
#line 1494 "lev_comp.y" /* yacc.c:1646  */
    {
		     break_stmt_end(splev);
		     in_container_obj--;
		     add_opcode(splev, SPO_POP_CONTAINER, NULL);
		 }
#line 3996 "y.tab.c" /* yacc.c:1646  */
    break;

  case 228:
#line 1502 "lev_comp.y" /* yacc.c:1646  */
    {
		      if (( (yyvsp[0].i) & 0x4000) && in_container_obj) lc_error("Object cannot have a coord when contained.");
		      else if (!( (yyvsp[0].i) & 0x4000) && !in_container_obj) lc_error("Object needs a coord when not contained.");
		  }
#line 4005 "y.tab.c" /* yacc.c:1646  */
    break;

  case 229:
#line 1509 "lev_comp.y" /* yacc.c:1646  */
    {
		      struct opvar *stopit = New(struct opvar);
		      set_opvar_int(stopit, SP_O_V_END);
		      add_opcode(splev, SPO_PUSH, stopit);
		      (yyval.i) = 0x00;
		  }
#line 4016 "y.tab.c" /* yacc.c:1646  */
    break;

  case 230:
#line 1516 "lev_comp.y" /* yacc.c:1646  */
    {
		      if (( (yyvsp[-2].i) & (yyvsp[0].i) ))
			  lc_error("OBJECT extra info '%s' defined twice.", curr_token);
		      (yyval.i) = ( (yyvsp[-2].i) | (yyvsp[0].i) );
		  }
#line 4026 "y.tab.c" /* yacc.c:1646  */
    break;

  case 231:
#line 1524 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "ii", (long)(yyvsp[0].i), SP_O_V_CURSE);
		      (yyval.i) = 0x0001;
		  }
#line 4035 "y.tab.c" /* yacc.c:1646  */
    break;

  case 232:
#line 1529 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "i", SP_O_V_CORPSENM);
		      (yyval.i) = 0x0002;
		  }
#line 4044 "y.tab.c" /* yacc.c:1646  */
    break;

  case 233:
#line 1534 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "i", SP_O_V_SPE);
		      (yyval.i) = 0x0004;
		  }
#line 4053 "y.tab.c" /* yacc.c:1646  */
    break;

  case 234:
#line 1539 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "i", SP_O_V_NAME);
		      (yyval.i) = 0x0008;
		  }
#line 4062 "y.tab.c" /* yacc.c:1646  */
    break;

  case 235:
#line 1544 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "i", SP_O_V_QUAN);
		      (yyval.i) = 0x0010;
		  }
#line 4071 "y.tab.c" /* yacc.c:1646  */
    break;

  case 236:
#line 1549 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "ii", 1, SP_O_V_BURIED);
		      (yyval.i) = 0x0020;
		  }
#line 4080 "y.tab.c" /* yacc.c:1646  */
    break;

  case 237:
#line 1554 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "ii", (long)(yyvsp[0].i), SP_O_V_LIT);
		      (yyval.i) = 0x0040;
		  }
#line 4089 "y.tab.c" /* yacc.c:1646  */
    break;

  case 238:
#line 1559 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "i", SP_O_V_ERODED);
		      (yyval.i) = 0x0080;
		  }
#line 4098 "y.tab.c" /* yacc.c:1646  */
    break;

  case 239:
#line 1564 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "ii", -1, SP_O_V_ERODED);
		      (yyval.i) = 0x0080;
		  }
#line 4107 "y.tab.c" /* yacc.c:1646  */
    break;

  case 240:
#line 1569 "lev_comp.y" /* yacc.c:1646  */
    {
		      if ((yyvsp[0].i) == D_LOCKED) {
			  add_opvars(splev, "ii", 1, SP_O_V_LOCKED);
			  (yyval.i) = 0x0100;
		      } else if ((yyvsp[0].i) == D_BROKEN) {
			  add_opvars(splev, "ii", 1, SP_O_V_BROKEN);
			  (yyval.i) = 0x0200;
		      } else
			  lc_error("OBJECT state can only be locked or broken.");
		  }
#line 4122 "y.tab.c" /* yacc.c:1646  */
    break;

  case 241:
#line 1580 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "ii", 1, SP_O_V_TRAPPED);
		      (yyval.i) = 0x0400;
		  }
#line 4131 "y.tab.c" /* yacc.c:1646  */
    break;

  case 242:
#line 1585 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "i", SP_O_V_RECHARGED);
		      (yyval.i) = 0x0800;
		  }
#line 4140 "y.tab.c" /* yacc.c:1646  */
    break;

  case 243:
#line 1590 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "ii", 1, SP_O_V_INVIS);
		      (yyval.i) = 0x1000;
		  }
#line 4149 "y.tab.c" /* yacc.c:1646  */
    break;

  case 244:
#line 1595 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "ii", 1, SP_O_V_GREASED);
		      (yyval.i) = 0x2000;
		  }
#line 4158 "y.tab.c" /* yacc.c:1646  */
    break;

  case 245:
#line 1600 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "i", SP_O_V_COORD);
		      (yyval.i) = 0x4000;
		  }
#line 4167 "y.tab.c" /* yacc.c:1646  */
    break;

  case 246:
#line 1607 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "io", (long)(yyvsp[-2].i), SPO_TRAP);
		  }
#line 4175 "y.tab.c" /* yacc.c:1646  */
    break;

  case 247:
#line 1613 "lev_comp.y" /* yacc.c:1646  */
    {
		       long d, state = 0;
		       /* convert dir from a DIRECTION to a DB_DIR */
		       d = (yyvsp[-2].i);
		       switch(d) {
		       case W_NORTH: d = DB_NORTH; break;
		       case W_SOUTH: d = DB_SOUTH; break;
		       case W_EAST:  d = DB_EAST;  break;
		       case W_WEST:  d = DB_WEST;  break;
		       default:
			   lc_error("Invalid drawbridge direction.");
			   break;
		       }

		       if ( (yyvsp[0].i) == D_ISOPEN )
			   state = 1;
		       else if ( (yyvsp[0].i) == D_CLOSED )
			   state = 0;
		       else if ( (yyvsp[0].i) == -1 )
			   state = -1;
		       else
			   lc_error("A drawbridge can only be open, closed or random!");
		       add_opvars(splev, "iio", state, d, SPO_DRAWBRIDGE);
		   }
#line 4204 "y.tab.c" /* yacc.c:1646  */
    break;

  case 248:
#line 1640 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "iiio",
				 (long)(yyvsp[0].i), 1, 0, SPO_MAZEWALK);
		  }
#line 4213 "y.tab.c" /* yacc.c:1646  */
    break;

  case 249:
#line 1645 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "iiio",
				 (long)(yyvsp[-3].i), (long)(yyvsp[-1].i), (long)(yyvsp[0].i), SPO_MAZEWALK);
		  }
#line 4222 "y.tab.c" /* yacc.c:1646  */
    break;

  case 250:
#line 1652 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "rio", SP_REGION_PACK(-1,-1,-1,-1), 0, SPO_WALLIFY);
		  }
#line 4230 "y.tab.c" /* yacc.c:1646  */
    break;

  case 251:
#line 1656 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "io", 1, SPO_WALLIFY);
		  }
#line 4238 "y.tab.c" /* yacc.c:1646  */
    break;

  case 252:
#line 1662 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "io", (long)(yyvsp[0].i), SPO_LADDER);
		  }
#line 4246 "y.tab.c" /* yacc.c:1646  */
    break;

  case 253:
#line 1668 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "io", (long)(yyvsp[0].i), SPO_STAIR);
		  }
#line 4254 "y.tab.c" /* yacc.c:1646  */
    break;

  case 254:
#line 1674 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "iiiii iiiii iiso",
				 (yyvsp[-4].lregn).x1, (yyvsp[-4].lregn).y1, (yyvsp[-4].lregn).x2, (yyvsp[-4].lregn).y2, (yyvsp[-4].lregn).area,
				 (yyvsp[-2].lregn).x1, (yyvsp[-2].lregn).y1, (yyvsp[-2].lregn).x2, (yyvsp[-2].lregn).y2, (yyvsp[-2].lregn).area,
				 (long)(((yyvsp[0].i)) ? LR_UPSTAIR : LR_DOWNSTAIR),
				 0, (char *)0, SPO_LEVREGION);
		  }
#line 4266 "y.tab.c" /* yacc.c:1646  */
    break;

  case 255:
#line 1684 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "iiiii iiiii iiso",
				 (yyvsp[-4].lregn).x1, (yyvsp[-4].lregn).y1, (yyvsp[-4].lregn).x2, (yyvsp[-4].lregn).y2, (yyvsp[-4].lregn).area,
				 (yyvsp[-2].lregn).x1, (yyvsp[-2].lregn).y1, (yyvsp[-2].lregn).x2, (yyvsp[-2].lregn).y2, (yyvsp[-2].lregn).area,
				 LR_PORTAL, 0, (yyvsp[0].map), SPO_LEVREGION);
		      Free((yyvsp[0].map));
		  }
#line 4278 "y.tab.c" /* yacc.c:1646  */
    break;

  case 256:
#line 1694 "lev_comp.y" /* yacc.c:1646  */
    {
		      long rtype = 0;
		      switch((yyvsp[0].i)) {
		      case -1: rtype = LR_TELE; break;
		      case  0: rtype = LR_DOWNTELE; break;
		      case  1: rtype = LR_UPTELE; break;
		      }
		      add_opvars(splev, "iiiii iiiii iiso",
				 (yyvsp[-3].lregn).x1, (yyvsp[-3].lregn).y1, (yyvsp[-3].lregn).x2, (yyvsp[-3].lregn).y2, (yyvsp[-3].lregn).area,
				 (yyvsp[-1].lregn).x1, (yyvsp[-1].lregn).y1, (yyvsp[-1].lregn).x2, (yyvsp[-1].lregn).y2, (yyvsp[-1].lregn).area,
				 rtype, 0, (char *)0, SPO_LEVREGION);
		  }
#line 4295 "y.tab.c" /* yacc.c:1646  */
    break;

  case 257:
#line 1709 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "iiiii iiiii iiso",
				 (yyvsp[-2].lregn).x1, (yyvsp[-2].lregn).y1, (yyvsp[-2].lregn).x2, (yyvsp[-2].lregn).y2, (yyvsp[-2].lregn).area,
				 (yyvsp[0].lregn).x1, (yyvsp[0].lregn).y1, (yyvsp[0].lregn).x2, (yyvsp[0].lregn).y2, (yyvsp[0].lregn).area,
				 (long)LR_BRANCH, 0, (char *)0, SPO_LEVREGION);
		  }
#line 4306 "y.tab.c" /* yacc.c:1646  */
    break;

  case 258:
#line 1718 "lev_comp.y" /* yacc.c:1646  */
    {
			(yyval.i) = -1;
		  }
#line 4314 "y.tab.c" /* yacc.c:1646  */
    break;

  case 259:
#line 1722 "lev_comp.y" /* yacc.c:1646  */
    {
			(yyval.i) = (yyvsp[0].i);
		  }
#line 4322 "y.tab.c" /* yacc.c:1646  */
    break;

  case 260:
#line 1728 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "o", SPO_FOUNTAIN);
		  }
#line 4330 "y.tab.c" /* yacc.c:1646  */
    break;

  case 261:
#line 1734 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "o", SPO_SINK);
		  }
#line 4338 "y.tab.c" /* yacc.c:1646  */
    break;

  case 262:
#line 1740 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "o", SPO_POOL);
		  }
#line 4346 "y.tab.c" /* yacc.c:1646  */
    break;

  case 263:
#line 1746 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.terr).lit = -2;
		      (yyval.terr).ter = what_map_char((char) (yyvsp[0].i));
		  }
#line 4355 "y.tab.c" /* yacc.c:1646  */
    break;

  case 264:
#line 1751 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.terr).lit = (yyvsp[-1].i);
		      (yyval.terr).ter = what_map_char((char) (yyvsp[-3].i));
		  }
#line 4364 "y.tab.c" /* yacc.c:1646  */
    break;

  case 265:
#line 1758 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "io", (yyvsp[0].i), SPO_REPLACETERRAIN);
		  }
#line 4372 "y.tab.c" /* yacc.c:1646  */
    break;

  case 266:
#line 1764 "lev_comp.y" /* yacc.c:1646  */
    {
		     add_opvars(splev, "o", SPO_TERRAIN);
		 }
#line 4380 "y.tab.c" /* yacc.c:1646  */
    break;

  case 267:
#line 1770 "lev_comp.y" /* yacc.c:1646  */
    {
		     add_opvars(splev, "o", SPO_NON_DIGGABLE);
		  }
#line 4388 "y.tab.c" /* yacc.c:1646  */
    break;

  case 268:
#line 1776 "lev_comp.y" /* yacc.c:1646  */
    {
		     add_opvars(splev, "o", SPO_NON_PASSWALL);
		  }
#line 4396 "y.tab.c" /* yacc.c:1646  */
    break;

  case 269:
#line 1782 "lev_comp.y" /* yacc.c:1646  */
    {
		      long irr;
		      long rt = (yyvsp[-1].i);
		      long flags = (yyvsp[0].i);
		      if (flags == -1) flags = (1 << 0);
		      if (!(( flags ) & 1)) rt += MAXRTYPE+1;
		      irr = ((( flags ) & 2) != 0);
		      add_opvars(splev, "iiio",
				 (long)(yyvsp[-3].i), rt, flags, SPO_REGION);
		      (yyval.i) = (irr || (flags & 1) || rt != OROOM);
		      break_stmt_start();
		  }
#line 4413 "y.tab.c" /* yacc.c:1646  */
    break;

  case 270:
#line 1795 "lev_comp.y" /* yacc.c:1646  */
    {
		      break_stmt_end(splev);
		      if ( (yyvsp[-1].i) ) {
			  add_opcode(splev, SPO_ENDROOM, NULL);
		      } else if ( (yyvsp[0].i) )
			  lc_error("Cannot use lev statements in non-permanent REGION");
		  }
#line 4425 "y.tab.c" /* yacc.c:1646  */
    break;

  case 271:
#line 1805 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = 0;
		  }
#line 4433 "y.tab.c" /* yacc.c:1646  */
    break;

  case 272:
#line 1809 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = (yyvsp[0].i);
		  }
#line 4441 "y.tab.c" /* yacc.c:1646  */
    break;

  case 273:
#line 1815 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "iio", (long)(yyvsp[0].i), (long)(yyvsp[-2].i), SPO_ALTAR);
		  }
#line 4449 "y.tab.c" /* yacc.c:1646  */
    break;

  case 274:
#line 1821 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "io", 2, SPO_GRAVE);
		  }
#line 4457 "y.tab.c" /* yacc.c:1646  */
    break;

  case 275:
#line 1825 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "sio",
				 (char *)0, 1, SPO_GRAVE);
		  }
#line 4466 "y.tab.c" /* yacc.c:1646  */
    break;

  case 276:
#line 1830 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "sio",
				 (char *)0, 0, SPO_GRAVE);
		  }
#line 4475 "y.tab.c" /* yacc.c:1646  */
    break;

  case 277:
#line 1837 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "o", SPO_GOLD);
		  }
#line 4483 "y.tab.c" /* yacc.c:1646  */
    break;

  case 278:
#line 1843 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "io",
				 (long)(yyvsp[-2].i), SPO_ENGRAVING);
		  }
#line 4492 "y.tab.c" /* yacc.c:1646  */
    break;

  case 279:
#line 1850 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "o", SPO_MINERALIZE);
		  }
#line 4500 "y.tab.c" /* yacc.c:1646  */
    break;

  case 280:
#line 1854 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "iiiio", -1L, -1L, -1L, -1L, SPO_MINERALIZE);
		  }
#line 4508 "y.tab.c" /* yacc.c:1646  */
    break;

  case 281:
#line 1860 "lev_comp.y" /* yacc.c:1646  */
    {
			int token = get_trap_type((yyvsp[0].map));
			if (token == ERR)
			    lc_error("Unknown trap type '%s'!", (yyvsp[0].map));
			(yyval.i) = token;
			Free((yyvsp[0].map));
		  }
#line 4520 "y.tab.c" /* yacc.c:1646  */
    break;

  case 283:
#line 1871 "lev_comp.y" /* yacc.c:1646  */
    {
			int token = get_room_type((yyvsp[0].map));
			if (token == ERR) {
			    lc_warning("Unknown room type \"%s\"!  Making ordinary room...", (yyvsp[0].map));
				(yyval.i) = OROOM;
			} else
				(yyval.i) = token;
			Free((yyvsp[0].map));
		  }
#line 4534 "y.tab.c" /* yacc.c:1646  */
    break;

  case 285:
#line 1884 "lev_comp.y" /* yacc.c:1646  */
    {
			(yyval.i) = -1;
		  }
#line 4542 "y.tab.c" /* yacc.c:1646  */
    break;

  case 286:
#line 1888 "lev_comp.y" /* yacc.c:1646  */
    {
			(yyval.i) = (yyvsp[0].i);
		  }
#line 4550 "y.tab.c" /* yacc.c:1646  */
    break;

  case 287:
#line 1894 "lev_comp.y" /* yacc.c:1646  */
    {
			(yyval.i) = (yyvsp[0].i);
		  }
#line 4558 "y.tab.c" /* yacc.c:1646  */
    break;

  case 288:
#line 1898 "lev_comp.y" /* yacc.c:1646  */
    {
			(yyval.i) = (yyvsp[-2].i) | (yyvsp[0].i);
		  }
#line 4566 "y.tab.c" /* yacc.c:1646  */
    break;

  case 289:
#line 1905 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = ((yyvsp[0].i) << 0);
		  }
#line 4574 "y.tab.c" /* yacc.c:1646  */
    break;

  case 290:
#line 1909 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = ((yyvsp[0].i) << 1);
		  }
#line 4582 "y.tab.c" /* yacc.c:1646  */
    break;

  case 291:
#line 1913 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = ((yyvsp[0].i) << 2);
		  }
#line 4590 "y.tab.c" /* yacc.c:1646  */
    break;

  case 298:
#line 1929 "lev_comp.y" /* yacc.c:1646  */
    {
			(yyval.i) = - MAX_REGISTERS - 1;
		  }
#line 4598 "y.tab.c" /* yacc.c:1646  */
    break;

  case 301:
#line 1937 "lev_comp.y" /* yacc.c:1646  */
    {
			(yyval.i) = - MAX_REGISTERS - 1;
		  }
#line 4606 "y.tab.c" /* yacc.c:1646  */
    break;

  case 304:
#line 1947 "lev_comp.y" /* yacc.c:1646  */
    {
			if ( (yyvsp[-1].i) >= 3 )
				lc_error("Register Index overflow!");
			else
				(yyval.i) = - (yyvsp[-1].i) - 1;
		  }
#line 4617 "y.tab.c" /* yacc.c:1646  */
    break;

  case 305:
#line 1956 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "s", (yyvsp[0].map));
		      Free((yyvsp[0].map));
		  }
#line 4626 "y.tab.c" /* yacc.c:1646  */
    break;

  case 306:
#line 1961 "lev_comp.y" /* yacc.c:1646  */
    {
		      check_vardef_type(variable_definitions, (yyvsp[0].map), SPOVAR_STRING);
		      vardef_used(variable_definitions, (yyvsp[0].map));
		      add_opvars(splev, "v", (yyvsp[0].map));
		      Free((yyvsp[0].map));
		  }
#line 4637 "y.tab.c" /* yacc.c:1646  */
    break;

  case 307:
#line 1968 "lev_comp.y" /* yacc.c:1646  */
    {
		      check_vardef_type(variable_definitions, (yyvsp[-3].map), SPOVAR_STRING|SPOVAR_ARRAY);
		      vardef_used(variable_definitions, (yyvsp[-3].map));
		      add_opvars(splev, "v", (yyvsp[-3].map));
		      Free((yyvsp[-3].map));
		  }
#line 4648 "y.tab.c" /* yacc.c:1646  */
    break;

  case 308:
#line 1978 "lev_comp.y" /* yacc.c:1646  */
    {
		      /* nothing */
		  }
#line 4656 "y.tab.c" /* yacc.c:1646  */
    break;

  case 309:
#line 1984 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "c", (yyvsp[0].i));
		  }
#line 4664 "y.tab.c" /* yacc.c:1646  */
    break;

  case 310:
#line 1988 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "o", SPO_SEL_RNDCOORD);
		  }
#line 4672 "y.tab.c" /* yacc.c:1646  */
    break;

  case 311:
#line 1992 "lev_comp.y" /* yacc.c:1646  */
    {
		      check_vardef_type(variable_definitions, (yyvsp[0].map), SPOVAR_COORD);
		      vardef_used(variable_definitions, (yyvsp[0].map));
		      add_opvars(splev, "v", (yyvsp[0].map));
		      Free((yyvsp[0].map));
		  }
#line 4683 "y.tab.c" /* yacc.c:1646  */
    break;

  case 312:
#line 1999 "lev_comp.y" /* yacc.c:1646  */
    {
		      check_vardef_type(variable_definitions, (yyvsp[-3].map), SPOVAR_COORD|SPOVAR_ARRAY);
		      vardef_used(variable_definitions, (yyvsp[-3].map));
		      add_opvars(splev, "v", (yyvsp[-3].map));
		      Free((yyvsp[-3].map));
		  }
#line 4694 "y.tab.c" /* yacc.c:1646  */
    break;

  case 313:
#line 2008 "lev_comp.y" /* yacc.c:1646  */
    {
		      if ((yyvsp[-3].i) < 0 || (yyvsp[-1].i) < 0 || (yyvsp[-3].i) >= COLNO || (yyvsp[-1].i) >= ROWNO)
			  lc_error("Coordinates (%li,%li) out of map range!", (yyvsp[-3].i), (yyvsp[-1].i));
		      (yyval.i) = SP_COORD_PACK((yyvsp[-3].i), (yyvsp[-1].i));
		  }
#line 4704 "y.tab.c" /* yacc.c:1646  */
    break;

  case 314:
#line 2014 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = SP_COORD_PACK_RANDOM(0);
		  }
#line 4712 "y.tab.c" /* yacc.c:1646  */
    break;

  case 315:
#line 2018 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = SP_COORD_PACK_RANDOM( (yyvsp[-1].i) );
		  }
#line 4720 "y.tab.c" /* yacc.c:1646  */
    break;

  case 316:
#line 2024 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = (yyvsp[0].i);
		  }
#line 4728 "y.tab.c" /* yacc.c:1646  */
    break;

  case 317:
#line 2028 "lev_comp.y" /* yacc.c:1646  */
    {
		      if (((yyvsp[-2].i) & (yyvsp[0].i)))
			  lc_warning("Humidity flag used twice.");
		      (yyval.i) = ((yyvsp[-2].i) | (yyvsp[0].i));
		  }
#line 4738 "y.tab.c" /* yacc.c:1646  */
    break;

  case 318:
#line 2036 "lev_comp.y" /* yacc.c:1646  */
    {
		      /* nothing */
		  }
#line 4746 "y.tab.c" /* yacc.c:1646  */
    break;

  case 319:
#line 2040 "lev_comp.y" /* yacc.c:1646  */
    {
		      check_vardef_type(variable_definitions, (yyvsp[0].map), SPOVAR_REGION);
		      vardef_used(variable_definitions, (yyvsp[0].map));
		      add_opvars(splev, "v", (yyvsp[0].map));
		      Free((yyvsp[0].map));
		  }
#line 4757 "y.tab.c" /* yacc.c:1646  */
    break;

  case 320:
#line 2047 "lev_comp.y" /* yacc.c:1646  */
    {
		      check_vardef_type(variable_definitions, (yyvsp[-3].map), SPOVAR_REGION|SPOVAR_ARRAY);
		      vardef_used(variable_definitions, (yyvsp[-3].map));
		      add_opvars(splev, "v", (yyvsp[-3].map));
		      Free((yyvsp[-3].map));
		  }
#line 4768 "y.tab.c" /* yacc.c:1646  */
    break;

  case 321:
#line 2056 "lev_comp.y" /* yacc.c:1646  */
    {
		      long r = SP_REGION_PACK((yyvsp[-7].i), (yyvsp[-5].i), (yyvsp[-3].i), (yyvsp[-1].i));
		      if ( (yyvsp[-7].i) > (yyvsp[-3].i) || (yyvsp[-5].i) > (yyvsp[-1].i) )
			  lc_error("Region start > end: (%li,%li,%li,%li)!", (yyvsp[-7].i), (yyvsp[-5].i), (yyvsp[-3].i), (yyvsp[-1].i));

		      add_opvars(splev, "r", r);
		      (yyval.i) = r;
		  }
#line 4781 "y.tab.c" /* yacc.c:1646  */
    break;

  case 322:
#line 2067 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "m", (yyvsp[0].i));
		  }
#line 4789 "y.tab.c" /* yacc.c:1646  */
    break;

  case 323:
#line 2071 "lev_comp.y" /* yacc.c:1646  */
    {
		      check_vardef_type(variable_definitions, (yyvsp[0].map), SPOVAR_MAPCHAR);
		      vardef_used(variable_definitions, (yyvsp[0].map));
		      add_opvars(splev, "v", (yyvsp[0].map));
		      Free((yyvsp[0].map));
		  }
#line 4800 "y.tab.c" /* yacc.c:1646  */
    break;

  case 324:
#line 2078 "lev_comp.y" /* yacc.c:1646  */
    {
		      check_vardef_type(variable_definitions, (yyvsp[-3].map), SPOVAR_MAPCHAR|SPOVAR_ARRAY);
		      vardef_used(variable_definitions, (yyvsp[-3].map));
		      add_opvars(splev, "v", (yyvsp[-3].map));
		      Free((yyvsp[-3].map));
		  }
#line 4811 "y.tab.c" /* yacc.c:1646  */
    break;

  case 325:
#line 2087 "lev_comp.y" /* yacc.c:1646  */
    {
		      if (what_map_char((char) (yyvsp[0].i)) != INVALID_TYPE)
			  (yyval.i) = SP_MAPCHAR_PACK(what_map_char((char) (yyvsp[0].i)), -2);
		      else {
			  lc_error("Unknown map char type '%c'!", (yyvsp[0].i));
			  (yyval.i) = SP_MAPCHAR_PACK(STONE, -2);
		      }
		  }
#line 4824 "y.tab.c" /* yacc.c:1646  */
    break;

  case 326:
#line 2096 "lev_comp.y" /* yacc.c:1646  */
    {
		      if (what_map_char((char) (yyvsp[-3].i)) != INVALID_TYPE)
			  (yyval.i) = SP_MAPCHAR_PACK(what_map_char((char) (yyvsp[-3].i)), (yyvsp[-1].i));
		      else {
			  lc_error("Unknown map char type '%c'!", (yyvsp[-3].i));
			  (yyval.i) = SP_MAPCHAR_PACK(STONE, (yyvsp[-1].i));
		      }
		  }
#line 4837 "y.tab.c" /* yacc.c:1646  */
    break;

  case 327:
#line 2107 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "M", (yyvsp[0].i));
		  }
#line 4845 "y.tab.c" /* yacc.c:1646  */
    break;

  case 328:
#line 2111 "lev_comp.y" /* yacc.c:1646  */
    {
		      check_vardef_type(variable_definitions, (yyvsp[0].map), SPOVAR_MONST);
		      vardef_used(variable_definitions, (yyvsp[0].map));
		      add_opvars(splev, "v", (yyvsp[0].map));
		      Free((yyvsp[0].map));
		  }
#line 4856 "y.tab.c" /* yacc.c:1646  */
    break;

  case 329:
#line 2118 "lev_comp.y" /* yacc.c:1646  */
    {
		      check_vardef_type(variable_definitions, (yyvsp[-3].map), SPOVAR_MONST|SPOVAR_ARRAY);
		      vardef_used(variable_definitions, (yyvsp[-3].map));
		      add_opvars(splev, "v", (yyvsp[-3].map));
		      Free((yyvsp[-3].map));
		  }
#line 4867 "y.tab.c" /* yacc.c:1646  */
    break;

  case 330:
#line 2127 "lev_comp.y" /* yacc.c:1646  */
    {
                      long m = get_monster_id((yyvsp[0].map), (char)0);
                      if (m == ERR) {
                          lc_error("Unknown monster \"%s\"!", (yyvsp[0].map));
                          (yyval.i) = -1;
                      } else
                          (yyval.i) = SP_MONST_PACK(m, def_monsyms[(int)mons[m].mlet].sym);
                  }
#line 4880 "y.tab.c" /* yacc.c:1646  */
    break;

  case 331:
#line 2136 "lev_comp.y" /* yacc.c:1646  */
    {
                        if (check_monster_char((char) (yyvsp[0].i)))
                            (yyval.i) = SP_MONST_PACK(-1, (yyvsp[0].i));
                        else {
                            lc_error("Unknown monster class '%c'!", (yyvsp[0].i));
                            (yyval.i) = -1;
                        }
                  }
#line 4893 "y.tab.c" /* yacc.c:1646  */
    break;

  case 332:
#line 2145 "lev_comp.y" /* yacc.c:1646  */
    {
                      long m = get_monster_id((yyvsp[-1].map), (char) (yyvsp[-3].i));
                      if (m == ERR) {
                          lc_error("Unknown monster ('%c', \"%s\")!", (yyvsp[-3].i), (yyvsp[-1].map));
                          (yyval.i) = -1;
                      } else
                          (yyval.i) = SP_MONST_PACK(m, (yyvsp[-3].i));
                  }
#line 4906 "y.tab.c" /* yacc.c:1646  */
    break;

  case 333:
#line 2154 "lev_comp.y" /* yacc.c:1646  */
    {
                      (yyval.i) = -1;
                  }
#line 4914 "y.tab.c" /* yacc.c:1646  */
    break;

  case 334:
#line 2160 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "O", (yyvsp[0].i));
		  }
#line 4922 "y.tab.c" /* yacc.c:1646  */
    break;

  case 335:
#line 2164 "lev_comp.y" /* yacc.c:1646  */
    {
		      check_vardef_type(variable_definitions, (yyvsp[0].map), SPOVAR_OBJ);
		      vardef_used(variable_definitions, (yyvsp[0].map));
		      add_opvars(splev, "v", (yyvsp[0].map));
		      Free((yyvsp[0].map));
		  }
#line 4933 "y.tab.c" /* yacc.c:1646  */
    break;

  case 336:
#line 2171 "lev_comp.y" /* yacc.c:1646  */
    {
		      check_vardef_type(variable_definitions, (yyvsp[-3].map), SPOVAR_OBJ|SPOVAR_ARRAY);
		      vardef_used(variable_definitions, (yyvsp[-3].map));
		      add_opvars(splev, "v", (yyvsp[-3].map));
		      Free((yyvsp[-3].map));
		  }
#line 4944 "y.tab.c" /* yacc.c:1646  */
    break;

  case 337:
#line 2180 "lev_comp.y" /* yacc.c:1646  */
    {
		      long m = get_object_id((yyvsp[0].map), (char)0);
		      if (m == ERR) {
			  lc_error("Unknown object \"%s\"!", (yyvsp[0].map));
			  (yyval.i) = -1;
		      } else
			  (yyval.i) = SP_OBJ_PACK(m, 1); /* obj class != 0 to force generation of a specific item */

		  }
#line 4958 "y.tab.c" /* yacc.c:1646  */
    break;

  case 338:
#line 2190 "lev_comp.y" /* yacc.c:1646  */
    {
			if (check_object_char((char) (yyvsp[0].i)))
			    (yyval.i) = SP_OBJ_PACK(-1, (yyvsp[0].i));
			else {
			    lc_error("Unknown object class '%c'!", (yyvsp[0].i));
			    (yyval.i) = -1;
			}
		  }
#line 4971 "y.tab.c" /* yacc.c:1646  */
    break;

  case 339:
#line 2199 "lev_comp.y" /* yacc.c:1646  */
    {
		      long m = get_object_id((yyvsp[-1].map), (char) (yyvsp[-3].i));
		      if (m == ERR) {
			  lc_error("Unknown object ('%c', \"%s\")!", (yyvsp[-3].i), (yyvsp[-1].map));
			  (yyval.i) = -1;
		      } else
			  (yyval.i) = SP_OBJ_PACK(m, (yyvsp[-3].i));
		  }
#line 4984 "y.tab.c" /* yacc.c:1646  */
    break;

  case 340:
#line 2208 "lev_comp.y" /* yacc.c:1646  */
    {
		      (yyval.i) = -1;
		  }
#line 4992 "y.tab.c" /* yacc.c:1646  */
    break;

  case 341:
#line 2214 "lev_comp.y" /* yacc.c:1646  */
    { }
#line 4998 "y.tab.c" /* yacc.c:1646  */
    break;

  case 342:
#line 2216 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "o", SPO_MATH_ADD);
		  }
#line 5006 "y.tab.c" /* yacc.c:1646  */
    break;

  case 343:
#line 2221 "lev_comp.y" /* yacc.c:1646  */
    { add_opvars(splev, "i", (yyvsp[0].i) ); }
#line 5012 "y.tab.c" /* yacc.c:1646  */
    break;

  case 344:
#line 2222 "lev_comp.y" /* yacc.c:1646  */
    { is_inconstant_number = 1; }
#line 5018 "y.tab.c" /* yacc.c:1646  */
    break;

  case 345:
#line 2223 "lev_comp.y" /* yacc.c:1646  */
    { add_opvars(splev, "i", (yyvsp[-1].i) ); }
#line 5024 "y.tab.c" /* yacc.c:1646  */
    break;

  case 346:
#line 2225 "lev_comp.y" /* yacc.c:1646  */
    {
		      check_vardef_type(variable_definitions, (yyvsp[0].map), SPOVAR_INT);
		      vardef_used(variable_definitions, (yyvsp[0].map));
		      add_opvars(splev, "v", (yyvsp[0].map));
		      Free((yyvsp[0].map));
		      is_inconstant_number = 1;
		  }
#line 5036 "y.tab.c" /* yacc.c:1646  */
    break;

  case 347:
#line 2233 "lev_comp.y" /* yacc.c:1646  */
    {
		      check_vardef_type(variable_definitions, (yyvsp[-3].map), SPOVAR_INT|SPOVAR_ARRAY);
		      vardef_used(variable_definitions, (yyvsp[-3].map));
		      add_opvars(splev, "v", (yyvsp[-3].map));
		      Free((yyvsp[-3].map));
		      is_inconstant_number = 1;
		  }
#line 5048 "y.tab.c" /* yacc.c:1646  */
    break;

  case 348:
#line 2240 "lev_comp.y" /* yacc.c:1646  */
    { add_opvars(splev, "o", SPO_MATH_ADD); }
#line 5054 "y.tab.c" /* yacc.c:1646  */
    break;

  case 349:
#line 2241 "lev_comp.y" /* yacc.c:1646  */
    { add_opvars(splev, "o", SPO_MATH_SUB); }
#line 5060 "y.tab.c" /* yacc.c:1646  */
    break;

  case 350:
#line 2242 "lev_comp.y" /* yacc.c:1646  */
    { add_opvars(splev, "o", SPO_MATH_MUL); }
#line 5066 "y.tab.c" /* yacc.c:1646  */
    break;

  case 351:
#line 2243 "lev_comp.y" /* yacc.c:1646  */
    { add_opvars(splev, "o", SPO_MATH_DIV); }
#line 5072 "y.tab.c" /* yacc.c:1646  */
    break;

  case 352:
#line 2244 "lev_comp.y" /* yacc.c:1646  */
    { add_opvars(splev, "o", SPO_MATH_MOD); }
#line 5078 "y.tab.c" /* yacc.c:1646  */
    break;

  case 353:
#line 2245 "lev_comp.y" /* yacc.c:1646  */
    { }
#line 5084 "y.tab.c" /* yacc.c:1646  */
    break;

  case 354:
#line 2249 "lev_comp.y" /* yacc.c:1646  */
    {
			      if (!strcmp("int", (yyvsp[0].map)) || !strcmp("integer", (yyvsp[0].map))) {
				  (yyval.i) = (int)'i';
			      } else lc_error("Unknown function parameter type '%s'", (yyvsp[0].map));
			  }
#line 5094 "y.tab.c" /* yacc.c:1646  */
    break;

  case 355:
#line 2255 "lev_comp.y" /* yacc.c:1646  */
    {
			      if (!strcmp("str", (yyvsp[0].map)) || !strcmp("string", (yyvsp[0].map))) {
				  (yyval.i) = (int)'s';
			      } else lc_error("Unknown function parameter type '%s'", (yyvsp[0].map));
			  }
#line 5104 "y.tab.c" /* yacc.c:1646  */
    break;

  case 356:
#line 2263 "lev_comp.y" /* yacc.c:1646  */
    {
			      struct lc_funcdefs_parm *tmp = New(struct lc_funcdefs_parm);

			      if (!curr_function)
				  lc_error("Function parameters outside function definition.");
			      else if (!tmp)
				  lc_error("Could not alloc function params.");
			      else {
				  tmp->name = strdup((yyvsp[-2].map));
				  tmp->parmtype = (char) (yyvsp[0].i);
				  tmp->next = curr_function->params;
				  curr_function->params = tmp;
				  curr_function->n_params++;
				  {
				      long vt;
				      switch (tmp->parmtype) {
				      case 'i': vt = SPOVAR_INT; break;
				      case 's': vt = SPOVAR_STRING; break;
				      default: lc_error("Unknown func param conversion."); break;
				      }
				      variable_definitions = add_vardef_type(variable_definitions, (yyvsp[-2].map), vt);
				  }
			      }
			      Free((yyvsp[-2].map));
			  }
#line 5134 "y.tab.c" /* yacc.c:1646  */
    break;

  case 361:
#line 2300 "lev_comp.y" /* yacc.c:1646  */
    {
			      (yyval.i) = (int)'i';
			  }
#line 5142 "y.tab.c" /* yacc.c:1646  */
    break;

  case 362:
#line 2304 "lev_comp.y" /* yacc.c:1646  */
    {
			      (yyval.i) = (int)'s';
			  }
#line 5150 "y.tab.c" /* yacc.c:1646  */
    break;

  case 363:
#line 2311 "lev_comp.y" /* yacc.c:1646  */
    {
			      char tmpbuf[2];
			      tmpbuf[0] = (char) (yyvsp[0].i);
			      tmpbuf[1] = '\0';
			      (yyval.map) = strdup(tmpbuf);
			  }
#line 5161 "y.tab.c" /* yacc.c:1646  */
    break;

  case 364:
#line 2318 "lev_comp.y" /* yacc.c:1646  */
    {
			      long len = strlen( (yyvsp[-2].map) );
			      char *tmp = (char *)alloc(len + 2);
			      sprintf(tmp, "%c%s", (char) (yyvsp[0].i), (yyvsp[-2].map) );
			      Free( (yyvsp[-2].map) );
			      (yyval.map) = tmp;
			  }
#line 5173 "y.tab.c" /* yacc.c:1646  */
    break;

  case 365:
#line 2328 "lev_comp.y" /* yacc.c:1646  */
    {
			      (yyval.map) = strdup("");
			  }
#line 5181 "y.tab.c" /* yacc.c:1646  */
    break;

  case 366:
#line 2332 "lev_comp.y" /* yacc.c:1646  */
    {
			      char *tmp = strdup( (yyvsp[0].map) );
			      Free( (yyvsp[0].map) );
			      (yyval.map) = tmp;
			  }
#line 5191 "y.tab.c" /* yacc.c:1646  */
    break;

  case 367:
#line 2340 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "o", SPO_SEL_POINT);
		  }
#line 5199 "y.tab.c" /* yacc.c:1646  */
    break;

  case 368:
#line 2344 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "o", SPO_SEL_RECT);
		  }
#line 5207 "y.tab.c" /* yacc.c:1646  */
    break;

  case 369:
#line 2348 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "o", SPO_SEL_FILLRECT);
		  }
#line 5215 "y.tab.c" /* yacc.c:1646  */
    break;

  case 370:
#line 2352 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "o", SPO_SEL_LINE);
		  }
#line 5223 "y.tab.c" /* yacc.c:1646  */
    break;

  case 371:
#line 2356 "lev_comp.y" /* yacc.c:1646  */
    {
		      /* randline (x1,y1),(x2,y2), roughness */
		      add_opvars(splev, "o", SPO_SEL_RNDLINE);
		  }
#line 5232 "y.tab.c" /* yacc.c:1646  */
    break;

  case 372:
#line 2361 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "io", W_ANY, SPO_SEL_GROW);
		  }
#line 5240 "y.tab.c" /* yacc.c:1646  */
    break;

  case 373:
#line 2365 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "io", (yyvsp[-3].i), SPO_SEL_GROW);
		  }
#line 5248 "y.tab.c" /* yacc.c:1646  */
    break;

  case 374:
#line 2369 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "iio", (yyvsp[-3].i), SPOFILTER_PERCENT, SPO_SEL_FILTER);
		  }
#line 5256 "y.tab.c" /* yacc.c:1646  */
    break;

  case 375:
#line 2373 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "io", SPOFILTER_SELECTION, SPO_SEL_FILTER);
		  }
#line 5264 "y.tab.c" /* yacc.c:1646  */
    break;

  case 376:
#line 2377 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "io", SPOFILTER_MAPCHAR, SPO_SEL_FILTER);
		  }
#line 5272 "y.tab.c" /* yacc.c:1646  */
    break;

  case 377:
#line 2381 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "o", SPO_SEL_FLOOD);
		  }
#line 5280 "y.tab.c" /* yacc.c:1646  */
    break;

  case 378:
#line 2385 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "oio", SPO_COPY, 1, SPO_SEL_ELLIPSE);
		  }
#line 5288 "y.tab.c" /* yacc.c:1646  */
    break;

  case 379:
#line 2389 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "oio", SPO_COPY, (yyvsp[-1].i), SPO_SEL_ELLIPSE);
		  }
#line 5296 "y.tab.c" /* yacc.c:1646  */
    break;

  case 380:
#line 2393 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "io", 1, SPO_SEL_ELLIPSE);
		  }
#line 5304 "y.tab.c" /* yacc.c:1646  */
    break;

  case 381:
#line 2397 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "io", (yyvsp[-1].i), SPO_SEL_ELLIPSE);
		  }
#line 5312 "y.tab.c" /* yacc.c:1646  */
    break;

  case 382:
#line 2401 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "iio", (yyvsp[-5].i), (yyvsp[-11].i), SPO_SEL_GRADIENT);
		  }
#line 5320 "y.tab.c" /* yacc.c:1646  */
    break;

  case 383:
#line 2405 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "o", SPO_SEL_COMPLEMENT);
		  }
#line 5328 "y.tab.c" /* yacc.c:1646  */
    break;

  case 384:
#line 2409 "lev_comp.y" /* yacc.c:1646  */
    {
		      check_vardef_type(variable_definitions, (yyvsp[0].map), SPOVAR_SEL);
		      vardef_used(variable_definitions, (yyvsp[0].map));
		      add_opvars(splev, "v", (yyvsp[0].map));
		      Free((yyvsp[0].map));
		  }
#line 5339 "y.tab.c" /* yacc.c:1646  */
    break;

  case 385:
#line 2416 "lev_comp.y" /* yacc.c:1646  */
    {
		      /* nothing */
		  }
#line 5347 "y.tab.c" /* yacc.c:1646  */
    break;

  case 386:
#line 2422 "lev_comp.y" /* yacc.c:1646  */
    {
		      /* nothing */
		  }
#line 5355 "y.tab.c" /* yacc.c:1646  */
    break;

  case 387:
#line 2426 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "o", SPO_SEL_ADD);
		  }
#line 5363 "y.tab.c" /* yacc.c:1646  */
    break;

  case 388:
#line 2432 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "iio", (yyvsp[0].dice).num, (yyvsp[0].dice).die, SPO_DICE);
		  }
#line 5371 "y.tab.c" /* yacc.c:1646  */
    break;

  case 392:
#line 2443 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "i", (yyvsp[0].i) );
		  }
#line 5379 "y.tab.c" /* yacc.c:1646  */
    break;

  case 393:
#line 2447 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "i", (yyvsp[0].i) );
		  }
#line 5387 "y.tab.c" /* yacc.c:1646  */
    break;

  case 394:
#line 2451 "lev_comp.y" /* yacc.c:1646  */
    {
		      add_opvars(splev, "i", (yyvsp[0].i) );
		  }
#line 5395 "y.tab.c" /* yacc.c:1646  */
    break;

  case 395:
#line 2455 "lev_comp.y" /* yacc.c:1646  */
    {
		      /* nothing */
		  }
#line 5403 "y.tab.c" /* yacc.c:1646  */
    break;

  case 404:
#line 2477 "lev_comp.y" /* yacc.c:1646  */
    {
			(yyval.lregn) = (yyvsp[0].lregn);
		  }
#line 5411 "y.tab.c" /* yacc.c:1646  */
    break;

  case 405:
#line 2481 "lev_comp.y" /* yacc.c:1646  */
    {
			if ((yyvsp[-7].i) <= 0 || (yyvsp[-7].i) >= COLNO)
			    lc_error("Region (%li,%li,%li,%li) out of level range (x1)!", (yyvsp[-7].i), (yyvsp[-5].i), (yyvsp[-3].i), (yyvsp[-1].i));
			else if ((yyvsp[-5].i) < 0 || (yyvsp[-5].i) >= ROWNO)
			    lc_error("Region (%li,%li,%li,%li) out of level range (y1)!", (yyvsp[-7].i), (yyvsp[-5].i), (yyvsp[-3].i), (yyvsp[-1].i));
			else if ((yyvsp[-3].i) <= 0 || (yyvsp[-3].i) >= COLNO)
			    lc_error("Region (%li,%li,%li,%li) out of level range (x2)!", (yyvsp[-7].i), (yyvsp[-5].i), (yyvsp[-3].i), (yyvsp[-1].i));
			else if ((yyvsp[-1].i) < 0 || (yyvsp[-1].i) >= ROWNO)
			    lc_error("Region (%li,%li,%li,%li) out of level range (y2)!", (yyvsp[-7].i), (yyvsp[-5].i), (yyvsp[-3].i), (yyvsp[-1].i));
			(yyval.lregn).x1 = (yyvsp[-7].i);
			(yyval.lregn).y1 = (yyvsp[-5].i);
			(yyval.lregn).x2 = (yyvsp[-3].i);
			(yyval.lregn).y2 = (yyvsp[-1].i);
			(yyval.lregn).area = 1;
		  }
#line 5431 "y.tab.c" /* yacc.c:1646  */
    break;

  case 406:
#line 2499 "lev_comp.y" /* yacc.c:1646  */
    {
/* This series of if statements is a hack for MSC 5.1.  It seems that its
   tiny little brain cannot compile if these are all one big if statement. */
			if ((yyvsp[-7].i) < 0 || (yyvsp[-7].i) > (int)max_x_map)
			    lc_error("Region (%li,%li,%li,%li) out of map range (x1)!", (yyvsp[-7].i), (yyvsp[-5].i), (yyvsp[-3].i), (yyvsp[-1].i));
			else if ((yyvsp[-5].i) < 0 || (yyvsp[-5].i) > (int)max_y_map)
			    lc_error("Region (%li,%li,%li,%li) out of map range (y1)!", (yyvsp[-7].i), (yyvsp[-5].i), (yyvsp[-3].i), (yyvsp[-1].i));
			else if ((yyvsp[-3].i) < 0 || (yyvsp[-3].i) > (int)max_x_map)
			    lc_error("Region (%li,%li,%li,%li) out of map range (x2)!", (yyvsp[-7].i), (yyvsp[-5].i), (yyvsp[-3].i), (yyvsp[-1].i));
			else if ((yyvsp[-1].i) < 0 || (yyvsp[-1].i) > (int)max_y_map)
			    lc_error("Region (%li,%li,%li,%li) out of map range (y2)!", (yyvsp[-7].i), (yyvsp[-5].i), (yyvsp[-3].i), (yyvsp[-1].i));
			(yyval.lregn).area = 0;
			(yyval.lregn).x1 = (yyvsp[-7].i);
			(yyval.lregn).y1 = (yyvsp[-5].i);
			(yyval.lregn).x2 = (yyvsp[-3].i);
			(yyval.lregn).y2 = (yyvsp[-1].i);
		  }
#line 5453 "y.tab.c" /* yacc.c:1646  */
    break;


#line 5457 "y.tab.c" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
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

  /* Do not reclaim the symbols of the rule whose action triggered
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
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
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

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


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

#if !defined yyoverflow || YYERROR_VERBOSE
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
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
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
  return yyresult;
}
#line 2519 "lev_comp.y" /* yacc.c:1906  */


/*lev_comp.y*/
