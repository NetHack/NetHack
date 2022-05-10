-- NetHack mines minetn-6.lua	$NHDT-Date: 1652196031 2022/05/10 15:20:31 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.3 $
--	Copyright (c) 1989-95 by Jean-Christophe Collet
--	Copyright (c) 1991-95 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
-- "Bustling Town" by Kelly Bailey

des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "inaccessibles")

des.level_init({ style="mines", fg=".", bg="-", smoothed=true, joined=true,lit=1,walled=true })

des.map({ halign = "center", valign = "top", map = [[
.-----................----------------.-
.|...|................|...|..|...|...|..
.|...+..--+--.........|...|..|...|...|..
.|...|..|...|..-----..|...|..|-+---+--..
.-----..|...|--|...|..--+---+-.........|
........|...|..|...+.............-----..
........-----..|...|......--+-...|...|..
.----...|...|+------..{...|..|...+...|..
.|..+...|...|.............|..|...|...|..
.|..|...|...|-+-.....---+-------------.|
.----...--+--..|..-+-|..................
...|........|..|..|..|----....---------.
...|..T.....----..|..|...+....|......|-.
...|-....{........|..|...|....+......|-.
...--..-....T.....--------....|......|-.
.......--.....................----------
]] });

des.region(selection.area(00,00,38,15),"lit")
des.levregion({ type="stair-up", region={01,03,20,19}, region_islev=1, exclude={0,0,39,15} })
des.levregion({ type="stair-down", region={61,03,75,19}, region_islev=1, exclude={0,0,39,15} })
des.feature("fountain" ,22,07)
des.feature("fountain", 09,13)
des.region(selection.area(13,5,14,6),"unlit")
des.region({ region={09,07, 11,09}, lit=1, type="candle shop", filled=1 })
des.region({ region={16,04, 18,06}, lit=1, type="tool shop", filled=1 })
des.region({ region={23,01, 25,03}, lit=1, type="shop", filled=1 })
des.region({ region={22,12, 24,13}, lit=1, type=monkfoodshop(), filled=1 })
des.region({ region={31,12, 36,14}, lit=1, type="temple", filled=1 })
des.altar({ x=35,y=13,align=align[1],type="shrine"})

des.door("closed",5,2)
des.door("locked",4,8)
des.door("closed",10,2)
des.door("closed",10,10)
des.door("locked",13,7)
des.door("locked",14,9)
des.door("closed",19,5)
des.door("closed",19,10)
des.door("closed",24,4)
des.door("closed",24,9)
des.door("closed",25,12)
des.door("closed",28,4)
des.door("locked",28,6)
des.door("closed",30,13)
des.door("closed",31,3)
des.door("closed",35,3)
des.door("closed",33,7)

des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome", 14, 6)
des.monster("gnome lord", 14, 5)
des.monster("gnome", 27, 8)
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

