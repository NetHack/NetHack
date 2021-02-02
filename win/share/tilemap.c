/* NetHack 3.7	tilemap.c	$NHDT-Date: 1596498340 2020/08/03 23:45:40 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.47 $ */
/*      Copyright (c) 2016 by Michael Allison                     */
/* NetHack may be freely redistributed.  See license for details. */

/*
 *      This source file is compiled twice:
 *      once without TILETEXT defined to make tilemap.{o,obj},
 *      then again with it defined to produce tiletxt.{o,obj}.
 */

#include "config.h"
#include "pm.h"
#include "onames.h"
#include "permonst.h"
#include "objclass.h"
#include "rm.h"
#include "display.h"

#define Fprintf (void) fprintf

/*
 * Defining OBTAIN_TILEMAP to get a listing of the tile-mappings
 * for debugging purposes requires that your link to produce
 * the tilemap utility must also include:
 *   objects.o, monst.o drawing.o
 */
/* #define OBTAIN_TILEMAP */

#if defined(OBTAIN_TILEMAP) && !defined(TILETEXT)
FILE *tilemap_file;
#endif

const char *tilename(int, int, int);
void init_tilemap(void);
void process_substitutions(FILE *);
boolean acceptable_tilename(int, int, const char *, const char *);

#if defined(MICRO) || defined(WIN32)
#undef exit
#if !defined(MSDOS) && !defined(WIN32)
extern void exit(int);
#endif
#endif

enum {MON_GLYPH, OBJ_GLYPH, OTH_GLYPH, TERMINATOR = -1};
#define EXTRA_SCROLL_DESCR_COUNT ((SCR_BLANK_PAPER - SCR_STINKING_CLOUD) - 1)

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

/*
 * Some entries in glyph2tile[] should be substituted for on various levels.
 * The tiles used for the substitute entries will follow the usual ones in
 * other.til in the order given here, which should have every substitution
 * for the same set of tiles grouped together.  You will have to change
 * more code in process_substitutions()/substitute_tiles() if the sets
 * overlap in the future.
 */
struct substitute {
    int first_glyph, last_glyph;
    const char *sub_name; /* for explanations */
    const char *level_test;
} substitutes[] = { { GLYPH_CMAP_OFF + S_vwall, GLYPH_CMAP_OFF + S_trwall,
                      "mine walls", "In_mines(plev)" },
                    { GLYPH_CMAP_OFF + S_vwall, GLYPH_CMAP_OFF + S_trwall,
                      "gehennom walls", "In_hell(plev)" },
                    { GLYPH_CMAP_OFF + S_vwall, GLYPH_CMAP_OFF + S_trwall,
                      "knox walls", "Is_knox(plev)" },
                    { GLYPH_CMAP_OFF + S_vwall, GLYPH_CMAP_OFF + S_trwall,
                      "sokoban walls", "In_sokoban(plev)" } };

#if defined(TILETEXT) || defined(OBTAIN_TILEMAP)
/*
 * file_entry is the position of the tile within the monsters/objects/other set
 */
