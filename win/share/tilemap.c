/*	SCCS Id: @(#)tilemap.c	3.4	2000/06/04	*/
/* NetHack may be freely redistributed.  See license for details. */

/*
 *	This source file is compiled twice:
 *	once without TILETEXT defined to make tilemap.{o,obj},
 *	then again with it defined to produce tiletxt.{o,obj}.
 */

#include "hack.h"

const char * FDECL(tilename, (int, int));
void NDECL(init_tilemap);
void FDECL(process_substitutions, (FILE *));

#if defined(MICRO) || defined(WIN32)
#undef exit
#if !defined(MSDOS) && !defined(WIN32)
extern void FDECL(exit, (int));
#endif
#endif

#define MON_GLYPH 1
#define OBJ_GLYPH 2
#define OTH_GLYPH 3	/* fortunately unnecessary */

/* note that the ifdefs here should be the opposite sense from monst.c/
 * objects.c/rm.h
 */

struct conditionals {
	int sequence, predecessor;
	const char *name;
} conditionals[] = {
#ifndef CHARON /* not supported yet */
	{ MON_GLYPH, PM_HELL_HOUND, "Cerberus" },
#endif
	/* commented out in monst.c at present */
	{ MON_GLYPH, PM_SHOCKING_SPHERE, "beholder" },
	{ MON_GLYPH, PM_BABY_SILVER_DRAGON, "baby shimmering dragon" },
	{ MON_GLYPH, PM_SILVER_DRAGON, "shimmering dragon" },
	{ MON_GLYPH, PM_JABBERWOCK, "vorpal jabberwock" },
#ifndef KOPS
	{ MON_GLYPH, PM_JABBERWOCK, "Keystone Kop" },
	{ MON_GLYPH, PM_JABBERWOCK, "Kop Sergeant" },
	{ MON_GLYPH, PM_JABBERWOCK, "Kop Lieutenant" },
	{ MON_GLYPH, PM_JABBERWOCK, "Kop Kaptain" },
#endif
	{ MON_GLYPH, PM_VAMPIRE_LORD, "vampire mage" },
#ifndef CHARON /* not supported yet */
	{ MON_GLYPH, PM_CROESUS, "Charon" },
#endif
#ifndef MAIL
	{ MON_GLYPH, PM_FAMINE, "mail daemon" },
#endif
#ifndef TOURIST
	{ MON_GLYPH, PM_SAMURAI, "tourist" },
#endif
	/* commented out in monst.c at present */
	{ MON_GLYPH, PM_SHAMAN_KARNOV, "Earendil" },
	{ MON_GLYPH, PM_SHAMAN_KARNOV, "Elwing" },
#ifndef TOURIST
	{ MON_GLYPH, PM_LORD_SATO, "Twoflower" },
#endif
	/* commented out in monst.c at present */
	{ MON_GLYPH, PM_CHROMATIC_DRAGON, "Goblin King" },
	{ MON_GLYPH, PM_NEANDERTHAL, "High-elf" },
#ifndef TOURIST
	{ MON_GLYPH, PM_ROSHI, "guide" },
#endif
#ifndef KOPS
	{ OBJ_GLYPH, CLUB, "rubber hose" },
#endif
	/* objects commented out in objects.c at present */
	{ OBJ_GLYPH, SILVER_DRAGON_SCALE_MAIL, "shimmering dragon scale mail" },
	{ OBJ_GLYPH, SILVER_DRAGON_SCALES, "shimmering dragon scales" },
#ifndef TOURIST
	{ OBJ_GLYPH, LEATHER_JACKET, "Hawaiian shirt" },
	{ OBJ_GLYPH, LEATHER_JACKET, "T-shirt" },
	{ OBJ_GLYPH, LOCK_PICK, "credit card" },
	{ OBJ_GLYPH, MAGIC_LAMP, "expensive camera" },
#endif
#ifndef STEED
	{ OBJ_GLYPH, TOWEL, "saddle" },
#endif
	/* allow slime mold to look like slice of pizza, since we
	 * don't know what a slime mold should look like when renamed anyway
	 */
#ifndef MAIL
	{ OBJ_GLYPH, SCR_STINKING_CLOUD+4, "stamped / mail" },
#endif
	{ 0, 0, 0}
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
	const char *sub_name;		/* for explanations */
	const char *level_test;
} substitutes[] = {
	{ GLYPH_CMAP_OFF + S_vwall, GLYPH_CMAP_OFF + S_trwall,
					"mine walls", "In_mines(plev)" },
	{ GLYPH_CMAP_OFF + S_vwall, GLYPH_CMAP_OFF + S_trwall,
					"gehennom walls", "In_hell(plev)" },
	{ GLYPH_CMAP_OFF + S_vwall, GLYPH_CMAP_OFF + S_trwall,
					"knox walls", "Is_knox(plev)" },
	{ GLYPH_CMAP_OFF + S_vwall, GLYPH_CMAP_OFF + S_trwall,
					"sokoban walls", "In_sokoban(plev)" }
};


