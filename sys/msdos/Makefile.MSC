# NetHack 3.6	Makefile.MSC	$NHDT-Date: 1432512792 2015/05/25 00:13:12 $  $NHDT-Branch: master $:$NHDT-Revision: 1.21 $
#	Copyright (c) NetHack PC Development Team 1997 - 2012.
#	PC NetHack 3.4x Makefile for MSC V1.52c (16 bit compiler)
#
#	For questions or comments: nethack-bugs@nethack.org
#
#	In addition to your C compiler,
#
#       if you want to change        you will need a
#       files with suffix            workalike for
#	      .y	                yacc
#	      .l	                lex
#
#

# Game Installation Variables
# NOTE: Make sure GAMEDIR exists before make is started.

GAME = nethack
GAMEDIR =..\binary

#
# Directories
#

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

CC    = cl
LINK  = link
MAKEBIN  = nmake

# if you have a uudecode program, add its name here
# otherwise leave blank
UUDECODE =

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
# Uncomment this line if you want to include support for ALT-numeric
# sequences, such as ALT-2 for toggling #twoweapon mode.  
# Note that this code did not get a thorough testing prior to 3.4.x
#NEWALT=/DNEW_ALT

#
# Uncomment this line if your shell doesn't support FOR bat syntax (DOSBOX).
NO_FOR=Y

#############################################################################
#
# nothing below this line should have to be changed
#

LNKOPT = SCHEMA35.DEF

#
# Controls whether MOVE tracing is enabled in the executable
# This should be left commented unless you are tinkering with the
# overlay structure of NetHack.  The executable runs _very_
# slowly when the movetr.lib is linked in.
#

#MOVETR= movetr.lib

# do not change this
! IF ("$(MOVETR)"!="")
MVTRCL = /DMOVE_PROF
! ELSE
MVTRCL =
! ENDIF

#
# Uncomment the line below if you want to store all the level files,
# help files, etc. in a single library file.

USE_DLB = Y

! IF ("$(USE_DLB)"=="Y")
DLBFLG = -DDLB
! ELSE
DLBFLG =
! ENDIF

LIBRARIES = $(LIBS) $(TERMLIB)

GAMEFILE = $(GAMEDIR)\$(GAME).exe

#
#  Flags.
#
# Debugging
#CFLAGS = /Zi /DFUNCTION_LEVEL_LINKING /DUSE_TILES /DDLB
#LFLAGS = /CODEVIEW /NOI/MAP /CPARM:1 /INFO

#    Normal
LFLAGS = /NOI/MAP /CPARM:1 /INFO
CFLAGS = /DFUNCTION_LEVEL_LINKING /DUSE_TILES /DDLB
SPECOPTS =
#
# Leaving MACHINE_CODE undefined will allow it to run
# on any Intel 8088 machines and above.
# Set to 1 for 80186 and above only
# Set to 2 for 80286 and above only
# Set to 3 for 80386 and above only
# 
MACHINE_CODE = 

#
#  Utility Objects.
#
#
# Shorten up the location for some files
#

O  = $(OBJ)\				# comment so \ isn't last char

U  = $(UTIL)\				# comment so \ isn't last char

SPLEVDES = $(DAT)\Arch.des $(DAT)\Barb.des $(DAT)\bigroom.des \
	$(DAT)\castle.des $(DAT)\Caveman.des $(DAT)\endgame.des \
	$(DAT)\gehennom.des $(DAT)\Healer.des $(DAT)\Knight.des \
	$(DAT)\knox.des $(DAT)\Monk.des $(DAT)\medusa.des \
	$(DAT)\mines.des $(DAT)\oracle.des $(DAT)\Priest.des \
	$(DAT)\Ranger.des $(DAT)\Rogue.des $(DAT)\Samurai.des \
	$(DAT)\Tourist.des $(DAT)\tower.des $(DAT)\Valkyrie.des \
	$(DAT)\Wizard.des $(DAT)\yendor.des

VGAOBJ      = vidvga.o

MAKESRC	    = $(U)makedefs.c

SPLEVSRC    = $(U)lev_yacc.c  $(U)lev_$(LEX).c	 $(U)lev_main.c  \
		$(U)panic.c

DGNCOMPSRC  = $(U)dgn_yacc.c  $(U)dgn_$(LEX).c	 $(U)dgn_main.c

MAKEOBJS    = makedefs.o  monst.o	 objects.o

SPLEVOBJS   = lev_yacc.o  lev_$(LEX).o	lev_main.o  alloc.o	\
		monst.o	    objects.o	    panic.o  \
		drawing.o	decl.o	stubvid.o

DGNCOMPOBJS = dgn_yacc.o  dgn_$(LEX).o	dgn_main.o  alloc.o	 \
		panic.o

RECOVOBJS   = recover.o


#  Tile related object files.

TILOBJ      = tile.o pctiles.o $(VGAOBJ)

TEXTIO      = tiletext.o tiletxt.o drawing.o decl.o monst.o objects.o stubvid.o

TEXTIO2     = tiletex2.o tiletxt2.o drawing.o decl.o monst.o objects.o stubvid.o


PLANAR_TIB  = NetHack1.tib

OVERVIEW_TIB = NetHacko.tib

TILEUTIL    = $(TILOBJ) tile2bin.exe til2bin2.exe $(PLANAR_TIB) $(OVERVIEW_TIB)

TILEFILES   = $(WSHR)\monsters.txt $(WSHR)\objects.txt $(WSHR)\other.txt

TILEFILES2  = $(WSHR)\monthin.txt $(WSHR)\objthin.txt $(WSHR)\oththin.txt

GIFREADERS  = gifread.o alloc.o panic.o

GIFREAD2    = gifread2.o alloc.o panic.o

PPMWRITERS  = ppmwrite.o alloc.o panic.o

PPMWRIT2    = ppmwrit2.o alloc.o panic.o

DLBOBJS     = dlb_main.o dlb.o alloc.o panic.o

#  Object files for the game itself.

VOBJ01 = allmain.o  alloc.o    apply.o	  artifact.o attrib.o
VOBJ02 = ball.o	    bones.o    botl.o	  cmd.o	     dbridge.o
VOBJ03 = decl.o	    detect.o   display.o  do.o	     do_name.o
VOBJ04 = do_wear.o  dog.o      dogmove.o  dokick.o   dothrow.o
VOBJ05 = drawing.o  dungeon.o  eat.o	  end.o	     engrave.o
VOBJ06 = exper.o    explode.o  extralev.o files.o    fountain.o
VOBJ07 = getline.o  hack.o     hacklib.o  invent.o   lock.o
VOBJ08 = mail.o	    main.o     makemon.o  mapglyph.o mcastu.o   mhitm.o
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
VOBJ23 = random.o   region.o   sys.o

