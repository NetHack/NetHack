/* NetHack 5.0	savefile.h	$NHDT-Date: 1738638877 2025/02/03 19:14:37 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.1476 $ */
/* Copyright (c) Michael Allison, 2025.                           */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef SAVEFILE_H
#define SAVEFILE_H

/* #define SAVEFILE_DEBUGGING */

extern void sf_init(void);
/* sfbase.c output functions */
extern void sfo_aligntyp(NHFILE *, aligntyp *, const char *);
extern void sfo_any(NHFILE *, anything *, const char *);
extern void sfo_boolean(NHFILE *, boolean *, const char *);
extern void sfo_char(NHFILE *, char *, const char *, int);
extern void sfo_genericptr(NHFILE *, genericptr_t *, const char *);
extern void sfo_int16(NHFILE *, int16 *, const char *);
extern void sfo_int32(NHFILE *, int32 *, const char *);
extern void sfo_int64(NHFILE *, int64 *, const char *);
extern void sfo_uchar(NHFILE *, uchar *, const char *);
extern void sfo_uint16(NHFILE *, uint16 *, const char *);
extern void sfo_uint32(NHFILE *, uint32 *, const char *);
extern void sfo_uint64(NHFILE *, uint64 *, const char *);
extern void sfo_size_t(NHFILE *, size_t *, const char *);
extern void sfo_time_t(NHFILE *, time_t *, const char *);
//extern void sfo_str(NHFILE *, char *, const char *, int);
extern void sfo_arti_info(NHFILE *nhfp,
                                  struct arti_info *d_arti_info,
                                  const char *myname);
extern void sfo_dgn_topology(NHFILE *nhfp,
                                  struct dgn_topology *d_dgn_topology,
                                  const char *myname);
extern void sfo_dungeon(NHFILE *nhfp, struct dungeon *d_dungeon,
                             const char *myname);
extern void sfo_branch(NHFILE *nhfp, struct branch *d_branch,
                            const char *myname);
extern void sfo_linfo(NHFILE *nhfp, struct linfo *d_linfo,
                           const char *myname);
extern void sfo_nhcoord(NHFILE *nhfp, struct nhcoord *d_nhcoord,
                             const char *myname);
extern void sfo_d_level(NHFILE *nhfp, struct d_level *d_d_level,
                           const char *myname);
extern void sfo_mapseen_feat(NHFILE *nhfp,
                                  struct mapseen_feat *d_mapseen_feat,
                                  const char *myname);
extern void sfo_mapseen_flags(NHFILE *nhfp,
                                   struct mapseen_flags *d_mapseen_flags,
                                   const char *myname);
extern void sfo_mapseen_rooms(NHFILE *nhfp,
                                   struct mapseen_rooms *d_mapseen_rooms,
                                   const char *myname);
extern void sfo_kinfo(NHFILE *nhfp,
                           struct kinfo *d_kinfo,
                           const char *myname);
extern void sfo_engr(NHFILE *, struct engr *, const char *);
extern void sfo_ls_t(NHFILE *, struct ls_t *, const char *);
extern void sfo_bubble(NHFILE *, struct bubble *, const char *);
extern void sfo_mkroom(NHFILE *, struct mkroom *, const char *);
extern void sfo_objclass(NHFILE *, struct objclass *, const char *);
extern void sfo_nhrect(NHFILE *, struct nhrect *, const char *);
extern void sfo_fe(NHFILE *, struct fe *, const char *);
extern void sfo_version_info(NHFILE *, struct version_info *,
                                  const char *);
extern void sfo_context_info(NHFILE *, struct context_info *,
                                  const char *);
