/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
/* NetHack may be freely redistributed.  See license for details. */

/* font management and such */

#include "mhfont.h"

#define MAXFONTS	64

/* font table - 64 fonts ought to be enough */
static struct font_table_entry {
	int		code;
	HFONT   hFont;
} font_table[MAXFONTS] ;
static int font_table_size = 0;

#define NHFONT_CODE(win, attr) (((attr&0xFF)<<8)|(win_type&0xFF))


/* create font based on window type, charater attributes and
   window device context */
HGDIOBJ mswin_create_font(int win_type, int attr, HDC hdc)
{
	HFONT fnt = NULL;
	LOGFONT lgfnt;
	int font_size;
	int i;

	ZeroMemory( &lgfnt, sizeof(lgfnt) );

	/* try find font in the table */
	for(i=0; i<font_table_size; i++) {
		if(NHFONT_CODE(win_type, attr)==font_table[i].code) {
			return font_table[i].hFont;
		}
	}

	switch(win_type) {
	case NHW_STATUS:
		lgfnt.lfHeight			=	-8*GetDeviceCaps(hdc, LOGPIXELSY)/72;	 // height of font
		lgfnt.lfWidth			=	0;				     // average character width
		lgfnt.lfEscapement		=	0;					 // angle of escapement
		lgfnt.lfOrientation		=	0;					 // base-line orientation angle
		lgfnt.lfWeight			=	FW_BOLD;             // font weight
		lgfnt.lfItalic			=	FALSE;		         // italic attribute option
		lgfnt.lfUnderline		=	FALSE;			     // underline attribute option
		lgfnt.lfStrikeOut		=	FALSE;			     // strikeout attribute option
		lgfnt.lfCharSet			=	OEM_CHARSET;     // character set identifier
		lgfnt.lfOutPrecision	=	OUT_DEFAULT_PRECIS;  // output precision
		lgfnt.lfClipPrecision	=	CLIP_DEFAULT_PRECIS; // clipping precision
		lgfnt.lfQuality			=	DEFAULT_QUALITY;     // output quality
		lgfnt.lfPitchAndFamily	=	FIXED_PITCH;		 // pitch and family
		/* lgfnt.lfFaceName */
		break;

	case NHW_MENU:
		font_size = (attr==ATR_INVERSE)? 7 : 7;
		lgfnt.lfHeight			=	-font_size*GetDeviceCaps(hdc, LOGPIXELSY)/72;	 // height of font
		lgfnt.lfWidth			=	0;				     // average character width
		lgfnt.lfEscapement		=	0;					 // angle of escapement
		lgfnt.lfOrientation		=	0;					 // base-line orientation angle
		lgfnt.lfWeight			=	(attr==ATR_BOLD || attr==ATR_INVERSE)? FW_BOLD : FW_NORMAL;   // font weight
		lgfnt.lfItalic			=	(attr==ATR_BLINK)? TRUE: FALSE;		     // italic attribute option
		lgfnt.lfUnderline		=	(attr==ATR_ULINE)? TRUE : FALSE;		 // underline attribute option
		lgfnt.lfStrikeOut		=	FALSE;				// strikeout attribute option
		lgfnt.lfCharSet			=	OEM_CHARSET;     // character set identifier
		lgfnt.lfOutPrecision	=	OUT_DEFAULT_PRECIS;  // output precision
		lgfnt.lfClipPrecision	=	CLIP_DEFAULT_PRECIS; // clipping precision
		lgfnt.lfQuality			=	DEFAULT_QUALITY;     // output quality
		lgfnt.lfPitchAndFamily	=	VARIABLE_PITCH;		 // pitch and family
		/* lgfnt.lfFaceName */
		break;

	case NHW_MESSAGE:
		font_size = (attr==ATR_INVERSE)? 10 : 9;
		lgfnt.lfHeight			=	-font_size*GetDeviceCaps(hdc, LOGPIXELSY)/72;	 // height of font
		lgfnt.lfWidth			=	0;				     // average character width
		lgfnt.lfEscapement		=	0;					 // angle of escapement
		lgfnt.lfOrientation		=	0;					 // base-line orientation angle
		lgfnt.lfWeight			=	(attr==ATR_BOLD || attr==ATR_INVERSE)? FW_BOLD : FW_NORMAL;   // font weight
		lgfnt.lfItalic			=	(attr==ATR_BLINK)? TRUE: FALSE;		     // italic attribute option
		lgfnt.lfUnderline		=	(attr==ATR_ULINE)? TRUE : FALSE;		 // underline attribute option
		lgfnt.lfStrikeOut		=	FALSE;			     // strikeout attribute option
		lgfnt.lfCharSet			=	OEM_CHARSET;     // character set identifier
		lgfnt.lfOutPrecision	=	OUT_DEFAULT_PRECIS;  // output precision
		lgfnt.lfClipPrecision	=	CLIP_DEFAULT_PRECIS; // clipping precision
		lgfnt.lfQuality			=	DEFAULT_QUALITY;     // output quality
		lgfnt.lfPitchAndFamily	=	VARIABLE_PITCH;		 // pitch and family
		/* lgfnt.lfFaceName */
		break;

	case NHW_TEXT:
		lgfnt.lfHeight			=	-8*GetDeviceCaps(hdc, LOGPIXELSY)/72;	 // height of font
		lgfnt.lfWidth			=	0;				     // average character width
		lgfnt.lfEscapement		=	0;					 // angle of escapement
		lgfnt.lfOrientation		=	0;					 // base-line orientation angle
		lgfnt.lfWeight			=	(attr==ATR_BOLD || attr==ATR_INVERSE)? FW_BOLD : FW_NORMAL;   // font weight
		lgfnt.lfItalic			=	(attr==ATR_BLINK)? TRUE: FALSE;		     // italic attribute option
		lgfnt.lfUnderline		=	(attr==ATR_ULINE)? TRUE : FALSE;		 // underline attribute option
		lgfnt.lfStrikeOut		=	FALSE;			     // strikeout attribute option
		lgfnt.lfCharSet			=	OEM_CHARSET;     // character set identifier
		lgfnt.lfOutPrecision	=	OUT_DEFAULT_PRECIS;  // output precision
		lgfnt.lfClipPrecision	=	CLIP_DEFAULT_PRECIS; // clipping precision
		lgfnt.lfQuality			=	DEFAULT_QUALITY;     // output quality
		lgfnt.lfPitchAndFamily	=	FIXED_PITCH;		 // pitch and family
		/* lgfnt.lfFaceName */
		break;
	}

	fnt = CreateFontIndirect(&lgfnt);

	/* add font to the table */
	if( font_table_size>=MAXFONTS ) panic( "font table overflow!" );

	font_table[font_table_size].code = NHFONT_CODE(win_type, attr);
	font_table[font_table_size].hFont = fnt;
	font_table_size++;

	return fnt;
}

/* dispose the font object */
void mswin_destroy_font( HGDIOBJ fnt )
{
	/* do nothing - we are going to reuse the font,
	   then it will destroyed when application exits 
	  (at least I hope it will) */

	/* if(fnt) DeleteObject(fnt); */
}

