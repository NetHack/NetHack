-- NetHack 3.6	yendor.des	$NHDT-Date: 1432512783 2015/05/25 00:13:03 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1992 by M. Stephenson and Izchak Miller
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style="mazegrid", bg ="-" });

des.level_flags("mazelevel", "noteleport", "hardfloor")
des.map([[
----------------------------.
|.....|.S....|.............|.
|.....|.-------S--------S--|.
|.....|.|.........|........|.
|..-S--S|.........|........|.
|..|....|.........|------S-|.
|..|....|.........|.....|..|.
|-S-----|.........|.....|..|.
|.......|.........|S--S--..|.
|.......|.........|.|......|.
|-----S----S-------.|......|.
|............|....S.|......|.
----------------------------.
]]);
des.levregion({ type="stair-up", region={01,00,79,20}, region_islev=1, exclude={0,0,28,12} })
des.levregion({ type="stair-down", region={01,00,79,20}, region_islev=1, exclude={0,0,28,12} })
des.levregion({ type="branch", region={01,00,79,20}, region_islev=1, exclude={0,0,28,12} })
des.teleport_region({ region={01,00,79,20}, region_islev=1, exclude={0,0,27,12} })
-- entire tower in a region, constrains monster migration
des.region({ region={01,01, 26,11}, lit=0, type="ordinary", prefilled=1 })
des.region({ region={09,03, 17,09}, lit=0, type="zoo", prefilled=0 })
des.door("closed",15,02)
des.door("closed",11,10)
des.mazewalk(28,05,"east")
des.ladder("up", 12,01)
des.ladder("down", 14,11)
-- Non diggable walls everywhere
des.non_diggable(selection.area(00,00,27,12))
--
des.non_passwall(selection.area(00,00,06,12))
des.non_passwall(selection.area(06,00,27,02))
des.non_passwall(selection.area(16,02,27,12))
des.non_passwall(selection.area(06,12,16,12))
-- Random traps.
des.trap("spiked pit")
des.trap("sleep gas")
des.trap("anti magic")
des.trap("magic")
-- Some random loot.
des.object("!")
des.object("!")
des.object("?")
des.object("?")
des.object("+")
-- treasures
des.object("\"", 04, 06)
