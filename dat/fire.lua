-- NetHack endgame fire.lua	$NHDT-Date: 1652196026 2022/05/10 15:20:26 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.1 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1992,1993 by Izchak Miller, David Cohrs,
--                      and Timo Hakulinen
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "noteleport", "hardfloor", "shortsighted", "hot", "fumaroles")
-- The player lands, upon arrival, in the
-- lower-right.  The location of the
-- portal to the next level is randomly chosen.
-- This map has no visible outer boundary, and
-- is mostly open area, with lava lakes and bunches of fire traps.
des.map([[
............................................................................
....LLLLLLLL............L.......................LLL.........................
...LL...................L......................LLLL................LL.......
...L.............LLLL...LL....LL...............LLLLL.............LLL........
.LLLL..............LL....L.....LLL..............LLLL..............LLLL......
..........LLLL...LLLL...LLL....LLL......L........LLLL....LL........LLL......
........LLLLLLL...LL.....L......L......LL.........LL......LL........LL...L..
........LL..LLL..LL......LL......LLLL..L.........LL......LLL............LL..
....L..LL....LLLLL.................LLLLLLL.......L......LL............LLLLLL
....L..L.....LL.LLLL.......L............L........LLLLL.LL......LL.........LL
....LL........L...LL......LL.............LLL.....L...LLL.......LLL.........L
.....LLLLLL........L.......LLL.............L....LL...L.LLL......LLLLLLL.....
..........LLLL............LL.L.............L....L...LL.........LLL..LLL.....
...........................LLLLL...........LL...L...L........LLLL..LLLLLL...
.....LLLL.............LL....LL.......LLL...LL.......L..LLL....LLLLLLL.......
.......LLL.........LLLLLLLLLLL......LLLLL...L...........LL...LL...LL........
.........LL.......LL.........LL.......LLL....L..LLL....LL.........LL........
..........LLLLLLLLL...........LL....LLL.......LLLLL.....LL........LL........
.................L.............LLLLLL............LL...LLLL.........LL.......
.................................LL....................LL...................
]]);
des.teleport_region({ region = {69,16,69,16} })
des.levregion({ region = {0,0,75,19}, exclude = {65,13,75,19}, type="portal", name="water" })

des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
des.trap("fire")
--  An assortment of fire-appropriate nasties
des.monster("red dragon")
des.monster("balrog")
des.monster({ id = "fire elemental", peaceful = 0 })
des.monster({ id = "fire elemental", peaceful = 0 })
des.monster("fire vortex")
des.monster("hell hound")
--
des.monster("fire giant")
des.monster("barbed devil")
des.monster("hell hound")
des.monster("stone golem")
des.monster("pit fiend")
des.monster({ id = "fire elemental", peaceful = 0 })
--
des.monster({ id = "fire elemental", peaceful = 0 })
des.monster("hell hound")
des.monster({ id = "fire elemental", peaceful = 0 })
des.monster({ id = "fire elemental", peaceful = 0 })
des.monster("scorpion")
des.monster("fire giant")
--
des.monster("hell hound")
des.monster("dust vortex")
des.monster("fire vortex")
des.monster({ id = "fire elemental", peaceful = 0 })
des.monster({ id = "fire elemental", peaceful = 0 })
des.monster({ id = "fire elemental", peaceful = 0 })
des.monster("hell hound")
des.monster({ id = "fire elemental", peaceful = 0 })
des.monster("stone golem")
des.monster("pit viper")
des.monster("pit viper")
des.monster("fire vortex")
--
des.monster({ id = "fire elemental", peaceful = 0 })
des.monster({ id = "fire elemental", peaceful = 0 })
des.monster("fire giant")
des.monster({ id = "fire elemental", peaceful = 0 })
des.monster("fire vortex")
des.monster("fire vortex")
des.monster("pit fiend")
des.monster({ id = "fire elemental", peaceful = 0 })
des.monster("pit viper")
--
des.monster({ id = "salamander", peaceful = 0 })
des.monster({ id = "salamander", peaceful = 0 })
des.monster("minotaur")
des.monster({ id = "salamander", peaceful = 0 })
des.monster("steam vortex")
des.monster({ id = "salamander", peaceful = 0 })
des.monster({ id = "salamander", peaceful = 0 })
--
des.monster("fire giant")
des.monster("barbed devil")
des.monster({ id = "fire elemental", peaceful = 0 })
des.monster("fire vortex")
des.monster({ id = "fire elemental", peaceful = 0 })
des.monster({ id = "fire elemental", peaceful = 0 })
des.monster("hell hound")
des.monster("fire giant")
des.monster("pit fiend")
des.monster({ id = "fire elemental", peaceful = 0 })
des.monster({ id = "fire elemental", peaceful = 0 })
--
des.monster("barbed devil")
des.monster({ id = "salamander", peaceful = 0 })
des.monster("steam vortex")
des.monster({ id = "salamander", peaceful = 0 })
des.monster({ id = "salamander", peaceful = 0 })

des.object("boulder")
des.object("boulder")
des.object("boulder")
des.object("boulder")
des.object("boulder")

