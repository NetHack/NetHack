
-- Test all of the special levels

function saferequire(file)
   if (not pcall(require, file)) then
       nh.pline("Cannot load level file '" .. file .. "'.");
       if (nhc.DLB == 1) then
           nh.pline("Maybe due to compile-time option DLB.")
       end
       return false;
   end
   return true;
end;

local special_levels = {
"air",
"asmodeus",
"astral",
"baalz",
"bigrm-1",
"bigrm-2",
"bigrm-3",
"bigrm-4",
"bigrm-5",
"bigrm-6",
"bigrm-7",
"bigrm-8",
"bigrm-9",
"bigrm-10",
"bigrm-11",
"castle",
"earth",
"fakewiz1",
"fakewiz2",
"fire",
"juiblex",
"knox",
"medusa-1",
"medusa-2",
"medusa-3",
"medusa-4",
"minefill",
"minend-1",
"minend-2",
"minend-3",
"minetn-1",
"minetn-2",
"minetn-3",
"minetn-4",
"minetn-5",
"minetn-6",
"minetn-7",
"oracle",
"orcus",
"sanctum",
"soko1-1",
"soko1-2",
"soko2-1",
"soko2-2",
"soko3-1",
"soko3-2",
"soko4-1",
"soko4-2",
"tower1",
"tower2",
"tower3",
"valley",
"water",
"wizard1",
"wizard2",
"wizard3"
}

local roles = { "Arc", "Bar", "Cav", "Hea", "Kni", "Mon", "Pri", "Ran", "Rog", "Sam", "Tou", "Val", "Wiz" }
local questlevs = { "fila", "filb", "goal", "loca", "strt" }

for _,role in ipairs(roles) do
   for _,qlev in ipairs(questlevs) do
      local lev = role .. "-" .. qlev
      table.insert(special_levels, lev)
   end
end

for _,lev in ipairs(special_levels) do
   des.reset_level();
   if (not saferequire(lev)) then
      error("Cannot load a required file.");
   end
   des.finalize_level();
end
