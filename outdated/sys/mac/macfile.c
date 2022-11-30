/* NetHack 3.6	macfile.c	$NHDT-Date: 1432512798 2015/05/25 00:13:18 $  $NHDT-Branch: master $:$NHDT-Revision: 1.11 $ */
/* Copyright (c) Jon W{tte, Hao-Yang Wang, Jonathan Handler 1992. */
/* NetHack may be freely redistributed.  See license for details. */
/*
 * macfile.c
 * MAC file I/O routines
 */

#include "hack.h"
#include "macwin.h"

#ifndef __MACH__
#include <files.h>
#include <errors.h>
#include <resources.h>
#include <memory.h>
#include <TextUtils.h>
#include <ToolUtils.h>
#endif

#include "dlb.h"

/*
 * We should get the default dirID and volRefNum (from name) from prefs and
 * the situation at startup... For now, this will have to do.
 */

/* The HandleFiles are resources built into the application which are treated
   as read-only files: if we fail to open a file we look for a resource */

#define FIRST_HF 32000 /* file ID of first HandleFile */
#define MAX_HF 6       /* Max # of open HandleFiles */

#define APP_NAME_RES_ID (-16396)

typedef struct handlefile {
    long type;   /* Resource type */
    short id;    /* Resource id */
    long mark;   /* Current position */
    long size;   /* total size */
    Handle data; /* The resource, purgeable */
} HandleFile;

static HandleFile *IsHandleFile(int);
static int OpenHandleFile(const unsigned char *, long);
static int CloseHandleFile(int);
static int ReadHandleFile(int, void *, unsigned);
static long SetHandleFilePos(int, short, long);

HandleFile theHandleFiles[MAX_HF];
MacDirs theDirs; /* also referenced in macwin.c */

static HandleFile *
IsHandleFile(int fd)
{
    HandleFile *hfp = NULL;

    if (fd >= FIRST_HF && fd < FIRST_HF + MAX_HF) {
        /* in valid range, check for data */
        hfp = &theHandleFiles[fd - FIRST_HF];
        if (!hfp->data)
            hfp = NULL;
    }
    return hfp;
}

static int
OpenHandleFile(const unsigned char *name, long fileType)
{
    int i;
    Handle h;
    Str255 s;

    for (i = 0; i < MAX_HF; i++) {
        if (theHandleFiles[i].data == 0L)
            break;
    }

    if (i >= MAX_HF)
        return -1;

    h = GetNamedResource(fileType, name);
    if (!h)
        return (-1);

    theHandleFiles[i].data = h;
    theHandleFiles[i].size = GetHandleSize(h);
    GetResInfo(h, &theHandleFiles[i].id, (void *) &theHandleFiles[i].type, s);
    theHandleFiles[i].mark = 0L;

    return (i + FIRST_HF);
}

static int
CloseHandleFile(int fd)
{
    if (!IsHandleFile(fd)) {
        return -1;
    }
    fd -= FIRST_HF;
    ReleaseResource(theHandleFiles[fd].data);
    theHandleFiles[fd].data = 0L;
    return (0);
}

static int
ReadHandleFile(int fd, void *ptr, unsigned len)
{
    unsigned maxBytes;
    Handle h;

    if (!IsHandleFile(fd))
        return -1;

    fd -= FIRST_HF;
    maxBytes = theHandleFiles[fd].size - theHandleFiles[fd].mark;
    if (len > maxBytes)
        len = maxBytes;

    h = theHandleFiles[fd].data;

    HLock(h);
    BlockMove(*h + theHandleFiles[fd].mark, ptr, len);
    HUnlock(h);
    theHandleFiles[fd].mark += len;

    return (len);
}

static long
SetHandleFilePos(int fd, short whence, long pos)
{
    long curpos;

    if (!IsHandleFile(fd))
        return -1;

    fd -= FIRST_HF;

    curpos = theHandleFiles[fd].mark;
    switch (whence) {
    case SEEK_CUR:
        curpos += pos;
        break;
    case SEEK_END:
        curpos = theHandleFiles[fd].size - pos;
        break;
    default: /* set */
        curpos = pos;
        break;
    }

    if (curpos < 0)
        curpos = 0;
    else if (curpos > theHandleFiles[fd].size)
        curpos = theHandleFiles[fd].size;

    theHandleFiles[fd].mark = curpos;

    return curpos;
}

