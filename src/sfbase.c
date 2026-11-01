/* NetHack 5.0	sfbase.c.template $NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* Copyright (c) Michael Allison, 2025. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "sfprocs.h"

#ifdef SFCTOOL
//#include "sfproto.h"
#endif

/* #define DO_DEBUG */

//#define TURN_OFF_LOGGING 0x20
#define TURN_OFF_LOGGING (UNCONVERTING << 1)

struct sf_structlevel_procs sfoprocs[NUM_SAVEFORMATS], sfiprocs[NUM_SAVEFORMATS],
                zerosfoprocs = {0}, zerosfiprocs = {0};
struct sf_fieldlevel_procs sfoflprocs[NUM_SAVEFORMATS], sfiflprocs[NUM_SAVEFORMATS],
                zerosfoflprocs = {0}, zerosfiflprocs = {0};

char *sfvalue_aligntyp(aligntyp *a);
char *sfvalue_any(anything *a);
char *sfvalue_genericptr(genericptr a);
char *sfvalue_int16(int16 *a);
char *sfvalue_int32(int32 *a);
char *sfvalue_int64(int64 *a);
char *sfvalue_uchar(uchar *a);
char *sfvalue_uint16(uint16 *a);
char *sfvalue_uint32(uint32 *a);
char *sfvalue_uint64(uint64 *a);
char *sfvalue_size_t(size_t *a);
char *sfvalue_time_t(time_t *a);
char *sfvalue_short(short *a);
char *sfvalue_ushort(ushort *a);
char *sfvalue_int(int *a);
char *sfvalue_unsigned(unsigned *a);
char *sfvalue_long(long *a);
char *sfvalue_ulong(ulong *a);
char *sfvalue_xint8(xint8 *a);
char *sfvalue_xint16(xint16 *a);
char *sfvalue_char(char *a, int n);
char *sfvalue_boolean(boolean *a);
char *sfvalue_schar(schar *a);
char *sfvalue_bitfield(uint8 *a);
char *complex_dump(uchar *a);
char *bitfield_dump(uint8 *a);

void sf_log(NHFILE *, const char *, size_t, int, char *);

#if NH_C < 202300L
#define Sfvalue_aligntyp(a) sfvalue_aligntyp(a)
#define Sfvalue_any(a) sfvalue_any(a)
#define Sfvalue_genericptr(a) sfvalue_genericptr(a)
#define Sfvalue_coordxy(a) sfvalue_int16(a)
#define Sfvalue_int16(a) sfvalue_int16(a)
#define Sfvalue_int32(a) sfvalue_int32(a)
#define Sfvalue_int64(a) sfvalue_int64(a)
#define Sfvalue_uchar(a) sfvalue_uchar(a)
#define Sfvalue_uint16(a) sfvalue_uint16(a)
#define Sfvalue_uint32(a) sfvalue_uint32(a)
#define Sfvalue_uint64(a) sfvalue_uint64(a)
#define Sfvalue_size_t(a) sfvalue_size_t(a)
#define Sfvalue_time_t(a) sfvalue_time_t(a)
#define Sfvalue_short(a) sfvalue_short(a)
#define Sfvalue_ushort(a) sfvalue_ushort(a)
#define Sfvalue_int(a) sfvalue_int(a)
#define Sfvalue_unsigned(a) sfvalue_unsigned(a)
#define Sfvalue_xint8(a) sfvalue_xint8(a)
#define Sfvalue_xint16(a) sfvalue_xint16(a)

#else

#define sfvalue(x)                          \
    _Generic( (x),                          \
        anything *: sfvalue_any,            \
        genericptr_t *: sfvalue_genericptr, \
        int16_t *: sfvalue_int16,           \
        int32_t *: sfvalue_int32,           \
        int64_t *: sfvalue_int64,           \
        uchar *: sfvalue_uchar,             \
        uint16_t *: sfvalue_uint16,         \
        uint32_t *: sfvalue_uint32,         \
        uint64_t *: sfvalue_uint64,         \
        xint8 *: sfvalue_xint8              \
    )(x)

#define Sfvalue_any(a) sfvalue(a)
#define Sfvalue_aligntyp(a) sfvalue(a)
#define Sfvalue_genericptr(a) sfvalue(a)
#define Sfvalue_coordxy(a) sfvalue(a)
#define Sfvalue_int16(a) sfvalue(a)
#define Sfvalue_int32(a) sfvalue(a)
#define Sfvalue_int64(a) sfvalue(a)
#define Sfvalue_uchar(a) sfvalue(a)
#define Sfvalue_unsigned(a) sfvalue(a)
#define Sfvalue_uchar(a) sfvalue(a)
#define Sfvalue_uint16(a) sfvalue(a)
#define Sfvalue_uint32(a) sfvalue(a)
#define Sfvalue_uint64(a) sfvalue(a)
#define Sfvalue_short(a) sfvalue(a)
#define Sfvalue_ushort(a) sfvalue(a)
#define Sfvalue_int(a) sfvalue(a)
#define Sfvalue_unsigned(a) sfvalue(a)
#define Sfvalue_xint8(a) sfvalue(a)
#define Sfvalue_xint16(a) sfvalue(a)
#endif

