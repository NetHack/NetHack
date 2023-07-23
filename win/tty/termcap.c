/* NetHack 3.7	termcap.c	$NHDT-Date: 1609459769 2021/01/01 00:09:29 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.41 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Pasi Kallinen, 2018. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#if defined(TTY_GRAPHICS) && !defined(NO_TERMS)

#include "wintty.h"
#include "tcap.h"

#define Tgetstr(key) (tgetstr(key, &tbufptr))

static char *s_atr2str(int);
static char *e_atr2str(int);

void cmov(int, int);
void nocmov(int, int);
void term_start_24bitcolor(struct unicode_representation *);
void term_end_24bitcolor(void);

#if defined(TEXTCOLOR) && defined(TERMLIB)
#if (!defined(UNIX) || !defined(TERMINFO)) && !defined(TOS)
static void analyze_seq(char *, int *, int *);
#endif
#endif

/* (see tcap.h) -- nh_CM, nh_ND, nh_CD, nh_HI,nh_HE, nh_US,nh_UE, ul_hack */
struct tc_lcl_data tc_lcl_data = { 0, 0, 0, 0, 0, 0, 0, FALSE };

static char *HO, *CL, *CE, *UP, *XD, *BC, *SE, *TI, *TE;
static char *VS, *VE;
static char *ME, *MR, *MB, *MH;
const char *SO, *MD;
static char *ZH, *ZR;

#ifdef TERMLIB
boolean dynamic_HIHE = FALSE;
static int SG;
static char PC = '\0';
static char tbuf[512];
#endif /*TERMLIB*/

static char *KS = (char *) 0, *KE = (char *) 0; /* keypad sequences */
static char nullstr[] = "";

#if defined(ASCIIGRAPH) && !defined(NO_TERMS)
extern boolean HE_resets_AS;
#endif

#ifndef TERMLIB
static char tgotobuf[20];
#ifdef TOS
#define tgoto(fmt, x, y) (Sprintf(tgotobuf, fmt, y + ' ', x + ' '), tgotobuf)
#else
#define tgoto(fmt, x, y) (Sprintf(tgotobuf, fmt, y + 1, x + 1), tgotobuf)
#endif
#endif /* TERMLIB */

/* these don't need to be part of 'struct instance_globals g' */
static char tty_standout_on[16], tty_standout_off[16];

