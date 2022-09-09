/* NetHack 3.7	files.c	$NHDT-Date: 1654069053 2022/06/01 07:37:33 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.351 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Derek S. Ray, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

#define NEED_VARARGS

#include "hack.h"
#include "dlb.h"
#include <ctype.h>

#ifdef TTY_GRAPHICS
#include "wintty.h" /* more() */
#endif

#if (!defined(MAC) && !defined(O_WRONLY) && !defined(AZTEC_C)) \
    || defined(USE_FCNTL)
#include <fcntl.h>
#endif

#include <errno.h>
#ifdef _MSC_VER /* MSC 6.0 defines errno quite differently */
#if (_MSC_VER >= 600)
#define SKIP_ERRNO
#endif
#else
#ifdef NHSTDC
#define SKIP_ERRNO
#endif
#endif
#ifndef SKIP_ERRNO
#ifdef _DCC
const
#endif
    extern int errno;
#endif

#ifdef ZLIB_COMP /* RLC 09 Mar 1999: Support internal ZLIB */
#include "zlib.h"
#ifndef COMPRESS_EXTENSION
#define COMPRESS_EXTENSION ".gz"
#endif
#endif

#if defined(UNIX) && defined(SELECTSAVED)
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#endif

#if defined(UNIX) || defined(VMS) || !defined(NO_SIGNAL)
#include <signal.h>
#endif

#if defined(MSDOS) || defined(OS2) || defined(TOS) || defined(WIN32)
#ifndef __DJGPP__
#include <sys\stat.h>
#else
#include <sys/stat.h>
#endif
#endif
#ifndef O_BINARY /* used for micros, no-op for others */
#define O_BINARY 0
#endif

#ifdef PREFIXES_IN_USE
#define FQN_NUMBUF 8
static char fqn_filename_buffer[FQN_NUMBUF][FQN_MAX_FILENAME];
#endif

#if !defined(SAVE_EXTENSION)
#ifdef MICRO
#define SAVE_EXTENSION ".sav"
#endif
#ifdef WIN32
#define SAVE_EXTENSION ".NetHack-saved-game"
#endif
#endif

#if defined(WIN32)
#include <share.h>
#endif

#ifdef AMIGA
extern char PATH[]; /* see sys/amiga/amidos.c */
extern char bbs_id[];
#ifdef __SASC_60
#include <proto/dos.h>
#endif

#include <libraries/dos.h>
extern void amii_set_text_font(char *, int);
#endif

#if defined(WIN32) || defined(MSDOS)
#ifdef MSDOS
#define Delay(a) msleep(a)
#endif
#define Close close
#ifndef WIN_CE
#define DeleteFile unlink
#endif
#ifdef WIN32
/*from windmain.c */
extern char *translate_path_variables(const char *, char *);
#endif
#endif

#ifdef MAC
#undef unlink
#define unlink macunlink
#endif

#if (defined(macintosh) && (defined(__SC__) || defined(__MRC__))) \
    || defined(__MWERKS__)
#define PRAGMA_UNUSED
#endif

#ifdef USER_SOUNDS
extern char *sounddir; /* defined in sounds.c */
#endif

static NHFILE *new_nhfile(void);
static void free_nhfile(NHFILE *);
#ifdef SELECTSAVED
static int QSORTCALLBACK strcmp_wrap(const void *, const void *);
#endif
static char *set_bonesfile_name(char *, d_level *);
static char *set_bonestemp_name(void);
#ifdef COMPRESS
static void redirect(const char *, const char *, FILE *, boolean);
#endif
#if defined(COMPRESS) || defined(ZLIB_COMP)
static void docompress_file(const char *, boolean);
#endif
#if defined(ZLIB_COMP)
static boolean make_compressed_name(const char *, char *);
#endif
#ifndef USE_FCNTL
static char *make_lockname(const char *, char *);
#endif
static void set_configfile_name(const char *);
static FILE *fopen_config_file(const char *, int);
static int get_uchars(char *, uchar *, boolean, int, const char *);
#ifdef NOCWD_ASSUMPTIONS
static void adjust_prefix(char *, int);
#endif
static char *choose_random_part(char *, char);
static boolean config_error_nextline(const char *);
static void free_config_sections(void);
static char *is_config_section(char *);
static boolean handle_config_section(char *);
boolean parse_config_line(char *);
static char *find_optparam(const char *);
struct _cnf_parser_state; /* defined below (far below...) */
static void cnf_parser_init(struct _cnf_parser_state *parser);
static void cnf_parser_done(struct _cnf_parser_state *parser);
static void parse_conf_buf(struct _cnf_parser_state *parser,
                           boolean (*proc)(char *arg));
boolean parse_conf_str(const char *str, boolean (*proc)(char *arg));
static boolean parse_conf_file(FILE *fp, boolean (*proc)(char *arg));
static void parseformat(int *, char *);
static FILE *fopen_wizkit_file(void);
static void wizkit_addinv(struct obj *);
boolean proc_wizkit_line(char *buf);
void read_wizkit(void);
static FILE *fopen_sym_file(void);

#ifdef SELF_RECOVER
static boolean copy_bytes(int, int);
#endif
static NHFILE *viable_nhfile(NHFILE *);

/*
 * fname_encode()
 *
 *   Args:
 *      legal       zero-terminated list of acceptable file name characters
 *      quotechar   lead-in character used to quote illegal characters as
 *                  hex digits
 *      s           string to encode
 *      callerbuf   buffer to house result
 *      bufsz       size of callerbuf
 *
 *   Notes:
 *      The hex digits 0-9 and A-F are always part of the legal set due to
 *      their use in the encoding scheme, even if not explicitly included in
 *      'legal'.
 *
 *   Sample:
 *      The following call:
 *  (void)fname_encode("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz",
 *                     '%', "This is a % test!", buf, 512);
 *      results in this encoding:
 *          "This%20is%20a%20%25%20test%21"
 */
char *
fname_encode(
    const char *legal,
    char quotechar,
    char *s,
    char *callerbuf,
    int bufsz)
{
    char *sp, *op;
    int cnt = 0;
    static char hexdigits[] = "0123456789ABCDEF";

    sp = s;
    op = callerbuf;
    *op = '\0';

    while (*sp) {
        /* Do we have room for one more character or encoding? */
        if ((bufsz - cnt) <= 4)
            return callerbuf;

        if (*sp == quotechar) {
            (void) sprintf(op, "%c%02X", quotechar, *sp);
            op += 3;
            cnt += 3;
        } else if ((index(legal, *sp) != 0) || (index(hexdigits, *sp) != 0)) {
            *op++ = *sp;
            *op = '\0';
            cnt++;
        } else {
            (void) sprintf(op, "%c%02X", quotechar, *sp);
            op += 3;
            cnt += 3;
        }
        sp++;
    }
    return callerbuf;
}

/*
 * fname_decode()
 *
 *   Args:
 *      quotechar   lead-in character used to quote illegal characters as
 *                  hex digits
 *      s           string to decode
 *      callerbuf   buffer to house result
 *      bufsz       size of callerbuf
 */
char *
fname_decode(char quotechar, char *s, char *callerbuf, int bufsz)
{
    char *sp, *op;
    int k, calc, cnt = 0;
    static char hexdigits[] = "0123456789ABCDEF";

    sp = s;
    op = callerbuf;
    *op = '\0';
    calc = 0;

    while (*sp) {
        /* Do we have room for one more character? */
        if ((bufsz - cnt) <= 2)
            return callerbuf;
        if (*sp == quotechar) {
            sp++;
            for (k = 0; k < 16; ++k)
                if (*sp == hexdigits[k])
                    break;
            if (k >= 16)
                return callerbuf; /* impossible, so bail */
            calc = k << 4;
            sp++;
            for (k = 0; k < 16; ++k)
                if (*sp == hexdigits[k])
                    break;
            if (k >= 16)
                return callerbuf; /* impossible, so bail */
            calc += k;
            sp++;
            *op++ = calc;
            *op = '\0';
        } else {
            *op++ = *sp++;
            *op = '\0';
        }
        cnt++;
    }
    return callerbuf;
}

#ifdef PREFIXES_IN_USE
#define UNUSED_if_not_PREFIXES_IN_USE /*empty*/
#else
#define UNUSED_if_not_PREFIXES_IN_USE UNUSED
#endif

/*ARGSUSED*/
const char *
fqname(const char *basenam,
       int whichprefix UNUSED_if_not_PREFIXES_IN_USE,
       int buffnum UNUSED_if_not_PREFIXES_IN_USE)
{
#ifdef PREFIXES_IN_USE
    char *bufptr;
#endif
#ifdef WIN32
    char tmpbuf[BUFSZ];
#endif

#ifndef PREFIXES_IN_USE
    return basenam;
#else
    if (!basenam || whichprefix < 0 || whichprefix >= PREFIX_COUNT)
        return basenam;
    if (!g.fqn_prefix[whichprefix])
        return basenam;
    if (buffnum < 0 || buffnum >= FQN_NUMBUF) {
        impossible("Invalid fqn_filename_buffer specified: %d", buffnum);
        buffnum = 0;
    }
    bufptr = g.fqn_prefix[whichprefix];
#ifdef WIN32
    if (strchr(g.fqn_prefix[whichprefix], '%')
        || strchr(g.fqn_prefix[whichprefix], '~'))
        bufptr = translate_path_variables(g.fqn_prefix[whichprefix], tmpbuf);
#endif
    if (strlen(bufptr) + strlen(basenam) >= FQN_MAX_FILENAME) {
        impossible("fqname too long: %s + %s", bufptr, basenam);
        return basenam; /* XXX */
    }
    Strcpy(fqn_filename_buffer[buffnum], bufptr);
    return strcat(fqn_filename_buffer[buffnum], basenam);
#endif /* !PREFIXES_IN_USE */
}

/* reasonbuf must be at least BUFSZ, supplied by caller */
int
validate_prefix_locations(char *reasonbuf)
{
#if defined(NOCWD_ASSUMPTIONS)
    FILE *fp;
    const char *filename;
    int prefcnt, failcount = 0;
    char panicbuf1[BUFSZ], panicbuf2[BUFSZ];
    const char *details;
#endif

    if (reasonbuf)
        reasonbuf[0] = '\0';
#if defined(NOCWD_ASSUMPTIONS)
    for (prefcnt = 1; prefcnt < PREFIX_COUNT; prefcnt++) {
        /* don't test writing to configdir or datadir; they're readonly */
        if (prefcnt == SYSCONFPREFIX || prefcnt == CONFIGPREFIX
            || prefcnt == DATAPREFIX)
            continue;
        filename = fqname("validate", prefcnt, 3);
        if ((fp = fopen(filename, "w"))) {
            fclose(fp);
            (void) unlink(filename);
        } else {
            if (reasonbuf) {
                if (failcount)
                    Strcat(reasonbuf, ", ");
                Strcat(reasonbuf, fqn_prefix_names[prefcnt]);
            }
            /* the paniclog entry gets the value of errno as well */
            Sprintf(panicbuf1, "Invalid %s", fqn_prefix_names[prefcnt]);
#if defined(NHSTDC) && !defined(NOTSTDC)
            if (!(details = strerror(errno)))
#endif
                details = "";
            Sprintf(panicbuf2, "\"%s\", (%d) %s",
                    g.fqn_prefix[prefcnt], errno, details);
            paniclog(panicbuf1, panicbuf2);
            failcount++;
        }
    }
    if (failcount)
        return 0;
    else
#endif
        return 1;
}

/* fopen a file, with OS-dependent bells and whistles */
/* NOTE: a simpler version of this routine also exists in util/dlb_main.c */
FILE *
fopen_datafile(const char *filename, const char *mode, int prefix)
{
    FILE *fp;

    filename = fqname(filename, prefix, prefix == TROUBLEPREFIX ? 3 : 0);
    fp = fopen(filename, mode);
    return fp;
}

/* ----------  EXTERNAL FILE SUPPORT ----------- */

/* determine byte order */
const int bei = 1;
#define IS_BIGENDIAN() ( (*(char*)&bei) == 0 )

void
zero_nhfile(NHFILE *nhfp)
{
    if (nhfp) {
        nhfp->fd = -1;
        nhfp->mode = COUNTING;
        nhfp->structlevel = FALSE;
        nhfp->fieldlevel = FALSE;
        nhfp->addinfo = FALSE;
        nhfp->bendian = IS_BIGENDIAN();
        nhfp->fpdef = (FILE *)0;
        nhfp->fplog = (FILE *)0;
        nhfp->fpdebug = (FILE *)0;
        nhfp->count = 0;
        nhfp->eof = FALSE;
        nhfp->fnidx = 0;
    }
}

static NHFILE *
new_nhfile(void)
{
    NHFILE *nhfp = (NHFILE *)alloc(sizeof(NHFILE));

    zero_nhfile(nhfp);
    return nhfp;
}

static void
free_nhfile(NHFILE *nhfp)
{
    if (nhfp) {
        zero_nhfile(nhfp);
        free(nhfp);
    }
}

void
close_nhfile(NHFILE *nhfp)
{
    if (nhfp) {
        if (nhfp->structlevel && nhfp->fd != -1)
            (void) nhclose(nhfp->fd), nhfp->fd = -1;
        zero_nhfile(nhfp);
        free_nhfile(nhfp);
    }
}

void
rewind_nhfile(NHFILE *nhfp)
{
    if (nhfp->structlevel) {
#ifdef BSD
        (void) lseek(nhfp->fd, 0L, 0);
#else
        (void) lseek(nhfp->fd, (off_t) 0, 0);
#endif
    }
}

static
NHFILE *
viable_nhfile(NHFILE *nhfp)
{
    /* perform some sanity checks before returning
       the pointer to the nethack file descriptor */
    if (nhfp) {
         /* check for no open file at all,
          * not a structlevel legacy file
          */
         if (nhfp->structlevel && nhfp->fd < 0) {
            /* not viable, start the cleanup */
            zero_nhfile(nhfp);
            free_nhfile(nhfp);
            nhfp = (NHFILE *) 0;
        }
    }
    return nhfp;
}

/* ----------  BEGIN LEVEL FILE HANDLING ----------- */

/* Construct a file name for a level-type file, which is of the form
 * something.level (with any old level stripped off).
 * This assumes there is space on the end of 'file' to append
 * a two digit number.  This is true for 'level'
 * but be careful if you use it for other things -dgk
 */
void
set_levelfile_name(char *file, int lev)
{
    char *tf;

    tf = rindex(file, '.');
    if (!tf)
        tf = eos(file);
    Sprintf(tf, ".%d", lev);
#ifdef VMS
    Strcat(tf, ";1");
#endif
    return;
}

NHFILE *
create_levelfile(int lev, char errbuf[])
{
    const char *fq_lock;
    NHFILE *nhfp = (NHFILE *) 0;

    if (errbuf)
        *errbuf = '\0';
    set_levelfile_name(g.lock, lev);
    fq_lock = fqname(g.lock, LEVELPREFIX, 0);

    nhfp = new_nhfile();
    if (nhfp) {
        nhfp->ftype = NHF_LEVELFILE;
        nhfp->mode = WRITING;
        nhfp->structlevel = TRUE; /* do set this TRUE for levelfiles */
        nhfp->fieldlevel = FALSE; /* don't set this TRUE for levelfiles */
        nhfp->addinfo = FALSE;
        nhfp->style.deflt = FALSE;
        nhfp->style.binary = TRUE;
        nhfp->fd = -1;
        nhfp->fpdef = (FILE *) 0;
#if defined(MICRO) || defined(WIN32)
        /* Use O_TRUNC to force the file to be shortened if it already
         * exists and is currently longer.
         */
        nhfp->fd = open(fq_lock, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY,
                        FCMASK);
#else
#ifdef MAC
        nhfp->fd = maccreat(fq_lock, LEVL_TYPE);
#else
        nhfp->fd = creat(fq_lock, FCMASK);
#endif
#endif /* MICRO || WIN32 */

        if (nhfp->fd >= 0)
            g.level_info[lev].flags |= LFILE_EXISTS;
        else if (errbuf) /* failure explanation */
            Sprintf(errbuf,
                    "Cannot create file \"%s\" for level %d (errno %d).",
                    g.lock, lev, errno);
    }
    nhfp = viable_nhfile(nhfp);
    return nhfp;
}

NHFILE *
open_levelfile(int lev, char errbuf[])
{
    const char *fq_lock;
    NHFILE *nhfp = (NHFILE *) 0;

    if (errbuf)
        *errbuf = '\0';
    set_levelfile_name(g.lock, lev);
    fq_lock = fqname(g.lock, LEVELPREFIX, 0);
    nhfp = new_nhfile();
    if (nhfp) {
        nhfp->mode = READING;
        nhfp->structlevel = TRUE; /* do set this TRUE for levelfiles */
        nhfp->fieldlevel = FALSE; /* do not set this TRUE for levelfiles */
        nhfp->addinfo = FALSE;
        nhfp->style.deflt = FALSE;
        nhfp->style.binary = TRUE;
        nhfp->ftype = NHF_LEVELFILE;
        nhfp->fd = -1;
        nhfp->fpdef = (FILE *) 0;
    }
    if (nhfp && nhfp->structlevel) {
#ifdef MAC
        nhfp->fd = macopen(fq_lock, O_RDONLY | O_BINARY, LEVL_TYPE);
#else
        nhfp->fd = open(fq_lock, O_RDONLY | O_BINARY, 0);
#endif

        /* for failure, return an explanation that our caller can use;
           settle for `lock' instead of `fq_lock' because the latter
           might end up being too big for nethack's BUFSZ */
        if (nhfp->fd < 0 && errbuf)
            Sprintf(errbuf,
                    "Cannot open file \"%s\" for level %d (errno %d).",
                    g.lock, lev, errno);
    }
    nhfp = viable_nhfile(nhfp);
    return nhfp;
}

void
delete_levelfile(int lev)
{
    /*
     * Level 0 might be created by port specific code that doesn't
     * call create_levfile(), so always assume that it exists.
     */
    if (lev == 0 || (g.level_info[lev].flags & LFILE_EXISTS)) {
        set_levelfile_name(g.lock, lev);
        (void) unlink(fqname(g.lock, LEVELPREFIX, 0));
        g.level_info[lev].flags &= ~LFILE_EXISTS;
    }
}

