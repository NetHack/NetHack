/* NetHack 3.7	vmsmail.c	$NHDT-Date: 1596498307 2020/08/03 23:45:07 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.11 $ */
/* Copyright (c) Robert Patrick Rankin, 1991.                     */
/* NetHack may be freely redistributed.  See license for details. */

#include "config.h"
#include "mail.h"

/* lint supression due to lack of extern.h */
unsigned long init_broadcast_trapping(void);
unsigned long enable_broadcast_trapping(void);
unsigned long disable_broadcast_trapping(void);
struct mail_info *parse_next_broadcast(void);

#ifdef MAIL
#include "wintype.h"
#include "winprocs.h"
#include <ctype.h>
#include <descrip.h>
#include <errno.h>
#ifndef __GNUC__
#include <msgdef.h>
#else
#define MSG$_TRMHANGUP 6
#define MSG$_TRMBRDCST 83
#endif /*__GNUC__*/
#include <signal.h>
/* #include <string.h> */
#define vms_ok(sts) ((sts) & 1)

static struct mail_info *parse_brdcst(char *);
static void filter_brdcst(char *);
static void flush_broadcasts(void);
static void broadcast_ast(int);
extern char *eos(char *);
extern char *strstri(const char *, const char *);
extern int strncmpi(const char *, const char *, int);

extern size_t strspn(const char *, const char *);
#ifndef __DECC
extern int VDECL(sscanf, (const char *, const char *, ...));
#endif
extern unsigned long smg$create_pasteboard(), smg$get_broadcast_message(),
    smg$set_broadcast_trapping(), smg$disable_broadcast_trapping();

extern volatile int broadcasts; /* defining declaration in mail.c */

static long pasteboard_id = 0; /* SMG's magic cookie */

/*
 * Mail (et al) overview:
 *
 *      When a broadcast is asynchronously captured, a volatile counter
 * ('broadcasts') is incremented.  Each player turn, ckmailstatus() polls
 * the counter and calls parse_next_broadcast() if it's positive; this
 * returns some display text, object name, and response command, which is
 * passed to newmail().  Routine newmail() generates a mail-daemon monster
 * who approaches the character, "speaks" the display text, and delivers
 * a scroll of mail pre-named to the object name; the response command is
 * also attached to the scroll's oextra->omailcmd field.
 * Unrecognized broadcasts result in the mail-daemon
 * arriving and announcing the display text, but no scroll being created.
 * If SHELL is undefined, then all broadcasts are treated as 'other'; since
 * no subproceses are allowed, there'd be no way to respond to the scroll.
 *
 *      When a scroll of mail is read by the character, readmail() extracts
 * the command string and uses it for the default when prompting the
 * player for a system command to spawn.  The player may enter any command
 * he or she chooses, or just <return> to accept the default or <escape> to
 * avoid executing any command.  If the command is "SPAWN", a regular shell
 * escape to DCL is performed; otherwise, the indicated single command is
 * spawned.  Either way, NetHack resumes play when the subprocess terminates
 * or explicitly reattaches to its parent.
 *
 * Broadcast parsing:
 *
 *      The following broadcast messages are [attempted to be] recognized:
 *    text fragment           name for scroll         default command
 *      New mail                VMSmail                 MAIL
 *      New ALL-IN-1 MAIL       A1mail                  A1M
 *      Software Tools mail     STmail                  MSG [+folder]
 *      MM mail                 MMmail                  MM
 *      WPmail: New mail        WPmail                  OFFICE/MAIL
 *      **M400 mail             M400mail                M400
 *      " mail", ^"mail "       unknown mail            SPAWN
 *      " phoning"              Phone call              PHONE ANSWER
 *      talk-daemon...by...foo  Talk request            TALK[/OLD] foo@bar
 *      (node)user -            Bitnet noise            XYZZY user@node
 * Anything else results in just the message text being passed along, no
 * scroll of mail so consequently no command to execute when scroll read.
 * The user can set up ``$ XYZZY :== SEND'' prior to invoking NetHack if
 * vanilla JNET responses to Bitnet messages are prefered.
 *
 *      Static return buffers are used because only one broadcast gets
 * processed at a time, and the essential information in each one is
 * either displayed and discarded or copied into a scroll-of-mail object.
 *
 *      The test driver code below can be used to check out potential new
 * entries without rebuilding NetHack itself.  CC/DEFINE="TEST_DRIVER"
 * Link it with hacklib.obj or nethack.olb/incl=hacklib (not nethack/lib).
 */

