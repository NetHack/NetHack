/*      SCCS Id: @(#)mapimail.c 3.3     2000/04/25        */
/* Copyright (c) Michael Allison, 1997                  */
/* NetHack may be freely redistributed.  See license for details. */

#ifdef MAX_BODY_SIZE
#undef MAX_BODY_SIZE
#define MAX_BODY_SIZE           2048    /* largest body held in ram in bytes */
#endif

#include "hack.h"
# ifdef LAN_MAIL
#include "win32api.h"
#include <mapi.h>

#define MAPI_MSGID_SIZE 512     /* as recommended */
#define MAPI_LIB_FAIL           1
#define MAPI_FUNC_FAIL          2

HANDLE hLibrary;                        /* Handle for MAPI32.DLL     */
LHANDLE MAPISession;                    /* Handle for MAPI session   */
char MAPIMessageID[MAPI_MSGID_SIZE];    /* Msg ID from provider      */
lpMapiMessage MAPIMessage;              /* Ptr to message from prov  */
struct lan_mail_struct received_msg;    /* store received msg here   */
#ifdef MAPI_VERBOSE
FILE *dbgfile;
#define MAPIDEBUGFILENAME	"mapidebug.log"
#endif

/*
 * Prototypes for functions in this file
 * (Not in normal NetHack style, but its doubtful that any
 *  of the code in here is at all portable between platforms)
 */
static long __stdcall start_mailthread(LPVOID ThreadParam);
static boolean MAPI_mail_check(char *);
static boolean MAPI_mail_fetch(char *);
static void MAPI_mail_finish(void);
static int MAPI_mail_context(int *);
static boolean MAPI_mail_init(char *);
static int InitMAPI(void);
static int DeInitMAPI(void);
static void MAPI_mail_abort(unsigned long);

/*
 * Declare the function pointers used to access MAPI routines
 * from the MAPI32.DLL
 */
LPMAPILOGON     fpMAPILogon;
LPMAPILOGOFF    fpMAPILogoff;
LPMAPIFINDNEXT  fpMAPIFindNext;
LPMAPIREADMAIL  fpMAPIReadMail;
LPMAPIFREEBUFFER fpMAPIFreeBuffer;


/*
 * Data requiring synchronized access between the
 * two thread contexts contained in this file
 * (nethack thread and the MAPI-mail thread)
 */
HANDLE mailthread = 0;          /* handle for the mail-thread       */
unsigned nhmailthread_ID;       /* thread ID for mail-thread        */
unsigned long nhmail_param;     /* value passed to mail-thread      */
long mailthread_continue = 0;   /* nh -> mapi thread: shut down!    */
long mailthread_stopping = 0;   /* mapi -> nh thread: MAPI is ill!  */

/*
 * Data updated by the NetHack thread only
 * but read by the MAPI-mail thread.
 *
 */
char MAPI_username[120];
boolean debugmapi;

/*
 * Data updated by the MAPI-mail thread only
 * but read by the NetHack thread.
 */
long mail_fetched = 0;

/*
 * Data used only by the MAPI-mail thread.
 */
long mailpasses;        /* counts the FindNext calls issued to MAPI */
char msgID[80];         /* message ID of msg under manipulation     */

/*===============================================================
 * NetHack thread routines
 *
 * These routines run in the context of the main NetHack thread.
 * They are used to provide an interface between the NetHack
 * LAN_MAIL code and the MAPI-thread code.
 *
 * These routines are referenced by shared code in
 * sys/share/nhlan.c and they are prototyped in include/extern.h
 *===============================================================
 */
boolean mail_check()
{
	if (!mailthread_stopping) {
		if (mail_fetched > 0)
			return TRUE;
	} else lan_mail_terminate();
	return FALSE;
}


boolean mail_fetch(msg)
struct lan_mail_struct *msg;
{
	/* shouldn't need to check mailthread_stopping here
	   as the data should be valid anyway */
	*msg = received_msg;
	iflags.lan_mail_fetched = TRUE;
	/* clear the marker now so new messages can arrive */
	InterlockedDecrement(&mail_fetched);
	/* check to see if the MAPI-mail thread is saying "stop" */
	if (mailthread_stopping) lan_mail_terminate();
	return TRUE;
}

