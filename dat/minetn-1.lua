-- Capernaum, a warm fishing town. Replaces Minetown.
-- Peter stands south of the door of his house. The closet behind that door
-- holds the Breastplate of Righteousness.
des.level_init({ style = "solidfill", fg = " " })
des.level_flags("mazelevel", "nomongen", "noflip", "noautosearch")

des.map([[
##########################################
#........................................#
#........................................#
#.#######..#######..########.............#
#.#.....#..#.....#..#......#...#######...#
#.#.....+..+.....#..#......#...#.....#...#
#.#.....#..#.....#..#......#...+.....#...#
#.#######..#######..###+####...#.....#...#
#..............................#######...#
#.....{.....}}}..........................#
#............}...........................#
#.........................#######........#
#.........................#.....#........#
#.........................#.....#........#
#.........................###+###........#
#........................................#
#........................................#
##########################################
]])

des.region(selection.area(1,1,40,16), "lit")
des.stair("up", 2, 1)
des.stair("down", 36, 16)

-- food shop, general store, temple, a fisher's house
des.region({ region={3,4, 7,6}, lit=1, type="food shop", filled=1 })
des.region({ region={12,4, 16,6}, lit=1, type="shop", filled=1 })
des.region({ region={21,4, 26,6}, lit=1, type="temple", filled=1 })
des.altar({ x=23, y=5, align="law", type="shrine" })
des.region({ region={32,5, 36,7}, lit=1, type="tool shop", filled=1 })

des.door("closed", 8, 5)
des.door("closed", 11, 5)
des.door("closed", 23, 7)
des.door("closed", 31, 6)
des.door("closed", 29, 14)

des.non_diggable(selection.area(26,11, 32,14))
des.object({ id="chest", x=29, y=12, locked=0, trapped=0,
             contents = function()
                des.object({ id="gleaming armor", name="Breastplate of Righteousness",
                             spe=2, buc="blessed" })
             end })
des.monster({ id="human", name="Peter the Fisherman", peaceful=1,
              waiting=1, female=0, x=29, y=15 })

des.monster({ id="dog", peaceful=1, x=4, y=9 })
des.monster({ id="dog", peaceful=1, x=18, y=2 })
des.monster({ id="cat", peaceful=1, x=8, y=10 })
des.monster({ id="cat", peaceful=1, x=34, y=2 })
des.monster({ id="human", peaceful=1, x=15, y=9 })
des.monster({ id="human", peaceful=1, x=22, y=9 })
des.monster({ id="human", peaceful=1, x=33, y=2 })
des.monster({ id="human", peaceful=1, x=10, y=15 })
des.monster({ id="human", peaceful=1, x=20, y=15 })
