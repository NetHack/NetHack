/* NetHack 3.7	utf8map.c	*/
/* Copyright (c) Michael Allison, 2021. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include <ctype.h>

extern const struct symparse loadsyms[];
extern struct enum_dump monsdump[];
extern struct enum_dump objdump[];
extern glyph_map glyphmap[MAX_GLYPH];
extern const char *const known_handling[];        /* symbols.c */

#ifdef ENHANCED_SYMBOLS

#define Fprintf (void) fprintf
enum reserved_activities { res_nothing, res_dump_glyphids, res_fill_cache };
enum things_to_find { find_nothing, find_pm, find_oc, find_cmap, find_glyph };
struct find_struct {
    enum things_to_find findtype;
    int val;
    int loadsyms_offset;
    int loadsyms_count;
    int *extraval;
    long color;
    const char *unicode_val; /* U+NNNN format */
    void (*callback)(int glyph, struct find_struct *);
    enum reserved_activities restype;
    genericptr_t reserved;
};
const struct find_struct zero_find = { 0 };
struct glyphid_cache_t {
    int glyphnum;
    char *id;
};
static struct glyphid_cache_t *glyphid_cache;
static unsigned glyphid_cache_lsize;
static size_t glyphid_cache_size;
struct find_struct glyphcache_find, to_custom_symbol_find;
static void init_glyph_cache(void);
static void add_glyph_to_cache(int glyphnum, const char *id);
static int find_glyph_in_cache(const char *id);
static uint32 glyph_hash(const char *id);
static void to_custom_symset_entry_callback(int glyph,
                                            struct find_struct *findwhat);
static int unicode_val(const char *cp);
static int parse_id(const char *id, struct find_struct *findwhat);
static int glyph_find_core(const char *id, struct find_struct *findwhat);
static char *fix_glyphname(char *str);
static int32_t rgbstr_to_int32(const char *rgbstr);
boolean closest_color(uint32_t lcolor, uint32_t *closecolor, int *clridx);

static void
to_custom_symset_entry_callback(int glyph, struct find_struct *findwhat)
{
#ifdef NO_PARSING_SYMSET
    glyph_map *gmap = &glyphmap[glyph];
#endif
    uint8 utf8str[6] = { 0, 0, 0, 0, 0, 0 };
    int uval;

    if (!findwhat->unicode_val)
        return;
    if (findwhat->extraval)
        *findwhat->extraval = glyph;
    uval = unicode_val(findwhat->unicode_val);
    if (unicodeval_to_utf8str(uval, utf8str, sizeof utf8str)) {
#ifdef NO_PARSING_SYMSET
        set_map_u(gmap, uval, utf8str,
                  (findwhat->color != 0L) ? findwhat->color : 0L);
#endif
        add_custom_urep_entry(known_handling[H_UTF8], glyph,
                              uval, utf8str, findwhat->color,
                              gs.symset_which_set);
    }
}

/*
 * Return value:
 *               1 = success
 *               0 = failure
 */
int
glyphrep_to_custom_map_entries(const char *op, int *glyphptr)
{
    to_custom_symbol_find = zero_find;
    char buf[BUFSZ], *c_glyphid, *c_unicode, *c_rgb, *cp;
    int milestone, reslt = 0;
    long rgb;
    boolean slash = FALSE;

    if (!glyphid_cache)
        reslt = 1; /* for debugger use only; no cache available */

    milestone = 0;
    Snprintf(buf, sizeof buf, "%s", op);
    c_unicode = c_rgb = (char *) 0;
    c_glyphid = cp = buf;
    while (*cp) {
        if ((*cp == '/') || (*cp == ':')) {
            *cp = '\0';
            milestone++;
            slash = TRUE;
        }
        cp++;
        if (slash) {
            if (milestone < 2)
                c_unicode = cp;
            else
                c_rgb = cp;
            slash = FALSE;
        }
    }
    /* some sanity checks */
    if (c_glyphid && *c_glyphid == ' ')
        c_glyphid++;
    if (c_unicode && *c_unicode == ' ')
        c_unicode++;
    if (c_rgb && *c_rgb == ' ')
        c_rgb++;
    if (c_unicode && (*c_unicode == 'U' || *c_unicode == 'u')
        && (c_unicode[1] == '+')) {
        /* unicode = unicode_val(c_unicode); */
        if ((rgb = rgbstr_to_int32(c_rgb)) != -1L || !c_rgb) {
            to_custom_symbol_find.unicode_val = c_unicode;
            /* if the color 0 is an actual color, as opposed to just "not set"
               we set a marker bit outside the 24-bit range to indicate a
               valid color value 0. That allows valid color 0, but allows a
               simple checking for 0 to detect "not set". The window port that
               implements the color switch, needs to either check that bit
               or appropriately mask colors with 0xFFFFFF. */
            to_custom_symbol_find.color = (rgb == -1 || !c_rgb)   ? 0L
                                          : (rgb == 0L) ? (0 & 0x1000000)
                                                        : rgb;
            to_custom_symbol_find.extraval = glyphptr;
            to_custom_symbol_find.callback = to_custom_symset_entry_callback;
            reslt = glyph_find_core(c_glyphid, &to_custom_symbol_find);
            return reslt;
        }
    }
    return 0;
}

static int32_t
rgbstr_to_int32(const char *rgbstr)
{
    int r, g, b, milestone = 0;
    char *cp, *c_r,*c_g,*c_b;
    int32_t rgb = 0;
    char buf[BUFSZ];
    boolean dash = FALSE;

    r = g = b = 0;
    c_g = c_b = (char *) 0;
    Snprintf(buf, sizeof buf, "%s", rgbstr);
    c_r = cp = buf;
    while (*cp) {
        if (digit(*cp) || *cp == '-') {
            if (*cp == '-') {
                *cp = '\0';
                milestone++;
                dash = TRUE;
            }
            cp++;
            if (dash) {
                if (milestone < 2)
                    c_g = cp;
                else
                    c_b = cp;
                dash = FALSE;
            }
        } else {
            return -1L;
        }
    }
    /* sanity checks */
    if (c_r && c_g && c_b
            && (strlen(c_r) > 0 && strlen(c_r) < 4)
            && (strlen(c_g) > 0 && strlen(c_g) < 4)
            && (strlen(c_b) > 0 && strlen(c_b) < 4)) {
        r = atoi(c_r);
        g = atoi(c_g);
        b = atoi(c_b);
        rgb = (r << 16) | (g << 8) | (b << 0);
        return rgb;
    }
    return -1L;
}

