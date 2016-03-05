/* NetHack 3.6	thintile.c	$NHDT-Date: 1457207049 2016/03/05 19:44:09 $  $NHDT-Branch: chasonr $:$NHDT-Revision: 1.10 $ */
/*   Copyright (c) NetHack Development Team 1995                    */
/*   NetHack may be freely redistributed.  See license for details. */

/* Create a set of overview tiles by eliminating even pixels in original */

#include "config.h"
#include "tile.h"

#ifdef __GO32__
#include <unistd.h>
#endif

static char pixels[TILE_Y][TILE_X];

static char *tilefiles[] = { "../win/share/monsters.txt",
                             "../win/share/objects.txt",
                             "../win/share/other.txt" };

static char *thinfiles[] = { "../win/share/monthin.txt",
                             "../win/share/objthin.txt",
                             "../win/share/oththin.txt" };
static FILE *infile, *outfile;
static int tilecount;
static int tilecount_per_file;
static int filenum;
static char comment[BUFSZ];

static void
copy_colormap()
{
    int r, g, b;
    char c[2];

    while (fscanf(infile, "%[A-Za-z0-9.] = (%d, %d, %d) ", c, &r, &g, &b)
           == 4) {
        Fprintf(outfile, "%c = (%d, %d, %d)\n", c[0], r, g, b);
    }
}

static boolean
read_txttile()
{
    int i, j;
    char buf[BUFSZ];
    char buf2[BUFSZ];

    char c[2];

    if (fscanf(infile, "# %s %d (%[^)])", buf2, &i, buf) <= 0)
        return FALSE;

    Sprintf(comment, "# tile %d (%s)", i, buf);

    /* look for non-whitespace at each stage */
    if (fscanf(infile, "%1s", c) < 0) {
        Fprintf(stderr, "unexpected EOF\n");
        return FALSE;
    }
    if (c[0] != '{') {
        Fprintf(stderr, "didn't find expected '{'\n");
        return FALSE;
    }
    for (j = 0; j < TILE_Y; j++) {
        for (i = 0; i < TILE_X; i++) {
            if (fscanf(infile, "%1s", c) < 0) {
                Fprintf(stderr, "unexpected EOF\n");
                return FALSE;
            }
            pixels[j][i] = c[0];
        }
    }
    if (fscanf(infile, "%1s ", c) < 0) {
        Fprintf(stderr, "unexpected EOF\n");
        return FALSE;
    }
    if (c[0] != '}') {
        Fprintf(stderr, "didn't find expected '}'\n");
        return FALSE;
    }
    return TRUE;
}

static void
write_thintile()
{
    int i, j;

    Fprintf(outfile, "%s\n", comment);
    Fprintf(outfile, "{\n");
    for (j = 0; j < TILE_Y; j++) {
        Fprintf(outfile, "  ");
        for (i = 0; i < TILE_X; i += 2) {
            (void) fputc(pixels[j][i], outfile);
        }
        Fprintf(outfile, "\n");
    }
    Fprintf(outfile, "}\n");
}
int
main(argc, argv)
int argc;
char *argv[];
{
    while (filenum < 3) {
        tilecount_per_file = 0;
        infile = fopen(tilefiles[filenum], RDTMODE);
        outfile = fopen(thinfiles[filenum], WRTMODE);
        copy_colormap();
        while (read_txttile()) {
            write_thintile();
            tilecount_per_file++;
            tilecount++;
        }
        fclose(outfile);
        fclose(infile);
        printf("%d tiles processed from %s\n", tilecount_per_file,
               tilefiles[filenum]);
        ++filenum;
    }
    printf("Grand total of %d tiles processed.\n", tilecount);
    exit(EXIT_SUCCESS);
    /*NOTREACHED*/
    return 0;
}

/*thintile.c*/
