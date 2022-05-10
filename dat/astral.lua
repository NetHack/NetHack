-- NetHack endgame astral.lua	$NHDT-Date: 1652196020 2022/05/10 15:20:20 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.7 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1992,1993 by Izchak Miller, David Cohrs,
--                      and Timo Hakulinen
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "noteleport", "hardfloor", "nommap", "shortsighted", "solidify")
des.message("You arrive on the Astral Plane!")
des.message("Here the High Temple of %d is located.")
des.message("You sense alarm, hostility, and excitement in the air!")
des.map([[
                              ---------------                              
                              |.............|                              
                              |..---------..|                              
                              |..|.......|..|                              
---------------               |..|.......|..|               ---------------
|.............|               |..|.......|..|               |.............|
|..---------..-|   |-------|  |..|.......|..|  |-------|   |-..---------..|
|..|.......|...-| |-.......-| |..|.......|..| |-.......-| |-...|.......|..|
|..|.......|....-|-.........-||..----+----..||-.........-|-....|.......|..|
|..|.......+.....+...........||.............||...........+.....+.......|..|
|..|.......|....-|-.........-|--|.........|--|-.........-|-....|.......|..|
|..|.......|...-| |-.......-|   -|---+---|-   |-.......-| |-...|.......|..|
|..---------..-|   |---+---|    |-.......-|    |---+---|   |-..---------..|
|.............|      |...|-----|-.........-|-----|...|      |.............|
---------------      |.........|...........|.........|      ---------------
                     -------...|-.........-|...-------                     
                           |....|-.......-|....|                           
                           ---...|---+---|...---                           
                             |...............|                             
                             -----------------                             
]]);

-- chance to alter above map and turn the wings of the bottom-center into
-- a pair of big (5x15) rooms
for i=1,2 do
   -- 3.6.[01]: 75% chance that both sides opened up, 25% that neither did;
   -- 3.6.2: 60% twice == 36% chance that both sides open up, 24% left side
   -- only, 24% right side only, 16% that neither side opens up
   local hall;
   if percent(60) then
     if i == 1 then
        des.terrain(selection.area(17,14, 30,18),".")
        des.wallify()
        -- temporarily close off the area to be filled so that it doesn't cover
        -- the entire entry area
        des.terrain(33,18, "|")
        hall = selection.floodfill(30,16)
        -- re-connect the opened wing with the rest of the map
        des.terrain(33,18, ".")
     else
        des.terrain(selection.area(44,14, 57,18),".")
        des.wallify()
        des.terrain(41,18, "|")
        hall = selection.floodfill(44,16)
        des.terrain(41,18, ".")
     end
     -- extra monsters; was [6 + 3d4] when both wings were opened up at once
     for i=1,3 + math.random(2 - 1,2*3) do
        des.monster({ id="Angel", coord = hall:rndcoord(1), align="noalign", peaceful=0 })
        if percent(50) then
           des.monster({ coord = hall:rndcoord(1), peaceful=0 })
        end
     end
   end
end

-- Rider locations
local place = selection.new();
place:set(23,9);
place:set(37,14);
place:set(51,9);

-- Where the player will land on arrival
des.teleport_region({ region = {29,15,45,15}, exclude = {30,15,44,15} })
-- Lit courts
des.region({ region={01,05,16,14},lit=1,type="ordinary",irregular=1 })
des.region({ region={31,01,44,10},lit=1,type="ordinary",irregular=1 })
des.region({ region={61,05,74,14},lit=1,type="ordinary",irregular=1 })
-- A Sanctum for each alignment
-- The shrines' alignments are shuffled for
-- each game
des.region({ region={04,07,10,11},lit=1,type="temple",filled=2 })
des.region({ region={34,03,40,07},lit=1,type="temple",filled=2 })
des.region({ region={64,07,70,11},lit=1,type="temple",filled=2 })

