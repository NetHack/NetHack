#! /bin/sh
# package up all the files in an official NetHack source distribution (for
# all supported ports) into a number of foo.tzu files (archive names
# representable on all systems that may want to handle them), which should
# all be under 60K and contain only ASCII characters so they can be safely
# mailed 

# a few large files get individual handling since they can't be combined with
# others in a 60K package

# this script should be run from the top directory of a distribution, and
# all the packed files will appear in the subdirectory PACKDIR

# to unpack foo1.tzu by hand,
#	move foo1.tzu into the appropriate directory
#	uudecode foo1.tzu
#	mv foo1.taz foo1.tar.Z
#	uncompress foo1.tar
#	tar xvf foo1.tar
#	rm foo1.tzu foo1.tar

douu=0

COMPRESS=compress
TZINT=taz
TZEXT=tar.Z
TZUEXT=tzu

while [ $# -gt 0 ] ; do
    case X$1 in
	X-x)	echo "usage $0 [-u|-gz|-x]"
		echo "-u    uuencode output."
		echo "-gz   use gzip as compress utility (default, compress)."
		echo "-x    display this text."
		exit ;;
	X-u)	douu=1 ;;
	X-gz)	COMPRESS=gzip
		TZINT=tgz
		TZEXT=tar.gz
		TZUEXT=tgu ;;
	X*)	echo "usage $0 [-u|-gz|-x]" ; exit ;;
    esac
    shift
done

PACKDIR=packages

if [ ! -d ${PACKDIR} ]
then
	mkdir ${PACKDIR}
fi

# wrap up files from top directory
TOP="Files Porting README"
tar cvf top.tar $TOP
for i in top
do
	$COMPRESS $i.tar
	if [ $douu -gt 0 ]
	then
		uuencode $i.${TZEXT} $i.${TZINT} >${PACKDIR}/$i.${TZUEXT}
		rm $i.${TZEXT}
	else
		mv $i.${TZEXT} ${PACKDIR}/$i.${TZINT}
	fi
done

# wrap up data files
DAT1="cmdhelp dungeon.def help hh history license opthelp wizhelp rumors.fal rumors.tru"
DAT2="Arch.des Barb.des Caveman.des Elf.des Healer.des Knight.des Priest.des Rogue.des Samurai.des Tourist.des Valkyrie.des Wizard.des"
DAT3="bigroom.des castle.des endgame.des gehennom.des knox.des medusa.des mines.des oracle.des tower.des yendor.des"
DAT4="oracles.txt quest.txt"
DAT5="data.base"
( cd dat ; tar cvf ../dat1.tar $DAT1 ; tar cvf ../dat2.tar $DAT2 )
( cd dat ; tar cvf ../dat3.tar $DAT3 ; tar cvf ../dat4.tar $DAT4 )
( cd dat ; tar cvf ../dat5.tar $DAT5 )
for i in dat1 dat2 dat3 dat4 dat5
do
	$COMPRESS $i.tar
	if [ $douu -gt 0 ]
	then
		uuencode $i.${TZEXT} $i.${TZINT} >${PACKDIR}/$i.${TZUEXT}
		rm $i.${TZEXT}
	else
		mv $i.${TZEXT} ${PACKDIR}/$i.${TZINT}
	fi
done

#wrap up doc files
DOC1="Guidebook.txt"
DOC2="Guidebook.mn"
DOC3="Guidebook.tex"
DOC4="dlb.6 dlb.txt dgn_comp.6 dgn_comp.txt lev_comp.6 lev_comp.txt nethack.6 nethack.txt recover.6 recover.txt"
DOC5="tmac.n window.doc"
( cd doc ; tar cvf ../doc1.tar $DOC1 ; tar cvf ../doc2.tar $DOC2 )
( cd doc ; tar cvf ../doc3.tar $DOC3 ; tar cvf ../doc4.tar $DOC4 )
( cd doc ; tar cvf ../doc5.tar $DOC5 )
for i in doc1 doc2 doc3 doc4 doc5
do
	$COMPRESS $i.tar
	if [ $douu -gt 0 ]
	then
		uuencode $i.${TZEXT} $i.${TZINT} >${PACKDIR}/$i.${TZUEXT}
		rm $i.${TZEXT}
	else
		mv $i.${TZEXT} ${PACKDIR}/$i.${TZINT}
	fi
done

