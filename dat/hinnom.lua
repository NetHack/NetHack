-- The Battle of Hinnom. Directly below Baalzebub.
des.level_init({ style = "solidfill", fg = " " })

des.level_flags("mazelevel", "noteleport", "hardfloor", "nommap", "noflip")

des.map({ halign = "left", valign = "top", map = [[
-------------------------------------------------------------------------------
|.............|.|.|.........|.....||....|FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF|
|.............|.|.---.-----.|.|.---|....|FLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLF|
|.............+.|.|...|.....|.|...||....|FL-----LLLLLLLLLLLLLLLLLLLLLLL-----LF|
|.............|.|.|.---.-.-------.|--+---FL|...|-----------------------|...|LF|
|.............|...|...|...........|....|.FL|...|......|......|.........|...|LF|
|.............|.|.---.-----------.|..LL|.FL|...|......|K.K.K.|.........|...|LF|
-------+-------.|.....|.........|.|......FL--+--+-----------+------+-----+--LF|
|L......LLLLLL|.------..-------.|.|.L---.FLL|.....|..|..|..|....|.........|LLF|
|L.LLLLLLLLLLL|.|.....|...|...|.|.|......FLL|.....|-+--+--+--+--|.........|LLF|
|L...........L|.|.|.---.|.|.|.---.|......+.L|.....+.............+.......\.|LLF|
|LLLLLLLLLLL.L|.|.|...|.|...|...|........FLL|.....|--+------+---|.........|LLF|
|LLLLLL......L|.|.---.|.-------.|.|--|...FLL|.....|.............|.........|LLF|
-------+-------.|.|.|...|.....|...|..|.L.FL--+-----+------+---------+----+--LF|
|.............|.|.|.-----.---.-----.L.L..FL|...|......|........|.......|...|LF|
|.............|.........|...|...|.|......FL|...|......|........|.......|...|LF|
|.............|.-------.---.---.|.|---+--FL|...|-----------------------|...|LF|
|.............|.|.....|...|.|...|.||....|FL-----LLLLLLLLLLLLLLLLLLLLLLL-----LF|
|.............|.|.-------.|.|.---.||....|FLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLF|
|.............|.|...........|.....||....|FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF|
-------------------------------------------------------------------------------
]] })

des.non_diggable(selection.area(0, 0, 78, 20))
des.region({ region = {1, 1, 13, 6}, lit = 1, type = "ordinary" })
des.region({ region = {1, 14, 13, 19}, lit = 0, type = "ordinary" })
-- Lit rooms: throne room, foyer, lab, lever room, and the ground
-- outside the gate. A lit region also lights its bounding walls.
des.region({ region = {65, 8, 73, 12}, lit = 1, type = "ordinary" })
des.region({ region = {45, 8, 49, 12}, lit = 1, type = "ordinary" })
des.region({ region = {55, 5, 60, 6}, lit = 1, type = "ordinary" })
des.region({ region = {55, 14, 62, 15}, lit = 1, type = "ordinary" })
des.region({ region = {34, 7, 40, 13}, lit = 1, type = "ordinary" })
des.stair("up", 4, 17)
des.stair("down", 69, 15)
des.teleport_region({ region = {1, 14, 13, 19}, dir = "down" })
des.teleport_region({ region = {1, 14, 13, 19}, dir = "up" })
des.drawbridge({ dir = "east", state = "closed", x = 43, y = 10 })
des.altar({ x = 69, y = 10, align = "noalign", type = "altar" })

-- Doors. Portal and arrival are open gaps. Ruins are broken.
des.door("nodoor", 7, 7)
des.door("nodoor", 14, 3)
des.door("nodoor", 7, 13)
des.door("broken", 37, 4)
des.door("broken", 38, 16)
des.door("locked", 41, 10)
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
des.door("locked", 68, 13)

des.trap({ type = "power lever", x = 55, y = 14, seen = 1 })
des.trap({ type = "power lever", x = 57, y = 14, seen = 1 })
des.trap({ type = "power lever", x = 61, y = 14, seen = 1 })
des.trap({ type = "power lever", x = 56, y = 15, seen = 1 })
des.trap({ type = "power lever", x = 60, y = 15, seen = 1 })

des.monster({ id = "Michael the Archangel", x = 7, y = 4, peaceful = 1,
              waiting = 1 })
des.monster({ id = "Angel", x = 5, y = 3, peaceful = 1, waiting = 1 })
des.monster({ id = "Angel", x = 9, y = 3, peaceful = 1, waiting = 1 })
des.monster({ id = "Angel", x = 5, y = 5, peaceful = 1, waiting = 1 })
des.monster({ id = "Angel", x = 9, y = 5, peaceful = 1, waiting = 1 })

des.monster({ id = "fortress guard", x = 40, y = 9, peaceful = 1, waiting = 1,
    inventory = function()
        des.object("Key to Hinnom's Gate")
    end })
des.monster({ id = "fortress guard", x = 40, y = 11, peaceful = 1, waiting = 1 })
des.monster({ id = "fortress guard", x = 47, y = 10, peaceful = 1 })
des.monster({ id = "horned devil", x = 46, y = 8, peaceful = 1 })
des.monster({ id = "horned devil", x = 46, y = 12, peaceful = 1 })
des.monster({ id = "barbed devil", x = 45, y = 5, peaceful = 1 })
des.monster({ id = "barbed devil", x = 73, y = 5, peaceful = 1 })
des.monster({ id = "bone devil", x = 45, y = 15, peaceful = 1 })
des.monster({ id = "ice devil", x = 73, y = 15, peaceful = 1 })
des.monster({ id = "nalfeshnee", x = 56, y = 10, peaceful = 1 })
des.monster({ id = "aligned cleric", x = 54, y = 10, peaceful = 1, align = "noalign" })
des.monster({ id = "aligned cleric", x = 52, y = 12, peaceful = 1, align = "noalign" })
des.monster({ id = "aligned cleric", x = 62, y = 12, peaceful = 1, align = "noalign" })
des.monster({ id = "pit fiend", x = 70, y = 9, peaceful = 1 })
des.monster({ id = "pit fiend", x = 70, y = 11, peaceful = 1 })
des.monster({ id = "Moloch", x = 72, y = 10, peaceful = 1,
    inventory = function()
        des.object("Key to the Sanctum")
    end })
des.monster({ id = "doctor", x = 57, y = 5, peaceful = 1 })
des.monster({ id = "child", x = 62, y = 5, peaceful = 1, waiting = 1 })
des.monster({ id = "child", x = 64, y = 5, peaceful = 1, waiting = 1 })
des.monster({ id = "child", x = 66, y = 5, peaceful = 1, waiting = 1 })
des.monster({ id = "child", x = 68, y = 5, peaceful = 1, waiting = 1 })
des.monster({ id = "child", x = 70, y = 5, peaceful = 1, waiting = 1 })
des.monster({ id = "child", x = 63, y = 6, peaceful = 1, waiting = 1 })

-- Lab holds potions and tools only.
des.object({ id = "booze", x = 56, y = 5 })
des.object({ id = "fruit juice", x = 58, y = 5 })
des.object({ id = "oil", x = 59, y = 6 })
des.object({ id = "sack", x = 56, y = 6 })
des.object({ id = "flint and tinder box", x = 58, y = 6 })
