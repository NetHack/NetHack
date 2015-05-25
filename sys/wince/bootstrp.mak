# NetHack 3.6	bootstrp.mak	$NHDT-Date: 1432512801 2015/05/25 00:13:21 $  $NHDT-Branch: master $:$NHDT-Revision: 1.13 $
#       Copyright (c) Michael Allison
#
#       NetHack Windows CE bootstrap file for MS Visual C++ V6.x and 
#       above and MS NMAKE
#
#       This will:
#         - build makedefs
#         - 
#==============================================================================
# Do not delete the following 3 lines.
#
TARGETOS=BOTH
APPVER=4.0
!include <win32.mak>

#
#  Source directories.    Makedefs hardcodes these, don't change them.
#

INCL  = ..\include   # NetHack include files
DAT   = ..\dat       # NetHack data files
DOC   = ..\doc       # NetHack documentation files
UTIL  = ..\util      # Utility source
SRC   = ..\src       # Main source
SSYS  = ..\sys\share # Shared system files
NTSYS = ..\sys\winnt # NT Win32 specific files
TTY   = ..\win\tty   # window port files (tty)
WIN32 = ..\win\win32 # window port files (WINCE)
WSHR  = ..\win\share # Tile support files 
SWINCE= ..\wince 	   # wince files
WINCE = ..\wince     # wince build area
OBJ   = $(WINCE)\ceobj
DLB = $(DAT)\nhdat

#==========================================
# Setting up the compiler and linker
# macros. All builds include the base ones.
#==========================================

CFLAGSBASE  = -c $(cflags) $(cvarsmt) -I$(INCL) -nologo $(cdebug) $(WINPINC) -DDLB
LFLAGSBASEC = $(linkdebug) /NODEFAULTLIB /INCREMENTAL:NO /RELEASE /NOLOGO -subsystem:console,4.0 $(conlibsmt)
LFLAGSBASEG = $(linkdebug) $(guiflags) $(guilibsmt) comctl32.lib

#==========================================
# Util builds
#==========================================

CFLAGSU = $(CFLAGSBASE) $(WINPFLAG)
LFLAGSU	= $(LFLAGSBASEC)

LEVCFLAGS= -c -nologo -DWINVER=0x0400 -DWIN32 -D_WIN32 \
	   -D_MT -MT -I..\include -nologo -Z7 -Od -DDLB


#==========================================
#================ RULES ==================
#==========================================

.SUFFIXES: .exe .o .til .uu .c .y .l

#==========================================
# Rules for files in src
#==========================================

#.c{$(OBJ)}.o:
#	$(cc) $(CFLAGSU)  -Fo$@ $<

{$(SRC)}.c{$(OBJ)}.o:
	$(CC) $(CFLAGSU)   -Fo$@  $<

#==========================================
# Rules for files in sys\share
#==========================================

{$(SSYS)}.c{$(OBJ)}.o:
	$(CC) $(CFLAGSU)  -Fo$@  $<

#==========================================
# Rules for files in sys\winnt
#==========================================

{$(NTSYS)}.c{$(OBJ)}.o:
	$(CC) $(CFLAGSU)  -Fo$@  $<

{$(NTSYS)}.h{$(INCL)}.h:
	copy $< $@

#==========================================
# Rules for files in util
#==========================================

{$(UTIL)}.c{$(OBJ)}.o:
	$(CC) $(CFLAGSU) -Fo$@ $<

#==========================================
# Rules for files in win\share
#==========================================

{$(WSHR)}.c{$(OBJ)}.o:
	$(CC) $(CFLAGSU)  -Fo$@ $<

{$(WSHR)}.h{$(INCL)}.h:
	copy $< $@

#{$(WSHR)}.txt{$(DAT)}.txt:
#	copy $< $@

#==========================================
# Rules for files in win\tty
#==========================================

{$(TTY)}.c{$(OBJ)}.o:
	$(CC) $(CFLAGSU)  -Fo$@  $<


#==========================================
# Rules for files in win\win32
#==========================================

{$(WIN32)}.c{$(OBJ)}.o:
	$(cc) $(CFLAGSU)  -Fo$@  $<

#==========================================
# Rules for files in sys\wince
#==========================================

{$(SWINCE)}.c{$(OBJ)}.o:
	$(cc) $(CFLAGSU)  -Fo$@  $<

#==========================================
#================ MACROS ==================
#==========================================

#
# Shorten up the location for some files
#

O  = $(OBJ)^\

U  = $(UTIL)^\

#
# Utility Objects.
#

MAKESRC        = $(U)makedefs.c

SPLEVSRC       = $(U)lev_yacc.c	$(U)lev_$(LEX).c $(U)lev_main.c  $(U)panic.c

DGNCOMPSRC     = $(U)dgn_yacc.c	$(U)dgn_$(LEX).c $(U)dgn_main.c

MAKEOBJS       = $(O)makedefs.o $(O)monst.o $(O)objects.o

SPLEVOBJS      = $(O)lev_yacc.o	$(O)lev_$(LEX).o $(O)lev_main.o \
		 $(O)alloc.o	$(O)decl.o	$(O)drawing.o \
		 $(O)monst.o	$(O)objects.o	$(O)panic.o

DGNCOMPOBJS    = $(O)dgn_yacc.o	$(O)dgn_$(LEX).o $(O)dgn_main.o \
		 $(O)alloc.o	$(O)panic.o

TILEFILES      = $(WSHR)\monsters.txt $(WSHR)\objects.txt $(WSHR)\other.txt

#
# These are not invoked during a normal game build in 3.5.0
#
TEXT_IO        = $(O)tiletext.o	$(O)tiletxt.o	$(O)drawing.o \
		 $(O)decl.o	$(O)monst.o	$(O)objects.o

TEXT_IO32      = $(O)tilete32.o $(O)tiletx32.o $(O)drawing.o \
		 $(O)decl.o	$(O)monst.o	$(O)objects.o

GIFREADERS     = $(O)gifread.o	$(O)alloc.o $(O)panic.o
GIFREADERS32   = $(O)gifrd32.o $(O)alloc.o $(O)panic.o

PPMWRITERS     = $(O)ppmwrite.o $(O)alloc.o $(O)panic.o

DLBOBJ = $(O)dlb.o

