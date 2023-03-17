/* NetHack 3.7	patchlevel.h	$NHDT-Date: 1651297020 2022/04/30 05:37:00 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.207 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef PATCHLEVEL_H
#define PATCHLEVEL_H

/* NetHack 3.7.x */
#define VERSION_MAJOR 3
#define VERSION_MINOR 7
/*
 * PATCHLEVEL is updated for each release.
 */
#define PATCHLEVEL 0
/*
 * Incrementing EDITLEVEL can be used to force invalidation of old bones
 * and save files.
 */
#define EDITLEVEL 80

/*
 * Development status possibilities.
 */
#define NH_STATUS_RELEASED    0         /* Released */
#define NH_STATUS_WIP         1         /* Work in progress */
#define NH_STATUS_BETA        2         /* BETA testing */
#define NH_STATUS_POSTRELEASE 3         /* patch commit point only */

/*
 * Development status of this NetHack version.
 */
#define NH_DEVEL_STATUS NH_STATUS_WIP

#ifndef DEBUG  /* allow tool chains to define without causing warnings */
#define DEBUG
#endif

#define COPYRIGHT_BANNER_A "NetHack, Copyright 1985-2023"
#define COPYRIGHT_BANNER_B \
    "         By Stichting Mathematisch Centrum and M. Stephenson."
/* nomakedefs.copyright_banner_c is generated at runtime */
#define COPYRIGHT_BANNER_C nomakedefs.copyright_banner_c
#define COPYRIGHT_BANNER_D "         See license for details."

/*
 * If two or more successive releases have compatible data files, define
 * this with the version number of the oldest such release so that the
 * new release will accept old save and bones files.  The format is
 *      0xMMmmPPeeL
 * 0x = literal prefix "0x", MM = major version, mm = minor version,
 * PP = patch level, ee = edit level, L = literal suffix "L",
 * with all four numbers specified as two hexadecimal digits.
 *
 * When developing a new version, VERSION_COMPATIBILITY should be
 * commented out so that increments of EDITLEVEL can be used to forcibly
 * invalidate existing save files whenever incompatible changes are made
 * to saved data.  [Bones files should be less sensitive (only changes
 * to individual level files matter; changes to general game state don't)
 * but the extra complexity to support that is not worth the effort.]
 */
/*#define VERSION_COMPATIBILITY 0x03070000L*/

/****************************************************************************/
/* Version 3.7.x */

/*
 *  NetHack 3.7.0, <insert date here>
 *
 */

/****************************************************************************/
/* Version 3.6.x */

/*  Patch 7, February 16, 2023
 *
 *  during engraving, spaces were counted instead of non-space (cherry-pick of
 *      4e0a1e04 from NetHack-3.7)
 *  avoid potential buffer overflow in append_str()
 *  resolve missing dependency in NetHack.sln
 *  code in include/tradstdc.h was trying to suppress warn_unused result by
 *      defining warn_unused_result to an empty string and that began causing
 *  a build error within a system-supplied header file cdefs.h when using
 *      ubuntu impish 21.10; disable that for any Linux unless GCC_URWARN is
 *      defined to force it back into effect
 *  update_inventory() after leash goes slack
 *  player assigned name for monsters, specific objects, or object types could be
        longer than what was intented to be allowed; for 'curses', much longer
 *  windows: added winflexbison to travis-ci configuration to permit full build of
 *      levcomp and dgncomp
 *  windows: a bad chdir specified in win/win32/dgnstuff.mak caused full build to
 *      abort
 *  windows: the console.rc file had outdated information stating 3.6.3 when the
 *      official 3.6.6 binary was built.
 *  windows: switch from using keyhandling dll's to incorporating the three
 *      variations (default, ray, 340) in sys/winnt/nttty.c
 *  curses: cherry-picked selectsaved code from 3.7 for menu of save files
 *  NetHackW: fix delayed rendering of cursor when using farlook
 */

/*  Patch 6, March 8, 2020
 *
 *  invalid status highlight color could be maliciously used to corrupt memory
 *  formatting corpse names used internal buffers differently from formatting
 *      other objects and could potentially clobber memory
 *  avoid divide by 0 crash if 'bogusmon' (file of bogus monster types) is empty
 *  avoid #wizrumorcheck crash if either 'rumors.tru' or 'rumors.fal' or both
 *      were empty when makedefs built 'rumors'
 */

