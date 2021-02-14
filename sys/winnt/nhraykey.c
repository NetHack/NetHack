/* NetHack 3.7	nhraykey.c	$NHDT-Date: 1596498314 2020/08/03 23:45:14 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.21 $ */
/* Copyright (c) NetHack PC Development Team 2003                      */
/* NetHack may be freely redistributed.  See license for details.      */

/*
 * Keystroke handling contributed by Ray Chason.
 * The following text was written by Ray Chason.
 *
 * The problem
 * ===========
 *
 * The console-mode Nethack wants both keyboard and mouse input.  The
 * problem is that the Windows API provides no easy way to get mouse input
 * and also keyboard input properly translated according to the user's
 * chosen keyboard layout.
 *
 * The ReadConsoleInput function returns a stream of keyboard and mouse
 * events.  Nethack is interested in those events that represent a key
 * pressed, or a click on a mouse button.  The keyboard events from
 * ReadConsoleInput are not translated according to the keyboard layout,
 * and do not take into account the shift, control, or alt keys.
 *
 * The PeekConsoleInput function works similarly to ReadConsoleInput,
 * except that it does not remove an event from the queue and it returns
 * instead of blocking when the queue is empty.
 *
 * A program can also use ReadConsole to get a properly translated stream
 * of characters.  Unfortunately, ReadConsole does not return mouse events,
 * does not distinguish the keypad from the main keyboard, does not return
 * keys shifted with Alt, and does not even return the ESC key when
 * pressed.
 *
 * We want both the functionality of ReadConsole and the functionality of
 * ReadConsoleInput.  But Microsoft didn't seem to think of that.
 *
 *
 * The solution, in the original code
 * ==================================
 *
 * The original 3.4.1 distribution tries to get proper keyboard translation
 * by passing keyboard events to the ToAscii function.  This works, to some
 * extent -- it takes the shift key into account, and it processes dead
 * keys properly.  But it doesn't take non-US keyboards into account.  It
 * appears that ToAscii is meant for windowed applications, and does not
 * have enough information to do its job properly in a console application.
 *
 *
 * The Finnish keyboard patch
 * ==========================
 *
 * This patch adds the "subkeyvalue" option to the defaults.nh file.  The
 * user can then add OPTIONS=sukeyvalue:171/92, for instance, to replace
 * the 171 character with 92, which is \.  This works, once properly
 * configured, but places too much burden on the user.  It also bars the
 * use of the substituted characters in naming objects or monsters.
 *
 *
 * The solution presented here
 * ===========================
 *
 * The best way I could find to combine the functionality of ReadConsole
 * with that of ReadConsoleInput is simple in concept.  First, call
 * PeekConsoleInput to get the first event.  If it represents a key press,
 * call ReadConsole to retrieve the key.  Otherwise, pop it off the queue
 * with ReadConsoleInput and, if it's a mouse click, return it as such.
 *
 * But the Devil, as they say, is in the details.  The problem is in
 * recognizing an event that ReadConsole will return as a key.  We don't
 * want to call ReadConsole unless we know that it will immediately return:
 * if it blocks, the mouse and the Alt sequences will cease to function
 * until it returns.
 *
 * Separating process_keystroke into two functions, one for commands and a
 * new one, process_keystroke2, for answering prompts, makes the job a lot
 * easier.  process_keystroke2 doesn't have to worry about mouse events or
 * Alt sequences, and so the consequences are minor if ReadConsole blocks.
 * process_keystroke, OTOH, never needs to return a non-ASCII character
 * that was read from ReadConsole; it returns bytes with the high bit set
 * only in response to an Alt sequence.
 *
 * So in process_keystroke, before calling ReadConsole, a bogus key event
 * is pushed on the queue.  This event causes ReadConsole to return, even
 * if there was no other character available.  Because the bogus key has
 * the eighth bit set, it is filtered out.  This is not done in
 * process_keystroke2, because that would render dead keys unusable.
 *
 * A separate process_keystroke2 can also process the numeric keypad in a
 * way that makes sense for prompts:  just return the corresponding symbol,
 * and pay no mind to number_pad or the num lock key.
 *
 * The recognition of Alt sequences is modified, to support the use of
 * characters generated with the AltGr key.  A keystroke is an Alt sequence
 * if an Alt key is seen that can't be an AltGr (since an AltGr sequence
 * could be a character, and in some layouts it could even be an ASCII
 * character).  This recognition is different on NT-based and 95-based
 * Windows:
 *
 *    * On NT-based Windows, AltGr signals as right Alt and left Ctrl
 *      together.  So an Alt sequence is recognized if either Alt key is
 *      pressed and if right Alt and left Ctrl are not both present.  This
 *      is true even if the keyboard in use does not have an AltGr key, and
 *      uses right Alt for AltGr.
 *
 *    * On 95-based Windows, with a keyboard that lacks the AltGr key, the
 *      right Alt key is used instead.  But it still signals as right Alt,
 *      without left Ctrl.  There is no way for the application to know
 *      whether right Alt is Alt or AltGr, and so it is always assumed
 *      to be AltGr.  This means that Alt sequences must be formed with
 *      left Alt.
 *
 * So the patch processes keystrokes as follows:
 *
 *     * If the scan and virtual key codes are both 0, it's the bogus key,
 *       and we ignore it.
 *
 *     * Keys on the numeric keypad are processed for commands as in the
 *       unpatched Nethack, and for prompts by returning the ASCII
 *       character, even if the num lock is off.
 *
 *     * Alt sequences are processed for commands as in the unpatched
 *       Nethack, and ignored for prompts.
 *
 *     * Control codes are returned as received, because ReadConsole will
 *       not return the ESC key.
 *
 *     * Other key-down events are passed to ReadConsole.  The use of
 *       ReadConsole is different for commands than for prompts:
 *
 *       o For commands, the bogus key is pushed onto the queue before
 *         ReadConsole is called.  On return, non-ASCII characters are
 *         filtered, so they are not mistaken for Alt sequences; this also
 *         filters the bogus key.
 *
 *       o For prompts, the bogus key is not used, because that would
 *         interfere with dead keys.  Eight bit characters may be returned,
 *         and are coded in the configured code page.
 *
 *
 * Possible improvements
 * =====================
 *
 * Some possible improvements remain:
 *
 *     * Integrate the existing Finnish keyboard patch, for use with non-
 *       QWERTY layouts such as the German QWERTZ keyboard or Dvorak.
 *
 *     * Fix the keyboard glitches in the graphical version.  Namely, dead
 *       keys don't work, and input comes in as ISO-8859-1 but is displayed
 *       as code page 437 if IBMgraphics is set on startup.
 *
 *     * Transform incoming text to ISO-8859-1, for full compatibility with
 *       the graphical version.
 *
 *     * After pushing the bogus key and calling ReadConsole, check to see
 *       if we got the bogus key; if so, and an Alt is pressed, process the
 *       event as an Alt sequence.
 *
 */

