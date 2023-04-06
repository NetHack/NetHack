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

function rnd_halign()
   local aligns = { "half-left", "center", "half-right" };
   return aligns[math.random(1, #aligns)];
end

function rnd_valign()
   local aligns = { "top", "center", "bottom" };
   return aligns[math.random(1, #aligns)];
end

-- the prefab maps must have contents-function, or populatemaze()
-- puts the stuff only inside the prefab map.
-- contains either a function, or an object with "repeatable" and "contents".
-- function alone implies not repeatable.
local hell_prefabs = {
   {
      repeatable = true,
      contents = function ()
      des.map({ halign = rnd_halign(), valign = "center", map = [[
......
......
......
......
......
......
......
......
......
......
......
......
......
......
......
......]], contents = function() end });
      end
   },
   {
      repeatable = true,
      contents = function ()
      des.map({ halign = rnd_halign(), valign = "center", map = [[
xxxxxx.....xxxxxx
xxxx.........xxxx
xx.............xx
xx.............xx
x...............x
x...............x
.................
.................
.................
.................
.................
x...............x
x...............x
xx.............xx
xx.............xx
xxxx.........xxxx
xxxxxx.....xxxxxx
]], contents = function() end });
      end
   },
   function (coldhell)
      des.map({ halign = rnd_halign(), valign = rnd_valign(), map = [[
xxxxxx.xxxxxx
xLLLLLLLLLLLx
xL---------Lx
xL|.......|Lx
xL|.......|Lx
.L|.......|L.
xL|.......|Lx
xL|.......|Lx
xL---------Lx
xLLLLLLLLLLLx
xxxxxx.xxxxxx
]], contents = function()
   des.non_diggable(selection.area(2,2, 10,8));
   des.region(selection.area(4,4, 8,6), "lit");
   if (coldhell) then
      des.replace_terrain({ region = {1,1, 11,9}, fromterrain="L", toterrain="P" });
   end
   local dblocs = {
      { x = 1, y = 5, dir="east", state="closed" },
      { x = 11, y = 5, dir="west", state="closed" },
      { x = 6, y = 1, dir="south", state="closed" },
      { x = 6, y = 9, dir="north", state="closed" }
   }
   shuffle(dblocs);
   for i = 1, math.random(1, #dblocs) do
      des.drawbridge(dblocs[i]);
   end
   local mons = { "H", "T", "@" };
   shuffle(mons);
   for i = 1, 3 + math.random(1, 5) do
      des.monster(mons[1], 6, 5);
   end
      end });
   end,
   {
      repeatable = true,
      contents = function ()
      des.map({ halign = "center", valign = "center", map = [[
..............................................................
..............................................................
..............................................................
..............................................................
..............................................................]], contents = function() end });
      end
   },
   {
      repeatable = true,
      contents = function ()
      des.map({ halign = rnd_halign(), valign = rnd_valign(), lit = true, map = [[
x.....x
.......
.......
.......
.......
.......
x.....x]], contents = function() end  });
   end
   },
   function ()
      des.map({ halign = rnd_halign(), valign = rnd_valign(), map = [[
BBBBBBB
B.....B
B.....B
B.....B
B.....B
B.....B
BBBBBBB]], contents = function()
   des.region({ region={2,2, 2,2}, type="temple", filled=1, irregular=1 });
   des.altar({ x=3, y=3, align="noalign", type=percent(75) and "altar" or "shrine" });
      end  });
   end,
   function ()
      des.map({ halign = rnd_halign(), valign = rnd_valign(), map = [[
..........
..........
..........
...FFFF...
...F..F...
...F..F...
...FFFF...
..........
..........
..........]], contents = function()
   local mons = { "Angel", "D", "H", "L" };
   des.monster(mons[math.random(1, #mons)], 4,4);
      end });
   end,

   function ()
      des.map({ halign = rnd_halign(), valign = rnd_valign(), map = [[
.........
.}}}}}}}.
.}}---}}.
.}--.--}.
.}|...|}.
.}--.--}.
.}}---}}.
.}}}}}}}.
.........
]], contents = function(rm)
   des.monster("L",04,04)
      end })
   end,
   function ()
      local mapstr = percent(30) and [[
.....
.LLL.
.LZL.
.LLL.
.....]] or [[
.....
.PPP.
.PWP.
.PPP.
.....]];
      for dx = 1, 5 do
         des.map({ x = dx*14 - 4, y = math.random(3, 15),
                   map = mapstr, contents = function() end })
      end
   end,
   {
      repeatable = true,
      contents = function ()
      local mapstr = [[
...
...
...
...
...
...
...
...
...
...
...
...
...
...
...
...
...]];
      for dx = 1, 3 do
         des.map({ x = math.random(3, 75), y = 3,
                   map = mapstr, contents = function() end })
      end
   end
   },
};

