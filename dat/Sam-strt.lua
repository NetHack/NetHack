-- NetHack Samurai Sam-strt.lua	$NHDT-Date: 1652196014 2022/05/10 15:20:14 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.3 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991-92 by M. Stephenson, P. Winner
-- NetHack may be freely redistributed.  See license for details.
--
--
--	The "start" level for the quest.
--
--	Here you meet your (besieged) class leader, Lord Sato
--	and receive your quest assignment.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "noteleport", "hardfloor")

des.map([[
..............................................................PP............
...............................................................PP...........
..........---------------------------------------------------...PPP.........
..........|......|.........|...|..............|...|.........|....PPPPP......
......... |......|.........S...|..............|...S.........|.....PPPP......
..........|......|.........|---|..............|---|.........|.....PPP.......
..........+......|.........+...-------++-------...+.........|......PP.......
..........+......|.........|......................|.........|......PP.......
......... |......---------------------++--------------------|........PP.....
..........|.................................................|.........PP....
..........|.................................................|...........PP..
..........----------------------------------------...-------|............PP.
..........................................|.................|.............PP
.............. ................. .........|.................|..............P
............. } ............... } ........|.................|...............
.............. ........PP....... .........|.................|...............
.....................PPP..................|.................|...............
......................PP..................-------------------...............
............................................................................
............................................................................
]]);
-- Dungeon Description
des.region(selection.area(00,00,75,19), "lit")
des.region({ region={18,03, 26,07}, lit=1, type="throne", filled=2 })
-- Portal arrival zone
des.levregion({ region = {62,12,70,17}, type="branch" })
-- Stairs
des.stair("down", 29,04)
-- Doors
des.door("locked",10,06)
des.door("locked",10,07)
des.door("closed",27,04)
des.door("closed",27,06)
des.door("closed",38,06)
des.door("locked",38,08)
des.door("closed",39,06)
des.door("locked",39,08)
des.door("closed",50,04)
des.door("closed",50,06)
-- Lord Sato
des.monster({ id = "Lord Sato", coord = {20, 04}, inventory = function()
   des.object({ id = "splint mail", spe = 5 });
   des.object({ id = "katana", spe = 4 });
end })
-- The treasure of Lord Sato
des.object("chest", 20, 04)
-- roshi guards for the audience chamber
des.monster("roshi", 18, 04)
des.monster("roshi", 18, 05)
des.monster("roshi", 18, 06)
des.monster("roshi", 18, 07)
des.monster("roshi", 26, 04)
des.monster("roshi", 26, 05)
des.monster("roshi", 26, 06)
des.monster("roshi", 26, 07)
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
des.monster({ id = "ninja", x=64, y=00, peaceful=0 })
des.monster("wolf", 65, 01)
des.monster({ id = "ninja", x=67, y=02, peaceful=0 })
des.monster({ id = "ninja", x=69, y=05, peaceful=0 })
des.monster({ id = "ninja", x=69, y=06, peaceful=0 })
des.monster("wolf", 69, 07)
des.monster({ id = "ninja", x=70, y=06, peaceful=0 })
des.monster({ id = "ninja", x=70, y=07, peaceful=0 })
des.monster({ id = "ninja", x=72, y=01, peaceful=0 })
des.monster("wolf", 75, 09)
des.monster({ id = "ninja", x=73, y=05, peaceful=0 })
des.monster({ id = "ninja", x=68, y=02, peaceful=0 })
des.monster("stalker")
