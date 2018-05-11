/* NetHack 3.6	pline.c	$NHDT-Date: 1520964541 2018/03/13 18:09:01 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.66 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2018. */
/* NetHack may be freely redistributed.  See license for details. */

#define NEED_VARARGS /* Uses ... */ /* comment line for pre-compiled headers \
                                       */
#include "hack.h"

static unsigned pline_flags = 0;
static char prevmsg[BUFSZ];

static char *You_buf(int);
#if defined(MSGHANDLER) && (defined(POSIX_TYPES) || defined(__GNUC__))
static void execplinehandler(const char *);
#endif

#ifdef DUMPLOG
/* also used in end.c */
unsigned saved_pline_index = 0; /* slot in saved_plines[] to use next */
char *saved_plines[DUMPLOG_MSG_COUNT] = { (char *) 0 };

/* keep the most recent DUMPLOG_MSG_COUNT messages */
void
dumplogmsg(line)
const char *line;
{
    /*
     * TODO:
     *  This essentially duplicates message history, which is
     *  currently implemented in an interface-specific manner.
     *  The core should take responsibility for that and have
     *  this share it.
     */
    unsigned indx = saved_pline_index; /* next slot to use */
    char *oldest = saved_plines[indx]; /* current content of that slot */

    if (oldest && strlen(oldest) >= strlen(line)) {
        /* this buffer will gradually shrink until the 'else' is needed;
           there's no pressing need to track allocation size instead */
        Strcpy(oldest, line);
    } else {
        if (oldest)
            free((genericptr_t) oldest);
        saved_plines[indx] = dupstr(line);
    }
    saved_pline_index = (indx + 1) % DUMPLOG_MSG_COUNT;
}

/* called during save (unlike the interface-specific message history,
   this data isn't saved and restored); end-of-game releases saved_pline[]
   while writing its contents to the final dump log */
void
dumplogfreemessages()
{
    unsigned indx;

    for (indx = 0; indx < DUMPLOG_MSG_COUNT; ++indx)
        if (saved_plines[indx])
            free((genericptr_t) saved_plines[indx]), saved_plines[indx] = 0;
    saved_pline_index = 0;
}
#endif

/*VARARGS1*/
/* Note that these declarations rely on knowledge of the internals
 * of the variable argument handling stuff in "tradstdc.h"
 */

static void vpline(const char *, va_list);

void pline
(const char *line, ...) {
    va_list the_args;
    va_start(the_args, line);
    vpline(line, the_args);
    va_end(the_args);
}

static void
vpline(const char *line, va_list the_args)
{       /* start of vpline() or of nested block in USE_OLDARG's pline() */
    static int in_pline = 0;
    char pbuf[3 * BUFSZ];
    int ln;
    int msgtyp;
    boolean no_repeat;
    /* Do NOT use VA_START and VA_END in here... see above */

    if (!line || !*line)
        return;
#ifdef HANGUPHANDLING
    if (program_state.done_hup)
        return;
#endif
    if (program_state.wizkit_wishing)
        return;

    if (index(line, '%')) {
        Vsprintf(pbuf, line, the_args);
        line = pbuf;
    }
    if ((ln = (int) strlen(line)) > BUFSZ - 1) {
        if (line != pbuf)                          /* no '%' was present */
            (void) strncpy(pbuf, line, BUFSZ - 1); /* caveat: unterminated */
        /* truncate, preserving the final 3 characters:
           "___ extremely long text" -> "___ extremely l...ext"
           (this may be suboptimal if overflow is less than 3) */
        (void) strncpy(pbuf + BUFSZ - 1 - 6, "...", 3);
        /* avoid strncpy; buffers could overlap if excess is small */
        pbuf[BUFSZ - 1 - 3] = line[ln - 3];
        pbuf[BUFSZ - 1 - 2] = line[ln - 2];
        pbuf[BUFSZ - 1 - 1] = line[ln - 1];
        pbuf[BUFSZ - 1] = '\0';
        line = pbuf;
    }

#ifdef DUMPLOG
    /* We hook here early to have options-agnostic output.
     * Unfortunately, that means Norep() isn't honored (general issue) and
     * that short lines aren't combined into one longer one (tty behavior).
     */
    if ((pline_flags & SUPPRESS_HISTORY) == 0)
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

    msgtyp = MSGTYP_NORMAL;
    no_repeat = (pline_flags & PLINE_NOREPEAT) ? TRUE : FALSE;
    if ((pline_flags & OVERRIDE_MSGTYPE) == 0) {
        msgtyp = msgtype_type(line, no_repeat);
        if (msgtyp == MSGTYP_NOSHOW
            || (msgtyp == MSGTYP_NOREP && !strcmp(line, prevmsg)))
            /* FIXME: we need a way to tell our caller that this message
             * was suppressed so that caller doesn't set iflags.last_msg
             * for something that hasn't been shown, otherwise a subsequent
             * message which uses alternate wording based on that would be
             * doing so out of context and probably end up seeming silly.
             * (Not an issue for no-repeat but matters for no-show.)
             */
            goto pline_done;
    }

    if (vision_full_recalc)
        vision_recalc(0);
    if (u.ux)
        flush_screen(1); /* %% */

    putstr(WIN_MESSAGE, 0, line);

#if defined(MSGHANDLER) && (defined(POSIX_TYPES) || defined(__GNUC__))
    execplinehandler(line);
#endif

    /* this gets cleared after every pline message */
    iflags.last_msg = PLNMSG_UNKNOWN;
    (void) strncpy(prevmsg, line, BUFSZ), prevmsg[BUFSZ - 1] = '\0';
    if (msgtyp == MSGTYP_STOP)
        display_nhwindow(WIN_MESSAGE, TRUE); /* --more-- */

 pline_done:
    --in_pline;
    return;
}

