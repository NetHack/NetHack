/* NetHack 3.7	mail.h	$NHDT-Date: 1596498544 2020/08/03 23:49:04 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.10 $ */
/*      Copyright (c) 2015 by Kenneth Lorber              */
/* NetHack may be freely redistributed.  See license for details. */

/* used by ckmailstatus() to pass information to the mail-daemon in newmail()
 */

#ifndef MAIL_H
#define MAIL_H

#define MSG_OTHER 0 /* catch-all; none of the below... */
#define MSG_MAIL 1  /* unimportant, uninteresting mail message */
#define MSG_CALL 2  /* annoying phone/talk/chat-type interruption */

struct mail_info {
    int message_typ;          /* MSG_foo value */
    const char *display_txt;  /* text for daemon to verbalize */
    const char *object_nam;   /* text to tag object with */
    const char *response_cmd; /* command to eventually execute */
};

#endif /* MAIL_H */