void
tty_startup(int *wid, int *hgt)
{
#ifdef TERMLIB
    register const char *term;
    register char *tptr;
    char *tbufptr, *pc;
    int i;

#ifdef VMS
    term = verify_termcap();
    if (!term)
#endif
        term = getenv("TERM");

#if defined(TOS) && defined(__GNUC__)
    if (!term)
        term = "builtin"; /* library has a default */
#endif
    if (!term)
#endif /* TERMLIB */
#ifndef ANSI_DEFAULT
        error("Can't get TERM.");
#else
#ifdef TOS
    {
        CO = 80;
        LI = 25;
        TI = VS = VE = TE = nullstr;
        /*
         * FIXME:  These variables ought to be declared 'const' (instead
         * of using nhStr() to cast away const) to avoid '-Wwrite-sttings'
         * warnings about assigning string literals to them.
         */
        HO = nhStr("\033H");
        CE = nhStr("\033K"); /* the VT52 termcap */
        UP = nhStr("\033A");
        nh_CM = nhStr("\033Y%c%c"); /* used with function tgoto() */
        nh_ND = nhStr("\033C");
        XD = nhStr("\033B");
        BC = nhStr("\033D");
        SO = nhStr("\033p");
        SE = nhStr("\033q");
        /* HI and HE will be updated in init_hilite if we're using color */
        nh_HI = nhStr("\033p");
        nh_HE = nhStr("\033q");
        *wid = CO;
        *hgt = LI;
        CL = nhStr("\033E"); /* last thing set */
        return;
    }
#else /* TOS */
    {
#ifdef MICRO
        get_scr_size();
#ifdef CLIPPING
        if (CO < COLNO || LI < ROWNO + 3)
            setclipped();
#endif
#endif
        HO = nhStr("\033[H");
        /*              nh_CD = nhStr("\033[J"); */
        CE = nhStr("\033[K"); /* the ANSI termcap */
#ifndef TERMLIB
        nh_CM = nhStr("\033[%d;%dH");
#else
        nh_CM = nhStr("\033[%i%d;%dH");
#endif
        UP = nhStr("\033[A");
        nh_ND = nhStr("\033[C");
        XD = nhStr("\033[B");
#ifdef MICRO /* backspaces are non-destructive */
        BC = nhStr("\b");
#else
        BC = nhStr("\033[D");
#endif
        nh_HI = SO = nhStr("\033[1m");
        nh_US = nhStr("\033[4m");
        MR = nhStr("\033[7m");
        TI = nh_HE = ME = SE = nh_UE = nhStr("\033[0m");
        /* strictly, SE should be 2, and nh_UE should be 24,
           but we can't trust all ANSI emulators to be
           that complete.  -3. */
#ifndef MICRO
        AS = nhStr("\016");
        AE = nhStr("\017");
#endif
        TE = VS = VE = nullstr;
#ifdef TEXTCOLOR
        init_hilite();
#endif /* TEXTCOLOR */
        *wid = CO;
        *hgt = LI;
        CL = nhStr("\033[2J"); /* last thing set */
        return;
    }
#endif /* TOS */
#endif /* ANSI_DEFAULT */

#ifdef TERMLIB
    tptr = (char *) alloc(1024);

    tbufptr = tbuf;
    if (!strncmp(term, "5620", 4))
        flags.null = FALSE; /* this should be a termcap flag */
    if (tgetent(tptr, term) < 1) {
        char buf[BUFSZ];
        (void) strncpy(buf, term,
                       (BUFSZ - 1) - (sizeof("Unknown terminal type: .  ")));
        buf[BUFSZ - 1] = '\0';
        error("Unknown terminal type: %s.", term);
    }
    if ((pc = Tgetstr(nhStr("pc"))) != 0)
        PC = *pc;

    if (!(BC = Tgetstr(nhStr("le")))) { /* both termcap and terminfo use le */
#ifdef TERMINFO
        error("Terminal must backspace.");
#else
        if (!(BC = Tgetstr(nhStr("bc")))) { /* termcap also uses bc/bs */
#ifndef MINIMAL_TERM
            if (!tgetflag(nhStr("bs")))
                error("Terminal must backspace.");
#endif
            BC = tbufptr;
            tbufptr += 2;
            *BC = '\b';
        }
#endif
    }

#ifdef MINIMAL_TERM
    HO = (char *) 0;
#else
    HO = Tgetstr(nhStr("ho"));
#endif
    /*
     * LI and CO are set in ioctl.c via a TIOCGWINSZ if available.  If
     * the kernel has values for either we should use them rather than
     * the values from TERMCAP ...
     */
#ifndef MICRO
    if (!CO)
        CO = tgetnum(nhStr("co"));
    if (!LI)
        LI = tgetnum(nhStr("li"));
#else
#if defined(TOS) && defined(__GNUC__)
    if (!strcmp(term, "builtin")) {
        get_scr_size();
    } else
#endif
    {
        CO = tgetnum(nhStr("co"));
        LI = tgetnum(nhStr("li"));
        if (!LI || !CO) /* if we don't override it */
            get_scr_size();
    }
#endif /* ?MICRO */
#ifdef CLIPPING
    if (CO < COLNO || LI < ROWNO + 3)
        setclipped();
#endif
    nh_ND = Tgetstr(nhStr("nd")); /* move cursor right 1 column */
    if (tgetflag(nhStr("os"))) /* term can overstrike */
        error("NetHack can't have OS.");
    if (tgetflag(nhStr("ul"))) /* underline by overstrike w/ underscore */
        ul_hack = TRUE;
    CE = Tgetstr(nhStr("ce")); /* clear line from cursor to eol */
    UP = Tgetstr(nhStr("up")); /* move cursor up 1 line */
    /* It seems that xd is no longer supported, and we should use
       a linefeed instead; unfortunately this requires resetting
       CRMOD, and many output routines will have to be modified
       slightly. Let's leave that till the next release. */
    XD = Tgetstr(nhStr("xd"));
    /* not:             XD = Tgetstr("do"); */
    if (!(nh_CM = Tgetstr(nhStr("cm")))) { /* cm: move cursor */
        if (!UP && !HO)
            error("NetHack needs CM or UP or HO.");
        tty_raw_print("Playing NetHack on terminals without CM is suspect.");
        tty_wait_synch();
    }
    SO = Tgetstr(nhStr("so")); /* standout start */
    SE = Tgetstr(nhStr("se")); /* standout end */
    nh_US = Tgetstr(nhStr("us")); /* underline start */
    nh_UE = Tgetstr(nhStr("ue")); /* underline end */
    ZH = Tgetstr(nhStr("ZH")); /* italic start */
    ZR = Tgetstr(nhStr("ZR")); /* italic end */
    SG = tgetnum(nhStr("sg")); /* -1: not fnd; else # of spaces left by so */
    if (!SO || !SE || (SG > 0))
        SO = SE = nh_US = nh_UE = nullstr;
    TI = Tgetstr(nhStr("ti")); /* nonconsequential cursor movement start */
    TE = Tgetstr(nhStr("te")); /* nonconsequential cursor movement end */
    VS = VE = nullstr;
#ifdef TERMINFO
    VS = Tgetstr(nhStr("eA")); /* enable graphics */
#endif
    KS = Tgetstr(nhStr("ks")); /* keypad start (special mode) */
    KE = Tgetstr(nhStr("ke")); /* keypad end (ordinary mode [ie, digits]) */
    MR = Tgetstr(nhStr("mr")); /* reverse */
    MB = Tgetstr(nhStr("mb")); /* blink */
    MD = Tgetstr(nhStr("md")); /* boldface */
    if (!SO)
        SO = MD;
    MH = Tgetstr(nhStr("mh")); /* dim */
    ME = Tgetstr(nhStr("me")); /* turn off all attributes */
    if (!ME)
        ME = SE ? SE : nullstr; /* default to SE value */

    /* Get rid of padding numbers for nh_HI and nh_HE.  Hope they
     * aren't really needed!!!  nh_HI and nh_HE are outputted to the
     * pager as a string - so how can you send it NULs???
     *  -jsb
     */
    for (i = 0; digit(SO[i]); ++i)
        continue;
    nh_HI = dupstr(&SO[i]);
    for (i = 0; digit(ME[i]); ++i)
        continue;
    nh_HE = dupstr(&ME[i]);
    dynamic_HIHE = TRUE;

    AS = Tgetstr(nhStr("as")); /* alt charset start */
    AE = Tgetstr(nhStr("ae")); /* alt charset end */
    nh_CD = Tgetstr(nhStr("cd")); /* clear lines from cursor and down */
#ifdef TEXTCOLOR
#if defined(TOS) && defined(__GNUC__)
    if (!strcmp(term, "builtin") || !strcmp(term, "tw52")
        || !strcmp(term, "st52")) {
        init_hilite();
    }
#else
    init_hilite();
#endif
#endif
    *wid = CO;
    *hgt = LI;
    /* cl: clear screen, set cursor to upper left */
    if (!(CL = Tgetstr(nhStr("cl")))) /* last thing set */
        error("NetHack needs CL.");
    if ((int) (tbufptr - tbuf) > (int) (sizeof tbuf))
        error("TERMCAP entry too big...\n");
    free((genericptr_t) tptr);
#endif /* TERMLIB */
    /* keep static copies of these so that raw_print_bold() will work
       after exit_nhwindows(); if the sequences are too long, then bold
       won't work after that--it will be rendered as ordinary text */
    if (nh_HI && strlen(nh_HI) < sizeof tty_standout_on)
        Strcpy(tty_standout_on, nh_HI);
    if (nh_HE && strlen(nh_HE) < sizeof tty_standout_off)
        Strcpy(tty_standout_off, nh_HE);
}

