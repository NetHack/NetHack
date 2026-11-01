-- Mines' End layout 3: the PETRA puzzle.
-- Letter rows y=3..6, x=3..7. Safe floor is y=7. Real door is x=6,y=2, above the A.
-- The true path is in src/hack.c. Decoy letters off that path are unsafe.
des.level_init({ style = "solidfill", fg = " " })
des.level_flags("mazelevel", "nomongen", "noteleport", "hardfloor", "noflip")

des.map([[
###############
#....#...#....#
##+#+#+####+###
#  .....      #
#  .....      #
#  .....      #
#  .....      #
#  .....      #
#             #
#      .      #
###############
]])

des.region(selection.area(0,0,14,10), "lit")
des.non_diggable(selection.area(0,0,14,10))
des.stair("up", 7, 9)
-- The up stair is the arrival floor at (7,9). Stone blocks every way off it
-- except south. He waits on that square, before the safe row at y=7.
des.monster({ id = "Philistine champion", peaceful = 0, x = 7, y = 10,
              inventory = function()
                 des.object({ id = "long sword", spe = 1, buc = "uncursed" })
                 des.object({ id = "scale mail", buc = "uncursed" })
                 des.object({ id = "helmet", buc = "uncursed" })
                 des.object({ id = "small shield", buc = "uncursed" })
              end })
des.engraving({ x=3, y=3, type="burn", text="O", degrade=false })
des.engraving({ x=4, y=3, type="burn", text="O", degrade=false })
des.engraving({ x=5, y=3, type="burn", text="E", degrade=false })
des.engraving({ x=6, y=3, type="burn", text="A", degrade=false })
des.engraving({ x=7, y=3, type="burn", text="N", degrade=false })
des.engraving({ x=3, y=4, type="burn", text="R", degrade=false })
des.engraving({ x=4, y=4, type="burn", text="E", degrade=false })
des.engraving({ x=5, y=4, type="burn", text="I", degrade=false })
des.engraving({ x=6, y=4, type="burn", text="R", degrade=false })
des.engraving({ x=7, y=4, type="burn", text="P", degrade=false })
des.engraving({ x=3, y=5, type="burn", text="I", degrade=false })
des.engraving({ x=4, y=5, type="burn", text="T", degrade=false })
des.engraving({ x=5, y=5, type="burn", text="E", degrade=false })
des.engraving({ x=6, y=5, type="burn", text="T", degrade=false })
des.engraving({ x=7, y=5, type="burn", text="E", degrade=false })
des.engraving({ x=3, y=6, type="burn", text="L", degrade=false })
des.engraving({ x=4, y=6, type="burn", text="O", degrade=false })
des.engraving({ x=5, y=6, type="burn", text="A", degrade=false })
des.engraving({ x=6, y=6, type="burn", text="T", degrade=false })
des.engraving({ x=7, y=6, type="burn", text="P", degrade=false })
des.engraving({ x=5, y=7, type="burn", text="Upon this rock I will build my church. Walk upon the rock in the tongue of the Greeks.", degrade=false })
des.door("closed", 6, 2)
des.door("closed", 2, 2)
des.door("closed", 4, 2)
des.door("closed", 11, 2)
des.object({ id="Peter's net", x=7, y=1 })
des.object({ id="Favor Stone", x=6, y=1, buc="not-cursed", achievement=1 })
