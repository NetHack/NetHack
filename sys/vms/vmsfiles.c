/* NetHack 3.7	vmsfiles.c	$NHDT-Date: 1596498306 2020/08/03 23:45:06 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.12 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2007. */
/* NetHack may be freely redistributed.  See license for details. */

/*
 *  VMS-specific file manipulation routines to implement some missing
 *  routines or substitute for ones where we want behavior modification.
 */
#include "config.h"
#include <ctype.h>

/* lint supression due to lack of extern.h */
int vms_link(const char *, const char *);
int vms_unlink(const char *);
int vms_creat(const char *, unsigned int);
boolean same_dir(const char *, const char *);
int c__translate(int);

#include <rms.h>
#if 0
#include <psldef.h>
#else
#define PSL$C_EXEC 1 /* executive mode, for priv'd logical name handling */
#endif
#include <errno.h>
#ifndef C$$TRANSLATE /* don't rely on VAXCRTL's internal routine */
#define C$$TRANSLATE(status) (errno = EVMSERR, vaxc$errno = (status))
#endif
extern unsigned long sys$parse(), sys$search(), sys$enter(), sys$remove();
extern int VDECL(lib$match_cond, (int, int, ...));

#define vms_success(sts) ((sts) & 1)         /* odd, */
#define vms_failure(sts) (!vms_success(sts)) /* even */

/* vms_link() -- create an additional directory for an existing file */
int
vms_link(const char *file, const char *new)
{
    struct FAB fab;
    struct NAM nam;
    unsigned short fid[3];
    char esa[NAM$C_MAXRSS];

    fab = cc$rms_fab; /* set block ID and length, zero the rest */
    fab.fab$l_fop = FAB$M_OFP;
    fab.fab$l_fna = (char *) file;
    fab.fab$b_fns = strlen(file);
    fab.fab$l_nam = &nam;
    nam = cc$rms_nam;
    nam.nam$l_esa = esa;
    nam.nam$b_ess = sizeof esa;

    if (vms_success(sys$parse(&fab)) && vms_success(sys$search(&fab))) {
        fid[0] = nam.nam$w_fid[0];
        fid[1] = nam.nam$w_fid[1];
        fid[2] = nam.nam$w_fid[2];
        fab.fab$l_fna = (char *) new;
        fab.fab$b_fns = strlen(new);

        if (vms_success(sys$parse(&fab))) {
            nam.nam$w_fid[0] = fid[0];
            nam.nam$w_fid[1] = fid[1];
            nam.nam$w_fid[2] = fid[2];
            nam.nam$l_esa = nam.nam$l_name;
            nam.nam$b_esl = nam.nam$b_name + nam.nam$b_type + nam.nam$b_ver;

            (void) sys$enter(&fab);
        }
    }

    if (vms_failure(fab.fab$l_sts)) {
        C$$TRANSLATE(fab.fab$l_sts);
        return -1;
    }
    return 0; /* success */
}

/*
   vms_unlink() -- remove a directory entry for a file; should only be used
   for files which have had extra directory entries added, not for deletion
   (because the file won't be deleted, just made inaccessible!).
 */
int
vms_unlink(const char *file)
{
    struct FAB fab;
    struct NAM nam;
    char esa[NAM$C_MAXRSS];

    fab = cc$rms_fab; /* set block ID and length, zero the rest */
    fab.fab$l_fop = FAB$M_DLT;
    fab.fab$l_fna = (char *) file;
    fab.fab$b_fns = strlen(file);
    fab.fab$l_nam = &nam;
    nam = cc$rms_nam;
    nam.nam$l_esa = esa;
    nam.nam$b_ess = sizeof esa;

    if (vms_failure(sys$parse(&fab)) || vms_failure(sys$remove(&fab))) {
        C$$TRANSLATE(fab.fab$l_sts);
        return -1;
    }
    return 0;
}

/*
   Substitute creat() routine -- if trying to create a specific version,
   explicitly remove an existing file of the same name.  Since it's only
   used when we expect exclusive access, add a couple RMS options for
   optimization.  (Don't allow sharing--eliminates coordination overhead,
   and use 32 block buffer for faster throughput; ~30% speedup measured.)
 */
