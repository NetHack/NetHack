/* NetHack 3.7	sf_base.c $NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* Copyright (c) Michael Allison, 2019. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "integer.h"
#include "sfprocs.h"

struct sf_procs sfoprocs[4], sfiprocs[4],
                zerosfoprocs = {0}, zerosfiprocs = {0};

void FDECL(sfi_log, (NHFILE *, const char *, const char *, const char *, int));

/*
 *----------------------------------------------------------------------------
 * initialize the function pointers. These are called from initoptions_init().
 *----------------------------------------------------------------------------
 */

void
sf_init()
{
    sfoprocs[invalid] = zerosfoprocs;
    sfiprocs[invalid] = zerosfiprocs;
    sfoprocs[historical] = zerosfoprocs;
    sfiprocs[historical] = zerosfiprocs;
    sfoprocs[lendian] = lendian_sfo_procs;
    sfiprocs[lendian] = lendian_sfi_procs;
    sfoprocs[ascii] = ascii_sfo_procs;
    sfiprocs[ascii] = ascii_sfi_procs;
}

/*
 *----------------------------------------
 * routines called from engine core and
 * from functions in generated sfdata.c
 *----------------------------------------
 */

void
sfo_any(nhfp, d_any, myparent, myname, cnt)
 NHFILE *nhfp;
union any *d_any;
const char *myparent;
const char *myname;
int cnt;
{
    (*sfoprocs[nhfp->fnidx].fn.sf_any)(nhfp, d_any, myparent, myname, cnt);
}

void
sfo_aligntyp(nhfp, d_aligntyp, myparent, myname, cnt)
NHFILE *nhfp;
aligntyp *d_aligntyp;
const char *myparent;
const char *myname;
int cnt;
{
    (*sfoprocs[nhfp->fnidx].fn.sf_aligntyp)(nhfp, d_aligntyp, myparent, myname, cnt);
}

void
sfo_bitfield(nhfp, d_bitfield, myparent, myname, cnt)
NHFILE *nhfp;
uint8_t *d_bitfield;
const char *myparent;
const char *myname;
int cnt;
{
    (*sfoprocs[nhfp->fnidx].fn.sf_bitfield)(nhfp, d_bitfield, myparent, myname, cnt);
}

void
sfo_boolean(nhfp, d_boolean, myparent, myname, cnt)
NHFILE *nhfp;
boolean *d_boolean;
const char *myparent;
const char *myname;
int cnt;
{
    (*sfoprocs[nhfp->fnidx].fn.sf_boolean)(nhfp, d_boolean, myparent, myname, cnt);
}

void
sfo_char(nhfp, d_char, myparent, myname, cnt)
NHFILE *nhfp;
char *d_char;
const char *myparent;
const char *myname;
int cnt;
{
    (*sfoprocs[nhfp->fnidx].fn.sf_char)(nhfp, d_char, myparent, myname, cnt);
}

void
sfo_genericptr(nhfp, d_genericptr, myparent, myname, cnt)
NHFILE *nhfp;
genericptr_t d_genericptr;
const char *myparent;
const char *myname;
int cnt;
{
    (*sfoprocs[nhfp->fnidx].fn.sf_genericptr)(nhfp, d_genericptr, myparent, myname, cnt);
}

void
sfo_int(nhfp, d_int, myparent, myname, cnt)
NHFILE *nhfp;
int *d_int;
const char *myparent;
const char *myname;
int cnt;
{
    (*sfoprocs[nhfp->fnidx].fn.sf_int)(nhfp, d_int, myparent, myname, cnt);
}

void
sfo_long(nhfp, d_long, myparent, myname, cnt)
NHFILE *nhfp;
long *d_long;
const char *myparent;
const char *myname;
int cnt;
{
    (*sfoprocs[nhfp->fnidx].fn.sf_long)(nhfp, d_long, myparent, myname, cnt);
}

void
sfo_schar(nhfp, d_schar, myparent, myname, cnt)
NHFILE *nhfp;
schar *d_schar;
const char *myparent;
const char *myname;
int cnt;
{
    (*sfoprocs[nhfp->fnidx].fn.sf_schar)(nhfp, d_schar, myparent, myname, cnt);
}

