/* NetHack 3.6	dlb.h	$NHDT-Date: 1432512780 2015/05/25 00:13:00 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $ */
/* Copyright (c) Kenneth Lorber, Bethesda, Maryland, 1993. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef DLB_H
#define DLB_H
/* definitions for data library */

#ifdef DLB

/* implementations */
#ifdef MAC
#define DLBRSRC /* use Mac resources */
#else
#define DLBLIB /* use a set of external files */
#endif

#ifdef DLBLIB
/* directory structure in memory */
typedef struct dlb_directory {
    char *fname;   /* file name as seen from calling code */
    long foffset;  /* offset in lib file to start of this file */
    long fsize;    /* file size */
    char handling; /* how to handle the file (compression, etc) */
} libdir;

/* information about each open library */
typedef struct dlb_library {
    FILE *fdata;   /* opened data file */
    long fmark;    /* current file mark */
    libdir *dir;   /* directory of library file */
    char *sspace;  /* pointer to string space */
    long nentries; /* # of files in directory */
    long rev;      /* dlb file revision */
    long strsize;  /* dlb file string size */
} library;

/* library definitions */
#ifndef DLBFILE
#ifndef VERSION_IN_DLB_FILENAME
#define DLBFILE "nhdat" /* name of library */
#else
#define MAX_DLB_FILENAME 256
#define DLBFILE dlbfilename
#define DLBBASENAME "nhdat"
extern char dlbfilename[MAX_DLB_FILENAME];
extern char *FDECL(build_dlb_filename, (const char *));
#endif
#endif
#ifndef FILENAME_CMP
#define FILENAME_CMP strcmp /* case sensitive */
#endif

#endif /* DLBLIB */

typedef struct dlb_handle {
    FILE *fp; /* pointer to an external file, use if non-null */
#ifdef DLBLIB
    library *lib; /* pointer to library structure */
    long start;   /* offset of start of file */
    long size;    /* size of file */
    long mark;    /* current file marker */
#endif
#ifdef DLBRSRC
    int fd; /* HandleFile file descriptor */
#endif
} dlb;

#if defined(ULTRIX_PROTO) && !defined(__STDC__)
/* buggy old Ultrix compiler wants this for the (*dlb_fread_proc)
   and (*dlb_fgets_proc) prototypes in struct dlb_procs (dlb.c);
   we'll use it in all the declarations for consistency */
#define DLB_P struct dlb_handle *
#else
#define DLB_P dlb *
#endif

boolean NDECL(dlb_init);
void NDECL(dlb_cleanup);

dlb *FDECL(dlb_fopen, (const char *, const char *));
int FDECL(dlb_fclose, (DLB_P));
int FDECL(dlb_fread, (char *, int, int, DLB_P));
int FDECL(dlb_fseek, (DLB_P, long, int));
char *FDECL(dlb_fgets, (char *, int, DLB_P));
int FDECL(dlb_fgetc, (DLB_P));
long FDECL(dlb_ftell, (DLB_P));

/* Resource DLB entry points */
#ifdef DLBRSRC
boolean rsrc_dlb_init(void);
void rsrc_dlb_cleanup(void);
boolean rsrc_dlb_fopen(dlb *dp, const char *name, const char *mode);
int rsrc_dlb_fclose(dlb *dp);
int rsrc_dlb_fread(char *buf, int size, int quan, dlb *dp);
int rsrc_dlb_fseek(dlb *dp, long pos, int whence);
char *rsrc_dlb_fgets(char *buf, int len, dlb *dp);
int rsrc_dlb_fgetc(dlb *dp);
long rsrc_dlb_ftell(dlb *dp);
#endif

#else /* DLB */

#define dlb FILE

#define dlb_init()
#define dlb_cleanup()

#define dlb_fopen fopen
#define dlb_fclose fclose
#define dlb_fread fread
#define dlb_fseek fseek
#define dlb_fgets fgets
#define dlb_fgetc fgetc
#define dlb_ftell ftell

#endif /* DLB */

/* various other I/O stuff we don't want to replicate everywhere */

#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif
#ifndef SEEK_END
#define SEEK_END 2
#endif

#define RDTMODE "r"
#if (defined(MSDOS) || defined(WIN32) || defined(TOS) || defined(OS2)) \
    && defined(DLB)
#define WRTMODE "w+b"
#else
#define WRTMODE "w+"
#endif
#if (defined(MICRO) && !defined(AMIGA)) || defined(THINK_C) \
    || defined(__MWERKS__) || defined(WIN32)
#define RDBMODE "rb"
#define WRBMODE "w+b"
#else
#define RDBMODE "r"
#define WRBMODE "w+"
#endif

#endif /* DLB_H */