/* pline() variant which can override MSGTYPE handling or suppress
   message history (tty interface uses pline() to issue prompts and
   they shouldn't be blockable via MSGTYPE=hide) */
/*VARARGS2*/
void custompline
(unsigned pflags, const char *line, ...)
{
    va_list the_args;
    va_start(the_args, line);
    pline_flags = pflags;
    vpline(line, the_args);
    pline_flags = 0;
    va_end(the_args);
    return;
}

/*VARARGS1*/
void Norep(const char *line, ...)
{
    va_list the_args;
    va_start(the_args, line);
    pline_flags = PLINE_NOREPEAT;
    vpline(line, the_args);
    pline_flags = 0;
    va_end(the_args);
    return;
}

/* work buffer for You(), &c and verbalize() */
static char *you_buf = 0;
static int you_buf_siz = 0;

static char *
You_buf(int siz)
{
    if (siz > you_buf_siz) {
        if (you_buf)
            free((genericptr_t) you_buf);
        you_buf_siz = siz + 10;
        you_buf = (char *) alloc((unsigned) you_buf_siz);
    }
    return you_buf;
}

void
free_youbuf()
{
    if (you_buf) {
        free((genericptr_t) you_buf); you_buf = (char *) 0;
    }
    you_buf_siz = 0;
}

/* `prefix' must be a string literal, not a pointer */
#define YouPrefix(pointer, prefix, text) \
    Strcpy((pointer = You_buf((int) (strlen(text) + sizeof prefix))), prefix)

#define YouMessage(pointer, prefix, text) \
    strcat((YouPrefix(pointer, prefix, text), pointer), text)

/*VARARGS1*/
void You(const char *line, ...)
{
    va_list the_args;
    char *tmp;
    va_start(the_args, line);
    vpline(YouMessage(tmp, "You ", line), the_args);
    va_end(the_args);
}

/*VARARGS1*/
void Your(const char *line, ...)
{
    va_list the_args;
    char *tmp;
    va_start(the_args, line);
    vpline(YouMessage(tmp, "Your ", line), the_args);
    va_end(the_args);
}

/*VARARGS1*/
void You_feel(const char *line, ...)
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

/*VARARGS1*/
void You_cant(const char *line, ...)
{
    va_list the_args;
    char *tmp;
    va_start(the_args, line);
    vpline(YouMessage(tmp, "You can't ", line), the_args);
    va_end(the_args);
}

/*VARARGS1*/
void pline_The(const char *line, ...)
{
    va_list the_args;
    char *tmp;
    va_start(the_args, line);
    vpline(YouMessage(tmp, "The ", line), the_args);
    va_end(the_args);
}

/*VARARGS1*/
void There(const char *line, ...)
{
    va_list the_args;
    char *tmp;
    va_start(the_args, line);
    vpline(YouMessage(tmp, "There ", line), the_args);
    va_end(the_args);
}

/*VARARGS1*/
void You_hear(const char *line, ...)
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
        YouPrefix(tmp, "You hear ", line);
    vpline(strcat(tmp, line), the_args);
    va_end(the_args);
}

/*VARARGS1*/
void You_see(const char *line, ...)
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
/*VARARGS1*/
void verbalize(const char *line, ...)
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

/*VARARGS1*/
/* Note that these declarations rely on knowledge of the internals
 * of the variable argument handling stuff in "tradstdc.h"
 */

static void vraw_printf(const char *, va_list);

void raw_printf(const char *line, ...)
{
    va_list the_args;
    va_start(the_args, line);
    vraw_printf(line, the_args);
    va_end(the_args);
}

static void
vraw_printf(const char *line, va_list the_args)
{
    char pbuf[3 * BUFSZ];
    int ln;
    /* Do NOT use VA_START and VA_END in here... see above */

    if (index(line, '%')) {
        Vsprintf(pbuf, line, the_args);
        line = pbuf;
    }
    if ((ln = (int) strlen(line)) > BUFSZ - 1) {
        if (line != pbuf)
            line = strncpy(pbuf, line, BUFSZ - 1);
        /* unlike pline, we don't futz around to keep last few chars */
        pbuf[BUFSZ - 1] = '\0'; /* terminate strncpy or truncate vsprintf */
    }
    raw_print(line);
#if defined(MSGHANDLER) && (defined(POSIX_TYPES) || defined(__GNUC__))
    execplinehandler(line);
#endif
}

/*VARARGS1*/
void impossible(const char *s, ...)
{
    va_list the_args;
    char pbuf[2 * BUFSZ];
    va_start(the_args, s);
    if (program_state.in_impossible)
        panic("impossible called impossible");

    program_state.in_impossible = 1;
    Vsprintf(pbuf, s, the_args);
    pbuf[BUFSZ - 1] = '\0'; /* sanity */
    paniclog("impossible", pbuf);
    pline("%s", pbuf);
    pline("%s", "Program in disorder!  (Saving and reloading may fix this problem.)");
    program_state.in_impossible = 0;
    va_end(the_args);
}

#if defined(MSGHANDLER) && (defined(POSIX_TYPES) || defined(__GNUC__))
static boolean use_pline_handler = TRUE;
static void
execplinehandler(const char *line)
{
    int f;
    const char *args[3];
    char *env;

    if (!use_pline_handler)
        return;

    if (!(env = nh_getenv("NETHACK_MSGHANDLER"))) {
        use_pline_handler = FALSE;
        return;
    }

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
        pline("%s", VA_PASS1("Fork to message handler failed."));
    }
}
#endif /* defined(POSIX_TYPES) || defined(__GNUC__) */

/*pline.c*/
