/* NetHack 3.7	report.c	$NHDT-Date: 1710525914 2024/03/15 18:05:14 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.7 $ */
/* Copyright (c) Kenneth Lorber, Kensington, Maryland, 2024 */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* NB: CRASHREPORT implies PANICTRACE */

# ifndef NO_SIGNAL
#include <signal.h>
# endif
#include <ctype.h>
# ifndef LONG_MAX
#include <limits.h>
# endif
#include "dlb.h"
#include <fcntl.h>

#include <errno.h>
# ifdef PANICTRACE_LIBC
#include <execinfo.h>
# endif

#ifdef CRASHREPORT
# ifdef WIN32
#  define HASH_PRAGMA_START
#  define HASH_PRAGMA_END
# else
#  define HASH_PRAGMA_START           \
    _Pragma("GCC diagnostic push"); \
    _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#  define HASH_PRAGMA_END _Pragma("GCC diagnostic pop");
# endif
# ifdef MACOS
#include <CommonCrypto/CommonDigest.h>
#  define HASH_CONTEXTPTR(CTXP) \
    unsigned char tmp[CC_MD4_DIGEST_LENGTH]; \
    CC_MD4_CTX CTXP ## _;                    \
    CC_MD4_CTX *CTXP = &CTXP ## _
#  define HASH_INIT(ctxp) !CC_MD4_Init(ctxp)
#  define HASH_UPDATE(ctx, ptr, len) !CC_MD4_Update(ctx, ptr, len)
#  define HASH_FINISH(ctxp) !CC_MD4_Final(tmp, ctxp)
#  define HASH_RESULT_SIZE(ctxp) CC_MD4_DIGEST_LENGTH
#  define HASH_RESULT(ctx, inp) *inp = (unsigned char *) ctx
#  define HASH_CLEANUP(ctxp)
#  define HASH_OFLAGS O_RDONLY
#  define HASH_BINFILE_DECL char *binfile = argv[0];
#  if (NH_DEVEL_STATUS == NH_STATUS_BETA)
#   define HASH_BINFILE \
    if (!binfile || !*binfile) {                                        \
        /* If this triggers, investigate CFBundleGetMainBundle */       \
        /* or CFBundleCopyExecutableURL. */                             \
        raw_print(                                                      \
          "BETA warning: crashreport_init called without useful info"); \
        goto skip;                                                      \
    }
#  else /* BETA */
#  define HASH_BINFILE() \
    if (!binfile || !*binfile) {                \
        goto skip;                              \
    }
#  endif  /* BETA */
# endif  /* MACOS */

# ifdef __linux__
#include "nhmd4.h"
/* v0 is just to suppress compiler warnings about unreachable code */
#  define HASH_CONTEXTPTR(CTXP) \
    volatile int v0 = 0; \
    unsigned char tmp[NHMD4_DIGEST_LENGTH];     \
    NHMD4_CTX CTXP ## _;                        \
    NHMD4_CTX *CTXP = &CTXP ## _
#  define HASH_INIT(ctxp) (nhmd4_init(ctxp), v0)
#  define HASH_UPDATE(ctx, ptr, len) (nhmd4_update(ctx, ptr, len), v0)
#  define HASH_FINISH(ctxp) (nhmd4_final(ctxp, tmp), v0)
#  define HASH_RESULT_SIZE(ctxp) NHMD4_RESULTLEN
#  define HASH_RESULT(ctx, inp) *inp = tmp
#  define HASH_CLEANUP(ctxp)
#  define HASH_OFLAGS O_RDONLY
#  define HASH_BINFILE_DECL char binfile[PATH_MAX+1];
#  define HASH_BINFILE() \
    int len = readlink("/proc/self/exe", binfile, sizeof binfile - 1);  \
    if (len > 0) {                                                      \
        binfile[len] = '\0';                                            \
    } else {                                                            \
        goto skip;                                                      \
    }
# endif // __linux__

# ifdef WIN32
/* WIN32 takes too much code and is dependent on OS includes we can't
 * pull in here, so we call out to code in sys/windows/windsys.c */
