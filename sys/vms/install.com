$ ! vms/install.com -- set up nethack 'playground'
$! $NHDT-Date: 1573172443 2019/11/08 00:20:43 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.12 $
$! Copyright (c) 2016 by Robert Patrick Rankin
$! NetHack may be freely redistributed.  See license for details.
$ !
$ ! $NHDT-Date: 1573172452 2019/11/08 00:20:52 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.12 $
$ !
$ ! Use vmsbuild.com to create nethack.exe, makedefs, and lev_comp *first*.
$ !
$ ! Note: this command procedure is also used by the top level Makefile
$ ! if you build and install with MMS or MMK.  In that situation, only the
$ ! Makefile will need any editing.
$ !
$ ! Edit this file to define gamedir & gameuic, or else invoke it with two
$ ! command line parameters, as in:
$ !	@[.sys.vms]install "disk$users:[games.nethack]" "games"
$ ! or	@[.sys.vms]install "[-.play]" "[40,1]"
$ !
$	! default location is old playground, default owner is installer
$	gamedir = f$trnlnm("NETHACKDIR")	!location of playground
$	if gamedir.eqs."" then  gamedir = f$trnlnm("HACKDIR")
$	gameuic = f$user()			!owner of playground
$	! --- nothing below this line should need to be changed ---
$	if p1.nes."" then  gamedir := 'p1'
$	if p2.nes."" then  gameuic := 'p2'
$
$	! note: all filespecs contain some punctuation,
$	!	to avoid inadvertent logical name interaction
$	play_files = "PERM.,RECORD.,LOGFILE.,XLOGFILE.,PANICLOG."
$	help_files = "HELP.,HH.,CMDHELP.,KEYHELP.,WIZHELP.,OPTHELP.," -
		   + "HISTORY.,LICENSE."
$	data_files = "DATA.,RUMORS.,ORACLES.,OPTIONS.,QUEST.DAT,TRIBUTE.," -
		   + "ENGRAVE.,EPITAPH.,BOGUSMON."
$	sysconf_file = "[.sys.vms]sysconf"
$	guidebook  = "[.doc]Guidebook.txt"
$	invoc_proc = "[.sys.vms]nethack.com"
$	trmcp_file = "[.sys.share]termcap"
$	spec_files = "air.lua,asmodeus.lua,astral.lua,baalz.lua,"	-
		+ "bigrm-*.lua,castle.lua,earth.lua,fakewiz%.lua,"	-
		+ "fire.lua,juiblex.lua,knox.lua,medusa-%.lua,"		-
		+ "minefill.lua,minetn-%.lua,minend-%.lua,nhlib.lua,"	-
		+ "oracle.lua,orcus.lua,sanctum.lua,soko%-%.lua,"	-
		+ "tower%.lua,valley.lua,water.lua,wizard%.lua,hellfill.lua,tut-%.lua"
$	qstl_files = "%%%-goal.lua,%%%-fil%.lua,%%%-loca.lua,%%%-strt.lua"
$	dngn_files = "dungeon.lua"
$!
$!	spec_files = "AIR.LEV,ASMODEUS.LEV,ASTRAL.LEV,BAALZ.LEV,BIGRM-%.LEV," -
$!		   + "CASTLE.LEV,EARTH.LEV,FAKEWIZ%.LEV,FIRE.LEV," -
$!		   + "JUIBLEX.LEV,KNOX.LEV,MEDUSA-%.LEV,MINEFILL.LEV," -
$!		   + "MINETN-%.LEV,MINEND-%.LEV,ORACLE.LEV,ORCUS.LEV," -
$!		   + "SANCTUM.LEV,SOKO%-%.LEV,TOWER%.LEV,VALLEY.LEV," -
$!		   + "WATER.LEV,WIZARD%.LEV"
$!	spec_input = "bigroom.des castle.des endgame.des " -
$!		   + "gehennom.des knox.des medusa.des mines.des " -
$!		   + "oracle.des sokoban.des tower.des yendor.des"
$!	qstl_files = "%%%-GOAL.LEV,%%%-FIL%.LEV,%%%-LOCA.LEV,%%%-STRT.LEV"
$!	qstl_input = "Arch.des Barb.des Caveman.des Healer.des " -
$!		   + "Knight.des Monk.des Priest.des Ranger.des Rogue.des " -
$!		   + "Samurai.des Tourist.des Wizard.des Valkyrie.des"
$!	dngn_files = "DUNGEON."
$!	dngn_input = "dungeon.pdf"
$!
$	dlb_files  = help_files + "," + data_files + "," -
		   + spec_files + "," + qstl_files + "," + dngn_files
