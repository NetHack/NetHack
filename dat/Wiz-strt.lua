-- NetHack Wizard Wiz-strt.lua	$NHDT-Date: 1652196019 2022/05/10 15:20:19 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.3 $
--	Copyright (c) 1992 by David Cohrs
-- NetHack may be freely redistributed.  See license for details.
--
--
--	The "start" level for the quest.
--
--	Here you meet your (besieged) class leader, Neferet the Green
--	and receive your quest assignment.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "noteleport", "hardfloor")

des.map([[
............................................................................
.....................C....CC.C........................C.....................
..........CCC.....................CCC.......................................
........CC........-----------.......C.C...C...C....C........................
.......C.....---------------------...C..C..C..C.............................
......C..C...------....\....------....C.....C...............................
........C...||....|.........|....||.........................................
.......C....||....|.........+....||.........................................
.......C...||---+--.........|....|||........................................
......C....||...............|--S--||........................................
...........||--+--|++----|---|..|.SS..........C......C......................
........C..||.....|..|...|...|--|.||..CC..C.....C..........C................
.......C...||.....|..|.--|.|.|....||.................C..C...................
.....C......||....|..|.....|.|.--||..C..C..........C...........}}}..........
......C.C...||....|..-----.|.....||...C.C.C..............C....}}}}}}........
.........C...------........|------....C..C.....C..CC.C......}}}}}}}}}}}.....
.........CC..---------------------...C.C..C.....CCCCC.C.......}}}}}}}}......
.........C........-----------..........C.C.......CCC.........}}}}}}}}}......
..........C.C.........................C............C...........}}}}}........
......................CCC.C.................................................
]]);

-- first do cloud everywhere
des.replace_terrain({ region={0,0, 75,19}, fromterrain=".", toterrain="C", chance=10 })
-- then replace clouds inside the tower back to floor
des.replace_terrain({ region={13,5, 33,15}, fromterrain="C", toterrain=".", chance=100 })

-- Dungeon Description
des.region(selection.area(00,00,75,19), "lit")
des.region(selection.area(35,00,49,03), "unlit")
des.region(selection.area(43,12,49,16), "unlit")
des.region({ region={19,11,33,15}, lit=0, type="ordinary", irregular=1 })
des.region(selection.area(30,10,31,10), "unlit")
-- Stairs
des.stair("down", 30,10)
-- Portal arrival point
des.terrain({63,06}, ".")
des.levregion({ region = {63,06,63,06}, type="branch" })
-- Doors
des.door("closed",31,09)
des.door("closed",16,08)
des.door("closed",28,07)
des.door("locked",34,10)
des.door("locked",35,10)
des.door("closed",15,10)
des.door("locked",19,10)
des.door("locked",20,10)
-- Neferet the Green, the quest leader
des.monster({ id = "Neferet the Green", coord = {23, 05}, inventory = function()
   des.object({ id = "elven cloak", spe = 5 });
   des.object({ id = "quarterstaff", spe = 5 });
end })
-- The treasure of the quest leader
des.object("chest", 24, 05)
-- apprentice guards for the audience chamber
des.monster("apprentice", 30, 07)
des.monster("apprentice", 24, 06)
des.monster("apprentice", 15, 06)
des.monster("apprentice", 15, 12)
des.monster("apprentice", 26, 11)
des.monster("apprentice", 27, 11)
des.monster("apprentice", 19, 09)
des.monster("apprentice", 20, 09)
-- Eels in the pond
des.monster("giant eel", 62, 14)
des.monster("giant eel", 69, 15)
des.monster("giant eel", 67, 17)
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
des.monster({ class = "B", x=60, y=09, peaceful = 0 })
des.monster({ class = "W", x=60, y=10, peaceful = 0 })
des.monster({ class = "B", x=60, y=11, peaceful = 0 })
des.monster({ class = "B", x=60, y=12, peaceful = 0 })
des.monster({ class = "i", x=60, y=13, peaceful = 0 })
des.monster({ class = "B", x=61, y=10, peaceful = 0 })
des.monster({ class = "B", x=61, y=11, peaceful = 0 })
des.monster({ class = "B", x=61, y=12, peaceful = 0 })
des.monster({ class = "B", x=35, y=03, peaceful = 0 })
des.monster({ class = "i", x=35, y=17, peaceful = 0 })
des.monster({ class = "B", x=36, y=17, peaceful = 0 })
des.monster({ class = "B", x=34, y=16, peaceful = 0 })
des.monster({ class = "i", x=34, y=17, peaceful = 0 })
des.monster({ class = "W", x=67, y=02, peaceful = 0 })
des.monster({ class = "B", x=10, y=19, peaceful = 0 })
