/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file implements the interface between the window port specific
 * code in the mswin port and the rest of the nethack game engine. 
*/

#include "hack.h"
#include "dlb.h"
#include "winMS.h"
#include "mhmap.h"
#include "mhstatus.h"
#include "mhtext.h"
#include "mhmsgwnd.h"
#include "mhmenu.h"
#include "mhmsg.h"
#include "mhinput.h"
#include "mhaskyn.h"
#include "mhdlg.h"
#include "mhrip.h"

#define LLEN 128

#ifdef _DEBUG
extern void logDebug(const char *fmt, ...);
#else
void logDebug(const char *fmt, ...) { }
#endif

static void mswin_main_loop(void);

/* Interface definition, for windows.c */
struct window_procs mswin_procs = {
    "MSWIN",
    WC_COLOR|WC_HILITE_PET|WC_ALIGN_MESSAGE|WC_ALIGN_STATUS|
	WC_INVERSE|WC_SCROLL_MARGIN|WC_MAP_MODE|
	WC_FONT_MESSAGE|WC_FONT_STATUS|WC_FONT_MENU|WC_FONT_TEXT|
	WC_FONTSIZ_MESSAGE|WC_FONTSIZ_STATUS|WC_FONTSIZ_MENU|WC_FONTSIZ_TEXT|
	WC_TILE_WIDTH|WC_TILE_HEIGHT|WC_TILE_FILE,
    mswin_init_nhwindows,
    mswin_player_selection,
    mswin_askname,
    mswin_get_nh_event,
    mswin_exit_nhwindows,
    mswin_suspend_nhwindows,
    mswin_resume_nhwindows,
    mswin_create_nhwindow,
    mswin_clear_nhwindow,
    mswin_display_nhwindow,
    mswin_destroy_nhwindow,
    mswin_curs,
    mswin_putstr,
    mswin_display_file,
    mswin_start_menu,
    mswin_add_menu,
    mswin_end_menu,
    mswin_select_menu,
    genl_message_menu,		/* no need for X-specific handling */
    mswin_update_inventory,
    mswin_mark_synch,
    mswin_wait_synch,
#ifdef CLIPPING
    mswin_cliparound,
#endif
#ifdef POSITIONBAR
    donull,
#endif
    mswin_print_glyph,
    mswin_raw_print,
    mswin_raw_print_bold,
    mswin_nhgetch,
    mswin_nh_poskey,
    mswin_nhbell,
    mswin_doprev_message,
    mswin_yn_function,
    mswin_getlin,
    mswin_get_ext_cmd,
    mswin_number_pad,
    mswin_delay_output,
#ifdef CHANGE_COLOR	/* only a Mac option currently */
	mswin,
	mswin_change_background,
#endif
    /* other defs that really should go away (they're tty specific) */
    mswin_start_screen,
    mswin_end_screen,
    mswin_outrip,
    genl_preference_update,
};


/*  
init_nhwindows(int* argcp, char** argv)
                -- Initialize the windows used by NetHack.  This can also
                   create the standard windows listed at the top, but does
                   not display them.
                -- Any commandline arguments relevant to the windowport
                   should be interpreted, and *argcp and *argv should
                   be changed to remove those arguments.
                -- When the message window is created, the variable
                   iflags.window_inited needs to be set to TRUE.  Otherwise
                   all plines() will be done via raw_print().
                ** Why not have init_nhwindows() create all of the "standard"
                ** windows?  Or at least all but WIN_INFO?      -dean
*/
void mswin_init_nhwindows(int* argc, char** argv)
{
	logDebug("mswin_init_nhwindows()\n");

#ifdef _DEBUG
	{
		/* truncate trace file */
		FILE *dfp = fopen("nhtrace.log", "w");
		fclose(dfp);
	}
#endif
    mswin_nh_input_init();

	iflags.window_inited = TRUE;
}


/* Do a window-port specific player type selection. If player_selection()
   offers a Quit option, it is its responsibility to clean up and terminate
   the process. You need to fill in pl_character[0].
*/
void mswin_player_selection(void)
{
	int nRole;

	logDebug("mswin_player_selection()\n");

	/* select a role */
	if( mswin_player_selection_window( &nRole ) == IDCANCEL ) {
		bail(0);
	}
}


/* Ask the user for a player name. */
void mswin_askname(void)
{
	logDebug("mswin_askname()\n");

	if( mswin_getlin_window("Who are you?", plname, PL_NSIZ)==IDCANCEL ) {
		bail("bye-bye");
		/* not reached */
	}
}


