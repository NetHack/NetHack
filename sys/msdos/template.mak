# SCCS Id: @(#)template.mak	3.4	1996/10/25
# Copyright (c) NetHack PC Development Team 1996
#
?BEGIN?
?SCCS?
?MSC?
# PC NetHack 3.4 Makefile for Microsoft(tm) "C" >= 7.0 and MSVC >= 1.0
?ENDMSC?
?BC?
# PC NetHack 3.4 Makefile for Borland C++ 3.1.
?ENDBC?
#
# Nota Bene:	Before you get to here you should have already read
# 		the Install.dos file located in the sys/msdos directory.
?BC?
#		Additionally, you should run this makefile with the -N
#		Microsoft Compatibility option.
#
# This Makefile is for use with Borland C++ version 3.1.
#
# This Makefile is specific to Borland's MAKE which is supplied with the
# compiler.  It supports only one overlay management facility - VROOMM.
# (This Makefile won't work with make45l or NDMAKE)
?ENDBC?
?MSC?
#
# This Makefile is for use with Microsoft C version 7 and Microsoft Visual C++
# Professional Edition (MSVC) version 1.0 or greater.
#
# This Makefile is specific to Microsoft's NMAKE which is supplied with the
# more recent Microsoft C compilers.
# It supports only one overlay management facility - MOVE.
# (This Makefile won't work with make45l or NDMAKE)
#
#	In addition to your C compiler,
#
#	if you want to change		you will need a
#	files with suffix		workalike for
#	       .y			    yacc   (such as bison or byacc)
#	       .l			    lex    (such as flex)
?ENDMSC?

#
# Game Installation Variables.
# NOTE: Make sure GAMEDIR exists before nmake is started.
#

GAME	= NetHack
GAMEDIR = c:\games\nethack

#
#
# Directories
#

DAT	= ..\dat
DOC	= ..\doc
INCL	= ..\include
SRC	= ..\src
OBJ	= o
MSYS	= ..\sys\msdos
SYS	= ..\sys\share
UTIL	= ..\util
WTTY	= ..\win\tty
WSHR	= ..\win\share


#
# Compiler File Info.
# ($(MAKE) macro is often predefined, so we use $(MAKEBIN) instead.)
#

?MSC?
CC	 = cl		# Compiler
LINK	 = link		# Linker
ASM	 = masm		# Assembler (not currently needed for MSC 7 and > )
MAKEBIN  = nmake
UUDECODE = uudecode	# Unix style uudecoder
?ENDMSC?
?BC?
CC	 = bcc		# Compiler
LINK	 = tlink	# Linker
ASM	 = tasm		# Assembler (not currently needed for BC)
MAKEBIN  = make
UUDECODE = uudecode	# Unix style uudecoder

#BCTOP	 = c:\borlandc	# main Borland C++ directory
BCTOP	 = c:\bc31
?ENDBC?

#
# Yacc/Lex ... if you got 'em.
#
# If you have yacc and lex programs (or work-alike such as bison
# and flex), comment out the upper two lines below, and uncomment
# the lower two.
?BC?
#
# On Borland C++, the newest versions of flex and bison provide
# problems when run from MAKE.
?ENDBC?
#

DO_YACC = YACC_MSG
DO_LEX  = LEX_MSG
#DO_YACC  = YACC_ACT
#DO_LEX   = LEX_ACT

#
# - Specify your yacc and lex programs (or work-alikes for each) here.
#

YACC	= bison -y
#YACC   = yacc
#YACC   = byacc

LEX     = flex
#LEX    = lex

#
# - Specify your flex skeleton file (if needed).
#
FLEXSKEL =
#FLEXSKEL = -Sc:\tools16\flex.ske

#
# - Your yacc (or work-alike) output files
#
YTABC	= y_tab.c
YTABH	= y_tab.h
#YTABC  = ytab.c
#YTABH  = ytab.h

#
# - Your lex (or work-alike) output files
#
LEXYYC	= lexyy.c
#LEXYYC	= lex.yy.c

#
# Optional high-quality BSD random number generation routines
# (see pcconf.h). Set to nothing if not used.
#

RANDOM	= $(OBJ)\random.o
#RANDOM	=

#
# If TERMLIB is #defined in the source (in include\pcconf.h),
# comment out the upper line and uncomment the lower.  Make sure
# that TERMLIB contains the full pathname to the termcap library.

TERMLIB =
#TERMLIB = $(SYS)\termcap.lib

#
# MEMORY USAGE AND OVERLAYING
#
# Overlay Schema 1
#
#   - Minimal extended memory available, lots of 640K base RAM free
#     Minimize overlay turns. Requires that a minimum of
?MSC?
#     560K RAM be free as follows:
#     430K  Executable load requirement
#      60K  Overlay buffer
#      70K  for malloc() calls
#     560K  Total memory requirement
?ENDMSC?
?BC?
#     607K RAM be free as follows:
#     462K  Executable load requirement
#     115K  for malloc() calls
#      30K  Overlay buffer
#     607K  Total memory requirement
?ENDBC?
#
# Overlay Schema 2
#
#   - Favor small load size, requires extended memory for bearable performance.
#     If you have very little base 640K RAM available, but lots of extended
#     memory for caching overlays, you might try this. (eg. A machine with
#     lots of TSR's or network drivers).  Do not try to set SCHEMA = 2
#     without a disk cache and extended memory.
?BC?
#     381K  Executable load requirement
#     115K  for malloc() calls
#      30K  Overlay buffer
#     526K  Total memory requirement
?ENDBC?
?MSC?
#     360K  Executable load requirement
#      60K  Overlay buffer
#      70K  for malloc() calls
#     419K  Total memory requirement
#
# Overlay Schema 3
#
#   - Minimal extended memory available, lots of 640K base RAM free
#     Similar to schema1, but the overlay buffer is twice as large, so
#     in theory more overlays can be resident at the same time. The cost is
#     that the base memory requirement goes up considerably.
#     This requires that you obtain the moveinit.c and moveapi.h files from
#     your Microsoft C source/move directory, and place them into the src
#     directory.  Then apply the patch moveinit.pat file found in sys/msdos.
#     Requirements:
#     360K  Executable load requirement
#      95K  Overlay buffer
#      70K  for malloc() calls
#     525K  Total memory requirement
?ENDMSC?
#
?BC?
# On Borland C++, you have to make a full rebuild of all object modules each
# time you change schemas.
#
?ENDBC?

SCHEMA	= 1

#
# OPTIONAL TILE SUPPORT.
#
#	This release of NetHack allows you to build a version of NetHack
#	that will draw 16x16 color tiles on the display to represent
#	NetHack maps, objects, monsters, etc. on machines with appropriate
#	display hardware.  Currently the only supported video hardware is
#	VGA.
#
#	Note:  You can build NetHack with tile support and then choose
#	whether to use it or not at runtime via the defaults.nh file option
#	"video".
#

TILESUPPORT = Y

#
#  C COMPILER AND LINKER SETTINGS
#
#   For debugging ability, comment out the upper three
#   macros and uncomment the lower three.  You can also
#   uncomment only either LDFLAGSU or LDFLAGSN if you
#   want to include debug information only in the utilities
#   or only in the game file.
?BC?

#   On Borland C++, you cannot include debug information for
#   all the object modules because the linker cannot handle
#   it.
?ENDBC?

#CDFLAGS  =
?MSC?
#LDFLAGSN =
?ENDMSC?
?BC?
LDFLAGSN  =
?ENDBC?
#LDFLAGSU =

?MSC?
CDFLAGS   = /Zi			# use debug info (compiler)
LDFLAGSN  = /CO			# use debug info (linker - game)
LDFLAGSU  = /CO			# use debug info (linker - utilities)
?ENDMSC?
?BC?
CDFLAGS	  = -v -vi		# use debug info (compiler)
#LDFLAGSN = /v			# use debug info (linker - game)
LDFLAGSU  = /v			# use debug info (linker - utilities)
?ENDBC?

#
?MSC?
# - Force a change in the C warning level for all builds.
#   (Its W0 setting in the CL environment variable will take
#   precedence if left blank here).
?ENDMSC?
?BC?
# - Don't warn about unreachable code because flex generates a whole bunch
#   of unreachable code warnings, which stops the compile process.
?ENDBC?
#

?MSC?
CW =
#CW =/W3
?ENDMSC?
?BC?
CW = -w-rch
?ENDBC?

#
#   Select whether to use pre-compiled headers or not.
#   Set PRECOMPHEAD to Y to use pre-compiled headers, set it to anything
#   else and pre-compiled headers will not be used.
#   (Pre-compiled headers speed up compiles, but require a bit more
#   disk space during the build.  The pre-compiled headers can be deleted
#   afterwards via DEL *.PCH if desired).
#

PRECOMPHEAD = N

#
#   C Compiler Flags
#
?MSC?
# Note:
#
#    CL environment variable should already be set to:
#    CL= /AL /G2 /Oo /Gs /Gt16 /Zp1 /W0 /I..\include /nologo /DMOVERLAY
#
?ENDMSC?

?MSC?
CFLAGS = /c
?ENDMSC?
?BC?
CFLAGS = -c
?ENDBC?

#  Uncomment the line below if you want to store all the level files,
#  help files, etc. in a single library file (recommended).

USE_DLB = Y

#
########################################################################
########################################################################
#
#  Nothing below here should have to be changed.
#
########################################################################
########################################################################
#
#  Warning:
#
#  Changing anything below here means that you should be *very*
#  familiar with your compiler's workings, *very* knowledgeable
#  about the overlay structure and mechanics of NetHack, and *very*
#  confident in your understanding of Makefiles and Make utilities.
#
########################################################################
#
# Default Make Procedure
#

default: $(GAME)

#
########################################################################
# Tile preparation
#

! IF ("$(TILESUPPORT)"=="Y")

TILEGAME  = $(OBJ)\tile.o	$(OBJ)\pctiles.0	$(OBJ)\pctiles.b

#
#   -  VGA Tile Support, uncomment these three lines.
#

TILEVGA    = $(OBJ)\vidvga.0 $(OBJ)\vidvga.1 $(OBJ)\vidvga.2 $(OBJ)\vidvga.b
PLANAR_TIB = NetHack1.tib
OVERVIEW_TIB = NetHacko.tib

#
# Leave this line uncommented and unchanged.
TILEUTIL  =  $(TILEGAME) $(TILEVGA) $(UTIL)\tile2bin.exe $(UTIL)\til2bin2.exe \
		 $(PLANAR_TIB) $(OVERVIEW_TIB)

! ENDIF

! IF ("$(USE_DLB)"=="Y")
DLB = nhdat
! ELSE
DLB =
! ENDIF

#
#############################################################################
#
# General Overlay Schema Settings
#

?MSC?
LNKOPT  = schema$(SCHEMA).def
?ENDMSC?
?BC?
!include schema$(SCHEMA).bc
OVLINIT =$(OBJ)\ovlinit.o
?ENDBC?

?MSC?
#
# - Specific Overlay Schema Settings
#

! IF ($(SCHEMA)==1)
INTOVL = /DYNAMIC:1250 /NOE
OVLINIT =
! ENDIF

! IF ($(SCHEMA)==2)
INTOVL = /DYNAMIC:1380	/NOE
OVLINIT =
! ENDIF

! IF ($(SCHEMA)==3)
INTOVL = /DYNAMIC:1170
OVLINIT = $(OBJ)\moveinit.o $(OBJ)\ovlinit.o
! ENDIF
?ENDMSC?

#
#############################################################################
#
# C Compiler and Linker Setup Options
# (To Maintainer; modify only if absolutely necessary)
#

?BC?
BCINCL	 = $(BCTOP)\include	# include directory for main BC headers
BCLIB	 = $(BCTOP)\lib		# library directory for main BC libraries
BCCFG	 = nethack.cfg		# name of the nethack configuration file
?ENDBC?

#
# Model
#

?MSC?
MODEL	 = L
?ENDMSC?
?BC?
MODEL	 = h
?ENDBC?

#
# - Optional C library specifier for those with non-standard
#   libraries or a multiple-target library setup.
#

CLIB    =
?MSC?
#CLIB    = llibcer /nod
?ENDMSC?

?BC?
#
# Borland C++ libraries
#

BCOVL	= $(BCLIB)\OVERLAY
BCMDL	= $(BCLIB)\C$(MODEL)

?ENDBC?
#
# Compiler Options
#

?MSC?
CNOLNK	= /c			# just generate .OBJ
CPCHUSE	= /YuHACK.H		# use precompiled headers
CPCHGEN	= /YcHACK.H		# generate precompiled headers
CPCHNAM	= /Fp			# set the name of the precompiled header file
CPCHEXT = .PCH			# precompiled header extension
CDEFINE	= /D			# define a macro
CCSNAM	= /NT			# set the code segment name
COBJNAM	= /Fo			# name the .OBJ file
CNOOPT  = /f- /Od		# disable optimizations (must be first in line)
				# /f- = don't use the "fast" compiler,its buggy
