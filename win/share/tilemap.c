/* NetHack 3.7	tilemap.c	$NHDT-Date: 1640987372 2021/12/31 21:49:32 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.64 $ */
/*      Copyright (c) 2016 by Michael Allison                     */
/* NetHack may be freely redistributed.  See license for details. */

/*
 *      This source file is compiled twice:
 *      once without TILETEXT defined to make tilemap.{o,obj},
 *      then again with it defined to produce tiletxt.{o,obj}.
 */

#if 0
#include "config.h"
/* #include "onames.h" */
#include "permonst.h"
#include "objclass.h"
/* #include "pm.h" */
#include "sym.h"
#include "rm.h"
#else
#include "hack.h"
#include "display.h"
#include <stdarg.h>
#endif

#ifdef MONITOR_HEAP
/* with heap monitoring enabled, free(ptr) is a macro which expands to
   nhfree(ptr,__FILE__,__LINE__); since tilemap doesn't link with
   src/alloc.o it doesn't have access to nhfree(); use actual free */
#undef free
#endif

#define Fprintf (void) fprintf
#define Snprintf(str, size, ...) \
    nh_snprintf(__func__, __LINE__, str, size, __VA_ARGS__)
void nh_snprintf(const char *func, int line, char *str, size_t size,
                 const char *fmt, ...);

/*
 * Defining OBTAIN_TILEMAP to get a listing of the tile-mappings
 * for debugging purposes requires that your link to produce
 * the tilemap utility must also include:
 *   objects.o, monst.o drawing.o
 */
#define OBTAIN_TILEMAP

#if defined(OBTAIN_TILEMAP) && !defined(TILETEXT)
FILE *tilemap_file;
#endif

#if defined(MICRO) || defined(WIN32)
#undef exit
#if !defined(MSDOS) && !defined(WIN32)
extern void exit(int);
#endif
#endif

#if defined(CROSSCOMPILE) && defined(ENHANCED_SYMBOLS)
#undef ENHANCED_SYMBOLS
#endif

struct {
    int idx;
    const char *tilelabel;
    const char *expectedlabel;
} altlabels[MAXPCHARS + 1] = {
#define PCHAR_TILES
#include "defsym.h"
#undef PCHAR_TILES
   { 0, 0, 0 }
};

enum {MON_GLYPH, OBJ_GLYPH, OTH_GLYPH, TERMINATOR = -1};
#define EXTRA_SCROLL_DESCR_COUNT ((SCR_BLANK_PAPER - SCR_STINKING_CLOUD) - 1)
const char *altar_text[] = {
    "unaligned", "chaotic", "neutral", "lawful", "other altar",
};
enum wall_levels { main_dungeon, mines, gehennom, knox, sokoban };

int wall_offsets[] = {
        GLYPH_CMAP_MAIN_OFF, GLYPH_CMAP_MINES_OFF,
        GLYPH_CMAP_GEH_OFF, GLYPH_CMAP_KNOX_OFF,
        GLYPH_CMAP_SOKO_OFF,
};
const char *wall_texts[] = {
        "main walls", "mines walls", "gehennom walls",
        "knox walls", "sokoban walls",
};
const char *walldesc[] = {
        "vertical",   "horizontal", "tlcorn", "trcorn", "blcorn", "brcorn",
        "cross wall", "tuwall",     "tdwall", "tlwall", "trwall",
};

int expl_offsets[] = {
    GLYPH_EXPLODE_DARK_OFF, GLYPH_EXPLODE_NOXIOUS_OFF,
    GLYPH_EXPLODE_MUDDY_OFF, GLYPH_EXPLODE_WET_OFF,
    GLYPH_EXPLODE_MAGICAL_OFF, GLYPH_EXPLODE_FIERY_OFF,
    GLYPH_EXPLODE_FROSTY_OFF,
};
const char *expl_texts[] = {
    "dark", "noxious", "muddy", "wet", "magical", "fiery", "frosty",
};

const char *zap_texts[] = {
    "missile", "fire",      "frost",      "sleep",
    "death",   "lightning", "poison gas", "acid",
};

enum tilesrc { monsters_file, objects_file, other_file, generated };
const char *tilesrc_texts[] = {
    "monsters.txt", "objects.txt", "other.txt", "generated",
};

struct tilemap_t {
    short tilenum;
#if defined(OBTAIN_TILEMAP)
    char name[127];
    int glyph;
#endif
} tilemap[MAX_GLYPH];

#define MAX_TILENAM 30
    /* List of tiles encountered and their usage */
struct tiles_used {
    int tilenum;
    enum tilesrc src;
    int file_entry;
    char tilenam[MAX_TILENAM];
    char references[120];
};
struct tiles_used *tilelist[2500] = { 0 };

/* Some special tiles used for init of some things */
int TILE_stone = 0,       /* will get set to correct tile later */
    TILE_unexplored = 0,  /* will get set to correct tile later */
    TILE_nothing = 0,     /* will get set to correct tile later */
    TILE_corr = 0;        /* will get set to correct tile later; X11 uses it */

/* prototypes for functions in this file */
const char *tilename(int, const int, int);
void init_tilemap(void);
void process_substitutions(FILE *);
boolean acceptable_tilename(int, int, const char *, const char *);
#if defined(OBTAIN_TILEMAP)
void precheck(int offset, const char *glyphtype);
void add_tileref(int n, int glyphref, enum tilesrc src, int tile_file_entry,
                 const char *nam, const char *prefix);
void dump_tilerefs(FILE *fp);
void free_tilerefs(void);
#endif

    /* note that the ifdefs here should be the opposite sense from monst.c/
 * objects.c/rm.h
 */

struct conditionals_t {
    int sequence, predecessor;
    const char *name;
} conditionals[] = {
#ifndef CHARON /* not supported */
    { MON_GLYPH, PM_HELL_HOUND, "Cerberus" },
#endif
    /* commented out in monst.c at present */
    { MON_GLYPH, PM_SHOCKING_SPHERE, "beholder" },
    { MON_GLYPH, PM_BABY_SILVER_DRAGON, "baby shimmering dragon" },
    { MON_GLYPH, PM_SILVER_DRAGON, "shimmering dragon" },
    { MON_GLYPH, PM_JABBERWOCK, "vorpal jabberwock" },
    { MON_GLYPH, PM_VAMPIRE_LEADER, "vampire mage" },
#ifndef CHARON /* not supported yet */
    { MON_GLYPH, PM_CROESUS, "Charon" },
#endif
#ifndef MAIL_STRUCTURES
    { MON_GLYPH, PM_FAMINE, "mail daemon" },
#endif
    /* commented out in monst.c at present */
    { MON_GLYPH, PM_SHAMAN_KARNOV, "Earendil" },
    { MON_GLYPH, PM_SHAMAN_KARNOV, "Elwing" },
    /* commented out in monst.c at present */
    { MON_GLYPH, PM_CHROMATIC_DRAGON, "Goblin King" },
    { MON_GLYPH, PM_NEANDERTHAL, "High-elf" },
    /* objects commented out in objects.c at present */
    { OBJ_GLYPH, SILVER_DRAGON_SCALE_MAIL, "shimmering dragon scale mail" },
    { OBJ_GLYPH, SILVER_DRAGON_SCALES, "shimmering dragon scales" },
/* allow slime mold to look like slice of pizza, since we
 * don't know what a slime mold should look like when renamed anyway
 */
#ifndef MAIL_STRUCTURES
    { OBJ_GLYPH, SCR_STINKING_CLOUD + EXTRA_SCROLL_DESCR_COUNT,
      "stamped / mail" },
#endif
    { TERMINATOR, 0, 0 }
};

#if defined(TILETEXT) || defined(OBTAIN_TILEMAP)
/*
 * file_entry is the position of the tile within the monsters/objects/other set
 */