/* Does window event processing (e.g. exposure events).
   A noop for the tty and X window-ports.
*/
void mswin_get_nh_event(void)
{
	logDebug("mswin_get_nh_event()\n");
	return;
}

/* Exits the window system.  This should dismiss all windows,
   except the "window" used for raw_print().  str is printed if possible.
*/
void mswin_exit_nhwindows(const char *str)
{
	logDebug("mswin_exit_nhwindows(%s)\n", str);
}

/* Prepare the window to be suspended. */
void mswin_suspend_nhwindows(const char *str)
{
	logDebug("mswin_suspend_nhwindows(%s)\n", str);
	return;
}


/* Restore the windows after being suspended. */
void mswin_resume_nhwindows()
{
	logDebug("mswin_resume_nhwindows()\n");
	return;
}

/*  Create a window of type "type" which can be 
        NHW_MESSAGE     (top line)
        NHW_STATUS      (bottom lines)
        NHW_MAP         (main dungeon)
        NHW_MENU        (inventory or other "corner" windows)
        NHW_TEXT        (help/text, full screen paged window)
*/
winid 
mswin_create_nhwindow(int type)
{
	winid i = 0;
	MSNHMsgAddWnd data;

	logDebug("mswin_create_nhwindow(%d)\n", type);

	/* Return the next available winid
	 */

	for (i=1; i<MAXWINDOWS; i++)
	  if (GetNHApp()->windowlist[i].win == NULL &&
		  !GetNHApp()->windowlist[i].dead)
		  break;
	if (i == MAXWINDOWS)
	  panic ("ERROR:  No windows available...\n");

    switch (type) {
    case NHW_MAP:
	{
		GetNHApp()->windowlist[i].win = mswin_init_map_window();
		GetNHApp()->windowlist[i].type = type;
		GetNHApp()->windowlist[i].dead = 0;
		break;
	}
    case NHW_MESSAGE:
	{
		GetNHApp()->windowlist[i].win = mswin_init_message_window();
		GetNHApp()->windowlist[i].type = type;
		GetNHApp()->windowlist[i].dead = 0;
		break;
	}
    case NHW_STATUS:
	{
		GetNHApp()->windowlist[i].win = mswin_init_status_window();
		GetNHApp()->windowlist[i].type = type;
		GetNHApp()->windowlist[i].dead = 0;
		break;
	}    
    case NHW_MENU:
	{
		GetNHApp()->windowlist[i].win = NULL; //will create later
		GetNHApp()->windowlist[i].type = type;
		GetNHApp()->windowlist[i].dead = 1;
		break;
	} 
    case NHW_TEXT:
	{
		GetNHApp()->windowlist[i].win = mswin_init_text_window();
		GetNHApp()->windowlist[i].type = type;
		GetNHApp()->windowlist[i].dead = 0;
		break;
	}
	}

	ZeroMemory(&data, sizeof(data) );
	data.wid = i;
	SendMessage( GetNHApp()->hMainWnd, 
		         WM_MSNH_COMMAND, (WPARAM)MSNH_MSG_ADDWND, (LPARAM)&data );
	return i;
}

/* Clear the given window, when asked to. */
void mswin_clear_nhwindow(winid wid)
{
	logDebug("mswin_clear_nhwindow(%d)\n", wid);

    if ((wid >= 0) && 
        (wid < MAXWINDOWS) &&
        (GetNHApp()->windowlist[wid].win != NULL))
    {
         SendMessage( 
			 GetNHApp()->windowlist[wid].win, 
			 WM_MSNH_COMMAND, (WPARAM)MSNH_MSG_CLEAR_WINDOW, (LPARAM)NULL );
    }

#ifdef REINCARNATION
    if( Is_rogue_level(&u.uz) ) 
		mswin_map_mode(mswin_hwnd_from_winid(WIN_MAP), ROGUE_LEVEL_MAP_MODE);
	else 
		mswin_map_mode(mswin_hwnd_from_winid(WIN_MAP), iflags.wc_map_mode);
#endif
}

