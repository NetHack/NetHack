/*	SCCS Id: @(#)mmodal.c	3.1	93/01/24		  */
/* Copyright (c) Jon W{tte, Hao-Yang Wang, Jonathan Handler 1992. */
/* NetHack may be freely redistributed.  See license for details. */

#include <Dialogs.h>
#if ENABLE_MAC_POPUP
#include "hack.h"
#include "mactty.h"
#endif
#include "macpopup.h"

/* Flash a dialog button when its accelerator key is pressed */
void
FlashButton (WindowPtr wind, short item) {
	short type;
	Handle handle;
	Rect rect;
	unsigned long ticks;

	/* Apple recommends 8 ticks */
	GetDItem(wind, item, &type, &handle, &rect);
	HiliteControl((ControlHandle)handle, kControlButtonPart);
	Delay(8, &ticks);
	HiliteControl((ControlHandle)handle, 0);
	return;
}


#if ENABLE_MAC_POPUP
static void mv_handle_click (EventRecord *theEvent);

#define MAX_MV_DIALOGS 20
static int old_dialog_count = 0;
static struct {
	short	  id;
	Boolean   init_visible;
	DialogPtr dialog;
} old_dialog[MAX_MV_DIALOGS];


static short frame_corner;
static pascal void FrameItem(DialogPtr dlog, short item);
static UserItemUPP FrameItemUPP = NULL;

static pascal void
FrameItem (DialogPtr dlog, short item) {
	short k;
	Handle h;
	Rect r;

	GetDItem (dlog, item, &k, &h, &r);
	PenSize (3, 3);
	FrameRoundRect (&r, frame_corner, frame_corner);
	PenNormal ();
}


static void
SetFrameItem (DialogPtr dlog, short frame, short item) {
	Rect r, r2;
	short kind;
	Handle h;

	if (!FrameItemUPP)	/* initialize handler routine */
		FrameItemUPP = NewUserItemProc(FrameItem);
		
	GetDItem (dlog, item, &kind, &h, &r);
	InsetRect (&r, -4, -4);
	r2 = r;
	GetDItem (dlog, frame, &kind, &h, &r);
	SetDItem (dlog, frame, kind, (Handle) FrameItemUPP, &r2);
	frame_corner = 16;
}


/* Instead of calling GetNewDialog everytime, just call
   SelectWindow/ShowWindow for the old dialog to remember its location.
*/
/*
 *	Unfortunately, this does not work, as it doesn't handle old text
 *	in edit text boxes, and not ParamText parameters either.
 *
 */
static DialogPtr
mv_get_new_dialog(short dialogID) {
	DialogPtr dialog;
	int d_idx = old_dialog_count;
	Rect oldRect;
	Boolean hadOld = 0;

	old_dialog[0].id = dialogID;
	while (old_dialog[d_idx].id != dialogID)
		--d_idx;

/*
 *	This routine modified so that the old dialog is
 *	disposed, and the new one read in after we remember
 *	the old dialog's position.
 *
 *	This takes care of strange default strings and ParamTexts
 *
 */

	if (d_idx) {
		dialog = old_dialog [d_idx] . dialog;
		oldRect = dialog->portBits . bounds;
		DisposeDialog (dialog);
		old_dialog [d_idx] . dialog = (DialogPtr) 0;
		hadOld = 1;

	} else {
		d_idx = ++ old_dialog_count;
	}

	dialog = GetNewDialog(dialogID, nil, (WindowPtr)-1);
	if (dialog) {
		if (hadOld) {
			MoveWindow (dialog, - oldRect . left, - oldRect . top, FALSE);
		}
		old_dialog[d_idx].id = dialogID;
		old_dialog[d_idx].init_visible
			= ((WindowPeek)dialog)->visible;
		old_dialog[d_idx].dialog = dialog;
	}
	return dialog;
}

/* Instead of actually closing the dialog, just hide it so its location
   is remembered. */
static void mv_close_dialog(DialogPtr dialog) {
	HideWindow(dialog);
}

/* This routine is stolen/borrowed from HandleClick (macwin.c).  See the
   comments in mv_modal_dialog for more information. */
static void
mv_handle_click (EventRecord *theEvent) {
	int code;
	WindowPtr theWindow;
	Rect r = (*GetGrayRgn ())->rgnBBox;

	InsetRect (&r, 4, 4);
	InitCursor ();

	code = FindWindow (theEvent->where, &theWindow);

	switch (code) {
	case inContent :
		if (theWindow != FrontWindow ()) {
			nhbell ();
		}
		break;
	case inDrag :
		SetCursor(&qd.arrow);
		DragWindow (theWindow, theEvent->where, &r);
		SaveWindowPos (theWindow);
		break;
	default :
		HandleEvent (theEvent);
	}
}