static struct mail_info msg;  /* parse_*()'s return buffer */
static char nam_buf[63],      /* maximum onamelth, size of ONAME(object) */
            cmd_buf[99],      /* arbitrary */
            txt_buf[255 + 1]; /* same size as used for message buf[] */

/* try to decipher and categorize broadcast message text
*/
static struct mail_info *
parse_brdcst(char *buf) /* called by parse_next_broadcast() */
                        /* input: filtered broadcast text */
{
    int typ;
    char *txt;
    const char *nam, *cmd;
#ifdef SHELL /* only parse if spawned commands are enabled */
    register char *p, *q;
    boolean is_jnet_send;
    char user[127 + 1], node[127 + 1], sentinel;

    /* Check these first; otherwise, their arbitrary text would enable
        easy spoofing of some other message patterns.  Unfortunately,
        any home-grown broadcast delivery program poses a similar risk. */
    if (!strncmpi(buf, "reply received", 14))
        goto other;
    is_jnet_send = (sscanf(buf, "(%[^)])%s -%c", node, user, &sentinel) == 3);
    if (is_jnet_send)
        goto jnet_send;

    /* scan the text more or less by brute force */
    if ((q = strstri(buf, " mail")) != 0 || /* all known mail broadcasts */
        !strncmpi(q = buf, "mail ", 5)) {   /* unexpected alternative */
        typ = MSG_MAIL;
        p = strstri(q, " from");
        txt = p ? strcat(strcpy(txt_buf, "Mail for you"), p) : (char *) 0;

        if (!strncmpi(buf, "new mail", 8)) {
            /*
             * New mail [on node FOO] from [SPAM::]BAR
             *  [\"personal_name\"] [\(HH:MM:SS\)]
             */
            nam = "VMSmail"; /* assume VMSmail */
            cmd = "MAIL";
            if (txt && (p = strrchr(txt, '(')) > txt && /* discard time */
                (--p, strspn(p, "0123456789( :.)") == strlen(p)))
                *p = '\0';
        } else if (!strncmpi(buf, "new all-in-1", 12)) {
            int i;
            /*
             * New ALL-IN-1 MAIL message [on node FOO] from Personal Name
             * \(BAR@SPAM\) [\(DD-MMM-YYYY HH:MM:SS\)]
             */
            nam = "A1mail";
            cmd = "A1M";
            if (txt && (p = strrchr(txt, '(')) > txt
                && /* discard date+time */
                sscanf(p - 1, " (%*d-%*[^-]-%*d %*d:%*d:%d) %c", &i,
                       &sentinel) == 1)
                *--p = '\0';
        } else if (!strncmpi(buf, "software tools", 14)) {
            /*
             * Software Tools mail has arrived on FOO from \'BAR\' [in SPAM]
             */
            nam = "STmail";
            cmd = "MSG";
            if (txt && (p = strstri(p, " in ")) != 0) /* specific folder */
                cmd = strcat(strcpy(cmd_buf, "MSG +"), p + 4);
        } else if (q - 2 >= buf && !strncmpi(q - 2, "mm", 2)) {
            /*
             * {MultiNet\ |PMDF\/}MM mail has arrived on FOO from BAR\n
             * [Subject: subject_text] (PMDF only)
             */
            nam = "MMmail"; /* MultiNet's version of MM */
            cmd = "MM";     /*{ perhaps "MM READ"? }*/
        } else if (!strncmpi(buf, "wpmail:", 7)) {
            /*
             * WPmail: New mail from BAR.  subject_text
             */
            nam = "WPmail"; /* WordPerfect [sic] Office */
            cmd = "OFFICE/MAIL";
        } else if (!strncmpi(buf, "**m400 mail", 7)) {
            /*
             * **M400 mail waiting**
             */
            nam = "M400mail"; /* Messenger 400 [not seen] */
            cmd = "M400";
        } else {
            /* not recognized, but presumed to be mail */
            nam = "unknown mail";
            cmd = "SPAWN";    /* generic escape back to DCL */
            txt = (char *) 0; /* don't rely on "from" info here */
        }

        if (!txt)
            txt = strcat(strcpy(txt_buf, "Mail for you: "), buf);

    /*
     * end of mail recognition; now check for call-type interruptions...
     */
    } else if ((q = strstri(buf, " phoning")) != 0) {
        /*
         * BAR is phoning you [on FOO] \(HH:MM:SS\)
         */
        typ = MSG_CALL;
        nam = "Phone call";
        cmd = "PHONE ANSWER";
        if (!strncmpi(q + 8, " you", 4))
            q += (8 + 4), *q = '\0';
        txt = strcat(strcpy(txt_buf, "Do you hear ringing?  "), buf);
    } else if ((q = strstri(buf, " talk-daemon")) != 0
               || (q = strstri(buf, " talk_daemon")) != 0) {
        /*
         * Message from TALK-DAEMON@FOO at HH:MM:SS\n
         * Connection request by BAR@SPAM\n
         * \[Respond with: TALK[/OLD] BAR@SPAM\]
         */
        typ = MSG_CALL;
        nam = "Talk request"; /* MultiNet's TALK and/or TALK/OLD */
        cmd = "TALK";
        if ((p = strstri(q, " by ")) != 0) {
            txt = strcat(strcpy(txt_buf, "Talk request from"), p + 3);
            if ((p = strstri(p, "respond with")) != 0) {
                if (*(p - 1) == '[')
                    *(p - 1) = '\0';
                else
                    *p = '\0'; /* terminate */
                p += (sizeof "respond with" - sizeof "");
                if (*p == ':')
                    p++;
                if (*p == ' ')
                    p++;
                cmd = strcpy(cmd_buf, p); /* "TALK[/OLD] bar@spam" */
                p = eos(cmd_buf);
                if (*--p == ']')
                    *p = '\0';
            }
        } else
            txt = strcat(strcpy(txt_buf, "Pardon the interruption: "), buf);
    } else if (is_jnet_send) { /* sscanf(,"(%[^)])%s -%c",,,)==3 */
    jnet_send:
        /*
         *      \(SPAM\)BAR - arbitrary_message_text (from BAR@SPAM)
         */
        typ = MSG_CALL;
        nam = "Bitnet noise"; /* RSCS/NJE message received via JNET */
        Sprintf(cmd_buf, "XYZZY %s@%s", user, node);
        cmd = cmd_buf;
        /*{ perhaps just vanilla SEND instead of XYZZY? }*/
        Sprintf(txt_buf, "Message from %s@%s:%s", user, node,
                /* "(node)user -" */
                &buf[1 + strlen(node) + 1 + strlen(user) + 2 - 1]);
        txt = txt_buf;

    /*
     * end of call recognition; anything else is none-of-the-above...
     */
    } else {
    other:
#endif  /* SHELL */
        /* arbitrary broadcast: batch job completed, system shutdown
         * imminent, &c
         */
        typ = MSG_OTHER;
        nam = (char *) 0; /*"captured broadcast message"*/
        cmd = (char *) 0;
        txt = strcat(strcpy(txt_buf, "Message for you: "), buf);
#ifdef SHELL
    }
    /* Daemon in newmail() will append period when the text is displayed */
    if ((p = eos(txt)) > txt && *--p == '.')
        *p = '\0';

    /* newmail() and readmail() used to assume that nam and cmd are
       concatenated but that is no longer the case */
    if (nam && nam != nam_buf) {
        (void) strncpy(nam_buf, nam, sizeof nam_buf - 1);
        nam_buf[sizeof nam_buf - 1] = '\0';
    }
    if (cmd && cmd != cmd_buf) {
        (void) strncpy(cmd_buf, cmd, sizeof cmd_buf - 1);
        cmd_buf[sizeof cmd_buf - 1] = '\0';
    }
#endif /* SHELL */
    /* truncate really long messages to prevent verbalize() from blowing up */
    if (txt && strlen(txt) > BUFSZ - 50)
        txt[BUFSZ - 50] = '\0';

    msgm.message_typ = typ;  /* simple index */
    msg.display_txt = txt;  /* text for daemon to pline() */
    msg.object_nam = nam;   /* 'name' for mail scroll */
    msgr.response_cmd = cmd; /* command to spawn when scroll read */
    return &msg;
}

