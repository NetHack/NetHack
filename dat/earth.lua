-- NetHack endgame earth.lua	$NHDT-Date: 1652196025 2022/05/10 15:20:25 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.1 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1992,1993 by Izchak Miller, David Cohrs,
--                      and Timo Hakulinen
-- NetHack may be freely redistributed.  See license for details.
--
--
-- These are the ENDGAME levels: earth, air, fire, water, and astral.
-- The top-most level, the Astral Level, has 3 temples and shrines.
-- Players are supposed to sacrifice the Amulet of Yendor on the appropriate
-- shrine.

des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "noteleport", "hardfloor", "shortsighted")

des.message("Well done, mortal!")
des.message("But now thou must face the final Test...")
des.message("Prove thyself worthy or perish!")

-- The player lands, upon arrival, in the
-- lower-right cavern.  The location of the
-- portal to the next level is randomly chosen.
-- This map has no visible outer boundary, and
-- is mostly diggable "rock".
des.map([[
                                                                            
  ...                                                                       
 ....                ..                                                     
 .....             ...                                      ..              
  ....              ....                                     ...            
   ....              ...                ....                 ...      .     
    ..                ..              .......                 .      ..     
                                      ..  ...                        .      
              .                      ..    .                         ...    
             ..  ..                  .     ..                         .     
            ..   ...                        .                               
            ...   ...                                                       
              .. ...                                 ..                     
               ....                                 ..                      
                          ..                                       ...      
                         ..                                       .....     
  ...                                                              ...      
 ....                                                                       
   ..                                                                       
                                                                            
]]);

des.replace_terrain({ region={0,0, 75,19}, fromterrain=" ", toterrain=".", lit=0, chance=5 })

--  Since there are no stairs, this forces the hero's initial placement
des.teleport_region({region = {69,16,69,16} })
des.levregion({ region = {0,0,75,19}, exclude = {65,13,75,19}, type="portal", name="air" })
--  Some helpful monsters.  Making sure a
--  pick axe and at least one wand of digging
--  are available.
des.monster("Elvenking", 67,16)
des.monster("minotaur", 67,14)
--  An assortment of earth-appropriate nasties
--  in each cavern.
des.monster({ id = "earth elemental", x = 52, y = 13, peaceful = 0 })
des.monster({ id = "earth elemental", x = 53, y = 13, peaceful = 0 })
des.monster("rock troll", 53,12)
des.monster("stone giant", 54,12)
--
des.monster("pit viper", 70,05)
des.monster("barbed devil", 69,06)
des.monster("stone giant", 69,08)
des.monster("stone golem", 71,08)
des.monster("pit fiend", 70,09)
des.monster({ id = "earth elemental", x = 70, y = 08, peaceful = 0 })
--
des.monster({ id = "earth elemental", x = 60, y = 03, peaceful = 0 })
des.monster("stone giant", 61,04)
des.monster({ id = "earth elemental", x = 62, y = 04, peaceful = 0 })
des.monster({ id = "earth elemental", x = 61, y = 05, peaceful = 0 })
des.monster("scorpion", 62,05)
des.monster("rock piercer", 63,05)
--
des.monster("umber hulk", 40,05)
des.monster("dust vortex", 42,05)
des.monster("rock troll", 38,06)
des.monster({ id = "earth elemental", x = 39, y = 06, peaceful = 0 })
des.monster({ id = "earth elemental", x = 41, y = 06, peaceful = 0 })
des.monster({ id = "earth elemental", x = 38, y = 07, peaceful = 0 })
des.monster("stone giant", 39,07)
des.monster({ id = "earth elemental", x = 43, y = 07, peaceful = 0 })
des.monster("stone golem", 37,08)
des.monster("pit viper", 43,08)
des.monster("pit viper", 43,09)
des.monster("rock troll", 44,10)
--
des.monster({ id = "earth elemental", x = 02, y = 01, peaceful = 0 })
des.monster({ id = "earth elemental", x = 03, y = 01, peaceful = 0 })
des.monster("stone golem", 01,02)
des.monster({ id = "earth elemental", x = 02, y = 02, peaceful = 0 })
des.monster("rock troll", 04,03)
des.monster("rock troll", 03,03)
des.monster("pit fiend", 03,04)
des.monster({ id = "earth elemental", x = 04, y = 05, peaceful = 0 })
des.monster("pit viper", 05,06)
--
des.monster({ id = "earth elemental", x = 21, y = 02, peaceful = 0 })
des.monster({ id = "earth elemental", x = 21, y = 03, peaceful = 0 })
des.monster("minotaur", 21,04)
des.monster({ id = "earth elemental", x = 21, y = 05, peaceful = 0 })
des.monster("rock troll", 22,05)
des.monster({ id = "earth elemental", x = 22, y = 06, peaceful = 0 })
des.monster({ id = "earth elemental", x = 23, y = 06, peaceful = 0 })
--
des.monster("pit viper", 14,08)
des.monster("barbed devil", 14,09)
des.monster({ id = "earth elemental", x = 13, y = 10, peaceful = 0 })
des.monster("rock troll", 12,11)
des.monster({ id = "earth elemental", x = 14, y = 12, peaceful = 0 })
des.monster({ id = "earth elemental", x = 15, y = 13, peaceful = 0 })
des.monster("stone giant", 17,13)
des.monster("stone golem", 18,13)
des.monster("pit fiend", 18,12)
des.monster({ id = "earth elemental", x = 18, y = 11, peaceful = 0 })
des.monster({ id = "earth elemental", x = 18, y = 10, peaceful = 0 })
--
des.monster("barbed devil", 02,16)
des.monster({ id = "earth elemental", x = 03, y = 16, peaceful = 0 })
des.monster("rock troll", 02,17)
des.monster({ id = "earth elemental", x = 04, y = 17, peaceful = 0 })
des.monster({ id = "earth elemental", x = 04, y = 18, peaceful = 0 })

des.object("boulder")

