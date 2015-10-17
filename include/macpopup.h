/* NetHack 3.6	macpopup.h	$NHDT-Date: 1432512781 2015/05/25 00:13:01 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $ */
/* Copyright (c) Nethack Development Team, 1999.		*/
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MACPOPUP_H
#define MACPOPUP_H

/* ### mmodal.c ### */

extern void FlashButton(DialogRef, short);
extern char queued_resp(char *resp);
extern char topl_yn_function(const char *query, const char *resp, char def);
extern int get_line_from_key_queue(char *bufp);

#endif /* MACPOPUP_H */
