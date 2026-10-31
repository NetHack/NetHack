#!/bin/bash

if [ ! -z "${TF_BUILD}" ]; then
	set -x
fi

if [ -z "$CI_BUILD_DIR" ]; then
	export DJGPP_TOP=$(pwd)/lib/djgpp
else
	export DJGPP_TOP="$CI_BUILD_DIR/lib/djgpp"
fi

if [ -z "$GCCVER" ]; then
       export GCCVER=gcc1220
fi

if [ -z "$LUA_VERSION" ]; then
	export LUA_VERSION=5.4.8
fi

if [ ! -d "$(pwd)/lib" ]; then
	echo "Set up for Unix build and 'make fetch-lua' first."
	exit 1
fi

#DJGPP_URL="https://github.com/andrewwutw/build-djgpp/releases/download/v2.9/"
#DJGPP_URL="https://github.com/andrewwutw/build-djgpp/releases/download/v3.0/"
#DJGPP_URL="https://github.com/andrewwutw/build-djgpp/releases/download/v3.1/"
#DJGPP_URL="https://github.com/andrewwutw/build-djgpp/releases/download/v3.1/"
#DJGPP_URL="https://github.com/andrewwutw/build-djgpp/releases/download/v3.3/"
DJGPP_URL="https://github.com/andrewwutw/build-djgpp/releases/download/v3.4/"

if [ "$(uname)" = "Darwin" ]; then
    #Mac
    DJGPP_FILE="djgpp-osx-$GCCVER.tar.bz2"
    if [ -z "HINTS" ]; then
        export HINTS=macOS.500
    fi
elif [ "$(expr substr $(uname -s) 1 5)" = "Linux" ]; then
    #Linux
    DJGPP_FILE="djgpp-linux64-$GCCVER.tar.bz2"
    if [ -z "$HINTS" ]; then
        export HINTS=linux.500
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
   echo "fetching djgpp cross-compiler int lib/djgpp"
   if [ "$(uname)" = "Darwin" ]; then
        #Mac
	curl -L $DJGPP_URL -o $DJGPP_FILE
   else
        wget --quiet --no-hsts "$DJGPP_URL"
   fi
fi

if [ ! -d djgpp/i586-pc-msdosdjgpp ]; then
    echo "extracting djgpp from $DJGPP_FILE"
    tar xjf "$DJGPP_FILE"
    #rm -f $DJGPP_FILE
fi

# DOS-extender for use with djgpp
if [ ! -d djgpp/cwsdpmi ]; then
    echo "fetching DOS extender for use with djgpp"
    if [ "$(uname)" = "Darwin" ]; then
      	#Mac
	curl http://sandmann.dotster.com/cwsdpmi/csdpmi7b.zip -o csdpmi7b.zip
    else
        wget --quiet --no-hsts https://www.delorie.com/pub/djgpp/current/v2misc/csdpmi7b.zip
        if [ $? -ne 0 ]; then
	    wget --quiet --no-hsts --timeout=20 http://sandmann.dotster.com/cwsdpmi/csdpmi7b.zip
        fi
    fi
    cd djgpp
    mkdir -p cwsdpmi
    cd cwsdpmi
    unzip ../../csdpmi7b.zip
    cd ../../
    rm csdpmi7b.zip
fi

#  PDCurses (non-Unicode build uses this)
if [ ! -d "pdcurses" ]; then
	echo "fetching ../pdcurses from https://github.com/wmcbrine/PDCurses.git" ; \
	git clone --depth 1 https://github.com/wmcbrine/PDCurses.git pdcurses
fi

#  PDCursesMod (Unicode build uses this)
if [ ! -d "pdcursesmod" ]; then
	echo "fetching ../pdcursesmod from https://github.com/Bill-Gray/PDCursesMod.git" ; \
	git clone --depth 1 https://github.com/Bill-Gray/PDCursesMod.git pdcursesmod
fi

