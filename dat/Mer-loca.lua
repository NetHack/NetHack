des.level_init({ style = "solidfill", fg = " " });
des.level_flags("mazelevel", "hardfloor")
des.map([[
}}}}}}}}}}}}}}}}}}}}}}}TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT
------}}}}}}}}}}}}}}}}}}}...TTTTTTTTTTTTTTTTTTTTTT...........TTTTTTTTTTTTT.T
|....|}}}}}}}}}}}}}}}}}}}}}.TTTTTTTTTTTTTTTTTTTTTT...........TTTTTTTTTTTTT.T
|....|..}}}}}}}}}}}}}}}}}}}.TTTTTTTTTTTTTTTTTTTTTT...........TTTTTTTTTTTTTTT
|....|T...}}}}}}}}}}}}}}}...TTTTTTTTTTTTTTTTTTTTTT...........TTTTTTTTTTTTT.T
|....|TTT...}}}}}}}}}}}}..TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT
|....|TTTTT...}}}}}}}}}}}...TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT.T
|....|TTTTTTT...}}}}}}}}}}}...TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT
-+---|TTTTTTTTT...}}}}}}}}}}}.TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT.T
|.+..|TTTTTTTTTTT....}}}}..}}.TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT
|.|..|TTTTTTTTTTT.}}...}}}....TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT.T
|.---|TTTTTTTTT...}}}}}}}}}}}.TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT
|....|TTTTTTT...}}}}}}}}}}}...TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT.T
|---.|TTTTT...}}}}}}}}}}}...TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT
|..+.|TTTTT.}}}}}}}}}}}...TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT.T
|..|.|TTTTT.}}}}}}}}}}}...TTTTTTTTTTTTTTTTTTTTTTTT...........TTTTTTTTTTTTTTT
|..|.|TTTTT...}}}}}}}}}}}.TTTTTTTTTTTTTTTTTTTTTTTT...........TTTTTTTTTTTTT.T
----........}}}}}}}}}}}...TTTTTTTTTTTTTTTTTTTTTTTT...........TTTTTTTTTTTTTTT
.....|TTTTT...}}}}}}}}}}}....TTTTTTTTTTTTTTTTTTTTT...........TTTTTTTTTTTTT.T
------TTTTTTTTTTTT}}}}}}}}}}}TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT
0123456789012345678901234567890123456789012345678901234567890123456789012345
]]);

-- Dungeon Description
des.region(selection.area(00,00,32,19), "lit")
des.region(selection.area(32,00,75,19), "unlit")
des.replace_terrain({ region={01,01, 74,18}, fromterrain="T", toterrain=".", chance=50 })

-- Stairs
des.stair("up", 00,18)
des.stair("down", 74,10)

-- Doors
des.door("closed", 01,08)
des.door("closed", 02,09)
des.door("closed", 03,14)

-- Rooms
des.region({ region={01,02, 04,07}, type="weapon shop", filled=1})
des.region({ region={03,09, 04,10}, type="book shop", filled=1})
des.region({ region={01,14, 02,16}, type="book shop", filled=1})
des.region({ region={50,01, 60,04}, type="beehive", filled=1})
des.region({ region={50,15, 60,18}, type="anthole", filled=1})

-- Monsters
des.monster({ id = "gold golem", peaceful=0 })
des.monster({ id = "gold golem", peaceful=0 })
des.monster({ id = "gold golem", peaceful=0 })
des.monster({ id = "gold golem", peaceful=0 })
des.monster({ id = "gold golem", peaceful=0 })
des.monster({ id = "gold golem", peaceful=0 })
des.monster({ id = "stone golem", peaceful=0 })
des.monster({ id = "stone golem", peaceful=0 })
des.monster({ class = "'", peaceful=0 })
des.monster({ class = "'", peaceful=0 })
des.monster({ id = "giant mimic", peaceful=0, appear_as = "ter:tree" })
des.monster({ id = "giant mimic", peaceful=0, appear_as = "ter:tree" })
des.monster({ id = "giant mimic", peaceful=0, appear_as = "ter:tree" })
des.monster({ id = "giant mimic", peaceful=0, appear_as = "ter:tree" })
des.monster({ id = "giant mimic", peaceful=0, appear_as = "ter:tree" })
des.monster({ class = "m", peaceful=0 })
des.monster({ class = "n", peaceful=0 })
des.monster({ class = "l", peaceful=0 })
des.monster({ class = "s", peaceful=0 })
des.monster({ class = "S", peaceful=0 })


-- Items
des.object({id = "chest", material="gold", x = 74, y = 01})
des.object({id = "chest", material="gold", x = 74, y = 18})
des.object({id = "chest", material="gold", x = 74, y = 04})
des.object({id = "chest", material="gold", x = 74, y = 16})
des.object({id = "statue", montype="merchant", material="gold", x = 74, y = 01})
des.object({id = "statue", montype="barbarian", material="gold", x = 74, y = 18})
des.object({id = "statue", montype="priest", material="gold", x = 74, y = 04})
des.object({id = "statue", montype="healer", material="gold", x = 74, y = 16})
des.object()
des.object()
des.object()
des.object()

-- Traps
des.trap("pit")
des.trap("pit")
des.trap("spiked pit")
des.trap("spiked pit")
des.trap("dart")
des.trap("dart")
des.trap("sleep gas")
des.trap("magic")
des.trap("polymorph")