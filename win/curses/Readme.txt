INTRO
=====

The "curses" windowport is a new text-based interface for NetHack,
using high-level curses routines to control the display.  Currently, it
has been compiled and tested on Linux and Windows, but it should also
be portable to a number of other systems, such as other forms of UNIX,
Mac OS X, MSDOS, and OS/2.

Some features of this interface compared to the traditional tty
interface include:

 * Dynamic window resizing (e.g. maximizing a terminal window)
 * Dynamic configurable placement of status and message windows,
 relative to the map
 * Makes better use of larger terminal windows
 * Fancier display (e.g. window borders, optional popup dialogs)
 * "cursesgraphics" option for fancier line-drawing characters for
 drawing the dungeon - this should work on most terminals/platforms


BUILDING
========

As of this writing code has been compiled on Linux and Windows.

UNIX/Linux build instructions: Follow the instructions in
sys/unix/Install.unx.  By default, the Makefile is setup to compile
against ncurses.  Edit Makefile.src if you wish to compile against a
different curses library, such as PDCurses for SDL.

Windows build instructions: If you are using Mingw32 as your compiler,
then follow the instructions in sys/winnt/Install.nt with the following
changes:

 * After running nhsetup, manually copy the file cursmake.gcc to the
 src/ subdirectory
 * Instead of typing "mingw32-make -f Makefile.gcc install" you will
 type "mingw32-make -f cursmake.gcc install"

If you are using a different compiler, you will have to manually modify
the appropriate Makefile to include the curses windowport files.


GAMEPLAY
========

Gameplay should be similar to the tty interface for NetHack; the 
differences are primarily visual.  This windowport supports dymanic
resizing of the terminal window, so you can play with it to see how it
looks best to you during a game.  Also, the align_status and
align_message options may be set during the game, so you can experiment
to see what arraingement looks best to you.

For menus, in addition to the normal configurable keybindings for menu
navigation descrived in the Guidebook, you can use the right and left
arrows to to forward or backward one page, respectively, and the home
and end keys to go to the first and last pages, respectively.

Some configuration options that are specific to or relevant to the
curses windowport are shown below.  Copy any of these that you like to
your nethack configuration file (e.g. .nethackrc for UNIX or
NetHack.cnf for Windows):
#
# Use this if the binary was compiled with multiple window interfaces,
# and curses is not the default
OPTIONS=windowtype:curses
#
# Set this for Windows systems, or for PDCurses for SDL on any system.
# The latter uses a cp437 font, which works with this option
#OPTIONS=IBMgraphics
#
# Set this if IBMgraphics above won't work for your system.  Mutually
# exclusive with the above option, and should work on nearly any
# system.
OPTIONS=cursesgraphics
#
# Optionally specify the alignment of the message and status windows
# relative to the map window.  If not specified, the code will default
# to the locations used in the tty interface: message window on top,
# and status window on bottom.  Placing either of these on the right or
# left really only works well for winder terminal windows.
OPTIONS=align_message:bottom,align_status:right
#
# Use a small popup "window" for short prompts, e.g. "Really save?".
# If this is not set, the message window will be used for these as is
# done for the tty interface.
OPTIONS=popup_dialog
#
# Specify the initial window size for NetHack in units of characters.
# This is supported on PDCurses for SDL as well as PDCurses for
# Windows.
OPTIONS=term_cols:110,term_rows:32
#
# Controls the usage of window borders for the main NetHack windows
# (message, map, and status windows).  A value of 1 forces the borders
# to be drawn, a value of 2 forces them to be off, and a value of 3
# allows the code to decide if they should be drawn based on the size
# of the terminal window.
OPTIONS=windowborders:3


CONTACT
=======

Please send any bug reports, suggestions, patches, or miscellaneous
feedback to me (Karl Garrison) at: kgarrison@pobox.com.  Note that as
of this writing, I only have sporatic Internet access, so I may not get
back to you right away.

Happy Hacking!

Karl Garrison
March, 2009