/* -- Display the window on the screen.  If there is data
                   pending for output in that window, it should be sent.
                   If blocking is TRUE, display_nhwindow() will not
                   return until the data has been displayed on the screen,
                   and acknowledged by the user where appropriate.
                -- All calls are blocking in the tty window-port.
                -- Calling display_nhwindow(WIN_MESSAGE,???) will do a
                   --more--, if necessary, in the tty window-port.
*/
void mswin_display_nhwindow(winid wid, BOOLEAN_P block)
{
	logDebug("mswin_display_nhwindow(%d, %d)\n", wid, block);
	if (GetNHApp()->windowlist[wid].win != NULL)
	{
		if (GetNHApp()->windowlist[wid].type == NHW_MENU) {
			MENU_ITEM_P* p;
			mswin_menu_window_select_menu(GetNHApp()->windowlist[wid].win, PICK_NONE, &p);
		} if (GetNHApp()->windowlist[wid].type == NHW_TEXT) {
			mswin_display_text_window(GetNHApp()->windowlist[wid].win);
		} if (GetNHApp()->windowlist[wid].type == NHW_RIP) {
			mswin_display_RIP_window(GetNHApp()->windowlist[wid].win);
		} else {
			if( !block ) {
				UpdateWindow(GetNHApp()->windowlist[wid].win);
			} else {
				if ( GetNHApp()->windowlist[wid].type == NHW_MAP ) {
					(void) mswin_nhgetch();
				}
			}
		}
		SetFocus(GetNHApp()->hMainWnd);
	}
}


HWND mswin_hwnd_from_winid(winid wid)
{
	if( wid>=0 && wid<MAXWINDOWS) {
		return GetNHApp()->windowlist[wid].win;
	} else {
		return NULL;
	}
}

winid mswin_winid_from_handle(HWND hWnd)
{
	winid i = 0;

	for (i=1; i<MAXWINDOWS; i++)
	  if (GetNHApp()->windowlist[i].win == hWnd)
		  return i;
	return -1;
}

winid mswin_winid_from_type(int type)
{
	winid i = 0;

	for (i=1; i<MAXWINDOWS; i++)
	  if (GetNHApp()->windowlist[i].type == type)
		  return i;
	return -1;
}

void mswin_window_mark_dead(winid wid)
{
	if( wid>=0 && wid<MAXWINDOWS) {
		GetNHApp()->windowlist[wid].win = NULL;
		GetNHApp()->windowlist[wid].dead = 1;
	}
}

/* Destroy will dismiss the window if the window has not 
 * already been dismissed.
*/
void mswin_destroy_nhwindow(winid wid)
{
	logDebug("mswin_destroy_nhwindow(%d)\n", wid);

    if ((GetNHApp()->windowlist[wid].type == NHW_MAP) || 
        (GetNHApp()->windowlist[wid].type == NHW_MESSAGE) || 
        (GetNHApp()->windowlist[wid].type == NHW_STATUS)) {
		/* main windows is going to take care of those */
		return;
    }

	if (GetNHApp()->windowlist[wid].type == NHW_TEXT) {
		/* this type takes care of themself */
		return;
	}

    if (wid != -1) {
		if( !GetNHApp()->windowlist[wid].dead &&
			GetNHApp()->windowlist[wid].win != NULL ) 
			DestroyWindow(GetNHApp()->windowlist[wid].win);
		GetNHApp()->windowlist[wid].win = NULL;
		GetNHApp()->windowlist[wid].type = 0;
		GetNHApp()->windowlist[wid].dead = 0;
	}
}

/* Next output to window will start at (x,y), also moves
 displayable cursor to (x,y).  For backward compatibility,
 1 <= x < cols, 0 <= y < rows, where cols and rows are
 the size of window.
*/
void mswin_curs(winid wid, int x, int y)
{
	logDebug("mswin_curs(%d, %d, %d)\n", wid, x, y);

    if ((wid >= 0) && 
        (wid < MAXWINDOWS) &&
        (GetNHApp()->windowlist[wid].win != NULL))
    {
		 MSNHMsgCursor data;
		 data.x = x;
		 data.y = y;
		 SendMessage( 
			 GetNHApp()->windowlist[wid].win, 
			 WM_MSNH_COMMAND, (WPARAM)MSNH_MSG_CURSOR, (LPARAM)&data );
    }
}

