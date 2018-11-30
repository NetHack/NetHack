#$NHDT-Date: 1524689255 2018/04/25 20:47:35 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.4 $
# Copyright (c) 2002 by Michael Allison
# NetHack may be freely redistributed.  See license for details.

SHELL=cmd.exe

default: all

all: ../win/win32/tiles.bmp

clean:
	-del ..\src\win\win32\tiles.bmp
	-del ..\win\win32\tiles.bmp

#==========================================
# Building the tiles file tile.bmp
#==========================================

../src/tiles.bmp : ../win/share/monsters.txt ../win/share/objects.txt \
			 ../win/share/other.txt
	   chdir ..\src
	   ..\util\tile2bmp.exe tiles.bmp
	   chdir ..\build

..\win\win32\tiles.bmp: ..\src\tiles.bmp
	@copy ..\src\tiles.bmp ..\win\win32\tiles.bmp

