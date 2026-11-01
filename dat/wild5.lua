-- Wilderness Trial 5: the garden. The Sword of the Spirit is on the altar.
des.level_init({ style = "solidfill", fg = " " })
des.level_flags("mazelevel", "nomongen", "noflip", "hardfloor", "noteleport")

des.map([[
###############
#.............#
#.............#
#..{.......{..#
#......_......#
#..{.......{..#
#.............#
#.............#
###############
]])
des.region(selection.area(1,1,13,7), "lit")
des.feature({ x=1, y=1, type="tree" })
des.feature({ x=13, y=1, type="tree" })
des.feature({ x=1, y=7, type="tree" })
des.feature({ x=13, y=7, type="tree" })
des.stair("up", 7, 1)
des.feature("fountain", 3, 3)
des.feature("fountain", 11, 3)
des.feature("fountain", 3, 5)
des.feature("fountain", 11, 5)
des.altar({ x=7, y=4, align="law", type="altar" })
des.engraving({ x=7, y=4, type="burn", degrade=false, text="And take the helmet of salvation, and the sword of the Spirit, which is the word of God:" })
des.object({ id="doubled-edged blade", x=7, y=4, spe=0, buc="blessed", name="Sword of the Spirit" })
