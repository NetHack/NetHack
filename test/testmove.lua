
-- Tests for moving the hero

nh.parse_config("OPTIONS=number_pad:0");
nh.parse_config("OPTIONS=runmode:teleport");

local POS = { x = 10, y = 05 };
local number_pad = 0;

function initlev()
   nh.debug_flags({mongen = false, hunger = false, overwrite_stairs = true });
   des.level_flags("noflip");
   des.reset_level();
   des.level_init({ style = "solidfill", fg = ".", lit = true });
   des.teleport_region({ region = {POS.x,POS.y,POS.x,POS.y}, region_islev = true, dir="both" });
   des.finalize_level();
   for k, v in pairs(nh.stairways()) do
      des.terrain(v.x, v.y, ".");
   end
end

function ctrl(key)
   return string.char(0x1F & string.byte(key));
end

function meta(key)
   return string.char(0x80 | string.byte(key));
end

function setup1(param)
   des.terrain(POS.x - 1, POS.y, param);
end

function setup2(param)
   des.terrain(POS.x + 16, POS.y, param);
end

function setup3(param)
   local mapstr = [[
-------
|CCCCC|
|C---C|
|C---C|
|CCCCC|
-------
]];
   des.map({ x = POS.x - 1, y = POS.y - 4, map = mapstr });
   des.replace_terrain({ region={01,01, 74,18}, fromterrain="C", toterrain=param, chance=100 });
end

function setup4(param)
   local mapstr = [[
------------
|CCCC|CCC| |
|---C|C|C|--
|  |CCC|CCC|
------------
]];
   des.map({ x = POS.x - 1, y = POS.y - 1, map = mapstr });
   des.replace_terrain({ region={01,01, 74,18}, fromterrain="C", toterrain=param, chance=100 });
end

