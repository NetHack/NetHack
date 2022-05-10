-- NetHack mines minend-1.lua	$NHDT-Date: 1652196029 2022/05/10 15:20:29 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.3 $
--	Copyright (c) 1989-95 by Jean-Christophe Collet
--	Copyright (c) 1991-95 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
-- Mine end level variant 1
-- "Mimic of the Mines"

des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel");

des.map([[
------------------------------------------------------------------   ------
|                        |.......|     |.......-...|       |.....|.       |
|    ---------        ----.......-------...........|       ---...-S-      |
|    |.......|        |..........................-S-      --.......|      |
|    |......-------   ---........................|.       |.......--      |
|    |..--........-----..........................|.       -.-..----       |
|    --..--.-----........-.....................---        --..--          |
|     --..--..| -----------..................---.----------..--           |
|      |...--.|    |..S...S..............---................--            |
|     ----..-----  ------------........--- ------------...---             |
|     |.........--            ----------              ---...-- -----      |
|    --.....---..--                           --------  --...---...--     |
| ----..-..-- --..---------------------      --......--  ---........|     |
|--....-----   --..-..................---    |........|    |.......--     |
|.......|       --......................S..  --......--    ---..----      |
|--.--.--        ----.................---     ------..------...--         |
| |....S..          |...............-..|         ..S...........|          |
--------            --------------------           ------------------------
]]);

-- Dungeon Description
local place = { {08,16},{13,07},{21,08},{41,14},{50,04},{50,16},{66,01} }
shuffle(place)

-- make the entry chamber a real room; it affects monster arrival
des.region({ region={26,01,32,01}, lit=0, type="ordinary", irregular=1, arrival_room=true })
des.region(selection.area(20,08,21,08),"unlit")
des.region(selection.area(23,08,25,08),"unlit");
-- Secret doors
des.door("locked",07,16)
des.door("locked",22,08)
des.door("locked",26,08)
des.door("locked",40,14)
des.door("locked",50,03)
des.door("locked",51,16)
des.door("locked",66,02)
-- Stairs
des.stair("up", 36,04)
-- Non diggable walls
des.non_diggable(selection.area(00,00,74,17))
-- Niches
-- Note: place[6] empty
des.object("diamond",place[7])
des.object("emerald",place[7])
des.object("worthless piece of violet glass",place[7])
des.monster({ class="m", coord=place[7], appear_as="obj:luckstone" })
des.object("worthless piece of white glass",place[1])
des.object("emerald",place[1])
des.object("amethyst",place[1])
des.monster({ class="m", coord=place[1], appear_as="obj:loadstone" })
des.object("diamond",place[2])
des.object("worthless piece of green glass",place[2])
des.object("amethyst",place[2])
des.monster({ class="m", coord=place[2], appear_as="obj:flint" })
des.object("worthless piece of white glass",place[3])
des.object("emerald",place[3])
des.object("worthless piece of violet glass",place[3])
des.monster({ class="m", coord=place[3], appear_as="obj:touchstone" })
des.object("worthless piece of red glass",place[4])
des.object("ruby",place[4])
des.object("loadstone",place[4])
des.object("ruby",place[5])
des.object("worthless piece of red glass",place[5])
des.object({ id="luckstone", coord=place[5], buc="not-cursed", achievement=1 })
-- Random objects
des.object("*")
des.object("*")
des.object("*")
des.object("*")
des.object("*")
des.object("*")
des.object("*")
des.object("(")
des.object("(")
des.object()
des.object()
des.object()
-- Random traps
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
-- Random monsters
des.monster("gnome king")
des.monster("gnome lord")
des.monster("gnome lord")
des.monster("gnome lord")
des.monster("gnomish wizard")
des.monster("gnomish wizard")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("gnome")
des.monster("hobbit")
des.monster("hobbit")
des.monster("dwarf")
des.monster("dwarf")
des.monster("dwarf")
des.monster("h")
