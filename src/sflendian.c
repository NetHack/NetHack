/* NetHack 3.6	sflendian.c $NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* Copyright (c) M. Allison, 2019. */
/* NetHack may be freely redistributed.  See license for details. */

/* fieldlevel little-endian binary file */
 
#include "hack.h"
#include "integer.h"
#include "sfprocs.h"

/*
+------------+--------+------+-------+-----------+--------+-----------+
| Data model | short  | int  | long  | long long | pointer|     OS    |
+------------+--------+------+-------+-----------+--------+-----------+
|   LLP64    |   16   |  32  |   32  |     64    |   64   | Windows   |
+------------+--------+------+-------+-----------+--------+-----------+
|    LP64    |   16   |  32  |   64  |     64    |   64   | Most Unix |
+------------+--------+------+-------+-----------+--------+-----------+
|    ILP64   |   16   |  64  |   64  |     64    |   64   |HAL,SPARC64|
+------------+--------+------+-------+-----------+--------+-----------+

  We're using this for NetHack's little-endian binary fieldlevel format
  because it involves the fewest compromises (sorry SPARC64,
  you'll have to use one of the text file formats instead):
  
  Data model | short  | int  | long  | long long | pointer|     OS    |
     LP64    |   16   |  32  |   64  |     64    |   64   | Most Unix |
*/

#if defined(_MSC_VER)
#include <intrin.h>
#define bswap16(x) _byteswap_ushort(x)
#define bswap32(x) _byteswap_ulong(x)
#define bswap64(x) _byteswap_uint64(x)
#elif defined(__GNUC__)
#define bswap16(x) __builtin_bswap16(x)
#define bswap32(x) __builtin_bswap32(x)
#define bswap64(x) __builtin_bswap64(x)
#else
/* else use ais523 approach */
# define bswap16(x)   ((((x) & 0x00ffU) << 8) | \
                           (((x) & 0xff00U) >> 8))

# define bswap32(x)   ((((x) & 0x000000ffLU) << 24) | \
                           (((x) & 0x0000ff00LU) <<  8) | \
                           (((x) & 0x00ff0000LU) >>  8) | \
                           (((x) & 0xff000000LU) >> 24))

# define bswap64(x)   ((((x) & 0x00000000000000ffLLU) << 56) | \
                           (((x) & 0x000000000000ff00LLU) << 40) | \
                           (((x) & 0x0000000000ff0000LLU) << 24) | \
                           (((x) & 0x00000000ff000000LLU) <<  8) | \
                           (((x) & 0x000000ff00000000LLU) <<  8) | \
                           (((x) & 0x0000ff0000000000LLU) << 24) | \
                           (((x) & 0x00ff000000000000LLU) << 40) | \
                           (((x) & 0xff00000000000000LLU) << 56))
#endif

#ifdef SAVEFILE_DEBUGGING
#if defined(__GNUC__)
#define DEBUGFORMATSTR64 "%s %s %ld %ld %d\n"
#elif defined(_MSC_VER)
#define DEBUGFORMATSTR64 "%s %s %lld %ld %d\n"
#endif
#endif

struct sf_procs lendian_sfo_procs = {
    ".le",
    {
        lendian_sfo_aligntyp,
        lendian_sfo_any,
        lendian_sfo_bitfield,
        lendian_sfo_boolean,
        lendian_sfo_char,
        lendian_sfo_genericptr,
        lendian_sfo_int,
        lendian_sfo_long,
        lendian_sfo_schar,
        lendian_sfo_short,
        lendian_sfo_size_t,
        lendian_sfo_time_t,
        lendian_sfo_unsigned,
        lendian_sfo_uchar,
        lendian_sfo_uint,
        lendian_sfo_ulong,
        lendian_sfo_ushort,
        lendian_sfo_xchar,
        lendian_sfo_str,
        lendian_sfo_addinfo,
    }
};

struct sf_procs lendian_sfi_procs =
{
    ".le",
    {
        lendian_sfi_aligntyp,
        lendian_sfi_any,
        lendian_sfi_bitfield,
        lendian_sfi_boolean,
        lendian_sfi_char,
        lendian_sfi_genericptr,
        lendian_sfi_int,
        lendian_sfi_long,
        lendian_sfi_schar,
        lendian_sfi_short,
        lendian_sfi_size_t,
        lendian_sfi_time_t,
        lendian_sfi_unsigned,
        lendian_sfi_uchar,
        lendian_sfi_uint,
        lendian_sfi_ulong,
        lendian_sfi_ushort,
        lendian_sfi_xchar,
        lendian_sfi_str,
        lendian_sfi_addinfo,
    }
};