static char *
fix_glyphname(char *str)
{
    char *c;

    for (c = str; *c; c++) {
        if (*c >= 'A' && *c <= 'Z')
            *c += (char) ('a' - 'A');
        else if (*c >= '0' && *c <= '9')
            ;
        else if (*c < 'a' || *c > 'z')
            *c = '_';
    }
    return str;
}

static int
unicode_val(const char *cp)
{
    const char *dp;
    int cval = 0, dcount, unicode = 0;
    static const char hex[] = "00112233445566778899aAbBcCdDeEfF";

    if (cp && *cp) {
        cval = dcount = 0;
        if ((unicode = ((*cp == 'U' || *cp == 'u') && cp[1] == '+')) && cp[2]
            && (dp = strchr(hex, cp[2])) != 0) {
            cp += 2; /* move past the 'U' and '+' */
            do {
                cval = (cval * 16) + ((int) (dp - hex) / 2);
            } while (*++cp && (dp = strchr(hex, *cp)) != 0 && ++dcount < 7);
        }
    }
    return cval;
}

int
set_map_u(glyph_map *gmap, uint32 utf32ch, const uint8 *utf8str, long ucolor)
{
    static uint32_t closecolor = 0;
    static int clridx = 0;

    if (gmap) {
        if (gmap->u == 0) {
            gmap->u = (struct unicode_representation *) alloc(sizeof *gmap->u);
            gmap->u->utf8str = 0;
        }
        if (gmap->u->utf8str != 0) {
            free(gmap->u->utf8str);
            gmap->u->utf8str = 0;
        }
        gmap->u->utf8str = (uint8 *) dupstr((const char *) utf8str);
        gmap->u->ucolor = ucolor;
        if (closest_color(ucolor, &closecolor, &clridx))
            gmap->u->u256coloridx = clridx;
        else
            gmap->u->u256coloridx = 0;
        gmap->u->utf32ch = utf32ch;
        return 1;
    }
    return 0;
}


static int
glyph_find_core(const char *id, struct find_struct *findwhat)
{
    int glyph;
    boolean do_callback, end_find = FALSE;

    if (parse_id(id, findwhat)) {
        if (findwhat->findtype == find_glyph) {
            (findwhat->callback)(findwhat->val, findwhat);
        } else {
            for (glyph = 0; glyph < MAX_GLYPH; ++glyph) {
                do_callback = FALSE;
                switch (findwhat->findtype) {
                case find_cmap:
                    if (glyph_to_cmap(glyph) == findwhat->val)
                        do_callback = TRUE;
                    break;
                case find_pm:
                    if (glyph_is_monster(glyph)
                        && mons[glyph_to_mon(glyph)].mlet == findwhat->val)
                        do_callback = TRUE;
                    break;
                case find_oc:
                    if (glyph_is_object(glyph)
                        && glyph_to_obj(glyph) == findwhat->val)
                        do_callback = TRUE;
                    break;
                case find_glyph:
                    if (glyph == findwhat->val) {
                        do_callback = TRUE;
                        end_find = TRUE;
                    }
                    break;
                case find_nothing:
                default:
                    end_find = TRUE;
                    break;
                }
                if (do_callback)
                    (findwhat->callback)(glyph, findwhat);
                if (end_find)
                    break;
            }
        }
        return 1;
    }
    return 0;
}

/*
 When we start to process a config file or a symbol file,
 that might have G_ entries, generating all 9000+ glyphid
 for comparison repeatedly each time we encounter a G_
 entry to decipher, then comparing against them, is obviously
 extremely performance-poor.

 Setting aside the "comparison" part for now (that has to be
 done in some manner), we can likely do something about the
 repeated "generation" of the names for parsing prior to the
 actual comparison part by generating them once, ahead of the
 bulk of the potential parsings. We can later free up
 all the memory those names consumed once the bulk parsing is
 overwith.
*/


void fill_glyphid_cache(void)
{
    int reslt = 0;

    if (!glyphid_cache) {
        init_glyph_cache();
    }
    if (glyphid_cache) {
        glyphcache_find = zero_find;
        glyphcache_find.findtype = find_nothing;
        glyphcache_find.reserved = (genericptr_t) glyphid_cache;
        glyphcache_find.restype = res_fill_cache;
        reslt = parse_id((char *) 0, &glyphcache_find);
        if (!reslt) {
            free_glyphid_cache();
            glyphid_cache = (struct glyphid_cache_t *) 0;
        }
    }
}

/*
 * The glyph ID cache is a simple double-hash table.
 * The cache size is a power of two, and two hashes are derived from the
 * cache ID. The first is a location in the table, and the second is an
 * offset. On any collision, the second hash is added to the first until
 * a match or an empty bucket is found.
 * The second hash is an odd number, which is necessary and sufficient
 * to traverse the entire table.
 */

static void
init_glyph_cache(void)
{
    size_t glyph;

    /* Cache size of power of 2 not less than 2*MAX_GLYPH */
    glyphid_cache_lsize = 0;
    glyphid_cache_size = 1;
    while (glyphid_cache_size < 2*MAX_GLYPH) {
        ++glyphid_cache_lsize;
        glyphid_cache_size <<= 1;
    }

    glyphid_cache = (struct glyphid_cache_t *) alloc(
        glyphid_cache_size * sizeof(struct glyphid_cache_t));
    for (glyph = 0; glyph < glyphid_cache_size; ++glyph) {
        glyphid_cache[glyph].glyphnum = 0;
        glyphid_cache[glyph].id = (char *) 0;
    }
}

void free_glyphid_cache(void)
{
    size_t idx;

    if (!glyphid_cache)
        return;
    for (idx = 0; idx < glyphid_cache_size; ++idx) {
        if (glyphid_cache[idx].id) {
            free(glyphid_cache[idx].id);
            glyphid_cache[idx].id = (char *) 0;
        }
    }
    free(glyphid_cache);
    glyphid_cache = (struct glyphid_cache_t *) 0;
}

