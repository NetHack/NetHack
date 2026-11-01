-- Wilderness Trial 3: Pride. Eight rooms, one lure each, shuffled.
des.level_init({ style = "solidfill", fg = " " })
des.level_flags("mazelevel", "nomongen", "noflip", "hardfloor", "noteleport")

des.map([[
######################################################################
#.................................................................#..#
#.................................................................#..#
#.................................................................#..#
#..##+###..##+###..##+###..##+###..##+###..##+###..##+###..##+###.#..#
#..#....#..#....#..#....#..#....#..#....#..#....#..#....#..#....#.#..#
#..#....#..#....#..#....#..#....#..#....#..#....#..#....#..#....#.#..#
#..######..######..######..######..######..######..######..######.#..#
######################################################################
]])
des.region(selection.area(0,0,69,8), "lit")
des.non_diggable(selection.area(0,0,69,8))
des.stair("up", 1, 1)
des.stair("down", 67, 1)
des.engraving({ x=34, y=2, type="engrave", degrade=false, text="Humble yourselves in the sight of the Lord, and he shall lift you up." })

local rooms = {
   { door=5,  out=5,  inn=5,  lure=4  },
   { door=13, out=13, inn=13, lure=12 },
   { door=21, out=21, inn=21, lure=20 },
   { door=29, out=29, inn=29, lure=28 },
   { door=37, out=37, inn=37, lure=36 },
   { door=45, out=45, inn=45, lure=44 },
   { door=53, out=53, inn=53, lure=52 },
   { door=61, out=61, inn=61, lure=60 },
}
local lures = {
   { id="long sword", spe=12, buc="blessed", name="silver long sword" },
   { id="gray dragon scale mail", spe=12, buc="blessed" },
   { id="ring of protection", name="ring of complete immunity" },
   { id="amulet of life saving", name="Amulet of Immortality" },
   { id="northern wind", name="Rod of Frozen Power" },
   { id="finger of execution", name="prophecy book of execution" },
   { id="magic lamp", name="magic lamp of unlimited wishing" },
   { id="bag of holding", name="bag of infinite holding" },
}
for i = #lures, 2, -1 do
   local j = math.random(i)
   lures[i], lures[j] = lures[j], lures[i]
end
for i = 1, 8 do
   local room = rooms[i]
   local lure = lures[i]
   des.door("closed", room.door, 4)
   -- Harmless mirror in the hall, just outside the door.
   des.object({ id="mirror", x=room.out, y=3 })
   -- Live mirror inside. spe 1 marks it. Picking up a lure, stepping
   -- on this mirror, or kicking it wakes the reflection.
   des.object({ id="mirror", x=room.inn, y=6, spe=1 })
   local spec = { id=lure.id, x=room.lure, y=6 }
   if lure.spe then spec.spe = lure.spe end
   if lure.buc then spec.buc = lure.buc end
   if lure.name then spec.name = lure.name end
   des.object(spec)
end
