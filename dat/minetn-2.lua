-- NetHack mines minetn-2.lua	$NHDT-Date: 1652196030 2022/05/10 15:20:30 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.3 $
--	Copyright (c) 1989-95 by Jean-Christophe Collet
--	Copyright (c) 1991-95 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
-- Minetown variant 2
-- "Town Square"

des.room({ type = "ordinary", lit=1, x=3, y=3,
           xalign="center", yalign="center", w=31, h=15,
           contents = function()
              des.feature("fountain", 17, 5)
              des.feature("fountain", 13, 8)

              if percent(75) then
                 des.room({ type = "ordinary", x=2,y=0, w=2,h=2,
                            contents = function()
                               des.door({ state="closed", wall="west" })
                            end
                 })
              end

              if percent(75) then
                 des.room({ type = "ordinary", lit=0, x=5,y=0, w=2,h=2,
                            contents = function()
                               des.door({ state="closed", wall="south" })
                            end
                 })
              end

              if percent(75) then
                 des.room({ type = "ordinary", x=8,y=0, w=2,h=2,
                            contents = function()
                               des.door({ state="closed", wall="east" })
                            end
                 })
              end

              if percent(75) then
                 des.room({ type = "ordinary", lit=1, x=16,y=0, w=2,h=2,
                            contents = function()
                               des.door({ state="closed", wall="west" })
                            end
                 })
              end

              if percent(75) then
                 des.room({ type = "ordinary", lit=0, x=19,y=0, w=2,h=2,
                            contents = function()
                               des.door({ state="closed", wall="south" })
                            end
                 })
              end

              if percent(75) then
                 des.room({ type = "ordinary", x=22,y=0, w=2,h=2,
                            contents = function()
                               des.door({ state="closed", wall="south" })
                               des.monster("gnome")
                            end
                 })
              end

              if percent(75) then
                 des.room({ type = "ordinary", lit=0, x=25,y=0, w=2,h=2,
                            contents = function()
                               des.door({ state="closed", wall="east" })
                            end
                 })
              end

              if percent(75) then
                 des.room({ type = "ordinary", lit=1, x=2,y=5, w=2,h=2,
                            contents = function()
                               des.door({ state="closed", wall="north" })
                            end
                 })
              end

              if percent(75) then
                 des.room({ type = "ordinary", lit=1, x=5,y=5, w=2,h=2,
                            contents = function()
                               des.door({ state="closed", wall="south" })
                            end
                 })
              end

              if percent(75) then
                 des.room({ type = "ordinary", x=8,y=5, w=2,h=2,
                            contents = function()
                               des.door({ state="locked", wall="north" })
                               des.monster("gnome")
                            end
                 })
              end

              des.room({ type="shop", chance=90, lit=1, x=2,y=10, w=4,h=3,
                         contents = function()
                            des.door({ state="closed", wall="west" })
                         end
              });

              des.room({ type = "tool shop", chance=90, lit=1, x=23,y=10, w=4,h=3,
                         contents = function()
                            des.door({ state="closed", wall="east" })
                         end
              });

              des.room({ type = monkfoodshop(), chance=90, lit=1, x=24,y=5, w=3,h=4,
                         contents = function()
                            des.door({ state="closed", wall="north" })
                         end
              });

              des.room({ type = "candle shop", lit=1, x=11,y=10, w=4,h=3,
                         contents = function()
                            des.door({ state="closed", wall="east" })
                         end
              });

              if percent(75) then
                 des.room({ type = "ordinary", lit=0, x=7,y=10, w=3,h=3,
                            contents = function()
                               des.door({ state="locked", wall="north" })
                               des.monster("gnome")
                            end
                 });
              end

              des.room({ type = "temple", lit=1, x=19,y=5, w=4,h=4,
                         contents = function()
                            des.door({ state="closed", wall="north" })
                            des.altar({ x=02, y=02, align=align[1],type="shrine" })
                            des.monster("gnomish wizard")
                            des.monster("gnomish wizard")
                         end
              });

              if percent(75) then
                 des.room({ type = "ordinary", lit=1, x=18,y=10, w=4,h=3,
                            contents = function()
                               des.door({ state="locked", wall="west" })
                               des.monster("gnome lord")
                            end
                 });
              end

              -- The Town Watch
              des.monster({ id = "watchman", peaceful = 1 })
              des.monster({ id = "watchman", peaceful = 1 })
              des.monster({ id = "watchman", peaceful = 1 })
              des.monster({ id = "watchman", peaceful = 1 })
              des.monster({ id = "watch captain", peaceful = 1 })
           end
});

des.room({ contents = function()
              des.stair("up")
                      end
});

des.room({ contents = function()
              des.stair("down")
              des.trap()
              des.monster("gnome")
              des.monster("gnome")
                      end
});

des.room({ contents = function()
              des.monster("dwarf")
                      end
});

des.room({ contents = function()
              des.trap()
              des.monster("gnome")
                      end
});

des.random_corridors()