#undef creat
int
vms_creat(const char *file, unsigned int mode)
{
    char filnambuf[BUFSIZ]; /*(not BUFSZ)*/

    if (index(file, ';')) {
        /* assumes remove or delete, not vms_unlink */
        if (!unlink(file)) {
            (void) sleep(1);
            (void) unlink(file);
        }
    } else if (!index(file, '.')) {
        /* force some punctuation to be present */
        file = strcat(strcpy(filnambuf, file), ".");
    }
    return creat(file, mode, "shr=nil", "mbc=32", "mbf=2", "rop=wbh");
}

/*
   Similar substitute for open() -- if an open attempt fails due to being
   locked by another user, retry it once (work-around for a limitation of
   at least one NFS implementation).
 */
#undef open
int
vms_open(const char *file, int flags, unsigned int mode)
{
    char filnambuf[BUFSIZ]; /*(not BUFSZ)*/
    int fd;

    if (!index(file, '.') && !index(file, ';')) {
        /* force some punctuation to be present to make sure that
           the file name can't accidentally match a logical name */
        file = strcat(strcpy(filnambuf, file), ";0");
    }
    fd = open(file, flags, mode, "mbc=32", "mbf=2", "rop=rah");
    if (fd < 0 && errno == EVMSERR && lib$match_cond(vaxc$errno, RMS$_FLK)) {
        (void) sleep(1);
        fd = open(file, flags, mode, "mbc=32", "mbf=2", "rop=rah");
    }
    return fd;
}

/* do likewise for fopen() */
#undef fopen
FILE *
vms_fopen(const char *file, const char *mode)
{
    char filnambuf[BUFSIZ]; /*(not BUFSZ)*/
    FILE *fp;

    if (!index(file, '.') && !index(file, ';')) {
        /* force some punctuation to be present to make sure that
           the file name can't accidentally match a logical name */
        file = strcat(strcpy(filnambuf, file), ";0");
    }
    fp = fopen(file, mode, "mbc=32", "mbf=2", "rop=rah");
    if (!fp && errno == EVMSERR && lib$match_cond(vaxc$errno, RMS$_FLK)) {
        (void) sleep(1);
        fp = fopen(file, mode, "mbc=32", "mbf=2", "rop=rah");
    }
    return fp;
}

/*
   Determine whether two strings contain the same directory name.
   Used for deciding whether installed privileges should be disabled
   when HACKDIR is defined in the environment (or specified via -d on
   the command line).  This version doesn't handle Unix-style file specs.
 */
boolean
same_dir(const char *d1, const char *d2)
{
    if (!d1 || !*d1 || !d2 || !*d2)
        return FALSE;
    else if (!strcmp(d1, d2)) /* strcmpi() would be better, but that leads */
        return TRUE;          /* to linking problems for the utilities */
    else {
        struct FAB f1, f2;
        struct NAM n1, n2;

        f1 = f2 = cc$rms_fab; /* initialize file access block */
        n1 = n2 = cc$rms_nam; /* initialize name block */
        f1.fab$b_acmodes = PSL$C_EXEC << FAB$V_LNM_MODE;
        f1.fab$b_fns = strlen(f1.fab$l_fna = (char *) d1);
        f2.fab$b_fns = strlen(f2.fab$l_fna = (char *) d2);
        f1.fab$l_nam = (genericptr_t) &n1; /* link nam to fab */
        f2.fab$l_nam = (genericptr_t) &n2;
        /* want true device name */
        n1.nam$b_nop = n2.nam$b_nop = NAM$M_NOCONCEAL;

        return (vms_success(sys$parse(&f1)) && vms_success(sys$parse(&f2))
                && n1.nam$t_dvi[0] == n2.nam$t_dvi[0]
                && !strncmp(&n1.nam$t_dvi[1], &n2.nam$t_dvi[1],
                            n1.nam$t_dvi[0])
                && !memcmp((genericptr_t) n1.nam$w_did,
                           (genericptr_t) n2.nam$w_did,
                           sizeof n1.nam$w_did)); /*{ short nam$w_did[3]; }*/
    }
}

