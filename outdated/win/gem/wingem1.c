/* NetHack 3.6	wingem1.c	$NHDT-Date: 1433806613 2015/06/08 23:36:53 $  $NHDT-Branch: master $:$NHDT-Revision: 1.13 $ */
/* Copyright (c) Christian Bressler 1999 	  */
/* NetHack may be freely redistributed.  See license for details. */

#define __TCC_COMPAT__

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <e_gem.h>
#include <string.h>

#include "gem_rsc.h"
#include "load_img.h"
#include "gr_rect.h"

#define genericptr_t void *
typedef signed char schar;
#include "wintype.h"
#undef genericptr_t

#define NDECL(f) f(void)
#define FDECL(f, p) f p
#define CHAR_P char
#define SCHAR_P schar
#define UCHAR_P uchar
#define XCHAR_P xchar
#define SHORT_P short
#define BOOLEAN_P boolean
#define ALIGNTYP_P aligntyp
typedef signed char xchar;
#include "wingem.h"
#undef CHAR_P
#undef SCHAR_P
#undef UCHAR_P
#undef XCHAR_P
#undef SHORT_P
#undef BOOLEAN_P
#undef ALIGNTYP_P
#undef NDECL
#undef FDECL

static char nullstr[] = "", md[] = "NetHack 3.6.0", strCancel[] = "Cancel",
            strOk[] = "Ok", strText[] = "Text";

extern winid WIN_MESSAGE, WIN_MAP, WIN_STATUS, WIN_INVEN;

#define MAXWIN 20
#define ROWNO 21
#define COLNO 80
#define MSGLEN 100

#define MAP_GADGETS                                                        \
    NAME | MOVER | CLOSER | FULLER | LFARROW | RTARROW | UPARROW | DNARROW \
        | VSLIDE | HSLIDE | SIZER | SMALLER
#define DIALOG_MODE AUTO_DIAL | MODAL | NO_ICONIFY

/*
 *  Keyboard translation tables.
 */
#define C(c) (0x1f & (c))
#define M(c) (0x80 | (c))

#define KEYPADLO 0x61
#define KEYPADHI 0x71

#define PADKEYS (KEYPADHI - KEYPADLO + 1)
#define iskeypad(x) (KEYPADLO <= (x) && (x) <= KEYPADHI)

/*
 * Keypad keys are translated to the normal values below.
 * When iflags.BIOS is active, shifted keypad keys are translated to the
 *    shift values below.
 */
static const struct pad {
    char normal, shift, cntrl;
} keypad[PADKEYS] =
    {
      { C('['), 'Q', C('[') }, /* UNDO */
      { '?', '/', '?' },       /* HELP */
      { '(', 'a', '(' },       /* ( */
      { ')', 'w', ')' },       /* ) */
      { '/', '/', '/' },       /* / */
      { C('p'), '$', C('p') }, /* * */
      { 'y', 'Y', C('y') },    /* 7 */
      { 'k', 'K', C('k') },    /* 8 */
      { 'u', 'U', C('u') },    /* 9 */
      { 'h', 'H', C('h') },    /* 4 */
      { '.', '.', '.' },
      { 'l', 'L', C('l') }, /* 6 */
      { 'b', 'B', C('b') }, /* 1 */
      { 'j', 'J', C('j') }, /* 2 */
      { 'n', 'N', C('n') }, /* 3 */
      { 'i', 'I', C('i') }, /* Ins */
      { '.', ':', ':' }     /* Del */
    },
  numpad[PADKEYS] = {
      { C('['), 'Q', C('[') }, /* UNDO */
      { '?', '/', '?' },       /* HELP */
      { '(', 'a', '(' },       /* ( */
      { ')', 'w', ')' },       /* ) */
      { '/', '/', '/' },       /* / */
      { C('p'), '$', C('p') }, /* * */
      { '7', M('7'), '7' },    /* 7 */
      { '8', M('8'), '8' },    /* 8 */
      { '9', M('9'), '9' },    /* 9 */
      { '4', M('4'), '4' },    /* 4 */
      { '.', '.', '.' },       /* 5 */
      { '6', M('6'), '6' },    /* 6 */
      { '1', M('1'), '1' },    /* 1 */
      { '2', M('2'), '2' },    /* 2 */
      { '3', M('3'), '3' },    /* 3 */
      { 'i', 'I', C('i') },    /* Ins */
      { '.', ':', ':' }        /* Del */
  };

#define TBUFSZ 300
#define BUFSZ 256
extern int yn_number;                          /* from decl.c */
extern char mapped_menu_cmds[];                /* from options.c */
extern int mar_iflags_numpad(void);            /* from wingem.c */
extern void Gem_raw_print(const char *);       /* from wingem.c */
extern int mar_hp_query(void);                 /* from wingem.c */
extern int mar_get_msg_history(void);          /* from wingem.c */
extern int mar_get_msg_visible(void);          /* from wingem.c */
extern void mar_get_font(int, char **, int *); /* from wingem.c */
extern int vdi2dev4[];                         /* from load_img.c */

void recalc_msg_win(GRECT *);
void recalc_status_win(GRECT *);
void calc_std_winplace(int, GRECT *);
int (*v_mtext)(int, int, int, char *);
static int no_glyph; /* the int indicating there is no glyph */
IMG_header tile_image, titel_image, rip_image;
MFDB Tile_bilder, Map_bild, Titel_bild, Rip_bild, Black_bild, Pet_Mark,
    FontCol_Bild;
static int Tile_width = 16, Tile_heigth = 16, Tiles_per_line = 20;
char *Tilefile = NULL;
/* pet_mark Design by Warwick Allison warwick@troll.no */
static int pet_mark_data[] = { 0x0000, 0x3600, 0x7F00, 0x7F00,
                               0x3E00, 0x1C00, 0x0800 };
static short *normal_palette = NULL;

static struct gw {
    WIN *gw_window;
    int gw_type, gw_dirty;
    GRECT gw_place;
} Gem_nhwindow[MAXWIN];

typedef struct {
    int id;
    int size;
    int cw, ch;
    int prop;
} NHGEM_FONT;

/*struct gemmapdata {*/
GRECT dirty_map_area = { COLNO - 1, ROWNO, 0, 0 };
int map_cursx = 0, map_cursy = 0, curs_col = WHITE;
int draw_cursor = TRUE, scroll_margin = -1;
NHGEM_FONT map_font;
SCROLL scroll_map;
char **map_glyphs = NULL;
dirty_rect *dr_map;
/*};*/

/*struct gemstatusdata{*/
char **status_line;
int Anz_status_lines, status_w, status_align = FALSE;
NHGEM_FONT status_font;
dirty_rect *dr_stat;
/*};*/

/*struct gemmessagedata{*/
int mar_message_pause = TRUE;
int mar_esc_pressed = FALSE;
int messages_pro_zug = 0;
char **message_line;
int *message_age;
int msg_pos = 0, msg_max = 0, msg_anz = 0, msg_width = 0, msg_vis = 3,
    msg_align = TRUE;
NHGEM_FONT msg_font;
dirty_rect *dr_msg;
/*};*/

/*struct geminvdata {*/
SCROLL scroll_menu;
Gem_menu_item *invent_list;
int Anz_inv_lines = 0, Inv_breite = 16;
NHGEM_FONT menu_font;
int Inv_how;
/*};*/

/*struct gemtextdata{*/
char **text_lines;
int Anz_text_lines = 0, text_width;
NHGEM_FONT text_font;
int use_rip = FALSE;
extern char **rip_line;
/*};*/

static OBJECT *zz_oblist[NHICON + 1];

MITEM scroll_keys[] = {
    /* menu, key, state, mode, msg */
    { FAIL, key(CTRLLEFT, 0), K_CTRL, PAGE_LEFT, FAIL },
    { FAIL, key(CTRLRIGHT, 0), K_CTRL, PAGE_RIGHT, FAIL },
    { FAIL, key(SCANUP, 0), K_SHIFT, PAGE_UP, FAIL },
    { FAIL, key(SCANDOWN, 0), K_SHIFT, PAGE_DOWN, FAIL },
    { FAIL, key(SCANLEFT, 0), 0, LINE_LEFT, FAIL },
    { FAIL, key(SCANRIGHT, 0), 0, LINE_RIGHT, FAIL },
    { FAIL, key(SCANUP, 0), 0, LINE_UP, FAIL },
    { FAIL, key(SCANDOWN, 0), 0, LINE_DOWN, FAIL },
    { FAIL, key(SCANLEFT, 0), K_SHIFT, LINE_START, FAIL },
    { FAIL, key(SCANRIGHT, 0), K_SHIFT, LINE_END, FAIL },
    { FAIL, key(SCANUP, 0), K_CTRL, WIN_START, FAIL },
    { FAIL, key(SCANDOWN, 0), K_CTRL, WIN_END, FAIL },
    { FAIL, key(SCANHOME, 0), K_SHIFT, WIN_END, FAIL },
    { FAIL, key(SCANHOME, 0), 0, WIN_START, FAIL }
};
#define SCROLL_KEYS 14

static DIAINFO *Inv_dialog;

#define null_free(ptr) free(ptr), (ptr) = NULL
#define test_free(ptr) \
    if (ptr)           \
    null_free(ptr)

static char *Menu_title = NULL;

void mar_display_nhwindow(winid);
void
mar_check_hilight_status(void)
{
} /* to be filled :-) */
static char *mar_copy_of(const char *);

extern void panic(const char *, ...);
void *
m_alloc(size_t amt)
{
    void *ptr;

    ptr = malloc(amt);
    if (!ptr)
        panic("Memory allocation failure; cannot get %lu bytes", amt);
    return (ptr);
}

void
mar_clear_messagewin(void)
{
    int i, *ptr = message_age;

    if (WIN_MESSAGE == WIN_ERR)
        return;
    for (i = msg_anz; --i >= 0; ptr++) {
        if (*ptr)
            Gem_nhwindow[WIN_MESSAGE].gw_dirty = TRUE;
        *ptr = FALSE;
    }
    mar_message_pause = FALSE;

    mar_display_nhwindow(WIN_MESSAGE);
}

void
clipbrd_save(void *data, int cnt, boolean append, boolean is_inv)
{
    char path[MAX_PATH], *text, *crlf = "\r\n";
    long handle;
    int i;

    if (data && cnt > 0 && scrp_path(path, "scrap.txt")
        && (handle = append ? Fopen(path, 1) : Fcreate(path, 0)) > 0) {
        if (append)
            Fseek(0L, (int) handle, SEEK_END);
        if (is_inv) {
            Gem_menu_item *it = (Gem_menu_item *) data;

            for (; it; it = it->Gmi_next) {
                text = it->Gmi_str;
                Fwrite((int) handle, strlen(text), text);
                Fwrite((int) handle, 2L, crlf);
            }
        } else {
            for (i = 0; i < cnt; i++) {
                text = ((char **) data)[i] + 1;
                Fwrite((int) handle, strlen(text), text);
                Fwrite((int) handle, 2L, crlf);
            }
        }
        Fclose((int) handle);

        scrp_changed(SCF_TEXT, 0x2e545854l); /* .TXT */
    }
}

void
move_win(WIN *z_win)
{
    GRECT frame = desk;

    v_set_mode(MD_XOR);
    v_set_line(BLACK, 1, 1, 0, 0);
    frame.g_w <<= 1, frame.g_h <<= 1;
    if (graf_rt_dragbox(FALSE, &z_win->curr, &frame, &z_win->curr.g_x,
                        &z_win->curr.g_y, NULL))
        window_size(z_win, &z_win->curr);
    else
        window_top(z_win);
}

void
message_handler(int x, int y)
{
    switch (objc_find(zz_oblist[MSGWIN], ROOT, MAX_DEPTH, x, y)) {
    case UPMSG:
        if (msg_pos > msg_vis - 1) {
            msg_pos--;
            Gem_nhwindow[WIN_MESSAGE].gw_dirty = TRUE;
            mar_display_nhwindow(WIN_MESSAGE);
        }
        Event_Timer(50, 0, TRUE);
        break;
    case DNMSG:
        if (msg_pos < msg_max) {
            msg_pos++;
            Gem_nhwindow[WIN_MESSAGE].gw_dirty = TRUE;
            mar_display_nhwindow(WIN_MESSAGE);
        }
        Event_Timer(50, 0, TRUE);
        break;
    case GRABMSGWIN:
    default:
        move_win(Gem_nhwindow[WIN_MESSAGE].gw_window);
        break;
    case -1:
        break;
    }
}

int
mar_ob_mapcenter(OBJECT *p_obj)
{
    WIN *p_w = WIN_MAP != WIN_ERR ? Gem_nhwindow[WIN_MAP].gw_window : NULL;

    if (p_obj && p_w) {
        p_obj->ob_x = p_w->work.g_x + p_w->work.g_w / 2 - p_obj->ob_width / 2;
        p_obj->ob_y =
            p_w->work.g_y + p_w->work.g_h / 2 - p_obj->ob_height / 2;
        return (DIA_LASTPOS);
    }
    return (DIA_CENTERED);
}

/****************************** set_no_glyph
 * *************************************/

void
mar_set_no_glyph(ng)
int ng;
{
    no_glyph = ng;
}

void
mar_set_tilefile(name)
char *name;
{
    Tilefile = name;
}
void
mar_set_tilex(value)
int value;
{
    Min(&value, 32);
    Max(&value, 1);
    Tile_width = value;
}
void
mar_set_tiley(value)
int value;
{
    Min(&value, 32);
    Max(&value, 1);
    Tile_heigth = value;
}
/****************************** userdef_draw
 * *************************************/