static char where_to_get_source[] = "http://www.nethack.org/";
static char author[] = "Ray Chason";

#include "win32api.h"
#include "hack.h"
#include "wintty.h"

extern HANDLE hConIn;
extern INPUT_RECORD ir;
char dllname[512];
char *shortdllname;

int __declspec(dllexport) __stdcall ProcessKeystroke(HANDLE hConIn,
               INPUT_RECORD *ir, boolean *valid, boolean numberpad, int portdebug);

static INPUT_RECORD bogus_key;

int WINAPI
DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved)
{
    char dlltmpname[512];
    char *tmp = dlltmpname, *tmp2;
    *(tmp + GetModuleFileName(hInstance, tmp, 511)) = '\0';
    (void) strcpy(dllname, tmp);
    tmp2 = strrchr(dllname, '\\');
    if (tmp2) {
        tmp2++;
        shortdllname = tmp2;
    }
    /* A bogus key that will be filtered when received, to keep ReadConsole
     * from blocking */
    bogus_key.EventType = KEY_EVENT;
    bogus_key.Event.KeyEvent.bKeyDown = 1;
    bogus_key.Event.KeyEvent.wRepeatCount = 1;
    bogus_key.Event.KeyEvent.wVirtualKeyCode = 0;
    bogus_key.Event.KeyEvent.wVirtualScanCode = 0;
    bogus_key.Event.KeyEvent.uChar.AsciiChar = (uchar) 0x80;
    bogus_key.Event.KeyEvent.dwControlKeyState = 0;
    return TRUE;
}

