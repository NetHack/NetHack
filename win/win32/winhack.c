/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
// winhack.cpp : Defines the entry point for the application.
//

#include <process.h>
#include "winMS.h"
#include "hack.h"
#include "dlb.h"
#include "resource.h"
#include "mhmain.h"
#include "mhmap.h"

#ifdef OVL0
#define SHARED_DCL
#else
#define SHARED_DCL extern
#endif

extern void FDECL(nethack_exit,(int));

// Global Variables:
NHWinApp _nethack_app;

#ifdef __BORLANDC__
#define _stricmp(s1,s2)     stricmp(s1,s2)
#define _strdup(s1)         strdup(s1)
#endif

// Foward declarations of functions included in this code module:
BOOL				InitInstance(HINSTANCE, int);

extern void FDECL(pcmain, (int,char **));
static void __cdecl mswin_moveloop(void *);
static BOOL initMapTiles(void);

#define MAX_CMDLINE_PARAM 255

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	INITCOMMONCONTROLSEX InitCtrls;
	int argc;
	char* argv[MAX_CMDLINE_PARAM];
	size_t len;
	TCHAR* p;
	TCHAR wbuf[BUFSZ];
	char buf[BUFSZ];

	/* init applicatio structure */
	_nethack_app.hApp = hInstance;
	_nethack_app.hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_NETHACKW);
	_nethack_app.hMainWnd = NULL;
	_nethack_app.hPopupWnd = NULL;
	_nethack_app.bmpTiles = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TILES));
	if( _nethack_app.bmpTiles==NULL ) panic("cannot load tiles bitmap");
	_nethack_app.bmpPetMark = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_PETMARK));
	if( _nethack_app.bmpPetMark==NULL ) panic("cannot load pet mark bitmap");
	_nethack_app.bmpMapTiles = _nethack_app.bmpTiles;
	_nethack_app.mapTile_X = TILE_X;
	_nethack_app.mapTile_Y = TILE_Y;
	_nethack_app.mapTilesPerLine = TILES_PER_LINE;

	_nethack_app.bNoHScroll = FALSE;
	_nethack_app.bNoVScroll = FALSE;
	_nethack_app.saved_text = strdup("");

	// init controls
	ZeroMemory(&InitCtrls, sizeof(InitCtrls));
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&InitCtrls);
	
	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	/* get command line parameters */	
	p = _tcstok(GetCommandLine(), TEXT(" "));
	for( argc=0; p && argc<MAX_CMDLINE_PARAM; argc++ ) {
		len = _tcslen(p);
		if( len>0 ) {
			argv[argc] = _strdup( NH_W2A(p, buf, BUFSZ) );
		} else {
			argv[argc] = "";
		}
		p = _tcstok(NULL, TEXT(" "));
	}
	GetModuleFileName(NULL, wbuf, BUFSZ);
	argv[0] = _strdup(NH_W2A(wbuf, buf, BUFSZ));

	pcmain(argc,argv);

	/* initialize map tiles bitmap */
	initMapTiles();

	moveloop();

	return 0;
}


//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hWnd = mswin_init_main_window();
   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   _nethack_app.hMainWnd = hWnd;

   return TRUE;
}

PNHWinApp GetNHApp()
{
	return &_nethack_app;
}

BOOL initMapTiles(void)
{
	HBITMAP hBmp;
	BITMAP  bm;
	TCHAR   wbuf[MAX_PATH];
	int     tl_num;
	SIZE    map_size;
	extern int total_tiles_used;

	/* no file - no tile */
	if( !(iflags.wc_tile_file && *iflags.wc_tile_file) ) 
		return TRUE;

	/* load bitmap */
	hBmp = LoadImage(
				GetNHApp()->hApp, 
				NH_A2W(iflags.wc_tile_file, wbuf, MAX_PATH),
				IMAGE_BITMAP,
				0,
				0,
				LR_LOADFROMFILE | LR_DEFAULTSIZE 
			);
	if( hBmp==NULL ) {
		raw_print("Cannot load tiles from the file. Reverting back to default.");
		return FALSE;
	}

	/* calculate tile dimensions */
	GetObject(hBmp, sizeof(BITMAP), (LPVOID)&bm);
	if( bm.bmWidth%iflags.wc_tile_width ||
		bm.bmHeight%iflags.wc_tile_height ) {
		DeleteObject(hBmp);
		raw_print("Tiles bitmap does not match tile_width and tile_height options. Reverting back to default.");
		return FALSE;
	}

	tl_num = (bm.bmWidth/iflags.wc_tile_width)*
		     (bm.bmHeight/iflags.wc_tile_height);
	if( tl_num<total_tiles_used ) {
		DeleteObject(hBmp);
		raw_print("Number of tiles in the bitmap is less than required by the game. Reverting back to default.");
		return FALSE;
	}

	/* set the tile information */
	if( GetNHApp()->bmpMapTiles!=GetNHApp()->bmpTiles ) {
		DeleteObject(GetNHApp()->bmpMapTiles);
	}

	GetNHApp()->bmpMapTiles = hBmp;
	GetNHApp()->mapTile_X = iflags.wc_tile_width;
	GetNHApp()->mapTile_Y = iflags.wc_tile_height;
	GetNHApp()->mapTilesPerLine = bm.bmWidth / iflags.wc_tile_width;

	map_size.cx = GetNHApp()->mapTile_X * COLNO;
	map_size.cy = GetNHApp()->mapTile_Y * ROWNO;
	mswin_map_stretch(
		mswin_hwnd_from_winid(WIN_MAP),
		&map_size,
		TRUE 
	);
	return TRUE;
}
