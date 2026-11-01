/* NetHack 5.0	sfctool.c */
/* Copyright (c) Michael Allison, 2025.                           */
/* NetHack may be freely redistributed.  See license for details. */

/*
 *  Utility for reading a binary save file in the native historical
 *  format of the accompanying NetHack executable, and writing it out
 *  in an export format, possibly destined for a different platform,
 *  architecture, or data model.
 *
 *  The resulting export file will only be useful for transport
 *  between different platforms and architectures that share the exact
 *  same version of NetHack with the same features. That is, the same
 *  fields must be present in the NetHack data structures, in the same
 *  sequence. The fields do not have to be the same size or use the same
 *  data model.
 *
 */

#if defined(WIN32) && !defined(__GNUC__)
#include "win32api.h"
#endif

#include "hack.h"

#include <stdint.h>
#include <string.h>
#include <ctype.h>
#ifdef UNIX
#include <fcntl.h>
#define O_BINARY 0
#endif
#include "integer.h"
#include "sfprocs.h"

#ifdef WIN32
#include <UserEnv.h>
#endif

#ifndef RDTMODE
#define RDTMODE "r"
#endif
#ifndef WRTMODE
#if (defined(MSDOS) || defined(WIN32))
#define WRTMODE "w+b"
#else
#define WRTMODE "w+"
#endif
#endif
#ifndef RDBMODE
#if (defined(MICRO) || defined(WIN32))
#define RDBMODE "rb"
#else
#define RDBMODE "r"
#endif
#endif
#ifndef WRBMODE
#if (defined(MICRO) || defined(WIN32))
#define WRBMODE "w+b"
#else
#define WRBMODE "w+"
#endif
#endif

#ifdef PANICTRACE
#error PANICTRACE is defined
#endif
#ifdef CRASHREPORT
#error CRASHREPORT is defined
#endif

/* functions in this file */
static int process_savefile(const char *, enum saveformats, char *, enum saveformats, boolean);
static void my_sf_init(void);
static NHFILE *open_srcfile(const char *, enum saveformats);
static NHFILE *create_dstfile(char *, enum saveformats);
static const char *style_to_text(enum saveformats style);
static void read_sysconf(void);
static int length_without_val(const char *user_string, int len);
static void usage(int argc, char **argv);
static const char *briefname(const char *fnam);

extern void init_nhfile(NHFILE *);  /* files.c */
extern NHFILE *new_nhfile(void);    /* files.c */
extern void free_nhfile(NHFILE *);  /* files.c */

/*
 * This is a replacement tempered down version of same-named
 * function in files.c within #ifndef SFCTOOL blocks.
 */
int delete_savefile(void);

// int nhclose(int fd);
int util_strncmpi(const char *s1, const char *s2, size_t sz);

#ifdef UNIX
#define nethack_exit exit
ATTRNORETURN void nh_terminate(int) NORETURN;   /* bwrite() calls this */
//static void chdirx(const char *, boolean);
#else
ATTRNORETURN extern void nethack_exit(int) NORETURN;
#ifdef WIN32
boolean get_user_home_folder(char *homebuf, size_t sz);
int GUILaunched;
#endif  /* WIN32 */
#endif
#define Fprintf (void) fprintf

/* Global data */

struct link_compat1 {
    volatile int done_hup;
};

/* worm segment structure */
struct wseg {
    struct wseg *nseg;
    coordxy wx, wy; /* the segment's position */
};

enum { UNCONVERTED = 0, CONVERTED };
char *unconverted_filename = 0;
char *converted_filename = 0;

/* from sfstruct.c */
extern struct restore_info restoreinfo;
/* from sfbase.c */
extern struct sf_structlevel_procs sfoprocs[NUM_SAVEFORMATS], sfiprocs[NUM_SAVEFORMATS];
extern struct sf_structlevel_procs zerosfoprocs, zerosfiprocs;
extern struct sf_fieldlevel_procs sfoflprocs[NUM_SAVEFORMATS], sfiflprocs[NUM_SAVEFORMATS];
extern struct sf_structlevel_procs zerosfodlprocs, zerosfidlprocs;
extern boolean close_check(int);
extern void bclose(int);
extern void config_error_init(boolean, const char *, boolean); /* files.c */
extern boolean get_user_home_folder(char *, size_t);
extern void make_version(void);

char plname[PL_NSIZ_PLUS];
struct version_info vers_info;
int renidx = -1;

const char *const rensuffixes[] = {
    "IL32LLP64", /* (3) Windows x64 savefile on x86 */
    "I32LP64",   /* (4) Unix 64 savefile on x86 */
    "ILP32LL64", /* (5) x86 savefile on Unix 64 */
    "ILP32LL64", /* (6) x86 savefile on Windows x64 */
    "I32LP64",   /* (7) Unix 64 savefile on Windows x64 */
    "IL32LLP64", /* (8) Windows x64 savefile on Unix 64 */
    "OTHER",     /* (9) */
};

