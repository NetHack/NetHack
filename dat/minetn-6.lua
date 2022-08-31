-- NetHack mines minetn-6.lua	$NHDT-Date: 1652196031 2022/05/10 15:20:31 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.3 $
--	Copyright (c) 1989-95 by Jean-Christophe Collet
--	Copyright (c) 1991-95 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
-- "Bustling Town" by Kelly Bailey

des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "inaccessibles")

des.level_init({ style="mines", fg=".", bg="-", smoothed=true, joined=true,lit=1,walled=true })

-- Map extends the full height of the playable area in order to prevent any of
-- the cavern fill from getting cut off by walls of the town buildings and
-- creating inaccessible spaces. The inaccessibles flag does compensate for
-- this, but it does so by doing things like creating backdoors into adjacent
-- shops which we don't want.
des.map({ halign = "center", valign = "top", map = [[
x--------xxxxxxxxxxx-------------------x
x------xxxxxxxxxxxxxx-----------------xx
.-----................----------------.x
.|...|................|...|..|...|...|..
.|...+..--+--.........|...|..|...|...|..
.|...|..|...|..-----..|...|..|-+---+--..
.-----..|...|--|...|..--+---+-.........x
........|...|..|...+.............-----..
........-----..|...|......--+-...|...|..
x----...|...|+------..{...|..|...+...|..
x|..+...|...|.............|..|...|...|..
x|..|...|...|-+-.....---+-------------.x
x----...--+--..|..-+-|..................
...|........|..|..|..|----....--------x.
...|..T.....----..|..|...+....|......|x.
...|-....{........|..|...|....+......|x.
...--..-....T.....--------....|......|x.
.......--.....................----------
xxxxx-----xxxxxxxxxxxxxxxxxx------------
xxxx-------xxxxxxxxxxxxxxx--------------
]] });

des.region(selection.area(00,00,39,19),"lit")
des.levregion({ type="stair-up", region={01,03,20,19}, region_islev=1, exclude={0,0,39,19} })
des.levregion({ type="stair-down", region={61,03,75,19}, region_islev=1, exclude={0,0,39,19} })
des.region(selection.area(13,7,14,8),"unlit")
des.region({ region={09,09, 11,11}, lit=1, type="candle shop", filled=1 })
des.region({ region={16,06, 18,08}, lit=1, type="tool shop", filled=1 })
des.region({ region={23,03, 25,05}, lit=1, type="shop", filled=1 })
des.region({ region={22,14, 24,15}, lit=1, type=monkfoodshop(), filled=1 })
des.region({ region={31,14, 36,16}, lit=1, type="temple", filled=1 })
des.altar({ x=35,y=15,align=align[1],type="shrine"})

des.door("closed",5,4)
des.door("locked",4,10)
des.door("closed",10,4)
des.door("closed",10,12)
des.door("locked",13,9)
des.door("locked",14,11)
des.door("closed",19,7)
des.door("closed",19,12)
des.door("closed",24,6)
des.door("closed",24,11)
des.door("closed",25,14)
des.door("closed",28,6)
des.door("locked",28,8)
des.door("closed",30,15)
des.door("closed",31,5)
des.door("closed",35,5)
des.door("closed",33,9)

des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome", 14, 8)
des.monster("gnome lord", 14, 7)
des.monster("gnome", 27, 10)
des.monster("gnome lord")
des.monster("gnome lord")
des.monster("dwarf")
des.monster("dwarf")
des.monster("dwarf")
des.monster({ id = "watchman", peaceful = 1 })
des.monster({ id = "watchman", peaceful = 1 })
des.monster({ id = "watchman", peaceful = 1 })
des.monster({ id = "watch captain", peaceful = 1 })
des.monster({ id = "watch captain", peaceful = 1 })

