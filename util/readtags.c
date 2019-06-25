/* NetHack 3.6	readtags.c	$Date$ $Revision$	          */
/* Copyright (c) Michael Allison, 2018			          */
/* NetHack may be freely redistributed.  See license for details. */

/*
 *  	Read the given ctags file and generate:
 *      Intermediate temp files:
 *              include/sfo_proto.tmp
 *              include/sfi_proto.tmp
 *		src/sfi_data.tmp
 *		src/sfo_data.tmp
 *      Final files:
 *		sfdata.c
 *              sfproto.h
 *
 */

#include "hack.h"
#include "lev.h"
#include "integer.h"
#include "wintype.h"
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#ifdef __GNUC__
#define strncmpi strncasecmp
#endif

/* version information */
#ifdef SHORT_FILENAMES
#include "patchlev.h"
#else
#include "patchlevel.h"
#endif

#define NHTYPE_SIMPLE	1
#define NHTYPE_COMPLEX	2
struct nhdatatypes_t {
	unsigned int dtclass;
	char *dtype;
	size_t dtsize;
};

struct tagstruct {
    int linenum;
    char ptr[128];
    char tag[100];
    char filename[128];
    char searchtext[255];
    char tagtype;
    char parent[100];
    char parenttype[100];
    char arraysize1[100];
    char arraysize2[100];
    struct tagstruct *next;
};

struct needs_array_handling {
    const char *nm;
    const char *parent;
};

#define SFO_DATA c_sfodata
#define SFI_DATA c_sfidata
#define SFDATATMP c_sfdatatmp
#define SFO_PROTO c_sfoproto
#define SFI_PROTO c_sfiproto
#define SFDATA c_sfdata
#define SFPROTO c_sfproto

static char *fgetline(FILE*);
static void quit(void);
static void out_of_memory(void);
static void doline(char *);
static void chain(struct tagstruct *);
static void showthem(void);
static char *stripspecial(char *);
static char *deblank(char *);
static char *deeol(char *);
static void generate_c_files();
static char *findtype(char *, char *);
static boolean FDECL(is_prim, (char *));
static void taglineparse(char *, struct tagstruct *);
static void parseExtensionFields(struct tagstruct *, char *);
static void set_member_array_size(struct tagstruct *);
static char *member_array_dims(struct tagstruct *, char *);
static char *member_array_size(struct tagstruct *, char *);
static void output_types(FILE *);
static char *FDECL(dtmacro,(const char *,int));
static char *FDECL(dtfn,(const char *,int, boolean *));
static char *FDECL(bfsize,(const char *));
static char *FDECL(fieldfix,(char *,char *));

#ifdef VMS
static FILE *vms_fopen(name, mode) const char *name, *mode;
{
    return fopen(name, mode, "mbc=64", "shr=nil");
}
# define fopen(f,m) vms_fopen(f,m)
#endif

#define Fprintf    (void) fprintf
#ifndef __GO32__
#define DEFAULTTAGNAME "../util/nethack.tags"
#else
#define DEFAULTTAGNAME "../util/nhtags.tag"
#endif
#ifndef _MAX_PATH
#define _MAX_PATH  120
#endif

#define TAB '\t'
#define SPACE ' '

struct tagstruct *first;
struct tagstruct zerotag;

static int tagcount;
static const char *infilenm;
static FILE *infile;
static char line[255];
static long lineno;
static char ssdef[BUFSZ];
static char fieldfixbuf[BUFSZ];

#define NHTYPE_SIMPLE    1
#define NHTYPE_COMPLEX   2

