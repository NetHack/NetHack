-- NetHack mines minetn-5.lua	$NHDT-Date: 1652196031 2022/05/10 15:20:31 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.5 $
--	Copyright (c) 1989-95 by Jean-Christophe Collet
--	Copyright (c) 1991-95 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
-- "Grotto Town" by Kelly Bailey

des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel");

des.map([[
-----         ---------                                                    
|...---  ------.......--    -------                       ---------------  
|.....----.........--..|    |.....|          -------      |.............|  
--..-....-.----------..|    |.....|          |.....|     --+---+--.----+-  
 --.--.....----     ----    |.....|  ------  --....----  |..-...--.-.+..|  
  ---.........----  -----   ---+---  |..+.|   ---..-..----..---+-..---..|  
    ----.-....|..----...--    |.|    |..|.|    ---+-.....-+--........--+-  
       -----..|....-.....---- |.|    |..|.------......--................|  
    ------ |..|.............---.--   ----.+..|-.......--..--------+--..--  
    |....| --......---...........-----  |.|..|-...{....---|.........|..--  
    |....|  |........-...-...........----.|..|--.......|  |.........|...|  
    ---+--------....-------...---......--.-------....---- -----------...|  
 ------.---...--...--..-..--...-..---...|.--..-...-....------- |.......--  
 |..|-.........-..---..-..---.....--....|........---...-|....| |.-------   
 |..+...............-+---+-----..--..........--....--...+....| |.|...S.    
-----.....{....----...............-...........--...-...-|....| |.|...|     
|..............-- --+--.---------.........--..-........------- |.--+-------
-+-----.........| |...|.|....|  --.......------...|....---------.....|....|
|...| --..------- |...|.+....|   ---...---    --..|...--......-...{..+..-+|
|...|  ----       ------|....|     -----       -----.....----........|..|.|
-----                   ------                     -------  ---------------
]]);

if percent(75) then
  if percent(50) then
    des.terrain(selection.line(25,8, 25,9), "|")
  else
    des.terrain(selection.line(16,13, 17,13), "-")
  end
end
if percent(75) then
  if percent(50) then
    des.terrain(selection.line(36,10, 36,11), "|")
  else
    des.terrain(selection.line(32,15, 33,15), "-")
  end
end
if percent(50) then
  des.terrain(selection.area(21,4, 22,5), ".")
  des.terrain(selection.line(14,9, 14,10), "|")
end
if percent(50) then
  des.terrain({46,13}, "|")
  des.terrain(selection.line(43,5, 47,5), "-")
  des.terrain(selection.line(42,6, 46,6), ".")
  des.terrain(selection.line(46,7, 47,7), ".")
end
if percent(50) then
  des.terrain(selection.area(69,11, 71,11), "-")
end

des.stair("up", 01,01)
des.stair("down", 46,03)
des.feature("fountain", 50,09)
des.feature("fountain", 10,15)
des.feature("fountain", 66,18)

des.region(selection.area(00,00,74,20),"unlit")
des.region(selection.area(09,13,11,17),"lit")
des.region(selection.area(08,14,12,16),"lit")
des.region(selection.area(49,07,51,11),"lit")
des.region(selection.area(48,08,52,10),"lit")
des.region(selection.area(64,17,68,19),"lit")
des.region(selection.area(37,13,39,17),"lit")
des.region(selection.area(36,14,40,17),"lit")
des.region(selection.area(59,02,72,10),"lit")

des.monster({ id = "watchman", peaceful = 1 })
des.monster({ id = "watchman", peaceful = 1 })
des.monster({ id = "watchman", peaceful = 1 })
des.monster({ id = "watchman", peaceful = 1 })
des.monster({ id = "watch captain", peaceful = 1 })
des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome lord")
des.monster("gnome lord")
des.monster("dwarf")
des.monster("dwarf")
des.monster("dwarf")

-- The shops
des.region({ region={25,17, 28,19}, lit=1, type="candle shop", filled=1 })
des.door("closed",24,18)
des.region({ region={59, 9, 67,10}, lit=1, type="shop", filled=1 })
des.door("closed",66,08)
des.region({ region={57,13, 60,15}, lit=1, type="tool shop", filled=1 })
des.door("closed",56,14)
des.region({ region={05,09, 08,10}, lit=1, type=monkfoodshop(), filled=1 })
des.door("closed",07,11)
-- Gnome homes
des.door("closed",04,14)
des.door("locked",01,17)
des.monster("gnomish wizard", 02, 19)
des.door("locked",20,16)
des.monster("G", 20, 18)
des.door("random",21,14)
des.door("random",25,14)
des.door("random",42,08)
des.door("locked",40,05)
des.monster("G", 38, 07)
des.door("random",59,03)
des.door("random",58,06)
des.door("random",63,03)
des.door("random",63,05)
des.door("locked",71,03)
des.door("locked",71,06)
des.door("closed",69,04)
des.door("closed",67,16)
des.monster("gnomish wizard", 67, 14)
des.object("=", 70, 14)
des.door("locked",69,18)
des.monster("gnome lord", 71, 19)
des.door("locked",73,18)
des.object("chest", 73, 19)
des.door("locked",50,06)
des.object("(", 50, 03)
des.object({ id = "statue", x=38, y=15, montype="gnome king", historic=1 })
-- Temple
des.region({ region={29,02, 33,04}, lit=1, type="temple", filled=1 })
des.door("closed",31,05)
des.altar({ x=31,y=03, align=align[1], type="shrine" })
