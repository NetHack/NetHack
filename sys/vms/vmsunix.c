/* NetHack 3.7	vmsunix.c	$NHDT-Date: 1605493693 2020/11/16 02:28:13 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.24 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

/* This file implements things from unixunix.c, plus related stuff */

#include "hack.h"

#ifdef VMSVSI
#include <lib$routines.h>
#include <smg$routines.h>
#include <starlet.h>
#define sys$imgsta SYS$IMGSTA
#include <unixio.h>
#endif

#include <descrip.h>
#include <dvidef.h>
#include <jpidef.h>
#include <ssdef.h>
#include <errno.h>
#include <signal.h>
#undef off_t
#ifdef GNUC
#include <sys/stat.h>
#else
#define umask hide_umask_dummy /* DEC C: avoid conflict with system.h */
#include <stat.h>
#undef umask
#endif
#include <ctype.h>

extern int debuggable; /* defined in vmsmisc.c */

#ifndef VMSVSI
extern void VDECL(lib$signal, (unsigned, ...));
extern unsigned long sys$setprv();
extern unsigned long lib$getdvi(), lib$getjpi(), lib$spawn(), lib$attach();
extern unsigned long smg$init_term_table_by_type(), smg$del_term_table();
#endif

#define vms_ok(sts) ((sts) & 1) /* odd => success */

/* this could be static; it's only used within this file;
   it won't be used at all if C_LIB$INTIALIZE gets commented out below,
   so make it global so that compiler won't complain that it's not used */
int vmsexeini(const void *, const void *, const unsigned char *);

static int veryold(int);
static char *verify_term(void);
#if defined(SHELL) || defined(SUSPEND)
static void hack_escape(boolean, const char *);
static void hack_resume(boolean);
#endif

static int
veryold(int fd)
{
    register int i;
    time_t date;
    struct stat buf;

    if (fstat(fd, &buf))
        return 0; /* cannot get status */
#ifndef INSURANCE
    if (buf.st_size != sizeof (int))
        return 0; /* not an xlock file */
#endif
    (void) time(&date);
    if (date - buf.st_mtime < 3L * 24L * 60L * 60L) { /* recent */
        int lockedpid; /* should be the same size as hackpid */
        unsigned long status, dummy, code = JPI$_PID;

        if (read(fd, (genericptr_t) &lockedpid, sizeof lockedpid)
            != sizeof(lockedpid)) /* strange ... */
            return 0;
        status = lib$getjpi(&code, &lockedpid, 0, &dummy);
        if (vms_ok(status) || status != SS$_NONEXPR)
            return 0;
    }
    (void) close(fd);

    /* cannot use maxledgerno() here, because we need to find a lock name
     * before starting everything (including the dungeon initialization
     * that sets astral_level, needed for maxledgerno()) up
     */
    for (i = 1; i <= MAXDUNGEON * MAXLEVEL + 1; i++) {
        /* try to remove all */
        set_levelfile_name(gl.lock, i);
        (void) delete (gl.lock);
    }
    set_levelfile_name(gl.lock, 0);
    if (delete (gl.lock))
        return 0; /* cannot remove it */
    return 1;     /* success! */
}

