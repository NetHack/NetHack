/* NetHack 3.6	winami.c	$NHDT-Date: 1501981093 2017/08/06 00:58:13 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.20 $ */
/* Copyright (c) Gregg Wonderly, Naperville, Illinois,  1991,1992,1993,1996.
 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef CROSS_TO_AMIGA
#include "NH:sys/amiga/windefs.h"
#include "NH:sys/amiga/winext.h"
#include "NH:sys/amiga/winproto.h"
#else
#include "windefs.h"
#include "winext.h"
#include "winproto.h"
#endif

#include "dlb.h"

#ifdef CROSS_TO_AMIGA
#define strnicmp strncmpi
#endif

#ifdef AMIGA_INTUITION

static int put_ext_cmd(char *, int, struct amii_WinDesc *, int);

struct amii_DisplayDesc *amiIDisplay; /* the Amiga Intuition descriptor */
struct Rectangle lastinvent, lastmsg;
int clipping = 0;
int clipx = 0;
int clipy = 0;
int clipxmax = 0;
int clipymax = 0;
int scrollmsg = 1;
int alwaysinvent = 0;
int amii_numcolors;
long amii_scrnmode;

/* Interface definition, for use by windows.c and winprocs.h to provide
 * the intuition interface for the amiga...
 */
struct window_procs amii_procs = {
    "amii", WC_COLOR | WC_HILITE_PET | WC_INVERSE,
    0L,
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},   /* color availability */
    amii_init_nhwindows,
    amii_player_selection, amii_askname, amii_get_nh_event,
    amii_exit_nhwindows, amii_suspend_nhwindows, amii_resume_nhwindows,
    amii_create_nhwindow, amii_clear_nhwindow, amii_display_nhwindow,
    amii_destroy_nhwindow, amii_curs, amii_putstr, genl_putmixed,
    amii_display_file, amii_start_menu, amii_add_menu, amii_end_menu,
    amii_select_menu, genl_message_menu, amii_update_inventory,
    amii_mark_synch, amii_wait_synch,
#ifdef CLIPPING
    amii_cliparound,
#endif
#ifdef POSITIONBAR
    donull,
#endif
    amii_print_glyph, amii_raw_print, amii_raw_print_bold, amii_nhgetch,
    amii_nh_poskey, amii_bell, amii_doprev_message, amii_yn_function,
    amii_getlin, amii_get_ext_cmd, amii_number_pad, amii_delay_output,
#ifdef CHANGE_COLOR /* only a Mac option currently */
    amii_change_color, amii_get_color_string,
#endif
    /* other defs that really should go away (they're tty specific) */
    amii_delay_output, amii_delay_output, amii_outrip, genl_preference_update,
    genl_getmsghistory, genl_putmsghistory,
    genl_status_init, genl_status_finish, genl_status_enablefield,
    genl_status_update,
    genl_can_suspend_yes,
};

/* The view window layout uses the same function names so we can use
 * a shared library to allow the executable to be smaller.
 */
struct window_procs amiv_procs = {
    "amitile", WC_COLOR | WC_HILITE_PET | WC_INVERSE,
    0L,
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},   /* color availability */
    amii_init_nhwindows,
    amii_player_selection, amii_askname, amii_get_nh_event,
    amii_exit_nhwindows, amii_suspend_nhwindows, amii_resume_nhwindows,
    amii_create_nhwindow, amii_clear_nhwindow, amii_display_nhwindow,
    amii_destroy_nhwindow, amii_curs, amii_putstr, genl_putmixed,
    amii_display_file, amii_start_menu, amii_add_menu, amii_end_menu,
    amii_select_menu, genl_message_menu, amii_update_inventory,
    amii_mark_synch, amii_wait_synch,
#ifdef CLIPPING
    amii_cliparound,
#endif
#ifdef POSITIONBAR
    donull,
#endif
    amii_print_glyph, amii_raw_print, amii_raw_print_bold, amii_nhgetch,
    amii_nh_poskey, amii_bell, amii_doprev_message, amii_yn_function,
    amii_getlin, amii_get_ext_cmd, amii_number_pad, amii_delay_output,
#ifdef CHANGE_COLOR /* only a Mac option currently */
    amii_change_color, amii_get_color_string,
#endif
    /* other defs that really should go away (they're tty specific) */
    amii_delay_output, amii_delay_output, amii_outrip, genl_preference_update,
    genl_getmsghistory, genl_putmsghistory,
    genl_status_init, genl_status_finish, genl_status_enablefield,
    genl_status_update,
    genl_can_suspend_yes,
};

unsigned short amii_initmap[AMII_MAXCOLORS];
/* Default pens used unless user overides in nethack.cnf. */
unsigned short amii_init_map[AMII_MAXCOLORS] = {
    0x0000, /* color #0  C_BLACK    */
    0x0FFF, /* color #1  C_WHITE    */
    0x0830, /* color #2  C_BROWN    */
    0x07ac, /* color #3  C_CYAN     */
    0x0181, /* color #4  C_GREEN    */
    0x0C06, /* color #5  C_MAGENTA  */
    0x023E, /* color #6  C_BLUE     */
    0x0c00, /* color #7  C_RED      */
};

unsigned short amiv_init_map[AMII_MAXCOLORS] = {
    0x0000, /* color #0  C_BLACK    */
    0x0fff, /* color #1  C_WHITE    */
    0x00bf, /* color #2  C_CYAN     */
    0x0f60, /* color #3  C_ORANGE   */
    0x000f, /* color #4  C_BLUE     */
    0x0090, /* color #5  C_GREEN    */
    0x069b, /* color #6  C_GREY     */
    0x0f00, /* color #7  C_RED      */
    0x06f0, /* color #8  C_LTGREEN  */
    0x0ff0, /* color #9  C_YELLOW   */
    0x0f0f, /* color #10 C_MAGENTA  */
    0x0940, /* color #11 C_BROWN    */
    0x0466, /* color #12 C_GREYBLUE */
    0x0c40, /* color #13 C_LTBROWN  */
    0x0ddb, /* color #14 C_LTGREY   */
    0x0fb9, /* color #15 C_PEACH    */

    /* Pens for dripens etc under AA or better */
    0x0222, /* color #16 */
    0x0fdc, /* color #17 */
    0x0000, /* color #18 */
    0x0ccc, /* color #19 */
    0x0bbb, /* color #20 */
    0x0BA9, /* color #21 */
    0x0999, /* color #22 */
    0x0987, /* color #23 */
    0x0765, /* color #24 */
    0x0666, /* color #25 */
    0x0555, /* color #26 */
    0x0533, /* color #27 */
    0x0333, /* color #28 */
    0x018f, /* color #29 */
    0x0f81, /* color #30 */
    0x0fff, /* color #31 */
};

#if !defined(TTY_GRAPHICS) \
    || defined(SHAREDLIB) /* this should be shared better */
char morc; /* the character typed in response to a --more-- prompt */
#endif
char spaces[76] = "                                                          "
                  "                 ";

winid WIN_BASE = WIN_ERR;
winid WIN_OVER = WIN_ERR;
winid amii_rawprwin = WIN_ERR;

/* Changed later during window/screen opens... */
int txwidth = FONTWIDTH, txheight = FONTHEIGHT, txbaseline = FONTBASELINE;

/* If a 240 or more row screen is in front when we start, this will be
 * set to 1, and the windows will be given borders to allow them to be
 * arranged differently.  The Message window may eventually get a scroller...
 */
int bigscreen = 0;

