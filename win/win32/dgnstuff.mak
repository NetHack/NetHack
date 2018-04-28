# $NHDT-Date: 1524689255 2018/04/25 20:47:35 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.12 $
# Copyright (c) 2018 by Michael Allison
# NetHack may be freely redistributed.  See license for details.

# Set all of these or none of them.
#
# bison and flex are the ones found in GnuWin32, which
# is probably the easiest set of these tools to find
# on Windows.
# 
#YACC    = bison.exe -y
#LEX     = flex.exe 
#YTABC   = y.tab.c 
#YTABH   = y.tab.h 
#LEXYYC  = lex.yy.c 


default: all

all: tools ..\util\dgn_yacc.c ..\util\dgn_lex.c

rebuild: clean all

clean:
	-del ..\util\dgn_lex.c
	-del ..\util\dgn_yacc.c
	-del ..\include\dgn_comp.h

tools:
!IFDEF YACC
	@echo Yacc-alike set to $(YACC)
	@echo YTABC set to $(YTABC)
	@echo YTABH set to $(YTABH)
!ENDIF

!IFDEF LEX
	@echo Lex-alike set to $(LEX)
	@echo LEXYYC set to $(LEXYYC)
!ENDIF


#==========================================
# Dungeon Compiler Stuff
#==========================================

..\include\dgn_comp.h : ..\util\dgn_comp.y
!IF "$(YACC)"==""
	   @echo Using pre-built dgn_comp.h
	   chdir ..\include
	   copy /y ..\sys\share\dgn_comp.h
	   copy /b dgn_comp.h+,,
	   chdir ..\src
!ELSE
	   chdir ..\util
	   $(YACC) -d dgn_comp.y
	   copy $(YTABC) $@
	   copy $(YTABH) ..\include\dgn_comp.h
	   @del $(YTABC)
	   @del $(YTABH)
	   chdir ..\build
!ENDIF

..\util\dgn_yacc.c : ..\util\dgn_comp.y
!IF "$(YACC)"==""
	   @echo Using pre-built dgn_yacc.c
	   chdir ..\util
	   copy /y ..\sys\share\dgn_yacc.c
	   copy /b dgn_yacc.c+,,
	   chdir ..\src
!ELSE
	   chdir ..\util
	   $(YACC) -d dgn_comp.y
	   copy $(YTABC) $@
	   copy $(YTABH) ..\include\dgn_comp.h
	   @del $(YTABC)
	   @del $(YTABH)
	   chdir ..\build
!ENDIF

..\util\dgn_lex.c: ..\util\dgn_comp.l
!IF "$(LEX)"==""
	   @echo Using pre-built dgn_lex.c
	   chdir ..\util
	   copy /y ..\sys\share\dgn_lex.c
	   copy /b dgn_lex.c+,,
	   chdir ..\src
!ELSE
	   chdir ..\util
	   $(LEX) dgn_comp.l
	   copy $(LEXYYC) $@
	   @del $(LEXYYC)
	   chdir ..\build
!ENDIF