struct nhdatatypes_t readtagstypes[] = {
    {NHTYPE_SIMPLE, (char *) "any", sizeof(anything)},
    {NHTYPE_SIMPLE, (char *) "genericptr_t", sizeof(genericptr_t)},
    {NHTYPE_SIMPLE, (char *) "aligntyp", sizeof(aligntyp)},
    {NHTYPE_SIMPLE, (char *) "Bitfield", sizeof(uint8_t)},
    {NHTYPE_SIMPLE, (char *) "boolean", sizeof(boolean)},
    {NHTYPE_SIMPLE, (char *) "char", sizeof(char)},
    {NHTYPE_SIMPLE, (char *) "int",  sizeof(int)},
    {NHTYPE_SIMPLE, (char *) "long", sizeof(long)},
    {NHTYPE_SIMPLE, (char *) "schar", sizeof(schar)},
    {NHTYPE_SIMPLE, (char *) "short", sizeof(short)},
    {NHTYPE_SIMPLE, (char *) "size_t", sizeof(size_t)},
    {NHTYPE_SIMPLE, (char *) "string", 1},
    {NHTYPE_SIMPLE, (char *) "time_t",  sizeof(time_t)},
    {NHTYPE_SIMPLE, (char *) "uchar",  sizeof(uchar)},
    {NHTYPE_SIMPLE, (char *) "unsigned char", sizeof(unsigned char)},
    {NHTYPE_SIMPLE, (char *) "unsigned int", sizeof(unsigned int)},
    {NHTYPE_SIMPLE, (char *) "unsigned long", sizeof(unsigned long)},
    {NHTYPE_SIMPLE, (char *) "unsigned short",  sizeof(unsigned short)},
    {NHTYPE_SIMPLE, (char *) "unsigned", sizeof(unsigned)},
    {NHTYPE_SIMPLE, (char *) "xchar",  sizeof(xchar)},
    {NHTYPE_COMPLEX, (char *) "align", sizeof(struct align)},
/*    {NHTYPE_COMPLEX, (char *) "attack", sizeof(struct attack)}, */ /* permonst affil */
    {NHTYPE_COMPLEX, (char *) "attribs", sizeof(struct attribs)},
    {NHTYPE_COMPLEX, (char *) "bill_x", sizeof(struct bill_x)},
    {NHTYPE_COMPLEX, (char *) "book_info", sizeof(struct book_info)},
    {NHTYPE_COMPLEX, (char *) "branch", sizeof(struct branch)},
    {NHTYPE_COMPLEX, (char *) "bubble", sizeof(struct bubble)},
    {NHTYPE_COMPLEX, (char *) "cemetery", sizeof(struct cemetery)},
/*    {NHTYPE_COMPLEX, (char *) "container", sizeof(struct container)}, */
    {NHTYPE_COMPLEX, (char *) "context_info", sizeof(struct context_info)},
    {NHTYPE_COMPLEX, (char *) "d_flags", sizeof(struct d_flags)},
    {NHTYPE_COMPLEX, (char *) "d_level", sizeof(struct d_level)},
    {NHTYPE_COMPLEX, (char *) "damage", sizeof(struct damage)},
    {NHTYPE_COMPLEX, (char *) "dest_area", sizeof(struct dest_area)},
    {NHTYPE_COMPLEX, (char *) "dgn_topology", sizeof(struct dgn_topology)},
    {NHTYPE_COMPLEX, (char *) "dig_info", sizeof(struct dig_info)},
    {NHTYPE_COMPLEX, (char *) "dungeon", sizeof(struct dungeon)},
    {NHTYPE_COMPLEX, (char *) "edog", sizeof(struct edog)},
    {NHTYPE_COMPLEX, (char *) "egd", sizeof(struct egd)},
    {NHTYPE_COMPLEX, (char *) "emin", sizeof(struct emin)},
    {NHTYPE_COMPLEX, (char *) "engr", sizeof(struct engr)},
    {NHTYPE_COMPLEX, (char *) "epri", sizeof(struct epri)},
    {NHTYPE_COMPLEX, (char *) "eshk", sizeof(struct eshk)},
    {NHTYPE_COMPLEX, (char *) "fakecorridor", sizeof(struct fakecorridor)},
    {NHTYPE_COMPLEX, (char *) "fe", sizeof(struct fe)},
    {NHTYPE_COMPLEX, (char *) "flag", sizeof(struct flag)},
    {NHTYPE_COMPLEX, (char *) "fruit", sizeof(struct fruit)},
    {NHTYPE_COMPLEX, (char *) "kinfo", sizeof(struct kinfo)},
    {NHTYPE_COMPLEX, (char *) "levelflags", sizeof(struct levelflags)},
    {NHTYPE_COMPLEX, (char *) "linfo", sizeof(struct linfo)},
    {NHTYPE_COMPLEX, (char *) "ls_t", sizeof(struct ls_t)},
    {NHTYPE_COMPLEX, (char *) "mapseen_feat", sizeof(struct mapseen_feat)},
    {NHTYPE_COMPLEX, (char *) "mapseen_flags", sizeof(struct mapseen_flags)},
    {NHTYPE_COMPLEX, (char *) "mapseen_rooms", sizeof(struct mapseen_rooms)},
    {NHTYPE_COMPLEX, (char *) "mapseen", sizeof(struct mapseen)},
    {NHTYPE_COMPLEX, (char *) "mextra", sizeof(struct mextra)},
    {NHTYPE_COMPLEX, (char *) "mkroom", sizeof(struct mkroom)},
    {NHTYPE_COMPLEX, (char *) "monst", sizeof(struct monst)},
    {NHTYPE_COMPLEX, (char *) "mvitals", sizeof(struct mvitals)},
    {NHTYPE_COMPLEX, (char *) "nhcoord", sizeof(struct nhcoord)},
    {NHTYPE_COMPLEX, (char *) "nhrect", sizeof(struct nhrect)},
    {NHTYPE_COMPLEX, (char *) "novel_tracking", sizeof(struct novel_tracking)},
    {NHTYPE_COMPLEX, (char *) "obj", sizeof(struct obj)},
    {NHTYPE_COMPLEX, (char *) "objclass", sizeof(struct objclass)},
    {NHTYPE_COMPLEX, (char *) "obj_split", sizeof(struct obj_split)},
    {NHTYPE_COMPLEX, (char *) "oextra", sizeof(struct oextra)},
/*    {NHTYPE_COMPLEX, (char *) "permonst", sizeof(struct permonst)}, */
    {NHTYPE_COMPLEX, (char *) "polearm_info", sizeof(struct polearm_info)},
    {NHTYPE_COMPLEX, (char *) "prop", sizeof(struct prop)},
    {NHTYPE_COMPLEX, (char *) "q_score", sizeof(struct q_score)},
    {NHTYPE_COMPLEX, (char *) "rm", sizeof(struct rm)},
    {NHTYPE_COMPLEX, (char *) "s_level", sizeof(struct s_level)},
    {NHTYPE_COMPLEX, (char *) "savefile_info", sizeof(struct savefile_info)},
    {NHTYPE_COMPLEX, (char *) "skills", sizeof(struct skills)},
    {NHTYPE_COMPLEX, (char *) "spell", sizeof(struct spell)},
    {NHTYPE_COMPLEX, (char *) "stairway", sizeof(struct stairway)},
#ifdef SYSFLAGS
    {NHTYPE_COMPLEX, (char *) "sysflag", sizeof(struct sysflag)},
#endif
    {NHTYPE_COMPLEX, (char *) "takeoff_info", sizeof(struct takeoff_info)},
    {NHTYPE_COMPLEX, (char *) "tin_info", sizeof(struct tin_info)},
    {NHTYPE_COMPLEX, (char *) "trap", sizeof(struct trap)},
    {NHTYPE_COMPLEX, (char *) "tribute_info", sizeof(struct tribute_info)},
    {NHTYPE_COMPLEX, (char *) "u_achieve", sizeof(struct u_achieve)},
    {NHTYPE_COMPLEX, (char *) "u_conduct", sizeof(struct u_conduct)},
    {NHTYPE_COMPLEX, (char *) "u_event", sizeof(struct u_event)},
    {NHTYPE_COMPLEX, (char *) "u_have", sizeof(struct u_have)},
    {NHTYPE_COMPLEX, (char *) "u_realtime", sizeof(struct u_realtime)},
    {NHTYPE_COMPLEX, (char *) "u_roleplay", sizeof(struct u_roleplay)},
    {NHTYPE_COMPLEX, (char *) "version_info", sizeof(struct version_info)},
    {NHTYPE_COMPLEX, (char *) "victual_info", sizeof(struct victual_info)},
    {NHTYPE_COMPLEX, (char *) "vlaunchinfo", sizeof(union vlaunchinfo)},
    {NHTYPE_COMPLEX, (char *) "vptrs", sizeof(union vptrs)},
    {NHTYPE_COMPLEX, (char *) "warntype_info", sizeof(struct warntype_info)},
    {NHTYPE_COMPLEX, (char *) "you", sizeof(struct you)}
};

/*
 * These have arrays of other structs, not just arrays of
 * simple types. We need to put array handling right into
 * the code for these ones.
 */
struct needs_array_handling nah[] = {
    {"fakecorr", (char *) "egd"},
    {"bill", "eshk"},
    {"msrooms", "mapseen"},
    {"mtrack", "monst"},
    {"ualignbase", "you"},
    {"weapon_skills", "you"},
};

/* conditional code tags - eecch */
char *condtag[] = {
#ifdef SYSFLAGS
    "sysflag","altmeta","#ifdef AMIFLUSH", "",
    "sysflag","amiflush","","#endif /*AMIFLUSH*/",
    "sysflag","numcols", "#ifdef AMII_GRAPHICS", "",
    "sysflag","amii_dripens","","",
    "sysflag","amii_curmap","","#endif",
    "sysflag","fast_map", "#ifdef OPT_DISMAP", "#endif",
    "sysflag","asksavedisk","#ifdef MFLOPPY","#endif",
    "sysflag","page_wait", "#ifdef MAC", "#endif",
#endif
    "linfo","where","#ifdef MFLOPPY","",
    "linfo","time","","",
    "linfo","size","","#endif /*MFLOPPY*/",
    "obj","oinvis","#ifdef INVISIBLE_OBJECTS", "#endif",
    (char *)0,(char *)0,(char *)0, (char *)0
};



int main(argc, argv)
int argc;
char *argv[];
{
    tagcount = 0;
    
    if (argc > 1) infilenm = argv[1];
    if (!infilenm || !*infilenm) infilenm = DEFAULTTAGNAME;

    infile = fopen(infilenm,"r");
    if (!infile) {
        printf("%s not found or unavailable\n",infilenm);
        quit();
    }

    while (fgets(line, sizeof line, infile)) {
        ++lineno;
/*        if (lineno == 868) DebugBreak(); */
        doline(line);
    }

    fclose(infile);
    printf("\nRead in %ld lines and stored %d tags.\n", lineno,  tagcount);
#if 0
    showthem(); 
#endif
    generate_c_files();
    exit(EXIT_SUCCESS);
    /*NOTREACHED*/
    return 0;
}

