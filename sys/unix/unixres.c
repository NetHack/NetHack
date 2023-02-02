/* NetHack 3.7	unixres.c	$NHDT-Date: 1596498298 2020/08/03 23:44:58 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.14 $ */
/* Copyright (c) Slash'EM development team, 2001. */
/* NetHack may be freely redistributed.  See license for details. */

/* [ALI] This module defines nh_xxx functions to replace getuid etc which
 * will hide privileges from the caller if so desired.
 *
 * Currently supported UNIX variants:
 *      Linux version 2.1.44 and above
 *      FreeBSD (versions unknown)
 *
 * Note: SunOS and Solaris have no mechanism for retrieving the saved id,
 * so temporarily dropping privileges on these systems is sufficient to
 * hide them.
 */

#include "config.h"

#ifdef GETRES_SUPPORT

#if defined(LINUX)

/* requires dynamic linking with libc */
#include <dlfcn.h>

static int
real_getresuid(uid_t *ruid, uid_t *euid, uid_t *suid)
{
    int (*f)(uid_t *, uid_t *, uid_t *); /* getresuid signature */

    f = dlsym(RTLD_NEXT, "getresuid");
    if (!f)
        return -1;

    return (*f)(ruid, euid, suid);
}

static int
real_getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid)
{
    int (*f)(gid_t *, gid_t *, gid_t *); /* getresgid signature */

    f = dlsym(RTLD_NEXT, "getresgid");
    if (!f)
        return -1;

    return (*f)(rgid, egid, sgid);
}

#else
#if defined(BSD) || defined(SVR4)

#ifdef SYS_getresuid

static int
real_getresuid(uid_t *ruid, uid_t *euid, uid_t *suid)
{
    return syscall(SYS_getresuid, ruid, euid, suid);
}

#else /* SYS_getresuid */

#ifdef SVR4
#include <sys/stat.h>
#endif /* SVR4 */

static int
real_getresuid(uid_t *ruid, uid_t *euid, uid_t *suid)
{
    int retval;
    int pfd[2];
    struct stat st;

    if (pipe(pfd))
        return -1;
    retval = fstat(pfd[0], &st);
    close(pfd[0]);
    close(pfd[1]);
    if (!retval) {
        *euid = st.st_uid;
        *ruid = syscall(SYS_getuid);
        *suid = *ruid; /* Not supported under SVR4 */
    }
    return retval;
}

#endif /* SYS_getresuid */

#ifdef SYS_getresgid

static int
real_getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid)
{
    return syscall(SYS_getresgid, rgid, egid, sgid);
}

#else /* SYS_getresgid */

static int
real_getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid)
{
    int retval;
    int pfd[2];
    struct stat st;

    if (pipe(pfd))
        return -1;
    retval = fstat(pfd[0], &st);
    close(pfd[0]);
    close(pfd[1]);
    if (!retval) {
        *egid = st.st_gid;
        *rgid = syscall(SYS_getgid);
        *sgid = *rgid; /* Not supported under SVR4 */
    }
    return retval;
}

#endif /* SYS_getresgid */
#endif /* BSD || SVR4 */
#endif /* LINUX */

static unsigned int hiding_privileges = 0;

/*
 * Note: returns the value _after_ action.
 */

int
hide_privileges(boolean flag)
{
    if (flag)
        hiding_privileges++;
    else if (hiding_privileges)
        hiding_privileges--;
    return hiding_privileges;
}

int
nh_getresuid(uid_t *ruid, uid_t *euid, uid_t *suid)
{
    int retval = real_getresuid(ruid, euid, suid);

    if (!retval && hiding_privileges)
        *euid = *suid = *ruid;
    return retval;
}

uid_t
nh_getuid(void)
{
    uid_t ruid, euid, suid;

    (void) real_getresuid(&ruid, &euid, &suid);
    return ruid;
}

uid_t
nh_geteuid(void)
{
    uid_t ruid, euid, suid;

    (void) real_getresuid(&ruid, &euid, &suid);
    if (hiding_privileges)
        euid = ruid;
    return euid;
}

int
nh_getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid)
{
    int retval = real_getresgid(rgid, egid, sgid);

    if (!retval && hiding_privileges)
        *egid = *sgid = *rgid;
    return retval;
}

gid_t
nh_getgid(void)
{
    gid_t rgid, egid, sgid;

    (void) real_getresgid(&rgid, &egid, &sgid);
    return rgid;
}

gid_t
nh_getegid(void)
{
    gid_t rgid, egid, sgid;

    (void) real_getresgid(&rgid, &egid, &sgid);
    if (hiding_privileges)
        egid = rgid;
    return egid;
}

#else /* GETRES_SUPPORT */

#ifdef GNOME_GRAPHICS
int
hide_privileges(boolean flag)
{
    return 0;
}
#endif

#endif /* GETRES_SUPPORT */
