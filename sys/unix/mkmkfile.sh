#!/bin/sh
# NetHack 3.7  mkmkfile.sh	$NHDT-Date: 1597332770 2020/08/13 15:32:50 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.15 $
# Copyright (c) Kenneth Lorber, Kensington, Maryland, 2007.
# NetHack may be freely redistributed.  See license for details.

# build one makefile
# args are:
#  $1 basefile
#  $2 basefile tag
#  $3 install path
#  $4 hints file (path)
#  $5 hints file (as given by user)

echo "#" > $3
echo "# This file is generated automatically.  Do not edit." >> $3
echo "# Your changes will be lost.  See sys/unix/NewInstall.unx." >> $3
echo "# Identify this file:" >> $3
echo "MAKEFILE_$2=1" >> $3
echo "" >> $3

echo "###" >> $3
echo "### Start $5 PRE" >> $3
echo "###" >> $3
awk '/^#-PRE/,/^#-POST/{ \
	if(index($0, "#-PRE") == 1) print "# (new segment at source line",NR,")"; \
	if(index($0, "#-INCLUDE") == 1)	system("cat hints/include/"$2); \
	else if(index($0, "#-P") != 1) print}' $4 >> $3
echo "### End $5 PRE" >> $3
echo "" >> $3

echo "###" >> $3
echo "### Start $1" >> $3
echo "###" >> $3
cat $1 >> $3
echo "### End $1" >> $3
echo "" >> $3

echo "###" >> $3
echo "### Start $5 POST" >> $3
echo "###" >> $3
awk '/^#-POST/,/^#-PRE/{ \
	if(index($0, "#-POST") == 1) print "# (new segment at source line",NR,")"; \
	if(index($0, "#-P") != 1) print}' $4 >> $3
echo "### End $5 POST" >> $3