void
getlock(void)
{
    register int i = 0, fd;

    /* idea from rpick%ucqais@uccba.uc.edu
     * prevent automated rerolling of characters
     * test input (fd0) so that tee'ing output to get a screen dump still
     * works
     * also incidentally prevents development of any hack-o-matic programs
     */
    if (isatty(0) <= 0)
        error("You must play from a terminal.");

    /* we ignore QUIT and INT at this point */
    if (!lock_file(HLOCK, LOCKPREFIX, 10)) {
        wait_synch();
        error("Quitting.");
    }

    /* default value of gl.lock[] is "1lock" where '1' gets changed to
       'a','b',&c below; override the default and use <uid><charname>
       if we aren't restricting the number of simultaneous games */
    if (!gl.locknum)
        Sprintf(gl.lock, "_%u%s", (unsigned) getuid(), gp.plname);

    regularize(gl.lock);
    set_levelfile_name(gl.lock, 0);
    if (gl.locknum > 25)
        gl.locknum = 25;

    do {
        if (gl.locknum)
            gl.lock[0] = 'a' + i++;

        if ((fd = open(gl.lock, 0, 0)) == -1) {
            if (errno == ENOENT)
                goto gotlock; /* no such file */
            perror(gl.lock);
            unlock_file(HLOCK);
            error("Cannot open %s", gl.lock);
        }

        if (veryold(fd)) /* if true, this closes fd and unlinks lock */
            goto gotlock;
        (void) close(fd);
    } while (i < gl.locknum);

    unlock_file(HLOCK);
    error(gl.locknum ? "Too many hacks running now."
                  : "There is a game in progress under your name.");

 gotlock:
    fd = creat(gl.lock, FCMASK);
    unlock_file(HLOCK);
    if (fd == -1) {
        error("cannot creat lock file.");
    } else {
        if (write(fd, (char *) &gh.hackpid, sizeof(gh.hackpid))
            != sizeof(gh.hackpid)) {
            error("cannot write lock");
        }
        if (close(fd) == -1) {
            error("cannot close lock");
        }
    }
}

/* normalize file name */
void regularize(register char *s)
{
    register char *lp;

    for (lp = s; *lp; lp++) /* note: '-' becomes '_' */
        if (!(isalpha(*lp) || isdigit(*lp) || *lp == '$'))
            *lp = '_';
}

#undef getuid
int
vms_getuid(void)
{
    return ((getgid() << 16) | getuid());
}

#ifndef FAB$C_STMLF
#define FAB$C_STMLF 5
#endif
/* check whether the open file specified by `fd' is in stream-lf format */
boolean
file_is_stmlf(int fd)
{
    int rfm;
    struct stat buf;

    if (fstat(fd, &buf))
        return FALSE; /* cannot get status? */

#ifdef stat_alignment_fix /* gcc-vms alignment kludge */
    rfm = stat_alignment_fix(&buf)->st_fab_rfm;
#else
    rfm = buf.st_fab_rfm;
#endif
    return (boolean) (rfm == FAB$C_STMLF);
}

/*------*/
#ifndef LNM$_STRING
#include <lnmdef.h> /* logical name definitions */
#endif
#define ENVSIZ LNM$C_NAMLENGTH /*255*/

#define ENV_USR 0 /* user-mode */
#define ENV_SUP 1 /* supervisor-mode */
#define ENV_JOB 2 /* job-wide entry */

/* vms_define() - assign a value to a logical name */
int
vms_define(const char *name, const char *value, int flag)
{
    struct dsc {
        unsigned short len, mbz;
        const char *adr;
    }; /* descriptor */
    struct itm3 {
        short buflen, itmcode;
        const char *bufadr;
        short *retlen;
    };
    static struct itm3 itm_lst[] = { { 0, LNM$_STRING, 0, 0 }, { 0, 0 } };
    struct dsc nam_dsc, val_dsc, tbl_dsc;
    unsigned long result;
#ifndef VMSVSI
    unsigned long sys$crelnm(), lib$set_logical();
#endif

    /* set up string descriptors */
    nam_dsc.mbz = val_dsc.mbz = tbl_dsc.mbz = 0;
    nam_dsc.len = strlen(nam_dsc.adr = name);
    val_dsc.len = strlen(val_dsc.adr = value);
    tbl_dsc.len = strlen(tbl_dsc.adr = "LNM$PROCESS");

    switch (flag) {
    case ENV_JOB: /* job logical name */
        tbl_dsc.len = strlen(tbl_dsc.adr = "LNM$JOB");
    /*FALLTHRU*/
    case ENV_SUP: /* supervisor-mode process logical name */
        result = lib$set_logical(&nam_dsc, &val_dsc, &tbl_dsc);
        break;
    case ENV_USR: /* user-mode process logical name */
        itm_lst[0].buflen = val_dsc.len;
        itm_lst[0].bufadr = val_dsc.adr;
        result = sys$crelnm(0, &tbl_dsc, &nam_dsc, 0, itm_lst);
        break;
    default: /*[ bad input ]*/
        result = 0;
        break;
    }
    result &= 1;    /* odd => success (== 1), even => failure (== 0) */
    return !result; /* 0 == success, 1 == failure */
}

