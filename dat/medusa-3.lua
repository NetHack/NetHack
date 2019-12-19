-- NetHack 3.6	medusa.des	$NHDT-Date: 1432512783 2015/05/25 00:13:03 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1990, 1991 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });
des.level_flags("noteleport", "mazelevel", "shortsighted")
--
-- Here you disturb ravens nesting in the trees.
--
des.map([[
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
}}}}}}}}}}.}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}.}}}}}}}}}}}}}}}}}}}}}}}}}}}}
}}}}}}}}T..T.}}}}}}}}}}}}}}}}}}}}..}}}}}}}}.}}}...}}}}}}}.}}}}}......}}}}}}}
}}}}}}.......T.}}}}}}}}}}}..}}}}..T.}}}}}}...T...T..}}...T..}}..-----..}}}}}
}}}...-----....}}}}}}}}}}.T..}}}}}...}}}}}.....T..}}}}}......T..|...|.T..}}}
}}}.T.|...|...T.}}}}}}}.T......}}}}..T..}}.}}}.}}...}}}}}.T.....+...|...}}}}
}}}}..|...|.}}.}}}}}.....}}}T.}}}}.....}}}}}}.T}}}}}}}}}}}}}..T.|...|.}}}}}}
}}}}}.|...|.}}}}}}..T..}}}}}}}}}}}}}T.}}}}}}}}..}}}}}}}}}}}.....-----.}}}}}}
}}}}}.--+--..}}}}}}...}}}}}}}}}}}}}}}}}}}T.}}}}}}}}}}}}}}}}.T.}........}}}}}
}}}}}.......}}}}}}..}}}}}}}}}.}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}.}}}.}}.T.}}}}}}
}}.T...T...}}}}T}}}}}}}}}}}....}}}}}}}}}}T}}}}}.T}}...}}}}}}}}}}}}}}...}}}}}
}}}...T}}}}}}}..}}}}}}}}}}}.T...}}}}}}}}.T.}.T.....T....}}}}}}}}}}}}}.}}}}}}
}}}}}}}}}}}}}}}....}}}}}}}...}}.}}}}}}}}}}............T..}}}}}.T.}}}}}}}}}}}
}}}}}}}}}}}}}}}}..T..}}}}}}}}}}}}}}..}}}}}..------+--...T.}}}....}}}}}}}}}}}
}}}}.}..}}}}}}}.T.....}}}}}}}}}}}..T.}}}}.T.|...|...|....}}}}}.}}}}}...}}}}}
}}}.T.}...}..}}}}T.T.}}}}}}.}}}}}}}....}}...|...+...|.}}}}}}}}}}}}}..T...}}}
}}}}..}}}.....}}...}}}}}}}...}}}}}}}}}}}}}T.|...|...|}}}}}}}}}}}....T..}}}}}
}}}}}..}}}.T..}}}.}}}}}}}}.T..}}}}}}}}}}}}}}---S-----}}}}}}}}}}}}}....}}}}}}
}}}}}}}}}}}..}}}}}}}}}}}}}}}.}}}}}}}}}}}}}}}}}T..T}}}}}}}}}}}}}}}}}}}}}}}}}}
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
]]);

local place = selection.new();
place:set(08,06);
place:set(66,05);
place:set(46,15);

des.region(selection.area(00,00,74,19),"lit")
des.region({ region={49,14, 51,16}, lit=-1, type="ordinary", prefilled = 1 });
des.region(selection.area(07,05,09,07),"unlit")
des.region(selection.area(65,04,67,06),"unlit")
des.region(selection.area(45,14,47,16),"unlit")
-- Non diggable walls
-- 4th room has diggable walls as Medusa is never placed there
des.non_diggable(selection.area(06,04,10,08))
des.non_diggable(selection.area(64,03,68,07))
des.non_diggable(selection.area(44,13,48,17))
-- All places are accessible also with jumping, so don't bother
-- restricting the placement when teleporting from levels below this.
des.teleport_region({ region = {33,02,38,07}, dir="down" })
des.levregion({ region = {32,01,39,07}, type="stair-up" });
local mx, my = place:rndcoord(1);
des.stair("down", mx, my)
des.door("locked",08,08)
des.door("locked",64,05)
des.door("random",50,13)
des.door("locked",48,15)
--
local px, py = place:rndcoord(1);
des.feature("fountain", px,py);
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
des.object("blank paper",48,18)
des.object("blank paper",48,18)
--
des.trap("rust")
des.trap("rust")
des.trap("board")
des.trap("board")
des.trap()
--
des.monster({ id = "Medusa", x=mx, y=my, asleep=1 })
des.monster("giant eel")
des.monster("giant eel")
des.monster("jellyfish")
des.monster("jellyfish")
des.monster("wood nymph")
des.monster("wood nymph")
des.monster("water nymph")
des.monster("water nymph")

for i=1,30 do
   des.monster({ id = "raven", hostile = 1 })
end

