#	SCCS Id :  @(#)Makefile.SC	      3.3     96/10/14
#	Copyright (c) NetHack Development Team 1996.
#
#	Symantec C compiler V7.2
#	Written for Symantec SMAKE utility
#
#	NOTE: This Makefile has not been tested for NetHack 3.3.0
#
#	For questions or comments :  nethack-bugs@linc.cis.upenn.edu
#
#	In addition to your C compiler,
#
#  if you want to change   you will need a
#  files with suffix   workalike for
#	  .y	     yacc
#	  .l	     lex
#

# Game Installation Variables
# NOTE :  Make sure GAMEDIR exists before make is started.

GAME = nethack
GAMEDIR = c:\games\nethacks

#
# Directories, gnu utilities like unix style directory specs
#

DUTIL = ../util

# But we must use dos directory specs to find src files, so....

DAT  = ..\dat
DOC  = ..\doc
INCL = ..\include
MSYS = ..\sys\msdos
SRC  = ..\src
SSHR = ..\sys\share
UTIL = ..\util
WIN  = ..\win\tty
WSHR = ..\win\share

#
#  Executables.

CC    = sc
LINK  = link
MAKEBIN  = smake

# if you have a uudecode program, add its name here
# otherwise leave blank
UUDECODE =


#
# Special libraries and how to link them in.

LIBS = 

# To build a DOS-extended executable uncomment the top line.
# To build a WIN32 console executable uncomment the second line.

TARGENV=DOS_EXTENDED
#TARGENV=WIN32_CONSOLE

#
#  Yacc/Lex ... if you got 'em.
#
# If you have yacc/lex or a work-alike set YACC_LEX to Y
#
YACC_LEX = N

# If YACC_LEX is Y above, set the following to values appropriate for
# your tools.
#
YACC   = bison -y
LEX    = flex
YTABC  = y_tab.c
YTABH  = y_tab.h
LEXYYC = lexyy.c

#
# Uncomment the line below if you want to store all the level files,
# help files, etc. in a single library file.

USE_DLB = N

#############################################################################
#
# nothing below this line should have to be changed
#

GAMEFILE = $(GAMEDIR)\$(GAME).exe

LIBRARIES = $(LIBS) $(TERMLIB)

# Changing this conditional block is not recommended
!IF "$(USE_DLB)"=="Y"
DLBFLG = -DDLB
!ELSE
DLBFLG =
!ENDIF

#######################################
!IF "$(TARGENV)"=="DOS_EXTENDED"

# DOS Extended Executable
#  Flags.
#
#   Debugging
CFLAGS = -c -Nc -w- -w2 -w3 -mx -g -I../include $(DLBFLG)
LFLAGS = /NOI /XREF /DETAILED /DEBUG cx
#   Normal
#CFLAGS = -c -Nc -w- -w2 -w3 -mx -I../include $(DLBFLG)
#LFLAGS = /NOI /MAP cx

!ELSE

# WIN32 Console Executable
#  Flags.
#
#   Debugging
#CFLAGS = -c -o -Nc -w- -w2 -w3 -mx -g -gt -I../include $(DLBFLG)
#LFLAGS = /NOI /XREF /DETAILED /DEBUG cx

#   Normal
CFLAGS = -c -o -Nc -w- -w2 -w3 -mx -gt -I../include $(DLBFLG)
LFLAGS = /NOI /MAP cx
!ENDIF

#
#  Utility Objects.
#

MAKEOBJS    = makedefs.o  monst.o	 objects.o

SPLEVOBJS   = lev_yacc.o  lev_$(LEX).o	lev_main.o  alloc.o	\
		monst.o	    objects.o	    panic.o  \
		drawing.o	decl.o

DGNCOMPOBJS = dgn_yacc.o  dgn_$(LEX).o	dgn_main.o  alloc.o	 \
		panic.o

RECOVOBJS   = recover.o

#  Tile related object files.

TILOBJ      = tile.o pctiles.o vidvga.o

TEXTIO      = tiletext.o tiletxt.o drawing.o decl.o monst.o objects.o

TEXTIO2     = tiletex2.o tiletxt2.o drawing.o decl.o monst.o objects.o

PLANAR_TIB  = NetHack1.tib

OVERVIEW_TIB = NetHacko.tib

#TILEUTIL    = $(TILOBJ) $(UTIL)\tile2bin.exe $(UTIL)\til2bin2.exe \
#		$(PLANAR_TIB) $(OVERVIEW_TIB)

TILEUTIL =

TILEFILES   = $(WSHR)\monsters.txt $(WSHR)\objects.txt $(WSHR)\other.txt

TILEFILES2  = $(WSHR)\monthin.txt $(WSHR)\objthin.txt $(WSHR)\oththin.txt

GIFREADERS  = gifread.o alloc.o panic.o

GIFREAD2    = gifread2.o alloc.o panic.o

GIFWRITERS  = gifwrite.o alloc.o panic.o

GIFWRIT2    = gifwrit2.o alloc.o panic.o

DLBOBJ = dlb.o

#  Object files for the game itself.

