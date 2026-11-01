/* NetHack 5.0	sfexpasc.c.c $NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* Copyright (c) Michael Allison, 2025. */
/* NetHack may be freely redistributed.  See license for details. */

/* avoid the global.h define */
#define STRNCMPI

#include "hack.h"
#include "integer.h"
#include "sfprocs.h"
#include "sfproto.h"

#if defined(MACOSX) || defined(VMS)
extern long long atoll(const char *);
#endif

static void put_savefield(NHFILE *, char *, size_t);
char *get_savefield(NHFILE *, char *, size_t);
#ifdef SAVEFILE_DEBUGGING
void report_problem_exportascii(NHFILE *, const char *, const char *,
                          const char *);
#endif

#ifdef _MSC_VER
#define strcmpi _stricmp
#else
#define strcmpi strcasecmp
#endif

static char linebuf[BUFSZ * 10];
static char outbuf[BUFSZ * 10];

#if 0
void exportascii_sfo_any(NHFILE *nhfp, union any *d_any, const char *myname UNUSED);
void exportascii_sfi_any(NHFILE *nhfp, union any *d_any, const char *myname UNUSED);
void exportascii_sfo_aligntyp(NHFILE *nhfp, aligntyp *d_aligntyp, const char *myname UNUSED);
void exportascii_sfo_bitfield(NHFILE *nhfp, uint8_t *d_bitfield, const char *myname UNUSED, int);
void exportascii_sfo_boolean(NHFILE *nhfp, boolean *d_boolean, const char *myname UNUSED);
void exportascii_sfo_genericptr(NHFILE *nhfp, genericptr_t *d_genericptr, const char *myname UNUSED);
void exportascii_sfo_int32(NHFILE *nhfp, int *d_int, const char *myname UNUSED);
void exportascii_sfo_long(NHFILE *nhfp, long *d_long, const char *myname UNUSED);
void exportascii_sfo_schar(NHFILE *nhfp, schar *d_schar, const char *myname UNUSED);
void exportascii_sfo_int16(NHFILE *nhfp, short *d_short, const char *myname UNUSED);
void exportascii_sfo_size_t(NHFILE *nhfp, size_t *d_size_t, const char *myname UNUSED);
void exportascii_sfo_time_t(NHFILE *nhfp, time_t *d_time_t, const char *myname UNUSED);
void exportascii_sfo_uint32(NHFILE *nhfp, unsigned *d_unsigned, const char *myname);
void exportascii_sfo_uint(NHFILE *nhfp, unsigned *d_unsigned, const char *myname);
void exportascii_sfo_uchar(NHFILE *nhfp, unsigned char *d_uchar, const char *myname UNUSED);
void exportascii_sfo_uint(NHFILE *nhfp, unsigned int *d_uint, const char *myname UNUSED);
void exportascii_sfo_ulong(NHFILE *nhfp, unsigned long *d_ulong, const char *myname UNUSED);
void exportascii_sfo_ushort(NHFILE *nhfp, unsigned short *d_ushort, const char *myname UNUSED);
void exportascii_sfo_coordxy(NHFILE *nhfp, coordxy *d_coordxy, const char *myname UNUSED);
void exportascii_sfo_char(NHFILE *nhfp, xint8 *d_xint8, const char *myname UNUSED);
void exportascii_sfo_xint16(NHFILE *nhfp, xint16 *d_xint16, const char *myname UNUSED);
void exportascii_sfo_char(NHFILE *nhfp, char *d_str, const char *myname UNUSED, int cnt);
void exportascii_sfo_addinfo(NHFILE *nhfp UNUSED, const char *parent UNUSED, const char *action UNUSED, const char *myname UNUSED, int indx UNUSED);
void exportascii_sfi_aligntyp(NHFILE *nhfp, aligntyp *d_aligntyp, const char *myname UNUSED);
void exportascii_sfi_bitfield(NHFILE *nhfp, uint8_t *d_bitfield, const char *myname UNUSED, int);
void exportascii_sfi_boolean(NHFILE *nhfp, boolean *d_boolean, const char *myname UNUSED);
void exportascii_sfi_genericptr(NHFILE *nhfp, genericptr_t *d_genericptr, const char *myname UNUSED);
void exportascii_sfi_int32(NHFILE *nhfp, int *d_int, const char *myname UNUSED);
void exportascii_sfi_long(NHFILE *nhfp, long *d_long, const char *myname UNUSED);
void exportascii_sfi_schar(NHFILE *nhfp, schar *d_schar, const char *myname UNUSED);
void exportascii_sfi_int16(NHFILE *nhfp, short *d_short, const char *myname UNUSED);
void exportascii_sfi_size_t(NHFILE *nhfp, size_t *d_size_t, const char *myname UNUSED);
void exportascii_sfi_time_t(NHFILE *nhfp, time_t *d_time_t, const char *myname UNUSED);
void exportascii_sfi_uint32(NHFILE *nhfp, unsigned *d_unsigned, const char *myname);
void exportascii_sfi_uint(NHFILE *nhfp, unsigned *d_unsigned, const char *myname);
void exportascii_sfi_uchar(NHFILE *nhfp, unsigned char *d_uchar, const char *myname UNUSED);
void exportascii_sfi_uint(NHFILE *nhfp, unsigned int *d_uint, const char *myname UNUSED);
void exportascii_sfi_ulong(NHFILE *nhfp, unsigned long *d_ulong, const char *myname UNUSED);
void exportascii_sfi_ushort(NHFILE *nhfp, unsigned short *d_ushort, const char *myname UNUSED);
void exportascii_sfi_coordxy(NHFILE *nhfp, coordxy *d_coordxy, const char *myname UNUSED);
void exportascii_sfi_xint8(NHFILE *nhfp, xint8 *d_xint8, const char *myname UNUSED);
void exportascii_sfi_xint16(NHFILE *nhfp, xint16 *d_xint16, const char *myname UNUSED);
void exportascii_sfi_cnt(NHFILE *nhfp, char *d_str, const char *myname UNUSED, int cnt);
#endif

