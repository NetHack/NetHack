/* NetHack 3.6 sfprocs.h	Tue Nov  6 19:38:48 2018 */
/* Copyright (c) NetHack Development Team 2025.                   */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef SFPROCS_H
#define SFPROCS_H

#define NHTYPE_SIMPLE    1
#define NHTYPE_COMPLEX   2

#define SF_PROTO(dtyp) \
    extern void sfo_##dtyp(NHFILE *, dtyp *, const char *);   \
    extern void sfi_##dtyp(NHFILE *, dtyp *, const char *);   \
    extern void sfo_x_##dtyp(NHFILE *, dtyp *, const char *); \
    extern void sfi_x_##dtyp(NHFILE *, dtyp *, const char *)
#define SF_PROTO_C(keyw, dtyp) \
    extern void sfo_##dtyp(NHFILE *, keyw dtyp *, const char *);   \
    extern void sfi_##dtyp(NHFILE *, keyw dtyp *, const char *);   \
    extern void sfo_x_##dtyp(NHFILE *, keyw dtyp *, const char *); \
    extern void sfi_x_##dtyp(NHFILE *, keyw dtyp *, const char *)
#define SF_PROTO_X(xxx, dtyp) \
    extern void sfo_##dtyp(NHFILE *, xxx *, const char *, int bfsz);   \
    extern void sfi_##dtyp(NHFILE *, xxx *, const char *, int bfsz);   \
    extern void sfo_x_##dtyp(NHFILE *, xxx *, const char *, int bfsz); \
    extern void sfi_x_##dtyp(NHFILE *, xxx *, const char *, int bfsz)

SF_PROTO_C(struct, arti_info);
SF_PROTO_C(struct, nhrect);
SF_PROTO_C(struct, branch);
SF_PROTO_C(struct, bubble);
SF_PROTO_C(struct, cemetery);
SF_PROTO_C(struct, context_info);
SF_PROTO_C(struct, nhcoord);
SF_PROTO_C(struct, damage);
SF_PROTO_C(struct, dest_area);
SF_PROTO_C(struct, dgn_topology);
SF_PROTO_C(struct, dungeon);
SF_PROTO_C(struct, d_level);
SF_PROTO_C(struct, ebones);
SF_PROTO_C(struct, edog);
SF_PROTO_C(struct, egd);
SF_PROTO_C(struct, emin);
SF_PROTO_C(struct, engr);
SF_PROTO_C(struct, epri);
SF_PROTO_C(struct, eshk);
SF_PROTO_C(struct, fe);
SF_PROTO_C(struct, flag);
SF_PROTO_C(struct, fruit);
SF_PROTO_C(struct, gamelog_line);
SF_PROTO_C(struct, kinfo);
SF_PROTO_C(struct, levelflags);
SF_PROTO_C(struct, ls_t);
SF_PROTO_C(struct, linfo);
SF_PROTO_C(struct, mapseen_feat);
SF_PROTO_C(struct, mapseen_flags);
SF_PROTO_C(struct, mapseen_rooms);
SF_PROTO_C(struct, mkroom);
SF_PROTO_C(struct, monst);
SF_PROTO_C(struct, mvitals);
SF_PROTO_C(struct, obj);
SF_PROTO_C(struct, objclass);
SF_PROTO_C(struct, q_score);
SF_PROTO_C(struct, rm);
SF_PROTO_C(struct, spell);
SF_PROTO_C(struct, stairway);
SF_PROTO_C(struct, s_level);
SF_PROTO_C(struct, trap);
SF_PROTO_C(struct, version_info);
SF_PROTO_C(struct, you);
SF_PROTO_C(union, any);
#ifdef DEMO_UPLIFTS
SF_PROTO_C(struct, mystruct);
SF_PROTO_C(struct, mystruct_rev0);
#endif
SF_PROTO(int16);
SF_PROTO(int32);
SF_PROTO(int64);
SF_PROTO(uchar);
SF_PROTO(uint16);
SF_PROTO(uint32);
SF_PROTO(uint64);
SF_PROTO(long);
SF_PROTO(ulong);
SF_PROTO(xint8);
SF_PROTO(boolean);
SF_PROTO(schar);
SF_PROTO(aligntyp);
SF_PROTO(genericptr);
SF_PROTO(size_t);
SF_PROTO(time_t);
SF_PROTO(int);
SF_PROTO(unsigned);
SF_PROTO(coordxy);
SF_PROTO(short);
SF_PROTO(xint16);
SF_PROTO(ushort);
SF_PROTO_X(uint8_t, bitfield);
SF_PROTO_X(char, char);

#undef SF_PROTO
#undef SF_PROTO_C
#undef SF_PROTO_X

