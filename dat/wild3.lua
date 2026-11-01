-- Wilderness Trial 3: Pride. One-way. Lures are real on this level.
des.level_init({ style = "solidfill", fg = " " })
des.level_flags("mazelevel", "nomongen", "noflip", "hardfloor", "noteleport")

des.map([[
############################################################
#<........................................................>#
#..######..######..######..######..######..######..######..#
#..#....#..#....#..#....#..#....#..#....#..#....#..#....#..#
#..#....+..+....+..+....+..+....+..+....+..+....+..+....+..#
#..#....#..#....#..#....#..#....#..#....#..#....#..#....#..#
#..######..######..######..######..######..######..######..#
#..........................................................#
#..........................#...............................#
############################################################
]])
des.region(selection.area(1,1,58,8), "lit")
des.stair("up", 1, 1)
des.stair("down", 58, 1)
des.engraving({ x=30, y=7, type="engrave", degrade=false, text="Humble yourselves in the sight of the Lord, and he shall lift you up." })
des.object({ id="mirror", x=5, y=4 })
des.object({ id="mirror", x=13, y=4 })
des.object({ id="mirror", x=21, y=4 })
des.object({ id="mirror", x=29, y=4 })
des.object({ id="mirror", x=37, y=4 })
des.object({ id="mirror", x=45, y=4 })
des.object({ id="mirror", x=53, y=4 })
des.object({ id="mirror", x=8, y=7 })
-- eight lures, one in each room. Rooms at x 3-6, 11-14, 19-22, 27-30, 35-38, 43-46, 51-54, and the eighth on the hall is wrong.
-- Put the eighth lure in the last room; only 7 rooms in the row. Add the bag in room 1 and shift.
des.object({ id="long sword", x=4, y=3, spe=12, buc="blessed", name="silver long sword" })
des.object({ id="gray dragon scale mail", x=12, y=3, spe=12, buc="blessed" })
des.object({ id="ring of protection", x=20, y=3, name="ring of complete immunity" })
des.object({ id="amulet of life saving", x=28, y=3, name="Amulet of Immortality" })
des.object({ id="wand of cold", x=36, y=3, name="Rod of Frozen Power" })
des.object({ id="finger of execution", x=44, y=3, name="prophecy book of execution" })
des.object({ id="magic lamp", x=52, y=3, name="magic lamp of unlimited wishing" })
des.object({ id="bag of holding", x=4, y=5, name="bag of infinite holding" })
