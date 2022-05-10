-- NetHack mines minetn-7.lua	$NHDT-Date: 1652196032 2022/05/10 15:20:32 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.3 $
--	Copyright (c) 1989-95 by Jean-Christophe Collet
--	Copyright (c) 1991-95 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
-- "Bazaar Town" by Kelly Bailey

des.room({ type="ordinary", lit=1, x=3,y=3,
           xalign="center",yalign="center", w=30,h=15,
           contents = function()
              des.feature("fountain", 12, 07)
              des.feature("fountain", 11, 13)

              if percent(75) then
                 des.room({ type="ordinary", x=2,y=2, w=4,h=2,
                            contents = function()
                               des.door({ state = "closed", wall="south" })
                            end
                 })
              end

              if percent(75) then
                 des.room({ type="ordinary", x=7,y=2, w=2,h=2,
                            contents = function()
                               des.door({ state = "closed", wall="north" })
                            end
                 })
              end

              if percent(75) then
                 des.room({ type="ordinary", x=7,y=5, w=2,h=2,
                            contents = function()
                               des.door({ state = "closed", wall="south" })
                            end
                 })
              end

              if percent(75) then
                 des.room({ type="ordinary", lit=1, x=10,y=2, w=3,h=4,
                            contents = function()
                               des.monster("gnome")
                               des.monster("monkey")
                               des.monster("monkey")
                               des.monster("monkey")
                               des.door({ state = "closed", wall="south" })
                            end
                 })
              end

              if percent(75) then
                 des.room({ type="ordinary", x=14,y=2, w=4,h=2,
                            contents = function()
                               des.door({ state = "closed", wall="south", pos=0 })
                               des.monster("n")
                            end
                 })
              end

              if percent(75) then
                 des.room({ type="ordinary", x=16,y=5, w=2,h=2,
                            contents = function()
                               des.door({ state = "closed", wall="south" })
                            end
                 })
              end

              if percent(75) then
                 des.room({ type="ordinary", lit=0, x=19,y=2, w=2,h=2,
                            contents = function()
                               des.door({ state = "locked", wall="east" })
                               des.monster("gnome king")
                            end
                 })
              end

              des.room({ type=monkfoodshop(), chance=50, lit=1, x=19,y=5, w=2,h=3,
                         contents = function()
                            des.door({ state = "closed", wall="south" })
                         end
              })

              if percent(75) then
                 des.room({ type="ordinary", x=2,y=7, w=2,h=2,
                            contents = function()
                               des.door({ state = "closed", wall="east" })
                            end
                 })
              end

              des.room({ type="tool shop", chance=50, lit=1, x=2,y=10, w=2,h=3,
                         contents = function()
                            des.door({ state = "closed", wall="south" })
                         end
              })

              des.room({ type="candle shop", lit=1, x=5,y=10, w=3,h=3,
                         contents = function()
                            des.door({ state = "closed", wall="north" })
                         end
              })

              if percent(75) then
                 des.room({ type="ordinary", x=11,y=10, w=2,h=2,
                            contents = function()
                               des.door({ state = "locked", wall="west" })
                               des.monster("G")
                            end
                 })
              end

              des.room({ type="shop", chance=60, lit=1, x=14,y=10, w=2,h=3,
                         contents = function()
                            des.door({ state = "closed", wall="north" })
                         end
              })

              if percent(75) then
                 des.room({ type="ordinary", x=17,y=11, w=4,h=2,
                            contents = function()
                               des.door({ state = "closed", wall="north" })
                            end
                 })
              end

              if percent(75) then
                 des.room({ type="ordinary", x=22,y=11, w=2,h=2,
                            contents = function()
                               des.door({ state = "closed", wall="south" })
                               des.feature("sink", 00,00)
                            end
                 })
              end

              des.room({ type=monkfoodshop(), chance=50, lit=1, x=25,y=11, w=3,h=2,
                         contents = function()
                            des.door({ state = "closed", wall="east" })
                         end
              })

              des.room({ type="tool shop", chance=30, lit=1, x=25,y=2, w=3,h=3,
                         contents = function()
                            des.door({ state = "closed", wall="west" })
                         end
              })

              des.room({ type="temple", lit=1, x=24,y=6, w=4,h=4,
                         contents = function()
                            des.door({ state = "closed", wall = "west" })
                            des.altar({ x=02, y=01, align=align[1], type="shrine" })
                            des.monster("gnomish wizard")
                            des.monster("gnomish wizard")
                         end
              })

              des.monster({ id = "watchman", peaceful = 1 })
              des.monster({ id = "watchman", peaceful = 1 })
              des.monster({ id = "watchman", peaceful = 1 })
              des.monster({ id = "watchman", peaceful = 1 })
              des.monster({ id = "watch captain", peaceful = 1 })
              des.monster("gnome")
              des.monster("gnome")
              des.monster("gnome")
              des.monster("gnome lord")
              des.monster("monkey")
              des.monster("monkey")

           end
})

des.room({ type="ordinary",
           contents = function()
              des.stair("up")
           end
})

des.room({ type="ordinary",
           contents = function()
              des.stair("down")
              des.trap()
              des.monster("gnome")
              des.monster("gnome")
           end
})

des.room({ type="ordinary",
           contents = function()
              des.monster("dwarf")
           end
})

des.room({ type="ordinary",
           contents = function()
              des.trap()
              des.monster("gnome")
           end
})

des.random_corridors()
