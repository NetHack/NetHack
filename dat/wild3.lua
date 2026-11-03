-- Wilderness Trial 3: Pride. Hall, eight treasure rooms, altar at the east end.
des.level_init({ style = "solidfill", fg = " " })
des.level_flags("mazelevel", "nomongen", "noflip", "hardfloor", "noteleport")

des.map([[
-------------------------------------------------------------------         
|...............|................|................|...............|         
|...............|................|................|...............|         
|...............|................|................|...............|         
|...............|................|................|...............|         
|...............|................|................|...............|         
--------+----------------+---------------+----------------+--------         
|.................................................................|........|
|.................................................................|........|
|.<...............................................................+........|
|.................................................................|........|
|.................................................................|........|
--------+----------------+---------------+----------------+--------         
|...............|................|................|...............|         
|...............|................|................|...............|         
|...............|................|................|...............|         
|...............|................|................|...............|         
|...............|................|................|...............|         
-------------------------------------------------------------------         
]])
des.region(selection.area(0,0,75,18), "lit")
des.non_diggable(selection.area(0,0,75,18))
des.stair("up", 2, 9)
-- The down stair is created only when the hero prays on the altar.
des.door("closed", 8, 6)
des.door("closed", 25, 6)
des.door("closed", 41, 6)
des.door("closed", 58, 6)
des.door("closed", 8, 12)
des.door("closed", 25, 12)
des.door("closed", 41, 12)
des.door("closed", 58, 12)
des.door("secret", 66, 9)
des.altar({ x=64, y=9, align="coaligned", type="altar" })
des.engraving({ x=33, y=9, type="verse", degrade=false, text="Humble yourselves in the sight of the Lord, and he shall lift you up." })

local spots = {
   {8,1}, {25,1}, {41,1}, {58,1},
   {8,17}, {25,17}, {41,17}, {58,17},
}
local lures = {
   { id="long sword", spe=12, buc="blessed", name="silver long sword" },
   { id="gray dragon scale mail", spe=12, buc="blessed" },
   { id="ring of protection", name="ring of complete immunity" },
   { id="amulet of life saving", name="Amulet of Immortality" },
   { id="rod of northern wind", name="Rod of Frozen Power" },
   { id="finger of execution", name="prophecy book of execution" },
   { id="magic lamp", name="magic lamp of unlimited wishing" },
   { id="bag of holding", name="bag of infinite holding" },
}
for i = #lures, 2, -1 do
   local j = math.random(i)
   lures[i], lures[j] = lures[j], lures[i]
end
local mirrors_hall = {
   {8,7},{25,7},{41,7},{58,7},
   {8,11},{25,11},{41,11},{58,11},
}
local mirrors_live = {
   {8,3},{25,3},{41,3},{58,3},
   {8,15},{25,15},{41,15},{58,15},
}
for i = 1, 8 do
   local s = spots[i]
   local lure = lures[i]
   local spec = { id=lure.id, x=s[1], y=s[2] }
   if lure.spe then spec.spe = lure.spe end
   if lure.buc then spec.buc = lure.buc end
   if lure.name then spec.name = lure.name end
   des.object(spec)
end
for i = 1, #mirrors_hall do
   des.object({ id="mirror", x=mirrors_hall[i][1], y=mirrors_hall[i][2] })
end
for i = 1, #mirrors_live do
   des.object({ id="mirror", x=mirrors_live[i][1], y=mirrors_live[i][2], spe=1 })
end
