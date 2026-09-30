/* NetHack 5.0	tile2pict_quant.h	*/
/* Copyright (c) Ingo Paschke, 2026. */
/* NetHack may be freely redistributed.  See license for details. */
/* tile2pict_quant.h — host-only color quantization helpers for tile2pict.
 * Implementations are inline in this header (single-translation-unit tool).
 */

#ifndef TILE2PICT_QUANT_H
#define TILE2PICT_QUANT_H

#include <stdlib.h>
#include <string.h>

/* Reduce a palette of n RGB triples (src[3*n]) to at most max_out triples
 * via median-cut. usage_hist[i] gives the pixel-count using src color i.
 * On output: out[3*max_out] holds representative RGB triples.
 * Returns the number of out colors actually used (<= max_out).
 */
static int median_cut(const unsigned char *src,
                      const unsigned long *usage_hist,
                      int n, int max_out,
                      unsigned char *out);

/* Re-color an 8bpp image (palette indices) into 4bpp using Floyd-Steinberg
 * error diffusion against a 16-color target palette.
 * src8[w*h]: source palette indices (0..src_n-1).
 * src_pal[3*src_n]: source RGB triples.
 * dst_pal[3*dst_n]: target RGB triples (dst_n <= 16).
 * dst4[w*h]: destination indices in 0..dst_n-1 (one byte per pixel; PACKING
 *   into 4bpp happens later in tile2pict.c, not here).
 */
static void dither_fs(const unsigned char *src8, int w, int h,
                      const unsigned char *src_pal, int src_n,
                      const unsigned char *dst_pal, int dst_n,
                      unsigned char *dst4);

/* -----------------------------------------------------------------------
 * median_cut implementation
 * ----------------------------------------------------------------------- */

/* One bucket: indices into the src[] palette sorted along a chosen channel. */
typedef struct {
    int members[256]; /* indices into src[] */
    int count;        /* number of members */
} MC_Bucket;

/* Sort helper state (qsort_r is non-portable; use a global for simplicity). */
static const unsigned char *mc_sort_src;
static int mc_sort_ch;

static int mc_cmp(const void *a, const void *b)
{
    int ia = *(const int *)a;
    int ib = *(const int *)b;
    int va = mc_sort_src[3 * ia + mc_sort_ch];
    int vb = mc_sort_src[3 * ib + mc_sort_ch];
    return va - vb;
}

static int median_cut(const unsigned char *src,
                      const unsigned long *usage_hist,
                      int n, int max_out,
                      unsigned char *out)
{
    MC_Bucket buckets[16];
    int nb = 0; /* number of buckets in use */
    int i, k;

    if (n <= 0 || max_out <= 0) return 0;
    if (max_out > 16) max_out = 16;

    /* Initialise bucket 0 with all source colors (sorted by first channel). */
    buckets[0].count = 0;
    for (i = 0; i < n; ++i)
        buckets[0].members[buckets[0].count++] = i;
    nb = 1;

    /* Repeatedly split until we have max_out buckets or can't split further. */
    while (nb < max_out) {
        /* Find the bucket with the largest range along any channel. */
        int best_b = -1, best_ch = 0, best_range = 0;
        for (k = 0; k < nb; ++k) {
            if (buckets[k].count <= 1) continue;
            int ch;
            for (ch = 0; ch < 3; ++ch) {
                int mn = 255, mx = 0, m;
                for (m = 0; m < buckets[k].count; ++m) {
                    int idx = buckets[k].members[m];
                    int v = src[3 * idx + ch];
                    if (v < mn) mn = v;
                    if (v > mx) mx = v;
                }
                int r = mx - mn;
                if (r > best_range) {
                    best_range = r;
                    best_b = k;
                    best_ch = ch;
                }
            }
        }
        if (best_b < 0 || best_range == 0) break; /* all degenerate */

        /* Sort best bucket's members along best_ch. */
        mc_sort_src = src;
        mc_sort_ch = best_ch;
        qsort(buckets[best_b].members, (size_t) buckets[best_b].count,
              sizeof(int), mc_cmp);

        /* Find weighted median: split where cumulative weight >= total/2. */
        unsigned long total_w = 0, cum_w = 0;
        int m;
        for (m = 0; m < buckets[best_b].count; ++m)
            total_w += usage_hist[buckets[best_b].members[m]];
        int split = 1; /* at least one member in first half */
        for (m = 0; m < buckets[best_b].count - 1; ++m) {
            cum_w += usage_hist[buckets[best_b].members[m]];
            if (cum_w * 2 >= total_w) {
                split = m + 1;
                break;
            }
        }

        /* Create new bucket from the upper half. */
        MC_Bucket *src_bkt = &buckets[best_b];
        MC_Bucket *new_bkt = &buckets[nb];
        new_bkt->count = src_bkt->count - split;
        memcpy(new_bkt->members, src_bkt->members + split,
               (size_t) new_bkt->count * sizeof(int));
        src_bkt->count = split;
        nb++;
    }

    /* Compute representative (weighted mean) for each bucket. */
    for (k = 0; k < nb; ++k) {
        unsigned long sr = 0, sg = 0, sb2 = 0, total_w = 0;
        int m;
        for (m = 0; m < buckets[k].count; ++m) {
            int idx = buckets[k].members[m];
            unsigned long w = usage_hist[idx] ? usage_hist[idx] : 1;
            sr += (unsigned long) src[3 * idx + 0] * w;
            sg += (unsigned long) src[3 * idx + 1] * w;
            sb2 += (unsigned long) src[3 * idx + 2] * w;
            total_w += w;
        }
        out[3 * k + 0] = (unsigned char) (sr / total_w);
        out[3 * k + 1] = (unsigned char) (sg / total_w);
        out[3 * k + 2] = (unsigned char) (sb2 / total_w);
    }

    return nb;
}