/* vms_putenv() - create or modify an environment value */
int
vms_putenv(const char *string)
{
    char name[ENVSIZ + 1], value[ENVSIZ + 1], *p; /* [255+1] */

    p = strchr(string, '=');
    if (p > string && p < string + sizeof name
        && strlen(p + 1) < sizeof value) {
        (void) strncpy(name, string, p - string), name[p - string] = '\0';
        (void) strcpy(value, p + 1);
        return vms_define(name, value, ENV_USR);
    } else
        return 1; /* failure */
}

/*
   Support for VT420 was added to VMS in version V5.4, but as of V5.5-2
   VAXCRTL still doesn't handle it and puts TERM=undefined into the
   environ[] array.  getenv("TERM") will return "undefined" instead of
   something sensible.  Even though that's finally fixed in V6.0, site
   defined terminals also return "undefined" so query SMG's TERMTABLE
   instead of just checking VMS's device-type value for VT400_Series.

   Called by verify_termcap() for convenience.
 */
static char *
verify_term(void)
{
    char *term = getenv("NETHACK_TERM");
    if (!term)
        term = getenv("HACK_TERM");
    if (!term)
        term = getenv("EMACS_TERM");
    if (!term)
        term = getenv("TERM");
    if (!term || !*term || !strcmpi(term, "undefined")
        || !strcmpi(term, "unknown")) {
        static char smgdevtyp[31 + 1]; /* size is somewhat arbitrary */
        static char dev_tty[] = "TT:";
        static $DESCRIPTOR(smgdsc, smgdevtyp);
        static $DESCRIPTOR(tt, dev_tty);
        unsigned short dvicode = DVI$_DEVTYPE;
        unsigned long devtype = 0L, termtab = 0L;

        (void) lib$getdvi(&dvicode, (unsigned short *) 0, &tt, &devtype,
                          (genericptr_t) 0, (unsigned short *) 0);

        if (devtype && vms_ok(smg$init_term_table_by_type(&devtype, &termtab,
                                                          &smgdsc))) {
            register char *p = &smgdevtyp[smgdsc.dsc$w_length];
            /* strip trailing blanks */
            while (p > smgdevtyp && *--p == ' ')
                *p = '\0';
            /* (void) smg$del_term_table(); */
            term = smgdevtyp;
        }
    }
    return term;
}

/*
   Figure out whether the termcap code will find a termcap file; if not,
   try to help it out.  This avoids modifying the GNU termcap sources and
   can simplify configuration for sites which don't already use termcap.
 */
#define GNU_DEFAULT_TERMCAP "emacs_library:[etc]termcap.dat"
#define NETHACK_DEF_TERMCAP "nethackdir:termcap"
#define HACK_DEF_TERMCAP "hackdir:termcap"

char *verify_termcap(void) /* called from startup(src/termcap.c) */
{
    struct stat dummy;
    const char *tc = getenv("TERMCAP");
    if (tc)
        return verify_term(); /* no termcap fixups needed */
    if (!tc && !stat(NETHACK_DEF_TERMCAP, &dummy))
        tc = NETHACK_DEF_TERMCAP;
    if (!tc && !stat(HACK_DEF_TERMCAP, &dummy))
        tc = HACK_DEF_TERMCAP;
    if (!tc && !stat(GNU_DEFAULT_TERMCAP, &dummy))
        tc = GNU_DEFAULT_TERMCAP;
    if (!tc && !stat("[]termcap", &dummy))
        tc = "[]termcap"; /* current dir */
    if (!tc && !stat("$TERMCAP", &dummy))
        tc = "$TERMCAP"; /* alt environ */
    if (tc) {
        /* putenv(strcat(strcpy(buffer,"TERMCAP="),tc)); */
        vms_define("TERMCAP", tc, ENV_USR);
    } else {
        /* perhaps someday we'll construct a termcap entry string */
    }
    return verify_term();
}
/*------*/

#ifdef SHELL
#ifndef CLI$M_NOWAIT
#define CLI$M_NOWAIT 1
#endif
#endif

#if defined(CHDIR) || defined(SHELL) || defined(SECURE)
static unsigned long oprv[2];

