-- NetHack 3.7	mines.des	$NHDT-Date: 1432512783 2015/05/25 00:13:03 $  $NHDT-Branch: master $:$NHDT-Revision: 1.25 $
--	Copyright (c) 1989-95 by Jean-Christophe Collet
--	Copyright (c) 1991-95 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
-- A tragic accident has occurred in Frontier Town....
--
-- Minetown variant 1
-- Orcish Town - a variant of Frontier Town that has been
-- overrun by orcs.  Note the barricades (iron bars).

des.level_flags("mazelevel")

des.level_init({ style="mines", fg=".", bg=" ", smoothed=true, joined=true, walled=true })

des.map([[
.....................................
.----------------F------------------.
.|.................................|.
.|.-------------......------------.|.
.|.|...|...|...|......|..|...|...|.|.
.F.|...|...|...|......|..|...|...|.|.
.|.|...|...|...|......|..|...|...|.F.
.|.|...|...|----......------------.|.
.|.---------.......................|.
.|.................................|.
.|.---------.....--...--...........|.
.|.|...|...|----.|.....|.---------.|.
.|.|...|...|...|.|.....|.|..|....|.|.
.|.|...|...|...|.|.....|.|..|....|.|.
.|.|...|...|...|.|.....|.|..|....|.|.
.|.-------------.-------.---------.|.
.|.................................F.
.-----------F------------F----------.
.....................................
]]);

-- Don't let the player fall into his likely death; used to explicitly exclude
-- the town, but that meant that you couldn't teleport out as well as not in.
des.teleport_region({ region={01,01,20,19}, region_islev=1 })
des.region(selection.area(01,01,35,17), "lit")
des.levregion({ type="stair-up", region={01,03,20,19}, region_islev=1,
		exclude={00,01,36,17} });
des.levregion({ type="stair-down", region={61,03,75,19}, region_islev=1,
		exclude={00,01,36,17} })

-- shame we can't make polluted fountains
des.feature("fountain",16,09)
des.feature("fountain",25,09)

-- the altar's defiled; useful for BUC but never coaligned
des.altar({ x=20,y=13,align="noalign", type="shrine" })

-- set up the shop doors; could be broken down
des.door("random",5,8)
des.door("random",9,8)
des.door("random",13,7)
des.door("random",22,5)
des.door("random",27,7)
des.door("random",31,7)
des.door("random",5,10)
des.door("random",9,10)
des.door("random",15,13)
des.door("random",25,13)
des.door("random",31,11)

-- knock a few holes in the shop interior walls
des.replace_terrain({ region={07,04,11,06}, fromterrain="|", toterrain=".", chance=18 })
des.replace_terrain({ region={25,04,29,06}, fromterrain="|", toterrain=".", chance=18 })
des.replace_terrain({ region={07,12,11,14}, fromterrain="|", toterrain=".", chance=18 })
des.replace_terrain({ region={28,12,28,14}, fromterrain="|", toterrain=".", chance=33 })

-- One spot each in most shops...
local place = { {05,04},{09,05},{13,04},{26,04},{31,05},{30,14},{05,14},{10,13},{26,14},{27,13} }
shuffle(place);

-- scatter some bodies
des.object({ id = "corpse", x=20,y=12, montype="aligned cleric" })
des.object({ id = "corpse", coord = place[1], montype="shopkeeper" })
des.object({ id = "corpse", coord = place[2], montype="shopkeeper" })
des.object({ id = "corpse", coord = place[3], montype="shopkeeper" })
des.object({ id = "corpse", coord = place[4], montype="shopkeeper" })
des.object({ id = "corpse", coord = place[5], montype="shopkeeper" })
des.object({ id = "corpse", montype="watchman" })
des.object({ id = "corpse", montype="watchman" })
des.object({ id = "corpse", montype="watchman" })
des.object({ id = "corpse", montype="watchman" })
des.object({ id = "corpse", montype="watch captain" })

-- Rubble!
for i=1,9 + math.random(2 - 1,2*5) do
  if percent(90) then
    des.object("boulder")
  end
  des.object("rock")
end

-- Guarantee 7 candles since we won't have Izchak available
des.object({ id = "wax candle", coord = place[4], quantity = math.random(1,2) })

des.object({ id = "wax candle", coord = place[1], quantity = math.random(2,4) })
des.object({ id = "wax candle", coord = place[2], quantity = math.random(1,2) })
des.object({ id = "tallow candle", coord = place[3], quantity = math.random(1,3) })
des.object({ id = "tallow candle", coord = place[2], quantity = math.random(1,2) })
des.object({ id = "tallow candle", coord = place[4], quantity = math.random(1,2) })

-- go ahead and leave a lamp next to one corpse to be suggestive
-- and some empty wands...
des.object("oil lamp",place[2])
des.object({ id = "wand of striking", coord = place[1], buc="uncursed", spe=0 })
des.object({ id = "wand of striking", coord = place[3], buc="uncursed", spe=0 })
des.object({ id = "wand of striking", coord = place[4], buc="uncursed", spe=0 })
des.object({ id = "wand of magic missile", coord = place[4], buc="uncursed", spe=0 })
des.object({ id = "wand of magic missile", coord = place[5], buc="uncursed", spe=0 })

-- the Orcish Army

local inside = selection.floodfill(18,8)
local near_temple = selection.area(17,8, 23,14) & inside

for i=1,5 + math.random(1 - 1,1*10) do
   if percent(50) then
      des.monster({ id = "orc-captain", coord = inside:rndcoord(1), peaceful=0 });
   else
      if percent(80) then
         des.monster({ id = "Uruk-hai", coord = inside:rndcoord(1), peaceful=0 })
      else
         des.monster({ id = "Mordor orc", coord = inside:rndcoord(1), peaceful=0 })
      end
   end
end
-- shamans can be hanging out in/near the temple
for i=1,math.random(2 - 1,2*3) do
   des.monster({ id = "orc shaman", coord = near_temple:rndcoord(0), peaceful=0 });
end
-- these are not such a big deal
-- to run into outside the bars
for i=1,9 + math.random(2 - 1,2*5) do
   if percent(90) then
      des.monster({ id = "hill orc", peaceful = 0 })
   else
      des.monster({ id = "goblin", peaceful = 0 })
   end
end

-- Hack to force full-level wallification

des.wallify()
