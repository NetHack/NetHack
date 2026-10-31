-- $NHDT-Header: NetHack 5.0 Arc-filb.lua 2026/06/20_22:35:38 NetHack-5.0:1.4 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
--
des.room({ type = "ordinary",
           contents = function()
              des.stair("up")
              des.object()
              des.monster("M")
           end
})

des.room({ type = "ordinary",
           contents = function()
              des.object()
              des.object()
              des.monster("M")
           end
})

des.room({ type = "ordinary",
           contents = function()
              des.object()
              des.trap()
              des.object()
              des.monster("M")
           end
})

des.room({ type = "ordinary",
           contents = function()
              des.stair("down")
              des.object()
              des.trap()
              des.monster("S")
              des.monster("human mummy")
           end
})

des.room({ type = "ordinary",
           contents = function()
              des.object()
              des.object()
              des.trap()
              des.monster("S")
           end
})

des.room({ type = "ordinary",
           contents = function()
              des.object()
              des.trap()
              des.monster("S")
           end
})

des.random_corridors()
