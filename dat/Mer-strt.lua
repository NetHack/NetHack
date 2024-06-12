des.level_init({ style = "solidfill", fg = " " });
des.level_flags("mazelevel", "noteleport", "hardfloor")
des.map([[
.T.....T.|------------------------------------------------------------------
.........|..................|.......|..........|.......|.......|...........|
.T.....T.|..................|.......|....PP....|.......|.......|...........|
.........|..................|.......|....PP....|.......|.......|...........|
.T.....T.|..................|---+---|..........----+----.......|...........|
.........|---------+---------.......|......................{...|...........|
.T.....T.|......|................T..|..........................|----+------|
.........|......|.....--------......-----------+----------------.......|...|
.T.....T.|......|..T..|......|.........................................+...|
.........|......|.....|......|...T...T...T...T...T...TT.T....T...T.....|...|
.T.....T.|......+.....|......|........................PPPPPPPPPPPT.....----|
.........|-------------......|...T.TT...T.TT..TT.TTT..PPPPPPPPPPP......|...|
.T.....T.|...|..|.....|......|...TPPPPPPPPPPPPPP.PPPPPPPPT.TTT.TTT.....+...|
.........|...|..|.....|+------....P..PPPP.T.PPPP..PPPPP..T.............|...|
.T.....T.|...|..+.....|...........PPPPPPPPPPPPPPP.PPPTT........-----+------|
.........|--+-..|.....|.----+---.TTT...TTT...TTTT....T...T...T.|...........|
.T.....T.|......|.....|.|......|...............................|...........|
.........|..T...-------.|......|.T...T...T...T...T...T...T...T.|...........|
........................|......|...............................|...........|
.T.....T.|------------------------------------------------------------------
]]);

-- Dungeon Description
des.region(selection.area(00,00,75,19), "lit")

-- Portal arrival point
des.terrain({04,00}, ".")
des.levregion({ region = {04,00,04,00}, type="branch" })
-- Stairs
if d(4) > 2 then
   des.stair("down", 73,08)
else
   des.stair("down", 73,12)
end
-- Doors
des.door("closed",12,15)
des.door("closed",16,10)
des.door("closed",16,14)
des.door("closed",19,05)
des.door("closed",23,13)
des.door("closed",28,15)
des.door("closed",32,04)
des.door("locked",47,07)
des.door("locked",51,04)
des.door("closed",68,06)
des.door("closed",68,14)
des.door("closed",71,08)
des.door("closed",71,12)
-- Special rooms
des.region({ region={10,01, 27,04}, lit=1, type="shop", filled=1 })
des.region({ region={17,12, 21,16}, lit=1, type="temple", filled=1 })
des.region({ region={29,01, 35,03}, lit=1, type="beehive", filled=1 })
des.region({ region={64,01, 74,05}, lit=1, type="shop", filled=1 })
des.region({ region={64,15, 74,18}, lit=1, type="morgue", filled=1 })
-- Temple altar
des.altar({ x=19, y=14, align="chaos", type="shrine" })
-- Pasion's office
des.monster({ id = "Pasion", coord = {51, 02}, inventory = function()
   des.object({ id = "skeleton key"});
end })
des.monster("trader", 50, 02)
des.monster("trader", 52, 02)
des.monster("trader", 51, 03)
des.monster("trader", 51, 01)
-- Pasion's yard
local yardlocs = selection.floodfill(51,05);
for i = 1,8 do
   des.monster("trader", yardlocs:rndcoord(1))
end
-- Undiggable
des.non_diggable(selection.area(00,00,75,19))

-- the Road
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