/*  Patch 5, January 27, 2020
 *
 *  fix accessing mons[-1] when trying to gate in a non-valid demon
 *  fix accessing mons[-1] when monster figures out if a tin cures stoning
 *  have string_for_opt() return empty_optstr on failure
 *  ensure existing callers of string_for_opt() check return value before using it
 *  fix potential buffer overflow in add_menu_coloring()
 *  fix potential buffer overflow in sym_val()
 *  fix potential buffer overflow in pline(), raw_printf(), and config_error_add()
 *  fix potential buffer overflow in choose_windows()
 *  use vsnprintf instead of vsprintf in pline.c where possible
 *  Windows: includes a fix from a 3.6.4 post-release update where
 *      OPTIONS=map_mode:fit_to_screen could cause a game start failure
 *  Windows: users with C-locale unmappable names could get game start failure
 */

/*  Patch 4, December 18, 2019
 *
 *  fix potential buffer overflow when parsing run-time configuration file
 *  GDBPATH and GREPPATH from sysconf or -D... on compilation command line were
 *      being processed even if PANICTRACE was disabled but only being freed
 *      at end of game when that was enabled
 *  fix the article used in the message when your steed encounters a polymorph
 *      trap
 *  allow teleporting onto the vibrating square
 *  message "your knapsack can't accommodate any more items" when picking stuff
 *      up or removing such from container was inaccurate if there was some
 *      gold pending; vary the message rather than add more convoluted pickup
 *      code
 *  dozen-ish assorted spelling/typo fixes in messages and source comments
 *  wizard mode wishing for terrain would leave it unmapped if done while blind
 *  wizard mode terrain wish could leave hero in water (severe vision limits)
 *      or in lava (trapped, sinking) which wasn't there any more
 *  flying hero can go down (via '>') holes or trap doors instead of escaping
 *      trap
 *  polymorphed hero hiding on the ceiling can now use '>' to unhide instead of
 *      being told "you can't go down here"
 *  fix compilation on platforms that split the ncurses and tinfo libraries
 *  Windows: allow all game files to be on a portable device via the sysconf
 *      option 'portable_device_paths'
 */

/*  Patch 3, December 5, 2019
 *
 *  Fixed stale 'thrownobj' pointer for returning thrown aklys while engulfed
 *  Fixed uarmh null pointer dereference if a helm of opposite alignment came
 *     off due to being polymorphed
 *  Fixed 'object lost' panic when attempting to crawl of of the water during
 *     emergency disrobing/dropping
 *  Running now stops when moving over engravings so you can tell where they are
 *  Fixed detection of unseen/secret doors which failed to find monsters hiding
 *     under objects and failed to find monsters hiding at trap locations
 *  Ensured fatal status conditions made it to disclosure and/or dumplog
 *  Fixed "Bad fruit #N" warnings when saving bones with 'perm_invent' On
 *  Fixed it so yellow dragons don't have green breath
 *  Added several grammar corrections
 *  Improved recognition of deafness for several situations including the
 *     playing of musical instruments and bribing negotiations with demon lords
 *  Fixed ignoring of god's wrath when hero injured himself during altar kick
 *  Fixed several cases where persistent inventory window was not updated
 *  Fixed temple priests or shopkeepers moving over other monsters
 *  Fixed a crash-causing bug when putting on levitation boots over a sink
 *  Ensured that thrown or kicked objects will end up in bones
 *  Made water go all the way to the edges of level on the Plane of Water
 *  Made clouds disrupt line of sight along the edges of the Plane of Air
 *  Improved and expanded usage of status highlighting percentage rules
 *  Added more than 15 improvements and fixes to the curses window port
 *  Added and documented significant changes to default directory choices for
 *    the Windows platform
 *  Improved the layout and display of the player selection dialog used on the
 *     Windows graphical implementation
 *  Improved performance of some tty versions by reducing the number of
 *     function calls made from mapglyph
 *  Allowed the msdos implementation to build with curses and PDCurses
 *  Included over 100 other fixes and improvements as outlined in doc/fixes36.3
 */

