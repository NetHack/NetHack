/* NetHack 5.0	tos.c	$NHDT-Date: 1501979358 2017/08/06 00:29:18 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.8 $ */
/* NetHack may be freely redistributed.  See license for details. */

/*
 *  TOS system functions.
 */

/* NetHack 5.0 with Lua needs significantly more stack than the
   default.  For GCC with mintlib, _stksize controls this. */
long _stksize = 256 * 1024L;

#define NEED_VARARGS
#include "hack.h"

#ifdef TTY_GRAPHICS
#include "tcap.h"
#else
/* Used by set_colors / restore_colors below when this TU is built
   without tcap.h. */
static char *nh_HE = "\033q";
#endif

#ifdef TOS

#include <osbind.h>
#ifdef MINT
#include <sys/ioctl.h>
#endif
#ifndef WORD
#define WORD short /* 16 bits -- redefine if necessary */
#endif

#include <ctype.h>

static char DOSgetch(void);
static char BIOSgetch(void);
static void init_aline(void);
char *_a_line; /* for Line A variables */
boolean colors_changed = FALSE;

int
tgetch(void)
{
    char ch;

    /* BIOSgetch can use the numeric key pad on IBM compatibles. */
    if (iflags.BIOS)
        ch = BIOSgetch();
    else
        ch = DOSgetch();
    return ((ch == '\r') ? '\n' : ch);
}

/*
 *  Keyboard translation tables.
 */
#define KEYPADLO 0x61
#define KEYPADHI 0x71

#define PADKEYS (KEYPADHI - KEYPADLO + 1)
#define iskeypad(x) (KEYPADLO <= (x) && (x) <= KEYPADHI)

/*
 * Keypad keys are translated to the normal values below.
 * When iflags.BIOS is active, shifted keypad keys are translated to the
 *    shift values below.
 */
static const struct pad {
    char normal, shift, cntrl;
} keypad[PADKEYS] =
    {
      { C('['), 'Q', C('[') }, /* UNDO */
      { '?', '/', '?' },       /* HELP */
      { '(', 'a', '(' },       /* ( */
      { ')', 'w', ')' },       /* ) */
      { '/', '/', '/' },       /* / */
      { C('p'), '$', C('p') }, /* * */
      { 'y', 'Y', C('y') },    /* 7 */
      { 'k', 'K', C('k') },    /* 8 */
      { 'u', 'U', C('u') },    /* 9 */
      { 'h', 'H', C('h') },    /* 4 */
      { '.', '.', '.' },
      { 'l', 'L', C('l') }, /* 6 */
      { 'b', 'B', C('b') }, /* 1 */
      { 'j', 'J', C('j') }, /* 2 */
      { 'n', 'N', C('n') }, /* 3 */
      { 'i', 'I', C('i') }, /* Ins */
      { '.', ':', ':' }     /* Del */
    },
  numpad[PADKEYS] = {
      { C('['), 'Q', C('[') }, /* UNDO */
      { '?', '/', '?' },       /* HELP */
      { '(', 'a', '(' },       /* ( */
      { ')', 'w', ')' },       /* ) */
      { '/', '/', '/' },       /* / */
      { C('p'), '$', C('p') }, /* * */
      { '7', M('7'), '7' },    /* 7 */
      { '8', M('8'), '8' },    /* 8 */
      { '9', M('9'), '9' },    /* 9 */
      { '4', M('4'), '4' },    /* 4 */
      { '.', '.', '.' },       /* 5 */
      { '6', M('6'), '6' },    /* 6 */
      { '1', M('1'), '1' },    /* 1 */
      { '2', M('2'), '2' },    /* 2 */
      { '3', M('3'), '3' },    /* 3 */
      { 'i', 'I', C('i') },    /* Ins */
      { '.', ':', ':' }        /* Del */
  };

/*
 * Unlike Ctrl-letter, the Alt-letter keystrokes have no specific ASCII
 * meaning unless assigned one by a keyboard conversion table, so the
 * keyboard BIOS normally does not return a character code when Alt-letter
 * is pressed.	So, to interpret unassigned Alt-letters, we must use a
 * scan code table to translate the scan code into a letter, then set the
 * "meta" bit for it.  -3.
 */
