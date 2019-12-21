-- NetHack 3.6	Priest.des	$NHDT-Date: 1432512784 2015/05/25 00:13:04 $  $NHDT-Branch: master $:$NHDT-Revision: 1.11 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991-2 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
--
des.room({ type = "ordinary",
           contents = function()
              des.stair("up")
              des.object()
              des.monster("human zombie")
           end
})

des.room({ type = "ordinary",
           contents = function()
              des.object()
              des.object()
           end
})

des.room({ type = "ordinary",
           contents = function()
              des.object()
              des.trap()
              des.object()
              des.monster("human zombie")
           end
})

des.room({ type = "morgue",
           contents = function()
              des.stair("down")
              des.object()
              des.trap()
           end
})

des.room({ type = "ordinary",
           contents = function()
              des.object()
              des.object()
              des.trap()
              des.monster("wraith")
           end
})

des.room({ type = "morgue",
           contents = function()
              des.object()
              des.trap()
           end
})

des.random_corridors()