static void
mv_modal_dialog(ModalFilterProcPtr filterProc, short *itemHit) {
	GrafPtr org_port;
	GetPort(&org_port);

	for (;;) {
		DialogPtr dialog = FrontWindow();
		EventRecord evt;

		WaitNextEvent(everyEvent, &evt, GetCaretTime(), (RgnHandle) nil);

		if (evt.what == keyDown)
			if (evt.modifiers &cmdKey) {
				if ((evt.message & charCodeMask) == '.') {
					/* 0x351b is the key code and character code of the esc key. */
					evt.message = 0x351b;
					evt.modifiers &= ~cmdKey;
				}
			} else
				trans_num_keys(&evt);

		if (filterProc) {
			if ((*filterProc)(dialog, &evt, itemHit))
				break;
		} else if (evt.what == keyDown) {
			char ch = evt.message & charCodeMask;
			if (ch == CHAR_CR || ch == CHAR_ENTER) {
				*itemHit = ok;
				FlashButton(dialog, ok);
				break;
			}
		}

		if (IsDialogEvent(&evt)) {
			DialogPtr dont_care;
			if (DialogSelect(&evt, &dont_care, itemHit))
				break;

		/* The following part is problemmatic: (1) Calling HandleEvent
		   here may cause some re-entrance problem (seems ok, but I am
		   not sure). (2) It is ugly to treat mouseDown events as a
		   special case.  If we can just say "else HandleEvent(&evt);"
		   here it will be better. */
		} else if (evt.what == mouseDown)
				mv_handle_click(&evt);
			else
				HandleEvent(&evt);

		SetPort(org_port);
	}
}

/*********************************************************************************
 * mactopl routines using dialogs
 *********************************************************************************/
 
#define YN_DLOG 133
#define YNQ_DLOG 134
#define YNAQ_DLOG 135
#define YNNAQ_DLOG 136

static int yn_user_item [] = {5, 6, 7, 8};
static short gEnterItem, gEscItem;
static const char *gRespStr = (const char *)0;
static char gDef = 0;
static short dlogID;


static void
SetEnterItem (DialogPtr dp, const short newEnterItem) {
	short kind;
	Handle item;
	Rect r, r2;

	if (gEnterItem != newEnterItem) {
		GetDItem (dp, gEnterItem, &kind, &item, &r2);
		InsetRect (&r2, - 4, - 4);
		EraseRect (&r2);
		InvalRect (&r2);

		gEnterItem = newEnterItem;

		GetDItem (dp, newEnterItem, &kind, &item, &r2);
		frame_corner = kind == ctrlItem + btnCtrl ? 16 : 0;
		InsetRect (&r2, - 4, - 4);
		InvalRect (&r2);
		r = r2;
		GetDItem (dp, yn_user_item [dlogID - YN_DLOG], &kind, &item, &r2);
		SetDItem (dp, yn_user_item [dlogID - YN_DLOG], kind, item, &r);
	}
}


static void
do_tabbing (DialogPtr dp) {
	SetEnterItem(dp, gEnterItem == 1 ? strlen(gRespStr) : gEnterItem - 1);
}


static void
set_yn_number(DialogPtr dp) {
	if (gRespStr && gRespStr[gEnterItem-1] == '#') {
		short k;
		Handle h;
		Rect r;
		Str255 s;
		GetDItem(dp, gEnterItem, &k, &h, &r);
		GetIText(h, s);
		if (s[0])
			StringToNum(s, &yn_number);
	}
}