/*  Patch 2, May 7, 2019
 *
 *  Over 320 bug fixes including a couple of crash bug fixes as outlined in
 *      doc/fixes36.2
 *  More than 15 enhancements or improvements
 *  Ensuring that unix Makefiles do not rely on features unique to gnu make
 *  Improvements to hilite_status parsing in an effort to ensure that expected
 *      and sensible values ranges are more easily specified and accepted
 *  Performance optimizations for tty
 *  Windows console tty via use of a back buffer
 *  Mac OS X build improvements and inclusion of an Xcode project
 *  Adopted the community curses window port
 */

/*  Patch 1, April 27, 2018
 *
 *  Over four hundred and seventy bug fixes and improvements as outlined in
 *      doc/fixes36.1
 */

/*
 *  NetHack 3.6.0, December 7, 2015
 *
 *  Hundreds of bug fixes as outlined in doc/fixes36.0.
 *  Some code reorganization.
 *  Some new features.
 *  Variations of some community patches rolled in.
 */

/****************************************************************************/
/* Version 3.5.x */

/*  Version 3.5 was never officially released.
 */

/****************************************************************************/
/* Version 3.4.x */

/*  Patch 3, December 7, 2003
 *  Several dozen general bug fixes including at least one fatal bug
 *  Correct several inconsistencies
 *  Handle level completely filled with monsters better
 *  Performance enhancements for win32tty port on Windows 98 and Me
 *  win32gui player selection fixes
 *  X11 player selection fixes, one of which could be fatal
 *  Eliminated a gold-in-shop-container cheat
 *  Include bones file version compatibility info in options file
 */

/*  Patch 2, August 30, 2003
 *  Fix a fatal bug that caused a crash when applying figurine, candle, or
 *      bell that gets used up
 *  Fix a fatal bug that triggered a panic when your secondary weapon was
 *      cursed during bones file creation
 *  Several dozen general bug fixes
 *  Fixed some Gnome compilation problems on Redhat 7.2 and 8.0
 *  Fixed a problem in the util Makefile
 *  Use random() by default under linux instead of lrand48()
 *  win32 tty adjustments and support for loading alternative key handlers
 */

/*  Patch 1, February 22, 2003
 *  Fix a few fatal errors including one for reentering shops, one
 *     involving land mines and boulders/statues, one for delayed
 *     polymorph, and one from a chest trap exploding ball and chain
 *  Fix a buffer overflow that could lead to security problems
 *  Hundreds of general bug fixes
 *  Several message and other glitches corrected
 *  Travel command adjustments and ability to disable travel command
 *  message recall window extensions (by Christian Cooper)
 *  win32: some interface improvements
 *  unix: improved tile support
 *  gnome: some fixes, and some enhancements by Dylan Alex Simon
 *  winCE: Windows CE port included (by Alex Kompel)
 */

/*
 *  NetHack 3.4.0, March 20, 2002
 *
 *  Hundreds of general bug fixes including some for sliming, zapping,
 *conduct,
 *      and several more for riding
 *  Eliminated a few potentially fatal bugs including one for stone-to-flesh,
 *      trouble-fixing during prayer, riding down stairs while punished,
 *      polyd player demon summoning, throwing digging tools into shops, and
 *      a couple from having the vision system enabled at inappropriate times
 *  Corrected some incorrect calculations in final scoring
 *  Enhanced config file processing and alert to duplication of entries
 *  Player selection prompt enhancements for TTY and X11
 *  Objects merge in containers
 *  Wish for "nothing", and genocide "none" to preserve your conduct
 *  Changes to Wizard quest
 *  Added the travel command which works by mouse click or '_' command
 *  Config file BOULDER option to specify the symbol for displaying boulders
 *  Incorporate modified versions of several 3.3.1 patches that have been
 *      in circulation in the NetHack community
 *  New Gnomish Mines levels (courtesy Kelly Bailey)
 *  Mac: command-key shortcuts in the player selection dialog
 *  Amiga: screenmode requester, and several amiga specific bug fixes
 *  Win32 graphical port contributed by Alex Kompel is now included
 */

