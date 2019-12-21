-- NetHack 3.6	medusa.des	$NHDT-Date: 1432512783 2015/05/25 00:13:03 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1990, 1991 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });
des.level_flags("noteleport", "mazelevel")
--
-- Here the Medusa rules some slithery monsters from her 'palace', with
-- a yellow dragon nesting in the backyard.
--
des.map([[
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
}}}}}}}}}}}}}}........}}}}}}}}}}}}}}}}}}}}}}}..}}}.....}}}}}}}}}}}----|}}}}}
}}}}}}..----------F-.....}}}}}}}}}}}}}}}}..---...}}}}....T.}}}}}}}....|}}}}}
}}}.....|...F......S}}}}....}}}}}}}...}}.....|}}.}}}}}}}......}}}}|......}}}
}}}.....+...|..{...|}}}}}}}}}}}}.....}}}}|...|}}}}}}}}}}}.}}}}}}}}----.}}}}}
}}......|...|......|}}}}}}}}}......}}}}}}|.......}}}}}}}}}}}}}..}}}}}...}}}}
}}|-+--F|-+--....|F|-|}}}}}....}}}....}}}-----}}.....}}}}}}}......}}}}.}}}}}
}}|...}}|...|....|}}}|}}}}}}}..}}}}}}}}}}}}}}}}}}}}....}}}}}}}}....T.}}}}}}}
}}|...}}F...+....F}}}}}}}..}}}}}}}}}}}}}}...}}}}}}}}}}}}}}}}}}}}}}....}}..}}
}}|...}}|...|....|}}}|}....}}}}}}....}}}...}}}}}...}}}}}}}}}}}}}}}}}.....}}}
}}--+--F|-+--....-F|-|....}}}}}}}}}}.T...}}}}....---}}}}}}}}}}}}}}}}}}}}}}}}
}}......|...|......|}}}}}.}}}}}}}}}....}}}}}}}.....|}}}}}}}}}.}}}}}}}}}}}}}}
}}}}....+...|..{...|.}}}}}}}}}}}}}}}}}}}}}}}}}}.|..|}}}}}}}......}}}}...}}}}
}}}}}}..|...F......|...}}}}}}}}}}..---}}}}}}}}}}--.-}}}}}....}}}}}}....}}}}}
}}}}}}}}-----S----F|....}}}}}}}}}|...|}}}}}}}}}}}}...}}}}}}...}}}}}}..}}}}}}
}}}}}}}}}..............T...}}}}}.|.......}}}}}}}}}}}}}}..}...}.}}}}....}}}}}
}}}}}}}}}}....}}}}...}...}}}}}.......|.}}}}}}}}}}}}}}.......}}}}}}}}}...}}}}
}}}}}}}}}}..}}}}}}}}}}.}}}}}}}}}}-..--.}}}}}}}}..}}}}}}..T...}}}..}}}}}}}}}}
}}}}}}}}}...}}}}}}}}}}}}}}}}}}}}}}}...}}}}}}}....}}}}}}}.}}}..}}}...}}}}}}}}
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}.}}}}}}....}}}}}}}}}}}}}}}}}}}...}}}}}}
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
]]);
--
local place = selection.new();
place:set(04,08);
place:set(10,04);
place:set(10,08);
place:set(10,12);
--
des.region(selection.area(00,00,74,19),"lit")
des.region({ region={13,03, 18,13}, lit=1, type="ordinary", prefilled=1 })
--
des.teleport_region({ region = {64,01,74,17}, dir="down" });
des.teleport_region({ region = {02,02,18,13}, dir="up" });
--
des.levregion({ region = {67,01,74,20}, type="stair-up" });
local mx, my = place:rndcoord(1);
des.stair("down", mx, my)
--
des.door("locked",04,06)
des.door("locked",04,10)
des.door("locked",08,04)
des.door("locked",08,12)
des.door("locked",10,06)
des.door("locked",10,10)
des.door("locked",12,08)
--
des.levregion({ region = {27,00,79,20}, type="branch" });
--
des.non_diggable(selection.area(01,01,22,14));
--
des.object("crystal ball", 07,08)
--
local px, py = place:rndcoord(1);
des.object({ id="statue",x=px, y=py, buc="uncursed",
                      montype="knight", historic=1, male=1,name="Perseus",
                      contents = function()
                         if math.random(0,99) < 75 then
                            des.object({ id = "shield of reflection", buc="cursed", spe=0 })
                         end
                         if math.random(0,99) < 25 then
                            des.object({ id = "levitation boots", spe=0 })
                         end
                         if math.random(0,99) < 50 then
                            des.object({ id = "scimitar", buc="blessed", spe=2 })
                         end
                         if math.random(0,99) < 50 then
                            des.object("sack")
                         end
                      end
});
--
des.object({ id = "statue", contents=0 })
des.object({ id = "statue", contents=0 })
des.object({ id = "statue", contents=0 })
des.object({ id = "statue", contents=0 })
des.object({ id = "statue", contents=0 })
des.object({ id = "statue", contents=0 })
des.object({ id = "statue", contents=0 })
for i=1,8 do
   des.object()
end
--
for i=1,7 do
   des.trap()
end
--
des.monster("Medusa", mx, my)
des.monster("kraken", 07,07)
--
-- the nesting dragon
des.monster({ id = "yellow dragon", x=05, y=04, asleep=1 })
if math.random(0,99) < 50 then
   des.monster({ id = "baby yellow dragon", x=04,y=04, asleep=1 })
end
if math.random(0,99) < 25 then
   des.monster({ id = "baby yellow dragon", x=04, y=05, asleep=1 })
end
des.object({ id = "egg", x=05, y=04, montype="yellow dragon" });
if math.random(0,99) < 50 then
   des.object({ id = "egg", x=05, y=04, montype="yellow dragon" });
end
if math.random(0,99) < 25 then
   des.object({ id = "egg", x=05, y=04, montype="yellow dragon" });
end
--
des.monster("giant eel")
des.monster("giant eel")
des.monster("jellyfish")
des.monster("jellyfish")
for i=1,14 do
   des.monster("S")
end
for i=1,4 do
   des.monster("black naga hatchling")
   des.monster("black naga")
end