# wrap up include files
INCL1="align.h amiconf.h artifact.h artilist.h attrib.h beconf.h color.h config.h coord.h decl.h def_os2.h dgn_file.h display.h dlb.h dungeon.h edog.h emin.h engrave.h epri.h"
INCL2="eshk.h flag.h func_tab.h global.h extern.h hack.h lev.h macwin.h macconf.h"
INCL3="mactty.h mail.h mfndpos.h micro.h mkroom.h monattk.h mondata.h monflag.h monst.h monsym.h mttypriv.h nhlan.h ntconf.h obj.h objclass.h os2conf.h patchlevel.h"
INCL4="pcconf.h permonst.h prop.h qtext.h quest.h rect.h rm.h sp_lev.h spell.h system.h tcap.h tile2x11.h timeout.h tosconf.h tradstdc.h trampoli.h"
INCL5="trap.h unixconf.h vault.h vision.h vmsconf.h winX.h winami.h winprocs.h wintty.h wintype.h xwindow.h xwindowp.h you.h youprop.h"
( cd include ; tar cvf ../incl1.tar $INCL1 ; tar cvf ../incl2.tar $INCL2 )
( cd include ; tar cvf ../incl3.tar $INCL3 ; tar cvf ../incl4.tar $INCL4 )
( cd include ; tar cvf ../incl5.tar $INCL5 )
for i in incl1 incl2 incl3 incl4 incl5
do
	$COMPRESS $i.tar
	if [ $douu -gt 0 ]
	then
		uuencode $i.${TZEXT} $i.${TZINT} >${PACKDIR}/$i.${TZUEXT}
		rm $i.${TZEXT}
	else
		mv $i.${TZEXT} ${PACKDIR}/$i.${TZINT}
	fi
done

# wrap up source files
SRC01="allmain.c alloc.c apply.c bones.c"
SRC02="artifact.c attrib.c ball.c botl.c"
SRC03="cmd.c dbridge.c decl.c dlb.c"
SRC04="detect.c display.c"
SRC05="dig.c dog.c do_name.c"
SRC06="do_wear.c do.c"
SRC07="dogmove.c dokick.c drawing.c"
SRC08="dothrow.c dungeon.c extralev.c"
SRC09="eat.c end.c exper.c"
SRC10="engrave.c explode.c files.c"
SRC11="fountain.c hack.c hacklib.c light.c"
SRC12="invent.c lock.c"
SRC13="mail.c mhitm.c mkmaze.c"
SRC14="mcastu.c mhitu.c minion.c mkmap.c"
SRC15="makemon.c mkobj.c"
SRC16="monst.c o_init.c"
SRC17="mkroom.c mon.c mplayer.c"
SRC18="monmove.c pray.c"
SRC19="music.c options.c"
SRC20="mondata.c mthrowu.c muse.c"
SRC21="objnam.c polyself.c"
SRC22="mklev.c objects.c"
SRC23="pager.c pickup.c pline.c"
SRC24="potion.c priest.c quest.c questpgr.c rect.c"
SRC25="read.c restore.c rip.c rnd.c rumors.c"
SRC26="shk.c"
SRC27="save.c shknam.c sit.c sounds.c steal.c"
SRC28="sp_lev.c spell.c"
SRC29="teleport.c timeout.c track.c"
SRC30="trap.c version.c"
SRC31="topten.c uhitm.c write.c"
SRC32="vault.c wield.c windows.c wizard.c"
SRC33="u_init.c weapon.c were.c worm.c"
SRC34="vision.c worn.c"
SRC35="zap.c"
( cd src ; tar cvf ../src01.tar $SRC01 ; tar cvf ../src02.tar $SRC02 )
( cd src ; tar cvf ../src03.tar $SRC03 ; tar cvf ../src04.tar $SRC04 )
( cd src ; tar cvf ../src05.tar $SRC05 ; tar cvf ../src06.tar $SRC06 )
( cd src ; tar cvf ../src07.tar $SRC07 ; tar cvf ../src08.tar $SRC08 )
( cd src ; tar cvf ../src09.tar $SRC09 ; tar cvf ../src10.tar $SRC10 )
( cd src ; tar cvf ../src11.tar $SRC11 ; tar cvf ../src12.tar $SRC12 )
( cd src ; tar cvf ../src13.tar $SRC13 ; tar cvf ../src14.tar $SRC14 )
( cd src ; tar cvf ../src15.tar $SRC15 ; tar cvf ../src16.tar $SRC16 )
( cd src ; tar cvf ../src17.tar $SRC17 ; tar cvf ../src18.tar $SRC18 )
( cd src ; tar cvf ../src19.tar $SRC19 ; tar cvf ../src20.tar $SRC20 )
( cd src ; tar cvf ../src21.tar $SRC21 ; tar cvf ../src22.tar $SRC22 )
( cd src ; tar cvf ../src23.tar $SRC23 ; tar cvf ../src24.tar $SRC24 )
( cd src ; tar cvf ../src25.tar $SRC25 ; tar cvf ../src26.tar $SRC26 )
( cd src ; tar cvf ../src27.tar $SRC27 ; tar cvf ../src28.tar $SRC28 )
( cd src ; tar cvf ../src29.tar $SRC29 ; tar cvf ../src30.tar $SRC30 )
( cd src ; tar cvf ../src31.tar $SRC31 ; tar cvf ../src32.tar $SRC32 )
( cd src ; tar cvf ../src33.tar $SRC33 ; tar cvf ../src34.tar $SRC34 )
( cd src ; tar cvf ../src35.tar $SRC35 )
for i in 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35
do
	$COMPRESS src$i.tar
	if [ $douu -gt 0 ]
	then
		uuencode src$i.${TZEXT} src$i.${TZINT} >${PACKDIR}/src$i.${TZUEXT}
		rm src$i.${TZEXT}
	else
		mv src$i.${TZEXT} ${PACKDIR}/src$i.${TZINT}
	fi
