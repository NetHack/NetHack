-- NetHack Samurai Sam-goal.lua	$NHDT-Date: 1652196013 2022/05/10 15:20:13 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.1 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991-92 by M. Stephenson, P. Winner
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "noteleport");

des.map([[
                                             
           .......................           
       ......-------------------......       
    ......----.................----......    
   ....----.....-------------.....----....   
  ....--.....----...........----.....--....  
  ...||....---....---------....---....||...  
  ...|....--....---.......---....--....|...  
 ....|...||...---...--+--...---...||...|.... 
 ....|...|....|....|-...-|....|....|...|.... 
 ....|...|....|....+.....+....|....|...|.... 
 ....|...|....|....|-...-|....|....|...|.... 
 ....|...||...---...--+--...---...||...|.... 
  ...|....--....---.......---....--....|...  
  ...||....---....---------....---....||...  
  ....--.....----...........----.....--....  
   ....----.....-------------.....----....   
    ......----.................----......    
       ......-------------------......       
           .......................           
]]);
-- Dungeon Description
local place = { {02,11},{42,09} }
local placeidx = math.random(1, #place);

des.region(selection.area(00,00,44,19), "unlit")
-- Doors
des.door("closed",19,10)
des.door("closed",22,08)
des.door("closed",22,12)
des.door("closed",25,10)
-- Stairs
des.stair({ dir = "up", coord = place[placeidx] })

-- Holes in the concentric ring walls
local place = { {22,14},{30,10},{22, 6},{14,10} }
local placeidx = math.random(1, #place);
des.terrain(place[placeidx], ".")
local place = { {22, 4},{35,10},{22,16},{ 9,10} }
local placeidx = math.random(1, #place);
des.terrain(place[placeidx], ".")
local place = { {22, 2},{22,18} }
local placeidx = math.random(1, #place);
des.terrain(place[placeidx], ".")

-- Non diggable walls
des.non_diggable(selection.area(00,00,44,19))
-- Objects
des.object({ id = "tsurugi", x=22, y=10, buc="blessed", spe=0, name="The Tsurugi of Muramasa" })
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
--
des.trap("board",22,09)
des.trap("board",24,10)
des.trap("board",22,11)
-- Random traps
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
-- Random monsters.
des.monster("Ashikaga Takauji", 22, 10)
des.monster({ id = "samurai", peaceful=0 })
des.monster({ id = "samurai", peaceful=0 })
des.monster({ id = "samurai", peaceful=0 })
des.monster({ id = "samurai", peaceful=0 })
des.monster({ id = "samurai", peaceful=0 })
des.monster({ id = "ninja", peaceful=0 })
des.monster({ id = "ninja", peaceful=0 })
des.monster({ id = "ninja", peaceful=0 })
des.monster({ id = "ninja", peaceful=0 })
des.monster({ id = "ninja", peaceful=0 })
des.monster("wolf")
des.monster("wolf")
des.monster("wolf")
des.monster("wolf")
des.monster("d")
des.monster("d")
des.monster("stalker")
des.monster("stalker")
des.monster("stalker")
des.monster("stalker")
des.monster("stalker")
des.monster("stalker")
des.monster("stalker")
des.monster("stalker")
des.monster("stalker")