void rearrange_windows(void);
void
mar_set_status_align(int sa)
{
    if (status_align != sa) {
        status_align = sa;
        rearrange_windows();
    }
}
void
mar_set_msg_align(int ma)
{
    if (msg_align != ma) {
        msg_align = ma;
        rearrange_windows();
    }
}
void
mar_set_msg_visible(int mv)
{
    if (mv != msg_vis) {
        Max(&mv, 1);
        Min(&mv, min(msg_anz, 20));
        Min(&mv, desk.g_h / msg_font.ch / 2);
        msg_vis = mv;
        rearrange_windows();
    }
}
/* size<0 cellheight; size>0 points */
void
mar_set_fontbyid(int type, int id, int size)
{
    int chardim[4];
    if (id <= 0)
        id = ibm_font_id;
    if ((size > -3 && size < 3) || size < -20 || size > 20)
        size = -ibm_font;
    /* MAR -- 17.Mar 2002 For now allow FNT_PROP only with NHW_TEXT */
    if (type != NHW_TEXT && (FontInfo(id)->type & (FNT_PROP | FNT_ASCII)))
        id = ibm_font_id;
    switch (type) {
    case NHW_MESSAGE:
        if (msg_font.size == -size && msg_font.id == id)
            break;
        msg_font.size = -size;
        msg_font.id = id;
        msg_font.prop = FontInfo(id)->type & (FNT_PROP | FNT_ASCII);
        v_set_text(msg_font.id, msg_font.size, BLACK, 0, 0, chardim);
        msg_font.ch = chardim[3] ? chardim[3] : 1;
        msg_font.cw = chardim[2] ? chardim[2] : 1;
        msg_width = min(max_w / msg_font.cw - 3, MSGLEN);
        rearrange_windows();
        break;
    case NHW_MAP:
        if (map_font.size != -size || map_font.id != id) {
            MFDB mtmp;
            map_font.size = -size;
            map_font.id = id;
            map_font.prop = FontInfo(id)->type & (FNT_PROP | FNT_ASCII);
            v_set_text(map_font.id, map_font.size, BLACK, 0, 0, chardim);
            map_font.ch = chardim[3] ? chardim[3] : 1;
            map_font.cw = chardim[2] ? chardim[2] : 1;
            mfdb(&mtmp, NULL, (COLNO - 1) * map_font.cw, ROWNO * map_font.ch,
                 0, planes);
            if (mfdb_size(&mtmp) > mfdb_size(&FontCol_Bild)
                && mfdb_size(&mtmp) > mfdb_size(&Map_bild)) {
                FontCol_Bild.fd_addr = Map_bild.fd_addr =
                    (int *) realloc(Map_bild.fd_addr, mfdb_size(&mtmp));
                if (!Map_bild.fd_addr) /* FIXME -- Not really neccessary since
                                          the former space is still valid */
                    panic("Not enough Space for the map.");
            }
            mfdb(&FontCol_Bild, FontCol_Bild.fd_addr,
                 (COLNO - 1) * map_font.cw, ROWNO * map_font.ch, 0, planes);
            rearrange_windows();
        }
        break;
    case NHW_STATUS:
        if (status_font.size == -size && status_font.id == id)
            break;
        status_font.size = -size;
        status_font.id = id;
        status_font.prop = FontInfo(id)->type & (FNT_PROP | FNT_ASCII);
        v_set_text(status_font.id, status_font.size, BLACK, 0, 0, chardim);
        status_font.ch = chardim[3] ? chardim[3] : 1;
        status_font.cw = chardim[2] ? chardim[2] : 1;
        rearrange_windows();
        break;
    case NHW_MENU:
        if (menu_font.size == -size && menu_font.id == id)
            break;
        menu_font.size = -size;
        menu_font.id = id;
        menu_font.prop = FontInfo(id)->type & (FNT_PROP | FNT_ASCII);
        v_set_text(menu_font.id, menu_font.size, BLACK, 0, 0, chardim);
        menu_font.ch = chardim[3] ? chardim[3] : 1;
        menu_font.cw = chardim[2] ? chardim[2] : 1;
        break;
    case NHW_TEXT:
        if (text_font.size == -size && text_font.id == id)
            break;
        text_font.size = -size;
        text_font.id = id;
        text_font.prop = FontInfo(id)->type & (FNT_PROP | FNT_ASCII);
        v_set_text(text_font.id, text_font.size, BLACK, 0, 0, chardim);
        text_font.ch = chardim[3] ? chardim[3] : 1;
        text_font.cw = chardim[2] ? chardim[2] : 1;
        break;
    default:
        break;
    }
}
void
mar_set_font(int type, const char *font_name, int size)
{
    int id = 0;
    /* MAR -- 17.Mar 2002 usual Gem behavior, use the Font-ID */
    if (font_name && *font_name) {
        id = atoi(font_name);
        if (id <= 0) {
            int i, tid;
            char name[32];
            for (i = fonts_loaded; --i >= 0;) {
                tid = vqt_name(x_handle, i, name);
                if (!stricmp(name, font_name)) {
                    id = tid;
                    break;
                }
            }
        }
    }
    mar_set_fontbyid(type, id, size);
}
void
rearrange_windows(void)
{
    GRECT area;
    int todo = TRUE;
    if (WIN_MAP != WIN_ERR && Gem_nhwindow[WIN_MAP].gw_window) {
        scroll_map.px_hline =
            mar_set_tile_mode(FAIL) ? Tile_width : map_font.cw;
        scroll_map.px_vline =
            mar_set_tile_mode(FAIL) ? Tile_heigth : map_font.ch;
        if (todo) {
            calc_std_winplace(FAIL, &area);
            todo = FALSE;
        }
        calc_std_winplace(NHW_MAP, &area);
        Gem_nhwindow[WIN_MAP].gw_window->max.g_w = area.g_w;
        Gem_nhwindow[WIN_MAP].gw_window->max.g_h = area.g_h;
        Gem_nhwindow[WIN_MAP].gw_window->max.g_w = area.g_w;
        window_reinit(Gem_nhwindow[WIN_MAP].gw_window, md, md, NULL, FALSE,
                      FALSE);
        {
            int buf[8];
            buf[3] = K_CTRL;
            buf[4] = C('L');
            AvSendMsg(ap_id, AV_SENDKEY, buf);
        }
    }
    if (WIN_MESSAGE != WIN_ERR && Gem_nhwindow[WIN_MESSAGE].gw_window) {
        if (todo) {
            calc_std_winplace(FAIL, &area);
            todo = FALSE;
        }
        calc_std_winplace(NHW_MESSAGE, &area);
        Gem_nhwindow[WIN_MESSAGE].gw_window->min_h = area.g_h;
        window_size(Gem_nhwindow[WIN_MESSAGE].gw_window, &area);
        redraw_window(Gem_nhwindow[WIN_MESSAGE].gw_window, NULL);
    }
    if (WIN_STATUS != WIN_ERR && Gem_nhwindow[WIN_STATUS].gw_window) {
        if (todo) {
            calc_std_winplace(FAIL, &area);
            todo = FALSE;
        }
        calc_std_winplace(NHW_STATUS, &area);
        Gem_nhwindow[WIN_STATUS].gw_window->min_h = area.g_h;
        window_size(Gem_nhwindow[WIN_STATUS].gw_window, &area);
        redraw_window(Gem_nhwindow[WIN_STATUS].gw_window, NULL);
    }
}
void
my_color_area(GRECT *area, int col)
{
    int pxy[4];

    v_set_fill(col, 1, IP_SOLID, 0);
    rc_grect_to_array(area, pxy);
    v_bar(x_handle, pxy);
}

void
my_clear_area(GRECT *area)
{
    my_color_area(area, WHITE);
}

int mar_set_tile_mode(int);

static void
win_draw_map(int first, WIN *win, GRECT *area)
{
    int pla[8], w = area->g_w - 1, h = area->g_h - 1;
    int i, x, y;
    GRECT back = *area;

    first = first;

    if (!mar_set_tile_mode(FAIL)) {
        int start =
            (area->g_x - win->work.g_x) / map_font.cw + scroll_map.hpos;
        int stop = (area->g_x + area->g_w + map_font.cw - 1 - win->work.g_x)
                       / map_font.cw
                   + scroll_map.hpos;
        int starty =
            (area->g_y - win->work.g_y) / map_font.ch + scroll_map.vpos;
        int stopy = min((area->g_y + area->g_h + map_font.ch - 1
                         - win->work.g_y) / map_font.ch
                            + scroll_map.vpos,
                        ROWNO);
        char tmp;
        v_set_text(map_font.id, map_font.size, WHITE, 0, 0, NULL);
        v_set_mode(MD_TRANS);

        x = win->work.g_x - scroll_map.px_hpos + start * map_font.cw;
        y = win->work.g_y - scroll_map.px_vpos + starty * map_font.ch;
        pla[2] = pla[0] = scroll_map.px_hpos + area->g_x - win->work.g_x;
        pla[3] = pla[1] = starty * map_font.ch;
        pla[2] += w;
        pla[3] += map_font.ch - 1;
        pla[6] = pla[4] = area->g_x; /* x_wert to */
        pla[7] = pla[5] = y;         /* y_wert to */
        pla[6] += w;
        pla[7] += map_font.ch - 1;
        back.g_h = map_font.ch;
        for (i = starty; i < stopy; i++, y += map_font.ch,
            pla[1] += map_font.ch, pla[3] += map_font.ch,
            pla[5] += map_font.ch, pla[7] += map_font.ch) {
            back.g_y = y;
            my_color_area(&back, BLACK);
            tmp = map_glyphs[i][stop];
            map_glyphs[i][stop] = 0;
            (*v_mtext)(x_handle, x, y, &map_glyphs[i][start]);
            map_glyphs[i][stop] = tmp;
            vro_cpyfm(x_handle, S_OR_D, pla, &FontCol_Bild, screen);
        }
    } else {
        v_set_mode(MD_REPLACE);
        pla[2] = pla[0] = scroll_map.px_hpos + area->g_x - win->work.g_x;
        pla[3] = pla[1] = scroll_map.px_vpos + area->g_y - win->work.g_y;
        pla[2] += w;
        pla[3] += h;
        pla[6] = pla[4] = area->g_x; /* x_wert to */
        pla[7] = pla[5] = area->g_y; /* y_wert to */
        pla[6] += w;
        pla[7] += h;
        vro_cpyfm(x_handle, S_ONLY, pla, &Map_bild, screen);
    }

    if (draw_cursor) {
        v_set_line(curs_col, 1, 1, 0, 0);
        pla[0] = pla[2] =
            win->work.g_x
            + scroll_map.px_hline * (map_cursx - scroll_map.hpos);
        pla[1] = pla[3] =
            win->work.g_y
            + scroll_map.px_vline * (map_cursy - scroll_map.vpos);
        pla[2] += scroll_map.px_hline - 1;
        pla[3] += scroll_map.px_vline - 1;
        v_rect(pla[0], pla[1], pla[2], pla[3]);
    }
}

static int
draw_titel(PARMBLK *pb)
{
    static int pla[8];
    GRECT work = *(GRECT *) &pb->pb_x;

    if (rc_intersect((GRECT *) &pb->pb_xc, &work)) {
        pla[0] = pla[1] = 0;
        pla[2] = pb->pb_w - 1;
        pla[3] = pb->pb_h - 1;
        pla[6] = pla[4] = pb->pb_x; /* x_wert to */
        pla[7] = pla[5] = pb->pb_y; /* y_wert to */
        pla[6] += pb->pb_w - 1;
        pla[7] += pb->pb_h - 1;

        vro_cpyfm(x_handle, S_ONLY, pla, &Titel_bild, screen);
    }

    return (0);
}

static int
draw_lines(PARMBLK *pb)
{
    GRECT area = *(GRECT *) &pb->pb_x;

    if (rc_intersect((GRECT *) &pb->pb_xc, &area)) {
        char **ptr;
        int x = pb->pb_x, y = pb->pb_y, start_line = (area.g_y - y);

        v_set_mode((text_font.cw & 7) == 0 && text_font.prop == 0 ? MD_REPLACE
                                                                  : MD_TRANS);

        /* void v_set_text(int font,int height,int color,int effect,int
         * rotate,int out[4])	*/
        v_set_text(text_font.id, text_font.size, BLACK, 0, 0, NULL);
        start_line /= text_font.ch;
        y += start_line * text_font.ch;
        x -= (int) scroll_menu.px_hpos;
        ptr = &text_lines[start_line += scroll_menu.vpos];
        start_line =
            min((area.g_y - y + area.g_h + text_font.ch - 1) / text_font.ch,
                Anz_text_lines - start_line);
        area.g_h = text_font.ch;
        Vsync();
        /*		x=(x+7) & ~7;*/
        for (; --start_line >= 0; y += text_font.ch) {
            area.g_y = y;
            my_clear_area(&area);
            if (**ptr - 1) {
                v_set_text(FAIL, 0, BLUE, 0x01, 0, NULL);
                (*v_mtext)(x_handle, x, y, (*ptr++) + 1);
                v_set_text(FAIL, 0, BLACK, 0x00, 0, NULL);
            } else
                (*v_mtext)(x_handle, x, y, (*ptr++) + 1);
        }
    }
    return (0);
}

static int
draw_rip(PARMBLK *pb)
{
    GRECT area = *(GRECT *) &pb->pb_x;
    if (rc_intersect((GRECT *) &pb->pb_xc, &area)) {
        char **ptr;
        int x = pb->pb_x, y = pb->pb_y, start_line = (area.g_y - y),
            chardim[4], pla[8], i;
        v_set_mode(MD_REPLACE);
        /* void v_set_text(int font,int height,int color,int effect,int
         * rotate,int out[4])	*/
        v_set_text(text_font.id, text_font.size, BLACK, 0, 0, chardim);
        start_line /= text_font.ch;
        y += start_line * text_font.ch;
        x -= scroll_menu.px_hpos;
        ptr = &text_lines[start_line += scroll_menu.vpos];
        start_line =
            min((area.g_y - y + area.g_h + text_font.ch - 1) / text_font.ch,
                Anz_text_lines - start_line);
        area.g_h = text_font.ch;
        Vsync();
        x = (x + 7) & ~7;
        for (; --start_line >= 0; y += text_font.ch) {
            area.g_y = y;
            my_clear_area(&area);
            if (**ptr - 1) {
                v_set_text(FAIL, 0, BLUE, 0x01, 0, NULL);
                (*v_mtext)(x_handle, x, y, (*ptr++) + 1);
                v_set_text(FAIL, 0, BLACK, 0x00, 0, NULL);
            } else
                (*v_mtext)(x_handle, x, y, (*ptr++) + 1);
        }
        pla[0] = pla[1] = 0;
        pla[2] = min(pb->pb_w - 1, Rip_bild.fd_w - 1);
        pla[3] = min(pb->pb_h - 1, Rip_bild.fd_h - 1);
        pla[6] = pla[4] =
            pb->pb_x + (pb->pb_w - Rip_bild.fd_w) / 2; /* x_wert to */
        pla[7] = pla[5] = pb->pb_y;                    /* y_wert to */
        pla[6] += pla[2];
        pla[7] += pla[3];
        vro_cpyfm(x_handle, S_ONLY, pla, &Rip_bild, screen);
        v_set_mode(MD_TRANS);
        vst_alignment(x_handle, 1, 5, &i, &i);
        pla[5] += 64;
        for (i = 0; i < 7; i++, pla[5] += chardim[3]) {
            v_set_text(text_font.id, (i == 0 || i == 6) ? text_font.size : 12,
                       WHITE, 1, 0, chardim);
            (*v_mtext)(x_handle, pla[4] + 157, pla[5], rip_line[i]);
            v_set_text(text_font.id, (i == 0 || i == 6) ? text_font.size : 12,
                       BLACK, 0, 0, chardim);
            (*v_mtext)(x_handle, pla[4] + 157, pla[5], rip_line[i]);
        }
        vst_alignment(x_handle, 0, 5, &i, &i);
    }
    return (0);
}

