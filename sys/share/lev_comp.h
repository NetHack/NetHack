
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
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

/* Line 1676 of yacc.c  */
#line 146 "lev_comp.y"

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



/* Line 1676 of yacc.c  */
#line 443 "y.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