/*
 *  Keyboard translation tables.
 *  (Adopted from the MSDOS port)
 */

#define KEYPADLO 0x47
#define KEYPADHI 0x53

#define PADKEYS (KEYPADHI - KEYPADLO + 1)
#define iskeypad(x) (KEYPADLO <= (x) && (x) <= KEYPADHI)
#define isnumkeypad(x) \
    (KEYPADLO <= (x) && (x) <= 0x51 && (x) != 0x4A && (x) != 0x4E)

#ifdef QWERTZ_SUPPORT
/* when 'numberpad' is 0 and Cmd.swap_yz is True
   (signaled by setting 0x10 on boolean numpad argument)
   treat keypress of numpad 7 as 'z' rather than 'y' */
static boolean qwertz = FALSE;
#endif

/*
 * Keypad keys are translated to the normal values below.
 * Shifted keypad keys are translated to the
 *    shift values below.
 */

static const struct pad {
    uchar normal, shift, cntrl;
} keypad[PADKEYS] =
    {
      { 'y', 'Y', C('y') },    /* 7 */
      { 'k', 'K', C('k') },    /* 8 */
      { 'u', 'U', C('u') },    /* 9 */
      { 'm', C('p'), C('p') }, /* - */
      { 'h', 'H', C('h') },    /* 4 */
      { 'g', 'G', 'g' },       /* 5 */
      { 'l', 'L', C('l') },    /* 6 */
      { '+', 'P', C('p') },    /* + */
      { 'b', 'B', C('b') },    /* 1 */
      { 'j', 'J', C('j') },    /* 2 */
      { 'n', 'N', C('n') },    /* 3 */
      { 'i', 'I', C('i') },    /* Ins */
      { '.', ':', ':' }        /* Del */
    },
  numpad[PADKEYS] = {
      { '7', M('7'), '7' },    /* 7 */
      { '8', M('8'), '8' },    /* 8 */
      { '9', M('9'), '9' },    /* 9 */
      { 'm', C('p'), C('p') }, /* - */
      { '4', M('4'), '4' },    /* 4 */
      { 'g', 'G', 'g' },       /* 5 */
      { '6', M('6'), '6' },    /* 6 */
      { '+', 'P', C('p') },    /* + */
      { '1', M('1'), '1' },    /* 1 */
      { '2', M('2'), '2' },    /* 2 */
      { '3', M('3'), '3' },    /* 3 */
      { 'i', 'I', C('i') },    /* Ins */
      { '.', ':', ':' }        /* Del */
  };

#define inmap(x, vk) (((x) > 'A' && (x) < 'Z') || (vk) == 0xBF || (x) == '2')

/* Use process_keystroke for key commands, process_keystroke2 for prompts */
/* int process_keystroke(INPUT_RECORD *ir, boolean *valid, int
 * portdebug); */
int process_keystroke2(HANDLE, INPUT_RECORD *ir, boolean *valid);
static int is_altseq(unsigned long shiftstate);

static int
is_altseq(unsigned long shiftstate)
{
    /* We need to distinguish the Alt keys from the AltGr key.
     * On NT-based Windows, AltGr signals as right Alt and left Ctrl together;
     * on 95-based Windows, AltGr signals as right Alt only.
     * So on NT, we signal Alt if either Alt is pressed and left Ctrl is not,
     * and on 95, we signal Alt for left Alt only. */
    switch (shiftstate
            & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | LEFT_CTRL_PRESSED)) {
    case LEFT_ALT_PRESSED:
    case LEFT_ALT_PRESSED | LEFT_CTRL_PRESSED:
        return 1;

    case RIGHT_ALT_PRESSED:
    case RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED:
        return (GetVersion() & 0x80000000) == 0;

    default:
        return 0;
    }
}

