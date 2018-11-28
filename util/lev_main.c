/* NetHack 3.6	lev_main.c	$NHDT-Date: 1543371692 2018/11/28 02:21:32 $  $NHDT-Branch: NetHack-3.6.2-beta01 $:$NHDT-Revision: 1.56 $ */
/*      Copyright (c) 1989 by Jean-Christophe Collet */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file contains the main function for the parser
 * and some useful functions needed by yacc
 */
#define SPEC_LEV /* for USE_OLDARGS (sp_lev.h) and for MPW (macconf.h) */

#define NEED_VARARGS
#include "hack.h"
#include "date.h"
#include "sp_lev.h"
#ifdef STRICT_REF_DEF
#include "tcap.h"
#endif
#include <ctype.h>

#ifdef MAC
#if defined(__SC__) || defined(__MRC__)
#define MPWTOOL
#define PREFIX ":dungeon:" /* place output files here */
#include <CursorCtl.h>
#else
#if !defined(__MACH__)
#define PREFIX ":lib:" /* place output files here */
#endif
#endif
#endif

#ifdef WIN_CE
#define PREFIX "\\nethack\\dat\\"
#endif

#ifndef MPWTOOL
#define SpinCursor(x)
#endif

#if defined(AMIGA) && defined(DLB)
#define PREFIX "NH:slib/"
#endif

#ifndef O_WRONLY
#include <fcntl.h>
#endif
#ifndef O_CREAT /* some older BSD systems do not define O_CREAT in <fcntl.h> \
                   */
#include <sys/file.h>
#endif
#ifndef O_BINARY /* used for micros, no-op for others */
#define O_BINARY 0
#endif

#if defined(MICRO) || defined(WIN32)
#define OMASK FCMASK
#else
#define OMASK 0644
#endif

#define ERR (-1)

#define NewTab(type, size) (type **) alloc(sizeof(type *) * size)
#define Free(ptr) \
    if (ptr)      \
        free((genericptr_t)(ptr))
/* write() returns a signed value but its size argument is unsigned;
   types might be int and unsigned or ssize_t and size_t; casting
   to long should be safe for all permutations (even if size_t is
   bigger, since the values involved won't be too big for long) */
#define Write(fd, item, size)                                          \
    if ((long) write(fd, (genericptr_t)(item), size) != (long) (size)) \
        return FALSE;

#if defined(__BORLANDC__) && !defined(_WIN32)
extern unsigned _stklen = STKSIZ;
#endif
#define MAX_ERRORS 25

extern int NDECL(yyparse);
extern void FDECL(init_yyin, (FILE *));
extern void FDECL(init_yyout, (FILE *));

int FDECL(main, (int, char **));
void FDECL(yyerror, (const char *));
void FDECL(yywarning, (const char *));
int NDECL(yywrap);
int FDECL(get_floor_type, (CHAR_P));
int FDECL(get_room_type, (char *));
int FDECL(get_trap_type, (char *));
int FDECL(get_monster_id, (char *, CHAR_P));
int FDECL(get_object_id, (char *, CHAR_P));
boolean FDECL(check_monster_char, (CHAR_P));
boolean FDECL(check_object_char, (CHAR_P));
char FDECL(what_map_char, (CHAR_P));
void FDECL(scan_map, (char *, sp_lev *));
boolean NDECL(check_subrooms);
boolean FDECL(write_level_file, (char *, sp_lev *));

struct lc_funcdefs *FDECL(funcdef_new, (long, char *));
void FDECL(funcdef_free_all, (struct lc_funcdefs *));
struct lc_funcdefs *FDECL(funcdef_defined,
                          (struct lc_funcdefs *, char *, int));

struct lc_vardefs *FDECL(vardef_new, (long, char *));
void FDECL(vardef_free_all, (struct lc_vardefs *));
struct lc_vardefs *FDECL(vardef_defined, (struct lc_vardefs *, char *, int));

void FDECL(splev_add_from, (sp_lev *, sp_lev *));

extern void NDECL(monst_init);
extern void NDECL(objects_init);
extern void NDECL(decl_init);

void FDECL(add_opcode, (sp_lev *, int, genericptr_t));

static boolean FDECL(write_common_data, (int));
static boolean FDECL(write_maze, (int, sp_lev *));
static void NDECL(init_obj_classes);
static int FDECL(case_insensitive_comp, (const char *, const char *));

void VDECL(lc_pline, (const char *, ...));
void VDECL(lc_error, (const char *, ...));
void VDECL(lc_warning, (const char *, ...));
char *FDECL(decode_parm_chr, (CHAR_P));
char *FDECL(decode_parm_str, (char *));
struct opvar *FDECL(set_opvar_int, (struct opvar *, long));
struct opvar *FDECL(set_opvar_coord, (struct opvar *, long));
struct opvar *FDECL(set_opvar_region, (struct opvar *, long));
struct opvar *FDECL(set_opvar_mapchar, (struct opvar *, long));
struct opvar *FDECL(set_opvar_monst, (struct opvar *, long));
struct opvar *FDECL(set_opvar_obj, (struct opvar *, long));
struct opvar *FDECL(set_opvar_str, (struct opvar *, const char *));
struct opvar *FDECL(set_opvar_var, (struct opvar *, const char *));
void VDECL(add_opvars, (sp_lev *, const char *, ...));
void NDECL(break_stmt_start);
void FDECL(break_stmt_end, (sp_lev *));
void FDECL(break_stmt_new, (sp_lev *, long));
char *FDECL(funcdef_paramtypes, (struct lc_funcdefs *));
const char *FDECL(spovar2str, (long));
void FDECL(vardef_used, (struct lc_vardefs *, char *));
void FDECL(check_vardef_type, (struct lc_vardefs *, char *, long));
struct lc_vardefs *FDECL(add_vardef_type,
                         (struct lc_vardefs *, char *, long));