done

# wrap up utility source files
UTIL1="lev_comp.l lev_comp.y lev_main.c dlb_main.c"
UTIL2="dgn_comp.l dgn_comp.y dgn_main.c makedefs.c panic.c recover.c"
( cd util ; tar cvf ../util1.tar $UTIL1 ; tar cvf ../util2.tar $UTIL2 )
for i in util1 util2
do
	$COMPRESS $i.tar
	if [ $douu -gt 0 ]
	then
		uuencode $i.${TZEXT} $i.${TZINT} >${PACKDIR}/$i.${TZUEXT}
		rm $i.${TZEXT}
	else
		mv $i.${TZEXT} ${PACKDIR}/$i.${TZINT}
	fi
done

# wrap up Amiga files
AMIGA1="Build.ami Install.ami Makefile.ami NetHack.cnf mkdmake amii.hlp hackwb.hlp ifchange"
AMIGA2="HackWB.uu NHinfo.uu NewGame.uu amifont.uu amifont8.uu ask.uu charwin.uu clipwin.uu colors.uu dflticon.uu randwin.uu scroll.uu string.uu wbwin.uu wbdefs.h wbprotos.h wbstruct.h windefs.h winext.h winproto.h"
AMIGA3="amidos.c amilib.c amigst.c amimenu.c amirip.c amisnd.c amitty.c amiwind.c clipwin.c"
AMIGA4="amiwbench.c winchar.c winkey.c winreq.c winstr.c"
AMIGA5="amidos.p amiwind.p winami.p dispmap.s colorwin.c winfuncs.c"
AMIGA6="wbcli.c wbgads.c wbwin.c"
AMIGA7="wb.c"
AMIGA8="grave16.xpm winmenu.c"
AMIGA9="char.c cvtsnd.c randwin.c wbdata.c winami.c txt2iff.c xpm2iff.c"
AMIGA10="amiout.h arg.c arg.h loader.c multi.c multi.h split.doc split.h splitter.c"
( cd sys/amiga ; tar cvf ../../amiga1.tar $AMIGA1 )
( cd sys/amiga ; tar cvf ../../amiga2.tar $AMIGA2 )
( cd sys/amiga ; tar cvf ../../amiga3.tar $AMIGA3 )
( cd sys/amiga ; tar cvf ../../amiga4.tar $AMIGA4 )
( cd sys/amiga ; tar cvf ../../amiga5.tar $AMIGA5 )
( cd sys/amiga ; tar cvf ../../amiga6.tar $AMIGA6 )
( cd sys/amiga ; tar cvf ../../amiga7.tar $AMIGA7 )
( cd sys/amiga ; tar cvf ../../amiga8.tar $AMIGA8 )
( cd sys/amiga ; tar cvf ../../amiga9.tar $AMIGA9 )
( cd sys/amiga/splitter ; tar cvf ../../../ami_spl.tar $AMIGA10 )
for i in amiga1 amiga2 amiga3 amiga4 amiga5 amiga6 amiga7 amiga8 amiga9 ami_spl
do
	$COMPRESS $i.tar
	if [ $douu -gt 0 ]
	then
		uuencode $i.${TZEXT} $i.${TZINT} >${PACKDIR}/$i.${TZUEXT}
		rm $i.${TZEXT}
	else
		mv $i.${TZEXT} ${PACKDIR}/$i.${TZINT}
	fi
