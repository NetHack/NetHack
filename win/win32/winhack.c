/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
// winhack.cpp : Defines the entry point for the application.
//

#include <process.h>
#include "winMS.h"
#include "hack.h"
#include "dlb.h"
#include "resource.h"
#include "mhmain.h"

#ifdef OVL0
#define SHARED_DCL
#else
#define SHARED_DCL extern
#endif

SHARED_DCL char orgdir[PATHLEN];	/* also used in pcsys.c, amidos.c */

extern void FDECL(nethack_exit,(int));

// Global Variables:
NHWinApp _nethack_app;

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
	TCHAR* p;
	TCHAR wbuf[BUFSZ];
	char buf[BUFSZ];

	/* init applicatio structure */
	_nethack_app.hApp = hInstance;
	_nethack_app.hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_WINHACK);
	_nethack_app.hMainWnd = NULL;
	_nethack_app.hMenuWnd = NULL;
	_nethack_app.bmpTiles = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TILES));
	if( _nethack_app.bmpTiles==NULL ) panic("cannot load tiles bitmap");
	_nethack_app.bNoHScroll = FALSE;
	_nethack_app.bNoVScroll = FALSE;

	// init controls
	LoadLibrary( TEXT("RICHED32.DLL") );

	ZeroMemory(&InitCtrls, sizeof(InitCtrls));
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&InitCtrls);
	
	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

#ifdef _DEBUG
	wizard = TRUE;
#endif

	/* get command line parameters */
	GetModuleFileName(NULL, wbuf, BUFSZ);
	argv[0] = _strdup(NH_W2A(wbuf, buf, BUFSZ));
	
	p = _tcstok(GetCommandLine(), TEXT(" "));
	for( argc=1; p && argc<MAX_CMDLINE_PARAM; argc++ ) {
		len = _tcslen(p);
		if( len>0 ) {
			argv[argc] = _strdup( NH_W2A(p, buf, BUFSZ) );
		} else {
			argv[argc] = "";
		}
		p = _tcstok(NULL, TEXT(" "));
	}

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

