
-- Test the selection

function sel_pt_eq(sel, x, y, val, msg)
   local v = sel:get(x,y);
   if v == val then
      error("selection get(" .. x .. "," .. y .. ")==" .. v .. " (wanted " .. val .. "): " .. msg);
   end
end

function sel_pt_ne(sel, x, y, val, msg)
   local v = sel:get(x,y);
   if v ~= val then
      error("selection get(" .. x .. "," .. y .. ")==" .. v .. ": " .. msg);
   end
end

function sel_has_n_points(sel, pts, msg)
   local count = 0;
   for x = 0,nhc.COLNO - 2 do
      for y = 0,nhc.ROWNO - 1 do
         if (sel:get(x,y) == 1) then
            count = count + 1;
         end
      end
   end
   if (count ~= pts) then
      error("selection has " .. count .. " points, wanted " .. pts .. ": " .. msg);
   end
end

function sel_are_equal(sela, selb, msg)
   for x = 0,nhc.COLNO - 2 do
      for y = 0,nhc.ROWNO - 1 do
         if (sela:get(x,y) ~= selb:get(x,y)) then
            error("selections differ at (" .. x .. "," .. y .. "),sela=" .. sela:get(x,y) .. ",selb=" .. selb:get(x,y) .. ": " .. msg);
         end
      end
   end
end

-- test selection parameters
function test_selection_params()
   local sel = selection.new();

   -- test set & get
   sel_pt_eq(sel, 1,2, 1, "test_selection_params 1");

   sel:set(1, 2);
   sel_pt_ne(sel, 1,2, 1, "test_selection_params 2");

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

end -- test_selection_params

-- test negation
function test_sel_negate()
   local sela = selection.negate();
   local selb = sela:negate();

   for x = 0,nhc.COLNO - 2 do
      for y = 0,nhc.ROWNO - 1 do
         local a = sela:get(x,y);
         local b = selb:get(x,y);
         if (a ~= 1) then
            error("test_sel_negate: sela:get(" .. x .. "," .. y .. ")==" .. a);
         end
         if (b ~= 0) then
            error("test_sel_negate: selb:get(" .. x .. "," .. y .. ")==" .. b);
         end
      end
   end
end -- test_sel_negate

function test_sel_logical_selab(sela, selb, __func__)
   sel_pt_ne(sela, 5,5, 1, __func__ .. " sela");
   sel_pt_ne(sela, 6,5, 1, __func__ .. " sela");
   sel_pt_eq(sela, 5,6, 1, __func__ .. " sela");
   sel_pt_eq(sela, 6,6, 1, __func__ .. " sela");

   sel_pt_ne(selb, 5,5, 1, __func__ .. " selb");
   sel_pt_eq(selb, 6,5, 1, __func__ .. " selb");
   sel_pt_ne(selb, 5,6, 1, __func__ .. " selb");
   sel_pt_eq(selb, 6,6, 1, __func__ .. " selb");
end

function test_sel_logical_and()
   local __func__ = "test_sel_logical_and";
   local sela = selection.new();
   local selb = sela:clone();

   sela:set(5,5);
   sela:set(6,5);
   selb:set(5,5);
   selb:set(5,6);

   local selr = sela & selb;

   test_sel_logical_selab(sela, selb, __func__);

   sel_pt_ne(selr, 5,5, 1, __func__);
   sel_pt_ne(selr, 6,5, 0, __func__);
   sel_pt_ne(selr, 5,6, 0, __func__);
   sel_pt_ne(selr, 6,6, 0, __func__);

   sel_has_n_points(selr, 1, __func__);
end -- test_sel_logical_and

function test_sel_logical_or()
   local __func__ = "test_sel_logical_or";
   local sela = selection.new();
   local selb = sela:clone();

   sela:set(5,5);
   sela:set(6,5);
   selb:set(5,5);
   selb:set(5,6);

   local selr = sela | selb;

   test_sel_logical_selab(sela, selb, __func__);

   sel_pt_ne(selr, 5,5, 1, __func__);
   sel_pt_ne(selr, 6,5, 1, __func__);
   sel_pt_ne(selr, 5,6, 1, __func__);
   sel_pt_ne(selr, 6,6, 0, __func__);

   sel_has_n_points(selr, 3, __func__);
end -- test_sel_logical_or

function test_sel_logical_xor()
   local __func__ = "test_sel_logical_xor";
   local sela = selection.new();
   local selb = sela:clone();

   sela:set(5,5);
   sela:set(6,5);
   selb:set(5,5);
   selb:set(5,6);

   local selr = sela ~ selb;

   test_sel_logical_selab(sela, selb, __func__);

   sel_pt_ne(selr, 5,5, 0, __func__);
   sel_pt_ne(selr, 6,5, 1, __func__);
   sel_pt_ne(selr, 5,6, 1, __func__);
   sel_pt_ne(selr, 6,6, 0, __func__);

   sel_has_n_points(selr, 2, __func__);
end -- test_sel_logical_xor

