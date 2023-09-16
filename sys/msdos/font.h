/* Maintain a data structure describing a monospaced bitmap font */

#ifndef FONT_H
#define FONT_H

/*
 * The file format is Linux PSF, version 2. Version 1 is not supported.
 * Actual Linux fonts are restricted to 256 or 512 glyphs; for NetHack, the
 * font can have any number of glyphs. The Unicode map is expected to be
 * present, but combining sequences are not supported.
 * The fonts supplied for use with this data structure have the first 256
 * glyphs arranged according to IBM code page 437, for simpler support of
 * the IBM handling mode for the map.
 */

/* For Unicode lookup, a three level tree provides constant-time access to
   the glyphs without using an excessive amount of memory.
   The root has seventeen entries, one for each plane of Unicode. Most fonts
   will populate only plane 0, and Unicode defines only planes 0, 1, 2, 3, 15
   and 16.
   The second level has 256 entries, each pointing to a third level node with
   256 entries. Each third level entry has type unsigned, and gives the index
   of the glyph.
   Given the Unicode code point, we can use bits 20 through 16 to index the
   root, bits 15 through 8 for the second level and bits 7 through 0 for the
   third level. */

struct BitmapFont {
    /* Dimensions of a single glyph */
    unsigned width;
    unsigned height;

    /* The glyphs, in the order that they appear in the font */
    /* IBM handling will index the glyphs this way */
    /* glyph points to an allocated array, each element of which points to
       another allocated array */
    unsigned num_glyphs;
    unsigned char **glyphs;

    /* The root node of the Unicode tree */
    unsigned **unicode[17];
};

extern struct BitmapFont *load_font(const char *filename);
extern void free_font(struct BitmapFont *font);
extern const unsigned char *get_font_glyph(
            struct BitmapFont *font,
            uint32 ch,
            boolean unicode);

#endif
