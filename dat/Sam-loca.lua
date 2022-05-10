-- NetHack Samurai Sam-loca.lua	$NHDT-Date: 1652196014 2022/05/10 15:20:14 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.1 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991-92 by M. Stephenson, P. Winner
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "hardfloor");

des.map([[
............................................................................
............................................................................
........-----..................................................-----........
........|...|..................................................|...|........
........|...---..}..--+------------------------------+--..}..---...|........
........|-|...|.....|...|....|....|....|....|....|.|...|.....|...|-|........
..........|...-------...|....|....|....|....|....S.|...-------...|..........
..........|-|.........------+----+-+-------+-+--------.........|-|..........
............|..--------.|}........................}|.--------..|............
............|..+........+..........................+........+..|............
............|..+........+..........................+........+..|............
............|..--------.|}........................}|.--------..|............
..........|-|.........--------+-+-------+-+----+------.........|-|..........
..........|...-------...|.S....|....|....|....|....|...-------...|..........
........|-|...|.....|...|.|....|....|....|....|....|...|.....|...|-|........
........|...---..}..--+------------------------------+--..}..---...|........
........|...|..................................................|...|........
........-----..................................................-----........
............................................................................
............................................................................
]]);
-- Dungeon Description
des.region(selection.area(00,00,75,19), "lit")
-- Doors
des.door("locked",22,04)
des.door("locked",22,15)
des.door("locked",53,04)
des.door("locked",53,15)
des.door("locked",49,06)
des.door("locked",26,13)
des.door("locked",28,07)
des.door("locked",30,12)
des.door("locked",33,07)
des.door("locked",32,12)
des.door("locked",35,07)
des.door("locked",40,12)
des.door("locked",43,07)
des.door("locked",42,12)
des.door("locked",45,07)
des.door("locked",47,12)
des.door("closed",15,09)
des.door("closed",15,10)
des.door("closed",24,09)
des.door("closed",24,10)
des.door("closed",51,09)
des.door("closed",51,10)
des.door("closed",60,09)
des.door("closed",60,10)
-- Stairs
des.stair("up", 10,10)
des.stair("down", 25,14)
-- Non diggable walls
des.non_diggable(selection.area(00,00,75,19))
-- Objects
des.object("*", 25, 05)
des.object("*", 26, 05)
des.object("*", 27, 05)
des.object("*", 28, 05)
des.object("*", 25, 06)
des.object("*", 26, 06)
des.object("*", 27, 06)
des.object("*", 28, 06)
--
des.object("[", 40, 05)
des.object("[", 41, 05)
des.object("[", 42, 05)
des.object("[", 43, 05)
des.object("[", 40, 06)
des.object("[", 41, 06)
des.object("[", 42, 06)
des.object("[", 43, 06)
--
des.object(")", 27, 13)
des.object(")", 28, 13)
des.object(")", 29, 13)
des.object(")", 30, 13)
des.object(")", 27, 14)
des.object(")", 28, 14)
des.object(")", 29, 14)
des.object(")", 30, 14)
--
des.object("(", 37, 13)
des.object("(", 38, 13)
des.object("(", 39, 13)
des.object("(", 40, 13)
des.object("(", 37, 14)
des.object("(", 38, 14)
des.object("(", 39, 14)
des.object("(", 40, 14)
-- Random traps
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
-- Random monsters.
des.monster({ id = "ninja", x=15, y=05, peaceful=0 })
des.monster({ id = "ninja", x=16, y=05, peaceful=0 })
des.monster("wolf", 17, 05)
des.monster("wolf", 18, 05)
des.monster({ id = "ninja", x=19, y=05, peaceful=0 })
des.monster("wolf", 15, 14)
des.monster("wolf", 16, 14)
des.monster({ id = "ninja", x=17, y=14, peaceful=0 })
des.monster({ id = "ninja", x=18, y=14, peaceful=0 })
des.monster("wolf", 56, 05)
des.monster({ id = "ninja", x=57, y=05, peaceful=0 })
des.monster("wolf", 58, 05)
des.monster("wolf", 59, 05)
des.monster({ id = "ninja", x=56, y=14, peaceful=0 })
des.monster("wolf", 57, 14)
des.monster({ id = "ninja", x=58, y=14, peaceful=0 })
des.monster("d", 59, 14)
des.monster("wolf", 60, 14)
des.monster("stalker")
des.monster("stalker")
des.monster("stalker")
des.monster("stalker")
des.monster("stalker")
des.monster("stalker")
des.monster("stalker")
des.monster("stalker")
des.monster("stalker")
--	"guards" for the central courtyard.
des.monster({ id = "samurai", x=30, y=05, peaceful=0 })
des.monster({ id = "samurai", x=31, y=05, peaceful=0 })
des.monster({ id = "samurai", x=32, y=05, peaceful=0 })
des.monster({ id = "samurai", x=32, y=14, peaceful=0 })
des.monster({ id = "samurai", x=33, y=14, peaceful=0 })
des.monster({ id = "samurai", x=34, y=14, peaceful=0 })
