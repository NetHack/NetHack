#!/bin/bash
# NetHack 3.6  setup.sh	$NHDT-Date: 1432512789 2015/05/25 00:13:09 $  $NHDT-Branch: master $:$NHDT-Revision: 1.14 $
# Copyright (c) Kenneth Lorber, Kensington, Maryland, 2007.
# NetHack may be freely redistributed.  See license for details.
#
# Build and install makefiles.
#
# This requires no argument
# e.g.:
#  ./setup.sh

# Cygwin is very much liks Unix.  We make use of as many of the sys/unix files
# as we can so that it requires less maintenance on update to the unix versions.

# Were we started from the top level?  Cope.
prefix=.
if [ -f sys/cygwin/Makefile.top ]; then cd sys/cygwin; prefix=../..; fi

hfile=cygwin_hints
hints=$prefix/$hfile

if [ ! -f "$hints" ]; then
    echo "Cannot find hints file $hfile"
    exit 1
fi

# Egalitarian wizardry, handy for debugging
sed -e 's/WIZARDS=.*$/WIZARDS=*/' ../unix/sysconf > sysconf

# All of these makefiles can be used as is from the Unix versions
/bin/sh ../unix/mkmkfile.sh ../unix/Makefile.top TOP ../../Makefile $hints $hfile
/bin/sh ../unix/mkmkfile.sh ../unix/Makefile.dat DAT ../../dat/Makefile $hints $hfile
/bin/sh ../unix/mkmkfile.sh ../unix/Makefile.doc DOC ../../doc/Makefile $hints $hfile
/bin/sh ../unix/mkmkfile.sh ../unix/Makefile.src SRC ../../src/Makefile $hints $hfile

# Plain old yacc is not currently available in cygwin's default package list,
# so we take the Unix Makefile.utl and replace the YACC=yacc line with YACC=byacc
sed -e 's/YACC.*=.*yacc/YACC=byacc/' ../unix/Makefile.utl > Makefile.utl
/bin/sh ../unix/mkmkfile.sh Makefile.utl UTL ../../util/Makefile $hints $hfile

