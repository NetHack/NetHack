-- NetHack mines minetn-3.lua	$NHDT-Date: 1652196030 2022/05/10 15:20:30 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.2 $
--	Copyright (c) 1989-95 by Jean-Christophe Collet
--	Copyright (c) 1991-95 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
-- Minetown variant 3 by Kelly Bailey
-- "Alley Town"

des.room({ type = "ordinary",lit=1,x=3,y=3,
           xalign="center",yalign="center",w=31,h=15,
           contents = function()
              des.feature("fountain", 01,06)
              des.feature("fountain", 29,13)

  des.room({ type = "ordinary",x=2,y=2,w=2,h=2,
             contents = function()
                des.door({ state="closed", wall="south" })
             end
  });

  des.room({ type = "tool shop", chance=30, lit=1,x=5,y=3,w=2,h=3,
             contents = function()
                des.door({ state="closed", wall="south" })
             end
  })

  des.room({ type = "ordinary",x=2,y=10,w=2,h=3,
             contents = function()
                des.door({ state ="locked", wall="north" })
                des.monster("G")
             end
  })

  des.room({ type = "ordinary", x=5,y=9,w=2,h=2,
             contents = function()
                des.door({ state="closed", wall="north" })
             end
  })

  des.room({ type = "temple",lit=1,x=10,y=2,w=3,h=4,
             contents = function()
                des.door({ state="closed", wall="east" })
                des.altar({ x=1, y=1, align = align[1], type="shrine" })
                des.monster("gnomish wizard")
                des.monster("gnomish wizard")
             end
  })

  des.room({ type = "ordinary",x=11,y=7,w=2,h=2,
             contents = function()
                des.door({ state="closed", wall="west" })
             end
  })

  des.room({ type = "shop",lit=1,x=10,y=10,w=3,h=3,
             contents = function()
                des.door({ state="closed", wall="west" })
             end
  })

  des.room({ type = "ordinary",random,x=14,y=8,w=2,h=2,
             contents = function()
                des.door({ state="locked", wall="north" })
                des.monster("G")
             end
  })

  des.room({ type = "ordinary",random,x=14,y=11,w=2,h=2,
             contents = function()
                des.door({ state="closed", wall="south" })
             end
  })

  des.room({ type = "tool shop", chance=40,lit=1,x=17,y=10,w=3,h=3,
             contents = function()
                des.door({ state="closed", wall="north" })
             end
  })

  des.room({ type = "ordinary",x=21,y=11,w=2,h=2,
             contents = function()
                des.door({ state="locked", wall="east" })
                des.monster("G")
             end
  })

  des.room({ type = monkfoodshop(), chance=90,lit=1,x=26,y=8,w=3,h=2,
             contents = function()
                des.door({ state="closed", wall="west" })
             end
  })

  des.room({ type = "ordinary",random,x=16,y=2,w=2,h=2,
             contents = function()
                des.door({ state="closed", wall="west" })
             end
  })

  des.room({ type = "ordinary",random,x=19,y=2,w=2,h=2,
             contents = function()
                des.door({ state="closed", wall="north" })
             end
  })

  des.room({ type = "wand shop", chance=30,lit=1,x=19,y=5,w=3,h=2,
             contents = function()
                des.door({ state="closed", wall="west" })
             end
  })

  des.room({ type = "candle shop",lit=1,x=25,y=2,w=3,h=3,
             contents = function()
                des.door({ state="closed", wall="south" })
             end
  })

  des.monster({ id = "watchman", peaceful = 1 })
  des.monster({ id = "watchman", peaceful = 1 })
  des.monster({ id = "watchman", peaceful = 1 })
  des.monster({ id = "watchman", peaceful = 1 })
  des.monster({ id = "watch captain", peaceful = 1 })

           end
})

des.room({ type = "ordinary", contents = function()
              des.stair("up")
                                         end
})

des.room({ type = "ordinary", contents = function()
              des.stair("down")
              des.trap()
              des.monster("gnome")
              des.monster("gnome")
                                         end
})

des.room({ type = "ordinary", contents = function()
              des.monster("dwarf")
                                          end
})

des.room({ type = "ordinary", contents = function()
              des.trap()
              des.monster("gnome")
                                         end
})

des.random_corridors()
