@echo off
REM  MSDOS, Windows NT NetHack distribution unpacking routine 96/03/28

REM ***************************************
REM * Housekeeping Section. DO NOT MODIFY *
REM ***************************************
REM This is just housekeeping, to prevent clobbering environment
REM variables already in use on system. Skip to the "Modify" section.
REM
set _O1=%_arch%
set _O2=%uncompress%
set _O3=%untar%
set _O4=%uudecode%
set _O5=%newdirmsg%
set _O6=%topdirmsg%
set _O7=%blankline%
set _O8=%F%
set _O9=%E%
set _OA=%compress%
set _OB=%unixcomp%

REM ***********************************
REM * Modify this section as required *
REM ***********************************
REM  Location of distribution (change as necessary or overide with param 1)
set _arch=\nhdev\320\arch
if NOT "%1"=="" set _arch=%1
if NOT exist %_arch%\*.* goto invalid

REM Note that output is a new tree beginning with your current directory

REM Utilities (change as necessary)
set untar=tar -xvf
set uncompress=comp16 -d -v
set compress=comp16 -v
set uudecode=uudecode

REM If your Unix-compatible decompressor changes the .taz to another extension
REM such as dropping the z, please specify the resulting extension here
set E=ta

REM The final step of this procedure can recompress the .tar files if you wish.
REM If you would prefer not to do this unix-style file-by-file compression, 
REM perhaps because you would rather use a .zip utility to recompress the 
REM files yourself, or because you have ample disk space and would rather 
REM not devote your machine's time to recompressing the files, 
REM leave the REM in front of the "set unixcomp=Y" line below.
REM Remove the REM from the statement to perform the unix-style compression.

REM set unixcomp=Y

REM Some short-forms
set topdirmsg=echo Returning to top directory
set newdirmsg=echo Entering directory
set blankline=echo.

REM *************************************************
REM * Nothing below here should need to be changed  *
REM *************************************************

REM * Make required subdirectory trees *
if not exist sys\nul mkdir sys
if not exist win\nul mkdir win

REM * top *
%blankline%
echo Entering Top directory
set F=%_arch%\top
if not exist %F%.* goto endtop
if exist %F%.taz %uncompress% %F%.taz
if exist %F%.%E% ren %F%.%E% *.tar
if exist %F%.tar %untar% %F%.tar
%blankline%
:endtop

REM * src *
set F=%_arch%\src
if not exist %F%*.* goto endsrc
if not exist src\nul mkdir src
%newdirmsg% src
chdir src
if exist %F%*.taz for %%N in (%F%*.taz) do %uncompress% %%N
if exist %F%*.%E% ren %F%*.%E% *.tar
if exist %F%*.tar for %%N in (%F%*.tar) do %untar% %%N
%topdirmsg%
%blankline%
chdir ..
:endsrc

REM * util *
set F=%_arch%\util
if not exist %F%*.* goto endutil
if not exist util\nul mkdir util
%newdirmsg% util
chdir util
if exist %F%*.taz for %%N in (%F%*.taz) do %uncompress% %%N
if exist %F%*.%E% ren %F%*.%E% *.tar
if exist %F%*.tar for %%N in (%F%*.tar) do %untar% %%N
%topdirmsg%
%blankline%
chdir ..
:endutil

REM * doc *
set F=%_arch%\doc
if not exist %F%*.* goto enddoc
if not exist doc\nul mkdir doc
%newdirmsg% doc
chdir doc
if exist %F%*.taz for %%N in (%F%*.taz) do %uncompress% %%N
if exist %F%*.%E% ren %F%*.%E% *.tar
if exist %F%*.tar for %%N in (%F%*.tar) do %untar% %%N
%topdirmsg%
%blankline%
chdir ..
:enddoc

REM * dat *
set F=%_arch%\dat
if not exist %F%*.* goto enddat
if not exist dat\nul mkdir dat
%newdirmsg% dat
chdir dat
if exist %F%*.taz for %%N in (%F%*.taz) do %uncompress% %%N
if exist %F%*.%E% ren %F%*.%E% *.tar
if exist %F%*.tar for %%N in (%F%*.tar) do %untar% %%N
%topdirmsg%
%blankline%
chdir ..
:enddat

REM * include *
set F=%_arch%\incl
if not exist %F%*.* goto endincl
if not exist include\nul mkdir include
%newdirmsg% include
chdir include
if exist %F%*.taz for %%N in (%F%*.taz) do %uncompress% %%N
if exist %F%*.%E% ren %F%*.%E% *.tar
if exist %F%*.tar for %%N in (%F%*.tar) do %untar% %%N
%topdirmsg%
%blankline%
chdir ..
:endincl

REM ** Windows NT system **
set F=%_arch%\nt_sys
if not exist %F%*.* goto endnt
if not exist sys\winnt\nul mkdir sys\winnt
%newdirmsg% sys\winnt
chdir sys\winnt
if exist %F%.taz %uncompress% %F%.taz
if exist %F%.%E% ren %F%.%E% *.tar
if exist %F%.tar %untar% %F%.tar
%topdirmsg%
%blankline%
chdir ..\..
:endnt