/* Version 3.4 */

/****************************************************************************/
/* Version 3.3.x */

/*  Patch 1, August 9, 2000
 *  Many, many general fixes, including a number for riding, twoweapon,
 *      and invisible monsters
 *  A security fix for a couple of potentially exploitable buffer overflows
 *      in previous versions
 *  Redo Ranger quest
 *  Introduction of differentiation between different causes of blindness
 *  Overhaul of warning
 *  Functionality restored to Amiga (courtesy Janne Salmijarvi) and Atari
 *      (courtesy Christian "Marvin" Bressler) ports
 *  Mac: multiple interface fixes
 *  win32: fixed bug that caused messages to stop displaying after escape
 *  tty: use ANSI color (AF) over standard color (Sf) when given the choice
 *  several ports: offer for player selection only choices consistent with
 *      those already made by config file/command line (e.g., only offer roles
 *      that are compatible with specified race)
 */

/*
 *  NetHack 3.3.0, December 10, 1999
 *
 *  Implement the differentiation of character class or role from the
 *  character race.
 *  Removal of the Elf class, in preference to the Elf as a race.
 *  Introduction of Dwarves, Elves, Gnomes and Orcs as distinct races in
 *  addition to the Human "norm".
 *  Addition of the Monk and Ranger classes.
 *  Integrate some of the features of several branch versions of the game,
 *  notably NetHack--, NHplus, SLASH, and Slash'em.
 *  Adopt "the wizard patch" spellcasting system.
 *  Support for the Qt widget set.
 *  Y2K fix: use 4 digit year values for the dates in the score file
 *  updated COPYRIGHT_BANNER_A to reflect year of release.
 *  Dozens of other bug fixes, and minor improvements.
 */

/* Version 3.3 */

/****************************************************************************/
/* Version 3.2.x */

/*  Patch 3, December 10, 1999
 *  Released simultaneously with 3.3.0 for the benefit of
 *  ports and platforms that were unable to get working
 *  versions of 3.3.0 ready prior to the year 2000. It
 *  consisted of just a few bug fixes and offered no new
 *  functionality changes over 3.2.2.
 *
 *  Y2K fix: use 4 digit year values for the dates in the score file
 *  updated COPYRIGHT_BANNER_A to reflect year of release
 *  Fatal Mac bug removed
 *  DOS Makefile problem removed
 *  several bugs that could potentially trigger crashes removed
 */

/*  Patch 2, December 10, 1996
 *  fix the `recover' utility
 *  fix priest/minion name overflow which could cause Astral Plane crashes
 *  avoid crash when hit by own thrown boomerang
 *    "     "    "   worn blindfold pushed off by applying cursed towel
 *  handle returning live Wizard correctly in deep dungeon levels
 *  don't occasionally display unseen areas of new levels during level change
 *  other minor display fixes
 *  fix several minor reason for death inconsistencies and shop bugs
 *  high dexterity doesn't guarantee that thrown eggs & potions will hit
 *
 *  Selected platform- or configuration-specific changes:
 *  Mac: update `mrecover'
 *  MSDOS: don't switch into tiles mode when resuming play on rogue level
 *  tty: support object class characters for 'I' command in menu mode
 *  Unix: work around several <curses.h> compilation problems
 *  X11: as tty above, plus implement tty-style count handling in menus;
 *      better window placement support for old window managers
 */