#  define HASH_CONTEXTPTR(CTXP)
#  define HASH_INIT(ctxp) win32_cr_helper('i', ctxp, NULL, 0)
#  define HASH_UPDATE(ctxp, ptr, len) win32_cr_helper('u', ctxp, ptr, len)
#  define HASH_FINISH(ctxp) win32_cr_helper('f', ctxp, NULL, 0)
#  define HASH_CLEANUP(ctxp) win32_cr_helper('c', ctxp, NULL, 0)
#  define HASH_RESULT_SIZE(ctxp) win32_cr_helper('s', ctxp, NULL, 0)
#  define HASH_RESULT(ctxp, inp) win32_cr_helper('r', ctxp, inp, 0)
#  define HASH_OFLAGS _O_RDONLY | _O_BINARY
#  define HASH_BINFILE_DECL char *binfile;
#  define HASH_BINFILE() \
    if (win32_cr_helper('b', NULL, &binfile, 0)) {      \
        goto skip;                                      \
    }
# endif // WIN32

/* Binary ID - Use only as a hint to contact.html for recognizing our own
   binaries.  This is easily spoofed! */
static char bid[40];

/* ARGSUSED */
void
crashreport_init(int argc UNUSED, char *argv[] UNUSED)
{
    static int once = 0;
    if (once++) /* NetHackW.exe calls us twice */
        return;
    HASH_BINFILE_DECL;
    HASH_PRAGMA_START
    HASH_CONTEXTPTR(ctxp);
    if (HASH_INIT(ctxp))
        goto skip;
    HASH_BINFILE(); /* Does "goto skip" on error. */

    int fd = open(binfile, HASH_OFLAGS, 0);
    if (fd == -1) {
# ifdef BETA
        raw_printf("open e=%s", strerror(errno));
# endif
        goto skip;
    }

    int segsize;
    unsigned char segment[4096];

    while (0 < (segsize = read(fd, segment, sizeof segment))) {
        if (HASH_UPDATE(ctxp, segment, segsize))
            goto skip;
    }
    if (segsize < 0) {
        close(fd);
        goto skip;
    }
    if (HASH_FINISH(ctxp))
        goto skip;
    close(fd);

    static const char hexdigits[] = "0123456789abcdef";
    char *p = bid;
    unsigned char *in;
    HASH_RESULT(ctxp, &in);
    uint8 cnt = (uint8) HASH_RESULT_SIZE(ctxp);
    /* Just in case, make sure not to overflow the bid buffer.
       Divide size by 2 because each octet in the hash uses two slots
       in bid[] when formatted as a pair of hexadecimal digits. */
    if (cnt >= (uint8) sizeof bid / 2)
        cnt = (uint8) sizeof bid / 2 - 1;
    while (cnt) {
        /* sprintf(p, "%02x", *in++), p += 2; */
        *p++ = hexdigits[(*in >> 4) & 0x0f];
        *p++ = hexdigits[*in++ & 0x0f];
        --cnt;
    }
    *p = '\0';
    return;

 skip:
    Strcpy(bid, "unknown");
    HASH_CLEANUP(ctxp);
    HASH_PRAGMA_END
}

#undef HASH_CONTEXTPTR
#undef HASH_INIT
#undef HASH_UPDATE
#undef HASH_FINISH
#undef HASH_CLEANUP
#undef HASH_RESULT
#undef HASH_RESULT_SIZE
#undef HASH_PRAGMA_START
#undef HASH_PRAGMA_END
#undef HASH_BINFILE_DECL
#undef HASH_BINFILE

void
crashreport_bidshow(void)
{
# if defined(WIN32) && !defined(WIN32CON)
    if (0 == win32_cr_helper('D', ctxp, bid, 0))
# endif
    {
        raw_print(bid);
# ifdef WIN32notyet
        wait_synch();
# endif
    }
}

/* Build a URL with a query string and try to launch a new browser window
 * to report from panic() or impossible().  Requires libc support for
 * the stacktrace.  Uses memory on the stack to avoid memory allocation
 * (on most platforms) (but libc can still do anything it wants). */

// No theoretial limit for URL length but reality is messy.
// This should work on all modern platforms.
# ifndef MAX_URL
# define MAX_URL 8192
# endif
# ifndef SWR_FRAMES
# define SWR_FRAMES 20
# endif

// mark holds the initial eos; if we can't get the value in
// then we can remove the whole item if desired.  For other
// semantics, caller can handle mark.
#define SWR_ADD(str) \
    utmp = strlen(str);                         \
    mark = uend;                                \
    if (utmp >= urem)                           \
        goto full;                              \
    memcpy(uend, str, utmp);			\
    uend += utmp; urem -= utmp;                 \
    *uend = '\0';