#ifdef SAVEFILE_DEBUGGING
static long floc = 0L;
#endif

/*
 *----------------------------------------------------------------------------
 * sfo_lendian_ routines
 *
 * Default output routines.
 *
 *----------------------------------------------------------------------------
 */
 
void
lendian_sfo_any(nhfp, d_any, myparent, myname, cnt)
NHFILE *nhfp;
union any *d_any;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    const char *parent = "any";
    int i;
    uint64_t ui64;
    int64_t i64;
    uint32_t ui32;
    int32_t i32;
    int8_t i8;

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        ui64 = (uint64_t) d_any->a_void;
        fwrite(&ui64, sizeof ui64, 1, nhfp->fpdef);
        i64 = (int64_t) d_any->a_ulong;
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug,"%s %s %ld %ld %d\n", myname,
                "any",
                d_any->a_ulong, ftell(nhfp->fpdef), cnt);
#endif
        fwrite(&i64, sizeof i64, 1, nhfp->fpdef);
        ui32 = (uint32_t) d_any->a_uint;
        fwrite(&ui32, sizeof ui32, 1, nhfp->fpdef);
        i32 = (int32_t) d_any->a_int;
        fwrite(&i32, sizeof i32, 1, nhfp->fpdef);
        i8 = (int8_t) d_any->a_char;
        fwrite(&i8, sizeof i8, 1, nhfp->fpdef);
        d_any++;
    }
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
lendian_sfo_aligntyp(nhfp, d_aligntyp, myparent, myname, cnt)
NHFILE *nhfp;
aligntyp *d_aligntyp;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    const char *parent = "aligntyp";
    int i;
    int16_t val;

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        val = nhfp->bendian ? bswap16(*d_aligntyp) : *d_aligntyp;
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug,"%s %s %hd %ld %d\n", myname,
                "aligntyp",
                val, ftell(nhfp->fpdef), cnt);
#endif
        fwrite(&val, sizeof val, 1, nhfp->fpdef);
        d_aligntyp++;
    }
}

void
lendian_sfo_bitfield(nhfp, d_bitfield, myparent, myname, cnt)
NHFILE *nhfp;
uint8_t *d_bitfield;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt UNUSED;
{
    const char *parent = "bitfield";

    nhUse(parent);
    /* for bitfields, cnt is the number of bits, not an array */
#ifdef SAVEFILE_DEBUGGING
    fprintf(nhfp->fpdebug,"%s %s %d %ld %d\n", myname,
            "bitfield",
            (int) *d_bitfield, ftell(nhfp->fpdef), cnt);
#endif
    fwrite(d_bitfield, sizeof *d_bitfield, 1, nhfp->fpdef);
}

void
lendian_sfo_boolean(nhfp, d_boolean, myparent, myname, cnt)
NHFILE *nhfp;
boolean *d_boolean;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    const char *parent = "boolean";
    int i;
    int8_t val;

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        val = *d_boolean;
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug,"%s %s %d %ld %d\n", myname,
                "boolean",
                (int) val, ftell(nhfp->fpdef), cnt);
#endif
        fwrite(&val, sizeof val, 1, nhfp->fpdef);
        d_boolean++;
    }
}

void
lendian_sfo_char(nhfp, d_char, myparent, myname, cnt)
NHFILE *nhfp;
char *d_char;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    const char *parent = "char";
    int8_t val;

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        val = *d_char;
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug,"%s %s %d %ld %d\n", myname,
                "char",
                (int) val, ftell(nhfp->fpdef), cnt);
#endif
        fwrite(&val, sizeof val, 1, nhfp->fpdef);
        d_char++;
    }
}

