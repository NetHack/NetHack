/* NetHack 3.7	pline.c	$NHDT-Date: 1646255375 2022/03/02 21:09:35 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.109 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2018. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#define BIGBUFSZ (5 * BUFSZ) /* big enough to format a 4*BUFSZ string (from
                              * config file parsing) with modest decoration;
                              * result will then be truncated to BUFSZ-1 */

static void putmesg(const char *);
static char *You_buf(int);
#if defined(MSGHANDLER)
static void execplinehandler(const char *);
#endif
#ifdef USER_SOUNDS
extern void maybe_play_sound(const char *);
#endif
#ifdef DUMPLOG

/* keep the most recent DUMPLOG_MSG_COUNT messages */
void
dumplogmsg(const char *line)
{
    /*
     * TODO:
     *  This essentially duplicates message history, which is
     *  currently implemented in an interface-specific manner.
     *  The core should take responsibility for that and have
     *  this share it.
     */
    unsigned indx = g.saved_pline_index; /* next slot to use */
    char *oldest = g.saved_plines[indx]; /* current content of that slot */

    if (!strncmp(line, "Unknown command", 15))
        return;
    if (oldest && strlen(oldest) >= strlen(line)) {
        /* this buffer will gradually shrink until the 'else' is needed;
           there's no pressing need to track allocation size instead */
        Strcpy(oldest, line);
    } else {
        if (oldest)
            free((genericptr_t) oldest);
        g.saved_plines[indx] = dupstr(line);
    }
    g.saved_pline_index = (indx + 1) % DUMPLOG_MSG_COUNT;
}

/* called during save (unlike the interface-specific message history,
   this data isn't saved and restored); end-of-game releases saved_plines[]
   while writing its contents to the final dump log */
void
dumplogfreemessages(void)
{
    unsigned i;

    for (i = 0; i < DUMPLOG_MSG_COUNT; ++i)
        if (g.saved_plines[i])
            free((genericptr_t) g.saved_plines[i]), g.saved_plines[i] = 0;
    g.saved_pline_index = 0;
}
#endif

/* keeps windowprocs usage out of pline() */
static void
putmesg(const char *line)
{
    int attr = ATR_NONE;

    if ((g.pline_flags & URGENT_MESSAGE) != 0
        && (windowprocs.wincap2 & WC2_URGENT_MESG) != 0)
        attr |= ATR_URGENT;
    if ((g.pline_flags & SUPPRESS_HISTORY) != 0
        && (windowprocs.wincap2 & WC2_SUPPRESS_HIST) != 0)
        attr |= ATR_NOHISTORY;

    putstr(WIN_MESSAGE, attr, line);
}

static void vpline(const char *, va_list);

DISABLE_WARNING_FORMAT_NONLITERAL

void
pline(const char *line, ...)
{
    va_list the_args;

    va_start(the_args, line);
    vpline(line, the_args);
    va_end(the_args);
}

static void
vpline(const char *line, va_list the_args)
{
    static int in_pline = 0;
    char pbuf[BIGBUFSZ]; /* will get chopped down to BUFSZ-1 if longer */
    int ln;
    int msgtyp;
#if !defined(NO_VSNPRINTF)
    int vlen = 0;
#endif
    boolean no_repeat;

    if (!line || !*line)
        return;
#ifdef HANGUPHANDLING
    if (g.program_state.done_hup)
        return;
#endif
    if (g.program_state.wizkit_wishing)
        return;

    if (index(line, '%')) {
#if !defined(NO_VSNPRINTF)
        vlen = vsnprintf(pbuf, sizeof(pbuf), line, the_args);
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED) && defined(DEBUG)
        if (vlen >= (int) sizeof pbuf)
            panic("%s: truncation of buffer at %zu of %d bytes",
                  "pline", sizeof pbuf, vlen);
#else
        nhUse(vlen);
#endif
#else
        Vsprintf(pbuf, line, the_args);
#endif
        line = pbuf;
    }
    if ((ln = (int) strlen(line)) > BUFSZ - 1) {
        if (line != pbuf)                          /* no '%' was present */
            (void) strncpy(pbuf, line, BUFSZ - 1); /* caveat: unterminated */
        /* truncate, preserving the final 3 characters:
           "___ extremely long text" -> "___ extremely l...ext"
           (this may be suboptimal if overflow is less than 3) */
        memcpy(pbuf + BUFSZ - 1 - 6, "...", 3);
        /* avoid strncpy; buffers could overlap if excess is small */
        pbuf[BUFSZ - 1 - 3] = line[ln - 3];
        pbuf[BUFSZ - 1 - 2] = line[ln - 2];
        pbuf[BUFSZ - 1 - 1] = line[ln - 1];
        pbuf[BUFSZ - 1] = '\0';
        line = pbuf;
    }
    msgtyp = MSGTYP_NORMAL;

