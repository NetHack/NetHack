#include "config.h"
#include "tileset.h"

#ifdef USE_PNG

#include <png.h>

boolean
read_png_tiles(const char *filename, struct TileSetImage *image)
{
    png_structp read_png_ptr = NULL; /* custodial */
    png_infop read_info_ptr = NULL; /* custodial */
    png_infop read_end_info = NULL; /* custodial */

    FILE *fp;
    unsigned char header[8];
    size_t size, num_pixels;
    unsigned x, y;
    png_bytepp pixels;
    png_byte color_type;
    int palette_size = 0;
    png_textp text_chunks;
    int num_text_chunks, i;

    image->width = 0;
    image->height = 0;
    image->indexes = NULL;      /* custodial, returned */
    image->pixels = NULL;       /* custodial, returned */
    image->image_desc = NULL;   /* custodial, returned */
    image->tile_width = 0;
    image->tile_height = 0;

    /* Open the file */
    fp = fopen(filename, "rb");
    if (fp == NULL) goto error;

    /* Check for PNG format */
    memset(header, 0, sizeof(header));
    size = fread(header, 1, sizeof(header), fp);
    if (size < sizeof(header)) goto error;
    if (png_sig_cmp(header, 0, sizeof(header)) != 0) goto error;

    /* Create read info structures */
    read_png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
            NULL, NULL);
    if (read_png_ptr == NULL) goto error;
    if (setjmp(png_jmpbuf(read_png_ptr)) != 0) goto error;
    read_info_ptr = png_create_info_struct(read_png_ptr);
    read_end_info = png_create_info_struct(read_png_ptr);

    /* Link the file we opened to the read info */
    png_init_io(read_png_ptr, fp);
    png_set_sig_bytes(read_png_ptr, sizeof(header));

    /* Read the image */
    png_read_png(read_png_ptr, read_info_ptr,
            PNG_TRANSFORM_PACKING | PNG_TRANSFORM_STRIP_16, NULL);

    pixels = png_get_rows(read_png_ptr, read_info_ptr);
    image->width = png_get_image_width(read_png_ptr, read_info_ptr);
    image->height = png_get_image_height(read_png_ptr, read_info_ptr);
    color_type = png_get_color_type(read_png_ptr, read_info_ptr);

    /* Read the palette, if there is one */
    if (color_type & PNG_COLOR_MASK_PALETTE) {
        png_colorp palette = NULL;
        png_bytep trans_alpha;
        int num_trans;

        png_get_PLTE(read_png_ptr, read_info_ptr, &palette, &palette_size);
        for (i = 0; i < 256; ++i) {
            image->palette[i].r = palette[i].red;
            image->palette[i].g = palette[i].green;
            image->palette[i].b = palette[i].blue;
            image->palette[i].a = 255;
        }
        if (png_get_tRNS(read_png_ptr, read_info_ptr, &trans_alpha, &num_trans,
                         NULL) != 0) {
            for (i = 0; i < num_trans; ++i) {
                image->palette[i].a = trans_alpha[i];
            }
        }
    }

    /* Allocate pixel area; watch out for overflow */
    num_pixels = (size_t) image->width * (size_t) image->height;
    if (num_pixels / image->width != image->height) goto error; /* overflow */
    size = num_pixels * sizeof(image->pixels[0]);
    if (size / sizeof(image->pixels[0]) != num_pixels) goto error; /* overflow */
    image->pixels = (struct Pixel *) alloc(size);
    if (color_type & PNG_COLOR_MASK_PALETTE) {
        image->indexes = (unsigned char *) alloc(num_pixels);
    }

    /* Convert the pixels */
    for (y = 0; y < image->height; ++y) {
        png_bytep png_row = pixels[y];
        size_t index = y * image->width;
        for (x = 0; x < image->width; ++x) {
            if (color_type & PNG_COLOR_MASK_PALETTE) {
                png_byte b = *png_row++;
                image->indexes[index] = b;
                image->pixels[index] = image->palette[b];
            } else {
                if (color_type & PNG_COLOR_MASK_COLOR) {
                    image->pixels[index].r = *png_row++;
                    image->pixels[index].g = *png_row++;
                    image->pixels[index].b = *png_row++;
                } else {
                    image->pixels[index].r = *png_row++;
                    image->pixels[index].g = image->pixels[index].r;
                    image->pixels[index].b = image->pixels[index].r;
                }
                if (color_type & PNG_COLOR_MASK_ALPHA) {
                    image->pixels[index].a = *png_row++;
                } else {
                    image->pixels[index].a = 255;
                }
            }
            ++index;
        }
    }

    /* Look for the tile information */
    png_get_text(read_png_ptr, read_info_ptr, &text_chunks, &num_text_chunks);
    for (i = 0; i < num_text_chunks; ++i) {
        if (strcmp(text_chunks[i].key, "NH_image_desc") == 0) {
            image->image_desc = (char *) alloc(text_chunks[i].text_length + 1);
            memcpy(image->image_desc, text_chunks[i].text, text_chunks[i].text_length);
            text_chunks[i].text[text_chunks[i].text_length] = '\0';
        }
    }

    png_destroy_read_struct(&read_png_ptr, &read_info_ptr, &read_end_info);
    return 1;

error:
    if (read_png_ptr) {
        png_destroy_read_struct(&read_png_ptr, &read_info_ptr, &read_end_info);
    }
    free(image->indexes);
    image->indexes = NULL;
    free(image->pixels);
    image->pixels = NULL;
    free(image->image_desc);
    image->image_desc = NULL;
    return 0;
}

#endif
