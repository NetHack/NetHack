add_library(hacklib hacklib.c)

# TODO cfgfiles.c
target_sources(NetHack PUBLIC allmain.c alloc.c apply.c artifact.c attrib.c
  ball.c bones.c botl.c calendar.c cmd.c cfgfiles.c coloratt.c dbridge.c
  decl.c detect.c dig.c display.c dlb.c do.c do_name.c do_wear.c dog.c
  dogmove.c dokick.c dothrow.c drawing.c dungeon.c earlyarg.c eat.c end.c
  engrave.c exper.c explode.c extralev.c files.c fountain.c getpos.c glyphs.c
  hack.c hacklib.c iactions.c insight.c invent.c isaac64.c light.c lock.c
  mail.c makemon.c mcastu.c mdlib.c mhitm.c mhitu.c minion.c mklev.c mkmap.c
  mkmaze.c mkobj.c mkroom.c mon.c mondata.c monmove.c monst.c mplayer.c
  mthrowu.c muse.c music.c nhlobj.c nhlsel.c nhlua.c nhmd4.c o_init.c objects.c
  objnam.c options.c pager.c pickup.c pline.c polyself.c potion.c pray.c
  priest.c quest.c questpgr.c read.c rect.c region.c report.c restore.c
  revision.c rip.c rnd.c role.c rumors.c save.c selvar.c sfbase.c sfstruct.c
  shk.c shknam.c sit.c sounds.c sp_lev.c spell.c stairs.c steal.c steed.c
  strutil.c symbols.c sys.c teleport.c timeout.c topten.c track.c trap.c
  u_init.c uhitm.c utf8map.c vault.c version.c vision.c weapon.c were.c wield.c
  windows.c wizard.c wizcmds.c worm.c worn.c write.c zap.c date.c)


target_link_libraries(NetHack hacklib)
