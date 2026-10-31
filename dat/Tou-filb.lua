-- $NHDT-Header: NetHack 5.0 Tou-filb.lua 2026/06/20_22:35:38 NetHack-5.0:1.5 $
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
des.monster({ id = "captain", peaceful = 0 })
des.monster({ id = "captain", peaceful = 0 })
des.monster({ class = "H", peaceful = 0 })
des.monster({ class = "H", peaceful = 0 })
des.monster({ class = "C", peaceful = 0 })
des.monster("s")