const char *
tilename(int set, const int file_entry, int gend UNUSED)
{
    int i, k, cmap, condnum, tilenum, gendnum;
    static char buf[BUFSZ];
#if 0
    int offset;
#endif
    (void) def_char_to_objclass(']');

    condnum = tilenum = gendnum = 0;

    buf[0] = '\0';
    if (set == MON_GLYPH) {
        for (i = 0; i < NUMMONS; i++) {
            if (tilenum == file_entry) {
                if (mons[i].pmnames[MALE])
                    Snprintf(buf, sizeof buf, "%s {%s}",
                             mons[i].pmnames[NEUTRAL], mons[i].pmnames[MALE]);
                else
                    Snprintf(buf, sizeof buf, "%s", mons[i].pmnames[NEUTRAL]);
                return buf;
            }
            tilenum++;
            if (tilenum == file_entry) {
                if (mons[i].pmnames[FEMALE])
                    Snprintf(buf, sizeof buf, "%s {%s}",
                             mons[i].pmnames[NEUTRAL],
                             mons[i].pmnames[FEMALE]);
                else
                    Snprintf(buf, sizeof buf, "%s", mons[i].pmnames[NEUTRAL]);
                return buf;
            }
            for (condnum = 0; conditionals[condnum].sequence != -1;
                 ++condnum) {
                if (conditionals[condnum].sequence == MON_GLYPH
                    && conditionals[condnum].predecessor == i) {
                    for (k = 0; k < 2; k++) { /* male and female */
                        tilenum++;
                        if (tilenum == file_entry)
                            return conditionals[condnum].name;
                    }
                }
            }
            tilenum++;
        }
        if (tilenum == file_entry)
            return "invisible monster";
    } /* MON_GLYPH */

    if (set == OBJ_GLYPH) {
        tilenum = 0; /* set-relative number */
        for (i = 0; i < NUM_OBJECTS; i++) {
            /* prefer to give the description - that's all the tile's
             * appearance should reveal */
            if (tilenum == file_entry) {
                if (!obj_descr[i].oc_descr)
                    return obj_descr[i].oc_name;
                if (!obj_descr[i].oc_name)
                    return obj_descr[i].oc_descr;

                Sprintf(buf, "%s / %s", obj_descr[i].oc_descr,
                        obj_descr[i].oc_name);
                return buf;
            }
            for (condnum = 0; conditionals[condnum].sequence != -1;
                 ++condnum) {
                if (conditionals[condnum].sequence == OBJ_GLYPH
                    && conditionals[condnum].predecessor == i) {
                    tilenum++;
                    if (tilenum == file_entry)
                        return conditionals[condnum].name;
                }
            }
            tilenum++;
        }
    } /* OBJ_GLYPH */

    if (set == OTH_GLYPH) {
        tilenum = 0; /* set-relative number */

        /* S_stone */
        for (cmap = S_stone; cmap <= S_stone; cmap++) {
            if (tilenum == file_entry) {
                if (*defsyms[cmap].explanation) {
                    return defsyms[cmap].explanation;
                } else if (altlabels[cmap].tilelabel
                           && *altlabels[cmap].tilelabel) {
                    Sprintf(buf, "%s", altlabels[cmap].tilelabel);
                    return buf;
                } else {
                    Sprintf(buf, "cmap %d %d", cmap, tilenum);
                    return buf;
                }
            }
            for (condnum = 0; conditionals[condnum].sequence != -1;
                 condnum++) {
                if (conditionals[condnum].sequence == OTH_GLYPH
                    && conditionals[condnum].predecessor == cmap) {
                    tilenum++;
                }
            }
            tilenum++;
        }

        /* walls - level specific */
        for (k = main_walls; k < mines_walls; k++) {
            for (cmap = S_vwall; cmap <= S_trwall; cmap++) {
                i = cmap - S_vwall;
                if (tilenum == file_entry) {
                    Sprintf(buf, "%s %s", wall_texts[k], walldesc[i]);
                    return buf;
                }
                for (condnum = 0; conditionals[condnum].sequence != -1;
                     condnum++) {
                    if (conditionals[condnum].sequence == OTH_GLYPH
                        && conditionals[condnum].predecessor == cmap) {
                        tilenum++;
                    }
                }
                tilenum++;
            }
        }

        /* cmap A */
        for (cmap = S_ndoor; cmap <= S_brdnladder; cmap++) {
            i = cmap - S_ndoor;
            if (tilenum == file_entry) {
                if (*defsyms[cmap].explanation) {
                    return defsyms[cmap].explanation;
                } else if (altlabels[cmap].tilelabel
                           && *altlabels[cmap].tilelabel) {
                    Sprintf(buf, "%s", altlabels[cmap].tilelabel);
                    return buf;
                } else {
                    Sprintf(buf, "cmap %d %d", cmap, tilenum);
                    return buf;
                }
            }
            for (condnum = 0; conditionals[condnum].sequence != -1;
                 ++condnum) {
                if (conditionals[condnum].sequence == OTH_GLYPH
                    && conditionals[condnum].predecessor == cmap) {
                    tilenum++;
                    if (tilenum == file_entry)
                        return conditionals[condnum].name;
                }
            }
            tilenum++;
        }

        /* Altars */
        cmap = S_altar;
        for (k = altar_unaligned; k <= altar_other; k++) {
            /* Since defsyms only has one altar symbol,
               it isn't much help in identifying details
               these. Roll our own name. */
            if (tilenum == file_entry) {
                Sprintf(buf, "%s altar", altar_text[k]);
                return buf;
            }
            tilenum++;
        }
        for (condnum = 0; conditionals[condnum].sequence != -1; ++condnum) {
            if (conditionals[condnum].sequence == OTH_GLYPH
                && conditionals[condnum].predecessor == cmap) {
                tilenum++;
                if (tilenum == file_entry)
                    return conditionals[condnum].name;
            }
        }

        /* cmap B */
        for (cmap = S_grave; cmap < S_arrow_trap + MAXTCHARS; cmap++) {
            i = cmap - S_grave;
            if (tilenum == file_entry) {
                if (*defsyms[cmap].explanation) {
                    return defsyms[cmap].explanation;
                } else if (altlabels[cmap].tilelabel
                           && *altlabels[cmap].tilelabel) {
                    Sprintf(buf, "%s", altlabels[cmap].tilelabel);
                    return buf;
                } else {
                    Sprintf(buf, "cmap %d %d", cmap, tilenum);
                    return buf;
                }
            }

            for (condnum = 0; conditionals[condnum].sequence != -1;
                 ++condnum) {
                if (conditionals[condnum].sequence == OTH_GLYPH
                    && conditionals[condnum].predecessor == cmap) {
                    tilenum++;
                    if (tilenum == file_entry)
                        return conditionals[condnum].name;
                }
            }
            tilenum++;
        }

#if 0
        /* zaps */
        for (k = 0; k < NUM_ZAP; k++) {
            offset = GLYPH_ZAP_OFF + (k * ((S_rslant - S_vbeam) + 1));
            for (cmap = S_vbeam; cmap <= S_rslant; cmap++) {
                i = cmap - S_vbeam;
                if (tilenum == file_entry) {
                    Sprintf(buf, "%s zap %d %d", zap_texts[k], k + 1, i % 4);
                    return buf;
                }
                for (condnum = 0; conditionals[condnum].sequence != -1;
                     condnum++) {
                    if (conditionals[condnum].sequence == OTH_GLYPH
                        && conditionals[condnum].predecessor == cmap) {
                        tilenum++;
                    }
                }
                tilenum++;
            }
        }
#else
        i = file_entry - tilenum;
        if (i < (NUM_ZAP << 2)) {
            Sprintf(buf, "%s zap %d %d", zap_texts[i / 4], (i / 4) + 1, i % 4);
            return buf;
        }
        tilenum += (NUM_ZAP << 2);
#endif

        /* cmap C */
        for (cmap = S_digbeam; cmap <= S_goodpos; cmap++) {
            i = cmap - S_digbeam;
            if (tilenum == file_entry) {
                if (*defsyms[cmap].explanation) {
                    return defsyms[cmap].explanation;
                } else if (altlabels[cmap].tilelabel
                           && *altlabels[cmap].tilelabel) {
                    Sprintf(buf, "%s", altlabels[cmap].tilelabel);
                    return buf;
                } else {
                    Sprintf(buf, "cmap %d %d", cmap, tilenum);
                    return buf;
                }
            }
            for (condnum = 0; conditionals[condnum].sequence != -1;
                 ++condnum) {
                if (conditionals[condnum].sequence == OTH_GLYPH
                    && conditionals[condnum].predecessor == cmap) {
                    tilenum++;
                    if (tilenum == file_entry)
                        return conditionals[condnum].name;
                }
            }
            tilenum++;
        }

        /* swallow */
        for (cmap = S_sw_tl; cmap <= S_sw_br; cmap++) {
            i = cmap - S_sw_tl;
            if (tilenum + i == file_entry) {
                if (*defsyms[cmap].explanation) {
                    return defsyms[cmap].explanation;
                } else if (altlabels[cmap].tilelabel
                           && *altlabels[cmap].tilelabel) {
                    Sprintf(buf, "%s", altlabels[cmap].tilelabel);
                    return buf;
                } else {
                    Sprintf(buf, "cmap swallow %d", cmap);
                    return buf;
                }
            }
        }
        tilenum += ((S_sw_br - S_sw_tl) + 1);

        /* explosions */
        for (k = expl_dark; k <= expl_frosty; k++) {
            for (cmap = S_expl_tl; cmap <= S_expl_br; cmap++) {
                i = cmap - S_expl_tl;
                if (tilenum == file_entry) {
                    /* substitute "explosion " in the tilelabel
                       with "explosion dark " etc */
                    Sprintf(buf, "explosion %s %s", expl_texts[k],
                            &altlabels[cmap].tilelabel[10]);
                    return buf;
                }
                for (condnum = 0; conditionals[condnum].sequence != -1;
                     ++condnum) {
                    if (conditionals[condnum].sequence == OTH_GLYPH
                        && conditionals[condnum].predecessor == cmap) {
                        tilenum++;
                        if (tilenum == file_entry)
                            return conditionals[condnum].name;
                    }
                }
                tilenum++;
            }
        }

        /* warnings */
        i = file_entry - tilenum;
        if (i < WARNCOUNT) {
            Sprintf(buf, "warning %d", i);
            return buf;
        }
        tilenum += WARNCOUNT;

        i = file_entry - tilenum;
        if (i < 1) {
            Sprintf(buf, "unexplored");
            return buf;
        }
        tilenum += 1;

        i = file_entry - tilenum;
        if (i < 1) {
            Sprintf(buf, "nothing");
            return buf;
        }
        tilenum++;

        /* other level walls */
        /* this batch starts at mines(1), not main(0) */
        for (k = mines_walls; k <= sokoban_walls; k++) {
            for (cmap = S_vwall; cmap <= S_trwall; cmap++) {
                i = cmap - S_vwall;
                if (tilenum == file_entry) {
                    Sprintf(buf, "%s %s", wall_texts[k], walldesc[i]);
                    return buf;
                }
                for (condnum = 0; conditionals[condnum].sequence != -1;
                     condnum++) {
                    if (conditionals[condnum].sequence == OTH_GLYPH
                        && conditionals[condnum].predecessor == cmap) {
                        tilenum++;
                    }
                }
                tilenum++;
            }
        }
    } /* OTH_GLYPH */
    Sprintf(buf, "unknown %d %d", set, file_entry);
    return buf;
}
#endif /* TILETEXT || OBTAIN_TILEMAP */

