/*	SCCS Id: @(#)nttty.c	3.4	$Date$   */
/* Copyright (c) NetHack PC Development Team 1993    */
/* NetHack may be freely redistributed.  See license for details. */

/* tty.c - (Windows NT) version */

/*                                                  
 * Initial Creation 				M. Allison	1993/01/31 
 * Switch to low level console output routines	M. Allison	2003/10/01
 * Restrict cursor movement until input pending	M. Lehotay	2003/10/02
 *
 */

#ifdef WIN32CON
#define NEED_VARARGS /* Uses ... */
#include "hack.h"
#include "wintty.h"
#include <sys\types.h>
#include <sys\stat.h>
#include "win32api.h"

void FDECL(cmov, (int, int));
void FDECL(nocmov, (int, int));
int FDECL(process_keystroke, (INPUT_RECORD *, boolean *,
    BOOLEAN_P numberpad, int portdebug));

/*
 * The following WIN32 Console API routines are used in this file.
 *
 * CreateFile
 * GetConsoleScreenBufferInfo
 * GetStdHandle
 * SetConsoleCursorPosition
 * SetConsoleTextAttribute
 * SetConsoleCtrlHandler
 * PeekConsoleInput
 * ReadConsoleInput
 * WriteConsoleOutputCharacter
 * FillConsoleOutputAttribute
 */

/* Win32 Console handles for input and output */
HANDLE hConIn;
HANDLE hConOut;

/* Win32 Screen buffer,coordinate,console I/O information */
CONSOLE_SCREEN_BUFFER_INFO csbi, origcsbi;
COORD ntcoord;
INPUT_RECORD ir;

/* Flag for whether NetHack was launched via the GUI, not the command line.
 * The reason we care at all, is so that we can get
 * a final RETURN at the end of the game when launched from the GUI
 * to prevent the scoreboard (or panic message :-|) from vanishing
 * immediately after it is displayed, yet not bother when started
 * from the command line. 
 */
int GUILaunched;
static BOOL FDECL(CtrlHandler, (DWORD));

/* dynamic keystroke handling .DLL support */
typedef int (__stdcall * PROCESS_KEYSTROKE)(
    HANDLE,
    INPUT_RECORD *,
    boolean *,
    BOOLEAN_P,
    int
);

typedef int (__stdcall * NHKBHIT)(
    HANDLE,
    INPUT_RECORD *
);

typedef int (__stdcall * CHECKINPUT)(
	HANDLE,
	INPUT_RECORD *,
	DWORD *,
	BOOLEAN_P,
	int,
	int *,
	coord *
);

typedef int (__stdcall * SOURCEWHERE)(
    char **
);

typedef int (__stdcall * SOURCEAUTHOR)(
    char **
);

typedef int (__stdcall * KEYHANDLERNAME)(
    char **,
    int
);

HANDLE hLibrary;
PROCESS_KEYSTROKE pProcessKeystroke;
NHKBHIT pNHkbhit;
CHECKINPUT pCheckInput;
SOURCEWHERE pSourceWhere;
SOURCEAUTHOR pSourceAuthor;
KEYHANDLERNAME pKeyHandlerName;

#ifndef CLR_MAX
#define CLR_MAX 16
#endif
int ttycolors[CLR_MAX];
# ifdef TEXTCOLOR
static void NDECL(init_ttycolor);
# endif

#define MAX_OVERRIDES	256
unsigned char key_overrides[MAX_OVERRIDES];

static char nullstr[] = "";
char erase_char,kill_char;

#define DEFTEXTCOLOR  ttycolors[7]
static WORD background = 0;
static WORD foreground = (FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_RED);
static WORD attr = (FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_RED);
static DWORD ccount, acount;
static COORD cursor;

/*
 * Called after returning from ! or ^Z
 */