extern void sfo_flag(NHFILE *, struct flag *, const char *);
extern void sfo_you(NHFILE *, struct you *, const char *);
extern void sfo_mvitals(NHFILE *, struct mvitals *, const char *);
extern void sfo_q_score(NHFILE *, struct q_score *, const char *);
extern void sfo_spell(NHFILE *, struct spell *, const char *);
extern void sfo_dest_area(NHFILE *, struct dest_area *, const char *);
extern void sfo_levelflags(NHFILE *, struct levelflags *, const char *);
extern void sfo_rm(NHFILE *, struct rm *, const char *);
extern void sfo_cemetery(NHFILE *, struct cemetery *, const char *);
extern void sfo_damage(NHFILE *, struct damage *, const char *);
extern void sfo_stairway(NHFILE *, struct stairway *, const char *);
extern void sfo_obj(NHFILE *, struct obj *, const char *);
extern void sfo_monst(NHFILE *, struct monst *, const char *);
extern void sfo_ebones(NHFILE *, struct ebones *, const char *);
extern void sfo_edog(NHFILE *, struct edog *, const char *);
extern void sfo_egd(NHFILE *, struct egd *, const char *);
extern void sfo_emin(NHFILE *, struct emin *, const char *);
extern void sfo_engr(NHFILE *, struct engr *, const char *);
extern void sfo_epri(NHFILE *, struct epri *, const char *);
extern void sfo_eshk(NHFILE *, struct eshk *, const char *);
extern void sfo_trap(NHFILE *, struct trap *, const char *);
extern void sfo_gamelog_line(NHFILE *, struct gamelog_line *, const char *);
extern void sfo_fruit(NHFILE *, struct fruit *, const char *);
extern void sfo_s_level(NHFILE *, struct s_level *, const char *);
extern void sfo_xint8(NHFILE *, xint8 *, const char *);
extern void sfo_xint16(NHFILE *, xint16 *, const char *);
extern void sfo_schar(NHFILE *, schar *, const char *);
extern void sfo_short(NHFILE *, short *, const char *);
extern void sfo_ushort(NHFILE *, ushort *, const char *);
extern void sfo_int(NHFILE *, int *, const char *);
extern void sfo_unsigned(NHFILE *, unsigned *, const char *);
extern void sfo_long(NHFILE *, long *, const char *);
extern void sfo_ulong(NHFILE *, ulong *, const char *);
/* sfbase.c input functions */
extern void sfi_addinfo(NHFILE *, const char *, const char *);
extern void sfi_aligntyp(NHFILE *, aligntyp *, const char *);
extern void sfi_any(NHFILE *, anything *, const char *);
extern void sfi_boolean(NHFILE *, boolean *, const char *);
extern void sfi_genericptr(NHFILE *, genericptr_t *, const char *);
extern void sfi_char(NHFILE *, char *, const char *, int);
extern void sfi_int16(NHFILE *, int16 *, const char *);
extern void sfi_int32(NHFILE *, int32 *, const char *);
extern void sfi_int64(NHFILE *, int64 *, const char *);
extern void sfi_uchar(NHFILE *, uchar *, const char *);
extern void sfi_uint16(NHFILE *, uint16 *, const char *);
extern void sfi_uint32(NHFILE *, uint32 *, const char *);
extern void sfi_uint64(NHFILE *, uint64 *, const char *);
extern void sfi_size_t(NHFILE *, size_t *, const char *);
extern void sfi_time_t(NHFILE *, time_t *, const char *);
extern void sfi_arti_info(NHFILE *nhfp,
                                  struct arti_info *d_arti_info,
                                  const char *myname);
extern void sfi_dungeon(NHFILE *nhfp, struct dungeon *d_dungeon,
                             const char *myname);
extern void sfi_dgn_topology(NHFILE *nhfp,
                                  struct dgn_topology *d_dgn_topology,
                                  const char *myname);
extern void sfi_branch(NHFILE *nhfp, struct branch *d_branch,
                            const char *myname);
extern void sfi_linfo(NHFILE *nhfp, struct linfo *d_linfo,
                         const char *myname);
extern void sfi_nhcoord(NHFILE *nhfp, struct nhcoord *d_nhcoord,
                           const char *myname);
extern void sfi_d_level(NHFILE *nhfp, struct d_level *d_d_level,
                             const char *myname);
extern void sfi_mapseen_feat(NHFILE *nhfp,
                                  struct mapseen_feat *d_mapseen_feat,
                                  const char *myname);
extern void sfi_mapseen_flags(NHFILE *nhfp,
                                   struct mapseen_flags *d_mapseen_flags,
                                   const char *myname);
extern void sfi_mapseen_rooms(NHFILE *nhfp,
                                   struct mapseen_rooms *d_mapseen_rooms,
                                   const char *myname);
extern void sfi_kinfo(NHFILE *nhfp,
                           struct kinfo *d_kinfo,
                           const char *myname);
extern void sfi_engr(NHFILE *, struct engr *, const char *);
extern void sfi_ls_t(NHFILE *, struct ls_t *, const char *);
extern void sfi_bubble(NHFILE *, struct bubble *, const char *);
extern void sfi_mkroom(NHFILE *, struct mkroom *, const char *);
extern void sfi_objclass(NHFILE *, struct objclass *, const char *);
extern void sfi_nhrect(NHFILE *, struct nhrect *, const char *);
extern void sfi_fe(NHFILE *, struct fe *, const char *);
extern void sfi_version_info(NHFILE *, struct version_info *,
                                  const char *);
extern void sfi_context_info(NHFILE *, struct context_info *,
                                  const char *);
