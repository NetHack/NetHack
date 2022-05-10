-- NetHack Archeologist Arc-strt.lua	$NHDT-Date: 1652195999 2022/05/10 15:19:59 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.2 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
--
--	The "start" level for the quest.
--
--	Here you meet your (besieged) class leader, Lord Carnarvon
--	and receive your quest assignment.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "noteleport", "hardfloor")

des.map([[
............................................................................
............................................................................
............................................................................
............................................................................
....................}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}.................
....................}-------------------------------------}.................
....................}|..S......+.................+.......|}.................
....................}-S---------------+----------|.......|}.................
....................}|.|...............|.......+.|.......|}.................
....................}|.|...............---------.---------}.................
....................}|.S.\.............+.................+..................
....................}|.|...............---------.---------}.................
....................}|.|...............|.......+.|.......|}.................
....................}-S---------------+----------|.......|}.................
....................}|..S......+.................+.......|}.................
....................}-------------------------------------}.................
....................}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}.................
............................................................................
............................................................................
............................................................................
]]);
-- Dungeon Description
des.region(selection.area(00,00,75,19), "lit")
des.region(selection.area(22,06,23,06), "unlit")
des.region(selection.area(25,06,30,06), "unlit")
des.region(selection.area(32,06,48,06), "unlit")
des.region(selection.area(50,06,56,08), "lit")
des.region(selection.area(40,08,46,08), "unlit")
des.region(selection.area(22,08,22,12), "unlit")
des.region(selection.area(24,08,38,12), "unlit")
des.region(selection.area(48,08,48,08), "lit")
des.region(selection.area(40,10,56,10), "lit")
des.region(selection.area(48,12,48,12), "lit")
des.region(selection.area(40,12,46,12), "unlit")
des.region(selection.area(50,12,56,14), "lit")
des.region(selection.area(22,14,23,14), "unlit")
des.region(selection.area(25,14,30,14), "unlit")
des.region(selection.area(32,14,48,14), "unlit")
-- Stairs
des.stair("down", 55,07)
-- Portal arrival point
des.levregion({ region = {63,06,63,06}, type="branch" })
-- Doors
des.door("closed",22,07)
des.door("closed",38,07)
des.door("locked",47,08)
des.door("locked",23,10)
des.door("locked",39,10)
des.door("locked",57,10)
des.door("locked",47,12)
des.door("closed",22,13)
des.door("closed",38,13)
des.door("locked",24,14)
des.door("closed",31,14)
des.door("locked",49,14)
-- Lord Carnarvon
des.monster({ id = "Lord Carnarvon", coord = {25, 10}, inventory = function()
   des.object({ id = "fedora", spe = 5 });
   des.object({ id = "bullwhip", spe = 4 });
end })
-- The treasure of Lord Carnarvon
des.object("chest", 25, 10)
-- student guards for the audience chamber
des.monster("student", 26, 09)
des.monster("student", 27, 09)
des.monster("student", 28, 09)
des.monster("student", 26, 10)
des.monster("student", 28, 10)
des.monster("student", 26, 11)
des.monster("student", 27, 11)
des.monster("student", 28, 11)
-- city watch guards in the antechambers
des.monster("watchman", 50, 06)
des.monster("watchman", 50, 14)
-- Eels in the moat
des.monster("giant eel", 20, 10)
des.monster("giant eel", 45, 04)
des.monster("giant eel", 33, 16)
-- Non diggable walls
des.non_diggable(selection.area(00,00,75,19))
-- Random traps
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
-- Monsters on siege duty.
des.monster("S", 60, 09)
des.monster("M", 60, 10)
des.monster("S", 60, 11)
des.monster("S", 60, 12)
des.monster("M", 60, 13)
des.monster("S", 61, 10)
des.monster("S", 61, 11)
des.monster("S", 61, 12)
des.monster("S", 30, 03)
des.monster("M", 20, 17)
des.monster("S", 67, 02)
des.monster("S", 10, 19)
