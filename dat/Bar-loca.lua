-- NetHack Barbarian Bar-loca.lua	$NHDT-Date: 1652196000 2022/05/10 15:20:00 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.1 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "hardfloor")

des.map([[
..........PPP.........................................                      
...........PP..........................................        .......      
..........PP...........-----..........------------------     ..........     
...........PP..........+...|..........|....S...........|..  ............    
..........PPP..........|...|..........|-----...........|...  .............  
...........PPP.........-----..........+....+...........|...  .............  
..........PPPPPPPPP...................+....+...........S.................   
........PPPPPPPPPPPPP.........-----...|-----...........|................    
......PPPPPPPPPPPPPP..P.......+...|...|....S...........|          ...       
.....PPPPPPP......P..PPPP.....|...|...------------------..         ...      
....PPPPPPP.........PPPPPP....-----........................      ........   
...PPPPPPP..........PPPPPPP..................................   ..........  
....PPPPPPP........PPPPPPP....................................  ..........  
.....PPPPP........PPPPPPP.........-----........................   ........  
......PPP..PPPPPPPPPPPP...........+...|.........................    .....   
..........PPPPPPPPPPP.............|...|.........................     ....   
..........PPPPPPPPP...............-----.........................       .    
..............PPP.................................................          
...............PP....................................................       
................PPP...................................................      
]]);
-- Dungeon Description
des.region(selection.area(00,00,75,19), "lit")
des.region(selection.area(24,03,26,04), "unlit")
des.region(selection.area(31,08,33,09), "unlit")
des.region(selection.area(35,14,37,15), "unlit")
des.region(selection.area(39,03,54,08), "lit")
des.region(selection.area(56,00,75,08), "unlit")
des.region(selection.area(64,09,75,16), "unlit")
-- Doors
des.door("open",23,03)
des.door("open",30,08)
des.door("open",34,14)
des.door("locked",38,05)
des.door("locked",38,06)
des.door("closed",43,03)
des.door("closed",43,05)
des.door("closed",43,06)
des.door("closed",43,08)
des.door("locked",55,06)
-- Stairs
des.stair("up", 05,02)
des.stair("down", 70,13)
-- Objects
des.object({ x = 42, y = 03 })
des.object({ x = 42, y = 03 })
des.object({ x = 42, y = 03 })
des.object({ x = 41, y = 03 })
des.object({ x = 41, y = 03 })
des.object({ x = 41, y = 03 })
des.object({ x = 41, y = 03 })
des.object({ x = 41, y = 08 })
des.object({ x = 41, y = 08 })
des.object({ x = 42, y = 08 })
des.object({ x = 42, y = 08 })
des.object({ x = 42, y = 08 })
des.object({ x = 71, y = 13 })
des.object({ x = 71, y = 13 })
des.object({ x = 71, y = 13 })
-- Random traps
des.trap("spiked pit",10,13)
des.trap("spiked pit",21,07)
des.trap("spiked pit",67,08)
des.trap("spiked pit",68,09)
des.trap()
des.trap()
des.trap()
des.trap()
-- Random monsters.
des.monster({ id = "ogre", x=12, y=09, peaceful=0 })
des.monster({ id = "ogre", x=18, y=11, peaceful=0 })
des.monster({ id = "ogre", x=45, y=05, peaceful=0 })
des.monster({ id = "ogre", x=45, y=06, peaceful=0 })
des.monster({ id = "ogre", x=47, y=05, peaceful=0 })
des.monster({ id = "ogre", x=46, y=05, peaceful=0 })
des.monster({ id = "ogre", x=56, y=03, peaceful=0 })
des.monster({ id = "ogre", x=56, y=04, peaceful=0 })
des.monster({ id = "ogre", x=56, y=05, peaceful=0 })
des.monster({ id = "ogre", x=56, y=06, peaceful=0 })
des.monster({ id = "ogre", x=57, y=03, peaceful=0 })
des.monster({ id = "ogre", x=57, y=04, peaceful=0 })
des.monster({ id = "ogre", x=57, y=05, peaceful=0 })
des.monster({ id = "ogre", x=57, y=06, peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ class = "O", peaceful=0 })
des.monster({ class = "T", peaceful=0 })
des.monster({ id = "rock troll", x=46, y=06, peaceful=0 })
des.monster({ id = "rock troll", x=47, y=06, peaceful=0 })
des.monster({ id = "rock troll", x=56, y=07, peaceful=0 })
des.monster({ id = "rock troll", x=57, y=07, peaceful=0 })
des.monster({ id = "rock troll", x=70, y=13, peaceful=0 })
des.monster({ id = "rock troll", peaceful=0 })
des.monster({ id = "rock troll", peaceful=0 })
des.monster({ class = "T", peaceful=0 })