void
privoff(void)
{
    unsigned long pid = 0, prv[2] = { ~0, ~0 };
    unsigned short code = JPI$_PROCPRIV;

    (void) sys$setprv(0, prv, 0, oprv);
    (void) lib$getjpi(&code, &pid, (genericptr_t) 0, prv);
    (void) sys$setprv(1, prv, 0, (unsigned long *) 0);
}

void
privon(void)
{
    (void) sys$setprv(1, oprv, 0, (unsigned long *) 0);
}
#endif /* CHDIR || SHELL || SECURE */

#ifdef SYSCF
boolean
check_user_string(const char *userlist)
{
    char usrnambuf[BUFSZ];
    const char *sptr, *p, *q;
    int ln;

    if (!strcmp(userlist, "*"))
        return TRUE;

    /* FIXME: ought to use $getjpi or $getuai to retrieve user name here... */
    Strcpy(usrnambuf, nh_getenv("USER"));
    ln = (int) strlen(usrnambuf);
    if (!ln)
        return FALSE;

    while ((sptr = strstri(userlist, usrnambuf)) != 0) {
        /* check for full word: start of list or following a space or comma */
        if ((sptr == userlist || sptr[-1] == ' ' || sptr[-1] == ',')
            /* and also preceding a space or comma or at end of list */
            && (sptr[ln] == ' ' || sptr[ln] == ',' || sptr[ln] == '\0'))
            return TRUE;
        /* doesn't match full word, but maybe we got a false hit when
           looking for "jane" in the list "janedoe jane" so keep going */
        p = strchr(sptr + 1, ' ');
        q = strchr(sptr + 1, ',');
        if (!p || (q && q < p))
            p = q;
        if (!p)
            break;
        userlist = p + 1;
    }

    return FALSE;
}
#endif /* SYSCF */

#if defined(SHELL) || defined(SUSPEND)
static void
hack_escape(boolean screen_manip, const char *msg_str)
{
    if (screen_manip)
        suspend_nhwindows(msg_str);  /* clear screen, reset terminal, &c */
    (void) signal(SIGQUIT, SIG_IGN); /* ignore ^Y */
    (void) signal(SIGINT, SIG_DFL);  /* don't trap ^C (implct cnvrs to ^Y) */
}

static void
hack_resume(boolean screen_manip)
{
    (void) signal(SIGINT, (SIG_RET_TYPE) done1);
    if (wizard)
        (void) signal(SIGQUIT, SIG_DFL);
    if (screen_manip)
        resume_nhwindows(); /* setup terminal modes, redraw screen, &c */
}
#endif /* SHELL || SUSPEND */

#ifdef SHELL
unsigned long dosh_pid = 0, /* this should cover any interactive escape */
              mail_pid = 0; /* this only covers the last mail or phone;
                               (mail & phone commands aren't expected to
                               leave any process hanging around) */

int
dosh(void)
{
#ifdef SYSCF
    if (!sysopt.shellers || !sysopt.shellers[0]
        || !check_user_string(sysopt.shellers)) {
        /* FIXME: should no longer assume a particular command keystroke */
        Norep("Unavailable command '!'.");
        return 0;
    }
#endif
    return vms_doshell("", TRUE); /* call for interactive child process */
}

/* vms_doshell -- called by dosh() and readmail()
 *
 * If execstring is not a null string, then it will be executed in a spawned
 * subprocess, which will then return.  It is for handling mail or phone
 * interactive commands, which are only available if both MAIL and SHELL are
 * #defined, but we don't bother making the support code conditionalized on
 * MAIL here, just on SHELL being enabled.
 *
 * Normally, all output from this interaction will be 'piped' to the user's
 * screen (SYS$OUTPUT).  However, if 'screenoutput' is set to FALSE, output
 * will be piped into oblivion.  Used for silent phone call rejection.
 */