#ifdef DUMPLOG
    /* We hook here early to have options-agnostic output.
     * Unfortunately, that means Norep() isn't honored (general issue) and
     * that short lines aren't combined into one longer one (tty behavior).
     */
    if ((g.pline_flags & SUPPRESS_HISTORY) == 0)
        dumplogmsg(line);
#endif
    /* use raw_print() if we're called too early (or perhaps too late
       during shutdown) or if we're being called recursively (probably
       via debugpline() in the interface code) */
    if (in_pline++ || !iflags.window_inited) {
        /* [we should probably be using raw_printf("\n%s", line) here] */
        raw_print(line);
        iflags.last_msg = PLNMSG_UNKNOWN;
        goto pline_done;
    }

    no_repeat = (g.pline_flags & PLINE_NOREPEAT) ? TRUE : FALSE;
    if ((g.pline_flags & OVERRIDE_MSGTYPE) == 0) {
        msgtyp = msgtype_type(line, no_repeat);
#ifdef USER_SOUNDS
        if (msgtyp == MSGTYP_NORMAL || msgtyp == MSGTYP_NOSHOW)
            maybe_play_sound(line);
#endif
        if ((g.pline_flags & URGENT_MESSAGE) == 0
            && (msgtyp == MSGTYP_NOSHOW
                || (msgtyp == MSGTYP_NOREP && !strcmp(line, g.prevmsg))))
            /* FIXME: we need a way to tell our caller that this message
             * was suppressed so that caller doesn't set iflags.last_msg
             * for something that hasn't been shown, otherwise a subsequent
             * message which uses alternate wording based on that would be
             * doing so out of context and probably end up seeming silly.
             * (Not an issue for no-repeat but matters for no-show.)
             */
            goto pline_done;
    }

    if (g.vision_full_recalc)
        vision_recalc(0);
    if (u.ux)
        flush_screen(1); /* %% */

    putmesg(line);

#if defined(MSGHANDLER)
    execplinehandler(line);
#endif

    /* this gets cleared after every pline message */
    iflags.last_msg = PLNMSG_UNKNOWN;
    (void) strncpy(g.prevmsg, line, BUFSZ), g.prevmsg[BUFSZ - 1] = '\0';
    if (msgtyp == MSGTYP_STOP)
        display_nhwindow(WIN_MESSAGE, TRUE); /* --more-- */

 pline_done:
    --in_pline;
}

RESTORE_WARNING_FORMAT_NONLITERAL

/* pline() variant which can override MSGTYPE handling or suppress
   message history (tty interface uses pline() to issue prompts and
   they shouldn't be blockable via MSGTYPE=hide) */
void
custompline(unsigned pflags, const char *line, ...)
{
    va_list the_args;

    va_start(the_args, line);
    g.pline_flags = pflags;
    vpline(line, the_args);
    g.pline_flags = 0;
    va_end(the_args);
}

/* if player has dismissed --More-- with ESC to suppress further messages
   until next input request, tell the interface that it should override that
   and re-enable them; equivalent to custompline(URGENT_MESSAGE, line, ...)
   but slightly simpler to use */