void
lendian_sfo_genericptr(nhfp, d_genericptr, myparent, myname, cnt)
NHFILE *nhfp;
genericptr_t *d_genericptr;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    int8_t p;

    /*
     * sbrooms is an array of pointers to mkroom.
     * That array dimension is MAX_SUBROOMS.
     * Even though the pointers themselves won't
     * be valid, we need to account for the existence
     * of the elements of that array, and whether each
     * is zero or non-zero.
     *
     * We only consume a single byte in the file for that
     * and we expand it back to pointer size later when
     * we read it back in, again preserving zero or non-zero.
     */

    for (i = 0; i < cnt; ++i) {
        p = (*d_genericptr) ? 1 : 0;
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug,"%s %s %d %ld %d\n", myname,
                "genericptr",
                (int) p, ftell(nhfp->fpdef), cnt);
#endif
        fwrite(&p, sizeof p, 1, nhfp->fpdef);
        d_genericptr++;
    }
}

void
lendian_sfo_int(nhfp, d_int, myparent, myname, cnt)
NHFILE *nhfp;
int *d_int;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    const char *parent = "int";
    int32_t i32, val;

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        i32 = (int32_t) *d_int;
        val = nhfp->bendian ? bswap32(i32) : i32;
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug,"%s %s %d %ld %d\n", myname,
                "int",
                val, ftell(nhfp->fpdef), cnt);
#endif
        fwrite(&val, sizeof val, 1, nhfp->fpdef);
        d_int++;
    }
}

void
lendian_sfo_long(nhfp, d_long, myparent, myname, cnt)
NHFILE *nhfp;
long *d_long;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    const char *parent = "long";
    int64_t i64, val64;

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        i64 = (int64_t) *d_long;
        val64 = nhfp->bendian ? bswap64(i64) : i64;
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug, DEBUGFORMATSTR64, myname,
                "long",
                val64, ftell(nhfp->fpdef), cnt);
#endif
        fwrite(&val64, sizeof val64, 1, nhfp->fpdef);
        d_long++;
    }
}

void
lendian_sfo_schar(nhfp, d_schar, myparent, myname, cnt)
NHFILE *nhfp;
schar *d_schar;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    int8_t itmp;
    const char *parent = "schar";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        itmp = (int8_t) *d_schar;
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug,"%s %s %d %ld %d\n", myname,
                "schar",
                (int) itmp, ftell(nhfp->fpdef), cnt);
#endif
        fwrite(&itmp, sizeof itmp, 1, nhfp->fpdef);
        d_schar++;
    }
}

void
lendian_sfo_short(nhfp, d_short, myparent, myname, cnt)
NHFILE *nhfp;
short *d_short;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    int16_t itmp;
    const char *parent = "short";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        itmp = (int16_t) *d_short;
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug,"%s %s %hd %ld %d\n", myname,
                "short",
                itmp, ftell(nhfp->fpdef), cnt);
#endif
        fwrite(&itmp, sizeof itmp, 1, nhfp->fpdef);
        d_short++;
    }
}

void
lendian_sfo_size_t(nhfp, d_size_t, myparent, myname, cnt)
NHFILE *nhfp;
size_t *d_size_t;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    uint64_t ui64, val;
    const char *parent = "size_t";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        ui64 = (uint64_t) *d_size_t;
        val = nhfp->bendian ? bswap64(ui64) : ui64;
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug, DEBUGFORMATSTR64, myname,
                "size_t",
                val, ftell(nhfp->fpdef), cnt);
#endif
        fwrite(&val, sizeof val, 1, nhfp->fpdef);
        d_size_t++;
    }
}

void
lendian_sfo_time_t(nhfp, d_time_t, myparent, myname, cnt)
NHFILE *nhfp;
time_t *d_time_t;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt UNUSED;
{
    char buf[BUFSZ];
    const char *parent = "time_t";

    nhUse(parent);
    Sprintf(buf, "%s", yyyymmddhhmmss(*d_time_t));
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug,"%s %s %s %ld %d\n", myname,
                "time",
                buf, ftell(nhfp->fpdef), cnt);
#endif
    fwrite(buf, sizeof (char), 15, nhfp->fpdef);
}

void
lendian_sfo_unsigned(nhfp, d_unsigned, myparent, myname, cnt)
NHFILE *nhfp;
unsigned *d_unsigned;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    lendian_sfo_uint(nhfp, d_unsigned, myparent, myname, cnt);
}

