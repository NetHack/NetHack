/* NetHack 3.6	sfascii.c $NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* Copyright (c) Michael Allison, 2019. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "integer.h"
#include "sfprocs.h"

#ifdef MACOSX
extern long long FDECL(atoll, (const char *));
#endif

static void FDECL(put_savefield, (NHFILE *, char *, size_t));
char *FDECL(get_savefield, (NHFILE *, char *, size_t));
#ifdef SAVEFILE_DEBUGGING
void FDECL(report_problem_ascii, (NHFILE *, const char *, const char *, const char *));
#endif

struct sf_procs ascii_sfo_procs = {
    ".ascii",
    {
        ascii_sfo_aligntyp,
        ascii_sfo_any,
        ascii_sfo_bitfield,
        ascii_sfo_boolean,
        ascii_sfo_char,
        ascii_sfo_genericptr,
        ascii_sfo_int,
        ascii_sfo_long,
        ascii_sfo_schar,
        ascii_sfo_short,
        ascii_sfo_size_t,
        ascii_sfo_time_t,
        ascii_sfo_unsigned,
        ascii_sfo_uchar,
        ascii_sfo_uint,
        ascii_sfo_ulong,
        ascii_sfo_ushort,
        ascii_sfo_xchar,
        ascii_sfo_str,
        ascii_sfo_addinfo,
    }
};

struct sf_procs ascii_sfi_procs =
{
    ".ascii",
    {
        ascii_sfi_aligntyp,
        ascii_sfi_any,
        ascii_sfi_bitfield,
        ascii_sfi_boolean,
        ascii_sfi_char,
        ascii_sfi_genericptr,
        ascii_sfi_int,
        ascii_sfi_long,
        ascii_sfi_schar,
        ascii_sfi_short,
        ascii_sfi_size_t,
        ascii_sfi_time_t,
        ascii_sfi_unsigned,
        ascii_sfi_uchar,
        ascii_sfi_uint,
        ascii_sfi_ulong,
        ascii_sfi_ushort,
        ascii_sfi_xchar,
        ascii_sfi_str,
        ascii_sfi_addinfo,
    }
};

static char linebuf[BUFSZ];
static char outbuf[BUFSZ];

/*
 *----------------------------------------------------------------------------
 * sfo_def_ routines
 *
 * Default output routines.
 *
 *----------------------------------------------------------------------------
 */
 
void
ascii_sfo_any(nhfp, d_any, myparent, myname, cnt)
NHFILE *nhfp;
union any *d_any;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt UNUSED;
{
    const char *parent = "any";

    nhUse(parent);
    Sprintf(outbuf, "%llx", (unsigned long long) d_any->a_void);
    put_savefield(nhfp, outbuf, BUFSZ);

    Sprintf(outbuf, "%lu", d_any->a_ulong);
    put_savefield(nhfp, outbuf, BUFSZ);

    Sprintf(outbuf, "%ld", d_any->a_long);
    put_savefield(nhfp, outbuf, BUFSZ);

    Sprintf(outbuf, "%d", d_any->a_uint);
    put_savefield(nhfp, outbuf, BUFSZ);

    Sprintf(outbuf, "%d", d_any->a_int);;
    put_savefield(nhfp, outbuf, BUFSZ);

    Sprintf(outbuf, "%hd", (short) d_any->a_char);
    put_savefield(nhfp, outbuf, BUFSZ);

#if 0
    sfo_genericptr(nhfp, d_any->a_void, parent, "a_void", 1);      /* (genericptr_t)    */
    sfo_genericptr(nhfp, d_any->a_obj, parent, "a_obj", 1);        /* (struct obj *)    */
    sfo_genericptr(nhfp, d_any->a_monst, parent, "a_monst", 1);    /* (struct monst *)  */
    sfo_int(nhfp, &d_any->a_int, parent, "a_int", 1);              /* (int)             */
    sfo_char(nhfp, &d_any->a_char, parent, "a_char", 1);           /* (char)            */
    sfo_schar(nhfp, &d_any->a_schar, parent, "a_schar", 1);        /* (schar)           */
    sfo_uchar(nhfp, &d_any->a_uchar, parent, "a_uchar", 1);        /* (uchar)           */
    sfo_uint(nhfp, &d_any->a_uint, parent, "a_uint", 1);           /* (unsigned int)    */
    sfo_long(nhfp, &d_any->a_long, parent, "a_long", 1);           /* (long)            */
    sfo_ulong(nhfp, &d_any->a_ulong, parent, "a_ulong", 1);        /* (unsigned long)   */
    sfo_genericptr(nhfp, d_any->a_iptr, parent, "a_iptr", 1);      /* (int *)           */
    sfo_genericptr(nhfp, d_any->a_lptr, parent, "a_lptr", 1);      /* (long *)          */
    sfo_genericptr(nhfp, d_any->a_ulptr, parent, "a_ulptr", 1);    /* (unsigned long *) */
    sfo_genericptr(nhfp, d_any->a_uptr, parent, "a_uptr", 1);      /* (unsigned *)      */my
    sfo_genericptr(nhfp, d_any->a_string, parent, "a_string", 1);  /* (const char *)    */
    sfo_ulong(nhfp, &d_any->a_mask32, parent, "a_mask32", 1);      /* (unsigned long)   */
#endif
}