int
vms_doshell(const char *execstring, boolean screenoutput)
{
    unsigned long status, new_pid, spawnflags = 0;
    struct dsc$descriptor_s comstring, *command, *inoutfile = 0;
    static char dev_null[] = "_NLA0:";
    static $DESCRIPTOR(nulldevice, dev_null);

    /* Is this an interactive shell spawn, or do we have a command to do? */
    if (execstring && *execstring) {
        comstring.dsc$w_length = strlen(execstring);
        comstring.dsc$b_dtype = DSC$K_DTYPE_T;
        comstring.dsc$b_class = DSC$K_CLASS_S;
        comstring.dsc$a_pointer = (char *) execstring;
        command = &comstring;
    } else
        command = 0;

    /* use asynch subprocess and suppress output iff one-shot command */
    if (!screenoutput) {
        spawnflags = CLI$M_NOWAIT;
        inoutfile = &nulldevice;
    }

    hack_escape(screenoutput,
                command ? (const char *) 0
 : "  \"Escaping\" into a subprocess; LOGOUT to reconnect and resume play. ");

    if (command || !dosh_pid || !vms_ok(status = lib$attach(&dosh_pid))) {
#ifdef CHDIR
        (void) chdir(getenv("PATH"));
#endif
        privoff();
        new_pid = 0;
        status = lib$spawn(command, inoutfile, inoutfile, &spawnflags,
                           (struct dsc$descriptor_s *) 0, &new_pid);
        if (!command)
            dosh_pid = new_pid;
        else
            mail_pid = new_pid;
        privon();
#ifdef CHDIR
        chdirx((char *) 0, 0);
#endif
    }

    hack_resume(screenoutput);

    if (!vms_ok(status)) {
        pline("  Spawn failed.  (%%x%08lX) ", status);
        mark_synch();
    }
    return 0;
}
#endif /* SHELL */

#ifdef SUSPEND
/* dosuspend() -- if we're a subprocess, attach to our parent;
 *                if not, there's nothing we can do.
 */
int
dosuspend(void)
{
    static long owner_pid = -1;
    unsigned long status;

    if (owner_pid == -1) /* need to check for parent */
        owner_pid = getppid();
    if (owner_pid == 0) {
        pline(
 "  No parent process.  Use '!' to Spawn, 'S' to Save, or '#quit' to Quit. ");
        mark_synch();
        return 0;
    }

    /* restore normal tty environment & clear screen */
    hack_escape(1,
     " Attaching to parent process; use the ATTACH command to resume play. ");

    status = lib$attach(&owner_pid); /* connect to parent */

    hack_resume(1); /* resume game tty environment & refresh screen */

    if (!vms_ok(status)) {
        pline("  Unable to attach to parent.  (%%x%08lX) ", status);
        mark_synch();
    }
    return 0;
}
#endif /* SUSPEND */

#ifdef SELECTSAVED
/* this would fit better in vmsfiles.c except that that gets linked
   with the utility programs and we don't want this code there */

static void savefile(const char *, int, int *, char ***);

static void
savefile(const char *name, int indx, int *asize, char ***array)
{
    char **newarray;
    int i, oldsize;

    /* (asize - 1) guarantees that [indx + 1] will exist and be set to null */
    while (indx >= *asize - 1) {
        oldsize = *asize;
        *asize += 5;
        newarray = (char **) alloc(*asize * sizeof (char *));
        /* poor man's realloc() */
        for (i = 0; i < *asize; ++i)
            newarray[i] = (i < oldsize) ? (*array)[i] : 0;
        if (*array)
            free((genericptr_t) *array);
        *array = newarray;
    }
    (*array)[indx] = dupstr(name);
}

struct dsc {
    unsigned short len, mbz;
    char *adr;
};                             /* descriptor */
typedef unsigned long vmscond; /* vms condition value */

#ifndef VMSVSI
vmscond lib$find_file(const struct dsc *, struct dsc *, genericptr *);
vmscond lib$find_file_end(void **);
#endif

