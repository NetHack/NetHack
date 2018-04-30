$! NetHack.Com -- sample command procedure for invoking NetHack  9-JAN-1993
$ v = 'f$verify(0)'
$!
$! $NHDT-Date: 1524689428 2018/04/25 20:50:28 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.5 $
$! Copyright (c) 2016 by Robert Patrick Rankin
$! NetHack may be freely redistributed.  See license for details.
$!
$!    Possible command line arguments include
$!	"-uConan-B"	!play a barbarian named Conan
$!	"-u" "Merlin-W" !play a wizard named Merlin (slight variant of above)
$!	"-e" or "-E"	!play an elf with default name (from environment
$!			! [ie, NETHACKOPTIONS logical name] or VMS username)
$!	"-a" or "-A", "-b" or "-B", "-c" or "-C", ... !specify character type
$!			!note: "-s" is ambiguous between "play as a samurai"
$!			!   vs "show scoreboard", so use "-S" for the former
$!	"-x" or "-X"	!play in 'explore' mode (practice for beginners)
$!	"-D"		!play in 'wizard' mode (for debugging, available only
$!			! to the username compiled into nethack.exe as WIZARD)
$!	"-dec"		!turn on DECgraphics mode (VT100 line drawing, done
$!			! automatically below if appropriate term attribs set)
$!	"-d" dir-path	!specify an alternate playground directory (not
$!			! recommended; define HACKDIR instead)
$!
$
$!
$! assume this command procedure has been placed in the playground directory;
$!	 get its device:[directory]
$	hackdir = f$parse("_._;0",f$environ("PROCEDURE")) - "_._;0"
$!
$! hackdir should point to the 'playground' directory
$ if f$trnlnm("HACKDIR").eqs."" then  define hackdir 'hackdir'
$!
$! termcap is a text file defining terminal capabilities and escape sequences
$ if f$trnlnm("TERMCAP").eqs."" then  define termcap hackdir:termcap
$!
! [ obsolete:  now handled within nethack itself ]
! $! prior to VMS v6, the C Run-Time Library doesn't understand vt420 :-(
! $	  TT$_VT400_Series = 113
! $ if f$getdvi("TT:","DEVTYPE").eq.TT$_VT400_Series -
!  .and. f$trnlnm("NETHACK_TERM").eqs."" then  define nethack_term "vt400"
$!
$! use the VT100 line drawing character set if possible
$ graphics = ""
$	usropt = f$trnlnm("NETHACKOPTIONS")
$	if usropt.eqs."" then  usropt = f$trnlnm("HACKOPTIONS")
$ if f$locate("DECG",f$edit(usropt,"UPCASE")) .ge. f$length(usropt) then -
    if f$getdvi("TT:","TT_DECCRT") .and. f$getdvi("TT:","TT_ANSICRT") then -
$	graphics = " -dec"	!select DECgraphics mode by default
$!
$! get input from the terminal, not from this .com file
$ deassign sys$input
$!
$	nethack := $hackdir:nethack
$	if p1.nes."-s" .and. p1.nes."-s all" then -
		nethack = nethack + graphics
$ nethack "''p1'" "''p2'" "''p3'" "''p4'" "''p5'" "''p6'" "''p7'" "''p8'"
$!
