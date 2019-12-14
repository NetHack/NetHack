
-- Test the des-file commands
-- reset_level is only needed here, not in normal special level scripts.

function is_map_at(x,y, mapch, lit)
   local rm = nh.getmap(x + 1, y); -- + 1 == g.xstart
   if rm.mapchr ~= mapch then
      error("Terrain at (" .. x .. "," .. y .. ") is not \"" .. mapch .. "\", but \"" .. rm.mapchr .. "\"");
   end
   if lit ~= nil then
      if rm.lit ~= lit then
         error("light state at (" .. x .. "," .. y .. ") is not \"" .. lit .. "\", but \"" .. tostring(rm.lit) .. "\"");
      end
   end
end

function test_level_init()
   des.reset_level();
   des.level_init();

   des.reset_level();
   des.level_init({ style = "solidfill" });

   des.reset_level();
   des.level_init({ style = "solidfill", fg = " " });

   des.reset_level();
   des.level_init({ style = "mazegrid", bg ="-" });

   des.reset_level();
   des.level_init({ style = "rogue" });

   des.reset_level();
   des.level_init({ style = "mines" });

   des.reset_level();
   des.level_init({ style = "mines", fg = ".", bg = " ", smoothed = true });

   des.reset_level();
   des.level_init({ style = "mines", fg = ".", bg = "}", joined = true });

   des.reset_level();
   des.level_init({ style = "mines", fg = ".", bg = "L", smoothed = 1, joined = 1, lit = 0 });

   des.reset_level();
   des.level_init({ style = "mines", fg = ".", bg = " ", smoothed = true, joined = true, walled = true });

   des.reset_level();
   des.level_init({ style = "solidfill", fg = ".", lit = 1 });
end

function test_message()
   des.message("Test message");
   des.message("Message 2");
end

function test_monster()
   des.monster();
   des.monster("gnome")
   des.monster("S")
   des.monster("giant eel",11,06);
   des.monster("hill giant", {08,06});
   des.monster({ id = "ogre" })
   des.monster({ class = "D" })
   des.monster({ id = "ogre", x = 10, y = 15 })
   des.monster({ class = "D", coord = {11,16} })
   des.monster({ x = 73, y = 16 });
   des.monster({ id = "watchman", peaceful = 1 })
   des.monster({ class = "H", peaceful = 0 })
   des.monster({ id = "giant mimic", appear_as = "obj:boulder" });
   des.monster({ id = "giant mimic", appear_as = "ter:altar" });
   des.monster({ id = "chameleon", appear_as = "mon:bat" });
   des.monster({ class = "H", asleep = 1, female = 1, invisible = 1, cancelled = 1, revived = 1, avenge = 1, fleeing = 20, blinded = 20, paralyzed = 20, stunned = 20, confused = 20 })
   des.monster({ id = "ogre", x = 10, y = 15, name = "Fred",
                 inventory = function()
                   des.object();
                   des.object("[");
                   des.object({ class = "/" });
                   des.object({ id = "statue", contents=0 })
                 end
   });
   des.reset_level();
   des.level_init();
end

function test_object()
   des.reset_level();
   des.level_init();
   des.object()
   des.object("*")
   des.object({ class = "%" });
   des.object({ id = "statue", contents=0 })
   des.object("sack")
   des.object({ x = 41, y = 03 })
   des.object({ coord = {42, 03} });
   des.object({ id = "boulder", coord = {03,12} });
   des.object("diamond", 69, 04)
   des.object("diamond", {68, 04})
   des.object({ id = "egg", x = 05, y = 04, montype = "yellow dragon" });
   des.object({ id = "egg", x = 06, y = 04, montype = "yellow dragon", laid_by_you = true });
   des.object({ id = "corpse", montype = "valkyrie" })
   des.object({ id = "statue", montype = "C", historic = true, male = true });
   des.object({ id = "statue", montype = "C", historic = true, female = true });
   des.object({ id = "chest", buried = true, locked = true,
                contents = function()
                   des.object();
                   des.object("*")
                   des.object({ class = "%" });
                   des.object({ id = "statue", contents=0 })
                end
   });
   des.object({ id = "chest", greased = true, broken = true, contents = 0 });
   des.object({ id = "chest", trapped = 1, broken = true, contents = 0 });
   des.object({ id = "oil lamp", lit = 1 });
   des.object({ id = "silver dagger", spe = 127, buc = "cursed" });
   des.object({ id = "silver dagger", spe = -127, buc = "blessed" });
   des.object({ id = "bamboo arrow", quantity = 100 });
   des.object({ id = "leather armor", eroded = 1 });
   des.object({ id = "probing", recharged = 2, spe = 3 });
   des.object({ name = "Random object" });
   des.object({ class = "*", name = "Random stone" });
   des.object({ id ="broadsword", name = "Dragonbane" })
   des.reset_level();
   des.level_init();
