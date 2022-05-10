-- NetHack Rogue Rog-loca.lua	$NHDT-Date: 1652196012 2022/05/10 15:20:12 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.2 $
--	Copyright (c) 1992 by Dean Luick
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel");

--         1         2         3         4         5         6         7
--123456789012345678901234567890123456789012345678901234567890123456789012345
des.map([[
             ----------------------------------------------------   --------
           ---.................................................-    --.....|
         ---...--------........-------.......................---     ---...|
       ---.....-      ---......-     ---..................----         --.--
     ---.....----       --------       --..................--         --..| 
   ---...-----                       ----.----.....----.....---      --..|| 
----..----                       -----..---  |...---  |.......---   --...|  
|...---                       ----....---    |.---    |.........-- --...||  
|...-                      ----.....---     ----      |..........---....|   
|...----                ----......---       |         |...|.......-....||   
|......-----          ---.........-         |     -----...|............|    
|..........-----   ----...........---       -------......||...........||    
|..............-----................---     |............|||..........|     
|------...............................---   |...........|| |.........||     
|.....|..............------.............-----..........||  ||........|      
|.....|.............--    ---.........................||    |.......||      
|.....|.............-       ---.....................--|     ||......|       
|-S----------.......----      --.................----        |.....||       
|...........|..........--------..............-----           ||....|        
|...........|............................-----                |....|        
------------------------------------------                    ------        
]]);
-- Dungeon Description
des.region(selection.area(00,00,75,20), "lit")
-- Doors
--DOOR:locked|closed|open,(xx,yy)
-- Stairs
des.stair("up")
des.stair("down")
-- Non diggable walls
des.non_diggable(selection.area(00,00,75,20))
-- Objects
des.object({ id = "scroll of teleportation", x=11, y=18, buc="cursed", spe=0 })
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
-- Random traps
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
-- Random monsters.
des.monster({ id = "leprechaun", peaceful=0 })
des.monster({ id = "leprechaun", peaceful=0 })
des.monster({ id = "leprechaun", peaceful=0 })
des.monster({ id = "leprechaun", peaceful=0 })
des.monster({ id = "leprechaun", peaceful=0 })
des.monster({ id = "leprechaun", peaceful=0 })
des.monster({ id = "leprechaun", peaceful=0 })
des.monster({ id = "leprechaun", peaceful=0 })
des.monster({ id = "leprechaun", peaceful=0 })
des.monster({ id = "leprechaun", peaceful=0 })
des.monster({ id = "leprechaun", peaceful=0 })
des.monster({ id = "leprechaun", peaceful=0 })
des.monster({ id = "leprechaun", peaceful=0 })
des.monster({ id = "leprechaun", peaceful=0 })
des.monster({ id = "leprechaun", peaceful=0 })
des.monster({ id = "leprechaun", peaceful=0 })
des.monster({ id = "leprechaun", peaceful=0 })
des.monster({ class = "l", peaceful=0 })
des.monster({ id = "guardian naga", peaceful=0 })
des.monster({ id = "guardian naga", peaceful=0 })
des.monster({ id = "guardian naga", peaceful=0 })
des.monster({ id = "guardian naga", peaceful=0 })
des.monster({ id = "guardian naga", peaceful=0 })
des.monster({ id = "guardian naga", peaceful=0 })
des.monster({ id = "guardian naga", peaceful=0 })
des.monster({ class = "N", peaceful=0 })
des.monster({ class = "N", peaceful=0 })
des.monster({ class = "N", peaceful=0 })
des.monster({ id = "chameleon", peaceful=0 })
des.monster({ id = "chameleon", peaceful=0 })
des.monster({ id = "chameleon", peaceful=0 })
des.monster({ id = "chameleon", peaceful=0 })
des.monster({ id = "chameleon", peaceful=0 })