#ifdef WIN32
extern boolean get_user_home_folder(char *homebuf, size_t sz); /* files.c */
extern void set_default_prefix_locations(const char *programPath);
#endif
enum saveformats convertstyle = exportascii;

boolean chosen_unconvert = FALSE, explicit_option = FALSE;
const char *thisdatamodel;
static char srclogfilenm[BUFSZ], dstlogfilenm[BUFSZ];

/*********
 * main  *
 *********/

int
main(int argc, char *argv[])
{
    int arg;
    char folderbuf[5000];
    const char *suffix = (convertstyle == exportascii) ? ".exportascii" : "";
    boolean add_folder = TRUE, add_extension = FALSE;

#ifdef WIN32
    const char *default_extension = ".NetHack-saved-game";
    size_t sz;
#else
    const char *default_extension = "";
#endif

    runtime_info_init(); /* mdlib.c */
#ifdef UNIX
    folderbuf[0] = '.';
    folderbuf[1] = '/';
    folderbuf[2] = '\0';
#ifdef CHDIR
    chdirx(HACKDIR, FALSE);
#endif
#endif
#ifdef UNIX
    Strcpy(folderbuf, "save/");
#endif
#ifdef WIN32
    if (!get_user_home_folder(folderbuf, sizeof folderbuf))
        exit(EXIT_FAILURE);
    sz = strlen(folderbuf);
    (void) snprintf(eos(folderbuf), sizeof folderbuf - sz,
                    "\\AppData\\Local\\NetHack\\5.0\\");
    // initoptions_init(); // This allows OPTIONS in syscf on Windows.
    set_default_prefix_locations(argv[0]);
#endif

    read_sysconf();
    thisdatamodel = datamodel(0);
    if (argc < 3 && !(argc == 2 && !strcmp(argv[1], "-d"))) {
        usage(argc, argv);
        exit(EXIT_FAILURE);
    }
    for (arg = 1; arg < argc; ++arg) {
        if (arg == 1 && !strcmp(argv[arg], "-d")) {
            fprintf(
                stdout,
                "\nThe historical savefile datamodel supported by this utility is %s (%s).\n",
                thisdatamodel, datamodel(1));
            exit(EXIT_SUCCESS);
        }

        if (arg == 1 && !strcmp(argv[arg], "-u")) {
            explicit_option = TRUE;
            chosen_unconvert = TRUE;
            continue;
        }
        if (arg == 1 && !strcmp(argv[arg], "-c")) {
            if (explicit_option && chosen_unconvert) {
                fprintf(stderr, "\nsfctool error - conflicting options.\n");
                exit(EXIT_FAILURE); /* both -u and -c not allowed */
            }
            explicit_option = TRUE;
            chosen_unconvert = FALSE;
            continue;
        }

        if (arg == 2) {
            size_t ln = strlen(argv[arg]);
            boolean addseparator = FALSE;

            if (!contains_directory(argv[arg])) {
                char finalchar = *(eos(folderbuf) - 1);

                if (!(finalchar == '\\' || finalchar == '/')) {
                    ln += 1;
                    addseparator = TRUE;
                }
            } else {
                add_folder = FALSE;
            }
#ifdef WIN32
            /* On Windows we allow specifying the savefile name without the extention
             * in the arguments */
            if (strstr(argv[arg], default_extension) == 0)
                add_extension = TRUE;
#endif
            if (explicit_option) {
                if (add_folder)
                    ln += strlen(folderbuf);
                if (add_extension)
                    ln += strlen(default_extension);
                unconverted_filename = (char *) alloc((int) ln + 1);
                Snprintf(unconverted_filename, ln + 1, "%s%s%s%s",
                         add_folder ? folderbuf : "",
                         addseparator ? "/" : "",
                         argv[arg],
                         add_extension ? default_extension : "");
                ln += strlen(suffix);
                converted_filename = (char *) alloc((int) ln + 1);
                Snprintf(converted_filename, ln + 1, "%s%s",
                         unconverted_filename, suffix);
            } else {
                fprintf(stderr, "\nsfctool error - missing -c or -u before "
                                "save filename.\n");
                exit(EXIT_FAILURE); /* need both filenames */
            }
        }
    }
    if (!converted_filename || !unconverted_filename) {
        fprintf(stderr, "\nsfctool error - missing %sconverted file name.\n",
                !converted_filename ? "" : "un");
        exit(EXIT_FAILURE); /* need both filenames */
    }

    my_sf_init();
    if (chosen_unconvert) {
        process_savefile(converted_filename, convertstyle,
                         unconverted_filename, historical, chosen_unconvert);
    } else {
        process_savefile(unconverted_filename, historical, converted_filename,
                         convertstyle, chosen_unconvert);
    }
}

/* ======================================================================== */
/*   Process the src savefile and create the dst file */
/*   Return 1 for success, 0 for failure */
/* ======================================================================== */