#==========================================
# Header file macros
#==========================================

CONFIG_H = $(INCL)\config.h $(INCL)\config1.h $(INCL)\tradstdc.h \
		$(INCL)\global.h $(INCL)\coord.h $(INCL)\vmsconf.h \
		$(INCL)\system.h $(INCL)\unixconf.h $(INCL)\os2conf.h \
		$(INCL)\micro.h $(INCL)\pcconf.h $(INCL)\tosconf.h \
		$(INCL)\amiconf.h $(INCL)\macconf.h $(INCL)\beconf.h \
		$(INCL)\ntconf.h $(INCL)\wceconf.h

HACK_H = $(INCL)\hack.h $(CONFIG_H) $(INCL)\align.h \
		$(INCL)\dungeon.h $(INCL)\monsym.h $(INCL)\mkroom.h \
		$(INCL)\objclass.h $(INCL)\youprop.h $(INCL)\prop.h \
		$(INCL)\permonst.h $(INCL)\mextra.h $(INCL)\monattk.h \
		$(INCL)\monflag.h $(INCL)\mondata.h $(INCL)\pm.h \
		$(INCL)\wintype.h $(INCL)\decl.h $(INCL)\quest.h \
		$(INCL)\spell.h $(INCL)\color.h $(INCL)\obj.h \
		$(INCL)\you.h $(INCL)\attrib.h $(INCL)\monst.h \
		$(INCL)\skills.h $(INCL)\onames.h $(INCL)\timeout.h \
		$(INCL)\trap.h $(INCL)\flag.h $(INCL)\rm.h \
		$(INCL)\vision.h $(INCL)\display.h $(INCL)\engrave.h \
		$(INCL)\rect.h $(INCL)\region.h $(INCL)\winprocs.h \
		$(INCL)\wintty.h $(INCL)\trampoli.h

LEV_H       = $(INCL)\lev.h
DGN_FILE_H  = $(INCL)\dgn_file.h
LEV_COMP_H  = $(INCL)\lev_comp.h
SP_LEV_H    = $(INCL)\sp_lev.h
TILE_H      = ..\win\share\tile.h

#==========================================
# Miscellaneous
#==========================================

DATABASE = $(DAT)\data.base

#==========================================
#=============== TARGETS ==================
#==========================================

#
#  The default make target (so just typing 'nmake' is useful).
#
default : all

#
#  Everything
#

all :	$(INCL)\date.h	$(INCL)\onames.h $(INCL)\pm.h \
	$(SRC)\monstr.c	$(SRC)\vis_tab.c $(U)lev_comp.exe $(INCL)\vis_tab.h \
	$(U)dgn_comp.exe $(U)uudecode.exe \
	$(DAT)\data	$(DAT)\rumors	 $(DAT)\dungeon \
	$(DAT)\oracles	$(DAT)\quest.dat $(O)sp_lev.tag $(DLB) $(SRC)\tile.c \
	$(SWINCE)\nethack.ico $(SWINCE)\tiles.bmp $(SWINCE)\mnsel.bmp \
	$(SWINCE)\mnunsel.bmp $(SWINCE)\petmark.bmp $(SWINCE)\mnselcnt.bmp \
	$(SWINCE)\keypad.bmp $(SWINCE)\menubar.bmp
	@echo Done!

$(O)sp_lev.tag:  $(DAT)\bigroom.des  $(DAT)\castle.des \
	$(DAT)\endgame.des $(DAT)\gehennom.des $(DAT)\knox.des   \
	$(DAT)\medusa.des  $(DAT)\oracle.des   $(DAT)\tower.des  \
	$(DAT)\yendor.des  $(DAT)\arch.des     $(DAT)\barb.des   \
	$(DAT)\caveman.des $(DAT)\healer.des   $(DAT)\knight.des \
	$(DAT)\monk.des    $(DAT)\priest.des   $(DAT)\ranger.des \
	$(DAT)\rogue.des   $(DAT)\samurai.des  $(DAT)\sokoban.des \
	$(DAT)\tourist.des $(DAT)\valkyrie.des $(DAT)\wizard.des
	cd $(DAT)
	$(U)lev_comp bigroom.des
	$(U)lev_comp castle.des
	$(U)lev_comp endgame.des
	$(U)lev_comp gehennom.des
	$(U)lev_comp knox.des
	$(U)lev_comp mines.des
	$(U)lev_comp medusa.des
	$(U)lev_comp oracle.des
	$(U)lev_comp sokoban.des
	$(U)lev_comp tower.des
	$(U)lev_comp yendor.des
	$(U)lev_comp arch.des
	$(U)lev_comp barb.des
	$(U)lev_comp caveman.des
	$(U)lev_comp healer.des
	$(U)lev_comp knight.des
	$(U)lev_comp monk.des
	$(U)lev_comp priest.des
	$(U)lev_comp ranger.des
	$(U)lev_comp rogue.des
	$(U)lev_comp samurai.des
	$(U)lev_comp tourist.des
	$(U)lev_comp valkyrie.des
	$(U)lev_comp wizard.des
	cd $(WINCE)
	echo sp_levs done > $(O)sp_lev.tag

#$(NHRES): $(TILEBMP16) $(WINCE)\winhack.rc $(WINCE)\mnsel.bmp \
#	$(WINCE)\mnselcnt.bmp $(WINCE)\mnunsel.bmp \
#	$(WINCE)\petmark.bmp $(WINCE)\NetHack.ico $(WINCE)\rip.bmp \
#	$(WINCE)\splash.bmp
#	$(rc) -r -fo$@ -i$(WINCE) -dNDEBUG $(WINCE)\winhack.rc

#
#  Utility Targets.
#
    
#==========================================
# Makedefs Stuff
#==========================================

$(U)makedefs.exe:	$(MAKEOBJS)
	$(link) $(LFLAGSU) -out:$@ $(MAKEOBJS)

$(O)makedefs.o: $(CONFIG_H)	$(INCL)\monattk.h $(INCL)\monflag.h   $(INCL)\objclass.h \
		 $(INCL)\monsym.h    $(INCL)\qtext.h	$(INCL)\patchlevel.h \
		 $(U)makedefs.c
	if not exist $(OBJ)\*.* echo creating directory $(OBJ)
	if not exist $(OBJ)\*.* mkdir $(OBJ)
	$(CC) $(CFLAGSU) -Fo$@ $(U)makedefs.c

