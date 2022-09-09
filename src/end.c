/* NetHack 3.7	end.c	$NHDT-Date: 1646322468 2022/03/03 15:47:48 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.240 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#define NEED_VARARGS /* comment line for pre-compiled headers */

#include "hack.h"
#ifndef NO_SIGNAL
#include <signal.h>
#endif
#include <ctype.h>
#ifndef LONG_MAX
#include <limits.h>
#endif
#include "dlb.h"


/* add b to long a, convert wraparound to max value */
#define nowrap_add(a, b) (a = ((a + b) < 0 ? LONG_MAX : (a + b)))

#ifndef NO_SIGNAL
static void done_intr(int);
#if defined(UNIX) || defined(VMS) || defined(__EMX__)
static void done_hangup(int);
#endif
#endif
static void disclose(int, boolean);
static void get_valuables(struct obj *);
static void sort_valuables(struct valuable_data *, int);
static void artifact_score(struct obj *, boolean, winid);
static void really_done(int) NORETURN;
static void savelife(int);
static boolean should_query_disclose_option(int, char *);
#ifdef DUMPLOG
static void dump_plines(void);
#endif
static void dump_everything(int, time_t);

#if defined(__BEOS__) || defined(MICRO) || defined(OS2) || defined(WIN32)
extern void nethack_exit(int) NORETURN;
#else
#define nethack_exit exit
#endif

#define done_stopprint g.program_state.stopprint

#ifndef PANICTRACE
#define NH_abort NH_abort_
#endif

#ifdef AMIGA
#define NH_abort_() Abort(0)
#else
#ifdef SYSV
#define NH_abort_() (void) abort()
#else
#ifdef WIN32
#define NH_abort_() win32_abort()
#else
#define NH_abort_() abort()
#endif
#endif /* !SYSV */
#endif /* !AMIGA */

#ifdef PANICTRACE
#include <errno.h>
#ifdef PANICTRACE_LIBC
#include <execinfo.h>
#endif

/* What do we try and in what order?  Tradeoffs:
 * libc: +no external programs required
 *        -requires newish libc/glibc
 *        -requires -rdynamic
 * gdb:   +gives more detailed information
 *        +works on more OS versions
 *        -requires -g, which may preclude -O on some compilers
 */
#ifdef SYSCF
#define SYSOPT_PANICTRACE_GDB sysopt.panictrace_gdb
#ifdef PANICTRACE_LIBC
#define SYSOPT_PANICTRACE_LIBC sysopt.panictrace_libc
#else
#define SYSOPT_PANICTRACE_LIBC 0
#endif
#else
#define SYSOPT_PANICTRACE_GDB (nh_getenv("NETHACK_USE_GDB") == 0 ? 0 : 2)
#ifdef PANICTRACE_LIBC
#define SYSOPT_PANICTRACE_LIBC 1
#else
#define SYSOPT_PANICTRACE_LIBC 0
#endif
#endif

static void NH_abort(void);
#ifndef NO_SIGNAL
static void panictrace_handler(int);
#endif
static boolean NH_panictrace_libc(void);
static boolean NH_panictrace_gdb(void);

#ifndef NO_SIGNAL
/* called as signal() handler, so sent at least one arg */
/*ARGUSED*/
void
panictrace_handler(int sig_unused UNUSED)
{
#define SIG_MSG "\nSignal received.\n"
    int f2;

#ifdef CURSES_GRAPHICS
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
#endif

    f2 = (int) write(2, SIG_MSG, sizeof SIG_MSG - 1);
    nhUse(f2);  /* what could we do if write to fd#2 (stderr) fails  */
    NH_abort(); /* ... and we're already in the process of quitting? */
}

void
panictrace_setsignals(boolean set)
{
#define SETSIGNAL(sig) \
    (void) signal(sig, set ? (SIG_RET_TYPE) panictrace_handler : SIG_DFL);
#ifdef SIGILL
    SETSIGNAL(SIGILL);
#endif
#ifdef SIGTRAP
    SETSIGNAL(SIGTRAP);
#endif
#ifdef SIGIOT
    SETSIGNAL(SIGIOT);
#endif
#ifdef SIGBUS
    SETSIGNAL(SIGBUS);
#endif
#ifdef SIGFPE
    SETSIGNAL(SIGFPE);
#endif
#ifdef SIGSEGV
    SETSIGNAL(SIGSEGV);
#endif
#ifdef SIGSTKFLT
    SETSIGNAL(SIGSTKFLT);
#endif
#ifdef SIGSYS
    SETSIGNAL(SIGSYS);
#endif
#ifdef SIGEMT
    SETSIGNAL(SIGEMT);
#endif
#undef SETSIGNAL
}
#endif /* NO_SIGNAL */

static void
NH_abort(void)
{
    int gdb_prio = SYSOPT_PANICTRACE_GDB;
    int libc_prio = SYSOPT_PANICTRACE_LIBC;
    static volatile boolean aborting = FALSE;

    /* don't execute this code recursively if a second abort is requested
       while this routine or the code it calls is executing */
    if (aborting)
        return;
    aborting = TRUE;

#ifndef VMS
    if (gdb_prio == libc_prio && gdb_prio > 0)
        gdb_prio++;

    if (gdb_prio > libc_prio) {
        (void) (NH_panictrace_gdb() || (libc_prio && NH_panictrace_libc()));
    } else {
        (void) (NH_panictrace_libc() || (gdb_prio && NH_panictrace_gdb()));
    }

#else /* VMS */
    /* overload otherwise unused priority for debug mode: 1 = show
       traceback and exit; 2 = show traceback and stay in debugger */
    /* if (wizard && gdb_prio == 1) gdb_prio = 2; */
    vms_traceback(gdb_prio);
    nhUse(libc_prio);

#endif /* ?VMS */

#ifndef NO_SIGNAL
    panictrace_setsignals(FALSE);
#endif
    NH_abort_();
}

static boolean
NH_panictrace_libc(void)
{
#ifdef PANICTRACE_LIBC
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
#else
    return FALSE;
#endif /* !PANICTRACE_LIBC */
}

/*
 *   fooPATH  file system path for foo
 *   fooVAR   (possibly const) variable containing fooPATH
 */
#ifdef PANICTRACE_GDB
#ifdef SYSCF
#define GDBVAR sysopt.gdbpath
#define GREPVAR sysopt.greppath
#else /* SYSCF */
#define GDBVAR GDBPATH
#define GREPVAR GREPPATH
#endif /* SYSCF */
#endif /* PANICTRACE_GDB */

static boolean
NH_panictrace_gdb(void)
{
#ifdef PANICTRACE_GDB
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
#else
    return FALSE;
#endif /* !PANICTRACE_GDB */
}
#endif /* PANICTRACE */

/*
 * The order of these needs to match the macros in hack.h.
 */
static NEARDATA const char *deaths[] = {
    /* the array of death */
    "died", "choked", "poisoned", "starvation", "drowning", "burning",
    "dissolving under the heat and pressure", "crushed", "turned to stone",
    "turned into slime", "genocided", "panic", "trickery", "quit",
    "escaped", "ascended"
};

static NEARDATA const char *ends[] = {
    /* "when you %s" */
    "died", "choked", "were poisoned",
    "starved", "drowned", "burned",
    "dissolved in the lava",
    "were crushed", "turned to stone",
    "turned into slime", "were genocided",
    "panicked", "were tricked", "quit",
    "escaped", "ascended"
};

static boolean Schroedingers_cat = FALSE;

/* called as signal() handler, so sent at least one arg */
/*ARGSUSED*/
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
        clear_nhwindow(WIN_MESSAGE);
        curs_on_u();
        wait_synch();
        if (g.multi > 0)
            nomul(0);
    } else {
        (void) done2();
    }
}

/* "#quit" command or keyboard interrupt */
int
done2(void)
{
    if (!paranoid_query(ParanoidQuit, "Really quit?")) {
#ifndef NO_SIGNAL
        (void) signal(SIGINT, (SIG_RET_TYPE) done1);
#endif
        clear_nhwindow(WIN_MESSAGE);
        curs_on_u();
        wait_synch();
        if (g.multi > 0)
            nomul(0);
        if (g.multi == 0) {
            u.uinvulnerable = FALSE; /* avoid ctrl-C bug -dlc */
            u.usleep = 0;
        }
        return ECMD_OK;
    }
#if (defined(UNIX) || defined(VMS) || defined(LATTICE))
    if (wizard) {
        int c;
#ifdef VMS
        extern int debuggable; /* sys/vms/vmsmisc.c, vmsunix.c */

        c = !debuggable ? 'n' : ynq("Enter debugger?");
#else
#ifdef LATTICE
        c = ynq("Create SnapShot?");
#else
        c = ynq("Dump core?");
#endif
#endif
        if (c == 'y') {
#ifndef NO_SIGNAL
            (void) signal(SIGINT, (SIG_RET_TYPE) done1);
#endif
            exit_nhwindows((char *) 0);
            NH_abort();
        } else if (c == 'q')
            done_stopprint++;
    }
#endif
#ifndef LINT
    done(QUIT);
#endif
    return ECMD_OK;
}

