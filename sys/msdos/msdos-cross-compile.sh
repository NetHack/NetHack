#!/bin/sh

if [ -z "$TRAVIS_BUILD_DIR" ]; then
	export DJGPP_TOP=$(pwd)/djgpp
else
	export DJGPP_TOP="$TRAVIS_BUILD_DIR/djgpp"
fi

DJGPP_URL="https://github.com/andrewwutw/build-djgpp/releases/download/v2.9/"
if [ "$(uname)" = "Darwin" ]; then
    #Mac
    DJGPP_FILE="djgpp-osx-gcc550.tar.bz2"
elif [ "$(expr substr $(uname -s) 1 5)" = "Linux" ]; then
    #Linux
    DJGPP_FILE="djgpp-linux64-gcc550.tar.bz2"
elif [ "$(expr substr $(uname -s) 1 10)" = "MINGW32_NT" ]; then
    #mingw
    DJGPP_FILE="djgpp-mingw-gcc550-standalone.zip"
else
    echo "No DJGPP release for you, sorry."
    exit 1
fi

DJGPP_URL="$DJGPP_URL$DJGPP_FILE"

# export

cd util
if [ ! -f "$DJGPP_FILE" ]; then
    wget --no-hsts "$DJGPP_URL"
fi
cd ../


if [ ! -d ../djgpp/i586-pc-msdosdjgpp ]; then
    tar xjf "util/$DJGPP_FILE"
fi

#echo after tar
# cd ../

#pwd

#  PDCurses
if [ ! -d "../pdcurses" ]; then
	echo "Getting ../pdcurses from https://github.com/wmcbrine/PDCurses.git"
	git clone --depth 1 https://github.com/wmcbrine/PDCurses.git ../pdcurses
fi

# DOS-extender for use with djgpp
cd djgpp
if [ ! -d cwsdpmi ]; then
	wget --no-hsts http://sandmann.dotster.com/cwsdpmi/csdpmi7b.zip
	mkdir -p cwsdpmi
	cd cwsdpmi
	unzip ../csdpmi7b.zip
	cd ../
	rm csdpmi7b.zip
fi
cd ../


#echo after dos extender


cd src

mkdir -p ../msdos-binary
cp ../dat/data.base ../dat/data.bas
cp ../include/patchlevel.h ../include/patchlev.h
cp ../doc/Guidebook.txt ../doc/guidebk.txt
cp ../sys/share/posixregex.c ../sys/share/posixreg.c
#cp ../sys/msdos/Makefile1.cross ../src/Makefile1
#cp ../sys/msdos/Makefile2.cross ../src/Makefile2
make -f ../sys/msdos/Makefile1.cross
#cat ../include/date.h
export GCC_EXEC_PREFIX=$DJGPP_TOP/lib/gcc/
# export

#pwd

make -f ../sys/msdos/Makefile2.cross
unset GCC_EXEC_PREFIX
#pwd

#ls ../djgpp/cwsdpmi/bin 
#ls .

if [ -f ../djgpp/cwsdpmi/bin/CWSDPMI.EXE ]; then
    cp  ../djgpp/cwsdpmi/bin/CWSDPMI.EXE ../msdos-binary/CWSDPMI.EXE;
fi

# ls -l ../msdos-binary
cd ../msdos-binary
zip -9 ../NH370DOS.ZIP *
cd ../
# ls -l NH370DOS.ZIP