void
ascii_sfo_aligntyp(nhfp, d_aligntyp, myparent, myname, cnt)
NHFILE *nhfp;
aligntyp *d_aligntyp;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt UNUSED;
{
    int itmp;
    const char *parent = "aligntyp";

    nhUse(parent);
    itmp = (int) *d_aligntyp;
    Sprintf(outbuf, "%d", (short) itmp);
    put_savefield(nhfp, outbuf, BUFSZ);
}

void
ascii_sfo_bitfield(nhfp, d_bitfield, myparent, myname, cnt)
NHFILE *nhfp;
uint8_t *d_bitfield;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt UNUSED;
{
    const char *parent = "bitfield";

    nhUse(parent);
    /* for bitfields, cnt is the number of bits, not an array */
    Sprintf(outbuf, "%hu", (unsigned short) *d_bitfield);
    put_savefield(nhfp, outbuf, BUFSZ);
}

void
ascii_sfo_boolean(nhfp, d_boolean, myparent, myname, cnt)
NHFILE *nhfp;
boolean *d_boolean;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    const char *parent = "boolean";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        if (nhfp->fpdebug)
            fprintf(nhfp->fpdebug, "(%s)\n", (*d_boolean) ? "TRUE" : "FALSE");
        Sprintf(outbuf, "%s", *d_boolean ? "true" : "false");
        put_savefield(nhfp, outbuf, BUFSZ);
        d_boolean++;
    }
}

void
ascii_sfo_char(nhfp, d_char, myparent, myname, cnt)
NHFILE *nhfp;
char *d_char;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i = cnt;
    const char *parent = "char";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        if (nhfp->fpdebug)
            fprintf(nhfp->fpdebug, "(%s)\n", d_char ? d_char : "");
        Sprintf(outbuf, "%hd", (short) *d_char);
        put_savefield(nhfp, outbuf, BUFSZ);
        d_char++;
    }
}

void
ascii_sfo_genericptr(nhfp, d_genericptr, myparent, myname, cnt)
NHFILE *nhfp;
genericptr_t *d_genericptr;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    unsigned long tmp;
    char *byteptr = (char *) d_genericptr;
    const char *parent = "genericptr";

    nhUse(parent);
    /*
     * sbrooms is an array of pointers to mkroom.
     * That array dimension is MAX_SUBROOMS.
     * Even though the pointers themselves won't
     * be valid, we need to account for the existence
     * of that array and perhaps zero or non-zero.
     */
    for (i = 0; i < cnt; ++i) {
        tmp = (*d_genericptr) ? 1UL : 0UL;
        Sprintf(outbuf, "%08lu", tmp);
        put_savefield(nhfp, outbuf, BUFSZ);
        if (cnt > 1) {
            byteptr += sizeof(void *);
            d_genericptr = (genericptr_t) byteptr;
	}
    }
}

void
ascii_sfo_int(nhfp, d_int, myparent, myname, cnt)
NHFILE *nhfp;
int *d_int;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    const char *parent = "int";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        Sprintf(outbuf, "%d", *d_int);
        put_savefield(nhfp, outbuf, BUFSZ);
        d_int++;
    }
}

void
ascii_sfo_long(nhfp, d_long, myparent, myname, cnt)
NHFILE *nhfp;
long *d_long;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    const char *parent = "long";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        Sprintf(outbuf, "%ld", *d_long);
        put_savefield(nhfp, outbuf, BUFSZ);
        d_long++;
    }
}

