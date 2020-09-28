-- NetHack 3.7	sokoban.des	$NHDT-Date: 1432512784 2015/05/25 00:13:04 $  $NHDT-Branch: master $:$NHDT-Revision: 1.13 $
--	Copyright (c) 1998-1999 by Kevin Hugo
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "noteleport", "premapped", "solidify");

des.map([[
  --------          
--|.|....|          
|........|----------
|.-...-..|.|.......|
|...-......|.......|
|.-....|...|.......|
|....-.--.-|.......|
|..........|.......|
|.--...|...|.......|
|....-.|---|.......|
--|....|----------+|
  |................|
  ------------------
]]);
des.stair("down", 06,11)
des.stair("up", 15,06)
des.door("locked",18,10)
des.region(selection.area(00,00,19,12), "lit");
des.non_diggable(selection.area(00,00,19,12));
des.non_passwall(selection.area(00,00,19,12));

-- Boulders
des.object("boulder",04,02)
des.object("boulder",04,03)
des.object("boulder",05,03)
des.object("boulder",07,03)
des.object("boulder",08,03)
des.object("boulder",02,04)
des.object("boulder",03,04)
des.object("boulder",05,05)
des.object("boulder",06,06)
des.object("boulder",09,06)
des.object("boulder",03,07)
des.object("boulder",04,07)
des.object("boulder",07,07)
des.object("boulder",06,09)
des.object("boulder",05,10)
des.object("boulder",05,11)

-- Traps
des.trap("hole",07,11)
des.trap("hole",08,11)
des.trap("hole",09,11)
des.trap("hole",10,11)
des.trap("hole",11,11)
des.trap("hole",12,11)
des.trap("hole",13,11)
des.trap("hole",14,11)
des.trap("hole",15,11)
des.trap("hole",16,11)
des.trap("hole",17,11)

-- Random objects
des.object({ class = "%" });
des.object({ class = "%" });
des.object({ class = "%" });
des.object({ class = "%" });
des.object({ class = "=" });
des.object({ class = "/" });
