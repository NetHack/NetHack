/* NetHack 3.7	vmstty.c	$NHDT-Date: 1596498309 2020/08/03 23:45:09 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.21 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */
/* tty.c - (VMS) version */

#define NEED_VARARGS
#include "hack.h"
#include "wintty.h"
#include "tcap.h"

#include <descrip.h>
#include <iodef.h>
#ifndef __GNUC__
#include <smgdef.h>
#include <ttdef.h>
#include <tt2def.h>
#else /* values needed from missing include files */
#define SMG$K_TRM_UP 274
#define SMG$K_TRM_DOWN 275
#define SMG$K_TRM_LEFT 276
#define SMG$K_TRM_RIGHT 277
#define TT$M_MECHTAB 0x00000100     /* hardware tab support */
#define TT$M_MECHFORM 0x00080000    /* hardware form-feed support */
#define TT$M_NOBRDCST 0x00020000    /* disable broadcast messages, but  */
#define TT2$M_BRDCSTMBX 0x00000010  /* catch them in associated mailbox */
#define TT2$M_APP_KEYPAD 0x00800000 /* application vs numeric keypad mode */
#endif /* __GNUC__ */
#ifdef USE_QIO_INPUT
#include <ssdef.h>
#endif
#include <errno.h>
#include <signal.h>

unsigned long lib$disable_ctrl(), lib$enable_ctrl();
unsigned long sys$assign(), sys$dassgn(), sys$qiow();
#ifndef USE_QIO_INPUT
unsigned long smg$create_virtual_keyboard(), smg$delete_virtual_keyboard(),
    smg$read_keystroke(), smg$cancel_input();
#else
static short parse_function_key(int);
#endif
static void setctty(void);
static void resettty(void);

#define vms_ok(sts) ((sts) &1)
#define META(c) ((c) | 0x80) /* 8th bit */
#define CTRL(c) ((c) & 0x1F)
#define CMASK(c) (1 << CTRL(c))
#define LIB$M_CLI_CTRLT CMASK('T') /* 0x00100000 */
#define LIB$M_CLI_CTRLY CMASK('Y') /* 0x02000000 */
#define ESC '\033'
#define CSI META(ESC)       /* '\233' */
#define SS3 META(CTRL('O')) /* '\217' */

extern short ospeed;
char erase_char, intr_char, kill_char;
static boolean settty_needed = FALSE, bombing = FALSE;
static unsigned long kb = 0;
#ifdef USE_QIO_INPUT
static char inputbuf[15 + 1], *inp = 0;
static int inc = 0;
#endif

#define QIO_FUNC IO$_TTYREADALL | IO$M_NOECHO | IO$M_TRMNOECHO
#ifdef MAIL
#define TT_SPECIAL_HANDLING (TT$M_MECHTAB | TT$M_MECHFORM | TT$M_NOBRDCST)
#define TT2_SPECIAL_HANDLING (TT2$M_BRDCSTMBX)
#else
#define TT_SPECIAL_HANDLING (TT$M_MECHTAB | TT$M_MECHFORM)
#define TT2_SPECIAL_HANDLING (0)
#endif
#define Uword unsigned short
#define Ubyte unsigned char
struct _rd_iosb { /* i/o status block for read */
    Uword status, trm_offset;
    Uword terminator, trm_siz;
};
struct _wr_iosb { /* i/o status block for write */
    Uword status, byte_cnt;
    unsigned : 32;
};
struct _sm_iosb { /* i/o status block for sense-mode qio */
    Uword status;
    Ubyte xmt_speed, rcv_speed;
    Ubyte cr_fill, lf_fill, parity;
    unsigned : 8;
};
struct _sm_bufr {             /* sense-mode characteristics buffer */
    Ubyte class, type;        /* class==DC$_TERM, type==(various) */
    Uword buf_siz;            /* aka page width */
#define page_width buf_siz    /* number of columns */
    unsigned tt_char : 24;    /* primary characteristics */
    unsigned page_length : 8; /* number of lines */
    unsigned tt2_char : 32;   /* secondary characteristics */
};
static struct {
    struct _sm_iosb io;
    struct _sm_bufr sm;
} sg = { { 0 }, { 0 } };
static unsigned short tt_chan = 0;
static unsigned long tt_char_restore = 0, tt_char_active = 0,
                     tt2_char_restore = 0, tt2_char_active = 0;