#ifndef NO_SIGNAL
/* called as signal() handler, so sent at least 1 arg */
/*ARGSUSED*/
static void
done_intr(int sig_unused UNUSED)
{
    done_stopprint++;
    (void) signal(SIGINT, SIG_IGN);
#if defined(UNIX) || defined(VMS)
    (void) signal(SIGQUIT, SIG_IGN);
#endif
    return;
}

#if defined(UNIX) || defined(VMS) || defined(__EMX__)
/* signal() handler */
static void
done_hangup(int sig)
{
    g.program_state.done_hup++;
    sethanguphandler((void (*)(int)) SIG_IGN);
    done_intr(sig);
    return;
}
#endif
#endif /* NO_SIGNAL */

DISABLE_WARNING_FORMAT_NONLITERAL /* one compiler warns if the format
                                     string is the result of a ? x : y */

void
done_in_by(struct monst *mtmp, int how)
{
    char buf[BUFSZ];
    struct permonst *mptr = mtmp->data,
                    *champtr = ((mtmp->cham >= LOW_PM)
                                   ? &mons[mtmp->cham]
                                   : mptr);
    boolean distorted = (boolean) (Hallucination && canspotmon(mtmp)),
            mimicker = (M_AP_TYPE(mtmp) == M_AP_MONSTER),
            imitator = (mptr != champtr || mimicker);

    You((how == STONING) ? "turn to stone..." : "die...");
    mark_synch(); /* flush buffered screen output */
    buf[0] = '\0';
    g.killer.format = KILLED_BY_AN;
    /* "killed by the high priest of Crom" is okay,
       "killed by the high priest" alone isn't */
    if ((mptr->geno & G_UNIQ) != 0 && !(imitator && !mimicker)
        && !(mptr == &mons[PM_HIGH_CLERIC] && !mtmp->ispriest)) {
        if (!type_is_pname(mptr))
            Strcat(buf, "the ");
        g.killer.format = KILLED_BY;
    }
    /* _the_ <invisible> <distorted> ghost of Dudley */
    if (mptr == &mons[PM_GHOST] && has_mgivenname(mtmp)) {
        Strcat(buf, "the ");
        g.killer.format = KILLED_BY;
    }
    (void) monhealthdescr(mtmp, TRUE, eos(buf));
    if (mtmp->minvis)
        Strcat(buf, "invisible ");
    if (distorted)
        Strcat(buf, "hallucinogen-distorted ");

    if (imitator) {
        char shape[BUFSZ];
        const char *realnm = pmname(champtr, Mgender(mtmp)),
                             *fakenm = pmname(mptr, Mgender(mtmp));
        boolean alt = is_vampshifter(mtmp);

        if (mimicker) {
            /* realnm is already correct because champtr==mptr;
               set up fake mptr for type_is_pname/the_unique_pm */
            mptr = &mons[mtmp->mappearance];
            fakenm = pmname(mptr, Mgender(mtmp));
        } else if (alt && strstri(realnm, "vampire")
                   && !strcmp(fakenm, "vampire bat")) {
            /* special case: use "vampire in bat form" in preference
               to redundant looking "vampire in vampire bat form" */
            fakenm = "bat";
        }
        /* for the alternate format, always suppress any article;
           pname and the_unique should also have s_suffix() applied,
           but vampires don't take on any shapes which warrant that */
        if (alt || type_is_pname(mptr)) /* no article */
            Strcpy(shape, fakenm);
        else if (the_unique_pm(mptr)) /* "the"; don't use the() here */
            Sprintf(shape, "the %s", fakenm);
        else /* "a"/"an" */
            Strcpy(shape, an(fakenm));
        /* omit "called" to avoid excessive verbosity */
        Sprintf(eos(buf),
                alt ? "%s in %s form"
                    : mimicker ? "%s disguised as %s"
                               : "%s imitating %s",
                realnm, shape);
        mptr = mtmp->data; /* reset for mimicker case */
    } else if (mptr == &mons[PM_GHOST]) {
        Strcat(buf, "ghost");
        if (has_mgivenname(mtmp))
            Sprintf(eos(buf), " of %s", MGIVENNAME(mtmp));
    } else if (mtmp->isshk) {
        const char *shknm = shkname(mtmp),
                   *honorific = shkname_is_pname(mtmp) ? ""
                                   : mtmp->female ? "Ms. " : "Mr. ";

        Sprintf(eos(buf), "%s%s, the shopkeeper", honorific, shknm);
        g.killer.format = KILLED_BY;
    } else if (mtmp->ispriest || mtmp->isminion) {
        /* m_monnam() suppresses "the" prefix plus "invisible", and
           it overrides the effect of Hallucination on priestname() */
        Strcat(buf, m_monnam(mtmp));
    } else {
        Strcat(buf, pmname(mptr, Mgender(mtmp)));
        if (has_mgivenname(mtmp))
            Sprintf(eos(buf), " called %s", MGIVENNAME(mtmp));
    }

    Strcpy(g.killer.name, buf);

    /* might need to fix up multi_reason if 'mtmp' caused the reason */
    if (g.multi_reason
        && g.multi_reason > g.multireasonbuf
        && g.multi_reason < g.multireasonbuf + sizeof g.multireasonbuf - 1) {
        char reasondummy, *p;
        unsigned reasonmid = 0;

        /*
         * multireasonbuf[] contains 'm_id:reason' and multi_reason
         * points at the text past the colon, so we have something
         * like "42:paralyzed by a ghoul"; if mtmp->m_id matches 42
         * then we truncate 'reason' at its first space so that final
         * death reason becomes "Killed by a ghoul, while paralyzed."
         * instead of "Killed by a ghoul, while paralyzed by a ghoul."
         * (3.6.x gave "Killed by a ghoul, while paralyzed by a monster."
         * which is potenitally misleading when the monster is also
         * the killer.)
         *
         * Note that if the hero is life-saved and then killed again
         * before the helplessness has cleared, the second death will
         * report the truncated helplessness reason even if some other
         * monster peforms the /coup de grace/.
         */
        if (sscanf(g.multireasonbuf, "%u:%c", &reasonmid, &reasondummy) == 2
            && mtmp->m_id == reasonmid) {
            if ((p = index(g.multireasonbuf, ' ')) != 0)
                *p = '\0';
        }
    }

    /*
     * Chicken and egg issue:
     *  Ordinarily Unchanging ought to override something like this,
     *  but the transformation occurs at death.  With the current code,
     *  the effectiveness of Unchanging stops first, but a case could
     *  be made that it should last long enough to prevent undead
     *  transformation.  (Turning to slime isn't an issue here because
     *  Unchanging prevents that from happening.)
     */
    if (mptr->mlet == S_WRAITH)
        u.ugrave_arise = PM_WRAITH;
    else if (mptr->mlet == S_MUMMY && g.urace.mummynum != NON_PM)
        u.ugrave_arise = g.urace.mummynum;
    else if (zombie_maker(mtmp) && zombie_form(g.youmonst.data) != NON_PM)
        u.ugrave_arise = zombie_form(g.youmonst.data);
    else if (mptr->mlet == S_VAMPIRE && Race_if(PM_HUMAN))
        u.ugrave_arise = PM_VAMPIRE;
    else if (mptr == &mons[PM_GHOUL])
        u.ugrave_arise = PM_GHOUL;
    /* this could happen if a high-end vampire kills the hero
       when ordinary vampires are genocided; ditto for wraiths */
    if (u.ugrave_arise >= LOW_PM
        && (g.mvitals[u.ugrave_arise].mvflags & G_GENOD))
        u.ugrave_arise = NON_PM;

    done(how);
    return;
}

RESTORE_WARNING_FORMAT_NONLITERAL