static void
add_glyph_to_cache(int glyphnum, const char *id)
{
    uint32 hash = glyph_hash(id);
    size_t hash1 = (size_t) (hash & (glyphid_cache_size - 1));
    size_t hash2 = (size_t)
            (((hash >> glyphid_cache_lsize) & (glyphid_cache_size - 1)) | 1);
    size_t i = hash1;

    do {
        if (glyphid_cache[i].id == NULL) {
            /* Empty bucket found */
            glyphid_cache[i].id = dupstr(id);
            glyphid_cache[i].glyphnum = glyphnum;
            return;
        }
        /* For speed, assume that no ID occurs twice */
        i = (i + hash2) & (glyphid_cache_size - 1);
    } while (i != hash1);
    /* This should never happen */
    panic("glyphid_cache full");
}

static int
find_glyph_in_cache(const char *id)
{
    uint32 hash = glyph_hash(id);
    size_t hash1 = (size_t) (hash & (glyphid_cache_size - 1));
    size_t hash2 = (size_t)
            (((hash >> glyphid_cache_lsize) & (glyphid_cache_size - 1)) | 1);
    size_t i = hash1;

    do {
        if (glyphid_cache[i].id == NULL) {
            /* Empty bucket found */
            return -1;
        }
        if (strcmpi(id, glyphid_cache[i].id) == 0) {
            /* Match found */
            return glyphid_cache[i].glyphnum;
        }
        i = (i + hash2) & (glyphid_cache_size - 1);
    } while (i != hash1);
    return -1;
}

static uint32
glyph_hash(const char *id)
{
    uint32 hash = 0;
    size_t i;

    for (i = 0; id[i] != '\0'; ++i) {
        char ch = id[i];
        if ('A' <= ch && ch <= 'Z') {
            ch += 'a' - 'A';
        }
        hash = (hash << 1) | (hash >> 31);
        hash ^= ch;
    }
    return hash;
}

boolean
glyphid_cache_status(void)
{
    return (glyphid_cache != 0);
}

void
free_all_glyphmap_u(void)
{
    int glyph;
    int x, y;

    for (glyph = 0; glyph < MAX_GLYPH; ++glyph) {
        if (glyphmap[glyph].u) {
            if (glyphmap[glyph].u->utf8str) {
                free(glyphmap[glyph].u->utf8str);
                glyphmap[glyph].u->utf8str = 0;
            }
            free(glyphmap[glyph].u);
            glyphmap[glyph].u = 0;
        }
    }
    /* Prevent use after free from gg.gbuf */
    for (y = 0; y < ROWNO; ++y) {
        for (x = 0; x < COLNO; ++x) {
            gg.gbuf[y][x].glyphinfo.gm.u = NULL;
        }
    }
}

/* helper routine if a window port wants to embed any UTF-8 sequences
   for the glyph representation in the string in place of the \GNNNNNNNN
   reference */
char *
mixed_to_utf8(char *buf, size_t bufsz, const char *str, int *retflags)
{
    char *put = buf;
    glyph_info glyphinfo = nul_glyphinfo;

    if (!str)
        return strcpy(buf, "");

    while (*str && put < (buf + bufsz) - 1) {
        if (*str == '\\') {
            int dcount, so, ggv;
            const char *save_str;

            save_str = str++;
            switch (*str) {
            case 'G': /* glyph value \GXXXXNNNN*/
                if ((dcount = decode_glyph(str + 1, &ggv))) {
                    str += (dcount + 1);
                    map_glyphinfo(0, 0, ggv, 0, &glyphinfo);
                    if (glyphinfo.gm.u && glyphinfo.gm.u->utf8str) {
                        uint8 *ucp = glyphinfo.gm.u->utf8str;

                        while (*ucp && put < (buf + bufsz) - 1)
                            *put++ = *ucp++;
                        if (retflags)
                            *retflags = 1;
                    } else {
                        so = glyphinfo.gm.sym.symidx;
                        *put++ = gs.showsyms[so];
                        if (retflags)
                            *retflags = 0;
                    }
                    /* 'str' is ready for the next loop iteration and
                        '*str' should not be copied at the end of this
                        iteration */
                    continue;
                } else {
                    /* possible forgery - leave it the way it is */
                    str = save_str;
                }
                break;
            case '\\':
                break;
            case '\0':
                /* String ended with '\\'.  This can happen when someone
                    names an object with a name ending with '\\', drops the
                    named object on the floor nearby and does a look at all
                    nearby objects. */
                /* brh - should we perhaps not allow things to have names
                    that contain '\\' */
                str = save_str;
                break;
            }
        }
        if (put < (buf + bufsz) - 1)
            *put++ = *str++;
    }
    *put = '\0';
    return buf;
}

void
dump_all_glyphids(FILE *fp)
{
    struct find_struct dump_glyphid_find = zero_find;

    dump_glyphid_find.findtype = find_nothing;
    dump_glyphid_find.reserved = (genericptr_t) fp;
    dump_glyphid_find.restype = res_dump_glyphids;
    (void) parse_id((char *) 0, &dump_glyphid_find);
}

int
match_glyph(char *buf)
{
    char workbuf[BUFSZ];

    /* buf contains a G_ glyph reference, not an S_ symbol.
        There could be an R-G-B color attached too.
        Let's get a copy to work with. */
    Snprintf(workbuf, sizeof workbuf, "%s", buf); /* get a copy */
    return glyphrep(workbuf);
}

int
glyphrep(const char *op)
{
    int reslt = 0, glyph = NO_GLYPH;
    if (!glyphid_cache)
        reslt = 1;      /* for debugger use only; no cache available */
    reslt = glyphrep_to_custom_map_entries(op, &glyph);
    if (reslt)
        return 1;
    return 0;
}

int
add_custom_urep_entry(
    const char *customization_name,
    int glyphidx,
    uint32 utf32ch,
    const uint8 *utf8str,
    long ucolor,
    enum graphics_sets which_set)
{
    static uint32_t closecolor = 0;
    static int clridx = 0;
    struct symset_customization *gdc = &gs.sym_customizations[which_set];
    struct customization_detail *details, *newdetails = 0;