#define SFO_BODY(dt) \
{                    \
}

#define SFI_BODY(dt) \
{                    \
}

#define SF_A(dtyp) \
void exportascii_sfo_##dtyp(NHFILE *, dtyp *d_##dtyp,                          \
                           const char *);                                      \
void exportascii_sfi_##dtyp(NHFILE *, dtyp *d_##dtyp,                          \
                           const char *);
                                                                               
/*
void exportascii_sfo_##dtyp(NHFILE *nhfp, dtyp *d_##dtyp,                      \
                           const char *myname UNUSED)                          \
    SFO_BODY(dtyp)                                                             \
                                                                               \
void exportascii_sfi_##dtyp(NHFILE *nhfp, dtyp *d_##dtyp,                      \
                  const char *myname UNUSED)                                   \
    SFI_BODY(dtyp)
*/

#define SF_C(keyw, dtyp) \
void exportascii_sfo_##dtyp(NHFILE *, keyw dtyp *d_##dtyp UNUSED,              \
                           const char *);                                      \
void exportascii_sfi_##dtyp(NHFILE *, keyw dtyp *d_##dtyp,                     \
                           const char *);                                      \
                                                                               \
void exportascii_sfo_##dtyp(NHFILE *nhfp UNUSED, keyw dtyp *d_##dtyp UNUSED,   \
                           const char *myname UNUSED)                          \
    SFO_BODY(dtyp)                                                             \
                                                                               \
void exportascii_sfi_##dtyp(NHFILE *nhfp UNUSED, keyw dtyp *d_##dtyp UNUSED,   \
                           const char *myname UNUSED)                          \
    SFI_BODY(dtyp)


#define SF_X(xxx, dtyp) \
void exportascii_sfo_##dtyp(NHFILE *, xxx *d_##dtyp,                           \
                           const char *);                                      \
void exportascii_sfi_##dtyp(NHFILE *, xxx *d_##dtyp,                           \
                           const char *);                                      \
                                                                               \
void exportascii_sfo_##dtyp(NHFILE *nhfp UNUSED, xxx *d_##dtyp UNUSED,         \
                           const char *myname UNUSED)                          \
    SFO_BODY(dtyp)                                                             \
                                                                               \
void exportascii_sfi_##dtyp(NHFILE *nhfp, xxx *d_##dtyp,                       \
                           const char *myname UNUSED)                          \
    SFI_BODY(dtyp)


#define SF_BF(xxx, dtyp) \
void exportascii_sfo_##dtyp(NHFILE *, xxx *d_##dtyp,                           \
                           const char *, int);                                 \
void exportascii_sfi_##dtyp(NHFILE *, xxx *d_##dtyp,                           \
                           const char *, int);
                                                                          
SF_C(struct, arti_info)
SF_C(struct, nhrect)
SF_C(struct, branch)
SF_C(struct, bubble)
SF_C(struct, cemetery)
SF_C(struct, context_info)
SF_C(struct, nhcoord)
SF_C(struct, damage)
SF_C(struct, dest_area)
SF_C(struct, dgn_topology)
SF_C(struct, dungeon)
SF_C(struct, d_level)
SF_C(struct, ebones)
SF_C(struct, edog)
SF_C(struct, egd)
SF_C(struct, emin)
SF_C(struct, engr)
SF_C(struct, epri)
SF_C(struct, eshk)
SF_C(struct, fe)
SF_C(struct, flag)
SF_C(struct, fruit)
SF_C(struct, gamelog_line)
SF_C(struct, kinfo)
SF_C(struct, levelflags)
SF_C(struct, ls_t)
SF_C(struct, linfo)
SF_C(struct, mapseen_feat)
SF_C(struct, mapseen_flags)
SF_C(struct, mapseen_rooms)
SF_C(struct, mkroom)
SF_C(struct, monst)
SF_C(struct, mvitals)
SF_C(struct, obj)
SF_C(struct, objclass)
SF_C(struct, q_score)
SF_C(struct, rm)
SF_C(struct, spell)
SF_C(struct, stairway)
SF_C(struct, s_level)
SF_C(struct, trap)
SF_C(struct, version_info)
SF_C(struct, you)
/* SF_C(union, any) */
void exportascii_sfo_any(NHFILE *nhfp, union any *d_any, const char *myname UNUSED);
void exportascii_sfi_any(NHFILE *nhfp, union any *d_any, const char *myname UNUSED);

