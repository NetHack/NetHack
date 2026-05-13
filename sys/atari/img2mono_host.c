/*
 * img2mono_host.c - Convert a colour XIMG to monochrome using Atkinson dithering.
 * Usage: img2mono_host input.img output.img
 * This is a HOST tool.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static int
clamp(int v) { return v < 0 ? 0 : v > 255 ? 255 : v; }

static void
die_eof(void)
{
    fprintf(stderr, "Unexpected EOF in IMG data\n");
    exit(1);
}

static void
wr16(FILE *f, uint16_t v)
{
    uint8_t b[2] = { v >> 8, v & 0xff };
    fwrite(b, 1, 2, f);
}

static uint16_t
rd16(FILE *f)
{
    uint8_t b[2];
    if (fread(b, 1, 2, f) != 2)
        die_eof();
    return (b[0] << 8) | b[1];
}

int
main(int argc, char **argv)
{
    FILE *inf, *outf;
    uint16_t version, headlen, nplanes, pat_len, pix_w, pix_h, img_w, img_h;
    uint32_t magic;
    uint16_t paltype;
    int ncolors, i, x, y, p;
    uint8_t *pal_r, *pal_g, *pal_b;
    int word_aligned, width;
    uint8_t *imgdata;
    long plane_size;
    int16_t *grey;   /* signed for dithering error */
    uint8_t *mono;   /* output 1-bit per pixel */

    if (argc != 3) {
        fprintf(stderr, "Usage: %s input.img output.img\n", argv[0]);
        return 1;
    }

    inf = fopen(argv[1], "rb");
    if (!inf) { perror(argv[1]); return 1; }

    /* Read IMG header */
    version = rd16(inf);
    headlen = rd16(inf);
    nplanes = rd16(inf);
    pat_len = rd16(inf);
    pix_w = rd16(inf);
    pix_h = rd16(inf);
    img_w = rd16(inf);
    img_h = rd16(inf);

    /* Read XIMG extension */
    magic = ((uint32_t)rd16(inf) << 16) | rd16(inf);
    paltype = rd16(inf);

    if (magic != 0x58494D47 || paltype != 0) {
        fprintf(stderr, "Not an XIMG RGB file\n");
        fclose(inf);
        return 1;
    }

    if (nplanes < 1 || nplanes > 8) {
        fprintf(stderr, "Unsupported plane count %u\n", nplanes);
        fclose(inf);
        return 1;
    }

    ncolors = 1 << nplanes;
    pal_r = calloc(ncolors, 1);
    pal_g = calloc(ncolors, 1);
    pal_b = calloc(ncolors, 1);

    /* Read palette (VDI 0-1000 range) */
    for (i = 0; i < ncolors; i++) {
        pal_r[i] = (uint8_t)(rd16(inf) * 255 / 1000);
        pal_g[i] = (uint8_t)(rd16(inf) * 255 / 1000);
        pal_b[i] = (uint8_t)(rd16(inf) * 255 / 1000);
    }

    /* Seek to bitmap data */
    fseek(inf, (long)headlen * 2, SEEK_SET);

    /* Read compressed bitmap data */
    word_aligned = ((img_w + 15) / 16) * 2;
    width = (img_w + 7) / 8;
    plane_size = (long)word_aligned * img_h;
    imgdata = calloc(1, plane_size * nplanes);

    for (y = 0; y < img_h; y++) {
        int scan_repeat = 1;
        for (p = 0; p < nplanes; p++) {
            uint8_t *to = imgdata + (long)(y + p * img_h) * word_aligned;
            uint8_t *endline = to + width;
            /* simplified IMG decompression */
            while (to < endline) {
                int opcode = fgetc(inf);
                if (opcode == EOF)
                    die_eof();
                if (opcode == 0) {
                    int next = fgetc(inf);
                    if (next == EOF)
                        die_eof();
                    if (next != 0) {
                        /* pattern repeat */
                        uint8_t pat[16];
                        if (pat_len > (int) sizeof pat
                            || to + (long) pat_len * next > endline
                            || fread(pat, pat_len, 1, inf) != 1)
                            die_eof();
                        memcpy(to, pat, pat_len);
                        to += pat_len;
                        while (--next) {
                            memcpy(to, pat, pat_len);
                            to += pat_len;
                        }
                    } else {
                        if (fgetc(inf) != 0xFF)
                            die_eof();
                        scan_repeat = fgetc(inf);
                        if (scan_repeat == EOF)
                            die_eof();
                    }
                } else if (opcode == 0x80) {
                    int count = fgetc(inf);
                    if (count == EOF
                        || to + count > endline
                        || fread(to, count, 1, inf) != 1)
                        die_eof();
                    to += count;
                } else {
                    int count = opcode & 0x7F;
                    uint8_t fill = (opcode & 0x80) ? 0xFF : 0x00;
                    if (to + count > endline)
                        die_eof();
                    memset(to, fill, count);
                    to += count;
                }
            }
        }
        /* handle scan_repeat */
        if (scan_repeat > 1) {
            for (p = 0; p < nplanes; p++) {
                uint8_t *src_line = imgdata + (long)(y + p * img_h) * word_aligned;
                for (i = 1; i < scan_repeat && (y + i) < img_h; i++) {
                    uint8_t *dst = imgdata + (long)(y + i + p * img_h) * word_aligned;
                    memcpy(dst, src_line, width);
                }
            }
            y += scan_repeat - 1;
        }
    }
    fclose(inf);

    /* Render to greyscale */
    grey = calloc(img_w * img_h, sizeof(int16_t));
    for (y = 0; y < img_h; y++) {
        for (x = 0; x < img_w; x++) {
            int pix_val = 0;
            int bit = 7 - (x & 7);
            int byte_off = y * word_aligned + x / 8;
            for (p = 0; p < nplanes; p++) {
                if (imgdata[p * plane_size + byte_off] & (1 << bit))
                    pix_val |= (1 << p);
            }
            /* Convert to luminance */
            int lum = (pal_r[pix_val] * 299 + pal_g[pix_val] * 587
                       + pal_b[pix_val] * 114) / 1000;
            grey[y * img_w + x] = (int16_t)lum;
        }
    }
    free(imgdata);
    free(pal_r); free(pal_g); free(pal_b);

    /* Atkinson dither to 1-bit */
    mono = calloc(img_w * img_h, 1);
    for (y = 0; y < img_h; y++) {
        for (x = 0; x < img_w; x++) {
            int idx = y * img_w + x;
            int old = grey[idx];
            int new_val = (old < 128) ? 0 : 255;
            int err = (old - new_val) / 8;
            mono[idx] = (new_val == 0) ? 1 : 0; /* 1=black, 0=white */

            #define ATK(dx, dy) do { \
                int nx = x+(dx), ny = y+(dy); \
                if (nx >= 0 && nx < img_w && ny >= 0 && ny < img_h) \
                    grey[ny * img_w + nx] = (int16_t)clamp(grey[ny * img_w + nx] + err); \
            } while(0)
            ATK(1,0); ATK(2,0); ATK(-1,1); ATK(0,1); ATK(1,1); ATK(0,2);
            #undef ATK
        }
    }
    free(grey);

    /* Write monochrome XIMG */
    outf = fopen(argv[2], "wb");
    if (!outf) { perror(argv[2]); return 1; }

    int out_headlen = 8 + 3 + 2 * 3; /* 2 colours */
    int rbw = ((img_w + 15) / 16) * 2;

    wr16(outf, 1);            /* version */
    wr16(outf, out_headlen);  /* header length */
    wr16(outf, 1);            /* 1 plane */
    wr16(outf, 2);            /* pattern length */
    wr16(outf, pix_w);        /* pixel width */
    wr16(outf, pix_h);        /* pixel height */
    wr16(outf, img_w);
    wr16(outf, img_h);
    wr16(outf, 0x5849); wr16(outf, 0x4D47); /* XIMG */
    wr16(outf, 0);            /* RGB palette */
    /* palette: white, black */
    wr16(outf, 1000); wr16(outf, 1000); wr16(outf, 1000);
    wr16(outf, 0); wr16(outf, 0); wr16(outf, 0);

    /* Write bitmap */
    {
        uint8_t *buf = malloc(rbw);
        if (!buf) { perror("malloc"); return 1; }
        for (y = 0; y < img_h; y++) {
            int left = rbw;
            uint8_t *src = buf;
            memset(buf, 0, rbw);
            for (x = 0; x < img_w; x++) {
                if (mono[y * img_w + x])
                    buf[x / 8] |= (0x80 >> (x & 7));
            }
            /* IMG literal-string opcode has an 8-bit count -- chunk if rbw>255 */
            while (left > 0) {
                int chunk = left > 255 ? 255 : left;
                fputc(0x80, outf);
                fputc(chunk, outf);
                fwrite(src, 1, chunk, outf);
                src += chunk;
                left -= chunk;
            }
        }
        free(buf);
    }
    fclose(outf);
    free(mono);

    printf("%s: %dx%d mono [Atkinson]\n", argv[2], img_w, img_h);
    return 0;
}