?ENDMSC?
?MSCMACRO:CSNAMOA= ?
?MSCMACRO:CSNAMOB=$(CCSNAM)$(@F) ?
?MSCMACRO:CSNAM0=$(CCSNAM)$(@F) ?
?MSCMACRO:CSNAM1=$(CCSNAM)$(@F) ?
?MSCMACRO:CSNAM2=$(CCSNAM)$(@F) ?
?MSCMACRO:CSNAM3=$(CCSNAM)$(@F) ?
?MSCMACRO:CSNAMB=$(CCSNAM)$(@F) ?
?BC?
CNOLNK	= -c			# just generate .OBJ
CPCHUSE	= -Hu			# use precompiled headers
CPCHGEN	= -H			# generate precompiled headers
CPCHNAM	= -H=			# set the name of the precompiled header file
CPCHEXT = .PCH			# precompiled header extension
CDEFINE	= -D			# define a macro
CSTKSZ	= -DSTKSIZ=		# set stack size
CCSNAM	= -zC			# set the code segment name
COBJNAM	= -o			# name the .OBJ file
?ENDBC?
?BCMACRO:CSNAMOA=$$($(@B)_o) ?
?BCMACRO:CSNAMOB=$$($(@B)_o) ?
?BCMACRO:CSNAM0=$$($(@B)_0) ?
?BCMACRO:CSNAM1=$$($(@B)_1) ?
?BCMACRO:CSNAM2=$$($(@B)_2) ?
?BCMACRO:CSNAM3=$$($(@B)_3) ?
?BCMACRO:CSNAMB=$$($(@B)_b) ?

#
# Linker Options
#

?MSC?
LWCASE	= /NOI			# treat case as significant
LMAP	= /MAP			# create map file
LSTKSZ	= /ST:			# set stack size
LMAXSEG	= /SE:400		# maximum number of segments allowed
LMAXALL	= /CPARM:1		# maximum program memory allocation (?)
LINFO	= /INFO			# display link information while processing
?ENDMSC?
?BC?
LWCASE	= /c			# treat case as significant
LMAP	= /m			# create map file
LINIT	= $(BCLIB)\C0$(MODEL)	# initialization object file
LOVL	= /oOVLY		# overlay all needed segments
?ENDBC?

#
# Stack Sizes
#

STKSUTL	= 4096			# Utilities Stack Size
STKSNRM = 5120			# Normal Stack Size

?MSC?
LUSTACK	= $(LSTKSZ)$(STKSUTL)	# Utilities Stack Set for Linker
LNSTACK	= $(LSTKSZ)$(STKSNRM)	# Normal Stack Set for Linker
?ENDMSC?
?BC?
CUSTACK	= $(CSTKSZ)$(STKSUTL)	# Utilities Stack Set for Compiler
CNSTACK	= $(CSTKSZ)$(STKSNRM)	# Normal Stack Set for Compiler
?ENDBC?


#
########################################################################
# DLB preparation
#

! IF ("$(USE_DLB)"=="Y")
DLBFLG = $(CDEFINE)DLB
! ELSE
DLBFLG =
! ENDIF

#
########################################################################
# tile preparation
#

! IF ("$(TILESUPPORT)"=="Y")
TILFLG = $(CDEFINE)USE_TILES
! ELSE
TILFLG =
! ENDIF

#############################################################################
#
# Overlay switches
#

COVL0	= $(CDEFINE)OVL0
COVL1	= $(CDEFINE)OVL1
COVL2	= $(CDEFINE)OVL2
COVL3	= $(CDEFINE)OVL3
COVLB	= $(CDEFINE)OVLB

#
# Flags
#

FLAGOPT = $(DLBFLG) $(TILFLG)

#
# Precompiled Header Section
#

?BC?
#common options (placed in $(BCCFG))
CFLGTOT = $(CDFLAGS) $(CFLAGS) $(FLAGOPT) $(CW)
#util builds
CFLAGSU	= +$(BCCFG) $(CUSTACK)
#normal build, no PCH
CFLAGSN = +$(BCCFG) $(CNSTACK)
?ENDBC?
?MSC?
#util builds
CFLAGSU	= $(CDFLAGS) $(CFLAGS) $(CW) $(FLAGOPT) $(CUSTACK)
#normal build, no PCH
CFLAGSN = $(CDFLAGS) $(CFLAGS) $(CW) $(FLAGOPT) $(CNSTACK)
?ENDMSC?
#no optimizations
CFLAGNO = $(CNOOPT) $(CFLAGSN)

! IF ("$(PRECOMPHEAD)"!="Y")

CFLAGCO = $(COVLO)
CFLAGUO = $(COVLO)
CFLAGC0 = $(COVL0)
CFLAGU0 = $(COVL0)
CFLAGC1 = $(COVL1)
CFLAGU1 = $(COVL1)
CFLAGC2 = $(COVL2)
CFLAGU2 = $(COVL2)
CFLAGC3 = $(COVL3)
CFLAGU3 = $(COVL3)
CFLAGCB = $(COVLB)
CFLAGUB = $(COVLB)
PCHO =
PCH0 =
PCH1 =
PCH2 =
PCH3 =
PCHB =

precomp.msg:
	@echo Not using precompiled headers...

! ELSE

# .o files
CFLAGUO	= $(CPCHUSE) $(CPCHNAM)PHO$(CPCHEXT) $(COVLO)
CFLAGCO	= $(CPCHGEN) $(CPCHNAM)PHO$(CPCHEXT) $(COVLO)
PCHO = PHO$(CPCHEXT)
# .0 files
CFLAGU0	= $(CPCHUSE) $(CPCHNAM)PH0$(CPCHEXT) $(COVL0)
CFLAGC0	= $(CPCHGEN) $(CPCHNAM)PH0$(CPCHEXT) $(COVL0)
PCH0 = PH0$(CPCHEXT)
# .1 files
CFLAGU1	= $(CPCHUSE) $(CPCHNAM)PH1$(CPCHEXT) $(COVL1)
CFLAGC1	= $(CPCHGEN) $(CPCHNAM)PH1$(CPCHEXT) $(COVL1)
PCH1 = PH1$(CPCHEXT)
# .2 files
CFLAGU2	= $(CPCHUSE) $(CPCHNAM)PH2$(CPCHEXT) $(COVL2)
CFLAGC2	= $(CPCHGEN) $(CPCHNAM)PH2$(CPCHEXT) $(COVL2)
PCH2 = PH2$(CPCHEXT)
# .3 files
CFLAGU3	= $(CPCHUSE) $(CPCHNAM)PH3$(CPCHEXT) $(COVL3)
CFLAGC3	= $(CPCHGEN) $(CPCHNAM)PH3$(CPCHEXT) $(COVL3)
PCH3 = PH3$(CPCHEXT)
# .B files
CFLAGUB	= $(CPCHUSE) $(CPCHNAM)PHB$(CPCHEXT) $(COVLB)
CFLAGCB	= $(CPCHGEN) $(CPCHNAM)PHB$(CPCHEXT) $(COVLB)
PCHB = PHB$(CPCHEXT)

precomp.msg:
	@echo Using precompiled headers...

! ENDIF


?BC?
FLAGCO  = $(CNSTACK) +CFLAGCO.CFG
FLAGUO  = $(CNSTACK) +CFLAGUO.CFG
FLAGC0  = $(CNSTACK) +CFLAGC0.CFG
FLAGU0  = $(CNSTACK) +CFLAGU0.CFG
FLAGC1  = $(CNSTACK) +CFLAGC1.CFG
FLAGU1  = $(CNSTACK) +CFLAGU1.CFG
FLAGC2  = $(CNSTACK) +CFLAGC2.CFG
FLAGU2  = $(CNSTACK) +CFLAGU2.CFG
FLAGC3  = $(CNSTACK) +CFLAGC3.CFG
FLAGU3  = $(CNSTACK) +CFLAGU3.CFG
FLAGCB  = $(CNSTACK) +CFLAGCB.CFG
FLAGUB  = $(CNSTACK) +CFLAGUB.CFG
?ENDBC?
?MSC?
FLAGCO  = $(CFLAGSN) $(CFLAGCO)
FLAGUO  = $(CFLAGSN) $(CFLAGUO)
FLAGC0  = $(CFLAGSN) $(CFLAGC0)
FLAGU0  = $(CFLAGSN) $(CFLAGU0)
FLAGC1  = $(CFLAGSN) $(CFLAGC1)
FLAGU1  = $(CFLAGSN) $(CFLAGU1)
FLAGC2  = $(CFLAGSN) $(CFLAGC2)
FLAGU2  = $(CFLAGSN) $(CFLAGU2)
FLAGC3  = $(CFLAGSN) $(CFLAGC3)
FLAGU3  = $(CFLAGSN) $(CFLAGU3)
FLAGCB  = $(CFLAGSN) $(CFLAGCB)
FLAGUB  = $(CFLAGSN) $(CFLAGUB)
?ENDMSC?

# End of Pre-compiled header section
#===========================================================================

?MSC?
#
# Controls whether MOVE tracing is enabled in the executable
# This should be left commented unless you are tinkering with the
# overlay structure of NetHack.  The executable runs _very_
# slowly when the movetr.lib is linked in.
#

#MOVETR= movetr.lib

# do not change this
! IF ("$(MOVETR)"!="")
MVTRCL = $(CDEFINE)MOVE_PROF
! ELSE
MVTRCL =
! ENDIF

?ENDMSC?
#
# Linker options for building various things.
#

LFLAGSU	= $(LDFLAGSU) $(LUSTACK) $(LINIT)
LFLAGSN	= $(LDFLAGSN) $(LNSTACK) $(LWCASE) $(LMAXSEG) $(INTOVL) $(LMAXALL) \
	  $(LINFO) $(LINIT) $(LOVL)

#
# Make Roolz dude.
# Due to the inadequacy of some makes these must accord with a
# topological sort of the generated-from relation... output on
# the left, input on the right. Trust me.
#

.SUFFIXES:  .exe .0 .1 .2 .3 .B .o .til .uu .c .y .l

#
# Rules for files in src
#


.c{$(OBJ)}.o:
	@$(CC) $(FLAGUO) ?[CSNAMOB]$(COBJNAM)$@ $<

{$(SRC)}.c{$(OBJ)}.o:
	$(CC) $(FLAGUO) ?[CSNAMOB]$(COBJNAM)$@  $<

{$(SRC)}.c{$(OBJ)}.0:
	$(CC) $(FLAGU0) ?[CSNAM0]$(COBJNAM)$@ $<

{$(SRC)}.c{$(OBJ)}.1:
	$(CC) $(FLAGU1) ?[CSNAM1]$(COBJNAM)$@ $<

{$(SRC)}.c{$(OBJ)}.2:
	$(CC) $(FLAGU2) ?[CSNAM2]$(COBJNAM)$@ $<

{$(SRC)}.c{$(OBJ)}.3:
	$(CC) $(FLAGU3) ?[CSNAM3]$(COBJNAM)$@ $<

{$(SRC)}.c{$(OBJ)}.B:
	$(CC) $(FLAGUB) ?[CSNAMB]$(COBJNAM)$@ $<

#
# Rules for files in sys\share
#

{$(SYS)}.c{$(OBJ)}.o:
	$(CC) $(FLAGUO) ?[CSNAMOA]$(COBJNAM)$@  $<

{$(SYS)}.c{$(OBJ)}.0:
	$(CC) $(FLAGU0) ?[CSNAM0]$(COBJNAM)$@ $<

{$(SYS)}.c{$(OBJ)}.1:
	$(CC) $(FLAGU1) ?[CSNAM1]$(COBJNAM)$@ $<

{$(SYS)}.c{$(OBJ)}.2:
	$(CC) $(FLAGU2) ?[CSNAM2]$(COBJNAM)$@ $<

{$(SYS)}.c{$(OBJ)}.3:
	$(CC) $(FLAGU3) ?[CSNAM3]$(COBJNAM)$@ $<

{$(SYS)}.c{$(OBJ)}.B:
	$(CC) $(FLAGUB) ?[CSNAMB]$(COBJNAM)$@ $<

#
# Rules for files in sys\msdos
#

{$(MSYS)}.c{$(OBJ)}.o:
	$(CC) $(FLAGUO) ?[CSNAMOA]$(COBJNAM)$@  $<

{$(MSYS)}.c{$(OBJ)}.0:
	$(CC) $(FLAGU0) ?[CSNAM0]$(COBJNAM)$@ $<

{$(MSYS)}.c{$(OBJ)}.1:
	$(CC) $(FLAGU1) ?[CSNAM1]$(COBJNAM)$@ $<

{$(MSYS)}.c{$(OBJ)}.2:
	$(CC) $(FLAGU2) ?[CSNAM2]$(COBJNAM)$@ $<

{$(MSYS)}.c{$(OBJ)}.3:
	$(CC) $(FLAGU3) ?[CSNAM3]$(COBJNAM)$@ $<

{$(MSYS)}.c{$(OBJ)}.B:
	$(CC) $(FLAGUB) ?[CSNAMB]$(COBJNAM)$@ $<

{$(MSYS)}.h{$(INCL)}.h:
	@copy $< $@

#
# Rules for files in util
#

