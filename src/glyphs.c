/* NetHack 3.7	glyphs.c	TODO: add NHDT branch/date/revision tags */
/* Copyright (c) Michael Allison, 2021. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

extern const struct symparse loadsyms[];
extern glyph_map glyphmap[MAX_GLYPH];
extern struct enum_dump monsdump[];
extern struct enum_dump objdump[];

#define Fprintf (void) fprintf

enum reserved_activities { res_nothing, res_dump_glyphids, res_fill_cache };
enum things_to_find { find_nothing, find_pm, find_oc, find_cmap, find_glyph };
struct find_struct {
    enum things_to_find findtype;
    int val;
    int loadsyms_offset;
    int loadsyms_count;
    int *extraval;
    uint32 color;
    const char *unicode_val; /* U+NNNN format */
    void (*callback)(int glyph, struct find_struct *);
    enum reserved_activities restype;
    genericptr_t reserved;
};
static const struct find_struct zero_find = { 0 };
struct glyphid_cache_t {
    int glyphnum;
    char *id;
};
static struct glyphid_cache_t *glyphid_cache;
static unsigned glyphid_cache_lsize;
static size_t glyphid_cache_size;
static struct find_struct glyphcache_find, to_custom_symbol_find;
static const long nonzero_black = CLR_BLACK | NH_BASIC_COLOR;

staticfn void init_glyph_cache(void);
staticfn void add_glyph_to_cache(int glyphnum, const char *id);
staticfn int find_glyph_in_cache(const char *id);
staticfn char *find_glyphid_in_cache_by_glyphnum(int glyphnum);
staticfn uint32 glyph_hash(const char *id);
staticfn void to_custom_symset_entry_callback(int glyph,
                                            struct find_struct *findwhat);
staticfn int parse_id(const char *id, struct find_struct *findwhat);
staticfn int glyph_find_core(const char *id, struct find_struct *findwhat);
staticfn char *fix_glyphname(char *str);
staticfn void shuffle_customizations(void);
/* staticfn void purge_custom_entries(enum graphics_sets which_set); */

staticfn void
to_custom_symset_entry_callback(
    int glyph,
    struct find_struct *findwhat)
{
    int idx = gs.symset_which_set;
#ifdef ENHANCED_SYMBOLS
    uint8 utf8str[6] = { 0, 0, 0, 0, 0, 0 };
    int uval = 0;
#endif

    if (findwhat->extraval)
        *findwhat->extraval = glyph;

    assert(idx >= 0 && idx < NUM_GRAPHICS);
#ifdef ENHANCED_SYMBOLS
    if (findwhat->unicode_val)
        uval = unicode_val(findwhat->unicode_val);
    if (uval && unicodeval_to_utf8str(uval, utf8str, sizeof utf8str)) {
        /* presently the customizations are affiliated with a particular
         * symset but if we don't have any symset context, ignore it for now
         * in order to avoid a segfault.
         * FIXME:
         * One future idea might be to store the U+ entries under "UTF8"
         * and apply those customizations to any current symset if it has
         * a UTF8 handler. Similar approach for unaffiliated glyph/symbols
         * non-UTF color customizations
         */
        if (gs.symset[idx].name) {
            add_custom_urep_entry(gs.symset[idx].name, glyph, uval,
                              utf8str, gs.symset_which_set);
        } else {
            static int glyphnag = 0;

            if (!glyphnag++)
                config_error_add("Unimplemented customization feature,"
                                 " ignoring for now");
        }
    }
#endif
    if (findwhat->color) {
        if (gs.symset[idx].name) {
            add_custom_nhcolor_entry(gs.symset[idx].name, glyph,
                                 findwhat->color, gs.symset_which_set);
        } else {
            static int colornag = 0;

            if (!colornag++)
                config_error_add("Unimplemented customization feature,"
                                 " ignoring for now");
        }
    }
}

/*
 * Return value:
 *               1 = success
 *               0 = failure
 */
