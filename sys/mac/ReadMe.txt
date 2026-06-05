                     NetHack 5.0 for 68k Macintosh
                     =============================

Requirements
------------
    - A Macintosh with a 68020 or later CPU.
    - System 7.0 or later, with 32-bit addressing enabled
      (Memory control panel; reboot after changing it).
    - 6 MB of free RAM minimum; 8 MB is the preferred partition.
    - Color is optional: tiles need a 4-bit or 8-bit screen,
      black & white Macs play in ASCII.

Installation
------------
The game is distributed two ways; pick one:

NetHack.img -- a complete, ready-to-play SCSI disk image (Apple
Partition Map + HFS volume "NetHack 5.0").  Attach it as a disk in
QEMU, or copy it onto a BlueSCSI SD card.  Nothing to install:
boot, open the volume, double-click NetHack.

NetHack.sit -- a StuffIt archive for installing onto an existing
system.  Expand it ON THE MAC with StuffIt Expander (expanding on
another machine loses resource forks).  It contains:

    NetHack             the application
    Recover             crash-recovery application
    nhdat               packed game data (levels, Lua, text)
    NetHack Defaults    configuration file (editable TEXT)
    Guidebook           how to play NetHack
    Read Me             this file
    license, symbols

Keep everything in one folder and double-click NetHack.  Save
files, level files, and the record (high score) file are created
in the same folder during play.

Display Modes
-------------
On a color screen the map starts in graphical tiles; choose
"Tile Mode" from the File menu at any time during play to switch
between tiles and ASCII.  Black & white screens always use ASCII.

The map lives in its own window: drag it where you like, resize
it, and on larger screens use its scrollbars.  Window positions
and sizes (per display mode) are remembered across games in
"NetHack Preferences" in the System Folder's Preferences folder.

You can click on the map to move there, and the message-line
prompts offer clickable buttons for yes/no questions.

Configuration
-------------
Edit "NetHack Defaults" with any text editor (SimpleText works);
the comments in the file describe each option.  Useful entries:

    OPTIONS=!tiled_map          start in ASCII even on color screens
    OPTIONS=menucolors          colored inventory entries

Menu color patterns use shell-style globs, so wrap them in '*':

    MENUCOLOR="* cursed *"=red
    MENUCOLOR="* blessed *"=cyan

Crashed Games
-------------
Checkpointing is on by default.  After a crash or power loss the
next start will refuse to begin a new game while the crashed
game's files are present.  Run the Recover application and choose
the crashed game's ".0" file (the level files are named after your
character: "1Brunhilda.0", "1Brunhilda.1", ...) to rebuild a save
file you can restore from.  To abandon the crashed game instead,
delete those numbered files from the game folder.

Notes
-----
Save files and bones files from earlier NetHack versions do not
work with 5.0.

Source code and build instructions (Retro68 cross-compile):
    https://github.com/ingpaschke/NetHack -- see sys/mac/BUILD.md

Based on the classic Macintosh port by Dean Luick, Kevin Hugo,
Mark Modrall, Jon W{tte, David Hairston, and Michael Hamel.
Revived for NetHack 5.0 by Ingo Paschke.

This is not an official NetHack port; please send bug
reports, suggestions, and comments to ipaschke@lpclabs.de.
