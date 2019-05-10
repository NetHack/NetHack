
-- run tests on the des-level commands

-- des.level_init({ style = "mines", joined = 1, lit = 1 });
-- des.level_init({ style = "mazegrid", lit = 1 });
-- des.level_init({ style = "solidfill" });
-- des.level_init({ style = "rogue" });
des.level_init();

function selection_tests ()
   -- OO style
   local sel = selection.new();
   sel:get(1, 2);
   sel:set(1, 2);
   sel:negate();
   sel:percentage(50);
   sel:rndcoord(1);
   sel:line(1,2, 50,20);
   sel:randline(1,2, 50,20, 7);
   sel:rect(1,2, 7,8);
   sel:fillrect(1,2, 7,8);
   sel:area(1,2, 7,8);
   sel:grow();
   sel:filter_mapchar(' ');
   sel:floodfill(1,1);
   sel:circle(40, 10, 9);
   sel:circle(40, 10, 9, 1);
   sel:ellipse(40, 10, 20, 8);
   sel:ellipse(40, 10, 20, 8, 1);

   -- initializers
   selection.set(1, 2);
   selection.negate();
   selection.line(1,2, 50,20);
   selection.randline(1,2, 50,20, 7);
   selection.rect(1,2, 7,8);
   selection.fillrect(1,2, 7,8);
   selection.area(1,2, 7,8);
   selection.floodfill(1,1);
   selection.circle(40, 10, 9);
   selection.circle(40, 10, 9, 1);
   selection.ellipse(40, 10, 20, 8);
   selection.ellipse(40, 10, 20, 8, 1);

   -- variable as param
   selection.get(sel, 1, 2);
   selection.set(sel, 1, 2);
   selection.negate(sel);
   selection.percentage(sel, 50);
   selection.rndcoord(sel, 1);
   selection.line(sel, 1,2, 50,20);
   selection.randline(sel, 1,2, 50,20, 7);
   selection.rect(sel, 1,2, 7,8);
   selection.fillrect(sel, 1,2, 7,8);
   selection.area(sel, 1,2, 7,8);
   selection.grow(sel);
   selection.filter_mapchar(sel, ' ');
   selection.floodfill(sel, 1,1);
   selection.circle(sel, 40, 10, 9);
   selection.circle(sel, 40, 10, 9, 1);
   selection.ellipse(sel, 40, 10, 20, 8);
   selection.ellipse(sel, 40, 10, 20, 8, 1);
end -- selection_tests()

-- set coordinate 4,5 to value 1
-- selection.set(sel, 4, 5);

-- set coordinate 7,7 to value 1
-- sel:set(7, 7);

-- set 5,5 to value 2
-- sel:set(5,5, 2);

-- get 3 coordinates, removing the coordinates from the selection
-- local tx1, ty1 = selection.rndcoord(sel, 1);
-- local tx2, ty2 = selection.rndcoord(sel, 1);
-- local tx3, ty3 = selection.rndcoord(sel, 1);

-- filter randomly 50% of the selection
-- local sel2 = selection.percentage(sel, 50);

-- negate the selection
-- local sel3 = selection.negate(sel);

-- negate first, then 50%
-- local sel4 = selection.percentage(selection.negate(sel), 50);

-- local yyy = sel:get(6, 7);

-- des.mazewalk({ x = 4, y = 6, stocked = 0 });

-- draw a line from (1,1) to (40,15)
-- selection.line(sel, 1,1, 40,15);

-- draw a rectangle with upper left at (1,1) to lower right at (10,10)
-- selection.rect(sel, 1,1, 10,10);

-- draw a filled rectangle with upper left at (1,1) to lower right at (10,10)
-- selection.fillrect(sel, 40,2, 50,5);

-- alias to fillrect
-- selection.area(sel, 40,2, 50,5);

-- randomized line from (73,1) to (1,20), with roughness 7
-- selection.randline(sel, 73, 1, 1, 20, 7);

-- grow selection to all directions
-- selection.grow(sel);

-- floodfill from (2,7), matching the same terrain type
-- sel:floodfill(2, 5);

-- draw a circle at (40,10) with radius 9
-- selection.circle(sel, 40, 10, 9);

-- draw a filled circle at (40,10) with radius 9
-- selection.circle(sel, 40, 10, 9, 1);

-- draw an ellipse at (40,10) with radii 20 and 8
-- selection.ellipse(sel, 40, 10, 20, 8);

-- draw a filled ellipse at (40,10) with radii 20 and 8
-- sel:ellipse(40, 10, 20, 8, 1);

-- filter selection by matching terrain
-- selection.filter_mapchar(sel, ' ');

-- set lava in the selection
local sel = selection.area(4,5, 40,10) ~ selection.rect(7,8, 60,14);
des.terrain(sel, "L");

-- des.terrain({ x = tx1, y = ty1, typ = "L" });
-- des.terrain({ x = tx2, y = ty2, typ = "T" });
-- des.terrain({ x = tx3, y = ty3, typ = "F" });

-- des.wall_property({ property = "nondiggable", x1 = 0, y1 = 0, x2 = 40, y2 = 20 });
-- des.wall_property();

des.wallify();



function test ()
rectroom = [[
----
|..|
|...
|..|
----]];

des.map({
      -- halign = "center", valign = "center",
      x = 5, y = 5,
      keepregion = 1,
      map = [[
------
|.....
|....|
|....|
.....|
|....|
|....|
------]]});

--[[
for i = 0, 5 do
  xp = math.random(70);
  yp = math.random(20);
  -- des.message("i=" .. i .. " (" .. xp .. "," .. yp .. ")");
  -- des.map({ x = xp, y = yp , map = rectroom, keepregion = 1 });
end
]]

-- des.wallify();

-- des.message("viesti");


for i = 1, 10 do
  mx = i;
  my = i;
  des.message(i .."=(" .. mx .. "," .. my .. ")");
  des.monster({ x = mx, y = my,
                name = "foo" .. mx .. "x" .. my,
                -- class = "F",
                id = "red mold",
                peaceful = 1,
                asleep = 1,
                female = 1
  });
  des.object({ x = mx + 1, y = my,
               -- class = "=",
               id = "silver dagger",
               buc = "blessed",
               quantity = 1,
               spe = "random"
  });
end

for i = 1, 1000 do
  des.object({ x = 40, y = 10,
               -- class = "=",
               id = "silver dagger",
               buc = "blessed",
               quantity = 1,
               -- spe = "random"
  });
end

end -- function test()