extern void sfi_flag(NHFILE *, struct flag *, const char *);
extern void sfi_you(NHFILE *, struct you *, const char *);
extern void sfi_mvitals(NHFILE *, struct mvitals *, const char *);
extern void sfi_q_score(NHFILE *, struct q_score *, const char *);
extern void sfi_spell(NHFILE *, struct spell *, const char *);
extern void sfi_dest_area(NHFILE *, struct dest_area *, const char *);
extern void sfi_levelflags(NHFILE *, struct levelflags *, const char *);
extern void sfi_rm(NHFILE *, struct rm *, const char *);
extern void sfi_cemetery(NHFILE *, struct cemetery *, const char *);
extern void sfi_damage(NHFILE *, struct damage *, const char *);
extern void sfi_stairway(NHFILE *, struct stairway *, const char *);
extern void sfi_obj(NHFILE *, struct obj *, const char *);
extern void sfi_monst(NHFILE *, struct monst *, const char *);
extern void sfi_ebones(NHFILE *, struct ebones *, const char *);
extern void sfi_edog(NHFILE *, struct edog *, const char *);
extern void sfi_egd(NHFILE *, struct egd *, const char *);
extern void sfi_emin(NHFILE *, struct emin *, const char *);
extern void sfi_engr(NHFILE *, struct engr *, const char *);
extern void sfi_epri(NHFILE *, struct epri *, const char *);
extern void sfi_eshk(NHFILE *, struct eshk *, const char *);
extern void sfi_trap(NHFILE *, struct trap *, const char *);
extern void sfi_fruit(NHFILE *, struct fruit *, const char *);
extern void sfi_gamelog_line(NHFILE *, struct gamelog_line *, const char *);
extern void sfi_s_level(NHFILE *, struct s_level *, const char *);
extern void sfi_xint8(NHFILE *, xint8 *, const char *);
extern void sfi_xint16(NHFILE *, xint16 *, const char *);
extern void sfi_schar(NHFILE *, schar *, const char *);
extern void sfi_short(NHFILE *, short *, const char *);
extern void sfi_ushort(NHFILE *, ushort *, const char *);
extern void sfi_int(NHFILE *, int *, const char *);
extern void sfi_unsigned(NHFILE *, unsigned *, const char *);
extern void sfi_long(NHFILE *, long *, const char *);
extern void sfi_ulong(NHFILE *, ulong *, const char *);
#if NH_C < 202300L
#define Sfo_aligntyp(a,b,c) sfo_aligntyp(a, b, c)
#define Sfo_any(a,b,c) sfo_any(a, b, c)
#define Sfo_genericptr(a,b,c) sfo_genericptr(a, b, c)
#define Sfo_coordxy(a,b,c) sfo_int16(a, b, c)
#define Sfo_char(a,b,c,d) sfo_char(a, b, c, d)
#define Sfo_int16(a,b,c) sfo_int16(a, b, c)
#define Sfo_int32(a,b,c) sfo_int32(a, b, c)
#define Sfo_int64(a,b,c) sfo_int64(a, b, c)
#define Sfo_uchar(a,b,c) sfo_uchar(a, b, c)
#define Sfo_uint16(a,b,c) sfo_uint16(a, b, c)
#define Sfo_uint32(a,b,c) sfo_uint32(a, b, c)
#define Sfo_uint64(a,b,c) sfo_uint64(a, b, c)
#define Sfo_size_t(a,b,c) sfo_size_t(a, b, c)
#define Sfo_time_t(a,b,c) sfo_time_t(a, b, c)
#define Sfo_str(a,b,c) sfo_str(a, b, c)
#define Sfo_arti_info(a,b,c) sfo_arti_info(a, b, c)
#define Sfo_dgn_topology(a,b,c) sfo_dgn_topology(a, b, c)
#define Sfo_dungeon(a,b,c) sfo_dungeon(a, b, c)
#define Sfo_branch(a,b,c) sfo_branch(a, b, c)
#define Sfo_linfo(a,b,c) sfo_linfo(a, b, c)
#define Sfo_nhcoord(a,b,c) sfo_nhcoord(a, b, c)
#define Sfo_d_level(a,b,c) sfo_d_level(a, b, c)
#define Sfo_mapseen_feat(a,b,c) sfo_mapseen_feat(a, b, c)
#define Sfo_mapseen_flags(a,b,c) sfo_mapseen_flags(a, b, c)
#define Sfo_mapseen_rooms(a,b,c) sfo_mapseen_rooms(a, b, c)
#define Sfo_kinfo(a,b,c) sfo_kinfo(a, b, c)
#define Sfo_engr(a,b,c) sfo_engr(a, b, c)
#define Sfo_ls_t(a,b,c) sfo_ls_t(a, b, c)
#define Sfo_bubble(a,b,c) sfo_bubble(a, b, c)
#define Sfo_mkroom(a,b,c) sfo_mkroom(a, b, c)
#define Sfo_objclass(a,b,c) sfo_objclass(a, b, c)
#define Sfo_nhrect(a,b,c) sfo_nhrect(a, b, c)
#define Sfo_fe(a,b,c) sfo_fe(a, b, c)
#define Sfo_version_info(a,b,c) sfo_version_info(a, b, c)
#define Sfo_context_info(a,b,c) sfo_context_info(a, b, c)
#define Sfo_flag(a,b,c) sfo_flag(a, b, c)
#define Sfo_you(a,b,c) sfo_you(a, b, c)
#define Sfo_mvitals(a,b,c) sfo_mvitals(a, b, c)
#define Sfo_q_score(a,b,c) sfo_q_score(a, b, c)
#define Sfo_spell(a,b,c) sfo_spell(a, b, c)
#define Sfo_dest_area(a,b,c) sfo_dest_area(a, b, c)
#define Sfo_levelflags(a,b,c) sfo_levelflags(a, b, c)
#define Sfo_rm(a,b,c) sfo_rm(a, b, c)
#define Sfo_cemetery(a,b,c) sfo_cemetery(a, b, c)
#define Sfo_damage(a,b,c) sfo_damage(a, b, c)
#define Sfo_stairway(a,b,c) sfo_stairway(a, b, c)
#define Sfo_obj(a,b,c) sfo_obj(a, b, c)
#define Sfo_monst(a,b,c) sfo_monst(a, b, c)
#define Sfo_ebones(a,b,c) sfo_ebones(a, b, c)
#define Sfo_edog(a,b,c) sfo_edog(a, b, c)
#define Sfo_egd(a,b,c) sfo_egd(a, b, c)
#define Sfo_emin(a,b,c) sfo_emin(a, b, c)
#define Sfo_engr(a,b,c) sfo_engr(a, b, c)
#define Sfo_epri(a,b,c) sfo_epri(a, b, c)
#define Sfo_eshk(a,b,c) sfo_eshk(a, b, c)
#define Sfo_trap(a,b,c) sfo_trap(a, b, c)
#define Sfo_gamelog_line(a,b,c) sfo_gamelog_line(a, b, c)
#define Sfo_fruit(a,b,c) sfo_fruit(a, b, c)
#define Sfo_s_level(a,b,c) sfo_s_level(a, b, c)
#define Sfo_short(a, b, c) sfo_short(a, b, c)
#define Sfo_ushort(a, b, c) sfo_ushort(a, b, c)
#define Sfo_int(a, b, c) sfo_int(a, b, c)
#define Sfo_unsigned(a, b, c) sfo_unsigned(a, b, c)
#define Sfo_xint8(a, b, c) sfo_xint8(a, b, c);
#define Sfo_xint16(a, b, c) sfo_xint16(a, b, c)
/* sfbase.c input functions */
#define Sfi_addinfo(a,b,c) sfi_addinfo(a, b, c)
#define Sfi_aligntyp(a,b,c) sfi_aligntyp(a, b, c)
#define Sfi_any(a,b,c) sfi_any(a, b, c)
#define Sfi_genericptr(a,b,c) sfi_genericptr(a, b, c)
#define Sfi_coordxy(a,b,c) sfi_int16(a, b, c)
#define Sfi_int16(a,b,c) sfi_int16(a, b, c)
#define Sfi_int32(a,b,c) sfi_int32(a, b, c)
#define Sfi_int64(a,b,c) sfi_int64(a, b, c)
#define Sfi_uchar(a,b,c) sfi_uchar(a, b, c)
#define Sfi_uint16(a,b,c) sfi_uint16(a, b, c)
#define Sfi_uint32(a,b,c) sfi_uint32(a, b, c)
#define Sfi_uint64(a,b,c) sfi_uint64(a, b, c)
#define Sfi_size_t(a,b,c) sfi_size_t(a, b, c)
#define Sfi_time_t(a,b,c) sfi_time_t(a, b, c)
#define Sfi_arti_info(a,b,c) sfi_arti_info(a, b, c)
#define Sfi_dungeon(a,b,c) sfi_dungeon(a, b, c)
#define Sfi_dgn_topology(a,b,c) sfi_dgn_topology(a, b, c)
#define Sfi_branch(a,b,c) sfi_branch(a, b, c)
#define Sfi_linfo(a,b,c) sfi_linfo(a, b, c)
#define Sfi_nhcoord(a,b,c) sfi_nhcoord(a, b, c)
#define Sfi_d_level(a,b,c) sfi_d_level(a, b, c)
#define Sfi_mapseen_feat(a,b,c) sfi_mapseen_feat(a, b, c)
#define Sfi_mapseen_flags(a,b,c) sfi_mapseen_flags(a, b, c)
#define Sfi_mapseen_rooms(a,b,c) sfi_mapseen_rooms(a, b, c)
#define Sfi_kinfo(a,b,c) sfi_kinfo(a, b, c)
#define Sfi_engr(a,b,c) sfi_engr(a, b, c)
#define Sfi_ls_t(a,b,c) sfi_ls_t(a, b, c)
#define Sfi_bubble(a,b,c) sfi_bubble(a, b, c)
#define Sfi_mkroom(a,b,c) sfi_mkroom(a, b, c)
#define Sfi_objclass(a,b,c) sfi_objclass(a, b, c)
#define Sfi_nhrect(a,b,c) sfi_nhrect(a, b, c)
#define Sfi_fe(a,b,c) sfi_fe(a, b, c)
#define Sfi_version_info(a,b,c) sfi_version_info(a, b, c)
#define Sfi_context_info(a,b,c) sfi_context_info(a, b, c)
#define Sfi_flag(a,b,c) sfi_flag(a, b, c)
#define Sfi_you(a,b,c) sfi_you(a, b, c)
#define Sfi_mvitals(a,b,c) sfi_mvitals(a, b, c)
#define Sfi_q_score(a,b,c) sfi_q_score(a, b, c)
#define Sfi_spell(a,b,c) sfi_spell(a, b, c)
#define Sfi_dest_area(a,b,c) sfi_dest_area(a, b, c)
#define Sfi_levelflags(a,b,c) sfi_levelflags(a, b, c)
#define Sfi_rm(a,b,c) sfi_rm(a, b, c)
#define Sfi_cemetery(a,b,c) sfi_cemetery(a, b, c)
#define Sfi_damage(a,b,c) sfi_damage(a, b, c)
#define Sfi_stairway(a,b,c) sfi_stairway(a, b, c)
#define Sfi_obj(a,b,c) sfi_obj(a, b, c)
#define Sfi_monst(a,b,c) sfi_monst(a, b, c)
#define Sfi_ebones(a,b,c) sfi_ebones(a, b, c)
#define Sfi_edog(a,b,c) sfi_edog(a, b, c)
#define Sfi_egd(a,b,c) sfi_egd(a, b, c)
#define Sfi_emin(a,b,c) sfi_emin(a, b, c)
#define Sfi_engr(a,b,c) sfi_engr(a, b, c)
#define Sfi_epri(a,b,c) sfi_epri(a, b, c)
#define Sfi_eshk(a,b,c) sfi_eshk(a, b, c)
#define Sfi_trap(a,b,c) sfi_trap(a, b, c)
#define Sfi_fruit(a,b,c) sfi_fruit(a, b, c)
#define Sfi_gamelog_line(a,b,c) sfi_gamelog_line(a, b, c)
#define Sfi_s_level(a,b,c) sfi_s_level(a, b, c)
#define Sfi_short(a, b, c) sfi_short(a, b, c)
#define Sfi_ushort(a, b, c) sfi_ushort(a, b, c)
#define Sfi_int(a, b, c) sfi_int(a, b, c);
#define Sfi_unsigned(a, b, c) sfi_unsigned(a, b, c);
#define Sfi_xint8(a, b, c) sfi_xint8(a, b, c);
#define Sfi_xint16(a, b, c) sfi_xint16(a, b, c);
#else