/* -----------------------------------------------------------------------
 * dither_fs implementation
 * ----------------------------------------------------------------------- */

/* Find nearest dst_pal entry to (tr, tg, tb) by squared Euclidean distance. */
static int nearest_color(int tr, int tg, int tb,
                         const unsigned char *dst_pal, int dst_n)
{
    int best = 0, j;
    long best_d = 0x7FFFFFFF;
    for (j = 0; j < dst_n; ++j) {
        long dr = tr - (int) dst_pal[3 * j + 0];
        long dg = tg - (int) dst_pal[3 * j + 1];
        long db = tb - (int) dst_pal[3 * j + 2];
        long d = dr * dr + dg * dg + db * db;
        if (d < best_d) { best_d = d; best = j; }
    }
    return best;
}

static void dither_fs(const unsigned char *src8, int w, int h,
                      const unsigned char *src_pal, int src_n,
                      const unsigned char *dst_pal, int dst_n,
                      unsigned char *dst4)
{
    /* Error buffer: 2 rows × w pixels × 3 channels (signed int). */
    int *err;
    int row_stride = w * 3;
    int x, y;

    (void) src_n; /* all src8 values assumed valid */

    err = (int *) calloc((size_t) 2 * (size_t) w * 3, sizeof(int));
    if (!err) {
        /* fallback: no dithering, just nearest-color */
        int i;
        for (i = 0; i < w * h; ++i) {
            int s = src8[i];
            dst4[i] = (unsigned char) nearest_color(
                src_pal[3*s+0], src_pal[3*s+1], src_pal[3*s+2],
                dst_pal, dst_n);
        }
        return;
    }

    for (y = 0; y < h; ++y) {
        int cur = y & 1;       /* current row slot in err[] */
        int nxt = 1 - cur;     /* next row slot */

        /* Zero out the next row's error before we fill it. */
        memset(err + nxt * row_stride, 0, (size_t) row_stride * sizeof(int));

        for (x = 0; x < w; ++x) {
            int s = src8[(size_t) y * w + x];
            /* Target RGB = source palette color + accumulated error. */
            int tr = (int) src_pal[3*s+0] + err[cur * row_stride + x * 3 + 0];
            int tg = (int) src_pal[3*s+1] + err[cur * row_stride + x * 3 + 1];
            int tb = (int) src_pal[3*s+2] + err[cur * row_stride + x * 3 + 2];

            /* Clamp to [0,255] before lookup. */
            if (tr < 0) tr = 0; else if (tr > 255) tr = 255;
            if (tg < 0) tg = 0; else if (tg > 255) tg = 255;
            if (tb < 0) tb = 0; else if (tb > 255) tb = 255;

            int chosen = nearest_color(tr, tg, tb, dst_pal, dst_n);
            dst4[(size_t) y * w + x] = (unsigned char) chosen;

            /* Quantization error. */
            int er = tr - (int) dst_pal[3*chosen+0];
            int eg = tg - (int) dst_pal[3*chosen+1];
            int eb = tb - (int) dst_pal[3*chosen+2];

            /* Distribute: 7/16 right, 3/16 lower-left, 5/16 below, 1/16 lower-right. */
            if (x + 1 < w) {
                err[cur * row_stride + (x+1) * 3 + 0] += er * 7 / 16;
                err[cur * row_stride + (x+1) * 3 + 1] += eg * 7 / 16;
                err[cur * row_stride + (x+1) * 3 + 2] += eb * 7 / 16;
            }
            if (x - 1 >= 0) {
                err[nxt * row_stride + (x-1) * 3 + 0] += er * 3 / 16;
                err[nxt * row_stride + (x-1) * 3 + 1] += eg * 3 / 16;
                err[nxt * row_stride + (x-1) * 3 + 2] += eb * 3 / 16;
            }
            err[nxt * row_stride + x * 3 + 0] += er * 5 / 16;
            err[nxt * row_stride + x * 3 + 1] += eg * 5 / 16;
            err[nxt * row_stride + x * 3 + 2] += eb * 5 / 16;
            if (x + 1 < w) {
                err[nxt * row_stride + (x+1) * 3 + 0] += er * 1 / 16;
                err[nxt * row_stride + (x+1) * 3 + 1] += eg * 1 / 16;
                err[nxt * row_stride + (x+1) * 3 + 2] += eb * 1 / 16;
            }
        }
    }

    free(err);
}

#endif /* TILE2PICT_QUANT_H */