void
tty_shutdown(void)
{
    reset_palette();
}

void
tty_number_pad(int state)
{
    switch (state) {
    case -1: /* activate keypad mode (escape sequences) */
        if (KS && *KS)
            xputs(KS);
        break;
    case 1: /* activate numeric mode for keypad (digits) */
        if (KE && *KE)
            xputs(KE);
        break;
    case 0: /* don't need to do anything--leave terminal as-is */
    default:
        break;
    }
}

#ifdef TERMLIB
extern void (*decgraphics_mode_callback)(void); /* defined in symbols.c */
static void tty_decgraphics_termcap_fixup(void);

/*
   We call this routine whenever DECgraphics mode is enabled, even if it
   has been previously set, in case the user manages to reset the fonts.
   The actual termcap fixup only needs to be done once, but we can't
   call xputs() from the option setting or graphics assigning routines,
   so this is a convenient hook.
 */
static void
tty_decgraphics_termcap_fixup(void)
{
    static char ctrlN[] = "\016";
    static char ctrlO[] = "\017";
    static char appMode[] = "\033=";
    static char numMode[] = "\033>";

    /* these values are missing from some termcaps */
    if (!AS)
        AS = ctrlN; /* ^N (shift-out [graphics font]) */
    if (!AE)
        AE = ctrlO; /* ^O (shift-in  [regular font])  */
    if (!KS)
        KS = appMode; /* ESC= (application keypad mode) */
    if (!KE)
        KE = numMode; /* ESC> (numeric keypad mode) */
    /*
     * Select the line-drawing character set as the alternate font.
     * Do not select NA ASCII as the primary font since people may
     * reasonably be using the UK character set.
     */
    if (SYMHANDLING(H_DEC))
        xputs("\033)0");

#if defined(ASCIIGRAPH) && !defined(NO_TERMS)
    /* some termcaps suffer from the bizarre notion that resetting
       video attributes should also reset the chosen character set */
    {
        const char *nh_he = nh_HE, *ae = AE;
        int he_limit, ae_length;

        if (digit(*ae)) { /* skip over delay prefix, if any */
            do
                ++ae;
            while (digit(*ae));
            if (*ae == '.') {
                ++ae;
                if (digit(*ae))
                    ++ae;
            }
            if (*ae == '*')
                ++ae;
        }
        /* can't use nethack's case-insensitive strstri() here, and some old
           systems don't have strstr(), so use brute force substring search */
        ae_length = strlen(ae), he_limit = strlen(nh_he);
        while (he_limit >= ae_length) {
            if (strncmp(nh_he, ae, ae_length) == 0) {
                HE_resets_AS = TRUE;
                break;
            }
            ++nh_he, --he_limit;
        }
    }
#endif
}
#endif /* TERMLIB */

