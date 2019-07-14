set BIN_DIR=..\..\..\..\bin\Debug\Win32

set FUZZER_LOG=%BIN_DIR%\fuzzer.log
set FUZZER_DIR=%BIN_DIR%\fuzzer

if exist %BIN_DIR%\%USERNAME%* del %BIN_DIR%\%USERNAME%*
if exist %FUZZER_LOG% del %FUZZER_LOG%