end

function test_level_flags()
   des.level_flags("noteleport")
   des.level_flags("noteleport", "hardfloor", "nommap", "shortsighted", "arboreal")
   des.level_flags("mazelevel", "shroud", "graveyard", "icedpools", "corrmaze")
   des.level_flags("premapped", "solidify", "inaccessibles")
end

function test_engraving()
   des.engraving({02,04},"engrave","Trespassers will be persecuted!")
   des.engraving({ x = 1, y = 2, type = "burn", text = "Elbereth" });
   des.engraving({ type = "dust", text = "X marks the spot." })
   des.engraving({ text = "Foobar" })
   des.engraving({ type = "mark", text = "X" })
   des.engraving({ type = "blood", text = "redrum" })
end

function test_mineralize()
   des.mineralize();
   des.mineralize({ gem_prob = 1, gold_prob = 10, kelp_moat = 25, kelp_pool = 100 })
end

function test_grave()
   des.grave();
   des.grave({ text = "Lil Miss Marker" });
   des.grave({ x = 40, y = 11 });
   des.grave({ x = 41, y = 12, text = "Bongo" });
   des.grave({ x = 42, y = 13, text = "" });
end

function test_altar()
   des.altar();
   des.altar({ x = 44, y = 20 });
   des.altar({ coord = {46, 20 } });
   des.altar({ coord = {48, 20 }, type = "altar", align = "law" });
   des.altar({ coord = {50, 20 }, type = "shrine", align = "noalign" });
   des.altar({ coord = {52, 20 }, type = "sanctum", align = "coaligned" });
end

function test_map()
   des.map([[
TTT
LTL
LTL]])
   des.map({ x = 60, y = 5, map = [[
FFF
F.F
FFF]] })
   des.map({ halign = "left", valign = "bottom", map = [[
III
.I.
III]] })
end

function test_feature()
   des.feature("fountain", 40, 08);
   des.feature("sink", {41, 08});
   des.feature({ type = "pool", x = 42, y = 08 });
end

function test_gold()
   des.gold({ amount = 999, x = 40, y = 07 });
end

function test_trap()
   des.trap("pit", 41, 06);
   des.trap("level teleport", {42, 06});
   des.trap({ type = "hole", x = 43, y = 06 });
   des.trap({ type = "hole", coord = {44, 06} });
   des.trap();
   des.trap("rust");
end

function test_wall_prop()
   des.wall_property({ x1 = 0, y1 = 0, x2 = 78, y2 = 20, property = "nondiggable" });
   des.wall_property({ region={0,0, 78,20}, property = "nondiggable" });
   des.non_diggable();
   des.non_diggable(selection.area(5,5, 15, 15));
   des.non_passwall();
   des.non_passwall(selection.area(5,5, 15, 15));
end

function test_wallify()
   des.wallify();
   des.wallify({ x1 = 0, y1 = 0, x2 = 78, y2 = 20 });
end

function test_teleport_region()
   des.teleport_region({ region = {69,00,79,20} })
   des.teleport_region({ region = {69,00,79,20}, dir="up" })
   des.teleport_region({ region = {69,00,79,20}, region_islev=1, dir="up" })
   des.teleport_region({ region = {01,00,10,20}, region_islev=1, exclude={1,1,61,15}, dir="down" })
   des.teleport_region({ region = {01,00,10,20}, region_islev=1, exclude={1,1,61,15}, exclude_islev=1 })
end

function test_region()
   des.region(selection.area(08,03,54,03),"unlit")
   des.region(selection.area(56,02,60,03),"lit")
   des.region({ region={16,05, 25,06}, lit=1, type="barracks", prefilled=0 })
   des.region({ region={1,5, 3,7}, lit=1, irregular=true, prefilled=true, joined=false })