SOBJ   = msdos.o    sound.o    pcsys.o	  tty.o	     unix.o    video.o \
	vidtxt.o    pckeys.o

VVOBJ  = version.o

VOBJ   = $(VOBJ01) $(VOBJ02) $(VOBJ03) $(VOBJ04) $(VOBJ05) \
	$(VOBJ06) $(VOBJ07) $(VOBJ08) $(VOBJ09) $(VOBJ10) \
	$(VOBJ11) $(VOBJ12) $(VOBJ13) $(VOBJ14) $(VOBJ15) \
	$(VOBJ16) $(VOBJ17) $(VOBJ18) $(VOBJ19) $(VOBJ20) \
	$(VOBJ21) $(VOBJ22) $(VOBJ23)

ALLOBJ = $(VOBJ) $(SOBJ) $(TILOBJ) $(VVOBJ)

#
#  Header Objects.
#

DGN_FILE_H = $(INCL)\align.h	$(INCL)\dgn_file.h
DUNGEON_H  = $(INCL)\align.h	$(INCL)\dungeon.h
MONDATA_H  = $(INCL)\align.h	$(INCL)\mondata.h
MONST_H	   = $(INCL)\mextra.h   $(INCL)\align.h	$(INCL)\monst.h
PERMONST_H = $(INCL)\monattk.h	$(INCL)\monflag.h   $(INCL)\align.h   \
	    $(INCL)\permonst.h
RM_H	   = $(INCL)\align.h	$(INCL)\rm.h
SP_LEV_H   = $(INCL)\align.h	$(INCL)\sp_lev.h
YOUPROP_H  = $(PERMONST_H)	$(MONDATA_H)	    $(INCL)\prop.h    \
	    $(INCL)\pm.h       $(INCL)\youprop.h
YOU_H	   = $(MONST_H)		$(YOUPROP_H)	    $(INCL)\align.h   \
	     $(INCL)\attrib.h	$(INCL)\you.h
DISPLAY_H  = $(MONDATA_H)	$(INCL)\vision.h    $(INCL)\display.h
PCCONF_H   = $(INCL)\micro.h	$(INCL)\system.h    $(INCL)\pcconf.h \
	    $(MSYS)\pcvideo.h
CONFIG_H   = $(GLOBAL_H)	$(INCL)\tradstdc.h  $(INCL)\config1.h \
	    $(INCL)\config.h
DECL_H	   = $(YOU_H)		$(INCL)\spell.h	    $(INCL)\color.h   \
	     $(INCL)\obj.h	$(INCL)\onames.h    $(INCL)\pm.h      \
	      $(INCL)\decl.h
GLOBAL_H   = $(PCCONF_H)	$(INCL)\coord.h	    $(INCL)\global.h
HACK_H	   = $(CONFIG_H)	$(INCL)\context.h   $(DUNGEON_H) $(DECL_H) \
	       $(DISPLAY_H)	  $(INCL)\monsym.h    $(INCL)\mkroom.h	\
	      $(INCL)\objclass.h $(INCL)\trap.h	     $(INCL)\flag.h    \
	      $(RM_H)		 $(INCL)\vision.h    $(INCL)\wintype.h \
	      $(INCL)\engrave.h	 $(INCL)\rect.h	     $(INCL)\sys.h \
	       $(INCL)\trampoli.h $(INCL)\hack.h
DLB_H      = $(INCL)\dlb.h
TILE_H	   = $(WSHR)\tile.h $(MSYS)\pctiles.h


# Make Roolz dude.
# Due to the inadequacy of some makes these must accord with a
# topological sort of the generated-from relation... output on
# the left, input on the right. Trust me.
#

.SUFFIXES:  .exe .o .til .uu .c .y .l

#
# Rules for files in src
#


.c{$(OBJ)}.o:
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $<

.c.o:
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $<

{$(SRC)}.c{$(OBJ)}.o:
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $<

{$(SRC)}.c.o:
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $<

#
# Rules for files in sys\share
#

{$(SYS)}.c{$(OBJ)}.o:
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $<

{$(SYS)}.c.o:
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $<

#
# Rules for files in sys\msdos
#

{$(MSYS)}.c{$(OBJ)}.o:
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $<

{$(MSYS)}.c.o:
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $<

{$(MSYS)}.h{$(INCL)}.h:
	@copy $< $@

#
# Rules for files in util
#

{$(UTIL)}.c{$(OBJ)}.o:
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $<

{$(UTIL)}.c.o:
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $<

#
# Rules for files in win\share
#

{$(WSHR)}.c.o:
	@@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $<

{$(WSHR)}.c{$(OBJ)}.o:
	@@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $<

{$(WSHR)}.h{$(INCL)}.h:
	@copy $< $@

{$(WSHR)}.txt{$(DAT)}.txt:
	@copy $< $@

#
# Rules for files in win\tty
#

{$(WTTY)}.c{$(OBJ)}.o:
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $<

{$(WTTY)}.c.o:
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $<


! IF ("$(USE_DLB)"=="Y")
DLB = nhdat
! ELSE
DLB =
! ENDIF

########################################################
#  
# TARGETS


#
#  The default make target (so just typing 'nmake' is useful).
#
default : envchk $(GAMEFILE)

#  The default target.

$(GAME): $(O)utility.tag $(GAMEFILE)
	@echo $(GAME) is up to date.

#
#  Everything
#

all: install

install: envchk $(GAME) $(O)install.tag
	@echo Done.

$(O)install.tag: 	$(DAT)\data	$(DAT)\rumors	 $(DAT)\dungeon \
	 	$(DAT)\oracles	$(DAT)\quest.dat $(O)sp_lev.tag $(DLB)
! IF ("$(USE_DLB)"=="Y")
	copy $(SRC)\nhdat         $(GAMEDIR)
	copy $(DAT)\license       $(GAMEDIR)
! ELSE
	copy $(DAT)\*.            $(GAMEDIR)
	copy $(DAT)\*.dat         $(GAMEDIR)
	copy $(DAT)\*.lev         $(GAMEDIR)
	if exist $(GAMEDIR)\makefile del $(GAMEDIR)\makefile
! ENDIF
	if exist $(DAT)\symbols copy $(DAT)\symbols $(GAMEDIR)
	copy $(SSHR)\termcap	  $(GAMEDIR)
	copy *.tib                $(GAMEDIR)
	copy $(SSHR)\NetHack.cnf  $(GAMEDIR)\defaults.nh
	copy $(MSYS)\NHAccess.nh  $(GAMEDIR)
	copy $(U)recover.exe          $(GAMEDIR)
	if exist $(DOC)\guideb*.txt copy $(DOC)\guideb*.txt $(GAMEDIR)
	if exist $(DOC)\recover.txt copy $(DOC)\recover.txt $(GAMEDIR)
	if exist $(DOC)\nethack.txt copy $(DOC)\nethack.txt $(GAMEDIR)
	echo install done > $@

