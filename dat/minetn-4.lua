-- NetHack mines minetn-4.lua	$NHDT-Date: 1652196031 2022/05/10 15:20:31 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.2 $
--	Copyright (c) 1989-95 by Jean-Christophe Collet
--	Copyright (c) 1991-95 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
-- Minetown variant 4 by Kelly Bailey
-- "College Town"

des.room({ type = "ordinary", lit=1, x=3,y=3,
           xalign="center", yalign="center", w=30, h=15,
           contents = function()
              des.feature("fountain", 08,07)
              des.feature("fountain", 18,07)

              des.room({ type = "book shop", lit=1, x=4, y=2, w=3, h=3,
                         contents = function()
                            des.door({ state = "closed", wall="south" })
                         end
              })

              des.room({ type = "ordinary", x=8, y=2, w=2, h=2,
                         contents = function()
                            des.door({ state = "closed", wall="south" })
                         end
              })

              des.room({ type = "temple", lit=1, x=11, y=3, w=5, h=4,
                         contents = function()
                            des.door({ state = "closed", wall="south" })
                            des.altar({ x=2,y=1,align=align[1], type="shrine" })
                            des.monster("gnomish wizard")
                            des.monster("gnomish wizard")
                         end
              })

              des.room({ type = "ordinary", x=19, y=2, w=2, h=2,
                         contents = function()
                            des.door({ state = "closed", wall="south" })
                            des.monster("G")
                         end
              })

              des.room({ type = "candle shop", lit=1, x=22, y=2, w=3, h=3,
                         contents = function()
                            des.door({ state = "closed", wall="south" })
                         end
              })

              des.room({ type = "ordinary", x=26, y=2, w=2, h=2,
                         contents = function()
                            des.door({ state = "locked", wall="east" })
                            des.monster("G")
                         end
              })

              des.room({ type = "tool shop", chance=90, lit=1, x=4,y=10, w=3,h=3,
                         contents = function()
                            des.door({ state = "closed", wall="north" })
                         end
              })

              des.room({ type = "ordinary", x=8, y=11, w=2, h=2,
                         contents = function()
                            des.door({ state = "locked", wall="south" })
                            des.monster("kobold shaman")
                            des.monster("kobold shaman")
                            des.monster("kitten")
                            des.monster("f")
                         end
              })

              des.room({ type = monkfoodshop(), chance=90, lit=1, x=11, y=11, w=3, h=2,
                         contents = function()
                            des.door({ state = "closed", wall="east" })
                         end
              })

              des.room({ type = "ordinary", x=17, y=11, w=2, h=2,
                         contents = function()
                            des.door({ state = "closed", wall="west" })
                         end
              })

              des.room({ type = "ordinary", x=20, y=10, w=2, h=2,
                         contents = function()
                            des.door({ state = "locked", wall="north" })
                            des.monster("G")
                         end
              })

              des.room({ type = "shop", chance=90, lit=1, x=23, y=10, w=3, h=3,
                         contents = function()
                            des.door({ state = "closed", wall="north" })
                         end
              })

              des.monster({ id = "watchman", peaceful = 1 })
              des.monster({ id = "watchman", peaceful = 1 })
              des.monster({ id = "watchman", peaceful = 1 })
              des.monster({ id = "watchman", peaceful = 1 })
              des.monster({ id = "watch captain", peaceful = 1 })
           end
})

des.room({ type = "ordinary",
           contents = function()
              des.stair("up")
           end
})

des.room({ type = "ordinary",
           contents = function()
              des.stair("down")
              des.trap()
              des.monster("gnome")
              des.monster("gnome")
           end
})

des.room({ type = "ordinary",
           contents = function()
              des.monster("dwarf")
           end
})

des.room({ type = "ordinary",
           contents = function()
              des.trap()
              des.monster("gnome")
           end
})

des.random_corridors()