void
C2P(const char *c, unsigned char *p)
{
    int len = strlen(c), i;

    if (len > 255)
        len = 255;

    for (i = len; i > 0; i--)
        p[i] = c[i - 1];
    p[0] = len;
}

void
P2C(const unsigned char *p, char *c)
{
    int idx = *p++;
    for (; idx > 0; idx--)
        *c++ = *p++;
    *c = '\0';
}

static void
replace_resource(Handle new_res, ResType its_type, short its_id,
                 Str255 its_name)
{
    Handle old_res;

    SetResLoad(false);
    old_res = Get1Resource(its_type, its_id);
    SetResLoad(true);
    if (old_res) {
        RemoveResource(old_res);
        DisposeHandle(old_res);
    }

    AddResource(new_res, its_type, its_id, its_name);
}

int
maccreat(const char *name, long fileType)
{
    return macopen(name, O_RDWR | O_CREAT | O_TRUNC, fileType);
}

int
macopen(const char *name, int flags, long fileType)
{
    short refNum;
    short perm;
    Str255 s;

    C2P(name, s);
    if (flags & O_CREAT) {
        if (HCreate(theDirs.dataRefNum, theDirs.dataDirID, s, TEXT_CREATOR,
                    fileType) && (flags & O_EXCL)) {
            return -1;
        }
#if 0 /* Fails during makedefs */
		if (fileType == SAVE_TYPE) {
			short resRef;
			HCreateResFile(theDirs.dataRefNum, theDirs.dataDirID, s);
			resRef = HOpenResFile(theDirs.dataRefNum, theDirs.dataDirID, s,
								  fsRdWrPerm);
			if (resRef != -1) {
				Handle name;
				Str255 plnamep;

				C2P(gp.plname, plnamep);
				name = (Handle)NewString(plnamep);
				if (name)
					replace_resource(name, 'STR ', PLAYER_NAME_RES_ID,
									"\pPlayer Name");

				/* The application name resource.  See IM VI, page 9-21. */
				name = (Handle)GetString(APP_NAME_RES_ID);
				if (name) {
					DetachResource(name);
					replace_resource(name, 'STR ', APP_NAME_RES_ID,
									 "\pApplication Name");
				}

				CloseResFile(resRef);
			}
		}
#endif
    }
    /*
     * Here, we should check for file type, maybe a SFdialog if
     * we fail with default, etc. etc. Besides, we should use HOpen
     * and permissions.
     */
    if ((flags & O_RDONLY) == O_RDONLY) {
        perm = fsRdPerm;
    }
    if ((flags & O_WRONLY) == O_WRONLY) {
        perm = fsWrPerm;
    }
    if ((flags & O_RDWR) == O_RDWR) {
        perm = fsRdWrPerm;
    }
    if (HOpen(theDirs.dataRefNum, theDirs.dataDirID, s, perm, &refNum)) {
        return OpenHandleFile(s, fileType);
    }
    if (flags & O_TRUNC) {
        if (SetEOF(refNum, 0L)) {
            FSClose(refNum);
            return -1;
        }
    }
    return refNum;
}

int
macclose(int fd)
{
    if (IsHandleFile(fd)) {
        CloseHandleFile(fd);
    } else {
        if (FSClose(fd)) {
            return -1;
        }
        FlushVol((StringPtr) 0, theDirs.dataRefNum);
    }
    return 0;
}

int
macread(int fd, void *ptr, unsigned len)
{
    long amt = len;

    if (IsHandleFile(fd)) {
        return ReadHandleFile(fd, ptr, amt);
    } else {
        short err = FSRead(fd, &amt, ptr);

        return ((err == noErr) || (err == eofErr && len)) ? amt : -1;
    }
}