SF_A(aligntyp)
SF_A(boolean)
SF_A(coordxy)
SF_A(genericptr)
SF_A(int)
SF_A(int16)
SF_A(int32)
SF_A(int64)
SF_A(long)
SF_A(schar)
SF_A(short)
SF_A(size_t)
SF_A(time_t)
SF_A(uchar)
SF_A(uint16)
SF_A(uint32)
SF_A(uint64)
SF_A(ulong)
SF_A(unsigned)
SF_A(ushort)
SF_A(xint16)
SF_A(xint8)
void exportascii_sfo_char(NHFILE *nhfp, char *d_char, const char *myname UNUSED, int cnt);
void exportascii_sfi_char(NHFILE *nhfp, char *d_char, const char *myname UNUSED, int cnt);
void exportascii_sfo_bitfield(NHFILE *nhfp, uint8_t *d_bitfield,
                        const char *myname UNUSED, int bflen UNUSED);
void exportascii_sfi_bitfield(NHFILE *nhfp, uint8_t *d_bitfield,
                        const char *myname UNUSED, int bflen UNUSED);
int critical_members_count_core(NHFILE *nhfp);


/*
 *----------------------------------------------------------------------------
 * Sfo_def_ routines
 *
 * Default output routines.
 *
 *----------------------------------------------------------------------------
 */


void
exportascii_sfo_any(NHFILE *nhfp, union any *d_any, const char *myname UNUSED)
{
    /* const char *parent = "any"; */

    /* nhUse(parent); */
    Sprintf(outbuf, "%llx", (unsigned long long) d_any->a_void);
    put_savefield(nhfp, outbuf, sizeof outbuf);

    Sprintf(outbuf, "%lu", d_any->a_ulong);
    put_savefield(nhfp, outbuf, sizeof outbuf);

    Sprintf(outbuf, "%ld", d_any->a_long);
    put_savefield(nhfp, outbuf, sizeof outbuf);

    Sprintf(outbuf, "%d", d_any->a_uint);
    put_savefield(nhfp, outbuf, sizeof outbuf);

    Sprintf(outbuf, "%d", d_any->a_int);;
    put_savefield(nhfp, outbuf, sizeof outbuf);

    Sprintf(outbuf, "%hd", (short) d_any->a_char);
    put_savefield(nhfp, outbuf, sizeof outbuf);

#if 0
    Sfo_genericptr(nhfp, d_any->a_void, parent, "a_void", 1);      /* (genericptr_t)    */
    Sfo_genericptr(nhfp, d_any->a_obj, parent, "a_obj", 1);        /* (struct obj *)    */
    Sfo_genericptr(nhfp, d_any->a_monst, parent, "a_monst", 1);    /* (struct monst *)  */
    Sfo_int32(nhfp, &d_any->a_int, parent, "a_int", 1);              /* (int)             */
    Sfo_char(nhfp, &d_any->a_char, parent, "a_char", 1);           /* (char)            */
    Sfo_schar(nhfp, &d_any->a_schar, parent, "a_schar", 1);        /* (schar)           */
    Sfo_uchar(nhfp, &d_any->a_uchar, parent, "a_uchar", 1);        /* (uchar)           */
    Sfo_uint(nhfp, &d_any->a_uint, parent, "a_uint", 1);           /* (unsigned int)    */
    Sfo_long(nhfp, &d_any->a_long, parent, "a_long", 1);           /* (long)            */
    Sfo_ulong(nhfp, &d_any->a_ulong, parent, "a_ulong", 1);        /* (unsigned long)   */
    Sfo_genericptr(nhfp, d_any->a_iptr, parent, "a_iptr", 1);      /* (int *)           */
    Sfo_genericptr(nhfp, d_any->a_lptr, parent, "a_lptr", 1);      /* (long *)          */
    Sfo_genericptr(nhfp, d_any->a_ulptr, parent, "a_ulptr", 1);    /* (unsigned long *) */
    Sfo_genericptr(nhfp, d_any->a_uptr, parent, "a_uptr", 1);      /* (unsigned *)      */my
    Sfo_genericptr(nhfp, d_any->a_string, parent, "a_string", 1);  /* (const char *)    */
    Sfo_ulong(nhfp, &d_any->a_mask32, parent, "a_mask32", 1);      /* (unsigned long)   */
#endif
}

void
exportascii_sfo_aligntyp(NHFILE *nhfp, aligntyp *d_aligntyp, const char *myname UNUSED)
{
    int itmp;
    itmp = (int) *d_aligntyp;
    Sprintf(outbuf, "%d", (short) itmp);
    put_savefield(nhfp, outbuf, sizeof outbuf);
}

void
exportascii_sfo_bitfield(NHFILE *nhfp, uint8_t *d_bitfield, const char *myname UNUSED, int bflen UNUSED)
{
    /* for bitfields, cnt is the number of bits, not an array */
    Sprintf(outbuf, "%hu", (unsigned short) *d_bitfield);
    put_savefield(nhfp, outbuf, sizeof outbuf);
}

void
exportascii_sfo_boolean(NHFILE *nhfp, boolean *d_boolean, const char *myname UNUSED)
{
    if (nhfp->fpdebug)
        fprintf(nhfp->fpdebug, "(%s)\n", (*d_boolean) ? "TRUE" : "FALSE");
    Sprintf(outbuf, "%s", *d_boolean ? "true" : "false");
    put_savefield(nhfp, outbuf, sizeof outbuf);
    d_boolean++;
}

void exportascii_sfo_genericptr(NHFILE *nhfp, genericptr_t *d_genericptr,
                          const char *myname UNUSED);