void
ascii_sfo_schar(nhfp, d_schar, myparent, myname, cnt)
NHFILE *nhfp;
schar *d_schar;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i, itmp;
    const char *parent = "schar";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        itmp = (int) *d_schar;
        Sprintf(outbuf, "%d", itmp);
        put_savefield(nhfp, outbuf, BUFSZ);
        d_schar++;
    }
}

void
ascii_sfo_short(nhfp, d_short, myparent, myname, cnt)
NHFILE *nhfp;
short *d_short;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    const char *parent = "short";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        Sprintf(outbuf, "%hd", *d_short);
        put_savefield(nhfp, outbuf, BUFSZ);
        d_short++;
    }
}

void
ascii_sfo_size_t(nhfp, d_size_t, myparent, myname, cnt)
NHFILE *nhfp;
size_t *d_size_t;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    const char *parent = "size_t";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        unsigned long ul = (unsigned long) *d_size_t;

        Sprintf(outbuf, "%lu", ul);
        put_savefield(nhfp, outbuf, BUFSZ);
        d_size_t++;
    }
}

void
ascii_sfo_time_t(nhfp, d_time_t, myparent, myname, cnt)
NHFILE *nhfp;
time_t *d_time_t;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt UNUSED;
{
    const char *parent = "time_t";

    nhUse(parent);
    Sprintf(outbuf, "%s", yyyymmddhhmmss(*d_time_t));
    put_savefield(nhfp, outbuf, BUFSZ);
}

void
ascii_sfo_unsigned(nhfp, d_unsigned, myparent, myname, cnt)
NHFILE *nhfp;
unsigned *d_unsigned;
const char *myparent;
const char *myname;
int cnt;
{
    ascii_sfo_uint(nhfp, d_unsigned, myparent, myname, cnt);
}

void
ascii_sfo_uchar(nhfp, d_uchar, myparent, myname, cnt)
NHFILE *nhfp;
unsigned char *d_uchar;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    const char *parent = "uchar";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        unsigned short us = (unsigned short) *d_uchar;

        Sprintf(outbuf, "%hu", us);
        put_savefield(nhfp, outbuf, BUFSZ);
        d_uchar++;
    }
}

void
ascii_sfo_uint(nhfp, d_uint, myparent, myname, cnt)
NHFILE *nhfp;
unsigned int *d_uint;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    const char *parent = "uint";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        Sprintf(outbuf, "%u", *d_uint);
        put_savefield(nhfp, outbuf, BUFSZ);
        d_uint++;
    }
}

void
ascii_sfo_ulong(nhfp, d_ulong, myparent, myname, cnt)
NHFILE *nhfp;
unsigned long *d_ulong;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    const char *parent = "ulong";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        Sprintf(outbuf, "%lu", *d_ulong);
        put_savefield(nhfp, outbuf, BUFSZ);
        d_ulong++;
    }
}

void
ascii_sfo_ushort(nhfp, d_ushort, myparent, myname, cnt)
NHFILE *nhfp;
unsigned short *d_ushort;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    const char *parent = "ushort";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        Sprintf(outbuf, "%hu", *d_ushort);
        put_savefield(nhfp, outbuf, BUFSZ);
        d_ushort++;
    }
}

void
ascii_sfo_xchar(nhfp, d_xchar, myparent, myname, cnt)
NHFILE *nhfp;
xchar *d_xchar;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    const char *parent = "xchar";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        short tmp;

        tmp = (short) *d_xchar;
        Sprintf(outbuf, "%hu", tmp);
        put_savefield(nhfp, outbuf, BUFSZ);
        d_xchar++;
    }
}

static char strbuf[BUFSZ * 4];

void
ascii_sfo_str(nhfp, d_str, myparent, myname, cnt)
NHFILE *nhfp;
char *d_str;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i, j, intval;
    const char *parent = "str";
    char sval[QBUFSZ], *src = d_str, *dest = strbuf;

    nhUse(parent);
    /* cnt is the number of characters */
    for (i = 0; i < cnt; ++i) {
        if ((*src < 32) || (*src == '\\') || (*src > 127)) {
            *dest++ = '\\';
            intval = (int) *src++;
            Sprintf(sval, "%03d", intval);
            for (j = 0; j < 3; ++j)
                *dest++ = sval[j];
        } else {
            *dest++ = *src++;
        }
    }
    put_savefield(nhfp, strbuf, BUFSZ * 4);
}

