-- NetHack Tourist Tou-fila.lua	$NHDT-Date: 1652196014 2022/05/10 15:20:14 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.2 $
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
