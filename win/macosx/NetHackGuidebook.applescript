#!/usr/bin/osascript
# NetHack 3.6.2  NetHackGuidebook.applescript $NHDT-Date: 1524684596 2018/04/25 19:29:56 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.8 $
# Copyright (c) Kenneth Lorber, Kensington, Maryland, 2011
# NetHack may be freely redistributed.  See license for details.

# Display the Guidebook from the GUI.

tell application "Finder"
        open location "file:///Library/Nethack/doc/NetHackGuidebook.pdf"
        delay 5
end tell