/*
putstr(window, attr, str)
                -- Print str on the window with the given attribute.  Only
                   printable ASCII characters (040-0126) must be supported.
                   Multiple putstr()s are output on separate lines.
Attributes
                   can be one of
                        ATR_NONE (or 0)
                        ATR_ULINE
                        ATR_BOLD
                        ATR_BLINK
                        ATR_INVERSE
                   If a window-port does not support all of these, it may map
                   unsupported attributes to a supported one (e.g. map them
                   all to ATR_INVERSE).  putstr() may compress spaces out of
                   str, break str, or truncate str, if necessary for the
                   display.  Where putstr() breaks a line, it has to clear
                   to end-of-line.
                -- putstr should be implemented such that if two putstr()s
                   are done consecutively the user will see the first and
                   then the second.  In the tty port, pline() achieves this
                   by calling more() or displaying both on the same line.
*/
void mswin_putstr(winid wid, int attr, const char *text)
{
	logDebug("mswin_putstr(%d, %d, %s)\n", wid, attr, text);

	if( (wid >= 0) && 
        (wid < MAXWINDOWS) )
	{
		if( GetNHApp()->windowlist[wid].win==NULL &&
			GetNHApp()->windowlist[wid].type==NHW_MENU ) {
			GetNHApp()->windowlist[wid].win = mswin_init_menu_window(MENU_TYPE_TEXT);
			GetNHApp()->windowlist[wid].dead = 0;
		}

		if (GetNHApp()->windowlist[wid].win != NULL)
		{
			 MSNHMsgPutstr data;
			 data.attr = attr;
			 data.text = text;
			 SendMessage( 
				 GetNHApp()->windowlist[wid].win, 
				 WM_MSNH_COMMAND, (WPARAM)MSNH_MSG_PUTSTR, (LPARAM)&data );
		}
	}
	else
	{
		// build text to display later in message box
		GetNHApp()->saved_text = realloc(GetNHApp()->saved_text, strlen(text) +
			strlen(GetNHApp()->saved_text) + 1);
		strcat(GetNHApp()->saved_text, text);
	}
}

/* Display the file named str.  Complain about missing files
                   iff complain is TRUE.
*/
void mswin_display_file(const char *filename,BOOLEAN_P must_exist)
{
	dlb *f;
	TCHAR wbuf[BUFSZ];

	logDebug("mswin_display_file(%s, %d)\n", filename, must_exist);

	f = dlb_fopen(filename, RDTMODE);
	if (!f) {
		if (must_exist) {
			TCHAR message[90];
			_stprintf(message, TEXT("Warning! Could not find file: %s\n"), NH_A2W(filename, wbuf, sizeof(wbuf)));
			MessageBox(GetNHApp()->hMainWnd, message, TEXT("ERROR"), MB_OK | MB_ICONERROR );
		} 
	} else {
		HWND hwnd;
		char line[LLEN];

		hwnd = mswin_init_text_window();

		while (dlb_fgets(line, LLEN, f)) {
			 MSNHMsgPutstr data;
			 size_t len;

			 len = strlen(line);
			 if( line[len-1]=='\n' ) line[len-1]='\x0';
			 data.attr = 0;
			 data.text = line;
			 SendMessage( hwnd, WM_MSNH_COMMAND, (WPARAM)MSNH_MSG_PUTSTR, (LPARAM)&data );
		}
		(void) dlb_fclose(f);

		mswin_display_text_window(hwnd);
	}
}

/* Start using window as a menu.  You must call start_menu()
   before add_menu().  After calling start_menu() you may not
   putstr() to the window.  Only windows of type NHW_MENU may
   be used for menus.
*/
void mswin_start_menu(winid wid)
{
	logDebug("mswin_start_menu(%d)\n", wid);
	if( (wid >= 0) && 
        (wid < MAXWINDOWS) ) {
		if( GetNHApp()->windowlist[wid].win==NULL &&
			GetNHApp()->windowlist[wid].type==NHW_MENU ) {
			GetNHApp()->windowlist[wid].win = mswin_init_menu_window(MENU_TYPE_MENU);
			GetNHApp()->windowlist[wid].dead = 0;
		}

		if(GetNHApp()->windowlist[wid].win != NULL)	{
			SendMessage( 
				 GetNHApp()->windowlist[wid].win, 
				 WM_MSNH_COMMAND, (WPARAM)MSNH_MSG_STARTMENU, (LPARAM)NULL
			);
		}
	}
}