/* This gadget data is replicated for menu/text windows... */
struct PropInfo PropScroll = {
    AUTOKNOB | FREEVERT, 0xffff, 0xffff, 0xffff, 0xffff,
};
struct Image Image1 = { 0, 0, 7, 102, 0, NULL, 0x0000, 0x0000, NULL };
struct Gadget MenuScroll = { NULL, -15, 10, 15, -19, GRELRIGHT | GRELHEIGHT,
                             RELVERIFY | FOLLOWMOUSE | RIGHTBORDER
                                 | GADGIMMEDIATE | RELVERIFY,
                             PROPGADGET, (APTR) &Image1, NULL, NULL, NULL,
                             (APTR) &PropScroll, 1, NULL };

/* This gadget is for the message window... */
struct PropInfo MsgPropScroll = {
    AUTOKNOB | FREEVERT, 0xffff, 0xffff, 0xffff, 0xffff,
};
struct Image MsgImage1 = { 0, 0, 7, 102, 0, NULL, 0x0000, 0x0000, NULL };
struct Gadget MsgScroll = { NULL, -15, 10, 14, -19, GRELRIGHT | GRELHEIGHT,
                            RELVERIFY | FOLLOWMOUSE | RIGHTBORDER
                                | GADGIMMEDIATE | RELVERIFY,
                            PROPGADGET, (APTR) &MsgImage1, NULL, NULL, NULL,
                            (APTR) &MsgPropScroll, 1, NULL };

int wincnt = 0; /* # of nh windows opened */

/* We advertise a public screen to allow some people to do other things
 * while they are playing...  like compiling...
 */

#ifdef INTUI_NEW_LOOK
extern struct Hook fillhook;
struct TagItem tags[] = {
    { WA_BackFill, (ULONG) &fillhook },
    { WA_PubScreenName, (ULONG) "NetHack" },
    { TAG_DONE, 0 },
};
#endif

/*
 * The default dimensions and status values for each window type.  The
 * data here is generally changed in create_nhwindow(), so beware that
 * what you see here may not be exactly what you get.
 */
struct win_setup new_wins[] = {

    /* First entry not used, types are based at 1 */
    { { 0 } },

    /* NHW_MESSAGE */
    { { 0, 1, 640, 11, 0xff, 0xff,
        NEWSIZE | GADGETUP | GADGETDOWN | MOUSEMOVE | MOUSEBUTTONS | RAWKEY,
        BORDERLESS | ACTIVATE | SMART_REFRESH
#ifdef INTUI_NEW_LOOK
            | WFLG_NW_EXTENDED
#endif
        ,
        NULL, NULL, (UBYTE *) "Messages", NULL, NULL, 320, 40, 0xffff, 0xffff,
#ifdef INTUI_NEW_LOOK
        PUBLICSCREEN, tags
#else
        CUSTOMSCREEN
#endif
      },
      0,
      0,
      1,
      1,
      80,
      80 },

    /* NHW_STATUS */
    { { 0, 181, 640, 24, 0xff, 0xff, RAWKEY | MENUPICK | DISKINSERTED,
        BORDERLESS | ACTIVATE | SMART_REFRESH | BACKDROP
#ifdef INTUI_NEW_LOOK
            | WFLG_NW_EXTENDED
#endif
        ,
        NULL, NULL, (UBYTE *) "Game Status", NULL, NULL, 0, 0, 0xffff, 0xffff,
#ifdef INTUI_NEW_LOOK
        PUBLICSCREEN, tags
#else
        CUSTOMSCREEN
#endif
      },
      0,
      0,
      2,
      2,
      78,
      78 },

    /* NHW_MAP */
    { { 0, 0, WIDTH, WINDOWHEIGHT, 0xff, 0xff,
        RAWKEY | MENUPICK | MOUSEBUTTONS | ACTIVEWINDOW | MOUSEMOVE,
        BORDERLESS | ACTIVATE | SMART_REFRESH | BACKDROP
#ifdef INTUI_NEW_LOOK
            | WFLG_NW_EXTENDED
#endif
        ,
        NULL, NULL, (UBYTE *) "Dungeon Map", NULL, NULL, 64, 64, 0xffff,
        0xffff,
#ifdef INTUI_NEW_LOOK
        PUBLICSCREEN, tags
#else
        CUSTOMSCREEN
#endif
      },
      0,
      0,
      22,
      22,
      80,
      80 },

    /* NHW_MENU */
    { { 400, 10, 10, 10, 0xff, 0xff,
        RAWKEY | MENUPICK | DISKINSERTED | MOUSEMOVE | MOUSEBUTTONS | GADGETUP
            | GADGETDOWN | CLOSEWINDOW | VANILLAKEY | NEWSIZE
            | INACTIVEWINDOW,
        WINDOWSIZING | WINDOWCLOSE | WINDOWDRAG | ACTIVATE | SMART_REFRESH
#ifdef INTUI_NEW_LOOK
            | WFLG_NW_EXTENDED
#endif
        ,
        &MenuScroll, NULL, NULL, NULL, NULL, 64, 32, 0xffff, 0xffff,
#ifdef INTUI_NEW_LOOK
        PUBLICSCREEN, tags
#else
        CUSTOMSCREEN
#endif
      },
      0,
      0,
      1,
      1,
      22,
      78 },

    /* NHW_TEXT */
    { { 0, 0, 640, 200, 0xff, 0xff,
        RAWKEY | MENUPICK | DISKINSERTED | MOUSEMOVE | GADGETUP | CLOSEWINDOW
            | VANILLAKEY | NEWSIZE,
        WINDOWSIZING | WINDOWCLOSE | WINDOWDRAG | ACTIVATE | SMART_REFRESH
#ifdef INTUI_NEW_LOOK
            | WFLG_NW_EXTENDED
#endif
        ,
        &MenuScroll, NULL, (UBYTE *) NULL, NULL, NULL, 100, 32, 0xffff,
        0xffff,
#ifdef INTUI_NEW_LOOK
        PUBLICSCREEN, tags
#else
        CUSTOMSCREEN
#endif
      },
      0,
      0,
      1,
      1,
      22,
      78 },

    /* NHW_BASE */
    { { 0, 0, WIDTH, WINDOWHEIGHT, 0xff, 0xff,
        RAWKEY | MENUPICK | MOUSEBUTTONS,
        BORDERLESS | ACTIVATE | SMART_REFRESH | BACKDROP
#ifdef INTUI_NEW_LOOK
            | WFLG_NW_EXTENDED
#endif
        ,
        NULL, NULL, (UBYTE *) NULL, NULL, NULL, -1, -1, 0xffff, 0xffff,
#ifdef INTUI_NEW_LOOK
        PUBLICSCREEN, tags
#else
        CUSTOMSCREEN
#endif
      },
      0,
      0,
      22,
      22,
      80,
      80 },

    /* NHW_OVER */
    { { 320, 20, 319, 179, 0xff, 0xff, RAWKEY | MENUPICK | MOUSEBUTTONS,
        BORDERLESS | ACTIVATE | SMART_REFRESH | BACKDROP
#ifdef INTUI_NEW_LOOK
            | WFLG_NW_EXTENDED
#endif
        ,
        NULL, NULL, (UBYTE *) NULL, NULL, NULL, 64, 32, 0xffff, 0xffff,
#ifdef INTUI_NEW_LOOK
        PUBLICSCREEN, tags
#else
        CUSTOMSCREEN
#endif
      },
      0,
      0,
      22,
      22,
      80,
      80 },
};

const char winpanicstr[] = "Bad winid %d in %s()";

