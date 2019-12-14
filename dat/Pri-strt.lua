-- NetHack 3.6	Priest.des	$NHDT-Date: 1432512784 2015/05/25 00:13:04 $  $NHDT-Branch: master $:$NHDT-Revision: 1.11 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991-2 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
--
--	The "start" level for the quest.
--
--	Here you meet your (besieged) class leader, High Priest
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
des.terrain({05,04}, ".")

-- Portal arrival point
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
des.altar({ x=28, y=09, align="noalign", type="altar" })
-- High Priest
des.monster("Arch Priest", 28, 10)
-- The treasure of High Priest
des.object("chest", 27, 10)
-- knight guards for the audience chamber
des.monster("acolyte", 32, 07)
des.monster("acolyte", 32, 08)
des.monster("acolyte", 32, 11)
des.monster("acolyte", 32, 12)
des.monster("acolyte", 33, 07)
des.monster("acolyte", 33, 08)
des.monster("acolyte", 33, 11)
des.monster("acolyte", 33, 12)
-- Non diggable walls
des.non_diggable(selection.area(18,03,55,16))
-- Random traps
des.trap("dart",20,09)
des.trap("dart",20,10)
des.trap()
des.trap()
des.trap()
des.trap()
-- Monsters on siege duty.
des.monster("human zombie", 37, 01)
des.monster("human zombie", 37, 18)
des.monster("human zombie", 03, 03)
des.monster("human zombie", 65, 04)
des.monster("human zombie", 12, 11)
des.monster("human zombie", 60, 12)
des.monster("human zombie", 14, 08)
des.monster("human zombie", 55, 00)
des.monster("human zombie", 18, 18)
des.monster("human zombie", 59, 10)
des.monster("human zombie", 13, 09)
des.monster("human zombie", 01, 17)
