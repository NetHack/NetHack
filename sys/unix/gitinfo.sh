#!/bin/sh
# NetHack 3.6  gitinfo.sh	$NHDT-Date: 1520201830 2018/03/04 22:17:10 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.0 $

# if dat/gitinfo.txt doesn't exist, try to make one; called from Makefile.src

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

# try to run a perl script which is part of nethack's git repository
if [[ ! -f $prefix/dat/gitinfo.txt ]]; then
  ( cd $prefix; \
    perl -IDEVEL/hooksdir -MNHgithook -e '&NHgithook::nhversioning' \
      2> /dev/null ) \
  || true
fi
exit 0