{$(UTIL)}.c{$(OBJ)}.o:
	$(CC) $(CFLAGSU) ?[CSNAMOB]$(COBJNAM)$@  $<

#
# Rules for files in win\share
#

{$(WSHR)}.c.o:
	@$(CC) $(FLAGUO) ?[CSNAMOA]$(COBJNAM)$@ $<

{$(WSHR)}.c{$(OBJ)}.o:
	@$(CC) $(FLAGUO) ?[CSNAMOA]$(COBJNAM)$@ $<

{$(WSHR)}.h{$(INCL)}.h:
	@copy $< $@

{$(WSHR)}.txt{$(DAT)}.txt:
	@copy $< $@

#
# Rules for files in win\tty
#

{$(WTTY)}.c{$(OBJ)}.o:
	$(CC) $(FLAGUO) ?[CSNAMOA]$(COBJNAM)$@  $<

{$(WTTY)}.c{$(OBJ)}.0:
	$(CC) $(FLAGU0) ?[CSNAM0]$(COBJNAM)$@ $<

{$(WTTY)}.c{$(OBJ)}.1:
	$(CC) $(FLAGU1) ?[CSNAM1]$(COBJNAM)$@ $<

{$(WTTY)}.c{$(OBJ)}.2:
	$(CC) $(FLAGU2) ?[CSNAM2]$(COBJNAM)$@ $<

{$(WTTY)}.c{$(OBJ)}.3:
	$(CC) $(FLAGU3) ?[CSNAM3]$(COBJNAM)$@ $<

{$(WTTY)}.c{$(OBJ)}.B:
	$(CC) $(FLAGUB) ?[CSNAMB]$(COBJNAM)$@ $<

#
# NETHACK OBJECTS
#
# This section creates shorthand macros for many objects
# referenced later on in the Makefile.
#

#
# Shorten up the location for some files
#

O  = $(OBJ)\				# comment so \ isn't last char

U  = $(UTIL)\				# comment so \ isn't last char

#
# Utility Objects.
#

MAKESRC        = $(U)makedefs.c

SPLEVSRC       = $(U)lev_yacc.c	$(U)lev_$(LEX).c $(U)lev_main.c  $(U)panic.c

DGNCOMPSRC     = $(U)dgn_yacc.c	$(U)dgn_$(LEX).c $(U)dgn_main.c

MAKEOBJS       = $(O)makedefs.o	$(O)monst.o	$(O)objects.o

?LIST:SPLEVOBJS?
		 $(O)lev_yacc.o	$(O)lev_$(LEX).o $(O)lev_main.o
		 $(O)alloc.o	$(O)decl.o	$(O)drawing.o
		 $(O)monst.o	$(O)objects.o	$(O)panic.o	$(O)stubvid.o
?ENDLIST?

?LIST:DGNCOMPOBJS?
		 $(O)dgn_yacc.o	$(O)dgn_$(LEX).o $(O)dgn_main.o
		 $(O)alloc.o	$(O)panic.o
?ENDLIST?

RECOVOBJS      = $(O)recover.o

?LIST:GIFREADERS?
		 $(O)gifread.o	$(O)alloc.o	$(O)panic.o
?ENDLIST?

?LIST:TEXT_IO?
		 $(O)tiletext.o $(O)tiletxt.o	$(O)drawing.o
		 $(O)decl.o	$(O)monst.o	$(O)objects.o
		 $(O)stubvid.o
?ENDLIST?

PPMWRITERS     = $(O)ppmwrite.o	$(O)alloc.o	$(O)panic.o

?LIST:GIFREAD2?
		 $(O)gifread2.o	$(O)alloc.o	$(O)panic.o
?ENDLIST?

?LIST:TEXT_IO2?
		 $(O)tiletex2.o $(O)tiletxt2.o	$(O)drawing.o
		 $(O)decl.o	$(O)monst.o	$(O)objects.o
		 $(O)stubvid.o
?ENDLIST?

PPMWRIT2       = $(O)ppmwrit2.o $(O)alloc.o	$(O)panic.o

TILEFILES      = $(WSHR)\monsters.txt $(WSHR)\objects.txt $(WSHR)\other.txt

TILEFILES2     = $(WSHR)\monthin.txt $(WSHR)\objthin.txt $(WSHR)\oththin.txt

DLBOBJS        = $(O)dlb_main.o $(O)dlb.o $(O)alloc.o $(O)panic.o

#
#  Object files for the game itself.
#

OBJ01 =	$(O)alloc.o	$(RANDOM)	$(O)decl.o     	$(O)objects.o	\
	$(O)muse.o	$(O)display.o	$(O)vision.o	\
	$(O)rect.o	$(O)vis_tab.o	$(O)monst.o	$(O)wintty.o	\
	$(O)files.o	$(O)sys.o	$(O)monstr.o	$(O)minion.o	\
	$(O)worm.o	$(O)detect.o 	$(O)exper.o	$(O)mplayer.o	\
	$(O)uhitm.o	$(O)pager.o 	$(O)windows.o	$(O)quest.o	\
	$(O)questpgr.o 	$(O)write.o	$(O)drawing.o	$(O)dokick.o	\
	$(O)dothrow.o 	$(O)pickup.o	$(O)pray.o	$(O)spell.o 	\
	$(O)ball.o	$(O)wield.o	$(O)worn.o	$(O)fountain.o	\
	$(O)music.o	$(O)rumors.o	$(O)dlb.o	$(O)sit.o 	\
	$(O)bones.o	$(O)mklev.o	$(O)save.o	$(O)restore.o 	\
	$(O)mkmaze.o	$(O)mkmap.o	$(O)end.o	$(O)o_init.o	\
	$(O)options.o	$(O)rip.o       $(O)sound.o	$(O)teleport.o	\
	$(O)topten.o	$(O)tty.o	$(O)u_init.o	$(O)extralev.o 	\
	$(O)sp_lev.o	$(O)dig.o	$(O)pckeys.o	$(O)role.o	\
	$(O)steed.o	$(O)region.o

OVL0 =	$(O)allmain.0	$(O)apply.0	$(O)artifact.0	$(O)attrib.0  \
	$(O)botl.0	$(O)cmd.0	$(O)dbridge.0	$(O)do.0      \
	$(O)do_name.0	$(O)do_wear.0	$(O)dogmove.0	$(O)dungeon.0 \
	$(O)eat.0	$(O)engrave.0	$(O)hacklib.0	$(O)invent.0  \
	$(O)lock.0	$(O)pcmain.0	$(O)mail.0	$(O)makemon.0 \
	$(O)mcastu.0	$(O)mhitm.0	$(O)mhitu.0	$(O)mkobj.0   \
	$(O)mkroom.0	$(O)mon.0	$(O)mondata.0	$(O)monmove.0 \
	$(O)mthrowu.0	$(O)objnam.0	$(O)polyself.0	$(O)priest.0  \
	$(O)rnd.0	$(O)shknam.0	$(O)sounds.0	$(O)steal.0   \
	$(O)timeout.0	$(O)track.0	$(O)trap.0	$(O)vault.0   \
	$(O)weapon.0	$(O)were.0	$(O)wizard.0	$(O)msdos.0   \
	$(O)termcap.0	$(O)video.0	$(O)vidtxt.0	$(O)zap.0     \
	$(O)explode.0	$(O)shk.0

OVL1 =	$(O)allmain.1	$(O)apply.1	$(O)artifact.1	$(O)attrib.1 \
	$(O)botl.1	$(O)cmd.1	$(O)dbridge.1	$(O)do.1     \
	$(O)do_wear.1	$(O)dog.1	$(O)dungeon.1	$(O)eat.1    \
	$(O)engrave.1	$(O)hack.1	$(O)hacklib.1	$(O)invent.1 \
	$(O)makemon.1	$(O)mhitu.1	$(O)mkobj.1	$(O)mon.1    \
	$(O)mondata.1	$(O)monmove.1	$(O)mthrowu.1	$(O)objnam.1 \
	$(O)pcmain.1	$(O)polyself.1	$(O)rnd.1	$(O)shk.1    \
	$(O)steal.1	$(O)timeout.1	$(O)track.1	$(O)trap.1   \
	$(O)weapon.1	$(O)getline.1	$(O)termcap.1	$(O)topl.1   \
	$(O)video.1	$(O)zap.1	$(O)explode.1

OVL2 =	$(O)attrib.2	$(O)do.2	$(O)do_name.2	$(O)do_wear.2 \
	$(O)dog.2	$(O)engrave.2	$(O)hack.2	$(O)hacklib.2 \
	$(O)invent.2	$(O)makemon.2	$(O)mon.2	$(O)mondata.2 \
	$(O)monmove.2	$(O)getline.2	$(O)shk.2	$(O)topl.2    \
	$(O)trap.2	$(O)zap.2

OVL3 =	$(O)do.3	$(O)hack.3	$(O)invent.3	$(O)light.3   \
	$(O)shk.3	$(O)trap.3	$(O)zap.3


OVLB =	$(O)allmain.B	$(O)apply.B	$(O)artifact.B	$(O)attrib.B	\
	$(O)botl.B	$(O)cmd.B	$(O)dbridge.B	$(O)do.B	\
	$(O)do_name.B	$(O)do_wear.B	$(O)dog.B	$(O)dogmove.B	\
	$(O)eat.B	$(O)engrave.B	$(O)hack.B	$(O)hacklib.B	\
	$(O)invent.B	$(O)lock.B	$(O)mail.B	$(O)makemon.B	\
	$(O)mcastu.B	$(O)mhitm.B	$(O)mhitu.B	$(O)mkobj.B	\
	$(O)mkroom.B	$(O)mon.B	$(O)mondata.B	$(O)monmove.B	\
	$(O)mthrowu.B	$(O)objnam.B	$(O)pcmain.B	$(O)pline.B	\
	$(O)polyself.B	$(O)potion.B	$(O)priest.B	$(O)read.B	\
	$(O)rnd.B	$(O)shk.B	$(O)shknam.B	$(O)sounds.B	\
	$(O)steal.B	$(O)timeout.B	$(O)track.B	$(O)trap.B	\
	$(O)vault.B	$(O)weapon.B	$(O)were.B	$(O)wizard.B	\
	$(O)msdos.B	$(O)pcunix.B	$(O)termcap.B	$(O)topl.B	\
	$(O)video.B	$(O)vidtxt.B	$(O)zap.B

TILOBJ = $(TILEGAME) $(TILEVGA)

VVOBJ =	$(O)version.o

NVOBJ = $(OBJ01)	$(OVL0)		$(OVL1)		$(OVL2) \
	$(OVL3) 	$(OVLB)		$(TILOBJ)

ALLOBJ= $(NVOBJ) $(VVOBJ) $(OVLINIT)

#
# Header objects
#

# This comment copied from sys/unix/Makefile.src,
# extern.h is ignored, even though its declared function types may affect the
# compilation of all the .c files, since extern.h changes every time the
# type of an external function does, and we would spend all our time recompiling
# if we did not ignore it.
#EXTERN_H    = $(INCL)\extern.h
EXTERN_H    =
PCCONF_H    = $(INCL)\pcconf.h $(INCL)\micro.h $(INCL)\system.h
PERMONST_H  = $(INCL)\monattk.h $(INCL)\monflag.h $(INCL)\align.h
YOUPROP_H   = $(INCL)\prop.h $(PERMONST_H) $(INCL)\pm.h $(INCL)\youprop.h \
	      $(INCL)\mondata.h
YOU_H	    = $(INCL)\attrib.h $(INCL)\monst.h $(YOUPROP_H) $(INCL)\align.h
DECL_H      = $(INCL)\quest.h $(INCL)\spell.h $(INCL)\color.h \
	      $(INCL)\obj.h $(YOU_H) $(INCL)\onames.h $(INCL)\pm.h

CONFIG_H    = $(INCL)\tradstdc.h $(INCL)\coord.h $(PCCONF_H) $(INCL)\config.h
HACK_H      = $(CONFIG_H) $(INCL)\dungeon.h $(INCL)\align.h $(INCL)\monsym.h \
              $(INCL)\mkroom.h $(INCL)\objclass.h $(DECL_H) \
	      $(INCL)\timeout.h $(INCL)\trap.h $(INCL)\flag.h $(INCL)\rm.h \
	      $(INCL)\vision.h $(INCL)\mondata.h $(INCL)\wintype.h \
	      $(INCL)\engrave.h $(INCL)\rect.h $(EXTERN_H) \
	      $(INCL)\winprocs.h $(INCL)\trampoli.h $(INCL)\display.h
