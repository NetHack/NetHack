-- Cave of Gibeon. One level above the Oracle.
-- The tunnel is dark corridor. The cave is lit. No random monsters.
des.level_init({ style = "solidfill", fg = " " })
des.level_flags("mazelevel", "corrmaze", "nomongen", "noflip", "noteleport", "hardfloor")

des.map({ x = 1, y = 0, map = [[
                                                                  
                                                                  
 ###############       ------------------------------------------ 
               #       |........................................| 
               #       |.................LLLLLLLLLLLLL..........| 
   #############       |.................L}}}}}}}}}}}L..........| 
   #                   |.................L}}}}}}}}}}}L..........| 
   #                   |.................L}}.......}}L..........| 
   #################   |.................L}}.......}}L..........| 
                   #   |.................L}}.......}}L..........| 
                   ####............................}}L..........| 
                       |.................L}}.......}}L..........| 
                       |.................L}}.......}}L..........| 
                       |.................L}}.......}}L..........| 
                       |.................L}}}}}}}}}}}L..........| 
                       |.................L}}}}}}}}}}}L..........| 
                       ------------------------------------------ 
                                                                  
]] })

des.region(selection.area(24, 2, 65, 16), "lit")
des.levregion({ region = {2, 2, 2, 2}, type = "branch" })
des.altar({ x = 48, y = 10, align = "law", type = "altar" })
des.object({ id = "chest", x = 46, y = 10, locked = 0, trapped = 0,
             contents = function()
                des.object({ id = "burnt offering", quantity = 1 })
             end })
des.monster({ id = "human", name = "King Solomon", peaceful = 1,
              waiting = 1, female = 0, x = 48, y = 9 })