extern void (*utf8graphics_mode_callback)(void); /* defined in symbols.c */

void
tty_start_screen(void)
{
    xputs(TI);
    xputs(VS);

#ifdef TERMLIB
    if (SYMHANDLING(H_DEC))
        tty_decgraphics_termcap_fixup();
    /* set up callback in case option is not set yet but toggled later */
    decgraphics_mode_callback = tty_decgraphics_termcap_fixup;
#endif
#ifdef ENHANCED_SYMBOLS
    utf8graphics_mode_callback = tty_utf8graphics_fixup;
#endif

    if (gc.Cmd.num_pad)
        tty_number_pad(1); /* make keypad send digits */
}

void
tty_end_screen(void)
{
    term_clear_screen();
    xputs(VE);
    xputs(TE);
}

/* Cursor movements */

/* Note to overlay tinkerers.  The placement of this overlay controls the
   location of the function xputc().  This function is not currently in
   trampoli.[ch] files for what is deemed to be performance reasons.  If
   this define is moved and or xputc() is taken out of the ROOT overlay,
   then action must be taken in trampoli.[ch]. */

void
nocmov(int x, int y)
{
    if ((int) ttyDisplay->cury > y) {
        if (UP) {
            while ((int) ttyDisplay->cury > y) { /* Go up. */
                xputs(UP);
                ttyDisplay->cury--;
            }
        } else if (nh_CM) {
            cmov(x, y);
        } else if (HO) {
            home();
            tty_curs(BASE_WINDOW, x + 1, y);
        } /* else impossible("..."); */
    } else if ((int) ttyDisplay->cury < y) {
        if (XD) {
            while ((int) ttyDisplay->cury < y) {
                xputs(XD);
                ttyDisplay->cury++;
            }
        } else if (nh_CM) {
            cmov(x, y);
        } else {
            while ((int) ttyDisplay->cury < y) {
                (void) xputc('\n');
                ttyDisplay->curx = 0;
                ttyDisplay->cury++;
            }
        }
    }
    if ((int) ttyDisplay->curx < x) { /* Go to the right. */
        if (!nh_ND) {
            cmov(x, y);
        } else { /* bah */
             /* should instead print what is there already */
            while ((int) ttyDisplay->curx < x) {
                xputs(nh_ND);
                ttyDisplay->curx++;
            }
        }
    } else if ((int) ttyDisplay->curx > x) {
        while ((int) ttyDisplay->curx > x) { /* Go to the left. */
            xputs(BC);
            ttyDisplay->curx--;
        }
    }
}