int __declspec(dllexport) __stdcall 
ProcessKeystroke(
    HANDLE hConIn,
    INPUT_RECORD *ir,
    boolean *valid,
    boolean numberpad,
    int portdebug)
{
    int keycode, vk;
    unsigned char ch, pre_ch;
    unsigned short int scan;
    unsigned long shiftstate;
    int altseq = 0;
    const struct pad *kpad;
    DWORD count;

#ifdef QWERTZ_SUPPORT
    if (numberpad & 0x10) {
        numberpad &= ~0x10;
        qwertz = TRUE;
    } else {
        qwertz = FALSE;
    }
#endif
    shiftstate = 0L;
    ch = pre_ch = ir->Event.KeyEvent.uChar.AsciiChar;
    scan = ir->Event.KeyEvent.wVirtualScanCode;
    vk = ir->Event.KeyEvent.wVirtualKeyCode;
    keycode = MapVirtualKey(vk, 2);
    shiftstate = ir->Event.KeyEvent.dwControlKeyState;
    if (scan == 0 && vk == 0) {
        /* It's the bogus_key */
        ReadConsoleInput(hConIn, ir, 1, &count);
        *valid = FALSE;
        return 0;
    }

    if (is_altseq(shiftstate)) {
        if (ch || inmap(keycode, vk))
            altseq = 1;
        else
            altseq = -1; /* invalid altseq */
    }
    if (ch || (iskeypad(scan)) || (altseq > 0))
        *valid = TRUE;
    /* if (!valid) return 0; */
    /*
     * shiftstate can be checked to see if various special
     * keys were pressed at the same time as the key.
     * Currently we are using the ALT & SHIFT & CONTROLS.
     *
     *           RIGHT_ALT_PRESSED, LEFT_ALT_PRESSED,
     *           RIGHT_CTRL_PRESSED, LEFT_CTRL_PRESSED,
     *           SHIFT_PRESSED,NUMLOCK_ON, SCROLLLOCK_ON,
     *           CAPSLOCK_ON, ENHANCED_KEY
     *
     * are all valid bit masks to use on shiftstate.
     * eg. (shiftstate & LEFT_CTRL_PRESSED) is true if the
     *      left control key was pressed with the keystroke.
     */
    if (iskeypad(scan)) {
        ReadConsoleInput(hConIn, ir, 1, &count);
        kpad = numberpad ? numpad : keypad;
        if (shiftstate & SHIFT_PRESSED) {
            ch = kpad[scan - KEYPADLO].shift;
        } else if (shiftstate & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) {
            ch = kpad[scan - KEYPADLO].cntrl;
        } else {
            ch = kpad[scan - KEYPADLO].normal;
        }
#ifdef QWERTZ_SUPPORT
        /* OPTIONS=number_pad:-1 is for qwertz keyboard; for that setting,
           'numberpad' will be 0; core swaps y to zap, z to move northwest;
           we want numpad 7 to move northwest, so when qwertz is set,
           tell core that user who types numpad 7 typed z rather than y */
        if (qwertz && kpad[scan - KEYPADLO].normal == 'y')
            ch += 1; /* changes y to z, Y to Z, ^Y to ^Z */
#endif /*QWERTZ_SUPPORT*/
    } else if (altseq > 0) { /* ALT sequence */
        ReadConsoleInput(hConIn, ir, 1, &count);
        if (vk == 0xBF)
            ch = M('?');
        else
            ch = M(tolower((uchar) keycode));
    } else if (ch < 32 && !isnumkeypad(scan)) {
        /* Control code; ReadConsole seems to filter some of these,
         * including ESC */
        ReadConsoleInput(hConIn, ir, 1, &count);
    }
    /* Attempt to work better with international keyboards. */
    else {
        CHAR ch2;
        DWORD written;
        /* The bogus_key guarantees that ReadConsole will return,
         * and does not itself do anything */
        WriteConsoleInput(hConIn, &bogus_key, 1, &written);
        ReadConsole(hConIn, &ch2, 1, &count, NULL);
        /* Prevent high characters from being interpreted as alt
         * sequences; also filter the bogus_key */
        if (ch2 & 0x80)
            *valid = FALSE;
        else
            ch = ch2;
        if (ch == 0)
            *valid = FALSE;
    }
    if (ch == '\r')
        ch = '\n';
#ifdef PORT_DEBUG
    if (portdebug) {
        char buf[BUFSZ];
        Sprintf(buf, "PORTDEBUG: ch=%u, scan=%u, vk=%d, pre=%d, "
                     "shiftstate=0x%lX (ESC to end)\n",
                ch, scan, vk, pre_ch, shiftstate);
        fprintf(stdout, "\n%s", buf);
    }
#endif
    return ch;
}

