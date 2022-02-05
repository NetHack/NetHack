# depend.awk -- awk script used to construct makefile dependencies
# for nethack's source files (`make depend' support for Makefile.src).
# $NHDT-Date: 1612127123 2021/01/31 21:05:23 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.13 $
#
# usage:
#   cd src ; nawk -f depend.awk ../include/*.h list-of-.c/.cpp-files
#
# This awk program scans each file in sequence, looking for lines beginning
# with `#include "' and recording the name inside the quotes.  For .h files,
# that's all it does.  For each .c file, it writes out a make rule for the
# corresponding .o file; dependencies in nested header files are propagated
# to the .o target.
#
# config.h and hack.h get special handling because of their heavy use;
#	timestamps for them allow make to avoid rechecking dates on
#	subsidiary headers for every source file;
# extern.h gets special handling to avoid excessive recompilation
#	during development;
# patchlev.h gets special handling because it only exists on systems
#	which consider filename patchlevel.h to be too long;
# amiconf.h moved from ../include/ to ../outdated/include/ so skip it
# interp.c gets special handling because it usually doesn't exist; it's
#	assumed to be the last #include in the file where it occurs.
# win32api.h gets special handling because it only exists for some ports;
#	it's assumed to be the last #include in the file where it occurs
# zlib.h ditto
#
BEGIN		{ FS = "\""			#for `#include "X"', $2 is X
		  special[++sp_cnt] = "../include/config.h"
		  special[++sp_cnt] = "../include/hack.h"
		  alt_deps["../include/extern.h"] = ""
		  alt_deps["../include/patchlev.h"] = ""
		  alt_deps["../include/amiconf.h"] = ""
		  alt_deps["interp.c"] = " #interp.c"	#comment it out
		  alt_deps["../include/win32api.h"] = " #../include/win32api.h"
		  alt_deps["../include/zlib.h"] = " #zlib.h"	#comment it out
		}
FNR == 1	{ output_dep()			#finish previous file
		  file = FILENAME		#setup for current file
		}
/^[#][ \t]*include[ \t]+["]/  {			#find `#include "X"'
		  incl = $2
		  #[3.4.0: gnomehack headers currently aren't in include]
		  #[3.6.2: Qt4 headers aren't in include either]
		  #[3.6.2: curses headers likewise]
		  #[3.7.0: Qt headers have moved; process 'moc' files]
		  if (incl ~ /[.]h$/) {
		    if (incl ~ "curses[.]h")
		      incl = ""	# skip "curses.h"; it should be <curses.h>
		    else if (incl ~ /^..\/lib\/lua-.*\/src\/l/)
		      incl = ""	# skip lua headers
		    else if (incl ~ /^curs/)	# curses special case
		      incl = "../win/curses/" incl
		    else if (incl ~ /(.*\/)*qt_/) {	# Qt special cases
		      # Qt v3 headers are in ../win/Qt3
		      # Qt v4/v5/v6 headers are in ../win/Qt
		      # *.moc files have path in their #include
		      if (file ~ /[.]moc$/)
			; # keep 'incl' as-is
		      else if (file ~ /^[.][.]\/win\/Qt3\/.*/)
			incl = "../win/Qt3/" incl
		      else			# Qt v4/v5/v6
			incl = "../win/Qt/" incl
		    } else if (incl ~ /^gn/)	# gnomehack special case
		      incl = "../win/gnome/" incl
		    else
		      incl = "../include/" incl
		  }
		  deps[file] = deps[file] " " incl
		}
END		{ output_dep() }		#finish the last file


#
# `file' has been fully scanned, so process it now; for .h files,
# don't do anything (we've just been collecting their dependencies);
# for .c files, output the `make' rule for corresponding .o file
#
function output_dep(				base, targ, moc)
{
  #get the file's base name (including suffix)
  base = file;  sub("^.+/", "", base)
  #for qt source files, add qt timestamp file as extra dependency
  moc = (base ~ /[.]moc$/)
  if (moc || base ~ /(.+\/)*qt_.*[.]cpp$/) {
    deps[file] = deps[file] " $(QTn_H)"
  }
  if (base ~ /[.]cp*$/ || moc) {
    #prior to very first .c|.cpp file, handle some special header file cases
    if (!c_count++)
      output_specials()
    #construct object filename from source filename
    targ = base;  sub("[.]cp*$", ".o", targ)
    #format and write the collected dependencies
    format_dep(targ, file)
  }
}