TILE_H      = $(INCL)\tile.h $(INCL)\pctiles.h
PCVIDEO_H   = $(INCL)\portio.h $(INCL)\pcvideo.h
ALIGN_H     = $(INCL)\align.h
ARTIFACT_H  = $(INCL)\artifact.h
ARTILIST_H  = $(INCL)\artilist.h
COLOR_H     = $(INCL)\color.h
DATE_H      = $(INCL)\date.h
DGN_FILE_H  = $(INCL)\dgn_file.h
DLB_H	    = $(INCL)\dlb.h
EMIN_H      = $(INCL)\emin.h
EPRI_H      = $(INCL)\epri.h
ESHK_H      = $(INCL)\eshk.h
EDOG_H      = $(INCL)\edog.h
FUNC_TAB_H  = $(INCL)\func_tab.h
LEV_H       = $(INCL)\lev.h
LEV_COMP_H  = $(INCL)\lev_comp.h
MAIL_H      = $(INCL)\mail.h
MFNDPOS_H   = $(INCL)\mfndpos.h
MONSYM_H    = $(INCL)\monsym.h
OBJ_H       = $(INCL)\obj.h
OBJCLASS_H  = $(INCL)\objclass.h
OBJECTS_H   = $(INCL)\objects.h
PROP_H      = $(INCL)\prop.h
QTEXT_H     = $(INCL)\qtext.h
QUEST_H     = $(INCL)\quest.h
SP_LEV_H    = $(INCL)\sp_lev.h
TERMCAP_H   = $(INCL)\tcap.h
VAULT_H     = $(INCL)\vault.h
VIS_TAB_H   = $(INCL)\vis_tab.h
WINTTY_H    = $(INCL)\wintty.h

#
# In the unix distribution this file is patchlevel.h, make it 8.3 here
# to avoid an nmake warning under dos.
#

PATCHLEVEL_H   = $(INCL)\patchlev.h


#
#  The name of the game.
#

GAMEFILE = $(GAMEDIR)\$(GAME).exe

#
# make data.base an 8.3 filename to prevent an nmake warning
#

DATABASE = $(DAT)\data.bas

#######################################################################
#
#  TARGETS

#
#  The main target.
#

$(GAME): obj.tag envchk $(U)utility.tag $(GAMEFILE)
	@echo $(GAME) is up to date.

#
#  Everything
#

all :	install

install: $(GAME) install.tag
	@echo Done.


install.tag: 	$(DAT)\data	$(DAT)\rumors	$(DAT)\dungeon \
	 	$(DAT)\oracles	$(DAT)\quest.dat $(DAT)\sp_lev.tag $(DLB)
! IF ("$(USE_DLB)"=="Y")
	copy nhdat                $(GAMEDIR)
	copy $(DAT)\license       $(GAMEDIR)
! ELSE
	copy $(DAT)\*.            $(GAMEDIR)
	copy $(DAT)\*.dat         $(GAMEDIR)
	copy $(DAT)\*.lev         $(GAMEDIR)
	copy $(MSYS)\msdoshlp.txt $(GAMEDIR)
	if exist $(GAMEDIR)\makefile del $(GAMEDIR)\makefile
! ENDIF
	copy $(SYS)\termcap       $(GAMEDIR)
	if exist $(DOC)\guideb*.txt copy $(DOC)\guideb*.txt  $(GAMEDIR)
	if exist $(DOC)\nethack.txt copy $(DOC)\nethack.txt  $(GAMEDIR)\NetHack.txt
	if exist $(DOC)\recover.txt copy $(DOC)\recover.txt  $(GAMEDIR)
	copy $(SYS)\nethack.cnf   $(GAMEDIR)\defaults.nh
	copy $(U)recover.exe  $(GAMEDIR)
	if exist *.tib copy *.tib $(GAMEDIR)
	echo install done > $@

$(DAT)\sp_lev.tag: $(U)utility.tag $(DAT)\bigroom.des  $(DAT)\castle.des \
	$(DAT)\endgame.des $(DAT)\gehennom.des $(DAT)\knox.des   \
	$(DAT)\medusa.des  $(DAT)\oracle.des   $(DAT)\tower.des  \
	$(DAT)\yendor.des  $(DAT)\arch.des     $(DAT)\barb.des   \
	$(DAT)\caveman.des $(DAT)\elf.des      $(DAT)\healer.des \
	$(DAT)\knight.des  $(DAT)\priest.des   $(DAT)\rogue.des  \
	$(DAT)\samurai.des $(DAT)\tourist.des  $(DAT)\valkyrie.des \
	$(DAT)\wizard.des
	cd $(DAT)
	$(U)lev_comp bigroom.des
	$(U)lev_comp castle.des
	$(U)lev_comp endgame.des
	$(U)lev_comp gehennom.des
	$(U)lev_comp knox.des
	$(U)lev_comp mines.des
	$(U)lev_comp medusa.des
	$(U)lev_comp oracle.des
	$(U)lev_comp tower.des
	$(U)lev_comp yendor.des
	$(U)lev_comp arch.des
	$(U)lev_comp barb.des
	$(U)lev_comp caveman.des
	$(U)lev_comp elf.des
	$(U)lev_comp healer.des
	$(U)lev_comp knight.des
	$(U)lev_comp priest.des
	$(U)lev_comp rogue.des
	$(U)lev_comp samurai.des
	$(U)lev_comp tourist.des
	$(U)lev_comp valkyrie.des
	$(U)lev_comp wizard.des
	cd $(SRC)
	echo sp_levs done > $(DAT)\sp_lev.tag

$(U)utility.tag: envchk			$(INCL)\date.h	$(INCL)\onames.h \
		$(INCL)\pm.h 		$(SRC)\monstr.c	$(SRC)\vis_tab.c \
		$(U)lev_comp.exe	$(VIS_TAB_H) 	$(U)dgn_comp.exe \
		$(U)recover.exe		$(TILEUTIL)
             @echo utilities made >$@
	     @echo utilities made.

tileutil: $(U)gif2txt.exe $(U)txt2ppm.exe
	@echo Optional tile development utilities are up to date.

?MSC?
#  The section for linking the NetHack image looks a little strange at
#  first, especially if you are used to UNIX makes, or NDMAKE.  It is
#  Microsoft nmake specific, and it gets around the problem of the link
#  command line being too long for the linker.  An "in-line" linker
#  response file is generated temporarily.
#
#  It takes advantage of the following features of nmake:
?ENDMSC?
#
#  Inline files :
#			Specifying the "<<" means to start an inline file.
#                 	Another "<<" at the start of a line closes the
#                 	inline file.
#
?MSC?
#  Substitution within Macros:
#                       $(mymacro:string1=string2) replaces every
#                       occurrence of string1 with string2 in the
#                       macro mymacro.  Special ascii key codes may be
#                       used in the substitution text by preceding it
#                       with ^ as we have done below.  Every occurrence
#                       of a <tab> in $(ALLOBJ) is replaced by
#                       <+><return><tab>.
#
?ENDMSC?
#  DO NOT INDENT THE << below!
#

?MSC?
$(GAMEFILE) :  $(LNKOPT) $(ALLOBJ)
?ENDMSC?
?BC?
$(GAMEFILE) :  $(ALLOBJ)
?ENDBC?
	@echo Linking....
	$(LINK) $(LFLAGSN) @<<$(GAME).lnk
?BC?
		$(ALLOBJ)
?ENDBC?
?MSC?
		$(ALLOBJ:^	=+^
		)
?ENDMSC?
		$(GAMEFILE)
		$(GAME)
		$(TERMLIB) $(MOVETR) $(CLIB) $(BCOVL) $(BCMDL)
?MSC?
		$(LNKOPT);
?ENDMSC?
<<
	@if exist $(GAMEDIR)\$(GAME).bak del $(GAMEDIR)\$(GAME).bak

#
# Makedefs Stuff
#

$(U)makedefs.exe:	$(MAKEOBJS)
	@$(LINK) $(LFLAGSU) $(MAKEOBJS), $@,, $(CLIB) $(BCMDL);

$(O)makedefs.o: $(CONFIG_H)	    $(PERMONST_H)      $(OBJCLASS_H) \
		 $(MONSYM_H)    $(QTEXT_H)	$(PATCHLEVEL_H) \
		 $(U)makedefs.c
	@$(CC) $(CFLAGSU) $(COBJNAM)$@ $(U)makedefs.c

#
#  date.h should be remade every time any of the source or include
#  files is modified.
#

$(INCL)\date.h : $(U)makedefs.exe
	$(U)makedefs -v
	@echo A new $@ has been created.

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

#
# Level Compiler Stuff
#

$(U)lev_comp.exe:  $(SPLEVOBJS)
	@echo Linking $@...
?MSC?
	$(LINK) $(LFLAGSU) @<<$(@B).lnk
?ENDMSC?
?BC?
	$(LINK) $(LFLAGSU) @&&!
?ENDBC?
?LINKLIST:SPLEVOBJS?
		$@
		$(@B)
		$(BCMDL);
?MSC?
<<
?ENDMSC?
?BC?
!
?ENDBC?

$(O)lev_yacc.o:  $(HACK_H)   $(SP_LEV_H) $(INCL)\lev_comp.h $(U)lev_yacc.c
	@$(CC) $(CFLAGSU) $(COBJNAM)$@ $(U)lev_yacc.c

$(O)lev_$(LEX).o:  $(HACK_H)   $(INCL)\lev_comp.h $(SP_LEV_H) \
	$(U)lev_$(LEX).c
	$(CC) $(CFLAGSU) $(COBJNAM)$@ $(U)lev_$(LEX).c

$(O)lev_main.o:	$(U)lev_main.c $(HACK_H)   $(SP_LEV_H)
	@$(CC) $(CFLAGSU) $(COBJNAM)$@ $(U)lev_main.c

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
	   @echo from $(SYS) to $(U)lev_yacc.c, and copy the prebuilt
	   @echo lev_comp.h from $(SYS) to $(UTIL)\lev_comp.h
	   @echo and use those.
	   @echo.
	   copy $(SYS)\lev_yacc.c $@ >nul
	   touch $@
	   copy $(SYS)\lev_comp.h $(INCL)\lev_comp.h >nul
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
	   @echo from $(SYS) to $@ and use it.
	   @echo.
	   copy $(SYS)\lev_lex.c $@ >nul
	   touch $@
!	ENDIF

#
# Dungeon Stuff
#

$(U)dgn_comp.exe: $(DGNCOMPOBJS)
    @echo Linking $@...
?MSC?
	$(LINK) $(LFLAGSU) @<<$(@B).lnk
?ENDMSC?
?BC?
	$(LINK) $(LFLAGSU) @&&!
?ENDBC?
?LINKLIST:DGNCOMPOBJS?
		$@
		$(@B)
		$(BCMDL);
?MSC?
<<
?ENDMSC?
?BC?
!
?ENDBC?

$(O)dgn_yacc.o:	$(HACK_H)   $(DGN_FILE_H) $(INCL)\dgn_comp.h \
	$(U)dgn_yacc.c
	@$(CC) $(CFLAGSU) $(COBJNAM)$@ $(U)dgn_yacc.c

$(O)dgn_$(LEX).o: $(HACK_H)   $(DGN_FILE_H)  $(INCL)\dgn_comp.h \
	$(U)dgn_$(LEX).c
	@$(CC) $(CFLAGSU) $(COBJNAM)$@ $(U)dgn_$(LEX).c

$(O)dgn_main.o:	$(HACK_H) $(U)dgn_main.c
	@$(CC) $(CFLAGSU) $(COBJNAM)$@ $(U)dgn_main.c

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
	   @echo $(SYS) to $(U)dgn_yacc.c, and copy the prebuilt
	   @echo dgn_comp.h from $(SYS) to $(INCL)\dgn_comp.h 
	   @echo and use those.
	   @echo.
	   copy $(SYS)\dgn_yacc.c $@ >nul
	   touch $@
	   copy $(SYS)\dgn_comp.h $(INCL)\dgn_comp.h >nul
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
	   @echo from $(SYS) to $@ and use it.
	   @echo.
	   copy $(SYS)\dgn_lex.c $@ >nul
	   touch $@
!	ENDIF


obj.tag:
	@if not exist $(O)*.* mkdir $(OBJ)
	@echo directory $(OBJ) created
	@echo directory $(OBJ) created >$@

?MSC?
#
#  The correct switches for the C compiler depend on the CL environment
#  variable being set correctly.  This will check that it is.
#  The correct setting needs to be:
#    CL= /AL /G2 /Oo /Gs /Gt16 /Zp1 /W0 /I..\include /nologo /DMOVERLAY
#

?ENDMSC?
envchk: precomp.msg
?MSC?
!	IF ("$(CL)"=="")
!	   MESSAGE The CL environment variable is not defined!
!	   MESSAGE You must CD $(MSYS) and execute the SETUP.BAT procedure
!	   MESSAGE ie.        setup MSC
!	   MESSAGE
!	   ERROR
!	ELSE
	   @echo CL Environment variable is defined:
	   @echo CL=$(CL)