function rnd_hell_prefab(coldhell)
   local dorepeat = true;
   local nloops = 0;
   repeat
      nloops = nloops + 1;
      local pf = math.random(1, #hell_prefabs);
      local fab = hell_prefabs[pf];
      local fabtype = type(fab);

      if (fabtype == "function") then
         fab(coldhell);
         dorepeat = false;
      elseif (fabtype == "table") then
         fab.contents(coldhell);
         dorepeat = not (fab.repeatable and math.random(0, nloops * 2) == 0);
      end
   until ((not dorepeat) or (nloops > 5));
end

hells = {
   -- 1: "mines" style with lava
   function ()
      des.level_init({ style = "solidfill", fg = " ", lit = 0 });
      des.level_flags("mazelevel", "noflip");
      des.level_init({ style="mines", fg=".", smoothed=true ,joined=true, lit=0, walled=true });
      des.replace_terrain({ fromterrain = " ", toterrain = "L" });
      des.replace_terrain({ fromterrain = ".", toterrain = "L", chance = 5 });
      des.replace_terrain({ mapfragment = [[w]], toterrain = "L", chance = 20 });
      des.replace_terrain({ mapfragment = [[w]], toterrain = ".", chance = 15 });
   end,

   -- 2: mazes like original, with some hell_tweaks
   function ()
      des.level_init({ style = "solidfill", fg = " ", lit = 0 });
      des.level_flags("mazelevel", "noflip");
      des.level_init({ style = "mazegrid", bg = "-" });
      des.mazewalk({ coord = {01,10}, dir = "east", stocked = false});
      local tmpbounds = selection.match("-");
      local bnds = tmpbounds:bounds();
      local protected_area = selection.fillrect(bnds.lx, bnds.ly + 1, bnds.hx - 2, bnds.hy - 1);
      hell_tweaks(protected_area:negate());
      if (percent(25)) then
         rnd_hell_prefab(false);
      end
   end,

   -- 3: mazes, style 1: wall thick = 1, random wid corr
   function ()
      des.level_init({ style = "solidfill", fg = " ", lit = 0 });
      des.level_flags("mazelevel", "noflip");
      des.level_init({ style = "maze", wallthick = 1 });
   end,

   -- 4: mazes, style 2: replace wall with iron bars or lava
   function ()
      local cwid = math.random(4);
      des.level_init({ style = "solidfill", fg = " ", lit = 0 });
      des.level_flags("mazelevel", "noflip");
      des.level_init({ style = "maze", wallthick = 1, corrwid = cwid });
      local outside_walls = selection.match(" ");
      local wallterrain = { "F", "L" };
      shuffle(wallterrain);
      des.replace_terrain({ mapfragment = "w", toterrain = wallterrain[1] });
      if (cwid == 1) then
         if (wallterrain[1] == "F" and percent(80)) then
            -- replace some horizontal iron bars walls with floor
            des.replace_terrain({ mapfragment = ".\nF\n.", toterrain = ".", chance = 25 * math.random(4) });
         elseif (percent(25)) then
            rnd_hell_prefab(false);
         end
      end
      des.terrain(outside_walls, " ");  -- return the outside back to solid wall
   end,

   -- 5: mazes, thick walls, occasionally lava instead of walls
   function ()
      des.level_init({ style = "solidfill", fg = " ", lit = 0 });
      des.level_flags("mazelevel", "noflip");
      des.level_init({ style = "maze", wallthick = 1 + math.random(2), corrwid = math.random(2) });
      if (percent(50)) then
         local outside_walls = selection.match(" ");
         des.replace_terrain({ mapfragment = "w", toterrain = "L" });
         des.terrain(outside_walls, " ");  -- return the outside back to solid wall
      end
   end,

   -- 6: cold maze, with ice and water
   function ()
      local cwid = math.random(4);
      des.level_init({ style = "solidfill", fg = " ", lit = 0 });
      des.level_flags("mazelevel", "noflip", "cold");
      des.level_init({ style = "maze", wallthick = 1, corrwid = cwid });
      local outside_walls = selection.match(" ");
      local icey = selection.negate():percentage(10):grow():filter_mapchar(".");
      des.terrain(icey, "I");
      if (cwid > 1) then
         -- turn some ice into wall of water
         des.terrain(icey:percentage(1), "W");
      end
      des.terrain(icey:percentage(5), "P");
      if (percent(25)) then
         des.terrain(selection.match("w"), "W"); -- walls of water
      end
      if (cwid == 1 and percent(25)) then
         rnd_hell_prefab(true);
      end
      des.terrain(outside_walls, " ");  -- return the outside back to solid wall
   end,
};

local hellno = math.random(1, #hells);
hells[hellno]();

--

des.stair("up")
if (u.invocation_level) then
   des.trap("vibrating square");
else
   des.stair("down")
end

populatemaze();
