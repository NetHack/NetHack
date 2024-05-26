des.level_init({ style = "solidfill", fg = " " });
des.level_flags("mazelevel", "noteleport", "hardfloor")
des.map([[
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}---..---}}}}}}}}}}}}}}}}}}}}}}}}}}}}}--------}}
}}}}}}}}}}}}}}}}}}}}}---------......---------}}}}}}}}}}}}}}}}}}}}}|......|}}
}}}}}}}}}}}}}}--------......|............|..---------}}}}}}}}}}}}}.......|}}
}}}}}}}}-------...|.........+............+..........--------}}}}}}|......|}}
}}}}}}}}|.........|.........|............|.................|}}}}}}--------}}
}}}}}}}}|.........+.........|-------------------------+----|}}}}}}}}}}}}}}}}
}}}}}}}}|.........|.........|........|..........|..........|}}}}}}}}}}}}}}}}
}}}}}}}}|.........|.........|........+..........|..........|}}}}}}}}}}}}}}}}
}}}}}}}}|.........|.........|........|..........|..........|}}}}}}}}}}}}}}}}
}}}}}}}}|----+-----------------------|------....+..........|}}}}}}}}}}}}}}}}
}}}}}}}}|...............|............|.....|....|..........|}}}}}}}}}}}}}}}}
}}}}}}}}|...............+............|.....|....|..........|}}}}}}}}}}}}}}}}
}}}}}}}}-------.........|............|.....+....|...--------}}}}}}}}}}}}}}}}
}}}}}}}}}}}}}}--------..|............|.....|---------}}}}}}}}}}}}}}}}}}}}}}}
}}}}}}}}}}}}}}}}}}}}}---------......---------}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}---..---}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
}}}}....................................................................}}}}
............................................................................
]]);

des.region(selection.area(00,00,75,19), "lit")

des.stair("up", 33, 19)

for i = 1,1000 do
	des.gold({amount = d(500)})
end

-- Doors
des.door("locked",24,12)
des.door("locked",13,10)
des.door("locked",18,06)
des.door("locked",28,04)
des.door("locked",41,04)
des.door("locked",54,06)
des.door("locked",48,10)
des.door("locked",43,13)
des.door("locked",37,08)

-- Caligula's room
des.object({ id = "gauntlets of power", coord = {33, 8}, buc="cursed", spe=0, name="The Touch of Midas" })
des.monster("Emperor Caligula", 33, 8)
des.monster("succubus",33,9)
des.monster("succubus",33,7)
des.monster("incubus",34,8)
des.monster("succubus",32,8)

-- Statue room
statue_room = selection.area(25,11,36,14)
for i = 1,40 do
	local c = statue_room:rndcoord(1)
	local mon = { "merchant", "soldier", "rogue", "ranger", "barbarian", "healer", "wizard" }
	local mon_idx = d(#mon)
	des.object({id = "statue", montype=mon[mon_idx], material="gold", x = c.x, y = c.y})
end

-- Gem room
gem_room = selection.area(09,11,23,12)
for i = 1,30 do
	local c = gem_room:rndcoord(1)
	local gem = { "dilithium crystal", "diamond", "ruby", "sapphire", "black opal", "emerald" }
	local gem_idx = d(#gem)
	des.object({id = gem[gem_idx], x = c.x, y = c.y})
end

-- Golem room
golem_room = selection.area(09,05,17,09)
for i = 1,20 do
	des.monster("gold golem", golem_room:rndcoord(1))
end
for i = 1,10 do
	des.monster("stone golem", golem_room:rndcoord(1))
end
for i = 1,10 do
	des.monster("'", golem_room:rndcoord(1))
end

-- Weapon room
weapon_room = selection.area(19,04,27,09)
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

-- Gold monster room
monster_room = selection.area(29,03,40,05)
for i = 1,35 do
	local c = monster_room:rndcoord(1)
	local mon = { "golden naga", "gold dragon" }
	local mon_idx = d(#mon)
	if i < 8 then
		des.monster({ id = mon[mon_idx], x=c.x, y=c.y, asleep=1, peaceful = 0})
	else
		des.object({id = "statue", montype=mon[mon_idx], material="gold", x = c.x, y = c.y})
	end
end

-- Animal room
animal_room = selection.area(42,04,51,05)
for i = 1,20 do
	local c = animal_room:rndcoord(1)
	local mon = { "dog", "jackal", "wolf", "lynx", "panther", "tiger" }
	local mon_idx = d(#mon)
	if i < 10 then
		des.monster({ id = mon[mon_idx], x=c.x, y=c.y, asleep=1, peaceful = 0})
	else
		des.object({id = "statue", montype=mon[mon_idx], material="gold", x = c.x, y = c.y})
	end
end

-- Barracks
des.region({ region={49,07,58,12}, type="barracks", filled=1 })

-- Mimic room
mimic_room_top = selection.area(38,07,47,09)
for i = 1,10 do
	local c = mimic_room_top:rndcoord(1)
	des.monster({id = "giant mimic", appear_as = "obj:gold piece", x = c.x, y = c.y})
end
mimic_room_bot = selection.area(44,10,47,13)
for i = 1,10 do
	local c = mimic_room_bot:rndcoord(1)
	des.monster({id = "giant mimic", appear_as = "obj:gold piece", x = c.x, y = c.y})
end

-- Incitatus' stable
des.monster({id = "warhorse", x = 40, y = 13, peaceful = 1, asleep=1, name = "Incitatus"})
incitatus_stable = selection.area(38,11,42,14)
for i = 1,10 do
	des.object("apple", incitatus_stable:rndcoord(1))
	des.object("carrot", incitatus_stable:rndcoord(1))
end

-- Floating Altar
des.object({id = "statue", montype="priest", material="gold", x=71,y=3,})
des.altar({ x=70,y=3,align="noalign",type="sanctum" })

-- Water
des.monster("giant eel", 0, 0)
des.monster("giant eel", 0, 1)
des.monster("giant eel", 0, 2)
des.monster("giant eel", 0, 3)
des.monster("electric eel", 0, 4)
des.monster("electric eel", 0, 5)
des.monster("jellyfish", 1, 0)
des.monster("jellyfish", 1, 1)
des.monster("jellyfish", 1, 2)
des.monster("jellyfish", 1, 3)
des.monster("jellyfish", 1, 4)
des.monster("jellyfish", 1, 5)