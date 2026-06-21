-- NetHack Rogue Rog-filb.lua	$NHDT-Date: 1781994872 2026/06/20 22:34:32 $  $NHDT-Branch: NetHack-5.0 $:$NHDT-Revision: 1.2 $
--	Copyright (c) 1992 by Dean Luick
-- NetHack may be freely redistributed.  See license for details.
--
--
des.room({ type = "ordinary",
           contents = function()
              des.stair("up")
              des.object()
              des.monster({ id = "leprechaun", peaceful=0 })
           end
})

des.room({ type = "ordinary",
           contents = function()
              des.object()
              des.object()
              des.monster({ id = "leprechaun", peaceful=0 })
              des.monster({ id = "guardian naga", peaceful=0 })
           end
})

des.room({ type = "ordinary",
           contents = function()
              des.object()
              des.trap()
              des.trap()
              des.object()
              des.monster({ id = "water nymph", peaceful=0 })
           end
})

des.room({ type = "ordinary",
           contents = function()
              des.stair("down")
              des.object()
              des.trap()
              des.trap()
              des.monster({ class = "l", peaceful=0 })
              des.monster({ id = "guardian naga", peaceful=0 })
           end
})

des.room({ type = "ordinary",
           contents = function()
              des.object()
              des.object()
              des.trap()
              des.trap()
              des.monster({ id = "leprechaun", peaceful=0 })
           end
})

des.room({ type = "ordinary",
           contents = function()
              des.object()
              des.trap()
              des.trap()
              des.monster({ id = "leprechaun", peaceful=0 })
              des.monster({ id = "water nymph", peaceful=0 })
           end
})

des.random_corridors()