des.altar({ x=07, y=09, align=align[1],type="sanctum" })
des.altar({ x=37, y=05, align=align[2],type="sanctum" })
des.altar({ x=67, y=09, align=align[3],type="sanctum" })
-- Doors
des.door("closed",11,09)
des.door("closed",17,09)
des.door("locked",23,12)
des.door("locked",37,08)
des.door("closed",37,11)
des.door("closed",37,17)
des.door("locked",51,12)
des.door("locked",57,09)
des.door("closed",63,09)
-- Non diggable and phazeable everywhere
des.non_diggable(selection.area(00,00,74,19))
des.non_passwall(selection.area(00,00,74,19))
-- Moloch's horde
-- West round room
des.monster({ id = "aligned cleric",x=18,y=09,align="noalign", peaceful=0 })
des.monster({ id = "aligned cleric",x=19,y=08,align="noalign", peaceful=0 })
des.monster({ id = "aligned cleric",x=19,y=09,align="noalign", peaceful=0 })
des.monster({ id = "aligned cleric",x=19,y=10,align="noalign", peaceful=0 })
des.monster({ id = "Angel",x=20,y=09,align="noalign", peaceful=0 })
des.monster({ id = "Angel",x=20,y=10,align="noalign", peaceful=0 })
des.monster({ id = "Pestilence", coord = place:rndcoord(1), peaceful=0 })
-- South-central round room
des.monster({ id = "aligned cleric",x=36,y=12,align="noalign", peaceful=0 })
des.monster({ id = "aligned cleric",x=37,y=12,align="noalign", peaceful=0 })
des.monster({ id = "aligned cleric",x=38,y=12,align="noalign", peaceful=0 })
des.monster({ id = "aligned cleric",x=36,y=13,align="noalign", peaceful=0 })
des.monster({ id = "Angel",x=38,y=13,align="noalign", peaceful=0 })
des.monster({ id = "Angel",x=37,y=13,align="noalign", peaceful=0 })
des.monster({ id = "Death", coord = place:rndcoord(1), peaceful=0 })
-- East round room
des.monster({ id = "aligned cleric",x=56,y=09,align="noalign", peaceful=0 })
des.monster({ id = "aligned cleric",x=55,y=08,align="noalign", peaceful=0 })
des.monster({ id = "aligned cleric",x=55,y=09,align="noalign", peaceful=0 })
des.monster({ id = "aligned cleric",x=55,y=10,align="noalign", peaceful=0 })
des.monster({ id = "Angel",x=54,y=09,align="noalign", peaceful=0 })
des.monster({ id = "Angel",x=54,y=10,align="noalign", peaceful=0 })
des.monster({ id = "Famine", coord = place:rndcoord(1), peaceful=0 })
--
-- The aligned horde
--
-- We do not know in advance the alignment of the
-- player.  The mpeaceful bit will need resetting
-- when the level is created.  The setting here is
-- but a place holder.
--
-- West court
des.monster({ id = "aligned cleric",x=12,y=07,align="chaos", peaceful=0 })
des.monster({ id = "aligned cleric",x=13,y=07,align="chaos",peaceful=1 })
des.monster({ id = "aligned cleric",x=14,y=07,align="law", peaceful=0 })
des.monster({ id = "aligned cleric",x=12,y=11,align="law",peaceful=1 })
des.monster({ id = "aligned cleric",x=13,y=11,align="neutral", peaceful=0 })
des.monster({ id = "aligned cleric",x=14,y=11,align="neutral",peaceful=1 })
des.monster({ id = "Angel",x=11,y=05,align="chaos", peaceful=0 })
des.monster({ id = "Angel",x=12,y=05,align="chaos",peaceful=1 })
des.monster({ id = "Angel",x=13,y=05,align="law", peaceful=0 })
des.monster({ id = "Angel",x=11,y=13,align="law",peaceful=1 })
des.monster({ id = "Angel",x=12,y=13,align="neutral", peaceful=0 })
des.monster({ id = "Angel",x=13,y=13,align="neutral",peaceful=1 })
-- Central court
des.monster({ id = "aligned cleric",x=32,y=09,align="chaos", peaceful=0 })
des.monster({ id = "aligned cleric",x=33,y=09,align="chaos",peaceful=1 })
des.monster({ id = "aligned cleric",x=34,y=09,align="law", peaceful=0 })
des.monster({ id = "aligned cleric",x=40,y=09,align="law",peaceful=1 })
des.monster({ id = "aligned cleric",x=41,y=09,align="neutral", peaceful=0 })
des.monster({ id = "aligned cleric",x=42,y=09,align="neutral",peaceful=1 })
des.monster({ id = "Angel",x=31,y=08,align="chaos", peaceful=0 })
des.monster({ id = "Angel",x=32,y=08,align="chaos",peaceful=1 })
des.monster({ id = "Angel",x=31,y=09,align="law", peaceful=0 })
des.monster({ id = "Angel",x=42,y=08,align="law",peaceful=1 })
des.monster({ id = "Angel",x=43,y=08,align="neutral", peaceful=0 })
des.monster({ id = "Angel",x=43,y=09,align="neutral",peaceful=1 })
-- East court
des.monster({ id = "aligned cleric",x=60,y=07,align="chaos", peaceful=0 })
des.monster({ id = "aligned cleric",x=61,y=07,align="chaos",peaceful=1 })
des.monster({ id = "aligned cleric",x=62,y=07,align="law", peaceful=0 })
des.monster({ id = "aligned cleric",x=60,y=11,align="law",peaceful=1 })
des.monster({ id = "aligned cleric",x=61,y=11,align="neutral", peaceful=0 })
des.monster({ id = "aligned cleric",x=62,y=11,align="neutral",peaceful=1 })
des.monster({ id = "Angel",x=61,y=05,align="chaos", peaceful=0 })
des.monster({ id = "Angel",x=62,y=05,align="chaos",peaceful=1 })
des.monster({ id = "Angel",x=63,y=05,align="law", peaceful=0 })
des.monster({ id = "Angel",x=61,y=13,align="law",peaceful=1 })
des.monster({ id = "Angel",x=62,y=13,align="neutral", peaceful=0 })
des.monster({ id = "Angel",x=63,y=13,align="neutral",peaceful=1 })
--
-- Assorted nasties
des.monster({ class = "L", peaceful=0 })
des.monster({ class = "L", peaceful=0 })
des.monster({ class = "L", peaceful=0 })
des.monster({ class = "V", peaceful=0 })
des.monster({ class = "V", peaceful=0 })
des.monster({ class = "V", peaceful=0 })
des.monster({ class = "D", peaceful=0 })
des.monster({ class = "D", peaceful=0 })
des.monster({ class = "D", peaceful=0 })