void
clearlocks(void)
{
    int x;

#ifdef HANGUPHANDLING
    if (g.program_state.preserve_locks)
        return;
#endif
#ifndef NO_SIGNAL
    (void) signal(SIGINT, SIG_IGN);
#if defined(UNIX) || defined(VMS)
    sethanguphandler((void (*)(int)) SIG_IGN);
#endif
#endif /* NO_SIGNAL */
    /* can't access maxledgerno() before dungeons are created -dlc */
    for (x = (g.n_dgns ? maxledgerno() : 0); x >= 0; x--)
        delete_levelfile(x); /* not all levels need be present */
}

#if defined(SELECTSAVED)
/* qsort comparison routine */
static int QSORTCALLBACK
strcmp_wrap(const void *p, const void *q)
{
#if defined(UNIX) && defined(QT_GRAPHICS)
    return strncasecmp(*(char **) p, *(char **) q, 16);
#else
    return strncmpi(*(char **) p, *(char **) q, 16);
#endif
}
#endif

int
nhclose(int fd)
{
    int retval = 0;

    if (fd >= 0) {
        if (close_check(fd))
            bclose(fd);
        else
            retval = close(fd);
    }
    return retval;
}

/* ----------  END LEVEL FILE HANDLING ----------- */

/* ----------  BEGIN BONES FILE HANDLING ----------- */

/* set up "file" to be file name for retrieving bones, and return a
 * bonesid to be read/written in the bones file.
 */
static char *
set_bonesfile_name(char *file, d_level *lev)
{
    s_level *sptr;
    char *dptr;

    /*
     * "bonD0.nn"   = bones for level nn in the main dungeon;
     * "bonM0.T"    = bones for Minetown;
     * "bonQBar.n"  = bones for level n in the Barbarian quest;
     * "bon3D0.nn"  = \
     * "bon3M0.T"   =  > same as above, but for bones pool #3.
     * "bon3QBar.n" = /
     *
     * Return value for content validation skips "bon" and the
     * pool number (if present), making it feasible for the admin
     * to manually move a bones file from one pool to another by
     * renaming it.
     */
    Strcpy(file, "bon");
#ifdef SYSCF
    if (sysopt.bones_pools > 1) {
        unsigned poolnum = min((unsigned) sysopt.bones_pools, 10);

        poolnum = (unsigned) ubirthday % poolnum; /* 0..9 */
        Sprintf(eos(file), "%u", poolnum);
    }
#endif
    dptr = eos(file);
    /* when this naming scheme was adopted, 'filecode' was one letter;
       3.3.0 turned it into a three letter string for quest levels */
    Sprintf(dptr, "%c%s", g.dungeons[lev->dnum].boneid,
            In_quest(lev) ? g.urole.filecode : "0");
    if ((sptr = Is_special(lev)) != 0)
        Sprintf(eos(dptr), ".%c", sptr->boneid);
    else
        Sprintf(eos(dptr), ".%d", lev->dlevel);
#ifdef VMS
    Strcat(dptr, ";1");
#endif
    return dptr;
}

/* set up temporary file name for writing bones, to avoid another game's
 * trying to read from an uncompleted bones file.  we want an uncontentious
 * name, so use one in the namespace reserved for this game's level files.
 * (we are not reading or writing level files while writing bones files, so
 * the same array may be used instead of copying.)
 */
static char *
set_bonestemp_name(void)
{
    char *tf;

    tf = rindex(g.lock, '.');
    if (!tf)
        tf = eos(g.lock);
    Sprintf(tf, ".bn");
#ifdef VMS
    Strcat(tf, ";1");
#endif
    return g.lock;
}

NHFILE *
create_bonesfile(d_level *lev, char **bonesid, char errbuf[])
{
    const char *file;
    NHFILE *nhfp = (NHFILE *) 0;
    int failed = 0;

    if (errbuf)
        *errbuf = '\0';
    *bonesid = set_bonesfile_name(g.bones, lev);
    file = set_bonestemp_name();
    file = fqname(file, BONESPREFIX, 0);

    nhfp = new_nhfile();
    if (nhfp) {
        nhfp->structlevel = TRUE;
        nhfp->fieldlevel = FALSE;
        nhfp->ftype = NHF_BONESFILE;
        nhfp->mode = WRITING;
        if (nhfp->structlevel) {
#if defined(MICRO) || defined(WIN32)
            /* Use O_TRUNC to force the file to be shortened if it already
             * exists and is currently longer.
             */
            nhfp->fd = open(file,
                            O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, FCMASK);
#else
#ifdef MAC
            nhfp->fd = maccreat(file, BONE_TYPE);
#else
            nhfp->fd = creat(file, FCMASK);
#endif
#endif
            if (nhfp->fd < 0)
                failed = errno;
        }
        if (failed && errbuf)  /* failure explanation */
            Sprintf(errbuf, "Cannot create bones \"%s\", id %s (errno %d).",
                    g.lock, *bonesid, errno);
    }
#if defined(VMS) && !defined(SECURE)
    /*
       Re-protect bones file with world:read+write+execute+delete access.
       umask() doesn't seem very reliable; also, vaxcrtl won't let us set
       delete access without write access, which is what's really wanted.
       Can't simply create it with the desired protection because creat
       ANDs the mask with the user's default protection, which usually
       denies some or all access to world.
     */
    (void) chmod(file, FCMASK | 007); /* allow other users full access */
#endif /* VMS && !SECURE */

    nhfp = viable_nhfile(nhfp);
    return nhfp;
}

/* move completed bones file to proper name */
void
commit_bonesfile(d_level *lev)
{
    const char *fq_bones, *tempname;
    int ret;

    (void) set_bonesfile_name(g.bones, lev);
    fq_bones = fqname(g.bones, BONESPREFIX, 0);
    tempname = set_bonestemp_name();
    tempname = fqname(tempname, BONESPREFIX, 1);

#if (defined(SYSV) && !defined(SVR4)) || defined(GENIX)
    /* old SYSVs don't have rename.  Some SVR3's may, but since they
     * also have link/unlink, it doesn't matter. :-)
     */
    (void) unlink(fq_bones);
    ret = link(tempname, fq_bones);
    ret += unlink(tempname);
#else
    ret = rename(tempname, fq_bones);
#endif
    if (wizard && ret != 0)
        pline("couldn't rename %s to %s.", tempname, fq_bones);
}

NHFILE *
open_bonesfile(d_level *lev, char **bonesid)
{
    const char *fq_bones;
    NHFILE *nhfp = (NHFILE *) 0;

    *bonesid = set_bonesfile_name(g.bones, lev);
    fq_bones = fqname(g.bones, BONESPREFIX, 0);
    nh_uncompress(fq_bones);  /* no effect if nonexistent */

    nhfp = new_nhfile();
    if (nhfp) {
        nhfp->structlevel = TRUE;
        nhfp->fieldlevel = FALSE;
        nhfp->ftype = NHF_BONESFILE;
        nhfp->mode = READING;
        if (nhfp->structlevel) {
#ifdef MAC
            nhfp->fd = macopen(fq_bones, O_RDONLY | O_BINARY, BONE_TYPE);
#else
            nhfp->fd = open(fq_bones, O_RDONLY | O_BINARY, 0);
#endif
        }
    }
    nhfp = viable_nhfile(nhfp);
    return nhfp;
}

int
delete_bonesfile(d_level *lev)
{
    (void) set_bonesfile_name(g.bones, lev);
    return !(unlink(fqname(g.bones, BONESPREFIX, 0)) < 0);
}

/* assume we're compressing the recently read or created bonesfile, so the
 * file name is already set properly */
void
compress_bonesfile(void)
{
    nh_compress(fqname(g.bones, BONESPREFIX, 0));
}

/* ----------  END BONES FILE HANDLING ----------- */

/* ----------  BEGIN SAVE FILE HANDLING ----------- */

/* set savefile name in OS-dependent manner from pre-existing g.plname,
 * avoiding troublesome characters */
void
set_savefile_name(boolean regularize_it)
{
    int regoffset = 0, overflow = 0,
        indicator_spot = 0; /* 0=no indicator, 1=before ext, 2=after ext */
    const char *postappend = (const char *) 0,
               *sfindicator = (const char *) 0;
#if defined(WIN32)
    char tmp[BUFSZ];
#endif

#ifdef VMS
    Sprintf(g.SAVEF, "[.save]%d%s", getuid(), g.plname);
    regoffset = 7;
    indicator_spot = 1;
    postappend = ";1";
#endif
#if defined(WIN32)
    if (regularize_it) {
        static const char okchars[] =
            "*ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_-.";
        const char *legal = okchars;

        ++legal; /* skip '*' wildcard character */
        (void) fname_encode(legal, '%', g.plname, tmp, sizeof tmp);
    } else {
        Sprintf(tmp, "%s", g.plname);
    }
    if (strlen(tmp) < (SAVESIZE - 1))
        Strcpy(g.SAVEF, tmp);
    else
        overflow = 1;
    indicator_spot = 1;
    regularize_it = FALSE;
#endif
#ifdef UNIX
    Sprintf(g.SAVEF, "save/%d%s", (int) getuid(), g.plname);
    regoffset = 5;
    indicator_spot = 2;
#endif
#if defined(MSDOS)
    if (strlen(g.SAVEP) < (SAVESIZE - 1))
        Strcpy(g.SAVEF, g.SAVEP);
    if (strlen(g.SAVEF) < (SAVESIZE - 1))
        (void) strncat(g.SAVEF, g.plname, (SAVESIZE - strlen(g.SAVEF)));
#endif
#if defined(MICRO) && !defined(VMS) && !defined(WIN32) && !defined(MSDOS)
    if (strlen(g.SAVEP) < (SAVESIZE - 1))
        Strcpy(g.SAVEF, g.SAVEP);
    else
#ifdef AMIGA
        if (strlen(g.SAVEP) + strlen(bbs_id) < (SAVESIZE - 1))
            strncat(g.SAVEF, bbs_id, PATHLEN);
#endif
    {
        int i = strlen(g.SAVEP);
#ifdef AMIGA
        /* g.plname has to share space with g.SAVEP and ".sav" */
        (void) strncat(g.SAVEF, g.plname,
                       FILENAME - i - strlen(SAVE_EXTENSION));
#else
        (void) strncat(g.SAVEF, g.plname, 8);
#endif
        regoffset = i;
    }
#endif /* MICRO */

    if (regularize_it)
         regularize(g.SAVEF + regoffset);
    if (indicator_spot == 1 && sfindicator && !overflow) {
        if (strlen(g.SAVEF) + strlen(sfindicator) < (SAVESIZE - 1))
            Strcat(g.SAVEF, sfindicator);
        else
            overflow = 2;
    }
#ifdef SAVE_EXTENSION
    /* (0) is placed in brackets below so that the [&& !overflow] is
       explicit dead code (the ">" comparison is detected as always
       FALSE at compile-time). Done to appease clang's -Wunreachable-code */
    if (strlen(SAVE_EXTENSION) > (0) && !overflow) {
        if (strlen(g.SAVEF) + strlen(SAVE_EXTENSION) < (SAVESIZE - 1)) {
            Strcat(g.SAVEF, SAVE_EXTENSION);
#ifdef MSDOS
        sfindicator = "";
#endif
        } else
            overflow = 3;
    }
#endif
    if (indicator_spot == 2 && sfindicator && !overflow) {
        if (strlen(g.SAVEF) + strlen(sfindicator) < (SAVESIZE - 1))
           Strcat(g.SAVEF, sfindicator);
        else
            overflow = 4;
    }
    if (postappend && !overflow) {
        if (strlen(g.SAVEF) + strlen(postappend) < (SAVESIZE - 1))
            Strcat(g.SAVEF, postappend);
        else
            overflow = 5;
    }
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED)
    if (overflow)
        impossible("set_savefile_name() couldn't complete without overflow %d",
                   overflow);
#endif
}

#ifdef INSURANCE
void
save_savefile_name(NHFILE *nhfp)
{
    if (nhfp->structlevel)
        (void) write(nhfp->fd, (genericptr_t) g.SAVEF, sizeof(g.SAVEF));
}
#endif

#ifndef MICRO
/* change pre-existing savefile name to indicate an error savefile */
void
set_error_savefile(void)
{
#ifdef VMS
    {
        char *semi_colon = rindex(g.SAVEF, ';');

        if (semi_colon)
            *semi_colon = '\0';
    }
    Strcat(g.SAVEF, ".e;1");
#else
#ifdef MAC
    Strcat(g.SAVEF, "-e");
#else
    Strcat(g.SAVEF, ".e");
#endif
#endif
}
#endif

/* create save file, overwriting one if it already exists */
NHFILE *
create_savefile(void)
{
    const char *fq_save;
    NHFILE *nhfp = (NHFILE *) 0;
    boolean do_historical = TRUE;

    fq_save = fqname(g.SAVEF, SAVEPREFIX, 0);
    nhfp = new_nhfile();
    if (nhfp) {
        nhfp->structlevel = TRUE;
        nhfp->fieldlevel = FALSE;
        nhfp->ftype = NHF_SAVEFILE;
        nhfp->mode = WRITING;
        if (g.program_state.in_self_recover || do_historical) {
            do_historical = TRUE;       /* force it */
            nhfp->structlevel = TRUE;
            nhfp->fieldlevel = FALSE;
            nhfp->addinfo = FALSE;
            nhfp->style.deflt = FALSE;
            nhfp->style.binary = TRUE;
            nhfp->fnidx = historical;
            nhfp->fd = -1;
            nhfp->fpdef = (FILE *) 0;
        }
        if (nhfp->structlevel) {
#if defined(MICRO) || defined(WIN32)
            nhfp->fd = open(fq_save, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC,
                            FCMASK);
#else
#ifdef MAC
            nhfp->fd = maccreat(fq_save, SAVE_TYPE);
#else
            nhfp->fd = creat(fq_save, FCMASK);
#endif
#endif /* MICRO || WIN32 */
        }
    }
#if defined(VMS) && !defined(SECURE)
    /*
       Make sure the save file is owned by the current process.  That's
       the default for non-privileged users, but for priv'd users the
       file will be owned by the directory's owner instead of the user.
    */
#undef getuid
    (void) chown(fq_save, getuid(), getgid());
#define getuid() vms_getuid()
#endif /* VMS && !SECURE */

    nhfp = viable_nhfile(nhfp);
    return nhfp;
}

/* open savefile for reading */
NHFILE *
open_savefile(void)
{
    const char *fq_save;
    NHFILE *nhfp = (NHFILE *) 0;
    boolean do_historical = TRUE;

    fq_save = fqname(g.SAVEF, SAVEPREFIX, 0);
    nhfp = new_nhfile();
    if (nhfp) {
        nhfp->structlevel = TRUE;
        nhfp->fieldlevel = FALSE;
        nhfp->ftype = NHF_SAVEFILE;
        nhfp->mode = READING;
        if (g.program_state.in_self_recover || do_historical) {
            do_historical = TRUE;       /* force it */
            nhfp->structlevel = TRUE;
            nhfp->fieldlevel = FALSE;
            nhfp->addinfo = FALSE;
            nhfp->style.deflt = FALSE;
            nhfp->style.binary = TRUE;
            nhfp->fnidx = historical;
            nhfp->fd = -1;
            nhfp->fpdef = (FILE *) 0;
        }
        if (nhfp->structlevel) {
#ifdef MAC
            nhfp->fd = macopen(fq_save, O_RDONLY | O_BINARY, SAVE_TYPE);
#else
            nhfp->fd = open(fq_save, O_RDONLY | O_BINARY, 0);
#endif
        }
    }
    nhfp = viable_nhfile(nhfp);
    return nhfp;
}

/* delete savefile */
int
delete_savefile(void)
{
    (void) unlink(fqname(g.SAVEF, SAVEPREFIX, 0));
    return 0; /* for restore_saved_game() (ex-xxxmain.c) test */
}

/* try to open up a save file and prepare to restore it */
NHFILE *
restore_saved_game(void)
{
    const char *fq_save;
    NHFILE *nhfp = (NHFILE *) 0;

    set_savefile_name(TRUE);
    fq_save = fqname(g.SAVEF, SAVEPREFIX, 0);

    nh_uncompress(fq_save);
    if ((nhfp = open_savefile()) != 0) {
        if (validate(nhfp, fq_save) != 0) {
            close_nhfile(nhfp);
            nhfp = (NHFILE *)0;
            (void) delete_savefile();
        }
    }
    return nhfp;
}

#if defined(SELECTSAVED)
char *
plname_from_file(const char *filename)
{
    NHFILE *nhfp = (NHFILE *) 0;
    char *result = 0;

    Strcpy(g.SAVEF, filename);
#ifdef COMPRESS_EXTENSION
    {
        /* if COMPRESS_EXTENSION is present, strip it off */
        int sln = (int) strlen(g.SAVEF),
            xln = (int) strlen(COMPRESS_EXTENSION);

        if (sln > xln && !strcmp(&g.SAVEF[sln - xln], COMPRESS_EXTENSION))
            g.SAVEF[sln - xln] = '\0';
    }
#endif
    nh_uncompress(g.SAVEF);
    if ((nhfp = open_savefile()) != 0) {
        if (validate(nhfp, filename) == 0) {
            char tplname[PL_NSIZ];

            get_plname_from_file(nhfp, tplname);
            result = dupstr(tplname);
        }
        close_nhfile(nhfp);
    }
    nh_compress(g.SAVEF);

    return result;
#if 0
/* --------- obsolete - used to be ifndef STORE_PLNAME_IN_FILE ----*/
#if defined(UNIX) && defined(QT_GRAPHICS)
    /* Name not stored in save file, so we have to extract it from
       the filename, which loses information
       (eg. "/", "_", and "." characters are lost. */
    int k;
    int uid;
    char name[64]; /* more than PL_NSIZ */
#ifdef COMPRESS_EXTENSION
#define EXTSTR COMPRESS_EXTENSION
#else
#define EXTSTR ""
#endif

    if ( sscanf( filename, "%*[^/]/%d%63[^.]" EXTSTR, &uid, name ) == 2 ) {
#undef EXTSTR
        /* "_" most likely means " ", which certainly looks nicer */
        for (k=0; name[k]; k++)
            if ( name[k] == '_' )
                name[k] = ' ';
        return dupstr(name);
    } else
#endif /* UNIX && QT_GRAPHICS */
    {
        return 0;
    }
/* --------- end of obsolete code ----*/
#endif /* 0 - WAS STORE_PLNAME_IN_FILE*/
}
#endif /* defined(SELECTSAVED) */

