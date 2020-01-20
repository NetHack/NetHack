#!/bin/sh
#set -x

if [ -z "$TRAVIS_BUILD_DIR" ]; then
	export DJGPP_TOP=$(pwd)/lib/djgpp
else
	export DJGPP_TOP="$TRAVIS_BUILD_DIR/lib/djgpp"
fi

if [ ! -d "$(pwd)/lib" ]; then
	echo "Set up for Unix build and 'make fetch-lua' first."
	exit 1
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

if [ ! -d lib ]; then
mkdir -p lib
fi

cd lib
if [ ! -f "$DJGPP_FILE" ]; then
   if [ "$(uname)" = "Darwin" ]; then
        #Mac
	curl -L $DJGPP_URL -o $DJGPP_FILE
   else
        wget --no-hsts "$DJGPP_URL"
   fi
fi

if [ ! -d djgpp/i586-pc-msdosdjgpp ]; then
    tar xjf "$DJGPP_FILE"
    rm -f $DJGPP_FILE
fi

# DOS-extender for use with djgpp
if [ ! -d djgpp/cwsdpmi ]; then
    if [ "$(uname)" = "Darwin" ]; then
      	#Mac
	curl http://sandmann.dotster.com/cwsdpmi/csdpmi7b.zip -o csdpmi7b.zip
    else
	wget --no-hsts http://sandmann.dotster.com/cwsdpmi/csdpmi7b.zip
    fi
    cd djgpp
    mkdir -p cwsdpmi
    cd cwsdpmi
    unzip ../../csdpmi7b.zip
    cd ../../
    rm csdpmi7b.zip
fi

#  PDCurses
if [ ! -d "pdcurses" ]; then
	echo "Getting ../pdcurses from https://github.com/wmcbrine/PDCurses.git" ; \
	git clone --depth 1 https://github.com/wmcbrine/PDCurses.git pdcurses
fi

cd ../

# Don't fail the build if lua fetch failed because we cannot do anything about it
# but don't bother proceeding forward either
if [ ! -d "lib/lua-$LUA_VERSION/src" ]; then
        exit 0
fi

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

if [ -f ../lib/djgpp/cwsdpmi/bin/CWSDPMI.EXE ]; then
    cp  ../lib/djgpp/cwsdpmi/bin/CWSDPMI.EXE ../msdos-binary/CWSDPMI.EXE;
fi

# ls -l ../msdos-binary
cd ../msdos-binary
zip -9 ../lib/NH370DOS.ZIP *
cd ../
ls -l lib/NH370DOS.ZIP