/* collect a list of character names from all save files for this player */
int
vms_get_saved_games(const char *savetemplate, /* wildcarded save file name in native VMS format */
                    char ***outarray)
{
    struct dsc in, out;
    unsigned short l;
    int count, asize;
    char *charname, wildcard[255 + 1], filename[255 + 1];
    genericptr_t context = 0;

    Strcpy(wildcard, savetemplate); /* plname_from_file overwrites gs.SAVEF */
    in.mbz = 0; /* class and type; leave them unspecified */
    in.len = (unsigned short) strlen(wildcard);
    in.adr = wildcard;
    out.mbz = 0;
    out.len = (unsigned short) (sizeof filename - 1);
    out.adr = filename;

    *outarray = 0;
    count = asize = 0;
    /* note: only works as intended if savetemplate is a wildcard filespec */
    while (lib$find_file(&in, &out, &context) & 1) {
        /* strip trailing blanks */
        for (l = out.len; l > 0; --l)
            if (filename[l - 1] != ' ')
                break;
        filename[l] = '\0';
        if ((charname = plname_from_file(filename)) != 0)
            savefile(charname, count++, &asize, outarray);
    }
    (void) lib$find_file_end(&context);

    return count;
}
#endif /* SELECTSAVED */

#ifdef PANICTRACE
/* nethack has detected an internal error; try to give a trace of call stack
 */
void
vms_traceback(int how) /* 1: exit after traceback; 2: stay in debugger */
{
    /* assumes that a static initializer applies to the first union
       field and that no padding will be placed between len and str */
    union dbgcmd {
        struct ascic {
            unsigned char len; /* 8-bit length prefix */
            char str[79]; /* could be up to 255, but we don't need so much */
        } cmd_fields;
        char cmd[1 + 79];
    };
#define DBGCMD(arg) { (unsigned char) (sizeof arg - sizeof ""), arg }
    static union dbgcmd dbg[3] = {
        /* prologue for less verbose feedback (when combined with
           $ define/User_mode dbg$output _NL: ) */
        DBGCMD("set Log SYS$OUTPUT: ; set Output Log,noTerminal,noVerify"),
        /* enable modules with calls present on stack, then show those calls;
           limit traceback to 18 stack frames to avoid scrolling off screen
           (could check termcap LI and maybe give more, but we're operating
           in a last-gasp environment so apply the KISS principle...) */
        DBGCMD("set Module/Calls ; show Calls 18"),
        /* epilogue; "exit" ends the sequence it's part of, but it doesn't
           seem able to cause program termination when used separately;
           instead of relying on it, we'll redirect debugger input to come
           from the null device so that it'll get an end-of-input condition
           when it tries to get a command from the user */
        DBGCMD("exit"),
    };
#undef DBGCMD

    /*
     * If we've been linked /noTraceback then we can't provide any
     * trace of the call stack.  Linking that way is required if
     * nethack.exe is going to be installed with privileges, so the
     * SECURE configuration usually won't have any trace feedback.
     */
    if (!debuggable) {
        ; /* debugger not available to catch lib$signal(SS$_DEBUG) */
    } else if (how == 2) {
        /* omit prologue and epilogue (dbg[0] and dbg[2]) */
        (void) lib$signal(SS$_DEBUG, 1, dbg[1].cmd);
    } else if (how == 1) {
        /*
         * Suppress most of debugger's initial feedback to avoid scaring
         * users (and scrolling panic message off the screen).  Also control
         * debugging environment to try to prevent unexpected complications.
         */
        /* start up with output going to /dev/null instead of stdout;
           once started, output is sent to log file that's actually stdout */
        (void) vms_define("DBG$OUTPUT", "_NL:", 0);
        /* take input from null device so debugger will see end-on-input
           and quit if/when it tries to get a command from the user */
        (void) vms_define("DBG$INPUT", "_NL:", 0);
        /* bypass any debugger initialization file the user might have */
        (void) vms_define("DBG$INIT", "_NL:", 0);
        /* force tty interface by suppressing DECwindows/Motif interface */
        (void) vms_define("DBG$DECW$DISPLAY", " ", 0);
        /* raise an exception for the debugger to catch */
        (void) lib$signal(SS$_DEBUG, 3, dbg[0].cmd, dbg[1].cmd, dbg[2].cmd);
    }

    vms_exit(2); /* don't return to caller (2==arbitrary non-zero) */
    /* NOT REACHED */
}
#endif /* PANICTRACE */

/*
 * Play Hunt the Wumpus to see whether the debugger lurks nearby.
 * It all takes place before nethack even starts, and sets up
 * `debuggable' to control possible use of lib$signal(SS$_DEBUG).
 */
typedef unsigned (*condition_handler)(unsigned *, unsigned *);
extern condition_handler lib$establish(condition_handler);
extern unsigned lib$sig_to_ret(unsigned *, unsigned *);