void
gettty()
{
#ifndef TEXTCOLOR
	int k;
#endif
	erase_char = '\b';
	kill_char = 21;		/* cntl-U */
	iflags.cbreak = TRUE;
#ifdef TEXTCOLOR
	init_ttycolor();
#else
	for(k=0; k < CLR_MAX; ++k)
		ttycolors[k] = 7;
#endif
}

/* reset terminal to original state */
void
settty(s)
const char *s;
{
	nocmov(ttyDisplay->curx, ttyDisplay->cury);
	end_screen();
	if(s) raw_print(s);
}

/* called by init_nhwindows() and resume_nhwindows() */
void
setftty()
{
	start_screen();
}

void
tty_startup(wid, hgt)
int *wid, *hgt;
{
	int twid = origcsbi.srWindow.Right - origcsbi.srWindow.Left + 1;

	if (twid > 80) twid = 80;
	*wid = twid;
	*hgt = origcsbi.srWindow.Bottom - origcsbi.srWindow.Top + 1;
	set_option_mod_status("mouse_support", SET_IN_GAME);
}

void
tty_number_pad(state)
int state;
{
}

void
tty_start_screen()
{
	if (iflags.num_pad) tty_number_pad(1);	/* make keypad send digits */
}

void
tty_end_screen()
{
	clear_screen();
	if (GetConsoleScreenBufferInfo(hConOut,&csbi))
	{
	    DWORD ccnt;
	    COORD newcoord;
	    
	    newcoord.X = 0;
	    newcoord.Y = 0;
	    FillConsoleOutputAttribute(hConOut,
	    		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
	    		csbi.dwSize.X * csbi.dwSize.Y,
	    		newcoord, &ccnt);
	    FillConsoleOutputCharacter(hConOut,' ',
			csbi.dwSize.X * csbi.dwSize.Y,
			newcoord, &ccnt);
	}
	FlushConsoleInputBuffer(hConIn);
}

extern boolean getreturn_disable;	/* from sys/share/pcsys.c */

static BOOL CtrlHandler(ctrltype)
DWORD ctrltype;
{
	switch(ctrltype) {
	/*	case CTRL_C_EVENT: */
		case CTRL_BREAK_EVENT:
			clear_screen();
		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			getreturn_disable = TRUE;
#ifndef NOSAVEONHANGUP
			hangup(0);
#endif
#if defined(SAFERHANGUP)
			CloseHandle(hConIn);	/* trigger WAIT_FAILED */
			return TRUE;
#endif
		default:
			return FALSE;
	}
}

/* called by init_tty in wintty.c for WIN32CON port only */
void
nttty_open()
{
        HANDLE hStdOut;
        DWORD cmode;
        long mask;

	load_keyboard_handler();
	/* Initialize the function pointer that points to
         * the kbhit() equivalent, in this TTY case nttty_kbhit()
         */
	nt_kbhit = nttty_kbhit;

        /* The following 6 lines of code were suggested by 
         * Bob Landau of Microsoft WIN32 Developer support,
         * as the only current means of determining whether
         * we were launched from the command prompt, or from
         * the NT program manager. M. Allison
         */
        hStdOut = GetStdHandle( STD_OUTPUT_HANDLE );
        GetConsoleScreenBufferInfo( hStdOut, &origcsbi);
        GUILaunched = ((origcsbi.dwCursorPosition.X == 0) &&
                           (origcsbi.dwCursorPosition.Y == 0));
        if ((origcsbi.dwSize.X <= 0) || (origcsbi.dwSize.Y <= 0))
            GUILaunched = 0;

        /* Obtain handles for the standard Console I/O devices */
	hConIn = GetStdHandle(STD_INPUT_HANDLE);
	hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
#if 0
	hConIn = CreateFile("CONIN$",
			GENERIC_READ |GENERIC_WRITE,
			FILE_SHARE_READ |FILE_SHARE_WRITE,
			0, OPEN_EXISTING, 0, 0);					
	hConOut = CreateFile("CONOUT$",
			GENERIC_READ |GENERIC_WRITE,
			FILE_SHARE_READ |FILE_SHARE_WRITE,
			0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,0);
#endif       

	GetConsoleMode(hConIn,&cmode);
#ifdef NO_MOUSE_ALLOWED
	mask = ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT |
	       ENABLE_MOUSE_INPUT | ENABLE_ECHO_INPUT | ENABLE_WINDOW_INPUT;   
#else
	mask = ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT |
	       ENABLE_ECHO_INPUT | ENABLE_WINDOW_INPUT;   
#endif
	/* Turn OFF the settings specified in the mask */
	cmode &= ~mask;
#ifndef NO_MOUSE_ALLOWED
	cmode |= ENABLE_MOUSE_INPUT;
#endif
	SetConsoleMode(hConIn,cmode);
	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE)) {
		/* Unable to set control handler */
		cmode = 0; 	/* just to have a statement to break on for debugger */
	}
	get_scr_size();
}

