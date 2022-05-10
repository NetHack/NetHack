-- NetHack Barbarian Bar-fila.lua	$NHDT-Date: 1652195999 2022/05/10 15:19:59 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.2 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "noflip");

des.level_init({ style="mines", fg=".", bg=".", smoothed=true, joined=true, lit=0, walled=false })

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
des.trap()
des.trap()
des.trap()
des.trap()
--
des.monster({ id = "ogre", peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ class = "O", peaceful=0 })
des.monster({ id = "rock troll", peaceful=0 })
