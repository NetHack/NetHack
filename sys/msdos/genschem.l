%{

#include "config.h"
#include <stdlib.h>
#include <ctype.h>

#define	MALLOC(type)	(type *)malloc(sizeof(type))

typedef unsigned char bool;

struct OvlNode {
	char FileName[13];
	char Comment[80];
	int  OvlNumber;
        struct OvlNode *Next;
};

int linenumber = 1;
int OvlNum = -1;
FILE *outf = NULL;
char *ovldesc = NULL;
struct OvlNode *List = NULL;
char *CommentTemplate;

struct OvlNode * FDECL(AddNode, (struct OvlNode *, struct OvlNode *));
struct OvlNode * FDECL(DelNode, (struct OvlNode *));
struct OvlNode * FDECL(ReverseList, (struct OvlNode *));
struct OvlNode * FDECL(SortList, (struct OvlNode *));
int NDECL (yylex);

#ifndef	yywrap
int NDECL (yywrap);
#endif

#ifdef exit
#undef exit
#endif

%}

FILECH	[A-Za-z0-9_]
FILE	{FILECH}{1,8}("."{FILECH}{0,3})?
SCCS	[Ss][Cc][Cc][Ss].*\n
COPYR	[Cc][Oo][Pp][Yy][Rr][Ii][Gg][Hh][Tt].*\n

%%

{SCCS}	{ linenumber ++; }
{COPYR}	{ linenumber ++; }
^#.*\n	{
	    if (OvlNum < 0) {
		yytext[yyleng - 1] = 0;
		fprintf (outf, CommentTemplate, yytext+1);
	    }
	    linenumber ++;
	}
\n	{ linenumber ++; }
[ \t]+	;	/* skip trailing tabs & spaces */
\[.*\]	{
	    yytext[yyleng-1] = 0; /* Discard the trailing \] */
	    if (ovldesc) free (ovldesc);
	    ovldesc = (char *) malloc(strlen(yytext+1)+1);
	    strcpy(ovldesc, yytext+1); /* Discard the first \[ */
	    OvlNum++;
	}
{FILE}	{
	    struct OvlNode *Tmp;

	    Tmp = MALLOC (struct OvlNode);
	    strcpy (Tmp->FileName, yytext);
	    Tmp->OvlNumber = OvlNum;
	    strncpy (Tmp->Comment, ovldesc, 80);
	    List = AddNode (List, Tmp);
	}
.	{
	    printf ("Line %d: Received character '%c' (%02x)\n",
			linenumber, *yytext, *yytext);
	}


%%

#ifndef yywrap
int
yywrap()
{
	return 1;
}
#endif

#if defined(UNIX) || defined(_MSC_VER)
char *
strlwr(s)
char *s;
{
	char *p;
	for (p = s; *p; p++)
		if (isupper(*p)) *p = tolower(*p);
	return s;
}

char *
strupr(s)
char *s;
{
	char *p;
	for (p = s; *p; p++)
		if (islower(*p)) *p = toupper(*p);
	return s;
}
#endif

struct OvlNode *
SortList (List)
struct OvlNode *List;
{
	struct OvlNode *List1 = NULL;
	struct OvlNode *List2 = NULL;
	struct OvlNode *Tmp;

	if (List == NULL) return NULL;
	if (List->Next == NULL) return List;

	while (List != NULL) {
	    if (List != NULL) {
		Tmp = List->Next;
		List1 = AddNode (List1, List);
		List = Tmp;
	    }
	    if (List != NULL) {
		Tmp = List->Next;
		List2 = AddNode (List2, List);
		List = Tmp;
	    }
	}

	List1 = SortList (List1);
	List2 = SortList (List2);

	while (List1 != NULL || List2 != NULL) {
	    while (List1 != NULL && List2 == NULL) {
		Tmp = List1->Next;
		List = AddNode (List, List1);
		List1 = Tmp;
	    }
	    while (List1 != NULL &&
		    strcmp(List1->FileName, List2->FileName) <= 0) {
		Tmp = List1->Next;
		List = AddNode (List, List1);
		List1 = Tmp;
	    }
	    while (List2 != NULL && List1 == NULL) {
		Tmp = List2->Next;
		List = AddNode (List, List2);
		List2 = Tmp;
	    }
	    while (List2 != NULL &&
		    strcmp(List1->FileName, List2->FileName) >= 0) {
		Tmp = List2->Next;
		List = AddNode (List, List2);
		List2 = Tmp;
	    }
	}

	return ReverseList (List);
}