/* some special cases for overriding while-helpless reason */
static const struct {
    int why, unmulti;
    const char *exclude, *include;
} death_fixups[] = {
    /* "petrified by <foo>, while getting stoned" -- "while getting stoned"
       prevented any last-second recovery, but it was not the cause of
       "petrified by <foo>" */
    { STONING, 1, "getting stoned", (char *) 0 },
    /* "died of starvation, while fainted from lack of food" is accurate
       but sounds a fairly silly (and doesn't actually appear unless you
       splice together death and while-helpless from xlogfile) */
    { STARVING, 0, "fainted from lack of food", "fainted" },
};

/* clear away while-helpless when the cause of death caused that
   helplessness (ie, "petrified by <foo> while getting stoned") */
static void
fixup_death(int how)
{
    int i;

    if (g.multi_reason) {
        for (i = 0; i < SIZE(death_fixups); ++i)
            if (death_fixups[i].why == how
                && !strcmp(death_fixups[i].exclude, g.multi_reason)) {
                if (death_fixups[i].include) /* substitute alternate reason */
                    g.multi_reason = death_fixups[i].include;
                else /* remove the helplessness reason */
                    g.multi_reason = (char *) 0;
                g.multireasonbuf[0] = '\0'; /* dynamic buf stale either way */
                if (death_fixups[i].unmulti) /* possibly hide helplessness */
                    g.multi = 0L;
                break;
            }
    }
}

#if defined(WIN32) && !defined(SYSCF)
#define NOTIFY_NETHACK_BUGS
#endif

DISABLE_WARNING_FORMAT_NONLITERAL

/*VARARGS1*/
void
panic VA_DECL(const char *, str)
{
    VA_START(str);
    VA_INIT(str, char *);

    if (g.program_state.panicking++)
        NH_abort(); /* avoid loops - this should never happen*/

    if (iflags.window_inited) {
        raw_print("\r\nOops...");
        wait_synch(); /* make sure all pending output gets flushed */
        exit_nhwindows((char *) 0);
        iflags.window_inited = 0; /* they're gone; force raw_print()ing */
    }

    raw_print(g.program_state.gameover
                  ? "Postgame wrapup disrupted."
                  : !g.program_state.something_worth_saving
                        ? "Program initialization has failed."
                        : "Suddenly, the dungeon collapses.");
#ifndef MICRO
#ifdef NOTIFY_NETHACK_BUGS
    if (!wizard)
        raw_printf("Report the following error to \"%s\" or at \"%s\".",
                   DEVTEAM_EMAIL, DEVTEAM_URL);
    else if (g.program_state.something_worth_saving)
        raw_print("\nError save file being written.\n");
#else /* !NOTIFY_NETHACK_BUGS */
    if (!wizard) {
        const char *maybe_rebuild = !g.program_state.something_worth_saving
                                     ? "."
                                     : "\nand it may be possible to rebuild.";

        if (sysopt.support)
            raw_printf("To report this error, %s%s", sysopt.support,
                       maybe_rebuild);
        else if (sysopt.fmtd_wizard_list) /* formatted SYSCF WIZARDS */
            raw_printf("To report this error, contact %s%s",
                       sysopt.fmtd_wizard_list, maybe_rebuild);
        else
            raw_printf("Report error to \"%s\"%s", WIZARD_NAME,
                       maybe_rebuild);
    }
#endif /* ?NOTIFY_NETHACK_BUGS */
    /* XXX can we move this above the prints?  Then we'd be able to
     * suppress "it may be possible to rebuild" based on dosave0()
     * or say it's NOT possible to rebuild. */
    if (g.program_state.something_worth_saving && !iflags.debug_fuzzer) {
        set_error_savefile();
        if (dosave0()) {
            /* os/win port specific recover instructions */
            if (sysopt.recover)
                raw_printf("%s", sysopt.recover);
        }
    }
#endif /* !MICRO */
    {
        char buf[BUFSZ];

#if !defined(NO_VSNPRINTF)
        (void) vsnprintf(buf, sizeof buf, str, VA_ARGS);
#else
        Vsprintf(buf, str, VA_ARGS);
#endif
        raw_print(buf);
        paniclog("panic", buf);
    }
#ifdef WIN32
    interject(INTERJECT_PANIC);
#endif
#if defined(UNIX) || defined(VMS) || defined(LATTICE) || defined(WIN32)
    if (wizard)
        NH_abort(); /* generate core dump */
#endif
    VA_END();
    really_done(PANICKED);
}

RESTORE_WARNING_FORMAT_NONLITERAL

static boolean
should_query_disclose_option(int category, char *defquery)
{
    int idx;
    char disclose, *dop;

    *defquery = 'n';
    if ((dop = index(disclosure_options, category)) != 0) {
        idx = (int) (dop - disclosure_options);
        if (idx < 0 || idx >= NUM_DISCLOSURE_OPTIONS) {
            impossible(
                   "should_query_disclose_option: bad disclosure index %d %c",
                       idx, category);
            *defquery = DISCLOSE_PROMPT_DEFAULT_YES;
            return TRUE;
        }
        disclose = flags.end_disclose[idx];
        if (disclose == DISCLOSE_YES_WITHOUT_PROMPT) {
            *defquery = 'y';
            return FALSE;
        } else if (disclose == DISCLOSE_SPECIAL_WITHOUT_PROMPT) {
            *defquery = 'a';
            return FALSE;
        } else if (disclose == DISCLOSE_NO_WITHOUT_PROMPT) {
            *defquery = 'n';
            return FALSE;
        } else if (disclose == DISCLOSE_PROMPT_DEFAULT_YES) {
            *defquery = 'y';
            return TRUE;
        } else if (disclose == DISCLOSE_PROMPT_DEFAULT_SPECIAL) {
            *defquery = 'a';
            return TRUE;
        } else {
            *defquery = 'n';
            return TRUE;
        }
    }
    impossible("should_query_disclose_option: bad category %c", category);
    return TRUE;
}

#ifdef DUMPLOG
static void
dump_plines(void)
{
    int i, j;
    char buf[BUFSZ], **strp;

    Strcpy(buf, " "); /* one space for indentation */
    putstr(0, 0, "Latest messages:");
    for (i = 0, j = (int) g.saved_pline_index; i < DUMPLOG_MSG_COUNT;
         ++i, j = (j + 1) % DUMPLOG_MSG_COUNT) {
        strp = &g.saved_plines[j];
        if (*strp) {
            copynchars(&buf[1], *strp, BUFSZ - 1 - 1);
            putstr(0, 0, buf);
#ifdef FREE_ALL_MEMORY
            free(*strp), *strp = 0;
#endif
        }
    }
}
#endif

/*ARGSUSED*/
static void
dump_everything(
    int how,     /* ASCENDED, ESCAPED, QUIT, etc */
    time_t when) /* date+time at end of game */
{
#ifdef DUMPLOG
    char pbuf[BUFSZ], datetimebuf[24]; /* [24]: room for 64-bit bogus value */

    dump_redirect(TRUE);
    if (!iflags.in_dumplog)
        return;

    init_symbols(); /* revert to default symbol set */

    /* one line version ID, which includes build date+time;
       it's conceivable that the game started with a different
       build date+time or even with an older nethack version,
       but we only have access to the one it finished under */
    putstr(0, 0, getversionstring(pbuf, sizeof pbuf));
    putstr(0, 0, "");

    /* game start and end date+time to disambiguate version date+time */
    Strcpy(datetimebuf, yyyymmddhhmmss(ubirthday));
    Sprintf(pbuf, "Game began %4.4s-%2.2s-%2.2s %2.2s:%2.2s:%2.2s",
            &datetimebuf[0], &datetimebuf[4], &datetimebuf[6],
            &datetimebuf[8], &datetimebuf[10], &datetimebuf[12]);
    Strcpy(datetimebuf, yyyymmddhhmmss(when));
    Sprintf(eos(pbuf), ", ended %4.4s-%2.2s-%2.2s %2.2s:%2.2s:%2.2s.",
            &datetimebuf[0], &datetimebuf[4], &datetimebuf[6],
            &datetimebuf[8], &datetimebuf[10], &datetimebuf[12]);
    putstr(0, 0, pbuf);
    putstr(0, 0, "");

    /* character name and basic role info */
    Sprintf(pbuf, "%s, %s %s %s %s",
            g.plname, aligns[1 - u.ualign.type].adj,
            genders[flags.female].adj, g.urace.adj,
            (flags.female && g.urole.name.f) ? g.urole.name.f
                                             : g.urole.name.m);
    putstr(0, 0, pbuf);
    putstr(0, 0, "");

    /* info about current game state */
    dump_map();
    putstr(0, 0, do_statusline1());
    putstr(0, 0, do_statusline2());
    putstr(0, 0, "");

    dump_plines();
    putstr(0, 0, "");
    putstr(0, 0, "Inventory:");
    (void) display_inventory((char *) 0, TRUE);
    container_contents(g.invent, TRUE, TRUE, FALSE);
    enlightenment((BASICENLIGHTENMENT | MAGICENLIGHTENMENT),
                  (how >= PANICKED) ? ENL_GAMEOVERALIVE : ENL_GAMEOVERDEAD);
    putstr(0, 0, "");

    /* overview of the game up to this point */
    show_gamelog((how >= PANICKED) ? ENL_GAMEOVERALIVE : ENL_GAMEOVERDEAD);
    putstr(0, 0, "");
    list_vanquished('d', FALSE); /* 'd' => 'y' */
    putstr(0, 0, "");
    list_genocided('d', FALSE); /* 'd' => 'y' */
    putstr(0, 0, "");
    show_conduct((how >= PANICKED) ? 1 : 2);
    putstr(0, 0, "");
    show_overview((how >= PANICKED) ? 1 : 2, how);
    putstr(0, 0, "");
    dump_redirect(FALSE);
#else
    nhUse(how);
    nhUse(when);
#endif
}

