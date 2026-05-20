/****************************\
* Bitmap mit Farbtabelle als *
* Graphik-Datei speichern		 *
* Autor: Gabriel Schmidt		 *
* (c} 1992 by MAXON-Computer *
* -> Header-Datei						 *
\****************************/

#ifndef BITMFILE_H
#define BITMFILE_H

#include <stdint.h>
typedef uint16_t UWORD;
typedef uint32_t ULONG;
typedef uint8_t UBYTE;

#define XIMG_MAGIC 0x58494D47

typedef enum { IMG, XIMG } FILE_TYP;

const char *get_file_ext(FILE_TYP typ);

struct RGB {
    UWORD r, g, b;
};

int bitmap_to_file(FILE_TYP typ, int ww, int wh, unsigned int pwx,
                   unsigned int pwy, unsigned int planes, unsigned int colors,
                   const char *filename,
                   void (*get_color)(unsigned int colind, struct RGB *rgb),
                   void (*get_pixel)(int x, int y, unsigned int *colind));

#endif /* BITMFILE_H */