void
ascii_sfo_addinfo(nhfp, parent, action, myname, indx)
NHFILE *nhfp UNUSED;
const char *parent UNUSED, *action UNUSED, *myname UNUSED;
int indx UNUSED;
{
    /* ignored */
}


static void
put_savefield(nhfp, obuf, outbufsz)
NHFILE *nhfp;
char *obuf;
size_t outbufsz UNUSED;
{
    nhfp->count++;
    fprintf(nhfp->fpdef, "%07ld|%s\n", nhfp->count, obuf);
}

/*
 *----------------------------------------------------------------------------
 * ascii_sfi_ routines called from functions in sfi_base.c
 *----------------------------------------------------------------------------
 */
 
void
ascii_sfi_any(nhfp, d_any, myparent, myname, cnt)
NHFILE *nhfp;
union any *d_any;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    char *rstr;
    long long tmp;
    const char *parent = "any";

    nhUse(parent);
    rstr = get_savefield(nhfp, linebuf, BUFSZ);
    tmp = atoll(rstr);
    d_any->a_void = (void *) tmp;

    rstr = get_savefield(nhfp, linebuf, BUFSZ);
    tmp = atoll(rstr);
    d_any->a_ulong = (unsigned long) tmp;

    rstr = get_savefield(nhfp, linebuf, BUFSZ);
    d_any->a_long = atol(rstr);

    rstr = get_savefield(nhfp, linebuf, BUFSZ);
    tmp = atoll(rstr);
    d_any->a_uint = (unsigned int) tmp;

    rstr = get_savefield(nhfp, linebuf, BUFSZ);
    d_any->a_int = atoi(rstr);

    rstr = get_savefield(nhfp, linebuf, BUFSZ);
    d_any->a_char = (char) atoi(rstr);

#if 0
    sfi_genericptr(nhfp, d_any->a_void, parent, "a_void", 1);
    sfi_genericptr(nhfp, d_any->a_obj, parent, "a_obj", 1);
    sfi_genericptr(nhfp, d_any->a_monst, parent, "a_monst", 1);
    sfi_int(nhfp, &d_any->a_int, parent, "a_int", 1);
    sfi_char(nhfp, &d_any->a_char, parent, "a_char", 1);
    sfi_schar(nhfp, &d_any->a_schar, parent, "a_schar", 1);
    sfi_uchar(nhfp, &d_any->a_uchar, parent, "a_uchar", 1);
    sfi_uint(nhfp, &d_any->a_uint, parent, "a_uint", 1);
    sfi_long(nhfp, &d_any->a_long, parent, "a_long", 1);
    sfi_ulong(nhfp, &d_any->a_ulong, parent, "a_ulong", 1);
    sfi_genericptr(nhfp, d_any->a_iptr, parent, "a_iptr", 1);
    sfi_genericptr(nhfp, d_any->a_lptr, parent, "a_lptr", 1);
    sfi_genericptr(nhfp, d_any->a_ulptr, parent, "a_ulptr", 1);
    sfi_genericptr(nhfp, d_any->a_uptr, parent, "a_uptr", 1);
    sfi_genericptr(nhfp, d_any->a_string, parent, "a_string", 1);
    sfi_ulong(nhfp, &d_any->a_mask32, parent, "a_mask32", 1);
#endif
}

void
ascii_sfi_aligntyp(nhfp, d_aligntyp, myparent, myname, cnt)
NHFILE *nhfp;
aligntyp *d_aligntyp;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt UNUSED;
{
    char *rstr;
    aligntyp tmp;
    long long lltmp;
    const char *parent = "aligntyp";

    nhUse(parent);
    rstr = get_savefield(nhfp, linebuf, BUFSZ);
    lltmp = atoll(rstr);
    tmp = (aligntyp) lltmp;
#ifdef SAVEFILE_DEBUGGING
    if (nhfp->structlevel && tmp != *d_aligntyp)
        report_problem_ascii(nhfp, myparent, myname, parent);
    else
#endif
    *d_aligntyp = tmp;
}

