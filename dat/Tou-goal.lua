-- NetHack Tourist Tou-goal.lua	$NHDT-Date: 1652196015 2022/05/10 15:20:15 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.4 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991,92 by M. Stephenson, P. Winner
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel");

des.map([[
----------------------------------------------------------------------------
|.........|.........|..........|..| |.................|........|........|..|
|.........|.........|..........|..| |....--------.....|........|........|..|
|------S--|--+-----------+------..| |....|......|.....|........|........|..|
|.........|.......................| |....|......+.....--+-------------+--..|
|.........|.......................| |....|......|..........................|
|-S-----S-|......----------.......| |....|......|..........................|
|..|..|...|......|........|.......| |....-----------.........----..........|
|..+..+...|......|........|.......| |....|.........|.........|}}|..........|
|..|..|...|......+........|.......| |....|.........+.........|}}|..........|
|..|..|...|......|........|.......S.S....|.........|.........----..........|
|---..----|......|........|.......| |....|.........|.......................|
|.........+......|+F-+F-+F|.......| |....-----------.......................|
|---..----|......|..|..|..|.......| |......................--------------..|
|..|..|...|......--F-F--F--.......| |......................+............|..|
|..+..+...|.......................| |--.---...-----+-----..|............|..|
|--|..----|--+-----------+------..| |.....|...|.........|..|------------|..|
|..+..+...|.........|..........|..| |.....|...|.........|..+............|..|
|..|..|...|.........|..........|..| |.....|...|.........|..|............|..|
----------------------------------------------------------------------------
]]);
-- Dungeon Description
des.region(selection.area(00,00,75,19), "lit")
-- The Inn
des.region(selection.area(01,01,09,02), "lit")
des.region({ region = {01,04,09,05}, lit=1, type = "barracks", filled = 1 })
des.region(selection.area(01,07,02,10), "unlit")
des.region(selection.area(07,07,09,10), "unlit")
des.region(selection.area(01,14,02,15), "unlit")
des.region(selection.area(07,14,09,15), "unlit")
des.region(selection.area(01,17,02,18), "unlit")
des.region(selection.area(07,17,09,18), "unlit")
--
des.region({ region = {11,01,19,02}, lit = 0, type = "barracks", filled = 1 })
des.region(selection.area(21,01,30,02), "unlit")
des.region({ region = {11,17,19,18}, lit = 0, type = "barracks", filled = 1 })
des.region(selection.area(21,17,30,18), "unlit")
-- Police Station
des.region(selection.area(18,07,25,11), "lit")
des.region(selection.area(18,13,19,13), "unlit")
des.region(selection.area(21,13,22,13), "unlit")
des.region(selection.area(24,13,25,13), "unlit")
-- The town itself
des.region(selection.area(42,03,47,06), "unlit")
des.region(selection.area(42,08,50,11), "unlit")
des.region({ region = {37,16,41,18}, lit = 0, type = "morgue", filled = 1 })
des.region(selection.area(47,16,55,18), "unlit")
des.region(selection.area(55,01,62,03), "unlit")
des.region(selection.area(64,01,71,03), "unlit")
des.region({ region = {60,14,71,15}, lit = 1, type = "shop", filled = 1 })
des.region({ region = {60,17,71,18}, lit = 1, type = "shop", filled = 1 })
-- Non diggable walls
des.non_diggable(selection.area(00,00,75,19))
-- Stairs
des.stair("up", 70,08)
-- Doors
des.door("locked",07,03)
des.door("locked",02,06)
des.door("locked",08,06)
des.door("closed",03,08)
des.door("closed",06,08)
des.door("open",10,12)
des.door("closed",03,15)
des.door("closed",06,15)
des.door("closed",03,17)
des.door("closed",06,17)
des.door("closed",13,03)
des.door("random",25,03)
des.door("closed",13,16)
des.door("random",25,16)
des.door("locked",17,09)
des.door("locked",18,12)
des.door("locked",21,12)
des.door("locked",24,12)
des.door("locked",34,10)
des.door("locked",36,10)
des.door("random",48,04)
des.door("random",56,04)
des.door("random",70,04)
des.door("random",51,09)
des.door("random",51,15)
des.door("open",59,14)
des.door("open",59,17)
-- Objects
des.object({ id = "credit card", x=04, y=01, buc="blessed", spe=0, name="The Platinum Yendorian Express Card" })
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
des.object()
des.object()
des.object()
des.object()
-- Random traps - must avoid the 2 shops
local validtraps = selection.area(00,00,75,19):filter_mapchar('.')
validtraps = validtraps - selection.area(60,14,71,18)
for i=1,6 do
   des.trap(validtraps:rndcoord(1))
end
-- Random monsters.
des.monster({ id = "Master of Thieves", x=04, y=01, peaceful = 0 })
des.monster("giant spider")
des.monster("giant spider")
des.monster("giant spider")
des.monster("giant spider")
des.monster("giant spider")
des.monster("giant spider")
des.monster("giant spider")
des.monster("giant spider")
des.monster("giant spider")
des.monster("giant spider")
des.monster("giant spider")
des.monster("giant spider")
des.monster("giant spider")
des.monster("giant spider")
des.monster("giant spider")
des.monster("giant spider")
des.monster("s")
des.monster("s")
-- ladies of the evening
des.monster("succubus", 02, 08)
des.monster("succubus", 08, 08)
des.monster("incubus", 02, 14)
des.monster("incubus", 08, 14)
des.monster("incubus", 02, 17)
des.monster("incubus", 08, 17)
-- Police station (with drunken prisoners)
des.monster({ id = "Kop Kaptain", x=24, y=09, peaceful = 0 })
des.monster({ id = "Kop Lieutenant", x=20, y=09, peaceful = 0 })
des.monster({ id = "Kop Lieutenant", x=22, y=11, peaceful = 0 })
des.monster({ id = "Kop Lieutenant", x=22, y=07, peaceful = 0 })
des.monster({ id = "Keystone Kop", x=19, y=07, peaceful = 0 })
des.monster({ id = "Keystone Kop", x=19, y=08, peaceful = 0 })
des.monster({ id = "Keystone Kop", x=22, y=09, peaceful = 0 })
des.monster({ id = "Keystone Kop", x=24, y=11, peaceful = 0 })
des.monster({ id = "Keystone Kop", x=19, y=11, peaceful = 0 })
des.monster("prisoner", 19, 13)
des.monster("prisoner", 21, 13)
des.monster("prisoner", 24, 13)
--
des.monster({ id = "watchman", x=33, y=10, peaceful = 0 })

des.wallify()
