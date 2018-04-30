#!/bin/sh
# NetHack 3.6  gitinfo.sh	$NHDT-Date: 1524689450 2018/04/25 20:50:50 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.2 $
# Copyright (c) 2018 by Robert Patrick Rankin
# NetHack may be freely redistributed.  See license for details.

# bring dat/gitinfo.txt up to date; called from Makefile.src

#
# gitinfo.txt is used during development to augment the version number
# (for nethack's 'v' command) with more specific information.  That is not
# necessary when building a released version and it is perfectly OK for
# this script to be skipped or to run but fail to generate dat/gitinfo.txt.
#

# try to figure out where we are: top, one level down (expected), or sys/unix
prefix=.
if [ -f ../sys/unix/gitinfo.sh ]; then prefix=..; fi
if [ -f ../../sys/unix/gitinfo.sh ]; then prefix=../..; fi

rungit=0
if [ $1 -eq 1 ]; then rungit=1; fi
if [ $1 = "force" ]; then rungit=1; fi
if [ ! -f $prefix/dat/gitinfo.txt ]; then rungit=1; fi

# try to run a perl script which is part of nethack's git repository
if [ $rungit -eq 1 ]; then
  ( cd $prefix; \
    perl -IDEVEL/hooksdir -MNHgithook -e '&NHgithook::nhversioning' \
      2> /dev/null ) \
  || true
fi
exit 0