VOBJ01 = allmain.o  alloc.o    apply.o	  artifact.o attrib.o
VOBJ02 = ball.o	    bones.o    botl.o	  cmd.o	     dbridge.o
VOBJ03 = decl.o	    detect.o   display.o  do.o	     do_name.o
VOBJ04 = do_wear.o  dog.o      dogmove.o  dokick.o   dothrow.o
VOBJ05 = drawing.o  dungeon.o  eat.o	  end.o	     engrave.o
VOBJ06 = exper.o    explode.o  extralev.o files.o    fountain.o
VOBJ07 = getline.o  hack.o     hacklib.o  invent.o   lock.o
VOBJ08 = mail.o	    main.o     makemon.o  mcastu.o   mhitm.o
VOBJ09 = mhitu.o    minion.o   mkmap.o	  mklev.o    mkmaze.o
VOBJ10 = mkobj.o    mkroom.o   mon.o	  mondata.o  monmove.o
VOBJ11 = monst.o    monstr.o   mplayer.o  mthrowu.o  muse.o
VOBJ12 = music.o    o_init.o   objects.o  objnam.o   options.o
VOBJ13 = pickup.o   pline.o    polyself.o potion.o   quest.o
VOBJ14 = questpgr.o pager.o    pray.o	  priest.o   read.o
VOBJ15 = rect.o	    restore.o  rip.o	  rnd.o	     role.o
VOBJ16 = rumors.o   save.o	   shk.o      shknam.o	 sit.o
VOBJ17 = sounds.o   sp_lev.o   spell.o    steal.o	 steed.o
VOBJ18 = termcap.o  timeout.o  topl.o	  topten.o   track.o
VOBJ19 = trap.o     u_init.o   uhitm.o    vault.o    vision.o
VOBJ20 = vis_tab.o  weapon.o   were.o	  wield.o    windows.o
VOBJ21 = wintty.o   wizard.o   worm.o	  worn.o     write.o
VOBJ22 = zap.o	    light.o    dlb.o      dig.o      teleport.o
VOBJ23 = random.o

SOBJ   = msdos.o    sound.o    sys.o	  tty.o	     unix.o    video.o \
	vidtxt.o    pckeys.o

VVOBJ  = version.o

VOBJ   = $(VOBJ01) $(VOBJ02) $(VOBJ03) $(VOBJ04) $(VOBJ05) \
	$(VOBJ06) $(VOBJ07) $(VOBJ08) $(VOBJ09) $(VOBJ10) \
	$(VOBJ11) $(VOBJ12) $(VOBJ13) $(VOBJ14) $(VOBJ15) \
	$(VOBJ16) $(VOBJ17) $(VOBJ18) $(VOBJ19) $(VOBJ20) \
	$(VOBJ21) $(VOBJ22) $(VOBJ23)

#ALLOBJ = $(VOBJ) $(SOBJ) $(TILOBJ) $(VVOBJ)
ALLOBJ = $(VOBJ) $(SOBJ) $(VVOBJ)

#
#  Header Objects.
#

DGN_FILE_H = $(INCL)\align.h	$(INCL)\dgn_file.h
DUNGEON_H  = $(INCL)\align.h	$(INCL)\dungeon.h
EMIN_H	   = $(DUNGEON_H)	$(INCL)\emin.h
EPRI_H	   = $(DUNGEON_H)	$(INCL)\align.h	    $(INCL)\epri.h
ESHK_H	   = $(DUNGEON_H)	$(INCL)\eshk.h
MONDATA_H  = $(INCL)\align.h	$(INCL)\mondata.h
MONST_H	   = $(INCL)\align.h	$(INCL)\monst.h
PERMONST_H = $(INCL)\monattk.h	$(INCL)\monflag.h   $(INCL)\align.h   \
	    $(INCL)\permonst.h
RM_H	   = $(INCL)\align.h	$(INCL)\rm.h
SP_LEV_H   = $(INCL)\align.h	$(INCL)\sp_lev.h
VAULT_H	   = $(DUNGEON_H)	$(INCL)\vault.h
YOUPROP_H  = $(PERMONST_H)	$(MONDATA_H)	    $(INCL)\prop.h    \
	    $(INCL)\pm.h       $(INCL)\youprop.h
YOU_H	   = $(MONST_H)		$(YOUPROP_H)	    $(INCL)\align.h   \
	     $(INCL)\attrib.h	$(INCL)\you.h
DISPLAY_H  = $(MONDATA_H)	$(INCL)\vision.h    $(INCL)\display.h
PCCONF_H   = $(INCL)\micro.h	$(INCL)\system.h    $(INCL)\pcconf.h
CONFIG_H   = $(GLOBAL_H)	$(INCL)\tradstdc.h  $(INCL)\config1.h \
	    $(INCL)\config.h
DECL_H	   = $(YOU_H)		$(INCL)\spell.h	    $(INCL)\color.h   \
	     $(INCL)\obj.h	$(INCL)\onames.h    $(INCL)\pm.h      \
	      $(INCL)\decl.h
GLOBAL_H   = $(PCCONF_H)	$(INCL)\coord.h	    $(INCL)\global.h
HACK_H	   = $(CONFIG_H)	$(DUNGEON_H)	    $(DECL_H)	      \
	       $(DISPLAY_H)	  $(INCL)\monsym.h    $(INCL)\mkroom.h	\
	      $(INCL)\objclass.h $(INCL)\trap.h	     $(INCL)\flag.h    \
	      $(RM_H)		 $(INCL)\vision.h    $(INCL)\wintype.h \
	      $(INCL)\engrave.h	 $(INCL)\rect.h	     \
	       $(INCL)\trampoli.h $(INCL)\hack.h
DLB_H      = $(INCL)\dlb.h
TILE_H	   = $(INCL)\tile.h $(INCL)\pctiles.h

