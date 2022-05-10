-- NetHack medusa medusa-4.lua	$NHDT-Date: 1652196028 2022/05/10 15:20:28 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.6 $
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
-- fixup_special hack: The first "room" region in Medusa levels gets filled with
-- some leaderboard statues, so this needs to be a room; setting irregular=1
-- will force this
des.region({ region={13,03, 18,13}, lit=1, type="ordinary", irregular=1 })
--
des.teleport_region({ region = {64,01,74,17}, dir="down" });
des.teleport_region({ region = {02,02,18,13}, dir="up" });
--
des.levregion({ region = {67,01,74,20}, type="stair-up" });

des.stair("down", place:rndcoord(1))
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
des.object({ id="statue",coord=place:rndcoord(1), buc="uncursed",
                      montype="knight", historic=1, male=1,name="Perseus",
                      contents = function()
                         if percent(75) then
                            des.object({ id = "shield of reflection", buc="cursed", spe=0 })
                         end
                         if percent(25) then
                            des.object({ id = "levitation boots", spe=0 })
                         end
                         if percent(50) then
                            des.object({ id = "scimitar", buc="blessed", spe=2 })
                         end
                         if percent(50) then
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
des.monster("Medusa", place:rndcoord(1))
des.monster("kraken", 07,07)
--
-- the nesting dragon
des.monster({ id = "yellow dragon", x=05, y=04, asleep=1 })
if percent(50) then
   des.monster({ id = "baby yellow dragon", x=04,y=04, asleep=1 })
end
if percent(25) then
   des.monster({ id = "baby yellow dragon", x=04, y=05, asleep=1 })
end
des.object({ id = "egg", x=05, y=04, montype="yellow dragon" });
if percent(50) then
   des.object({ id = "egg", x=05, y=04, montype="yellow dragon" });
end
if percent(25) then
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
