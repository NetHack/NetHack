-- NetHack sokoban soko1-2.lua	$NHDT-Date: 1652196034 2022/05/10 15:20:34 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.6 $
--	Copyright (c) 1998-1999 by Kevin Hugo
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "noteleport", "premapped", "sokoban", "solidify", "nomongen");

des.map([[
  ------------------------
  |......................|
  |..-------------------.|
----.|    -----        |.|
|..|.--  --...|        |.|
|.....|--|....|        |.|
|.....|..|....|        |.|
--....|......--        |.|
 |.......|...|   ------|.|
 |....|..|...| --|.....|.|
 |....|--|...| |.+.....|.|
 |.......|..-- |-|.....|.|
 ----....|.--  |.+.....+.|
    ---.--.|   |-|.....|--
     |.....|   |.+.....|  
     |..|..|   --|.....|  
     -------     -------  
]]);

local place = selection.new();
place:set(16,10);
place:set(16,12);
place:set(16,14);

des.stair("down", 06,15);
des.region(selection.area(00,00,25,16),"lit");
des.non_diggable(selection.area(00,00,25,16));
des.non_passwall(selection.area(00,00,25,16));

-- Boulders
des.object("boulder",04,04);
des.object("boulder",02,06);
des.object("boulder",03,06);
des.object("boulder",04,07);
des.object("boulder",05,07);
des.object("boulder",02,08);
des.object("boulder",05,08);
des.object("boulder",03,09);
des.object("boulder",04,09);
des.object("boulder",03,10);
des.object("boulder",05,10);
des.object("boulder",06,12);
--
des.object("boulder",07,14);
--
des.object("boulder",11,05);
des.object("boulder",12,06);
des.object("boulder",10,07);
des.object("boulder",11,07);
des.object("boulder",10,08);
des.object("boulder",12,09);
des.object("boulder",11,10);

-- prevent monster generation over the (filled) holes
des.exclusion({ type = "monster-generation", region = { 05,01, 23,01 } });
-- Traps
des.trap("rolling boulder",05,01)
des.trap("hole",06,01)
des.trap("hole",07,01)
des.trap("hole",08,01)
des.trap("hole",09,01)
des.trap("hole",10,01)
des.trap("hole",11,01)
des.trap("hole",12,01)
des.trap("hole",13,01)
des.trap("hole",14,01)
des.trap("hole",15,01)
des.trap("hole",16,01)
des.trap("hole",17,01)
des.trap("hole",18,01)
des.trap("hole",19,01)
des.trap("hole",20,01)
des.trap("hole",21,01)
des.trap("hole",22,01)
des.trap("hole",23,01)

des.monster({ id = "giant mimic", appear_as = "obj:boulder" });
des.monster({ id = "giant mimic", appear_as = "obj:boulder" });

-- Random objects
des.object({ class = "%" });
des.object({ class = "%" });
des.object({ class = "%" });
des.object({ class = "%" });
des.object({ class = "=" });
des.object({ class = "/" });

-- Rewards
des.door("locked",23,12)
des.door("closed",17,10)
des.door("closed",17,12)
des.door("closed",17,14)
des.region({ region={18,09, 22,15}, lit = 1, type = "zoo", filled = 0, irregular = 1 });
-- Builders stand in the corners of this room, not on the branch stair.
des.levregion({ type = "branch", region = {20,11,20,11} })
local builders = {
   {18,09,"Akkadian builder"}, {22,09,"Sumerian builder"},
   {18,15,"Hebrew builder"}, {22,15,"Canaanite builder"},
   {18,10,"Egyptian builder"}, {22,14,"Elamite builder"},
}
for _, b in ipairs(builders) do
   des.monster({ id = "human", x = b[1], y = b[2], peaceful = 1,
                 confused = true, waiting = 1, name = b[3] })
end

local pt = selection.rndcoord(place);
if percent(25) then
   des.object({ id="bag of holding", coord=pt,
		buc="not-cursed", achievement=1 });
else
   des.object({ id="amulet of reflection", coord=pt,
		buc="not-cursed", achievement=1 });
end
des.engraving({ coord = pt, type = "burn", text = "PSALM 91" });
des.object({ id = "verse of righteous fear", coord = pt, buc = "cursed" });