static int
process_savefile(const char *srcfnam, enum saveformats srcstyle,
                 char *dstfnam, enum saveformats cvtstyle, boolean unconvert)
{
    NHFILE *nhfp[2];                 /* one for UNCONVERTED, one for CONVERTED */
    int srcidx = unconvert ? CONVERTED : UNCONVERTED,
        dstidx = unconvert ? UNCONVERTED : CONVERTED,
        sfstatus = 0, i;
    char indicator, file_csc_count;
    extern struct version_info vers_info;
    extern uchar cscbuf[];
    /* nh_uncompress(fq_save); */
    const char *dmfile;

    if ((nhfp[srcidx] = open_srcfile(srcfnam, srcstyle)) == 0)
        return 0;
    sfstatus = validate(nhfp[srcidx], srcfnam, FALSE);
    dmfile = what_datamodel_is_this(0,
                                    cscbuf[1],  /* short */
                                    cscbuf[2],  /* int */
                                    cscbuf[3],  /* long */
                                    cscbuf[4],  /* long long */
                                    cscbuf[5]); /* ptr */
    if (sfstatus > SF_UPTODATE
        && ((sfstatus <= SF_CRITICAL_BYTE_COUNT_MISMATCH) || !unconvert)) {
        if (sfstatus == SF_OUTDATED) {
            fprintf(stderr,
                "The %s savefile is outdated with respect to this %d.%d.%d EDITLEVEL %ld "
                "%s%s%s.\n",
                briefname(srcfnam),
                VERSION_MAJOR, VERSION_MINOR, PATCHLEVEL,
                (long) EDITLEVEL,
                thisdatamodel ? thisdatamodel : "",
                thisdatamodel ? " " : "",
                "sfctool");
            return 0;
        } else {
            fprintf(stderr,
                "The %s savefile %s%s%s not compatible with this %s%s %s utility.\n",
                briefname(srcfnam),
                dmfile ? "is a " : "",
                dmfile ? dmfile : "",
                dmfile ? " savefile, thus" : " is",
                thisdatamodel ? thisdatamodel : "",
                thisdatamodel ? " " : "",
                "sfctool");
            return 0;
        }
    }
    if (sfstatus >= SF_DM_IL32LLP64_ON_ILP32LL64) {
        renidx = sfstatus - SF_DM_IL32LLP64_ON_ILP32LL64;
    } else if (sfstatus == SF_UPTODATE) {
        renidx = -2;
    }
    if ((nhfp[dstidx] = create_dstfile(dstfnam, cvtstyle)) == 0) {
        close_nhfile(nhfp[dstidx]);
        nh_compress(unconverted_filename);
        return 0;
    }
    if (unconvert) {
        nhfp[srcidx]->nhfpconvert = nhfp[UNCONVERTED];
    } else {
        /* converting */
        nhfp[srcidx]->nhfpconvert = nhfp[CONVERTED];
    }
    if (unconvert)
        fprintf(stdout, "\n\nunconverting %s to %s savefile called %s.\n",
                briefname((const char *) converted_filename),
                dmfile,
                briefname((const char *) unconverted_filename));
    else
        fprintf(stdout, "\n\nconverting %s %s format %s to %s format\n",
                style_to_text(srcstyle),
                thisdatamodel,
                briefname((const char *) unconverted_filename),
                style_to_text(cvtstyle));

    rewind_nhfile(nhfp[srcidx]);
#ifdef SAVEFILE_DEBUGGING
    nhfp[srcidx]->fplog = fopen(srclogfilenm, "w");
#endif
    if (unconvert) {
        nhfp[srcidx]->mode |= UNCONVERTING;
    } else {
        /* converting */
        nhfp[srcidx]->mode |= CONVERTING;
    }
    nhfp[srcidx]->rcount = 0;
    Sfi_char(nhfp[srcidx], &indicator, "indicate-format", 1);
    Sfi_char(nhfp[srcidx], &file_csc_count, "count-critical_sizes", 1);
    for (i = 0; i < (int) file_csc_count; ++i) {
        Sfi_uchar(nhfp[srcidx], &cscbuf[i], "critical_sizes");
    }
    rewind_nhfile(nhfp[dstidx]);
#ifdef SAVEFILE_DEBUGGING
    nhfp[dstidx]->fplog = fopen(dstlogfilenm, "w");
#endif
    /*
     * store_critical_bytes() will take care of inserting the
     * indicate-format, count-critical_sizes, and critical_sizes for
     * this platform/data-model destination, instead of copying those
     * values from the savefile that was converted.
     */
    store_critical_bytes(nhfp[dstidx]);

    Sfi_version_info(nhfp[srcidx], &vers_info, "version_info");
    svm.moves = 1L;  /* match u_init.c */

       /********************
        * player name info *
        ********************/

    get_plname_from_file(nhfp[srcidx], plname, TRUE);

    {
        /********************
         *    lev 0         *
         ********************/
        xint8 lev = 0;

        getlev(nhfp[srcidx], lev, FALSE);
    }


    {
        /********************
         *    gamestate     *
         ********************/
/*        unsigned int stuckid, steedid; */
        (void) restgamestate(nhfp[srcidx]);
    }

    {
        /********************
         * Do all levels    *
         ********************/
        xint8 ltmp;

        restoreinfo.mread_flags = 1; /* return despite error */
        while (1) {
            ltmp = -1;
            Sfi_xint8(nhfp[srcidx], &ltmp, "gamestate-level_number");
            if (nhfp[srcidx]->eof || ltmp == -1)
                break;

            getlev(nhfp[srcidx], 0, ltmp);
        }
        restoreinfo.mread_flags = 0;
    }
    nhfp[srcidx]->mode &= ~(CONVERTING | UNCONVERTING);
    nhfp[srcidx]->nhfpconvert = (NHFILE *) 0;
    close_nhfile(nhfp[srcidx]);
    close_nhfile(nhfp[dstidx]);
    nh_compress(dstfnam);
    nh_compress(srcfnam);
    return 1;
}