void
lendian_sfo_uchar(nhfp, d_uchar, myparent, myname, cnt)
NHFILE *nhfp;
unsigned char *d_uchar;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    const char *parent = "uchar";
    uint8_t ui8;

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        ui8 = (uint8_t) *d_uchar;
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug,"%s %s %u %ld %d\n", myname,
                "uchar",
                (unsigned int) ui8, ftell(nhfp->fpdef), cnt);
#endif
        fwrite(&ui8, sizeof ui8, 1, nhfp->fpdef);
        d_uchar++;
    }
}

void
lendian_sfo_uint(nhfp, d_uint, myparent, myname, cnt)
NHFILE *nhfp;
unsigned int *d_uint;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    const char *parent = "uint";
    uint32_t ui32, val;
    
    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        ui32 = (uint32_t) *d_uint;
        val = nhfp->bendian ? bswap32(ui32) : ui32;
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug,"%s %s %u %ld %d\n", myname,
                "uint",
                val, ftell(nhfp->fpdef), cnt);
#endif
        fwrite(&val, sizeof val, 1, nhfp->fpdef);
        d_uint++;
    }
}

void
lendian_sfo_ulong(nhfp, d_ulong, myparent, myname, cnt)
NHFILE *nhfp;
unsigned long *d_ulong;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    const char *parent = "ulong";
    uint64_t ul64, val64;

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        ul64 = (uint64_t) *d_ulong;
        val64 = nhfp->bendian ? bswap64(ul64) : ul64;
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug, DEBUGFORMATSTR64, myname,
                "ulong",
                val64, ftell(nhfp->fpdef), cnt);
#endif
        fwrite(&val64, sizeof val64, 1, nhfp->fpdef);
        d_ulong++;
    }
}

void
lendian_sfo_ushort(nhfp, d_ushort, myparent, myname, cnt)
NHFILE *nhfp;
unsigned short *d_ushort;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    const char *parent = "ushort";
    uint16_t ui16, val16;

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        ui16 = (uint16_t) *d_ushort;
        val16 = nhfp->bendian ? bswap16(ui16) : ui16;
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug,"%s %s %hd %ld %d\n", myname,
                "ushort",
                val16, ftell(nhfp->fpdef), cnt);
#endif
        fwrite(&val16, sizeof val16, 1, nhfp->fpdef);
        d_ushort++;
    }
}

void
lendian_sfo_xchar(nhfp, d_xchar, myparent, myname, cnt)
NHFILE *nhfp;
xchar *d_xchar;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    const char *parent = "xchar";
    int16_t i16, val16;

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        i16 = (int16_t) *d_xchar;
        val16 = nhfp->bendian ? bswap16(i16) : i16;
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug,"%s %s %hd %ld %d\n", myname,
                "xchar",
                val16, ftell(nhfp->fpdef), cnt);
#endif
        fwrite(&val16, sizeof val16, 1, nhfp->fpdef);
        d_xchar++;
    }
}

static char strbuf[BUFSZ * 4];

void
lendian_sfo_str(nhfp, d_str, myparent, myname, cnt)
NHFILE *nhfp;
char *d_str;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i, j, intval;
    int16_t i16, outcount = 0;
    const char *parent = "str";
    char sval[QBUFSZ], *src = d_str, *dest = strbuf;

    nhUse(parent);
    /* cnt is the number of characters */
    for (i = 0; i < cnt; ++i) {
        if ((*src < 32) || (*src == '\\') || (*src > 127)) {
            *dest++ = '\\';
            outcount++;
            intval = (int) *src++;
            Sprintf(sval, "%03d", intval);
            for (j = 0; j < 3; ++j) {
                *dest++ = sval[j];
                outcount++;
            }
        } else {
            *dest++ = *src++;
            outcount++;
        }
    }
    *dest = '\0';
    i16 = nhfp->bendian ? bswap16(outcount) : outcount;
#ifdef SAVEFILE_DEBUGGING
    fprintf(nhfp->fpdebug,"%s %s %hd %ld %d\n", myname,
            "str-count",
            i16, ftell(nhfp->fpdef), cnt);
#endif
    fwrite(&i16, sizeof i16, 1, nhfp->fpdef);
#ifdef SAVEFILE_DEBUGGING
    fprintf(nhfp->fpdebug,"%s %s %s %ld %d\n", myname,
            "str",
            strbuf, ftell(nhfp->fpdef), cnt);