/*
 * c__translate -- substitute for VAXCRTL routine C$$TRANSLATE.
 *
 *      Try to convert a VMS status code into its Unix equivalent,
 *      then set `errno' to that value; use EVMSERR if there's no
 *      appropriate translation; set `vaxc$errno' to the original
 *      status code regardless.
 *
 *      These translations match only a subset of VAXCRTL's lookup
 *      table, but work even if the severity has been adjusted or
 *      the inhibit-message bit has been set.
 */
#include <errno.h>
#include <ssdef.h>
#include <rmsdef.h>
/* #include <libdef.h> */
/* #include <mthdef.h> */

#define VALUE(U) \
    trans = U;   \
    break
#define CASE1(V) case (V >> 3)
#define CASE2(V, W) CASE1(V) : CASE1(W)

int
c__translate(int code)
{
    register int trans;

/* clang-format off */
/* *INDENT-OFF* */
    switch ((code & 0x0FFFFFF8) >> 3) { /* strip upper 4 and bottom 3 bits */
    CASE2(RMS$_PRV, SS$_NOPRIV):
        VALUE(EPERM); /* not owner */
    CASE2(RMS$_DNF, RMS$_DIR):
    CASE2(RMS$_FNF, RMS$_FND):
    CASE1(SS$_NOSUCHFILE):
        VALUE(ENOENT); /* no such file or directory */
    CASE2(RMS$_IFI, RMS$_ISI):
        VALUE(EIO); /* i/o error */
    CASE1(RMS$_DEV):
    CASE2(SS$_NOSUCHDEV, SS$_DEVNOTMOUNT):
        VALUE(ENXIO); /* no such device or address codes */
    CASE1(RMS$_DME):
 /* CASE1(LIB$INSVIRMEM): */
    CASE2(SS$_VASFULL, SS$_INSFWSL):
        VALUE(ENOMEM); /* not enough core */
    CASE1(SS$_ACCVIO):
        VALUE(EFAULT); /* bad address */
    CASE2(RMS$_DNR, SS$_DEVASSIGN):
    CASE2(SS$_DEVALLOC, SS$_DEVALRALLOC):
    CASE2(SS$_DEVMOUNT, SS$_DEVACTIVE):
        VALUE(EBUSY); /* mount device busy codes to name a few */
    CASE2(RMS$_FEX, SS$_FILALRACC):
        VALUE(EEXIST); /* file exists */
    CASE2(RMS$_IDR, SS$_BADIRECTORY):
        VALUE(ENOTDIR); /* not a directory */
    CASE1(SS$_NOIOCHAN):
        VALUE(EMFILE); /* too many open files */
    CASE1(RMS$_FUL):
    CASE2(SS$_DEVICEFULL, SS$_EXDISKQUOTA):
        VALUE(ENOSPC); /* no space left on disk codes */
    CASE2(RMS$_WLK, SS$_WRITLCK):
        VALUE(EROFS); /* read-only file system */
    default:
        VALUE(EVMSERR);
    };
/* clang-format on */
/* *INDENT-ON* */

    errno = trans;
    vaxc$errno = code;
    return code; /* (not very useful) */
}

#undef VALUE
#undef CASE1
#undef CASE2

static char base_name[NAM$C_MAXRSS + 1];

/* return a copy of the 'base' portion of a filename */
char *
vms_basename(const char *name)
{
    unsigned len;
    char *base, *base_p;
    register const char *name_p;

    /* skip directory/path */
    if ((name_p = strrchr(name, ']')) != 0)
        name = name_p + 1;
    if ((name_p = strrchr(name, '>')) != 0)
        name = name_p + 1;
    if ((name_p = strrchr(name, ':')) != 0)
        name = name_p + 1;
    if ((name_p = strrchr(name, '/')) != 0)
        name = name_p + 1;
    if (!*name)
        name = "."; /* this should never happen */

    /* find extension/version and derive length of basename */
    if ((name_p = strchr(name, '.')) == 0 || name_p == name)
        name_p = strchr(name, ';');
    len = (name_p && name_p > name) ? name_p - name : strlen(name);

    /* return a lowercase copy of the name in a private static buffer */
    base = strncpy(base_name, name, len);
    base[len] = '\0';
    /* we don't use lcase() so that utilities won't need hacklib.c */
    for (base_p = base; base_p < &base[len]; base_p++)
        if (isupper(*base_p))
            *base_p = tolower(*base_p);

    return base;
}

/*vmsfiles.c*/