#define sfo(nhfp, dt, tag)                     \
  _Generic( (dt),                              \
    anything *            : sfo_any,           \
    int16_t *             : sfo_int16,         \
    int32_t *             : sfo_int32,         \
    int64_t *             : sfo_int64,         \
    uchar *               : sfo_uchar,         \
    uint16_t *            : sfo_uint16,        \
    uint32_t *            : sfo_uint32,        \
    uint64_t *            : sfo_uint64,        \
    xint8 *               : sfo_xint8,         \
    struct arti_info *    : sfo_arti_info,     \
    struct nhrect *       : sfo_nhrect,        \
    struct branch *       : sfo_branch,        \
    struct bubble *       : sfo_bubble,        \
    struct cemetery *     : sfo_cemetery,      \
    struct context_info * : sfo_context_info,  \
    coord *               : sfo_nhcoord,       \
    struct damage *       : sfo_damage,        \
    struct dgn_topology * : sfo_dgn_topology,  \
    dungeon *             : sfo_dungeon,       \
    d_level *             : sfo_d_level,       \
    struct levelflags *   : sfo_levelflags,    \
    light_source *        : sfo_ls_t,          \
    struct dest_area *    : sfo_dest_area,     \
    struct ebones *       : sfo_ebones,        \
    struct edog *         : sfo_edog,          \
    struct egd *          : sfo_egd,           \
    struct emin *         : sfo_emin,          \
    struct engr *         : sfo_engr,          \
    struct epri *         : sfo_epri,          \
    struct eshk *         : sfo_eshk,          \
    struct fe *           : sfo_fe,            \
    struct flag *         : sfo_flag,          \
    struct fruit *        : sfo_fruit,         \
    struct gamelog_line * : sfo_gamelog_line,  \
    struct kinfo *        : sfo_kinfo,         \
    struct linfo *        : sfo_linfo,         \
    struct mapseen_feat * : sfo_mapseen_feat,  \
    struct mapseen_flags *: sfo_mapseen_flags, \
    struct mapseen_rooms *: sfo_mapseen_rooms, \
    struct mkroom *       : sfo_mkroom,        \
    struct monst *        : sfo_monst,         \
    struct mvitals *      : sfo_mvitals,       \
    struct obj *          : sfo_obj,           \
    struct objclass *     : sfo_objclass,      \
    struct q_score *      : sfo_q_score,       \
    struct rm *           : sfo_rm,            \
    struct spell *        : sfo_spell,         \
    struct stairway *     : sfo_stairway,      \
    struct s_level *      : sfo_s_level,       \
    struct trap *         : sfo_trap,          \
    struct version_info * : sfo_version_info,  \
    struct you *          : sfo_you            \
  ) (nhfp, dt, tag)