#endif
    fwrite(strbuf, sizeof (char), outcount, nhfp->fpdef);
}

void
lendian_sfo_addinfo(nhfp, parent, action, myname, indx)
NHFILE *nhfp UNUSED;
const char *parent UNUSED, *action UNUSED, *myname UNUSED;
int indx UNUSED;
{
    /* ignored */
}


/*
 *----------------------------------------------------------------------------
 * lendian_sfi_ routines called from functions in sfi_base.c
 *----------------------------------------------------------------------------
 */
 
void
lendian_sfi_any(nhfp, d_any, myparent, myname, cnt)
NHFILE *nhfp;
union any *d_any;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    const char *parent = "any";
    int i;
    uint64_t ui64;
    int64_t i64;
    uint32_t ui32;
    int32_t i32;
    int8_t i8;

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
        fread(&ui64, sizeof ui64, 1, nhfp->fpdef);
        if (feof(nhfp->fpdef)) {
            nhfp->eof = TRUE;
            return;
	}
#ifdef SAVEFILE_DEBUGGING
        floc = ftell(nhfp->fpdef);
#endif
        fread(&i64, sizeof i64, 1, nhfp->fpdef);
        if (feof(nhfp->fpdef)) {
            nhfp->eof = TRUE;
            return;
	}
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug, DEBUGFORMATSTR64, myname,
                "any",
                ui64, floc, cnt);
#endif
        fread(&ui32, sizeof ui32, 1, nhfp->fpdef);
        if (feof(nhfp->fpdef)) {
            nhfp->eof = TRUE;
            return;
	}
        fread(&i32, sizeof i32, 1, nhfp->fpdef);
        if (feof(nhfp->fpdef)) {
            nhfp->eof = TRUE;
            return;
	}
        fread(&i8, sizeof i8, 1, nhfp->fpdef);
        if (feof(nhfp->fpdef)) {
            nhfp->eof = TRUE;
            return;
	}
        d_any->a_void = (genericptr_t) ui64;
        d_any->a_ulong = (unsigned long) i64;
        d_any->a_uint = (unsigned int) ui32;
        d_any->a_int = (int) i32;
        d_any->a_char = (char) i8;
        d_any++;
    }
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
lendian_sfi_aligntyp(nhfp, d_aligntyp, myparent, myname, cnt)
NHFILE *nhfp;
aligntyp *d_aligntyp;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    const char *parent = "aligntyp";
    int i;
    int16_t val, i16;

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
#ifdef SAVEFILE_DEBUGGING
        floc = ftell(nhfp->fpdef);
#endif
        fread(&val, sizeof val, 1, nhfp->fpdef);
        if (feof(nhfp->fpdef)) {
            nhfp->eof = TRUE;
            return;
	}
        i16 = nhfp->bendian ? bswap16(val) : val;
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug,"%s %s %hd %ld %d\n", myname,
                "aligntyp",
                i16, floc, cnt);
#endif
        *d_aligntyp = (aligntyp) i16;
        d_aligntyp++;
    }
}

void
lendian_sfi_bitfield(nhfp, d_bitfield, myparent, myname, cnt)
NHFILE *nhfp;
uint8_t *d_bitfield;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt UNUSED;
{
    const char *parent = "bitfield";

    nhUse(parent);
#ifdef SAVEFILE_DEBUGGING
    floc = ftell(nhfp->fpdef);
#endif
    fread(d_bitfield, sizeof *d_bitfield, 1, nhfp->fpdef);
    if (feof(nhfp->fpdef)) {
        nhfp->eof = TRUE;
        return;
    }
#ifdef SAVEFILE_DEBUGGING
    fprintf(nhfp->fpdebug,"%s %s %d %ld %d\n", myname,
            "bitfield",
            (int) *d_bitfield, floc, cnt);
#endif
}

void
lendian_sfi_boolean(nhfp, d_boolean, myparent, myname, cnt)
NHFILE *nhfp;
boolean *d_boolean;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    const char *parent = "boolean";
    int8_t i8;

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
#ifdef SAVEFILE_DEBUGGING
        floc = ftell(nhfp->fpdef);
