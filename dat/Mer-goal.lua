des.level_init({ style = "solidfill", fg = " " });
des.level_flags("mazelevel")
des.map([[
TTTTTTTTT}}}}}TTT-----------------------------------------------------------
TTTTTTTT..}}}}}.T|................T|.|.|.|............+...........+........|
TTTTTTTT...}}}}}.F....{.....T.T...T|.+.+.|............|...........|........|
TTTTTT...}}}}}...F..........T.T...T|.|.|.|............|-------------------S|
TTTTTT.}}}}}...TT|................T|.F.F.|............|..................|+|
TTTTTT..}}}}}.TTTF..........T.T...T---+--|............|..................|+|
TTTTTTT.}}}}}.TTTF..........T.T...TTTT.TT|-----+------|..................|+|
TTTTTTT..}}}..TTT|.........TT.TT.........|............|..................|+|
TTTTTTTT..}..TTTTF.TT.TT.TTT...TTT.TT.TT.|............|..................|+|
T........}}}.....+...........{...........+............+.................\S+|
TTTTTTTT..}..TTTTF.TT.TT.TTT...TTT.TT.TT.|............|..................|+|
TTTTTTT..}}}..TTT|.........TT.TT.........|............|..................|+|
TTTTTTT.}}}}}..TTF..........T.T...TTTT.TT|-----+------|..................|+|
TTTTTTT..}}}}}...F..........T.T...T---+--|............|..................|+|
TTTTTTTT...}}}}}.|................T|.F.F.|............|..................|+|
TTTTTTT...}}}}}..F..........T.T...T|.|.|.|............|..................|+|
TTTTT...}}}}}...TF....{.....T.T...T|.+.+.|............|-------------------S|
TTT...}}}}}...TTT|..........T.T...T|.|.|.|............|...........|........|
TTT.}}}}}...TTTTT|................T|.|.|.|............+...........+........|
TTT}}}}}TTTTTTTTT-----------------------------------------------------------
]]);

des.region(selection.area(00,00,40,19), "lit")
des.region(selection.area(41,00,75,19), "unlit")
des.replace_terrain({ region={00,00, 16,19}, fromterrain="T", toterrain=".", chance=50 })

des.stair("up", 1, 9)

-- Doors
des.door("locked", 17,09)
des.door("closed", 37,02)
des.door("closed", 37,16)
des.door("locked", 38,05)
des.door("locked", 38,13)
des.door("closed", 39,02)
des.door("closed", 39,16)
des.door("locked", 41,09)
des.door("locked", 47,06)
des.door("locked", 47,12)
des.door("locked", 54,01)
des.door("locked", 54,09)
des.door("locked", 54,18)
des.door("locked", 66,01)
des.door("locked", 66,18)
des.door("locked", 73,09)
des.door("locked", 74,03)
des.door("locked", 74,04)
des.door("locked", 74,05)
des.door("locked", 74,06)
des.door("locked", 74,07)
des.door("locked", 74,08)
des.door("locked", 74,09)
des.door("locked", 74,10)
des.door("locked", 74,11)
des.door("locked", 74,12)
des.door("locked", 74,13)
des.door("locked", 74,14)
des.door("locked", 74,15)
des.door("locked", 74,16)

-- Sostratus' room
des.object({ id = "dunce cap", coord = {72, 09}, buc="cursed", spe=3, name="The Crown of Midas" })
des.monster("Sostratus", 72, 09)
des.monster("succubus",72,07)
des.monster("succubus",72,08)
des.monster("incubus",72,10)
des.monster("succubus",72,11)
local main_room = selection.area(55,04,71,15)
for i=1,100 do
	if d(20) > 15 then
		local cm = main_room:rndcoord(1)
		des.monster({id = "giant mimic", appear_as = "obj:gold piece", x = cm.x, y = cm.y})
	end
	local cg = main_room:rndcoord(1)
	des.gold({amount = d(500), x = cg.x, y = cg.y})
	des.trap("board", main_room:rndcoord(1))
end