local basicmoves = {
   -- move
   h = { dx = -1,  dy =  0, number_pad = 0 },
   j = { dx =  0,  dy =  1, number_pad = 0 },
   k = { dx =  0,  dy = -1, number_pad = 0 },
   l = { dx =  1,  dy =  0, number_pad = 0 },
   y = { dx = -1,  dy = -1, number_pad = 0 },
   u = { dx =  1,  dy = -1, number_pad = 0 },
   b = { dx = -1,  dy =  1, number_pad = 0 },
   n = { dx =  1,  dy =  1, number_pad = 0 },

   -- run
   H = { x = 2,  y = POS.y, number_pad = 0 },
   J = { x = POS.x, y = nhc.ROWNO-1, number_pad = 0 },
   K = { x = POS.x, y = 0, number_pad = 0 },
   L = { x = nhc.COLNO-2, y = POS.y, number_pad = 0 },
   Y = { x = POS.x - POS.y, y = 0, number_pad = 0 },
   U = { x = POS.x + POS.y, y = 0, number_pad = 0 },
   B = { x = 2, y = 13, number_pad = 0 },
   N = { x = 25, y = nhc.ROWNO-1, number_pad = 0 },

   -- rush
   [ctrl("h")] = { x = 2,  y = POS.y, number_pad = 0 },
   [ctrl("j")] = { x = POS.x, y = nhc.ROWNO-1, number_pad = 0 },
   [ctrl("k")] = { x = POS.x, y = 0, number_pad = 0 },
   [ctrl("l")] = { x = nhc.COLNO-2, y = POS.y, number_pad = 0 },
   [ctrl("y")] = { x = POS.x - POS.y, y = 0, number_pad = 0 },
   [ctrl("u")] = { x = POS.x + POS.y, y = 0, number_pad = 0 },
   [ctrl("b")] = { x = 2, y = 13, number_pad = 0 },
   [ctrl("n")] = { x = 25, y = nhc.ROWNO-1, number_pad = 0 },

   -- move, number_pad
   ["4"] = { dx = -1,  dy =  0, number_pad = 1 },
   ["2"] = { dx =  0,  dy =  1, number_pad = 1 },
   ["8"] = { dx =  0,  dy = -1, number_pad = 1 },
   ["6"] = { dx =  1,  dy =  0, number_pad = 1 },
   ["7"] = { dx = -1,  dy = -1, number_pad = 1 },
   ["9"] = { dx =  1,  dy = -1, number_pad = 1 },
   ["1"] = { dx = -1,  dy =  1, number_pad = 1 },
   ["3"] = { dx =  1,  dy =  1, number_pad = 1 },

   -- run (or rush), number_pad
   [meta("4")] = { x = 2,  y = POS.y, number_pad = 1 },
   [meta("2")] = { x = POS.x, y = nhc.ROWNO-1, number_pad = 1 },
   [meta("8")] = { x = POS.x, y = 0, number_pad = 1 },
   [meta("6")] = { x = nhc.COLNO-2, y = POS.y, number_pad = 1 },
   [meta("7")] = { x = POS.x - POS.y, y = 0, number_pad = 1 },
   [meta("9")] = { x = POS.x + POS.y, y = 0, number_pad = 1 },
   [meta("1")] = { x = 2, y = 13, number_pad = 1 },
   [meta("3")] = { x = 25, y = nhc.ROWNO-1, number_pad = 1 },

   -- check some terrains
   { key = "h", dx =  0, dy = 0, number_pad = 0, setup = setup1, param = " " },
   { key = "h", dx =  0, dy = 0, number_pad = 0, setup = setup1, param = "|" },
   { key = "h", dx =  0, dy = 0, number_pad = 0, setup = setup1, param = "-" },
   { key = "h", dx =  0, dy = 0, number_pad = 0, setup = setup1, param = "F" },
   { key = "h", dx = -1, dy = 0, number_pad = 0, setup = setup1, param = "#" },
   { key = "h", dx = -1, dy = 0, number_pad = 0, setup = setup1, param = "." },

   -- run and terrains
   { key = "L", x = POS.x + 15, y = POS.y, number_pad = 0, setup = setup2, param = " " },
   { key = "L", x = POS.x + 15, y = POS.y, number_pad = 0, setup = setup2, param = "|" },
   { key = "L", x = POS.x + 15, y = POS.y, number_pad = 0, setup = setup2, param = "-" },
   { key = "L", x = POS.x + 15, y = POS.y, number_pad = 0, setup = setup2, param = "F" },
   { key = "L", x = POS.x + 15, y = POS.y, number_pad = 0, setup = setup2, param = "L" },
   { key = "L", x = nhc.COLNO - 2, y = POS.y, number_pad = 0, setup = setup2, param = "#" },
   { key = "L", x = nhc.COLNO - 2, y = POS.y, number_pad = 0, setup = setup2, param = "C" },

   -- rush behaves differently in corridors vs in rooms
   { key = ctrl("l"), x = POS.x + 4, y = POS.y, number_pad = 0, setup = setup3, param = "." },
   { key = ctrl("l"), x = POS.x + 4, y = POS.y - 3, number_pad = 0, setup = setup3, param = "#" },
   { key = ctrl("k"), x = POS.x, y = POS.y - 3, number_pad = 0, setup = setup3, param = "." },
   { key = ctrl("k"), x = POS.x + 4, y = POS.y - 3, number_pad = 0, setup = setup3, param = "#" },

   { key = ctrl("l"), x = POS.x + 3, y = POS.y, number_pad = 0, setup = setup4, param = "." },
   { key = ctrl("l"), x = POS.x + 9, y = POS.y + 2, number_pad = 0, setup = setup4, param = "#" },

};



for k, v in pairs(basicmoves) do
   initlev();

   local key = v.key and v.key or k;

   if (v.number_pad ~= nil) then
      if (v.number_pad ~= number_pad) then
         nh.parse_config("OPTIONS=number_pad:" .. v.number_pad);
         number_pad = v.number_pad;
      end
   end

   if (v.setup ~= nil) then
      v.setup(v.param);
   end

   local x = u.ux;
   local y = u.uy;

   nh.pushkey(key);
   nh.doturn(true);

   if (v.dx ~= nil) then
      if (not ((x == (u.ux - v.dx)) and (y == (u.uy - v.dy)))) then
         error(string.format("Move: key '%s' gave (%i,%i), should have been (%i,%i)",
                             key, u.ux, u.uy, x + v.dx, y + v.dy));
         return;
      end
   elseif (v.x ~= nil) then
      if (not (u.ux == v.x and u.uy == v.y)) then
         error(string.format("Move: key '%s' gave (%i,%i), should have been (%i,%i)",
                             key, u.ux, u.uy, v.x, v.y));
         return;
      end
   end
end

initlev();