#
#  date.h should be remade every time any of the source or include
#  files is modified.
#

$(INCL)\date.h $(OPTIONS_FILE) : $(U)makedefs.exe
	$(U)makedefs -v

$(INCL)\onames.h : $(U)makedefs.exe
	$(U)makedefs -o

$(INCL)\pm.h : $(U)makedefs.exe
	$(U)makedefs -p

#$(INCL)\trap.h : $(U)makedefs.exe
#	$(U)makedefs -t

$(SRC)\monstr.c: $(U)makedefs.exe
	$(U)makedefs -m

$(INCL)\vis_tab.h: $(U)makedefs.exe
	$(U)makedefs -z

$(SRC)\vis_tab.c: $(U)makedefs.exe
	$(U)makedefs -z

#==========================================
# uudecode utility and uuencoded targets
#==========================================

$(U)uudecode.exe: $(O)uudecode.o
	$(link) $(LFLAGSU) -out:$@ $(O)uudecode.o

$(O)uudecode.o: $(SSYS)\uudecode.c

$(SWINCE)\NetHack.ico : $(U)uudecode.exe $(SWINCE)\nhico.uu 
	chdir $(SWINCE)
	..\util\uudecode.exe nhico.uu
	chdir $(WINCE)

$(SWINCE)\mnsel.bmp: $(U)uudecode.exe $(SWINCE)\mnsel.uu
	chdir $(SWINCE)
	..\util\uudecode.exe mnsel.uu
	chdir $(WINCE)

$(SWINCE)\mnselcnt.bmp: $(U)uudecode.exe $(SWINCE)\mnselcnt.uu
	chdir $(SWINCE)
	..\util\uudecode.exe mnselcnt.uu
	chdir $(WINCE)

$(SWINCE)\mnunsel.bmp: $(U)uudecode.exe $(SWINCE)\mnunsel.uu
	chdir $(SWINCE)
	..\util\uudecode.exe mnunsel.uu
	chdir $(WINCE)

$(SWINCE)\petmark.bmp: $(U)uudecode.exe $(SWINCE)\petmark.uu
	chdir $(SWINCE)
	..\util\uudecode.exe petmark.uu
	chdir $(WINCE)

$(SWINCE)\rip.bmp: $(U)uudecode.exe $(SWINCE)\rip.uu
	chdir $(SWINCE)
	..\util\uudecode.exe rip.uu
	chdir $(WINCE)

$(SWINCE)\splash.bmp: $(U)uudecode.exe $(SWINCE)\splash.uu
	chdir $(SWINCE)
	..\util\uudecode.exe splash.uu
	chdir $(WINCE)

$(SWINCE)\keypad.bmp: $(U)uudecode.exe $(SWINCE)\keypad.uu
	chdir $(SWINCE)
	..\util\uudecode.exe keypad.uu
	chdir $(WINCE)

$(SWINCE)\menubar.bmp: $(U)uudecode.exe $(SWINCE)\menubar.uu
	chdir $(SWINCE)
	..\util\uudecode.exe menubar.uu
	chdir $(WINCE)

#==========================================
# Level Compiler Stuff
#==========================================

$(U)lev_comp.exe: $(SPLEVOBJS)
	echo Linking $@...
	$(link) $(LFLAGSU) -out:$@ @<<$(@B).lnk
 		$(SPLEVOBJS:^	=^
		)
<<

$(O)lev_yacc.o: $(HACK_H)   $(SP_LEV_H) $(INCL)\lev_comp.h $(U)lev_yacc.c
	$(CC) $(LEVCFLAGS) -W0 -Fo$@ $(U)lev_yacc.c

$(O)lev_$(LEX).o: $(HACK_H)   $(INCL)\lev_comp.h $(SP_LEV_H) \
               $(U)lev_$(LEX).c
	$(CC) $(LEVCFLAGS) -W0 -Fo$@ $(U)lev_$(LEX).c

$(O)lev_main.o:	$(U)lev_main.c $(HACK_H)   $(SP_LEV_H)
	$(CC) $(LEVCFLAGS) -W0 -Fo$@ $(U)lev_main.c


$(U)lev_yacc.c $(INCL)\lev_comp.h : $(U)lev_comp.y
	   @echo We will copy the prebuilt lev_yacc.c and 
	   @echo lev_comp.h from $(SSYS) into $(UTIL) and use them.
	   @copy $(SSYS)\lev_yacc.c $(U)lev_yacc.c >nul
	   @copy $(SSYS)\lev_comp.h $(INCL)\lev_comp.h >nul
	   @echo /**/ >>$(U)lev_yacc.c
	   @echo /**/ >>$(INCL)\lev_comp.h

$(U)lev_$(LEX).c: $(U)lev_comp.l
	   @echo We will copy the prebuilt lev_lex.c 
	   @echo from $(SSYS) into $(UTIL) and use it.
	   @copy $(SSYS)\lev_lex.c $@ >nul
	   @echo /**/ >>$@

#==========================================
# Dungeon Compiler Stuff
#==========================================

$(U)dgn_comp.exe: $(DGNCOMPOBJS)
    @echo Linking $@...
	$(link) $(LFLAGSU) -out:$@ @<<$(@B).lnk
		$(DGNCOMPOBJS:^	=^
		)
<<

$(O)dgn_yacc.o:	$(HACK_H)   $(DGN_FILE_H) $(INCL)\dgn_comp.h $(U)dgn_yacc.c
	$(CC) $(LEVCFLAGS) -W0 -Fo$@ $(U)dgn_yacc.c

$(O)dgn_$(LEX).o: $(HACK_H)   $(DGN_FILE_H)  $(INCL)\dgn_comp.h \
	$(U)dgn_$(LEX).c
	$(CC) $(LEVCFLAGS) -W0 -Fo$@ $(U)dgn_$(LEX).c

$(O)dgn_main.o:	$(HACK_H) $(U)dgn_main.c
	$(CC) $(LEVCFLAGS) -W0 -Fo$@ $(U)dgn_main.c