void
urgent_pline(const char *line, ...)
{
    va_list the_args;

    va_start(the_args, line);
    g.pline_flags = URGENT_MESSAGE;
    vpline(line, the_args);
    g.pline_flags = 0;
    va_end(the_args);
}

void
Norep(const char *line, ...)
{
    va_list the_args;

    va_start(the_args, line);
    g.pline_flags = PLINE_NOREPEAT;
    vpline(line, the_args);
    g.pline_flags = 0;
    va_end(the_args);
}

static char *
You_buf(int siz)
{
    if (siz > g.you_buf_siz) {
        if (g.you_buf)
            free((genericptr_t) g.you_buf);
        g.you_buf_siz = siz + 10;
        g.you_buf = (char *) alloc((unsigned) g.you_buf_siz);
    }
    return g.you_buf;
}

void
free_youbuf(void)
{
    if (g.you_buf)
        free((genericptr_t) g.you_buf), g.you_buf = (char *) 0;
    g.you_buf_siz = 0;
}

/* `prefix' must be a string literal, not a pointer */
#define YouPrefix(pointer, prefix, text) \
    Strcpy((pointer = You_buf((int) (strlen(text) + sizeof prefix))), prefix)

#define YouMessage(pointer, prefix, text) \
    strcat((YouPrefix(pointer, prefix, text), pointer), text)

void
You(const char *line, ...)
{
    va_list the_args;
    char *tmp;

    va_start(the_args, line);
    vpline(YouMessage(tmp, "You ", line), the_args);
    va_end(the_args);
}

void
Your(const char *line, ...)
{
    va_list the_args;
    char *tmp;

    va_start(the_args, line);
    vpline(YouMessage(tmp, "Your ", line), the_args);
    va_end(the_args);
}

void
You_feel(const char *line, ...)
{
    va_list the_args;
    char *tmp;

    va_start(the_args, line);
    if (Unaware)
        YouPrefix(tmp, "You dream that you feel ", line);
    else
        YouPrefix(tmp, "You feel ", line);
    vpline(strcat(tmp, line), the_args);
    va_end(the_args);
}

void
You_cant(const char *line, ...)
{
    va_list the_args;
    char *tmp;

    va_start(the_args, line);
    vpline(YouMessage(tmp, "You can't ", line), the_args);
    va_end(the_args);
}

void
pline_The(const char *line, ...)
{
    va_list the_args;
    char *tmp;

    va_start(the_args, line);
    vpline(YouMessage(tmp, "The ", line), the_args);
    va_end(the_args);
}

void
There(const char *line, ...)
{
    va_list the_args;
    char *tmp;

    va_start(the_args, line);
    vpline(YouMessage(tmp, "There ", line), the_args);
    va_end(the_args);
}

void
You_hear(const char *line, ...)
{
    va_list the_args;
    char *tmp;

    if (Deaf || !flags.acoustics)
        return;
    va_start(the_args, line);
    if (Underwater)
        YouPrefix(tmp, "You barely hear ", line);
    else if (Unaware)
        YouPrefix(tmp, "You dream that you hear ", line);
    else
        YouPrefix(tmp, "You hear ", line);  /* Deaf-aware */
    vpline(strcat(tmp, line), the_args);
    va_end(the_args);
}

void
You_see(const char *line, ...)
{
    va_list the_args;
    char *tmp;

    va_start(the_args, line);
    if (Unaware)
        YouPrefix(tmp, "You dream that you see ", line);
    else if (Blind) /* caller should have caught this... */
        YouPrefix(tmp, "You sense ", line);
    else
        YouPrefix(tmp, "You see ", line);
    vpline(strcat(tmp, line), the_args);
    va_end(the_args);
}

/* Print a message inside double-quotes.
 * The caller is responsible for checking deafness.
 * Gods can speak directly to you in spite of deafness.
 */