/* The opened windows information */
struct amii_WinDesc *amii_wins[MAXWIN + 1];

#ifdef INTUI_NEW_LOOK
/*
 * NUMDRIPENS varies based on headers, so don't use it
 * here, its value is used elsewhere.
 */
UWORD amii_defpens[20];

struct TagItem scrntags[] = {
    { SA_PubName, (ULONG) "NetHack" },
    { SA_Overscan, OSCAN_TEXT },
    { SA_AutoScroll, TRUE },
#if LIBRARY_VERSION >= 39
    { SA_Interleaved, TRUE },
#endif
    { SA_Pens, (ULONG) 0 },
    { SA_DisplayID, 0 },
    { TAG_DONE, 0 },
};

#endif

struct NewScreen NewHackScreen = { 0, 0, WIDTH, SCREENHEIGHT, 3, 0,
                                   1, /* DetailPen, BlockPen */
                                   HIRES, CUSTOMSCREEN
#ifdef INTUI_NEW_LOOK
                                              | NS_EXTENDED
#endif
                                   ,
                                   &Hack80, /* Font */
                                   NULL,    /*(UBYTE *)" NetHack X.Y.Z" */
                                   NULL,    /* Gadgets */
                                   NULL,    /* CustomBitmap */
#ifdef INTUI_NEW_LOOK
                                   scrntags
#endif
};

/*
 * plname is filled either by an option (-u Player  or  -uPlayer) or
 * explicitly (by being the wizard) or by askname.
 * It may still contain a suffix denoting pl_character.
 * Always called after init_nhwindows() and before
 * init_sound_and_display_gamewindows().
 */
void
amii_askname()
{
    char plnametmp[300]; /* From winreq.c: sizeof(StrStringSIBuff) */
    *plnametmp = 0;
    do {
        amii_getlin("Who are you?", plnametmp);
    } while (strlen(plnametmp) == 0);

    strncpy(gp.plname, plnametmp, PL_NSIZ - 1); /* Avoid overflowing plname[] */
    gp.plname[PL_NSIZ - 1] = 0;

    if (*gp.plname == '\33') {
        clearlocks();
        exit_nhwindows(NULL);
        nh_terminate(0);
    }
}

/* Discarded ... -jhsa
#include "NH:sys/amiga/char.c"
*/

/* Get the player selection character */

#if 0 /* New function at the bottom */
void
amii_player_selection()
{
    register struct Window *cwin;
    register struct IntuiMessage *imsg;
    register int aredone = 0;
    register struct Gadget *gd;
    static int once = 0;
    long class, code;

    amii_clear_nhwindow( WIN_BASE );
    if (validrole(flags.initrole))
	return;
    else {
	flags.initrole = randrole(FALSE);
	return;
    }
#if 0 /* Don't query the user ... instead give random character -jhsa */

#if 0 /* OBSOLETE */
    if( *gp.pl_character ){
	gp.pl_character[ 0 ] = toupper( gp.pl_character[ 0 ] );
	if( strchr( pl_classes, gp.pl_character[ 0 ] ) )
	    return;
    }
#endif

    if( !once ){
	if( bigscreen ){
	    Type_NewWindowStructure1.TopEdge =
	      (HackScreen->Height/2) - (Type_NewWindowStructure1.Height/2);
	}
	for( gd = Type_NewWindowStructure1.FirstGadget; gd;
	  gd = gd->NextGadget )
	{
	    if( gd->GadgetID != 0 )
		SetBorder( gd );
	}
	once = 1;
    }

    if( WINVERS_AMIV )
    {
#ifdef INTUI_NEW_LOOK
	Type_NewWindowStructure1.Extension = wintags;
	Type_NewWindowStructure1.Flags |= WFLG_NW_EXTENDED;
	fillhook.h_Entry = (ULONG(*)())LayerFillHook;
	fillhook.h_Data = (void *)-2;
	fillhook.h_SubEntry = 0;
#endif
    }

    Type_NewWindowStructure1.Screen = HackScreen;
    if( ( cwin = OpenShWindow( (void *)&Type_NewWindowStructure1 ) ) == NULL )
    {
	return;
    }
#if 0
    WindowToFront( cwin );
#endif

    while( !aredone )
    {
	WaitPort( cwin->UserPort );
	while( ( imsg = (void *) GetMsg( cwin->UserPort ) ) != NULL )
	{
	    class = imsg->Class;
	    code = imsg->Code;
	    gd = (struct Gadget *)imsg->IAddress;
	    ReplyMsg( (struct Message *)imsg );

	    switch( class )
	    {
	    case VANILLAKEY:
		if( strchr( pl_classes, toupper( code ) ) )
		{
		    gp.pl_character[0] = toupper( code );
		    aredone = 1;
		}
		else if( code == ' ' || code == '\n' || code == '\r' )
		{
		    flags.initrole = randrole(FALSE);
#if 0 /* OBSOLETE */
		    strcpy( gp.pl_character, roles[ rnd( 11 ) ] );
#endif
		    aredone = 1;
		    amii_clear_nhwindow( WIN_BASE );
		    CloseShWindow( cwin );
		    RandomWindow( gp.pl_character );
		    return;
		}
		else if( code == 'q' || code == 'Q' )
		{
		CloseShWindow( cwin );
		clearlocks();
		exit_nhwindows(NULL);
		nh_terminate(0);
		}
		else
		    DisplayBeep( NULL );
		break;

	    case GADGETUP:
		switch( gd->GadgetID )
		{
		case 1: /* Random Character */
		    flags.initrole = randrole(FALSE);
#if 0 /* OBSOLETE */
		    strcpy( gp.pl_character, roles[ rnd( 11 ) ] );
#endif
		    amii_clear_nhwindow( WIN_BASE );
		    CloseShWindow( cwin );
		    RandomWindow( gp.pl_character );
		    return;

		default:
		    gp.pl_character[0] = gd->GadgetID;
		    break;
		}
		aredone = 1;
		break;

	    case CLOSEWINDOW:
		CloseShWindow( cwin );
		clearlocks();
		exit_nhwindows(NULL);
		nh_terminate(0);
		break;
	    }
	}
    }
    amii_clear_nhwindow( WIN_BASE );
    CloseShWindow( cwin );
#endif /* Do not query user ... -jhsa */
}
#endif /* Function elsewhere */

#if 0 /* Unused ... -jhsa */

#include "NH:sys/amiga/randwin.c"

