# gulam shell script -- should work with tcsh and many
# other Atari shells, too

# UNIX shells use '/' in file names, but at least some Atari shells need '\'
# so we process the UNIX makefiles to make that switch

# sed script not included as a here document in this script because at
# least some Atari shells don't do that

sed -f unx2atar.sed < ..\unix\Makefile.top > ..\..\Makefile
sed -f unx2atar.sed < ..\unix\Makefile.dat > ..\..\dat\Makefile
sed -f unx2atar.sed < ..\unix\Makefile.doc > ..\..\doc\Makefile
sed -f unx2atar.sed < ..\unix\Makefile.src > ..\..\src\Makefile
sed -f unx2atar.sed < ..\unix\Makefile.utl > ..\..\util\Makefile

# KLUDGE to fix a Makefile problem
echo > ..\..\include\win32api.h