#ifdef TILETEXT

/*
 * entry is the position of the tile within the monsters/objects/other set
 */
const char *
tilename(set, entry)
int set, entry;
{
	int i, j, condnum, tilenum;
	static char buf[BUFSZ];

	/* Note:  these initializers don't do anything except guarantee that
		we're linked properly.
	*/
	monst_init();
	objects_init();
	(void) def_char_to_objclass(']');

	condnum = tilenum = 0;

	for (i = 0; i < NUMMONS; i++) {
		if (set == MON_GLYPH && tilenum == entry)
			return mons[i].mname;
		tilenum++;
		while (conditionals[condnum].sequence == MON_GLYPH &&
			conditionals[condnum].predecessor == i) {
			if (set == MON_GLYPH && tilenum == entry)
				return conditionals[condnum].name;
			condnum++;
			tilenum++;
		}
	}
	if (set == MON_GLYPH && tilenum == entry)
		return "invisible monster";

	tilenum = 0;	/* set-relative number */
	for (i = 0; i < NUM_OBJECTS; i++) {
		/* prefer to give the description - that's all the tile's
		 * appearance should reveal */
		if (set == OBJ_GLYPH && tilenum == entry) {
			if ( !obj_descr[i].oc_descr )
			    return obj_descr[i].oc_name;
			if ( !obj_descr[i].oc_name )
			    return obj_descr[i].oc_descr;

			Sprintf(buf, "%s / %s",
				obj_descr[i].oc_descr,
				obj_descr[i].oc_name);
			return buf;
		}

		tilenum++;
		while (conditionals[condnum].sequence == OBJ_GLYPH &&
			conditionals[condnum].predecessor == i) {
			if (set == OBJ_GLYPH && tilenum == entry)
				return conditionals[condnum].name;
			condnum++;
			tilenum++;
		}
	}

	tilenum = 0;	/* set-relative number */
	for (i = 0; i < (MAXPCHARS - MAXEXPCHARS); i++) {
		if (set == OTH_GLYPH && tilenum == entry) {
			if (*defsyms[i].explanation)
				return defsyms[i].explanation;
			else {
				/* if SINKS are turned off, this
				 * string won't be there (and can't be there
				 * to prevent symbol-identification and
				 * special-level mimic appearances from
				 * thinking the items exist)
				 */
				switch (i) {
				    case S_sink:
					    Sprintf(buf, "sink");
					    break;
				    default:
					    Sprintf(buf, "cmap %d", tilenum);
					    break;
				}
				return buf;
			}
		}
		tilenum++;
		while (conditionals[condnum].sequence == OTH_GLYPH &&
			conditionals[condnum].predecessor == i) {
			if (set == OTH_GLYPH && tilenum == entry)
				return conditionals[condnum].name;
			condnum++;
			tilenum++;
		}
	}
	/* explosions */
	tilenum = MAXPCHARS - MAXEXPCHARS;
	i = entry - tilenum;
	if (i < (MAXEXPCHARS * EXPL_MAX)) {
	    if (set == OTH_GLYPH) {
		static char *explosion_types[] = { /* hack.h */
			"dark", "noxious", "muddy", "wet",
			"magical", "fiery", "frosty"
		};
		Sprintf(buf, "explosion %s %d",
			explosion_types[i / MAXEXPCHARS], i % MAXEXPCHARS);
		return buf;
	    }
	}
	tilenum += (MAXEXPCHARS * EXPL_MAX);

	i = entry - tilenum;
	if (i < (NUM_ZAP << 2)) {
		if (set == OTH_GLYPH) {
			Sprintf(buf, "zap %d %d", i/4, i%4);
			return buf;
		}
	}
	tilenum += (NUM_ZAP << 2);

	i = entry - tilenum;
	if (i < WARNCOUNT) {
		if (set == OTH_GLYPH) {
			Sprintf(buf, "warning %d", i);
			return buf;
	        }
	}
	tilenum += WARNCOUNT;

	for (i = 0; i < SIZE(substitutes); i++) {
	    j = entry - tilenum;
	    if (j <= substitutes[i].last_glyph - substitutes[i].first_glyph) {
		if (set == OTH_GLYPH) {
		    Sprintf(buf, "sub %s %d", substitutes[i].sub_name, j);
		    return buf;
		}
	    }
	    tilenum += substitutes[i].last_glyph
				- substitutes[i].first_glyph + 1;
	}

	Sprintf(buf, "unknown %d %d", set, entry);
	return buf;
}