static void doline(line)
char *line;
{
    char buf[255], *tmp1 = (char *)0;
    struct tagstruct *tmptag = malloc(sizeof(struct tagstruct));

    if (!tmptag) {
        out_of_memory();
    }
    *tmptag = zerotag;
    
    if (!line || (line && *line == '!')) {
        free(tmptag);
        return;
    }

    strncpy(buf, deeol(line), 254);
    taglineparse(buf,tmptag);
    chain(tmptag);
    return;
}

static void chain(tag)
struct tagstruct *tag;
{
    static struct tagstruct *prev = (struct tagstruct *)0;
    if (!first) {
        tag->next = (struct tagstruct *)0;
        first = tag;
    } else {
        tag->next = (struct tagstruct *)0;
        if (prev) {
            prev->next = tag;
        } else {
            printf("Error - No previous tag at %s\n", tag->tag);
        }
    }
    prev = tag;
    ++tagcount;
}
static void quit()
{
    exit(EXIT_FAILURE);
}

static void out_of_memory()
{
    printf("maketags: out of memory at line %ld of %s\n",
        lineno, infilenm);
    quit();
}

static char *member_array_dims(struct tagstruct *tmptag, char *buf)
{
    if (buf && tmptag) {
        if (tmptag->arraysize1[0])
            Sprintf(buf, "[%s]", tmptag->arraysize1);
        if (tmptag->arraysize2[0])
            Sprintf(eos(buf), "[%s]", tmptag->arraysize2);
        return buf;
    }
    return "";
}

static char *member_array_size(struct tagstruct *tmptag, char *buf)
{
    if (buf && tmptag) {
        if (tmptag->arraysize1[0])
            strcpy(buf, tmptag->arraysize1);
        if (tmptag->arraysize2[0])
            Sprintf(eos(buf), " * %s", tmptag->arraysize2);
        return buf;
    }
    return "";
}

void set_member_array_size(struct tagstruct *tmptag)
{
    char buf[BUFSZ];
    static char result[49];
    char *arr1 = (char *)0, *arr2 = (char *)0, *tmp;
    int cnt = 0;

    if (!tmptag || (tmptag && !tmptag->searchtext)) return;
    strcpy(buf, tmptag->searchtext);
    
    tmptag->arraysize1[0] = '\0';
    tmptag->arraysize2[0] = '\0';

    /* find left-open square bracket */
    tmp = strchr(buf, '[');
    if (tmp) {
        arr1 = tmp;
        *tmp = '\0';
        --tmp;
        /* backup and make sure the [] are on the right tag */
        while (!(*tmp == SPACE || *tmp == TAB || *tmp ==',' || cnt > 50)) {
            --tmp;
            cnt++;
        }
        if (cnt > 50) return;
        tmp++;
        if (strcmp(tmp, tmptag->tag) == 0) {
            ++arr1;
            tmp = strchr(arr1, ']');
            if (tmp) {
                arr2 = tmp;
                ++arr2;
                *tmp = '\0';
                if (*arr2 == '[') { /* two-dimensional array */
                    ++arr2;
                    tmp = strchr(arr2, ']');
                    if (tmp) *tmp = '\0';
                } else {
                    arr2 = (char *)0;
                }
            }
        } else {
            arr1 = (char *)0;
        }
    }
    if (arr1) (void)strcpy(tmptag->arraysize1, arr1);
    if (arr2) (void)strcpy(tmptag->arraysize2, arr2);
}

static void parseExtensionFields (tmptag, buf)
struct tagstruct *tmptag;
char *buf;
{
    char *p = buf;
    while (p != (char *)0  &&  *p != '\0') {
        while (*p == TAB)
            *p++ = '\0';
        if (*p != '\0') {
            char *colon;
            char *field = p;
            
            p = strchr (p, TAB);
            if (p != (char *)0)
                *p++ = '\0';
            colon = strchr (field, ':');
            if (colon == (char *)0) {
                tmptag->tagtype = *field;
            } else {
                const char *key = field;
                const char *value = colon + 1;
                *colon = '\0';
                if ((strcmp (key, "struct") == 0) ||
                    (strcmp (key, "union") == 0)) {
                    colon = strstr(value,"::");
                    if (colon)
                        value = colon +2;
                    strcpy(tmptag->parenttype, key);
                    strcpy(tmptag->parent, value);
                }
            }
        }
    }
}

void
taglineparse(p,tmptag)
char *p;
struct tagstruct *tmptag;
{
    int fieldsPresent = 0;
    char *filenm = 0, *pattern = 0, *tmp1 = 0;
    int linenumber = 0;
    char *tab = strchr (p, TAB);

    if (tab != NULL) {
        *tab = '\0';
        strcpy(tmptag->tag,p);
        p = tab + 1;
        filenm = p;
        tab = strchr (p, TAB);
        if (tab != NULL) {
            *tab = '\0';
            p = tab + 1;
            if (*p == '/'  ||  *p == '?') {
                /* parse pattern */
                int delimiter = *(unsigned char *) p;
                linenumber = 0;
                pattern = p;
                do {
                    p = strchr (p + 1, delimiter);
                } while (p != (char *)0  &&  *(p - 1) == '\\');

                if (p == (char *)0) {
                    /* invalid pattern */
                } else
                    ++p;
            } else if (isdigit ((int) *(unsigned char *) p)) {
                /* parse line number */
                pattern = p;
                linenumber = atol(p);
                while (isdigit((int) *(unsigned char *) p))
                    ++p;
            } else {
                /* invalid pattern */
            }
            fieldsPresent = (strncmp (p, ";\"", 2) == 0);
            *p = '\0';

            if (fieldsPresent)
                parseExtensionFields (tmptag, p + 2);
        }
    }

    strcpy(tmptag->searchtext, pattern);
    tmptag->linenum = linenumber;

    /* add the array dimensions */
    set_member_array_size(tmptag);

    /* determine if this is a pointer and mark it as such */
    if (tmptag->searchtext &&
        (tmptag->tagtype == 'm' || tmptag->tagtype == 's')) {
        char ptrbuf[BUFSZ], searchbuf[BUFSZ];

        (void) strcpy(ptrbuf, tmptag->searchtext);
        Sprintf(searchbuf,"*%s", tmptag->tag);
        tmp1 = strstr(ptrbuf, searchbuf);
        if (!tmp1) {
            Sprintf(searchbuf,"* %s", tmptag->tag);
            tmp1 = strstr(ptrbuf, searchbuf);
        }
        if (tmp1) {
            while ((tmp1 > ptrbuf) && (*tmp1 != SPACE) &&
                    (*tmp1 != TAB) && (*tmp1 != ','))
                tmp1--;
            tmp1++;
            while (*tmp1 == '*')
                tmp1++;
            *tmp1 = '\0';
            /* now find the first * before this in case multiple things
               are declared on this line */
            tmp1 = strchr(ptrbuf+2, '*');
            if (tmp1) {
                tmp1++;
                *tmp1 = '\0';
                tmp1 = ptrbuf + 2;
                while (*tmp1 == SPACE || *tmp1 == TAB || *tmp1 == ',')
                    ++tmp1;
                (void)strcpy(tmptag->ptr, tmp1);
            }
        }
    }
}

/* eos() is copied from hacklib.c */
/* return the end of a string (pointing at '\0') */
char *
eos(s)            
char *s;
{
    while (*s) s++;    /* s += strlen(s); */
    return s;
}

static char stripbuf[255];

static char *stripspecial(char *st)
{
    char *out = stripbuf;
    *out = '\0';
    if (!st) return st;
    while(*st) {
        if (*st >= SPACE)
            *out++ = *st++;
        else
            *st++;
    }
    *out = '\0';
    return stripbuf;
}

