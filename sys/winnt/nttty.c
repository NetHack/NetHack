/*	SCCS Id: @(#)nttty.c	3.4	$Date$   */
/* Copyright (c) NetHack PC Development Team 1993    */
/* NetHack may be freely redistributed.  See license for details. */

/* tty.c - (Windows NT) version */
/*                                                  
 * Initial Creation 				M. Allison	93/01/31 
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
 * WriteConsole
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
	int *,
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

#define DEFTEXTCOLOR  ttycolors[7]
#ifdef TEXTCOLOR
#define DEFGLYPHBGRND (0)
#else
#define DEFGLYPHBGRND (0)
#endif

static char nullstr[] = "";
char erase_char,kill_char;

static char currentcolor = FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_BLUE;
static char noninvertedcurrentcolor = FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_BLUE;
static char currenthilite = 0;
static char currentbackground = 0;
static boolean colorchange = TRUE;

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
	end_screen();
	if(s) raw_print(s);
}

/*
 * mode == 0	set processed console output mode.
 * mode == 1	set raw console output mode (no control character expansion).
 */
void
set_output_mode(mode)
int mode;
{
	static DWORD save_output_cmode = 0;
	static boolean initmode = FALSE;
	DWORD cmode, mask = ENABLE_PROCESSED_OUTPUT;
	if (!initmode) {
		/* fetch original output mode */
		GetConsoleMode(hConOut,&save_output_cmode);
		initmode = TRUE;
	}
	if (mode == 0) {
		cmode = save_output_cmode;
		/* Turn ON the settings specified in the mask */
		cmode |= mask;
		SetConsoleMode(hConOut,cmode);
		iflags.rawio = 0;
	} else {
		cmode = save_output_cmode;
		/* Turn OFF the settings specified in the mask */
		cmode &= ~mask;
		SetConsoleMode(hConOut,cmode);
		iflags.rawio = 1;
	}
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
/*	int twid = origcsbi.dwSize.X; */
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
			set_output_mode(0);  /* Allow processed output */
			getreturn_disable = TRUE;
#ifndef NOSAVEONHANGUP
			hangup(0);
#endif
#if 0
			clearlocks();
			terminate(EXIT_FAILURE);
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
	hConIn = GetStdHandle(STD_INPUT_HANDLE);
	hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
#if 0
        /* Obtain handles for the standard Console I/O devices */
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

	set_output_mode(1);	/* raw output mode; no tab expansion */
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
	return pCheckInput(hConIn, &ir, &count, iflags.num_pad, 0, &mod, &cc);
}

int
ntposkey(x, y, mod)
int *x, *y, *mod;
{
	int ch;
	coord cc;
	DWORD count;
	ch = pCheckInput(hConIn, &ir, &count, iflags.num_pad, 1, mod, &cc);
	if (!ch) {
		*x = cc.x;
		*y = cc.y;
	}
	return ch;
}

void
nocmov(x, y)
int x,y;
{
	ntcoord.X = x;
	ntcoord.Y = y;
	SetConsoleCursorPosition(hConOut,ntcoord);
}

void
cmov(x, y)
register int x, y;
{
	ntcoord.X = x;
	ntcoord.Y = y;
	SetConsoleCursorPosition(hConOut,ntcoord);
	ttyDisplay->cury = y;
	ttyDisplay->curx = x;
}

void
xputc(c)
char c;
{
	DWORD count;

	switch(c) {
	    case '\n':
	    case '\r':
		    cmov(ttyDisplay->curx, ttyDisplay->cury);
		    return;
	}
	if (colorchange) {
		SetConsoleTextAttribute(hConOut,
			(currentcolor | currenthilite | currentbackground));
		colorchange = FALSE;
	}
	WriteConsole(hConOut,&c,1,&count,0);
}

void
xputs(s)
const char *s;
{
	DWORD count;
	if (colorchange) {
		SetConsoleTextAttribute(hConOut,
			(currentcolor | currenthilite | currentbackground));
		colorchange = FALSE;
	}
	WriteConsole(hConOut,s,strlen(s),&count,0);
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
    DWORD count = 1;
    int tcolor;
    int bckgnd = currentbackground;

    if (colorchange) {
	tcolor = currentcolor | bckgnd | currenthilite;
	SetConsoleTextAttribute(hConOut, tcolor);
    }
    WriteConsole(hConOut,&ch,1,&count,0);
    colorchange = TRUE;		/* force next output back to current nethack values */
    return;
}