/* open srcfile for reading */
static NHFILE *
open_srcfile(const char *fnam, enum saveformats mystyle)
{
    int fd;
    const char *fq_name;
    NHFILE *nhfp = (NHFILE *) 0;

    nhfp = new_nhfile();
    if (nhfp) {
        nhfp->mode = READING;
        nhfp->structlevel = (mystyle == historical);
        nhfp->fieldlevel = (mystyle > historical);
        nhfp->ftype = NHF_SAVEFILE;
        nhfp->fnidx = mystyle;
        nhfp->fd = -1;
        nhfp->addinfo =
            (nhfp->fieldlevel && (mystyle = exportascii))
                ? TRUE
                : FALSE;
        (void) snprintf(srclogfilenm, sizeof srclogfilenm, "srcfile.%s.log",
                        (mystyle == historical) ? "historical" : "exportascii");
    }

    fq_name = fqname(fnam, SAVEPREFIX, 0);
    nh_uncompress(fq_name);
    if (nhfp && nhfp->structlevel) {
        fd = open(fq_name, O_RDONLY | O_BINARY, 0);
        if (fd < 0) {
            free_nhfile(nhfp);
            fprintf(stderr,
                    "\nsfctool error - unable to open historical-style "
                    "source file %s.\n",
                    fnam);
            nhfp = (NHFILE *) 0;
            nh_compress(fq_name);
        } else {
            nhfp->fd = fd;
#if defined(MSDOS)
            setmode(nhfp->fd, O_BINARY);
#endif
        }
    }
    if (nhfp && nhfp->fieldlevel) {
        /*        char savenamebuf[BUFSZ]; */

        nhfp->fpdef = fopen(fnam, RDBMODE);
        if (!nhfp->fpdef) {
            free_nhfile(nhfp);
            fprintf(stderr,
                    "\nsfctool error - unable to open fieldlevel-style "
                    "source file %s.\n",
                    fnam);
            nhfp = (NHFILE *) 0;
            nh_compress(fq_name);
        }
    }
    return nhfp;
}