$(O)sp_lev.tag: $(O)utility.tag $(SPLEVDES)
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
	cd $(SRC)
#	-@if exist $(O)sp_lev.tag del $(O)sp_lev.tag
	@echo sp_levs done >$(O)sp_lev.tag
	
$(O)utility.tag: $(INCL)\date.h $(INCL)\trap.h  \
        $(INCL)\onames.h $(INCL)\pm.h monstr.c vis_tab.c \
	$(U)lev_comp.exe $(U)dgn_comp.exe $(U)recover.exe $(TILEUTIL)
	-@if exist $(O)utility.tag del $(O)utility.tag
	@echo utilities made > $@

tileutil: gif2txt.exe txt2ppm.exe
	@echo Optional tile development utilities are up to date.

.PHONEY: envchk

envchk:
! 	IF ("$(MACHINE_CODE)"!="")
	   @SET MC=/G$(MACHINE_CODE)
!	ELSE
	   @SET MC=
!	ENDIF
!	IF ("$(CL)"=="")
	   @echo CL Environment variable is defined as follows:
	   SET CL=/AL $(MC) /Oo /Gy /Gs /Gt10 /Gf /Zp1 /W2 /I$(INCL) /I$(MSYS) /I$(WSHR) /nologo /c
!	ELSE
	   @echo Warning CL Environment variable is defined:
	   @echo CL=$(CL)
	   @echo Overriding that definition as follows:
	   SET CL=/AL $(MC) /Oo /Gy /Gs /Gt10 /Gf /Zp1 /W2 /I$(INCL) /I$(MSYS) /I$(WSHR) /nologo /c
!	ENDIF

#  The main target.

$(GAMEFILE) :  $(LNKOPT) $(ALLOBJ)
	@echo Linking....
	$(LINK) $(LFLAGS) /SE:1000 /DYNAMIC:2650 /NOE /ST:6000 @<<$(GAME).lnk
		$(ALLOBJ:^	=+^
		)
		$(GAMEFILE)
		$(GAME)
		$(TERMLIB) $(MOVETR) $(CLIB) $(BCOVL) $(BCMDL)
		$(LNKOPT);
<<
	@if exist $(O)install.tag del $(O)install.tag
	@if exist $(GAMEDIR)\$(GAME).bak del $(GAMEDIR)\$(GAME).bak

#
#  Housekeeping.
#

clean:
	del *.o
	del *.map
	del $(U)dlb_main.exe

spotless: clean
	if exist $(O)utility.tag	   del $(O)utility.tag
	if exist $(O)install.tag   del $(O)install.tag
	if exist $(GAME).lnk	   del $(GAME).lnk
	if exist $(U)makedefs.exe  del $(U)makedefs.exe
	if exist $(U)lev_comp.exe  del $(U)lev_comp.exe
	if exist $(U)dgn_comp.exe  del $(U)dgn_comp.exe
	if exist $(SRC)\lev_lex.c  del $(SRC)\lev_lex.c
	if exist $(SRC)\lev_yacc.c del $(SRC)\lev_yacc.c
	if exist $(SRC)\dgn_lex.c  del $(SRC)\dgn_lex.c
	if exist $(SRC)\dgn_yacc.c del $(SRC)\dgn_yacc.c
	if exist $(U)recover.exe   del $(U)recover.exe
	if exist $(INCL)\onames.h  del $(INCL)\onames.h
	if exist $(INCL)\pm.h	   del $(INCL)\pm.h
	if exist $(INCL)\vis_tab.h del $(INCL)\vis_tab.h
	if exist $(INCL)\pcvideo.h del $(INCL)\pcvideo.h
	if exist $(MSYS)\pctiles.h del $(MSYS)\pctiles.h
	if exist $(INCL)\portio.h  del $(MSYS)\portio.h
	if exist $(WSHR)\tile.h	   del $(WSHR)\tile.h
	if exist monstr.c	   del monstr.c
	if exist vis_tab.c	   del vis_tab.c
	if exist $(SRC)\panic.c	   del $(SRC)\panic.c
	if exist $(SRC)\makedefs.c del $(SRC)\makedefs.c
	if exist $(SRC)\recover.c  del $(SRC)\recover.c
	if exist $(SRC)\lev_main.c del $(SRC)\lev_main.c
	if exist $(SRC)\dlb_main.c del $(SRC)\dlb_main.c
	if exist $(SRC)\dgn_main.c del $(SRC)\dgn_main.c
	if exist $(SRC)\wintty.c   del $(SRC)\wintty.c
	if exist $(SRC)\topl.c	   del $(SRC)\topl.c
	if exist $(SRC)\getline.c  del $(SRC)\getline.c
	if exist $(SRC)\termcap.c  del $(SRC)\termcap.c
	if exist $(SRC)\tile2bin.c del $(SRC)\tile2bin.c
	if exist $(SRC)\msdos.c	   del $(SRC)\msdos.c
	if exist $(SRC)\pckeys.c   del $(SRC)\pckeys.c
	if exist $(SRC)\video.c	   del $(SRC)\video.c
	if exist $(SRC)\sound.c	   del $(SRC)\sound.c
	if exist $(SRC)\tilemap.c  del $(SRC)\tilemap.c
	if exist $(SRC)\gifread.c  del $(SRC)\gifread.c
	if exist $(SRC)\ppmwrite.c del $(SRC)\ppmwrite.c
	if exist $(SRC)\pcmain.c   del $(SRC)\pcmain.c
	if exist $(SRC)\pcunix.c   del $(SRC)\pcunix.c
	if exist $(SRC)\pcsys.c	   del $(SRC)\pcsys.c
	if exist $(SRC)\pctty.c	   del $(SRC)\pctty.c
	if exist $(SRC)\tile.c	   del $(SRC)\tile.c
	if exist $(INCL)\date.h	   del $(INCL)\date.h
	if exist $(INCL)\onames.h  del $(INCL)\onames.h
	if exist $(INCL)\pm.h	   del $(INCL)\pm.h
	if exist $(INCL)\vis_tab.h del $(INCL)\vis_tab.h
	if exist vis_tab.c	   del vis_tab.c
	if exist *.lnk		   del *.lnk
	if exist *.def		   del *.def
	if exist *.map		   del *.map
	if exist a.out		   del a.out
	if exist tilemap.exe       del tilemap.exe
	if exist tile2bin.exe      del tile2bin.exe
	if exist $(DAT)\data	   del $(DAT)\data
	if exist $(DAT)\*.lev      del $(DAT)\*.lev
	if exist $(DAT)\data	   del $(DAT)\data
	if exist $(DAT)\dungeon	   del $(DAT)\dungeon
	if exist $(DAT)\options	   del $(DAT)\options
	if exist $(DAT)\oracles	   del $(DAT)\oracles
	if exist $(DAT)\rumors	   del $(DAT)\rumors
	if exist $(DAT)\quest.dat  del $(DAT)\quest.dat
	if exist $(SRC)\nhdat      del $(SRC)\nhdat
	if exist $(DAT)\dlb.lst    del $(DAT)\dlb.lst
	if exist $(DAT)\msdoshlp.txt del $(DAT)\msdoshlp.txt
	if exist $(DAT)\dlb_main.exe del $(DAT)\dlb_main.exe
	if exist $(DAT)\lev_comp.exe del $(DAT)\lev_comp.exe
	if exist $(DAT)\dgn_comp.exe del $(DAT)\dgn_comp.exe
	if exist $(O)sp_lev.tag        del $(O)sp_lev.tag
	if exist $(PLANAR_TIB)	   del $(PLANAR_TIB)
	if exist $(OVERVIEW_TIB)   del $(OVERVIEW_TIB)