int process_keystroke(ir, valid, numberpad, portdebug)
INPUT_RECORD *ir;
boolean *valid;
boolean numberpad;
int portdebug;
{
	int ch = pProcessKeystroke(hConIn, ir, valid, numberpad, portdebug);
	/* check for override */
	if (ch && ch < MAX_OVERRIDES && key_overrides[ch])
		ch = key_overrides[ch];
	return ch;
}

int
nttty_kbhit()
{
	return pNHkbhit(hConIn, &ir);
}


void
get_scr_size()
{
	GetConsoleScreenBufferInfo(hConOut, &csbi);
  
	LI = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	CO = csbi.srWindow.Right - csbi.srWindow.Left + 1;

	if ( (LI < 25) || (CO < 80) ) {
		COORD newcoord;
    
		LI = 25;
		CO = 80;

		newcoord.Y = LI;
		newcoord.X = CO;

		SetConsoleScreenBufferSize( hConOut, newcoord );
	}
}

int
tgetch()
{
	int mod;
	coord cc;
	DWORD count;
	nocmov(ttyDisplay->curx, ttyDisplay->cury);
	return (program_state.done_hup) ?
		'\033' :
		pCheckInput(hConIn, &ir, &count, iflags.num_pad, 0, &mod, &cc);
}

int
ntposkey(x, y, mod)
int *x, *y, *mod;
{
	int ch;
	coord cc;
	DWORD count;
	nocmov(ttyDisplay->curx, ttyDisplay->cury);
	ch = (program_state.done_hup) ?
		'\033' :
		pCheckInput(hConIn, &ir, &count, iflags.num_pad, 1, mod, &cc);
	if (!ch) {
		*x = cc.x;
		*y = cc.y;
	}
	return ch;
}

void
cmov(x, y)
register int x, y;
{
	ttyDisplay->curx = x;
	ttyDisplay->cury = y;
	cursor.X = x;
	cursor.Y = y;
	SetConsoleCursorPosition(hConOut, cursor);
}

void
nocmov(x, y)
int x,y;
{
	cursor.Y = y;
	cursor.X = x;
	SetConsoleCursorPosition(hConOut, cursor);
}

void
xputc(ch)
char ch;
{
	cursor.X = ttyDisplay->curx;
	cursor.Y = ttyDisplay->cury;
	switch(ch) {
	    case '\n':
	    case '\r':
		    cmov(cursor.X, cursor.Y);
		    return;
	}
	WriteConsoleOutputAttribute(hConOut,&attr,1,cursor,&acount);
	WriteConsoleOutputCharacter(hConOut,&ch,1,cursor,&ccount);
}

void
xputs(s)
const char *s;
{
	int slen = strlen(s);
	cursor.X = ttyDisplay->curx;
	cursor.Y = ttyDisplay->cury;
	FillConsoleOutputAttribute(hConOut,attr,slen,cursor,&acount);
	WriteConsoleOutputCharacter(hConOut,s,slen,cursor,&ccount);
}

/*
 * Overrides winntty.c function of the same name
 * for win32. It is used for glyphs only, not text.
 */