static char *deblank(char *st)
{
    char *out = stripbuf;
    *out = '\0';
    if (!st) return st;
    while(*st) {
        if (*st == SPACE) { 
            *out++ = '_';
            st++;
        } else
            *out++ = *st++;
    }
    *out = '\0';
    return stripbuf;
}

static char *deeol(char *st)
{
    char *out = stripbuf;
    *out = '\0';
    if (!st) return st;
    while(*st) {
        if ((*st == '\r') || (*st == '\n')) { 
            st++;
        } else
            *out++ = *st++;
    }
    *out = '\0';
    return stripbuf;
}

#if 0
static void showthem()
{
    char buf[BUFSZ], *tmp;
    struct tagstruct *t = first;
    while(t) {
        printf("%-28s %c, %-16s %-10s", t->tag, t->tagtype,
            t->parent, t->parenttype);
#if 0
            t->parent[0] ? t->searchtext : "");
#endif
        buf[0] = '\0';
        tmp = member_array_dims(t,buf);        
        if (tmp) printf("%s", tmp);
        printf("%s","\n");
        t = t->next;
    }
}
#endif

static boolean
is_prim(sdt)
char *sdt;
{
    int k = 0;
    if (sdt) {
        /* special case where we don't match entire thing */
        if (!strncmpi(sdt, "Bitfield",8))
            return TRUE;
        for (k = 0; k < SIZE(readtagstypes); ++k) {
            if (!strcmpi(readtagstypes[k].dtype, sdt)) {
            if (readtagstypes[k].dtclass == NHTYPE_SIMPLE)
                return TRUE;
            else
                return FALSE;
            }
        }
    }
    return FALSE;
}

char *
findtype(st, tag)
char *st;
char *tag;
{
    static char ftbuf[512];
    static char prevbuf[512];
    char *tmp1, *tmp2, *tmp3, *tmp4, *r;
    if (!st) return (char *)0;
    if (st[0] == '/' && st[1] == '^') {
        tmp2 = tmp3 = tmp4 = (char *)0;
        tmp1 = &st[3];
        while (*tmp1) {
            if (isspace(*tmp1))
                ; /* skip it */
            else
                break;
            ++tmp1;
        }
        if (!strncmp(tmp1, tag, strlen(tag))) {
            if(strlen(tag) == 1) {
                char *sc = tmp1;
                /* Kludge: single char match is too iffy,
                   check to make sure its a complete
                   token that we're comparing to. */
                ++sc;
                if (!(*sc == '_' || (*sc > 'a' && *sc < 'z') ||
                     (*sc > 'A' && *sc < 'Z') || (*sc > '0' && *sc < '9')))
                    return (char *)0;
            } else {
                return (char *)0;
            }
        }
        if (*tmp1) {
            if (!strncmp(tmp1, "Bitfield", 8)) {
                strcpy(ftbuf, tmp1);
                tmp1 = ftbuf;
                tmp3 = strchr(tmp1, ')');
                if (tmp3) {
                    tmp3++;
                    *tmp3 = '\0';
                    return ftbuf;
                }
                return (char *)0;
            }
        }
        if (*tmp1) {
            int prevchar = 0;
            strcpy(ftbuf, tmp1);
            tmp1 = ftbuf;
            /* find space separating first word with second */
            while (!isspace(*tmp1)) {
                prevchar = *tmp1;
                ++tmp1;
            }
            
            /* some oddball cases */
            if (prevchar == ',' || prevchar == ';') {
                tmp3 = strchr(ftbuf, ',');
                tmp2 = strstr(ftbuf, tag);
                return prevbuf;
            } else {
            /* a comma means that more than one thing declared on line */
                 tmp3 = strchr(tmp1, ',');
                tmp2 = strstr(tmp1, tag);
            }
            /* make sure we're matching a complete token */
            if (tmp2) {
                tmp4 = tmp2 + strlen(tag);
                if ((*tmp4 == '_') || (*tmp4 >= 'a' && *tmp4 <= 'z') ||
                 (*tmp4 >= 'A' && *tmp4 <= 'Z') || (*tmp4 >= '0' && *tmp4 <= '9'))
                /* jump to next occurence then */
                tmp2 = strstr(tmp4, tag);
            }
               /* tag w/o comma OR tag found w comma and tag before comma */
            if ((tmp2 && !tmp3) || ((tmp2 && tmp3) && (tmp2 < tmp3))) {
                *tmp2 = '\0';
                --tmp2;
                while (isspace(*tmp2))
                    --tmp2;
                tmp2++;
                *tmp2 = '\0';
            }
            /* comma and no tag OR tag w comma and comma before tag */
            else if ((tmp3 && !tmp2) || ((tmp2 && tmp3) && (tmp3 < tmp2))) {
                --tmp3;
                if (isspace(*tmp3)) {
                    while (isspace(*tmp3))
                        --tmp3;
                }
                while (!isspace(*tmp3) && (*tmp3 != '*')) 
                    --tmp3;
                while (isspace(*tmp3))
                    --tmp3;
                tmp3++;
                *tmp3 = '\0';
            }
            /* comma or semicolon immediately following tag */
            else {
                volatile int y;
                y = 1;
            }
            if (strncmpi(ftbuf, "struct ", 7) == 0)
                r = ftbuf + 7;
            else if (strncmpi(ftbuf, "union ", 6) == 0)
                r = ftbuf + 6;
            /* a couple of kludges follow unfortunately */
            else if (strncmpi(ftbuf, "coord", 5) == 0)
                r = "nhcoord";
            else if (strncmpi(ftbuf, "anything", 8) == 0)
                r = "any";
            else if (strncmpi(ftbuf, "const char", 10) == 0)
                r = "char";
            else
                r = ftbuf;
            strcpy(prevbuf, r);
            return r;
        }
    }
    prevbuf[0] = '\0';
    return (char *)0;
}

boolean
listed(t)
struct tagstruct *t;
{
    int k;

    if ((strncmpi(t->tag, "Bitfield", 8) == 0) ||
                    (strcmpi(t->tag, "string") == 0))
        return TRUE;
    for (k = 0; k < SIZE(readtagstypes); ++k) {
        /*  This needs to be case-sensitive to avoid generating collision
         *  between 'align' and 'Align'.
         */
        if (strcmp(readtagstypes[k].dtype, t->tag) == 0)
            return TRUE;
    }
    return FALSE;
}


char *preamble[] = {
    "/* Copyright (c) NetHack Development Team 2018.                   */\n",
    "/* NetHack may be freely redistributed.  See license for details. */\n\n",
    "/* THIS IS AN AUTOGENERATED FILE. DO NOT EDIT THIS FILE!          */\n\n",
    "#include \"hack.h\"\n",
    "#include \"artifact.h\"\n",
    "#include \"func_tab.h\"\n",
    "#include \"lev.h\"\n",
        "#include \"integer.h\"\n",
        "#include \"wintype.h\"\n",
    (char *)0
};

