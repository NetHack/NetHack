/****************************\
* Bitmap mit Farbtabelle als *
* Graphik-Datei speichern		 *
* Autor: Gabriel Schmidt		 *
* (c} 1992 by MAXON-Computer *
* -> Header-Datei						 *
\****************************/

#ifndef H_TO_FILE
#define H_TO_FILE

/*	#include <portab.h>	*/
#define UWORD unsigned short
#define ULONG unsigned long
#define UBYTE unsigned char

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

#endif
