des.level_init({ style = "solidfill", fg = " " });
des.level_flags("mazelevel", "noteleport", "hardfloor")
des.map([[
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
.}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}....}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
..}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}........}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
....}}}}}}}}}}}}}}}}}}}}}}}}}}}}}......TTTT......}}}}}}}}}}}}}}}}}}}}}}}....
.TT..}}}}}}}}}}}}}}}}}}}}}}}}}}...TTTTTTTTTTTTT......}}}}}}}}}}}}}}}......TT
.TTT..}}}}}}}}}}}}}}}}}}}}}}}}...TTTTTTTTTTTTTTTTTT......}}}}}}}.........TTT
..TTT..}}}}}}}}}}}}}}}}}}}}}}...TTTTTTTTTTTTTTTTTTTTTTT....}}....TTTTTT.TTTT
T.TTTT...}}}}}}}}}}}}}}}}}}....TTT-----------------TTTTTT......TTTTTTT..TTTT
T.TTTTTT.....}}}}}}}}}}..........T|.......|.......|-----------TTTTTTT..TTTTT
-+----TTTT......}}}}.....TTTTTT...|.......+.......+..........|TTTT....TTTTTT
|....|TTTTTT............TTTTTTTT..+.......|.......|..........+......TTTTTTTT
|....|TTTTTTTTTTTTTTTT.....TTT....|.......|-------|..........|TTTTTTTTTTTTTT
|....|TTTTTTTTTTTTTTTTTT........TT|.......+.......|..........|TTTTTTTTTT.TTT
|....|TTTTTTTTTTTTTT........TTTTTT--------|.......|-----------TTTTTTTTTT.TTT
|....|TTTTTTTTTT........TTTTTTTTTTTTTTTTTT|.......|..|TTTTTTTTTTTTTTTTT..TTT
------TTTTTT........TTTTTTTTTTTTTTTTTTTTTT|.......+..|TTTTTTTTTTTTTTTT..TTTT
TTTTTTTT........TTTTTTTTTTTTTTTTTTTTTTTTTT------------TTTTTTTTTTTTTTT..TTTTT
TTTT........TTTTTTTTTTTTTTT......TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT....TTTTTT
........TTTTTTTTTTTTTTTTTTTTTTT.....................................TTTTTTTT
]]);

-- Dungeon Description
des.region(selection.area(00,00,75,19), "lit")
des.replace_terrain({ region={00,00, 75,19}, fromterrain="T", toterrain=".", chance=20 })

-- Stairs
des.stair("up", 00,19)
des.stair("down", 75,04)
-- Doors
des.door("locked",47,07)
des.door("locked",51,04)
-- Rooms
des.region({ region={01,11, 04,15}, lit=1, type="shop", filled=1 })
des.region({ region={35,09, 41,13}, lit=0, type="beehive", filled=1 })
des.region({ region={43,13, 49,16}, lit=0, type="morgue", filled=1 })
des.region({ region={43,09, 49,11}, lit=0, type="barracks", filled=1 })
-- Doors
des.door("locked",34,11)
des.door("locked",42,10)
des.door("locked",42,13)
des.door("locked",50,10)
des.door("locked",50,16)
des.door("locked",61,11)
-- Monsters
des.monster({ id = "gold golem", peaceful=0 })
des.monster({ id = "gold golem", peaceful=0 })
des.monster({ id = "gold golem", peaceful=0 })
des.monster({ id = "gold golem", peaceful=0 })
des.monster({ id = "gold golem", peaceful=0 })
des.monster({ id = "gold golem", peaceful=0 })
des.monster({ id = "gold golem", peaceful=0 })
des.monster({ id = "gold golem", peaceful=0 })
des.monster({ id = "gold golem", peaceful=0 })
des.monster({ id = "gold golem", peaceful=0 })
des.monster({ id = "stone golem", peaceful=0 })
des.monster({ id = "stone golem", peaceful=0 })
des.monster({ id = "stone golem", peaceful=0 })
des.monster({ id = "stone golem", peaceful=0 })
des.monster({ class = "'", peaceful=0 })
des.monster({ class = "'", peaceful=0 })
des.monster({ class = "'", peaceful=0 })
des.monster({ id = "giant mimic", peaceful=0 })
des.monster({ id = "giant mimic", peaceful=0 })
des.monster({ id = "giant mimic", peaceful=0 })
des.monster({ id = "giant mimic", peaceful=0 })
des.monster({ id = "giant mimic", peaceful=0 })
des.monster("jellyfish",1, 0)
des.monster("jellyfish",3, 0)
des.monster("jellyfish",5, 0)
des.monster("jellyfish",7, 0)
des.monster("jellyfish",9, 0)
des.monster("giant eel",1, 1)
des.monster("giant eel",3, 1)
des.monster("giant eel",5, 1)
des.monster("giant eel",7, 1)
des.monster("electric eel",9, 1)
des.monster("electric eel",11, 1)


-- Items
des.object({id = "chest", material="gold", x = 51, y = 16})
des.object({id = "chest", material="gold", x = 51, y = 15})
des.object({id = "chest", material="gold", x = 52, y = 16})
des.object({id = "chest", material="gold", x = 52, y = 15})
des.object({id = "statue", montype="merchant", material="gold", x = 51, y = 16})
des.object({id = "statue", montype="barbarian", material="gold", x = 51, y = 15})
des.object({id = "statue", montype="priest", material="gold", x = 52, y = 16})
des.object({id = "statue", montype="healer", material="gold", x = 52, y = 15})
