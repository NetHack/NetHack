/* NetHack 3.6	mhtxtbuf.c	$NHDT-Date: 1432512803 2015/05/25 00:13:23 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $ */
/* Copyright (C) 2003 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

#include "mhtxtbuf.h"

/* Collect Nethack text messages and render text into edit box.
   Wrap text if necessary.
   Recognize formatted lines as having more that 4 consecutive.
   spaces inside the string.
   Strip leading and trailing spaces.
   Always break at the original line end (do not merge text that comes
   from NetHack engine)
*/

/*----------------------------------------------------------------*/
#define NHTEXT_BUFFER_INCREMENT 10
/*----------------------------------------------------------------*/
struct text_buffer_line {
    int attr;
    short beg_padding;
    short end_padding;
    BOOL formatted;
    char *text;
};
/*----------------------------------------------------------------*/
typedef struct mswin_nethack_text_buffer {
    BOOL b_wrap_text;
    int n_size;
    int n_used;
    struct text_buffer_line *text_buffer_line;
} NHTextBuffer, *PNHTextBuffer;
/*----------------------------------------------------------------*/
#define NHTextLine(pb, i) ((pb)->text_buffer_line[(i)])
static TCHAR *nh_append(TCHAR *s, int *size, const char *ap);
/*----------------------------------------------------------------*/
PNHTextBuffer
mswin_init_text_buffer(BOOL wrap_text)
{
    PNHTextBuffer pb = (PNHTextBuffer) malloc(sizeof(NHTextBuffer));
    if (!pb)
        panic("Out of memory");

    ZeroMemory(pb, sizeof(NHTextBuffer));
    pb->b_wrap_text = wrap_text;
    pb->n_size = 0;
    pb->n_used = 0;
    pb->text_buffer_line = NULL;
    return pb;
}
/*----------------------------------------------------------------*/
void
mswin_free_text_buffer(PNHTextBuffer pb)
{
    int i;

    if (!pb)
        return;

    for (i = 0; i < pb->n_used; i++) {
        free(pb->text_buffer_line[i].text);
    }
    free(pb->text_buffer_line);
    free(pb);
}
/*----------------------------------------------------------------*/
void
mswin_add_text(PNHTextBuffer pb, int attr, const char *text)
{
    char *p;
    struct text_buffer_line *new_line;

    /* grow buffer */
    if (pb->n_used >= pb->n_size) {
        pb->n_size += NHTEXT_BUFFER_INCREMENT;
        pb->text_buffer_line = (struct text_buffer_line *) realloc(
            pb->text_buffer_line,
            pb->n_size * sizeof(struct text_buffer_line));
        if (!pb->text_buffer_line)
            panic("Memory allocation error");
    }

    /* analyze the new line of text */
    new_line = &NHTextLine(pb, pb->n_used);
    new_line->attr = attr;
    new_line->beg_padding = 0;
    new_line->text = strdup(text);
    for (p = new_line->text; *p && isspace(*p); p++) {
        new_line->beg_padding++;
    }
    if (*p) {
        memmove(new_line->text, new_line->text + new_line->beg_padding,
                strlen(new_line->text) - new_line->beg_padding + 1);
        for (p = new_line->text + strlen(new_line->text);
             p >= new_line->text && isspace(*p); p--) {
            new_line->end_padding++;
            *p = 0;
        }

        /* if there are 3 (or more) consecutive spaces inside the string
           consider it formatted */
        new_line->formatted = (strstr(new_line->text, "   ") != NULL)
                              || (new_line->beg_padding > 8);
    } else {
        new_line->end_padding = 0;
        new_line->text[0] = 0;
        new_line->formatted = FALSE;
    }
    pb->n_used++;
}
/*----------------------------------------------------------------*/
void
mswin_set_text_wrap(PNHTextBuffer pb, BOOL wrap_text)
{
    pb->b_wrap_text = wrap_text;
}
/*----------------------------------------------------------------*/
BOOL
mswin_get_text_wrap(PNHTextBuffer pb)
{
    return pb->b_wrap_text;
}
/*----------------------------------------------------------------*/
static TCHAR *
nh_append(TCHAR *s, int *size, const char *ap)
{
    int tlen, tnewlen;

    if (!(ap && *ap))
        return s;

    /* append the calculated line to the text buffer */
    tlen = s ? _tcslen(s) : 0;
    tnewlen = tlen + strlen(ap);
    if (tnewlen >= *size) {
        *size = max(tnewlen, *size + BUFSZ);
        s = (TCHAR *) realloc(s, *size * sizeof(TCHAR));
        if (!s)
            panic("Out of memory");
        ZeroMemory(s + tlen, (*size - tlen) * sizeof(TCHAR));
    }
    if (strcmp(ap, "\r\n") == 0) {
        _tcscat(s, TEXT("\r\n"));
    } else {
        NH_A2W(ap, s + tlen, strlen(ap));
        s[tnewlen] = 0;
    }
    return s;
}
/*----------------------------------------------------------------*/
void
mswin_render_text(PNHTextBuffer pb, HWND edit_control)
{
    RECT rt_client;    /* boundaries of the client area of the edit control */
    SIZE size_text;    /* size of the edit control */
    RECT rt_text;      /* calculated text rectangle for the visible line */
    char buf[BUFSZ];   /* buffer for the visible line */
    TCHAR tbuf[BUFSZ]; /* temp buffer for DrawText */
    TCHAR *pText = NULL;    /* resulting text (formatted) */
    int pTextSize = 0;      /* resulting text size */
    char *p_cur = NULL;     /* current position in the
                               NHTextBuffer->text_buffer_line->text */
    char *p_buf_cur = NULL; /* current position in the visible line buffer */
    int i;
    HDC hdcEdit;           /* device context for the edit control */
    HFONT hFont, hOldFont; /* edit control font */

    GetClientRect(edit_control, &rt_client);
    size_text.cx = rt_client.right - rt_client.left;
    size_text.cy = rt_client.bottom - rt_client.top;
    size_text.cx -=
        GetSystemMetrics(SM_CXVSCROLL); /* add a slight right margin - the
                                           text looks better that way */
    hdcEdit = GetDC(edit_control);
    hFont = (HFONT) SendMessage(edit_control, WM_GETFONT, 0, 0);
    if (hFont)
        hOldFont = SelectObject(hdcEdit, hFont);

    /* loop through each line (outer loop) and wrap it around (inner loop) */
    ZeroMemory(buf, sizeof(buf));
    p_buf_cur = buf;
    for (i = 0; i < pb->n_used; i++) {
        if (pb->b_wrap_text) {
            p_cur = NHTextLine(pb, i).text;

            /* insert an line break for the empty string */
            if (!NHTextLine(pb, i).text[0]) {
                pText = nh_append(pText, &pTextSize, "\r\n");
                continue;
            }

            /* add margin to the "formatted" line of text */
            if (NHTextLine(pb, i).formatted) {
                strcpy(buf, "   ");
                p_buf_cur += 3;
            }

            /* scroll thourgh the current line of text and wrap it
               so it fits to width of the edit control */
            while (*p_cur) {
                char *p_word_pos = p_buf_cur;

                /* copy one word into the buffer */
                while (*p_cur && isspace(*p_cur))
                    if (p_buf_cur != buf)
                        *p_buf_cur++ = *p_cur++;
                    else
                        p_cur++;

                while (*p_cur && !isspace(*p_cur))
                    *p_buf_cur++ = *p_cur++;

                /* check if it fits */
                SetRect(&rt_text, 0, 0, size_text.cx, size_text.cy);
                DrawText(hdcEdit, NH_A2W(buf, tbuf, p_buf_cur - buf),
                         p_buf_cur - buf, &rt_text,
                         DT_CALCRECT | DT_LEFT | DT_SINGLELINE | DT_NOCLIP);
                if ((rt_text.right - rt_text.left) >= size_text.cx) {
                    /* Backtrack.
                       Only backtrack if the last word caused the overflow -
                       do not backtrack if the entire current line does not
                       fit the visible area.
                       Otherwise it is a infinite loop.
                    */
                    if (p_word_pos > buf) {
                        p_cur -= (p_buf_cur - p_word_pos);
                        p_buf_cur = p_word_pos;
                    }
                    *p_buf_cur = 0; /* break the line */

                    /* append the calculated line to the text buffer */
                    pText = nh_append(pText, &pTextSize, buf);
                    pText = nh_append(pText, &pTextSize, "\r\n");
                    ZeroMemory(buf, sizeof(buf));
                    p_buf_cur = buf;
                }
            }

            /* always break the line at the end of the buffer text */
            if (p_buf_cur != buf) {
                /* flush the current buffrer */
                *p_buf_cur = 0; /* break the line */
                pText = nh_append(pText, &pTextSize, buf);
                pText = nh_append(pText, &pTextSize, "\r\n");
                ZeroMemory(buf, sizeof(buf));
                p_buf_cur = buf;
            }
        } else { /* do not wrap text */
            int j;
            for (j = 0; j < NHTextLine(pb, i).beg_padding; j++)
                pText = nh_append(pText, &pTextSize, " ");
            pText = nh_append(pText, &pTextSize, NHTextLine(pb, i).text);
            pText = nh_append(pText, &pTextSize, "\r\n");
        }
    }

    /* cleanup */
    if (hFont)
        SelectObject(hdcEdit, hOldFont);
    ReleaseDC(edit_control, hdcEdit);

    /* update edit control text */
    if (pText) {
        SendMessage(edit_control, EM_FMTLINES, 1, 0);
        SetWindowText(edit_control, pText);
        free(pText);
    }
}
/*----------------------------------------------------------------*/