void
exportascii_sfo_genericptr(NHFILE *nhfp, genericptr_t *d_genericptr, const char *myname UNUSED)
{
    unsigned long tmp;
    char *byteptr = (char *) d_genericptr;
    /*
     * sbrooms is an array of pointers to mkroom.
     * That array dimension is MAX_SUBROOMS.
     * Even though the pointers themselves won't
     * be valid, we need to account for the existence
     * of that array and perhaps zero or non-zero.
     */
    tmp = (*d_genericptr) ? 1UL : 0UL;
    Sprintf(outbuf, "%08lu", tmp);
    put_savefield(nhfp, outbuf, sizeof outbuf);
    byteptr += sizeof(void *);
    d_genericptr = (genericptr_t) byteptr;
}

#if 0
void
exportascii_sfo_int32(NHFILE *nhfp, int *d_int, const char *myname UNUSED)
{
    Sprintf(outbuf, "%d", *d_int);
    put_savefield(nhfp, outbuf, sizeof outbuf);
    d_int++;
}
#endif

void
exportascii_sfo_long(NHFILE *nhfp, long *d_long, const char *myname UNUSED)
{
    Sprintf(outbuf, "%ld", *d_long);
    put_savefield(nhfp, outbuf, sizeof outbuf);
    d_long++;
}

void
exportascii_sfo_schar(NHFILE *nhfp, schar *d_schar, const char *myname UNUSED)
{
    int itmp;
    itmp = (int) *d_schar;
    Sprintf(outbuf, "%d", itmp);
    put_savefield(nhfp, outbuf, sizeof outbuf);
    d_schar++;
}

void
exportascii_sfo_int16(NHFILE *nhfp, short *d_short, const char *myname UNUSED)
{
    Sprintf(outbuf, "%hd", *d_short);
    put_savefield(nhfp, outbuf, sizeof outbuf);
    d_short++;
}

void
exportascii_sfo_size_t(NHFILE *nhfp, size_t *d_size_t, const char *myname UNUSED)
{
    unsigned long ul = (unsigned long) *d_size_t;

    Sprintf(outbuf, "%lu", ul);
    put_savefield(nhfp, outbuf, sizeof outbuf);
    d_size_t++;
}

void
exportascii_sfo_time_t(NHFILE *nhfp, time_t *d_time_t, const char *myname UNUSED)
{
    Sprintf(outbuf, "%s", yyyymmddhhmmss(*d_time_t));
    put_savefield(nhfp, outbuf, sizeof outbuf);
}
void
exportascii_sfo_xint8(NHFILE *nhfp, int8 *d_int8, const char *myname UNUSED)
{
    Sprintf(outbuf, "%d", (int) *d_int8);
    put_savefield(nhfp, outbuf, sizeof outbuf);
    d_int8++;
}
#if 0
void
exportascii_sfo_int16(NHFILE *nhfp, int16 *d_int16, const char *myname UNUSED)
{
    Sprintf(outbuf, "%d", (int) *d_int16);
    put_savefield(nhfp, outbuf, sizeof outbuf);
    d_uint16++;
}
#endif

#if 0
void
exportascii_sfo_char(NHFILE *nhfp, xint8 *d_xint8, const char *myname UNUSED, int cnt)
{
    short tmp;

    tmp = (int) *d_xint8;
    Sprintf(outbuf, "%d", tmp);
    put_savefield(nhfp, outbuf, sizeof outbuf);
    d_xint8++;
}
#endif

void
exportascii_sfo_int32(NHFILE *nhfp, int32 *d_int32, const char *myname UNUSED)
{
    Sprintf(outbuf, "%d", *d_int32);
    put_savefield(nhfp, outbuf, sizeof outbuf);
    d_int32++;
}

void
exportascii_sfo_int64(NHFILE *nhfp, int64 *d_int64, const char *myname UNUSED)
{
    Sprintf(outbuf, "%lld", (long long) *d_int64);
    put_savefield(nhfp, outbuf, sizeof outbuf);
    d_int64++;
}

void
exportascii_sfo_int(NHFILE *nhfp, int *d_int, const char *myname UNUSED)
{
    Sprintf(outbuf, "%d", *d_int);
    put_savefield(nhfp, outbuf, sizeof outbuf);
    d_int++;
}
void
exportascii_sfo_short(NHFILE *nhfp, short *d_short, const char *myname UNUSED)
{
    Sprintf(outbuf, "%d", (int) *d_short);
    put_savefield(nhfp, outbuf, sizeof outbuf);
    d_short++;
}
/* aka exportascii_sfo_uint8 */
void
exportascii_sfo_uchar(NHFILE *nhfp, unsigned char *d_uchar, const char *myname UNUSED)
{
    unsigned x_uint32 = (uint32) *d_uchar;

    Sprintf(outbuf, "%u", x_uint32);
    put_savefield(nhfp, outbuf, sizeof outbuf);
    d_uchar++;
}

void
exportascii_sfo_uint16(NHFILE *nhfp, uint16 *d_uint16, const char *myname UNUSED)
{
    Sprintf(outbuf, "%u", (uint32) *d_uint16);
    put_savefield(nhfp, outbuf, sizeof outbuf);
    d_uint16++;
}

void
exportascii_sfo_uint32(NHFILE *nhfp, uint32 *d_uint32, const char *myname UNUSED)
{
    Sprintf(outbuf, "%u", *d_uint32);
    put_savefield(nhfp, outbuf, sizeof outbuf);
    d_uint32++;
}

