/*
 * Convert the given input files into an output file that is expected
 * by nethack.
 * 
 * Assumptions:
 * 	+ Two dimensional byte arrays are in row order and are not padded
 *	  between rows (x11_colormap[][]).
 */
#include "hack.h"		/* for MAX_GLYPH */
#include "tile.h"
#include "tile2x11.h"		/* x11 output file header structure */

#define OUTNAME "x11tiles"	/* output file name */
/* #define PRINT_COLORMAP */	/* define to print the colormap */


x11_header	header;
unsigned char	tile_bytes[TILE_X*TILE_Y*MAX_GLYPH];
unsigned char	*curr_tb = tile_bytes;
unsigned char	x11_colormap[MAXCOLORMAPSIZE][3];


/* Look up the given pixel and return its colormap index. */
static unsigned char
pix_to_colormap(pix)
    pixel pix;
{
    int i;

    for (i = 0; i < header.ncolors; i++) {
	if (pix.r == ColorMap[CM_RED][i] &&
		pix.g == ColorMap[CM_GREEN][i] &&
		pix.b == ColorMap[CM_BLUE][i])
	    break;
    }

    if (i == header.ncolors) {
	Fprintf(stderr, "can't find color: [%u,%u,%u]\n", pix.r, pix.g, pix.b);
	exit(1);
    }
    return (unsigned char) (i & 0xFF);
}


/* Convert the tiles in the file to our format of bytes. */
static unsigned long
convert_tiles(tb_ptr)
    unsigned char **tb_ptr;	/* pointer to a tile byte pointer */
{
    unsigned char *tb = *tb_ptr;
    unsigned long count = 0;
    pixel tile[TILE_Y][TILE_X];
    int x, y;

    while (read_text_tile(tile)) {
	count++;
	for (y = 0; y < TILE_Y; y++)
	    for (x = 0; x < TILE_X; x++)
		*tb++ = pix_to_colormap(tile[y][x]);
    }

    *tb_ptr = tb;	/* update return val */
    return count;
}


/* Merge the current text colormap (ColorMap) with ours (x11_colormap). */
static void
merge_text_colormap()
{
    int i, j;

    for (i = 0; i < colorsinmap; i++) {
	for (j = 0; j < header.ncolors; j++)
	    if (x11_colormap[j][CM_RED] == ColorMap[CM_RED][i] &&
		    x11_colormap[j][CM_GREEN] == ColorMap[CM_GREEN][i] &&
		    x11_colormap[j][CM_BLUE] == ColorMap[CM_BLUE][i])
		break;

	if (j >= MAXCOLORMAPSIZE) {
	    Fprintf(stderr, "colormap overflow\n");
	    exit(1);
	}

	if (j == header.ncolors) {	/* couldn't find it */
#ifdef PRINT_COLORMAP
	    printf("color %2d: %3d %3d %3d\n", header.ncolors,
	    	ColorMap[CM_RED][i], ColorMap[CM_GREEN][i],
	    	ColorMap[CM_BLUE][i]);
#endif

	    x11_colormap[j][CM_RED] = ColorMap[CM_RED][i];
	    x11_colormap[j][CM_GREEN] = ColorMap[CM_GREEN][i];
	    x11_colormap[j][CM_BLUE] = ColorMap[CM_BLUE][i];
	    header.ncolors++;
	}
    }
}


/* Open the given file, read & merge the colormap, convert the tiles. */
static void
process_file(fname)
    char *fname;
{
    unsigned long count;

    if (!fopen_text_file(fname, RDTMODE)) {
	Fprintf(stderr, "can't open file \"%s\"\n", fname);
	exit(1);
	}
    merge_text_colormap();
    count = convert_tiles(&curr_tb);
    Fprintf(stderr, "%s: %lu tiles\n", fname, count);
    header.ntiles += count;
    fclose_text_file();
}


#ifdef USE_XPM
static int
xpm_write(fp)
FILE *fp;
{
    int i,j,n;

    if (header.ncolors > 64) {
	Fprintf(stderr, "Sorry, only configured for up to 64 colors\n");
	exit(1);
	/* All you need to do is add more char per color - below */
    }

    fprintf(fp, "/* XPM */\n");
    fprintf(fp, "static char* nhtiles[] = {\n");
    fprintf(fp, "\"%lu %lu %lu %d\",\n",
		header.tile_width,
		header.tile_height*header.ntiles,
		header.ncolors,
		1 /* char per color */);
    for (i = 0; i < header.ncolors; i++)
	fprintf(fp, "\"%c  c #%02x%02x%02x\",\n",
		i+'0', /* just one char per color */
		x11_colormap[i][0],
		x11_colormap[i][1],
		x11_colormap[i][2]);

    n=0;
    for (i = 0; i < header.tile_height*header.ntiles; i++) {
	fprintf(fp, "\"");
	for (j = 0; j < header.tile_width; j++) {
	    /* just one char per color */
	    fputc(tile_bytes[n++]+'0', fp);
	}
	if (j==header.tile_width-1)
	    fprintf(fp, "\"\n");
	else
	    fprintf(fp, "\",\n");
    }

    return fprintf(fp, "};\n")>=0;
}
#endif	/* USE_XPM */

int
main(argc, argv)
    int argc;
    char **argv;
{
    FILE *fp;
    int i;

    header.version	= 1;
    header.ncolors	= 0;
    header.tile_width	= TILE_X;
    header.tile_height	= TILE_Y;
    header.ntiles	= 0;		/* updated as we read in files */

    if (argc == 1) {
	Fprintf(stderr, "usage: %s txt_file1 [txt_file2 ...]\n", argv[0]);
	exit(1);
    }

    fp = fopen(OUTNAME, "w");
    if (!fp) {
	Fprintf(stderr, "can't open output file\n");
	exit(1);
	}

    for (i = 1; i < argc; i++)
	process_file(argv[i]);
    Fprintf(stderr, "Total tiles: %ld\n", header.ntiles);

#ifdef USE_XPM
    if (xpm_write(fp) == 0) {
	Fprintf(stderr, "can't write XPM file\n");
	exit(1);
    }
#else
    if (fwrite((char *)&header, sizeof(x11_header), 1, fp) == 0) {
	Fprintf(stderr, "can't open output header\n");
	exit(1);
	}

    if (fwrite((char *)x11_colormap, 1, header.ncolors*3, fp) == 0) {
	Fprintf(stderr, "can't write output colormap\n");
	exit(1);
    }

    if (fwrite((char *)tile_bytes, 1,
	(int) header.ntiles*header.tile_width*header.tile_height, fp) == 0) {

	Fprintf(stderr, "can't write tile bytes\n");
	exit(1);
    }
#endif

    fclose(fp);
    return 0;
}
