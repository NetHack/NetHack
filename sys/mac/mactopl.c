/* NetHack 3.6	mactopl.c	$NHDT-Date: 1431192785 2015/05/09 17:33:05 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/* NetHack 3.6	mactopl.c	$Date: 2009/05/06 10:49:14 $  $Revision: 1.5 $ */
/*	SCCS Id: @(#)mactopl.c	3.5	1991/07/23 */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "mactty.h"
#include "macwin.h"
#include "macpopup.h"

char
queued_resp(char *resp)
{
    char buf[30];
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

char
topl_yn_function(const char *query, const char *resp, char def)
{
    char buf[30];
    char c = queued_resp((char *) resp);
    if (!c) {
        enter_topl_mode((char *) query);
        topl_set_resp((char *) resp, def);

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

char
mac_yn_function(query, resp, def)
const char *query, *resp;
char def;
/*
 *   Generic yes/no function. 'def' is the default (returned by space or
 *   return; 'esc' returns 'q', or 'n', or the default, depending on
 *   what's in the string. The 'query' string is printed before the user
 *   is asked about the string.
 *   If resp is NULL, any single character is accepted and returned.
 */
{
    return topl_yn_function(query, resp, def);
}

/* mactopl.c */
