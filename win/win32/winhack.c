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
	_nethack_app.bmpPetMark = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_PETMARK));
	if( _nethack_app.bmpPetMark==NULL ) panic("cannot load pet mark bitmap");
	_nethack_app.bNoHScroll = FALSE;
	_nethack_app.bNoVScroll = FALSE;
	_nethack_app.mapDisplayMode = NHMAP_VIEW_TILES;
	_nethack_app.winStatusAlign = NHWND_ALIGN_BOTTOM;
	_nethack_app.winMessageAlign = NHWND_ALIGN_TOP;

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

/* options */
struct t_win32_opt_int {
	const char* val;
	int   opt;
};
static struct t_win32_opt_int _win32_map_mode[] = 
{
	{ "tiles", NHMAP_VIEW_TILES				},
	{ "ascii4x6", NHMAP_VIEW_ASCII4x6		},	
	{ "ascii6x8", NHMAP_VIEW_ASCII6x8		},	
	{ "ascii8x8", NHMAP_VIEW_ASCII8x8		},	
	{ "ascii16x8", NHMAP_VIEW_ASCII16x8		},
	{ "ascii7x12", NHMAP_VIEW_ASCII7x12		},
	{ "ascii8x12", NHMAP_VIEW_ASCII8x12		},
	{ "ascii16x12", NHMAP_VIEW_ASCII16x12	},	
	{ "ascii12x16", NHMAP_VIEW_ASCII12x16	},	
	{ "ascii10x18", NHMAP_VIEW_ASCII10x18	},	
	{ "fit_to_screen", NHMAP_VIEW_FIT_TO_SCREEN	},
	{ NULL, -1 }
};

static struct t_win32_opt_int _win32_align[] = 
{
	{ "left", NHWND_ALIGN_LEFT },
	{ "right", NHWND_ALIGN_RIGHT },
	{ "top", NHWND_ALIGN_TOP },
	{ "bottom", NHWND_ALIGN_BOTTOM },
	{ NULL, -1 }
};

int set_win32_option( const char * name, const char * val)
{
	struct t_win32_opt_int* p;
	if( _stricmp(name, "win32_map_mode")==0 ) {
		for( p=_win32_map_mode; p->val; p++ ) {
			if( _stricmp(p->val, val)==0 ){
				GetNHApp()->mapDisplayMode = p->opt;
				return 1;
			}
		}
		return 0;
	} else if( _stricmp(name, "win32_align_status")==0 ) {
		for( p=_win32_align; p->val; p++ ) {
			if( _stricmp(p->val, val)==0 ) {
				GetNHApp()->winStatusAlign = p->opt;
				return 1;
			}
		}
		return 0;
	} else if( _stricmp(name, "win32_align_message")==0 ) {
		for( p=_win32_align; p->val; p++ ) {
			if( _stricmp(p->val, val)==0 ) {
				GetNHApp()->winMessageAlign = p->opt;
				return 1;
			}
		}
		return 0;
	}
	return 0;
}