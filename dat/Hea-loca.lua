-- NetHack Healer Hea-loca.lua	$NHDT-Date: 1652196004 2022/05/10 15:20:04 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.2 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991, 1993 by M. Stephenson, P. Winner
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "hardfloor");
--
des.level_init({ style="mines", fg=".", bg="P", smoothed=true ,joined=true, lit=1, walled=false })

des.map([[
PPPPPPPPPPPPP.......PPPPPPPPPPP
PPPPPPPP...............PPPPPPPP
PPPP.....-------------...PPPPPP
PPPPP....|.S.........|....PPPPP
PPP......+.|.........|...PPPPPP
PPP......+.|.........|..PPPPPPP
PPPP.....|.S.........|..PPPPPPP
PPPPP....-------------....PPPPP
PPPPPPPP...............PPPPPPPP
PPPPPPPPPPP........PPPPPPPPPPPP
]]);
-- Dungeon Description
des.region(selection.area(00,00,30,09), "lit")
des.region({ region={12,03, 20,06}, lit=1, type="temple", filled=1 })
-- Doors
des.door("closed",09,04)
des.door("closed",09,05)
des.door("locked",11,03)
des.door("locked",11,06)
-- Stairs
des.stair("up", 04,04)
des.stair("down", 20,06)
-- Non diggable walls
des.non_diggable(selection.area(11,02,21,07))
-- Altar in the temple.
des.altar({ x=13,y=05, align="chaos", type="shrine" })
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
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
-- Random monsters.
des.monster("rabid rat")
des.monster("rabid rat")
des.monster("rabid rat")
des.monster("rabid rat")
des.monster("rabid rat")
des.monster("rabid rat")
des.monster("rabid rat")
des.monster("rabid rat")
des.monster({ class = "r", peaceful=0 })
des.monster("giant eel")
des.monster("giant eel")
des.monster("giant eel")
des.monster("giant eel")
des.monster("giant eel")
des.monster("electric eel")
des.monster("electric eel")
des.monster("kraken")
des.monster("shark")
des.monster("shark")
des.monster({ class = ";", peaceful=0 })
des.monster({ class = ";", peaceful=0 })
des.monster({ class = "D", peaceful=0 })
des.monster({ class = "D", peaceful=0 })
des.monster({ class = "D", peaceful=0 })
des.monster({ class = "D", peaceful=0 })
des.monster({ class = "D", peaceful=0 })
des.monster({ class = "S", peaceful=0 })
des.monster({ class = "S", peaceful=0 })
des.monster({ class = "S", peaceful=0 })
des.monster({ class = "S", peaceful=0 })
des.monster({ class = "S", peaceful=0 })
des.monster({ class = "S", peaceful=0 })
des.monster({ class = "S", peaceful=0 })
des.monster({ class = "S", peaceful=0 })
des.monster({ class = "S", peaceful=0 })