/*  Patch 1, May 28, 1996
 *  eliminate `#qualifications'; fix weapon proficiency handling for missiles
 *  keep Medusa from continuing to move after she's been killed by reflection
 *      of her own gaze (fixes relmon panic)
 *  make monsters a little smarter; assorted eating and chatting changes
 *  fix object amnesia for spellbooks; fix Nazgul's sleep gas attack
 *  fix bullwhip usage for case of having recently been in a trap
 *  egg hatching fixes, oil potion fixes, magic marker fixes
 *  support object class chars as selection accelerators for some menus
 *  stricter parsing of run-time options at startup time
 *  interactive setting of options via menu (courtesy Per Liboriussen)
 *
 *  Selected platform- or configuration-specific changes:
 *  Amiga: fix panic for tiles display in Gnomish mines
 *  BeOS: preliminary support for new BeBox platform; initially tty only
 *  DLB: avoid excessive fseek calls (major performance hit for MSDOS)
 *  HPUX: workaround for gcc-2.6.3 bug adversely affecting monster generation
 *  Mac: avoid MW 68K struct copy optimization bug which caused crashes;
 *      fix dragging of scrollbar; boost partitions to 2MB minimum
 *  MSDOS: wasn't safe to enter endgame for MFLOPPY configuration;
 *      fix re-entry into game after "!" (shell escape) + chdir + EXIT;
 *      F3/F4/F5 display interface swapping improvements;
 *      add support for preloading all tiles in protected mode environment
 *  TERMINFO: colors were wrong for some systems, such as Linux
 *  X11: display help files properly
 */

/*
 *  NetHack 3.2.0, April 11, 1996
 *  enhancements to the windowing systems including "tiles" or icons to
 *      visually represent monsters and objects (courtesy Warwick Allison)
 *  window-based menu system introduced for inventory and selection
 *  moving light sources besides the player
 *  improved #untrap (courtesy Helge Hafting)
 *  spellcasting logic changes to balance spellcasting towards magic-using
 *      classes (courtesy Stephen White)
 *  many, many bug fixes and abuse eliminations
 */

/* Version 3.2 */

/****************************************************************************/
/* Version 3.1.x */

/*
 *  Patch 3, July 12, 1993
 *  further revise Mac windowing and extend to Think C (courtesy
 *      Barton House)
 *  fix confusing black/gray/white display on some MSDOS hardware
 *  remove fatal bugs dealing with horns of plenty and VMS bones levels,
 *      as well as more minor ones
 */

/*
 *  Patch 2, June 1, 1993
 *  add tty windowing to Mac and Amiga ports and revise native windowing
 *  allow direct screen I/O for MS-DOS versions instead of going through
 *      termcap routines (courtesy Michael Allison and Kevin Smolkowski)
 *  changes for NEC PC-9800 and various termcap.zip fixes by Yamamoto Keizo
 *  SYSV 386 music driver ported to 386BSD (courtesy Andrew Chernov) and
 *      SCO UNIX (courtesy Andreas Arens)
 *  enhanced pickup and disclosure options
 *  removed fatal bugs dealing with cursed bags of holding, renaming
 *      shopkeepers, objects falling through trapdoors on deep levels,
 *      and kicking embedded objects loose, and many more minor ones
 */

/*
 *  Patch 1, February 25, 1993
 *  add Windows NT console port (courtesy Michael Allison)
 *  polishing of Amiga, Mac, and X11 windowing
 *  fixing many small bugs, including the infamous 3.0 nurse relmon bug
 */

/*
 *  NetHack 3.1.0, January 25, 1993
 *  many, many changes and bugfixes -- some of the highlights include:
 *  display rewrite using line-of-sight vision
 *  general window interface, with the ability to use multiple interfaces
 *      in the same executable
 *  intelligent monsters
 *  enhanced dungeon mythology
 *  branching dungeons with more special levels, quest dungeons, and
 *      multi-level endgame
 *  more artifacts and more uses for artifacts
 *  generalization to multiple shops with damage repair
 *  X11 interface
 *  ability to recover crashed games
 *  full rewrite of Macintosh port
 *  Amiga splitter
 *  directory rearrangement (dat, doc, sys, win, util)
 */

/* Version 3.1 */

/****************************************************************************/
/* Version 3.0 */

/*
 *  Patch 10, February 5, 1991
 *  extend overlay manager to multiple files for easier binary distribution
 *  allow for more system and compiler variance
 *  remove more small insects
 */

/*
 *  Patch 9, June 26, 1990
 *  clear up some confusing documentation
 *  smooth some more rough edges in various ports
 *  and fix a couple more bugs
 */