    if (!gdc->details) {
        gdc->customization_name = dupstr(customization_name);
        gdc->custtype = custom_ureps;
        gdc->details = 0;
        gdc->details_end = 0;
    }
    details = find_matching_symset_customiz(customization_name,
                                            custom_symbols, which_set);
    if (details) {
        while (details) {
            if (details->content.urep.glyphidx == glyphidx) {
                if (details->content.urep.u.utf8str)
                    free(details->content.urep.u.utf8str);
                details->content.urep.u.utf8str =
                    (uint8 *) dupstr((const char *) utf8str);
                details->content.urep.u.ucolor = ucolor;
                if (closest_color(ucolor, &closecolor, &clridx))
                    details->content.urep.u.u256coloridx = clridx;
                else
                    details->content.urep.u.u256coloridx = 0;
                details->content.urep.u.utf32ch = utf32ch;
                return 1;
            }
            details = details->next;
        }
    }
    /* create new details entry */
    newdetails = (struct customization_detail *) alloc(
        sizeof(struct customization_detail));
    newdetails->content.urep.glyphidx = glyphidx;
    newdetails->content.urep.u.utf8str =
        (uint8 *) dupstr((const char *) utf8str);
    newdetails->content.urep.u.ucolor = ucolor;
    if (closest_color(ucolor, &closecolor, &clridx))
        newdetails->content.urep.u.u256coloridx = clridx;
    else
        newdetails->content.urep.u.u256coloridx = 0;
    newdetails->content.urep.u.utf32ch = utf32ch;
    newdetails->next = (struct customization_detail *) 0;
    if (gdc->details == NULL) {
        gdc->details = newdetails;
    } else {
        gdc->details_end->next = newdetails;
    }
    gdc->details_end = newdetails;
    gdc->count++;
    return 1;
}