static void
disclose(int how, boolean taken)
{
    char c = '\0', defquery;
    char qbuf[QBUFSZ];
    boolean ask = FALSE;

    if (g.invent && !done_stopprint) {
        if (taken)
            Sprintf(qbuf, "Do you want to see what you had when you %s?",
                    (how == QUIT) ? "quit" : "died");
        else
            Strcpy(qbuf, "Do you want your possessions identified?");

        ask = should_query_disclose_option('i', &defquery);
        c = ask ? yn_function(qbuf, ynqchars, defquery, TRUE) : defquery;
        if (c == 'y') {
            /* caller has already ID'd everything */
            (void) display_inventory((char *) 0, FALSE);
            container_contents(g.invent, TRUE, TRUE, FALSE);
        }
        if (c == 'q')
            done_stopprint++;
    }

    if (!done_stopprint) {
        ask = should_query_disclose_option('a', &defquery);
        c = ask ? yn_function("Do you want to see your attributes?", ynqchars,
                              defquery, TRUE)
                : defquery;
        if (c == 'y')
            enlightenment((BASICENLIGHTENMENT | MAGICENLIGHTENMENT),
                          (how >= PANICKED) ? ENL_GAMEOVERALIVE
                                            : ENL_GAMEOVERDEAD);
        if (c == 'q')
            done_stopprint++;
    }

    if (!done_stopprint) {
        ask = should_query_disclose_option('v', &defquery);
        list_vanquished(defquery, ask);
    }

    if (!done_stopprint) {
        ask = should_query_disclose_option('g', &defquery);
        list_genocided(defquery, ask);
    }

    if (!done_stopprint) {
        if (should_query_disclose_option('c', &defquery)) {
            int acnt = count_achievements();

            Sprintf(qbuf, "Do you want to see your conduct%s?",
                    /* this was distinguishing between one achievement and
                       multiple achievements, but "conduct and achievement"
                       looked strange if multiple conducts got shown (which
                       is usual for an early game death); we could switch
                       to plural vs singular for conducts but the less
                       specific "conduct and achievements" is sufficient */
                    (acnt > 0) ? " and achievements" : "");
            c = yn_function(qbuf, ynqchars, defquery, TRUE);
        } else {
            c = defquery;
        }
        if (c == 'y')
            show_conduct((how >= PANICKED) ? 1 : 2);
        if (c == 'q')
            done_stopprint++;
    }

    if (!done_stopprint) {
        ask = should_query_disclose_option('o', &defquery);
        c = ask ? yn_function("Do you want to see the dungeon overview?",
                              ynqchars, defquery, TRUE)
                : defquery;
        if (c == 'y')
            show_overview((how >= PANICKED) ? 1 : 2, how);
        if (c == 'q')
            done_stopprint++;
    }
}

/* try to get the player back in a viable state after being killed */
static void
savelife(int how)
{
    int uhpmin;
    int givehp = 50 + 10 * (ACURR(A_CON) / 2);

    /* life-drain/level-loss to experience level 0 kills without actually
       reducing ulevel below 1, but include this for bulletproofing */
    if (u.ulevel < 1)
        u.ulevel = 1;
    uhpmin = minuhpmax(10);
    if (u.uhpmax < uhpmin)
        setuhpmax(uhpmin);
    u.uhp = min(u.uhpmax, givehp);
    if (Upolyd) /* Unchanging, or death which bypasses losing hit points */
        u.mh = min(u.mhmax, givehp);
    if (u.uhunger < 500 || how == CHOKING) {
        init_uhunger();
    }
    /* cure impending doom of sickness hero won't have time to fix
       [shouldn't this also be applied to other fatal timeouts?] */
    if ((Sick & TIMEOUT) == 1L) {
        make_sick(0L, (char *) 0, FALSE, SICK_ALL);
    }
    g.nomovemsg = "You survived that attempt on your life.";
    g.context.move = 0;

    g.multi = -1; /* can't move again during the current turn */
    /* in case being life-saved is immediately followed by being killed
       again (perhaps due to zap rebound); this text will be appended to
          "killed by <something>, while "
       in high scores entry, if any, and in logfile (but not on tombstone) */
    g.multi_reason = Role_if(PM_TOURIST) ? "being toyed with by Fate"
                                         : "attempting to cheat Death";

    if (u.utrap && u.utraptype == TT_LAVA)
        reset_utrap(FALSE);
    g.context.botl = TRUE;
    u.ugrave_arise = NON_PM;
    HUnchanging = 0L;
    curs_on_u();
    if (!g.context.mon_moving)
        endmultishot(FALSE);
    if (u.uswallow) {
        /* might drop hero onto a trap that kills her all over again */
        expels(u.ustuck, u.ustuck->data, TRUE);
    } else if (u.ustuck) {
        if (Upolyd && sticks(g.youmonst.data))
            You("release %s.", mon_nam(u.ustuck));
        else
            pline("%s releases you.", Monnam(u.ustuck));
        unstuck(u.ustuck);
    }
}

/*
 * Get valuables from the given list.  Revised code: the list always remains
 * intact.
 */
static void
get_valuables(struct obj *list) /* inventory or container contents */
{
    register struct obj *obj;
    register int i;

    /* find amulets and gems, ignoring all artifacts */
    for (obj = list; obj; obj = obj->nobj)
        if (Has_contents(obj)) {
            get_valuables(obj->cobj);
        } else if (obj->oartifact) {
            continue;
        } else if (obj->oclass == AMULET_CLASS) {
            i = obj->otyp - FIRST_AMULET;
            if (!g.amulets[i].count) {
                g.amulets[i].count = obj->quan;
                g.amulets[i].typ = obj->otyp;
            } else
                g.amulets[i].count += obj->quan; /* always adds one */
        } else if (obj->oclass == GEM_CLASS && obj->otyp < LUCKSTONE) {
            i = min(obj->otyp, LAST_GEM + 1) - FIRST_GEM;
            if (!g.gems[i].count) {
                g.gems[i].count = obj->quan;
                g.gems[i].typ = obj->otyp;
            } else
                g.gems[i].count += obj->quan;
        }
    return;
}

/*
 *  Sort collected valuables, most frequent to least.  We could just
 *  as easily use qsort, but we don't care about efficiency here.
 */
static void
sort_valuables(struct valuable_data list[],
               int size) /* max value is less than 20 */
{
    register int i, j;
    struct valuable_data ltmp;

    /* move greater quantities to the front of the list */
    for (i = 1; i < size; i++) {
        if (list[i].count == 0)
            continue;   /* empty slot */
        ltmp = list[i]; /* structure copy */
        for (j = i; j > 0; --j)
            if (list[j - 1].count >= ltmp.count)
                break;
            else {
                list[j] = list[j - 1];
            }
        list[j] = ltmp;
    }
    return;
}

#if 0
/*
 * odds_and_ends() was used for 3.6.0 and 3.6.1.
 * Schroedinger's Cat is handled differently as of 3.6.2.
 */
static boolean odds_and_ends(struct obj *, int);

#define CAT_CHECK 2

static boolean
odds_and_ends(struct obj *list, int what)
{
    struct obj *otmp;

    for (otmp = list; otmp; otmp = otmp->nobj) {
        switch (what) {
        case CAT_CHECK: /* Schroedinger's Cat */
            /* Ascending is deterministic */
            if (SchroedingersBox(otmp))
                return rn2(2);
            break;
        }
        if (Has_contents(otmp))
            return odds_and_ends(otmp->cobj, what);
    }
    return FALSE;
}
#endif

