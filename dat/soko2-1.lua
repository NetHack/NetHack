-- NetHack sokoban soko2-1.lua	$NHDT-Date: 1652196035 2022/05/10 15:20:35 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.1 $
--	Copyright (c) 1998-1999 by Kevin Hugo
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "noteleport", "premapped", "solidify", "noflip");

des.map([[
--------------------
|........|...|.....|
|.....-..|.-.|.....|
|..|.....|...|.....|
|-.|..-..|.-.|.....|
|...--.......|.....|
|...|...-...-|.....|
|...|..|...--|.....|
|-..|..|----------+|
|..................|
|...|..|------------
--------            
]]);
des.stair("down", 06,10);
des.stair("up", 16,04);
des.door("locked", 18,08);
des.region(selection.area(00,00, 19,11), "lit");
des.non_diggable(selection.area(00,00,19,11));
des.non_passwall(selection.area(00,00,19,11));

-- Boulders
des.object("boulder",02,02)
des.object("boulder",03,02)
--
des.object("boulder",05,03)
des.object("boulder",07,03)
des.object("boulder",07,02)
des.object("boulder",08,02)
--
des.object("boulder",10,03)
des.object("boulder",11,03)
--
des.object("boulder",02,07)
des.object("boulder",02,08)
des.object("boulder",03,09)
--
des.object("boulder",05,07)
des.object("boulder",06,06)

-- Traps
des.trap("hole",08,09)
des.trap("hole",09,09)
des.trap("hole",10,09)
des.trap("hole",11,09)
des.trap("hole",12,09)
des.trap("hole",13,09)
des.trap("hole",14,09)
des.trap("hole",15,09)
des.trap("hole",16,09)
des.trap("hole",17,09)

-- Random objects
des.object({ class = "%" });
des.object({ class = "%" });
des.object({ class = "%" });
des.object({ class = "%" });
des.object({ class = "=" });
des.object({ class = "/" });
