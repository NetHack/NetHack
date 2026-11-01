-- Mines' End layout 3: the PETRA puzzle.
-- Letter rows y=3..6, x=3..7. Safe floor is y=7. Real door is x=6,y=2, above the A.
-- The champion is in a separate room. The passage to it does not cross the letters.
-- True-path letters use step 1..5. Decoys use step 0. The hint engraving is in the southwest corner of the safe row.
des.level_init({ style = "solidfill", fg = " " })
des.level_flags("mazelevel", "nomongen", "noteleport", "hardfloor", "noflip")

des.map([[
###############                              
#....#...#....#   ########################## 
##+###+####+###   #                        # 
#  .....      #   #                        # 
#  .....      #   #                        # 
#  .....      #   #                        # 
#  .....      #   #                        # 
#  .....      #   #                 .....  # 
#             #   #                 .....  # 
#      .      #   #                 .....  # 
################  #                 .....  # 
               #  #                   +    # 
  ##############  #                   #    # 
  #               #####################    # 
  #                                        # 
  ########################################## 
]])

des.region(selection.area(0,0,14,7), "lit")
des.non_diggable(selection.area(0,0,44,15))
des.stair("up", 7, 9)
-- The champion's room is dark, at (38,9), behind the
-- closed door at (38,11). The passage joins the arrival corridor only
-- at (15,10), then winds under the puzzle, around the outer edge, and
-- back in. It does not touch the letter floor or the safe row.
des.monster({ id = "Philistine Champion", peaceful = 0, asleep = false, x = 38, y = 9,
              inventory = function()
                 des.object({ id = "two-handed sword", spe = 2, buc = "uncursed" })
                 des.object({ id = "bronze plate mail", buc = "uncursed" })
                 des.object({ id = "helmet", buc = "uncursed" })
              end })
des.petra_letter({ x=3, y=3, text="O", step=0 })
des.petra_letter({ x=4, y=3, text="O", step=0 })
des.petra_letter({ x=5, y=3, text="E", step=0 })
des.petra_letter({ x=6, y=3, text="A", step=5 })
des.petra_letter({ x=7, y=3, text="N", step=0 })
des.petra_letter({ x=3, y=4, text="R", step=0 })
des.petra_letter({ x=4, y=4, text="E", step=0 })
des.petra_letter({ x=5, y=4, text="I", step=0 })
des.petra_letter({ x=6, y=4, text="R", step=4 })
des.petra_letter({ x=7, y=4, text="P", step=0 })
des.petra_letter({ x=3, y=5, text="I", step=0 })
des.petra_letter({ x=4, y=5, text="T", step=0 })
des.petra_letter({ x=5, y=5, text="E", step=0 })
des.petra_letter({ x=6, y=5, text="T", step=3 })
des.petra_letter({ x=7, y=5, text="E", step=2 })
des.petra_letter({ x=3, y=6, text="L", step=0 })
des.petra_letter({ x=4, y=6, text="O", step=0 })
des.petra_letter({ x=5, y=6, text="A", step=0 })
des.petra_letter({ x=6, y=6, text="T", step=0 })
des.petra_letter({ x=7, y=6, text="P", step=1 })
des.engraving({ x=3, y=7, type="burn", text="Upon this rock I will build my church. Walk upon the rock in the tongue of the Greeks.", degrade=false })
des.door("closed", 6, 2)
des.door("closed", 2, 2)
des.door("closed", 11, 2)
des.object({ id="loadstone", x=1, y=1 })
des.object({ id="loadstone", x=12, y=1 })
des.door("closed", 38, 11)
des.object({ id="Peter's net", x=7, y=1 })
des.object({ id="Favor Stone", x=6, y=1, buc="not-cursed", achievement=1 })