done

# wrap up Atari files
ATARI="Install.tos Makefile.utl atari.cnf atarifnt.uue nethack.mnu setup.g tos.c"
( cd sys/atari ; tar cvf ../../atari.tar $ATARI )
for i in atari
do
	$COMPRESS $i.tar
	if [ $douu -gt 0 ]
	then
		uuencode $i.${TZEXT} $i.${TZINT} >${PACKDIR}/$i.${TZUEXT}
		rm $i.${TZEXT}
	else
		mv $i.${TZEXT} ${PACKDIR}/$i.${TZINT}
	fi
done

# wrap up BeBox files
BE="README bemain.c"
( cd sys/be ; tar cvf ../../be.tar $BE )
for i in be
do
	$COMPRESS $i.tar
	if [ $douu -gt 0 ]
	then
		uuencode $i.${TZEXT} $i.${TZINT} >${PACKDIR}/$i.${TZUEXT}
		rm $i.${TZEXT}
	else
		mv $i.${TZEXT} ${PACKDIR}/$i.${TZINT}
	fi
done

# wrap up Mac files
MAC1="Files.r Install.mw MacHelp NHDeflts NHrsrc.hqx News README macmain.c"
MAC2="dprintf.c maccurs.c macerrs.c macfile.c macsnd.c mactopl.c mactty.c macunix.c"
MAC3="macwin.c mgetline.c mmodal.c mstring.c"
MAC4="macmenu.c mttymain.c mrecover.c mrecover.hqx"
MACOLD1="Install.mpw Install.thk DCproj.hqx LCproj.hqx"
MACOLD2="MDproj.hqx NHmake.hqx NetHack.r mhdump.c mpwhack.h"
( cd sys/mac ; tar cvf ../../mac1.tar $MAC1 ; tar cvf ../../mac2.tar $MAC2 )
( cd sys/mac ; tar cvf ../../mac3.tar $MAC3 ; tar cvf ../../mac4.tar $MAC4 )
( cd sys/mac/old ; tar cvf ../../../macold1.tar $MACOLD1 )
( cd sys/mac/old ; tar cvf ../../../macold2.tar $MACOLD2 )
for i in mac1 mac2 mac3 mac4 macold1 macold2
do
	$COMPRESS $i.tar
	if [ $douu -gt 0 ]
	then
		uuencode $i.${TZEXT} $i.${TZINT} >${PACKDIR}/$i.${TZUEXT}
		rm $i.${TZEXT}
	else
		mv $i.${TZEXT} ${PACKDIR}/$i.${TZINT}
	fi
done
cp sys/mac/old/NHproj.hqx ${PACKDIR}/mac-proj.hqx
cp sys/mac/NHsound.hqx ${PACKDIR}/mac-snd.hqx

#wrap up MSDOS files
MS1="Install.dos Makefile.BC schema1.BC schema2.BC pckeys.c"
MS2="Makefile.MSC setup.bat pctiles.h pcvideo.h portio.h nhico.uu nhpif.uu ovlinit.c moveinit.pat"
MS3="msdoshlp.txt msdos.c Makefile.GCC Makefile.SC schema1.MSC schema2.MSC schema3.MSC"
MS4="pctiles.c sound.c tile2bin.c video.c vidtxt.c vidvga.c"
MSOLD1="README.old exesmurf.c exesmurf.doc maintovl.doc schema.old trampoli.c"
MSOLD2="ovlmgr.asm ovlmgr.doc ovlmgr.uu"
MSOLD3="MakeMSC.src MakeMSC.utl Makefile.dat"
( cd sys/msdos ; tar cvf ../../msdos1.tar $MS1 ;  tar cvf ../../msdos2.tar $MS2 )
( cd sys/msdos ; tar cvf ../../msdos3.tar $MS3 ;  tar cvf ../../msdos4.tar $MS4 )
( cd sys/msdos/old ; tar cvf ../../../msold1.tar $MSOLD1 )
( cd sys/msdos/old ; tar cvf ../../../msold2.tar $MSOLD2 )
( cd sys/msdos/old ; tar cvf ../../../msold3.tar $MSOLD3 )
for i in msdos1 msdos2 msdos3 msdos4 msold1 msold2 msold3
do
	$COMPRESS $i.tar
	if [ $douu -gt 0 ]
	then
		uuencode $i.${TZEXT} $i.${TZINT} >${PACKDIR}/$i.${TZUEXT}
		rm $i.${TZEXT}
	else
		mv $i.${TZEXT} ${PACKDIR}/$i.${TZINT}
	fi
