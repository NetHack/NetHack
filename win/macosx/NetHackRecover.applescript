#!/usr/bin/osascript
# NetHack 3.6  NetHackRecover.applescript $NHDT-Date: 1524684596 2018/04/25 19:29:56 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.9 $
# Copyright (c) Kenneth Lorber, Kensington, Maryland, 2009
# NetHack may be freely redistributed.  See license for details. 



set canceled to false
try
	display dialog "Welcome to the NetHack recover program.  Please make sure NetHack is not running before continuing.  Ready?" with title "NetHackRecover"
on error number -128
	set canceled to true
end try
if not canceled then
	set hpath to the path to me
	set mpath to the POSIX path of hpath
	considering case
		--set lastpos to offset of "/nethackdir" in mpath
		--set lastpos to lastpos + (length of "/nethackdir")
		--set rawpath to (get text 1 through lastpos of mpath) & "/recover.pl"
		--set safepath to the quoted form of rawpath
		set safepath to the quoted form of "/Library/Nethack/nethackdir/recover.pl"
	end considering
	do shell script safepath
	display dialog result with title "NetHackRecover Output"
end if
