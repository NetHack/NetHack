#!/bin/sh
set -x

if [ -z "$CI_BUILD_DIR" ]; then
	export DJGPP_TOP=$(pwd)/lib/djgpp
else
	export DJGPP_TOP="$CI_BUILD_DIR/lib/djgpp"
fi

if [ -z "$GCCVER" ]; then
	export GCCVER=gcc1020
fi

if [ -z "$LUA_VERSION" ]; then
	export LUA_VERSION=5.4.4
fi

if [ ! -d "$(pwd)/lib" ]; then
	echo "Set up for Unix build and 'make fetch-lua' first."
	exit 1
fi

#DJGPP_URL="https://github.com/andrewwutw/build-djgpp/releases/download/v2.9/"
#DJGPP_URL="https://github.com/andrewwutw/build-djgpp/releases/download/v3.0/"
DJGPP_URL="https://github.com/andrewwutw/build-djgpp/releases/download/v3.1/"
if [ "$(uname)" = "Darwin" ]; then
    #Mac
    DJGPP_FILE="djgpp-osx-$GCCVER.tar.bz2"
    if [ -z "HINTS" ]; then
        export HINTS=macOS.370
    fi
elif [ "$(expr substr $(uname -s) 1 5)" = "Linux" ]; then
    #Linux
    DJGPP_FILE="djgpp-linux64-$GCCVER.tar.bz2"
    if [ -z "$HINTS" ]; then
        export HINTS=linux.370
    fi
elif [ "$(expr substr $(uname -s) 1 10)" = "MINGW32_NT" ]; then
    #mingw
    DJGPP_FILE="djgpp-mingw-$GCCVER-standalone.zip"
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
        wget --quiet --no-hsts "$DJGPP_URL"
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
	wget --quiet --no-hsts http://sandmann.dotster.com/cwsdpmi/csdpmi7b.zip
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

