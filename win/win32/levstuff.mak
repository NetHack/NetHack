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

all: ..\util\lev_yacc.c ..\util\lev_lex.c

rebuild: clean all

clean:
	-del ..\util\lev_lex.c
	-del ..\util\lev_yacc.c
	-del ..\include\lev_comp.h

#==========================================
# Level Compiler Stuff
#==========================================
..\util\lev_yacc.c ..\include\lev_comp.h: ..\util\lev_comp.y
!IF "$(YACC)"==""
	   @echo Using pre-built lev_yacc.c and lev_comp.h
	   @copy ..\sys\share\lev_yacc.c ..\util\lev_yacc.c
	   @copy ..\sys\share\lev_comp.h ..\include\lev_comp.h
!ELSE
	   chdir ..\util
	   $(YACC) -d lev_comp.y
	   copy $(YTABC) $@
	   copy $(YTABH) ..\include\lev_comp.h
	   @del $(YTABC)
	   @del $(YTABH)
	   chdir ..\build
!ENDIF

..\util\lev_lex.c: ..\util\lev_comp.l
!IF "$(LEX)"==""
	   @echo Using pre-built lev_lex.c
	   @copy ..\sys\share\lev_lex.c $@
!ELSE
	   chdir ..\util
	   $(LEX) lev_comp.l
	   copy $(LEXYYC) $@
	   @del $(LEXYYC)
	   chdir ..\build
!ENDIF