!	ENDIF
?ENDMSC?
?COMMENT?
#    CL= /AL /G2 /Oo /Gs /Gt16 /Zp1 /W0 /I..\include /nologo /DMOVERLAY
?ENDCOMMENT?
?BC?
#
# Borland Configuration File Section
#
	@echo Making Borland configuration files...
	@echo -Y -O -Z -Oe -Ob -Os -Ff -I$(BCINCL);$(INCL) > $(BCCFG)
	@echo -m$(MODEL) -D__IO_H $(CFLGTOT) -DSTRNCMPI >> $(BCCFG)
	@type $(BCCFG) > CFLAGCO.CFG
	@type $(BCCFG) > CFLAGUO.CFG
	@type $(BCCFG) > CFLAGC0.CFG
	@type $(BCCFG) > CFLAGU0.CFG
	@type $(BCCFG) > CFLAGC1.CFG
	@type $(BCCFG) > CFLAGU1.CFG
	@type $(BCCFG) > CFLAGC2.CFG
	@type $(BCCFG) > CFLAGU2.CFG
	@type $(BCCFG) > CFLAGC3.CFG
	@type $(BCCFG) > CFLAGU3.CFG
	@type $(BCCFG) > CFLAGCB.CFG
	@type $(BCCFG) > CFLAGUB.CFG
    	@echo -Y $(CFLAGCO) >> CFLAGCO.CFG
	@echo -Y $(CFLAGUO) >> CFLAGUO.CFG
	@echo -Y $(CFLAGC0) >> CFLAGC0.CFG
	@echo -Y $(CFLAGU0) >> CFLAGU0.CFG
	@echo -Y $(CFLAGC1) >> CFLAGC1.CFG
	@echo -Y $(CFLAGU1) >> CFLAGU1.CFG
	@echo -Y $(CFLAGC2) >> CFLAGC2.CFG
	@echo -Y $(CFLAGU2) >> CFLAGU2.CFG
	@echo -Y $(CFLAGC3) >> CFLAGC3.CFG
	@echo -Y $(CFLAGU3) >> CFLAGU3.CFG
	@echo -Y $(CFLAGCB) >> CFLAGCB.CFG
	@echo -Y $(CFLAGUB) >> CFLAGUB.CFG
?ENDBC?
!	IF "$(TILEGAME)"==""
	   @echo.
	   @echo NOTE: This build will NOT include tile support.
	   @echo.
!	ELSE
	   @echo.
	   @echo This build includes tile support.
	   @echo.
!	ENDIF

#
# SECONDARY TARGETS
#

#
# Header files NOT distributed in ..\include
#

$(INCL)\tile.h: $(WSHR)\tile.h
	copy $(WSHR)\tile.h $@

$(INCL)\pctiles.h: $(MSYS)\pctiles.h
	copy $(MSYS)\pctiles.h $@

$(INCL)\pcvideo.h: $(MSYS)\pcvideo.h
	copy $(MSYS)\pcvideo.h $@

$(INCL)\portio.h: $(MSYS)\portio.h
	copy $(MSYS)\portio.h $@

#
#  Recover Utility
#

$(U)recover.exe: $(RECOVOBJS)
	@$(LINK) $(LFLAGSU) $(RECOVOBJS),$@,, $(CLIB) $(BCMDL);

#
#  Tile Mapping
#

$(SRC)\tile.c: $(U)tilemap.exe
	@echo A new $@ is being created.
	@$(U)tilemap

$(U)tilemap.exe: $(O)tilemap.o
	@$(LINK) $(LFLAGSU) $(O)tilemap.o,$@,, $(CLIB) $(BCMDL);

$(O)tilemap.o:  $(WSHR)\tilemap.c $(HACK_H)
	$(CC) $(CFLAGSU) $(COBJNAM)$@ $(WSHR)\tilemap.c


#
# Tile Utilities
#

#
#  Optional (for development)
#



#

$(U)gif2txt.exe: $(GIFREADERS) $(TEXT_IO)
	@$(LINK) $(LFLAGSU) $(GIFREADERS) $(TEXT_IO),$@,, \
		$(CLIB) $(BCMDL);

$(U)txt2ppm.exe: $(PPMWRITERS) $(TEXT_IO)
	@$(LINK) $(LFLAGSU) $(PPMWRITERS) $(TEXT_IO),$@,, \
		$(CLIB) $(BCMDL);

$(U)gif2txt2.exe: $(GIFREAD2) $(TEXT_IO2)
	@$(LINK) $(LFLAGSU) $(GIFREAD2) $(TEXT_IO2),$@,, \
		$(CLIB) $(BCMDL);

$(U)txt2ppm2.exe: $(PPMWRIT2) $(TEXT_IO2)
	@$(LINK) $(LFLAGSU) $(PPMWRIT2) $(TEXT_IO2),$@,, \
		$(CLIB) $(BCMDL);

#
#  Required for tile support
#

NetHack1.tib: $(TILEFILES) $(U)tile2bin.exe
	@echo Creating binary tile files (this may take some time)
	@$(U)tile2bin

NetHackO.tib: thintile.tag $(TILEFILES2) $(U)til2bin2.exe
	@echo Creating overview binary tile files (this may take some time)
	@$(U)til2bin2

thintile.tag: $(U)thintile.exe $(TILEFILES)
	$(U)thintile
	@echo thintiles created >thintile.tag

$(U)tile2bin.exe: $(O)tile2bin.o $(TEXT_IO)
    @echo Linking $@...
?MSC?
	$(LINK) $(LFLAGSU) @<<$(@B).lnk
?ENDMSC?
?BC?
	$(LINK) $(LFLAGSU) @&&!
?ENDBC?
		$(O)tile2bin.o+
?LINKLIST:TEXT_IO?
		$@
		$(@B)
		$(BCMDL);
?MSC?
<<
?ENDMSC?
?BC?
!
?ENDBC?

$(U)til2bin2.exe: $(O)til2bin2.o $(TEXT_IO2)
    @echo Linking $@...
?MSC?
	$(LINK) $(LFLAGSU) @<<$(@B).lnk
?ENDMSC?
?BC?
	$(LINK) $(LFLAGSU) @&&!
?ENDBC?
		$(O)til2bin2.o+
?LINKLIST:TEXT_IO2?
		$@
		$(@B)
		$(BCMDL);
?MSC?
<<
?ENDMSC?
?BC?
!
?ENDBC?


$(U)thintile.exe: $(O)thintile.o
	@$(LINK) $(LFLAGSU) $(O)thintile.o,$@,, $(CLIB) $(BCMDL);

$(O)thintile.o:  $(HACK_H) $(INCL)\tile.h $(WSHR)\thintile.c
	$(CC) $(CFLAGSU) $(COBJNAM)$@ $(WSHR)\thintile.c

$(O)tile2bin.o:  $(HACK_H) $(TILE_H) $(PCVIDEO_H)
	$(CC) $(CFLAGSU) $(COBJNAM)$@ $(MSYS)\tile2bin.c

$(O)til2bin2.o:  $(HACK_H) $(TILE_H) $(PCVIDEO_H)
	$(CC) $(CFLAGSU) $(CDEFINE)TILE_X=8 $(CDEFINE)OVERVIEW_FILE \
		$(COBJNAM)$@ $(MSYS)\tile2bin.c

?COMMENT?
$(U)tile2btb.exe: $(O)tile2btb.o $(GIFREADERS)
    @echo Linking $@...
	$(LINK) $(LFLAGSU) @&&!
		$(O)tile2btb.o+
?LINKLIST:GIFREADERS?
		$@
		$(@B)
		$(BCMDL) $(BGI_LIB);
!

$(O)tile2btb.o:  $(HACK_H) $(TILE_H) $(PCVIDEO_H) $(MSYS)\tile2btb.c
	$(CC) -DBGI_FILE $(CFLAGSU) $(COBJNAM)$@ $(MSYS)\tile2btb.c
?ENDCOMMENT?
  
#
# DLB stuff
#

nhdat:	$(U)dlb_main.exe
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
	@for %%N in (*.lev) do echo %%N >>dlb.lst
	$(U)dlb_main cvIf dlb.lst $(SRC)\nhdat
	@cd $(SRC)

$(U)dlb_main.exe: $(DLBOBJS)
	@$(LINK) $(LFLAGSU) $(DLBOBJS),$@,, $(CLIB) $(BCMDL);

$(O)dlb_main.o: $(U)dlb_main.c $(INCL)\config.h $(DLB_H)
	$(CC) $(CFLAGSU) $(COBJNAM)$@ $(U)dlb_main.c

#
# Housekeeping
#

spotless: clean
	rmdir $(OBJ)
	if exist $(DATE_H)    del $(DATE_H)
	if exist $(INCL)\onames.h  del $(INCL)\onames.h
	if exist $(INCL)\pm.h      del $(INCL)\pm.h
	if exist $(VIS_TAB_H) del $(VIS_TAB_H)
	if exist $(SRC)\vis_tab.c  del $(SRC)\vis_tab.c
	if exist $(SRC)\tile.c     del $(SRC)\tile.c
	if exist $(DAT)\rumors     del $(DAT)\rumors
	if exist $(DAT)\data		del $(DAT)\data
	if exist $(DAT)\dungeon		del $(DAT)\dungeon
	if exist $(DAT)\dungeon.pdf	del $(DAT)\dungeon.pdf
	if exist $(DAT)\options		del $(DAT)\options
	if exist $(DAT)\oracles		del $(DAT)\oracles
	if exist $(DAT)\rumors		del $(DAT)\rumors
	if exist $(DAT)\quest.dat	del $(DAT)\quest.dat
	if exist $(DAT)\*.lev		del $(DAT)\*.lev
	if exist $(DAT)\sp_lev.tag	del $(DAT)\sp_lev.tag
	if exist $(SRC)\monstr.c        del $(SRC)\monstr.c
	if exist $(SRC)\vis_tab.c       del $(SRC)\vis_tab.c
	if exist $(SRC)\$(PLANAR_TIB)   del $(SRC)\$(PLANAR_TIB)
	if exist $(SRC)\$(OVERVIEW_TIB) del $(SRC)\$(OVERVIEW_TIB)
	if exist $(U)recover.exe        del $(U)recover.exe

clean:
	if exist $(O)*.o del $(O)*.o
	if exist $(O)*.0 del $(O)*.0
	if exist $(O)*.1 del $(O)*.1
	if exist $(O)*.2 del $(O)*.2
	if exist $(O)*.3 del $(O)*.3
	if exist $(O)*.b del $(O)*.b
	if exist $(U)utility.tag   del $(U)utility.tag
	if exist $(U)makedefs.exe  del $(U)makedefs.exe
	if exist $(U)lev_comp.exe  del $(U)lev_comp.exe
	if exist $(U)dgn_comp.exe  del $(U)dgn_comp.exe
	if exist $(U)dlb_main.exe  del $(U)dlb_main.exe
	if exist $(SRC)\*.lnk      del $(SRC)\*.lnk
	if exist $(SRC)\*.map      del $(SRC)\*.map
	if exist $(SRC)\*$(CPCHEXT) del $(SRC)\*$(CPCHEXT)
?BC?
	if exist $(SRC)\*.cfg      del $(SRC)\*.cfg
?ENDBC?
	if exist $(DAT)\dlb.lst    del $(DAT)\dlb.lst

pch.c:	$(HACK_H)
	@echo ^#include "hack.h" > $@
	@echo main(int argc, char *argv[]) >> $@
	@echo { >> $@
	@echo } >> $@
	@echo. >> $@

#
# OTHER DEPENDENCIES
#

#
# Precompiled Header dependencies
# (We need to force the generation of these at the beginning)
#

PHO$(CPCHEXT): $(HACK_H) pch.c
	@echo Generating new precompiled header for .O files
	@$(CC) $(FLAGCO) pch.c
PH0$(CPCHEXT): $(HACK_H) pch.c
	@echo Generating new precompiled header for .0 files
	@$(CC) $(FLAGC0) pch.c
PH1$(CPCHEXT): $(HACK_H) pch.c
	@echo Generating new precompiled header for .1 files
	@$(CC) $(FLAGC1) pch.c
PH2$(CPCHEXT): $(HACK_H) pch.c
	@echo Generating new precompiled header for .2 files
	@$(CC) $(FLAGC2) pch.c
PH3$(CPCHEXT): $(HACK_H) pch.c
	@echo Generating new precompiled header for .3 files
	@$(CC) $(FLAGC3) pch.c
PHB$(CPCHEXT): $(HACK_H) pch.c
	@echo Generating new precompiled header for .B files
	@$(CC) $(FLAGCB) pch.c

?MSC?
#
# Compiler supplied, manually moved file - MOVEINIT.C.
# - This is only compiled if you selected the alternate overlay
#   schema3. (MOVEAPI.H must reside in your include search list,
#   and MOVEINIT.C must be in your src directory).  The patch
#   in sys/msdos/moveinit.pat must be applied to moveinit.c
#   MS will not allow us to distribute an already patched version.

$(O)moveinit.o: $(SRC)\moveinit.c
	$(CC) $(CFLAGSN) $(COBJNAM)$@ $(MVTRCL) $(SRC)\moveinit.c

$(SRC)\moveinit.c:
	@echo.
	@echo * CANNOT COMPLETE THE BUILD *
	@echo You must manually copy moveinit.c and moveinit.h
	@echo from your Microsoft C Compiler directory tree
	@echo source/move directory and apply the sys/msdos/moveinit.pat
	@echo patch to moveinit.c after doing so.
	@echo.
?ENDMSC?

?BC?
# Overlay initialization routines used by pcmain() at startup to
# determine EMS/XMS memory usage.

# Comment out the following line if you don't want Borland C++ to check for
# extended memory.
RECOGNIZE_XMS = $(CDEFINE)RECOGNIZE_XMS