REM * Shared system **
set F=%_arch%\shr_sys
if not exist %_arch%\shr_sys*.* goto endshr
if not exist sys\share\nul mkdir sys\share
%newdirmsg% sys\share
chdir sys\share
if exist %F%*.taz for %%N in (%F%*.taz) do %uncompress% %%N
if exist %F%*.%E% ren %F%*.%E% *.tar
if exist %F%*.tar for %%N in (%F%*.tar) do %untar% %%N
%topdirmsg%
%blankline%
chdir ..\..
:endshr

REM * termcap **
set F=%_arch%\shr_tc
if not exist %F%.uu goto endtc
if not exist sys\share\nul mkdir sys\share
%newdirmsg% sys\share
chdir sys\share
if exist termcap.zip del termcap.zip
if exist %F%.uu %uudecode% %F%.uu
%topdirmsg%
%blankline%
chdir ..\..
:endtc

REM * Shared sound *
set F=%_arch%\sound
if not exist %F%*.* goto endsnd
if not exist sys\share\sound\nul mkdir sys\share\sound
%newdirmsg% sys\share\sound
chdir sys\share\sound
if exist %F%*.taz for %%N in (%F%*.taz) do %uncompress% %%N
if exist %F%*.%E% ren %F%*.%E% *.tar
if exist %F%*.tar for %%N in (%F%*.tar) do %untar% %%N
%topdirmsg%
%blankline%
chdir ..\..\..
:endsnd

REM ** Amiga *
set F=%_arch%\amiga
if not exist %F%*.* goto endamiga
if not exist sys\amiga\nul mkdir sys\amiga
if not exist sys\amiga\splitter\nul mkdir sys\amiga\splitter
%newdirmsg% sys\amiga
chdir sys\amiga
if exist %F%*.taz for %%N in (%F%*.taz) do %uncompress% %%N
if exist %F%*.%E% ren %F%*.%E% *.tar
if exist %F%*.tar for %%N in (%F%*.tar) do %untar% %%N
%newdirmsg% sys\amiga\splitter
chdir splitter
set F=%_arch%\ami_spl
if exist %F%.taz %uncompress% %F%.taz
if exist %F%.%E% ren %F%.%E% *.tar
if exist %F%.tar %untar% %F%.tar
%topdirmsg%
%blankline%
chdir ..\..\..
:endamiga

REM ** Atari *
set F=%_arch%\atari
if not exist %F%*.* goto endatari
if not exist sys\atari\nul mkdir sys\atari
%newdirmsg% sys\atari
chdir sys\atari
if exist %F%.taz %uncompress% %F%.taz
if exist %F%.%E% ren %F%.%E% *.tar
if exist %F%.tar %untar% %F%.tar
%topdirmsg%
%blankline%
chdir ..\..
:endatari

REM ** Be OS *
set F=%_arch%\be
if not exist %F%*.* goto endbe
if not exist sys\be\nul mkdir sys\be
%newdirmsg% sys\be
chdir sys\be
if exist %F%.taz %uncompress% %F%.taz
if exist %F%.%E% ren %F%.%E% *.tar
if exist %F%.tar %untar% %F%.tar
%topdirmsg%
%blankline%
chdir ..\..
:endbe

REM * mac *
set F=%_arch%\mac
if not exist %F%*.* goto endmac
if not exist sys\mac\nul mkdir sys\mac
if not exist sys\mac\old\nul mkdir sys\mac\old
%newdirmsg% sys\mac
chdir sys\mac
if exist %F%?.taz for %%N in (%F%?.taz) do %uncompress% %%N
if exist %F%?.%E% ren %F%?.%E% *.tar
if exist %F%?.tar for %%N in (%F%?.tar) do %untar% %%N
%newdirmsg% sys\mac\old
chdir old
set F=%_arch%\macold
if exist %F%*.taz for %%N in (%F%*.taz) do %uncompress% %%N
if exist %F%*.%E% ren %F%*.%E% *.tar
if exist %F%*.tar for %%N in (%F%*.tar) do %untar% %%N
%topdirmsg%
%blankline%
chdir ..\..\..
:endmac

REM ** msdos *
set F=%_arch%\msdos
if not exist %F%*.* goto endmsdos
if not exist sys\msdos\nul mkdir sys\msdos
if not exist sys\msdos\old\nul mkdir sys\msdos\old
%newdirmsg% sys\msdos
chdir sys\msdos
if exist %F%*.taz for %%N in (%F%*.taz) do %uncompress% %%N
if exist %F%*.%E% ren %F%*.%E% *.tar
if exist %F%*.tar for %%N in (%F%*.tar) do %untar% %%N
%newdirmsg% sys\msdos\old
chdir old
set F=%_arch%\msold
if exist %F%*.taz for %%N in (%F%*.taz) do %uncompress% %%N
if exist %F%*.%E% ren %F%*.%E% *.tar
if exist %F%*.tar for %%N in (%F%*.tar) do %untar% %%N
%topdirmsg%
%blankline%
chdir ..\..\..
:endmsdos

