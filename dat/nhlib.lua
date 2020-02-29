
math.randomseed( os.time() )

function shuffle(list)
   for i = #list, 2, -1 do
      local j = math.random(i)
      list[i], list[j] = list[j], list[i]
   end
end

align = { "law", "neutral", "chaos" };
shuffle(align);

-- d(2,6) = 2d6
-- d(20) = 1d20 (single argument = implicit 1 die)
function d(dice, faces)
   if (faces == nil) then
      -- 1-arg form: argument "dice" is actually the number of faces
      return math.random(1, dice)
   else
      local sum = 0
      for i=1,dice do
         sum = sum + math.random(1, faces)
      end
      return sum
   end
end

-- percent(20) returns true 20% of the time
function percent(threshold)
   return math.random(0, 99) < threshold
end
