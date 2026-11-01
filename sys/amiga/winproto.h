/* NetHack 3.6	winproto.h	$NHDT-Date: 1433806597 2015/06/08 23:36:37 $  $NHDT-Branch: master $:$NHDT-Revision: 1.9 $ */
/* Copyright (c) Gregg Wonderly, Naperville, Illinois,  1991,1992,1993. */
/* NetHack may be freely redistributed.  See license for details. */

/* winreq.c */
void EditColor(void);
void EditClipping(void);
void DrawCol(struct Window *w, int idx, UWORD *colors);
void DispCol(struct Window *w, int idx, UWORD *colors);
void amii_change_color(int, long, int);
char *amii_get_color_string(void);
void amii_getlin(const char *prompt, char *bufp);
void getlind(const char *prompt, char *bufp, const char *dflt);
int filecopy(char *from, char *to);
char *basename(char *str);
char *dirname(char *str);

/* winstr.c */
void amii_putstr(winid window, int attr, const char *str);
void outmore(struct amii_WinDesc *cw);
void outsubstr(struct amii_WinDesc *cw, char *str, int len, int fudge);
void amii_putsym(winid st, int i, int y, CHAR_P c);
void amii_addtopl(const char *s);
void TextSpaces(struct RastPort *rp, int nr);
void amii_remember_topl(void);
long CountLines(winid);
long FindLine(winid, int);
int amii_doprev_message(void);
void flushIDCMP(struct MsgPort *);
int amii_msgborder(struct Window *);
void amii_scrollmsg(register struct Window *w,
                    register struct amii_WinDesc *cw);

/* winkey.c */
int amii_nh_poskey(coordxy *x, coordxy *y, int *mod);
int amii_nhgetch(void);
void amii_get_nh_event(void);
void amii_getret(void);

/* winmenu.c */
void amii_start_menu(winid window, unsigned long);
void amii_add_menu(winid, const glyph_info *, const anything *, CHAR_P, CHAR_P,
                   int, int, const char *, unsigned int);
void amii_end_menu(winid, const char *);
int amii_select_menu(winid, int, menu_item **);
int DoMenuScroll(int win, int blocking, int how, menu_item **);
void ReDisplayData(winid win);
void DisplayData(winid win, int start);
void SetPropInfo(struct Window *win, struct Gadget *gad, long vis, long total,
                 long top);

/* amiwind.c */
struct Window *OpenShWindow(struct NewWindow *nw);
void CloseShWindow(struct Window *win);
int ConvertKey(struct IntuiMessage *message);
int kbhit(void);
int amikbhit(void);
int WindowGetchar(void);
WETYPE WindowGetevent(void);
void amii_cleanup(void);
#ifndef SHAREDLIB
void Abort(long rc);
#endif
void CleanUp(void);
void flush_glyph_buffer(struct Window *w);
void amiga_print_glyph(winid window, int color_index, int glyph);
void start_glyphout(winid window);
void amii_end_glyphout(winid window);
struct NewWindow *DupNewWindow(struct NewWindow *win);
void FreeNewWindow(struct NewWindow *win);
void bell(void);
void amii_delay_output(void);
void amii_number_pad(int state);
#ifndef SHAREDLIB
void amiv_loadlib(void);
void amii_loadlib(void);
#endif
void preserve_icon(void);
void clear_icon(void);

/* winfuncs.c */
void amii_destroy_nhwindow(winid win);
int amii_create_nhwindow(int type);
void amii_init_nhwindows(int *, char **);
void amii_setdrawpens(struct Window *, int type);
void amii_sethipens(struct Window *, int type, int attr);
void amii_setfillpens(struct Window *, int type);
void amii_clear_nhwindow(winid win);
void dismiss_nhwindow(winid win);
void amii_exit_nhwindows(const char *str);
void amii_display_nhwindow(winid win, boolean blocking);
void amii_curs(winid window, int x, int y);
void kill_nhwindows(int all);
void amii_cl_end(struct amii_WinDesc *cw, int i);
void cursor_off(winid window);
void cursor_on(winid window);
void amii_suspend_nhwindows(const char *str);
void amii_resume_nhwindows(void);
void amii_bell(void);
void removetopl(int cnt);
void port_help(void);
void amii_print_glyph(winid win, coordxy x, coordxy y, const glyph_info *glyphinfo, const glyph_info *bkglyphinfo);
void amii_raw_print(const char *s);
void amii_raw_print_bold(const char *s);
void amii_update_inventory(int);
void amii_mark_synch(void);
void amii_wait_synch(void);
void amii_setclipped(void);
void amii_cliparound(int x, int y);
void amii_set_text_font(char *font, int size);
BitMapHeader ReadImageFile(const char *, struct BitMap **);
void FreeImageFile(struct BitMap **);
BitMapHeader ReadTileImageFiles(void);
void FreeTileImageFiles(void);

/* winami.c */
#ifdef SHAREDLIB
int __UserLibInit(void);
void __UserLibCleanup(void);
#endif
void amii_askname(void);
void amii_player_selection(void);
void RandomWindow(char *name);
int amii_get_ext_cmd(void);
char amii_yn_function(const char *query, const char *resp, char def);
void amii_display_file(const char *fn, boolean complain);
void SetBorder(struct Gadget *gd);
/* malloc/free provided by stdlib.h */

#ifdef SHAREDLIB
/* amilib.c */
void amii_loadlib(void);
void amiv_loadlib(void);
void CleanUp(void);
void setup_librefs(WinamiBASE *base);
#else
void Abort(long rc);
#endif

win_request_info *amii_ctrl_nhwindow(winid, int, win_request_info *);

/* amirip.c */
void amii_outrip(winid tmpwin, int how, time_t when);

/* winchar.c */
void SetMazeType(MazeType);
int GlyphToIcon(int glyph);
#ifdef OPT_DISPMAP
void dispmap_sanity(void);
int dispmap_sanity1(int);
#endif