/* create dst file, overwriting one if it already exists */
static NHFILE *
create_dstfile(char *fnam, enum saveformats mystyle)
{
    int fd, ret;
    unsigned ln = 0;
    FILE *cf;
    NHFILE *nhfp = (NHFILE *) 0;
    const char *fq_name;
    char dstfnam[2048];
    char *dsttmp;
    boolean dst_file_exists = FALSE, ren_file_exists = FALSE;

    nhUse(ret);
    Snprintf(dstfnam, sizeof dstfnam, "%s", fnam);
    if ((cf = fopen(dstfnam, RDBMODE)) != (FILE *) 0) {
        dst_file_exists = TRUE;
        (void) fclose(cf);
    }

    if (dst_file_exists) {
        if (chosen_unconvert) {
            ln = strlen(fnam);
            if (renidx >= 0) {
                ln += strlen(rensuffixes[renidx]) + 1; /* +1 for '.' */
            } else if (renidx == -2) {
                ln += strlen(thisdatamodel) + 1; /* +1 for '.' */
            } else {
                ln += strlen(thisdatamodel) + 1
                      + 4; /* +1 for '.'; +4 for "not_" */
            }
            dsttmp = (char *) alloc(ln + 1);
            Strcpy(dsttmp, fnam);
            Strcat(dsttmp, ".");
            if (renidx >= 0) {
                Strcat(dsttmp, rensuffixes[renidx]);
            } else if (renidx == -2) {
                Strcat(dsttmp, thisdatamodel);
            } else {
                Strcat(dsttmp, "not_");
                Strcat(dsttmp, thisdatamodel);
            }
            if ((cf = fopen(dsttmp, RDBMODE)) != (FILE *) 0) {
                ren_file_exists = TRUE;
                (void) fclose(cf);
            }
            if (ren_file_exists) {
                (void) unlink(dsttmp);
            }
            ret = rename(fnam, dsttmp);
            free((genericptr_t) dsttmp), dsttmp = 0;
        } else {
            if ((cf = fopen(dstfnam, RDBMODE)) != (FILE *) 0) {
                ren_file_exists = TRUE;
                (void) fclose(cf);
            }
            if (ren_file_exists) {
                (void) unlink(dstfnam);
            }
        }
    }

    nhfp = new_nhfile();
    if (nhfp) {
        nhfp->mode = WRITING;
        nhfp->ftype = NHF_SAVEFILE;
        nhfp->structlevel = (mystyle == historical);
        nhfp->fieldlevel = (mystyle > historical);
        nhfp->fnidx = mystyle;
        nhfp->fd = -1;
        (void) snprintf(dstlogfilenm, sizeof dstlogfilenm, "dstfile.%s.log",
                        (mystyle == historical) ? "historical" : "exportascii");
    }

    if (nhfp && nhfp->structlevel) {
        fq_name = fqname(dstfnam, SAVEPREFIX, 0);
#if defined(MICRO) || defined(WIN32)
        fd = open(dstfnam, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, FCMASK);
#else
        fd = creat(dstfnam, FCMASK);
#endif
        if (fd < 0) {
            free_nhfile(nhfp);
            fprintf(
                stderr,
                "Unable to create historical-style destination file %s.\n",
                fnam);
            nhfp = (NHFILE *) 0;
        } else {
            nhfp->fd = fd;
#if defined(MSDOS)
            setmode(nhfp->fd, O_BINARY);
#endif
        }
    } else if (nhfp && nhfp->fieldlevel) {
        fq_name = fqname(dstfnam, SAVEPREFIX, 0);
        nhfp->fpdef = fopen(fq_name, WRBMODE);
        if (!nhfp->fpdef) {
            free_nhfile(nhfp);
            fprintf(
                stderr,
                "Unable to create fieldlevel-style destination file %s.\n",
                fnam);
            nhfp = (NHFILE *) 0;
        }
    }
    return nhfp;
}

static const char *
briefname(const char *fnam)
{
    const char *bn = fnam, *sep = (const char *) 0;

    if ((sep = strrchr(fnam, '/')) || (sep = strrchr(fnam, '\\'))
        || (sep = strrchr(fnam, ':')))
        bn = sep + 1;
    return bn;
}

static const char *
style_to_text(enum saveformats style)
{
    const char *txt;

    switch (style) {
    case historical:
        txt = "historical";
        break;
    case exportascii:
        txt = "exportascii";
        break;
    case invalid:
    default:
        txt = "invalid";
        break;
    }
    return txt;
}

void
my_sf_init(void)
{
    decl_globals_init();
    sfoprocs[invalid] = zerosfoprocs;
    sfiprocs[invalid] = zerosfiprocs;
    sfoprocs[historical] = historical_sfo_procs;
    sfiprocs[historical] = historical_sfi_procs;
    sfoflprocs[exportascii] = exportascii_sfo_procs;
    sfiflprocs[exportascii] = exportascii_sfi_procs;
}

/* delete savefile */
int
delete_savefile(void)
{
    return 0; /* for restore_saved_game() (ex-xxxmain.c) test */
}

static void
read_sysconf(void)
{
#ifdef SYSCF
/* someday there may be other SYSCF alternatives besides text file */
#ifdef SYSCF_FILE
    /* If SYSCF_FILE is specified, it _must_ exist... */
    assure_syscf_file();
    config_error_init(TRUE, SYSCF_FILE, FALSE);
    if (!read_config_file(SYSCF_FILE, set_in_sysconf)) {
        if (config_error_done() && !iflags.initoptions_noterminate)
            nh_terminate(EXIT_FAILURE);
    }
    config_error_done();
    /*
     * TODO [maybe]: parse the sysopt entries which are space-separated
     * lists of usernames into arrays with one name per element.
     */
#endif
#endif /* SYSCF */
}

/* provided for linkage only */

DISABLE_WARNING_FORMAT_NONLITERAL

void
raw_printf(const char *line, ...)
{
    va_list the_args;

    va_start(the_args, line);
    vfprintf(stdout, line, the_args);
    va_end(the_args);
}

void
error(const char *s, ...)
{
    va_list the_args;

    va_start(the_args, s);
    vprintf(s, the_args);
    va_end(the_args);
    exit(EXIT_FAILURE);
}

void
pline(const char *s, ...)
{
    va_list the_args;

    va_start(the_args, s);
    vprintf(s, the_args);
    va_end(the_args);
}