char **
get_saved_games(void)
{
#if defined(SELECTSAVED)
#if defined(WIN32) || defined(UNIX)
    int n;
#endif
    int j = 0;
    char **result = 0;
#ifdef WIN32
    {
        char *foundfile;
        const char *fq_save;
        const char *fq_new_save;
        const char *fq_old_save;
        char **files = 0;
        int i;

        Strcpy(g.plname, "*");
        set_savefile_name(FALSE);
#if defined(ZLIB_COMP)
        Strcat(g.SAVEF, COMPRESS_EXTENSION);
#endif
        fq_save = fqname(g.SAVEF, SAVEPREFIX, 0);

        n = 0;
        foundfile = foundfile_buffer();
        if (findfirst((char *) fq_save)) {
            do {
                ++n;
            } while (findnext());
        }

        if (n > 0) {
            files = (char **) alloc((n + 1) * sizeof(char *)); /* at most */
            (void) memset((genericptr_t) files, 0, (n + 1) * sizeof(char *));
            if (findfirst((char *) fq_save)) {
                i = 0;
                do {
                    files[i++] = dupstr(foundfile);
                } while (findnext());
            }
        }

        if (n > 0) {
            result = (char **) alloc((n + 1) * sizeof(char *)); /* at most */
            (void) memset((genericptr_t) result, 0, (n + 1) * sizeof(char *));
            for(i = 0; i < n; i++) {
                char *r;
                r = plname_from_file(files[i]);

                if (r) {

                    /* rename file if it is not named as expected */
                    Strcpy(g.plname, r);
                    set_savefile_name(FALSE);
                    fq_new_save = fqname(g.SAVEF, SAVEPREFIX, 0);
                    fq_old_save = fqname(files[i], SAVEPREFIX, 1);

                    if (strcmp(fq_old_save, fq_new_save) != 0
                        && !file_exists(fq_new_save))
                        (void) rename(fq_old_save, fq_new_save);

                    result[j++] = r;
                }
            }
        }

        free_saved_games(files);

    }
#endif
#ifdef UNIX
    /* posixly correct version */
    int myuid = getuid();
    DIR *dir;

    if ((dir = opendir(fqname("save", SAVEPREFIX, 0)))) {
        for (n = 0; readdir(dir); n++)
            ;
        closedir(dir);
        if (n > 0) {
            int i;

            if (!(dir = opendir(fqname("save", SAVEPREFIX, 0))))
                return 0;
            result = (char **) alloc((n + 1) * sizeof(char *)); /* at most */
            (void) memset((genericptr_t) result, 0, (n + 1) * sizeof(char *));
            for (i = 0, j = 0; i < n; i++) {
                int uid;
                char name[64]; /* more than PL_NSIZ */
                struct dirent *entry = readdir(dir);

                if (!entry)
                    break;
                if (sscanf(entry->d_name, "%d%63s", &uid, name) == 2) {
                    if (uid == myuid) {
                        char filename[BUFSZ];
                        char *r;

                        Sprintf(filename, "save/%d%s", uid, name);
                        r = plname_from_file(filename);
                        if (r)
                            result[j++] = r;
                    }
                }
            }
            closedir(dir);
        }
    }
#endif
#ifdef VMS
    Strcpy(g.plname, "*");
    set_savefile_name(FALSE);
    j = vms_get_saved_games(g.SAVEF, &result);
#endif /* VMS */

    if (j > 0) {
        if (j > 1)
            qsort(result, j, sizeof (char *), strcmp_wrap);
        result[j] = 0;
        return result;
    } else if (result) { /* could happen if save files are obsolete */
        free_saved_games(result);
    }
#endif /* SELECTSAVED */
    return 0;
}

void
free_saved_games(char **saved)
{
    if (saved) {
        int i = 0;

        while (saved[i])
            free((genericptr_t) saved[i++]);
        free((genericptr_t) saved);
    }
}

/* ----------  END SAVE FILE HANDLING ----------- */

/* ----------  BEGIN FILE COMPRESSION HANDLING ----------- */

#ifdef COMPRESS /* external compression */

static void
redirect(const char *filename, const char *mode, FILE *stream, boolean uncomp)
{
    if (freopen(filename, mode, stream) == (FILE *) 0) {
        const char *details;

#if defined(NHSTDC) && !defined(NOTSTDC)
        if ((details = strerror(errno)) == 0)
#endif
            details = "";
        (void) fprintf(stderr,
                       "freopen of %s for %scompress failed; (%d) %s\n",
                       filename, uncomp ? "un" : "", errno, details);
        nh_terminate(EXIT_FAILURE);
    }
}

/*
 * using system() is simpler, but opens up security holes and causes
 * problems on at least Interactive UNIX 3.0.1 (SVR3.2), where any
 * setuid is renounced by /bin/sh, so the files cannot be accessed.
 *
 * cf. child() in unixunix.c.
 */
static void
docompress_file(const char *filename, boolean uncomp)
{
    char *cfn = 0;
    const char *xtra;
    FILE *cf;
    const char *args[10];
#ifdef COMPRESS_OPTIONS
    char opts[80];
#endif
    int i = 0;
    int f;
    unsigned ln;
#ifdef TTY_GRAPHICS
    boolean istty = WINDOWPORT(tty);
#endif

#ifdef COMPRESS_EXTENSION
    xtra = COMPRESS_EXTENSION;
#else
    xtra = "";
#endif
    ln = (unsigned) (strlen(filename) + strlen(xtra));
    cfn = (char *) alloc(ln + 1);
    Strcpy(cfn, filename);
    Strcat(cfn, xtra);

    /* when compressing, we know the file exists */
    if (uncomp) {
        if ((cf = fopen(cfn, RDBMODE)) == (FILE *) 0) {
            free((genericptr_t) cfn);
            return;
        }
        (void) fclose(cf);
    }

    args[0] = COMPRESS;
    if (uncomp)
        args[++i] = "-d"; /* uncompress */
#ifdef COMPRESS_OPTIONS
    {
        /* we can't guarantee there's only one additional option, sigh */
        char *opt;
        boolean inword = FALSE;

        Strcpy(opts, COMPRESS_OPTIONS);
        opt = opts;
        while (*opt) {
            if ((*opt == ' ') || (*opt == '\t')) {
                if (inword) {
                    *opt = '\0';
                    inword = FALSE;
                }
            } else if (!inword) {
                args[++i] = opt;
                inword = TRUE;
            }
            opt++;
        }
    }
#endif
    args[++i] = (char *) 0;

#ifdef TTY_GRAPHICS
    /* If we don't do this and we are right after a y/n question *and*
     * there is an error message from the compression, the 'y' or 'n' can
     * end up being displayed after the error message.
     */
    if (istty)
        mark_synch();
#endif
    f = fork();
    if (f == 0) { /* child */
#ifdef TTY_GRAPHICS
        /* any error messages from the compression must come out after
         * the first line, because the more() to let the user read
         * them will have to clear the first line.  This should be
         * invisible if there are no error messages.
         */
        if (istty)
            raw_print("");
#endif
        /* run compressor without privileges, in case other programs
         * have surprises along the line of gzip once taking filenames
         * in GZIP.
         */
        /* assume all compressors will compress stdin to stdout
         * without explicit filenames.  this is true of at least
         * compress and gzip, those mentioned in config.h.
         */
        if (uncomp) {
            redirect(cfn, RDBMODE, stdin, uncomp);
            redirect(filename, WRBMODE, stdout, uncomp);
        } else {
            redirect(filename, RDBMODE, stdin, uncomp);
            redirect(cfn, WRBMODE, stdout, uncomp);
        }
        (void) setgid(getgid());
        (void) setuid(getuid());
        (void) execv(args[0], (char *const *) args);
        perror((char *) 0);
        (void) fprintf(stderr, "Exec to %scompress %s failed.\n",
                       uncomp ? "un" : "", filename);
        free((genericptr_t) cfn);
        nh_terminate(EXIT_FAILURE);
    } else if (f == -1) {
        perror((char *) 0);
        pline("Fork to %scompress %s failed.", uncomp ? "un" : "", filename);
        free((genericptr_t) cfn);
        return;
    }
#ifndef NO_SIGNAL
    (void) signal(SIGINT, SIG_IGN);
    (void) signal(SIGQUIT, SIG_IGN);
    (void) wait((int *) &i);
    (void) signal(SIGINT, (SIG_RET_TYPE) done1);
    if (wizard)
        (void) signal(SIGQUIT, SIG_DFL);
#else
    /* I don't think we can really cope with external compression
     * without signals, so we'll declare that compress failed and
     * go on.  (We could do a better job by forcing off external
     * compression if there are no signals, but we want this for
     * testing with FailSafeC
     */
    i = 1;
#endif
    if (i == 0) {
        /* (un)compress succeeded: remove file left behind */
        if (uncomp)
            (void) unlink(cfn);
        else
            (void) unlink(filename);
    } else {
        /* (un)compress failed; remove the new, bad file */
        if (uncomp) {
            raw_printf("Unable to uncompress %s", filename);
            (void) unlink(filename);
        } else {
            /* no message needed for compress case; life will go on */
            (void) unlink(cfn);
        }
#ifdef TTY_GRAPHICS
        /* Give them a chance to read any error messages from the
         * compression--these would go to stdout or stderr and would get
         * overwritten only in tty mode.  It's still ugly, since the
         * messages are being written on top of the screen, but at least
         * the user can read them.
         */
        if (istty && iflags.window_inited) {
            clear_nhwindow(WIN_MESSAGE);
            more();
            /* No way to know if this is feasible */
            /* doredraw(); */
        }
#endif
    }

    free((genericptr_t) cfn);
}

#endif /* COMPRESS : external compression */

#if defined(COMPRESS) || defined(ZLIB_COMP)
#define UNUSED_if_not_COMPRESS /*empty*/
#else
#define UNUSED_if_not_COMPRESS UNUSED
#endif

/* compress file */
void
nh_compress(const char *filename UNUSED_if_not_COMPRESS)
{
#if !defined(COMPRESS) && !defined(ZLIB_COMP)
#ifdef PRAGMA_UNUSED
#pragma unused(filename)
#endif
#else
    docompress_file(filename, FALSE);
#endif
}

/* uncompress file if it exists */
void
nh_uncompress(const char *filename UNUSED_if_not_COMPRESS)
{
#if !defined(COMPRESS) && !defined(ZLIB_COMP)
#ifdef PRAGMA_UNUSED
#pragma unused(filename)
#endif
#else
    docompress_file(filename, TRUE);
#endif
}

#ifdef ZLIB_COMP /* RLC 09 Mar 1999: Support internal ZLIB */
static boolean
make_compressed_name(const char *filename, char *cfn)
{
#ifndef SHORT_FILENAMES
    /* Assume free-form filename with no 8.3 restrictions */
    strcpy(cfn, filename);
    strcat(cfn, COMPRESS_EXTENSION);
    return TRUE;
#else
#ifdef SAVE_EXTENSION
    char *bp = (char *) 0;

    strcpy(cfn, filename);
    if ((bp = strstri(cfn, SAVE_EXTENSION))) {
        strsubst(bp, SAVE_EXTENSION, ".saz");
        return TRUE;
    } else {
        /* find last occurrence of bon */
        bp = eos(cfn);
        while (bp-- > cfn) {
            if (strstri(bp, "bon")) {
                strsubst(bp, "bon", "boz");
                return TRUE;
            }
        }
    }
#endif /* SAVE_EXTENSION */
    return FALSE;
#endif /* SHORT_FILENAMES */
}

static void
docompress_file(const char *filename, boolean uncomp)
{
    gzFile compressedfile;
    FILE *uncompressedfile;
    char cfn[256];
    char buf[1024];
    unsigned len, len2;

    if (!make_compressed_name(filename, cfn))
        return;

    if (!uncomp) {
        /* Open the input and output files */
        /* Note that gzopen takes "wb" as its mode, even on systems where
           fopen takes "r" and "w" */

        uncompressedfile = fopen(filename, RDBMODE);
        if (!uncompressedfile) {
            pline("Error in zlib docompress_file %s", filename);
            return;
        }
        compressedfile = gzopen(cfn, "wb");
        if (compressedfile == NULL) {
            if (errno == 0) {
                pline("zlib failed to allocate memory");
            } else {
                panic("Error in docompress_file %d", errno);
            }
            fclose(uncompressedfile);
            return;
        }

        /* Copy from the uncompressed to the compressed file */

        while (1) {
            len = fread(buf, 1, sizeof(buf), uncompressedfile);
            if (ferror(uncompressedfile)) {
                pline("Failure reading uncompressed file");
                pline("Can't compress %s.", filename);
                fclose(uncompressedfile);
                gzclose(compressedfile);
                (void) unlink(cfn);
                return;
            }
            if (len == 0)
                break; /* End of file */

            len2 = gzwrite(compressedfile, buf, len);
            if (len2 == 0) {
                pline("Failure writing compressed file");
                pline("Can't compress %s.", filename);
                fclose(uncompressedfile);
                gzclose(compressedfile);
                (void) unlink(cfn);
                return;
            }
        }

        fclose(uncompressedfile);
        gzclose(compressedfile);

        /* Delete the file left behind */

        (void) unlink(filename);

    } else { /* uncomp */

        /* Open the input and output files */
        /* Note that gzopen takes "rb" as its mode, even on systems where
           fopen takes "r" and "w" */

        compressedfile = gzopen(cfn, "rb");
        if (compressedfile == NULL) {
            if (errno == 0) {
                pline("zlib failed to allocate memory");
            } else if (errno != ENOENT) {
                panic("Error in zlib docompress_file %s, %d", filename,
                      errno);
            }
            return;
        }
        uncompressedfile = fopen(filename, WRBMODE);
        if (!uncompressedfile) {
            pline("Error in zlib docompress file uncompress %s", filename);
            gzclose(compressedfile);
            return;
        }

        /* Copy from the compressed to the uncompressed file */

        while (1) {
            len = gzread(compressedfile, buf, sizeof(buf));
            if (len == (unsigned) -1) {
                pline("Failure reading compressed file");
                pline("Can't uncompress %s.", filename);
                fclose(uncompressedfile);
                gzclose(compressedfile);
                (void) unlink(filename);
                return;
            }
            if (len == 0)
                break; /* End of file */

            fwrite(buf, 1, len, uncompressedfile);
            if (ferror(uncompressedfile)) {
                pline("Failure writing uncompressed file");
                pline("Can't uncompress %s.", filename);
                fclose(uncompressedfile);
                gzclose(compressedfile);
                (void) unlink(filename);
                return;
            }
        }

        fclose(uncompressedfile);
        gzclose(compressedfile);

        /* Delete the file left behind */
        (void) unlink(cfn);
    }
}
#endif /* RLC 09 Mar 1999: End ZLIB patch */

/* ----------  END FILE COMPRESSION HANDLING ----------- */

/* ----------  BEGIN FILE LOCKING HANDLING ----------- */

#if defined(NO_FILE_LINKS) || defined(USE_FCNTL) /* implies UNIX */
static int lockfd = -1; /* for lock_file() to pass to unlock_file() */
#endif
#ifdef USE_FCNTL
struct flock sflock; /* for unlocking, same as above */
#endif

#define HUP if (!g.program_state.done_hup)

#ifndef USE_FCNTL
static char *
make_lockname(const char *filename, char *lockname)
{
#if defined(UNIX) || defined(VMS) || defined(AMIGA) || defined(WIN32) \
    || defined(MSDOS)
#ifdef NO_FILE_LINKS
    Strcpy(lockname, LOCKDIR);
    Strcat(lockname, "/");
    Strcat(lockname, filename);
#else
    Strcpy(lockname, filename);
#endif
#ifdef VMS
    {
        char *semi_colon = rindex(lockname, ';');
        if (semi_colon)
            *semi_colon = '\0';
    }
    Strcat(lockname, ".lock;1");
#else
    Strcat(lockname, "_lock");
#endif
    return lockname;
#else /* !(UNIX || VMS || AMIGA || WIN32 || MSDOS) */
#ifdef PRAGMA_UNUSED
#pragma unused(filename)
#endif
    lockname[0] = '\0';
    return (char *) 0;
#endif
}
#endif /* !USE_FCNTL */

