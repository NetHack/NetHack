/*
 * $NHDT-Date: 1432512809 2015/05/25 00:13:29 $  $NHDT-Branch: master $:$NHDT-Revision: 1.5 $
 */
#define __TCC_COMPAT__
#include <stdio.h>
#include <string.h>
#include <osbind.h>
#include <memory.h>
#include <aesbind.h>
#include <vdibind.h>
#include <gemfast.h>
#include <e_gem.h>
#include "load_img.h"

#ifndef FALSE
#define FALSE 0
#define TRUE !FALSE
#endif

/* VDI <-> Device palette order conversion matrixes: */
/* Four-plane vdi-device */
int vdi2dev4[] = { 0, 15, 1, 2, 4, 6, 3, 5, 7, 8, 9, 10, 12, 14, 11, 13 };
/* Two-plane vdi-device */
int vdi2dev2[] = { 0, 3, 1, 2 };

void
get_colors(int handle, short *palette, int col)
{
    int i, idx;

    /* get current color palette */
    for (i = 0; i < col; i++) {
        /* device->vdi->device palette order */
        switch (planes) {
        case 1:
            idx = i;
            break;
        case 2:
            idx = vdi2dev2[i];
            break;
        case 4:
            idx = vdi2dev4[i];
            break;
        default:
            if (i < 16)
                idx = vdi2dev4[i];
            else
                idx = i == 255 ? 1 : i;
        }
        vq_color(handle, i, 0, (int *) palette + idx * 3);
    }
}

void
img_set_colors(int handle, short *palette, int col)
{
    int i, idx, end;

    /* set color palette */
    end = min(1 << col, 1 << planes);
    for (i = 0; i < end; i++) {
        switch (planes) { /* MAR -- war col 10.01.2001 */
        case 1:
            idx = i;
            break;
        case 2:
            idx = vdi2dev2[i];
            break;
        case 4:
            idx = vdi2dev4[i];
            break;
        default:
            if (i < 16)
                idx = vdi2dev4[i];
            else
                idx = i == 255 ? 1 : i;
        }
        vs_color(handle, i, (int *) palette + idx * 3);
    }
}

int
convert(MFDB *image, long size)
{
    int plane, mplanes;
    char *line_addr, *buf_addr, *new_addr, *new1_addr, *image_addr,
        *screen_addr;
    MFDB dev_form, tmp;
    long new_size;

    /* convert size from words to bytes */
    size <<= 1;

    /* memory for the device raster */
    new_size = size * (long) planes;
    if ((new_addr = (char *) calloc(1, new_size)) == NULL)
        return (FALSE);

    /* initialize MFDBs */
    tmp = *image;
    tmp.fd_nplanes = planes;
    tmp.fd_addr = new_addr;
    tmp.fd_stand = 1; /* standard format */
    dev_form = tmp;
    screen_addr = new_addr;
    dev_form.fd_stand = 0; /* device format */
    image_addr = (char *) image->fd_addr;

    /* initialize some variables and zero temp. line buffer */
    mplanes = min(image->fd_nplanes, planes);
    /* convert image */
    line_addr = image_addr;
    buf_addr = screen_addr;
    if (mplanes > 1) {
        /* cut/pad color planes into temp buf */
        for (plane = 0; plane < mplanes; plane++) {
            memcpy(buf_addr, line_addr, size);
            line_addr += size;
            buf_addr += size;
        }
    } else {
        /* fill temp line bitplanes with a b&w line */
        for (plane = 0; plane < planes; plane++) {
            memcpy(buf_addr, line_addr, size);
            buf_addr += size;
        }
    }
    free(image->fd_addr);
    /* convert image line in temp into current device raster format */
    if ((new1_addr = (char *) calloc(1, new_size)) == NULL)
        return (FALSE);
    dev_form.fd_addr = new1_addr;
    vr_trnfm(x_handle, &tmp, &dev_form);
    free(new_addr);

    /* change image description */
    image->fd_stand = 0; /* device format */
    image->fd_addr = new1_addr;
    image->fd_nplanes = planes;
    return (TRUE);
}

int
transform_img(MFDB *image)
{ /* return FALSE if transform_img fails */
    int success;
    long size;

    if (!image->fd_addr)
        return (FALSE);

    size = (long) ((long) image->fd_wdwidth * (long) image->fd_h);
    success = convert(
        image, size); /* Use vr_trfm(), which needs quite a lot memory. */
    if (success)
        return (TRUE);
    /*	else				show_error(ERR_ALLOC);	*/
    return (FALSE);
}

