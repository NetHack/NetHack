/*	SCCS Id: @(#)def2mak.c	3.4	1995/03/19	*/
/* Copyright (c) NetHack PC Development Team, 1994. */
/* NetHack may be freely redistributed.  See license for details. */

#include "config.h"
#include <malloc.h>
#include <ctype.h>
#ifndef _MSC_VER
#include <stdlib.h>
#else
int __cdecl _strcmpi(const char *, const char *);
int __cdecl _stricmp(const char *, const char *);
int __cdecl _strnicmp(const char *, const char *,size_t);
#endif

#define	MALLOC(type)	(type *)malloc(sizeof(type))

#define MACROID_SIZ	50
#define MACROVAL_SIZ	300

typedef unsigned char bool;

struct MacroNode * FDECL(AddMacro, (struct MacroNode *, char *, char *));
char * FDECL(ApplyMacros, (char *, struct MacroNode *));
struct MacroNode * FDECL(DelMacro, (struct MacroNode *));
struct MacroNode * FDECL(DelMacroList, (struct MacroNode *));
void FDECL(COMPILER, (bool, char *, char *, FILE *));
void FDECL(SaveNewLine, (char *, char *));
struct ListItem * FDECL(AddItem, (struct ListItem *, char *));
struct ListItem * FDECL(ReadList, (FILE *, int));
void FDECL(LINKLIST, (bool, struct ListItem *, char *, char *));
struct ListTable * FDECL(AddList, (struct ListTable *, struct ListItem *,
    char *));
struct ListItem * FDECL(DelTableList, (struct ListItem *));
struct ListTable * FDECL(DelTable, (struct ListTable *));
struct ListItem * FDECL(FindList, (struct ListTable *, char *));

#ifdef strcmpi
# undef strcmpi		/* don't want to drag in hacklib.c */
#endif

#ifdef _MSC_VER
#define stricmp _strcmpi
#define strnicmp _strnicmp
#endif

int
strcmpi(s1, s2)
char *s1, *s2;
{
	char t1, t2;

	while (*s1 || *s2) {
		if (!*s2) return 1;		/* s1 > s2 */
		else if (!*s1) return -1;	/* s1 < s2 */
		if (isupper(*s1)) t1 = tolower(*s1); else t1 = *s1;
		if (isupper(*s2)) t2 = tolower(*s2); else t2 = *s2;
		if (t1 != t2) return (t1 > t2) ? 1 : -1;
		s1++; s2++;
	}
	return 0;				/* s1 == s2 */
}

struct MacroNode {
	char Name[MACROID_SIZ];
	char Val[MACROVAL_SIZ];
	struct MacroNode *Next;
};

struct MacroNode *
AddMacro (List, MID, MVal)
struct MacroNode *List;
char *MID;
char *MVal;
{
	struct MacroNode *Tmp;
	Tmp = MALLOC (struct MacroNode);
	strncpy (Tmp->Name, MID, MACROID_SIZ);
	strncpy (Tmp->Val, MVal, MACROVAL_SIZ);
	Tmp->Next = List;
        return Tmp;
}

char *
ApplyMacros (Line, List)
char *Line;
struct MacroNode *List;
{
	static char TotalLine[MACROVAL_SIZ + 100];
	char TmpLine[MACROVAL_SIZ + 100];
	int AfterLine;
        char MacroName[MACROID_SIZ];
	struct MacroNode *TmpList;

	strcpy (TotalLine, "");
	strcpy (TmpLine, Line);
        if (sscanf (TmpLine, "%[^?]?[%[^]]]%n", TotalLine, MacroName,
		&AfterLine) <= 1) return Line;
        for (TmpList = List; TmpList; TmpList = TmpList->Next)
	    if (!strcmp(TmpList->Name, MacroName)) {
		strcat (TotalLine, TmpList->Val);
		break;
	    }
	strcat (TotalLine, TmpLine + AfterLine);
	return TotalLine;
}