static int
parse_id(const char *id, struct find_struct *findwhat)
{
    FILE *fp = (FILE *) 0;
    int i = 0, j, mnum, glyph, pm_offset = 0, oc_offset = 0, cmap_offset = 0,
        pm_count = 0, oc_count = 0, cmap_count = 0;
    boolean skip_base = FALSE, skip_this_one, dump_ids = FALSE,
            filling_cache = FALSE, is_S = FALSE, is_G = FALSE;
    char buf[4][QBUFSZ];

    if (findwhat->findtype == find_nothing && findwhat->restype) {
        if (findwhat->restype == res_dump_glyphids) {
            if (findwhat->reserved) {
                fp = (FILE *) findwhat->reserved;
                dump_ids = TRUE;
            } else {
                return 0;
            }
        }
        if (findwhat->restype == res_fill_cache) {
            if (findwhat->reserved
                && findwhat->reserved == (genericptr_t) glyphid_cache) {
                filling_cache = TRUE;
            } else {
                return 0;
            }
        }
    }

    is_G = (id && id[0] == 'G' && id[1] == '_');
    is_S = (id && id[0] == 'S' && id[1] == '_');

    if ((is_G && !glyphid_cache) || filling_cache || dump_ids || is_S) {
        while (loadsyms[i].range) {
            if (!pm_offset && loadsyms[i].range == SYM_MON)
                pm_offset = i;
            if (!pm_count && pm_offset && loadsyms[i].range != SYM_MON)
                pm_count = i - pm_offset;
            if (!oc_offset && loadsyms[i].range == SYM_OC)
                oc_offset = i;
            if (!oc_count && oc_offset && loadsyms[i].range != SYM_OC)
                oc_count = i - oc_offset;
            if (!cmap_offset && loadsyms[i].range == SYM_PCHAR)
                cmap_offset = i;
            if (!cmap_count && cmap_offset && loadsyms[i].range != SYM_PCHAR)
                cmap_count = i - cmap_offset;
            i++;
        }
    }
    if (is_G || filling_cache || dump_ids) {
        if (!filling_cache && id && glyphid_cache) {
            int val = find_glyph_in_cache(id);
            if (val >= 0) {
                findwhat->findtype = find_glyph;
                findwhat->val = val;
                findwhat->loadsyms_offset = 0;
                return 1;
            } else {
                return 0;
            }
        } else {
            /* individual matching glyph entries */
            for (glyph = 0; glyph < MAX_GLYPH; ++glyph) {
                skip_base = FALSE;
                skip_this_one = FALSE;
                buf[0][0] = buf[1][0] = buf[2][0] = buf[3][0] = '\0';
                if (glyph_is_monster(glyph)) {
                    /* buf2 will hold the distinguishing prefix */
                    /* buf3 will hold the base name */
                    const char *buf2 = "";
                    const char *buf3 = monsdump[glyph_to_mon(glyph)].nm;
                    if (glyph_is_normal_male_monster(glyph)) {
                        buf2 = "male_";
                    } else if (glyph_is_normal_female_monster(glyph)) {
                        buf2 = "female_";
                    } else if (glyph_is_ridden_male_monster(glyph)) {
                        buf2 = "ridden_male_";
                    } else if (glyph_is_ridden_female_monster(glyph)) {
                        buf2 = "ridden_female_";
                    } else if (glyph_is_detected_male_monster(glyph)) {
                        buf2 = "detected_male_";
                    } else if (glyph_is_detected_female_monster(glyph)) {
                        buf2 = "detected_female_";
                    } else if (glyph_is_male_pet(glyph)) {
                        buf2 = "pet_male_";
                    } else if (glyph_is_female_pet(glyph)) {
                        buf2 = "pet_female_";
                    }
                    Strcpy(buf[0], "G_");
                    Strcat(buf[0], buf2);
                    Strcat(buf[0], buf3);
                } else if (glyph_is_body(glyph)) {
                    /* buf2 will hold the distinguishing prefix */
                    /* buf3 will hold the base name */
                    const char *buf2 = "";
                    const char *buf3 = monsdump[glyph_to_body_corpsenm(glyph)].nm;
                    if (glyph_is_body_piletop(glyph)) {
                        buf2 = "piletop_body_";
                    } else {
                        buf2 = "body_";
                    }
                    Strcpy(buf[0], "G_");
                    Strcat(buf[0], buf2);
                    Strcat(buf[0], buf3);
                } else if (glyph_is_statue(glyph)) {
                    /* buf2 will hold the distinguishing prefix */
                    /* buf3 will hold the base name */
                    const char *buf2 = "";
                    const char *buf3 = monsdump[glyph_to_statue_corpsenm(glyph)].nm;
                    if (glyph_is_fem_statue_piletop(glyph)) {
                        buf2 = "piletop_statue_of_female_";
                    } else if (glyph_is_fem_statue(glyph)) {
                        buf2 = "statue_of_female_";
                    } else if (glyph_is_male_statue_piletop(glyph)) {
                        buf2 = "piletop_statue_of_male_";
                    } else if (glyph_is_male_statue(glyph)) {
                        buf2 = "statue_of_male_";
                    }
                    Strcpy(buf[0], "G_");
                    Strcat(buf[0], buf2);
                    Strcat(buf[0], buf3);
                } else if (glyph_is_object(glyph)) {
                    i = glyph_to_obj(glyph);
                    /* buf2 will hold the distinguishing prefix */
                    /* buf3 will hold the base name */
                    const char *buf2 = "";
                    const char *buf3 = "";
                    if (((i > SCR_STINKING_CLOUD) && (i < SCR_MAIL))
                        || ((i > WAN_LIGHTNING) && (i < GOLD_PIECE)))
                        skip_this_one = TRUE;
                    if (!skip_this_one) {
                        if ((i >= WAN_LIGHT) && (i <= WAN_LIGHTNING))
                            buf2 = "wand of ";
                        else if ((i >= SPE_DIG) && (i < SPE_BLANK_PAPER))
                            buf2 = "spellbook of ";
                        else if ((i >= SCR_ENCHANT_ARMOR)
                                 && (i <= SCR_STINKING_CLOUD))
                            buf2 = "scroll of ";
                        else if ((i >= POT_GAIN_ABILITY) && (i <= POT_WATER))
                            buf2 = (i == POT_WATER) ? "flask of n"
                                                    : "potion of ";
                        else if ((i >= RIN_ADORNMENT)
                                 && (i <= RIN_PROTECTION_FROM_SHAPE_CHAN))
                            buf2 = "ring of ";
                        else if (i == LAND_MINE)
                            buf2 = "unset ";
                        buf3 = (i == SCR_BLANK_PAPER) ? "blank scroll"
                               : (i == SPE_BLANK_PAPER) ? "blank spellbook"
                                 : (i == SLIME_MOLD) ? "slime mold"
                                   : obj_descr[i].oc_name
                                     ? obj_descr[i].oc_name
                                     : obj_descr[i].oc_descr;
                        Strcpy(buf[0], "G_");
                        if (glyph_is_normal_piletop_obj(glyph))
                            Strcat(buf[0], "piletop_");
                        Strcat(buf[0], buf2);
                        Strcat(buf[0], buf3);
                    }
                } else if (glyph_is_cmap(glyph) || glyph_is_cmap_zap(glyph)
                           || glyph_is_swallow(glyph)
                           || glyph_is_explosion(glyph)) {
                    int cmap = -1;

                    /* buf2 will hold the distinguishing prefix */
                    /* buf3 will hold the base name */
                    /* buf4 will hold the distinguishing suffix */
                    const char *buf2 = "";
                    const char *buf3 = "";
                    const char *buf4 = "";
                    if (glyph == GLYPH_CMAP_OFF) {
                        cmap = S_stone;
                        buf3 = "stone substrate";
                        skip_base = TRUE;
                    } else if (glyph_is_cmap_gehennom(glyph)) {
                        cmap = (glyph - GLYPH_CMAP_GEH_OFF) + S_vwall;
                        buf4 = "_gehennom";
                    } else if (glyph_is_cmap_knox(glyph)) {
                        cmap = (glyph - GLYPH_CMAP_KNOX_OFF) + S_vwall;
                        buf4 = "_knox";
                    } else if (glyph_is_cmap_main(glyph)) {
                        cmap = (glyph - GLYPH_CMAP_MAIN_OFF) + S_vwall;
                        buf4 = "_main";
                    } else if (glyph_is_cmap_mines(glyph)) {
                        cmap = (glyph - GLYPH_CMAP_MINES_OFF) + S_vwall;
                        buf4 = "_mines";
                    } else if (glyph_is_cmap_sokoban(glyph)) {
                        cmap = (glyph - GLYPH_CMAP_SOKO_OFF) + S_vwall;
                        buf4 = "_sokoban";
                    } else if (glyph_is_cmap_a(glyph)) {
                        cmap = (glyph - GLYPH_CMAP_A_OFF) + S_ndoor;
                    } else if (glyph_is_cmap_altar(glyph)) {
                        const char *altar_text[] = {
                            "unaligned", "chaotic", "neutral",
                            "lawful",    "other",
                        };
                        j = (glyph - GLYPH_ALTAR_OFF);
                        cmap = S_altar;
                        if (j != altar_other) {
                            Snprintf(buf[2], sizeof buf[2], "%s_",
                                     altar_text[j]);
                            buf2 = buf[2];
                        } else {
                            buf3 = "altar other";
                            skip_base = TRUE;
                        }
                    } else if (glyph_is_cmap_b(glyph)) {
                        cmap = (glyph - GLYPH_CMAP_B_OFF) + S_grave;
                    } else if (glyph_is_cmap_zap(glyph)) {
                        static const char *zap_texts[] = {
                            "missile", "fire",      "frost",      "sleep",
                            "death",   "lightning", "poison gas", "acid"
                        };
                        j = (glyph - GLYPH_ZAP_OFF);
                        cmap = (j % 4) + S_vbeam;
                        Snprintf(buf[2], sizeof buf[2], "%s",
                                 loadsyms[cmap + cmap_offset].name + 2);
                        Snprintf(buf[3], sizeof buf[3], "%s zap %s",
                                 zap_texts[j / 4], fix_glyphname(buf[2]));
                        buf3 = buf[3];
                        buf2 = "";
                        skip_base = TRUE;
                    } else if (glyph_is_cmap_c(glyph)) {
                        cmap = (glyph - GLYPH_CMAP_C_OFF) + S_digbeam;
                    } else if (glyph_is_swallow(glyph)) {
                        static const char *swallow_texts[] = {
                            "top left",      "top center",   "top right",
                            "middle left",   "middle right", "bottom left",
                            "bottom center", "bottom right",
                        };
                        j = glyph - GLYPH_SWALLOW_OFF;
                        cmap = glyph_to_swallow(glyph);
                        mnum = j / ((S_sw_br - S_sw_tl) + 1);
                        i = cmap - S_sw_tl;
                        Strcpy(buf[3], "swallow ");
                        Strcat(buf[3], monsdump[mnum].nm);
                        Strcat(buf[3], " ");
                        Strcat(buf[3], swallow_texts[cmap]);
                        buf3 = buf[3];
                        skip_base = TRUE;
                    } else if (glyph_is_explosion(glyph)) {
                        int expl;
                        static const char *expl_type_texts[] = {
                            "dark",    "noxious", "muddy",  "wet",
                            "magical", "fiery",   "frosty",
                        };
                        static const char *expl_texts[] = {
                            "tl", "tc", "tr", "ml", "mc",
                            "mr", "bl", "bc", "br",
                        };

                        j = glyph - GLYPH_EXPLODE_OFF;
                        expl = j / ((S_expl_br - S_expl_tl) + 1);
                        cmap =
                            (j % ((S_expl_br - S_expl_tl) + 1)) + S_expl_tl;
                        i = cmap - S_expl_tl;
                        Snprintf(buf[2], sizeof buf[2], "%s ",
                                 expl_type_texts[expl]);
                        buf2 = buf[2];
                        Snprintf(buf[3], sizeof buf[3], "%s%s", "expl_",
                                 expl_texts[i]);
                        buf3 = buf[3];
                        skip_base = TRUE;
                    }
                    if (!skip_base) {
                        if (cmap >= 0 && cmap < MAXPCHARS) {
                            buf3 = loadsyms[cmap + cmap_offset].name + 2;
                        }
                    }
                    Strcpy(buf[0], "G_");
                    Strcat(buf[0], buf2);
                    Strcat(buf[0], buf3);
                    Strcat(buf[0], buf4);
                } else if (glyph_is_invisible(glyph)) {
                    Strcpy(buf[0], "G_invisible");
                } else if (glyph_is_nothing(glyph)) {
                    Strcpy(buf[0], "G_nothing");
                } else if (glyph_is_unexplored(glyph)) {
                    Strcpy(buf[0], "G_unexplored");
                } else if (glyph_is_warning(glyph)) {
                    j = glyph - GLYPH_WARNING_OFF;
                    Snprintf(buf[0], sizeof buf[0], "G_%s%d", "warning", j);
                }
                if (memchr(buf[0], '\0', sizeof buf[0]) == NULL)
                    panic("parse_id: buf[0] overflowed");
                if (!skip_this_one) {
                    fix_glyphname(buf[0]+2);
                    if (dump_ids) {
                        Fprintf(fp, "(%04d) %s\n", glyph, buf[0]);
                    } else if (filling_cache) {
                        add_glyph_to_cache(glyph, buf[0]);
                    } else if (id) {
                        if (!strcmpi(id, buf[0])) {
                            findwhat->findtype = find_glyph;
                            findwhat->val = glyph;
                            findwhat->loadsyms_offset = 0;
                            return 1;
                        }
                    }
                }
            }
        } /* not glyphid_cache */
    } else if (is_S) {
        /* cmap entries */
        for (i = 0; i < cmap_count; ++i) {
            if (!strcmpi(loadsyms[i + cmap_offset].name + 2, id + 2)) {
                findwhat->findtype = find_cmap;
                findwhat->val = i;
                findwhat->loadsyms_offset = i + cmap_offset;
                return 1;
            }
        }
        /* objclass entries */
        for (i = 0; i < oc_count; ++i) {
            if (!strcmpi(loadsyms[i + oc_offset].name + 2, id + 2)) {
                findwhat->findtype = find_oc;
                findwhat->val = i;
                findwhat->loadsyms_offset = i + oc_offset;
                return 1;
            }
        }
        /* permonst entries */
        for (i = 0; i <= pm_count; ++i) {
            if (!strcmpi(loadsyms[i + pm_offset].name + 2, id + 2)) {
                findwhat->findtype = find_pm;
                findwhat->val = i + 1; /* starts at 1 */
                findwhat->loadsyms_offset = i + pm_offset;
                return 1;
            }
        }
    }
    if (dump_ids || filling_cache)
        return 1;
    findwhat->findtype = find_nothing;
    findwhat->val = 0;
    findwhat->loadsyms_offset = 0;
    return 0;
}

