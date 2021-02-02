/* NetHack 3.6	amidos.p	$NHDT-Date: 1432512796 2015/05/25 00:13:16 $  $NHDT-Branch: master $:$NHDT-Revision: 1.7 $ */
/* Copyright (c) Kenneth Lorber, Bethesda, Maryland, 1992, 1993. */
/* NetHack may be freely redistributed.  See license for details. */

/* amidos.c */
void flushout (void);
#ifndef	getuid
int getuid (void);
#endif
#ifndef	getpid
int getpid (void);
#endif
#ifndef	getlogin
char *getlogin (void);
#endif
#ifndef	abs
int abs(int );
#endif
int tgetch (void);
int dosh (void);
long freediskspace(char *);
long filesize(char *);
void eraseall(const char * , const char *);
char *CopyFile(const char * , const char *);
void copybones(int );
void playwoRAMdisk (void);
int saveDiskPrompt(int );
void gameDiskPrompt (void);
void append_slash(char *);
void getreturn(const char *);
#ifndef msmsg
void msmsg( const char *, ... );
#endif
#if !defined(__SASC_60) && !defined(_DCC)
int chdir(char *);
#endif
#ifndef	strcmpi
int strcmpi(char * , char *);
#endif
#if !defined(memcmp) && !defined(AZTEC_C) && !defined(_DCC) && !defined(__GNUC__)
int memcmp(unsigned char * , unsigned char * , int );
#endif