#define SCANLO 0x10

static const char scanmap[] = {
    /* ... */
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a',
    's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x',
    'c', 'v', 'b', 'N', 'm', ',', '.', '?' /* ... */
};

#define inmap(x) (SCANLO <= (x) && (x) < SCANLO + SIZE(scanmap))

/*
 * BIOSgetch gets keys directly with a BIOS call.
 */
#define SHIFT (0x1 | 0x2)
#define CTRL 0x4
#define ALT 0x8

static char
BIOSgetch(void)
{
    unsigned char scan, shift, ch;
    const struct pad *kpad;

    long x;

    /* Get scan code.
     */
    x = Crawcin();
    ch = x & 0x0ff;
    scan = (x & 0x00ff0000L) >> 16;
    /* Get shift status.
     */
    shift = Kbshift(-1);

    /* Translate keypad keys */
    if (iskeypad(scan)) {
        kpad = iflags.num_pad ? numpad : keypad;
        if (shift & SHIFT)
            ch = kpad[scan - KEYPADLO].shift;
        else if (shift & CTRL)
            ch = kpad[scan - KEYPADLO].cntrl;
        else
            ch = kpad[scan - KEYPADLO].normal;
    }
    /* Translate unassigned Alt-letters */
    if ((shift & ALT) && !ch) {
        if (inmap(scan))
            ch = scanmap[scan - SCANLO];
        return (isprint(ch) ? M(ch) : ch);
    }
    return ch;
}

static char
DOSgetch(void)
{
    return (Crawcin() & 0x007f);
}

long
freediskspace(char *path)
{
    int drive = 0;
    struct {
        long freal; /*free allocation units*/
        long total; /*total number of allocation units*/
        long bps;   /*bytes per sector*/
        long pspal; /*physical sectors per allocation unit*/
    } freespace;
    if (path[0] && path[1] == ':')
        drive = (toupper(path[0]) - 'A') + 1;
    if (Dfree(&freespace, drive) < 0)
        return -1;
    return freespace.freal * freespace.bps * freespace.pspal;
}

/*
 * Functions to get filenames using wildcards
 */
int
findfirst(char *path)
{
    return (Fsfirst(path, 0) == 0);
}

int
findnext(void)
{
    return (Fsnext() == 0);
}

char *
foundfile_buffer(void)
{
    return (char *) Fgetdta() + 30;
}

long
filesize(char *file)
{
    long sz;

    if (!findfirst(file))
        return -1L;
    /* The DTA d_length field at offset 26 is not guaranteed to be
       4-byte aligned; a direct long dereference faults on 68000. */
    memcpy(&sz, (char *) Fgetdta() + 26, sizeof sz);
    return sz;
}

/*
 * Chdrive() changes the default drive.
 */
void
chdrive(const char *str)
{
    char *ptr;
    char drive;

    if ((ptr = strchr(str, ':')) != (char *) 0 && ptr > str) {
        drive = toupper(*(ptr - 1));
        (void) Dsetdrv(drive - 'A');
    }
    return;
}

void
get_scr_size(void)
{
#ifdef MINT
    struct winsize win;
    char *tmp;

    LI = CO = 0;
    if ((tmp = nh_getenv("LINES")) || (tmp = nh_getenv("ROWS")))
        LI = atoi(tmp);
    if ((tmp = nh_getenv("COLUMNS")))
        CO = atoi(tmp);
    if (!LI || !CO) {
        if (ioctl(0, TIOCGWINSZ, &win) == 0) {
            if (!LI) LI = win.ws_row;
            if (!CO) CO = win.ws_col;
        }
    }
    if (!LI) LI = 25;
    if (!CO) CO = 80;
#else
    WORD rows, cols;

    init_aline();
    /* Line-A variables live at fixed offsets below _a_line; the base
       isn't guaranteed to be word-aligned, so copy out instead of
       casting through a (WORD *) which faults on 68000. */
    memcpy(&rows, _a_line - 42, sizeof rows);
    memcpy(&cols, _a_line - 44, sizeof cols);
    LI = rows + 1;
    CO = cols + 1;
#endif
}

