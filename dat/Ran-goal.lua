-- NetHack Ranger Ran-goal.lua	$NHDT-Date: 1652196010 2022/05/10 15:20:10 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.1 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel");

des.map([[
                                                                            
  ...                                                                  ...  
 .......................................................................... 
  ...                                +                                 ...  
   .     ............     .......    .                   .......        .   
   .  .............................  .       ........   .........S..    .   
   .   ............    .  ......     .       .      .    .......   ..   .   
   .     .........     .   ....      +       . ...  .               ..  .   
   .        S          .         .........   .S.    .S...............   .   
   .  ...   .     ...  .         .........          .                   .   
   . ........    .....S.+.......+....\....+........+.                   .   
   .  ...         ...    S       .........           ..      .....      .   
   .                    ..       .........            ..      ......    .   
   .      .......     ...            +       ....    ....    .......... .   
   . ..............  ..              .      ......  ..  .............   .   
   .     .............               .     ..........          ......   .   
  ...                                +                                 ...  
 .......................................................................... 
  ...                                                                  ...  
                                                                            
]]);
-- Dungeon Description
des.region(selection.area(00,00,75,19), "lit")
-- Stairs
des.stair("up", 19,10)
-- Non diggable walls
des.non_diggable(selection.area(00,00,75,19))
-- Objects
des.object({ id = "bow", x=37, y=10, buc="blessed", spe=0, name="The Longbow of Diana" })
des.object("chest", 37, 10)
des.object({ coord = { 36, 09 } })
des.object({ coord = { 36, 10 } })
des.object({ coord = { 36, 11 } })
des.object({ coord = { 37, 09 } })
des.object({ coord = { 37, 11 } })
des.object({ coord = { 38, 09 } })
des.object({ coord = { 38, 10 } })
des.object({ coord = { 38, 11 } })
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
-- doors
des.door("locked",12,08)
des.door("closed",22,10)
des.door("locked",24,10)
des.door("closed",25,11)
des.door("closed",32,10)
des.door("closed",37,03)
des.door("closed",37,07)
des.door("closed",37,13)
des.door("closed",37,16)
des.door("closed",42,10)
des.door("locked",46,08)
des.door("closed",51,10)
des.door("locked",53,08)
des.door("closed",65,05)
-- Random monsters.
des.monster({ id = "Scorpius", x=37, y=10, peaceful=0 })
des.monster({ id = "forest centaur", x=36, y=09, peaceful=0 })
des.monster({ id = "forest centaur", x=36, y=10, peaceful=0 })
des.monster({ id = "forest centaur", x=36, y=11, peaceful=0 })
des.monster({ id = "forest centaur", x=37, y=09, peaceful=0 })
des.monster({ id = "forest centaur", x=37, y=11, peaceful=0 })
des.monster({ id = "forest centaur", x=38, y=09, peaceful=0 })
des.monster({ id = "mountain centaur", x=38, y=10, peaceful=0 })
des.monster({ id = "mountain centaur", x=38, y=11, peaceful=0 })
des.monster({ id = "mountain centaur", x=02, y=02, peaceful=0 })
des.monster({ id = "mountain centaur", x=71, y=02, peaceful=0 })
des.monster({ id = "mountain centaur", x=02, y=16, peaceful=0 })
des.monster({ id = "mountain centaur", x=71, y=16, peaceful=0 })
des.monster({ id = "forest centaur", peaceful=0 })
des.monster({ id = "forest centaur", peaceful=0 })
des.monster({ id = "mountain centaur", peaceful=0 })
des.monster({ id = "mountain centaur", peaceful=0 })
des.monster({ class = "C", peaceful=0 })
des.monster({ class = "C", peaceful=0 })
des.monster({ id = "scorpion", x=03, y=02, peaceful=0 })
des.monster({ id = "scorpion", x=72, y=02, peaceful=0 })
des.monster({ id = "scorpion", x=03, y=17, peaceful=0 })
des.monster({ id = "scorpion", x=72, y=17, peaceful=0 })
des.monster({ id = "scorpion", x=41, y=10, peaceful=0 })
des.monster({ id = "scorpion", x=33, y=09, peaceful=0 })
des.monster({ id = "scorpion", peaceful=0 })
des.monster({ id = "scorpion", peaceful=0 })
des.monster({ class = "s", peaceful=0 })

des.wallify()
