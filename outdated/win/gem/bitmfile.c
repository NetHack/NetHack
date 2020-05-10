/****************************\
* Bitmap mit Farbtabelle als *
* Graphik-Datei speichern		 *
* Autor: Gabriel Schmidt		 *
* (c) 1992 by MAXON-Computer *
* Modifiziert von Sebastian	 *
* Bieber, Dez. 1994					 *
* -> Programmcode						 *
\****************************/
/*
 * $NHDT-Date: 1432512809 2015/05/25 00:13:29 $  $NHDT-Branch: master $:$NHDT-Revision: 1.4 $
 */

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "bitmfile.h"

/* --- (X) IMG-Implementation ----------------- */

#define IMG_COMPRESSED

typedef struct {
    UWORD img_version;
    UWORD img_headlen;
    UWORD img_nplanes;
    UWORD img_patlen;
    UWORD img_pixw;
    UWORD img_pixh;
    UWORD img_w;
    UWORD img_h;
} IMG_HEADER;

typedef enum { NONE, SOLID0, SOLID1, PATRUN, BITSTR } IMG_MODE;

typedef UBYTE IMG_SOLID;

typedef enum { RGB = 0, CMY = 1, Pantone = 2 } XIMG_COLMODEL;

typedef struct {
    ULONG img_ximg;
    XIMG_COLMODEL img_colmodel;
} XIMG_HEADER;

typedef struct RGB XIMG_RGB;

