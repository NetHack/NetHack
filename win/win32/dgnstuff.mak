#Set all of these or none of them
#YACC   = byacc.exe
#LEX	= flex.exe
#YTABC   = y_tab.c
#YTABH   = y_tab.h
#LEXYYC  = lexyy.c

!IF "$(YACC)"!=""
@echo Yacc-alike set to $(YACC)
@echo YTABC set to $(YTABC)
@echo YTABH set to $(YTABH)
!ENDIF

!IF "$(LEX)"!=""
@echo Lex-alike set to $(LEX)
@echo LEXYYC set to $(LEXYYC)
!ENDIF

default: all

all: ..\util\dgn_yacc.c ..\util\dgn_lex.c

rebuild: clean all

clean:
	-del ..\util\dgn_lex.c
	-del ..\util\dgn_yacc.c
	-del ..\include\dgn_comp.h

#==========================================
# Dungeon Compiler Stuff
#==========================================

..\util\dgn_yacc.c ..\include\dgn_comp.h : ..\util\dgn_comp.y
!IF "$(YACC)"==""
	   @echo Using pre-built dgn_yacc.c and dgn_comp.h
	   @copy ..\sys\share\dgn_yacc.c ..\util\dgn_yacc.c
	   @copy ..\sys\share\dgn_comp.h ..\include\dgn_comp.h
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
	   @copy ..\sys\share\dgn_lex.c $@
!ELSE
	   chdir ..\util
	   $(LEX) dgn_comp.l
	   copy $(LEXYYC) $@
	   @del $(LEXYYC)
	   chdir ..\build
!ENDIF