void
cl_end()
{
		DWORD count;

		ntcoord.X = ttyDisplay->curx;
		ntcoord.Y = ttyDisplay->cury;
	    	FillConsoleOutputAttribute(hConOut, DEFTEXTCOLOR,
	    		CO - ntcoord.X,ntcoord, &count);
		ntcoord.X = ttyDisplay->curx;
		ntcoord.Y = ttyDisplay->cury;
		FillConsoleOutputCharacter(hConOut,' ',
			CO - ntcoord.X,ntcoord,&count);
		tty_curs(BASE_WINDOW, (int)ttyDisplay->curx+1,
						(int)ttyDisplay->cury);
		colorchange = TRUE;
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
	    newcoord.X = 0;
	    newcoord.Y = 0;
	    FillConsoleOutputCharacter(hConOut,' ',
			csbi.dwSize.X * csbi.dwSize.Y,
			newcoord, &ccnt);
	}
	colorchange = TRUE;
	home();
}


void
home()
{
	tty_curs(BASE_WINDOW, 1, 0);
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
	/* colorchange shouldn't ever happen here but.. */
	if (colorchange) {
		SetConsoleTextAttribute(hConOut,
				(currentcolor|currenthilite|currentbackground));
		colorchange = FALSE;
	}
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

void
cl_eos()
{
	    register int cy = ttyDisplay->cury+1;		
#if 0
		while(cy <= LI-2) {
			cl_end();
			xputc('\n');
			cy++;
		}
		cl_end();
#else
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
	colorchange = TRUE;
#endif
		tty_curs(BASE_WINDOW, (int)ttyDisplay->curx+1,
						(int)ttyDisplay->cury);
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
	ttycolors[CLR_BLUE] = FOREGROUND_BLUE|FOREGROUND_INTENSITY;
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
	ttycolors[CLR_BRIGHT_CYAN] = FOREGROUND_GREEN|FOREGROUND_BLUE;
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
term_start_attr(int attr)
{
    switch(attr){
        case ATR_INVERSE:
		if (iflags.wc_inverse) {
 		   noninvertedcurrentcolor = currentcolor;
		   /* Suggestion by Lee Berger */
		   if ((currentcolor & (FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_RED)) ==
			(FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_RED))
			currentcolor = 0;
		   currentbackground = (BACKGROUND_RED|BACKGROUND_BLUE|BACKGROUND_GREEN);
		   colorchange = TRUE;
		   break;
		} /* else */
		/*FALLTHRU*/
        case ATR_ULINE:
        case ATR_BOLD:
        case ATR_BLINK:
		standoutbeg();
                break;
        default:
#ifdef DEBUG
		impossible("term_start_attr: strange attribute %d", attr);
#endif
		standoutend();
		if (currentcolor == 0)
			currentcolor = FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_RED;
		currentbackground = 0;
                break;
    }
}

void
term_end_attr(int attr)
{
    switch(attr){

        case ATR_INVERSE:
       	  if (iflags.wc_inverse) {
			if (currentcolor == 0 && noninvertedcurrentcolor != 0)
				currentcolor = noninvertedcurrentcolor;
			noninvertedcurrentcolor = 0;
		    currentbackground = 0;
		    colorchange = TRUE;
		    break;
		  } /* else */
		/*FALLTHRU*/
        case ATR_ULINE:
        case ATR_BOLD:
        case ATR_BLINK:
        default:
		standoutend();
		if (currentcolor == 0)
			currentcolor = FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_RED;
		currentbackground = 0;
                break;
    }                
}

void
term_end_raw_bold(void)
{
    standoutend();    
}

void
term_start_raw_bold(void)
{
    standoutbeg();
}

void
term_start_color(int color)
{
# ifdef TEXTCOLOR
        if (color >= 0 && color < CLR_MAX) {
	    currentcolor = ttycolors[color];
	    colorchange = TRUE;
	}
# endif
}

void
term_end_color(void)
{
# ifdef TEXTCOLOR
	currentcolor = DEFTEXTCOLOR;
	colorchange = TRUE;
# endif
}


void
standoutbeg()
{
	currenthilite = FOREGROUND_INTENSITY;
	colorchange = TRUE;
}


void
standoutend()
{
	currenthilite = 0;
	colorchange = TRUE;
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
	   ReadConsoleInput(hConIn,&ir,1,&count);
	   if ((ir.EventType == KEY_EVENT) && ir.Event.KeyEvent.bKeyDown)
		ch = process_keystroke(&ir, &valid, 1);
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
