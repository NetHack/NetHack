-- NetHack Knight Kni-loca.lua	$NHDT-Date: 1652196005 2022/05/10 15:20:05 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.2 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991,92 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "hardfloor")

des.level_init({ style="mines", fg=".", bg="P", smoothed=false, joined=true, lit=1, walled=false })

des.map([[
xxxxxxxxx......xxxx...........xxxxxxxxxx
xxxxxxx.........xxx.............xxxxxxxx
xxxx..............................xxxxxx
xx.................................xxxxx
....................................xxxx
.......................................x
........................................
xx...................................xxx
xxxx..............................xxxxxx
xxxxxx..........................xxxxxxxx
xxxxxxxx.........xx..........xxxxxxxxxxx
xxxxxxxxx.......xxxxxx.....xxxxxxxxxxxxx
]]);
-- Dungeon Description
-- The Isle of Glass is a Tor rising out of the swamps surrounding it.
des.region(selection.area(00,00,39,11), "lit")
-- The top area of the Tor is a holy site.
des.region({ region={09,02, 27,09}, lit=1, type="temple", filled=2 })
-- Stairs
des.stair("up", 38,0)
des.stair("down", 18,05)
-- The altar atop the Tor and its attendant (creating altar makes the priest).
des.altar({ x=17, y=05, align="neutral", type="shrine" })
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
-- All of the avenues are guarded by magic except for the East.
-- South
des.trap("magic",08,11)
des.trap("magic",09,11)
des.trap("magic",10,11)
des.trap("magic",11,11)
des.trap("magic",12,11)
des.trap("magic",13,11)
des.trap("magic",14,11)
des.trap("magic",15,11)
des.trap("magic",16,11)
des.trap("magic",20,11)
des.trap("magic",21,11)
des.trap("magic",22,11)
des.trap("magic",23,11)
des.trap("magic",24,11)
des.trap("magic",25,11)
des.trap("magic",26,11)
des.trap("magic",27,11)
des.trap("magic",28,11)
-- West
des.trap("magic",00,03)
des.trap("magic",00,04)
des.trap("magic",00,05)
des.trap("magic",00,06)
-- North
des.trap("magic",06,00)
des.trap("magic",07,00)
des.trap("magic",08,00)
des.trap("magic",09,00)
des.trap("magic",10,00)
des.trap("magic",11,00)
des.trap("magic",12,00)
des.trap("magic",13,00)
des.trap("magic",14,00)
des.trap("magic",19,00)
des.trap("magic",20,00)
des.trap("magic",21,00)
des.trap("magic",22,00)
des.trap("magic",23,00)
des.trap("magic",24,00)
des.trap("magic",25,00)
des.trap("magic",26,00)
des.trap("magic",27,00)
des.trap("magic",28,00)
des.trap("magic",29,00)
des.trap("magic",30,00)
des.trap("magic",31,00)
des.trap("magic",32,00)
-- Even so, there are magic "sinkholes" around.
des.trap("anti magic")
des.trap("anti magic")
des.trap("anti magic")
des.trap("anti magic")
des.trap("anti magic")
des.trap("anti magic")
des.trap("anti magic")
-- Random monsters.
des.monster({ id = "quasit", peaceful=0 })
des.monster({ id = "quasit", peaceful=0 })
des.monster({ id = "quasit", peaceful=0 })
des.monster({ id = "quasit", peaceful=0 })
des.monster({ id = "quasit", peaceful=0 })
des.monster({ id = "quasit", peaceful=0 })
des.monster({ id = "quasit", peaceful=0 })
des.monster({ id = "quasit", peaceful=0 })
des.monster({ id = "quasit", peaceful=0 })
des.monster({ id = "quasit", peaceful=0 })
des.monster({ id = "quasit", peaceful=0 })
des.monster({ id = "quasit", peaceful=0 })
des.monster({ id = "quasit", peaceful=0 })
des.monster({ id = "quasit", peaceful=0 })
des.monster({ id = "quasit", peaceful=0 })
des.monster({ id = "quasit", peaceful=0 })
des.monster({ id = "quasit", peaceful=0 })
des.monster({ class = "i", peaceful=0 })
des.monster({ class = "j", peaceful=0 })
des.monster({ id = "ochre jelly", peaceful=0 })
des.monster({ id = "ochre jelly", peaceful=0 })
des.monster({ id = "ochre jelly", peaceful=0 })
des.monster({ id = "ochre jelly", peaceful=0 })
des.monster({ id = "ochre jelly", peaceful=0 })
des.monster({ id = "ochre jelly", peaceful=0 })
des.monster({ id = "ochre jelly", peaceful=0 })
des.monster({ class = "j", peaceful=0 })
