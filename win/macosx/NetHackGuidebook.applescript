#!/usr/bin/osascript
# $NHDT-Header: NetHack 5.0 NetHackGuidebook.applescript 2026/06/20_22:55:28 NetHack-5.0:1.17 $
# Copyright (c) Kenneth Lorber, Kensington, Maryland, 2011
# NetHack may be freely redistributed.  See license for details.

# Display the Guidebook from the GUI.

tell application "Finder"
        open location "file:///Library/Nethack/doc/NetHackGuidebook.pdf"
        delay 5
end tell
