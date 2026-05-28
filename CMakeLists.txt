cmake_minimum_required(VERSION 3.15)
project(NetHack VERSION 5.0.0)

include("CMakeOptions.txt")

if(CMAKE_BUILD_TYPE)
  message("Build type: ${CMAKE_BUILD_TYPE}.")
endif()

message(STATUS "Building NetHack version: ${NetHack_VERSION}.")


include_directories(SYSTEM ${LUA_INCLUDE_DIR})
add_subdirectory(util)
add_subdirectory(dat)

set(NETHACK_SRC "src/alloc.c" "src/apply.c" "src/artifact.c" "src/attrib.c"
  "src/ball.c" "src/bones.c" "src/botl.c" "src/calendar.c" "src/cfgfiles.c"
  "src/cmd.c" "src/coloratt.c" "src/date.c" "src/dbridge.c" "src/decl.c"
  "src/detect.c" "src/dig.c" "src/display.c" "src/dlb.c" "src/do.c"
  "src/do_name.c" "src/do_wear.c" "src/dog.c" "src/dogmove.c" "src/dokick.c"
  "src/dothrow.c" "src/drawing.c" "src/dungeon.c" "src/earlyarg.c" "src/eat.c"
  "src/end.c" "src/engrave.c" "src/exper.c" "src/explode.c" "src/extralev.c"
  "src/files.c" "src/fountain.c" "src/getpos.c" "src/glyphs.c" "src/hack.c"
  "src/hacklib.c" "src/iactions.c" "src/insight.c" "src/invent.c"
  "src/isaac64.c" "src/light.c" "src/lock.c" "src/mail.c" "src/makemon.c"
  "src/mcastu.c" "src/mdlib.c" "src/mhitm.c" "src/mhitu.c" "src/minion.c"
  "src/mklev.c" "src/mkmap.c" "src/mkmaze.c" "src/mkobj.c" "src/mkroom.c"
  "src/mon.c" "src/mondata.c" "src/monmove.c" "src/monst.c" "src/mplayer.c"
  "src/mthrowu.c" "src/muse.c" "src/music.c" "src/nhlobj.c" "src/nhlsel.c"
  "src/nhlua.c" "src/nhmd4.c" "src/o_init.c" "src/objects.c" "src/objnam.c"
  "src/options.c" "src/pager.c" "src/pickup.c" "src/pline.c" "src/polyself.c"
  "src/potion.c" "src/pray.c" "src/priest.c" "src/quest.c" "src/questpgr.c"
  "src/read.c" "src/rect.c" "src/region.c" "src/restore.c" "src/revision.c"
  "src/rip.c" "src/rnd.c" "src/role.c" "src/rumors.c" "src/save.c"
  "src/sfbase.c" "src/sfstruct.c" "src/shk.c" "src/shknam.c" "src/sit.c"
  "src/sounds.c" "src/sp_lev.c" "src/spell.c" "src/steal.c" "src/steed.c"
  "src/symbols.c" "src/sys.c" "src/teleport.c" "src/timeout.c" "src/topten.c"
  "src/track.c" "src/trap.c" "src/u_init.c" "src/uhitm.c" "src/utf8map.c"
  "src/vault.c" "src/version.c" "src/vision.c" "src/weapon.c" "src/were.c"
  "src/wield.c" "src/windows.c" "src/wizard.c" "src/wizcmds.c" "src/worm.c"
  "src/worn.c" "src/write.c" "src/zap.c" "sys/share/ioctl.c"
  "sys/share/pcmain.c" "sys/share/pcsys.c" "sys/share/pctty.c"
  "sys/share/pcunix.c" "sys/share/pmatchregex.c" "sys/share/posixregex.c"
  "sys/share/random.c" "sys/share/tclib.c" "sys/share/unixtty.c"
  "sys/share/uudecode.c" "sys/unix/unixunix.c" "sys/unix/unixmain.c"
  "sys/unix/unixres.c" "win/tty/getline.c" "win/tty/termcap.c" "win/tty/topl.c"
  "win/tty/wintty.c"
)