struct OvlNode *
AddNode (List, ToAdd)
struct OvlNode *List;
struct OvlNode *ToAdd;
{
	ToAdd->Next = List;
        return ToAdd;
}

struct OvlNode *
DelNode (List)
struct OvlNode *List;
{
	struct OvlNode *tmp;

        tmp = List->Next;
        free (List);
        return tmp;
}

struct OvlNode *
DelOvlList (List)
struct OvlNode *List;
{
	while (List != NULL)
            List = DelNode (List);
	return List;
}

struct OvlNode *
ReverseList (List)
struct OvlNode *List;
{
	struct OvlNode *Temp, *Last;

	Last = NULL; Temp = List;
	while (Temp) {
	    Temp = List->Next;
	    List->Next = Last;
	    Last = List;
	    List = Temp;
	}
	return Last;
}

/*
 * Deletes all nodes with filename equal to that of the first node, except
 * for the first node, itself, which it keeps.
 */

void
DelFile (List)
struct OvlNode *List;
{
	struct OvlNode *tmp;

        tmp = List;
        while (tmp->Next != NULL) {
	    if (!stricmp(List->FileName, tmp->Next->FileName))
                tmp->Next = DelNode (tmp->Next);
            else
                tmp = tmp->Next;
        };
}

int
InList (List, ToFind)
struct OvlNode *List;
struct OvlNode *ToFind;
{
        while (List != NULL) {
            if (!stricmp(List->FileName, ToFind->FileName))
                return 1;
            List = List->Next;
        };
        return 0;
}

int
main (argc, argv)
int argc;
char *argv[];
{
        bool MSC, BC;
	char *Header, *Header2, *Header3, *RootLine, *OvlLine;
	char *c;
	char FileName[9];
	time_t timer;
	struct tm *curtim;

        if (argc < 3) {

	printf ("Too few arguments.  Correct usage is:\n");
	printf ("\t%s {/MSC || /BC} schemafile deffile\n\n", "genschem");
	printf ("\t{/MSC || /BC} indicate the compiler to use.\n");
	printf ("\tschemafile is the schema file to process.\n");
	printf ("\tdeffile is the definition file to produce.\n");
	printf ("\t\tif deffile is missing, stdout is assumed.\n\n");
        	return 1;
	};
	if (!stricmp(argv[1], "/MSC") || !stricmp(argv[1], "-MSC")) {
            MSC = TRUE;
            BC = FALSE;
	} else if (!stricmp(argv[1], "/BC") || !stricmp(argv[1], "-BC")) {
            MSC = FALSE;
            BC = TRUE;
	} else {
            fprintf (stderr, "Unknown compiler format: %s\n", argv[1]);
            return 1;
	};

	Header = BC ? "/* SCCS Id: @(#)%s\t3.4\t %02d/%02d/%02d */\n" :
	    "; SCCS Id: @(#)%s\t3.4\t %02d/%02d/%02d\n";
	Header2 = BC ? "/* Copyright (c) Yitzhak Sapir, %d */\n" :
	    "; Copyright (c) NetHack PC Development Team, %d\n";
	Header3 = BC ? "\n\n" : ";\n\nSEGMENTS\n\n";
	RootLine = BC ? "-zC%s\n" :
	    "\"%s\" OVL:0\n";
	OvlLine = BC ?
	    "-zC%s -zAOVLY -zCOVL%d\n" :
	    "\"%s\" OVL:%d\n";
	CommentTemplate = BC ? "/* %s */\n" : ";%s\n";

	yyin = fopen (argv[2], "r");

	if (yyin == NULL) {
	    fprintf (stderr, "Error: Input file incorrect\n");
	    exit (1);
	}

	outf = fopen (argv[3], "w");

	if (outf == NULL)
	    if (argc == 4) {
		fprintf (stderr, "Error: Output file incorrect\n");
		exit (1);
	    } else outf = stdout;

	time (&timer);
	curtim = localtime(&timer);
	fprintf (outf, Header, argv[3], curtim->tm_year, curtim->tm_mon + 1,
		curtim->tm_mday);
	fprintf (outf, Header2, curtim->tm_year +1900);
	yylex();

	fprintf (outf, Header3);

	for (List = SortList (List); List != NULL; List = DelNode (List)) {
	    if (BC) {
		for (c = strlwr(List->FileName); *c; c++)
		    if (*c == '.') *c = '_';
	    } else strupr(List->FileName);
	    if (List->OvlNumber)
		fprintf (outf, OvlLine, List->FileName, List->OvlNumber);
	    else fprintf (outf, RootLine, List->FileName, List->FileName);
	}
	fclose (outf);

}