#ifndef TILETEXT
#define TILE_FILE "tile.c"
#ifdef AMIGA
#define SOURCE_TEMPLATE "NH:src/%s"
#define INCLUDE_TEMPLATE "NH:include/t.%s"
#else
#ifdef MAC
#define SOURCE_TEMPLATE ":src:%s"
#define INCLUDE_TEMPLATE ":include:%s"
#else
#define SOURCE_TEMPLATE "../src/%s"
#define INCLUDE_TEMPLATE "../include/%s"
#endif
#endif

#ifndef STATUES_DONT_LOOK_LIKE_MONSTERS
int lastmontile, lastobjtile, lastothtile, laststatuetile;
#else
int lastmontile, lastobjtile, lastothtile;
#endif

/* Number of tiles for invisible monsters */
#define NUM_INVIS_TILES 1

/*
 * set up array to map glyph numbers to tile numbers
 *
 * assumes tiles are numbered sequentially through monsters/objects/other,
 * with entries for all supported compilation options. monsters have two
 * tiles for each (male + female).
 *
 * "other" contains cmap and zaps (the swallow sets are a repeated portion
 * of cmap), as well as the "flash" glyphs for the new warning system
 * introduced in 3.3.1.
 */
void
init_tilemap(void)
{
    int i, j, k, cmap, condnum, tilenum, offset;
    int corpsetile, swallowbase;
    int file_entry = 0;

#if defined(OBTAIN_TILEMAP)
    tilemap_file = fopen("tilemappings.lst", "w");
    Fprintf(tilemap_file, "NUMMONS = %d\n", NUMMONS);
    Fprintf(tilemap_file, "NUM_OBJECTS = %d\n", NUM_OBJECTS);
    Fprintf(tilemap_file, "MAXEXPCHARS = %d\n", MAXEXPCHARS);
    Fprintf(tilemap_file, "MAXPCHARS = %d\n", MAXPCHARS);
    Fprintf(tilemap_file, "MAX_GLYPH = %d\n", MAX_GLYPH);
    Fprintf(tilemap_file, "GLYPH_MON_OFF = %d\n", GLYPH_MON_OFF);
    Fprintf(tilemap_file, "GLYPH_MON_MALE_OFF = %d\n", GLYPH_MON_MALE_OFF);
    Fprintf(tilemap_file, "GLYPH_MON_FEM_OFF = %d\n", GLYPH_MON_FEM_OFF);
    Fprintf(tilemap_file, "GLYPH_PET_OFF = %d\n", GLYPH_PET_OFF);
    Fprintf(tilemap_file, "GLYPH_PET_MALE_OFF = %d\n", GLYPH_PET_MALE_OFF);
    Fprintf(tilemap_file, "GLYPH_PET_FEM_OFF = %d\n", GLYPH_PET_FEM_OFF);
    Fprintf(tilemap_file, "GLYPH_INVIS_OFF = %d\n", GLYPH_INVIS_OFF);
    Fprintf(tilemap_file, "GLYPH_DETECT_OFF = %d\n", GLYPH_DETECT_OFF);
    Fprintf(tilemap_file, "GLYPH_DETECT_MALE_OFF = %d\n",
            GLYPH_DETECT_MALE_OFF);
    Fprintf(tilemap_file, "GLYPH_DETECT_FEM_OFF = %d\n",
            GLYPH_DETECT_FEM_OFF);
    Fprintf(tilemap_file, "GLYPH_BODY_OFF = %d\n", GLYPH_BODY_OFF);
    Fprintf(tilemap_file, "GLYPH_RIDDEN_OFF = %d\n", GLYPH_RIDDEN_OFF);
    Fprintf(tilemap_file, "GLYPH_RIDDEN_MALE_OFF = %d\n",
            GLYPH_RIDDEN_MALE_OFF);
    Fprintf(tilemap_file, "GLYPH_RIDDEN_FEM_OFF = %d\n",
            GLYPH_RIDDEN_FEM_OFF);
    Fprintf(tilemap_file, "GLYPH_OBJ_OFF = %d\n", GLYPH_OBJ_OFF);
    Fprintf(tilemap_file, "GLYPH_CMAP_OFF = %d\n", GLYPH_CMAP_OFF);
    Fprintf(tilemap_file, "GLYPH_CMAP_STONE_OFF = %d\n", GLYPH_CMAP_STONE_OFF);
    Fprintf(tilemap_file, "GLYPH_CMAP_MAIN_OFF = %d\n", GLYPH_CMAP_MAIN_OFF);
    Fprintf(tilemap_file, "GLYPH_CMAP_MINES_OFF = %d\n",
            GLYPH_CMAP_MINES_OFF);
    Fprintf(tilemap_file, "GLYPH_CMAP_GEH_OFF = %d\n", GLYPH_CMAP_GEH_OFF);
    Fprintf(tilemap_file, "GLYPH_CMAP_KNOX_OFF = %d\n", GLYPH_CMAP_KNOX_OFF);
    Fprintf(tilemap_file, "GLYPH_CMAP_SOKO_OFF = %d\n", GLYPH_CMAP_SOKO_OFF);
    Fprintf(tilemap_file, "GLYPH_CMAP_A_OFF = %d\n", GLYPH_CMAP_A_OFF);
    Fprintf(tilemap_file, "GLYPH_ALTAR_OFF = %d\n", GLYPH_ALTAR_OFF);
    Fprintf(tilemap_file, "GLYPH_CMAP_B_OFF = %d\n", GLYPH_CMAP_B_OFF);
    Fprintf(tilemap_file, "GLYPH_ZAP_OFF = %d\n", GLYPH_ZAP_OFF);
    Fprintf(tilemap_file, "GLYPH_CMAP_C_OFF = %d\n", GLYPH_CMAP_C_OFF);
    Fprintf(tilemap_file, "GLYPH_SWALLOW_OFF = %d\n", GLYPH_SWALLOW_OFF);
    Fprintf(tilemap_file, "GLYPH_EXPLODE_OFF = %d\n", GLYPH_EXPLODE_OFF);
    Fprintf(tilemap_file, "GLYPH_EXPLODE_DARK_OFF = %d\n",
            GLYPH_EXPLODE_DARK_OFF);
    Fprintf(tilemap_file, "GLYPH_EXPLODE_NOXIOUS_OFF = %d\n",
            GLYPH_EXPLODE_NOXIOUS_OFF);
    Fprintf(tilemap_file, "GLYPH_EXPLODE_MUDDY_OFF = %d\n",
            GLYPH_EXPLODE_MUDDY_OFF);
    Fprintf(tilemap_file, "GLYPH_EXPLODE_WET_OFF = %d\n",
            GLYPH_EXPLODE_WET_OFF);
    Fprintf(tilemap_file, "GLYPH_EXPLODE_MAGICAL_OFF = %d\n",
            GLYPH_EXPLODE_MAGICAL_OFF);
    Fprintf(tilemap_file, "GLYPH_EXPLODE_FIERY_OFF = %d\n",
            GLYPH_EXPLODE_FIERY_OFF);
    Fprintf(tilemap_file, "GLYPH_EXPLODE_FROSTY_OFF = %d\n",
            GLYPH_EXPLODE_FROSTY_OFF);
    Fprintf(tilemap_file, "GLYPH_WARNING_OFF = %d\n", GLYPH_WARNING_OFF);
    Fprintf(tilemap_file, "GLYPH_STATUE_OFF = %d\n", GLYPH_STATUE_OFF);
    Fprintf(tilemap_file, "GLYPH_STATUE_MALE_OFF = %d\n",
            GLYPH_STATUE_MALE_OFF);
    Fprintf(tilemap_file, "GLYPH_STATUE_FEM_OFF = %d\n",
            GLYPH_STATUE_FEM_OFF);
    Fprintf(tilemap_file, "GLYPH_OBJ_PILETOP_OFF = %d\n",
            GLYPH_OBJ_PILETOP_OFF);
    Fprintf(tilemap_file, "GLYPH_BODY_PILETOP_OFF = %d\n",
            GLYPH_BODY_PILETOP_OFF);
    Fprintf(tilemap_file, "GLYPH_STATUE_MALE_PILETOP_OFF = %d\n",
            GLYPH_STATUE_MALE_PILETOP_OFF);
    Fprintf(tilemap_file, "GLYPH_STATUE_FEM_PILETOP_OFF = %d\n",
            GLYPH_STATUE_FEM_PILETOP_OFF);
    Fprintf(tilemap_file, "GLYPH_UNEXPLORED_OFF = %d\n",
            GLYPH_UNEXPLORED_OFF);
    Fprintf(tilemap_file, "GLYPH_NOTHING_OFF = %d\n", GLYPH_NOTHING_OFF);
#endif

    for (i = 0; i < MAX_GLYPH; i++) {
        tilemap[i].tilenum = -1;
    }

    corpsetile = NUMMONS                                   /* MON_MALE */
               + NUMMONS                                   /* MON_FEM */
               + NUM_INVIS_TILES                           /* INVIS */
               + CORPSE;                                   /* within OBJ */

    swallowbase = NUMMONS                                  /* MON_MALE */
                + NUMMONS                                  /* MON_FEM */
                + NUM_INVIS_TILES                          /* INVIS */
                + NUM_OBJECTS                              /* Objects */
                + 1                                        /* Stone */
                + ((S_trwall - S_vwall) + 1)               /* main walls */
                + ((S_brdnladder - S_ndoor) + 1)           /* cmap A */
                + (4 + 1)                                  /* 5 altar tiles */
                + (S_arrow_trap + MAXTCHARS - S_grave)     /* cmap B */
                + (NUM_ZAP << 2)                           /* zaps */
                + ((S_goodpos - S_digbeam) + 1);           /* cmap C */

    /* add number compiled out */
    for (i = 0; conditionals[i].sequence != TERMINATOR; i++) {
        switch (conditionals[i].sequence) {
        case MON_GLYPH:
            corpsetile += 2;
            swallowbase += 2;
            break;
        case OBJ_GLYPH:
            if (conditionals[i].predecessor < CORPSE)
                corpsetile++;
            swallowbase++;
            break;
        case OTH_GLYPH:
            if (conditionals[i].predecessor < S_sw_tl)
                swallowbase++;
            break;
        }
    }

    tilenum = 0;
    for (i = 0; i < NUMMONS; i++) {
#if defined(OBTAIN_TILEMAP)
        char buf[256];
#endif

        tilemap[GLYPH_MON_MALE_OFF + i].tilenum = tilenum;
        tilemap[GLYPH_PET_MALE_OFF + i].tilenum = tilenum;
        tilemap[GLYPH_DETECT_MALE_OFF + i].tilenum = tilenum;
        tilemap[GLYPH_RIDDEN_MALE_OFF + i].tilenum = tilenum;
        tilemap[GLYPH_BODY_OFF + i].tilenum = corpsetile;
        tilemap[GLYPH_BODY_PILETOP_OFF + i].tilenum = corpsetile;
#if defined(OBTAIN_TILEMAP)
        Sprintf(buf, "%s (mnum=%d)", tilename(MON_GLYPH, file_entry, 0), i);
        Snprintf(tilemap[GLYPH_MON_MALE_OFF + i].name,
                 sizeof tilemap[0].name,"male %s", buf);
        Snprintf(tilemap[GLYPH_PET_MALE_OFF + i].name,
                 sizeof tilemap[0].name, "%s male %s", "pet", buf);
        Snprintf(tilemap[GLYPH_DETECT_MALE_OFF + i].name,
                 sizeof tilemap[0].name, "%s male %s", "detected", buf);
        Snprintf(tilemap[GLYPH_RIDDEN_MALE_OFF + i].name,
                 sizeof tilemap[0].name, "%s male %s", "ridden", buf);
        Snprintf(tilemap[GLYPH_BODY_OFF + i].name,
                 sizeof tilemap[0].name, "%s %s", "body of", buf);
        Snprintf(tilemap[GLYPH_BODY_PILETOP_OFF + i].name,
                 sizeof tilemap[0].name, "%s %s", "piletop body of", buf);
        add_tileref(tilenum, GLYPH_MON_MALE_OFF + i, monsters_file,
                    file_entry, tilemap[GLYPH_MON_MALE_OFF + i].name, "");
        add_tileref(tilenum, GLYPH_PET_MALE_OFF + i, monsters_file,
                    file_entry, tilemap[GLYPH_PET_MALE_OFF + i].name, "");
        add_tileref(tilenum, GLYPH_DETECT_MALE_OFF + i, monsters_file,
                    file_entry, tilemap[GLYPH_DETECT_MALE_OFF + i].name,"");
        add_tileref(tilenum, GLYPH_RIDDEN_MALE_OFF + i, monsters_file,
                    file_entry, tilemap[GLYPH_RIDDEN_MALE_OFF + i].name, "");
        add_tileref(corpsetile, GLYPH_BODY_OFF + i, objects_file, CORPSE,
                    tilemap[GLYPH_BODY_OFF + i].name, "");
        add_tileref(corpsetile, GLYPH_BODY_PILETOP_OFF + i, objects_file,
                    CORPSE, tilemap[GLYPH_BODY_PILETOP_OFF + i].name, "");
#endif
        tilenum++;
        file_entry++;
        tilemap[GLYPH_MON_FEM_OFF + i].tilenum = tilenum;
        tilemap[GLYPH_PET_FEM_OFF + i].tilenum = tilenum;
        tilemap[GLYPH_DETECT_FEM_OFF + i].tilenum = tilenum;
        tilemap[GLYPH_RIDDEN_FEM_OFF + i].tilenum = tilenum;
#if defined(OBTAIN_TILEMAP)
        Sprintf(buf, "%s (mnum=%d)", tilename(MON_GLYPH, file_entry, 0), i);
        Snprintf(tilemap[GLYPH_MON_FEM_OFF + i].name, 
                 sizeof tilemap[0].name, "female %s", buf);
        Snprintf(tilemap[GLYPH_PET_FEM_OFF + i].name,
                 sizeof tilemap[0].name, "%s female %s", "pet",
                 buf);
        Snprintf(tilemap[GLYPH_DETECT_FEM_OFF + i].name,
                 sizeof tilemap[0].name, "%s female %s",
                 "detected", buf);
        Snprintf(tilemap[GLYPH_RIDDEN_FEM_OFF + i].name,
                 sizeof tilemap[0].name, "%s female %s",
                 "ridden", buf);
        Snprintf(tilemap[GLYPH_BODY_OFF + i].name,
                 sizeof tilemap[0].name, "%s %s", "body of", buf);
        Snprintf(tilemap[GLYPH_BODY_PILETOP_OFF + i].name,
                 sizeof tilemap[0].name, "%s %s",
                 "piletop body of", buf);
        add_tileref(tilenum, GLYPH_MON_FEM_OFF + i, monsters_file,
                    file_entry, tilemap[GLYPH_MON_FEM_OFF + i].name, "");
        add_tileref(tilenum, GLYPH_PET_FEM_OFF + i, monsters_file,
                    file_entry, tilemap[GLYPH_PET_FEM_OFF + i].name, "");
        add_tileref(tilenum, GLYPH_DETECT_FEM_OFF + i, monsters_file,
                    file_entry, tilemap[GLYPH_DETECT_FEM_OFF + i].name, "");
        add_tileref(tilenum, GLYPH_RIDDEN_FEM_OFF + i, monsters_file,
                    file_entry, tilemap[GLYPH_RIDDEN_FEM_OFF + i].name, "");
        add_tileref(corpsetile, GLYPH_BODY_OFF + i, objects_file, CORPSE,
                    tilemap[GLYPH_BODY_OFF + i].name, "");
        add_tileref(corpsetile, GLYPH_BODY_PILETOP_OFF + i, objects_file,
                    corpsetile, tilemap[GLYPH_BODY_PILETOP_OFF + i].name, "");
#endif

        for (condnum = 0; conditionals[condnum].sequence != -1; condnum++) {
            if (conditionals[condnum].sequence == MON_GLYPH
                && conditionals[condnum].predecessor == i) {
                tilenum += 2;       /* male and female */
                file_entry += 2;
#if defined(OBTAIN_TILEMAP)
                Fprintf(tilemap_file, "skipping monst %s (%d)\n",
                        tilename(MON_GLYPH, file_entry, 0), file_entry);
#endif
            }
        }
        tilenum++; /* male + female tiles for each */
        file_entry++;
    }
    tilemap[GLYPH_INVISIBLE].tilenum = tilenum;
#if defined(OBTAIN_TILEMAP)
    Sprintf(tilemap[GLYPH_INVISIBLE].name, "%s (mnum=%d)", "invisible mon",
            file_entry);
    add_tileref(tilenum, GLYPH_INVISIBLE, monsters_file,
                file_entry, tilemap[GLYPH_INVISIBLE].name, "invisible ");
#endif
    lastmontile = tilenum;
    tilenum++;
    file_entry++; /* non-productive, but in case something ever gets
                     inserted right below here ahead of objects */

    /* start of objects */
    file_entry = 0;
    for (i = 0; i < NUM_OBJECTS; i++) {
        tilemap[GLYPH_OBJ_OFF + i].tilenum = tilenum;
        tilemap[GLYPH_OBJ_PILETOP_OFF + i].tilenum = tilenum;
#if defined(OBTAIN_TILEMAP)
        Snprintf(tilemap[GLYPH_OBJ_OFF + i].name,
                 sizeof tilemap[GLYPH_OBJ_OFF + i].name,
                 "%s (onum=%d)",
                tilename(OBJ_GLYPH, file_entry, 0), i);
        Snprintf(tilemap[GLYPH_OBJ_PILETOP_OFF + i].name,
                 sizeof tilemap[GLYPH_OBJ_PILETOP_OFF + i].name,
                 "%s %s (onum=%d)",
                 "piletop", tilename(OBJ_GLYPH, file_entry, 0), i);
        add_tileref(tilenum, GLYPH_OBJ_OFF + i,
                    objects_file, file_entry,
                    tilemap[GLYPH_OBJ_OFF + i].name, "");
        add_tileref(tilenum, GLYPH_OBJ_PILETOP_OFF + i,
                    objects_file, file_entry,
                    tilemap[GLYPH_OBJ_PILETOP_OFF + i].name, "");
#endif
        for (condnum = 0; conditionals[condnum].sequence != -1; condnum++) {
            if (conditionals[condnum].sequence == OBJ_GLYPH
                && conditionals[condnum].predecessor == i) {
                tilenum++;
                file_entry++;
#if defined(OBTAIN_TILEMAP)
                Fprintf(tilemap_file, "skipping obj %s (%d)\n",
                        tilename(OBJ_GLYPH, file_entry, 0), file_entry);
#endif
            }
        }
        tilenum++;
        file_entry++;
    }
    lastobjtile = tilenum - 1;

    file_entry = 0;
    /* S_stone */
    cmap = S_stone;
    precheck((GLYPH_CMAP_STONE_OFF), "stone");
    tilemap[GLYPH_CMAP_STONE_OFF].tilenum = tilenum;
#if defined(OBTAIN_TILEMAP)
    Snprintf(tilemap[GLYPH_CMAP_STONE_OFF].name,
            sizeof tilemap[0].name,
            "%s (cmap=%d)",
            tilename(OTH_GLYPH, file_entry, 0),
            cmap);
    add_tileref(tilenum, GLYPH_CMAP_STONE_OFF,
                other_file, file_entry,
                tilemap[GLYPH_CMAP_STONE_OFF].name, "");
#endif
    TILE_stone = tilenum;   /* Used to init nul_glyphinfo tileidx entry */
    tilenum++;
    file_entry++;

    /* walls in the main dungeon */
    for (k = main_walls; k < mines_walls; k++) {
        offset = wall_offsets[k];
        for (cmap = S_vwall; cmap <= S_trwall; cmap++) {
            i = cmap - S_vwall;
            precheck(offset + i, "walls");
            tilemap[offset + i].tilenum = tilenum;
#if defined(OBTAIN_TILEMAP)
            Snprintf(tilemap[offset + i].name,
                    sizeof tilemap[0].name,
                    "%s (cmap=%d)",
                    tilename(OTH_GLYPH, file_entry, 0),
                    cmap);
            add_tileref(tilenum, offset + i, other_file, file_entry,
                        tilemap[offset + i].name, "");
#endif
            for (condnum = 0; conditionals[condnum].sequence != -1;
                 condnum++) {
                if (conditionals[condnum].sequence == OTH_GLYPH
                    && conditionals[condnum].predecessor == cmap) {
                    tilenum++;
                    file_entry++;
#if defined(OBTAIN_TILEMAP)
                    Fprintf(tilemap_file, "skipping cmap %s (%d) (%d)\n",
                            tilename(OTH_GLYPH, file_entry, 0), file_entry,
                            cmap);
#endif
                }
            }
            tilenum++;
            file_entry++;
        }
    }

    /* cmap A */
    for (cmap = S_ndoor; cmap <= S_brdnladder; cmap++) {
        i = cmap - S_ndoor;
        precheck((GLYPH_CMAP_A_OFF + i), "cmap A");
        tilemap[GLYPH_CMAP_A_OFF + i].tilenum = tilenum;
#if defined(OBTAIN_TILEMAP)
        Snprintf(tilemap[GLYPH_CMAP_A_OFF + i].name,
                 sizeof tilemap[0].name,
                 "cmap A %s (cmap=%d)",
                 tilename(OTH_GLYPH, file_entry, 0), cmap);
        add_tileref(tilenum, GLYPH_CMAP_A_OFF + i, other_file, file_entry,
                    tilemap[GLYPH_CMAP_A_OFF + i].name, "");
#endif
        for (condnum = 0; conditionals[condnum].sequence != -1; condnum++) {
            if (conditionals[condnum].sequence == OTH_GLYPH
                && conditionals[condnum].predecessor == cmap) {
                tilenum++;
                file_entry++;
#if defined(OBTAIN_TILEMAP)
                Fprintf(tilemap_file, "skipping cmap A %s (%d) (%d)\n",
                        tilename(OTH_GLYPH, file_entry, 0), file_entry, cmap);
#endif
            }
        }
        if (cmap == S_corr)
            TILE_corr = tilenum;    /* X11 references this tile during tile init */
        tilenum++;
        file_entry++;
    }

    /* Altars */
    cmap = S_altar;
    j = 0;
    for (k = altar_unaligned; k <= altar_other; k++) {
        offset = GLYPH_ALTAR_OFF + j;
        precheck((offset), "altar");
        tilemap[offset].tilenum = tilenum;
#if defined(OBTAIN_TILEMAP)
        Snprintf(tilemap[offset].name,
                 sizeof tilemap[0].name,
                "%s %s (cmap=%d)",
                altar_text[j], tilename(OTH_GLYPH, file_entry, 0), cmap);
        add_tileref(tilenum, offset, other_file, file_entry,
                    tilemap[offset].name, "");
#endif
        for (condnum = 0; conditionals[condnum].sequence != -1; condnum++) {
            if (conditionals[condnum].sequence == OTH_GLYPH
                && conditionals[condnum].predecessor == cmap) {
#if defined(OBTAIN_TILEMAP)
                Fprintf(tilemap_file, "skipping %s %s (%d)\n",
                        altar_text[j],
                        tilename(OTH_GLYPH, file_entry, 0), cmap);
#endif
                tilenum++;
                file_entry++;
            }
        }
        j++;
        tilenum++;
        file_entry++;
    }

    /* cmap B */
    for (cmap = S_grave; cmap < S_arrow_trap + MAXTCHARS; cmap++) {
        i = cmap - S_grave;
        precheck((GLYPH_CMAP_B_OFF + i), "cmap B");
        tilemap[GLYPH_CMAP_B_OFF + i].tilenum = tilenum;
#if defined(OBTAIN_TILEMAP)
        Snprintf(tilemap[GLYPH_CMAP_B_OFF + i].name,
                 sizeof tilemap[0].name,
                "%s (cmap=%d)",
                tilename(OTH_GLYPH, file_entry, 0), cmap);
        add_tileref(tilenum, GLYPH_CMAP_B_OFF + i, other_file, file_entry,
                    tilemap[GLYPH_CMAP_B_OFF + i].name, "");
#endif
        for (condnum = 0; conditionals[condnum].sequence != -1; condnum++) {
            if (conditionals[condnum].sequence == OTH_GLYPH
                && conditionals[condnum].predecessor == cmap) {
                tilenum++;
                file_entry++;
#if defined(OBTAIN_TILEMAP)
                Fprintf(tilemap_file, "skipping cmap %s (%d) (%d)\n",
                        tilename(OTH_GLYPH, file_entry, 0), file_entry, cmap);
#endif
            }
        }
        tilenum++;
        file_entry++;
    }

    /* zaps */
#if 0
    for (k = 0; k < NUM_ZAP; k++) {
        offset = GLYPH_ZAP_OFF + (k * ((S_rslant - S_vbeam) + 1));
        for (cmap = S_vbeam; cmap <= S_rslant; cmap++) {
            i = cmap - S_vbeam;
            precheck((offset + i), "zaps");
            tilemap[offset + i].tilenum = tilenum;
#if defined(OBTAIN_TILEMAP)
            Snprintf(tilemap[offset + i].name,
                     sizeof tilemap[0].name,
                    "%s (%d) (%d)",
                    tilename(OTH_GLYPH, file_entry, 0), file_entry, cmap);
            add_tileref(tilenum, offset + i, other_file, file_entry,
                    tilemap[offset + i].name, "");
#endif
            for (condnum = 0; conditionals[condnum].sequence != -1;
                 condnum++) {
                if (conditionals[condnum].sequence == OTH_GLYPH
                    && conditionals[condnum].predecessor == cmap) {
#if defined(OBTAIN_TILEMAP)
                    Fprintf(tilemap_file, "skipping zap %s (%d) (%d)\n",
                            tilename(OTH_GLYPH, file_entry, 0), file_entry,
                            cmap);
#endif
                    file_entry++;
                    tilenum++;
                }

            }
            tilenum++;
            file_entry++;
        }
    }
#else
    for (i = 0; i < NUM_ZAP << 2; i++) {
        tilemap[GLYPH_ZAP_OFF + i].tilenum = tilenum;
#if defined(OBTAIN_TILEMAP)
        Snprintf(tilemap[GLYPH_ZAP_OFF + i].name,
                 sizeof tilemap[0].name,
                "zap %s (cmap=%d)",
                tilename(OTH_GLYPH, file_entry, 0), (i >> 2));
        add_tileref(tilenum, GLYPH_ZAP_OFF + i, other_file, file_entry,
                    tilemap[GLYPH_ZAP_OFF + i].name, "");
#endif
        tilenum++;
        file_entry++;
        for (condnum = 0; conditionals[condnum].sequence != -1; condnum++) {
            if (conditionals[condnum].sequence == OTH_GLYPH
               && conditionals[condnum].predecessor == (i + MAXEXPCHARS)) {
#if defined(OBTAIN_TILEMAP)
                Fprintf(tilemap_file, "skipping zap %s (%d)\n",
                        tilename(OTH_GLYPH, file_entry, 0), file_entry);
#endif
                file_entry++;
                tilenum++;
           }
        }
    }
#endif

    /* cmap C */
    for (cmap = S_digbeam; cmap <= S_goodpos; cmap++) {
        i = cmap - S_digbeam;
        precheck((GLYPH_CMAP_C_OFF + i), "cmap C");
        tilemap[GLYPH_CMAP_C_OFF + i].tilenum = tilenum;
#if defined(OBTAIN_TILEMAP)
        Snprintf(tilemap[GLYPH_CMAP_C_OFF + i].name,
                 sizeof tilemap[0].name,
                 "%s (cmap=%d)",
                 tilename(OTH_GLYPH, file_entry, 0), cmap);
        add_tileref(tilenum, GLYPH_CMAP_C_OFF + i, other_file, file_entry,
                    tilemap[GLYPH_CMAP_C_OFF + i].name, "");
#endif
        for (condnum = 0; conditionals[condnum].sequence != -1; condnum++) {
            if (conditionals[condnum].sequence == OTH_GLYPH
                && conditionals[condnum].predecessor == cmap) {
                tilenum++;
                file_entry++;
#if defined(OBTAIN_TILEMAP)
                Fprintf(tilemap_file, "skipping cmap %s (%d) (%d)\n",
                        tilename(OTH_GLYPH, file_entry, 0), file_entry, cmap);
#endif
            }
        }
        tilenum++;
        file_entry++;
    }
    /* swallow */
    const char *swallow_text[] = {
        "swallow top left",      "swallow top center",
        "swallow top right",     "swallow middle left",
        "swallow middle right",  "swallow bottom left",
        "swallow bottom center", "swallow bottom right",
    };

    offset = GLYPH_SWALLOW_OFF;
    for (k = 0; k < NUMMONS; k++) {
//        if (k == 0) {
//            swallowbase = tilenum;
//        }
        for (cmap = S_sw_tl; cmap <= S_sw_br; cmap++) {
            i = cmap - S_sw_tl;
            precheck((offset + i), "swallows");
            tilemap[offset + i].tilenum = swallowbase + i;
#if defined(OBTAIN_TILEMAP)
            Snprintf(tilemap[offset + i].name,
                     sizeof tilemap[0].name,
                     "%s %s (cmap=%d)",
                     swallow_text[i],
                     mons[k].pmnames[NEUTRAL], cmap);
            add_tileref(swallowbase + i, offset + i,
                        other_file, file_entry + i,
                        tilemap[offset + i].name, "");
#endif
        }
        offset += ((S_sw_br - S_sw_tl) + 1);
    }
    tilenum += ((S_sw_br - S_sw_tl) + 1);
    file_entry += ((S_sw_br - S_sw_tl) + 1);

    /* explosions */
    for (k = expl_dark; k <= expl_frosty; k++) {
        offset = expl_offsets[k];
        for (cmap = S_expl_tl; cmap <= S_expl_br; cmap++) {
            i = cmap - S_expl_tl;
            precheck((offset + i), "explosions");
            tilemap[offset + i].tilenum = tilenum;
#if defined(OBTAIN_TILEMAP)
            Snprintf(tilemap[offset + i].name,
                     sizeof tilemap[0].name,
                     "%s (cmap=%d)",
                     tilename(OTH_GLYPH, file_entry, 0), cmap);
            add_tileref(tilenum, offset + i, other_file, file_entry,
                        tilemap[offset + i].name, "");
#endif

            for (condnum = 0; conditionals[condnum].sequence != -1;
                 condnum++) {
                if (conditionals[condnum].sequence == OTH_GLYPH
                    && conditionals[condnum].predecessor == cmap) {
#if defined(OBTAIN_TILEMAP)
                    Fprintf(tilemap_file, "skipping cmap %s (%d)\n",
                            tilename(OTH_GLYPH, file_entry, 0), cmap);
#endif
                    tilenum++;
                    file_entry++;
                }
            }
            tilenum++;
            file_entry++;
        }
    }

    for (i = 0; i < WARNCOUNT; i++) {
        precheck((GLYPH_WARNING_OFF + i), "warnings");
        tilemap[GLYPH_WARNING_OFF + i].tilenum = tilenum;
#if defined(OBTAIN_TILEMAP)
        Snprintf(tilemap[GLYPH_WARNING_OFF + i].name,
                 sizeof tilemap[0].name,
                 "%s (warn=%d)",
                 tilename(OTH_GLYPH, file_entry, 0), file_entry);
        add_tileref(tilenum, GLYPH_WARNING_OFF + i, other_file, file_entry,
                    tilemap[GLYPH_WARNING_OFF + i].name, "");
#endif
        tilenum++;
        file_entry++;
    }

    for (i = 0; i < 1; i++) {
        precheck((GLYPH_UNEXPLORED_OFF + i), "unexplored");
        tilemap[GLYPH_UNEXPLORED_OFF + i].tilenum = tilenum;
#if defined(OBTAIN_TILEMAP)
        Snprintf(tilemap[GLYPH_UNEXPLORED_OFF + i].name,
                 sizeof tilemap[0].name,
                 "unexplored %s (%d)",
                 tilemap[GLYPH_UNEXPLORED_OFF + i].name, file_entry);
        add_tileref(tilenum, GLYPH_UNEXPLORED_OFF + i, other_file, file_entry,
                    tilemap[GLYPH_UNEXPLORED_OFF + i].name, "");
#endif
        TILE_unexplored = tilenum;      /* for writing into tiledef.h */
        tilenum++;
        file_entry++;
    }

    for (i = 0; i < 1; i++) {
        precheck(GLYPH_NOTHING + i, "nothing");
        tilemap[GLYPH_NOTHING + i].tilenum = tilenum;
#if defined(OBTAIN_TILEMAP)
        Snprintf(tilemap[GLYPH_NOTHING + i].name,
                 sizeof tilemap[0].name,
                 " nothing %s (%d)",
                 tilename(OTH_GLYPH, file_entry, 0), file_entry);
        add_tileref(tilenum, GLYPH_NOTHING + i, other_file, file_entry,
                    tilemap[GLYPH_NOTHING + i].name, "");
#endif
        TILE_nothing = tilenum;      /* for writing into tiledef.h */
        tilenum++;
        file_entry++;
    }
    /* other walls beyond the main walls  */
    for (k = mines_walls; k <= sokoban_walls; k++) {
        offset = wall_offsets[k];
        for (cmap = S_vwall; cmap <= S_trwall; cmap++) {
            i = cmap - S_vwall;
            precheck(offset + i, "walls");
            tilemap[offset + i].tilenum = tilenum;
#if defined(OBTAIN_TILEMAP)
            Snprintf(tilemap[offset + i].name,
                     sizeof tilemap[0].name,
                     "%s (cmap=%d)",
                     tilename(OTH_GLYPH, file_entry, 0),
                     cmap);
            add_tileref(tilenum, offset + i, other_file, file_entry,
                        tilemap[offset + i].name, "");
#endif
            for (condnum = 0; conditionals[condnum].sequence != -1; condnum++) {
                if (conditionals[condnum].sequence == OTH_GLYPH
                    && conditionals[condnum].predecessor == cmap) {
                    tilenum++;
                    file_entry++;
#if defined(OBTAIN_TILEMAP)
                    Fprintf(tilemap_file, "skipping cmap %s (%d) (%d)\n",
                            tilename(OTH_GLYPH, file_entry, 0), file_entry, cmap);
#endif
                }
            }
            tilenum++;
            file_entry++;
        }
    }

#ifdef STATUES_DONT_LOOK_LIKE_MONSTERS
    /* statue patch: statues still use the same glyph as in 3.4.x */

    for (i = 0; i < NUMMONS; i++) {
        tilemap[GLYPH_STATUE_OFF + i].tilenum =
            tilemap[GLYPH_OBJ_OFF + STATUE].tilenum;
#ifdef OBTAIN_TILEMAP
        Snprintf(tilemap[GLYPH_STATUE_OFF + i].name,
                 sizeof tilemap[0].name,
                 "%s (%d)",
                 tilename(OTH_GLYPH, file_entry, 0), file_entry);
#endif
    }
#endif
    lastothtile = tilenum - 1;

#ifndef STATUES_DONT_LOOK_LIKE_MONSTERS
    /* STATUES _DO_ LOOK LIKE MONSTERS */
    file_entry = 0;
    /* statue patch: statues look more like the monster */
    for (i = 0; i < NUMMONS; i++) {
        precheck(GLYPH_STATUE_MALE_OFF + i, "male statues");
        tilemap[GLYPH_STATUE_MALE_OFF + i].tilenum = tilenum;
        precheck(GLYPH_STATUE_MALE_PILETOP_OFF + i, "male statue piletop");
        tilemap[GLYPH_STATUE_MALE_PILETOP_OFF + i].tilenum = tilenum;
#if defined(OBTAIN_TILEMAP)
        Snprintf(tilemap[GLYPH_STATUE_MALE_OFF + i].name,
                sizeof tilemap[0].name,
                "statue of male %s (mnum=%d)",
                tilename(MON_GLYPH, file_entry, 0), file_entry);
        Snprintf(tilemap[GLYPH_STATUE_MALE_PILETOP_OFF + i].name,
                sizeof tilemap[0].name,
                "piletop statue of male %s (mnum=%d)",
                tilename(MON_GLYPH, file_entry, 0), file_entry);
        add_tileref(tilenum, GLYPH_STATUE_MALE_OFF + i, generated, file_entry,
                    tilemap[GLYPH_STATUE_MALE_OFF + i].name,
                    "");
        add_tileref(tilenum, GLYPH_STATUE_MALE_PILETOP_OFF + i, generated,
                    file_entry,
                    tilemap[GLYPH_STATUE_MALE_PILETOP_OFF + i].name,
                    "");
#endif
        tilenum++;
        file_entry++;
        precheck(GLYPH_STATUE_FEM_OFF + i, "female statues");
        tilemap[GLYPH_STATUE_FEM_OFF + i].tilenum = tilenum;
        precheck(GLYPH_STATUE_FEM_PILETOP_OFF + i, "female statue piletop");
        tilemap[GLYPH_STATUE_FEM_PILETOP_OFF + i].tilenum = tilenum;
#if defined(OBTAIN_TILEMAP)
        Snprintf(tilemap[GLYPH_STATUE_FEM_OFF + i].name,
                sizeof tilemap[0].name,
                "statue of female %s (mnum=%d)",
                tilename(MON_GLYPH, file_entry, 0), file_entry);
        Sprintf(tilemap[GLYPH_STATUE_FEM_PILETOP_OFF + i].name,
                "piletop statue of female %s (mnum=%d)",
                tilename(MON_GLYPH, file_entry, 0), file_entry);
        add_tileref(tilenum, GLYPH_STATUE_FEM_OFF + i, generated, file_entry,
                    tilemap[GLYPH_STATUE_FEM_OFF + i].name, "");
        add_tileref(tilenum, GLYPH_STATUE_FEM_PILETOP_OFF + i, generated,
                    file_entry,
                    tilemap[GLYPH_STATUE_FEM_PILETOP_OFF + i].name, "");
#endif
        for (condnum = 0; conditionals[condnum].sequence != -1; condnum++) {
            if (conditionals[condnum].sequence == MON_GLYPH
                && conditionals[condnum].predecessor == i) {
                file_entry += 2; /* skip female tile too */
                tilenum += 2;
#if defined(OBTAIN_TILEMAP)
                Fprintf(tilemap_file, "skipping statue of %s (%d)\n",
                        tilename(MON_GLYPH, file_entry, 0), file_entry);
#endif
            }
        }
        tilenum++;
        file_entry++;
    }
    /* go beyond NUMMONS to cover off the invisible tile at the
       end of monsters.txt so that the tile mapping matches things
       in the .bmp file (for example) */
    file_entry = 0;
#if defined(OBTAIN_TILEMAP)
    add_tileref(tilenum, NO_GLYPH, monsters_file, file_entry,
                "invisible statue", "");
#endif
    laststatuetile = tilenum - 1;
#endif /* STATUES_DONT_LOOK_LIKE_MONSTERS */

#if defined(OBTAIN_TILEMAP)
    for (i = 0; i < MAX_GLYPH; ++i) {
        Fprintf(tilemap_file, "glyph[%04d] [%04d] %-80s\n",
                i, tilemap[i].tilenum, tilemap[i].name);
    }
    dump_tilerefs(tilemap_file);
    fclose(tilemap_file);
#endif
}