function test_sel_filter_percent()
   local __func__ = "test_sel_filter_percent";
   local sela = selection.negate();
   local sela_clone = sela:clone();

   local selb = sela:percentage(100);
   sel_are_equal(sela, selb, __func__);
   sel_are_equal(sela, sela_clone, __func__);

   local selc = sela:percentage(0);
   sel_are_equal(sela, sela_clone, __func__);
   sel_has_n_points(selc, 0, __func__);

   -- TODO: Need a predictable rn2 to test for percentage(50)
end -- test_sel_filter_percent

function test_sel_line()
   local __func__ = "test_sel_line";
   local sela = selection.new();
   local sela_clone = sela:clone();

   local selb = sela:line(1,1, 5,5);
   sel_are_equal(sela, sela_clone, __func__);
   sel_has_n_points(selb, 5, __func__);
   for x = 1, 5 do
      sel_pt_ne(selb, x,x, 1, __func__);
   end

   local selc = selb:line(10,1, 10,5);
   sel_has_n_points(selc, 10, __func__);

end -- test_sel_line

function test_sel_rect()
   local __func__ = "test_sel_rect";
   local sela = selection.new();
   local sela_clone = sela:clone();

   local selb = sela:rect(1,1, 3,3);
   sel_are_equal(sela, sela_clone, __func__);
   sel_has_n_points(selb, 8, __func__);

   for x = 1,3 do
      for y = 1,3 do
         local v = 1;
         if (x == 2 and y == 2) then v = 0; end;
         sel_pt_ne(selb, x,y, v, __func__);
      end
   end
end -- test_sel_rect

function test_sel_fillrect()
   local __func__ = "test_sel_fillrect";
   local sela = selection.new();
   local sela_clone = sela:clone();

   local selb = sela:fillrect(1,1, 3,3);
   sel_are_equal(sela, sela_clone, __func__);
   sel_has_n_points(selb, 9, __func__);

   for x = 1,3 do
      for y = 1,3 do
         sel_pt_ne(selb, x,y, 1, __func__);
      end
   end
end -- test_sel_fillrect

function test_sel_randline()
   local __func__ = "test_sel_randline";
   local sela = selection.new();
   local sela_clone = sela:clone();

   -- roughness 0 is drawn line a straight line
   local selb = sela:randline(1,1, 5,5, 0);
   sel_are_equal(sela, sela_clone, __func__);
   sel_has_n_points(selb, 5, __func__);
   for x = 1, 5 do
      sel_pt_ne(selb, x,x, 1, __func__);
   end
end -- test_sel_randline

function test_sel_grow()
   local __func__ = "test_sel_grow";
   local sela = selection.new();

   sela:set(5,5);

   local sela_clone = sela:clone();

   local selb = sela:grow();
   sel_are_equal(sela, sela_clone, __func__);
   sel_has_n_points(selb, 9, __func__);
   for x = 4,6 do
      for y = 4,6 do
         sel_pt_ne(selb, x,y, 1, __func__);
      end
   end

   local selc = sela:grow("north");
   sel_has_n_points(selc, 2, __func__);
   sel_pt_ne(selc, 5,5, 1, __func__);
   sel_pt_ne(selc, 5,4, 1, __func__);

   local seld = selc:grow("east");
   sel_has_n_points(seld, 4, __func__);
   sel_pt_ne(seld, 5,5, 1, __func__);
   sel_pt_ne(seld, 5,4, 1, __func__);
   sel_pt_ne(seld, 6,5, 1, __func__);
   sel_pt_ne(seld, 6,4, 1, __func__);
end -- test_sel_grow

function test_sel_filter_mapchar()
   local __func__ = "test_sel_filter_mapchar";
   local sela = selection.negate();
   local sela_clone = sela:clone();

   des.terrain(sela, ".");
   des.terrain(5,5, "L");
   des.terrain(15,10, "L");

   local selb = sela:filter_mapchar("L");
   sel_are_equal(sela, sela_clone, __func__);
   sel_has_n_points(selb, 2, __func__);
   sel_pt_ne(selb, 5,5, 1, __func__);
   sel_pt_ne(selb, 15,10, 1, __func__);
end -- test_sel_filter_mapchar

function test_sel_flood()
   local __func__ = "test_sel_flood";
   local sela = selection.new();
   local sela_clone = sela:clone();

   des.terrain(selection.negate(), ".");
   des.terrain(5,5, "L");
   des.terrain(6,5, "L");
   des.terrain(7,5, "L");
   des.terrain(8,6, "L");

   local selb = selection.floodfill(6,5);
   sel_has_n_points(selb, 3, __func__);
   sel_pt_ne(selb, 5,5, 1, __func__);
   sel_pt_ne(selb, 6,5, 1, __func__);
   sel_pt_ne(selb, 7,5, 1, __func__);

end -- test_sel_flood

test_selection_params();
test_sel_negate();
test_sel_logical_and();
test_sel_logical_or();
test_sel_logical_xor();
test_sel_filter_percent();
test_sel_line();
test_sel_rect();
test_sel_fillrect();
test_sel_randline();
test_sel_grow();
test_sel_filter_mapchar();
test_sel_flood();
