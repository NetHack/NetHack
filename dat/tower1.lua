-- NetHack tower tower1.lua	$NHDT-Date: 1652196037 2022/05/10 15:20:37 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.1 $
--	Copyright (c) 1989 by Jean-Christophe Collet
-- NetHack may be freely redistributed.  See license for details.
--
--
-- Upper stage of Vlad's tower
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "noteleport", "hardfloor", "solidify")
des.map({ halign = "half-left", valign = "center", map = [[
  --- --- ---  
  |.| |.| |.|  
---S---S---S---
|.......+.+...|
---+-----.-----
  |...\.|.+.|  
---+-----.-----
|.......+.+...|
---S---S---S---
  |.| |.| |.|  
  --- --- ---  
]] });

local niches = { {03,01}, {03,09}, {07,01}, {07,09}, {11,01}, {11,09} };
shuffle(niches);

des.ladder("down", 11,05)
-- The lord and his court
des.monster("Vlad the Impaler", 06, 05)
des.monster("V",niches[1])
des.monster("V",niches[2])
des.monster("V",niches[3])
des.monster("V",niches[4])
des.monster("V",niches[5])
des.monster("V",niches[6])
-- The doors
des.door("closed",08,03)
des.door("closed",10,03)
des.door("closed",03,04)
des.door("locked",10,05)
des.door("locked",08,07)
des.door("locked",10,07)
des.door("closed",03,06)
-- treasures
des.object("chest", 07,05)

des.object("chest",niches[6])
des.object("chest",niches[1])
des.object("chest",niches[2])
des.object("chest",niches[3])
des.object({ id = "chest", coord=niches[4],
             contents = function()
                des.object({ id = "wax candle", quantity=math.random(4,8) })
             end
});
des.object({ id = "chest", coord=niches[5],
             contents = function()
                des.object({ id = "tallow candle", quantity=math.random(4,8) })
             end
});
-- We have to protect the tower against outside attacks
des.non_diggable(selection.area(00,00,14,10))