#if defined(OBTAIN_TILEMAP)
extern void monst_globals_init(void);
extern void objects_globals_init(void);
#endif

DISABLE_WARNING_UNREACHABLE_CODE

int
main(int argc UNUSED, char *argv[] UNUSED)
{
    int i, tilenum;
    char filename[30];
    FILE *ofp;
    const char *enhanced;
    const char indent[] = "    ";

#if defined(OBTAIN_TILEMAP)
    objects_globals_init();
    monst_globals_init();
#endif

    init_tilemap();

#ifdef ENHANCED_SYMBOLS
    enhanced = ", utf8rep";
#else
    enhanced = "";
#endif
    /*
     * create the source file, "tile.c"
     */
    Snprintf(filename, sizeof filename, SOURCE_TEMPLATE, TILE_FILE);
    if (!(ofp = fopen(filename, "w"))) {
        perror(filename);
        exit(EXIT_FAILURE);
    }
    Fprintf(ofp,
            "/* This file is automatically generated.  Do not edit. */\n");
    Fprintf(ofp, "\n#include \"hack.h\"\n");
    Fprintf(ofp, "\n#ifndef TILES_IN_GLYPHMAP\n");
    Fprintf(ofp, "\n#else /* ?TILES_IN_GLYPHMAP */\n");
    Fprintf(ofp, "\nenum special_tiles {\n");
    Fprintf(ofp, "    TILE_CORR = %d,\n", TILE_corr); 
    Fprintf(ofp, "    TILE_STONE = %d,\n", TILE_stone); 
    Fprintf(ofp, "    TILE_UNEXPLORED = %d\n", TILE_unexplored);
    Fprintf(ofp, "};\n");
    Fprintf(ofp, "\nint total_tiles_used = %d,\n", laststatuetile + 1);
    Fprintf(ofp, "%sTile_corr = TILE_CORR,\n", indent); /* X11 uses it */
    Fprintf(ofp, "%sTile_stone = TILE_STONE,\n",  indent);
    Fprintf(ofp, "%sTile_unexplored = TILE_UNEXPLORED,\n",  indent);
    Fprintf(ofp, "%smaxmontile = %d,\n", indent, lastmontile);
    Fprintf(ofp, "%smaxobjtile = %d,\n", indent, lastobjtile);
    Fprintf(ofp, "%smaxothtile = %d;\n\n", indent, lastothtile);
    Fprintf(ofp, "/* glyph, ttychar, { %s%s } */\n",
            "glyphflags, {color, symidx}, ovidx, tileidx", enhanced);
#ifdef ENHANCED_SYMBOLS
    enhanced = ", 0"; /* replace ", utf8rep" since we're done with that */
#endif
    Fprintf(ofp, "const glyph_info nul_glyphinfo = { \n");
    Fprintf(ofp, "%sNO_GLYPH, ' ', NO_COLOR,\n", indent);
    Fprintf(ofp, "%s%s/* glyph_map */\n", indent, indent);
    Fprintf(ofp, "%s%s{ %s, TILE_UNEXPLORED%s }\n", indent, indent,
            "MG_UNEXPL, { NO_COLOR, SYM_UNEXPLORED + SYM_OFF_X }",
            enhanced);
    Fprintf(ofp, "};\n");
    Fprintf(ofp, "\nglyph_map glyphmap[MAX_GLYPH] = {\n");

    for (i = 0; i < MAX_GLYPH; i++) {
        tilenum = tilemap[i].tilenum;
        if (tilenum < 0) { /* will be -1 if not assigned a real value */
            Fprintf(stderr, "ERROR: glyph %d maps to tile %d.\n", i, tilenum);
            (void) fclose(ofp);
            unlink(filename);
            exit(EXIT_FAILURE);
            /*NOTREACHED*/
        }
        Fprintf(ofp,
                "    { 0U, { 0, 0 }, %4d%s },   /* [%04d] %s:%03d %s */\n",
                tilenum, enhanced, i,
                tilesrc_texts[tilelist[tilenum]->src],
                tilelist[tilenum]->file_entry,
                tilemap[i].name);
    }
    Fprintf(ofp, "};\n");
    Fprintf(ofp, "\n#endif /* TILES_IN_GLYPHMAP */\n");
    Fprintf(ofp, "\n/*tile.c*/\n");

    (void) fclose(ofp);
    free_tilerefs();
    exit(EXIT_SUCCESS);
    /*NOTREACHED*/
    return 0;
}

