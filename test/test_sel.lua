

-- Test the selection

function test_selection()
   local sel = selection.new();

   -- test set & get
   local ret = sel:get(1,2)
   if ret == 1 then
      error("sel:get returned " .. ret .. " instead of 0");
   end

   sel:set(1, 2);
   ret = sel:get(1, 2);
   if ret ~= 1 then
      error("sel:get returned " .. ret .. " instead of 1");
   end

   local x,y = sel:rndcoord(1);
   if x ~= 1 or y ~= 2 then
      error("sel:rndcoord returned unset coordinate");
   end

   x,y = sel:rndcoord(1);
   if x ~= -2 and y ~= -1 then
      error("sel:rndcoord returned (" .. x .. "," .. y .. ") coordinate");
   end

   -- OO style
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

   local sel2 = selection.clone(sel);
   local sel3 = sel2:clone();
end -- selection_tests()


test_selection();