int
glyphrep_to_custom_map_entries(
    const char *op,
    int *glyphptr)
{
    to_custom_symbol_find = zero_find;
    char buf[BUFSZ], *c_glyphid, *c_unicode, *c_colorval, *cp;
    int reslt = 0;
    long rgb = 0L;
    boolean slash = FALSE, colon = FALSE;

    if (!glyphid_cache)
        reslt = 1; /* for debugger use only; no cache available */

    Snprintf(buf, sizeof buf, "%s", op);
    c_unicode = c_colorval = (char *) 0;
    c_glyphid = cp = buf;
    while (*cp) {
        if (*cp == ':' || *cp == '/') {
            if (*cp == ':') {
                colon = TRUE;
                *cp = '\0';
            }
            if (*cp == '/') {
                slash = TRUE;
                *cp = '\0';
            }
        }
        cp++;
        if (colon) {
            c_unicode = cp;
            colon = FALSE;
        }
        if (slash) {
            c_colorval = cp;
            slash = FALSE;
        }
    }
    /* some sanity checks */
    if (c_glyphid && *c_glyphid == ' ')
        c_glyphid++;
    if (c_colorval && *c_colorval == ' ')
        c_colorval++;
    if (c_unicode && *c_unicode == ' ') {
        while (*c_unicode == ' ') {
            c_unicode++;
        }
    }
    if (c_unicode && !*c_unicode)
        c_unicode = 0;

    if ((c_colorval && (rgb = rgbstr_to_int32(c_colorval)) != -1L)
        || !c_colorval) {
        /* if the color 0 is an actual color, as opposed to just "not set"
           we set a marker bit outside the 24-bit range to indicate a
           valid color value 0. That allows valid color 0, but allows a
           simple checking for 0 to detect "not set". The window port that
           implements the color switch, needs to either check that bit
           or appropriately mask colors with 0xFFFFFF. */
        to_custom_symbol_find.color = (rgb == -1 || !c_colorval) ? 0L
                                      : (rgb == 0L) ? nonzero_black
                                                    : rgb;
    }
    if (c_unicode)
        to_custom_symbol_find.unicode_val = c_unicode;
    to_custom_symbol_find.extraval = glyphptr;
    to_custom_symbol_find.callback = to_custom_symset_entry_callback;
    reslt = glyph_find_core(c_glyphid, &to_custom_symbol_find);
    return reslt;
}

staticfn char *
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

int
glyph_to_cmap(int glyph)
{
    if (glyph == GLYPH_CMAP_STONE_OFF)
        return S_stone;
    else if (glyph_is_cmap_main(glyph))
        return (glyph - GLYPH_CMAP_MAIN_OFF) + S_vwall;
    else if (glyph_is_cmap_mines(glyph))
        return (glyph - GLYPH_CMAP_MINES_OFF) + S_vwall;
    else if (glyph_is_cmap_gehennom(glyph))
        return (glyph - GLYPH_CMAP_GEH_OFF) + S_vwall;
    else if (glyph_is_cmap_knox(glyph))
        return (glyph - GLYPH_CMAP_KNOX_OFF) + S_vwall;
    else if (glyph_is_cmap_sokoban(glyph))
        return (glyph - GLYPH_CMAP_SOKO_OFF) + S_vwall;
    else if (glyph_is_cmap_a(glyph))
        return (glyph - GLYPH_CMAP_A_OFF) + S_ndoor;
    else if (glyph_is_cmap_altar(glyph))
        return S_altar;
    else if (glyph_is_cmap_b(glyph))
        return (glyph - GLYPH_CMAP_B_OFF) + S_grave;
    else if (glyph_is_cmap_c(glyph))
        return (glyph - GLYPH_CMAP_C_OFF) + S_digbeam;
    else if (glyph_is_cmap_zap(glyph))
        return ((glyph - GLYPH_ZAP_OFF) % 4) + S_vbeam;
    else if (glyph_is_swallow(glyph))
        return glyph_to_swallow(glyph) + S_sw_tl;
    else if (glyph_is_explosion(glyph))
        return glyph_to_explosion(glyph) + S_expl_tl;
    else
        return MAXPCHARS;    /* MAXPCHARS is legal array index because
                              * of trailing fencepost entry */
}

