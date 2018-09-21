		NetHack Fixes List	Revision 2.3e

	New Files:

Fixes.2.3	This file.

Makefile.3B2	A new Makefile for the ATT 3B2.

Spoilers.mm	As above - pre nroff -mm

food.tbl	Tables for above manual
fount.tbl
monster.tbl
weapon.tbl

nansi.doc	Documentation for nansi.sys.

hh		Fixed improper def of "X" for casting spells.

fight.c		Dogname fix for "you hit your dog" line.  (Janet Walz)

pri.c		Patch for "SCORE_ON_BOTL".  (Gary Erickson)
config.h

pray.c		Null refrence bug fix for Suns. (The Unknown Hacker)

termcap.c	null pointer bug fix. (Michael Grenier)

mon.c		chameleon shape made dependent on current lev. (RPH)
spell.c
monmove.c

rnd.c		fixed SYSV/BSD problems with random(). (many sources)
config.h

shk.c		fix to allow compilation with or without HARD option.
		(reported by The Unknown Hacker)

termcap.c	ospeed fix for SYSV. (reported by Eric Rapin)

unixunix.c	fix for NFS sharing of playground. (Paul Meyer)

options.c	many misc. bugs while compiling for the PC. (Tom Almy)
pcmain.c
pctty.c
hack.h
nethack.cnf
pager.c
fight.c

mkroom.h	Addition of reorganized special-room code.
mkshop.c	(Eric S. Raymond)
mklev.c
rnd.c

trap.c		Fixed teleport to hell problems caused by rnz().
		(reported by Janet Walz)

termcap.c	Various fixes to MSDOSCOLOR code. (Tom Almy)
end.c

hack.c		Fixed typos. (Steve Creps)
msdos.c
pcmain.c

config.h	Enabled the use of termcap file c:\etc\termcap or
termcap.c	.\termcap.cnf for MS Dos (Steve Creps)

eat.c		Continued implementation of Soldier code.
makedefs.c	Added Barracks code. (Steve Creps)
makemon.c
mklev.c
mkroom.h
objects.h
mkshop.c
mon.c
monst.c
permonst.h

makedefs.c	Added Landmines code. (Steve Creps)
mklev.c
trap.c

mkroom.h	Fixed item / shop probablility code. (Stefan Wrammerfors)
shknam.c	Added additional item typ available.

pray.c		Randomized time between prayers. (Stefan Wrammerfors)

fight.c		Fixed typo. (Stefan Wrammerfors)

mhitu.c		Fixed monster special abilities usage bug.
		(Stefan Wrammerfors)

objnam.c	Randomized max number and max bonus limits on objects
		wished for.
		Identified cursed items shown as such. (Stefan Wrammerfors)

topten.c	Added logfile to allow overall game tuning for balance.
config.h	(Stefan Wrammerfors)

mklev.c		moved code identifying medusa & wizard levs to eliminate
*main.c		garbage on screen. (Izchak Miller)

u_init.c	fixed up luck wraparound bug (Izchak Miller)
hack.c
dothrow.c
fight.c
mon.c
play.c
save.c
sit.c

mon.c		fixed null referenced pointer bug. (The Unknown Hacker)

config.h	Hawaiian shirt code by Steve Linhart
decl.c
do_wear.c
extern.h
invent.c
mhitu.c
obj.h
objects.h
objnam.c
polyself.c
read.c
steal.c
u_init.c
worn.c

config.h	"THEOLOGY" code addition (The Unknown Hacker)
pray.c

mklev.c		Added typecasts to fix object generation bug on Microport
fountain.c	Sys V/AT. (Jerry Lahti)

Makefile.unix	Added ${CFLAGS} to makedefs build line. (Jerry Lahti)
Makefile.att
Makefile.3B2

pager.c		Inventory fix for "things that are here". (Steve Creps)

cmd.c		More wizard debugging tools:
			^F = level map
			^E = find doors & traps		(Steve Creps)

apply.c		Blindfolded set/unset to "INTRINSIC" (many sources)

lev.c		new restmonchn() code. (Tom May)
save.c

o_init.c	OS independance in init_objects (Tom May)
objclass.h	(removal of oc_descr_i)

shk.c		declaration of typename() (Tom May)

apply.c		declaration of lmonnam() (Tom May)

mklev.c		fixes to make medusa and wizard levels dependent on MAXLEVEL
		(Richard Hughey as pointed out to him by John Sin)

fountain.c	added "coins in fountain" code. (Chris Woodbury)

objects.h	bound gem color to type (Janet Walz)
o_init.c

spell.c		spell list now displayed in the corner (Bruce Mewborne)

apply.c		multiple dragon types. (Bruce Mewborne)
cmd.c		wand of lightning.
do.c		ring of shock resistance.
do_name.c	giant eel replaced by electric eel.
end.c		"STOOGES" - three stooges code.
engrave.c	Named dagger/short sword "Sting".
fight.c		Lamps & Magic Lamps.
makemon.c	A Badge - identifies you as a Kop.
config.h	^X option for wizard mode - gives current abilities.
mhitu.c		New monster djinni '&' for Magic lamps.
mklev.c		#rub command for lamps.
mkobj.c		New monster: Gremlin 'G' augments Gnome on lower levels.
mon.c		major modifications to buzz() code cleans it up.
monmove.c
monst.c		objnam.c	polyself.c	potion.c
pri.c		rip.c		shk.c		sit.c
u_init.c	wizard.c	zap.c		makedefs.c
monst.h		obj.h		objects.h	permonst.h
you.h		*main.c		*unix.c

rnd.c		fixed portability bug for 16 bit machines (Paul Eggert)

mhitu.c		fixed many lint flagged bugs (Izchak Miller)
apply.c
fight.c		mklev.c		mkmaze.c	mkshop.c
monmove.c	shknam.c	trap.c		wizard.c
zap.c		shk.c		do_name.c	invent.c
unixtty.c	pctty.c		unixmain.c	pcmain.c
do.c		options.c	termcap.c	makemon.c
spell.c

termcap.c	major rewrite *** NOTE: Untested by MRS *** (Kevin Sweet)

do_name.c	reversed quantity check for "n blessed +xx items"
		(Roland McGrath)

config.h	Added Kitchen Sink code (Janet Walz)
rm.h
decl.c		do.c		hack.c		invent.c
makedefs.c	mklev.c		options.c	pager.c
prisym.c	wizard.c	zap.c

end.c		Fixed "killed by" bugs in endgame code. (Steve Creps)

objects.h	Changed weight of a leash to a reasonable value. (Janet Walz)