done

#wrap up OS/2 files
OS2="Install.os2 Makefile.os2 nhpmico.uu os2.c"
( cd sys/os2 ; tar cvf ../../os2.tar $OS2 )
for i in os2
do
	$COMPRESS $i.tar
	if [ $douu -gt 0 ]
	then
		uuencode $i.${TZEXT} $i.${TZINT} >${PACKDIR}/$i.${TZUEXT}
		rm $i.${TZEXT}
	else
		mv $i.${TZEXT} ${PACKDIR}/$i.${TZINT}
	fi
done

#wrap up shared system files
SSHR1="Makefile.lib NetHack.cnf termcap ioctl.c nhlan.c pcmain.c pcsys.c pctty.c pcunix.c random.c unixtty.c"
SSHR2="tclib.c dgn_yacc.c lev_lex.c"
SSHR3="dgn_comp.h dgn_lex.c lev_comp.h lev_yacc.c"
( cd sys/share ; tar cvf ../../shr_sys1.tar $SSHR1 )
( cd sys/share ; tar cvf ../../shr_sys2.tar $SSHR2 )
( cd sys/share ; tar cvf ../../shr_sys3.tar $SSHR3 )
for i in shr_sys1 shr_sys2 shr_sys3
do
	$COMPRESS $i.tar
	if [ $douu -gt 0 ]
	then
		uuencode $i.${TZEXT} $i.${TZINT} >${PACKDIR}/$i.${TZUEXT}
		rm $i.${TZEXT}
	else
		mv $i.${TZEXT} ${PACKDIR}/$i.${TZINT}
	fi
done
cp sys/share/termcap.uu ${PACKDIR}/shr_tc.uu

#wrap up shared sound files
SND1="README bell.uu firehorn.uu"
SND2="erthdrum.uu frsthorn.uu"
SND3="bugle.uu lethdrum.uu"
SND4="mgcflute.uu mgcharp.uu"
SND5="toolhorn.uu wdnflute.uu wdnharp.uu"
( cd sys/share/sounds ; tar cvf ../../../sound1.tar $SND1 )
( cd sys/share/sounds ; tar cvf ../../../sound2.tar $SND2 )
( cd sys/share/sounds ; tar cvf ../../../sound3.tar $SND3 )
( cd sys/share/sounds ; tar cvf ../../../sound4.tar $SND4 )
( cd sys/share/sounds ; tar cvf ../../../sound5.tar $SND5 )
for i in sound1 sound2 sound3 sound4 sound5
do
	$COMPRESS $i.tar
	if [ $douu -gt 0 ]
	then
		uuencode $i.${TZEXT} $i.${TZINT} >${PACKDIR}/$i.${TZUEXT}
		rm $i.${TZEXT}
	else
		mv $i.${TZEXT} ${PACKDIR}/$i.${TZINT}
	fi
done


#wrap up UNIX files
UNX1="Install.unx Makefile.dat Makefile.doc Makefile.src Makefile.top Makefile.utl depend.awk nethack.sh setup.sh"
UNX2="unixmain.c unixunix.c snd86unx.shr"
( cd sys/unix ; tar cvf ../../unix1.tar $UNX1 ; tar cvf ../../unix2.tar $UNX2 )
for i in unix1 unix2
do
	$COMPRESS $i.tar
	if [ $douu -gt 0 ]
	then
		uuencode $i.${TZEXT} $i.${TZINT} >${PACKDIR}/$i.${TZUEXT}
		rm $i.${TZEXT}
	else
		mv $i.${TZEXT} ${PACKDIR}/$i.${TZINT}
	fi
done
for i in 1 2 3
do
	cp sys/unix/cpp$i.shr ${PACKDIR}/cpp$i.shr
done

# wrap up VMS files
VMS1="Install.vms Makefile.dat Makefile.doc Makefile.src Makefile.top Makefile.utl"
VMS2="install.com nethack.com spec_lev.com vmsbuild.com lev_lex.h"
VMS3="oldcrtl.c vmsfiles.c vmsmail.c vmsmain.c vmsmisc.c vmstty.c vmsunix.c"
( cd sys/vms ; tar cvf ../../vms1.tar $VMS1 ; tar cvf ../../vms2.tar $VMS2 )
( cd sys/vms ; tar cvf ../../vms3.tar $VMS3 )
for i in vms1 vms2 vms3
do
	$COMPRESS $i.tar
	if [ $douu -gt 0 ]
	then
		uuencode $i.${TZEXT} $i.${TZINT} >${PACKDIR}/$i.${TZUEXT}
		rm $i.${TZEXT}
	else
		mv $i.${TZEXT} ${PACKDIR}/$i.${TZINT}
	fi
