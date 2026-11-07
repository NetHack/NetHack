-- The Valley of Berachah. Fortress geometry as Hinnom, with the
-- varied village in columns 16-33. The whole level is lit in code.
des.level_init({ style = "solidfill", fg = " " })

des.level_flags("mazelevel", "nomongen", "noteleport", "hardfloor", "noflip")

des.map({ halign = "left", valign = "top", map = [[
-------------------------------------------------------------------------------
|T...........T|.-----.....-------.||....|TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT|
|.............|.|...+.....|.....|.-|....|TPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPT|
|..T.......T..+.|...|.....|.....|.||....|TP-----PPPPPPPPPPPPPPPPPPPPPPP-----PT|
|.............|.-----.....---+---.|--+---TP|...|-----------------------|...|PT|
|.............|...................|....|.TP|...|......|......|T.T.T.T.T|...|PT|
|T...........T|.-----.......------|..TT|.TP|...|......|......|.........|...|PT|
-------+-------.|...|.......|....||..{...TP--+--+-----------+------+-----+--PT|
|T......TTTTTT|.|...+.......+....||.T---.TPP|.....|..|..|..|....|.........|PPT|
|T.TTTTTTTTTTT|.|...|...{...|....||......TPP|.....|-+--+--+--+--|.........|PPT|
|T...........T|.|...|.......------.......+........+.............+.........|PPT|
|TTTTTTTTTTT.T|.-----....................TPP|.....|--+------+---|.........|PPT|
|TTTTTT......T|...................|--|...TPP|.....|.............|.........|PPT|
-------+-------...............----|..|.T.TP--+-----+------+---------+----+--PT|
|T...........T|.......|-----..|..|-.T.T..TP|...|......|T......T|.......|...|PT|
|.............|.--+--.|....+..-+--|......TP|...|......|T..{...T|.......|...|PT|
|.............|.|...|.|.|---......|---+--TP|...|-----------------------|...|PT|
|.............|.-----.|.|....-----||....|TP-----PPPPPPPPPPPPPPPPPPPPPPP-----PT|
|.............|.......---....+...|||....|TPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPT|
|T...........T|..............-----||....|TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT|
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
-- Village doors. Open, so the lane and the houses are not a maze.
des.door("open", 20, 2)
des.door("open", 29, 4)
des.door("open", 20, 8)
des.door("open", 28, 8)
des.door("open", 18, 15)
des.door("open", 27, 15)
des.door("open", 31, 15)
des.door("open", 29, 18)

des.monster({ id = "Michael the Archangel", x = 72, y = 10, peaceful = 1 })
des.monster({ id = "Angel", x = 40, y = 9, peaceful = 1 })
des.monster({ id = "Angel", x = 40, y = 11, peaceful = 1 })
des.monster({ id = "Angel", x = 46, y = 8, peaceful = 1 })
-- Male prisoner, shown as "town elder". Off the bridge, in the gatehouse,
-- so the road from the court to the green stays clear.
-- A guide would add the quest-guardian death penalty on top of murder.
des.monster({ id = "prisoner", name = "town elder", x = 45, y = 12,
              peaceful = 1, waiting = 1, female = false })
des.monster({ id = "Angel", x = 71, y = 9, peaceful = 1 })
des.monster({ id = "Angel", x = 73, y = 8, peaceful = 1 })
des.monster({ id = "Angel", x = 73, y = 12, peaceful = 1 })
des.monster({ id = "child", x = 66, y = 8, peaceful = 1, waiting = 1 })
des.monster({ id = "child", x = 68, y = 8, peaceful = 1, waiting = 1 })
des.monster({ id = "child", x = 70, y = 8, peaceful = 1, waiting = 1 })
des.monster({ id = "child", x = 66, y = 12, peaceful = 1, waiting = 1 })
des.monster({ id = "child", x = 68, y = 12, peaceful = 1, waiting = 1 })
des.monster({ id = "child", x = 70, y = 12, peaceful = 1, waiting = 1 })
-- Eight jobs. Prisoners, not shopkeepers: the work is #chat, not a shop.
-- The two chests in the gifted house start empty. Their contents are the
-- level's own objects, so a later visit loads them and does not refill them.
des.monster({ id = "prisoner", name = "baker", x = 18, y = 2,
              peaceful = 1, waiting = 1 })
des.monster({ id = "prisoner", name = "blacksmith", x = 28, y = 2,
              peaceful = 1, waiting = 1 })
des.monster({ id = "prisoner", name = "priest", x = 18, y = 8,
              peaceful = 1, waiting = 1 })
des.monster({ id = "prisoner", name = "tailor", x = 31, y = 14,
              peaceful = 1, waiting = 1 })
des.monster({ id = "prisoner", name = "farmer", x = 18, y = 16,
              peaceful = 1, waiting = 1 })
des.monster({ id = "prisoner", name = "carpenter", x = 25, y = 15,
              peaceful = 1, waiting = 1 })
des.monster({ id = "prisoner", name = "scribe", x = 31, y = 18,
              peaceful = 1, waiting = 1 })
des.monster({ id = "prisoner", name = "steward", x = 30, y = 8,
              peaceful = 1, waiting = 1 })
des.object({ id = "chest", x = 32, y = 7, locked = 0, trapped = 0 })
des.object({ id = "chest", x = 32, y = 9, locked = 0, trapped = 0 })
-- Spare Key to the Sanctum, in the northeast tower of the town.
-- Moloch may already have dropped the other during the battle.
des.object({ id = "chest", x = 73, y = 5, locked = 0, trapped = 0,
             contents = function()
                des.object("Key to the Sanctum")
             end })