void
cmov(register int x, register int y)
{
    xputs(tgoto(nh_CM, x, y));
    ttyDisplay->cury = y;
    ttyDisplay->curx = x;
}

/* See note above.  xputc() is a special function for overlays. */
int
xputc(int c) /* actually char, but explicitly specify its widened type */
{
    /*
     * Note:  xputc() as a direct all to putchar() doesn't make any
     * sense _if_ putchar() is a function.  But if it is a macro, an
     * overlay configuration would want to avoid hidden code bloat
     * from multiple putchar() expansions.  And it gets passed as an
     * argument to tputs() so we have to guarantee an actual function
     * (while possibly lacking ANSI's (func) syntax to override macro).
     *
     * xputc() used to be declared as 'void xputc(c) char c; {}' but
     * avoiding the proper type 'int' just to avoid (void) casts when
     * ignoring the result can't have been sufficent reason to add it.
     * It also had '#if apollo' conditional to have the arg be int.
     * Matching putchar()'s declaration and using explicit casts where
     * warranted is more robust, so we're just a jacket around that.
     */
    return putchar(c);
}

void
xputs(const char *s)
{
#ifndef TERMLIB
    (void) fputs(s, stdout);
#else
    tputs(s, 1, xputc);
#endif
}

void
cl_end(void)
{
    if (CE) {
        xputs(CE);
    } else { /* no-CE fix - free after Harold Rynes */
        register int cx = ttyDisplay->curx + 1;

        /* this looks terrible, especially on a slow terminal
           but is better than nothing */
        while (cx < CO) {
            (void) xputc(' ');
            cx++;
        }
        tty_curs(BASE_WINDOW, (int) ttyDisplay->curx + 1,
                 (int) ttyDisplay->cury);
    }
}

void
term_clear_screen(void)
{
    /* note: if CL is null, then termcap initialization failed,
            so don't attempt screen-oriented I/O during final cleanup.
     */
    if (CL) {
        xputs(CL);
        home();
    }
}

void
home(void)
{
    if (HO)
        xputs(HO);
    else if (nh_CM)
        xputs(tgoto(nh_CM, 0, 0));
    else
        tty_curs(BASE_WINDOW, 1, 0); /* using UP ... */
    ttyDisplay->curx = ttyDisplay->cury = 0;
}

void
standoutbeg(void)
{
    if (SO)
        xputs(SO);
}

void
standoutend(void)
{
    if (SE)
        xputs(SE);
}

#if 0 /* if you need one of these, uncomment it (here and in extern.h) */
void
revbeg(void)
{
    if (MR)
        xputs(MR);
}

void
boldbeg(void)
{
    if (MD)
        xputs(MD);
}

void
blinkbeg(void)
{
    if (MB)
        xputs(MB);
}

void
dimbeg(void)
{
    /* not in most termcap entries */
    if (MH)
        xputs(MH);
}

void
m_end(void)
{
    if (ME)
        xputs(ME);
}
#endif /*0*/

void
backsp(void)
{
    xputs(BC);
}

void
tty_nhbell(void)
{
    if (flags.silent)
        return;
    (void) putchar('\007'); /* curx does not change */
    (void) fflush(stdout);
}