static pascal Boolean
YNAQFilter (DialogPtr dp, EventRecord *ev, short *itemHit) {
	unsigned char code;
	char ch;
	char *re = (char *) gRespStr;

	if (ev->what != keyDown) {

		return 0;
	}
	code = (ev->message & 0xff00) >> 8;
	ch = ev->message & 0xff;

	switch (code) {
	case 0x24 :
	case 0x4c :
		set_yn_number (dp);
		*itemHit = gEnterItem;
		FlashButton (dp, *itemHit);
		return 1;
	case 0x35 :
	case 0x47 :
		*itemHit = gEscItem;
		FlashButton (dp, *itemHit);
		return 1;
	case 0x30 :
		do_tabbing (dp);
		return 0;
	}
	switch (ch) {
	case '\r' :
	case '\n' :
	case ' ' :
	case 3 :
		set_yn_number (dp);
		*itemHit = gEnterItem;
		FlashButton (dp, *itemHit);
		return 1;

	case 9 :
		do_tabbing (dp);
		return 0;

	case 27 :
		*itemHit = gEscItem;
		FlashButton (dp, *itemHit);
		return 1;

	case CHAR_BS :
	case 28 : case 29 : case 30 : case 31 : /* the four arrow keys */
	case '0' : case '1' : case '2' : case '3' : case '4' :
	case '5' : case '6' : case '7' : case '8' : case '9' : {
		char *loc = strchr (gRespStr, '#');
		if (loc) {
			SetEnterItem(dp, loc - gRespStr + 1);
			return 0; /* Dialog Manager will then put this key into the text field. */
		}
	}
	}

	while (*re) {
		if (*re == ch) {
			*itemHit = (re - gRespStr) + 1;
			FlashButton (dp, *itemHit);
			return 1;
		}
		re ++;
	}

	nhbell ();
	ev->what = nullEvent;
	return 0;
}


static char
do_question_dialog (char *query, int dlog, int defbut, char *resp) {
	Str255 p;
	DialogPtr dp;
	short item;

	char c = queued_resp ((char *) resp);
	if (c)
		return c;

	dlogID = dlog;
	C2P (query, p);
	ParamText ((char *)p, (uchar *) 0, (uchar *) 0, (uchar *) 0);
	dp = mv_get_new_dialog (dlog);
	if (! dp) {
		return 0;
	}
	SetPort (dp);
	ShowWindow (dp);

	gEscItem = strlen (resp);
	gEnterItem = defbut;
	gRespStr = resp;

	SetFrameItem (dp, yn_user_item [dlogID - YN_DLOG], gEnterItem);

	InitCursor ();
	mv_modal_dialog (YNAQFilter, &item);
	mv_close_dialog (dp);
	return resp [item - 1];
}


static pascal Boolean
OneCharDLOGFilter (DialogPtr dp, EventRecord *ev, short *item) {
	char ch;
	short k;
	Handle h;
	Rect r;
	unsigned char com [2];

	if (ev->what != keyDown) {
		return 0;
	}
	ch = ev->message & 0xff;

	com [0] = 1;
	com [1] = ch;

	if (ch == 27) {
		GetDItem (dp, 4, &k, &h, &r);
		SetIText (h, com);
		*item = 2;
		FlashButton (dp, 2);
		return 1;
	}
	if (! gRespStr || strchr (gRespStr, ch)) {
		GetDItem (dp, 4, &k, &h, &r);
		SetIText (h, com);
		*item = 1;
		FlashButton (dp, 1);
		return 1;
	}
	if (ch == 10 || ch == 13 || ch == 3 || ch == 32) {
		com [1] = gDef;
		GetDItem (dp, 4, &k, &h, &r);
		SetIText (h, com);
		*item = 1;
		FlashButton (dp, 1);
		return 1;
	}
	if (ch > 32 && ch < 127) {
		GetDItem (dp, 4, &k, &h, &r);
		SetIText (h, com);
		*item = 1;
		FlashButton (dp, 1);
		return 1;
	}
	nhbell ();
	ev->what = nullEvent;
	return 1;
}


static char
generic_yn_function (query, resp, def)
const char *query, *resp;
char def;
{
	DialogPtr dp;
	short k, item;
	Handle h;
	Rect r;
	unsigned char com [32] = {1, 27}; // margin for getitext
	Str255 pQuery;

	char c = queued_resp ((char *) resp);
	if (c)
		return c;

	dp = mv_get_new_dialog (137);
	if (! dp) {
		return 0;
	}
	SetPort (dp);
	ShowWindow (dp);
	InitCursor ();
	SetFrameItem (dp, 6, 1);
	if (def) {
		com [1] = def;
	}
	strcpy ((char *) &pQuery[1], query);
	if (resp && *resp) {
		strcat ((char *) &pQuery[1], " (");
		strcat ((char *) &pQuery[1], resp);
		strcat ((char *) &pQuery[1], ")");
	}
	pQuery[0] = strlen (&pQuery[1]);
	ParamText ((char *) pQuery, (uchar *) 0, (uchar *) 0, (uchar *) 0);
	GetDItem (dp, 4, &k, &h, &r);
	SetIText (h, com);
	SelIText (dp, 4, 0, 0x7fff);
	InitCursor ();
	SetFrameItem (dp, 6, 1);
	gRespStr = resp;
	gDef = def;
	do {
		mv_modal_dialog (OneCharDLOGFilter, &item);

	} while (item != 1 && item != 2);
	GetIText (h, com);

	mv_close_dialog (dp);
	if (item == 2 || ! com [0]) {
		return 27; // escape
	}
	return com [1];
}


