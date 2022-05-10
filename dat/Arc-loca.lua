-- NetHack Archeologist Arc-loca.lua	$NHDT-Date: 1652195999 2022/05/10 15:19:59 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.2 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "hardfloor")

des.map([[
............................................................................
............................................................................
............................................................................
........................-------------------------------.....................
........................|....|.S......................|.....................
........................|....|.|.|+------------------.|.....................
........................|....|.|.|.|.........|......|.|.....................
........................|....|.|.|.|.........|......|.|.....................
........................|---+-.|.|.|..---....+......|.|.....................
........................|....|.|.|.---|.|....|......|.|.....................
........................|....S.|.|.+..S.|--S-----S--|.|.....................
........................|....|.|.|.---|.|....|......+.|.....................
........................|---+-.|.|.|..---....|.------.|.....................
........................|....|.|.|.|.........|.|....+.|.....................
........................|....|.|.|.|.........|+|....|-|.....................
........................|....|.|.|------------+------.S.....................
........................|....|.S......................|.....................
........................-------------------------------.....................
............................................................................
............................................................................
]]);
-- Dungeon Description
des.region(selection.area(00,00,75,19), "lit")
des.region({ region={25,04, 28,07}, lit=1, type="temple", filled=2 })
des.region({ region={25,09, 28,11}, lit=0, type="temple", filled=2 })
des.region({ region={25,13, 28,16}, lit=1, type="temple", filled=2 })
des.region(selection.area(30,04,30,16), "lit")
des.region(selection.area(32,04,32,16), "unlit")
des.region({ region={33,04, 53,04}, lit=0, type="ordinary", irregular=1 })
des.region(selection.area(36,10,37,10), "unlit")
des.region(selection.area(39,09,39,11), "unlit")
des.region({ region={36,06, 42,08}, lit=0, type="ordinary", irregular=1 })
des.region({ region={36,12, 42,14}, lit=0, type="ordinary", irregular=1 })
des.region(selection.area(46,06,51,09), "unlit")
des.region({ region={46,11, 49,11}, lit=0, type="ordinary", irregular=1 })
des.region(selection.area(48,13,51,14), "unlit")
-- Doors
des.door("closed",31,04)
des.door("closed",28,08)
des.door("locked",29,10)
des.door("closed",28,12)
des.door("closed",31,16)
des.door("locked",34,05)
des.door("locked",35,10)
des.door("locked",38,10)
des.door("closed",43,10)
des.door("closed",45,08)
des.door("locked",46,14)
des.door("locked",46,15)
des.door("locked",49,10)
des.door("locked",52,11)
des.door("closed",52,13)
des.door("closed",54,15)
-- Stairs
des.stair("up", 03,17)
des.stair("down", 39,10)
-- Altars - three types.  All are unattended.
des.altar({ x=26,y=05,align=align[1], type="altar" })
des.altar({ x=26,y=10,align=align[2], type="altar" })
des.altar({ x=26,y=15,align=align[3], type="altar" })
-- Non diggable walls
des.non_diggable(selection.area(00,00,75,19))
-- Objects
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
des.object()
-- Treasure?
des.engraving({ type="engrave", text="X marks the spot." })
des.engraving({ type="engrave", text="X marks the spot." })
des.engraving({ type="engrave", text="X marks the spot." })
des.engraving({ type="engrave", text="X marks the spot." })
-- Random traps
des.trap("spiked pit",24,02)
des.trap("spiked pit",37,00)
des.trap("spiked pit",23,05)
des.trap("spiked pit",26,19)
des.trap("spiked pit",55,10)
des.trap("spiked pit",55,08)
des.trap("pit",51,01)
des.trap("pit",23,18)
des.trap("pit",31,18)
des.trap("pit",48,19)
des.trap("pit",55,15)
des.trap("magic",60,04)
des.trap("statue",72,07)
des.trap("statue")
des.trap("statue")
des.trap("anti magic",64,12)
des.trap("sleep gas")
des.trap("sleep gas")
des.trap("dart")
des.trap("dart")
des.trap("dart")
des.trap("rolling boulder",32,10)
des.trap("rolling boulder",40,16)
-- Random monsters.
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("M")
des.monster("human mummy")
des.monster("human mummy")
des.monster("human mummy")
des.monster("human mummy")
des.monster("human mummy")
des.monster("human mummy")
des.monster("human mummy")
des.monster("M")