/*
add_menu(windid window, int glyph, const anything identifier,
                                char accelerator, char groupacc,
                                int attr, char *str, boolean preselected)
                -- Add a text line str to the given menu window.  If identifier
                   is 0, then the line cannot be selected (e.g. a title).
                   Otherwise, identifier is the value returned if the line is
                   selected.  Accelerator is a keyboard key that can be used
                   to select the line.  If the accelerator of a selectable
                   item is 0, the window system is free to select its own
                   accelerator.  It is up to the window-port to make the
                   accelerator visible to the user (e.g. put "a - " in front
                   of str).  The value attr is the same as in putstr().
                   Glyph is an optional glyph to accompany the line.  If
                   window port cannot or does not want to display it, this
                   is OK.  If there is no glyph applicable, then this
                   value will be NO_GLYPH.
                -- All accelerators should be in the range [A-Za-z].
                -- It is expected that callers do not mix accelerator
                   choices.  Either all selectable items have an accelerator
                   or let the window system pick them.  Don't do both.
                -- Groupacc is a group accelerator.  It may be any character
                   outside of the standard accelerator (see above) or a
                   number.  If 0, the item is unaffected by any group
                   accelerator.  If this accelerator conflicts with
                   the menu command (or their user defined alises), it loses.
                   The menu commands and aliases take care not to interfere
                   with the default object class symbols.
                -- If you want this choice to be preselected when the
                   menu is displayed, set preselected to TRUE.
*/
void mswin_add_menu(winid wid, int glyph, const ANY_P * identifier,
		CHAR_P accelerator, CHAR_P group_accel, int attr, 
		const char *str, BOOLEAN_P presel)
{
	logDebug("mswin_add_menu(%d, %d, %p, %c, %c, %d, %s, %d)\n",
		     wid, glyph, identifier, (char)accelerator, (char)group_accel,
			 attr, str, presel);
	if ((wid >= 0) && 
		(wid < MAXWINDOWS) &&
		(GetNHApp()->windowlist[wid].win != NULL))
	{
		MSNHMsgAddMenu data;
		ZeroMemory(&data, sizeof(data));
		data.glyph = glyph;
		data.identifier = identifier;
		data.accelerator = accelerator;
		data.group_accel = group_accel;
		data.attr = attr;
		data.str = str;
		data.presel = presel;

		SendMessage( 
			 GetNHApp()->windowlist[wid].win, 
			 WM_MSNH_COMMAND, (WPARAM)MSNH_MSG_ADDMENU, (LPARAM)&data
		);
	}
}

/*
end_menu(window, prompt)
                -- Stop adding entries to the menu and flushes the window
                   to the screen (brings to front?).  Prompt is a prompt
                   to give the user.  If prompt is NULL, no prompt will
                   be printed.
                ** This probably shouldn't flush the window any more (if
                ** it ever did).  That should be select_menu's job.  -dean
*/
void mswin_end_menu(winid wid, const char *prompt)
{
	logDebug("mswin_end_menu(%d, %s)\n", wid, prompt);
	if ((wid >= 0) && 
		(wid < MAXWINDOWS) &&
		(GetNHApp()->windowlist[wid].win != NULL))
	{
		MSNHMsgEndMenu data;
		ZeroMemory(&data, sizeof(data));
		data.text = prompt;

		SendMessage( 
			 GetNHApp()->windowlist[wid].win, 
			 WM_MSNH_COMMAND, (WPARAM)MSNH_MSG_ENDMENU, (LPARAM)&data
		);
	}
}

/*
int select_menu(windid window, int how, menu_item **selected)
                -- Return the number of items selected; 0 if none were chosen,
                   -1 when explicitly cancelled.  If items were selected, then
                   selected is filled in with an allocated array of menu_item
                   structures, one for each selected line.  The caller must
                   free this array when done with it.  The "count" field
                   of selected is a user supplied count.  If the user did
                   not supply a count, then the count field is filled with
                   -1 (meaning all).  A count of zero is equivalent to not
                   being selected and should not be in the list.  If no items
                   were selected, then selected is NULL'ed out.  How is the
                   mode of the menu.  Three valid values are PICK_NONE,
                   PICK_ONE, and PICK_N, meaning: nothing is selectable,
                   only one thing is selectable, and any number valid items
                   may selected.  If how is PICK_NONE, this function should
                   never return anything but 0 or -1.
                -- You may call select_menu() on a window multiple times --
                   the menu is saved until start_menu() or destroy_nhwindow()
                   is called on the window.
                -- Note that NHW_MENU windows need not have select_menu()
                   called for them. There is no way of knowing whether
                   select_menu() will be called for the window at
                   create_nhwindow() time.
*/
int mswin_select_menu(winid wid, int how, MENU_ITEM_P **selected)
{
	int nReturned = -1;

	logDebug("mswin_select_menu(%d, %d)\n", wid, how);

	if ((wid >= 0) && 
		(wid < MAXWINDOWS) &&
		(GetNHApp()->windowlist[wid].win != NULL))
	{
		nReturned = mswin_menu_window_select_menu(GetNHApp()->windowlist[wid].win, how, selected);
	}
    return nReturned;
}