staticfn int
glyph_find_core(
    const char *id,
    struct find_struct *findwhat)
{
    int glyph;
    boolean do_callback, end_find = FALSE;

    if (parse_id(id, findwhat)) {
        if (findwhat->findtype == find_glyph) {
            (*findwhat->callback)(findwhat->val, findwhat);
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
                        && monsym(&mons[glyph_to_mon(glyph)])
                           == findwhat->val)
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
 over with.
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

staticfn void
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
                        glyphid_cache_size * sizeof (struct glyphid_cache_t));
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

staticfn void
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

staticfn int
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

staticfn char *
find_glyphid_in_cache_by_glyphnum(int glyphnum)
{
    size_t idx;

    if (!glyphid_cache)
        return (char *) 0;
    for (idx = 0; idx < glyphid_cache_size; ++idx) {
        if (glyphid_cache[idx].glyphnum == glyphnum
            && glyphid_cache[idx].id != 0) {
            /* Match found */
            return glyphid_cache[idx].id;
        }
    }
    return (char *) 0;
}

staticfn uint32
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
add_custom_nhcolor_entry(
    const char *customization_name,
    int glyphidx,
    uint32 nhcolor,
    enum graphics_sets which_set)
{
    struct symset_customization *gdc
                        = &gs.sym_customizations[which_set][custom_nhcolor];
    struct customization_detail *details, *newdetails = 0;
#if 0
    static uint32 closecolor = 0;
    static int clridx = 0;
#endif

    if (!gdc->details) {
        gdc->customization_name = dupstr(customization_name);
        gdc->custtype = custom_nhcolor;
        gdc->details = 0;
        gdc->details_end = 0;
    }
    details = find_matching_customization(customization_name,
                                          custom_nhcolor, which_set);
    if (details) {
        while (details) {
            if (details->content.ccolor.glyphidx == glyphidx) {
                details->content.ccolor.nhcolor = nhcolor;
                return 1;
            }
            details = details->next;
        }
    }
    /* create new details entry */
    newdetails = (struct customization_detail *) alloc(sizeof *newdetails);
    newdetails->content.urep.glyphidx = glyphidx;
    newdetails->content.ccolor.nhcolor = nhcolor;
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

void
apply_customizations(
    enum graphics_sets which_set,
    enum do_customizations docustomize)
{
    glyph_map *gmap;
    struct customization_detail *details;
    struct symset_customization *sc;
    boolean at_least_one = FALSE,
            do_colors = ((docustomize & do_custom_colors) != 0),
            do_symbols = ((docustomize & do_custom_symbols) != 0);
    int custs;

