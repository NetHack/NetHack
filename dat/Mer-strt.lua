des.level_init({ style = "solidfill", fg = " " });
des.level_flags("mazelevel", "noteleport", "hardfloor")
des.map([[
TT.....TTTT------------------TTTTTTT---------------------------|TTTTTTTTTTTT
TT.....TTTT|................|TTTTTTT|..........|.......|.......|TTTTTTTTTTTT
TT.....TTTT|................|TTTTTTT|....PP....|.......|.......|TTTTTTTTTTTT
TT.....TTTT|................|TTTTTTT|....PP....|.......|.......|TTTTTTTTTTTT
TT.....TTTT|................|TTTTTTT|..........----+----.......|TTTTTTTTTTTT
TT.....TTTT--------+---------TTTTTTT|......................{...|TTTTTTTTTTTT
TT.....TTTTTTTTTTTT.TTTTTTTTTTTTTTTT|..........................|TTTTTTTTTTTT
TT.....TTTTTTTTTTTT...TTTTTTTTTTTTTT-----------+----------------TTTTTTTTTTTT
TT.....TTTTTTTTTTTTTT....TTTTTTTTTTTTTTTTT......TTTTTTTTTTTTTTTTTT..........
TT.....TTTTTTTTTTTTTTTT.....TTTTTTTTT........TTTTTTTTTTTTTT.........TTTTTTTT
TT.....TTTTTTTTTTTTTTTTTTT..............TTTTTTTTTTTTTT......TTTTTTTTTTTTTTTT
TT.....TTTTTTTTTTTTTTTTTTTTTTTTTTTT........TTTTTTT.....TTTT.........TTTTTTTT
TT.....TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT.............TTTTTTTTTTTTTTTT..TTTTTTT
TT.....TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT.....TTTTTTTTTTTTTTTTTTTTT.TTTTTTT
TT.....TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT.....TTTTTTTT}TTTTTTTTTTT-----+-----TT
TT.....TTTTTTTTTTTTTTTTTTTTTTTTTT......TTTTTTTTTTT}}}TTTTTTTTTT|.........|TT
TT.....TTTTTTTTTTTTTTTTTTTTT......TTTTTTTTTTTTTTT}}}}}TTTTTTTTT|.........|TT
TT.....TTTTTTTTTTTTTTTT......TTTTTTTTTTTTTTTTTTTTT}}}TTTTTTTTTT|.........|TT
TT......................TTTTTTTTTTTTTTTTTTTTTTTTTTT}TTTTTTTTTTT-----------TT
TT.....TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT
]]);

-- Dungeon Description
des.region(selection.area(00,00,75,19), "lit")
des.replace_terrain({ region={00,00, 75,19}, fromterrain="T", toterrain=".", chance=20 })

-- Portal arrival point
des.terrain({04,00}, ".")
des.levregion({ region = {04,00,04,00}, type="branch" })
-- Stairs
des.stair("down", 75,08)
-- Doors
des.door("locked",47,07)
des.door("locked",51,04)
-- Shops
des.region({ region={12,01, 27,04}, lit=1, type="shop", filled=1 })
des.region({ region={64,15, 72,17}, lit=1, type="shop", filled=1 })
-- Pasion
des.monster({ id = "Pasion", coord = {51, 02}, inventory = function()
   des.object({ id = "skeleton key"});
end })
-- guards for Pasion's office
des.monster("trader", 50, 02)
des.monster("trader", 52, 02)
des.monster("trader", 51, 03)
des.monster("trader", 51, 01)
-- guards for the yard
local yardlocs = selection.floodfill(51,05);
for i = 1,8 do
   des.monster("trader", yardlocs:rndcoord(1))
end

-- the middle of town
des.object({id = "statue", montype="trader", material="gold", x = 38, y = 10})

-- the Appian Way
for i = 1,9 do
   local y = (i*2) + 1
   local road_roll = d(4)
   if road_roll == 1 or road_roll == 2 then
      des.object({id = "statue", x = 2, y = y})
      des.object({id = "statue", x = 6, y = y})
   else
      des.monster({id = "giant mimic", appear_as = "obj:statue", x = 2, y = y})
      des.monster({id = "giant mimic", appear_as = "obj:statue", x = 6, y = y})
   end
end
local roadlocs = selection.area(3,4,5,19)
for i = 1,4 do
   des.monster("gold golem", roadlocs:rndcoord(1))
   des.monster("'", roadlocs:rndcoord(1))
   des.trap("magic", roadlocs:rndcoord(1))
end
des.object("!", roadlocs:rndcoord(1))
des.object("!", roadlocs:rndcoord(1))
des.object("!", roadlocs:rndcoord(1))
des.object("!", roadlocs:rndcoord(1))
des.object("=", roadlocs:rndcoord(1))
des.object("%", roadlocs:rndcoord(1))
des.object("?", roadlocs:rndcoord(1))
des.object("?", roadlocs:rndcoord(1))