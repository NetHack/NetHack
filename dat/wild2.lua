-- Wilderness Trial 2: Greed. One-way from here on.
-- The gate square is (12,3). Drop side is x<12. Restore square is (16,3).
des.level_init({ style = "solidfill", fg = " " })
des.level_flags("mazelevel", "nomongen", "noflip", "hardfloor", "noteleport")

des.map([[
########################################
#...........#..........................#
#...........#..........................#
#...........+..........................#
#...........#..........................#
#...#.#.#...#..........................#
#...#.#.#...#..........................#
###+#####+###..........................#
#.....#.....#..........................#
#.....#.....#..........................#
#.....#.....#..........................#
#.....#.....#..........................#
#.....#.....#..........................#
########################################
]])
des.region(selection.area(0,0,39,13), "lit")
des.non_diggable(selection.area(0,0,39,13))
des.stair("up", 1, 1)
des.stair("down", 30, 5)
des.door("closed", 12, 3)
des.door("closed", 3, 7)
des.door("closed", 9, 7)
des.engraving({ x=2, y=2, type="burn", degrade=false, text="For what is a man profited, if he shall gain the whole world, and lose his own soul? or what shall a man give in exchange for his soul?" })
des.engraving({ x=11, y=3, type="burn", degrade=false, text="And said, Naked came I out of my mother's womb, and naked shall I return thither: the LORD gave, and the LORD hath taken away; blessed be the name of the LORD." })
des.object({ id="diamond", x=3, y=9, name="exquisite flawless diamond" })
des.object({ id="emerald", x=3, y=11, name="flawless emerald" })
des.object({ id="ruby", x=9, y=9, name="flawless ruby" })
des.object({ id="sapphire", x=9, y=11, name="flawless sapphire" })
