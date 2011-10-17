#!/usr/bin/osascript
# NetHack 3.5.0  NetHackGuidebook.applescript $Date$  $Revision$
# Copyright (c) Kenneth Lorber, Kensington, Maryland, 2011
# NetHack may be freely redistributed.  See license for details.

# Display the Guidebook from the GUI.

tell application "Finder"
        open location "file:///usr/games/doc/NetHackGuidebook.pdf"
        delay 5
end tell
