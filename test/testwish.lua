
local wishtest_objects = {
   ["a rock"] = { otyp_name = "rock", quan = 1, oclass = "*" },
   ["a gold piece"] = { otyp_name = "gold piece", quan = 1, oclass = "$" },
   ["2 zorkmids"] = { otyp_name = "gold piece", quan = 2, oclass = "$" },
   ["2 worthless pieces of red glass"] = { otyp_name = "worthless piece of red glass", quan = 2, oclass = "*" },
   ["red glass"] = { otyp_name = "worthless piece of red glass", quan = 1, oclass = "*" },
   ["red gem"] = { otyp_descr = "red", oclass = "*" },
   ["orange gem"] = { otyp_descr = "orange", oclass = "*" },
   ["5 +3 blessed silver daggers"] = { otyp_name = "silver dagger", blessed = 1, cursed = 0, spe = 3, quan = 5 },
   ["an empty locked large box"] = { otyp_name = "large box", is_container = 1, has_contents = 0, olocked = 1 },
   ["an empty trapped unlocked chest"] = { otyp_name = "chest", is_container = 1, has_contents = 0, olocked = 0,  otrapped = 1 },
   ["an empty untrapped locked chest"] = { otyp_name = "chest", is_container = 1, has_contents = 0, olocked = 1,  otrapped = 0 },
   ["an empty locked broken chest"] = { otyp_name = "chest", is_container = 1, has_contents = 0, olocked = 0,  obroken = 1 },
   ["broken empty chest"] = { otyp_name = "chest", obroken = 1 },
   ["potion of holy water"] = { otyp_name = "water", oclass = "!", blessed = 1, cursed = 0 },
   ["potion of unholy water"] = { otyp_name = "water", oclass = "!", blessed = 0, cursed = 1 },
   ["cursed greased +2 grey dragon scale mail"] = { otyp_name = "gray dragon scale mail", oclass = "[", blessed = 0, cursed = 1, spe = 2, greased = 1 },
   ["+1 yellow dragon scales"] = { otyp_name = "yellow dragon scales", oclass = "[", spe = 1 },
   ["uncursed magic marker (11)"] = { otyp_name = "magic marker", blessed = 0, cursed = 0, spe = 11 },
   ["wand of locking (1:5)"] = { otyp_name = "locking", oclass = "/", recharged = 1, spe = 5 },
   ["wand of opening (0:4)"] = { otyp_name = "opening", oclass = "/", recharged = 0, spe = 4 },
   ["lit oil lamp"] = { otyp_name = "oil lamp", lamplit = 1 },
   ["oil lamp (lit)"] = { otyp_name = "oil lamp", lamplit = 1 },
   ["6 burning tallow candles"] = { otyp_name = "tallow candle", lamplit = 1, quan = 6 },
   ["unlit oil lamp"] = { otyp_name = "oil lamp", lamplit = 0 },
   ["7 extinguished wax candles"] = { otyp_name = "wax candle", lamplit = 0, quan = 7 },
   ["2 blank scrolls"] = { otyp_name = "blank paper", quan = 2 },
   ["3 unlabeled scrolls"] = { otyp_name = "blank paper", quan = 3 },
   ["1 unlabelled scroll"] = { otyp_name = "blank paper", quan = 1 },
   ["blank spellbook"] = { otyp_name = "blank paper", oclass = "+" },
   ["unlabeled spellbook"] = { otyp_name = "blank paper", oclass = "+" },
   ["unlabelled spellbook"] = { otyp_name = "blank paper", oclass = "+" },
   ["3 rusty poisoned darts"] = { otyp_name = "dart", quan = 3, opoisoned = 1, oeroded = 1 },
   ["4 diluted dark green potions named whisky"] = { otyp_descr = "dark green", oclass = "!", quan = 4, odiluted = 1, has_oname = 1, oname = "whisky" },
   ["poisoned food ration"] = { otyp_name = "food ration", oclass = "%", age = 1 },
   ["empty tin"] = { otyp_name = "tin", oclass = "%", corpsenm = -1, spe = 0 },
   ["blessed tin of spinach"] = { otyp_name = "tin", oclass = "%", corpsenm = -1, spe = 1, blessed = 1 },
   ["spinach"] = { otyp_name = "tin", oclass = "%", corpsenm = -1, spe = 1 },
   ["trapped tin of floating eye meat"] = { otyp_name = "tin", oclass = "%", otrapped = 1, corpsenm_name = "floating eye" },
   ["hill orc corpse"] = { otyp_name = "corpse", oclass = "%", corpsenm_name = "hill orc" },
   -- TODO: zombifying and other timers cannot be seen via lua
   ["zombifying elf corpse"] = { otyp_name = "corpse", oclass = "%", corpsenm_name = "elf" },
   ["destroy armor"] = { otyp_name = "destroy armor", oclass = "?" },
   ["enchant weapon"] = { otyp_name = "enchant weapon", oclass = "?" },
   ["scroll of food detection"] = { otyp_name = "food detection", oclass = "?" },
   ["scroll of detect food"] = { otyp_name = "food detection", oclass = "?" },
   ["2 scrolls of charging"] = { otyp_name = "charging", oclass = "?", quan = 2 },
   ["spellbook of food detection"] = { otyp_name = "detect food", oclass = "+" },
   ["spell"] = { NO_OBJ = 1 },
   ["-1 ring mail"] = { otyp_name = "ring mail", oclass = "[", spe = -1 },
   ["studded leather armor"] = { otyp_name = "studded leather armor", oclass = "[" },
   ["leather armor"] = { otyp_name = "leather armor", oclass = "[" },
   ["plate armor"] = { otyp_name = "plate mail", oclass = "[" },
   ["yellow dragon scale armor"] = { otyp_name = "yellow dragon scale mail", oclass = "[" },
   ["speed boots"] = { otyp_name = "speed boots", oclass = "[" },
   ["speedboots"] = { otyp_name = "speed boots", oclass = "[" },
   ["erodeproof speedboots"] = { otyp_name = "speed boots", oclass = "[", oerodeproof = 1 },
   ["fixed boots of speed"] = { otyp_name = "speed boots", oclass = "[", oerodeproof = 1 },
   ["blessed fireproof +2 pair of speed boots"] = { otyp_name = "speed boots", oclass = "[", oerodeproof = 1, blessed = 1, spe = 2 },
   ["tooled horn"] = { otyp_name = "tooled horn", oclass = "(" },
   ["meat ring"] = { otyp_name = "meat ring", oclass = "%" },
   ["cursed +3 ring of increase accuracy"] = { otyp_name = "increase accuracy", oclass = "=", spe = 3, cursed = 1 },
   ["ring of accuracy"] = { otyp_name = "increase accuracy", oclass = "=" },
   ["accuracy"] = { otyp_name = "increase accuracy", oclass = "=" },
   ["beartrap"] = { otyp_name = "beartrap", oclass = "(" },
   ["bear trap"] = { otyp_name = "beartrap", oclass = "(" },
   ["landmine"] = { otyp_name = "land mine", oclass = "(" },
   ["land mine"] = { otyp_name = "land mine", oclass = "(" },
   ["bag of tricks"] = { otyp_name = "bag of tricks", oclass = "(" },
   ["bags of tricks"] = { otyp_name = "bag of tricks", oclass = "(" },
   ["sprig of wolfsbane"] = { otyp_name = "sprig of wolfsbane", oclass = "%" },
   ["wolfsbane"] = { otyp_name = "sprig of wolfsbane", oclass = "%" },
   ["clove of garlic"] = { otyp_name = "clove of garlic", oclass = "%" },
   ["garlic"] = { otyp_name = "clove of garlic", oclass = "%" },
   ["lump of royal jelly"] = { otyp_name = "lump of royal jelly", oclass = "%" },
   ["royal jelly"] = { otyp_name = "lump of royal jelly", oclass = "%" },
   ["blessed historic statue of woodland-elf named Foo"] = { otyp_name = "statue", blessed = 1, historic = 1, corpsenm_name = "Woodland-elf", oname = "Foo" },
   ["blessed figurine of a ki-rin"] = { otyp_name = "figurine", blessed = 1, corpsenm_name = "ki-rin" },
   ["partly eaten orange"] = { otyp_name = "orange", oclass = "%", oeaten = function ()
                                  local oc = obj.class(obj.new("orange"));
                                  return (oc.nutrition // 2);
                             end },
};

for str, tbl in pairs(wishtest_objects) do
   local o = obj.new(str):totable();
   for field, value in pairs(tbl) do
      local val;
      if (type(value) == "function") then
         val = value();
      else
         val = value;
      end
      if (o[field] ~= val) then
         error("wished " .. str .. ", but " .. field .. " is " .. o[field] .. ", not " .. val);
      end
   end
end
pline("testwish: OK");
