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

-- First real room, so fixup_special can place its statues.
-- Same purpose as the irregular room in medusa-2.lua (lines
-- 35-37): an ordinary unlit region that registers as a room.
-- This rectangle is the open walk on the shore side of Tyre,
-- map (38,2) through (41,12). Every square is already floor.
-- It is not a house, not the up stair, and not Lot's wife.
-- arrival_room makes it a real room without flooding into the
-- rest of the shore, so the statue box stays on this walk.
des.region({ region = {38, 2, 41, 12}, lit = 0, type = "ordinary", arrival_room = true })
des.region(selection.area(0,0,69,14), "unlit")
des.stair("up", 1, 7)
des.stair("down", 57, 7)
des.object({ id="statue", x=2, y=7, montype="human", female=1, historic=1, name="Lot's wife" })
des.engraving({ x=2, y=7, type="burn", text="But his wife looked back from behind him, and she became a pillar of salt.", degrade=false })
des.monster({ id="Medusa", x=55, y=7, peaceful=0 })
-- Each house rolls its own occupant. percent(50) is an independent
-- coin flip; nothing forces a mix of the two.
local function sodom_occupant(x, y)
   local id = "woman of Gomorrah"
   if percent(50) then
      id = "man of Sodom"
   end
   des.monster({ id = id, peaceful = 1, waiting = 1, x = x, y = y })
end
sodom_occupant(4, 2)
sodom_occupant(11, 2)
sodom_occupant(18, 2)
sodom_occupant(4, 13)
sodom_occupant(11, 13)
sodom_occupant(18, 13)
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