void
sfo_short(nhfp, d_short, myparent, myname, cnt)
NHFILE *nhfp;
short *d_short;
const char *myparent;
const char *myname;
int cnt;
{
    (*sfoprocs[nhfp->fnidx].fn.sf_short)(nhfp, d_short, myparent, myname, cnt);
}

void
sfo_size_t(nhfp, d_size_t, myparent, myname, cnt)
NHFILE *nhfp;
size_t *d_size_t;
const char *myparent;
const char *myname;
int cnt;
{
    (*sfoprocs[nhfp->fnidx].fn.sf_size_t)(nhfp, d_size_t, myparent, myname, cnt);
}

void
sfo_time_t(nhfp, d_time_t, myparent, myname, cnt)
NHFILE *nhfp;
time_t *d_time_t;
const char *myparent;
const char *myname;
int cnt;
{
    (*sfoprocs[nhfp->fnidx].fn.sf_time_t)(nhfp, d_time_t, myparent, myname, cnt);
}

void
sfo_unsigned(nhfp, d_unsigned, myparent, myname, cnt)
NHFILE *nhfp;
unsigned *d_unsigned;
const char *myparent;
const char *myname;
int cnt;
{
    (*sfoprocs[nhfp->fnidx].fn.sf_unsigned)(nhfp, d_unsigned, myparent, myname, cnt);
}

void
sfo_uchar(nhfp, d_uchar, myparent, myname, cnt)
NHFILE *nhfp;
unsigned char *d_uchar;
const char *myparent;
const char *myname;
int cnt;
{
    (*sfoprocs[nhfp->fnidx].fn.sf_uchar)(nhfp, d_uchar, myparent, myname, cnt);
}

void
sfo_uint(nhfp, d_uint, myparent, myname, cnt)
NHFILE *nhfp;
unsigned int *d_uint;
const char *myparent;
const char *myname;
int cnt;
{
    (*sfoprocs[nhfp->fnidx].fn.sf_uint)(nhfp, d_uint, myparent, myname, cnt);
}

void
sfo_ulong(nhfp, d_ulong, myparent, myname, cnt)
NHFILE *nhfp;
unsigned long *d_ulong;
const char *myparent;
const char *myname;
int cnt;
{
    (*sfoprocs[nhfp->fnidx].fn.sf_ulong)(nhfp, d_ulong, myparent, myname, cnt);
}

void
sfo_ushort(nhfp, d_ushort, myparent, myname, cnt)
NHFILE *nhfp;
unsigned short *d_ushort;
const char *myparent;
const char *myname;
int cnt;
{
    (*sfoprocs[nhfp->fnidx].fn.sf_ushort)(nhfp, d_ushort, myparent, myname, cnt);
}

void
sfo_xchar(nhfp, d_xchar, myparent, myname, cnt)
NHFILE *nhfp;
xchar *d_xchar;
const char *myparent;
const char *myname;
int cnt;
{
    (*sfoprocs[nhfp->fnidx].fn.sf_xchar)(nhfp, d_xchar, myparent, myname, cnt);
}

void
sfo_str(nhfp, d_str, myparent, myname, cnt)
NHFILE *nhfp;
char *d_str;
const char *myparent;
const char *myname;
int cnt;
{
    (*sfoprocs[nhfp->fnidx].fn.sf_str)(nhfp, d_str, myparent, myname, cnt);
}

void
sfo_addinfo(nhfp, parent, action, myname, index)
NHFILE *nhfp;
const char *parent, *action, *myname;
int index;
{
    (*sfoprocs[nhfp->fnidx].fn.sf_addinfo)(nhfp, parent, action, myname, index);
}

/*
 *----------------------------------------------------------------------------
 * routines called from core and from functions in generated sfi_data.c
 *----------------------------------------------------------------------------
 */

void
sfi_any(nhfp, d_any, myparent, myname, cnt)
NHFILE *nhfp;
union any *d_any;
const char *myparent;
const char *myname;
int cnt;
{
    const char *parent = "any";

    sfi_log(nhfp, myparent, myname, parent, cnt);
    (*sfiprocs[nhfp->fnidx].fn.sf_any)(nhfp, d_any, myparent, myname, cnt);
#ifdef DO_DEBUG
    if (nhfp->fpdebug)
        fprintf(nhfp->fpdebug, "(%ld)\n", d_any->a_long);
#endif
}