/*    struct container * : sfo_container, */
/*    struct mapseen *   : sfo_mapseen,   */
/*    struct mextra *    : sfo_mextra,    */
/*    struct oextra *    : sfo_oextra,    */
/*    struct permonst *  : sfo_permonst,  */

#define sfi(nhfp, dt, tag)                     \
  _Generic( (dt),                              \
    anything *            : sfi_any,           \
    int16_t *             : sfi_int16,         \
    int32_t *             : sfi_int32,         \
    int64_t *             : sfi_int64,         \
    uchar *               : sfi_uchar,         \
    uint16_t *            : sfi_uint16,        \
    uint32_t *            : sfi_uint32,        \
    uint64_t *            : sfi_uint64,        \
    xint8 *               : sfi_xint8,         \
    struct arti_info *    : sfi_arti_info,     \
    struct nhrect *       : sfi_nhrect,        \
    struct branch *       : sfi_branch,        \
    struct bubble *       : sfi_bubble,        \
    struct cemetery *     : sfi_cemetery,      \
    struct context_info * : sfi_context_info,  \
    coord *               : sfi_nhcoord,       \
    struct damage *       : sfi_damage,        \
    struct dgn_topology * : sfi_dgn_topology,  \
    dungeon *             : sfi_dungeon,       \
    d_level *             : sfi_d_level,       \
    struct levelflags *   : sfi_levelflags,    \
    light_source *        : sfi_ls_t,          \
    struct dest_area *    : sfi_dest_area,     \
    struct ebones *       : sfi_ebones,        \
    struct edog *         : sfi_edog,          \
    struct egd *          : sfi_egd,           \
    struct emin *         : sfi_emin,          \
    struct engr *         : sfi_engr,          \
    struct epri *         : sfi_epri,          \
    struct eshk *         : sfi_eshk,          \
    struct fe *           : sfi_fe,            \
    struct flag *         : sfi_flag,          \
    struct fruit *        : sfi_fruit,         \
    struct gamelog_line * : sfi_gamelog_line,  \
    struct kinfo *        : sfi_kinfo,         \
    struct linfo *        : sfi_linfo,         \
    struct mapseen_feat * : sfi_mapseen_feat,  \
    struct mapseen_flags *: sfi_mapseen_flags, \
    struct mapseen_rooms *: sfi_mapseen_rooms, \
    struct mkroom *       : sfi_mkroom,        \
    struct monst *        : sfi_monst,         \
    struct mvitals *      : sfi_mvitals,       \
    struct obj *          : sfi_obj,           \
    struct objclass *     : sfi_objclass,      \
    struct q_score *      : sfi_q_score,       \
    struct rm *           : sfi_rm,            \
    struct spell *        : sfi_spell,         \
    struct stairway *     : sfi_stairway,      \
    struct s_level *      : sfi_s_level,       \
    struct trap *         : sfi_trap,          \
    struct version_info * : sfi_version_info,  \
    struct you *          : sfi_you            \
  ) (nhfp, dt, tag)

