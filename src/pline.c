/* NetHack 3.7	pline.c	$NHDT-Date: 1693083243 2023/08/26 20:54:03 $  $NHDT-Branch: keni-crashweb2 $:$NHDT-Revision: 1.124 $ */
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
    unsigned indx = gs.saved_pline_index; /* next slot to use */
    char *oldest = gs.saved_plines[indx]; /* current content of that slot */

    if (!strncmp(line, "Unknown command", 15))
        return;
    if (oldest && strlen(oldest) >= strlen(line)) {
        /* this buffer will gradually shrink until the 'else' is needed;
           there's no pressing need to track allocation size instead */
        Strcpy(oldest, line);
    } else {
        if (oldest)
            free((genericptr_t) oldest);
        gs.saved_plines[indx] = dupstr(line);
    }
    gs.saved_pline_index = (indx + 1) % DUMPLOG_MSG_COUNT;
}

/* called during save (unlike the interface-specific message history,
   this data isn't saved and restored); end-of-game releases saved_plines[]
   while writing its contents to the final dump log */
void
dumplogfreemessages(void)
{
    unsigned i;

    for (i = 0; i < DUMPLOG_MSG_COUNT; ++i)
        if (gs.saved_plines[i])
            free((genericptr_t) gs.saved_plines[i]), gs.saved_plines[i] = 0;
    gs.saved_pline_index = 0;
}
#endif