int FDECL(reverse_jmp_opcode, (int));
struct opvar *FDECL(opvar_clone, (struct opvar *));
void FDECL(start_level_def, (sp_lev **, char *));

static struct {
    const char *name;
    int type;
} trap_types[] = { { "arrow", ARROW_TRAP },
                   { "dart", DART_TRAP },
                   { "falling rock", ROCKTRAP },
                   { "board", SQKY_BOARD },
                   { "bear", BEAR_TRAP },
                   { "land mine", LANDMINE },
                   { "rolling boulder", ROLLING_BOULDER_TRAP },
                   { "sleep gas", SLP_GAS_TRAP },
                   { "rust", RUST_TRAP },
                   { "fire", FIRE_TRAP },
                   { "pit", PIT },
                   { "spiked pit", SPIKED_PIT },
                   { "hole", HOLE },
                   { "trap door", TRAPDOOR },
                   { "teleport", TELEP_TRAP },
                   { "level teleport", LEVEL_TELEP },
                   { "magic portal", MAGIC_PORTAL },
                   { "web", WEB },
                   { "statue", STATUE_TRAP },
                   { "magic", MAGIC_TRAP },
                   { "anti magic", ANTI_MAGIC },
                   { "polymorph", POLY_TRAP },
                   { "vibrating square", VIBRATING_SQUARE },
                   { 0, 0 } };

static struct {
    const char *name;
    int type;
} room_types[] = {
    /* for historical reasons, room types are not contiguous numbers */
    /* (type 1 is skipped) */
    { "ordinary", OROOM },
    { "throne", COURT },
    { "swamp", SWAMP },
    { "vault", VAULT },
    { "beehive", BEEHIVE },
    { "morgue", MORGUE },
    { "barracks", BARRACKS },
    { "zoo", ZOO },
    { "delphi", DELPHI },
    { "temple", TEMPLE },
    { "anthole", ANTHOLE },
    { "cocknest", COCKNEST },
    { "leprehall", LEPREHALL },
    { "shop", SHOPBASE },
    { "armor shop", ARMORSHOP },
    { "scroll shop", SCROLLSHOP },
    { "potion shop", POTIONSHOP },
    { "weapon shop", WEAPONSHOP },
    { "food shop", FOODSHOP },
    { "ring shop", RINGSHOP },
    { "wand shop", WANDSHOP },
    { "tool shop", TOOLSHOP },
    { "book shop", BOOKSHOP },
    { "health food shop", FODDERSHOP },
    { "candle shop", CANDLESHOP },
    { 0, 0 }
};

const char *fname = "(stdin)";
int fatal_error = 0;
int got_errors = 0;
int be_verbose = 0;
int fname_counter = 1;

#ifdef FLEX23_BUG
/* Flex 2.3 bug work around; not needed for 2.3.6 or later */
int yy_more_len = 0;
#endif

extern unsigned int max_x_map, max_y_map;

extern int nh_line_number;

extern int token_start_pos;
extern char curr_token[512];

struct lc_vardefs *vardefs = NULL;
struct lc_funcdefs *function_definitions = NULL;

extern int allow_break_statements;
extern struct lc_breakdef *break_list;

int
main(argc, argv)
int argc;
char **argv;
{
    FILE *fin;
    int i;
    boolean errors_encountered = FALSE;
#if defined(MAC) && (defined(THINK_C) || defined(__MWERKS__))
    static char *mac_argv[] = {
        "lev_comp", /* dummy argv[0] */
        ":dat:Arch.des",    ":dat:Barb.des",     ":dat:Caveman.des",
        ":dat:Healer.des",  ":dat:Knight.des",   ":dat:Monk.des",
        ":dat:Priest.des",  ":dat:Ranger.des",   ":dat:Rogue.des",
        ":dat:Samurai.des", ":dat:Tourist.des",  ":dat:Valkyrie.des",
        ":dat:Wizard.des",  ":dat:bigroom.des",  ":dat:castle.des",
        ":dat:endgame.des", ":dat:gehennom.des", ":dat:knox.des",
        ":dat:medusa.des",  ":dat:mines.des",    ":dat:oracle.des",
        ":dat:sokoban.des", ":dat:tower.des",    ":dat:yendor.des"
    };

    argc = SIZE(mac_argv);
    argv = mac_argv;
#endif
    /* Note:  these initializers don't do anything except guarantee that
     *        we're linked properly.
     */
    monst_init();
    objects_init();
    decl_init();
    /* this one does something... */
    init_obj_classes();

    init_yyout(stdout);
    if (argc == 1) { /* Read standard input */
        init_yyin(stdin);
        (void) yyparse();
        if (fatal_error > 0) {
            errors_encountered = TRUE;
        }
    } else { /* Otherwise every argument is a filename */
        for (i = 1; i < argc; i++) {
            fname = argv[i];
            if (!strcmp(fname, "-v")) {
                be_verbose++;
                continue;
            }
            fin = freopen(fname, "r", stdin);
            if (!fin) {
                lc_pline("Can't open \"%s\" for input.\n", VA_PASS1(fname));
                perror(fname);
                errors_encountered = TRUE;
            } else {
                fname_counter = 1;
                init_yyin(fin);
                (void) yyparse();
                nh_line_number = 1;
                if (fatal_error > 0 || got_errors > 0) {
                    errors_encountered = TRUE;
                    fatal_error = 0;
                }
            }
        }
    }
    exit(errors_encountered ? EXIT_FAILURE : EXIT_SUCCESS);
    /*NOTREACHED*/
    return 0;
}

/*
 * Each time the parser detects an error, it uses this function.
 * Here we take count of the errors. To continue farther than
 * MAX_ERRORS wouldn't be reasonable.
 * Assume that explicit calls from lev_comp.y have the 1st letter
 * capitalized, to allow printing of the line containing the start of
 * the current declaration, instead of the beginning of the next declaration.
 */
