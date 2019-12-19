-- NetHack 3.6	Barb.des	$NHDT-Date: 1432512784 2015/05/25 00:13:04 $  $NHDT-Branch: master $:$NHDT-Revision: 1.9 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel");

des.map([[
                                                                            
                               .............                                
                             ..................                             
        ....              .........................          ....           
      .......          ..........................           .......         
      ......             ........................          .......          
      ..  ......................................             ..             
       ..                 .....................             ..              
        ..                 ..................              ..               
         ..         ..S...S..............   ................                
          ..                   ........                ...                  
       .........                                         ..                 
       ......  ..                                         ...  ....         
      .. ...    ..                             ......       ........        
   ....          .. ..................        ........       ......         
  ......          ......................       ......         ..            
   ....             ..................              ...........             
                      ..............                                        
                        ...........                                         
                                                                            
]]);
-- Dungeon Description
des.region(selection.area(00,00,75,19), "unlit")
-- Secret doors
des.door("locked",22,09)
des.door("locked",26,09)
-- Stairs
des.stair("up", 36,05)
-- The altar.  Unattended.
des.altar({ x=63,y=04,align="noncoaligned", type="altar" })
des.non_diggable(selection.area(00,00,75,19))
-- Objects
des.object({ id = "luckstone", x=63, y=04,buc="blessed",spe=0,name="The Heart of Ahriman" })
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
des.monster({ id = "Thoth Amon", x=63, y=04, peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ id = "ogre", peaceful=0 })
des.monster({ class = "O", peaceful=0 })
des.monster({ class = "O", peaceful=0 })
des.monster({ id = "rock troll", peaceful=0 })
des.monster({ id = "rock troll", peaceful=0 })
des.monster({ id = "rock troll", peaceful=0 })
des.monster({ id = "rock troll", peaceful=0 })
des.monster({ id = "rock troll", peaceful=0 })
des.monster({ id = "rock troll", peaceful=0 })
des.monster({ id = "rock troll", peaceful=0 })
des.monster({ id = "rock troll", peaceful=0 })
des.monster({ class = "T", peaceful=0 })
des.wallify()

