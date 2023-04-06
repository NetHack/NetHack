
des.level_init({ style = "solidfill", fg = " " });
des.level_flags("mazelevel", "noflip",
                "nomongen", "nodeathdrops", "noautosearch");

des.map([[
---------------------------------------------------------------------------
|-.--|.......|......|..S....|.F.......|.............|.....................|
|.-..........|......|--|....|.F.....|.|S-------.....|.....................|
||.--|.......|..T......|....|.F.....|.|.......|.....|.....................|
||.|.|.......|......|-.|....|.F.....|.|.......|.....|.....................|
||.|.|.......|......||.|-.-----------.-.......|------.....................|
|-+-S---------..---.||........................|...........................|
|......|          |.-------------------.......|...........................|
|......|  ######  |.........|      |..S.......|...........................|
|----.-| -+-   #  |.....---.|######+..|.......S...........................|
|----+----.----+---.|.--|.|.|#     ------------...........................|
|........|.|......|.|...F...|#  ........|.....|...........................|
|.P......-S|......|------.---# .........|.....+...........................|
|..........|......+.|...|.|.S# ..--S-----.....|...........................|
|.W......---......|.|.|.|.|.|# ..|......|.....|...........................|
|....Z.L.S.F......|.|.|.|.---#   |......+.....|...........................|
|........|--......|...|.....|####+......|.....|...........................|
---------------------------------------------------------------------------
]]);


des.region(selection.area(01,01, 73, 16), "lit");

des.non_diggable();

des.teleport_region({ region = { 9,3, 9,3 } });

-- TODO:
--  - save (more of) hero state when entering
--  - quit-command should maybe exit the tutorial?

-- turn on some newbie-friendly options
nh.parse_config("OPTIONS=mention_walls");
nh.parse_config("OPTIONS=lit_corridor");

local movekeys = nh.eckey("movewest") .. " " ..
   nh.eckey("movesouth") .. " " ..
   nh.eckey("movenorth") .. " " ..
   nh.eckey("moveeast");

local diagmovekeys = nh.eckey("movesouthwest") .. " " ..
   nh.eckey("movenortheast") .. " " ..
   nh.eckey("movesoutheast") .. " " ..
   nh.eckey("movenorthwest");

des.engraving({ coord = { 9,3 }, type = "engrave", text = "Move around with " .. movekeys, degrade = false });
des.engraving({ coord = { 5,2 }, type = "engrave", text = "Move diagonally with " .. diagmovekeys, degrade = false });

--

des.engraving({ coord = { 2,4 }, type = "engrave", text = "Some actions may require multiple tries before succeeding", degrade = false });
des.engraving({ coord = { 2,5 }, type = "engrave", text = "Open the door by moving into it", degrade = false });
des.door({ coord = { 2,6 }, state = "closed" });

des.engraving({ coord = { 2,7 }, type = "engrave", text = "Close the door with '" .. nh.eckey("close") .. "'", degrade = false });


--

des.engraving({ coord = { 4,5 }, type = "engrave", text = "You can leave the tutorial via the magic portal.", degrade = false });
des.trap({ type = "magic portal", coord = { 4,4 }, seen = true });

--

des.engraving({ coord = { 5,9 }, type = "engrave", text = "This door is locked. Kick it with '" .. nh.eckey("kick") .. "'", degrade = false });
des.door({ coord = { 5,10 }, state = "locked" });

--

des.engraving({ coord = { 10,13 }, type = "engrave", text = "Use '" .. nh.eckey("search") .. "' to search for secret doors", degrade = false });

--

des.engraving({ coord = { 10,10 }, type = "engrave", text = "Behind this door is a dark corridor", degrade = false });
des.door({ coord = { 10,9 }, state = percent(50) and "locked" or "closed" });
des.region(selection.match("#"), "unlit");
des.region(selection.match(" "), "unlit");
des.door({ coord = { 15,10 }, state = percent(50) and "locked" or "closed" });

