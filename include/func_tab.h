/* NetHack 3.6	func_tab.h	$NHDT-Date: 1432512775 2015/05/25 00:12:55 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef FUNC_TAB_H
#define FUNC_TAB_H

struct func_tab {
    char f_char;
    boolean can_if_buried;
    int NDECL((*f_funct));
    const char *f_text;
};

struct ext_func_tab {
    const char *ef_txt, *ef_desc;
    int NDECL((*ef_funct));
    boolean can_if_buried;
};

extern struct ext_func_tab extcmdlist[];

#endif /* FUNC_TAB_H */
