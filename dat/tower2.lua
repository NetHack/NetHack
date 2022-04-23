-- NetHack 3.7	tower.des	$NHDT-Date: 1432512784 2015/05/25 00:13:04 $  $NHDT-Branch: master $:$NHDT-Revision: 1.9 $
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
---.------+----
  |......|..|  
--------.------
|.S......+..S.|
---S---S---S---
  |.| |.| |.|  
  --- --- ---  
]] });
-- Random places are the 10 niches
local place = { {03,01},{07,01},{11,01},{01,03},{13,03},
	   {01,07},{13,07},{03,09},{07,09},{11,09} }
shuffle(place)

des.ladder("up", 11,05)
des.ladder("down", 03,07)
des.door("locked",10,04)
des.door("locked",09,07)
des.monster("&",place[10])
des.monster("&",place[1])
des.monster("hell hound pup",place[2])
des.monster("hell hound pup",place[3])
des.monster("winter wolf",place[4])
des.object({ id = "chest", coord = place[5],
             contents = function()
                des.object("amulet of life saving")
             end
});
des.object({ id = "chest", coord = place[6],
             contents = function()
                des.object("amulet of strangulation")
             end
});
des.object("water walking boots",place[7])
des.object("crystal plate mail",place[8])
des.object("spellbook of invisibility",place[9])
-- Walls in the tower are non diggable
des.non_diggable(selection.area(00,00,14,10))