#ifdef ASCIIGRAPH
void
graph_on(void)
{
    if (AS)
        xputs(AS);
}

void
graph_off(void)
{
    if (AE)
        xputs(AE);
}
#endif

#if !defined(MICRO)
#ifdef VMS
static const short tmspc10[] = { /* from termcap */
                                 0, 2000, 1333, 909, 743, 666, 333, 166, 83,
                                 55, 50, 41, 27, 20, 13, 10, 5
};
#else
static const short tmspc10[] = { /* from termcap */
                                 0, 2000, 1333, 909, 743, 666, 500, 333, 166,
                                 83, 55, 41, 20, 10, 5
};
#endif
#endif

/* delay 50 ms */
void
tty_delay_output(void)
{
#if defined(MICRO)
    register int i;
#endif
    if (iflags.debug_fuzzer)
        return;
#ifdef TIMED_DELAY
    if (flags.nap) {
        (void) fflush(stdout);
        msleep(50); /* sleep for 50 milliseconds */
        return;
    }
#endif
#if defined(MICRO)
    /* simulate the delay with "cursor here" */
    for (i = 0; i < 3; i++) {
        cmov(ttyDisplay->curx, ttyDisplay->cury);
        (void) fflush(stdout);
    }
#else /* MICRO */
    /* BUG: if the padding character is visible, as it is on the 5620
       then this looks terrible. */
    if (flags.null) {
        tputs(
#ifdef TERMINFO
              "$<50>",
#else
              "50",
#endif
              1, xputc);

    } else if (ospeed > 0 && ospeed < SIZE(tmspc10) && nh_CM) {
        /* delay by sending cm(here) an appropriate number of times */
        register int cmlen =
            (int) strlen(tgoto(nh_CM, ttyDisplay->curx, ttyDisplay->cury));
        register int i = 500 + tmspc10[ospeed] / 2;

        while (i > 0) {
            cmov((int) ttyDisplay->curx, (int) ttyDisplay->cury);
            i -= cmlen * tmspc10[ospeed];
        }
    }
#endif /* MICRO */
}

/* must only be called with curx = 1 */
void
cl_eos(void) /* free after Robert Viduya */
{
    if (nh_CD) {
        xputs(nh_CD);
    } else {
        register int cy = ttyDisplay->cury + 1;

        while (cy <= LI - 2) {
            cl_end();
            (void) xputc('\n');
            cy++;
        }
        cl_end();
        tty_curs(BASE_WINDOW, (int) ttyDisplay->curx + 1,
                 (int) ttyDisplay->cury);
    }
}

#if 0
static void
kill_hilite(void)
{
    int c;

    /* if colors weren't available, no freeing needed */
    if (hilites[CLR_BLACK] == nh_HI)
        return;

    if (hilites[CLR_BLACK]) {
        if (hilites[CLR_BLACK] != hilites[CLR_BLUE])
            free(hilites[CLR_BLACK]);
        hilites[CLR_BLACK] = 0;
    }

    if (hilites[CLR_DARKGRAY]) {
        if (hilites[CLR_DARKGRAY] != hilites[NO_COLOR])
            free(hilites[CLR_DARKGRAY]);
        hilites[CLR_DARKGRAY] = 0;
    }
    /* NO_COLOR is static 'nullstr', do not free */

    for (c = 1; c < 8; c++) {
        if (hilites[c]) {
            free(hilites[c]);
            hilites[c] = 0;
        }
        if (hilites[c|BRIGHT]) {
            free(hilites[c|BRIGHT]);
            hilites[c|BRIGHT] = 0;
        }
    }
}