if [ ! -d djgpp/djgpp-patch ]; then
    echo "fetching djlsr205.zip" ;
    cd djgpp
    mkdir -p djgpp-patch
    cd djgpp-patch
    if [ "$(uname)" = "Darwin" ]; then
	#Mac
	curl --output djlsr205.zip http://www.mirrorservice.org/sites/ftp.delorie.com/pub/djgpp/current/v2/djlsr205.zip
        export cmdstatus=$?
    else
	wget --quiet --no-hsts http://www.mirrorservice.org/sites/ftp.delorie.com/pub/djgpp/current/v2/djlsr205.zip
        export cmdstatus=$?
    fi
    ls -l
    if [ $cmdstatus -eq 0 ]; then
	    echo "fetch of djgpp-patch was successful"
    else
	if [ -z "${TF_BUILD}" ]; then
		echo "Unable to complete the build, exiting..."
	else
		set +x
	        echo "##vso[task.logissue type=warning;]Trouble downloading djgpp-patch"    
    	fi
	exit 121
    fi
    mkdir -p src/libc/go32
    unzip -p djlsr205.zip src/libc/go32/exceptn.S >src/libc/go32/exceptn.S
    patch -p0 -l -i ../../../sys/msdos/exceptn.S.patch
    echo "djgpp patch applied"
    cd ../../
fi

# create a directories to hold some native DOS pieces that might get deployed
if [ ! -d djgpp/target ]; then
    cd djgpp
    mkdir -p target
    cd ../
fi
if [ ! -d djgpp/target/bin ]; then
    mkdir -p djgpp/target/bin
fi
if [ ! -d djgpp/target/lib ]; then
    mkdir -p djgpp/target/lib
fi
if [ ! -d djgpp/target/include ]; then
    mkdir -p djgpp/target/include
fi


# These 4 arrays must have their elements kept in synch.
#
# full URL for the remote zip file to fetch
zipurl=(
	"http://www.mirrorservice.org/sites/ftp.delorie.com/pub/djgpp/current/v2/djdev205.zip"
	"http://www.mirrorservice.org/sites/ftp.delorie.com/pub/djgpp/current/v2gnu/gdb801b.zip"
)
# name of zip file upon landing here
ziplocal=(
	"djdev205.zip"
	"gdb801b.zip"
)
# name of file after extraction from zip file
exesought=(
	"bin/symify.exe"
	"bin/gdb.exe"

)
# stored full path inside zip file
exepathinzip=(
	"bin/symify.exe"
	"bin/gdb.exe"
)

if [ -d djgpp/target ]; then
   cd djgpp/target
   count=0
   while [ "x${zipurl[count]}" != "x" ]
   do
     if [ ! -f ${exesought[count]} ]; then
       if [ ! -f ${ziplocal[count]} ]; then
         echo "Getting ${ziplocal[count]}" ;
         if [ "$(uname)" = "Darwin" ]; then
           #Mac
	   curl --output ${ziplocal[count]} ${zipurl[count]}
           export cmdstatus=$?
         else
	   wget --quiet --no-hsts ${zipurl[count]}
           export cmdstatus=$?
         fi
         if [ $cmdstatus -eq 0 ]; then
	   echo "fetch of ${ziplocal[count]} was successful"
         fi
       fi
       if [ -f ${ziplocal[count]} ]; then
          echo "unzipping ${ziplocal[count]}"
          unzip -p ${ziplocal[count]} ${exepathinzip[count]} >./${exesought[count]}
       fi
     fi
     count=$(( $count + 1 ))
   done

   echo "Native DOS executables in lib/djgpp/target:"
   ls -l bin/*.exe
   #
   cd ../../
fi


FONT_VERSION="4.49"
FONT_FILE="terminus-font-$FONT_VERSION"
FONT_LFILE="$FONT_FILE.1"
FONT_RFILE="$FONT_LFILE.tar.gz"
FONT_URL="https://sourceforge.net/projects/terminus-font/files/$FONT_FILE/$FONT_RFILE"

#  fonts
if [ ! -d "$FONT_LFILE" ]; then
    echo "Getting terminus fonts"
    if [ "$(uname)" = "Darwin" ]; then
	#Mac
	curl -L $FONT_URL --output $FONT_RFILE
    else
	wget --quiet --no-hsts $FONT_URL
    fi
    tar -xvf $FONT_RFILE
    rm $FONT_RFILE
else
	echo "terminus fonts are already available in lib/$FONT_LFILE"
fi

cd ../

# Don't fail the build if lua fetch failed because we cannot do anything about it
# but don't bother proceeding forward either
if [ ! -d "lib/lua-$LUA_VERSION/src" ]; then
        exit 0
fi