/*
    -- Indicate to the window port that the inventory has been changed.
    -- Merely calls display_inventory() for window-ports that leave the 
	window up, otherwise empty.
*/
void mswin_update_inventory()
{
	logDebug("mswin_update_inventory()\n");
}

/*
mark_synch()    -- Don't go beyond this point in I/O on any channel until
                   all channels are caught up to here.  Can be an empty call
                   for the moment
*/
void mswin_mark_synch()
{
	logDebug("mswin_mark_synch()\n");
}

/*
wait_synch()    -- Wait until all pending output is complete (*flush*() for
                   streams goes here).
                -- May also deal with exposure events etc. so that the
                   display is OK when return from wait_synch().
*/
void mswin_wait_synch()
{
	logDebug("mswin_wait_synch()\n");
}

/*
cliparound(x, y)-- Make sure that the user is more-or-less centered on the
                   screen if the playing area is larger than the screen.
                -- This function is only defined if CLIPPING is defined.
*/
void mswin_cliparound(int x, int y)
{
	winid wid = WIN_MAP;

	logDebug("mswin_cliparound(%d, %d)\n", x, y);

    if ((wid >= 0) && 
        (wid < MAXWINDOWS) &&
        (GetNHApp()->windowlist[wid].win != NULL))
    {
		 MSNHMsgClipAround data;
		 data.x = x;
		 data.y = y;
         SendMessage( 
			 GetNHApp()->windowlist[wid].win, 
			 WM_MSNH_COMMAND, (WPARAM)MSNH_MSG_CLIPAROUND, (LPARAM)&data );
    }
}

/*
print_glyph(window, x, y, glyph)
                -- Print the glyph at (x,y) on the given window.  Glyphs are
                   integers at the interface, mapped to whatever the window-
                   port wants (symbol, font, color, attributes, ...there's
                   a 1-1 map between glyphs and distinct things on the map).
*/
void mswin_print_glyph(winid wid,XCHAR_P x,XCHAR_P y,int glyph)
{
    logDebug("mswin_print_glyph(%d, %d, %d, %d)\n", wid, x, y, glyph);

    if ((wid >= 0) && 
        (wid < MAXWINDOWS) &&
        (GetNHApp()->windowlist[wid].win != NULL))
    {
		MSNHMsgPrintGlyph data;

		ZeroMemory(&data, sizeof(data) );
		data.x = x;
		data.y = y;
		data.glyph = glyph;
		SendMessage( GetNHApp()->windowlist[wid].win, 
		         WM_MSNH_COMMAND, (WPARAM)MSNH_MSG_PRINT_GLYPH, (LPARAM)&data );
	}
}

/*
raw_print(str)  -- Print directly to a screen, or otherwise guarantee that
                   the user sees str.  raw_print() appends a newline to str.
                   It need not recognize ASCII control characters.  This is
                   used during startup (before windowing system initialization
                   -- maybe this means only error startup messages are raw),
                   for error messages, and maybe other "msg" uses.  E.g.
                   updating status for micros (i.e, "saving").
*/
void mswin_raw_print(const char *str)
{
	TCHAR wbuf[255];
    logDebug("mswin_raw_print(%s)\n", str);
	if( str && *str )
		MessageBox(GetNHApp()->hMainWnd, NH_A2W(str, wbuf, sizeof(wbuf)), TEXT("NetHack"), MB_OK );
}

/*
raw_print_bold(str)
                -- Like raw_print(), but prints in bold/standout (if
possible).
*/
void mswin_raw_print_bold(const char *str)
{
	TCHAR wbuf[255];
    logDebug("mswin_raw_print_bold(%s)\n", str);
	if( str && *str )
		MessageBox(GetNHApp()->hMainWnd, NH_A2W(str, wbuf, sizeof(wbuf)), TEXT("NetHack"), MB_OK );
}

