/* alternate compilation for tilemap.c to create tiletxt.o
   that does not rely on "cc -c -o tiletxt.o tilemap.c"
   since many pre-POSIX compilers did not support that */
#define TILETEXT
#include "tilemap.c"
/*tiletxt.c*/
