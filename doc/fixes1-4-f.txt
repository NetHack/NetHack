		NetHack Fixes List	Revision 1.4

dogmove.c:	Death code fixed so dog with <1 hp doesn't "die of hunger".
		Slack leash message not invoked unless leash attached.
		Added "DOGNAME" option (thanks to Roland McGrath).

u_init.c:	Priest(esse)s start off with their weapon blessed.

spell.c:	Allows high level characters to cast spells upon themselves
zap.c:		(such as polymorph).

pray.c:		allows for de-cursing worn rings as well as weapons.
		Knights, Priests and Priestesses can now "#turn" undead
		(UNDEAD = "VWZ &").
		fixed bugs in blessings bestowed by gods.

read.c:		SPE_CAUSE_FEAR does not give a "You hear..." message unless no
		monsters are nearby.
		fix to bug in "do_genocide" which allowed player to wipe out
		all '@'s and survive.

polyself.c:	fix to rehumanize to catch players who wipe out '@'s while
		in polymorphed form as they de-polymorph.

wield.c:	bug causing segmentation fault on "w-" followed by "w[weapon]"
		fixed by chuq@sun

trap.c:		similar null pointer problem corrected.

make.exe.uu:	correction of names at top of uuencoded files which caused the
nansi.sys.uu:	original files to be overwritten by the decoded files when
		uudecode was invoked. (found by len@elxsi).

nethack.6:	general beautification and appropriate acknowledgement of
		trademarks to avoid getting sued. :-) (thanks chuq)

help:		addition of "V" and "#" commands to help text.

invent.c:	fix to REDO bug inhibiting the appearance of the item usage
		prompt.

makedefs.c:	added "{", "\" and corrected Rockmole definition in "data".
		fixed things for Dos users (file open modes, etc.)

cmd.c:		added #[command] auto-substitution.

termcap.c:	fixed termlib / curses dependencies.
		fixed null padding bug on output of SO/SE/HI/HE.

pcmain.c:	many fixes by Ralf Brown to allow the
pcunix.c:	program to be compiled using the Turbo C compiler in a Dos
Makefile.tcc:	enviornment.

engrave.c:	engravings burned or engraved into the floor (as opposed to
		those made with finger or marker) can be felt out when blind.
		(inspired by Stefan Wrammerfors).

mon.c:		fixed "monster looked at a strange trap" bug for 16 bit
		machines (long vs. int problem reported by mike@cimcor). 

fight.c:	hitmm() fixed to return 0 (no hit) if either monster passed
		in is non-existant.  This fixes a number of null reference
		problems (ex. monmove.c[~360]).
		Misc zero reference errors fixed by Paul Eggert. eggert@grand

Makefile.xenix:	New version (that works) supplied by Greg Laskin greg@smash

fountain.c:	Misc zero reference errors fixed by Paul Eggert. eggert@grand
search.c:

options.c:	New options added for GRAPHICS, DOGNAME, and new routine
		added to allow easier sorting of string parameters (":,"
		are considered to be string terminators for copying in
		name, and dogname).


