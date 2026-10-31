/* NetHack 3.6	sftags.c	$Date$ $Revision$	          */
/* Copyright (c) Michael Allison, 2025                            */
/* NetHack may be freely redistributed.  See license for details. */

/*
 *      Read the given ctags file and generate:
 *      Intermediate temp files:
 *              include/sfo_proto.tmp
 *              include/sfi_proto.tmp
 *		util/sfi_data.tmp
 *		util/sfo_data.tmp
 *      util/sfnormalize.tmp
 *      Final files:
 *		sfdata.c
 *              sfproto.h
 *
 */

/* avoid global.h define */
#define STRNCMPI

#include "hack.h"
#include "integer.h"
#include "wintype.h"

#ifdef __GNUC__
#include <strings.h>
#ifndef WIN32
#define strncmpi strncasecmp
#endif
#define strcmpi strcasecmp
#elif defined(_MSC_VER)
#define strcmpi _stricmp
#ifndef strncmpi
#define strncmpi _strnicmp
#endif
#endif

#if 0
/* version information */
#ifdef SHORT_FILENAMES
#include "patchlev.h"
#else
#include "patchlevel.h"
#endif
#endif

#define NHTYPE_SIMPLE   1
#define NHTYPE_COMPLEX  2
struct nhdatatypes_t {
        uint dtclass;
        char *dtype;
        size_t dtsize;
};

