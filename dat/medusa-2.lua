-- NetHack medusa medusa-2.lua	$NHDT-Date: 1652196027 2022/05/10 15:20:27 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.4 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1990, 1991 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "noteleport")

des.map([[
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
}------}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}-------}}}}}}}}--------------}
}|....|}}}}}}}}}..}.}}..}}}}}}}}}}}}}..}}}}}}-.....--}}}}}}}|............|}
}|....|.}}}}}}}}}}}.}...}}..}}}}}}}}}}}}}}}}}---......}}}}}.|............|}
}S....|.}}}}}}---}}}}}}}}}}}}}}}}}}}}}}}}}}---...|..-}}}}}}.S..----------|}
}|....|.}}}}}}-...}}}}}}}}}.}}...}.}}}}.}}}......----}}}}}}.|............|}
}|....|.}}}}}}-....--}}}}}}}}}}}}}}}}}}}}}}----...--}}}}}}}.|..--------+-|}
}|....|.}}}}}}}......}}}}...}}}}}}.}}}}}}}}}}}---..---}}}}}.|..|..S...|..|}
}|....|.}}}}}}-....-}}}}}}}------}}}}}}}}}}}}}}-...|.-}}}}}.|..|..|...|..|}
}|....|.}}}}}}}}}---}}}}}}}........}}}}}}}}}}---.|....}}}}}.|..|..|...|..|}
}|....|.}}}}}}}}}}}}}}}}}}-....|...-}}}}}}}}--...----.}}}}}.|..|..|...|..|}
}|....|.}}}}}}..}}}}}}}}}}---..--------}}}}}-..---}}}}}}}}}.|..|..-------|}
}|...}|...}}}.}}}}}}...}}}}}--..........}}}}..--}}}}}}}}}}}.|..|.........|}
}|...}S...}}.}}}}}}}}}}}}}}}-..--------}}}}}}}}}}}}}}...}}}.|..--------..S}
}|...}|...}}}}}}}..}}}}}}----..|....-}}}}}}}}}}}}}}}}}..}}}.|............|}
}|....|}}}}}....}}}}..}}.-.......----}}......}}}}}}.......}}|............|}
}------}}}}}}}}}}}}}}}}}}---------}}}}}}}}}}}}}}}}}}}}}}}}}}--------------}
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
]]);
-- Dungeon Description
des.region(selection.area(00,00,74,19),"lit")
des.region(selection.area(02,03,05,16),"unlit")
-- fixup_special hack: the first room defined on a Medusa level gets some
-- leaderboard statues; setting the region as irregular makes it a room
des.region({ region={61,03, 72,16}, lit=0, type="ordinary",irregular = 1 })
des.region(selection.area(71,08,72,11),"unlit")
-- make the downstairs area a real room to control arriving monsters
des.region({ region={67,08,69,11}, lit=1, type="ordinary", arrival_room=true })
-- Teleport: down to up stairs island, up to Medusa's island
des.teleport_region({ region = {02,03,05,16}, dir="down" })
des.teleport_region({ region = {61,03,72,16}, dir="up" })
-- Stairs
des.stair("up", 04,09)
des.stair("down", 68,10)
-- Doors
des.door("locked", 71,07)
-- Branch, not allowed on Medusa's island.
des.levregion({ type="branch", region = {01,00,79,20}, exclude = {59,01,73,17} })
-- Non diggable walls
des.non_diggable(selection.area(01,02,06,17))
des.non_diggable(selection.area(60,02,73,17))
-- Objects
des.object({ id = "statue", x=68,y=10,buc="uncursed",
                      montype="knight", historic=1, male=1,name="Perseus",
                      contents = function()
                         if percent(25) then
                            des.object({ id = "shield of reflection", buc="cursed", spe=0 })
                         end
                         if percent(75) then
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
des.object({ id = "statue", x=64, y=08, contents=0 })
des.object({ id = "statue", x=65, y=08, contents=0 })
des.object({ id = "statue", x=64, y=09, contents=0 })
des.object({ id = "statue", x=65, y=09, contents=0 })
des.object({ id = "statue", x=64, y=10, contents=0 })
des.object({ id = "statue", x=65, y=10, contents=0 })
des.object({ id = "statue", x=64, y=11, contents=0 })
des.object({ id = "statue", x=65, y=11, contents=0 })
des.object("boulder",04,04)
des.object("/",52,09)
des.object("boulder",52,09)
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
-- Traps
des.trap("magic",03,12)
des.trap()
des.trap()
des.trap()
des.trap()
-- Monsters.
des.monster({ id="Medusa",x=68,y=10,asleep=1 })
des.monster("gremlin",02,14)
des.monster("titan",02,05)
des.monster("electric eel",10,13)
des.monster("electric eel",11,13)
des.monster("electric eel",10,14)
des.monster("electric eel",11,14)
des.monster("electric eel",10,15)
des.monster("electric eel",11,15)
des.monster("jellyfish",01,01)
des.monster("jellyfish",00,08)
des.monster("jellyfish",04,19)
des.monster({ id = "stone golem",x=64,y=08,asleep=1 })
des.monster({ id = "stone golem",x=65,y=08,asleep=1 })
des.monster({ id = "stone golem",x=64,y=09,asleep=1 })
des.monster({ id = "stone golem",x=65,y=09,asleep=1 })
des.monster({ id = "cobra",x=64,y=10,asleep=1 })
des.monster({ id = "cobra",x=65,y=10,asleep=1 })
des.monster("A",72,08)
des.monster({ id = "yellow light",x=72,y=11,asleep=1 })
des.monster({ x = 17, y = 07 })
des.monster({ x = 28, y = 11 })
des.monster({ x = 32, y = 13 })
des.monster({ x = 49, y = 09 })
des.monster({ x = 48, y = 07 })
des.monster({ x = 65, y = 03 })
des.monster({ x = 70, y = 04 })
des.monster({ x = 70, y = 15 })
des.monster({ x = 65, y = 16 })
des.monster()
des.monster()
des.monster()
des.monster()