const char *
tilename(int set, int file_entry, int gend)
{
    int i, j, condnum, tilenum, gendnum;
    static char buf[BUFSZ];

    (void) def_char_to_objclass(']');

    condnum = tilenum = gendnum = 0;

    for (i = 0; i < NUMMONS; i++) {
        if (set == MON_GLYPH && tilenum == file_entry && gend == 0)
            return mons[i].pmnames[NEUTRAL];
        for (condnum = 0; conditionals[condnum].sequence != -1; ++condnum) {
            if (conditionals[condnum].sequence == MON_GLYPH
                && conditionals[condnum].predecessor == i) {
                tilenum += 2;
                if (set == MON_GLYPH && tilenum == file_entry)
                    return conditionals[condnum].name;
            }
        }
        tilenum += 2;
    }
    if (set == MON_GLYPH && tilenum == file_entry)
        return "invisible monster";

    tilenum = 0; /* set-relative number */
    for (i = 0; i < NUM_OBJECTS; i++) {
        /* prefer to give the description - that's all the tile's
         * appearance should reveal */
        if (set == OBJ_GLYPH && tilenum == file_entry) {
            if (!obj_descr[i].oc_descr)
                return obj_descr[i].oc_name;
            if (!obj_descr[i].oc_name)
                return obj_descr[i].oc_descr;

            Sprintf(buf, "%s / %s", obj_descr[i].oc_descr,
                    obj_descr[i].oc_name);
            return buf;
        }
	for (condnum = 0; conditionals[condnum].sequence != -1; ++condnum) {
            if (conditionals[condnum].sequence == OBJ_GLYPH
                && conditionals[condnum].predecessor == i) {
                tilenum++;
                if (set == OBJ_GLYPH && tilenum == file_entry)
                    return conditionals[condnum].name;
            }
        }
        tilenum++;
    }

    tilenum = 0; /* set-relative number */
    for (i = 0; i < (MAXPCHARS - MAXEXPCHARS); i++) {
        if (set == OTH_GLYPH && tilenum == file_entry) {
            if (*defsyms[i].explanation) {
                return defsyms[i].explanation;
            } else {
                Sprintf(buf, "cmap %d", tilenum);
                return buf;
            }
        }
	for (condnum = 0; conditionals[condnum].sequence != -1; ++condnum) {
            if (conditionals[condnum].sequence == OTH_GLYPH
                && conditionals[condnum].predecessor == i) {
                tilenum++;
                if (set == OTH_GLYPH && tilenum == file_entry)
                    return conditionals[condnum].name;
            }
        }
        tilenum++;
    }
    /* explosions */
    tilenum = MAXPCHARS - MAXEXPCHARS;
    i = file_entry - tilenum;
    if (i < (MAXEXPCHARS * EXPL_MAX)) {
        if (set == OTH_GLYPH) {
            static const char *explosion_types[] = {
                /* hack.h */
                "dark", "noxious", "muddy", "wet", "magical", "fiery",
                "frosty"
            };
            Sprintf(buf, "explosion %s %d", explosion_types[i / MAXEXPCHARS],
                    i % MAXEXPCHARS);
            return buf;
        }
    }
    tilenum += (MAXEXPCHARS * EXPL_MAX);

    i = file_entry - tilenum;
    if (i < (NUM_ZAP << 2)) {
        if (set == OTH_GLYPH) {
            Sprintf(buf, "zap %d %d", i / 4, i % 4);
            return buf;
        }
    }
    tilenum += (NUM_ZAP << 2);

    i = file_entry - tilenum;
    if (i < WARNCOUNT) {
        if (set == OTH_GLYPH) {
            Sprintf(buf, "warning %d", i);
            return buf;
        }
    }
    tilenum += WARNCOUNT;

    i = file_entry - tilenum;
    if (i < 1) {
        if (set == OTH_GLYPH) {
            Sprintf(buf, "unexplored");
            return buf;
        }
    }
    tilenum += 1;

    i = file_entry - tilenum;
    if (i < 1) {
        if (set == OTH_GLYPH) {
            Sprintf(buf, "nothing");
            return buf;
        }
    }
    tilenum++;

    for (i = 0; i < SIZE(substitutes); i++) {
        j = file_entry - tilenum;
        if (j <= substitutes[i].last_glyph - substitutes[i].first_glyph) {
            if (set == OTH_GLYPH) {
                Sprintf(buf, "sub %s %d", substitutes[i].sub_name, j);
                return buf;
            }
        }
        tilenum += substitutes[i].last_glyph - substitutes[i].first_glyph + 1;
    }

    Sprintf(buf, "unknown %d %d", set, file_entry);
    return buf;
}
#endif

#ifndef TILETEXT
#define TILE_FILE "tile.c"

