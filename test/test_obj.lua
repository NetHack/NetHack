
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
