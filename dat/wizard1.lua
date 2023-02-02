-- NetHack yendor wizard1.lua	$NHDT-Date: 1652196039 2022/05/10 15:20:39 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.3 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1992 by M. Stephenson and Izchak Miller
-- NetHack may be freely redistributed.  See license for details.
--
--
-- The top (real) wizard level.
-- Keeping the Moat for old-time's sake
des.level_init({ style="mazegrid", bg ="-" });

des.level_flags("mazelevel", "noteleport", "hardfloor")

local tmpbounds = selection.match("-");
local bnds = tmpbounds:bounds();
local bounds2 = selection.fillrect(bnds.lx, bnds.ly + 1, bnds.hx - 2, bnds.hy - 1);

local wiz1 = des.map({ halign = "center", valign = "center", map = [[
----------------------------x
|.......|..|.........|.....|x
|.......S..|.}}}}}}}.|.....|x
|..--S--|..|.}}---}}.|---S-|x
|..|....|..|.}--.--}.|..|..|x
|..|....|..|.}|...|}.|..|..|x
|..--------|.}--.--}.|..|..|x
|..|.......|.}}---}}.|..|..|x
|..S.......|.}}}}}}}.|..|..|x
|..|.......|.........|..|..|x
|..|.......|-----------S-S-|x
|..|.......S...............|x
----------------------------x
]], contents = function(rm)
   des.levregion({ type="stair-up", region={01,00,79,20}, region_islev=1, exclude={0,0,28,12} })
   des.levregion({ type="stair-down", region={01,00,79,20}, region_islev=1, exclude={0,0,28,12} })
   des.levregion({ type="branch", region={01,00,79,20}, region_islev=1, exclude={0,0,28,12} })
   des.teleport_region({ region={01,00,79,20}, region_islev=1, exclude={0,0,27,12} })
   des.region({ region={12,01, 20,09}, lit=0, type="morgue", filled=2, contents=function()
                   local sdwall = { "south", "west", "east" };
                   des.door({ wall = sdwall[math.random(1, #sdwall)], state = "secret" });
   end })
   -- another region to constrain monster arrival
   des.region({ region={01,01, 10,11}, lit=0, type="ordinary", arrival_room=true })
   des.mazewalk(28,05,"east")
   des.ladder("down", 06,05)
   -- Non diggable walls
   -- Walls inside the moat stay diggable
   des.non_diggable(selection.area(00,00,11,12))
   des.non_diggable(selection.area(11,00,21,00))
   des.non_diggable(selection.area(11,10,27,12))
   des.non_diggable(selection.area(21,00,27,10))
   -- Non passable walls
   des.non_passwall(selection.area(00,00,11,12))
   des.non_passwall(selection.area(11,00,21,00))
   des.non_passwall(selection.area(11,10,27,12))
   des.non_passwall(selection.area(21,00,27,10))
   -- The wizard and his guards
   des.monster({ id = "Wizard of Yendor", x=16, y=05, asleep=1 })
   des.monster("hell hound", 15, 05)
   des.monster("vampire lord", 17, 05)
   -- The local treasure
   des.object("Book of the Dead", 16, 05)
   -- Surrounding terror
   des.monster("kraken", 14, 02)
   des.monster("giant eel", 17, 02)
   des.monster("kraken", 13, 04)
   des.monster("giant eel", 13, 06)
   des.monster("kraken", 19, 04)
   des.monster("giant eel", 19, 06)
   des.monster("kraken", 15, 08)
   des.monster("giant eel", 17, 08)
   des.monster("piranha", 15, 02)
   des.monster("piranha", 19, 08)
   -- Random monsters
   des.monster("D")
   des.monster("H")
   des.monster("&")
   des.monster("&")
   des.monster("&")
   des.monster("&")
   -- And to make things a little harder.
   des.trap("board",16,04)
   des.trap("board",16,06)
   des.trap("board",15,05)
   des.trap("board",17,05)
   -- Random traps.
   des.trap("spiked pit")
   des.trap("sleep gas")
   des.trap("anti magic")
   des.trap("magic")
   -- Some random loot.
   des.object("ruby")
   des.object("!")
   des.object("!")
   des.object("?")
   des.object("?")
   des.object("+")
   des.object("+")
   des.object("+")
end
});

local protected = bounds2:negate() | wiz1;
hell_tweaks(protected);