void
g_putch(in_ch)
int in_ch;
{
	char ch = (char)in_ch;
	cursor.X = ttyDisplay->curx;
	cursor.Y = ttyDisplay->cury;
#if 0
	switch(ch) {
	    case '\n':
	    case '\r':
		    cmov(cursor.X, cursor.Y);
		    return;
	}
#endif
	WriteConsoleOutputAttribute(hConOut,&attr,1,cursor,&acount);
	WriteConsoleOutputCharacter(hConOut,&ch,1,cursor,&ccount);
}

void
cl_end()
{
	int cx;
	cursor.X = ttyDisplay->curx;
	cursor.Y = ttyDisplay->cury;
	cx = CO - cursor.X;
	FillConsoleOutputAttribute(hConOut, DEFTEXTCOLOR, cx, cursor, &acount);
	FillConsoleOutputCharacter(hConOut,' ', cx, cursor,&ccount);
	tty_curs(BASE_WINDOW, (int)ttyDisplay->curx+1,
			(int)ttyDisplay->cury);
}


void
clear_screen()
{
	if (GetConsoleScreenBufferInfo(hConOut,&csbi)) {
	    DWORD ccnt;
	    COORD newcoord;
	    
	    newcoord.X = 0;
	    newcoord.Y = 0;
	    FillConsoleOutputAttribute(hConOut,
	    		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
	    		csbi.dwSize.X * csbi.dwSize.Y,
	    		newcoord, &ccnt);
	    FillConsoleOutputCharacter(hConOut,' ',
			csbi.dwSize.X * csbi.dwSize.Y,
			newcoord, &ccnt);
	}
	home();
}


void
home()
{
	cursor.X = cursor.Y = 0;
	ttyDisplay->curx = ttyDisplay->cury = 0;
}


void
backsp()
{
 	GetConsoleScreenBufferInfo(hConOut,&csbi);
 	if (csbi.dwCursorPosition.X > 0)
 		ntcoord.X = csbi.dwCursorPosition.X-1;
 	ntcoord.Y = csbi.dwCursorPosition.Y;
 	SetConsoleCursorPosition(hConOut,ntcoord);
}

void
cl_eos()
{
	register int cy = ttyDisplay->cury+1;		
	if (GetConsoleScreenBufferInfo(hConOut,&csbi)) {
	    DWORD ccnt;
	    COORD newcoord;
	    
	    newcoord.X = 0;
	    newcoord.Y = ttyDisplay->cury;
	    FillConsoleOutputAttribute(hConOut,
	    		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
	    		csbi.dwSize.X * csbi.dwSize.Y - cy,
	    		newcoord, &ccnt);
	    newcoord.X = 0;
	    newcoord.Y = ttyDisplay->cury;
	    FillConsoleOutputCharacter(hConOut,' ',
			csbi.dwSize.X * csbi.dwSize.Y - cy,
			newcoord, &ccnt);
	}
	tty_curs(BASE_WINDOW, (int)ttyDisplay->curx+1, (int)ttyDisplay->cury);
}

void
tty_nhbell()
{
	if (flags.silent) return;
	Beep(8000,500);
}

volatile int junk;	/* prevent optimizer from eliminating loop below */

void
tty_delay_output()
{
	/* delay 50 ms - uses ANSI C clock() function now */
	clock_t goal;
	int k;

	goal = 50 + clock();
	while (goal > clock()) {
	    k = junk;  /* Do nothing */
	}
}

# ifdef TEXTCOLOR
/*
 * CLR_BLACK		0
 * CLR_RED		1
 * CLR_GREEN		2
 * CLR_BROWN		3	low-intensity yellow
 * CLR_BLUE		4
 * CLR_MAGENTA 		5
 * CLR_CYAN		6
 * CLR_GRAY		7	low-intensity white
 * NO_COLOR		8
 * CLR_ORANGE		9
 * CLR_BRIGHT_GREEN	10
 * CLR_YELLOW		11
 * CLR_BRIGHT_BLUE	12
 * CLR_BRIGHT_MAGENTA  	13
 * CLR_BRIGHT_CYAN	14
 * CLR_WHITE		15
 * CLR_MAX		16
 * BRIGHT		8
 */

