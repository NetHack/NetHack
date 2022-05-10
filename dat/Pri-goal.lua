-- NetHack Priest Pri-goal.lua	$NHDT-Date: 1652196008 2022/05/10 15:20:08 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.1 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991-2 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel");

des.level_init({ style="mines", fg="L", bg=".", smoothed=false, joined=false, lit=0, walled=false })

des.map([[
xxxxxx..xxxxxx...xxxxxxxxx
xxxx......xx......xxxxxxxx
xx.xx.............xxxxxxxx
x....................xxxxx
......................xxxx
......................xxxx
xx........................
xxx......................x
xxx................xxxxxxx
xxxx.....x.xx.......xxxxxx
xxxxx...xxxxxx....xxxxxxxx
]]);
-- Dungeon Description
local place = { {14,04}, {13,07} }
local placeidx = math.random(1, #place);

des.region(selection.area(00,00,25,10), "unlit")
-- Stairs
des.stair("up", 20,05)
-- Objects
des.object({ id = "helm of brilliance", coord = place[placeidx], buc="blessed", spe=0, name="The Mitre of Holiness" })
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
des.monster("Nalzok",place[placeidx])
des.monster("human zombie")
des.monster("human zombie")
des.monster("human zombie")
des.monster("human zombie")
des.monster("human zombie")
des.monster("human zombie")
des.monster("human zombie")
des.monster("human zombie")
des.monster("human zombie")
des.monster("human zombie")
des.monster("human zombie")
des.monster("human zombie")
des.monster("human zombie")
des.monster("human zombie")
des.monster("human zombie")
des.monster("human zombie")
des.monster("Z")
des.monster("Z")
des.monster("wraith")
des.monster("wraith")
des.monster("wraith")
des.monster("wraith")
des.monster("wraith")
des.monster("wraith")
des.monster("wraith")
des.monster("wraith")
des.monster("W")
