NetHack 3.4.0 Linux Elf

This README provides the instructions for using the official Linux binary,
system platform requirements, as well as steps used to create that binary.
The same steps can be used from the source distribution to create a similar
binary.

The official Linux binary has support for tty and X11 windowing systems, but
not Qt.  This means you will need to have X11 libraries installed on your
system to run this binary, even in its tty flavor.


The Linux binary package assumes that you have a user and a group named
"games" on your system.  If you do not, you can simplify installation by
creating them first.

gunzip and untar the package, preserving permissions, from / to put the
NetHack files in /usr/games/nethack and /usr/games/lib/nethackdir.

(If you have old record and logfile entries from a previous NetHack
version, you might want to save copies before they get overwritten by the
new empty files; old saved games and bones files won't work with 3.4.0).

In addition to data files for running the game, you will find other useful
things in /usr/games/lib/nethackdir (such as a copy of this README :-).

The general documentation Guidebook.txt and the processed man pages
nethack.txt and recover.txt should provide an introduction to the game.

The sample config file called dot.nethackrc can be used by copying
it to your home directory as .nethackrc and modifying it to your liking.

If you are running X11 copy the nh10.pcf and ibm.pcf font files from
/usr/games/lib/nethackdir to a X11 fonts directory (such as
/usr/X11/lib/X11/fonts/misc) and run "mkfontdir", then restart X
windows to load them.  Also consider putting NetHack.ad in
/usr/X11/lib/X11/app-defaults or its contents in your .Xdefaults or
.Xresources.


The official Linux binary is set up to run setgid games, which allows
multiple users on your system to play the game and prevents cheating by
unprivileged users.  The usual default for NetHack is setuid games, but
this causes problems with accessing .nethackrc on distributions with
restrictive default security on home directories and users who don't know
the tradeoffs of various permission settings.


If you have problems, send us some email.

nethack-bugs@nethack.org



Steps used to build this binary release

System:  egcs-1.1.2, XFree86-3.3.6, ncurses-5.0, glibc-2.1.3

0.  Makefile.top: GAMEGRP = games
		  GAMEPERM = 02775
		  FILEPERM = 0664
		  EXEPERM = 0775
		  DIRPERM = 0775
		  VARDATND = x11tiles pet_mark.xbm rip.xpm

1.  Makefile.src: define LINUX and linux options throughout
		  WINSRC = $(WINTTYSRC) $(WINX11SRC)
		  WINOBJ = $(WINTTYOBJ) $(WINX11OBJ)
		  WINTTYLIB = /usr/lib/libncurses.a
                  Include Xpm libs in WINX11LIB
		  WINLIB = $(WINTTYLIB) $(WINX11LIB)

2.  Makefile.utl: define LINUX and linux options throughout
                  Use bison/flex instead of yacc/lex

3.  config.h: define X11 window support, XPM support.
              define COMPRESS as /bin/gzip as that is where it
              seems to reside on newer Linux's.
              define COMPRESS_EXTENSION as ".gz"
              define DLB

4.  unixconf.h: define LINUX
                define TIMED_DELAY

5.  make all; (cd util; make recover); su; make install;
    cp util/recover /usr/games/lib/nethackdir/recover

6.  Convert nh10.bdf and ibm.bdf to proper font files and place in
    font path.