static struct {
    int index;
    uint32_t value;
} color_definitions_256[] = {
    /* color values are from unnethack */
    {  16, 0x000000 }, {  17, 0x00005f }, {  18, 0x000087 },
    {  19, 0x0000af }, {  20, 0x0000d7 }, {  21, 0x0000ff },
    {  22, 0x005f00 }, {  23, 0x005f5f }, {  24, 0x005f87 },
    {  25, 0x005faf }, {  26, 0x005fd7 }, {  27, 0x005fff },
    {  28, 0x008700 }, {  29, 0x00875f }, {  30, 0x008787 },
    {  31, 0x0087af }, {  32, 0x0087d7 }, {  33, 0x0087ff },
    {  34, 0x00af00 }, {  35, 0x00af5f }, {  36, 0x00af87 },
    {  37, 0x00afaf }, {  38, 0x00afd7 }, {  39, 0x00afff },
    {  40, 0x00d700 }, {  41, 0x00d75f }, {  42, 0x00d787 },
    {  43, 0x00d7af }, {  44, 0x00d7d7 }, {  45, 0x00d7ff },
    {  46, 0x00ff00 }, {  47, 0x00ff5f }, {  48, 0x00ff87 },
    {  49, 0x00ffaf }, {  50, 0x00ffd7 }, {  51, 0x00ffff },
    {  52, 0x5f0000 }, {  53, 0x5f005f }, {  54, 0x5f0087 },
    {  55, 0x5f00af }, {  56, 0x5f00d7 }, {  57, 0x5f00ff },
    {  58, 0x5f5f00 }, {  59, 0x5f5f5f }, {  60, 0x5f5f87 },
    {  61, 0x5f5faf }, {  62, 0x5f5fd7 }, {  63, 0x5f5fff },
    {  64, 0x5f8700 }, {  65, 0x5f875f }, {  66, 0x5f8787 },
    {  67, 0x5f87af }, {  68, 0x5f87d7 }, {  69, 0x5f87ff },
    {  70, 0x5faf00 }, {  71, 0x5faf5f }, {  72, 0x5faf87 },
    {  73, 0x5fafaf }, {  74, 0x5fafd7 }, {  75, 0x5fafff },
    {  76, 0x5fd700 }, {  77, 0x5fd75f }, {  78, 0x5fd787 },
    {  79, 0x5fd7af }, {  80, 0x5fd7d7 }, {  81, 0x5fd7ff },
    {  82, 0x5fff00 }, {  83, 0x5fff5f }, {  84, 0x5fff87 },
    {  85, 0x5fffaf }, {  86, 0x5fffd7 }, {  87, 0x5fffff },
    {  88, 0x870000 }, {  89, 0x87005f }, {  90, 0x870087 },
    {  91, 0x8700af }, {  92, 0x8700d7 }, {  93, 0x8700ff },
    {  94, 0x875f00 }, {  95, 0x875f5f }, {  96, 0x875f87 },
    {  97, 0x875faf }, {  98, 0x875fd7 }, {  99, 0x875fff },
    { 100, 0x878700 }, { 101, 0x87875f }, { 102, 0x878787 },
    { 103, 0x8787af }, { 104, 0x8787d7 }, { 105, 0x8787ff },
    { 106, 0x87af00 }, { 107, 0x87af5f }, { 108, 0x87af87 },
    { 109, 0x87afaf }, { 110, 0x87afd7 }, { 111, 0x87afff },
    { 112, 0x87d700 }, { 113, 0x87d75f }, { 114, 0x87d787 },
    { 115, 0x87d7af }, { 116, 0x87d7d7 }, { 117, 0x87d7ff },
    { 118, 0x87ff00 }, { 119, 0x87ff5f }, { 120, 0x87ff87 },
    { 121, 0x87ffaf }, { 122, 0x87ffd7 }, { 123, 0x87ffff },
    { 124, 0xaf0000 }, { 125, 0xaf005f }, { 126, 0xaf0087 },
    { 127, 0xaf00af }, { 128, 0xaf00d7 }, { 129, 0xaf00ff },
    { 130, 0xaf5f00 }, { 131, 0xaf5f5f }, { 132, 0xaf5f87 },
    { 133, 0xaf5faf }, { 134, 0xaf5fd7 }, { 135, 0xaf5fff },
    { 136, 0xaf8700 }, { 137, 0xaf875f }, { 138, 0xaf8787 },
    { 139, 0xaf87af }, { 140, 0xaf87d7 }, { 141, 0xaf87ff },
    { 142, 0xafaf00 }, { 143, 0xafaf5f }, { 144, 0xafaf87 },
    { 145, 0xafafaf }, { 146, 0xafafd7 }, { 147, 0xafafff },
    { 148, 0xafd700 }, { 149, 0xafd75f }, { 150, 0xafd787 },
    { 151, 0xafd7af }, { 152, 0xafd7d7 }, { 153, 0xafd7ff },
    { 154, 0xafff00 }, { 155, 0xafff5f }, { 156, 0xafff87 },
    { 157, 0xafffaf }, { 158, 0xafffd7 }, { 159, 0xafffff },
    { 160, 0xd70000 }, { 161, 0xd7005f }, { 162, 0xd70087 },
    { 163, 0xd700af }, { 164, 0xd700d7 }, { 165, 0xd700ff },
    { 166, 0xd75f00 }, { 167, 0xd75f5f }, { 168, 0xd75f87 },
    { 169, 0xd75faf }, { 170, 0xd75fd7 }, { 171, 0xd75fff },
    { 172, 0xd78700 }, { 173, 0xd7875f }, { 174, 0xd78787 },
    { 175, 0xd787af }, { 176, 0xd787d7 }, { 177, 0xd787ff },
    { 178, 0xd7af00 }, { 179, 0xd7af5f }, { 180, 0xd7af87 },
    { 181, 0xd7afaf }, { 182, 0xd7afd7 }, { 183, 0xd7afff },
    { 184, 0xd7d700 }, { 185, 0xd7d75f }, { 186, 0xd7d787 },
    { 187, 0xd7d7af }, { 188, 0xd7d7d7 }, { 189, 0xd7d7ff },
    { 190, 0xd7ff00 }, { 191, 0xd7ff5f }, { 192, 0xd7ff87 },
    { 193, 0xd7ffaf }, { 194, 0xd7ffd7 }, { 195, 0xd7ffff },
    { 196, 0xff0000 }, { 197, 0xff005f }, { 198, 0xff0087 },
    { 199, 0xff00af }, { 200, 0xff00d7 }, { 201, 0xff00ff },
    { 202, 0xff5f00 }, { 203, 0xff5f5f }, { 204, 0xff5f87 },
    { 205, 0xff5faf }, { 206, 0xff5fd7 }, { 207, 0xff5fff },
    { 208, 0xff8700 }, { 209, 0xff875f }, { 210, 0xff8787 },
    { 211, 0xff87af }, { 212, 0xff87d7 }, { 213, 0xff87ff },
    { 214, 0xffaf00 }, { 215, 0xffaf5f }, { 216, 0xffaf87 },
    { 217, 0xffafaf }, { 218, 0xffafd7 }, { 219, 0xffafff },
    { 220, 0xffd700 }, { 221, 0xffd75f }, { 222, 0xffd787 },
    { 223, 0xffd7af }, { 224, 0xffd7d7 }, { 225, 0xffd7ff },
    { 226, 0xffff00 }, { 227, 0xffff5f }, { 228, 0xffff87 },
    { 229, 0xffffaf }, { 230, 0xffffd7 }, { 231, 0xffffff },
    { 232, 0x080808 }, { 233, 0x121212 }, { 234, 0x1c1c1c },
    { 235, 0x262626 }, { 236, 0x303030 }, { 237, 0x3a3a3a },
    { 238, 0x444444 }, { 239, 0x4e4e4e }, { 240, 0x585858 },
    { 241, 0x626262 }, { 242, 0x6c6c6c }, { 243, 0x767676 },
    { 244, 0x808080 }, { 245, 0x8a8a8a }, { 246, 0x949494 },
    { 247, 0x9e9e9e }, { 248, 0xa8a8a8 }, { 249, 0xb2b2b2 },
    { 250, 0xbcbcbc }, { 251, 0xc6c6c6 }, { 252, 0xd0d0d0 },
    { 253, 0xdadada }, { 254, 0xe4e4e4 }, { 255, 0xeeeeee },
};