void
RandomWindow( name )
    char *name;
{
    struct MsgPort *tport;
    struct timerequest *trq;
    static int once = 0;
    struct Gadget *gd;
    struct Window *w;
    struct IntuiMessage *imsg;
    int ticks = 0, aredone = 0, timerdone = 0;
    long mask, got;

    tport = CreateMsgPort();
    trq = (struct timerequest *)CreateIORequest( tport, sizeof( *trq ) );
    if( tport == NULL || trq == NULL )
    {
allocerr:
	if( tport ) DeleteMsgPort( tport );
	if( trq ) DeleteIORequest( (struct IORequest *)trq );
	Delay( 8 * 50 );
	return;
    }

    if( OpenDevice( TIMERNAME, UNIT_VBLANK, (struct IORequest *)trq, 0L ) != 0 )
	goto allocerr;

    trq->tr_node.io_Command = TR_ADDREQUEST;
    trq->tr_time.tv_secs = 8;
    trq->tr_time.tv_micro = 0;

    SendIO( (struct IORequest *)trq );

    /* Place the name in the center of the screen */
    Rnd_IText5.IText = name;
    Rnd_IText6.LeftEdge = Rnd_IText4.LeftEdge +
		(strlen(Rnd_IText4.IText)+1)*8;
    Rnd_NewWindowStructure1.Width = (
	    (strlen( Rnd_IText2.IText )+1) * 8 ) +
	    HackScreen->WBorLeft + HackScreen->WBorRight;
    Rnd_IText5.LeftEdge = (Rnd_NewWindowStructure1.Width -
	    (strlen(name)*8))/2;

    gd = Rnd_NewWindowStructure1.FirstGadget;
    gd->LeftEdge = (Rnd_NewWindowStructure1.Width - gd->Width)/2;
	/* Chose correct modifier */
    Rnd_IText6.IText = "a";
    switch( *name )
    {
    case 'a': case 'e': case 'i': case 'o':
    case 'u': case 'A': case 'E': case 'I':
    case 'O': case 'U':
	Rnd_IText6.IText = "an";
	break;
    }

    if( !once )
    {
	if( bigscreen )
	{
	    Rnd_NewWindowStructure1.TopEdge =
		(HackScreen->Height/2) - (Rnd_NewWindowStructure1.Height/2);
	}
	for( gd = Rnd_NewWindowStructure1.FirstGadget; gd; gd = gd->NextGadget )
	{
	    if( gd->GadgetID != 0 )
		SetBorder( gd );
	}
	Rnd_NewWindowStructure1.IDCMPFlags |= VANILLAKEY;

	once = 1;
    }

    if( WINVERS_AMIV )
    {
#ifdef INTUI_NEW_LOOK
	Rnd_NewWindowStructure1.Extension = wintags;
	Rnd_NewWindowStructure1.Flags |= WFLG_NW_EXTENDED;
	fillhook.h_Entry = (ULONG(*)())LayerFillHook;
	fillhook.h_Data = (void *)-2;
	fillhook.h_SubEntry = 0;
#endif
    }

    Rnd_NewWindowStructure1.Screen = HackScreen;
    if( ( w = OpenShWindow( (void *)&Rnd_NewWindowStructure1 ) ) == NULL )
    {
	AbortIO( (struct IORequest *)trq );
	WaitIO( (struct IORequest *)trq );
	CloseDevice( (struct IORequest *)trq );
	DeleteIORequest( (struct IORequest *) trq );
	DeleteMsgPort( tport );
	Delay( 50 * 8 );
	return;
    }

    PrintIText( w->RPort, &Rnd_IntuiTextList1, 0, 0 );

    mask = (1L << tport->mp_SigBit)|(1L << w->UserPort->mp_SigBit);
    while( !aredone )
    {
	got = Wait( mask );
	if( got & (1L << tport->mp_SigBit ) )
	{
	    aredone = 1;
	    timerdone = 1;
	    GetMsg( tport );
        }
        while( w && ( imsg = (struct IntuiMessage *) GetMsg( w->UserPort ) ) )
        {
	    switch( (long)imsg->Class )
	    {
		/* Must be up for a little while... */
	    case INACTIVEWINDOW:
		if( ticks >= 40 )
		    aredone = 1;
		break;

	    case INTUITICKS:
		++ticks;
		break;

	    case GADGETUP:
		aredone = 1;
		break;

	    case VANILLAKEY:
		if(imsg->Code=='\n' || imsg->Code==' ' || imsg->Code=='\r')
		    aredone = 1;
		break;
	    }
	    ReplyMsg( (struct Message *)imsg );
        }
    }

    if( !timerdone )
    {
	AbortIO( (struct IORequest *)trq );
	WaitIO( (struct IORequest *)trq );
    }

    CloseDevice( (struct IORequest *)trq );
    DeleteIORequest( (struct IORequest *) trq );
    DeleteMsgPort( tport );
    if(w) CloseShWindow( w );
}
#endif /* Discarded randwin ... -jhsa */

/* this should probably not be needed (or be renamed)
void
flush_output(){} */

/* Read in an extended command - doing command line completion for
 * when enough characters have been entered to make a unique command.
 */
int
amii_get_ext_cmd(void)
{
    menu_item *mip;
    anything id;
    struct amii_WinDesc *cw;
#ifdef EXTMENU
    winid win;
    int i;
    char buf[256];
#endif
    int colx;
    int bottom = 0;
    struct Window *w;
    char obufp[100];
    register char *bufp = obufp;
    register int c;
    int com_index, oindex;
    int did_comp = 0; /* did successful completion? */
    int sel = -1;

    if (WIN_MESSAGE == WIN_ERR || (cw = amii_wins[WIN_MESSAGE]) == NULL)
        panic(winpanicstr, WIN_MESSAGE, "get_ext_cmd");
    w = cw->win;
    bottom = amii_msgborder(w);
    colx = 3;

#ifdef EXTMENU
    if (iflags.extmenu) {
        win = amii_create_nhwindow(NHW_MENU);
        amii_start_menu(win, MENU_BEHAVE_STANDARD);
        pline("#");
        amii_putstr(WIN_MESSAGE, -1, " ");

        for (i = 0; extcmdlist[i].ef_txt != NULL; ++i) {
            id.a_char = *extcmdlist[i].ef_txt;
            sprintf(buf, "%-10s - %s ", extcmdlist[i].ef_txt,
                    extcmdlist[i].ef_desc);
            amii_add_menu(win, NO_GLYPH, &id, extcmdlist[i].ef_txt[0], 0, 0,
                          buf, MENU_ITEMFLAGS_NONE);
        }

        amii_end_menu(win, (char *) 0);
        sel = amii_select_menu(win, PICK_ONE, &mip);
        amii_destroy_nhwindow(win);

        if (sel == 1) {
            sel = mip->item.a_char;
            for (i = 0; extcmdlist[i].ef_txt != NULL; ++i) {
                if (sel == extcmdlist[i].ef_txt[0])
                    break;
            }

            /* copy in the text */
            if (extcmdlist[i].ef_txt != NULL) {
                amii_clear_nhwindow(WIN_MESSAGE);
                (void) put_ext_cmd((char *) extcmdlist[i].ef_txt, 0, cw,
                                   bottom);
                return (i);
            } else
                DisplayBeep(NULL);
        }

        return (-1);
    }
#endif

    amii_clear_nhwindow(WIN_MESSAGE); /* Was NHW_MESSAGE */
    if (scrollmsg) {
        pline("#");
        amii_addtopl(" ");
    } else {
        pline("# ");
    }

    sel = -1;
    while ((c = WindowGetchar()) != EOF) {
        amii_curs(WIN_MESSAGE, colx, bottom);
        if (c == '?') {
            int win, i;
            char buf[100];

            if (did_comp) {
                while (bufp != obufp) {
                    bufp--;
                    amii_curs(WIN_MESSAGE, --colx, bottom);
                    Text(w->RPort, spaces, 1);
                    amii_curs(WIN_MESSAGE, colx, bottom);
                    did_comp = 0;
                }
            }

            win = amii_create_nhwindow(NHW_MENU);
            amii_start_menu(win, MENU_BEHAVE_STANDARD);

            for (i = 0; extcmdlist[i].ef_txt != NULL; ++i) {
                id.a_char = extcmdlist[i].ef_txt[0];
                sprintf(buf, "%-10s - %s ", extcmdlist[i].ef_txt,
                        extcmdlist[i].ef_desc);
                amii_add_menu(win, NO_GLYPH, &id, extcmdlist[i].ef_txt[0], 0,
                              0, buf, MENU_ITEMFLAGS_NONE);
            }

            amii_end_menu(win, (char *) 0);
            sel = amii_select_menu(win, PICK_ONE, &mip);
            amii_destroy_nhwindow(win);

            if (sel == 0) {
                return (-1);
            } else {
                sel = mip->item.a_char;
                for (i = 0; extcmdlist[i].ef_txt != NULL; ++i) {
                    if (sel == extcmdlist[i].ef_txt[0])
                        break;
                }

                /* copy in the text */
                if (extcmdlist[i].ef_txt != NULL) {
                    amii_clear_nhwindow(WIN_MESSAGE);
                    strcpy(bufp = obufp, extcmdlist[i].ef_txt);
                    (void) put_ext_cmd(obufp, colx, cw, bottom);
                    return (i);
                } else
                    DisplayBeep(NULL);
            }
        } else if (c == '\033') {
            return (-1);
        } else if (c == '\b') {
            if (did_comp) {
                while (bufp != obufp) {
                    bufp--;
                    amii_curs(WIN_MESSAGE, --colx, bottom);
                    Text(w->RPort, spaces, 1);
                    amii_curs(WIN_MESSAGE, colx, bottom);
                    did_comp = 0;
                    sel = -1;
                }
            } else if (bufp != obufp) {
                sel = -1;
                bufp--;
                amii_curs(WIN_MESSAGE, --colx, bottom);
                Text(w->RPort, spaces, 1);
                amii_curs(WIN_MESSAGE, colx, bottom);
            } else
                DisplayBeep(NULL);
        } else if (c == '\n' || c == '\r') {
            return (sel);
        } else if (c >= ' ' && c < '\177') {
            /* avoid isprint() - some people don't have it
               ' ' is not always a printing char */
            *bufp = c;
            bufp[1] = 0;
            oindex = 0;
            com_index = -1;

            while (extcmdlist[oindex].ef_txt != NULL) {
                if (!strnicmp(obufp, (char *) extcmdlist[oindex].ef_txt,
                              strlen(obufp))) {
                    if (com_index == -1) /* No matches yet*/
                        com_index = oindex;
                    else /* More than 1 match */
                        com_index = -2;
                }
                oindex++;
            }

            if (com_index >= 0 && *obufp) {
                Strcpy(obufp, extcmdlist[com_index].ef_txt);
                /* finish printing our string */
                colx = put_ext_cmd(obufp, colx, cw, bottom);
                bufp = obufp; /* reset it */
                if (strlen(obufp) < BUFSZ - 1 && strlen(obufp) < COLNO)
                    bufp += strlen(obufp);
                did_comp = 1;
                sel = com_index;
            } else {
                colx = put_ext_cmd(obufp, colx, cw, bottom);
                if (bufp - obufp < BUFSZ - 1 && bufp - obufp < COLNO)
                    bufp++;
            }
        } else if (c == ('X' - 64) || c == '\177') {
            colx = 0;
            amii_clear_nhwindow(WIN_MESSAGE);
            pline("# ");
            bufp = obufp;
        } else
            DisplayBeep(NULL);
    }
    return (-1);
}

