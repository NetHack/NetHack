$! vmsunpack.com -- unpack NetHack's *.taz or *.tzu archive packages	[pr]
$!		into individual source files, creating subdirectories
$!		as needed.  The current directory must hold the input
$!		packages and will become the 'top' of Nethack source tree.
$!
$! Site-specific setup--define appropriate commands for unpacking operations.
$	uudecode   := $rpr:uudecode
$	!! uncompress := $rpr:lzdcmp -b
$	uncompress := $pub:gzip -d
$	untar      := $rpr:tar2vms xv "!"
$	tar_setup  := define/user_mode TAPE
$! [Nothing below this line should need to be changed.]
$!
$! Operation (make sure that all archive packages are named correctly):
$!	uudecode   SOMETHING.tzu -> SOMETHING.taz
$!	uncompress SOMETHING.taz -> SOMETHING.tar
$!	tar_setup  SOMETHING.tar -> preparation for poor tar2vms interface
$!	untar      SOMETHING.tar -> individual files extracted from archive
$!	delete SOMETHING.tar;,SOMETHING.taz;
$!	  note: original .tzu file kept; it should be deleted manually.
$!
$ ARCHIVES1="top dat1 dat2 dat3 dat4 dat5 doc1 doc2 doc3 doc4 doc5 incl1 "-
	   +"incl2 incl3 incl4 incl5 util1 util2"
$ ARCHIVES2="src01 src02 src03 src04 src05 src06 src07 src08 src09 src10 "-
	   +"src11 src12 src13 src14 src15 src16 src17 src18 src19 src20 "-
	   +"src21 src22 src23 src24 src25 src26 src27 src28 src29 src30 "-
	   +"src31 src32 src33 src34 src35"
$ ARCHIVES3="amiga1 amiga2 amiga3 amiga4 amiga5 amiga6 amiga7 amiga8 amiga9 "-
	   +"ami_spl atari be mac1 mac2 mac3 mac4 macold1 macold2"
$ ARCHIVES4="msdos1 msdos2 msdos3 msdos4 msold1 msold2 msold3 nt_sys os2 "-
	   +"shr_sys1 shr_sys2 shr_sys3 sound1 sound2 sound3 sound4 sound5"
$ ARCHIVES5="unix1 unix2 vms1 vms2 vms3 shr_win1 shr_win2 shr_win3 shr_win4 "-
	   +"tty1 tty2 nt_win x11-1 x11-2 x11-3 x11-4 x11-5 dev1 dev2 dev3 dev4"
$ all = f$edit(ARCHIVES1+" "+ARCHIVES2+" "+ARCHIVES3+" "+ARCHIVES4+" "+ARCHIVES5,"COMPRESS")
$ kits = "top    |dat    |doc    |incl   |util   |src    |amiga  |ami_spl|"-
       + "atari  |be     |mac    |macold |msdos  |msold  |nt_sys |os2    |"-
       + "shr_sys|sound  |unix   |vms    |shr_win|tty    |nt_win |x11-   |"-
       + "dev    |"
$ dirs = "[],[.dat],[.doc],[.include],[.util],[.src],"-
       + "[.sys.amiga],[.sys.amiga.splitter],[.sys.atari],[.sys.be],"-
       + "[.sys.mac],[.sys.mac.old],[.sys.msdos],[.sys.msdos.old],"-
       + "[.sys.winnt],[.sys.os2],[.sys.share],[.sys.share.sounds],"-
       + "[.sys.unix],[.sys.vms],"-
       + "[.win.share],[.win.tty],[.win.win32],[.win.X11],[]"
$! VMS can live without these:
$ skippable = ":amiga:ami_spl:atari:be:mac:macold:msdos:msold:nt_sys:os2:"-
       + "sound:unix:shr_win:nt_win:x11-:dev:"