end

function test_door()
   des.door("nodoor", 12,12);
   des.door({ x = 13, y = 12, state = "open" });
   des.room({ type = "graveyard", contents = function()
                 des.door({ wall = "north", pos = 1 });
                 des.door({ wall = "random", state = "locked" });
                         end
   });
end

function test_mazewalk()
   des.reset_level();
   des.level_init({ style = "mazegrid", bg ="-" });
   des.mazewalk(01,10,"east")

   des.reset_level();
   des.level_init({ style = "mazegrid", bg ="-" });
   des.mazewalk({ x=2,y=10, dir="north", typ="L", stocked=true });
end

function test_room()
   des.reset_level();
   des.level_init({ style = "solidfill", fg=" " });
   des.room({ type = " ordinary", lit = 1,
              x=3, y=3, xalign="center", yalign="center",
              w=11, h=9, contents = function()
                 des.room({ x=4, y=3, w=3,h=3 });
              end
   });
   des.room();
   des.room({ contents = function()
                 des.object();
                 des.monster();
                         end
   });
   des.random_corridors();
end

function test_stair()
   des.reset_level();
   des.level_init();

   des.stair("up");
   des.stair("down", 4, 7);
   des.stair({ dir = "down", x = 5,  y = 7 });
   des.stair({ dir = "down", coord = {6, 7} });
end

function test_ladder()
   des.reset_level();
   des.level_init();

   des.ladder("up");
   des.ladder("down", 4, 7);
   des.ladder({ dir = "down", x = 5,  y = 7 });
   des.ladder({ dir = "down", coord = {6, 7} });
end

function test_terrain()
   des.reset_level();
   des.level_init();

   des.terrain(2, 2, "L");
   is_map_at(2,2, "L");

   des.terrain({6,7}, "L");
   is_map_at(6,7, "L");

   des.terrain({ x = 5, y = 5, typ = "L" });
   is_map_at(5,5, "L");

   -- TODO: allow lit = false
   -- des.terrain({ x = 5, y = 5, typ = ".", lit = false });
   -- is_map_at(5,5, ".", false);

   des.terrain({ x = 5, y = 5, typ = ".", lit = 1 });
   is_map_at(5,5, ".", true);

   des.terrain({ x = 5, y = 5, typ = " ", lit = 0 });
   is_map_at(5,5, " ", false);

   des.terrain({ selection = selection.area(4,4, 6,6), typ = "L", lit = 0 });
   for x = 4,6 do
      for y = 4,6 do
         is_map_at(x,y, "L", true);
      end
   end

   des.terrain(selection.area(2,2, 4,4), "T");
   for x = 2,4 do
      for y = 2,4 do
         is_map_at(x,y, "T");
      end
   end
end

function test_replace_terrain()
   des.replace_terrain({ x1=1, y1=1, x2=70,y2=19, fromterrain=".", toterrain="I", lit=1 });
   des.replace_terrain({ x1=1, y1=1, x2=70,y2=19, fromterrain=".", toterrain="I", chance=50 });
   des.replace_terrain({ region={1,1, 70,19}, fromterrain=".", toterrain="L", chance=25 });
end

function test_corridor()
   des.reset_level();
   des.level_init({ style = "solidfill", fg=" " });
   des.room({ x=2, y=2, xalign="center", yalign="center", w=4, h=4,
              contents = function()
                 des.door({ wall = "south", pos = 2 });
                         end
   });
   des.room({ x=1, y=3, xalign="center", yalign="center", w=6, h=6,
              contents = function()
                 des.door({ wall = "north", pos = 1 });
                         end
   });
   des.corridor({ srcroom=0, srcwall="south", srcdoor=0, destroom=1, destwall="north", destdoor=0 });
end

function run_tests()
   test_level_init();
   test_message();
   test_monster();
   test_object();
   test_level_flags();
   test_engraving();
   test_mineralize();
   test_grave();
   test_altar();
   test_feature();
   test_gold();
   test_trap();
   test_door();
   test_map();
   test_wall_prop();
   test_wallify();
   test_teleport_region();
   test_region();
   test_mazewalk();
   test_room();
   test_stair();
   test_ladder();
   test_terrain();
   test_replace_terrain();
   test_corridor();

   des.reset_level();
   des.level_init();
end

run_tests();
