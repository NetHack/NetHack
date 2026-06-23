/* NetHack 5.0	mactopl.c	$NHDT-Date: 1432512797 2015/05/25 00:13:17 $  $NHDT-Branch: master $:$NHDT-Revision: 1.9 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Kenneth Lorber, Kensington, Maryland, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "mactty.h"
#include "macwin.h"

static char
queued_resp(char *resp)
{
    char buf[QUEUE_LEN + 1]; /* try_key_queue's required minimum */
    if (try_key_queue(buf)) {
        if (!resp || strchr(resp, buf[0]))
            return buf[0];
        if (digit(buf[0]) && strchr(resp, '#')) {
            yn_number = atoi(buf);
            return '#';
        }
    }
    return '\0';
}

static char
topl_yn_function(const char *query, const char *resp, char def)
{
    char buf[BUFSZ]; /* leave_topl_mode can write up to BUFSZ-1 chars,
                        e.g. digits typed at a '#' numeric prompt */
    char c = queued_resp((char *) resp);
    if (!c) {
        enter_topl_mode((char *) query);
        /* A NULL resp means "any key" (getobj/getlin-style prompts validate
           the key themselves); don't draw a yn-button strip for those -- only
           genuine yes/no-style prompts (non-NULL resp) get buttons. */
        topl_set_resp(resp ? (char *) resp : "", def);

        do {
            c = readchar();
            if (c && resp && !strchr(resp, c)) {
                nhbell();
                c = '\0';
            }
        } while (!c);

        topl_set_resp("", '\0');
        leave_topl_mode(buf);
        if (c == '#')
            yn_number = atoi(buf);
    }
    return c;
}

/* Generic yes/no prompt; if resp is NULL, any single character is accepted. */
char
mac_yn_function(const char *query, const char *resp, char def)
{
    return topl_yn_function(query, resp, def);
}

/* mactopl.c */
