#!/bin/sh
# NetHack 3.5  mkmkfile.sh	$Date$  $Revision$
#      SCCS Id: @(#)macosx 3.5     2007/12/12
# Copyright (c) Kenneth Lorber, Kensington, Maryland, 2007.
# NetHack may be freely redistributed.  See license for details.

# build one makefile
# args are:
#  $1 basefile
#  $2 install path
#  $3 hints file

echo "#" > $2
echo "# This file is generated automatically.  Do not edit." >> $2
echo "# Your changes will be lost.  See sys/unix/NewInstall.unx." >> $2
echo "###" >> $2
echo "### Start $3" >> $2
echo "###" >> $2
cat $3 >> $2
echo "### end of file" >> $2
echo "### Start $1" >> $2
cat $1 >> $2
echo "### end of file" >> $2
