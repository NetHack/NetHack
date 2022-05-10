-- NetHack Barbarian Bar-strt.lua	$NHDT-Date: 1652196001 2022/05/10 15:20:01 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.4 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
--
--	The "start" level for the quest.
--
--	Here you meet your (besieged) class leader, Pelias,
--	and receive your quest assignment.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "noteleport", "hardfloor")

des.map([[
..................................PP........................................
...................................PP.......................................
...................................PP.......................................
....................................PP......................................
........--------------......-----....PPP....................................
........|...S........|......+...|...PPP.....................................
........|----........|......|...|....PP.....................................
........|.\..........+......-----...........................................
........|----........|...............PP.....................................
........|...S........|...-----.......PPP....................................
........--------------...+...|......PPPPP...................................
.........................|...|.......PPP....................................
...-----......-----......-----........PP....................................
...|...+......|...+..--+--.............PP...................................
...|...|......|...|..|...|..............PP..................................
...-----......-----..|...|.............PPPP.................................
.....................-----............PP..PP................................
.....................................PP...PP................................
....................................PP...PP.................................
....................................PP....PP................................
]]);

-- the forest beyond the river
des.replace_terrain({ region={37,00, 59,19}, fromterrain=".", toterrain="T", chance=5 })
des.replace_terrain({ region={60,00, 64,19}, fromterrain=".", toterrain="T", chance=10 })
des.replace_terrain({ region={65,00, 75,19}, fromterrain=".", toterrain="T", chance=20 })
-- guarantee a path and free spot for the portal
des.terrain(selection.randline(selection.new(), 37,7, 62,02, 7), ".")
des.terrain({62,02}, ".")

-- Dungeon Description
des.region(selection.area(00,00,75,19), "lit")
des.region(selection.area(09,05,11,05), "unlit")
des.region(selection.area(09,07,11,07), "lit")
des.region(selection.area(09,09,11,09), "unlit")
des.region(selection.area(13,05,20,09), "lit")
des.region(selection.area(29,05,31,06), "lit")
des.region(selection.area(26,10,28,11), "lit")
des.region(selection.area(04,13,06,14), "lit")
des.region(selection.area(15,13,17,14), "lit")
des.region(selection.area(22,14,24,15), "lit")
-- Stairs
des.stair("down", 09,09)
-- Portal arrival point
des.levregion({ region = {62,02,62,02}, type="branch" })
-- Doors
des.door("locked",12,05)
des.door("locked",12,09)
des.door("closed",21,07)
des.door("open",07,13)
des.door("open",18,13)
des.door("open",23,13)
des.door("open",25,10)
des.door("open",28,05)
-- Elder
des.monster({ id = "Pelias", coord = {10, 07}, inventory = function()
   des.object({ id = "runesword", spe = 5 });
   des.object({ id = "chain mail", spe = 5 });
end })
-- The treasure of Pelias
des.object("chest", 09, 05)
-- chieftain guards for the audience chamber
des.monster("chieftain", 10, 05)
des.monster("chieftain", 10, 09)
des.monster("chieftain", 11, 05)
des.monster("chieftain", 11, 09)
des.monster("chieftain", 14, 05)
des.monster("chieftain", 14, 09)
des.monster("chieftain", 16, 05)
des.monster("chieftain", 16, 09)
-- Non diggable walls
des.non_diggable(selection.area(00,00,75,19))
-- One trap to keep the ogres at bay.
des.trap("spiked pit",37,07)
-- Eels in the river
des.monster("giant eel", 36, 01)
des.monster("giant eel", 37, 09)
des.monster("giant eel", 39, 15)
-- Monsters on siege duty.
local ogrelocs = selection.floodfill(37,7) & selection.area(40,03, 45,20)
for i = 0, 11 do
   des.monster({ id = "ogre", coord = ogrelocs:rndcoord(1), peaceful = 0 })
end
