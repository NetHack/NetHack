-- NetHack Monk Mon-loca.lua	$NHDT-Date: 1652196007 2022/05/10 15:20:07 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.1 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991-2 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel");

--         1         2         3         4         5         6         7 
--123456789012345678901234567890123456789012345678901234567890123456789012345
des.map([[
             ----------------------------------------------------   --------
           ---.................................................-    --.....|
         ---...--------........------........................---     ---...|
       ---.....-      --.......-    ----..................----         --.--
     ---.....----      ---------       --..................--         --..| 
   ---...-----                       ----.----.....----.....---      --..|| 
----..----                       -----..---  |...---  |.......---   --...|  
|...---                       ----....---    |.---    |.........-- --...||  
|...-                      ----.....---     ----      |..........---....|   
|...----                ----......---       |         |...|.......-....||   
|......-----          ---.........-         |     -----...|............|    
|..........-----   ----...........---       -------......||...........||    
|..............-----................---     |............|||..........|     
|-S----...............................---   |...........|| |.........||     
|.....|..............------.............-----..........||  ||........|      
|.....|.............--    ---.........................||    |.......||      
|.....|.............-       ---.....................--|     ||......|       
|---S--------.......----      --.................----        |.....||       
|...........|..........--------..............-----           ||....|        
|...........|............................-----                |....|        
------------------------------------------                    ------        
]]);
-- Random Monsters

-- Dungeon Description
des.region(selection.area(00,00,75,20), "lit")
-- Stairs
des.stair("up")
des.stair("down")
-- Non diggable walls
des.non_diggable(selection.area(00,00,75,20))
-- Objects
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
des.object()
-- since vegetarian monks shouldn't eat giant corpses, give a chance for
-- Str boost that isn't throttled by exercise restrictions;
-- make a modest effort (Elbereth only) to prevent xorns from eating the tins
local tinplace = selection.negate():filter_mapchar('.')
local tinloc = tinplace:rndcoord(0)
des.object({ id="tin", coord=tinloc, quantity=2, buc="blessed",
             montype="spinach" })
des.engraving({ coord=tinloc, type="burn", text="Elbereth" })
-- Random traps
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
-- Random monsters.
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("xorn")
des.monster("xorn")
des.monster("xorn")
des.monster("xorn")
des.monster("xorn")
des.monster("xorn")
des.monster("xorn")
des.monster("xorn")
des.monster("xorn")
