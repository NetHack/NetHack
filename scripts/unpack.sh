#!/bin/sh
# unpack.sh -- UNIX shell script to unpack nethack archives
#
# This eliminates a lot of the tedium from snarfing a new nethack version
# from a nethack-bugs archive site.  It automatically creates appropriate
# directories, undoes uuencoding and compression, and untars tar files.
# When it's done running, you should have a complete clean source-tree.

# System V style tars need the addition of the o option here
#
# If you didn't change this before running the script, you at least own
# the created directories, so you can slowly and painfully rename and copy
# each file so that you own it (or write your own simple script to
# automate the process).  Using chown with superuser privileges is of
# course a much quicker recovery...
#
TAROPTS=xvf
# TAROPTS=xovf


# Nothing below here should need to be changed.

DECOMPRESS="compress -d"
TZINT=taz
TZEXT=tar.Z
TZUEXT=tzu

while [ $# -gt 0 ] ; do
    case X$1 in
	X-x)	echo "usage $0 [-gz|-x]"
		echo "-gz   use gunzip as decompress utility (default, decompress)."
		echo "-x    display this text."
		exit ;;
	X-gz)	DECOMPRESS=gunzip
		TZINT=tgz
		TZEXT=tar.gz
		TZUEXT=tgu ;;
	X*)	echo "usage $0 [-gz|-x]" ; exit ;;
    esac
    shift
done

# Changes to the source set should require at most additions to ARCHIVES
# and (possibly) additions to the case statement mapping archives to
# subdirectory names.
#
ARCHIVES1="top dat1 dat2 dat3 dat4 dat5 doc1 doc2 doc3 doc4 doc5 incl1 incl2 incl3 incl4 incl5 util1 util2"
ARCHIVES2="src01 src02 src03 src04 src05 src06 src07 src08 src09 src10 src11 src12 src13 src14 src15 src16 src17 src18 src19 src20 src21 src22 src23 src24 src25 src26 src27 src28 src29 src30 src31 src32 src33 src34 src35"
ARCHIVES3="amiga1 amiga2 amiga3 amiga4 amiga5 amiga6 amiga7 amiga8 amiga9 ami_spl atari be mac1 mac2 mac3 mac4 macold1 macold2"
ARCHIVES4="msdos1 msdos2 msdos3 msdos4 msold1 msold2 msold3 nt_sys os2 shr_sys1 shr_sys2 shr_sys3 sound1 sound2 sound3 sound4 sound5"
ARCHIVES5="unix1 unix2 vms1 vms2 vms3 shr_win1 shr_win2 shr_win3 shr_win4 tty1 tty2 nt_win x11-1 x11-2 x11-3 x11-4 x11-5 dev1 dev2 dev3 dev4"

# unconditionally create directories we may try to create subdirectories of
if [ ! -d sys ]
then
	mkdir sys
fi
if [ ! -d sys/amiga ]
then
	mkdir sys/amiga
fi
if [ ! -d sys/mac ]
then
	mkdir sys/mac 
fi
if [ ! -d sys/msdos ]
then
	mkdir sys/msdos 
fi
if [ ! -d sys/share ]
then
	mkdir sys/share
fi
if [ ! -d win ]
then
	mkdir win
fi


# look for files packaged individually
#
if [ -f shr_tc.uu ]
then
	# sys/share guaranteed via subdirectory above
	mv shr_tc.uu sys/share/termcap.uu
fi
if [ -f mac-snd.hqx ]
then
	mv mac-snd.hqx sys/mac/NHsound.hqx
fi
if [ -f mac-proj.hqx ]
then
	if [ ! -d sys/mac/old ]
	then
		mkdir sys/mac/old
	fi
	mv mac-proj.hqx sys/mac/old/NHproj.hqx
fi
for i in 1 2 3
do
	if [ -f cpp$i.shr ]
	then
		if [ ! -d sys/unix ]
		then
			mkdir sys/unix
		fi
		mv cpp$i.shr sys/unix/cpp$i.shr
	fi
done

# unpack the archives
#
topdir=`pwd`

for f in $ARCHIVES1 $ARCHIVES2 $ARCHIVES3 $ARCHIVES4 $ARCHIVES5
do
	if [ -f ${f}.${TZUEXT} ]
	then
		uudecode ${f}.${TZUEXT}; rm ${f}.${TZUEXT}
	fi
	if [ -f ${f}.${TZINT} ]
	then
		mv ${f}.${TZINT} ${f}.${TZEXT}
		$DECOMPRESS ${f}.${TZEXT}
	fi

	if [ -f ${f}.tar ]
	then
		# Here's the part that may need hacking as we add more machines
		case $f in
			amiga*) dir=sys/amiga;;
			ami_spl*) dir=sys/amiga/splitter;;
			atari*) dir=sys/atari;;
			be*) dir=sys/be;;
			dat*) dir=dat;;
			dev*) dir=.;;
			doc*) dir=doc;;
			incl*) dir=include;;
			macold*) dir=sys/mac/old;;
			mac*) dir=sys/mac;;
			msdos*) dir=sys/msdos;;
			msold*) dir=sys/msdos/old;;
			nt_sys*) dir=sys/winnt;;
			nt_win*) dir=win/win32;;
			os2*) dir=sys/os2;;
			shr_sys*) dir=sys/share;;
			shr_win*) dir=win/share;;
			sound*) dir=sys/share/sounds;;
			src*) dir=src;;
			top*) dir=.;;
			tty*) dir=win/tty;;
			unix*) dir=sys/unix;;
			util*) dir=util;;
			vms*) dir=sys/vms;;
			x11-*) dir=win/X11;;
		esac

		echo Unpacking $f.tar into $dir ...
		if [ ! -d $dir ]
		then
			mkdir $dir
		fi
		if [ $dir != "." ]
		then
			mv $f.tar $dir;
			cd $dir
		fi
		tar $TAROPTS $f.tar
		rm $f.tar
		if [ $dir != "." ]
		then
			cd $topdir
		fi
	fi
done

echo "Remember to run setup.sh from the sys/unix directory if you're compiling"
echo "for UNIX."