/* keeps windowprocs usage out of pline() */
static void
putmesg(const char *line)
{
    int attr = ATR_NONE;

    if ((gp.pline_flags & URGENT_MESSAGE) != 0
        && (windowprocs.wincap2 & WC2_URGENT_MESG) != 0)
        attr |= ATR_URGENT;
    if ((gp.pline_flags & SUPPRESS_HISTORY) != 0
        && (windowprocs.wincap2 & WC2_SUPPRESS_HIST) != 0)
        attr |= ATR_NOHISTORY;
    putstr(WIN_MESSAGE, attr, line);
    SoundSpeak(line);
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

void
pline_dir(int dir, const char *line, ...)
{
    va_list the_args;

    set_msg_dir(dir);

    va_start(the_args, line);
    vpline(line, the_args);
    va_end(the_args);
}

void
pline_xy(coordxy x, coordxy y, const char *line, ...)
{
    va_list the_args;

    set_msg_xy(x, y);

    va_start(the_args, line);
    vpline(line, the_args);
    va_end(the_args);
}

/* set the direction where next message happens */
void
set_msg_dir(int dir)
{
    dtoxy(&a11y.msg_loc, dir);
    a11y.msg_loc.x += u.ux;
    a11y.msg_loc.y += u.uy;
}

/* set the coordinate where next message happens */
void
set_msg_xy(coordxy x, coordxy y)
{
    a11y.msg_loc.x = x;
    a11y.msg_loc.y = y;
}

static void
vpline(const char *line, va_list the_args)
{
    static int in_pline = 0;
    char pbuf[BIGBUFSZ]; /* will get chopped down to BUFSZ-1 if longer */
    int ln;
    int msgtyp;
    boolean no_repeat;

    if (!line || !*line)
        return;
#ifdef HANGUPHANDLING
    if (gp.program_state.done_hup)
        return;
#endif
    if (gp.program_state.wizkit_wishing)
        return;

    if (a11y.accessiblemsg && isok(a11y.msg_loc.x,a11y.msg_loc.y)) {
        char *tmp;
        char *dirstr;
        static char dirstrbuf[BUFSZ];
        int g = (iflags.getpos_coords == GPCOORDS_NONE)
            ? GPCOORDS_COMFULL : iflags.getpos_coords;

        dirstr = coord_desc(a11y.msg_loc.x, a11y.msg_loc.y, dirstrbuf, g);
        a11y.msg_loc.x = a11y.msg_loc.y = 0;
        tmp = (char *)alloc(strlen(line) + sizeof ": " + strlen(dirstr));
        Strcpy(tmp, dirstr);
        Strcat(tmp, ": ");
        Strcat(tmp, line);
        vpline(tmp, the_args);
        free((genericptr_t) tmp);
        return;
    }

    if (!strchr(line, '%')) {
        /* format does not specify any substitutions; use it as-is */
        ln = (int) strlen(line);
    } else if (line[0] == '%' && line[1] == 's' && !line[2]) {
        /* "%s" => single string; skip format and use its first argument;
           unlike with the format, it is irrelevant whether the argument
           contains any percent signs */
        line = va_arg(the_args, const char *); /*VA_NEXT(line,const char *);*/
        ln = (int) strlen(line);
    } else {
        /* perform printf() formatting */
        ln = vsnprintf(pbuf, sizeof pbuf, line, the_args);
        line = pbuf;
        /* note: 'ln' is number of characters attempted, not necessarily
           strlen(line); that matters for the overflow check; if we avoid
           the extremely-too-long panic then 'ln' will be actual length */
    }
    if (ln > (int) sizeof pbuf - 1) /* extremely too long */
        panic("pline attempting to print %d characters!", ln);

    if (ln > BUFSZ - 1) {
        /* too long but modestly so; allow but truncate, preserving final
           3 chars: "___ extremely long text" -> "___ extremely l...ext"
           (this may be suboptimal if overflow is less than 3) */
        if (line != pbuf) /* no '%' was present or format was just "%s" */
            (void) strncpy(pbuf, line, BUFSZ - 1); /* caveat: unterminated */
        pbuf[BUFSZ - 1 - 6] = pbuf[BUFSZ - 1 - 5] = pbuf[BUFSZ - 1 - 4] = '.';
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
    if ((gp.pline_flags & SUPPRESS_HISTORY) == 0)
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

    no_repeat = (gp.pline_flags & PLINE_NOREPEAT) ? TRUE : FALSE;
    if ((gp.pline_flags & OVERRIDE_MSGTYPE) == 0) {
        msgtyp = msgtype_type(line, no_repeat);
#ifdef USER_SOUNDS
        if (msgtyp == MSGTYP_NORMAL || msgtyp == MSGTYP_NOSHOW)
            maybe_play_sound(line);
#endif
        if ((gp.pline_flags & URGENT_MESSAGE) == 0
            && (msgtyp == MSGTYP_NOSHOW
                || (msgtyp == MSGTYP_NOREP && !strcmp(line, gp.prevmsg))))
            /* FIXME: we need a way to tell our caller that this message
             * was suppressed so that caller doesn't set iflags.last_msg
             * for something that hasn't been shown, otherwise a subsequent
             * message which uses alternate wording based on that would be
             * doing so out of context and probably end up seeming silly.
             * (Not an issue for no-repeat but matters for no-show.)
             */
            goto pline_done;
    }

    if (gv.vision_full_recalc)
        vision_recalc(0);
    if (u.ux)
        flush_screen((gp.pline_flags & NO_CURS_ON_U) ? 0 : 1); /* %% */

    putmesg(line);

#if defined(MSGHANDLER)
    execplinehandler(line);
#endif

    /* this gets cleared after every pline message */
    iflags.last_msg = PLNMSG_UNKNOWN;
    (void) strncpy(gp.prevmsg, line, BUFSZ), gp.prevmsg[BUFSZ - 1] = '\0';
    if (msgtyp == MSGTYP_STOP)
        display_nhwindow(WIN_MESSAGE, TRUE); /* --more-- */
 pline_done:
#ifdef SND_SPEECH
    /* clear the SPEECH flag so caller never has to */
    gp.pline_flags &= ~PLINE_SPEECH;
#endif
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
    gp.pline_flags = pflags;
    vpline(line, the_args);
    gp.pline_flags = 0;
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
    gp.pline_flags = URGENT_MESSAGE;
    vpline(line, the_args);
    gp.pline_flags = 0;
    va_end(the_args);
}

void
Norep(const char *line, ...)
{
    va_list the_args;

    va_start(the_args, line);
    gp.pline_flags = PLINE_NOREPEAT;
    vpline(line, the_args);
    gp.pline_flags = 0;
    va_end(the_args);
}

static char *
You_buf(int siz)
{
    if (siz > gy.you_buf_siz) {
        if (gy.you_buf)
            free((genericptr_t) gy.you_buf);
        gy.you_buf_siz = siz + 10;
        gy.you_buf = (char *) alloc((unsigned) gy.you_buf_siz);
    }
    return gy.you_buf;
}

void
free_youbuf(void)
{
    if (gy.you_buf)
        free((genericptr_t) gy.you_buf), gy.you_buf = (char *) 0;
    gy.you_buf_siz = 0;
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

    if ((Deaf && !Unaware) || !flags.acoustics)
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
    gp.pline_flags |= PLINE_VERBALIZE;
    tmp = You_buf((int) strlen(line) + sizeof "\"\"");
    Strcpy(tmp, "\"");
    Strcat(tmp, line);
    Strcat(tmp, "\"");
    vpline(tmp, the_args);
    gp.pline_flags &= ~PLINE_VERBALIZE;
    va_end(the_args);
}

#ifdef CHRONICLE

void
gamelog_add(long glflags, long gltime, const char *str)
{
    struct gamelog_line *tmp;
    struct gamelog_line *lst = gg.gamelog;

    tmp = (struct gamelog_line *) alloc(sizeof (struct gamelog_line));
    tmp->turn = gltime;
    tmp->flags = glflags;
    tmp->text = dupstr(str);
    tmp->next = NULL;
    while (lst && lst->next)
        lst = lst->next;
    if (!lst)
        gg.gamelog = tmp;
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

    gamelog_add(ll_type, gm.moves, gamelogbuf);
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
    if (!gp.program_state.beyond_savefile_load)
        ge.early_raw_messages++;
}

DISABLE_WARNING_FORMAT_NONLITERAL

static void
vraw_printf(const char *line, va_list the_args)
{
    char pbuf[BIGBUFSZ]; /* will be chopped down to BUFSZ-1 if longer */

    if (strchr(line, '%')) {
        (void) vsnprintf(pbuf, sizeof(pbuf), line, the_args);
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
    if (!gp.program_state.beyond_savefile_load)
        ge.early_raw_messages++;
}

void
impossible(const char *s, ...)
{
    va_list the_args;
    char pbuf[BIGBUFSZ]; /* will be chopped down to BUFSZ-1 if longer */
    char pbuf2[BUFSZ];

    va_start(the_args, s);
    if (gp.program_state.in_impossible)
        panic("impossible called impossible");

    gp.program_state.in_impossible = 1;
    (void) vsnprintf(pbuf, sizeof pbuf, s, the_args);
    va_end(the_args);
    pbuf[BUFSZ - 1] = '\0'; /* sanity */
    paniclog("impossible", pbuf);
    if (iflags.debug_fuzzer)
        panic("%s", pbuf);

    gp.pline_flags = URGENT_MESSAGE;
    pline("%s", pbuf);
    gp.pline_flags = 0;

    Strcpy(pbuf2, "Program in disorder!");
    if (gp.program_state.something_worth_saving)
        Strcat(pbuf2, "  (Saving and reloading may fix this problem.)");
    pline("%s", pbuf2);
    pline("Please report these messages to %s.", DEVTEAM_EMAIL);
    if (sysopt.support) {
        pline("Alternatively, contact local support: %s", sysopt.support);
    }

#ifdef CRASHREPORT
    if(sysopt.crashreporturl){
        boolean report = ('y' == yn_function("Report now?","yn",'n',FALSE));
        raw_print("");  // prove to the user the character was accepted
        if(report){
            submit_web_report("Impossible", pbuf);
        }
    }
#endif

    gp.program_state.in_impossible = 0;
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
    int vlen = 0;
    char buf[BIGBUFSZ]; /* will be chopped down to BUFSZ-1 if longer */

    vlen = vsnprintf(buf, sizeof buf, str, the_args);
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED) && defined(DEBUG)
    if (vlen >= (int) sizeof buf)
        panic("%s: truncation of buffer at %zu of %d bytes",
              "config_error_add", sizeof buf, vlen);
#else
    nhUse(vlen);
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