/* SYS$IMGSTA() is not documented:  if called at image startup, it controls
   access to the debugger; fortunately, the linker knows now to find it
   without needing to link against sys.stb (VAX) or use LINK/System (Alpha).
   We won't be calling it, but we indirectly check whether it has already
   been called by checking if nethack.exe has it as a transfer address. */
extern unsigned sys$imgsta(void);

/*
 * These structures are in header files contained in sys$lib_c.tlb,
 * but that isn't available on sufficiently old versions of VMS.
 * Construct our own:  partly stubs, with simpler field names and
 * without ugly unions.  Contents derived from Bliss32 definitions
 * in lib.req and/or Macro32 definitions in lib.mlb.
 */
struct ihd { /* (vax) image header, $IHDDEF */
    unsigned short size, activoff;
    unsigned char otherstuff[512 - 4];
};
struct eihd { /* extended image header, $EIHDDEF */
    unsigned long majorid, minorid, size, isdoff, activoff;
    unsigned char otherstuff[512 - 20];
};
struct iha { /* (vax) image header activation block, $IHADEF */
    unsigned long trnadr1, trnadr2, trnadr3;
    unsigned long fill_, inishr;
};
struct eiha { /* extended image header activation block, $EIHADEF */
    unsigned long size, spare;
    unsigned long trnadr1[2], trnadr2[2], trnadr3[2], trnadr4[2], inishr[2];
};

/*
 *      We're going to use lib$initialize, not because we need or
 *      want to be called before main(), but because one of the
 *      arguments passed to a lib$initialize callback is a pointer
 *      to the image header (somewhat complex data structure which
 *      includes the memory location(s) of where to start executing)
 *      of the program being initialized.  It comes in two flavors,
 *      one used by VAX and the other by Alpha and IA64.
 *
 *      An image can have up to three transfer addresses; one of them
 *      decides whether to run under debugger control (RUN/Debug, or
 *      LINK/Debug + plain RUN), another handles lib$initialize calls
 *      if that's used, and the last is to start the program itself
 *      (a jacket built around main() for code compiled with DEC C).
 *      They aren't always all present; some might be zero/null.
 *      A shareable image (pre-linked library) usually won't have any,
 *      but can have a separate initializer (not of interest here).
 *
 *      The transfer targets don't have fixed slots but do occur in a
 *      particular order:
 *                    link      link     lib$initialize lib$initialize
 *          sharable  /noTrace  /Trace    + /noTrace     + /Traceback
 *      1:  (none)    main      debugger  init-handler   debugger
 *      2:                      main      main           init-handler
 *      3:                                               main
 *
 *      We check whether the first transfer address is SYS$IMGSTA().
 *      If it is, the debugger should be available to catch SS$_DEBUG
 *      exception even when we don't start up under debugger control.
 *      One extra complication:  if we *do* start up under debugger
 *      control, the first address in the in-memory copy of the image
 *      header will be changed from sys$imgsta() to a value in system
 *      space.  [I don't know how to reference that one symbolically,
 *      so I'm going to treat any address in system space as meaning
 *      that the debugger is available.  pr]
 */

/* called via lib$initialize during image activation:  before main() and
   with magic arguments; C run-time library won't be initialized yet */