/* filter out non-printable characters and redundant noise
*/
static void filter_brdcst(register char *buf) /* called by parse_next_broadcast() */
                                        /* in: original text; out: filtered text */
{
    register char c, *p, *buf_p;

    /* filter the text; restrict consecutive spaces or dots to just two */
    for (p = buf_p = buf; *buf_p; buf_p++) {
        c = *buf_p & '\177';
        if (c == ' ' || c == '\t' || c == '\n') {
            if (p == buf || /* ignore leading whitespace */
                (p >= buf + 2 && *(p - 1) == ' ' && *(p - 2) == ' '))
                continue;
            else
                c = ' ';
        } else if (c == '.' || c < ' ' || c == '\177') {
            if (p == buf || /* skip leading beeps & such */
                (p >= buf + 2 && *(p - 1) == '.' && *(p - 2) == '.'))
                continue;
            else
                c = '.';
        } else if (c == '%' && /* trim %%% OPCOM verbosity %%% */
                   p >= buf + 2 && *(p - 1) == '%' && *(p - 2) == '%') {
            continue;
        }
        *p++ = c;
    }
    *p = '\0'; /* terminate, then strip trailing junk */
    while (p > buf && (*--p == ' ' || *p == '.'))
        *p = '\0';
    return;
}

static char empty_string[] = "";

