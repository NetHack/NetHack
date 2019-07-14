call clean.bat

set BIN_DIR=..\..\..\..\bin\Debug\Win32
set SAVED_GAME=%USERNAME%-wizard.NetHack-saved-game
set FUZZER_DIR=%BIN_DIR%\fuzzer

copy %FUZZER_DIR%\%SAVED_GAME% %BIN_DIR%\%SAVED_GAME%