static int
put_ext_cmd(obufp, colx, cw, bottom)
char * obufp;
int colx, bottom;
struct amii_WinDesc *cw;
{
    struct Window *w = cw->win;
    char *t;

    t = (char *) alloc(strlen(obufp) + 7);
    if (t != NULL) {
        if (scrollmsg) {
            sprintf(t, "xxx%s", obufp);
            t[0] = 1;
            t[1] = 1;
            t[2] = '#';
            amii_curs(WIN_MESSAGE, 0, bottom);
            SetAPen(w->RPort, C_WHITE);
            Text(w->RPort, "># ", 3);
            /* SetAPen( w->RPort, C_BLACK ); */ /* Black text on black
                                                 * screen doesn't look
                                                 * too well ... -jhsa */
            Text(w->RPort, t + 3, strlen(t) - 3);
        } else {
            sprintf(t, "# %s", obufp);
            amii_curs(WIN_MESSAGE, 0, bottom);
            SetAPen(w->RPort, C_WHITE);
            Text(w->RPort, t, strlen(t));
        }
        if (scrollmsg)
            SetAPen(w->RPort, C_WHITE);
        if (cw->data[cw->maxrow - 1])
            free(cw->data[cw->maxrow - 1]);
        cw->data[cw->maxrow - 1] = t;
    } else {
        amii_curs(WIN_MESSAGE, 0, bottom);
        SetAPen(w->RPort, C_WHITE);
        Text(w->RPort, "# ", 2);
        /* SetAPen( w->RPort, C_BLACK ); */ /* Black on black ... -jhsa */
        Text(w->RPort, obufp, strlen(obufp));
        SetAPen(w->RPort, C_WHITE);
    }
    amii_curs(WIN_MESSAGE, colx = strlen(obufp) + 3 + (scrollmsg != 0),
              bottom);
    return colx;
}

/* Ask a question and get a response */
char
amii_yn_function(query, resp, def)
const char * query, *resp;
char def;
{
    /*
     *   Generic yes/no function. 'def' is the default (returned by space or
     *   return; 'esc' returns 'q', or 'n', or the default, depending on
     *   what's in the string. The 'query' string is printed before the user
     *   is asked about the string.
     *   If resp is NULL, any single character is accepted and returned.
     *   If not-NULL, only characters in it are allowed (exceptions:  the
     *   quitchars are always allowed, and if it contains '#' then digits
     *   are allowed); if it includes an <esc>, anything beyond that won't
     *   be shown in the prompt to the user but will be acceptable as input.
     */
    register char q;
    char rtmp[40];
    boolean digit_ok, allow_num;
    char prompt[BUFSZ];
    register struct amii_WinDesc *cw;

    if (cw = amii_wins[WIN_MESSAGE])
        cw->disprows = 0;
    if (resp) {
        char *rb, respbuf[QBUFSZ];

        allow_num = (strchr(resp, '#') != 0);
        Strcpy(respbuf, resp);
        /* any acceptable responses that follow <esc> aren't displayed */
        if ((rb = strchr(respbuf, '\033')) != 0)
            *rb = '\0';
        (void) strncpy(prompt, query, QBUFSZ - 1);
        prompt[QBUFSZ - 1] = '\0';
        Sprintf(eos(prompt), " [%s]", respbuf);
        if (def)
            Sprintf(eos(prompt), " (%c)", def);
        Strcat(prompt, " ");
        pline("%s", prompt);
    } else {
        amii_putstr(WIN_MESSAGE, 0, query);
        cursor_on(WIN_MESSAGE);
        q = WindowGetchar();
        cursor_off(WIN_MESSAGE);
        *rtmp = q;
        rtmp[1] = '\0';
        amii_addtopl(rtmp);
        goto clean_up;
    }

    do { /* loop until we get valid input */
        cursor_on(WIN_MESSAGE);
        q = lowc(WindowGetchar());
        cursor_off(WIN_MESSAGE);
#if 0
/* fix for PL2 */
        if (q == '\020') { /* ctrl-P */
            if(!doprev)
                (void) tty_doprev_message(); /* need two initially */
            (void) tty_doprev_message();
            q = (char)0;
            doprev = 1;
            continue;
        } else if (doprev) {
            tty_clear_nhwindow(WIN_MESSAGE);
            cw->maxcol = cw->maxrow;
            doprev = 0;
            amii_addtopl(prompt);
            continue;
        }
#endif /*0*/
        digit_ok = allow_num && isdigit(q);
        if (q == '\033') {
            if (strchr(resp, 'q'))
                q = 'q';
            else if (strchr(resp, 'n'))
                q = 'n';
            else
                q = def;
            break;
        } else if (strchr(quitchars, q)) {
            q = def;
            break;
        }
        if (!strchr(resp, q) && !digit_ok) {
            amii_bell();
            q = (char) 0;
        } else if (q == '#' || digit_ok) {
            char z, digit_string[2];
            int n_len = 0;
            long value = 0;

            amii_addtopl("#"), n_len++;
            digit_string[1] = '\0';
            if (q != '#') {
                digit_string[0] = q;
                amii_addtopl(digit_string), n_len++;
                value = q - '0';
                q = '#';
            }
            do { /* loop until we get a non-digit */
                cursor_on(WIN_MESSAGE);
                z = lowc(WindowGetchar());
                cursor_off(WIN_MESSAGE);
                if (isdigit(z)) {
                    value = (10 * value) + (z - '0');
                    if (value < 0)
                        break; /* overflow: try again */
                    digit_string[0] = z;
                    amii_addtopl(digit_string), n_len++;
                } else if (z == 'y' || strchr(quitchars, z)) {
                    if (z == '\033')
                        value = -1; /* abort */
                    z = '\n';       /* break */
                } else if (z == '\b') {
                    if (n_len <= 1) {
                        value = -1;
                        break;
                    } else {
                        value /= 10;
                        removetopl(1), n_len--;
                    }
                } else {
                    value = -1; /* abort */
                    amii_bell();
                    break;
                }
            } while (z != '\n');
            if (value > 0)
                yn_number = value;
            else if (value == 0)
                q = 'n'; /* 0 => "no" */
            else {       /* remove number from top line, then try again */
                removetopl(n_len), n_len = 0;
                q = '\0';
            }
        }
    } while (!q);

    if (q != '#' && q != '\033') {
        Sprintf(rtmp, "%c", q);
        amii_addtopl(rtmp);
    }
 clean_up:
    cursor_off(WIN_MESSAGE);
    clear_nhwindow(WIN_MESSAGE);
    return q;
}