// NB: on overflow this rolls us back to mark, so if we don't
//     want to roll back to the last SWR_ADD, update mark before
//     calling this macro.
#define SWR_ADD_URIcoded(str) \
    if (swr_add_uricoded(str, &uend, &urem, mark))      \
        goto full;

/* On overflow, truncate to markp (but only if markp != NULL). */
boolean
swr_add_uricoded(
    const char *in,
    char **out,
    int *remaining,
    char *markp)
{
    while (*in) {
        if (isalnum(*in) || strchr("_-.~", *in)) {
            **out = *in;
            (*out)++;
            (*remaining)--;
        } else if (*in == ' ') {
            **out = '+';
            (*out)++;
            (*remaining)--;
        } else {
            if (*remaining <= 3) {
                if (markp)
                    *out = markp, *remaining = 0;
                **out = '\0';
                return TRUE;
            }
            int x = snprintf(*out, *remaining, "%%%02X", *in);
            *out += x;
            *remaining -= x;
        }
        in++;
        if (!*remaining) {
            if (markp)
                *out = markp, *remaining = 0;
            **out = '\0';
            return TRUE;
        }
        **out = '\0';
    }
    return FALSE; /* normal return */
}

static char url[MAX_URL];   // XXX too bad this isn't allocated as needed
static int urem = MAX_URL;  // adjusted for gc.crash_urlmax below
static char *uend = url;
static int utmp;            // used inside macros
static char *mark;          // holds previous terminator (generally)

boolean
submit_web_report(int cos, const char *msg, const char *why)
{
    urem = (gc.crash_urlmax < 0 || gc.crash_urlmax > MAX_URL)
           ? MAX_URL : min(MAX_URL,gc.crash_urlmax);
    char temp[200];
    char temp2[200];
    int countpp = 0; /* pre and post traceback lines */
//  URL loaded for creating reports to the NetHack DevTeam
// CRASHREPORTURL=https://nethack.org/links/cr-37BETA.html
    if (!sysopt.crashreporturl)
        return FALSE;
    SWR_ADD(sysopt.crashreporturl);
        /* cos - operation, v - version */
    Snprintf(temp, sizeof temp, "?cos=%d&v=1", cos);
    SWR_ADD(temp);

        /* msg==NULL for #bugreport */
    if (msg) {
        SWR_ADD("&subject=");
        Snprintf(temp, sizeof temp, "%s report for NetHack %s",
                 msg, version_string(temp2, sizeof temp2 ));
        SWR_ADD_URIcoded(temp);
    }

    SWR_ADD("&gitver=");
    SWR_ADD_URIcoded(getversionstring(temp2, sizeof temp2));

    if (gc.crash_name) {
        SWR_ADD("&name=");
        SWR_ADD_URIcoded(gc.crash_name);
    }

    if(gc.crash_email) {
        SWR_ADD("&email=");
        SWR_ADD_URIcoded(gc.crash_email);
    }
            // hardware: leave for user
            // software: leave for user
            // comments: leave for user

    SWR_ADD("&details=");
    if (why) {
        SWR_ADD_URIcoded(why);
        SWR_ADD_URIcoded("\n");
        mark = uend;
        countpp++;
    }

    SWR_ADD_URIcoded("bid: ");
    SWR_ADD_URIcoded(bid);
    SWR_ADD_URIcoded("\n");
    mark = uend;
    countpp++;

    int count = 0;
    if (cos == 1) {
# ifdef WIN32
        count = win32_cr_gettrace(SWR_FRAMES, uend, MAX_URL - (uend - url));
        uend = eos(url);
# else
        void *bt[SWR_FRAMES];
        int x;
        char **info;

        count = backtrace(bt, SIZE(bt));
        info = backtrace_symbols(bt, count);
        for (x = 0; x < count; x++) {
            copynchars(temp, info[x], (int) sizeof temp - 1 - 1); /* \n\0 */
            /* try to remove up to 16 blank spaces by removing 8 twice */
            (void) strsubst(temp, "        ", "");
            (void) strsubst(temp, "        ", "");
            (void) strncat(temp, "\n", sizeof temp - 1);
#  if 0 // __linux__
// not needed for MacOS
// XXX is it actually needed for linux?  TBD
            Snprintf(temp2, sizeof temp2, "[%02lu]\n", (unsigned long) x);
            uend--;    // remove the \n we added above
            SWR_ADD_URIcoded(temp2);
#  endif // linux
            SWR_ADD_URIcoded(temp);
            mark = uend;
        }
# endif  // !WIN32
    }

# ifdef DUMPLOG_CORE
        // config.h turns this on, but make it easy to turn off if needed
    if (cos == 1) {
        int k;
        SWR_ADD_URIcoded("Latest messages:\n");
        mark=uend;
        countpp++;
        for (k = 0; k < 5; k++) {
            const char *line = get_saved_pline(k);
            if (!line)
                break;
            SWR_ADD_URIcoded(line);
            SWR_ADD_URIcoded("\n");
            countpp++;
            mark = uend;
        }
    }
# endif /* DUMPLOG_CORE */

                // detailrows: Guess since we can't know the
                //     width of the window.
        SWR_ADD("&detailrows=");
        Snprintf(temp, sizeof temp, "%d", min(count + countpp, 30));
        SWR_ADD_URIcoded(temp);

 full:
        ;
//printf("URL=%ld '%s'\n",strlen(url),url);
# ifdef WIN32
        int *rv = win32_cr_shellexecute(url);
// XXX TESTING
printf("ShellExecute returned: %p\n",rv);   // >32 is ok
# else  /* !WIN32 */
        const char *xargv[] = {
            CRASHREPORT,
            url,
            NULL
        };
        int pid = fork();
        extern char **environ;
        if (pid == 0) {
            char err[100];
#  ifdef CRASHREPORT_EXEC_NOSTDERR
	    int devnull;
                /* Keep the output clean - firefox spews useless errors on
                 * my system. */
            (void) close(2);
            devnull =  open("/dev/null", O_WRONLY);
#  endif

            (void) execve(CRASHREPORT, (char * const *) xargv, environ);
            Sprintf(err, "Can't start " CRASHREPORT ": %s", strerror(errno));
            raw_print(err);
#  ifdef CRASHREPORT_EXEC_NOSTDERR
	    (void) close(devnull);
#  endif
	    exit(1);
        } else {
            int status;
            errno = 0;
            (void) waitpid(pid, &status, 0);
            if (status) {         /* XXX check could be more precise */
#  ifdef BETA
                    /* Not useful at the moment. XXX */
                char err[100];

                Sprintf(err, "pid=%d e=%d status=%0x", wpid, errno, status);
                raw_print(err);
#  endif
                return FALSE;
            }
        }
        /* free(info);   -- Don't risk it. */
# endif  /* !WIN32 */
        return TRUE;
}

