-- NetHack bigroom bigrm-12.lua	$NHDT-Date: $  $NHDT-Branch: NetHack-5.0 $
--	Copyright (c) 2024 by Pasi Kallinen
-- NetHack may be freely redistributed.  See license for details.
--
-- Two hexagons

des.level_flags("mazelevel", "noflipy");
des.level_init({ style = "solidfill", fg = " " });

des.map([[
                                                                           
         .......................           .......................         
        .........................         .........................        
       ...........................       ...........................       
      .............................     .............................      
     ........PPPPPPPPPPPPPPP........   ........LLLLLLLLLLLLLLL........     
    ........PPPPPPPPPPPPPPPPP........ ........LLLLLLLLLLLLLLLLL........    
   ........PPPWWWWWWWWWWWWWPPP...............LLLZZZZZZZZZZZZZLLL........   
  ........PPPWWWWWWWWWWWWWWWPPP.............LLLZZZZZZZZZZZZZZZLLL........  
 ........PPPWWWWWWWWWWWWWWWWWPPP...........LLLZZZZZZZZZZZZZZZZZLLL........ 
  ........PPPWWWWWWWWWWWWWWWPPP.............LLLZZZZZZZZZZZZZZZLLL........  
   ........PPPWWWWWWWWWWWWWPPP...............LLLZZZZZZZZZZZZZLLL........   
    ........PPPPPPPPPPPPPPPPP........ ........LLLLLLLLLLLLLLLLL........    
     ........PPPPPPPPPPPPPPP........   ........LLLLLLLLLLLLLLL........     
      .............................     .............................      
       ...........................       ...........................       
        .........................         .........................        
         .......................           .......................         
                                                                           
]]);

-- maybe replace lavawalls/waterwalls with stone walls
if percent(20) then
   if percent(50) then
      des.replace_terrain({ fromterrain = "W", toterrain = "-" });
   end
   if percent(50) then
      des.replace_terrain({ fromterrain = "Z", toterrain = "-" });
   end
end

-- maybe replace pools with floor and then possibly walls with pools
if percent(25) then
   des.replace_terrain({ fromterrain = "P", toterrain = "." });
   if percent(75) then
      des.replace_terrain({ fromterrain = "W", toterrain = "P" });
   end
end
if percent(25) then
   des.replace_terrain({ fromterrain = "L", toterrain = "." });
   if percent(75) then
      des.replace_terrain({ fromterrain = "Z", toterrain = "L" });
   end
end

-- maybe make both sides have the same terrain
if percent(20) then
   if percent(50) then
      -- both are lava
      des.replace_terrain({ fromterrain = "P", toterrain = "L" });
      des.replace_terrain({ fromterrain = "W", toterrain = "Z" });
   else
      -- both are water
      des.replace_terrain({ fromterrain = "L", toterrain = "P" });
      des.replace_terrain({ fromterrain = "Z", toterrain = "W" });
   end
end

des.region(selection.area(00,00,75,19), "lit")
des.non_diggable();

des.wallify();

des.stair("up");
des.stair("down");

for i = 1,15 do
   des.object();
end
for i = 1,6 do
   des.trap();
end
for i = 1,28 do
  des.monster();
end