!IF "$(USE_DLB)"=="Y"
DLB = dlb
DLBOBJS = dlb_main.o dlb.o alloc.o panic.o
!ELSE
DLB =
DLBOBJS =
!ENDIF

#
#  Make Rules.
#

.SUFFIXES :   .exe .o .c .y .l

.c.o : 
	$(CC) $(CFLAGS) -o$@ $<

#
#  Primary Targets.
#

#  The default target.

default :  $(GAMEFILE)

all :  install.tag

util :  utility.tag

install :  install.tag

utility.tag :  $(INCL)\date.h $(INCL)\trap.h    $(INCL)\onames.h	       \
	$(INCL)\pm.h	  monstr.c   vis_tab.c	\
	$(UTIL)\lev_comp.exe	 $(UTIL)\dgn_comp.exe	 \
	$(UTIL)\recover.exe $(TILEUTIL)
	echo utilities made > utility.tag

gifutil :  $(UTIL)\gif2txt.exe $(UTIL)\txt2gif.exe
	echo Optional GIF development utilities are up to date.

install.tag :  dat.tag $(GAMEFILE)
!IF "$(USE_DLB)"=="Y"
	copy $(DAT)\nhdat         $(GAMEDIR)
	copy $(DAT)\license       $(GAMEDIR)
!ELSE
	copy $(DAT)\*.            $(GAMEDIR)
	copy $(DAT)\*.dat         $(GAMEDIR)
	copy $(DAT)\*.lev         $(GAMEDIR)
	copy $(MSYS)\msdoshlp.txt $(GAMEDIR)
	if exist $(GAMEDIR)\makefile. del $(GAMEDIR)\makefile.
!ENDIF
	copy $(SSHR)\termcap	  $(GAMEDIR)
	copy *.tib                $(GAMEDIR)
	copy $(SSHR)\NetHack.cnf  $(GAMEDIR)\defaults.nh
	copy $(UTIL)\recover.exe  $(GAMEDIR)
	copy $(DOC)\guideb*.txt   $(GAMEDIR)
	echo install done > install.tag

#  The main target.

$(GAMEFILE) :  utility.tag $(ALLOBJ)
	$(LINK) /STACK:100000 $(LFLAGS) @<<$(GAME).lnk
		$(ALLOBJ:^	=+^
		)
		$(GAMEFILE)
		$(GAME);
<<KEEP

#
#  Housekeeping.
#

clean : 
	del *.o
	del *.map
	del $(UTIL)\dlb_main.exe
	if exist utility.tag	del utility.tag
	if exist install.tag	del install.tag
	if exist dat.tag	del dat.tag
spotless :  clean
	if exist $(GAME).lnk	   del $(GAME).lnk
	if exist $(UTIL)\makedefs.exe	   del $(UTIL)\makedefs.exe
	if exist $(UTIL)\lev_comp.exe	   del $(UTIL)\lev_comp.exe
	if exist $(UTIL)\dgn_comp.exe	   del $(UTIL)\dgn_comp.exe
	if exist $(UTIL)\recover.exe	   del $(UTIL)\recover.exe
	if exist $(INCL)\onames.h  del $(INCL)\onames.h
	if exist $(INCL)\pm.h	   del $(INCL)\pm.h
	if exist $(INCL)\vis_tab.h del $(INCL)\vis_tab.h
	if exist $(INCL)\pcvideo.h del $(INCL)\pcvideo.h
	if exist $(INCL)\pctiles.h del $(INCL)\pctiles.h
	if exist $(INCL)\portio.h  del $(INCL)\portio.h
	if exist $(INCL)\tile.h	   del $(INCL)\tile.h
	if exist $(SRC)\monstr.c   del $(SRC)\monstr.c
	if exist $(SRC)\vis_tab.c  del $(SRC)\vis_tab.c
	if exist $(SRC)\tile.c	   del $(SRC)\tile.c
	if exist $(INCL)\date.h	   del $(INCL)\date.h
	if exist $(INCL)\onames.h  del $(INCL)\onames.h
	if exist $(INCL)\pm.h	   del $(INCL)\pm.h
	if exist $(INCL)\vis_tab.h del $(INCL)\vis_tab.h
	if exist *.lnk		   del *.lnk
	if exist *.map		   del *.map
	if exist $(UTIL)\tilemap.exe       del $(UTIL)\tilemap.exe
	if exist $(UTIL)\tile2bin.exe      del $(UTIL)\tile2bin.exe
	if exist $(UTIL)\til2bin2.exe      del $(UTIL)\til2bin2.exe
	if exist $(UTIL)\viewtib.exe       del $(UTIL)\viewtib.exe
	if exist $(DAT)\data	   del $(DAT)\data
	if exist $(DAT)\*.lev      del $(DAT)\*.lev
	if exist $(DAT)\data	   del $(DAT)\data
	if exist $(DAT)\dungeon	   del $(DAT)\dungeon
	if exist $(DAT)\options	   del $(DAT)\options
	if exist $(DAT)\oracles	   del $(DAT)\oracles
	if exist $(DAT)\rumors	   del $(DAT)\rumors
	if exist $(DAT)\quest.dat  del $(DAT)\quest.dat
	if exist $(DAT)\nhdat      del $(DAT)\nhdat
	if exist $(DAT)\dlb.lst    del $(DAT)\dlb.lst
	if exist sp_lev.tag	   del sp_lev.tag
	if exist $(PLANAR_TIB)	   del $(PLANAR_TIB)
	if exist $(OVERVIEW_TIB)   del $(OVERVIEW_TIB)