/* not in _Generic */ 
#define Sfvalue_long(a) sfvalue_long(a)
#define Sfvalue_ulong(a) sfvalue_ulong(a)
#define Sfvalue_char(a, d) sfvalue_char(a, d)
#define Sfvalue_boolean(a) sfvalue_boolean(a)
#define Sfvalue_schar(a) sfvalue_schar(a)
#define Sfvalue_bitfield(a) sfvalue_bitfield(a)
#define Sfvalue_time_t(a) sfvalue_time_t(a)
#define Sfvalue_size_t(a) sfvalue_size_t(a)

#define SF_A(dtyp) \
void sfo_##dtyp(NHFILE *nhfp, dtyp *d_##dtyp, const char *myname)               \
{                                                                               \
    if (nhfp->fplog)                                                            \
        sf_log(nhfp, myname, sizeof *d_##dtyp, 1, Sfvalue_##dtyp(d_##dtyp));    \
    if (nhfp->structlevel) {                                                    \
        (*sfoprocs[nhfp->fnidx].fn.sf_##dtyp)(nhfp, d_##dtyp, myname);          \
    } else {                                                                    \
        FILE *save_fplog = nhfp->fplog;                                         \
                                                                                \
        nhfp->fplog = 0;                                                        \
        (*sfoflprocs[nhfp->fnidx].fn_x.sf_##dtyp)(nhfp, d_##dtyp, myname);      \
        nhfp->fplog = save_fplog;                                               \
    }                                                                           \
}                                                                               \
                                                                                \
void sfi_##dtyp(NHFILE *nhfp, dtyp *d_##dtyp, const char *myname)               \
{                                                                               \
    if (nhfp->structlevel) {                                                    \
        (*sfiprocs[nhfp->fnidx].fn.sf_##dtyp)(nhfp, d_##dtyp, myname);          \
    } else {                                                                    \
        int save_mode = nhfp->mode;                                             \
                                                                                \
        nhfp->mode &= ~(CONVERTING | UNCONVERTING);                             \
        nhfp->mode |= TURN_OFF_LOGGING;                                         \
        (*sfiflprocs[nhfp->fnidx].fn_x.sf_##dtyp)(nhfp, d_##dtyp, myname);      \
        nhfp->mode = save_mode;                                                 \
    }                                                                           \
    if (!nhfp->eof) {                                                           \
        if ((((nhfp->mode & CONVERTING) != 0)                                   \
            || ((nhfp->mode & UNCONVERTING) != 0)) && nhfp->nhfpconvert)  {     \
            sfo_##dtyp(nhfp->nhfpconvert, d_##dtyp, myname);                    \
        }                                                                       \
        if (nhfp->fplog)                                                        \
            sf_log(nhfp, myname, sizeof *d_##dtyp, 1,                           \
                   Sfvalue_##dtyp(d_##dtyp));                                   \
    }                                                                           \
}

#define SF_C(keyw, dtyp) \
void sfo_##dtyp(NHFILE *nhfp, keyw dtyp *d_##dtyp, const char *myname)          \
{                                                                               \
    if (nhfp->fplog)                                                            \
        sf_log(nhfp, myname, sizeof *d_##dtyp, 1,                               \
               complex_dump((uchar *) d_##dtyp));                               \
    if (nhfp->structlevel) {                                                    \
        (*sfoprocs[nhfp->fnidx].fn.sf_##dtyp)(nhfp, d_##dtyp, myname);          \
    } else {                                                                    \
        FILE *save_fplog = nhfp->fplog;                                         \
                                                                                \
        nhfp->fplog = 0;                                                        \
        (*sfoflprocs[nhfp->fnidx].fn_x.sf_##dtyp)(nhfp, d_##dtyp, myname);      \
        nhfp->fplog = save_fplog;                                               \
    }                                                                           \
}                                                                               \
                                                                                \
void sfi_##dtyp(NHFILE *nhfp, keyw dtyp *d_##dtyp, const char *myname)          \
{                                                                               \
    if (nhfp->structlevel) {                                                    \
        (*sfiprocs[nhfp->fnidx].fn.sf_##dtyp)(nhfp, d_##dtyp, myname);          \
    } else {                                                                    \
        int save_mode = nhfp->mode;                                             \
                                                                                \
        nhfp->mode &= ~(CONVERTING | UNCONVERTING);                             \
        nhfp->mode |= TURN_OFF_LOGGING;                                         \
        (*sfiflprocs[nhfp->fnidx].fn_x.sf_##dtyp)(nhfp, d_##dtyp, myname);      \
        nhfp->mode = save_mode;                                                 \
    }                                                                           \
    if (!nhfp->eof) {                                                           \
        if ((((nhfp->mode & CONVERTING) != 0)                                   \
            || ((nhfp->mode & UNCONVERTING) != 0))                              \
            && nhfp->nhfpconvert) {                                             \
            sfo_##dtyp(nhfp->nhfpconvert, d_##dtyp, myname);                    \
        }                                                                       \
        if (nhfp->fplog)                                                        \
            sf_log(nhfp, myname, sizeof *d_##dtyp, 1,                           \
                       complex_dump((uchar *) d_##dtyp));                       \
    }                                                                           \
}
  
#define SF_X(xxx, dtyp) \
void sfo_##dtyp(NHFILE *nhfp, xxx *d_##dtyp, const char *myname, int bfsz)      \
{                                                                               \
    if (nhfp->fplog)                                                            \
            sf_log(nhfp, myname, sizeof *d_##dtyp, 1,                           \
                   Sfvalue_##dtyp(d_##dtyp));                                   \
    if (nhfp->structlevel) {                                                    \
        (*sfoprocs[nhfp->fnidx].fn.sf_##dtyp)(nhfp, d_##dtyp, myname, bfsz);    \
    } else {                                                                    \
        FILE *save_fplog = nhfp->fplog;                                         \
                                                                                \
        nhfp->fplog = 0;                                                        \
        (*sfoflprocs[nhfp->fnidx].fn_x.sf_##dtyp)(nhfp, d_##dtyp,               \
                                                  myname, bfsz);                \
        nhfp->fplog = save_fplog;                                               \
    }                                                                           \
    if (nhfp->fplog && !nhfp->eof)                                              \
        sf_log(nhfp, myname, sizeof *d_##dtyp, 1, Sfvalue_##dtyp(d_##dtyp));    \
}                                                                               \
                                                                                \
void sfi_##dtyp(NHFILE *nhfp, xxx *d_##dtyp, const char *myname, int bfsz)      \
{                                                                               \
    if (nhfp->structlevel) {                                                    \
        (*sfiprocs[nhfp->fnidx].fn.sf_##dtyp)(nhfp, d_##dtyp, myname, bfsz);    \
    } else {                                                                    \
        int save_mode = nhfp->mode;                                             \
                                                                                \
        nhfp->mode &= ~(CONVERTING | UNCONVERTING);                             \
        nhfp->mode |= TURN_OFF_LOGGING;                                         \
        (*sfiflprocs[nhfp->fnidx].fn_x.sf_##dtyp)(nhfp, d_##dtyp,               \
                                                  myname, bfsz);                \
        nhfp->mode = save_mode;                                                 \
    }                                                                           \
    if (!nhfp->eof) {                                                           \
        if ((((nhfp->mode & CONVERTING) != 0)                                   \
            || ((nhfp->mode & UNCONVERTING) != 0))                              \
                       && nhfp->nhfpconvert) {                                  \
            sfo_##dtyp(nhfp->nhfpconvert, d_##dtyp, myname, bfsz);              \
        }                                                                       \
        if (nhfp->fplog)                                                        \
            sf_log(nhfp, myname, sizeof *d_##dtyp, 1,                           \
                   dtyp##_dump(d_##dtyp));                                      \
    }                                                                           \
}

#include "sfmacros.h"

SF_X(uint8_t, bitfield)

void
sfo_char(NHFILE *nhfp, char *d_char, const char *myname, int cnt)
{
    if (nhfp->fplog)
        sf_log(nhfp, myname, sizeof(char), cnt, Sfvalue_char(d_char, cnt));
    if (nhfp->structlevel) {
        (*sfoprocs[nhfp->fnidx].fn.sf_char)(nhfp, d_char, myname, cnt);
    } else {
        FILE *save_fplog = nhfp->fplog;

        nhfp->fplog = 0;
        (*sfoflprocs[nhfp->fnidx].fn_x.sf_char)(nhfp, d_char, myname, cnt);
        nhfp->fplog = save_fplog;
    }
}

void
sfi_char(NHFILE *nhfp, char *d_char, const char *myname, int cnt)
{
    if (nhfp->structlevel) {
        (*sfiprocs[nhfp->fnidx].fn.sf_char)(nhfp, d_char, myname, cnt);
    } else {
        int save_mode = nhfp->mode;

        nhfp->mode &= ~(CONVERTING | UNCONVERTING);
        nhfp->mode |= TURN_OFF_LOGGING;
        (*sfiflprocs[nhfp->fnidx].fn_x.sf_char)(nhfp, d_char, myname, cnt);
        nhfp->mode = save_mode;
    }
    if (!nhfp->eof) {
        if ((((nhfp->mode & CONVERTING) != 0)
             || ((nhfp->mode & UNCONVERTING) != 0))
            && nhfp->nhfpconvert) {
            sfo_char(nhfp->nhfpconvert, d_char, myname, cnt);
        }
        if (nhfp->fplog)
            sf_log(nhfp, myname, sizeof(char), cnt,
                   Sfvalue_char(d_char, cnt));
    }
}

void
sfo_genericptr(NHFILE *nhfp, void **d_genericptr, const char *myname)
{
    if (nhfp->fplog)
        sf_log(nhfp, myname, sizeof *d_genericptr, 1,
               Sfvalue_genericptr(d_genericptr));
    if (nhfp->structlevel) {
        (*sfoprocs[nhfp->fnidx].fn.sf_genericptr)(nhfp, d_genericptr, myname);
    } else {
        FILE *save_fplog = nhfp->fplog;
        nhfp->fplog = 0;
        (*sfoflprocs[nhfp->fnidx].fn_x.sf_genericptr)(nhfp, d_genericptr,
                                                    myname);
        nhfp->fplog = save_fplog;
    }
}
void
sfi_genericptr(NHFILE *nhfp, void **d_genericptr, const char *myname)
{
    if (nhfp->structlevel) {
        (*sfiprocs[nhfp->fnidx].fn.sf_genericptr)(nhfp, d_genericptr, myname);
    } else {
        int save_mode = nhfp->mode;
        nhfp->mode &= ~(CONVERTING | UNCONVERTING);
        nhfp->mode |= TURN_OFF_LOGGING;
        (*sfiflprocs[nhfp->fnidx].fn_x.sf_genericptr)(nhfp, d_genericptr,
                                                    myname);
        nhfp->mode = save_mode;
    }
    if (!nhfp->eof) {
        if ((((nhfp->mode & CONVERTING) != 0) || ((nhfp->mode & UNCONVERTING) != 0))
            && nhfp->nhfpconvert) {
            sfo_genericptr(nhfp->nhfpconvert, d_genericptr, myname);
        }
        if (nhfp->fplog)
            sf_log(nhfp, myname, sizeof *d_genericptr, 1,
                   Sfvalue_genericptr(d_genericptr));
    }
}

void
sfo_version_info(NHFILE *nhfp, struct version_info *d_version_info,
                 const char *myname)
{
    if (nhfp->fplog)
        sf_log(nhfp, myname, sizeof *d_version_info, 1,
               complex_dump((uchar *) d_version_info));
    if (nhfp->structlevel) {
        (*sfoprocs[nhfp->fnidx].fn.sf_version_info)(nhfp, d_version_info,
                                                    myname);
    } else {
        FILE *save_fplog = nhfp->fplog;
        nhfp->fplog = 0;
        (*sfoflprocs[nhfp->fnidx].fn_x.sf_version_info)(nhfp, d_version_info,
                                                      myname);
        nhfp->fplog = save_fplog;
    }
}
void
sfi_version_info(NHFILE *nhfp, struct version_info *d_version_info,
                 const char *myname)
{
    if (nhfp->structlevel) {
        (*sfiprocs[nhfp->fnidx].fn.sf_version_info)(nhfp, d_version_info,
                                                    myname);
    } else {
        int save_mode = nhfp->mode;
        nhfp->mode &= ~(CONVERTING | UNCONVERTING);
        nhfp->mode |= TURN_OFF_LOGGING;
        (*sfiflprocs[nhfp->fnidx].fn_x.sf_version_info)(nhfp, d_version_info,
                                                      myname);
        nhfp->mode = save_mode;
    }
    if (!nhfp->eof) {
        if ((((nhfp->mode & CONVERTING) != 0) || ((nhfp->mode & UNCONVERTING) != 0))
            && nhfp->nhfpconvert) {
            d_version_info->feature_set |= SFCTOOL_BIT;
            sfo_version_info(nhfp->nhfpconvert, d_version_info, myname);
        }
        if (nhfp->fplog)
            sf_log(nhfp, myname, sizeof *d_version_info, 1,
                   complex_dump((uchar *) d_version_info));
    }
}

/* ---------------------------------------------------------------*/

void
sf_log(NHFILE *nhfp, const char *t1, size_t sz, int cnt, char *txtvalue)
{
    FILE *fp = nhfp->fplog;
    long *iocount;
    boolean dolog = ((nhfp->mode & TURN_OFF_LOGGING) == 0);

    if (fp && dolog) {
        iocount = ((nhfp->mode & WRITING) == 0) ? &nhfp->rcount : &nhfp->wcount;
        (void) fprintf(fp,
#ifndef VMS
                       "%08ld %s sz=%zu cnt=%d |%s|\n",
#else
                       "%08ld %s sz=%lu cnt=%d |%s|\n",
#endif
                       *iocount,
                       t1,
#ifndef VMS
                       sz,
#else
                       (unsigned long) sz,
#endif
                       cnt, txtvalue);
//        (*iocount)++;
//        if (*iocount == 87)
//            __debugbreak();
        fflush(fp);
    }
}

char *sfvalue_char(char *a, int n)
{
    int i;
    static char buf[120];
    char *cp;

    cp = &buf[0];
    if (n < (int) (sizeof buf - 1))
        buf[n] = '\0';
    else
        buf[(int) (sizeof buf - 1)] = '\0';
    for (i = 0; i < n; ++i, ++cp, ++a)
        *cp = *a;
    *cp = '\0';
    return buf;
}

char *sfvalue_boolean(boolean *a)
{
    static char buf[20];

    Snprintf(buf, sizeof buf, "%s",
             (*a == 0) ? "false" : "true");
    return buf;
}

char *sfvalue_schar(schar *a)
{
    static char buf[20];

    Snprintf(buf, sizeof buf, "%d", (int) *a);
    return buf;
}

char * sfvalue_aligntyp(aligntyp *a)
{
    static char buf[20];

    Snprintf(buf, sizeof buf, "%d", (int) *a);
    return buf;
}

char *
sfvalue_any(anything *a)
{
    static char buf[20];

    Snprintf(buf, sizeof buf,
             "%" PRId64,
             a->a_int64);
    return buf;
}

char *
sfvalue_genericptr(genericptr a)
{
    static char buf[20];

    Snprintf(buf, sizeof buf, "%s", 
             (a == 0) ? "0" : "glorkum");
    return buf;
}

char * sfvalue_int16(int16 *a)
{
    static char buf[20];

    Snprintf(buf, sizeof buf, "%d", *a);
    return buf;
}

char * sfvalue_int32(int32 *a)
{
    static char buf[20];

    Snprintf(buf, sizeof buf, "%" PRId32, *a);
    return buf;
}

char * sfvalue_int64(int64 *a)
{
    static char buf[20];
    Snprintf(buf, sizeof buf, "%" PRId64, *a);
    return buf;
}

char * sfvalue_uchar(uchar *a)
{
    static char buf[20];
    unsigned x;

    x = *a;
    Snprintf(buf, sizeof buf, "%03u", x);
    return buf;
}

char * sfvalue_uint16(uint16 *a)
{
    static char buf[20];

    Snprintf(buf, sizeof buf, "%u", (uint) *a);
    return buf;
}

char * sfvalue_uint32(uint32 *a)
{
    static char buf[20];

    Snprintf(buf, sizeof buf, "%" PRIu32, *a);
    return buf;
}

char * sfvalue_uint64(uint64 *a)
{
    static char buf[20];

    Snprintf(buf, sizeof buf, "%" PRIu64, *a);
    return buf;
}

char * sfvalue_size_t(size_t *a UNUSED)
{
    static char buf[20];

    Snprintf(buf, sizeof buf, "%s", (char *) "");
    return buf;
}

char * sfvalue_time_t(time_t *a UNUSED)
{
    static char buf[20];

    Snprintf(buf, sizeof buf, "%s", (char *) "");
    return buf;
}

char * sfvalue_short(short *a)
{
    static char buf[20];

    Snprintf(buf, sizeof buf, "%d", (int) *a);
    return buf;
}

char * sfvalue_ushort(ushort *a)
{
    static char buf[20];

    Snprintf(buf, sizeof buf, "%u", (unsigned) *a);
    return buf;
}

char * sfvalue_int(int *a)
{
    static char buf[20];

    Snprintf(buf, sizeof buf, "%d", *a);
    return buf;
}

char * sfvalue_unsigned(unsigned *a)
{
    static char buf[20];

    Snprintf(buf, sizeof buf, "%u", *a);
    return buf;
}

char * sfvalue_long(long *a)
{
    static char buf[20];

    Snprintf(buf, sizeof buf, "%ld", *a);
    return buf;
}

char * sfvalue_ulong(ulong *a)
{
    static char buf[20];

    Snprintf(buf, sizeof buf, "%lu", *a);
    return buf;
}

char * sfvalue_xint8(xint8 *a)
{
    static char buf[20];

    Snprintf(buf, sizeof buf, "%d", (int) *a);
    return buf;
}

char * sfvalue_xint16(xint16 *a)
{
    static char buf[20];


    Snprintf(buf, sizeof buf, "%d", (int) *a);
    return buf;
}

char *
sfvalue_bitfield(uint8 *a)
{
    static char buf[20];

    Snprintf(buf, sizeof buf, "%u", (uint) *a);
    return buf;
}

char *
bitfield_dump(uint8 *a)
{
    static char buf[20];

    Snprintf(buf, sizeof buf, "%u", (uint) *a);
    return buf;
}
char *
complex_dump(uchar *a)
{
    int i;
    uchar *uc = a;
    static char buf[50];
    unsigned x[10];

    for (i = 0; i < SIZE(x); ++i) {
        x[i] = *uc++;
    }
    Snprintf(buf, sizeof buf, "%03x %03x %03x %03x %03x %03x %03x %03x %03x %03x", 
             x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7], x[8], x[9]);
    buf[40] = '\0';
    return buf;
}
/*
 *----------------------------------------------------------------------------
 * initialize the function pointers. These are called from initoptions_init().
 *----------------------------------------------------------------------------
 */

void
sf_init(void)
{
    sfoprocs[invalid] = zerosfoprocs;
    sfiprocs[invalid] = zerosfiprocs;
    sfoprocs[historical] = historical_sfo_procs;
    sfiprocs[historical] = historical_sfi_procs;
    sfoflprocs[exportascii] = zerosfoflprocs;
    sfiflprocs[exportascii] = zerosfiflprocs;
}

void
sf_setprocs(int idx, struct sf_structlevel_procs *sfi, struct sf_structlevel_procs *sfo)
{
    sfoprocs[idx] = *sfo;
    sfiprocs[idx] = *sfi;
}
void
sf_setflprocs(int idx, struct sf_fieldlevel_procs *flsfi,
            struct sf_fieldlevel_procs *flsfo)
{
    sfoflprocs[idx] = *flsfo;
    sfiflprocs[idx] = *flsfi;
}

#ifndef SFCTOOL
void norm_ptrs_any(union any *d_any);
void norm_ptrs_align(struct align *d_align);
void norm_ptrs_arti_info(struct arti_info *d_arti_info);
void norm_ptrs_attribs(struct attribs *d_attribs);
void norm_ptrs_bill_x(struct bill_x *d_bill_x);
void norm_ptrs_branch(struct branch *d_branch);
void norm_ptrs_bubble(struct bubble *d_bubble);
void norm_ptrs_cemetery(struct cemetery *d_cemetery);
void norm_ptrs_context_info(struct context_info *d_context_info);
void norm_ptrs_achievement_tracking(
    struct achievement_tracking *d_achievement_tracking);
void norm_ptrs_book_info(struct book_info *d_book_info);
void norm_ptrs_dig_info(struct dig_info *d_dig_info);
void norm_ptrs_engrave_info(struct engrave_info *d_engrave_info);
void norm_ptrs_obj_split(struct obj_split *d_obj_split);
void norm_ptrs_polearm_info(struct polearm_info *d_polearm_info);
void norm_ptrs_takeoff_info(struct takeoff_info *d_takeoff_info);
void norm_ptrs_tin_info(struct tin_info *d_tin_info);
void norm_ptrs_tribute_info(struct tribute_info *d_tribute_info);
void norm_ptrs_victual_info(struct victual_info *d_victual_info);
void norm_ptrs_warntype_info(struct warntype_info *d_warntype_info);
void norm_ptrs_d_flags(struct d_flags *d_d_flags);
void norm_ptrs_d_level(struct d_level *d_d_level);
void norm_ptrs_damage(struct damage *d_damage);
void norm_ptrs_dest_area(struct dest_area *d_dest_area);
void norm_ptrs_dgn_topology(struct dgn_topology *d_dgn_topology);
void norm_ptrs_dungeon(struct dungeon *d_dungeon);
void norm_ptrs_ebones(struct ebones *d_ebones);
void norm_ptrs_edog(struct edog *d_edog);
void norm_ptrs_egd(struct egd *d_egd);
void norm_ptrs_emin(struct emin *d_emin);
void norm_ptrs_engr(struct engr *d_engr);
void norm_ptrs_epri(struct epri *d_epri);
void norm_ptrs_eshk(struct eshk *d_eshk);
void norm_ptrs_fakecorridor(struct fakecorridor *d_fakecorridor);
void norm_ptrs_fe(struct fe *d_fe);
void norm_ptrs_flag(struct flag *d_flag);
void norm_ptrs_fruit(struct fruit *d_fruit);
void norm_ptrs_gamelog_line(struct gamelog_line *d_gamelog_line);
void norm_ptrs_kinfo(struct kinfo *d_kinfo);
void norm_ptrs_levelflags(struct levelflags *d_levelflags);
void norm_ptrs_linfo(struct linfo *d_linfo);
void norm_ptrs_ls_t(struct ls_t *d_ls_t);
void norm_ptrs_mapseen_feat(struct mapseen_feat *d_mapseen_feat);
void norm_ptrs_mapseen_flags(struct mapseen_flags *d_mapseen_flags);
void norm_ptrs_mapseen_rooms(struct mapseen_rooms *d_mapseen_rooms);
void norm_ptrs_mapseen(struct mapseen *d_mapseen);
void norm_ptrs_mextra(struct mextra *d_mextra);
void norm_ptrs_mkroom(struct mkroom *d_mkroom);
void norm_ptrs_monst(struct monst *d_monst);
void norm_ptrs_mvitals(struct mvitals *d_mvitals);
void norm_ptrs_nhcoord(struct nhcoord *d_nhcoord);
void norm_ptrs_nhrect(struct nhrect *d_nhrect);
void norm_ptrs_novel_tracking(struct novel_tracking *d_novel_tracking);
void norm_ptrs_obj(struct obj *d_obj);
void norm_ptrs_objclass(struct objclass *d_objclass);
void norm_ptrs_oextra(struct oextra *d_oextra);
void norm_ptrs_prop(struct prop *d_prop);
void norm_ptrs_q_score(struct q_score *d_q_score);
void norm_ptrs_rm(struct rm *d_rm);
void norm_ptrs_s_level(struct s_level *d_s_level);
void norm_ptrs_skills(struct skills *d_skills);
void norm_ptrs_spell(struct spell *d_spell);
void norm_ptrs_stairway(struct stairway *d_stairway);
void norm_ptrs_trap(struct trap *d_trap);
void norm_ptrs_u_conduct(struct u_conduct *d_u_conduct);
void norm_ptrs_u_event(struct u_event *d_u_event);
void norm_ptrs_u_have(struct u_have *d_u_have);
void norm_ptrs_u_realtime(struct u_realtime *d_u_realtime);
void norm_ptrs_u_roleplay(struct u_roleplay *d_u_roleplay);
void norm_ptrs_version_info(struct version_info *d_version_info);
void norm_ptrs_vlaunchinfo(union vlaunchinfo *d_vlaunchinfo);
void norm_ptrs_vptrs(union vptrs *d_vptrs);
void norm_ptrs_you(struct you *d_you);

void
norm_ptrs_any(union any *d_any UNUSED)
{
}
void
norm_ptrs_align(struct align *d_align UNUSED)
{
}

void
norm_ptrs_arti_info(struct arti_info *d_arti_info UNUSED)
{
}

void
norm_ptrs_attribs(struct attribs *d_attribs UNUSED)
{
}

void
norm_ptrs_bill_x(struct bill_x *d_bill_x UNUSED)
{
}

void
norm_ptrs_branch(struct branch *d_branch UNUSED)
{
}

void
norm_ptrs_bubble(struct bubble *d_bubble UNUSED)
{
}

void
norm_ptrs_cemetery(struct cemetery *d_cemetery UNUSED)
{
}

void
norm_ptrs_context_info(struct context_info *d_context_info UNUSED)
{
}

void
norm_ptrs_achievement_tracking(struct achievement_tracking *d_achievement_tracking UNUSED)
{
}

void
norm_ptrs_book_info(struct book_info *d_book_info UNUSED)
{
}

void
norm_ptrs_dig_info(struct dig_info *d_dig_info UNUSED)
{
}

void
norm_ptrs_engrave_info(struct engrave_info *d_engrave_info UNUSED)
{
}

void
norm_ptrs_obj_split(struct obj_split *d_obj_split UNUSED)
{
}

void
norm_ptrs_polearm_info(struct polearm_info *d_polearm_info UNUSED)
{
}

void
norm_ptrs_takeoff_info(struct takeoff_info *d_takeoff_info UNUSED)
{
}

void
norm_ptrs_tin_info(struct tin_info *d_tin_info UNUSED)
{
}

void
norm_ptrs_tribute_info(struct tribute_info *d_tribute_info UNUSED)
{
}

void
norm_ptrs_victual_info(struct victual_info *d_victual_info UNUSED)
{
}

void
norm_ptrs_warntype_info(struct warntype_info *d_warntype_info UNUSED)
{
}

void
norm_ptrs_d_flags(struct d_flags *d_d_flags UNUSED)
{
}

void
norm_ptrs_d_level(struct d_level *d_d_level UNUSED)
{
}

void
norm_ptrs_damage(struct damage *d_damage UNUSED)
{
}

void
norm_ptrs_dest_area(struct dest_area *d_dest_area UNUSED)
{
}

void
norm_ptrs_dgn_topology(struct dgn_topology *d_dgn_topology UNUSED)
{
}

void
norm_ptrs_dungeon(struct dungeon *d_dungeon UNUSED)
{
}

void
norm_ptrs_ebones(struct ebones *d_ebones UNUSED)
{
}

void
norm_ptrs_edog(struct edog *d_edog UNUSED)
{
}

void
norm_ptrs_egd(struct egd *d_egd UNUSED)
{
}

void
norm_ptrs_emin(struct emin *d_emin UNUSED)
{
}

void
norm_ptrs_engr(struct engr *d_engr UNUSED)
{
}

void
norm_ptrs_epri(struct epri *d_epri UNUSED)
{
}

void
norm_ptrs_eshk(struct eshk *d_eshk UNUSED)
{
}

void
norm_ptrs_fakecorridor(struct fakecorridor *d_fakecorridor UNUSED)
{
}

void
norm_ptrs_fe(struct fe *d_fe UNUSED)
{
}

void
norm_ptrs_flag(struct flag *d_flag UNUSED)
{
}

void
norm_ptrs_fruit(struct fruit *d_fruit UNUSED)
{
}

void
norm_ptrs_gamelog_line(struct gamelog_line *d_gamelog_line UNUSED)
{
}

void
norm_ptrs_kinfo(struct kinfo *d_kinfo UNUSED)
{
}

void
norm_ptrs_levelflags(struct levelflags *d_levelflags UNUSED)
{
}

void
norm_ptrs_linfo(struct linfo *d_linfo UNUSED)
{
}

void
norm_ptrs_ls_t(struct ls_t *d_ls_t UNUSED)
{
}

void
norm_ptrs_mapseen_feat(struct mapseen_feat *d_mapseen_feat UNUSED)
{
}

void
norm_ptrs_mapseen_flags(struct mapseen_flags *d_mapseen_flags UNUSED)
{
}

void
norm_ptrs_mapseen_rooms(struct mapseen_rooms *d_mapseen_rooms UNUSED)
{
}

void
norm_ptrs_mapseen(struct mapseen *d_mapseen UNUSED)
{
}

void
norm_ptrs_mextra(struct mextra *d_mextra UNUSED)
{
}

void
norm_ptrs_mkroom(struct mkroom *d_mkroom UNUSED)
{
}

void
norm_ptrs_monst(struct monst *d_monst UNUSED)
{
}

void
norm_ptrs_mvitals(struct mvitals *d_mvitals UNUSED)
{
}

void
norm_ptrs_nhcoord(struct nhcoord *d_nhcoord UNUSED)
{
}

void
norm_ptrs_nhrect(struct nhrect *d_nhrect UNUSED)
{
}

void
norm_ptrs_novel_tracking(struct novel_tracking *d_novel_tracking UNUSED)
{
}

void
norm_ptrs_obj(struct obj *d_obj UNUSED)
{
}

void
norm_ptrs_objclass(struct objclass *d_objclass UNUSED)
{
}

void
norm_ptrs_oextra(struct oextra *d_oextra UNUSED)
{
}

void
norm_ptrs_prop(struct prop *d_prop UNUSED)
{
}

void
norm_ptrs_q_score(struct q_score *d_q_score UNUSED)
{
}

void
norm_ptrs_rm(struct rm *d_rm UNUSED)
{
}

void
norm_ptrs_s_level(struct s_level *d_s_level UNUSED)
{
}

void
norm_ptrs_skills(struct skills *d_skills UNUSED)
{
}

void
norm_ptrs_spell(struct spell *d_spell UNUSED)
{
}

void
norm_ptrs_stairway(struct stairway *d_stairway UNUSED)
{
}

void
norm_ptrs_trap(struct trap *d_trap UNUSED)
{
}

void
norm_ptrs_u_conduct(struct u_conduct *d_u_conduct UNUSED)
{
}

void
norm_ptrs_u_event(struct u_event *d_u_event UNUSED)
{
}

void
norm_ptrs_u_have(struct u_have *d_u_have UNUSED)
{
}

void
norm_ptrs_u_realtime(struct u_realtime *d_u_realtime UNUSED)
{
}

void
norm_ptrs_u_roleplay(struct u_roleplay *d_u_roleplay UNUSED)
{
}

void
norm_ptrs_version_info(struct version_info *d_version_info UNUSED)
{
}

void
norm_ptrs_vlaunchinfo(union vlaunchinfo *d_vlaunchinfo UNUSED)
{
}

void
norm_ptrs_vptrs(union vptrs *d_vptrs UNUSED)
{
}

void
norm_ptrs_you(struct you *d_you UNUSED)
{
}
#endif  /* SFCTOOL */

#undef SF_X
#undef SF_C
#undef SF_A

/* end of sfbase.c */