static void output_types(fp1)
FILE *fp1;
{
    int k, cnt, hcnt = 1;
    struct tagstruct *t = first;

    Fprintf(fp1, "%s",
        "struct nhdatatypes_t nhdatatypes[] = {\n");

    for (k = 0; k < SIZE(readtagstypes); ++k) {
        if (readtagstypes[k].dtclass == NHTYPE_SIMPLE) {
            Fprintf(fp1,"\t{NHTYPE_SIMPLE,\"%s\", sizeof(%s)},\n",
                readtagstypes[k].dtype,
                (strncmpi(readtagstypes[k].dtype, "Bitfield", 8) == 0) ?
                "uint8_t" :
                (strcmpi(readtagstypes[k].dtype, "string") == 0) ?
                "uchar" :
                (strcmpi(readtagstypes[k].dtype, "any") == 0) ?
                                "anything" : readtagstypes[k].dtype);
/*                dtmacro(readtagstypes[k].dtype,0)); */
#if 0
            Fprintf(fp2, "#define %s\t%s%d\n", dtmacro(readtagstypes[k].dtype,1),
                    (strlen(readtagstypes[k].dtype) > 12) ? "" :
                (strlen(readtagstypes[k].dtype) < 5) ? "\t\t" :
                "\t", hcnt++);
#endif
        }
    }
    cnt = 0;
    while(t) {
        if (listed(t) && ((t->tagtype == 's') || (t->tagtype == 'u'))) {
            if (!strcmp(t->tag, "any")) {
                t = t->next;
                continue;
            }
            if (cnt > 0)
                Fprintf(fp1, "%s", ",\n");
            Fprintf(fp1, "\t{NHTYPE_COMPLEX,\"%s\", sizeof(%s %s)}",
                    t->tag,
                    (t->tagtype == 's') ? "struct" : "union", t->tag);
            cnt += 1;
        }
        t = t->next;
    }
    Fprintf(fp1, "%s", "\n};\n\n");
    Fprintf(fp1, "int nhdatatypes_size()\n{\n\treturn SIZE(nhdatatypes);\n}\n\n");
}