static unsigned long ctrl_mask = 0;

#ifdef DEBUG
extern int nh_vms_getchar(void);

/* rename the real vms_getchar and interpose this one in front of it */
int
vms_getchar(void)
{
    static int althack = 0, altprefix;
    char *nhalthack;
    int res;

    if (!althack) {
        /* one-time init */
        nhalthack = nh_getenv("NH_ALTHACK");
        althack = nhalthack ? 1 : -1;
        if (althack > 0)
            altprefix = *nhalthack;
    }

#define vms_getchar nh_vms_getchar

    res = vms_getchar();
    if (althack > 0 && res == altprefix) {
        res = vms_getchar();
        if (res != ESC)
            res = META(res);
    }
    return res;
}
#endif /*DEBUG*/

int
vms_getchar(void)
{
    short key;
#ifdef USE_QIO_INPUT
    struct _rd_iosb iosb;
    unsigned long sts;
    unsigned char kb_buf;
#else /* SMG input */
    static volatile int recurse = 0; /* SMG is not AST re-entrant! */
#endif

    if (g.program_state.done_hup) {
        /* hangup has occurred; do not attempt to get further user input */
        return ESC;
    }

#ifdef USE_QIO_INPUT
    if (inc > 0) {
        /* we have buffered character(s) from previous read */
        kb_buf = *inp++;
        --inc;
        sts = SS$_NORMAL;
    } else {
        sts = sys$qiow(0, tt_chan, QIO_FUNC, &iosb, (void (*) ()) 0, 0,
                       &kb_buf, sizeof kb_buf, 0, 0, 0, 0);
    }
    if (vms_ok(sts)) {
        if (kb_buf == CTRL('C')) {
            if (intr_char)
                gsignal(SIGINT);
            key = (short) kb_buf;
        } else if (kb_buf == '\r') { /* <return> */
            key = (short) '\n';
        } else if (kb_buf == ESC || kb_buf == CSI || kb_buf == SS3) {
            switch (parse_function_key((int) kb_buf)) {
            case SMG$K_TRM_UP:
                key = g.Cmd.move_N;
                break;
            case SMG$K_TRM_DOWN:
                key = g.Cmd.move_S;
                break;
            case SMG$K_TRM_LEFT:
                key = g.Cmd.move_W;
                break;
            case SMG$K_TRM_RIGHT:
                key = g.Cmd.move_E;
                break;
            default:
                key = ESC;
                break;
            }
        } else {
            key = (short) kb_buf;
        }
    } else if (sts == SS$_HANGUP || iosb.status == SS$_HANGUP
               || sts == SS$_DEVOFFLINE) {
        gsignal(SIGHUP);
        key = ESC;
    } else /*(this should never happen)*/
        key = getchar();

#else  /*!USE_QIO_INPUT*/
    if (recurse++ == 0 && kb != 0) {
        smg$read_keystroke(&kb, &key);
        switch (key) {
        case SMG$K_TRM_UP:
            key = g.Cmd.move_N;
            break;
        case SMG$K_TRM_DOWN:
            key = g.Cmd.move_S;
            break;
        case SMG$K_TRM_LEFT:
            key = g.Cmd.move_W;
            break;
        case SMG$K_TRM_RIGHT:
            key = g.Cmd.move_E;
            break;
        case '\r':
            key = '\n';
            break;
        default:
            if (key > 255)
                key = ESC;
            break;
        }
    } else {
        /* abnormal input--either SMG didn't initialize properly or
         * vms_getchar() has been called recursively (via SIGINT handler).
         */
        if (kb != 0)               /* must have been a recursive call */
            smg$cancel_input(&kb); /*  from an interrupt handler      */
        key = getchar();
    }
    --recurse;
#endif /* USE_QIO_INPUT */

    return (int) key;
}