/* lock a file */
boolean
lock_file(const char *filename, int whichprefix, int retryct)
{
#if defined(PRAGMA_UNUSED) && !(defined(UNIX) || defined(VMS)) \
    && !(defined(AMIGA) || defined(WIN32) || defined(MSDOS))
#pragma unused(retryct)
#endif
#ifndef USE_FCNTL
    char locknambuf[BUFSZ];
    const char *lockname;
#endif

    g.nesting++;
    if (g.nesting > 1) {
        impossible("TRIED TO NEST LOCKS");
        return TRUE;
    }

#ifndef USE_FCNTL
    lockname = make_lockname(filename, locknambuf);
#ifndef NO_FILE_LINKS /* LOCKDIR should be subsumed by LOCKPREFIX */
    lockname = fqname(lockname, LOCKPREFIX, 2);
#endif
#endif
    filename = fqname(filename, whichprefix, 0);
#ifdef USE_FCNTL
    lockfd = open(filename, O_RDWR);
    if (lockfd == -1) {
        HUP raw_printf("Cannot open file %s.  Is NetHack installed correctly?",
                       filename);
        g.nesting--;
        return FALSE;
    }
    sflock.l_type = F_WRLCK;
    sflock.l_whence = SEEK_SET;
    sflock.l_start = 0;
    sflock.l_len = 0;
#endif

#if defined(UNIX) || defined(VMS)
#ifdef USE_FCNTL
    while (fcntl(lockfd, F_SETLK, &sflock) == -1) {
#else
#ifdef NO_FILE_LINKS
    while ((lockfd = open(lockname, O_RDWR | O_CREAT | O_EXCL, 0666)) == -1) {
#else
    while (link(filename, lockname) == -1) {
#endif
#endif

#ifdef USE_FCNTL
        if (retryct--) {
            HUP raw_printf(
               "Waiting for release of fcntl lock on %s.  (%d retries left.)",
                           filename, retryct);
            sleep(1);
        } else {
            HUP raw_print("I give up.  Sorry.");
            HUP raw_printf("Some other process has an unnatural grip on %s.",
                           filename);
            g.nesting--;
            return FALSE;
        }
#else
        int errnosv = errno;

        switch (errnosv) { /* George Barbanis */
        case EEXIST:
            if (retryct--) {
                HUP raw_printf("Waiting for access to %s.  (%d retries left).",
                               filename, retryct);
#if defined(SYSV) || defined(ULTRIX) || defined(VMS)
                (void)
#endif
                    sleep(1);
            } else {
                HUP raw_print("I give up.  Sorry.");
                HUP raw_printf("Perhaps there is an old %s around?",
                               lockname);
                g.nesting--;
                return FALSE;
            }

            break;
        case ENOENT:
            HUP raw_printf("Can't find file %s to lock!", filename);
            g.nesting--;
            return FALSE;
        case EACCES:
            HUP raw_printf("No write permission to lock %s!", filename);
            g.nesting--;
            return FALSE;
#ifdef VMS /* c__translate(vmsfiles.c) */
        case EPERM:
            /* could be misleading, but usually right */
            HUP raw_printf("Can't lock %s due to directory protection.",
                           filename);
            g.nesting--;
            return FALSE;
#endif
        case EROFS:
            /* take a wild guess at the underlying cause */
            HUP perror(lockname);
            HUP raw_printf("Cannot lock %s.", filename);
            HUP raw_printf(
  "(Perhaps you are running NetHack from inside the distribution package?).");
            g.nesting--;
            return FALSE;
        default:
            HUP perror(lockname);
            HUP raw_printf("Cannot lock %s for unknown reason (%d).",
                           filename, errnosv);
            g.nesting--;
            return FALSE;
        }
#endif /* USE_FCNTL */
    }
#endif /* UNIX || VMS */

#if (defined(AMIGA) || defined(WIN32) || defined(MSDOS)) \
    && !defined(USE_FCNTL)
#ifdef AMIGA
#define OPENFAILURE(fd) (!fd)
    g.lockptr = 0;
#else
#define OPENFAILURE(fd) (fd < 0)
    g.lockptr = -1;
#endif
    while (--retryct && OPENFAILURE(g.lockptr)) {
#if defined(WIN32) && !defined(WIN_CE)
        g.lockptr = sopen(lockname, O_RDWR | O_CREAT, SH_DENYRW, S_IWRITE);
#else
        (void) DeleteFile(lockname); /* in case dead process was here first */
#ifdef AMIGA
        g.lockptr = Open(lockname, MODE_NEWFILE);
#else
        g.lockptr = open(lockname, O_RDWR | O_CREAT | O_EXCL, S_IWRITE);
#endif
#endif
        if (OPENFAILURE(g.lockptr)) {
            raw_printf("Waiting for access to %s.  (%d retries left).",
                       filename, retryct);
            Delay(50);
        }
    }
    if (!retryct) {
        raw_printf("I give up.  Sorry.");
        g.nesting--;
        return FALSE;
    }
#endif /* AMIGA || WIN32 || MSDOS */
    return TRUE;
}

#ifdef VMS /* for unlock_file, use the unlink() routine in vmsunix.c */
#ifdef unlink
#undef unlink
#endif
#define unlink(foo) vms_unlink(foo)
#endif

/* unlock file, which must be currently locked by lock_file */
void
unlock_file(const char *filename)
{
#ifndef USE_FCNTL
    char locknambuf[BUFSZ];
    const char *lockname;
#endif

    if (g.nesting == 1) {
#ifdef USE_FCNTL
        sflock.l_type = F_UNLCK;
        if (lockfd >= 0) {
            if (fcntl(lockfd, F_SETLK, &sflock) == -1)
                HUP raw_printf("Can't remove fcntl lock on %s.", filename);
            (void) close(lockfd), lockfd = -1;
        }
#else
        lockname = make_lockname(filename, locknambuf);
#ifndef NO_FILE_LINKS /* LOCKDIR should be subsumed by LOCKPREFIX */
        lockname = fqname(lockname, LOCKPREFIX, 2);
#endif

#if defined(UNIX) || defined(VMS)
        if (unlink(lockname) < 0)
            HUP raw_printf("Can't unlink %s.", lockname);
#ifdef NO_FILE_LINKS
        (void) nhclose(lockfd), lockfd = -1;
#endif

#endif /* UNIX || VMS */

#if defined(AMIGA) || defined(WIN32) || defined(MSDOS)
        if (g.lockptr)
            Close(g.lockptr);
        DeleteFile(lockname);
        g.lockptr = 0;
#endif /* AMIGA || WIN32 || MSDOS */
#endif /* USE_FCNTL */
    }

    g.nesting--;
}

/* ----------  END FILE LOCKING HANDLING ----------- */

/* ----------  BEGIN CONFIG FILE HANDLING ----------- */

const char *default_configfile =
#ifdef UNIX
    ".nethackrc";
#else
#if defined(MAC) || defined(__BEOS__)
    "NetHack Defaults";
#else
#if defined(MSDOS) || defined(WIN32)
    CONFIG_FILE;
#else
    "NetHack.cnf";
#endif
#endif
#endif

/* used for messaging */
char configfile[BUFSZ];

#ifdef MSDOS
/* conflict with speed-dial under windows
 * for XXX.cnf file so support of NetHack.cnf
 * is for backward compatibility only.
 * Preferred name (and first tried) is now defaults.nh but
 * the game will try the old name if there
 * is no defaults.nh.
 */
const char *backward_compat_configfile = "nethack.cnf";
#endif

/* #saveoptions - save config options into file */
int
do_write_config_file(void)
{
    FILE *fp;
    char tmp[BUFSZ];

    if (!configfile[0]) {
        pline("Strange, could not figure out config file name.");
        return ECMD_OK;
    }
    if (flags.suppress_alert < FEATURE_NOTICE_VER(3,7,0)) {
        pline("Warning: saveoptions is highly experimental!");
        wait_synch();
        pline("Some settings are not saved!");
        wait_synch();
        pline("All manual customization and comments are removed from the file!");
        wait_synch();
    }
#define overwrite_prompt "Overwrite config file %.*s?"
    Sprintf(tmp, overwrite_prompt, (int)(BUFSZ - sizeof overwrite_prompt - 2), configfile);
#undef overwrite_prompt
    if (!paranoid_query(TRUE, tmp))
        return ECMD_OK;

    fp = fopen(configfile, "w");
    if (fp) {
        size_t len, wrote;
        strbuf_t buf;

        strbuf_init(&buf);
        all_options_strbuf(&buf);
        len = strlen(buf.str);
        wrote = fwrite(buf.str, 1, len, fp);
        fclose(fp);
        strbuf_empty(&buf);
        if (wrote != len)
            pline("An error occurred, wrote only partial data (%lu/%lu).", wrote, len);
    }
    return ECMD_OK;
}

/* remember the name of the file we're accessing;
   if may be used in option reject messages */
static void
set_configfile_name(const char *fname)
{
    (void) strncpy(configfile, fname, sizeof configfile - 1);
    configfile[sizeof configfile - 1] = '\0';
}

static FILE *
fopen_config_file(const char *filename, int src)
{
    FILE *fp;
#if defined(UNIX) || defined(VMS)
    char tmp_config[BUFSZ];
    char *envp;
#endif

    if (src == set_in_sysconf) {
        /* SYSCF_FILE; if we can't open it, caller will bail */
        if (filename && *filename) {
            set_configfile_name(fqname(filename, SYSCONFPREFIX, 0));
            fp = fopen(configfile, "r");
        } else
            fp = (FILE *) 0;
        return  fp;
    }
    /* If src != set_in_sysconf, "filename" is an environment variable, so it
     * should hang around. If set, it is expected to be a full path name
     * (if relevant)
     */
    if (filename && *filename) {
        set_configfile_name(filename);
#ifdef UNIX
        if (access(configfile, 4) == -1) { /* 4 is R_OK on newer systems */
            /* nasty sneaky attempt to read file through
             * NetHack's setuid permissions -- this is the only
             * place a file name may be wholly under the player's
             * control (but SYSCF_FILE is not under the player's
             * control so it's OK).
             */
            raw_printf("Access to %s denied (%d).", configfile, errno);
            wait_synch();
            /* fall through to standard names */
        } else
#endif
        if ((fp = fopen(configfile, "r")) != (FILE *) 0) {
            return  fp;
#if defined(UNIX) || defined(VMS)
        } else {
            /* access() above probably caught most problems for UNIX */
            raw_printf("Couldn't open requested config file %s (%d).",
                       configfile, errno);
            wait_synch();
#endif
        }
    }
    /* fall through to standard names */

#if defined(MICRO) || defined(MAC) || defined(__BEOS__) || defined(WIN32)
    set_configfile_name(fqname(default_configfile, CONFIGPREFIX, 0));
    if ((fp = fopen(configfile, "r")) != (FILE *) 0) {
        return fp;
    } else if (strcmp(default_configfile, configfile)) {
        set_configfile_name(default_configfile);
        if ((fp = fopen(configfile, "r")) != (FILE *) 0)
            return fp;
    }
#ifdef MSDOS
    set_configfile_name(fqname(backward_compat_configfile, CONFIGPREFIX, 0));
    if ((fp = fopen(configfile, "r")) != (FILE *) 0) {
        return fp;
    } else if (strcmp(backward_compat_configfile, configfile)) {
        set_configfile_name(backward_compat_configfile);
        if ((fp = fopen(configfile, "r")) != (FILE *) 0)
            return fp;
    }
#endif
#else
/* constructed full path names don't need fqname() */
#ifdef VMS
    /* no punctuation, so might be a logical name */
    set_configfile_name("nethackini");
    if ((fp = fopen(configfile, "r")) != (FILE *) 0)
        return fp;
    set_configfile_name("sys$login:nethack.ini");
    if ((fp = fopen(configfile, "r")) != (FILE *) 0)
        return fp;

    envp = nh_getenv("HOME");
    if (!envp || !*envp)
        Strcpy(tmp_config, "NetHack.cnf");
    else
        Sprintf(tmp_config, "%s%s%s", envp,
                !index(":]>/", envp[strlen(envp) - 1]) ? "/" : "",
                "NetHack.cnf");
    set_configfile_name(tmp_config);
    if ((fp = fopen(configfile, "r")) != (FILE *) 0)
        return fp;
#else /* should be only UNIX left */
    envp = nh_getenv("HOME");
    if (!envp)
        Strcpy(tmp_config, ".nethackrc");
    else
        Sprintf(tmp_config, "%s/%s", envp, ".nethackrc");

    set_configfile_name(tmp_config);
    if ((fp = fopen(configfile, "r")) != (FILE *) 0)
        return fp;
#if defined(__APPLE__) /* UNIX+__APPLE__ => MacOSX */
    /* try an alternative */
    if (envp) {
        /* OSX-style configuration settings */
        Sprintf(tmp_config, "%s/%s", envp,
                "Library/Preferences/NetHack Defaults");
        set_configfile_name(tmp_config);
        if ((fp = fopen(configfile, "r")) != (FILE *) 0)
            return fp;
        /* may be easier for user to edit if filename has '.txt' suffix */
        Sprintf(tmp_config, "%s/%s", envp,
                "Library/Preferences/NetHack Defaults.txt");
        set_configfile_name(tmp_config);
        if ((fp = fopen(configfile, "r")) != (FILE *) 0)
            return fp;
    }
#endif /*__APPLE__*/
    if (errno != ENOENT) {
        const char *details;

        /* e.g., problems when setuid NetHack can't search home
           directory restricted to user */
#if defined(NHSTDC) && !defined(NOTSTDC)
        if ((details = strerror(errno)) == 0)
#endif
            details = "";
        raw_printf("Couldn't open default config file %s %s(%d).",
                   configfile, details, errno);
        wait_synch();
    }
#endif /* !VMS => Unix */
#endif /* !(MICRO || MAC || __BEOS__ || WIN32) */
    return (FILE *) 0;
}

/*
 * Retrieve a list of integers from buf into a uchar array.
 *
 * NOTE: zeros are inserted unless modlist is TRUE, in which case the list
 *  location is unchanged.  Callers must handle zeros if modlist is FALSE.
 */
static int
get_uchars(char *bufp,       /* current pointer */
           uchar *list,      /* return list */
           boolean modlist,  /* TRUE: list is being modified in place */
           int size,         /* return list size */
           const char *name) /* name of option for error message */
{
    unsigned int num = 0;
    int count = 0;
    boolean havenum = FALSE;

    while (1) {
        switch (*bufp) {
        case ' ':
        case '\0':
        case '\t':
        case '\n':
            if (havenum) {
                /* if modifying in place, don't insert zeros */
                if (num || !modlist)
                    list[count] = num;
                count++;
                num = 0;
                havenum = FALSE;
            }
            if (count == size || !*bufp)
                return count;
            bufp++;
            break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            havenum = TRUE;
            num = num * 10 + (*bufp - '0');
            bufp++;
            break;

        case '\\':
            goto gi_error;
            break;

        default:
 gi_error:
            raw_printf("Syntax error in %s", name);
            wait_synch();
            return count;
        }
    }
    /*NOTREACHED*/
}

#ifdef NOCWD_ASSUMPTIONS
static void
adjust_prefix(char *bufp, int prefixid)
{
    char *ptr;

    if (!bufp)
        return;
#ifdef WIN32
    if (fqn_prefix_locked[prefixid])
        return;
#endif
    /* Backward compatibility, ignore trailing ;n */
    if ((ptr = index(bufp, ';')) != 0)
        *ptr = '\0';
    if (strlen(bufp) > 0) {
        g.fqn_prefix[prefixid] = (char *) alloc(strlen(bufp) + 2);
        Strcpy(g.fqn_prefix[prefixid], bufp);
        append_slash(g.fqn_prefix[prefixid]);
    }
}
#endif

/* Choose at random one of the sep separated parts from str. Mangles str. */
static char *
choose_random_part(char *str, char sep)
{
    int nsep = 1;
    int csep;
    int len = 0;
    char *begin = str;

    if (!str)
        return (char *) 0;

    while (*str) {
        if (*str == sep)
            nsep++;
        str++;
    }
    csep = rn2(nsep);
    str = begin;
    while ((csep > 0) && *str) {
        str++;
        if (*str == sep)
            csep--;
    }
    if (*str) {
        if (*str == sep)
            str++;
        begin = str;
        while (*str && *str != sep) {
            str++;
            len++;
        }
        *str = '\0';
        if (len)
            return begin;
    }
    return (char *) 0;
}

static void
free_config_sections(void)
{
    if (g.config_section_chosen) {
        free(g.config_section_chosen);
        g.config_section_chosen = NULL;
    }
    if (g.config_section_current) {
        free(g.config_section_current);
        g.config_section_current = NULL;
    }
}

/* check for " [ anything-except-bracket-or-empty ] # arbitrary-comment"
   with spaces optional; returns pointer to "anything-except..." (with
   trailing " ] #..." stripped) if ok, otherwise Null */
static char *
is_config_section(char *str) /* trailing spaces will be stripped,
                                ']' too iff result is good */
{
    char *a, *c, *z;

    /* remove any spaces at start and end; won't significantly interfere
       with echoing the string in a config error message, if warranted */
    a = trimspaces(str);
    /* first character should be open square bracket; set pointer past it */
    if (*a++ != '[')
        return (char *) 0;
    /* last character should be close bracket, ignoring any comment */
    z = index(a, ']');
    if (!z)
        return (char *) 0;
    for (c = z + 1; *c && *c != '#'; ++c)
        continue;
    if (*c && *c != '#')
        return (char *) 0;
    /* we now know that result is good; there won't be a config error
       message so we can modify the input string */
    *z = '\0';
    /* 'a' points past '[' and the string ends where ']' was; remove any
       spaces between '[' and choice-start and between choice-end and ']' */
    return trimspaces(a);
}

static boolean
handle_config_section(char *buf)
{
    char *sect = is_config_section(buf);

    if (sect) {
        if (g.config_section_current)
            free(g.config_section_current), g.config_section_current = 0;
        /* is_config_section() removed brackets from 'sect' */
        if (!g.config_section_chosen) {
            config_error_add("Section \"[%s]\" without CHOOSE", sect);
            return TRUE;
        }
        if (*sect) { /* got a section name */
            g.config_section_current = dupstr(sect);
            debugpline1("set config section: '%s'", g.config_section_current);
        } else { /* empty section name => end of sections */
            free_config_sections();
            debugpline0("unset config section");
        }
        return TRUE;
    }

    if (g.config_section_current) {
        if (!g.config_section_chosen)
            return TRUE;
        if (strcmp(g.config_section_current, g.config_section_chosen))
            return TRUE;
    }
    return FALSE;
}

#define match_varname(INP, NAM, LEN) match_optname(INP, NAM, LEN, TRUE)

/* find the '=' or ':' */
static char *
find_optparam(const char *buf)
{
    char *bufp, *altp;

    bufp = index(buf, '=');
    altp = index(buf, ':');
    if (!bufp || (altp && altp < bufp))
        bufp = altp;

    return bufp;
}

boolean
parse_config_line(char *origbuf)
{
#if defined(MICRO) && !defined(NOCWD_ASSUMPTIONS)
    static boolean ramdisk_specified = FALSE;
#endif
#ifdef SYSCF
    int n, src = iflags.parse_config_file_src;
    boolean in_sysconf = (src == set_in_sysconf);
#endif
    char *bufp, buf[4 * BUFSZ];
    uchar translate[MAXPCHARS];
    int len;
    boolean retval = TRUE;

    while (*origbuf == ' ' || *origbuf == '\t') /* skip leading whitespace */
        ++origbuf;                   /* (caller probably already did this) */
    (void) strncpy(buf, origbuf, sizeof buf - 1);
    buf[sizeof buf - 1] = '\0'; /* strncpy not guaranteed to NUL terminate */
    /* convert any tab to space, condense consecutive spaces into one,
       remove leading and trailing spaces (exception: if there is nothing
       but spaces, one of them will be kept even though it leads/trails) */
    mungspaces(buf);

    /* find the '=' or ':' */
    bufp = find_optparam(buf);
    if (!bufp) {
        config_error_add("Not a config statement, missing '='");
        return FALSE;
    }
    /* skip past '=', then space between it and value, if any */
    ++bufp;
    if (*bufp == ' ')
        ++bufp;

    /* Go through possible variables */
    /* some of these (at least LEVELS and SAVE) should now set the
     * appropriate g.fqn_prefix[] rather than specialized variables
     */
    if (match_varname(buf, "OPTIONS", 4)) {
        /* hack: un-mungspaces to allow consecutive spaces in
           general options until we verify that this is unnecessary;
           '=' or ':' is guaranteed to be present */
        bufp = find_optparam(origbuf);
        ++bufp; /* skip '='; parseoptions() handles spaces */

        if (!parseoptions(bufp, TRUE, TRUE))
            retval = FALSE;
    } else if (match_varname(buf, "AUTOPICKUP_EXCEPTION", 5)) {
        add_autopickup_exception(bufp);
    } else if (match_varname(buf, "BINDINGS", 4)) {
        if (!parsebindings(bufp))
            retval = FALSE;
    } else if (match_varname(buf, "AUTOCOMPLETE", 5)) {
        parseautocomplete(bufp, TRUE);
    } else if (match_varname(buf, "MSGTYPE", 7)) {
        if (!msgtype_parse_add(bufp))
            retval = FALSE;
#ifdef NOCWD_ASSUMPTIONS
    } else if (match_varname(buf, "HACKDIR", 4)) {
        adjust_prefix(bufp, HACKPREFIX);
    } else if (match_varname(buf, "LEVELDIR", 4)
               || match_varname(buf, "LEVELS", 4)) {
        adjust_prefix(bufp, LEVELPREFIX);
    } else if (match_varname(buf, "SAVEDIR", 4)) {
        adjust_prefix(bufp, SAVEPREFIX);
    } else if (match_varname(buf, "BONESDIR", 5)) {
        adjust_prefix(bufp, BONESPREFIX);
    } else if (match_varname(buf, "DATADIR", 4)) {
        adjust_prefix(bufp, DATAPREFIX);
    } else if (match_varname(buf, "SCOREDIR", 4)) {
        adjust_prefix(bufp, SCOREPREFIX);
    } else if (match_varname(buf, "LOCKDIR", 4)) {
        adjust_prefix(bufp, LOCKPREFIX);
    } else if (match_varname(buf, "CONFIGDIR", 4)) {
        adjust_prefix(bufp, CONFIGPREFIX);
    } else if (match_varname(buf, "TROUBLEDIR", 4)) {
        adjust_prefix(bufp, TROUBLEPREFIX);
#else /*NOCWD_ASSUMPTIONS*/
#ifdef MICRO
    } else if (match_varname(buf, "HACKDIR", 4)) {
        (void) strncpy(g.hackdir, bufp, PATHLEN - 1);
    } else if (match_varname(buf, "LEVELS", 4)) {
        if (strlen(bufp) >= PATHLEN)
            bufp[PATHLEN - 1] = '\0';
        Strcpy(g.permbones, bufp);
        if (!ramdisk_specified || !*levels)
            Strcpy(levels, bufp);
        g.ramdisk = (strcmp(g.permbones, levels) != 0);
    } else if (match_varname(buf, "SAVE", 4)) {
        char *ptr;

        if ((ptr = index(bufp, ';')) != 0) {
            *ptr = '\0';
        }

        (void) strncpy(g.SAVEP, bufp, SAVESIZE - 1);
        append_slash(g.SAVEP);
#endif /* MICRO */
#endif /*NOCWD_ASSUMPTIONS*/

    } else if (match_varname(buf, "NAME", 4)) {
        (void) strncpy(g.plname, bufp, PL_NSIZ - 1);
    } else if (match_varname(buf, "ROLE", 4)
               || match_varname(buf, "CHARACTER", 4)) {
        if ((len = str2role(bufp)) >= 0)
            flags.initrole = len;
    } else if (match_varname(buf, "dogname", 3)) {
        (void) strncpy(g.dogname, bufp, PL_PSIZ - 1);
    } else if (match_varname(buf, "catname", 3)) {
        (void) strncpy(g.catname, bufp, PL_PSIZ - 1);

#ifdef SYSCF
    } else if (in_sysconf && match_varname(buf, "WIZARDS", 7)) {
        if (sysopt.wizards)
            free((genericptr_t) sysopt.wizards);
        sysopt.wizards = dupstr(bufp);
        if (strlen(sysopt.wizards) && strcmp(sysopt.wizards, "*")) {
            /* pre-format WIZARDS list now; it's displayed during a panic
               and since that panic might be due to running out of memory,
               we don't want to risk attempting to allocate any memory then */
            if (sysopt.fmtd_wizard_list)
                free((genericptr_t) sysopt.fmtd_wizard_list);
            sysopt.fmtd_wizard_list = build_english_list(sysopt.wizards);
        }
    } else if (in_sysconf && match_varname(buf, "SHELLERS", 8)) {
        if (sysopt.shellers)
            free((genericptr_t) sysopt.shellers);
        sysopt.shellers = dupstr(bufp);
    } else if (in_sysconf && match_varname(buf, "EXPLORERS", 7)) {
        if (sysopt.explorers)
            free((genericptr_t) sysopt.explorers);
        sysopt.explorers = dupstr(bufp);
    } else if (in_sysconf && match_varname(buf, "DEBUGFILES", 5)) {
        /* if showdebug() has already been called (perhaps we've added
           some debugpline() calls to option processing) and has found
           a value for getenv("DEBUGFILES"), don't override that */
        if (sysopt.env_dbgfl <= 0) {
            if (sysopt.debugfiles)
                free((genericptr_t) sysopt.debugfiles);
            sysopt.debugfiles = dupstr(bufp);
        }
    } else if (in_sysconf && match_varname(buf, "DUMPLOGFILE", 7)) {
#ifdef DUMPLOG
        if (sysopt.dumplogfile)
            free((genericptr_t) sysopt.dumplogfile);
        sysopt.dumplogfile = dupstr(bufp);
#endif
    } else if (in_sysconf && match_varname(buf, "GENERICUSERS", 12)) {
        if (sysopt.genericusers)
            free((genericptr_t) sysopt.genericusers);
        sysopt.genericusers = dupstr(bufp);
    } else if (in_sysconf && match_varname(buf, "BONES_POOLS", 10)) {
        /* max value of 10 guarantees (N % bones.pools) will be one digit
           so we don't lose control of the length of bones file names */
        n = atoi(bufp);
        sysopt.bones_pools = (n <= 0) ? 0 : min(n, 10);
        /* note: right now bones_pools==0 is the same as bones_pools==1,
           but we could change that and make bones_pools==0 become an
           indicator to suppress bones usage altogether */
    } else if (in_sysconf && match_varname(buf, "SUPPORT", 7)) {
        if (sysopt.support)
            free((genericptr_t) sysopt.support);
        sysopt.support = dupstr(bufp);
    } else if (in_sysconf && match_varname(buf, "RECOVER", 7)) {
        if (sysopt.recover)
            free((genericptr_t) sysopt.recover);
        sysopt.recover = dupstr(bufp);
    } else if (in_sysconf && match_varname(buf, "CHECK_SAVE_UID", 14)) {
        n = atoi(bufp);
        sysopt.check_save_uid = n;
    } else if (in_sysconf && match_varname(buf, "CHECK_PLNAME", 12)) {
        n = atoi(bufp);
        sysopt.check_plname = n;
    } else if (match_varname(buf, "SEDUCE", 6)) {
        n = !!atoi(bufp); /* XXX this could be tighter */
        /* allow anyone to disable it but can only enable it in sysconf
           or as a no-op for the user when sysconf hasn't disabled it */
        if (!in_sysconf && !sysopt.seduce && n != 0) {
            config_error_add("Illegal value in SEDUCE");
            n = 0;
        }
        sysopt.seduce = n;
        sysopt_seduce_set(sysopt.seduce);
    } else if (in_sysconf && match_varname(buf, "MAXPLAYERS", 10)) {
        n = atoi(bufp);
        /* XXX to get more than 25, need to rewrite all lock code */
        if (n < 0 || n > 25) {
            config_error_add("Illegal value in MAXPLAYERS (maximum is 25)");
            n = 5;
        }
        sysopt.maxplayers = n;
    } else if (in_sysconf && match_varname(buf, "PERSMAX", 7)) {
        n = atoi(bufp);
        if (n < 1) {
            config_error_add("Illegal value in PERSMAX (minimum is 1)");
            n = 0;
        }
        sysopt.persmax = n;
    } else if (in_sysconf && match_varname(buf, "PERS_IS_UID", 11)) {
        n = atoi(bufp);
        if (n != 0 && n != 1) {
            config_error_add("Illegal value in PERS_IS_UID (must be 0 or 1)");
            n = 0;
        }
        sysopt.pers_is_uid = n;
    } else if (in_sysconf && match_varname(buf, "ENTRYMAX", 8)) {
        n = atoi(bufp);
        if (n < 10) {
            config_error_add("Illegal value in ENTRYMAX (minimum is 10)");
            n = 10;
        }
        sysopt.entrymax = n;
    } else if (in_sysconf && match_varname(buf, "POINTSMIN", 9)) {
        n = atoi(bufp);
        if (n < 1) {
            config_error_add("Illegal value in POINTSMIN (minimum is 1)");
            n = 100;
        }
        sysopt.pointsmin = n;
    } else if (in_sysconf && match_varname(buf, "MAX_STATUENAME_RANK", 10)) {
        n = atoi(bufp);
        if (n < 1) {
            config_error_add(
                      "Illegal value in MAX_STATUENAME_RANK (minimum is 1)");
            n = 10;
        }
        sysopt.tt_oname_maxrank = n;
    } else if (in_sysconf && match_varname(buf, "LIVELOG", 7)) {
        /* using 0 for base accepts "dddd" as decimal provided that first 'd'
           isn't '0', "0xhhhh" as hexadecimal, and "0oooo" as octal; ignores
           any trailing junk, including '8' or '9' for leading '0' octal */
        long L = strtol(bufp, NULL, 0);

        if (L < 0L || L > 0xffffL) {
            config_error_add(
                 "Illegal value for LIVELOG (must be between 0 and 0xFFFF).");
            return 0;
        }
        sysopt.livelog = L;

    /* SYSCF PANICTRACE options */
    } else if (in_sysconf && match_varname(buf, "PANICTRACE_LIBC", 15)) {
        n = atoi(bufp);
#if defined(PANICTRACE) && defined(PANICTRACE_LIBC)
        if (n < 0 || n > 2) {
            config_error_add("Illegal value in PANICTRACE_LIBC (not 0,1,2)");
            n = 0;
        }
#endif
        sysopt.panictrace_libc = n;
    } else if (in_sysconf
               && match_varname(buf, "PANICTRACE_GDB", 14)) {
        n = atoi(bufp);
#if defined(PANICTRACE)
        if (n < 0 || n > 2) {
            config_error_add("Illegal value in PANICTRACE_GDB (not 0,1,2)");
            n = 0;
        }
#endif
        sysopt.panictrace_gdb = n;
    } else if (in_sysconf && match_varname(buf, "GDBPATH", 7)) {
#if defined(PANICTRACE) && !defined(VMS)
        if (!file_exists(bufp)) {
            config_error_add("File specified in GDBPATH does not exist");
            return FALSE;
        }
#endif
        if (sysopt.gdbpath)
            free((genericptr_t) sysopt.gdbpath);
        sysopt.gdbpath = dupstr(bufp);
    } else if (in_sysconf && match_varname(buf, "GREPPATH", 7)) {
#if defined(PANICTRACE) && !defined(VMS)
        if (!file_exists(bufp)) {
            config_error_add("File specified in GREPPATH does not exist");
            return FALSE;
        }
#endif
        if (sysopt.greppath)
            free((genericptr_t) sysopt.greppath);
        sysopt.greppath = dupstr(bufp);
    /* SYSCF SAVE and BONES format options */
    } else if (in_sysconf && match_varname(buf, "SAVEFORMAT", 10)) {
        parseformat(sysopt.saveformat, bufp);
    } else if (in_sysconf && match_varname(buf, "BONESFORMAT", 11)) {
        parseformat(sysopt.bonesformat, bufp);
    } else if (in_sysconf && match_varname(buf, "ACCESSIBILITY", 13)) {
        n = atoi(bufp);
        if (n < 0 || n > 1) {
            config_error_add("Illegal value in ACCESSIBILITY (not 0,1)");
            n = 0;
        }
        sysopt.accessibility = n;
    } else if (in_sysconf && match_varname(buf, "PORTABLE_DEVICE_PATHS", 8)) {
#ifdef WIN32
        n = atoi(bufp);
        if (n < 0 || n > 1) {
            config_error_add(
                         "Illegal value in PORTABLE_DEVICE_PATHS (not 0,1)");
            n = 0;
        }
        sysopt.portable_device_paths = n;
#else   /* Windows-only directive encountered by non-Windows config */
        config_error_add("PORTABLE_DEVICE_PATHS is not supported");
#endif

#endif /* SYSCF */

    } else if (match_varname(buf, "BOULDER", 3)) {
        (void) get_uchars(bufp, &g.ov_primary_syms[SYM_BOULDER + SYM_OFF_X],
                          TRUE, 1, "BOULDER");
    } else if (match_varname(buf, "MENUCOLOR", 9)) {
        if (!add_menu_coloring(bufp))
            retval = FALSE;
    } else if (match_varname(buf, "HILITE_STATUS", 6)) {
#ifdef STATUS_HILITES
        if (!parse_status_hl1(bufp, TRUE))
            retval = FALSE;
#endif
    } else if (match_varname(buf, "WARNINGS", 5)) {
        (void) get_uchars(bufp, translate, FALSE, WARNCOUNT, "WARNINGS");
        assign_warnings(translate);
    } else if (match_varname(buf, "ROGUESYMBOLS", 4)) {
        if (!parsesymbols(bufp, ROGUESET)) {
            config_error_add("Error in ROGUESYMBOLS definition '%s'", bufp);
            retval = FALSE;
        }
        switch_symbols(TRUE);
    } else if (match_varname(buf, "SYMBOLS", 4)) {
        if (!parsesymbols(bufp, PRIMARYSET)) {
            config_error_add("Error in SYMBOLS definition '%s'", bufp);
            retval = FALSE;
        }
        switch_symbols(TRUE);
    } else if (match_varname(buf, "WIZKIT", 6)) {
        (void) strncpy(g.wizkit, bufp, WIZKIT_MAX - 1);
#ifdef AMIGA
    } else if (match_varname(buf, "FONT", 4)) {
        char *t;

        if (t = strchr(buf + 5, ':')) {
            *t = 0;
            amii_set_text_font(buf + 5, atoi(t + 1));
            *t = ':';
        }
    } else if (match_varname(buf, "PATH", 4)) {
        (void) strncpy(PATH, bufp, PATHLEN - 1);
    } else if (match_varname(buf, "DEPTH", 5)) {
        extern int amii_numcolors;
        int val = atoi(bufp);

        amii_numcolors = 1L << min(DEPTH, val);
    } else if (match_varname(buf, "SCREENMODE", 10)) {
        extern long amii_scrnmode;

        if (!stricmp(bufp, "req"))
            amii_scrnmode = 0xffffffff; /* Requester */
        else if (sscanf(bufp, "%x", &amii_scrnmode) != 1)
            amii_scrnmode = 0;
    } else if (match_varname(buf, "MSGPENS", 7)) {
        extern int amii_msgAPen, amii_msgBPen;
        char *t = strtok(bufp, ",/");

        if (t) {
            sscanf(t, "%d", &amii_msgAPen);
            if (t = strtok((char *) 0, ",/"))
                sscanf(t, "%d", &amii_msgBPen);
        }
    } else if (match_varname(buf, "TEXTPENS", 8)) {
        extern int amii_textAPen, amii_textBPen;
        char *t = strtok(bufp, ",/");

        if (t) {
            sscanf(t, "%d", &amii_textAPen);
            if (t = strtok((char *) 0, ",/"))
                sscanf(t, "%d", &amii_textBPen);
        }
    } else if (match_varname(buf, "MENUPENS", 8)) {
        extern int amii_menuAPen, amii_menuBPen;
        char *t = strtok(bufp, ",/");

        if (t) {
            sscanf(t, "%d", &amii_menuAPen);
            if (t = strtok((char *) 0, ",/"))
                sscanf(t, "%d", &amii_menuBPen);
        }
    } else if (match_varname(buf, "STATUSPENS", 10)) {
        extern int amii_statAPen, amii_statBPen;
        char *t = strtok(bufp, ",/");

        if (t) {
            sscanf(t, "%d", &amii_statAPen);
            if (t = strtok((char *) 0, ",/"))
                sscanf(t, "%d", &amii_statBPen);
        }
    } else if (match_varname(buf, "OTHERPENS", 9)) {
        extern int amii_otherAPen, amii_otherBPen;
        char *t = strtok(bufp, ",/");

        if (t) {
            sscanf(t, "%d", &amii_otherAPen);
            if (t = strtok((char *) 0, ",/"))
                sscanf(t, "%d", &amii_otherBPen);
        }
    } else if (match_varname(buf, "PENS", 4)) {
        extern unsigned short amii_init_map[AMII_MAXCOLORS];
        int i;
        char *t;

        for (i = 0, t = strtok(bufp, ",/");
             i < AMII_MAXCOLORS && t != (char *) 0;
             t = strtok((char *) 0, ",/"), ++i) {
            sscanf(t, "%hx", &amii_init_map[i]);
        }
        amii_setpens(amii_numcolors = i);
    } else if (match_varname(buf, "FGPENS", 6)) {
        extern int foreg[AMII_MAXCOLORS];
        int i;
        char *t;

        for (i = 0, t = strtok(bufp, ",/");
             i < AMII_MAXCOLORS && t != (char *) 0;
             t = strtok((char *) 0, ",/"), ++i) {
            sscanf(t, "%d", &foreg[i]);
        }
    } else if (match_varname(buf, "BGPENS", 6)) {
        extern int backg[AMII_MAXCOLORS];
        int i;
        char *t;

        for (i = 0, t = strtok(bufp, ",/");
             i < AMII_MAXCOLORS && t != (char *) 0;
             t = strtok((char *) 0, ",/"), ++i) {
            sscanf(t, "%d", &backg[i]);
        }
#endif /*AMIGA*/
#ifdef USER_SOUNDS
    } else if (match_varname(buf, "SOUNDDIR", 8)) {
        if (sounddir)
            free((genericptr_t) sounddir);
        sounddir = dupstr(bufp);
    } else if (match_varname(buf, "SOUND", 5)) {
        add_sound_mapping(bufp);
#else /* !USER_SOUNDS */
    } else if (match_varname(buf, "SOUNDDIR", 8)
               || match_varname(buf, "SOUND", 5)) {
        if (!g.no_sound_notified++) {
            config_error_add("SOUND and SOUNDDIR are not available");
        }
        ; /* skip this and any further SOUND or SOUNDDIR lines
           * but leave 'retval' set to True */
#endif /* ?USER_SOUNDS */
    } else if (match_varname(buf, "QT_TILEWIDTH", 12)) {
#ifdef QT_GRAPHICS
        extern char *qt_tilewidth;

        if (qt_tilewidth == NULL)
            qt_tilewidth = dupstr(bufp);
#endif
    } else if (match_varname(buf, "QT_TILEHEIGHT", 13)) {
#ifdef QT_GRAPHICS
        extern char *qt_tileheight;

        if (qt_tileheight == NULL)
            qt_tileheight = dupstr(bufp);
#endif
    } else if (match_varname(buf, "QT_FONTSIZE", 11)) {
#ifdef QT_GRAPHICS
        extern char *qt_fontsize;

        if (qt_fontsize == NULL)
            qt_fontsize = dupstr(bufp);
#endif
    } else if (match_varname(buf, "QT_COMPACT", 10)) {
#ifdef QT_GRAPHICS
        extern int qt_compact_mode;

        qt_compact_mode = atoi(bufp);
#endif
    } else {
        config_error_add("Unknown config statement");
        return FALSE;
    }
    return retval;
}

#ifdef USER_SOUNDS
boolean
can_read_file(const char *filename)
{
    return (boolean) (access(filename, 4) == 0);
}
#endif /* USER_SOUNDS */

struct _config_error_errmsg {
    int line_num;
    char *errormsg;
    struct _config_error_errmsg *next;
};

struct _config_error_frame {
    int line_num;
    int num_errors;
    boolean origline_shown;
    boolean fromfile;
    boolean secure;
    char origline[4 * BUFSZ];
    char source[BUFSZ];
    struct _config_error_frame *next;
};

static struct _config_error_frame *config_error_data = 0;
static struct _config_error_errmsg *config_error_msg = 0;

void
config_error_init(boolean from_file, const char *sourcename, boolean secure)
{
    struct _config_error_frame *tmp = (struct _config_error_frame *)
                                                           alloc(sizeof *tmp);

    tmp->line_num = 0;
    tmp->num_errors = 0;
    tmp->origline_shown = FALSE;
    tmp->fromfile = from_file;
    tmp->secure = secure;
    tmp->origline[0] = '\0';
    if (sourcename && sourcename[0]) {
        (void) strncpy(tmp->source, sourcename, sizeof (tmp->source) - 1);
        tmp->source[sizeof (tmp->source) - 1] = '\0';
    } else
        tmp->source[0] = '\0';

    tmp->next = config_error_data;
    config_error_data = tmp;
    g.program_state.config_error_ready = TRUE;
}

static boolean
config_error_nextline(const char *line)
{
    struct _config_error_frame *ced = config_error_data;

    if (!ced)
        return FALSE;

    if (ced->num_errors && ced->secure)
        return FALSE;

    ced->line_num++;
    ced->origline_shown = FALSE;
    if (line && line[0]) {
        (void) strncpy(ced->origline, line, sizeof (ced->origline) - 1);
        ced->origline[sizeof (ced->origline) - 1] = '\0';
    } else
        ced->origline[0] = '\0';

    return TRUE;
}

int
l_get_config_errors(lua_State *L)
{
    struct _config_error_errmsg *dat = config_error_msg;
    struct _config_error_errmsg *tmp;
    int idx = 1;

    lua_newtable(L);

    while (dat) {
        lua_pushinteger(L, idx++);
        lua_newtable(L);
        nhl_add_table_entry_int(L, "line", dat->line_num);
        nhl_add_table_entry_str(L, "error", dat->errormsg);
        lua_settable(L, -3);
        tmp = dat->next;
        free(dat->errormsg);
        dat->errormsg = (char *) 0;
        free(dat);
        dat = tmp;
    }
    config_error_msg = (struct _config_error_errmsg *) 0;

    return 1;
}

/* varargs 'config_error_add()' moved to pline.c */
void
config_erradd(const char *buf)
{
    char lineno[QBUFSZ];
    const char *punct;

    if (!buf || !*buf)
        buf = "Unknown error";

    /* if buf[] doesn't end in a period, exclamation point, or question mark,
       we'll include a period (in the message, not appended to buf[]) */
    punct = eos((char *) buf) - 1; /* eos(buf)-1 is valid; cast away const */
    punct = index(".!?", *punct) ? "" : ".";

    if (!g.program_state.config_error_ready) {
        /* either very early, where pline() will use raw_print(), or
           player gave bad value when prompted by interactive 'O' command */
        pline("%s%s%s", !iflags.window_inited ? "config_error_add: " : "",
              buf, punct);
        wait_synch();
        return;
    }

    if (iflags.in_lua) {
        struct _config_error_errmsg *dat
                         = (struct _config_error_errmsg *) alloc(sizeof *dat);

        dat->next = config_error_msg;
        dat->line_num = config_error_data->line_num;
        dat->errormsg = dupstr(buf);
        config_error_msg = dat;
        return;
    }

    config_error_data->num_errors++;
    if (!config_error_data->origline_shown && !config_error_data->secure) {
        pline("\n%s", config_error_data->origline);
        config_error_data->origline_shown = TRUE;
    }
    if (config_error_data->line_num > 0 && !config_error_data->secure) {
        Sprintf(lineno, "Line %d: ", config_error_data->line_num);
    } else
        lineno[0] = '\0';

    pline("%s %s%s%s", config_error_data->secure ? "Error:" : " *",
          lineno, buf, punct);
}

int
config_error_done(void)
{
    int n;
    struct _config_error_frame *tmp = config_error_data;

    if (!config_error_data)
        return 0;
    n = config_error_data->num_errors;
#ifndef USER_SOUNDS
    if (g.no_sound_notified > 0) {
        /* no USER_SOUNDS; config_error_add() was called once for first
           SOUND or SOUNDDIR entry seen, then skipped for any others;
           include those skipped ones in the total error count */
        n += (g.no_sound_notified - 1);
        g.no_sound_notified = 0;
    }
#endif
    if (n) {
        boolean cmdline = !strcmp(config_error_data->source, "command line");

        pline("\n%d error%s %s %s.\n", n, plur(n), cmdline ? "on" : "in",
              *config_error_data->source ? config_error_data->source
                                         : configfile);
        wait_synch();
    }
    config_error_data = tmp->next;
    free(tmp);
    g.program_state.config_error_ready = (config_error_data != 0);
    return n;
}

boolean
read_config_file(const char *filename, int src)
{
    FILE *fp;
    boolean rv = TRUE;

    if (!(fp = fopen_config_file(filename, src)))
        return FALSE;

    /* begin detection of duplicate configfile options */
    reset_duplicate_opt_detection();
    free_config_sections();
    iflags.parse_config_file_src = src;

    rv = parse_conf_file(fp, parse_config_line);
    (void) fclose(fp);

    free_config_sections();
    /* turn off detection of duplicate configfile options */
    reset_duplicate_opt_detection();
    return rv;
}

struct _cnf_parser_state {
    char *inbuf;
    unsigned inbufsz;
    int rv;
    char *ep;
    char *buf;
    boolean skip, morelines;
    boolean cont;
    boolean pbreak;
};

/* Initialize config parser data */
static void
cnf_parser_init(struct _cnf_parser_state *parser)
{
    parser->rv = TRUE; /* assume successful parse */
    parser->ep = parser->buf = (char *) 0;
    parser->skip = FALSE;
    parser->morelines = FALSE;
    parser->inbufsz = 4 * BUFSZ;
    parser->inbuf = (char *) alloc(parser->inbufsz);
    parser->cont = FALSE;
    parser->pbreak = FALSE;
    memset(parser->inbuf, 0, parser->inbufsz);
}

/* caller has finished with 'parser' (except for 'rv' so leave that intact) */
static void
cnf_parser_done(struct _cnf_parser_state *parser)
{
    parser->ep = 0; /* points into parser->inbuf, so becoming stale */
    if (parser->inbuf)
        free(parser->inbuf), parser->inbuf = 0;
    if (parser->buf)
        free(parser->buf), parser->buf = 0;
}

/*
 * Parse config buffer, handling comments, empty lines, config sections,
 * CHOOSE, and line continuation, calling proc for every valid line.
 *
 * Continued lines are merged together with one space in between.
 */
static void
parse_conf_buf(struct _cnf_parser_state *p, boolean (*proc)(char *arg))
{
    p->cont = FALSE;
    p->pbreak = FALSE;
    p->ep = index(p->inbuf, '\n');
    if (p->skip) { /* in case previous line was too long */
        if (p->ep)
            p->skip = FALSE; /* found newline; next line is normal */
    } else {
        if (!p->ep) {  /* newline missing */
            if (strlen(p->inbuf) < (p->inbufsz - 2)) {
                /* likely the last line of file is just
                   missing a newline; process it anyway  */
                p->ep = eos(p->inbuf);
            } else {
                config_error_add("Line too long, skipping");
                p->skip = TRUE; /* discard next fgets */
            }
        } else {
            *p->ep = '\0'; /* remove newline */
        }
        if (p->ep) {
            char *tmpbuf = (char *) 0;
            int len;
            boolean ignoreline = FALSE;
            boolean oldline = FALSE;

            /* line continuation (trailing '\') */
            p->morelines = (--p->ep >= p->inbuf && *p->ep == '\\');
            if (p->morelines)
                *p->ep = '\0';

            /* trim off spaces at end of line */
            while (p->ep >= p->inbuf
                   && (*p->ep == ' ' || *p->ep == '\t' || *p->ep == '\r'))
                *p->ep-- = '\0';

            if (!config_error_nextline(p->inbuf)) {
                p->rv = FALSE;
                if (p->buf)
                    free(p->buf), p->buf = (char *) 0;
                p->pbreak = TRUE;
                return;
            }

            p->ep = p->inbuf;
            while (*p->ep == ' ' || *p->ep == '\t')
                ++p->ep;

            /* ignore empty lines and full-line comment lines */
            if (!*p->ep || *p->ep == '#')
                ignoreline = TRUE;

            if (p->buf)
                oldline = TRUE;

            /* merge now read line with previous ones, if necessary */
            if (!ignoreline) {
                len = (int) strlen(p->ep) + 1; /* +1: final '\0' */
                if (p->buf)
                    len += (int) strlen(p->buf) + 1; /* +1: space */
                tmpbuf = (char *) alloc(len);
                *tmpbuf = '\0';
                if (p->buf) {
                    Strcat(strcpy(tmpbuf, p->buf), " ");
                    free(p->buf), p->buf = 0;
                }
                p->buf = strcat(tmpbuf, p->ep);
                if (strlen(p->buf) >= p->inbufsz)
                    p->buf[p->inbufsz - 1] = '\0';
            }

            if (p->morelines || (ignoreline && !oldline))
                return;

            if (handle_config_section(p->buf)) {
                free(p->buf), p->buf = (char *) 0;
                return;
            }

            /* from here onwards, we'll handle buf only */

            if (match_varname(p->buf, "CHOOSE", 6)) {
                char *section;
                char *bufp = find_optparam(p->buf);

                if (!bufp) {
                    config_error_add("Format is CHOOSE=section1,section2,...");
                    p->rv = FALSE;
                    free(p->buf), p->buf = (char *) 0;
                    return;
                }
                bufp++;
                if (g.config_section_chosen)
                    free(g.config_section_chosen),
                        g.config_section_chosen = 0;
                section = choose_random_part(bufp, ',');
                if (section) {
                    g.config_section_chosen = dupstr(section);
                } else {
                    config_error_add("No config section to choose");
                    p->rv = FALSE;
                }
                free(p->buf), p->buf = (char *) 0;
                return;
            }

            if (!(*proc)(p->buf))
                p->rv = FALSE;

            free(p->buf), p->buf = (char *) 0;
        }
    }
}

boolean
parse_conf_str(const char *str, boolean (*proc)(char *arg))
{
    size_t len;
    struct _cnf_parser_state parser;

    cnf_parser_init(&parser);
    free_config_sections();
    config_error_init(FALSE, "parse_conf_str", FALSE);
    while (str && *str) {
        len = 0;
        while (*str && len < (parser.inbufsz-1)) {
            parser.inbuf[len] = *str;
            len++;
            str++;
            if (parser.inbuf[len-1] == '\n')
                break;
        }
        parser.inbuf[len] = '\0';
        parse_conf_buf(&parser, proc);
        if (parser.pbreak)
            break;
    }
    cnf_parser_done(&parser);

    free_config_sections();
    config_error_done();
    return parser.rv;
}

/* parse_conf_file
 *
 * Read from file fp, calling parse_conf_buf for each line.
 */
static boolean
parse_conf_file(FILE *fp, boolean (*proc)(char *arg))
{
    struct _cnf_parser_state parser;

    cnf_parser_init(&parser);
    free_config_sections();

    while (fgets(parser.inbuf, parser.inbufsz, fp)) {
        parse_conf_buf(&parser, proc);
        if (parser.pbreak)
            break;
    }
    cnf_parser_done(&parser);

    free_config_sections();
    return parser.rv;
}

static void
parseformat(int *arr, char *str)
{
    const char *legal[] = { "historical", "lendian", "ascii" };
    int i, kwi = 0, words = 0;
    char *p = str, *keywords[2];

    while (*p) {
        while (*p && isspace((uchar) *p)) {
            *p = '\0';
            p++;
        }
        if (*p) {
            words++;
            if (kwi < 2)
                keywords[kwi++] = p;
        }
        while (*p && !isspace((uchar) *p))
            p++;
    }
    if (!words) {
        impossible("missing format list");
        return;
    }
    while (--kwi >= 0)
        if (kwi < 2) {
            for (i = 0; i < SIZE(legal); ++i) {
               if (!strcmpi(keywords[kwi], legal[i]))
                   arr[kwi] = i + 1;
            }
        }
}

/* ----------  END CONFIG FILE HANDLING ----------- */

/* ----------  BEGIN WIZKIT FILE HANDLING ----------- */

static FILE *
fopen_wizkit_file(void)
{
    FILE *fp;
#if defined(VMS) || defined(UNIX)
    char tmp_wizkit[BUFSZ];
#endif
    char *envp;

    envp = nh_getenv("WIZKIT");
    if (envp && *envp)
        (void) strncpy(g.wizkit, envp, WIZKIT_MAX - 1);
    if (!g.wizkit[0])
        return (FILE *) 0;

#ifdef UNIX
    if (access(g.wizkit, 4) == -1) {
        /* 4 is R_OK on newer systems */
        /* nasty sneaky attempt to read file through
         * NetHack's setuid permissions -- this is a
         * place a file name may be wholly under the player's
         * control
         */
        raw_printf("Access to %s denied (%d).", g.wizkit, errno);
        wait_synch();
        /* fall through to standard names */
    } else
#endif
        if ((fp = fopen(g.wizkit, "r")) != (FILE *) 0) {
        return fp;
#if defined(UNIX) || defined(VMS)
    } else {
        /* access() above probably caught most problems for UNIX */
        raw_printf("Couldn't open requested wizkit file %s (%d).", g.wizkit,
                   errno);
        wait_synch();
#endif
    }

#if defined(MICRO) || defined(MAC) || defined(__BEOS__) || defined(WIN32)
    if ((fp = fopen(fqname(g.wizkit, CONFIGPREFIX, 0), "r")) != (FILE *) 0)
        return fp;
#else
#ifdef VMS
    envp = nh_getenv("HOME");
    if (envp)
        Sprintf(tmp_wizkit, "%s%s", envp, g.wizkit);
    else
        Sprintf(tmp_wizkit, "%s%s", "sys$login:", g.wizkit);
    if ((fp = fopen(tmp_wizkit, "r")) != (FILE *) 0)
        return fp;
#else /* should be only UNIX left */
    envp = nh_getenv("HOME");
    if (envp)
        Sprintf(tmp_wizkit, "%s/%s", envp, g.wizkit);
    else
        Strcpy(tmp_wizkit, g.wizkit);
    if ((fp = fopen(tmp_wizkit, "r")) != (FILE *) 0)
        return fp;
    else if (errno != ENOENT) {
        /* e.g., problems when setuid NetHack can't search home
         * directory restricted to user */
        raw_printf("Couldn't open default g.wizkit file %s (%d).", tmp_wizkit,
                   errno);
        wait_synch();
    }
#endif
#endif
    return (FILE *) 0;
}

/* add to hero's inventory if there's room, otherwise put item on floor */
static void
wizkit_addinv(struct obj *obj)
{
    if (!obj || obj == &cg.zeroobj)
        return;

    /* subset of starting inventory pre-ID */
    obj->dknown = 1;
    if (Role_if(PM_CLERIC))
        obj->bknown = 1; /* ok to bypass set_bknown() */
    /* same criteria as lift_object()'s check for available inventory slot */
    if (obj->oclass != COIN_CLASS && inv_cnt(FALSE) >= 52
        && !merge_choice(g.invent, obj)) {
        /* inventory overflow; can't just place & stack object since
           hero isn't in position yet, so schedule for arrival later */
        add_to_migration(obj);
        obj->ox = 0; /* index of main dungeon */
        obj->oy = 1; /* starting level number */
        obj->owornmask =
            (long) (MIGR_WITH_HERO | MIGR_NOBREAK | MIGR_NOSCATTER);
    } else {
        (void) addinv(obj);
    }
}

boolean
proc_wizkit_line(char *buf)
{
    struct obj *otmp;

    if (strlen(buf) >= BUFSZ)
        buf[BUFSZ - 1] = '\0';
    otmp = readobjnam(buf, (struct obj *) 0);

    if (otmp) {
        if (otmp != &cg.zeroobj)
            wizkit_addinv(otmp);
    } else {
        /* .60 limits output line width to 79 chars */
        config_error_add("Bad wizkit item: \"%.60s\"", buf);
        return FALSE;
    }
    return TRUE;
}

void
read_wizkit(void)
{
    FILE *fp;

    if (!wizard || !(fp = fopen_wizkit_file()))
        return;

    g.program_state.wizkit_wishing = 1;
    config_error_init(TRUE, "WIZKIT", FALSE);

    parse_conf_file(fp, proc_wizkit_line);
    (void) fclose(fp);

    config_error_done();
    g.program_state.wizkit_wishing = 0;

    return;
}

/* ----------  END WIZKIT FILE HANDLING ----------- */

/* ----------  BEGIN SYMSET FILE HANDLING ----------- */

extern const char *known_handling[];     /* symbols.c */
extern const char *known_restrictions[]; /* symbols.c */

static
FILE *
fopen_sym_file(void)
{
    FILE *fp;

    fp = fopen_datafile(SYMBOLS, "r",
#ifdef WIN32
                            SYSCONFPREFIX
#else
                            HACKPREFIX
#endif
                       );

    return fp;
}

/*
 * Returns 1 if the chosen symset was found and loaded.
 *         0 if it wasn't found in the sym file or other problem.
 */
int
read_sym_file(int which_set)
{
    FILE *fp;

    g.symset[which_set].explicitly = FALSE;
    if (!(fp = fopen_sym_file()))
        return 0;

    g.symset[which_set].explicitly = TRUE;
    g.chosen_symset_start = g.chosen_symset_end = FALSE;
    g.symset_which_set = which_set;
    g.symset_count = 0;

    config_error_init(TRUE, "symbols", FALSE);

    parse_conf_file(fp, proc_symset_line);
    (void) fclose(fp);

    if (!g.chosen_symset_start && !g.chosen_symset_end) {
        /* name caller put in symset[which_set].name was not found;
           if it looks like "Default symbols", null it out and return
           success to use the default; otherwise, return failure */
        if (g.symset[which_set].name
            && (fuzzymatch(g.symset[which_set].name, "Default symbols",
                           " -_", TRUE)
                || !strcmpi(g.symset[which_set].name, "default")))
            clear_symsetentry(which_set, TRUE);
        config_error_done();

        /* If name was defined, it was invalid.  Then we're loading fallback */
        if (g.symset[which_set].name) {
            g.symset[which_set].explicitly = FALSE;
            return 0;
        }

        return 1;
    }
    if (!g.chosen_symset_end)
        config_error_add("Missing finish for symset \"%s\"",
                         g.symset[which_set].name ? g.symset[which_set].name
                                                : "unknown");
    config_error_done();
    return 1;
}

/* ----------  END SYMSET FILE HANDLING ----------- */

/* ----------  BEGIN SCOREBOARD CREATION ----------- */

#ifdef OS2_CODEVIEW
#define UNUSED_if_not_OS2_CODEVIEW /*empty*/
#else
#define UNUSED_if_not_OS2_CODEVIEW UNUSED
#endif

/* verify that we can write to scoreboard file; if not, try to create one */
/*ARGUSED*/
void
check_recordfile(const char *dir UNUSED_if_not_OS2_CODEVIEW)
{
#if defined(PRAGMA_UNUSED) && !defined(OS2_CODEVIEW)
#pragma unused(dir)
#endif
    const char *fq_record;
    int fd;

#if defined(UNIX) || defined(VMS)
    fq_record = fqname(RECORD, SCOREPREFIX, 0);
    fd = open(fq_record, O_RDWR, 0);
    if (fd >= 0) {
#ifdef VMS /* must be stream-lf to use UPDATE_RECORD_IN_PLACE */
        if (!file_is_stmlf(fd)) {
            raw_printf(
                   "Warning: scoreboard file '%s' is not in stream_lf format",
                       fq_record);
            wait_synch();
        }
#endif
        (void) nhclose(fd); /* RECORD is accessible */
    } else if ((fd = open(fq_record, O_CREAT | O_RDWR, FCMASK)) >= 0) {
        (void) nhclose(fd); /* RECORD newly created */
#if defined(VMS) && !defined(SECURE)
        /* Re-protect RECORD with world:read+write+execute+delete access. */
        (void) chmod(fq_record, FCMASK | 007);
#endif /* VMS && !SECURE */
    } else {
        raw_printf("Warning: cannot write scoreboard file '%s'", fq_record);
        wait_synch();
    }
#endif /* !UNIX && !VMS */
#if defined(MICRO) || defined(WIN32)
    char tmp[PATHLEN];

#ifdef OS2_CODEVIEW /* explicit path on opening for OS/2 */
    /* how does this work when there isn't an explicit path or fopen
     * for later access to the file via fopen_datafile? ? */
    (void) strncpy(tmp, dir, PATHLEN - 1);
    tmp[PATHLEN - 1] = '\0';
    if ((strlen(tmp) + 1 + strlen(RECORD)) < (PATHLEN - 1)) {
        append_slash(tmp);
        Strcat(tmp, RECORD);
    }
    fq_record = tmp;
#else
    Strcpy(tmp, RECORD);
    fq_record = fqname(RECORD, SCOREPREFIX, 0);
#endif
#ifdef WIN32
    /* If dir is NULL it indicates create but
       only if it doesn't already exist */
    if (!dir) {
        char buf[BUFSZ];

        buf[0] = '\0';
        fd = open(fq_record, O_RDWR);
        if (!(fd == -1 && errno == ENOENT)) {
            if (fd >= 0) {
                (void) nhclose(fd);
            } else {
                /* explanation for failure other than missing file */
                Sprintf(buf, "error   \"%s\", (errno %d).",
                        fq_record, errno);
                paniclog("scorefile", buf);
            }
            return;
        }
        Sprintf(buf, "missing \"%s\", creating new scorefile.",
                fq_record);
        paniclog("scorefile", buf);
    }
#endif

    if ((fd = open(fq_record, O_RDWR)) < 0) {
        /* try to create empty 'record' */
#if defined(AZTEC_C) || defined(_DCC) \
    || (defined(__GNUC__) && defined(__AMIGA__))
        /* Aztec doesn't use the third argument */
        /* DICE doesn't like it */
        fd = open(fq_record, O_CREAT | O_RDWR);
#else
        fd = open(fq_record, O_CREAT | O_RDWR, S_IREAD | S_IWRITE);
#endif
        if (fd <= 0) {
            raw_printf("Warning: cannot write record '%s'", tmp);
            wait_synch();
        } else {
            (void) nhclose(fd);
        }
    } else {
        /* open succeeded => 'record' exists */
        (void) nhclose(fd);
    }
#else /* MICRO || WIN32*/

#ifdef MAC
    /* Create the "record" file, if necessary */
    fq_record = fqname(RECORD, SCOREPREFIX, 0);
    fd = macopen(fq_record, O_RDWR | O_CREAT, TEXT_TYPE);
    if (fd != -1)
        macclose(fd);
#endif /* MAC */

#endif /* MICRO || WIN32*/
}

/* ----------  END SCOREBOARD CREATION ----------- */

/* ----------  BEGIN PANIC/IMPOSSIBLE/TESTING LOG ----------- */

/*ARGSUSED*/
void
paniclog(
    const char *type,   /* panic, impossible, trickery, [lua] */
    const char *reason) /* explanation */
{
#ifdef PANICLOG
    FILE *lfile;

    if (!g.program_state.in_paniclog) {
        g.program_state.in_paniclog = 1;
        lfile = fopen_datafile(PANICLOG, "a", TROUBLEPREFIX);
        if (lfile) {
#ifdef PANICLOG_FMT2
            (void) fprintf(lfile, "%ld %s: %s %s\n",
                           ubirthday, (g.plname[0] ? g.plname : "(none)"),
                           type, reason);
#else
            char buf[BUFSZ];
            time_t now = getnow();
            int uid = getuid();
            char playmode = wizard ? 'D' : discover ? 'X' : '-';

            (void) fprintf(lfile, "%s %08ld %06ld %d %c: %s %s\n",
                           version_string(buf, sizeof buf),
                           yyyymmdd(now), hhmmss(now),
                           uid, playmode, type, reason);
#endif /* !PANICLOG_FMT2 */
            (void) fclose(lfile);
        }
        g.program_state.in_paniclog = 0;
    }
#endif /* PANICLOG */
    return;
}

void
testinglog(const char *filenm,   /* ad hoc file name */
           const char *type,
           const char *reason)   /* explanation */
{
    FILE *lfile;
    char fnbuf[BUFSZ];

    if (!filenm)
        return;
    Strcpy(fnbuf, filenm);
    if (index(fnbuf, '.') == 0)
        Strcat(fnbuf, ".log");
    lfile = fopen_datafile(fnbuf, "a", TROUBLEPREFIX);
    if (lfile) {
        (void) fprintf(lfile, "%s\n%s\n", type, reason);
        (void) fclose(lfile);
    }
    return;
}

/* ----------  END PANIC/IMPOSSIBLE/TESTING LOG ----------- */

#ifdef SELF_RECOVER

/* ----------  BEGIN INTERNAL RECOVER ----------- */
boolean
recover_savefile(void)
{
    NHFILE *gnhfp, *lnhfp, *snhfp;
    int lev, savelev, hpid, pltmpsiz, filecmc;
    xint16 levc;
    struct version_info version_data;
    int processed[256];
    char savename[SAVESIZE], errbuf[BUFSZ], indicator;
    struct savefile_info sfi;
    char tmpplbuf[PL_NSIZ];
    const char *savewrite_failure = (const char *) 0;

    for (lev = 0; lev < 256; lev++)
        processed[lev] = 0;

    /* level 0 file contains:
     *  pid of creating process (ignored here)
     *  level number for current level of save file
     *  name of save file nethack would have created
     *  savefile info
     *  player name
     *  and game state
     */

    gnhfp = open_levelfile(0, errbuf);
    if (!gnhfp) {
        raw_printf("%s\n", errbuf);
        return FALSE;
    }
    if (read(gnhfp->fd, (genericptr_t) &hpid, sizeof hpid) != sizeof hpid) {
        raw_printf("\n%s\n%s\n",
            "Checkpoint data incompletely written or subsequently clobbered.",
                   "Recovery impossible.");
        close_nhfile(gnhfp);
        return FALSE;
    }
    if (read(gnhfp->fd, (genericptr_t) &savelev, sizeof(savelev))
        != sizeof(savelev)) {
        raw_printf(
         "\nCheckpointing was not in effect for %s -- recovery impossible.\n",
                   g.lock);
        close_nhfile(gnhfp);
        return FALSE;
    }
    if ((read(gnhfp->fd, (genericptr_t) savename, sizeof savename)
         != sizeof savename)
        || (read(gnhfp->fd, (genericptr_t) &indicator, sizeof indicator)
            != sizeof indicator)
        || (read(gnhfp->fd, (genericptr_t) &filecmc, sizeof filecmc)
            != sizeof filecmc)
        || (read(gnhfp->fd, (genericptr_t) &version_data, sizeof version_data)
            != sizeof version_data)
        || (read(gnhfp->fd, (genericptr_t) &sfi, sizeof sfi) != sizeof sfi)
        || (read(gnhfp->fd, (genericptr_t) &pltmpsiz, sizeof pltmpsiz)
            != sizeof pltmpsiz) || (pltmpsiz > PL_NSIZ)
        || (read(gnhfp->fd, (genericptr_t) &tmpplbuf, pltmpsiz) != pltmpsiz)) {
        raw_printf("\nError reading %s -- can't recover.\n", g.lock);
        close_nhfile(gnhfp);
        return FALSE;
    }

    /* save file should contain:
     *  format indicator and cmc
     *  version info
     *  savefile info
     *  player name
     *  current level (including pets)
     *  (non-level-based) game state
     *  other levels
     */

    /*
     * Set a flag for the savefile routines to know the
     * circumstances and act accordingly:
     *    g.program_state.in_self_recover
     */
    g.program_state.in_self_recover = TRUE;
    set_savefile_name(TRUE);
    snhfp = create_savefile();
    if (!snhfp) {
        raw_printf("\nCannot recover savefile %s.\n", g.SAVEF);
        close_nhfile(gnhfp);
        return FALSE;
    }

    lnhfp = open_levelfile(savelev, errbuf);
    if (!lnhfp) {
        raw_printf("\n%s\n", errbuf);
        close_nhfile(gnhfp);
        close_nhfile(snhfp);
        delete_savefile();
        return FALSE;
    }

    /*
     * Our savefile output format might _not_ be structlevel.
     * We have to check and use the correct output routine here.
     */
    /*store_formatindicator(snhfp); */
    store_version(snhfp);

    if (snhfp->structlevel) {
        if (write(snhfp->fd, (genericptr_t) &sfi, sizeof sfi) != sizeof sfi)
            savewrite_failure = "savefileinfo";
    }
    if (savewrite_failure)
        goto cleanup;

    if (snhfp->structlevel) {
        if (write(snhfp->fd, (genericptr_t) &pltmpsiz, sizeof pltmpsiz)
            != sizeof pltmpsiz)
            savewrite_failure = "player name size";
    }
    if (savewrite_failure)
        goto cleanup;

    if (snhfp->structlevel) {
        if (write(snhfp->fd, (genericptr_t) &tmpplbuf, pltmpsiz) != pltmpsiz)
            savewrite_failure = "player name";
    }
    if (savewrite_failure)
        goto cleanup;

    if (!copy_bytes(lnhfp->fd, snhfp->fd)) {
        close_nhfile(gnhfp);
        close_nhfile(snhfp);
        close_nhfile(lnhfp);
        delete_savefile();
        return FALSE;
    }
    close_nhfile(lnhfp);
    processed[savelev] = 1;

    if (!copy_bytes(gnhfp->fd, snhfp->fd)) {
        close_nhfile(gnhfp);
        close_nhfile(snhfp);
        delete_savefile();
        return FALSE;
    }
    close_nhfile(gnhfp);
    processed[0] = 1;

    for (lev = 1; lev < 256; lev++) {
        /* level numbers are kept in xint16s in save.c, so the
         * maximum level number (for the endlevel) must be < 256
         */
        if (lev != savelev) {
            lnhfp = open_levelfile(lev, (char *) 0);
            if (lnhfp) {
                /* any or all of these may not exist */
                levc = (xint16) lev;
                (void) write(snhfp->fd, (genericptr_t) &levc, sizeof(levc));
                if (!copy_bytes(lnhfp->fd, snhfp->fd)) {
                    close_nhfile(lnhfp);
                    close_nhfile(snhfp);
                    delete_savefile();
                    return FALSE;
                }
                close_nhfile(lnhfp);
                processed[lev] = 1;
            }
        }
    }
    close_nhfile(snhfp);
    /*
     * We have a successful savefile!
     * Only now do we erase the level files.
     */
    for (lev = 0; lev < 256; lev++) {
        if (processed[lev]) {
            const char *fq_lock;

            set_levelfile_name(g.lock, lev);
            fq_lock = fqname(g.lock, LEVELPREFIX, 3);
            (void) unlink(fq_lock);
        }
    }
 cleanup:
    if (savewrite_failure) {
        raw_printf("\nError writing %s; recovery failed (%s).\n",
                   g.SAVEF, savewrite_failure);
        close_nhfile(gnhfp);
        close_nhfile(snhfp);
        close_nhfile(lnhfp);
        g.program_state.in_self_recover = FALSE;
        delete_savefile();
        return FALSE;
    }
    /* we don't clear g.program_state.in_self_recover here, we
       leave it as a flag to reload the structlevel savefile
       in the caller. The caller should then clear it. */
    return TRUE;
}

boolean
copy_bytes(int ifd, int ofd)
{
    char buf[BUFSIZ];
    int nfrom, nto;

    do {
        nfrom = read(ifd, buf, BUFSIZ);
        nto = write(ofd, buf, nfrom);
        if (nto != nfrom)
            return FALSE;
    } while (nfrom == BUFSIZ);
    return TRUE;
}

/* ----------  END INTERNAL RECOVER ----------- */
#endif /*SELF_RECOVER*/

/* ----------  OTHER ----------- */

#ifdef SYSCF
#ifdef SYSCF_FILE
void
assure_syscf_file(void)
{
    int fd;

#ifdef WIN32
    /* We are checking that the sysconf exists ... lock the path */
    fqn_prefix_locked[SYSCONFPREFIX] = TRUE;
#endif
    /*
     * All we really care about is the end result - can we read the file?
     * So just check that directly.
     *
     * Not tested on most of the old platforms (which don't attempt
     * to implement SYSCF).
     * Some ports don't like open()'s optional third argument;
     * VMS overrides open() usage with a macro which requires it.
     */
#ifndef VMS
# if defined(NOCWD_ASSUMPTIONS) && defined(WIN32)
    fd = open(fqname(SYSCF_FILE, SYSCONFPREFIX, 0), O_RDONLY);
# else
    fd = open(SYSCF_FILE, O_RDONLY);
# endif
#else
    fd = open(SYSCF_FILE, O_RDONLY, 0);
#endif
    if (fd >= 0) {
        /* readable */
        close(fd);
        return;
    }
    raw_printf("Unable to open SYSCF_FILE.\n");
    exit(EXIT_FAILURE);
}

#endif /* SYSCF_FILE */
#endif /* SYSCF */

#ifdef DEBUG
/* used by debugpline() to decide whether to issue a message
 * from a particular source file; caller passes __FILE__ and we check
 * whether it is in the source file list supplied by SYSCF's DEBUGFILES
 *
 * pass FALSE to override wildcard matching; useful for files
 * like dungeon.c and questpgr.c, which generate a ridiculous amount of
 * output if DEBUG is defined and effectively block the use of a wildcard */
boolean
debugcore(const char *filename, boolean wildcards)
{
    const char *debugfiles, *p;

    if (!filename || !*filename)
        return FALSE; /* sanity precaution */

    if (sysopt.env_dbgfl == 0) {
        /* check once for DEBUGFILES in the environment;
           if found, it supersedes the sysconf value
           [note: getenv() rather than nh_getenv() since a long value
           is valid and doesn't pose any sort of overflow risk here] */
        if ((p = getenv("DEBUGFILES")) != 0) {
            if (sysopt.debugfiles)
                free((genericptr_t) sysopt.debugfiles);
            sysopt.debugfiles = dupstr(p);
            sysopt.env_dbgfl = 1;
        } else
            sysopt.env_dbgfl = -1;
    }

    debugfiles = sysopt.debugfiles;
    /* usual case: sysopt.debugfiles will be empty */
    if (!debugfiles || !*debugfiles)
        return FALSE;

/* strip filename's path if present */
#ifdef UNIX
    if ((p = rindex(filename, '/')) != 0)
        filename = p + 1;
#endif
#ifdef VMS
    filename = vms_basename(filename);
    /* vms_basename strips off 'type' suffix as well as path and version;
       we want to put suffix back (".c" assumed); since it always returns
       a pointer to a static buffer, we can safely modify its result */
    Strcat((char *) filename, ".c");
#endif

    /*
     * Wildcard match will only work if there's a single pattern (which
     * might be a single file name without any wildcarding) rather than
     * a space-separated list.
     * [to NOT do: We could step through the space-separated list and
     * attempt a wildcard match against each element, but that would be
     * overkill for the intended usage.]
     */
    if (wildcards && pmatch(debugfiles, filename))
        return TRUE;

    /* check whether filename is an element of the list */
    if ((p = strstr(debugfiles, filename)) != 0) {
        int l = (int) strlen(filename);

        if ((p == debugfiles || p[-1] == ' ' || p[-1] == '/')
            && (p[l] == ' ' || p[l] == '\0'))
            return TRUE;
    }
    return FALSE;
}

#endif /*DEBUG*/

#ifdef UNIX
#ifndef PATH_MAX
#include <limits.h>
#endif
#endif

void
reveal_paths(void)
{
    const char *fqn, *nodumpreason;
    char buf[BUFSZ];
#if defined(SYSCF) || !defined(UNIX) || defined(DLB)
    const char *filep;
#ifdef SYSCF
    const char *gamename = (g.hname && *g.hname) ? g.hname : "NetHack";
#endif
#endif
#if defined(PREFIXES_IN_USE)
    const char *cstrp;
#endif
#ifdef UNIX
    char *endp, *envp, cwdbuf[PATH_MAX];
#endif
#ifdef PREFIXES_IN_USE
    int i, maxlen = 0;

    raw_print("Variable playground locations:");
    for (i = 0; i < PREFIX_COUNT; i++)
        raw_printf("    [%-10s]=\"%s\"", fqn_prefix_names[i],
                   g.fqn_prefix[i] ? g.fqn_prefix[i] : "not set");
#endif

    /* sysconf file */

#ifdef SYSCF
#ifdef PREFIXES_IN_USE
    cstrp = fqn_prefix_names[SYSCONFPREFIX];
    maxlen = BUFSZ - sizeof " (in )";
    if (cstrp && (int) strlen(cstrp) < maxlen)
        Sprintf(buf, " (in %s)", cstrp);
#else
    buf[0] = '\0';
#endif
    raw_printf("%s system configuration file%s:", s_suffix(gamename), buf);
#ifdef SYSCF_FILE
    filep = SYSCF_FILE;
#else
    filep = "sysconf";
#endif
    fqn = fqname(filep, SYSCONFPREFIX, 0);
    if (fqn) {
        set_configfile_name(fqn);
        filep = configfile;
    }
    raw_printf("    \"%s\"", filep);
#else /* !SYSCF */
    raw_printf("No system configuration file.");
#endif /* ?SYSCF */

    /* symbols file */

    buf[0] = '\0';
#ifndef UNIX
#ifdef PREFIXES_IN_USE
#ifdef WIN32
    cstrp = fqn_prefix_names[SYSCONFPREFIX];
#else
    cstrp = fqn_prefix_names[HACKPREFIX];
#endif /* WIN32 */
    maxlen = BUFSZ - sizeof " (in )";
    if (cstrp && (int) strlen(cstrp) < maxlen)
        Sprintf(buf, " (in %s)", cstrp);
#endif /* PREFIXES_IN_USE */
    raw_printf("The loadable symbols file%s:", buf);
#endif /* UNIX */

#ifdef UNIX
    envp = getcwd(cwdbuf, PATH_MAX);
    if (envp) {
        raw_print("The loadable symbols file:");
        raw_printf("    \"%s/%s\"", envp, SYMBOLS);
    }
#else /* UNIX */
    filep = SYMBOLS;
#ifdef PREFIXES_IN_USE
#ifdef WIN32
    fqn = fqname(filep, SYSCONFPREFIX, 1);
#else
    fqn = fqname(filep, HACKPREFIX, 1);
#endif /* WIN32 */
    if (fqn)
        filep = fqn;
#endif /* PREFIXES_IN_USE */
    raw_printf("    \"%s\"", filep);
#endif /* UNIX */

    /* dlb vs non-dlb */

    buf[0] = '\0';
#ifdef PREFIXES_IN_USE
    cstrp = fqn_prefix_names[DATAPREFIX];
    maxlen = BUFSZ - sizeof " (in )";
    if (cstrp && (int) strlen(cstrp) < maxlen)
        Sprintf(buf, " (in %s)", cstrp);
#endif
#ifdef DLB
    raw_printf("Basic data files%s are collected inside:", buf);
    filep = DLBFILE;
#ifdef VERSION_IN_DLB_FILENAME
    Strcpy(buf, build_dlb_filename((const char *) 0));
#ifdef PREFIXES_IN_USE
    fqn = fqname(buf, DATAPREFIX, 1);
    if (fqn)
        filep = fqn;
#endif /* PREFIXES_IN_USE */
#endif
    raw_printf("    \"%s\"", filep);
#ifdef DLBFILE2
    filep = DLBFILE2;
    raw_printf("    \"%s\"", filep);
#endif
#else /* !DLB */
    raw_printf("Basic data files%s are in many separate files.", buf);
#endif /* ?DLB */

    /* dumplog */

#ifndef DUMPLOG
    nodumpreason = "not supported";
#else
    nodumpreason = "disabled";
#ifdef SYSCF
    fqn = sysopt.dumplogfile;
#else  /* !SYSCF */
#ifdef DUMPLOG_FILE
    fqn = DUMPLOG_FILE;
#else
    fqn = (char *) 0;
#endif
#endif /* ?SYSCF */
    if (fqn && *fqn) {
        raw_print("Your end-of-game disclosure file:");
        (void) dump_fmtstr(fqn, buf, FALSE);
        buf[sizeof buf - sizeof "    \"\""] = '\0';
        raw_printf("    \"%s\"", buf);
    } else
#endif /* ?DUMPLOG */
        raw_printf("No end-of-game disclosure file (%s).", nodumpreason);

#ifdef WIN32
    if (sysopt.portable_device_paths) {
        const char *pd = get_portable_device();

        /* an empty value for pd indicates that portable_device_paths
           got set TRUE in a sysconf file other than the one containing
           the executable; disregard it */
        if (strlen(pd) > 0) {
            raw_printf("portable_device_paths (set in sysconf):");
            raw_printf("    \"%s\"", pd);
        }
    }
#endif

    /* personal configuration file */

    buf[0] = '\0';
#ifdef PREFIXES_IN_USE
    cstrp = fqn_prefix_names[CONFIGPREFIX];
    maxlen = BUFSZ - sizeof " (in )";
    if (cstrp && (int) strlen(cstrp) < maxlen)
        Sprintf(buf, " (in %s)", cstrp);
#endif /* PREFIXES_IN_USE */
    raw_printf("Your personal configuration file%s:", buf);

#ifdef UNIX
    buf[0] = '\0';
    if ((envp = nh_getenv("HOME")) != 0) {
        copynchars(buf, envp, (int) sizeof buf - 1 - 1);
        Strcat(buf, "/");
    }
    endp = eos(buf);
    copynchars(endp, default_configfile,
               (int) (sizeof buf - 1 - strlen(buf)));
#if defined(__APPLE__) /* UNIX+__APPLE__ => MacOSX aka OSX aka macOS */
    if (envp) {
        if (access(buf, 4) == -1) { /* 4: R_OK, -1: failure */
            /* read access to default failed; might be protected excessively
               but more likely it doesn't exist; try first alternate:
               "$HOME/Library/Pref..."; 'endp' points past '/' */
            copynchars(endp, "Library/Preferences/NetHack Defaults",
                       (int) (sizeof buf - 1 - strlen(buf)));
            if (access(buf, 4) == -1) {
                /* first alternate failed, try second:
                   ".../NetHack Defaults.txt"; no 'endp', just append */
                copynchars(eos(buf), ".txt",
                           (int) (sizeof buf - 1 - strlen(buf)));
                if (access(buf, 4) == -1) {
                    /* second alternate failed too, so revert to the
                       original default ("$HOME/.nethackrc") for message */
                    copynchars(endp, default_configfile,
                               (int) (sizeof buf - 1 - strlen(buf)));
                }
            }
        }
    }
#endif /* __APPLE__ */
    raw_printf("    \"%s\"", buf);
#else /* !UNIX */
    fqn = (const char *) 0;
#ifdef PREFIXES_IN_USE
    fqn = fqname(default_configfile, CONFIGPREFIX, 2);
#endif
    raw_printf("    \"%s\"", fqn ? fqn : default_configfile);
#endif  /* ?UNIX */

    raw_print("");
}

/* ----------  BEGIN TRIBUTE ----------- */

/* 3.6 tribute code
 */

#define SECTIONSCOPE 1
#define TITLESCOPE 2
#define PASSAGESCOPE 3

#define MAXPASSAGES SIZE(g.context.novel.pasg) /* 20 */

static int choose_passage(int, unsigned);

/* choose a random passage that hasn't been chosen yet; once all have
   been chosen, reset the tracking to make all passages available again */
static int
choose_passage(int passagecnt, /* total of available passages */
               unsigned oid)   /* book.o_id, used to determine whether
                                  re-reading same book */
{
    int idx, res;

    if (passagecnt < 1)
        return 0;

    /* if a different book or we've used up all the passages already,
       reset in order to have all 'passagecnt' passages available */
    if (oid != g.context.novel.id || g.context.novel.count == 0) {
        int i, range = passagecnt, limit = MAXPASSAGES;

        g.context.novel.id = oid;
        if (range <= limit) {
            /* collect all of the N indices */
            g.context.novel.count = passagecnt;
            for (idx = 0; idx < MAXPASSAGES; idx++)
                g.context.novel.pasg[idx] = (xint16) ((idx < passagecnt)
                                                   ? idx + 1 : 0);
        } else {
            /* collect MAXPASSAGES of the N indices */
            g.context.novel.count = MAXPASSAGES;
            for (idx = i = 0; i < passagecnt; ++i, --range)
                if (range > 0 && rn2(range) < limit) {
                    g.context.novel.pasg[idx++] = (xint16) (i + 1);
                    --limit;
                }
        }
    }

    idx = rn2(g.context.novel.count);
    res = (int) g.context.novel.pasg[idx];
    /* move the last slot's passage index into the slot just used
       and reduce the number of passages available */
    g.context.novel.pasg[idx] = g.context.novel.pasg[--g.context.novel.count];
    return res;
}

/* Returns True if you were able to read something. */
boolean
read_tribute(const char *tribsection, const char *tribtitle,
             int tribpassage, char *nowin_buf, int bufsz,
             unsigned oid) /* book identifier */
{
    dlb *fp;
    char line[BUFSZ], lastline[BUFSZ];

    int scope = 0;
    int linect = 0, passagecnt = 0, targetpassage = 0;
    const char *badtranslation = "an incomprehensible foreign translation";
    boolean matchedsection = FALSE, matchedtitle = FALSE;
    winid tribwin = WIN_ERR;
    boolean grasped = FALSE;
    boolean foundpassage = FALSE;

    if (nowin_buf)
        *nowin_buf = '\0';

    /* check for mandatories */
    if (!tribsection || !tribtitle) {
        if (!nowin_buf)
            pline("It's %s of \"%s\"!", badtranslation, tribtitle);
        return grasped;
    }

    debugpline3("read_tribute %s, %s, %d.", tribsection, tribtitle,
                tribpassage);

    fp = dlb_fopen(TRIBUTEFILE, "r");
    if (!fp) {
        /* this is actually an error - cannot open tribute file! */
        if (!nowin_buf)
            pline("You feel too overwhelmed to continue!");
        return grasped;
    }

    /*
     * Syntax (not case-sensitive):
     *  %section books
     *
     * In the books section:
     *    %title booktitle (n)
     *          where booktitle=book title without quotes
     *          (n)= total number of passages present for this title
     *    %passage k
     *          where k=sequential passage number
     *
     * %e ends the passage/book/section
     *    If in a passage, it marks the end of that passage.
     *    If in a book, it marks the end of that book.
     *    If in a section, it marks the end of that section.
     *
     *  %section death
     */

    *line = *lastline = '\0';
    while (dlb_fgets(line, sizeof line, fp) != 0) {
        linect++;
        (void) strip_newline(line);
        switch (line[0]) {
        case '%':
            if (!strncmpi(&line[1], "section ", sizeof "section " - 1)) {
                char *st = &line[9]; /* 9 from "%section " */

                scope = SECTIONSCOPE;
                matchedsection = !strcmpi(st, tribsection) ? TRUE : FALSE;
            } else if (!strncmpi(&line[1], "title ", sizeof "title " - 1)) {
                char *st = &line[7]; /* 7 from "%title " */
                char *p1, *p2;

                if ((p1 = index(st, '(')) != 0) {
                    *p1++ = '\0';
                    (void) mungspaces(st);
                    if ((p2 = index(p1, ')')) != 0) {
                        *p2 = '\0';
                        passagecnt = atoi(p1);
                        scope = TITLESCOPE;
                        if (matchedsection && !strcmpi(st, tribtitle)) {
                            matchedtitle = TRUE;
                            targetpassage = !tribpassage
                                             ? choose_passage(passagecnt, oid)
                                             : (tribpassage <= passagecnt)
                                                ? tribpassage : 0;
                        } else {
                            matchedtitle = FALSE;
                        }
                    }
                }
            } else if (!strncmpi(&line[1], "passage ",
                                 sizeof "passage " - 1)) {
                int passagenum = 0;
                char *st = &line[9]; /* 9 from "%passage " */

                mungspaces(st);
                passagenum = atoi(st);
                if (passagenum > 0 && passagenum <= passagecnt) {
                    scope = PASSAGESCOPE;
                    if (matchedtitle && passagenum == targetpassage) {
                        foundpassage = TRUE;
                        if (!nowin_buf) {
                            tribwin = create_nhwindow(NHW_MENU);
                            if (tribwin == WIN_ERR)
                                goto cleanup;
                        }
                    }
                }
            } else if (!strncmpi(&line[1], "e ", sizeof "e " - 1)) {
                if (foundpassage)
                    goto cleanup;
                if (scope == TITLESCOPE)
                    matchedtitle = FALSE;
                if (scope == SECTIONSCOPE)
                    matchedsection = FALSE;
                if (scope)
                    --scope;
            } else {
                debugpline1("tribute file error: bad %% command, line %d.",
                            linect);
            }
            break;
        case '#':
            /* comment only, next! */
            break;
        default:
            if (foundpassage) {
                if (!nowin_buf) {
                    /* outputting multi-line passage to text window */
                    putstr(tribwin, 0, line);
                    if (*line)
                        Strcpy(lastline, line);
                } else {
                    /* fetching one-line passage into buffer */
                    copynchars(nowin_buf, line, bufsz - 1);
                    goto cleanup; /* don't wait for "%e passage" */
                }
            }
        }
    }

 cleanup:
    (void) dlb_fclose(fp);
    if (nowin_buf) {
        /* one-line buffer */
        grasped = *nowin_buf ? TRUE : FALSE;
    } else {
        if (tribwin != WIN_ERR) { /* implies 'foundpassage' */
            /* multi-line window, normal case;
               if lastline is empty, there were no non-empty lines between
               "%passage n" and "%e passage" so we leave 'grasped' False */
            if (*lastline) {
                char *p;

                display_nhwindow(tribwin, FALSE);
                /* put the final attribution line into message history,
                   analogous to the summary line from long quest messages */
                if (index(lastline, '['))
                    mungspaces(lastline); /* to remove leading spaces */
                else /* construct one if necessary */
                    Sprintf(lastline, "[%s, by Terry Pratchett]", tribtitle);
                if ((p = rindex(lastline, ']')) != 0)
                    Sprintf(p, "; passage #%d]", targetpassage);
                putmsghistory(lastline, FALSE);
                grasped = TRUE;
            }
            destroy_nhwindow(tribwin);
        }
        if (!grasped)
            /* multi-line window, problem */
            pline("It seems to be %s of \"%s\"!", badtranslation, tribtitle);
    }
    return grasped;
}

boolean
Death_quote(char *buf, int bufsz)
{
    unsigned death_oid = 1; /* chance of oid #1 being a novel is negligible */

    return read_tribute("Death", "Death Quotes", 0, buf, bufsz, death_oid);
}

/* ----------  END TRIBUTE ----------- */

#ifdef LIVELOG
#define LLOG_SEP "\t" /* livelog field separator, as a string literal */
#define LLOG_EOL "\n" /* end-of-line, for abstraction consistency */

/* Locks the live log file and writes 'buffer'
 * iff the ll_type matches sysopt.livelog mask.
 * lltype is included in LL entry for post-process filtering also.
 */
void
livelog_add(long ll_type, const char *str)
{
    FILE *livelogfile;
    time_t now;
    int gindx, aindx;

    if (!(ll_type & sysopt.livelog))
        return;

    if (lock_file(LIVELOGFILE, SCOREPREFIX, 10)) {
        if (!(livelogfile = fopen_datafile(LIVELOGFILE, "a", SCOREPREFIX))) {
            pline("Cannot open live log file!");
            unlock_file(LIVELOGFILE);
            return;
        }

        now = getnow();
        gindx = flags.female ? 1 : 0;
        /* note on alignment designation:
               aligns[] uses [0] lawful, [1] neutral, [2] chaotic;
               u.ualign.type uses -1 chaotic, 0 neutral, 1 lawful;
           so subtracting from 1 converts from either to the other */
        aindx = 1 - u.ualign.type;
        /* format relies on STD C's implicit concatenation of
           adjacent string literals */
        (void) fprintf(livelogfile,
                       "lltype=%ld"  LLOG_SEP  "name=%s"       LLOG_SEP
                       "role=%s"     LLOG_SEP  "race=%s"       LLOG_SEP
                       "gender=%s"   LLOG_SEP  "align=%s"      LLOG_SEP
                       "turns=%ld"   LLOG_SEP  "starttime=%ld" LLOG_SEP
                       "curtime=%ld" LLOG_SEP  "message=%s"    LLOG_EOL,
                       (ll_type & sysopt.livelog), g.plname,
                       g.urole.filecode, g.urace.filecode,
                       genders[gindx].filecode, aligns[aindx].filecode,
                       g.moves, timet_to_seconds(ubirthday),
                       timet_to_seconds(now), str);
        (void) fclose(livelogfile);
        unlock_file(LIVELOGFILE);
    }
}
#undef LLOG_SEP
#undef LLOG_EOL

#else

void
livelog_add(long ll_type UNUSED, const char *str UNUSED)
{
    /* nothing here */
}

#endif /* !LIVELOG */

/*files.c*/
