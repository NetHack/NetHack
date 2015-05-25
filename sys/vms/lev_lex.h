/* NetHack 3.6	lev_lex.h	$NHDT-Date: 1432512790 2015/05/25 00:13:10 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/* "vms/lev_lex.h" copied into "util/stdio.h" for use in *_lex.c only!
 * This is an awful kludge to allow util/*_lex.c made by SunOS's `lex'
 * to be compiled as is.  (It isn't needed with `flex' or VMS POSIX
 * `lex' and is benign when either of those configurations are used.)
 * It works because the actual setup of yyin & yyout is performed in
 * src/lev_main.c, where stdin & stdout are still correctly defined.
 *
 * The troublesome code is
 *	#include "stdio.h"
 *		...
 *	FILE *yyin = stdin, *yyout = stdout;
 * The file scope initializers with non-constant values require this
 * hack, and the quotes instead of brackets makes it easy to do.
 */

#include <stdio.h>
#ifdef stdin
#undef stdin
#endif
#define stdin 0
#ifdef stdout
#undef stdout
#endif
#define stdout 0