/*    char *                : sfo_char,   */
/*    char *                : sfi_char,   */
/*    struct container * : sfi_container, */
/*    struct mapseen *   : sfi_mapseen,   */
/*    struct mextra *    : sfi_mextra,    */
/*    struct oextra *    : sfi_oextra,    */
/*    struct permonst *  : sfi_permonst,  */

#define Sfo_any(a,b,c) sfo(a, b, c)
#define Sfo_aligntyp(a,b,c) sfo(a, b, c)
#define Sfo_genericptr(a,b,c) sfo(a, b, c)
#define Sfo_coordxy(a,b,c) sfo(a, b, c)
#define Sfo_int16(a,b,c) sfo(a, b, c)
#define Sfo_int32(a,b,c) sfo(a, b, c)
#define Sfo_int64(a,b,c) sfo(a, b, c)
#define Sfo_uchar(a,b,c) sfo(a, b, c)
#define Sfo_unsigned(a,b,c) sfo(a, b, c)
#define Sfo_uchar(a,b,c) sfo(a, b, c)
#define Sfo_uint16(a,b,c) sfo(a, b, c)
#define Sfo_uint32(a,b,c) sfo(a, b, c)
#define Sfo_uint64(a,b,c) sfo(a, b, c)
#define Sfo_size_t(a,b,c) sfo(a, b, c)
#define Sfo_time_t(a,b,c) sfo(a, b, c)
#define Sfo_str(a,b,c) sfo(a, b, c)
#define Sfo_arti_info(a,b,c) sfo(a, b, c)
#define Sfo_dgn_topology(a,b,c) sfo(a, b, c)
#define Sfo_dungeon(a,b,c) sfo(a, b, c)
#define Sfo_branch(a,b,c) sfo(a, b, c)
#define Sfo_linfo(a,b,c) sfo(a, b, c)
#define Sfo_nhcoord(a,b,c) sfo(a, b, c)
#define Sfo_d_level(a,b,c) sfo(a, b, c)
#define Sfo_mapseen_feat(a,b,c) sfo(a, b, c)
#define Sfo_mapseen_flags(a,b,c) sfo(a, b, c)
#define Sfo_mapseen_rooms(a,b,c) sfo(a, b, c)
#define Sfo_kinfo(a,b,c) sfo(a, b, c)
#define Sfo_engr(a,b,c) sfo(a, b, c)
#define Sfo_ls_t(a,b,c) sfo(a, b, c)
#define Sfo_bubble(a,b,c) sfo(a, b, c)
#define Sfo_mkroom(a,b,c) sfo(a, b, c)
#define Sfo_objclass(a,b,c) sfo(a, b, c)
#define Sfo_nhrect(a,b,c) sfo(a, b, c)
#define Sfo_fe(a,b,c) sfo(a, b, c)
#define Sfo_version_info(a,b,c) sfo(a, b, c)
#define Sfo_context_info(a,b,c) sfo(a, b, c)
#define Sfo_flag(a,b,c) sfo(a, b, c)
#define Sfo_you(a,b,c) sfo(a, b, c)
#define Sfo_mvitals(a,b,c) sfo(a, b, c)
#define Sfo_q_score(a,b,c) sfo(a, b, c)
#define Sfo_spell(a,b,c) sfo(a, b, c)
#define Sfo_dest_area(a,b,c) sfo(a, b, c)
#define Sfo_levelflags(a,b,c) sfo(a, b, c)
#define Sfo_rm(a,b,c) sfo(a, b, c)
#define Sfo_cemetery(a,b,c) sfo(a, b, c)
#define Sfo_damage(a,b,c) sfo(a, b, c)
#define Sfo_stairway(a,b,c) sfo(a, b, c)
#define Sfo_obj(a,b,c) sfo(a, b, c)
#define Sfo_monst(a,b,c) sfo(a, b, c)
#define Sfo_ebones(a,b,c) sfo(a, b, c)
#define Sfo_edog(a,b,c) sfo(a, b, c)
#define Sfo_egd(a,b,c) sfo(a, b, c)
#define Sfo_emin(a,b,c) sfo(a, b, c)
#define Sfo_engr(a,b,c) sfo(a, b, c)
#define Sfo_epri(a,b,c) sfo(a, b, c)
#define Sfo_eshk(a,b,c) sfo(a, b, c)
#define Sfo_trap(a,b,c) sfo(a, b, c)
#define Sfo_gamelog_line(a,b,c) sfo(a, b, c)
#define Sfo_fruit(a,b,c) sfo(a, b, c)
#define Sfo_s_level(a,b,c) sfo(a, b, c)
#define Sfo_short(a, b, c) sfo(a, b, c)
#define Sfo_ushort(a, b, c) sfo(a, b, c)
#define Sfo_int(a, b, c) sfo(a, b, c)
#define Sfo_unsigned(a, b, c) sfo(a, b, c)
#define Sfo_xint8(a, b, c) sfo(a, b, c)
#define Sfo_xint16(a, b, c) sfo(a, b, c)

