-- $NHDT-Header: NetHack 5.0 Kni-filb.lua 2026/06/20_22:35:38 NetHack-5.0:1.5 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991,1992 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = "." });

des.level_flags("mazelevel", "noflip");

des.level_init({ style="mines", fg=".", bg="P", smoothed=false, joined=true, lit=1, walled=false })

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
des.monster({ id = "quasit", peaceful=0 })
des.monster({ id = "quasit", peaceful=0 })
des.monster({ id = "quasit", peaceful=0 })
des.monster({ id = "quasit", peaceful=0 })
des.monster({ class = "i", peaceful=0 })
des.monster({ id = "ochre jelly", peaceful=0 })
des.monster({ id = "ochre jelly", peaceful=0 })
des.monster({ id = "ochre jelly", peaceful=0 })
--
des.trap()
des.trap()
des.trap()
des.trap()
