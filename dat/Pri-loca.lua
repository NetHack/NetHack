-- NetHack Priest Pri-loca.lua	$NHDT-Date: 1652196009 2022/05/10 15:20:09 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.4 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991-2 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "hardfloor", "noflip")
-- This is a kludge to init the level as a lit field.
des.level_init({ style="mines", fg=".", bg=".", smoothed=false, joined=false, lit=1, walled=false })

des.map([[
........................................
........................................
..........----------+----------.........
..........|........|.|........|.........
..........|........|.|........|.........
..........|----.----.----.----|.........
..........+...................+.........
..........+...................+.........
..........|----.----.----.----|.........
..........|........|.|........|.........
..........|........|.|........|.........
..........----------+----------.........
........................................
........................................
]]);
-- Dungeon Description
des.region({ region={00,00, 09,13}, lit=0, type="morgue", filled=1 })
des.region({ region={09,00, 30,01}, lit=0, type="morgue", filled=1 })
des.region({ region={09,12, 30,13}, lit=0, type="morgue", filled=1 })
des.region({ region={31,00, 39,13}, lit=0, type="morgue", filled=1 })
des.region({ region={11,03, 29,10}, lit=1, type="temple", filled=1, irregular=1 })
-- The altar inside the temple
des.altar({ x=20,y=07, align="noalign", type="shrine" })
des.monster({ id = "aligned cleric", x=20, y=07, align="noalign", peaceful = 0 })
-- Doors
des.door("locked",10,06)
des.door("locked",10,07)
des.door("locked",20,02)
des.door("locked",20,11)
des.door("locked",30,06)
des.door("locked",30,07)
-- Stairs
-- Note:  The up stairs are *intentionally* off of the map.
des.stair("up", 43,05)
des.stair("down", 20,06)
-- Non diggable walls
des.non_diggable(selection.area(10,02,30,13))
-- Objects (inside the antechambers).
des.object({ coord = { 14, 03 } })
des.object({ coord = { 15, 03 } })
des.object({ coord = { 16, 03 } })
des.object({ coord = { 14, 10 } })
des.object({ coord = { 15, 10 } })
des.object({ coord = { 16, 10 } })
des.object({ coord = { 17, 10 } })
des.object({ coord = { 24, 03 } })
des.object({ coord = { 25, 03 } })
des.object({ coord = { 26, 03 } })
des.object({ coord = { 27, 03 } })
des.object({ coord = { 24, 10 } })
des.object({ coord = { 25, 10 } })
des.object({ coord = { 26, 10 } })
des.object({ coord = { 27, 10 } })
-- Random traps
des.trap({ coord = { 15,04 } })
des.trap({ coord = { 25,04 } })
des.trap({ coord = { 15,09 } })
des.trap({ coord = { 25,09 } })
des.trap()
des.trap()
-- No random monsters - the morgue generation will put them in.