$(U)dgn_yacc.c $(INCL)\dgn_comp.h : $(U)dgn_comp.y
	   @echo We will copy the prebuilt $(U)dgn_yacc.c and 
	   @echo dgn_comp.h from $(SSYS) into $(UTIL) and use them.
	   @copy $(SSYS)\dgn_yacc.c $(U)dgn_yacc.c >nul
	   @copy $(SSYS)\dgn_comp.h $(INCL)\dgn_comp.h >nul
	   @echo /**/ >>$(U)dgn_yacc.c
	   @echo /**/ >>$(INCL)\dgn_comp.h

$(U)dgn_$(LEX).c: $(U)dgn_comp.l
	   @echo We will copy the prebuilt dgn_lex.c 
	   @echo from $(SSYS) into $(UTIL) and use it.
	   @copy $(SSYS)\dgn_lex.c $@ >nul
	   @echo /**/ >>$@

#==========================================
# Create directory for holding object files
#==========================================

$(O)obj.tag:
	if not exist $(OBJ)\*.* echo creating directory $(OBJ)
	if not exist $(OBJ)\*.* mkdir $(OBJ)
	echo directory created >$@

#==========================================
# Notify of any CL environment variables
# in effect since they change the compiler
# options.
#==========================================

envchk:
!	IF "$(CL)"!=""
	   @echo Warning, the CL Environment variable is defined:
	   @echo CL=$(CL)
!	ENDIF
	   @echo ----
	   @echo NOTE: This build will include tile support.
	   @echo ----

#==========================================
#=========== SECONDARY TARGETS ============
#==========================================

#===========================================
# Header files NOT distributed in ..\include
#===========================================

$(INCL)\win32api.h: $(NTSYS)\win32api.h
	copy $(NTSYS)\win32api.h $@


#==========================================
# DLB utility and nhdat file creation
#==========================================

$(U)dlb_main.exe: $(DLBOBJ) $(O)dlb.o
	$(link) $(LFLAGSU) -out:$@ @<<$(@B).lnk
		$(O)dlb_main.o
		$(O)dlb.o
		$(O)alloc.o
		$(O)panic.o
<<

$(O)dlb.o:	$(O)dlb_main.o $(O)alloc.o $(O)panic.o $(INCL)\dlb.h
	$(CC) $(CFLAGSU) /Fo$@ $(SRC)\dlb.c
	
$(O)dlb_main.o: $(UTIL)\dlb_main.c $(INCL)\config.h $(INCL)\dlb.h
	$(CC) $(CFLAGSU) /Fo$@ $(UTIL)\dlb_main.c

#$(DAT)\porthelp: $(NTSYS)\porthelp
#	copy $(NTSYS)\porthelp $@ >nul

$(DAT)\nhdat:	$(U)dlb_main.exe $(DAT)\data $(DAT)\oracles $(OPTIONS_FILE) \
	$(DAT)\quest.dat $(DAT)\rumors $(DAT)\help $(DAT)\hh $(DAT)\cmdhelp \
	$(DAT)\history $(DAT)\opthelp $(DAT)\wizhelp $(DAT)\dungeon  \
	$(DAT)\license $(O)sp_lev.tag
	cd $(DAT)
	echo data >dlb.lst
	echo oracles >>dlb.lst
	if exist options echo options >>dlb.lst
	if exist ttyoptions echo ttyoptions >>dlb.lst
	if exist guioptions echo guioptions >>dlb.lst
	if exist porthelp echo porthelp >>dlb.lst
	echo quest.dat >>dlb.lst
	echo rumors >>dlb.lst
	echo help >>dlb.lst
	echo hh >>dlb.lst
	echo cmdhelp >>dlb.lst
	echo history >>dlb.lst
	echo opthelp >>dlb.lst
	echo wizhelp >>dlb.lst
	echo dungeon >>dlb.lst
	echo license >>dlb.lst
	for %%N in (*.lev) do echo %%N >>dlb.lst
	$(U)dlb_main cIf dlb.lst nhdat
	cd $(WINCE)

#==========================================
#  Tile Mapping
#==========================================

$(SRC)\tile.c: $(U)tilemap.exe
	echo A new $@ has been created
	$(U)tilemap

$(U)tilemap.exe: $(O)tilemap.o
	$(link) $(LFLAGSU) -out:$@ $(O)tilemap.o

$(O)tilemap.o: $(WSHR)\tilemap.c $(HACK_H)
	$(CC) $(CFLAGSU) -Fo$@ $(WSHR)\tilemap.c

$(O)tiletx32.o: $(WSHR)\tilemap.c $(HACK_H)
	$(CC) $(CFLAGSU) /DTILETEXT /DTILE_X=32 /DTILE_Y=32 -Fo$@ $(WSHR)\tilemap.c

$(O)tiletxt.o: $(WSHR)\tilemap.c $(HACK_H)
	$(CC) $(CFLAGSU) /DTILETEXT -Fo$@ $(WSHR)\tilemap.c

$(O)gifread.o: $(WSHR)\gifread.c  $(CONFIG_H) $(TILE_H)
	$(CC) $(CFLAGSU) -I$(WSHR) -Fo$@ $(WSHR)\gifread.c

$(O)gifrd32.o: $(WSHR)\gifread.c  $(CONFIG_H) $(TILE_H)
	$(CC) $(CFLAGSU) -I$(WSHR) /DTILE_X=32 /DTILE_Y=32 -Fo$@ $(WSHR)\gifread.c

$(O)ppmwrite.o: $(WSHR)\ppmwrite.c $(CONFIG_H) $(TILE_H)
	$(CC) $(CFLAGSU) -I$(WSHR) -Fo$@ $(WSHR)\ppmwrite.c

$(O)tiletext.o: $(WSHR)\tiletext.c  $(CONFIG_H) $(TILE_H)
	$(CC) $(CFLAGSU) -I$(WSHR) -Fo$@ $(WSHR)\tiletext.c

$(O)tilete32.o: $(WSHR)\tiletext.c  $(CONFIG_H) $(TILE_H)
	$(CC) $(CFLAGSU) -I$(WSHR) /DTILE_X=32 /DTILE_Y=32 -Fo$@ $(WSHR)\tiletext.c

