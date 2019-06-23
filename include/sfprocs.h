/* NetHack 3.6 sfprocs.h	Tue Nov  6 19:38:48 2018 */
/* Copyright (c) NetHack Development Team 2018.                   */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef SFPROCS_H
#define SFPROCS_H

#include "hack.h"
#include "integer.h"
#include "wintype.h"

#define E extern

/* output routines */
E void FDECL(ascii_sfo_aligntyp, (NHFILE *, aligntyp *, const char *, const char *, int));
E void FDECL(ascii_sfo_any, (NHFILE *, union any *d_any, const char *, const char *, int));
E void FDECL(ascii_sfo_bitfield, (NHFILE *, uint8_t *, const char *, const char *, int));
E void FDECL(ascii_sfo_boolean, (NHFILE *, boolean *, const char *, const char *, int));
E void FDECL(ascii_sfo_char, (NHFILE *, char *, const char *, const char *, int));
E void FDECL(ascii_sfo_genericptr, (NHFILE *, genericptr_t *, const char *, const char *, int));
E void FDECL(ascii_sfo_int, (NHFILE *, int *, const char *, const char *, int));
E void FDECL(ascii_sfo_long, (NHFILE *, long *, const char *, const char *, int));
E void FDECL(ascii_sfo_schar, (NHFILE *, schar *, const char *, const char *, int));
E void FDECL(ascii_sfo_short, (NHFILE *, short *, const char *, const char *, int));
E void FDECL(ascii_sfo_size_t, (NHFILE *, size_t *, const char *, const char *, int));
E void FDECL(ascii_sfo_time_t, (NHFILE *, time_t *, const char *, const char *, int));
E void FDECL(ascii_sfo_unsigned, (NHFILE *, unsigned *, const char *, const char *, int));
E void FDECL(ascii_sfo_uchar, (NHFILE *, unsigned char *, const char *, const char *, int));
E void FDECL(ascii_sfo_uint, (NHFILE *, unsigned int *, const char *, const char *, int));
E void FDECL(ascii_sfo_ulong, (NHFILE *, unsigned long *, const char *, const char *, int));
E void FDECL(ascii_sfo_ushort, (NHFILE *, unsigned short *, const char *, const char *, int));
E void FDECL(ascii_sfo_xchar, (NHFILE *, xchar *, const char *, const char *, int));
E void FDECL(ascii_sfo_str, (NHFILE *, char *, const char *, const char *, int));
E void FDECL(ascii_sfo_addinfo, (NHFILE *, const char *, const char *, const char *, int));

/* input routines */
E void FDECL(ascii_sfi_aligntyp, (NHFILE *, aligntyp *, const char *, const char *, int));
E void FDECL(ascii_sfi_any, (NHFILE *, union any *d_any, const char *, const char *, int));
E void FDECL(ascii_sfi_bitfield, (NHFILE *, uint8_t *, const char *, const char *, int));
E void FDECL(ascii_sfi_boolean, (NHFILE *, boolean *, const char *, const char *, int));
E void FDECL(ascii_sfi_char, (NHFILE *, char *, const char *, const char *, int));
E void FDECL(ascii_sfi_genericptr, (NHFILE *, genericptr_t *, const char *, const char *, int));
E void FDECL(ascii_sfi_int, (NHFILE *, int *, const char *, const char *, int));
E void FDECL(ascii_sfi_long, (NHFILE *, long *, const char *, const char *, int));
E void FDECL(ascii_sfi_schar, (NHFILE *, schar *, const char *, const char *, int));
E void FDECL(ascii_sfi_short, (NHFILE *, short *, const char *, const char *, int));
E void FDECL(ascii_sfi_size_t, (NHFILE *, size_t *, const char *, const char *, int));
E void FDECL(ascii_sfi_time_t, (NHFILE *, time_t *, const char *, const char *, int));
E void FDECL(ascii_sfi_unsigned, (NHFILE *, unsigned *, const char *, const char *, int));
E void FDECL(ascii_sfi_uchar, (NHFILE *, unsigned char *, const char *, const char *, int));
E void FDECL(ascii_sfi_uint, (NHFILE *, unsigned int *, const char *, const char *, int));
E void FDECL(ascii_sfi_ulong, (NHFILE *, unsigned long *, const char *, const char *, int));
E void FDECL(ascii_sfi_ushort, (NHFILE *, unsigned short *, const char *, const char *, int));
E void FDECL(ascii_sfi_xchar, (NHFILE *, xchar *, const char *, const char *, int));
E void FDECL(ascii_sfi_str, (NHFILE *, char *, const char *, const char *, int));
E void FDECL(ascii_sfi_addinfo, (NHFILE *, const char *, const char *, const char *, int));