/* sfbase.c input functions */
#define Sfi_addinfo(a,b,c) sfi(a, b, c)
#define Sfi_aligntyp(a,b,c) sfi(a, b, c)
#define Sfi_any(a,b,c) sfi(a, b, c)
#define Sfi_genericptr(a,b,c) sfi(a, b, c)
#define Sfi_coordxy(a,b,c) sfi(a, b, c)
#define Sfi_int16(a,b,c) sfi(a, b, c)
#define Sfi_int32(a,b,c) sfi(a, b, c)
#define Sfi_int64(a,b,c) sfi(a, b, c)
#define Sfi_uchar(a,b,c) sfi(a, b, c)
#define Sfi_uint16(a,b,c) sfi(a, b, c)
#define Sfi_uint32(a,b,c) sfi(a, b, c)
#define Sfi_uint64(a,b,c) sfi(a, b, c)
#define Sfi_size_t(a,b,c) sfi(a, b, c)
#define Sfi_time_t(a,b,c) sfi(a, b, c)
#define Sfi_arti_info(a,b,c) sfi(a, b, c)
#define Sfi_dungeon(a,b,c) sfi(a, b, c)
#define Sfi_dgn_topology(a,b,c) sfi(a, b, c)
#define Sfi_branch(a,b,c) sfi(a, b, c)
#define Sfi_linfo(a,b,c) sfi(a, b, c)
#define Sfi_nhcoord(a,b,c) sfi(a, b, c)
#define Sfi_d_level(a,b,c) sfi(a, b, c)
#define Sfi_mapseen_feat(a,b,c) sfi(a, b, c)
#define Sfi_mapseen_flags(a,b,c) sfi(a, b, c)
#define Sfi_mapseen_rooms(a,b,c) sfi(a, b, c)
#define Sfi_kinfo(a,b,c) sfi(a, b, c)
#define Sfi_engr(a,b,c) sfi(a, b, c)
#define Sfi_ls_t(a,b,c) sfi(a, b, c)
#define Sfi_bubble(a,b,c) sfi(a, b, c)
#define Sfi_mkroom(a,b,c) sfi(a, b, c)
#define Sfi_objclass(a,b,c) sfi(a, b, c)
#define Sfi_nhrect(a,b,c) sfi(a, b, c)
#define Sfi_fe(a,b,c) sfi(a, b, c)
#define Sfi_version_info(a,b,c) sfi(a, b, c)
#define Sfi_context_info(a,b,c) sfi(a, b, c)
#define Sfi_flag(a,b,c) sfi(a, b, c)
#define Sfi_you(a,b,c) sfi(a, b, c)
#define Sfi_mvitals(a,b,c) sfi(a, b, c)
#define Sfi_q_score(a,b,c) sfi(a, b, c)
#define Sfi_spell(a,b,c) sfi(a, b, c)
#define Sfi_dest_area(a,b,c) sfi(a, b, c)
#define Sfi_levelflags(a,b,c) sfi(a, b, c)
#define Sfi_rm(a,b,c) sfi(a, b, c)
#define Sfi_cemetery(a,b,c) sfi(a, b, c)
#define Sfi_damage(a,b,c) sfi(a, b, c)
#define Sfi_stairway(a,b,c) sfi(a, b, c)
#define Sfi_obj(a,b,c) sfi(a, b, c)
#define Sfi_monst(a,b,c) sfi(a, b, c)
#define Sfi_ebones(a,b,c) sfi(a, b, c)
#define Sfi_edog(a,b,c) sfi(a, b, c)
#define Sfi_egd(a,b,c) sfi(a, b, c)
#define Sfi_emin(a,b,c) sfi(a, b, c)
#define Sfi_engr(a,b,c) sfi(a, b, c)
#define Sfi_epri(a,b,c) sfi(a, b, c)
#define Sfi_eshk(a,b,c) sfi(a, b, c)
#define Sfi_trap(a,b,c) sfi(a, b, c)
#define Sfi_fruit(a,b,c) sfi(a, b, c)
#define Sfi_gamelog_line(a,b,c) sfi(a, b, c)
#define Sfi_s_level(a,b,c) sfi(a, b, c)
#define Sfi_short(a, b, c) sfi(a, b, c)
#define Sfi_ushort(a, b, c) sfi(a, b, c)
#define Sfi_int(a,b,c) sfi(a, b, c)
#define Sfi_unsigned(a, b, c) sfi(a, b, c)
#define Sfi_xint8(a, b, c) sfi(a, b, c)
#define Sfi_xint16(a, b, c) sfi(a, b, c)
#endif

/* not in _Generic */
#define Sfo_long(a,b,c) sfo_long(a, b, c);
#define Sfo_ulong(a,b,c) sfo_ulong(a, b, c);
#define Sfo_char(a,b,c,d) sfo_char(a, b, c, d)
#define Sfo_boolean(a,b,c) sfo_boolean(a, b, c)
#define Sfo_schar(a,b,c) sfo_schar(a, b, c)

#define Sfi_long(a,b,c) sfi_long(a, b, c);
#define Sfi_ulong(a,b,c) sfi_ulong(a, b, c);
#define Sfi_char(a,b,c,d) sfi_char(a, b, c, d)
#define Sfi_boolean(a,b,c) sfi_boolean(a, b, c)
#define Sfi_schar(a,b,c) sfi_schar(a, b, c)

#endif /* SAVEFILE_H */

