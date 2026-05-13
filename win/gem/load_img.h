
/* ------------------------------------------- */
#define XIMG 0x58494D47

/* Header of GEM Image Files   */
typedef struct IMG_HEADER {
    short version;  /* Img file format version (1) */
    short length;   /* Header length in words  (8) */
    short planes;   /* Number of bit-planes    (1) */
    short pat_len;  /* length of Patterns      (2) */
    short pix_w;    /* Pixel width in 1/1000 mmm  (372)    */
    short pix_h;    /* Pixel height in 1/1000 mmm (372)    */
    short img_w;    /* Pixels per line (=(x+7)/8 Bytes)    */
    short img_h;    /* Total number of lines               */
    long magic;     /* Contains "XIMG" if standard color   */
    short paltype;  /* palette type (0=RGB (short each)) */
    short *palette; /* palette etc.                        */
    char *addr;     /* Address for the depacked bit-planes */
} IMG_header;

/* ------------------------------------------- */
/* error codes */
#define ERR_HEADER 1
#define ERR_ALLOC 2
#define ERR_FILE 3
#define ERR_DEPACK 4
#define ERR_COLOR 5

/* saves the current colorpalette with col colors in palette */
void get_colors(int handle, short *palette, int col);

/* sets col colors from palette */
void img_set_colors(int handle, short *palette, int col);

/* converts MFDB  of size from standard to deviceformat (0 if succeded, else
 * error). */
int convert(MFDB *, long);

/* transforms image in VDI-Device format */
int transform_img(MFDB *);

/* Loads & depacks IMG (0 if succeded, else error). */
/* Bitplanes are one after another in address IMG_HEADER.addr. */
int depack_img(char *, IMG_header *);

/* Halves IMG in Device-format, dest memory has to be allocated*/
int half_img(MFDB *, MFDB *);