int
bitmap_to_img(FILE_TYP typ, int ww, int wh, unsigned int pixw,
              unsigned int pixh, unsigned int planes, unsigned int colors,
              const char *filename,
              void (*get_color)(unsigned int colind, struct RGB *rgb),
              void (*get_pixel)(int x, int y, unsigned int *colind))
{
    int file, error, cnt;
    IMG_HEADER header;
    XIMG_HEADER xheader;
    XIMG_RGB xrgb;
    IMG_MODE mode;
    UBYTE *line_buf, *write_buf;
    register UBYTE *startpnt, *bufpnt;
    unsigned int colind, line_len, line, bit;
    register unsigned int byte;
    register UBYTE count;

    /* fill in (X) IMG-Header */

    header.img_version = 1;
    header.img_headlen = (UWORD) sizeof(header) / 2;
    if (typ == XIMG)
        header.img_headlen +=
            (UWORD)(sizeof(xheader) + colors * sizeof(xrgb)) / 2;

    header.img_nplanes = planes;
    header.img_patlen = 2;
    header.img_pixw = pixw;
    header.img_pixh = pixh;
    header.img_w = ww;
    header.img_h = wh;

    xheader.img_ximg = XIMG_MAGIC;
    xheader.img_colmodel = RGB;

    /* calculate linelength, allocate buffer. */

    line_len = (ww + 7) / 8;

    line_buf = malloc((size_t) planes * line_len);
    if (line_buf == NULL)
        return (ENOMEM);

    /* Worst case: the bufferd line could grow to max. 3 times the length */
    /* of the original!
     */

    write_buf = malloc((size_t) 3 * line_len);
    if (write_buf == NULL) {
        free(line_buf);
        return (ENOMEM);
    };

    /* open file */

    file = open(filename, O_WRONLY | O_CREAT | O_TRUNC);
    if (file < 0) {
        error = errno;
        free(line_buf);
        free(write_buf);
        return (error);
    };

    /* write Header */

    if (write(file, &header, sizeof(header)) != sizeof(header)
        || (typ == XIMG
            && write(file, &xheader, sizeof(xheader)) != sizeof(xheader))) {
        error = errno;
        close(file);
        free(line_buf);
        free(write_buf);
        return (error);
    };

    /* save the colortable if possible */

    if (typ == XIMG)
        for (cnt = 0; cnt < colors; cnt++) {
            get_color(cnt, &xrgb);
            if (write(file, &xrgb, sizeof(xrgb)) != sizeof(xrgb)) {
                error = errno;
                close(file);
                free(line_buf);
                free(write_buf);
                return (error);
            };
        };

    /* And now line by line ... */

    for (line = 0; line < wh; line++) {
        /* get pixel, split it up and */
        /* store it as planes in buffer */

        for (byte = 0; byte < line_len; byte++) {
            for (cnt = 0; cnt < planes; cnt++)
                line_buf[cnt * line_len + byte] = 0x00;

            for (bit = 0; bit < 8; bit++) {
                if (8 * byte + bit < ww)
                    get_pixel(8 * byte + bit, line, &colind);

                for (cnt = 0; cnt < planes; cnt++) {
                    line_buf[cnt * line_len + byte] <<= 1;
                    line_buf[cnt * line_len + byte] |= colind & 0x01;
                    colind >>= 1;
                };
            };
        };

        /* compress bitstrings in buffer */
        /* and write it to file        */

        for (cnt = 0; cnt < planes; cnt++) {
            /* Bitstringpointer to start of plane */

            startpnt = &line_buf[cnt * line_len];
            bufpnt = write_buf;

            while (startpnt < &line_buf[(cnt + 1) * line_len]) {
                /*********************************************/
                /* Which _new_ compression-mode "fits" the	*/
                /* the current byte?
                 */
                /* Note: the compressing modes get choosen	*/
                /* "positive". The non compressing BITSTR-	*/
                /* mode is choosen only if nothing else 		*/
                /* "fits" ...
                 */
                /*********************************************/

                switch (*startpnt) {
                case 0x00:
                    mode = SOLID0;
                    break;
                case 0xFF:
                    mode = SOLID1;
                    break;
                default:
                    if (startpnt < &line_buf[(cnt + 1) * line_len - 3]
                        && *(startpnt) == *(startpnt + 2)
                        && *(startpnt + 1) == *(startpnt + 3))
                        mode = PATRUN;
                    else
                        mode = BITSTR;
                };

                /************************************************/
                /* The mode is choosen, now work with it.
                 */
                /* The compressing modi stay current as long as	*/
                /* possible.
                 */
                /************************************************/

                count = 0;

                switch (mode) {
                case SOLID0:
                    while ((startpnt < &line_buf[(cnt + 1) * line_len])
                           && (*(startpnt) == 0x00) && (count < 0x7F)) {
                        startpnt++;
                        count++;
                    };
                    *(bufpnt++) = count;
                    break;

                case SOLID1:
                    while ((startpnt < &line_buf[(cnt + 1) * line_len])
                           && (*(startpnt) == 0xFF) && (count < 0x7F)) {
                        startpnt++;
                        count++;
                    };
                    *(bufpnt++) = 0x80 | count;
                    break;

                case PATRUN:
                    *(bufpnt++) = 0x00;
                    startpnt += 2;
                    count = 1;
                    while (startpnt < &line_buf[(cnt + 1) * line_len - 1]
                           && *(startpnt) == *(startpnt - 2)
                           && *(startpnt + 1) == *(startpnt - 1)
                           && (count < 0xFF)) {
                        count++;
                        startpnt += 2;
                    };
                    *(bufpnt++) = count;
                    *(bufpnt++) = *(startpnt - 2);
                    *(bufpnt++) = *(startpnt - 1);
                    break;

                /************************************************/
                /* The while Condition is ment as follows:		*/
                /*																							*/
                /* while ( NOT(2-Byte-Solidrun possible)	&&
                 */
                /*			 NOT(6-Byte-Patternrun possible)	&&
                 */
                /*				 count < 0xFF
                 * &&		*/
                /*			 still Bytes remaining			)
                 */
                /*																							*/
                /* As soon as a _compressing_ alternative	shows	*/
                /* up, BITSTR gets cancelled!
                 */
                /************************************************/

                case BITSTR:
                    *(bufpnt++) = 0x80;
                    while (!(((startpnt + count)
                              < &line_buf[(cnt + 1) * line_len - 1])
                             && (((*(startpnt + count) == 0xFF)
                                  && (*(startpnt + count + 1) == 0xFF))
                                 || ((*(startpnt + count) == 0x00)
                                     && (*(startpnt + count + 1) == 0x00))))
                           && !(((startpnt + count)
                                 < &line_buf[(cnt + 1) * line_len - 5])
                                && (*(startpnt + count)
                                    == *(startpnt + count + 2))
                                && (*(startpnt + count + 1)
                                    == *(startpnt + count + 3))
                                && (*(startpnt + count)
                                    == *(startpnt + count + 4))
                                && (*(startpnt + count + 1)
                                    == *(startpnt + count + 5)))
                           && (count < 0xFF)
                           && ((startpnt + count)
                               < &line_buf[(cnt + 1) * line_len]))
                        count++;
                    *(bufpnt++) = count;
                    for (; count > 0; count--)
                        *(bufpnt++) = *(startpnt++);
                    break;
                };
            };

            if (write(file, write_buf, bufpnt - write_buf)
                != (bufpnt - write_buf)) {
                error = errno;
                close(file);
                free(line_buf);
                free(write_buf);
                return (error);
            };
        };
    };

    /*close file, free buffer. */

    close(file);
    free(line_buf);
    free(write_buf);
    return (0);
}

/*---filetype-dispatcher--------------------*/

const char *
get_file_ext(FILE_TYP typ)
{
    switch (typ) {
    case IMG:
    case XIMG:
        return ("IMG");
    default:
        return ("");
    };
}

int
bitmap_to_file(FILE_TYP typ, int ww, int wh, unsigned int pwx,
               unsigned int pwy, unsigned int planes, unsigned int colors,
               const char *filename,
               void (*get_color)(unsigned int colind, struct RGB *rgb),
               void (*get_pixel)(int x, int y, unsigned int *colind))
{
    switch (typ) {
    case IMG:
    case XIMG:
        return (bitmap_to_img(typ, ww, wh, pwx, pwy, planes, colors, filename,
                              get_color, get_pixel));
    default:
        return (-1);
    };
}