struct MacroNode *
DelMacro (List)
struct MacroNode *List;
{
	struct MacroNode *tmp;

        tmp = List->Next;
        free (List);
        return tmp;
}

struct MacroNode *
DelMacroList (List)
struct MacroNode *List;
{
	while (List != NULL)
            List = DelMacro (List);
	return List;
}

void
COMPILER (display, SavedNewLine, EndString, input)
bool display;
char *SavedNewLine;
char *EndString;
FILE *input;
{
        char buffer[100];

        fgets (buffer, 100, input);
        while (strnicmp(buffer, EndString, strlen(EndString))) {
            if (display)
                SaveNewLine(buffer, SavedNewLine);
            fgets (buffer, 100, input);
        };
}

void
SaveNewLine (Line, NewLine)
char *Line;
char *NewLine;
{
	char tempNewLine[2];

	strcpy (tempNewLine, "");
	if (Line[strlen(Line)-1] == '\n') {
        	Line[strlen(Line)-1] = '\0';
                strcpy (tempNewLine, "\n");
	};
        printf ("%s%s", NewLine, Line);
        strcpy (NewLine, tempNewLine);
}

struct ListItem {
	char Item[40];
	struct ListItem *next;
};

struct ListTable {
	char name[40];
	struct ListItem *List;
	struct ListTable *next;
};

struct ListItem *
AddItem (List, Item)
struct ListItem *List;
char Item[40];
{
	struct ListItem *temp;

	temp = MALLOC(struct ListItem);
	temp->next = List;
	strncpy (temp->Item, Item, 39);
	return temp;
}

struct ListItem *
ReadList (input, LinePos)
FILE *input;
int LinePos;
{
	char Item[40];
	char buffer[100];
	struct ListItem *List;

	List = NULL;
	fscanf (input, "%39s", Item);
	while (strncmp(Item, "?ENDLIST?", 9)) {
	    if (LinePos > 60) {
		LinePos = 18;
		printf ("\\\n\t\t");
	    }
	    List = AddItem(List, Item);
	    printf ("%s ", Item);
	    LinePos += 1 + strlen(Item);
	    fscanf (input, "%39s", Item);
	}
	printf ("\n");
	fgets (buffer, 100, input);
	return List;
}

void
LINKLIST (BC, List, Listname, SavedNewLine)
bool BC;
struct ListItem *List;
char *Listname;
char *SavedNewLine;
{
	int LinePos;

	printf ("%s\t\t", SavedNewLine);
	strcpy (SavedNewLine, "");

        if (!BC) {
            printf ("$(%s:^\t=+^\n\t\t)\n", Listname);
	    return;
        };

	if (List == NULL) return;

	LinePos = 18;
	while (List != NULL) {
	    if (LinePos > 60) {
		LinePos = 18;
		printf ("+\n\t\t");
	    }
	    printf ("%s ", List->Item);
	    LinePos += 1 + strlen(List->Item);
	    List = List->next;
	}
	printf ("\n");
}

struct ListTable *
AddList (Table, List, Name)
struct ListTable *Table;
struct ListItem *List;
char *Name;
{
	struct ListTable *temp;
	temp = MALLOC (struct ListTable);
	temp->next = Table;
	temp->List = List;
	strncpy (temp->name, Name, 39);
	return temp;
}

struct ListItem *
DelTableList (List)
struct ListItem *List;
{
	struct ListItem *temp;

	while (List != NULL) {
	    temp = List->next;
	    free (List);
	    List = temp;
	};
	return List;
}

struct ListTable *
DelTable (Table)
struct ListTable *Table;
{
	struct ListTable *temp;

	while (Table != NULL) {
	    temp = Table->next;
	    Table->List = DelTableList(Table->List);
	    free(Table);
	    Table=temp;
	};
	return Table;
}

struct ListItem *
FindList (Table, Item)
struct ListTable *Table;
char *Item;
{
	while (Table != NULL) {
	    if (!stricmp(Table->name, Item)) return Table->List;
	    Table = Table->next;
	}
	return NULL;
}