int
process_keystroke2(
    HANDLE hConIn,
    INPUT_RECORD *ir,
    boolean *valid)
{
    /* Use these values for the numeric keypad */
    static const char keypad_nums[] = "789-456+1230.";

    unsigned char ch;
    int vk;
    unsigned short int scan;
    unsigned long shiftstate;
    int altseq;
    DWORD count;

    ch = ir->Event.KeyEvent.uChar.AsciiChar;
    vk = ir->Event.KeyEvent.wVirtualKeyCode;
    scan = ir->Event.KeyEvent.wVirtualScanCode;
    shiftstate = ir->Event.KeyEvent.dwControlKeyState;

    if (scan == 0 && vk == 0) {
        /* It's the bogus_key */
        ReadConsoleInput(hConIn, ir, 1, &count);
        *valid = FALSE;
        return 0;
    }

    altseq = is_altseq(shiftstate);
    if (ch || (iskeypad(scan)) || altseq)
        *valid = TRUE;
    /* if (!valid) return 0; */
    /*
     * shiftstate can be checked to see if various special
     * keys were pressed at the same time as the key.
     * Currently we are using the ALT & SHIFT & CONTROLS.
     *
     *           RIGHT_ALT_PRESSED, LEFT_ALT_PRESSED,
     *           RIGHT_CTRL_PRESSED, LEFT_CTRL_PRESSED,
     *           SHIFT_PRESSED,NUMLOCK_ON, SCROLLLOCK_ON,
     *           CAPSLOCK_ON, ENHANCED_KEY
     *
     * are all valid bit masks to use on shiftstate.
     * eg. (shiftstate & LEFT_CTRL_PRESSED) is true if the
     *      left control key was pressed with the keystroke.
     */
    if (iskeypad(scan) && !altseq) {
        ReadConsoleInput(hConIn, ir, 1, &count);
        ch = keypad_nums[scan - KEYPADLO];
    } else if (ch < 32 && !isnumkeypad(scan)) {
        /* Control code; ReadConsole seems to filter some of these,
         * including ESC */
        ReadConsoleInput(hConIn, ir, 1, &count);
    }
    /* Attempt to work better with international keyboards. */
    else {
        CHAR ch2;
        ReadConsole(hConIn, &ch2, 1, &count, NULL);
        ch = ch2 & 0xFF;
        if (ch == 0)
            *valid = FALSE;
    }
    if (ch == '\r')
        ch = '\n';
    return ch;
}

int __declspec(dllexport) __stdcall 
CheckInput(
    HANDLE hConIn,
    INPUT_RECORD *ir,
    DWORD *count,
    boolean numpad,
    int mode,
    int *mod,
    coord *cc)
{
#if defined(SAFERHANGUP)
    DWORD dwWait;
#endif
    int ch = 0;
    boolean valid = 0, done = 0;

#ifdef QWERTZ_SUPPORT
    if (numpad & 0x10) {
        numpad &= ~0x10;
        qwertz = TRUE;
    } else {
        qwertz = FALSE;
    }
#endif
    while (!done) {
        *count = 0;
        dwWait = WaitForSingleObject(hConIn, INFINITE);
#if defined(SAFERHANGUP)
        if (dwWait == WAIT_FAILED)
            return '\033';
#endif
        PeekConsoleInput(hConIn, ir, 1, count);
        if (mode == 0) {
            if ((ir->EventType == KEY_EVENT) && ir->Event.KeyEvent.bKeyDown) {
                ch = process_keystroke2(hConIn, ir, &valid);
                done = valid;
            } else
                ReadConsoleInput(hConIn, ir, 1, count);
        } else {
            ch = 0;
            if (count > 0) {
                if (ir->EventType == KEY_EVENT
                    && ir->Event.KeyEvent.bKeyDown) {
#ifdef QWERTZ_SUPPORT
                    if (qwertz)
                        numpad |= 0x10;
#endif
                    ch = ProcessKeystroke(hConIn, ir, &valid, numpad,
#ifdef PORTDEBUG
                                          1);
#else
                                          0);
#endif
#ifdef QWERTZ_SUPPORT
                    numpad &= ~0x10;
#endif                    
                    if (valid)
                        return ch;
                } else {
                    ReadConsoleInput(hConIn, ir, 1, count);
                    if (ir->EventType == MOUSE_EVENT) {
                        if ((ir->Event.MouseEvent.dwEventFlags == 0)
                            && (ir->Event.MouseEvent.dwButtonState
                                & MOUSEMASK)) {
                            cc->x =
                                ir->Event.MouseEvent.dwMousePosition.X + 1;
                            cc->y =
                                ir->Event.MouseEvent.dwMousePosition.Y - 1;

                            if (ir->Event.MouseEvent.dwButtonState
                                & LEFTBUTTON)
                                *mod = CLICK_1;
                            else if (ir->Event.MouseEvent.dwButtonState
                                     & RIGHTBUTTON)
                                *mod = CLICK_2;
#if 0 /* middle button */			       
				else if (ir->Event.MouseEvent.dwButtonState & MIDBUTTON)
			      		*mod = CLICK_3;
#endif
                            return 0;
                        }
                    }
#if 0
			/* We ignore these types of console events */
		        else if (ir->EventType == FOCUS_EVENT) {
		        }
		        else if (ir->EventType == MENU_EVENT) {
		        }
#endif
                }
            } else
                done = 1;
        }
    }
    *mod = 0;
    return ch;
}

