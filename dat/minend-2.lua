-- NetHack mines minend-2.lua	$NHDT-Date: 1652196029 2022/05/10 15:20:29 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.4 $
--	Copyright (c) 1989-95 by Jean-Christophe Collet
--	Copyright (c) 1991-95 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
-- Mine end level variant 2
-- "Gnome King's Wine Cellar"

des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel");

des.map([[
---------------------------------------------------------------------------
|...................................................|                     |
|.|---------S--.--|...|--------------------------|..|                     |
|.||---|   |.||-| |...|..........................|..|                     |
|.||...| |-|.|.|---...|.............................|                ..   |
|.||...|-|.....|....|-|..........................|..|.               ..   |
|.||.....|-S|..|....|............................|..|..                   |
|.||--|..|..|..|-|..|----------------------------|..|-.                   |
|.|   |..|..|....|..................................|...                  |
|.|   |..|..|----|..-----------------------------|..|....                 |
|.|---|..|--|.......|----------------------------|..|.....                |
|...........|----.--|......................|     |..|.......              |
|-----------|...|.| |------------------|.|.|-----|..|.....|..             |
|-----------|.{.|.|--------------------|.|..........|.....|....           |
|...............|.S......................|-------------..-----...         |
|.--------------|.|--------------------|.|.........................       |
|.................|                    |.....................|........    |
---------------------------------------------------------------------------
]]);

if percent(50) then
   des.terrain({55,14},"-")
   des.terrain({56,14},"-")
   des.terrain({61,15},"|")
   des.terrain({52,5}, "S")
   des.door("locked", 52,5)
end
if percent(50) then
   des.terrain({18,1}, "|")
   des.terrain(selection.area(7,12, 8,13), ".")
end
if percent(50) then
   des.terrain({49,4}, "|")
   des.terrain({21,5}, ".")
end
if percent(50) then
   if percent(50) then
      des.terrain({22,1}, "|")
   else
      des.terrain({50,7}, "-")
      des.terrain({51,7}, "-")
   end
end


-- Dungeon Description
des.feature("fountain", {14,13})
des.region(selection.area(23,03,48,06),"lit")
des.region(selection.area(21,06,22,06),"lit")
des.region(selection.area(14,04,14,04),"unlit")
des.region(selection.area(10,05,14,08),"unlit")
des.region(selection.area(10,09,11,09),"unlit")
des.region(selection.area(15,08,16,08),"unlit")
-- Secret doors
des.door("locked",12,02)
des.door("locked",11,06)
-- Stairs
des.stair("up", 36,04)
-- Non diggable walls
des.non_diggable(selection.area(00,00,52,17))
des.non_diggable(selection.area(53,00,74,00))
des.non_diggable(selection.area(53,17,74,17))
des.non_diggable(selection.area(74,01,74,16))
des.non_diggable(selection.area(53,07,55,07))
des.non_diggable(selection.area(53,14,61,14))
-- The Gnome King's wine cellar.
-- the Trespassers sign is a long-running joke
des.engraving({12,03}, "engrave",
	      "You are now entering the Gnome King's wine cellar.")
des.engraving({12,04}, "engrave", "Trespassers will be persecuted!")
des.object("potion of booze", 10, 07)
des.object("potion of booze", 10, 07)
des.object("!", 10, 07)
des.object("potion of booze", 10, 08)
des.object("potion of booze", 10, 08)
des.object("!", 10, 08)
des.object("potion of booze", 10, 09)
des.object("potion of booze", 10, 09)
des.object("potion of object detection", 10, 09)
-- Objects
-- The Treasure chamber...
des.object("diamond", 69, 04)
des.object("*", 69, 04)
des.object("diamond", 69, 04)
des.object("*", 69, 04)
des.object("emerald", 70, 04)
des.object("*", 70, 04)
des.object("emerald", 70, 04)
des.object("*", 70, 04)
des.object("emerald", 69, 05)
des.object("*", 69, 05)
des.object("ruby", 69, 05)
des.object("*", 69, 05)
des.object("ruby", 70, 05)
des.object("amethyst", 70, 05)
des.object("*", 70, 05)
des.object("amethyst", 70, 05)
des.object({ id="luckstone", x=70, y=05,
	     buc="not-cursed", achievement=1 });
-- Scattered gems...
des.object("*")
des.object("*")
des.object("*")
des.object("*")
des.object("*")
des.object("*")
des.object("*")
des.object("(")
des.object("(")
des.object()
des.object()
des.object()
-- Random traps
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
-- Random monsters.
des.monster("gnome king")
des.monster("gnome lord")
des.monster("gnome lord")
des.monster("gnome lord")
des.monster("gnomish wizard")
des.monster("gnomish wizard")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("hobbit")
des.monster("hobbit")
des.monster("dwarf")
des.monster("dwarf")
des.monster("dwarf")
des.monster("h")