#
#  Secondary Targets.
#
#
#  Makedefs Stuff
#

$(U)makedefs.exe:  $(MAKEOBJS)
        @echo Linking....
        @$(LINK) $(LFLAGS) @<<$(@B).lnk
                $(MAKEOBJS:^      =+^
                )
                $@
                $(@B)
                ;
<<

makedefs.o:  $(CONFIG_H)	$(PERMONST_H)	   $(INCL)\objclass.h \
		$(INCL)\monsym.h   $(INCL)\qtext.h $(UTIL)\makedefs.c

#  The following include files depend on makedefs to be created.
#
#  date.h should be remade every time any of the source or include
#  files is modified.


$(INCL)\date.h : $(U)makedefs.exe
	-$(U)makedefs -v

$(INCL)\onames.h: $(U)makedefs.exe
	-$(U)makedefs -o

$(INCL)\pm.h: $(U)makedefs.exe
	-$(U)makedefs -p

monstr.c: $(U)makedefs.exe
	-$(U)makedefs -m

$(INCL)\vis_tab.h: $(U)makedefs.exe
	-$(U)makedefs -z

vis_tab.c: $(U)makedefs.exe
	-$(U)makedefs -z

#
# Level Compiler Stuff
#

$(U)lev_comp.exe:  $(SPLEVOBJS)
	@echo Linking $@...
	@$(LINK) $(LFLAGS) @<<$(@B).lnk
		$(SPLEVOBJS:^	=+^
		)
		$@
		$(@B)
		$(BCMDL);
<<

$(O)lev_yacc.o:  $(HACK_H)   $(SP_LEV_H) $(INCL)\lev_comp.h $(U)lev_yacc.c

$(O)lev_$(LEX).o:  $(HACK_H)   $(INCL)\lev_comp.h $(SP_LEV_H) \
	$(U)lev_$(LEX).c

$(O)lev_main.o:	$(U)lev_main.c $(HACK_H)   $(SP_LEV_H)

$(U)lev_yacc.c $(INCL)\lev_comp.h : $(U)lev_comp.y
!	IF "$(DO_YACC)"=="YACC_ACT"
	   $(YACC) -d -l $(U)lev_comp.y
	   copy $(YTABC) $(U)lev_yacc.c
	   copy $(YTABH) $(INCL)\lev_comp.h
	   @del $(YTABC)
	   @del $(YTABH)
!	ELSE
	   @echo.
	   @echo $(U)lev_comp.y has changed.
	   @echo To update $(U)lev_yacc.c and $(INCL)\lev_comp.h run $(YACC).
	   @echo.
	   @echo For now, we will copy the prebuilt lev_yacc.c
	   @echo from $(SSHR) to $(U)lev_yacc.c, and copy the prebuilt
	   @echo lev_comp.h from $(SYS) to $(UTIL)\lev_comp.h
	   @echo and use those.
	   @echo.
	   copy $(SSHR)\lev_yacc.c $@ >nul
	   touch $@
	   copy $(SSHR)\lev_comp.h $(INCL)\lev_comp.h >nul
	   touch $(INCL)\lev_comp.h
!	ENDIF

$(U)lev_$(LEX).c:  $(U)lev_comp.l
!	IF "$(DO_LEX)"=="LEX_ACT"
	   $(LEX) $(FLEXSKEL) $(U)lev_comp.l
	   copy $(LEXYYC) $@
	   @del $(LEXYYC)
!	ELSE
	   @echo.
	   @echo $(U)lev_comp.l has changed. To update $@ run $(LEX).
	   @echo.
	   @echo For now, we will copy a prebuilt lev_lex.c
	   @echo from $(SSHR) to $@ and use it.
	   @echo.
	   copy $(SSHR)\lev_lex.c $@ >nul
	   touch $@
!	ENDIF

#
# Dungeon Stuff
#

$(U)dgn_comp.exe: $(DGNCOMPOBJS)
    @echo Linking $@...
	@$(LINK) $(LFLAGS) @<<$(@B).lnk
		$(DGNCOMPOBJS:^	=+^
		)
		$@
		$(@B)
		$(BCMDL);
<<

$(O)dgn_yacc.o:	$(HACK_H)   $(DGN_FILE_H) $(INCL)\dgn_comp.h \
	$(U)dgn_yacc.c

$(O)dgn_$(LEX).o: $(HACK_H)   $(DGN_FILE_H)  $(INCL)\dgn_comp.h \
	$(U)dgn_$(LEX).c

$(O)dgn_main.o:	$(HACK_H) $(U)dgn_main.c

$(U)dgn_yacc.c $(INCL)\dgn_comp.h : $(U)dgn_comp.y
!	IF "$(DO_YACC)"=="YACC_ACT"
	   $(YACC) -d -l $(U)dgn_comp.y
	   copy $(YTABC) $(U)dgn_yacc.c
	   copy $(YTABH) $(INCL)\dgn_comp.h
	   @del $(YTABC)
	   @del $(YTABH)
!	ELSE
	   @echo.
	   @echo $(U)dgn_comp.y has changed. To update $@ and
	   @echo $(INCL)\dgn_comp.h run $(YACC).
	   @echo.
	   @echo For now, we will copy the prebuilt dgn_yacc.c from
	   @echo $(SSHR) to $(U)dgn_yacc.c, and copy the prebuilt
	   @echo dgn_comp.h from $(SSHR) to $(INCL)\dgn_comp.h 
	   @echo and use those.
	   @echo.
	   copy $(SSHR)\dgn_yacc.c $@ >nul
	   touch $@
	   copy $(SSHR)\dgn_comp.h $(INCL)\dgn_comp.h >nul
	   touch $(INCL)\dgn_comp.h
