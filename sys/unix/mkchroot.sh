#!/bin/sh

# Modified from https://github.com/paxed/dgamelaunch/blob/master/dgl-create-chroot

# Ideas and some parts from the original dgl-create-chroot (by joshk@triplehelix.org, modifications by jilles@stack.nl)
# This one by paxed@alt.org

# Same as $PREFIX in hints file
CHROOT="/opt/dgl"
# COMPRESS from include/config.h; the compression binary to copy. leave blank to skip.
COMPRESSBIN="/bin/gzip"
# nethack binary
NETHACKBIN="$CHROOT/nh370/nethack"

# END OF CONFIG
##############################################################################

errorexit()
{
    echo "Error: $@" >&2
    exit 1
}

findlibs()
{
  for i in "$@"; do
      if [ -z "`ldd "$i" | grep 'not a dynamic executable'`" ]; then
         echo $(ldd "$i" | awk '{ print $3 }' | egrep -v ^'\(')
         echo $(ldd "$i" | grep 'ld-linux' | awk '{ print $1 }')
      fi
  done
}

##############################################################################

if [ -z "$TERMDATA" ]; then
    SEARCHTERMDATA="/etc/terminfo /usr/share/lib/terminfo /usr/share/terminfo /lib/terminfo"
    for dir in $SEARCHTERMDATA; do
	if [ -e "$dir/x/xterm" ]; then
	    TERMDATA="$TERMDATA $dir"
	fi
    done
    if [ -z "$TERMDATA" ]; then
	errorexit "Couldn't find terminfo definitions. Please specify in 'TERMDATA' variable."
    fi
fi

# remove trailing slash, if any
CHROOT="`echo ${CHROOT%/}`"

set -e
umask 022

cd "$CHROOT"

if [ -n "$COMPRESSBIN" -a -e "`which $COMPRESSBIN`" ]; then
  COMPRESSDIR="`dirname $COMPRESSBIN`"
  COMPRESSDIR="`echo ${COMPRESSDIR%/}`"
  COMPRESSDIR="`echo ${COMPRESSDIR#/}`"
  echo "Copying $COMPRESSBIN to $COMPRESSDIR"
  mkdir -p "$COMPRESSDIR"
  cp "`which $COMPRESSBIN`" "$COMPRESSDIR/"
  LIBS="$LIBS `findlibs $COMPRESSBIN`"
fi

if [ -n "$NETHACKBIN" -a ! -e "$NETHACKBIN" ]; then
  errorexit "Cannot find NetHack binary $NETHACKBIN"
  echo
fi

LIBS="$LIBS `findlibs $NETHACKBIN`"

# Curses junk
if [ -n "$TERMDATA" ]; then
    echo "Copying termdata files from $TERMDATA"
    for termdat in $TERMDATA; do
	mkdir -p "$CHROOT`dirname $termdat`"
	if [ -d $termdat/. ]; then
		cp -LR $termdat/. $CHROOT$termdat
	else
		cp $termdat $CHROOT`dirname $termdat`
	fi
    done
fi

LIBS=`for lib in $LIBS; do echo $lib; done | sort | uniq`
echo "Copying libraries:" $LIBS
for lib in $LIBS; do
        mkdir -p "$CHROOT`dirname $lib`"
        cp $lib "$CHROOT$lib"
done

echo "Finished."
