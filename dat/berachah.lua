-- The Valley of Berachah. Same geometry as Hinnom, redeemed.
des.level_init({ style = "solidfill", fg = " " })

des.level_flags("mazelevel", "nomongen", "noteleport", "hardfloor")

des.map({ halign = "left", valign = "top", map = [[
-------------------------------------------------------------------------------
|T...........T|.|T|T........|....T||....|TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT|
|.............|.|.---.-----.|.|.---|....|TPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPT|
|..T.......T..+.|.|...|.....|T|...||....|TP-----PPPPPPPPPPPPPPPPPPPPPPP-----PT|
|.............|.|.|.---.-.-------.|--+---TP|...|-----------------------|...|PT|
|.............|...|...|...........|....|.TP|...|......|......|T.T.T.T.T|...|PT|
|T...........T|.|.---.-----------.|..TT|.TP|...|......|......|.........|...|PT|
-------+-------.|.....|.........|.|..{...TP--+--+-----------+------+-----+--PT|
|T......TTTTTT|.------T.-------.|.|.T---.TPP|.....|..|..|..|....|.........|PPT|
|T.TTTTTTTTTTT|.|....T|...|...|T|.|......TPP|.....|-+--+--+--+--|.........|PPT|
|T...........T|.|.|.---.|.|.|.---.|......+........+.............+.........|PPT|
|TTTTTTTTTTT.T|.|.|...|.|...|...|........TPP|.....|--+------+---|.........|PPT|
|TTTTTT......T|.|.---.|.-------.|.|--|...TPP|.....|.............|.........|PPT|
-------+-------.|.|T|...|.....|...|..|.T.TP--+-----+------+---------+----+--PT|
|T...........T|.|.|.-----.---.-----.T.T..TP|...|......|T......T|.......|...|PT|
|.............|.........|...|...|T|......TP|...|......|T..{...T|.......|...|PT|
|.............|.-------.---.---.|.|---+--TP|...|-----------------------|...|PT|
|.............|.|....T|...|.|...|.||....|TP-----PPPPPPPPPPPPPPPPPPPPPPP-----PT|
|.............|.|.-------.|.|.---.||....|TPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPT|
|T...........T|.|...........|.....||....|TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT|
-------------------------------------------------------------------------------
]] })

des.non_diggable(selection.area(0, 0, 78, 20))
des.region({ region = {1, 1, 77, 19}, lit = 1, type = "ordinary" })
des.stair("up", 4, 17)
des.stair("down", 69, 15)
des.altar({ x = 69, y = 10, align = "law", type = "altar" })

des.region({ region = {48, 5, 53, 6}, lit = 1, type = "scroll shop", filled = 1 })
des.region({ region = {55, 5, 60, 6}, lit = 1, type = "book shop", filled = 1 })
des.region({ region = {48, 14, 53, 15}, lit = 1, type = "shop", filled = 1 })

des.door("nodoor", 7, 7)
des.door("nodoor", 14, 3)
des.door("nodoor", 7, 13)
des.door("open", 41, 10)
des.door("nodoor", 44, 10)
des.door("closed", 45, 7)
des.door("closed", 73, 7)
des.door("closed", 45, 13)
des.door("closed", 73, 13)
des.door("closed", 48, 7)
des.door("closed", 60, 7)
des.door("closed", 61, 9)
des.door("closed", 52, 9)
des.door("closed", 55, 9)
des.door("closed", 58, 9)
des.door("closed", 53, 11)
des.door("closed", 60, 11)
des.door("closed", 50, 10)
des.door("closed", 51, 13)
des.door("closed", 58, 13)
des.door("closed", 64, 10)
des.door("closed", 67, 7)
des.door("closed", 68, 13)
des.door("broken", 37, 4)
des.door("broken", 38, 16)

des.monster({ id = "Michael", x = 72, y = 10, peaceful = 1 })
des.monster({ id = "Angel", x = 40, y = 9, peaceful = 1 })
des.monster({ id = "Angel", x = 40, y = 11, peaceful = 1 })
des.monster({ id = "Angel", x = 46, y = 10, peaceful = 1 })
des.monster({ id = "Angel", x = 57, y = 10, peaceful = 1 })
des.monster({ id = "Angel", x = 73, y = 8, peaceful = 1 })
des.monster({ id = "Angel", x = 73, y = 12, peaceful = 1 })
des.monster({ id = "child", x = 66, y = 8, peaceful = 1, waiting = 1 })
des.monster({ id = "child", x = 68, y = 8, peaceful = 1, waiting = 1 })
des.monster({ id = "child", x = 70, y = 8, peaceful = 1, waiting = 1 })
des.monster({ id = "child", x = 66, y = 12, peaceful = 1, waiting = 1 })
des.monster({ id = "child", x = 68, y = 12, peaceful = 1, waiting = 1 })
des.monster({ id = "child", x = 70, y = 12, peaceful = 1, waiting = 1 })