void
exportascii_sfo_uint64(NHFILE *nhfp, uint64 *d_uint64, const char *myname UNUSED)
{
    Sprintf(outbuf, "%llu", (unsigned long long) *d_uint64);
    put_savefield(nhfp, outbuf, sizeof outbuf);
    d_uint64++;
}

void
exportascii_sfo_ulong(NHFILE *nhfp, unsigned long *d_ulong, const char *myname UNUSED)
{
    Sprintf(outbuf, "%lu", *d_ulong);
    put_savefield(nhfp, outbuf, sizeof outbuf);
    d_ulong++;
}


void
exportascii_sfo_ushort(NHFILE *nhfp, unsigned short *d_ushort, const char *myname UNUSED)
{
    Sprintf(outbuf, "%hu", *d_ushort);
    put_savefield(nhfp, outbuf, sizeof outbuf);
    d_ushort++;
}

void
exportascii_sfo_unsigned(NHFILE *nhfp, unsigned *d_unsigned, const char *myname UNUSED)
{
    Sprintf(outbuf, "%u", *d_unsigned);
    put_savefield(nhfp, outbuf, sizeof outbuf);
    d_unsigned++;
}

void
exportascii_sfo_coordxy(NHFILE *nhfp, coordxy *d_coordxy, const char *myname UNUSED)
{
    short tmp;

    tmp = (short) *d_coordxy;
    Sprintf(outbuf, "%hu", tmp);
    put_savefield(nhfp, outbuf, sizeof outbuf);
    d_coordxy++;
}


void
exportascii_sfo_xint16(NHFILE *nhfp, xint16 *d_xint16, const char *myname UNUSED)
{
    short tmp;

    tmp = (short) *d_xint16;
    Sprintf(outbuf, "%hu", tmp);
    put_savefield(nhfp, outbuf, sizeof outbuf);
    d_xint16++;
}

static char strbuf[BUFSZ * 10];

void
exportascii_sfo_char(NHFILE *nhfp, char *d_char, const char *myname UNUSED, int cnt)
{
    int i, j;
    uint uintval;
    char sval[QBUFSZ];
    uchar *usrc = (uchar *) d_char, *udest = (uchar *)strbuf;

    /* cnt is the number of characters */
    for (i = 0; i < cnt; ++i) {
        if ((*usrc < 32) || (*usrc == '\\') || (*usrc == '|')) {
            *udest++ = '\\';
            uintval = *usrc++;
            Sprintf(sval, "%03u", uintval);
            for (j = 0; j < 3; ++j)
                *udest++ = (uchar) sval[j];
        } else {
            *udest++ = *usrc++;
        }
    }
    *udest = '\0';
    put_savefield(nhfp, strbuf, strlen(strbuf));
}

static void
put_savefield(NHFILE *nhfp, char *obuf, size_t outbufsz UNUSED)
{
    nhfp->wcount++;
    fprintf(nhfp->fpdef, "%07ld|%s\n", nhfp->wcount, obuf);
    fflush(nhfp->fpdef);
}

/*
 *----------------------------------------------------------------------------
 * exportascii_sfi_ routines called from functions in Sfi_base.c
 *----------------------------------------------------------------------------
 */

void exportascii_sfi_any(NHFILE *nhfp, union any *d_any, const char *myname UNUSED);

void
exportascii_sfi_any(NHFILE *nhfp, union any *d_any, const char *myname UNUSED)
{
    char *rstr;
    long long tmp;

    rstr = get_savefield(nhfp, linebuf, BUFSZ);
    tmp = atoll(rstr);
    d_any->a_uint64 = (uint64) tmp;

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
    Sfi_genericptr(nhfp, d_any->a_void, parent, "a_void", 1);
    Sfi_genericptr(nhfp, d_any->a_obj, parent, "a_obj", 1);
    Sfi_genericptr(nhfp, d_any->a_monst, parent, "a_monst", 1);
    Sfi_int32(nhfp, &d_any->a_int, parent, "a_int", 1);
    Sfi_char(nhfp, &d_any->a_char, parent, "a_char", 1);
    Sfi_schar(nhfp, &d_any->a_schar, parent, "a_schar", 1);
    Sfi_uchar(nhfp, &d_any->a_uchar, parent, "a_uchar", 1);
    Sfi_uint(nhfp, &d_any->a_uint, parent, "a_uint", 1);
    Sfi_long(nhfp, &d_any->a_long, parent, "a_long", 1);
    Sfi_ulong(nhfp, &d_any->a_ulong, parent, "a_ulong", 1);
    Sfi_genericptr(nhfp, d_any->a_iptr, parent, "a_iptr", 1);
    Sfi_genericptr(nhfp, d_any->a_lptr, parent, "a_lptr", 1);
    Sfi_genericptr(nhfp, d_any->a_ulptr, parent, "a_ulptr", 1);
    Sfi_genericptr(nhfp, d_any->a_uptr, parent, "a_uptr", 1);
    Sfi_genericptr(nhfp, d_any->a_string, parent, "a_string", 1);
    Sfi_ulong(nhfp, &d_any->a_mask32, parent, "a_mask32", 1);
#endif
}