static char
ynaq_dialog (query, resp, def)
const char *query, *resp;
char def;
{
	int dia = 0;

	if (resp) {
		if (! strcmp (resp, ynchars)) {
			dia = YN_DLOG;
		}
		if (! strcmp (resp, ynqchars)) {
			dia = YNQ_DLOG;
		}
		if (! strcmp (resp, ynaqchars)) {
			dia = YNAQ_DLOG;
		}
		if (! strcmp (resp, ynNaqchars)) {
			dia = YNNAQ_DLOG;
		}
	}
	if (! dia) {
		return generic_yn_function (query, resp, def);
	}

	return do_question_dialog ((char *) query, dia ,
		(strchr (resp, def) - resp) + 1, (char *) resp);
}


char
popup_yn_function(const char *query, const char *resp, char def) {
	char ch;

	if (ch = ynaq_dialog (query, resp, def))
		return ch;

	return topl_yn_function(query, resp, def);
}


/*********************************************************************************
 * mgetline routines using dialogs
 *********************************************************************************/

static pascal Boolean
getlinFilter (DialogPtr dp, EventRecord *ev, short *itemHit) {
	if (ev->what == keyDown) {
		int key = ev->message & keyCodeMask,
			ch	= ev->message & charCodeMask;

		if (ch == 0x1b || key == 0x3500 || key == 0x4700) {
			*itemHit = 2;
			FlashButton(dp, 2);
			return true;
		} else if (ch == CHAR_CR || ch == CHAR_ENTER) {
			*itemHit = 1;
			FlashButton(dp, 1);
			return true;
		}
	}
	return false;
}



static Boolean
ExtendedCommandDialogFilter (DialogPtr dp, EventRecord *ev, short *item) {
	int ix;
	Handle h;
	Rect r;
	short k;
	Str255 s;
	unsigned char com [2];

	if (ev->what != keyDown) {
		return 0;
	}
	com [0] = 1;
	com [1] = ev->message & 0xff;

	if (com [1] == 10 || com [1] == 13 || com [1] == 32 ||
		com [1] == 3) { // various "OK"
		*item = 1;
		FlashButton (dp, 1);
		return 1;
	}
	if (com [1] == 27 || (ev->message & 0xff00 == 0x3500)) { // escape
		*item = 2;
		FlashButton (dp, 2);
		return 1;
	}
	for (ix = 3; ix; ix ++) {
		h = (Handle) 0;
		k = 0;
		GetDItem (dp, ix, &k, &h, &r);
		if (! k || ! h) {
			return 0;
		}
		if (k == 6) {	//	Radio Button Item
			GetCTitle ((ControlHandle) h, s);
			s [0] = 1;
			if (! IUEqualString (com, s)) {
				*item = ix;
				return 1;
			}
		}
	}
/*NOTREACHED*/
	return 0;
}


void
popup_getlin (const char *query, char *bufp) {
	ControlHandle	ctrl;
	DialogPtr		promptDialog;
	short			itemHit, type;
	Rect			box;
	Str255			pasStr;

	if (get_line_from_key_queue (bufp))
		return;

	/*
	** Make a copy of the prompt string and convert the copy to a Pascal string.
	*/
	
	C2P(query, pasStr);
	
	/*
	** Set the query line as parameter text.
	*/
	
	ParamText(pasStr, "\p", "\p", "\p");
	
	promptDialog = mv_get_new_dialog(130);
	ShowWindow(promptDialog);

	InitCursor ();
	SetFrameItem (promptDialog, 6, 1);
	do {
		mv_modal_dialog(&getlinFilter, &itemHit);
	} while ((itemHit != 1) && (itemHit != 2));
	
	if (itemHit != 2) {
		/*
		** Get the text from the text edit item.
		*/
		
		GetDItem(promptDialog, 4, &type, (Handle *) &ctrl, &box);
		GetIText((Handle) ctrl, pasStr);
		
		/*
		** Convert it to a 'C' string and copy it into the return value.
		*/
		
		P2C (pasStr, bufp);
	} else {
		/*
		** Return a null-terminated string consisting of a single <ESC>.
		*/
		
		bufp[0] = '\033';
		bufp[1] = '\0';
	}
	
	mv_close_dialog(promptDialog);
}

#endif /* ENABLE_MAC_POPUP */