#endif
        fread(&i8, sizeof i8, 1, nhfp->fpdef);
        if (feof(nhfp->fpdef)) {
            nhfp->eof = TRUE;
            return;
        }
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug,"%s %s %d %ld %d\n", myname,
                "boolean",
                (int) i8, floc, cnt);
#endif
        *d_boolean = (boolean) i8;
        d_boolean++;
    }
}

void
lendian_sfi_char(nhfp, d_char, myparent, myname, cnt)
NHFILE *nhfp;
char *d_char;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    int8_t i8;

    for (i = 0; i < cnt; ++i) {
#ifdef SAVEFILE_DEBUGGING
        floc = ftell(nhfp->fpdef);
#endif
        fread(&i8, sizeof i8, 1, nhfp->fpdef);
        if (feof(nhfp->fpdef)) {
            nhfp->eof = TRUE;
            return;
        }
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug,"%s %s %d %ld %d\n", myname,
                "char",
                (int) i8, floc, cnt);
#endif
        *d_char = (char) i8;
        d_char++;
    }
}

void
lendian_sfi_genericptr(nhfp, d_genericptr, myparent, myname, cnt)
NHFILE *nhfp;
genericptr_t *d_genericptr;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    static const char *glorkum = "glorkum";
    int8_t p;

    for (i = 0; i < cnt; ++i) {
#ifdef SAVEFILE_DEBUGGING
        floc = ftell(nhfp->fpdef);
#endif
        fread(&p, sizeof p, 1, nhfp->fpdef);
        if (feof(nhfp->fpdef)) {
            nhfp->eof = TRUE;
            return;
        }
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug,"%s %s %d %ld %d\n", myname,
                "genericptr",
                (int) p, floc, cnt);
#endif
        *d_genericptr = p ? (genericptr_t) glorkum  : (genericptr_t) 0;
        d_genericptr++;
    }
}

void
lendian_sfi_int(nhfp, d_int, myparent, myname, cnt)
NHFILE *nhfp;
int *d_int;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    int32_t val, i32;

    for (i = 0; i < cnt; ++i) {
#ifdef SAVEFILE_DEBUGGING
        floc = ftell(nhfp->fpdef);
#endif
        fread(&val, sizeof val, 1, nhfp->fpdef);
        if (feof(nhfp->fpdef)) {
            nhfp->eof = TRUE;
            return;
        }
        i32 = nhfp->bendian ? bswap32(val) : val;
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug,"%s %s %d %ld %d\n", myname,
                "int",
                i32, floc, cnt);
#endif
        *d_int = (int) i32;
        d_int++;
    }
}

void
lendian_sfi_long(nhfp, d_long, myparent, myname, cnt)
NHFILE *nhfp;
long *d_long;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    int64_t val64, i64;

    for (i = 0; i < cnt; ++i) {
#ifdef SAVEFILE_DEBUGGING
        floc = ftell(nhfp->fpdef);
#endif
        fread(&val64, sizeof val64, 1, nhfp->fpdef);
        if (feof(nhfp->fpdef)) {
            nhfp->eof = TRUE;
            return;
        }
        i64 = nhfp->bendian ? bswap64(val64) : val64;
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug, DEBUGFORMATSTR64, myname,
                "long",
                i64, floc, cnt);
#endif
        *d_long = (long) i64;
        d_long++;
    }
}

void
lendian_sfi_schar(nhfp, d_schar, myparent, myname, cnt)
NHFILE *nhfp;
schar *d_schar;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    int8_t i8;

    for (i = 0; i < cnt; ++i) {
#ifdef SAVEFILE_DEBUGGING
        floc = ftell(nhfp->fpdef);
#endif
        fread(&i8, sizeof i8, 1, nhfp->fpdef);
        if (feof(nhfp->fpdef)) {
            nhfp->eof = TRUE;
            return;
        }
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug,"%s %s %d %ld %d\n", myname, 
                "schar",
                (int) i8, floc, cnt);
#endif
        *d_schar = (schar) i8;
        d_schar++;
    }
}

