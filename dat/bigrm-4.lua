-- $NHDT-Header: NetHack 5.0 bigrm-4.lua 2026/06/20_22:35:38 NetHack-5.0:1.7 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1990 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });
des.level_flags("mazelevel", "noflip");

des.map([[
-----------                                                     -----------
|.........|                                                     |.........|
|.........-------------                             -------------.........|
---...................------------       ------------...................---
  --.............................---------.............................--  
   --.................................................................--   
    --...............................................................--    
     --......LLLLL.......................................LLLLL......--     
      --.....LLLLL.......................................LLLLL.....--      
      --.....LLLLL.......................................LLLLL.....--      
     --......LLLLL.......................................LLLLL......--     
    --...............................................................--    
   --.................................................................--   
  --.............................---------.............................--  
---...................------------       ------------...................---
|.........-------------                             -------------.........|
|.........|                                                     |.........|
-----------                                                     -----------
]]);

local terrains = { ".", ".", ".", ".", "P", "L", "-", "T", "W", "Z" };
local tidx = math.random(1, #terrains);
local toterr = terrains[tidx];
if (toterr ~= "L") then
   des.replace_terrain({ fromterrain = "L", toterrain = toterr });
end

des.feature("fountain", 05,02);
des.feature("fountain", 05,15);
des.feature("fountain", 69,02);
des.feature("fountain", 69,15);

des.region(selection.area(01,01,73,16), "lit");

des.stair("up");
des.stair("down");

des.non_diggable();

for i = 1,15 do
   des.object();
end

for i = 1,6 do
   des.trap();
end

for i = 1,28 do
  des.monster();
end
