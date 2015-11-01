#!/usr/bin/osascript
# NetHack 3.6.0  NetHackGuidebook.applescript $NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$
# Copyright (c) Kenneth Lorber, Kensington, Maryland, 2011
# NetHack may be freely redistributed.  See license for details.

# Display the Guidebook from the GUI.

tell application "Finder"
        open location "file:///Library/Nethack/doc/NetHackGuidebook.pdf"
        delay 5
end tell