/* output routines */
E void FDECL(lendian_sfo_aligntyp, (NHFILE *, aligntyp *, const char *, const char *, int));
E void FDECL(lendian_sfo_any, (NHFILE *, union any *d_any, const char *, const char *, int));
E void FDECL(lendian_sfo_bitfield, (NHFILE *, uint8_t *, const char *, const char *, int));
E void FDECL(lendian_sfo_boolean, (NHFILE *, boolean *, const char *, const char *, int));
E void FDECL(lendian_sfo_char, (NHFILE *, char *, const char *, const char *, int));
E void FDECL(lendian_sfo_genericptr, (NHFILE *, genericptr_t *, const char *, const char *, int));
E void FDECL(lendian_sfo_int, (NHFILE *, int *, const char *, const char *, int));
E void FDECL(lendian_sfo_long, (NHFILE *, long *, const char *, const char *, int));
E void FDECL(lendian_sfo_schar, (NHFILE *, schar *, const char *, const char *, int));
E void FDECL(lendian_sfo_short, (NHFILE *, short *, const char *, const char *, int));
E void FDECL(lendian_sfo_size_t, (NHFILE *, size_t *, const char *, const char *, int));
E void FDECL(lendian_sfo_time_t, (NHFILE *, time_t *, const char *, const char *, int));
E void FDECL(lendian_sfo_unsigned, (NHFILE *, unsigned *, const char *, const char *, int));
E void FDECL(lendian_sfo_uchar, (NHFILE *, unsigned char *, const char *, const char *, int));
E void FDECL(lendian_sfo_uint, (NHFILE *, unsigned int *, const char *, const char *, int));
E void FDECL(lendian_sfo_ulong, (NHFILE *, unsigned long *, const char *, const char *, int));
E void FDECL(lendian_sfo_ushort, (NHFILE *, unsigned short *, const char *, const char *, int));
E void FDECL(lendian_sfo_xchar, (NHFILE *, xchar *, const char *, const char *, int));
E void FDECL(lendian_sfo_str, (NHFILE *, char *, const char *, const char *, int));
E void FDECL(lendian_sfo_addinfo, (NHFILE *, const char *, const char *, const char *, int));

/* input routines */
E void FDECL(lendian_sfi_aligntyp, (NHFILE *, aligntyp *, const char *, const char *, int));
E void FDECL(lendian_sfi_any, (NHFILE *, union any *d_any, const char *, const char *, int));
E void FDECL(lendian_sfi_bitfield, (NHFILE *, uint8_t *, const char *, const char *, int));
E void FDECL(lendian_sfi_boolean, (NHFILE *, boolean *, const char *, const char *, int));
E void FDECL(lendian_sfi_char, (NHFILE *, char *, const char *, const char *, int));
E void FDECL(lendian_sfi_genericptr, (NHFILE *, genericptr_t *, const char *, const char *, int));
E void FDECL(lendian_sfi_int, (NHFILE *, int *, const char *, const char *, int));
E void FDECL(lendian_sfi_long, (NHFILE *, long *, const char *, const char *, int));
E void FDECL(lendian_sfi_schar, (NHFILE *, schar *, const char *, const char *, int));
E void FDECL(lendian_sfi_short, (NHFILE *, short *, const char *, const char *, int));
E void FDECL(lendian_sfi_size_t, (NHFILE *, size_t *, const char *, const char *, int));
E void FDECL(lendian_sfi_time_t, (NHFILE *, time_t *, const char *, const char *, int));
E void FDECL(lendian_sfi_unsigned, (NHFILE *, unsigned *, const char *, const char *, int));
E void FDECL(lendian_sfi_uchar, (NHFILE *, unsigned char *, const char *, const char *, int));
E void FDECL(lendian_sfi_uint, (NHFILE *, unsigned int *, const char *, const char *, int));
E void FDECL(lendian_sfi_ulong, (NHFILE *, unsigned long *, const char *, const char *, int));
E void FDECL(lendian_sfi_ushort, (NHFILE *, unsigned short *, const char *, const char *, int));
E void FDECL(lendian_sfi_xchar, (NHFILE *, xchar *, const char *, const char *, int));
E void FDECL(lendian_sfi_str, (NHFILE *, char *, const char *, const char *, int));
E void FDECL(lendian_sfi_addinfo, (NHFILE *, const char *, const char *, const char *, int));