#ifdef USE_QIO_INPUT
/*
 * We've just gotten an <escape> character.  Do a timed read to
 * get any other characters, then try to parse them as an escape
 * sequence.  This isn't perfect, since there's no guarantee
 * that a full escape sequence will be available, or even if one
 * is, it might actually by regular input from a fast typist or
 * a stalled input connection.  {For packetized environments,
 * cross plural(body_part(FINGER)) and hope for best. :-}
 *
 * This is needed to preserve compatibility with SMG interface
 * for two reasons:
 *    1) retain support for arrow keys, and
 *    2) treat other VTxxx function keys as <esc> for aborting
 *      various NetHack prompts.
 * The second reason is compelling; otherwise remaining chars of
 * an escape sequence get treated as inappropriate user commands.
 *
 * SMG code values for these key sequences fall in the range of
 * 256 thru 3xx.  The assignments are not particularly intuitive.
 */
/*=
     -- Summary of VTxxx-style keyboards and transmitted escape sequences. --
Keypad codes are prefixed by 7 bit (\033 O) or 8 bit SS3:
        keypad:  PF1 PF2 PF3 PF4       codes:   P   Q   R   S
                  7   8   9   -                 w   x   y   m
                  4   5   6   .                 t   u   v   n
                  1   2   3  :en-:              q   r   s  : :
                 ...0...  ,  :ter:             ...p...  l  :M:
Arrows are prefixed by either SS3 or CSI (either 7 or 8 bit), depending on
whether the terminal is in application or numeric mode (ditto for PF keys):
        arrows: <up> <dwn> <lft> <rgt>          A   B   D   C
Additional function keys (vk201/vk401) generate CSI nn ~ (nn is 1 or 2 digits):
    vk201 keys:  F6 F7 F8 F9 F10   F11 F12 F13 F14  Help Do   F17 F18 F19 F20
   'nn' digits:  17 18 19 20 21    23  24  25  26    28  29   31  32  33  34
     alternate:  ^C                ^[  ^H  ^J           (when in VT100 mode)
   edit keypad: <fnd> <ins> <rmv>     digits:   1   2   3
                <sel> <prv> <nxt>               4   5   6
VT52 mode:  arrows and PF keys send ESCx where x is in A-D or P-S.
=*/

static const char *arrow_or_PF = "ABCDPQRS", /* suffix char */
                  *smg_keypad_codes = "PQRSpqrstuvwxyMmlnABDC";
/* PF1..PF4,KP0..KP9,enter,dash,comma,dot,up-arrow,down,left,right */
/* Ultimate return value is (index into smg_keypad_codes[] + 256). */

static short
parse_function_key(register int c)
{
    struct _rd_iosb iosb;
    unsigned long sts;
    char seq_buf[15 + 1]; /* plenty room for escape sequence + slop */
    short result = ESC;   /* translate to <escape> by default */

    /*
     * Read whatever we can from type-ahead buffer (1 second timeout).
     * If the user typed an actual <escape> to deliberately abort
     * something, he or she should be able to tolerate the necessary
     * restriction of a negligible pause before typing anything else.
     * We might already have [at least some of] an escape sequence from a
     * previous read, particularly if user holds down the arrow keys...
     */
    if (inc > 0)
        strncpy(seq_buf, inp, inc);
    if (inc < (int) (sizeof seq_buf) - 1) {
        sts = sys$qiow(0, tt_chan, QIO_FUNC | IO$M_TIMED, &iosb,
                       (void (*) ()) 0, 0, seq_buf + inc,
                       sizeof seq_buf - 1 - inc, 1, 0, 0, 0);
        if (vms_ok(sts))
            sts = iosb.status;
    } else
        sts = SS$_NORMAL;
    if (vms_ok(sts) || sts == SS$_TIMEOUT) {
        register int cnt = iosb.trm_offset + iosb.trm_siz + inc;
        register char *p = seq_buf;

        if (c == ESC) /* check for 7-bit vt100/ANSI, or vt52 */
            if (*p == '[' || *p == 'O')
                c = META(CTRL(*p++)), cnt--;
            else if (strchr(arrow_or_PF, *p))
                c = SS3; /*CSI*/
        if (cnt > 0 && (c == SS3 || (c == CSI && strchr(arrow_or_PF, *p)))) {
            register char *q = strchr(smg_keypad_codes, *p);

            if (q)
                result = 256 + (q - smg_keypad_codes);
            p++, --cnt; /* one more char consumed */
        } else if (cnt > 1 && c == CSI) {
            static short /* "CSI nn ~" -> F_keys[nn] */
                F_keys[35] = {
                    ESC,                          /*(filler)*/
                    311, 312, 313, 314, 315, 316, /* E1-E6 */
                    ESC, ESC, ESC, ESC,           /*(more filler)*/
                    281, 282, 283, 284, 285, ESC, /* F1-F5 */
                    286, 287, 288, 289, 290, ESC, /* F6-F10*/
                    291, 292, 293, 294, ESC,      /*F11-F14*/
                    295, 296, ESC,                /*<help>,<do>, aka F15,F16*/
                    297, 298, 299, 300            /*F17-F20*/
                }; /* note: there are several missing nn in CSI nn ~ values */
            int nn;
            char *q;

            *(p + cnt) = '\0'; /* terminate string */
            q = strchr(p, '~');
            if (q && sscanf(p, "%d~", &nn) == 1) {
                if (nn > 0 && nn < SIZE(F_keys))
                    result = F_keys[nn];
                cnt -= (++q - p);
                p = q;
            }
        }
        if (cnt > 0)
            strncpy((inp = inputbuf), p, (inc = cnt));
        else
            inc = 0, inp = 0;
    }
    return result;
}
#endif /* USE_QIO_INPUT */

