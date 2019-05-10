
-- s = string.format("(%i,%i),[%i,%i,%i]", u.ux, u.uy, u.dx, u.dy, u.dz);
-- nh.pline(s);


nh.test({ x = 3, y = 7, name="foo" });

local l = nh.getmap(u.ux, u.uy);

nh.pline(string.format("map(%i,%i): glyph=%i, typ=%i, lit=%i, candig=%i",
                       u.ux, u.uy,
                       l.glyph, l.typ, l.lit, l.candig));

-- nh.pline("lit: " .. x.lit .. ". ")

-- str = nh.getlin("What do you call it?");
-- nh.verbalize("Hello, this is " .. str .. " speaking");


-- nh.pline("COLNOxROWNO=(" .. nhc.COLNO .. "x" .. nhc.ROWNO .. ")")


-- local opts = { a = "valinta a", b = "bbbbb", c = "option c" }
-- local ret = nh.menu("Chooooooose one", "a", "one", opts)
-- nh.pline("You chose: '" .. ret .. "', that is: '" .. opts[ret] .. "'");


-- nh.pline("makeplural: " .. nh.makeplural("zorkmid"));
-- nh.pline("makesingular: " .. nh.makesingular("zorkmids"));


-- local opts = {
--   { key = "a", text = "valinta a" },
--   { key = "b", text = "bbbbb" },
--   { key = "c", text = "option c" }
-- }
--local ret = nh.menu("Choose one (1)", nil, nil, opts)
--nh.pline("You chose: '" .. ret .. "'")


--local ret = nh.menu("Choose one (2)", opts)
--nh.pline("You chose: '" .. ret .. "'")

--[[

nh.level_init({
      style = "none", -- or "fill", "grid", "mines", "rogue"
      filling = "tree",
      init_present = 1,
      padding = 1,
      fg = "floor",
      bg = "wall",
      smoothed = 0,
      joined = 1,
      lit = -1,
      walled = 0,
      icedpools = 0
});

nh.level_flags({"noteleport", "hardfloor"});

nh.message("What a strange feeling!");

nh.map({horiz = "center",
        vert = "center",
        map = "
.....
.L.L.
.....
" });

nh.monster({
      class = "V",
      typename = "vampire",
      coord = {5, 10},
      name = "Phil",
      attitude = "peaceful",
      alertness = "asleep",
      alignment = "lawful",
      -- appearance = ...?
      female = 1,
      invisible = 1,
      cancelled = 1,
      revived = 1,
      avenge = 1,
      fleeing = 23,
      blinded = 23,
      paralyzed = 23,
      stunned = 1,
      confused = 1,
      seentraps = { "pit", "magic trap" }, -- could be "all"
      -- how to add inventory for this monster?
      -- should be able to just directly set struct monst fields
});

nh.object({
});

-- ]]