static int
draw_msgline(PARMBLK *pb)
{
    GRECT area = *(GRECT *) &pb->pb_x;

    if (rc_intersect((GRECT *) &pb->pb_xc, &area)) {
        int x = pb->pb_x, y = pb->pb_y + (msg_vis - 1) * msg_font.ch, foo, i;
        char **ptr = &message_line[msg_pos], tmp;
        int startx, stopx, starty, stopy;

        x = (x + 7) & ~7; /* Byte alignment speeds output up */

        v_set_mode(MD_REPLACE);

        /* void v_set_text(int font,int height,int color,int effect,int
         * rotate,int out[4])	*/
        v_set_text(msg_font.id, msg_font.size, FAIL, FAIL, 0, NULL);
        vst_alignment(x_handle, 0, 5, &foo, &foo);
        stopy = min(msg_pos, msg_vis);
        /*		Vsync();*/
        startx =
            (area.g_x - x) / msg_font.cw
            - 1; /* MAR 06.02.2001 -- because italic covers the next char */
        Max(&startx, 0);
        stopx = (area.g_x + area.g_w + msg_font.cw - x - 1) / msg_font.cw;
        x += startx * msg_font.cw;
        for (i = 0; i < stopy; i++, y -= msg_font.ch, ptr--) {
            if (message_age[msg_pos - i])
                v_set_text(FAIL, 0, BLACK, 0, 0, NULL);
            else
                v_set_text(FAIL, 0, LBLACK, 4, 0, NULL);
            tmp = (*ptr)[stopx];
            (*ptr)[stopx] = 0;
            (*v_mtext)(x_handle, x, y, &(*ptr)[startx]);
            (*ptr)[stopx] = tmp;
        }
    }
    return (0);
}

static int
draw_status(PARMBLK *pb)
{
    GRECT area = *(GRECT *) &pb->pb_x;

    area.g_x += 2 * status_font.cw - 2;
    area.g_w -= 2 * status_font.cw - 2;
    if (rc_intersect((GRECT *) &pb->pb_xc, &area)) {
        int x = pb->pb_x, y = pb->pb_y, startx, stopx, starty, stopy, i;
        char tmp;

        /* void v_set_text(int font,int height,int color,int effect,int
         * rotate,int out[4])	*/
        v_set_mode(MD_REPLACE);
        v_set_text(status_font.id, status_font.size, BLACK, 0, 0, NULL);
        x = (x + 2 * status_font.cw + 6) & ~7;

        startx = (area.g_x - x) / status_font.cw;
        starty = (area.g_y - y) / status_font.ch;
        stopx =
            (area.g_x + area.g_w + status_font.ch - 1 - x) / status_font.cw;
        stopy =
            (area.g_y + area.g_h + status_font.ch - 1 - y) / status_font.ch;
        Max(&startx, 0); /* MAR -- Hmm, area.g_x could end up 1 below x */
        Max(&stopx, 0);
        x += startx * status_font.cw;
        y += starty * status_font.ch;
        /*		Vsync();*/
        area.g_h = status_font.ch;
        for (i = starty; i < min(2, stopy);
             i++, area.g_y += status_font.ch, y += status_font.ch) {
            my_clear_area(&area);
            tmp = status_line[i][stopx];
            status_line[i][stopx] = 0;
            (*v_mtext)(x_handle, x, y, &status_line[i][startx]);
            status_line[i][stopx] = tmp;
        }
    }
    return (0);
}

static int
draw_inventory(PARMBLK *pb)
{
    GRECT area = *(GRECT *) &pb->pb_x;

    if (rc_intersect((GRECT *) &pb->pb_xc, &area)) {
        int gl, i, x = pb->pb_x, y = pb->pb_y, start_line = area.g_y - y;
        Gem_menu_item *it;

        v_set_mode(MD_REPLACE);
        v_set_text(menu_font.id, menu_font.size, BLACK, 0, 0, NULL);

        start_line /= menu_font.ch;
        y += start_line * menu_font.ch;
        x -= scroll_menu.px_hpos;
        start_line += scroll_menu.vpos;

        for (it = invent_list, i = start_line; --i >= 0 && it;
             it = it->Gmi_next)
            ;

        i = min((area.g_y - y + area.g_h + menu_font.ch - 1) / menu_font.ch,
                Anz_inv_lines - start_line);

        Vsync();
        area.g_h = menu_font.ch;

        for (; (--i >= 0) && it; it = it->Gmi_next, y += menu_font.ch) {
            if (it->Gmi_attr)
                v_set_text(FAIL, FALSE, BLUE, 1, FAIL, NULL); /* Bold */
            else
                v_set_text(FAIL, FALSE, BLACK, 0, FAIL, NULL);

            area.g_y = y;
            my_clear_area(&area);
            if ((gl = it->Gmi_glyph) != no_glyph) {
                int pla[8], h = min(menu_font.ch, Tile_heigth) - 1;

                pla[0] = pla[2] =
                    (gl % Tiles_per_line) * Tile_width; /* x_wert from */
                pla[1] = pla[3] =
                    (gl / Tiles_per_line) * Tile_heigth; /* y_wert from */
                pla[4] = pla[6] = x;                     /* x_wert to */
                pla[5] = pla[7] = y;                     /* y_wert to */
                pla[2] += Tile_width - 1;
                pla[3] += h;
                pla[6] += Tile_heigth - 1;
                pla[7] += h;

                vro_cpyfm(x_handle, S_ONLY, pla, &Tile_bilder, screen);
            }
            if (it->Gmi_identifier)
                it->Gmi_str[2] = it->Gmi_selected
                                     ? (it->Gmi_count == -1L ? '+' : '#')
                                     : '-';
            (*v_mtext)(x_handle, (x + 23) & ~7, y, it->Gmi_str);
        }
    }
    return (0);
}

static int
draw_prompt(PARMBLK *pb)
{
    GRECT area = *(GRECT *) &pb->pb_x;

    if (rc_intersect((GRECT *) &pb->pb_xc, &area)) {
        char **ptr = (char **) pb->pb_parm;
        int x = pb->pb_x, y = pb->pb_y, chardim[4];

        /* void v_set_text(int font,int height,int color,int effect,int
         * rotate,int out[4])	*/
        v_set_mode(MD_TRANS);
        v_set_text(ibm_font_id, ibm_font, WHITE, 0, 0, chardim);
        Vsync();
        if (planes < 4) {
            int pxy[4];
            v_set_fill(BLACK, 2, 4, 0);
            rc_grect_to_array(&area, pxy);
            v_bar(x_handle, pxy);
        } else
            my_color_area(&area, LWHITE);
        (*v_mtext)(x_handle, x, y, *(ptr++));
        if (*ptr)
            (*v_mtext)(x_handle, x, y + chardim[3], *ptr);
    }
    return (0);
}

static USERBLK ub_lines = { draw_lines, 0L }, ub_msg = { draw_msgline, 0L },
               ub_inventory = { draw_inventory, 0L },
               ub_titel = { draw_titel, 0L }, ub_status = { draw_status, 0L },
               ub_prompt = { draw_prompt, 0L };

/**************************** rsc_funktionen *****************************/

void
my_close_dialog(DIAINFO *dialog, boolean shrink_box)
{
    close_dialog(dialog, shrink_box);
    Event_Timer(0, 0, TRUE);
}

void
mar_get_rsc_tree(obj_number, z_ob_obj)
int obj_number;
OBJECT **z_ob_obj;
{
    rsrc_gaddr(R_TREE, obj_number, z_ob_obj);
    fix_objects(*z_ob_obj, SCALING, 0, 0);
}

void mar_clear_map(void);

void
img_error(errnumber)
int errnumber;
{
    char buf[BUFSZ];

    switch (errnumber) {
    case ERR_HEADER:
        sprintf(buf, "%s", "[1][ Image Header | corrupt. ][ Oops ]");
        break;
    case ERR_ALLOC:
        sprintf(buf, "%s",
                "[1][ Not enough | memory for | an image. ][ Oops ]");
        break;
    case ERR_FILE:
        sprintf(buf, "%s",
                "[1][ The Image-file | is not available ][ Oops ]");
        break;
    case ERR_DEPACK:
        sprintf(buf, "%s", "[1][ The Image-file | is corrupt ][ Oops ]");
        break;
    case ERR_COLOR:
        sprintf(buf, "%s", "[1][ Number of colors | not supported ][ Oops ]");
        break;
    default:
        sprintf(buf, "[1][ img_error | strange error | number: %i ][ Hmm ]",
                errnumber);
        break;
    }
    form_alert(1, buf);
}

void
mar_change_button_char(OBJECT *z_ob, int nr, char ch)
{
    *ob_get_text(z_ob, nr, 0) = ch;
    ob_set_hotkey(z_ob, nr, ch);
}

void
mar_set_dir_keys()
{
    static int mi_numpad = FAIL;
    char mcmd[] = "bjnh.lyku", npcmd[] = "123456789", *p_cmd;

    if (mi_numpad != mar_iflags_numpad()) {
        OBJECT *z_ob = zz_oblist[DIRECTION];
        int i;
        mi_numpad = mar_iflags_numpad();
        ob_set_hotkey(z_ob, DIRDOWN, '>');
        ob_set_hotkey(z_ob, DIRUP, '<');
        p_cmd = mi_numpad ? npcmd : mcmd;
        for (i = 0; i < 9; i++)
            mar_change_button_char(z_ob, DIR1 + 2 * i, p_cmd[i]);
    }
}

extern int total_tiles_used; /* tile.c */

int
mar_gem_init()
{
    int i, bild_fehler = FALSE, fsize;
    char *fname;
    static MITEM wish_workaround = { FAIL, key(0, 'J'), K_CTRL, W_CYCLE,
                                     FAIL };
    OBJECT *z_ob;

    if ((i = open_rsc("gem_rsc.rsc", NULL, md, md, md, 0, 0, 0)) <= 0) {
        graf_mouse(M_OFF, NULL);
        if (i < 0)
            form_alert(1, "[3][| Fatal Error | File: GEM_RSC.RSC | not "
                          "found. ][ grumble ]");
        else
            form_alert(1, "[3][| Fatal Error | GEM initialisation | failed. "
                          "][ a pity ]");
        return (0);
    }
    if (planes < 1 || planes > 8) {
        form_alert(
            1,
            "[3][ Color-depth | not supported, | try 2-256 colors. ][ Ok ]");
        return (0);
    }
    MouseBee();

    /* MAR -- 17.Mar 2002 NVDI 3.0 or better uses v_ftext */
    v_mtext = speedo == 3 ? &v_ftext : &v_gtext;
    for (i = 0; i < NHICON; i++)
        mar_get_rsc_tree(i, &zz_oblist[i]);

    z_ob = zz_oblist[ABOUT];
    ob_hide(z_ob, OKABOUT, TRUE);
    ob_draw_dialog(z_ob, 0, 0, 0, 0);

    mar_get_font(NHW_MESSAGE, &fname, &fsize);
    mar_set_font(NHW_MESSAGE, fname, fsize);
    mar_get_font(NHW_MAP, &fname, &fsize);
    mar_set_font(NHW_MAP, fname, fsize);
    mar_get_font(NHW_STATUS, &fname, &fsize);
    mar_set_font(NHW_STATUS, fname, fsize);
    mar_get_font(NHW_MENU, &fname, &fsize);
    mar_set_font(NHW_MENU, fname, fsize);
    mar_get_font(NHW_TEXT, &fname, &fsize);
    mar_set_font(NHW_TEXT, fname, fsize);
    msg_anz = mar_get_msg_history();
    mar_set_msg_visible(mar_get_msg_visible());
    msg_width = min(max_w / msg_font.cw - 3, MSGLEN);

    if (max_w / status_font.cw < COLNO - 1)
        mar_set_fontbyid(NHW_STATUS, small_font_id, -small_font);
    status_w = min(max_w / status_font.cw - 3, MSGLEN);

    if (planes > 0 && planes < 9) {
        normal_palette = (short *) m_alloc(3 * colors * sizeof(short));
        get_colors(x_handle, normal_palette, colors);
    }

loadimg:
    bild_fehler = depack_img(Tilefile ? Tilefile : (planes >= 4) ? "NH16.IMG"
                                                                 : "NH2.IMG",
                             &tile_image);
    if (bild_fehler) {
        z_ob = zz_oblist[ABOUT];
        ob_undraw_dialog(z_ob, 0, 0, 0, 0);
        ob_hide(z_ob, OKABOUT, FALSE);
        img_error(bild_fehler);
        return (0);
    }
    if (tile_image.img_w % Tile_width || tile_image.img_h % Tile_heigth) {
        Tilefile = NULL;
        Tile_width = Tile_heigth = 16;
        printf("size didn't match.\n");
        goto loadimg;
    }
    if ((tile_image.img_w / Tile_width) * (tile_image.img_h / Tile_heigth)
        < total_tiles_used) {
        Tilefile = NULL;
        Tile_width = Tile_heigth = 16;
        printf("Too few Tiles in Image.\n");
        goto loadimg;
    }
    Tiles_per_line = tile_image.img_w / Tile_width;

    if (planes >= 4) {
        if (tile_image.planes > 1)
            img_set_colors(x_handle, tile_image.palette, tile_image.planes);
#if 0
		else{
			int mypalette[]={};
			img_set_colors(x_handle, mypalette, 4);
		}
#endif
    }

    mfdb(&Tile_bilder, (int *) tile_image.addr, tile_image.img_w,
         tile_image.img_h, 1, tile_image.planes);
    transform_img(&Tile_bilder);

    mfdb(&Map_bild, NULL, (COLNO - 1) * Tile_width, ROWNO * Tile_heigth, 0,
         planes);
    mfdb(&FontCol_Bild, NULL, (COLNO - 1) * map_font.cw, ROWNO * map_font.ch,
         0, planes);
    Map_bild.fd_addr =
        (int *) m_alloc(mfdb_size(&Map_bild) > mfdb_size(&FontCol_Bild)
                            ? mfdb_size(&Map_bild)
                            : mfdb_size(&FontCol_Bild));
    FontCol_Bild.fd_addr = Map_bild.fd_addr;

    mfdb(&Pet_Mark, pet_mark_data, 8, 7, 1, 1);
    vr_trnfm(x_handle, &Pet_Mark, &Pet_Mark);

    mfdb(&Black_bild, NULL, 16, 32, 1,
         1); /* MAR -- 17.Mar 2002 that should cover the biggest map-font */
    Black_bild.fd_addr = (int *) m_alloc(mfdb_size(&Black_bild));
    memset(Black_bild.fd_addr, 255, mfdb_size(&Black_bild));
    vr_trnfm(x_handle, &Black_bild, &Black_bild);

    for (i = 0; i < MAXWIN; i++) {
        Gem_nhwindow[i].gw_window = NULL;
        Gem_nhwindow[i].gw_type = 0;
        Gem_nhwindow[i].gw_dirty = TRUE;
    }

    memset(&scroll_menu, 0, sizeof(scroll_menu));
    scroll_menu.scroll = AUTO_SCROLL;
    scroll_menu.obj = LINESLIST;
    scroll_menu.px_hline = menu_font.cw;
    scroll_menu.px_vline = menu_font.ch;
    scroll_menu.hscroll = scroll_menu.vscroll = 1;
    scroll_menu.tbar_d = 2 * gr_ch - 2;

    mar_set_dir_keys();

    memset(&scroll_map, 0, sizeof(scroll_map));
    scroll_map.scroll = AUTO_SCROLL;
    scroll_map.obj = ROOT;
    scroll_map.px_hline = mar_set_tile_mode(FAIL) ? Tile_width : map_font.cw;
    scroll_map.px_vline = mar_set_tile_mode(FAIL) ? Tile_heigth : map_font.ch;
    scroll_map.hsize = COLNO - 1;
    scroll_map.vsize = ROWNO;
    scroll_map.hpage = 8;
    scroll_map.vpage = 8;
    scroll_map.hscroll = 1;
    scroll_map.vscroll = 1;

    /* dial_options( round, niceline, standard, return_default, background,
       nonselectable,
            always_keys, toMouse, clipboard, hz);	*/
    dial_options(TRUE, TRUE, FALSE, RETURN_DEFAULT, AES_BACK, TRUE,
                 KEY_ALWAYS, FALSE, TRUE, 3);
    /*	dial_colors( dial_pattern, dial_color, dial_frame, hotkey, alert,
       cycle_button,
            check_box, radio_button, arrow, cycle_backgrnd, check_backgrnd,
       radio_backgrnd,
            arrow_backgrnd, edit_3d, draw_3d)	*/
    if (planes < 4)
        dial_colors(4, BLACK, WHITE, RED, RED, WHITE, BLACK, BLACK, BLACK,
                    FAIL, FAIL, FAIL, FAIL, TRUE, TRUE);
    else
        dial_colors(7, LWHITE, BLACK, RED, RED, BLACK, BLACK, BLACK, BLACK,
                    WHITE, WHITE, WHITE, WHITE, TRUE, TRUE);

    /* void MenuItems(MITEM *close,MITEM *closeall,MITEM *cycle,MITEM
       *invcycle,
            MITEM *globcycle,MITEM *full,MITEM *bottom,MITEM *iconify,MITEM
       *iconify_all,
            MITEM *menu,int menu_cnt) */
    /* Ctrl-W ist normaly bound to cycle */
    MenuItems(NULL, NULL, &wish_workaround, NULL, NULL, NULL, NULL, NULL,
              NULL, NULL, 0);

    menu_install(zz_oblist[MENU], TRUE);

    z_ob = zz_oblist[ABOUT];
    ob_undraw_dialog(z_ob, 0, 0, 0, 0);
    ob_hide(z_ob, OKABOUT, FALSE);

    return (1);
}

