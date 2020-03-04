$ ! vms/vmsbuild.com -- compile and link NetHack 3.7.*			[pr]
$	version_number = "3.7.0"
$ ! $NHDT-Date: 1557701799 2019/05/12 22:56:39 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.23 $
$ ! Copyright (c) 2018 by Robert Patrick Rankin
$ ! NetHack may be freely redistributed.  See license for details.
$
$!TODO: Separate the lua build and create an object library for it instead
$!	of putting lua modules into nethack.olb.
$
$ !
$ ! usage:
$ !   $ set default [.src]	!or [-.-.src] if starting from [.sys.vms]
$ !   $ @[-.sys.vms]vmsbuild  [compiler-option]  [link-option]  [cc-switches] -
$ !			      [linker-switches]  [interface]
$ ! options:
$ !	compiler-option :  either "VAXC", "DECC" or "GNUC" or "" !default VAXC
$ !	link-option	:  either "SHARE[able]" or "LIB[rary]"	!default SHARE
$ !	cc-switches	:  optional qualifiers for CC (such as "/noOpt/Debug")
$ !     linker-switches :  optional qualifers for LINK (/Debug or /noTraceback)
$ !     interface	:  "TTY" or "CURSES" or "TTY+CURSES" or "CURSES+TTY"
$ ! notes:
$ !	If the symbol "CC" is defined, compiler-option is not used (unless it
$ !	  is "LINK").
$ !	The link-option refers to VAXCRTL (C Run-Time Library) handling;
$ !	  to specify it while letting compiler-option default, use "" as
$ !	  the compiler-option.
$ !	To re-link without compiling, use "LINK" as special 'compiler-option';
$ !	  to re-link with GNUC library, 'CC' must begin with "G" (or "g").
$ !	All options are positional; to specify a later one without an earlier
$ !	  one, use "" in the earlier one's position, such as
$ !	$ @[-.sys.vms]vmsbuild "" "" "" "" "TTY+CURSES"
$
$	  decc_dflt = f$trnlnm("DECC$CC_DEFAULT")
$	  j = (decc_dflt.nes."") .and. 1
$	vaxc_ = "CC" + f$element(j,"#","#/VAXC") + "/NOLIST/OPTIMIZE=NOINLINE"
$	decc_ = "CC" + f$element(j,"#","#/DECC") + "/PREFIX=ALL/NOLIST"
$	gnuc_ = "GCC"
$     if f$type(gcc).eqs."STRING" then  gnuc_ = gcc
$	gnulib = "gnu_cc:[000000]gcclib/Library"    !(not used w/ vaxc)
$ ! common CC options (/obj=file doesn't work for GCC 1.36, use rename instead)
$ !	c_c_  = "/INCLUDE=([-.INCLUDE],[-.LIB.LUA535.SRC])/DEFINE=(""LUA_USE_C89"",""LUA_32BITS"")"
$	c_c_  = "/INCLUDE=([-.INCLUDE],[-.LIB.LUA535.SRC])/DEFINE=(""LUA_USE_C89"")"
$	veryold_vms = f$extract(1,1,f$getsyi("VERSION")).eqs."4" -
		.and. f$extract(3,3,f$getsyi("VERSION")).lts."6"
$	if veryold_vms then  c_c_ = c_c_ + "/DEFINE=(""VERYOLD_VMS"")"
$	axp = (f$getsyi("CPU").ge.128)	!f$getsyi("ARCH_NAME").eqs."Alpha"
$ ! miscellaneous setup
$	ivqual = %x00038240	!DCL-W-IVQUAL (used to check for ancient vaxc)
$	abort := exit %x1000002A
$	cur_dir  = f$environment("DEFAULT")
$	vmsbuild = f$environment("PROCEDURE")
$ ! validate first parameter
$	p1 := 'p1'
$	if p1.eqs."" .and. (axp .or. decc_dflt.eqs."/DECC") then  p1 = "DECC"
$	o_VAXC =  0	!(c_opt substring positions)
$	o_DECC =  5
$	o_GNUC = 10
$	o_LINK = 15
$	o_SPCL = 20
$	c_opt = f$locate("|"+p1, "|VAXC|DECC|GNUC|LINK|SPECIAL|") !5
$     if (c_opt/5)*5 .eq. c_opt then  goto p1_ok
$	copy sys$input: sys$error:	!p1 usage
%first arg is compiler option; it must be one of
       "VAXC" -- use VAX C to compile everything
   or  "DECC" -- use DEC C to compile everything
   or  "GNUC" -- use GNU C to compile everything
   or  "LINK" -- skip compilation, just relink nethack.exe
   or  "SPEC[IAL]" -- just compile and link dlb.exe and recover.exe
   or    ""   -- default operation (VAXC unless 'CC' is defined)

Note: if a DCL symbol for CC is defined, "VAXC" and "GNUC" are no-ops.
      If the symbol value begins with "G" (or "g"), then the GNU C
      library will be included in all link operations.  Do not rebuild
      dlb+recover with "SPECIAL" unless you have a CC symbol setup with
      the proper options.
$	abort
$p1_ok:
$ ! validate second parameter
$	p2 := 'p2'
$	l_opt = f$locate("|"+p2, "|SHAREABLE|LIBRARY__|NONE_____|") !10
$     if (l_opt/10)*10 .eq. l_opt then	goto p2_ok
$	copy sys$input: sys$error:	!p2 usage
%second arg is C run-time library handling; it must be one of
       "SHAREABLE" -- link with SYS$SHARE:VAXCRTL.EXE/SHAREABLE
   or   "LIBRARY"  -- link with SYS$LIBRARY:VAXCRTL.OLB/LIBRARY
   or    "NONE"    -- explicitly indicate DECC$SHR
   or      ""      -- default operation (use shareable image)

Note: for MicroVMS 4.x, "SHAREABLE" (which is the default) is required.
      Specify "NONE" if using DEC C with a CC symbol overriding 1st arg.
$	abort
$p2_ok:
$ ! start from a known location -- [.sys.vms], then move to [-.-.src]
$	set default 'f$parse(vmsbuild,,,"DEVICE")''f$parse(vmsbuild,,,"DIRECTORY")'
$	set default [-.-.src]	!move to source directory
$ ! compiler setup; if a symbol for "CC" is already defined it will be used
$     if f$type(cc).eqs."STRING" then  goto got_cc
$	cc = vaxc_			!assume "VAXC" requested or defaulted
$	if c_opt.eq.o_GNUC then  goto chk_gcc !explicitly invoked w/ "GNUC" option
$	if c_opt.eq.o_DECC then  cc = decc_
$	if c_opt.ne.o_VAXC then  goto got_cc !"SPEC" or "LINK", skip compiler check
$	! we want to prevent function inlining with vaxc v3.x (/opt=noinline)
$	!   but we can't use noInline with v2.x, so need to determine version
$	  set noOn
$	  msgenv = f$environment("MESSAGE")
$	  set message/noFacil/noSever/noIdent/noText
$	  cc/noObject _NLA0:/Include=[]     !strip 'noinline' if error
$	  sts = $status
$	if sts then  goto reset_msg	!3.0 or later will check out OK
$	! must be dealing with vaxc 2.x; ancient version (2.2 or earlier)
$	!   can't handle /include='dir', needs c$include instead
$	  cc = cc - "=NOINLINE" - ",NOINLINE" - "NOINLINE,"
$	  if sts.ne.IVQUAL then  goto reset_msg
$	    define/noLog c$include [-.INCLUDE]
$	    c_c_ = "/DEFINE=(""ANCIENT_VAXC"")"
$	    if veryold_vms then  c_c_ = c_c_ - ")" + ",""VERYOLD_VMS"")"
$reset_msg:
$	  set message 'msgenv'
$	  set On
$	  goto got_cc
$ !
$chk_gcc:
$	cc = gnuc_
$ ! old versions of gcc-vms don't have <varargs.h> or <stdarg.h> available
$	  c_c_ = "/DEFINE=(""USE_OLDARGS"")"
$	  if veryold_vms then  c_c_ = c_c_ - ")" + ",""VERYOLD_VMS"")"
$	  if veryold_vms then  goto chk_gas	!avoid varargs & stdarg
$	  if f$search("gnu_cc_include:[000000]varargs.h").nes."" then -
		c_c_ = "/DEFINE=(""USE_VARARGS"")"
$	  if f$search("gnu_cc_include:[000000]stdarg.h").nes."" then -
		c_c_ = "/DEFINE=(""USE_STDARG"")"
$chk_gas:
$ ! test whether this version of gas handles the 'const' construct correctly
$ gas_chk_tmp = "sys$scratch:gcc-gas-chk.tmp"
$ if f$search(gas_chk_tmp).nes."" then  delete/noconfirm/nolog 'gas_chk_tmp';*
$ gas_ok = 0	!assume bad
$ on warning then goto skip_gas
$ define/user/nolog sys$error 'gas_chk_tmp'
$ mcr gnu_cc:[000000]gcc-as sys$input: -o _NLA0:
$DECK
.const
.comm dummy,0
.const
.comm dummy,0
$EOD
$ gas_ok = 1	!assume good
$ if f$search(gas_chk_tmp).eqs."" then  goto skip_gas
$ ! if the error file is empty, gas can deal properly with const
$  gas_ok = f$file_attrib(gas_chk_tmp,"EOF") .eq. 0
$  delete/noconfirm/nolog 'gas_chk_tmp';*
$skip_gas:
$ on warning then continue
$	  if .not.gas_ok then  c_c_ = c_c_ - ")" + ",""const="")"
$	  c_c_ = "/INCLUDE=[-.INCLUDE]" + c_c_
$ !
$got_cc:
$	cc = cc + c_c_			!append common qualifiers
$	if p3.nes."" then  cc = cc + p3 !append optional user preferences
$	g := 'f$extract(0,1,cc)'
$	if g.eqs."$" then  g := 'f$extract(1,1,cc)'	!"foreign" gcc
$	if f$edit(f$extract(1,1,cc),"UPCASE").eqs."E" then  g := X	!GEMC
$	if g.nes."G" .and. c_opt.ne.o_GNUC then  gnulib = ""
$ ! linker setup; if a symbol for "LINK" is defined, we'll use it
$	if f$type(link).nes."STRING" then  link = "LINK/NOMAP"
$	if p4.nes."" then  link = link + p4 !append optional user preferences
$	if f$trnlnm("F").nes."" then  close/noLog f
$	create crtl.opt	!empty
$	open/Append f crtl.opt
$	write f "! crtl.opt"
$   if c_opt.eq.o_DECC .or. l_opt.eq.20
$   then  $! l_opt=="none", leave crtl.opt empty (shs$share:decc$shr.exe/Share)
$   else
$	! gnulib order:  vaxcrtl.exe+gcclib.olb vs gcclib.olb+vaxcrtl.olb
$	if l_opt.eq.0 then  write f "sys$share:vaxcrtl.exe/Shareable"
$	if gnulib.nes."" then  write f gnulib
$	if l_opt.ne.0 then  write f "sys$library:vaxcrtl.olb/Library"
$   endif
$	close f
$	if f$search("crtl.opt;-2").nes."" then  purge/Keep=2/noLog crtl.opt
$ ! version ID info for linker to record in .EXE files
$	create ident.opt
$	open/Append f ident.opt
$	write f "! ident.opt"
$	write f "identification=""",version_number,"""	!version"
$	close f
$	if f$search("ident.opt;-1").nes."" then  purge/noLog ident.opt
$ ! final setup
$	nethacklib = "[-.src]nethack.olb"
$	create nethack.opt
! nethack.opt
nethack.olb/Include=(vmsmain)/Library
![-.lib.lua]liblua.olb/Library
! lib$initialize is used to call a routine (before main()) in vmsunix.c that
! tries to check whether debugger support has been linked in, for PANICTRACE
sys$library:starlet.olb/Include=(lib$initialize)
! psect_attr=lib$initialize, Con,Usr,noPic,Rel,Gbl,noShr,noExe,Rd,noWrt,Long
! IA64 linker doesn't support Usr or Pic and complains that Long is too small
psect_attr=lib$initialize, Con,Rel,Gbl,noShr,noExe,Rd,noWrt
! increase memory available to RMS (the default iosegment is probably adequate)
iosegment=128
$	if f$search("nethack.opt;-2").nes."" then  purge/Keep=2/noLog nethack.opt
$	milestone = "write sys$output f$fao("" !5%T "",0),"
$     if c_opt.eq.o_LINK then  goto link  !"LINK" requested, skip compilation
$	rename	 := rename/New_Vers
$	touch	 := set file/Truncate
$	makedefs := $sys$disk:[-.util]makedefs
$	show symbol cc
$	goto begin	!skip subroutines
$!
$compile_file:	!input via 'c_file'
$	no_lib = ( f$extract(0,1,c_file) .eqs. "#" )
$	if no_lib then	c_file = f$extract(1,255,c_file)
$	c_name = f$edit(f$parse(c_file,,,"NAME"),"LOWERCASE")
$	f_opts = ""	!options for this file
$	if f$type('c_name'_options).nes."" then  f_opts = 'c_name'_options
$	milestone " (",c_name,")"
$	if f$search("''c_name'.obj").nes."" then  delete 'c_name'.obj;*
$	cc 'f_opts' 'c_file'
$	if .not.no_lib then  nh_obj_list == nh_obj_list + ",''c_name'.obj;0"
$     return
$!
$compile_list:	!input via 'c_list'
$	nh_obj_list == ""
$	j = -1
$ c_loop:
$	j = j + 1
$	c_file = f$element(j,",",c_list)  !get next file
$	if c_file.eqs."," then	goto c_done
$	c_file = c_file + ".c"
$	gosub compile_file
$	goto c_loop
$ c_done:
$	nh_obj_list == f$extract(1,999,nh_obj_list)
$	if nh_obj_list.nes."" then  libr/Obj 'nethacklib' 'nh_obj_list'/Replace
$	if nh_obj_list.nes."" then  delete 'nh_obj_list'
$	delete/symbol/global nh_obj_list
$     return
$!
$begin:
$!
$! miscellaneous special source file setup
$!
$ if f$search("pmatchregex.c").eqs."" then  copy [-.sys.share]pmatchregex.c []*.*
$ if f$search("random.c").eqs."" then  copy [-.sys.share]random.c []*.*
$ if f$search("tclib.c") .eqs."" then  copy [-.sys.share]tclib.c  []*.*
$!
$	p5 := 'p5'
$	ttysrc = "[-.win.tty]getline,[-.win.tty]termcap" -
		+ ",[-.win.tty]topl,[-.win.tty]wintty"
$	cursessrc = "[-.win.curses]cursdial,[-.win/curses]cursmesg" -
		+ ",[-.win.curses]cursinit,[-.win.curses]cursmisc" -
		+ ",[-.win.curses]cursinvt,[-.win.curses]cursstat" -
		+ ",[-.win.curses]cursmain,[-.win.curses]curswins"
$	interface = ttysrc !default
$	if p5.eqs."CURSES" then  interface = cursessrc
$	if p5.eqs."TTY+CURSES" then  interface = ttysrc + "," + cursessrc
$	if p5.eqs."CURSES+TTY" then  interface = cursessrc + "," + ttysrc
$
$ if f$search("[-.include]nhlua.h").eqs.""
$ then
$	create [-.include]nhlua.h	!empty
$       set file/att=(RFM:STM) [-.include]nhlua.h
$	open/Append f [-.include]nhlua.h
$       write f "/* nhlua.h - generated by vmsbuild.com */"
$       write f "#include ""[-.lib.lua535.src]lua.h"""
$       write f "LUA_API int (lua_error) (lua_State *L) NORETURN;"
$       write f "#include ""[-.lib.lua535.src]lualib.h"""
$       write f "#include ""[-.lib.lua535.src]lauxlib.h"""
$       write f "/*nhlua.h*/"
$	close f
$ endif
$!
$! create object library
$!
$     if c_opt.ne.o_SPCL .or. f$search(nethacklib).eqs."" then -
  libr/Obj 'nethacklib'/Create=(Block=3000,Hist=0)
$ if f$search("''nethacklib';-1").nes."" then  purge 'nethacklib'
$!
$! compile and link makedefs, then nethack, lev_comp+dgn_comp, dlb+recover.
$!
$ milestone "<compiling...>"
$ c_list = "[-.sys.vms]vmsmisc,[-.sys.vms]vmsfiles,[]alloc,dlb,monst,objects"
$     if c_opt.eq.o_SPCL then  c_list = c_list + ",decl,drawing"
$ gosub compile_list
$     if c_opt.eq.o_SPCL then  goto special !"SPECIAL" requested, skip main build
$ set default [-.util]
$ c_list = "#makedefs"
$ gosub compile_list
$ link makedefs.obj,'nethacklib'/Lib,[-.src]ident.opt/Opt,[-.src]crtl/Opt
$ milestone "makedefs"
$! create some build-time files
$ makedefs -p	!pm.h
$ makedefs -o	!onames.h
$ makedefs -v	!date.h
$ milestone " (*.h)"
$ makedefs -z	!../src/vis_tab.c, ../include/vis_tab.h
$ milestone " (*.c)"
$ set default [-.src]
$! compile most of the source files:
$ c_list = "decl,version,[-.sys.vms]vmsmain,[-.sys.vms]vmsunix" -
	+ ",[-.sys.vms]vmstty,[-.sys.vms]vmsmail" -
	+ ",[]isaac64" -			!already in [.src]
	+ ",[]random,[]tclib,[]pmatchregex"	!copied from [-.sys.share]
$ gosub compile_list
$ c_list = interface !ttysrc or cursessrc or both
$ gosub compile_list
$ c_list = "allmain,apply,artifact,attrib,ball,bones,botl,cmd,dbridge" -
	+ ",dothrow,drawing,detect,dig,display,do,do_name,do_wear,dog" -
	+ ",dogmove,dokick,dungeon,eat,end,engrave,exper,explode" -
	+ ",extralev,files,fountain"
$ gosub compile_list
$ c_list = "hack,hacklib,insight,invent,light,lock,mail,makemon" -
	+ ",mapglyph,mcastu,mhitm,mhitu,minion,mklev,mkmap,mkmaze" -
	+ ",mkobj,mkroom,mon,mondata,monmove,mplayer,mthrowu,muse" -
	+ ",music,o_init,objnam,options,pager,pickup"
$ gosub compile_list
$ c_list = "pline,polyself,potion,pray,priest,quest,questpgr,read" -
	+ ",rect,region,restore,rip,rnd,role,rumors,save,shk,shknam,sit" -
	+ ",sounds,sp_lev,spell,steal,steed,sys,teleport,timeout,topten" -
	+ ",track,trap,u_init"
$ gosub compile_list
$ c_list = "uhitm,vault,vision,vis_tab,weapon,were,wield,windows" -
	+ ",wizard,worm,worn,write,zap"
$ gosub compile_list
$!
$! Files added in 3.7
$!
$ c_list = "nhlua,nhlobj,nhlsel" !,sfstruct
$ gosub compile_list
$!
$! 3.7 runtime LUA level parser/loader
$!
$ c_list = "[-.lib.lua535.src]lapi,[-.lib.lua535.src]lauxlib,[-.lib.lua535.src]lbaselib" -
	+ ",[-.lib.lua535.src]lbitlib,[-.lib.lua535.src]lcode,[-.lib.lua535.src]lcorolib" -
	+ ",[-.lib.lua535.src]lctype,[-.lib.lua535.src]ldblib,[-.lib.lua535.src]ldebug" -
	+ ",[-.lib.lua535.src]ldo,[-.lib.lua535.src]ldump,[-.lib.lua535.src]lfunc" -
	+ ",[-.lib.lua535.src]lgc,[-.lib.lua535.src]linit,[-.lib.lua535.src]liolib" -
	+ ",[-.lib.lua535.src]llex"
$ gosub compile_list
$ c_list = "[-.lib.lua535.src]lmathlib,[-.lib.lua535.src]lmem,[-.lib.lua535.src]loadlib" -
	+ ",[-.lib.lua535.src]lobject,[-.lib.lua535.src]lopcodes,[-.lib.lua535.src]loslib" -
	+ ",[-.lib.lua535.src]lparser,[-.lib.lua535.src]lstate,[-.lib.lua535.src]lstring" -
	+ ",[-.lib.lua535.src]lstrlib,[-.lib.lua535.src]ltable,[-.lib.lua535.src]ltablib" -
	+ ",[-.lib.lua535.src]ltm,[-.lib.lua535.src]lundump,[-.lib.lua535.src]lutf8lib" -
	+ ",[-.lib.lua535.src]lvm,[-.lib.lua535.src]lzio"
$ gosub compile_list
$!
$link:
$ milestone "<linking...>"
$ link/Exe=nethack.exe nethack.opt/Options,ident.opt/Options,crtl.opt/Options
$ milestone "NetHack"
$     if c_opt.eq.o_LINK then  goto done	!"LINK" only
$special:
$!
$! utilities only [dgn_comp and lev_comp are gone]
$!
$ set default [-.util]
$ c_list = "#panic,#dlb_main,#recover"
$ gosub compile_list
$ link/exe=dlb.exe dlb_main.obj,-
	panic.obj,'nethacklib'/Lib,[-.src]ident.opt/Opt,[-.src]crtl.opt/Opt
$ milestone "dlb"
$ link/exe=recover.exe recover.obj,-
	'nethacklib'/Lib,[-.src]ident.opt/Opt,[-.src]crtl.opt/Opt
$ milestone "recover"
$!
$done:
$	set default 'cur_dir'
$ exit
