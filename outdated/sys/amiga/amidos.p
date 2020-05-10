/* NetHack 3.6	amidos.p	$NHDT-Date: 1432512796 2015/05/25 00:13:16 $  $NHDT-Branch: master $:$NHDT-Revision: 1.7 $ */
/* Copyright (c) Kenneth Lorber, Bethesda, Maryland, 1992, 1993. */
/* NetHack may be freely redistributed.  See license for details. */

/* amidos.c */
void NDECL(flushout );
#ifndef	getuid
int NDECL(getuid );
#endif
#ifndef	getpid
int NDECL(getpid );
#endif
#ifndef	getlogin
char *NDECL(getlogin );
#endif
#ifndef	abs
int FDECL(abs, (int ));
#endif
int NDECL(tgetch );
int NDECL(dosh );
long FDECL(freediskspace, (char *));
long FDECL(filesize, (char *));
void FDECL(eraseall, (const char * , const char *));
char *FDECL(CopyFile, (const char * , const char *));
void FDECL(copybones, (int ));
void NDECL(playwoRAMdisk );
int FDECL(saveDiskPrompt, (int ));
void NDECL(gameDiskPrompt );
void FDECL(append_slash, (char *));
void FDECL(getreturn, (const char *));
#ifndef msmsg
void FDECL(msmsg, ( const char *, ... ));
#endif
#if !defined(__SASC_60) && !defined(_DCC)
int FDECL(chdir, (char *));
#endif
#ifndef	strcmpi
int FDECL(strcmpi, (char * , char *));
#endif
#if !defined(memcmp) && !defined(AZTEC_C) && !defined(_DCC) && !defined(__GNUC__)
int FDECL(memcmp, (unsigned char * , unsigned char * , int ));
#endif