#
# handle some targets (config.h, hack.h) via special timestamping rules
#
function output_specials(			i, sp, alt_sp)
{
  for (i = 1; i <= sp_cnt; i++) {
    sp = special[i]
    #change "../include/foo.h" first to "foo.h", then ultimately to "$(FOO_H)"
    alt_sp = sp;  sub("^.+/", "", alt_sp)
    print "#", alt_sp, "timestamp"	#output a `make' comment
 #- sub("[.]", "_", alt_sp);  alt_sp = "$(" toupper(alt_sp) ")"
 #+ Some nawks don't have toupper(), so hardwire these instead.
    sub("config.h", "$(CONFIG_H)", alt_sp);  sub("hack.h", "$(HACK_H)", alt_sp)
    format_dep(alt_sp, sp)		#output the target
    print "\ttouch " alt_sp		#output a build command
    alt_deps[sp] = alt_sp		#alternate dependency for depend()
  }
  print "#"
}

#
# write a target and its dependency list in pretty-printed format;
# if target's primary source file has a path prefix, also write build command
#
function format_dep(target, source,		col, n, i, list, prefix, moc)
{
  split("", done)			#``for (x in done) delete done[x]''
  moc = (target ~ /[.]moc$/)
  prefix = (moc || substr(target,1,1) == "$") ? "" : "$(TARGETPFX)"
  printf("%s%s:", prefix, target);  col = length(target) + 1 + length(prefix)
  #- printf("\t");  col += 8 - (col % 8);
  #- if (col == 8) { printf("\t"); col += 8 }
  source = depend("", source, 0)
  n = split(source, list, " +")
  #first: leading whitespace yields empty 1st element; not sure why moc
  #files duplicate the target as next element but we need to skip that too
  first = moc ? 3 : 2
  for (i = first; i <= n; i++) {
    if (col + length(list[i]) >= (i < n ? 78 : 80) - 1) {
      printf(" \\\n\t\t");  col = 16	#make a backslash+newline split
    } else {
      printf(" ");  col++;
    }
    printf("%s", list[i]);  col += length(list[i])
  }
  printf("\n")				#terminate
  #write build command if first source entry has non-include path prefix
  source = list[first]
  if (moc) {
    print "\t$(MOCPATH) -o $@ " source
  } else if (source ~ /\// && substr(source, 1, 11) != "../include/") {
    if (source ~ /[.]cpp$/ )
      print "\t$(TARGET_CXX) $(TARGET_CXXFLAGS) -c -o $@ " source
    else if (source ~ /\/X11\//)	# "../win/X11/foo.c"
      print "\t$(TARGET_CC) $(TARGET_CFLAGS) $(X11CFLAGS) -c -o $@ " source
    else if (source ~ /\/gnome\//)	# "../win/gnome/foo.c"
      print "\t$(TARGET_CC) $(TARGET_CFLAGS) $(GNOMEINC) -c -o $@ " source
    else
      print "\t$(TARGET_CC) $(TARGET_CFLAGS) -c -o $@ " source
  }
}

#
# recursively add the dependencies for file `name' to string `inout'
# (unless `skip', in which case we're only marking files as already done)
#
function depend(inout, name, skip,		n, i, list)
{
  if (!done[name]++) {
    if (name in alt_deps) {	#some names have non-conventional dependencies
      if (!skip) inout = inout " " alt_deps[name]
      skip = 1
    } else {			#ordinary name
      if (!skip) inout = inout " " name
    }
    if (name in deps) {
 #-   n = split(deps[name], list, " +")
 #-   for (i = 2; i <= n; i++)	#(leading whitespace yields empty 1st element)
 #-	inout = depend(inout, list[i], skip)
 #+  At least one implementation of nawk handles the local array `list' wrong,
 #+  so the clumsier substitute code below is used as a workaround.
      list = deps[name];  sub("^ +", "", list)
      while (list) {
	match((list " "), " +");  i = RSTART;  n = RLENGTH
	inout = depend(inout, substr(list, 1, i-1), skip)
	list = substr(list, i+n)
      }
    }
  }
  return inout
}

#depend.awk#