$!
$ if f$parse("[.sys]").eqs."" then  create/dir [.sys]/log
$ if f$parse("[.win]").eqs."" then  create/dir [.win]/log
$!
$! First handle some miscellaneous files --what a nuisance :-(
$ if f$search("shr_tc.uu").nes.""
$ then	if f$parse("[.sys.share]").eqs."" then  create/dir [.sys.share]/log
$	rename/new_vers shr_tc.uu [.sys.share]termcap.uu
$ endif
$ if f$search("mac-snd.hqx").nes.""
$ then	if f$parse("[.sys.mac]").eqs."" then  create/dir [.sys.mac]/log
$	rename/new_vers mac-snd.hqx [.sys.mac]NHsound.hqx
$ endif
$ if f$search("mac-proj.hqx").nes.""
$ then	if f$parse("[.sys.mac.old]").eqs."" then  create/dir [.sys.mac.old]/log
$	rename/new_vers mac-proj.hqx [.sys.mac.old]NHproj.hqx
$ endif
$ if f$search("cpp%.shr").nes.""
$ then	if f$parse("[.sys.unix]").eqs."" then  create/dir [.sys.unix]/log
$	rename/new_vers cpp%.shr [.sys.unix]*.*
$ endif
$! [note: the above files aren't needed for the VMS port.]
$!
$ topdir = f$directory()
$ kitsiz = f$length(f$element(0,"|",kits))+1
$ kits = "|" + f$edit(kits,"UPCASE")
$ k = 0 !count of archive files skipped
$ i = 0 !loop index for archive substring
$loop:
$	f = f$element(i," ",all)
$	if f.eqs."" .or. f.eqs." " then  goto done
$	d = f - "0" - "1" - "2" - "3" - "4" - "5" - "6" - "7" - "8" - "9" -
	      - "0" - "1" - "2" - "3" - "4" - "5" - "6" - "7" - "8" - "9"
$	if d.eqs."os" then  d = "os2"
$	if f$extract(0,2,d).eqs."x-" then  d = "x11-"
$   if f$search("''f'.tar").nes."" then  goto detar
$   if f$search("''f'.taz").nes."" then  goto decompress
$   if f$search("''f'.tzu").nes."" then -
	uudecode 'f'.tzu  !>'f'.taz
$decompress:
$   if f$search("''f'.taz").nes."" then -
 -	!! uncompress 'f'.taz 'f'.tar
	uncompress 'f'.taz
$   if f$search("''f'.tar").nes."" then  goto detar
$!
$!     an expected file wasn't found--that's OK in _some_ instances
$	msg_fmt = "!/ Missing archive file !AS skipped.!/"
$	if d.eqs."dev" then -	!not part of the public distribution
		msg_fmt = "!/ Optional archive file !AS skipped.!/"
$	if f$locate(":"+d+":",skippable).ge.f$length(skippable) then -
		msg_fmt = "!/% Expected archive file !AS is missing!!!/"
$	write sys$output f$fao(msg_fmt,"''f'.tzu")
$	k = k + 1
$	goto skip
$!
$detar:
$	p = f$locate(f$edit("|"+d,"UPCASE"),kits) / kitsiz
$	d = f$element(p,",",dirs)
$	set default 'd'
$	if f$parse("[]").eqs."" then  create/dir []/log
$	write sys$output "Unpacking ''f'.tar into ''d'..."
$	f = topdir + f
$	tar_setup 'f'.tar	!define/user TAPE 'f'.tar
$	untar 'f'.tar		!tar2vms xv
$!
$	if f$search("''f'.tzu").nes."" -
	 .or. f$search("''f'.taz").nes."" then  delete 'f'.tar;
$	if f$search("''f'.tzu").nes."" -
	.and. f$search("''f'.taz").nes."" then  delete 'f'.taz;
$	set default 'topdir'
$skip:
$	i = i + 1
$	goto loop
$done:
$	msg_fmt = "!/ Unpacking completed."
$	if k.gt.0 then -
		msg_fmt = msg_fmt + "  !SL archive package!%S skipped.!/"
$	write sys$output f$fao(msg_fmt,k)
$ exit