#
#  Secondary Targets.
#
#  The following include files depend on makedefs to be created.
#
#  date.h should be remade every time any of the source or include
#  files is modified.


$(INCL)\date.h :  $(UTIL)\makedefs.exe
	$(UTIL)\makedefs -v

$(INCL)\onames.h :  $(UTIL)\makedefs.exe
	$(UTIL)\makedefs -o

$(INCL)\pm.h :  $(UTIL)\makedefs.exe
	$(UTIL)\makedefs -p

monstr.c :  $(UTIL)\makedefs.exe
	$(UTIL)\makedefs -m

$(INCL)\vis_tab.h :  $(UTIL)\makedefs.exe
	$(UTIL)\makedefs -z

vis_tab.c :  $(UTIL)\makedefs.exe
	$(UTIL)\makedefs -z

#
#  Makedefs Stuff
#

$(UTIL)\makedefs.exe :   $(MAKEOBJS)
	$(LINK) $(LFLAGS) $(MAKEOBJS),$@

makedefs.o :   $(CONFIG_H)	$(PERMONST_H)	   $(INCL)\objclass.h \
		$(INCL)\monsym.h   $(INCL)\qtext.h $(UTIL)\makedefs.c
	$(CC) $(CFLAGS) -o$@ $(UTIL)\makedefs.c

#
#  Level Compiler Dependencies
#

$(UTIL)\lev_comp.exe :   $(SPLEVOBJS)
	$(LINK) $(LFLAGS) $(SPLEVOBJS),$@

!IF "$(YACC_LEX)"=="Y"

lev_yacc.o :   $(HACK_H)	 $(SP_LEV_H) $(UTIL)\lev_yacc.c
	$(CC) $(CFLAGS) -o$@ $(UTIL)\lev_yacc.c

lev_$(LEX).o :   $(HACK_H)   $(SP_LEV_H)	  $(INCL)\lev_comp.h \
	lev_$(LEX).c
	$(CC) $(CFLAGS) -o$@ lev_$(LEX).c

$(INCL)\lev_comp.h :  $(UTIL)\lev_yacc.c

$(UTIL)\lev_yacc.c :   $(UTIL)\lev_comp.y
	$(YACC) -d $(DUTIL)/lev_comp.y
	copy $(YTABC) $@
	copy $(YTABH) $(INCL)\lev_comp.h
	del $(YTABC)
	del $(YTABH)

$(UTIL)\lev_$(LEX).c :   $(UTIL)\lev_comp.l
	$(LEX) $(DUTIL)/lev_comp.l
	copy $(LEXYYC) $@
	del $(LEXYYC)

!ELSE

lev_yacc.o :   $(HACK_H)	 $(SP_LEV_H) $(INCL)\lev_comp.h $(SSHR)\lev_yacc.c
	$(CC) $(CFLAGS) -o$@ $(SSHR)\lev_yacc.c

lev_$(LEX).o :   $(HACK_H)   $(SP_LEV_H) $(INCL)\lev_comp.h $(SSHR)\lev_lex.c
	$(CC) $(CFLAGS) -o$@ $(SSHR)\lev_lex.c

$(INCL)\lev_comp.h :  $(SSHR)\lev_comp.h
	copy $(SSHR)\lev_comp.h $@

$(UTIL)\lev_$(LEX).c :  $(SSHR)\lev_lex.c
	copy $(SSHR)\lev_lex.c $@

$(UTIL)\lev_yacc.c : $(SSHR)\lev_yacc.c
	copy $(SSHR)\lev_yacc.c $@

!ENDIF

lev_main.o :   $(HACK_H) $(UTIL)\lev_main.c
	$(CC) $(CFLAGS) -o$@ $(UTIL)\lev_main.c

#
#  Dungeon Dependencies
#

$(UTIL)\dgn_comp.exe :   $(DGNCOMPOBJS)
	$(LINK) $(LFLAGS) $(DGNCOMPOBJS),$@

!IF "$(YACC_LEX)"=="Y"

dgn_yacc.o :   $(HACK_H)	 $(DGN_FILE_H)	$(UTIL)\dgn_yacc.c
	$(CC) $(CFLAGS) -o$@ $(UTIL)\dgn_yacc.c

$(UTIL)\dgn_yacc.c :  $(UTIL)\dgn_comp.y
	$(YACC) -d $(DUTIL)/dgn_comp.y
	copy $(YTABC) $@
	copy $(YTABH) $(INCL)\dgn_comp.h
	del $(YTABC)
	del $(YTABH)

$(UTIL)\dgn_$(LEX).c :   $(UTIL)\dgn_comp.l
	$(LEX) $(DUTIL)/dgn_comp.l
	copy $(LEXYYC) $@
	del $(LEXYYC)

dgn_$(LEX).o : $(HACK_H) $(DGN_FILE_H) $(INCL)\dgn_comp.h $(UTIL)\dgn_$(LEX).c
	$(CC) $(CFLAGS) -o$@ $(UTIL)\dgn_$(LEX).c

$(INCL)\dgn_comp.h :   $(UTIL)\dgn_yacc.c

!ELSE

dgn_yacc.o : $(HACK_H)	 $(DGN_FILE_H)	$(INCL)\dgn_comp.h $(SSHR)\dgn_yacc.c
	$(CC) $(CFLAGS) -I$(SSHR) -o$@ $(SSHR)\dgn_yacc.c

