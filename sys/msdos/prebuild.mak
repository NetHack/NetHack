#       SCCS Id: @(#)prebuild.mak       3.4     1997/09/28
#
# Makefile for building the genschem utility, the .def files and
# the Makefiles for distribution.
#

NHINCL     =..\..\include

!       IF "$(MAKE)"=="NMAKE"
CC        =cl
MODEL     =L
CEXENAM   =/Fe                  # name the .EXE file
CFLAGS    =/A$(MODEL) /Zp1 /nologo /F 1400 /D__STDC__ /I$(NHINCL)
!       ELSE                    #       Assume Borland
CC        =bcc                  # TARGSTRING
MODEL     =h
BCTOP     =c:\borlandc          # main Borland C directory
BCINCL    =$(BCTOP)\include     # include directory for main BC headers
CEXENAM   =-e                   # name the .EXE file
CFLAGS    =-I$(BCINCL) -I$(NHINCL) -m$(MODEL) -DSTRNCMPI
!       ENDIF

LEX = flex
#LEX = flex -Sc:\tools16\flex.ske
# LEX = lex

# these are the names of the output files from LEX. Under MS-DOS
# and similar systems, they may differ
LEXYYC = lex.yy.c
#LEXYYC = lexyy.c


SCHEMAS = schema1.BC schema2.BC 
MAKES = Makefile.BC

all: $(SCHEMAS)

genschem.exe: genschem.c
	$(CC) $(CFLAGS) $(CEXENAM)$@ genschem.c

genschem.c: genschem.l
	$(LEX) $(FLEXSKEL) genschem.l
	copy $(LEXYYC) $@
	@del $(LEXYYC)

schema1.BC: genschem.exe schema1
	genschem /BC schema1 schema1.BC
schema2.BC: genschem.exe schema2
	genschem /BC schema2 schema2.BC
#
# NOTE: MSC no longer uses these overlay definitions
# since switching to the use of packaged functions
#
#schema1.MSC: genschem.exe schema1
#	genschem /MSC schema1 schema1.MSC
#schema2.MSC: genschem.exe schema2
#	genschem /MSC schema2 schema2.MSC
#schema3.MSC: genschem.exe schema3
#	genschem /MSC schema3 schema3.MSC

def2mak.exe: def2mak.c
	$(CC) $(CFLAGS) $(CEXENAM)$@ def2mak.c

#Makefile.BC: def2mak.exe template.mak
#	def2mak /BC template.mak >Makefile.BC
#Makefile.MSC: def2mak.exe template.mak
#	def2mak /MSC template.mak >Makefile.MSC