#ifdef JSON_SUPPORT
/* output routines */
E void FDECL(json_sfo_aligntyp, (NHFILE *, aligntype *, const char *, const char *, int));
E void FDECL(json_sfo_any, (NHFILE *, union any *d_any, const char *, const char *, int));
E void FDECL(json_sfo_bitfield, (NHFILE *, uint8_t *, const char *, const char *, int));
E void FDECL(json_sfo_boolean, (NHFILE *, boolean *, const char *, const char *, int));
E void FDECL(json_sfo_char, (NHFILE *, char *, const char *, const char *, int));
E void FDECL(json_sfo_genericptr, (NHFILE *, genericptr_t *, const char *, const char *, int));
E void FDECL(json_sfo_int, (NHFILE *, int *, const char *, const char *, int));
E void FDECL(json_sfo_long, (NHFILE *, long *, const char *, const char *, int));
E void FDECL(json_sfo_schar, (NHFILE *, schar *, const char *, const char *, int));
E void FDECL(json_sfo_short, (NHFILE *, short *, const char *, const char *, int));
E void FDECL(json_sfo_size_t, (NHFILE *, size_t *, const char *, const char *, int));
E void FDECL(json_sfo_time_t, (NHFILE *, time_t *, const char *, const char *, int));
E void FDECL(json_sfo_unsigned, (NHFILE *, unsigned *, const char *, const char *, int));
E void FDECL(json_sfo_uchar, (NHFILE *, unsigned char *, const char *, const char *, int));
E void FDECL(json_sfo_uint, (NHFILE *, unsigned int *, const char *, const char *, int));
E void FDECL(json_sfo_ulong, (NHFILE *, unsigned long *, const char *, const char *, int));
E void FDECL(json_sfo_ushort, (NHFILE *, unsigned short *, const char *, const char *, int));
E void FDECL(json_sfo_xchar, (NHFILE *, xchar *, const char *, const char *, int));
E void FDECL(json_sfo_str, (NHFILE *, char *, const char *, const char *, int));
E void FDECL(json_sfo_addinfo), (NHFILE *, const char *, const char *, const char *, int));