/*ARGSUSED*/
int
vmsexeini(const void *inirtn_unused, const void *clirtn_unused,
          const unsigned char *imghdr)
{
    const struct ihd *vax_hdr;
    const struct eihd *axp_hdr;
    const struct iha *vax_xfr;
    const struct eiha *axp_xfr;
    unsigned long trnadr1;

    (void) lib$establish(lib$sig_to_ret); /* set up condition handler */

    /*
     * Check the first of three transfer addresses to see whether
     * it is SYS$IMGSTA().  Note that they come from a file,
     * where they reside as longword or quadword integers rather
     * than function pointers.  (Basically just a C type issue;
     * casting back and forth between integer and pointer doesn't
     * change any bits for the architectures VMS runs on.)
     */
    debuggable = 0;
    /* start with a guess rather than bothering to figure out architecture */
    vax_hdr = (struct ihd *) imghdr;
    if (vax_hdr->size >= 512) {
        /* this is a VAX-specific header; addresses are longwords */
        vax_xfr = (struct iha *) (imghdr + vax_hdr->activoff);
        trnadr1 = vax_xfr->trnadr1;
    } else {
        /* the guess above was wrong; imghdr's first word is not
           the size field, it's a version number component */
        axp_hdr = (struct eihd *) imghdr;
        /* this is an Alpha or IA64 header; addresses are quadwords
           but we ignore the upper half which will be all 0's or 0xF's
           (we hope; if not, assume it still won't matter for this test) */
        axp_xfr = (struct eiha *) (imghdr + axp_hdr->activoff);
        trnadr1 = axp_xfr->trnadr1[0];
    }
    if ((unsigned (*) ()) trnadr1 == sys$imgsta ||
        /* check whether first transfer address points to system space
           [we want (trnadr1 >= 0x80000000UL) but really old compilers
           don't support the UL suffix, so do a signed compare instead] */
        (long) trnadr1 < 0L)
        debuggable = 1;
    return 1; /* success (return value here doesn't actually matter) */
}

/*
 * Setting up lib$initialize transfer block is trivial with Macro32,
 * but we don't want to introduce use of assembler code.  Doing it
 * with C requires jiggery-pokery here and again when linking, and
 * may not work with some compiler versions.  The lib$initialize
 * transfer block is an open-ended array of 32-bit routine addresses
 * in a psect named "lib$initialize" with particular attributes (one
 * being "concatenate" so that multiple instances of lib$initialize
 * are appended rather than overwriting each other).
 *
 * VAX C made global variables become named program sections, to be
 * compatable with Fortran COMMON blocks, simplifying mixed-language
 * programs.  GNU C for VAX/VMS did the same, to be compatable with
 * VAX C.  By default, DEC C makes global variables be global symbols
 * instead, with its /Extern_Model=Relaxed_Ref_Def mode, but can be
 * told to be VAX C compatable by using /Extern_Model=Common_Block.
 *
 * We don't want to force that for the whole program; occasional use
 * of /Extern_Model=Strict_Ref_Def to find mistakes is too useful.
 * Also, using symbols instead of psects is more robust when linking
 * with an object library if the module defining the symbol contains
 * only data.  With a psect, any declaration is enough to become a
 * definition and the linker won't bother hunting through a library
 * to find another one unless explicitly told to do so.  Bad news
 * if that other one happens to include the intended initial value
 * and someone bypasses `make' to link interactively but neglects
 * to give the linker enough explicit directions.  Linking like that
 * would work, but the program wouldn't.
 *
 * So, we switch modes for this hack only.  Besides, psect attributes
 * for lib$initialize are different from the ones used for ordinary
 * variables, so we'd need to resort to some linker magic anyway.
 * (With assembly language, in addtion to having full control of the
 * psect attributes in the source code, Macro32 would include enough
 * information in its object file such that linker wouldn't need any
 * extra instructions from us to make this work.)  [If anyone links
 * manually now and neglects the esoteric details, vmsexeini() won't
 * get called and `debuggable' will stay 0, so lib$signal(SS$_DEBUG)
 * will be avoided even when its use is viable.  But the program will
 * still work correctly.]
 */
#define C_LIB$INITIALIZE /* comment out if this won't compile...   */
/* (then `debuggable' will always stay 0) */
#ifdef C_LIB$INITIALIZE
#ifdef __DECC
#pragma extern_model save         /* push current mode */
#pragma extern_model common_block /* set new mode */
#endif
/* values are 32-bit function addresses; pointers might be 64 so avoid them */
extern const unsigned long lib$initialize[1]; /* size is actually variable */
const unsigned long lib$initialize[] = { (unsigned long) (void *) vmsexeini };
#ifdef __DECC
#pragma extern_model restore /* pop previous mode */
#endif
/*      We also need to link against a linker options file containing:
sys$library:starlet.olb/Include=(lib$initialize)
psect_attr=lib$initialize, Con,Rel,Gbl,noShr,noExe,Rd,noWrt
 */
#endif /* C_LIB$INITIALIZE */
/* End of debugger hackery. */

/*vmsunix.c*/
