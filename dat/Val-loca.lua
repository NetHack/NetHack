-- NetHack 3.6	Valkyrie.des	$NHDT-Date: 1553807172 2019/03/28 21:06:12 $  $NHDT-Branch: NetHack-3.6.2-beta01 $:$NHDT-Revision: 1.15 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991-2 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "hardfloor", "icedpools", "noflip")

des.level_init({ style="mines", fg=".", bg="I", smoothed=true, joined=false, lit=1, walled=false })

des.map([[
PPPPxxxx                      xxxxPPPPPx
PLPxxx                          xPPLLLPP
PPP    .......................    PPPLLP
xx   ............................   PPPP
x  ...............................  xxxx
  .................................   xx
....................................   x
  ...................................   
x  ..................................  x
xx   ..............................   PP
xPPP  ..........................     PLP
xPLLP                             xxPLLP
xPPPPxx                         xxxxPPPP
]]);
-- Dungeon Description
des.region(selection.area(00,00,39,12), "lit")
-- Stairs
des.stair("up", 48,14)
des.stair("down", 20,06)
-- Non diggable walls
des.non_diggable(selection.area(00,00,39,12))
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
-- Random traps
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap()
des.trap()
-- Random monsters.
des.monster("fire ant")
des.monster("fire ant")
des.monster("fire ant")
des.monster("fire ant")
des.monster("fire ant")
des.monster("fire ant")
des.monster("fire ant")
des.monster("fire ant")
des.monster("fire ant")
des.monster("fire ant")
des.monster("fire ant")
des.monster("fire ant")
des.monster("fire ant")
des.monster("fire ant")
des.monster("fire ant")
des.monster("fire ant")
des.monster("fire ant")
des.monster("a")
des.monster({ class = "H", peaceful = 0 })
des.monster({ id = "fire giant", peaceful = 0 })
des.monster({ id = "fire giant", peaceful = 0 })
des.monster({ id = "fire giant", peaceful = 0 })
des.monster({ id = "fire giant", peaceful = 0 })
des.monster({ id = "fire giant", peaceful = 0 })
des.monster({ id = "fire giant", peaceful = 0 })
des.monster({ id = "fire giant", peaceful = 0 })
des.monster({ class = "H", peaceful = 0 })