/* find the foreground and background colors set by nh_HI or nh_HE */
static void
analyze_seq(char *str, int *fg, int *bg)
{
    register int c, code;
    int len;

    *bg = NO_COLOR;
#ifdef MICRO
    *fg = CLR_GRAY;
#else
    *fg = NO_COLOR;
#endif

    c = (str[0] == '\233') ? 1 : 2; /* index of char beyond esc prefix */
    len = strlen(str) - 1;          /* length excluding attrib suffix */
    if ((c != 1 && (str[0] != '\033' || str[1] != '[')) || (len - c) < 1
        || str[len] != 'm')
        return;

    while (c < len) {
        if ((code = atoi(&str[c])) == 0) { /* reset */
            /* this also catches errors */
            *bg = NO_COLOR;
#ifdef MICRO
            *fg = CLR_GRAY;
#else
            *fg = NO_COLOR;
#endif
        } else if (code == 1) { /* bold */
            *fg |= BRIGHT;
#if 0
        /* I doubt we'll ever resort to using blinking characters,
           unless we want a pulsing glow for something.  But, in case
           we do... -3. */
        } else if (code == 5) { /* blinking */
            *fg |= BLINK;
        } else if (code == 25) { /* stop blinking */
            *fg &= ~BLINK;
#endif
        } else if (code == 7 || code == 27) { /* reverse */
            code = *fg & ~BRIGHT;
            *fg = *bg | (*fg & BRIGHT);
            *bg = code;
        } else if (code >= 30 && code <= 37) { /* hi_foreground RGB */
            *fg = code - 30;
        } else if (code >= 40 && code <= 47) { /* hi_background RGB */
            *bg = code - 40;
        }
        while (digit(str[++c]))
            ;
        c++;
    }
}
/*
 * Sets up highlighting sequences, using ANSI escape sequences (highlight code
 * found in print.c).  The nh_HI and nh_HE sequences (usually from SO) are
 * scanned to find foreground and background colors.
 */