    for (custs = 0; custs < (int) custom_count; ++custs) {
        sc = &gs.sym_customizations[which_set][custs];
        if (sc->count && sc->details) {
            at_least_one = TRUE;
            /* These glyph customizations get applied to the glyphmap array,
               not to symset entries */
            details = sc->details;
            while (details) {
#ifdef ENHANCED_SYMBOLS
                if (iflags.customsymbols && do_symbols) {
                    if (sc->custtype == custom_ureps) {
                        gmap = &glyphmap[details->content.urep.glyphidx];
                        if (gs.symset[which_set].handling == H_UTF8)
                            (void) set_map_u(gmap,
                                             details->content.urep.u.utf32ch,
                                             details->content.urep.u.utf8str);
                    }
                }
#endif
                if (iflags.customcolors && do_colors) {
                    if (sc->custtype == custom_nhcolor) {
                        gmap = &glyphmap[details->content.ccolor.glyphidx];
                        (void) set_map_customcolor(gmap,
                                             details->content.ccolor.nhcolor);
                    }
                }
                details = details->next;
            }
        }
    }
    if (at_least_one) {
        shuffle_customizations();
    }
}

/* Shuffle the customizations to match shuffled object descriptions,
 * so a red potion isn't displayed with a blue customization, and so on.
 */

#if 0
staticfn void
shuffle_customizations(void)
{
    static const int offsets[2] = { GLYPH_OBJ_OFF, GLYPH_OBJ_PILETOP_OFF };
    int j;

    for (j = 0; j < SIZE(offsets); j++) {
        glyph_map *obj_glyphs = glyphmap + offsets[j];
        int i;
        struct unicode_representation *tmp_u[NUM_OBJECTS];
        int duplicate[NUM_OBJECTS];

        for (i = 0; i < NUM_OBJECTS; i++) {
            duplicate[i] = -1;
            tmp_u[i] = (struct unicode_representation *) 0;
        }
        for (i = 0; i < NUM_OBJECTS; i++) {
            int idx = objects[i].oc_descr_idx;

            /*
             * Shuffling gem appearances can cause the same oc_descr_idx to
             * appear more than once. Detect this condition and ensure that
             * each pointer points to a unique allocation.
             */
            if (duplicate[idx] >= 0) {
                /* Current structure already appears in tmp_u */
                struct unicode_representation *other = tmp_u[duplicate[idx]];

                tmp_u[i] = (struct unicode_representation *)
                           alloc(sizeof *tmp_u[i]);
                *tmp_u[i] = *other;
                if (other->utf8str != NULL) {
                    tmp_u[i]->utf8str = (uint8 *)
                                        dupstr((const char *) other->utf8str);
                }
            } else {
                tmp_u[i] = obj_glyphs[idx].u;
                if (obj_glyphs[idx].u != NULL)  {
                    duplicate[idx] = i;
                    obj_glyphs[idx].u = NULL;
                }
            }
        }
        for (i = 0; i < NUM_OBJECTS; i++) {
            /* Some glyphmaps may not have been transferred */
            if (obj_glyphs[i].u != NULL) {
                free(obj_glyphs[i].u->utf8str);
                free(obj_glyphs[i].u);
            }
            obj_glyphs[i].u = tmp_u[i];
        }
    }
}

#else
staticfn void
shuffle_customizations(void)
{
    static const int offsets[2] = { GLYPH_OBJ_OFF, GLYPH_OBJ_PILETOP_OFF };
    int j;

    for (j = 0; j < SIZE(offsets); j++) {
        glyph_map *obj_glyphs = glyphmap + offsets[j];
        int i;
#ifdef ENHANCED_SYMBOLS
        struct unicode_representation *tmp_u[NUM_OBJECTS];
#endif
        uint32 tmp_customcolor[NUM_OBJECTS];
        uint16 tmp_color256idx[NUM_OBJECTS];

        int duplicate[NUM_OBJECTS];

        for (i = 0; i < NUM_OBJECTS; i++) {
            duplicate[i] = -1;
#ifdef ENHANCED_SYMBOLS
            tmp_u[i] = (struct unicode_representation *) 0;
#endif
            tmp_customcolor[i] = 0;
            tmp_color256idx[i] = 0;
        }
        for (i = 0; i < NUM_OBJECTS; i++) {
            int idx = objects[i].oc_descr_idx;

            /*
             * Shuffling gem appearances can cause the same oc_descr_idx to
             * appear more than once. Detect this condition and ensure that
             * each pointer points to a unique allocation.
             */
            if (duplicate[idx] >= 0) {
#ifdef ENHANCED_SYMBOLS
                /* Current structure already appears in tmp_u */
                struct unicode_representation *other = tmp_u[duplicate[idx]];
#endif
                uint32 other_customcolor = tmp_customcolor[duplicate[idx]];
                uint16 other_color256idx = tmp_color256idx[duplicate[idx]];

                tmp_customcolor[i] = other_customcolor;
                tmp_color256idx[i] = other_color256idx;
#ifdef ENHANCED_SYMBOLS
                tmp_u[i] = (struct unicode_representation *)
                           alloc(sizeof *tmp_u[i]);
                *tmp_u[i] = *other;
                if (other->utf8str != NULL) {
                    tmp_u[i]->utf8str = (uint8 *)
                                        dupstr((const char *) other->utf8str);
                }
#endif
            } else {
                tmp_customcolor[i] = obj_glyphs[idx].customcolor;
                tmp_color256idx[i] = obj_glyphs[idx].color256idx;
#ifdef ENHANCED_SYMBOLS
                tmp_u[i] = obj_glyphs[idx].u;
#endif
                if (
#ifdef ENHANCED_SYMBOLS
                    obj_glyphs[idx].u != NULL ||
#endif
                    obj_glyphs[idx].customcolor != 0) {
                    duplicate[idx] = i;
#ifdef ENHANCED_SYMBOLS
                    obj_glyphs[idx].u = NULL;
#endif
                    obj_glyphs[idx].customcolor = 0;
                    obj_glyphs[idx].color256idx = 0;
                }
            }
        }
        for (i = 0; i < NUM_OBJECTS; i++) {
            /* Some glyphmaps may not have been transferred */
#ifdef ENHANCED_SYMBOLS
            if (obj_glyphs[i].u != NULL) {
                free(obj_glyphs[i].u->utf8str);
                free(obj_glyphs[i].u);
            }
            obj_glyphs[i].u = tmp_u[i];
#endif
            obj_glyphs[i].customcolor = tmp_customcolor[i];
            obj_glyphs[i].color256idx = tmp_color256idx[i];
        }
    }
}
#endif

struct customization_detail *
find_matching_customization(
    const char *customization_name,
    enum customization_types custtype,
    enum graphics_sets which_set)
{
    struct symset_customization *gdc
        = &gs.sym_customizations[which_set][custtype];

