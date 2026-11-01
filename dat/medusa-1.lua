-- Sodom and Tyre. Medusa's level. Pitch black. Map from the design plan.
-- House doors start open. Men of Sodom stand inside. Tyre's gold is real.
des.level_init({ style = "solidfill", fg = " " })
des.level_flags("mazelevel", "nomongen", "noflip", "noteleport", "hardfloor")

des.map([[
                        }}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
  ------ ------ ------  }}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
  |....| |....| |....|  }}}}}}}}}}}}}}............................}}}}
  |....| |....| |....|  }}}}}}}}}}}}}}....|------------------|....}}}}
  |-+--| |-+--| |-+--|  }}}}}}}}}}}}}}....|....|.............|....}}}}
    .      .      .     }}}}}}}}}}}}}}....|---.|.....-+-----.|....}}}}
 ......  ......  ...... }}}}}}}}}}}}}}....|..|.|.....|.....|.|....}}}}
 ..   .  .    .  .   ...}}}}}}}}}}}}}}....+..|...|...|.....|.|....}}}}
 .    ....    ....   .  }}}}}}}}}}}}}}....|..|.|.---.|.....|.|....}}}}
 .......  .....  ...... }}}}}}}}}}}}}}....|..|.|.....-------.|....}}}}
    .      .      .     }}}}}}}}}}}}}}....|....|.............|....}}}}
  |-+--| |-+--| |-+--|  }}}}}}}}}}}}}}....|------------------|....}}}}
  |....| |....| |....|  }}}}}}}}}}}}}}............................}}}}
  |....| |....| |....|  }}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
  ------ ------ ------  }}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
]])

des.region(selection.area(0,0,69,14), "unlit")
des.stair("up", 1, 7)
des.stair("down", 57, 7)
des.object({ id="statue", x=2, y=7, montype="human", female=1, historic=1, name="Lot's wife" })
des.engraving({ x=2, y=7, type="burn", text="But his wife looked back from behind him, and she became a pillar of salt.", degrade=false })
des.monster({ id="Medusa", x=55, y=7, peaceful=0 })
des.monster({ id="human", name="man of Sodom", peaceful=1, waiting=1, female=0, x=4, y=2 })
des.monster({ id="human", name="man of Sodom", peaceful=1, waiting=1, female=0, x=11, y=2 })
des.monster({ id="human", name="man of Sodom", peaceful=1, waiting=1, female=0, x=18, y=2 })
des.monster({ id="human", name="man of Sodom", peaceful=1, waiting=1, female=0, x=4, y=13 })
des.monster({ id="human", name="man of Sodom", peaceful=1, waiting=1, female=0, x=11, y=13 })
des.monster({ id="human", name="man of Sodom", peaceful=1, waiting=1, female=0, x=18, y=13 })
des.gold({ x=45, y=4, amount=200 })
des.gold({ x=55, y=4, amount=200 })
des.gold({ x=49, y=6, amount=200 })
des.gold({ x=44, y=8, amount=200 })
des.gold({ x=50, y=9, amount=200 })
des.gold({ x=52, y=10, amount=200 })
des.door("open", 4, 4)
des.door("open", 11, 4)
des.door("open", 18, 4)
des.door("open", 54, 5)
des.door("open", 42, 7)
des.door("open", 4, 11)
des.door("open", 11, 11)
des.door("open", 18, 11)
