-- NetHack 3.6	Barb.des	$NHDT-Date: 1432512784 2015/05/25 00:13:04 $  $NHDT-Branch: master $:$NHDT-Revision: 1.9 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "noflip");

des.level_init({ style="mines", fg=".", bg=" ", smoothed=true, joined=true, lit=0, walled=true })

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
des.monster({ id = "ogre", peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ class = "O", peaceful=0 })
des.monster({ id = "rock troll", peaceful=0 })
des.monster({ id = "rock troll", peaceful=0 })
des.monster({ id = "rock troll", peaceful=0 })
des.monster({ class = "T", peaceful=0 })