$	data_libry = "nh-data.dlb"
$	xtrn_files = "LICENSE.,HISTORY.,OPTIONS.,SYMBOLS."
$ makedefs := $sys$disk:[-.util]makedefs
$ lev_comp := $sys$disk:[-.util]lev_comp
$ dgn_comp := $sys$disk:[-.util]dgn_comp
$ dlb	   := $sys$disk:[-.util]dlb
$ milestone = "write sys$output f$fao("" !5%T "",0),"
$ if p3.nes."" .and. f$edit(p4,"UPCASE").nes."VERBOSE" then  milestone = "!"
$ echo = "write sys$output"
$ warn = echo	!could be "write sys$error"
$!
$! make sure we've got a playground location
$ gamedir := 'gamedir'
$ if gamedir.eqs."" then  gamedir = "[.play]"	!last ditch default
$ gamedir = f$parse(gamedir,,,,"SYNTAX_ONLY") - ".;"
$ if gamedir.eqs."" then  write sys$error "% must specify playground directory"
$ if gamedir.eqs."" then  exit %x1000002C	!ss$_abort
$
$!
$!	['p3' is used in Makefile.top]
$ if p3.nes."" then  goto make_'p3'
$
$	milestone "<installation...>"
$!
$make_data_plus_dlb:
$make_data:
$	! start from a known location -- [.sys.vms]
$	set default 'f$parse(f$environment("PROCEDURE"),,,"DIRECTORY")'
$! generate miscellaneous data files
$	set default [-.-.dat]	!move to data directory
$	milestone "(data)"
$ makedefs -d	!data.base -> data
$	milestone "(rumors)"
$ makedefs -r	!rumors.tru + rumors.fal -> rumors
$	milestone "(oracles)"
$ makedefs -h	!oracles.txt -> oracles
$	milestone "(dungeon preprocess)"
$ makedefs -s
$	milestone "(engrave, epitaph, bogusmon)"
$! makedefs -e	!dungeon.def -> dungeon.pdf
$!	milestone "(quest text)"
$ makedefs -q	!quest.txt -> quest.dat
$	milestone "(special levels)"
$! lev_comp 'spec_input' !special levels
$!	milestone "(quest levels)"
$! lev_comp 'qstl_input' !quest levels
$!	milestone "(dungeon compile)"
$! dgn_comp 'dngn_input' !dungeon database
$	set default [-]		!move up
$ if p3.nes."" .and. f$edit(p3,"UPCASE").nes."DATA_PLUS_DLB" then  exit
$
$make_dlb:
$	! start from a known location -- [.sys.vms]
$	set default 'f$parse(f$environment("PROCEDURE"),,,"DIRECTORY")'
$! construct data library
$	set default [-.-.dat]	!move to data directory
$	milestone "(dlb setup)"
$! since DLB doesn't support wildcard expansion and we don't have shell
$! file globbing, start by making a file listing its intended contents
$ create nhdat.lst
$	if f$search("nhdat.lst;-1").nes."" then -
		purge/noConfirm/noLog nhdat.lst
$! an old data file might fool us later, so get rid of it
$	if f$search(data_libry).nes."" then -
		delete/noConfirm/noLog 'data_libry';*
