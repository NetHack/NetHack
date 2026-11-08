-- $NHDT-Header: NetHack 5.0 Mon-loca.lua 2026/06/20_22:34:29 NetHack-5.0:1.7 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991-2 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "mines", fg = ".", smoothed = true, joined = true, lit = 1, walled = true });

des.level_flags("mazelevel", "noflip");

local floor = selection.match("."):grow();
des.terrain({ selection = floor, typ = ".", lit = 1 });
des.terrain({ selection = selection.rect(00,00,78,20), typ = " " });

-- Dungeon Description
des.region(selection.area(00,00,78,20), "lit")
des.non_diggable(selection.area(00,00,75,20))



local formations = {

-- wu wei
[[
......................xxxx
..L............L...L....xx
..LLLLLLLLL.....L..L.....x
.L.L.L.L.L....LLLLLLLLL..x
...L.L.L.L........L...L...
.LLLLLLLLLLL.....LLLLLLL..
...L.L.L.L.......L.....L..
...L.L.L.L......LLLLLLLLL.
..LLLLLLLLL....L........L.
..............L.L.L.L.L.L.
..L.L..L..L.....L.L.L...L.
.L...L..L..L...L..L....LL.
..........................
]],

[[
..........................
.LLLLL.L...........LLLLL..
...L...LLLLL.......L......
...L...L..L....LLLLLLLLLL.
..L..L.L..L....L...L....L.
.LLLL.L.L.L....L.LLLLLL...
...L....L.L....L...L...L..
...L.....L.....L...LLLL...
.LLLLL...L.....L.L.L.L.L..
...L....L.L....L.LLL.LLL..
...LLL..L.L...L....L.L....
.LLL...L...L..L.LLLLLLLLL.
..........................
]],

};

des.map({ halign = "center", valign = "center",
          map = formations[math.random(#formations)], lit = 1,
          contents = function() end });

if (percent(75)) then
   des.replace_terrain({ selection = selection.negate(), fromterrain ="L", toterrain = "P" });
end

des.wallify();

-- Stairs
des.stair("up")
des.stair("down")
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
-- since vegetarian monks shouldn't eat giant corpses, give a chance for
-- Str boost that isn't throttled by exercise restrictions;
-- make a modest effort (Elbereth only) to prevent xorns from eating the tins
local tinplace = selection.negate():filter_mapchar('.')
local tinloc = tinplace:rndcoord(0)
des.object({ id="tin", coord=tinloc, quantity=2, buc="blessed",
             montype="spinach" })
des.engraving({ coord=tinloc, type="burn", text="Elbereth" })
-- Random traps
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
-- Random monsters.
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("earth elemental")
des.monster("xorn")
des.monster("xorn")
des.monster("xorn")
des.monster("xorn")
des.monster("xorn")
des.monster("xorn")
des.monster("xorn")
des.monster("xorn")
des.monster("xorn")
