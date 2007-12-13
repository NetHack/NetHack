#!/bin/sh
#      SCCS Id: @(#)macosx 3.5     2007/12/12
# Copyright (c) Kenneth Lorber, Kensington, Maryland, 2007.
# NetHack may be freely redistributed.  See license for details.

# build one makefile
# args are:
#  $1 basefile
#  $2 install path
#  $3 hints file

cat $3 $1 > $2
