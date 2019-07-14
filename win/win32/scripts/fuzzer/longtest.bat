echo off

SETLOCAL ENABLEEXTENSIONS
SETLOCAL ENABLEDELAYEDEXPANSION

call setenv.bat

if exist %FUZZER_DIR% rmdir /s /q %FUZZER_DIR%

mkdir %FUZZER_DIR%
mkdir %SAVE_DIR%

for /L %%i in (%START_MOVE%, %STEP_SIZE%, %FINAL_MOVE%) do (

    call runtill.bat %%i
	
	if ERRORLEVEL 1 (
	    echo FAILED getting running to %%i.
		exit /b 1
	)

)

echo SUCCESS.


