-- NetHack Samurai Sam-filb.lua	$NHDT-Date: 1781994873 2026/06/20 22:34:33 $  $NHDT-Branch: NetHack-5.0 $:$NHDT-Revision: 1.2 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991-92 by M. Stephenson, P. Winner
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel");

des.map([[
-------------                                  -------------
|...........|                                  |...........|
|...-----...|----------------------------------|...-----...|
|...|   |...|..................................|...|   |...|
|...-----..........................................-----...|
|...........|--S----------------------------S--|...........|
----...--------.|..........................|.--------...----
   |...|........+..........................+........|...|   
   |...|........+..........................+........|...|   
----...--------.|..........................|.--------...----
|...........|--S----------------------------S--|...........|
|...-----..........................................-----...|
|...|   |...|..................................|...|   |...|
|...-----...|----------------------------------|...-----...|
|...........|                                  |...........|
-------------                                  -------------
]]);
des.region(selection.area(00,00,59,15), "unlit")
-- Doors
des.door("closed",16,07)
des.door("closed",16,08)
des.door("closed",43,07)
des.door("closed",43,08)
--
des.stair("up")
des.stair("down")
--
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
des.monster("d")
des.monster("wolf")
des.monster("wolf")
des.monster("wolf")
des.monster("wolf")
des.monster("stalker")
des.monster("stalker")
des.monster("stalker")
--
des.trap()
des.trap()
des.trap()
des.trap()