static void generate_c_files()
{
    struct tagstruct *t = first;
#ifdef KR1ED
    long clocktim = 0;
#else
    time_t clocktim = 0;
#endif
    char *c, cbuf[60], sfparent[BUFSZ], *substruct, *line;
    FILE *SFO_DATA, *SFI_DATA, *SFDATATMP, *SFO_PROTO, *SFI_PROTO,
         *SFDATA, *SFPROTO;
    int k = 0, j, opening, closetag = 0;
    char *iftxt = "} else ";
    char *ft, *pt, *layout, *last_ft = (char *)0;
    int okeydokey, x, a;
    boolean did_i;

    SFDATA = fopen("../src/sfdata.c", "w");
    if (!SFDATA) return;

    SFPROTO = fopen("../include/sfproto.h", "w");
    if (!SFPROTO) return;

    SFO_DATA = fopen("../src/sfo_data.tmp", "w");
    if (!SFO_DATA) return;

    SFO_PROTO = fopen("../include/sfo_proto.tmp", "w");
    if (!SFO_PROTO) return;

    SFI_PROTO = fopen("../include/sfi_proto.tmp", "w");
    if (!SFI_PROTO) return;

    SFI_DATA = fopen("../src/sfi_data.tmp", "w");
    if (!SFI_DATA) return;

    SFDATATMP = fopen("../src/sfdata.tmp", "w");
    if (!SFDATATMP) return;

    (void) time(&clocktim);
    Strcpy(cbuf, ctime(&clocktim));

    for (c = cbuf; *c; c++)
        if (*c == '\n')
            break;
    *c = '\0';    /* strip off the '\n' */

    /* begin sfproto.h */
    Fprintf(SFPROTO,"/* NetHack %d.%d sfproto.h\t%s */\n", 
               VERSION_MAJOR, VERSION_MINOR, cbuf);
    for (j = 0; j < 3; ++j)
        Fprintf(SFPROTO, "%s", preamble[j]);
    Fprintf(SFPROTO, "#ifndef SFPROTO_H\n#define SFPROTO_H\n\n");
    Fprintf(SFPROTO, "#include \"hack.h\"\n#include \"integer.h\"\n#include \"wintype.h\"\n\n"
                          "#define E extern\n\n");

    /* sfbase.c function prototypes */
    Fprintf(SFPROTO,"%s\n", "E int NDECL(critical_members_count);");
    Fprintf(SFPROTO,"/* sfbase.c output functions */\n");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfo_addinfo, (NHFILE *, const char *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfo_aligntyp, (NHFILE *, aligntyp *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfo_any, (NHFILE *, anything *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfo_bitfield, (NHFILE *, uint8_t *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfo_boolean, (NHFILE *, boolean *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfo_char, (NHFILE *, const char *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfo_genericptr, (NHFILE *, genericptr_t *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfo_int, (NHFILE *, int *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfo_long, (NHFILE *, long *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfo_schar, (NHFILE *, schar *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfo_short, (NHFILE *, short *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfo_size_t, (NHFILE *, size_t *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfo_time_t, (NHFILE *, time_t *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfo_uchar, (NHFILE *, uchar *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfo_unsigned, (NHFILE *, unsigned *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfo_uchar, (NHFILE *, unsigned char *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfo_uint, (NHFILE *, unsigned int *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfo_ulong, (NHFILE *, unsigned long *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfo_ushort, (NHFILE *, unsigned short *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfo_xchar, (NHFILE *, xchar *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfo_str, (NHFILE *, const char *, const char *, const char *, int));");
    Fprintf(SFPROTO,"/* sfbase.c input functions */\n");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfi_addinfo, (NHFILE *, const char *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfi_aligntyp, (NHFILE *, aligntyp *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfi_any, (NHFILE *, anything *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfi_bitfield, (NHFILE *, uint8_t *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfi_boolean, (NHFILE *, boolean *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfi_char, (NHFILE *, const char *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfi_genericptr, (NHFILE *, genericptr_t *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfi_int, (NHFILE *, int *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfi_long, (NHFILE *, long *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfi_schar, (NHFILE *, schar *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfi_short, (NHFILE *, short *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfi_size_t, (NHFILE *, size_t *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfi_time_t, (NHFILE *, time_t *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfi_uchar, (NHFILE *, uchar *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfi_unsigned, (NHFILE *, unsigned *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfi_uchar, (NHFILE *, unsigned char *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfi_uint, (NHFILE *, unsigned int *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfi_ulong, (NHFILE *, unsigned long *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfi_ushort, (NHFILE *, unsigned short *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfi_xchar, (NHFILE *, xchar *, const char *, const char *, int));");
    Fprintf(SFPROTO,"%s\n", "E void FDECL(sfi_str, (NHFILE *, const char *, const char *, const char *, int));");
    Fprintf(SFO_PROTO, "/* generated output functions */\n");
    Fprintf(SFI_PROTO, "/* generated input functions */\n");
    
    /* begin sfdata.c */
    Fprintf(SFDATA,"/* NetHack %d.%d sfdata.c\t$Date$ $Revision$ */\n",
            VERSION_MAJOR, VERSION_MINOR);
    for (j = 0; preamble[j]; ++j)
        Fprintf(SFDATA, "%s", preamble[j]);
    Fprintf(SFDATA, "#include \"sfproto.h\"\n\n");
    Fprintf(SFDATA, "#define BUILD_DATE \"%s\"\n", cbuf);
    Fprintf(SFDATA, "#define BUILD_TIME (%ldL)\n\n", (long) clocktim);
    Fprintf(SFDATA, "#define NHTYPE_SIMPLE    1\n");
    Fprintf(SFDATA, "#define NHTYPE_COMPLEX   2\n");
    Fprintf(SFDATA, "struct nhdatatypes_t {\n");
    Fprintf(SFDATA, "    unsigned int dtclass;\n");
    Fprintf(SFDATA, "    char *dtype;\n");
    Fprintf(SFDATA, "    size_t dtsize;\n};\n\n");
    Fprintf(SFDATA,"static uint8_t bitfield = 0;\n");

    output_types(SFDATATMP);
    Fprintf(SFDATATMP,"char *critical_members[] = {\n");

    k = 0;
    opening = 1;
    did_i = FALSE;
    while(k < SIZE(readtagstypes)) {
      boolean insert_const = FALSE;

      if (readtagstypes[k].dtclass == NHTYPE_COMPLEX) {
        
          if (!strncmpi(readtagstypes[k].dtype,"Bitfield",8)) {            
              Fprintf(SFO_DATA,
                    "\nvoid\nsfo_bitfield(nhfp, d_bitfield, myparent, myname, cnt)\n"
                    "NHFILE *nhfp;\n"
                    "uint8_t *d_bitfield;\n"
                    "const char *myparent;\n"
                    "const char *myname;\n"
                    "int cnt;\n"
                    "{\n");

              Fprintf(SFI_DATA,
                    "\nvoid\nsfi_bitfield(nhfp, d_bitfield, myparent, myname, cnt)\n"
                    "NHFILE *nhfp;\n"
                    "uint8_t *d_bitfield;\n"
                    "const char *myparent;\n"
                    "const char *myname;\n"
                    "int cnt;\n"
                    "{\n");
        }

#if 0
        if (!strncmpi(readtagstypes[k].dtype,"version_info",8))
            insert_const = TRUE;
#endif

        pt = layout = (char *)0;
        t = first;
        while(t) {
            if (t->tagtype == 'u' && !strcmp(t->tag, readtagstypes[k].dtype)) {
                pt = "union";
                break;
            }
            t = t->next;
        }

        if (!pt) {
            pt = "struct";
        }
        Fprintf(SFO_PROTO,
                "E void FDECL(sfo_%s, (NHFILE *, %s%s %s *, const char *, const char *, int));\n",
                readtagstypes[k].dtype,
                insert_const ? "const " : "",
                pt, readtagstypes[k].dtype);

        Fprintf(SFO_DATA,
                "\nvoid\nsfo_%s(nhfp, d_%s, myparent, myname, cnt)\n"
                "NHFILE *nhfp;\n"
                "%s%s %s *d_%s;\n"
                "const char *myparent;\n"
                "const char *myname;\n"
                "int cnt;\n"
                "{\n",
                readtagstypes[k].dtype,
                deblank(readtagstypes[k].dtype),
                insert_const ? "const " : "",
                pt, readtagstypes[k].dtype,
                deblank(readtagstypes[k].dtype));
                        
        Fprintf(SFO_DATA,
                "    const char *parent = \"%s\";\n",
                readtagstypes[k].dtype);

        Fprintf(SFI_PROTO,
                "E void FDECL(sfi_%s, (NHFILE *, %s%s %s *, const char *, const char *, int));\n",
                readtagstypes[k].dtype,
                insert_const ? "const " : "",
                pt, readtagstypes[k].dtype);

        Fprintf(SFI_DATA,
                "\nvoid\nsfi_%s(nhfp, d_%s, myparent, myname, cnt)\n"
                "NHFILE *nhfp;\n"
                "%s%s %s *d_%s;\n"
                "const char *myparent;\n"
                "const char *myname;\n"
                "int cnt;\n"
                "{\n",
                readtagstypes[k].dtype,
                deblank(readtagstypes[k].dtype),
                insert_const ? "const " : "",
                pt, readtagstypes[k].dtype,
                deblank(readtagstypes[k].dtype));                    

        Fprintf(SFI_DATA,
                "    const char *parent = \"%s\";\n",
                readtagstypes[k].dtype);

        Sprintf(sfparent, "%s %s", pt, readtagstypes[k].dtype);

        for (a = 0; a < SIZE(nah); ++a) {
            if (!strcmp(nah[a].parent, readtagstypes[k].dtype)) {
                if (!did_i) {
                    Fprintf(SFO_DATA, "    int i;\n");
                    Fprintf(SFI_DATA, "    int i;\n");
                    did_i = TRUE;
                }
            }
        }

        Fprintf(SFO_DATA, "\n");
        Fprintf(SFI_DATA, "\n");

        Fprintf(SFO_DATA, "    nhUse(myname);\n    nhUse(cnt);\n");
        Fprintf(SFO_DATA,
                "    if (nhfp->addinfo)\n"
                "        sfo_addinfo(nhfp, myparent, \"start\", \"%s\", 1);\n",
                readtagstypes[k].dtype);

        Fprintf(SFI_DATA, "    nhUse(myname);\n    nhUse(cnt);\n");
        Fprintf(SFI_DATA,
                "    if (nhfp->addinfo)\n"
                "        sfi_addinfo(nhfp, myparent, \"start\", \"%s\", 1);\n",
                readtagstypes[k].dtype);

        Fprintf(SFO_DATA, "\n");
        Fprintf(SFI_DATA, "\n");

        /********************************************************
         *  cycle through all the tags and find every tag with  *
         *  a parent matching readtagstypes[k].dtype            *
         ********************************************************/

        t = first;

        while(t) { 
            x = 0;
            okeydokey = 0;

            if (t->tagtype == 's')  {
                char *ss = strstr(t->searchtext,"{$/");

                if (ss) {
                    strcpy(ssdef, t->tag);
                }
                t = t->next;
                continue;
            }

            /************insert opening conditional if needed ********/
            while(condtag[x]) {
                if (!strcmp(condtag[x],readtagstypes[k].dtype) &&
                    !strcmp(condtag[x+1],t->tag)) {
                    okeydokey = 1;
                    break;
                }
                x = x + 4;
            }

            /* some structs are entirely defined within another struct declaration.
             * Legal, but unfortunate for us here.
             */
            substruct = strstr(t->parent, "::");
            if (substruct) {
                substruct += 2;
            }

            if ((strcmp(readtagstypes[k].dtype, t->parent) == 0) ||
                 (substruct && strcmp(readtagstypes[k].dtype, substruct) == 0)) {
                ft = (char *)0;
                if (t->ptr[0])
                    ft = &t->ptr[0];
                else
                    ft = findtype(t->searchtext, t->tag);
                if (ft) {
                    last_ft = ft;
                    if (okeydokey && condtag[x+2] && strlen(condtag[x+2]) > 0) {
                        Fprintf(SFO_DATA,"%s\n", condtag[x+2]);
                        Fprintf(SFI_DATA,"%s\n", condtag[x+2]);
                        Fprintf(SFDATATMP,"%s\n", condtag[x+2]);
                    }
                } else {
                    /* use the last found one as last resort then */
                    ft = last_ft;
                }

                /*****************  Bitfield *******************/
                if (!strncmpi(ft, "Bitfield", 8)) {
                    char lbuf[BUFSZ];
                    int j, z;

                    Sprintf(lbuf, 
                            "    "
                            "bitfield = d_%s->%s;",
                            readtagstypes[k].dtype, t->tag);
                    z = (int) strlen(lbuf);
                    for (j = 0; j < (65 - z); ++j)
                        Strcat(lbuf, " ");
                    Sprintf(eos(lbuf), "/* (%s) */\n", ft);
                    Fprintf(SFO_DATA, "%s", lbuf);
                    Fprintf(SFO_DATA,
                            "    "
                            "sfo_bitfield(nhfp, &bitfield, parent, \"%s\", %s);\n",
                            t->tag, bfsize(ft));       
#if 0
                    Fprintf(SFI_DATA,
                            "    "
                            "bitfield = 0;\n");
#else
                    Fprintf(SFI_DATA,
                            "    "
                            "bitfield = d_%s->%s;       /* set it to current value for testing */\n",
                            readtagstypes[k].dtype, t->tag);
#endif
                    Fprintf(SFI_DATA,
                            "    "
                            "sfi_bitfield(nhfp, &bitfield, parent, \"%s\", %s);\n",
                            t->tag, bfsize(ft));

                    Fprintf(SFI_DATA,
                            "    "
                            "d_%s->%s = bitfield;\n\n",
                            readtagstypes[k].dtype, t->tag);
                    Fprintf(SFDATATMP,
                            "\t\"%s:%s:%s\",\n",
                            sfparent, t->tag, ft);
                } else {
                   /**************** not a bitfield ****************/
                    char arrbuf[BUFSZ];
                    char lbuf[BUFSZ];
                    char fnbuf[BUFSZ];
                    char altbuf[BUFSZ];
                    boolean isptr = FALSE, kludge_sbrooms = FALSE;
                    boolean insert_loop = FALSE;
                    int j, z;

                    /*************** kludge for sbrooms *************/
                    if (!strcmp(t->tag, "sbrooms")) {
                        kludge_sbrooms = TRUE;
                        (void) strcpy(t->arraysize1,"MAX_SUBROOMS");
                    }

                    if (t->arraysize2[0]) {
                        Sprintf(arrbuf, "(%s * %s)",
                                t->arraysize1, t->arraysize2);
                        isptr = TRUE; /* suppress the & in function args */
                    } else if (t->arraysize1[0]) {
                        Sprintf(arrbuf, "%s", t->arraysize1);
                        isptr = TRUE; /* suppress the & in function args */
                    } else {
                        Strcpy(arrbuf, "1");
                    }
                    Strcpy(fnbuf, dtfn(ft,0,&isptr));
                    /*
                     * determine if this is one of the special cases
                     * where there's an array of structs instead of
                     * an array of simple types. We need to insert
                     * a for loop in those cases.
                     */
                    for (a = 0; a < SIZE(nah); ++a) {
                        if (!strcmp(nah[a].parent, t->parent))
                            if (!strcmp(nah[a].nm, t->tag))
                                insert_loop = TRUE;
                    }

                    if (isptr && !strcmp(fnbuf, readtagstypes[k].dtype)) {
                        Strcpy(altbuf, "genericptr");
                    } else if (isptr &&
                              (!strcmp(t->ptr, "struct permonst *")  ||
                               !strcmp(t->ptr, "struct monst *")     ||
                               !strcmp(t->ptr, "struct obj *")       ||
                               !strcmp(t->ptr, "struct cemetery *")  ||
                               !strcmp(t->ptr, "struct container *") ||
                               !strcmp(t->ptr, "struct mextra *")    ||
                               !strcmp(t->ptr, "struct oextra *")    ||
                               !strcmp(t->ptr, "struct s_level *")   ||
                               !strcmp(t->ptr, "struct bill_x *")    ||
                               !strcmp(t->ptr, "struct trap *"))) {
                        Strcpy(altbuf, "genericptr");
                    } else if (isptr &&
                              (!strcmp(t->parent, "engr") &&
                               !strcmp(t->tag, "engr_txt"))) {
                        Strcpy(altbuf, "genericptr");
                    } else if (isptr && !strcmp(t->tag, "oc_uname")) {
                        Strcpy(altbuf, "genericptr");
                    } else {
                        Strcpy(altbuf, fnbuf);
                    }

                    if (insert_loop) {
                        Fprintf(SFO_DATA,
                                "    for (i = 0; i < %s; ++i)\n",
                                arrbuf);
                        Fprintf(SFI_DATA,
                                "    for (i = 0; i < %s; ++i)\n",
                                arrbuf);
                        arrbuf[0] = '1';
                        arrbuf[1] = '\0';
                    }

                    Sprintf(lbuf,
                        "    "
                        "%ssfo_%s(nhfp, %s%sd_%s->%s%s, parent, \"%s\", %s);",
                        insert_loop ? "    " : "",
                        altbuf,
                        (isptr && !strcmp(altbuf, "genericptr")) ? "(genericptr_t) " : "",
                        (isptr && !insert_loop && !kludge_sbrooms
                                    && strcmp(altbuf, "genericptr")) ? "" : "&",
                        readtagstypes[k].dtype,
                        t->tag,
                        kludge_sbrooms ? "[0]" : insert_loop ? "[i]" : "",
                        t->tag,
                        arrbuf);
                    /* align comments */
                    z = (int) strlen(lbuf);
                    for (j = 0; j < (65 - z); ++j)
                        Strcat(lbuf, " ");
                    Sprintf(eos(lbuf), "/* (%s) */\n", ft);
                    Fprintf(SFO_DATA, "%s", lbuf);

                    Sprintf(lbuf,
                        "    "
                        "%ssfi_%s(nhfp, %s%sd_%s->%s%s, parent, \"%s\", %s);\n",
                        insert_loop ? "    " : "",
                        altbuf,
                        (isptr && !strcmp(altbuf, "genericptr")) ? "(genericptr_t) " : "",
                        (isptr && !insert_loop && !kludge_sbrooms
                                    && strcmp(altbuf, "genericptr")) ? "" : "&",
                        readtagstypes[k].dtype,
                        t->tag,
                        kludge_sbrooms ? "[0]" : insert_loop ? "[i]" : "",
                        t->tag,
                        arrbuf);
                    Fprintf(SFI_DATA, "%s", lbuf);
                    Fprintf(SFDATATMP,
                        "\t\"%s:%s:%s\",\n",
                        sfparent, t->tag,fieldfix(ft,ssdef));
                    kludge_sbrooms = FALSE;
                }

                /************insert closing conditional if needed ********/
                if (okeydokey && condtag[x+3] && strlen(condtag[x+3]) > 0) {
                    Fprintf(SFO_DATA,"%s\n", condtag[x+3]);
                    Fprintf(SFI_DATA,"%s\n", condtag[x+3]);
                    Fprintf(SFDATATMP,"%s\n", condtag[x+3]);
                }
            }
            t = t->next;
        }

        Fprintf(SFO_DATA, "\n");
        Fprintf(SFI_DATA, "\n");

        Fprintf(SFO_DATA,
                "    if (nhfp->addinfo)\n"
                "        sfo_addinfo(nhfp, myparent, \"end\", \"%s\", 1);\n",
                readtagstypes[k].dtype);

        Fprintf(SFI_DATA,
                "    if (nhfp->addinfo)\n"
                "        sfi_addinfo(nhfp, myparent, \"end\", \"%s\", 1);\n",
                readtagstypes[k].dtype);

        Fprintf(SFO_DATA, "}\n");
        Fprintf(SFI_DATA, "}\n");
        opening = 0;
      }
      ++k;
      did_i = FALSE;
    }

    Fprintf(SFDATATMP,"};\n\n");
    Fprintf(SFDATATMP, "int critical_members_count()\n{\n\treturn SIZE(critical_members);\n}\n\n");

    fclose(SFO_DATA);
    fclose(SFI_DATA);
    fclose(SFO_PROTO);
    fclose(SFI_PROTO);
    fclose(SFDATATMP);

    /* Consolidate SFO_* and SFI_* into single files */

    SFO_DATA = fopen("../src/sfo_data.tmp", "r");
    if (!SFO_DATA) return;
    while ((line = fgetline(SFO_DATA)) != 0) {
        (void) fputs(line, SFDATA);
        free(line);
    }
    (void) fclose(SFO_DATA);
    (void) remove("../src/sfo_data.tmp");   

    SFI_DATA = fopen("../src/sfi_data.tmp", "r");
    if (!SFI_DATA) return;
    while ((line = fgetline(SFI_DATA)) != 0) {
        (void) fputs(line, SFDATA);
        free(line);
    }
    (void) fclose(SFI_DATA);
    (void) remove("../src/sfi_data.tmp");   

    SFO_PROTO = fopen("../include/sfo_proto.tmp", "r");
    if (!SFO_PROTO) return;
    while ((line = fgetline(SFO_PROTO)) != 0) {
        (void) fputs(line, SFPROTO);
        free(line);
    }
    (void) fclose(SFO_PROTO);
    (void) remove("../include/sfo_proto.tmp");   

    SFI_PROTO = fopen("../include/sfi_proto.tmp", "r");
    if (!SFI_PROTO) return;
    while ((line = fgetline(SFI_PROTO)) != 0) {
        (void) fputs(line, SFPROTO);
        free(line);
    }
    (void) fclose(SFI_PROTO);
    (void) remove("../include/sfi_proto.tmp");   

    SFDATATMP = fopen("../src/sfdata.tmp", "r");
    if (!SFDATATMP) return;
    while ((line = fgetline(SFDATATMP)) != 0) {
        (void) fputs(line, SFDATA);
        free(line);
    }
    (void) fclose(SFDATATMP);
    (void) remove("../src/sfdata.tmp");   
    
    Fprintf(SFDATA, "/*sfdata.c*/\n");
    Fprintf(SFPROTO,"#endif /* SFPROTO_H */\n");
    (void) fclose(SFDATA);
    (void) fclose(SFPROTO);
}

static char *
dtmacro(str,n)
const char *str;
int n;     /* 1 = supress appending |SF_PTRMASK */
{
    static char buf[128], buf2[128];
    char *nam, *c;
    int ispointer = 0;

    if (!str)
        return (char *)0;
    (void)strncpy(buf, str, 127);

    c = buf;
    while (*c)
        c++;    /* eos */

    c--;
    if (*c == '*') {
        ispointer = 1;
        *c = '\0';
        c--;
    }
    while(isspace(*c)) {
        c--;
    }
    *(c+1) = '\0';
    c = buf;

    if (strncmpi(c, "Bitfield", 8) == 0) {
        *(c+8) = '\0';
    } else if (strcmpi(c, "genericptr_t") == 0) {
        ispointer = 1;
    } else if (strncmpi(c, "const ", 6) == 0) {
        c = buf + 6;
    } else if ((strncmpi(c, "struct ", 7) == 0) ||
           (strncmpi(c, "struct\t", 7) == 0)) {
        c = buf + 7;
    } else if (strncmpi(c, "union ", 6) == 0) {
        c = buf + 6;
    }

    /* end of substruct within struct definition */
    if (strcmp(buf,"}") == 0 && strlen(ssdef) > 0) {
        strcpy(buf,ssdef);
        c = buf;
    }

    for (nam = c; *c; c++) {
        if (*c >= 'a' && *c <= 'z')
            *c -= (char)('a' - 'A');
        else if (*c < 'A' || *c > 'Z')
            *c = '_';
    }
    (void)sprintf(buf2, "SF_%s%s", nam,
            (ispointer && (n == 0)) ? " | SF_PTRMASK" : "");
    return buf2;
}

static char *
dtfn(str,n, isptr)
const char *str;
int n;     /* 1 = supress appending |SF_PTRMASK */
boolean *isptr;
{
    static char buf[128], buf2[128];
    char *nam, *c;
    int ispointer = 0;

    if (!str)
        return (char *)0;
    (void)strncpy(buf, str, 127);

    c = buf;
    while (*c) c++;    /* eos */

    c--;
    if (*c == '*') {
        ispointer = 1;
        *c = '\0';
        c--;
    }
    while(isspace(*c)) {
        c--;
    }
    *(c+1) = '\0';
    c = buf;

    if (strncmpi(c, "Bitfield", 8) == 0) {
        *(c+8) = '\0';
    } else if (strcmpi(c, "genericptr_t") == 0) {
        ispointer = 1;
    } else if (strncmpi(c, "const ", 6) == 0) {
        c = buf + 6;
    } else if ((strncmpi(c, "struct ", 7) == 0) ||
                   (strncmpi(c, "struct\t", 7) == 0)) {
        c = buf + 7;
    } else if (strncmpi(c, "union ", 6) == 0) {
        c = buf + 6;
    }

    /* end of substruct within struct definition */
    if (strcmp(buf,"}") == 0 && strlen(ssdef) > 0) {
        strcpy(buf,ssdef);
        c = buf;
    }

    for (nam = c; *c; c++) {
        if (*c >= 'A' && *c <= 'Z')
            *c = tolower(*c);
        else if (*c == ' ')
            *c = '_';
    }
    /* some fix-ups */
    if (!strcmp(nam, "genericptr_t"))
        nam = "genericptr";
    else if (!strcmp(nam, "unsigned_int"))
        nam = "uint";
    else if (!strcmp(nam, "unsigned_long"))
        nam = "ulong";
    else if (!strcmp(nam, "unsigned_char"))
        nam = "uchar";
    else if (!strcmp(nam, "unsigned_short"))
        nam = "ushort";

    if (ispointer && isptr && n == 0)
        *isptr = TRUE;
    (void)sprintf(buf2, "%s%s", nam, "");
    return buf2;
}

static char *
fieldfix(f,ss)
char *f, *ss;
{
    char *c, *dest = fieldfixbuf;

    if (strcmp(f,"}") == 0 && strlen(ss) > 0 && strlen(ss) < BUFSZ - 1) {
        /* (void)sprintf(fieldfixbuf,"struct %s", ss); */
        strcpy(fieldfixbuf,ss);
    } else {
        if (strlen(f) < BUFSZ - 1) strcpy(fieldfixbuf,f);
    }

    /* converting any tabs to space */
    for (c = fieldfixbuf; *c; c++)
        if (*c == TAB) *c = SPACE;

    return fieldfixbuf;
}

static char *
bfsize(str)
const char *str;
{
    static char buf[128];
    const char *c1;
    char *c2, *subst;

    if (!str)
        return (char *)0;

    /* kludge */
    subst = strstr(str, ",$/");
    if (subst != 0) {
        subst++;
        *subst++ = ' ';
        *subst++ = '1';
    }
    
    c2 = buf;
    c1 = str;
    while (*c1) {
        if (*c1 == ',')
            break;
        c1++;
    }

    if (*c1 == ',') {
        c1++;
        while (*c1 && *c1 != ')') {
            *c2++ = *c1++;
        }
        *c2 = '\0';
    } else {
        return (char *)0;
    }
    return buf;
}

/* Read one line from input, up to and including the next newline
 * character. Returns a pointer to the heap-allocated string, or a
 * null pointer if no characters were read.
 */
static char *
fgetline(fd)
FILE *fd;
{
    static const int inc = 256;
    int len = inc;
    char *c = malloc(len), *ret;

    for (;;) {
        ret = fgets(c + len - inc, inc, fd);
        if (!ret) {
            free(c);
            c = NULL;
            break;
        } else if (index(c, '\n')) {
            /* normal case: we have a full line */
            break;
        }
        len += inc;
        c = realloc(c, len);
    }
    return c;
}

/*readtags2.c*/