static void
setctty(void)
{
    struct _sm_iosb iosb;
    unsigned long status;

    status = sys$qiow(0, tt_chan, IO$_SETMODE, &iosb, (void (*) ()) 0, 0,
                      &sg.sm, sizeof sg.sm, 0, 0, 0, 0);
    if (vms_ok(status))
        status = iosb.status;
    if (vms_ok(status)) {
        /* try to force terminal into synch with TTDRIVER's setting */
        number_pad((sg.sm.tt2_char & TT2$M_APP_KEYPAD) ? -1 : 1);
    } else {
        raw_print("");
        errno = EVMSERR, vaxc$errno = status;
        perror("NetHack(setctty: setmode)");
        wait_synch();
    }
}

/* atexit() routine */
static void
resettty(void)
{
    if (settty_needed) {
        bombing = TRUE; /* don't clear screen; preserve traceback info */
        settty((char *) 0);
    }
    (void) sys$dassgn(tt_chan), tt_chan = 0;
}

/*
 * Get initial state of terminal, set ospeed (for termcap routines)
 * and switch off tab expansion if necessary.
 * Called by init_nhwindows() and resume_nhwindows() in wintty.c
 * (for initial startup and for returning from '!' or ^Z).
 */
void
gettty(void)
{
    static char dev_tty[] = "TT:";
    static $DESCRIPTOR(tty_dsc, dev_tty);
    int err = 0;
    unsigned long status, zero = 0;

    if (tt_chan == 0) {                        /* do this stuff once only */
        iflags.cbreak = OFF, iflags.echo = ON; /* until setup is complete */
        status = sys$assign(&tty_dsc, &tt_chan, 0, 0);
        if (!vms_ok(status)) {
            raw_print(""), err++;
            errno = EVMSERR, vaxc$errno = status;
            perror("NetHack(gettty: $assign)");
        }
        atexit(resettty); /* register an exit handler to reset things */
    }
    status = sys$qiow(0, tt_chan, IO$_SENSEMODE, &sg.io, (void (*) ()) 0, 0,
                      &sg.sm, sizeof sg.sm, 0, 0, 0, 0);
    if (vms_ok(status))
        status = sg.io.status;
    if (!vms_ok(status)) {
        raw_print(""), err++;
        errno = EVMSERR, vaxc$errno = status;
        perror("NetHack(gettty: sensemode)");
    }
    ospeed = sg.io.xmt_speed;
    erase_char = '\177'; /* <rubout>, aka <delete> */
    kill_char = CTRL('U');
    intr_char = CTRL('C');
    (void) lib$enable_ctrl(&zero, &ctrl_mask);
    /* Use the systems's values for lines and columns if it has any idea. */
    if (sg.sm.page_length)
        LI = sg.sm.page_length;
    if (sg.sm.page_width)
        CO = sg.sm.page_width;
    /* suppress tab and form-feed expansion, in case termcap uses them */
    tt_char_restore = sg.sm.tt_char;
    tt_char_active = sg.sm.tt_char |= TT_SPECIAL_HANDLING;
    tt2_char_restore = sg.sm.tt2_char;
    tt2_char_active = sg.sm.tt2_char |= TT2_SPECIAL_HANDLING;
#if 0 /*[ defer until setftty() ]*/
    setctty();
#endif

    if (err)
        wait_synch();
}