/* fetch the text of a captured broadcast, then mangle and decipher it
 */
struct mail_info *parse_next_broadcast(void) /* called by ckmailstatus(mail.c) */
{
    short length, msg_type;
    $DESCRIPTOR(message, empty_string); /* string descriptor for buf[] */
    struct mail_info *result = 0;
    /* messages could actually be longer; let long ones be truncated */
    char buf[255 + 1];

    message.dsc$a_pointer = buf, message.dsc$w_length = sizeof buf - 1;
    msg_type = length = 0;
    smg$get_broadcast_message(&pasteboard_id, &message, &length, &msg_type);
    if (msg_type == MSG$_TRMBRDCST) {
        buf[length] = '\0';
        filter_brdcst(buf);         /* mask non-printable characters */
        result = parse_brdcst(buf); /* do the real work */
    } else if (msg_type == MSG$_TRMHANGUP) {
        (void) gsignal(SIGHUP);
    }
    return result;
}

/* spit out any pending broadcast messages whenever we leave
 */
static void flush_broadcasts(void) /* called from disable_broadcast_trapping() */
{
    if (broadcasts > 0) {
        short len, typ;
        $DESCRIPTOR(msg_dsc, empty_string);
        char buf[512 + 1];

        msg_dsc.dsc$a_pointer = buf, msg_dsc.dsc$w_length = sizeof buf - 1;
        raw_print(""); /* print at least one line for wait_synch() */
        do {
            typ = len = 0;
            smg$get_broadcast_message(&pasteboard_id, &msg_dsc, &len, &typ);
            if (typ == MSG$_TRMBRDCST)
                buf[len] = '\0', raw_print(buf);
        } while (--broadcasts);
        wait_synch(); /* prompt with "Hit return to continue: " */
    }
}

/* AST routine called when terminal's associated mailbox receives a message
 */
/*ARGSUSED*/
static void
broadcast_ast(int dummy UNUSED) /* called asynchronously by terminal driver */
{
    broadcasts++;
}