--

des.engraving({ coord = { 15,11 }, type = "engrave", text = "There are four traps next to you! Search for them.", degrade = false });
local locs = { {14,11}, {14,12}, {15,12}, {16,12}, {16,11} };
shuffle(locs);
for i = 1, 4 do
   des.trap({ type = percent(50) and "sleep gas" or "board",
              coord = locs[i], victim = false });
end

--

des.door({ coord = { 18,13 }, state = "closed" });

des.engraving({ coord = { 19,13 }, type = "engrave", text = "Pick up items with '" .. nh.eckey("pickup") .. "'", degrade = false });

local armor = (u.role == "Monk") and "leather gloves" or "leather armor";

des.object({ id = armor, spe = 0, buc = "cursed", coord = { 19,14} });

des.engraving({ coord = { 19,15 }, type = "engrave", text = "Wear armor with '" .. nh.eckey("wear") .. "'", degrade = false });

des.object({ id = "dagger", spe = 0, buc = "not-cursed", coord = { 21,15} });

des.engraving({ coord = { 21,14 }, type = "engrave", text = "Wield weapons with '" .. nh.eckey("wield") .. "'", degrade = false });


des.engraving({ coord = { 22,13 }, type = "engrave", text = "Hit monsters by walking into them.", degrade = false });

des.monster({ id = "lichen", coord = { 23,15 }, waiting = true, countbirth = false });

--

des.engraving({ coord = { 24,16 }, type = "engrave", text = "Now you know the very basics. You can leave the tutorial via the magic portal.", degrade = false });

des.engraving({ coord = { 26,16 }, type = "engrave", text = "Step into this portal to leave the tutorial", degrade = false });
des.trap({ type = "magic portal", coord = { 27,16 }, seen = true });

--

des.engraving({ coord = { 25,13 }, type = "engrave", text = "Push boulders by moving into them", degrade = false });
des.object({ id = "boulder", coord = {25,12} });

--

des.engraving({ coord = { 27,9 }, type = "engrave", text = "Take off armor with '" .. nh.eckey("takeoff") .. "'", degrade = false });

--

des.object({ class = "?", id = "remove curse", buc = "blessed", coord = {23,11} })
des.engraving({ coord = { 22,11 }, type = "engrave", text = "Some items have shuffled descriptions, different each game", degrade = false });
des.engraving({ coord = { 23,11 }, type = "engrave", text = "Pick up this scroll, read it with '" .. nh.eckey("read") .. "', and try to remove the armor again", degrade = false });

--

des.engraving({ coord = { 19,10 }, type = "engrave", text = "Another magic portal, a way to leave this tutorial", degrade = false });
des.trap({ type = "magic portal", coord = { 19,11 }, seen = true });

--

-- rock fall
des.object({ coord = {14, 5}, id = "rock", quantity = math.random(50,99) });
des.object({ coord = {15, 5}, id = "rock", quantity = math.random(10,30) });
des.object({ coord = {14, 4}, id = "rock", quantity = math.random(10,30) });
des.object({ coord = {15, 6}, id = "rock", quantity = math.random(30,60) });
des.object({ coord = {14, 6}, id = "rock", quantity = math.random(30,60) });
des.object({ coord = {14, 6}, id = "boulder" });

des.door({ coord = { 20,3 }, state = percent(50) and "open" or "closed" });

des.engraving({ coord = { 21,3 }, type = "engrave", text = "Avoid being burdened, it slows you down", degrade = false });
des.engraving({ coord = { 22,3 }, type = "engrave", text = "Drop items with '" .. nh.eckey("drop") .. "'", degrade = false });
des.engraving({ coord = { 22,4 }, type = "engrave", text = "You can drop partial stacks by prefixing the item slot letter with a number", degrade = false });

--

des.monster({ id = "yellow mold", coord = { 26,2 }, waiting = true, countbirth = false });

