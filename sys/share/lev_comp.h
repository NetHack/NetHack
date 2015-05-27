
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
     INTEGER = 258,
     A_DUNGEON = 259,
     BRANCH = 260,
     CHBRANCH = 261,
     LEVEL = 262,
     RNDLEVEL = 263,
     CHLEVEL = 264,
     RNDCHLEVEL = 265,
     UP_OR_DOWN = 266,
     PROTOFILE = 267,
     DESCRIPTION = 268,
     DESCRIPTOR = 269,
     LEVELDESC = 270,
     ALIGNMENT = 271,
     LEVALIGN = 272,
     ENTRY = 273,
     STAIR = 274,
     NO_UP = 275,
     NO_DOWN = 276,
     PORTAL = 277,
     STRING = 278
   };
#endif
/* Tokens.  */
#define INTEGER 258
#define A_DUNGEON 259
#define BRANCH 260
#define CHBRANCH 261
#define LEVEL 262
#define RNDLEVEL 263
#define CHLEVEL 264
#define RNDCHLEVEL 265
#define UP_OR_DOWN 266
#define PROTOFILE 267
#define DESCRIPTION 268
#define DESCRIPTOR 269
#define LEVELDESC 270
#define ALIGNMENT 271
#define LEVALIGN 272
#define ENTRY 273
#define STAIR 274
#define NO_UP 275
#define NO_DOWN 276
#define PORTAL 277
#define STRING 278




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 67 "dgn_comp.y"

	int	i;
	char*	str;



/* Line 1676 of yacc.c  */
#line 105 "y.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