static void
init_ttycolor()
{
	ttycolors[CLR_BLACK] = FOREGROUND_INTENSITY;  /* fix by Quietust */
	ttycolors[CLR_RED] = FOREGROUND_RED;
	ttycolors[CLR_GREEN] = FOREGROUND_GREEN;
	ttycolors[CLR_BROWN] = FOREGROUND_GREEN|FOREGROUND_RED;
	ttycolors[CLR_BLUE] = FOREGROUND_BLUE;
	ttycolors[CLR_MAGENTA] = FOREGROUND_BLUE|FOREGROUND_RED;
	ttycolors[CLR_CYAN] = FOREGROUND_GREEN|FOREGROUND_BLUE;
	ttycolors[CLR_GRAY] = FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_BLUE;
	ttycolors[BRIGHT] = FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_RED|\
						FOREGROUND_INTENSITY;
	ttycolors[CLR_ORANGE] = FOREGROUND_RED|FOREGROUND_INTENSITY;
	ttycolors[CLR_BRIGHT_GREEN] = FOREGROUND_GREEN|FOREGROUND_INTENSITY;
	ttycolors[CLR_YELLOW] = FOREGROUND_GREEN|FOREGROUND_RED|\
						FOREGROUND_INTENSITY;
	ttycolors[CLR_BRIGHT_BLUE] = FOREGROUND_BLUE|FOREGROUND_INTENSITY;
	ttycolors[CLR_BRIGHT_MAGENTA] = FOREGROUND_BLUE|FOREGROUND_RED|\
						FOREGROUND_INTENSITY;
	ttycolors[CLR_BRIGHT_CYAN] = FOREGROUND_GREEN|FOREGROUND_BLUE|\
						FOREGROUND_INTENSITY;
	ttycolors[CLR_WHITE] = FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_RED|\
						FOREGROUND_INTENSITY;
}
# endif /* TEXTCOLOR */

int
has_color(int color)
{
# ifdef TEXTCOLOR
    return 1;
# else
    if (color == CLR_BLACK)
    	return 1;
    else if (color == CLR_WHITE)
	return 1;
    else
	return 0;
# endif 
}

void
term_start_attr(int attrib)
{
    switch(attrib){
        case ATR_INVERSE:
		if (iflags.wc_inverse) {
		   /* Suggestion by Lee Berger */
		   if ((foreground & (FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_RED)) ==
			(FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_RED))
			foreground &= ~(FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_RED);
		   background = (BACKGROUND_RED|BACKGROUND_BLUE|BACKGROUND_GREEN);
		   break;
		}
		/*FALLTHRU*/
        case ATR_ULINE:
        case ATR_BLINK:
        case ATR_BOLD:
		foreground |= FOREGROUND_INTENSITY;
                break;
        default:
		foreground &= ~FOREGROUND_INTENSITY;
                break;
    }
    attr = (foreground | background);
}

void
term_end_attr(int attrib)
{
    switch(attrib){

        case ATR_INVERSE:
		if ((foreground & (FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_RED)) == 0)
		     foreground |= (FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_RED);
		background = 0;
		break;
        case ATR_ULINE:
        case ATR_BLINK:
        case ATR_BOLD:
		foreground &= ~FOREGROUND_INTENSITY;
		break;
    }                
    attr = (foreground | background);
}

void
term_end_raw_bold(void)
{
    term_end_attr(ATR_BOLD);
}

void
term_start_raw_bold(void)
{
    term_start_attr(ATR_BOLD);
}