/* deal with some objects which may be in an abnormal state at end of game */
void
done_object_cleanup(void)
{
    int ox, oy;

    /* might have been killed while using a disposable item, so make sure
       it's gone prior to inventory disclosure and creation of bones */
    inven_inuse(TRUE);
    /*
     * Hero can die when throwing an object (by hitting an adjacent
     * gas spore, for instance, or being hit by mis-returning Mjollnir),
     * or while in transit (from falling down stairs).  If that happens,
     * some object(s) might be in limbo rather than on the map or in
     * any inventory.  Saving bones with an active light source in limbo
     * would trigger an 'object not local' panic.
     *
     * We used to use dealloc_obj() on g.thrownobj and g.kickedobj but
     * that keeps them out of bones and could leave uball in a confused
     * state (gone but still attached).  Place them on the map but
     * bypass flooreffects().  That could lead to minor anomalies in
     * bones, like undamaged paper at water or lava locations or piles
     * not being knocked down holes, but it seems better to get this
     * game over with than risk being tangled up in more and more details.
     */
    ox = u.ux + u.dx, oy = u.uy + u.dy;
    if (!isok(ox, oy) || !accessible(ox, oy))
        ox = u.ux, oy = u.uy;
    /* put thrown or kicked object on map (for bones); location might
       be incorrect (perhaps killed by divine lightning when throwing at
       a temple priest?) but this should be better than just vanishing
       (fragile stuff should be taken care of before getting here) */
    if (g.thrownobj && g.thrownobj->where == OBJ_FREE) {
        place_object(g.thrownobj, ox, oy);
        stackobj(g.thrownobj), g.thrownobj = 0;
    }
    if (g.kickedobj && g.kickedobj->where == OBJ_FREE) {
        place_object(g.kickedobj, ox, oy);
        stackobj(g.kickedobj), g.kickedobj = 0;
    }
    /* if Punished hero dies during level change or dies or quits while
       swallowed, uball and uchain will be in limbo; put them on floor
       so bones will have them and object list cleanup finds them */
    if (uchain && uchain->where == OBJ_FREE) {
        /* placebc(); */
        lift_covet_and_placebc(override_restriction);
    }
    /* persistent inventory window now obsolete since disclosure uses
       a normal popup one; avoids "Bad fruit #n" when saving bones */
    if (iflags.perm_invent) {
        iflags.perm_invent = FALSE;
        update_inventory(); /* make interface notice the change */
    }
    return;
}

/* called twice; first to calculate total, then to list relevant items */
static void
artifact_score(
    struct obj *list,
    boolean counting, /* true => add up points; false => display them */
    winid endwin)
{
    char pbuf[BUFSZ];
    struct obj *otmp;
    long value, points;

    for (otmp = list; otmp; otmp = otmp->nobj) {
        if (otmp->oartifact || otmp->otyp == BELL_OF_OPENING
            || otmp->otyp == SPE_BOOK_OF_THE_DEAD
            || otmp->otyp == CANDELABRUM_OF_INVOCATION) {
            value = arti_cost(otmp); /* zorkmid value */
            points = value * 5 / 2;  /* score value */
            if (counting) {
                nowrap_add(u.urexp, points);
            } else {
                discover_object(otmp->otyp, TRUE, FALSE);
                otmp->known = otmp->dknown = otmp->bknown = otmp->rknown = 1;
                /* assumes artifacts don't have quan > 1 */
                Sprintf(pbuf, "%s%s (worth %ld %s and %ld points)",
                        the_unique_obj(otmp) ? "The " : "",
                        otmp->oartifact ? artiname(otmp->oartifact)
                                        : OBJ_NAME(objects[otmp->otyp]),
                        value, currency(value), points);
                putstr(endwin, 0, pbuf);
            }
        }
        if (Has_contents(otmp))
            artifact_score(otmp->cobj, counting, endwin);
    }
}

/* Be careful not to call panic from here! */
void
done(int how)
{
    boolean survive = FALSE;

    if (how == TRICKED) {
        if (g.killer.name[0]) {
            paniclog("trickery", g.killer.name);
            g.killer.name[0] = '\0';
        }
        if (wizard) {
            You("are a very tricky wizard, it seems.");
            g.killer.format = KILLED_BY_AN; /* reset to 0 */
            return;
        }
    }
    if (g.program_state.panicking
#ifdef HANGUPHANDLING
        || g.program_state.done_hup
#endif
        || (how == QUIT && done_stopprint)) {
        /* skip status update if panicking or disconnected
           or answer of 'q' to "Really quit?" */
        g.context.botl = g.context.botlx = iflags.time_botl = FALSE;
    } else {
        /* otherwise force full status update */
        g.context.botlx = TRUE;
        bot();
    }

    if (iflags.debug_fuzzer) {
        if (!(g.program_state.panicking || how == PANICKED)) {
            savelife(how);
            /* periodically restore characteristics and lost exp levels
               or cure lycanthropy */
            if (!rn2(10)) {
                struct obj *potion = mksobj((u.ulycn > LOW_PM && !rn2(3))
                                            ? POT_WATER : POT_RESTORE_ABILITY,
                                            TRUE, FALSE);

                bless(potion);
                (void) peffects(potion); /* always -1 for restore ability */
                /* not useup(); we haven't put this potion into inventory */
                obfree(potion, (struct obj *) 0);
            }
            g.killer.name[0] = '\0';
            g.killer.format = 0;
            return;
        }
    } else
    if (how == ASCENDED || (!g.killer.name[0] && how == GENOCIDED))
        g.killer.format = NO_KILLER_PREFIX;
    /* Avoid killed by "a" burning or "a" starvation */
    if (!g.killer.name[0] && (how == STARVING || how == BURNING))
        g.killer.format = KILLED_BY;
    if (!g.killer.name[0] || how >= PANICKED)
        Strcpy(g.killer.name, deaths[how]);

    if (how < PANICKED) {
        u.umortality++;
        /* in case caller hasn't already done this */
        if (u.uhp != 0 || (Upolyd && u.mh != 0)) {
            /* force HP to zero in case it is still positive (some
               deaths aren't triggered by loss of hit points), or
               negative (-1 is used as a flag in some circumstances
               which don't apply when actually dying due to HP loss) */
            u.uhp = u.mh = 0;
            g.context.botl = 1;
        }
    }
    if (Lifesaved && (how <= GENOCIDED)) {
        pline("But wait...");
        makeknown(AMULET_OF_LIFE_SAVING);
        Your("medallion %s!", !Blind ? "begins to glow" : "feels warm");
        if (how == CHOKING)
            You("vomit ...");
        You_feel("much better!");
        pline_The("medallion crumbles to dust!");
        if (uamul)
            useup(uamul);

        (void) adjattrib(A_CON, -1, TRUE);
        savelife(how);
        if (how == GENOCIDED) {
            pline("Unfortunately you are still genocided...");
        } else {
            char killbuf[BUFSZ];
            formatkiller(killbuf, BUFSZ, how, FALSE);
            livelog_printf(LL_LIFESAVE, "averted death (%s)", killbuf);
            survive = TRUE;
        }
    }
    /* explore and wizard modes offer player the option to keep playing */
    if (!survive && (wizard || discover) && how <= GENOCIDED
        && !paranoid_query(ParanoidDie, "Die?")) {
        pline("OK, so you don't %s.", (how == CHOKING) ? "choke" : "die");
        iflags.last_msg = PLNMSG_OK_DONT_DIE;
        savelife(how);
        survive = TRUE;
    }

    if (survive) {
        g.killer.name[0] = '\0';
        g.killer.format = KILLED_BY_AN; /* reset to 0 */
        return;
    }
    really_done(how);
    /*NOTREACHED*/
}

