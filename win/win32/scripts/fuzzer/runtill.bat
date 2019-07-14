REM
REM runtill target_move
REM 
echo off

SETLOCAL ENABLEEXTENSIONS
SETLOCAL ENABLEDELAYEDEXPANSION

set TARGET_MOVE=%1

if %TARGET_MOVE% == "" (
  echo Usage:runtill target_move
  goto :eof
)

set BIN_DIR=..\..\..\..\bin\Debug\Win32
set SAVED_GAME=%USERNAME%-wizard.NetHack-saved-game
set LOG_FILE=%BIN_DIR%\runtil.log
set FUZZER_LOG=%BIN_DIR%\fuzzer.log
set FUZZER_DIR=%BIN_DIR%\fuzzer
set SAVE_DIR=%FUZZER_DIR%\save
set BASELINE=%FUZZER_DIR%\fuzzer.log

if not exist %FUZZER_DIR% mkdir %FUZZER_DIR%
if not exist %SAVE_DIR% mkdir %SAVE_DIR%

call clean.bat

if not exist %FUZZER_DIR%\%SAVED_GAME% (
	%BIN_DIR%\nethack -D -F 0

	copy %BIN_DIR%\%SAVED_GAME% %FUZZER_DIR%
	copy %BIN_DIR%\%SAVED_GAME% %SAVE_DIR%\0.save
)

call restore.bat

%BIN_DIR%\nethack -D -F %TARGET_MOVE%

move %BIN_DIR%\*.snap %BIN_DIR%\snapshots
copy %FUZZER_LOG% %BASELINE%

for /f "tokens=2,3 delims=: usebackq" %%i in (`findstr /c:START %BASELINE%`) do (
	set START_SEED=%%j
	set START_MOVE=%%i
)

for /f "tokens=2,3 delims=: usebackq" %%i in (`findstr /c:STOP %BASELINE%`) do (
	set STOP_SEED=%%j
	set STOP_MOVE=%%i
)

if !STOP_MOVE! LSS %TARGET_MOVE% (
	cls
	echo FAILED: Failed to reach target move. !STOP_MOVE! is not GTE %TARGET_MOVE%.
	exit /b 1
)

call restore.bat

%BIN_DIR%\nethack -D -F %TARGET_MOVE%

fc %FUZZER_LOG% %BASELINE%
  
if ERRORLEVEL 1 (
	cls
	echo FAILED: Unable to reproduce same timeline
	exit /b 1
)

del /q %FUZZER_DIR%\%SAVED_GAME%

copy %BIN_DIR%\%SAVED_GAME% %FUZZER_DIR%
copy %BIN_DIR%\%SAVED_GAME% %SAVE_DIR%\!STOP_MOVE!.save

echo !START_MOVE! to !STOP_MOVE!.
echo SUCCESS.