done

# wrap up NT files
NT1="Install.nt Makefile.nt mapimail.c nethack.def nhico.uu nhsetup.bat ntsound.c nttty.c win32api.h winnt.c winnt.cnf"
( cd sys/winnt ; tar cvf ../../nt_sys.tar $NT1 )
for i in nt_sys
do
	$COMPRESS $i.tar
	if [ $douu -gt 0 ]
	then
		uuencode $i.${TZEXT} $i.${TZINT} >${PACKDIR}/$i.${TZUEXT}
		rm $i.${TZEXT}
	else
		mv $i.${TZEXT} ${PACKDIR}/$i.${TZINT}
	fi
done

#wrap up shared window files
WSH1="tile.doc tile.h gifread.c ppmwrite.c thintile.c tilemap.c other.txt"
WSH2="monsters.txt"
WSH3="tiletext.c objects.txt"
WSH4="mthread.h nhprocs.c"
( cd win/share ; tar cvf ../../shr_win1.tar $WSH1 )
( cd win/share ; tar cvf ../../shr_win2.tar $WSH2 )
( cd win/share ; tar cvf ../../shr_win3.tar $WSH3 )
( cd win/share ; tar cvf ../../shr_win4.tar $WSH4 )
for i in shr_win1 shr_win2 shr_win3 shr_win4
do
	$COMPRESS $i.tar
	if [ $douu -gt 0 ]
	then
		uuencode $i.${TZEXT} $i.${TZINT} >${PACKDIR}/$i.${TZUEXT}
		rm $i.${TZEXT}
	else
		mv $i.${TZEXT} ${PACKDIR}/$i.${TZINT}
	fi
done

# wrap up tty files
TTY1="getline.c termcap.c topl.c"
TTY2="wintty.c"
( cd win/tty ; tar cvf ../../tty1.tar $TTY1 ; tar cvf ../../tty2.tar $TTY2 )
for i in tty1 tty2
do
	$COMPRESS $i.tar
	if [ $douu -gt 0 ]
	then
		uuencode $i.${TZEXT} $i.${TZINT} >${PACKDIR}/$i.${TZUEXT}
		rm $i.${TZEXT}
	else
		mv $i.${TZEXT} ${PACKDIR}/$i.${TZINT}
	fi
done

# wrap up NT Win32 files
NTW1="mtprocs.c nhwin32.h nhwin32.rc nhwin32x.h tile2bmp.c win32msg.c winmain.c"
( cd win/win32 ; tar cvf ../../nt_win.tar $NTW1 )
for i in nt_win
do
	$COMPRESS $i.tar
	if [ $douu -gt 0 ]
	then
		uuencode $i.${TZEXT} $i.${TZINT} >${PACKDIR}/$i.${TZUEXT}
		rm $i.${TZEXT}
	else
		mv $i.${TZEXT} ${PACKDIR}/$i.${TZINT}
	fi
done

# wrap up X files
X1="Install.X11 NetHack.ad ibm.bdf nethack.rc nh10.bdf nh32icon nh56icon nh72icon nh_icon.xpm pet_mark.xbm tile2x11.c"
X2="Window.c dialogs.c winmesg.c winmisc.c wintext.c"
X3="winmap.c winmenu.c winval.c"
X4="winstat.c winX.c"
X5="rip.xpm"
( cd win/X11 ; tar cvf ../../x11-1.tar $X1 ; tar cvf ../../x11-2.tar $X2 )
( cd win/X11 ; tar cvf ../../x11-3.tar $X3 ; tar cvf ../../x11-4.tar $X4 )
( cd win/X11 ; tar cvf ../../x11-5.tar $X5 )
for i in x11-1 x11-2 x11-3 x11-4 x11-5
do
	$COMPRESS $i.tar
	if [ $douu -gt 0 ]
	then
		uuencode $i.${TZEXT} $i.${TZINT} >${PACKDIR}/$i.${TZUEXT}
		rm $i.${TZEXT}
	else
		mv $i.${TZEXT} ${PACKDIR}/$i.${TZINT}
	fi
done
