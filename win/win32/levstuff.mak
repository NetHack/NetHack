# $NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$

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

all: tools ..\util\lev_yacc.c ..\util\lev_lex.c

rebuild: clean all

clean:
	-del ..\util\lev_lex.c
	-del ..\util\lev_yacc.c
	-del ..\include\lev_comp.h

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
# Level Compiler Stuff
#==========================================

..\util\lev_yacc.c ..\include\lev_comp.h: ..\util\lev_comp.y
!IFNDEF YACC
	   @echo Using pre-built lev_yacc.c and lev_comp.h
	   @copy ..\sys\share\lev_yacc.c ..\util\lev_yacc.c
	   @copy ..\sys\share\lev_comp.h ..\include\lev_comp.h
!ELSE
	   @echo Generating lev_yacc.c and lev_comp.h
	   chdir ..\util
	   $(YACC) -d lev_comp.y
	   copy $(YTABC) $@
	   copy $(YTABH) ..\include\lev_comp.h
	   @del $(YTABC)
	   @del $(YTABH)
	   chdir ..\build
!ENDIF

..\util\lev_lex.c: ..\util\lev_comp.l
!IFNDEF LEX
	   @echo Using pre-built lev_lex.c
	   @copy ..\sys\share\lev_lex.c $@
!ELSE
	   @echo Generating lev_lex.c
	   chdir ..\util
	   $(LEX) lev_comp.l
	   copy $(LEXYYC) $@
	   @del $(LEXYYC)
	   chdir ..\build
!ENDIF