$(SWINCE)\tiles.bmp: $(U)tile2bmp.exe $(TILEFILES)
	echo Creating 16x16 binary tile files (this may take some time)
	$(U)tile2bmp $@

#$(TILEBMP32): $(TILEUTIL32) $(TILEFILES32)
#	echo Creating 32x32 binary tile files (this may take some time)
#	$(U)til2bm32 $(TILEBMP32)


$(U)tile2bmp.exe: $(O)tile2bmp.o $(TEXT_IO)
    @echo Linking $@...
	$(link) $(LFLAGSU) -out:$@ @<<$(@B).lnk
		$(O)tile2bmp.o
		$(TEXT_IO:^  =^
		)
<<

$(U)til2bm32.exe: $(O)til2bm32.o $(TEXT_IO32)
    @echo Linking $@...
	$(link) $(LFLAGSU) -out:$@ @<<$(@B).lnk
		$(O)til2bm32.o
		$(TEXT_IO32:^  =^
		)
<<

$(O)tile2bmp.o: $(WSHR)\tile2bmp.c $(HACK_H) $(TILE_H) $(INCL)\win32api.h
	$(CC) $(CFLAGSU) -I$(WSHR) /DPACKED_FILE /Fo$@ $(WSHR)\tile2bmp.c

$(O)til2bm32.o: $(WSHR)\tile2bmp.c $(HACK_H) $(TILE_H) $(INCL)\win32api.h
	$(CC) $(CFLAGSU) -I$(WSHR) /DPACKED_FILE /DTILE_X=32 /DTILE_Y=32 /Fo$@ $(WSHR)\tile2bmp.c

#===================================================================
# OTHER DEPENDENCIES
#===================================================================

#
# dat dependencies
#

$(DAT)\data: $(UTIL)\makedefs.exe
	$(U)makedefs -d

$(DAT)\rumors: $(UTIL)\makedefs.exe    $(DAT)\rumors.tru   $(DAT)\rumors.fal
	$(U)makedefs -r

$(DAT)\quest.dat: $(UTIL)\makedefs.exe  $(DAT)\quest.txt
	$(U)makedefs -q

$(DAT)\oracles: $(UTIL)\makedefs.exe    $(DAT)\oracles.txt
	$(U)makedefs -h

$(DAT)\dungeon: $(UTIL)\makedefs.exe  $(DAT)\dungeon.def
	$(U)makedefs -e
	cd $(DAT)
	$(U)dgn_comp dungeon.pdf
	cd $(WINCE)

#
# NT dependencies
#
#
#$(O)nttty.o:   $(HACK_H) $(TILE_H) $(INCL)\win32api.h $(NTSYS)\nttty.c
#	$(CC) $(CFLAGSU) -I$(WSHR) -Fo$@  $(NTSYS)\nttty.c
#$(O)winnt.o: $(HACK_H) $(INCL)\win32api.h $(NTSYS)\winnt.c
#	$(CC) $(CFLAGSU) -Fo$@  $(NTSYS)\winnt.c
#$(O)ntsound.o: $(HACK_H) $(NTSYS)\ntsound.c
#	$(CC) $(CFLAGSU)  -Fo$@ $(NTSYS)\ntsound.c

# 
# util dependencies
#

$(O)panic.o:  $(U)panic.c $(CONFIG_H)
	$(CC) $(CFLAGSU) -Fo$@ $(U)panic.c

#
# The rest are stolen from sys/unix/Makefile.src, 
# with slashes changed to back-slashes 
# and -c (which is included in CFLAGSU) substituted
# with -Fo$@ , but otherwise untouched. That
# means that there is some irrelevant stuff
# in here, but maintenance should be easier.
#
$(O)tos.o: ..\sys\atari\tos.c $(HACK_H) $(INCL)\tcap.h
	$(CC) $(CFLAGSU) -Fo$@ ..\sys\atari\tos.c
$(O)pcmain.o: ..\sys\share\pcmain.c $(HACK_H) $(INCL)\dlb.h \
		$(INCL)\win32api.h
	$(CC) $(CFLAGSU) -Fo$@ ..\sys\share\pcmain.c
$(O)pcsys.o: ..\sys\share\pcsys.c $(HACK_H)
	$(CC) $(CFLAGSU) -Fo$@ ..\sys\share\pcsys.c
$(O)pctty.o: ..\sys\share\pctty.c $(HACK_H)
	$(CC) $(CFLAGSU) -Fo$@ ..\sys\share\pctty.c
$(O)pcunix.o: ..\sys\share\pcunix.c $(HACK_H)
	$(CC) $(CFLAGSU) -Fo$@ ..\sys\share\pcunix.c
$(O)random.o: ..\sys\share\random.c $(HACK_H)
	$(CC) $(CFLAGSU) -Fo$@ ..\sys\share\random.c
$(O)ioctl.o: ..\sys\share\ioctl.c $(HACK_H) $(INCL)\tcap.h
	$(CC) $(CFLAGSU) -Fo$@ ..\sys\share\ioctl.c
$(O)unixtty.o: ..\sys\share\unixtty.c $(HACK_H)
	$(CC) $(CFLAGSU) -Fo$@ ..\sys\share\unixtty.c
$(O)unixmain.o: ..\sys\unix\unixmain.c $(HACK_H) $(INCL)\dlb.h
	$(CC) $(CFLAGSU) -Fo$@ ..\sys\unix\unixmain.c
$(O)unixunix.o: ..\sys\unix\unixunix.c $(HACK_H)
	$(CC) $(CFLAGSU) -Fo$@ ..\sys\unix\unixunix.c
$(O)bemain.o: ..\sys\be\bemain.c $(HACK_H) $(INCL)\dlb.h
	$(CC) $(CFLAGSU) -Fo$@ ..\sys\be\bemain.c
$(O)getline.o: ..\win\tty\getline.c $(HACK_H) $(INCL)\func_tab.h
	$(CC) $(CFLAGSU) -Fo$@ ..\win\tty\getline.c
$(O)termcap.o: ..\win\tty\termcap.c $(HACK_H) $(INCL)\tcap.h
	$(CC) $(CFLAGSU) -Fo$@ ..\win\tty\termcap.c
$(O)topl.o: ..\win\tty\topl.c $(HACK_H) $(INCL)\tcap.h
	$(CC) $(CFLAGSU) -Fo$@ ..\win\tty\topl.c