void mail_finish()
{
	InterlockedDecrement(&mailthread_continue);
}

void mail_init(uname)
char *uname;
{
  /* This routine invokes the _beginthreadex()
   * run-time library call to start the execution
   * of the MAPI-mail thread.  It also performs
   * few other preparatory tasks.
   */
	if (mailthread_continue) {
		/* Impossible - something is really messed up */
		/* We better do some sanity checking */
		/* Is there a valid thread handle and ID? */
		if (mailthread && nhmailthread_ID) {
			/* TODO: check 'em via WIN32 call */
			/* if valid, do something about them */
			/* if not, clear them */
		}
	}
	mailthread = 0;
	nhmailthread_ID = 0;
	mailthread_continue = 0;  /* no interlock needed yet */
	mailthread_stopping = 0;  /* no interlock needed yet */
#ifdef MAPI_VERBOSE
	if (getenv("DEBUG_MAPI")) debugmapi = TRUE; 
#endif
	if (uname)
		strcpy(MAPI_username, uname);
	else {
#ifdef MAPI_VERBOSE
		if (debugmapi) dbgfile = fopen(MAPIDEBUGFILENAME,"w");
		if (dbgfile) {
		       fprintf(dbgfile,
			      "mapi initialization bypassed because uname not available\n");
		       fclose(dbgfile);
		}
#endif
		return;
	}
	mailthread = (HANDLE)_beginthreadex(
			(void *)0,
			(unsigned)0,
			start_mailthread,
			(void *)&nhmail_param,
			(unsigned)0, (unsigned *)&nhmailthread_ID);
#if 0
	/* TODO: check for failure of the thread. For now nethack
	 *       doesn't care
	 */
	if (!mailthread) {

	}
#endif
}

/*===============================================================
 * MAPI-mail thread routines
 *
 * These routines run in the context of their own
 * MAPI-mail thread.  They were placed into their own
 * thread, because MAPI calls can sometimes (often?) take
 * a horribly long time to return (minutes even).
 *
 * Each of the routines below are referenced only by other
 * routines below, with the exception of start_mailthread(),
 * of course, which is started by a run-time library call
 * issued from the main NetHack thread.
 *===============================================================
 */

/*
 * start_mailthread() is the entry point of the MAPI-mail thread.
 *
 */

static long __stdcall start_mailthread(LPVOID ThreadParam)
{
    char *lu = MAPI_username;
    if(MAPI_mail_init(lu)) {
	InterlockedIncrement(&mailthread_continue);
	while (mailthread_continue) {
		mailpasses++;
		if (MAPI_mail_check(msgID)) {
			if (MAPI_mail_fetch(msgID)) {
				/* getting here means success */
			}
		}
		Sleep(MAILTHREADFREQ);
	}
#ifdef MAPI_VERBOSE
	if (debugmapi && !mailthread_continue) {
	  fprintf(dbgfile,
		"MAPI-thread detected mailthread_continue change.\n");
	  fprintf(dbgfile,
		"NetHack has requested that the MAPI-thread cease.\n");
	}
#endif
    }
    return 0;
}

static int
MAPI_mail_context(mcount)
int *mcount;
{
	unsigned long status;
	char tmpID[80];
	int count = 0;

	tmpID[0] = '\0';
	MAPIMessageID[0] = '\0';

	/* Get the ID of the first unread message */
	status = fpMAPIFindNext(MAPISession, 0, 0, 0,
		MAPI_UNREAD_ONLY | MAPI_GUARANTEE_FIFO,
		0, tmpID);
	/* Now loop through them all until we have no more */
	while (status == SUCCESS_SUCCESS) {
		strcpy(MAPIMessageID, tmpID);
		count++;
		status = fpMAPIFindNext(MAPISession, 0,
			0, MAPIMessageID,
			MAPI_UNREAD_ONLY | MAPI_GUARANTEE_FIFO,
			0, tmpID);
	}
	if (status == MAPI_E_NO_MESSAGES) {
		/* context is now at last message */
		if (mcount) *mcount = count;
		return SUCCESS_SUCCESS;
	}
	return status;
}

