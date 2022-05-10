-- NetHack Rogue Rog-strt.lua	$NHDT-Date: 1652196012 2022/05/10 15:20:12 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.4 $
--	Copyright (c) 1992 by Dean Luick
-- NetHack may be freely redistributed.  See license for details.
--
--
--	The "start" level for the quest.
--
--	Here you meet your (besieged) class leader, Master of Thieves
--	and receive your quest assignment.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "noteleport", "hardfloor", "nommap")

--         1         2         3         4         5         6         7
--123456789012345678901234567890123456789012345678901234567890123456789012345
des.map([[
---------------------------------.------------------------------------------
|.....|.||..........|....|......|.|.........|.......+............---.......|
|.....|..+..........+....---....S.|...-S-----.-----.|............+.+.......|
|.....+.||........---......|....|.|...|.....|.|...|.---.....------.--------|
|-----|.-------|..|........------.-----.....|.--..|...-------..............|
|.....|........------+------..........+.....|..--S---.........------.-----..
|.....|.------...............-----.}}.--------.|....-------.---....|.+...--|
|..-+--.|....|-----.--------.|...|.....+.....|.|....|.....+.+......|.--....|
|..|....|....|....+.|......|.|...-----.|.....|.--...|.....|.|......|..|....|
|..|.-----S----...|.+....-----...|...|.----..|..|.---....--.---S-----.|----|
|..|.|........|...------.|.S.....|...|....-----.+.|......|..|.......|.|....|
|---.-------..|...|....|.|.|.....|...----.|...|.|---.....|.|-.......|.---..|
...........|..S...|....---.----S----..|...|...+.|..-------.---+-....|...--+|
|---------.---------...|......|....S..|.---...|.|..|...........----.---....|
|........|.........|...+.------....|---.---...|.--+-.----.----....|.+...--+|
|........|.---+---.|----.--........|......-----......|..|..|.--+-.|.-S-.|..|
|........|.|.....|........----------.----.......---.--..|-.|....|.-----.|..|
|----....+.|.....----+---............|..|--------.+.|...SS.|....|.......|..|
|...--+-----.....|......|.------------............---...||.------+--+----..|
|..........S.....|......|.|..........S............|.....||...|.....|....|..|
-------------------------.--------------------------------------------------
]]);
-- Dungeon Description
--REGION:(00,00,75,20),lit,"ordinary"

local streets = selection.floodfill(0,12)

-- The down stairs is at one of the 4 "exits".  The others are mimics,
-- mimicing stairwells.
local place = { {33,0}, {0,12}, {25,20}, {75,05} }
shuffle(place)

des.stair({ dir = "down", coord = place[1] })
des.monster({ id = "giant mimic", coord = place[2], appear_as = "ter:staircase down" })
des.monster({ id = "large mimic", coord = place[3], appear_as = "ter:staircase down" })
des.monster({ id = "small mimic", coord = place[4], appear_as = "ter:staircase down" })
-- Portal arrival point
des.levregion({ region = {19,09,19,09}, type="branch" })
-- Doors (secret)
--DOOR:locked|closed|open,(xx,yy)
des.door("locked", 32, 2)
des.door("locked", 63, 9)
des.door("locked", 27,10)
des.door("locked", 31,12)
des.door("locked", 35,13)
des.door("locked", 69,15)
des.door("locked", 56,17)
des.door("locked", 57,17)
des.door("locked", 11,19)
des.door("locked", 37,19)
des.door("locked", 39, 2)
des.door("locked", 49, 5)
des.door("locked", 10, 9)
des.door("locked", 14,12)
-- Doors (regular)
des.door("closed", 52, 1)
des.door("closed",  9, 2)
des.door("closed", 20, 2)
des.door("closed", 65, 2)
des.door("closed", 67, 2)
des.door("closed",  6, 3)
des.door("closed", 21, 5)
des.door("closed", 38, 5)
des.door("closed", 69, 6)
des.door("closed",  4, 7)
des.door("closed", 39, 7)
des.door("closed", 58, 7)
des.door("closed", 60, 7)
des.door("closed", 18, 8)
des.door("closed", 20, 9)
des.door("closed", 48,10)
des.door("closed", 46,12)
des.door("closed", 62,12)
des.door("closed", 74,12)
des.door("closed", 23,14)
des.door("closed", 23,14)
des.door("closed", 50,14)
des.door("closed", 68,14)
des.door("closed", 74,14)
des.door("closed", 14,15)
des.door("closed", 63,15)
des.door("closed",  9,17)
des.door("closed", 21,17)
des.door("closed", 50,17)
des.door("closed",  6,18)
des.door("closed", 65,18)
des.door("closed", 68,18)
-- Master of Thieves
des.monster({ id = "Master of Thieves", coord = {36, 11}, inventory = function()
   des.object({ id = "leather armor", spe = 5 });
   des.object({ id = "silver dagger", spe = 4 });
   des.object({ id = "dagger", spe = 2, quantity = d(2,4), buc = "not-cursed" });
end })
-- The treasure of Master of Thieves
des.object("chest", 36, 11)
-- thug guards, room #1
des.monster("thug", 28, 10)
des.monster("thug", 29, 11)
des.monster("thug", 30, 09)
des.monster("thug", 31, 07)
-- thug guards, room #2
des.monster("thug", 31, 13)
des.monster("thug", 33, 14)
des.monster("thug", 30, 15)
--thug guards, room #3
des.monster("thug", 35, 09)
des.monster("thug", 36, 13)
-- Non diggable walls
des.non_diggable(selection.area(00,00,75,20))
-- Random traps
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
--
-- Monsters to get in the way.
--
-- West exit
des.monster({ id = "leprechaun", x=01, y=12, peaceful=0 })
des.monster({ id = "water nymph", x=02, y=12, peaceful=0 })
-- North exit
des.monster({ id = "water nymph", x=33, y=01, peaceful=0 })
des.monster({ id = "leprechaun", x=33, y=02, peaceful=0 })
-- East exit
des.monster({ id = "water nymph", x=74, y=05, peaceful=0 })
des.monster({ id = "leprechaun", x=74, y=04, peaceful=0 })
-- South exit
des.monster({ id = "leprechaun", x=25, y=19, peaceful=0 })
des.monster({ id = "water nymph", x=25, y=18, peaceful=0 })
-- Wandering the streets.
for i=1,4 + math.random(1 - 1,1*3)  do
   des.monster({ id = "water nymph", coord = streets:rndcoord(1), peaceful=0 })
   des.monster({ id = "leprechaun", coord = streets:rndcoord(1), peaceful=0 })
end
for i=1,7 + math.random(1 - 1,1*3)  do
   des.monster({ id = "chameleon", coord = streets:rndcoord(1), peaceful=0 })
end