RESTORE_WARNINGS

#endif /* TILETEXT */

boolean
acceptable_tilename(
    int glyph_set,
    int idx,
    const char *encountered,
    const char *expected)
{
    int i;
    size_t a, b;
    char buf[BUFSZ];
    const char *pastprefix = encountered;
    struct aliaslist {
        const char *original;
        const char *alias;
    };
    struct aliaslist aliases[] = {
        { "wall", "vertical wall" },
        { "wall", "horizontal wall" },
        { "wall", "top left corner wall" },
        { "wall", "top right corner wall" },
        { "wall", "bottom left corner wall" },
        { "wall", "bottom right corner wall" },
        { "open door", "vertical open door" },
        { "open door", "horizontal open door" },
        { "open door", "no door" },
        { "altar", "chaotic altar" },
        { "altar", "neutral altar" },
        { "altar", "lawful altar" },
        { "opulent throne", "throne" },
        { "water", "pool" },
        { "lowered drawbridge", "vertical open drawbridge" },
        { "lowered drawbridge", "horizontal open drawbridge" },
        { "raised drawbridge", "vertical closed drawbridge" },
        { "raised drawbridge", "horizontal closed drawbridge" },
        { "altar", "unaligned altar" },
        { "altar", "other altar" },
#if 0
        { "dark part of a room", "stone" },
#endif
    };

    if (glyph_set == OTH_GLYPH) {
        if (idx >= 0 && idx < (SIZE(altlabels) - 1)) {
            if (!strcmp(altlabels[idx].tilelabel, encountered))
                return TRUE;
        }
        a = strlen(encountered);
        for (i = 0; i < SIZE(aliases); i++) {
            if (!strcmp(pastprefix, aliases[i].alias))
                return TRUE;
            pastprefix = encountered;
            b = strlen(aliases[i].alias);
            if (a > b) {
                pastprefix = encountered + (a - b);
                if (!strcmp(pastprefix, aliases[i].alias))
                    return TRUE;
            }
            if (!strcmp(encountered, aliases[i].alias)
                && !strcmp(expected, aliases[i].original)) {
                return TRUE;
            }
        }
        Snprintf(buf, sizeof buf, "cmap tile %d", idx);
        if (!strcmp(expected, buf))
            return TRUE;
        return FALSE;
    }
    return TRUE;
}

