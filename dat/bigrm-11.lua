-- NetHack bigroom bigrm-11.lua	$NHDT-Date: 1781994878 2026/06/20 22:34:38 $  $NHDT-Branch: NetHack-5.0 $:$NHDT-Revision: 1.1 $
--	Copyright (c) 2021 by Pasi Kallinen
-- NetHack may be freely redistributed.  See license for details.
--
-- Boulder "maze" with wide corridors

function t_or_f() return percent(50) and true or false; end

des.level_flags("mazelevel", "noflip");
des.level_init({ style = "maze", corrwid = 3 + nh.rn2(3), wallthick = 1, deadends=t_or_f() });

des.region(selection.area(00,00,75,18), "lit");
des.non_diggable();

function replace_wall_boulder(x,y)
   des.terrain(x, y, ".");
   des.object("boulder", x, y);
end

-- replace horizontal and vertical walls
local sel = selection.match([[.w.]]) | selection.match(".\nw\n.");
sel:iterate(replace_wall_boulder);
-- replace the leftover corner walls
local sel = selection.match([[.w.]]);
sel:iterate(replace_wall_boulder);

des.stair("up");
des.stair("down");

for i = 1,15 do
   des.object();
end
for i = 1,6 do
   des.trap("rolling boulder");
end
for i = 1,28 do
  des.monster();
end