void
verbalize(const char *line, ...)
{
    va_list the_args;
    char *tmp;

    va_start(the_args, line);
    tmp = You_buf((int) strlen(line) + sizeof "\"\"");
    Strcpy(tmp, "\"");
    Strcat(tmp, line);
    Strcat(tmp, "\"");
    vpline(tmp, the_args);
    va_end(the_args);
}

#ifdef CHRONICLE

void
gamelog_add(long glflags, long gltime, const char *str)
{
    struct gamelog_line *tmp;
    struct gamelog_line *lst = g.gamelog;

    tmp = (struct gamelog_line *) alloc(sizeof (struct gamelog_line));
    tmp->turn = gltime;
    tmp->flags = glflags;
    tmp->text = dupstr(str);
    tmp->next = NULL;
    while (lst && lst->next)
        lst = lst->next;
    if (!lst)
        g.gamelog = tmp;
    else
        lst->next = tmp;
}

void
livelog_printf(long ll_type, const char *line, ...)
{
    char gamelogbuf[BUFSZ * 2];
    va_list the_args;

    va_start(the_args, line);
    (void) vsnprintf(gamelogbuf, sizeof gamelogbuf, line, the_args);
    va_end(the_args);

    gamelog_add(ll_type, g.moves, gamelogbuf);
    strNsubst(gamelogbuf, "\t", "_", 0);
    livelog_add(ll_type, gamelogbuf);
}

#else

void
gamelog_add(
    long glflags UNUSED, long gltime UNUSED, const char *msg UNUSED)
{
    ; /* nothing here */
}

void
livelog_printf(
    long ll_type UNUSED, const char *line UNUSED, ...)
{
    ; /* nothing here */
}

#endif /* !CHRONICLE */

static void vraw_printf(const char *, va_list);

void
raw_printf(const char *line, ...)
{
    va_list the_args;

    va_start(the_args, line);
    vraw_printf(line, the_args);
    va_end(the_args);
    if (!g.program_state.beyond_savefile_load)
        g.early_raw_messages++;
}

DISABLE_WARNING_FORMAT_NONLITERAL

static void
vraw_printf(const char *line, va_list the_args)
{
    char pbuf[BIGBUFSZ]; /* will be chopped down to BUFSZ-1 if longer */

    if (index(line, '%')) {
#if !defined(NO_VSNPRINTF)
        (void) vsnprintf(pbuf, sizeof(pbuf), line, the_args);
#else
        Vsprintf(pbuf, line, the_args);
#endif
        line = pbuf;
    }
    if ((int) strlen(line) > BUFSZ - 1) {
        if (line != pbuf)
            line = strncpy(pbuf, line, BUFSZ - 1);
        /* unlike pline, we don't futz around to keep last few chars */
        pbuf[BUFSZ - 1] = '\0'; /* terminate strncpy or truncate vsprintf */
    }
    raw_print(line);
#if defined(MSGHANDLER)
    execplinehandler(line);
#endif
    if (!g.program_state.beyond_savefile_load)
        g.early_raw_messages++;
}

void
impossible(const char *s, ...)
{
    va_list the_args;
    char pbuf[BIGBUFSZ]; /* will be chopped down to BUFSZ-1 if longer */

    va_start(the_args, s);
    if (g.program_state.in_impossible)
        panic("impossible called impossible");

    g.program_state.in_impossible = 1;
#if !defined(NO_VSNPRINTF)
    (void) vsnprintf(pbuf, sizeof(pbuf), s, the_args);
#else
    Vsprintf(pbuf, s, the_args);
#endif
    va_end(the_args);
    pbuf[BUFSZ - 1] = '\0'; /* sanity */
    paniclog("impossible", pbuf);
    if (iflags.debug_fuzzer)
        panic("%s", pbuf);
    pline("%s", pbuf);
    /* reuse pbuf[] */
    Strcpy(pbuf, "Program in disorder!");
    if (g.program_state.something_worth_saving)
        Strcat(pbuf, "  (Saving and reloading may fix this problem.)");
    pline("%s", pbuf);
    pline("Please report these messages to %s.", DEVTEAM_EMAIL);
    if (sysopt.support) {
        pline("Alternatively, contact local support: %s", sysopt.support);
    }

    g.program_state.in_impossible = 0;
}

