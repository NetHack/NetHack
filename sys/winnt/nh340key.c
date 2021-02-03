/* NetHack 3.7	nh340key.c	$NHDT-Date: 1596498312 2020/08/03 23:45:12 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.16 $ */
/* Copyright (c) NetHack PC Development Team 2003                      */
/* NetHack may be freely redistributed.  See license for details.      */

/*
 * This is the NetHack keystroke processing from NetHack 3.4.0.
 * It can be built as a run-time loadable dll (nh340key.dll),
 * placed in the same directory as the nethack.exe executable,
 * and loaded by specifying OPTIONS=altkeyhandler:nh340key
 * in defaults.nh
 */

static char where_to_get_source[] = "http://www.nethack.org/";
static char author[] = "The NetHack Development Team";

#include "win32api.h"
#include "hack.h"
#include "wintty.h"

extern HANDLE hConIn;
extern INPUT_RECORD ir;
char dllname[512];
char *shortdllname;

int __declspec(dllexport) __stdcall ProcessKeystroke(HANDLE hConIn,
               INPUT_RECORD *ir, boolean *valid, boolean numberpad, int portdebug);

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

    if (shiftstate & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) {
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
        if (vk == 0xBF)
            ch = M('?');
        else
            ch = M(tolower((uchar) keycode));
    }
    if (ch == '\r')
        ch = '\n';
#ifdef PORT_DEBUG
    if (portdebug) {
        char buf[BUFSZ];
        Sprintf(buf,
                "PORTDEBUG (%s): ch=%u, sc=%u, vk=%d, sh=0x%lX (ESC to end)",
                shortdllname, ch, scan, vk, shiftstate);
        fprintf(stdout, "\n%s", buf);
    }
#endif
    return ch;
}

int __declspec(dllexport) __stdcall 
NHkbhit(
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
                keycode = MapVirtualKey(vk, 2);
                if (shiftstate & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) {
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

int __declspec(dllexport) __stdcall CheckInput(
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
#if defined(SAFERHANGUP)
        dwWait = WaitForSingleObjectEx(hConIn,   // event object to wait for
                                       INFINITE, // waits indefinitely
                                       TRUE);    // alertable wait enabled
        if (dwWait == WAIT_FAILED)
            return '\033';
#endif
        ReadConsoleInput(hConIn, ir, 1, count);
        if (mode == 0) {
            if ((ir->EventType == KEY_EVENT) && ir->Event.KeyEvent.bKeyDown) {
#ifdef QWERTZ_SUPPORT
                if (qwertz)
                    numpad |= 0x10;
#endif
                ch = ProcessKeystroke(hConIn, ir, &valid, numpad, 0);
#ifdef QWERTZ_SUPPORT
                numpad &= ~0x10;
#endif                    
                done = valid;
            }
        } else {
            if (count > 0) {
                if (ir->EventType == KEY_EVENT
                    && ir->Event.KeyEvent.bKeyDown) {
                    ch = ProcessKeystroke(hConIn, ir, &valid, numpad, 0);
                    if (valid)
                        return ch;
                } else if (ir->EventType == MOUSE_EVENT) {
                    if ((ir->Event.MouseEvent.dwEventFlags == 0)
                        && (ir->Event.MouseEvent.dwButtonState & MOUSEMASK)) {
                        cc->x = ir->Event.MouseEvent.dwMousePosition.X + 1;
                        cc->y = ir->Event.MouseEvent.dwMousePosition.Y - 1;

                        if (ir->Event.MouseEvent.dwButtonState & LEFTBUTTON)
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
            } else
                done = 1;
        }
    }
    return mode ? 0 : ch;
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

int __declspec(dllexport) __stdcall
KeyHandlerName(char** buf, int full)
{
    if (!buf)
        return 0;
    if (full)
        *buf = dllname;
    else
        *buf = shortdllname;
    return 1;
}