void
yyerror(s)
const char *s;
{
    char *e = ((char *) s + strlen(s) - 1);

    (void) fprintf(stderr, "%s: line %d, pos %d: %s", fname, nh_line_number,
                   token_start_pos - (int) strlen(curr_token), s);
    if (*e != '.' && *e != '!')
        (void) fprintf(stderr, " at \"%s\"", curr_token);
    (void) fprintf(stderr, "\n");

    if (++fatal_error > MAX_ERRORS) {
        (void) fprintf(stderr, "Too many errors, good bye!\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Just display a warning (that is : a non fatal error)
 */
void
yywarning(s)
const char *s;
{
    (void) fprintf(stderr, "%s: line %d : WARNING : %s\n", fname,
                   nh_line_number, s);
}

/*
 * Stub needed for lex interface.
 */
int
yywrap()
{
    return 1;
}

/*
 * lc_pline(): lev_comp version of pline(), stripped down version of
 * core's pline(), with convoluted handling for variadic arguments to
 * support <stdarg.h>, <varargs.h>, and neither of the above.
 *
 * Using state for message/warning/error mode simplifies calling interface.
 */
#define LC_PLINE_MESSAGE 0
#define LC_PLINE_WARNING 1
#define LC_PLINE_ERROR 2
static int lc_pline_mode = LC_PLINE_MESSAGE;

#if defined(USE_STDARG) || defined(USE_VARARGS)
static void FDECL(lc_vpline, (const char *, va_list));

void lc_pline
VA_DECL(const char *, line)
{
    VA_START(line);
    VA_INIT(line, char *);
    lc_vpline(line, VA_ARGS);
    VA_END();
}

#ifdef USE_STDARG
static void
lc_vpline(const char *line, va_list the_args)
#else
static void
lc_vpline(line, the_args)
const char *line;
va_list the_args;
#endif

#else /* USE_STDARG | USE_VARARG */

#define lc_vpline lc_pline

void
lc_pline
VA_DECL(const char *, line)
#endif /* USE_STDARG | USE_VARARG */
{   /* opening brace for lc_vpline, nested block for USE_OLDARGS lc_pline */

    char pbuf[3 * BUFSZ];
    static char nomsg[] = "(no message)";
    /* Do NOT use VA_START and VA_END in here... see above */

    if (!line || !*line)
        line = nomsg; /* shouldn't happen */
    if (index(line, '%')) {
        Vsprintf(pbuf, line, VA_ARGS);
        pbuf[BUFSZ - 1] = '\0'; /* truncate if long */
        line = pbuf;
    }
    switch (lc_pline_mode) {
    case LC_PLINE_ERROR:
        yyerror(line);
        break;
    case LC_PLINE_WARNING:
        yywarning(line);
        break;
    default:
        (void) fprintf(stderr, "%s\n", line);
        break;
    }
    lc_pline_mode = LC_PLINE_MESSAGE; /* reset to default */
    return;
#if !(defined(USE_STDARG) || defined(USE_VARARGS))
    VA_END();  /* closing brace ofr USE_OLDARGS's nested block */
#endif
}

/*VARARGS1*/
void
lc_error
VA_DECL(const char *, line)
{
    VA_START(line);
    VA_INIT(line, const char *);
    lc_pline_mode = LC_PLINE_ERROR;
    lc_vpline(line, VA_ARGS);
    VA_END();
    return;
}

/*VARARGS1*/
void
lc_warning
VA_DECL(const char *, line)
{
    VA_START(line);
    VA_INIT(line, const char *);
    lc_pline_mode = LC_PLINE_WARNING;
    lc_vpline(line, VA_ARGS);
    VA_END();
    return;
}

char *
decode_parm_chr(chr)
char chr;
{
    static char buf[32];

    switch (chr) {
    default:
        Strcpy(buf, "unknown");
        break;
    case 'i':
        Strcpy(buf, "int");
        break;
    case 'r':
        Strcpy(buf, "region");
        break;
    case 's':
        Strcpy(buf, "str");
        break;
    case 'O':
        Strcpy(buf, "obj");
        break;
    case 'c':
        Strcpy(buf, "coord");
        break;
    case ' ':
        Strcpy(buf, "nothing");
        break;
    case 'm':
        Strcpy(buf, "mapchar");
        break;
    case 'M':
        Strcpy(buf, "monster");
        break;
    }
    return buf;
}

char *
decode_parm_str(str)
char *str;
{
    static char tmpbuf[1024];
    char *p = str;
    tmpbuf[0] = '\0';
    if (str) {
        for (; *p; p++) {
            Strcat(tmpbuf, decode_parm_chr(*p));
            if (*(p + 1))
                Strcat(tmpbuf, ", ");
        }
    }
    return tmpbuf;
}

struct opvar *
set_opvar_int(ov, val)
struct opvar *ov;
long val;
{
    if (ov) {
        ov->spovartyp = SPOVAR_INT;
        ov->vardata.l = val;
    }
    return ov;
}

struct opvar *
set_opvar_coord(ov, val)
struct opvar *ov;
long val;
{
    if (ov) {
        ov->spovartyp = SPOVAR_COORD;
        ov->vardata.l = val;
    }
    return ov;
}

struct opvar *
set_opvar_region(ov, val)
struct opvar *ov;
long val;
{
    if (ov) {
        ov->spovartyp = SPOVAR_REGION;
        ov->vardata.l = val;
    }
    return ov;
}

struct opvar *
set_opvar_mapchar(ov, val)
struct opvar *ov;
long val;
{
    if (ov) {
        ov->spovartyp = SPOVAR_MAPCHAR;
        ov->vardata.l = val;
    }
    return ov;
}

struct opvar *
set_opvar_monst(ov, val)
struct opvar *ov;
long val;
{
    if (ov) {
        ov->spovartyp = SPOVAR_MONST;
        ov->vardata.l = val;
    }
    return ov;
}

struct opvar *
set_opvar_obj(ov, val)
struct opvar *ov;
long val;
{
    if (ov) {
        ov->spovartyp = SPOVAR_OBJ;
        ov->vardata.l = val;
    }
    return ov;
}

struct opvar *
set_opvar_str(ov, val)
struct opvar *ov;
const char *val;
{
    if (ov) {
        ov->spovartyp = SPOVAR_STRING;
        ov->vardata.str = (val) ? strdup(val) : NULL;
    }
    return ov;
}

struct opvar *
set_opvar_var(ov, val)
struct opvar *ov;
const char *val;
{
    if (ov) {
        ov->spovartyp = SPOVAR_VARIABLE;
        ov->vardata.str = (val) ? strdup(val) : NULL;
    }
    return ov;
}

#define New(type) \
    (type *) memset((genericptr_t) alloc(sizeof(type)), 0, sizeof(type))

#if defined(USE_STDARG) || defined(USE_VARARGS)
static void FDECL(vadd_opvars, (sp_lev *, const char *, va_list));

void add_opvars
VA_DECL2(sp_lev *, sp, const char *, fmt)
{
    VA_START(fmt);
    VA_INIT(fmt, char *);
    vadd_opvars(sp, fmt, VA_ARGS);
    VA_END();
}

#ifdef USE_STDARG
static void
vadd_opvars(sp_lev *sp, const char *fmt, va_list the_args)

#else
static void
vadd_opvars(sp, fmt, the_args)
sp_lev *sp;
const char *fmt;
va_list the_args;

#endif

#else /* USE_STDARG | USE_VARARG */

#define vadd_opvars add_opvars

void add_opvars
VA_DECL2(sp_lev *, sp, const char *, fmt)
#endif /* USE_STDARG | USE_VARARG */
{
    const char *p, *lp;
    long la;
    /* Do NOT use VA_START and VA_END in here... see above */

    for (p = fmt; *p != '\0'; p++) {
        switch (*p) {
        case ' ':
            break;
        case 'i': /* integer (via plain 'int') */
        {
            struct opvar *ov = New(struct opvar);

            set_opvar_int(ov, (long) VA_NEXT(la, int));
            add_opcode(sp, SPO_PUSH, ov);
            break;
        }
        case 'l': /* integer (via 'long int') */
        {
            struct opvar *ov = New(struct opvar);

            set_opvar_int(ov, VA_NEXT(la, long));
            add_opcode(sp, SPO_PUSH, ov);
            break;
        }
        case 'c': /* coordinate */
        {
            struct opvar *ov = New(struct opvar);
            set_opvar_coord(ov, VA_NEXT(la, long));
            add_opcode(sp, SPO_PUSH, ov);
            break;
        }
        case 'r': /* region */
        {
            struct opvar *ov = New(struct opvar);
            set_opvar_region(ov, VA_NEXT(la, long));
            add_opcode(sp, SPO_PUSH, ov);
            break;
        }
        case 'm': /* mapchar */
        {
            struct opvar *ov = New(struct opvar);
            set_opvar_mapchar(ov, VA_NEXT(la, long));
            add_opcode(sp, SPO_PUSH, ov);
            break;
        }
        case 'M': /* monster */
        {
            struct opvar *ov = New(struct opvar);
            set_opvar_monst(ov, VA_NEXT(la, long));
            add_opcode(sp, SPO_PUSH, ov);
            break;
        }
        case 'O': /* object */
        {
            struct opvar *ov = New(struct opvar);
            set_opvar_obj(ov, VA_NEXT(la, long));
            add_opcode(sp, SPO_PUSH, ov);
            break;
        }
        case 's': /* string */
        {
            struct opvar *ov = New(struct opvar);
            set_opvar_str(ov, VA_NEXT(lp, const char *));
            add_opcode(sp, SPO_PUSH, ov);
            break;
        }
        case 'v': /* variable */
        {
            struct opvar *ov = New(struct opvar);
            set_opvar_var(ov, VA_NEXT(lp, const char *));
            add_opcode(sp, SPO_PUSH, ov);
            break;
        }
        case 'o': /* opcode */
        {
            long i = VA_NEXT(la, int);
            if (i < 0 || i >= MAX_SP_OPCODES)
                lc_pline("add_opvars: unknown opcode '%ld'.", VA_PASS1(i));
            add_opcode(sp, i, NULL);
            break;
        }
        default:
            lc_pline("add_opvars: illegal format character '%ld'.",
                     VA_PASS1((long) *p));
            break;
        }
    }

#if !(defined(USE_STDARG) || defined(USE_VARARGS))
    /* provide closing brace for USE_OLDARGS nested block from VA_DECL2() */
    VA_END();
#endif
}

void
break_stmt_start()
{
    allow_break_statements++;
}

void
break_stmt_end(splev)
sp_lev *splev;
{
    struct lc_breakdef *tmp = break_list;
    struct lc_breakdef *prv = NULL;

    while (tmp) {
        if (tmp->break_depth == allow_break_statements) {
            struct lc_breakdef *nxt = tmp->next;
            set_opvar_int(tmp->breakpoint,
                          splev->n_opcodes - tmp->breakpoint->vardata.l - 1);
            tmp->next = NULL;
            Free(tmp);
            if (!prv)
                break_list = NULL;
            else
                prv->next = nxt;
            tmp = nxt;
        } else {
            prv = tmp;
            tmp = tmp->next;
        }
    }
    allow_break_statements--;
}

void
break_stmt_new(splev, i)
sp_lev *splev;
long i;
{
    struct lc_breakdef *tmp = New(struct lc_breakdef);

    tmp->breakpoint = New(struct opvar);
    tmp->break_depth = allow_break_statements;
    tmp->next = break_list;
    break_list = tmp;
    set_opvar_int(tmp->breakpoint, i);
    add_opcode(splev, SPO_PUSH, tmp->breakpoint);
    add_opcode(splev, SPO_JMP, NULL);
}

struct lc_funcdefs *
funcdef_new(addr, name)
long addr;
char *name;
{
    struct lc_funcdefs *f = New(struct lc_funcdefs);

    if (!f) {
        lc_error("Could not alloc function definition for '%s'.",
                 VA_PASS1(name));
        return NULL;
    }
    f->next = NULL;
    f->addr = addr;
    f->name = strdup(name);
    f->n_called = 0;
    f->n_params = 0;
    f->params = NULL;
    f->code.opcodes = NULL;
    f->code.n_opcodes = 0;
    return f;
}

void
funcdef_free_all(fchain)
struct lc_funcdefs *fchain;
{
    struct lc_funcdefs *tmp = fchain;
    struct lc_funcdefs *nxt;
    struct lc_funcdefs_parm *tmpparam;

    while (tmp) {
        nxt = tmp->next;
        Free(tmp->name);
        while (tmp->params) {
            tmpparam = tmp->params->next;
            Free(tmp->params->name);
            tmp->params = tmpparam;
        }
        /* FIXME: free tmp->code */
        Free(tmp);
        tmp = nxt;
    }
}

char *
funcdef_paramtypes(f)
struct lc_funcdefs *f;
{
    int i = 0;
    struct lc_funcdefs_parm *fp = f->params;
    char *tmp = (char *) alloc((f->n_params) + 1);

    if (!tmp)
        return NULL;
    while (fp) {
        tmp[i++] = fp->parmtype;
        fp = fp->next;
    }
    tmp[i] = '\0';
    return tmp;
}

struct lc_funcdefs *
funcdef_defined(f, name, casesense)
struct lc_funcdefs *f;
char *name;
int casesense;
{
    while (f) {
        if (casesense) {
            if (!strcmp(name, f->name))
                return f;
        } else {
            if (!case_insensitive_comp(name, f->name))
                return f;
        }
        f = f->next;
    }
    return NULL;
}

struct lc_vardefs *
vardef_new(typ, name)
long typ;
char *name;
{
    struct lc_vardefs *f = New(struct lc_vardefs);

    if (!f) {
        lc_error("Could not alloc variable definition for '%s'.",
                 VA_PASS1(name));
        return NULL;
    }
    f->next = NULL;
    f->var_type = typ;
    f->name = strdup(name);
    f->n_used = 0;
    return f;
}

void
vardef_free_all(fchain)
struct lc_vardefs *fchain;
{
    struct lc_vardefs *tmp = fchain;
    struct lc_vardefs *nxt;

    while (tmp) {
        if (be_verbose && (tmp->n_used == 0))
            lc_warning("Unused variable '%s'", VA_PASS1(tmp->name));
        nxt = tmp->next;
        Free(tmp->name);
        Free(tmp);
        tmp = nxt;
    }
}

struct lc_vardefs *
vardef_defined(f, name, casesense)
struct lc_vardefs *f;
char *name;
int casesense;
{
    while (f) {
        if (casesense) {
            if (!strcmp(name, f->name))
                return f;
        } else {
            if (!case_insensitive_comp(name, f->name))
                return f;
        }
        f = f->next;
    }
    return NULL;
}

const char *
spovar2str(spovar)
long spovar;
{
    static int togl = 0;
    static char buf[2][128];
    const char *n = NULL;
    int is_array = (spovar & SPOVAR_ARRAY);

    spovar &= ~SPOVAR_ARRAY;
    switch (spovar) {
    default:
        lc_error("spovar2str(%ld)", VA_PASS1(spovar));
        break;
    case SPOVAR_INT:
        n = "integer";
        break;
    case SPOVAR_STRING:
        n = "string";
        break;
    case SPOVAR_VARIABLE:
        n = "variable";
        break;
    case SPOVAR_COORD:
        n = "coordinate";
        break;
    case SPOVAR_REGION:
        n = "region";
        break;
    case SPOVAR_MAPCHAR:
        n = "mapchar";
        break;
    case SPOVAR_MONST:
        n = "monster";
        break;
    case SPOVAR_OBJ:
        n = "object";
        break;
    }

    togl = ((togl + 1) % 2);

    Sprintf(buf[togl], "%s%s", n, (is_array ? " array" : ""));
    return buf[togl];
}

void
vardef_used(vd, varname)
struct lc_vardefs *vd;
char *varname;
{
    struct lc_vardefs *tmp;

    if ((tmp = vardef_defined(vd, varname, 1)) != 0)
        tmp->n_used++;
}

void
check_vardef_type(vd, varname, vartype)
struct lc_vardefs *vd;
char *varname;
long vartype;
{
    struct lc_vardefs *tmp;

    if ((tmp = vardef_defined(vd, varname, 1)) != 0) {
        if (tmp->var_type != vartype)
            lc_error("Trying to use variable '%s' as %s, when it is %s.",
                     VA_PASS3(varname,
                              spovar2str(vartype),
                              spovar2str(tmp->var_type)));
    } else
        lc_error("Variable '%s' not defined.", VA_PASS1(varname));
}

struct lc_vardefs *
add_vardef_type(vd, varname, vartype)
struct lc_vardefs *vd;
char *varname;
long vartype;
{
    struct lc_vardefs *tmp;

    if ((tmp = vardef_defined(vd, varname, 1)) != 0) {
        if (tmp->var_type != vartype)
            lc_error("Trying to redefine variable '%s' as %s, when it is %s.",
                     VA_PASS3(varname,
                              spovar2str(vartype),
                              spovar2str(tmp->var_type)));
    } else {
        tmp = vardef_new(vartype, varname);
        tmp->next = vd;
        return tmp;
    }
    return vd;
}

int
reverse_jmp_opcode(opcode)
int opcode;
{
    switch (opcode) {
    case SPO_JE:
        return SPO_JNE;
    case SPO_JNE:
        return SPO_JE;
    case SPO_JL:
        return SPO_JGE;
    case SPO_JG:
        return SPO_JLE;
    case SPO_JLE:
        return SPO_JG;
    case SPO_JGE:
        return SPO_JL;
    default:
        lc_error("Cannot reverse comparison jmp opcode %ld.",
                 VA_PASS1((long) opcode));
        return SPO_NULL;
    }
}

/* basically copied from src/sp_lev.c */
struct opvar *
opvar_clone(ov)
struct opvar *ov;
{
    if (ov) {
        struct opvar *tmpov = (struct opvar *) alloc(sizeof(struct opvar));

        if (!tmpov) { /* lint suppression */
            /*NOTREACHED*/
#if 0
            /* not possible; alloc() never returns Null */
            panic("could not alloc opvar struct");
            /*NOTREACHED*/
#endif
            return (struct opvar *) 0;
        }
        switch (ov->spovartyp) {
        case SPOVAR_COORD:
        case SPOVAR_REGION:
        case SPOVAR_MAPCHAR:
        case SPOVAR_MONST:
        case SPOVAR_OBJ:
        case SPOVAR_INT: {
            tmpov->spovartyp = ov->spovartyp;
            tmpov->vardata.l = ov->vardata.l;
        } break;
        case SPOVAR_VARIABLE:
        case SPOVAR_STRING: {
            int len = strlen(ov->vardata.str);
            tmpov->spovartyp = ov->spovartyp;
            tmpov->vardata.str = (char *) alloc(len + 1);
            (void) memcpy((genericptr_t) tmpov->vardata.str,
                          (genericptr_t) ov->vardata.str, len);
            tmpov->vardata.str[len] = '\0';
        } break;
        default: {
            lc_error("Unknown opvar_clone value type (%ld)!",
                     VA_PASS1((long) ov->spovartyp));
        } /* default */
        } /* switch */
        return tmpov;
    }
    return (struct opvar *) 0;
}

void
splev_add_from(splev, from_splev)
sp_lev *splev;
sp_lev *from_splev;
{
    int i;

    if (splev && from_splev)
        for (i = 0; i < from_splev->n_opcodes; i++)
            add_opcode(splev, from_splev->opcodes[i].opcode,
                       opvar_clone(from_splev->opcodes[i].opdat));
}

void
start_level_def(splev, ldfname)
sp_lev **splev;
char *ldfname;
{
    struct lc_funcdefs *f;

    if (index(ldfname, '.'))
        lc_error("Invalid dot ('.') in level name '%s'.", VA_PASS1(ldfname));
    if ((int) strlen(ldfname) > 14)
        lc_error("Level names limited to 14 characters ('%s').",
                 VA_PASS1(ldfname));
    f = function_definitions;
    while (f) {
        f->n_called = 0;
        f = f->next;
    }
    *splev = (sp_lev *) alloc(sizeof(sp_lev));
    (*splev)->n_opcodes = 0;
    (*splev)->opcodes = NULL;

    vardef_free_all(vardefs);
    vardefs = NULL;
}

/*
 * Find the type of floor, knowing its char representation.
 */
int
get_floor_type(c)
char c;
{
    int val;

    SpinCursor(3);
    val = what_map_char(c);
    if (val == INVALID_TYPE) {
        val = ERR;
        yywarning("Invalid fill character in MAZE declaration");
    }
    return val;
}

/*
 * Find the type of a room in the table, knowing its name.
 */
int
get_room_type(s)
char *s;
{
    register int i;

    SpinCursor(3);
    for (i = 0; room_types[i].name; i++)
        if (!strcmp(s, room_types[i].name))
            return ((int) room_types[i].type);
    return ERR;
}

/*
 * Find the type of a trap in the table, knowing its name.
 */
int
get_trap_type(s)
char *s;
{
    register int i;

    SpinCursor(3);
    for (i = 0; trap_types[i].name; i++)
        if (!strcmp(s, trap_types[i].name))
            return trap_types[i].type;
    return ERR;
}

/*
 * Find the index of a monster in the table, knowing its name.
 */
int
get_monster_id(s, c)
char *s;
char c;
{
    register int i, class;

    SpinCursor(3);
    class = c ? def_char_to_monclass(c) : 0;
    if (class == MAXMCLASSES)
        return ERR;

    for (i = LOW_PM; i < NUMMONS; i++)
        if (!class || class == mons[i].mlet)
            if (!strcmp(s, mons[i].mname))
                return i;
    /* didn't find it; lets try case insensitive search */
    for (i = LOW_PM; i < NUMMONS; i++)
        if (!class || class == mons[i].mlet)
            if (!case_insensitive_comp(s, mons[i].mname)) {
                if (be_verbose)
                    lc_warning("Monster type \"%s\" matches \"%s\".",
                               VA_PASS2(s, mons[i].mname));
                return i;
            }
    return ERR;
}

/*
 * Find the index of an object in the table, knowing its name.
 */
int
get_object_id(s, c)
char *s;
char c; /* class */
{
    int i, class;
    const char *objname;

    SpinCursor(3);
    class = (c > 0) ? def_char_to_objclass(c) : 0;
    if (class == MAXOCLASSES)
        return ERR;

    for (i = class ? bases[class] : 0; i < NUM_OBJECTS; i++) {
        if (class && objects[i].oc_class != class)
            break;
        objname = obj_descr[i].oc_name;
        if (objname && !strcmp(s, objname))
            return i;
    }
    for (i = class ? bases[class] : 0; i < NUM_OBJECTS; i++) {
        if (class && objects[i].oc_class != class)
            break;
        objname = obj_descr[i].oc_name;
        if (objname && !case_insensitive_comp(s, objname)) {
            if (be_verbose)
                lc_warning("Object type \"%s\" matches \"%s\".",
                           VA_PASS2(s, objname));
            return i;
        }
    }
    return ERR;
}

static void
init_obj_classes()
{
    int i, class, prev_class;

    prev_class = -1;
    for (i = 0; i < NUM_OBJECTS; i++) {
        class = objects[i].oc_class;
        if (class != prev_class) {
            bases[class] = i;
            prev_class = class;
        }
    }
}

/*
 * Is the character 'c' a valid monster class ?
 */
boolean
check_monster_char(c)
char c;
{
    return (def_char_to_monclass(c) != MAXMCLASSES);
}

/*
 * Is the character 'c' a valid object class ?
 */
boolean
check_object_char(c)
char c;
{
    return (def_char_to_objclass(c) != MAXOCLASSES);
}

/*
 * Convert .des map letter into floor type.
 */
char
what_map_char(c)
char c;
{
    SpinCursor(3);
    switch (c) {
    case ' ':
        return (STONE);
    case '#':
        return (CORR);
    case '.':
        return (ROOM);
    case '-':
        return (HWALL);
    case '|':
        return (VWALL);
    case '+':
        return (DOOR);
    case 'A':
        return (AIR);
    case 'B':
        return (CROSSWALL); /* hack: boundary location */
    case 'C':
        return (CLOUD);
    case 'S':
        return (SDOOR);
    case 'H':
        return (SCORR);
    case '{':
        return (FOUNTAIN);
    case '\\':
        return (THRONE);
    case 'K':
        return (SINK);
    case '}':
        return (MOAT);
    case 'P':
        return (POOL);
    case 'L':
        return (LAVAPOOL);
    case 'I':
        return (ICE);
    case 'W':
        return (WATER);
    case 'T':
        return (TREE);
    case 'F':
        return (IRONBARS); /* Fe = iron */
    case 'x':
        return (MAX_TYPE); /* "see-through" */
    }
    return (INVALID_TYPE);
}

void
add_opcode(sp, opc, dat)
sp_lev *sp;
int opc;
genericptr_t dat;
{
    long nop = sp->n_opcodes;
    _opcode *tmp;

    if ((opc < 0) || (opc >= MAX_SP_OPCODES))
        lc_error("Unknown opcode '%ld'", VA_PASS1((long) opc));

    tmp = (_opcode *) alloc(sizeof(_opcode) * (nop + 1));
    if (!tmp) { /* lint suppression */
        /*NOTREACHED*/
#if 0
        /* not possible; alloc() never returns Null */
        lc_error("%s", VA_PASS1("Could not alloc opcode space"));
#endif
        return;
    }

    if (sp->opcodes && nop) {
        (void) memcpy(tmp, sp->opcodes, sizeof(_opcode) * nop);
        free(sp->opcodes);
    }
    sp->opcodes = tmp;

    sp->opcodes[nop].opcode = opc;
    sp->opcodes[nop].opdat = dat;

    sp->n_opcodes++;
}

/*
 * Yep! LEX gives us the map in a raw mode.
 * Just analyze it here.
 */
void
scan_map(map, sp)
char *map;
sp_lev *sp;
{
    register int i, len;
    register char *s1, *s2;
    int max_len = 0;
    int max_hig = 0;
    char *tmpmap[MAP_Y_LIM+1];
    int dx, dy;
    char *mbuf;

    /* First, strip out digits 0-9 (line numbering) */
    for (s1 = s2 = map; *s1; s1++)
        if (*s1 < '0' || *s1 > '9')
            *s2++ = *s1;
    *s2 = '\0';

    /* Second, find the max width of the map */
    s1 = map;
    while (s1 && *s1) {
        s2 = index(s1, '\n');
        if (s2) {
            len = (int) (s2 - s1);
            s1 = s2 + 1;
        } else {
            len = (int) strlen(s1);
            s1 = (char *) 0;
        }
        if (len > max_len)
            max_len = len;
    }

    /* Then parse it now */
    while (map && *map) {
        if (max_hig > MAP_Y_LIM)
            break;
        tmpmap[max_hig] = (char *) alloc(max_len);
        s1 = index(map, '\n');
        if (s1) {
            len = (int) (s1 - map);
            s1++;
        } else {
            len = (int) strlen(map);
            s1 = map + len;
        }
        for (i = 0; i < len; i++)
            if ((tmpmap[max_hig][i] = what_map_char(map[i]))
                == INVALID_TYPE) {
                lc_warning(
                "Invalid character '%ld' @ (%ld, %ld) - replacing with stone",
                           VA_PASS3((long) map[i], (long) max_hig, (long) i));
                tmpmap[max_hig][i] = STONE;
            }
        while (i < max_len)
            tmpmap[max_hig][i++] = STONE;
        map = s1;
        max_hig++;
    }

    /* Memorize boundaries */

    max_x_map = max_len - 1;
    max_y_map = max_hig - 1;

    if (max_len > MAP_X_LIM || max_hig > MAP_Y_LIM) {
        lc_error("Map too large at (%ld x %ld), max is (%ld x %ld)",
                 VA_PASS4((long) max_len, (long) max_hig,
                          (long) MAP_X_LIM, (long) MAP_Y_LIM));
    }

    mbuf = (char *) alloc(((max_hig - 1) * max_len) + (max_len - 1) + 2);
    for (dy = 0; dy < max_hig; dy++)
        for (dx = 0; dx < max_len; dx++)
            mbuf[(dy * max_len) + dx] = (tmpmap[dy][dx] + 1);

    mbuf[((max_hig - 1) * max_len) + (max_len - 1) + 1] = '\0';

    add_opvars(sp, "sllo", VA_PASS4(mbuf, (long) max_hig, (long) max_len,
                                    SPO_MAP));

    for (dy = 0; dy < max_hig; dy++)
        Free(tmpmap[dy]);
    Free(mbuf);
}

/*
 * Output some info common to all special levels.
 */
static boolean
write_common_data(fd)
int fd;
{
    static struct version_info version_data = {
        VERSION_NUMBER, VERSION_FEATURES, VERSION_SANITY1, VERSION_SANITY2,
        VERSION_SANITY3
    };

    Write(fd, &version_data, sizeof version_data);
    return TRUE;
}

/*
 * Here we write the sp_lev structure in the specified file (fd).
 * Also, we have to free the memory allocated via alloc().
 */
static boolean
write_maze(fd, maze)
int fd;
sp_lev *maze;
{
    int i;

    if (!write_common_data(fd))
        return FALSE;

    Write(fd, &(maze->n_opcodes), sizeof(maze->n_opcodes));

    for (i = 0; i < maze->n_opcodes; i++) {
        _opcode tmpo = maze->opcodes[i];

        Write(fd, &(tmpo.opcode), sizeof(tmpo.opcode));

        if (tmpo.opcode < SPO_NULL || tmpo.opcode >= MAX_SP_OPCODES)
            panic("write_maze: unknown opcode (%d).", tmpo.opcode);

        if (tmpo.opcode == SPO_PUSH) {
            genericptr_t opdat = tmpo.opdat;
            if (opdat) {
                struct opvar *ov = (struct opvar *) opdat;
                int size;
                Write(fd, &(ov->spovartyp), sizeof(ov->spovartyp));
                switch (ov->spovartyp) {
                case SPOVAR_NULL:
                    break;
                case SPOVAR_COORD:
                case SPOVAR_REGION:
                case SPOVAR_MAPCHAR:
                case SPOVAR_MONST:
                case SPOVAR_OBJ:
                case SPOVAR_INT:
                    Write(fd, &(ov->vardata.l), sizeof(ov->vardata.l));
                    break;
                case SPOVAR_VARIABLE:
                case SPOVAR_STRING:
                    if (ov->vardata.str)
                        size = strlen(ov->vardata.str);
                    else
                        size = 0;
                    Write(fd, &size, sizeof(size));
                    if (size) {
                        Write(fd, ov->vardata.str, size);
                        Free(ov->vardata.str);
                    }
                    break;
                default:
                    panic("write_maze: unknown data type (%d).",
                          ov->spovartyp);
                }
            } else
                panic("write_maze: PUSH with no data.");
        } else {
            /* sanity check */
            genericptr_t opdat = tmpo.opdat;
            if (opdat)
                panic("write_maze: opcode (%d) has data.", tmpo.opcode);
        }

        Free(tmpo.opdat);
    }
    /* clear the struct for next user */
    Free(maze->opcodes);
    maze->opcodes = NULL;
    /*(void) memset((genericptr_t) &maze->init_lev, 0, sizeof
     * maze->init_lev);*/

    return TRUE;
}

/*
 * Open and write maze or rooms file, based on which pointer is non-null.
 * Return TRUE on success, FALSE on failure.
 */
boolean
write_level_file(filename, lvl)
char *filename;
sp_lev *lvl;
{
    int fout;
    char lbuf[60];

    lbuf[0] = '\0';
#ifdef PREFIX
    Strcat(lbuf, PREFIX);
#endif
    Strcat(lbuf, filename);
    Strcat(lbuf, LEV_EXT);

#if defined(MAC) && (defined(__SC__) || defined(__MRC__))
    fout = open(lbuf, O_WRONLY | O_CREAT | O_BINARY);
#else
    fout = open(lbuf, O_WRONLY | O_CREAT | O_BINARY, OMASK);
#endif
    if (fout < 0)
        return FALSE;

    if (!lvl)
        panic("write_level_file");

    if (be_verbose)
        fprintf(stdout, "File: '%s', opcodes: %ld\n", lbuf, lvl->n_opcodes);

    if (!write_maze(fout, lvl))
        return FALSE;

    (void) close(fout);

    return TRUE;
}

static int
case_insensitive_comp(s1, s2)
const char *s1;
const char *s2;
{
    uchar u1, u2;

    for (;; s1++, s2++) {
        u1 = (uchar) *s1;
        if (isupper(u1))
            u1 = tolower(u1);
        u2 = (uchar) *s2;
        if (isupper(u2))
            u2 = tolower(u2);
        if (u1 == '\0' || u1 != u2)
            break;
    }
    return u1 - u2;
}

#ifdef STRICT_REF_DEF
/*
 * Any globals declared in hack.h and descendents which aren't defined
 * in the modules linked into lev_comp should be defined here.  These
 * definitions can be dummies:  their sizes shouldn't matter as long as
 * as their types are correct; actual values are irrelevant.
 */
#define ARBITRARY_SIZE 1
/* attrib.c */
struct attribs attrmax, attrmin;
/* files.c */
const char *configfile;
char lock[ARBITRARY_SIZE];
char SAVEF[ARBITRARY_SIZE];
#ifdef MICRO
char SAVEP[ARBITRARY_SIZE];
#endif
/* termcap.c */
struct tc_lcl_data tc_lcl_data;
#ifdef TEXTCOLOR
#ifdef TOS
const char *hilites[CLR_MAX];
#else
char NEARDATA *hilites[CLR_MAX];
#endif
#endif
/* trap.c */
const char *traps[TRAPNUM];
/* window.c */
#ifdef HANGUPHANDLING
volatile
#endif
    struct window_procs windowprocs;
/* xxxtty.c */
#ifdef DEFINE_OSPEED
short ospeed;
#endif
#endif /* STRICT_REF_DEF */

/*lev_main.c*/