des.engraving({ coord = { 25,5 }, type = "engrave", text = "Throw items with '" .. nh.eckey("throw") .. "'", degrade = false });

des.trap({ type = "magic portal", coord = { 21,1 }, seen = true });

--

des.monster({ id = "wolf", coord = { 29,2 }, peaceful = 0, waiting = true, countbirth = false });

des.engraving({ coord = { 37,4 }, type = "engrave", text = "Missiles, such as rocks, work better when fired from appropriate launcher", degrade = false });

des.object({ coord = { 37,3 }, id = "sling", buc = "not-cursed", spe = 9 });
des.engraving({ coord = { 37,3 }, type = "engrave", text = "Wield the sling", degrade = false });
des.engraving({ coord = { 36,1 }, type = "engrave", text = "Use '" .. nh.eckey("fire") .. "' to fire missiles with the wielded launcher", degrade = false });

des.engraving({ coord = { 35,4 }, type = "engrave", text = "Firing launches items from your quiver; Use '" .. nh.eckey("quiver") .. "' to put items in it", degrade = false });

des.engraving({ coord = { 33,4 }, type = "engrave", text = "You can wait a turn with '" .. nh.eckey("wait") .. "'", degrade = false });


--

des.door({ coord = { 38,6 }, state = "closed" });

des.engraving({ coord = { 39,6 }, type = "engrave", text = "You loot containers with '" .. nh.eckey("loot") .. "'", degrade = false });

des.object({ coord = { 42,6 }, id = "large box", broken = true,
             contents = function(obj)
                des.object({ id = "secret door detection", class = "/", spe = 30 }); end
});

des.engraving({ coord = { 45,6 }, type = "engrave", text = "Magic wands are used with '" .. nh.eckey("zap") .. "'", degrade = false });

--

des.engraving({ coord = { 36,9 }, type = "engrave", text = "You can run by prefixing a movement key with '" .. nh.eckey("run") .. "'", degrade = false });

--

des.trap({ type = "magic portal", coord = { 27,14 }, seen = true });

--

des.engraving({ coord = { 48,1 }, type = "burn", text = "Use '" .. nh.eckey("eat") .. "' to eat edible things", degrade = false });

des.object({ coord = { 50,3 }, id = "apple", buc = "not-cursed"  });
des.object({ coord = { 50,3 }, id = "candy bar", buc = "not-cursed"  });

local otmp = des.object({ coord = { 50,3 }, id = "corpse", montype = "newt", buc = "not-cursed" });
otmp:stop_timer("rot-corpse");

--

des.door({ coord = { 46,12 }, state = "random" });

des.engraving({ coord = { 43,11 }, type = "burn", text = "Use '" .. nh.eckey("twoweapon") .. "' to use two weapons at once", degrade = false });
des.object({ coord = { 43,13 }, id = "knife", buc = "uncursed" });
des.object({ coord = { 43,14 }, id = "dagger", buc = "blessed" });

des.engraving({ coord = { 43,16 }, type = "burn", text = "Swap weapons quickly with '" .. nh.eckey("swap") .. "'", degrade = false });

des.door({ coord = { 40,15 }, state = "random" });

--

des.engraving({ coord = { 55,9 }, type = "burn", text = "UNDER CONSTRUCTION", degrade = false });

des.trap({ type = "magic portal", coord = { 60,9 }, seen = true });

----------------

nh.callback("cmd_before", "tutorial_cmd_before");
nh.callback("level_enter", "tutorial_enter");
nh.callback("level_leave", "tutorial_leave");
nh.callback("end_turn", "tutorial_turn");

----------------

-- temporary stuff here
-- des.trap({ type = "magic portal", coord = { 9,5 }, seen = true });
-- des.trap({ type = "magic portal", coord = { 9,1 }, seen = true });
-- des.object({ id = "leather armor", spe = 0, coord = { 9,2} });

