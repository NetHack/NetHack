-- Wilderness Trial 2: Greed, the Cave of Wonders. One-way from here on.
-- Gate (55,10). Drop side is x<55. Restore square is (58,10). Entrance stair (1,10).
des.level_init({ style = "solidfill", fg = " " })
des.level_flags("mazelevel", "nomongen", "noflip", "hardfloor", "noteleport")

des.map([[
--------------------------------------------------------                       
|......|.......|....................|..................|                       
|......|.......|..........................|............|                       
|......|.......|..........|............|...|...........|                       
|......|.......|.........|........................|....|                       
|......|.......|..|................|.............|.....|                       
|......|.......|.......|.........||........|.|.........|---------------        
|......|.......|......|............|.....|.............|..............|        
-----+------+---....|.....|................|..........||..............|        
|...............|..|.............................|.....|..............|        
|<.....................................................+...........>..|        
|..................................................|...|..............|        
-----+------+---...|.|...........|.......|.............|..............|        
|......|.......||..............................|..|....|..............|        
|......|.......|.......|................|.|............|---------------        
|......|.......|.......................................|                       
|......|.......|.......................................|                       
|......|.......|...|......|............................|                       
|......|.......||......................................|                       
|......|.......|.............|............|............|                       
--------------------------------------------------------                       
]])
des.region(selection.area(0,0,78,20), "lit")
des.non_diggable(selection.area(0,0,78,20))
des.stair("up", 1, 10)
des.stair("down", 67, 10)
des.door("closed", 5, 8)
des.door("closed", 12, 8)
des.door("closed", 5, 12)
des.door("closed", 12, 12)
des.door("closed", 55, 10)
des.engraving({ x=2, y=10, type="verse", degrade=false, text="For what is a man profited, if he shall gain the whole world, and lose his own soul? or what shall a man give in exchange for his soul?" })
des.engraving({ x=54, y=10, type="verse", degrade=false, text="And said, Naked came I out of my mother's womb, and naked shall I return thither: the LORD gave, and the LORD hath taken away; blessed be the name of the LORD." })
des.engraving({ x=58, y=10, type="verse", degrade=false, text="But seek ye first the kingdom of God, and his righteousness; and all these things shall be added unto you." })

local pillars = { {36,1},{42,2},{26,3},{39,3},{43,3},{25,4},{50,4},{18,5},{35,5},{49,5},{23,6},{33,6},{34,6},{43,6},{45,6},{22,7},{35,7},{41,7},{20,8},{26,8},{43,8},{54,8},{16,9},{19,9},{49,9},{51,11},{19,12},{21,12},{33,12},{41,12},{16,13},{47,13},{50,13},{23,14},{40,14},{42,14},{19,17},{26,17},{16,18},{29,19},{42,19} }
local ispill = {}
for i = 1, #pillars do
   ispill[pillars[i][1] .. "," .. pillars[i][2]] = true
end
local gems = {
   "diamond", "ruby", "jacinth", "sapphire", "black opal", "emerald",
   "turquoise", "citrine", "aquamarine", "amber", "topaz", "jet",
   "opal", "chrysoberyl", "garnet", "amethyst", "jasper", "fluorite",
   "obsidian", "agate", "jade"
}
local function rnd(a, b)
   return a + math.random(b - a + 1) - 1
end
local function place_treasure(x, y)
   if math.random(4) == 1 then
      local q = (math.random(5) == 1) and rnd(10000, 20000) or rnd(1000, 5000)
      des.object({ id="gold piece", x=x, y=y, quantity=q })
   else
      local id = gems[math.random(#gems)]
      des.object({ id=id, x=x, y=y, name="flawless " .. id })
   end
end
local function treasure_floor(x, y)
   if ispill[x .. "," .. y] then return false end
   if y == 10 and x >= 1 and x <= 69 then return false end
   if x >= 16 and x <= 54 and y >= 1 and y <= 19 then return true end
   if (x >= 1 and x <= 6 or x >= 8 and x <= 14) and ((y >= 1 and y <= 7) or (y >= 13 and y <= 19)) then
      return true
   end
   return false
end
for x = 1, 54 do
   for y = 1, 19 do
      if treasure_floor(x, y) then
         place_treasure(x, y)
      end
   end
end
