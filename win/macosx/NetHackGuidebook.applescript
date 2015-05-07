#!/usr/bin/osascript
# NetHack 3.6.0  NetHackGuidebook.applescript $NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$
# NetHack 3.6.0  NetHackGuidebook.applescript $Date: 2011/10/17 01:29:20 $  $Revision: 1.2 $
# Copyright (c) Kenneth Lorber, Kensington, Maryland, 2011
# NetHack may be freely redistributed.  See license for details.

# Display the Guidebook from the GUI.

tell application "Finder"
        open location "file:///usr/games/doc/NetHackGuidebook.pdf"
        delay 5
end tell
