-- $NHDT-Header: NetHack 5.0 Tou-fila.lua 1970/01/01_00:00:00 NetHack-5.0:1.6 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991,92 by M. Stephenson, P. Winner
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "noflip");

des.level_init({ style="mines", fg=".", bg=" ", smoothed=true, joined=true, walled=true })

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
--
des.trap()
des.trap()
des.trap()
des.trap()
--
des.monster({ id = "soldier", peaceful = 0 })
des.monster({ id = "soldier", peaceful = 0 })
des.monster({ id = "soldier", peaceful = 0 })
des.monster({ id = "soldier", peaceful = 0 })
des.monster({ id = "soldier", peaceful = 0 })
des.monster({ class = "H", peaceful = 0 })
des.monster({ class = "C", peaceful = 0 })
-- Ankh-Morpork citizens
des.monster({ id = "dwarf", peaceful = 1})
if percent(50) then
	des.monster({ id = "dwarf", peaceful = 1})
	des.monster({ id = "dwarf", peaceful = 1})
end;
if percent(50) then
	des.monster({ id = "rock troll", peaceful = 1})
	des.monster({ id = "rock troll", peaceful = 1})
end;
