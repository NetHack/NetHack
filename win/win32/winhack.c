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
static TCHAR* _get_cmd_arg(TCHAR* pCmdLine);

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
	TCHAR *p;
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
	p = _get_cmd_arg(GetCommandLine());
	p = _get_cmd_arg(NULL); /* skip first paramter - command name */
	for( argc = 1; p && argc<MAX_CMDLINE_PARAM; argc++ ) {
		len = _tcslen(p);
		if( len>0 ) {
			argv[argc] = _strdup( NH_W2A(p, buf, BUFSZ) );
		} else {
			argv[argc] = "";
		}
		p = _get_cmd_arg(NULL);
	}
	GetModuleFileName(NULL, wbuf, BUFSZ);
	argv[0] = _strdup(NH_W2A(wbuf, buf, BUFSZ));

	pcmain(argc,argv);

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

TCHAR* _get_cmd_arg(TCHAR* pCmdLine)
{
        static TCHAR* pArgs = NULL;
        TCHAR  *pRetArg;
        BOOL   bQuoted;

        if( !pCmdLine && !pArgs ) return NULL;
        if( !pArgs ) pArgs = pCmdLine;

        /* skip whitespace */
        for(pRetArg = pArgs; *pRetArg && _istspace(*pRetArg); pRetArg = CharNext(pRetArg));
		if( !*pRetArg ) {
			pArgs = NULL;
			return NULL;
		}

        /* check for quote */
        if( *pRetArg==TEXT('"') ) {
                bQuoted = TRUE;
                pRetArg = CharNext(pRetArg);
				pArgs = _tcschr(pRetArg, TEXT('"'));
	    } else {
			/* skip to whitespace */
			for(pArgs = pRetArg; *pArgs && !_istspace(*pArgs); pArgs = CharNext(pArgs));
		}
		
		if( pArgs && *pArgs ) {
			TCHAR* p;
			p = pArgs;
			pArgs = CharNext(pArgs);
			*p = (TCHAR)0;
		} else {
			pArgs = NULL;
		}

		return pRetArg;
}

