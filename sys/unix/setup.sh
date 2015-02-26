#!/bin/sh
# NetHack 3.5  setup.sh	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$
# NetHack 3.5  setup.sh	$Date: 2011/12/27 00:34:52 $  $Revision: 1.6 $
# Copyright (c) Kenneth Lorber, Kensington, Maryland, 2007.
# NetHack may be freely redistributed.  See license for details.
#
# Build and install makefiles.
#
# Argument is the hints file to use (or no argument for traditional setup).
# e.g.:
#  sh setup.sh
# or
#  sh setup.sh hints/macosx10.5 (from sys/unix)
# or
#  sh setup.sh sys/unix/hints/macosx10.5 (from top)

# Were we started from the top level?  Cope.
prefix=.
if [ -f sys/unix/Makefile.top ]; then cd sys/unix; prefix=../..; fi

case "x$1" in
x)      hints=/dev/null
	hfile=/dev/null
        ;;
*)      hints=$prefix/$1
	hfile=$1
        ;;
esac

/bin/sh ./mkmkfile.sh Makefile.top TOP ../../Makefile $hints $hfile
/bin/sh ./mkmkfile.sh Makefile.dat DAT ../../dat/Makefile $hints $hfile
/bin/sh ./mkmkfile.sh Makefile.doc DOC ../../doc/Makefile $hints $hfile
/bin/sh ./mkmkfile.sh Makefile.src SRC ../../src/Makefile $hints $hfile
/bin/sh ./mkmkfile.sh Makefile.utl UTL ../../util/Makefile $hints $hfile