void
sfi_aligntyp(nhfp, d_aligntyp, myparent, myname, cnt)
NHFILE *nhfp;
aligntyp *d_aligntyp;
const char *myparent;
const char *myname;
int cnt;
{
    const char *parent = "aligntyp";

    sfi_log(nhfp, myparent, myname, parent, cnt);
    (*sfiprocs[nhfp->fnidx].fn.sf_aligntyp)(nhfp, d_aligntyp, myparent, myname, cnt);
#ifdef DO_DEBUG
    if (nhfp->fpdebug)
        fprintf(nhfp->fpdebug, "(%d)\n", *d_aligntyp);
#endif
}

void
sfi_bitfield(nhfp, d_bitfield, myparent, myname, cnt)
NHFILE *nhfp;
uint8_t *d_bitfield;
const char *myparent;
const char *myname;
int cnt;
{
    const char *parent = "bitfield";

    sfi_log(nhfp, myparent, myname, parent, cnt);
    (*sfiprocs[nhfp->fnidx].fn.sf_bitfield)(nhfp, d_bitfield, myparent, myname, cnt);
#ifdef DO_DEBUG
    if (nhfp->fpdebug)
        fprintf(nhfp->fpdebug, "(%hd)\n", *d_bitfield);
#endif
}

void
sfi_boolean(nhfp, d_boolean, myparent, myname, cnt)
NHFILE *nhfp;
boolean *d_boolean;
const char *myparent;
const char *myname;
int cnt;
{
    const char *parent = "boolean";

    sfi_log(nhfp, myparent, myname, parent, cnt);
    (*sfiprocs[nhfp->fnidx].fn.sf_boolean)(nhfp, d_boolean, myparent, myname, cnt);
#ifdef DO_DEBUG
    if (nhfp->fpdebug)
        fprintf(nhfp->fpdebug, "(%s)\n", (*d_boolean) ? "true" : "false");
#endif
}

void
sfi_char(nhfp, d_char, myparent, myname, cnt)
NHFILE *nhfp;
char *d_char;
const char *myparent;
const char *myname;
int cnt;
{
    const char *parent = "char";

    sfi_log(nhfp, myparent, myname, parent, cnt);
    (*sfiprocs[nhfp->fnidx].fn.sf_char)(nhfp, d_char, myparent, myname, cnt);
#ifdef DO_DEBUG
    if (nhfp->fpdebug)
        fprintf(nhfp->fpdebug, "(%s)\n", d_char ? d_char : "");
#endif
}

void
sfi_genericptr(nhfp, d_genericptr, myparent, myname, cnt)
NHFILE *nhfp;
genericptr_t d_genericptr;
const char *myparent;
const char *myname;
int cnt;
{
    const char *parent = "genericptr";

    sfi_log(nhfp, myparent, myname, parent, cnt);
    (*sfiprocs[nhfp->fnidx].fn.sf_genericptr)(nhfp, d_genericptr, myparent, myname, cnt);
#ifdef DO_DEBUG
    if (nhfp->fpdebug)
        fprintf(nhfp->fpdebug, "(%s)\n", (d_genericptr) ? "set" : "null");
#endif
}

void
sfi_int(nhfp, d_int, myparent, myname, cnt)
NHFILE *nhfp;
int *d_int;
const char *myparent;
const char *myname;
int cnt;
{
    const char *parent = "int";

    sfi_log(nhfp, myparent, myname, parent, cnt);
    (*sfiprocs[nhfp->fnidx].fn.sf_int)(nhfp, d_int, myparent, myname, cnt);
#ifdef DO_DEBUG
    if (nhfp->fpdebug)
        fprintf(nhfp->fpdebug, "(%d)\n", *d_int);
#endif
}

void
sfi_long(nhfp, d_long, myparent, myname, cnt)
NHFILE *nhfp;
long *d_long;
const char *myparent;
const char *myname;
int cnt;
{
    const char *parent = "long";

    sfi_log(nhfp, myparent, myname, parent, cnt);
    (*sfiprocs[nhfp->fnidx].fn.sf_long)(nhfp, d_long, myparent, myname, cnt);
#ifdef DO_DEBUG
    if (nhfp->fpdebug)
        fprintf(nhfp->fpdebug, "(%ld)\n", *d_long);
#endif
}