int
dobugreport(void)
{
    if (!submit_web_report(2, NULL, "#bugreport command")) {
        pline("Unable to send bug report.  Please visit %s instead.",
              (sysopt.crashreporturl && *sysopt.crashreporturl)
              ? sysopt.crashreporturl
              : "https://www.nethack.org"
        );
    }
    return ECMD_OK;
}

#undef SWR_ADD
#undef SWR_ADD_URIcoded
#undef SWR_FRAMES
#undef SWR_HDR
#undef SWR_LINES

#endif /* CRASHREPORT */

#ifdef PANICTRACE

/*ARGSUSED*/
boolean
NH_panictrace_libc(void)
{
# if 0       /* XXX how did this get left here? */
    if (submit_web_report("Panic", why))
        return TRUE;
# endif

# ifdef PANICTRACE_LIBC
    void *bt[20];
    int count, x;
    char **info, buf[BUFSZ];

    raw_print("  Generating more information you may report:\n");
    count = backtrace(bt, SIZE(bt));
    info = backtrace_symbols(bt, count);
    for (x = 0; x < count; x++) {
        copynchars(buf, info[x], (int) sizeof buf - 1);
        /* try to remove up to 16 blank spaces by removing 8 twice */
        (void) strsubst(buf, "        ", "");
        (void) strsubst(buf, "        ", "");
        raw_printf("[%02lu] %s", (unsigned long) x, buf);
    }
    /* free(info);   -- Don't risk it. */
    return TRUE;
# else
    return FALSE;
# endif /* !PANICTRACE_LIBC */
}

/*
 *   fooPATH  file system path for foo
 *   fooVAR   (possibly const) variable containing fooPATH
 */
