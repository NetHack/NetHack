         NetHack 5.0.0 -- General information

NetHack 5.0 is an enhancement to the dungeon exploration game NetHack,
which is a distant descendent of Rogue and Hack, and a direct descendent
of NetHack 3.6.

NetHack 5.0.0 is a release of NetHack. As a .0 version, there may be some
bugs encountered. Constructive suggestions, GitHub pull requests, and bug
reports are all welcome and encouraged.

The file doc/fixes5-0-0.txt in the source distribution contains a full list
of fixes and changes as they were committed. The text in there was written
for the development team's own use and is provided  "as is", so please do
not ask us to further explain the entries in that file. Some entries might
be considered "spoilers", particularly in the "new features" section.

Along with the game improvements and bug fixes, NetHack 5.0 strives to make
some general architectural improvements to the game or to its building
process. Among them, 5.0 has:

 *  The source code is compliant with the C99 standard.

 *  removes barriers to building NetHack on one platform and operating system,
    for later execution on another (possibly quite different) platform and/or
    operating system. That capability is generally known as "cross-compiling."
    See the file "Cross-compiling" in the top-level folder for more information
    on that.

 *  The build-time "yacc and lex"-based level compiler, the
    "yacc and lex"-based dungeon compiler, and the quest text file processing
    previously done by NetHack's "makedefs" utility, have been replaced with
    Lua text alternatives that are loaded and processed by the game during play.

Here are some other general notes on the changes in NetHack 5.0 that were not
considered spoilers:
 -  automatic annotation "gateway to Moloch's Sanctum" for vibrating square
        level once that square's location becomes known (found or magic
        mapped); goes away once sanctum temple is found (entered or high altar
        mapped)

                        - - - - - - - - - - -

Please read items (1), (2) and (3) BEFORE doing anything with your new code.

1.  Unpack the code in a dedicated new directory.  We will refer to that
    directory as the 'Top' directory.  It makes no difference what you
    call it.

2.  Having unpacked, you should have a file called 'Files' in your Top
    directory.

    This file contains the list of all the files you now SHOULD
    have in each directory.  Please check the files in each directory
    against this list to make sure that you have a complete set.

    This file also contains a list of what files are created during
    the build process.

    The names of the directories listed should not be changed unless you
    are ready to go through the makefiles and the makedefs program and change
    all the directory references in them.

3.  Before you do anything else, please read carefully the file called
    "license" in the 'dat' subdirectory.  It is expected that you comply
    with the terms of that license, and we are very serious about it.

4.  If you are attempting to build NetHack on one platform/processor, to
    produce a game on a different platform/processor it may behoove you to
    read the file "Cross-compiling" in your Top directory.

5.  If everything is in order, you can now turn to trying to get the program
    to compile and run on your particular system.  It is worth mentioning
    that the default configuration is SysV/Sun/Solaris2.x (simply because
    the code was housed on such a system).

    The files sys/*/Install.*, or sys/*/NewInstall.* were written to guide
    you in configuring the program for your operating system.  The files
    win/*/Install.* are available, where necessary, to help you in
    configuring the program for particular windowing environments.
    Reading them, and the man pages, should answer most of your questions.


    At the time of the most recent official release, NetHack 5.0, it had
    been tested to run and/or compile on:

        Intel Pentium or better running Linux, BSDI
            [compiles, runs]
        Intel Pentium or better running Windows 10 or 11
            [compiles, runs]
        Intel-based, or Apple M1, M2, M3, M4, M5 Macs running
            macOS 10.11 (El Capitan) to macOS 26 (Tahoe)
            [compiles, runs]
            (follow the instructions in sys/unix/NewInstall.unx)
        Intel 80386 or greater running MS-DOS with DPMI
            [cross-compiles on Linux, runs on MS-DOS, emulator/dosbox]
            built via djgpp compiler (native or Linux-hosted cross-compiler)
        AmigaOS 3.0 or later
            [cross-compiles on Linux or macOS, runs on Amiga or emulator]
            Running requires Kickstart 39+
            6 MB free RAM minimum (8 MB recommended)
            Hard drive with ~5 MB free space

    There has also been some success runing/comiling on:
        OpenVMS (aka VMS) V8.4 on Alpha and on Integrity/Itanium/IA64
        OpenVMS (ask VMS) V9.2-3 on x86-64

                        - - - - - - - - - - -

If you have problems building the game, or you find bugs in it, we recommend
filing a bug report from our "Contact Us" web page at:
    https://www.nethack.org/common/contact.html
Please include the version information from #version or the command line
option --version in the appropriate field.

A public repository of the latest NetHack code that we've made
available can be obtained via git here:
    https://github.com/NetHack/NetHack
      or
    https://sourceforge.net/p/nethack/NetHack/

When sending correspondence, please observe the following:
o Please be sure to include your machine type, OS, and patchlevel.
o Please avoid sending us binary files (e.g. save files or bones files).
  If you have found a bug and think that your save file would aid in solving
  the problem, send us a description in words of the problem, your machine
  type, your operating system, and the version of NetHack.  Tell us that you
  have a save file, but do not actually send it.
  You may then be contacted by a member of the development team with the
  address of a specific person to send the save file to.
o Though we make an effort to reply to each bug report, it may take some
  time before you receive feedback.  This is especially true during the
  period immediately after a new release, when we get the most bug reports.
o We don't give hints for playing the game.
o Don't bother to ask when the next version will be out or you can expect
  to receive a stock answer.

If you want to submit a patch for the NetHack source code via email directly,
you can direct it to this address:
    nethack-bugs (at) nethack.org

If a feature is not accepted you are free, of course, to post the patches
to the net yourself and let the marketplace decide their worth.

All of this amounts to the following:  If you decide to apply a free-lanced
patch to your NetHack 5.0 code, you are welcome to do so, of course, but we
won't be able to provide support or receive bug reports for it.

In our own patches, we will assume that your code is synchronized with ours.

                  -- Good luck, and happy Hacking --

# $NHDT-Date: 1779926755 2026/05/28 00:05:55 $ $NHDT-Branch: NetHack-5.0 $:$NHDT-Revision: 1.113 $
# Copyright (c) 2012 by Michael Allison
# NetHack may be freely redistributed.  See license for details.