int __declspec(dllexport) __stdcall NHkbhit(
    HANDLE hConIn,
    INPUT_RECORD *ir)
{
    int done = 0; /* true =  "stop searching"        */
    int retval;   /* true =  "we had a match"        */
    DWORD count;
    unsigned short int scan;
    unsigned char ch;
    unsigned long shiftstate;
    int altseq = 0, keycode, vk;

    done = 0;
    retval = 0;
    while (!done) {
        count = 0;
        PeekConsoleInput(hConIn, ir, 1, &count);
        if (count > 0) {
            if (ir->EventType == KEY_EVENT && ir->Event.KeyEvent.bKeyDown) {
                ch = ir->Event.KeyEvent.uChar.AsciiChar;
                scan = ir->Event.KeyEvent.wVirtualScanCode;
                shiftstate = ir->Event.KeyEvent.dwControlKeyState;
                vk = ir->Event.KeyEvent.wVirtualKeyCode;
                if (scan == 0 && vk == 0) {
                    /* It's the bogus_key.  Discard it */
                    ReadConsoleInput(hConIn,ir,1,&count);
                } else {
                    keycode = MapVirtualKey(vk, 2);
                    if (is_altseq(shiftstate)) {
                        if (ch || inmap(keycode, vk))
                            altseq = 1;
                        else
                            altseq = -1; /* invalid altseq */
                    }
                    if (ch || iskeypad(scan) || altseq) {
                        done = 1;   /* Stop looking         */
                        retval = 1; /* Found what we sought */
                    } else {
                        /* Strange Key event; let's purge it to avoid trouble */
                        ReadConsoleInput(hConIn, ir, 1, &count);
                    }
                }

            } else if ((ir->EventType == MOUSE_EVENT
                        && (ir->Event.MouseEvent.dwButtonState
                            & MOUSEMASK))) {
                done = 1;
                retval = 1;
            }

            else /* Discard it, it's an insignificant event */
                ReadConsoleInput(hConIn, ir, 1, &count);
        } else /* There are no events in console event queue */ {
            done = 1; /* Stop looking               */
            retval = 0;
        }
    }
    return retval;
}

int __declspec(dllexport) __stdcall 
SourceWhere(char** buf)
{
    if (!buf)
        return 0;
    *buf = where_to_get_source;
    return 1;
}

int __declspec(dllexport) __stdcall 
SourceAuthor(char** buf)
{
    if (!buf)
        return 0;
    *buf = author;
    return 1;
}

int __declspec(dllexport) __stdcall KeyHandlerName(
    char **buf,
    int full)
{
    if (!buf)
        return 0;
    if (full)
        *buf = dllname;
    else
        *buf = shortdllname;
    return 1;
}
