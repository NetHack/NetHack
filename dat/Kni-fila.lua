-- NetHack Knight Kni-fila.lua	$NHDT-Date: 1781994867 2026/06/20 22:34:27 $  $NHDT-Branch: NetHack-5.0 $:$NHDT-Revision: 1.3 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991,92 by M. Stephenson
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
--
des.monster({ id = "quasit", peaceful=0 })
des.monster({ id = "quasit", peaceful=0 })
des.monster({ id = "quasit", peaceful=0 })
des.monster({ id = "quasit", peaceful=0 })
des.monster({ class = "i", peaceful=0 })
des.monster({ id = "ochre jelly", peaceful=0 })
--
des.trap()
des.trap()
des.trap()
des.trap()
