-- NetHack gehennom juiblex.lua	$NHDT-Date: 1652196026 2022/05/10 15:20:26 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.5 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1992 by M. Stephenson and Izchak Miller
-- NetHack may be freely redistributed.  See license for details.
--

des.level_flags("mazelevel", "shortsighted", "noflip")
-- des.level_init(mines,'.','}',true,true,unlit,false)
des.level_init({ style = "swamp", lit = 0 });
-- guarantee at least one open spot to ensure successful stair placement
des.map({ halign = "left", valign = "bottom", map = [[
xxxxxxxx
xx...xxx
xxx...xx
xxxx.xxx
xxxxxxxx
]] });
des.object("boulder")
des.map({ halign = "right", valign = "top", map = [[
xxxxxxxx
xxxx.xxx
xxx...xx
xx...xxx
xxxxxxxx
]] });
des.object("boulder")
-- lair
des.map([[
xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
xxxx.xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xxxx
xxx...xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx...xxx
xxxx.xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xxxx
xxxxxxxxxxxxxxxxxxxxxxxx}}}xxxxxxxxxxxxxxx}}}}}xxxx
xxxxxxxxxxxxxxxxxxxxxxx}}}}}xxxxxxxxxxxxx}.....}xxx
xxxxxxxxxxxxxxxxxxxxxx}}...}}xxxxxxxxxxx}..P.P..}xx
xxxxxxxxxxxxxxxxxxxxx}}..P..}}xxxxxxxxxxx}.....}xxx
xxxxxxxxxxxxxxxxxxxxx}}.P.P.}}xxxxxxxxxxxx}...}xxxx
xxxxxxxxxxxxxxxxxxxxx}}..P..}}xxxxxxxxxxxx}...}xxxx
xxxxxxxxxxxxxxxxxxxxxx}}...}}xxxxxxxxxxxxxx}}}xxxxx
xxxxxxxxxxxxxxxxxxxxxxx}}}}}xxxxxxxxxxxxxxxxxxxxxxx
xxxxxxxxxxxxxxxxxxxxxxxx}}}xxxxxxxxxxxxxxxxxxxxxxxx
xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
xxxx.xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xxxx
xxx...xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx...xxx
xxxx.xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xxxx
xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
]]);
-- Random registers
local monster = { "j","b","P","F" }
shuffle(monster)

local place = selection.new();
place:set(04,02);
place:set(46,02);
place:set(04,15);
place:set(46,15);

-- Dungeon description
des.region({ region={00,00,50,17}, lit=0, type="swamp", filled=2 })
des.levregion({ region = {01,00,11,20}, region_islev=1, exclude={0,0,50,17}, type="stair-down" });
des.levregion({ region = {69,00,79,20}, region_islev=1, exclude={0,0,50,17}, type="stair-up" });
des.levregion({ region = {01,00,11,20}, region_islev=1, exclude={0,0,50,17}, type="branch" });
des.teleport_region({ region = {01,00,11,20}, region_islev=1, exclude={0,0,50,17},dir="up" })
des.teleport_region({ region = {69,00,79,20}, region_islev=1, exclude={0,0,50,17},dir="down" })
des.feature("fountain", place:rndcoord(1))
des.monster({ id = "giant mimic", coord = place:rndcoord(1), appear_as = "ter:fountain" })
des.monster({ id = "giant mimic", coord = place:rndcoord(1), appear_as = "ter:fountain" })
des.monster({ id = "giant mimic", coord = place:rndcoord(1), appear_as = "ter:fountain" })
-- The demon of the swamp
des.monster("Juiblex",25,08)
-- And a couple demons
des.monster("lemure",43,08)
des.monster("lemure",44,08)
des.monster("lemure",45,08)
-- Some liquids and gems
des.object("*",43,06)
des.object("*",45,06)
des.object("!",43,09)
des.object("!",44,09)
des.object("!",45,09)
-- And lots of blobby monsters
des.monster(monster[4],25,06)
des.monster(monster[1],24,07)
des.monster(monster[2],26,07)
des.monster(monster[3],23,08)
des.monster(monster[3],27,08)
des.monster(monster[2],24,09)
des.monster(monster[1],26,09)
des.monster(monster[4],25,10)
des.monster("j")
des.monster("j")
des.monster("j")
des.monster("j")
des.monster("P")
des.monster("P")
des.monster("P")
des.monster("P")
des.monster("b")
des.monster("b")
des.monster("b")
des.monster("F")
des.monster("F")
des.monster("F")
des.monster("m")
des.monster("m")
des.monster("jellyfish")
des.monster("jellyfish")
-- Some random objects
des.object("!")
des.object("!")
des.object("!")
des.object("%")
des.object("%")
des.object("%")
des.object("boulder")
-- Some traps
des.trap("sleep gas")
des.trap("sleep gas")
des.trap("anti magic")
des.trap("anti magic")
des.trap("magic")
des.trap("magic")