static void
init_hilite(void)
{
    register int c;
    int backg, foreg, hi_backg, hi_foreg;

    for (c = 1; c < 16; c++)
        hilites[c] = nh_HI;
    hilites[NO_COLOR] = (char *) 0;

    analyze_seq(nh_HI, &hi_foreg, &hi_backg);
    analyze_seq(nh_HE, &foreg, &backg);

    for (c = 1; c < 16; c++)
        /* avoid invisibility */
        if ((backg & ~BRIGHT) != c) {
#ifdef MICRO
            if (c == CLR_BLUE)
                continue;
#endif
            if (c == foreg)
                hilites[c] = (char *) 0;
            else if (c != hi_foreg || backg != hi_backg) {
                hilites[c] = (char *) alloc(sizeof "\033[%d;3%d;4%dm");
                Sprintf(hilites[c], "\033[%d", !!(c & BRIGHT));
                if ((c | BRIGHT) != (foreg | BRIGHT))
                    Sprintf(eos(hilites[c]), ";3%d", c & ~BRIGHT);
                if (backg != 0) // !black
                    Sprintf(eos(hilites[c]), ";4%d", backg & ~BRIGHT);
                Strcat(hilites[c], "m");
            }
        }

#ifdef MICRO
    /* brighten low-visibility colors */
    hilites[CLR_BLUE] = hilites[CLR_BLUE | BRIGHT];
#endif
#endif

static char *
s_atr2str(int n)
{
    switch (n) {
    case ATR_ITALIC:
        /* if italic isn't available, fall through to underline */
        if (ZH && *ZH)
            return ZH;
        /*FALLTHRU*/
    case ATR_BLINK:
    case ATR_ULINE:
        if (n == ATR_BLINK) {
            if (MB && *MB)
                return MB;
        } else { /* Underline */
            if (nh_US && *nh_US)
                return nh_US;
        }
        /*FALLTHRU*/
    case ATR_BOLD:
        if (MD && *MD)
            return MD;
        if (nh_HI && *nh_HI)
            return nh_HI;
        break;
    case ATR_INVERSE:
        if (MR && *MR)
            return MR;
        break;
    case ATR_DIM:
        if (MH && *MH)
            return MH;
        break;
    }
    return nullstr;
}

static char *
e_atr2str(int n)
{
    switch (n) {
    case ATR_ITALIC:
        /* send ZR unless we didn't have ZH and substituted US */
        if (ZR && *ZR && ZH && *ZH)
            return ZR;
        /*FALLTHRU*/
    case ATR_ULINE:
        if (nh_UE && *nh_UE)
            return nh_UE;
        /*FALLTHRU*/
    case ATR_BOLD:
    case ATR_BLINK:
        if (nh_HE && *nh_HE)
            return nh_HE;
        /*FALLTHRU*/
    case ATR_DIM:
    case ATR_INVERSE:
        if (ME && *ME)
            return ME;
        break;
    }
    return nullstr;
}

/* suppress nonfunctional highlights so render_status() might be able to
   optimize more; keep this in sync with s_atr2str() */
int
term_attr_fixup(int msk)
{
    /* underline is converted to bold if its start sequence isn't available */
    if ((msk & HL_ULINE) && (!nh_US || !*nh_US)) {
        msk |= HL_BOLD;
        msk &= ~HL_ULINE;
    }
    /* blink used to be converted to bold unconditionally; now depends on MB */
    if ((msk & HL_BLINK) && (!MB || !*MB)) {
        msk |= HL_BOLD;
        msk &= ~HL_BLINK;
    }
    /* dim is ignored if its start sequence isn't available */
    if ((msk & HL_DIM) && (!MH || !*MH)) {
        msk &= ~HL_DIM;
    }
    return msk;
}

void
term_start_attr(int attr)
{
    if (attr) {
        const char *astr = s_atr2str(attr);

        if (astr && *astr)
            xputs(astr);
    }
}

void
term_end_attr(int attr)
{
    if (attr) {
        const char *astr = e_atr2str(attr);

        if (astr && *astr)
            xputs(astr);
    }
}

/* this is called 'start bold' but HI is derived from SO (standout) rather
   than from MD (start bold attribute) */
void
term_start_raw_bold(void)
{
    const char *soOn = nh_HI ? nh_HI : tty_standout_on;

    if (*soOn)
        xputs(soOn);
}

/* this is called 'end bold' but HE is derived from ME (end all attributes) */
void
term_end_raw_bold(void)
{
    const char *soOff = nh_HE ? nh_HE : tty_standout_off;

    if (*soOff)
        xputs(soOff);
}

#ifdef TEXTCOLOR

void
term_end_color(void)
{
    xputs(nh_HE);
}

void
term_start_color(int color)
{
    if (color < CLR_MAX && hilites[color] && *hilites[color])
        xputs(hilites[color]);
}

void
term_start_bgcolor(int color)
{
    xputs(bghilites[color]);
}
#endif /* TEXTCOLOR */

#ifdef ENHANCED_SYMBOLS

#ifndef SEP2
#define tcfmtstr "\033[38;2;%ld;%ld;%ldm"
#ifdef UNIX
#define tcfmtstr24bit "\033[38;2;%u;%u;%um"
#define tcfmtstr256 "\033[38;5;%dm"
#else
#define tcfmtstr24bit "\033[38;2;%lu;%lu;%lum"
#define tcfmtstr256 "\033[38:5:%ldm"
#endif
#endif

static void emit24bit(long mcolor);
static void emit256(int u256coloridx);

static void emit24bit(long mcolor)
{
    static char tcolorbuf[QBUFSZ];

    Snprintf(tcolorbuf, sizeof tcolorbuf, tcfmtstr,
             ((mcolor >> 16) & 0xFF),   /* red */
             ((mcolor >>  8) & 0xFF),   /* green */
             ((mcolor >>  0) & 0xFF));  /* blue */
    xputs(tcolorbuf);
}

static void emit256(int u256coloridx)
{
    static char tcolorbuf[QBUFSZ];

    Snprintf(tcolorbuf, sizeof tcolorbuf, tcfmtstr256,
             u256coloridx);
    xputs(tcolorbuf);
}

void
term_start_24bitcolor(struct unicode_representation *urep)
{
    if (urep && SYMHANDLING(H_UTF8)) {
        /* color 0 has bit 0x1000000 set */
        long mcolor = (urep->ucolor & 0xFFFFFF);
        if (iflags.colorcount == 256)
            emit256(urep->u256coloridx);
        else
            emit24bit(mcolor);
    }
}

void
term_end_24bitcolor(void)
{
    if (SYMHANDLING(H_UTF8)) {
        xputs("\033[0m");
    }
}
#endif /* ENHANCED_SYMBOLS */
#endif /* TTY_GRAPHICS && !NO_TERMS  */

/*termcap.c*/
