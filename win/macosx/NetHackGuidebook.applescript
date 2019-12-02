#!/usr/bin/osascript
# NetHack 3.6.3  NetHackGuidebook.applescript $NHDT-Date: 1575245175 2019/12/02 00:06:15 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.10 $
# Copyright (c) Kenneth Lorber, Kensington, Maryland, 2011
# NetHack may be freely redistributed.  See license for details.

# Display the Guidebook from the GUI.

tell application "Finder"
        open location "file:///Library/Nethack/doc/NetHackGuidebook.pdf"
        delay 5
end tell
