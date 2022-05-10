-- NetHack Monk Mon-strt.lua	$NHDT-Date: 1652196007 2022/05/10 15:20:07 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.4 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991-2 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
--
--	The "start" level for the quest.
--
--	Here you meet your (besieged) class leader, the Grand Master
--	and receive your quest assignment.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "noteleport", "hardfloor")

des.map([[
............................................................................
............................................................................
............................................................................
....................------------------------------------....................
....................|................|.....|.....|.....|....................
....................|..------------..|--+-----+-----+--|....................
....................|..|..........|..|.................|....................
....................|..|..........|..|+---+---+-----+--|....................
..................---..|..........|......|...|...|.....|....................
..................+....|..........+......|...|...|.....|....................
..................+....|..........+......|...|...|.....|....................
..................---..|..........|......|...|...|.....|....................
....................|..|..........|..|+-----+---+---+--|....................
....................|..|..........|..|.................|....................
....................|..------------..|--+-----+-----+--|....................
....................|................|.....|.....|.....|....................
....................------------------------------------....................
............................................................................
............................................................................
............................................................................
]]);
-- Dungeon Description
des.region(selection.area(00,00,75,19), "lit")
des.region({ region={24,06, 33,13}, lit=1, type="temple" })

des.replace_terrain({ region={00,00, 10,19}, fromterrain=".", toterrain="T", chance=10 })
des.replace_terrain({ region={65,00, 75,19}, fromterrain=".", toterrain="T", chance=10 })

local spacelocs = selection.floodfill(05,04);

-- Portal arrival point
des.terrain({05,04}, ".")
des.levregion({ region = {05,04,05,04}, type="branch" })
-- Stairs
des.stair("down", 52,09)
-- Doors
des.door("locked",18,09)
des.door("locked",18,10)
des.door("closed",34,09)
des.door("closed",34,10)
des.door("closed",40,05)
des.door("closed",46,05)
des.door("closed",52,05)
des.door("locked",38,07)
des.door("closed",42,07)
des.door("closed",46,07)
des.door("closed",52,07)
des.door("locked",38,12)
des.door("closed",44,12)
des.door("closed",48,12)
des.door("closed",52,12)
des.door("closed",40,14)
des.door("closed",46,14)
des.door("closed",52,14)
-- Unattended Altar - unaligned due to conflict - player must align it.
des.altar({ x=28,y=09, align="noalign", type="altar" })
-- The Grand Master
des.monster({ id = "Grand Master", coord = {28, 10}, inventory = function()
   des.object({ id = "robe", spe = 6 });
end })
-- No treasure chest!
-- guards for the audience chamber
des.monster("abbot", 32, 07)
des.monster("abbot", 32, 08)
des.monster("abbot", 32, 11)
des.monster("abbot", 32, 12)
des.monster("abbot", 33, 07)
des.monster("abbot", 33, 08)
des.monster("abbot", 33, 11)
des.monster("abbot", 33, 12)
-- Non diggable walls
des.non_diggable(selection.area(18,03,55,16))
-- Random traps
for i = 1, 2 do
   des.trap("dart", spacelocs:rndcoord(1))
end
des.trap()
des.trap()
des.trap()
des.trap()
-- Monsters on siege duty.
for i = 1, 8 do
   des.monster("earth elemental", spacelocs:rndcoord(1))
end
for i = 1, 4 do
   des.monster("xorn", spacelocs:rndcoord(1))
end
