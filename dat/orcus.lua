-- NetHack gehennom orcus.lua	$NHDT-Date: 1652196033 2022/05/10 15:20:33 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.3 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1992 by M. Stephenson and Izchak Miller
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style="mazegrid", bg ="-" });

des.level_flags("mazelevel", "shortsighted")

local tmpbounds = selection.match("-");
local bnds = tmpbounds:bounds();
local bounds2 = selection.fillrect(bnds.lx, bnds.ly + 1, bnds.hx - 2, bnds.hy - 1);

-- A ghost town
local orcus1 = des.map({ halign = "right", valign = "center", map = [[
.|....|....|....|..............|....|........
.|....|....|....|..............|....|........
.|....|....|....|--...-+-------|.............
.|....|....|....|..............+.............
.|.........|....|..............|....|........
.--+-...-+----+--....-------...--------.-+---
.....................|.....|.................
.....................|.....|.................
.--+----....-+---....|.....|...----------+---
.|....|....|....|....---+---...|......|......
.|.........|....|..............|......|......
.----...---------.....-----....+......|......
.|........................|....|......|......
.----------+-...--+--|....|....----------+---
.|....|..............|....+....|.............
.|....+.......|......|....|....|.............
.|....|.......|......|....|....|.............
]], contents = function(rm)
   des.mazewalk(00,06,"west")
   -- Entire main area
   des.region(selection.area(01,00,44,16),"unlit")
   des.stair("down", 33,15)
   -- Wall "ruins"
   des.object("boulder",19,02)
   des.object("boulder",20,02)
   des.object("boulder",21,02)
   des.object("boulder",36,02)
   des.object("boulder",36,03)
   des.object("boulder",06,04)
   des.object("boulder",05,05)
   des.object("boulder",06,05)
   des.object("boulder",07,05)
   des.object("boulder",39,05)
   des.object("boulder",08,08)
   des.object("boulder",09,08)
   des.object("boulder",10,08)
   des.object("boulder",11,08)
   des.object("boulder",06,10)
   des.object("boulder",05,11)
   des.object("boulder",06,11)
   des.object("boulder",07,11)
   des.object("boulder",21,11)
   des.object("boulder",21,12)
   des.object("boulder",13,13)
   des.object("boulder",14,13)
   des.object("boulder",15,13)
   des.object("boulder",14,14)
   -- Doors
   des.door("closed",23,02)
   des.door("open",31,03)
   des.door("nodoor",03,05)
   des.door("closed",09,05)
   des.door("closed",14,05)
   des.door("closed",41,05)
   des.door("open",03,08)
   des.door("nodoor",13,08)
   des.door("open",41,08)
   des.door("closed",24,09)
   des.door("closed",31,11)
   des.door("open",11,13)
   des.door("closed",18,13)
   des.door("closed",41,13)
   des.door("open",26,14)
   des.door("closed",06,15)
   -- Special rooms
   des.altar({ x=24,y=07,align="noalign",type="sanctum" })
   des.region({ region={22,12,25,16},lit=0,type="morgue",filled=1 })
   des.region({ region={32,09,37,12},lit=1,type="shop",filled=1 })
   des.region({ region={12,00,15,04},lit=1,type="shop",filled=1 })
   -- Some traps.
   des.trap("spiked pit")
   des.trap("sleep gas")
   des.trap("anti magic")
   des.trap("fire")
   des.trap("fire")
   des.trap("fire")
   des.trap("magic")
   des.trap("magic")
   -- Some random objects
   des.object()
   des.object()
   des.object()
   des.object()
   des.object()
   des.object()
   des.object()
   des.object()
   des.object()
   des.object()
   -- The resident nasty
   des.monster("Orcus",33,15)
   -- And its preferred companions
   des.monster("human zombie",32,15)
   des.monster("shade",32,14)
   des.monster("shade",32,16)
   des.monster("vampire",35,16)
   des.monster("vampire",35,14)
   des.monster("vampire lord",36,14)
   des.monster("vampire lord",36,15)
   -- Randomly placed companions
   des.monster("skeleton")
   des.monster("skeleton")
   des.monster("skeleton")
   des.monster("skeleton")
   des.monster("skeleton")
   des.monster("shade")
   des.monster("shade")
   des.monster("shade")
   des.monster("shade")
   des.monster("giant zombie")
   des.monster("giant zombie")
   des.monster("giant zombie")
   des.monster("ettin zombie")
   des.monster("ettin zombie")
   des.monster("ettin zombie")
   des.monster("human zombie")
   des.monster("human zombie")
   des.monster("human zombie")
   des.monster("vampire")
   des.monster("vampire")
   des.monster("vampire")
   des.monster("vampire lord")
   des.monster("vampire lord")
   -- A few more for the party
   des.monster()
   des.monster()
   des.monster()
   des.monster()
   des.monster()
end });

des.levregion({ region={01,00,12,20}, region_islev=1, exclude={20,01,70,20}, exclude_islev=1, type="stair-up" });
des.levregion({ region={01,00,12,20}, region_islev=1, exclude={20,01,70,20}, exclude_islev=1, type="branch" });
des.teleport_region({ region={01,00,12,20}, region_islev=1, exclude={20,01,70,20}, exclude_islev=1 });

local protected = bounds2:negate() | orcus1;
hell_tweaks(protected);