    if ((gdc->custtype == custtype) && gdc->customization_name
        && (strcmp(customization_name, gdc->customization_name) == 0))
        return gdc->details;
    return (struct customization_detail *) 0;
}

void
purge_all_custom_entries(void)
{
    int i;

    for (i = 0; i < NUM_GRAPHICS + 1; ++i) {
        purge_custom_entries(i);
    }
}

void
purge_custom_entries(enum graphics_sets which_set)
{
    enum customization_types custtype;
    struct symset_customization *gdc;
    struct customization_detail *details, *next;

    for (custtype = custom_none; custtype < custom_count; ++custtype) {
        gdc = &gs.sym_customizations[which_set][custtype];
        details = gdc->details;
        while (details) {
            next = details->next;
            if (gdc->custtype == custom_ureps) {
                if (details->content.urep.u.utf8str)
                    free(details->content.urep.u.utf8str);
                details->content.urep.u.utf8str = (uint8 *) 0;
            } else if (gdc->custtype == custom_symbols) {
                details->content.sym.symparse = (struct symparse *) 0;
                details->content.sym.val = 0;
            } else if (gdc->custtype == custom_nhcolor) {
                details->content.ccolor.nhcolor = 0;
                details->content.ccolor.glyphidx = 0;
            }
            free(details);
            details = next;
        }
        gdc->details = 0;
        gdc->details_end = 0;
        if (gdc->customization_name) {
            free((genericptr_t) gdc->customization_name);
            gdc->customization_name = 0;
        }
        gdc->count = 0;
    }
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

void
wizcustom_glyphids(winid win)
{
    int glyphnum;
    char *id;

    if (!glyphid_cache)
        return;
    for (glyphnum = 0; glyphnum < MAX_GLYPH; ++glyphnum) {
        id = find_glyphid_in_cache_by_glyphnum(glyphnum);
        if (id) {
            wizcustom_callback(win, glyphnum, id);
        }
    }
 }

staticfn int
parse_id(
    const char *id,
    struct find_struct *findwhat)
{
    FILE *fp = (FILE *) 0;
    int i = 0, j, mnum, glyph,
        pm_offset = 0, oc_offset = 0, cmap_offset = 0,
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
            const char *buf2, *buf3, *buf4;

            /* individual matching glyph entries */
            for (glyph = 0; glyph < MAX_GLYPH; ++glyph) {
                skip_base = FALSE;
                skip_this_one = FALSE;
                buf[0][0] = buf[1][0] = buf[2][0] = buf[3][0] = '\0';
                if (glyph_is_monster(glyph)) {
                    /* buf2 will hold the distinguishing prefix */
                    /* buf3 will hold the base name */
                    buf2 = "";
                    buf3 = monsdump[glyph_to_mon(glyph)].nm;

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
                    buf2 = ""; /* superfluous */
                    buf3 = monsdump[glyph_to_body_corpsenm(glyph)].nm;
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
                    buf2 = "";
                    buf3 = monsdump[glyph_to_statue_corpsenm(glyph)].nm;
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
                    buf2 = "";
                    buf3 = "";
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
                    buf2 = "";
                    buf3 = "";
                    buf4 = "";
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
                        static const char *const altar_text[] = {
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
                        static const char *const zap_texts[] = {
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
                        static const char *const swallow_texts[] = {
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
                        static const char *const expl_type_texts[] = {
                            "dark",    "noxious", "muddy",  "wet",
                            "magical", "fiery",   "frosty",
                        };
                        static const char *const expl_texts[] = {
                            "tl", "tc", "tr", "ml", "mc",
                            "mr", "bl", "bc", "br",
                        };
                        int expl;

                        j = glyph - GLYPH_EXPLODE_OFF;
                        expl = j / ((S_expl_br - S_expl_tl) + 1);
                        cmap = glyph_to_explosion(glyph) + S_expl_tl;
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

/* extern glyph_map glyphmap[MAX_GLYPH]; */

void
clear_all_glyphmap_colors(void)
{
    int glyph;

    for (glyph = 0; glyph < MAX_GLYPH; ++glyph) {
        if (glyphmap[glyph].customcolor)
            glyphmap[glyph].customcolor = 0;
        glyphmap[glyph].color256idx = 0;
    }
}

void reset_customcolors(void)
{
    clear_all_glyphmap_colors();
    apply_customizations(gc.currentgraphics, do_custom_colors);
}

/* not used yet */

#if 0
staticfn struct customization_detail *find_display_sym_customization(
    const char *customization_name, const struct symparse *symparse,
    enum graphics_sets which_set);
staticfn struct customization_detail *find_display_urep_customization(
    const char *customization_name, int glyphidx,
    enum graphics_sets which_set);

struct customization_detail *
find_display_sym_customization(
    const char *customization_name,
    const struct symparse *symparse,
    enum graphics_sets which_set)
{
    struct symset_customization *gdc;
    struct customization_detail *symdetails;

    gdc = &gs.sym_customizations[which_set][custom_symbols];
    if ((gdc->custtype == custom_symbols)
        && (strcmp(customization_name, gdc->customization_name) == 0)) {
        symdetails = gdc->details;
        while (symdetails) {
            if (symdetails->content.sym.symparse == symparse)
                return symdetails;
            symdetails = symdetails->next;
        }
    }
    return (struct customization_detail *) 0;
}

struct customization_detail *
find_display_urep_customization(
    const char *customization_name,
    int glyphidx,
    enum graphics_sets which_set)
{
    struct symset_customization *gdc = &gs.sym_customizations[which_set];
    struct customization_detail *urepdetails;

    if ((gdc->custtype == custom_reps)
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
#endif  /* 0 not used yet */

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

staticfn void
just_find_callback(int glyph UNUSED, struct find_struct *findwhat UNUSED)
{
    return;
}

staticfn int
find_glyphs(const char *id)
{
    struct find_struct find_only = zero_find;

    find_only.unicode_val = 0;
    find_only.callback = just_find_callback;
    return glyph_find_core(id, &find_only);
}

staticfn void
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
    to_unicode.color = (clr == -1) ? 0L : (clr == 0L) ? nonzero_black : clr;
    return glyph_find_core(id, &to_unicode);
}

#endif /* SOME TEST STUFF */

/* glyphs.c */



