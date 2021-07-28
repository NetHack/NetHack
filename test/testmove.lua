
-- Tests for moving the hero

-- TODO: running stops if hero walks over stairs -> test fails.
--       prevent stair generation? check where the stairs are?
-- TODO: prevent hero from getting hungry


nh.parse_config("OPTIONS=number_pad:0");
nh.parse_config("OPTIONS=runmode:teleport");

local POS = { x = 10, y = 05 };
local number_pad = 0;

function initlev()
   nh.monster_generation(false);
   des.level_flags("noflip");
   des.reset_level();
   des.level_init({ style = "solidfill", fg = ".", lit = true });
   des.teleport_region({ region = {POS.x,POS.y,POS.x,POS.y}, region_islev = true, dir="both" });
   des.finalize_level();
end

function ctrl(key)
   return string.char(0x1F & string.byte(key));
end

function meta(key)
   return string.char(0x80 | string.byte(key));
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
};


for k, v in pairs(basicmoves) do
   initlev();

   if (v.number_pad ~= nil) then
      if (v.number_pad ~= number_pad) then
         nh.parse_config("OPTIONS=number_pad:" .. v.number_pad);
         number_pad = v.number_pad;
      end
   end

   local x = u.ux;
   local y = u.uy;

   nh.pushkey(k);
   nh.doturn(true);

   if (v.dx ~= nil) then
      if (not (x == u.ux - v.dx and y == u.uy - v.dy)) then
         error(string.format("Move: key '%s' gave (%i,%i), should have been (%i,%i)",
                             k, u.ux, u.uy, u.ux - v.dx, u.uy - v.dy));
         return;
      end
   elseif (v.x ~= nil) then
      if (not (u.ux == v.x and u.uy == v.y)) then
         error(string.format("Move: key '%s' gave (%i,%i), should have been (%i,%i)",
                             k, u.ux, u.uy, v.x, v.y));
         return;
      end
   end
end

initlev();
