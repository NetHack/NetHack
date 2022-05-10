-- NetHack Tourist Tou-loca.lua	$NHDT-Date: 1652196015 2022/05/10 15:20:15 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.2 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991,92 by M. Stephenson, P. Winner
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "hardfloor")
des.map([[
----------------------------------------------------------------------------
|....|......|..........|......|......|...|....|.....|......|...............|
|....|......|.|------|.|......|......|.|.|....|..}..|......|.|----------|..|
|....|--+----.|......|.|-S---+|+-----|.|.S....|.....|---+--|.|..........+..|
|....|........|......|.|...|.........|.|------|..............|..........|-+|
|....+...}}...+......|.|...|.|-----|.|..............|--+----------------|..|
|----|........|------|.|---|.|.....|......|-----+-|.|.......|...........|--|
|............................|.....|.|--+-|.......|.|.......|...........|..|
|----|.....|-------------|...|--+--|.|....|.......|.|-----------+-------|..|
|....+.....+.........S...|...........|....|-------|........................|
|....|.....|.........|...|.|---------|....|.........|-------|.|----------|.|
|....|.....|---------|---|.|......|..+....|-------|.|.......|.+......S.\.|.|
|....|.....+.........S...|.|......|..|....|.......|.|.......|.|......|...|.|
|-------|..|.........|---|.|+-------------------+-|.|.......+.|----------|.|
|.......+..|---------|.........|.........|..........|.......|.|..........|.|
|.......|..............|--+--|.|.........|.|----+-----------|.|..........|.|
|---------+-|--+-----|-|.....|.|.........|.|........|.|.....+.|..........+.|
|...........|........|.S.....|.|----+----|.|--------|.|.....|.|----------|.|
|...........|........|.|.....|........................|.....|..............|
----------------------------------------------------------------------------
]]);
-- Dungeon Description
des.region(selection.area(00,00,75,19), "lit")
des.non_diggable(selection.area(00,00,75,19))
--
des.region({ region={01,01, 04,05}, lit=0, type="morgue", filled=1 })
des.region({ region={15,03, 20,05}, lit=1, type="shop", filled=1 })
des.region({ region={62,03, 71,04}, lit=1, type="shop", filled=1 })
des.region({ region={01,17, 11,18}, lit=1, type="barracks", filled=1 })
des.region({ region={12,09, 20,10}, lit=1, type="barracks", filled=1 })
des.region({ region={53,11, 59,14}, lit=1, type="zoo", filled=1 })
des.region({ region={63,14, 72,16}, lit=1, type="barracks", filled=1 })
des.region({ region={32,14, 40,16}, lit=1, type="temple", filled=1 })
--
des.region({ region = {06,01,11,02}, type = "ordinary" })
des.region({ region = {24,01,29,02}, type = "ordinary" })
des.region({ region = {31,01,36,02}, type = "ordinary" })
des.region({ region = {42,01,45,03}, type = "ordinary" })
des.region({ region = {53,01,58,02}, type = "ordinary" })
des.region({ region = {24,04,26,05}, type = "ordinary" })
des.region({ region = {30,06,34,07}, type = "ordinary" })
des.region(selection.area(73,05,74,05), "unlit")
des.region({ region = {01,09,04,12}, type = "ordinary" })
des.region({ region = {01,14,07,15}, type = "ordinary" })
des.region({ region = {12,12,20,13}, type = "ordinary" })
des.region({ region = {13,17,20,18}, type = "ordinary" })
des.region({ region = {22,09,24,10}, type = "ordinary" })
des.region({ region = {22,12,24,12}, type = "ordinary" })
des.region({ region = {24,16,28,18}, type = "ordinary" })
des.region({ region = {28,11,33,12}, type = "ordinary" })
des.region(selection.area(35,11,36,12), "lit")
des.region({ region = {38,08,41,12}, type = "ordinary" })
des.region({ region = {43,07,49,08}, type = "ordinary" })
des.region({ region = {43,12,49,12}, type = "ordinary" })
des.region({ region = {44,16,51,16}, type = "ordinary" })
des.region({ region = {53,06,59,07}, type = "ordinary" })
des.region({ region = {61,06,71,07}, type = "ordinary" })
des.region({ region = {55,16,59,18}, type = "ordinary" })
des.region({ region = {63,11,68,12}, type = "ordinary" })
des.region({ region = {70,11,72,12}, type = "ordinary" })
-- Stairs
des.stair("up", 10,04)
des.stair("down", 73,05)
-- Non diggable walls
des.non_diggable(selection.area(00,00,75,19))
des.door("closed",05,05)
des.door("closed",05,09)
des.door("closed",08,14)
des.door("closed",08,03)
des.door("closed",11,09)
des.door("closed",11,12)
des.door("closed",10,16)
des.door("closed",14,05)
des.door("closed",15,16)
des.door("locked",21,09)
des.door("locked",21,12)
des.door("closed",23,17)
des.door("closed",25,03)
des.door("closed",26,15)
des.door("closed",29,03)
des.door("closed",28,13)
des.door("closed",31,03)
des.door("closed",32,08)
des.door("closed",37,11)
des.door("closed",36,17)
des.door("locked",41,03)
des.door("closed",40,07)
des.door("closed",48,06)
des.door("closed",48,13)
des.door("closed",48,15)
des.door("closed",56,03)
des.door("closed",55,05)
des.door("closed",72,03)
des.door("locked",74,04)
des.door("closed",64,08)
des.door("closed",62,11)
des.door("closed",69,11)
des.door("closed",60,13)
des.door("closed",60,16)
des.door("closed",73,16)

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
-- Toilet paper
des.object("blank paper", 71, 12)
des.object("blank paper", 71, 12)
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
-- Random monsters.
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