void
sfi_schar(nhfp, d_schar, myparent, myname, cnt)
NHFILE *nhfp;
schar *d_schar;
const char *myparent;
const char *myname;
int cnt;
{
    const char *parent = "schar";

    sfi_log(nhfp, myparent, myname, parent, cnt);
    (*sfiprocs[nhfp->fnidx].fn.sf_schar)(nhfp, d_schar, myparent, myname, cnt);
#ifdef DO_DEBUG
    if (nhfp->fpdebug)
        fprintf(nhfp->fpdebug, "(%hd)\n", (short) *d_schar);
#endif
}

void
sfi_short(nhfp, d_short, myparent, myname, cnt)
NHFILE *nhfp;
short *d_short;
const char *myparent;
const char *myname;
int cnt;
{
    const char *parent = "short";

    sfi_log(nhfp, myparent, myname, parent, cnt);
    (*sfiprocs[nhfp->fnidx].fn.sf_short)(nhfp, d_short, myparent, myname, cnt);
#ifdef DO_DEBUG
    if (nhfp->fpdebug)
        fprintf(nhfp->fpdebug, "(%hd)\n", *d_short);
#endif
}

void
sfi_size_t(nhfp, d_size_t, myparent, myname, cnt)
NHFILE *nhfp;
size_t *d_size_t;
const char *myparent;
const char *myname;
int cnt;
{
    const char *parent = "size_t";

    sfi_log(nhfp, myparent, myname, parent, cnt);
    (*sfiprocs[nhfp->fnidx].fn.sf_size_t)(nhfp, d_size_t, myparent, myname, cnt);
#ifdef DO_DEBUG
    if (nhfp->fpdebug)
        fprintf(nhfp->fpdebug, "(%lu)\n", (unsigned long) *d_size_t);
#endif
}

void
sfi_time_t(nhfp, d_time_t, myparent, myname, cnt)
NHFILE *nhfp;
time_t *d_time_t;
const char *myparent;
const char *myname;
int cnt;
{
    const char *parent = "time_t";

    sfi_log(nhfp, myparent, myname, parent, cnt);
    (*sfiprocs[nhfp->fnidx].fn.sf_time_t)(nhfp, d_time_t, myparent, myname, cnt);
#ifdef DO_DEBUG
    if (nhfp->fpdebug)
        fprintf(nhfp->fpdebug, "(%s)\n", yyyymmddhhmmss(*d_time_t));
#endif
}

void
sfi_unsigned(nhfp, d_unsigned, myparent, myname, cnt)
NHFILE *nhfp;
unsigned *d_unsigned;
const char *myparent;
const char *myname;
int cnt;
{
    const char *parent = "unsigned";

    sfi_log(nhfp, myparent, myname, parent, cnt);
    (*sfiprocs[nhfp->fnidx].fn.sf_unsigned)(nhfp, d_unsigned, myparent, myname, cnt);
#ifdef DO_DEBUG
    if (nhfp->fpdebug)
        fprintf(nhfp->fpdebug, "(%u)\n", *d_unsigned);
#endif
}

void
sfi_uchar(nhfp, d_uchar, myparent, myname, cnt)
NHFILE *nhfp;
unsigned char *d_uchar;
const char *myparent;
const char *myname;
int cnt;
{
    const char *parent = "uchar";

    sfi_log(nhfp, myparent, myname, parent, cnt);
    (*sfiprocs[nhfp->fnidx].fn.sf_uchar)(nhfp, d_uchar, myparent, myname, cnt);
#ifdef DO_DEBUG
    if (nhfp->fpdebug)
        fprintf(nhfp->fpdebug, "(%hu)\n", (unsigned short) *d_uchar);
#endif
}