void
amii_display_file(fn, complain)
const char * fn;
boolean complain;
{
    register struct amii_WinDesc *cw;
    register int win;
    register dlb *fp;
    register char *t;
    char buf[200];

    if (fn == NULL)
        panic("NULL file name in display_file()");

    if ((fp = dlb_fopen(fn, RDTMODE)) == (dlb *) NULL) {
        if (complain) {
            sprintf(buf, "Can't display %s: %s", fn,
#if defined(_DCC) || defined(__GNUC__)
                    strerror(errno)
#else
#ifdef __SASC_60
                    __sys_errlist[errno]
#else
                    sys_errlist[errno]
#endif
#endif
                    );
            amii_addtopl(buf);
        }
        return;
    }
    win = amii_create_nhwindow(NHW_TEXT);

    /* Set window title to file name */
    if (cw = amii_wins[win])
        cw->morestr = (char *) fn;

    while (dlb_fgets(buf, sizeof(buf), fp) != NULL) {
        if (t = strchr(buf, '\n'))
            *t = 0;
        amii_putstr(win, 0, buf);
    }
    dlb_fclose(fp);

    /* If there were lines in the file, display those lines */
    if (amii_wins[win]->cury > 0)
        amii_display_nhwindow(win, TRUE);

    amii_wins[win]->morestr = NULL; /* don't free title string */
    amii_destroy_nhwindow(win);
}

/* Put a 3-D motif border around the gadget.  String gadgets or those
 * which do not have highlighting are rendered down.  Boolean gadgets
 * are rendered in the up position by default.
 */
void
SetBorder(gd)
register struct Gadget * gd;
{
    register struct Border *bp;
    register short *sp;
    register int i, inc = -1, dec = -1;
    int borders = 6;
    int hipen = sysflags.amii_dripens[SHINEPEN],
        shadowpen = sysflags.amii_dripens[SHADOWPEN];
#ifdef INTUI_NEW_LOOK
    struct DrawInfo *dip;
#endif

#ifdef INTUI_NEW_LOOK
    if (IntuitionBase->LibNode.lib_Version >= 37) {
        if ((dip = GetScreenDrawInfo(HackScreen)) != 0) {
            hipen = dip->dri_Pens[SHINEPEN];
            shadowpen = dip->dri_Pens[SHADOWPEN];
            FreeScreenDrawInfo(HackScreen, dip);
        }
    }
#endif
    /* Allocate two border structures one for up image and one for down
     * image, plus vector arrays for the border lines.
     */
    if (gd->GadgetType == STRGADGET)
        borders = 12;
    if ((bp = (struct Border *) alloc(((sizeof (struct Border) * 2)
                                       + (sizeof (short) * borders)) * 2))
        == NULL) {
        return;
    }

    /* For a string gadget, we expand the border beyond the area where
     * the text will be entered.
     */

    /* Remove any special rendering flags to avoid confusing intuition */
    gd->Flags &= ~(GADGHIGHBITS | GADGIMAGE);

    sp = (short *) (bp + 4);
    if (gd->GadgetType == STRGADGET
        || (gd->GadgetType == BOOLGADGET
            && (gd->Flags & GADGHIGHBITS) == GADGHNONE)) {
        sp[0] = -1;
        sp[1] = gd->Height - 1;
        sp[2] = -1;
        sp[3] = -1;
        sp[4] = gd->Width - 1;
        sp[5] = -1;

        sp[6] = gd->Width + 1;
        sp[7] = -2;
        sp[8] = gd->Width + 1;
        sp[9] = gd->Height + 1;
        sp[10] = -2;
        sp[11] = gd->Height + 1;

        sp[12] = -2;
        sp[13] = gd->Height;
        sp[14] = -2;
        sp[15] = -2;
        sp[16] = gd->Width;
        sp[17] = -2;
        sp[18] = gd->Width;
        sp[19] = gd->Height;
        sp[20] = -2;
        sp[21] = gd->Height;

        for (i = 0; i < 3; ++i) {
            bp[i].LeftEdge = bp[i].TopEdge = -1;
            bp[i].FrontPen = (i == 0 || i == 1) ? shadowpen : hipen;

            /* Have to use JAM2 so that the old colors disappear. */
            bp[i].BackPen = C_BLACK;
            bp[i].DrawMode = JAM2;
            bp[i].Count = (i == 0 || i == 1) ? 3 : 5;
            bp[i].XY = &sp[i * 6];
            bp[i].NextBorder = (i == 2) ? NULL : &bp[i + 1];
        }

        /* bp[0] and bp[1] two pieces for the up image */
        gd->GadgetRender = (APTR) bp;

        /* No image change for select */
        gd->SelectRender = (APTR) bp;

        gd->LeftEdge++;
        gd->TopEdge++;
        gd->Flags |= GADGHCOMP;
    } else {
        /* Create the border vector values for up and left side, and
         * also the lower and right side.
         */
        sp[0] = dec;
        sp[1] = gd->Height + inc;
        sp[2] = dec;
        sp[3] = dec;
        sp[4] = gd->Width + inc;
        sp[5] = dec;

        sp[6] = gd->Width + inc;
        sp[7] = dec;
        sp[8] = gd->Width + inc;
        sp[9] = gd->Height + inc;
        sp[10] = dec;
        sp[11] = gd->Height + inc;

        /* We are creating 4 sets of borders, the two sides of the
         * rectangle share the border vectors with the opposite image,
         * but specify different colors.
         */
        for (i = 0; i < 4; ++i) {
            bp[i].TopEdge = bp[i].LeftEdge = 0;

            /* A GADGHNONE is always down */
            if (gd->GadgetType == BOOLGADGET
                && (gd->Flags & GADGHIGHBITS) != GADGHNONE) {
                bp[i].FrontPen = (i == 1 || i == 2) ? shadowpen : hipen;
            } else {
                bp[i].FrontPen = (i == 1 || i == 3) ? hipen : shadowpen;
            }

            /* Have to use JAM2 so that the old colors disappear. */
            bp[i].BackPen = C_BLACK;
            bp[i].DrawMode = JAM2;
            bp[i].Count = 3;
            bp[i].XY = &sp[6 * ((i & 1) != 0)];
            bp[i].NextBorder = (i == 1 || i == 3) ? NULL : &bp[i + 1];
        }

        /* bp[0] and bp[1] two pieces for the up image */
        gd->GadgetRender = (APTR) bp;

        /* bp[2] and bp[3] two pieces for the down image */
        gd->SelectRender = (APTR)(bp + 2);
        gd->Flags |= GADGHIMAGE;
    }
}

