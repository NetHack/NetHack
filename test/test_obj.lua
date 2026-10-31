
local o = obj.new("rock");
o:placeobj(u.ux, u.uy);

local o2 = obj.at(u.ux, u.uy);
local o2tbl = o2:totable();
if (o2tbl.otyp_name ~= "rock") then
   error("no rock under you");
end


local box = obj.new("empty large box");
local boxtbl = box:totable();
if (boxtbl.has_contents ~= 0) then
   error("empty box has contents");
end

box:addcontent(obj.new("diamond"));
boxtbl = box:totable();
if (boxtbl.has_contents ~= 1) then
   error("box has no contents");
end

local diamond = box:contents():totable();
if (diamond.otyp_name ~= "diamond") then
   error("box contents is not a diamond");
end

box:placeobj(u.ux, u.uy);

local o3 = obj.at(u.ux, u.uy);
local o3tbl = o3:totable();
if (o3tbl.otyp_name ~= "large box") then
   error("no large box under you");
end

local o4 = o3:next();
local o4tbl = o4:totable();
if (o4tbl.otyp_name ~= "rock") then
   error("no rock under the large box");
end


local oc = obj.class(o4tbl.otyp);
if (oc.name ~= "rock") then
   error("object class is not rock, part 1");
end
if (oc.class ~= "*") then
   error("object class is not *, part 1");
end

local oc2 = obj.class(o);
if (oc2.name ~= "rock") then
   error("object class is not rock, part 2");
end
if (oc2.class ~= "*") then
   error("object class is not *, part 2");
end

local oc3 = obj.class(obj.new("rock"));
if (oc3.name ~= "rock") then
   error("object class is not rock, part 3");
end
if (oc3.class ~= "*") then
   error("object class is not *, part 3");
end

local oc4 = o:class();
if (oc4.name ~= "rock") then
   error("object class is not rock, part 4");
end
if (oc4.class ~= "*") then
   error("object class is not *, part 4");
end


-- placing obj into container even when obj is somewhere else already
local o5 = obj.new("dagger");
o5:placeobj(u.ux, u.uy);
box:addcontent(o5);


local o6 = obj.new("statue");
o6:addcontent(obj.new("spellbook"));


-- generate one of each object, check the name and class matches
for i = nhc.FIRST_OBJECT, nhc.LAST_OBJECT do
   local oid, oclass = nh.int_to_objname(i);
   if (oid ~= "") then
      local oi = obj.new({ id = oid, class = oclass });
      local oi_t = oi:totable();

      if (oi_t.otyp_name ~= oid) then
         error("object name \"" .. oi_t.otyp_name .. "\" created, wanted \"" .. oid .. "\"");
      end
      if (oi_t.oclass ~= oclass) then
         local str = string.format("object class \"%s\" created, wanted \"%s\" (%s)", oi_t.oclass, oclass, oid);
         error(str);
      end

   end
end


function test_use_item(action, itemname, otherkeys)
   nh.debug_flags({ prevent_pline = true });
   u.clear_inventory();
   u.giveobj(obj.new(itemname));
   local o = u.inventory;
   local ot = o:totable();

   nh.pushkey(action);
   nh.pushkey(ot.invlet);

   if (otherkeys ~= nil and type(otherkeys) == "string") then
      nh.pushkey(otherkeys);
   end

   nh.doturn();
   nh.debug_flags({ prevent_pline = false });
end

nh.parse_config("OPTIONS=number_pad:0");
nh.parse_config("OPTIONS=!timed_delay");

-- apply
test_use_item("a", "uncursed tin whistle");
test_use_item("a", "cursed tin whistle");
test_use_item("a", "blessed magic whistle");

test_use_item("a", "uncursed camera", "h");
test_use_item("a", "uncursed camera", "j");
test_use_item("a", "uncursed camera", "k");
test_use_item("a", "blessed camera", ">");
test_use_item("a", "+0 blessed camera", ">");

test_use_item("a", "blessed stethoscope", "h");
test_use_item("a", "blessed stethoscope", "j");
test_use_item("a", "blessed stethoscope", ".");
test_use_item("a", "blessed stethoscope", ">");
test_use_item("a", "blessed stethoscope", "<");
obj.new("corpse"):placeobj(u.ux, u.uy);
test_use_item("a", "blessed stethoscope", ">");
obj.new("statue"):placeobj(u.ux, u.uy);
test_use_item("a", "blessed stethoscope", ">");
