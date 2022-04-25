set -x
export VSVER=2017
export MSVER=14.16.27023
export SDKVER=10.0.17763.0
export FRAMEVER=4.0.30319
export NETFXVER=4.6.1
export WKITVER=10.0.17134.0
#export TOOLSVER=Enterprise
export TOOLSVER=BuildTools
export PATH=/c/Program\ Files\ \(x86\)/Microsoft\ Visual\ Studio/$VSVER/$TOOLSVER/Common7/IDE/VC/VCPackages:$PATH
export PATH=/c/Program\ Files\ \(x86\)/Windows\ Kits/10/bin/$WKITVER/x64:$PATH
export PATH=/c/Program\ Files\ \(x86\)/Microsoft\ Visual\ Studio/$VSVER/$TOOLSVER/VC/Tools/MSVC/$MSVER/bin/HostX64/x64:$PATH
export PATH=/c/Program\ Files\ \(x86\)/Microsoft\ Visual\ Studio/$VSVER/$TOOLSVER/VC/Tools/MSVC/$MSVER/bin/HostX64/x86:$PATH
export PATH=$PATH:/c/Program\ Files\ \(x86\)/Microsoft\ Visual\ Studio/$VSVER/$TOOLSVER/Common7/IDE/CommonExtensions/Microsoft/TestWindow
export PATH=$PATH:/c/Program\ Files\ \(x86\)/Microsoft\ Visual\ Studio/$VSVER/$TOOLSVER/MSBuild/Current/bin/Roslyn
export INCLUDE=/c/Program\ Files\ \(x86\)/Microsoft\ Visual\ Studio/2017/$TOOLSVER/VC/Tools/MSVC/$MSVER/include
export INCLUDE=$INCLUDE:/c/Program\ Files\ \(x86\)/Windows\ Kits/10/Include/$WKITVER/ucrt
export INCLUDE=$INCLUDE:/c/Program\ Files\ \(x86\)/Windows\ Kits/10/include/$WKITVER/ucrt
export INCLUDE=$INCLUDE:/c/Program\ Files\ \(x86\)/Windows\ Kits/10/include/$WKITVER/shared
export INCLUDE=$INCLUDE:/c/Program\ Files\ \(x86\)/Windows\ Kits/10/include/$WKITVER/um
export INCLUDE=$INCLUDE:/c/Program\ Files\ \(x86\)/Windows\ Kits/10/include/$WKITVER/winrt
export INCLUDE=$INCLUDE:/c/Program\ Files\ \(x86\)/Windows\ Kits/10/include/$WKITVER/cppwinrt
export LIB=/c/Program\ Files\ \(x86\)/Microsoft\ Visual\ Studio/$VSVER/$TOOLSVER/VC/Tools/MSVC/$MSVER/ATLMFC/lib/x86
export LIB=/c/Program\ Files\ \(x86\)/Microsoft\ Visual\ Studio/$VSVER/$TOOLSVER/VC/Tools/MSVC/$MSVER/lib/x86:$LIB
export LIB=/c/Program\ Files\ \(x86\)/Windows\ Kits/10/lib/$WKITVER/ucrt/x86:$LIB
export LIB=/c/Program\ Files\ \(x86\)/Windows\ Kits/10/lib/$WKITVER/um/x86:$LIB
export LUA_VERSION=5.4.3
mkdir -p lib
cd lib
git clone --depth 1 https://github.com/wmcbrine/PDCurses.git pdcurses
#git clone --depth 1 https://github.com/universal-ctags/ctags.git ctags
curl -R -O http://www.lua.org/ftp/lua-$LUA_VERSION.tar.gz
tar zxf lua-$LUA_VERSION.tar.gz
#cd ctags
#nmake -f mk_mvc.mak
#cd ../../
cd ../
test -d "lib/lua-$LUA_VERSION/src" || echo "Lua-$LUA_VERSION fetch failed"
test -d "lib/pdcurses" || echo "pdcurses fetch failed"
test -d "lib/pdcurses" || exit 0
test -d "lib/lua-$LUA_VERSION/src" || exit 0
export ADD_CURSES=Y
export PDCURSES_TOP=../lib/pdcurses
export
cd src
cp ../sys/windows/Makefile.nmake ./Makefile
nmake install
cd ..
powershell -Command "Compress-Archive -U -Path binary/* -DestinationPath $TRAVIS_TAG.x86.zip"