$(O)wintty.o: ..\win\tty\wintty.c $(HACK_H) $(INCL)\dlb.h \
		$(INCL)\date.h $(INCL)\patchlevel.h $(INCL)\tcap.h
	$(CC) $(CFLAGSU) -Fo$@ ..\win\tty\wintty.c
$(O)Window.o: ..\win\X11\Window.c $(INCL)\xwindowp.h $(INCL)\xwindow.h \
		$(CONFIG_H)
	$(CC) $(CFLAGSU) -Fo$@ ..\win\X11\Window.c
$(O)dialogs.o: ..\win\X11\dialogs.c $(CONFIG_H)
	$(CC) $(CFLAGSU) -Fo$@ ..\win\X11\dialogs.c
$(O)winX.o: ..\win\X11\winX.c $(HACK_H) $(INCL)\winX.h $(INCL)\dlb.h \
		$(INCL)\patchlevel.h ..\win\X11\nh72icon \
		..\win\X11\nh56icon ..\win\X11\nh32icon
	$(CC) $(CFLAGSU) -Fo$@ ..\win\X11\winX.c
$(O)winmap.o: ..\win\X11\winmap.c $(INCL)\xwindow.h $(HACK_H) $(INCL)\dlb.h \
		$(INCL)\winX.h $(INCL)\tile2x11.h
	$(CC) $(CFLAGSU) -Fo$@ ..\win\X11\winmap.c
$(O)winmenu.o: ..\win\X11\winmenu.c $(HACK_H) $(INCL)\winX.h
	$(CC) $(CFLAGSU) -Fo$@ ..\win\X11\winmenu.c
$(O)winmesg.o: ..\win\X11\winmesg.c $(INCL)\xwindow.h $(HACK_H) $(INCL)\winX.h
	$(CC) $(CFLAGSU) -Fo$@ ..\win\X11\winmesg.c
$(O)winmisc.o: ..\win\X11\winmisc.c $(HACK_H) $(INCL)\func_tab.h \
		$(INCL)\winX.h
	$(CC) $(CFLAGSU) -Fo$@ ..\win\X11\winmisc.c
$(O)winstat.o: ..\win\X11\winstat.c $(HACK_H) $(INCL)\winX.h
	$(CC) $(CFLAGSU) -Fo$@ ..\win\X11\winstat.c
$(O)wintext.o: ..\win\X11\wintext.c $(HACK_H) $(INCL)\winX.h $(INCL)\xwindow.h
	$(CC) $(CFLAGSU) -Fo$@ ..\win\X11\wintext.c
$(O)winval.o: ..\win\X11\winval.c $(HACK_H) $(INCL)\winX.h
	$(CC) $(CFLAGSU) -Fo$@ ..\win\X11\winval.c
$(O)tile.o: $(SRC)\tile.c $(HACK_H)
$(O)gnaskstr.o: ..\win\gnome\gnaskstr.c ..\win\gnome\gnaskstr.h \
		..\win\gnome\gnmain.h
	$(CC) $(CFLAGSU) $(GNOMEINC) -c ..\win\gnome\gnaskstr.c
$(O)gnbind.o: ..\win\gnome\gnbind.c ..\win\gnome\gnbind.h ..\win\gnome\gnmain.h \
		..\win\gnome\gnaskstr.h ..\win\gnome\gnyesno.h
	$(CC) $(CFLAGSU) $(GNOMEINC) -c ..\win\gnome\gnbind.c
$(O)gnglyph.o: ..\win\gnome\gnglyph.c ..\win\gnome\gnglyph.h
	$(CC) $(CFLAGSU) $(GNOMEINC) -c ..\win\gnome\gnglyph.c
$(O)gnmain.o: ..\win\gnome\gnmain.c ..\win\gnome\gnmain.h ..\win\gnome\gnsignal.h \
		..\win\gnome\gnbind.h ..\win\gnome\gnopts.h $(HACK_H) \
		$(INCL)\date.h
	$(CC) $(CFLAGSU) $(GNOMEINC) -c ..\win\gnome\gnmain.c
$(O)gnmap.o: ..\win\gnome\gnmap.c ..\win\gnome\gnmap.h ..\win\gnome\gnglyph.h \
		..\win\gnome\gnsignal.h $(HACK_H)
	$(CC) $(CFLAGSU) $(GNOMEINC) -c ..\win\gnome\gnmap.c
$(O)gnmenu.o: ..\win\gnome\gnmenu.c ..\win\gnome\gnmenu.h ..\win\gnome\gnmain.h \
		..\win\gnome\gnbind.h
	$(CC) $(CFLAGSU) $(GNOMEINC) -c ..\win\gnome\gnmenu.c
$(O)gnmesg.o: ..\win\gnome\gnmesg.c ..\win\gnome\gnmesg.h ..\win\gnome\gnsignal.h
	$(CC) $(CFLAGSU) $(GNOMEINC) -c ..\win\gnome\gnmesg.c
$(O)gnopts.o: ..\win\gnome\gnopts.c ..\win\gnome\gnopts.h ..\win\gnome\gnglyph.h \
		..\win\gnome\gnmain.h ..\win\gnome\gnmap.h $(HACK_H)
	$(CC) $(CFLAGSU) $(GNOMEINC) -c ..\win\gnome\gnopts.c
$(O)gnplayer.o: ..\win\gnome\gnplayer.c ..\win\gnome\gnplayer.h \
		..\win\gnome\gnmain.h $(HACK_H)
	$(CC) $(CFLAGSU) $(GNOMEINC) -c ..\win\gnome\gnplayer.c
$(O)gnsignal.o: ..\win\gnome\gnsignal.c ..\win\gnome\gnsignal.h \
		..\win\gnome\gnmain.h
	$(CC) $(CFLAGSU) $(GNOMEINC) -c ..\win\gnome\gnsignal.c
$(O)gnstatus.o: ..\win\gnome\gnstatus.c ..\win\gnome\gnstatus.h \
		..\win\gnome\gnsignal.h ..\win\gnome\gn_xpms.h \
		..\win\gnome\gnomeprv.h
	$(CC) $(CFLAGSU) $(GNOMEINC) -c ..\win\gnome\gnstatus.c