struct tagstruct {
    uint marker;
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
#define SF_NORMALIZE_POINTERS c_sfnormalize
#define SFPROTO_NAME "../include/sfproto.h"
#define SFDATA_NAME "../util/sfdata.c"

static char *fgetline(FILE*);
ATTRNORETURN static void quit(void) NORETURN;
static void out_of_memory(void);
static void doline(char *);
static void chain(struct tagstruct *);
#if 0
static void showthem(void);
static char *stripspecial(char *);
#endif
static char *deblank(char *);
static char *deeol(char *);
static void generate_c_files(void);
static char *findtype(char *, char *);
#if 0
static boolean is_prim(char *);
#endif
static void taglineparse(char *, struct tagstruct *);
static void parseExtensionFields(struct tagstruct *, char *);
static void set_member_array_size(struct tagstruct *);
#if 0
static char *member_array_dims(struct tagstruct *, char *);
static char *member_array_size(struct tagstruct *, char *);
#endif
static void output_types(FILE *);
#if 0
static char *dtmacro(const char *,int);
#endif
static char *dtfn(const char *,int, boolean *);
static char *bfsize(const char *);
static char *fieldfix(char *,char *);
static boolean listed(struct tagstruct *t);
static const char *fn(const char *f);
static boolean no_x(const char *s);

#ifdef VMS
static FILE *vms_fopen(name, mode) const char *name, *mode;
{
    return fopen(name, mode, "mbc=64", "shr=nil");
}
# define fopen(f,m) vms_fopen(f,m)
#endif

#define Fprintf    (void) fprintf
#ifndef __GO32__
#define DEFAULTTAGNAME "../util/sf.tags"
#else
#define DEFAULTTAGNAME "../util/sftags.tag"
#endif
#ifndef _MAX_PATH
#define _MAX_PATH  120
#endif

#define TAB '\t'
#define SPACE ' '

#ifdef MACOS
#define ALIGN32 __attribute__((aligned(32)))
#else
#define ALIGN32
#endif

struct tagstruct *first;
struct tagstruct zerotag = { 0 };

static int tagcount;
static const char *infilenm;
static FILE *infile;
static char line[2048];
static long lineno;
static char ssdef[BUFSZ];
static char fieldfixbuf[BUFSZ];
static boolean suppress_count;
static char folderprefix[BUFSZ];
static char filenmbuf[
#if defined(UNIX) && defined(PATHLEN)
    PATHLEN
#elif defined(WIN32) && defined(_MAX_PATH)
    _MAX_PATH
#else
    BUFSZ * 4
#endif
        ];

#define NHTYPE_SIMPLE    1
#define NHTYPE_COMPLEX   2

struct nhdatatypes_t readtagstypes[] = {
    { NHTYPE_SIMPLE, (char *) "any", sizeof(anything) },
    { NHTYPE_SIMPLE, (char *) "genericptr_t", sizeof(genericptr_t) },
    { NHTYPE_SIMPLE, (char *) "aligntyp", sizeof(aligntyp) },
    { NHTYPE_SIMPLE, (char *) "Bitfield", sizeof(uint8_t) },
    { NHTYPE_SIMPLE, (char *) "boolean", sizeof(boolean) },
    { NHTYPE_SIMPLE, (char *) "char", sizeof(char) },
    { NHTYPE_SIMPLE, (char *) "int", sizeof(int) },
    { NHTYPE_SIMPLE, (char *) "long", sizeof(long) },
    { NHTYPE_SIMPLE, (char *) "schar", sizeof(schar) },
    { NHTYPE_SIMPLE, (char *) "short", sizeof(short) },
    { NHTYPE_SIMPLE, (char *) "size_t", sizeof(size_t) },
    { NHTYPE_SIMPLE, (char *) "string", 1 },
    { NHTYPE_SIMPLE, (char *) "time_t", sizeof(time_t) },
    { NHTYPE_SIMPLE, (char *) "uchar", sizeof(uchar) },
    { NHTYPE_SIMPLE, (char *) "unsigned char", sizeof(unsigned char) },
    { NHTYPE_SIMPLE, (char *) "uint", sizeof(uint) },
    { NHTYPE_SIMPLE, (char *) "unsigned long", sizeof(unsigned long) },
    { NHTYPE_SIMPLE, (char *) "unsigned short", sizeof(unsigned short) },
    { NHTYPE_SIMPLE, (char *) "unsigned", sizeof(unsigned) },
    { NHTYPE_SIMPLE, (char *) "xint8", sizeof(xint8) },
    { NHTYPE_SIMPLE, (char *) "xint16", sizeof(xint16) },
    { NHTYPE_SIMPLE, (char *) "coordxy", sizeof(coordxy) },
    { NHTYPE_COMPLEX, (char *) "align", sizeof(struct align) },
    /* { NHTYPE_COMPLEX, (char *) "attack", sizeof(struct attack) }, */
    /* ^ permonst affil */
    { NHTYPE_COMPLEX, (char *) "arti_info", sizeof(struct arti_info) },
    { NHTYPE_COMPLEX, (char *) "attribs", sizeof(struct attribs) },
    { NHTYPE_COMPLEX, (char *) "bill_x", sizeof(struct bill_x) },
    { NHTYPE_COMPLEX, (char *) "branch", sizeof(struct branch) },
    { NHTYPE_COMPLEX, (char *) "bubble", sizeof(struct bubble) },
    { NHTYPE_COMPLEX, (char *) "cemetery", sizeof(struct cemetery) },
    /*{ NHTYPE_COMPLEX, (char *) "container", sizeof(struct container) }, */
    { NHTYPE_COMPLEX, (char *) "context_info", sizeof(struct context_info) },
    /* context sub-structures */
    { NHTYPE_COMPLEX, (char *) "achievement_tracking",
      sizeof(struct achievement_tracking) },
    { NHTYPE_COMPLEX, (char *) "book_info", sizeof(struct book_info) },
    { NHTYPE_COMPLEX, (char *) "dig_info",
      sizeof(struct dig_info) }, /* context */
    { NHTYPE_COMPLEX, (char *) "engrave_info", sizeof(struct engrave_info) },
    { NHTYPE_COMPLEX, (char *) "lifelists", sizeof(struct lifelists) },
    { NHTYPE_COMPLEX, (char *) "obj_split", sizeof(struct obj_split) },
    { NHTYPE_COMPLEX, (char *) "polearm_info", sizeof(struct polearm_info) },
    { NHTYPE_COMPLEX, (char *) "takeoff_info", sizeof(struct takeoff_info) },
    { NHTYPE_COMPLEX, (char *) "tin_info", sizeof(struct tin_info) },
    { NHTYPE_COMPLEX, (char *) "tribute_info", sizeof(struct tribute_info) },
    { NHTYPE_COMPLEX, (char *) "victual_info", sizeof(struct victual_info) },
    { NHTYPE_COMPLEX, (char *) "warntype_info",
      sizeof(struct warntype_info) },
    /* end of context sub-structures */
    { NHTYPE_COMPLEX, (char *) "d_flags", sizeof(struct d_flags) },
    { NHTYPE_COMPLEX, (char *) "d_level", sizeof(struct d_level) },
    { NHTYPE_COMPLEX, (char *) "damage", sizeof(struct damage) },
    { NHTYPE_COMPLEX, (char *) "dest_area", sizeof(struct dest_area) },
    { NHTYPE_COMPLEX, (char *) "dgn_topology", sizeof(struct dgn_topology) },
    { NHTYPE_COMPLEX, (char *) "dungeon", sizeof(struct dungeon) },
    { NHTYPE_COMPLEX, (char *) "ebones", sizeof(struct ebones) },
    { NHTYPE_COMPLEX, (char *) "edog", sizeof(struct edog) },
    { NHTYPE_COMPLEX, (char *) "egd", sizeof(struct egd) },
    { NHTYPE_COMPLEX, (char *) "emin", sizeof(struct emin) },
    { NHTYPE_COMPLEX, (char *) "engr", sizeof(struct engr) },
    { NHTYPE_COMPLEX, (char *) "epri", sizeof(struct epri) },
    { NHTYPE_COMPLEX, (char *) "eshk", sizeof(struct eshk) },
    { NHTYPE_COMPLEX, (char *) "fakecorridor", sizeof(struct fakecorridor) },
    { NHTYPE_COMPLEX, (char *) "fe", sizeof(struct fe) },
    { NHTYPE_COMPLEX, (char *) "flag", sizeof(struct flag) },
    { NHTYPE_COMPLEX, (char *) "fruit", sizeof(struct fruit) },
    { NHTYPE_COMPLEX, (char *) "gamelog_line", sizeof(struct gamelog_line) },
    { NHTYPE_COMPLEX, (char *) "kinfo", sizeof(struct kinfo) },
    { NHTYPE_COMPLEX, (char *) "levelflags", sizeof(struct levelflags) },
    { NHTYPE_COMPLEX, (char *) "linfo", sizeof(struct linfo) },
    { NHTYPE_COMPLEX, (char *) "ls_t", sizeof(struct ls_t) },
    { NHTYPE_COMPLEX, (char *) "mapseen_feat", sizeof(struct mapseen_feat) },
    { NHTYPE_COMPLEX, (char *) "mapseen_flags",
      sizeof(struct mapseen_flags) },
    { NHTYPE_COMPLEX, (char *) "mapseen_rooms",
      sizeof(struct mapseen_rooms) },
    { NHTYPE_COMPLEX, (char *) "mapseen", sizeof(mapseen) },
    { NHTYPE_COMPLEX, (char *) "mextra", sizeof(struct mextra) },
    { NHTYPE_COMPLEX, (char *) "mkroom", sizeof(struct mkroom) },
    { NHTYPE_COMPLEX, (char *) "monst", sizeof(struct monst) },
    { NHTYPE_COMPLEX, (char *) "mvitals", sizeof(struct mvitals) },
    { NHTYPE_COMPLEX, (char *) "nhcoord", sizeof(struct nhcoord) },
    { NHTYPE_COMPLEX, (char *) "nhrect", sizeof(struct nhrect) },
    { NHTYPE_COMPLEX, (char *) "novel_tracking",
      sizeof(struct novel_tracking) },
    { NHTYPE_COMPLEX, (char *) "obj", sizeof(struct obj) },
    { NHTYPE_COMPLEX, (char *) "objclass", sizeof(struct objclass) },
    { NHTYPE_COMPLEX, (char *) "oextra", sizeof(struct oextra) },
    /*    {NHTYPE_COMPLEX, (char *) "permonst", sizeof(struct permonst)}, */
    { NHTYPE_COMPLEX, (char *) "prop", sizeof(struct prop) },
    { NHTYPE_COMPLEX, (char *) "q_score", sizeof(struct q_score) },
    { NHTYPE_COMPLEX, (char *) "rm", sizeof(struct rm) },
    { NHTYPE_COMPLEX, (char *) "s_level", sizeof(struct s_level) },
    { NHTYPE_COMPLEX, (char *) "skills", sizeof(struct skills) },
    { NHTYPE_COMPLEX, (char *) "spell", sizeof(struct spell) },
    { NHTYPE_COMPLEX, (char *) "stairway", sizeof(struct stairway) },
#ifdef SYSFLAGS
    { NHTYPE_COMPLEX, (char *) "sysflag", sizeof(struct sysflag) },
#endif
    { NHTYPE_COMPLEX, (char *) "trap", sizeof(struct trap) },
    /*    {NHTYPE_COMPLEX, (char *) "u_achieve", sizeof(struct u_achieve)}, */
    { NHTYPE_COMPLEX, (char *) "u_conduct", sizeof(struct u_conduct) },
    { NHTYPE_COMPLEX, (char *) "u_event", sizeof(struct u_event) },
    { NHTYPE_COMPLEX, (char *) "u_have", sizeof(struct u_have) },
    { NHTYPE_COMPLEX, (char *) "u_realtime", sizeof(struct u_realtime) },
    { NHTYPE_COMPLEX, (char *) "u_roleplay", sizeof(struct u_roleplay) },
    { NHTYPE_COMPLEX, (char *) "version_info", sizeof(struct version_info) },
    { NHTYPE_COMPLEX, (char *) "vlaunchinfo", sizeof(union vlaunchinfo) },
    { NHTYPE_COMPLEX, (char *) "vptrs", sizeof(union vptrs) },
    { NHTYPE_COMPLEX, (char *) "you", sizeof(struct you) },
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
const char *condtag[] = {
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

DISABLE_WARNING_UNREACHABLE_CODE

int main(int argc, char *argv[])
{
    tagcount = 0;
#if defined(WIN32)
    char fbuf[_MAX_PATH];
#endif

    folderprefix[0] = '\0';
    if (argc > 1) infilenm = argv[1];
    if (!infilenm || !*infilenm) infilenm = DEFAULTTAGNAME;

#if defined(WIN32)
    if (!file_exists(infilenm)) {
        /* (void) GetCurrentDirectoryA(sizeof cdir, (LPSTR) cdir); */
        Strcpy(folderprefix, "../../../");
        (void) snprintf(fbuf, sizeof fbuf, "%s%s", folderprefix, DEFAULTTAGNAME);
        if (!file_exists(fbuf)) {
            fprintf(stdout, "%s doesn't exist\n", fbuf);
            exit(EXIT_SUCCESS);
        }
        infilenm = (const char *) fbuf;
    }
#endif

    infile = fopen(infilenm,"r");
    if (!infile) {
        printf("%s not found or unavailable\n", infilenm);
        quit();
    } else {
        while (fgets(line, sizeof line, infile)) {
            ++lineno;
            /*        if (lineno == 868) DebugBreak(); */
            doline(line);
        }

        fclose(infile);
        printf("\nRead in %ld lines and stored %d tags in memory.\n", lineno,
               tagcount);
#if 0
        showthem();
#endif
        generate_c_files();
        printf("Created %s\n", SFDATA_NAME);
        printf("Created %s\n", SFPROTO_NAME);
        exit(EXIT_SUCCESS);
        /*NOTREACHED*/
        return 0;
    }
}

#ifdef WIN32
boolean
file_exists(const char *path)
{
    struct stat sb;

    /* Just see if it's there */
    if (stat(path, &sb)) {
        return FALSE;
    }
    return TRUE;
}
#endif

RESTORE_WARNINGS

static void doline(char *aline)
{
    char buf[255], *cp;
    struct tagstruct * ALIGN32 tmptag;
    size_t slen;

    if (!aline || (aline && *aline == '!')) {
        return;
    }
    cp = deeol(aline);
    slen = strlen(cp);
    if (slen > sizeof buf - 1) {
        slen = sizeof buf - 1;
    }

    tmptag = malloc(sizeof *tmptag);
    if (!tmptag) {
        out_of_memory();
    }
    assert(tmptag != 0);
    *tmptag = zerotag;
    tmptag->marker = 0xDEADBEEF;

    strncpy(buf, cp, slen);
    buf[sizeof buf - 1] = '\0';
    taglineparse(buf, tmptag);
    chain(tmptag);
    return;
}

struct tagstruct * ALIGN32 prevtag = 0;

static void
chain(struct tagstruct *tag)
{
    if (!first) {
        tag->next = (struct tagstruct *) 0;
        first = tag;
    } else {
        tag->next = (struct tagstruct *) 0;

        if (prevtag->marker == 0xDEADBEEF) {
            prevtag->next = tag;
        } else {
            printf("Error - No previous tag at %s\n", tag->tag);
        }
    }
    prevtag = tag;
    ++tagcount;
}
static void quit(void)
{
    exit(EXIT_FAILURE);
}

static void out_of_memory(void)
{
    printf("maketags: out of memory at line %ld of %s\n",
        lineno, infilenm);
    quit();
}

#if 0
static char empt[] = {0, 0, 0, 0, 0, 0, 0, 0};
#endif

#if 0
static char *member_array_dims(struct tagstruct *tmptag, char *buf)
{
    if (buf && tmptag) {
        if (tmptag->arraysize1[0])
            Sprintf(buf, "[%s]", tmptag->arraysize1);
        if (tmptag->arraysize2[0])
            Sprintf(eos(buf), "[%s]", tmptag->arraysize2);
        return buf;
    }
    return empt;
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
    return empt;
}
#endif

void set_member_array_size(struct tagstruct *tmptag)
{
    char buf[BUFSZ];
    /* static char result[49]; */
    char *arr1 = (char *)0, *arr2 = (char *)0, *tmp;
    int cnt = 0;

    if (!tmptag) return;
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

static void parseExtensionFields (struct tagstruct *tmptag, char *buf)
{
    char *p = buf;
#ifdef WIN32
    int debug_catch = 0;
#endif

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
                char *key = field;
                char *value = colon + 1;
                *colon = '\0';
                if ((strcmp (key, "struct") == 0) ||
                    (strcmp (key, "union") == 0)) {
#ifdef WIN32
                    if (!strcmp(key, "union"))
                        debug_catch = 1;
#endif
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
taglineparse(char *p, struct tagstruct *tmptag)
{
    int fieldsPresent = 0;
    char *pattern = 0, *tmp1 = 0;
    int linenumber = 0;
    char *tab = strchr (p, TAB);

    if (tab != NULL) {
        *tab = '\0';
        strcpy(tmptag->tag,p);
        p = tab + 1;
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
    assert(pattern != NULL);

    strcpy(tmptag->searchtext, pattern);
    tmptag->linenum = linenumber;

    /* add the array dimensions */
    set_member_array_size(tmptag);

    /* determine if this is a pointer and mark it as such */
    if (tmptag->searchtext[0] &&
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

static char stripbuf[255];

#if 0
static char *stripspecial(char *st)
{
    char *out = stripbuf;
    *out = '\0';
    if (!st) return st;
    while(*st) {
        if (*st >= SPACE)
            *out++ = *st++;
        else
            st++;
    }
    *out = '\0';
    return stripbuf;
}
#endif

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
static void showthem(void)
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

#if 0
static boolean
is_prim(char *sdt)
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
#endif

char *
findtype(char *st, char *tag)
{
    static char ftbuf[512];
    static char prevbuf[512];
    char *tmp1, *tmp2, *tmp3, *tmp4;
    const char *r;

    if (!st) return (char *)0;

#if 0
    if (st && strstr(st, "mapseen")) {
        int xx = 0;

        xx++;
    }
#endif
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

            prevchar = 0;
            /* some oddball cases */
            if (prevchar == ',' || prevchar == ';') {
                tmp3 = strchr(ftbuf, ',');
                tmp2 = strstr(ftbuf, tag);
                return prevbuf;
            } else {
                int chkcnt = 0;

                /* a comma means that more than one thing declared on ine */
                tmp3 = strchr(tmp1, ',');
                while (chkcnt < 3 && (tmp2 = strstr(tmp1, tag))
                       && (prevchar = *(tmp2 - 1))
                       && ((prevchar == '_')
                           || (prevchar >= 'a' && prevchar <= 'z')
                           || (prevchar >= 'A' && prevchar <= 'Z')
                           || (prevchar >= '0' && prevchar <= '9'))) {
                    tmp1 = tmp2 + 1;
                    chkcnt++;
                }
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
                volatile int y = 0;
                nhUse(y);
                y = 1;
            }
            if (strncmpi(ftbuf, "struct ", 7) == 0)
                r = (const char *) (ftbuf + 7);
            else if (strncmpi(ftbuf, "union ", 6) == 0)
                r = (const char *) (ftbuf + 6);
            /* a couple of kludges follow unfortunately */
            else if (strncmpi(ftbuf, "coord", 5) == 0
                     && strncmpi(ftbuf, "coordxy", 7) != 0)
                r = "nhcoord";
            else if (strncmpi(ftbuf, "anything", 8) == 0)
                r = "any";
            else if (strncmpi(ftbuf, "const char", 10) == 0)
                r = "char";
            else
                r = (const char *) ftbuf;
            strcpy(prevbuf, r);
            return prevbuf;
        }
    }
    prevbuf[0] = '\0';
    return (char *)0;
}

int current_type = 64, gotit = 0;

boolean
listed(struct tagstruct *t)
{
    int k;

    if ((strncmpi(t->tag, "Bitfield", 8) == 0) ||
                    (strcmpi(t->tag, "string") == 0))
        return TRUE;
    for (k = 0; k < SIZE(readtagstypes); ++k) {
        if (k == current_type)
            gotit = k;
        /*  This needs to be case-sensitive to avoid generating collision
         *  between 'align' and 'Align'.
         */
        if (strcmp(readtagstypes[k].dtype, t->tag) == 0)
            return TRUE;
    }
    return FALSE;
}

/* TIME_type: type of the argument to time(); we actually use &(time_t) */
#if defined(BSD) && !defined(POSIX_TYPES)
#define TIME_type long *
#else
#define TIME_type time_t *
#endif
/* LOCALTIME_type: type of the argument to localtime() */
#if (defined(ULTRIX) && !(defined(ULTRIX_PROTO) || defined(NHSTDC))) \
    || (defined(BSD) && !defined(POSIX_TYPES))
#define LOCALTIME_type long *
#else
#define LOCALTIME_type time_t *
#endif

const char *preamble[] = {
    "/* Copyright (c) NetHack Development Team %d.                   */\n",
    "/* NetHack may be freely redistributed.  See license for details. */\n\n",
    "/* THIS IS AN AUTOGENERATED FILE. DO NOT EDIT THIS FILE!          */\n\n",
    "#include \"hack.h\"\n",
    "#include \"artifact.h\"\n",
    "#include \"func_tab.h\"\n",
    "#include \"integer.h\"\n",
    "#include \"wintype.h\"\n",
    (char *)0
};
char crbuf[BUFSZ];

DISABLE_WARNING_FORMAT_NONLITERAL

static const char *get_preamble(int n)
{
    const char *r = preamble[n];

    if (!n) {
        time_t datetime = 0;
        struct tm *lt;

        (void) time((TIME_type) &datetime);
        lt = localtime((LOCALTIME_type) &datetime);
        Sprintf(crbuf, preamble[0], (1900 + lt->tm_year));
        r = crbuf;
    }
    return r;
}

RESTORE_WARNING_FORMAT_NONLITERAL

static void output_types(FILE *fp1)
{
    int k, cnt /*, hcnt = 1 */;
    struct tagstruct *t = first;

    Fprintf(fp1, "%s",
        "struct nhdatatypes_t nhdatatypes[] = {\n");

    for (k = 0; k < SIZE(readtagstypes); ++k) {
        if (readtagstypes[k].dtclass == NHTYPE_SIMPLE) {
            Fprintf(fp1,"    {NHTYPE_SIMPLE, (char *) \"%s\", sizeof(%s)},\n",
                readtagstypes[k].dtype,
                (strncmpi(readtagstypes[k].dtype, "Bitfield", 8) == 0) ?
                "uint8_t" :
                (strcmpi(readtagstypes[k].dtype, "string") == 0) ?
                "uchar" :
                (strcmpi(readtagstypes[k].dtype, "any") == 0) ?
                                "anything" : readtagstypes[k].dtype);
/*                dtmacro(readtagstypes[k].dtype,0)); */
#if 0
            Fprintf(fp2, "#define %s    %s%d\n", dtmacro(readtagstypes[k].dtype,1),
                    (strlen(readtagstypes[k].dtype) > 12) ? "" :
                (strlen(readtagstypes[k].dtype) < 5) ? "        " :
                "    ", hcnt++);
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
            Fprintf(fp1, "    {NHTYPE_COMPLEX, (char *) \"%s\", sizeof(%s %s)}",
                    t->tag,
                    (t->tagtype == 's') ? "struct" : "union", t->tag);
            cnt += 1;
        }
        t = t->next;
    }
    Fprintf(fp1, "%s", "\n};\n\n");
    Fprintf(fp1, "int nhdatatypes_size(void)\n{\n    return SIZE(nhdatatypes);\n}\n\n");
}

static void generate_c_files(void)
{
    struct tagstruct *t = first;
#ifdef KR1ED
    long clocktim = 0;
#else
    time_t clocktim = 0;
#endif
    char *c, cbuf[60], sfparent[BUFSZ], funcnam[2][BUFSZ], *substruct, *gline,
        norm_param_buf[BUFSZ];
    FILE *SFO_DATA, *SFI_DATA, *SFDATATMP, *SFO_PROTO, *SFI_PROTO,
         *SFDATA, *SFPROTO, *SF_NORMALIZE_POINTERS;
    int k = 0, j /*, opening, , closetag = 0 */;
    const char *pt;
    /* char *layout; */
    char *ft, *last_ft = (char *)0;
    int okeydokey, x, a;
    boolean did_i;
    boolean normalize_param_used;

#ifdef WIN32
    int debug_catch = 0;
#endif

    Snprintf(filenmbuf, sizeof filenmbuf, "%s%s", folderprefix,SFDATA_NAME);
    SFDATA = fopen(filenmbuf, "w");
    if (!SFDATA) return;

    Snprintf(filenmbuf, sizeof filenmbuf, "%s%s", folderprefix, SFPROTO_NAME);
    SFPROTO = fopen(filenmbuf, "w");
    if (!SFPROTO) return;

    Snprintf(filenmbuf, sizeof filenmbuf, "%s%s", folderprefix,
             "../util/sfo_data.tmp");
    SFO_DATA = fopen(filenmbuf, "w");
    if (!SFO_DATA) return;

    Snprintf(filenmbuf, sizeof filenmbuf, "%s%s", folderprefix, "../include/sfo_proto.tmp");
    SFO_PROTO = fopen(filenmbuf, "w");
    if (!SFO_PROTO) return;

    Snprintf(filenmbuf, sizeof filenmbuf, "%s%s", folderprefix,
             "../include/sfi_proto.tmp");
    SFI_PROTO = fopen(filenmbuf, "w");
    if (!SFI_PROTO) return;

    Snprintf(filenmbuf, sizeof filenmbuf, "%s%s", folderprefix,
             "../util/sfi_data.tmp");
    SFI_DATA = fopen(filenmbuf, "w");
    if (!SFI_DATA) return;

    Snprintf(filenmbuf, sizeof filenmbuf, "%s%s", folderprefix,
             "../util/sfdata.tmp");
    SFDATATMP = fopen(filenmbuf, "w");
    if (!SFDATATMP) return;

    Snprintf(filenmbuf, sizeof filenmbuf, "%s%s", folderprefix,
             "../util/sfnormptrs.tmp");
    SF_NORMALIZE_POINTERS = fopen(filenmbuf, "w");
    if (!SF_NORMALIZE_POINTERS) return;

    (void) time(&clocktim);
    Strcpy(cbuf, ctime(&clocktim));

    for (c = cbuf; *c; c++)
        if (*c == '\n')
            break;
    *c = '\0';    /* strip off the '\n' */

    /* begin sfproto.h */
    Fprintf(SFPROTO,"/* NetHack %d.%d sfproto.h */\n",
               VERSION_MAJOR, VERSION_MINOR);
    for (j = 0; j < 3; ++j)
        Fprintf(SFPROTO, "%s", get_preamble(j));
    Fprintf(SFPROTO, "#ifndef SFPROTO_H\n#define SFPROTO_H\n\n");
    Fprintf(SFPROTO, "#include \"hack.h\"\n#include \"integer.h\"\n#include \"wintype.h\"\n\n");

    Fprintf(SFPROTO,"%s\n", "extern int critical_members_count(void);");
    Fprintf(SFPROTO,"%s\n", "extern void sfo_bitfield(NHFILE *, uint8_t *, const char *, int);");
    Fprintf(SFPROTO,"%s\n", "extern void sfi_bitfield(NHFILE *, uint8_t *, const char *, int);");
    Fprintf(SFO_PROTO, "/* generated output functions */\n");
    Fprintf(SFI_PROTO, "/* generated input functions */\n");

    /* begin sfdata.c */
    Fprintf(SFDATA,"/* NetHack %d.%d sfdata.c */\n",
            VERSION_MAJOR, VERSION_MINOR);
    for (j = 0; preamble[j]; ++j)
        Fprintf(SFDATA, "%s", get_preamble(j));
    Fprintf(SFDATA, "#include \"sfprocs.h\"\n");
    Fprintf(SFDATA, "#include \"sfproto.h\"\n\n");
    Fprintf(SFDATA, "#define NHTYPE_SIMPLE    1\n");
    Fprintf(SFDATA, "#define NHTYPE_COMPLEX   2\n\n");
    Fprintf(SFDATA, "struct nhdatatypes_t {\n");
    Fprintf(SFDATA, "    uint dtclass;\n");
    Fprintf(SFDATA, "    char *dtype;\n");
    Fprintf(SFDATA, "    size_t dtsize;\n};\n\n");
    Fprintf(SFDATA,"static uint8_t bitfield = 0;\n");

    /* begin sfnormptrs.tmp */
    Fprintf(SF_NORMALIZE_POINTERS,
            "void norm_ptrs_any(union any *d_any);\n\n");
    Fprintf(SF_NORMALIZE_POINTERS,
            "void\n"
            "norm_ptrs_any(union any *d_any UNUSED)\n"
            "{\n"
            "}\n");

    output_types(SFDATATMP);
    Fprintf(SFDATATMP, "const char *critical_members[] = {\n");

    k = 0;
    did_i = FALSE;
    while(k < SIZE(readtagstypes)) {
      boolean insert_const = FALSE;
      suppress_count = TRUE;

      if (readtagstypes[k].dtclass == NHTYPE_COMPLEX) {


          if (!strncmpi(readtagstypes[k].dtype,"Bitfield",8)) {
              Fprintf(SFO_DATA,
                    "\nvoid\nsfo_bitfield(nhfp,\n"
                    "NHFILE *nhfp,\n"
                    "uint8_t *d_bitfield,\n"
                    "const char *myname\n"
                    "int bflen)\n"
                    "{\n");

              Fprintf(SFI_DATA,
                    "\nvoid\nsfi_bitfield(\n"
                    "NHFILE *nhfp,\n"
                    "uint8_t *d_bitfield,\n"
                    "const char *myname\n"
                    "int bflen)\n"
                    "{\n");
        }


#if 0
        if (!strncmpi(readtagstypes[k].dtype,"version_info",8))
            insert_const = TRUE;
#endif
#ifdef WIN32
        if (!strncmpi(readtagstypes[k].dtype, "vlaunchinfo", 11)
            || !strncmpi(readtagstypes[k].dtype, "vptrs", 5)
            || k == SIZE(readtagstypes) - 2)
            debug_catch = TRUE;
#endif

        pt = (const char *) 0;
        t = first;
        while(t) {
            if (!strcmp(t->tag, readtagstypes[k].dtype)) {
                pt = (t->tagtype == 'u') ? "union" : "struct";
                break;
            }
            t = t->next;
        }
        if (!pt) {
            pt = "struct";
        }

        (void) snprintf(funcnam[0], sizeof funcnam[0], "sfo_x_%s", readtagstypes[k].dtype);
        Fprintf(SFO_PROTO,
                "extern void %s(NHFILE *, %s%s %s *, const char *);\n",
                fn(funcnam[0]),
                insert_const ? "const " : "",
                pt, readtagstypes[k].dtype);

        Fprintf(SFO_DATA,
                "\nvoid\n%s(\n"
                "NHFILE *nhfp,\n"
                "%s%s %s *d_%s,\n"
                "const char *myname)\n"
                "{\n",
                fn(funcnam[0]),
/*                deblank(readtagstypes[k].dtype), */
                insert_const ? "const " : "",
                pt, readtagstypes[k].dtype,
                deblank(readtagstypes[k].dtype));

#if 0
        Fprintf(SFO_DATA,
                "    const char *parent = \"%s\";\n",
                readtagstypes[k].dtype);
#endif

        (void) snprintf(funcnam[0], sizeof funcnam[0], "sfi_x_%s",
                        readtagstypes[k].dtype);
        Fprintf(SFI_PROTO,
                "extern void %s(NHFILE *, %s%s %s *, const char *);\n",
                fn(funcnam[0]),
                insert_const ? "const " : "",
                pt, readtagstypes[k].dtype);

        Fprintf(SFI_DATA,
                "\nvoid\n%s(\n"
                "NHFILE *nhfp,\n"
                "%s%s %s *d_%s,\n"
                "const char *myname)\n"
                "{\n",
                fn(funcnam[0]),
/*                deblank(readtagstypes[k].dtype), */
                insert_const ? "const " : "",
                pt, readtagstypes[k].dtype,
                deblank(readtagstypes[k].dtype));

#if 0
        Fprintf(SFI_DATA,
                "    const char *parent = \"%s\";\n",
                readtagstypes[k].dtype);
#endif

        Sprintf(sfparent, "%s %s", pt, readtagstypes[k].dtype);


        Fprintf(SF_NORMALIZE_POINTERS,
                "\nvoid norm_ptrs_%s(%s "
                "*d_%s);\n",
                readtagstypes[k].dtype, sfparent, readtagstypes[k].dtype);
        Fprintf(SF_NORMALIZE_POINTERS,
                "\nvoid\nnorm_ptrs_%s(%s "
                "*d_%s)\n{\n",
                readtagstypes[k].dtype, sfparent, readtagstypes[k].dtype);
        Snprintf(norm_param_buf, sizeof norm_param_buf, "d_%s",
                 readtagstypes[k].dtype);

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

        Fprintf(SFO_DATA, "    nhUse(myname);\n");
        Fprintf(SFI_DATA, "    nhUse(myname);\n");
        Fprintf(SFO_DATA, "\n");
        Fprintf(SFI_DATA, "\n");

        /********************************************************
         *  cycle through all the tags and find every tag with  *
         *  a parent matching readtagstypes[k].dtype            *
         ********************************************************/

        t = first;
        normalize_param_used = FALSE;

        while(t) {
            x = 0;
            okeydokey = 0;
/*
            if (!strcmp(t->tag, "nextc")
                && !strcmp(readtagstypes[k].dtype, "engrave_info"))
                __debugbreak();
*/

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
             * Legal, but a greater challenge for us here.
             */
            substruct = strstr(t->parent, "::");
            if (substruct) {
                substruct += 2;
            }

            if ((strcmp(readtagstypes[k].dtype, t->parent) == 0) ||
                 (substruct && strcmp(readtagstypes[k].dtype, substruct) == 0)) {
                ft = (char *)0;

                if (t->ptr[0] != '\0')
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
                    int j2, z;

                    Sprintf(lbuf,
                            "    "
                            "bitfield = d_%s->%s;",
                            readtagstypes[k].dtype, t->tag);
                    z = (int) strlen(lbuf);
                    for (j2 = 0; j2 < (65 - z); ++j2)
                        Strcat(lbuf, " ");
                    Sprintf(eos(lbuf), "/* (%s) */\n", ft);
                    Fprintf(SFO_DATA, "%s", lbuf);
                    Fprintf(SFO_DATA,
                            "    "
                            "sfo_bitfield(nhfp, &bitfield, \"%s\", %s);\n",
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
                            "sfi_bitfield(nhfp, &bitfield, \"%s\", %s);\n",
                            t->tag, bfsize(ft));

                    Fprintf(SFI_DATA,
                            "    "
                            "d_%s->%s = bitfield;\n\n",
                            readtagstypes[k].dtype, t->tag);
                    Fprintf(SFDATATMP,
                            "    \"%s:%s:%s\",\n",
                            sfparent, t->tag, ft);
                } else {
                    /**************** not a bitfield ****************/
                    char arrbuf[BUFSZ];
                    char lbuf[BUFSZ * 2]; /* sprintf target for others, gcc
                                             complaint */
                    char fnbuf[BUFSZ];
                    char altbuf[BUFSZ];
                    boolean isptr = FALSE, kludge_sbrooms = FALSE, array_of_ptrs = FALSE;
                    boolean insert_loop = FALSE;
                    int j2, z;

                    altbuf[0] = '\0';
                    /*************** kludge for sbrooms *************/
                    if (!strcmp(t->tag, "sbrooms")) {
                        kludge_sbrooms = TRUE;
                        (void) strcpy(t->arraysize1, "MAX_SUBROOMS");
                        insert_loop = TRUE;
                        array_of_ptrs = TRUE;
                    }
                    if (!strcmp(t->parent, "engr")
                        && !strcmp(t->tag, "engr_txt")) {
                        (void) strcpy(t->arraysize1, "text_states");
                        insert_loop = TRUE;
                        array_of_ptrs = TRUE;
                    }
                    if (t->arraysize2[0]) {
                        Sprintf(arrbuf, "(%s * %s)", t->arraysize1,
                                t->arraysize2);
                        isptr = TRUE; /* suppress the & in function args */
                    } else if (t->arraysize1[0]) {
                        Sprintf(arrbuf, "%s", t->arraysize1);
                        isptr = TRUE; /* suppress the & in function args */
                    } else {
                        Strcpy(arrbuf, "1");
                    }
                    Strcpy(fnbuf, dtfn(ft, 0, &isptr));
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
                    } else if (isptr
                               && (!strcmp(t->ptr, "struct permonst *")
                                   || !strcmp(t->ptr, "struct monst *")
                                   || !strcmp(t->ptr, "struct obj *")
                                   || !strcmp(t->ptr, "struct cemetery *")
                                   || !strcmp(t->ptr, "struct container *")
                                   || !strcmp(t->ptr, "struct mextra *")
                                   || !strcmp(t->ptr, "struct oextra *")
                                   || !strcmp(t->ptr, "struct s_level *")
                                   || !strcmp(t->ptr, "struct bill_x *")
                                   || !strcmp(t->ptr, "struct trap *")
                                   || !strcmp(t->ptr, "struct egd *")
                                   || !strcmp(t->ptr, "struct epri *")
                                   || !strcmp(t->ptr, "struct eshk *")
                                   || !strcmp(t->ptr, "struct emin *")
                                   || !strcmp(t->ptr, "struct ebones *")
                                   || !strcmp(t->ptr, "struct edog *"))) {
                        Strcpy(altbuf, "genericptr");
                    } else if (isptr
                               && (!strcmp(t->parent, "engr")
                                   && !strcmp(t->tag, "engr_txt"))) {
                        Strcpy(altbuf, "genericptr");
                    } else if (isptr
                               && (!strcmp(t->parent, "mapseen")
                                   && !strcmp(t->tag, "br"))) {
                        Strcpy(altbuf, "genericptr");

                        //                    } else if (isptr
                        //                               &&
                        //                               (!strcmp(t->parent,
                        //                               "engrave_info")
                        //                                   &&
                        //                                   !strcmp(t->tag,
                        //                                   "nextc"))) {
                        //                        Strcpy(altbuf,
                        //                        "genericptr");

                    } else if (
                        (isptr && !strcmp(t->ptr, "char *"))
                        && ((!strcmp(t->parent, "engrave_info")
                             && !strcmp(t->tag, "nextc"))
                            || (!strcmp(t->parent, "mapseen")
                                && !strcmp(t->tag, "custom"))
                            || (!strcmp(t->parent, "mapseen")
                                && !strcmp(t->tag, "custom"))
                            || (!strcmp(t->parent, "mextra")
                                && !strcmp(t->tag, "mgivenname"))
                            || (!strcmp(t->parent, "gamelog_line")
                                       && !strcmp(t->tag, "text"))
                            || (!strcmp(t->parent, "oextra")
                                && !strcmp(t->tag, "oname"))
                            || (!strcmp(t->parent, "oextra")
                                && !strcmp(t->tag, "omailcmd")))) {
                        Strcpy(altbuf, "genericptr");
                    } else if (isptr && !strcmp(t->tag, "oc_uname")) {
                        Strcpy(altbuf, "genericptr");
                    } else {
                        Strcpy(altbuf, fnbuf);
                    }
                    if (isptr && (strcmp(altbuf, "genericptr") != 0)
                        && (t->ptr[0] != 0
                            && (*(t->ptr + (strlen(t->ptr) - 1)) == '*')
                            && (strcmp(t->ptr, altbuf) != 0))) {
                        fprintf(
                            stderr,
                            "WARNING - \"%s\" in %s called \"%s\" "
                            "resulted in an unexpected set of inputs/outputs "
                            "(%s)\n",
                            (t->ptr[0] != 0 && strlen(t->ptr)) ? t->ptr : "unknown",
                            (t->parent[0] != 0 && strlen(t->parent)) ? t->parent
                                                             : "?",
                            (t->tag[0] != 0 && strlen(t->tag)) ? t->tag : "?",
                            altbuf);
                    }
                    /* kludge for attribs */
                    if (!strcmp(readtagstypes[k].dtype, "attribs")
                        && !strcmp(arrbuf, "A_MAX")) {
                        insert_loop = TRUE;
                    }
                    if (insert_loop) {
                        Fprintf(SFO_DATA, "    for (%si = 0; i < %s; ++i)\n",
                                did_i ? "" : "int ", arrbuf);
                        Fprintf(SFI_DATA, "    for (%si = 0; i < %s; ++i)\n",
                                did_i ? "" : "int ", arrbuf);
                        if (array_of_ptrs) {
                            Fprintf(SF_NORMALIZE_POINTERS,
                                    "    for (%si = 0; i < %s; ++i)\n",
                                    did_i ? "" : "int ", arrbuf);
                        }
                        arrbuf[0] = '1';
                        arrbuf[1] = '\0';
                    }
                    if (isptr
                        && (t->ptr[0] != 0
                            && (*(t->ptr + (strlen(t->ptr) - 1)) == '*')
                            && (strcmp(t->ptr, altbuf) != 0))) {
                        Fprintf(
                            SF_NORMALIZE_POINTERS,
                            "    %sd_%s->%s%s%s%s = d_%s->%s%s%s%s ? (%s) 1 : (%s) 0;\n",
                            (insert_loop && array_of_ptrs) ? "    " : "",
                            t->parent, t->tag,
                            (insert_loop && array_of_ptrs) ? "[" : "",
                            (insert_loop && array_of_ptrs) ? "i" : "",
                            (insert_loop && array_of_ptrs) ? "]" : "",
                            t->parent, t->tag,
                            (insert_loop && array_of_ptrs) ? "[" : "",
                            (insert_loop && array_of_ptrs) ? "i" : "",
                            (insert_loop && array_of_ptrs) ? "]" : "",
                            t->ptr,
                            t->ptr);
                        normalize_param_used = TRUE;
                    }
                    Snprintf(lbuf, sizeof lbuf,
                             "    "
                             "%ssfo%s_%s(nhfp, %s%sd_%s->%s%s, \"%s\"%s%s);",
                             insert_loop ? "    " : "",
                             (readtagstypes[k].dtclass == NHTYPE_SIMPLE || no_x(altbuf)) ? ""
                                                                       : "_x",
                                 altbuf,
                             (isptr && !strcmp(altbuf, "genericptr"))
                                 ? "(genericptr_t) "
                                 : "",
                             (isptr && !insert_loop && !kludge_sbrooms
                              && strcmp(altbuf, "genericptr"))
                                 ? ""
                                 : "&",
                             readtagstypes[k].dtype, t->tag,
                             insert_loop  ? "[i]"
                                            : "",
                             t->tag, strcmp(altbuf, "char") != 0 ? "" : ", ",
                             strcmp(altbuf, "char") != 0 ? "" : arrbuf);
                    /* align comments */
                    z = (int) strlen(lbuf);
                    for (j2 = 0; j2 < (65 - z); ++j2)
                        Strcat(lbuf, " ");
                    Sprintf(eos(lbuf), "/* (%s) */\n", ft);
                    Fprintf(SFO_DATA, "%s", lbuf);

                    Snprintf(
                        lbuf, sizeof lbuf,
                        "    "
                        "%ssfi%s_%s(nhfp, %s%sd_%s->%s%s, \"%s\"%s%s);\n",
                        insert_loop ? "    " : "",
                        (readtagstypes[k].dtclass == NHTYPE_SIMPLE
                         || no_x(altbuf))
                            ? ""
                            : "_x",
                        altbuf,
                        (isptr && !strcmp(altbuf, "genericptr"))
                            ? "(genericptr_t) "
                            : "",
                        (isptr && !insert_loop && !kludge_sbrooms
                         && strcmp(altbuf, "genericptr"))
                            ? ""
                            : "&",
                        readtagstypes[k].dtype, t->tag,
                        kludge_sbrooms ? "[0]"
                        : insert_loop  ? "[i]"
                                       : "",
                        t->tag, strcmp(altbuf, "char") != 0 ? "" : ", ",
                        strcmp(altbuf, "char") != 0 ? "" : arrbuf);
                    Fprintf(SFI_DATA, "%s", lbuf);
                    Fprintf(SFDATATMP,
                        "    \"%s:%s:%s\",\n",
                        sfparent, t->tag,fieldfix(ft,ssdef));
                    kludge_sbrooms = FALSE;
                    array_of_ptrs = FALSE;
                    altbuf[0] = '\0';
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

        Fprintf(SFO_DATA, "}\n");
        Fprintf(SFI_DATA, "}\n");
        if (!normalize_param_used) {
            Fprintf(SF_NORMALIZE_POINTERS, "    nhUse(%s);\n",
                    norm_param_buf);
        }
        Fprintf(SF_NORMALIZE_POINTERS, "}\n");
      }
      ++k;
      did_i = FALSE;
    }

    Fprintf(SFDATATMP,"};\n\n");
    Fprintf(SFDATATMP, "int critical_members_count(void)\n{\n    return SIZE(critical_members);\n}\n\n");

    fclose(SFO_DATA);
    fclose(SFI_DATA);
    fclose(SFO_PROTO);
    fclose(SFI_PROTO);
    fclose(SFDATATMP);
    fclose(SF_NORMALIZE_POINTERS);

    /* Consolidate SFO_* and SFI_* etc into single files */

    Snprintf(filenmbuf, sizeof filenmbuf, "%s%s", folderprefix,
             "../util/sfo_data.tmp");
    SFO_DATA = fopen(filenmbuf, "r");
    if (!SFO_DATA) return;
    while ((gline = fgetline(SFO_DATA)) != 0) {
        (void) fputs(gline, SFDATA);
        free(gline);
    }
    (void) fclose(SFO_DATA);
    (void) remove(filenmbuf);

    Snprintf(filenmbuf, sizeof filenmbuf, "%s%s", folderprefix,
             "../util/sfi_data.tmp");
    SFI_DATA = fopen(filenmbuf, "r");
    if (!SFI_DATA) return;
    while ((gline = fgetline(SFI_DATA)) != 0) {
        (void) fputs(gline, SFDATA);
        free(gline);
    }
    (void) fclose(SFI_DATA);
    (void) remove(filenmbuf);

    Snprintf(filenmbuf, sizeof filenmbuf, "%s%s", folderprefix,
             "../include/sfo_proto.tmp");
    SFO_PROTO = fopen(filenmbuf, "r");
    if (!SFO_PROTO) return;
    while ((gline = fgetline(SFO_PROTO)) != 0) {
        (void) fputs(gline, SFPROTO);
        free(gline);
    }
    (void) fclose(SFO_PROTO);
    (void) remove(filenmbuf);

    Snprintf(filenmbuf, sizeof filenmbuf, "%s%s", folderprefix,
             "../include/sfi_proto.tmp");
    SFI_PROTO = fopen(filenmbuf, "r");
    if (!SFI_PROTO) return;
    while ((gline = fgetline(SFI_PROTO)) != 0) {
        (void) fputs(gline, SFPROTO);
        free(gline);
    }
    (void) fclose(SFI_PROTO);
    (void) remove(filenmbuf);

    Snprintf(filenmbuf, sizeof filenmbuf, "%s%s", folderprefix,
             "../util/sfdata.tmp");
    SFDATATMP = fopen(filenmbuf, "r");
    if (!SFDATATMP) return;
    while ((gline = fgetline(SFDATATMP)) != 0) {
        (void) fputs(gline, SFDATA);
        free(gline);
    }
    (void) fclose(SFDATATMP);
    (void) remove(filenmbuf);

    Snprintf(filenmbuf, sizeof filenmbuf, "%s%s", folderprefix,
             "../util/sfnormptrs.tmp");
    SF_NORMALIZE_POINTERS = fopen(filenmbuf, "r");
    if (!SF_NORMALIZE_POINTERS)
        return;
    while ((gline = fgetline(SF_NORMALIZE_POINTERS)) != 0) {
        (void) fputs(gline, SFDATA);
        free(gline);
    }
    (void) fclose(SF_NORMALIZE_POINTERS);
    (void) remove(filenmbuf);

    Fprintf(SFDATA, "/*sfdata.c*/\n");
    Fprintf(SFPROTO,"#endif /* SFPROTO_H */\n");
    (void) fclose(SFDATA);
    (void) fclose(SFPROTO);
}

#if 0
static char *
dtmacro(const char *str,
        int n)     /* 1 = supress appending |SF_PTRMASK */
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
           (strncmpi(c, "struct    ", 7) == 0)) {
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
#endif

static char *
dtfn(const char *str,
     int n,     /* 1 = supress appending |SF_PTRMASK */
     boolean *isptr)
{
    static char buf[128], buf2[128];
    const char *nam;
    char *c;
    int ispointer = 0;

    if (!str)
        return (char *)0;
    (void)strncpy(buf, str, 127);
    buf[sizeof buf - 1] = '\0';

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
                   (strncmpi(c, "struct    ", 7) == 0)) {
        c = buf + 7;
    } else if (strncmpi(c, "union ", 6) == 0) {
        c = buf + 6;
    }

    /* end of substruct within struct definition */
    if (strcmp(buf,"}") == 0 && strlen(ssdef) > 0) {
        strcpy(buf,ssdef);
        c = buf;
    }

    for (nam = (const char *) c; *c; c++) {
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
fieldfix(char *f, char *ss)
{
    char *c /*, *dest = fieldfixbuf */;

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
bfsize(const char *str)
{
    static char buf[128];
    char *copy_str, *c1, *c2, *subst;
    char *retval = buf;

    if (!str)
        return (char *)0;

    copy_str = dupstr(str);
    /* kludge */
    subst = strstr(copy_str, ",$/");
    if (subst != 0) {
        subst++;
        *subst++ = ' ';
        *subst++ = '1';
    }

    c2 = buf;
    c1 = copy_str;
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
        retval = (char *) 0; 
    }
    free((genericptr_t) copy_str);
    return retval;
}

/* Read one line from input, up to and including the next newline
 * character. Returns a pointer to the heap-allocated string, or a
 * null pointer if no characters were read.
 */
static char *
fgetline(FILE *fd)
{
    static const int inc = 256;
    int len = inc;
    char *c = malloc(len), *ret, *loc, *avoidleak;

    if (!c) {
        fprintf(stderr, "Memory issue\n");
        quit();
    }
    for (;;) {
        loc = c + len - inc;
        if (!loc) {
            quit();
        }
        ret = fgets(loc, inc, fd);
        if (!ret) {
            free(c);
            c = NULL;
            break;
        } else if (strchr(c, '\n')) {
            /* normal case: we have a full line */
            break;
        }
        len += inc;
        avoidleak = c;
        c = realloc(c, len);
        if (!c) {
            free(avoidleak);
            quit();
        }
    }
    return c;
}

#ifndef _MSC_VER
int
strncmpi(const char *s1, const char *s2, size_t n)
{
    register char t1, t2;

    while (n--) {
        if (!*s2)
            return (*s1 != 0); /* s1 >= s2 */
        else if (!*s1)
            return -1; /* s1  < s2 */
        t1 = lowc(*s1++);
        t2 = lowc(*s2++);
        if (t1 != t2)
            return (t1 > t2) ? 1 : -1;
    }
    return 0; /* s1 == s2 */
}
#endif


struct already_in_sfbase {
    int typ;
    const char *actual;
    const char *replacement;
};

/* The ones that can't be broken down into subfields
 * are NHTYPE_SIMPLE.
 */
const struct already_in_sfbase already[] = {
    /* input */
    { NHTYPE_SIMPLE, "Sfi_any", "Sfi_any" },
    { NHTYPE_SIMPLE, "Sfi_boolean", "Sfi_boolean" },
    { NHTYPE_SIMPLE, "Sfi_char", "Sfi_char" },
    { NHTYPE_SIMPLE, "Sfi_coordxy", "Sfi_coordxy" },
    { NHTYPE_SIMPLE, "Sfi_int16", "Sfi_int16" },
    { NHTYPE_SIMPLE, "Sfi_int32", "Sfi_int32" },
    { NHTYPE_SIMPLE, "Sfi_long", "Sfi_long" },
    { NHTYPE_SIMPLE, "Sfi_nhcoord", "Sfi_nhcoord" },
    { NHTYPE_SIMPLE, "Sfi_schar", "Sfi_schar" },
    { NHTYPE_SIMPLE, "Sfi_uint32", "Sfi_uint32" },
    { NHTYPE_SIMPLE, "Sfi_ulong", "Sfi_ulong" },
    { NHTYPE_SIMPLE, "Sfi_xint8", "Sfi_xint8" },
    { NHTYPE_COMPLEX, "Sfi_arti_info", "Sfi_x_arti_info" },
    { NHTYPE_COMPLEX, "Sfi_branch", "Sfi_x_branch" },
    { NHTYPE_COMPLEX, "Sfi_bubble", "Sfi_x_bubble" },
    { NHTYPE_COMPLEX, "Sfi_cemetery", "Sfi_x_cemetery" },
    { NHTYPE_COMPLEX, "Sfi_context_info", "Sfi_x_context_info" },
    { NHTYPE_COMPLEX, "Sfi_d_level", "Sfi_x_d_level" },
    { NHTYPE_COMPLEX, "Sfi_damage", "Sfi_x_damage" },
    { NHTYPE_COMPLEX, "Sfi_dest_area", "Sfi_x_dest_area" },
    { NHTYPE_COMPLEX, "Sfi_dgn_topology", "Sfi_x_dgn_topology" },
    { NHTYPE_COMPLEX, "Sfi_dungeon", "Sfi_x_dungeon" },
    { NHTYPE_COMPLEX, "Sfi_ebones", "Sfi_x_ebones" },
    { NHTYPE_COMPLEX, "Sfi_edog", "Sfi_x_edog" },
    { NHTYPE_COMPLEX, "Sfi_egd", "Sfi_x_egd" },
    { NHTYPE_COMPLEX, "Sfi_emin", "Sfi_x_emin" },
    { NHTYPE_COMPLEX, "Sfi_engr", "Sfi_x_engr" },
    { NHTYPE_COMPLEX, "Sfi_epri", "Sfi_x_epri" },
    { NHTYPE_COMPLEX, "Sfi_eshk", "Sfi_x_eshk" },
    { NHTYPE_COMPLEX, "Sfi_fe", "Sfi_x_fe" },
    { NHTYPE_COMPLEX, "Sfi_flag", "Sfi_x_flag" },
    { NHTYPE_COMPLEX, "Sfi_fruit", "Sfi_x_fruit" },
    { NHTYPE_COMPLEX, "Sfi_gamelog_line", "Sfi_x_gamelog_line" },
    { NHTYPE_COMPLEX, "Sfi_kinfo", "Sfi_x_kinfo" },
    { NHTYPE_COMPLEX, "Sfi_levelflags", "Sfi_x_levelflags" },
    { NHTYPE_COMPLEX, "Sfi_linfo", "Sfi_x_linfo" },
    { NHTYPE_COMPLEX, "Sfi_ls_t", "Sfi_x_ls_t" },
    { NHTYPE_COMPLEX, "Sfi_mapseen_feat", "Sfi_x_mapseen_feat" },
    { NHTYPE_COMPLEX, "Sfi_mapseen_flags", "Sfi_x_mapseen_flags" },
    { NHTYPE_COMPLEX, "Sfi_mapseen_rooms", "Sfi_x_mapseen_rooms" },
    { NHTYPE_COMPLEX, "Sfi_mkroom", "Sfi_x_mkroom" },
    { NHTYPE_COMPLEX, "Sfi_monst", "Sfi_x_monst" },
    { NHTYPE_COMPLEX, "Sfi_mvitals", "Sfi_x_mvitals" },
    { NHTYPE_COMPLEX, "Sfi_nhrect", "Sfi_x_nhrect" },
    { NHTYPE_COMPLEX, "Sfi_obj", "Sfi_x_obj" },
    { NHTYPE_COMPLEX, "Sfi_objclass", "Sfi_x_objclass" },
    { NHTYPE_COMPLEX, "Sfi_q_score", "Sfi_x_q_score" },
    { NHTYPE_COMPLEX, "Sfi_rm", "Sfi_x_rm" },
    { NHTYPE_COMPLEX, "Sfi_s_level", "Sfi_x_s_level" },
    { NHTYPE_COMPLEX, "Sfi_spell", "Sfi_x_spell" },
    { NHTYPE_COMPLEX, "Sfi_stairway", "Sfi_x_stairway" },
    { NHTYPE_COMPLEX, "Sfi_trap", "Sfi_x_trap" },
    { NHTYPE_COMPLEX, "Sfi_version_info", "Sfi_x_version_info" },
    { NHTYPE_COMPLEX, "Sfi_you", "Sfi_x_you" },
    /* output */
    { NHTYPE_SIMPLE, "Sfo_any", "Sfo_any" },
    { NHTYPE_SIMPLE, "Sfo_boolean", "Sfo_boolean" },
    { NHTYPE_SIMPLE, "Sfo_char", "Sfo_char" },
    { NHTYPE_SIMPLE, "Sfo_coordxy", "Sfo_coordxy" },
    { NHTYPE_SIMPLE, "Sfo_int16", "Sfo_int16" },
    { NHTYPE_SIMPLE, "Sfo_int32", "Sfo_int32" },
    { NHTYPE_SIMPLE, "Sfo_long", "Sfo_long" },
    { NHTYPE_SIMPLE, "Sfo_nhcoord", "Sfo_nhcoord" },
    { NHTYPE_SIMPLE, "Sfo_schar", "Sfo_schar" },
    { NHTYPE_SIMPLE, "Sfo_uint32", "Sfo_uint32" },
    { NHTYPE_SIMPLE, "Sfo_ulong", "Sfo_ulong" },
    { NHTYPE_SIMPLE, "Sfo_xint8", "Sfo_xint8" },
    { NHTYPE_COMPLEX, "Sfo_arti_info", "Sfo_x_arti_info" },
    { NHTYPE_COMPLEX, "Sfo_branch", "Sfo_x_branch" },
    { NHTYPE_COMPLEX, "Sfo_bubble", "Sfo_x_bubble" },
    { NHTYPE_COMPLEX, "Sfo_cemetery", "Sfo_x_cemetery" },
    { NHTYPE_COMPLEX, "Sfo_context_info", "Sfo_x_context_info" },
    { NHTYPE_COMPLEX, "Sfo_d_level", "Sfo_x_d_level" },
    { NHTYPE_COMPLEX, "Sfo_damage", "Sfo_x_damage" },
    { NHTYPE_COMPLEX, "Sfo_dest_area", "Sfo_x_dest_area" },
    { NHTYPE_COMPLEX, "Sfo_dgn_topology", "Sfo_x_dgn_topology" },
    { NHTYPE_COMPLEX, "Sfo_dungeon", "Sfo_x_dungeon" },
    { NHTYPE_COMPLEX, "Sfo_ebones", "Sfo_x_ebones" },
    { NHTYPE_COMPLEX, "Sfo_edog", "Sfo_x_edog" },
    { NHTYPE_COMPLEX, "Sfo_egd", "Sfo_x_egd" },
    { NHTYPE_COMPLEX, "Sfo_emin", "Sfo_x_emin" },
    { NHTYPE_COMPLEX, "Sfo_engr", "Sfo_x_engr" },
    { NHTYPE_COMPLEX, "Sfo_epri", "Sfo_x_epri" },
    { NHTYPE_COMPLEX, "Sfo_eshk", "Sfo_x_eshk" },
    { NHTYPE_COMPLEX, "Sfo_fe", "Sfo_x_fe" },
    { NHTYPE_COMPLEX, "Sfo_flag", "Sfo_x_flag" },
    { NHTYPE_COMPLEX, "Sfo_fruit", "Sfo_x_fruit" },
    { NHTYPE_COMPLEX, "Sfo_gamelog_line", "Sfo_x_gamelog_line" },
    { NHTYPE_COMPLEX, "Sfo_kinfo", "Sfo_x_kinfo" },
    { NHTYPE_COMPLEX, "Sfo_levelflags", "Sfo_x_levelflags" },
    { NHTYPE_COMPLEX, "Sfo_linfo", "Sfo_x_linfo" },
    { NHTYPE_COMPLEX, "Sfo_ls_t", "Sfo_x_ls_t" },
    { NHTYPE_COMPLEX, "Sfo_mapseen_feat", "Sfo_x_mapseen_feat" },
    { NHTYPE_COMPLEX, "Sfo_mapseen_flags", "Sfo_x_mapseen_flags" },
    { NHTYPE_COMPLEX, "Sfo_mapseen_rooms", "Sfo_x_mapseen_rooms" },
    { NHTYPE_COMPLEX, "Sfo_mkroom", "Sfo_x_mkroom" },
    { NHTYPE_COMPLEX, "Sfo_monst", "Sfo_x_monst" },
    { NHTYPE_COMPLEX, "Sfo_mvitals", "Sfo_x_mvitals" },
    { NHTYPE_COMPLEX, "Sfo_nhrect", "Sfo_x_nhrect" },
    { NHTYPE_COMPLEX, "Sfo_obj", "Sfo_x_obj" },
    { NHTYPE_COMPLEX, "Sfo_objclass", "Sfo_x_objclass" },
    { NHTYPE_COMPLEX, "Sfo_q_score", "Sfo_x_q_score" },
    { NHTYPE_COMPLEX, "Sfo_rm", "Sfo_x_rm" },
    { NHTYPE_COMPLEX, "Sfo_s_level", "Sfo_x_s_level" },
    { NHTYPE_COMPLEX, "Sfo_spell", "Sfo_x_spell" },
    { NHTYPE_COMPLEX, "Sfo_stairway", "Sfo_x_stairway" },
    { NHTYPE_COMPLEX, "Sfo_trap", "Sfo_x_trap" },
    { NHTYPE_COMPLEX, "Sfo_version_info", "Sfo_x_version_info" },
    { NHTYPE_COMPLEX, "Sfo_you", "Sfo_x_you" },
};

static const char *
fn(const char *f)
{
    int i;

    for (i = 0; i < SIZE(already); ++i) {
        if (!strcmp(already[i].actual, f))
            return already[i].replacement;
    }
    return f;
}

const char *force_no_x[] = {
    "aligntyp", "any",      "boolean", "char",   "coordxy", "genericptr",
    "int",      "int16",    "int32",   "int64",  "long",    "schar",
    "short",    "size_t",   "time_t",  "uchar",  "uint32",  "uint64",
    "ulong",    "unsigned", "ushort",  "xint16", "xint8",
};

static boolean
no_x(const char *s)
{
    int i;

    for (i = 0; i < SIZE(force_no_x); ++i) {
        if (!strcmp(force_no_x[i], s))
            return TRUE;
    }
    return FALSE;
}

struct nh_classification {
    int in_sfbase;
    uint dtclass;
    const char *fn;
    const char *replacement_fn;
};

struct nh_classification nhdatatypes[] = {
    { 0, NHTYPE_COMPLEX, "sfi_achievement_tracking",
      "sfi_achievement_tracking" },
    { 0, NHTYPE_COMPLEX, "sfi_align", "sfi_align" },
    { 0, NHTYPE_COMPLEX, "sfi_attribs", "sfi_attribs" },
    { 0, NHTYPE_COMPLEX, "sfi_bill_x", "sfi_bill_x" },
    { 0, NHTYPE_COMPLEX, "sfi_book_info", "sfi_book_info" },
    { 0, NHTYPE_COMPLEX, "sfi_branch", "sfi_branch" },
    { 0, NHTYPE_COMPLEX, "sfi_bubble", "sfi_bubble" },
    { 0, NHTYPE_COMPLEX, "sfi_cemetery", "sfi_cemetery" },
    { 0, NHTYPE_COMPLEX, "sfi_context_info", "sfi_context_info" },
    { 0, NHTYPE_COMPLEX, "sfi_d_flags", "sfi_d_flags" },
    { 0, NHTYPE_COMPLEX, "sfi_d_level", "sfi_d_level" },
    { 0, NHTYPE_COMPLEX, "sfi_damage", "sfi_damage" },
    { 0, NHTYPE_COMPLEX, "sfi_dest_area", "sfi_dest_area" },
    { 0, NHTYPE_COMPLEX, "sfi_dgn_topology", "sfi_dgn_topology" },
    { 0, NHTYPE_COMPLEX, "sfi_dig_info", "sfi_dig_info" },
    { 0, NHTYPE_COMPLEX, "sfi_dungeon", "sfi_dungeon" },
    { 0, NHTYPE_COMPLEX, "sfi_ebones", "sfi_ebones" },
    { 0, NHTYPE_COMPLEX, "sfi_edog", "sfi_edog" },
    { 0, NHTYPE_COMPLEX, "sfi_egd", "sfi_egd" },
    { 0, NHTYPE_COMPLEX, "sfi_emin", "sfi_emin" },
    { 0, NHTYPE_COMPLEX, "sfi_engr", "sfi_engr" },
    { 0, NHTYPE_COMPLEX, "sfi_engrave_info", "sfi_engrave_info" },
    { 0, NHTYPE_COMPLEX, "sfi_epri", "sfi_epri" },
    { 0, NHTYPE_COMPLEX, "sfi_eshk", "sfi_eshk" },
    { 0, NHTYPE_COMPLEX, "sfi_fakecorridor", "sfi_fakecorridor" },
    { 0, NHTYPE_COMPLEX, "sfi_fe", "sfi_fe" },
    { 0, NHTYPE_COMPLEX, "sfi_flag", "sfi_flag" },
    { 0, NHTYPE_COMPLEX, "sfi_fruit", "sfi_fruit" },
    { 0, NHTYPE_COMPLEX, "sfi_kinfo", "sfi_kinfo" },
    { 0, NHTYPE_COMPLEX, "sfi_levelflags", "sfi_levelflags" },
    { 0, NHTYPE_COMPLEX, "sfi_linfo", "sfi_linfo" },
    { 0, NHTYPE_COMPLEX, "sfi_ls_t", "sfi_ls_t" },
    { 0, NHTYPE_COMPLEX, "sfi_mapseen", "sfi_mapseen" },
    { 0, NHTYPE_COMPLEX, "sfi_mapseen_feat", "sfi_mapseen_feat" },
    { 0, NHTYPE_COMPLEX, "sfi_mapseen_flags", "sfi_mapseen_flags" },
    { 0, NHTYPE_COMPLEX, "sfi_mapseen_rooms", "sfi_mapseen_rooms" },
    { 0, NHTYPE_COMPLEX, "sfi_mextra", "sfi_mextra" },
    { 0, NHTYPE_COMPLEX, "sfi_mkroom", "sfi_mkroom" },
    { 0, NHTYPE_COMPLEX, "sfi_monst", "sfi_monst" },
    { 0, NHTYPE_COMPLEX, "sfi_mvitals", "sfi_mvitals" },
    { 0, NHTYPE_COMPLEX, "sfi_nhcoord", "sfi_nhcoord" },
    { 0, NHTYPE_COMPLEX, "sfi_nhrect", "sfi_nhrect" },
    { 0, NHTYPE_COMPLEX, "sfi_novel_tracking", "sfi_novel_tracking" },
    { 0, NHTYPE_COMPLEX, "sfi_obj", "sfi_obj" },
    { 0, NHTYPE_COMPLEX, "sfi_obj_split", "sfi_obj_split" },
    { 0, NHTYPE_COMPLEX, "sfi_objclass", "sfi_objclass" },
    { 0, NHTYPE_COMPLEX, "sfi_oextra", "sfi_oextra" },
    { 0, NHTYPE_COMPLEX, "sfi_polearm_info", "sfi_polearm_info" },
    { 0, NHTYPE_COMPLEX, "sfi_prop", "sfi_prop" },
    { 0, NHTYPE_COMPLEX, "sfi_q_score", "sfi_q_score" },
    { 0, NHTYPE_COMPLEX, "sfi_rm", "sfi_rm" },
    { 0, NHTYPE_COMPLEX, "sfi_s_level", "sfi_s_level" },
    { 0, NHTYPE_COMPLEX, "sfi_skills", "sfi_skills" },
    { 0, NHTYPE_COMPLEX, "sfi_spell", "sfi_spell" },
    { 0, NHTYPE_COMPLEX, "sfi_stairway", "sfi_stairway" },
    { 0, NHTYPE_COMPLEX, "sfi_takeoff_info", "sfi_takeoff_info" },
    { 0, NHTYPE_COMPLEX, "sfi_tin_info", "sfi_tin_info" },
    { 0, NHTYPE_COMPLEX, "sfi_trap", "sfi_trap" },
    { 0, NHTYPE_COMPLEX, "sfi_tribute_info", "sfi_tribute_info" },
    { 0, NHTYPE_COMPLEX, "sfi_u_conduct", "sfi_u_conduct" },
    { 0, NHTYPE_COMPLEX, "sfi_u_event", "sfi_u_event" },
    { 0, NHTYPE_COMPLEX, "sfi_u_have", "sfi_u_have" },
    { 0, NHTYPE_COMPLEX, "sfi_u_realtime", "sfi_u_realtime" },
    { 0, NHTYPE_COMPLEX, "sfi_u_roleplay", "sfi_u_roleplay" },
    { 0, NHTYPE_COMPLEX, "sfi_version_info", "sfi_version_info" },
    { 0, NHTYPE_COMPLEX, "sfi_victual_info", "sfi_victual_info" },
    { 0, NHTYPE_COMPLEX, "sfi_vlaunchinfo", "sfi_vlaunchinfo" },
    { 0, NHTYPE_COMPLEX, "sfi_vptrs", "sfi_vptrs" },
    { 0, NHTYPE_COMPLEX, "sfi_warntype_info", "sfi_warntype_info" },
    { 0, NHTYPE_COMPLEX, "sfi_you", "sfi_you" },
    { 0, NHTYPE_COMPLEX, "sfo_achievement_tracking",
      "sfo_achievement_tracking" },
    { 0, NHTYPE_COMPLEX, "sfo_align", "sfo_align" },
    { 0, NHTYPE_COMPLEX, "sfo_attribs", "sfo_attribs" },
    { 0, NHTYPE_COMPLEX, "sfo_bill_x", "sfo_bill_x" },
    { 0, NHTYPE_COMPLEX, "sfo_book_info", "sfo_book_info" },
    { 0, NHTYPE_COMPLEX, "sfo_branch", "sfo_branch" },
    { 0, NHTYPE_COMPLEX, "sfo_bubble", "sfo_bubble" },
    { 0, NHTYPE_COMPLEX, "sfo_cemetery", "sfo_cemetery" },
    { 0, NHTYPE_COMPLEX, "sfo_context_info", "sfo_context_info" },
    { 0, NHTYPE_COMPLEX, "sfo_d_flags", "sfo_d_flags" },
    { 0, NHTYPE_COMPLEX, "sfo_d_level", "sfo_d_level" },
    { 0, NHTYPE_COMPLEX, "sfo_damage", "sfo_damage" },
    { 0, NHTYPE_COMPLEX, "sfo_dest_area", "sfo_dest_area" },
    { 0, NHTYPE_COMPLEX, "sfo_dgn_topology", "sfo_dgn_topology" },
    { 0, NHTYPE_COMPLEX, "sfo_dig_info", "sfo_dig_info" },
    { 0, NHTYPE_COMPLEX, "sfo_dungeon", "sfo_dungeon" },
    { 0, NHTYPE_COMPLEX, "sfo_ebones", "sfo_ebones" },
    { 0, NHTYPE_COMPLEX, "sfo_edog", "sfo_edog" },
    { 0, NHTYPE_COMPLEX, "sfo_egd", "sfo_egd" },
    { 0, NHTYPE_COMPLEX, "sfo_emin", "sfo_emin" },
    { 0, NHTYPE_COMPLEX, "sfo_engr", "sfo_engr" },
    { 0, NHTYPE_COMPLEX, "sfo_engrave_info", "sfo_engrave_info" },
    { 0, NHTYPE_COMPLEX, "sfo_epri", "sfo_epri" },
    { 0, NHTYPE_COMPLEX, "sfo_eshk", "sfo_eshk" },
    { 0, NHTYPE_COMPLEX, "sfo_fakecorridor", "sfo_fakecorridor" },
    { 0, NHTYPE_COMPLEX, "sfo_fe", "sfo_fe" },
    { 0, NHTYPE_COMPLEX, "sfo_flag", "sfo_flag" },
    { 0, NHTYPE_COMPLEX, "sfo_fruit", "sfo_fruit" },
    { 0, NHTYPE_COMPLEX, "sfo_kinfo", "sfo_kinfo" },
    { 0, NHTYPE_COMPLEX, "sfo_levelflags", "sfo_levelflags" },
    { 0, NHTYPE_COMPLEX, "sfo_linfo", "sfo_linfo" },
    { 0, NHTYPE_COMPLEX, "sfo_ls_t", "sfo_ls_t" },
    { 0, NHTYPE_COMPLEX, "sfo_mapseen", "sfo_mapseen" },
    { 0, NHTYPE_COMPLEX, "sfo_mapseen_feat", "sfo_mapseen_feat" },
    { 0, NHTYPE_COMPLEX, "sfo_mapseen_flags", "sfo_mapseen_flags" },
    { 0, NHTYPE_COMPLEX, "sfo_mapseen_rooms", "sfo_mapseen_rooms" },
    { 0, NHTYPE_COMPLEX, "sfo_mextra", "sfo_mextra" },
    { 0, NHTYPE_COMPLEX, "sfo_mkroom", "sfo_mkroom" },
    { 0, NHTYPE_COMPLEX, "sfo_monst", "sfo_monst" },
    { 0, NHTYPE_COMPLEX, "sfo_mvitals", "sfo_mvitals" },
    { 0, NHTYPE_COMPLEX, "sfo_nhcoord", "sfo_nhcoord" },
    { 0, NHTYPE_COMPLEX, "sfo_nhrect", "sfo_nhrect" },
    { 0, NHTYPE_COMPLEX, "sfo_novel_tracking", "sfo_novel_tracking" },
    { 0, NHTYPE_COMPLEX, "sfo_obj", "sfo_obj" },
    { 0, NHTYPE_COMPLEX, "sfo_obj_split", "sfo_obj_split" },
    { 0, NHTYPE_COMPLEX, "sfo_objclass", "sfo_objclass" },
    { 0, NHTYPE_COMPLEX, "sfo_oextra", "sfo_oextra" },
    { 0, NHTYPE_COMPLEX, "sfo_polearm_info", "sfo_polearm_info" },
    { 0, NHTYPE_COMPLEX, "sfo_prop", "sfo_prop" },
    { 0, NHTYPE_COMPLEX, "sfo_q_score", "sfo_q_score" },
    { 0, NHTYPE_COMPLEX, "sfo_rm", "sfo_rm" },
    { 0, NHTYPE_COMPLEX, "sfo_s_level", "sfo_s_level" },
    { 0, NHTYPE_COMPLEX, "sfo_skills", "sfo_skills" },
    { 0, NHTYPE_COMPLEX, "sfo_spell", "sfo_spell" },
    { 0, NHTYPE_COMPLEX, "sfo_stairway", "sfo_stairway" },
    { 0, NHTYPE_COMPLEX, "sfo_takeoff_info", "sfo_takeoff_info" },
    { 0, NHTYPE_COMPLEX, "sfo_tin_info", "sfo_tin_info" },
    { 0, NHTYPE_COMPLEX, "sfo_trap", "sfo_trap" },
    { 0, NHTYPE_COMPLEX, "sfo_tribute_info", "sfo_tribute_info" },
    { 0, NHTYPE_COMPLEX, "sfo_u_conduct", "sfo_u_conduct" },
    { 0, NHTYPE_COMPLEX, "sfo_u_event", "sfo_u_event" },
    { 0, NHTYPE_COMPLEX, "sfo_u_have", "sfo_u_have" },
    { 0, NHTYPE_COMPLEX, "sfo_u_realtime", "sfo_u_realtime" },
    { 0, NHTYPE_COMPLEX, "sfo_u_roleplay", "sfo_u_roleplay" },
    { 0, NHTYPE_COMPLEX, "sfo_version_info", "sfo_version_info" },
    { 0, NHTYPE_COMPLEX, "sfo_victual_info", "sfo_victual_info" },
    { 0, NHTYPE_COMPLEX, "sfo_vlaunchinfo", "sfo_vlaunchinfo" },
    { 0, NHTYPE_COMPLEX, "sfo_vptrs", "sfo_vptrs" },
    { 0, NHTYPE_COMPLEX, "sfo_warntype_info", "sfo_warntype_info" },
    { 0, NHTYPE_COMPLEX, "sfo_you", "sfo_you" },
    { 0, NHTYPE_SIMPLE, "sfi_any", "sfi_any" },
    { 0, NHTYPE_SIMPLE, "sfi_aligntyp", "Sfi_aligntyp" },
    { 0, NHTYPE_SIMPLE, "sfi_Bitfield", "Sfi_Bitfield" },
    { 0, NHTYPE_SIMPLE, "sfi_boolean", "Sfi_boolean" },
    { 0, NHTYPE_SIMPLE, "sfi_char", "Sfi_char" },
    { 0, NHTYPE_SIMPLE, "sfi_coordxy", "Sfi_Coordxy" },
    { 0, NHTYPE_SIMPLE, "sfi_int", "Sfi_int32" },
    { 0, NHTYPE_SIMPLE, "sfi_long", "Sfi_long" },
    { 0, NHTYPE_SIMPLE, "sfi_schar", "Sfi_schar" },
    { 0, NHTYPE_SIMPLE, "sfi_short", "Sfi_int16" },
    { 0, NHTYPE_SIMPLE, "sfi_size_t", "Sfi_size_t" },
    { 0, NHTYPE_SIMPLE, "sfi_string", "Sfi_string" },
    { 0, NHTYPE_SIMPLE, "sfi_time_t", "Sfi_time_t" },
    { 0, NHTYPE_SIMPLE, "sfi_uchar", "Sfi_uchar" },
    { 0, NHTYPE_SIMPLE, "sfi_unsigned", "Sfi_uint32" },
    { 0, NHTYPE_SIMPLE, "sfi_xint16", "Sfi_int16" },
    { 0, NHTYPE_SIMPLE, "sfi_xint8", "Sfi_xint8" },
    { 0, NHTYPE_SIMPLE, "sfo_aligntyp", "Sfo_aligntyp" },
    { 0, NHTYPE_SIMPLE, "sfo_any", "Sfo_any" },
    { 0, NHTYPE_SIMPLE, "sfo_Bitfield", "Sfo_Bitfield" },
    { 0, NHTYPE_SIMPLE, "sfo_boolean", "Sfo_boolean" },
    { 0, NHTYPE_SIMPLE, "sfo_char", "Sfo_char" },
    { 0, NHTYPE_SIMPLE, "sfo_coordxy", "Sfo_coordxy" },
    { 0, NHTYPE_SIMPLE, "sfo_int", "Sfo_int32" },
    { 0, NHTYPE_SIMPLE, "sfo_long", "sfo_long" },
    { 0, NHTYPE_SIMPLE, "sfo_schar", "Sfo_schar" },
    { 0, NHTYPE_SIMPLE, "sfo_short", "Sfo_short" },
    { 0, NHTYPE_SIMPLE, "sfo_size_t", "Sfo_size_t" },
    { 0, NHTYPE_SIMPLE, "sfo_string", "Sfo_string" },
    { 0, NHTYPE_SIMPLE, "sfo_time_t", "Sfo_time_t" },
    { 0, NHTYPE_SIMPLE, "sfo_uchar", "Sfo_uchar" },
    { 0, NHTYPE_SIMPLE, "sfo_unsigned", "Sfo_uint32" },
    { 0, NHTYPE_SIMPLE, "sfo_xint16", "Sfo_int16" },
    { 0, NHTYPE_SIMPLE, "sfo_xint8", "Sfo_xint8" }
};

    /*sftags.c*/


