/* NetHack 3.6  wintype.h       $NHDT-Date: 1433207914 2015/06/02 01:18:34 $  $NHDT-Branch: master $:$NHDT-Revision: 1.15 $ */
/* Copyright (c) David Cohrs, 1991                                */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef WINTYPE_H
#define WINTYPE_H

typedef int winid; /* a window identifier */

/* generic parameter - must not be any larger than a pointer */
typedef union any {
    genericptr_t a_void;
    struct obj *a_obj;
    struct monst *a_monst;
    int a_int;
    char a_char;
    schar a_schar;
    unsigned int a_uint;
    long a_long;
    unsigned long a_ulong;
    int *a_iptr;
    long *a_lptr;
    unsigned long *a_ulptr;
    unsigned *a_uptr;
    const char *a_string;
    /* add types as needed */
} anything;
#define ANY_P union any /* avoid typedef in prototypes */
                        /* (buggy old Ultrix compiler) */

/* symbolic names for the data types housed in anything */
/* clang-format off */
#define ANY_VOID         1
#define ANY_OBJ          2      /* struct obj */
#define ANY_MONST        3      /* struct monst (not used) */
#define ANY_INT          4      /* int */
#define ANY_CHAR         5      /* char */
#define ANY_UCHAR        6      /* unsigned char */
#define ANY_SCHAR        7      /* signed char */
#define ANY_UINT         8      /* unsigned int */
#define ANY_LONG         9      /* long */
#define ANY_ULONG       10      /* unsigned long */
#define ANY_IPTR        11      /* pointer to int */
#define ANY_UPTR        12      /* pointer to unsigned int */
#define ANY_LPTR        13      /* pointer to long */
#define ANY_ULPTR       14      /* pointer to unsigned long */
#define ANY_STR         15      /* pointer to null-terminated char string */
#define ANY_MASK32      16      /* 32-bit mask (stored as unsigned long) */
/* clang-format on */

/* menu return list */
typedef struct mi {
    anything item; /* identifier */
    long count;    /* count */
} menu_item;
#define MENU_ITEM_P struct mi

/* select_menu() "how" argument types */
#define PICK_NONE 0 /* user picks nothing (display only) */
#define PICK_ONE 1  /* only pick one */
#define PICK_ANY 2  /* can pick any amount */

/* window types */
/* any additional port specific types should be defined in win*.h */
#define NHW_MESSAGE 1
#define NHW_STATUS 2
#define NHW_MAP 3
#define NHW_MENU 4
#define NHW_TEXT 5

/* attribute types for putstr; the same as the ANSI value, for convenience */
#define ATR_NONE 0
#define ATR_BOLD 1
#define ATR_DIM 2
#define ATR_ULINE 4
#define ATR_BLINK 5
#define ATR_INVERSE 7

/* nh_poskey() modifier types */
#define CLICK_1 1
#define CLICK_2 2

/* invalid winid */
#define WIN_ERR ((winid) -1)

/* menu window keyboard commands (may be mapped) */
/* clang-format off */
#define MENU_FIRST_PAGE         '^'
#define MENU_LAST_PAGE          '|'
#define MENU_NEXT_PAGE          '>'
#define MENU_PREVIOUS_PAGE      '<'
#define MENU_SELECT_ALL         '.'
#define MENU_UNSELECT_ALL       '-'
#define MENU_INVERT_ALL         '@'
#define MENU_SELECT_PAGE        ','
#define MENU_UNSELECT_PAGE      '\\'
#define MENU_INVERT_PAGE        '~'
#define MENU_SEARCH             ':'
/* clang-format on */

#endif /* WINTYPE_H */