-- River
river = selection.area(01,01,16,18)
des.object({id = "statue", montype="merchant", material="gold", x = 3, y = 9})
for i = 1,15 do
	for j=1,5 do
		local cg = river:rndcoord(1)
		des.gold({amount = d(500), x = cg.x, y = cg.y})
	end
	local cs = river:rndcoord(1)
	des.object({id = "statue", montype="merchant", material="gold", x = cs.x, y = cs.y})
	des.monster("acid blob", river:rndcoord(1))
	des.monster("spotted jelly", river:rndcoord(1))
end

-- Courtyard
courtyard = selection.area(18,01,33,18)
for i = 1,10 do
	des.monster("gold golem", courtyard:rndcoord(1))
	des.monster("'", courtyard:rndcoord(1))
end

-- Stable
if d(4) == 4 then
	des.monster("horse", 36, 01)
else
	des.object({id = "statue", montype="horse", material="gold", x = 36, y = 01})
end
if d(4) == 4 then
	des.monster("horse", 36, 18)
else
	des.object({id = "statue", montype="horse", material="gold", x = 36, y = 18})
end
des.object("saddle", 38, 01)
if d(4) == 4 then
	des.monster("horse", 40, 01)
else
	des.object({id = "statue", montype="horse", material="gold", x = 40, y = 01})
end
if d(4) == 4 then
	des.monster("horse", 40, 18)
else
	des.object({id = "statue", montype="horse", material="gold", x = 40, y = 18})
end
des.object("saddle", 38, 18)

-- Statue room
statue_room = selection.area(42,07,53,11)
for i = 1,40 do
	local c = statue_room:rndcoord(1)
	local mon = { "merchant", "soldier", "rogue", "ranger", "barbarian", "healer", "wizard" }
	local mon_idx = d(#mon)
	des.object({id = "statue", montype=mon[mon_idx], material="gold", x = c.x, y = c.y})
end

-- Weapon room
weapon_room = selection.area(42,01,53,05)
for i = 1,40 do
	local c = weapon_room:rndcoord(1)
	local weap = { "dart", "spear", "javelin", "trident", "dagger", "knife", "stiletto",
				   "axe", "battle-axe", "short sword", "scimitar", "saber", "broadsword",
				   "long sword", "two-handed sword", "partisan", "ranseur", "spetum",
				   "glaive", "halberd", "bardiche", "voulge", "fauchard", "guisarme",
				   "bill-guisarme", "lucern hammer", "bec de corbin", "lance", "mace",
				   "war hammer", "aklys"}
	local weap_idx = d(#weap)
	local weap_mat = "gold"
	if d(6) == 6 then
		weap_mat = "silver"
	end
	des.object({id = weap[weap_idx], material=weap_mat, x = c.x, y = c.y})
end

-- Armor room
armor_room = selection.area(42,13,53,18)
for i = 1,40 do
	local c = armor_room:rndcoord(1)
	local armr = { "dented pot", "helm of caution", "helm of opposite alignment",
				   "helm of telepathy", "plate mail", "splint mail", "banded mail",
				   "chain mail", "scale mail", "ring mail", "small shield",
				   "large shield"}
	local armr_idx = d(#armr)
	local armr_mat = "gold"
	if d(6) == 6 then
		armr_mat = "copper"
	end
	des.object({id = armr[armr_idx], material=armr_mat, x = c.x, y = c.y})
end

-- Dragon room
local dragon_room = selection.area(55,01,65,02)
des.monster("gold dragon",65,01)
des.monster("yellow dragon",65,02)
for i=1,5 do
	local cs = dragon_room:rndcoord(1)
	des.object({id = "statue", montype="gold dragon", material="gold", x = cs.x, y = cs.y})
end

-- Golden naga room
local naga_room = selection.area(55,17,65,18)
des.monster("golden naga",65,17)
des.monster("black naga",65,18)
for i=1,5 do
	local cs = naga_room:rndcoord(1)
	des.object({id = "statue", montype="golden naga", material="gold", x = cs.x, y = cs.y})
end

-- Chests
for x=67,74 do
	des.object({id = "chest", material="gold", x = x, y = 1})
	des.object({id = "chest", material="gold", x = x, y = 2})
	des.object({id = "chest", material="gold", x = x, y = 17})
	des.object({id = "chest", material="gold", x = x, y = 18})
end