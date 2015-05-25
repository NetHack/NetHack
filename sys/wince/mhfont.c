/* NetHack 3.6	mhfont.c	$NHDT-Date: 1432512800 2015/05/25 00:13:20 $  $NHDT-Branch: master $:$NHDT-Revision: 1.18 $ */
/* Copyright (C) 2001 by Alex Kompel 	 */
/* NetHack may be freely redistributed.  See license for details. */

/* font management and such */

#include "mhfont.h"

#define MAXFONTS 64

/* font table - 64 fonts ought to be enough */
static struct font_table_entry {
    int code;
    HFONT hFont;
} font_table[MAXFONTS];
static int font_table_size = 0;
HFONT version_splash_font;
HFONT extrainfo_splash_font;

#define NHFONT_CODE(win, attr) (((attr & 0xFF) << 8) | (win_type & 0xFF))

static void __cdecl font_table_cleanup(void);

/* create font based on window type, charater attributes and
   window device context */
HGDIOBJ
mswin_get_font(int win_type, int attr, HDC hdc, BOOL replace)
{
    HFONT fnt = NULL;
    LOGFONT lgfnt;
    int font_size;
    int font_index;
    static BOOL once = FALSE;

    if (!once) {
        once = TRUE;
        atexit(font_table_cleanup);
    }

    ZeroMemory(&lgfnt, sizeof(lgfnt));

    /* try find font in the table */
    for (font_index = 0; font_index < font_table_size; font_index++)
        if (NHFONT_CODE(win_type, attr) == font_table[font_index].code)
            break;

    if (!replace && font_index < font_table_size)
        return font_table[font_index].hFont;

    switch (win_type) {
    case NHW_STATUS:
        lgfnt.lfHeight = -iflags.wc_fontsiz_status
                         * GetDeviceCaps(hdc, LOGPIXELSY)
                         / 72;             // height of font
        lgfnt.lfWidth = 0;                 // average character width
        lgfnt.lfEscapement = 0;            // angle of escapement
        lgfnt.lfOrientation = 0;           // base-line orientation angle
        lgfnt.lfWeight = FW_NORMAL;        // font weight
        lgfnt.lfItalic = FALSE;            // italic attribute option
        lgfnt.lfUnderline = FALSE;         // underline attribute option
        lgfnt.lfStrikeOut = FALSE;         // strikeout attribute option
        lgfnt.lfCharSet = mswin_charset(); // character set identifier
        lgfnt.lfOutPrecision = OUT_DEFAULT_PRECIS;   // output precision
        lgfnt.lfClipPrecision = CLIP_DEFAULT_PRECIS; // clipping precision
        lgfnt.lfQuality = DEFAULT_QUALITY;           // output quality
        if (iflags.wc_font_status && *iflags.wc_font_status) {
            lgfnt.lfPitchAndFamily = DEFAULT_PITCH; // pitch and family
            NH_A2W(iflags.wc_font_status, lgfnt.lfFaceName, LF_FACESIZE);
        } else {
            lgfnt.lfPitchAndFamily = FIXED_PITCH; // pitch and family
        }
        break;

    case NHW_MENU:
        lgfnt.lfHeight = -iflags.wc_fontsiz_menu
                         * GetDeviceCaps(hdc, LOGPIXELSY)
                         / 72;   // height of font
        lgfnt.lfWidth = 0;       // average character width
        lgfnt.lfEscapement = 0;  // angle of escapement
        lgfnt.lfOrientation = 0; // base-line orientation angle
        lgfnt.lfWeight = (attr == ATR_BOLD || attr == ATR_INVERSE)
                             ? FW_BOLD
                             : FW_NORMAL; // font weight
        lgfnt.lfItalic =
            (attr == ATR_BLINK) ? TRUE : FALSE; // italic attribute option
        lgfnt.lfUnderline =
            (attr == ATR_ULINE) ? TRUE : FALSE; // underline attribute option
        lgfnt.lfStrikeOut = FALSE;              // strikeout attribute option
        lgfnt.lfCharSet = mswin_charset();      // character set identifier
        lgfnt.lfOutPrecision = OUT_DEFAULT_PRECIS;   // output precision
        lgfnt.lfClipPrecision = CLIP_DEFAULT_PRECIS; // clipping precision
        lgfnt.lfQuality = DEFAULT_QUALITY;           // output quality
        if (iflags.wc_font_menu && *iflags.wc_font_menu) {
            lgfnt.lfPitchAndFamily = DEFAULT_PITCH; // pitch and family
            NH_A2W(iflags.wc_font_menu, lgfnt.lfFaceName, LF_FACESIZE);
        } else {
            lgfnt.lfPitchAndFamily = FIXED_PITCH; // pitch and family
        }
        break;

    case NHW_MESSAGE:
        font_size = (attr == ATR_INVERSE) ? iflags.wc_fontsiz_message + 1
                                          : iflags.wc_fontsiz_message;
        lgfnt.lfHeight = -font_size * GetDeviceCaps(hdc, LOGPIXELSY)
                         / 72;   // height of font
        lgfnt.lfWidth = 0;       // average character width
        lgfnt.lfEscapement = 0;  // angle of escapement
        lgfnt.lfOrientation = 0; // base-line orientation angle
        lgfnt.lfWeight = (attr == ATR_BOLD || attr == ATR_INVERSE)
                             ? FW_BOLD
                             : FW_NORMAL; // font weight
        lgfnt.lfItalic =
            (attr == ATR_BLINK) ? TRUE : FALSE; // italic attribute option
        lgfnt.lfUnderline =
            (attr == ATR_ULINE) ? TRUE : FALSE; // underline attribute option
        lgfnt.lfStrikeOut = FALSE;              // strikeout attribute option
        lgfnt.lfCharSet = mswin_charset();      // character set identifier
        lgfnt.lfOutPrecision = OUT_DEFAULT_PRECIS;   // output precision
        lgfnt.lfClipPrecision = CLIP_DEFAULT_PRECIS; // clipping precision
        lgfnt.lfQuality = DEFAULT_QUALITY;           // output quality
        if (iflags.wc_font_message && *iflags.wc_font_message) {
            lgfnt.lfPitchAndFamily = DEFAULT_PITCH; // pitch and family
            NH_A2W(iflags.wc_font_message, lgfnt.lfFaceName, LF_FACESIZE);
        } else {
            lgfnt.lfPitchAndFamily = VARIABLE_PITCH; // pitch and family
        }
        break;

    case NHW_TEXT:
        lgfnt.lfHeight = -iflags.wc_fontsiz_text
                         * GetDeviceCaps(hdc, LOGPIXELSY)
                         / 72;   // height of font
        lgfnt.lfWidth = 0;       // average character width
        lgfnt.lfEscapement = 0;  // angle of escapement
        lgfnt.lfOrientation = 0; // base-line orientation angle
        lgfnt.lfWeight = (attr == ATR_BOLD || attr == ATR_INVERSE)
                             ? FW_BOLD
                             : FW_NORMAL; // font weight
        lgfnt.lfItalic =
            (attr == ATR_BLINK) ? TRUE : FALSE; // italic attribute option
        lgfnt.lfUnderline =
            (attr == ATR_ULINE) ? TRUE : FALSE; // underline attribute option
        lgfnt.lfStrikeOut = FALSE;              // strikeout attribute option
        lgfnt.lfCharSet = mswin_charset();      // character set identifier
        lgfnt.lfOutPrecision = OUT_DEFAULT_PRECIS;   // output precision
        lgfnt.lfClipPrecision = CLIP_DEFAULT_PRECIS; // clipping precision
        lgfnt.lfQuality = DEFAULT_QUALITY;           // output quality
        if (iflags.wc_font_text && *iflags.wc_font_text) {
            lgfnt.lfPitchAndFamily = DEFAULT_PITCH; // pitch and family
            NH_A2W(iflags.wc_font_text, lgfnt.lfFaceName, LF_FACESIZE);
        } else {
            lgfnt.lfPitchAndFamily = FIXED_PITCH; // pitch and family
        }
        break;
    }

    fnt = CreateFontIndirect(&lgfnt);

    /* add font to the table */
    if (font_index == font_table_size) {
        if (font_table_size >= MAXFONTS)
            panic("font table overflow!");
        font_table_size++;
    } else {
        DeleteObject(font_table[font_index].hFont);
    }

    font_table[font_index].code = NHFONT_CODE(win_type, attr);
    font_table[font_index].hFont = fnt;
    return fnt;
}

UINT
mswin_charset()
{
    CHARSETINFO cis;
    if (SYMHANDLING(H_IBM))
        if (TranslateCharsetInfo((DWORD *) GetOEMCP(), &cis, TCI_SRCCODEPAGE))
            return cis.ciCharset;
        else
            return OEM_CHARSET;
    else if (TranslateCharsetInfo((DWORD *) GetACP(), &cis, TCI_SRCCODEPAGE))
        return cis.ciCharset;
    else
        return ANSI_CHARSET;
}

void __cdecl font_table_cleanup(void)
{
    int i;
    for (i = 0; i < font_table_size; i++) {
        DeleteObject(font_table[i].hFont);
    }
    font_table_size = 0;
}