!	ENDIF

$(U)dgn_$(LEX).c:  $(U)dgn_comp.l
!	IF "$(DO_LEX)"=="LEX_ACT"
	   $(LEX) $(FLEXSKEL)  $(U)dgn_comp.l
	   copy $(LEXYYC) $@
	   @del $(LEXYYC)
!	ELSE
	   @echo.
	   @echo $(U)dgn_comp.l has changed. To update $@ run $(LEX).
	   @echo.
	   @echo For now, we will copy a prebuilt dgn_lex.c
	   @echo from $(SSHR) to $@ and use it.
	   @echo.
	   copy $(SSHR)\dgn_lex.c $@ >nul
	   touch $@
!	ENDIF

#
#  Recover Utility
#

$(U)recover.exe:   $(RECOVOBJS)
        @echo Linking....
        @$(LINK) $(LFLAGS) @<<$(@B).lnk
                $(RECOVOBJS:^      =+^
                )
                $@
                $(@B)
                ;
<<

recover.o:   $(CONFIG_H) $(UTIL)\recover.c

#
#  Header file moves required for tile support
#

#$(WSHR)\tile.h: $(WSHR)\tile.h
#	copy $(WSHR)\tile.h $@

#$(MSYS)\pctiles.h: $(MSYS)\pctiles.h
#	copy $(MSYS)\pctiles.h $@

#$(INCL)\pcvideo.h: $(MSYS)\pcvideo.h
#	copy $(MSYS)\pcvideo.h $@

#$(MSYS)\portio.h: $(MSYS)\portio.h
#	copy $(MSYS)\portio.h $@

#
#  Tile Mapping
#

tile.c: tilemap.exe
	@tilemap
	@echo A new $@ has been created

tilemap.exe: tilemap.o
        @echo Linking....
        @$(LINK) $(LFLAGS) @<<$(@B).lnk
		tilemap.o
                $@
                $(@B)
                ;
<<

tilemap.c: $(WSHR)\tilemap.c
	copy $(WSHR)\tilemap.c .

tilemap.o: tilemap.c $(HACK_H) $(TILE_H)

#
#   Tile Utilities
#
#
#  Required for tile support
#

NetHack1.tib: $(TILEFILES) tile2bin.exe
	@echo Creating binary tile files (this may take some time)
	@tile2bin

NetHacko.tib: thintile.tag $(TILEFILES2) til2bin2.exe
	@echo Creating overview binary tile files (this may take some time)
	@til2bin2

tile2bin.exe: tile2bin.o $(TEXTIO)
	@$(LINK) $(LFLAGS) tile2bin.o $(TEXTIO),$@,$(@B);

til2bin2.exe: til2bin2.o $(TEXTIO2)
	@$(LINK) $(LFLAGS) til2bin2.o $(TEXTIO2),$@,$(@B);

thintile.exe: thintile.o 
	@$(LINK) $(LFLAGS) thintile.o,$@,$(@B);

thintile.o:  $(HACK_H) $(WSHR)\tile.h $(WSHR)\thintile.c

thintile.tag: thintile.exe $(TILEFILES)
	thintile
	@echo thintiles created >thintile.tag

tile2bin.o:  $(HACK_H) $(WSHR)\tile.h $(MSYS)\pctiles.h $(MSYS)\pcvideo.h \
	$(MSYS)\tile2bin.c

til2bin2.o:  $(HACK_H) $(WSHR)\tile.h $(MSYS)\pctiles.h $(MSYS)\pcvideo.h \
	$(MSYS)\tile2bin.c
	-@$(CC) $(CFLAGS) $(SPECOPTS) -DTILE_X=8 -DOVERVIEW_FILE /Zg $(MSYS)\tile2bin.c >$(@B).pro
	@$(CC) $(CFLAGS) $(SPECOPTS) -DTILE_X=8 -DOVERVIEW_FILE /Fo$@ $(MSYS)\tile2bin.c

tiletext.o:  $(CONFIG_H) $(WSHR)\tile.h $(WSHR)\tiletext.c

tiletex2.o:  $(CONFIG_H) $(WSHR)\tile.h $(WSHR)\tiletext.c
	-@$(CC) $(CFLAGS) $(SPECOPTS) -DTILE_X=8 /Zg $(WSHR)\tiletext.c >$(@B).pro
	@$(CC) $(CFLAGS) $(SPECOPTS) -DTILE_X=8 /Fo$@ $(WSHR)\tiletext.c

tiletxt.o: $(CONFIG_H)	$(WSHR)\tile.h tilemap.c
	-@$(CC) $(CFLAGS) $(SPECOPTS) -DTILETEXT /Zg tilemap.c >$(@B).pro
	@$(CC) $(CFLAGS) $(SPECOPTS) -DTILETEXT /Fo$@ tilemap.c
  
tiletxt2.o: $(CONFIG_H)	$(WSHR)\tile.h tilemap.c
	-@$(CC) $(CFLAGS) $(SPECOPTS) -DTILETEXT /Zg tilemap.c >$(@B).pro
	@$(CC) $(CFLAGS) $(SPECOPTS) -DTILETEXT -DTILE_X=8 /Fo$@ tilemap.c
#
# Optional GIF Utilities (for development)
#

gif2txt.exe: $(GIFREADERS) $(TEXTIO)
	@$(LINK) $(LFLAGS) $(GIFREADERS) $(TEXTIO),$@,$(@B);

gif2txt2.exe: $(GIFREAD2) $(TEXTIO2)
	@$(LINK) $(LFLAGS) $(GIFREAD2) $(TEXTIO2),$@,$(@B);

txt2ppm.exe: $(PPMWRITERS) $(TEXTIO)
	@$(LINK) $(LFLAGS) $(PPMWRITERS) $(TEXTIO),$@,$(@B);

txt2ppm2.exe: $(PPMWRIT2) $(TEXTIO2)
	@$(LINK) $(LFLAGS) $(PPMWRIT2) $(TEXTIO2),$@,$(@B);

gifread.o: $(CONFIG_H) $(WSHR)\tile.h

gifread2.o: $(CONFIG_H) $(WSHR)\tile.h $(WSHR)\gifread.c
	-@$(CC) $(CFLAGS) $(SPECOPTS) -DTILE_X=8 /Zg gifread.c >$(@B).pro
	@$(CC) $(CFLAGS) $(SPECOPTS) -DTILE_X=8 $(WSHR)\gifread.c

ppmwrite.o: $(CONFIG_H)	$(WSHR)\tile.h $(WSHR)\ppmwrite.c