void
term_start_color(int color)
{
#ifdef TEXTCOLOR
        if (color >= 0 && color < CLR_MAX) {
	    foreground = (background != 0 && (color == CLR_GRAY || color == CLR_WHITE)) ?
			ttycolors[0] : ttycolors[color];
	}
#else
	foreground = DEFTEXTCOLOR;
#endif
	attr = (foreground | background);
}

void
term_end_color(void)
{
#ifdef TEXTCOLOR
	foreground = DEFTEXTCOLOR;
#endif
	attr = (foreground | background);
}


void
standoutbeg()
{
    term_start_attr(ATR_BOLD);
}


void
standoutend()
{
    term_end_attr(ATR_BOLD);
}

#ifndef NO_MOUSE_ALLOWED
void
toggle_mouse_support()
{
        DWORD cmode;
	GetConsoleMode(hConIn,&cmode);
	if (iflags.wc_mouse_support)
		cmode |= ENABLE_MOUSE_INPUT;
	else
		cmode &= ~ENABLE_MOUSE_INPUT;
	SetConsoleMode(hConIn,cmode);
}
#endif

/* handle tty options updates here */
void nttty_preference_update(pref)
const char *pref;
{
	if( stricmp( pref, "mouse_support")==0) {
#ifndef NO_MOUSE_ALLOWED
		toggle_mouse_support();
#endif
	}
	return;
}

#ifdef PORT_DEBUG
void
win32con_debug_keystrokes()
{
	DWORD count;
	boolean valid = 0;
	int ch;
	xputs("\n");
	while (!valid || ch != 27) {
	   nocmov(ttyDisplay->curx, ttyDisplay->cury);
	   ReadConsoleInput(hConIn,&ir,1,&count);
	   if ((ir.EventType == KEY_EVENT) && ir.Event.KeyEvent.bKeyDown)
		ch = process_keystroke(&ir, &valid, iflags.num_pad, 1);
	}
	(void)doredraw();
}
void
win32con_handler_info()
{
	char *buf;
	int ci;
	if (!pSourceAuthor && !pSourceWhere)
	    pline("Keyboard handler source info and author unavailable.");
	else {
		if (pKeyHandlerName && pKeyHandlerName(&buf, 1)) {
			xputs("\n");
			xputs("Keystroke handler loaded: \n    ");
			xputs(buf);
		}
		if (pSourceAuthor && pSourceAuthor(&buf)) {
			xputs("\n");
			xputs("Keystroke handler Author: \n    ");
			xputs(buf);
		}
		if (pSourceWhere && pSourceWhere(&buf)) {
			xputs("\n");
			xputs("Keystroke handler source code available at:\n    ");
			xputs(buf);
		}
		xputs("\nPress any key to resume.");
		ci=nhgetch();
		(void)doredraw();
	}
}
#endif

void
map_subkeyvalue(op)
register char *op;
{
	char digits[] = "0123456789";
	int length, i, idx, val;
	char *kp;

	idx = -1;
	val = -1;
	kp = index(op, '/');
	if (kp) {
		*kp = '\0';
		kp++;
		length = strlen(kp);
		if (length < 1 || length > 3) return;
		for (i = 0; i < length; i++)
			if (!index(digits, kp[i])) return;
		val = atoi(kp);
		length = strlen(op);
		if (length < 1 || length > 3) return;
		for (i = 0; i < length; i++)
			if (!index(digits, op[i])) return;
		idx = atoi(op);
	}
	if (idx >= MAX_OVERRIDES || idx < 0 || val >= MAX_OVERRIDES || val < 1)
		return;
	key_overrides[idx] = val;
}

