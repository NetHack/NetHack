-- NetHack 3.7	hellfill.des	$NHDT-Date: 1432512783 2015/05/25 00:13:03 $  $NHDT-Branch: master $:$NHDT-Revision: 1.25 $
--	Copyright (c) 2022 by Pasi Kallinen
-- NetHack may be freely redistributed.  See license for details.
--
--

--	The "fill" level for gehennom.
--
--	This level is used to fill out any levels not occupied by
--	specific levels.
--

function hellobjects()
   local objclass = { "(", "/", "=", "+", ")", "[", "?", "*", "%" };
   shuffle(objclass);

   des.object(objclass[1]);
   des.object(objclass[1]);
   des.object(objclass[2]);
   des.object(objclass[3]);
   des.object(objclass[4]);
   des.object(objclass[5]);
   des.object()
   des.object()
end

--

function hellmonsters()
   local monclass = { "V", "D", " ", "&", "Z" };
   shuffle(monclass);

   des.monster({ class = monclass[1], peaceful = 0 });
   des.monster({ class = monclass[1], peaceful = 0 });
   des.monster({ class = monclass[2], peaceful = 0 });
   des.monster({ class = monclass[2], peaceful = 0 });
   des.monster({ class = monclass[3], peaceful = 0 });
   des.monster({ class = monclass[4], peaceful = 0 });
   des.monster({ peaceful = 0 });
   des.monster({ class = "H", peaceful = 0 });
end

--

function helltraps()
   for i = 1, 12 do
      des.trap()
   end
end

--

function populatemaze()
   for i = 1, math.random(8) + 11 do
      if (percent(50)) then
         des.object("*");
      else
         des.object();
      end
   end

   for i = 1, math.random(10) + 2 do
      des.object("`");
   end

   for i = 1, math.random(3) do
      des.monster({ id = "minotaur", peaceful = 0 });
   end

   for i = 1, math.random(5) + 7 do
      des.monster({ peaceful = 0 });
   end

   for i = 1, math.random(6) + 7 do
      des.gold();
   end

   for i = 1, math.random(6) + 7 do
      des.trap();
   end
end

--

-- TODO: cold hells? (ice & water instead of lava. sometimes floor = ice)
-- TODO: more hell_tweaks:
--    - replacing walls with iron bars
--    - random prefab areas
--    - replace all walls (in a full level width/height) in a small area

hells = {
   -- 1: "mines" style with lava
   function ()
      des.level_init({ style = "solidfill", fg = " " });
      des.level_flags("mazelevel", "noflip");
      des.level_init({ style="mines", fg=".", smoothed=true ,joined=true, lit=0, walled=true });
      des.replace_terrain({ fromterrain = " ", toterrain = "L" });
      des.replace_terrain({ fromterrain = ".", toterrain = "L", chance = 5 });
      des.replace_terrain({ mapfragment = [[w]], toterrain = "L", chance = 20 });
      des.replace_terrain({ mapfragment = [[w]], toterrain = ".", chance = 15 });
   end,

   -- 2: mazes like original, with some hell_tweaks
   function ()
      des.level_flags("mazelevel", "noflip");
      des.level_init({ style = "mazegrid", bg = "-" });
      des.mazewalk(01,10,"east");
      local tmpbounds = selection.match("-");
      local bnds = tmpbounds:bounds();
      local protected_area = selection.fillrect(bnds.lx, bnds.ly + 1, bnds.hx - 2, bnds.hy - 1);
      hell_tweaks(protected_area:negate());
   end,

   -- 3: mazes, style 1: wall thick = 1, random wid corr
   function ()
      des.level_init({ style = "solidfill", fg = " " });
      des.level_flags("mazelevel", "noflip");
      des.level_init({ style = "maze", wallthick = 1 });
   end,

   -- 4: mazes, style 2: replace wall with iron bars of lava
   function ()
      des.level_init({ style = "solidfill", fg = " " });
      des.level_flags("mazelevel", "noflip");
      des.level_init({ style = "maze", wallthick = 1 });
      local outside_walls = selection.match(" ");
      local wallterrain = { "F", "L" };
      shuffle(wallterrain);
      des.replace_terrain({ mapfragment = "w", toterrain = wallterrain[1] });
      des.terrain(outside_walls, " ");  -- return the outside back to solid wall
   end,

   -- 5: mazes, thick walls, occasionally lava instead of walls
   function ()
      des.level_init({ style = "solidfill", fg = " " });
      des.level_flags("mazelevel", "noflip");
      des.level_init({ style = "maze", wallthick = 1 + math.random(2), corrwid = math.random(2) });
      if (percent(50)) then
         local outside_walls = selection.match(" ");
         des.replace_terrain({ mapfragment = "w", toterrain = "L" });
         des.terrain(outside_walls, " ");  -- return the outside back to solid wall
      end
   end,
};

local hellno = math.random(1, #hells);
hells[hellno]();

--

des.stair("up")
des.stair("down")

des.region(selection.area(00,00,77,21),"unlit");
populatemaze();
