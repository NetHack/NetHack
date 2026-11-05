-- $NHDT-Header: NetHack 5.0 Pri-filb.lua 2026/06/20_22:35:38 NetHack-5.0:1.4 $
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
              des.monster("wraith")
           end
})

des.room({ type = "morgue",
           contents = function()
              des.object()
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
              des.monster("wraith")
           end
})

des.room({ type = "morgue",
           contents = function()
              des.stair("down")
              des.object()
              des.object()
              des.trap()
           end
})

des.room({ type = "ordinary",
           contents = function()
              des.object()
              des.object()
              des.trap()
              des.monster("human zombie")
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
