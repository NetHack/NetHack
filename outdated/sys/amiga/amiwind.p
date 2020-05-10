/* NetHack 3.6	amiwind.p	$NHDT-Date: 1432512795 2015/05/25 00:13:15 $  $NHDT-Branch: master $:$NHDT-Revision: 1.6 $ */
/*   Copyright (c) Gregg Wonderly, Naperville, IL, 1992, 1993	  */
/* NetHack may be freely redistributed.  See license for details. */

/* amiwind.c */
#ifdef	INTUI_NEW_LOOK
struct Window *FDECL( OpenShWindow, (struct ExtNewWindow *) );
#else
struct Window *FDECL( OpenShWindow, (struct NewWindow *) );
#endif
void FDECL( CloseShWindow, (struct Window *));
int NDECL( kbhit );
int NDECL( amikbhit );
int NDECL( WindowGetchar );
WETYPE NDECL( WindowGetevent );
void NDECL( WindowFlush );
void FDECL( WindowPutchar, (char ));
void FDECL( WindowFPuts, (const char *));
void FDECL( WindowPuts, (const char *));
void FDECL( WindowPrintf, ( char *,... ));
void NDECL( CleanUp );
int FDECL( ConvertKey, ( struct IntuiMessage * ));
#ifndef	SHAREDLIB
void FDECL( Abort, (long ));
#endif
void FDECL( flush_glyph_buffer, (struct Window *));
void FDECL( amiga_print_glyph, (winid , int , int ));
void FDECL( start_glyphout, (winid ));
void FDECL( amii_end_glyphout, (winid ));
#ifdef	INTUI_NEW_LOOK
struct ExtNewWindow *FDECL( DupNewWindow, (struct ExtNewWindow *));
void FDECL( FreeNewWindow, (struct ExtNewWindow *));
#else
struct NewWindow *FDECL( DupNewWindow, (struct NewWindow *));
void FDECL( FreeNewWindow, (struct NewWindow *));
#endif
void NDECL( bell );
void NDECL( amii_delay_output );
void FDECL( amii_number_pad, (int ));
void amii_cleanup( void );