/** Calculate the color distance between two colors.
 *
 * Algorithm taken from UnNetHack which took it from
 * https://www.compuphase.com/cmetric.htm
 **/

static int
color_distance(uint32_t rgb1, uint32_t rgb2)
{
    int r1 = (rgb1 >> 16) & 0xFF;
    int g1 = (rgb1 >> 8) & 0xFF;
    int b1 = (rgb1) &0xFF;
    int r2 = (rgb2 >> 16) & 0xFF;
    int g2 = (rgb2 >> 8) & 0xFF;
    int b2 = (rgb2) &0xFF;

    int rmean = (r1 + r2) / 2;
    int r = r1 - r2;
    int g = g1 - g2;
    int b = b1 - b2;
    return ((((512 + rmean) * r * r) >> 8) + 4 * g * g
                 + (((767 - rmean) * b * b) >> 8));
}

boolean
closest_color(uint32_t lcolor, uint32_t *closecolor, int *clridx)
{
    int i, color_index = -1, similar = INT_MAX, current;
    boolean retbool = FALSE;

    for (i = 0; i < SIZE(color_definitions_256); i++) {
        /* look for an exact match */
        if (lcolor == color_definitions_256[i].value) {
            color_index = i;
            break;
        }
        /* find a close color match */
        current = color_distance(lcolor, color_definitions_256[i].value);
        if (current < similar) {
            color_index = i;
            similar = current;
        }
    }
    if (closecolor && clridx && color_index >= 0) {
        *closecolor = color_definitions_256[color_index].value;
        *clridx = color_definitions_256[color_index].index;
        retbool = TRUE;
    }
    return retbool;
}
#endif /* ENHANCED_SYMBOLS */