/* separated from done() in order to specify the __noreturn__ attribute */
static void
really_done(int how)
{
    boolean taken;
    char pbuf[BUFSZ];
    winid endwin = WIN_ERR;
    boolean bones_ok, have_windows = iflags.window_inited;
    struct obj *corpse = (struct obj *) 0;
    time_t endtime;
    long umoney;
    long tmp;

    /*
     *  The game is now over...
     */
    g.program_state.gameover = 1;
    /* in case of a subsequent panic(), there's no point trying to save */
    g.program_state.something_worth_saving = 0;
#ifdef HANGUPHANDLING
    if (g.program_state.done_hup)
        done_stopprint++;
#endif
    /* render vision subsystem inoperative */
    iflags.vision_inited = 0;

    /* maybe use up active invent item(s), place thrown/kicked missile,
       deal with ball and chain possibly being temporarily off the map */
    if (!g.program_state.panicking)
        done_object_cleanup();
    /* in case we're panicking; normally cleared by done_object_cleanup() */
    iflags.perm_invent = FALSE;

    /* remember time of death here instead of having bones, rip, and
       topten figure it out separately and possibly getting different
       time or even day if player is slow responding to --More-- */
    urealtime.finish_time = endtime = getnow();
    urealtime.realtime += timet_delta(endtime, urealtime.start_timing);
    /* collect these for end of game disclosure (not used during play) */
    iflags.at_night = night();
    iflags.at_midnight = midnight();

    /* final achievement tracking; only show blind and nudist if some
       tangible progress has been made; always show ascension last */
    if (u.uachieved[0] || !flags.beginner) {
        if (u.uroleplay.blind)
            record_achievement(ACH_BLND); /* blind the whole game */
        if (u.uroleplay.nudist)
            record_achievement(ACH_NUDE); /* never wore armor */
    }
    if (how == ASCENDED)
        record_achievement(ACH_UWIN);

    dump_open_log(endtime);
    /* Sometimes you die on the first move.  Life's not fair.
     * On those rare occasions you get hosed immediately, go out
     * smiling... :-)  -3.
     */
    if (g.moves <= 1 && how < PANICKED && !done_stopprint)
        pline("Do not pass Go.  Do not collect 200 %s.", currency(200L));

    if (have_windows)
        wait_synch(); /* flush screen output */
#ifndef NO_SIGNAL
    (void) signal(SIGINT, (SIG_RET_TYPE) done_intr);
#if defined(UNIX) || defined(VMS) || defined(__EMX__)
    (void) signal(SIGQUIT, (SIG_RET_TYPE) done_intr);
    sethanguphandler(done_hangup);
#endif
#endif /* NO_SIGNAL */

    bones_ok = (how < GENOCIDED) && can_make_bones();

    if (bones_ok && launch_in_progress())
        force_launch_placement();

    /* maintain ugrave_arise even for !bones_ok */
    if (how == PANICKED)
        u.ugrave_arise = (NON_PM - 3); /* no corpse, no grave */
    else if (how == BURNING || how == DISSOLVED) /* corpse burns up too */
        u.ugrave_arise = (NON_PM - 2); /* leave no corpse */
    else if (how == STONING)
        u.ugrave_arise = (NON_PM - 1); /* statue instead of corpse */
    else if (how == TURNED_SLIME
             /* it's possible to turn into slime even though green slimes
                have been genocided:  genocide could occur after hero is
                already infected or hero could eat a glob of one created
                before genocide; don't try to arise as one if they're gone */
             && !(g.mvitals[PM_GREEN_SLIME].mvflags & G_GENOD))
        u.ugrave_arise = PM_GREEN_SLIME;

    if (how == QUIT) {
        g.killer.format = NO_KILLER_PREFIX;
        if (u.uhp < 1) {
            how = DIED;
            u.umortality++; /* skipped above when how==QUIT */
            Strcpy(g.killer.name, "quit while already on Charon's boat");
        }
    }
    if (how == ESCAPED || how == PANICKED)
        g.killer.format = NO_KILLER_PREFIX;

    fixup_death(how); /* actually, fixup g.multi_reason */

    if (how != PANICKED) {
        boolean silently = done_stopprint ? TRUE : FALSE;

        /* these affect score and/or bones, but avoid them during panic */
        taken = paybill((how == ESCAPED) ? -1 : (how != QUIT), silently);
        paygd(silently);
        clearpriests();
    } else
        taken = FALSE; /* lint; assert( !bones_ok ); */

    clearlocks();

    if (have_windows)
        display_nhwindow(WIN_MESSAGE, FALSE);

    if (how != PANICKED) {
        struct obj *obj;

        /*
         * This is needed for both inventory disclosure and dumplog.
         * Both are optional, so do it once here instead of duplicating
         * it in both of those places.
         */
        for (obj = g.invent; obj; obj = obj->nobj) {
            discover_object(obj->otyp, TRUE, FALSE);
            obj->known = obj->bknown = obj->dknown = obj->rknown = 1;
            set_cknown_lknown(obj); /* set flags when applicable */
            /* we resolve Schroedinger's cat now in case of both
               disclosure and dumplog, where the 50:50 chance for
               live cat has to be the same both times */
            if (SchroedingersBox(obj)) {
                if (!Schroedingers_cat) {
                    /* tell observe_quantum_cat() not to create a cat; if it
                       chooses live cat in this situation, it will leave the
                       SchroedingersBox flag set (for container_contents()) */
                    observe_quantum_cat(obj, FALSE, FALSE);
                    if (SchroedingersBox(obj))
                        Schroedingers_cat = TRUE;
                } else
                    obj->spe = 0; /* ordinary box with cat corpse in it */
            }
        }

        if (strcmp(flags.end_disclose, "none"))
            disclose(how, taken);

        /* it would be better to do this after killer.name fixups but
           that comes too late; included in final dumplog but might be
           excluded by active livelog */
        formatkiller(pbuf, (unsigned) sizeof pbuf, how, TRUE);
        if (!*pbuf)
            Strcpy(pbuf, deaths[how]);
        livelog_printf(LL_DUMP, "%s", pbuf);

        dump_everything(how, endtime);
    }

    /* if pets will contribute to score, populate g.mydogs list now
       (bones creation isn't a factor, but pline() messaging is; used to
       be done even sooner, but we need it to come after dump_everything()
       so that any accompanying pets are still on the map during dump) */
    if (how == ESCAPED || how == ASCENDED)
        keepdogs(TRUE);

    /* finish_paybill should be called after disclosure but before bones */
    if (bones_ok && taken)
        finish_paybill();

    /* grave creation should be after disclosure so it doesn't have
       this grave in the current level's features for #overview */
    if (bones_ok && u.ugrave_arise == NON_PM
        && !(g.mvitals[u.umonnum].mvflags & G_NOCORPSE)) {
        /* Base corpse on race when not poly'd since original u.umonnum
           is based on role, and all role monsters are human. */
        int mnum = !Upolyd ? g.urace.mnum : u.umonnum;

        corpse = mk_named_object(CORPSE, &mons[mnum], u.ux, u.uy, g.plname);
        Sprintf(pbuf, "%s, ", g.plname);
        formatkiller(eos(pbuf), sizeof pbuf - Strlen(pbuf), how, TRUE);
        make_grave(u.ux, u.uy, pbuf);
    }
    pbuf[0] = '\0'; /* clear grave text; also lint suppression */

    /* calculate score, before creating bones [container gold] */
    {
        int deepest = deepest_lev_reached(FALSE);

        umoney = money_cnt(g.invent);
        tmp = u.umoney0;
        umoney += hidden_gold(TRUE); /* accumulate gold from containers */
        tmp = umoney - tmp;          /* net gain */

        if (tmp < 0L)
            tmp = 0L;
        if (how < PANICKED)
            tmp -= tmp / 10L;
        tmp += 50L * (long) (deepest - 1);
        if (deepest > 20)
            tmp += 1000L * (long) ((deepest > 30) ? 10 : deepest - 20);
        nowrap_add(u.urexp, tmp);

        /* ascension gives a score bonus iff offering to original deity */
        if (how == ASCENDED && u.ualign.type == u.ualignbase[A_ORIGINAL]) {
            /* retaining original alignment: score *= 2;
               converting, then using helm-of-OA to switch back: *= 1.5 */
            tmp = (u.ualignbase[A_CURRENT] == u.ualignbase[A_ORIGINAL])
                      ? u.urexp
                      : (u.urexp / 2L);
            nowrap_add(u.urexp, tmp);
        }
    }

    if (u.ugrave_arise >= LOW_PM && !done_stopprint) {
        /* give this feedback even if bones aren't going to be created,
           so that its presence or absence doesn't tip off the player to
           new bones or their lack; it might be a lie if makemon fails */
        Your("%s as %s...",
             (u.ugrave_arise != PM_GREEN_SLIME)
                 ? "body rises from the dead"
                 : "revenant persists",
             an(pmname(&mons[u.ugrave_arise], Ugender)));
        display_nhwindow(WIN_MESSAGE, FALSE);
    }

    if (bones_ok) {
        if (!wizard || paranoid_query(ParanoidBones, "Save bones?"))
            savebones(how, endtime, corpse);
        /* corpse may be invalid pointer now so
            ensure that it isn't used again */
        corpse = (struct obj *) 0;
    }

    /* update gold for the rip output, which can't use hidden_gold()
       (containers will be gone by then if bones just got saved...) */
    g.done_money = umoney;

    /* clean up unneeded windows */
    if (have_windows) {
        wait_synch();
        free_pickinv_cache(); /* extra persistent window if perm_invent */
        if (WIN_INVEN != WIN_ERR) {
            destroy_nhwindow(WIN_INVEN),  WIN_INVEN = WIN_ERR;
            /* precaution in case any late update_inventory() calls occur */
            iflags.perm_invent = 0;
        }
        display_nhwindow(WIN_MESSAGE, TRUE);
        destroy_nhwindow(WIN_MAP),  WIN_MAP = WIN_ERR;
        if (WIN_STATUS != WIN_ERR)
            destroy_nhwindow(WIN_STATUS),  WIN_STATUS = WIN_ERR;
        destroy_nhwindow(WIN_MESSAGE),  WIN_MESSAGE = WIN_ERR;

        if (!done_stopprint || flags.tombstone)
            endwin = create_nhwindow(NHW_TEXT);

        if (how < GENOCIDED && flags.tombstone && endwin != WIN_ERR)
            outrip(endwin, how, endtime);
    } else
        done_stopprint = 1; /* just avoid any more output */

#ifdef DUMPLOG
    /* 'how' reasons beyond genocide shouldn't show tombstone;
       for normal end of game, genocide doesn't either */
    if (how <= GENOCIDED) {
        dump_redirect(TRUE);
        if (iflags.in_dumplog)
            genl_outrip(0, how, endtime);
        dump_redirect(FALSE);
    }
#endif
    if (u.uhave.amulet) {
        Strcat(g.killer.name, " (with the Amulet)");
    } else if (how == ESCAPED) {
        if (Is_astralevel(&u.uz)) /* offered Amulet to wrong deity */
            Strcat(g.killer.name, " (in celestial disgrace)");
        else if (carrying(FAKE_AMULET_OF_YENDOR))
            Strcat(g.killer.name, " (with a fake Amulet)");
        /* don't bother counting to see whether it should be plural */
    }

    Sprintf(pbuf, "%s %s the %s...", Goodbye(), g.plname,
            (how != ASCENDED)
                ? (const char *) ((flags.female && g.urole.name.f)
                    ? g.urole.name.f
                    : g.urole.name.m)
                : (const char *) (flags.female ? "Demigoddess" : "Demigod"));
    dump_forward_putstr(endwin, 0, pbuf, done_stopprint);
    dump_forward_putstr(endwin, 0, "", done_stopprint);

    if (how == ESCAPED || how == ASCENDED) {
        struct monst *mtmp;
        struct obj *otmp;
        register struct val_list *val;
        register int i;

        for (val = g.valuables; val->list; val++)
            for (i = 0; i < val->size; i++) {
                val->list[i].count = 0L;
            }
        get_valuables(g.invent);

        /* add points for collected valuables */
        for (val = g.valuables; val->list; val++)
            for (i = 0; i < val->size; i++)
                if (val->list[i].count != 0L) {
                    tmp = val->list[i].count
                          * (long) objects[val->list[i].typ].oc_cost;
                    nowrap_add(u.urexp, tmp);
                }

        /* count the points for artifacts */
        artifact_score(g.invent, TRUE, endwin);

        g.viz_array[0][0] |= IN_SIGHT; /* need visibility for naming */
        mtmp = g.mydogs;
        Strcpy(pbuf, "You");
        if (mtmp || Schroedingers_cat) {
            while (mtmp) {
                Sprintf(eos(pbuf), " and %s", mon_nam(mtmp));
                if (mtmp->mtame)
                    nowrap_add(u.urexp, mtmp->mhp);
                mtmp = mtmp->nmon;
            }
            /* [it might be more robust to create a housecat and add it to
               g.mydogs; it doesn't have to be placed on the map for that] */
            if (Schroedingers_cat) {
                int mhp, m_lev = adj_lev(&mons[PM_HOUSECAT]);

                mhp = d(m_lev, 8);
                nowrap_add(u.urexp, mhp);
                Strcat(eos(pbuf), " and Schroedinger's cat");
            }
            dump_forward_putstr(endwin, 0, pbuf, done_stopprint);
            pbuf[0] = '\0';
        } else {
            Strcat(pbuf, " ");
        }
        Sprintf(eos(pbuf), "%s with %ld point%s,",
                (how == ASCENDED) ? "went to your reward"
                                  : "escaped from the dungeon",
                u.urexp, plur(u.urexp));
        dump_forward_putstr(endwin, 0, pbuf, done_stopprint);

        if (!done_stopprint)
            artifact_score(g.invent, FALSE, endwin); /* list artifacts */
#ifdef DUMPLOG
        dump_redirect(TRUE);
        if (iflags.in_dumplog)
            artifact_score(g.invent, FALSE, 0);
        dump_redirect(FALSE);
#endif

        /* list valuables here */
        for (val = g.valuables; val->list; val++) {
            sort_valuables(val->list, val->size);
            for (i = 0; i < val->size && !done_stopprint; i++) {
                int typ = val->list[i].typ;
                long count = val->list[i].count;

                if (count == 0L)
                    continue;
                if (objects[typ].oc_class != GEM_CLASS || typ <= LAST_GEM) {
                    otmp = mksobj(typ, FALSE, FALSE);
                    discover_object(otmp->otyp, TRUE, FALSE);
                    otmp->known = 1;  /* for fake amulets */
                    otmp->dknown = 1; /* seen it (blindness fix) */
                    if (has_oname(otmp))
                        free_oname(otmp);
                    otmp->quan = count;
                    Sprintf(pbuf, "%8ld %s (worth %ld %s),", count,
                            xname(otmp), count * (long) objects[typ].oc_cost,
                            currency(2L));
                    obfree(otmp, (struct obj *) 0);
                } else {
                    Sprintf(pbuf, "%8ld worthless piece%s of colored glass,",
                            count, plur(count));
                }
                dump_forward_putstr(endwin, 0, pbuf, 0);
            }
        }

    } else {
        /* did not escape or ascend */
        if (u.uz.dnum == 0 && u.uz.dlevel <= 0) {
            /* level teleported out of the dungeon; `how' is DIED,
               due to falling or to "arriving at heaven prematurely" */
            Sprintf(pbuf, "You %s beyond the confines of the dungeon",
                    (u.uz.dlevel < 0) ? "passed away" : ends[how]);
        } else {
            /* more conventional demise */
            const char *where = g.dungeons[u.uz.dnum].dname;

            if (Is_astralevel(&u.uz))
                where = "The Astral Plane";
            Sprintf(pbuf, "You %s in %s", ends[how], where);
            if (!In_endgame(&u.uz) && !single_level_branch(&u.uz))
                Sprintf(eos(pbuf), " on dungeon level %d",
                        In_quest(&u.uz) ? dunlev(&u.uz) : depth(&u.uz));
        }

        Sprintf(eos(pbuf), " with %ld point%s,", u.urexp, plur(u.urexp));
        dump_forward_putstr(endwin, 0, pbuf, done_stopprint);
    }

    Sprintf(pbuf, "and %ld piece%s of gold, after %ld move%s.", umoney,
            plur(umoney), g.moves, plur(g.moves));
    dump_forward_putstr(endwin, 0, pbuf, done_stopprint);
    Sprintf(pbuf,
            "You were level %d with a maximum of %d hit point%s when you %s.",
            u.ulevel, u.uhpmax, plur(u.uhpmax), ends[how]);
    dump_forward_putstr(endwin, 0, pbuf, done_stopprint);
    dump_forward_putstr(endwin, 0, "", done_stopprint);
    if (!done_stopprint)
        display_nhwindow(endwin, TRUE);
    if (endwin != WIN_ERR)
        destroy_nhwindow(endwin);

    dump_close_log();
    /*
     * "So when I die, the first thing I will see in Heaven is a score list?"
     *
     * topten() updates 'logfile' and 'xlogfile', when they're enabled.
     * Then the current game's score is shown in its relative position
     * within high scores, and 'record' is updated if that makes the cut.
     *
     * FIXME!
     *  If writing topten with raw_print(), which will usually be sent to
     *  stdout, we call exit_nhwindows() first in case it erases the screen.
     *  But when writing topten to a window, we call exit_nhwindows()
     *  after topten() because that needs the windowing system to still
     *  be up.  This sequencing is absurd; we need something like
     *  raw_prompt("--More--") (or "Press <return> to continue.") that
     *  topten() can call for !toptenwin before returning here.
     */
    if (have_windows && !iflags.toptenwin)
        exit_nhwindows((char *) 0), have_windows = FALSE;
    topten(how, endtime);
    if (have_windows)
        exit_nhwindows((char *) 0);

    if (done_stopprint) {
        raw_print("");
        raw_print("");
    }
    nh_terminate(EXIT_SUCCESS);
}