$(O)gntext.o: ..\win\gnome\gntext.c ..\win\gnome\gntext.h ..\win\gnome\gnmain.h \
		..\win\gnome\gn_rip.h
	$(CC) $(CFLAGSU) $(GNOMEINC) -c ..\win\gnome\gntext.c
$(O)gnyesno.o: ..\win\gnome\gnyesno.c ..\win\gnome\gnbind.h ..\win\gnome\gnyesno.h
	$(CC) $(CFLAGSU) $(GNOMEINC) -c ..\win\gnome\gnyesno.c
$(O)wingem.o: ..\win\gem\wingem.c $(HACK_H) $(INCL)\func_tab.h $(INCL)\dlb.h \
		$(INCL)\patchlevel.h $(INCL)\wingem.h
	$(CC) $(CFLAGSU) -Fo$@ ..\win\gem\wingem.c
$(O)wingem1.o: ..\win\gem\wingem1.c $(INCL)\gem_rsc.h $(INCL)\load_img.h \
		$(INCL)\wintype.h $(INCL)\wingem.h
	$(CC) $(CFLAGSU) -Fo$@ ..\win\gem\wingem1.c
$(O)load_img.o: ..\win\gem\load_img.c $(INCL)\load_img.h
	$(CC) $(CFLAGSU) -Fo$@ ..\win\gem\load_img.c
$(O)tile.o: $(SRC)\tile.c $(HACK_H)
$(O)qt_win.o: ..\win\Qt\qt_win.cpp $(HACK_H) $(INCL)\func_tab.h \
		$(INCL)\dlb.h $(INCL)\patchlevel.h $(INCL)\qt_win.h \
		$(INCL)\qt_clust.h $(INCL)\qt_kde0.h \
		$(INCL)\qt_xpms.h qt_win.moc qt_kde0.moc
	$(CXX) $(CXXFLAGS) -c ..\win\Qt\qt_win.cpp
$(O)qt_clust.o: ..\win\Qt\qt_clust.cpp $(INCL)\qt_clust.h
	$(CXX) $(CXXFLAGS) -c ..\win\Qt\qt_clust.cpp
$(O)monstr.o: $(SRC)\monstr.c $(CONFIG_H)
$(O)vis_tab.o: $(SRC)\vis_tab.c $(CONFIG_H) $(INCL)\vis_tab.h
$(O)allmain.o: $(SRC)\allmain.c $(HACK_H)
$(O)alloc.o: $(SRC)\alloc.c $(CONFIG_H)
$(O)apply.o: $(SRC)\apply.c $(HACK_H)
$(O)artifact.o: $(SRC)\artifact.c $(HACK_H) $(INCL)\artifact.h $(INCL)\artilist.h
$(O)attrib.o: $(SRC)\attrib.c $(HACK_H) $(INCL)\artifact.h
$(O)ball.o: $(SRC)\ball.c $(HACK_H)
$(O)bones.o: $(SRC)\bones.c $(HACK_H) $(INCL)\lev.h
$(O)botl.o: $(SRC)\botl.c $(HACK_H)
$(O)cmd.o: $(SRC)\cmd.c $(HACK_H) $(INCL)\func_tab.h
$(O)dbridge.o: $(SRC)\dbridge.c $(HACK_H)
$(O)decl.o: $(SRC)\decl.c $(HACK_H)
$(O)detect.o: $(SRC)\detect.c $(HACK_H) $(INCL)\artifact.h
$(O)dig.o: $(SRC)\dig.c $(HACK_H)
$(O)display.o: $(SRC)\display.c $(HACK_H)
$(O)dlb.o: $(SRC)\dlb.c $(CONFIG_H) $(INCL)\dlb.h
$(O)do.o: $(SRC)\do.c $(HACK_H) $(INCL)\lev.h
$(O)do_name.o: $(SRC)\do_name.c $(HACK_H)
$(O)do_wear.o: $(SRC)\do_wear.c $(HACK_H)
$(O)dog.o: $(SRC)\dog.c $(HACK_H)
$(O)dogmove.o: $(SRC)\dogmove.c $(HACK_H) $(INCL)\mfndpos.h
$(O)dokick.o: $(SRC)\dokick.c $(HACK_H)
$(O)dothrow.o: $(SRC)\dothrow.c $(HACK_H)
$(O)drawing.o: $(SRC)\drawing.c $(HACK_H) $(INCL)\tcap.h
$(O)dungeon.o: $(SRC)\dungeon.c $(HACK_H) $(INCL)\dgn_file.h $(INCL)\dlb.h
$(O)eat.o: $(SRC)\eat.c $(HACK_H)
$(O)end.o: $(SRC)\end.c $(HACK_H) $(INCL)\lev.h $(INCL)\dlb.h
$(O)engrave.o: $(SRC)\engrave.c $(HACK_H) $(INCL)\lev.h
$(O)exper.o: $(SRC)\exper.c $(HACK_H)
$(O)explode.o: $(SRC)\explode.c $(HACK_H)
$(O)extralev.o: $(SRC)\extralev.c $(HACK_H)
$(O)files.o: $(SRC)\files.c $(HACK_H) $(INCL)\dlb.h
$(O)fountain.o: $(SRC)\fountain.c $(HACK_H)
$(O)hack.o: $(SRC)\hack.c $(HACK_H)
$(O)hacklib.o: $(SRC)\hacklib.c $(HACK_H)
$(O)invent.o: $(SRC)\invent.c $(HACK_H) $(INCL)\artifact.h
$(O)light.o: $(SRC)\light.c $(HACK_H) $(INCL)\lev.h
$(O)lock.o: $(SRC)\lock.c $(HACK_H)
$(O)mail.o: $(SRC)\mail.c $(HACK_H) $(INCL)\mail.h
$(O)makemon.o: $(SRC)\makemon.c $(HACK_H)
$(O)mapglyph.o: $(SRC)\mapglyph.c $(HACK_H)
$(O)mcastu.o: $(SRC)\mcastu.c $(HACK_H)
$(O)mhitm.o: $(SRC)\mhitm.c $(HACK_H) $(INCL)\artifact.h
$(O)mhitu.o: $(SRC)\mhitu.c $(HACK_H) $(INCL)\artifact.h
$(O)minion.o: $(SRC)\minion.c $(HACK_H)
$(O)mklev.o: $(SRC)\mklev.c $(HACK_H)
$(O)mkmap.o: $(SRC)\mkmap.c $(HACK_H) $(INCL)\sp_lev.h
$(O)mkmaze.o: $(SRC)\mkmaze.c $(HACK_H) $(INCL)\sp_lev.h $(INCL)\lev.h
$(O)mkobj.o: $(SRC)\mkobj.c $(HACK_H) $(INCL)\artifact.h
$(O)mkroom.o: $(SRC)\mkroom.c $(HACK_H)
$(O)mon.o: $(SRC)\mon.c $(HACK_H) $(INCL)\mfndpos.h
$(O)mondata.o: $(SRC)\mondata.c $(HACK_H)
$(O)monmove.o: $(SRC)\monmove.c $(HACK_H) $(INCL)\mfndpos.h $(INCL)\artifact.h
$(O)monst.o: $(SRC)\monst.c $(CONFIG_H) $(INCL)\permonst.h $(INCL)\align.h \
		$(INCL)\monattk.h $(INCL)\monflag.h $(INCL)\monsym.h \
		$(INCL)\dungeon.h $(INCL)\color.h
