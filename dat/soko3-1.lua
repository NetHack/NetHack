-- NetHack sokoban soko3-1.lua	$NHDT-Date: 1652196035 2022/05/10 15:20:35 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.1 $
--	Copyright (c) 1998-1999 by Kevin Hugo
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "noteleport", "premapped", "solidify");

des.map([[
-----------       -----------
|....|....|--     |.........|
|....|......|     |.........|
|.........|--     |.........|
|....|....|       |.........|
|-.---------      |.........|
|....|.....|      |.........|
|....|.....|      |.........|
|..........|      |.........|
|....|.....|---------------+|
|....|......................|
-----------------------------
]]);
des.stair("down", 11,02)
des.stair("up", 23,04)
des.door("locked", 27,09)
des.region(selection.area(00,00,28,11), "lit")
des.non_diggable(selection.area(00,00,28,11))
des.non_passwall(selection.area(00,00,28,11))

-- Boulders
des.object("boulder",03,02)
des.object("boulder",04,02)
--
des.object("boulder",06,02)
des.object("boulder",06,03)
des.object("boulder",07,02)
--
des.object("boulder",03,06)
des.object("boulder",02,07)
des.object("boulder",03,07)
des.object("boulder",03,08)
des.object("boulder",02,09)
des.object("boulder",03,09)
des.object("boulder",04,09)
--
des.object("boulder",06,07)
des.object("boulder",06,09)
des.object("boulder",08,07)
des.object("boulder",08,10)
des.object("boulder",09,08)
des.object("boulder",09,09)
des.object("boulder",10,07)
des.object("boulder",10,10)

-- Traps
des.trap("hole",12,10)
des.trap("hole",13,10)
des.trap("hole",14,10)
des.trap("hole",15,10)
des.trap("hole",16,10)
des.trap("hole",17,10)
des.trap("hole",18,10)
des.trap("hole",19,10)
des.trap("hole",20,10)
des.trap("hole",21,10)
des.trap("hole",22,10)
des.trap("hole",23,10)
des.trap("hole",24,10)
des.trap("hole",25,10)
des.trap("hole",26,10)

-- Random objects
des.object({ class = "%" });
des.object({ class = "%" });
des.object({ class = "%" });
des.object({ class = "%" });
des.object({ class = "=" });
des.object({ class = "/" });