/*
int nhgetch()   -- Returns a single character input from the user.
                -- In the tty window-port, nhgetch() assumes that tgetch()
                   will be the routine the OS provides to read a character.
                   Returned character _must_ be non-zero.
*/
int mswin_nhgetch()
{
	PMSNHEvent event;
    int key = 0;

	logDebug("mswin_nhgetch()\n");
	

	while( (event = mswin_input_pop()) == NULL ||
		   event->type != NHEVENT_CHAR ) 
		mswin_main_loop();

	key = event->kbd.ch;
    return (key);
}

/*
int nh_poskey(int *x, int *y, int *mod)
                -- Returns a single character input from the user or a
                   a positioning event (perhaps from a mouse).  If the
                   return value is non-zero, a character was typed, else,
                   a position in the MAP window is returned in x, y and mod.
                   mod may be one of

                        CLICK_1         -- mouse click type 1 
                        CLICK_2         -- mouse click type 2 

                   The different click types can map to whatever the
                   hardware supports.  If no mouse is supported, this
                   routine always returns a non-zero character.
*/
int mswin_nh_poskey(int *x, int *y, int *mod)
{
	PMSNHEvent event;
  	int key;

	logDebug("mswin_nh_poskey()\n");

	while( (event = mswin_input_pop())==NULL ) mswin_main_loop();

	if( event->type==NHEVENT_MOUSE ) {
		*mod = event->ms.mod;
		*x = event->ms.x;
		*y = event->ms.y;
		key = 0;
	} else {
		key = event->kbd.ch;
	}
	return (key);
}

/*
nhbell()        -- Beep at user.  [This will exist at least until sounds are
                   redone, since sounds aren't attributable to windows anyway.]
*/
void mswin_nhbell()
{
	logDebug("mswin_nhbell()\n");
}

/*
doprev_message()
                -- Display previous messages.  Used by the ^P command.
                -- On the tty-port this scrolls WIN_MESSAGE back one line.
*/
int mswin_doprev_message()
{
    logDebug("mswin_doprev_message()\n");
    return 0;
}

/*
char yn_function(const char *ques, const char *choices, char default)
                -- Print a prompt made up of ques, choices and default.
                   Read a single character response that is contained in
                   choices or default.  If choices is NULL, all possible
                   inputs are accepted and returned.  This overrides
                   everything else.  The choices are expected to be in
                   lower case.  Entering ESC always maps to 'q', or 'n',
                   in that order, if present in choices, otherwise it maps
                   to default.  Entering any other quit character (SPACE,
                   RETURN, NEWLINE) maps to default.
                -- If the choices string contains ESC, then anything after
                   it is an acceptable response, but the ESC and whatever
                   follows is not included in the prompt.
                -- If the choices string contains a '#' then accept a count.
                   Place this value in the global "yn_number" and return '#'.
                -- This uses the top line in the tty window-port, other
                   ports might use a popup.
*/
char mswin_yn_function(const char *question, const char *choices,
		CHAR_P def)
{
    int result=-1;
    char ch;
    char yn_esc_map='\033';
    char message[BUFSZ];

	logDebug("mswin_yn_function(%s, %s, %d)\n", question, choices, def);

    if (WIN_MESSAGE == WIN_ERR && choices == ynchars) {
        char *text = realloc(strdup(GetNHApp()->saved_text), strlen(question)
			+ strlen(GetNHApp()->saved_text) + 1);
        DWORD box_result;
        strcat(text, question);
        box_result = MessageBox(NULL,
             NH_W2A(text, message, sizeof(message)),
             TEXT("NetHack for Windows"),
             MB_YESNOCANCEL |
             ((def == 'y') ? MB_DEFBUTTON1 :
              (def == 'n') ? MB_DEFBUTTON2 : MB_DEFBUTTON3));
        free(text);
		GetNHApp()->saved_text = strdup("");
        return box_result == IDYES ? 'y' : box_result == IDNO ? 'n' : '\033';
    }

    if (choices) {
	char *cb, choicebuf[QBUFSZ];
	Strcpy(choicebuf, choices);
	if ((cb = index(choicebuf, '\033')) != 0) {
	    /* anything beyond <esc> is hidden */
	    *cb = '\0';
	}
	sprintf(message, "%s [%s] ", question, choicebuf);
	if (def) sprintf(eos(message), "(%c) ", def);
	/* escape maps to 'q' or 'n' or default, in that order */
	yn_esc_map = (index(choices, 'q') ? 'q' :
		 (index(choices, 'n') ? 'n' : def));
    } else {
	Strcpy(message, question);
    }

    mswin_putstr(WIN_MESSAGE, ATR_BOLD, message);

    /* Only here if main window is not present */
    while (result<0) {
	ch=mswin_nhgetch();
	if (ch=='\033') {
	    result=yn_esc_map;
	} else if (choices && !index(choices,ch)) {
	    /* FYI: ch==-115 is for KP_ENTER */
	    if (def && (ch==' ' || ch=='\r' || ch=='\n' || ch==-115)) {
		result=def;
	    } else {
		mswin_nhbell();
		/* and try again... */
	    }
	} else {
	    result=ch;
	}
    }
    return result;
}