#ifdef TEST_GLYPHNAMES

static struct {
    int idx;
    const char *nm1;
    const char *nm2;
} cmapname[MAXPCHARS] = {
#define PCHAR_TILES
#include "defsym.h"
#undef PCHAR_TILES
};

static int glyphs_to_unicode(const char *id, const char *unicode_val,
                             long clr);
static int find_glyphs(const char *id);
static void just_find_callback(int glyph, struct find_struct *findwhat);
static void to_unicode_callback(int glyph, struct find_struct *findwhat);
static struct customization_detail *find_display_urep_customization(
                const char *customization_name, int glyphidx,
                enum graphics_sets which_set);

void
test_glyphnames(void)
{
    int reslt;

    reslt = find_glyphs("G_potion_of_monster_detection");
    reslt = find_glyphs("G_piletop_body_chickatrice");
    reslt = find_glyphs("G_detected_male_homunculus");
    reslt = find_glyphs("S_pool");
    reslt = find_glyphs("S_dog");
    reslt = glyphs_to_unicode("S_dog", "U+130E6", 0L);
}

static void
just_find_callback(int glyph UNUSED, struct find_struct *findwhat UNUSED)
{
    return;
}

static int
find_glyphs(const char *id)
{
    struct find_struct find_only = zero_find;

    find_only.unicode_val = 0;
    find_only.callback = just_find_callback;
    return glyph_find_core(id, &find_only);
}

static void
to_unicode_callback(int glyph UNUSED, struct find_struct *findwhat)
{
    int uval;
#ifdef NO_PARSING_SYMSET
    glyph_map *gm = &glyphmap[glyph];
#endif
    uint8 utf8str[6];

    if (!findwhat->unicode_val)
        return;
    uval = unicode_val(findwhat->unicode_val);
    if (unicodeval_to_utf8str(uval, utf8str, sizeof utf8str)) {
#ifdef NO_PARSING_SYMSET
        set_map_u(gm, uval, utf8str,
                  (findwhat->color != 0L) ? findwhat->color : 0L);
#else

#endif
    }
}

int
glyphs_to_unicode(const char *id, const char *unicode_val, long clr)
{
    struct find_struct to_unicode = zero_find;

    to_unicode.unicode_val = unicode_val;
    to_unicode.callback = to_unicode_callback;
    /* if the color 0 is an actual color, as opposed to just "not set"
       we set a marker bit outside the 24-bit range to indicate a
       valid color value 0. That allows valid color 0, but allows a
       simple checking for 0 to detect "not set". The window port that
       implements the color switch, needs to either check that bit
       or appropriately mask colors with 0xFFFFFF. */
    to_unicode.color = (clr == -1) ? 0L : (clr == 0L) ? (0 & 0x1000000) : clr;
    return glyph_find_core(id, &to_unicode);
}

#if 0
struct customization_detail *
find_display_urep_customization(
    const char *customization_name,
    int glyphidx,
    enum graphics_sets which_set)
{
    struct symset_customization *gdc = &gs.sym_customizations[which_set];
    struct customization_detail *urepdetails;

    if ((gdc->custtype == custom_ureps)
        || (strcmp(customization_name, gdc->customization_name) == 0)) {
        urepdetails = gdc->details;
        while (urepdetails) {
            if (urepdetails->content.urep.glyphidx == glyphidx)
                return urepdetails;
            urepdetails = urepdetails->next;
        }
    }
    return (struct customization_detail *) 0;
}
#endif
#endif /* SOME TEST STUFF */

/* utf8map.c */