#else	/* TILETEXT */

#define TILE_FILE	"tile.c"

#ifdef AMIGA
# define SOURCE_TEMPLATE	"NH:src/%s"
#else
# ifdef MAC
#   define SOURCE_TEMPLATE	":src:%s"
# else
#   define SOURCE_TEMPLATE	"../src/%s"
# endif
#endif

short tilemap[MAX_GLYPH];
int lastmontile, lastobjtile, lastothtile;

/* Number of tiles for invisible monsters */
#define NUM_INVIS_TILES 1

/*
 * set up array to map glyph numbers to tile numbers
 *
 * assumes tiles are numbered sequentially through monsters/objects/other,
 * with entries for all supported compilation options
 *
 * "other" contains cmap and zaps (the swallow sets are a repeated portion
 * of cmap), as well as the "flash" glyphs for the new warning system
 * introduced in 3.3.1.
 */
void
init_tilemap()
{
	int i, j, condnum, tilenum;
	int corpsetile, swallowbase;

	for (i = 0; i < MAX_GLYPH; i++) {
		tilemap[i] = -1;
	}

	corpsetile = NUMMONS + NUM_INVIS_TILES + CORPSE;
	swallowbase= NUMMONS + NUM_INVIS_TILES + NUM_OBJECTS + S_sw_tl;

	/* add number compiled out */
	for (i = 0; conditionals[i].sequence; i++) {
		switch (conditionals[i].sequence) {
			case MON_GLYPH:
				corpsetile++;
				swallowbase++;
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

	condnum = tilenum = 0;
	for (i = 0; i < NUMMONS; i++) {
		tilemap[GLYPH_MON_OFF+i] = tilenum;
		tilemap[GLYPH_PET_OFF+i] = tilenum;
		tilemap[GLYPH_DETECT_OFF+i] = tilenum;
		tilemap[GLYPH_RIDDEN_OFF+i] = tilenum;
		tilemap[GLYPH_BODY_OFF+i] = corpsetile;
		j = GLYPH_SWALLOW_OFF + 8*i;
		tilemap[j] = swallowbase;
		tilemap[j+1] = swallowbase+1;
		tilemap[j+2] = swallowbase+2;
		tilemap[j+3] = swallowbase+3;
		tilemap[j+4] = swallowbase+4;
		tilemap[j+5] = swallowbase+5;
		tilemap[j+6] = swallowbase+6;
		tilemap[j+7] = swallowbase+7;
		tilenum++;
		while (conditionals[condnum].sequence == MON_GLYPH &&
			conditionals[condnum].predecessor == i) {
			condnum++;
			tilenum++;
		}
	}
	tilemap[GLYPH_INVISIBLE] = tilenum++;
	lastmontile = tilenum - 1;

	for (i = 0; i < NUM_OBJECTS; i++) {
		tilemap[GLYPH_OBJ_OFF+i] = tilenum;
		tilenum++;
		while (conditionals[condnum].sequence == OBJ_GLYPH &&
			conditionals[condnum].predecessor == i) {
			condnum++;
			tilenum++;
		}
	}
	lastobjtile = tilenum - 1;

	for (i = 0; i < (MAXPCHARS - MAXEXPCHARS); i++) {
		tilemap[GLYPH_CMAP_OFF+i] = tilenum;
		tilenum++;
		while (conditionals[condnum].sequence == OTH_GLYPH &&
			conditionals[condnum].predecessor == i) {
			condnum++;
			tilenum++;
		}
	}

	for (i = 0; i < (MAXEXPCHARS * EXPL_MAX); i++) {
		tilemap[GLYPH_EXPLODE_OFF+i] = tilenum;
		tilenum++;
		while (conditionals[condnum].sequence == OTH_GLYPH &&
			conditionals[condnum].predecessor == (i + MAXPCHARS)) {
			condnum++;
			tilenum++;
		}
	}

	for (i = 0; i < NUM_ZAP << 2; i++) {
		tilemap[GLYPH_ZAP_OFF+i] = tilenum;
		tilenum++;
		while (conditionals[condnum].sequence == OTH_GLYPH &&
			conditionals[condnum].predecessor == (i + MAXEXPCHARS)) {
			condnum++;
			tilenum++;
		}
	}

	for (i = 0; i < WARNCOUNT; i++) {
		tilemap[GLYPH_WARNING_OFF+i] = tilenum;
		tilenum++;
	}

	lastothtile = tilenum - 1;
}

const char *prolog[] = {
	"",
	"",
	"void",
	"substitute_tiles(plev)",
	"d_level *plev;",
	"{",
	"\tint i;",
	""
};

const char *epilog[] = {
	"}"
};

/* write out the substitutions in an easily-used form. */
void
process_substitutions(ofp)
FILE *ofp;
{
	int i, j, k, span, start;

	fprintf(ofp, "\n\n");

	j = 0;	/* unnecessary */
	span = -1;
	for (i = 0; i < SIZE(substitutes); i++) {
	    if (i == 0
		|| substitutes[i].first_glyph != substitutes[j].first_glyph
		|| substitutes[i].last_glyph != substitutes[j].last_glyph) {
			j = i;
			span++;
			fprintf(ofp, "short std_tiles%d[] = { ", span);
			for (k = substitutes[i].first_glyph;
				k < substitutes[i].last_glyph; k++)
					fprintf(ofp, "%d, ", tilemap[k]);
			fprintf(ofp, "%d };\n",
				tilemap[substitutes[i].last_glyph]);
	    }
	}

	for (i = 0; i < SIZE(prolog); i++) {
		fprintf(ofp, "%s\n", prolog[i]);
	}
	j = -1;
	span = -1;
	start = lastothtile + 1;
	for (i = 0; i < SIZE(substitutes); i++) {
	    if (i == 0
		    || substitutes[i].first_glyph != substitutes[j].first_glyph
		    || substitutes[i].last_glyph != substitutes[j].last_glyph) {
		if (i != 0) {	/* finish previous span */
		    fprintf(ofp, "\t} else {\n");
		    fprintf(ofp, "\t\tfor (i = %d; i <= %d; i++)\n",
					substitutes[j].first_glyph,
					substitutes[j].last_glyph);
		    fprintf(ofp, "\t\t\tglyph2tile[i] = std_tiles%d[i - %d];\n",
					span, substitutes[j].first_glyph);
		    fprintf(ofp, "\t}\n\n");
		}
		j = i;
		span++;
	    }
	    if (i != j) fprintf(ofp, "\t} else ");
	    fprintf(ofp, "\tif (%s) {\n", substitutes[i].level_test);
	    fprintf(ofp, "\t\tfor (i = %d; i <= %d; i++)\n",
				substitutes[i].first_glyph,
				substitutes[i].last_glyph);
	    fprintf(ofp, "\t\t\tglyph2tile[i] = %d + i - %d;\n",
				start, substitutes[i].first_glyph);
	    start += substitutes[i].last_glyph - substitutes[i].first_glyph + 1;
	}
	/* finish last span */
	fprintf(ofp, "\t} else {\n");
	fprintf(ofp, "\t\tfor (i = %d; i <= %d; i++)\n",
			    substitutes[j].first_glyph,
			    substitutes[j].last_glyph);
	fprintf(ofp, "\t\t\tglyph2tile[i] = std_tiles%d[i - %d];\n",
			    span, substitutes[j].first_glyph);
	fprintf(ofp, "\t}\n\n");

	for (i = 0; i < SIZE(epilog); i++) {
		fprintf(ofp, "%s\n", epilog[i]);
	}

	fprintf(ofp, "\nint total_tiles_used = %d;\n", start);
	lastothtile = start - 1;
}

int main()
{
    register int i;
    char filename[30];
    FILE *ofp;

    init_tilemap();

    /*
     * create the source file, "tile.c"
     */
    Sprintf(filename, SOURCE_TEMPLATE, TILE_FILE);
    if (!(ofp = fopen(filename, "w"))) {
	    perror(filename);
	    exit(EXIT_FAILURE);
    }
    fprintf(ofp,"/* This file is automatically generated.  Do not edit. */\n");
    fprintf(ofp,"\n#include \"hack.h\"\n\n");
    fprintf(ofp,"short glyph2tile[MAX_GLYPH] = {\n");

    for (i = 0; i < MAX_GLYPH; i++) {
	fprintf(ofp,"%2d,%c", tilemap[i], (i % 12) ? ' ' : '\n');
    }
    fprintf(ofp,"%s};\n", (i % 12) ? "\n" : "");

    process_substitutions(ofp);

    fprintf(ofp,"\n#define MAXMONTILE %d\n", lastmontile);
    fprintf(ofp,"#define MAXOBJTILE %d\n", lastobjtile);
    fprintf(ofp,"#define MAXOTHTILE %d\n", lastothtile);

    fprintf(ofp,"\n/*tile.c*/\n");

    fclose(ofp);
    exit(EXIT_SUCCESS);
    /*NOTREACHED*/
    return 0;
}

#endif	/* TILETEXT */
