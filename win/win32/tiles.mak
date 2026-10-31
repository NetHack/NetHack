# $NHDT-Header: NetHack 5.0 tiles.mak 2018/04/25_20:54:50 NetHack-5.0:1.6 $
# Copyright (c) 2002 by Michael Allison
# NetHack may be freely redistributed.  See license for details.

default: all

all: ..\win\win32\tiles.bmp

clean:
	-del ..\src\win\win32\tiles.bmp
	-del ..\win\win32\tiles.bmp

#==========================================
# Building the tiles file tile.bmp
#==========================================

..\src\tiles.bmp : ..\win\share\monsters.txt ..\win\share\objects.txt \
			 ..\win\share\other.txt
	   chdir ..\src
	   ..\util\tile2bmp.exe tiles.bmp
	   chdir ..\build

..\win\win32\tiles.bmp: ..\src\tiles.bmp
	@copy ..\src\tiles.bmp ..\win\win32\tiles.bmp

