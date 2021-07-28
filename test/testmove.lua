
-- Tests for moving the hero

-- TODO: running stops if hero walks over stairs -> test fails.
--       prevent stair generation? check where the stairs are?

nh.parse_config("OPTIONS=number_pad:0");
nh.parse_config("OPTIONS=runmode:teleport");

local POS = { x = 10, y = 05 };

function initlev()
   nh.monster_generation(false);
   des.level_flags("noflip");
   des.reset_level();
   des.level_init({ style = "solidfill", fg = ".", lit = true });
   des.teleport_region({ region = {POS.x,POS.y,POS.x,POS.y}, region_islev = true, dir="both" });
   des.finalize_level();
end

local basicmoves = {
   h = { dx = -1,  dy =  0 },
   j = { dx =  0,  dy =  1 },
   k = { dx =  0,  dy = -1 },
   l = { dx =  1,  dy =  0 },
   y = { dx = -1,  dy = -1 },
   u = { dx =  1,  dy = -1 },
   b = { dx = -1,  dy =  1 },
   n = { dx =  1,  dy =  1 },
   H = { x = 2,  y = POS.y },
   J = { x = POS.x, y = nhc.ROWNO-1 },
   K = { x = POS.x, y = 0 },
   L = { x = nhc.COLNO-2, y = POS.y },
   Y = { x = POS.x - POS.y, y = 0 },
   U = { x = POS.x + POS.y, y = 0 },
   B = { x = 2, y = 13 },
   N = { x = 25, y = nhc.ROWNO-1 },
};


for k, v in pairs(basicmoves) do
   initlev();

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
