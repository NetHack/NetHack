/* NetHack 3.7	utf8map.c	*/
/* Copyright (c) Michael Allison, 2021. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include <ctype.h>

#ifdef ENHANCED_SYMBOLS

extern const struct symparse loadsyms[];
extern struct enum_dump monsdump[];
extern struct enum_dump objdump[];
extern glyph_map glyphmap[MAX_GLYPH];
extern const char *const known_handling[];        /* symbols.c */

/* hexdd[] is defined in decl.c */

int
unicode_val(const char *cp)
{
    const char *dp;
    int cval = 0, dcount;

    if (cp && *cp) {
        cval = dcount = 0;
        if ((*cp == 'U' || *cp == 'u')
            && cp[1] == '+' && cp[2] && (dp = strchr(hexdd, cp[2])) != 0) {
            cp += 2; /* move past the 'U' and '+' */
            do {
                cval = (cval * 16) + ((int) (dp - hexdd) / 2);
            } while (*++cp && (dp = strchr(hexdd, *cp)) != 0 && ++dcount < 7);
        }
    }
    return cval;
}

int
set_map_u(glyph_map *gmap, uint32 utf32ch, const uint8 *utf8str)
{
    glyph_map *tmpgm = gmap;

    if (!tmpgm || !utf32ch)
        return 0;

    if (gmap->u == 0) {
        gmap->u =
            (struct unicode_representation *) alloc(sizeof *gmap->u);
        gmap->u->utf8str = 0;
    }
    if (gmap->u->utf8str != 0) {
        free(gmap->u->utf8str);
        gmap->u->utf8str = 0;
    }
    gmap->u->utf8str = (uint8 *) dupstr((const char *) utf8str);
    gmap->u->utf32ch = utf32ch;
    return 1;
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

int
add_custom_urep_entry(
    const char *customization_name,
    int glyphidx,
    uint32 utf32ch,
    const uint8 *utf8str,
    enum graphics_sets which_set)
{
    struct symset_customization *gdc
        = &gs.sym_customizations[which_set][custom_ureps];
    struct customization_detail *details, *newdetails = 0;


    if (!gdc->details) {
        gdc->customization_name = dupstr(customization_name);
        gdc->custtype = custom_ureps;
        gdc->details = 0;
        gdc->details_end = 0;
    }
    details = find_matching_customization(customization_name,
                                          custom_ureps, which_set); /* FIXME */
    if (details) {
        while (details) {
            if (details->content.urep.glyphidx == glyphidx) {
                if (details->content.urep.u.utf8str)
                    free(details->content.urep.u.utf8str);
                if (utf32ch) {
                    details->content.urep.u.utf8str =
                        (uint8 *) dupstr((const char *) utf8str);
                    details->content.urep.u.utf32ch = utf32ch;
                } else {
                    details->content.urep.u.utf8str = (uint8 *) 0;
                    details->content.urep.u.utf32ch = 0;
                }
                return 1;
            }
            details = details->next;
        }
    }
    /* create new details entry */
    newdetails = (struct customization_detail *) alloc(
                                        sizeof (struct customization_detail));
    newdetails->content.urep.glyphidx = glyphidx;
    if (utf8str && *utf8str) {
        newdetails->content.urep.u.utf8str =
            (uint8 *) dupstr((const char *) utf8str);
    } else {
        newdetails->content.urep.u.utf8str =
            (uint8 *) 0;
    }
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
#endif /* ENHANCED_SYMBOLS */

void reset_customsymbols(void)
{
#ifdef ENHANCED_SYMBOLS
    free_all_glyphmap_u();
    apply_customizations(gc.currentgraphics, do_custom_symbols);
#endif
}

/* utf8map.c */