/* Following function copied from wintty.c;
   Modified slightly to fit amiga needs */
void
amii_player_selection()
{
    int i, k, n;
    char pick4u = 'n', thisch, lastch = 0;
    char pbuf[QBUFSZ], plbuf[QBUFSZ], rolenamebuf[QBUFSZ];
    winid win;
    anything any;
    menu_item *selected = 0;

    rigid_role_checks();

    /* Should we randomly pick for the player? */
    if (flags.initrole == ROLE_NONE || flags.initrace == ROLE_NONE
        || flags.initgend == ROLE_NONE || flags.initalign == ROLE_NONE) {
        char *prompt;

        prompt = build_plselection_prompt(pbuf, QBUFSZ,
                                          flags.initrole, flags.initrace,
                                          flags.initgend, flags.initalign);
        pline("%s", prompt);
        do { /* loop until we get valid input */
            cursor_on(WIN_MESSAGE);
            pick4u = lowc(WindowGetchar());
            cursor_off(WIN_MESSAGE);
            if (strchr(quitchars, pick4u))
                pick4u = 'y';
        } while (!strchr(ynqchars, pick4u));
        pbuf[0] = pick4u;
        pbuf[1] = 0;
        amii_addtopl(pbuf);

        if (pick4u != 'y' && pick4u != 'n') {
        give_up: /* Quit */
            if (selected)
                free((genericptr_t) selected);
            clearlocks();
            exit_nhwindows(NULL);
            nh_terminate(0);
            /*NOTREACHED*/
            return;
        }
    }

    (void) root_plselection_prompt(plbuf, QBUFSZ - 1,
                                   flags.initrole, flags.initrace,
                                   flags.initgend, flags.initalign);

    /* Select a role, if necessary */
    /* we'll try to be compatible with pre-selected race/gender/alignment,
     * but may not succeed */
    if (flags.initrole < 0) {
        /* Process the choice */
        if (pick4u == 'y' || flags.initrole == ROLE_RANDOM
            || flags.randomall) {
            /* Pick a random role */
            flags.initrole = pick_role(flags.initrace, flags.initgend,
                                       flags.initalign, PICK_RANDOM);
            if (flags.initrole < 0) {
                amii_putstr(WIN_MESSAGE, 0, "Incompatible role!");
                flags.initrole = randrole(FALSE);
            }
        } else {
            /* Prompt for a role */
            win = create_nhwindow(NHW_MENU);
            start_menu(win, MENU_BEHAVE_STANDARD);
            any.a_void = 0; /* zero out all bits */
            for (i = 0; roles[i].name.m; i++) {
                if (ok_role(i, flags.initrace, flags.initgend,
                            flags.initalign)) {
                    any.a_int = i + 1; /* must be non-zero */
                    thisch = lowc(roles[i].name.m[0]);
                    if (thisch == lastch)
                        thisch = highc(thisch);
                    if (flags.initgend != ROLE_NONE
                        && flags.initgend != ROLE_RANDOM) {
                        if (flags.initgend == 1 && roles[i].name.f)
                            Strcpy(rolenamebuf, roles[i].name.f);
                        else
                            Strcpy(rolenamebuf, roles[i].name.m);
                    } else {
                        if (roles[i].name.f) {
                            Strcpy(rolenamebuf, roles[i].name.m);
                            Strcat(rolenamebuf, "/");
                            Strcat(rolenamebuf, roles[i].name.f);
                        } else
                            Strcpy(rolenamebuf, roles[i].name.m);
                    }
                    add_menu(win, NO_GLYPH, &any, thisch, 0, ATR_NONE,
                             an(rolenamebuf), MENU_ITEMFLAGS_NONE);
                    lastch = thisch;
                }
            }
            any.a_int = pick_role(flags.initrace, flags.initgend,
                                  flags.initalign, PICK_RANDOM) + 1;
            if (any.a_int == 0) /* must be non-zero */
                any.a_int = randrole(FALSE) + 1;
            add_menu(win, NO_GLYPH, &any, '*', 0, ATR_NONE, "Random",
                     MENU_ITEMFLAGS_NONE);
            any.a_int = i + 1; /* must be non-zero */
            add_menu(win, NO_GLYPH, &any, 'q', 0, ATR_NONE, "Quit",
                     MENU_ITEMFLAGS_NONE);
            Sprintf(pbuf, "Pick a role for your %s", plbuf);
            end_menu(win, pbuf);
            n = select_menu(win, PICK_ONE, &selected);
            destroy_nhwindow(win);

            /* Process the choice */
            if (n != 1 || selected[0].item.a_int == any.a_int)
                goto give_up; /* Selected quit */

            flags.initrole = selected[0].item.a_int - 1;
            free((genericptr_t) selected), selected = 0;
        }
        (void) root_plselection_prompt(plbuf, QBUFSZ - 1,
                                       flags.initrole, flags.initrace,
                                       flags.initgend, flags.initalign);
    }

    /* Select a race, if necessary */
    /* force compatibility with role, try for compatibility with
     * pre-selected gender/alignment */
    if (flags.initrace < 0
        || !validrace(flags.initrole, flags.initrace)) {
        /* pre-selected race not valid */
        if (pick4u == 'y' || flags.initrace == ROLE_RANDOM
            || flags.randomall) {
            flags.initrace = pick_race(flags.initrole, flags.initgend,
                                       flags.initalign, PICK_RANDOM);
            if (flags.initrace < 0) {
                amii_putstr(WIN_MESSAGE, 0, "Incompatible race!");
                flags.initrace = randrace(flags.initrole);
            }
        } else { /* pick4u == 'n' */
            /* Count the number of valid races */
            n = 0; /* number valid */
            k = 0; /* valid race */
            for (i = 0; races[i].noun; i++) {
                if (ok_race(flags.initrole, i, flags.initgend,
                            flags.initalign)) {
                    n++;
                    k = i;
                }
            }
            if (n == 0) {
                for (i = 0; races[i].noun; i++) {
                    if (validrace(flags.initrole, i)) {
                        n++;
                        k = i;
                    }
                }
            }

            /* Permit the user to pick, if there is more than one */
            if (n > 1) {
                win = create_nhwindow(NHW_MENU);
                start_menu(win, MENU_BEHAVE_STANDARD);
                any.a_void = 0; /* zero out all bits */
                for (i = 0; races[i].noun; i++)
                    if (ok_race(flags.initrole, i, flags.initgend,
                                flags.initalign)) {
                        any.a_int = i + 1; /* must be non-zero */
                        add_menu(win, NO_GLYPH, &any, races[i].noun[0], 0,
                                 ATR_NONE, races[i].noun,
                                 MENU_ITEMFLAGS_NONE);
                    }
                any.a_int = pick_race(flags.initrole, flags.initgend,
                                      flags.initalign, PICK_RANDOM) + 1;
                if (any.a_int == 0) /* must be non-zero */
                    any.a_int = randrace(flags.initrole) + 1;
                add_menu(win, NO_GLYPH, &any, '*', 0, ATR_NONE, "Random",
                         MENU_ITEMFLAGS_NONE);
                any.a_int = i + 1; /* must be non-zero */
                add_menu(win, NO_GLYPH, &any, 'q', 0, ATR_NONE, "Quit",
                         MENU_ITEMFLAGS_NONE);
                Sprintf(pbuf, "Pick the race of your %s", plbuf);
                end_menu(win, pbuf);
                n = select_menu(win, PICK_ONE, &selected);
                destroy_nhwindow(win);
                if (n != 1 || selected[0].item.a_int == any.a_int)
                    goto give_up; /* Selected quit */

                k = selected[0].item.a_int - 1;
                free((genericptr_t) selected), selected = 0;
            }
            flags.initrace = k;
        }
        (void) root_plselection_prompt(plbuf, QBUFSZ - 1,
                                       flags.initrole, flags.initrace,
                                       flags.initgend, flags.initalign);
    }

    /* Select a gender, if necessary */
    /* force compatibility with role/race, try for compatibility with
     * pre-selected alignment */
    if (flags.initgend < 0
        || !validgend(flags.initrole, flags.initrace, flags.initgend)) {
        /* pre-selected gender not valid */
        if (pick4u == 'y' || flags.initgend == ROLE_RANDOM
            || flags.randomall) {
            flags.initgend = pick_gend(flags.initrole, flags.initrace,
                                       flags.initalign, PICK_RANDOM);
            if (flags.initgend < 0) {
                amii_putstr(WIN_MESSAGE, 0, "Incompatible gender!");
                flags.initgend = randgend(flags.initrole, flags.initrace);
            }
        } else { /* pick4u == 'n' */
            /* Count the number of valid genders */
            n = 0; /* number valid */
            k = 0; /* valid gender */
            for (i = 0; i < ROLE_GENDERS; i++) {
                if (ok_gend(flags.initrole, flags.initrace, i,
                            flags.initalign)) {
                    n++;
                    k = i;
                }
            }
            if (n == 0) {
                for (i = 0; i < ROLE_GENDERS; i++) {
                    if (validgend(flags.initrole, flags.initrace, i)) {
                        n++;
                        k = i;
                    }
                }
            }

            /* Permit the user to pick, if there is more than one */
            if (n > 1) {
                win = create_nhwindow(NHW_MENU);
                start_menu(win, MENU_BEHAVE_STANDARD);
                any.a_void = 0; /* zero out all bits */
                for (i = 0; i < ROLE_GENDERS; i++)
                    if (ok_gend(flags.initrole, flags.initrace, i,
                                flags.initalign)) {
                        any.a_int = i + 1;
                        add_menu(win, NO_GLYPH, &any, genders[i].adj[0], 0,
                                 ATR_NONE, genders[i].adj, MENU_ITEMFLAGS_NONE);
                    }
                any.a_int = pick_gend(flags.initrole, flags.initrace,
                                      flags.initalign, PICK_RANDOM) + 1;
                if (any.a_int == 0) /* must be non-zero */
                    any.a_int = randgend(flags.initrole, flags.initrace) + 1;
                add_menu(win, NO_GLYPH, &any, '*', 0, ATR_NONE, "Random",
                         MENU_ITEMFLAGS_NONE);
                any.a_int = i + 1; /* must be non-zero */
                add_menu(win, NO_GLYPH, &any, 'q', 0, ATR_NONE, "Quit",
                         MENU_ITEMFLAGS_NONE);
                Sprintf(pbuf, "Pick the gender of your %s", plbuf);
                end_menu(win, pbuf);
                n = select_menu(win, PICK_ONE, &selected);
                destroy_nhwindow(win);
                if (n != 1 || selected[0].item.a_int == any.a_int)
                    goto give_up; /* Selected quit */

                k = selected[0].item.a_int - 1;
                free((genericptr_t) selected), selected = 0;
            }
            flags.initgend = k;
        }
        (void) root_plselection_prompt(plbuf, QBUFSZ - 1,
                                       flags.initrole, flags.initrace,
                                       flags.initgend, flags.initalign);
    }

    /* Select an alignment, if necessary */
    /* force compatibility with role/race/gender */
    if (flags.initalign < 0
        || !validalign(flags.initrole, flags.initrace, flags.initalign)) {
        /* pre-selected alignment not valid */
        if (pick4u == 'y' || flags.initalign == ROLE_RANDOM
            || flags.randomall) {
            flags.initalign = pick_align(flags.initrole, flags.initrace,
                                         flags.initgend, PICK_RANDOM);
            if (flags.initalign < 0) {
                amii_putstr(WIN_MESSAGE, 0, "Incompatible alignment!");
                flags.initalign = randalign(flags.initrole, flags.initrace);
            }
        } else { /* pick4u == 'n' */
            /* Count the number of valid alignments */
            n = 0; /* number valid */
            k = 0; /* valid alignment */
            for (i = 0; i < ROLE_ALIGNS; i++) {
                if (ok_align(flags.initrole, flags.initrace,
                             flags.initgend, i)) {
                    n++;
                    k = i;
                }
            }
            if (n == 0) {
                for (i = 0; i < ROLE_ALIGNS; i++) {
                    if (validalign(flags.initrole, flags.initrace, i)) {
                        n++;
                        k = i;
                    }
                }
            }

            /* Permit the user to pick, if there is more than one */
            if (n > 1) {
                win = create_nhwindow(NHW_MENU);
                start_menu(win, MENU_BEHAVE_STANDARD);
                any.a_void = 0; /* zero out all bits */
                for (i = 0; i < ROLE_ALIGNS; i++)
                    if (ok_align(flags.initrole, flags.initrace,
                                 flags.initgend, i)) {
                        any.a_int = i + 1;
                        add_menu(win, NO_GLYPH, &any, aligns[i].adj[0], 0,
                                 ATR_NONE, aligns[i].adj, MENU_ITEMFLAGS_NONE);
                    }
                any.a_int = pick_align(flags.initrole, flags.initrace,
                                       flags.initgend, PICK_RANDOM) + 1;
                if (any.a_int == 0) /* must be non-zero */
                    any.a_int = randalign(flags.initrole, flags.initrace) + 1;
                add_menu(win, NO_GLYPH, &any, '*', 0, ATR_NONE, "Random",
                         MENU_ITEMFLAGS_NONE);
                any.a_int = i + 1; /* must be non-zero */
                add_menu(win, NO_GLYPH, &any, 'q', 0, ATR_NONE, "Quit",
                         MENU_ITEMFLAGS_NONE);
                Sprintf(pbuf, "Pick the alignment of your %s", plbuf);
                end_menu(win, pbuf);
                n = select_menu(win, PICK_ONE, &selected);
                destroy_nhwindow(win);
                if (n != 1 || selected[0].item.a_int == any.a_int)
                    goto give_up; /* Selected quit */

                k = selected[0].item.a_int - 1;
                free((genericptr_t) selected), selected = 0;
            }
            flags.initalign = k;
        }
    }
    /* Success! */
}
#endif /* AMIGA_INTUITION */
