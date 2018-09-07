#!/bin/sh

#    NetHack 3.6 setup.sh   $NHDT-Date: 1432512792 2015/05/25 00:13:12 $ $NHDT-Branch: master $:$NHDT-Revision: 1.9 $
#    Copyright (c) NetHack PC Development Team 1990 - 2018
#    NetHack may be freely redistributed.  See license for details.

echo
echo "   Copyright (c) NetHack PC Development Team 1990 - 2018"
echo "   NetHack may be freely redistributed.  See license for details."
echo

echo Checking to see if directories are set up properly ...
for f in ../../include/hack.h ../../src/hack.c ../../dat/wizard.des ../../util/makedefs.c ../../win/tty/wintty.c ../share/lev_yacc.c; do
    if [ ! -e $f ]; then
        echo Your directories are not set up properly, please re-read the
        echo Install.dos and README documentation.
        exit 1
    fi
done
echo "Directories OK."

mkdir -p ../../binary
cp -u ../../dat/license ../../binary/license

# Missing guidebook is not fatal to the build process
cp -p ../../doc/guidebook.txt ../../doc/guidebk.txt
if [ ! -e ../../doc/guidebk.txt ]; then
    echo "Warning - There is no NetHack Guidebook (../../doc/guidebk.txt)"
    echo "          included in your distribution.  Build will proceed anyway."
fi

echo "Makefile.GCC -> ../../src/makefile"
cp -p makefile.CROSS ../../src/makefile

echo "Setup Done!"
echo "Please continue with next step from Install.dos."
