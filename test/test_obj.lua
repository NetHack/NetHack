
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
