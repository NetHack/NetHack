-- Wilderness Trial 1: the desert. Two-way. No random monsters.
des.level_init({ style = "solidfill", fg = " " })
des.level_flags("mazelevel", "nomongen", "noflip", "hardfloor")

des.map([[
######################
#<...................#
#....................#
#....................#
#...................>#
#....................#
######################
]])
des.region(selection.area(1,1,20,5), "lit")
des.stair("up", 1, 1)
des.stair("down", 20, 4)
des.engraving({ x=2, y=1, type="burn", degrade=false, text="But he answered and said, It is written, Man shall not live by bread alone, but by every word that proceedeth out of the mouth of God." })
des.object({ id="food ration", x=4, y=2, name="loaf of bread" })
des.object({ id="food ration", x=8, y=3, name="loaf of bread" })
des.object({ id="food ration", x=12, y=2, name="loaf of bread" })
des.object({ id="food ration", x=6, y=4, name="loaf of bread" })
des.monster({ id="cobra", x=18, y=3, peaceful=0 })
des.monster({ id="soldier ant", x=17, y=4, peaceful=0 })
des.monster({ id="scorpion", x=19, y=2, peaceful=0 })
