/*	SCCS Id: @(#)nhlan.c	3.4	1999/11/21	*/
/* Copyright (c) Michael Allison, 1997                  */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Currently shared by the following ports:
 *	WIN32
 *
 * The code in here is used to take advantage of added features
 * that might be available in a Local Area Network environment.
 *
 * 	Network Username of player
 *	Mail
 * Futures
 *	Shared bones
 *		To implement this: code, data files, and configuration
 *		files need to be separated from writeable files such
 *		as level files, bones files, and save files.
 *
 */

#include "hack.h"
#include <ctype.h>

#ifdef LAN_FEATURES

#ifdef LAN_MAIL
/* Port specific code needs to implement these routines for LAN_MAIL */
extern char *FDECL(get_username, (int *));
extern boolean NDECL(mail_check);
extern boolean FDECL(mail_fetch, (struct lan_mail_struct *)); 
extern void FDECL(mail_init, (char *));
extern void NDECL(mail_finish);

struct lan_mail_struct mailmessage;
#endif /* LAN_MAIL */


void init_lan_features()
{
	lan_username();
#ifdef LAN_MAIL
	lan_mail_init();
#endif
#ifdef LAN_SHARED_BONES
#endif
}
/*
 * The get_lan_username() call is a required call, since some of
 * the other LAN features depend on a unique username being available.
 *
 */
char lusername[MAX_LAN_USERNAME];
int lusername_size = MAX_LAN_USERNAME;

char *lan_username()
{
	char *lu;
	lu = get_username(&lusername_size);
	if (lu) {
	 Strcpy(lusername, lu);
	 return lusername;
	} else return (char *)0;
}

# ifdef LAN_MAIL
#if 0
static void
mail_by_pline(msg)
struct lan_mail_struct *msg;
{
	long	size;

	for (size = 0; size < qt_msg->size; size += (long)strlen(in_line)) {
	    (void) dlb_fgets(in_line, 80, msg_file);
	    convert_line();
	    pline(out_line);
	}

}
#endif /* 0 */

static void
mail_by_window(msg)
struct lan_mail_struct *msg;
{
	char buf[BUFSZ];
	winid datawin = create_nhwindow(NHW_TEXT);
	char *get, *put;
	int ccount = 0;
	
	get = msg->body;
	put = buf;
	while (*get) {
	     if (ccount > 79) {
	     	*put = '\0';
	     	putstr(datawin, 0, buf);
	     	put = buf;
		ccount = 0;
	     }
	     if (*get == '\r') {
		get++;
	     } else if (*get == '\n') { 
	     	*put = '\0';
	     	putstr(datawin, 0, buf);
	     	put = buf;
	     	get++;
		ccount = 0;
	     } else if (!isprint(*get)) {
		get++;
	     } else {
	 	*put++ = *get++;
		ccount++;
	     }
	}
	*put = '\0';
	putstr(datawin, 0, buf);
	putstr(datawin, 0, "");	       
	display_nhwindow(datawin, TRUE);
	destroy_nhwindow(datawin);
}

/* this returns TRUE if there is mail ready to be read */
boolean lan_mail_check()
{
	if (flags.biff) {
		if (mail_check()) return TRUE;
	}
	return FALSE;
}

void lan_mail_read(otmp)
struct obj *otmp;
{
	if (flags.biff) {
		(void) mail_fetch(&mailmessage);
		/* after a successful fetch iflags.lan_mail_fetched
		 * should be TRUE.  If it isn't then we don't
		 * trust the contents of mailmessage.  This
		 * ensures that things work correctly across
		 * save/restores where mailmessage isn't
		 * saved (nor should it be since it may be
		 * way out of context by then).
		 */
		 if (iflags.lan_mail_fetched) {
		    if (mailmessage.body_in_ram) {
		    	mail_by_window(&mailmessage);
		 	return;
		    }
		 }
	}
	pline_The("text has faded and is no longer readable.");
}

void lan_mail_init()
{
	if (!flags.biff) return;
	(void) mail_init(lusername);
}

void lan_mail_finish()
{
	if (iflags.lan_mail)
		(void) mail_finish();
}

/* If ever called, the underlying mail system ran into trouble
 * and wants us to cease bothering it immediately.
 * Don't call mail_finish() because the underlying mail system
 * may already be unavailable. Just clean up the NetHack side
 * of things to prevent a crash.
 */
void lan_mail_terminate()
{
	/* Step 1. Clear iflags.lan_mail to indicate "not inited" */
	iflags.lan_mail = FALSE;

	/* Step 2. Clear iflags.lan_mail_fetched */
	iflags.lan_mail_fetched = FALSE;

	/* Once having gotten to this point, the only
	   way to resume NetHack mail features again is
	   to Save/Quit game, or for the user to clear
	   iflags.biff and then set it once again,
	   which triggers mail initialization */
}

# endif /*LAN_MAIL*/

#endif /*LAN_FEATURES*/
/*nhlan.c*/