void
exportascii_sfi_aligntyp(NHFILE *nhfp, aligntyp *d_aligntyp, const char *myname UNUSED)
{
    char *rstr;
    aligntyp tmp;
    long long lltmp;

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
exportascii_sfi_bitfield(NHFILE *nhfp, uint8_t *d_bitfield, const char *myname UNUSED, int bflen UNUSED)
{
    char *rstr;
    uint8_t tmp;

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
exportascii_sfi_boolean(NHFILE *nhfp, boolean *d_boolean, const char *myname UNUSED)
{
    char *rstr;

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

void exportascii_sfi_genericptr(NHFILE *nhfp, genericptr_t *d_genericptr,
                          const char *myname UNUSED);

void
exportascii_sfi_genericptr(NHFILE *nhfp, genericptr_t *d_genericptr, const char *myname UNUSED)
{
    long long lltmp;
    char *rstr;
    static const char *glorkum = "glorkum";

    /*
     * sbrooms is an array of pointers to mkroom.
     * That array dimension is MAX_SUBROOMS.
     * Even though the pointers themselves won't
     * be valid, we need to account for the existence
     * of that array.
     */
    /* these pointers can't actually be valid */
    rstr = get_savefield(nhfp, linebuf, sizeof linebuf);
    lltmp = atoll(rstr);
    *d_genericptr = lltmp ? (genericptr_t) glorkum : (genericptr_t) 0;
}

void
exportascii_sfi_int32(NHFILE *nhfp, int32 *d_int32, const char *myname UNUSED)
{
    int32 tmp;
    char *rstr;
    long ltmp;

    rstr = get_savefield(nhfp, linebuf, BUFSZ);
    ltmp = atol(rstr);
    tmp = (int32) ltmp;
#ifdef SAVEFILE_DEBUGGING
    if (nhfp->structlevel && tmp != *d_int32)
        report_problem_ascii(nhfp, myparent, myname, parent);
    else
#endif
    *d_int32 = (int32) tmp;
    d_int32++;
}

void
exportascii_sfi_int(NHFILE *nhfp, int *d_int, const char *myname UNUSED)
{
    int tmp;
    char *rstr;
    long ltmp;

    rstr = get_savefield(nhfp, linebuf, BUFSZ);
    ltmp = atol(rstr);
    tmp = (int) ltmp;
#ifdef SAVEFILE_DEBUGGING
    if (nhfp->structlevel && tmp != *d_int)
        report_problem_ascii(nhfp, myparent, myname, parent);
    else
#endif
        *d_int = (int32) tmp;
    d_int++;
}
void
exportascii_sfi_long(NHFILE *nhfp, long *d_long, const char *myname UNUSED)
{
    long tmp;
    long long lltmp;
    char *rstr;

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

void
exportascii_sfi_schar(NHFILE *nhfp, schar *d_schar, const char *myname UNUSED)
{
    schar tmp;
    char *rstr;

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

void
exportascii_sfi_int16(NHFILE *nhfp, short *d_short, const char *myname UNUSED)
{
    short tmp;
    char *rstr;

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

void
exportascii_sfi_int64(NHFILE *nhfp, int64 *d_int64, const char *myname UNUSED)
{
    int64 tmp;
    char *rstr;

    rstr = get_savefield(nhfp, linebuf, BUFSZ);
    tmp = (int64) atol(rstr);
#ifdef SAVEFILE_DEBUGGING
    if (nhfp->structlevel && tmp != *d_int64)
        report_problem_ascii(nhfp, myparent, myname, parent);
    else
#endif
        *d_int64 = tmp;
    d_int64++;
}
void
exportascii_sfi_size_t(NHFILE *nhfp, size_t *d_size_t, const char *myname UNUSED)
{
    size_t tmp;
    char *rstr;

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

void
exportascii_sfi_time_t(NHFILE *nhfp, time_t *d_time_t, const char *myname UNUSED)
{
    time_t tmp;
    char *rstr;

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

#if 0
void
exportascii_sfi_uint32(NHFILE *nhfp, unsigned *d_unsigned, const char *myname)
{
    /* deferal */
    exportascii_sfi_uint(nhfp, d_unsigned, myname);
}
#endif

void
exportascii_sfi_uchar(NHFILE *nhfp, unsigned char *d_uchar, const char *myname UNUSED)
{
    uchar tmp;
    int itmp;
    char *rstr;

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

void
exportascii_sfi_uint16(NHFILE *nhfp, uint16 *d_uint16, const char *myname UNUSED)
{
    char *rstr;
    unsigned int tmp;
    long long lltmp;

    rstr = get_savefield(nhfp, linebuf, BUFSZ);
    lltmp = atoll(rstr);
    tmp = (unsigned int) lltmp;
#ifdef SAVEFILE_DEBUGGING
    if (nhfp->structlevel && tmp != *d_uint)
        report_problem_ascii(nhfp, myparent, myname, parent);
    else
#endif
    *d_uint16 = tmp;
    d_uint16++;
}

void
exportascii_sfi_uint32(NHFILE *nhfp, uint32 *d_uint32, const char *myname UNUSED)
{
    char *rstr;
    uint32 tmp;
    long long lltmp;

    rstr = get_savefield(nhfp, linebuf, BUFSZ);
    lltmp = atoll(rstr);
    tmp = (uint32) lltmp;
#ifdef SAVEFILE_DEBUGGING
    if (nhfp->structlevel && tmp != *d_uint)
        report_problem_ascii(nhfp, myparent, myname, parent);
    else
#endif
    *d_uint32 = tmp;
    d_uint32++;
}

void
exportascii_sfi_uint64(NHFILE *nhfp, uint64 *d_uint64, const char *myname UNUSED)
{
    char *rstr;
    uint64 tmp;
    long long lltmp;

    rstr = get_savefield(nhfp, linebuf, BUFSZ);
    lltmp = atoll(rstr);
    tmp = (uint64) lltmp;
#ifdef SAVEFILE_DEBUGGING
    if (nhfp->structlevel && tmp != *d_uint)
        report_problem_ascii(nhfp, myparent, myname, parent);
    else
#endif
    *d_uint64 = tmp;
    d_uint64++;
}

void
exportascii_sfi_unsigned(NHFILE *nhfp, unsigned *d_unsigned, const char *myname UNUSED)
{
    char *rstr;
    uint32 tmp;
    long long lltmp;

    rstr = get_savefield(nhfp, linebuf, BUFSZ);
    lltmp = atoll(rstr);
    tmp = (uint32) lltmp;
#ifdef SAVEFILE_DEBUGGING
    if (nhfp->structlevel && tmp != *d_unsigned)
        report_problem_ascii(nhfp, "", myname, parent);
    else
#endif
        *d_unsigned = tmp;
    d_unsigned++;
}

void
exportascii_sfi_ulong(NHFILE *nhfp, unsigned long *d_ulong, const char *myname UNUSED)
{
    unsigned long tmp;
    long long lltmp;
    char *rstr;

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

void
exportascii_sfi_ushort(NHFILE *nhfp, unsigned short *d_ushort, const char *myname UNUSED)
{
    short tmp;
    long long lltmp;
    char *rstr;

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

void
exportascii_sfi_coordxy(NHFILE *nhfp, coordxy *d_coordxy, const char *myname UNUSED)
{
    coordxy tmp;
    int itmp;
    char *rstr;

    rstr = get_savefield(nhfp, linebuf, BUFSZ);
    itmp = atoi(rstr);
    tmp = (coordxy) itmp;
#ifdef SAVEFILE_DEBUGGING
    if (nhfp->structlevel && tmp != *d_coordxy)
        report_problem_ascii(nhfp, myparent, myname, parent);
    else
#endif
    *d_coordxy = tmp;
    d_coordxy++;
}

void
exportascii_sfi_xint8(NHFILE *nhfp, xint8 *d_xint8, const char *myname UNUSED)
{
    xint8 tmp;
    int itmp;
    char *rstr;

    rstr = get_savefield(nhfp, linebuf, BUFSZ);
    if (!nhfp->eof) {
        itmp = atoi(rstr);
        tmp = (xint8) itmp;
    } else {
        return;
    }
#ifdef SAVEFILE_DEBUGGING
    if (nhfp->structlevel && tmp != *d_xint8)
        report_problem_ascii(nhfp, myparent, myname, parent);
    else
#endif
    *d_xint8 = tmp;
    d_xint8++;
}

void
exportascii_sfi_xint16(NHFILE *nhfp, xint16 *d_xint16, const char *myname UNUSED)
{
    xint16 tmp;
    int itmp;
    char *rstr;

    rstr = get_savefield(nhfp, linebuf, BUFSZ);
    itmp = atoi(rstr);
    tmp = (xint16) itmp;
#ifdef SAVEFILE_DEBUGGING
    if (nhfp->structlevel && tmp != *d_xint16)
        report_problem_ascii(nhfp, myparent, myname, parent);
    else
#endif
    *d_xint16 = tmp;
    d_xint16++;
}

void
exportascii_sfi_short(NHFILE *nhfp, short *d_short, const char *myname UNUSED)
{
    xint16 tmp;
    int itmp;
    char *rstr;

    rstr = get_savefield(nhfp, linebuf, BUFSZ);
    itmp = atoi(rstr);
    tmp = (xint16) itmp;
#ifdef SAVEFILE_DEBUGGING
    if (nhfp->structlevel && tmp != *d_short)
        report_problem_ascii(nhfp, "", myname, parent);
    else
#endif
        *d_short = tmp;
    d_short++;
}


void
exportascii_sfi_char(NHFILE *nhfp, char *d_char, const char *myname UNUSED, int cnt)
{
    int i, j, sval;
    char n[4], *rstr;
    char *src, *dest;

    /* cnt is the length of the string */
    rstr = get_savefield(nhfp, strbuf, sizeof strbuf);
    if (!rstr) {
        nhfp->eof = TRUE;
        return;
    }
    src = rstr;
    dest =
#ifdef SAVEFILE_DEBUGGING
            testbuf;
#else
            d_char;
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
        dest = d_char;
        match = TRUE;
        for (i = 0; i < cnt; ++i) {
            if (*src++ != *dest++)
                match = FALSE;
        }
        if (!match)
            report_problem_ascii(nhfp, myparent, myname, parent);
        else {
            src = testbuf;
            dest = d_char;
            for (i = 0; i < cnt; ++i)
                *dest++ = *src++;
        }
    }
#endif
}

char *
get_savefield(NHFILE *nhfp, char *inbuf, size_t inbufsz)
{
    char *ep, *sep;
    char *res = 0;

    if ((res =fgets(inbuf, (int) inbufsz, nhfp->fpdef)) != 0) {
        nhfp->rcount++;
        ep = strchr(inbuf, '\n');
        if (!ep) {  /* newline missing */
            if (strlen(inbuf) < (inbufsz - 2)) {
                /* likely the last line of file is just
                   missing a newline; process it anyway  */
                ep = eos(inbuf);
            }
        }
        if (ep)
            *ep = '\0'; /* remove newline */
        sep = strchr(inbuf, '|');
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
report_problem_ascii(NHFILE *nhfp, const char *s1, const char *s2, const char *s3)
{
    fprintf(nhfp->fpdebug, "faulty value preservation "
            "(%ld, %s, %s, %s)\n", 
            ((nhfp->mode & READING) != 0) ? nhfp->rcount : nhfp->wcount, s1, s2, s3);
}
#endif

int exportascii_critical_members_count(void);

int
exportascii_critical_members_count(void)
{
    return 0;
}

struct sf_fieldlevel_procs exportascii_sfo_procs = {
    "exportascii",
    /* sf_x */
    {
        sfo_x_arti_info,
        sfo_x_nhrect,
        sfo_x_branch,
        sfo_x_bubble,
        sfo_x_cemetery,
        sfo_x_context_info,
        sfo_x_nhcoord,
        sfo_x_damage,
        sfo_x_dest_area,
        sfo_x_dgn_topology,
        sfo_x_dungeon,
        sfo_x_d_level,
        sfo_x_ebones,
        sfo_x_edog,
        sfo_x_egd,
        sfo_x_emin,
        sfo_x_engr,
        sfo_x_epri,
        sfo_x_eshk,
        sfo_x_fe,
        sfo_x_flag,
        sfo_x_fruit,
        sfo_x_gamelog_line,
        sfo_x_kinfo,
        sfo_x_levelflags,
        sfo_x_ls_t,
        sfo_x_linfo,
        sfo_x_mapseen_feat,
        sfo_x_mapseen_flags,
        sfo_x_mapseen_rooms,
        sfo_x_mkroom,
        sfo_x_monst,
        sfo_x_mvitals,
        sfo_x_obj,
        sfo_x_objclass,
        sfo_x_q_score,
        sfo_x_rm,
        sfo_x_spell,
        sfo_x_stairway,
        sfo_x_s_level,
        sfo_x_trap,
        sfo_x_version_info,
        sfo_x_you,

        exportascii_sfo_any,
        exportascii_sfo_aligntyp,
        exportascii_sfo_boolean,
        exportascii_sfo_coordxy,
        exportascii_sfo_genericptr,
        exportascii_sfo_int,
        exportascii_sfo_int16,
        exportascii_sfo_int32,
        exportascii_sfo_int64,
        exportascii_sfo_long,
        exportascii_sfo_schar,
        exportascii_sfo_short,
        exportascii_sfo_size_t,
        exportascii_sfo_time_t,
        exportascii_sfo_uchar,
        exportascii_sfo_uint16,
        exportascii_sfo_uint32,
        exportascii_sfo_uint64,
        exportascii_sfo_ulong,
        exportascii_sfo_unsigned,
        exportascii_sfo_ushort,
        exportascii_sfo_xint16,
        exportascii_sfo_xint8,
        exportascii_sfo_char,
        exportascii_sfo_bitfield,
    }
};

struct sf_fieldlevel_procs exportascii_sfi_procs = {
    "le",
    /* sf_x */
    {
        sfi_x_arti_info,
        sfi_x_nhrect,
        sfi_x_branch,
        sfi_x_bubble,
        sfi_x_cemetery,
        sfi_x_context_info,
        sfi_x_nhcoord,
        sfi_x_damage,
        sfi_x_dest_area,
        sfi_x_dgn_topology,
        sfi_x_dungeon,
        sfi_x_d_level,
        sfi_x_ebones,
        sfi_x_edog,
        sfi_x_egd,
        sfi_x_emin,
        sfi_x_engr,
        sfi_x_epri,
        sfi_x_eshk,
        sfi_x_fe,
        sfi_x_flag,
        sfi_x_fruit,
        sfi_x_gamelog_line,
        sfi_x_kinfo,
        sfi_x_levelflags,
        sfi_x_ls_t,
        sfi_x_linfo,
        sfi_x_mapseen_feat,
        sfi_x_mapseen_flags,
        sfi_x_mapseen_rooms,
        sfi_x_mkroom,
        sfi_x_monst,
        sfi_x_mvitals,
        sfi_x_obj,
        sfi_x_objclass,
        sfi_x_q_score,
        sfi_x_rm,
        sfi_x_spell,
        sfi_x_stairway,
        sfi_x_s_level,
        sfi_x_trap,
        sfi_x_version_info,
        sfi_x_you,

        exportascii_sfi_any,
        exportascii_sfi_aligntyp,
        exportascii_sfi_boolean,
        exportascii_sfi_coordxy,
        exportascii_sfi_genericptr,
        exportascii_sfi_int,
        exportascii_sfi_int16,
        exportascii_sfi_int32,
        exportascii_sfi_int64,
        exportascii_sfi_long,
        exportascii_sfi_schar,
        exportascii_sfi_short,
        exportascii_sfi_size_t,
        exportascii_sfi_time_t,
        exportascii_sfi_uchar,
        exportascii_sfi_uint16,
        exportascii_sfi_uint32,
        exportascii_sfi_uint64,
        exportascii_sfi_ulong,
        exportascii_sfi_unsigned,
        exportascii_sfi_ushort,
        exportascii_sfi_xint16,
        exportascii_sfi_xint8,
        exportascii_sfi_char,
        exportascii_sfi_bitfield,
    } 
};