?ENDBC?
?MSC?
# Overlay initialization routines used by MOVEINIT.C
?ENDMSC?

$(O)ovlinit.o: $(MSYS)\ovlinit.c $(HACK_H)
	$(CC) $(CFLAGSN) $(RECOGNIZE_XMS) $(COBJNAM)$@ $(MSYS)\ovlinit.c

#
# dat dependencies
#

$(DAT)\data: $(U)utility.tag    $(DATABASE)
	$(U)makedefs -d

$(DAT)\rumors: $(U)utility.tag    $(DAT)\rumors.tru   $(DAT)\rumors.fal
	$(U)makedefs -r

$(DAT)\quest.dat: $(U)utility.tag  $(DAT)\quest.txt
	$(U)makedefs -q

$(DAT)\oracles: $(U)utility.tag    $(DAT)\oracles.txt
	$(U)makedefs -h

$(DAT)\dungeon: $(U)utility.tag  $(DAT)\dungeon.def
	$(U)makedefs -e
	cd $(DAT)
	$(U)dgn_comp dungeon.pdf
	cd $(SRC)

#
#  Util Dependencies.
#

$(O)panic.o:   $(U)panic.c $(CONFIG_H)
	$(CC) $(CFLAGSU) $(COBJNAM)$@ $(U)panic.c

$(O)recover.o: $(CONFIG_H) $(U)recover.c
	$(CC) $(CFLAGSU) $(COBJNAM)$@ $(U)recover.c

#
#  from win\share
#

$(O)tiletxt.o:  $(WSHR)\tilemap.c $(HACK_H)
	$(CC) $(CFLAGSU) $(CDEFINE)TILETEXT $(COBJNAM)$@ $(WSHR)\tilemap.c

$(O)tiletxt2.o:  $(WSHR)\tilemap.c $(HACK_H)
	$(CC) $(CFLAGSU) $(CDEFINE)TILETEXT \
		$(CDEFINE)TILE_X=8 $(COBJNAM)$@ $(WSHR)\tilemap.c

$(O)gifread.o:  $(WSHR)\gifread.c  $(CONFIG_H) $(INCL)\tile.h
	$(CC) $(CFLAGSU) $(COBJNAM)$@ $(WSHR)\gifread.c

$(O)gifread2.o:  $(WSHR)\gifread.c  $(CONFIG_H) $(INCL)\tile.h
	$(CC) $(CFLAGSU) $(COBJNAM)$@ $(CDEFINE)TILE_X=8 $(WSHR)\gifread.c

$(O)ppmwrite.o: $(WSHR)\ppmwrite.c $(CONFIG_H) $(INCL)\tile.h
	$(CC) $(CFLAGSU) $(COBJNAM)$@ $(WSHR)\ppmwrite.c

$(O)ppmwrit2.o: $(WSHR)\ppmwrite.c $(CONFIG_H) $(INCL)\tile.h
	$(CC) $(CFLAGSU) $(COBJNAM)$@ $(CDEFINE)TILE_X=8 $(WSHR)\ppmwrite.c

$(O)tiletext.o:   $(WSHR)\tiletext.c  $(CONFIG_H) $(INCL)\tile.h
	$(CC) $(CFLAGSU) $(COBJNAM)$@ $(WSHR)\tiletext.c

$(O)tiletex2.o:   $(WSHR)\tiletext.c  $(CONFIG_H) $(INCL)\tile.h
	$(CC) $(CFLAGSU) $(CDEFINE)TILE_X=8 $(COBJNAM)$@ $(WSHR)\tiletext.c

#
#  from win\tty
#

$(O)getline.1:  $(PCH1) $(WTTY)\getline.c  $(HACK_H) $(WINTTY_H) $(FUNC_TAB_H)
	$(CC) $(FLAGU1) ?[CSNAM1]$(COBJNAM)$@ $(WTTY)\getline.c

$(O)getline.2:  $(PCH2) $(WTTY)\getline.c  $(HACK_H) $(WINTTY_H) $(FUNC_TAB_H)
	$(CC) $(FLAGU2) ?[CSNAM2]$(COBJNAM)$@ $(WTTY)\getline.c

$(O)termcap.0:  $(PCH0) $(WTTY)\termcap.c  $(HACK_H) $(WINTTY_H) $(TERMCAP_H)
	$(CC) $(FLAGU0) ?[CSNAM0]$(COBJNAM)$@ $(WTTY)\termcap.c

$(O)termcap.1:  $(PCH1) $(WTTY)\termcap.c  $(HACK_H) $(WINTTY_H) $(TERMCAP_H)
	$(CC) $(FLAGU1) ?[CSNAM1]$(COBJNAM)$@ $(WTTY)\termcap.c

$(O)termcap.B:  $(PCHB) $(WTTY)\termcap.c  $(HACK_H) $(WINTTY_H) $(TERMCAP_H)
	$(CC) $(FLAGUB) ?[CSNAMB]$(COBJNAM)$@ $(WTTY)\termcap.c

$(O)topl.1:     $(PCH1) $(WTTY)\topl.c     $(HACK_H) $(TERMCAP_H) $(WINTTY_H)
	$(CC) $(FLAGU1) ?[CSNAM1]$(COBJNAM)$@ $(WTTY)\topl.c

$(O)topl.2:     $(PCH2) $(WTTY)\topl.c     $(HACK_H) $(TERMCAP_H) $(WINTTY_H)
	$(CC) $(FLAGU2) ?[CSNAM2]$(COBJNAM)$@ $(WTTY)\topl.c

$(O)topl.B:     $(PCHB) $(WTTY)\topl.c     $(HACK_H) $(TERMCAP_H) $(WINTTY_H)
	$(CC) $(FLAGUB) ?[CSNAMB]$(COBJNAM)$@ $(WTTY)\topl.c

$(O)wintty.o: $(PCHO) $(CONFIG_H) $(WTTY)\wintty.c $(PATCHLEVEL_H)
	$(CC) $(FLAGUO) ?[CSNAMOB]$(COBJNAM)$@ $(WTTY)\wintty.c

#
# from sys\share
#

$(O)pcmain.0:   $(PCH0) $(HACK_H) $(SYS)\pcmain.c
	$(CC)  $(FLAGU0) ?[CSNAM0]$(COBJNAM)$@ $(SYS)\pcmain.c

$(O)pcmain.1:   $(PCH1) $(HACK_H) $(SYS)\pcmain.c
	$(CC)  $(FLAGU1) ?[CSNAM1]$(COBJNAM)$@ $(SYS)\pcmain.c

$(O)pcmain.B:   $(PCHB) $(HACK_H) $(SYS)\pcmain.c
	$(CC)  $(FLAGUB) ?[CSNAMB]$(COBJNAM)$@ $(SYS)\pcmain.c

$(O)pcunix.B:   $(PCHB) $(SYS)\pcunix.c   $(HACK_H)
	$(CC) $(FLAGUB) ?[CSNAMB]$(COBJNAM)$@ $(SYS)\pcunix.c

$(O)tty.o:     $(HACK_H) $(WINTTY_H) $(SYS)\pctty.c
	$(CC)  $(CFLAGSN) ?[CSNAMOB]$(COBJNAM)$@  $(SYS)\pctty.c

$(O)sys.o:    $(HACK_H) $(SYS)\pcsys.c
	$(CC)  $(CFLAGSN) ?[CSNAMOB]$(COBJNAM)$@ $(SYS)\pcsys.c

$(O)random.o: $(PCHO) $(HACK_H) $(SYS)\random.c
	$(CC) $(FLAGUO) ?[CSNAMOB]$(COBJNAM)$@ $(SYS)\random.c

#
# from sys\msdos
#

$(O)msdos.0: $(MSYS)\msdos.c   $(HACK_H) $(PCVIDEO_H)
?BC?
	$(CC) $(CFLAGSN) $(COVL0) $$($(@B)_0) $(COBJNAM)$@ $(MSYS)\msdos.c
?ENDBC?
?MSC?
	$(CC) $(FLAGU0) $(CCSNAM)$(@F) $(COBJNAM)$@ $(MSYS)\msdos.c
?ENDMSC?
?COMMENT?
	$(CC) $(CFLAGSN) $(COVL0) ?[CSNAM0]$(COBJNAM)$@ $(MSYS)\vidtxt.c
?ENDCOMMENT?

$(O)msdos.B: $(MSYS)\msdos.c   $(HACK_H) $(PCVIDEO_H)
?BC?
	$(CC) $(CFLAGSN) $(COVLB) $$($(@B)_b) $(COBJNAM)$@ $(MSYS)\msdos.c
?ENDBC?
?MSC?
	$(CC) $(FLAGUB) $(CCSNAM)$(@F) $(COBJNAM)$@ $(MSYS)\msdos.c
?ENDMSC?
?COMMENT?
	$(CC) $(CFLAGSN) $(COVLB) ?[CSNAMB]$(COBJNAM)$@ $(MSYS)\vidtxt.c
?ENDCOMMENT?

$(O)pctiles.0: $(PCH0) $(MSYS)\pctiles.c $(HACK_H) $(TILE_H) $(PCVIDEO_H)
	$(CC) $(FLAGU0) ?[CSNAM0]$(COBJNAM)$@ $(MSYS)\pctiles.c

$(O)pctiles.B: $(PCHB) $(MSYS)\pctiles.c $(HACK_H) $(TILE_H) $(PCVIDEO_H)
	$(CC) $(FLAGUB) ?[CSNAMB]$(COBJNAM)$@ $(MSYS)\pctiles.c

$(O)sound.o: $(PCH0) $(MSYS)\sound.c   $(HACK_H) $(INCL)\portio.h
	$(CC) $(FLAGUO) ?[CSNAMOB]$(COBJNAM)$@ $(MSYS)\sound.c

$(O)pckeys.o: $(PCHO) $(MSYS)\pckeys.c   $(HACK_H) $(PCVIDEO_H)
	$(CC) $(FLAGUO) ?[CSNAMOB]$(COBJNAM)$@ $(MSYS)\pckeys.c

$(O)stubvid.o : $(MSYS)\video.c $(HACK_H)
	$(CC) $(FLAGUO) $(CDEFINE)STUBVIDEO ?[CSNAMOB]$(COBJNAM)$@ $(MSYS)\video.c

$(O)video.0: $(PCH0) $(MSYS)\video.c   $(HACK_H) $(WINTTY_H) $(PCVIDEO_H) \
                $(TILE_H)
	$(CC) $(FLAGU0) ?[CSNAM0]$(COBJNAM)$@ $(MSYS)\video.c

$(O)video.1: $(PCH1) $(MSYS)\video.c   $(HACK_H) $(WINTTY_H) $(PCVIDEO_H) \
                $(TILE_H)
	$(CC) $(FLAGU1) ?[CSNAM1]$(COBJNAM)$@ $(MSYS)\video.c

$(O)video.B: $(PCHB) $(MSYS)\video.c   $(HACK_H) $(WINTTY_H) $(PCVIDEO_H) \
                $(TILE_H)
	$(CC) $(FLAGUB) ?[CSNAMB]$(COBJNAM)$@ $(MSYS)\video.c

$(O)vidtxt.0: $(MSYS)\vidtxt.c  $(HACK_H) $(WINTTY_H) $(PCVIDEO_H)
?BC?
	$(CC) $(CFLAGSN) $(COVL0) $$($(@B)_0) $(COBJNAM)$@ $(MSYS)\vidtxt.c
?ENDBC?
?MSC?
	$(CC) $(FLAGU0) $(CCSNAM)$(@F) $(COBJNAM)$@ $(MSYS)\vidtxt.c
?ENDMSC?
?COMMENT?
	$(CC) $(CFLAGSN) $(COVL0) ?[CSNAM0]$(COBJNAM)$@ $(MSYS)\vidtxt.c
?ENDCOMMENT?

$(O)vidtxt.B: $(MSYS)\vidtxt.c  $(HACK_H) $(WINTTY_H) $(PCVIDEO_H)
?BC?
	$(CC) $(CFLAGSN) $(COVLB) $$($(@B)_b) $(COBJNAM)$@ $(MSYS)\vidtxt.c
?ENDBC?
?MSC?
	$(CC) $(FLAGUB) $(CCSNAM)$(@F) $(COBJNAM)$@ $(MSYS)\vidtxt.c
?ENDMSC?
?COMMENT?
	$(CC) $(CFLAGSN) $(COVLB) ?[CSNAMB]$(COBJNAM)$@ $(MSYS)\vidtxt.c
?ENDCOMMENT?

$(O)vidvga.0: $(PCH0) $(MSYS)\vidvga.c  $(HACK_H) $(WINTTY_H) $(PCVIDEO_H) \
		$(TILE_H)
	$(CC) $(FLAGU0) ?[CSNAM0]$(COBJNAM)$@ $(MSYS)\vidvga.c

$(O)vidvga.1: $(PCH1) $(MSYS)\vidvga.c  $(HACK_H) $(WINTTY_H) $(PCVIDEO_H) \
		$(TILE_H)
	$(CC) $(FLAGU1) ?[CSNAM0]$(COBJNAM)$@ $(MSYS)\vidvga.c

