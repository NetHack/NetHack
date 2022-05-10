-- NetHack tower tower3.lua	$NHDT-Date: 1652196038 2022/05/10 15:20:38 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.1 $
--	Copyright (c) 1989 by Jean-Christophe Collet
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "noteleport", "hardfloor", "solidify")
des.map({ halign = "half-left", valign = "center", map = [[
    --- --- ---    
    |.| |.| |.|    
  ---S---S---S---  
  |.S.........S.|  
-----.........-----
|...|.........+...|
|.---.........---.|
|.|.S.........S.|.|
|.---S---S---S---.|
|...|.|.|.|.|.|...|
---.---.---.---.---
  |.............|  
  ---------------  
]] });
-- Random places are the 10 niches
local place = { {05,01},{09,01},{13,01},{03,03},{15,03},
	   {03,07},{15,07},{05,09},{09,09},{13,09} }

des.levregion({ type="branch", region={02,05,02,05} })
des.ladder("up", 05,07)
-- Entry door is, of course, locked
des.door("locked",14,05)
-- Let's put a dragon behind the door, just for the fun...
des.monster("D", 13, 05)
des.monster({ x=12, y=04 })
des.monster({ x=12, y=06 })
des.monster()
des.monster()
des.monster()
des.monster()
des.monster()
des.monster()
des.object("long sword",place[4])
des.trap({ coord = place[4] })
des.object("lock pick",place[1])
des.trap({ coord = place[1] })
des.object("elven cloak",place[2])
des.trap({ coord = place[2] })
des.object("blindfold",place[3])
des.trap({ coord = place[3] })
-- Walls in the tower are non diggable
des.non_diggable(selection.area(00,00,18,12))
