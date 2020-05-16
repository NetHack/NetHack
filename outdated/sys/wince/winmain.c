// winmain.cpp : Defines the entry point for the application.
//
// $NHDT-Date: 1432512799 2015/05/25 00:13:19 $  $NHDT-Branch: master $:$NHDT-Revision: 1.5 $

#include "winMS.h"
#include <string.h>

#define MAX_CMDLINE_PARAM 255

extern int FDECL(main, (int, char **));
static TCHAR *_get_cmd_arg(TCHAR *pCmdLine);

int APIENTRY
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine,
        int nCmdShow)
{
    int argc;
    char *argv[MAX_CMDLINE_PARAM];
    size_t len;
    TCHAR *p;
    TCHAR wbuf[NHSTR_BUFSIZE];
    char buf[NHSTR_BUFSIZE];

    /* get command line parameters */
    p = _get_cmd_arg(
#if defined(WIN_CE_PS2xx) || defined(WIN32_PLATFORM_HPCPRO)
        lpCmdLine
#else
        GetCommandLine()
#endif
        );
    for (argc = 1; p && argc < MAX_CMDLINE_PARAM; argc++) {
        len = _tcslen(p);
        if (len > 0) {
            argv[argc] = _strdup(NH_W2A(p, buf, BUFSZ));
        } else {
            argv[argc] = "";
        }
        p = _get_cmd_arg(NULL);
    }
    GetModuleFileName(NULL, wbuf, BUFSZ);
    argv[0] = _strdup(NH_W2A(wbuf, buf, BUFSZ));

    main(argc, argv);

    return 0;
}

TCHAR *
_get_cmd_arg(TCHAR *pCmdLine)
{
    static TCHAR *pArgs = NULL;
    TCHAR *pRetArg;
    BOOL bQuoted;

    if (!pCmdLine && !pArgs)
        return NULL;
    if (!pArgs)
        pArgs = pCmdLine;

    /* skip whitespace */
    for (pRetArg = pArgs; *pRetArg && _istspace(*pRetArg);
         pRetArg = CharNext(pRetArg))
        ;
    if (!*pRetArg) {
        pArgs = NULL;
        return NULL;
    }

    /* check for quote */
    if (*pRetArg == TEXT('"')) {
        bQuoted = TRUE;
        pRetArg = CharNext(pRetArg);
        pArgs = _tcschr(pRetArg, TEXT('"'));
    } else {
        /* skip to whitespace */
        for (pArgs = pRetArg; *pArgs && !_istspace(*pArgs);
             pArgs = CharNext(pArgs))
            ;
    }

    if (pArgs && *pArgs) {
        TCHAR *p;
        p = pArgs;
        pArgs = CharNext(pArgs);
        *p = (TCHAR) 0;
    } else {
        pArgs = NULL;
    }

    return pRetArg;
}

#ifndef STRNCMPI
char lowc(c) /* force 'c' into lowercase */
char c;
{
    return ((char) (('A' <= c && c <= 'Z') ? (c | 040) : c));
}

int strncmpi(s1, s2, n) /* case insensitive counted string comparison */
register const char *s1, *s2;
register int n; /*(should probably be size_t, which is usually unsigned)*/
{               /*{ aka strncasecmp }*/
    register char t1, t2;

    while (n--) {
        if (!*s2)
            return (*s1 != 0); /* s1 >= s2 */
        else if (!*s1)
            return -1; /* s1  < s2 */
        t1 = lowc(*s1++);
        t2 = lowc(*s2++);
        if (t1 != t2)
            return (t1 > t2) ? 1 : -1;
    }
    return 0; /* s1 == s2 */
}
#endif /* STRNCMPI */