$(O)vidvga.2: $(PCH2) $(MSYS)\vidvga.c  $(HACK_H) $(WINTTY_H) $(PCVIDEO_H) \
		$(TILE_H)
	$(CC) $(FLAGU2) ?[CSNAM0]$(COBJNAM)$@ $(MSYS)\vidvga.c

$(O)vidvga.B: $(PCHB) $(MSYS)\vidvga.c  $(HACK_H) $(WINTTY_H) $(PCVIDEO_H) \
		$(TILE_H)
	$(CC) $(FLAGUB) ?[CSNAMB]$(COBJNAM)$@ $(MSYS)\vidvga.c

#
# from src
#

$(O)alloc.o:     $(SRC)\alloc.c    $(CONFIG_H)
	$(CC) $(CFLAGSN) ?[CSNAMOB]$(COBJNAM)$@ $(SRC)\alloc.c
$(O)ball.o:      $(PCHO) $(SRC)\ball.c     $(HACK_H)
$(O)bones.o:     $(PCHO) $(SRC)\bones.c    $(HACK_H) $(LEV_H)
$(O)decl.o:      $(PCHO) $(SRC)\decl.c     $(HACK_H) $(QUEST_H)
$(O)detect.o:    $(PCHO) $(SRC)\detect.c   $(HACK_H) $(ARTIFACT_H)
$(O)dig.o:	 $(PCHO) $(SRC)\dig.c	   $(HACK_H) $(EDOG_H) # check dep
$(O)display.o:	 $(PCHO) $(SRC)\display.c  $(HACK_H)
$(O)dlb.o:	 $(SRC)\dlb.c	   $(DLB_H) $(HACK_H)
	$(CC) $(CFLAGSN) ?[CSNAMOB]$(COBJNAM)$@ $(SRC)\dlb.c
$(O)dokick.o:    $(PCHO) $(SRC)\dokick.c   $(HACK_H) $(ESHK_H)
$(O)dothrow.o:   $(PCHO) $(SRC)\dothrow.c  $(HACK_H)
$(O)drawing.o:   $(SRC)\drawing.c  $(HACK_H) $(TERMCAP_H)
	$(CC) $(CFLAGSN) ?[CSNAMOB]$(COBJNAM)$@ $(SRC)\drawing.c
$(O)end.o:       $(SRC)\end.c      $(HACK_H) $(ESHK_H) $(DLB_H)
	$(CC) $(CFLAGSN) ?[CSNAMOB]$(COBJNAM)$@ $(SRC)\end.c
$(O)exper.o:     $(PCHO) $(SRC)\exper.c    $(HACK_H)
$(O)extralev.o:  $(PCHO) $(SRC)\extralev.c $(HACK_H)
$(O)files.o:	 $(PCHO) $(SRC)\files.c    $(HACK_H) $(DLB_H)
$(O)fountain.o:  $(PCHO) $(SRC)\fountain.c $(HACK_H)
$(O)minion.o:    $(PCHO) $(SRC)\minion.c   $(HACK_H) $(EMIN_H) $(EPRI_H)
$(O)mklev.o:     $(PCHO) $(SRC)\mklev.c    $(HACK_H)
$(O)mkmap.o:     $(PCHO) $(SRC)\mkmap.c    $(HACK_H) $(SP_LEV_H)
$(O)mkmaze.o:	 $(PCHO) $(SRC)\mkmaze.c   $(HACK_H) $(SP_LEV_H) $(LEV_H)
$(O)monst.o:     $(SRC)\monst.c    $(CONFIG_H) $(PERMONST_H) $(MONSYM_H) \
		 $(ESHK_H) $(EPRI_H) $(COLOR_H) $(ALIGN_H)
	$(CC) $(CFLAGSN) ?[CSNAMOB]$(COBJNAM)$@ $(SRC)\monst.c
$(O)monstr.o:    $(SRC)\monstr.c   $(CONFIG_H)
	$(CC) $(CFLAGSN) ?[CSNAMOB]$(COBJNAM)$@ $(SRC)\monstr.c
$(O)mplayer.o:   $(PCHO) $(SRC)\mplayer.c  $(HACK_H)
$(O)muse.o:      $(PCHO) $(SRC)\muse.c     $(HACK_H)
$(O)music.o:     $(PCHO) $(SRC)\music.c    $(HACK_H)
$(O)o_init.o:	 $(PCHO) $(SRC)\o_init.c   $(HACK_H) $(LEV_H)
$(O)objects.o:   $(SRC)\objects.c  $(CONFIG_H) $(OBJ_H) $(OBJCLASS_H) \
                 $(PROP_H) $(COLOR_H)
	$(CC) $(CFLAGSN) ?[CSNAMOB]$(COBJNAM)$@ $(SRC)\objects.c
$(O)options.o:	 $(SRC)\options.c  $(HACK_H) $(TERMCAP_H) $(OBJCLASS_H)
	$(CC) $(CFLAGSN) ?[CSNAMOB]$(COBJNAM)$@ $(SRC)\options.c
$(O)pager.o:	 $(SRC)\pager.c    $(HACK_H) $(DLB_H)
	$(CC) $(CFLAGNO) $(COBJNAM)$@ ?[CSNAMOA]$(SRC)\pager.c
$(O)pickup.o:    $(PCHO) $(SRC)\pickup.c   $(HACK_H)
$(O)pray.o:      $(PCHO) $(SRC)\pray.c     $(HACK_H) $(EPRI_H)
$(O)quest.o:     $(PCHO) $(SRC)\quest.c    $(HACK_H) $(QUEST_H) $(QTEXT_H)
$(O)questpgr.o:  $(PCHO) $(SRC)\questpgr.c $(HACK_H) $(QTEXT_H) $(DLB_H)
$(O)rect.o:      $(PCHO) $(SRC)\rect.c     $(HACK_H)
$(O)region.o:    $(PCHO) $(SRC)\region.c   $(HACK_H)
$(O)restore.o:   $(PCHO) $(SRC)\restore.c  $(HACK_H) $(LEV_H) $(TERMCAP_H) \
		 $(QUEST_H)
$(O)rip.o:       $(PCHO) $(SRC)\rip.c      $(HACK_H)
$(O)role.o:	   $(PCHO) $(SRC)\role.c     $(HACK_H)
$(O)rumors.o:	 $(PCHO) $(SRC)\rumors.c   $(HACK_H) $(DLB_H)
$(O)save.o:      $(PCHO) $(SRC)\save.c     $(HACK_H) $(LEV_H) $(QUEST_H)
$(O)sit.o:       $(PCHO) $(SRC)\sit.c      $(HACK_H) $(ARTIFACT_H)
$(O)steed.o:	   $(PCHO) $(SRC)\steed.c    $(HACK_H)
$(O)sp_lev.o:	 $(PCHO) $(SRC)\sp_lev.c   $(HACK_H) $(SP_LEV_H) $(DLB_H)
$(O)spell.o:     $(PCHO) $(SRC)\spell.c    $(HACK_H)
$(O)teleport.o:  $(PCHO) $(SRC)\teleport.c $(HACK_H)	# check dep
$(O)tile.o:      $(PCHO) $(SRC)\tile.c     $(HACK_H)
$(O)topten.o:	 $(PCHO) $(SRC)\topten.c   $(HACK_H) $(DLB_H) $(PATCHLEVEL_H)
$(O)u_init.o:    $(PCHO) $(SRC)\u_init.c   $(HACK_H)
$(O)uhitm.o:     $(PCHO) $(SRC)\uhitm.c    $(HACK_H)
$(O)version.o:   $(PCHO) $(SRC)\version.c  $(HACK_H) $(PATCHLEVEL_H)
$(O)vision.o:    $(PCHO) $(SRC)\vision.c   $(HACK_H) $(VIS_TAB_H)
$(O)vis_tab.o:   $(SRC)\vis_tab.c  $(HACK_H) $(VIS_TAB_H)
	$(CC) $(CFLAGSN) ?[CSNAMOB]$(COBJNAM)$@ $(SRC)\vis_tab.c
$(O)wield.o:     $(PCHO) $(SRC)\wield.c    $(HACK_H)
$(O)windows.o:   $(PCHO) $(SRC)\windows.c  $(HACK_H) $(WINTTY_H)
$(O)worm.o:      $(PCHO) $(SRC)\worm.c     $(HACK_H) $(LEV_H)
$(O)worn.o:      $(PCHO) $(SRC)\worn.c     $(HACK_H)
$(O)write.o:     $(PCHO) $(SRC)\write.c    $(HACK_H)

#
# Overlays
#

# OVL0
#

$(O)allmain.0:  $(PCH0) $(SRC)\allmain.c  $(HACK_H)
$(O)apply.0:    $(PCH0) $(SRC)\apply.c    $(HACK_H) $(EDOG_H)
$(O)artifact.0: $(PCH0) $(SRC)\artifact.c $(HACK_H) $(ARTIFACT_H) $(ARTILIST_H)
$(O)attrib.0:   $(PCH0) $(SRC)\attrib.c   $(HACK_H)
$(O)botl.0:     $(PCH0) $(SRC)\botl.c     $(HACK_H)
$(O)cmd.0:      $(PCH0) $(SRC)\cmd.c      $(HACK_H) $(FUNC_TAB_H)
$(O)dbridge.0:  $(PCH0) $(SRC)\dbridge.c  $(HACK_H)
$(O)do.0:       $(PCH0) $(SRC)\do.c       $(HACK_H) $(LEV_H)
$(O)do_name.0:  $(PCH0) $(SRC)\do_name.c  $(HACK_H)
$(O)do_wear.0:  $(PCH0) $(SRC)\do_wear.c  $(HACK_H)
$(O)dogmove.0:  $(PCH0) $(SRC)\dogmove.c  $(HACK_H) $(MFNDPOS_H) $(EDOG_H)
$(O)dungeon.0:	$(PCH0) $(SRC)\dungeon.c  $(HACK_H) $(ALIGN_H) $(DGN_FILE_H) \
		$(DLB_H)
$(O)eat.0:      $(PCH0) $(SRC)\eat.c      $(HACK_H)
$(O)engrave.0:  $(PCH0) $(SRC)\engrave.c  $(HACK_H) $(LEV_H)
$(O)explode.0:  $(PCH0) $(SRC)\explode.c  $(HACK_H)
$(O)hacklib.0:  $(PCH0) $(SRC)\hacklib.c  $(HACK_H)
$(O)invent.0:   $(PCH0) $(SRC)\invent.c   $(HACK_H) $(ARTIFACT_H)
$(O)lock.0:     $(PCH0) $(SRC)\lock.c     $(HACK_H)
$(O)mail.0:     $(PCH0) $(SRC)\mail.c     $(HACK_H) $(MAIL_H) $(PATCHLEVEL_H)
$(O)makemon.0:  $(PCH0) $(SRC)\makemon.c  $(HACK_H) $(EPRI_H) $(EMIN_H)
$(O)mcastu.0:   $(PCH0) $(SRC)\mcastu.c   $(HACK_H)
$(O)mhitm.0:    $(PCH0) $(SRC)\mhitm.c    $(HACK_H) $(ARTIFACT_H) $(EDOG_H)
$(O)mhitu.0:    $(PCH0) $(SRC)\mhitu.c    $(HACK_H) $(ARTIFACT_H) $(EDOG_H)
$(O)mkobj.0:    $(PCH0) $(SRC)\mkobj.c    $(HACK_H) $(ARTIFACT_H) $(PROP_H)
$(O)mkroom.0:   $(PCH0) $(SRC)\mkroom.c   $(HACK_H)
$(O)mon.0:      $(PCH0) $(SRC)\mon.c      $(HACK_H) $(MFNDPOS_H) $(EDOG_H)
$(O)mondata.0:  $(PCH0) $(SRC)\mondata.c  $(HACK_H) $(ESHK_H) $(EPRI_H)
$(O)monmove.0:  $(PCH0) $(SRC)\monmove.c  $(HACK_H) $(MFNDPOS_H) $(ARTIFACT_H)
$(O)mthrowu.0:  $(PCH0) $(SRC)\mthrowu.c  $(HACK_H)
$(O)objnam.0:   $(PCH0) $(SRC)\objnam.c   $(HACK_H)
$(O)polyself.0: $(PCH0) $(SRC)\polyself.c $(HACK_H)
$(O)priest.0:   $(PCH0) $(SRC)\priest.c   $(HACK_H) $(MFNDPOS_H) $(ESHK_H) \
		$(EPRI_H) $(EMIN_H)