#if 0  /* this function isn't used, if you use it, uncomment prototype in \
          macwin.h */
char *
macgets (int fd, char *ptr, unsigned len)
{
        int idx = 0;
        char c;

        while (-- len > 0) {
                if (macread (fd, ptr + idx, 1) <= 0)
                        return (char *)0;
                c = ptr[idx++];
                if (c  == '\n' || c == '\r')
                        break;
        }
        ptr [idx] = '\0';
        return ptr;
}
#endif /* 0 */

int
macwrite(int fd, void *ptr, unsigned len)
{
    long amt = len;

    if (IsHandleFile(fd))
        return -1;
    if (FSWrite(fd, &amt, ptr) == noErr)
        return (amt);
    else
        return (-1);
}

long
macseek(int fd, long where, short whence)
{
    short posMode;
    long curPos;

    if (IsHandleFile(fd)) {
        return SetHandleFilePos(fd, whence, where);
    }

    switch (whence) {
    default:
        posMode = fsFromStart;
        break;
    case SEEK_CUR:
        posMode = fsFromMark;
        break;
    case SEEK_END:
        posMode = fsFromLEOF;
        break;
    }

    if (SetFPos(fd, posMode, where) == noErr && GetFPos(fd, &curPos) == noErr)
        return (curPos);
    else
        return (-1);
}

int
macunlink(const char *name)
{
    Str255 pname;

    C2P(name, pname);
    return (HDelete(theDirs.dataRefNum, theDirs.dataDirID, pname) == noErr
                ? 0
                : -1);
}

/* ---------------------------------------------------------------------- */

boolean
rsrc_dlb_init(void)
{
    return TRUE;
}

void
rsrc_dlb_cleanup(void)
{
}

boolean
rsrc_dlb_fopen(dlb *dp, const char *name, const char *mode)
{
#if defined(__SC__) || defined(__MRC__)
#pragma unused(mode)
#endif
    Str255 pname;

    C2P(name, pname);
    dp->fd = OpenHandleFile(pname, 'File'); /* automatically read-only */
    return dp->fd >= 0;
}

int
rsrc_dlb_fclose(dlb *dp)
{
    return CloseHandleFile(dp->fd);
}

int
rsrc_dlb_fread(char *buf, int size, int quan, dlb *dp)
{
    int nread;

    if (size < 0 || quan < 0)
        return 0;
    nread = ReadHandleFile(dp->fd, buf, (unsigned) size * (unsigned) quan);

    return nread / size; /* # of whole pieces (== quan in normal case) */
}

int
rsrc_dlb_fseek(dlb *dp, long pos, int whence)
{
    return SetHandleFilePos(dp->fd, whence, pos);
}

char *
rsrc_dlb_fgets(char *buf, int len, dlb *dp)
{
    HandleFile *hfp = IsHandleFile(dp->fd);
    char *p;
    int bytesLeft, n = 0;

    if (hfp && hfp->mark < hfp->size) {
        bytesLeft = hfp->size - hfp->mark;
        if (bytesLeft < len)
            len = bytesLeft;

        HLock(hfp->data);
        for (n = 0, p = *hfp->data + hfp->mark; n < len; n++, p++) {
            buf[n] = *p;
            if (*p == '\r')
                buf[n] = '\n';
            if (buf[n] == '\n') {
                n++; /* we want the return in the buffer */
                break;
            }
        }
        HUnlock(hfp->data);

        hfp->mark += n;
        if (n != 0)
            buf[n] = '\0'; /* null terminate result */
    }

    return n ? buf : NULL;
}

int
rsrc_dlb_fgetc(dlb *dp)
{
    HandleFile *hfp = IsHandleFile(dp->fd);
    int ret;

    if (!hfp || hfp->size <= hfp->mark)
        return EOF;

    ret = *(unsigned char *) (*hfp->data + hfp->mark);
    hfp->mark++;
    return ret;
}

long
rsrc_dlb_ftell(dlb *dp)
{
    HandleFile *hfp = IsHandleFile(dp->fd);

    if (!hfp)
        return 0;
    return hfp->mark;
}