/************************* mar_exit_nhwindows *******************************/

void
mar_exit_nhwindows()
{
    int i;

    for (i = MAXWIN; --i >= 0;)
        if (Gem_nhwindow[i].gw_type)
            mar_destroy_nhwindow(i);

    if (normal_palette) {
        img_set_colors(x_handle, normal_palette, tile_image.planes);
        null_free(normal_palette);
    }
    test_free(tile_image.palette);
    test_free(tile_image.addr);
    test_free(titel_image.palette);
    test_free(titel_image.addr);
}

/************************* mar_curs *******************************/

void
mar_curs(x, y)
int x, y;
{
    Min(&dirty_map_area.g_x, x);
    Min(&dirty_map_area.g_y, y);
    Max(&dirty_map_area.g_w, x);
    Max(&dirty_map_area.g_h, y);
    Min(&dirty_map_area.g_x, map_cursx);
    Min(&dirty_map_area.g_y, map_cursy);
    Max(&dirty_map_area.g_w, map_cursx);
    Max(&dirty_map_area.g_h, map_cursy);

    map_cursx = x;
    map_cursy = y;

    if (WIN_MAP != WIN_ERR)
        Gem_nhwindow[WIN_MAP].gw_dirty = TRUE;
}

void mar_cliparound(void);
void
mar_map_curs_weiter(void)
{
    static int once = TRUE;

    if (once) {
        redraw_window(Gem_nhwindow[WIN_STATUS].gw_window, NULL);
        redraw_window(Gem_nhwindow[WIN_MESSAGE].gw_window, NULL);
        once = FALSE;
    }
    mar_curs(map_cursx + 1, map_cursy);
    mar_cliparound();
}

/************************* about *******************************/

void
mar_about()
{
    xdialog(zz_oblist[ABOUT], md, NULL, NULL, DIA_CENTERED, FALSE,
            DIALOG_MODE);
    Event_Timer(0, 0, TRUE);
}

/************************* ask_name *******************************/

char *
mar_ask_name()
{
    OBJECT *z_ob = zz_oblist[NAMEGET];
    int bild_fehler;
    char who_are_you[] = "Who are you? ";

    bild_fehler =
        depack_img(planes < 4 ? "TITLE2.IMG" : "TITLE.IMG", &titel_image);
    if (bild_fehler) { /* MAR -- this isn't lethal */
        ob_set_text(z_ob, NETHACKPICTURE, "missing title.img.");
    } else {
        mfdb(&Titel_bild, (int *) titel_image.addr, titel_image.img_w,
             titel_image.img_h, 1, titel_image.planes);
        transform_img(&Titel_bild);
        z_ob[NETHACKPICTURE].ob_type = G_USERDEF;
        z_ob[NETHACKPICTURE].ob_spec.userblk = &ub_titel;
    }

    ob_clear_edit(z_ob);
    xdialog(z_ob, who_are_you, NULL, NULL, DIA_CENTERED, FALSE, DIALOG_MODE);
    Event_Timer(0, 0, TRUE);

    test_free(titel_image.palette);
    test_free(titel_image.addr);
    test_free(Titel_bild.fd_addr);
    return (ob_get_text(z_ob, PLNAME, 0));
}

/************************* more *******************************/

void
send_key(int key)
{
    int buf[8];

    buf[3] = 0; /* No Shift/Ctrl/Alt */
    buf[4] = key;
    AvSendMsg(ap_id, AV_SENDKEY, buf);
}

void
send_return()
{
    send_key(key(SCANRET, 0));
}

int
K_Init(xev, availiable)
XEVENT *xev;
int availiable;
{
    xev = xev;
    return (MU_KEYBD & availiable);
}

int
KM_Init(xev, availiable)
XEVENT *xev;
int availiable;
{
    xev = xev;
    return ((MU_KEYBD | MU_MESAG) & availiable);
}

int
M_Init(xev, availiable)
XEVENT *xev;
int availiable;
{
    xev = xev;
    return (MU_MESAG & availiable);
}

#define More_Init K_Init

int
More_Handler(xev)
XEVENT *xev;
{
    int ev = xev->ev_mwich;

    if (ev & MU_KEYBD) {
        char ch = (char) (xev->ev_mkreturn & 0x00FF);
        DIAINFO *dinf;
        WIN *w;

        switch (ch) {
        case '\033': /* no more more more */
        case ' ':
            if ((w = get_top_window()) && (dinf = (DIAINFO *) w->dialog)
                && dinf->di_tree == zz_oblist[PAGER]) {
                if (ch == '\033')
                    mar_esc_pressed = TRUE;
                send_return();
                break;
            }
        /* Fall thru */
        default:
            ev &= ~MU_KEYBD; /* unknown key */
            break;
        }
    }
    return (ev);
}

void
mar_more()
{
    if (!mar_esc_pressed) {
        OBJECT *z_ob = zz_oblist[PAGER];
        WIN *p_w;

        Event_Handler(More_Init, More_Handler);
        dial_colors(7, RED, BLACK, RED, RED, BLACK, BLACK, BLACK, BLACK,
                    WHITE, WHITE, WHITE, WHITE, TRUE, TRUE);
        if (WIN_MESSAGE != WIN_ERR
            && (p_w = Gem_nhwindow[WIN_MESSAGE].gw_window)) {
            z_ob->ob_x = p_w->work.g_x;
            z_ob->ob_y = p_w->curr.g_y + p_w->curr.g_h + gr_ch;
        }
        xdialog(z_ob, NULL, NULL, NULL, DIA_LASTPOS, FALSE, DIALOG_MODE);
        Event_Timer(0, 0, TRUE);
        Event_Handler(NULL, NULL);

        if (planes < 4)
            dial_colors(4, BLACK, WHITE, RED, RED, WHITE, BLACK, BLACK, BLACK,
                        FAIL, FAIL, FAIL, FAIL, TRUE, TRUE);
        else
            dial_colors(7, LWHITE, BLACK, RED, RED, BLACK, BLACK, BLACK,
                        BLACK, WHITE, WHITE, WHITE, WHITE, TRUE, TRUE);
    }
}

/************************* Gem_start_menu *******************************/
void
Gem_start_menu(win, mbehavior)
winid win;
unsigned long mbehavior UNUSED;
{
    win = win;
    if (invent_list) {
        Gem_menu_item *curr, *next;

        for (curr = invent_list; curr; curr = next) {
            next = curr->Gmi_next;
            free(curr->Gmi_str);
            free(curr);
        }
    }
    invent_list = NULL;
    Anz_inv_lines = 0;
    Inv_breite = 16;
}

/************************* mar_add_menu *******************************/

void
mar_add_menu(win, item)
winid win;
Gem_menu_item *item;
{
    win = win;
    item->Gmi_next = invent_list;
    invent_list = item;
    Anz_inv_lines++;
}

void
mar_reverse_menu()
{
    Gem_menu_item *next, *head = 0, *curr = invent_list;

    while (curr) {
        next = curr->Gmi_next;
        curr->Gmi_next = head;
        head = curr;
        curr = next;
    }
    invent_list = head;
}

void
mar_set_accelerators()
{
    char ch = 'a';
    Gem_menu_item *curr;

    for (curr = invent_list; curr; curr = curr->Gmi_next) {
        int extent[8];
        v_set_text(menu_font.id, menu_font.size, BLACK, 0, 0, NULL);
        vqt_extent(x_handle, curr->Gmi_str, extent);
        Max(&Inv_breite, extent[4] + Tile_width + menu_font.cw);
        if (ch && curr->Gmi_accelerator == 0 && curr->Gmi_identifier) {
            curr->Gmi_accelerator = ch;
            curr->Gmi_str[0] = ch;
            if (ch == 'z')
                ch = 'A';
            else if (ch == 'Z')
                ch = 0;
            else
                ch++;
        }
    }
}

Gem_menu_item *
mar_hol_inv()
{
    return (invent_list);
}

/************************* mar_putstr_text *********************/

void mar_raw_print(const char *);

void
mar_set_text_to_rip(winid w)
{
    use_rip = TRUE;
}
void
mar_putstr_text(winid window, int attr, const char *str)
{
    static int zeilen_frei = 0;
    int breite;
    char *ptr;

    window = window;
    if (!text_lines) {
        text_lines = (char **) m_alloc(12 * sizeof(char *));
        zeilen_frei = 12;
    }
    if (!zeilen_frei) {
        text_lines = (char **) realloc(text_lines, (Anz_text_lines + 12)
                                                       * sizeof(char *));
        zeilen_frei = 12;
    }
    if (!text_lines) {
        mar_raw_print("No room for Text");
        return;
    }

    if (str)
        breite = strlen(str);
    Min(&breite, 80);
    ptr = text_lines[Anz_text_lines] =
        (char *) m_alloc(breite * sizeof(char) + 2);
    *ptr = (char) (attr + 1); /* avoid 0 */
    strncpy(ptr + 1, str, breite);
    ptr[breite + 1] = 0;
    Anz_text_lines++;
    zeilen_frei--;
}

int
mar_set_inv_win(Anzahl, Breite)
int Anzahl, Breite;
{
    OBJECT *z_ob = zz_oblist[LINES];
    int retval = WIN_DIAL | MODAL | NO_ICONIFY;

    scroll_menu.hsize = 0;
    scroll_menu.vpage = (desk.g_h - 3 * gr_ch) / scroll_menu.px_vline;
    if (Anzahl > scroll_menu.vpage) {
        retval |= WD_VSLIDER;
        if (Breite > max_w - 3 * scroll_menu.px_hline) {
            retval |= WD_HSLIDER;
            scroll_menu.hpage =
                (max_w - 3 * scroll_menu.px_hline) / scroll_menu.px_hline;
            scroll_menu.hpos = 0;
            scroll_menu.hsize = Breite / scroll_menu.px_hline;
            scroll_menu.vpage =
                (desk.g_h - 4 * gr_ch - 1) / scroll_menu.px_vline;
        }
        Anzahl = scroll_menu.vpage;
    } else {
        if (Breite > max_w - scroll_menu.px_hline) {
            retval |= WD_HSLIDER;
            scroll_menu.hpage =
                (max_w - scroll_menu.px_hline) / scroll_menu.px_hline;
            scroll_menu.hpos = 0;
            scroll_menu.hsize = Breite / scroll_menu.px_hline;
            scroll_menu.vpage =
                (desk.g_h - 4 * gr_ch - 1) / scroll_menu.px_vline;
            if (Anzahl > scroll_menu.vpage) {
                retval |= WD_VSLIDER;
                Anzahl = scroll_menu.vpage;
            }
        }
        scroll_menu.vpage = Anzahl;
    }
    if ((scroll_menu.hmax = scroll_menu.hsize - scroll_menu.hpage) < 0)
        scroll_menu.hmax = 0;
    if ((scroll_menu.vmax = scroll_menu.vsize - scroll_menu.vpage) < 0)
        scroll_menu.vmax = 0;

    /* left/right/up 2 pixel border down 2gr_ch toolbar */
    z_ob[ROOT].ob_width = z_ob[LINESLIST].ob_width = Breite;
    z_ob[ROOT].ob_height = z_ob[QLINE].ob_y = z_ob[LINESLIST].ob_height =
        scroll_menu.px_vline * Anzahl;
    z_ob[QLINE].ob_y += gr_ch / 2;
    z_ob[ROOT].ob_width += 4;
    z_ob[ROOT].ob_height += 2 * gr_ch + 2;

    return (retval);
}

