#!/usr/bin/osascript
# NetHack 3.7  NetHackGuidebook.applescript $NHDT-Date: 1596498328 2020/08/03 23:45:28 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.13 $
# Copyright (c) Kenneth Lorber, Kensington, Maryland, 2011
# NetHack may be freely redistributed.  See license for details.

# Display the Guidebook from the GUI.

tell application "Finder"
        open location "file:///Library/Nethack/doc/NetHackGuidebook.pdf"
        delay 5
end tell