/* reset terminal to original state */
void
settty(const char *s)
{
    if (!bombing)
        end_screen();
    if (s)
        raw_print(s);
    if (settty_needed) {
        disable_broadcast_trapping();
#if 0  /* let SMG's exit handler do the cleanup (as per doc) */
/* #ifndef USE_QIO_INPUT */
        if (kb)
            smg$delete_virtual_keyboard(&kb),  kb = 0;
#endif /* 0 (!USE_QIO_INPUT) */
        if (ctrl_mask)
            (void) lib$enable_ctrl(&ctrl_mask, 0);
        iflags.echo = ON;
        iflags.cbreak = OFF;
        /* reset original tab, form-feed, broadcast settings */
        sg.sm.tt_char = tt_char_restore;
        sg.sm.tt2_char = tt2_char_restore;
        setctty();

        settty_needed = FALSE;
    }
}

/* same as settty, with no clearing of the screen */
void
shuttty(const char *s)
{
    bombing = TRUE;
    settty(s);
    bombing = FALSE;
}

void
setftty(void)
{
    unsigned long mask = LIB$M_CLI_CTRLT | LIB$M_CLI_CTRLY;

    (void) lib$disable_ctrl(&mask, 0);
    if (kb == 0) { /* do this stuff once only */
#ifdef USE_QIO_INPUT
        kb = tt_chan;
#else  /*!USE_QIO_INPUT*/
        smg$create_virtual_keyboard(&kb);
#endif /*USE_QIO_INPUT*/
        init_broadcast_trapping();
    }
    enable_broadcast_trapping(); /* no-op if !defined(MAIL) */
    iflags.cbreak = (kb != 0) ? ON : OFF;
    iflags.echo = (kb != 0) ? OFF : ON;
    /* disable tab & form-feed expansion; prepare for broadcast trapping */
    sg.sm.tt_char = tt_char_active;
    sg.sm.tt2_char = tt2_char_active;
    setctty();

    start_screen();
    settty_needed = TRUE;
}

/* enable kbd interupts if enabled when game started */
void
intron(void)
{
    intr_char = CTRL('C');
}

/* disable kbd interrupts if required*/
void
introff(void)
{
    intr_char = 0;
}

#ifdef TIMED_DELAY

extern unsigned long lib$emul(const long *, const long *, const long *,
                              long *);
extern unsigned long sys$schdwk(), sys$hiber();

#define VMS_UNITS_PER_SECOND 10000000L /* hundreds of nanoseconds, 1e-7 */
/* constant for conversion from milliseconds to VMS delta time (negative) */
static const long mseconds_to_delta = VMS_UNITS_PER_SECOND / 1000L * -1L;

/* sleep for specified number of milliseconds (note: the timer used
   generally only has 10-millisecond resolution at the hardware level...) */
void
msleep(unsigned mseconds) /* milliseconds */
{
    long pid = 0L, zero = 0L, msec, qtime[2];

    msec = (long) mseconds;
    if (msec > 0 &&
        /* qtime{0:63} = msec{0:31} * mseconds_to_delta{0:31} + zero{0:31} */
        vms_ok(lib$emul(&msec, &mseconds_to_delta, &zero, qtime))) {
        /* schedule a wake-up call, then go to sleep */
        if (vms_ok(sys$schdwk(&pid, (genericptr_t) 0, qtime, (long *) 0)))
            (void) sys$hiber();
    }
}

#endif /* TIMED_DELAY */

/* fatal error */
/*VARARGS1*/
void error
VA_DECL(const char *, s)
{
    VA_START(s);
    VA_INIT(s, const char *);

    if (settty_needed)
        settty((char *) 0);
    Vprintf(s, VA_ARGS);
    (void) putchar('\n');
    VA_END();
#ifndef SAVE_ON_FATAL_ERROR
    /* prevent vmsmain's exit handler byebye() from calling hangup() */
    sethanguphandler((void (*)(int) )) SIG_DFL;
#endif
    exit(EXIT_FAILURE);
}
