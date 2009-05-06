#!/bin/sh
# NetHack 3.5  setup.sh	$Date$  $Revision$
#      SCCS Id: @(#)setup.sh 3.5     2007/12/12
# Copyright (c) Kenneth Lorber, Kensington, Maryland, 2007.
# NetHack may be freely redistributed.  See license for details.
#
# Build and install makefiles.
#
# Argument is the hints file to use (or no argument for traditional setup).

# Were we started from the top level?  Cope.
if [ -f sys/unix/Makefile.top ]; then cd sys/unix; fi

case "x$1" in
x)      hints=/dev/null
        ;;
*)      hints=$1
        ;;
esac

/bin/sh ./mkmkfile.sh Makefile.top ../../Makefile $hints
/bin/sh ./mkmkfile.sh Makefile.dat ../../dat/Makefile $hints
/bin/sh ./mkmkfile.sh Makefile.doc ../../doc/Makefile $hints
/bin/sh ./mkmkfile.sh Makefile.src ../../src/Makefile $hints
/bin/sh ./mkmkfile.sh Makefile.utl ../../util/Makefile $hints
