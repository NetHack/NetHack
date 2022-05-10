-- NetHack Knight Kni-strt.lua	$NHDT-Date: 1652196006 2022/05/10 15:20:06 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.4 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991,92 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
--
--	The "start" level for the quest.
--
--	Here you meet your (besieged) class leader, King Arthur
--	and receive your quest assignment.
--
des.level_init({ style = "solidfill", fg = "." });

des.level_flags("mazelevel", "noteleport", "hardfloor")
-- This is a kludge to init the level as a lit field.
des.level_init({ style="mines", fg=".", bg=".", smoothed=false, joined=false, lit=1, walled=false })

des.map([[
..................................................
.-----......................................-----.
.|...|......................................|...|.
.--|+-------------------++-------------------+|--.
...|...................+..+...................|...
...|.|-----------------|++|-----------------|.|...
...|.|.................|..|.........|.......|.|...
...|.|...\.............+..+.........|.......|.|...
...|.|.................+..+.........+.......|.|...
...|.|.................|..|.........|.......|.|...
...|.|--------------------------------------|.|...
...|..........................................|...
.--|+----------------------------------------+|--.
.|...|......................................|...|.
.-----......................................-----.
..................................................
]]);
-- Dungeon Description
des.region(selection.area(00,00,49,15), "lit")
des.region(selection.area(04,04,45,11), "unlit")
des.region({ region={06,06,22,09}, lit=1, type="throne", filled=2 })
des.region(selection.area(27,06,43,09), "lit")
-- Portal arrival point
des.levregion({ region = {20,14,20,14}, type="branch" })
-- Stairs
des.stair("down", 40,7)
-- Doors
-- Outside Doors
des.door("locked",24,03)
des.door("locked",25,03)
-- Inside Doors
des.door("closed",23,04)
des.door("closed",26,04)
des.door("locked",24,05)
des.door("locked",25,05)
des.door("closed",23,07)
des.door("closed",26,07)
des.door("closed",23,08)
des.door("closed",26,08)
des.door("closed",36,08)
-- Watchroom Doors
des.door("closed",04,03)
des.door("closed",45,03)
des.door("closed",04,12)
des.door("closed",45,12)
-- King Arthur
des.monster({ id = "King Arthur", coord = {09, 07}, inventory = function()
   des.object({ id = "long sword", spe = 4, buc = "blessed", name = "Excalibur" });
   des.object({ id = "plate mail", spe = 4 });
end })
-- The treasure of King Arthur
des.object("chest", 09, 07)
-- knight guards for the watchrooms
des.monster({ id = "knight", x=04, y=02, peaceful = 1 })
des.monster({ id = "knight", x=04, y=13, peaceful = 1 })
des.monster({ id = "knight", x=45, y=02, peaceful = 1 })
des.monster({ id = "knight", x=45, y=13, peaceful = 1 })
-- page guards for the audience chamber
des.monster("page", 16, 06)
des.monster("page", 18, 06)
des.monster("page", 20, 06)
des.monster("page", 16, 09)
des.monster("page", 18, 09)
des.monster("page", 20, 09)
-- Non diggable walls
des.non_diggable(selection.area(00,00,49,15))
-- Random traps
des.trap("sleep gas",24,04)
des.trap("sleep gas",25,04)
des.trap()
des.trap()
des.trap()
des.trap()
-- Monsters on siege duty.
des.monster({ id = "quasit", x=14, y=00, peaceful=0 })
des.monster({ id = "quasit", x=16, y=00, peaceful=0 })
des.monster({ id = "quasit", x=18, y=00, peaceful=0 })
des.monster({ id = "quasit", x=20, y=00, peaceful=0 })
des.monster({ id = "quasit", x=22, y=00, peaceful=0 })
des.monster({ id = "quasit", x=24, y=00, peaceful=0 })
des.monster({ id = "quasit", x=26, y=00, peaceful=0 })
des.monster({ id = "quasit", x=28, y=00, peaceful=0 })
des.monster({ id = "quasit", x=30, y=00, peaceful=0 })
des.monster({ id = "quasit", x=32, y=00, peaceful=0 })
des.monster({ id = "quasit", x=34, y=00, peaceful=0 })
des.monster({ id = "quasit", x=36, y=00, peaceful=0 })

-- Some warhorses
for i = 1, 2 + nh.rn2(3) do
    des.monster({ id = "warhorse", peaceful = 1, inventory = function() if percent(50) then des.object("saddle"); end end });
end