dgn_$(LEX).o : $(HACK_H) $(DGN_FILE_H) $(SSHR)\dgn_comp.h $(SSHR)\dgn_lex.c
	$(CC) $(CFLAGS) -o$@ $(SSHR)\dgn_lex.c

$(INCL)\dgn_comp.h :   $(SSHR)\dgn_comp.h
	copy $(SSHR)\dgn_comp.h $@

!ENDIF

dgn_main.o :   $(HACK_H) $(UTIL)\dgn_main.c
	$(CC) $(CFLAGS) -o$@ $(UTIL)\dgn_main.c

#
#  Recover Utility
#

$(UTIL)\recover.exe :    $(RECOVOBJS)
	$(LINK) $(LFLAGS) recover.o,$@

recover.o :    $(CONFIG_H) $(UTIL)\recover.c
	$(CC) $(CFLAGS) -o$@ $(UTIL)\recover.c


#
#  Header file moves required for tile support
#

$(INCL)\tile.h :  $(WSHR)\tile.h
	copy $(WSHR)\tile.h $@

$(INCL)\pctiles.h :  $(MSYS)\pctiles.h
	copy $(MSYS)\pctiles.h $@

$(INCL)\pcvideo.h :  $(MSYS)\pcvideo.h
	copy $(MSYS)\pcvideo.h $@

$(INCL)\portio.h :  $(MSYS)\portio.h
	copy $(MSYS)\portio.h $@

#
#  Tile Mapping
#
tile.o :  $(HACK_H) $(TILE_H) tile.c
	$(CC) $(CFLAGS) -o$@ tile.c

tile.c :  $(UTIL)\tilemap.exe
	$(UTIL)\tilemap
	@echo A new $@ has been created

$(UTIL)\tilemap.exe :  tilemap.o
	$(LINK) $(LFLAGS) tilemap.o,$@

tilemap.o :  $(WSHR)\tilemap.c $(HACK_H) $(TILE_H)
	$(CC) $(CFLAGS) -o$@ $(WSHR)\tilemap.c

#
#   Tile Utilities
#
#
#  Required for tile support
#

NetHack1.tib :  $(TILEFILES) $(UTIL)\tile2bin.exe
	@echo Creating binary tile files (this may take some time)
	$(UTIL)\tile2bin

NetHacko.tib :  thintile.tag $(TILEFILES2) $(UTIL)\til2bin2.exe
	@echo Creating overview binary tile files (this may take some time)
	$(UTIL)\til2bin2

$(UTIL)\tile2bin.exe :  tile2bin.o $(TEXTIO)
	$(LINK) $(LFLAGS) tile2bin.o $(TEXTIO),$@

$(UTIL)\til2bin2.exe :  til2bin2.o $(TEXTIO2)
	$(LINK) $(LFLAGS) til2bin2.o $(TEXTIO2),$@

$(UTIL)\thintile.exe :  thintile.o 
	$(LINK) $(LFLAGS) thintile.o,$@

thintile.o :   $(HACK_H) $(INCL)\tile.h $(WSHR)\thintile.c
	$(CC) $(CFLAGS) -o$@ $(WSHR)\thintile.c

thintile.tag :  $(UTIL)\thintile.exe $(TILEFILES)
	$(UTIL)\thintile
	@echo thintiles created >thintile.tag

tile2bin.o :   $(HACK_H) $(INCL)\tile.h $(INCL)\pctiles.h $(INCL)\pcvideo.h \
	$(MSYS)\tile2bin.c
	$(CC) $(CFLAGS) -o$@ $(MSYS)\tile2bin.c

til2bin2.o :   $(HACK_H) $(INCL)\tile.h $(INCL)\pctiles.h $(INCL)\pcvideo.h \
	$(MSYS)\tile2bin.c
	$(CC) $(CFLAGS) -DTILE_X=8 -DOVERVIEW_FILE -o$@ $(MSYS)\tile2bin.c

tiletext.o :   $(CONFIG_H) $(INCL)\tile.h $(WSHR)\tiletext.c
	$(CC) $(CFLAGS) -o$@ $(WSHR)\tiletext.c

tiletex2.o :   $(CONFIG_H) $(INCL)\tile.h $(WSHR)\tiletext.c
	$(CC) $(CFLAGS) -DTILE_X=8 -o$@ $(WSHR)\tiletext.c

tiletxt.o :  $(CONFIG_H)	$(INCL)\tile.h $(WSHR)\tilemap.c
	$(CC) $(CFLAGS) -DTILETEXT -o$@ $(WSHR)\tilemap.c
  
tiletxt2.o :  $(CONFIG_H)	$(INCL)\tile.h $(WSHR)\tilemap.c
	$(CC) $(CFLAGS) -DTILETEXT -DTILE_X=8 -o$@ $(WSHR)\tilemap.c
#
# Optional GIF Utilities (for development)
#

$(UTIL)\gif2txt.exe :  $(GIFREADERS) $(TEXTIO)
	$(LINK) $(LFLAGS) $(GIFREADERS) $(TEXTIO)

$(UTIL)\gif2txt2.exe :  $(GIFREAD2) $(TEXTIO2)
	$(LINK) $(LFLAGS) $(GIFREAD2) $(TEXTIO2)

$(UTIL)\txt2gif.exe :  $(GIFWRITERS) $(TEXTIO)
	$(LINK) $(LFLAGS) $(GIFWRITERS) $(TEXTIO)

$(UTIL)\txt2gif2.exe :  $(GIFWRIT2) $(TEXTIO2)
	$(LINK) $(LFLAGS) $(GIFWRIT2) $(TEXTIO2)

