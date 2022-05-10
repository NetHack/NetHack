-- NetHack Wizard Wiz-loca.lua	$NHDT-Date: 1652196019 2022/05/10 15:20:19 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.2 $
--	Copyright (c) 1992 by David Cohrs
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "hardfloor")

des.map([[
.............        .......................................................
..............       .............}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}.......
..............      ..............}.................................}.......
..............      ..............}.-------------------------------.}.......
...............     .........C....}.|.............................|.}.......
...............    ..........C....}.|.---------------------------.|.}.......
...............    .........CCC...}.|.|.........................|.|.}.......
................   ....C....CCC...}.|.|.-----------------------.|.|.}.......
.......C..C.....  .....C....CCC...}.|.|.|......+.......+......|.|.|.}.......
.............C..CC.....C....CCC...}.|.|.|......|-------|......|.|.|.}.......
................   ....C....CCC...}.|.|.|......|.......|......|.|.|.}.......
......C..C.....    ....C....CCC...}.|.|.|......|-------|......|.|.|.}.......
............C..     ...C....CCC...}.|.|.|......+.......+......|.|.|.}.......
........C......    ....C....CCC...}.|.|.-----------------------.|.|.}.......
....C......C...     ........CCC...}.|.|.........................|.|.}.......
......C..C....      .........C....}.|.---------------------------.|.}.......
..............      .........C....}.|.............................|.}.......
.............       ..............}.-------------------------------.}.......
.............        .............}.................................}.......
.............        .............}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}.......
.............        .......................................................
]]);

des.replace_terrain({ region = { 0, 0,30,20}, fromterrain=".", toterrain="C", chance=15 })
des.replace_terrain({ region = {68, 0,75,20}, fromterrain=".", toterrain="}", chance=25 })
des.replace_terrain({ region = {34, 1,68,19}, fromterrain="}", toterrain=".", chance=2 })

-- Dungeon Description
des.region(selection.area(00,00,75,20), "lit")
des.region({ region={37,04,65,16}, lit=0, type="ordinary", irregular=1,
             contents = function()
                des.door({ state="secret", wall="random" })
                end
})
des.region({ region={39,06,63,14}, lit=0, type="ordinary", irregular=1,
             contents = function()
                des.door({ state="secret", wall="random" })
             end
})

des.region({ region={41,08,46,12}, lit=1, type="ordinary", irregular=1,
             contents = function()
                local walls = { "north", "south", "west" }
                local widx = math.random(1, #walls)
                des.door({ state="secret", wall=walls[widx] })
             end
})

des.region({ region={56,08,61,12}, lit=1, type="ordinary", irregular=1,
             contents = function()
                local walls = { "north", "south", "east" }
                local widx = math.random(1, #walls)
                des.door({ state="secret", wall=walls[widx] })
             end
})

des.region(selection.area(48,08,54,08), "unlit")
des.region(selection.area(48,12,54,12), "unlit")

des.region({ region={48,10,54,10}, lit=0, type="ordinary", irregular=1,
             contents = function()
                des.door({ state="secret", wall="random" })
             end
})

-- Doors
des.door("locked",55,08)
des.door("locked",55,12)
des.door("locked",47,08)
des.door("locked",47,12)
-- Stairs
des.terrain({03,17}, ".")
des.stair("up", 03,17)
des.stair("down", 48,10)
-- Non diggable walls
des.non_diggable(selection.area(00,00,75,20))
-- Objects
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
-- Random traps
des.trap("spiked pit",24,02)
des.trap("spiked pit",07,10)
des.trap("spiked pit",23,05)
des.trap("spiked pit",26,19)
des.trap("spiked pit",72,02)
des.trap("spiked pit",72,12)
des.trap("falling rock",45,16)
des.trap("falling rock",65,13)
des.trap("falling rock",55,06)
des.trap("falling rock",39,11)
des.trap("falling rock",57,09)
des.trap("magic")
des.trap("statue")
des.trap("statue")
des.trap("polymorph")
des.trap("anti magic",53,10)
des.trap("sleep gas")
des.trap("sleep gas")
des.trap("dart")
des.trap("dart")
des.trap("dart")
-- Random monsters.
des.monster({ class = "B", peaceful = 0 })
des.monster({ class = "B", peaceful = 0 })
des.monster({ class = "B", peaceful = 0 })
des.monster({ class = "B", peaceful = 0 })
des.monster({ class = "B", peaceful = 0 })
des.monster({ class = "B", peaceful = 0 })
des.monster({ class = "B", peaceful = 0 })
des.monster({ class = "B", peaceful = 0 })
des.monster({ class = "B", peaceful = 0 })
des.monster({ class = "B", peaceful = 0 })
des.monster({ class = "B", peaceful = 0 })
des.monster({ class = "B", peaceful = 0 })
des.monster({ class = "i", peaceful = 0 })
des.monster({ class = "i", peaceful = 0 })
des.monster({ class = "i", peaceful = 0 })
des.monster({ class = "i", peaceful = 0 })
des.monster({ class = "i", peaceful = 0 })
des.monster({ class = "i", peaceful = 0 })
des.monster({ class = "i", peaceful = 0 })
des.monster("vampire bat")
des.monster("vampire bat")
des.monster("vampire bat")
des.monster("vampire bat")
des.monster("vampire bat")
des.monster("vampire bat")
des.monster("vampire bat")
des.monster({ class = "i", peaceful = 0 })
