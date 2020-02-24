-- NetHack 3.6	Samurai.des	$NHDT-Date: 1432512783 2015/05/25 00:13:03 $  $NHDT-Branch: master $:$NHDT-Revision: 1.11 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991-92 by M. Stephenson, P. Winner
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "noflip");

des.level_init({ style="mines", fg=".", bg="P", smoothed=true, joined=true, walled=true })

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
des.monster("d")
des.monster("wolf")
des.monster("wolf")
des.monster("wolf")
des.monster("wolf")
des.monster("wolf")
des.monster("stalker")
--
des.trap()
des.trap()
des.trap()
des.trap()
