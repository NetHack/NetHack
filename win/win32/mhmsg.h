/* NetHack 3.7	mhmsg.h	$NHDT-Date: 1596498356 2020/08/03 23:45:56 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.21 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MHNethackMessages_H
#define MHNethackMessages_H

/* nethack messages */
#define WM_MSNH_COMMAND (WM_APP + 1)

#define MSNH_MSG_ADDWND 100
#define MSNH_MSG_PUTSTR 101
#define MSNH_MSG_PRINT_GLYPH 102
#define MSNH_MSG_CLEAR_WINDOW 103
#define MSNH_MSG_CLIPAROUND 104
#define MSNH_MSG_STARTMENU 105
#define MSNH_MSG_ADDMENU 106
#define MSNH_MSG_CURSOR 107
#define MSNH_MSG_ENDMENU 108
#define MSNH_MSG_DIED 109
#define MSNH_MSG_CARET 110
#define MSNH_MSG_GETTEXT 111
#define MSNH_MSG_UPDATE_STATUS 112
#define MSNH_MSG_RANDOM_INPUT 113

typedef struct mswin_nhmsg_add_wnd {
    winid wid;
} MSNHMsgAddWnd, *PMSNHMsgAddWnd;

typedef struct mswin_nhmsg_putstr {
    int attr;
    const char *text;
    int append;
} MSNHMsgPutstr, *PMSNHMsgPutstr;

typedef struct mswin_nhmsg_print_glyph {
    coordxy x;
    coordxy y;
    glyph_info glyphinfo;
    glyph_info bkglyphinfo;
} MSNHMsgPrintGlyph, *PMSNHMsgPrintGlyph;

typedef struct mswin_nhmsg_cliparound {
    int x;
    int y;
} MSNHMsgClipAround, *PMSNHMsgClipAround;

typedef struct mswin_nhmsg_add_menu {
    glyph_info glyphinfo;
    const ANY_P *identifier;
    char accelerator;
    char group_accel;
    int attr;
    int clr;
    const char *str;
    boolean presel;
    unsigned int itemflags;
} MSNHMsgAddMenu, *PMSNHMsgAddMenu;

typedef struct mswin_nhmsg_cursor {
    int x;
    int y;
} MSNHMsgCursor, *PMSNHMsgCursor;

typedef struct mswin_nhmsg_end_menu {
    const char *text;
} MSNHMsgEndMenu, *PMSNHMsgEndMenu;

typedef struct mswin_nhmsg_get_text {
    size_t max_size;
    char buffer[TEXT_BUFFER_SIZE];
} MSNHMsgGetText, *PMSNHMsgGetText;

typedef struct mswin_nhmsg_update_status {
    struct mswin_status_lines * status_lines;
} MSNHMsgUpdateStatus, *PMSNHMsgUpdateStatus;

#endif