/* input routines */
E void FDECL(json_sfi_aligntyp, (NHFILE *, aligntype *, const char *, const char *, int));
E void FDECL(json_sfi_any, (NHFILE *, union any *d_any, const char *, const char *, int));
E void FDECL(json_sfi_bitfield, (NHFILE *, uint8_t *, const char *, const char *, int));
E void FDECL(json_sfi_boolean, (NHFILE *, boolean *, const char *, const char *, int));
E void FDECL(json_sfi_char, (NHFILE *, char *, const char *, const char *, int));
E void FDECL(json_sfi_genericptr, (NHFILE *, genericptr_t *, const char *, const char *, int));
E void FDECL(json_sfi_int, (NHFILE *, int *, const char *, const char *, int));
E void FDECL(json_sfi_long, (NHFILE *, long *, const char *, const char *, int));
E void FDECL(json_sfi_schar, (NHFILE *, schar *, const char *, const char *, int));
E void FDECL(json_sfi_short, (NHFILE *, short *, const char *, const char *, int));
E void FDECL(json_sfi_size_t, (NHFILE *, size_t *, const char *, const char *, int));
E void FDECL(json_sfi_time_t, (NHFILE *, time_t *, const char *, const char *, int));
E void FDECL(json_sfi_unsigned, (NHFILE *, unsigned *, const char *, const char *, int));
E void FDECL(json_sfi_uchar, (NHFILE *, unsigned char *, const char *, const char *, int));
E void FDECL(json_sfi_uint, (NHFILE *, unsigned int *, const char *, const char *, int));
E void FDECL(json_sfi_ulong, (NHFILE *, unsigned long *, const char *, const char *, int));
E void FDECL(json_sfi_ushort, (NHFILE *, unsigned short *, const char *, const char *, int));
E void FDECL(json_sfi_xchar, (NHFILE *, xchar *, const char *, const char *, int));
E void FDECL(json_sfi_str, (NHFILE *, char *, const char *, const char *, int));
E void FDECL(json_sfi_addinfo), (NHFILE *, const char *, const char *, const char *, int));
#endif /* JSON_SUPPORT */

struct fieldlevel_procs {
    void FDECL((*sf_aligntyp), (NHFILE *, aligntyp *, const char *, const char *, int));
    void FDECL((*sf_any), (NHFILE *, union any *d_any, const char *, const char *, int));
    void FDECL((*sf_bitfield), (NHFILE *, uint8_t *, const char *, const char *, int));
    void FDECL((*sf_boolean), (NHFILE *, boolean *, const char *, const char *, int));
    void FDECL((*sf_char), (NHFILE *, char *, const char *, const char *, int));
    void FDECL((*sf_genericptr), (NHFILE *, genericptr_t *, const char *, const char *, int));
    void FDECL((*sf_int), (NHFILE *, int *, const char *, const char *, int));
    void FDECL((*sf_long), (NHFILE *, long *, const char *, const char *, int));
    void FDECL((*sf_schar), (NHFILE *, schar *, const char *, const char *, int));
    void FDECL((*sf_short), (NHFILE *, short *, const char *, const char *, int));
    void FDECL((*sf_size_t), (NHFILE *, size_t *, const char *, const char *, int));
    void FDECL((*sf_time_t), (NHFILE *, time_t *, const char *, const char *, int));
    void FDECL((*sf_unsigned), (NHFILE *, unsigned *, const char *, const char *, int));
    void FDECL((*sf_uchar), (NHFILE *, unsigned char *, const char *, const char *, int));
    void FDECL((*sf_uint), (NHFILE *, unsigned int *, const char *, const char *, int));
    void FDECL((*sf_ulong), (NHFILE *, unsigned long *, const char *, const char *, int));
    void FDECL((*sf_ushort), (NHFILE *, unsigned short *, const char *, const char *, int));
    void FDECL((*sf_xchar), (NHFILE *, xchar *, const char *, const char *, int));
    void FDECL((*sf_str), (NHFILE *, char *, const char *, const char *, int));
    void FDECL((*sf_addinfo), (NHFILE *, const char *, const char *, const char *, int));
};

struct sf_procs {
    const char *ext;
    struct fieldlevel_procs fn;
};

E void NDECL(sf_init);
E struct sf_procs sfoprocs[4], sfiprocs[4];

E struct sf_procs ascii_sfo_procs;
E struct sf_procs ascii_sfi_procs;
E struct sf_procs lendian_sfo_procs;
E struct sf_procs lendian_sfi_procs;
E struct sf_procs stub_sfo_procs;
E struct sf_procs stub_sfi_procs;

#ifdef JSON_SUPPORT
E struct sf_procs json_sfo_procs;
E struct sf_procs json_sfi_procs;
#endif

#undef E
#endif /* SFPROCS_H */