/************************* mar_status_dirty *******************************/

void
mar_status_dirty()
{
    int ccol;

    ccol = mar_hp_query();

    if (ccol < 2)
        curs_col = WHITE; /* 50-100% : 0 */
    else if (ccol < 3)
        curs_col = YELLOW; /* 33-50% : 6 */
    else if (ccol < 5)
        curs_col = LYELLOW; /* 20-33% : 14*/
    else if (ccol < 10)
        curs_col = RED; /* 10-20% : 2 */
    else
        curs_col = MAGENTA; /* <10% : 7*/
}

/************************* mar_add_message *******************************/

void
mar_add_message(str)
const char *str;
{
    int i, mesg_hist = mar_get_msg_history();
    char *tmp, *rest, buf[TBUFSZ];

    if (WIN_MESSAGE == WIN_ERR)
        return;

    if (!mar_message_pause) {
        mar_message_pause = TRUE;
        messages_pro_zug = 0;
        msg_pos = msg_max;
    }

    if (msg_max > mesg_hist - 2) {
        msg_max = mesg_hist - 2;
        msg_pos--;
        if (msg_pos < 0)
            msg_pos = 0;
        tmp = message_line[0];
        for (i = 0; i < mesg_hist - 1; i++) {
            message_line[i] = message_line[i + 1];
            message_age[i] = message_age[i + 1];
        }
        message_line[mesg_hist - 1] = tmp;
    }
    strcpy(gt.toplines, str);
    messages_pro_zug++;
    msg_max++;

    if ((int) strlen(gt.toplines) >= msg_width) {
        int pos = msg_width;
        tmp = gt.toplines + msg_width;
        while (*tmp != ' ' && pos >= 0) {
            tmp--;
            pos--;
        }
        if (pos <= 0)
            pos = msg_width; /* Mar -- Oops, what a word :-) */
        message_age[msg_max] = TRUE;
        strncpy(message_line[msg_max], gt.toplines, pos);
        message_line[msg_max][pos] = 0;
        rest = strcpy(buf, gt.toplines + pos);
    } else {
        message_age[msg_max] = TRUE;
        strncpy(message_line[msg_max], gt.toplines, msg_width);
        rest = 0;
    }

    Gem_nhwindow[WIN_MESSAGE].gw_dirty = TRUE;
    if (messages_pro_zug
        >= mesg_hist) { /* MAR -- Greater then should never happen */
        messages_pro_zug = mesg_hist;
        mar_display_nhwindow(WIN_MESSAGE);
    }

    if (rest)
        mar_add_message(rest);
}

/************************* mar_add_status_str *******************************/

void
mar_add_status_str(str, line)
const char *str;
int line;
{
    int i, last_diff = -1;
    GRECT area = { 0, line * status_font.ch, status_font.cw, status_font.ch };
    for (i = 0; (i < status_w - 2) && str[i]; i++)
        if (str[i] != status_line[line][i]) {
            if (last_diff == -1)
                area.g_x = i * status_font.cw;
            else
                area.g_w += status_font.cw;
            last_diff = i;
            status_line[line][i] = str[i];
        } else if (last_diff >= 0) {
            add_dirty_rect(dr_stat, &area);
            last_diff = -1;
            area.g_w = status_font.cw;
        }
    for (; i < status_w - 1; i++) {
        if (status_line[line][i]) {
            if (last_diff == -1)
                area.g_x = i * status_font.cw;
            else
                area.g_w += status_font.cw;
            last_diff = i;
        }
        status_line[line][i] = 0;
    }
    if (last_diff >= 0)
        add_dirty_rect(dr_stat, &area);
}

/************************* mar_set_menu_title *******************************/

void
mar_set_menu_title(str)
const char *str;
{
    test_free(Menu_title); /* just in case */
    Menu_title = mar_copy_of(str ? str : nullstr);
}

/************************* mar_set_menu_type *******************************/

void
mar_set_menu_type(how)
int how;
{
    Inv_how = how;
}

/************************* Inventory Utils *******************************/

void
set_all_on_page(start, page)
int start, page;
{
    Gem_menu_item *curr;

    if (start < 0 || page < 0)
        return;

    for (curr = invent_list; start-- && curr; curr = curr->Gmi_next)
        ;
    for (; page-- && curr; curr = curr->Gmi_next)
        if (curr->Gmi_identifier && !curr->Gmi_selected)
            curr->Gmi_selected = TRUE;
}

void
unset_all_on_page(start, page)
int start, page;
{
    Gem_menu_item *curr;

    if (start < 0 || page < 0)
        return;

    for (curr = invent_list; start-- && curr; curr = curr->Gmi_next)
        ;
    for (; page-- && curr; curr = curr->Gmi_next)
        if (curr->Gmi_identifier && curr->Gmi_selected) {
            curr->Gmi_selected = FALSE;
            curr->Gmi_count = -1L;
        }
}

