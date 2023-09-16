$ ! vms/vmsbuild.com -- compile and link NetHack 3.7.*			[pr]
$	version_number = "3.7.0"
$ ! $NHDT-Date: 1687541093 2023/06/23 17:24:53 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.39 $
$ ! Copyright (c) 2018 by Robert Patrick Rankin
$ ! NetHack may be freely redistributed.  See license for details.
$ !
$ ! usage:
$ !   $ set default [.src]	!or [-.-.src] if starting from [.sys.vms]
$ !   $ @[-.sys.vms]vmsbuild  [compiler-option]  [link-option]  [cc-switches] -
$ !			      [linker-switches]  [interface]
$ ! options:
$ !     compiler-option :  either "VSIC", "VAXC", "DECC",
$ !                        "GNUC" or "" or "fetchlua" !default in 3.7 is VSIC
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
$ !
$ ! Lua Version
$ luaver = "546"
$ luadotver = "5.4.6"
$ luaunderver = "5_4_6"
$
$	  decc_dflt = f$trnlnm("DECC$CC_DEFAULT")
$	  j = (decc_dflt.nes."") .and. 1
$	vsic_ = "CC" + f$element(j,"#","#/DECC") + "/NOLIST"
$	vaxc_ = "CC" + f$element(j,"#","#/VAXC") + "/NOLIST/OPTIMIZE=NOINLINE"
$	decc_ = "CC" + f$element(j,"#","#/DECC") + "/PREFIX=ALL/NOLIST"
$	gnuc_ = "GCC"
$     if f$type(gcc).eqs."STRING" then  gnuc_ = gcc
$	gnulib = "gnu_cc:[000000]gcclib/Library"    !(not used w/ vaxc)
$ ! common CC options (/obj=file doesn't work for GCC 1.36, use rename instead)
$	c_c_  = "/INCLUDE=([-.INCLUDE],[-.LIB.lua''luaver'.SRC])" -
		+ "/DEFINE=(""LUA_USE_C89"",USE_FCNTL)"
$       cxx_c_ = "/INCLUDE=([-.INCLUDE],[-.LIB.LUA''luaver'.SRC])"
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
$       o_VSIC =  0
$	o_VAXC =  5	!(c_opt substring positions)
$	o_DECC = 10
$	o_GNUC = 15
$	o_LINK = 20
$	o_SPCL = 25
$	o_FETCHLUA = 33
$       o_BUILDLUA = 42
$	c_opt = f$locate("|"+p1, -
	"|VSIC|VAXC|DECC|GNUC|LINK|SPECIAL|FETCHLUA|BUILDLUA")
$!     write sys$output c_opt
$     if (c_opt/5)*5 .eq. c_opt then  goto p1_ok
$     if (c_opt .eq. o_FETCHLUA) .OR. (c_opt .eq. o_BUILDLUA) then goto p1_ok
$	copy sys$input: sys$error:	!p1 usage
%first arg is compiler option; it must be one of
       "VSIC"      -- use VSI C to compile everything
   or  "VAXC"      -- use VAX C to compile everything
   or  "DECC"      -- use DEC C to compile everything
   or  "GNUC"      -- use GNU C to compile everything
   or  "LINK"      -- skip compilation, just relink nethack.exe
   or  "SPEC[IAL]" -- just compile and link dlb.exe and recover.exe
   or  "FETCHLUA"  -- skip compilaton, just fetch lua from lua.org
   or  "BUILDLUA"  -- build [-.lib.lua]lua'LUAVER'.olb
   or  ""          -- carry out default operation (VAXC unless 'CC' is defined)

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
        "SHAREABLE" -- link with SYS$SHARE:LIBRTL.EXE/SHAREABLE
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
$	cc = vsic_			!assume "VSIC" requested or defaulted
$	cxx = "CXX"
$	if c_opt.eq.o_GNUC then  goto chk_gcc !explicitly invoked w/ "GNUC" option
$	if c_opt.eq.o_DECC then  cc = decc_
$	if c_opt.eq.o_VAXC then  cc = vaxc_
$	if c_opt.ne.o_VSIC then  goto got_cc !"SPEC" or "LINK", skip compiler check
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
$	  cxx_c_ = "/INCLUDE=[-.INCLUDE]" + cxx_c_
$ !
$got_cc:
$
$!      not sure if 8.x DECC supports /NAMES=(AS_IS). Use next line, if so
$!	if (c_opt.ne.o_VAXC) .and. (c_opt.ne.o_GNUC) .and. (c_opt.ne.o_DECC)
$	if (c_opt.ne.o_VAXC) .and. (c_opt.ne.o_GNUC)
$	then
$	  c_c_ = c_c_ + "/NAMES=(AS_IS)
$	  cxx_c_ = cxx_c_ + "/NAMES=(AS_IS)
$       endif
$	cc = cc + c_c_			!append common qualifiers
$	cxx = cxx + cxx_c_
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
$   if c_opt.eq.o_DECC .or. c_opt.eq.o_VSIC .or. l_opt.eq.20
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
$	if f$search("''c_file'.cpp").nes.""  ! for regex
$ 	then
$		c_file = c_file + ".cpp"
$		compiler = "''cxx'"
$ 	else
$		c_file = c_file + ".c"
$		compiler = "''cc'"
$	endif
$	c_name = f$edit(f$parse(c_file,,,"NAME"),"LOWERCASE")
$	f_opts = ""	!options for this file
$	if f$type('c_name'_options).nes."" then  f_opts = 'c_name'_options
$	milestone " (",c_name,")"
$	if f$search("''c_name'.obj").nes."" then  delete 'c_name'.obj;*
$	'compiler' 'f_opts' 'c_file'
$	if .not.no_lib
$       then
$	    libr/Obj/Replace 'nethacklib' 'c_name'.obj;0
$	endif
$     return
$!
$compile_list:	!input via 'c_list'
$	nh_obj_list == ""
$	j = -1
$ c_loop:
$	j = j + 1
$	c_file = f$element(j,",",c_list)  !get next file
$	if c_file.eqs."," then	goto c_done
$	gosub compile_file
$	goto c_loop
$ c_done:
$     return
$fetchlua:
$	create/dir [-.lib]
$	set def [-.lib]
$	on error then goto luafixdir
$	pipe -
	if f$search("lua''luaver'.tar.gz").eqs."" then -
		curl http://www.lua.org/ftp/lua-'luadotver'.tar.gz -
			--output lua'luaver'.tgz && -
	if (f$search("lua''luaver'.tar").eqs."") .AND. -
		(f$search("lua''luaver'.tgz").nes."") then -
		gzip -d lua'luaver'.tgz && -
	if f$search("lua''luaver'.tar").nes."" then -
		tar -xf lua'luaver'.tar && -
	if (f$search("lua-''luaunderver'.DIR;1").nes."") .AND. -
		(f$search("lua''luaver'.dir;1").eqs."") then -
		rename lua-'luaunderver'.DIR;1  lua'luaver'.dir;1
$ milestone "[-.lib.lua'''luaver']"
$luafixdir:
	set def [-.src]
$       if f$search("[-.include]nhlua.h;-1").nes."" then -
		purge [-.include]nhlua.h
$	if f$search("[-.include]nhlua.h").nes."" then -
		delete [-.include]nhlua.h;
$ milestone " (wiped existing [-.include]nhlua.h)"
$ exit
$!
$! 3.7 runtime LUA level parser/loader
$!
$buildlua:
$ if f$search("[-.lib]lua.dir;").eqs."" then -
	create/dir [-.lib.lua]
$ save_nethacklib = nethacklib
$!
$! Temporarily override the value of nethacklib so that
$! the lua modules go into the lua library, not nethacklib.
$!
$ nethacklib = "[-.lib.lua]lua''luaver'.olb"
$ if f$search("''nethacklib'").eqs."" then -
  libr/Obj 'nethacklib'/Create
$ if f$search("''nethacklib';-1").nes."" then -
  purge 'nethacklib'
$ c_list = "[-.lib.lua''luaver'.src]lapi,[-.lib.lua''luaver'.src]lauxlib" -
	+ ",[-.lib.lua''luaver'.src]lbaselib" -
	+ ",[-.lib.lua''luaver'.src]lcode,[-.lib.lua''luaver'.src]lcorolib" -
	+ ",[-.lib.lua''luaver'.src]lctype,[-.lib.lua''luaver'.src]ldblib" -
	+ ",[-.lib.lua''luaver'.src]ldebug,[-.lib.lua''luaver'.src]ldo" -
	+ ",[-.lib.lua''luaver'.src]ldump,[-.lib.lua''luaver'.src]lfunc" -
	+ ",[-.lib.lua''luaver'.src]lgc,[-.lib.lua''luaver'.src]linit" -
	+ ",[-.lib.lua''luaver'.src]liolib,[-.lib.lua''luaver'.src]llex" -
        + ",[-.lib.lua''luaver'.src]lmathlib,[-.lib.lua''luaver'.src]lmem" -
	+ ",[-.lib.lua''luaver'.src]loadlib,[-.lib.lua''luaver'.src]lobject" -
	+ ",[-.lib.lua''luaver'.src]lopcodes,[-.lib.lua''luaver'.src]loslib" -
	+ ",[-.lib.lua''luaver'.src]lparser,[-.lib.lua''luaver'.src]lstate" -
	+ ",[-.lib.lua''luaver'.src]lstring,[-.lib.lua''luaver'.src]lstrlib" -
	+ ",[-.lib.lua''luaver'.src]ltable,[-.lib.lua''luaver'.src]ltablib" -
	+ ",[-.lib.lua''luaver'.src]ltm,[-.lib.lua''luaver'.src]lundump" -
	+ ",[-.lib.lua''luaver'.src]lutf8lib,[-.lib.lua''luaver'.src]lvm" -
	+ ",[-.lib.lua''luaver'.src]lzio"
$ gosub compile_list
$ milestone " ''nethacklib'"
$ luafinish:
$ nethacklib = save_nethacklib
$ return
$begin:
$ if (c_opt .eq. o_FETCHLUA) .OR. (c_opt .eq. o_BUILDLUA)
$ then
$     if c_opt .eq. 33 then gosub fetchlua
$     if c_opt .eq. 42 then gosub buildlua
$     goto done
$ endif
$!
$! Check some prerequisites
$!
$ if f$search("[-.lib]lua''luaver'.dir").eqs.""
$ then
$     write sys$output "You need to: @vmsbuild fetchlua
$     exit
$ endif
$ if f$search("[-.lib.lua]lua''luaver'.olb").eqs.""
$ then
$     write sys$output "You need to: @vmsbuild buildlua
$     exit
$ endif
$!
$! miscellaneous special source file setup
$!
$! default to using cppregex
$ if f$search("regex.c").eqs."" then -
	copy [-.sys.share]cppregex.cpp []regex.cpp
$! if f$search("random.c").eqs."" then  copy [-.sys.share]random.c []*.*
$ if f$search("tclib.c") .eqs."" then -
	copy [-.sys.share]tclib.c  []*.*
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
$       write f "#include ""[-.lib.lua''luaver'.src]lua.h"""
$       write f "LUA_API int (lua_error) (lua_State *L) NORETURN;"
$       write f "#include ""[-.lib.lua''luaver'.src]lualib.h"""
$       write f "#include ""[-.lib.lua''luaver'.src]lauxlib.h"""
$       write f "/*nhlua.h*/"
$	close f
$ endif
$ milestone " ([-.include]nhlua.h)"
$!
$! create object library
$!
$     if c_opt.ne.o_SPCL .or. f$search(nethacklib).eqs."" then -
  libr/Obj 'nethacklib'/Create=(Block=3000,Hist=0)
$ if f$search("''nethacklib';-1").nes."" then  purge 'nethacklib'
$!
$! compile and link makedefs, then nethack, dlb+recover.
$!
$ milestone "<compiling...>"
$!
$ c_list = "[-.sys.vms]vmsmisc,[-.sys.vms]vmsfiles,[]alloc,dlb," -
	+ "monst,objects,date,#[-.util]panic"
$     if c_opt.eq.o_SPCL then  c_list = c_list + ",decl,drawing"
$ gosub compile_list
$     if c_opt.eq.o_SPCL then  goto special !"SPECIAL" requested, skip main build
$ set default [-.util]
$ c_list = "#makedefs"
$ gosub compile_list
$ link makedefs.obj,[-.src]panic.obj,'nethacklib'/Lib,[-.src]ident.opt/Opt,[-.src]crtl/Opt
$ milestone "makedefs"
$! create some build-time files
$! 3.7 does not require these
$! makedefs -p	!pm.h
$! makedefs -o	!onames.h
$! makedefs -v	!date.h
$ milestone " (*.c)"
$ set default [-.src]
$! compile most of the source files:
$ c_list = "decl,version,[-.sys.vms]vmsunix" -
	+ ",[-.sys.vms]vmstty,[-.sys.vms]vmsmail" -
	+ ",[]isaac64" -			!already in [.src]
	+ ",[]tclib,[]regex"			!copied from [-.sys.share]
$ gosub compile_list
$ c_list = interface !ttysrc or cursessrc or both
$ gosub compile_list
$ c_list = "allmain,apply,artifact,attrib,ball,bones,botl,cmd,dbridge" -
	+ ",dothrow,drawing,detect,dig,display,do,do_name,do_wear,dog" -
	+ ",dogmove,dokick,dungeon,eat,end,engrave,exper,explode" -
	+ ",extralev,files,fountain"
$ gosub compile_list
$ c_list = "hack,hacklib,insight,invent,light,lock,mail,makemon" -
	+ ",mcastu,mdlib,mhitm,mhitu,minion,mklev,mkmap,mkmaze" -
	+ ",mkobj,mkroom,mon,mondata,monmove,mplayer,mthrowu,muse" -
	+ ",music"
$ gosub compile_list
$!
$! Files added in 3.7 for Lua glue
$!
$ c_list = "nhlua,nhlobj,nhlsel"
$ gosub compile_list
$ c_list = "o_init,objnam,options,pager,pickup" -
	+ ",pline,polyself,potion,pray,priest,quest,questpgr,read" -
	+ ",rect,region,restore,rip,rnd,role,rumors,save,sfstruct,shk" -
	+ ",shknam,sit,sounds,sp_lev,spell,steal,steed,symbols" -
	+ ",sys,teleport,timeout,topten,track,trap,utf8map,u_init"
$ gosub compile_list
$ c_list = "uhitm,vault,vision,weapon,were,wield,windows" -
	+ ",wizard,worm,worn,write,zap"
$ gosub compile_list
$!
$link:
$! We do these at the end
$ c_list = "#[-.sys.vms]vmsmain,#date"
$ gosub compile_list
$ milestone "<linking...>"
$ link /EXECUTABLE=nethack.exe vmsmain.obj,date.obj-
        +[-.src]nethack.olb/library -
        +sys$disk:[-.lib.lua]lua546.olb/library
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