REM * OS/2 *
set F=%_arch%\os2
if not exist %F%*.* goto endos2
if not exist sys\os2\nul mkdir sys\os2
%newdirmsg% sys\os2
chdir sys\os2
if exist %F%.taz %uncompress% %F%.taz
if exist %F%.%E% ren %F%.%E% *.tar
if exist %F%.tar %untar% %F%.tar
%topdirmsg%
%blankline%
chdir ..\..
:endos2

REM * Unix *
set F=%_arch%\unix
if not exist %F%*.* goto endunix
if not exist sys\unix\nul mkdir sys\unix
%newdirmsg% sys\unix
chdir sys\unix
if exist %F%*.taz for %%N in (%F%*.taz) do %uncompress% %%N
if exist %F%*.%E% ren %F%*.%E% *.tar
if exist %F%*.tar for %%N in (%F%*.tar) do %untar% %%N
%topdirmsg%
%blankline%
chdir ..\..
:endunix

REM * VMS *
set F=%_arch%\vms
if not exist %F%*.* goto endvms
if not exist sys\vms\nul mkdir sys\vms
%newdirmsg% sys\vms
chdir sys\vms
if exist %F%*.taz for %%N in (%F%*.taz) do %uncompress% %%N
if exist %F%*.%E% ren %F%*.%E% *.tar
if exist %F%*.tar for %%N in (%F%*.tar) do %untar% %%N
copy %_arch%\vmsunp*.com
%topdirmsg%
%blankline%
chdir ..\..
:endvms

REM * TTY Window port *
set F=%_arch%\tty
if not exist %F%*.* goto endtty
if not exist win\tty\nul mkdir win\tty
%newdirmsg% win\tty
chdir win\tty
if exist %F%*.taz for %%N in (%F%*.taz) do %uncompress% %%N
if exist %F%*.%E% ren %F%*.%E% *.tar
if exist %F%*.tar for %%N in (%F%*.tar) do %untar% %%N
%topdirmsg%
%blankline%
chdir ..\..
:endtty

REM * win32 *
set F=%_arch%\nt_win
if not exist %F%.* goto endwin32
if not exist win\win32\nul mkdir win\win32
%newdirmsg% win\win32
chdir win\win32
if exist %F%.taz %uncompress% %F%.taz
if exist %F%.%E% ren %F%.%E% *.tar
if exist %F%.tar %untar% %F%.tar
%topdirmsg%
%blankline%
chdir ..\..
:endwin32

REM ** shared window stuff *
set F=%_arch%\shr_win
if not exist %F%*.* goto endwshr
if not exist win\share\nul mkdir win\share
%newdirmsg% win\share
chdir win\share
if exist %F%*.taz for %%N in (%F%*.taz) do %uncompress% %%N
if exist %F%*.%E% ren %F%*.%E% *.tar
if exist %F%*.tar for %%N in (%F%*.tar) do %untar% %%N
%topdirmsg%
%blankline%
chdir ..\..
:endwshr

REM * X11 *
set F=%_arch%\x11
if not exist %F%*.* goto endx11
if not exist win\x11\nul mkdir win\x11
%newdirmsg% win\x11
chdir win\x11
if exist %F%*.taz for %%N in (%F%*.taz) do %uncompress% %%N
if exist %F%*.%E% ren %F%*.%E% *.tar
if exist %F%*.tar for %%N in (%F%*.tar) do %untar% %%N
%topdirmsg%
%blankline%
chdir ..\..
:endx11

REM * dev *
set F=%_arch%\dev
if not exist %F%*.* goto enddev
if exist %F%*.taz for %%N in (%F%*.taz) do %uncompress% %%N
if exist %F%*.%E% ren %F%*.%E% *.tar
if exist %F%*.tar for %%N in (%F%*.tar) do %untar% %%N
:enddev

:comp
if "%unixcomp%"=="" goto endcomp
echo Recompressing the tar files to save disk space.
set F=%_arch%\
if not exist %F%*.tar goto endcomp
if exist %F%*.tar for %%N in (%F%*.tar) do %compress% %%N
%blankline%
:endcomp

echo Unpacking of NetHack completed.

goto done

:invalid
echo The directory you specified as containing a NetHack distribution
echo did not exist or was invalid.  This procedure was looking for files
echo %_arch%\*.*

:done
set _arch=%_O1%
set uncompress=%_O2%
set untar=%_O3%
set uudecode=%_O4%
set newdirmsg=%_O5%
set topdirmsg=%_O6%
set blankline=%_O7%
set F=%_O8%
set E=%_O9%
set compress=%_OA%
set unixcomp=%_OB%
set _O1=
set _O2=
set _O3=
set _O4=
set _O5=
set _O6=
set _O7=
set _O8=
set _O9=
set _OA=
set _OB=