RESTORE_WARNING_FORMAT_NONLITERAL

#if defined(MSGHANDLER)
static boolean use_pline_handler = TRUE;

static void
execplinehandler(const char *line)
{
#if defined(POSIX_TYPES) || defined(__GNUC__)
    int f;
#endif
    const char *args[3];
    char *env;

    if (!use_pline_handler)
        return;

    if (!(env = nh_getenv("NETHACK_MSGHANDLER"))) {
        use_pline_handler = FALSE;
        return;
    }

#if defined(POSIX_TYPES) || defined(__GNUC__)
    f = fork();
    if (f == 0) { /* child */
        args[0] = env;
        args[1] = line;
        args[2] = NULL;
        (void) setgid(getgid());
        (void) setuid(getuid());
        (void) execv(args[0], (char *const *) args);
        perror((char *) 0);
        (void) fprintf(stderr, "Exec to message handler %s failed.\n", env);
        nh_terminate(EXIT_FAILURE);
    } else if (f > 0) {
        int status;

        waitpid(f, &status, 0);
    } else if (f == -1) {
        perror((char *) 0);
        use_pline_handler = FALSE;
        pline("%s", "Fork to message handler failed.");
    }
#elif defined(WIN32)
    {
        intptr_t ret;
        args[0] = env;
        args[1] = line;
        args[2] = NULL;
        ret = _spawnv(_P_NOWAIT, env, args);
    }
#else
#error MSGHANDLER is not implemented on this sysytem.
#endif
}
#endif /* MSGHANDLER */

/*
 * varargs handling for files.c
 */
static void vconfig_error_add(const char *, va_list);

DISABLE_WARNING_FORMAT_NONLITERAL

void
config_error_add(const char *str, ...)
{
    va_list the_args;

    va_start(the_args, str);
    vconfig_error_add(str, the_args);
    va_end(the_args);
}

static void
vconfig_error_add(const char *str, va_list the_args)
{       /* start of vconf...() or of nested block in USE_OLDARG's conf...() */
#if !defined(NO_VSNPRINTF)
    int vlen = 0;
#endif
    char buf[BIGBUFSZ]; /* will be chopped down to BUFSZ-1 if longer */

#if !defined(NO_VSNPRINTF)
    vlen = vsnprintf(buf, sizeof buf, str, the_args);
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED) && defined(DEBUG)
    if (vlen >= (int) sizeof buf)
        panic("%s: truncation of buffer at %zu of %d bytes",
              "config_error_add", sizeof buf, vlen);
#else
    nhUse(vlen);
#endif
#else
    Vsprintf(buf, str, the_args);
#endif
    buf[BUFSZ - 1] = '\0';
    config_erradd(buf);
}

RESTORE_WARNING_FORMAT_NONLITERAL

/* nhassert_failed is called when an nhassert's condition is false */
void
nhassert_failed(const char *expression, const char *filepath, int line)
{
    const char *filename, *p;

    /* Attempt to get filename from path.
       TODO: we really need a port provided function to return a filename
       from a path. */
    filename = filepath;
    if ((p = strrchr(filename, '/')) != 0)
        filename = p + 1;
    if ((p = strrchr(filename, '\\')) != 0)
        filename = p + 1;
#ifdef VMS
    /* usually "device:[directory]name"
       but might be "device:[root.][directory]name"
       and either "[directory]" or "[root.]" or both can be delimited
       by <> rather than by []; find the last of ']', '>', and ':'  */
    if ((p = strrchr(filename, ']')) != 0)
        filename = p + 1;
    if ((p = strrchr(filename, '>')) != 0)
        filename = p + 1;
    if ((p = strrchr(filename, ':')) != 0)
        filename = p + 1;
#endif

    impossible("nhassert(%s) failed in file '%s' at line %d",
               expression, filename, line);
}

/*pline.c*/