#ifdef AMIGA
#define SOURCE_TEMPLATE "NH:src/%s"
#else
#ifdef MAC
#define SOURCE_TEMPLATE ":src:%s"
#else
#define SOURCE_TEMPLATE "../src/%s"
#endif
#endif

struct tilemap_t {
    short tilenum;
#ifdef OBTAIN_TILEMAP
    char name[80];
    int glyph;
#endif
} tilemap[MAX_GLYPH];


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
    int i, j, condnum, tilenum;
    int corpsetile, swallowbase;
    int file_entry = 0;

    for (i = 0; i < MAX_GLYPH; i++) {
        tilemap[i].tilenum = -1;
    }

    corpsetile =  NUMMONS + NUMMONS + NUM_INVIS_TILES + CORPSE;
    swallowbase = NUMMONS + NUMMONS + NUM_INVIS_TILES + NUM_OBJECTS + S_sw_tl;

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

#ifdef OBTAIN_TILEMAP
    tilemap_file = fopen("tilemappings.lst", "w");
#endif
    tilenum = 0;
    for (i = 0; i < NUMMONS; i++) {
#ifdef OBTAIN_TILEMAP
        char buf[256];
#endif
        tilemap[GLYPH_MON_OFF + i].tilenum = tilenum;
        tilemap[GLYPH_PET_OFF + i].tilenum = tilenum;
        tilemap[GLYPH_DETECT_OFF + i].tilenum = tilenum;
        tilemap[GLYPH_RIDDEN_OFF + i].tilenum = tilenum;
        tilemap[GLYPH_BODY_OFF + i].tilenum = corpsetile;
        j = GLYPH_SWALLOW_OFF + 8 * i;
        tilemap[j].tilenum = swallowbase;
        tilemap[j + 1].tilenum = swallowbase + 1;
        tilemap[j + 2].tilenum = swallowbase + 2;
        tilemap[j + 3].tilenum = swallowbase + 3;
        tilemap[j + 4].tilenum = swallowbase + 4;
        tilemap[j + 5].tilenum = swallowbase + 5;
        tilemap[j + 6].tilenum = swallowbase + 6;
        tilemap[j + 7].tilenum = swallowbase + 7;
#ifdef OBTAIN_TILEMAP
        Sprintf(buf, "%s (%d)", tilename(MON_GLYPH, file_entry, 0), file_entry);
        Sprintf(tilemap[GLYPH_MON_OFF + i].name,
                "%s (%d)", buf, i);
        Sprintf(tilemap[GLYPH_PET_OFF + i].name,
                "%s %s (%d)", buf, "pet", i);
        Sprintf(tilemap[GLYPH_DETECT_OFF + i].name,
                "%s %s (%d)", buf, "detected", i);
        Sprintf(tilemap[GLYPH_RIDDEN_OFF + i].name,
                "%s %s (%d)", buf, "ridden", i);
        Sprintf(tilemap[GLYPH_BODY_OFF + i].name,
                "%s %s (%d)", buf, "corpse", i);
        Sprintf(tilemap[j + 0].name, "%s swallow0 (%d)", buf, i);
        Sprintf(tilemap[j + 1].name, "%s swallow1 (%d)", buf, i);
        Sprintf(tilemap[j + 2].name, "%s swallow2 (%d)", buf, i);
        Sprintf(tilemap[j + 3].name, "%s swallow3 (%d)", buf, i);
        Sprintf(tilemap[j + 4].name, "%s swallow4 (%d)", buf, i);
        Sprintf(tilemap[j + 5].name, "%s swallow5 (%d)", buf, i);
        Sprintf(tilemap[j + 6].name, "%s swallow6 (%d)", buf, i);
        Sprintf(tilemap[j + 7].name, "%s swallow7 (%d)", buf, i);
#endif
        for (condnum = 0; conditionals[condnum].sequence != -1; condnum++) {
            if (conditionals[condnum].sequence == MON_GLYPH
                && conditionals[condnum].predecessor == i) {
                tilenum += 2;
                file_entry += 2;
#ifdef OBTAIN_TILEMAP
                Fprintf(tilemap_file, "skipping monst %s (%d)\n",
                        tilename(MON_GLYPH, file_entry, 0), file_entry);
#endif
            }
        }
        tilenum += 2;  /* male + female tiles for each */
        file_entry += 2;
    }
    tilemap[GLYPH_INVISIBLE].tilenum = tilenum++;
    file_entry++;
