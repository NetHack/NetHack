-- NetHack Ranger Ran-loca.lua	$NHDT-Date: 1652196010 2022/05/10 15:20:10 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.1 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "hardfloor")
--1234567890123456789012345678901234567890123456789012345678901234567890
des.map([[
              .......  .........  .......              
     ...................       ...................     
  ....        .......             .......        ....  
...    .....     .       .....       .     .....    ...
.   .......... .....  ...........  ..... ..........   .
.  ..  ..... ..........  .....  .......... .....  ..  .
.  .     .     .....       .       .....     .     .  .
.  .   .....         .............         .....   .  .
.  .  ................  .......  ................  .  .
.  .   .....            .......            .....   .  .
.  .     .    ......               ......    .     .  .
.  .     ...........   .........   ...........     .  .
.  .          ..........       ..........          .  .
.  ..  .....     .       .....       .     .....  ..  .
.   .......... .....  ...........  ..... ..........   .
.      ..... ..........  .....  .......... .....      .
.        .     .....       .       .....     .        .
...   .......           .......           .......   ...
  ..............     .............     ..............  
      .......  .......  .......  .......  .......      
]]);
-- Dungeon Description
des.region(selection.area(00,00,54,19), "lit")
-- Stairs
des.stair("up", 25,05)
des.stair("down", 27,18)
-- Non diggable walls
des.non_diggable(selection.area(00,00,54,19))
-- Objects
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
-- Random traps
des.trap("spiked pit")
des.trap("spiked pit")
des.trap("teleport")
des.trap("teleport")
des.trap("arrow")
des.trap("arrow")
-- Random monsters.
des.monster({ id = "wumpus", x=27, y=18, peaceful=0, asleep=1 })
des.monster({ id = "giant bat", peaceful=0 })
des.monster({ id = "giant bat", peaceful=0 })
des.monster({ id = "giant bat", peaceful=0 })
des.monster({ id = "giant bat", peaceful=0 })
des.monster({ id = "forest centaur", peaceful=0 })
des.monster({ id = "forest centaur", peaceful=0 })
des.monster({ id = "forest centaur", peaceful=0 })
des.monster({ id = "forest centaur", peaceful=0 })
des.monster({ id = "mountain centaur", peaceful=0 })
des.monster({ id = "mountain centaur", peaceful=0 })
des.monster({ id = "mountain centaur", peaceful=0 })
des.monster({ id = "mountain centaur", peaceful=0 })
des.monster({ id = "mountain centaur", peaceful=0 })
des.monster({ id = "mountain centaur", peaceful=0 })
des.monster({ id = "mountain centaur", peaceful=0 })
des.monster({ id = "mountain centaur", peaceful=0 })
des.monster({ id = "scorpion", peaceful=0 })
des.monster({ id = "scorpion", peaceful=0 })
des.monster({ id = "scorpion", peaceful=0 })
des.monster({ id = "scorpion", peaceful=0 })
des.monster({ class = "s", peaceful=0 })
des.monster({ class = "s", peaceful=0 })