void
impossible(const char *s, ...)
{
    va_list the_args;

    va_start(the_args, s);
    vprintf(s, the_args);
    va_end(the_args);
    exit(EXIT_FAILURE);
}

RESTORE_WARNING_FORMAT_NONLITERAL

#ifdef UNIX
/* normalize file name - we don't like .'s, /'s, spaces */
void
regularize(char *s)
{
    register char *lp;

    while ((lp = strchr(s, '.')) != 0 || (lp = strchr(s, '/')) != 0
           || (lp = strchr(s, ' ')) != 0)
        *lp = '_';
#if defined(SYSV) && !defined(AIX_31) && !defined(SVR4) && !defined(LINUX) \
    && !defined(__APPLE__)
/* avoid problems with 14 character file name limit */
#ifdef COMPRESS
    /* leave room for .e from error and .Z from compress appended to
     * save files */
    {
#ifdef COMPRESS_EXTENSION
        int i = 12 - strlen(COMPRESS_EXTENSION);
#else
        int i = 10; /* should never happen... */
#endif
        if (strlen(s) > i)
            s[i] = '\0';
    }
#else
    if (strlen(s) > 11)
        /* leave room for .nn appended to level files */
        s[11] = '\0';
#endif
#endif
}
#endif /* UNIX */

int
util_strncmpi(const char *s1, const char *s2, size_t sz)
{
    register char t1, t2;

    while (sz--) {
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

/* should be called with either EXIT_SUCCESS or EXIT_FAILURE */
void
nh_terminate(int status)
{
    nethack_exit(status);
}

#ifndef UNIX
void
nethack_exit(int code)
{
    exit(code);
}
#endif  /* UNIX */

#ifdef UNIX
#ifdef CHDIR
void
chdirx(const char *dir, boolean wr UNUSED)
{
    if (dir) {
#ifdef SECURE
        (void) setgid(getgid());
        (void) setuid(getuid()); /* Ron Wessels */
#endif
    } else {
        /* non-default data files is a sign that scores may not be
         * compatible, or perhaps that a binary not fitting this
         * system's layout is being used.
         */
#ifdef VAR_PLAYGROUND
        int len = strlen(VAR_PLAYGROUND);

        gf.fqn_prefix[SCOREPREFIX] = (char *) alloc(len + 2);
        Strcpy(gf.fqn_prefix[SCOREPREFIX], VAR_PLAYGROUND);
        if (gf.fqn_prefix[SCOREPREFIX][len - 1] != '/') {
            gf.fqn_prefix[SCOREPREFIX][len] = '/';
            gf.fqn_prefix[SCOREPREFIX][len + 1] = '\0';
        }

#endif
    }

#ifdef HACKDIR
    if (dir == (const char *) 0)
        dir = HACKDIR;
#endif

    if (dir && chdir(dir) < 0) {
        perror(dir);
        error("Cannot chdir to %s.", dir);
    }
}
#endif /* CHDIR */
#endif /* UNIX */

#ifdef WIN32

/*
 * Strip out troublesome file system characters.
 */

void nt_regularize(char* s) /* normalize file name */
{
    unsigned char *lp;

    for (lp = (unsigned char *) s; *lp; lp++)
        if (*lp == '?' || *lp == '"' || *lp == '\\' || *lp == '/'
            || *lp == '>' || *lp == '<' || *lp == '*' || *lp == '|'
            || *lp == ':' || (*lp > 127))
            *lp = '_';
}
#endif /* WIN32 */

/* duplicated code from options.c */
/* most environment variables will eventually be printed in an error
 * message if they don't work, and most error message paths go through
 * BUFSZ buffers, which could be overflowed by a maliciously long
 * environment variable.  If a variable can legitimately be long, or
 * if it's put in a smaller buffer, the responsible code will have to
 * bounds-check itself.
 */
char *
nh_getenv(const char *ev)
{
    char *getev = getenv(ev);

    if (getev && strlen(getev) <= (BUFSZ / 2))
        return getev;
    else
        return (char *) 0;
}

void
done1(int sig_unused UNUSED)
{
#ifndef NO_SIGNAL
    (void) signal(SIGINT, SIG_IGN);
#endif
    if (flags.ignintr) {
#ifndef NO_SIGNAL
        (void) signal(SIGINT, (SIG_RET_TYPE) done1);
#endif
    }
}

/*
 * I hate having to duplicate this code here, but it is much simpler to
 * add these here than take steps to link with mkobj.c, do_name.c, priest.c,
 * vault.c, shknam.c, minion.c, dog.c, etc.
 */

/* allocate space for a monster's name; removes old name if there is one */
void
new_mgivenname(struct monst *mon,
               int lth) /* desired length (caller handles adding 1
                           for terminator) */
{
    if (lth) {
        /* allocate mextra if necessary; otherwise get rid of old name */
        if (!mon->mextra)
            mon->mextra = newmextra();
        else
            free_mgivenname(mon); /* already has mextra, might also have name */
        MGIVENNAME(mon) = (char *) alloc((unsigned) lth);
    } else {
        /* zero length: the new name is empty; get rid of the old name */
        if (has_mgivenname(mon))
            free_mgivenname(mon);
    }
}

/* release a monster's name; retains mextra even if all fields are now null */
void
free_mgivenname(struct monst *mon)
{
    if (has_mgivenname(mon)) {
        free((genericptr_t) MGIVENNAME(mon));
        MGIVENNAME(mon) = (char *) 0;
    }
}
void
newegd(struct monst *mtmp)
{
    if (!mtmp->mextra)
        mtmp->mextra = newmextra();
    if (!EGD(mtmp)) {
        EGD(mtmp) = (struct egd *) alloc(sizeof (struct egd));
        (void) memset((genericptr_t) EGD(mtmp), 0, sizeof (struct egd));
    }
}

void
free_egd(struct monst *mtmp)
{
    if (mtmp->mextra && EGD(mtmp)) {
        free((genericptr_t) EGD(mtmp));
        EGD(mtmp) = (struct egd *) 0;
    }
    mtmp->isgd = 0;
}
void
newepri(struct monst *mtmp)
{
    if (!mtmp->mextra)
        mtmp->mextra = newmextra();
    if (!EPRI(mtmp)) {
        EPRI(mtmp) = (struct epri *) alloc(sizeof(struct epri));
        (void) memset((genericptr_t) EPRI(mtmp), 0, sizeof(struct epri));
    }
}

void
free_epri(struct monst *mtmp)
{
    if (mtmp->mextra && EPRI(mtmp)) {
        free((genericptr_t) EPRI(mtmp));
        EPRI(mtmp) = (struct epri *) 0;
    }
    mtmp->ispriest = 0;
}
void
neweshk(struct monst* mtmp)
{
    if (!mtmp->mextra)
        mtmp->mextra = newmextra();
    if (!ESHK(mtmp))
        ESHK(mtmp) = (struct eshk *) alloc(sizeof(struct eshk));
    (void) memset((genericptr_t) ESHK(mtmp), 0, sizeof(struct eshk));
    ESHK(mtmp)->bill_p = (struct bill_x *) 0;
}

void
free_eshk(struct monst* mtmp)
{
    if (mtmp->mextra && ESHK(mtmp)) {
        free((genericptr_t) ESHK(mtmp));
        ESHK(mtmp) = (struct eshk *) 0;
    }
    mtmp->isshk = 0;
}

void
newemin(struct monst *mtmp)
{
    if (!mtmp->mextra)
        mtmp->mextra = newmextra();
    if (!EMIN(mtmp)) {
        EMIN(mtmp) = (struct emin *) alloc(sizeof(struct emin));
        (void) memset((genericptr_t) EMIN(mtmp), 0, sizeof(struct emin));
    }
}

void
free_emin(struct monst *mtmp)
{
    if (mtmp->mextra && EMIN(mtmp)) {
        free((genericptr_t) EMIN(mtmp));
        EMIN(mtmp) = (struct emin *) 0;
    }
    mtmp->isminion = 0;
}

void
newedog(struct monst *mtmp)
{
    if (!mtmp->mextra)
        mtmp->mextra = newmextra();
    if (!EDOG(mtmp)) {
        EDOG(mtmp) = (struct edog *) alloc(sizeof(struct edog));
        (void) memset((genericptr_t) EDOG(mtmp), 0, sizeof(struct edog));
    }
}

void
free_edog(struct monst *mtmp)
{
    if (mtmp->mextra && EDOG(mtmp)) {
        free((genericptr_t) EDOG(mtmp));
        EDOG(mtmp) = (struct edog *) 0;
    }
    mtmp->mtame = 0;
}

void
newebones(struct monst *mtmp)
{
    if (!mtmp->mextra)
        mtmp->mextra = newmextra();
    if (!EBONES(mtmp)) {
        EBONES(mtmp) = (struct ebones *) alloc(sizeof(struct ebones));
        (void) memset((genericptr_t) EBONES(mtmp), 0, sizeof(struct ebones));
    }
}

void
free_ebones(struct monst *mtmp)
{
    if (mtmp->mextra && EBONES(mtmp)) {
        free((genericptr_t) EBONES(mtmp));
        EBONES(mtmp) = (struct ebones *) 0;
    }
}

static const struct mextra zeromextra = DUMMY;

static void
init_mextra(struct mextra *mex)
{
    *mex = zeromextra;
    mex->mcorpsenm = NON_PM;
}

struct mextra *
newmextra(void)
{
    struct mextra *mextra;

    mextra = (struct mextra *) alloc(sizeof(struct mextra));
    init_mextra(mextra);
    return mextra;
}

void
newomonst(struct obj* otmp)
{
    if (!otmp->oextra)
        otmp->oextra = newoextra();

    if (!OMONST(otmp)) {
        struct monst *m = newmonst();

        *m = cg.zeromonst;
        OMONST(otmp) = m;
    }
}

void
free_omonst(struct obj* otmp)
{
    if (otmp->oextra) {
        struct monst *m = OMONST(otmp);

        if (m) {
            if (m->mextra)
                dealloc_mextra(m);
            free((genericptr_t) m);
            OMONST(otmp) = (struct monst *) 0;
        }
    }
}

void
newomid(struct obj* otmp)
{
    if (!otmp->oextra)
        otmp->oextra = newoextra();
    OMID(otmp) = 0;
}

void
free_omid(struct obj* otmp)
{
    OMID(otmp) = 0;
}

void
new_omailcmd(struct obj* otmp, const char * response_cmd)
{
    if (!otmp->oextra)
        otmp->oextra = newoextra();
    if (OMAILCMD(otmp))
        free_omailcmd(otmp);
    OMAILCMD(otmp) = dupstr(response_cmd);
}

void
free_omailcmd(struct obj* otmp)
{
    if (otmp->oextra && OMAILCMD(otmp)) {
        free((genericptr_t) OMAILCMD(otmp));
        OMAILCMD(otmp) = (char *) 0;
    }
}

static const struct oextra zerooextra = DUMMY;

static void
init_oextra(struct oextra* oex)
{
    *oex = zerooextra;
}


struct oextra *
newoextra(void)
{
    struct oextra *oextra;

    oextra = (struct oextra *) alloc(sizeof (struct oextra));
    init_oextra(oextra);
    return oextra;
}

void
dealloc_mextra(struct monst* m)
{
    struct mextra *x = m->mextra;

    if (x) {
        if (x->mgivenname)
            free((genericptr_t) x->mgivenname);
        if (x->egd)
            free((genericptr_t) x->egd);
        if (x->epri)
            free((genericptr_t) x->epri);
        if (x->eshk)
            free((genericptr_t) x->eshk);
        if (x->emin)
            free((genericptr_t) x->emin);
        if (x->edog)
            free((genericptr_t) x->edog);
        if (x->ebones)
            free((genericptr_t) x->ebones);
        /* [no action needed for x->mcorpsenm] */

        free((genericptr_t) x);
        m->mextra = (struct mextra *) 0;
    }
}

void
dealloc_monst(struct monst* mon)
{
    if (mon->mextra)
        dealloc_mextra(mon);
    free((genericptr_t) mon);
}

/* allocate space for an object's name; removes old name if there is one */
void
new_oname(struct obj *obj,
          int lth) /* desired length (caller handles adding 1
                      for terminator) */
{
    if (lth) {
        /* allocate oextra if necessary; otherwise get rid of old name */
        if (!obj->oextra)
            obj->oextra = newoextra();
        else
            free_oname(obj); /* already has oextra, might also have name */
        ONAME(obj) = (char *) alloc((unsigned) lth);
    } else {
        /* zero length: the new name is empty; get rid of the old name */
        if (has_oname(obj))
            free_oname(obj);
    }
}

/* release an object's name; retains oextra even if all fields are now null */
void
free_oname(struct obj *obj)
{
    if (has_oname(obj)) {
        free((genericptr_t) ONAME(obj));
        ONAME(obj) = (char *) 0;
    }
}

#ifdef WIN32
void
win32_abort(void)
{
    abort();
}
#endif

static int
length_without_val(const char *user_string, int len)
{
    const char *p = strchr(user_string, ':'), *q = strchr(user_string, '=');

    if (!p || (q && q < p))
        p = q;
    if (p) {
        /* 'user_string' hasn't necessarily been through mungspaces()
           so might have tabs or consecutive spaces */
        while (p > user_string && isspace((uchar) * (p - 1)))
            p--;
        len = (int) (p - user_string);
    }
    return len;
}

/* check whether a user-supplied option string is a proper leading
   substring of a particular option name; option string might have
   a colon or equals sign and arbitrary value appended to it */
boolean
match_optname(const char *user_string, const char *optn_name, int min_length,
              boolean val_allowed)
{
    int len = (int) strlen(user_string);

    if (val_allowed)
        len = length_without_val(user_string, len);

    return (boolean) (len >= min_length
                      && !strncmpi(optn_name, user_string, len));
}

staticfn void
usage(int argc UNUSED, char **argv)
{
    char *cp = argv[0], *sep = (char *) 0;

    if ((sep = strrchr(cp, '/')) || (sep = strrchr(cp, '\\'))
        || (sep = strrchr(cp, ':')))
        cp = sep + 1;
    fprintf(stderr,
            "\nTo convert a savefile to export format:\n    %s %s %s\n", cp,
            "-c", "savefile");
    fprintf(stderr,
            "\nTo unconvert an exported savefile back into a savefile:\n    %s %s %s\n", cp,
            "-u", "savefile");
}
/* sfctool.c */