ppmwrit2.o:  $(CONFIG_H) $(WSHR)\tile.h $(WSHR)\ppmwrite.c
	-@$(CC) $(CFLAGS) $(SPECOPTS) -DTILE_X=8 /Zg ppmwrite.c >$(@B).pro
	@$(CC) $(CFLAGS) $(SPECOPTS) -DTILE_X=8 $(WSHR)\ppmwrite.c

#
#  Optional tile viewer (development sources only)
#

viewtib.exe: viewtib.o
	@$(LINK) $(LFLAGS) -oviewtib.exe viewtib.o $(LIBRARIES)

viewtib.o: $(MSYS)\viewtib.c

#
#  Other Util Dependencies.
#

alloc.o: $(CONFIG_H)	 alloc.c
drawing.o: $(CONFIG_H) drawing.c $(MSYS)\pcvideo.h
decl.o: $(CONFIG_H) decl.c
monst.o: $(CONFIG_H)	     $(PERMONST_H)	$(INCL)\monsym.h   \
	$(INCL)\color.h	 monst.c

objects.o: $(CONFIG_H)	      $(INCL)\obj.h	 $(INCL)\objclass.h \
	$(INCL)\prop.h	    $(INCL)\color.h    objects.c

panic.o:   $(CONFIG_H)	  $(UTIL)\panic.c



#
# make data.base an 8.3 filename to prevent an nmake warning
#

DATABASE = $(DAT)\data.bas

$(DAT)\data:	 $(O)utility.tag	$(DATABASE)
	$(U)makedefs -d

$(DAT)\rumors:	     $(O)utility.tag    $(DAT)\rumors.tru	$(DAT)\rumors.fal
	$(U)makedefs -r

$(DAT)\quest.dat: $(O)utility.tag  $(DAT)\quest.txt
	$(U)makedefs -q

$(DAT)\oracles:	     $(O)utility.tag    $(DAT)\oracles.txt
	$(U)makedefs -h

$(DAT)\dungeon: $(O)utility.tag  $(DAT)\dungeon.def
	$(U)makedefs -e
	cd $(DAT)
	$(U)dgn_comp dungeon.pdf
	cd $(SRC)

#
# DLB stuff
#
#
nhdat:	$(U)dlb_main.exe $(DAT)\data $(DAT)\oracles $(DAT)\options \
	$(DAT)\quest.dat $(DAT)\rumors $(DAT)\help $(DAT)\hh $(DAT)\cmdhelp \
	$(DAT)\history $(DAT)\opthelp $(DAT)\wizhelp $(DAT)\dungeon \
	$(DAT)\license $(O)sp_lev.tag
	@copy $(MSYS)\msdoshlp.txt $(DAT)
	@cd $(DAT)
	@echo data >dlb.lst
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
	@echo dungeon >>dlb.lst
	@echo license >>dlb.lst
	@echo msdoshlp.txt >>dlb.lst
! IF ("$(NO_FOR)"=="Y")
	echo AIR.LEV       >>dlb.lst
	echo ARC-FILA.LEV  >>dlb.lst
	echo ARC-FILB.LEV  >>dlb.lst
	echo ARC-GOAL.LEV  >>dlb.lst
	echo ARC-LOCA.LEV  >>dlb.lst
	echo ARC-STRT.LEV  >>dlb.lst
	echo ASMODEUS.LEV  >>dlb.lst
	echo ASTRAL.LEV    >>dlb.lst
	echo BAALZ.LEV     >>dlb.lst
	echo BAR-FILA.LEV  >>dlb.lst
	echo BAR-FILB.LEV  >>dlb.lst
	echo BAR-GOAL.LEV  >>dlb.lst
	echo BAR-LOCA.LEV  >>dlb.lst
	echo BAR-STRT.LEV  >>dlb.lst
	echo BIGRM-1.LEV   >>dlb.lst
	echo BIGRM-2.LEV   >>dlb.lst
	echo BIGRM-3.LEV   >>dlb.lst
	echo BIGRM-4.LEV   >>dlb.lst
	echo BIGRM-5.LEV   >>dlb.lst
	echo CASTLE.LEV    >>dlb.lst
	echo CAV-FILA.LEV  >>dlb.lst
	echo CAV-FILB.LEV  >>dlb.lst
	echo CAV-GOAL.LEV  >>dlb.lst
	echo CAV-LOCA.LEV  >>dlb.lst
	echo CAV-STRT.LEV  >>dlb.lst
	echo EARTH.LEV     >>dlb.lst
	echo FAKEWIZ1.LEV  >>dlb.lst
	echo FAKEWIZ2.LEV  >>dlb.lst
	echo FIRE.LEV      >>dlb.lst
	echo HEA-FILA.LEV  >>dlb.lst
	echo HEA-FILB.LEV  >>dlb.lst
	echo HEA-GOAL.LEV  >>dlb.lst
	echo HEA-LOCA.LEV  >>dlb.lst
	echo HEA-STRT.LEV  >>dlb.lst
	echo JUIBLEX.LEV   >>dlb.lst
	echo KNI-FILA.LEV  >>dlb.lst
	echo KNI-FILB.LEV  >>dlb.lst
	echo KNI-GOAL.LEV  >>dlb.lst
	echo KNI-LOCA.LEV  >>dlb.lst
	echo KNI-STRT.LEV  >>dlb.lst
	echo KNOX.LEV      >>dlb.lst
	echo MEDUSA-1.LEV  >>dlb.lst
	echo MEDUSA-2.LEV  >>dlb.lst
	echo MINEFILL.LEV  >>dlb.lst
	echo MINEND-1.LEV  >>dlb.lst
	echo MINEND-2.LEV  >>dlb.lst
	echo MINEND-3.LEV  >>dlb.lst
	echo MINETN-1.LEV  >>dlb.lst
	echo MINETN-2.LEV  >>dlb.lst
	echo MINETN-3.LEV  >>dlb.lst
	echo MINETN-4.LEV  >>dlb.lst
	echo MINETN-5.LEV  >>dlb.lst
	echo MINETN-6.LEV  >>dlb.lst
	echo MINETN-7.LEV  >>dlb.lst
	echo MON-FILA.LEV  >>dlb.lst
	echo MON-FILB.LEV  >>dlb.lst
	echo MON-GOAL.LEV  >>dlb.lst
	echo MON-LOCA.LEV  >>dlb.lst
	echo MON-STRT.LEV  >>dlb.lst
	echo ORACLE.LEV    >>dlb.lst
	echo ORCUS.LEV     >>dlb.lst
	echo PRI-FILA.LEV  >>dlb.lst
	echo PRI-FILB.LEV  >>dlb.lst
	echo PRI-GOAL.LEV  >>dlb.lst
	echo PRI-LOCA.LEV  >>dlb.lst
	echo PRI-STRT.LEV  >>dlb.lst
	echo RAN-FILA.LEV  >>dlb.lst
	echo RAN-FILB.LEV  >>dlb.lst
	echo RAN-GOAL.LEV  >>dlb.lst
	echo RAN-LOCA.LEV  >>dlb.lst
	echo RAN-STRT.LEV  >>dlb.lst
	echo ROG-FILA.LEV  >>dlb.lst
	echo ROG-FILB.LEV  >>dlb.lst
	echo ROG-GOAL.LEV  >>dlb.lst
	echo ROG-LOCA.LEV  >>dlb.lst
	echo ROG-STRT.LEV  >>dlb.lst
	echo SAM-FILA.LEV  >>dlb.lst
	echo SAM-FILB.LEV  >>dlb.lst
	echo SAM-GOAL.LEV  >>dlb.lst
	echo SAM-LOCA.LEV  >>dlb.lst
	echo SAM-STRT.LEV  >>dlb.lst
	echo SANCTUM.LEV   >>dlb.lst
	echo SOKO1-1.LEV   >>dlb.lst
	echo SOKO1-2.LEV   >>dlb.lst
	echo SOKO2-1.LEV   >>dlb.lst
	echo SOKO2-2.LEV   >>dlb.lst
	echo SOKO3-1.LEV   >>dlb.lst
	echo SOKO3-2.LEV   >>dlb.lst
	echo SOKO4-1.LEV   >>dlb.lst
	echo SOKO4-2.LEV   >>dlb.lst
	echo TOU-FILA.LEV  >>dlb.lst
	echo TOU-FILB.LEV  >>dlb.lst
	echo TOU-GOAL.LEV  >>dlb.lst
	echo TOU-LOCA.LEV  >>dlb.lst
	echo TOU-STRT.LEV  >>dlb.lst
	echo TOWER1.LEV    >>dlb.lst
	echo TOWER2.LEV    >>dlb.lst
	echo TOWER3.LEV    >>dlb.lst
	echo VAL-FILA.LEV  >>dlb.lst
	echo VAL-FILB.LEV  >>dlb.lst
	echo VAL-GOAL.LEV  >>dlb.lst
	echo VAL-LOCA.LEV  >>dlb.lst
	echo VAL-STRT.LEV  >>dlb.lst
	echo VALLEY.LEV    >>dlb.lst
	echo WATER.LEV     >>dlb.lst
	echo WIZ-FILA.LEV  >>dlb.lst
	echo WIZ-FILB.LEV  >>dlb.lst
	echo WIZ-GOAL.LEV  >>dlb.lst
	echo WIZ-LOCA.LEV  >>dlb.lst
	echo WIZ-STRT.LEV  >>dlb.lst
	echo WIZARD1.LEV   >>dlb.lst
	echo WIZARD2.LEV   >>dlb.lst
	echo WIZARD3.LEV   >>dlb.lst
