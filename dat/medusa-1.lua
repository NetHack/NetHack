-- NetHack medusa medusa-1.lua	$NHDT-Date: 1652196027 2022/05/10 15:20:27 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.4 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1990, 1991 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
--
-- These are the Medusa's levels :
--

des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "noteleport")

des.map([[
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
}}.}}}}}..}}}}}......}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}....}}}...}}}}}
}...}}.....}}}}}....}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}...............}
}....}}}}}}}}}}....}}}..}}}}}}}}}}}.......}}}}}}}}}}}}}}}}..}}.....}}}...}}
}....}}}}}}}}.....}}}}..}}}}}}.................}}}}}}}}}}}.}}}}.....}}...}}
}....}}}}}}}}}}}}.}}}}.}}}}}}.-----------------.}}}}}}}}}}}}}}}}}.........}
}....}}}}}}}}}}}}}}}}}}.}}}...|...............S...}}}}}}}}}}}}}}}}}}}....}}
}.....}.}}....}}}}}}}}}.}}....--------+--------....}}}}}}..}}}}}}}}}}}...}}
}......}}}}..}}}}}}}}}}}}}........|.......|........}}}}}....}}}}}}}}}}}}}}}
}.....}}}}}}}}}}}}}}}}}}}}........|.......|........}}}}}...}}}}}}}}}.}}}}}}
}.....}}}}}}}}}}}}}}}}}}}}....--------+--------....}}}}}}.}.}}}}}}}}}}}}}}}
}......}}}}}}}}}}}}}}}}}}}}...S...............|...}}}}}}}}}}}}}}}}}.}}}}}}}
}.......}}}}}}}..}}}}}}}}}}}}.-----------------.}}}}}}}}}}}}}}}}}....}}}}}}
}........}}.}}....}}}}}}}}}}}}.................}}}}}..}}}}}}}}}.......}}}}}
}.......}}}}}}}......}}}}}}}}}}}}}}.......}}}}}}}}}.....}}}}}}...}}..}}}}}}
}.....}}}}}}}}}}}.....}}}}}}}}}}}}}}}}}}}}}}.}}}}}}}..}}}}}}}}}}....}}}}}}}
}}..}}}}}}}}}}}}}....}}}}}}}}}}}}}}}}}}}}}}...}}..}}}}}}}.}}.}}}}..}}}}}}}}
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
]]);
-- Dungeon Description
des.region(selection.area(00,00,74,19),"lit")
des.region(selection.area(31,07,45,07),"unlit")
-- make the downstairs room a real room to control arriving monsters, 
-- and also as a fixup_special hack; the first room defined on Medusa's level
-- receives some statues
des.region({ region={35,09, 41,10}, lit = 0, type="ordinary", arrival_room=true })
des.region(selection.area(31,12,45,12),"unlit")
-- Teleport: down to up stairs island, up to Medusa's island
des.teleport_region({ region = {01,01,05,17}, dir="down" })
des.teleport_region({ region = {26,04,50,15}, dir="up" })
-- Stairs
des.stair("up", 05,14)
des.stair("down", 36,10)
-- Doors
des.door("closed",46,07)
des.door("locked",38,08)
des.door("locked",38,11)
des.door("closed",30,12)
-- Branch, not allowed inside Medusa's building.
des.levregion({ region = {01,00,79,20}, exclude = {30,06,46,13}, type = "branch" })
-- Non diggable walls
des.non_diggable(selection.area(30,06,46,13))
-- Objects
des.object({ id = "statue", x=36,y=10, buc="uncursed",
             montype="knight", historic=1, male = 1, name="Perseus",
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

-- Specifying explicit contents forces them to be empty.
des.object({ id = "statue", contents = 0 })
des.object({ id = "statue", contents = 0 })
des.object({ id = "statue", contents = 0 })
des.object({ id = "statue", contents = 0 })
des.object({ id = "statue", contents = 0 })
des.object({ id = "statue", contents = 0 })
des.object({ id = "statue", contents = 0 })
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
-- Random traps
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap("board",38,07)
des.trap("board",38,12)
-- Random monsters
des.monster({ id = "Medusa", x=36,y=10, asleep=1 })
des.monster("giant eel",11,06)
des.monster("giant eel",23,13)
des.monster("giant eel",29,02)
des.monster("jellyfish",02,02)
des.monster("jellyfish",00,08)
des.monster("jellyfish",04,18)
des.monster("water troll",51,03)
des.monster("water troll",64,11)
des.monster({ class = 'S', x=38, y=07 })
des.monster({ class = 'S', x=38, y=12 })
des.monster()
des.monster()
des.monster()
des.monster()
des.monster()
des.monster()
des.monster()
des.monster()
des.monster()
des.monster()