void
ascii_sfi_bitfield(nhfp, d_bitfield, myparent, myname, cnt)
NHFILE *nhfp;
uint8_t *d_bitfield;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt UNUSED;
{
    char *rstr;
    uint8_t tmp;
    const char *parent = "bitfield";

    nhUse(parent);
    /* cnt is the number of bits in the bitfield, not an array dimension */
    rstr = get_savefield(nhfp, linebuf, BUFSZ);
    tmp = (uint8_t) atoi(rstr);
#ifdef SAVEFILE_DEBUGGING
    if (nhfp->structlevel && tmp != *d_bitfield)
        report_problem_ascii(nhfp, myparent, myname, parent);
        else
#endif
    *d_bitfield = tmp;
}

void
ascii_sfi_boolean(nhfp, d_boolean, myparent, myname, cnt)
NHFILE *nhfp;
boolean *d_boolean;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    char *rstr;
    int i;
    const char *parent = "boolean";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        rstr = get_savefield(nhfp, linebuf, BUFSZ);
#ifdef SAVEFILE_DEBUGGING
        if (!strcmpi(rstr, "false") &&
                !strcmpi(rstr, "true"))
            report_problem_ascii(nhfp, myparent, myname, parent);
        else
#endif
        if (!strcmpi(rstr, "false"))
            *d_boolean = FALSE;
        else
            *d_boolean = TRUE;
        d_boolean++;
    }
}

void
ascii_sfi_char(nhfp, d_char, myparent, myname, cnt)
NHFILE *nhfp;
char *d_char;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    char *rstr;
    int i;
    char tmp;
    const char *parent = "char";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        rstr = get_savefield(nhfp, linebuf, BUFSZ);
        tmp = (char) atoi(rstr);
#ifdef SAVEFILE_DEBUGGING
        if (nhfp->structlevel && tmp != *d_char)
            report_problem_ascii(nhfp, myparent, myname, parent);
        else
#endif
        *d_char = tmp;
        d_char++;
    }
}

void
ascii_sfi_genericptr(nhfp, d_genericptr, myparent, myname, cnt)
NHFILE *nhfp;
genericptr_t *d_genericptr;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    long long lltmp;
    char *rstr;
    const char *parent = "genericptr";
    static const char *glorkum = "glorkum";
    char *byteptr = (char *) d_genericptr;

    nhUse(parent);
    /*
     * sbrooms is an array of pointers to mkroom.
     * That array dimension is MAX_SUBROOMS.
     * Even though the pointers themselves won't
     * be valid, we need to account for the existence
     * of that array.
     */
    for (i = 0; i < cnt; ++i) {
        /* these pointers can't actually be valid */
        byteptr = (char *) d_genericptr;
        rstr = get_savefield(nhfp, linebuf, BUFSZ);
        lltmp = atoll(rstr);
        *d_genericptr = lltmp ? (genericptr_t) glorkum : (genericptr_t) 0;
        if (cnt > 1) {
            byteptr += sizeof(void *);
            d_genericptr = (genericptr_t) byteptr;
	}
    }
}

void
ascii_sfi_int(nhfp, d_int, myparent, myname, cnt)
NHFILE *nhfp;
int *d_int;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i, tmp;
    char *rstr;
    long long lltmp;
    const char *parent = "int";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        rstr = get_savefield(nhfp, linebuf, BUFSZ);
        lltmp = atoll(rstr);
        tmp = (int) lltmp;
#ifdef SAVEFILE_DEBUGGING
        if (nhfp->structlevel && tmp != *d_int)
            report_problem_ascii(nhfp, myparent, myname, parent);
        else
#endif
        *d_int = tmp;
        d_int++;
    }
}

void
ascii_sfi_long(nhfp, d_long, myparent, myname, cnt)
NHFILE *nhfp;
long *d_long;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    long tmp;
    long long lltmp;
    char *rstr;
    const char *parent = "long";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        rstr = get_savefield(nhfp, linebuf, BUFSZ);
        lltmp = atoll(rstr);
        tmp = (long) lltmp;
#ifdef SAVEFILE_DEBUGGING
        if (nhfp->structlevel && tmp != *d_long)
            report_problem_ascii(nhfp, myparent, myname, parent);
        else
#endif
        *d_long = tmp;
        d_long++;
    }
}

void
ascii_sfi_schar(nhfp, d_schar, myparent, myname, cnt)
NHFILE *nhfp;
schar *d_schar;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    schar tmp;
    char *rstr;
    const char *parent = "schar";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        rstr = get_savefield(nhfp, linebuf, BUFSZ);
        tmp = (schar) atoi(rstr);