static boolean
MAPI_mail_check(mID)
char *mID;
{
	unsigned long status;
	char tmpID[80];

	tmpID[0] = '\0';

#ifdef MAPI_VERBOSE
	if (debugmapi) fprintf(dbgfile, "MAPI_mail_check() ");
#endif
	if (mail_fetched) {
#ifdef MAPI_VERBOSE
	if (debugmapi) fprintf(dbgfile, "returning FALSE (buffer occupied)\n");
#endif
		return FALSE; /* buffer occupied, don't bother */
	}
	/* Get the ID of the next unread message if there is one */
	status = fpMAPIFindNext(MAPISession, 0, 0, MAPIMessageID,
		MAPI_UNREAD_ONLY | MAPI_GUARANTEE_FIFO,
		0, tmpID);
	if (status == SUCCESS_SUCCESS) {
		strcpy(mID, tmpID);
#ifdef MAPI_VERBOSE
	if (debugmapi) fprintf(dbgfile, "returning TRUE\n");
#endif
		return TRUE;
	}
	if (status == MAPI_E_NO_MESSAGES) {
#ifdef MAPI_VERBOSE
		if (debugmapi) fprintf(dbgfile, "returning FALSE\n");
#endif
		return FALSE;
	}
#ifdef MAPI_VERBOSE
	if (debugmapi) fprintf(dbgfile,"Error, check_newmail() status: %d\n", status);
	MAPI_mail_abort(status);
#endif
	return FALSE;
}

static boolean
MAPI_mail_fetch(mID)
char *mID;
{
	unsigned long status;

#ifdef MAPI_VERBOSE
	if (debugmapi) fprintf(dbgfile, "MAPI_mail_fetch() ");
#endif
	/*
	 *  Update context right away so we don't loop if there
	 *  was a problem getting the message
	 */
	strcpy(MAPIMessageID, mID);

	if (mail_fetched) {
#ifdef MAPI_VERBOSE
	if (debugmapi) fprintf(dbgfile, "returning FALSE (buffer occupied)\n");
#endif
		 return FALSE;  /* buffer occupied */
	}

	status = fpMAPIReadMail(MAPISession, 0, mID,
		 MAPI_SUPPRESS_ATTACH | MAPI_PEEK,
		 0, &MAPIMessage);
	if (status == SUCCESS_SUCCESS) {
		strncpy(received_msg.subject,
			MAPIMessage->lpszSubject,
			sizeof(received_msg.subject) - 1);
		if((MAPIMessage->lpOriginator->lpszName != (char *)0)
		 && MAPIMessage->lpOriginator->lpszName[0] != '\0')
			strncpy(received_msg.sender,
				MAPIMessage->lpOriginator->lpszName,
				sizeof(received_msg.sender) - 1);
		else
			strncpy(received_msg.sender,
				MAPIMessage->lpOriginator->lpszAddress,
				sizeof(received_msg.sender) - 1);
		strncpy(received_msg.body,
			MAPIMessage->lpszNoteText,MAX_BODY_SIZE - 1);
		received_msg.body[MAX_BODY_SIZE - 1] = '\0';
		received_msg.body_in_ram = TRUE;
		status = fpMAPIFreeBuffer(MAPIMessage);
		InterlockedIncrement(&mail_fetched);
#ifdef MAPI_VERBOSE
		if (debugmapi) fprintf(dbgfile, "returning TRUE\n");
#endif
		return TRUE;
	}
#ifdef MAPI_VERBOSE
	else
		if (debugmapi) fprintf(dbgfile,"MAPIRead failed, status = %d\n", status);
	if (debugmapi) fprintf(dbgfile, "returning FALSE (failed)\n");
#endif
	return FALSE;
}

static void
MAPI_mail_finish()
{
	InterlockedIncrement(&mailthread_stopping);
	(void) fpMAPILogoff(MAPISession,0,0,0);
	(void) DeInitMAPI();
#ifdef MAPI_VERBOSE
	if (debugmapi) fclose(dbgfile);
#endif
	(void) _endthreadex(0);
}

static void
MAPI_mail_abort(reason)
unsigned long reason;
{
#ifdef MAPI_VERBOSE
	if (debugmapi) fprintf(dbgfile,
		"Terminating MAPI-thread due to error %d.\n", reason);
#endif
	MAPI_mail_finish();
}

