%{
/* NetHack 3.6  dgn_comp.l	$NHDT-Date: 1455415007 2016/02/14 01:56:47 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.13 $ */
/*	Copyright (c) 1989 by Jean-Christophe Collet */
/*	Copyright (c) 1990 by M. Stephenson	     */
/* NetHack may be freely redistributed.  See license for details. */

#define DGN_COMP

#include "config.h"
#include "dgn_comp.h"
#include "dgn_file.h"

/*
 * Most of these don't exist in flex, yywrap is macro and
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

/* this doesn't always get put in dgn_comp.h
 * (esp. when using older versions of bison)
 */
extern YYSTYPE yylval;

int nh_line_number = 1;

%}
%%
DUNGEON		return(A_DUNGEON);
up		{ yylval.i=1; return(UP_OR_DOWN); }
down		{ yylval.i=0; return(UP_OR_DOWN); }
ENTRY		return(ENTRY);
stair		return(STAIR);
no_up		return(NO_UP);
no_down		return(NO_DOWN);
portal		return(PORTAL);
PROTOFILE	return(PROTOFILE);
DESCRIPTION	return(DESCRIPTION);
LEVELDESC	return(LEVELDESC);
ALIGNMENT       return(ALIGNMENT);
LEVALIGN        return(LEVALIGN);
town		{ yylval.i=TOWN ; return(DESCRIPTOR); }
hellish		{ yylval.i=HELLISH ; return(DESCRIPTOR); }
mazelike	{ yylval.i=MAZELIKE ; return(DESCRIPTOR); }
roguelike	{ yylval.i=ROGUELIKE ; return(DESCRIPTOR); }
unaligned       { yylval.i=D_ALIGN_NONE ; return(DESCRIPTOR); }
noalign         { yylval.i=D_ALIGN_NONE ; return(DESCRIPTOR); }
lawful          { yylval.i=D_ALIGN_LAWFUL ; return(DESCRIPTOR); }
neutral         { yylval.i=D_ALIGN_NEUTRAL ; return(DESCRIPTOR); }
chaotic         { yylval.i=D_ALIGN_CHAOTIC ; return(DESCRIPTOR); }
BRANCH		return(BRANCH);
CHAINBRANCH	return(CHBRANCH);
LEVEL		return(LEVEL);
RNDLEVEL	return(RNDLEVEL);
CHAINLEVEL	return(CHLEVEL);
RNDCHLEVEL	return(RNDCHLEVEL);
[-0-9]+		{ yylval.i=atoi(yytext); return(INTEGER); }
\"[^"]*\"	{ yytext[yyleng - 1] = '\0'; /* discard the trailing \" */
		  yylval.str = dupstr(yytext + 1); /* skip the first \" */
		  return STRING; }
^#.*\n		{ nh_line_number++; }
\r?\n		{ nh_line_number++; }
[ \t]+		;	/* skip trailing tabs & spaces */
.		{ return yytext[0]; }
%%

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
}

/* analogous routine (for completeness) */
void
init_yyout( output_f )
FILE *output_f;
{
    yyout = output_f;
}

/*dgn_comp.l*/