! ELSE
	@for %%N in (*.lev) do echo %%N >>dlb.lst
! ENDIF
	$(U)dlb_main cvIf dlb.lst $(SRC)\nhdat
	@cd $(SRC)

$(U)dlb_main.exe: $(DLBOBJS)
	link $(LFLAGS) /ST:5120 $(DLBOBJS),$@,$(@B);

dlb_main.o: $(U)dlb_main.c $(INCL)\config.h $(DLB_H)
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $(U)dlb_main.c

# Game Dependencies

# sys/share
main.o: $(SSHR)\pcmain.c $(HACK_H) $(INCL)\dlb.h \
		#$(INCL)\win32api.h
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $(SSHR)\pcmain.c
pcsys.o: $(SSHR)\pcsys.c $(HACK_H)
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $(SSHR)\pcsys.c
tty.o: $(SSHR)\pctty.c $(HACK_H)
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $(SSHR)\pctty.c
unix.o: $(SSHR)\pcunix.c $(HACK_H)
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $(SSHR)\pcunix.c
random.o: $(SSHR)\random.c $(HACK_H)
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $(SSHR)\random.c

# sys/msdos
msdos.o : $(HACK_H)  $(MSYS)\msdos.c
	@$(CC) $(CFLAGS) $(SPECOPTS) $(NEWALT) /Fo$@ $(MSYS)\msdos.c
pckeys.o : $(HACK_H)  $(MSYS)\pckeys.c
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $(MSYS)\pckeys.c
pctiles.o : $(HACK_H)  $(MSYS)\pctiles.c $(MSYS)\portio.h
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $(MSYS)\pctiles.c
sound.o : $(HACK_H)  $(MSYS)\sound.c $(MSYS)\portio.h
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $(MSYS)\sound.c
video.o : $(HACK_H)  $(MSYS)\pcvideo.h $(MSYS)\portio.h $(MSYS)\video.c
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $(MSYS)\video.c
vidvga.o : $(HACK_H)  $(MSYS)\pcvideo.h $(MSYS)\portio.h $(TILE_H) \
	$(MSYS)\vidvga.c
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $(MSYS)\vidvga.c
vidtxt.o : $(HACK_H)  $(MSYS)\pcvideo.h $(MSYS)\portio.h $(TILE_H) \
	$(MSYS)\vidtxt.c
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $(MSYS)\vidtxt.c
stubvid.o : $(HACK_H) $(MSYS)\video.c
	@$(CC) $(CFLAGS) $(SPECOPTS) -DSTUBVIDEO /Fo$@ $(MSYS)\video.c

# win/tty
getline.o: $(WIN)\getline.c $(HACK_H) $(INCL)\func_tab.h
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $(WIN)\getline.c
termcap.o: $(WIN)\termcap.c $(HACK_H) $(INCL)\tcap.h
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $(WIN)\termcap.c
topl.o: $(WIN)\topl.c $(HACK_H) $(INCL)\tcap.h
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $(WIN)\topl.c
wintty.o: $(WIN)\wintty.c $(HACK_H) $(INCL)\dlb.h \
		$(INCL)\date.h $(INCL)\patchlev.h $(INCL)\tcap.h
	@$(CC) $(CFLAGS) $(SPECOPTS) /Fo$@ $(WIN)\wintty.c

