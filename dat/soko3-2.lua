-- NetHack sokoban soko3-2.lua	$NHDT-Date: 1652196036 2022/05/10 15:20:36 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.1 $
--	Copyright (c) 1998-1999 by Kevin Hugo
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "noteleport", "premapped", "solidify", "noflip");

des.map([[
 ----          -----------
-|..|-------   |.........|
|..........|   |.........|
|..-----.-.|   |.........|
|..|...|...|   |.........|
|.........-|   |.........|
|.......|..|   |.........|
|.----..--.|   |.........|
|........|.--  |.........|
|.---.-.....------------+|
|...|...-................|
|.........----------------
----|..|..|               
    -------               
]]);
des.stair("down", 03,01)
des.stair("up", 20,04)
des.door("locked",24,09)
des.region(selection.area(00,00,25,13), "lit")
des.non_diggable(selection.area(00,00,25,13))
des.non_passwall(selection.area(00,00,25,13))

-- Boulders
des.object("boulder",02,03)
des.object("boulder",08,03)
des.object("boulder",09,04)
des.object("boulder",02,05)
des.object("boulder",04,05)
des.object("boulder",09,05)
des.object("boulder",02,06)
des.object("boulder",05,06)
des.object("boulder",06,07)
des.object("boulder",03,08)
des.object("boulder",07,08)
des.object("boulder",05,09)
des.object("boulder",10,09)
des.object("boulder",07,10)
des.object("boulder",10,10)
des.object("boulder",03,11)

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

-- Random objects
des.object({ class = "%" });
des.object({ class = "%" });
des.object({ class = "%" });
des.object({ class = "%" });
des.object({ class = "=" });
des.object({ class = "/" });