static boolean
MAPI_mail_init(uname)
char *uname;
{
	unsigned long status;
	int count = 0;

#ifdef MAPI_VERBOSE
	if (debugmapi) dbgfile = fopen(MAPIDEBUGFILENAME,"w");
	if (debugmapi) fprintf(dbgfile,"Hello %s, NetHack is initializing MAPI.\n",
		uname);
#endif
	status = InitMAPI();
	if (status) {
#ifdef MAPI_VERBOSE
	    if (debugmapi) fprintf(dbgfile,"Error initializing MAPI %d\n", status);
#endif
	    return FALSE;
	}
	status = fpMAPILogon(0,uname,0L,0L,0L,(LPLHANDLE)&MAPISession);
	if (status != SUCCESS_SUCCESS) {
#ifdef MAPI_VERBOSE
	    if (debugmapi) fprintf(dbgfile,"Status of MAPI logon is %d\n", status);
#endif
	    return FALSE;
	}
#ifdef MAPI_VERBOSE
	if (debugmapi) fprintf(dbgfile,
		"Stage 1 of MAPI initialization successful.\n");
	if (debugmapi) fprintf(dbgfile,"MAPI Session handle: %d\n", MAPISession);
#endif
	status = MAPI_mail_context(&count);
	if (status == SUCCESS_SUCCESS) {
#ifdef MAPI_VERBOSE
	    if (debugmapi) fprintf(dbgfile,
		    "Stage 2 of MAPI initialization successful.\n");
	    if (debugmapi) fprintf(dbgfile,"Detected %d old unread messages.\n",
		    count);
#endif
	    return TRUE;
	}
#ifdef MAPI_VERBOSE
	if (debugmapi) fprintf(dbgfile,
		"Error, status of MAPI_mail_context() is %d.\n",
		status);
	if (debugmapi) fprintf(dbgfile,
		"Dismantling MAPI interface and cleaning up.\n");
#endif
	MAPI_mail_finish();
	return FALSE;
}

int InitMAPI()
{

    if (!(hLibrary = LoadLibrary("MAPI32.DLL")))
      return MAPI_LIB_FAIL;

    if ((fpMAPILogon = (LPMAPILOGON)GetProcAddress(hLibrary,"MAPILogon")) == NULL)
      return MAPI_FUNC_FAIL;

    if ((fpMAPILogoff = (LPMAPILOGOFF)GetProcAddress(hLibrary,"MAPILogoff")) == NULL)
      return MAPI_FUNC_FAIL;

    if ((fpMAPIFindNext= (LPMAPIFINDNEXT)GetProcAddress(hLibrary,"MAPIFindNext")) == NULL)
      return MAPI_FUNC_FAIL;

    if ((fpMAPIReadMail= (LPMAPIREADMAIL)GetProcAddress(hLibrary,"MAPIReadMail")) == NULL)
      return MAPI_FUNC_FAIL;

    if ((fpMAPIFindNext= (LPMAPIFINDNEXT)GetProcAddress(hLibrary,"MAPIFindNext")) == NULL)
      return MAPI_FUNC_FAIL;

    if ((fpMAPIFreeBuffer= (LPMAPIFREEBUFFER)GetProcAddress(hLibrary,"MAPIFreeBuffer")) == NULL)
      return MAPI_FUNC_FAIL;

#ifdef MAPI_VERBOSE
    if (debugmapi) {
	fprintf(dbgfile,"Entry Points:\n");
    	fprintf(dbgfile,"MAPILogon      = %p\n",fpMAPILogon);
    	fprintf(dbgfile,"MAPILogoff     = %p\n",fpMAPILogoff);
    	fprintf(dbgfile,"MAPIFindNext   = %p\n",fpMAPIFindNext);
   	fprintf(dbgfile,"MAPIFreeBuffer = %p\n",fpMAPIReadMail);
    	fprintf(dbgfile,"MAPIReadMail   = %p\n",fpMAPIFreeBuffer);
    }
#endif
    return 0;
}

int DeInitMAPI()
{

    fpMAPILogon = NULL;
    fpMAPILogoff= NULL;
    fpMAPIFindNext= NULL;
    fpMAPIReadMail= NULL;
    fpMAPIFreeBuffer = NULL;
    FreeLibrary(hLibrary);
    return 0;
}

#endif /*LAN_MAIL*/

