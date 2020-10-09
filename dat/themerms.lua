
-- themerooms is an array of tables and/or functions.
-- the tables define "frequency", "contents", "mindiff" and "maxdiff".
-- frequency is optional; if omitted, 1 is assumed.
-- mindiff and maxdiff are optional and independent; if omitted, the room is
-- not constrained by level difficulty.
-- a plain function has frequency of 1, and no difficulty constraints.
-- des.room({ type = "ordinary", filled = 1 })
--   - ordinary rooms can be converted to shops or any other special rooms.
--   - filled = 1 means the room gets random room contents, even if it
--     doesn't get converted into a special room. Without filled,
--     the room only gets what you define in here.
--   - use type = "themed" to force a room that's never converted
--     to a special room, such as a shop or a temple.
-- core calls themerooms_generate() multiple times per level
-- to generate a single themed room.

themerooms = {
   {
     -- the "default" room
      frequency = 1000,
      contents = function()
         des.room({ type = "ordinary", filled = 1 });
         end
   },

   -- Fake Delphi
   function()
      des.room({ type = "ordinary", w = 11,h = 9, filled = 1,
                 contents = function()
                    des.room({ type = "ordinary", x = 4,y = 3, w = 3,h = 3, filled = 1,
                               contents = function()
                                  des.door({ state="random", wall="all" });
                               end
                    });
                 end
      });
   end,

   -- Room in a room
   -- FIXME: subroom location is too often left/top?
   function()
      des.room({ type = "ordinary", filled = 1,
                 contents = function()
                    des.room({ type = "ordinary",
                               contents = function()
                                  des.door({ state="random", wall="all" });
                               end
                    });
                 end
      });
   end,

   -- Huge room, with another room inside (90%)
   function()
      des.room({ type = "ordinary", w = nh.rn2(10)+11,h = nh.rn2(5)+8, filled = 1,
                 contents = function()
                    if (percent(90)) then
                    des.room({ type = "ordinary", filled = 1,
                               contents = function()
                                  des.door({ state="random", wall="all" });
                                  if (percent(50)) then
                                     des.door({ state="random", wall="all" });
                                  end
                               end
                    });
                    end
                 end
      });
   end,

   -- Ice room
   function()
      des.room({ type = "themed", filled = 1,
                 contents = function()
                    des.terrain(selection.floodfill(1,1), "I");
                 end
      });
   end,

   -- Boulder room
   {
      mindiff = 4,
      contents = function()
         des.room({ type = "themed",
                  contents = function(rm)
                     for x = 0, rm.width - 1 do
                        for y = 0, rm.height - 1 do
                           if (percent(30)) then
                              if (percent(50)) then
                                 des.object("boulder");
                              else
                                 des.trap("rolling boulder");
                              end
                           end
                        end
                     end
                  end
         });
      end
   },

   -- Spider nest
   {
      mindiff = 10,
      contents = function()
         des.room({ type = "themed",
                  contents = function(rm)
                     for x = 0, rm.width - 1 do
                        for y = 0, rm.height - 1 do
                           if (percent(30)) then
                              des.trap("web", x, y);
                           end
                        end
                     end
                  end
         });
      end
   },

   -- Trap room
   function()
      des.room({ type = "themed", filled = 0,
                 contents = function(rm)
                    local traps = { "arrow", "dart", "falling rock", "bear",
                                    "land mine", "sleep gas", "rust",
                                    "anti magic" };
                    shuffle(traps);
                    for x = 0, rm.width - 1 do
                       for y = 0, rm.height - 1 do
                          if (percent(30)) then
                             des.trap(traps[1], x, y);
                          end
                       end
                    end
                 end
      });
   end,

   -- Buried treasure
   function()
      des.room({ type = "ordinary", filled = 1,
                 contents = function()
                    des.object({ id = "chest", buried = true, contents = function()
                                    for i = 1, d(3,4) do
                                       des.object();
                                    end
                    end });
                 end
      });
   end,

   -- Massacre
   function()
      des.room({ type = "themed",
                 contents = function()
                    local mon = { "apprentice", "warrior", "ninja", "thug",
                                  "hunter", "acolyte", "abbot", "page",
                                  "attendant", "neanderthal", "chieftain",
                                  "student", "wizard", "valkyrie", "tourist",
                                  "samurai", "rogue", "ranger", "priestess",
                                  "priest", "monk", "knight", "healer",
                                  "cavewoman", "caveman", "barbarian",
                                  "archeologist" };
                    shuffle(mon);
                    for i = 1, d(5,5) do
                       if (percent(10)) then shuffle(mon); end
                       des.object({ id = "corpse", montype = mon[1] });
                    end
                 end
      });
   end,

   -- Pillars
   function()
      des.room({ type = "themed", w = 10, h = 10,
                 contents = function(rm)
                    local terr = { "-", "-", "-", "-", "L", "P", "T" };
                    shuffle(terr);
                    for x = 0, (rm.width / 4) - 1 do
                       for y = 0, (rm.height / 4) - 1 do
                          des.terrain({ x = x * 4 + 2, y = y * 4 + 2, typ = terr[1], lit = -2 });
                          des.terrain({ x = x * 4 + 3, y = y * 4 + 2, typ = terr[1], lit = -2 });
                          des.terrain({ x = x * 4 + 2, y = y * 4 + 3, typ = terr[1], lit = -2 });
                          des.terrain({ x = x * 4 + 3, y = y * 4 + 3, typ = terr[1], lit = -2 });
                       end
                    end
                 end
      });
   end,

   -- Statuary
   function()
      des.room({ type = "themed",
                 contents = function()
                    for i = 1, d(5,5) do
                       des.object({ id = "statue" });
                    end
                    for i = 1, d(3) do
                       des.trap("statue");
                    end
                 end
      });
   end,

   -- Light source
   function()
      des.room({ type = "themed", lit = 0,
                 contents = function()
                    des.object({ id = "oil lamp", lit = true });
                 end
      });
   end,

   -- Temple of the gods
   function()
      des.room({ type = "themed",
                 contents = function()
                    des.altar({ align = align[1] });
                    des.altar({ align = align[2] });
                    des.altar({ align = align[3] });
                 end
      });
   end,

   -- Mausoleum
   function()
      des.room({ type = "themed", w = 5 + nh.rn2(3)*2, h = 5 + nh.rn2(3)*2,
                 contents = function(rm)
                    des.room({ type = "themed",
			       x = (rm.width - 1) / 2, y = (rm.height - 1) / 2,
			       w = 1, h = 1, joined = 0,
                               contents = function()
                                  if (percent(50)) then
                                     local mons = { "M", "V", "L", "Z" };
                                     shuffle(mons);
                                     des.monster(mons[1], 0,0);
                                  else
                                     des.object({ id = "corpse", montype = "@", coord = {0,0} });
                                  end
                                  if (percent(20)) then
                                     des.door({ state="secret", wall="all" });
                                  end
                               end
                    });
                 end
      });
   end,

   -- Random dungeon feature in the middle of a odd-sized room
   function()
      local wid = 3 + (nh.rn2(3) * 2);
      local hei = 3 + (nh.rn2(3) * 2);
      des.room({ type = "ordinary", filled = 1, w = wid, h = hei,
                 contents = function(rm)
                    local feature = { "C", "L", "I", "P", "T" };
                    shuffle(feature);
                    des.terrain((rm.width - 1) / 2, (rm.height - 1) / 2,
				feature[1]);
                 end
      });
   end,

   -- L-shaped
   function()
      des.map({ map = [[
-----xxx
|...|xxx
|...|xxx
|...----
|......|
|......|
|......|
--------]], contents = function(m) des.region({ region={1,1,3,3}, type="ordinary", irregular=true, filled=1 }); end });
   end,

   -- L-shaped, rot 1
   function()
      des.map({ map = [[
xxx-----
xxx|...|
xxx|...|
----...|
|......|
|......|
|......|
--------]], contents = function(m) des.region({ region={5,1,5,3}, type="ordinary", irregular=true, filled=1 }); end });
   end,

   -- L-shaped, rot 2
   function()
      des.map({ map = [[
--------
|......|
|......|
|......|
----...|
xxx|...|
xxx|...|
xxx-----]], contents = function(m) des.region({ region={1,1,2,2}, type="ordinary", irregular=true, filled=1 }); end });
   end,

   -- L-shaped, rot 3
   function()
      des.map({ map = [[
--------
|......|
|......|
|......|
|...----
|...|xxx
|...|xxx
-----xxx]], contents = function(m) des.region({ region={1,1,2,2}, type="ordinary", irregular=true, filled=1 }); end });
   end,

   -- Blocked center
   function()
      des.map({ map = [[
-----------
|.........|
|.........|
|.........|
|...LLL...|
|...LLL...|
|...LLL...|
|.........|
|.........|
|.........|
-----------]], contents = function(m)
if (percent(30)) then
   local terr = { "-", "P" };
   shuffle(terr);
   des.replace_terrain({ region = {1,1, 9,9}, fromterrain = "L", toterrain = terr[1] });
end
des.region({ region={1,1,2,2}, type="ordinary", irregular=true, filled=1 });
end });
   end,

   -- Circular, small
   function()
      des.map({ map = [[
xx---xx
x--.--x
--...--
|.....|
--...--
x--.--x
xx---xx]], contents = function(m) des.region({ region={3,3,3,3}, type="ordinary", irregular=true, filled=1 }); end });
   end,

   -- Circular, medium
   function()
      des.map({ map = [[
xx-----xx
x--...--x
--.....--
|.......|
|.......|
|.......|
--.....--
x--...--x
xx-----xx]], contents = function(m) des.region({ region={4,4,4,4}, type="ordinary", irregular=true, filled=1 }); end });
   end,

   -- Circular, big
   function()
      des.map({ map = [[
xxx-----xxx
x---...---x
x-.......-x
--.......--
|.........|
|.........|
|.........|
--.......--
x-.......-x
x---...---x
xxx-----xxx]], contents = function(m) des.region({ region={5,5,5,5}, type="ordinary", irregular=true, filled=1 }); end });
   end,

   -- T-shaped
   function()
      des.map({ map = [[
xxx-----xxx
xxx|...|xxx
xxx|...|xxx
----...----
|.........|
|.........|
|.........|
-----------]], contents = function(m) des.region({ region={5,5,5,5}, type="ordinary", irregular=true, filled=1 }); end });
   end,

   -- T-shaped, rot 1
   function()
      des.map({ map = [[
-----xxx
|...|xxx
|...|xxx
|...----
|......|
|......|
|......|
|...----
|...|xxx
|...|xxx
-----xxx]], contents = function(m) des.region({ region={2,2,2,2}, type="ordinary", irregular=true, filled=1 }); end });
   end,

   -- T-shaped, rot 2
   function()
      des.map({ map = [[
-----------
|.........|
|.........|
|.........|
----...----
xxx|...|xxx
xxx|...|xxx
xxx-----xxx]], contents = function(m) des.region({ region={2,2,2,2}, type="ordinary", irregular=true, filled=1 }); end });
   end,

   -- T-shaped, rot 3
   function()
      des.map({ map = [[
xxx-----
xxx|...|
xxx|...|
----...|
|......|
|......|
|......|
----...|
xxx|...|
xxx|...|
xxx-----]], contents = function(m) des.region({ region={5,5,5,5}, type="ordinary", irregular=true, filled=1 }); end });
   end,

   -- S-shaped
   function()
      des.map({ map = [[
-----xxx
|...|xxx
|...|xxx
|...----
|......|
|......|
|......|
----...|
xxx|...|
xxx|...|
xxx-----]], contents = function(m) des.region({ region={2,2,2,2}, type="ordinary", irregular=true, filled=1 }); end });
   end,

   -- S-shaped, rot 1
   function()
      des.map({ map = [[
xxx--------
xxx|......|
xxx|......|
----......|
|......----
|......|xxx
|......|xxx
--------xxx]], contents = function(m) des.region({ region={5,5,5,5}, type="ordinary", irregular=true, filled=1 }); end });
   end,

   -- Z-shaped
   function()
      des.map({ map = [[
xxx-----
xxx|...|
xxx|...|
----...|
|......|
|......|
|......|
|...----
|...|xxx
|...|xxx
-----xxx]], contents = function(m) des.region({ region={5,5,5,5}, type="ordinary", irregular=true, filled=1 }); end });
   end,

   -- Z-shaped, rot 1
   function()
      des.map({ map = [[
--------xxx
|......|xxx
|......|xxx
|......----
----......|
xxx|......|
xxx|......|
xxx--------]], contents = function(m) des.region({ region={2,2,2,2}, type="ordinary", irregular=true, filled=1 }); end });
   end,

   -- Cross
   function()
      des.map({ map = [[
xxx-----xxx
xxx|...|xxx
xxx|...|xxx
----...----
|.........|
|.........|
|.........|
----...----
xxx|...|xxx
xxx|...|xxx
xxx-----xxx]], contents = function(m) des.region({ region={6,6,6,6}, type="ordinary", irregular=true, filled=1 }); end });
   end,

   -- Four-leaf clover
   function()
      des.map({ map = [[
-----x-----
|...|x|...|
|...---...|
|.........|
---.....---
xx|.....|xx
---.....---
|.........|
|...---...|
|...|x|...|
-----x-----]], contents = function(m) des.region({ region={6,6,6,6}, type="ordinary", irregular=true, filled=1 }); end });
   end,

   -- Water-surrounded vault
   function()
      des.map({ map = [[
}}}}}}
}----}
}|..|}
}|..|}
}----}
}}}}}}]], contents = function(m) des.region({ region={3,3,3,3}, type="themed", irregular=true, filled=0, joined=false });
     local nasty_undead = { "giant zombie", "ettin zombie", "vampire lord" };
     des.object("chest", 2, 2);
     des.object("chest", 3, 2);
     des.object("chest", 2, 3);
     des.object("chest", 3, 3);
     shuffle(nasty_undead);
     des.monster(nasty_undead[1], 2, 2);
end });
   end,

};

function is_eligible(room)
   local t = type(room);
   local diff = nh.level_difficulty();
   if (t == "table") then
      if (room.mindiff ~= nil and diff < room.mindiff) then
         return false
      elseif (room.maxdiff ~= nil and diff > room.maxdiff) then
         return false
      end
   elseif (t == "function") then
      -- functions currently have no constraints
   end
   return true
end

function themerooms_generate()
   local pick = 1;
   local total_frequency = 0;
   for i = 1, #themerooms do
      -- Reservoir sampling: select one room from the set of eligible rooms,
      -- which may change on different levels because of level difficulty.
      if is_eligible(themerooms[i]) then
         local this_frequency;
         if (type(themerooms[i]) == "table" and themerooms[i].frequency ~= nil) then
            this_frequency = themerooms[i].frequency;
         else
            this_frequency = 1;
         end
         total_frequency = total_frequency + this_frequency;
         -- avoid rn2(0) if a room has freq 0
         if this_frequency > 0 and nh.rn2(total_frequency) < this_frequency then
            pick = i;
         end
      end
   end

   local t = type(themerooms[pick]);
   if (t == "table") then
      themerooms[pick].contents();
   elseif (t == "function") then
      themerooms[pick]();
   end
end