void
sfi_uint(nhfp, d_uint, myparent, myname, cnt)
NHFILE *nhfp;
unsigned int *d_uint;
const char *myparent;
const char *myname;
int cnt;
{
    const char *parent = "uint";

    sfi_log(nhfp, myparent, myname, parent, cnt);
    (*sfiprocs[nhfp->fnidx].fn.sf_uint)(nhfp, d_uint, myparent, myname, cnt);
#ifdef DO_DEBUG
    if (nhfp->fpdebug)
        fprintf(nhfp->fpdebug, "(%u)\n", *d_uint);
#endif
}

void
sfi_ulong(nhfp, d_ulong, myparent, myname, cnt)
NHFILE *nhfp;
unsigned long *d_ulong;
const char *myparent;
const char *myname;
int cnt;
{
    const char *parent = "ulong";

    sfi_log(nhfp, myparent, myname, parent, cnt);
    (*sfiprocs[nhfp->fnidx].fn.sf_ulong)(nhfp, d_ulong, myparent, myname, cnt);
#ifdef DO_DEBUG
    if (nhfp->fpdebug)
        fprintf(nhfp->fpdebug, "(%lu)\n", *d_ulong);
#endif
}

void
sfi_ushort(nhfp, d_ushort, myparent, myname, cnt)
NHFILE *nhfp;
unsigned short *d_ushort;
const char *myparent;
const char *myname;
int cnt;
{
    const char *parent = "ushort";

    sfi_log(nhfp, myparent, myname, parent, cnt);
    (*sfiprocs[nhfp->fnidx].fn.sf_ushort)(nhfp, d_ushort, myparent, myname, cnt);
#ifdef DO_DEBUG
    if (nhfp->fpdebug)
        fprintf(nhfp->fpdebug, "(%hu)\n", *d_ushort);
#endif
}

void
sfi_xchar(nhfp, d_xchar, myparent, myname, cnt)
NHFILE *nhfp;
xchar *d_xchar;
const char *myparent;
const char *myname;
int cnt;
{
    const char *parent = "xchar";

    sfi_log(nhfp, myparent, myname, parent, cnt);
    (*sfiprocs[nhfp->fnidx].fn.sf_xchar)(nhfp, d_xchar, myparent, myname, cnt);
#ifdef DO_DEBUG
    if (nhfp->fpdebug)
        fprintf(nhfp->fpdebug, "(%hd)\n", (short) *d_xchar);
#endif
}
 
void
sfi_str(nhfp, d_str, myparent, myname, cnt)
NHFILE *nhfp;
char *d_str;
const char *myparent;
const char *myname;
int cnt;
{
    const char *parent = "str";

    sfi_log(nhfp, myparent, myname, parent, cnt);
    (*sfiprocs[nhfp->fnidx].fn.sf_str)(nhfp, d_str, myparent, myname, cnt);
#ifdef DO_DEBUG
    if (nhfp->fpdebug)
        fprintf(nhfp->fpdebug, "(\"%s\")\n", d_str);
#endif
}

void 
sfi_addinfo(nhfp, myparent, action, myname, index)
NHFILE *nhfp;
const char *myparent, *action, *myname;
int index;
{
    if (nhfp) {
        if (nhfp->fplog) 
            (void) fprintf(nhfp->fplog, "# %s %s %s %d\n", myparent, action, myname, index);
#ifdef DO_DEBUG
        if (nhfp->fpdebug)
            (void) fprintf(nhfp->fpdebug, "# %s %s %s %d\n", myparent, action, myname, index);
#endif
    }
    (*sfiprocs[nhfp->fnidx].fn.sf_addinfo)(nhfp, myparent, action, myname, index);
}

void
sfi_log(nhfp, t1, t2, t3, cnt)
NHFILE *nhfp;
const char *t1, *t2, *t3;
int cnt;
{
#ifdef DO_DEBUG
#ifdef SAVEFILE_DEBUGGING
    if (nhfp) {
        if (nhfp->fplog) 
            (void) fprintf(nhfp->fplog, "%s %s %s cnt=%d\n", t1, t2, t3, cnt);
        if (nhfp->fpdebug)
            (void) fprintf(nhfp->fpdebug, "%s %s %s cnt=%d ", t1, t2, t3, cnt);
    }
#endif
#else
    nhUse(nhfp);
    nhUse(t1);
    nhUse(t2);
    nhUse(t3);
    nhUse(cnt);
#endif
}