#if defined(OBTAIN_TILEMAP)
void
precheck(int offset, const char *glyphtype)
{
    if (tilemap[offset].tilenum != -1)
        Fprintf(stderr, "unexpected re-write of tile mapping [%s]\n",
                glyphtype);
}

void add_tileref(
    int n,
    int glyphref,
    enum tilesrc src,
    int entrynum,
    const char *nam,
    const char *prefix)
{
    struct tiles_used temp = { 0 };
    static const char ellipsis[] UNUSED = "...";
    char buf[BUFSZ];

    if (!tilelist[n]) {
        tilelist[n] = malloc(sizeof temp);
        tilelist[n]->tilenum = n;
        tilelist[n]->src = src;
        tilelist[n]->file_entry = entrynum;
        /* leave room for trailing "...nnnn" */
        Snprintf(tilelist[n]->tilenam, sizeof tilelist[n]->tilenam - 7,
                 "%s%s", prefix, nam);
        tilelist[n]->references[0] = '\0';
    }
    Snprintf(temp.references,
             sizeof temp.references - 7, /* room for "...nnnn" */
             "%s%s%d", tilelist[n]->references,
             (tilelist[n]->references[0] != '\0') ? ", " : "", glyphref);
    Snprintf(buf, sizeof buf, "...%4d", glyphref);
    Snprintf(tilelist[n]->references, sizeof tilelist[n]->references, "%s%s",
             temp.references,
             (strlen(temp.references) >= (sizeof temp.references - 7) - 1)
                 ? buf
                 : "");
}

void
dump_tilerefs(FILE * fp)
{
    int i;

    Fprintf(fp, "\n");
    for (i = 0; i < SIZE(tilelist); i++) {
        if (tilelist[i]) {
            Fprintf(fp, "tile[%04d] %s[%04d] %-25s: %s\n", i,
                    tilesrc_texts[tilelist[i]->src],
                    tilelist[i]->file_entry,
                    tilelist[i]->tilenam,
                    tilelist[i]->references);
        }
    }
}

void
free_tilerefs(void)
{
    int i;

    for (i = 0; i < SIZE(tilelist); i++) {
        if (tilelist[i])
            free((genericptr_t) tilelist[i]), tilelist[i] = 0;
    }
}

#endif

DISABLE_WARNING_FORMAT_NONLITERAL

void
nh_snprintf(
    const char *func UNUSED,
    int line UNUSED,
    char *str, size_t size,
    const char *fmt, ...)
{
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(str, size, fmt, ap);
    va_end(ap);

    if (n < 0 || (size_t) n >= size) { /* is there a problem? */
        str[size - 1] = 0;             /* make sure it is nul terminated */
    }
}

RESTORE_WARNING_FORMAT_NONLITERAL


/*tilemap.c*/