/* Loads & depacks IMG (0 if succeded, else error). */
/* Bitplanes are one after another in address IMG_HEADER.addr. */
int
depack_img(char *name, IMG_header *pic)
{
    int b, line, plane, width, word_aligned, opcode, patt_len, pal_size,
        byte_repeat, patt_repeat, scan_repeat, error = FALSE;
    char *pattern, *to, *endline, *puffer, sol_pat;
    long size;
    FILE *fp;

    if ((fp = fopen(name, "rb")) == NULL)
        return (ERR_FILE);

    setvbuf(fp, NULL, _IOLBF, BUFSIZ);

    /* read header info (bw & ximg) into image structure */
    fread((char *) &(pic->version), 2, 8 + 3, fp);

    /* only 2-256 color imgs */
    if (pic->planes < 1 || pic->planes > 8) {
        error = ERR_COLOR;
        goto end_depack;
    }

    /* if XIMG, read info */
    if (pic->magic == XIMG && pic->paltype == 0) {
        pal_size = (1 << pic->planes) * 3 * 2;
        if ((pic->palette = (short *) calloc(1, pal_size))) {
            fread((char *) pic->palette, 1, pal_size, fp);
        }
    } else {
        pic->palette = NULL;
    }

    /* width in bytes word aliged */
    word_aligned = (pic->img_w + 15) >> 4;
    word_aligned <<= 1;

    /* width byte aligned */
    width = (pic->img_w + 7) >> 3;

    /* allocate memory for the picture */
    free(pic->addr);
    size = (long) ((long) word_aligned * (long) pic->img_h
                   * (long) pic->planes); /*MAR*/

    /* check for header validity & malloc long... */
    if (pic->length > 7 && pic->planes < 33 && pic->img_w > 0
        && pic->img_h > 0) {
        if (!(pic->addr = (char *) calloc(1, size))) {
            error = ERR_ALLOC;
            goto end_depack;
        }
    } else {
        error = ERR_HEADER;
        goto end_depack;
    }

    patt_len = pic->pat_len;

    /* jump over the header and possible (XIMG) info */
    fseek(fp, (long) pic->length * 2L, SEEK_SET);

    for (line = 0, to = pic->addr; line < pic->img_h;
         line += scan_repeat) { /* depack whole img */
        for (plane = 0, scan_repeat = 1; plane < pic->planes;
             plane++) { /* depack one scan line */
            puffer = to =
                pic->addr
                + (long) (line + plane * pic->img_h) * (long) word_aligned;
            endline = puffer + width;
            do { /* depack one line in one bitplane */
                switch ((opcode = fgetc(fp))) {
                case 0: /* pattern or scan repeat */
                    if ((patt_repeat = fgetc(fp))) { /* repeat a pattern */
                        fread(to, patt_len, 1, fp);
                        pattern = to;
                        to += patt_len;
                        while (--patt_repeat) { /* copy pattern */
                            memcpy(to, pattern, patt_len);
                            to += patt_len;
                        }
                    } else { /* repeat a line */
                        if (fgetc(fp) == 0xFF)
                            scan_repeat = fgetc(fp);
                        else {
                            error = ERR_DEPACK;
                            goto end_depack;
                        }
                    }
                    break;
                case 0x80: /* Literal */
                    byte_repeat = fgetc(fp);
                    fread(to, byte_repeat, 1, fp);
                    to += byte_repeat;
                    break;
                default: /* Solid run */
                    byte_repeat = opcode & 0x7F;
                    sol_pat = opcode & 0x80 ? 0xFF : 0x00;
                    while (byte_repeat--)
                        *to++ = sol_pat;
                }
            } while (to < endline);

            if (to == endline) {
                /* ensure that lines aren't repeated past the end of the img
                 */
                if (line + scan_repeat > pic->img_h)
                    scan_repeat = pic->img_h - line;
                /* copy line to image buffer */
                if (scan_repeat > 1) {
                    /* calculate address of a current line in a current
                     * bitplane */
                    /*					to=pic->addr+(long)(line+1+plane*pic->img_h)*(long)word_aligned;*/
                    for (b = scan_repeat - 1; b; --b) {
                        memcpy(to, puffer, width);
                        to += word_aligned;
                    }
                }
            } else {
                error = ERR_DEPACK;
                goto end_depack;
            }
        }
    }

end_depack:
    fclose(fp);
    return (error);
}

int
half_img(MFDB *s, MFDB *d)
{
    int pxy[8], i, j;
    MFDB tmp;

    mfdb(&tmp, NULL, s->fd_w / 2, s->fd_h, s->fd_stand, s->fd_nplanes);
    tmp.fd_w = s->fd_w / 2;
    tmp.fd_addr = calloc(1, mfdb_size(&tmp));
    if (!tmp.fd_addr)
        return (FALSE);

    pxy[1] = pxy[5] = 0;
    pxy[3] = pxy[7] = s->fd_h - 1;
    for (i = 0; i < s->fd_w / 2; i++) {
        pxy[0] = pxy[2] = 2 * i;
        pxy[4] = pxy[6] = i;
        vro_cpyfm(x_handle, S_ONLY, pxy, s, &tmp);
    }
    pxy[0] = pxy[4] = 0;
    pxy[2] = pxy[6] = s->fd_w / 2 - 1;
    for (j = 0; j < s->fd_h / 2; j++) {
        pxy[1] = pxy[3] = 2 * j;
        pxy[5] = pxy[7] = j;
        vro_cpyfm(x_handle, S_ONLY, pxy, &tmp, d);
    }
    free(tmp.fd_addr);
    return (TRUE);
}
