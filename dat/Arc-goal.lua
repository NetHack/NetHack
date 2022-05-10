-- NetHack Archeologist Arc-goal.lua	$NHDT-Date: 1652195998 2022/05/10 15:19:58 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.2 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel");

des.map([[
                                                                            
                                  ---------                                 
                                  |..|.|..|                                 
                       -----------|..S.S..|-----------                      
                       |.|........|+-|.|-+|........|.|                      
                       |.S........S..|.|..S........S.|                      
                       |.|........|..|.|..|........|.|                      
                    ------------------+------------------                   
                    |..|..........|.......|..........|..|                   
                    |..|..........+.......|..........S..|                   
                    |..S..........|.......+..........|..|                   
                    |..|..........|.......|..........|..|                   
                    ------------------+------------------                   
                       |.|........|..|.|..|........|.|                      
                       |.S........S..|.|..S........S.|                      
                       |.|........|+-|.|-+|........|.|                      
                       -----------|..S.S..|-----------                      
                                  |..|.|..|                                 
                                  ---------                                 
                                                                            
]]);
-- Dungeon Description
des.region(selection.area(00,00,75,19), "lit")
des.region(selection.area(35,02,36,03), "unlit")
des.region(selection.area(40,02,41,03), "unlit")
des.region(selection.area(24,04,24,06), "unlit")
des.region(selection.area(26,04,33,06), "lit")
des.region(selection.area(38,02,38,06), "unlit")
des.region(selection.area(43,04,50,06), "lit")
des.region(selection.area(52,04,52,06), "unlit")
des.region(selection.area(35,05,36,06), "unlit")
des.region(selection.area(40,05,41,06), "unlit")
des.region(selection.area(21,08,22,11), "unlit")
des.region(selection.area(24,08,33,11), "lit")
des.region(selection.area(35,08,41,11), "unlit")
des.region(selection.area(43,08,52,11), "lit")
des.region(selection.area(54,08,55,11), "unlit")
des.region(selection.area(24,13,24,15), "unlit")
des.region(selection.area(26,13,33,15), "unlit")
des.region(selection.area(35,13,36,14), "unlit")
des.region(selection.area(35,16,36,17), "unlit")
des.region(selection.area(38,13,38,17), "unlit")
des.region(selection.area(40,13,41,14), "unlit")
des.region(selection.area(40,16,41,17), "unlit")
des.region({ region={43,13, 50,15}, lit=0, type="temple", filled=2 })
des.region(selection.area(52,13,52,15), "unlit")
-- Stairs
des.stair("up", 38,10)
-- Non diggable walls
des.non_diggable(selection.area(00,00,75,19))
-- The altar of Huhetotl.  Unattended.
des.altar({ x=50,y=14,align="chaos",type="altar" })
-- Objects
des.object({ id = "crystal ball", x=50, y=14,buc="blessed",spe=5,name="The Orb of Detection" })
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
des.trap("rolling boulder",46,14)
-- Random monsters.
des.monster("Minion of Huhetotl", 50, 14)
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("S")
des.monster("human mummy")
des.monster("human mummy")
des.monster("human mummy")
des.monster("human mummy")
des.monster("human mummy")
des.monster("human mummy")
des.monster("human mummy")
des.monster("human mummy")
des.monster("M")