/*
getlin(const char *ques, char *input)
	    -- Prints ques as a prompt and reads a single line of text,
	       up to a newline.  The string entered is returned without the
	       newline.  ESC is used to cancel, in which case the string
	       "\033\000" is returned.
	    -- getlin() must call flush_screen(1) before doing anything.
	    -- This uses the top line in the tty window-port, other
	       ports might use a popup.
*/
void mswin_getlin(const char *question, char *input)
{
	logDebug("mswin_getlin(%s, %p)\n", question, input);
	if( mswin_getlin_window(question, input, BUFSZ)==IDCANCEL ) {
		strcpy(input, "\033");
	}
}

/*
int get_ext_cmd(void)
	    -- Get an extended command in a window-port specific way.
	       An index into extcmdlist[] is returned on a successful
	       selection, -1 otherwise.
*/
int mswin_get_ext_cmd()
{
	int ret;
	logDebug("mswin_get_ext_cmd()\n");

	if(mswin_ext_cmd_window (&ret) == IDCANCEL)
		return -1;
	else 
		return ret;
}


/*
number_pad(state)
	    -- Initialize the number pad to the given state.
*/
void mswin_number_pad(int state)
{
    /* Do Nothing */
	logDebug("mswin_number_pad(%d)\n", state);
}

/*
delay_output()  -- Causes a visible delay of 50ms in the output.
	       Conceptually, this is similar to wait_synch() followed
	       by a nap(50ms), but allows asynchronous operation.
*/
void mswin_delay_output()
{
	logDebug("mswin_delay_output()\n");
	Sleep(50);
}

void mswin_change_color() 
{ 
	logDebug("mswin_change_color()\n"); 
}

char *mswin_get_color_string() 
{ 
	logDebug("mswin_get_color_string()\n");
	return( "" ); 
}

/*
start_screen()  -- Only used on Unix tty ports, but must be declared for
	       completeness.  Sets up the tty to work in full-screen
	       graphics mode.  Look at win/tty/termcap.c for an
	       example.  If your window-port does not need this function
	       just declare an empty function.
*/
void mswin_start_screen()
{
    /* Do Nothing */
	logDebug("mswin_start_screen()\n");
}

/*
end_screen()    -- Only used on Unix tty ports, but must be declared for
	       completeness.  The complement of start_screen().
*/
void mswin_end_screen()
{
    /* Do Nothing */
	logDebug("mswin_end_screen()\n");
}

/*
outrip(winid, int)
	    -- The tombstone code.  If you want the traditional code use
	       genl_outrip for the value and check the #if in rip.c.
*/
void mswin_outrip(winid wid, int how)
{
   	logDebug("mswin_outrip(%d)\n", wid, how);
    if ((wid >= 0) && (wid < MAXWINDOWS) ) {
		DestroyWindow(GetNHApp()->windowlist[wid].win);
		GetNHApp()->windowlist[wid].win = mswin_init_RIP_window();
		GetNHApp()->windowlist[wid].type = NHW_RIP;
		GetNHApp()->windowlist[wid].dead = 0;
	}

	genl_outrip(wid, how);
}


void mswin_main_loop()
{
	MSG msg;

	while( !mswin_have_input() &&
		   GetMessage(&msg, NULL, 0, 0)!=0 ) {
		if (!TranslateAccelerator(msg.hwnd, GetNHApp()->hAccelTable, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	} 
}

/* clean up and quit */
void bail(const char *mesg)
{
    clearlocks();
    mswin_exit_nhwindows(mesg);
    terminate(EXIT_SUCCESS);
    /*NOTREACHED*/
}

#ifdef _DEBUG
#include <stdarg.h>

void
logDebug(const char *fmt, ...)
{
  FILE *dfp = fopen("nhtrace.log", "a");

  if (dfp) {
     va_list args;

     va_start(args, fmt);
     vfprintf(dfp, fmt, args);
     va_end(args);
     fclose(dfp);
  }
}

#endif