gifread.o :  $(CONFIG_H) $(INCL)\tile.h $(WSHR)\gifread.c
	$(CC) $(CFLAGS) -o$@ $(WSHR)\gifread.c

gifread2.o :  $(CONFIG_H) $(INCL)\tile.h gifread.c
	$(CC) $(CFLAGS) -DTILE_X=8 -o$@ $(WSHR)\gifread.c

gifwrite.o :  $(CONFIG_H)	$(INCL)\tile.h $(WSHR)\gifwrite.c
	$(CC) $(CFLAGS) -o$@ $(WSHR)\gifwrite.c

gifwrit2.o :   $(CONFIG_H) $(INCL)\tile.h $(WSHR)\gifwrite.c
	$(CC) $(CFLAGS) -DTILE_X=8 -o$@ $(WSHR)\gifwrite.c

#
#  Optional tile viewer (development sources only)
#

$(UTIL)\viewtib.exe :  viewtib.o
	$(LINK) $(LFLAGS) viewtib.o,$@

viewtib.o :  $(MSYS)\viewtib.c
	$(CC) $(CFLAGS) -o$@ $(MSYS)\viewtib.c

#
#  Other Util Dependencies.
#

alloc.o :  $(CONFIG_H)	 alloc.c
	$(CC) $(CFLAGS) -o$@ alloc.c

drawing.o :  $(CONFIG_H) drawing.c
	$(CC) $(CFLAGS) -o$@ drawing.c

decl.o :  $(CONFIG_H) decl.c
	$(CC) $(CFLAGS) -o$@ decl.c

monst.o :  $(CONFIG_H)	     $(PERMONST_H)	$(ESHK_H)	   \
	$(EPRI_H)	      $(VAULT_H)	 $(INCL)\monsym.h   \
	$(INCL)\color.h	 monst.c
	$(CC) $(CFLAGS) -o$@ monst.c

objects.o :  $(CONFIG_H)	      $(INCL)\obj.h	 $(INCL)\objclass.h \
	$(INCL)\prop.h	    $(INCL)\color.h    objects.c
	$(CC) $(CFLAGS) -o$@ objects.c

panic.o :    $(CONFIG_H)	  $(UTIL)\panic.c
	$(CC) $(CFLAGS) -o$@ $(UTIL)\panic.c




#
# make data.base an 8.3 filename to prevent an nmake warning
#

DATABASE = $(DAT)\data.bas


dat.tag :  $(DAT)\data	     $(DAT)\rumors	 $(DAT)\dungeon \
	$(DAT)\oracles	    $(DAT)\quest.dat	sp_lev.tag $(DLB)
	@echo dat done >dat.tag

$(DAT)\data : 	 utility.tag	$(DATABASE)
	$(UTIL)\makedefs -d

$(DAT)\rumors : 	     utility.tag    $(DAT)\rumors.tru	$(DAT)\rumors.fal
	$(UTIL)\makedefs -r

$(DAT)\quest.dat :  utility.tag  $(DAT)\quest.txt
	$(UTIL)\makedefs -q

$(DAT)\oracles : 	     utility.tag    $(DAT)\oracles.txt
	$(UTIL)\makedefs -h

sp_lev.tag :  utility.tag $(DAT)\bigroom.des  $(DAT)\castle.des \
	$(DAT)\endgame.des $(DAT)\gehennom.des $(DAT)\knox.des	 \
	$(DAT)\medusa.des	 $(DAT)\oracle.des   $(DAT)\tower.des  \
	$(DAT)\yendor.des	 $(DAT)\arch.des     $(DAT)\barb.des   \
	$(DAT)\caveman.des   $(DAT)\healer.des   $(DAT)\knight.des \
	$(DAT)\monk.des      $(DAT)\priest.des   $(DAT)\ranger.des \
	$(DAT)\rogue.des     $(DAT)\samurai.des  $(DAT)\tourist.des \
	$(DAT)\valkyrie.des  $(DAT)\wizard.des
	cd $(DAT)
	$(UTIL)\lev_comp bigroom.des
	$(UTIL)\lev_comp castle.des
	$(UTIL)\lev_comp endgame.des
	$(UTIL)\lev_comp gehennom.des
	$(UTIL)\lev_comp knox.des
	$(UTIL)\lev_comp mines.des
	$(UTIL)\lev_comp medusa.des
	$(UTIL)\lev_comp oracle.des
	$(UTIL)\lev_comp sokoban.des
	$(UTIL)\lev_comp tower.des
	$(UTIL)\lev_comp yendor.des
	$(UTIL)\lev_comp arch.des
	$(UTIL)\lev_comp barb.des
	$(UTIL)\lev_comp caveman.des
	$(UTIL)\lev_comp healer.des
	$(UTIL)\lev_comp knight.des
	$(UTIL)\lev_comp monk.des
	$(UTIL)\lev_comp priest.des
	$(UTIL)\lev_comp ranger.des
	$(UTIL)\lev_comp rogue.des
	$(UTIL)\lev_comp samurai.des
	$(UTIL)\lev_comp tourist.des
	$(UTIL)\lev_comp valkyrie.des
	$(UTIL)\lev_comp wizard.des
	cd $(SRC)
	echo sp_levs done > sp_lev.tag
	
$(DAT)\dungeon : 	  utility.tag  $(DAT)\dungeon.def
	$(UTIL)\makedefs -e
	cd $(DAT)
	$(UTIL)\dgn_comp dungeon.pdf
	cd $(SRC)
