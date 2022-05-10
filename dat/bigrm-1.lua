-- NetHack bigroom bigrm-1.lua	$NHDT-Date: 1652196021 2022/05/10 15:20:21 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.3 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1990 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });
des.level_flags("mazelevel", "noflip");

des.map([[
---------------------------------------------------------------------------
|.........................................................................|
|.........................................................................|
|.........................................................................|
|.........................................................................|
|.........................................................................|
|.........................................................................|
|.........................................................................|
|.........................................................................|
|.........................................................................|
|.........................................................................|
|.........................................................................|
|.........................................................................|
|.........................................................................|
|.........................................................................|
|.........................................................................|
|.........................................................................|
---------------------------------------------------------------------------
]]);


if percent(75) then
   local terrains = { "-", "F", "L", "T", "C" };
   local tidx = math.random(1, #terrains);
   local choice = math.random(0, 4);
   if choice == 0 then
      -- one horizontal line
      des.terrain(selection.line(10,8, 65,8), terrains[tidx]);
   elseif choice == 1 then
      -- two vertical lines
      local sel = selection.line(15,4, 15, 13) | selection.line(59,4, 59, 13);
      des.terrain(sel, terrains[tidx]);
   elseif choice == 2 then
      -- plus sign
      local sel = selection.line(10,8, 64, 8) | selection.line(37,3, 37, 14);
      des.terrain(sel, terrains[tidx]);
   elseif choice == 3 then
      -- brackets:  [  ]
      des.terrain(selection.rect(4,4, 70,13), terrains[tidx]);
      local sel = selection.line(25,4, 50,4) | selection.line(25,13, 50,13);
      des.terrain(sel, '.');
   else
      -- nothing
   end
end

des.region(selection.area(01,01, 73, 16), "lit");

des.stair("up");
des.stair("down");

des.non_diggable();

for i = 1,15 do
   des.object();
end

for i = 1,6 do
   des.trap();
end

for i = 1,28 do
  des.monster();
end
