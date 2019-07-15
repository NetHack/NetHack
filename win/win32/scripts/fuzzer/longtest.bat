echo off

SETLOCAL ENABLEEXTENSIONS
SETLOCAL ENABLEDELAYEDEXPANSION

set STEP_SIZE=5000
set FINAL_MOVE=500000
set START_MOVE=5000

for /L %%i in (%START_MOVE%, %STEP_SIZE%, %FINAL_MOVE%) do (

    call runtill.bat %%i
	
	if ERRORLEVEL 1 (
	    echo FAILED getting running to %%i.
		exit /b 1
	)

)

echo SUCCESS.