int
main (argc, argv)
int argc;
char *argv[];
{
        FILE *makfile;
        char buffer[100];
        char SavedNewLine[2];
	char Listname[40];
	struct ListTable *Table;
        time_t timer;
        bool MSC, BC;
	struct MacroNode *MacroList = NULL;
	char MacroName[MACROID_SIZ];
	char MacroVal[MACROVAL_SIZ];

        if (argc < 3) {
		printf ("Too few arguments.  Correct usage is:\n");
		printf ("\t%s {/MSC || /BC} template\n\n", "def2mak");
		printf ("\t{/MSC || /BC} indicate the compiler to use.\n");
		printf ("\ttemplate is the template file to process.\n\n");
		printf ("The output makefile goes to standard output.\n");
        	return 1;
	};
	Table = NULL;
	if (!strcmpi(argv[1], "/MSC") || !strcmpi(argv[1], "-MSC")) {
            MSC = TRUE;
            BC = FALSE;
	} else if (!strcmpi(argv[1], "/BC") || !strcmpi(argv[1], "-BC")) {
            MSC = FALSE;
            BC = TRUE;
	} else {
            fprintf (stderr, "Unknown compiler format: %s\n", argv[1]);
            return 1;
	};

        strcpy (SavedNewLine, "");
        if ((makfile = fopen (argv[2], "r")) == NULL)
        	return 2;
	COMPILER (0, SavedNewLine, "?BEGIN?", makfile);
	while (!feof(makfile)) {
            if (fgets(buffer, 100, makfile) == NULL)
		break;
	    if (!strnicmp(buffer, "?SCCS?", 6)) {
		time (&timer);
	      printf ("%s#\tSCCS Id: @(#)Makefile.%s\t3.4\t%02d/%02d/%02d\n",
                    SavedNewLine,
		    MSC ? "MSC" : BC ? "BC" : "???",
		    localtime(&timer)->tm_year,
		    localtime(&timer)->tm_mon + 1,
		    localtime(&timer)->tm_mday);
		printf ("# Copyright (c) %s, %d.\n",
		    BC ? "Yitzhak Sapir" : "NetHack PC Development Team",
		    localtime(&timer)->tm_year + 1900);
		printf ("# NetHack may be freely distributed.  ");
		printf ("See license for details.\n#\n\n");
		strcpy (SavedNewLine, "");
            } else if (MSC ? sscanf (buffer, "?MSCMACRO:%[^=]=%[^?]?",
			MacroName, MacroVal) :
			BC ? sscanf (buffer, "?BCMACRO:%[^=]=%[^?]?",
			MacroName, MacroVal) : 0)
		MacroList = AddMacro(MacroList, MacroName, MacroVal);
	    else if (!strnicmp(buffer, "?BC?", 4))
                COMPILER(BC, SavedNewLine, "?ENDBC?", makfile);
            else if (!strnicmp(buffer, "?MSC?", 5))
                COMPILER(MSC, SavedNewLine, "?ENDMSC?", makfile);
            else if (!strnicmp(buffer, "?COMMENT?", 9))
                COMPILER(FALSE, SavedNewLine, "?ENDCOMMENT?", makfile);
	    else if (sscanf(buffer, "?LIST:%[^?]?", Listname)
		    == 1) {
		printf ("%s%s\t=", SavedNewLine, Listname);
		Table = AddList (Table, ReadList (makfile, 18), Listname);
		strcpy (SavedNewLine, "");
	    } else if (sscanf(buffer, "?LINKLIST:%[^?]?", Listname)
		    == 1)
		LINKLIST (BC, FindList(Table, Listname), Listname,
		    SavedNewLine);
            else if (buffer[0] != '?')
                SaveNewLine(ApplyMacros(buffer, MacroList), SavedNewLine);
        };
	printf ("%s", SavedNewLine);
        fclose (makfile);
	Table = DelTable(Table);
	MacroList = DelMacroList (MacroList);
	return 0;
}