# ifdef PANICTRACE_GDB
#  ifdef SYSCF
#  define GDBVAR sysopt.gdbpath
#  define GREPVAR sysopt.greppath
#  else /* SYSCF */
#  define GDBVAR GDBPATH
#  define GREPVAR GREPPATH
#  endif /* SYSCF */
# endif /* PANICTRACE_GDB */

boolean
NH_panictrace_gdb(void)
{
# ifdef PANICTRACE_GDB
    /* A (more) generic method to get a stack trace - invoke
     * gdb on ourself. */
    const char *gdbpath = GDBVAR;
    const char *greppath = GREPVAR;
    char buf[BUFSZ];
    FILE *gdb;

    if (gdbpath == NULL || gdbpath[0] == 0)
        return FALSE;
    if (greppath == NULL || greppath[0] == 0)
        return FALSE;

    sprintf(buf, "%s -n -q %s %d 2>&1 | %s '^#'",
            gdbpath, ARGV0, getpid(), greppath);
    gdb = popen(buf, "w");
    if (gdb) {
        raw_print("  Generating more information you may report:\n");
        fprintf(gdb, "bt\nquit\ny");
        fflush(gdb);
        sleep(4); /* ugly */
        pclose(gdb);
        return TRUE;
    } else {
        return FALSE;
    }
# else
    return FALSE;
# endif /* !PANICTRACE_GDB */
}

#ifdef DUMPLOG_CORE
#define USED_if_dumplog
#else
#define USED_if_dumplog UNUSED
#endif

/* lineno==0 gives the most recent message (e.g.
   "Do you want to call panic..." if called from #panic) */
const char *
get_saved_pline(int lineno USED_if_dumplog)
{
#ifdef DUMPLOG_CORE
    int p;
    int limit = DUMPLOG_MSG_COUNT;
    if (lineno >= DUMPLOG_MSG_COUNT)
        return NULL;
    p = (gs.saved_pline_index - 1) % DUMPLOG_MSG_COUNT;

    while (limit--) {
        if (gs.saved_plines[p]) { /* valid line */
            if (lineno--) {
                p = (p - 1 + DUMPLOG_MSG_COUNT) % DUMPLOG_MSG_COUNT;
            } else {
                return gs.saved_plines[p];
            }
        }
    }
#endif /* DUMPLOG_CORE */
    return NULL;
}

#undef USED_if_dumplog

# ifndef NO_SIGNAL
/* called as signal() handler, so sent at least one arg */
/*ARGUSED*/
void
panictrace_handler(int sig_unused UNUSED)
{
#define SIG_MSG "\nSignal received.\n"
    int f2;

#  ifdef CURSES_GRAPHICS
    if (iflags.window_inited && WINDOWPORT(curses)) {
        extern void curses_uncurse_terminal(void); /* wincurs.h */

        /* it is risky calling this during a program-terminating signal,
           but without it the subsequent backtrace is useless because
           that ends up being scrawled all over the screen; call is
           here rather than in NH_abort() because panic() calls both
           exit_nhwindows(), which makes this same call under curses,
           then NH_abort() and we don't want to call this twice */
        curses_uncurse_terminal();
    }
#  endif

    f2 = (int) write(2, SIG_MSG, sizeof SIG_MSG - 1);
    nhUse(f2);  /* what could we do if write to fd#2 (stderr) fails  */
    NH_abort(NULL); /* ... and we're already in the process of quitting? */
}

void
panictrace_setsignals(boolean set)
{
#define SETSIGNAL(sig) \
    (void) signal(sig, set ? (SIG_RET_TYPE) panictrace_handler : SIG_DFL);
# ifdef SIGILL
    SETSIGNAL(SIGILL);
# endif
# ifdef SIGTRAP
    SETSIGNAL(SIGTRAP);
# endif
# ifdef SIGIOT
    SETSIGNAL(SIGIOT);
# endif
# ifdef SIGBUS
    SETSIGNAL(SIGBUS);
# endif
# ifdef SIGFPE
    SETSIGNAL(SIGFPE);
# endif
# ifdef SIGSEGV
    SETSIGNAL(SIGSEGV);
# endif
# ifdef SIGSTKFLT
    SETSIGNAL(SIGSTKFLT);
# endif
# ifdef SIGSYS
    SETSIGNAL(SIGSYS);
# endif
# ifdef SIGEMT
    SETSIGNAL(SIGEMT);
# endif
#undef SETSIGNAL
}
# endif /* NO_SIGNAL */
#endif /* PANICTRACE */

/*report.c*/