void
load_keyboard_handler()
{
	char suffx[] = ".dll";
	char *truncspot;
#define MAX_DLLNAME 25
	char kh[MAX_ALTKEYHANDLER];
	if (iflags.altkeyhandler[0]) {
		if (hLibrary) {	/* already one loaded apparently */
			FreeLibrary(hLibrary);
			hLibrary = (HANDLE)0;
		   pNHkbhit = (NHKBHIT)0;
		   pCheckInput = (CHECKINPUT)0; 
		   pSourceWhere = (SOURCEWHERE)0; 
		   pSourceAuthor = (SOURCEAUTHOR)0; 
		   pKeyHandlerName = (KEYHANDLERNAME)0; 
		   pProcessKeystroke = (PROCESS_KEYSTROKE)0;
		}
		if ((truncspot = strstri(iflags.altkeyhandler, suffx)) != 0)
			*truncspot = '\0';
		(void) strncpy(kh, iflags.altkeyhandler,
				(MAX_ALTKEYHANDLER - sizeof suffx) - 1);
		kh[(MAX_ALTKEYHANDLER - sizeof suffx) - 1] = '\0';
		Strcat(kh, suffx);
		Strcpy(iflags.altkeyhandler, kh);
		hLibrary = LoadLibrary(kh);
		if (hLibrary) {
		   pProcessKeystroke =
		   (PROCESS_KEYSTROKE) GetProcAddress (hLibrary, TEXT ("ProcessKeystroke"));
		   pNHkbhit =
		   (NHKBHIT) GetProcAddress (hLibrary, TEXT ("NHkbhit"));
		   pCheckInput =
		   (CHECKINPUT) GetProcAddress (hLibrary, TEXT ("CheckInput"));
		   pSourceWhere =
		   (SOURCEWHERE) GetProcAddress (hLibrary, TEXT ("SourceWhere"));
		   pSourceAuthor =
		   (SOURCEAUTHOR) GetProcAddress (hLibrary, TEXT ("SourceAuthor"));
		   pKeyHandlerName =
		   (KEYHANDLERNAME) GetProcAddress (hLibrary, TEXT ("KeyHandlerName"));
		}
	}
	if (!pProcessKeystroke || !pNHkbhit || !pCheckInput) {
		if (hLibrary) {
			FreeLibrary(hLibrary);
			hLibrary = (HANDLE)0;
			pNHkbhit = (NHKBHIT)0; 
			pCheckInput = (CHECKINPUT)0; 
			pSourceWhere = (SOURCEWHERE)0; 
			pSourceAuthor = (SOURCEAUTHOR)0; 
			pKeyHandlerName = (KEYHANDLERNAME)0; 
			pProcessKeystroke = (PROCESS_KEYSTROKE)0;
		}
		(void)strncpy(kh, "nhdefkey.dll", (MAX_ALTKEYHANDLER - sizeof suffx) - 1);
		kh[(MAX_ALTKEYHANDLER - sizeof suffx) - 1] = '\0';
		Strcpy(iflags.altkeyhandler, kh);
		hLibrary = LoadLibrary(kh);
		if (hLibrary) {
		   pProcessKeystroke =
		   (PROCESS_KEYSTROKE) GetProcAddress (hLibrary, TEXT ("ProcessKeystroke"));
		   pCheckInput =
		   (CHECKINPUT) GetProcAddress (hLibrary, TEXT ("CheckInput"));
		   pNHkbhit =
		   (NHKBHIT) GetProcAddress (hLibrary, TEXT ("NHkbhit"));
		   pSourceWhere =
		   (SOURCEWHERE) GetProcAddress (hLibrary, TEXT ("SourceWhere"));
		   pSourceAuthor =
		   (SOURCEAUTHOR) GetProcAddress (hLibrary, TEXT ("SourceAuthor"));
		   pKeyHandlerName =
		   (KEYHANDLERNAME) GetProcAddress (hLibrary, TEXT ("KeyHandlerName"));
		}
	}
	if (!pProcessKeystroke || !pNHkbhit || !pCheckInput) {
		if (!hLibrary)
			raw_printf("\nNetHack was unable to load keystroke handler.\n");
		else {
			FreeLibrary(hLibrary);
			hLibrary = (HANDLE)0;
			raw_printf("\nNetHack keystroke handler is invalid.\n");
		}
		exit(EXIT_FAILURE);
	}
}

#endif /* WIN32CON */