void
lendian_sfi_short(nhfp, d_short, myparent, myname, cnt)
NHFILE *nhfp;
short *d_short;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    int16_t val16, i16;

    for (i = 0; i < cnt; ++i) {
#ifdef SAVEFILE_DEBUGGING
        floc = ftell(nhfp->fpdef);
#endif
        fread(&val16, sizeof val16, 1, nhfp->fpdef);
        if (feof(nhfp->fpdef)) {
            nhfp->eof = TRUE;
            return;
        }
        i16 = nhfp->bendian ? bswap16(val16) : val16;
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug,"%s %s %hd %ld %d\n", myname,
                "short",
                i16, floc, cnt);
#endif
        *d_short = (short) i16;
        d_short++;
    }
}

void
lendian_sfi_size_t(nhfp, d_size_t, myparent, myname, cnt)
NHFILE *nhfp;
size_t *d_size_t;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    uint64_t ui64, val;
    const char *parent = "size_t";

    nhUse(parent);
    for (i = 0; i < cnt; ++i) {
#ifdef SAVEFILE_DEBUGGING
        floc = ftell(nhfp->fpdef);
#endif
        fread(&val, sizeof val, 1, nhfp->fpdef);
        if (feof(nhfp->fpdef)) {
            nhfp->eof = TRUE;
            return;
        }
        ui64 = nhfp->bendian ? bswap64(val) : val;
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug, DEBUGFORMATSTR64, myname,
                "size_t",
                ui64, floc, cnt);
#endif
        *d_size_t = (size_t) ui64;
        d_size_t++;
    }
}

void
lendian_sfi_time_t(nhfp, d_time_t, myparent, myname, cnt)
NHFILE *nhfp;
time_t *d_time_t;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt UNUSED;
{
    time_t tmp;
    char buf[BUFSZ];
    const char *parent = "time_t";

    nhUse(parent);
#ifdef SAVEFILE_DEBUGGING
    floc = ftell(nhfp->fpdef);
#endif
    fread(buf, 1, 15, nhfp->fpdef);
        if (feof(nhfp->fpdef)) {
            nhfp->eof = TRUE;
            return;
        }
#ifdef SAVEFILE_DEBUGGING
    fprintf(nhfp->fpdebug,"%s %s %s %ld %d\n", myname, 
            "time",
            buf, floc, cnt);
#endif
    tmp = time_from_yyyymmddhhmmss(buf);
    *d_time_t = tmp;
}

void
lendian_sfi_unsigned(nhfp, d_unsigned, myparent, myname, cnt)
NHFILE *nhfp;
unsigned *d_unsigned;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    /* deferal */
    lendian_sfi_uint(nhfp, d_unsigned, myparent, myname, cnt);
}

void
lendian_sfi_uchar(nhfp, d_uchar, myparent, myname, cnt)
NHFILE *nhfp;
unsigned char *d_uchar;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    uint8_t ui8;

    for (i = 0; i < cnt; ++i) {
#ifdef SAVEFILE_DEBUGGING
        floc = ftell(nhfp->fpdef);
#endif
        fread(&ui8, sizeof ui8, 1, nhfp->fpdef);
        if (feof(nhfp->fpdef)) {
            nhfp->eof = TRUE;
            return;
        }
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug,"%s %s %hu %ld %d\n", myname,
                "uchar",
                (unsigned short) ui8, floc, cnt);
#endif
        *d_uchar = (uchar) ui8;
        d_uchar++;
    }
}

void
lendian_sfi_uint(nhfp, d_uint, myparent, myname, cnt)
NHFILE *nhfp;
unsigned int *d_uint;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    uint32_t val, ui32;

    for (i = 0; i < cnt; ++i) {
#ifdef SAVEFILE_DEBUGGING
        floc = ftell(nhfp->fpdef);
#endif
        fread(&val, sizeof val, 1, nhfp->fpdef);
        if (feof(nhfp->fpdef)) {
            nhfp->eof = TRUE;
            return;
        }
        ui32 = nhfp->bendian ? bswap32(val) : val;
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug,"%s %s %u %ld %d\n", myname, 
                "uint",
                ui32, floc, cnt);
#endif
        *d_uint = (unsigned int) ui32;
        d_uint++;
    }
}