#
# DLB stuff
#
dlb : 	$(UTIL)\dlb_main.exe
	cd $(DAT)
	copy $(MSYS)\msdoshlp.txt .
	@echo data >dlb.lst
	@echo dungeon >>dlb.lst
	@echo oracles >>dlb.lst
	@echo options >>dlb.lst
	@echo quest.dat >>dlb.lst
	@echo rumors >>dlb.lst
	@echo help >>dlb.lst
	@echo hh >>dlb.lst
	@echo cmdhelp >>dlb.lst
	@echo history >>dlb.lst
	@echo opthelp >>dlb.lst
	@echo wizhelp >>dlb.lst
	@echo license >>dlb.lst
	@echo msdoshlp.txt >>dlb.lst
	for %%N in (*.lev) do echo %%N >>dlb.lst
	$(UTIL)\dlb_main cvIf dlb.lst nhdat
	cd $(SRC)

$(UTIL)\dlb_main.exe :  $(DLBOBJS)
	$(LINK) /STACK:65000 $(LFLAGS) $(DLBOBJS),$@

dlb_main.o :  $(UTIL)\dlb_main.c $(INCL)\config.h $(DLB_H)
	$(CC) $(CFLAGS) -odlb_main.o $(UTIL)\dlb_main.c

# Game Dependencies
# Some files require movement as Gnu make doesn't like unix style '/'
# directory specs and djgcc doesn't like dos style directory specs.
# So we get to copy stuff where we might need it.   Fun eh?

# sys/share
main.o : 	 $(HACK_H) $(DLB_H) $(SSHR)\pcmain.c
	$(CC) $(CFLAGS) -o$@ $(SSHR)\pcmain.c

tty.o : 	 $(HACK_H) $(INCL)\wintty.h $(SSHR)\pctty.c
	$(CC) $(CFLAGS) -o$@ $(SSHR)\pctty.c

unix.o : 	 $(HACK_H) $(SSHR)\pcunix.c
	$(CC) $(CFLAGS) -o$@ $(SSHR)\pcunix.c

sys.o :  $(HACK_H) $(SSHR)\pcsys.c
	$(CC) $(CFLAGS) -o$@ $(SSHR)\pcsys.c

random.o : $(HACK_H) $(SSHR)\random.c
	$(CC) $(CFLAGS) -o$@ $(SSHR)\random.c

# sys/msdos
msdos.o :  $(HACK_H)  $(MSYS)\msdos.c
	$(CC) $(CFLAGS) -o$@ $(MSYS)\msdos.c

pckeys.o :  $(HACK_H)  $(MSYS)\pckeys.c
	$(CC) $(CFLAGS) -o$@ $(MSYS)\pckeys.c

pctiles.o :  $(HACK_H)  $(MSYS)\pctiles.c $(INCL)\portio.h
	$(CC) $(CFLAGS) -o$@ $(MSYS)\pctiles.c

sound.o :  $(HACK_H)  $(MSYS)\sound.c $(INCL)\portio.h
	$(CC) $(CFLAGS) -o$@ $(MSYS)\sound.c

video.o :  $(HACK_H)  $(INCL)\pcvideo.h $(INCL)\portio.h $(MSYS)\video.c
	$(CC) $(CFLAGS) -o$@ $(MSYS)\video.c

vidvga.o :  $(HACK_H)  $(INCL)\pcvideo.h $(INCL)\portio.h $(TILE_H) \
	$(MSYS)\vidvga.c
	$(CC) $(CFLAGS) -e -o$@ $(MSYS)\vidvga.c

vidtxt.o :  $(HACK_H)  $(INCL)\pcvideo.h $(INCL)\portio.h $(TILE_H) \
	$(MSYS)\vidtxt.c
	$(CC) $(CFLAGS) -o$@ $(MSYS)\vidtxt.c



# win/tty
getline.o :  $(HACK_H) $(INCL)\wintty.h $(WIN)\getline.c
	$(CC) $(CFLAGS) -o$@ $(WIN)\getline.c

termcap.o :  $(CONFIG_H) $(WIN)\termcap.c
	$(CC) $(CFLAGS) -o$@ $(WIN)\termcap.c

topl.o :  $(CONFIG_H) $(WIN)\topl.c
	$(CC) $(CFLAGS) -o$@ $(WIN)\topl.c

wintty.o :  $(CONFIG_H) $(WIN)\wintty.c
	$(CC) $(CFLAGS) -o$@ $(WIN)\wintty.c