/*
 *  Patch 8, June 3, 1990
 *  further debug and refine Macintosh port
 *  refine the overlay manager, rearrange the OVLx breakdown for better
 *      efficiency, rename the overlay macros, and split off the overlay
 *      instructions to Install.ovl
 *  introduce NEARDATA for better Amiga efficiency
 *  support for more VMS versions (courtesy Joshua Delahunty and Pat Rankin)
 *  more const fixes
 *  better support for common graphics (DEC VT and IBM)
 *  and a number of simple fixes and consistency extensions
 */

/*
 *  Patch 7, February 19, 1990
 *  refine overlay support to handle portions of .c files through OVLx
 *      (courtesy above plus Kevin Smolkowski)
 *  update and extend Amiga port and documentation (courtesy Richard Addison,
 *      Jochen Erwied, Mark Gooderum, Ken Lorber, Greg Olson, Mike Passaretti,
 *      and Gregg Wonderly)
 *  refine and extend Macintosh port and documentation (courtesy Johnny Lee,
 *      Kevin Sitze, Michael Sokolov, Andy Swanson, Jon Watte, and Tom West)
 *  refine VMS documentation
 *  continuing ANSIfication, this time of const usage
 *  teach '/' about differences within monster classes
 *  smarter eating code (yet again), death messages, and treatment of
 *      non-animal monsters, monster unconsciousness, and naming
 *  extended version command to give compilation options
 *  and the usual bug fixes and hole plugs
 */

/*
 *  Patch 6, November 19, 1989
 *  add overlay support for MS-DOS (courtesy Pierre Martineau, Stephen
 *      Spackman, and Norm Meluch)
 *  refine Macintosh port
 *  different door states show as different symbols (courtesy Ari Huttunen)
 *  smarter drawbridges (courtesy Kevin Darcy)
 *  add CLIPPING and split INFERNO off HARD
 *  further refine eating code wrt picking up and resumption
 *  make first few levels easier, by adding :x monsters and increasing initial
 *      attribute points and hitting probability
 *  teach '/' about configurable symbols
 */

/*
 *  Patch 5, October 15, 1989
 *  add support for Macintosh OS (courtesy Johnny Lee)
 *  fix annoying dependency loop via new color.h file
 *  allow interruption while eating -- general handling of partially eaten
 * food
 *  smarter treatment of iron balls (courtesy Kevin Darcy)
 *  a handful of other bug fixes
 */

/*
 *  Patch 4, September 27, 1989
 *  add support for VMS (courtesy David Gentzel)
 *  move monster-on-floor references into functions and implement the new
 *      lookup structure for both objects and monsters
 *  extend the definitions of objects and monsters to provide "living color"
 *      in the dungeon, instead of a single monster color
 *  ifdef varargs usage to satisfy ANSI compilers
 *  standardize on the color 'gray'
 *  assorted bug fixes
 */

/*
 *  Patch 3, September 6, 1989
 *  add war hammers and revise object prices
 *  extend prototypes to ANSI compilers in addition to the previous MSDOS ones
 *  move object-on-floor references into functions in preparation for planned
 *      data structures to allow faster access and better colors
 *  fix some more bugs, and extend the portability of things added in earlier
 *      patches
 */

/*
 *  Patch 2, August 16, 1989
 *  add support for OS/2 (courtesy Timo Hakulinen)
 *  add a better makefile for MicroSoft C (courtesy Paul Gyugyi)
 *  more accommodation of compilers and preprocessors
 *  add better screen-size sensing
 *  expand color use for PCs and introduce it for SVR3 UNIX machines
 *  extend '/' to multiple identifications
 *  allow meta key to be used to invoke extended commands
 *  fix various minor bugs, and do further code cleaning
 */

/*
 *  Patch 1, July 31, 1989
 *  add support for Atari TOS (courtesy Eric Smith) and Andrew File System
 *      (courtesy Ralf Brown)
 *  include the uuencoded version of termcap.arc for the MSDOS versions that
 *      was included with 2.2 and 2.3
 *  make a number of simple changes to accommodate various compilers
 *  fix a handful of bugs, and do some code cleaning elsewhere
 *  add more instructions for new environments and things commonly done wrong
 */

/*
 *  NetHack 3.0 baseline release, July, 1989
 */

/* Version 3.0 */

/****************************************************************************/

#endif /* PATHLEVEL_H */
/*patchlevel.h*/