/* used for disclosure and for the ':' choice when looting a container */
void
container_contents(
    struct obj *list,
    boolean identified,
    boolean all_containers,
    boolean reportempty)
{
    register struct obj *box, *obj;
    char buf[BUFSZ];
    boolean cat, dumping = iflags.in_dumplog;

    for (box = list; box; box = box->nobj) {
        if (Is_container(box) || box->otyp == STATUE) {
            if (!box->cknown || (identified && !box->lknown)) {
                box->cknown = 1; /* we're looking at the contents now */
                if (identified)
                    box->lknown = 1;
                update_inventory();
            }
            if (box->otyp == BAG_OF_TRICKS) {
                continue; /* wrong type of container */
            } else if (box->cobj) {
                winid tmpwin = create_nhwindow(NHW_MENU);
                Loot *sortedcobj, *srtc;
                unsigned sortflags;

                /* at this stage, the SchroedingerBox() flag is only set
                   if the cat inside the box is alive; the box actually
                   contains a cat corpse that we'll pretend is not there;
                   for dead cat, the flag will be clear and there'll be
                   a cat corpse inside the box; either way, inventory
                   reports the box as containing "1 item" */
                cat = SchroedingersBox(box);

                Sprintf(buf, "Contents of %s:", the(xname(box)));
                putstr(tmpwin, 0, buf);
                if (!dumping)
                    putstr(tmpwin, 0, "");
                buf[0] = buf[1] = ' '; /* two leading spaces */
                if (box->cobj && !cat) {
                    sortflags = (((flags.sortloot == 'l'
                                   || flags.sortloot == 'f')
                                     ? SORTLOOT_LOOT : 0)
                                 | (flags.sortpack ? SORTLOOT_PACK : 0));
                    sortedcobj = sortloot(&box->cobj, sortflags, FALSE,
                                          (boolean (*)(OBJ_P)) 0);
                    for (srtc = sortedcobj; ((obj = srtc->obj) != 0); ++srtc) {
                        if (identified) {
                            discover_object(obj->otyp, TRUE, FALSE);
                            obj->known = obj->bknown = obj->dknown
                                = obj->rknown = 1;
                            if (Is_container(obj) || obj->otyp == STATUE)
                                obj->cknown = obj->lknown = 1;
                        }
                        Strcpy(&buf[2], doname_with_price(obj));
                        putstr(tmpwin, 0, buf);
                    }
                    unsortloot(&sortedcobj);
                } else if (cat) {
                    Strcpy(&buf[2], "Schroedinger's cat!");
                    putstr(tmpwin, 0, buf);
                }
                if (dumping)
                    putstr(0, 0, "");
                display_nhwindow(tmpwin, TRUE);
                destroy_nhwindow(tmpwin);
                if (all_containers)
                    container_contents(box->cobj, identified, TRUE,
                                       reportempty);
            } else if (reportempty) {
                pline("%s is empty.", upstart(thesimpleoname(box)));
                display_nhwindow(WIN_MESSAGE, FALSE);
            }
        }
        if (!all_containers)
            break;
    }
}