#ifdef OBTAIN_TILEMAP
    Sprintf(tilemap[GLYPH_INVISIBLE].name,
            "%s (%d)", "invisible mon", file_entry);
#endif
    lastmontile = tilenum - 1;

    file_entry = 0;
    for (i = 0; i < NUM_OBJECTS; i++) {
        tilemap[GLYPH_OBJ_OFF + i].tilenum = tilenum;
#ifdef OBTAIN_TILEMAP
        Sprintf(tilemap[GLYPH_OBJ_OFF + i].name, "%s (%d)",
                tilename(OBJ_GLYPH, file_entry, 0), file_entry);
#endif
        for (condnum = 0; conditionals[condnum].sequence != -1; condnum++) {
            if (conditionals[condnum].sequence == OBJ_GLYPH
                && conditionals[condnum].predecessor == i) {
                tilenum++;
                file_entry++;
#ifdef OBTAIN_TILEMAP
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
    for (i = 0; i < (MAXPCHARS - MAXEXPCHARS); i++) {
        tilemap[GLYPH_CMAP_OFF + i].tilenum = tilenum;
#ifdef OBTAIN_TILEMAP
        Sprintf(tilemap[GLYPH_CMAP_OFF + i].name, "cmap %s (%d)",
                tilename(OTH_GLYPH, file_entry, 0), file_entry);
#endif
        tilenum++;
        file_entry++;
        for (condnum = 0; conditionals[condnum].sequence != -1; condnum++) {
            if (conditionals[condnum].sequence == OTH_GLYPH
                && conditionals[condnum].predecessor == i) {
                tilenum++;
                file_entry++;
#ifdef OBTAIN_TILEMAP
                Fprintf(tilemap_file, "skipping cmap %s (%d)\n",
                        tilename(OTH_GLYPH, file_entry, 0), file_entry);
#endif
            }
        }
    }

    for (i = 0; i < (MAXEXPCHARS * EXPL_MAX); i++) {
        tilemap[GLYPH_EXPLODE_OFF + i].tilenum = tilenum;
#ifdef OBTAIN_TILEMAP
        Sprintf(tilemap[GLYPH_EXPLODE_OFF + i].name, "explosion %s (%d)",
                tilename(OTH_GLYPH, file_entry, 0), file_entry);
#endif
        for (condnum = 0; conditionals[condnum].sequence != -1; condnum++) {
            if (conditionals[condnum].sequence == OTH_GLYPH
                && conditionals[condnum].predecessor == i + MAXPCHARS) {
                tilenum++;
                file_entry++;
#ifdef OBTAIN_TILEMAP
                Fprintf(tilemap_file, "skipping explosion %s (%d)\n",
                        tilename(OTH_GLYPH, file_entry, 0), file_entry);
#endif
            }
        }
        tilenum++;
        file_entry++;
    }

    for (i = 0; i < NUM_ZAP << 2; i++) {
        tilemap[GLYPH_ZAP_OFF + i].tilenum = tilenum;
#ifdef OBTAIN_TILEMAP
        Sprintf(tilemap[GLYPH_ZAP_OFF + i].name, "zap %s (%d)",
                tilename(OTH_GLYPH, file_entry, 0), file_entry);
#endif
        tilenum++;
        file_entry++;
        for (condnum = 0; conditionals[condnum].sequence != -1; condnum++) {
            if (conditionals[condnum].sequence == OTH_GLYPH
               && conditionals[condnum].predecessor == (i + MAXEXPCHARS)) {
#ifdef OBTAIN_TILEMAP
                Fprintf(tilemap_file, "skipping zap %s (%d)\n",
                        tilename(OTH_GLYPH, file_entry, 0), file_entry);
#endif
                file_entry++;
                tilenum++;
           }
        }
    }

    for (i = 0; i < WARNCOUNT; i++) {
        tilemap[GLYPH_WARNING_OFF + i].tilenum = tilenum;
#ifdef OBTAIN_TILEMAP
        Sprintf(tilemap[GLYPH_WARNING_OFF + i].name, "%s (%d)",
                tilename(OTH_GLYPH, file_entry, 0), file_entry);
#endif
        tilenum++;
        file_entry++;
    }

    for (i = 0; i < 1; i++) {
        tilemap[GLYPH_UNEXPLORED_OFF + i].tilenum = tilenum;
#ifdef OBTAIN_TILEMAP
        Sprintf(tilemap[GLYPH_UNEXPLORED_OFF + i].name, "unexplored %s (%d)",
                tilename(OTH_GLYPH, file_entry, 0), file_entry);
#endif
        tilenum++;
        file_entry++;
    }

    for (i = 0; i < 1; i++) {
        tilemap[GLYPH_NOTHING + i].tilenum = tilenum;
#ifdef OBTAIN_TILEMAP
        Sprintf(tilemap[GLYPH_NOTHING + i].name, " nothing %s (%d)",
                tilename(OTH_GLYPH, file_entry, 0), file_entry);
#endif
        tilenum++;
        file_entry++;
    }

#ifdef STATUES_DONT_LOOK_LIKE_MONSTERS
    /* statue patch: statues still use the same glyph as in 3.4.x */

    for (i = 0; i < NUMMONS; i++) {
        tilemap[GLYPH_STATUE_OFF + i].tilenum
                    = tilemap[GLYPH_OBJ_OFF + STATUE].tilenum;
#ifdef OBTAIN_TILEMAP
        Sprintf(tilemap[GLYPH_STATUE_OFF + i].name, "%s (%d)",
                tilename(OTH_GLYPH, file_entry, 0), file_entry);
#endif
    }
#endif

    lastothtile = tilenum - 1;

#ifndef STATUES_DONT_LOOK_LIKE_MONSTERS
    file_entry = 0;
    /* fast-forward over the substitutes to grayscale statues loc */
    for (i = 0; i < SIZE(substitutes); i++) {
        tilenum += substitutes[i].last_glyph - substitutes[i].first_glyph + 1;
    }

    /* statue patch: statues look more like the monster */
    for (i = 0; i < NUMMONS; i++) {
        tilemap[GLYPH_STATUE_OFF + i].tilenum = tilenum;
#ifdef OBTAIN_TILEMAP
        Sprintf(tilemap[GLYPH_STATUE_OFF + i].name, "statue of %s (%d)",
                tilename(MON_GLYPH, file_entry, 0), file_entry);
#endif
        for (condnum = 0; conditionals[condnum].sequence != -1; condnum++) {
            if (conditionals[condnum].sequence == MON_GLYPH
                && conditionals[condnum].predecessor == i) {
                file_entry += 2;   /* skip female tile too */
                tilenum += 2;
#ifdef OBTAIN_TILEMAP
                Fprintf(tilemap_file, "skipping statue of %s (%d)\n",
                        tilename(MON_GLYPH, file_entry, 0), file_entry);
#endif
            }
        }
        tilenum += 2;
        file_entry += 2;
    }
    laststatuetile = tilenum - 2;
#endif /* STATUES_DONT_LOOK_LIKE_MONSTERS */
#ifdef OBTAIN_TILEMAP
    for (i = 0; i < MAX_GLYPH; ++i) {
        Fprintf(tilemap_file, "[%04d] [%04d] %-80s\n",
                i, tilemap[i].tilenum, tilemap[i].name);
    }
    fclose(tilemap_file);
#endif
}

const char *prolog[] = { "", "void", "substitute_tiles(d_level *plev)",
                         "{", "    int i;", "" };

const char *epilog[] = { "    return;", "}" };

/* write out the substitutions in an easily-used form. */
void
process_substitutions(FILE *ofp)
{
    static const char Dent[] = "    "; /* 4 space indentation */
    int i, j, k, span, start;

    Fprintf(ofp, "\n");

    j = 0; /* unnecessary */
    span = -1;
    for (i = 0; i < SIZE(substitutes); i++) {
        if (i == 0 || substitutes[i].first_glyph != substitutes[j].first_glyph
            || substitutes[i].last_glyph != substitutes[j].last_glyph) {
            j = i;
            span++;
            Fprintf(ofp, "short std_tiles%d[] = { ", span);
            for (k = substitutes[i].first_glyph;
                 k < substitutes[i].last_glyph; k++)
                Fprintf(ofp, "%d, ", tilemap[k].tilenum);
            Fprintf(ofp, "%d };\n", tilemap[substitutes[i].last_glyph].tilenum);
        }
    }

    for (i = 0; i < SIZE(prolog); i++) {
        Fprintf(ofp, "%s\n", prolog[i]);
    }
    j = -1;
    span = -1;
    start = lastothtile + 1;
    for (i = 0; i < SIZE(substitutes); i++) {
        if (i == 0 || substitutes[i].first_glyph != substitutes[j].first_glyph
            || substitutes[i].last_glyph != substitutes[j].last_glyph) {
            if (i != 0) { /* finish previous span */
                Fprintf(ofp, "%s} else {\n", Dent);
                Fprintf(ofp, "%s%sfor (i = %d; i <= %d; i++)\n", Dent, Dent,
                        substitutes[j].first_glyph, substitutes[j].last_glyph);
                Fprintf(ofp, "%s%s%sglyph2tile[i] = std_tiles%d[i - %d];\n",
                        Dent, Dent, Dent, span, substitutes[j].first_glyph);
                Fprintf(ofp, "%s}\n\n", Dent);
            }
            j = i;
            span++;
        }
        Fprintf(ofp, "%s%sif (%s) {\n", Dent, (i == j) ? "" : "} else ",
                substitutes[i].level_test);
        Fprintf(ofp, "%s%sfor (i = %d; i <= %d; i++)\n", Dent, Dent,
                substitutes[i].first_glyph, substitutes[i].last_glyph);
        Fprintf(ofp, "%s%s%sglyph2tile[i] = %d + i - %d;\n",
                Dent, Dent, Dent, start, substitutes[i].first_glyph);
        start += substitutes[i].last_glyph - substitutes[i].first_glyph + 1;
    }
    /* finish last span */
    Fprintf(ofp, "%s} else {\n", Dent);
    Fprintf(ofp, "%s%sfor (i = %d; i <= %d; i++)\n", Dent, Dent,
            substitutes[j].first_glyph, substitutes[j].last_glyph);
    Fprintf(ofp, "%s%s%sglyph2tile[i] = std_tiles%d[i - %d];\n",
            Dent, Dent, Dent, span, substitutes[j].first_glyph);
    Fprintf(ofp, "%s}\n", Dent);

    for (i = 0; i < SIZE(epilog); i++) {
        Fprintf(ofp, "%s\n", epilog[i]);
    }

    lastothtile = start - 1;
#ifndef STATUES_DONT_LOOK_LIKE_MONSTERS
    start = laststatuetile + 1;
#endif
    Fprintf(ofp, "\nint total_tiles_used = %d;\n", start);
}

#ifdef OBTAIN_TILEMAP
extern void monst_globals_init(void);
extern void objects_globals_init(void);
#endif

DISABLE_WARNING_UNREACHABLE_CODE

int
main(int argc UNUSED, char *argv[] UNUSED)
{
    register int i;
    char filename[30];
    FILE *ofp;

#ifdef OBTAIN_TILEMAP
    objects_globals_init();
    monst_globals_init();
#endif

    init_tilemap();

    /*
     * create the source file, "tile.c"
     */
    Sprintf(filename, SOURCE_TEMPLATE, TILE_FILE);
    if (!(ofp = fopen(filename, "w"))) {
        perror(filename);
        exit(EXIT_FAILURE);
    }
    Fprintf(ofp,
            "/* This file is automatically generated.  Do not edit. */\n");
    Fprintf(ofp, "\n#include \"hack.h\"\n");
    Fprintf(ofp, "\nshort glyph2tile[MAX_GLYPH] = {\n");

    for (i = 0; i < MAX_GLYPH; i++) {
        Fprintf(ofp, " %4d,", tilemap[i].tilenum);
        if ((i % 12) == 11 || i == MAX_GLYPH - 1)
            Fprintf(ofp, "\n");
    }
    Fprintf(ofp, "};\n");

    process_substitutions(ofp);

    Fprintf(ofp, "\n#define MAXMONTILE %d\n", lastmontile);
    Fprintf(ofp, "#define MAXOBJTILE %d\n", lastobjtile);
    Fprintf(ofp, "#define MAXOTHTILE %d\n", lastothtile);
#ifndef STATUES_DONT_LOOK_LIKE_MONSTERS
    Fprintf(ofp, "/* #define MAXSTATUETILE %d */\n", laststatuetile);
#endif
    Fprintf(ofp, "\n/*tile.c*/\n");

    (void) fclose(ofp);
    exit(EXIT_SUCCESS);
    /*NOTREACHED*/
    return 0;
}

RESTORE_WARNINGS

#endif /* TILETEXT */

struct {
    int idx;
    const char *betterlabel;
    const char *expectedlabel;
} altlabels[] = {
{S_stone,    "dark part of a room", "dark part of a room"},
{S_vwall,    "vertical wall", "wall"},
{S_hwall,    "horizontal wall", "wall"},
{S_tlcorn,   "top left corner wall", "wall"},
{S_trcorn,   "top right corner wall", "wall"},
{S_blcorn,   "bottom left corner wall", "wall"},
{S_brcorn,   "bottom right corner wall", "wall"},
{S_crwall,   "cross wall", "wall"},
{S_tuwall,   "tuwall", "wall"},
{S_tdwall,   "tdwall", "wall"},
{S_tlwall,   "tlwall", "wall"},
{S_trwall,   "trwall", "wall"},
{S_ndoor,    "no door", "doorway"},
{S_vodoor,   "vertical open door", "open door"},
{S_hodoor,   "horizontal open door", "open door"},
{S_vcdoor,   "vertical closed door", "closed door"},
{S_hcdoor,   "horizontal closed door", "closed door"},
{S_bars,     "iron bars", "iron bars"},
{S_tree,     "tree", "tree"},
{S_room,     "room", "floor of a room"},
{S_darkroom, "darkroom", "dark part of a room"},
{S_corr,     "corridor", "corridor"},
{S_litcorr,  "lit corridor", "lit corridor"},
{S_upstair,  "up stairs", "staircase up"},
{S_dnstair,  "down stairs", "staircase down"},
{S_upladder, "up ladder", "ladder up"},
{S_dnladder, "down ladder", "ladder down"},
{S_altar,    "altar", "altar"},
{S_grave,    "grave", "grave"},
{S_throne,   "throne", "opulent throne"},
{S_sink,     "sink", "sink"},
{S_fountain, "fountain", "fountain"},
{S_pool,     "pool", "water"},
{S_ice,      "ice", "ice"},
{S_lava,     "lava", "molten lava"},
{S_vodbridge, "vertical open drawbridge", "lowered drawbridge"},
{S_hodbridge, "horizontal open drawbridge", "lowered drawbridge"},
{S_vcdbridge, "vertical closed drawbridge", "raised drawbridge"},
{S_hcdbridge, "horizontal closed drawbridge", "raised drawbridge"},
{S_air,      "air", "air"},
{S_cloud,    "cloud", "cloud"},
{S_water,    "water", "water"},
{S_arrow_trap,           "arrow trap", "arrow trap"},
{S_dart_trap,            "dart trap", "dart trap"},
{S_falling_rock_trap,    "falling rock trap", "falling rock trap"},
{S_squeaky_board,        "squeaky board", "squeaky board"},
{S_bear_trap,            "bear trap", "bear trap"},
{S_land_mine,            "land mine", "land mine"},
{S_rolling_boulder_trap, "rolling boulder trap", "rolling boulder trap"},
{S_sleeping_gas_trap,    "sleeping gas trap", "sleeping gas trap"},
{S_rust_trap,            "rust trap", "rust trap"},
{S_fire_trap,            "fire trap", "fire trap"},
{S_pit,                  "pit", "pit"},
{S_spiked_pit,           "spiked pit", "spiked pit"},
{S_hole,                 "hole", "hole"},
{S_trap_door,            "trap door", "trap door"},
{S_teleportation_trap,   "teleportation trap", "teleportation trap"},
{S_level_teleporter,     "level teleporter", "level teleporter"},
{S_magic_portal,         "magic portal", "magic portal"},
{S_web,                  "web", "web"},
{S_statue_trap,          "statue trap", "statue trap"},
{S_magic_trap,           "magic trap", "magic trap"},
{S_anti_magic_trap,      "anti magic trap", "anti-magic field"},
{S_polymorph_trap,       "polymorph trap", "polymorph trap"},
{S_vibrating_square,     "vibrating square", "vibrating square"},
{S_vbeam,    "vertical beam", "cmap 65"},
{S_hbeam,    "horizontal beam", "cmap 66"},
{S_lslant,   "left slant beam", "cmap 67"},
{S_rslant,   "right slant beam", "cmap 68"},
{S_digbeam,  "dig beam", "cmap 69"},
{S_flashbeam, "flash beam", "cmap 70"},
{S_boomleft, "boom left", "cmap 71"},
{S_boomright, "boom right", "cmap 72"},
{S_ss1,      "shield1", "cmap 73"},
{S_ss2,      "shield2", "cmap 74"},
{S_ss3,      "shield3", "cmap 75"},
{S_ss4,      "shield4", "cmap 76"},
{S_poisoncloud, "poison cloud", "poison cloud"},
{S_goodpos,  "valid position", "valid position"},
{S_sw_tl,    "swallow top left", "cmap 79"},
{S_sw_tc,    "swallow top center", "cmap 80"},
{S_sw_tr,    "swallow top right", "cmap 81"},
{S_sw_ml,    "swallow middle left", "cmap 82"},
{S_sw_mr,    "swallow middle right", "cmap 83"},
{S_sw_bl,    "swallow bottom left ", "cmap 84"},
{S_sw_bc,    "swallow bottom center", "cmap 85"},
{S_sw_br,    "swallow bottom right", "cmap 86"},
{S_explode1, "explosion top left", "explosion dark 0"},
{S_explode2, "explosion top centre", "explosion dark 1"},
{S_explode3, "explosion top right", "explosion dark 2"},
{S_explode4, "explosion middle left", "explosion dark 3"},
{S_explode5, "explosion middle center", "explosion dark 4"},
{S_explode6, "explosion middle right", "explosion dark 5"},
{S_explode7, "explosion bottom left", "explosion dark 6"},
{S_explode8, "explosion bottom center", "explosion dark 7"},
{S_explode9, "explosion bottom right", "explosion dark 8"},
};

boolean
acceptable_tilename(int glyph_set, int idx, const char *encountered,
                    const char *expected)
{
    if (glyph_set == OTH_GLYPH) {
        if (idx >= 0 && idx < SIZE(altlabels)) {
            if (!strcmp(altlabels[idx].expectedlabel, expected)) {
                if (!strcmp(altlabels[idx].betterlabel, encountered))
                    return TRUE;
            }
        }
        return FALSE;
    }
    return TRUE;
}

/*tilemap.c*/
