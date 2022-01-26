set -x
mkdir -p lib
cd lib
git clone --depth 1 https://github.com/wmcbrine/PDCurses.git pdcurses
#git clone --depth 1 https://github.com/universal-ctags/ctags.git ctags
curl -R -O http://www.lua.org/ftp/lua-5.4.4.tar.gz
tar zxf lua-5.4.4.tar.gz
cd ../