#define SF_ENTRY(dtyp)                                 \
    void (*sf_##dtyp)(NHFILE *, dtyp *, const char *)
#define SF_ENTRY_C(keyw, dtyp)                              \
    void (*sf_##dtyp)(NHFILE *, keyw dtyp *, const char *)
#define SF_ENTRY_X(xxx, dtyp)                                   \
    void (*sf_##dtyp)(NHFILE *, xxx *, const char *, int bfsz)

struct sf_procs {
    SF_ENTRY_C(struct, arti_info);
    SF_ENTRY_C(struct, nhrect);
    SF_ENTRY_C(struct, branch);
    SF_ENTRY_C(struct, bubble);
    SF_ENTRY_C(struct, cemetery);
    SF_ENTRY_C(struct, context_info);
    SF_ENTRY_C(struct, nhcoord);
    SF_ENTRY_C(struct, damage);
    SF_ENTRY_C(struct, dest_area);
    SF_ENTRY_C(struct, dgn_topology);
    SF_ENTRY_C(struct, dungeon);
    SF_ENTRY_C(struct, d_level);
    SF_ENTRY_C(struct, ebones);
    SF_ENTRY_C(struct, edog);
    SF_ENTRY_C(struct, egd);
    SF_ENTRY_C(struct, emin);
    SF_ENTRY_C(struct, engr);
    SF_ENTRY_C(struct, epri);
    SF_ENTRY_C(struct, eshk);
    SF_ENTRY_C(struct, fe);
    SF_ENTRY_C(struct, flag);
    SF_ENTRY_C(struct, fruit);
    SF_ENTRY_C(struct, gamelog_line);
    SF_ENTRY_C(struct, kinfo);
    SF_ENTRY_C(struct, levelflags);
    SF_ENTRY_C(struct, ls_t);
    SF_ENTRY_C(struct, linfo);
    SF_ENTRY_C(struct, mapseen_feat);
    SF_ENTRY_C(struct, mapseen_flags);
    SF_ENTRY_C(struct, mapseen_rooms);
    SF_ENTRY_C(struct, mkroom);
    SF_ENTRY_C(struct, monst);
    SF_ENTRY_C(struct, mvitals);
    SF_ENTRY_C(struct, obj);
    SF_ENTRY_C(struct, objclass);
    SF_ENTRY_C(struct, q_score);
    SF_ENTRY_C(struct, rm);
    SF_ENTRY_C(struct, spell);
    SF_ENTRY_C(struct, stairway);
    SF_ENTRY_C(struct, s_level);
    SF_ENTRY_C(struct, trap);
    SF_ENTRY_C(struct, version_info);
    SF_ENTRY_C(struct, you);
    SF_ENTRY_C(union, any);
#ifdef DEMO_UPLIFTS
    SF_ENTRY_C(struct, mystruct);
    SF_ENTRY_C(struct, mystruct_rev0);
#endif

    SF_ENTRY(aligntyp);
    SF_ENTRY(boolean);
    SF_ENTRY(coordxy);
    SF_ENTRY(genericptr);
    SF_ENTRY(int);
    SF_ENTRY(int16);
    SF_ENTRY(int32);
    SF_ENTRY(int64);
    SF_ENTRY(long);
    SF_ENTRY(schar);
    SF_ENTRY(short);
    SF_ENTRY(size_t);
    SF_ENTRY(time_t);
    SF_ENTRY(uchar);
    SF_ENTRY(uint16);
    SF_ENTRY(uint32);
    SF_ENTRY(uint64);
    SF_ENTRY(ulong);
    SF_ENTRY(unsigned);
    SF_ENTRY(ushort);
    SF_ENTRY(xint16);
    SF_ENTRY(xint8);
    SF_ENTRY_X(char, char);
    SF_ENTRY_X(uint8_t, bitfield);
};

#undef SF_ENTRY
#undef SF_ENTRY_C
#undef SF_ENTRY_X

struct sf_structlevel_procs {
    const char *ext;
    struct sf_procs fn;    /* called for structlevel (historical) */
};
struct sf_fieldlevel_procs {
    const char *ext;
    struct sf_procs fn_x; /* called for fieldlevel */
};
extern struct sf_structlevel_procs sfoprocs[NUM_SAVEFORMATS], sfiprocs[NUM_SAVEFORMATS];
extern void sf_setprocs(int, struct sf_structlevel_procs *, struct sf_structlevel_procs *);
extern void sf_setflprocs(int, struct sf_fieldlevel_procs *, struct sf_fieldlevel_procs *);
extern struct sf_structlevel_procs sfoprocs[NUM_SAVEFORMATS], sfiprocs[NUM_SAVEFORMATS];
extern struct sf_fieldlevel_procs sfoflprocs[NUM_SAVEFORMATS], sfiflprocs[NUM_SAVEFORMATS];
extern struct sf_structlevel_procs historical_sfo_procs;
extern struct sf_structlevel_procs historical_sfi_procs;
extern struct sf_fieldlevel_procs exportascii_sfo_procs;
extern struct sf_fieldlevel_procs exportascii_sfi_procs;

#endif /* SFPROCS_H */

