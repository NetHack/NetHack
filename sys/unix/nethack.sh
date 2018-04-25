#!/bin/sh
# NetHack 3.6  nethack.sh	$NHDT-Date: 1524689450 2018/04/25 20:50:50 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.19 $
# Copyright (c) 2015 by Kenneth Lorber, Kensington, Maryland
# NetHack may be freely redistributed.  See license for details.

HACKDIR=/usr/games/lib/nethackdir
export HACKDIR
HACK=$HACKDIR/nethack

# Since Nethack.ad is installed in HACKDIR, add it to XUSERFILESEARCHPATH
case "x$XUSERFILESEARCHPATH" in
x)	XUSERFILESEARCHPATH="$HACKDIR/%N.ad"
	;;
*)	XUSERFILESEARCHPATH="$XUSERFILESEARCHPATH:$HACKDIR/%N.ad"
	;;
esac
export XUSERFILESEARCHPATH

# Get font dir added, but only once (and only if there's an xset to be found).
test -n "$DISPLAY" -a -e $HACKDIR/fonts.dir && xset p >/dev/null 2>&1 && (
	xset fp- $HACKDIR >/dev/null 2>&1;
	xset fp+ $HACKDIR
)

# see if we can find the full path name of PAGER, so help files work properly
# assume that if someone sets up a special variable (HACKPAGER) for NetHack,
# it will already be in a form acceptable to NetHack
# ideas from brian@radio.astro.utoronto.ca
if test \( "xxx$PAGER" != xxx \) -a \( "xxx$HACKPAGER" = xxx \)
then

	HACKPAGER=$PAGER

#	use only the first word of the pager variable
#	this prevents problems when looking for file names with trailing
#	options, but also makes the options unavailable for later use from
#	NetHack
	for i in $HACKPAGER
	do
		HACKPAGER=$i
		break
	done

	if test ! -f $HACKPAGER
	then
		IFS=:
		for i in $PATH
		do
			if test -f $i/$HACKPAGER
			then
				HACKPAGER=$i/$HACKPAGER
				export HACKPAGER
				break
			fi
		done
		IFS=' 	'
	fi
	if test ! -f $HACKPAGER
	then
		echo Cannot find $PAGER -- unsetting PAGER.
		unset HACKPAGER
		unset PAGER
	fi
fi


cd $HACKDIR
exec $HACK "$@"