void
lendian_sfi_ulong(nhfp, d_ulong, myparent, myname, cnt)
NHFILE *nhfp;
unsigned long *d_ulong;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    uint64_t val64, ui64;

    for (i = 0; i < cnt; ++i) {
#ifdef SAVEFILE_DEBUGGING
        floc = ftell(nhfp->fpdef);
#endif
        fread(&val64, sizeof val64, 1, nhfp->fpdef);
        if (feof(nhfp->fpdef)) {
            nhfp->eof = TRUE;
            return;
        }
        ui64 = nhfp->bendian ? bswap64(val64) : val64;
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug, DEBUGFORMATSTR64, myname,
                "ulong",
                ui64, floc, cnt);
#endif
        *d_ulong = (unsigned long) ui64;
        d_ulong++;
    }
}

void
lendian_sfi_ushort(nhfp, d_ushort, myparent, myname, cnt)
NHFILE *nhfp;
unsigned short *d_ushort;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    uint16_t val16, ui16;

    for (i = 0; i < cnt; ++i) {
#ifdef SAVEFILE_DEBUGGING
        floc = ftell(nhfp->fpdef);
#endif
        fread(&val16, sizeof val16, 1, nhfp->fpdef);
        if (feof(nhfp->fpdef)) {
            nhfp->eof = TRUE;
            return;
        }
        ui16 = nhfp->bendian ? bswap16(val16) : val16;
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug,"%s %s %hu %ld %d\n", myname,
                "ushort",
                ui16, floc, cnt);
#endif
        *d_ushort = (unsigned short) ui16;
        d_ushort++;
    }
}

void
lendian_sfi_xchar(nhfp, d_xchar, myparent, myname, cnt)
NHFILE *nhfp;
xchar *d_xchar;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
    int i;
    int16_t val16, i16;

    for (i = 0; i < cnt; ++i) {
#ifdef SAVEFILE_DEBUGGING
        floc = ftell(nhfp->fpdef);
#endif
        fread(&val16, sizeof val16, 1, nhfp->fpdef);
        if (feof(nhfp->fpdef)) {
            nhfp->eof = TRUE;
            return;
        }
        i16 = nhfp->bendian ? bswap16(val16) : val16;
#ifdef SAVEFILE_DEBUGGING
        fprintf(nhfp->fpdebug,"%s %s %hd %ld %d\n", myname,
                "xchar",
                i16, floc, cnt);
#endif
        *d_xchar = (xchar) i16;
        d_xchar++;
    }
}

static char strbuf[BUFSZ * 4];

void
lendian_sfi_str(nhfp, d_str, myparent, myname, cnt)
NHFILE *nhfp;
char *d_str;
const char *myparent UNUSED;
const char *myname UNUSED;
int cnt;
{
#ifdef SAVEFILE_DEBUGGING
    char testbuf[BUFSZ];
#endif
    int i, j, sval;
    const char *parent = "str";
    char n[4];
    char *src, *dest;
    int16_t i16, incount = 0;

    nhUse(parent);
#ifdef SAVEFILE_DEBUGGING
    floc = ftell(nhfp->fpdef);
#endif
    fread(&i16, sizeof i16, 1, nhfp->fpdef);
    if (feof(nhfp->fpdef)) {
        nhfp->eof = TRUE;
        return;
    }
    incount = nhfp->bendian ? bswap16(i16) : i16;
#ifdef SAVEFILE_DEBUGGING
    fprintf(nhfp->fpdebug,"%s %s %hd %ld %d\n", myname,
            "str-count",
            incount, floc, cnt);
#endif
    if (incount >= (BUFSZ * 4) - 1)
        panic("overflow on sflendian string read %d %d",
                    incount, cnt);
#ifdef SAVEFILE_DEBUGGING
    floc = ftell(nhfp->fpdef);
#endif
    fread(strbuf, sizeof (char), incount, nhfp->fpdef);
    if (feof(nhfp->fpdef)) {
        nhfp->eof = TRUE;
        return;
    }
    strbuf[incount] = '\0';
#ifdef SAVEFILE_DEBUGGING
    fprintf(nhfp->fpdebug,"%s %s %s %ld %d\n", myname,
                "str",
                strbuf, floc, cnt);
#endif
    src = strbuf;
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
}
 
void
lendian_sfi_addinfo(nhfp, myparent, action, myname, indx)
NHFILE *nhfp UNUSED;
const char *myparent UNUSED, *action UNUSED, *myname UNUSED;
int indx UNUSED;
{
    /* not doing anything here */
}