#ifdef SAVEFILE_DEBUGGING
        if (nhfp->structlevel && tmp != *d_schar)
            report_problem_ascii(nhfp, myparent, myname, parent);
        else
#endif
        *d_schar = tmp;
        d_schar++;
    }
}

void
ascii_sfi_short(nhfp, d_short, myparent, myname, cnt)
NHFILE *nhfp;
short *d_short;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    short tmp;
    char *rstr;
    const char *parent = "short";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        rstr = get_savefield(nhfp, linebuf, BUFSZ);
        tmp = (short) atoi(rstr);
#ifdef SAVEFILE_DEBUGGING
        if (nhfp->structlevel && tmp != *d_short)
            report_problem_ascii(nhfp, myparent, myname, parent);
        else
#endif
        *d_short = tmp;
        d_short++;
    }
}

void
ascii_sfi_size_t(nhfp, d_size_t, myparent, myname, cnt)
NHFILE *nhfp;
size_t *d_size_t;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    size_t tmp;
    char *rstr;
    const char *parent = "size_t";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        rstr = get_savefield(nhfp, linebuf, BUFSZ);
        tmp = (size_t) atol(rstr);
#ifdef SAVEFILE_DEBUGGING
        if (nhfp->structlevel && tmp != *d_size_t)
            report_problem_ascii(nhfp, myparent, myname, parent);
        else
#endif
        *d_size_t = tmp;
        d_size_t++;
    }
}

void
ascii_sfi_time_t(nhfp, d_time_t, myparent, myname, cnt)
NHFILE *nhfp;
time_t *d_time_t;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt UNUSED;
{
    int i;
    time_t tmp;
    char *rstr;
    const char *parent = "time_t";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        rstr = get_savefield(nhfp, linebuf, BUFSZ);
        tmp = time_from_yyyymmddhhmmss(rstr);
#ifdef SAVEFILE_DEBUGGING
        if (nhfp->structlevel && tmp != *d_time_t)
            report_problem_ascii(nhfp, myparent, myname, parent);
        else
#endif
        *d_time_t = tmp;
        d_time_t++;
    }
}

void
ascii_sfi_unsigned(nhfp, d_unsigned, myparent, myname, cnt)
NHFILE *nhfp;
unsigned *d_unsigned;
const char *myparent;
const char *myname;
int cnt;
{
    /* deferal */
    ascii_sfi_uint(nhfp, d_unsigned, myparent, myname, cnt);
}

void
ascii_sfi_uchar(nhfp, d_uchar, myparent, myname, cnt)
NHFILE *nhfp;
unsigned char *d_uchar;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    uchar tmp;
    int i, itmp;
    char *rstr;
    const char *parent = "uchar";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        rstr = get_savefield(nhfp, linebuf, BUFSZ);
        itmp = atoi(rstr);
        tmp = (char ) itmp;
#ifdef SAVEFILE_DEBUGGING
        if (nhfp->structlevel && tmp != *d_uchar)
            report_problem_ascii(nhfp, myparent, myname, parent);
        else
#endif
        *d_uchar = tmp;
        d_uchar++;
    }
}

void
ascii_sfi_uint(nhfp, d_uint, myparent, myname, cnt)
NHFILE *nhfp;
unsigned int *d_uint;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    char *rstr;
    unsigned int tmp;
    long long lltmp;
    const char *parent = "uint";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        rstr = get_savefield(nhfp, linebuf, BUFSZ);
        lltmp = atoll(rstr);
        tmp = (unsigned int) lltmp;
#ifdef SAVEFILE_DEBUGGING
        if (nhfp->structlevel && tmp != *d_uint)
            report_problem_ascii(nhfp, myparent, myname, parent);
        else
#endif
        *d_uint = tmp;
        d_uint++;
    }
}

void
ascii_sfi_ulong(nhfp, d_ulong, myparent, myname, cnt)
NHFILE *nhfp;
unsigned long *d_ulong;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    unsigned long tmp;
    long long lltmp;
    char *rstr;
    const char *parent = "ulong";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        rstr = get_savefield(nhfp, linebuf, BUFSZ);
        lltmp = atoll(rstr);
        tmp = (unsigned long) lltmp;
#ifdef SAVEFILE_DEBUGGING
        if (nhfp->structlevel && tmp != *d_ulong)
            report_problem_ascii(nhfp, myparent, myname, parent);
        else