$(O)mplayer.o: $(SRC)\mplayer.c $(HACK_H)
$(O)mthrowu.o: $(SRC)\mthrowu.c $(HACK_H)
$(O)muse.o: $(SRC)\muse.c $(HACK_H)
$(O)music.o: $(SRC)\music.c $(HACK_H) #interp.c
$(O)o_init.o: $(SRC)\o_init.c $(HACK_H) $(INCL)\lev.h
$(O)objects.o: $(SRC)\objects.c $(CONFIG_H) $(INCL)\obj.h $(INCL)\objclass.h \
		$(INCL)\prop.h $(INCL)\skills.h $(INCL)\color.h
$(O)objnam.o: $(SRC)\objnam.c $(HACK_H)
$(O)options.o: $(SRC)\options.c $(CONFIG_H) $(INCL)\objclass.h $(INCL)\flag.h \
		$(HACK_H) $(INCL)\tcap.h
$(O)pager.o: $(SRC)\pager.c $(HACK_H) $(INCL)\dlb.h
$(O)pickup.o: $(SRC)\pickup.c $(HACK_H)
$(O)pline.o: $(SRC)\pline.c $(HACK_H)
$(O)polyself.o: $(SRC)\polyself.c $(HACK_H)
$(O)potion.o: $(SRC)\potion.c $(HACK_H)
$(O)pray.o: $(SRC)\pray.c $(HACK_H)
$(O)priest.o: $(SRC)\priest.c $(HACK_H) $(INCL)\mfndpos.h
$(O)quest.o: $(SRC)\quest.c $(HACK_H) $(INCL)\qtext.h
$(O)questpgr.o: $(SRC)\questpgr.c $(HACK_H) $(INCL)\dlb.h $(INCL)\qtext.h
$(O)read.o: $(SRC)\read.c $(HACK_H)
$(O)rect.o: $(SRC)\rect.c $(HACK_H)
$(O)region.o: $(SRC)\region.c $(HACK_H) $(INCL)\lev.h
$(O)restore.o: $(SRC)\restore.c $(HACK_H) $(INCL)\lev.h $(INCL)\tcap.h
$(O)rip.o: $(SRC)\rip.c $(HACK_H)
$(O)rnd.o: $(SRC)\rnd.c $(HACK_H)
$(O)role.o: $(SRC)\role.c $(HACK_H)
$(O)rumors.o: $(SRC)\rumors.c $(HACK_H) $(INCL)\lev.h $(INCL)\dlb.h
$(O)save.o: $(SRC)\save.c $(HACK_H) $(INCL)\lev.h
$(O)shk.o: $(SRC)\shk.c $(HACK_H)
$(O)shknam.o: $(SRC)\shknam.c $(HACK_H)
$(O)sit.o: $(SRC)\sit.c $(HACK_H) $(INCL)\artifact.h
$(O)sounds.o: $(SRC)\sounds.c $(HACK_H)
$(O)sp_lev.o: $(SRC)\sp_lev.c $(HACK_H) $(INCL)\dlb.h $(INCL)\sp_lev.h
$(O)spell.o: $(SRC)\spell.c $(HACK_H)
$(O)steal.o: $(SRC)\steal.c $(HACK_H)
$(O)steed.o: $(SRC)\steed.c $(HACK_H)
$(O)teleport.o: $(SRC)\teleport.c $(HACK_H)
$(O)timeout.o: $(SRC)\timeout.c $(HACK_H) $(INCL)\lev.h
$(O)topten.o: $(SRC)\topten.c $(HACK_H) $(INCL)\dlb.h $(INCL)\patchlevel.h
$(O)track.o: $(SRC)\track.c $(HACK_H)
$(O)trap.o: $(SRC)\trap.c $(HACK_H)
$(O)u_init.o: $(SRC)\u_init.c $(HACK_H)
$(O)uhitm.o: $(SRC)\uhitm.c $(HACK_H)
$(O)vault.o: $(SRC)\vault.c $(HACK_H)
$(O)version.o: $(SRC)\version.c $(HACK_H) $(INCL)\date.h $(INCL)\patchlevel.h
$(O)vision.o: $(SRC)\vision.c $(HACK_H) $(INCL)\vis_tab.h
$(O)weapon.o: $(SRC)\weapon.c $(HACK_H)
$(O)were.o: $(SRC)\were.c $(HACK_H)
$(O)wield.o: $(SRC)\wield.c $(HACK_H)
$(O)windows.o: $(SRC)\windows.c $(HACK_H) $(INCL)\wingem.h $(INCL)\winGnome.h
$(O)wizard.o: $(SRC)\wizard.c $(HACK_H) $(INCL)\qtext.h
$(O)worm.o: $(SRC)\worm.c $(HACK_H) $(INCL)\lev.h
$(O)worn.o: $(SRC)\worn.c $(HACK_H)
$(O)write.o: $(SRC)\write.c $(HACK_H)
$(O)zap.o: $(SRC)\zap.c $(HACK_H)

# end of file