# src dependencies
allmain.o: allmain.c $(HACK_H)
alloc.o: alloc.c $(CONFIG_H)
apply.o: apply.c $(HACK_H)
artifact.o: artifact.c $(HACK_H) $(INCL)\artifact.h $(INCL)\artilist.h
attrib.o: attrib.c $(HACK_H) $(INCL)\artifact.h
ball.o: ball.c $(HACK_H)
bones.o: bones.c $(HACK_H) $(INCL)\lev.h
botl.o: botl.c $(HACK_H)
cmd.o: cmd.c $(HACK_H) $(INCL)\func_tab.h
dbridge.o: dbridge.c $(HACK_H)
decl.o: decl.c $(HACK_H)
detect.o: detect.c $(HACK_H) $(INCL)\artifact.h
dig.o: dig.c $(HACK_H)
display.o: display.c $(HACK_H)
dlb.o: dlb.c $(CONFIG_H) $(INCL)\dlb.h
do.o: do.c $(HACK_H) $(INCL)\lev.h
do_name.o: do_name.c $(HACK_H)
do_wear.o: do_wear.c $(HACK_H)
dog.o: dog.c $(HACK_H)
dogmove.o: dogmove.c $(HACK_H) $(INCL)\mfndpos.h
dokick.o: dokick.c $(HACK_H)
dothrow.o: dothrow.c $(HACK_H)
drawing.o: drawing.c $(HACK_H) $(INCL)\tcap.h
dungeon.o: dungeon.c $(HACK_H) $(INCL)\dgn_file.h $(INCL)\dlb.h
eat.o: eat.c $(HACK_H)
end.o: end.c $(HACK_H) $(INCL)\lev.h $(INCL)\dlb.h
engrave.o: engrave.c $(HACK_H) $(INCL)\lev.h
exper.o: exper.c $(HACK_H)
explode.o: explode.c $(HACK_H)
extralev.o: extralev.c $(HACK_H)
files.o: files.c $(HACK_H) $(INCL)\dlb.h
fountain.o: fountain.c $(HACK_H)
hack.o: hack.c $(HACK_H)
hacklib.o: hacklib.c $(HACK_H)
invent.o: invent.c $(HACK_H) $(INCL)\artifact.h
light.o: light.c $(HACK_H) $(INCL)\lev.h
lock.o: lock.c $(HACK_H)
mail.o: mail.c $(HACK_H) $(INCL)\mail.h
makemon.o: makemon.c $(HACK_H)
mapglyph.o: mapglyph.c $(HACK_H)
mcastu.o: mcastu.c $(HACK_H)
mhitm.o: mhitm.c $(HACK_H) $(INCL)\artifact.h
mhitu.o: mhitu.c $(HACK_H) $(INCL)\artifact.h
minion.o: minion.c $(HACK_H)
mklev.o: mklev.c $(HACK_H)
mkmap.o: mkmap.c $(HACK_H) $(INCL)\sp_lev.h
mkmaze.o: mkmaze.c $(HACK_H) $(INCL)\sp_lev.h $(INCL)\lev.h
mkobj.o: mkobj.c $(HACK_H) $(INCL)\artifact.h
mkroom.o: mkroom.c $(HACK_H)
mon.o: mon.c $(HACK_H) $(INCL)\mfndpos.h
mondata.o: mondata.c $(HACK_H)
monmove.o: monmove.c $(HACK_H) $(INCL)\mfndpos.h $(INCL)\artifact.h
monst.o: monst.c $(CONFIG_H) $(INCL)\permonst.h $(INCL)\align.h \
		$(INCL)\monattk.h $(INCL)\monflag.h $(INCL)\monsym.h \
		$(INCL)\dungeon.h $(INCL)\color.h
mplayer.o: mplayer.c $(HACK_H)
mthrowu.o: mthrowu.c $(HACK_H)
muse.o: muse.c $(HACK_H)
music.o: music.c $(HACK_H) #interp.c
o_init.o: o_init.c $(HACK_H) $(INCL)\lev.h
objects.o: objects.c $(CONFIG_H) $(INCL)\obj.h $(INCL)\objclass.h \
		$(INCL)\prop.h $(INCL)\skills.h $(INCL)\color.h
objnam.o: objnam.c $(HACK_H)
options.o: options.c $(CONFIG_H) $(INCL)\objclass.h $(INCL)\flag.h \
		$(HACK_H) $(INCL)\tcap.h
pager.o: pager.c $(HACK_H) $(INCL)\dlb.h
	@$(CC) $(CFLAGS) $(SPECOPTS) /f- /Od /Fo$@ pager.c
pickup.o: pickup.c $(HACK_H)
pline.o: pline.c $(HACK_H)
polyself.o: polyself.c $(HACK_H)
potion.o: potion.c $(HACK_H)
pray.o: pray.c $(HACK_H)
priest.o: priest.c $(HACK_H) $(INCL)\mfndpos.h
quest.o: quest.c $(HACK_H) $(INCL)\qtext.h
questpgr.o: questpgr.c $(HACK_H) $(INCL)\dlb.h $(INCL)\qtext.h
read.o: read.c $(HACK_H)
rect.o: rect.c $(HACK_H)
region.o: region.c $(HACK_H)
restore.o: restore.c $(HACK_H) $(INCL)\lev.h $(INCL)\tcap.h
rip.o: rip.c $(HACK_H)
rnd.o: rnd.c $(HACK_H)
role.o: role.c $(HACK_H)
rumors.o: rumors.c $(HACK_H) $(INCL)\lev.h $(INCL)\dlb.h
save.o: save.c $(HACK_H) $(INCL)\lev.h
shk.o: shk.c $(HACK_H)
shknam.o: shknam.c $(HACK_H)
sit.o: sit.c $(HACK_H) $(INCL)\artifact.h
sounds.o: sounds.c $(HACK_H)
sp_lev.o: sp_lev.c $(HACK_H) $(INCL)\dlb.h $(INCL)\sp_lev.h
spell.o: spell.c $(HACK_H)
steal.o: steal.c $(HACK_H)
steed.o: steed.c $(HACK_H)
sys.o: sys.c $(HACK_H)
teleport.o: teleport.c $(HACK_H)
timeout.o: timeout.c $(HACK_H) $(INCL)\lev.h
topten.o: topten.c $(HACK_H) $(INCL)\dlb.h $(INCL)\patchlev.h
track.o: track.c $(HACK_H)
trap.o: trap.c $(HACK_H)
u_init.o: u_init.c $(HACK_H)
uhitm.o: uhitm.c $(HACK_H)
vault.o: vault.c $(HACK_H)
version.o: version.c $(HACK_H) $(INCL)\date.h $(INCL)\patchlev.h
vision.o: vision.c $(HACK_H) $(INCL)\vis_tab.h
weapon.o: weapon.c $(HACK_H)
were.o: were.c $(HACK_H)
wield.o: wield.c $(HACK_H)
windows.o: windows.c $(HACK_H) $(INCL)\wingem.h
wizard.o: wizard.c $(HACK_H) $(INCL)\qtext.h
worm.o: worm.c $(HACK_H) $(INCL)\lev.h
worn.o: worn.c $(HACK_H)
write.o: write.c $(HACK_H)
zap.o: zap.c $(HACK_H)

# end of file