$(O)rnd.0:      $(PCH0) $(SRC)\rnd.c      $(HACK_H)
$(O)shk.0:      $(PCH0) $(SRC)\shk.c      $(HACK_H) $(ESHK_H)
$(O)shknam.0:   $(PCH0) $(SRC)\shknam.c   $(HACK_H) $(ESHK_H)
$(O)sounds.0:   $(PCH0) $(SRC)\sounds.c   $(HACK_H) $(EDOG_H)
$(O)steal.0:    $(PCH0) $(SRC)\steal.c    $(HACK_H)
$(O)timeout.0:	$(PCH0) $(SRC)\timeout.c  $(HACK_H) $(LEV_H)
$(O)track.0:    $(PCH0) $(SRC)\track.c    $(HACK_H)
$(O)trap.0:     $(PCH0) $(SRC)\trap.c     $(HACK_H)
$(O)vault.0:    $(PCH0) $(SRC)\vault.c    $(HACK_H) $(VAULT_H)
$(O)weapon.0:   $(PCH0) $(SRC)\weapon.c   $(HACK_H)
$(O)were.0:     $(PCH0) $(SRC)\were.c     $(HACK_H)
$(O)wizard.0:   $(PCH0) $(SRC)\wizard.c   $(HACK_H) $(QTEXT_H)
$(O)zap.0:      $(PCH0) $(SRC)\zap.c      $(HACK_H)

#
# OVL1
#

$(O)allmain.1:  $(PCH1) $(SRC)\allmain.c  $(HACK_H)
$(O)apply.1:    $(PCH1) $(SRC)\apply.c    $(HACK_H) $(EDOG_H)
$(O)artifact.1: $(PCH1) $(SRC)\artifact.c $(HACK_H) $(ARTIFACT_H) $(ARTILIST_H)
$(O)attrib.1:   $(PCH1) $(SRC)\attrib.c   $(HACK_H)
$(O)botl.1:     $(PCH1) $(SRC)\botl.c     $(HACK_H)
$(O)cmd.1:      $(PCH1) $(SRC)\cmd.c      $(HACK_H) $(FUNC_TAB_H)
$(O)dbridge.1:  $(PCH1) $(SRC)\dbridge.c  $(HACK_H)
$(O)do.1:       $(PCH1) $(SRC)\do.c       $(HACK_H) $(LEV_H)
$(O)do_wear.1:  $(PCH1) $(SRC)\do_wear.c  $(HACK_H)
$(O)dog.1:      $(PCH1) $(SRC)\dog.c      $(HACK_H) $(EDOG_H)
$(O)dungeon.1:	$(PCH1) $(SRC)\dungeon.c  $(HACK_H) $(ALIGN_H) $(DGN_FILE_H) $(DLB_H)
$(O)eat.1:      $(PCH1) $(SRC)\eat.c      $(HACK_H)
$(O)engrave.1:  $(PCH1) $(SRC)\engrave.c  $(HACK_H) $(LEV_H)
$(O)explode.1:  $(PCH1) $(SRC)\explode.c  $(HACK_H)
$(O)hack.1:     $(PCH1) $(SRC)\hack.c     $(HACK_H)
$(O)hacklib.1:  $(PCH1) $(SRC)\hacklib.c  $(HACK_H)
$(O)invent.1:   $(PCH1) $(SRC)\invent.c   $(HACK_H) $(ARTIFACT_H)
$(O)makemon.1:  $(PCH1) $(SRC)\makemon.c  $(HACK_H) $(EPRI_H) $(EMIN_H)
$(O)mhitu.1:    $(PCH1) $(SRC)\mhitu.c    $(HACK_H) $(ARTIFACT_H) $(EDOG_H)
$(O)mkobj.1:    $(PCH1) $(SRC)\mkobj.c    $(HACK_H) $(ARTIFACT_H) $(PROP_H)
$(O)mon.1:      $(PCH1) $(SRC)\mon.c      $(HACK_H) $(MFNDPOS_H) $(EDOG_H)
$(O)mondata.1:  $(PCH1) $(SRC)\mondata.c  $(HACK_H) $(ESHK_H) $(EPRI_H)
$(O)monmove.1:  $(PCH1) $(SRC)\monmove.c  $(HACK_H) $(MFNDPOS_H) $(ARTIFACT_H)
$(O)mthrowu.1:  $(PCH1) $(SRC)\mthrowu.c  $(HACK_H)
$(O)objnam.1:   $(PCH1) $(SRC)\objnam.c   $(HACK_H)
$(O)polyself.1: $(PCH1) $(SRC)\polyself.c $(HACK_H)
$(O)rnd.1:      $(PCH1) $(SRC)\rnd.c      $(HACK_H)
$(O)shk.1:      $(PCH1) $(SRC)\shk.c      $(HACK_H) $(ESHK_H)
$(O)steal.1:    $(PCH1) $(SRC)\steal.c    $(HACK_H)
$(O)timeout.1:	$(PCH1) $(SRC)\timeout.c  $(HACK_H) $(LEV_H)
$(O)track.1:    $(PCH1) $(SRC)\track.c    $(HACK_H)
$(O)trap.1:     $(PCH1) $(SRC)\trap.c     $(HACK_H)
$(O)weapon.1:   $(PCH1) $(SRC)\weapon.c   $(HACK_H)
$(O)zap.1:      $(PCH1) $(SRC)\zap.c      $(HACK_H)

#
# OVL2
#

$(O)attrib.2:   $(PCH2) $(SRC)\attrib.c   $(HACK_H)
$(O)do.2:       $(PCH2) $(SRC)\do.c       $(HACK_H) $(LEV_H)
$(O)do_name.2:  $(PCH2) $(SRC)\do_name.c  $(HACK_H)
$(O)do_wear.2:  $(PCH2) $(SRC)\do_wear.c  $(HACK_H)
$(O)dog.2:      $(PCH2) $(SRC)\dog.c      $(HACK_H) $(EDOG_H)
$(O)engrave.2:  $(PCH2) $(SRC)\engrave.c  $(HACK_H) $(LEV_H)
$(O)hack.2:     $(PCH2) $(SRC)\hack.c     $(HACK_H)
$(O)hacklib.2:  $(PCH2) $(SRC)\hacklib.c  $(HACK_H)
$(O)invent.2:   $(PCH2) $(SRC)\invent.c   $(HACK_H) $(ARTIFACT_H)
$(O)makemon.2:  $(PCH2) $(SRC)\makemon.c  $(HACK_H) $(EPRI_H) $(EMIN_H)
$(O)mon.2:      $(PCH2) $(SRC)\mon.c      $(HACK_H) $(MFNDPOS_H) $(EDOG_H)
$(O)mondata.2:  $(PCH2) $(SRC)\mondata.c  $(HACK_H) $(ESHK_H) $(EPRI_H)
$(O)monmove.2:  $(PCH2) $(SRC)\monmove.c  $(HACK_H) $(MFNDPOS_H) $(ARTIFACT_H)
$(O)shk.2:      $(PCH2) $(SRC)\shk.c      $(HACK_H) $(ESHK_H)
$(O)trap.2:     $(PCH2) $(SRC)\trap.c     $(HACK_H)
$(O)zap.2:      $(PCH2) $(SRC)\zap.c      $(HACK_H)

#
# OVL3
#

$(O)do.3:       $(PCH3) $(SRC)\do.c       $(HACK_H) $(LEV_H)
$(O)hack.3:     $(PCH3) $(SRC)\hack.c     $(HACK_H)
$(O)invent.3:	$(PCH3) $(SRC)\invent.c   $(HACK_H) $(ARTIFACT_H)
$(O)light.3:	$(PCH3) $(SRC)\light.c	  $(HACK_H)
$(O)shk.3:      $(PCH3) $(SRC)\shk.c      $(HACK_H) $(ESHK_H)
$(O)trap.3:     $(PCH3) $(SRC)\trap.c     $(HACK_H)
$(O)zap.3:      $(PCH3) $(SRC)\zap.c      $(HACK_H)

#
# OVLB
#

$(O)allmain.B:  $(PCHB) $(SRC)\allmain.c  $(HACK_H)
$(O)apply.B:    $(PCHB) $(SRC)\apply.c    $(HACK_H) $(EDOG_H)
$(O)artifact.B: $(PCHB) $(SRC)\artifact.c $(HACK_H) $(ARTIFACT_H) $(ARTILIST_H)
$(O)attrib.B:   $(PCHB) $(SRC)\attrib.c   $(HACK_H)
$(O)botl.B:     $(PCHB) $(SRC)\botl.c     $(HACK_H)
$(O)cmd.B:      $(PCHB) $(SRC)\cmd.c      $(HACK_H) $(FUNC_TAB_H)
$(O)dbridge.B:  $(PCHB) $(SRC)\dbridge.c  $(HACK_H)
$(O)do.B:       $(PCHB) $(SRC)\do.c       $(HACK_H) $(LEV_H)
$(O)do_name.B:  $(PCHB) $(SRC)\do_name.c  $(HACK_H)
$(O)do_wear.B:  $(PCHB) $(SRC)\do_wear.c  $(HACK_H)
$(O)dog.B:      $(PCHB) $(SRC)\dog.c      $(HACK_H) $(EDOG_H)
$(O)dogmove.B:  $(PCHB) $(SRC)\dogmove.c  $(HACK_H) $(MFNDPOS_H) $(EDOG_H)
$(O)eat.B:      $(PCHB) $(SRC)\eat.c      $(HACK_H)
$(O)engrave.B:  $(PCHB) $(SRC)\engrave.c  $(HACK_H) $(LEV_H)
$(O)hack.B:     $(PCHB) $(SRC)\hack.c     $(HACK_H)
$(O)hacklib.B:  $(PCHB) $(SRC)\hacklib.c  $(HACK_H)
$(O)invent.B:   $(PCHB) $(SRC)\invent.c   $(HACK_H) $(ARTIFACT_H)
$(O)lock.B:     $(PCHB) $(SRC)\lock.c     $(HACK_H)
$(O)mail.B:     $(PCHB) $(SRC)\mail.c     $(HACK_H) $(MAIL_H) $(PATCHLEVEL_H)
$(O)makemon.B:  $(PCHB) $(SRC)\makemon.c  $(HACK_H) $(EPRI_H) $(EMIN_H)
$(O)mcastu.B:   $(PCHB) $(SRC)\mcastu.c   $(HACK_H)
$(O)mhitm.B:    $(PCHB) $(SRC)\mhitm.c    $(HACK_H) $(ARTIFACT_H) $(EDOG_H)
$(O)mhitu.B:    $(PCHB) $(SRC)\mhitu.c    $(HACK_H) $(ARTIFACT_H) $(EDOG_H)
$(O)mkobj.B:    $(PCHB) $(SRC)\mkobj.c    $(HACK_H) $(ARTIFACT_H) $(PROP_H)
$(O)mkroom.B:   $(PCHB) $(SRC)\mkroom.c   $(HACK_H)
$(O)mon.B:      $(PCHB) $(SRC)\mon.c      $(HACK_H) $(MFNDPOS_H) $(EDOG_H)
$(O)mondata.B:  $(PCHB) $(SRC)\mondata.c  $(HACK_H) $(ESHK_H) $(EPRI_H)
$(O)monmove.B:  $(PCHB) $(SRC)\monmove.c  $(HACK_H) $(MFNDPOS_H) $(ARTIFACT_H)
$(O)mthrowu.B:  $(PCHB) $(SRC)\mthrowu.c  $(HACK_H)
$(O)objnam.B:   $(PCHB) $(SRC)\objnam.c   $(HACK_H)
$(O)pline.B:    $(SRC)\pline.c    $(HACK_H) $(EPRI_H)
	$(CC) $(CFLAGSN) ?[CSNAMB]$(COBJNAM)$@ $(SRC)\pline.c
$(O)polyself.B: $(PCHB) $(SRC)\polyself.c $(HACK_H)
$(O)potion.B:   $(PCHB) $(SRC)\potion.c   $(HACK_H)
$(O)priest.B:   $(PCHB) $(SRC)\priest.c   $(HACK_H) $(MFNDPOS_H) $(ESHK_H) \
		$(EPRI_H) $(EMIN_H)
$(O)read.B:     $(PCHB) $(SRC)\read.c     $(HACK_H)
$(O)rnd.B:      $(PCHB) $(SRC)\rnd.c      $(HACK_H)
$(O)shk.B:      $(PCHB) $(SRC)\shk.c      $(HACK_H) $(ESHK_H)
$(O)shknam.B:   $(PCHB) $(SRC)\shknam.c   $(HACK_H) $(ESHK_H)
$(O)sounds.B:   $(PCHB) $(SRC)\sounds.c   $(HACK_H) $(EDOG_H)
$(O)steal.B:    $(PCHB) $(SRC)\steal.c    $(HACK_H)
$(O)timeout.B:	$(PCHB) $(SRC)\timeout.c  $(HACK_H) $(LEV_H)
$(O)track.B:    $(PCHB) $(SRC)\track.c    $(HACK_H)
$(O)trap.B:     $(PCHB) $(SRC)\trap.c     $(HACK_H)
$(O)vault.B:    $(PCHB) $(SRC)\vault.c    $(HACK_H) $(VAULT_H)
$(O)weapon.B:   $(PCHB) $(SRC)\weapon.c   $(HACK_H)
$(O)were.B:     $(PCHB) $(SRC)\were.c     $(HACK_H)
$(O)wizard.B:   $(PCHB) $(SRC)\wizard.c   $(HACK_H) $(QTEXT_H)
$(O)zap.B:      $(PCHB) $(SRC)\zap.c      $(HACK_H)

# end of file