/* initialize the broadcast manipulation code; SMG makes this easy
*/
unsigned long
init_broadcast_trapping(void) /* called by setftty() [once only] */
{
    unsigned long sts, preserve_screen_flag = 1;

    /* we need a pasteboard to pass to the broadcast setup/teardown routines */
    sts = smg$create_pasteboard(&pasteboard_id, 0, 0, 0,
                                &preserve_screen_flag);
    if (!vms_ok(sts)) {
        errno = EVMSERR, vaxc$errno = sts;
        raw_print("");
        perror("?can't create SMG pasteboard for broadcast trapping");
        wait_synch();
        broadcasts = -1; /* flag that trapping is currently broken */
    }
    return sts;
}

/* set up the terminal driver to deliver $brkthru data to a mailbox device
 */
unsigned long
enable_broadcast_trapping(void) /* called by setftty() */
{
    unsigned long sts = 1;

    if (broadcasts >= 0) { /* (-1 => no pasteboard, so don't even try) */
        /* register callback routine to be triggered when broadcasts arrive */
        /* Note side effect:  also intercepts hangup notification. */
        /* Another note:  TMPMBX privilege is required. */
        sts = smg$set_broadcast_trapping(&pasteboard_id, broadcast_ast, 0);
        if (!vms_ok(sts)) {
            errno = EVMSERR, vaxc$errno = sts;
            raw_print("");
            perror("?can't enable broadcast trapping");
            wait_synch();
        }
    }
    return sts;
}

/* return to 'normal'; $brkthru data goes straight to the terminal
 */
unsigned long
disable_broadcast_trapping(void) /* called by settty() */
{
    unsigned long sts = 1;

    if (broadcasts >= 0) {
        /* disable trapping; releases associated MBX so that SPAWN can work */
        sts = smg$disable_broadcast_trapping(&pasteboard_id);
        if (!vms_ok(sts))
            errno = EVMSERR, vaxc$errno = sts;
        flush_broadcasts(); /* don't hold on to any buffered ones */
    }
    return sts;
}

#else  /* MAIL */

/* simple stubs for non-mail configuration */
unsigned long
init_broadcast_trapping(void)
{
    return 1;
}
unsigned long
enable_broadcast_trapping(void)
{
    return 1;
}
unsigned long
disable_broadcast_trapping(void)
{
    return 1;
}
struct mail_info *
parse_next_broadcast(void)
{
    return 0;
}

#endif /* MAIL */

/*----------------------------------------------------------------------*/

#ifdef TEST_DRIVER
/* (Take parse_next_broadcast for a spin. :-) */

volatile int broadcasts = 0;

void
newmail(foo)
struct mail_info *foo;
{
#define STRING(s) ((s) ? (s) : "<null>")
    printf("\n\
  message type = %d\n\
  display text = \"%s\"\n\
  object name  = \"%.*s\"\n\
  response cmd = \"%s\"\n\
",
           foo->message_typ, STRING(foo->display_txt),
           (foo->object_nam && foo->response_cmd)
               ? (foo->response_cmd - foo->object_nam - 1)
               : strlen(STRING(foo->object_nam)),
           STRING(foo->object_nam), STRING(foo->response_cmd));
#undef STRING
}

void
ckmailstatus(void)
{
    struct mail_info *brdcst, *parse_next_broadcast();

    while (broadcasts > 0) { /* process all trapped broadcasts [until] */
        broadcasts--;
        if ((brdcst = parse_next_broadcast()) != 0) {
            newmail(brdcst);
            break; /* only handle one real message at a time */
        } else
            printf("\n--< non-broadcast encountered >--\n");
    }
}

int
main(int argc UNUSED, char *argv[] UNUSED)
{
    char dummy[BUFSIZ];

    init_broadcast_trapping();
    enable_broadcast_trapping();
    for (;;) {
        ckmailstatus();
        printf("> "), fflush(stdout); /* issue a prompt */
        if (!gets(dummy))
            break; /* wait for a response */
    }
    disable_broadcast_trapping();
    return 1;
}

void
panic(char *s)
{
    raw_print(s);
    exit(EXIT_FAILURE);
}

void
raw_print(char *s)
{
    puts(s);
    fflush(stdout);
}

void
wait_synch(void)
{
    char dummy[BUFSIZ];

    printf("\nPress <return> to continue: ");
    fflush(stdout);
    (void) gets(dummy);
}
#endif /* TEST_DRIVER */

/*vmsmail.c*/