/* should be called with either EXIT_SUCCESS or EXIT_FAILURE */
void
nh_terminate(int status)
{
    g.program_state.in_moveloop = 0; /* won't be returning to normal play */

    l_nhcore_call(NHCORE_GAME_EXIT);
#ifdef MAC
    getreturn("to exit");
#endif
    /* don't bother to try to release memory if we're in panic mode, to
       avoid trouble in case that happens to be due to memory problems */
    if (!g.program_state.panicking) {
        freedynamicdata();
        dlb_cleanup();
        l_nhcore_done();
    }

#ifdef VMS
    /*
     *  This is liable to draw a warning if compiled with gcc, but it's
     *  more important to flag panic() -> really_done() -> nh_terminate()
     *  as __noreturn__ then to avoid the warning.
     */
    /* don't call exit() if already executing within an exit handler;
       that would cancel any other pending user-mode handlers */
    if (g.program_state.exiting)
        return;
#endif
    g.program_state.exiting = 1;
    nethack_exit(status);
}

/* set a delayed killer, ensure non-delayed killer is cleared out */
void
delayed_killer(int id, int format, const char *killername)
{
    struct kinfo *k = find_delayed_killer(id);

    if (!k) {
        /* no match, add a new delayed killer to the list */
        k = (struct kinfo *) alloc(sizeof (struct kinfo));
        (void) memset((genericptr_t) k, 0, sizeof (struct kinfo));
        k->id = id;
        k->next = g.killer.next;
        g.killer.next = k;
    }

    k->format = format;
    Strcpy(k->name, killername ? killername : "");
    g.killer.name[0] = 0;
}

struct kinfo *
find_delayed_killer(int id)
{
    struct kinfo *k;

    for (k = g.killer.next; k != (struct kinfo *) 0; k = k->next) {
        if (k->id == id)
            break;
    }
    return k;
}

void
dealloc_killer(struct kinfo *kptr)
{
    struct kinfo *prev = &g.killer, *k;

    if (kptr == (struct kinfo *) 0)
        return;
    for (k = g.killer.next; k != (struct kinfo *) 0; k = k->next) {
        if (k == kptr)
            break;
        prev = k;
    }

    if (k == (struct kinfo *) 0) {
        impossible("dealloc_killer (#%d) not on list", kptr->id);
    } else {
        prev->next = k->next;
        free((genericptr_t) k);
        debugpline1("freed delayed killer #%d", kptr->id);
    }
}

void
save_killers(NHFILE *nhfp)
{
    struct kinfo *kptr;

    if (perform_bwrite(nhfp)) {
        for (kptr = &g.killer; kptr != (struct kinfo *) 0; kptr = kptr->next) {
            if (nhfp->structlevel)
                bwrite(nhfp->fd, (genericptr_t)kptr, sizeof(struct kinfo));
        }
    }
    if (release_data(nhfp)) {
        while (g.killer.next) {
            kptr = g.killer.next->next;
            free((genericptr_t) g.killer.next);
            g.killer.next = kptr;
        }
    }
}

void
restore_killers(NHFILE *nhfp)
{
    struct kinfo *kptr;

    for (kptr = &g.killer; kptr != (struct kinfo *) 0; kptr = kptr->next) {
        if (nhfp->structlevel)
            mread(nhfp->fd, (genericptr_t)kptr, sizeof(struct kinfo));
        if (kptr->next) {
            kptr->next = (struct kinfo *) alloc(sizeof (struct kinfo));
        }
    }
}

static int
wordcount(char *p)
{
    int words = 0;

    while (*p) {
        while (*p && isspace((uchar) *p))
            p++;
        if (*p)
            words++;
        while (*p && !isspace((uchar) *p))
            p++;
    }
    return words;
}

static void
bel_copy1(char **inp, char *out)
{
    char *in = *inp;

    out += strlen(out); /* eos() */
    while (*in && isspace((uchar) *in))
        in++;
    while (*in && !isspace((uchar) *in))
        *out++ = *in++;
    *out = '\0';
    *inp = in;
}

char *
build_english_list(char *in)
{
    char *out, *p = in;
    int len = (int) strlen(p), words = wordcount(p);

    /* +3: " or " - " "; +(words - 1): (N-1)*(", " - " ") */
    if (words > 1)
        len += 3 + (words - 1);
    out = (char *) alloc(len + 1);
    *out = '\0'; /* bel_copy1() appends */

    switch (words) {
    case 0:
        impossible("no words in list");
        break;
    case 1:
        /* "single" */
        bel_copy1(&p, out);
        break;
    default:
        if (words == 2) {
            /* "first or second" */
            bel_copy1(&p, out);
            Strcat(out, " ");
        } else {
            /* "first, second, or third */
            do {
                bel_copy1(&p, out);
                Strcat(out, ", ");
            } while (--words > 1);
        }
        Strcat(out, "or ");
        bel_copy1(&p, out);
        break;
    }
    return out;
}

/*end.c*/