#endif
        *d_ulong = tmp;
        d_ulong++;
    }
}

void
ascii_sfi_ushort(nhfp, d_ushort, myparent, myname, cnt)
NHFILE *nhfp;
unsigned short *d_ushort;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    short tmp;
    long long lltmp;
    char *rstr;
    const char *parent = "ushort";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        rstr = get_savefield(nhfp, linebuf, BUFSZ);
        lltmp = atoll(rstr);
        tmp = (unsigned short) lltmp;
#ifdef SAVEFILE_DEBUGGING
        if (nhfp->structlevel && tmp != *d_ushort)
            report_problem_ascii(nhfp, myparent, myname, parent);
        else
#endif
        *d_ushort = tmp;
        d_ushort++;
    }
}

void
ascii_sfi_xchar(nhfp, d_xchar, myparent, myname, cnt)
NHFILE *nhfp;
xchar *d_xchar;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    xchar tmp;
    int i, itmp;
    char *rstr;
    const char *parent = "xchar";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        rstr = get_savefield(nhfp, linebuf, BUFSZ);
        itmp = atoi(rstr);
        tmp = (xchar) itmp;
#ifdef SAVEFILE_DEBUGGING
        if (nhfp->structlevel && tmp != *d_xchar)
            report_problem_ascii(nhfp, myparent, myname, parent);
        else
#endif
        *d_xchar = tmp;
        d_xchar++;
    }
}

static char strbuf[BUFSZ * 4];

void
ascii_sfi_str(nhfp, d_str, myparent, myname, cnt)
NHFILE *nhfp;
char *d_str;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i, j, sval;
    char n[4], *rstr;
    const char *parent = "str";
    char *src, *dest;
#ifdef SAVEFILE_DEBUGGING
    boolean match;
    char testbuf[BUFSZ];
#endif

    nhUse(parent);
    /* cnt is the length of the string */
    rstr = get_savefield(nhfp, strbuf, BUFSZ * 4);
    src = rstr;
    dest =
#ifdef SAVEFILE_DEBUGGING
            testbuf;
#else
            d_str;
#endif

    for (i = 0; i < cnt; ++i) {
        if (*src == '\\') {
            src++;
            for (j = 0; j < 4; ++j) {
                if (j < 3)
                    n[j] = *src++;
                else
                    n[j] = '\0';
	    }
            sval = atoi(n);
            *dest++ = (char) sval;
        } else
            *dest++ = *src++;
    }
#ifdef SAVEFILE_DEBUGGING
    if (nhfp->structlevel) {
        src = testbuf;
        dest = d_str;
        match = TRUE;
        for (i = 0; i < cnt; ++i) {
            if (*src++ != *dest++)
                match = FALSE;
        }
        if (!match)
            report_problem_ascii(nhfp, myparent, myname, parent);
        else {
            src = testbuf;
            dest = d_str;
            for (i = 0; i < cnt; ++i)
                *dest++ = *src++;
        }
    }
#endif
}
 
void
ascii_sfi_addinfo(nhfp, myparent, action, myname, indx)
NHFILE *nhfp UNUSED;
const char *myparent UNUSED, *action UNUSED, *myname UNUSED;
int indx UNUSED;
{
    /* not doing anything here */
}

char *
get_savefield(nhfp, inbuf, inbufsz)
NHFILE *nhfp;
char *inbuf;
size_t inbufsz;
{
    char *ep, *sep;

    if (fgets(inbuf, (int) inbufsz, nhfp->fpdef)) {
        nhfp->count++;
        ep = index(inbuf, '\n');
        if (!ep) {  /* newline missing */
            if (strlen(inbuf) < (inbufsz - 2)) {
                /* likely the last line of file is just
                   missing a newline; process it anyway  */
                ep = eos(inbuf);
            }
        }
        if (ep)
            *ep = '\0'; /* remove newline */
        sep = index(inbuf, '|');
        if (sep)
            sep++;
        
        return sep;
    }
    inbuf[0] = '\0';
    nhfp->eof = TRUE;
    return inbuf;
}

#ifdef SAVEFILE_DEBUGGING
void
report_problem_ascii(nhfp, s1, s2, s3)
NHFILE *nhfp;
const char *s1, *s2, *s3;
{
    fprintf(nhfp->fpdebug, "faulty value preservation "
            "(%ld, %s, %s, %s)\n", nhfp->count, s1, s2, s3);
}
#endif


