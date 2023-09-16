#!/usr/bin/osascript
# NetHack 3.6.3  NetHackTerm.applescript $NHDT-Date: 1575245179 2019/12/02 00:06:19 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.10 $
# Copyright (c) Kenneth Lorber, Kensington, Maryland, 2011
# NetHack may be freely redistributed.  See license for details.

# Run the terminal port from the GUI.

# BUGS:
# We close any terminal windows with no processes in them, even if they
# don't belong to us because we can't really tell which ones do belong to us.
# Shouldn't be serious since anyone using this is unlikely to be using Terminal
# for anything else.
set debug to false

set needshutdown to false

tell application "Finder"
	set bundleAppPath to POSIX path of (application file id "org.nethack.macos" as text)
end tell

tell application "Terminal"
	# see if we're going to have to shut it down at the end because we started it up
	if it is not running then
		set needshutdown to true
	end if
	
	activate
	#open new window and run NetHack in it
	do script with command "clear;sleep 1;pwd;" & bundleAppPath & "/Contents/MacOS/nethack;echo '(press RETURN to exit)';awk '{exit}';exit"
	set nhresult to result -- class is tab
	set nhresrec to result as record
	set nhreslist to result as list
	set nhwin to item 4 of nhreslist
	set custom title of nhwin to "NH"
	#set title displays custom title of nhresult to true
	set title displays device name of nhresult to false
	set title displays shell path of nhresult to false
	set title displays window size of nhresult to false
	set title displays file name of nhresult to false
	#set idnum to id of nhresult
	set xxx to class of nhresrec
	get class of nhresrec -- record
	get length of nhresrec -- 4
	set nhwinname to name of nhwin
	set nhtab to selected tab of nhwin
	get processes of nhtab
	
	# main loop - wait for all nethack processes to exit
	set hit to true
	set nhname to "nethack" as text
	delay 4
	repeat while hit
		set hit to false
		delay 0.5
		
		# don't blow up if the window has gone away
		try
			set procs to get processes of nhtab
		on error number -1728
			exit repeat
		end try
		
		repeat with pname in procs
			if debug then
				display alert "PNAME=" & pname & "=" & nhname & "="
			end if
			# XXX this test is odd, but more obvious tests fail
			if pname starts with nhname then
				#display alert "HIT"  -- dangerous - infinite loop
				set hit to true
			end if
			# yes, this is scary.
			if pname starts with ("awk" as text) then
				set hit to true
			end if
		end repeat
	end repeat
	if debug then
		display alert "DONE"
	end if
	
	# window may have already closed or dropped its last process (error -1728)
	try
		close window nhwinname
	on error errText number errNum
		if errNum = -1728 then
			if debug then
				display alert "close failed (expected)"
			end if
		else
			-- unexpected error - show it
			display alert "close failed: " & errText & " errnum=" & errNum
		end if
	end try
	
end tell

# Close all the windows that don't have any live processes in them.
tell application "Terminal"
	set wc to count windows
	set pending to {}
	if debug then
		display alert "COUNT is " & wc
	end if
	repeat with win in windows
		if debug then
			display alert "WIN: " & (name of win) & " TABS: " & (count of tabs of win) & " PROCS: " & (count of processes of item 1 of tabs of win)
		end if
		set pcount to count of processes of item 1 of tabs of win
		if pcount is 0 then
			copy win to the end of pending
			set wc to wc - 1
		end if
	end repeat
end tell

if debug then
	display alert "LATE COUNT is " & wc
end if
repeat with win in pending
	try
		close win
	end try
end repeat

# If we started Terminal.app and the user doesn't have anything else running,
# shut it down.
if needshutdown and (wc is 0) then
	if debug then
		display alert "trying shutdown"
	end if
	tell application "Terminal"
		quit
	end tell
end if