#define BIGBUF 8192

int
_copyfile(char *from, char *to)
{
    int fromfd, tofd, r;
    char *buf;

    fromfd = open(from, O_RDONLY | O_BINARY, 0);
    if (fromfd < 0)
        return -1;
    tofd = open(to, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, FCMASK);
    if (tofd < 0) {
        close(fromfd);
        return -1;
    }
    buf = (char *) alloc((unsigned) BIGBUF);
    while ((r = read(fromfd, buf, BIGBUF)) > 0) {
        if (write(tofd, buf, r) != r) {
            close(fromfd);
            close(tofd);
            free((genericptr_t) buf);
            return -1;
        }
    }
    close(fromfd);
    close(tofd);
    free((genericptr_t) buf);
    return (r < 0) ? -1 : 0;
}

extern short mar_kbhit(void); /* win/gem/wingem1.c */

int
kbhit(void)
{
    /* Use the AES event multiplexer instead of the BIOS console:
       under MiNT, Cconis() bounces through the kernel console device
       and the AES console host per call, which is hot during long
       occupations (eat / dig / travel / run, see src/allmain.c). */
    return mar_kbhit();
}

static void
init_aline(void)
{
#ifdef __GNUC__
    /* line A calls nuke registers d0-d2,a0-a2; not all compilers regard these
       as scratch registers, though, so we save them
     */
    asm(" moveml d0-d2/a0-a2, sp@-");
    asm(" .word 0xa000; movel d0, __a_line");
    asm(" moveml sp@+, d0-d2/a0-a2");
#else
    asm(" movem.l d0-d2/a0-a2, -(sp)");
    asm(" .dc.w 0xa000"); /* tweak as necessary for your compiler */
    asm(" move.l d0, __a_line");
    asm(" movem.l (sp)+, d0-d2/a0-a2");
#endif
}

/* used in termcap.c to decide how to set up the hilites */
unsigned long tos_numcolors = 2;

void
set_colors(void)
{
    static char colorHE[] = "\033q\033b0";

    if (!iflags.BIOS)
        return;
    init_aline();
    tos_numcolors = 1 << (((unsigned char *) _a_line)[1]);
    if (tos_numcolors <= 2) { /* mono */
        iflags.use_color = FALSE;
        return;
    } else {
        colors_changed = TRUE;
        nh_HE = colorHE;
    }
}

void
restore_colors(void)
{
    static char plainHE[] = "\033q";

    if (colors_changed)
        nh_HE = plainHE;
    colors_changed = FALSE;
}

#ifdef SUSPEND

#include <signal.h>

#ifdef MINT
extern int __mint;
#endif

int
dosuspend(void)
{
#ifdef MINT
    if (__mint == 0) {
#endif
        pline("Sorry, it seems we have no SIGTSTP here.  Try ! or S.");
#ifdef MINT
    } else if (signal(SIGTSTP, SIG_IGN) == SIG_DFL) {
        suspend_nhwindows((char *) 0);
        (void) signal(SIGTSTP, SIG_DFL);
        (void) kill(0, SIGTSTP);
        get_scr_size();
        resume_nhwindows();
    } else {
        pline("I don't think your shell has job control.");
    }
#endif /* MINT */
    return (0);
}
#endif /* SUSPEND */

#ifdef NOTTYGRAPHICS
/* stub for pcsys.c when building without TTY */
void
term_curs_set(int visibility UNUSED)
{
}
#endif

/* NetHack's error() must be provided here because unixtty.c is not
   compiled for TOS, and MiNT libc has a weak error() symbol with an
   incompatible signature (GNU error()) that would be used otherwise. */
void
error(const char *s, ...)
{
    va_list the_args;

    va_start(the_args, s);
    if (iflags.window_inited)
        exit_nhwindows((char *) 0);
    vprintf(s, the_args);
    (void) putchar('\n');
    va_end(the_args);
    nethack_exit(EXIT_FAILURE);
}

#endif /* TOS */
