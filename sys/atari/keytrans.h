/* NetHack 5.0	keytrans.h	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Atari keypad translation tables, shared by the BIOS/TTY input path
 * (sys/atari/tos.c) and the GEM input path (win/gem/wingem1.c).
 */

#ifndef KEYTRANS_H
#define KEYTRANS_H

#ifndef C
#define C(c) (0x1f & (c))
#endif
#ifndef M
#define M(c) (0x80 | (c))
#endif

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

#endif /* KEYTRANS_H */