$	if f$trnlnm("PFILE$").nes."" then  close/noLog pfile$
$ open/Append pfile$ nhdat.lst
$ i = 0
$dloop:
$   g = f$element(i,",",dlb_files)
$   if g.eqs."," then  goto ddone
$   wild = f$locate("*",g).ne.f$locate("%",g)
$   fcnt = 0
$floop:
$	f = f$search(g)
$	if f.eqs."" then  goto fdone
$	fcnt = fcnt + 1
$! strip device, directory, and version from name
$	f = f$parse(f,,,"NAME") + f$parse(f,,,"TYPE")
$! strip trailing dot, if present, and change case
$	f = f$edit(f + "#" - ".#" - "#","LOWERCASE")
$	if f$extract(3,1,f).eqs."-" then -	!"xyz-foo.lev" -> "Xyz-foo.lev"
		f = f$edit(f$extract(0,1,f),"UPCASE") + f$extract(1,255,f)
$	write pfile$ f
$	if wild then  goto floop
$fdone:
$   if fcnt.eq.0 then  warn "? no file(s) found for """,g,""""
$   i = i + 1
$   goto dloop
$ddone:
$ close pfile$
$	milestone "(dlb create)"
$ dlb "-cfI" 'data_libry' nhdat.lst
$	set default [-]		!move up
$ if p3.nes."" then  exit
$
$!
$! set up the playground and save directories
$	milestone "(directories)"
$make_directories:
$	srctree = f$environment("DEFAULT")
$	set default 'gamedir'
$ if f$parse("[-]").eqs."" then  create/dir/log [-] !default owner & protection
$ if f$parse("[]" ).eqs."" then - !needs to be world writable
   create/directory/owner='gameuic'/prot=(s:rwe,o:rwe,g:rwe,w:rwe)/log []
$ if f$search("SAVE.DIR;1").eqs."" then -
   create/directory/owner='gameuic'/prot=(s:rwe,o:rwe,g:rwe,w:rwe)/log -
	[.SAVE]/version_limit=2
$	set default 'srctree'
$ if p3.nes."" then  exit
$!
$! create empty writeable files -- logfile, scoreboard, multi-user access lock
$! [if old versions are already present, validate and retain them if possible]
$make_writeable_files:
$	milestone "(writeable files)"
!-!$ create/owner='gameuic'/prot=(s:rwed,o:rwed,g:rwed,w:rwed) -
!-!	'gamedir''play_files'
$	i = 0
$ploop:	if f$trnlnm("PFILE$").nes."" then  close/nolog pfile$
$	f = f$element(i,",",play_files)
$	if f.eqs."," then  goto pdone
$	i = i + 1
$	f = gamedir + f
$	if f$search(f).eqs."" then  goto pmake	!make it if not found
$	if f$file_attrib(f,"RFM").nes."STMLF" then  goto prej !must be stream_lf
$	open/read/error=prej pfile$ 'f'
$	read/end=ploop pfile$ pline	!empty is ok
$	close pfile$
$	pfield = f$element(0," ",pline)	!1st field is version number
$	if f$locate(".",pfield).lt.f$length(pfield) then  goto ploop	!keep
$prej:	rename/new_vers 'f' *.old	!reject old version
$pmake:	create/fdl=sys$input:/owner='gameuic' 'f'/log
file
 organization sequential
 protection (system:rwd,owner:rwd,group:rw,world:rw)
record
 format stream_lf
$	goto ploop
$pdone:
$ if p3.nes."" then  exit
$!
$! copy over the remaining game files, then make them readonly
$make_readonly_files:
$	milestone "(readonly files)"
$ if f$search("[.dat]''data_libry'").nes.""
$ then	call copyfiles 'f$string(data_libry+","+xtrn_files)' [.dat] "r"
$ else	!'dlb_files' is too long for a single command
$	k = 200 + f$locate(",",f$extract(200,999,dlb_files))
$	call copyfiles 'f$extract(0,k,dlb_files)' [.dat] "r"
$	call copyfiles 'f$extract(k+1,999,dlb_files)' [.dat] "r"
$ endif
$ if p3.nes."" then  exit
$!
$make_executable:
$	milestone "(nethack.exe)"
$ call copy_file [.src]nethack.exe 'gamedir'nethack.exe "re"
$ if p3.nes."" then  exit
$!
$! provide invocation procedure (if available)
$make_procedure:
$ if f$search(invoc_proc).eqs."" then  goto skip_dcl
$ if f$search("''gamedir'nethack.com").nes."" then -
    if f$cvtime(f$file_attr("''gamedir'nethack.com","RDT")) -
      .ges. f$cvtime(f$file_attr(invoc_proc,"RDT")) then  goto skip_dcl
$	milestone "(nethack.com)"
$ call copy_file 'invoc_proc' 'gamedir'nethack.com "re"
$skip_dcl:
$ if p3.nes."" then  exit
$!
$! provide plain-text Guidebook doc file (if available)
$make_documentation:
$ if f$search(guidebook).eqs."" then  goto skip_doc
$	milestone "(Guidebook)"
$ call copy_file 'guidebook' 'gamedir'Guidebook.doc "r"
$skip_doc:
$ if p3.nes."" then  exit
$!
$! provide last-resort termcap file (if available)
$make_termcap:
$ if f$search(trmcp_file).eqs."" then  goto skip_termcap
$ if f$search("''gamedir'termcap").nes."" then  goto skip_termcap
$	milestone "(termcap)"
$ call copy_file 'trmcp_file' 'gamedir'termcap "r"
$skip_termcap:
$ if p3.nes."" then  exit
$!
$! provide template sysconf file (needed if nethack is built w/ SYSCF enabled)
$make_sysconf:
$ if f$search(sysconf_file).eqs."" then  goto skip_sysconf
$ if f$search("''gamedir'sysconf_file").nes."" then  goto skip_sysconf
$       milestone "(sysconf)"
$ call copy_file 'sysconf_file' 'gamedir'sysconf "r"
$!	owner should be able to manually edit sysconf; others shouldn't
$ set file/Prot=(s:rwd,o:rwd,g:r,w:r) 'gamedir'sysconf
$skip_sysconf:
$ if p3.nes."" then  exit
$!
$! done
$	milestone "<done>"
$ define/nolog nethackdir 'gamedir'
$ define/nolog hackdir 'gamedir'
$ echo -
    f$fao("!/ Nethack installation complete. !/ Playground is !AS !/",gamedir)
$ exit
$
$!
$! copy one file, resetting the protection on an earlier version first
$copy_file: subroutine
$ if f$search(p2).nes."" then  set file/Prot=(s:rwed,o:rwed) 'p2'
$ copy/Prot=(s:'p3'wd,o:'p3'wd,g:'p3',w:'p3') 'p1' 'p2'
$ set file/Owner='gameuic'/Prot=(s:'p3',o:'p3') 'p2'
$endsubroutine !copy_file
$
$!
$! copy a comma-separated list of wildcarded files, one file at a time
$copyfiles: subroutine
$ i = 0
$lloop:
$   g = f$element(i,",",p1)
$   if g.eqs."," then  goto ldone
$   g = p2 + g
$   wild = f$locate("*",g).ne.f$locate("%",g)
$   fcnt = 0
$eloop:
$	f = f$search(g)
$	if f.eqs."" then  goto edone
$	fcnt = fcnt + 1
$	f = f - f$parse(f,,,"VERSION")
$	e = f$parse(f,,,"NAME") + f$parse(f,,,"TYPE")
$	call copy_file 'f' 'gamedir''e' "''p3'"
$	if wild then  goto eloop
$edone:
$   if fcnt.eq.0 then  warn "? no file(s) found for """,g,""""
$   i = i + 1
$   goto lloop
$ldone:
$endsubroutine !copyfiles
$
$!<eof>
