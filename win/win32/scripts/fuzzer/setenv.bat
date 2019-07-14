echo on

set STEP_SIZE=5000
set FINAL_MOVE=500000
set START_MOVE=5000

set BIN_DIR=..\..\..\..\bin\Debug\Win32
set SAVED_GAME=wizard.NetHack-saved-game
set LOG_FILE=%BIN_DIR%\runtil.log
set FUZZER_LOG=%BIN_DIR%\fuzzer.log
set FUZZER_DIR=%BIN_DIR%\fuzzer
set SAVE_DIR=%FUZZER_DIR%\save
set BASELINE=%FUZZER_DIR%\fuzzer.log