# src dependencies
allmain.o :   $(HACK_H)
alloc.o :     $(CONFIG_H)
apply.o :     $(HACK_H) $(INCL)\edog.h
artifact.o :  $(HACK_H) $(INCL)\artifact.h $(INCL)\artilist.h
attrib.o :    $(HACK_H) $(INCL)\artifact.h
ball.o : 	    $(HACK_H)
bones.o :     $(HACK_H) $(INCL)\lev.h
botl.o : 	    $(HACK_H)
cmd.o : 	    $(HACK_H) $(INCL)\func_tab.h
dbridge.o :   $(HACK_H)
decl.o : 	    $(HACK_H) $(INCL)\quest.h
detect.o :    $(HACK_H) $(INCL)\artifact.h
dig.o :       $(HACK_H)
display.o :   $(HACK_H)
dlb.o :       $(HACK_H) $(DLB_H)
do.o : 	    $(HACK_H) $(INCL)\lev.h
do_name.o :   $(HACK_H)
do_wear.o :   $(HACK_H)
dog.o : 	    $(HACK_H) $(INCL)\edog.h
dogmove.o :   $(HACK_H) $(INCL)\mfndpos.h $(INCL)\edog.h
dokick.o :    $(HACK_H) $(ESHK_H)
dothrow.o :   $(HACK_H)
drawing.o :   $(HACK_H) $(INCL)\tcap.h
dungeon.o :   $(HACK_H) $(INCL)\dgn_file.h
eat.o : 	    $(HACK_H)
end.o : 	    $(HACK_H) $(ESHK_H)
engrave.o :   $(HACK_H) $(INCL)\lev.h
exper.o :     $(HACK_H)
explode.o :   $(HACK_H)
extralev.o :   $(HACK_H)
files.o :      $(HACK_H)
fountain.o :   $(HACK_H)
hack.o : 	     $(HACK_H)
hacklib.o :    $(HACK_H)
invent.o :     $(HACK_H) $(INCL)\artifact.h
light.o :      $(HACK_H) $(INCL)\lev.h
lock.o : 	     $(HACK_H)
mail.o : 	     $(HACK_H) $(INCL)\mail.h
makemon.o :    $(HACK_H) $(EPRI_H) $(EMIN_H)
mcastu.o :     $(HACK_H)
mhitm.o :      $(HACK_H) $(INCL)\artifact.h $(INCL)\edog.h
mhitu.o :      $(HACK_H) $(INCL)\artifact.h $(INCL)\edog.h
minion.o :     $(HACK_H) $(EMIN_H) $(EPRI_H)
mklev.o :      $(HACK_H)
mkmap.o :      $(HACK_H) $(INCL)\sp_lev.h
mkmaze.o :     $(HACK_H) $(INCL)\sp_lev.h
mkobj.o :      $(HACK_H) $(INCL)\artifact.h $(INCL)\prop.h
mkroom.o :     $(HACK_H)
mon.o : 	     $(HACK_H) $(INCL)\mfndpos.h $(INCL)\edog.h
mondata.o :    $(HACK_H) $(ESHK_H) $(EPRI_H)
monmove.o :    $(HACK_H) $(INCL)\mfndpos.h $(INCL)\artifact.h
monst.o :      $(CONFIG_H) $(PERM_H) $(ESHK_H) $(EPRI_H) \
	  $(INCL)\color.h $(INCL)\monsym.h $(INCL)\vault.h
mplayer.o :    $(HACK_H)
mthrowu.o :    $(HACK_H)
muse.o : 	     $(HACK_H)
music.o :      $(HACK_H)
o_init.o :     $(HACK_H)
objects.o :    $(CONFIG_H) $(INCL)\obj.h $(INCL)\objclass.h \
	       $(INCL)\prop.h $(INCL)\color.h
objnam.o :     $(HACK_H)
options.o :    $(CONFIG_H) $(HACK_H) $(INCL)\objclass.h $(INCL)\flag.h \
	      $(INCL)\tcap.h
pager.o :      $(HACK_H)
pickup.o :     $(HACK_H)
pline.o :      $(HACK_H) $(EPRI_H)
polyself.o :   $(HACK_H)
potion.o :     $(HACK_H)
pray.o : 	     $(HACK_H) $(EPRI_H)
priest.o :     $(HACK_H) $(INCL)\mfndpos.h $(ESHK_H) $(EPRI_H) $(EMIN_H)
quest.o :      $(HACK_H) $(INCL)\quest.h $(INCL)\qtext.h
questpgr.o :   $(HACK_H) $(INCL)\qtext.h
read.o : 	     $(HACK_H)
region.o :     $(HACK_H)
rect.o : 	     $(HACK_H)
restore.o :    $(HACK_H) $(INCL)\lev.h $(INCL)\tcap.h $(INCL)\quest.h
rip.o : 	     $(HACK_H)
rnd.o : 	     $(HACK_H)
role.o :       $(HACK_H)
rumors.o :     $(HACK_H)
save.o : 	     $(HACK_H) $(INCL)\lev.h $(INCL)\quest.h
shk.o : 	     $(HACK_H) $(ESHK_H)
shknam.o :     $(HACK_H) $(ESHK_H)
sit.o : 	     $(HACK_H) $(INCL)\artifact.h
sounds.o :     $(HACK_H) $(INCL)\edog.h
sp_lev.o :     $(HACK_H) $(INCL)\sp_lev.h $(INCL)\align.h $(INCL)\rect.h
spell.o :      $(HACK_H)
steal.o :      $(HACK_H)
steed.o :      $(HACK_H)
teleport.o :   $(HACK_H)
timeout.o :    $(HACK_H)
topten.o :     $(HACK_H)
track.o :      $(HACK_H)
trap.o : 	     $(HACK_H)
u_init.o :     $(HACK_H)
uhitm.o :      $(HACK_H)
vault.o :      $(HACK_H) $(INCL)\vault.h
version.o :    $(HACK_H) $(INCL)\patchlev.h
vision.o :     $(HACK_H) $(INCL)\vis_tab.h
weapon.o :     $(HACK_H)
were.o : 	     $(HACK_H)
wield.o :      $(HACK_H)
windows.o :    $(HACK_H) $(INCL)\wintty.h
wizard.o :     $(HACK_H) $(INCL)\qtext.h
worm.o : 	     $(HACK_H) $(INCL)\lev.h
worn.o : 	     $(HACK_H)
write.o :      $(HACK_H)
zap.o : 	     $(HACK_H)

# end of file