void
invert_all_on_page(start, page, acc)
int start, page;
char acc;
{
    Gem_menu_item *curr;

    if (start < 0 || page < 0)
        return;

    for (curr = invent_list; start-- && curr; curr = curr->Gmi_next)
        ;
    for (; page-- && curr; curr = curr->Gmi_next) {
        if (!menuitem_invert_test(0, curr->Gmi_itemflags, curr->Gmi_selected)
            continue;

        if (curr->Gmi_identifier && (acc == 0 || curr->Gmi_groupacc == acc)) {
            if (curr->Gmi_selected) {
                curr->Gmi_selected = FALSE;
                curr->Gmi_count = -1L;
            } else
                curr->Gmi_selected = TRUE;
        }
    }
}

/************************* Inv_Handler and Inv_Init
 * *******************************/

int
scroll_top_dialog(char ch)
{
    WIN *w;
    DIAINFO *dinf;

    if ((w = get_top_window()) && (dinf = (DIAINFO *) w->dialog)
        && dinf->di_tree == zz_oblist[LINES]) {
        switch (ch) {
        case ' ':
            if (scroll_menu.vpos == scroll_menu.vmax) {
                send_return();
                break;
            }
        /* Fall thru */
        case MENU_NEXT_PAGE:
            scroll_window(w, PAGE_DOWN, NULL);
            break;
        case MENU_PREVIOUS_PAGE:
            scroll_window(w, PAGE_UP, NULL);
            break;
        case MENU_FIRST_PAGE:
            scroll_window(w, WIN_START, NULL);
            break;
        case MENU_LAST_PAGE:
            scroll_window(w, WIN_END, NULL);
            break;
        default:
            return (FALSE);
        }
        return (TRUE);
    }
    return (FALSE);
}

#define Text_Init KM_Init

int
Text_Handler(xev)
XEVENT *xev;
{
    int ev = xev->ev_mwich;

    if (ev & MU_MESAG) {
        int *buf = xev->ev_mmgpbuf, y_wo, i;
        if (*buf == FONT_CHANGED) {
            if (buf[3] >= 0) {
                mar_set_fontbyid(NHW_TEXT, buf[4], buf[5]);
                FontAck(buf[1], 1);
            }
        }
    }
    if (ev & MU_KEYBD) {
        char ch = (char) (xev->ev_mkreturn & 0x00FF);

        if (!scroll_top_dialog(ch))
            switch (ch) {
            case '\033':
                send_return(); /* just closes the textwin */
                break;
            case C('c'):
                clipbrd_save(text_lines, Anz_text_lines,
                             xev->ev_mmokstate & K_SHIFT, FALSE);
                break;
            default:
                ev &= ~MU_KEYBD; /* unknown key */
                break;
            }
    }
    return (ev);
}

#define Inv_Init KM_Init

static long count = 0;
int
Inv_Handler(xev)
XEVENT *xev;
{
    int ev = xev->ev_mwich;
    Gem_menu_item *it;
    GRECT area;
    OBJECT *z_ob = zz_oblist[LINES];

    ob_pos(z_ob, LINESLIST, &area);
    if (ev & MU_MESAG) {
        int *buf = xev->ev_mmgpbuf, y_wo, i;

        if (*buf == FONT_CHANGED) {
            if (buf[3] >= 0) {
                mar_set_fontbyid(NHW_MENU, buf[4], buf[5]);
                FontAck(buf[1], 1);
            }
        } else if (*buf == OBJC_CHANGED && buf[3] == LINESLIST) {
            ob_undostate(z_ob, LINESLIST, SELECTED);
            mouse(NULL, &y_wo);
            y_wo = (y_wo - area.g_y) / menu_font.ch + scroll_menu.vpos;
            for (it = invent_list, i = 0; i < y_wo && it;
                 it = it->Gmi_next, i++)
                ;
            if (it->Gmi_identifier) {
                it->Gmi_selected = !it->Gmi_selected;
                it->Gmi_count = count == 0L ? -1L : count;
                count = 0L;
                if (Inv_how != PICK_ANY) {
                    /*my_close_dialog(Inv_dialog,TRUE);*/
                    send_return();
                } else {
                    area.g_x = (area.g_x + 23 + 2 * menu_font.cw) & ~7;
                    area.g_w = menu_font.cw;
                    area.g_h = menu_font.ch;
                    area.g_y += (y_wo - scroll_menu.vpos) * menu_font.ch;
                    ob_draw_chg(Inv_dialog, LINESLIST, &area, FAIL);
                }            /* how != PICK_ANY */
            }                /* identifier */
        } else               /* LINESLIST changed */
            ev &= ~MU_MESAG; /* unknown message not used */
    }                        /* MU_MESAG */

    if (ev & MU_KEYBD) {
        char ch = (char) (xev->ev_mkreturn & 0x00FF);

        if (!scroll_top_dialog(ch)) {
            switch (ch) {
            case '0': /* special 0 is also groupaccelerator for balls */
                if (count <= 0)
                    goto find_acc;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                if (Inv_how == PICK_NONE)
                    goto find_acc;
                count = (count * 10L) + (long) (ch - '0');
                break;
            case '\033': /* cancel - from counting or loop */
                if (count > 0L)
                    count = 0L;
                else {
                    unset_all_on_page(0, (int) scroll_menu.vsize);
                    my_close_dialog(Inv_dialog, TRUE);
                    return (ev);
                }
                break;
            case '\0': /* finished (commit) */
            case '\n':
            case '\r':
                break;
            case MENU_SELECT_PAGE:
                if (Inv_how == PICK_NONE)
                    goto find_acc;
                if (Inv_how == PICK_ANY)
                    set_all_on_page((int) scroll_menu.vpos,
                                    scroll_menu.vpage);
                break;
            case MENU_SELECT_ALL:
                if (Inv_how == PICK_NONE)
                    goto find_acc;
                if (Inv_how == PICK_ANY)
                    set_all_on_page(0, (int) scroll_menu.vsize);
                break;
            case MENU_UNSELECT_PAGE:
                unset_all_on_page((int) scroll_menu.vpos, scroll_menu.vpage);
                break;
            case MENU_UNSELECT_ALL:
                unset_all_on_page(0, (int) scroll_menu.vsize);
                break;
            case MENU_INVERT_PAGE:
                if (Inv_how == PICK_NONE)
                    goto find_acc;
                if (Inv_how == PICK_ANY)
                    invert_all_on_page((int) scroll_menu.vpos,
                                       scroll_menu.vpage, 0);
                break;
            case MENU_INVERT_ALL:
                if (Inv_how == PICK_NONE)
                    goto find_acc;
                if (Inv_how == PICK_ANY)
                    invert_all_on_page(0, (int) scroll_menu.vsize, 0);
                break;
            case MENU_SEARCH:
                if (Inv_how != PICK_NONE) {
                    char buf[BUFSZ];
                    Gem_getlin("Search for:", buf);
                    if (!*buf || buf[0] == '\033')
                        break;
                    for (it = invent_list; it; it = it->Gmi_next) {
                        if (it->Gmi_identifier && strstr(it->Gmi_str, buf)) {
                            it->Gmi_selected = TRUE;
                            if (Inv_how != PICK_ANY) {
                                my_close_dialog(Inv_dialog, FALSE);
                                break;
                            }
                        }
                    }
                }
                break;
            case C('c'):
                clipbrd_save(invent_list, Anz_inv_lines,
                             xev->ev_mmokstate & K_SHIFT, TRUE);
                break;
            default:
            find_acc:
                if (Inv_how == PICK_NONE)
                    my_close_dialog(Inv_dialog, TRUE);
                else
                    for (it = invent_list; it; it = it->Gmi_next) {
                        if (it->Gmi_identifier
                            && (it->Gmi_accelerator == ch
                                || it->Gmi_groupacc == ch)) {
                            it->Gmi_selected = !it->Gmi_selected;
                            it->Gmi_count = count == 0L ? -1L : count;
                            count = 0L;
                            if (Inv_how != PICK_ANY)
                                my_close_dialog(Inv_dialog, TRUE);
                        }
                    }
                break;
            } /* end switch(ch) */
            if (Inv_how == PICK_ANY) {
                area.g_x = (area.g_x + 23 + 2 * menu_font.cw) & ~7;
                area.g_w = menu_font.cw;
                ob_draw_chg(Inv_dialog, LINESLIST, &area, FAIL);
            }
        } /* !scroll_Inv_dialog */
    }     /* MU_KEYBD */

    if (Inv_how == PICK_ANY) {
        ob_set_text(Inv_dialog->di_tree, QLINE, strCancel);
        for (it = invent_list; it; it = it->Gmi_next)
            if (it->Gmi_identifier && it->Gmi_selected) {
                ob_set_text(Inv_dialog->di_tree, QLINE, strOk);
                break;
            }
        ob_draw_chg(Inv_dialog, QLINE, NULL, FAIL);
    }
    return (ev);
}

/************************* draw_window *******************************/

static void
mar_draw_window(first, win, area)
int first;
WIN *win;
GRECT *area;
{
    OBJECT *obj = (OBJECT *) win->para;

    if (obj) {
        if (first) {
            obj->ob_x = win->work.g_x;
            obj->ob_y = win->work.g_y;
        }
        if (area == NULL)
            area = &(win->work);
        objc_draw(obj, ROOT, MAX_DEPTH, area->g_x, area->g_y, area->g_w,
                  area->g_h);
    }
}

/************************* mar_display_nhwindow
 * *******************************/

void
redraw_winwork(WIN *w, GRECT *area)
{
    area->g_x += w->work.g_x;
    area->g_y += w->work.g_y;
    redraw_window(w, area);
}
void
mar_menu_set_slider(WIN *p_win)
{
    if (p_win) {
        SCROLL *sc = p_win->scroll;

        if (!sc)
            return;

        if (p_win->gadgets & HSLIDE) {
            long hsize = 1000l;

            if (sc->hsize > 0 && sc->hpage > 0) {
                hsize *= sc->hpage;
                hsize /= sc->hsize;
            }
            window_slider(p_win, HOR_SLIDER, 0, (int) hsize);
        }
        if (p_win->gadgets & VSLIDE) {
            long vsize = 1000l;

            if (sc->vsize > 0 && sc->vpage > 0) {
                vsize *= sc->vpage;
                vsize /= sc->vsize;
            }
            window_slider(p_win, VERT_SLIDER, 0, (int) vsize);
        }
    }
}

void
recalc_msg_win(GRECT *area)
{
    OBJECT *z_ob;
    z_ob = zz_oblist[MSGWIN];
    z_ob[MSGLINES].ob_spec.userblk = &ub_msg;
    z_ob[MSGLINES].ob_width = z_ob[ROOT].ob_width =
        (msg_width + 3) * msg_font.cw;
    z_ob[MSGLINES].ob_width -= z_ob[UPMSG].ob_width;
    z_ob[ROOT].ob_height = z_ob[GRABMSGWIN].ob_height =
        z_ob[MSGLINES].ob_height = msg_vis * msg_font.ch;
    z_ob[DNMSG].ob_y = z_ob[GRABMSGWIN].ob_height - z_ob[DNMSG].ob_height;
    window_border(0, 0, 0, z_ob->ob_width, z_ob->ob_height, area);
}
void
recalc_status_win(GRECT *area)
{
    OBJECT *z_ob;
    z_ob = zz_oblist[STATUSLINE];
    z_ob[ROOT].ob_type = G_USERDEF;
    z_ob[ROOT].ob_spec.userblk = &ub_status;
    z_ob[ROOT].ob_width = (status_w + 2) * status_font.cw;
    z_ob[ROOT].ob_height = z_ob[GRABSTATUS].ob_height = 2 * status_font.ch;
    z_ob[GRABSTATUS].ob_width = 2 * status_font.cw - 2;
    window_border(0, 0, 0, z_ob->ob_width, z_ob->ob_height, area);
}
void
calc_std_winplace(int which, GRECT *place)
{
    static int todo = TRUE;
    static GRECT me, ma, st;

    if (todo || which < 0) {
        OBJECT *z_ob;
        int map_h_off, foo;

        /* First the messagewin */
        recalc_msg_win(&me);

        /* Now the map */
        wind_calc(WC_BORDER, MAP_GADGETS, 0, 0,
                  scroll_map.px_hline * (COLNO - 1),
                  scroll_map.px_vline * ROWNO, &foo, &foo, &foo, &map_h_off);
        map_h_off -= scroll_map.px_vline * ROWNO;
        window_border(MAP_GADGETS, 0, 0, scroll_map.px_hline * (COLNO - 1),
                      scroll_map.px_vline * ROWNO, &ma);

        /* Next the statuswin */
        recalc_status_win(&st);

        /* And last but not least a final test */
        ma.g_h = map_h_off + scroll_map.px_vline * ROWNO;
        while (me.g_h + ma.g_h + st.g_h >= desk.g_h)
            ma.g_h -= scroll_map.px_vline;
        /* stack the windows */
        ma.g_y = me.g_y = st.g_y = desk.g_y;
        if (status_align) {
            ma.g_y += st.g_h;
            if (msg_align) {
                st.g_y += me.g_h;
                ma.g_y += me.g_h;
            } else {
                me.g_y += st.g_h + ma.g_h;
            }
        } else {
            if (msg_align) {
                ma.g_y += me.g_h;
            } else {
                me.g_y += ma.g_h;
            }
            st.g_y += me.g_h + ma.g_h;
        }

        if (which)
            todo = FALSE;
    }
    switch (which) {
    case NHW_MESSAGE:
        *place = me;
        break;
    case NHW_MAP:
        *place = ma;
        break;
    case NHW_STATUS:
        *place = st;
        break;
    default:
        break;
    }
}

void
mar_display_nhwindow(wind)
winid wind;
{
    DIAINFO *dlg_info;
    OBJECT *z_ob;
    int d_exit = W_ABANDON, i, breite, mar_di_mode, tmp_magx = magx;
    GRECT g_mapmax, area;
    char *tmp_button;
    struct gw *p_Gw;

    if (wind == WIN_ERR)
        return;

    p_Gw = &Gem_nhwindow[wind];
    switch (p_Gw->gw_type) {
    case NHW_TEXT:
        if (WIN_MESSAGE != WIN_ERR && Gem_nhwindow[WIN_MESSAGE].gw_window)
            mar_display_nhwindow(WIN_MESSAGE);
        z_ob = zz_oblist[LINES];
        scroll_menu.vsize = Anz_text_lines;
        scroll_menu.vpos = 0;
        if (use_rip) {
            if (!depack_img(planes < 4 ? "RIP2.IMG" : "RIP.IMG",
                            &rip_image)) {
                mfdb(&Rip_bild, (int *) rip_image.addr, rip_image.img_w,
                     rip_image.img_h, 1, rip_image.planes);
                transform_img(&Rip_bild);
            }
            ub_lines.ub_code = draw_rip;
        } else
            ub_lines.ub_code = draw_lines;
        z_ob[LINESLIST].ob_spec.userblk = &ub_lines;
        breite = 16;
        v_set_text(text_font.id, text_font.size, BLACK, 0, 0, NULL);
        for (i = 0; i < Anz_text_lines; i++) {
            int eout[8];
            vqt_extent(x_handle, text_lines[i], eout);
            Max(&breite, eout[4]);
        }
        scroll_menu.px_vline = text_font.ch;
        scroll_menu.px_hline = text_font.cw;
        mar_di_mode = mar_set_inv_win(Anz_text_lines, breite);
        tmp_button = ob_get_text(z_ob, QLINE, 0);
        ob_set_text(z_ob, QLINE, strOk);
        ob_undoflag(z_ob, LINESLIST, TOUCHEXIT);
        Event_Handler(Text_Init, Text_Handler);
        if ((dlg_info = open_dialog(z_ob, strText, NULL, NULL,
                                    mar_ob_mapcenter(z_ob), FALSE,
                                    mar_di_mode, FAIL, NULL, NULL)) != NULL) {
            WIN *ptr_win = dlg_info->di_win;

            ptr_win->scroll = &scroll_menu;
            mar_menu_set_slider(ptr_win);
            WindowItems(ptr_win, SCROLL_KEYS, scroll_keys);
            if ((d_exit = X_Form_Do(NULL)) != W_ABANDON) {
                my_close_dialog(dlg_info, FALSE);
                if (d_exit != W_CLOSED)
                    ob_undostate(z_ob, d_exit & NO_CLICK, SELECTED);
            }
        }
        Event_Handler(NULL, NULL);
        ob_set_text(z_ob, QLINE, tmp_button);
        break;
    case NHW_MENU:
        if (WIN_MESSAGE != WIN_ERR && Gem_nhwindow[WIN_MESSAGE].gw_window)
            mar_display_nhwindow(WIN_MESSAGE);
        z_ob = zz_oblist[LINES];
        scroll_menu.vsize = Anz_inv_lines;
        scroll_menu.vpos = 0;
        z_ob[LINESLIST].ob_spec.userblk = &ub_inventory;
        if ((Menu_title)
            && (wind != WIN_INVEN)) /* because I sets no Menu_title */
            Max(&Inv_breite, gr_cw * strlen(Menu_title) + 16);
        scroll_menu.px_vline = menu_font.ch;
        scroll_menu.px_hline = menu_font.cw;
        mar_di_mode = mar_set_inv_win(Anz_inv_lines, Inv_breite, NHW_MENU);
        tmp_button = ob_get_text(z_ob, QLINE, 0);
        ob_set_text(z_ob, QLINE, Inv_how != PICK_NONE ? strCancel : strOk);
        ob_doflag(z_ob, LINESLIST, TOUCHEXIT);
        Event_Handler(Inv_Init, Inv_Handler);
        if ((Inv_dialog =
                 open_dialog(z_ob, (wind == WIN_INVEN)
                                       ? "Inventory"
                                       : (Menu_title ? Menu_title : "Staun"),
                             NULL, NULL, mar_ob_mapcenter(z_ob), FALSE,
                             mar_di_mode, FAIL, NULL, NULL)) != NULL) {
            WIN *ptr_win = Inv_dialog->di_win;

            ptr_win->scroll = &scroll_menu;
            mar_menu_set_slider(ptr_win);
            WindowItems(ptr_win, SCROLL_KEYS, scroll_keys);
            do {
                int y_wo, x_wo, ru_w = 1, ru_h = 1;
                GRECT oarea;
                Gem_menu_item *it;
                d_exit = X_Form_Do(NULL);
                if ((d_exit & NO_CLICK) == LINESLIST) {
                    ob_pos(z_ob, LINESLIST, &oarea);
                    if (mouse(&x_wo, &y_wo) && Inv_how == PICK_ANY) {
                        graf_rt_rubberbox(FALSE, x_wo, y_wo, FAIL, FAIL,
                                          &oarea, &ru_w, &ru_h, NULL);
                        invert_all_on_page(
                            (int) ((y_wo - oarea.g_y) / menu_font.ch
                                   + scroll_menu.vpos),
                            (ru_h + menu_font.ch - 1) / menu_font.ch, 0);
                    } else {
                        for (it = invent_list, i = 0;
                             i < ((y_wo - oarea.g_y) / menu_font.ch
                                  + scroll_menu.vpos)
                             && it;
                             it = it->Gmi_next, i++)
                            ;
                        if (it && it->Gmi_identifier) {
                            it->Gmi_selected = !it->Gmi_selected;
                            it->Gmi_count = count == 0L ? -1L : count;
                            count = 0L;
                            if (Inv_how != PICK_ANY)
                                break;
                        } /* identifier */
                    }
                    oarea.g_x = (oarea.g_x + 23 + 2 * menu_font.cw) & ~7;
                    oarea.g_y = y_wo - (y_wo - oarea.g_y) % menu_font.ch;
                    oarea.g_w = menu_font.cw;
                    oarea.g_h = ((ru_h + menu_font.ch - 1) / menu_font.ch)
                                * menu_font.ch;
                    ob_draw_chg(Inv_dialog, LINESLIST, &oarea, FAIL);
                }
                if (Inv_how == PICK_ANY) {
                    ob_set_text(Inv_dialog->di_tree, QLINE, strCancel);
                    for (it = invent_list; it; it = it->Gmi_next)
                        if (it->Gmi_identifier && it->Gmi_selected) {
                            ob_set_text(Inv_dialog->di_tree, QLINE, strOk);
                            break;
                        }
                    ob_draw_chg(Inv_dialog, QLINE, NULL, FAIL);
                }
            } while ((d_exit & NO_CLICK) == LINESLIST);
            if (d_exit != W_ABANDON) {
                my_close_dialog(Inv_dialog, FALSE);
                if (d_exit != W_CLOSED)
                    ob_undostate(z_ob, d_exit & NO_CLICK, SELECTED);
            }
        }
        Event_Handler(NULL, NULL);
        ob_set_text(z_ob, QLINE, tmp_button);
        break;
    case NHW_MAP:
        if (p_Gw->gw_window == NULL) {
            calc_std_winplace(NHW_MAP, &p_Gw->gw_place);
            window_border(MAP_GADGETS, 0, 0, Tile_width * (COLNO - 1),
                          Tile_heigth * ROWNO, &g_mapmax);
            p_Gw->gw_window = open_window(
                md, md, NULL, zz_oblist[NHICON], MAP_GADGETS, TRUE, 128, 128,
                &g_mapmax, &p_Gw->gw_place, &scroll_map, win_draw_map, NULL,
                XM_TOP | XM_BOTTOM | XM_SIZE);
            WindowItems(p_Gw->gw_window, SCROLL_KEYS - 1,
                        scroll_keys); /* ClrHome centers on u */
            mar_clear_map();
        }
        if (p_Gw->gw_dirty) {
            area.g_x = p_Gw->gw_window->work.g_x
                       + scroll_map.px_hline
                             * (dirty_map_area.g_x - scroll_map.hpos);
            area.g_y = p_Gw->gw_window->work.g_y
                       + scroll_map.px_vline
                             * (dirty_map_area.g_y - scroll_map.vpos);
            area.g_w = (dirty_map_area.g_w - dirty_map_area.g_x + 1)
                       * scroll_map.px_hline;
            area.g_h = (dirty_map_area.g_h - dirty_map_area.g_y + 1)
                       * scroll_map.px_vline;

            redraw_window(p_Gw->gw_window, &area);

            dirty_map_area.g_x = COLNO - 1;
            dirty_map_area.g_y = ROWNO;
            dirty_map_area.g_w = dirty_map_area.g_h = 0;
        }
        break;
    case NHW_MESSAGE:
        if (p_Gw->gw_window == NULL) {
            calc_std_winplace(NHW_MESSAGE, &p_Gw->gw_place);
            z_ob = zz_oblist[MSGWIN];
            magx = 0; /* MAR -- Fake E_GEM to remove Backdropper */
            p_Gw->gw_window = open_window(
                NULL, NULL, NULL, NULL, 0, 0, 0, 0, NULL, &p_Gw->gw_place,
                NULL, mar_draw_window, z_ob, XM_TOP | XM_BOTTOM | XM_SIZE);
            magx = tmp_magx;
            window_size(p_Gw->gw_window, &p_Gw->gw_window->curr);
            p_Gw->gw_dirty = TRUE;
        }

        if (p_Gw->gw_dirty) {
            ob_pos(zz_oblist[MSGWIN], MSGLINES, &area);
            while (messages_pro_zug > 3) {
                messages_pro_zug -= 3;
                msg_pos += 3;
                redraw_window(p_Gw->gw_window, &area);
                mar_more();
            }
            msg_pos += messages_pro_zug;
            messages_pro_zug = 0;
            if (msg_pos > msg_max)
                msg_pos = msg_max;
            redraw_window(p_Gw->gw_window, &area);
            mar_message_pause = FALSE;
        }
        break;
    case NHW_STATUS:
        if (p_Gw->gw_window == NULL) {
            z_ob = zz_oblist[STATUSLINE];
            calc_std_winplace(NHW_STATUS, &p_Gw->gw_place);
            magx = 0; /* MAR -- Fake E_GEM to remove Backdropper */
            p_Gw->gw_window = open_window(
                NULL, NULL, NULL, NULL, 0, FALSE, 0, 0, NULL, &p_Gw->gw_place,
                NULL, mar_draw_window, z_ob, XM_TOP | XM_BOTTOM | XM_SIZE);
            magx = tmp_magx;
            /* Because 2*status_font.ch is smaller then e_gem expects the
             * minimum win_height */
            p_Gw->gw_window->min_h = z_ob[ROOT].ob_height;
            window_size(p_Gw->gw_window, &p_Gw->gw_place);
            p_Gw->gw_dirty = TRUE;
            add_dirty_rect(dr_stat, &p_Gw->gw_place);
        }
        while (get_dirty_rect(dr_stat, &area)) {
            area.g_x = (area.g_x + p_Gw->gw_window->work.g_x
                        + 2 * status_font.cw + 6) & ~7;
            area.g_y += p_Gw->gw_window->work.g_y;
            redraw_window(p_Gw->gw_window, &area);
        }
        break;
    default:
        if (p_Gw->gw_dirty)
            redraw_window(p_Gw->gw_window, NULL);
    }
    p_Gw->gw_dirty = FALSE;
}

/************************* create_window *******************************/

int
mar_hol_win_type(window)
winid window;
{
    return (Gem_nhwindow[window].gw_type);
}

winid
mar_create_window(type)
int type;
{
    winid newid;
    static char name[] = "Gem";
    int i;
    struct gw *p_Gw = &Gem_nhwindow[0];

    for (newid = 0; p_Gw->gw_type && newid < MAXWIN; newid++, p_Gw++)
        ;

    switch (type) {
    case NHW_MESSAGE:
        message_line = (char **) m_alloc(msg_anz * sizeof(char *));
        message_age = (int *) m_alloc(msg_anz * sizeof(int));
        for (i = 0; i < msg_anz; i++) {
            message_age[i] = FALSE;
            message_line[i] = (char *) m_alloc((MSGLEN + 1) * sizeof(char));
            *message_line[i] = 0;
        }
        dr_msg = new_dirty_rect(10);
        if (!dr_msg)
            panic("Memory allocation failure (dr_msg)");
        break;
    case NHW_STATUS:
        status_line = (char **) m_alloc(2 * sizeof(char *));
        for (i = 0; i < 2; i++) {
            status_line[i] = (char *) m_alloc(status_w * sizeof(char));
            memset(status_line[i], 0, status_w);
        }
        dr_stat = new_dirty_rect(10);
        if (!dr_stat)
            panic("Memory allocation failure (dr_stat)");
        break;
    case NHW_MAP:
        map_glyphs = (char **) m_alloc((long) ROWNO * sizeof(char *));
        for (i = 0; i < ROWNO; i++) {
            map_glyphs[i] = (char *) m_alloc((long) COLNO * sizeof(char));
            *map_glyphs[i] = map_glyphs[i][COLNO - 1] = 0;
        }
        dr_map = new_dirty_rect(10);
        if (!dr_map)
            panic("Memory allocation failure (dr_map)");

        mar_clear_map();
        break;
    case NHW_MENU:
    case NHW_TEXT: /* They are no more treated as dialog */
        break;
    default:
        p_Gw->gw_window = open_window(
            "Sonst", name, NULL, NULL, NAME | MOVER | CLOSER, 0, 0, 0, NULL,
            &p_Gw->gw_place, NULL, NULL, NULL, XM_TOP | XM_BOTTOM | XM_SIZE);
        break;
    }

    p_Gw->gw_type = type;

    return (newid);
}

void
mar_change_menu_2_text(win)
winid win;
{
    Gem_nhwindow[win].gw_type = NHW_TEXT;
}

/************************* mar_clear_map *******************************/

void
mar_clear_map()
{
    int pla[8];
    int x, y;

    pla[0] = pla[1] = pla[4] = pla[5] = 0;
    pla[2] = pla[6] = scroll_map.px_hline * (COLNO - 1) - 1;
    pla[3] = pla[7] = scroll_map.px_vline * ROWNO - 1;
    for (y = 0; y < ROWNO; y++)
        for (x = 0; x < COLNO - 1; x++)
            map_glyphs[y][x] = ' ';
    vro_cpyfm(x_handle, ALL_BLACK, pla, &Tile_bilder,
              &Map_bild); /* MAR -- 17.Mar 2002 Hmm, what if FontCol_Bild is
                             bigger? */
    if (WIN_MAP != WIN_ERR && Gem_nhwindow[WIN_MAP].gw_window)
        redraw_window(Gem_nhwindow[WIN_MAP].gw_window, NULL);
}

/************************* destroy_window *******************************/

void
mar_destroy_nhwindow(window)
winid window;
{
    int i;

    switch (Gem_nhwindow[window].gw_type) {
    case NHW_TEXT:
        for (i = 0; i < Anz_text_lines; i++)
            free(text_lines[i]);
        null_free(text_lines);
        Anz_text_lines = 0;
        use_rip = FALSE;
        break;
    case NHW_MENU:
        Gem_start_menu(window, MENU_BEHAVE_STANDARD); /* delete invent_list */
        test_free(Menu_title);
        break;
    case 0: /* No window available, probably an error message? */
        break;
    default:
        close_window(Gem_nhwindow[window].gw_window, 0);
        break;
    }
    Gem_nhwindow[window].gw_window = NULL;
    Gem_nhwindow[window].gw_type = 0;
    Gem_nhwindow[window].gw_dirty = FALSE;

    if (window == WIN_MAP) {
        for (i = 0; i < ROWNO; i++) {
            free(map_glyphs[i]);
        }
        null_free(map_glyphs);
        WIN_MAP = WIN_ERR;
    }
    if (window == WIN_STATUS) {
        for (i = 0; i < 2; i++)
            free(status_line[i]);
        null_free(status_line);
        WIN_STATUS = WIN_ERR;
    }
    if (window == WIN_MESSAGE) {
        for (i = 0; i < msg_anz; i++)
            free(message_line[i]);
        null_free(message_line);
        null_free(message_age);
        WIN_MESSAGE = WIN_ERR;
    }
    if (window == WIN_INVEN)
        WIN_INVEN = WIN_ERR;
}

/************************* nh_poskey *******************************/

void
mar_set_margin(int m)
{
    Max(&m, 0);
    Min(&m,
        min(ROWNO, COLNO)); /* MAR 16.Mar 2002 -- the larger the less sense */
    scroll_margin = m;
}
void
mar_cliparound()
{
    if (WIN_MAP != WIN_ERR && Gem_nhwindow[WIN_MAP].gw_window) {
        int breite = scroll_margin > 0 ? scroll_margin
                                       : max(scroll_map.hpage / 4, 1),
            hoehe = scroll_margin > 0 ? scroll_margin
                                      : max(scroll_map.vpage / 4, 1),
            adjust_needed;
        adjust_needed = FALSE;
        if ((map_cursx < scroll_map.hpos + breite)
            || (map_cursx >= scroll_map.hpos + scroll_map.hpage - breite)) {
            scroll_map.hpos = map_cursx - scroll_map.hpage / 2;
            adjust_needed = TRUE;
        }
        if ((map_cursy < scroll_map.vpos + hoehe)
            || (map_cursy >= scroll_map.vpos + scroll_map.vpage - hoehe)) {
            scroll_map.vpos = map_cursy - scroll_map.vpage / 2;
            adjust_needed = TRUE;
        }
        if (adjust_needed)
            scroll_window(Gem_nhwindow[WIN_MAP].gw_window, WIN_SCROLL, NULL);
    }
}

void
mar_update_value()
{
    if (WIN_MESSAGE != WIN_ERR) {
        mar_message_pause = FALSE;
        mar_esc_pressed = FALSE;
        mar_display_nhwindow(WIN_MESSAGE);
    }

    if (WIN_MAP != WIN_ERR)
        mar_cliparound();

    if (WIN_STATUS != WIN_ERR) {
        mar_check_hilight_status();
        mar_display_nhwindow(WIN_STATUS);
    }
}

int
Main_Init(xev, availiable)
XEVENT *xev;
int availiable;
{
    xev->ev_mb1mask = xev->ev_mb1state = 1;
    xev->ev_mb1clicks = xev->ev_mb2clicks = xev->ev_mb2mask =
        xev->ev_mb2state = 2;
    return ((MU_KEYBD | MU_BUTTON1 | MU_BUTTON2 | MU_MESAG) & availiable);
}

/*
 * return a key, or 0, in which case a mouse button was pressed
 * mouse events should be returned as character postitions in the map window.
 */
/*ARGSUSED*/
int
mar_nh_poskey(x, y, mod)
int *x, *y, *mod;
{
    static XEVENT xev;
    int retval, ev;

    xev.ev_mflags = Main_Init(&xev, 0xFFFF);
    ev = Event_Multi(&xev);

    retval = FAIL;

    if (ev & MU_KEYBD) {
        char ch = xev.ev_mkreturn & 0x00FF;
        char scan = (xev.ev_mkreturn & 0xff00) >> 8;
        int shift = xev.ev_mmokstate;
        const struct pad *kpad;

        /* Translate keypad keys */
        if (iskeypad(scan)) {
            kpad = mar_iflags_numpad() == 1 ? numpad : keypad;
            if (shift & K_SHIFT)
                ch = kpad[scan - KEYPADLO].shift;
            else if (shift & K_CTRL) {
                if (scan >= 0x67 && scan <= 0x6f && scan != 0x6b) {
                    send_key(kpad[scan - KEYPADLO].normal);
                    ch = 'g';
                } else {
                    ch = kpad[scan - KEYPADLO].cntrl;
                }
            } else
                ch = kpad[scan - KEYPADLO].normal;
        }
        if (scan == SCANHOME)
            mar_cliparound();
        else if (scan == SCANF1)
            retval = 'h';
        else if (scan == SCANF2) {
            mar_set_tile_mode(!mar_set_tile_mode(FAIL));
            retval = C('l'); /* trigger full-redraw */
        } else if (scan == SCANF3) {
            draw_cursor = !draw_cursor;
            mar_curs(map_cursx, map_cursy);
            mar_display_nhwindow(WIN_MAP);
        } else if (scan == SCANF4) { /* Font-Selector */
            if (!CallFontSelector(0, FAIL, FAIL, FAIL, FAIL)) {
                xalert(1, 1, X_ICN_ALERT, NULL, SYS_MODAL, BUTTONS_RIGHT,
                       TRUE, "Hello", "Fontselector not available!", NULL);
            }
        } else if (!ch && shift & K_CTRL && scan == -57) {
            /* MAR -- nothing ignore Ctrl-Alt-Clr/Home == MagiC's restore
             * screen */
        } else {
            if (!ch)
                ch = (char) M(tolower(scan_2_ascii(xev.ev_mkreturn, shift)));
            if (((int) ch) == -128)
                ch = '\033';
            retval = ch;
        }
    }

    if (ev & MU_BUTTON1 || ev & MU_BUTTON2) {
        int ex = xev.ev_mmox, ey = xev.ev_mmoy;
        WIN *akt_win = window_find(ex, ey);

        if (WIN_MAP != WIN_ERR
            && akt_win == Gem_nhwindow[WIN_MAP].gw_window) {
            *x = max(min((ex - akt_win->work.g_x) / scroll_map.px_hline
                             + scroll_map.hpos,
                         COLNO - 1),
                     0) + 1;
            *y = max(min((ey - akt_win->work.g_y) / scroll_map.px_vline
                             + scroll_map.vpos,
                         ROWNO),
                     0);
            *mod = xev.ev_mmobutton;
            retval = 0;
        } else if (WIN_STATUS != WIN_ERR
                   && akt_win == Gem_nhwindow[WIN_STATUS].gw_window) {
            move_win(akt_win);
        } else if (WIN_MESSAGE != WIN_ERR
                   && akt_win == Gem_nhwindow[WIN_MESSAGE].gw_window) {
            message_handler(ex, ey);
        }
    }

    if (ev & MU_MESAG) {
        int *buf = xev.ev_mmgpbuf;
        char *str;
        OBJECT *z_ob = zz_oblist[MENU];

        switch (*buf) {
        case MN_SELECTED:
            menu_tnormal(z_ob, buf[3], TRUE); /* unselect menu header */
            str = ob_get_text(z_ob, buf[4], 0);
            str += strlen(str) - 2;
            switch (*str) {
            case ' ': /* just that command */
                retval = str[1];
                break;
            case '\005': /* Alt command */
            case '\007':
                retval = M(str[1]);
                break;
            case '^': /* Ctrl command */
                retval = C(str[1]);
                break;
            case 'f': /* Func Key */
                switch (str[1]) {
                case '1':
                    retval = 'h';
                    break;
                case '2':
                    mar_set_tile_mode(!mar_set_tile_mode(FAIL));
                    retval = C('l'); /* trigger full-redraw */
                    break;
                case '3':
                    draw_cursor = !draw_cursor;
                    mar_curs(map_cursx, map_cursy);
                    mar_display_nhwindow(WIN_MAP);
                    break;
                default:
                }
                break;
            default:
                mar_about();
                break;
            }
            break; /* MN_SELECTED */
        case WM_CLOSED:
            WindowHandler(W_ICONIFYALL, NULL, NULL);
            break;
        case AP_TERM:
            retval = 'S';
            break;
        case FONT_CHANGED:
            if (buf[3] >= 0) {
                if (buf[3] == Gem_nhwindow[WIN_MESSAGE].gw_window->handle) {
                    mar_set_fontbyid(NHW_MESSAGE, buf[4], buf[5]);
                    mar_display_nhwindow(WIN_MESSAGE);
                } else if (buf[3]
                           == Gem_nhwindow[WIN_MAP].gw_window->handle) {
                    mar_set_fontbyid(NHW_MAP, buf[4], buf[5]);
                    mar_display_nhwindow(WIN_MAP);
                } else if (buf[3]
                           == Gem_nhwindow[WIN_STATUS].gw_window->handle) {
                    mar_set_fontbyid(NHW_STATUS, buf[4], buf[5]);
                    mar_display_nhwindow(WIN_STATUS);
                }
                FontAck(buf[1], 1);
            }
            break;
        default:
            break;
        }
    } /* MU_MESAG */

    if (retval == FAIL)
        retval = mar_nh_poskey(x, y, mod);

    return (retval);
}

int
Gem_nh_poskey(x, y, mod)
int *x, *y, *mod;
{
    mar_update_value();
    return (mar_nh_poskey(x, y, mod));
}

void
Gem_delay_output()
{
    Event_Timer(50, 0, FALSE); /* wait 50ms */
}

int
Gem_doprev_message()
{
    if (msg_pos > 2) {
        msg_pos--;
        if (WIN_MESSAGE != WIN_ERR)
            Gem_nhwindow[WIN_MESSAGE].gw_dirty = TRUE;
        mar_display_nhwindow(WIN_MESSAGE);
    }
    return (0);
}

/************************* print_glyph *******************************/

int mar_set_rogue(int);

int
mar_set_tile_mode(tiles)
int tiles;
{
    static int tile_mode = TRUE;
    static GRECT prev;
    WIN *z_w = WIN_MAP != WIN_ERR ? Gem_nhwindow[WIN_MAP].gw_window : NULL;

    if (tiles < 0)
        return (tile_mode);
    else if (!z_w)
        tile_mode = tiles;
    else if (tile_mode == tiles || (mar_set_rogue(FAIL) && tiles))
        return (FAIL);
    else {
        GRECT tmp;

        tile_mode = tiles;
        scroll_map.px_hline = tiles ? Tile_width : map_font.cw;
        scroll_map.px_vline = tiles ? Tile_heigth : map_font.ch;
        window_border(MAP_GADGETS, 0, 0, scroll_map.px_hline * (COLNO - 1),
                      scroll_map.px_vline * ROWNO, &tmp);
        z_w->max.g_w = tmp.g_w;
        z_w->max.g_h = tmp.g_h;
        if (tiles)
            z_w->curr = prev;
        else
            prev = z_w->curr;

        window_reinit(z_w, md, md, NULL, FALSE, FALSE);
    }
    return (FAIL);
}

int
mar_set_rogue(what)
int what;
{
    static int rogue = FALSE, prev_mode = TRUE;

    if (what < 0)
        return (rogue);
    if (what != rogue) {
        rogue = what;
        if (rogue) {
            prev_mode = mar_set_tile_mode(FAIL);
            mar_set_tile_mode(FALSE);
        } else
            mar_set_tile_mode(prev_mode);
    }
    return (FAIL);
}

void
mar_add_pet_sign(window, x, y)
winid window;
int x, y;
{
    if (window != WIN_ERR && window == WIN_MAP) {
        static int pla[8] = { 0, 0, 7, 7, 0, 0, 0, 0 },
                   colindex[2] = { RED, WHITE };

        pla[4] = pla[6] = scroll_map.px_hline * x;
        pla[5] = pla[7] = scroll_map.px_vline * y;
        pla[6] += 7;
        pla[7] += 6;
        vrt_cpyfm(x_handle, MD_TRANS, pla, &Pet_Mark, &Map_bild, colindex);
    }
}

void
mar_print_glyph(window, x, y, gl, bkgl)
winid window;
int x, y, gl, bkgl;
{
    if (window != WIN_ERR && window == WIN_MAP) {
        static int pla[8];

        pla[2] = pla[0] = (gl % Tiles_per_line) * Tile_width;
        pla[3] = pla[1] = (gl / Tiles_per_line) * Tile_heigth;
        pla[2] += Tile_width - 1;
        pla[3] += Tile_heigth - 1;
        pla[6] = pla[4] = Tile_width * x;  /* x_wert to */
        pla[7] = pla[5] = Tile_heigth * y; /* y_wert to */
        pla[6] += Tile_width - 1;
        pla[7] += Tile_heigth - 1;

        vro_cpyfm(x_handle, gl != -1 ? S_ONLY : ALL_BLACK, pla, &Tile_bilder,
                  &Map_bild);
    }
}

void
mar_print_char(window, x, y, ch, col)
winid window;
int x, y;
char ch;
int col;
{
    if (window != WIN_ERR && window == WIN_MAP) {
        static int gem_color[16] = { 9, 2,  11, 10, 4, 7,  8,  15,
                                     0, 14, 3,  6,  5, 13, 15, 0 };
        int pla[8], colindex[2];

        map_glyphs[y][x] = ch;

        pla[0] = pla[1] = 0;
        pla[2] = map_font.cw - 1;
        pla[3] = map_font.ch - 1;
        pla[6] = pla[4] = map_font.cw * x;
        pla[7] = pla[5] = map_font.ch * y;
        pla[6] += map_font.cw - 1;
        pla[7] += map_font.ch - 1;
        colindex[0] = gem_color[col];
        colindex[1] = WHITE;
        vrt_cpyfm(x_handle, MD_REPLACE, pla, &Black_bild, &FontCol_Bild,
                  colindex);
    }
}

/************************* getlin *******************************/

void
Gem_getlin(ques, input)
const char *ques;
char *input;
{
    OBJECT *z_ob = zz_oblist[LINEGET];
    int d_exit, length;
    char *pr[2], *tmp;

    if (WIN_MESSAGE != WIN_ERR && Gem_nhwindow[WIN_MESSAGE].gw_window)
        mar_display_nhwindow(WIN_MESSAGE);

    z_ob[LGPROMPT].ob_type = G_USERDEF;
    z_ob[LGPROMPT].ob_spec.userblk = &ub_prompt;
    z_ob[LGPROMPT].ob_height = 2 * gr_ch;

    length = z_ob[LGPROMPT].ob_width / gr_cw;
    if (strlen(ques) > length) {
        tmp = ques + length;
        while (*tmp != ' ' && tmp >= ques) {
            tmp--;
        }
        if (tmp <= ques)
            tmp = ques + length; /* Mar -- Oops, what a word :-) */
        pr[0] = ques;
        *tmp = 0;
        pr[1] = ++tmp;
    } else {
        pr[0] = ques;
        pr[1] = NULL;
    }
    ub_prompt.ub_parm = (long) pr;

    ob_clear_edit(z_ob);
    d_exit = xdialog(z_ob, nullstr, NULL, NULL, mar_ob_mapcenter(z_ob), FALSE,
                     DIALOG_MODE);
    Event_Timer(0, 0, TRUE);

    if (d_exit == W_CLOSED || d_exit == W_ABANDON
        || (d_exit & NO_CLICK) == QLG) {
        *input = '\033';
        input[1] = 0;
    } else
        strncpy(input, ob_get_text(z_ob, LGREPLY, 0), length);
}

/************************* ask_direction *******************************/

#define Dia_Init K_Init

int
Dia_Handler(xev)
XEVENT *xev;
{
    int ev = xev->ev_mwich;
    char ch = (char) (xev->ev_mkreturn & 0x00FF);

    if (ev & MU_KEYBD) {
        WIN *w;
        DIAINFO *dinf;

        switch (ch) {
        case 's':
            send_key((int) (mar_iflags_numpad() ? '5' : '.'));
            break;
        case '.':
            send_key('5'); /* MAR -- '.' is a button if numpad isn't set */
            break;
        case '\033': /*ESC*/
            if ((w = get_top_window()) && (dinf = (DIAINFO *) w->dialog)
                && dinf->di_tree == zz_oblist[DIRECTION]) {
                my_close_dialog(dinf, FALSE);
                break;
            }
        /* Fall thru */
        default:
            ev &= ~MU_KEYBD; /* let the dialog handle it */
            break;
        }
    }
    return (ev);
}

int
mar_ask_direction()
{
    int d_exit;
    OBJECT *z_ob = zz_oblist[DIRECTION];

    Event_Handler(Dia_Init, Dia_Handler);
    mar_set_dir_keys();
    d_exit = xdialog(z_ob, nullstr, NULL, NULL, mar_ob_mapcenter(z_ob), FALSE,
                     DIALOG_MODE);
    Event_Timer(0, 0, TRUE);
    Event_Handler(NULL, NULL);

    if (d_exit == W_CLOSED || d_exit == W_ABANDON)
        return ('\033');
    if ((d_exit & NO_CLICK) == DIRDOWN)
        return ('>');
    if ((d_exit & NO_CLICK) == DIRUP)
        return ('<');
    if ((d_exit & NO_CLICK) == (DIR1 + 8)) /* 5 or . */
        return ('.');
    return (*ob_get_text(z_ob, d_exit & NO_CLICK, 0));
}

/************************* yn_function *******************************/

#define any_init M_Init

static int
any_handler(xev)
XEVENT *xev;
{
    int ev = xev->ev_mwich;

    if (ev & MU_MESAG) {
        int *buf = xev->ev_mmgpbuf;

        if (*buf == OBJC_EDITED)
            my_close_dialog(*(DIAINFO **) &buf[4], FALSE);
        else
            ev &= ~MU_MESAG;
    }
    return (ev);
}

int
send_yn_esc(char ch)
{
    static char esc_char = 0;

    if (ch < 0) {
        if (esc_char) {
            send_key((int) esc_char);
            return (TRUE);
        }
        return (FALSE);
    } else
        esc_char = ch;
    return (TRUE);
}

#define single_init K_Init

static int
single_handler(xev)
XEVENT *xev;
{
    int ev = xev->ev_mwich;

    if (ev & MU_KEYBD) {
        char ch = (char) xev->ev_mkreturn & 0x00FF;
        WIN *w;
        DIAINFO *dinf;

        switch (ch) {
        case ' ':
            send_return();
            break;
        case '\033':
            if ((w = get_top_window()) && (dinf = (DIAINFO *) w->dialog)
                && dinf->di_tree == zz_oblist[YNCHOICE]) {
                if (!send_yn_esc(FAIL))
                    my_close_dialog(dinf, FALSE);
                break;
            }
        /* Fall thru */
        default:
            ev &= ~MU_MESAG;
        }
    }
    return (ev);
}

char
Gem_yn_function(query, resp, def)
const char *query, *resp;
char def;
{
    OBJECT *z_ob = zz_oblist[YNCHOICE];
    int d_exit, i, len;
    long anzahl;
    char *tmp;
    const char *ptr;

    if (WIN_MESSAGE != WIN_ERR && Gem_nhwindow[WIN_MESSAGE].gw_window)
        mar_display_nhwindow(WIN_MESSAGE);

    /* if query for direction the special dialog */
    if (strstr(query, "irect"))
        return (mar_ask_direction());

    len = min(strlen(query), (max_w - 8 * gr_cw) / gr_cw);
    z_ob[ROOT].ob_width = (len + 8) * gr_cw;
    z_ob[YNPROMPT].ob_width = gr_cw * len + 8;
    tmp = ob_get_text(z_ob, YNPROMPT, 0);
    ob_set_text(z_ob, YNPROMPT, mar_copy_of(query));

    if (resp) { /* single inputs */
        ob_hide(z_ob, SOMECHARS, FALSE);
        ob_hide(z_ob, ANYCHAR, TRUE);

        if (strchr(resp, 'q'))
            send_yn_esc('q');
        else if (strchr(resp, 'n'))
            send_yn_esc('n');
        else
            send_yn_esc(
                def); /* strictly def should be returned, but in trad. I it's
                         0 */

        if (strchr(resp, '#')) { /* count possible */
            ob_hide(z_ob, YNOK, FALSE);
            ob_hide(z_ob, COUNT, FALSE);
        } else { /* no count */
            ob_hide(z_ob, YNOK, TRUE);
            ob_hide(z_ob, COUNT, TRUE);
        }

        if ((anzahl = (long) strchr(resp, '\033'))) {
            anzahl -= (long) resp;
        } else {
            anzahl = strlen(resp);
        }
        for (i = 0, ptr = resp; i < 2 * anzahl; i += 2, ptr++) {
            ob_hide(z_ob, YN1 + i, FALSE);
            mar_change_button_char(z_ob, YN1 + i, *ptr);
            ob_undoflag(z_ob, YN1 + i, DEFAULT);
            if (*ptr == def)
                ob_doflag(z_ob, YN1 + i, DEFAULT);
        }

        z_ob[SOMECHARS].ob_width = z_ob[YN1 + i].ob_x + 8;
        z_ob[SOMECHARS].ob_height = z_ob[YN1 + i].ob_y + gr_ch + gr_ch / 2;
        Max((int *) &z_ob[ROOT].ob_width,
            z_ob[SOMECHARS].ob_width + 4 * gr_cw);
        z_ob[ROOT].ob_height = z_ob[SOMECHARS].ob_height + 4 * gr_ch;
        if (strchr(resp, '#'))
            z_ob[ROOT].ob_height = z_ob[YNOK].ob_y + 2 * gr_ch;

        for (i += YN1; i < (YNN + 1); i += 2) {
            ob_hide(z_ob, i, TRUE);
        }
        Event_Handler(single_init, single_handler);
    } else { /* any input */
        ob_hide(z_ob, SOMECHARS, TRUE);
        ob_hide(z_ob, ANYCHAR, FALSE);
        ob_hide(z_ob, YNOK, TRUE);
        ob_hide(z_ob, COUNT, TRUE);
        z_ob[ANYCHAR].ob_height = 2 * gr_ch;
        z_ob[CHOSENCH].ob_y = z_ob[CHOSENCH + 1].ob_y = gr_ch / 2;
        z_ob[ROOT].ob_width =
            max(z_ob[YNPROMPT].ob_width + z_ob[YNPROMPT].ob_x,
                z_ob[ANYCHAR].ob_width + z_ob[ANYCHAR].ob_x) + 2 * gr_cw;
        z_ob[ROOT].ob_height =
            z_ob[ANYCHAR].ob_height + z_ob[ANYCHAR].ob_y + gr_ch / 2;
        *ob_get_text(z_ob, CHOSENCH, 0) = '?';
        Event_Handler(any_init, any_handler);
    }

    d_exit = xdialog(z_ob, nullstr, NULL, NULL, mar_ob_mapcenter(z_ob), FALSE,
                     DIALOG_MODE);
    Event_Timer(0, 0, TRUE);
    Event_Handler(NULL, NULL);
    /* display of count is missing (through the core too) */

    free(ob_get_text(z_ob, YNPROMPT, 0));
    ob_set_text(z_ob, YNPROMPT, tmp);

    if (resp && (d_exit == W_CLOSED || d_exit == W_ABANDON))
        return ('\033');
    if ((d_exit & NO_CLICK) == YNOK) {
        yn_number = atol(ob_get_text(z_ob, COUNT, 0));
        return ('#');
    }
    if (!resp)
        return (*ob_get_text(z_ob, CHOSENCH, 0));
    return (*ob_get_text(z_ob, d_exit & NO_CLICK, 0));
}

/*
 * Allocate a copy of the given string.  If null, return a string of
 * zero length.
 *
 * This is an exact duplicate of copy_of() in X11/winmenu.c.
 */
static char *
mar_copy_of(s)
const char *s;
{
    if (!s)
        s = nullstr;
    return strcpy((char *) m_alloc((unsigned) (strlen(s) + 1)), s);
}

const char *strRP = "raw_print", *strRPB = "raw_print_bold";

void
mar_raw_print(str)
const char *str;
{
    xalert(1, FAIL, X_ICN_INFO, NULL, APPL_MODAL, BUTTONS_CENTERED, TRUE,
           strRP, str, NULL);
}

void
mar_raw_print_bold(str)
const char *str;
{
    char buf[BUFSZ];

    sprintf(buf, "!%s", str);
    xalert(1, FAIL, X_ICN_INFO, NULL, APPL_MODAL, BUTTONS_CENTERED, TRUE,
           strRPB, buf, NULL);
}

/*wingem1.c*/
