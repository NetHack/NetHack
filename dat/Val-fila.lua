-- NetHack 3.6	Valkyrie.des	$NHDT-Date: 1553807172 2019/03/28 21:06:12 $  $NHDT-Branch: NetHack-3.6.2-beta01 $:$NHDT-Revision: 1.15 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991-2 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = "I" });

des.level_flags("mazelevel", "icedpools", "noflip")

des.level_init({ style="mines", fg=".", bg="I", smoothed=true, joined=true, lit=1, walled=false })

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
--
des.monster("fire ant")
des.monster("fire ant")
des.monster("fire ant")
des.monster("fire ant")
des.monster("fire ant")
des.monster("a")
des.monster({ id = "fire giant", peaceful = 0 })
--
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
